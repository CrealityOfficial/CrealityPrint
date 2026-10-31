#ifndef slic3r_PreviewLayerRange_hpp_
#define slic3r_PreviewLayerRange_hpp_

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>

namespace Slic3r::GUI {

// A slider may still refer to the previous result while a shorter preview is
// being loaded. Empty previews have no valid layer index, including zero.
inline std::optional<std::array<unsigned int, 2>>
clamp_preview_layer_range(const std::array<unsigned int, 2>& requested, std::size_t layer_count)
{
    if (layer_count == 0)
        return std::nullopt;

    const auto upper = static_cast<unsigned int>(
        std::min<std::size_t>(requested[1], layer_count - 1));
    return std::array<unsigned int, 2>{std::min(requested[0], upper), upper};
}

} // namespace Slic3r::GUI

#endif
