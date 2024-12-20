#ifndef creative_kernel_OPERATION_UTIL_H
#define creative_kernel_OPERATION_UTIL_H
#include "basickernelexport.h"
#include "trimesh2/Box.h"
#include "qtuser3d/math/box3d.h"
#include "qtuser3d/math/ray.h"

namespace creative_kernel
{
	bool checkLayerHeightEqual(const std::vector<std::vector<double>>& objectsLayers);
}
#endif // creative_kernel_OPERATION_UTIL_H