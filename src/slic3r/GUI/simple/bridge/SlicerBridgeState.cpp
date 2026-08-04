#include "SlicerBridge.hpp"
#include "SlicerBridgeDiagnostics.hpp"

#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/PartPlate.hpp"
#include "slic3r/GUI/Tab.hpp"
#include "slic3r/GUI/NotificationManager.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/ModelObject.hpp"
#include "libslic3r/ModelVolume.hpp"
#include "libslic3r/ModelInstance.hpp"
#include "libslic3r/TriangleMesh.hpp"
#include "slic3r/GUI/Selection.hpp"
#include "slic3r/GUI/GUI_ObjectList.hpp"
#include "slic3r/GUI/GLCanvas3D.hpp"
#include "slic3r/GUI/print_manage/data/DataCenter.hpp"
#include "slic3r/GUI/print_manage/PrinterMgr.hpp"
#include <wx/mstream.h>
#include <boost/beast/core/detail/base64.hpp>

#include <boost/log/trivial.hpp>
#include <cmath>
#include <algorithm>
#include <cctype>
#include <unordered_set>
#include <limits>
#include <sstream>
#include <stdexcept>

using json = nlohmann::json;

namespace Slic3r {
namespace GUI {
namespace Bridge {

namespace {

bool IsSupportedCaptureView(const std::string& view)
{
    static const std::unordered_set<std::string> kSupportedViews = {
        "left", "right", "bottom", "top", "front", "rear", "iso",
        "iso_front_right", "iso_rear_right", "iso_rear_left"
    };
    return kSupportedViews.count(view) > 0;
}

std::string TrimCopy(std::string value)
{
    const auto is_ws = [](unsigned char ch) { return std::isspace(ch) != 0; };
    while (!value.empty() && is_ws(static_cast<unsigned char>(value.front()))) value.erase(value.begin());
    while (!value.empty() && is_ws(static_cast<unsigned char>(value.back()))) value.pop_back();
    return value;
}

std::vector<std::string> ParseCaptureViews(const json& params)
{
    std::vector<std::string> views;
    if (params.contains("views")) {
        if (params["views"].is_array()) {
            for (const auto& item : params["views"]) {
                if (!item.is_string())
                    continue;
                const std::string view = TrimCopy(item.get<std::string>());
                if (!view.empty() && IsSupportedCaptureView(view) &&
                    std::find(views.begin(), views.end(), view) == views.end()) {
                    views.push_back(view);
                }
            }
        } else if (params["views"].is_string()) {
            std::stringstream ss(params["views"].get<std::string>());
            std::string token;
            while (std::getline(ss, token, ',')) {
                const std::string view = TrimCopy(token);
                if (!view.empty() && IsSupportedCaptureView(view) &&
                    std::find(views.begin(), views.end(), view) == views.end()) {
                    views.push_back(view);
                }
            }
        }
    }

    if (views.empty())
        views = {"front", "rear", "left", "right", "top", "bottom", "iso", "iso_front_right", "iso_rear_right", "iso_rear_left"};
    return views;
}

std::string ThumbnailToDataUrl(const ThumbnailData& thumbnail)
{
    wxImage image(thumbnail.width, thumbnail.height);
    image.InitAlpha();
    for (unsigned int row = 0; row < thumbnail.height; ++row) {
        unsigned int flipped_row = (thumbnail.height - 1 - row) * thumbnail.width;
        for (unsigned int col = 0; col < thumbnail.width; ++col) {
            unsigned char* px = (unsigned char*)thumbnail.pixels.data() + 4 * (flipped_row + col);
            image.SetRGB((int)col, (int)row, px[0], px[1], px[2]);
            image.SetAlpha((int)col, (int)row, px[3]);
        }
    }

    wxMemoryOutputStream mem_stream;
    if (!image.SaveFile(mem_stream, wxBITMAP_TYPE_PNG))
        throw std::runtime_error("Failed to encode thumbnail as PNG");

    const size_t png_size = mem_stream.GetSize();
    std::vector<unsigned char> png_bytes(png_size);
    mem_stream.CopyTo(png_bytes.data(), png_size);

    std::string encoded(boost::beast::detail::base64::encoded_size(png_size), '\0');
    boost::beast::detail::base64::encode(&encoded[0], png_bytes.data(), png_size);
    return "data:image/png;base64," + encoded;
}

json BuildCurrentPlateSliceResultState(PartPlateList& plate_list, PartPlate* current_plate)
{
    json slice_result_state = json::object();
    if (!current_plate)
        return slice_result_state;

    auto* current_result = plate_list.get_current_slice_result();
    if (!current_result)
        return slice_result_state;

    const auto& current_print_statistics = plate_list.get_current_fff_print().print_statistics();

    double filament_used_mm = current_print_statistics.total_used_filament;
    double filament_weight_g = current_print_statistics.total_weight;
    if (filament_used_mm <= 0.0 || filament_weight_g <= 0.0) {
        filament_used_mm = 0.0;
        filament_weight_g = 0.0;
        for (const auto& role_entry : current_result->print_statistics.used_filaments_per_role) {
            filament_used_mm += role_entry.second.first;
            filament_weight_g += role_entry.second.second;
        }
    }

    double estimated_time_s = 0.0;
    if (!current_result->print_statistics.modes.empty())
        estimated_time_s = current_result->print_statistics.modes[0].model_time_s();
    if (estimated_time_s <= 0.0)
        estimated_time_s = current_result->print_statistics.total_estimated_time;

    slice_result_state["filename"] = current_result->filename;
    slice_result_state["estimated_time_s"] = estimated_time_s;
    slice_result_state["filament_cost"] =
        current_print_statistics.total_cost > 0.0
            ? current_print_statistics.total_cost
            : current_result->print_statistics.total_filament_cost;
    slice_result_state["filament_used_mm"] = filament_used_mm;
    slice_result_state["filament_weight_g"] = filament_weight_g;
    slice_result_state["total_layer_count"] = current_print_statistics.total_layer_count;
    slice_result_state["toolpath_outside"] = current_result->toolpath_outside;
    slice_result_state["warnings"] = json::array();
    for (const auto& warning : current_result->warnings) {
        slice_result_state["warnings"].push_back({
            {"level", warning.level},
            {"message", warning.msg},
            {"error_code", warning.error_code}
        });
    }

    return slice_result_state;
}

std::string JsonStringValue(const json& item, const char* key)
{
    if (!item.is_object() || !item.contains(key) || !item.at(key).is_string())
        return std::string();
    return item.at(key).get<std::string>();
}

std::string LowerAsciiCopy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string SeverityFromNotification(const std::string& type, const std::string& level)
{
    const std::string normalized_type = LowerAsciiCopy(type);
    const std::string normalized_level = LowerAsciiCopy(level);
    if (normalized_level == "error" || normalized_level == "warning" ||
        normalized_level == "important" || normalized_level == "critical" ||
        normalized_level == "danger")
        return normalized_level;
    if (normalized_type.find("error") != std::string::npos)
        return "error";
    if (normalized_type.find("warning") != std::string::npos)
        return "warning";
    return std::string();
}

bool IsBlockingSeverity(const std::string& severity)
{
    const std::string normalized = LowerAsciiCopy(severity);
    return normalized == "error" || normalized == "critical" || normalized == "danger";
}

bool IsScenePrecheckNotificationType(const std::string& type)
{
    const std::string normalized = LowerAsciiCopy(type);
    return normalized == "validate_error" ||
           normalized == "validate_warning" ||
           normalized == "slicing_error" ||
           normalized == "slicing_serious_warning" ||
           normalized == "slicing_warning" ||
           normalized == "plater_error" ||
           normalized == "plater_warning" ||
           normalized == "object_info_warning";
}

std::string IssueTypeFromNotification(const std::string& type, const std::string& text)
{
    const std::string normalized_type = LowerAsciiCopy(type);
    const std::string normalized_text = LowerAsciiCopy(text);
    if (normalized_text.find("conflict") != std::string::npos)
        return "gcode_conflict";
    if (normalized_text.find("support") != std::string::npos &&
        normalized_text.find("outside") != std::string::npos)
        return "support_outside_bed";
    if (normalized_text.find("g-code") != std::string::npos ||
        normalized_text.find("gcode") != std::string::npos) {
        if (normalized_text.find("height") != std::string::npos)
            return "tool_height_outside";
        return "toolpath_outside_bed";
    }
    if (normalized_type == "object_info_warning")
        return "object_clearance";
    return "scene_warning";
}

bool HasPrecheckIssueType(const json& issues, const std::string& issue_type)
{
    if (!issues.is_array())
        return false;
    for (const auto& issue : issues) {
        if (issue.is_object() && JsonStringValue(issue, "issue_type") == issue_type)
            return true;
    }
    return false;
}

void AppendPrecheckIssueFromNotification(
    json& warnings,
    json& precheck_issues,
    const json& notification,
    int notification_index,
    int plate_index)
{
    const std::string type = JsonStringValue(notification, "type");
    if (!IsScenePrecheckNotificationType(type))
        return;

    const std::string text = JsonStringValue(notification, "text");
    if (text.empty())
        return;

    const std::string severity = SeverityFromNotification(type, JsonStringValue(notification, "level"));
    if (severity.empty())
        return;

    const std::string issue_type = IssueTypeFromNotification(type, text);
    warnings.push_back({
        {"level", severity},
        {"message", text},
        {"type", type},
        {"source", "ui_notification"}
    });

    precheck_issues.push_back({
        {"issue_id", "ui_notification_" + std::to_string(notification_index)},
        {"issue_type", issue_type},
        {"severity", severity},
        {"blocking", IsBlockingSeverity(severity)},
        {"title", text},
        {"description", text},
        {"evidence", {
            {"source", "ui_notification"},
            {"notification_type", type},
            {"notification_level", severity},
            {"warning_text", text},
            {"plate_index", plate_index}
        }}
    });
}

void AppendToolpathOutsideIssueIfNeeded(json& warnings, json& precheck_issues, int plate_index)
{
    if (HasPrecheckIssueType(precheck_issues, "toolpath_outside_bed"))
        return;

    const std::string text = "A G-code path goes beyond the boundary of plate.";
    warnings.push_back({
        {"level", "warning"},
        {"message", text},
        {"type", "slicing_serious_warning"},
        {"source", "slice_result"}
    });
    precheck_issues.push_back({
        {"issue_id", "toolpath_outside_bed_current_plate"},
        {"issue_type", "toolpath_outside_bed"},
        {"severity", "warning"},
        {"blocking", false},
        {"title", text},
        {"description", text},
        {"evidence", {
            {"source", "slice_result"},
            {"slice_toolpath_outside", true},
            {"plate_index", plate_index}
        }}
    });
}


void AppendCurrentPlateValidationIssues(json& warnings, json& precheck_issues, const json& validation)
{
    if (!validation.is_object() || !validation.value("available", false))
        return;

    const int plate_index = validation.value("plate_index", -1);
    const json outside_instances = validation.value("outside_instances", json::array());
    if (outside_instances.is_array()) {
        for (const auto& item : outside_instances) {
            if (!item.is_object())
                continue;

            const int object_id = item.value("object_id", -1);
            const int instance_id = item.value("instance_id", -1);
            if (object_id < 0)
                continue;

            const std::string name = item.value("name", std::string());
            const bool exceeds_height = item.value("exceeds_height", false);
            const bool larger_than_plate = item.value("larger_than_plate", false);
            const std::string title = exceeds_height
                ? "Object exceeds build height limit."
                : "Object is outside the current plate build volume.";

            warnings.push_back({
                {"level", "error"},
                {"message", title},
                {"type", "part_plate_validation"},
                {"source", "part_plate"}
            });
            precheck_issues.push_back({
                {"issue_id", "model_out_of_bed_obj_" + std::to_string(object_id) + "_inst_" + std::to_string(instance_id)},
                {"issue_type", "model_out_of_bed"},
                {"severity", "error"},
                {"blocking", true},
                {"title", title},
                {"description", title},
                {"candidate_tools", json::array({"move_object"})},
                {"evidence", {
                    {"source", "part_plate"},
                    {"object_id", object_id},
                    {"object_index", object_id},
                    {"object_name", name},
                    {"instance_id", instance_id},
                    {"plate_index", plate_index},
                    {"outside_x", item.value("outside_x", false)},
                    {"outside_y", item.value("outside_y", false)},
                    {"exceeds_height", exceeds_height},
                    {"larger_than_plate", larger_than_plate},
                    {"bbox_min", item.value("bbox_min", json::array())},
                    {"bbox_max", item.value("bbox_max", json::array())},
                    {"build_volume_min", item.value("build_volume_min", json::array())},
                    {"build_volume_max", item.value("build_volume_max", json::array())}
                }}
            });
        }
    }

    const json overlap_pairs = validation.value("overlap_pairs", json::array());
    if (!overlap_pairs.is_array())
        return;

    for (size_t i = 0; i < overlap_pairs.size(); ++i) {
        const auto& pair = overlap_pairs[i];
        if (!pair.is_object())
            continue;

        const json a = pair.value("a", json::object());
        const json b = pair.value("b", json::object());
        const int a_object_id = a.value("object_id", -1);
        const int b_object_id = b.value("object_id", -1);
        if (a_object_id < 0 || b_object_id < 0)
            continue;

        const std::string title = "Models overlap on the current plate.";
        warnings.push_back({
            {"level", "error"},
            {"message", title},
            {"type", "part_plate_validation"},
            {"source", "part_plate"}
        });
        precheck_issues.push_back({
            {"issue_id", "model_overlap_pair_" + std::to_string(a_object_id) + "_" + std::to_string(b_object_id) + "_" + std::to_string(i)},
            {"issue_type", "model_collision"},
            {"severity", "error"},
            {"blocking", true},
            {"title", title},
            {"description", title},
            {"candidate_tools", json::array({"move_object", "auto_arrange"})},
            {"evidence", {
                {"source", "part_plate"},
                {"plate_index", plate_index},
                {"current_plate_validation", {
                    {"available", true},
                    {"has_model_overlap", true},
                    {"overlap_pairs", json::array({pair})},
                    {"plate_index", plate_index}
                }},
                {"object_id", b_object_id},
                {"object_index", b_object_id},
                {"object_name", b.value("name", std::string())},
                {"reference_object_id", a_object_id},
                {"reference_object_index", a_object_id},
                {"reference_object_name", a.value("name", std::string())}
            }}
        });
    }
}
} // namespace

json SlicerBridge::DoCaptureModelViews(const json& params)
{
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};

