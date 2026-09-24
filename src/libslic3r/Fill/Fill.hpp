#ifndef slic3r_Fill_hpp_
#define slic3r_Fill_hpp_

#include <memory.h>
#include <memory>
#include <float.h>
#include <stdint.h>
#include <map>

#include "../libslic3r.h"
#include "../PrintConfig.hpp"

#include "FillBase.hpp"

namespace Slic3r {

class ExtrusionEntityCollection;
class Layer;
class LayerRegion;

struct LockedZagSkeletonMetrics
{
    double trajectory_length_mm = 0.;
    double extrusion_volume_mm3 = 0.;
};

using LockedZagSkeletonMetricsByRegion = std::map<const LayerRegion*, LockedZagSkeletonMetrics>;

LockedZagSkeletonMetricsByRegion simulate_layer_filament_wipe_locked_zag(
    const Layer& layer, float skeleton_density_percent);

class LockedZagSkeletonSimulation
{
public:
    explicit LockedZagSkeletonSimulation(const Layer& layer, bool include_solid_as_sparse = false);
    ~LockedZagSkeletonSimulation();

    LockedZagSkeletonSimulation(const LockedZagSkeletonSimulation&) = delete;
    LockedZagSkeletonSimulation& operator=(const LockedZagSkeletonSimulation&) = delete;

    LockedZagSkeletonMetricsByRegion simulate(float skeleton_density_percent) const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

// An interface class to Perl, aggregating an instance of a Fill and a FillData.
class Filler
{
public:
    Filler() : fill(nullptr) {}
    ~Filler() { 
        delete fill; 
        fill = nullptr;
    }
    Fill        *fill;
    FillParams   params;
};

} // namespace Slic3r

#endif // slic3r_Fill_hpp_
