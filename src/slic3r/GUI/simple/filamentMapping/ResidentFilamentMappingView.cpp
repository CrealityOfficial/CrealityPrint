#include "ResidentFilamentMappingView.hpp"

#include "ImGuiSpoolWidget.hpp"

#include <algorithm>
#include <unordered_map>

namespace Slic3r {
namespace GUI {
namespace ResidentFilamentMappingView {

namespace {

using ResidentFilamentMapping::RowPresentation;
using ResidentFilamentMapping::RowViewModel;
using ResidentFilamentMapping::SummaryTone;
using ResidentFilamentMapping::UiMode;
using ResidentFilamentMapping::UnifiedOutputInput;

static ImU32 col(const ImVec4& v)
{
    return ImGui::GetColorU32(v);
}

static ImVec4 rgba(float r, float g, float b, float a)
{
    return ImVec4(r, g, b, a);
}

static std::unordered_map<std::string, size_t>& popup_group_state()
{
    static std::unordered_map<std::string, size_t> state;
    return state;
}

static bool& other_plates_foldout_open()
{
    static bool open = false;
    return open;
}

static bool use_dark_text(const ImVec4& bg)
{
    const float brightness = (bg.x * 299.f + bg.y * 587.f + bg.z * 114.f) / 1000.f;
    return brightness > 0.62f;
}

static void draw_card(const ImVec2& min, const ImVec2& max, const ImVec4& fill, const ImVec4& border, float rounding, float thickness = 1.f)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(min, max, col(fill), rounding);
    draw_list->AddRect(min, max, col(border), rounding, 0, thickness);
}

static void draw_centered_text(const ImVec2& min, const ImVec2& max, ImU32 text_col, const std::string& text)
{
    const ImVec2 text_size = ImGui::CalcTextSize(text.c_str());
    const ImVec2 pos(min.x + (max.x - min.x - text_size.x) * 0.5f, min.y + (max.y - min.y - text_size.y) * 0.5f);
    ImGui::GetWindowDrawList()->AddText(pos, text_col, text.c_str());
}

static void draw_clipped_text(const ImVec2& min, const ImVec2& max, ImU32 text_col, const std::string& text)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->PushClipRect(min, max, true);
    draw_list->AddText(min, text_col, text.c_str());
    draw_list->PopClipRect();
}

static ImVec4 tone_fill(SummaryTone tone)
{
    switch (tone) {
    case SummaryTone::Good: return rgba(0.12f, 0.42f, 0.24f, 0.96f);
    case SummaryTone::Offline: return rgba(0.40f, 0.22f, 0.22f, 0.96f);
    case SummaryTone::Warn:
    default: return rgba(0.42f, 0.33f, 0.12f, 0.96f);
    }
}

static ImVec4 tone_banner_fill(SummaryTone tone)
{
    switch (tone) {
    case SummaryTone::Good: return rgba(0.10f, 0.24f, 0.17f, 0.94f);
    case SummaryTone::Offline: return rgba(0.27f, 0.15f, 0.15f, 0.94f);
    case SummaryTone::Warn:
    default: return rgba(0.27f, 0.22f, 0.12f, 0.94f);
    }
}

struct StatusBadgeVisual {
    const char* label = "";
    ImVec4      fill  = rgba(1.f, 1.f, 1.f, 0.08f);
    ImVec4      text  = rgba(0.88f, 0.92f, 0.96f, 1.f);
};
static float rows_scroll_height(size_t row_count, float scale)
{
    if (row_count == 0)
        return 0.f;
    const float row_h = 72.f * scale;
    const float gap_h = 7.f * scale;
    return row_count * row_h + (row_count - 1) * gap_h + 2.f * scale;
}
static StatusBadgeVisual row_status_badge(const RowViewModel& row)
{
    if (row.presentation == RowPresentation::UnifiedOutput)
        return { u8"\u7edf\u4e00\u8f93\u51fa", rgba(0.19f, 0.33f, 0.52f, 0.92f), rgba(0.90f, 0.95f, 1.f, 1.f) };

    if (row.presentation == RowPresentation::DisabledSelector) {
        if (row.using_cached_target)
            return { u8"\u4ec5\u4f9b\u53c2\u8003", rgba(0.35f, 0.29f, 0.13f, 0.92f), rgba(1.f, 0.92f, 0.75f, 1.f) };
        return { u8"\u79bb\u7ebf", rgba(0.34f, 0.18f, 0.18f, 0.92f), rgba(1.f, 0.88f, 0.88f, 1.f) };
    }

    if (row.selector_placeholder)
        return { u8"\u5f85\u6620\u5c04", rgba(0.35f, 0.29f, 0.13f, 0.92f), rgba(1.f, 0.92f, 0.75f, 1.f) };

    return { u8"\u5df2\u6620\u5c04", rgba(0.11f, 0.38f, 0.22f, 0.92f), rgba(0.89f, 0.98f, 0.92f, 1.f) };
}

static void draw_status_badge(const ImVec2& pos, float scale, const StatusBadgeVisual& visual)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 text_size = ImGui::CalcTextSize(visual.label);
    const ImVec2 size(std::max(72.f * scale, text_size.x + 18.f * scale), 28.f * scale);
    draw_list->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), col(visual.fill), 999.f);
    draw_list->AddText(ImVec2(pos.x + (size.x - text_size.x) * 0.5f, pos.y + (size.y - text_size.y) * 0.5f), col(visual.text), visual.label);
}

static void render_summary_card(const ResidentFilamentMapping::SummaryViewModel& summary, float scale)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 top_left = ImGui::GetCursorScreenPos();
    const float content_w = ImGui::GetContentRegionAvail().x;
    const ImVec2 size(content_w, 94.f * scale);
    const ImVec2 bottom_right(top_left.x + size.x, top_left.y + size.y);

    draw_card(top_left, bottom_right, rgba(0.09f, 0.11f, 0.15f, 0.98f), rgba(1.f, 1.f, 1.f, 0.08f), 20.f * scale);

    draw_list->AddText(ImVec2(top_left.x + 16.f * scale, top_left.y + 14.f * scale), IM_COL32(245, 248, 252, 255), u8"\u573a\u666f\u8017\u6750\u6620\u5c04");

    const ImVec2 pill_size(84.f * scale, 30.f * scale);
    const ImVec2 pill_pos(bottom_right.x - pill_size.x - 14.f * scale, top_left.y + 12.f * scale);
    draw_list->AddRectFilled(pill_pos, ImVec2(pill_pos.x + pill_size.x, pill_pos.y + pill_size.y), col(tone_fill(summary.tone)), 999.f);
    draw_centered_text(pill_pos, ImVec2(pill_pos.x + pill_size.x, pill_pos.y + pill_size.y), IM_COL32(246, 250, 252, 255), summary.pill_text);

    draw_list->AddText(ImVec2(top_left.x + 16.f * scale, top_left.y + 38.f * scale), IM_COL32(184, 193, 204, 255), summary.subtitle.c_str());

    const ImVec2 banner_pos(top_left.x + 16.f * scale, top_left.y + 56.f * scale);
    const ImVec2 banner_size(size.x - 32.f * scale, 24.f * scale);
    draw_list->AddRectFilled(banner_pos, ImVec2(banner_pos.x + banner_size.x, banner_pos.y + banner_size.y), col(tone_banner_fill(summary.tone)), 12.f * scale);
    draw_list->AddText(ImVec2(banner_pos.x + 10.f * scale, banner_pos.y + 5.f * scale), IM_COL32(233, 238, 244, 255), summary.banner_text.c_str());

    ImGui::Dummy(size);
}

