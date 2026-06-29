#pragma once

#include <cstdint>
#include <vector>

#include "imgui/imgui.h"
#include "ThumbnailDataRecolor.hpp"

namespace Slic3r {
struct ThumbnailData;
namespace GUI {

class ImGuiThumbnailPreview final
{
public:
    ImGuiThumbnailPreview() = default;
    ImGuiThumbnailPreview(const ImGuiThumbnailPreview&) = delete;
    ImGuiThumbnailPreview& operator=(const ImGuiThumbnailPreview&) = delete;
    ImGuiThumbnailPreview(ImGuiThumbnailPreview&&) = delete;
    ImGuiThumbnailPreview& operator=(ImGuiThumbnailPreview&&) = delete;

    ~ImGuiThumbnailPreview();

    void reset();
    void draw(const ThumbnailData* thumbnail, const ImVec2& size, float scale);
    void draw_recolored(const ThumbnailData* lit_thumbnail,
                        const ThumbnailData* no_light_thumbnail,
                        const std::vector<RGB8>& extruder_colors,
                        const ThumbnailRecolorParams& params,
                        const ImVec2& size,
                        float scale);

private:
    void update_texture(const ThumbnailData& td);
    void update_texture(const ThumbnailData& td, unsigned& tex, unsigned& w, unsigned& h, uint64_t& sig, int filter);

    bool ensure_recolor_program();
    bool update_recolored_texture(const ThumbnailData& lit_td,
                                  const ThumbnailData& no_light_td,
                                  const std::vector<RGB8>& extruder_colors,
                                  const ThumbnailRecolorParams& params);

    static uint64_t signature(const ThumbnailData& td);
    static uint64_t signature(const std::vector<RGB8>& extruder_colors, const ThumbnailRecolorParams& params);

private:
    unsigned m_tex = 0;
    unsigned m_w = 0;
    unsigned m_h = 0;
    uint64_t m_sig = 0;

    unsigned m_lit_tex = 0;
    unsigned m_lit_w   = 0;
    unsigned m_lit_h   = 0;
    uint64_t m_lit_sig = 0;

    unsigned m_no_light_tex = 0;
    unsigned m_no_light_w   = 0;
    unsigned m_no_light_h   = 0;
    uint64_t m_no_light_sig = 0;

    unsigned m_recolored_tex = 0;
    unsigned m_recolored_w   = 0;
    unsigned m_recolored_h   = 0;
    uint64_t m_recolored_sig = 0;

    unsigned m_recolor_fbo  = 0;
    unsigned m_recolor_vao  = 0;
    unsigned m_recolor_vbo  = 0;
};

} // namespace GUI
} // namespace Slic3r
