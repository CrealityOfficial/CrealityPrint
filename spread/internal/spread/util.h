#ifndef SLIC3RWRAPPER_UTIL_1632383314974_H
#define SLIC3RWRAPPER_UTIL_1632383314974_H

#include "stl.h"
#include "Slice3rBase/ExPolygon.hpp"
#include "trimesh2/Vec.h"

namespace Slic3r {
	class TriangleMesh;
}

namespace trimesh {
	class TriMesh;
}

namespace ccglobal
{
	class Tracer;
}

namespace spread
{
	Slic3r::TriangleMesh* trimesh2Slic3rTriangleMesh(trimesh::TriMesh* mesh, ccglobal::Tracer* tracer = nullptr);
	void trimesh2IndexTriangleSet(trimesh::TriMesh* mesh, indexed_triangle_set& indexTriangleSet, ccglobal::Tracer* tracer = nullptr);
	Slic3r::TriangleMesh* constructTriangleMeshFromIndexTriangleSet(const indexed_triangle_set& itset, ccglobal::Tracer* tracer = nullptr);


	trimesh::TriMesh* slic3rMesh2TriMesh(const Slic3r::TriangleMesh& mesh);

	Slic3r::TriangleMesh* simpleConvert(trimesh::TriMesh* mesh, ccglobal::Tracer* tracer = nullptr);
	void indexed2TriangleSoup(const indexed_triangle_set& indexed, std::vector<trimesh::vec3>& triangles);

	inline trimesh::vec3 toVector(const stl_vertex& vertex)
	{
		return trimesh::vec3(vertex.x(), vertex.y(), vertex.z());
	}
}

#endif // SLIC3RWRAPPER_UTIL_1632383314974_H