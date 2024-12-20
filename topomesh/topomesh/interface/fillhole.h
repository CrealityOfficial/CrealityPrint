#ifndef TOPOMESH_FILLHOLE_1697178498734_H
#define TOPOMESH_FILLHOLE_1697178498734_H
#include "topomesh/interface/idata.h"

namespace topomesh
{
	TOPOMESH_API void findHoles(trimesh::TriMesh* mesh, /*out*/ std::vector<std::vector<int>>& holes);
}

#endif // TOPOMESH_FILLHOLE_1697178498734_H