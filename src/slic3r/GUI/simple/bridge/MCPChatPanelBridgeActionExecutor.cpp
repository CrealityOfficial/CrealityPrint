#include "slic3r/GUI/simple/MCPChatPanel.hpp"

#include "slic3r/GUI/simple/bridge/SlicerBridgeDiagnostics.hpp"

#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/Plater.hpp"

#include <boost/log/trivial.hpp>

#include <unordered_map>
#include <unordered_set>

using json = nlohmann::json;

namespace Slic3r {
namespace GUI {

void MCPChatPanel::ExecuteBridgeAction(const std::string& action_id, const json& data)
{
    // Use CallAfter to defer execution to the main event loop.
    // This is critical because OnScriptMessage runs inside a WebView callback,
    // and UI operations like arrange/orient need the event loop to be free.
    auto action = action_id;
    auto params = data;
    CallAfter([this, action, params]() {
        auto& bridge = Bridge::SlicerBridge::Instance();
       json result = bridge.Execute(action, params);
        if (action == Bridge::ActionID::AUTO_MAP_FILAMENTS &&
            (!params.is_object() || !params.contains("include_project_context") || params.value("include_project_context", true)) &&
            !result.contains("project_context")) {
            json state_result = bridge.Execute(Bridge::ActionID::GET_SLICER_STATE, json::object());
            if (state_result.value("success", false))
                result["project_context"] = state_result.value("state", json::object());
        }

        if (params.contains("request_id") && !result.contains("request_id"))
            result["request_id"] = params["request_id"];

        // Map action_id to a JS event name that the front-end already handles
        static const std::unordered_map<std::string, std::string> event_map = {
            {Bridge::ActionID::GET_PRESETS,       "presets_data"},
            {Bridge::ActionID::SELECT_PRESET,     "preset_selected"},
            {Bridge::ActionID::APPLY_CONFIG,      "config_applied"},
            {Bridge::ActionID::SET_OBJECT_COLOR,  "object_color_set"},
            {Bridge::ActionID::GET_EDITED_CONFIG, "edited_config"},
            {Bridge::ActionID::GET_SLICER_STATE,  "slicer_state"},
            {Bridge::ActionID::CAPTURE_MODEL_VIEWS, "model_views_captured"},
            {Bridge::ActionID::IMPORT_MODEL,      "model_import"},
            {Bridge::ActionID::OPEN_MODEL_LIBRARY, "open_model_library_result"},
            {Bridge::ActionID::SMART_MODEL_SEARCH, "smart_model_search_result"},
            {Bridge::ActionID::IMPORT_MODEL_FROM_SEARCH, "import_model_from_search_result"},
            {Bridge::ActionID::AUTO_ORIENT,       "auto_orient_done"},
            {Bridge::ActionID::AUTO_ARRANGE,      "auto_arrange_done"},
            {Bridge::ActionID::UNDO,              "undo_result"},
            {Bridge::ActionID::REDO,              "redo_result"},
            {Bridge::ActionID::START_SLICE,       "slice_started"},
            {Bridge::ActionID::SEND_TO_PRINTER,   "print_sent"},
            {Bridge::ActionID::EXPORT_GCODE,      "gcode_exported"},
            {Bridge::ActionID::GET_CONFIG_OPTIONS,"config_options"},
            {Bridge::ActionID::MOVE_OBJECT,       "move_object_result"},
            {Bridge::ActionID::ROTATE_OBJECT,     "rotate_object_result"},
            {Bridge::ActionID::SCALE_OBJECT,      "scale_object_result"},
            {Bridge::ActionID::SELECT_OBJECTS,    "select_objects_result"},
            {Bridge::ActionID::DELETE_MODEL,      "delete_model_result"},
            {Bridge::ActionID::CLONE_MODEL,       "clone_model_result"},
            {Bridge::ActionID::ARRANGE_SINGLE_PLATE, "arrange_single_plate_result"},
            {Bridge::ActionID::ARRANGE_ALL_PLATES,   "arrange_all_plates_result"},
            {Bridge::ActionID::FILL_BED,          "fill_bed_result"},
            {Bridge::ActionID::ADD_FILAMENT,      "filament_added"},
            {Bridge::ActionID::DELETE_FILAMENT,   "filament_deleted"},
            {Bridge::ActionID::SET_FILAMENT_TYPE, "filament_type_set"},
            {Bridge::ActionID::AUTO_MAP_FILAMENTS, "auto_map_filaments"},
        };

        std::string event_name = "action_result";  // fallback
        auto it = event_map.find(action);
        if (it != event_map.end())
            event_name = it->second;

        // Diagnostic reproduction of the old result finalizer: scene-changing
        // tools publish a fresh slicer_state before their tool-result reaches JS.
        static const std::unordered_set<std::string> scene_state_actions = {
            Bridge::ActionID::APPLY_CONFIG,
            Bridge::ActionID::AUTO_ORIENT,
            Bridge::ActionID::AUTO_ARRANGE,
            Bridge::ActionID::UNDO,
            Bridge::ActionID::REDO,
            Bridge::ActionID::MOVE_OBJECT,
            Bridge::ActionID::ROTATE_OBJECT,
            Bridge::ActionID::SCALE_OBJECT,
            Bridge::ActionID::SELECT_OBJECTS,
            Bridge::ActionID::DELETE_MODEL,
            Bridge::ActionID::CLONE_MODEL,
            Bridge::ActionID::ARRANGE_SINGLE_PLATE,
            Bridge::ActionID::ARRANGE_ALL_PLATES,
            Bridge::ActionID::FILL_BED,
        };
        const bool needs_post_action_state = scene_state_actions.count(action) > 0;

        // if (!needs_post_action_state || !result.value("success", false)) {
        //     SendCommandToJS(event_name, result);
        //     return;
        // }

        try {
            auto* plater = wxGetApp().plater();
            bool model_fits = false;
            bool validate_err = false;

            if (plater) {
                plater->update(false, true); // Force background_process + validation to sync before the post-action snapshot.
                plater->validate_current_plate(model_fits, validate_err);
            }

            Bridge::AttachCurrentPlateValidationResult(result, plater, model_fits, validate_err, params);
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(warning)
                << "[MCPChatPanel] plater validation failed after bridge action="
                << action << " error=" << e.what();
        }

        SendCommandToJS(event_name, result);

    });
}

} // namespace GUI
} // namespace Slic3r
