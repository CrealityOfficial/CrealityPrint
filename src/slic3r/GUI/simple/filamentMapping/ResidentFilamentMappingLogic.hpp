#pragma once

#include <string>
#include <vector>

#include "imgui/imgui.h"

namespace Slic3r {
namespace GUI {
namespace ResidentFilamentMapping {

enum class UiMode {
    MultiColorOnline = 0,
    SingleColorDevice,
    MultiColorOffline
};

enum class SummaryTone {
    Good = 0,
    Warn,
    Offline
};

enum class RowPresentation {
    InteractiveSelector = 0,
    DisabledSelector,
    UnifiedOutput
};

struct RuntimeSignals {
    bool device_is_online = false;
    bool device_supports_multi_color = false;
    bool device_materials_available = false;
    int  scene_color_count = 0;
    bool has_cached_mapping_result = false;
};

struct UnifiedOutputInput {
    bool        valid = false;
    std::string slot_label;
    std::string material_type;
    ImVec4      material_color = ImVec4(1.f, 1.f, 1.f, 1.f);
};

struct RowInput {
    int         item_index = 0;
    ImVec4      scene_color = ImVec4(1.f, 1.f, 1.f, 1.f);
    std::string scene_label;
    std::string scene_material_type;

    bool        has_target = false;
    std::string target_slot_label;
    std::string target_material_type;
    ImVec4      target_material_color = ImVec4(1.f, 1.f, 1.f, 1.f);
};

struct SummaryViewModel {
    SummaryTone tone = SummaryTone::Warn;
    std::string subtitle;
    std::string pill_text;
    std::string banner_text;
};

struct RowViewModel {
    int             item_index = 0;
    RowPresentation presentation = RowPresentation::InteractiveSelector;
    ImVec4          scene_color = ImVec4(1.f, 1.f, 1.f, 1.f);
    std::string     scene_label;
    std::string     scene_material_type;
    std::string     status_text;

    bool            selector_enabled = false;
    bool            selector_show_chevron = false;
    bool            selector_placeholder = false;
    bool            using_cached_target = false;

    std::string     target_slot_label;
    std::string     target_material_type;
    ImVec4          target_material_color = ImVec4(1.f, 1.f, 1.f, 1.f);
};

struct UiModel {
    UiMode                     mode = UiMode::MultiColorOnline;
    SummaryViewModel           summary;
    bool                       show_unified_output_card = false;
    UnifiedOutputInput         unified_output;
    std::vector<RowViewModel>  rows;
};

UiMode resolve_ui_mode(const RuntimeSignals& signals);

UiModel build_ui_model(
    const RuntimeSignals& signals,
    const std::vector<RowInput>& row_inputs,
    const UnifiedOutputInput& unified_output);

} // namespace ResidentFilamentMapping
} // namespace GUI
} // namespace Slic3r
