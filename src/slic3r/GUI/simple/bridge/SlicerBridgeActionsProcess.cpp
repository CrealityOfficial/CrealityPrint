#include "SlicerBridge.hpp"

#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/PartPlate.hpp"
#include "slic3r/GUI/Selection.hpp"
#include "slic3r/GUI/GLCanvas3D.hpp"
#include "slic3r/GUI/Tab.hpp"
#include "slic3r/GUI/simple/gpu/GpuOrient.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/ModelObject.hpp"
#include "libslic3r/ModelInstance.hpp"
#include "libslic3r/Orient.hpp"
#include "libslic3r/PresetBundle.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstring>
#include <string>
#include <initializer_list>

#include <boost/log/trivial.hpp>

using json = nlohmann::json;

namespace Slic3r {
namespace GUI {
namespace Bridge {

namespace {

// Lowercase ASCII helper used for parsing mode strings.
std::string ao_to_lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

// Trim ASCII whitespace.
std::string ao_trim(std::string s)
{
    auto is_ws = [](unsigned char ch) { return std::isspace(ch) != 0; };
    while (!s.empty() && is_ws((unsigned char)s.front())) s.erase(s.begin());
    while (!s.empty() && is_ws((unsigned char)s.back())) s.pop_back();
    return s;
}

// Tolerant bool parser (true/false/yes/no/1/0/on/off, numbers).
bool ao_parse_bool(const json& value, bool& out)
{
    try {
        if (value.is_boolean()) { out = value.get<bool>(); return true; }
        if (value.is_number_integer()) { out = value.get<int>() != 0; return true; }
        if (value.is_number()) { out = std::fabs(value.get<double>()) > 1e-12; return true; }
        if (value.is_string()) {
            std::string v = ao_to_lower(ao_trim(value.get<std::string>()));
            if (v == "1" || v == "true"  || v == "yes" || v == "on")  { out = true;  return true; }
            if (v == "0" || v == "false" || v == "no"  || v == "off") { out = false; return true; }
        }
    } catch (...) {}
    return false;
}

bool ao_get_bool(const json& params, std::initializer_list<const char*> keys, bool& out)
{
    for (const char* k : keys) {
        if (params.contains(k) && ao_parse_bool(params[k], out))
            return true;
    }
    return false;
}

std::string ao_get_string(const json& params, std::initializer_list<const char*> keys)
{
    for (const char* k : keys) {
        if (!params.contains(k)) continue;
        try {
            if (params[k].is_string())
                return ao_to_lower(ao_trim(params[k].get<std::string>()));
        } catch (...) {}
    }
    return {};
}

// Resolve orientation mode from JSON args.
// Priority: explicit string `mode`/`orient_type` -> boolean flags -> default MinArea.
orientation::EOrientType ao_resolve_mode(const json& params)
{
    const std::string mode_str = ao_get_string(params, {"mode", "orient_type", "type"});
    if (!mode_str.empty()) {
        if (mode_str == "min_volume" || mode_str == "minvolume" || mode_str == "volume" || mode_str == "min-volume")
            return orientation::MinVolume;
        if (mode_str == "min_time" || mode_str == "mintime" || mode_str == "time" || mode_str == "min-time")
            return orientation::MinTime;
        if (mode_str == "min_area" || mode_str == "minarea" || mode_str == "area" || mode_str == "min-area")
            return orientation::MinArea;
    }

    bool b = false;
    // The bool flags are mutually-exclusive in OrientSettings semantics;
    // honor them in priority order so the AI side can flip one at a time.
    if (ao_get_bool(params, {"min_volume", "minvolume"}, b) && b)
        return orientation::MinVolume;
    if (ao_get_bool(params, {"min_time", "mintime"}, b) && b)
        return orientation::MinTime;
    if (ao_get_bool(params, {"min_area", "minarea"}, b) && b)
        return orientation::MinArea;

    return orientation::MinArea;
}

const char* ao_mode_label(orientation::EOrientType t)
{
    switch (t) {
        case orientation::MinVolume: return "min_volume";
        case orientation::MinTime:   return "min_time";
        case orientation::MinArea:
        default:                     return "min_area";
    }
}

// Build an OrientMesh from a ModelInstance, mirroring OrientJob::get_orient_mesh.
orientation::OrientMesh ao_make_orient_mesh(ModelInstance* instance)
{
    orientation::OrientMesh om;
    auto* obj = instance->get_object();
    om.name = obj->name;
    om.mesh = obj->mesh();
    if (obj->config.has("support_threshold_angle")) {
        om.overhang_angle = obj->config.opt_int("support_threshold_angle");
    } else {
        const Slic3r::DynamicPrintConfig& full_cfg = wxGetApp().preset_bundle->full_config();
        om.overhang_angle = full_cfg.opt_int("support_threshold_angle");
    }
    om.setter = [instance](const orientation::OrientMesh& p) {
        instance->rotate(p.rotation_matrix);
        instance->get_object()->invalidate_bounding_box();
        instance->get_object()->ensure_on_bed();
    };
    return om;
}

} // anonymous namespace

json SlicerBridge::DoAutoOrient(const json& params)
{
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};

