#include "SceneFilamentSourceSnapshotManager.hpp"

#include "../../GUI_App.hpp"
#include "libslic3r/Config.hpp"
#include "libslic3r/Preset.hpp"
#include "libslic3r/PresetBundle.hpp"

#include <sstream>

namespace Slic3r {
namespace GUI {

void SceneFilamentSourceSnapshotManager::reset()
{
    m_initialized = false;
    m_items = nlohmann::json::array();
    m_scene_fingerprint.clear();
}

bool SceneFilamentSourceSnapshotManager::initialized() const
{
    return is_easy_mode_active() && m_initialized;
}

bool SceneFilamentSourceSnapshotManager::capture_from_current_config_if_needed()
{
    if (m_initialized)
        return true;

    nlohmann::json source_items = nlohmann::json::array();
    std::string    fingerprint;
    if (!build_items_from_current_config(source_items, fingerprint))
        return false;

    m_items = std::move(source_items);
    m_scene_fingerprint = std::move(fingerprint);
    m_initialized = true;
    return true;
}

bool SceneFilamentSourceSnapshotManager::capture_from_loaded_3mf_config(const DynamicConfig& loaded_config)
{
    if (!is_easy_mode_active())
        return false;

    nlohmann::json source_items = nlohmann::json::array();
    std::string    fingerprint;
    if (!build_items_from_loaded_3mf_config(loaded_config, source_items, fingerprint))
        return false;

    m_items = std::move(source_items);
    m_scene_fingerprint = std::move(fingerprint);
    m_initialized = true;
    return true;
}

nlohmann::json SceneFilamentSourceSnapshotManager::export_items() const
{
    if (!m_initialized)
        return nlohmann::json::array();
    return m_items;
}

bool SceneFilamentSourceSnapshotManager::is_easy_mode_active() const
{
    return wxGetApp().easy_mode();
}

bool SceneFilamentSourceSnapshotManager::build_items_from_current_config(
    nlohmann::json& out_items,
    std::string& out_fingerprint) const
{
    auto* bundle = wxGetApp().preset_bundle;
    if (bundle == nullptr)
        return false;

    const auto* colors_opt = bundle->project_config.option<ConfigOptionStrings>("filament_colour");
    if (colors_opt == nullptr || colors_opt->values.empty())
        return false;

    out_items = nlohmann::json::array();
    std::ostringstream fingerprint;

    const auto& filament_presets = bundle->filament_presets;
    const size_t item_count = colors_opt->values.size();
    for (size_t i = 0; i < item_count; ++i) {
        const int item_index = static_cast<int>(i);
        const std::string source_color = colors_opt->values[i];
        const std::string source_filament_preset =
            i < filament_presets.size() ? filament_presets[i] : std::string();

        std::string preset_display = source_filament_preset;
        std::string filament_type;
        if (!source_filament_preset.empty()) {
            if (auto* preset = bundle->filaments.find_preset(source_filament_preset)) {
                preset_display = preset->name.empty() ? source_filament_preset : preset->name;
                preset->get_filament_type(filament_type);
            }
        }

        nlohmann::json source_item;
        source_item["item_index"] = item_index;
        source_item["extruderId"] = item_index + 1;
        source_item["sourceColor"] = source_color;
        source_item["sourceFilamentPreset"] = source_filament_preset;
        source_item["presetDisplay"] = preset_display;
        source_item["extruderFilamentType"] = filament_type;
        out_items.push_back(std::move(source_item));

        fingerprint << item_index << '|'
                    << source_color << '|'
                    << source_filament_preset << '|'
                    << filament_type << '|'
                    << preset_display << ';';
    }

    if (out_items.empty())
        return false;

    out_fingerprint = fingerprint.str();
    return true;
}

bool SceneFilamentSourceSnapshotManager::build_items_from_loaded_3mf_config(
    const DynamicConfig& loaded_config,
    nlohmann::json& out_items,
    std::string& out_fingerprint) const
{
    const auto* colors_opt = loaded_config.option<ConfigOptionStrings>("filament_colour");
    if (colors_opt == nullptr || colors_opt->values.empty())
        return false;

    const auto* preset_ids_opt = loaded_config.option<ConfigOptionStrings>("filament_settings_id");
    const auto* filament_types_opt = loaded_config.option<ConfigOptionStrings>("filament_type");

    out_items = nlohmann::json::array();
    std::ostringstream fingerprint;

    const size_t item_count = colors_opt->values.size();
    for (size_t i = 0; i < item_count; ++i) {
        const int item_index = static_cast<int>(i);
        const std::string source_color = colors_opt->values[i];
        const std::string source_filament_preset =
            (preset_ids_opt != nullptr && i < preset_ids_opt->values.size()) ? preset_ids_opt->values[i] : std::string();
        const std::string filament_type =
            (filament_types_opt != nullptr && i < filament_types_opt->values.size()) ? filament_types_opt->values[i] : std::string();

        nlohmann::json source_item;
        source_item["item_index"] = item_index;
        source_item["extruderId"] = item_index + 1;
        source_item["sourceColor"] = source_color;
        source_item["sourceFilamentPreset"] = source_filament_preset;
        source_item["presetDisplay"] = source_filament_preset;
        source_item["extruderFilamentType"] = filament_type;
        out_items.push_back(std::move(source_item));

        fingerprint << item_index << '|'
                    << source_color << '|'
                    << source_filament_preset << '|'
                    << filament_type << '|'
                    << source_filament_preset << ';';
    }

    if (out_items.empty())
        return false;

    out_fingerprint = fingerprint.str();
    return true;
}

} // namespace GUI
} // namespace Slic3r
