#include "SlicerBridge.hpp"

#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/PartPlate.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/Tab.hpp"
#include "slic3r/GUI/simple/filamentMapping/FilamentMappingService.hpp"
#include "slic3r/GUI/print_manage/data/DataCenter.hpp"
#include "libslic3r/PresetBundle.hpp"
#include <boost/log/trivial.hpp>

#include <algorithm>
#include <cmath>
#include <cctype>
#include <exception>
#include <string>
#include <vector>
#include <utility>

#include <wx/event.h>

using json = nlohmann::json;

namespace Slic3r {
namespace GUI {
namespace Bridge {

namespace {

std::string trim_copy(std::string value)
{
    auto is_ws = [](unsigned char ch) { return std::isspace(ch) != 0; };
    while (!value.empty() && is_ws(static_cast<unsigned char>(value.front()))) value.erase(value.begin());
    while (!value.empty() && is_ws(static_cast<unsigned char>(value.back()))) value.pop_back();
    return value;
}

bool parse_int_value(const json& value, int& out)
{
    try {
        if (value.is_number_integer()) {
            out = value.get<int>();
            return true;
        }
        if (value.is_number()) {
            out = static_cast<int>(std::llround(value.get<double>()));
            return true;
        }
        if (value.is_string()) {
            const std::string s = trim_copy(value.get<std::string>());
            if (s.empty())
                return false;
            out = std::stoi(s);
            return true;
        }
    } catch (...) {
    }
    return false;
}

bool add_filament_like_sidebar_button()
{
    auto* plater = wxGetApp().plater();
    auto* bundle = wxGetApp().preset_bundle;
    if (!plater || !bundle)
        return false;

    // Keep behavior aligned with Plater.cpp add_btn->Bind(wxEVT_LEFT_DOWN, ...).
    int filament_count = plater->sidebar().filament_size();
    if (filament_count <= 0)
        filament_count = static_cast<int>(bundle->filament_presets.size());

    if (filament_count >= FILAMENT_SYSTEM_COLORS_NUM)
        return false;

    ++filament_count;
    const wxColour new_col = Plater::get_next_color_for_filament();
    const std::string new_color = new_col.GetAsString(wxC2S_HTML_SYNTAX).ToStdString();

    bundle->set_num_filaments(static_cast<unsigned int>(filament_count), new_color);
    plater->on_filaments_change(filament_count);

    if (auto* print_tab = wxGetApp().get_tab(Preset::TYPE_PRINT))
        print_tab->update();

    if (wxGetApp().app_config)
        bundle->export_selections(*wxGetApp().app_config);

    plater->sidebar().auto_calc_flushing_volumes(filament_count - 1);
    return true;
}

bool parse_bool_param(const json& params, const char* key, bool fallback)
{
    if (!params.is_object() || !params.contains(key))
        return fallback;

    const json& value = params[key];
    if (value.is_boolean())
        return value.get<bool>();
    if (value.is_number())
        return value.get<double>() != 0.0;
    if (value.is_string()) {
        std::string normalized = trim_copy(value.get<std::string>());
        std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        if (normalized == "true" || normalized == "1" || normalized == "yes" || normalized == "on")
            return true;
        if (normalized == "false" || normalized == "0" || normalized == "no" || normalized == "off")
            return false;
    }
    return fallback;
}

std::string parse_string_param(const json& params, const char* key, const std::string& fallback)
{
    if (!params.is_object() || !params.contains(key))
        return fallback;

    const json& value = params[key];
    if (!value.is_string())
        return fallback;

    std::string parsed = trim_copy(value.get<std::string>());
    return parsed.empty() ? fallback : parsed;
}

std::string normalized_strategy(std::string strategy)
{
    std::transform(strategy.begin(), strategy.end(), strategy.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    if (strategy == "type_then_color")
        return strategy;
    return "type_then_color";
}

bool bridge_box_matches_mode(const std::string& mode, int box_type)
{
    if (mode == "external")
        return box_type == 1;
    if (mode == "cfs")
        return box_type == 0 || box_type == 2;
    if (mode == "all")
        return box_type == 0 || box_type == 1 || box_type == 2;
    return true;
}

int count_device_materials_for_mode(const DM::Device& device, const std::string& mode)
{
    int count = 0;
    for (const auto& box : device.materialBoxes) {
        if (!bridge_box_matches_mode(mode, box.box_type))
            continue;
        for (const auto& material : box.materials) {
            if (!material.color.empty() || !material.type.empty() || !material.name.empty())
                ++count;
        }
    }
    return count;
}

json device_summary_json(const DM::Device& device, const std::string& mode)
{
    json summary;
    summary["valid"] = device.valid;
    summary["name"] = device.name;
    summary["model"] = device.model;
    summary["model_name"] = device.modelName;
    summary["mac"] = device.mac;
    summary["address"] = device.address;
    summary["online"] = device.online;
    summary["is_multi_color_device"] = device.isMultiColorDevice;
    summary["material_box_count"] = static_cast<int>(device.materialBoxes.size());
    summary["usable_material_count"] = count_device_materials_for_mode(device, mode);
    return summary;
}

json collect_unmapped_items(const json& items)
{
    json unmapped = json::array();
    if (!items.is_array())
        return unmapped;

    for (const auto& item : items) {
        if (item.is_object() && !item.value("mapped", false))
            unmapped.push_back(item);
    }
    return unmapped;
}

bool device_has_usable_materials(const DM::Device& device)
{
    for (const auto& box : device.materialBoxes) {
        for (const auto& material : box.materials) {
            if (!material.color.empty() || !material.type.empty() || !material.name.empty())
                return true;
        }
    }
    return false;
}

int count_mapped_items(const json& items)
{
    if (!items.is_array())
        return 0;

    int mapped = 0;
    for (const auto& item : items) {
        if (item.is_object() && item.value("mapped", false))
            ++mapped;
    }
    return mapped;
}


json build_mapping_summary(
    int project_filament_count,
    int device_material_count,
    int mapped_count,
    int unmapped_count,
    int type_mismatch_count)
{
    json summary;
    summary["project_filament_count"] = project_filament_count;
    summary["device_material_count"] = device_material_count;
    summary["mapped_count"] = mapped_count;
    summary["unmapped_count"] = unmapped_count;
    summary["type_mismatch_count"] = type_mismatch_count;
    return summary;
}

void attach_mapping_counts(
    json& result,
    const json& items,
    int device_material_count,
    int type_mismatch_count)
{
    const int total_count = items.is_array() ? static_cast<int>(items.size()) : 0;
    const int mapped_count = count_mapped_items(items);
    const int unmapped_count = total_count - mapped_count;

    result["mapping_required"] = total_count > 0;
    result["mapping_complete"] = total_count > 0 && unmapped_count == 0;
    result["total_count"] = total_count;
    result["mapped_count"] = mapped_count;
    result["unmapped_count"] = unmapped_count;
    result["summary"] = build_mapping_summary(
        total_count,
        device_material_count,
        mapped_count,
        unmapped_count,
        type_mismatch_count);
    result["unmapped_items"] = collect_unmapped_items(items);
}

void log_auto_map_result(const json& result)
{
    const json summary = result.contains("summary") ? result["summary"] : json::object();
    BOOST_LOG_TRIVIAL(warning)
        << "[SlicerBridge] auto_map_filaments result success="
        << result.value("success", false)
        << " code="
        << result.value("code", std::string())
        << " mapped="
        << result.value("mapped_count", 0)
        << "/"
        << result.value("total_count", 0)
        << " applied="
        << result.value("applied", false)
        << " summary="
        << summary.dump();
}

void maybe_open_filament_mapping_panel(bool enabled, json& result)
{
    if (!enabled)
        return;

    auto* plater = wxGetApp().plater();
    if (!plater)
        return;

    wxPostEvent(plater, wxCommandEvent(Slic3r::GUI::EVT_ON_MAPPING_DEVICE_FILAMENT));
    result["mapping_panel_opened"] = true;
}
} // namespace

json SlicerBridge::DoAddFilament(const json& /*params*/)
{
    auto* plater = wxGetApp().plater();
    auto* bundle = wxGetApp().preset_bundle;
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};
    if (!bundle)
        return {{"success", false}, {"message", "PresetBundle not available"}};

