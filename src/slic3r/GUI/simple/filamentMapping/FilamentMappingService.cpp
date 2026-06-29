// FilamentMappingService contains the core consumable mapping rules without any
// ImGui or view state.  The service treats the original scene/source filament
// snapshot as the left side of the mapping and the current device material
// slots as the right side.  It is responsible for building mapping JSON for the
// AI card, applying automatic/manual selections, and committing mapped presets,
// colors, and flushing-volume recalculation back to the slicer project.

#include "FilamentMappingService.hpp"
#include "match_color.hpp"

#include "../../GUI_App.hpp"
#include "../../PartPlate.hpp"
#include "../../Plater.hpp"
#include "libslic3r/Config.hpp"
#include "libslic3r/Preset.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/MixedFilament.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <iterator>
#include <limits>
#include <sstream>
#include <unordered_set>
#include <vector>

namespace Slic3r {
namespace GUI {

using json = nlohmann::json;

namespace {

struct DeviceMaterialSlot
{
    int         box_type = -1;
    int         box_id = -1;
    int         material_id = -1;
    std::string selection_token;
    std::string slot_label;
    std::string color_hex;
    std::string material_type;
    std::string material_name;
    bool        available = false;
};

static bool is_cfs_box_type(int box_type)
{
    return box_type == 0 || box_type == 2;
}

static bool is_supported_box_type(int box_type)
{
    return box_type == 0 || box_type == 1 || box_type == 2;
}

static FilamentMappingService::MappingChannel channel_from_box_type(int box_type)
{
    if (is_cfs_box_type(box_type))
        return FilamentMappingService::MappingChannel::CFS;
    if (box_type == 1)
        return FilamentMappingService::MappingChannel::External;
    return FilamentMappingService::MappingChannel::None;
}

static bool box_matches_mode(FilamentMappingService::Mode mode, int box_type)
{
    if (mode == FilamentMappingService::Mode::All)
        return is_supported_box_type(box_type);
    return mode == FilamentMappingService::Mode::External ? box_type == 1 : is_cfs_box_type(box_type);
}

static bool should_include_material_option(int box_type, int material_id)
{
    if (box_type == 0 && (material_id < 0 || material_id > 3))
        return false;
    return true;
}

static std::string clamp_hex(std::string value)
{
    if (value.empty())
        return "#000000";
    if (value.front() != '#')
        value.insert(value.begin(), '#');
    return value;
}

static std::string normalize_hex(std::string value)
{
    value = clamp_hex(std::move(value));
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return value;
}

static std::string build_selection_token(int box_type, int box_id, int material_id)
{
    return std::to_string(box_type) + ":" + std::to_string(box_id) + ":" + std::to_string(material_id);
}

static bool parse_selection_token(const std::string& token, int& box_type, int& box_id, int& material_id)
{
    box_type = -1;
    box_id = -1;
    material_id = -1;

    std::stringstream ss(token);
    std::string segment;
    std::vector<int> parts;
    while (std::getline(ss, segment, ':')) {
        if (segment.empty())
            return false;
        try {
            parts.push_back(std::stoi(segment));
        } catch (...) {
            return false;
        }
    }

    if (parts.size() != 3)
        return false;

    box_type = parts[0];
    box_id = parts[1];
    material_id = parts[2];
    return true;
}

static std::string slot_label_for(int box_type, int box_id, int material_id)
{
    if (box_type == 1 || box_type == 2)
        return "EXT";
    return std::to_string(box_id) + static_cast<char>('A' + (material_id % 26));
}

static std::string group_label_for(int box_type, int box_id)
{
    if (box_type == 1 || box_type == 2)
        return "EXT";
    return std::string("CFS ") + std::to_string(box_id);
}

static std::string material_match_key(const DM::Material& material)
{
    if (!material.type.empty())
        return material.type;
    if (!material.name.empty())
        return material.name;
    return {};
}

static std::string material_display_name(const DM::Material& material)
{
    const bool no_color = material.color.empty();
    if (material.state == -1 || (material.state == 0 && no_color))
        return "/";
    if (no_color)
        return "?";
    if (!material.name.empty())
        return material.name;
    if (!material.type.empty())
        return material.type;
    return "Filament";
}

static std::string trim_copy(std::string value)
{
    const auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

static bool is_placeholder_material_text(const std::string& value)
{
    const std::string normalized = trim_copy(value);
    return normalized.empty() || normalized == "?" || normalized == "/" || normalized == "\\";
}

static bool is_material_available_for_mapping(const DM::Material& material)
{
    if (material.state == -1 || material.color.empty())
        return false;

    return !is_placeholder_material_text(material_display_name(material));
}

static std::vector<DeviceMaterialSlot> collect_slots(
    const DM::Device& device,
    FilamentMappingService::Mode mode)
{
    std::vector<DeviceMaterialSlot> slots;
    if (!device.valid)
        return slots;

    for (const auto& box : device.materialBoxes) {
        if (!box_matches_mode(mode, box.box_type))
            continue;

        for (const auto& material : box.materials) {
            if (!should_include_material_option(box.box_type, material.material_id))
                continue;

            DeviceMaterialSlot slot;
            slot.box_type = box.box_type;
            slot.box_id = box.box_id;
            slot.material_id = material.material_id;
            slot.selection_token = build_selection_token(slot.box_type, slot.box_id, slot.material_id);
            slot.slot_label = slot_label_for(slot.box_type, slot.box_id, slot.material_id);
            slot.color_hex = normalize_hex(material.color);
            slot.material_type = material_match_key(material);
            slot.material_name = material_display_name(material);
            slot.available = is_material_available_for_mapping(material);
            slots.push_back(std::move(slot));
        }
    }

    return slots;
}

static const DeviceMaterialSlot* find_slot_by_token(
    const std::vector<DeviceMaterialSlot>& slots,
    const std::string& selection_token)
{
    const auto it = std::find_if(slots.begin(), slots.end(), [&](const DeviceMaterialSlot& slot) {
        return slot.selection_token == selection_token;
    });
    return it == slots.end() ? nullptr : &(*it);
}

static const DeviceMaterialSlot* find_slot_for_mapping_item(
    const std::vector<DeviceMaterialSlot>& slots,
    const json& item)
{
    if (!item.is_object())
        return nullptr;

    const std::string selection_token = item.value("selection_token", item.value("selectionToken", std::string()));
    if (!selection_token.empty()) {
        if (const DeviceMaterialSlot* slot = find_slot_by_token(slots, selection_token))
            return slot;
    }

    const int box_type = item.value("boxType", item.value("box_type", -1));
    const int box_id = item.value("boxId", item.value("box_id", -1));
    const int material_id = item.value("materialId", item.value("material_id", -1));
    if (box_type < 0 || box_id < 0 || material_id < 0)
        return nullptr;

    const auto it = std::find_if(slots.begin(), slots.end(), [&](const DeviceMaterialSlot& slot) {
        return slot.box_type == box_type &&
               slot.box_id == box_id &&
               slot.material_id == material_id;
    });
    return it == slots.end() ? nullptr : &(*it);
}

static std::string resolve_cid_for_slot(const DM::Device& device, const DeviceMaterialSlot& slot)
{
    for (const auto& box_color_info : device.boxColorInfos) {
        if (box_color_info.boxId == slot.box_id &&
            box_color_info.materialId == slot.material_id &&
            !box_color_info.cId.empty()) {
            return box_color_info.cId;
        }
    }
    return {};
}

static std::string resolve_item_filament_type(const json& item)
{
    if (!item.is_object())
        return {};

    const std::string extruder_filament_type = item.value("extruderFilamentType", std::string());
    if (!extruder_filament_type.empty())
        return extruder_filament_type;

    const std::string preset_display = item.value("presetDisplay", std::string());
    if (!preset_display.empty())
        return preset_display;

    return item.value("sourceFilamentPreset", std::string());
}

static FilamentMappingService::MappingChannel resolve_item_channel(const json& item)
{
    if (!item.is_object() || !item.value("mapped", false))
        return FilamentMappingService::MappingChannel::None;

    const int box_type = item.value("boxType", item.value("box_type", -1));
    FilamentMappingService::MappingChannel channel = channel_from_box_type(box_type);
    if (channel != FilamentMappingService::MappingChannel::None)
        return channel;

    int token_box_type = -1;
    int token_box_id = -1;
    int token_material_id = -1;
    if (parse_selection_token(item.value("selection_token", item.value("selectionToken", std::string())),
                              token_box_type,
                              token_box_id,
                              token_material_id)) {
        channel = channel_from_box_type(token_box_type);
        if (channel != FilamentMappingService::MappingChannel::None)
            return channel;
    }

    return FilamentMappingService::MappingChannel::None;
}

static FilamentMappingService::MappingChannel resolve_item_channel_with_slot(
    const json& item,
    const DeviceMaterialSlot* slot)
{
    if (slot != nullptr) {
        const FilamentMappingService::MappingChannel slot_channel = channel_from_box_type(slot->box_type);
        if (slot_channel != FilamentMappingService::MappingChannel::None)
            return slot_channel;
    }

    return resolve_item_channel(item);
}

static json slot_to_mapping_patch(const DeviceMaterialSlot& slot)
{
    return {
        {"mapped", true},
        {"slotLabel", slot.slot_label},
        {"matchColor", slot.color_hex},
        {"materialType", slot.material_type},
        {"materialName", slot.material_name},
        {"selection_token", slot.selection_token},
        {"boxType", slot.box_type},
        {"boxId", slot.box_id},
        {"materialId", slot.material_id},
        {"sharedSlot", false},
        {"autoReused", false},
        {"mappingWarning", std::string()}
    };
}

static void apply_slot_to_item(json& item, const DeviceMaterialSlot& slot)
{
    const json patch = slot_to_mapping_patch(slot);
    for (auto it = patch.begin(); it != patch.end(); ++it)
        item[it.key()] = it.value();
}

static std::unordered_set<int> collect_plate_item_indices_impl(int item_count, int preferred_plate_index)
{
    std::unordered_set<int> item_indices;
    if (item_count <= 0 || wxGetApp().plater() == nullptr)
        return item_indices;

    PartPlateList& plate_list = wxGetApp().plater()->get_partplate_list();
    const int plate_index = preferred_plate_index >= 0
        ? preferred_plate_index
        : plate_list.get_curr_plate_index();
    PartPlate* plate = plate_index >= 0 ? plate_list.get_plate(plate_index) : nullptr;
    if (plate == nullptr)
        return item_indices;

    auto* bundle = wxGetApp().preset_bundle;
    const size_t num_physical = bundle != nullptr ? bundle->filament_presets.size() : 0;

    for (const int extruder_id : plate->get_model_volume_extruders()) {
        if (extruder_id <= 0)
            continue;

        const auto component_ids = FilamentMappingService::resolve_physical_source_filament_ids(static_cast<unsigned int>(extruder_id), num_physical);
        if (!component_ids.empty()) {
            for (const unsigned int component_id : component_ids) {
                const int item_index = static_cast<int>(component_id) - 1;
                if (item_index < 0 || item_index >= item_count)
                    continue;
                item_indices.insert(item_index);
            }
            continue;
        }

        const int item_index = extruder_id - 1;
        if (item_index < 0 || item_index >= item_count)
            continue;
        item_indices.insert(item_index);
    }
    return item_indices;
}

static bool item_is_on_current_plate(const json& item, const std::unordered_set<int>& current_plate_indices)
{
    if (current_plate_indices.empty())
        return false;
    return current_plate_indices.count(item.value("item_index", -1)) != 0;
}

static std::string canonicalize_filament_type_token(std::string value)
{
    value = trim_copy(std::move(value));

    std::string normalized;
    normalized.reserve(value.size() * 2);
    for (unsigned char ch : value) {
        if (ch == '+') {
            normalized += "PLUS";
            continue;
        }
        if (!std::isalnum(ch))
            continue;
        normalized.push_back(static_cast<char>(std::toupper(ch)));
    }

    return normalized;
}

static bool preset_matches_filament_type_candidate(const Preset& preset, const std::string& candidate)
{
    const std::string normalized_candidate = canonicalize_filament_type_token(candidate);
    if (normalized_candidate.empty())
        return false;

    std::string display_filament_type;
    const std::string filament_type = const_cast<Preset&>(preset).get_filament_type(display_filament_type);
    if (canonicalize_filament_type_token(filament_type) == normalized_candidate)
        return true;

    return canonicalize_filament_type_token(display_filament_type) == normalized_candidate;
}

static std::string resolve_preferred_filament_preset_name_for_item(const PresetBundle& bundle, int item_index)
{
    if (item_index >= 0 && item_index < static_cast<int>(bundle.filament_presets.size()) &&
        !bundle.filament_presets[item_index].empty())
        return bundle.filament_presets[item_index];

    return bundle.filaments.get_selected_preset_name();
}

static std::string resolve_filament_preset_name_by_type_candidate(const std::string& candidate, int item_index)
{
    auto* bundle = wxGetApp().preset_bundle;
    if (bundle == nullptr || candidate.empty())
        return {};

    const PresetWithVendorProfile active_printer =
        bundle->printers.get_preset_with_vendor_profile(bundle->printers.get_selected_preset());
    const std::string preferred_preset_name = resolve_preferred_filament_preset_name_for_item(*bundle, item_index);

    const Preset* best_preset = nullptr;
    int best_score = std::numeric_limits<int>::min();

    for (const Preset& preset : bundle->filaments.get_presets()) {
        if (!preset_matches_filament_type_candidate(preset, candidate))
            continue;

        const bool is_preferred = !preferred_preset_name.empty() && preset.name == preferred_preset_name;
        const bool is_visible = preset.is_visible;
        const bool is_compatible =
            is_compatible_with_printer(bundle->filaments.get_preset_with_vendor_profile(preset), active_printer);

        int score = 0;
        if (is_visible && is_compatible)
            score += 100;
        if (is_visible)
            score += 20;
        if (is_compatible)
            score += 10;
        if (is_preferred)
            score += 5;
        if (!preset.is_default)
            score += 1;

        if (score > best_score) {
            best_score = score;
            best_preset = &preset;
        }
    }

    return best_preset != nullptr ? best_preset->name : std::string();
}

static std::string resolve_filament_preset_name_from_candidate(const std::string& candidate)
{
    auto* bundle = wxGetApp().preset_bundle;
    if (bundle == nullptr || candidate.empty() || candidate == "/" || candidate == "?")
        return {};

    // Device material metadata may already carry the exact slicer preset name.
    if (const Preset* exact_preset = bundle->filaments.find_preset(candidate, false))
        return exact_preset->name;

    const std::string resolved = bundle->get_preset_name_by_alias(Preset::TYPE_FILAMENT, candidate);
    if (resolved.empty())
        return {};

    if (const Preset* resolved_preset = bundle->filaments.find_preset(resolved, false))
        return resolved_preset->name;

    return {};
}

static std::vector<std::string> build_filament_family_fallbacks(const std::string& candidate)
{
    if (candidate.empty())
        return {};

    std::string normalized = candidate;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });

