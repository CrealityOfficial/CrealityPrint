#include "slic3r/GUI/simple/toolcalls/MCPToolCallsCommon.hpp"

#include "libslic3r/Utils.hpp"
#include "slic3r/GUI/NotificationManager.hpp"
#include "slic3r/GUI/simple/bridge/SlicerAction.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>
#include <boost/nowide/convert.hpp>
#include <boost/nowide/fstream.hpp>

#include <fstream>
#include <initializer_list>
#include <unordered_set>
#include <vector>

using json = nlohmann::json;

namespace Slic3r {
namespace GUI {
namespace ToolCalls {
namespace {

std::string WebViewLogFilePath()
{
#ifdef _WIN32
    const boost::filesystem::path log_dir = boost::filesystem::path(boost::nowide::widen(Slic3r::data_dir())) / "log";
#else
    const boost::filesystem::path log_dir = boost::filesystem::path(Slic3r::data_dir()) / "log";
#endif
    if (!boost::filesystem::exists(log_dir))
        boost::filesystem::create_directories(log_dir);
#ifdef _WIN32
    return boost::nowide::narrow((log_dir / "chat_webview.log").wstring());
#else
    return (log_dir / "chat_webview.log").string();
#endif
}

bool IsNonBlockingFactNotification(const std::string& text)
{
    if (text.empty())
        return false;

    const std::string lower = boost::algorithm::to_lower_copy(text);
    static const std::initializer_list<const char*> tokens = {
        "updateparameter",
        "update parameter",
        "parameter pack",
        "parameter preset",
    };

    for (const char* token : tokens) {
        if (lower.find(token) != std::string::npos)
            return true;
    }

    return false;
}

std::vector<std::string> CollectBlockingSceneErrors(const json& state)
{
    std::vector<std::string> errors;
    const json notifications = state.value("ui_notifications", json::array());
    if (!notifications.is_array())
        return errors;

    for (const auto& entry : notifications) {
        if (!entry.is_object())
            continue;

        const std::string level = entry.value("level", "");
        if (level != "error" && level != "important")
            continue;

        std::string text = entry.value("text", "");
        if (text.empty())
            text = entry.value("message", "");
        if (text.empty())
            text = entry.value("title", "");
        if (text.empty() || IsNonBlockingFactNotification(text))
            continue;

        errors.push_back(text);
    }

    return errors;
}

std::string MapBlockingErrorCode(const std::string& text)
{
    if (text.empty())
        return "UNKNOWN_BLOCKING_ERROR";

    const std::string lower = boost::algorithm::to_lower_copy(text);
    if (lower.find("height limit") != std::string::npos)
        return "MODEL_EXCEEDS_HEIGHT_LIMIT";
    if (lower.find("outside") != std::string::npos ||
        lower.find("out of bed") != std::string::npos ||
        lower.find("build plate") != std::string::npos ||
        lower.find("boundary") != std::string::npos)
        return "MODEL_OUT_OF_BOUNDS";
    if (lower.find("filament mapping") != std::string::npos ||
        lower.find("material mapping") != std::string::npos ||
        lower.find("map filament") != std::string::npos ||
        lower.find("ams") != std::string::npos)
        return "FILAMENT_MAPPING_REQUIRED";
    if (lower.find("critical") != std::string::npos &&
        lower.find("warning") != std::string::npos)
        return "CRITICAL_SLICE_WARNING";

    return "UNKNOWN_BLOCKING_ERROR";
}

const std::vector<BridgeToolRouteSpec>& BridgeToolRouteSpecsStorage()
{
    using namespace Bridge;

    static const std::vector<BridgeToolRouteSpec> specs = {
        {ActionID::GET_SCENE_DIAGNOSTICS, ActionID::GET_SCENE_DIAGNOSTICS, true, true, "diagnosing", "Collecting scene diagnostics", "Scene diagnostics collected", "GET_SCENE_DIAGNOSTICS_FAILED", "Failed to collect scene diagnostics.", false, false},
        {ActionID::GET_SLICER_STATE, ActionID::GET_SLICER_STATE, true, true, "collecting", "Collecting slicer state", "Slicer state collected", "GET_SLICER_STATE_FAILED", "Failed to collect slicer state.", false, false},
        {"get_project_context", ActionID::GET_SLICER_STATE, false, true, "collecting", "Collecting project context", "Project context collected", "GET_PROJECT_CONTEXT_FAILED", "Failed to collect project context.", false, false},
        {ActionID::GET_SCENE_WARNINGS, ActionID::GET_SCENE_WARNINGS, true, true, "collecting", "Collecting scene warnings", "Scene warnings collected", "GET_SCENE_WARNINGS_FAILED", "Failed to collect scene warnings.", false, false},
        {ActionID::GET_PRESETS, ActionID::GET_PRESETS, true, true, "collecting", "Collecting presets", "Presets collected", "GET_PRESETS_FAILED", "Failed to collect presets.", false, false},
        {"list_presets", ActionID::GET_PRESETS, false, true, "collecting", "Collecting presets", "Presets collected", "LIST_PRESETS_FAILED", "Failed to collect presets.", false, false},
        {ActionID::GET_EDITED_CONFIG, ActionID::GET_EDITED_CONFIG, true, true, "collecting", "Collecting current config", "Current config collected", "GET_EDITED_CONFIG_FAILED", "Failed to collect current config.", false, false},
        {"get_current_slice_params", ActionID::GET_EDITED_CONFIG, false, true, "collecting", "Collecting current slice parameters", "Current slice parameters collected", "CURRENT_SLICE_PARAMS_FAILED", "Failed to collect current slice parameters.", false, false},
        {ActionID::GET_CONFIG_OPTIONS, ActionID::GET_CONFIG_OPTIONS, true, true, "collecting", "Collecting config options", "Config options collected", "GET_CONFIG_OPTIONS_FAILED", "Failed to collect config options.", false, false},
        {"get_config_schema", ActionID::GET_CONFIG_OPTIONS, false, true, "collecting", "Collecting config schema", "Config schema collected", "GET_CONFIG_SCHEMA_FAILED", "Failed to collect config schema.", false, false},
        {ActionID::APPLY_CONFIG, ActionID::APPLY_CONFIG, true, true, "applying", "Applying config", "Config applied", "APPLY_CONFIG_FAILED", "Failed to apply config.", true, true},
        {"apply_param_patch", ActionID::APPLY_CONFIG, false, true, "applying", "Applying parameter patch", "Parameter patch applied", "APPLY_PARAM_PATCH_FAILED", "Failed to apply parameter patch.", true, true},
        {ActionID::AUTO_ORIENT, ActionID::AUTO_ORIENT, true, true, "orienting", "Starting auto orient", "Auto orient completed", "AUTO_ORIENT_FAILED", "Failed to auto orient models.", false, true},
        {"auto_orient_model", ActionID::AUTO_ORIENT, false, true, "orienting", "Starting auto orient", "Auto orient completed", "AUTO_ORIENT_FAILED", "Failed to auto orient models.", false, true},
        {ActionID::AUTO_ARRANGE, ActionID::AUTO_ARRANGE, true, true, "arranging", "Starting auto arrange", "Auto arrange job started", "AUTO_ARRANGE_FAILED", "Failed to auto arrange models.", false, true},
        {"arrange_current_plate", ActionID::ARRANGE_SINGLE_PLATE, false, true, "arranging", "Arranging current plate", "Current plate arrange job started", "ARRANGE_CURRENT_PLATE_FAILED", "Failed to arrange current plate.", false, true},
        {ActionID::ARRANGE_SINGLE_PLATE, ActionID::ARRANGE_SINGLE_PLATE, true, true, "arranging", "Arranging current plate", "Current plate arrange job started", "ARRANGE_SINGLE_PLATE_FAILED", "Failed to arrange current plate.", false, true},
        {ActionID::ARRANGE_ALL_PLATES, ActionID::ARRANGE_ALL_PLATES, true, true, "arranging", "Arranging all plates", "All plates arrange job started", "ARRANGE_ALL_PLATES_FAILED", "Failed to arrange all plates.", false, true},
        {ActionID::MOVE_OBJECT, ActionID::MOVE_OBJECT, true, true, "moving", "Moving object", "Object moved", "MOVE_OBJECT_FAILED", "Failed to move object.", false, true},
        {ActionID::ROTATE_OBJECT, ActionID::ROTATE_OBJECT, true, true, "rotating", "Rotating object", "Object rotated", "ROTATE_OBJECT_FAILED", "Failed to rotate object.", false, true},
        {ActionID::SCALE_OBJECT, ActionID::SCALE_OBJECT, true, true, "scaling", "Scaling object", "Object scaled", "SCALE_OBJECT_FAILED", "Failed to scale object.", false, true},
        {ActionID::SELECT_OBJECTS, ActionID::SELECT_OBJECTS, true, true, "selecting", "Selecting objects", "Objects selected", "SELECT_OBJECTS_FAILED", "Failed to select objects.", false, true},
        {ActionID::DELETE_MODEL, ActionID::DELETE_MODEL, true, false, "", "", "", "", "", false, false},
        {ActionID::CLONE_MODEL, ActionID::CLONE_MODEL, true, false, "", "", "", "", "", false, false},
        {ActionID::REPAIR_MESH, ActionID::REPAIR_MESH, true, true, "repairing", "Starting mesh repair", "Mesh repair completed", "REPAIR_MESH_FAILED", "Failed to repair mesh.", false, true},
        {ActionID::SIMPLIFY_MODEL, ActionID::SIMPLIFY_MODEL, true, false, "", "", "", "", "", false, false},
        {ActionID::ADD_TEST_MODEL, ActionID::ADD_TEST_MODEL, true, false, "", "", "", "", "", false, false},
        {ActionID::FILL_BED, ActionID::FILL_BED, true, false, "", "", "", "", "", false, false},
        {ActionID::EDIT_PLATE_NAME, ActionID::EDIT_PLATE_NAME, true, false, "", "", "", "", "", false, false},
        {"rename_plate", ActionID::EDIT_PLATE_NAME, true, false, "", "", "", "", "", false, false},
        {"set_plate_name", ActionID::EDIT_PLATE_NAME, true, false, "", "", "", "", "", false, false},
        {ActionID::ADD_PLATE, ActionID::ADD_PLATE, true, true, "editing", "Adding plate", "Plate added", "ADD_PLATE_FAILED", "Failed to add plate.", false, true},
        {ActionID::DELETE_PLATE, ActionID::DELETE_PLATE, true, false, "", "", "", "", "", false, false},
        {ActionID::TOGGLE_PREVIEW_LITE_MODE, ActionID::TOGGLE_PREVIEW_LITE_MODE, true, false, "", "", "", "", "", false, false},
        {ActionID::ADD_FILAMENT, ActionID::ADD_FILAMENT, true, false, "", "", "", "", "", false, false},
        {ActionID::DELETE_FILAMENT, ActionID::DELETE_FILAMENT, true, false, "", "", "", "", "", false, false},
        {ActionID::SET_FILAMENT_TYPE, ActionID::SET_FILAMENT_TYPE, true, false, "", "", "", "", "", false, false},
        {ActionID::AUTO_MAP_FILAMENTS, ActionID::AUTO_MAP_FILAMENTS, true, false, "", "", "", "", "", false, false},
        {"auto_match_filaments", ActionID::AUTO_MAP_FILAMENTS, true, false, "", "", "", "", "", false, false},
        {"map_filaments", ActionID::AUTO_MAP_FILAMENTS, true, false, "", "", "", "", "", false, false},
        {"auto_map_materials", ActionID::AUTO_MAP_FILAMENTS, true, false, "", "", "", "", "", false, false},
        {"auto_match_materials", ActionID::AUTO_MAP_FILAMENTS, true, false, "", "", "", "", "", false, false},
        {"map_materials", ActionID::AUTO_MAP_FILAMENTS, true, false, "", "", "", "", "", false, false},
        {"switch_printer", ActionID::SELECT_PRINTER, true, false, "", "", "", "", "", false, false},
        {"change_printer", ActionID::SELECT_PRINTER, true, false, "", "", "", "", "", false, false},
        {"select_device", ActionID::SELECT_PRINTER, true, false, "", "", "", "", "", false, false},
        {ActionID::UNDO, ActionID::UNDO, true, true, "undoing", "Undoing last action", "Undo completed", "UNDO_FAILED", "Failed to undo last action.", false, true},
        {ActionID::REDO, ActionID::REDO, true, true, "redoing", "Redoing last undone action", "Redo completed", "REDO_FAILED", "Failed to redo last action.", false, true}
    };

    return specs;
}

} // namespace

const std::vector<BridgeToolRouteSpec>& GetBridgeToolRouteSpecs()
{
    return BridgeToolRouteSpecsStorage();
}

const BridgeToolRouteSpec* FindBridgeToolRouteSpec(const std::string& tool)
{
    for (const auto& spec : BridgeToolRouteSpecsStorage()) {
        if (tool == spec.tool)
            return &spec;
    }
    return nullptr;
}

json BuildBlockingErrorsPayload(const json& state)
{
    json items = json::array();
    std::unordered_set<std::string> seen_keys;

    auto append_error = [&items, &seen_keys](const std::string& code,
                                             const std::string& source,
                                             const std::string& message) {
        if (code.empty())
            return;

        const std::string dedupe_key = code + "|" + source;
        if (seen_keys.count(dedupe_key) > 0)
            return;

        seen_keys.insert(dedupe_key);
        items.push_back({
            {"code", code},
            {"source", source},
            {"message", message},
        });
    };

    const bool has_model = state.value("has_model", false);
    const json current_plate = state.value("current_plate", json::object());
    const bool has_printable_instances = current_plate.contains("has_printable_instances")
        ? current_plate.value("has_printable_instances", false)
        : has_model;

    if (!has_model) {
        append_error("NO_MODEL_LOADED", "project_state", "No model is loaded.");
    } else if (!has_printable_instances) {
        append_error("NO_PRINTABLE_INSTANCES", "project_state", "No printable instances are available on the current plate.");
    }

    if (current_plate.value("slice_has_critical_warning", false))
        append_error("CRITICAL_SLICE_WARNING", "project_state", "Critical slice warning is active.");

    for (const auto& text : CollectBlockingSceneErrors(state))
        append_error(MapBlockingErrorCode(text), "ui_notifications", text);

    return items;
}

json BuildExplicitFactsFromState(const json& state)
{
    const json current_plate = state.value("current_plate", json::object());
    const json current_device = state.value("current_device", json::object());
    const bool has_model = state.value("has_model", false);
    const bool has_printable_instances = current_plate.contains("has_printable_instances")
        ? current_plate.value("has_printable_instances", false)
        : has_model;
    const bool slice_ready_for_print = current_plate.value("slice_result_ready_for_print", false);
    const bool gcode_available =
        current_plate.value("valid_gcode_file", false) ||
        slice_ready_for_print;

    const json blocking_errors = BuildBlockingErrorsPayload(state);
    bool no_blocking_errors = blocking_errors.empty();
    bool layout_valid = true;
    bool filament_mapping_valid = true;

    for (const auto& item : blocking_errors) {
        if (!item.is_object())
            continue;

        const std::string code = item.value("code", "");
        if (code.empty())
            continue;

        no_blocking_errors = false;
        if (code == "MODEL_OUT_OF_BOUNDS" || code == "MODEL_EXCEEDS_HEIGHT_LIMIT")
            layout_valid = false;
        if (code == "FILAMENT_MAPPING_REQUIRED")
            filament_mapping_valid = false;
    }

    return {
        {"project.has_model", has_model},
        {"plate.current.has_objects", !current_plate.value("empty", false)},
        {"plate.current.has_printable_instances", has_printable_instances},
        {"plate.current.slice_completed", current_plate.value("slice_result_valid", false)},
        {"plate.current.slice_ready_for_print", slice_ready_for_print},
        {"plate.current.gcode_available", gcode_available},
        {"plate.current.no_critical_warnings", !current_plate.value("slice_has_critical_warning", false)},
        {"device.current.valid", current_device.value("valid", false)},
        {"device.current.online", current_device.value("online", false)},
        {"device.current.idle", current_device.value("is_idle", false)},
        {"scene.no_blocking_errors", no_blocking_errors},
        {"scene.layout.valid", layout_valid},
        {"scene.filament_mapping.valid", filament_mapping_valid},
    };
}

void AttachExplicitFactsFromState(json& payload, const json& state)
{
    payload["facts"] = BuildExplicitFactsFromState(state);
    payload["scene"] = {
        {"blocking_errors", BuildBlockingErrorsPayload(state)},
    };
}

json BuildGeometryAnalysisFromState(const json& state)
{
    json geometry_analysis = {
        {"has_model", state.value("has_model", false)},
        {"object_count", state.value("model_count", 0)},
        {"plate_count", state.value("plate_count", 0)},
        {"objects", json::array()},
        {"warnings", state.value("warnings", json::array())},
        {"ui_notifications", state.value("ui_notifications", json::array())}
    };

    if (state.contains("inter_model") &&
        state["inter_model"].is_object() &&
        state["inter_model"].contains("warnings"))
        geometry_analysis["inter_model"] = {{"warnings", state["inter_model"]["warnings"]}};

    if (state.contains("objects") && state["objects"].is_array()) {
        for (const auto& object : state["objects"]) {
            if (!object.is_object())
                continue;

            geometry_analysis["objects"].push_back({
                {"object_index", object.value("object_index", -1)},
                {"name", object.value("name", std::string())},
                {"plate_index", object.value("plate_index", -1)},
                {"plate_name", object.value("plate_name", std::string())},
                {"dimensions", object.value("dimensions", json::array())},
                {"position", object.value("position", json::array())},
                {"volume_mm3", object.value("volume_mm3", 0.0)},
                {"triangle_count", object.value("triangle_count", 0)},
                {"extruder_id", object.value("extruder_id", 1)}
            });
        }
    }

    return geometry_analysis;
}

json BuildVisualRecommendationGeometryFromState(const json& state)
{
    json visual_geometry = {
        {"has_model", state.value("has_model", false)},
        {"object_count", state.value("model_count", 0)},
        {"plate_count", state.value("plate_count", 0)},
        {"dimensions", json::array()},
        {"volume_mm3", 0.0},
        {"face_count", 0},
        {"vertex_count", nullptr},
        {"is_watertight", nullptr},
        {"primary_object", nullptr},
        {"objects", json::array()},
        {"warnings", state.value("warnings", json::array())},
        {"ui_notifications", state.value("ui_notifications", json::array())}
    };

    if (state.contains("inter_model") &&
        state["inter_model"].is_object() &&
        state["inter_model"].contains("warnings"))
        visual_geometry["inter_model"] = {{"warnings", state["inter_model"]["warnings"]}};

    double total_volume_mm3 = 0.0;
    int total_face_count = 0;
    int total_vertex_count = 0;
    bool all_watertight = true;
    bool has_watertight_value = false;
    bool primary_object_set = false;

    if (state.contains("objects") && state["objects"].is_array()) {
        for (const auto& object : state["objects"]) {
            if (!object.is_object())
                continue;

            const int triangle_count = object.value("triangle_count", 0);
            const int vertex_count = object.value("vertex_count", 0);
            const double volume_mm3 = object.value("volume_mm3", 0.0);
            const bool object_is_watertight = object.value("is_watertight", false);
            json item = {
                {"object_index", object.value("object_index", -1)},
                {"name", object.value("name", std::string())},
                {"plate_index", object.value("plate_index", -1)},
                {"plate_name", object.value("plate_name", std::string())},
                {"dimensions", object.value("dimensions", json::array())},
                {"position", object.value("position", json::array())},
                {"volume_mm3", volume_mm3},
                {"face_count", triangle_count},
                {"triangle_count", triangle_count},
                {"vertex_count", vertex_count},
                {"is_watertight", object_is_watertight},
                {"extruder_id", object.value("extruder_id", 1)}
            };

            visual_geometry["objects"].push_back(item);
            total_volume_mm3 += volume_mm3;
            total_face_count += triangle_count;
            total_vertex_count += vertex_count;
            has_watertight_value = true;
            all_watertight = all_watertight && object_is_watertight;

            if (!primary_object_set) {
                visual_geometry["primary_object"] = item;
                visual_geometry["dimensions"] = item["dimensions"];
                primary_object_set = true;
            }
        }
    }

    visual_geometry["volume_mm3"] = total_volume_mm3;
    visual_geometry["face_count"] = total_face_count;
    visual_geometry["vertex_count"] = total_vertex_count;
    visual_geometry["is_watertight"] = has_watertight_value ? json(all_watertight) : json(nullptr);
    return visual_geometry;
}

json NormalizeApplyParamPatchArgs(const json& args)
{
    if (!args.is_object())
        return json::object();

    if (!args.contains("patch") || !args["patch"].is_array())
        return args;

    json normalized = json::object();
    for (const auto& item : args["patch"]) {
        if (!item.is_object())
            continue;

        const std::string key = item.value("key", "");
        if (key.empty() || !item.contains("to"))
            continue;

        normalized[key] = item["to"];
    }

    return normalized;
}

std::string SafeJsonDumpForLog(const json& value)
{
    try {
        return value.dump();
    } catch (...) {
        return "<json_dump_failed>";
    }
}

void LogAISendPanelStage(const std::string& stage,
                         const std::string& card_id,
                         const std::string& request_id,
                         const std::string& message)
{
    BOOST_LOG_TRIVIAL(info)
        << "[MCPChatPanel][AISend] stage=" << stage
        << " card_id=" << card_id
        << " request_id=" << request_id
        << " " << message;
}

void ClearWebViewLogFile()
{
    boost::nowide::ofstream stream(WebViewLogFilePath(), std::ios::out | std::ios::trunc);
}

void AppendWebViewLogLine(const json& payload)
{
    const std::string level = payload.value("level", "log");
    const std::string timestamp = payload.value("timestamp", "");
    boost::nowide::ofstream stream(WebViewLogFilePath(), std::ios::out | std::ios::app);
    if (!stream.is_open())
        return;

    stream << '[' << (timestamp.empty() ? "no-ts" : timestamp) << "] [" << level << "] ";
    if (payload.contains("args") && payload["args"].is_array()) {
        bool first = true;
        for (const auto& item : payload["args"]) {
            if (!first)
                stream << " | ";
            first = false;
            if (item.is_string())
                stream << item.get<std::string>();
            else
                stream << item.dump();
        }
    } else {
        stream << payload.dump();
    }
    stream << std::endl;
}

void DismissNotificationEntry(Slic3r::GUI::NotificationManager* notification_manager,
                              const json& entry)
{
    if (!notification_manager || !entry.is_object())
        return;

    const std::string type = entry.value("type", "");
    const std::string text = entry.value("text", "");

    if (type == "plater_error") {
        if (!text.empty())
            notification_manager->close_plater_error_notification(text);
        return;
    }
    if (type == "plater_warning") {
        if (!text.empty())
            notification_manager->close_plater_warning_notification(text);
        return;
    }
    if (type == "slicing_error") {
        if (!text.empty())
            notification_manager->close_slicing_error_notification(text);
        else
            notification_manager->close_slicing_errors_and_warnings();
        return;
    }
    if (type == "slicing_warning") {
        notification_manager->close_slicing_errors_and_warnings();
        return;
    }
    if (type == "slicing_serious_warning") {
        if (!text.empty())
            notification_manager->close_slicing_serious_warning_notification(text);
        else
            notification_manager->close_notification_of_type(Slic3r::GUI::NotificationType::SlicingSeriousWarning);
        return;
    }
    if (type == "validate_error") {
        notification_manager->close_notification_of_type(Slic3r::GUI::NotificationType::ValidateError);
        return;
    }
    if (type == "validate_warning") {
        notification_manager->close_notification_of_type(Slic3r::GUI::NotificationType::ValidateWarning);
        return;
    }
    if (type == "object_info_warning") {
        notification_manager->bbl_close_objectsinfo_notification();
        return;
    }

    if (!text.empty())
        notification_manager->close_slicing_serious_warning_notification(text);
}

} // namespace ToolCalls
} // namespace GUI
} // namespace Slic3r