    const int before_count = static_cast<int>(bundle->filament_presets.size());
    const bool ok = add_filament_like_sidebar_button();
    const int after_count = static_cast<int>(bundle->filament_presets.size());

    if (!ok || after_count <= before_count) {
        return {{"success", false},
                {"message", "Failed to add filament (same behavior as Add one filament button)"},
                {"before_count", before_count},
                {"after_count", after_count}};
    }

    json result;
    result["success"] = true;
    result["message"] = "Filament added";
    result["before_count"] = before_count;
    result["after_count"] = after_count;
    result["filament_id"] = after_count;
    result["filament_index"] = after_count - 1;
    result["filament_presets"] = bundle->filament_presets;
    return result;
}

json SlicerBridge::DoDeleteFilament(const json& params)
{
    auto* plater = wxGetApp().plater();
    auto* bundle = wxGetApp().preset_bundle;
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};
    if (!bundle)
        return {{"success", false}, {"message", "PresetBundle not available"}};

    const int before_count = static_cast<int>(bundle->filament_presets.size());
    if (before_count <= 1) {
        return {{"success", false},
                {"message", "At least one filament must remain"},
                {"before_count", before_count}};
    }

    int target_index = before_count - 1; // default delete last
    int parsed = -1;

    if (params.contains("filament_index") && parse_int_value(params["filament_index"], parsed)) {
        target_index = parsed;
    } else if (params.contains("filament_id") && parse_int_value(params["filament_id"], parsed)) {
        target_index = parsed - 1; // 1-based id
    }

