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
	//	//��xyƽ�洴��1���뾶Ϊ1��Բ

	//	Handle(Geom_Circle) C1 = new Geom_Circle(gp::XOY(), 1.0);

	//	//��xyƽ����z��ƽ��3������1���뾶Ϊ2��Բ

	//	Handle(Geom_Circle) C2 = new Geom_Circle(gp::XOY().Translated(gp_Vec(0, 0, 3)), 2.0);

	//	//��Geom_Circle���͵�Բת��ΪTopoDS_Edge����

	//	TopoDS_Edge C1_edge = BRepBuilderAPI_MakeEdge(C1);

	//	TopoDS_Edge C2_edge = BRepBuilderAPI_MakeEdge(C2);

	//	//��TopoDS_Edge���͵�Բת��ΪTopoDS_Wire����

	//	TopoDS_Wire C1_wire = BRepBuilderAPI_MakeWire(C1_edge);

	//	TopoDS_Wire C2_wire = BRepBuilderAPI_MakeWire(C2_edge);

	//	//����һ�����������������

	//	BRepOffsetAPI_ThruSections generator;

	//	//�������Բ

	//	generator.AddWire(C1_wire);

	//	generator.AddWire(C2_wire);

	//	//������յ�shape

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