    auto* canvas = plater->get_view3D_canvas3D();
    if (!canvas)
        return {{"success", false}, {"message", "3D canvas not available"}};
    if (!canvas->make_current_for_postinit())
        return {{"success", false}, {"message", "Failed to activate OpenGL context"}};

    Model& model = plater->model();
    if (model.objects.empty())
        return {{"success", false}, {"message", "No model loaded"}};

    int object_idx = -1;
    if (params.contains("object_index")) {
        if (params["object_index"].is_number_integer()) {
            object_idx = params["object_index"].get<int>();
        } else if (params["object_index"].is_string()) {
            try { object_idx = std::stoi(params["object_index"].get<std::string>()); } catch (...) {}
        }
    }

    if (object_idx < 0)
        object_idx = canvas->get_selection().get_object_idx();

    if (object_idx < 0) {
        int only_object_idx = -1;
        for (int idx = 0; idx < (int)model.objects.size(); ++idx) {
            if (model.objects[idx] == nullptr)
                continue;
            if (only_object_idx != -1)
                return {{"success", false}, {"message", "Please select a single model before capturing views"}};
            only_object_idx = idx;
        }
        object_idx = only_object_idx;
    }

    if (object_idx < 0 || object_idx >= (int)model.objects.size() || model.objects[object_idx] == nullptr)
        return {{"success", false}, {"message", "Invalid or missing target model object"}};

