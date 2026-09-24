#ifndef slic3r_Fill_Tower_FillLayerFilamentWipePacking_hpp_
#define slic3r_Fill_Tower_FillLayerFilamentWipePacking_hpp_

#include "../../libslic3r.h"

#include <map>
#include <set>

namespace Slic3r {

class LayerRegion;
class Print;
class Surface;
class ToolOrdering;

namespace FillTower {

struct LayerFilamentWipePackingTransition
{
    coord_t      print_z      = 0;
    unsigned int old_extruder = (unsigned int)-1;
    unsigned int new_extruder = (unsigned int)-1;

    bool operator==(const LayerFilamentWipePackingTransition& rhs) const
    {
        return print_z == rhs.print_z && old_extruder == rhs.old_extruder && new_extruder == rhs.new_extruder;
    }

    bool operator<(const LayerFilamentWipePackingTransition& rhs) const
    {
        if (print_z != rhs.print_z)
            return print_z < rhs.print_z;
        if (old_extruder != rhs.old_extruder)
            return old_extruder < rhs.old_extruder;
        return new_extruder < rhs.new_extruder;
    }
};

using LayerFilamentWipePackingTransitionRegionVolumes =
    std::map<const LayerRegion*, std::map<unsigned int, float>>;

struct LayerFilamentWipePackingPlan
{
    std::map<const Surface*, float>                    sparse_infill_densities;
    std::map<const Surface*, float>                    skin_depths;
    std::set<const Surface*>                           plain_infill_surfaces;
    std::set<LayerFilamentWipePackingTransition>       infill_transitions;
    std::set<LayerFilamentWipePackingTransition>       external_flush_transitions;
    std::map<LayerFilamentWipePackingTransition,
             LayerFilamentWipePackingTransitionRegionVolumes> transition_region_volumes;
};

LayerFilamentWipePackingTransition layer_filament_wipe_packing_transition_key(
    coordf_t print_z, unsigned int old_extruder, unsigned int new_extruder);

LayerFilamentWipePackingPlan plan_layer_filament_wipe_packing(
    const Print& print, const ToolOrdering& tool_ordering,
    const std::set<LayerFilamentWipePackingTransition>& blocked_transitions = {},
    bool use_actual_trajectory_capacity = false,
    bool perform_density_search = true);

} // namespace FillTower

} // namespace Slic3r

#endif // slic3r_Fill_Tower_FillLayerFilamentWipePacking_hpp_
