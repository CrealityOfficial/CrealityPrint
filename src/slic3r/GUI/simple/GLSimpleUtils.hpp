// GLSimpleUtils.hpp
#pragma once

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui/imgui_internal.h>

namespace Slic3r { namespace GUI {

static inline float px_to_ndc_x(float x_px, float canvas_w) 
{
    return (x_px / canvas_w) * 2.0f - 1.0f;
}
static inline float px_to_ndc_y(float y_px, float canvas_h) 
{
    // NDC y: +1 is top, -1 is bottom; screen y grows downward
    return 1.0f - (y_px / canvas_h) * 2.0f;
}

// Draw a rounded white background behind the main toolbar using ImGui draw list.
static inline void draw_toolbar_bg_round_rect_px(
    float left_px, float top_px, float right_px, float bottom_px,
    float radius_px, float border_px,
    ImU32 fill_col, ImU32 border_col,
    bool drop_shadow, float shadow_px, ImU32 shadow_col)
{
    ImDrawList* bg = ImGui::GetBackgroundDrawList();

    // Optional shadow (simple blurred-like outer rect; cheap & good enough)
    if (drop_shadow && shadow_px > 0.f) {
        ImVec2 s0(left_px - shadow_px, top_px - shadow_px);
        ImVec2 s1(right_px + shadow_px, bottom_px + shadow_px);
        bg->AddRectFilled(s0, s1, shadow_col, radius_px + shadow_px);
    }

    // Fill
    ImVec2 p0(left_px,  top_px);
    ImVec2 p1(right_px, bottom_px);
    bg->AddRectFilled(p0, p1, fill_col, radius_px);

    // Border
    if (border_px > 0.f) {
        bg->AddRect(p0, p1, border_col, radius_px, 0, border_px);
    }
}

inline void destroy_imgui_texture(ImTextureID& tex) 
{
    if (!tex) return;
    GLuint id = (GLuint)(uintptr_t)tex;
    glsafe(::glDeleteTextures(1, &id));
    tex = nullptr;
}

inline bool caseInsensitiveFind(const std::string& str, const std::string& substr)
{
    return str.end() != std::search(
        str.begin(), str.end(),
        substr.begin(), substr.end(),
        [](char ch1, char ch2) {
            return std::toupper(ch1) == std::toupper(ch2);
        }
    );
}

}} // namespace