static void render_unified_output_card(const UnifiedOutputInput& unified, float scale)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 top_left = ImGui::GetCursorScreenPos();
    const float content_w = ImGui::GetContentRegionAvail().x;
    const ImVec2 size(content_w, 74.f * scale);
    const ImVec2 bottom_right(top_left.x + size.x, top_left.y + size.y);

    draw_card(top_left, bottom_right, rgba(1.f, 1.f, 1.f, 0.045f), rgba(1.f, 1.f, 1.f, 0.07f), 18.f * scale);

    draw_spool_widget(draw_list,
                      ImVec2(top_left.x + 12.f * scale, top_left.y + 9.f * scale),
                      ImVec2(68.f * scale, 54.f * scale),
                      ImGuiSpoolVisualSpec{ unified.material_color, unified.valid ? 1.f : 0.55f });

    const std::string title = unified.valid ? (unified.slot_label + " / " + unified.material_type) : std::string(u8"\u672a\u8bfb\u53d6\u5230\u7edf\u4e00\u8f93\u51fa\u8017\u6750");
    const std::string subtitle = unified.valid
        ? std::string(u8"\u5f53\u524d\u6240\u6709\u573a\u666f\u989c\u8272\u5c06\u7edf\u4e00\u4f7f\u7528\u8be5\u8017\u6750")
        : std::string(u8"\u8bf7\u5148\u8fde\u63a5\u8bbe\u5907\u5e76\u8bfb\u53d6\u53ef\u7528\u8017\u6750");

    draw_list->AddText(ImVec2(top_left.x + 90.f * scale, top_left.y + 16.f * scale), IM_COL32(245, 248, 252, 255), title.c_str());
    draw_list->AddText(ImVec2(top_left.x + 90.f * scale, top_left.y + 38.f * scale), IM_COL32(188, 197, 207, 255), subtitle.c_str());

    ImGui::Dummy(size);
}

struct SceneTileResult {
    bool   color_clicked = false;
    bool   material_clicked = false;
    ImVec2 material_anchor_min = ImVec2(0.f, 0.f);
    ImVec2 material_anchor_max = ImVec2(0.f, 0.f);
};

static SceneTileResult draw_scene_tile(const RowViewModel& row, const ImVec2& pos, const ImVec2& size, float scale)
{
    SceneTileResult result;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    const float gap = 4.f * scale;
    const float material_h = 18.f * scale;
    const float color_h = std::max(28.f * scale, size.y - material_h - gap);

    const ImVec2 color_min = pos;
    const ImVec2 color_max(pos.x + size.x, pos.y + color_h);
    const ImVec2 material_min(pos.x, color_max.y + gap);
    const ImVec2 material_max(pos.x + size.x, pos.y + size.y);

    ImGui::SetCursorScreenPos(color_min);
    ImGui::InvisibleButton("##scene_color_tile", ImVec2(size.x, color_h));
    const bool color_hovered = ImGui::IsItemHovered();
    result.color_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);

    const ImVec4 base = row.scene_color;
    const ImVec4 top_tint(std::min(base.x + 0.12f, 1.f), std::min(base.y + 0.12f, 1.f), std::min(base.z + 0.12f, 1.f), 1.f);
    draw_list->AddRectFilledMultiColor(color_min, color_max, col(top_tint), col(top_tint), col(base), col(base));
    draw_list->AddRect(color_min,
                       color_max,
                       color_hovered ? IM_COL32(255, 255, 255, 74) : IM_COL32(255, 255, 255, 30),
                       14.f * scale,
                       0,
                       color_hovered ? 1.8f : 1.0f);

    const bool dark_text = use_dark_text(base);
    const ImU32 badge_fill = dark_text ? IM_COL32(255, 255, 255, 90) : IM_COL32(0, 0, 0, 58);
    const ImU32 badge_text = dark_text ? IM_COL32(18, 24, 28, 255) : IM_COL32(248, 250, 252, 255);
    const ImVec2 badge_pos(color_min.x + 8.f * scale, color_min.y + 7.f * scale);
    const ImVec2 badge_size(20.f * scale, 18.f * scale);
    draw_list->AddRectFilled(badge_pos,
                             ImVec2(badge_pos.x + badge_size.x, badge_pos.y + badge_size.y),
                             badge_fill,
                             6.f * scale);
    draw_centered_text(badge_pos,
                       ImVec2(badge_pos.x + badge_size.x, badge_pos.y + badge_size.y),
                       badge_text,
                       std::to_string(row.item_index + 1));

    ImGui::SetCursorScreenPos(material_min);
    ImGui::InvisibleButton("##scene_material_tile", ImVec2(size.x, material_h));
    const bool material_hovered = ImGui::IsItemHovered();
    result.material_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    result.material_anchor_min = material_min;
    result.material_anchor_max = material_max;

    draw_list->AddRectFilled(material_min,
                             material_max,
                             material_hovered ? IM_COL32(255, 255, 255, 18) : IM_COL32(255, 255, 255, 10),
                             10.f * scale);
    draw_list->AddRect(material_min,
                       material_max,
                       material_hovered ? IM_COL32(255, 255, 255, 42) : IM_COL32(255, 255, 255, 14),
                       10.f * scale,
                       0,
                       material_hovered ? 1.4f : 1.0f);

    const std::string material_text = row.scene_material_type.empty() ? std::string("Hyper PLA") : row.scene_material_type;
    const float chevron_reserve = 14.f * scale;
    const ImVec2 text_min(material_min.x + 8.f * scale, material_min.y + 3.f * scale);
    const ImVec2 text_max(material_max.x - chevron_reserve, material_max.y - 2.f * scale);
    draw_clipped_text(text_min, text_max, IM_COL32(231, 236, 241, 255), material_text);

    const ImVec2 chevron_center(material_max.x - 8.f * scale, material_min.y + material_h * 0.5f + 0.5f * scale);
    const float len = 3.2f * scale;
    const ImU32 chev_col = material_hovered ? IM_COL32(248, 251, 255, 255) : IM_COL32(198, 206, 214, 235);
    draw_list->AddLine(ImVec2(chevron_center.x - len, chevron_center.y - len * 0.45f),
                       ImVec2(chevron_center.x, chevron_center.y + len * 0.55f),
                       chev_col,
                       1.5f);
    draw_list->AddLine(ImVec2(chevron_center.x, chevron_center.y + len * 0.55f),
                       ImVec2(chevron_center.x + len, chevron_center.y - len * 0.45f),
                       chev_col,
                       1.5f);

    return result;
}

static void draw_arrow_glyph(const ImVec2& center, float scale)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const float len = 8.f * scale;
    const float half_h = 5.f * scale;
    const ImU32 arrow_col = IM_COL32(176, 186, 197, 255);

    draw_list->AddLine(ImVec2(center.x - len, center.y), ImVec2(center.x + len - 4.f * scale, center.y), arrow_col, 2.5f);
    draw_list->AddLine(ImVec2(center.x + len - 9.f * scale, center.y - half_h), ImVec2(center.x + len - 1.f * scale, center.y), arrow_col, 2.5f);
    draw_list->AddLine(ImVec2(center.x + len - 9.f * scale, center.y + half_h), ImVec2(center.x + len - 1.f * scale, center.y), arrow_col, 2.5f);
}

