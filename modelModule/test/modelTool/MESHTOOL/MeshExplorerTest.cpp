// 测试代码

#include <gtest/gtest.h>
#include "MESHTOOL/MeshExplorer.hpp"

namespace test_modelTool_MESHTOOL_MeshExplorerTest
{
	MESH::MeshPtr buildMesh()
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

	TEST(MESHTOOL, MeshExplorer_stat) {
		MESH::MeshPtr mesh = buildMesh();
		MESHTOOL::MeshExplorer explorer;
		std::unordered_set<MESH::VertexPtr> vertices = explorer.borderVertices(mesh);

		ASSERT_EQ(4, vertices.size());
	}
}