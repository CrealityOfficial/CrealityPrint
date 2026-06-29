#include "ThumbnailDataRecolor.hpp"

#include <algorithm>
#include <cmath>

#include <tbb/parallel_for.h>

#include "libslic3r/GCode/ThumbnailData.hpp"

namespace Slic3r {
namespace GUI {

namespace {

static inline float u8_to_f(std::uint8_t v) { return (float)v * (1.0f / 255.0f); }

static inline std::uint8_t f_to_u8(float v)
{
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    return (std::uint8_t)std::lround(v * 255.0f);
}

} // namespace

bool recolor_thumbnail_with_no_light(ThumbnailData&                in_out_thumbnail,
                                    const ThumbnailData&          lit_reference,
                                    const ThumbnailData&          no_light_reference,
                                    const std::vector<RGB8>&      extruder_colors,
                                    const ThumbnailRecolorParams& params)
{
    if (!lit_reference.is_valid() || !no_light_reference.is_valid())
        return false;
    if (lit_reference.width != no_light_reference.width || lit_reference.height != no_light_reference.height)
        return false;
    if (lit_reference.pixels.size() != no_light_reference.pixels.size())
        return false;
    if ((lit_reference.width == 0) || (lit_reference.height == 0))
        return false;
    if (lit_reference.pixels.size() != (size_t)lit_reference.width * (size_t)lit_reference.height * 4)
        return false;

    // Start from the lit reference (preserve its alpha/background).
    in_out_thumbnail.set(lit_reference.width, lit_reference.height);
    in_out_thumbnail.pixels = lit_reference.pixels;

    if (extruder_colors.empty())
        return true;

    float emission_factor = params.emission_factor;
    if (emission_factor < 0.0f) emission_factor = 0.0f;
    if (emission_factor > 1.0f) emission_factor = 1.0f;

    struct ColorPre {
        float tr = 0.0f;
        float tg = 0.0f;
        float tb = 0.0f;
        float brightness = 0.0f;
        float diffuse_coef = 1.0f;
        float highlight_coef = 0.8f;
        bool  is_black = false;
    };

    std::vector<ColorPre> pre;
    pre.reserve(extruder_colors.size());
    for (const auto& c : extruder_colors) {
        ColorPre p;
        p.tr = u8_to_f(c.r);
        p.tg = u8_to_f(c.g);
        p.tb = u8_to_f(c.b);
        p.brightness = 0.299f * p.tr + 0.587f * p.tg + 0.114f * p.tb;
        p.is_black = (p.brightness < 0.01f);
        if (!p.is_black) {
            // Keep the original (lit) brightness for non-black colors by making
            // `diffuse + emission ~= 1` when intensity_x == 1.
            p.diffuse_coef = 1.0f - emission_factor;
            p.highlight_coef = 1.2f;
        }
        pre.push_back(p);
    }

    const float eps             = 1e-6f;

    const size_t px_count = (size_t)lit_reference.width * (size_t)lit_reference.height;
    const size_t grain = std::max<size_t>((size_t)lit_reference.width, 4096);
    tbb::parallel_for(tbb::blocked_range<size_t>(0, px_count, grain), [&](const tbb::blocked_range<size_t>& range) {
        for (size_t i = range.begin(); i < range.end(); ++i) {
            const size_t off = 4 * i;

            const std::uint8_t a_nl = no_light_reference.pixels[off + 3];
            const int          idx  = 255 - (int)a_nl; // 0-based extruder index, background => 255
            if (idx < 0 || idx >= (int)pre.size())
                continue;

            const ColorPre& c = pre[(size_t)idx];
            const float tr = c.tr;
            const float tg = c.tg;
            const float tb = c.tb;

            const float pr = u8_to_f(lit_reference.pixels[off + 0]);
            const float pg = u8_to_f(lit_reference.pixels[off + 1]);
            const float pb = u8_to_f(lit_reference.pixels[off + 2]);

            const float nr = u8_to_f(no_light_reference.pixels[off + 0]);
            const float ng = u8_to_f(no_light_reference.pixels[off + 1]);
            const float nb = u8_to_f(no_light_reference.pixels[off + 2]);

            const float dr = std::min(nr, pr);
            const float dg = std::min(ng, pg);
            const float db = std::min(nb, pb);

            const float sr = pr - dr;
            const float sg = pg - dg;
            const float sb = pb - db;

            // Estimate diffuse intensity as mean over non-zero channels to avoid dimming saturated colors.
            float ix_sum = 0.0f;
            int   ix_n   = 0;
            if (nr > eps) {
                ix_sum += dr / nr;
                ++ix_n;
            }
            if (ng > eps) {
                ix_sum += dg / ng;
                ++ix_n;
            }
            if (nb > eps) {
                ix_sum += db / nb;
                ++ix_n;
            }
            const float intensity_x = (ix_n > 0) ? (ix_sum / (float)ix_n) : 0.0f;
            float       intensity_y = (sr + sg + sb) / 3.0f;

            const float diffuse_coef   = c.diffuse_coef;
            const float highlight_coef = c.highlight_coef;
            if (c.is_black) {
                const float base_light = 0.02f;
                if (intensity_y <= 0.0f)
                    intensity_y = intensity_x * 0.1f + base_light;
                else if (intensity_y >= 0.001f)
                    intensity_y = intensity_x * 0.1f + 0.03f;
            }

            if (intensity_y < 0.0f)
                intensity_y = 0.0f;

            const float rr = tr * intensity_x * diffuse_coef + intensity_y * highlight_coef + tr * emission_factor;
            const float rg = tg * intensity_x * diffuse_coef + intensity_y * highlight_coef + tg * emission_factor;
            const float rb = tb * intensity_x * diffuse_coef + intensity_y * highlight_coef + tb * emission_factor;

            in_out_thumbnail.pixels[off + 0] = f_to_u8(rr);
            in_out_thumbnail.pixels[off + 1] = f_to_u8(rg);
            in_out_thumbnail.pixels[off + 2] = f_to_u8(rb);
            // keep alpha from lit reference (already copied)
        }
    });

    return true;
}

} // namespace GUI
} // namespace Slic3r
