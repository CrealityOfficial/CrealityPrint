#include "SlicerBridge.hpp"

#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/PartPlate.hpp"
#include "slic3r/GUI/Tab.hpp"
#include "slic3r/GUI/NotificationManager.hpp"
#include "slic3r/GUI/ParamsPanel.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/ModelObject.hpp"
#include "libslic3r/ModelVolume.hpp"
#include "libslic3r/ModelInstance.hpp"
#include "slic3r/GUI/Selection.hpp"
#include "slic3r/GUI/GUI_ObjectList.hpp"
#include "slic3r/GUI/GLCanvas3D.hpp"
#include "slic3r/GUI/format.hpp"
#include "slic3r/GUI/print_manage/data/DataCenter.hpp"

#include <boost/log/trivial.hpp>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

using json = nlohmann::json;

namespace Slic3r {
namespace GUI {
namespace Bridge {

json SlicerBridge::DoGetPresets(const json& params)
{
    auto* bundle = wxGetApp().preset_bundle;
    if (!bundle)
        return {{"success", false}, {"message", "PresetBundle not available"}};

    std::string type = params.value("type", "all");
    json result;
    result["success"] = true;
    result["message"] = "OK";

    auto serialize_collection = [](const PresetCollection& col) -> json {
        json arr = json::array();
        for (auto it = col.begin(); it != col.end(); ++it) {
            arr.push_back({
                {"name", it->name},
                {"is_visible", it->is_visible},
                {"is_default", it->is_default},
                {"is_system", it->is_system}
            });
        }
        return arr;
    };

    if (type == "all" || type == "print") {
        result["print_presets"]  = serialize_collection(bundle->prints);
        result["current_print"]  = bundle->prints.get_edited_preset().name;
    }
    if (type == "all" || type == "filament") {
        result["filament_presets"]  = serialize_collection(bundle->filaments);
        result["current_filament"]  = bundle->filaments.get_edited_preset().name;
    }
    if (type == "all" || type == "printer") {
        result["printer_presets"]  = serialize_collection(bundle->printers);
        result["current_printer"]  = bundle->printers.get_edited_preset().name;
    }

    return result;
}

json SlicerBridge::DoGetEditedConfig(const json& params)
{
    auto* bundle = wxGetApp().preset_bundle;
    if (!bundle)
        return {{"success", false}, {"message", "PresetBundle not available"}};

    json result;
    result["success"] = true;
    result["message"] = "OK";

    const auto& print_cfg    = bundle->prints.get_edited_preset().config;
    const auto& filament_cfg = bundle->filaments.get_edited_preset().config;
    const auto& printer_cfg  = bundle->printers.get_edited_preset().config;

    // 获取当前选中对象的 per-object config（用于覆盖全局值）
    const ModelConfig* object_cfg = nullptr;
    if (auto* plater = wxGetApp().plater()) {
        auto& selection = plater->get_selection();
        const int obj_idx = selection.get_object_idx();
        const auto& model = plater->model();
        if (obj_idx >= 0 && obj_idx < (int)model.objects.size() && model.objects[obj_idx]) {
            const auto& obj_config = model.objects[obj_idx]->config;
            if (!obj_config.empty())
                object_cfg = &obj_config;
        }
    }

    // get_opt: 优先从对象 per-object config 取，fallback 到全局 preset
    auto get_opt = [&](const std::string& key) -> std::string {
        if (object_cfg) {
            const ConfigOption* opt = object_cfg->option(key);
            if (opt) return opt->serialize();
        }
        const ConfigOption* opt = print_cfg.option(key);
        if (!opt) opt = filament_cfg.option(key);
        if (!opt) opt = printer_cfg.option(key);
        return opt ? opt->serialize() : "";
    };

    auto get_filament_opt = [&](const DynamicPrintConfig& cfg, const std::string& key) -> std::string {
        const ConfigOption* opt = cfg.option(key);
        return opt ? opt->serialize() : "";
    };

    // If caller specified keys, return only those
    if (params.contains("keys") && params["keys"].is_array()) {
        json values;
        for (const auto& k : params["keys"]) {
            const std::string key = k.get<std::string>();
            const std::string v = get_opt(key);
            if (!v.empty()) values[key] = v;
        }
        result["config"] = values;
        return result;
    }

    // Default: return all keys that have a value in the current print preset.
    // This eliminates the need for a hardcoded whitelist and covers every parameter
    // visible in the process panel, including those added in future versions.
    for (const auto& [key, def] : print_config_def.options) {
        if (!def.is_scalar()) continue;
        if (def.label.empty()) continue;
        {
            auto is_blank = [](const std::string& s) {
                return s.find_first_not_of(" \t\r\n") == std::string::npos;
            };
            if (is_blank(def.label)) continue;
        }
        if (def.printer_technology == ptSLA) continue;
        if (def.mode == comDevelop) continue;
        const std::string v = get_opt(key);
        if (!v.empty()) result["print"][key] = v;
    }

    const std::vector<std::string> filament_keys = {
        "nozzle_temperature", "nozzle_temperature_initial_layer",
        "bed_temperature", "bed_temperature_initial_layer",
        "filament_flow_ratio", "fan_min_speed", "fan_max_speed"
    };
    for (const auto& k : filament_keys) {
        const std::string v = get_filament_opt(filament_cfg, k);
        if (!v.empty()) result["filament"][k] = v;
    }

    result["printer"]["printer_model"]  = get_filament_opt(printer_cfg, "printer_model");
    result["printer"]["nozzle_diameter"] = get_filament_opt(printer_cfg, "nozzle_diameter");

    return result;
}

json SlicerBridge::DoSelectPreset(const json& params)
{
    std::string type = params.value("type", "");
    std::string name = params.value("name", "");

    if (type.empty() || name.empty())
        return {{"success", false}, {"message", "type and name are required"}};

    auto* tab = wxGetApp().get_tab(
        type == "print"    ? Preset::TYPE_PRINT
      : type == "filament" ? Preset::TYPE_FILAMENT
      : type == "printer"  ? Preset::TYPE_PRINTER
                           : Preset::TYPE_INVALID);
    if (!tab)
        return {{"success", false}, {"message", "Invalid preset type: " + type}};

    tab->select_preset(name);
    return {{"success", true}, {"message", "Preset selected"},
            {"type", type}, {"name", name}};
}

json SlicerBridge::DoApplyConfig(const json& params)
{
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};

