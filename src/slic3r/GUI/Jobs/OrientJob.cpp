#include "OrientJob.hpp"

#include "libslic3r/Model.hpp"
#include "libslic3r/ModelInstance.hpp"
#include "libslic3r/ModelObject.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/GUI.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/NotificationManager.hpp"
#include "slic3r/GUI/PartPlate.hpp"
#include "slic3r/GUI/simple/MCPChatPanel.hpp"
#include "slic3r/GUI/simple/gpu/GpuOrient.hpp"
#include "libslic3r/PresetBundle.hpp"

namespace Slic3r { namespace GUI {


void OrientJob::clear_input()
{
    const Model &model = m_plater->model();

    size_t count = 0, cunprint = 0; // To know how much space to reserve
    for (auto obj : model.objects)
        for (auto mi : obj->instances)
            mi->printable ? count++ : cunprint++;

    m_selected.clear();
    m_unselected.clear();
    m_unprintable.clear();
    m_selected.reserve(count);
    m_unselected.reserve(count);
    m_unprintable.reserve(cunprint);
}

//BBS: add only one plate mode and lock logic
void OrientJob::prepare_selection(std::vector<bool> obj_sel, bool only_one_plate)
{
    Model& model = m_plater->model();
    PartPlateList& plate_list = m_plater->get_partplate_list();
    //OrientMeshs selected_in_lock, unselect_in_lock;
    bool selected_is_locked = false;

    // Go through the objects and check if inside the selection
    for (size_t oidx = 0; oidx < obj_sel.size(); ++oidx) {
        bool selected = obj_sel[oidx];
        ModelObject* mo = model.objects[oidx];

        for (size_t inst_idx = 0; inst_idx < mo->instances.size(); ++inst_idx)
        {
            ModelInstance* mi = mo->instances[inst_idx];
            OrientMesh&& om = create_orientation_input(mi, oidx, inst_idx);

            bool locked = false;
            if (!only_one_plate) {
                int plate_index = plate_list.find_instance(oidx, inst_idx);
                if ((plate_index >= 0)&&(plate_index < plate_list.get_plate_count())) {
                    if (plate_list.is_locked(plate_index)) {
                        if (selected) {
                            //selected_in_lock.emplace_back(std::move(om));
                            selected_is_locked = true;
                        }
                        //else
                        //    unselect_in_lock.emplace_back(std::move(om));
                        continue;
                    }
                }
            }
            auto& cont = mo->printable ? (selected ? m_selected : m_unselected) : m_unprintable;

            cont.emplace_back(std::move(om));
        }
    }

    // If the selection was empty orient everything
    if (m_selected.empty()) {
        if (!selected_is_locked) {
            m_selected.swap(m_unselected);
            //m_unselected.insert(m_unselected.begin(), unselect_in_lock.begin(), unselect_in_lock.end());
        }
        else {
            m_plater->get_notification_manager()->push_notification(NotificationType::BBLPlateInfo,
                NotificationManager::NotificationLevel::WarningNotificationLevel, into_u8(_L("All the selected objects are on the locked plate,\nWe can not do auto-orient on these objects.")));
        }
    }
}

void OrientJob::prepare_selected() {
    clear_input();

    Model &model = m_plater->model();

    std::vector<bool> obj_sel(model.objects.size(), false);

    for (auto &s : m_plater->get_selection().get_content())
        if (s.first < int(obj_sel.size()))
            obj_sel[size_t(s.first)] = !s.second.empty();

   //BBS: add only one plate mode
    prepare_selection(obj_sel, false);
}

//BBS: prepare current part plate for orienting
void OrientJob::prepare_partplate() {
    clear_input();

    PartPlateList& plate_list = m_plater->get_partplate_list();
    PartPlate* plate = plate_list.get_curr_plate();
    assert(plate != nullptr);

    if (plate->empty())
    {
        //no instances on this plate
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": no instances in current plate!");

        return;
    }

    if (plate->is_locked()) {
        m_plater->get_notification_manager()->push_notification(NotificationType::BBLPlateInfo,
            NotificationManager::NotificationLevel::WarningNotificationLevel, into_u8(_L("This plate is locked,\nWe can not do auto-orient on this plate.")));
        return;
    }

    Model& model = m_plater->model();

    std::vector<bool> obj_sel(model.objects.size(), false);

