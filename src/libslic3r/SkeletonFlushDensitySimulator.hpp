#ifndef slic3r_SkeletonFlushDensitySimulator_hpp_
#define slic3r_SkeletonFlushDensitySimulator_hpp_

#include "libslic3r.h"

namespace Slic3r {

// Portion of the available purge volume that is intentionally absorbed by
// the object skeleton.  The remainder stays available for the wipe tower (or
// other regular infill targets).
constexpr float kSkeletonFlushShare = 0.5f;

class LayerRegion;

struct SkeletonFlushDensityResult
{
    bool   replaced          = false;
    bool   target_met        = false;
    float  selected_density  = 0.f;
    float  base_volume       = 0.f;
    float  simulated_volume  = 0.f;
    float  required_volume   = 0.f;
    size_t simulation_count  = 0;
};

// Rebuild the Locked-Zag skeleton of a layer region. Check max_density first,
// then use a discrete binary search to select the lowest density that can absorb
// at least 50% of current_flush_volume in addition to its current skeleton volume.
SkeletonFlushDensityResult replace_skeleton_with_flush_density(
    LayerRegion& layer_region,
    float current_flush_volume,
    float density_step = 0.01f,
    float max_density  = 0.99f);

} // namespace Slic3r

#endif // slic3r_SkeletonFlushDensitySimulator_hpp_
