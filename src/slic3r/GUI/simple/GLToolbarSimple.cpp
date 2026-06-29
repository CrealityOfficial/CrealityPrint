#include "libslic3r/Point.hpp"
#include "libslic3r/libslic3r.h"

#include "GLToolbar.hpp"

#include "slic3r/GUI/GLCanvas3D.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/Camera.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/Gizmos/GLGizmosManager.hpp"

#include <wx/event.h>
#include <wx/bitmap.h>
#include <wx/dcmemory.h>
#include <wx/settings.h>
#include <wx/glcanvas.h>

#include <float.h>
#include <string>
#include "GLSimpleUtils.hpp"

namespace Slic3r {
namespace GUI 
{

static bool is_top_row_item(const GLToolbarItem* item) 
{
    if (!item) return false;
    const std::string& name = item->get_name();
    return name == "clone" || name == "remove" || name == "orient" || name == "arrange" || name == _u8L("Cut");
}


void GLToolbarItem::render_icon_top_label_bottom_px(
    unsigned int icons_atlas_id,
    unsigned int icons_tex_w,
    unsigned int icons_tex_h,
    unsigned int icon_size_px,
    float left_px, float right_px, float bottom_px, float top_px,
    float canvas_w, float canvas_h) const
{
    const float h_px  = top_px - bottom_px;
    const float split = std::clamp(m_data.icon_label_split, 0.45f, 0.85f);

    // --- icon region (upper part in screen space)
    const float icon_h     = (h_px < 0.0f ? -1.0f : 1.0f) * (right_px - left_px) * split;
    const float icon_top   = top_px;              // larger y on screen (higher visually)
    const float icon_bottom= icon_top - icon_h;   // smaller y on screen

    // --- label region (lower part)
    const float label_top    = icon_bottom;
    const float label_bottom = bottom_px;

    // square icon centered horizontally
    const float icon_w = icon_h;
    const float cx     = 0.5f * (left_px + right_px);
    const float ix0    = cx - 0.5f * icon_w;   // icon left
    const float ix1    = cx + 0.5f * icon_w;   // icon right

    // map px -> NDC **with top→ndc_top, bottom→ndc_bottom**
    const float ndc_l = px_to_ndc_x(ix1,        canvas_w);
    const float ndc_r = px_to_ndc_x(ix0,        canvas_w);
    const float ndc_t = px_to_ndc_y(icon_top, canvas_h); // top in NDC corresponds to smaller screen y
    const float ndc_b = px_to_ndc_y(icon_bottom,    canvas_h); // bottom in NDC corresponds to larger screen y

    if (!is_action_with_text_image()) {
        if (icons_atlas_id && icons_tex_w > 0 && icons_tex_h > 0) {
            auto uv = get_uvs(icons_tex_w, icons_tex_h, icon_size_px);
            ImGui::GetBackgroundDrawList()->AddImage(
                (ImTextureID)(intptr_t)icons_atlas_id,
                ImVec2(ix0, icon_top),
                ImVec2(ix1, icon_bottom),
                ImVec2(uv.left_top.u, uv.left_top.v),
                ImVec2(uv.right_bottom.u, uv.right_bottom.v),
                ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f))
            );
        }
    } else if (m_data.left.can_render()) {
        m_data.left.render_callback(ndc_l, ndc_r, ndc_b, ndc_t);
    }

    if (is_pressed()) {
        if ((m_last_action_type == Left)  && m_data.left.can_render())
            m_data.left.render_callback(ndc_l, ndc_r, ndc_b, ndc_t);
        else if ((m_last_action_type == Right) && m_data.right.can_render())
            m_data.right.render_callback(ndc_l, ndc_r, ndc_b, ndc_t);
    }