    const auto views = ParseCaptureViews(params);
    const unsigned int width = static_cast<unsigned int>(std::max(128, std::min(2048, params.value("width", 512))));
    const unsigned int height = static_cast<unsigned int>(std::max(128, std::min(2048, params.value("height", 512))));
    const int plate_idx = plater->get_partplate_list().get_curr_plate_index();
    const ThumbnailsParams thumbnail_params = { {}, true, true, true, true, plate_idx };

    auto& volumes = const_cast<GLVolumeCollection&>(canvas->get_volumes());
    std::vector<bool> original_printable;
    original_printable.reserve(volumes.volumes.size());

    bool has_target_volume = false;
    for (GLVolume* volume : volumes.volumes) {
        original_printable.push_back(volume->printable);
        if (volume->object_idx() == object_idx)
            has_target_volume = true;
    }
    if (!has_target_volume)
        return {{"success", false}, {"message", "Target model has no renderable volumes"}};

    struct RestorePrintableState {
        GLVolumeCollection& volumes;
        const std::vector<bool>& original;
        ~RestorePrintableState() {
            for (size_t idx = 0; idx < volumes.volumes.size() && idx < original.size(); ++idx)
                volumes.volumes[idx]->printable = original[idx];
        }
    } restore{volumes, original_printable};

    for (size_t idx = 0; idx < volumes.volumes.size(); ++idx) {
        GLVolume* volume = volumes.volumes[idx];
        const bool keep = volume->object_idx() == object_idx;
        volume->printable = keep ? original_printable[idx] : false;
    }

    json captured_views = json::array();
    for (const std::string& view : views) {
        ThumbnailData thumbnail;
        canvas->render_thumbnail(thumbnail, width, height, thumbnail_params, Camera::EType::Ortho, false, false, false, view);
        if (!thumbnail.is_valid())
            return {{"success", false}, {"message", "Failed to render thumbnail for view: " + view}};

        json item;
        item["view"] = view;
        item["width"] = thumbnail.width;
        item["height"] = thumbnail.height;
        item["mime_type"] = "image/png";
        item["data_url"] = ThumbnailToDataUrl(thumbnail);
        captured_views.push_back(std::move(item));
    }

