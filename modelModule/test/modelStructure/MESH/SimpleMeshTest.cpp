// 测试代码

#include <gtest/gtest.h>
#include "MESH/SimpleMesh.hpp"

namespace test_modelStructure_MESH_SimpleMeshTest
{
	MESH::SimpleMeshPtr buildSimpleMesh()
	{
		MESH::SimpleMeshPtr simpleMesh = std::make_shared<MESH::SimpleMesh>();
		simpleMesh->addPoint(0, 0, 0);
		simpleMesh->addPoint(1, 0, 0);
		simpleMesh->addPoint(1, 1, 0);
		simpleMesh->addPoint(0, 0, 0);
		simpleMesh->addPoint(1, 1, 0);
		simpleMesh->addPoint(0, 1, 0);
		simpleMesh->addFace(0, 1, 2);
		simpleMesh->addFace(3, 4, 5);
		return simpleMesh;
	}

	TEST(MESH, SimpleMesh_duplicate) {
		MESH::SimpleMeshPtr simpleMesh = buildSimpleMesh();
		simpleMesh->duplicate();
		ASSERT_EQ(4, simpleMesh->pointCount());
		ASSERT_EQ(2, simpleMesh->faceCount());
	}

	TEST(MESH, SimpleMesh_getNormals) {
		MESH::SimpleMeshPtr simpleMesh = buildSimpleMesh();
		std::vector<GEOM::Vector>& normals = simpleMesh->getNormals();
		GEOM::Vector normalVerify(0, 0, 1);
		ASSERT_EQ(normalVerify, normals.at(0));
	}
}