    // --- Label region via ImGui (pixel-space window centered text)
    if (is_action_with_text()) {
        const char* text =
            !m_data.button_text.empty() ? m_data.button_text.c_str() :
            !m_data.tooltip.empty()     ? m_data.tooltip.c_str() :
                                          m_data.name.c_str();

        ImGuiWindowFlags wflags = ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoResize   | ImGuiWindowFlags_NoScrollbar
            | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_NoBackground;

        const float label_left_px   = left_px;
        const float label_right_px  = right_px;
        const float label_top_px    = label_top;
        const float label_bottom_px = label_bottom;
        const float label_w         = label_right_px - label_left_px;
        const float label_h         = std::max(0.0f, label_bottom_px - label_top_px);

        // Make the window name unique per item (pointer-based suffix is fine)
        char win_name[128];
        std::snprintf(win_name, sizeof(win_name), "##toolbar_item_label_px_%p", (const void*)this);

        ImGui::PushID(this);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
        ImGui::SetNextWindowPos(ImVec2(label_left_px, label_top_px), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(label_w, label_h), ImGuiCond_Always);

        if (ImGui::Begin(win_name, nullptr, wflags)) {

            const bool dark = wxGetApp().dark_mode();

            ImVec4 base_col = dark
                ? ImVec4(0.92f, 0.94f, 0.97f, 1.00f) 
                : ImVec4(0.14f, 0.16f, 0.18f, 0.95f);

            ImGui::PushStyleColor(ImGuiCol_Text, base_col);

            // clip to the label rect to avoid overflow
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const ImVec2 win_pos = ImGui::GetWindowPos();
            const ImVec2 win_sz  = ImGui::GetWindowSize();
            const ImVec2 win_max = ImVec2(win_pos.x + win_sz.x, win_pos.y + win_sz.y);

            ImGui::PushClipRect(win_pos, win_max, true);

            std::vector<std::string> lines;
            std::string              current;
            std::string              pending(text);

            while (!pending.empty() && lines.size() < 2) {
                size_t space_pos = pending.find(' ');
                std::string word = space_pos == std::string::npos ? pending : pending.substr(0, space_pos);
                pending = space_pos == std::string::npos ? std::string() : pending.substr(space_pos + 1);
                while (!pending.empty() && pending.front() == ' ')
                    pending.erase(pending.begin());

                std::string candidate = current.empty() ? word : current + " " + word;
                if (!current.empty() && ImGui::CalcTextSize(candidate.c_str()).x > label_w) {
                    lines.push_back(current);
                    current = word;
                } else {
                    current = candidate;
                }
            }

            if (!current.empty() && lines.size() < 2)
                lines.push_back(current);
            if (!pending.empty() && !lines.empty())
                lines.back() += " " + pending;
            if (lines.empty())
                lines.emplace_back(text);

            const float line_h = ImGui::GetTextLineHeight();
            const float total_h = line_h * float(lines.size());
            const float gutter_px = 0.0f;
            float cy = std::max(0.0f, 0.5f * (label_h - total_h) + gutter_px);

            for (const std::string& line : lines) {
                ImVec2 text_sz = ImGui::CalcTextSize(line.c_str());
                float cx = std::max(0.0f, 0.5f * (label_w - text_sz.x));
                ImGui::SetCursorPos(ImVec2(cx, cy));
                ImGui::TextUnformatted(line.c_str());
                cy += line_h;
            }
            // ImGui::SetWindowFontScale(1.0f);
            ImGui::PopClipRect();

            ImGui::PopStyleColor();
        }
        ImGui::End();
        ImGui::PopStyleVar(); // WindowPadding
        ImGui::PopID();
    }
}

bool GLToolbarItem::is_always_enable() const
{
    return m_data.always_enable;
}