    return {
        {"success", true},
        {"message", "Captured model views"},
        {"object_index", object_idx},
        {"object_name", model.objects[object_idx]->name},
        {"views", std::move(captured_views)}
    };
}

// ===========================================================================
// Overhang analysis helper
// ===========================================================================

OverhangResult SlicerBridge::AnalyzeOverhang(const ModelObject& obj, double threshold_deg)
{
    OverhangResult result;
    if (obj.instances.empty() || obj.volumes.empty())
        return result;

    const Transform3d& trafo = obj.instances[0]->get_matrix();
    const Eigen::Matrix3d normal_matrix = trafo.linear();
    const double cos_thresh = -std::cos(threshold_deg * M_PI / 180.0);
    constexpr double z_epsilon = 0.05;
    constexpr double clearance_threshold = 1.0;

    double total_area          = 0.0;
    double downward_area       = 0.0;
    double unsupported_area    = 0.0;
    double bottom_contact_area = 0.0;
    double max_angle_deg       = 0.0;
    double object_min_z        = std::numeric_limits<double>::infinity();

    for (const ModelVolume* vol : obj.volumes) {
        if (!vol->is_model_part())
            continue;

        const indexed_triangle_set& its = vol->mesh().its;
        for (const auto& vertex : its.vertices) {
            const Vec3d world = trafo * vertex.cast<double>();
            object_min_z = std::min(object_min_z, world.z());
        }
    }

    if (!std::isfinite(object_min_z))
        return result;

    for (const ModelVolume* vol : obj.volumes) {
        if (!vol->is_model_part())
            continue;

        const indexed_triangle_set& its = vol->mesh().its;
        for (size_t fi = 0; fi < its.indices.size(); ++fi) {
            const auto& face = its.indices[fi];
            const Vec3d local_v0 = its.vertices[face[0]].cast<double>();
            const Vec3d local_v1 = its.vertices[face[1]].cast<double>();
            const Vec3d local_v2 = its.vertices[face[2]].cast<double>();

            const Vec3d world_v0 = trafo * local_v0;
            const Vec3d world_v1 = trafo * local_v1;
            const Vec3d world_v2 = trafo * local_v2;

            const Vec3d cross = (world_v1 - world_v0).cross(world_v2 - world_v0);
            const double area = 0.5 * cross.norm();
            if (area < 1e-12)
                continue;

            total_area += area;

            const Vec3d normal = (normal_matrix * (local_v1 - local_v0).cross(local_v2 - local_v0)).normalized();
            if (normal.z() < cos_thresh) {
                downward_area += area;

                const double face_min_z = std::min({world_v0.z(), world_v1.z(), world_v2.z()});
                const double face_max_z = std::max({world_v0.z(), world_v1.z(), world_v2.z()});
                const double clearance = face_min_z - object_min_z;

                if (face_max_z <= object_min_z + z_epsilon) {
                    bottom_contact_area += area;
                    continue;
                }

                if (clearance <= clearance_threshold)
                    continue;

                unsupported_area += area;
                double angle = std::acos(std::clamp(normal.z(), -1.0, 1.0)) * 180.0 / M_PI;
                double overhang_angle = angle - 90.0;
                if (overhang_angle > max_angle_deg)
                    max_angle_deg = overhang_angle;
            }
        }
    }

    result.downward_area_ratio = (total_area > 1e-12) ? (downward_area / total_area) : 0.0;
    result.unsupported_area_ratio = (total_area > 1e-12) ? (unsupported_area / total_area) : 0.0;
    result.overhang_ratio = result.unsupported_area_ratio;
    result.bottom_contact_ratio = (total_area > 1e-12) ? (bottom_contact_area / total_area) : 0.0;
    result.bottom_contact_area_mm2 = bottom_contact_area;
    result.max_overhang_angle = max_angle_deg;
    return result;
}

// ===========================================================================
// Action implementations (migrated from MCPChatPanel)
// ===========================================================================

json SlicerBridge::DoGetSlicerState(const json& /*params*/)
{
    auto* bundle = wxGetApp().preset_bundle;
    auto* plater = wxGetApp().plater();
    json state;

    // --- Current tab (page) ---
    {
        auto* mainframe = wxGetApp().mainframe;
        int current_tab_index = -1;
        std::string current_tab_name;
        if (mainframe && mainframe->m_tabpanel) {
            current_tab_index = static_cast<int>(mainframe->m_tabpanel->GetSelection());
        }
        // Map tab index to human-readable name
        switch (current_tab_index) {
            case MainFrame::tpHome:         current_tab_name = "home";         break;
            case MainFrame::tpOnlineModel:  current_tab_name = "online_model"; break;
            case MainFrame::tp3DEditor:     current_tab_name = "prepare";      break;
            case MainFrame::tpPreview:      current_tab_name = "preview";      break;
            case MainFrame::tpMonitor:      current_tab_name = "monitor";      break;
            case MainFrame::tpMultiDevice:  current_tab_name = "multi_device"; break;
            case MainFrame::tpDeviceMgr:    current_tab_name = "device_mgr";   break;
            default:                        current_tab_name = "unknown";      break;
        }
        state["current_tab"] = current_tab_name;
        state["current_tab_index"] = current_tab_index;
    }

    // --- Preset names (keep original) ---
    if (bundle) {
        state["current_print_preset"]    = bundle->prints.get_edited_preset().name;
        state["current_filament_preset"] = bundle->filaments.get_edited_preset().name;
        state["current_printer_preset"]  = bundle->printers.get_edited_preset().name;

        const auto get_opt = [](const DynamicPrintConfig& cfg, const std::string& key) -> std::string {
            const ConfigOption* opt = cfg.option(key);
            return opt ? opt->serialize() : "";
        };

        const DynamicPrintConfig& printer_cfg = bundle->printers.get_edited_preset().config;
        const DynamicPrintConfig& filament_cfg = bundle->filaments.get_edited_preset().config;
        state["printer_model"] = get_opt(printer_cfg, "printer_model");
        state["nozzle_diameter"] = get_opt(printer_cfg, "nozzle_diameter");
        state["current_filament_type"] = get_opt(filament_cfg, "filament_type");

        // --- software_context: request-level context consumed by the AI knowledge
        // base (智能问答). Assembled here because the slicer is the source of truth
        // for the live machine/material configuration. ---
        json software_context = json::object();
        software_context["software_version"] = GUI_App::format_display_version();

        // machine_model: strip the leading "Creality " vendor prefix so the AI sees
        // the bare model name (e.g. "K1 Max" instead of "Creality K1 Max").
        {
            std::string machine_model = get_opt(printer_cfg, "printer_model");
            static const std::string kVendorPrefix = "creality ";
            if (machine_model.size() >= kVendorPrefix.size() &&
                LowerAsciiCopy(machine_model.substr(0, kVendorPrefix.size())) == kVendorPrefix) {
                machine_model = TrimCopy(machine_model.substr(kVendorPrefix.size()));
            }
            software_context["machine_model"] = machine_model;
        }

        // nozzle_diameter is serialized as a (possibly multi-extruder) string such
        // as "0.4" or "0.4,0.4"; expose the first value as a number.
        {
            const std::string nozzle_str = get_opt(printer_cfg, "nozzle_diameter");
            const size_t split_pos = nozzle_str.find_first_of(",; ");
            const std::string first_nozzle = (split_pos == std::string::npos)
                ? nozzle_str
                : nozzle_str.substr(0, split_pos);
            try {
                if (!first_nozzle.empty())
                    software_context["nozzle_diameter_mm"] = std::stod(first_nozzle);
            } catch (...) {
                // leave unset when unparseable
            }
        }

        // active_filament_type: reserved field, currently always "".
        software_context["active_filament_type"] = "";

        // available_filament_types: filament type per loaded slot, in order, so the
        // array maps one-to-one to the filament slots shown in the UI (no dedup).
        json available_filament_types = json::array();
        for (const std::string& preset_name : bundle->filament_presets) {
            const Preset* preset = bundle->filaments.find_preset(preset_name);
            if (!preset)
                continue;
            available_filament_types.push_back(get_opt(preset->config, "filament_type"));
        }
        software_context["available_filament_types"] = available_filament_types;

        software_context["process_preset"] = bundle->prints.get_edited_preset().name;

        state["software_context"] = software_context;
    }

    if (!plater)
        return {{"success", true}, {"message", "OK"}, {"state", state}};

    const bool project_dirty = plater->is_project_dirty();
    const std::string project_path = into_u8(plater->get_project_filename());
    state["project_dirty"] = project_dirty;
    state["project_saved"] = !project_dirty;
    state["project_path"] = project_path;
    state["project_file_path"] = project_path;

    const Model& model = plater->model();
    state["has_model"]   = !model.objects.empty();
    state["model_count"] = (int)model.objects.size();

    auto round2 = [](double v) { return std::round(v * 100.0) / 100.0; };

    // --- Plate summary and geometry ---
    PartPlateList& plate_list = plater->get_partplate_list();
    const int plate_count = plate_list.get_plate_count();
    const int current_plate_index = plate_list.get_curr_plate_index();
    PartPlate* current_plate = plate_list.get_curr_plate();
    state["plate_count"] = plate_count;
    state["current_plate_index"] = current_plate_index;

    if (current_plate) {
        json current_plate_state;
        auto* slice_result = current_plate->get_slice_result();
        bool has_critical_warning = false;
        bool toolpath_outside = false;
        bool is_all_plates_selected = false;
        bool can_start_print = false;
        const bool can_slice = (wxGetApp().mainframe && wxGetApp().mainframe->get_enable_slice_status());
        const bool plate_dirty = !(plater && plater->only_gcode_mode()) && can_slice;
        if (slice_result != nullptr)
            toolpath_outside = slice_result->toolpath_outside;
        try {
            has_critical_warning = current_plate->get_print().has_critical_warning_status();
        } catch (...) {
            has_critical_warning = false;
        }
        try {
            if (plater && plater->get_preview_canvas3D())
                is_all_plates_selected = plater->get_preview_canvas3D()->is_all_plates_selected();
        } catch (...) {
            is_all_plates_selected = false;
        }

        const bool slice_result_valid = current_plate->is_slice_result_valid() && !plate_dirty;
        const bool slice_result_ready_for_print = current_plate->is_slice_result_ready_for_print() && !plate_dirty;
        const bool slice_result_ready_for_export = current_plate->is_slice_result_ready_for_export() && !plate_dirty;
        const bool valid_gcode_file = current_plate->is_valid_gcode_file() && !plate_dirty;
        json current_plate_slice_result = BuildCurrentPlateSliceResultState(plate_list, current_plate);
        if (plater && plater->only_gcode_mode()) {
            can_start_print = valid_gcode_file ||
                              (slice_result != nullptr && !slice_result->filename.empty());
        } else {
            can_start_print = current_plate->has_printable_instances() &&
                              slice_result_ready_for_print;
        }
        can_start_print = can_start_print && !is_all_plates_selected;

        current_plate_state["index"] = current_plate->get_index();
        current_plate_state["name"] = current_plate->get_plate_name();
        current_plate_state["printable"] = current_plate->is_printable();
        current_plate_state["empty"] = current_plate->empty();
        current_plate_state["has_printable_instances"] = current_plate->has_printable_instances();
        current_plate_state["printable_instance_count"] = current_plate->printable_instance_size();
        current_plate_state["can_slice"] = can_slice;
        current_plate_state["plate_dirty"] = plate_dirty;
        current_plate_state["slice_result_exists"] = (slice_result != nullptr);
        current_plate_state["slice_result_valid"] = slice_result_valid;
        current_plate_state["slice_result_ready_for_print"] = slice_result_ready_for_print;
        current_plate_state["slice_result_ready_for_export"] = slice_result_ready_for_export;
        current_plate_state["slice_has_critical_warning"] = has_critical_warning;
        current_plate_state["slice_toolpath_outside"] = toolpath_outside;
        current_plate_state["valid_gcode_file"] = valid_gcode_file;
        current_plate_state["all_plates_selected"] = is_all_plates_selected;
        current_plate_state["can_start_print"] = can_start_print;
        if (!current_plate_slice_result.empty()) {
            current_plate_state["slice_result"] = current_plate_slice_result;
            current_plate_state["filename"] = current_plate_slice_result.value("filename", std::string());
            current_plate_state["estimated_time_s"] = current_plate_slice_result.value("estimated_time_s", 0.0);
            current_plate_state["filament_cost"] = current_plate_slice_result.value("filament_cost", 0.0);
            current_plate_state["filament_used_mm"] = current_plate_slice_result.value("filament_used_mm", 0.0);
            current_plate_state["filament_weight_g"] = current_plate_slice_result.value("filament_weight_g", 0.0);
            current_plate_state["total_layer_count"] = current_plate_slice_result.value("total_layer_count", 0);
            current_plate_state["warnings"] = current_plate_slice_result.value("warnings", json::array());
        }

        BOOST_LOG_TRIVIAL(info)
            << "[SlicerBridgeState] current_plate index=" << current_plate->get_index()
            << " printable_instance_count=" << current_plate->printable_instance_size()
            << " can_slice=" << current_plate_state["can_slice"].dump()
            << " plate_dirty=" << current_plate_state["plate_dirty"].dump()
            << " slice_result_valid=" << current_plate_state["slice_result_valid"].dump()
            << " slice_result_ready_for_print=" << current_plate_state["slice_result_ready_for_print"].dump()
            << " valid_gcode_file=" << current_plate_state["valid_gcode_file"].dump()
            << " can_start_print=" << current_plate_state["can_start_print"].dump();

        state["current_plate"] = std::move(current_plate_state);
        state["current_plate_can_print"] = state["current_plate"]["can_start_print"];
    } else {
        state["current_plate_can_print"] = false;
    }


    int plate_w = 0, plate_d = 0, plate_h = 0;
    plate_list.get_plate_size(plate_w, plate_d, plate_h);
    state["plate_size"] = { plate_w, plate_d, plate_h };

    json plates_arr = json::array();

    for (int pi = 0; pi < plate_count; ++pi) {
        PartPlate* plate = plate_list.get_plate(pi);
        if (!plate)
            continue;

        json p;
        p["index"] = plate->get_index();
        p["name"] = plate->get_plate_name();
        p["locked"] = plate->is_locked();
        p["empty"] = plate->empty();
        p["printable_instance_count"] = plate->printable_instance_size();

        Vec3d origin = plate->get_origin();
        Vec3d center = plate->get_center_origin();
        Vec2d size_xy = plate->get_size();
        BoundingBoxf3 bv = plate->get_build_volume();

        p["origin"] = { round2(origin.x()), round2(origin.y()), round2(origin.z()) };
        p["center"] = { round2(center.x()), round2(center.y()), round2(center.z()) };
        p["size"] = { round2(size_xy.x()), round2(size_xy.y()), plate_h };
        p["build_volume"] = {
            {"min", { round2(bv.min.x()), round2(bv.min.y()), round2(bv.min.z()) }},
            {"max", { round2(bv.max.x()), round2(bv.max.y()), round2(bv.max.z()) }}
        };

        plates_arr.push_back(std::move(p));
    }
    state["plates"] = plates_arr;

    std::vector<int> object_plate_index(model.objects.size(), -1);

    DM::Device cur_dev = DM::DataCenter::Ins().get_current_device_data();
    json raw_current_device = DM::DataCenter::Ins().get_current_device();
    if (!cur_dev.valid) {
        const std::string selected_mac = DM::DeviceMgr::Ins().GetCurrentDevice();
        if (!selected_mac.empty()) {
            json selected_device = DM::DataCenter::Ins().find_printer_by_mac(selected_mac);
            if (selected_device.is_object() && !selected_device.empty()) {
                cur_dev = DM::Device::deserialize(selected_device);
                cur_dev.mac = selected_mac;
                cur_dev.isCurrentDevice = true;
                raw_current_device = selected_device;
            }
        }
    }
    int current_device_material_box_count = 0;
    int current_device_cfs_box_count = 0;
    for (const auto& box : cur_dev.materialBoxes) {
        ++current_device_material_box_count;
        if (box.box_type == 0 || box.box_type == 2)
            ++current_device_cfs_box_count;
    }

    json current_device_features = json::array();
    if (raw_current_device.is_object() && raw_current_device.contains("features") && raw_current_device["features"].is_array())
        current_device_features = raw_current_device["features"];
    state["current_device"] = {
        {"valid", cur_dev.valid},
        {"name", cur_dev.name},
        {"address", cur_dev.address},
        {"mac", cur_dev.mac},
        {"tb_id", cur_dev.tbId},
        {"model_name", cur_dev.modelName},
        {"online", cur_dev.online},
        {"device_state", cur_dev.deviceState},
        {"device_type", cur_dev.deviceType},
        {"webrtc_support", cur_dev.webrtcSupport},
        {"old_printer", cur_dev.oldPrinter},
        {"is_multi_color_device", cur_dev.isMultiColorDevice},
        {"material_box_count", current_device_material_box_count},
        {"cfs_box_count", current_device_cfs_box_count},
        {"has_cfs_box", current_device_cfs_box_count > 0},
        {"features", current_device_features},
        {"video_token", raw_current_device.is_object() ? raw_current_device.value("videoToken", "") : ""},
        {"ctrol", raw_current_device.is_object() && raw_current_device.contains("ctrol") && raw_current_device["ctrol"].is_object() ? raw_current_device["ctrol"] : json::object()},
        {"curPosition", raw_current_device.is_object() ? raw_current_device.value("curPosition", "") : ""},
        {"is_idle", cur_dev.valid && cur_dev.deviceState == 0},
        {"has_bound_device", cur_dev.valid && !cur_dev.address.empty()}
    };

    // --- Helper to read config value as string ---
    auto get_opt = [](const DynamicPrintConfig& cfg, const std::string& key) -> std::string {
        const ConfigOption* opt = cfg.option(key);
        return opt ? opt->serialize() : "";
    };

    // --- Filament info ---
    if (bundle) {
        json filaments_arr = json::array();
        for (size_t i = 0; i < bundle->filament_presets.size(); ++i) {
            json fi;
            fi["id"] = (int)(i + 1);
            fi["name"] = bundle->filament_presets[i];
            // Try to get filament_type from the preset config
            const Preset* preset = bundle->filaments.find_preset(bundle->filament_presets[i]);
            if (preset) {
                fi["type"] = get_opt(preset->config, "filament_type");
            }
            filaments_arr.push_back(fi);
        }
        state["filaments"] = filaments_arr;
        state["is_multi_filament"] = bundle->filament_presets.size() > 1;

        const std::vector<std::string> extruder_colors = plater->get_extruder_colors_from_plater_config();
        state["extruder_colors"] = extruder_colors;
    }

    // --- Per-model detailed info ---
    const DynamicPrintConfig& print_cfg = bundle ? bundle->prints.get_edited_preset().config : DynamicPrintConfig();
    double support_threshold = 30.0;  // default
    if (bundle) {
        std::string thresh_str = get_opt(print_cfg, "support_threshold_angle");
        if (!thresh_str.empty()) {
            try { support_threshold = std::stod(thresh_str); }
            catch (...) {}
        }
    }

    // Map each object to a plate index (first matching plate/instance).
    for (size_t oi = 0; oi < model.objects.size(); ++oi) {
        const ModelObject* mo = model.objects[oi];
        if (!mo || mo->instances.empty())
            continue;

        for (int pi = 0; pi < plate_count; ++pi) {
            PartPlate* plate = plate_list.get_plate(pi);
            if (!plate)
                continue;

            bool on_plate = false;
            for (size_t ii = 0; ii < mo->instances.size(); ++ii) {
                if (plate->contain_instance((int)oi, (int)ii)) {
                    on_plate = true;
                    break;
                }
            }

            if (on_plate) {
                object_plate_index[oi] = pi;
                break;
            }
        }
    }
    json objects_arr = json::array();
    json mesh_errors_arr = json::array();
    json precheck_issues = json::array();

    for (size_t oi = 0; oi < model.objects.size(); ++oi) {
        const ModelObject* mo = model.objects[oi];
        if (!mo || mo->instances.empty())
            continue;

        json obj_info;
        obj_info["object_index"] = (int)oi;
        obj_info["name"] = mo->name;

        const int pidx = (oi < object_plate_index.size()) ? object_plate_index[oi] : -1;
        obj_info["plate_index"] = pidx;
        if (pidx >= 0 && pidx < plate_count) {
            PartPlate* p = plate_list.get_plate(pidx);
            if (p)
                obj_info["plate_name"] = p->get_plate_name();
        }

        // Bounding box (with instance transform)
        BoundingBoxf3 bb = mo->instance_bounding_box(0);
        obj_info["dimensions"] = { 
            std::round(bb.size().x() * 100.0) / 100.0,
            std::round(bb.size().y() * 100.0) / 100.0,
            std::round(bb.size().z() * 100.0) / 100.0 
        };
        obj_info["position"] = {
            std::round(bb.center().x() * 100.0) / 100.0,
            std::round(bb.center().y() * 100.0) / 100.0,
            std::round(bb.min.z() * 100.0) / 100.0
        };

        // Mesh stats
        try {
            const TriangleMesh& raw = mo->raw_mesh();
            const TriangleMeshStats& mesh_stats = raw.stats();

            obj_info["volume_mm3"]     = std::round(const_cast<TriangleMesh&>(raw).volume() * 100.0) / 100.0;
            obj_info["triangle_count"] = (int)raw.its.indices.size();
            obj_info["vertex_count"]   = (int)raw.its.vertices.size();
            obj_info["is_watertight"]  = mesh_stats.manifold();
            obj_info["open_edges"] = mesh_stats.open_edges;
            obj_info["non_manifold_edge_count"] = mesh_stats.manifold() ? 0 : mesh_stats.open_edges;

            wxString sidebar_info;
            int non_manifold_edges = 0;
            std::string warning_icon_name;
            std::string tooltip_text;
            if (auto* obj_list = wxGetApp().obj_list()) {
                const MeshErrorsInfo mesh_errors_info = obj_list->get_mesh_errors_info((int)oi, -1, &sidebar_info, &non_manifold_edges);
                warning_icon_name = mesh_errors_info.warning_icon_name;
                if (!mesh_errors_info.tooltip.empty())
                    tooltip_text = into_u8(mesh_errors_info.tooltip);
            }

            json mesh_errors = {
                {"has_errors", mesh_stats.repaired() || !mesh_stats.manifold()},
                {"is_manifold", mesh_stats.manifold()},
                {"is_watertight", mesh_stats.manifold()},
                {"repaired", mesh_stats.repaired()},
                {"open_edges", mesh_stats.open_edges},
                {"non_manifold_edge_count", mesh_stats.manifold() ? 0 : mesh_stats.open_edges},
                {"warning_icon_name", warning_icon_name},
                {"sidebar_info", sidebar_info.empty() ? std::string() : into_u8(sidebar_info)},
                {"tooltip", tooltip_text}
            };
            obj_info["mesh_errors"] = mesh_errors;

            if (mesh_stats.open_edges > 0) {
                json mesh_error_summary = {
                    {"object_index", (int)oi},
                    {"object_name", mo->name},
                    {"plate_index", pidx},
                    {"plate_name", obj_info.value("plate_name", std::string())},
                    {"issue_type", "mesh_non_manifold"},
                    {"severity", "warning"},
                    {"blocking", false},
                    {"open_edges", mesh_stats.open_edges},
                    {"non_manifold_edge_count", non_manifold_edges > 0 ? non_manifold_edges : mesh_stats.open_edges},
                    {"is_watertight", mesh_stats.manifold()},
                    {"sidebar_info", sidebar_info.empty() ? std::string() : into_u8(sidebar_info)},
                    {"tooltip", tooltip_text}
                };
                mesh_errors_arr.push_back(mesh_error_summary);

                std::string title = sidebar_info.empty()
                    ? ("Error: " + std::to_string(mesh_stats.open_edges) + " non-manifold edges.")
                    : into_u8(sidebar_info);
                precheck_issues.push_back({
                    {"issue_id", "mesh_non_manifold_obj_" + std::to_string(oi)},
                    {"issue_type", "mesh_non_manifold"},
                    {"severity", "warning"},
                    {"blocking", false},
                    {"title", title},
                    {"description", title},
                    {"candidate_tools", json::array({"repair_mesh"})},
                    {"evidence", {
                        {"object_ids", json::array({std::to_string(oi)})},
                        {"object_index", (int)oi},
                        {"object_name", mo->name},
                        {"plate_index", pidx},
                        {"open_edges", mesh_stats.open_edges},
                        {"non_manifold_edge_count", non_manifold_edges > 0 ? non_manifold_edges : mesh_stats.open_edges},
                        {"is_watertight", mesh_stats.manifold()}
                    }}
                });
            }
        } catch (...) {
            obj_info["volume_mm3"]     = 0;
            obj_info["triangle_count"] = 0;
            obj_info["vertex_count"]   = 0;
            obj_info["is_watertight"]  = false;
            obj_info["open_edges"] = 0;
            obj_info["non_manifold_edge_count"] = 0;
            obj_info["mesh_errors"] = {
                {"has_errors", false},
                {"is_manifold", false},
                {"is_watertight", false},
                {"repaired", false},
                {"open_edges", 0},
                {"non_manifold_edge_count", 0},
                {"warning_icon_name", std::string()},
                {"sidebar_info", std::string()},
                {"tooltip", std::string()}
            };
        }

        // Extruder assignment (first model-part volume)
        int extruder_id = 1;
        for (const ModelVolume* vol : mo->volumes) {
            if (vol->is_model_part()) {
                extruder_id = vol->extruder_id();
                if (extruder_id <= 0) extruder_id = 1;
                break;
            }
        }
        obj_info["extruder_id"] = extruder_id;

        // the following logic is just for test, because it will cost running time
        //OverhangResult oh = AnalyzeOverhang(*mo, support_threshold);
        //obj_info["overhang"] = {
        //    {"ratio", std::round(oh.overhang_ratio * 1000.0) / 1000.0},
        //    {"unsupported_ratio", std::round(oh.unsupported_area_ratio * 1000.0) / 1000.0},
        //    {"downward_ratio", std::round(oh.downward_area_ratio * 1000.0) / 1000.0},
        //    {"bottom_contact_ratio", std::round(oh.bottom_contact_ratio * 1000.0) / 1000.0},
        //    {"bottom_contact_area_mm2", std::round(oh.bottom_contact_area_mm2 * 100.0) / 100.0},
        //    {"max_angle", std::round(oh.max_overhang_angle * 10.0) / 10.0}
        //};

        objects_arr.push_back(obj_info);
    }
    state["objects"] = objects_arr;
    state["mesh_errors"] = mesh_errors_arr;
    state["precheck_issues"] = precheck_issues;
    json selection_info;
    selection_info["is_empty"] = true;
    selection_info["mode"] = "unknown";
    selection_info["selected_object_count"] = 0;
    selection_info["selected_objects"] = json::array();

    if (auto* canvas = plater->canvas3D()) {
        const Selection& selection = canvas->get_selection();
        selection_info["is_empty"] = selection.is_empty();
        selection_info["mode"] = selection.is_instance_mode() ? "instance" : "volume";
        
        const auto& selected_content = selection.get_content();
        json selected_objects = json::array();
        for (const auto& obj_it : selected_content) {
            const int selected_obj_idx = obj_it.first;
            if (selected_obj_idx < 0 || selected_obj_idx >= (int)model.objects.size())
                continue;
            const ModelObject* selected_obj = model.objects[selected_obj_idx];
            if (!selected_obj)
                continue;

            json selected_item;
            selected_item["object_index"] = selected_obj_idx;
            selected_item["object_name"] = selected_obj->name;

            json selected_instance_indices = json::array();
            for (int inst_idx : obj_it.second)
                selected_instance_indices.push_back(inst_idx);

            selected_item["selected_instance_indices"] = selected_instance_indices;
            selected_item["selected_instance_count"] = (int)obj_it.second.size();
            selected_item["instance_count"] = (int)selected_obj->instances.size();
            selected_item["all_instances_selected"] = !selected_obj->instances.empty() && ((int)obj_it.second.size() == (int)selected_obj->instances.size());
            selected_objects.push_back(std::move(selected_item));
        }

        selection_info["selected_object_count"] = (int)selected_objects.size();
        selection_info["selected_objects"] = selected_objects;
        selection_info["is_single_full_object"] = selection.is_single_full_object();
        selection_info["is_single_full_instance"] = selection.is_single_full_instance();

        const int active_object_idx = selection.get_object_idx();
        if (active_object_idx >= 0 && active_object_idx < (int)model.objects.size() && model.objects[active_object_idx]) {
            selection_info["active_object_index"] = active_object_idx;
            selection_info["active_object_name"] = model.objects[active_object_idx]->name;
        }

        const int active_instance_idx = selection.get_instance_idx();
        if (active_instance_idx >= 0)
            selection_info["active_instance_index"] = active_instance_idx;
    }
    state["selection"] = selection_info;

    // --- Wipe tower ---
    if (bundle) {
        const auto& full_cfg = bundle->full_config();
        std::string prime_str = get_opt(print_cfg, "enable_prime_tower");
        bool prime_tower_enabled = (prime_str == "1");
        state["wipe_tower"]["enabled"] = prime_tower_enabled;
        if (prime_tower_enabled) {
            std::string w = get_opt(print_cfg, "prime_tower_width");
            if (!w.empty()) {
                try { state["wipe_tower"]["width"] = std::stod(w); }
                catch (...) {}
            }
            if (!model.wipe_tower.positions.empty()) {
                auto& pos = model.wipe_tower.positions[0];
                state["wipe_tower"]["position"] = { pos.x(), pos.y() };
            }
        }
    }

    // --- Inter-model collision warnings (from NotificationManager) ---
    // Read existing warnings from the UI instead of re-computing distances.
    {
        auto* nm = plater->get_notification_manager();
        if (nm) {
            json all_notifs = nm->get_all_notification();
            json inter_warnings = json::array();
            for (const auto& n : all_notifs) {
                if (n.value("type", "") == "object_info_warning")
                    inter_warnings.push_back(n.value("text", ""));
            }
            if (!inter_warnings.empty())
                state["inter_model"]["warnings"] = inter_warnings;
        }
    }

    // --- Key settings summary ---
    if (bundle) {
        state["settings"]["support_enabled"]   = get_opt(print_cfg, "enable_support");
        state["settings"]["support_type"]      = get_opt(print_cfg, "support_type");
        state["settings"]["layer_height"]      = get_opt(print_cfg, "layer_height");
        state["settings"]["infill_density"]    = get_opt(print_cfg, "sparse_infill_density");
        state["settings"]["prime_tower"]       = get_opt(print_cfg, "enable_prime_tower");
    }

    // --- Active UI notifications (warnings/errors from NotificationManager) ---
    json ui_notifs = json::array();
    if (auto* nm = plater->get_notification_manager())
        ui_notifs = nm->get_all_notification();
    state["ui_notifications"] = ui_notifs;

    // --- Auto-generated and normalized warnings ---
    json warnings = json::array();
    const size_t current_plate_used_filament_count =
        current_plate != nullptr ? current_plate->get_extruders(true).size() : 0;
    if (current_plate_used_filament_count > 1) {
        std::string pt_str = get_opt(print_cfg, "enable_prime_tower");
        if (pt_str != "1") {
            warnings.push_back({
                {"level", "info"},
                {"message", _u8L("Multi-filament detected but prime/wipe tower is disabled")}
            });
        }
    }

    if (state.contains("inter_model") && state["inter_model"].value("too_close", false)) {
        warnings.push_back({
            {"level", "warning"},
            {"message", _u8L("Some models are very close together (< 2mm), risk of collision")}
        });
    }

    for (size_t ni = 0; ni < ui_notifs.size(); ++ni) {
        AppendPrecheckIssueFromNotification(
            warnings,
            precheck_issues,
            ui_notifs[ni],
            static_cast<int>(ni),
            current_plate_index);
    }

    bool slice_toolpath_outside = false;
    if (state.contains("current_plate") && state["current_plate"].is_object())
        slice_toolpath_outside = state["current_plate"].value("slice_toolpath_outside", false);
    if (slice_toolpath_outside)
        AppendToolpathOutsideIssueIfNeeded(warnings, precheck_issues, current_plate_index);

    state["warnings"] = warnings;
    state["precheck_issues"] = precheck_issues;

    return {{"success", true}, {"message", "OK"}, {"state", state}};
}

json SlicerBridge::DoGetSceneDiagnostics(const json& params)
{
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};

