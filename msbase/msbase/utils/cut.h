#ifndef MMESH_BASE_CUT_1612341980784_H
#define MMESH_BASE_CUT_1612341980784_H
#include "msbase/interface.h"
#include "trimesh2/TriMesh.h"

namespace msbase
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

	MSBASE_API bool planeCut(trimesh::TriMesh* input, const CutPlane& plane,
		std::vector<trimesh::TriMesh*>& outMeshes, const CutParam& param = CutParam());

	//ÇÐ¸îÇø¼ä
	MSBASE_API bool splitRangeZ(trimesh::TriMesh* inputMesh, float Upz, float Dowmz, trimesh::TriMesh** mesh);
}

#endif // MMESH_BASE_CUT_1612341980784_H
