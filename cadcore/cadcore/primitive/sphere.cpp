#include "sphere.h"
#include <assert.h>

#include "cadcore/internal/util.h"

#include "BRepPrimAPI_MakeSphere.hxx"
#include "trimesh2/TriMesh_algo.h"

namespace cadcore
{
	trimesh::TriMesh* makeSphere(float r)
	{
		BRepPrimAPI_MakeSphere maker(1.0);
		maker.Build();

		assert(maker.IsDone());

		const TopoDS_Shape& shape = maker.Shape();
		trimesh::TriMesh* mesh = Shape_Triangulation(shape, 0.1);

		if (mesh)
		{
			trimesh::xform xf = trimesh::xform::scale((double)r);
			trimesh::apply_xform(mesh, xf);
		}

		return mesh;
	}
}