    // 1) Resolve target mode and reflect it onto the canvas OrientSettings so the
    //    UI / async fallback path stay consistent with what the AI asked for.
    const orientation::EOrientType orient_type = ao_resolve_mode(params);
    if (auto* canvas = plater->canvas3D()) {
        GLCanvas3D::OrientSettings& settings = canvas->get_orient_settings();
        settings.min_area   = (orient_type == orientation::MinArea);
        settings.min_volume = (orient_type == orientation::MinVolume);
        settings.min_time   = (orient_type == orientation::MinTime);
    }

    // 2) Build OrientParams that match OrientJob::process() exactly.
    orientation::OrientParams op_params;
    if (orient_type == orientation::MinArea) {
        orientation::OrientParamsArea params_area;
        std::memcpy(&op_params, &params_area, sizeof(op_params));
    }
    op_params.orient_type = orient_type;

    // 3) Try GPU-accelerated path (T7a: AI/easy_mode first). Falls back to the
    //    legacy async plater->orient() when the GPU context is unavailable.
    static orientation::GpuOrient s_gpu_orienter;
    if (s_gpu_orienter.available()) {
        Model& model = plater->model();
        orientation::OrientMeshs items;
        orientation::OrientMeshs excludes; // AI mode: orient everything printable, nothing excluded.
        size_t printable_count = 0;
        for (auto* obj : model.objects)
            for (auto* mi : obj->instances)
                if (mi && mi->printable) ++printable_count;
        items.reserve(printable_count);
        for (auto* obj : model.objects) {
            if (!obj) continue;
            for (auto* mi : obj->instances) {
                if (!mi || !mi->printable) continue;
                items.emplace_back(ao_make_orient_mesh(mi));
            }
        }

        if (items.empty()) {
            return {{"success", true},
                    {"message", std::string("Auto orient skipped: no printable instances (") + ao_mode_label(orient_type) + ")"}};
        }

        plater->take_snapshot(_u8L("Orient"));

        const auto t_start = std::chrono::steady_clock::now();
        std::string error;
        const bool ok = s_gpu_orienter.orient(items, excludes, op_params,
                                              /*fallback_to_cpu=*/true, &error);
        const auto t_end = std::chrono::steady_clock::now();
        const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();

        if (ok) {
            for (auto& mesh : items)
                mesh.apply();
            plater->update();
            const char* path_label = error.empty() ? "GPU" : "CPU fallback";
            BOOST_LOG_TRIVIAL(info) << "DoAutoOrient: " << path_label
                                    << " mode=" << ao_mode_label(orient_type)
                                    << " items=" << items.size()
                                    << " elapsed_ms=" << elapsed_ms
                                    << (error.empty() ? "" : (" warn=" + error));
            return {{"success", true},
                    {"message", std::string("Auto orient completed (") + path_label + ", " + ao_mode_label(orient_type) + ")"}};
        }

        BOOST_LOG_TRIVIAL(warning) << "DoAutoOrient: GPU orient failed and CPU fallback returned false: " << error;
        // Fall through to async CPU path below.
    }