    auto* bundle = wxGetApp().preset_bundle;
    if (!bundle)
        return {{"success", false}, {"message", "PresetBundle not available"}};

    // Compat: LLM sometimes calls with {"parameter": "key", "value": val}
    // instead of the expected {"key": val} flat form. Detect and convert.
    json resolved_params = params;
    if (params.is_object() && params.contains("parameter") && params.contains("value")) {
        const auto& param_val = params["parameter"];
        if (param_val.is_string()) {
            const std::string key = param_val.get<std::string>();
            // Build a corrected flat object with just the one key-value pair,
            // preserving any extra fields (e.g. request_id) that aren't "parameter"/"value".
            resolved_params = json::object();
            for (auto& [k, v] : params.items()) {
                if (k != "parameter" && k != "value")
                    resolved_params[k] = v;
            }
            resolved_params[key] = params["value"];
            BOOST_LOG_TRIVIAL(info)
                << "[SlicerBridge] DoApplyConfig: converted {parameter,value} format"
                << " key=" << key;
        }
    }
    // Also handle array form: [{"parameter": "k", "value": v}, ...]
    else if (params.is_array()) {
        resolved_params = json::object();
        for (const auto& item : params) {
            if (item.is_object() && item.contains("parameter") && item.contains("value")) {
                const auto& pv = item["parameter"];
                if (pv.is_string())
                    resolved_params[pv.get<std::string>()] = item["value"];
            }
        }
    }

    // Get mutable references to edited preset configs
    auto& print_cfg    = bundle->prints.get_edited_preset().config;
    auto& filament_cfg = bundle->filaments.get_edited_preset().config;
    auto& printer_cfg  = bundle->printers.get_edited_preset().config;

