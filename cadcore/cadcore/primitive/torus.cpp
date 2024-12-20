#include "torus.h"
#include <assert.h>

#include "cadcore/internal/util.h"

#include "BRepPrimAPI_MakeTorus.hxx"
#include "trimesh2/TriMesh_algo.h"

namespace cadcore
{
	trimesh::TriMesh* createTorus(double r1, double r2)
	{
		BRepPrimAPI_MakeTorus maker(1, r2 / r1);
		maker.Build();

		assert(maker.IsDone());

		const TopoDS_Shape& shape = maker.Shape();
		trimesh::TriMesh* mesh = Shape_Triangulation(shape, 0.1);

		if (mesh)
		{
			trimesh::xform xf = trimesh::xform::scale(r1, r1, r1);
			trimesh::apply_xform(mesh, xf);
		}

		return mesh;
	}
}