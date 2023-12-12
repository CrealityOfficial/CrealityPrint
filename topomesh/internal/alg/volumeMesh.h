#pragma once
#include "trimesh2/TriMesh.h"

namespace topomesh {
	float getMeshTotalVolume(trimesh::TriMesh* mesh);
	float getMeshVolume(trimesh::TriMesh* mesh,std::vector<int>& faces);
	float getPointCloudVolume(trimesh::TriMesh* mesh,std::vector<int>& vexter);
}