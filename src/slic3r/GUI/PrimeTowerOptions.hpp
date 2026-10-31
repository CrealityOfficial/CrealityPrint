#ifndef slic3r_GUI_PrimeTowerOptions_hpp_
#define slic3r_GUI_PrimeTowerOptions_hpp_

#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/Preset.hpp"

namespace Slic3r::GUI {

inline void apply_no_wipe_tower_no_purge_linked_options(DynamicPrintConfig& config)
{
    if (config.has("sparse_infill_density"))
        config.set_key_value("sparse_infill_density", new ConfigOptionPercent(10));
    if (config.has("ai_infill"))
        config.set_key_value("ai_infill", new ConfigOptionBool(false));
    if (config.has("sparse_infill_pattern"))
        config.set_key_value("sparse_infill_pattern", new ConfigOptionEnum<InfillPattern>(ipLockedZag));
    if (config.has("skin_infill_line_width"))
        config.set_key_value("skin_infill_line_width", new ConfigOptionFloatsOrPercentsNullable{{100, true}});
    if (config.has("flush_into_solid_skeleton"))
        config.set_key_value("flush_into_solid_skeleton", new ConfigOptionBool(false));
    // Smooth timelapse uses a camera tower while material purge may stay on
    // model infill, objects or support.
    const bool smooth_timelapse = config.has("timelapse_type") &&
        config.opt_enum<TimelapseType>("timelapse_type") == tlSmooth;
    if (!smooth_timelapse) {
        if (config.has("flush_into_infill"))
            config.set_key_value("flush_into_infill", new ConfigOptionBool(false));
        if (config.has("flush_into_objects"))
            config.set_key_value("flush_into_objects", new ConfigOptionBool(false));
        if (config.has("flush_into_support"))
            config.set_key_value("flush_into_support", new ConfigOptionBool(false));
    }
    if (config.has("flush_into_skeleton"))
        config.set_key_value("flush_into_skeleton", new ConfigOptionBool(false));
}

inline void apply_no_wipe_tower_no_purge_default_options(DynamicPrintConfig& config)
{
    if (config.has("top_bottom_infill_wall_overlap"))
        config.set_key_value("top_bottom_infill_wall_overlap", new ConfigOptionPercent(25));
    if (config.has("infill_anchor"))
        config.set_key_value("infill_anchor", new ConfigOptionFloatOrPercent(400, true));
    if (config.has("infill_anchor_max"))
        config.set_key_value("infill_anchor_max", new ConfigOptionFloatOrPercent(20, false));
    if (config.has("infill_wall_overlap"))
        config.set_key_value("infill_wall_overlap", new ConfigOptionPercent(15));
    if (config.has("external_infill_margin"))
        config.set_key_value("external_infill_margin", new ConfigOptionFloatOrPercent(0, false));
    if (config.has("internal_solid_infill_pattern"))
        config.set_key_value("internal_solid_infill_pattern", new ConfigOptionEnum<InfillPattern>(ipMonotonic));
    if (config.has("gap_fill_target"))
        config.set_key_value("gap_fill_target", new ConfigOptionEnum<GapFillTarget>(gftEverywhere));
    if (config.has("filter_out_gap_fill"))
        config.set_key_value("filter_out_gap_fill", new ConfigOptionFloat(0.0));
    if (config.has("infill_lock_depth"))
        config.set_key_value("infill_lock_depth", new ConfigOptionFloat(0.0));
    if (config.has("locked_skin_infill_pattern"))
        config.set_key_value("locked_skin_infill_pattern", new ConfigOptionEnum<InfillPattern>(ipConcentric));
    if (config.has("locked_skeleton_infill_pattern"))
        config.set_key_value("locked_skeleton_infill_pattern", new ConfigOptionEnum<InfillPattern>(ipGrid));
    if (config.has("skin_infill_density"))
        config.set_key_value("skin_infill_density", new ConfigOptionPercent(100));
    if (config.has("minimum_skeleton_infill_density"))
        config.set_key_value("minimum_skeleton_infill_density", new ConfigOptionPercent(15));
    if (config.has("skin_infill_depth"))
        config.set_key_value("skin_infill_depth", new ConfigOptionFloat(0.8));
}

// Only these process options participate in no-tower mode switching.
inline const std::vector<const char*>& no_wipe_tower_linked_process_keys()
{
    static const std::vector<const char*> keys = {
        "sparse_infill_density",
        "ai_infill",
        "sparse_infill_pattern",
        "skin_infill_line_width",
        "flush_into_solid_skeleton",
        "flush_into_infill",
        "flush_into_objects",
        "flush_into_support",
        "flush_into_skeleton",
        "top_bottom_infill_wall_overlap",
        "infill_anchor",
        "infill_anchor_max",
        "infill_wall_overlap",
        "external_infill_margin",
        "internal_solid_infill_pattern",
        "gap_fill_target",
        "filter_out_gap_fill",
        "infill_lock_depth",
        "locked_skin_infill_pattern",
        "locked_skeleton_infill_pattern",
        "skin_infill_density",
        "minimum_skeleton_infill_density",
        "skin_infill_depth",
    };
    return keys;
}

// Read installed presets directly: find_preset() may return the edited project
// config for the selected name, even when that name belongs to a system preset.
inline const DynamicPrintConfig& prime_tower_system_config(const PresetCollection& presets)
{
    const Preset* current = &presets.get_edited_preset();
    std::vector<std::string> visited;
    while (current) {
        if (std::find(visited.begin(), visited.end(), current->name) != visited.end())
            break;
        visited.push_back(current->name);
        const Preset* installed = nullptr;
        for (const auto& preset : presets.get_presets())
            if (preset.is_system && !preset.is_project_embedded && !preset.is_external &&
                (preset.name == current->name ||
                 std::find(preset.renamed_from.begin(), preset.renamed_from.end(), current->name) != preset.renamed_from.end())) {
                installed = &preset;
                break;
            }
        if (installed)
            return installed->config;

        const std::string parent = current->inherits();
        current = nullptr;
        if (parent.empty())
            break;
        for (const auto& preset : presets.get_presets())
            if (preset.name == parent ||
                std::find(preset.renamed_from.begin(), preset.renamed_from.end(), parent) != preset.renamed_from.end()) {
                current = &preset;
                break;
            }
    }
    // A foreign project may have no installed system ancestor. Use built-in
    // defaults in that case, never the imported project or a user preset.
    return presets.default_preset().config;
}

// Only call for an explicit field edit, before writing the requested value.
// Editing a preset must work before models/material assignments exist.
// Enabling restores only linked options from the installed system process.
inline bool apply_prime_tower_user_change(DynamicPrintConfig& config, bool enabled, bool link_no_tower_options,
                                         const DynamicPrintConfig* system_process = nullptr)
{
    const auto* current = config.option<ConfigOptionBool>("enable_prime_tower");
    if (!current || current->value == enabled)
        return false;

    const bool smooth_timelapse = config.has("timelapse_type") &&
        config.opt_enum<TimelapseType>("timelapse_type") == tlSmooth;
    if (enabled && !smooth_timelapse && link_no_tower_options && system_process)
        for (const auto* key : no_wipe_tower_linked_process_keys())
            if (const auto* value = system_process->option(key))
                if (config.has(key))
                    config.set_key_value(key, value->clone());
    config.set_key_value("enable_prime_tower", new ConfigOptionBool(enabled));
    if (!enabled && !smooth_timelapse && link_no_tower_options) {
        apply_no_wipe_tower_no_purge_linked_options(config);
        apply_no_wipe_tower_no_purge_default_options(config);
    }
    return true;
}

// Purge-in-tower belongs to the printer preset, so its system baseline must
// come from the printer collection rather than the process or imported project.
inline void apply_prime_tower_printer_user_change(DynamicPrintConfig& printer, bool enabled,
                                                 bool link_no_tower_options, bool smooth_timelapse,
                                                 const DynamicPrintConfig& system_printer)
{
    if (!link_no_tower_options || smooth_timelapse || !printer.has("purge_in_prime_tower"))
        return;
    if (!enabled)
        printer.set_key_value("purge_in_prime_tower", new ConfigOptionBool(false));
    else if (const auto* value = system_printer.option<ConfigOptionBool>("purge_in_prime_tower"))
        printer.set_key_value("purge_in_prime_tower", value->clone());
}

// Restore only values still owned by a mode toggle, preserving later user edits.
class LinkedOptionBackup
{
public:
    void capture(const DynamicPrintConfig& before, const DynamicPrintConfig& after)
    {
        clear();
        for (const auto& key : before.diff(after)) {
            const ConfigOption* old_value = before.option(key);
            const ConfigOption* new_value = after.option(key);
            if (old_value && new_value) {
                m_before.set_key_value(key, old_value->clone());
                m_after.set_key_value(key, new_value->clone());
            }
        }
    }

    void restore(DynamicPrintConfig& config)
    {
        for (const auto& key : m_before.keys()) {
            const ConfigOption* current = config.option(key);
            if (current && *current == *m_after.option(key))
                config.set_key_value(key, m_before.option(key)->clone());
        }
        clear();
    }

    void clear()
    {
        m_before.clear();
        m_after.clear();
    }

private:
    DynamicPrintConfig m_before;
    DynamicPrintConfig m_after;
};

} // namespace Slic3r::GUI

#endif
