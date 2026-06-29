#include "FixModelByWin10.hpp"
#include "libslic3r/Exception.hpp"

#ifndef HAS_WIN10SDK

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <exception>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#ifdef HAS_IMATISTL
#include <imatistl.h>
#endif

#include "libslic3r/Format/STL.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/ModelObject.hpp"
#include "libslic3r/ModelVolume.hpp"
#include "libslic3r/Print.hpp"
#include "../GUI/I18N.hpp"

#include <wx/msgdlg.h>
#include <wx/progdlg.h>

namespace Slic3r {

class RepairCanceledException : public std::exception {
public:
    const char* what() const throw() override { return "Model repair has been canceled"; }
};

struct RepairProgressState {
    std::string message;
    int         percent = 0;
    bool        updated = false;
};

#ifdef HAS_IMATISTL
struct RepairMessageBridge {
    std::mutex*              mutex = nullptr;
    std::condition_variable* condition = nullptr;
    RepairProgressState*     progress = nullptr;
    std::atomic<bool>*       canceled = nullptr;
};

static thread_local RepairMessageBridge* s_repair_message_bridge = nullptr;
#endif

static std::string normalize_progress_message(const char* msg)
{
    if (msg == nullptr)
        return {};

    std::string text = msg;
    const char* whitespace = " \t\r\n\f\v";
    const size_t begin = text.find_first_not_of(whitespace);
    if (begin == std::string::npos)
        return {};
    const size_t end = text.find_last_not_of(whitespace);
    text = text.substr(begin, end - begin + 1);

    const char* prefixes[] = {"INFO- ", "WARNING- ", "ERROR- "};
    for (const char* prefix : prefixes) {
        const size_t prefix_len = std::strlen(prefix);
        if (text.compare(0, prefix_len, prefix) == 0) {
            text.erase(0, prefix_len);
            break;
        }
    }

    return text;
}

#ifdef HAS_IMATISTL
static void imatistl_display_message(const char* msg, int action)
{
    if (s_repair_message_bridge == nullptr)
        return;

    if (s_repair_message_bridge->canceled != nullptr && *(s_repair_message_bridge->canceled))
        throw RepairCanceledException();

    if (action != DISPMSG_ACTION_PUTPROGRESS &&
        action != DISPMSG_ACTION_PUTMESSAGE &&
        action != DISPMSG_ACTION_ERRORDIALOG)
        return;

    std::string text = normalize_progress_message(msg);
    if (text.empty())
        return;

    if (action == DISPMSG_ACTION_ERRORDIALOG)
    {
        const uint64_t trace_id = (uint64_t)std::chrono::steady_clock::now().time_since_epoch().count();
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << boost::format(" ImatiSTL error: %1%") % text << " tid=" << std::this_thread::get_id() << " rid=" << trace_id;
        boost::log::core::get()->flush();
        throw Slic3r::RuntimeError(L("Repair failed."));
    }

    std::unique_lock<std::mutex> lock(*s_repair_message_bridge->mutex);
    s_repair_message_bridge->progress->message = std::move(text);
    s_repair_message_bridge->progress->updated = true;
    s_repair_message_bridge->condition->notify_all();
}
#endif

using RepairProgressFn = std::function<void(const char*, unsigned)>;
using RepairThrowIfCanceledFn = std::function<void()>;

inline bool is_windows10() { return false; }

bool has_mesh_repair_backend()
{
#ifdef HAS_IMATISTL
    return true;
#else
    return false;
#endif
}

static void fix_model_by_imati_stl(TriangleMesh&                  mesh,
                                   const RepairProgressFn&        on_progress = {},
                                   const RepairThrowIfCanceledFn& throw_if_canceled = {})
{
    const uint64_t trace_id = (uint64_t)std::chrono::steady_clock::now().time_since_epoch().count();

#ifdef HAS_IMATISTL
    if (mesh.empty())
        return;

    auto report_progress = [&on_progress, &throw_if_canceled](const char* message, unsigned percent) {
        if (throw_if_canceled)
            throw_if_canceled();
        if (on_progress)
            on_progress(message, percent);
        if (throw_if_canceled)
            throw_if_canceled();
    };

    auto to_imatistl_mesh = [](const TriangleMesh& src, IMATI_STL::TriMesh& dst) -> bool {
        std::vector<T_MESH::ExtVertex*> ext_vertices;
        ext_vertices.reserve(src.its.vertices.size());

        for (const auto& vertex : src.its.vertices) {
            auto* im_vertex = dst.newVertex(vertex.x(), vertex.y(), vertex.z());
            dst.V.appendTail(im_vertex);
            ext_vertices.emplace_back(new T_MESH::ExtVertex(im_vertex));
        }

        bool has_triangle = false;
        for (const auto& face : src.its.indices) {
            const int i0 = face[0];
            const int i1 = face[1];
            const int i2 = face[2];
            if (i0 < 0 || i1 < 0 || i2 < 0 ||
                i0 >= int(src.its.vertices.size()) ||
                i1 >= int(src.its.vertices.size()) ||
                i2 >= int(src.its.vertices.size()) ||
                i0 == i1 || i1 == i2 || i0 == i2)
                continue;

            if (dst.CreateIndexedTriangle(ext_vertices.data(), i0, i1, i2) != nullptr)
                has_triangle = true;
        }

        for (auto* ext_vertex : ext_vertices)
            delete ext_vertex;

        return has_triangle && dst.T.numels() > 0;
    };

    auto from_imatistl_mesh = [](IMATI_STL::TriMesh& src, TriangleMesh& dst) -> bool {
        std::vector<Vec3f>   vertices;
        std::vector<Vec3i32> faces;
        vertices.reserve(size_t(src.V.numels()));
        faces.reserve(size_t(src.T.numels()));

        std::unordered_map<T_MESH::Vertex*, int> vertex_indices;
        vertex_indices.reserve(size_t(src.V.numels()));

        for (T_MESH::Node* node = src.V.head(); node != nullptr; node = node->next()) {
            auto* vertex = static_cast<T_MESH::Vertex*>(node->data);
            if (!vertex->isLinked())
                continue;

            const int index = int(vertices.size());
            vertex_indices.emplace(vertex, index);
            vertices.emplace_back(
                float(TMESH_TO_DOUBLE(vertex->x)),
                float(TMESH_TO_DOUBLE(vertex->y)),
                float(TMESH_TO_DOUBLE(vertex->z)));
        }

        for (T_MESH::Node* node = src.T.head(); node != nullptr; node = node->next()) {
            auto* triangle = static_cast<T_MESH::Triangle*>(node->data);
            if (!triangle->isLinked())
                continue;

            auto it0 = vertex_indices.find(triangle->v1());
            auto it1 = vertex_indices.find(triangle->v2());
            auto it2 = vertex_indices.find(triangle->v3());
            if (it0 == vertex_indices.end() || it1 == vertex_indices.end() || it2 == vertex_indices.end())
                continue;
            if (it0->second == it1->second || it1->second == it2->second || it0->second == it2->second)
                continue;

            faces.emplace_back(it0->second, it1->second, it2->second);
        }

        if (faces.empty())
            return false;

        dst = TriangleMesh(std::move(vertices), std::move(faces));
        return true;
    };

    report_progress(L("Initializing ImatiSTL backend"), 2);
    IMATI_STL::ImatiSTL::init(&imatistl_display_message);

    report_progress(L("Converting model to ImatiSTL mesh"), 8);
    IMATI_STL::TriMesh tin;
    if (!to_imatistl_mesh(mesh, tin)) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << boost::format(" Convert TriangleMeshto ImatiSTL mesh failed.") << " tid=" << std::this_thread::get_id() << " rid=" << trace_id;
        boost::log::core::get()->flush();
        throw Slic3r::RuntimeError(L("Repair failed."));
    }

