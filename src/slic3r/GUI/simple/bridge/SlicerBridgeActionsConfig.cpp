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
#include <unordered_map>

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

    // Meta fields that may ride along in the tool-call args but must never be
    // treated as slicer config keys. The SAgent native dispatcher no longer
    // injects workflow_id/request_id for this immediate bridge path (fixed at the
    // source), but we keep a defensive filter here so any other caller (WebSocket /
    // HTTP direct bridge calls) can't reintroduce spurious "Unknown config key"
    // warnings. "scope"/"object_name" are consumed above; "parameter"/"value"/
    // "patch" are handled by the arg-shape normalization.
    static const std::unordered_set<std::string> kMetaKeys = {
        "scope", "object_name", "request_id", "workflow_id", "workflowId",
        "parameter", "value", "patch"
    };

    // Compute the set of config keys whose UI fields are currently DISABLED, by
    // replaying the professional panel's linkage rules (ConfigManipulation::
    // toggle_print_fff_options) against the current full config.
    //
    // We can't rely on querying the live wx field's IsEnabled(): in AI (easy)
    // mode the professional settings Tab isn't (re)built/toggled, so its fields
    // report as enabled even when their linkage condition (e.g. skirt_loops==0
    // greys out skirt_height) says otherwise. Running the linkage rules against
    // the config values is the authoritative source of truth and works
    // regardless of whether the Tab UI exists.
    //
    // A throwaway ConfigManipulation with capture-only callbacks lets us collect
    // the toggle decisions without touching any real UI. We feed it a copy of the
    // config because toggle_print_fff_options may normalize a few values in place.
    //
    // Crucially, we pre-apply the incoming patch onto the copy first, so linkage
    // is evaluated against the *resulting* config. This keeps existing behaviour
    // intact: e.g. changing skirt_loops and skirt_height in one request, or the
    // support_type -> enable_support smart linkage below, must still succeed even
    // though the target field looks disabled under the current (pre-patch) config.
    std::unordered_set<std::string> disabled_keys;
    {
        DynamicPrintConfig cfg_copy = bundle->full_config();
        for (auto& [key, value] : resolved_params.items()) {
            if (kMetaKeys.count(key) || !print_config_def.has(key))
                continue;
            try {
                std::string str_val;
                if (value.is_number_float())        str_val = std::to_string(value.get<double>());
                else if (value.is_number_integer()) str_val = std::to_string(value.get<int>());
                else if (value.is_string())         str_val = value.get<std::string>();
                else if (value.is_boolean())        str_val = value.get<bool>() ? "1" : "0";
                else                                continue;
                cfg_copy.set_deserialize_strict(key, str_val);
            } catch (...) {
                // Ignore un-parseable values here; the real apply loop below will
                // surface the appropriate warning.
            }
        }
        // Mirror the smart-support linkage applied later: changing support_type
        // implicitly enables support. Reflect it here so support_type and its
        // dependent fields aren't wrongly judged as disabled.
        if (resolved_params.contains("support_type") && !resolved_params.contains("enable_support")) {
            try { cfg_copy.set_deserialize_strict("enable_support", "1"); } catch (...) {}
        }
        ConfigManipulation probe(
            /*load_config*/    []() {},
            /*cb_toggle_field*/[&disabled_keys](const std::string& opt_key, bool toggle, int /*opt_index*/) {
                if (!toggle) disabled_keys.insert(opt_key);
                else         disabled_keys.erase(opt_key);
            },
            /*cb_toggle_line*/ [](const std::string&, bool) {},
            /*cb_value_change*/[](const std::string&, const boost::any&) {},
            /*local_config*/   nullptr,
            /*msg_dlg_parent*/ nullptr);
        try {
            probe.toggle_print_fff_options(&cfg_copy, /*is_global_config*/ true);
        } catch (...) {
            // If linkage evaluation fails for any reason, fall back to "no
            // restrictions" so we never wrongly block a legitimate edit.
            disabled_keys.clear();
        }
    }

    // Helper: a key is editable unless the linkage rules marked it disabled.
    auto is_field_editable = [&](const std::string& key) -> bool {
        return disabled_keys.find(key) == disabled_keys.end();
    };

    // Keys that were rejected because their UI field is currently non-editable
    // (linkage-disabled). Tracked separately so we can fail the whole call with a
    // clear "not modifiable" message instead of returning a success card. Each
    // entry is an object {key, label} where label is the localized display name.
    json blocked_keys = json::array();

    // Helper: localized display name for a config key (falls back to the key).
    // Used only for the PARAM_NOT_MODIFIABLE message; the apply_config change
    // rows carry no label — the frontend resolves display names from its own
    // multilingual parameterLabels map (single source of truth for UI naming).
    auto display_label_for = [&](const std::string& key) -> std::string {
        auto it = print_config_def.options.find(key);
        if (it != print_config_def.options.end() && !it->second.label.empty()) {
            const std::string localized = _u8L(it->second.label.c_str());
            if (!localized.empty())
                return localized;
        }
        return key;
    };

    // Helper: record a key rejected as non-editable, capturing its display label
    // so the agent can build a friendly "can't modify <name> yet" message.
    auto record_blocked = [&](const std::string& key) {
        blocked_keys.push_back({{"key", key}, {"label", display_label_for(key)}});
    };

    // Helper: build a "not modifiable" message listing the blocked display names.
    auto build_blocked_message = [](const json& keys) -> std::string {
        std::string list;
        for (const auto& entry : keys) {
            std::string name;
            if (entry.is_object())
                name = entry.value("label", entry.value("key", std::string()));
            else if (entry.is_string())
                name = entry.get<std::string>();
            if (name.empty()) continue;
            if (!list.empty()) list += "、";
            list += name;
        }
        return format(_u8L("\"%1%\" cannot be modified for now, likely because a prerequisite option isn't enabled"), list);
    };

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

        for (auto& [key, value] : resolved_params.items()) {
            if (kMetaKeys.count(key)) continue;
            if (!print_config_def.has(key)) {
                warnings.push_back("Unknown config key: " + key);
                continue;
            }
            if (!is_field_editable(key)) {
                record_blocked(key);
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
                    changes.push_back({{"key", key}, {"old_value", old_val}, {"new_value", new_val}});
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

        // If every requested key was rejected as non-editable and nothing was
        // applied, fail the call so the agent replies in natural language
        // ("this parameter can't be modified") instead of rendering a success card.
        if (changes.empty() && !blocked_keys.empty()) {
            return {
                {"success", false},
                {"code", "PARAM_NOT_MODIFIABLE"},
                {"message", build_blocked_message(blocked_keys)},
                {"blocked_keys", blocked_keys},
                {"scope", "object"}
            };
        }

        json result;
        result["success"] = true;
        result["message"] = changes.empty()
            ? _u8L("Config already up to date")
            : format(_u8L("%1% config keys applied"), applied);
        result["changes"] = changes;
        result["scope"] = "object";
        result["object_name"] = target_object->name;
        if (!blocked_keys.empty())
            result["blocked_keys"] = blocked_keys;
        if (!warnings.empty())
            result["warnings"] = warnings;
        return result;
    }

    for (auto& [key, value] : resolved_params.items()) {
        // Skip meta fields (workflow_id / request_id / scope ...) that ride along
        // in the args but are not slicer config keys.
        if (kMetaKeys.count(key)) continue;
        // Validate key exists in print_config_def
        if (!print_config_def.has(key)) {
            warnings.push_back("Unknown config key: " + key);
            continue;
        }
        // Block writes to fields that are currently disabled/greyed out in the UI,
        // surfacing a clear warning instead of silently applying an invisible change.
        if (!is_field_editable(key)) {
            record_blocked(key);
            continue;
        }

        // ---- Special case: flush_multiplier lives in project_config, not in any preset ----
        if (key == "flush_multiplier") {
            try {
                float new_fval = 0.f;
                if (value.is_number_float())        new_fval = static_cast<float>(value.get<double>());
                else if (value.is_number_integer()) new_fval = static_cast<float>(value.get<int>());
                else if (value.is_string())         new_fval = std::stof(value.get<std::string>());
                else { warnings.push_back("flush_multiplier: unsupported value type"); continue; }

                auto& project_config = bundle->project_config;
                ConfigOptionFloat* opt = project_config.option<ConfigOptionFloat>("flush_multiplier");
                if (!opt) { warnings.push_back("flush_multiplier: option not found in project_config"); continue; }

                const float old_fval = opt->getFloat();
                opt->set(new ConfigOptionFloat(new_fval));
                wxGetApp().app_config->set("flush_multiplier", std::to_string(new_fval));
                wxGetApp().preset_bundle->export_selections(*wxGetApp().app_config);
                plater->update_project_dirty_from_presets();

                // Always record the change so the frontend diff card shows old→new,
                // even when old == new (user explicitly requested this value).
                // Use old_value/new_value to match the ActionResultCard field names.
                const std::string old_str = [old_fval]() {
                    std::ostringstream ss;
                    ss << old_fval;
                    return ss.str();
                }();
                const std::string new_str = [new_fval]() {
                    std::ostringstream ss;
                    ss << new_fval;
                    return ss.str();
                }();
                changes.push_back({{"key", key}, {"old_value", old_str}, {"new_value", new_str}});
                ++applied;
                BOOST_LOG_TRIVIAL(info)
                    << "[SlicerBridge] flush_multiplier set via project_config: "
                    << old_fval << " -> " << new_fval;
            } catch (const std::exception& e) {
                warnings.push_back(std::string("Failed to set flush_multiplier: ") + e.what());
            }
            continue;  // handled, skip normal preset path
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
                changes.push_back({{"key", key}, {"old_value", old_val}, {"new_value", new_val}});
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
                        changes.push_back({{"key", "support_type"}, {"old_value", old_type}, {"new_value", new_type}});
                } catch (...) {}
            }
        }

        if (has_support_type && !has_enable_support) {
            std::string old_enable = get_current_value("enable_support");
            if (old_enable != "1") {
                try {
                    apply_to_preset("enable_support", "1");
                    changes.push_back({{"key", "enable_support"}, {"old_value", old_enable}, {"new_value", std::string("1")}});
                } catch (...) {}
            }
        }
    }

    // If every requested key was rejected as non-editable and nothing was
    // applied, fail the call so the agent replies in natural language
    // ("this parameter can't be modified") instead of rendering a success card.
    if (changes.empty() && !blocked_keys.empty()) {
        return {
            {"success", false},
            {"code", "PARAM_NOT_MODIFIABLE"},
            {"message", build_blocked_message(blocked_keys)},
            {"blocked_keys", blocked_keys}
        };
    }

    // Notify plater with full config (same approach as UI code)
    if (!changes.empty()) {
        plater->on_config_change(bundle->full_config());

        // Refresh the professional Print settings Tab so the parameter panel's
        // field enable/disable linkage (toggle_options) reflects the new values.
        // Without this, changing e.g. "enable_support" from the AI side leaves the
        // dependent support fields greyed out until the user manually re-toggles it.
        if (auto* print_tab = wxGetApp().get_tab(Preset::TYPE_PRINT)) {
            print_tab->update_dirty();
            print_tab->reload_config();
            print_tab->update();
        }
    }

    json result;
    result["success"] = true;
    result["message"] = changes.empty()
        ? _u8L("Config already up to date")
        : format(_u8L("%1% config keys applied"), changes.size());
    result["changes"] = changes;
    if (!blocked_keys.empty())
        result["blocked_keys"] = blocked_keys;
    if (!warnings.empty())
        result["warnings"] = warnings;
    return result;
}