    // Helper: get current value from the appropriate preset config
    auto get_current_value = [&](const std::string& key) -> std::string {
        const ConfigOption* opt = print_cfg.option(key);
        if (!opt) opt = filament_cfg.option(key);
        if (!opt) opt = printer_cfg.option(key);
        return opt ? opt->serialize() : "";
    };

    // Helper: find which config owns this key and apply the value
    auto apply_to_preset = [&](const std::string& key, const std::string& str_val) -> bool {
        // Try to deserialize into a temp config to validate
        DynamicPrintConfig tmp;
        tmp.set_deserialize_strict(key, str_val);  // throws on error
        const ConfigOption* new_opt = tmp.option(key);
        if (!new_opt) return false;

        // Write directly to the appropriate preset edited config
        if (print_cfg.option(key))
            print_cfg.set_key_value(key, new_opt->clone());
        else if (filament_cfg.option(key))
            filament_cfg.set_key_value(key, new_opt->clone());
        else if (printer_cfg.option(key))
            printer_cfg.set_key_value(key, new_opt->clone());
        else {
            // Key exists in print_config_def but not in any current preset;
            // add to print config as default
            print_cfg.set_key_value(key, new_opt->clone());
        }
        return true;
    };

    int applied = 0;
    json changes = json::array();
    json warnings = json::array();

    // ---- Per-object parameter path ----
    // Apply to object config when:
    // 1. Explicitly requested via scope=="object" or object_name, OR
    // 2. An object is currently selected in the UI
    // When no object is selected, always write to the global preset.
    const std::string scope_val = resolved_params.value("scope", "");
    const std::string object_name_val = resolved_params.value("object_name", "");

    // Determine target object
    auto* model = plater ? &plater->model() : nullptr;
    ModelObject* target_object = nullptr;

    if (model) {
        if (!object_name_val.empty()) {
            // Explicit object name — find it
            for (auto* obj : model->objects) {
                if (!obj) continue;
                if (obj->name == object_name_val) { target_object = obj; break; }
                const wxString wx_obj = wxString::FromUTF8(obj->name.c_str());
                const wxString wx_tgt = wxString::FromUTF8(object_name_val.c_str());
                if (wx_obj == wx_tgt) { target_object = obj; break; }
            }
        }
        // Only fall back to selected object — do NOT auto-target the sole scene object.
        // When nothing is selected the intent is global preset modification.
        if (!target_object) {
            const int obj_idx = plater->get_selection().get_object_idx();
            if (obj_idx >= 0 && obj_idx < (int)model->objects.size())
                target_object = model->objects[obj_idx];
        }
    }

    const bool apply_to_object = target_object != nullptr &&
        (scope_val == "object" || !object_name_val.empty() || target_object != nullptr);

    if (apply_to_object) {
        BOOST_LOG_TRIVIAL(info)
            << "[SlicerBridge] DoApplyConfig per-object:"
            << " target=" << (target_object ? target_object->name : "(null)")
            << " object_count=" << (model ? (int)model->objects.size() : -1);

        if (!target_object)
            return {{"success", false}, {"message", "Target object not found for per-object config"}};

        // Skip meta fields when iterating params
        static const std::unordered_set<std::string> skip_keys = {"scope", "object_name", "request_id"};

        for (auto& [key, value] : resolved_params.items()) {
            if (skip_keys.count(key)) continue;
            if (!print_config_def.has(key)) {
                warnings.push_back("Unknown config key: " + key);
                continue;
            }

            try {
                std::string str_val;
                if (value.is_number_float())
                    str_val = std::to_string(value.get<double>());
                else if (value.is_number_integer())
                    str_val = std::to_string(value.get<int>());
                else if (value.is_string())
                    str_val = value.get<std::string>();
                else if (value.is_boolean())
                    str_val = value.get<bool>() ? "1" : "0";
                else
                    continue;

                // Get old value from object config, fallback to preset
                std::string old_val;
                {
                    const ConfigOption* old_opt = target_object->config.option(key);
                    if (old_opt)
                        old_val = old_opt->serialize();
                    else
                        old_val = get_current_value(key);
                }

                DynamicPrintConfig tmp;
                tmp.set_deserialize_strict(key, str_val);
                const ConfigOption* new_opt = tmp.option(key);
                if (!new_opt) continue;

                target_object->config.set_key_value(key, new_opt->clone());
                std::string new_val = target_object->config.option(key)->serialize();

                if (new_val != old_val) {
                    changes.push_back({{"key", key}, {"old", old_val}, {"new", new_val}});
                    ++applied;
                }
            } catch (const std::exception& e) {
                warnings.push_back("Failed to set " + key + ": " + e.what());
            } catch (...) {
                warnings.push_back("Failed to set " + key);
            }
        }

        if (!changes.empty()) {
            wxGetApp().obj_list()->object_config_options_changed({target_object, nullptr});
            wxGetApp().plater()->update();
            // 刷新参数面板，使 UI 立即显示新的对象参数
            wxGetApp().obj_list()->update_and_show_object_settings_item();
            if (auto* pp = wxGetApp().params_panel()) {
                pp->notify_object_config_changed();
                pp->switch_to_object_if_has_object_configs();
            }
        }

        json result;
        result["success"] = true;
        result["message"] = changes.empty()
            ? _u8L("Config already up to date")
            : format(_u8L("%1% config keys applied"), applied);
        result["changes"] = changes;
        result["scope"] = "object";
        result["object_name"] = target_object->name;
        if (!warnings.empty())
            result["warnings"] = warnings;
        return result;
    }