    const bool force_validate = !params.is_object() || params.value("force_validate", true);
    const bool include_scene_context = !params.is_object() || params.value("include_scene_context", true);
    const bool include_notifications = !params.is_object() || params.value("include_notifications", true);
    const bool include_precheck_issues = !params.is_object() || params.value("include_precheck_issues", true);
    const bool include_plate_validation = !params.is_object() || params.value("include_plate_validation", true);

    bool model_fits = false;
    bool validate_error = false;
    if (force_validate) {
        plater->update(false, true);
        plater->validate_current_plate(model_fits, validate_error);
    }

    json state = json::object();
    if (include_scene_context) {
        json state_result = DoGetSlicerState(json::object());
        if (!state_result.value("success", false))
            return state_result;
        state = state_result.value("state", json::object());
    }

    json current_plate_validation = json::object();
    if (include_plate_validation)
        current_plate_validation = BuildCurrentPlateValidationSnapshot(plater, model_fits, validate_error);

    json diagnostics = json::object();
    diagnostics["warnings"] = include_scene_context ? state.value("warnings", json::array()) : json::array();
    diagnostics["precheck_issues"] = include_precheck_issues && include_scene_context ? state.value("precheck_issues", json::array()) : json::array();
    diagnostics["ui_notifications"] = include_notifications && include_scene_context ? state.value("ui_notifications", json::array()) : json::array();
    diagnostics["current_plate_validation"] = current_plate_validation;
    diagnostics["issues"] = include_plate_validation ? BuildStructuredSceneIssuesFromPlateValidation(current_plate_validation) : json::array();
    if (include_precheck_issues && include_plate_validation)
        AppendCurrentPlateValidationIssues(diagnostics["warnings"], diagnostics["precheck_issues"], current_plate_validation);

