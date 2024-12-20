// 测试代码

#include <gtest/gtest.h>
#include "MESHTOOL/MeshIntersectionChecker.hpp"

namespace test_modelTool_MESHTOOL_MeshIntersectionChecker
{
	MESH::MeshPtr buildMesh1()
	{
		GEOM::Point p1(0, 0, 0);
		GEOM::Point p2(1, 0, 0);
		GEOM::Point p3(1, 1, 0);
		GEOM::Point p4(0, 1, 0);
		std::vector<GEOM::Point> points = { p1, p2, p3, p4 };
		std::vector<unsigned int> faceIndexes = { 0, 1, 2, 0, 2, 3 };
		MESH::MeshPtr mesh = std::make_shared<MESH::Mesh>();
		mesh->addFaces(points, faceIndexes);
		return mesh;
	}

	MESH::MeshPtr buildMesh2()
	{
		GEOM::Point p1(0, 0, 0);
		GEOM::Point p2(-1, 0, 0);
		GEOM::Point p3(-1, -1, 0);
		GEOM::Point p4(0, -1, 0);
		std::vector<GEOM::Point> points = { p1, p2, p3, p4 };
		std::vector<unsigned int> faceIndexes = { 0, 1, 2, 0, 2, 3 };
		MESH::MeshPtr mesh = std::make_shared<MESH::Mesh>();
		mesh->addFaces(points, faceIndexes);
		return mesh;
	}

	TEST(MESHTOOL, MeshIntersectionChecker_borderContact) {
		MESH::MeshPtr mesh1 = buildMesh1();
		MESH::MeshPtr mesh2 = buildMesh2();
		MESHTOOL::MeshIntersectionChecker checker;
		ASSERT_EQ(true, checker.borderContact(mesh1, mesh2));
	}
}