static bool draw_header_add_button(bool enabled, float scale, const ImVec2& pos, const ImVec2& size)
{
    ImGui::SetCursorScreenPos(pos);
    ImGui::InvisibleButton("##resident_add_color", size);
    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = enabled && ImGui::IsItemClicked(ImGuiMouseButton_Left);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 max(pos.x + size.x, pos.y + size.y);
    const ImU32 shadow = enabled
        ? ImGui::GetColorU32(hovered ? rgba(0.17f, 0.42f, 0.27f, 0.20f) : rgba(0.14f, 0.34f, 0.23f, 0.12f))
        : ImGui::GetColorU32(rgba(0.f, 0.f, 0.f, 0.f));
    const ImU32 fill = enabled
        ? ImGui::GetColorU32(hovered ? rgba(0.13f, 0.27f, 0.20f, 0.98f) : rgba(0.10f, 0.20f, 0.16f, 0.94f))
        : ImGui::GetColorU32(rgba(0.16f, 0.18f, 0.21f, 0.74f));
    const ImU32 border = enabled
        ? ImGui::GetColorU32(hovered ? rgba(0.34f, 0.86f, 0.54f, 0.78f) : rgba(0.28f, 0.72f, 0.47f, 0.32f))
        : ImGui::GetColorU32(rgba(1.f, 1.f, 1.f, 0.08f));
    const ImU32 text_col = enabled
        ? (hovered ? IM_COL32(238, 248, 242, 255) : IM_COL32(220, 241, 229, 255))
        : IM_COL32(128, 137, 145, 255);

    draw_list->AddRectFilled(ImVec2(pos.x, pos.y + 2.f * scale), ImVec2(max.x, max.y + 2.f * scale), shadow, 999.f);
    draw_list->AddRectFilled(pos, max, fill, 999.f);
    draw_list->AddRect(pos, max, border, 999.f, 0, hovered && enabled ? 1.8f : 1.2f);

    const float plus_size = 8.f * scale;
    const ImVec2 plus_center(pos.x + 15.f * scale, pos.y + size.y * 0.5f);
    draw_list->AddLine(ImVec2(plus_center.x - plus_size * 0.5f, plus_center.y),
                       ImVec2(plus_center.x + plus_size * 0.5f, plus_center.y),
                       text_col,
                       hovered && enabled ? 2.0f : 1.7f);
    draw_list->AddLine(ImVec2(plus_center.x, plus_center.y - plus_size * 0.5f),
                       ImVec2(plus_center.x, plus_center.y + plus_size * 0.5f),
                       text_col,
                       hovered && enabled ? 2.0f : 1.7f);

    const char* label = u8"\u6dfb\u52a0\u989c\u8272";
    draw_list->AddText(ImVec2(pos.x + 26.f * scale, pos.y + (size.y - ImGui::CalcTextSize(label).y) * 0.5f - 0.5f * scale), text_col, label);

    if (!enabled && hovered)
        ImGui::SetTooltip("%s", u8"\u5df2\u8fbe\u5230\u53ef\u6dfb\u52a0\u7684\u989c\u8272\u4e0a\u9650");

    return clicked;
}

static bool draw_row_delete_button(const std::string& widget_id, bool enabled, float scale, const ImVec2& pos, const ImVec2& size)
{
    ImGui::SetCursorScreenPos(pos);
    ImGui::InvisibleButton(widget_id.c_str(), size);
    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = enabled && ImGui::IsItemClicked(ImGuiMouseButton_Left);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 max(pos.x + size.x, pos.y + size.y);
    const ImU32 shadow = enabled
        ? ImGui::GetColorU32(hovered ? rgba(0.34f, 0.12f, 0.12f, 0.16f) : rgba(0.f, 0.f, 0.f, 0.08f))
        : ImGui::GetColorU32(rgba(0.f, 0.f, 0.f, 0.f));
    const ImU32 fill = enabled
        ? ImGui::GetColorU32(hovered ? rgba(0.35f, 0.15f, 0.15f, 0.96f) : rgba(1.f, 1.f, 1.f, 0.04f))
        : ImGui::GetColorU32(rgba(1.f, 1.f, 1.f, 0.02f));
    const ImU32 border = enabled
        ? ImGui::GetColorU32(hovered ? rgba(1.f, 0.50f, 0.50f, 0.56f) : rgba(1.f, 1.f, 1.f, 0.08f))
        : ImGui::GetColorU32(rgba(1.f, 1.f, 1.f, 0.05f));
    const ImU32 icon_col = enabled
        ? (hovered ? IM_COL32(255, 223, 223, 255) : IM_COL32(214, 220, 227, 228))
        : IM_COL32(96, 102, 110, 180);
    const ImU32 text_col = enabled
        ? (hovered ? IM_COL32(255, 223, 223, 255) : IM_COL32(192, 200, 208, 220))
        : IM_COL32(96, 102, 110, 180);

    draw_list->AddRectFilled(ImVec2(pos.x, pos.y + 1.5f * scale), ImVec2(max.x, max.y + 1.5f * scale), shadow, 999.f);
    draw_list->AddRectFilled(pos, max, fill, 999.f);
    draw_list->AddRect(pos, max, border, 999.f, 0, hovered && enabled ? 1.6f : 1.f);

    const float icon_left = pos.x + 9.f * scale;
    const float top = pos.y + 6.5f * scale;
    const float can_w = 8.f * scale;
    const float can_h = 8.f * scale;
    draw_list->AddRect(ImVec2(icon_left, top + 3.f * scale),
                       ImVec2(icon_left + can_w, top + 3.f * scale + can_h),
                       icon_col,
                       2.f * scale,
                       0,
                       hovered && enabled ? 1.5f : 1.3f);
    draw_list->AddLine(ImVec2(icon_left - 2.f * scale, top + 3.f * scale),
                       ImVec2(icon_left + can_w + 2.f * scale, top + 3.f * scale),
                       icon_col,
                       hovered && enabled ? 1.5f : 1.3f);
    draw_list->AddLine(ImVec2(icon_left + 1.5f * scale, top + 1.5f * scale),
                       ImVec2(icon_left + can_w - 1.5f * scale, top + 1.5f * scale),
                       icon_col,
                       hovered && enabled ? 1.5f : 1.3f);
    draw_list->AddLine(ImVec2(icon_left + 2.6f * scale, top),
                       ImVec2(icon_left + can_w - 2.6f * scale, top),
                       icon_col,
                       hovered && enabled ? 1.5f : 1.3f);

    const char* label = u8"\u5220\u9664";
    draw_list->AddText(ImVec2(pos.x + 24.f * scale, pos.y + (size.y - ImGui::CalcTextSize(label).y) * 0.5f - 0.5f * scale), text_col, label);

    if (hovered)
        ImGui::SetTooltip("%s", enabled ? u8"\u5220\u9664\u8be5\u989c\u8272" : u8"\u81f3\u5c11\u4fdd\u7559 1 \u4e2a\u573a\u666f\u989c\u8272");

    return clicked;
}

