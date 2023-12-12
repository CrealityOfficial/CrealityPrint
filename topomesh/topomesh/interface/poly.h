#ifndef TOPOMESH_POLY_1692613164094_H
#define TOPOMESH_POLY_1692613164094_H
#include "topomesh/interface/idata.h"
#include <list>

namespace topomesh
{
	TOPOMESH_API void simplifyPolygons(TriPolygons& polys);

	TOPOMESH_API void generateEdgePolygons(trimesh::TriMesh* mesh, const std::vector<int>& removedFaces,
		/*out*/ TriPolygons& polys);
}

#endif // TOPOMESH_POLY_1692613164094_H