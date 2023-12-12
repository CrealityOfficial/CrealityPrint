#include "anulus.h"
#include <assert.h>
#include "trimesh2/TriMesh_algo.h"
#include "cadcore/internal/util.h"
#include "gp_Circ.hxx"
#include "Geom_Circle.hxx"
#include "gp_Ax2.hxx"
#include "gp_Sphere.hxx"
#include "BRepBuilderAPI_MakeFace.hxx"

#include "BRepBuilderAPI_MakeEdge.hxx"
#include "BRepBuilderAPI_MakeWire.hxx"
//#include "BRepOffsetAPI_ThruSections.hxx"
#include "TopoDS_Wire.hxx"
namespace cadcore
{
	trimesh::TriMesh* createAnulus(double r1, double r2)
	{
	//	//在xy平面创建1个半径为1的圆

	//	Handle(Geom_Circle) C1 = new Geom_Circle(gp::XOY(), 1.0);

	//	//将xy平面沿z轴平移3，创建1个半径为2的圆

	//	Handle(Geom_Circle) C2 = new Geom_Circle(gp::XOY().Translated(gp_Vec(0, 0, 3)), 2.0);

	//	//将Geom_Circle类型的圆转化为TopoDS_Edge类型

	//	TopoDS_Edge C1_edge = BRepBuilderAPI_MakeEdge(C1);

	//	TopoDS_Edge C2_edge = BRepBuilderAPI_MakeEdge(C2);

	//	//将TopoDS_Edge类型的圆转化为TopoDS_Wire类型

	//	TopoDS_Wire C1_wire = BRepBuilderAPI_MakeWire(C1_edge);

	//	TopoDS_Wire C2_wire = BRepBuilderAPI_MakeWire(C2_edge);

	//	//声明一个放样计算求解器；

	//	BRepOffsetAPI_ThruSections generator;

	//	//添加两个圆

	//	generator.AddWire(C1_wire);

	//	generator.AddWire(C2_wire);

	//	//获得最终的shape

	//	TopoDS_Shape newShape = generator.Shape();

		trimesh::TriMesh* mesh = new trimesh::TriMesh();
		if (mesh)
		{
			trimesh::xform xf = trimesh::xform::scale((double)10);
			trimesh::apply_xform(mesh, xf);
		}

		return mesh;
	}
}