    if (target_index < 0 || target_index >= before_count) {
        return {{"success", false},
                {"message", "filament_id/filament_index out of range"},
                {"before_count", before_count},
                {"target_index", target_index}};
    }

    plater->sidebar().delete_filament(static_cast<size_t>(target_index), -1);
    const int after_count = static_cast<int>(bundle->filament_presets.size());

    if (after_count >= before_count) {
        return {{"success", false},
                {"message", "Failed to delete filament"},
                {"before_count", before_count},
                {"after_count", after_count},
                {"target_index", target_index}};
    }

    json result;
    result["success"] = true;
    result["message"] = "Filament deleted";
    result["before_count"] = before_count;
    result["after_count"] = after_count;
    result["deleted_filament_index"] = target_index;
    result["deleted_filament_id"] = target_index + 1;
    result["filament_presets"] = bundle->filament_presets;
    return result;
}

json SlicerBridge::DoSetFilamentType(const json& params)
{
    auto* plater = wxGetApp().plater();
    auto* bundle = wxGetApp().preset_bundle;
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};
    if (!bundle)
        return {{"success", false}, {"message", "PresetBundle not available"}};

    const int filament_count = static_cast<int>(bundle->filament_presets.size());
    if (filament_count <= 0)
        return {{"success", false}, {"message", "No filament slot available"}};

    int target_index = 0;
    int parsed = -1;
    if (params.contains("filament_index") && parse_int_value(params["filament_index"], parsed)) {
        target_index = parsed;
    } else if (params.contains("filament_id") && parse_int_value(params["filament_id"], parsed)) {
        target_index = parsed - 1;
    }

    if (target_index < 0 || target_index >= filament_count) {
        return {{"success", false},
                {"message", "filament_id/filament_index out of range"},
                {"filament_count", filament_count},
                {"target_index", target_index}};
    }

    std::string target_name = trim_copy(params.value("name", ""));
    if (target_name.empty())
        target_name = trim_copy(params.value("preset_name", ""));

    if (target_name.empty() && params.contains("filament_type")) {
        target_name = trim_copy(params.value("filament_type", ""));
        if (!target_name.empty()) {
            const std::string resolved = bundle->get_preset_name_by_alias(Preset::TYPE_FILAMENT, target_name);
            if (!resolved.empty())
                target_name = resolved;
        }
    }

    if (target_name.empty()) {
        return {{"success", false},
                {"message", "name/preset_name/filament_type is required"}};
    }

    const Preset* preset = bundle->filaments.find_preset(target_name, false);
    if (!preset) {
        return {{"success", false},
                {"message", "Filament preset not found: " + target_name}};
    }

    const std::string old_name = (target_index >= 0 && target_index < filament_count)
        ? bundle->filament_presets[target_index]
        : std::string();

