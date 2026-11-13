#ifndef slic3r_MixedSublayer_hpp_
#define slic3r_MixedSublayer_hpp_

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <utility>
#include <vector>

namespace Slic3r {

// Preserve the layer's total thickness while reserving at least min_height
// for every component. Unconstrained components retain their relative weights.
// Failure leaves heights untouched. Zero disables the limit.
inline bool limit_mixed_sublayer_heights(std::vector<double>& heights, double min_height)
{
    if (heights.empty() || !std::isfinite(min_height) || min_height < 0.)
        return false;
    double total = 0.;
    for (double height : heights) {
        if (!std::isfinite(height) || height < 0.)
            return false;
        total += height;
    }
    if (!std::isfinite(total) || total <= 0.)
        return false;
    if (min_height == 0.)
        return true;

    const double required = min_height * heights.size();
    // Layer profiles may carry float-rounded heights (e.g. 0.08f).
    const double tolerance = 1e-7 * std::max(1., total);
    if (!std::isfinite(required) || required > total + tolerance)
        return false;
    if (std::abs(required - total) <= tolerance) {
        std::fill(heights.begin(), heights.end(), total / heights.size());
        return true;
    }
    if (*std::min_element(heights.begin(), heights.end()) >= min_height)
        return true;

    std::vector<double> adjusted(heights.size(), 0.);
    std::vector<bool> fixed(heights.size(), false);
    double remaining_height = total;
    double remaining_weight = total;
    for (;;) {
        const double scale = remaining_height / remaining_weight;
        bool changed = false;
        for (std::size_t i = 0; i < heights.size(); ++i) {
            if (!fixed[i] && heights[i] * scale < min_height) {
                fixed[i] = true;
                adjusted[i] = min_height;
                remaining_height -= min_height;
                remaining_weight -= heights[i];
                changed = true;
            }
        }
        if (!changed) {
            for (std::size_t i = 0; i < heights.size(); ++i)
                if (!fixed[i])
                    adjusted[i] = heights[i] * scale;
            break;
        }
    }

    // Put floating-point roundoff into the thickest component, which has room
    // above the lower bound, rather than moving every component below its bound.
    *std::max_element(adjusted.begin(), adjusted.end()) +=
        total - std::accumulate(adjusted.begin(), adjusted.end(), 0.);
    heights = std::move(adjusted);
    return true;
}

} // namespace Slic3r

#endif