    report_progress(L("Rebuilding mesh connectivity"), 20);
    tin.rebuildConnectivity();

    report_progress(L("Checking mesh boundaries"), 40);
    const bool has_boundaries = tin.boundaries();
    if (has_boundaries) {
        report_progress(L("Filling mesh boundaries"), 55);
        tin.fillSmallBoundaries(tin.E.numels());
    } else {
        report_progress(L("No open boundaries detected"), 55);
    }

    report_progress(L("Computing outer hull"), 75);
    tin.computeOuterHull();

    report_progress(L("Converting repaired mesh back"), 90);
    TriangleMesh repaired;
    if (!from_imatistl_mesh(tin, repaired)) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << boost::format(" Convert repaired ImatiSTL mesh back to TriangleMesh failed.") << " tid=" << std::this_thread::get_id() << " rid=" << trace_id;
        boost::log::core::get()->flush();
        throw Slic3r::RuntimeError(L("Repair failed."));
    }

    report_progress(L("Finalizing repaired mesh"), 98);
    mesh = std::move(repaired);
    report_progress(L("Repair finished"), 100);
    return;
#else
    (void)mesh;
    (void)on_progress;
    (void)throw_if_canceled;

    BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << boost::format(" ImatiSTL backend is not available in this build.") << " tid=" << std::this_thread::get_id() << " rid=" << trace_id;
    boost::log::core::get()->flush();
    throw Slic3r::RuntimeError(L("Repair failed."));

    return;
#endif
}

