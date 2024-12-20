#pragma once

#include "CUT/interface.h"
#include "trimesh2/TriMesh.h"

namespace CUT
{
	struct CutPlane
	{
		trimesh::vec3 normal = trimesh::vec3(0.0f, 0.0f, 1.0f);
		trimesh::vec3 position = trimesh::vec3(0.0f, 0.0f, 0.0f);
	};

	struct CutParam
	{
		bool fillHole = true;
	};

	CUT_API bool planeCut(trimesh::TriMesh* input, const CutPlane& plane,
		std::vector<trimesh::TriMesh*>& outMeshes, const CutParam& param = CutParam());

	//切割区间
	CUT_API bool splitRangeZ(trimesh::TriMesh* inputMesh, float Upz, float Dowmz, trimesh::TriMesh** mesh);
}
