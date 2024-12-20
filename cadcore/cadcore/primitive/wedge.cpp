#include "wedge.h"
#include <assert.h>

#include "cadcore/internal/util.h"

#include "BRepPrimAPI_MakeWedge.hxx"
#include "trimesh2/TriMesh_algo.h"

namespace cadcore
{
	trimesh::TriMesh* createWedge(double r1, double r2, double r3, double r4)
	{
		BRepPrimAPI_MakeWedge maker( r1,  r2,  r3,  r4);
		maker.Build();

		assert(maker.IsDone());

		const TopoDS_Shape& shape = maker.Shape();
		trimesh::TriMesh* mesh = Shape_Triangulation(shape, 0.1);

		if (mesh)
		{
			trimesh::xform xf = trimesh::xform::scale((double)1);
			trimesh::apply_xform(mesh, xf);
		}

		return mesh;
	}
}