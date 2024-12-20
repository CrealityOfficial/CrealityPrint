#include "prism.h"
#include <assert.h>

#include "cadcore/internal/util.h"

#include "BRepPrimAPI_MakePrism.hxx"
#include "BRepBuilderAPI_MakeEdge.hxx"
#include "BRepBuilderAPI_MakeWire.hxx"
#include "BRepBuilderAPI_MakeFace.hxx"
#include "trimesh2/TriMesh_algo.h"
#include "gp_Ax2.hxx"
#include "gp_Elips.hxx"
#include "gp_Pln.hxx"
#include "gp_XYZ.hxx"
#include "GeomAPI_PointsToBSpline.hxx"
#include "Geom_BSplineCurve.hxx"
namespace cadcore
{
	trimesh::TriMesh* createPrism(double r)
	{
      //  创建线性扫掠体

		//TColgp_Array1OfPnt array(1, 5);
		//array.SetValue(1, gp_Pnt(0, 0, 0));
		//array.SetValue(2, gp_Pnt(1, 2, 0));
		//array.SetValue(3, gp_Pnt(2, 3, 0));
		//array.SetValue(4, gp_Pnt(4, 3, 0));
		//array.SetValue(5, gp_Pnt(5, 5, 0));
		//TColgp_Array1OfPnt array(1,4);
		//array.SetValue(1, gp_Pnt(0, 0, 0));
		//array.SetValue(2, gp_Pnt(1, 1, 0));
		//array.SetValue(3, gp_Pnt(2, 0, 0));
		//array.SetValue(4, gp_Pnt(0, 0, 0));
		//
		//
		//
		// GeomAPI_PointsToBSpline spline = GeomAPI_PointsToBSpline(array);
		// Handle(Geom_BSplineCurve) curve = spline.Curve();
		// BRepBuilderAPI_MakeEdge profile (curve);
		//
		//	/////# the linear path
		//gp_Pnt	starting_point (0.0, 0.0, 0.0);
		//gp_Pnt end_point (0.0, 0.0, 6.0);
		//gp_Vec vec  (starting_point, end_point);
		//BRepBuilderAPI_MakeEdge  path(starting_point, end_point);
		//path .Edge();
		//
		//	//# extrusion
		//BRepPrimAPI_MakePrism prism(profile, vec);
		//prism.Shape();
		//
		//
		//trimesh::TriMesh* mesh = Shape_Triangulation(prism, 0.1);
		//if (mesh)
		//{
		//	trimesh::xform xf = trimesh::xform::scale((double)r);
		//	trimesh::apply_xform(mesh, xf);
		//}
		//
		//return mesh;
		return nullptr;
	}
}