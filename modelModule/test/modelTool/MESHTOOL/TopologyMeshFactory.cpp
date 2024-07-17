// 测试代码

#include <gtest/gtest.h>
#include "MESHTOOL/TopologyMeshFactory.hpp"

namespace test_modelTool_MESHTOOL_TopologyMeshFactoryTest
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


	TEST(MESHTOOL, TopologyMeshFactory_build_fromTriMesh) {
		MESHTOOL::TopologyMeshFactory topologyMeshFactory;
		MESHTOOL::TriMeshPtr triMesh = buildTriMesh();
		MESH::TopologyMeshPtr mesh = topologyMeshFactory.build(triMesh);
		ASSERT_EQ(4, mesh->vertexCount());
		ASSERT_EQ(5, mesh->edgeCount());
		ASSERT_EQ(2, mesh->faceCount());
	}

}