json SlicerBridge::BuildPanelNameMap()
{
    // key -> { group_en, group_loc, line_loc }
    //
    // Translation note: optgroup titles are created with the L(...) macro, which
    // is only a *marker* for xgettext and returns the ENGLISH original at runtime;
    // the actual translation happens at display time. So og->title holds English
    // and must be translated here via _u8L. Line labels, by contrast, are built
    // with Line(label) → label(_(label)), i.e. already translated — do not
    // re-translate them.
    json map = json::object();

    auto harvest_tab = [&](Preset::Type type) {
        Tab* tab = wxGetApp().get_tab(type);
        if (!tab) return;
        for (const auto& page : tab->get_pages()) {
            if (!page) continue;
            for (const auto& og : page->m_optgroups) {
                if (!og) continue;
                const std::string group_en  = og->title.utf8_string();
                const std::string group_loc = _u8L(og->title);
                for (const Line& line : og->get_lines()) {
                    if (line.is_separator()) continue;
                    const auto& opts = line.get_options();
                    // A line label only adds meaning when the line bundles
                    // multiple options (otherwise it duplicates the option name).
                    const bool line_adds_context = opts.size() > 1 && !line.label.IsEmpty();
                    for (const auto& opt : opts) {
                        const std::string& k = opt.opt_id;
                        if (k.empty() || map.contains(k)) continue;
                        json entry;
                        entry["group_en"]  = group_en;
                        entry["group_loc"] = group_loc;
                        if (line_adds_context)
                            entry["line_loc"] = line.label.utf8_string();
                        map[k] = std::move(entry);
                    }
                }
            }
        }
    };
    harvest_tab(Preset::TYPE_PRINT);
    harvest_tab(Preset::TYPE_FILAMENT);
    harvest_tab(Preset::TYPE_PRINTER);
    return map;
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

    // Authoritative parameter classification (bug 17390): the AI assistant may
    // only modify *process* parameters. Filament/printer parameters are exported
    // with param_class so the agent can reject AI edits to them (and steer the
    // user to the professional mode) instead of mis-mapping them onto the closest
    // process key. The classification sets come straight from the preset schema —
    // no hand-maintained tables.
    auto build_key_set = [](const std::vector<std::string>& keys) {
        return std::unordered_set<std::string>(keys.begin(), keys.end());
    };
    const std::unordered_set<std::string> filament_keys = build_key_set(Preset::filament_options());
    const std::unordered_set<std::string> printer_keys  = build_key_set(Preset::printer_options());
    auto classify_param = [&](const std::string& key) -> std::string {
        if (filament_keys.count(key)) return "filament";
        if (printer_keys.count(key))  return "printer";
        return "process";
    };

    // Build an authoritative display-name map from the professional-panel layout.
    //
    // A parameter's user-facing name lives in the GUI layout (Tab → Page →
    // OptionsGroup → Line), NOT in ConfigOptionDef. E.g. bridge_speed's def.label
    // is just "External"; the meaningful name comes from its optgroup ("桥接" /
    // Bridge) and, when a line groups several options, the per-option label.
    //
    // Recover each parameter's real user-facing group/line name (e.g. "线宽",
    // "桥接") from the professional-panel layout — this lives in the GUI layout,
    // not in ConfigOptionDef.
    const json panel_names = BuildPanelNameMap();

    json options_arr = json::array();

    for (const auto& [key, def] : print_config_def.options) {
        // Skip only genuinely non-editable vector/coordinate types (points).
        // NOTE: per-extruder scalars (e.g. pressure_advance, nozzle_temperature,
        // nozzle_diameter) are stored as ConfigOptionFloats/Ints vectors and would
        // be dropped by a blanket !is_scalar() filter — yet the agent must know them
        // to reject AI edits. Keep them; exclude only point/points containers.
        if (def.type == coPoint || def.type == coPoints) continue;
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

        // Build a disambiguated "<group> / <leaf>" name from the professional
        // panel — exactly what the user sees and says ("线宽 / 外墙",
        // "桥接 / 外部"). English and localized are built symmetrically so they
        // never mix languages.
        //   panel group + line  → group + line label   (multi-option line)
        //   panel group + def   → group + this option's own label
        //   no panel entry      → category + def label  → def label (fallback)
        const std::string def_leaf_en  =
            !def.full_label.empty() ? def.full_label : def.label;
        const std::string def_leaf_loc = _u8L(def.label.c_str());

        auto compose = [](const std::string& group, const std::string& leaf) -> std::string {
            if (group.empty()) return leaf;
            if (leaf.empty() || leaf == group) return group;
            return group + " / " + leaf;
        };

        // Join non-empty segments with " / ", skipping duplicates/empties.
        auto join_path = [](std::initializer_list<std::string> segs) -> std::string {
            std::string out;
            for (const std::string& s : segs) {
                if (s.empty()) continue;
                if (!out.empty()) {
                    // Avoid immediate duplicate segments (e.g. group == line).
                    const std::string tail = " / " + s;
                    if (out == s || (out.size() >= tail.size() &&
                                     out.compare(out.size() - tail.size(), tail.size(), tail) == 0))
                        continue;
                    out += " / ";
                }
                out += s;
            }
            return out;
        };

        std::string label_en;
        std::string label_loc;
        auto pn_it = panel_names.find(key);
        if (pn_it != panel_names.end() && !pn_it->value("group_loc", std::string()).empty()) {
            const json& pn = *pn_it;
            const std::string group_en  = pn.value("group_en", std::string());
            const std::string group_loc = pn.value("group_loc", std::string());
            const std::string line_loc  = pn.value("line_loc", std::string());
            // Full hierarchy as shown in the professional panel:
            //   optgroup / line / option   e.g. "悬垂速度 / 桥接 / 内部"
            // The line segment only appears for multi-option lines (single-option
            // lines duplicate the option name, so line_loc is empty there) →
            //   "线宽 / 外墙"  (two segments)
            label_loc = join_path({group_loc, line_loc, def_leaf_loc});
            // English has no per-line string, so it degrades to group / option.
            label_en = join_path({group_en, def_leaf_en});
        } else if (!def.category.empty()) {
            // Not on any panel: fall back to category for some grouping context.
            label_en  = compose(def.category, def_leaf_en);
            label_loc = compose(_u8L(def.category.c_str()), def_leaf_loc);
        } else {
            label_en  = def_leaf_en;
            label_loc = def_leaf_loc;
        }

        item["label"] = label_en;
        if (!label_loc.empty() && label_loc != label_en)
            item["label_localized"] = label_loc;

        item["type"]     = type_to_string(def.type);
        item["category"] = def.category.empty() ? "Other" : def.category;
        // process | filament | printer — consumed by the agent's param domain guard.
        item["param_class"] = classify_param(key);
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

        // Include default value if available.
        //
        // IMPORTANT: enum option types (coEnum / coEnums) must NOT be serialized
        // here. Their serialize() dereferences an internal `keys_map` pointer that
        // is only populated when the option is created via load_option_from_archive
        // — the ConfigOptionDef::default_value objects are constructed without it,
        // so `keys_map` is null and serialize() causes an access violation (a
        // hardware/SEH fault, which try/catch(...) does NOT catch under the default
        // /EHsc model). Instead we recover the default from enum_values using the
        // stored integer index, which lives safely in the ConfigOptionInt(s) base.
        if (def.default_value) {
            const ConfigOptionType t = def.default_value->type();
            if (t == coEnum || t == coEnums) {
                // Recover the enum index without touching the (null) keys_map.
                // coEnum is a scalar (getInt); coEnums is an int vector (get_at).
                int idx = -1;
                try {
                    if (t == coEnum) {
                        idx = def.default_value->getInt();
                    } else {
                        // coEnums derives from ConfigOptionInts (int vector); the
                        // type() check above guarantees the runtime type, so a
                        // static_cast is safe and avoids RTTI/dynamic_cast issues.
                        const auto* ev =
                            static_cast<const ConfigOptionInts*>(def.default_value.get());
                        if (!ev->values.empty())
                            idx = ev->values.front();
                    }
                } catch (...) {
                    idx = -1;
                }
                if (idx >= 0 && idx < static_cast<int>(def.enum_values.size()))
                    item["default"] = def.enum_values[idx];
            } else {
                // Non-enum scalars/vectors serialize safely (nullable vectors emit
                // "nil" tokens rather than crashing).
                item["default"] = def.default_value->serialize();
            }
        }

        options_arr.push_back(std::move(item));
    }

    // Sort for a stable, human-readable layout: group by param_class
    // (process → filament → printer). The output stays a single flat array —
    // consumers only read fields, not order — but the file is far easier to
    // scan/diff with each class in its own contiguous block.
    auto class_rank = [](const std::string& c) -> int {
        if (c == "process")  return 0;
        if (c == "filament") return 1;
        if (c == "printer")  return 2;
        return 3;
    };
    // Within the process block, order categories by the professional-panel tabs:
    // 质量 → 强度 → 速度 → 支撑 → 材料 → 其他. Categories not in this list sort
    // after the named ones (rank 100), then alphabetically. filament/printer
    // blocks fall back to plain alphabetical category order.
    auto category_rank = [](const std::string& cat) -> int {
        if (cat == "Quality")  return 0;
        if (cat == "Strength") return 1;
        if (cat == "Speed")    return 2;
        if (cat == "Support")  return 3;
        if (cat == "Material") return 4;
        if (cat == "Other" || cat == "Others") return 5;
        return 100;
    };
    std::sort(options_arr.begin(), options_arr.end(),
              [&](const json& a, const json& b) {
        const std::string cls_a = a.value("param_class", "process");
        const std::string cls_b = b.value("param_class", "process");
        const int ra = class_rank(cls_a);
        const int rb = class_rank(cls_b);
        if (ra != rb) return ra < rb;

        const std::string ca = a.value("category", "");
        const std::string cb = b.value("category", "");
        // Business-ordered categories for the process block; alphabetical elsewhere.
        if (cls_a == "process") {
            const int cra = category_rank(ca);
            const int crb = category_rank(cb);
            if (cra != crb) return cra < crb;
        }
        if (ca != cb) return ca < cb;
        return a.value("key", "") < b.value("key", "");
    });

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

