// FilamentMappingService is the UI-independent business layer for consumable
// mapping used by the AI send workflow and future non-ImGui mapping entry
// points.  It intentionally does not depend on ImGuiFilamentPanel or any ImGui
// state: callers provide source filament data, device material data, and the
// service returns or applies plain mapping data.  This keeps original 3mf
// source information separate from panel refresh lifecycles and makes the
// mapping path reusable by both simple and professional workflows.

#pragma once

#include "nlohmann/json.hpp"
#include "print_manage/data/DataType.hpp"

#include <cstddef>
#include <string>
#include <unordered_set>
#include <vector>

namespace Slic3r {
namespace GUI {

class FilamentMappingService
{
public:
    enum class Mode
    {
        All,
        CFS,
        External
    };

    enum class MappingChannel
    {
        None,
        CFS,
        External,
        Mixed
    };

    static Mode resolve_mode_for_device(const DM::Device& device, Mode desired = Mode::CFS);
    static Mode resolve_auto_mapping_mode(const DM::Device& device);
    static std::string mode_to_string(Mode mode);
    static bool device_has_available_materials(const DM::Device& device, Mode mode);
    static MappingChannel resolve_mapping_channel(const nlohmann::json& mapping_items);
    static std::string mapping_channel_to_string(MappingChannel channel);
    static bool has_mixed_mapping_channels(const nlohmann::json& mapping_items);
    static std::unordered_set<int> collect_plate_item_indices(int item_count, int preferred_plate_index = -1);
    static std::vector<unsigned int> resolve_physical_source_filament_ids(unsigned int filament_id, size_t num_physical);
    static nlohmann::json filter_source_items_to_plate(
        const nlohmann::json& source_items,
        int preferred_plate_index = -1);

    static nlohmann::json build_mapping_items(
        const nlohmann::json& source_items,
        const DM::Device& device,
        Mode mode);

    static nlohmann::json build_mapping_option_groups(
        const DM::Device& device,
        Mode mode);

    static nlohmann::json auto_match(
        const nlohmann::json& source_items,
        const DM::Device& device,
        Mode mode);

    static nlohmann::json match_current_scene(
        const nlohmann::json& source_items,
        const DM::Device& device,
        Mode mode);

    static nlohmann::json build_color_match_info(
        const nlohmann::json& mapping_items,
        const DM::Device& device);

    static bool has_cfs_mapping(
        const nlohmann::json& mapping_items,
        const DM::Device& device);

    static bool apply_selection(
        nlohmann::json& mapping_items,
        int item_index,
        const std::string& selection_token,
        const DM::Device& device,
        Mode mode);

    static std::string selection_token_for_slot_label(
        const std::string& slot_label,
        const DM::Device& device,
        Mode mode);

    static bool is_valid_selection_token(
        const std::string& selection_token,
        const DM::Device& device,
        Mode mode);

    static bool apply_mapping_to_scene(
        const nlohmann::json& mapping_items,
        const DM::Device& device,
        Mode mode,
        int plate_index = -1);

    // Returns true when the current slicer scene/config already reflects the
    // provided mapping for the active plate. This is used by workflow session
    // code to distinguish "mapping exists" from "mapping still needs apply".
    static bool scene_matches_mapping(
        const nlohmann::json& mapping_items,
        const DM::Device& device,
        Mode mode,
        int plate_index = -1);

    static nlohmann::json build_scene_match_diagnostics(
        const nlohmann::json& mapping_items,
        const DM::Device& device,
        Mode mode,
        int plate_index = -1);
};

} // namespace GUI
} // namespace Slic3r
