#ifndef MSBASE_INTERSECT_1695465928791_H
#define MSBASE_INTERSECT_1695465928791_H
#include "msbase/interface.h"
#include "trimesh2/Vec.h"

namespace msbase
{
	MSBASE_API bool rayIntersectTriangle(const trimesh::vec3& orig, const trimesh::vec3& dir,
		trimesh::vec3& v0, trimesh::vec3& v1, trimesh::vec3& v2, float* t, float* u, float* v);
	MSBASE_API bool rayIntersectTriangle(const trimesh::vec3& orig, const trimesh::vec3& dir,
		trimesh::vec3& v0, trimesh::vec3& v1, trimesh::vec3& v2, trimesh::vec3& baryPosition);

	MSBASE_API bool rayIntersectPlane(const trimesh::vec3& orig, const trimesh::vec3& dir, trimesh::vec3& vertex, trimesh::vec3& normal, float& t);
	MSBASE_API bool PointinTriangle(const trimesh::vec3& A, const trimesh::vec3& B, const trimesh::vec3& C, const trimesh::vec3& P);
}

#endif // MSBASE_INTERSECT_1695465928791_H