static bool draw_preview_view_chip(const char* widget_id,
                                   const char* label,
                                   PreviewViewType chip_view,
                                   PreviewViewType selected_view,
                                   float scale,
                                   const ImVec2& pos,
                                   const ImVec2& size)
{
    ImGui::SetCursorScreenPos(pos);
    ImGui::InvisibleButton(widget_id, size);
    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    const bool active = chip_view == selected_view;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 max(pos.x + size.x, pos.y + size.y);
    const ImU32 fill = active
        ? ImGui::GetColorU32(hovered ? rgba(0.24f, 0.33f, 0.49f, 0.98f) : rgba(0.20f, 0.29f, 0.44f, 0.96f))
        : ImGui::GetColorU32(hovered ? rgba(1.f, 1.f, 1.f, 0.10f) : rgba(1.f, 1.f, 1.f, 0.045f));
    const ImU32 border = active
        ? ImGui::GetColorU32(hovered ? rgba(0.65f, 0.79f, 1.f, 0.82f) : rgba(0.54f, 0.70f, 0.94f, 0.56f))
        : ImGui::GetColorU32(hovered ? rgba(1.f, 1.f, 1.f, 0.24f) : rgba(1.f, 1.f, 1.f, 0.10f));
    const ImU32 text_col = active
        ? IM_COL32(242, 247, 252, 255)
        : (hovered ? IM_COL32(230, 236, 243, 255) : IM_COL32(182, 191, 201, 255));
    draw_list->AddRectFilled(pos, max, fill, 999.f);
    draw_list->AddRect(pos, max, border, 999.f, 0, active ? 1.6f : 1.0f);
    draw_centered_text(pos, max, text_col, label);
    return clicked;
}
static bool draw_selector_card_widget(const RowViewModel& row, const ImVec2& size, float scale, bool popup_open)
{
    const ImVec2 top_left = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##selector", size);
    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = row.selector_enabled && ImGui::IsItemClicked(ImGuiMouseButton_Left);
    const ImVec2 bottom_right = ImGui::GetItemRectMax();

    const bool emphasized = row.selector_enabled && (hovered || popup_open);
    const ImU32 border = emphasized ? IM_COL32(255, 191, 71, 220) : IM_COL32(255, 255, 255, row.selector_enabled ? 26 : 16);
    const ImU32 fill = col(row.selector_enabled ? rgba(0.20f, 0.23f, 0.28f, 0.98f) : rgba(0.18f, 0.20f, 0.24f, 0.72f));
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(top_left, bottom_right, fill, 18.f * scale);
    draw_list->AddRect(top_left, bottom_right, border, 18.f * scale, 0, emphasized ? 2.f : 1.f);

    draw_spool_widget(draw_list,
                      ImVec2(top_left.x + 2.f * scale, top_left.y + 1.f * scale),
                      ImVec2(76.f * scale, 62.f * scale),
                      ImGuiSpoolVisualSpec{ row.target_material_color, row.selector_enabled ? 1.f : 0.58f });

    const bool placeholder = row.selector_placeholder;
    const std::string slot_label = row.target_slot_label.empty() ? "--" : row.target_slot_label;
    const float caret_reserve = row.selector_show_chevron ? 20.f * scale : 0.f;
    const float meta_x = top_left.x + 80.f * scale;
    const float meta_right = bottom_right.x - 8.f * scale - caret_reserve;
    const ImVec2 pill_pos(meta_x, top_left.y + (placeholder ? 9.f : 10.f) * scale);
    const float desired_pill_w = std::max(38.f * scale, ImGui::CalcTextSize(slot_label.c_str()).x + 16.f * scale);
    const ImVec2 pill_size(std::min(desired_pill_w, std::max(32.f * scale, meta_right - meta_x)), 22.f * scale);
    const ImU32 pill_fill = placeholder ? IM_COL32(255, 191, 71, 24) : IM_COL32(255, 255, 255, 22);
    const ImU32 pill_text = placeholder ? IM_COL32(252, 229, 184, 255) : IM_COL32(236, 241, 246, 255);
    draw_list->AddRectFilled(pill_pos, ImVec2(pill_pos.x + pill_size.x, pill_pos.y + pill_size.y), pill_fill, 999.f);
    draw_centered_text(pill_pos, ImVec2(pill_pos.x + pill_size.x, pill_pos.y + pill_size.y), pill_text, slot_label);

    const ImVec2 text_min(meta_x + 1.f * scale, top_left.y + 38.f * scale);
    const ImVec2 text_max(meta_right, top_left.y + size.y - 8.f * scale);
    draw_clipped_text(text_min,
                      text_max,
                      placeholder ? IM_COL32(219, 207, 179, 255) : IM_COL32(190, 199, 208, 255),
                      row.target_material_type);

    if (row.selector_show_chevron) {
        const ImVec2 chevron_center(bottom_right.x - 11.f * scale, top_left.y + size.y * 0.5f);
        const float len = 3.8f * scale;
        draw_list->AddLine(ImVec2(chevron_center.x - len, chevron_center.y - len * 0.4f), ImVec2(chevron_center.x, chevron_center.y + len * 0.6f), IM_COL32(212, 220, 228, emphasized ? 255 : 224), 1.8f);
        draw_list->AddLine(ImVec2(chevron_center.x, chevron_center.y + len * 0.6f), ImVec2(chevron_center.x + len, chevron_center.y - len * 0.4f), IM_COL32(212, 220, 228, emphasized ? 255 : 224), 1.8f);
    }

    return clicked;
}

struct PopupCandidateCardResult {
    bool clicked = false;
    bool hovered = false;
};

static PopupCandidateCardResult draw_popup_candidate_card(const PopupOptionViewModel& option, float scale)
{
    const ImVec2 size(136.f * scale, 110.f * scale);
    ImGui::InvisibleButton(option.widget_id.c_str(), size);
    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    const ImVec2 rmin = ImGui::GetItemRectMin();
    const ImVec2 rmax = ImGui::GetItemRectMax();

    const ImVec4 fill = option.disabled ? rgba(0.18f, 0.20f, 0.23f, 0.78f) : rgba(0.19f, 0.21f, 0.26f, 0.99f);
    const ImU32 border = option.selected
        ? IM_COL32(255, 191, 71, 230)
        : (hovered && !option.disabled ? IM_COL32(255, 255, 255, 40) : IM_COL32(255, 255, 255, 18));

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    if (option.selected)
        draw_list->AddRectFilled(ImVec2(rmin.x - 1.f * scale, rmin.y - 1.f * scale), ImVec2(rmax.x + 1.f * scale, rmax.y + 1.f * scale), IM_COL32(255, 191, 71, 20), 18.f * scale);
    draw_list->AddRectFilled(rmin, rmax, col(fill), 16.f * scale);
    draw_list->AddRect(rmin, rmax, border, 16.f * scale, 0, option.selected || (hovered && !option.disabled) ? 2.f : 1.f);

    draw_spool_widget(draw_list,
                      ImVec2(rmin.x + 20.f * scale, rmin.y + 8.f * scale),
                      ImVec2(90.f * scale, 68.f * scale),
                      ImGuiSpoolVisualSpec{ option.disabled ? rgba(0.50f, 0.53f, 0.58f, 1.f) : option.material_color, option.disabled ? 0.62f : 1.f });

    const ImVec2 slot_pos(rmin.x + 12.f * scale, rmax.y - 28.f * scale);
    const ImVec2 slot_size(42.f * scale, 20.f * scale);
    draw_list->AddRectFilled(slot_pos, ImVec2(slot_pos.x + slot_size.x, slot_pos.y + slot_size.y), IM_COL32(255, 255, 255, 14), 999.f);
    draw_centered_text(slot_pos, ImVec2(slot_pos.x + slot_size.x, slot_pos.y + slot_size.y), IM_COL32(235, 240, 245, 255), option.slot_label);
    draw_list->AddText(ImVec2(rmin.x + 62.f * scale, rmax.y - 25.f * scale), IM_COL32(190, 199, 208, 255), option.material_label.c_str());

    PopupCandidateCardResult result;
    result.clicked = clicked && !option.disabled;
    result.hovered = hovered && !option.disabled;
    return result;
}

static void draw_popup_scene_badge(const RowViewModel& row, const ImVec2& pos, const ImVec2& size, float scale)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 max(pos.x + size.x, pos.y + size.y);
    const ImVec4 base = row.scene_color;
    const ImVec4 top_tint(std::min(base.x + 0.12f, 1.f), std::min(base.y + 0.12f, 1.f), std::min(base.z + 0.12f, 1.f), 1.f);
    draw_list->AddRectFilledMultiColor(pos, max, col(top_tint), col(top_tint), col(base), col(base));
    draw_list->AddRect(pos, max, IM_COL32(255, 255, 255, 34), 12.f * scale, 0, 1.0f);

    const bool dark_text = use_dark_text(base);
    const ImU32 badge_fill = dark_text ? IM_COL32(255, 255, 255, 96) : IM_COL32(0, 0, 0, 56);
    const ImU32 badge_text = dark_text ? IM_COL32(18, 24, 28, 255) : IM_COL32(248, 250, 252, 255);
    const ImVec2 badge_pos(pos.x + 7.f * scale, pos.y + 5.f * scale);
    const ImVec2 badge_size(20.f * scale, 18.f * scale);
    draw_list->AddRectFilled(badge_pos,
                             ImVec2(badge_pos.x + badge_size.x, badge_pos.y + badge_size.y),
                             badge_fill,
                             6.f * scale);
    draw_centered_text(badge_pos,
                       ImVec2(badge_pos.x + badge_size.x, badge_pos.y + badge_size.y),
                       badge_text,
                       std::to_string(row.item_index + 1));
}