    for (auto& [key, value] : resolved_params.items()) {
        // Validate key exists in print_config_def
        if (!print_config_def.has(key)) {
            warnings.push_back("Unknown config key: " + key);
            continue;
        }

        std::string old_val = get_current_value(key);

        try {
            std::string str_val;
            if (value.is_number_float())
                str_val = std::to_string(value.get<double>());
            else if (value.is_number_integer())
                str_val = std::to_string(value.get<int>());
            else if (value.is_string())
                str_val = value.get<std::string>();
            else if (value.is_boolean())
                str_val = value.get<bool>() ? "1" : "0";

            apply_to_preset(key, str_val);

            std::string new_val = get_current_value(key);
            if (new_val != old_val) {
                changes.push_back({{"key", key}, {"old", old_val}, {"new", new_val}});
                ++applied;
            }
        } catch (const std::exception& e) {
            warnings.push_back("Failed to set " + key + ": " + e.what());
            BOOST_LOG_TRIVIAL(warning) << "[SlicerBridge] Failed to set config key: " << key << " - " << e.what();
        } catch (...) {
            warnings.push_back("Failed to set " + key);
            BOOST_LOG_TRIVIAL(warning) << "[SlicerBridge] Failed to set config key: " << key;
        }
    }

    // ---- Smart support linkage ----
    {
        bool has_enable_support = resolved_params.contains("enable_support");
        bool has_support_type   = resolved_params.contains("support_type");

        if (has_enable_support && !has_support_type) {
            const auto& val = resolved_params["enable_support"];
            bool enabling = false;
            if (val.is_boolean()) enabling = val.get<bool>();
            else if (val.is_number_integer()) enabling = val.get<int>() != 0;
            else if (val.is_string()) enabling = (val.get<std::string>() == "1" || val.get<std::string>() == "true");

            if (enabling) {
                // When enabling support, always ensure support_type is normal(auto)
                std::string old_type = get_current_value("support_type");
                try {
                    apply_to_preset("support_type", "normal(auto)");
                    std::string new_type = get_current_value("support_type");
                    if (new_type != old_type)
                        changes.push_back({{"key", "support_type"}, {"old", old_type}, {"new", new_type}});
                } catch (...) {}
            }
        }

        if (has_support_type && !has_enable_support) {
            std::string old_enable = get_current_value("enable_support");
            if (old_enable != "1") {
                try {
                    apply_to_preset("enable_support", "1");
                    changes.push_back({{"key", "enable_support"}, {"old", old_enable}, {"new", "1"}});
                } catch (...) {}
            }
        }
    }

    // Notify plater with full config (same approach as UI code)
    if (!changes.empty())
        plater->on_config_change(bundle->full_config());

