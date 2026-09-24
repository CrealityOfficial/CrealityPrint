#ifndef slic3r_NoWipeTowerMaterialChange_hpp_
#define slic3r_NoWipeTowerMaterialChange_hpp_

#include "../ExtrusionEntity.hpp"
#include "../Point.hpp"

namespace Slic3r {

class Print;

struct NoWipeTowerMaterialChangeEntrance
{
    bool  valid      = false;
    Point start      = Point::Zero();
    Point toolchange = Point::Zero();
};

bool flush_into_skeleton_packing_mode_enabled(const Print& print);
bool flush_into_skeleton_force_external_fill_flush(const Print& print, coordf_t print_z);

NoWipeTowerMaterialChangeEntrance no_wipe_tower_material_change_entrance(
    ExtrusionEntitiesPtr candidates,
    const Point&         start_near,
    double               scaled_distance);

} // namespace Slic3r

#endif // slic3r_NoWipeTowerMaterialChange_hpp_
