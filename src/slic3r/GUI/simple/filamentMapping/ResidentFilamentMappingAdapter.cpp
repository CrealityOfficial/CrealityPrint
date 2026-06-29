#include "ResidentFilamentMappingAdapter.hpp"
#include "ResidentFilamentMappingPopupPolicy.hpp"

#include "../../GUI_App.hpp"
#include "../../PartPlate.hpp"
#include "../../Plater.hpp"

#include <algorithm>
#include <unordered_set>
#include <sstream>
#include <utility>

namespace Slic3r {
namespace GUI {
namespace ResidentFilamentMappingAdapter {

namespace {

using ResidentFilamentMapping::RowInput;
using ResidentFilamentMapping::RowViewModel;
using ResidentFilamentMapping::UiModel;
using ResidentFilamentMapping::UnifiedOutputInput;
using ResidentFilamentMappingView::PanelViewData;
using ResidentFilamentMappingView::RowViewData;

struct PopupMaterialOption {
    int          box_id = 0;
    int          box_type = 0;
    DM::Material material;
};

struct PlateRowSplit {
    bool                    valid = false;
    int                     current_plate_index = -1;
    int                     plate_count = 0;
    std::unordered_set<int> current_plate_item_indices;
    std::unordered_set<int> other_plate_item_indices;
};

static std::string clamp_hex(const std::string& in)
{
    if (in.empty())
        return "#000000";
    if (in[0] != '#')
        return "#" + in;
    return in;
}

static ImVec4 local_hex_to_imvec4(const std::string& hex_in)
{
    std::string hex = clamp_hex(hex_in);
    unsigned int r = 255, g = 255, b = 255, a = 255;
    if (hex.size() == 7 || hex.size() == 9) {
        unsigned int v = 0;
        std::stringstream ss;
        ss << std::hex << hex.substr(1);
        ss >> v;
        if (hex.size() == 7) {
            r = (v >> 16) & 0xFF;
            g = (v >> 8) & 0xFF;
            b = v & 0xFF;
            a = 255;
        } else {
            r = (v >> 24) & 0xFF;
            g = (v >> 16) & 0xFF;
            b = (v >> 8) & 0xFF;
            a = v & 0xFF;
        }
    }
    return ImVec4(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
}

static std::string readable_scene_label(const ImGuiFilamentItemState& item)
{
    if (!item.type_label.empty())
        return item.type_label;
    if (!item.preset_display.empty())
        return item.preset_display;
    return "Filament";
}

static std::string popup_material_match_key(const DM::Material& material)
{
    if (!material.type.empty())
        return material.type;
    if (!material.name.empty())
        return material.name;
    return std::string();
}

static std::string popup_material_display_label(const DM::Material& material)
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

static std::string popup_slot_label(const PopupMaterialOption& option)
{
    if (option.box_type == 2)
        return "EXT";

    const char letter = static_cast<char>('A' + (option.material.material_id % 26));
    return std::to_string(option.box_id) + letter;
}

static std::string popup_material_label(const PopupMaterialOption& option)
{
    return popup_material_display_label(option.material);
}

static std::string popup_option_token(const PopupMaterialOption& option)
{
    return std::to_string(option.box_type) + ":" + std::to_string(option.box_id) + ":" + std::to_string(option.material.material_id);
}

static bool is_cfs_box_type(int box_type)
{
    return box_type == 0 || box_type == 2;
}

static const DM::Material* find_material_by_slot_label(const DM::Device& device, const std::string& slot_label)
{
    if (slot_label.empty())
        return nullptr;

    for (const auto& box : device.materialBoxes) {
        if (!does_box_type_match_mode(ImGuiFilamentPanel::Mode::CFS, box.box_type) && box.box_type != 1)
            continue;

        for (const auto& material : box.materials) {
            PopupMaterialOption option;
            option.box_id = box.box_id;
            option.box_type = box.box_type;
            option.material = material;
            if (popup_slot_label(option) == slot_label)
                return &material;
        }
    }

    return nullptr;
}

static std::string resolve_target_material_label(const ImGuiFilamentItemState& item, const DM::Device& device)
{
    if (!item.device_match_slot.empty()) {
        if (const DM::Material* material = find_material_by_slot_label(device, item.device_match_slot))
            return popup_material_display_label(*material);
        return u8"\u8bbe\u5907\u8017\u6750";
    }

    return readable_scene_label(item);
}

static std::vector<PopupMaterialOption> collect_popup_options(const DM::Device& device)
{
    std::vector<PopupMaterialOption> options;
    if (!device.valid)
        return options;

    for (const auto& box : device.materialBoxes) {
        if (!is_cfs_box_type(box.box_type))
            continue;

        for (const auto& material : box.materials) {
            if (!ResidentFilamentMappingPopupPolicy::should_include_material_option(box.box_type, material.material_id))
                continue;

            bool duplicate = false;
            for (const auto& existing : options) {
                if (existing.box_id == box.box_id &&
                    existing.box_type == box.box_type &&
                    existing.material.material_id == material.material_id) {
                    duplicate = true;
                    break;
                }
            }
            if (duplicate)
                continue;

            PopupMaterialOption option;
            option.box_id = box.box_id;
            option.box_type = box.box_type;
            option.material = material;
            options.push_back(std::move(option));
        }
    }

    return options;
}

static void append_plate_extruders_to_item_indices(
    const std::vector<int>& extruders,
    int item_count,
    std::unordered_set<int>& item_indices)
{
    for (const int extruder_id : extruders) {
        const int item_index = extruder_id - 1;
        if (item_index < 0 || item_index >= item_count)
            continue;
        item_indices.insert(item_index);
    }
}

static PlateRowSplit collect_plate_row_split(int item_count)
{
    PlateRowSplit split;
    if (item_count <= 0 || wxGetApp().plater() == nullptr)
        return split;

    PartPlateList& plate_list = wxGetApp().plater()->get_partplate_list();
    split.plate_count = plate_list.get_plate_count();
    split.current_plate_index = plate_list.get_curr_plate_index();

    if (split.plate_count <= 0 ||
        split.current_plate_index < 0 ||
        split.current_plate_index >= split.plate_count)
        return split;

    split.valid = true;
    for (int plate_index = 0; plate_index < split.plate_count; ++plate_index) {
        PartPlate* plate = plate_list.get_plate(plate_index);
        if (plate == nullptr)
            continue;

        if (plate_index == split.current_plate_index)
            append_plate_extruders_to_item_indices(plate->get_extruders(true), item_count, split.current_plate_item_indices);
        else
            append_plate_extruders_to_item_indices(plate->get_extruders(true), item_count, split.other_plate_item_indices);
    }

    for (const int item_index : split.current_plate_item_indices)
        split.other_plate_item_indices.erase(item_index);

    return split;
}

static std::string build_plate_scope_subtitle(const PlateRowSplit& split)
{
    if (!split.valid)
        return std::string();

    if (split.plate_count <= 1)
        return u8"当前仅 1 个盘";

    return u8"当前盘 P" + std::to_string(split.current_plate_index + 1) +
           u8" / 共 " + std::to_string(split.plate_count) + u8" 盘";
}

static std::string build_color_count_text(size_t count)
{
    return std::to_string(static_cast<int>(count)) + u8" 个颜色";
}

static RowViewData build_row_view_data(
    const RowViewModel& row,
    const PopupOptionCatalog& popup_catalog,
    const std::vector<ImGuiFilamentItemState>& items,
    const ResidentFilamentMappingPopupPolicy::PopupMatchPolicyConfig& popup_policy)
{
    RowViewData row_data;
    row_data.row = row;
    row_data.popup_id = "MapPopup##" + std::to_string(row.item_index);
    if (row.item_index >= 0 && row.item_index < static_cast<int>(items.size())) {
        row_data.popup_groups = ResidentFilamentMappingPopupPolicy::build_popup_group_view_models(
            popup_catalog,
            items[row.item_index],
            row.target_slot_label,
            popup_policy);
    }

    return row_data;
}

} // namespace

bool does_box_type_match_mode(ImGuiFilamentPanel::Mode mode, int box_type)
{
    return mode == ImGuiFilamentPanel::Mode::External ? (box_type == 1) : is_cfs_box_type(box_type);
}

bool device_has_available_materials(const DM::Device& device, ImGuiFilamentPanel::Mode mode)
{
    if (!device.valid)
        return false;

    for (const auto& box : device.materialBoxes) {
        if (!does_box_type_match_mode(mode, box.box_type))
            continue;
        for (const auto& material : box.materials) {
            if (!material.color.empty())
                return true;
        }
    }

    return false;
}

bool has_cached_mapping_result(const std::vector<ImGuiFilamentItemState>& items)
{
    return std::any_of(items.begin(), items.end(), [](const ImGuiFilamentItemState& item) {
        return !item.device_match_slot.empty();
    });
}

UnifiedOutputInput collect_unified_output_input(const DM::Device& device)
{
    UnifiedOutputInput unified;
    if (!device.valid)
        return unified;

    const DM::Material* chosen = nullptr;
    int chosen_box_type = -1;
    int chosen_box_id = -1;

    auto pick_from_box_type = [&](int box_type) -> bool {
        const DM::Material* fallback = nullptr;
        int fallback_box_id = -1;
        for (const auto& box : device.materialBoxes) {
            if (box.box_type != box_type)
                continue;
            for (const auto& material : box.materials) {
                if (material.color.empty())
                    continue;
                if (material.selected) {
                    chosen = &material;
                    chosen_box_type = box.box_type;
                    chosen_box_id = box.box_id;
                    return true;
                }
                if (fallback == nullptr) {
                    fallback = &material;
                    fallback_box_id = box.box_id;
                }
            }
        }
        if (fallback != nullptr) {
            chosen = fallback;
            chosen_box_type = box_type;
            chosen_box_id = fallback_box_id;
            return true;
        }
        return false;
    };

    if (!pick_from_box_type(1))
        if (!pick_from_box_type(2))
            pick_from_box_type(0);

    if (chosen == nullptr)
        return unified;

    unified.valid = true;
    unified.material_type = !chosen->type.empty() ? chosen->type : (!chosen->name.empty() ? chosen->name : "Filament");
    unified.material_color = local_hex_to_imvec4(chosen->color);
    if (chosen_box_type == 1 || chosen_box_type == 2)
        unified.slot_label = "EXT";
    else
        unified.slot_label = std::to_string(chosen_box_id) + static_cast<char>('A' + (chosen->material_id % 26));

    return unified;
}

std::vector<RowInput> collect_row_inputs_from_items(const std::vector<ImGuiFilamentItemState>& items, const DM::Device& device)
{
    std::vector<RowInput> rows;
    rows.reserve(items.size());

    for (const auto& item : items) {
        RowInput row;
        row.item_index = item.index;
        row.scene_color = item.color;
        row.scene_label = readable_scene_label(item);
        row.scene_material_type = readable_scene_label(item);
        row.has_target = !item.device_match_slot.empty();
        row.target_slot_label = item.device_match_slot;
        row.target_material_type = resolve_target_material_label(item, device);
        row.target_material_color = row.has_target ? item.match_color : item.color;
        rows.push_back(std::move(row));
    }

    return rows;
}

PopupOptionCatalog build_popup_option_catalog(const DM::Device& device)
{
    PopupOptionCatalog catalog;
    const std::vector<PopupMaterialOption> options = collect_popup_options(device);

    for (const PopupMaterialOption& option : options) {
        PopupOptionSeed seed;
        seed.selection_token = popup_option_token(option);
        seed.slot_label = popup_slot_label(option);
        seed.material_label = popup_material_label(option);
        seed.material_match_key = popup_material_match_key(option.material);
        seed.material_color = local_hex_to_imvec4(option.material.color);
        seed.available = !option.material.color.empty() && option.material.state != -1;

        const std::string group_label =
            ResidentFilamentMappingPopupPolicy::group_label_for_option(option.box_type, option.box_id);
        auto group_it = std::find_if(catalog.groups.begin(), catalog.groups.end(), [&](const PopupGroupSeed& group) {
            return group.label == group_label;
        });
        if (group_it == catalog.groups.end()) {
            catalog.groups.push_back(PopupGroupSeed{ group_label, {} });
            group_it = std::prev(catalog.groups.end());
        }
        group_it->options.push_back(seed);

        catalog.selection_lookup.emplace(seed.selection_token, PopupSelectionPayload{ seed.slot_label, seed.material_color });
    }

    return catalog;
}

PanelViewData build_panel_view_data(
    const UiModel& ui_model,
    const PopupOptionCatalog& popup_catalog,
    const std::vector<ImGuiFilamentItemState>& items,
    float scale,
    float child_h,
    float preview_w,
    float split_gap)
{
    return build_panel_view_data(
        ui_model,
        popup_catalog,
        items,
        scale,
        child_h,
        preview_w,
        split_gap,
        ResidentFilamentMappingPopupPolicy::default_match_policy_config());
}

PanelViewData build_panel_view_data(
    const UiModel& ui_model,
    const PopupOptionCatalog& popup_catalog,
    const std::vector<ImGuiFilamentItemState>& items,
    float scale,
    float child_h,
    float preview_w,
    float split_gap,
    const ResidentFilamentMappingPopupPolicy::PopupMatchPolicyConfig& popup_policy)
{
    PanelViewData view_data;
    view_data.mode = ui_model.mode;
    view_data.summary = ui_model.summary;
    view_data.show_unified_output_card = ui_model.show_unified_output_card;
    view_data.unified_output = ui_model.unified_output;
    view_data.scale = scale;
    view_data.child_h = child_h;
    view_data.preview_w = preview_w;
    view_data.split_gap = split_gap;
    view_data.rows.reserve(ui_model.rows.size());
    view_data.current_plate_rows.reserve(ui_model.rows.size());
    view_data.inactive_global_rows.reserve(ui_model.rows.size());
    view_data.non_current_rows.reserve(ui_model.rows.size());
    view_data.other_plate_rows.reserve(ui_model.rows.size());

    const PlateRowSplit plate_row_split = collect_plate_row_split(static_cast<int>(items.size()));
    view_data.plate_scope_valid = plate_row_split.valid;
    view_data.current_plate_index = plate_row_split.current_plate_index;
    view_data.plate_count = plate_row_split.plate_count;
    view_data.plate_scope_subtitle = build_plate_scope_subtitle(plate_row_split);
    view_data.current_plate_title = u8"当前盘颜色";
    view_data.other_plates_title = u8"其他盘颜色";

    for (const RowViewModel& row : ui_model.rows) {
        RowViewData row_data = build_row_view_data(row, popup_catalog, items, popup_policy);
        view_data.rows.push_back(std::move(row_data));
    }

    if (!plate_row_split.valid) {
        view_data.current_plate_rows = view_data.rows;
    } else {
        for (const RowViewData& row_data : view_data.rows) {
            const int item_index = row_data.row.item_index;
            const bool in_current = plate_row_split.current_plate_item_indices.count(item_index) > 0;
            if (in_current) {
                view_data.current_plate_rows.push_back(row_data);
                continue;
            }

            if (plate_row_split.other_plate_item_indices.count(item_index) > 0)
                view_data.other_plate_rows.push_back(row_data);

            view_data.inactive_global_rows.push_back(row_data);
            view_data.non_current_rows.push_back(row_data);
        }

    }

    view_data.current_plate_count_text = build_color_count_text(view_data.current_plate_rows.size());
    view_data.other_plates_count_text = build_color_count_text(view_data.non_current_rows.size());
    view_data.show_other_plates_section =
        view_data.plate_scope_valid &&
        !view_data.non_current_rows.empty();

    return view_data;
}

bool apply_popup_selection(const PopupOptionCatalog& popup_catalog, const std::string& selection_token, ImGuiFilamentItemState& item)
{
    const PopupSelectionPayload* payload = find_popup_selection_payload(popup_catalog, selection_token);
    if (payload == nullptr)
        return false;

    item.device_match_slot = payload->slot_label;
    item.sync_label = item.device_match_slot;
    item.match_color = payload->material_color;
    return true;
}

const PopupSelectionPayload* find_popup_selection_payload(const PopupOptionCatalog& popup_catalog, const std::string& selection_token)
{
    const auto it = popup_catalog.selection_lookup.find(selection_token);
    if (it == popup_catalog.selection_lookup.end())
        return nullptr;
    return &it->second;
}

} // namespace ResidentFilamentMappingAdapter
} // namespace GUI
} // namespace Slic3r