static void render_popup_header(const RowViewModel& row, float scale)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 top_left = ImGui::GetCursorScreenPos();
    const float content_w = ImGui::GetContentRegionAvail().x;
    const ImVec2 size(content_w, 64.f * scale);
    const ImVec2 bottom_right(top_left.x + size.x, top_left.y + size.y);

    draw_card(top_left, bottom_right, rgba(1.f, 1.f, 1.f, 0.045f), rgba(1.f, 1.f, 1.f, 0.07f), 14.f * scale);
    draw_list->AddText(ImVec2(top_left.x + 12.f * scale, top_left.y + 10.f * scale),
                       IM_COL32(243, 247, 251, 255),
                       u8"\u9009\u62e9\u6620\u5c04\u76ee\u6807");

    const std::string subtitle = row.scene_material_type.empty() ? row.scene_label : row.scene_material_type;
    draw_list->AddText(ImVec2(top_left.x + 12.f * scale, top_left.y + 31.f * scale),
                       IM_COL32(184, 193, 203, 255),
                       subtitle.c_str());

    const ImVec2 swatch_pos(bottom_right.x - 74.f * scale, top_left.y + 12.f * scale);
    draw_popup_scene_badge(row, swatch_pos, ImVec2(62.f * scale, 28.f * scale), scale);

    ImGui::Dummy(size);
}

static size_t selected_popup_group_index(const RowViewData& row_data)
{
    if (row_data.popup_groups.empty())
        return 0;

    for (size_t group_index = 0; group_index < row_data.popup_groups.size(); ++group_index) {
        const PopupGroupViewModel& group = row_data.popup_groups[group_index];
        const bool has_selected = std::any_of(group.options.begin(), group.options.end(), [](const PopupOptionViewModel& option) {
            return option.selected;
        });
        if (has_selected)
            return group_index;
    }

    return 0;
}

static size_t popup_group_selection_for_render(const RowViewData& row_data)
{
    auto& state = popup_group_state();
    const auto it = state.find(row_data.popup_id);
    const size_t default_index = selected_popup_group_index(row_data);
    if (it == state.end() || it->second >= row_data.popup_groups.size()) {
        state[row_data.popup_id] = default_index;
        return default_index;
    }
    return it->second;
}

static void set_popup_group_selection(const RowViewData& row_data, size_t group_index)
{
    popup_group_state()[row_data.popup_id] = group_index;
}

static size_t render_popup_group_chips(const RowViewData& row_data, float scale)
{
    size_t selected_index = popup_group_selection_for_render(row_data);

    for (size_t i = 0; i < row_data.popup_groups.size(); ++i) {
        const PopupGroupViewModel& group = row_data.popup_groups[i];
        if (i > 0)
            ImGui::SameLine(0.f, 8.f * scale);

        ImGui::PushID(static_cast<int>(i));
        const ImVec2 chip_text_size = ImGui::CalcTextSize(group.label.c_str());
        const ImVec2 chip_size(std::max(58.f * scale, chip_text_size.x + 20.f * scale), 28.f * scale);
        ImGui::InvisibleButton("##popup_group_chip", chip_size);

        const bool hovered = ImGui::IsItemHovered();
        const bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
        const ImVec2 cmin = ImGui::GetItemRectMin();
        const ImVec2 cmax = ImGui::GetItemRectMax();
        const bool active = i == selected_index;

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(cmin, cmax,
                                 active ? IM_COL32(255, 191, 71, 30) : IM_COL32(255, 255, 255, hovered ? 18 : 10),
                                 999.f);
        draw_list->AddRect(cmin, cmax,
                           active ? IM_COL32(255, 191, 71, 210) : IM_COL32(255, 255, 255, hovered ? 38 : 18),
                           999.f,
                           0,
                           active ? 2.f : 1.f);
        draw_centered_text(cmin, cmax,
                           active ? IM_COL32(247, 238, 214, 255) : IM_COL32(188, 197, 207, 255),
                           group.label);

        if (clicked) {
            selected_index = i;
            set_popup_group_selection(row_data, i);
        }
        ImGui::PopID();
    }

    return selected_index;
}

static void render_popup_content(const RowViewData& row_data, float scale, const Callbacks& callbacks)
{
    render_popup_header(row_data.row, scale);
    ImGui::Dummy(ImVec2(0.f, 10.f * scale));

    if (row_data.popup_groups.empty()) {
        if (callbacks.on_hover_option)
            callbacks.on_hover_option(row_data.row.item_index, std::string());
        ImGui::TextUnformatted(u8"\u672a\u8bfb\u53d6\u5230\u53ef\u7528\u8bbe\u5907\u8017\u6750");
        return;
    }

    const size_t selected_group_index = render_popup_group_chips(row_data, scale);
    ImGui::Dummy(ImVec2(0.f, 10.f * scale));

    const PopupGroupViewModel& group = row_data.popup_groups[std::min(selected_group_index, row_data.popup_groups.size() - 1)];
    const float content_h = 248.f * scale;
    bool any_hovered_option = false;
    ImGui::BeginChild("popup_group_content", ImVec2(0.f, content_h), false, ImGuiWindowFlags_NoScrollWithMouse);
    for (size_t i = 0; i < group.options.size(); ++i) {
        if (i > 0 && (i % 2) != 0)
            ImGui::SameLine(0.f, 12.f * scale);

        const PopupCandidateCardResult card_result = draw_popup_candidate_card(group.options[i], scale);
        if (card_result.hovered) {
            any_hovered_option = true;
            if (callbacks.on_hover_option)
                callbacks.on_hover_option(row_data.row.item_index, group.options[i].selection_token);
        }

        if (card_result.clicked && callbacks.on_select_option) {
            callbacks.on_select_option(row_data.row.item_index, group.options[i].selection_token);
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::EndChild();

    if (!any_hovered_option && callbacks.on_hover_option)
        callbacks.on_hover_option(row_data.row.item_index, std::string());
}

static void render_row(const RowViewData& row_data, UiMode mode, float scale, bool can_delete_rows, const Callbacks& callbacks)
{
    const RowViewModel& row = row_data.row;
    const StatusBadgeVisual badge = row_status_badge(row);
    ImGui::PushID(row.item_index);
    const float row_h = 72.f * scale;
    const float content_w = ImGui::GetContentRegionAvail().x;
    const bool compact = content_w < 372.f * scale;
    const ImVec2 row_size(content_w, row_h);
    const ImVec2 row_pos = ImGui::GetCursorScreenPos();
    ImGui::Dummy(row_size);
    const ImVec2 row_br(row_pos.x + row_size.x, row_pos.y + row_size.y);
    draw_card(row_pos, row_br, rgba(1.f, 1.f, 1.f, 0.042f), rgba(1.f, 1.f, 1.f, 0.055f), 18.f * scale);
    const ImVec2 delete_btn_size((compact ? 48.f : 54.f) * scale, 22.f * scale);
    const ImVec2 delete_btn_pos(row_br.x - delete_btn_size.x - 10.f * scale, row_pos.y + 7.f * scale);
    if (draw_row_delete_button("##row_delete_btn", can_delete_rows, scale, delete_btn_pos, delete_btn_size) &&
        callbacks.on_request_delete_row)
        callbacks.on_request_delete_row(row.item_index);
    const ImVec2 scene_pos(row_pos.x + 12.f * scale, row_pos.y + 8.f * scale);
    const ImVec2 scene_size((compact ? 80.f : 88.f) * scale, 54.f * scale);
    const SceneTileResult scene_tile_result = draw_scene_tile(row, scene_pos, scene_size, scale);
    if (scene_tile_result.color_clicked && callbacks.on_request_edit_scene_color)
        callbacks.on_request_edit_scene_color(row.item_index);
    if (scene_tile_result.material_clicked && callbacks.on_request_edit_scene_material)
        callbacks.on_request_edit_scene_material(row.item_index,
                                                 scene_tile_result.material_anchor_min,
                                                 scene_tile_result.material_anchor_max);
    const float badge_w = std::max(58.f * scale, ImGui::CalcTextSize(badge.label).x + 16.f * scale);
    const ImVec2 badge_pos(row_br.x - badge_w - 10.f * scale, row_br.y - 10.f * scale - 28.f * scale);
    draw_status_badge(badge_pos, scale, badge);
    const ImVec2 arrow_center(scene_pos.x + scene_size.x + 15.f * scale, row_pos.y + row_h * 0.5f);
    draw_arrow_glyph(arrow_center, scale);
    const float selector_min_w = compact ? 138.f * scale : 154.f * scale;
    const float selector_max_w = compact ? 150.f * scale : 162.f * scale;
    const float selector_w = std::min(selector_max_w, std::max(selector_min_w, content_w * (compact ? 0.32f : 0.35f)));
    const ImVec2 selector_size(selector_w, 60.f * scale);
    const ImVec2 selector_pos(badge_pos.x - selector_w - 10.f * scale, row_pos.y + 6.f * scale);

    ImGui::SetCursorScreenPos(selector_pos);
    const bool popup_open = ImGui::IsPopupOpen(row_data.popup_id.c_str());
    const bool clicked = draw_selector_card_widget(row, selector_size, scale, popup_open);
    if (mode == UiMode::MultiColorOnline && clicked && row.selector_enabled && !popup_open) {
        set_popup_group_selection(row_data, selected_popup_group_index(row_data));
        ImGui::OpenPopup(row_data.popup_id.c_str());
    }

    if (mode == UiMode::MultiColorOnline) {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 popup_pos(selector_pos.x + selector_size.x + 12.f * scale, selector_pos.y - 2.f * scale);
        const float estimated_popup_w = 322.f * scale;
        const float estimated_popup_h = 348.f * scale;
        if (popup_pos.x + estimated_popup_w > viewport->WorkPos.x + viewport->WorkSize.x)
            popup_pos.x = selector_pos.x - estimated_popup_w - 8.f * scale;
        if (popup_pos.y + estimated_popup_h > viewport->WorkPos.y + viewport->WorkSize.y)
            popup_pos.y = std::max(viewport->WorkPos.y + 8.f * scale, selector_pos.y + selector_size.y - estimated_popup_h);

        ImGui::SetNextWindowPos(popup_pos, ImGuiCond_Appearing);
        ImGui::SetNextWindowSize(ImVec2(estimated_popup_w, 0.f), ImGuiCond_Appearing);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 18.f * scale);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.f * scale, 12.f * scale));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.f * scale, 9.f * scale));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, rgba(0.10f, 0.12f, 0.15f, 0.99f));
        ImGui::PushStyleColor(ImGuiCol_Border, rgba(1.f, 1.f, 1.f, 0.08f));
        if (ImGui::BeginPopup(row_data.popup_id.c_str())) {
            render_popup_content(row_data, scale, callbacks);
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
    }

    ImGui::PopID();
}

