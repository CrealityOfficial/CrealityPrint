#ifndef _ADAPTIVE_SLICE_1709888420000_H
#define _ADAPTIVE_SLICE_1709888420000_H

#include "trimesh2/TriMesh.h"
#include <vector>
#include "basickernelexport.h"

namespace creative_kernel
{
	BASIC_KERNEL_API std::vector<double> getLayerHeightProfileAdaptive(trimesh::TriMesh* triMesh, float quality);
	BASIC_KERNEL_API std::vector<double> smoothHeightProfile(const std::vector<double>& layer, trimesh::TriMesh* triMesh, unsigned int radius, bool keep_min);
	BASIC_KERNEL_API std::vector<double> generateObjectLayers(const std::vector<double>& layer, trimesh::TriMesh* triMesh);
}

#endif // _ADAPTIVE_SLICE_1709888420000_H