    // Go through the objects and check if inside the selection
    for (size_t oidx = 0; oidx < model.objects.size(); ++oidx)
    {
        ModelObject* mo = model.objects[oidx];
        for (size_t inst_idx = 0; inst_idx < mo->instances.size(); ++inst_idx)
        {
            obj_sel[oidx] = plate->contain_instance(oidx, inst_idx);
        }
    }

    prepare_selection(obj_sel, true);
}

//BBS: add partplate logic
void OrientJob::prepare()
{
    int state = m_plater->get_prepare_state();
    m_plater->get_notification_manager()->bbl_close_plateinfo_notification();
    if (state == Job::JobPrepareState::PREPARE_STATE_DEFAULT) {
        // only_on_partplate = false;
        prepare_selected();
    }
    else if (state == Job::JobPrepareState::PREPARE_STATE_MENU) {
        // only_on_partplate = true;   // only arrange items on current plate
        prepare_partplate();
    }
}

void OrientJob::process(Ctl &ctl)
{
    static const auto arrangestr = _u8L("Orienting...");

    ctl.update_status(0, arrangestr);
    ctl.call_on_main_thread([this]{ prepare(); }).wait();;

    auto start = std::chrono::steady_clock::now();

    const GLCanvas3D::OrientSettings& settings = m_plater->canvas3D()->get_orient_settings();

    orientation::OrientParams params;
    orientation::OrientParamsArea params_area;
    if (settings.min_area) {
        memcpy(&params, &params_area, sizeof(params));
    }

    if (settings.min_volume)
        params.orient_type = orientation::MinVolume;
    else if (settings.min_time)
        params.orient_type = orientation::MinTime;
    else
        params.orient_type = orientation::MinArea;

    auto count = unsigned(m_selected.size() + m_unprintable.size());
    params.stopcondition = [&ctl]() { return ctl.was_canceled(); };

    params.progressind = [this, count, &ctl](unsigned st, std::string orientstr) {
        st += m_unprintable.size();
        if (st > 0) ctl.update_status(int(st / float(count) * 100), _u8L("Orienting") + " " + orientstr);
    };

    // Prefer the platform GPU compute implementation. GpuOrient preserves the
    // existing CPU behavior when the GPU path is unavailable or fails.
    static orientation::GpuOrient gpu_orienter;
    const bool gpu_available = gpu_orienter.available();
    std::string orient_error;
    const bool orient_ok = gpu_orienter.orient(
        m_selected,
        m_unselected,
        params,
        /*fallback_to_cpu=*/true,
        &orient_error);

    if (!orient_ok && !ctl.was_canceled()) {
        BOOST_LOG_TRIVIAL(error) << "OrientJob: auto orient failed: " << orient_error;
    } else if (orient_ok) {
        BOOST_LOG_TRIVIAL(info)
            << "OrientJob: auto orient completed via "
            << (gpu_available && orient_error.empty() ? "GPU" : "CPU fallback")
            << (orient_error.empty() ? "" : ", GPU error: " + orient_error);
    }

    auto time_elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start);

    std::stringstream ss;
    if (!m_selected.empty())
        ss << std::fixed << std::setprecision(3) << "Orient " << m_selected.back().name << " in " << time_elapsed.count() << " seconds. "
        << "Orientation: " << m_selected.back().orientation.transpose() << "; v,phi: " << m_selected.back().axis.transpose() << ", " << m_selected.back().angle << "; euler: " << m_selected.back().euler_angles.transpose();

    // finalize just here.
    ctl.update_status(100,
        ctl.was_canceled() ? _u8L("Orienting canceled.")
        : _u8L(ss.str().c_str()));
    wxGetApp().plater()->show_status_message(ctl.was_canceled() ? "Orienting canceled." : ss.str());
}

OrientJob::OrientJob() : m_plater{wxGetApp().plater()} {}

void OrientJob::finalize(bool canceled, std::exception_ptr &eptr)
{
    auto report_orient_completion = [&](bool success,
                                        const std::string& completion_message,
                                        const nlohmann::json& details = nlohmann::json::object()) {
        if (wxGetApp().easy_mode()) {
            if (auto* panel = GetEmbeddedAIChatPanel())
                panel->CompletePendingAsyncToolCall("job:auto_orient", success, completion_message, details);
        }
    };

    try {
        if (eptr)
            std::rethrow_exception(eptr);
        eptr = nullptr;
    } catch (...) {
        eptr = std::current_exception();
    }

    if (canceled || eptr) {
        report_orient_completion(
            false,
            canceled ? std::string("Auto orient canceled") : std::string("Auto orient failed before finalize"),
            {{"code", canceled ? "AUTO_ORIENT_CANCELED" : "AUTO_ORIENT_FINALIZE_FAILED"}, {"canceled", canceled}, {"has_exception", static_cast<bool>(eptr)}});
        return;
    }

    for (OrientMesh& mesh : m_selected)
    {
        mesh.apply();
    }

    m_plater->update();

    report_orient_completion(
        true,
        "Auto orient completed",
        {{"oriented_count", m_selected.size()}, {"unprintable_count", m_unprintable.size()}});

    // BBS
    //wxGetApp().obj_manipul()->set_dirty();
}