float GLToolbar::get_rendered_right_edge_px(const GLCanvas3D& parent) const
{
    const Size  cnv_size = parent.get_canvas_size();
    const float cnv_w    = float(cnv_size.get_width());
    const float cnv_h    = float(cnv_size.get_height());
    if (cnv_w <= 0.f || cnv_h <= 0.f) return 0.f;

    // 和渲染里保持一致：图标 / 间距 用“像素→NDC”的同一套换算
    const float icons_size_x_ndc = 2.0f * (m_layout.icons_size * m_layout.scale) / cnv_w;
    const float gap_size_ndc     = 2.0f * (m_layout.gap_size   * m_layout.scale) / cnv_w;

    float right_edge_px = 0.f;

    for (const GLToolbarItem* item : m_items) {
        if (!item->is_visible() || item->is_separator())
            continue;

        // 该 item 左边界（NDC）
        const float l_ndc = item->render_left_pos;

        // 该 item 本身的 NDC 宽度：图标宽 +（如果有文字）额外宽度
        float w_ndc = icons_size_x_ndc;
        if (item->is_action_with_text()) {
            // 文本额外宽度 = ratio * 图标宽（和 render_horizontal_simple 用的一致）
            w_ndc += item->get_extra_size_ratio() * icons_size_x_ndc;
        }

        // 右缘（NDC）
        const float r_ndc = l_ndc + w_ndc;

        // NDC -> 像素： x_px = (ndc + 1) * 0.5 * cnv_w
        const float r_px = (r_ndc + 1.0f) * 0.5f * cnv_w;

        if (r_px > right_edge_px)
            right_edge_px = r_px;
    }

    return right_edge_px;
}