static void render_rows_scroll(const PanelViewData& view_data, const Callbacks& callbacks, float scroll_h, const char* child_id)
{
    if (view_data.rows.empty())
        return;

    const float one_row_h = rows_scroll_height(1, view_data.scale);
    const float full_h = rows_scroll_height(view_data.rows.size(), view_data.scale);
    const bool can_delete_rows = view_data.rows.size() > 1;
    const float effective_scroll_h = std::max(one_row_h, scroll_h);
    const bool needs_child_scroll = effective_scroll_h + 0.5f * view_data.scale < full_h;

    if (!needs_child_scroll) {
        for (size_t i = 0; i < view_data.rows.size(); ++i) {
            render_row(view_data.rows[i], view_data.mode, view_data.scale, can_delete_rows, callbacks);
            if (i + 1 < view_data.rows.size())
                ImGui::Dummy(ImVec2(0.f, 7.f * view_data.scale));
        }
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_ChildBg, rgba(0.f, 0.f, 0.f, 0.f));
    ImGui::BeginChild(child_id, ImVec2(0.f, effective_scroll_h), false);
    for (size_t i = 0; i < view_data.rows.size(); ++i) {
        render_row(view_data.rows[i], view_data.mode, view_data.scale, can_delete_rows, callbacks);
        if (i + 1 < view_data.rows.size())
            ImGui::Dummy(ImVec2(0.f, 7.f * view_data.scale));
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

static void render_preview_card(const PanelViewData& view_data, const Callbacks& callbacks)
{
    const float scale = view_data.scale;
    const ImVec2 top_left = ImGui::GetCursorScreenPos();
    const float content_w = ImGui::GetContentRegionAvail().x;
    const float min_card_h = 168.f * scale;
    const float card_h = std::max(min_card_h, ImGui::GetContentRegionAvail().y);
    const ImVec2 size(content_w, card_h);
    const ImVec2 bottom_right(top_left.x + size.x, top_left.y + size.y);

    draw_card(top_left, bottom_right, rgba(1.f, 1.f, 1.f, 0.038f), rgba(1.f, 1.f, 1.f, 0.045f), 18.f * scale);
    ImGui::Dummy(size);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const char* preview_title = u8"\u6253\u5370\u6548\u679c\u9884\u89c8";
    const ImVec2 title_pos(top_left.x + 14.f * scale, top_left.y + 12.f * scale);
    draw_list->AddText(title_pos, IM_COL32(244, 247, 251, 255), preview_title);

    auto draw_rotate_button = [&](const char* widget_id, bool point_left, bool enabled, const ImVec2& pos, const ImVec2& btn_size) {
        ImGui::SetCursorScreenPos(pos);
        ImGui::InvisibleButton(widget_id, btn_size);
        const bool hovered = ImGui::IsItemHovered();
        const bool clicked = enabled && ImGui::IsItemClicked(ImGuiMouseButton_Left);
        const ImVec2 max(pos.x + btn_size.x, pos.y + btn_size.y);

        const ImU32 fill = enabled
            ? ImGui::GetColorU32(hovered ? rgba(1.f, 1.f, 1.f, 0.10f) : rgba(1.f, 1.f, 1.f, 0.045f))
            : ImGui::GetColorU32(rgba(1.f, 1.f, 1.f, 0.024f));
        const ImU32 border = enabled
            ? ImGui::GetColorU32(hovered ? rgba(1.f, 1.f, 1.f, 0.24f) : rgba(1.f, 1.f, 1.f, 0.10f))
            : ImGui::GetColorU32(rgba(1.f, 1.f, 1.f, 0.05f));
        const ImU32 icon_col = enabled
            ? (hovered ? IM_COL32(236, 242, 247, 255) : IM_COL32(186, 196, 206, 255))
            : IM_COL32(108, 116, 126, 180);

        ImDrawList* button_draw_list = ImGui::GetWindowDrawList();
        button_draw_list->AddRectFilled(pos, max, fill, 999.f);
        button_draw_list->AddRect(pos, max, border, 999.f, 0, hovered && enabled ? 1.4f : 1.0f);

        const ImVec2 center(pos.x + btn_size.x * 0.5f, pos.y + btn_size.y * 0.5f);
        const float len = 4.1f * scale;
        const float wing = 3.0f * scale;
        if (point_left) {
            button_draw_list->AddLine(ImVec2(center.x + len * 0.5f, center.y - wing),
                                      ImVec2(center.x - len * 0.5f, center.y),
                                      icon_col,
                                      1.6f);
            button_draw_list->AddLine(ImVec2(center.x - len * 0.5f, center.y),
                                      ImVec2(center.x + len * 0.5f, center.y + wing),
                                      icon_col,
                                      1.6f);
        } else {
            button_draw_list->AddLine(ImVec2(center.x - len * 0.5f, center.y - wing),
                                      ImVec2(center.x + len * 0.5f, center.y),
                                      icon_col,
                                      1.6f);
            button_draw_list->AddLine(ImVec2(center.x + len * 0.5f, center.y),
                                      ImVec2(center.x - len * 0.5f, center.y + wing),
                                      icon_col,
                                      1.6f);
        }

        return clicked;
    };

    const float chip_gap = 5.f * scale;
    const ImVec2 chip_size(42.f * scale, 24.f * scale);
    const ImVec2 rotate_btn_size(22.f * scale, 22.f * scale);
    float controls_right = bottom_right.x - 12.f * scale;

    if (view_data.show_preview_view_switcher && callbacks.on_change_preview_view) {
        const ImVec2 top_pos(controls_right - chip_size.x, top_left.y + 9.f * scale);
        controls_right = top_pos.x - chip_gap;
        if (draw_preview_view_chip("##resident_preview_view_top",
                                   u8"\u4fef\u89c6",
                                   PreviewViewType::Top,
                                   view_data.preview_view,
                                   scale,
                                   top_pos,
                                   chip_size)) {
            callbacks.on_change_preview_view(PreviewViewType::Top);
        }

        const ImVec2 front_pos(controls_right - chip_size.x, top_left.y + 9.f * scale);
        controls_right = front_pos.x - chip_gap;
        if (draw_preview_view_chip("##resident_preview_view_front",
                                   u8"\u6b63\u89c6",
                                   PreviewViewType::Front,
                                   view_data.preview_view,
                                   scale,
                                   front_pos,
                                   chip_size)) {
            callbacks.on_change_preview_view(PreviewViewType::Front);
        }

        const ImVec2 iso_pos(controls_right - chip_size.x, top_left.y + 9.f * scale);
        controls_right = iso_pos.x;
        if (draw_preview_view_chip("##resident_preview_view_iso",
                                   u8"\u7b49\u8f74",
                                   PreviewViewType::Iso,
                                   view_data.preview_view,
                                   scale,
                                   iso_pos,
                                   chip_size)) {
            callbacks.on_change_preview_view(PreviewViewType::Iso);
        }
    }

    const float header_h = 40.f * scale;
    const ImVec2 stage_pos(top_left.x + 10.f * scale, top_left.y + header_h);
    const ImVec2 stage_size(std::max(1.f, size.x - 20.f * scale),
                            std::max(1.f, size.y - header_h - 8.f * scale));
    draw_card(stage_pos, ImVec2(stage_pos.x + stage_size.x, stage_pos.y + stage_size.y), rgba(1.f, 1.f, 1.f, 0.018f), rgba(1.f, 1.f, 1.f, 0.032f), 14.f * scale);

    if (callbacks.render_preview) {
        const float preview_child_w = std::max(1.f, stage_size.x - 8.f * scale);
        const float preview_child_h = std::max(1.f, stage_size.y - 8.f * scale);
        ImGui::SetCursorScreenPos(ImVec2(stage_pos.x + 4.f * scale, stage_pos.y + 4.f * scale));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, rgba(0.f, 0.f, 0.f, 0.f));
        ImGui::BeginChild("resident_preview_stage", ImVec2(preview_child_w, preview_child_h), false,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        callbacks.render_preview(preview_child_w, preview_child_h, scale);
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    if (view_data.show_preview_rotate_buttons) {
        const float overlay_pad = 10.f * scale;
        const float overlay_gap = 6.f * scale;
        const ImVec2 overlay_frame_size(rotate_btn_size.x * 2.f + overlay_gap + 10.f * scale,
                                        rotate_btn_size.y + 8.f * scale);
        const ImVec2 overlay_frame_pos(stage_pos.x + stage_size.x - overlay_pad - overlay_frame_size.x,
                                       stage_pos.y + overlay_pad);

        ImGui::SetCursorScreenPos(overlay_frame_pos);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, rgba(0.f, 0.f, 0.f, 0.f));
        ImGui::BeginChild("resident_preview_rotate_overlay",
                          overlay_frame_size,
                          false,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground);

        ImDrawList* overlay_draw_list = ImGui::GetWindowDrawList();
        const ImVec2 overlay_min = ImGui::GetWindowPos();
        const ImVec2 overlay_max(overlay_min.x + overlay_frame_size.x, overlay_min.y + overlay_frame_size.y);
        overlay_draw_list->AddRectFilled(overlay_min,
                                         overlay_max,
                                         IM_COL32(33, 39, 46, 224),
                                         10.f * scale);
        overlay_draw_list->AddRect(overlay_min,
                                   overlay_max,
                                   IM_COL32(255, 255, 255, 44),
                                   10.f * scale,
                                   0,
                                   1.1f);

        const ImVec2 left_btn_pos(overlay_min.x + 5.f * scale, overlay_min.y + 4.f * scale);
        if (draw_rotate_button("##resident_preview_rotate_left_overlay",
                               true,
                               view_data.preview_rotate_left_enabled,
                               left_btn_pos,
                               rotate_btn_size) && callbacks.on_rotate_preview_left) {
            callbacks.on_rotate_preview_left();
        }

        const ImVec2 right_btn_pos(left_btn_pos.x + rotate_btn_size.x + overlay_gap, left_btn_pos.y);
        if (draw_rotate_button("##resident_preview_rotate_right_overlay",
                               false,
                               view_data.preview_rotate_right_enabled,
                               right_btn_pos,
                               rotate_btn_size) && callbacks.on_rotate_preview_right) {
            callbacks.on_rotate_preview_right();
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
}

static void render_list_header(const PanelViewData& view_data, const Callbacks& callbacks)
{
    const float scale = view_data.scale;
    const ImVec2 top_left = ImGui::GetCursorScreenPos();
    const float content_w = ImGui::GetContentRegionAvail().x;
    const bool compact = content_w < 372.f * scale;
    const ImVec2 size(content_w, 40.f * scale);
    const ImVec2 bottom_right(top_left.x + size.x, top_left.y + size.y);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    const std::string title = view_data.current_plate_title.empty() ? std::string(u8"\u5f53\u524d\u76d8\u989c\u8272") : view_data.current_plate_title;
    const ImVec2 title_size = ImGui::CalcTextSize(title.c_str());
    const float title_y = top_left.y + (size.y - title_size.y) * 0.5f - 0.5f * scale;
    draw_list->AddText(ImVec2(top_left.x, title_y), IM_COL32(244, 247, 251, 255), title.c_str());

    const std::string count_text = view_data.current_plate_count_text.empty()
        ? std::to_string((int)view_data.rows.size()) + u8" \u4e2a\u989c\u8272"
        : view_data.current_plate_count_text;
    const ImVec2 count_size = ImGui::CalcTextSize(count_text.c_str());
    const ImVec2 count_pos(top_left.x + title_size.x + 10.f * scale, top_left.y + (size.y - count_size.y) * 0.5f);
    draw_list->AddText(count_pos, IM_COL32(156, 166, 176, 255), count_text.c_str());

    const float add_btn_w = compact ? 88.f * scale : 98.f * scale;
    const ImVec2 add_btn_size(add_btn_w, 28.f * scale);
    const ImVec2 add_btn_pos(bottom_right.x - add_btn_size.x, top_left.y + (size.y - add_btn_size.y) * 0.5f);
    if (!view_data.plate_scope_subtitle.empty()) {
        const ImVec2 subtitle_size = ImGui::CalcTextSize(view_data.plate_scope_subtitle.c_str());
        const ImVec2 pill_pos(count_pos.x + count_size.x + 9.f * scale, top_left.y + (size.y - 20.f * scale) * 0.5f + 0.5f * scale);
        const ImVec2 pill_size(std::max(72.f * scale, subtitle_size.x + 14.f * scale), 20.f * scale);
        if (pill_pos.x + pill_size.x <= add_btn_pos.x - 8.f * scale) {
            draw_list->AddRectFilled(pill_pos,
                                     ImVec2(pill_pos.x + pill_size.x, pill_pos.y + pill_size.y),
                                     IM_COL32(255, 255, 255, 10),
                                     999.f);
            draw_list->AddRect(pill_pos,
                               ImVec2(pill_pos.x + pill_size.x, pill_pos.y + pill_size.y),
                               IM_COL32(255, 255, 255, 14),
                               999.f);
            draw_centered_text(pill_pos,
                               ImVec2(pill_pos.x + pill_size.x, pill_pos.y + pill_size.y),
                               IM_COL32(138, 148, 159, 255),
                               view_data.plate_scope_subtitle);
        }
    }
    if (draw_header_add_button(view_data.add_enabled, scale, add_btn_pos, add_btn_size) && callbacks.on_request_add)
        callbacks.on_request_add(add_btn_pos, ImVec2(add_btn_pos.x + add_btn_size.x, add_btn_pos.y + add_btn_size.y));

    draw_list->AddLine(ImVec2(top_left.x, bottom_right.y + 1.f * scale),
                       ImVec2(bottom_right.x, bottom_right.y + 1.f * scale),
                       IM_COL32(255, 255, 255, 9),
                       1.f);

    ImGui::Dummy(size);
}

static bool render_fold_section_header(
    const std::string& title,
    const std::string& count_text,
    bool open,
    float scale)
{
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const float content_w = ImGui::GetContentRegionAvail().x;
    const ImVec2 size(content_w, 30.f * scale);

    ImGui::InvisibleButton("##other_plates_fold_header", size);
    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);

    const ImVec2 min = pos;
    const ImVec2 max(pos.x + size.x, pos.y + size.y);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(min, max,
                             hovered ? IM_COL32(255, 255, 255, 8) : IM_COL32(255, 255, 255, 4),
                             10.f * scale);
    draw_list->AddLine(ImVec2(min.x, max.y),
                       ImVec2(max.x, max.y),
                       hovered ? IM_COL32(255, 255, 255, 12) : IM_COL32(255, 255, 255, 8),
                       1.f);

    const ImVec2 chevron_center(min.x + 14.f * scale, min.y + size.y * 0.5f);
    const float len = 3.6f * scale;
    const ImU32 chev_col = hovered ? IM_COL32(224, 231, 238, 255) : IM_COL32(162, 171, 181, 210);
    if (open) {
        draw_list->AddLine(ImVec2(chevron_center.x - len, chevron_center.y - len * 0.45f),
                           ImVec2(chevron_center.x, chevron_center.y + len * 0.55f),
                           chev_col,
                           1.4f);
        draw_list->AddLine(ImVec2(chevron_center.x, chevron_center.y + len * 0.55f),
                           ImVec2(chevron_center.x + len, chevron_center.y - len * 0.45f),
                           chev_col,
                           1.4f);
    } else {
        draw_list->AddLine(ImVec2(chevron_center.x - len * 0.45f, chevron_center.y - len),
                           ImVec2(chevron_center.x + len * 0.55f, chevron_center.y),
                           chev_col,
                           1.4f);
        draw_list->AddLine(ImVec2(chevron_center.x + len * 0.55f, chevron_center.y),
                           ImVec2(chevron_center.x - len * 0.45f, chevron_center.y + len),
                           chev_col,
                           1.4f);
    }

    draw_list->AddText(ImVec2(min.x + 28.f * scale, min.y + 7.f * scale), IM_COL32(202, 210, 218, 255), title.c_str());
    if (!count_text.empty()) {
        const ImVec2 title_size = ImGui::CalcTextSize(title.c_str());
        draw_list->AddText(ImVec2(min.x + 28.f * scale + title_size.x + 8.f * scale, min.y + 7.f * scale),
                           IM_COL32(124, 134, 144, 255),
                           count_text.c_str());
    }

    return clicked;
}

} // namespace

void render_panel(const PanelViewData& view_data, const Callbacks& callbacks)
{
    const float scale = view_data.scale;
    const ImVec2 panel_pos = ImGui::GetCursorScreenPos();
    const ImVec2 panel_size = ImGui::GetContentRegionAvail();

    if (!view_data.embed_in_unified_panel)
        draw_card(panel_pos, ImVec2(panel_pos.x + panel_size.x, panel_pos.y + panel_size.y), rgba(0.08f, 0.10f, 0.13f, 0.94f), rgba(1.f, 1.f, 1.f, 0.08f), 22.f * scale);

    const float pad = 14.f * scale;
    ImGui::SetCursorScreenPos(ImVec2(panel_pos.x + pad, panel_pos.y + pad));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, rgba(0.f, 0.f, 0.f, 0.f));
    ImGui::BeginChild("resident_filament_mapping_root", ImVec2(panel_size.x - pad * 2.f, panel_size.y - pad * 2.f), false);

    render_summary_card(view_data.summary, scale);
    ImGui::Dummy(ImVec2(0.f, 10.f * scale));

    if (view_data.show_unified_output_card) {
        render_unified_output_card(view_data.unified_output, scale);
        ImGui::Dummy(ImVec2(0.f, 10.f * scale));
    }

    render_list_header(view_data, callbacks);
    ImGui::Dummy(ImVec2(0.f, 10.f * scale));

    PanelViewData current_plate_view = view_data;
    current_plate_view.rows = view_data.current_plate_rows.empty() ? view_data.rows : view_data.current_plate_rows;
    const bool show_other_section = view_data.show_other_plates_section;
    bool& other_section_open = other_plates_foldout_open();
    const size_t other_row_count = view_data.non_current_rows.empty() ? view_data.other_plate_rows.size() : view_data.non_current_rows.size();
    const float preview_min_h = 150.f * scale;
    const float one_row_h = rows_scroll_height(1, scale);
    const float current_rows_full_h = rows_scroll_height(current_plate_view.rows.size(), scale);
    const float other_rows_full_h = rows_scroll_height(other_row_count, scale);
    const float current_rows_budget = std::max(one_row_h,
        ImGui::GetContentRegionAvail().y - preview_min_h - 12.f * scale -
        (show_other_section ? (12.f * scale + 30.f * scale + (other_section_open ? (8.f * scale + std::min(other_rows_full_h, 148.f * scale)) : 0.f)) : 0.f));
    render_rows_scroll(current_plate_view, callbacks, std::min(current_rows_full_h, current_rows_budget), "resident_current_rows_scroll");
    if (show_other_section) {
        ImGui::Dummy(ImVec2(0.f, 12.f * scale));
        if (render_fold_section_header(
                view_data.other_plates_title.empty() ? std::string(u8"\u5176\u4ed6\u76d8\u989c\u8272") : view_data.other_plates_title,
                view_data.other_plates_count_text,
                other_section_open,
                scale)) {
            other_section_open = !other_section_open;
        }
        if (other_section_open) {
            ImGui::Dummy(ImVec2(0.f, 8.f * scale));
            PanelViewData other_plate_view = view_data;
            other_plate_view.rows = view_data.non_current_rows.empty() ? view_data.other_plate_rows : view_data.non_current_rows;
            const float other_rows_budget = std::max(one_row_h,
                ImGui::GetContentRegionAvail().y - preview_min_h - 12.f * scale);
            render_rows_scroll(other_plate_view, callbacks, std::min(other_rows_full_h, other_rows_budget), "resident_other_rows_scroll");
        }
    }

    ImGui::Dummy(ImVec2(0.f, 12.f * scale));

    render_preview_card(view_data, callbacks);

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

} // namespace ResidentFilamentMappingView
} // namespace GUI
} // namespace Slic3r