    // 4) CPU async fallback through the existing OrientJob worker. OrientSettings
    //    was already updated above so the worker will pick up the requested mode.
    plater->orient();
    return {{"success", true},
            {"message", std::string("Auto orient queued (CPU, ") + ao_mode_label(orient_type) + ")"}};
}

json SlicerBridge::DoAutoArrange(const json& /*params*/)
{
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};

    plater->arrange();
    return {{"success", true}, {"message", "Auto arrange completed"}};
}

json SlicerBridge::DoUndo(const json& /*params*/)
{
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};
    if (!plater->can_undo())
        return {{"success", false}, {"message", "No undo snapshot available"}};

    plater->undo();
    return {{"success", true}, {"message", "Undo completed"}};
}

json SlicerBridge::DoRedo(const json& /*params*/)
{
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};
    if (!plater->can_redo())
        return {{"success", false}, {"message", "No redo snapshot available"}};

    plater->redo();
    return {{"success", true}, {"message", "Redo completed"}};
}

json SlicerBridge::DoStartSlice(const json& params)
{
    auto* mainframe = wxGetApp().mainframe;
    auto* plater = wxGetApp().plater();
    auto* bundle = wxGetApp().preset_bundle;
    if (!mainframe)
        return {{"success", false}, {"message", "MainFrame not available"}};
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};

    PartPlateList& plate_list = plater->get_partplate_list();
    const int plate_count = plate_list.get_plate_count();
    if (plate_count <= 0)
        return {{"success", false}, {"message", "No plate available"}};

    auto trim = [](std::string s) {
        auto is_ws = [](unsigned char ch) { return std::isspace(ch) != 0; };
        while (!s.empty() && is_ws((unsigned char)s.front())) s.erase(s.begin());
        while (!s.empty() && is_ws((unsigned char)s.back())) s.pop_back();
        return s;
    };

    auto to_lower_ascii = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
        return s;
    };

    auto parse_int_relaxed = [&trim](const json& value, int& out) -> bool {
        try {
            if (value.is_number_integer()) {
                out = value.get<int>();
                return true;
            }
            if (value.is_number()) {
                out = (int)std::llround(value.get<double>());
                return true;
            }
            if (value.is_string()) {
                const std::string s = trim(value.get<std::string>());
                if (s.empty())
                    return false;

                try {
                    size_t pos = 0;
                    int v = std::stoi(s, &pos);
                    if (pos == s.size()) {
                        out = v;
                        return true;
                    }
                } catch (...) {
                }

                std::string token;
                bool started = false;
                for (unsigned char ch : s) {
                    if (!started) {
                        if (std::isdigit(ch) || ch == '+' || ch == '-') {
                            token.push_back((char)ch);
                            started = true;
                        }
                    } else {
                        if (std::isdigit(ch))
                            token.push_back((char)ch);
                        else
                            break;
                    }
                }

                if (!token.empty() && token != "+" && token != "-") {
                    out = std::stoi(token);
                    return true;
                }
            }
        } catch (...) {
        }
        return false;
    };

    auto parse_bool_like = [&trim, &to_lower_ascii](const json& value, bool& out) -> bool {
        try {
            if (value.is_boolean()) {
                out = value.get<bool>();
                return true;
            }
            if (value.is_number_integer()) {
                out = value.get<int>() != 0;
                return true;
            }
            if (value.is_number()) {
                out = std::fabs(value.get<double>()) > 1e-12;
                return true;
            }
            if (value.is_string()) {
                std::string v = to_lower_ascii(trim(value.get<std::string>()));
                if (v == "1" || v == "true" || v == "yes" || v == "on") { out = true; return true; }
                if (v == "0" || v == "false" || v == "no" || v == "off") { out = false; return true; }
            }
        } catch (...) {
        }
        return false;
    };

    auto parse_key = [&](const char* key, int& out) -> bool {
        return params.contains(key) && parse_int_relaxed(params[key], out);
    };

    auto parse_string_key = [&trim, &params](const char* key) -> std::string {
        if (!params.contains(key))
            return {};
        try {
            if (params[key].is_string())
                return trim(params[key].get<std::string>());
        } catch (...) {
        }
        return {};
    };

    auto first_non_empty = [](std::initializer_list<std::string> values) -> std::string {
        for (const auto& value : values) {
            if (!value.empty())
                return value;
        }
        return {};
    };

    auto current_print_preset_name = [&]() -> std::string {
        return bundle ? bundle->prints.get_edited_preset().name : std::string();
    };
    auto current_filament_preset_name = [&]() -> std::string {
        return bundle ? bundle->filaments.get_edited_preset().name : std::string();
    };
    auto current_printer_preset_name = [&]() -> std::string {
        return bundle ? bundle->printers.get_edited_preset().name : std::string();
    };

    auto collection_has_preset = [](const PresetCollection& collection, const std::string& preset_name) -> bool {
        if (preset_name.empty())
            return false;
        for (const Preset& preset : collection.get_presets()) {
            if (preset.name == preset_name)
                return true;
        }
        return false;
    };

    auto select_preset_if_requested = [&](Preset::Type preset_type,
                                          const std::string& preset_name,
                                          PresetCollection& collection,
                                          const char* error_label) -> json {
        if (preset_name.empty())
            return json();
        if (!bundle) {
            return {
                {"success", false},
                {"message", std::string("Preset bundle not available while applying ") + error_label}
            };
        }
        if (!collection_has_preset(collection, preset_name)) {
            return {
                {"success", false},
                {"message", std::string("Invalid ") + error_label + ": " + preset_name}
            };
        }

        auto* tab = wxGetApp().get_tab(preset_type);
        if (!tab) {
            return {
                {"success", false},
                {"message", std::string("Preset tab not available for ") + error_label}
            };
        }

        tab->select_preset(preset_name);
        return json();
    };

    const std::string requested_print_preset = first_non_empty({
        parse_string_key("process_profile"),
        parse_string_key("print_preset"),
        parse_string_key("print_profile"),
        parse_string_key("process_preset")
    });
    const std::string requested_filament_preset = first_non_empty({
        parse_string_key("material_profile"),
        parse_string_key("filament_preset"),
        parse_string_key("filament_profile"),
        parse_string_key("material_preset")
    });
    const std::string requested_printer_preset = first_non_empty({
        parse_string_key("printer_profile"),
        parse_string_key("printer_preset"),
        parse_string_key("machine_preset"),
        parse_string_key("machine_profile")
    });

    if (bundle) {
        if (json error = select_preset_if_requested(Preset::TYPE_PRINT, requested_print_preset, bundle->prints, "print/process preset"); !error.is_null())
            return error;
        if (json error = select_preset_if_requested(Preset::TYPE_FILAMENT, requested_filament_preset, bundle->filaments, "filament/material preset"); !error.is_null())
            return error;
        if (json error = select_preset_if_requested(Preset::TYPE_PRINTER, requested_printer_preset, bundle->printers, "printer/machine preset"); !error.is_null())
            return error;
    }


    const std::string output_path = first_non_empty({
        parse_string_key("output_path"),
        parse_string_key("export_path"),
        parse_string_key("gcode_path")
    });
    std::string export_strategy = first_non_empty({
        parse_string_key("export_strategy"),
        parse_string_key("export_mode")
    });
    export_strategy = to_lower_ascii(export_strategy);
    if (export_strategy.empty())
        export_strategy = output_path.empty() ? "slice_only" : "auto_export";

    auto is_all_keyword = [&trim, &to_lower_ascii](const json& value) -> bool {
        if (!value.is_string())
            return false;
        std::string v = to_lower_ascii(trim(value.get<std::string>()));
        v.erase(std::remove_if(v.begin(), v.end(), [](unsigned char ch) {
            return std::isspace(ch) != 0 || ch == '_' || ch == '-';
        }), v.end());
        return v == "all" || v == "allplates" || v == "allplate";
    };

    bool slice_all_requested = false;
    auto parse_slice_all_bool_key = [&](const char* key) {
        if (!params.contains(key))
            return;
        bool b = false;
        if (parse_bool_like(params[key], b) && b)
            slice_all_requested = true;
        else if (is_all_keyword(params[key]))
            slice_all_requested = true;
    };

    auto parse_slice_all_keyword_key = [&](const char* key) {
        if (!params.contains(key))
            return;
        if (is_all_keyword(params[key]))
            slice_all_requested = true;
    };

    parse_slice_all_bool_key("all_plates");
    parse_slice_all_bool_key("allPlates");
    parse_slice_all_bool_key("slice_all");
    parse_slice_all_bool_key("sliceAll");

    parse_slice_all_keyword_key("scope");
    parse_slice_all_keyword_key("mode");
    parse_slice_all_keyword_key("plate");
    parse_slice_all_keyword_key("plate_number");
    parse_slice_all_keyword_key("plateNumber");

    if (slice_all_requested) {
        mainframe->slice_plate(MainFrame::eSliceAll);
        return {
            {"success", true},
            {"message", "Slice all started"},
            {"mode", "all_plates"},
            {"plate_count", plate_count},
            {"export_strategy", export_strategy},
            {"output_path", output_path},
            {"current_print_preset", current_print_preset_name()},
            {"current_filament_preset", current_filament_preset_name()},
            {"current_printer_preset", current_printer_preset_name()}
        };
    }

    auto find_plate_by_public_index = [&](int requested) -> int {
        for (int pi = 0; pi < plate_count; ++pi) {
            PartPlate* plate = plate_list.get_plate(pi);
            if (plate && plate->get_index() == requested)
                return pi;
        }
        return -1;
    };

    auto resolve_plate_index_like = [&](int requested) -> int {
        if (requested > 0 && requested <= plate_count)
            return requested - 1;
        if (requested == 0 && plate_count > 0)
            return 0;
        if (requested >= 0 && requested < plate_count)
            return requested;
        return find_plate_by_public_index(requested);
    };

    auto resolve_plate_number = [&](int plate_number) -> int {
        if (plate_number > 0 && plate_number <= plate_count)
            return plate_number - 1;
        return -1;
    };

    const int current_plate_idx = plate_list.get_curr_plate_index();
    int target_plate_idx = current_plate_idx;
    bool has_target = false;
    int requested = -1;

    if (parse_key("plate_number", requested) || parse_key("plateNumber", requested) ||
        parse_key("plate_no", requested) || parse_key("plateNo", requested) ||
        parse_key("target_plate_number", requested) || parse_key("targetPlateNumber", requested)) {
        target_plate_idx = resolve_plate_number(requested);
        has_target = true;
    } else if (parse_key("plate", requested) || parse_key("target_plate", requested) || parse_key("targetPlate", requested)) {
        target_plate_idx = resolve_plate_number(requested);
        if (target_plate_idx < 0)
            target_plate_idx = resolve_plate_index_like(requested);
        has_target = true;
    } else if (parse_key("plate_index", requested) || parse_key("plateIndex", requested)) {
        // plate_index is a genuine 0-based index (matches current_plate_index),
        // so use it directly. Do NOT route through resolve_plate_index_like,
        // which treats a positive value as a 1-based plate number and subtracts
        // one (mapping plate_index=1 to plate 0, i.e. slicing plate 1 instead of
        // plate 2).
        if (requested >= 0 && requested < plate_count)
            target_plate_idx = requested;
        else
            target_plate_idx = find_plate_by_public_index(requested);
        has_target = true;
    }

    if (!has_target) {
        auto* canvas = plater->canvas3D();
        if (canvas) {
            const Selection& selection = canvas->get_selection();
            const auto& selected_content = selection.get_content();
            const Model& model = plater->model();

            int inferred_plate_idx = -1;
            bool ambiguous = false;

            for (const auto& kv : selected_content) {
                const int obj_idx = (int)kv.first;
                if (obj_idx < 0 || obj_idx >= (int)model.objects.size() || !model.objects[obj_idx])
                    continue;

                int obj_plate_idx = -1;
                for (int pi = 0; pi < plate_count; ++pi) {
                    PartPlate* plate = plate_list.get_plate(pi);
                    if (!plate)
                        continue;
                    const ModelObject* obj = model.objects[obj_idx];
                    for (int ii = 0; ii < (int)obj->instances.size(); ++ii) {
                        if (plate->contain_instance(obj_idx, ii)) {
                            obj_plate_idx = pi;
                            break;
                        }
                    }
                    if (obj_plate_idx >= 0)
                        break;
                }

                if (obj_plate_idx < 0)
                    continue;

                if (inferred_plate_idx < 0)
                    inferred_plate_idx = obj_plate_idx;
                else if (inferred_plate_idx != obj_plate_idx) {
                    ambiguous = true;
                    break;
                }
            }

            if (inferred_plate_idx >= 0 && !ambiguous) {
                target_plate_idx = inferred_plate_idx;
                has_target = true;
            }
        }
    }

    if (has_target) {
        if (target_plate_idx < 0 || target_plate_idx >= plate_count) {
            return {
                {"success", false},
                {"message", "Invalid target plate. Prefer plate_number (1-based UI number)."},
                {"requested_plate", requested},
                {"plate_count", plate_count},
                {"current_print_preset", current_print_preset_name()},
                {"current_filament_preset", current_filament_preset_name()},
                {"current_printer_preset", current_printer_preset_name()}
            };
        }

        if (target_plate_idx != current_plate_idx) {
            const int ret = plater->select_plate(target_plate_idx, false);
            if (ret != 0) {
                return {
                    {"success", false},
                    {"message", "Failed to switch to target plate before slicing"},
                    {"target_plate_index", target_plate_idx},
                    {"current_plate_index", current_plate_idx}
                };
            }
        }
    }

    mainframe->slice_plate(MainFrame::eSlicePlate);
    return {
        {"success", true},
        {"message", "Slicing started"},
        {"target_plate_index", target_plate_idx},
        {"target_plate_number", target_plate_idx + 1},
        {"current_plate_index", plate_list.get_curr_plate_index()},
        {"resolved_by", has_target ? "target_or_selection" : "current_plate"},
        {"mode", "single_plate"},
        {"export_strategy", export_strategy},
        {"output_path", output_path},
        {"current_print_preset", current_print_preset_name()},
        {"current_filament_preset", current_filament_preset_name()},
        {"current_printer_preset", current_printer_preset_name()}
    };
}

