// 测试代码

#include <gtest/gtest.h>
#include "MESHTOOL/MeshMerger.hpp"

namespace test_modelTool_MESHTOOL_MeshMergerTest
{
	MESH::MeshPtr buildMesh1()
	{
		GEOM::Point p1(0, 0, 0);
		GEOM::Point p2(1, 0, 0);
		GEOM::Point p3(1, 1, 0);
		std::vector<GEOM::Point> points = { p1, p2, p3 };
		std::vector<unsigned int> faceIndexes = { 0, 1, 2 };
		MESH::MeshPtr mesh = std::make_shared<MESH::Mesh>();
		mesh->addFaces(points, faceIndexes);
		return mesh;
	}

	MESH::MeshPtr buildMesh2()
	{
		GEOM::Point p1(0, 0, 0);
		GEOM::Point p2(1, 1, 0);
		GEOM::Point p3(0, 1, 0);
		std::vector<GEOM::Point> points = { p1, p2, p3 };
		std::vector<unsigned int> faceIndexes = { 0, 1, 2 };
		MESH::MeshPtr mesh = std::make_shared<MESH::Mesh>();
		mesh->addFaces(points, faceIndexes);
		return mesh;
	}

	MESH::SimpleMeshPtr buildSimpleMesh1()
	{
		MESH::SimpleMeshPtr simpleMesh = std::make_shared<MESH::SimpleMesh>();
		simpleMesh->addPoint(0, 0, 0);
		simpleMesh->addPoint(1, 0, 0);
		simpleMesh->addPoint(1, 1, 0);
		simpleMesh->addFace(0, 1, 2);
		return simpleMesh;
	}

	MESH::SimpleMeshPtr buildSimpleMesh2()
	{
		MESH::SimpleMeshPtr simpleMesh = std::make_shared<MESH::SimpleMesh>();
		simpleMesh->addPoint(0, 0, 0);
		simpleMesh->addPoint(1, 1, 0);
		simpleMesh->addPoint(0, 1, 0);
		simpleMesh->addFace(0, 1, 2);
		return simpleMesh;
	}

	TEST(MESHTOOL, MeshMerger_merge_fromMesh) {
		MESH::MeshPtr mesh1 = buildMesh1();
		MESH::MeshPtr mesh2 = buildMesh2();
		std::vector<MESH::MeshPtr> meshes = { mesh1, mesh2 };
		MESHTOOL::MeshMerger merger;
		MESH::MeshPtr mesh = merger.merge(meshes);
		ASSERT_EQ(4, mesh->vertexCount());
		ASSERT_EQ(5, mesh->edgeCount());
		ASSERT_EQ(2, mesh->faceCount());
	}

	TEST(MESHTOOL, MeshMerger_merge_fromSimpleMesh) {
		MESH::SimpleMeshPtr mesh1 = buildSimpleMesh1();
		MESH::SimpleMeshPtr mesh2 = buildSimpleMesh2();
		std::vector<MESH::SimpleMeshPtr> meshes = { mesh1, mesh2 };
		MESHTOOL::MeshMerger merger;
		MESH::SimpleMeshPtr mesh = merger.merge(meshes);
		ASSERT_EQ(6, mesh->pointCount());
		ASSERT_EQ(2, mesh->faceCount());
	}
}