    bundle->set_filament_preset(static_cast<size_t>(target_index), target_name);
    plater->update_project_dirty_from_presets();
    if (wxGetApp().app_config)
        bundle->export_selections(*wxGetApp().app_config);
    plater->sidebar().update_dynamic_filament_list();
    plater->on_config_change(bundle->full_config());

    json result;
    result["success"] = true;
    result["message"] = "Filament type updated";
    result["filament_index"] = target_index;
    result["filament_id"] = target_index + 1;
    result["old_preset"] = old_name;
    result["new_preset"] = bundle->filament_presets[target_index];
    result["filament_presets"] = bundle->filament_presets;
    return result;
}

json SlicerBridge::DoAutoMapFilaments(const json& params)
{
    const bool apply = parse_bool_param(params, "apply", true);
    const bool require_complete = parse_bool_param(params, "require_complete", true);
    const bool open_mapping_panel_on_conflict = parse_bool_param(params, "open_mapping_panel_on_conflict", true);
    const bool allow_type_mismatch = parse_bool_param(params, "allow_type_mismatch", false);
    const std::string requested_strategy = parse_string_param(params, "strategy", "type_then_color");
    const std::string strategy = normalized_strategy(requested_strategy);

    const DM::Device& current_device = DM::DataCenter::Ins().get_current_device_data();
    std::string mode = "unknown";
    int type_mismatch_count = 0;
    json warnings = json::array();
    json items = json::array();

    if (requested_strategy != strategy) {
        json warning;
        warning["code"] = "UNSUPPORTED_FILAMENT_MAPPING_STRATEGY";
        warning["message"] = "Unsupported filament mapping strategy. Falling back to type_then_color.";
        warning["requested_strategy"] = requested_strategy;
        warning["strategy"] = strategy;
        warnings.push_back(std::move(warning));
    }

    BOOST_LOG_TRIVIAL(warning)
        << "[SlicerBridge] auto_map_filaments start apply="
        << apply
        << " require_complete="
        << require_complete
        << " open_mapping_panel_on_conflict="
        << open_mapping_panel_on_conflict
        << " allow_type_mismatch="
        << allow_type_mismatch
        << " strategy="
        << strategy
        << " device_valid="
        << current_device.valid
        << " device="
        << current_device.name;

    auto build_result = [&]() -> json {
        json result;
        result["success"] = false;
        result["source_action"] = ActionID::AUTO_MAP_FILAMENTS;
        result["applied"] = false;
        result["mapping_panel_opened"] = false;
        result["items"] = items;
        result["invalidates"] = json::array();
        result["device"] = device_summary_json(current_device, mode);
        result["mode"] = mode;
        result["strategy"] = strategy;
        result["allow_type_mismatch"] = allow_type_mismatch;
        result["warnings"] = warnings;
        attach_mapping_counts(
            result,
            items,
            count_device_materials_for_mode(current_device, mode),
            type_mismatch_count);
        return result;
    };

    auto finish = [&](json result) -> json {
        if (!result.contains("source_action"))
            result["source_action"] = ActionID::AUTO_MAP_FILAMENTS;
        if (!result.contains("applied"))
            result["applied"] = false;
        if (!result.contains("mapping_panel_opened"))
            result["mapping_panel_opened"] = false;
        if (!result.contains("invalidates"))
            result["invalidates"] = json::array();
        if (!result.contains("items"))
            result["items"] = items;

        result["device"] = device_summary_json(current_device, mode);
        result["mode"] = mode;
        result["strategy"] = strategy;
        result["allow_type_mismatch"] = allow_type_mismatch;
        result["warnings"] = warnings;
        attach_mapping_counts(
            result,
            result.value("items", json::array()),
            count_device_materials_for_mode(current_device, mode),
            type_mismatch_count);
        log_auto_map_result(result);
        return result;
    };

    auto* plater = wxGetApp().plater();
    if (!plater) {
        json result = build_result();
        result["code"] = "FILAMENT_PANEL_NOT_AVAILABLE";
        result["message"] = "Plater is not available for filament mapping.";
        return finish(std::move(result));
    }
    if (!current_device.valid) {
        json result = build_result();
        result["code"] = "DEVICE_NOT_AVAILABLE";
        result["message"] = "Current printer device is not available.";
        maybe_open_filament_mapping_panel(open_mapping_panel_on_conflict, result);
        return finish(std::move(result));
    }

    const FilamentMappingService::Mode resolved_mode =
        FilamentMappingService::resolve_auto_mapping_mode(current_device);
    mode = FilamentMappingService::mode_to_string(resolved_mode);

    if (!device_has_usable_materials(current_device) ||
        !FilamentMappingService::device_has_available_materials(current_device, resolved_mode)) {
        json result = build_result();
        result["code"] = "DEVICE_MATERIALS_NOT_AVAILABLE";
        result["message"] = "Current printer device has no usable material data.";
        maybe_open_filament_mapping_panel(open_mapping_panel_on_conflict, result);
        return finish(std::move(result));
    }

    json source_items = json::array();

    try {
        plater->capture_scene_filament_source_snapshot_if_needed();
        source_items = FilamentMappingService::filter_source_items_to_plate(
            plater->get_scene_filament_source_snapshot(),
            plater->get_partplate_list().get_curr_plate_index());
        items = FilamentMappingService::auto_match(source_items, current_device, resolved_mode);
    } catch (const std::exception& ex) {
        json result = build_result();
        result["code"] = "FILAMENT_MAPPING_EXCEPTION";
        result["message"] = std::string("Automatic filament mapping failed: ") + ex.what();
        maybe_open_filament_mapping_panel(open_mapping_panel_on_conflict, result);
        return finish(std::move(result));
    } catch (...) {
        json result = build_result();
        result["code"] = "FILAMENT_MAPPING_EXCEPTION";
        result["message"] = "Automatic filament mapping failed with an unknown error.";
        maybe_open_filament_mapping_panel(open_mapping_panel_on_conflict, result);
        return finish(std::move(result));
    }

    json result = build_result();
    result["success"] = true;
    result["items"] = items;
    attach_mapping_counts(
        result,
        items,
        count_device_materials_for_mode(current_device, mode),
        type_mismatch_count);

    const int total_count = result.value("total_count", 0);
    const int unmapped_count = result.value("unmapped_count", 0);
    const bool mapping_complete = result.value("mapping_complete", false);

    if (total_count <= 0) {
        result["success"] = false;
        result["code"] = "FILAMENT_MAPPING_NO_ITEMS";
        result["message"] = "No project filament items are available for mapping.";
        maybe_open_filament_mapping_panel(open_mapping_panel_on_conflict, result);
        return finish(std::move(result));
    }

    if (!mapping_complete && require_complete) {
        result["success"] = false;
        result["code"] = "FILAMENT_MAPPING_INCOMPLETE";
        result["message"] = "Automatic filament mapping is incomplete. Please review the mapping panel.";
        maybe_open_filament_mapping_panel(open_mapping_panel_on_conflict, result);
        return finish(std::move(result));
    }

    if (apply) {
        bool applied = false;
        try {
            applied = FilamentMappingService::apply_mapping_to_scene(items, current_device, resolved_mode);
        } catch (...) {
            applied = false;
        }

        if (!applied) {
            result["success"] = false;
            result["code"] = "FILAMENT_MAPPING_APPLY_FAILED";
            result["message"] = "Failed to apply filament mapping to the scene.";
            maybe_open_filament_mapping_panel(open_mapping_panel_on_conflict, result);
            return finish(std::move(result));
        }

        result["applied"] = true;
        result["invalidates"] = json::array({
            "scene.filament_mapping.valid",
            "plate.current.slice_ready_for_print",
            "plate.current.gcode_available"
        });
    }

    if (!apply) {
        result["message"] = mapping_complete
            ? "Filament mapping preview completed. Scene was not modified."
            : "Filament mapping preview completed with partial matches. Scene was not modified.";
    } else {
        result["message"] = mapping_complete
            ? "Filament mapping completed."
            : "Filament mapping completed with partial matches.";
    }

    if (!mapping_complete && unmapped_count > 0) {
        json warning;
        warning["code"] = "FILAMENT_MAPPING_PARTIAL";
        warning["message"] = "Some project filament items are still unmapped.";
        warning["unmapped_count"] = unmapped_count;
        warnings.push_back(std::move(warning));
        result["warnings"] = warnings;
    }

    return finish(std::move(result));
}
} // namespace Bridge
} // namespace GUI
} // namespace Slic3r
