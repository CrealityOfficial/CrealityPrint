// 测试代码

#include <gtest/gtest.h>
#include "MESHTOOL/MeshSpliter.hpp"

namespace test_modelTool_MESHTOOL_MeshSpliterTest
{
	MESH::TopologyMeshPtr buildSplitedMesh()
	{
		GEOM::Point p1(0, 0, 0);
		GEOM::Point p2(1, 0, 0);
		GEOM::Point p3(1, 1, 0);
		GEOM::Point p4(0, 1, 0);
		GEOM::Point p5(0, 0, 1);
		GEOM::Point p6(1, 0, 1);
		GEOM::Point p7(1, 1, 1);
		GEOM::Point p8(0, 1, 1);
		std::vector<GEOM::Point> points = { p1, p2, p3, p4, p5, p6, p7, p8 };
		std::vector<unsigned int> faceIndexes = { 0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7 };
		MESH::TopologyMeshPtr mesh = std::make_shared<MESH::TopologyMesh>();
		mesh->addFaces(points, faceIndexes);
		return mesh;
	}

	TEST(MESHTOOL, MeshSpliter_split) {
		MESH::TopologyMeshPtr mesh = buildSplitedMesh();
		MESHTOOL::MeshSpliter meshSpliter;
		std::vector<MESH::TopologyMeshPtr> meshes = meshSpliter.split(mesh);
		ASSERT_EQ(2, meshes.size());
	}
}
