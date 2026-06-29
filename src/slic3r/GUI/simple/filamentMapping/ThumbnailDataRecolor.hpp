#pragma once

#include <cstdint>
#include <vector>

namespace Slic3r {
struct ThumbnailData;

namespace GUI {

struct RGB8 {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
};

struct ThumbnailRecolorParams {
    float emission_factor = 0.1f;
};

// Recolor a lit thumbnail in-place using a no-light thumbnail as mask/reference.
// - `lit_reference` is the original lit thumbnail (kept unchanged).
// - `no_light_reference` is the no-light thumbnail; its alpha channel encodes extruder index (0-based): idx = 255 - alpha.
// - `extruder_colors[idx]` provides the desired RGB for that extruder index.
// On success, `in_out_thumbnail` becomes the recolored image (same w/h as references).
bool recolor_thumbnail_with_no_light(ThumbnailData&                 in_out_thumbnail,
                                    const ThumbnailData&           lit_reference,
                                    const ThumbnailData&           no_light_reference,
                                    const std::vector<RGB8>&       extruder_colors,
                                    const ThumbnailRecolorParams&  params = {});

} // namespace GUI
} // namespace Slic3r

