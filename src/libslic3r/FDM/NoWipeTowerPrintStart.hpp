#ifndef slic3r_NoWipeTowerPrintStart_hpp_
#define slic3r_NoWipeTowerPrintStart_hpp_

#include "../BoundingBox.hpp"
#include "../ExtrusionEntity.hpp"

#include <memory>
#include <vector>

namespace Slic3r {

// Identifies an existing legal endpoint inside one already selected infill
// island. Indices are relative to that source tree and never own its entities.
struct NoWipeTowerPrintStartCandidate
{
    Point               start = Point::Zero();
    std::vector<size_t> child_indices;
    bool                reverse = false;
};

// Ordinary planar skeleton paths only. A non-sortable ancestor freezes the
// whole subtree; otherwise reversible paths may offer their last endpoint.
std::vector<NoWipeTowerPrintStartCandidate> no_wipe_tower_print_start_candidates(
    const ExtrusionEntity& selected_island, size_t max_candidates = 32);

// Match the existing first-island direction and one-level chaining while
// preserving subclass and base-class metadata on the source's deep copy.
std::unique_ptr<ExtrusionEntity> no_wipe_tower_print_start_baseline(
    const ExtrusionEntity& selected_island, const Point& reference, bool reverse = false);

// Region::append may flatten a sortable infill collection into independent
// paths. Recreate that already selected region's baseline output group, with
// the same top-level chain and one-level chaining inside each collection.
std::unique_ptr<ExtrusionEntity> no_wipe_tower_print_start_baseline(
    const ExtrusionEntitiesPtr& selected_region, const Point& reference);

// Deep-copy and promote the selected endpoint without splitting or deleting
// extrusion geometry. Output ordering and directions are locked. A stale or
// illegal candidate returns nullptr so callers can retain their original plan.
std::unique_ptr<ExtrusionEntity> no_wipe_tower_print_start_clone(
    const ExtrusionEntity& selected_island, const NoWipeTowerPrintStartCandidate& candidate);

class NoWipeTowerPrintStartHistory
{
public:
    // layer_index is the actual output-layer ordinal, shared across objects and
    // tools. Locations use platform coordinates in millimetres.
    void advance(size_t layer_index);
    void remember(const Vec2d& world_mm);
    bool separated(const Vec2d& world_mm, double min_distance_mm) const;

private:
    struct Entry {
        size_t layer_index;
        Vec2d  world_mm;
    };
    size_t             m_layer_index = 0;
    bool               m_initialized = false;
    std::vector<Entry> m_entries;
};

} // namespace Slic3r

#endif // slic3r_NoWipeTowerPrintStart_hpp_