void GLToolbar::render_horizontal_simple(const GLCanvas3D& parent,
                                         GLToolbarItem::EType type,
                                         float scroll)
{
    const Size  cnv_size = parent.get_canvas_size();
    const float cnv_w    = float(cnv_size.get_width());
    const float cnv_h    = float(cnv_size.get_height());
    if (cnv_w == 0.f || cnv_h == 0.f) return;

    // NDC (-1..+1) -> pixels
    auto ndc_to_px = [&](float x_ndc, float y_ndc, float& x_px, float& y_px) {
        x_px = (x_ndc * 0.5f + 0.5f) * cnv_w;
        // In NDC, +1 is top and -1 is bottom; screen Y increases downward
        y_px = (0.5f - y_ndc * 0.5f) * cnv_h;
    };
    auto px_to_ndc = [&](float x_px, float y_px, float& x_ndc, float& y_ndc) {
        x_ndc = (x_px / cnv_w - 0.5f) * 2.0f;
        y_ndc = (0.5f - y_px / cnv_h) * 2.0f;
    };

    const float inv_w = 1.f / cnv_w;
    const float inv_h = 1.f / cnv_h;

    // Layout units
    const float icon_px      = m_layout.icons_size * m_layout.scale;
    float gap_px = m_layout.gap_size * (m_layout.gap_scale > 0.f ? m_layout.gap_scale : 1.f) * m_layout.scale;
    gap_px = std::min(gap_px, icon_px * 0.20f);
    const float border_px         = m_layout.border * m_layout.scale;
    const float toolbar_height_px = get_height_horizontal_simple(false);
    const float cell_height_px    = toolbar_height_px - 2.0f * border_px;
    const float top_toolbar_height_px = get_height_horizontal_simple(true);
    const float top_cell_height_px    = top_toolbar_height_px - 2.0f * border_px;
    const float row_gap_px = gap_px; // vertical spacing between rows

    const float width_ndc    = 2.f * get_width()  * inv_w;  // full toolbar background width (content+borders)
    const float height_ndc   = 2.f * toolbar_height_px * inv_h;

    const float cell_stride_px = icon_px + gap_px;

    const bool extra_row_below = (m_layout.vertical_orientation == Layout::VO_Bottom);

    // Band (content area) left aligned with scroll; background must align to this
    const float band_left_ndc  = 2.f * (m_layout.left + scroll) * inv_w;
    const float band_width_ndc = (m_layout.limit_width > 0.f)
                               ? 2.f * m_layout.limit_width * inv_w
                               : width_ndc;
    const float band_right_ndc = band_left_ndc + band_width_ndc;

    // Background rect (NDC)
    const float bg_left_ndc    = band_left_ndc;
    const float bg_top_ndc     = 2.f * m_layout.top * inv_h;
    const float bg_bottom_ndc  = bg_top_ndc - height_ndc;
    const float bg_right_ndc   = band_right_ndc;

    // Convert bg NDC rect to pixels
    float bg_l_px, bg_t_px, bg_r_px, bg_b_px;
    ndc_to_px(bg_left_ndc,  bg_top_ndc,    bg_l_px, bg_t_px); // top-left
    ndc_to_px(bg_right_ndc, bg_bottom_ndc, bg_r_px, bg_b_px); // bottom-right

    // Visual params
    const bool  dark      = wxGetApp().dark_mode();
    const float radius_px = 8.0f * m_layout.scale;
    const float stroke_px = 1.0f;
    const bool  drop_shadow = false;
    const float shadow_px = 8.0f * m_layout.scale;

    ImU32 fill = ImGui::GetColorU32(dark ? ImVec4(0.10f,0.10f,0.12f,0.50f)
                                         : ImVec4(1.00f,1.00f,1.00f,0.50f));
    ImU32 bord = ImGui::GetColorU32(dark ? ImVec4(0.30f,0.30f,0.35f,0.60f)
                                         : ImVec4(0.00f,0.00f,0.00f,0.10f));
    ImU32 shad = ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 60.0f/255.0f));

    draw_toolbar_bg_round_rect_px(bg_l_px, bg_t_px, bg_r_px, bg_b_px,
                                  radius_px, stroke_px, fill, bord,
                                  drop_shadow, shadow_px, shad);

    // cache bg rect
    m_has_last_bg_rect_px = true;
    m_last_bg_l_px = bg_l_px;
    m_last_bg_t_px = bg_t_px;
    m_last_bg_r_px = bg_r_px;
    m_last_bg_b_px = bg_b_px;

    // split items into rows
    std::vector<GLToolbarItem*> top_row;
    std::vector<GLToolbarItem*> bottom_row;
    for (GLToolbarItem* item : m_items) {
        if (!item->is_visible() || item->is_separator()) continue;
        if (is_top_row_item(item))
            top_row.push_back(item);
        else
            bottom_row.push_back(item);
    }

    auto row_width_px = [&](const std::vector<GLToolbarItem*>& row) {
        if (row.empty()) return 0.0f;
        return icon_px * float(row.size()) + gap_px * float(row.size() - 1);
    };

    // Cache atlas
    const unsigned int atlas_id     = m_icons_texture.get_id();
    const int          atlas_w      = m_icons_texture.get_width();
    const int          atlas_h      = m_icons_texture.get_height();
    const unsigned int icon_size_px = (unsigned int)(m_layout.icons_size * m_layout.scale);

    auto render_row_px = [&](const std::vector<GLToolbarItem*>& row, float row_left_px, float row_top_px, float row_height_px) {
        float left_px = row_left_px;
        for (size_t i = 0; i < row.size(); ++i) {
            GLToolbarItem* item = row[i];
            float right_px  = left_px + icon_px;
            float bottom_px = row_top_px + row_height_px;

            float l_ndc, r_ndc, t_ndc, b_ndc;
            px_to_ndc(left_px,  row_top_px, l_ndc, t_ndc);
            px_to_ndc(right_px, bottom_px, r_ndc, b_ndc);

            item->render_left_pos = l_ndc;
            item->render_icon_top_label_bottom_px(
                atlas_id, (unsigned)atlas_w, (unsigned)atlas_h, icon_size_px,
                left_px, right_px, bottom_px, row_top_px, cnv_w, cnv_h
            );

            if (i + 1 != row.size())
                left_px += cell_stride_px;
        }
    };

    // Main row (bottom)
    const float content_left_px = bg_l_px + border_px;
    const float content_top_px  = bg_t_px + border_px;
    render_row_px(bottom_row, content_left_px, content_top_px, cell_height_px);

    // Extra row (aligned to the right of the main row)
    const float bottom_row_width_px = row_width_px(bottom_row);
    const float top_row_width_px    = row_width_px(top_row);
    const float content_right_px    = content_left_px + bottom_row_width_px;

    if (!top_row.empty() && icon_px > 0.0f) {
        const float top_row_left_px = content_right_px - top_row_width_px;

        float top_bg_l_px, top_bg_r_px, top_bg_t_px, top_bg_b_px;
        if (extra_row_below) {
            top_bg_t_px = bg_b_px + row_gap_px;
            top_bg_b_px = top_bg_t_px + top_cell_height_px + 2.0f * border_px;
        } else {
            top_bg_b_px = content_top_px - row_gap_px;
            top_bg_t_px = top_bg_b_px - top_cell_height_px - 2.0f * border_px;
        }
        top_bg_l_px = top_row_left_px - border_px;
        top_bg_r_px = top_row_left_px + top_row_width_px + border_px;

        draw_toolbar_bg_round_rect_px(top_bg_l_px, top_bg_t_px, top_bg_r_px, top_bg_b_px,
                                      radius_px, stroke_px, fill, bord,
                                      drop_shadow, shadow_px, shad);

        render_row_px(top_row, top_row_left_px, top_bg_t_px + border_px, top_cell_height_px);
    }
}
void GLToolbar::set_layout_gap_scale(float gap_scale)
{
    m_layout.gap_scale = gap_scale;
}

