#include "AIProcessSwitchService.hpp"

#include "../bridge/SlicerBridge.hpp"
#include "../../GUI_App.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/PrintConfig.hpp"

#include <algorithm>
#include <cctype>

namespace Slic3r {
namespace GUI {

using json = nlohmann::json;

namespace {

std::string get_string_value(const json& value, const char* key)
{
    if (!value.is_object() || !value.contains(key) || !value[key].is_string())
        return {};
    return value[key].get<std::string>();
}

std::string normalize_scope_text(const std::string& value)
{
    std::string out;
    out.reserve(value.size());

    bool previous_space = true;
    for (unsigned char ch : value) {
        if (std::isalnum(ch) != 0) {
            out.push_back(static_cast<char>(std::tolower(ch)));
            previous_space = false;
            continue;
        }

        if (!previous_space) {
            out.push_back(' ');
            previous_space = true;
        }
    }

    while (!out.empty() && out.back() == ' ')
        out.pop_back();
    return out;
}

void add_scope_marker(
    std::vector<std::string>& markers,
    const std::string& raw,
    bool add_without_leading_token = true)
{
    const std::string normalized = normalize_scope_text(raw);
    if (normalized.empty())
        return;

    if (std::find(markers.begin(), markers.end(), normalized) == markers.end())
        markers.push_back(normalized);

    if (!add_without_leading_token)
        return;

    const std::size_t first_space = normalized.find(' ');
    if (first_space == std::string::npos)
        return;

    const std::string without_brand = normalized.substr(first_space + 1);
    if (!without_brand.empty()
        && std::find(markers.begin(), markers.end(), without_brand) == markers.end()) {
        markers.push_back(without_brand);
    }
}

std::string extract_machine_scope(const std::string& preset_name)
{
    const std::size_t at_pos = preset_name.find('@');
    if (at_pos == std::string::npos || at_pos + 1 >= preset_name.size())
        return {};
    return normalize_scope_text(preset_name.substr(at_pos + 1));
}

bool contains_scope_marker(const std::string& normalized_text, const std::string& marker)
{
    return !marker.empty() && normalized_text.find(marker) != std::string::npos;
}

std::vector<std::string> build_full_scope_markers(const AIProcessResolveContext& context)
{
    std::vector<std::string> markers;
    add_scope_marker(markers, context.printer_preset_name);
    add_scope_marker(markers, extract_machine_scope(context.current_print_preset_name));
    add_scope_marker(markers, extract_machine_scope(context.default_print_preset_name));
    return markers;
}

std::vector<std::string> build_model_scope_markers(const AIProcessResolveContext& context)
{
    std::vector<std::string> markers;
    add_scope_marker(markers, context.printer_model);
    return markers;
}

std::vector<std::string> build_nozzle_scope_markers(const AIProcessResolveContext& context)
{
    std::vector<std::string> markers;
    if (context.nozzle_diameter.empty())
        return markers;

    add_scope_marker(markers, context.nozzle_diameter, false);
    add_scope_marker(markers, context.nozzle_diameter + " nozzle", false);
    return markers;
}

bool matches_current_printer_scope(
    const AIProcessResolveContext& context,
    const std::string& preset_name)
{
    const std::string candidate_scope = extract_machine_scope(preset_name);
    if (candidate_scope.empty())
        return true;

    const std::vector<std::string> full_scope_markers = build_full_scope_markers(context);
    for (const std::string& marker : full_scope_markers) {
        if (contains_scope_marker(candidate_scope, marker) || contains_scope_marker(marker, candidate_scope))
            return true;
    }

    const std::vector<std::string> model_scope_markers = build_model_scope_markers(context);
    bool matches_model = false;
    for (const std::string& marker : model_scope_markers) {
        if (contains_scope_marker(candidate_scope, marker)) {
            matches_model = true;
            break;
        }
    }

    if (!matches_model)
        return false;

    const std::vector<std::string> nozzle_scope_markers = build_nozzle_scope_markers(context);
    if (nozzle_scope_markers.empty() || candidate_scope.find("nozzle") == std::string::npos)
        return true;

    for (const std::string& marker : nozzle_scope_markers) {
        if (contains_scope_marker(candidate_scope, marker))
            return true;
    }

    return false;
}

} // namespace

json AIProcessSwitchService::get_slicer_state() const
{
    return Bridge::SlicerBridge::Instance().Execute(Bridge::ActionID::GET_SLICER_STATE, json::object());
}

json AIProcessSwitchService::get_presets() const
{
    return Bridge::SlicerBridge::Instance().Execute(Bridge::ActionID::GET_PRESETS, {{"type", "print"}});
}

std::string AIProcessSwitchService::resolve_default_print_preset_name(
    const json& slicer_state,
    const json& presets_result) const
{
    if (wxGetApp().preset_bundle != nullptr) {
        const auto& printer_preset = wxGetApp().preset_bundle->printers.get_edited_preset();
        if (const ConfigOption* option = printer_preset.config.option("default_print_profile")) {
            const std::string value = option->serialize();
            if (!value.empty())
                return value;
        }
    }

    const json state = slicer_state.value("state", json::object());
    std::string current = get_string_value(state, "current_print_preset");
    if (!current.empty())
        return current;

    current = presets_result.value("current_print", std::string());
    return current;
}

std::vector<std::string> AIProcessSwitchService::collect_print_preset_names(
    const AIProcessResolveContext& context,
    const json& presets_result) const
{
    std::vector<std::string> filtered_names;
    std::vector<std::string> fallback_names;
    const json print_presets = presets_result.value("print_presets", json::array());
    if (!print_presets.is_array())
        return filtered_names;

    filtered_names.reserve(print_presets.size());
    fallback_names.reserve(print_presets.size());
    for (const auto& item : print_presets) {
        if (item.is_object() && item.contains("is_visible") && item["is_visible"].is_boolean()
            && !item["is_visible"].get<bool>()) {
            continue;
        }

        const std::string name = get_string_value(item, "name");
        if (!name.empty())
            fallback_names.push_back(name);

        if (!name.empty() && matches_current_printer_scope(context, name))
            filtered_names.push_back(name);
    }

    return filtered_names.empty() ? fallback_names : filtered_names;
}

AIProcessResolveContext AIProcessSwitchService::BuildContext() const
{
    const json slicer_state = get_slicer_state();
    const json presets_result = get_presets();

    AIProcessResolveContext context;
    if (!slicer_state.value("success", false) || !presets_result.value("success", false))
        return context;

    const json state = slicer_state.value("state", json::object());
    context.printer_preset_name = get_string_value(state, "current_printer_preset");
    context.printer_model = get_string_value(state, "printer_model");
    context.nozzle_diameter = get_string_value(state, "nozzle_diameter");
    context.current_print_preset_name = get_string_value(state, "current_print_preset");
    context.default_print_preset_name = resolve_default_print_preset_name(slicer_state, presets_result);
    context.available_print_preset_names = collect_print_preset_names(context, presets_result);
    return context;
}

AIProcessApplyResult AIProcessSwitchService::apply_print_preset(
    AIProcessIntent intent,
    const AIProcessIntentResolution& resolution) const
{
    AIProcessApplyResult result;
    result.intent = intent;
    result.resolution = resolution;

    if (!resolution.success) {
        result.code = "PROCESS_RESOLUTION_FAILED";
        result.message = resolution.summary_text.empty() ? "Failed to resolve target process preset." : resolution.summary_text;
        return result;
    }

    if (!resolution.requires_change) {
        result.success = true;
        result.changed = false;
        result.reslice_expected = false;
        result.message = resolution.summary_text.empty() ? "Process preset remains unchanged." : resolution.summary_text;
        return result;
    }

    const json bridge_result = Bridge::SlicerBridge::Instance().Execute(
        Bridge::ActionID::SELECT_PRESET,
        {
            {"type", "print"},
            {"name", resolution.resolved_preset_name}
        });

    result.bridge_result = bridge_result;
    if (!bridge_result.value("success", false)) {
        result.code = "PROCESS_SWITCH_FAILED";
        result.message = bridge_result.value("message", std::string("Failed to apply print/process preset."));
        return result;
    }

    result.success = true;
    result.changed = true;
    result.reslice_expected = true;
    result.message = resolution.summary_text.empty() ? "Process preset applied." : resolution.summary_text;
    return result;
}

AIProcessApplyResult AIProcessSwitchService::ApplyIntent(AIProcessIntent intent) const
{
    AIProcessApplyResult result;
    result.intent = intent;

    const json slicer_state = get_slicer_state();
    if (!slicer_state.value("success", false)) {
        result.code = "PROCESS_STATE_UNAVAILABLE";
        result.message = slicer_state.value("message", std::string("Failed to collect slicer state."));
        return result;
    }

    const json presets_result = get_presets();
    if (!presets_result.value("success", false)) {
        result.code = "PROCESS_PRESETS_UNAVAILABLE";
        result.message = presets_result.value("message", std::string("Failed to collect print presets."));
        return result;
    }

    AIProcessResolveContext context;
    const json state = slicer_state.value("state", json::object());
    context.printer_preset_name = get_string_value(state, "current_printer_preset");
    context.printer_model = get_string_value(state, "printer_model");
    context.nozzle_diameter = get_string_value(state, "nozzle_diameter");
    context.current_print_preset_name = get_string_value(state, "current_print_preset");
    context.default_print_preset_name = resolve_default_print_preset_name(slicer_state, presets_result);
    context.available_print_preset_names = collect_print_preset_names(context, presets_result);

    result.resolution = AIProcessPresetIntentResolver::Resolve(context, intent);
    return apply_print_preset(intent, result.resolution);
}

} // namespace GUI
} // namespace Slic3r