    json result;
    result["success"] = true;
    result["message"] = changes.empty()
        ? _u8L("Config already up to date")
        : format(_u8L("%1% config keys applied"), changes.size());
    result["changes"] = changes;
    if (!warnings.empty())
        result["warnings"] = warnings;
    return result;
}

json SlicerBridge::BuildConfigSchemaArray()
{
    auto type_to_string = [](ConfigOptionType t) -> std::string {
        switch (t) {
            case coFloat:
            case coFloats:          return "float";
            case coInt:
            case coInts:            return "int";
            case coBool:
            case coBools:           return "bool";
            case coString:
            case coStrings:         return "string";
            case coPercent:
            case coPercents:        return "percent";
            case coFloatOrPercent:  return "float_or_percent";
            case coEnum:            return "enum";
            case coPoint:
            case coPoints:          return "point";
            default:                return "other";
        }
    };

    json options_arr = json::array();

    for (const auto& [key, def] : print_config_def.options) {
        // Skip vector types and internal/hidden options
        if (!def.is_scalar()) continue;
        if (def.label.empty()) continue;
        // Skip SLA/internal params whose label or category is whitespace-only
        {
            auto is_blank = [](const std::string& s) {
                return s.find_first_not_of(" \t\r\n") == std::string::npos;
            };
            if (is_blank(def.label)) continue;
        }
        // Only expose FFF (or technology-agnostic) parameters — skip SLA-only params
        if (def.printer_technology == ptSLA) continue;
        // Skip hidden/expert-only options that are not user-facing
        if (def.mode == comDevelop) continue;

        json item;
        item["key"]      = key;
        item["label"]    = def.label;
        // Include localized label so the agent can match user input in any language.
        const std::string localized_label = _u8L(def.label.c_str());
        if (localized_label != def.label)
            item["label_localized"] = localized_label;
        item["type"]     = type_to_string(def.type);
        item["category"] = def.category.empty() ? "Other" : def.category;
        if (!def.tooltip.empty())
            item["tooltip"] = def.tooltip;
        if (!def.sidetext.empty())
            item["unit"] = def.sidetext;
        if (def.min != INT_MIN)
            item["min"] = def.min;
        if (def.max != INT_MAX)
            item["max"] = def.max;

        // For enums, include allowed values
        if (!def.enum_values.empty()) {
            item["enum_values"] = def.enum_values;
            if (!def.enum_labels.empty())
                item["enum_labels"] = def.enum_labels;
        }

        // Include default value if available
        if (def.default_value)
            item["default"] = def.default_value->serialize();

        options_arr.push_back(std::move(item));
    }

    return options_arr;
}

bool SlicerBridge::ExportConfigSchemaToFile(const std::string& utf8_path,
                                            std::string* out_message)
{
    try {
        json schema = BuildConfigSchemaArray();
        std::ofstream ofs(utf8_path, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!ofs.is_open()) {
            if (out_message)
                *out_message = "failed to open file for writing: " + utf8_path;
            return false;
        }
        ofs << schema.dump(2);
        ofs.close();
        if (!ofs) {
            if (out_message)
                *out_message = "write error while flushing: " + utf8_path;
            return false;
        }
        if (out_message)
            *out_message = std::to_string(schema.size()) + " options written to " + utf8_path;
        return true;
    } catch (const std::exception& e) {
        if (out_message)
            *out_message = std::string("exception: ") + e.what();
        return false;
    } catch (...) {
        if (out_message)
            *out_message = "unknown exception while exporting config schema";
        return false;
    }
}

json SlicerBridge::DoGetConfigOptions(const json& params)
{
    std::string filter_category = params.value("category", "");

    json options_arr = BuildConfigSchemaArray();

    // Apply category filter if specified (post-build to keep BuildConfigSchemaArray pure)
    if (!filter_category.empty()) {
        json filtered = json::array();
        for (auto& item : options_arr) {
            if (item.value("category", "") == filter_category)
                filtered.push_back(std::move(item));
        }
        options_arr = std::move(filtered);
    }

    return {{"success", true},
            {"message", std::to_string(options_arr.size()) + " config options"},
            {"options", options_arr}};
}


} // namespace Bridge
} // namespace GUI
} // namespace Slic3r