void GLToolbar::set_layout_step_scale(float step_scale)
{
    m_layout.sep_scale = step_scale;
}

float GLToolbar::get_height_horizontal_simple(bool top_row) const
{
    const float icon_px = m_layout.icons_size * m_layout.scale;
    float extra_height_px = 0.0f;

    for (const GLToolbarItem* item : m_items) {
        if (item == nullptr || !item->is_visible() || is_top_row_item(item) != top_row || !item->is_action_with_text() || !item->m_data.label_below_icon)
            continue;

        const char* text = !item->m_data.button_text.empty() ? item->m_data.button_text.c_str() :
                           !item->m_data.tooltip.empty() ? item->m_data.tooltip.c_str() : item->m_data.name.c_str();
        if (std::strchr(text, ' ') == nullptr || ImGui::CalcTextSize(text).x <= icon_px)
            continue;

        const float split = std::clamp(item->m_data.icon_label_split, 0.45f, 0.85f);
        const float available_label_height = icon_px * (1.0f - split);
        const float required_label_height = 2.0f * ImGui::GetTextLineHeight();
        extra_height_px = std::max(extra_height_px, required_label_height - available_label_height);
    }

    const float base_height_px = (2.0f * m_layout.border + m_layout.icons_size) * m_layout.scale;
    return base_height_px + std::max(0.0f, extra_height_px);
}
float GLToolbar::get_width_horizontal_simple() const
{
    const float border_px = m_layout.border;
    const float icon_px   = m_layout.icons_size;
    float gap_px    = m_layout.gap_size * (m_layout.gap_scale > 0.f ? m_layout.gap_scale : 1.f);
    gap_px = std::min(gap_px, icon_px * 0.20f);
    float sep_px    = m_layout.separator_size * (m_layout.sep_scale > 0.f ? m_layout.sep_scale : 1.f);
    const float sep_stride_px = sep_px + gap_px * 0.5f;

    auto measure_row = [&](const std::vector<const GLToolbarItem*>& row) {
        if (row.empty()) return 0.0f;

        int last_icon_idx = -1; // index in row
        for (int i = int(row.size()) - 1; i >= 0; --i) {
            if (!row[i]->is_separator()) { last_icon_idx = i; break; }
        }

        float size_px = 0.0f;
        for (int i = 0; i < (int)row.size(); ++i) {
            const GLToolbarItem* it = row[i];
            if (it->is_separator()) {
                size_px += sep_stride_px;
            } else {
                size_px += icon_px;
                if (i != last_icon_idx)
                    size_px += gap_px;
            }
        }
        return size_px;
    };

    std::vector<const GLToolbarItem*> bottom_row;
    std::vector<const GLToolbarItem*> top_row;
    bottom_row.reserve(m_items.size());
    top_row.reserve(m_items.size());
    for (const GLToolbarItem* it : m_items) {
        if (it && it->is_visible()) {
            if (is_top_row_item(it))
                top_row.push_back(it);
            else
                bottom_row.push_back(it);
        }
    }

    const float main_w = measure_row(bottom_row);
    const float top_w  = measure_row(top_row);

    float size_px = 2.0f * border_px + std::max(main_w, top_w);
    return size_px * m_layout.scale;
}

