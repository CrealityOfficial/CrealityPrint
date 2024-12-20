#ifndef VARY_XYD274874
#define VARY_XYD274874
#include "communication/sliceDataStorage.h"
#include "utils/polygon.h"
#include "utils/Simplify.h"
#include "utils/VoronoiUtils.h"

namespace cura52
{

   Polygons  generateVaryingXYDisallowedArea(const SliceMeshStorage& storage, const Settings& infill_settings, const LayerIndex layer_idx);
}

#endif