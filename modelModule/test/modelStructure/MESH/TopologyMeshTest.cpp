// 测试代码

#include <gtest/gtest.h>
#include "MESH/TopologyMesh.hpp"

namespace test_modelStructure_MESH_TopologyMeshTest
{
	MESH::MeshPtr buildTopologyMesh()
	{
		GEOM::Point p1(0, 0, 0);
		GEOM::Point p2(1, 0, 0);
		GEOM::Point p3(1, 1, 0);
		GEOM::Point p4(0, 1, 0);
		std::vector<GEOM::Point> points = { p1, p2, p3, p4 };
		std::vector<unsigned int> faceIndexes = { 0, 1, 2, 0, 2, 3 };
		MESH::MeshPtr mesh = std::make_shared<MESH::TopologyMesh>();
		mesh->addFaces(points, faceIndexes);
		return mesh;
	}

	TEST(MESH, TopologyMesh_stat) {
		MESH::MeshPtr mesh = buildTopologyMesh();
		ASSERT_EQ(4, mesh->vertexCount());
		ASSERT_EQ(5, mesh->edgeCount());
		ASSERT_EQ(2, mesh->faceCount());
	}

	TEST(MESH, TopologyMesh_Vertex_NeighborFace) {
		MESH::MeshPtr mesh = buildTopologyMesh();
		auto& vertices = mesh->getVertices();
		unsigned int totalNeighborFaceCount = 0;
		for (auto& vertex : vertices)
		{
			totalNeighborFaceCount += vertex->neighborFace().size();
		}
		ASSERT_EQ(6, totalNeighborFaceCount);
	}

	TEST(MESH, TopologyMesh_Vertex_NeighborVertex) {
		MESH::MeshPtr mesh = buildTopologyMesh();
		auto& vertices = mesh->getVertices();
		unsigned int totalNeighborVertexCount = 0;
		for (auto& vertex : vertices)
		{
			totalNeighborVertexCount += vertex->neighborVertex().size();
		}
		ASSERT_EQ(10, totalNeighborVertexCount);
	}

	TEST(MESH, TopologyMesh_Vertex_NeighborFaceAndNeighborEdge) {
		MESH::MeshPtr mesh = buildTopologyMesh();
		auto& vertices = mesh->getVertices();
		unsigned int borderVertexCount = 0;
		for (auto& vertex : vertices)
		{
			if (vertex->neighborFace().size() != vertex->neighborEdge().size())
			{
				borderVertexCount++;
			}
		}
		ASSERT_EQ(4, borderVertexCount);
	}

	TEST(MESH, TopologyMesh_Edge_NeighborFace) {
		MESH::MeshPtr mesh = buildTopologyMesh();
		auto& edges = mesh->getEdges();
		unsigned int borderEdgeCount = 0;
		for (auto& edge : edges)
		{
			if (edge->neighborFace().size() == 1)
			{
				borderEdgeCount++;
			}
		}
		ASSERT_EQ(4, borderEdgeCount);
	}

	TEST(MESH, TopologyMesh_AddFace) {
		MESH::MeshPtr mesh = buildTopologyMesh();
		GEOM::Point p1(0, 1, 0);
		GEOM::Point p2(1, 1, 0);
		GEOM::Point p3(1, 2, 0);
		mesh->addFace(p1, p2, p3);
		unsigned int count3 = 0;
		unsigned int count4 = 0;
		for (auto& vertex : mesh->getVertices())
		{
			if (vertex->neighborEdge().size() == 3)
			{
				count3++;
			}
			if (vertex->neighborEdge().size() == 4)
			{
				count4++;
			}
		}
		ASSERT_EQ(2, count3);
		ASSERT_EQ(1, count4);
	}

	TEST(MESH, TopologyMesh_AddFaceAndDeleteFace) {
		MESH::MeshPtr mesh = buildTopologyMesh();
		GEOM::Point p1(0, 1, 0);
		GEOM::Point p2(1, 1, 0);
		GEOM::Point p3(1, 2, 0);
		mesh->addFace(p1, p2, p3);
		auto& faces = mesh->getFaces();
		auto& face = faces.at(1);
		face->deleteFace();
		unsigned int count2 = 0;
		unsigned int count3 = 0;
		unsigned int count4 = 0;
		for (auto& vertex : mesh->getVertices())
		{
			if (vertex->neighborEdge().size() == 2)
			{
				count2++;
			}
			if (vertex->neighborEdge().size() == 3)
			{
				count3++;
			}
			if (vertex->neighborEdge().size() == 4)
			{
				count4++;
			}
		}
		ASSERT_EQ(4, count2);
		ASSERT_EQ(0, count3);
		ASSERT_EQ(1, count4);
	}

	TEST(MESH, TopologyMesh_DeleteFaceAndAddFace) {
		MESH::MeshPtr mesh = buildTopologyMesh();
		auto& faces = mesh->getFaces();
		auto& face = faces.at(1);
		face->deleteFace();
		GEOM::Point p1(0, 1, 0);
		GEOM::Point p2(1, 1, 0);
		GEOM::Point p3(1, 2, 0);
		mesh->addFace(p1, p2, p3);
		unsigned int count2 = 0;
		unsigned int count3 = 0;
		unsigned int count4 = 0;
		for (auto& vertex : mesh->getVertices())
		{
			if (vertex->neighborEdge().size() == 2)
			{
				count2++;
			}
			if (vertex->neighborEdge().size() == 3)
			{
				count3++;
			}
			if (vertex->neighborEdge().size() == 4)
			{
				count4++;
			}
		}
		ASSERT_EQ(4, count2);
		ASSERT_EQ(0, count3);
		ASSERT_EQ(1, count4);
	}
}