void GLToolbar::update_hover_state_horizontal_simple(const Vec2d& mouse_pos, GLCanvas3D& parent)
{
    const Size  cnv_size = parent.get_canvas_size();
    const float cnv_w    = float(cnv_size.get_width());
    const float cnv_h    = float(cnv_size.get_height());
    if (cnv_w <= 0.f || cnv_h <= 0.f) return;

    auto ndc_to_px = [&](float x_ndc, float y_ndc, float& x_px, float& y_px) {
        x_px = (x_ndc * 0.5f + 0.5f) * cnv_w;
        y_px = (0.5f - y_ndc * 0.5f) * cnv_h; // +1 top, -1 bottom
    };

    const float mx_px = float(mouse_pos.x());
    const float my_px = float(mouse_pos.y());

    const float inv_w = 1.f / cnv_w;
    const float inv_h = 1.f / cnv_h;

    const float icon_px = m_layout.icons_size * m_layout.scale;
    float gap_px = m_layout.gap_size * (m_layout.gap_scale > 0.f ? m_layout.gap_scale : 1.f) * m_layout.scale;
    gap_px = std::min(gap_px, icon_px * 0.20f);
    const float border_px         = m_layout.border * m_layout.scale;
    const float toolbar_height_px = get_height_horizontal_simple(false);
    const float cell_height_px    = toolbar_height_px - 2.0f * border_px;
    const float top_toolbar_height_px = get_height_horizontal_simple(true);
    const float top_cell_height_px    = top_toolbar_height_px - 2.0f * border_px;
    const float row_gap_px = gap_px;

    const float width_ndc    = 2.f * get_width() * inv_w;
    const float height_ndc   = 2.f * toolbar_height_px * inv_h;

    const float band_left_ndc  = 2.f * (m_layout.left + m_layout.scroll) * inv_w;
    const float band_width_ndc = (m_layout.limit_width > 0.f)
                               ? 2.f * m_layout.limit_width * inv_w
                               : width_ndc;
    const float band_right_ndc = band_left_ndc + band_width_ndc;

    const float bg_left_ndc    = band_left_ndc;
    const float bg_top_ndc     = 2.f * m_layout.top * inv_h;
    const float bg_bottom_ndc  = bg_top_ndc - height_ndc;
    const float bg_right_ndc   = band_right_ndc;

    float bg_l_px, bg_t_px, bg_r_px, bg_b_px;
    ndc_to_px(bg_left_ndc,  bg_top_ndc,    bg_l_px, bg_t_px);
    ndc_to_px(bg_right_ndc, bg_bottom_ndc, bg_r_px, bg_b_px);

    std::vector<GLToolbarItem*> top_row;
    std::vector<GLToolbarItem*> bottom_row;
    for (GLToolbarItem* item : m_items) {
        if (!item->is_visible() || item->is_separator()) continue;
        if (is_top_row_item(item))
            top_row.push_back(item);
        else
            bottom_row.push_back(item);
    }

    auto row_width_px = [&](const std::vector<GLToolbarItem*>& row) {
        if (row.empty()) return 0.0f;
        return icon_px * float(row.size()) + gap_px * float(row.size() - 1);
    };

    const float content_left_px = bg_l_px + border_px;
    const float content_top_px  = bg_t_px + border_px;
    const float bottom_row_width_px = row_width_px(bottom_row);
    const float top_row_width_px    = row_width_px(top_row);
    const float content_right_px    = content_left_px + bottom_row_width_px;
    const float cell_stride_px      = icon_px + gap_px;

    const float top_row_left_px = content_right_px - top_row_width_px;
    const bool  extra_row_below = (m_layout.vertical_orientation == Layout::VO_Bottom);
    const float top_row_top_px  = extra_row_below ? (bg_b_px + row_gap_px + border_px)
                                                  : (content_top_px - row_gap_px - top_cell_height_px - border_px);

    bool any_dirty = false;

    auto update_row = [&](const std::vector<GLToolbarItem*>& row, float row_left_px, float row_top_px) {
        float left_px = row_left_px;
        for (GLToolbarItem* item : row) {
            const float right_px = left_px + icon_px;
            const float split    = std::clamp(item->m_data.icon_label_split, 0.45f, 0.85f);
            const float icon_h   = icon_px * split;
            const float icon_top = row_top_px;
            const float icon_bot = row_top_px + icon_h;

            const bool inside_icon = (mx_px >= left_px) && (mx_px <= right_px) &&
                                     (my_px >= icon_top) && (my_px <= icon_bot);

            const GLToolbarItem::EState st = item->get_state();
            auto set_and_dirty = [&](GLToolbarItem::EState ns){
                if (st != ns) { item->set_state(ns); any_dirty = true; }
            };

            switch (st) {
                case GLToolbarItem::Normal:        if (inside_icon) set_and_dirty(GLToolbarItem::Hover); break;
                case GLToolbarItem::Hover:         if (!inside_icon) set_and_dirty(GLToolbarItem::Normal); break;
                case GLToolbarItem::Pressed:       if (inside_icon) set_and_dirty(GLToolbarItem::HoverPressed); break;
                case GLToolbarItem::HoverPressed:  if (!inside_icon) set_and_dirty(GLToolbarItem::Pressed); break;
                case GLToolbarItem::Disabled:      if (inside_icon) set_and_dirty(GLToolbarItem::HoverDisabled); break;
                case GLToolbarItem::HoverDisabled: if (!inside_icon) set_and_dirty(GLToolbarItem::Disabled); break;
                default: break;
            }

            left_px += cell_stride_px;
        }
    };

    if (!bottom_row.empty())
        update_row(bottom_row, content_left_px, content_top_px);
    if (!top_row.empty())
        update_row(top_row, top_row_left_px, top_row_top_px);

    if (any_dirty) parent.set_as_dirty();
}
bool GLToolbar::get_last_bg_rect_px(float& l, float& t, float& r, float& b) const
{
    if (!m_has_last_bg_rect_px) return false;
    l = m_last_bg_l_px; t = m_last_bg_t_px; r = m_last_bg_r_px; b = m_last_bg_b_px;
    return true;
}