    static const std::vector<std::string> known_families = {
        "PLA", "PETG", "TPU", "ABS", "ASA", "PA", "PET", "PC", "PP", "PVA", "HIPS"
    };

    std::vector<std::string> fallbacks;
    fallbacks.reserve(known_families.size());
    for (const std::string& family : known_families) {
        if (normalized.find(family) != std::string::npos)
            fallbacks.push_back(family);
    }

    return fallbacks;
}

static std::string resolve_target_filament_preset_name_from_slot(const DeviceMaterialSlot& slot, int item_index = -1)
{
    std::vector<std::string> candidates;
    candidates.reserve(8);

    auto append_candidate = [&candidates](const std::string& value) {
        if (value.empty())
            return;
        if (std::find(candidates.begin(), candidates.end(), value) == candidates.end())
            candidates.push_back(value);
    };

    append_candidate(slot.material_name);
    append_candidate(slot.material_type);

    for (const std::string& fallback : build_filament_family_fallbacks(slot.material_name))
        append_candidate(fallback);
    for (const std::string& fallback : build_filament_family_fallbacks(slot.material_type))
        append_candidate(fallback);

    for (const std::string& candidate : candidates) {
        const std::string preset_name = resolve_filament_preset_name_from_candidate(candidate);
        if (!preset_name.empty())
            return preset_name;
    }

    for (const std::string& candidate : candidates) {
        const std::string preset_name = resolve_filament_preset_name_by_type_candidate(candidate, item_index);
        if (!preset_name.empty())
            return preset_name;
    }

    return {};
}

static bool sync_filament_preset(int item_index, const DeviceMaterialSlot& slot)
{
    auto* bundle = wxGetApp().preset_bundle;
    if (bundle == nullptr)
        return false;

    const std::string preset_name = resolve_target_filament_preset_name_from_slot(slot, item_index);
    if (preset_name.empty())
        return false;

    if (item_index >= 0 && item_index < static_cast<int>(bundle->filament_presets.size()) &&
        bundle->filament_presets[item_index] == preset_name)
        return true;

    bundle->set_filament_preset(item_index, preset_name);
    return true;
}

static bool sync_filament_color(int item_index, const std::string& color_hex)
{
    auto* bundle = wxGetApp().preset_bundle;
    if (bundle == nullptr || color_hex.empty())
        return false;

    DynamicPrintConfig* cfg = &bundle->project_config;
    const auto* current_colors = cfg->option<ConfigOptionStrings>("filament_colour");
    if (current_colors == nullptr || item_index < 0 || item_index >= static_cast<int>(current_colors->values.size()))
        return false;

    auto* colors = static_cast<ConfigOptionStrings*>(current_colors->clone());
    if (colors == nullptr)
        return false;

    const std::string normalized_color = normalize_hex(color_hex);
    if (colors->values[item_index] == normalized_color) {
        delete colors;
        return true;
    }

    colors->values[item_index] = normalized_color;
    DynamicPrintConfig cfg_new = *cfg;
    cfg_new.set_key_value("filament_colour", colors);
    cfg->apply(cfg_new);
    return true;
}

static int resolve_mapping_item_count_hint(const json& mapping_items)
{
    int item_count = mapping_items.is_array() ? static_cast<int>(mapping_items.size()) : 0;
    if (!mapping_items.is_array())
        return item_count;

    for (const auto& item : mapping_items) {
        if (!item.is_object())
            continue;
        item_count = std::max(item_count, item.value("item_index", -1) + 1);
    }

    return item_count;
}

static FilamentMappingService::MappingChannel resolve_current_plate_mapping_channel(const json& mapping_items, int plate_index)
{
    const auto current_plate_indices = FilamentMappingService::collect_plate_item_indices(resolve_mapping_item_count_hint(mapping_items), plate_index);
    if (current_plate_indices.empty())
        return FilamentMappingService::MappingChannel::None;

    bool has_cfs = false;
    bool has_external = false;
    for (const auto& item : mapping_items) {
        if (!item.is_object() || !item.value("mapped", false))
            continue;
        if (!item_is_on_current_plate(item, current_plate_indices))
            continue;

        switch (resolve_item_channel(item)) {
        case FilamentMappingService::MappingChannel::CFS:
            has_cfs = true;
            break;
        case FilamentMappingService::MappingChannel::External:
            has_external = true;
            break;
        default:
            break;
        }

        if (has_cfs && has_external)
            return FilamentMappingService::MappingChannel::Mixed;
    }

    if (has_cfs)
        return FilamentMappingService::MappingChannel::CFS;
    if (has_external)
        return FilamentMappingService::MappingChannel::External;
    return FilamentMappingService::MappingChannel::None;
}

static bool scene_item_matches_slot(int item_index, const DeviceMaterialSlot& slot)
{
    auto* bundle = wxGetApp().preset_bundle;
    if (bundle == nullptr)
        return false;

    const auto* colors = bundle->project_config.option<ConfigOptionStrings>("filament_colour");
    if (colors == nullptr || item_index < 0 || item_index >= static_cast<int>(colors->values.size()))
        return false;
    if (item_index >= static_cast<int>(bundle->filament_presets.size()))
        return false;

    const std::string expected_preset = resolve_target_filament_preset_name_from_slot(slot, item_index);
    if (expected_preset.empty())
        return false;

    return bundle->filament_presets[item_index] == expected_preset &&
           normalize_hex(colors->values[item_index]) == normalize_hex(slot.color_hex);
}

} // namespace

std::vector<unsigned int> FilamentMappingService::resolve_physical_source_filament_ids(unsigned int filament_id, size_t num_physical)
{
    std::vector<unsigned int> component_ids;
    if (filament_id < 1 || num_physical == 0)
        return component_ids;

    auto append_component = [&component_ids, num_physical](unsigned int component_id) {
        if (component_id < 1 || component_id > num_physical)
            return;
        if (std::find(component_ids.begin(), component_ids.end(), component_id) == component_ids.end())
            component_ids.push_back(component_id);
    };

    if (filament_id <= num_physical) {
        append_component(filament_id);
        return component_ids;
    }

    auto* bundle = wxGetApp().preset_bundle;
    if (bundle == nullptr)
        return component_ids;

    const MixedFilament* mixed = bundle->mixed_filaments.mixed_filament_from_id(filament_id, num_physical);
    if (mixed == nullptr)
        return component_ids;

    if (!mixed->gradient_component_ids.empty()) {
        for (const char component_token : mixed->gradient_component_ids) {
            if (component_token >= '1' && component_token <= '9')
                append_component(static_cast<unsigned int>(component_token - '0'));
        }
    } else if (!mixed->manual_pattern.empty()) {
        for (const char component_token : mixed->manual_pattern) {
            if (component_token < '1' || component_token > '9')
                continue;
            const unsigned int component_id = static_cast<unsigned int>(component_token - '0');
            if (component_id == 1)
                append_component(mixed->component_a);
            else if (component_id == 2)
                append_component(mixed->component_b);
            else
                append_component(component_id);
        }
    } else {
        append_component(mixed->component_a);
        append_component(mixed->component_b);
    }

    return component_ids;
}

std::unordered_set<int> FilamentMappingService::collect_plate_item_indices(int item_count, int preferred_plate_index)
{
    return collect_plate_item_indices_impl(item_count, preferred_plate_index);
}

json FilamentMappingService::filter_source_items_to_plate(
    const json& source_items,
    int preferred_plate_index)
{
    json filtered = json::array();
    if (!source_items.is_array())
        return filtered;

    int item_count_hint = 0;
    for (const auto& source_item : source_items) {
        if (!source_item.is_object())
            continue;

        int item_index = source_item.value("item_index", -1);
        if (item_index < 0)
            item_index = source_item.value("extruderId", 0) - 1;
        if (item_index < 0)
            continue;

        item_count_hint = std::max(item_count_hint, item_index + 1);
    }

    if (item_count_hint <= 0)
        return filtered;

    const auto plate_item_indices = collect_plate_item_indices(item_count_hint, preferred_plate_index);
    if (plate_item_indices.empty())
        return filtered;

    for (const auto& source_item : source_items) {
        if (!source_item.is_object())
            continue;

        int item_index = source_item.value("item_index", -1);
        if (item_index < 0)
            item_index = source_item.value("extruderId", 0) - 1;
        if (item_index < 0 || plate_item_indices.count(item_index) == 0)
            continue;

        filtered.push_back(source_item);
    }

    return filtered;
}

FilamentMappingService::Mode FilamentMappingService::resolve_mode_for_device(const DM::Device& device, Mode desired)
{
    if (desired == Mode::All)
        return Mode::All;

    if (device_has_available_materials(device, desired))
        return desired;

    const Mode fallback = desired == Mode::CFS ? Mode::External : Mode::CFS;
    if (device_has_available_materials(device, fallback))
        return fallback;

    return desired;
}

FilamentMappingService::Mode FilamentMappingService::resolve_auto_mapping_mode(const DM::Device& device)
{
    return resolve_mode_for_device(device, Mode::CFS);
}

std::string FilamentMappingService::mode_to_string(Mode mode)
{
    if (mode == Mode::All)
        return std::string("all");
    return mode == Mode::External ? std::string("external") : std::string("cfs");
}

bool FilamentMappingService::device_has_available_materials(const DM::Device& device, Mode mode)
{
    const std::vector<DeviceMaterialSlot> slots = collect_slots(device, mode);
    return std::any_of(slots.begin(), slots.end(), [](const DeviceMaterialSlot& slot) {
        return slot.available;
    });
}

FilamentMappingService::MappingChannel FilamentMappingService::resolve_mapping_channel(const json& mapping_items)
{
    if (!mapping_items.is_array())
        return MappingChannel::None;

    bool has_cfs = false;
    bool has_external = false;
    for (const auto& item : mapping_items) {
        switch (resolve_item_channel(item)) {
        case MappingChannel::CFS:
            has_cfs = true;
            break;
        case MappingChannel::External:
            has_external = true;
            break;
        default:
            break;
        }

        if (has_cfs && has_external)
            return MappingChannel::Mixed;
    }

    if (has_cfs)
        return MappingChannel::CFS;
    if (has_external)
        return MappingChannel::External;
    return MappingChannel::None;
}

std::string FilamentMappingService::mapping_channel_to_string(MappingChannel channel)
{
    switch (channel) {
    case MappingChannel::CFS:
        return "cfs";
    case MappingChannel::External:
        return "external";
    case MappingChannel::Mixed:
        return "mixed";
    default:
        return "none";
    }
}

bool FilamentMappingService::has_mixed_mapping_channels(const json& mapping_items)
{
    return resolve_mapping_channel(mapping_items) == MappingChannel::Mixed;
}

json FilamentMappingService::build_mapping_items(
    const json& source_items,
    const DM::Device& /*device*/,
    Mode /*mode*/)
{
    json items = json::array();
    if (!source_items.is_array())
        return items;

    for (const auto& source_item : source_items) {
        if (!source_item.is_object())
            continue;

        int item_index = source_item.value("item_index", -1);
        if (item_index < 0)
            item_index = source_item.value("extruderId", 0) - 1;
        if (item_index < 0)
            continue;

        const std::string source_color = normalize_hex(source_item.value("sourceColor", std::string()));
        const std::string preset_display = source_item.value("presetDisplay", std::string());
        const std::string source_preset = source_item.value("sourceFilamentPreset", std::string());
        const std::string filament_type = source_item.value("extruderFilamentType", std::string());

        json item;
        item["item_index"] = item_index;
        item["extruderId"] = source_item.value("extruderId", item_index + 1);
        item["extruderFilamentType"] = filament_type;
        item["presetDisplay"] = preset_display.empty() ? source_preset : preset_display;
        item["sourceFilamentPreset"] = source_preset;
        item["sourceColor"] = source_color;
        item["mapped"] = false;
        item["slotLabel"] = std::string();
        item["matchColor"] = source_color;
        item["boxType"] = -1;
        item["boxId"] = -1;
        item["materialId"] = -1;
        item["materialType"] = std::string();
        item["materialName"] = std::string();
        item["selection_token"] = std::string();
        item["sharedSlot"] = false;
        item["autoReused"] = false;
        item["mappingWarning"] = std::string();
        item["source_snapshot"] = true;
        items.push_back(std::move(item));
    }

    return items;
}

json FilamentMappingService::build_mapping_option_groups(const DM::Device& device, Mode mode)
{
    json groups = json::array();
    const std::vector<DeviceMaterialSlot> slots = collect_slots(device, mode);

    for (const DeviceMaterialSlot& slot : slots) {
        const std::string group_label = group_label_for(slot.box_type, slot.box_id);
        auto group_it = std::find_if(groups.begin(), groups.end(), [&](const json& group) {
            return group.is_object() && group.value("label", std::string()) == group_label;
        });

        if (group_it == groups.end()) {
            groups.push_back({
                {"label", group_label},
                {"options", json::array()}
            });
            group_it = std::prev(groups.end());
        }

        (*group_it)["options"].push_back({
            {"selection_token", slot.selection_token},
            {"slot_label", slot.slot_label},
            {"material_label", slot.material_name},
            {"material_match_key", slot.material_type},
            {"available", slot.available},
            {"material_color", slot.color_hex}
        });
    }

    return groups;
}

json FilamentMappingService::auto_match(
    const json& source_items,
    const DM::Device& device,
    Mode mode)
{
    json items = build_mapping_items(source_items, device, mode);
    const std::vector<DeviceMaterialSlot> slots = collect_slots(device, mode);
    if (!items.is_array() || slots.empty())
        return items;

    std::unordered_set<std::string> used_selection_tokens;
    for (auto& item : items) {
        if (!item.is_object())
            continue;

        const DeviceMaterialSlot* best_slot = nullptr;
        double best_distance = std::numeric_limits<double>::max();
        const std::string source_color = item.value("sourceColor", std::string());

        for (const DeviceMaterialSlot& slot : slots) {
            if (!slot.available)
                continue;
            if (used_selection_tokens.find(slot.selection_token) != used_selection_tokens.end())
                continue;

            const double distance = ColorMatch::calculatePerceptualColorDistance(source_color, slot.color_hex);
            if (best_slot == nullptr || distance < best_distance) {
                best_slot = &slot;
                best_distance = distance;
            }
        }

        if (best_slot != nullptr) {
            apply_slot_to_item(item, *best_slot);
            used_selection_tokens.insert(best_slot->selection_token);
        }
    }

    // When there are more scene items than unique device slots, do a second
    // pass that allows slot reuse so the user starts from a fully populated
    // suggestion instead of manual cleanup from blank rows.
    for (auto& item : items) {
        if (!item.is_object() || item.value("mapped", false))
            continue;

        const DeviceMaterialSlot* best_slot = nullptr;
        double best_distance = std::numeric_limits<double>::max();
        const std::string source_color = item.value("sourceColor", std::string());

        for (const DeviceMaterialSlot& slot : slots) {
            if (!slot.available)
                continue;

            const double distance = ColorMatch::calculatePerceptualColorDistance(source_color, slot.color_hex);
            if (best_slot == nullptr || distance < best_distance) {
                best_slot = &slot;
                best_distance = distance;
            }
        }

        if (best_slot != nullptr) {
            apply_slot_to_item(item, *best_slot);
            item["sharedSlot"] = true;
            item["autoReused"] = true;
            item["mappingWarning"] = "slot_reused";
        }
    }

    return items;
}
json FilamentMappingService::match_current_scene(
    const json& source_items,
    const DM::Device& device,
    Mode mode)
{
    json items = build_mapping_items(source_items, device, mode);
    const std::vector<DeviceMaterialSlot> slots = collect_slots(device, mode);
    if (!items.is_array() || slots.empty())
        return items;

    for (auto& item : items) {
        if (!item.is_object())
            continue;

        const int item_index = item.value("item_index", -1);
        for (const DeviceMaterialSlot& slot : slots) {
            if (!slot.available)
                continue;
            if (!scene_item_matches_slot(item_index, slot))
                continue;

            apply_slot_to_item(item, slot);
            break;
        }
    }

    return items;
}

json FilamentMappingService::build_color_match_info(
    const json& mapping_items,
    const DM::Device& device)
{
    json arr = json::array();
    if (!mapping_items.is_array() || !device.valid)
        return arr;

    const std::vector<DeviceMaterialSlot> slots = collect_slots(device, Mode::All);
    for (const auto& item : mapping_items) {
        if (!item.is_object() || !item.value("mapped", false))
            continue;

        const DeviceMaterialSlot* slot = find_slot_for_mapping_item(slots, item);
        if (slot == nullptr)
            continue;

        const std::string filament_type = resolve_item_filament_type(item);
        const std::string extruder_color = normalize_hex(item.value("sourceColor", std::string()));

        json info;
        info["boxId"] = slot->box_id;
        info["boxType"] = slot->box_type;
        info["extruderId"] = item.value("extruderId", item.value("item_index", -1) + 1);
        info["extruderFilamentType"] = filament_type;
        info["extruderColor"] = extruder_color;
        info["filamentType"] = filament_type;
        info["matchColor"] = slot->color_hex;
        info["materialId"] = slot->material_id;
        info["materialName"] = slot->material_name;
        info["slotLabel"] = slot->slot_label;
        info["selection_token"] = slot->selection_token;
        info["cId"] = resolve_cid_for_slot(device, *slot);
        arr.push_back(std::move(info));
    }

    return arr;
}

bool FilamentMappingService::has_cfs_mapping(
    const json& mapping_items,
    const DM::Device& device)
{
    if (!mapping_items.is_array() || !device.valid)
        return false;

    return resolve_mapping_channel(mapping_items) == MappingChannel::CFS;
}

bool FilamentMappingService::apply_selection(
    json& mapping_items,
    int item_index,
    const std::string& selection_token,
    const DM::Device& device,
    Mode mode)
{
    if (!mapping_items.is_array() || item_index < 0 || selection_token.empty())
        return false;

    const std::vector<DeviceMaterialSlot> slots = collect_slots(device, mode);
    const DeviceMaterialSlot* slot = find_slot_by_token(slots, selection_token);
    if (slot == nullptr || !slot->available)
        return false;

    for (auto& item : mapping_items) {
        if (!item.is_object() || item.value("item_index", -1) != item_index)
            continue;

        apply_slot_to_item(item, *slot);
        return true;
    }

    return false;
}

std::string FilamentMappingService::selection_token_for_slot_label(
    const std::string& slot_label,
    const DM::Device& device,
    Mode mode)
{
    if (slot_label.empty())
        return {};

    const std::vector<DeviceMaterialSlot> slots = collect_slots(device, mode);
    const auto it = std::find_if(slots.begin(), slots.end(), [&](const DeviceMaterialSlot& slot) {
        return slot.slot_label == slot_label && slot.available;
    });

    return it == slots.end() ? std::string() : it->selection_token;
}

bool FilamentMappingService::is_valid_selection_token(
    const std::string& selection_token,
    const DM::Device& device,
    Mode mode)
{
    if (selection_token.empty())
        return false;

    int box_type = -1;
    int box_id = -1;
    int material_id = -1;
    if (!parse_selection_token(selection_token, box_type, box_id, material_id))
        return false;

    const std::vector<DeviceMaterialSlot> slots = collect_slots(device, mode);
    const DeviceMaterialSlot* slot = find_slot_by_token(slots, selection_token);
    return slot != nullptr && slot->available;
}

bool FilamentMappingService::apply_mapping_to_scene(
    const json& mapping_items,
    const DM::Device& device,
    Mode mode,
    int plate_index)
{
    if (!mapping_items.is_array() || !device.valid)
        return false;
    if (resolve_current_plate_mapping_channel(mapping_items, plate_index) == MappingChannel::Mixed)
        return false;

    auto* plater = wxGetApp().plater();
    if (plater == nullptr || wxGetApp().preset_bundle == nullptr)
        return false;

    const std::vector<DeviceMaterialSlot> slots = collect_slots(device, mode);
    const auto current_plate_indices = FilamentMappingService::collect_plate_item_indices(resolve_mapping_item_count_hint(mapping_items), plate_index);
    if (current_plate_indices.empty())
        return false;

    bool applied_any = false;
    bool failed_required_sync = false;
    for (const auto& item : mapping_items) {
        if (!item.is_object() || !item.value("mapped", false))
            continue;
        if (!item_is_on_current_plate(item, current_plate_indices))
            continue;

        const std::string selection_token = item.value("selection_token", std::string());
        const DeviceMaterialSlot* slot = find_slot_by_token(slots, selection_token);
        if (slot == nullptr || !slot->available) {
            failed_required_sync = true;
            continue;
        }

        const int item_index = item.value("item_index", -1);
        const bool preset_synced = sync_filament_preset(item_index, *slot);
        const bool color_synced = sync_filament_color(item_index, slot->color_hex);
        if (!preset_synced || !color_synced) {
            failed_required_sync = true;
            continue;
        }

        plater->sidebar().sync_filament_box_state(item_index, slot->color_hex, wxString::FromUTF8(slot->slot_label.c_str()));
        plater->sidebar().auto_calc_flushing_volumes(item_index);
        applied_any = true;
    }

    if (applied_any) {
        plater->update_project_dirty_from_presets();
        if (wxGetApp().app_config != nullptr)
            wxGetApp().preset_bundle->export_selections(*wxGetApp().app_config);
        // Propagate the merged preset/color config back into the plater so the
        // live 3D scene refreshes its model render colors immediately.
        plater->on_config_change(wxGetApp().preset_bundle->full_config());
        plater->sidebar().update_dynamic_filament_list();
        plater->sidebar().update_filament_panel();
        plater->sidebar().update_mixed_filament_panel(false);
    }

    return applied_any && !failed_required_sync;
}

bool FilamentMappingService::scene_matches_mapping(
    const json& mapping_items,
    const DM::Device& device,
    Mode mode,
    int plate_index)
{
    return build_scene_match_diagnostics(mapping_items, device, mode, plate_index).value("matches", false);
}

json FilamentMappingService::build_scene_match_diagnostics(
    const json& mapping_items,
    const DM::Device& device,
    Mode mode,
    int plate_index)
{
    json diagnostics = {
        {"matches", false},
        {"reason", "unknown"},
        {"items", json::array()}
    };

    if (!mapping_items.is_array()) {
        diagnostics["reason"] = "mapping_items_not_array";
        return diagnostics;
    }
    if (!device.valid) {
        diagnostics["reason"] = "device_invalid";
        return diagnostics;
    }
    if (resolve_current_plate_mapping_channel(mapping_items, plate_index) == MappingChannel::Mixed) {
        diagnostics["reason"] = "mixed_mapping_channels";
        return diagnostics;
    }

    auto* bundle = wxGetApp().preset_bundle;
    if (bundle == nullptr) {
        diagnostics["reason"] = "preset_bundle_missing";
        return diagnostics;
    }

    const auto* colors = bundle->project_config.option<ConfigOptionStrings>("filament_colour");
    const std::vector<DeviceMaterialSlot> slots = collect_slots(device, mode);
    const auto current_plate_indices = FilamentMappingService::collect_plate_item_indices(resolve_mapping_item_count_hint(mapping_items), plate_index);
    if (current_plate_indices.empty()) {
        diagnostics["reason"] = "current_plate_indices_empty";
        return diagnostics;
    }

    bool has_relevant_mapped_item = false;
    std::string first_failure_reason;
    for (const auto& item : mapping_items) {
        if (!item.is_object() || !item.value("mapped", false))
            continue;
        if (!item_is_on_current_plate(item, current_plate_indices))
            continue;

        has_relevant_mapped_item = true;

        const int item_index = item.value("item_index", -1);
        const std::string selection_token = item.value("selection_token", std::string());
        const DeviceMaterialSlot* slot = find_slot_by_token(slots, selection_token);

        json item_diag = {
            {"item_index", item_index},
            {"selection_token", selection_token},
            {"slot_found", slot != nullptr},
            {"slot_available", slot != nullptr && slot->available}
        };

        if (slot != nullptr) {
            item_diag["slot_label"] = slot->slot_label;
            item_diag["slot_material_name"] = slot->material_name;
            item_diag["slot_material_type"] = slot->material_type;
            item_diag["slot_box_type"] = slot->box_type;
            item_diag["slot_box_id"] = slot->box_id;
            item_diag["slot_material_id"] = slot->material_id;
            item_diag["expected_color"] = normalize_hex(slot->color_hex);
            item_diag["expected_preset"] = resolve_target_filament_preset_name_from_slot(*slot, item_index);
        } else {
            item_diag["expected_color"] = std::string();
            item_diag["expected_preset"] = std::string();
        }

        std::string actual_preset;
        if (item_index >= 0 && item_index < static_cast<int>(bundle->filament_presets.size()))
            actual_preset = bundle->filament_presets[item_index];
        item_diag["actual_preset"] = actual_preset;

        std::string actual_color;
        if (colors != nullptr && item_index >= 0 && item_index < static_cast<int>(colors->values.size()))
            actual_color = normalize_hex(colors->values[item_index]);
        item_diag["actual_color"] = actual_color;

        const std::string expected_preset = item_diag.value("expected_preset", std::string());
        const std::string expected_color = item_diag.value("expected_color", std::string());
        const bool preset_match = !expected_preset.empty() && actual_preset == expected_preset;
        const bool color_match = !expected_color.empty() && actual_color == expected_color;
        const FilamentMappingService::MappingChannel item_channel = resolve_item_channel_with_slot(item, slot);
        const bool color_required = item_channel != FilamentMappingService::MappingChannel::External;
        item_diag["mapping_channel"] = FilamentMappingService::mapping_channel_to_string(item_channel);
        item_diag["preset_match"] = preset_match;
        item_diag["color_match"] = color_match;
        item_diag["color_required"] = color_required;

        std::string item_reason = "matched";
        if (slot == nullptr)
            item_reason = "slot_not_found";
        else if (!slot->available)
            item_reason = "slot_unavailable";
        else if (expected_preset.empty())
            item_reason = "expected_preset_unresolved";
        else if (!preset_match)
            item_reason = "preset_mismatch";
        else if (color_required && !color_match)
            item_reason = "color_mismatch";
        else if (!color_required && !color_match)
            item_diag["color_mismatch_ignored"] = true;
        item_diag["reason"] = item_reason;
        item_diag["matches"] = item_reason == "matched";

        diagnostics["items"].push_back(item_diag);
        if (item_reason != "matched" && first_failure_reason.empty())
            first_failure_reason = item_reason;
    }

    if (!has_relevant_mapped_item) {
        diagnostics["reason"] = "no_relevant_mapped_items";
        return diagnostics;
    }

    if (!first_failure_reason.empty()) {
        diagnostics["reason"] = first_failure_reason;
        return diagnostics;
    }

    diagnostics["matches"] = true;
    diagnostics["reason"] = "matched";
    return diagnostics;
}

} // namespace GUI
} // namespace Slic3r
