#ifndef slic3r_WipeTowerPlacement_hpp_
#define slic3r_WipeTowerPlacement_hpp_

#include "Polygon.hpp"
#include <optional>
#include <functional>

namespace Slic3r {
// All coordinates are scaled and relative to one plate. The local footprint
// includes rotation, reinforcement and brim; obstacles are convex hulls.
std::optional<Point> nearest_wipe_tower_position(const Polygon& footprint, const Polygon& bed,
                                                const Polygons& obstacles, const Point& anchor);

// Drag clearance is measured from the rotated brim to the printable boundary.
std::optional<Point> dragged_wipe_tower_position(const Polygon& footprint, Polygon bed,
                                                 const Point& anchor, bool is_sliced);

struct WipeTowerPose {
    Point anchor;
    double rotation_degrees;
};

// Try the saved orientation even when it only fits without the preferred clearance.
std::optional<WipeTowerPose> find_loaded_wipe_tower_pose(const Polygon& local_footprint, const Polygon& bed,
    const Polygons& obstacles, WipeTowerPose saved, bool allow_rotation, const std::function<bool()>& canceled = {});

// A refresh or an explicit position edit must not undo manual placement.
// The first observation establishes a baseline, including after load/undo.
struct WipeTowerPlacementObservation {
    Polygon footprint;
    Point anchor = Point(0, 0);
    bool initialized = false;

    bool observe(const Polygon& next_footprint, const Point& next_anchor)
    {
        const bool changed = initialized && !next_footprint.empty() &&
                             footprint.points != next_footprint.points && anchor == next_anchor;
        footprint = next_footprint;
        anchor = next_anchor;
        initialized = true;
        return changed;
    }
};
}
#endif
