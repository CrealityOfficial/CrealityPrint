#ifndef _ADAPTIVE_SLICE_1709888420000_H
#define _ADAPTIVE_SLICE_1709888420000_H

#include "trimesh2/TriMesh.h"
#include <vector>
#include <memory>
#include "basickernelexport.h"

namespace creative_kernel
{
	using TriMeshPtr = std::shared_ptr<trimesh::TriMesh>;
	BASIC_KERNEL_API std::vector<double> getLayerHeightProfileAdaptive(std::vector<TriMeshPtr> triMesh, float quality);
	BASIC_KERNEL_API std::vector<double> smoothHeightProfile(const std::vector<double>& layer, std::vector<TriMeshPtr> triMesh, unsigned int radius, bool keep_min);
	BASIC_KERNEL_API std::vector<double> generateObjectLayers(const std::vector<double>& layer, std::vector<TriMeshPtr> triMesh);
}

#endif // _ADAPTIVE_SLICE_1709888420000_H
