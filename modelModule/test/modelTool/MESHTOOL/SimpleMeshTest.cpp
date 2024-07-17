// 测试代码

#include <gtest/gtest.h>
#include "MESHTOOL/SimpleMeshFactory.hpp"

namespace test_modelTool_MESHTOOL_SimpleMeshFactoryTest
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

	MESH::TopologyMeshPtr buildTopologyMesh()
	{
		GEOM::Point p1(0, 0, 0);
		GEOM::Point p2(1, 0, 0);
		GEOM::Point p3(1, 1, 0);
		GEOM::Point p4(0, 1, 0);
		std::vector<GEOM::Point> points = { p1, p2, p3, p4 };
		std::vector<unsigned int> faceIndexes = { 0, 1, 2, 0, 2, 3 };
		MESH::TopologyMeshPtr mesh = std::make_shared<MESH::TopologyMesh>();
		mesh->addFaces(points, faceIndexes);
		return mesh;
	}

	TEST(MESHTOOL, SimpleMeshFactory_build_fromTriMesh) {
		MESHTOOL::SimpleMeshFactory simpleMeshFactory;
		std::vector<trimesh::TriMesh::Face> points;
		MESHTOOL::TriMeshPtr triMesh = buildTriMesh();
		trimesh::TriMesh::Face f1 = { 0, 1, 2 };
		trimesh::TriMesh::Face f2 = { 0, 2, 3 };
		std::vector<trimesh::TriMesh::Face> faces = { f1, f2 };
		MESH::SimpleMeshPtr mesh = simpleMeshFactory.build(faces, triMesh);
		ASSERT_EQ(6, mesh->pointCount());
		ASSERT_EQ(2, mesh->faceCount());
	}

	TEST(MESHTOOL, SimpleMeshFactory_build_fromTopologyMesh) {
		MESH::TopologyMeshPtr topologyMesh = buildTopologyMesh();
		MESHTOOL::SimpleMeshFactory simpleMeshFactory;
		MESH::SimpleMeshPtr mesh = simpleMeshFactory.build(topologyMesh);
		ASSERT_EQ(6, mesh->pointCount());
		ASSERT_EQ(2, mesh->faceCount());
	}

}