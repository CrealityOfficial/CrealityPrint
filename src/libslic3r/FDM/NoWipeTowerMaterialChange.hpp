#ifndef slic3r_NoWipeTowerMaterialChange_hpp_
#define slic3r_NoWipeTowerMaterialChange_hpp_

#include "../ExtrusionEntity.hpp"
#include "../Point.hpp"

namespace Slic3r {

class Print;
class PrintConfig;
class PrintRegionConfig;

struct NoWipeTowerMaterialChangeEntrance
{
    bool  valid      = false;
    Point start      = Point::Zero();
    Point toolchange = Point::Zero();
    // Toolchange point back to the selected continuous path's own start.
    // This may differ from start when a later path supplied the parking point.
    Polyline reverse_wipe;
};

// Shared by no-tower skin generation and skeleton capacity planning. The
// configured depth is kept verbatim only above 85% host-wall brightness.
float no_wipe_tower_skin_infill_depth(float configured_depth_mm, float grayscale);
float no_wipe_tower_skin_infill_depth(const PrintConfig& print_config, const PrintRegionConfig& region_config);

bool no_wipe_tower_no_purge_mode_enabled(const Print& print);
// Select by the tower switch, including single-filament and non-planning paths.
float configured_skeleton_infill_density_percent(const PrintConfig& print_config, const PrintRegionConfig& region_config);
// A native skin/skeleton fill needs no purge protection when only one physical
// filament is used. This does not enable multicolor packing or change patterns.
bool no_wipe_tower_single_filament_skin_disabled(const Print& print);
bool flush_into_skeleton_packing_mode_enabled(const Print& print);
bool flush_into_skeleton_force_external_fill_flush(const Print& print, coordf_t print_z);

NoWipeTowerMaterialChangeEntrance no_wipe_tower_material_change_entrance(
    ExtrusionEntitiesPtr candidates,
    const Point&         start_near,
    double               scaled_distance);

} // namespace Slic3r

#endif // slic3r_NoWipeTowerMaterialChange_hpp_
