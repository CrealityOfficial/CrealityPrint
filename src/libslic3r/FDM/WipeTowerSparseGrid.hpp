#ifndef slic3r_WipeTowerSparseGrid_hpp_
#define slic3r_WipeTowerSparseGrid_hpp_

#include <algorithm>
#include <cmath>

namespace Slic3r {

// Thin wipe lines need shorter unsupported spans: 0.08/0.12/0.16/0.20 mm
// layers use at most 4/6/8/10 mm. Also support the next layer if it is thinner.
inline float wipe_tower_sparse_grid_spacing(float configured_spacing, float layer_height, float next_layer_height)
{
    if (!std::isfinite(configured_spacing) || configured_spacing <= 0.f)
        configured_spacing = 10.f;
    if (!std::isfinite(layer_height) || layer_height <= 0.f)
        layer_height = 0.2f;
    if (std::isfinite(next_layer_height) && next_layer_height > 0.f)
        layer_height = std::min(layer_height, next_layer_height);

    // Bound automatic densification for exceptionally small layer heights.
    const float automatic_spacing = 10.f * std::clamp(layer_height / 0.2f, 0.1f, 1.f);
    return std::min(configured_spacing, automatic_spacing);
}

} // namespace Slic3r

#endif
