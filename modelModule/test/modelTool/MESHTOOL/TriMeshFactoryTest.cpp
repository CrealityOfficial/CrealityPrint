// 测试代码

#include <gtest/gtest.h>
#include "MESHTOOL/TriMeshFactory.hpp"

namespace test_modelTool_MESHTOOL_TriMeshFactoryTest
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

	TEST(MESHTOOL, TriMeshFactory_build_FromMesh) {
		MESH::MeshPtr mesh = buildMesh();
		MESHTOOL::TriMeshFactory trimeshFactory;
		MESHTOOL::TriMeshPtr trimesh = trimeshFactory.build(mesh);
		ASSERT_EQ(trimesh->vertices.size(), 4);
		ASSERT_EQ(trimesh->faces.size(), 2);
	}

	TEST(MESHTOOL, TriMeshFactory_build_FromSimpleMesh) {
		MESH::SimpleMeshPtr mesh = buildSimpleMesh();
		MESHTOOL::TriMeshFactory trimeshFactory;
		MESHTOOL::TriMeshPtr trimesh = trimeshFactory.build(mesh);
		ASSERT_EQ(trimesh->vertices.size(), 6);
		ASSERT_EQ(trimesh->faces.size(), 2);
	}
}