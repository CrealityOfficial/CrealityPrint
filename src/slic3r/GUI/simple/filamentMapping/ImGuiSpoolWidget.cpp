#include "ImGuiSpoolWidget.hpp"

#include "../../GUI_App.hpp"
#include "../../ImGuiWrapper.hpp"

#include <algorithm>
#include <cmath>

namespace Slic3r {
namespace GUI {

namespace {

static ImU32 rgba(unsigned char r, unsigned char g, unsigned char b, float alpha)
{
    const float a = std::max(0.f, std::min(alpha, 1.f));
    return IM_COL32(r, g, b, (int)std::lround(a * 255.f));
}

struct SpoolTextureSet {
    ImTextureID back     = nullptr;
    ImTextureID filament = nullptr;
    ImTextureID front    = nullptr;
    bool        attempted = false;
};

static SpoolTextureSet& spool_textures()
{
    static SpoolTextureSet set;
    return set;
}

static void ensure_spool_textures_loaded()
{
    SpoolTextureSet& set = spool_textures();
    if (set.attempted)
        return;

    set.attempted = true;
    const std::string base_dir = Slic3r::resources_dir() + "/images/";
    const unsigned tex_w = 224;
    const unsigned tex_h = 160;
    IMTexture::load_from_svg_file(base_dir + "simple_mode_spool_back.svg", tex_w, tex_h, set.back);
    IMTexture::load_from_svg_file(base_dir + "simple_mode_spool_filament.svg", tex_w, tex_h, set.filament);
    IMTexture::load_from_svg_file(base_dir + "simple_mode_spool_front.svg", tex_w, tex_h, set.front);
}

static void draw_spool_widget_fallback(ImDrawList* draw_list, const ImVec2& pos, const ImVec2& size, const ImGuiSpoolVisualSpec& spec)
{
    const float alpha = std::max(0.f, std::min(spec.alpha, 1.f));
    const float w = size.x;
    const float h = size.y;
    const float rounding = h * 0.26f;

    const ImVec2 shadow_min(pos.x + w * 0.06f, pos.y + h * 0.18f);
    const ImVec2 shadow_max(pos.x + w * 0.94f, pos.y + h * 0.90f);
    draw_list->AddRectFilled(shadow_min, shadow_max, rgba(0, 0, 0, alpha * 0.14f), rounding + 2.f);

    const ImVec2 back_min(pos.x + w * 0.14f, pos.y + h * 0.12f);
    const ImVec2 back_max(pos.x + w * 0.44f, pos.y + h * 0.90f);
    const ImVec2 spool_min(pos.x + w * 0.35f, pos.y + h * 0.30f);
    const ImVec2 spool_max(pos.x + w * 0.58f, pos.y + h * 0.70f);
    const ImVec2 front_min(pos.x + w * 0.42f, pos.y + h * 0.08f);
    const ImVec2 front_max(pos.x + w * 0.74f, pos.y + h * 0.92f);

    draw_list->AddRectFilledMultiColor(
        back_min,
        back_max,
        rgba(92, 98, 108, alpha * 0.96f),
        rgba(52, 57, 67, alpha * 0.96f),
        rgba(44, 48, 56, alpha * 0.96f),
        rgba(80, 86, 96, alpha * 0.96f));
    const ImVec4 filament_color = spec.filament_color;
    const ImVec4 filament_dark(
        filament_color.x * 0.55f,
        filament_color.y * 0.55f,
        filament_color.z * 0.55f,
        1.f);
    draw_list->AddRectFilledMultiColor(
        spool_min,
        spool_max,
        ImGui::GetColorU32(ImVec4(filament_dark.x, filament_dark.y, filament_dark.z, alpha)),
        ImGui::GetColorU32(ImVec4(filament_color.x, filament_color.y, filament_color.z, alpha)),
        ImGui::GetColorU32(ImVec4(filament_color.x, filament_color.y, filament_color.z, alpha)),
        ImGui::GetColorU32(ImVec4(filament_dark.x, filament_dark.y, filament_dark.z, alpha)));

    for (int i = 1; i < 11; ++i) {
        const float t = (float)i / 11.f;
        const float y = spool_min.y + (spool_max.y - spool_min.y) * t;
        draw_list->AddLine(
            ImVec2(spool_min.x + 1.f, y),
            ImVec2(spool_max.x - 1.f, y),
            rgba(255, 255, 255, alpha * (i % 2 == 0 ? 0.10f : 0.06f)),
            1.f);
    }

    draw_list->AddRectFilledMultiColor(
        front_min,
        front_max,
        rgba(70, 76, 86, alpha),
        rgba(22, 25, 32, alpha),
        rgba(18, 21, 28, alpha),
        rgba(52, 58, 68, alpha));

    const ImVec2 hub_min(pos.x + w * 0.53f, pos.y + h * 0.32f);
    const ImVec2 hub_max(pos.x + w * 0.66f, pos.y + h * 0.68f);
    draw_list->AddRectFilledMultiColor(
        hub_min,
        hub_max,
        rgba(26, 29, 36, alpha),
        rgba(6, 8, 12, alpha),
        rgba(6, 8, 12, alpha),
        rgba(18, 21, 26, alpha));
}

} // namespace

void draw_spool_widget(ImDrawList* draw_list, const ImVec2& pos, const ImVec2& size, const ImGuiSpoolVisualSpec& spec)
{
    if (draw_list == nullptr)
        return;

    ensure_spool_textures_loaded();
    const SpoolTextureSet& set = spool_textures();

    const ImVec4 tint(spec.filament_color.x, spec.filament_color.y, spec.filament_color.z, std::max(0.f, std::min(spec.alpha, 1.f)));
    const ImU32 base_col = ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, tint.w));
    const ImU32 filament_col = ImGui::GetColorU32(tint);
    const ImVec2 max(pos.x + size.x, pos.y + size.y);
    // Crop away transparent padding in the source art so the reel body fills the widget box.
    const ImVec2 uv_min(0.08f, 0.08f);
    const ImVec2 uv_max(0.90f, 0.94f);

    if (set.back != nullptr && set.filament != nullptr && set.front != nullptr) {
        draw_list->AddImage(set.back, pos, max, uv_min, uv_max, base_col);
        draw_list->AddImage(set.filament, pos, max, uv_min, uv_max, filament_col);
        draw_list->AddImage(set.front, pos, max, uv_min, uv_max, base_col);
        return;
    }

    draw_spool_widget_fallback(draw_list, pos, size, spec);
}

} // namespace GUI
} // namespace Slic3r
