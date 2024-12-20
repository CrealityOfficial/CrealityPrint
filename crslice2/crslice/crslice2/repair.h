#ifndef CRSLICE_SLICE_REPAIR_H_2
#define CRSLICE_SLICE_REPAIR_H_2
#include "crslice2/interface.h"
#include "crslice2/header.h"
#include <string>

namespace crslice2
{
	struct CheckInfo
	{
		std::string tooltip;
		std::string warning_icon_name;
	};

	CRSLICE2_API CheckInfo checkMesh(trimesh::TriMesh* mesh);
	CRSLICE2_API trimesh::TriMesh* repairMesh(trimesh::TriMesh* mesh, ccglobal::Tracer* tracer = nullptr);
	CRSLICE2_API bool cut_horizontal(trimesh::TriMesh* in_mesh, float z, std::vector<trimesh::TriMesh*>& outMeshes,int attributes);


}
#endif // CRSLICE_SLICE_REPAIR_H_2