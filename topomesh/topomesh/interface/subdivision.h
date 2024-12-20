#ifndef TOPOMESH_SUBDIVISION_1692613164094_H
#define TOPOMESH_SUBDIVISION_1692613164094_H
#include "topomesh/interface/idata.h"

namespace topomesh {
	TOPOMESH_API void SimpleMidSubdivision(trimesh::TriMesh* mesh, std::vector<int>& faceindexs);
	TOPOMESH_API void loopSubdivision(trimesh::TriMesh* mesh, std::vector<int>& faceindexs,std::vector<int>& outfaces);
}

#endif // TOPOMESH_SUBDIVISION_1692613164094_H