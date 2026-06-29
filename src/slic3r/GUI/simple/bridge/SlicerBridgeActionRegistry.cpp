#include "SlicerBridge.hpp"

#include <boost/log/trivial.hpp>

using json = nlohmann::json;

namespace Slic3r {
namespace GUI {
namespace Bridge {

void SlicerBridge::RegisterAllActions()
{
    // ----- query actions (no confirmation needed) -----

    RegisterAction(
        { ActionID::GET_SLICER_STATE,
          "\xe5\x88\x87\xe7\x89\x87\xe5\x99\xa8\xe7\x8a\xb6\xe6\x80\x81",  // Slicer state (localized Chinese label)
          "Slicer State",
          "Get comprehensive scene state: models with geometry analysis (dimensions, position, overhang detection), "
          "filament info, wipe tower status, inter-model distances, settings summary, and auto-generated warnings.",
          false, {} },
        [this](const json& p) { return DoGetSlicerState(p); });
    RegisterAction(
        { ActionID::GET_SCENE_DIAGNOSTICS,
          "Get Scene Diagnostics",
          "Get Scene Diagnostics",
          "Return normalized scene diagnostics for automatic repair orchestration, including notifications, precheck issues, current plate validation, and optional scene context.",
          false,
          { {"force_validate", "bool", "Force current plate validation before building diagnostics.", false, "true"},
            {"include_scene_context", "bool", "Include full get_slicer_state scene context.", false, "true"},
            {"include_notifications", "bool", "Include active UI notifications.", false, "true"},
            {"include_precheck_issues", "bool", "Include normalized precheck issues.", false, "true"},
            {"include_plate_validation", "bool", "Include current plate validation snapshot.", false, "true"},
            {"scope", "string", "Diagnostic scope. Currently supports current_plate.", false, "current_plate"} } },
        [this](const json& p) { return DoGetSceneDiagnostics(p); });

    RegisterAction(
        { ActionID::CAPTURE_MODEL_VIEWS,
          "???????",
          "Capture Model Views",
          "Capture multi-view screenshots for the selected or specified model object and return PNG data URLs.",
          false,
          { {"object_index", "number", "Target model object index. Defaults to the current selection when omitted.", false, ""},
            {"views", "string[]|string", "Requested views. Defaults to front,rear,left,right,top,bottom,iso,iso_front_right,iso_rear_right,iso_rear_left.", false, "front,rear,left,right,top,bottom,iso,iso_front_right,iso_rear_right,iso_rear_left"},
            {"width", "number", "Output image width in pixels.", false, "512"},
            {"height", "number", "Output image height in pixels.", false, "512"} } },
        [this](const json& p) { return DoCaptureModelViews(p); });

    RegisterAction(
        { ActionID::GET_PRESETS,
          "\xe8\x8e\xb7\xe5\x8f\x96\xe9\xa2\x84\xe8\xae\xbe",  // Get presets (localized Chinese label)
          "Get Presets",
          "List available presets (print / filament / printer).",
          false,
          { {"type", "string", "Preset type: 'all', 'print', 'filament', or 'printer'", false, "all"} } },
        [this](const json& p) { return DoGetPresets(p); });

    RegisterAction(
        { ActionID::GET_EDITED_CONFIG,
          "\xe8\x8e\xb7\xe5\x8f\x96\xe5\xbd\x93\xe5\x89\x8d\xe9\x85\x8d\xe7\xbd\xae",  // Get current config (localized Chinese label)
          "Get Current Config",
          "Return key print/filament/printer parameters of the active configuration.",
          false, {} },
        [this](const json& p) { return DoGetEditedConfig(p); });

    // ----- mutation actions (some need confirmation) -----

    RegisterAction(
        { ActionID::SELECT_PRESET,
          "\xe9\x80\x89\xe6\x8b\xa9\xe9\xa2\x84\xe8\xae\xbe",  // Select preset (localized Chinese label)
          "Select Preset",
          "Switch to a named preset.",
          false,
          { {"type", "string", "Preset type: 'print', 'filament', or 'printer'", true, ""},
            {"name", "string", "Preset name to activate", true, ""} } },
        [this](const json& p) { return DoSelectPreset(p); });

    RegisterAction(
        { ActionID::LIST_PRINTERS,
          "List Printers",
          "List Printers",
          "List available printers and optionally filter by model, name, identity, or online and idle status.",
          false,
          {
              {"printer_model", "string", "Filter by printer model.", false, ""},
              {"printer_name", "string", "Filter by printer display name.", false, ""},
              {"device_id", "string", "Filter by device id, MAC, IP, or device key.", false, ""},
              {"ip", "string", "Filter by printer IP address.", false, ""},
              {"online_only", "bool", "Only return online printers.", false, "false"},
              {"idle_only", "bool", "Only return idle printers.", false, "false"},
              {"visible_only", "bool", "Only return visible printers.", false, "true"},
              {"limit", "number", "Maximum number of returned printers. Default is 20.", false, "20"}
          } },
        [this](const json& p) { return DoListPrinters(p); });

    RegisterAction(
        { ActionID::SELECT_PRINTER,
          "Select Printer",
          "Select Printer",
          "Switch the current physical printer and optionally synchronize the printer preset.",
          false,
          {
              {"mac", "string", "Target printer MAC address.", false, ""},
              {"device_id", "string", "Target printer device id.", false, ""},
              {"printer_name", "string", "Target printer display name.", false, ""},
              {"printer_model", "string", "Target printer model.", false, ""},
              {"preset_name", "string", "Printer preset name to select.", false, ""},
              {"sync_printer_preset", "bool", "Synchronize printer preset after selecting device.", false, "true"},
              {"require_online", "bool", "Fail if target printer is offline.", false, "false"},
              {"prefer_online", "bool", "Prefer online device when matching by name/model.", false, "true"},
              {"pick_first_match", "bool", "When multiple devices match by name/model, pick the first matched device instead of returning ambiguity.", false, "false"}
          } },
        [this](const json& p) { return DoSelectPrinter(p); });
    RegisterAction(
        { ActionID::MOVE_PRINT_HEAD,
          "?????",
          "Move Print Head",
          "Move the current printer to an absolute X/Y/Z pose for hotbed inspection without using cloud print from the host.",
          false,
          {
              {"x", "number", "Target X coordinate in mm.", false, ""},
              {"y", "number", "Target Y coordinate in mm.", false, ""},
              {"z", "number", "Target Z coordinate in mm.", false, ""},
              {"speed_mm_per_min", "number", "Optional move speed in mm/min.", false, ""},
              {"request_id", "string", "Optional request id for async result correlation.", false, ""}
          } },
        [this](const json& p) { return DoMovePrintHead(p); });

    RegisterAction(
        { ActionID::PRINT_CONTROL,
          "Print Control",
          "Print Control",
          "Pause, resume, or stop the current print job through the device manager host bridge.",
          false,
          {
              {"action", "string", "Print control action: pause, resume, or stop.", true, ""},
              {"request_id", "string", "Optional request id for async result correlation.", false, ""}
          } },
        [this](const json& p) { return DoPrintControl(p); });

    RegisterAction(
        { ActionID::APPLY_CONFIG,
          "\xe5\xba\x94\xe7\x94\xa8\xe9\x85\x8d\xe7\xbd\xae",  // Apply config (localized Chinese label)
          "Apply Config",
          "Set one or more print config key-value pairs (e.g. layer_height, sparse_infill_density). IMPORTANT: Do NOT use this tool to reset parameters to defaults. Use reset_config for reset operations.",
          false,
          { {"<key>", "string|number|bool", "Any valid print config key and its value", true, ""} } },
        [this](const json& p) { return DoApplyConfig(p); });

    RegisterAction(
        { ActionID::ADD_FILAMENT,
          "Add Filament",
          "Add Filament",
          "Add one filament slot using the next available color.",
          false, {} },
        [this](const json& p) { return DoAddFilament(p); });

    RegisterAction(
        { ActionID::DELETE_FILAMENT,
          "Delete Filament",
          "Delete Filament",
          "Delete a filament slot by id/index. Defaults to deleting the last slot.",
          false,
          { {"filament_id", "number", "Filament id (1-based) to delete", false, ""},
            {"filament_index", "number", "Filament index (0-based) to delete", false, ""} } },
        [this](const json& p) { return DoDeleteFilament(p); });

    RegisterAction(
        { ActionID::SET_FILAMENT_TYPE,
          "Set Filament Type",
          "Set Filament Type",
          "Set filament preset/type for a target filament slot.",
          false,
          { {"filament_id", "number", "Target filament id (1-based). Defaults to 1 when omitted and only one slot exists.", false, ""},
            {"filament_index", "number", "Target filament index (0-based).", false, ""},
            {"name", "string", "Filament preset name", false, ""},
            {"preset_name", "string", "Filament preset name (alias of name)", false, ""},
            {"filament_type", "string", "Filament alias/type (will be resolved to preset)", false, ""} } },
        [this](const json& p) { return DoSetFilamentType(p); });

    RegisterAction(
        { ActionID::AUTO_MAP_FILAMENTS,
          "Auto Map Filaments",
          "Auto Map Filaments",
          "Automatically match project filaments/materials to the current printer device materials and optionally apply the mapping back to the scene.",
          false,
          { {"apply", "bool", "Apply mapped filament colors/materials to the scene after matching.", false, "true"},
            {"require_complete", "bool", "Require every project filament item to be mapped before applying.", false, "true"},
            {"open_mapping_panel_on_conflict", "bool", "Open the filament mapping panel when mapping is incomplete or cannot be applied.", false, "true"},
            {"include_project_context", "bool", "Include refreshed slicer state in the result.", false, "true"},
            {"strategy", "string", "Mapping strategy. Currently supports type_then_color.", false, "type_then_color"},
            {"allow_type_mismatch", "bool", "Allow mapping different material types when colors are close.", false, "false"} } },
        [this](const json& p) { return DoAutoMapFilaments(p); });

    RegisterAction(
        { ActionID::SET_OBJECT_COLOR,
          "Set Object Color",
          "Set Object Color",
          "Set model color by assigning an extruder. Supports object_indices/object_index/object_name or current selection. You can pass extruder_id directly, or pass color (hex/name) to auto-match nearest filament color.",
          false,
          { {"object_name", "string", "Target object name from get_slicer_state.objects[].name", false, ""},
            {"object_index", "number", "Target object index from get_slicer_state.objects[]", false, ""},
            {"object_indices", "array[number]", "Target object index list for multi-object color update", false, ""},
            {"extruder_id", "number", "Target extruder id (1-based). If omitted, color will be mapped.", false, ""},
            {"color", "string", "Target color name or hex (e.g. green, #00FF00)", false, ""} } },
        [this](const json& p) { return DoSetObjectColor(p); });

    RegisterAction(
        { ActionID::IMPORT_MODEL,
          "\xe5\xaf\xbc\xe5\x85\xa5\xe6\xa8\xa1\xe5\x9e\x8b",  // Import model (localized Chinese label)
          "Import Model",
          "Import a model file by path without opening a file dialog.",
          false,
          { {"path", "string", "File path to import.", true, ""}, {"file_path", "string", "Alias of path.", false, ""} } },
        [this](const json& p) { return DoImportModel(p); });

    RegisterAction(
        { ActionID::OPEN_MODEL_LIBRARY,
          "Open Model Library",
          "Open Model Library",
          "Open the built-in online model library so the user can browse or search models.",
          false,
          { {"query", "string", "Optional model search keyword.", false, ""} } },
        [this](const json& p) { return DoOpenModelLibrary(p); });

    RegisterAction(
        { ActionID::RECOMMEND_MODEL,
          "Recommend Model",
          "Recommend Model",
          "Recommend one or more random trending online models that have importable 3mf files.",
          false,
          { {"count", "number", "Number of recommended models to return.", false, "1"},
            {"random_seed", "number", "Optional deterministic random seed for repeatable recommendations.", false, ""},
            {"candidate_pool_size", "number", "How many trending candidates to sample from before picking random models.", false, "30"} } },
        [this](const json& p) { return DoRecommendModel(p); });

    RegisterAction(
        { ActionID::SMART_MODEL_SEARCH,
          "Smart Model Search",
          "Smart Model Search",
          "Search the online model library and return the best matched model for user confirmation.",
          false,
          { {"keyword", "string", "Search keyword for the model.", true, ""},
            {"sort_by", "string", "Sort rule: likes/downloads/favorites/views/date/score.", false, "downloads"},
            {"sort_label", "string", "Optional display label for the selected sort type.", false, ""},
            {"limit", "number", "Number of results to return.", false, "1"} } },
        [this](const json& p) { return DoSmartModelSearch(p); });

    RegisterAction(
        { ActionID::IMPORT_MODEL_FROM_SEARCH,
          "Import Model From Search",
          "Import Model From Search",
          "Import the model previously selected by smart_model_search.",
          false,
          { {"model_id", "string", "Target model id returned by smart_model_search.", false, ""} } },
        [this](const json& p) { return DoImportModelFromSearch(p); });

    RegisterAction(
        { ActionID::AUTO_ORIENT,
          "Auto Orient",
          "Auto Orient",
          "Automatically orient all printable models on the build plate for best printing. "
          "Supports three optimization modes: 'min_area' (minimize support area, default), "
          "'min_volume' (minimize support volume), and 'min_time' (minimize print time). "
          "GPU-accelerated when available; falls back to CPU automatically.",
          true,
          { {"mode",       "string", "Optimization mode: 'min_area' | 'min_volume' | 'min_time'. Default 'min_area'.", false, "min_area"},
            {"min_area",   "bool",   "Shortcut flag: set true to use the minimize-support-area mode.",   false, ""},
            {"min_volume", "bool",   "Shortcut flag: set true to use the minimize-support-volume mode.", false, ""},
            {"min_time",   "bool",   "Shortcut flag: set true to use the minimize-print-time mode.",     false, ""} } },
        [this](const json& p) { return DoAutoOrient(p); });

    RegisterAction(
        { ActionID::REPAIR_MESH,
          "Repair Mesh",
          "Repair Mesh",
          "Repair selected model meshes with geometry errors. Supports object_name/object_index/object_indices and falls back to the current selection.",
          true,
          { {"object_name", "string", "Name of the object to repair (from get_slicer_state)", false, ""},
            {"object_index", "number", "Index of the object to repair (from get_slicer_state.objects[])", false, ""},
            {"object_indices", "array[number]", "Object index list for multi-object repair", false, ""} } },
        [this](const json& p) { return DoRepairMesh(p); });


    RegisterAction(
        { ActionID::SIMPLIFY_MODEL,
          "\xe7\xae\x80\xe5\x8c\x96\xe6\xa8\xa1\xe5\x9e\x8b",  // ��ģ��
          "Simplify Model",
          "Simplify (reduce the triangle count of) the currently selected or specified model using Quadric Edge Collapse. Accepts a ratio parameter (0.05-0.95, default 0.6) to control how many triangles to retain relative to the original.",
          true,
          { {"ratio", "number", "Fraction of original triangles to retain (0.05-0.95). Default is 0.6 (keep 60%%).", false, "0.6"},
            {"object_name", "string", "Name of the model object to simplify.", false, ""},
            {"object_index", "number", "Index of the model object to simplify.", false, ""} } },
        [this](const json& p) { return DoSimplifyModel(p); });

    RegisterAction(
        { ActionID::AUTO_ARRANGE,
          "\xe8\x87\xaa\xe5\x8a\xa8\xe6\x8e\x92\xe5\x88\x97",  // Auto arrange (localized Chinese label)
          "Auto Arrange",
          "Automatically arrange all models on the build plate.",
          true, {} },
        [this](const json& p) { return DoAutoArrange(p); });

    RegisterAction(
        { ActionID::UNDO,
          "Undo",
          "Undo",
          "Undo the last slicer operation when the undo stack has an available snapshot.",
          false, {} },
        [this](const json& p) { return DoUndo(p); });

    RegisterAction(
        { ActionID::REDO,
          "Redo",
          "Redo",
          "Redo the last undone slicer operation when the redo stack has an available snapshot.",
          false, {} },
        [this](const json& p) { return DoRedo(p); });

    RegisterAction(
        { ActionID::START_SLICE,
          "\xe5\xbc\x80\xe5\xa7\x8b\xe5\x88\x87\xe7\x89\x87",  // Start slice (localized Chinese label)
          "Start Slice",
          "Slice current plate, a target plate, or all plates with active settings. Supports preset switching and optional automatic G-code export.",
          true,
          {
              {"plate_number", "number", "Target plate number shown in UI (1-based, e.g. plate 3). Optional.", false, ""},
              {"plate", "number", "Alias of plate_number for natural language requests. Optional.", false, ""},
              {"all_plates", "bool", "Slice all plates in current project. If true, plate_number is ignored.", false, "false"},
              {"plate_index", "number", "Legacy compatibility field. Avoid in natural language requests; prefer plate_number.", false, ""},
              {"process_profile", "string", "Print/process preset name. Aliases: print_preset, print_profile, process_preset.", false, ""},
              {"material_profile", "string", "Filament/material preset name. Aliases: filament_preset, filament_profile, material_preset.", false, ""},
              {"printer_profile", "string", "Printer/machine preset name. Aliases: printer_preset, machine_preset, machine_profile.", false, ""},
              {"output_path", "string", "Optional G-code output path for automatic export after slicing. Aliases: export_path, gcode_path.", false, ""},
              {"export_strategy", "string", "slice_only or auto_export. If output_path is present, default becomes auto_export.", false, "slice_only"}
          } },
        [this](const json& p) { return DoStartSlice(p); });
    RegisterAction(
        { ActionID::SEND_TO_PRINTER,
          "Send To Current Printer",
          "Send To Current Printer",
          "Send the current plate to the currently selected printer. The printer must be bound, online, and idle before sending starts.",
          false,
          {
              {"safety_confirmed", "bool", "Set true when the user already confirmed the printer platform is physically safe for printing.", false, "false"},
              {"skip_local_confirmation", "bool", "Set true to skip the local native confirmation because confirmation already happened in chat.", false, "false"},
          } },
        [this](const json& p) { return DoSendToPrinter(p); });

    RegisterAction(
        { ActionID::EXPORT_GCODE,
          "\xe5\xaf\xbc\xe5\x87\xbaG\xe4\xbb\xa3\xe7\xa0\x81",  // Export G-code (localized Chinese label)
          "Export G-code",
          "Export sliced G-code to a file (opens save dialog).",
          true, {} },
        [this](const json& p) { return DoExportGcode(p); });

    RegisterAction(
        { ActionID::GET_CONFIG_OPTIONS,
          "\xe8\x8e\xb7\xe5\x8f\x96\xe5\x8f\x82\xe6\x95\xb0\xe5\x85\x83\xe6\x95\xb0\xe6\x8d\xae",  // Get config option metadata (localized Chinese label)
          "Get Config Options",
          "Return metadata for available print config options: key, label, type, category, unit, min, max, enum_values. Use category param to filter.",
          false,
          { {"category", "string", "Filter by category (e.g. 'Quality','Speed','Strength'). Empty = all", false, ""} } },
        [this](const json& p) { return DoGetConfigOptions(p); });

    RegisterAction(
        { ActionID::MOVE_OBJECT,
          "\xe7\xa7\xbb\xe5\x8a\xa8\xe5\xaf\xb9\xe8\xb1\xa1",
          "Move Object",
          "Move object(s) by relative offset, or move object(s) to target plate center.",
          false,
          { {"object_name", "string", "Name of the object to move (from get_slicer_state). Use wipe_tower/prime_tower with movable=wipe_tower to move the wipe tower.", false, ""},
            {"object_index", "number", "Index of the object to move (from get_slicer_state.objects[]). For movable=wipe_tower, this may be used as the reference object for clearance repair.", false, ""},
            {"object_indices", "array[number]", "Object indices for multi-object move", false, ""},
            {"dx", "number", "Relative displacement in X (mm)", false, "0"},
            {"dy", "number", "Relative displacement in Y (mm)", false, "0"},
            {"dz", "number", "Relative displacement in Z (mm)", false, "0"},
            {"auto_fix_bounds", "bool", "When true, calculate the minimum XY delta that moves the target back inside the plate build volume.", false, "false"},
            {"auto_fix_clearance", "bool", "When true, calculate a deterministic XY delta to satisfy min_clearance_mm from the nearest/reference object.", false, "false"},
            {"movable", "string", "Target kind for automatic movement. Supported values: object, wipe_tower, prime_tower.", false, "object"},
            {"reference_object_index", "number", "Reference object index used by wipe tower clearance repair.", false, ""},
            {"bounds_margin_mm", "number", "Safety margin to keep from the plate boundary when auto_fix_bounds is enabled.", false, "1"},
            {"min_clearance_mm", "number", "Required object clearance for auto_fix_clearance.", false, "8"},
            {"clearance_margin_mm", "number", "Extra clearance margin added to min_clearance_mm.", false, "2"},
            {"plate_number", "number", "Target plate number in UI (1-based). When set, objects are moved to target plate center.", false, ""},
            {"plate_index", "number", "Target plate index (0-based).", false, ""},
            {"plate", "number", "Alias of plate_number.", false, ""} } },
        [this](const json& p) { return DoMoveObject(p); });

    RegisterAction(
        { ActionID::ROTATE_OBJECT,
          "Rotate Object",
          "Rotate Object",
          "Rotate a model object by relative Euler angles in degrees (rx_deg, ry_deg, rz_deg). Target can be resolved by object_index, object_name, or current single selection.",
          false,
          { {"object_name", "string", "Name of the object to rotate (from get_slicer_state)", false, ""},
            {"object_index", "number", "Index of the object to rotate (from get_slicer_state.objects[])", false, ""},
            {"rx_deg", "number", "Relative rotation around X axis (degrees)", false, "0"},
            {"ry_deg", "number", "Relative rotation around Y axis (degrees)", false, "0"},
            {"rz_deg", "number", "Relative rotation around Z axis (degrees)", false, "0"} } },
        [this](const json& p) { return DoRotateObject(p); });

    RegisterAction(
        { ActionID::SCALE_OBJECT,
          "Scale Object",
          "Scale Object",
          "Scale model objects by relative factors (sx, sy, sz). Target can be resolved by object_index, object_name, current selection, or scope (all/plate/object). When no explicit target is specified and no object is selected, defaults to scope=object. Use scope=plate to scale all objects on a plate, or scope=all to scale all objects.",
          false,
          { {"object_name", "string", "Name of the object to scale (from get_slicer_state)", false, ""},
            {"object_index", "number", "Index of the object to scale (from get_slicer_state.objects[])", false, ""},
            {"object_indices", "array[number]", "List of object indices to scale", false, ""},
            {"scope", "string", "Target scope: object | plate | all. Defaults to object. Use plate to scale all models on a plate, all for all models.", false, "object"},
            {"plate_number", "number", "Target plate number shown in UI (1-based, used when scope=plate)", false, ""},
            {"plate", "number", "Alias of plate_number (scope=plate)", false, ""},
            {"plate_index", "number", "Legacy compatibility field. Prefer plate_number (scope=plate)", false, ""},
            {"sx", "number", "Relative scale factor on X axis", false, "1"},
            {"sy", "number", "Relative scale factor on Y axis", false, "1"},
            {"sz", "number", "Relative scale factor on Z axis", false, "1"} } },
        [this](const json& p) { return DoScaleObject(p); });

    RegisterAction(
        { ActionID::SELECT_OBJECTS,
          "Select Objects",
          "Select Objects",
          "Select objects by scope (all/plate/color/object). Supports object_index/object_indices/object_name, plate_number(1-based), plate_index(legacy), color/extruder_id, and append mode.",
          false,
          { {"scope", "string", "Selection scope: all | plate | color | object", false, "object"},
            {"append", "bool", "If true, add to current selection instead of replacing it", false, "false"},
            {"object_name", "string", "Target object name (scope=object)", false, ""},
            {"object_index", "number", "Target object index (scope=object)", false, ""},
            {"object_indices", "array[number]", "Target object index list (scope=object)", false, ""},
            {"plate_number", "number", "Target plate number shown in UI (1-based, scope=plate)", false, ""},
            {"plate", "number", "Alias of plate_number (scope=plate)", false, ""},
            {"plate_index", "number", "Legacy compatibility field. Prefer plate_number (scope=plate)", false, ""},
            {"color", "string", "Target color name/hex (scope=color)", false, ""},
            {"extruder_id", "number", "Target extruder id (scope=color)", false, ""} } },
        [this](const json& p) { return DoSelectObjects(p); });

    RegisterAction(
        { ActionID::DELETE_MODEL,
          "Delete Model",
          "Delete Model",
          "Delete a model object from the scene by object_name or object_index.",
          false,
          { {"object_name", "string", "Name of the object to delete", false, ""},
            {"object_index", "number", "Index of the object to delete", false, ""} } },
        [this](const json& p) { return DoDeleteModel(p); });

    RegisterAction(
        { ActionID::CLONE_MODEL,
          "Clone Model",
          "Clone Model",
          "Clone (duplicate) a model object by object_name or object_index.",
          false,
          { {"object_name", "string", "Name of the object to clone", false, ""},
            {"object_index", "number", "Index of the object to clone", false, ""} } },
        [this](const json& p) { return DoCloneModel(p); });

        RegisterAction(
        { ActionID::ARRANGE_SINGLE_PLATE,
          "Arrange Current Plate",
          "Arrange Single Plate",
          "Arrange models on the current plate only.",
          true,
          {} },
        [this](const json& p) { return DoArrangeSinglePlate(p); });

        RegisterAction(
        { ActionID::ARRANGE_ALL_PLATES,
          "Arrange All Plates",
          "Arrange All Plates",
          "Arrange models globally across all plates.",
          true,
          {} },
        [this](const json& p) { return DoArrangeAllPlates(p); });

        RegisterAction(
        { ActionID::FILL_BED,
          "Fill Bed",
          "Fill Bed",
          "Duplicate the target model to fill the bed. Requires one target object.",
          true,
          { {"object_name", "string", "Target object name", false, ""},
            {"object_index", "number", "Target object index", false, ""} } },
        [this](const json& p) { return DoFillBed(p); });
    RegisterAction(
        { ActionID::GET_SCENE_WARNINGS,
          "\xe8\x8e\xb7\xe5\x8f\x96\xe5\x9c\xba\xe6\x99\xaf\xe8\xad\xa6\xe5\x91\x8a",
          "Get Scene Warnings",
          "Return current scene warnings and errors from the notification system (validation warnings, collision alerts, etc.). Lightweight alternative to get_slicer_state for checking scene health.",
          false,
          {} },
        [this](const json& p) { return DoGetSceneWarnings(p); });
    RegisterAction(
        { ActionID::EDIT_PLATE_NAME,
          "\xe4\xbf\xae\xe6\x94\xb9\xe7\x9b\x98\xe5\x90\x8d\xe7\xa7\xb0",  // Edit plate name (localized Chinese label)
          "Edit Plate Name",
          "Rename the current plate or a target plate with the given name.",
          false,
          { {"name", "string", "New plate name", true, ""},
            {"plate_number", "number", "Target plate number in UI (1-based). Defaults to current plate.", false, ""},
            {"plate_index", "number", "Target plate index (0-based).", false, ""} } },
        [this](const json& p) { return DoRenamePlate(p); });
    RegisterAction(
        { ActionID::ADD_PLATE,
          "\xe6\xb7\xbb\xe5\x8a\xa0\xe6\x96\xb0\xe7\x9b\x98",  // ��������
          "Add Plate",
          "Add a new build plate to the project. The new plate becomes the current selected plate.",
          false,
          {} },
        [this](const json& p) { return DoAddPlate(p); });
    RegisterAction(
        { ActionID::DELETE_PLATE,
          "\xe5\x88\xa0\xe9\x99\xa4\xe7\x9b\x98",  // ɾ����
          "Delete Plate",
          "Delete a build plate from the project. At least one plate must remain.",
          true,
          { {"plate_number", "number", "Target plate number in UI (1-based). Defaults to current plate.", false, ""},
            {"plate_index", "number", "Target plate index (0-based).", false, ""} } },
        [this](const json& p) { return DoDeletePlate(p); });

    RegisterAction(
        { ActionID::TOGGLE_PREVIEW_LITE_MODE,
          "\xe5\x88\x87\xe6\x8d\xa2\xe7\xb2\xbe\xe7\xae\x80\xe6\xa8\xa1\xe5\xbc\x8f",  // �л�����ģʽ
          "Toggle GCode Preview Lite Mode",
          "Enable or disable the G-code preview lite/simplified mode. When enabled, hides less common line types (retract, wipe, travel, etc.) in the preview panel for a cleaner view.",
          false,
          { {"enabled", "boolean", "true to enable lite mode, false to disable. If omitted, toggles the current state.", false, ""} } },
        [this](const json& p) { return DoTogglePreviewLiteMode(p); });

    RegisterAction(
        { ActionID::SPLIT_MODEL,
          "\xe5\x88\x87\xe5\x89\xb2\xe6\xa8\xa1\xe5\x9e\x8b",  // �и�ģ��
          "Split Model",
          "Split/cut the selected model into parts at the specified height. If no Z is given, defaults to the middle Z of the object. Supports object_name or object_index to identify the target.",
          true,
          { {"object_name", "string", "Name of the object to split (from get_slicer_state)", false, ""},
            {"object_index", "number", "Index of the object to split (from get_slicer_state.objects[])", false, ""},
            {"z", "number", "Z-height at which to cut the model (in mm). Defaults to the middle Z of the object.", false, ""} } },
        [this](const json& p) { return DoSplitModel(p); });

    RegisterAction(
        { ActionID::ADD_TEST_MODEL,
          "\xe6\xb7\xbb\xe5\x8a\xa0\xe6\xb5\x8b\xe8\xaf\x95\xe6\xa8\xa1\xe5\x9e\x8b",  // ���Ӳ���ģ��
          "Add Test Model",
          "Add a test model to the build plate. Supports procedural shapes (Cube, Sphere, Cylinder, Cone, Truncated Cone, Torus, Pyramid, Prism, Disc) and file-based test models (Block20XY, 3DBenchy, Complex, Overhang, Square columns Z axis, Square prism Z axis).",
          false,
          { {"type_name", "string", "Test model type name, e.g. Cube, Sphere, Cylinder, Cone, Truncated Cone, Torus, Pyramid, Prism, Disc, Block20XY, 3DBenchy, Complex, Overhang, Square columns Z axis, Square prism Z axis", true, ""},
            {"name", "string", "Alias of type_name", false, ""} } },
        [this](const json& p) { return DoAddTestModel(p); });

    BOOST_LOG_TRIVIAL(info) << "[SlicerBridge] Registered " << m_actions.size() << " actions";
}

} // namespace Bridge
} // namespace GUI
} // namespace Slic3r