namespace {

// Canonical tilt with no Z rotation. At a vertical X axis, fix roll to zero
// rather than deriving it from roundoff, so the remaining heading is stable.
Vec3d orientation_tilt(const Matrix3d& rotation)
{
    const double cos_pitch = std::hypot(rotation(2, 1), rotation(2, 2));
    return Vec3d(cos_pitch < 1e-10 ? 0.0 : std::atan2(rotation(2, 1), rotation(2, 2)),
                 std::atan2(-rotation(2, 0), cos_pitch), 0.0);
}

void apply_orientation_transform(ModelInstance*                  instance,
                                 size_t                          object_index,
                                 size_t                          instance_index,
                                 const orientation::OrientMesh& orientation_input)
{
    ModelObject* object = instance->get_object();
    // Apply an absolute tilt in the unrotated instance frame. Composing
    // successive shortest-arc rotations can accumulate Z twist across modes.
    // In-plane heading does not affect the orientation objective.
    const Matrix3d current = instance->get_transformation().get_rotation_matrix().linear();
    const Matrix3d tilt = Geometry::rotation_transform(orientation_tilt(current)).linear();
    const Matrix3d heading = current * tilt.transpose();
    Vec3d rotation = orientation_tilt(orientation_input.rotation_matrix);
    rotation.z() = std::atan2(heading(1, 0), heading(0, 0));
    instance->set_rotation(rotation);
    object->invalidate_bounding_box();
    object->ensure_on_bed();

    Plater* plater = wxGetApp().plater();
    if (plater == nullptr)
        return;

    PartPlateList& plate_list  = plater->get_partplate_list();
    const int      plate_index = plate_list.find_instance(static_cast<int>(object_index),
                                                          static_cast<int>(instance_index));
    if (plate_index < 0)
        return;

    PartPlate*     plate       = plate_list.get_plate(plate_index);
    if (plate == nullptr || !plate->check_outside(static_cast<int>(object_index),
                                                  static_cast<int>(instance_index)))
        return;

    const BoundingBoxf3 instance_bounds = object->instance_convex_hull_bounding_box(instance_index);
    const Vec3d         plate_center     = plate->get_center_origin();
    const Vec3d         instance_center  = instance_bounds.center();
    const Vec3d         planar_translation(plate_center.x() - instance_center.x(),
                                           plate_center.y() - instance_center.y(),
                                           0.0);

    object->translate_instance(instance_index, planar_translation);

    if (plate->check_outside(static_cast<int>(object_index), static_cast<int>(instance_index))) {
        BOOST_LOG_TRIVIAL(warning)
            << "Auto orientation placement remains outside printable area: object_index=" << object_index
            << ", instance_index=" << instance_index << ", plate_index=" << plate_index;
    }
}

} // namespace

orientation::OrientMesh OrientJob::create_orientation_input(ModelInstance* instance,
                                                             size_t         object_index,
                                                             size_t         instance_index)
{
    using OrientMesh = orientation::OrientMesh;
    OrientMesh om;
    auto obj = instance->get_object();
    om.name = obj->name;
    // Keep volume transforms and this instance's scale/mirror, but exclude
    // previous rotations and other instances so mode switches are independent.
    om.mesh = obj->raw_mesh();
    om.mesh.transform(instance->get_transformation().get_matrix(
        /*dont_translate=*/true, /*dont_rotate=*/true));
    if (obj->config.has("support_threshold_angle"))
        om.overhang_angle = obj->config.opt_int("support_threshold_angle");
    else {
        const Slic3r::DynamicPrintConfig& config = wxGetApp().preset_bundle->full_config();
        om.overhang_angle = config.opt_int("support_threshold_angle");
    }

    om.setter = [instance, object_index, instance_index](const OrientMesh& orientation_input) {
        apply_orientation_transform(instance, object_index, instance_index, orientation_input);
    };
    return om;
}

}} // namespace Slic3r::GUI
