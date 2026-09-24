#ifndef slic3r_NoWipeTowerFlushIntoSkeleton_hpp_
#define slic3r_NoWipeTowerFlushIntoSkeleton_hpp_

#include <optional>

#include "../Point.hpp"

namespace Slic3r {

class ExtrusionEntity;
class LayerRegion;

struct NoWipeTowerFlushIntoSkeletonEntrance
{
    bool   valid         = false;
    Point  start         = Point::Zero();
    Point  toolchange    = Point::Zero();
    double trim_distance = 0.;
};

// Pick the real skeleton start and the point where the no-wipe-tower toolchange
// should happen after moving along the first skeleton path by scaled_distance.
NoWipeTowerFlushIntoSkeletonEntrance no_wipe_tower_flush_into_skeleton_entrance(const ExtrusionEntity& first_entity, double scaled_distance);

// Geometrical plan for the no-wipe-tower "flush into skeleton" receiver. The
// current workflow uses the whole eligible internal receiver as skeleton and
// keeps visible skin/walls outside this planner.
struct NoWipeTowerFlushIntoSkeletonPlan
{
    double skin_depth_mm               = 0.;
    double requested_volume_mm3        = 0.;
    double skeleton_volume_mm3         = 0.;
    double available_region_volume_mm3 = 0.;
};

// Use the whole eligible internal receiver as skeleton. In the no-wipe-tower
// workflow the skeleton is the flush carrier itself and must not be clipped by
// the configured cleaning volume.
std::optional<NoWipeTowerFlushIntoSkeletonPlan> plan_no_wipe_tower_flush_into_skeleton(const LayerRegion& layer_region);

} // namespace Slic3r

#endif // slic3r_NoWipeTowerFlushIntoSkeleton_hpp_