bool GLToolbar::update_items_state_simple(bool always)
{
    if(always)
        return true;

    bool changed = false;
    for (auto* item : m_items) {
        if (!item)
            continue;
        if (item->update_visibility())
            changed = true;
        if (item->update_enabled_state())
            changed = true;
    }
    return changed;

}

void GLToolbar::reset_item_state_simple()
{
    m_pressed_toggable_id = -1;
    for (GLToolbarItem* item : m_items) {
        if (!item->is_visible() || item->is_separator())
            continue;

        item->set_state(GLToolbarItem::Normal);
        item->reset_last_action_type();
        wxGetApp().plater()->get_current_canvas3D()->set_as_dirty();
    }

}

int GLToolbar::contains_mouse_horizontal_simple(const Vec2d& mouse_pos, const GLCanvas3D& parent) const
{
    const Size cnv_size = parent.get_canvas_size();
    const float cnv_w    = float(cnv_size.get_width());
    const float cnv_h    = float(cnv_size.get_height());
    if (cnv_w <= 0.f || cnv_h <= 0.f) return -1;

    auto ndc_to_px = [&](float x_ndc, float y_ndc, float& x_px, float& y_px) {
        x_px = (x_ndc * 0.5f + 0.5f) * cnv_w;
        y_px = (0.5f - y_ndc * 0.5f) * cnv_h;
    };

    const float mx_px = float(mouse_pos.x());
    const float my_px = float(mouse_pos.y());

    const float inv_w = 1.f / cnv_w;
    const float inv_h = 1.f / cnv_h;

    const float icon_px = m_layout.icons_size * m_layout.scale;
    float gap_px = m_layout.gap_size * (m_layout.gap_scale > 0.f ? m_layout.gap_scale : 1.f) * m_layout.scale;
    gap_px = std::min(gap_px, icon_px * 0.20f);
    const float border_px         = m_layout.border * m_layout.scale;
    const float toolbar_height_px = get_height_horizontal_simple(false);
    const float cell_height_px    = toolbar_height_px - 2.0f * border_px;
    const float top_toolbar_height_px = get_height_horizontal_simple(true);
    const float top_cell_height_px    = top_toolbar_height_px - 2.0f * border_px;
    const float row_gap_px = gap_px;

    const float width_ndc    = 2.f * m_layout.width * inv_w;
    const float height_ndc   = 2.f * toolbar_height_px * inv_h;

    const float band_left_ndc  = 2.f * (m_layout.left + m_layout.scroll) * inv_w;
    const float band_width_ndc = (m_layout.limit_width > 0.f)
                               ? 2.f * m_layout.limit_width * inv_w
                               : width_ndc;
    const float band_right_ndc = band_left_ndc + band_width_ndc;

    const float bg_left_ndc    = band_left_ndc;
    const float bg_top_ndc     = 2.f * m_layout.top * inv_h;
    const float bg_bottom_ndc  = bg_top_ndc - height_ndc;
    const float bg_right_ndc   = band_right_ndc;

    float bg_l_px, bg_t_px, bg_r_px, bg_b_px;
    ndc_to_px(bg_left_ndc,  bg_top_ndc,    bg_l_px, bg_t_px);
    ndc_to_px(bg_right_ndc, bg_bottom_ndc, bg_r_px, bg_b_px);

    std::vector<std::pair<const GLToolbarItem*, size_t>> top_row;
    std::vector<std::pair<const GLToolbarItem*, size_t>> bottom_row;
    for (size_t idx = 0; idx < m_items.size(); ++idx) {
        const GLToolbarItem* item = m_items[idx];
        if (!item->is_visible() || item->is_separator()) continue;
        if (is_top_row_item(item))
            top_row.push_back({item, idx});
        else
            bottom_row.push_back({item, idx});
    }

    auto row_width_px = [&](const auto& row) {
        if (row.empty()) return 0.0f;
        return icon_px * float(row.size()) + gap_px * float(row.size() - 1);
    };

    const float content_left_px = bg_l_px + border_px;
    const float content_top_px  = bg_t_px + border_px;
    const float bottom_row_width_px = row_width_px(bottom_row);
    const float top_row_width_px    = row_width_px(top_row);
    const float content_right_px    = content_left_px + bottom_row_width_px;
    const float cell_stride_px      = icon_px + gap_px;

    const float top_row_left_px = content_right_px - top_row_width_px;
    const bool  extra_row_below = (m_layout.vertical_orientation == Layout::VO_Bottom);
    const float top_row_top_px  = extra_row_below ? (bg_b_px + row_gap_px + border_px)
                                                  : (content_top_px - row_gap_px - top_cell_height_px - border_px);

    auto hit_row = [&](const auto& row, float row_left_px, float row_top_px, float row_height_px) -> int {
        float left_px = row_left_px;
        for (size_t i = 0; i < row.size(); ++i) {
            const float right_px  = left_px + icon_px;
            const float bottom_px = row_top_px + row_height_px;

            if (mx_px >= left_px && mx_px <= right_px && my_px >= row_top_px && my_px <= bottom_px)
                return int(row[i].second);

            left_px = right_px;
            if (i + 1 != row.size()) {
                float gap_r = left_px + gap_px;
                if (mx_px >= left_px && mx_px <= gap_r && my_px >= row_top_px && my_px <= bottom_px)
                    return -2;
                left_px = gap_r;
            }
        }
        return -1;
    };

    int hit = -1;
    if (!top_row.empty()) {
        hit = hit_row(top_row, top_row_left_px, top_row_top_px, top_cell_height_px);
        if (hit != -1) return hit;
    }

    if (!bottom_row.empty()) {
        hit = hit_row(bottom_row, content_left_px, content_top_px, cell_height_px);
        if (hit != -1) return hit;
    }

    return -1;
}

}// namespace GUI

} // namespace Slic3r
