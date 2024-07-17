#pragma once
#include "topomesh/interface/idata.h"
namespace topomesh {
	TOPOMESH_API float getMeshTotalVolume(trimesh::TriMesh* mesh);
	TOPOMESH_API float getMeshVolume(trimesh::TriMesh* mesh,std::vector<int>& faces);
	TOPOMESH_API float getPointCloudVolume(trimesh::TriMesh* mesh,std::vector<int>& vexter);
}