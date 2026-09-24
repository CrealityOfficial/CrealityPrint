#ifndef slic3r_FillAIInfill_hpp_
#define slic3r_FillAIInfill_hpp_

#include <cmath>
#include <vector>

#include "../libslic3r.h"

namespace Slic3r {
namespace AIInfill {

// These are physical distances, not fixed integer-coordinate constants.
// Convert on each call: SCALING_FACTOR can change with printer precision.
inline std::vector<coord_t> depth_thresholds()
{
    return {
        static_cast<coord_t>(std::llround(scale_(-1.0))),
        static_cast<coord_t>(std::llround(scale_(-1.5))),
        static_cast<coord_t>(std::llround(scale_(-3.0))),
        static_cast<coord_t>(std::llround(scale_(-5.0)))
    };
}

inline coord_t max_grid_edge()
{
    return static_cast<coord_t>(std::llround(scale_(10.0)));
}

} // namespace AIInfill
} // namespace Slic3r

#endif
