#pragma once

#include <functional>
#include <string>
#include <vector>

#include "imgui/imgui.h"

#include "ResidentFilamentMappingLogic.hpp"

namespace Slic3r {
namespace GUI {
namespace ResidentFilamentMappingView {

enum class PreviewViewType {
    Iso = 0,
    Front,
    Top
};

struct PopupOptionViewModel {
    std::string widget_id;
    std::string selection_token;
    std::string slot_label;
    std::string material_label;
    ImVec4      material_color = ImVec4(1.f, 1.f, 1.f, 1.f);
    bool        disabled = false;
    bool        selected = false;
};

struct PopupGroupViewModel {
    std::string                        label;
    std::vector<PopupOptionViewModel> options;
};

struct RowViewData {
    ResidentFilamentMapping::RowViewModel row;
    std::string                           popup_id;
    std::vector<PopupGroupViewModel>      popup_groups;
};

struct PanelViewData {
    ResidentFilamentMapping::UiMode            mode = ResidentFilamentMapping::UiMode::MultiColorOnline;
    ResidentFilamentMapping::SummaryViewModel  summary;
    bool                                       embed_in_unified_panel = false;
    bool                                       show_unified_output_card = false;
    ResidentFilamentMapping::UnifiedOutputInput unified_output;
    std::vector<RowViewData>                   rows;
    std::vector<RowViewData>                   current_plate_rows;
    std::vector<RowViewData>                   inactive_global_rows;
    std::vector<RowViewData>                   non_current_rows;
    std::vector<RowViewData>                   other_plate_rows;
    bool                                       plate_scope_valid = false;
    int                                        current_plate_index = -1;
    int                                        plate_count = 0;
    std::string                                plate_scope_subtitle;
    std::string                                current_plate_title;
    std::string                                current_plate_count_text;
    bool                                       show_other_plates_section = false;
    std::string                                other_plates_title;
    std::string                                other_plates_count_text;
    PreviewViewType                            preview_view = PreviewViewType::Iso;
    bool                                       show_preview_view_switcher = true;
    bool                                       show_preview_rotate_buttons = false;
    bool                                       preview_rotate_left_enabled = false;
    bool                                       preview_rotate_right_enabled = false;
    std::string                                preview_direction_hint;
    bool                                       add_enabled = true;
    float                                      scale = 1.f;
    float                                      child_h = 0.f;
    float                                      preview_w = 0.f;
    float                                      split_gap = 0.f;
};

struct Callbacks {
    std::function<void(int item_index, const std::string& selection_token)> on_select_option;
    std::function<void(int item_index, const std::string& selection_token)> on_hover_option;
    std::function<void(int item_index)>                                     on_request_edit_scene_color;
    std::function<void(int item_index, const ImVec2& anchor_min, const ImVec2& anchor_max)> on_request_edit_scene_material;
    std::function<void(const ImVec2& anchor_min, const ImVec2& anchor_max)> on_request_add;
    std::function<void(int item_index)>                                     on_request_delete_row;
    std::function<void(PreviewViewType view_type)>                          on_change_preview_view;
    std::function<void()>                                                   on_rotate_preview_left;
    std::function<void()>                                                   on_rotate_preview_right;
    std::function<void(float preview_w, float child_h, float scale)>        render_preview;
};

void render_panel(const PanelViewData& view_data, const Callbacks& callbacks);

} // namespace ResidentFilamentMappingView
} // namespace GUI
} // namespace Slic3r
