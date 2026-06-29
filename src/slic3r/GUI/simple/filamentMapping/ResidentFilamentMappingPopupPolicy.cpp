#include "ResidentFilamentMappingPopupPolicy.hpp"

#include <algorithm>
#include <cctype>
#include <utility>

namespace Slic3r {
namespace GUI {
namespace ResidentFilamentMappingPopupPolicy {

namespace {

static std::string to_lower_copy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

static std::string extract_material_family(const std::string& value)
{
    const std::string normalized = to_lower_copy(value);
    static const char* kFamilies[] = {
        "pla", "petg", "abs", "asa", "tpu", "pa", "nylon", "pc", "hips", "pva", "support"
    };

    for (const char* family : kFamilies) {
        if (normalized.find(family) != std::string::npos)
            return family;
    }

    return normalized;
}

static bool is_loose_match(const std::string& lhs, const std::string& rhs)
{
    if (lhs.empty() || rhs.empty())
        return true;
    if (lhs == rhs)
        return true;
    if (lhs.find(rhs) != std::string::npos || rhs.find(lhs) != std::string::npos)
        return true;

    return extract_material_family(lhs) == extract_material_family(rhs);
}

} // namespace

PopupMatchPolicyConfig default_match_policy_config()
{
    return PopupMatchPolicyConfig{};
}

PopupMatchPolicyConfig popup_policy_config_for_runtime(
    const DM::Device& device,
    ImGuiFilamentPanel::Mode mode,
    const ResidentFilamentMapping::RuntimeSignals& signals)
{
    PopupMatchPolicyConfig config = default_match_policy_config();

    if (!signals.device_is_online || !signals.device_materials_available) {
        config.match_mode = PopupMatchMode::AvailabilityOnly;
        return config;
    }

    if (mode == ImGuiFilamentPanel::Mode::External || !signals.device_supports_multi_color) {
        config.match_mode = PopupMatchMode::LooseType;
        return config;
    }

    bool has_external_source = false;
    for (const auto& box : device.materialBoxes) {
        if (box.box_type == 1 || box.box_type == 2) {
            has_external_source = true;
            break;
        }
    }

    config.match_mode = has_external_source ? PopupMatchMode::LooseType : PopupMatchMode::StrictType;
    return config;
}

bool should_include_material_option(int box_type, int material_id)
{
    if (box_type == 0 && (material_id < 0 || material_id > 3))
        return false;
    return true;
}

std::string group_label_for_option(int box_type, int box_id)
{
    return (box_type == 2) ? std::string("EXT") : (std::string("CFS ") + std::to_string(box_id));
}

bool does_option_match_item(
    const ImGuiFilamentItemState& item,
    const ResidentFilamentMappingAdapter::PopupOptionSeed& option,
    const PopupMatchPolicyConfig& config)
{
    if (item.type_label.empty())
        return true;

    const std::string scene_type = to_lower_copy(item.type_label);
    const std::string option_type = to_lower_copy(option.material_match_key);

    switch (config.match_mode) {
    case PopupMatchMode::AvailabilityOnly:
        return true;
    case PopupMatchMode::LooseType:
        return is_loose_match(scene_type, option_type);
    case PopupMatchMode::StrictType:
    default:
        return scene_type == option_type;
    }
}

bool is_option_disabled(
    const ImGuiFilamentItemState& item,
    const ResidentFilamentMappingAdapter::PopupOptionSeed& option,
    const PopupMatchPolicyConfig& config)
{
    if (!option.available)
        return true;

    if (!does_option_match_item(item, option, config))
        return true;

    return false;
}

std::vector<ResidentFilamentMappingView::PopupGroupViewModel> build_popup_group_view_models(
    const ResidentFilamentMappingAdapter::PopupOptionCatalog& popup_catalog,
    const ImGuiFilamentItemState& item,
    const std::string& selected_slot_label,
    const PopupMatchPolicyConfig& config)
{
    std::vector<ResidentFilamentMappingView::PopupGroupViewModel> view_groups;
    view_groups.reserve(popup_catalog.groups.size());

    for (const ResidentFilamentMappingAdapter::PopupGroupSeed& popup_group : popup_catalog.groups) {
        ResidentFilamentMappingView::PopupGroupViewModel view_group;
        view_group.label = popup_group.label;
        view_group.options.reserve(popup_group.options.size());

        for (const ResidentFilamentMappingAdapter::PopupOptionSeed& option : popup_group.options) {
            ResidentFilamentMappingView::PopupOptionViewModel view_option;
            view_option.selection_token = option.selection_token;
            view_option.widget_id = "##candidate" + option.selection_token;
            view_option.slot_label = option.slot_label;
            view_option.material_label = option.material_label;
            view_option.material_color = option.material_color;
            view_option.disabled = is_option_disabled(item, option, config);
            view_option.selected = (selected_slot_label == option.slot_label);
            view_group.options.push_back(std::move(view_option));
        }

        view_groups.push_back(std::move(view_group));
    }

    return view_groups;
}

} // namespace ResidentFilamentMappingPopupPolicy
} // namespace GUI
} // namespace Slic3r
