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
		//debug
		std::string fileName;
	};

	MSBASE_API bool planeCut(trimesh::TriMesh* input, const CutPlane& plane,
		std::vector<trimesh::TriMesh*>& outMeshes, const CutParam& param = CutParam());

	//�и�����
	MSBASE_API bool splitRangeZ(trimesh::TriMesh* inputMesh, float Upz, float Dowmz, trimesh::TriMesh** mesh, const char* fileName = nullptr);

	MSBASE_API bool planeCutFromFile(const std::string& fileName, std::vector<trimesh::TriMesh*>& outMeshes);

	MSBASE_API bool splitRangeZFromFile(const std::string& fileName, trimesh::TriMesh** mesh);
}

#endif // MMESH_BASE_CUT_1612341980784_H
