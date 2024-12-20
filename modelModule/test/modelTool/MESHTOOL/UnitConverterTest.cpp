// 测试代码

#include <gtest/gtest.h>
#include "MESHTOOL/UnitConverter.hpp"

namespace test_modelTool_MESHTOOL_UnitConverterTest
{
	MESHTOOL::TriMeshPtr buildTriMesh()
	{
		trimesh::point p1 = { 0, 0, 0 };
		trimesh::point p2 = { 1, 0, 0 };
		trimesh::point p3 = { 1, 1, 0 };
		trimesh::point p4 = { 0, 1, 0 };
		trimesh::TriMesh::Face f1 = { 0, 1, 2 };
		trimesh::TriMesh::Face f2 = { 0, 2, 3 };
		std::vector<trimesh::point> points = { p1, p2, p3, p4 };
		std::vector<trimesh::TriMesh::Face> faces = { f1, f2 };
		MESHTOOL::TriMeshPtr trimesh = std::make_shared<trimesh::TriMesh>();
		trimesh->vertices = points;
		trimesh->faces = faces;
		return trimesh;
	}

	TEST(MESHTOOL, UnitConverter_toOutside) {
		MESHTOOL::UnitConverter converter;
		float factor = converter.getFactor();
		GEOM::Point p(factor, 0, 0);
		auto &coord = converter.toOutside(p);
		EXPECT_NEAR(1, coord.at(0), 1e-4);
	}

	TEST(MESHTOOL, UnitConverter_toInside_geomPoint) {
		MESHTOOL::UnitConverter converter;
		float factor = converter.getFactor();
		trimesh::point p = { 1, 0, 0 };
		GEOM::Point point = converter.toInside(p);
		ASSERT_EQ(factor, point.x);
	}

}