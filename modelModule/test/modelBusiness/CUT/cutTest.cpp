#include <gtest/gtest.h>
#include "CUT/cut.h"
#include "MESHTOOL/MeshFactory.hpp"
#include "MESHTOOL/MeshOutputer.hpp"

namespace test_modelBusiness_CUT_cutTest
{
	trimesh::TriMesh* buildTriMesh()
	{
		trimesh::point p1 = { 0, 0, 0 };
		trimesh::point p2 = { 1, 0, 0 };
		trimesh::point p3 = { 1, 1, 0 };
		trimesh::point p4 = { 0, 1, 0 };
		trimesh::point p5 = { 0, 0, 1 };
		trimesh::point p6 = { 1, 0, 1 };
		trimesh::point p7 = { 1, 1, 1 };
		trimesh::point p8 = { 0, 1, 1 };
		trimesh::TriMesh::Face f1 = { 0, 2, 1 };
		trimesh::TriMesh::Face f2 = { 0, 3, 2 };
		trimesh::TriMesh::Face f3 = { 4, 5, 6 };
		trimesh::TriMesh::Face f4 = { 4, 6, 7 };
		trimesh::TriMesh::Face f5 = { 1, 2, 6 };
		trimesh::TriMesh::Face f6 = { 1, 6, 5 };
		trimesh::TriMesh::Face f7 = { 0, 1, 5 };
		trimesh::TriMesh::Face f8 = { 0, 5, 4 };
		trimesh::TriMesh::Face f9 = { 0, 4, 3 };
		trimesh::TriMesh::Face f10 = { 3, 4, 7 };
		trimesh::TriMesh::Face f11 = { 2, 7, 6 };
		trimesh::TriMesh::Face f12 = { 2, 3, 7 };
		std::vector<trimesh::point> points = { p1, p2, p3, p4, p5, p6, p7, p8 };
		std::vector<trimesh::TriMesh::Face> faces = { f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12 };
		trimesh::TriMesh* mesh = new trimesh::TriMesh();
		mesh->vertices = points;
		mesh->faces = faces;
		return mesh;
	}

	TEST(CUT, cut_stat) {
		trimesh::TriMesh* mesh = buildTriMesh();
		std::vector<trimesh::TriMesh*> outMeshes;
		CUT::CutPlane cutPlane;
		cutPlane.normal = trimesh::vec3(0.0f, 0.0f, 1.0f);
		cutPlane.position = trimesh::vec3(0.5f, 0.5f, 0.5f);
		CUT::CutParam cutParam;
		cutParam.fillHole = true;
		planeCut(mesh, cutPlane, outMeshes, cutParam);
		
		ASSERT_EQ(2, outMeshes.size());

		unsigned int i = 0;
		for (trimesh::TriMesh* &trimeshPtr: outMeshes)
		{
			MESHTOOL::MeshFactory factory;
			MESHTOOL::TriMeshPtr trimesh(trimeshPtr);
			MESH::MeshPtr mesh = factory.build(trimesh);
			MESHTOOL::MeshOutputer outputer;
			outputer.writeSTLBinary(mesh, "cutTestMesh" + std::to_string(i++) + ".stl");
		}
	}
}