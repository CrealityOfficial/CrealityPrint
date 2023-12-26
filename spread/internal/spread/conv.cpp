#include "spread/conv.h"
#include "util.h"
#include "Slice3rBase/TriangleMesh.hpp"

namespace spread
{
	void testConvertIndexed(trimesh::TriMesh* mesh, ccglobal::Tracer* tracer)
	{
		indexed_triangle_set indexed_set;
		trimesh2IndexTriangleSet(mesh, indexed_set, tracer);
	}

	void testConvertTriangleMesh(trimesh::TriMesh* mesh, ccglobal::Tracer* tracer)
	{
		std::shared_ptr<Slic3r::TriangleMesh> slic3rMesh(simpleConvert(mesh, tracer));
	}
}