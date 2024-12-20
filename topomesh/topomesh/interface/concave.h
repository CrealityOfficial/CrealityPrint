#ifndef TOPOMESH_POLY_CONCAVE_1692613164094_H
#define TOPOMESH_POLY_CONCAVE_1692613164094_H
#include "topomesh/interface/idata.h"
#include "trimesh2/quaternion.h"

namespace topomesh
{
	TOPOMESH_API void meshConcave(trimesh::TriMesh* mesh, std::vector<trimesh::vec3>& concave,
		const trimesh::quaternion& rotation, const trimesh::vec3& scale);
}

#endif // TOPOMESH_POLY_CONCAVE_1692613164094_H