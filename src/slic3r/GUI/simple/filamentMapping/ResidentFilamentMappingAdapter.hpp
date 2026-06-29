#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "ImGuiFilamentPanel.hpp"
#include "ResidentFilamentMappingLogic.hpp"
#include "ResidentFilamentMappingView.hpp"

namespace Slic3r {
namespace GUI {
namespace ResidentFilamentMappingPopupPolicy {
struct PopupMatchPolicyConfig;
}

namespace ResidentFilamentMappingAdapter {

struct PopupSelectionPayload {
    std::string slot_label;
    ImVec4      material_color = ImVec4(1.f, 1.f, 1.f, 1.f);
};

struct PopupOptionSeed {
    std::string selection_token;
    std::string slot_label;
    std::string material_label;
    std::string material_match_key;
    ImVec4      material_color = ImVec4(1.f, 1.f, 1.f, 1.f);
    bool        available = false;
};

struct PopupGroupSeed {
    std::string                  label;
    std::vector<PopupOptionSeed> options;
};

struct PopupOptionCatalog {
    std::vector<PopupGroupSeed>                         groups;
    std::unordered_map<std::string, PopupSelectionPayload> selection_lookup;
};

bool does_box_type_match_mode(ImGuiFilamentPanel::Mode mode, int box_type);

bool device_has_available_materials(const DM::Device& device, ImGuiFilamentPanel::Mode mode);

bool has_cached_mapping_result(const std::vector<ImGuiFilamentItemState>& items);

ResidentFilamentMapping::UnifiedOutputInput collect_unified_output_input(const DM::Device& device);

std::vector<ResidentFilamentMapping::RowInput> collect_row_inputs_from_items(
    const std::vector<ImGuiFilamentItemState>& items,
    const DM::Device& device);

PopupOptionCatalog build_popup_option_catalog(const DM::Device& device);

ResidentFilamentMappingView::PanelViewData build_panel_view_data(
    const ResidentFilamentMapping::UiModel& ui_model,
    const PopupOptionCatalog& popup_catalog,
    const std::vector<ImGuiFilamentItemState>& items,
    float scale,
    float child_h,
    float preview_w,
    float split_gap);

ResidentFilamentMappingView::PanelViewData build_panel_view_data(
    const ResidentFilamentMapping::UiModel& ui_model,
    const PopupOptionCatalog& popup_catalog,
    const std::vector<ImGuiFilamentItemState>& items,
    float scale,
    float child_h,
    float preview_w,
    float split_gap,
    const ResidentFilamentMappingPopupPolicy::PopupMatchPolicyConfig& popup_policy);

bool apply_popup_selection(
    const PopupOptionCatalog& popup_catalog,
    const std::string& selection_token,
    ImGuiFilamentItemState& item);

const PopupSelectionPayload* find_popup_selection_payload(
    const PopupOptionCatalog& popup_catalog,
    const std::string& selection_token);

} // namespace ResidentFilamentMappingAdapter
} // namespace GUI
} // namespace Slic3r
