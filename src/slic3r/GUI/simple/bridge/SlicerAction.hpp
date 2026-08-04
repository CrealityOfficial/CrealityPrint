#ifndef slic3r_SlicerAction_hpp_
#define slic3r_SlicerAction_hpp_

#include <string>
#include <vector>

namespace Slic3r {
namespace GUI {
namespace Bridge {

// ---------------------------------------------------------------------------
// Parameter definition for an action
// ---------------------------------------------------------------------------

struct ParamDef {
    std::string name;           // e.g. "type"
    std::string type;           // "string" | "number" | "bool"
    std::string description;    // human-readable description
    bool        required;       // whether the parameter is mandatory
    std::string default_value;  // default value (empty = none)
};

// ---------------------------------------------------------------------------
// Action definition �C metadata for one slicer operation
// ---------------------------------------------------------------------------

struct ActionDef {
    std::string              id;               // unique key, e.g. "start_slice"
    std::string              name_zh;          // Chinese display name
    std::string              name_en;          // English display name
    std::string              description;      // what the action does (for system prompt)
    bool                     requires_confirm; // true = front-end must confirm before executing
    std::vector<ParamDef>    params;           // parameter definitions
};

// ---------------------------------------------------------------------------
// Action result returned after execution
// ---------------------------------------------------------------------------

struct ActionResult {
    bool        success = false;
    std::string action_id;
    std::string message;        // human-readable result summary
    std::string json_payload;   // optional: serialised JSON detail
};

// ---------------------------------------------------------------------------
// Built-in action IDs (string constants for type-safety)
// ---------------------------------------------------------------------------

namespace ActionID {
    constexpr const char* GET_PRESETS       = "get_presets";
    constexpr const char* SELECT_PRESET     = "select_preset";
    constexpr const char* LIST_PRINTERS = "list_printers";
    constexpr const char* SELECT_PRINTER = "select_printer";
    constexpr const char* APPLY_CONFIG      = "apply_config";
    constexpr const char* SET_OBJECT_COLOR  = "set_object_color";
    constexpr const char* GET_EDITED_CONFIG = "get_edited_config";
    constexpr const char* GET_SLICER_STATE  = "get_slicer_state";
    constexpr const char* GET_SCENE_DIAGNOSTICS = "get_scene_diagnostics";
    constexpr const char* CAPTURE_MODEL_VIEWS = "capture_model_views";
    constexpr const char* IMPORT_MODEL      = "import_model";
    constexpr const char* IMPORT_MODEL_FROM_SEARCH = "import_model_from_search";
    constexpr const char* OPEN_MODEL_LIBRARY = "open_model_library";
    constexpr const char* RECOMMEND_MODEL = "recommend_model";
    constexpr const char* SMART_MODEL_SEARCH = "smart_model_search";
    constexpr const char* AUTO_ORIENT       = "auto_orient";
    constexpr const char* REPAIR_MESH       = "repair_mesh";
    constexpr const char* AUTO_ARRANGE      = "auto_arrange";
    constexpr const char* UNDO              = "undo";
    constexpr const char* REDO              = "redo";
    constexpr const char* START_SLICE       = "start_slice";
    constexpr const char* SEND_TO_PRINTER   = "send_to_printer";
    constexpr const char* EXPORT_GCODE      = "export_gcode";
    constexpr const char* GET_CONFIG_OPTIONS = "get_config_options";
    constexpr const char* MOVE_OBJECT       = "move_object";
    constexpr const char* ROTATE_OBJECT     = "rotate_object";
    constexpr const char* SCALE_OBJECT      = "scale_object";
    constexpr const char* SELECT_OBJECTS    = "select_objects";
    constexpr const char* DELETE_MODEL      = "delete_model";
    constexpr const char* CLONE_MODEL       = "clone_model";
    constexpr const char* ARRANGE_SINGLE_PLATE = "arrange_single_plate";
    constexpr const char* ARRANGE_ALL_PLATES   = "arrange_all_plates";
    constexpr const char* FILL_BED          = "fill_bed";
    constexpr const char* GET_SCENE_WARNINGS = "get_scene_warnings";
    constexpr const char* ADD_FILAMENT      = "add_filament";
    constexpr const char* DELETE_FILAMENT   = "delete_filament";
    constexpr const char* SET_FILAMENT_TYPE = "set_filament_type";
    constexpr const char* AUTO_MAP_FILAMENTS = "auto_map_filaments";
    constexpr const char* MOVE_PRINT_HEAD   = "move_print_head";
    constexpr const char* PRINT_CONTROL     = "print_control";
    constexpr const char* EDIT_PLATE_NAME   = "edit_plate_name";
    constexpr const char* SIMPLIFY_MODEL    = "simplify_model";
    constexpr const char* SPLIT_MODEL       = "split_model";
    constexpr const char* ADD_TEST_MODEL    = "add_test_model";
    constexpr const char* ADD_PLATE         = "add_plate";
    constexpr const char* DELETE_PLATE      = "delete_plate";
    constexpr const char* TOGGLE_PREVIEW_LITE_MODE = "toggle_preview_lite_mode";
    constexpr const char* NEW_PROJECT         = "new_project";
    constexpr const char* SAVE_AND_CREATE_NEW_PROJECT = "save_and_create_new_project";
    constexpr const char* OPEN_FILAMENT_MAPPING = "open_filament_mapping";
    constexpr const char* SEND_PRINT          = "send_print";
    constexpr const char* CAPTURE_DEVICE_CAMERA_FRAME = "capture_device_camera_frame";
}

} // namespace Bridge
} // namespace GUI
} // namespace Slic3r

#endif // slic3r_SlicerAction_hpp_