    const bool has_warnings = !diagnostics["warnings"].empty();
    const bool has_issues = !diagnostics["issues"].empty();
    const bool has_precheck_issues = !diagnostics["precheck_issues"].empty();
    const bool has_overlap = current_plate_validation.value("has_model_overlap", false);
    const bool has_outside = current_plate_validation.value("has_model_outside", false);
    const bool validate_failed = current_plate_validation.value("validate_error", false);
    diagnostics["warning_count"] = diagnostics["warnings"].size() + diagnostics["issues"].size();
    diagnostics["error_count"] = validate_failed ? 1 : 0;
    diagnostics["has_blocking_issue"] = has_warnings || has_issues || has_precheck_issues || has_overlap || has_outside || validate_failed;

    json scene = {
        {"client_id", params.is_object() ? params.value("client_id", std::string()) : std::string()},
        {"scene_id", params.is_object() ? params.value("scene_id", std::string()) : std::string()},
        {"session_id", params.is_object() ? params.value("session_id", std::string()) : std::string()},
        {"scope", params.is_object() ? params.value("scope", std::string("current_plate")) : std::string("current_plate")},
        {"revision", state.value("revision", 0)},
        {"plate_index", current_plate_validation.value("plate_index", -1)},
        {"plate_count", state.contains("plates") && state["plates"].is_array() ? static_cast<int>(state["plates"].size()) : 0}
    };

    json result = {
        {"success", true},
        {"message", "OK"},
        {"schema_version", "1.0"},
        {"source", "cxx_slicer"},
        {"scene", scene},
        {"diagnostics", diagnostics}
    };
    if (params.is_object() && params.contains("request_id"))
        result["request_id"] = params["request_id"];
    if (include_scene_context)
        result["scene_context"] = state;
    return result;
}
json SlicerBridge::DoGetSceneWarnings(const json& /*params*/)
{
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}, {"warnings", json::array()}};

    auto* nm = plater->get_notification_manager();
    if (!nm)
        return {{"success", true}, {"message", "OK"}, {"warnings", json::array()}};

    json ui_notifs = nm->get_all_notification();
    return {{"success", true}, {"message", "OK"}, {"warnings", ui_notifs}};
}
} // namespace Bridge
} // namespace GUI
} // namespace Slic3r