json SlicerBridge::DoSendToPrinter(const json& params)
{
    if (!m_send_to_printer_delegate) {
        return {
            {"success", false},
            {"code", "AI_SEND_WORKFLOW_UNAVAILABLE"},
            {"message", "AI send workflow service is not available for send_to_printer."}
        };
    }

    json delegate_params = params.is_object() ? params : json::object();
    if (!delegate_params.contains("entry_mode"))
        delegate_params["entry_mode"] = "send_workflow";
    if (!delegate_params.contains("direct_start_print"))
        delegate_params["direct_start_print"] = true;

    json result = m_send_to_printer_delegate(delegate_params);
    if (!result.contains("source_action"))
        result["source_action"] = ActionID::SEND_TO_PRINTER;
    return result;
}

json SlicerBridge::DoExportGcode(const json& /*params*/)
{
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};

    plater->export_gcode(false);
    return {{"success", true}, {"message", "G-code export started"}};
}

json SlicerBridge::DoArrangeSinglePlate(const json& /*params*/)
{
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};
    if (!plater->can_arrange())
        return {{"success", false}, {"message", "Arrange is currently unavailable"}};

    plater->select_curr_plate_all();
    plater->arrange();
    return {{"success", true}, {"message", "Arrange job started for current plate"}};
}

json SlicerBridge::DoArrangeAllPlates(const json& /*params*/)
{
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};
    if (!plater->can_arrange())
        return {{"success", false}, {"message", "Arrange is currently unavailable"}};

    plater->select_all();
    plater->arrange();
    return {{"success", true}, {"message", "Global arrange job started"}};
}

} // namespace Bridge
} // namespace GUI
} // namespace Slic3r
