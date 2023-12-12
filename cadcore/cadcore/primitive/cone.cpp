#include "cone.h"
#include <assert.h>

#include "cadcore/internal/util.h"

#include "BRepPrimAPI_MakeCone.hxx"
#include "trimesh2/TriMesh_algo.h"

namespace cadcore
{
	trimesh::TriMesh* createCone(double r1, double r2, double h)
	{
		BRepPrimAPI_MakeCone maker(1, r2/r1,1);
		maker.Build();

		assert(maker.IsDone());

		const TopoDS_Shape& shape = maker.Shape();
		trimesh::TriMesh* mesh = Shape_Triangulation(shape, 0.1);

		if (mesh)
		{
			trimesh::xform xf = trimesh::xform::scale(r1, r1, h);
			trimesh::apply_xform(mesh, xf);
		}

		return mesh;
	}
}