bool fix_model_by_imati_stl_gui(ModelObject& model_object, int volume_idx,
                                GUI::ProgressDialog& progress_dialog, const wxString& msg_header,
                                std::string& fix_result)
{
    const uint64_t trace_id = (uint64_t)std::chrono::steady_clock::now().time_since_epoch().count();

    if (!has_mesh_repair_backend()) {
        fix_result = L("Mesh repair backend is not available.");
        return true;
    }

    std::vector<ModelVolume*> volumes;
    if (volume_idx == -1)
        volumes = model_object.volumes;
    else if (volume_idx >= 0 && volume_idx < int(model_object.volumes.size()))
        volumes.emplace_back(model_object.volumes[volume_idx]);

    if (volumes.empty()) {
        fix_result.clear();
        return true;
    }

    std::mutex              mtx;
    std::condition_variable condition;
    RepairProgressState     progress;

    std::atomic<bool> canceled = false;
    std::atomic<bool> finished = false;
    bool              success  = false;
    size_t            ivolume  = 0;

    auto on_progress = [&mtx, &condition, &ivolume, &volumes, &progress](const char* msg, unsigned prcnt) {
        std::unique_lock<std::mutex> lock(mtx);
        progress.message = msg != nullptr ? msg : "";
        progress.percent = int((double(prcnt) + double(ivolume) * 100.0) / double(volumes.size()));
        progress.updated = true;
        condition.notify_all();
    };

    auto worker_thread = std::thread([&model_object, &volumes, &ivolume, &on_progress, &success, &canceled, &finished, &condition, &mtx, &progress, &trace_id]() {
        RepairMessageBridge bridge;
        try {
            bridge.canceled  = &canceled;
            bridge.condition = &condition;
            bridge.mutex     = &mtx;
            bridge.progress  = &progress;
            s_repair_message_bridge = &bridge;

            std::vector<TriangleMesh> meshes_repaired;
            meshes_repaired.reserve(volumes.size());

            for (; ivolume < volumes.size(); ++ivolume) {
                on_progress(L("Preparing mesh repair"), 0);

                TriangleMesh mesh = volumes[ivolume]->mesh();
                fix_model_by_imati_stl(mesh, on_progress,
                    [&canceled]() { if (canceled) throw RepairCanceledException(); });

                meshes_repaired.emplace_back(std::move(mesh));
            }

            for (size_t i = 0; i < volumes.size(); ++i) {
                volumes[i]->set_mesh(std::move(meshes_repaired[i]));
                volumes[i]->calculate_convex_hull();
                volumes[i]->invalidate_convex_hull_2d();
                volumes[i]->set_new_unique_id();
            }

            model_object.invalidate_bounding_box();
            if (ivolume > 0)
                --ivolume;
            on_progress(L("Repair finished"), 100);
            success  = true;
            finished = true;
        } catch (RepairCanceledException&) {
            canceled = true;
            finished = true;
            on_progress(L("Repair canceled"), 100);
        } catch (const std::exception& ex) {
            success = false;
            finished = true;
            on_progress(ex.what(), 100);
            BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << boost::format(" exception: %1%") % ex.what() << " tid=" << std::this_thread::get_id() << " rid=" << trace_id;
            boost::log::core::get()->flush();
        } catch (...) {
            success  = false;
            finished = true;
            on_progress(L("Unknown error during mesh repair."), 100);
        }
        s_repair_message_bridge = nullptr;
    });

    while (!finished) {
        std::unique_lock<std::mutex> lock(mtx);
        condition.wait_for(lock, std::chrono::milliseconds(250), [&progress] { return progress.updated; });
        if (!progress_dialog.Update(progress.percent - 1, msg_header + wxString::FromUTF8(progress.message.c_str())))
            canceled = true;
        else
            progress_dialog.Fit();
        progress.updated = false;
    }

    worker_thread.join();

    if (canceled) {
        // Nothing to show.
    } else if (success) {
        fix_result.clear();
    } else {
        fix_result = progress.message;
    }
    return !canceled;
}

inline bool fix_model_by_win10_sdk_gui(ModelObject& model_object, int volume_idx, GUI::ProgressDialog& progress_dialog,
                                const wxString& msg_header, std::string& fix_result)
{
    return false;
}

} // namespace Slic3r

#endif

namespace Slic3r {

bool fix_model(ModelObject& model_object, int volume_idx, GUI::ProgressDialog& progress_dialog,
               const wxString& msg_header, std::string& fix_result)
{
#ifdef HAS_WIN10SDK
    return fix_model_by_win10_sdk_gui(model_object, volume_idx, progress_dialog, msg_header, fix_result);
#else
    return fix_model_by_imati_stl_gui(model_object, volume_idx, progress_dialog, msg_header, fix_result);
#endif
}

} // namespace Slic3r
