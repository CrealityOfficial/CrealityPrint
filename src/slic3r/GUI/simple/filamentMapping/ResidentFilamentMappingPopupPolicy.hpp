#pragma once

#include <string>
#include <vector>

#include "ImGuiFilamentPanel.hpp"
#include "ResidentFilamentMappingAdapter.hpp"
#include "ResidentFilamentMappingView.hpp"

namespace Slic3r {
namespace GUI {
namespace ResidentFilamentMappingPopupPolicy {

enum class PopupMatchMode {
    StrictType = 0,
    LooseType,
    AvailabilityOnly
};

struct PopupMatchPolicyConfig {
    PopupMatchMode match_mode = PopupMatchMode::StrictType;
};

PopupMatchPolicyConfig default_match_policy_config();

PopupMatchPolicyConfig popup_policy_config_for_runtime(
    const DM::Device& device,
    ImGuiFilamentPanel::Mode mode,
    const ResidentFilamentMapping::RuntimeSignals& signals);

bool should_include_material_option(int box_type, int material_id);

std::string group_label_for_option(int box_type, int box_id);

bool does_option_match_item(
    const ImGuiFilamentItemState& item,
    const ResidentFilamentMappingAdapter::PopupOptionSeed& option,
    const PopupMatchPolicyConfig& config);

bool is_option_disabled(
    const ImGuiFilamentItemState& item,
    const ResidentFilamentMappingAdapter::PopupOptionSeed& option,
    const PopupMatchPolicyConfig& config);

std::vector<ResidentFilamentMappingView::PopupGroupViewModel> build_popup_group_view_models(
    const ResidentFilamentMappingAdapter::PopupOptionCatalog& popup_catalog,
    const ImGuiFilamentItemState& item,
    const std::string& selected_slot_label,
    const PopupMatchPolicyConfig& config);

} // namespace ResidentFilamentMappingPopupPolicy
} // namespace GUI
} // namespace Slic3r
