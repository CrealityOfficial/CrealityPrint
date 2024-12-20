// 测试代码

#include <gtest/gtest.h>
#include "MESH/mesh.hpp"

const double areaEpsilon = 1e-6;

namespace test_modelStructure_MESH_MeshTest
{
	MESH::MeshPtr buildTriangle()
	{
		GEOM::Point p1(0, 0, 0);
		GEOM::Point p2(1, 0, 0);
		GEOM::Point p3(1, 1, 0);
		GEOM::Point p4(0, 1, 0);
		std::vector<GEOM::Point> points = { p1, p2, p3, p4 };
		std::vector<unsigned int> faceIndexes = { 0, 1, 2 };
		MESH::MeshPtr mesh = std::make_shared<MESH::Mesh>();
		mesh->addFaces(points, faceIndexes);
		return mesh;
	}

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

	TEST(MESH, Mesh_stat) {

		MESH::MeshPtr mesh = buildMesh();
		ASSERT_EQ(4, mesh->vertexCount());
		ASSERT_EQ(5, mesh->edgeCount());
		ASSERT_EQ(2, mesh->faceCount());
	}

	TEST(MESH, Vertex_isBorder) {
		MESH::MeshPtr mesh = buildMesh();
		unsigned int vertexBorderCount = 0;
		for (auto& vertex : mesh->getVertices())
		{
			if (vertex->isBorder())
			{
				vertexBorderCount++;
			}
		}
		ASSERT_EQ(4, vertexBorderCount);
	}

	TEST(MESH, Vertex_refFaceCount) {
		MESH::MeshPtr mesh = buildMesh();
		unsigned int totalRefFaceCount = 0;
		for (auto& vertex : mesh->getVertices())
		{
			totalRefFaceCount += vertex->getRefFaceCount();
		}
		ASSERT_EQ(6, totalRefFaceCount);
	}

	TEST(MESH, Edge_isBorder) {
		MESH::MeshPtr mesh = buildMesh();
		unsigned int edgeBorderCount = 0;
		for (auto& edge : mesh->getEdges())
		{
			if (edge->isBorder())
			{
				edgeBorderCount++;
			}
		}
		ASSERT_EQ(4, edgeBorderCount);
	}

	TEST(MESH, Face_normal) {
		MESH::MeshPtr mesh = buildMesh();
		std::vector<MESH::FacePtr> faces = mesh->getFaces();
		MESH::FacePtr face = faces.at(0);
		GEOM::Vector vNormal = face->normal();
		vNormal.normalize();
		GEOM::Vector v(0, 0, 1);
		(v, vNormal);
	}

	TEST(MESH, Face_area) {
		MESH::MeshPtr mesh = buildMesh();
		std::vector<MESH::FacePtr> faces = mesh->getFaces();
		double areaTotal = 0;
		for (auto& face : faces)
		{
			areaTotal += face->area();
		}
		EXPECT_NEAR(1, areaTotal, areaEpsilon);
	}

	TEST(MESH, Face_oppositeVertex) {
		MESH::MeshPtr mesh = buildTriangle();
		auto& faces = mesh->getFaces();
		auto& face = faces.at(0);
		auto& vertices = face->getVertices();
		auto& vertex = vertices.at(0);
		auto& edge = face->oppositeEdge(vertex);
		auto& vertex1 = vertices.at(1);
		auto& vertex2 = vertices.at(2);
		MESH::EdgePtr edgeVerify = std::make_shared<MESH::Edge>(vertex1, vertex2, mesh);
		ASSERT_EQ(edge->getId(), edgeVerify->getId());
	}

	TEST(MESH, Face_oppositeEdge) {
		MESH::MeshPtr mesh = buildTriangle();
		auto& faces = mesh->getFaces();
		auto& face = faces.at(0);
		auto& edges = face->getEdges();
		auto& edge = edges.at(0);
		auto& vertex = face->oppositeVertex(edge);
		auto& v1 = edge->getBegin();
		auto& v2 = edge->getEnd();
		if (vertex->getId() != v1->getId() && vertex->getId() != v2->getId())
		{
			GTEST_SUCCEED();
		}
		else
		{
			GTEST_FAIL();
		}
	}

	TEST(MESH, Mesh_stat_afterDeleteFace) {
		MESH::MeshPtr mesh = buildMesh();
		auto& faces = mesh->getFaces();
		auto& face = faces.at(0);
		face->deleteFace();
		ASSERT_EQ(3, mesh->vertexCount());
		ASSERT_EQ(3, mesh->edgeCount());
		ASSERT_EQ(1, mesh->faceCount());
	}

	TEST(MESH, Mesh_stat_afterAddDegenenrateFace) {
		MESH::MeshPtr mesh = buildMesh();
		GEOM::Point p1(0, 0, 0);
		GEOM::Point p2(0, 0, 0);
		GEOM::Point p3(1, 1, 0);
		mesh->addFace(p1, p2, p3);
		ASSERT_EQ(4, mesh->vertexCount());
		ASSERT_EQ(5, mesh->edgeCount());
		ASSERT_EQ(2, mesh->faceCount());
	}

	TEST(MESH, Mesh_stat_afterAddFace) {
		MESH::MeshPtr mesh = buildMesh();
		GEOM::Point p1(0, 1, 0);
		GEOM::Point p2(1, 1, 0);
		GEOM::Point p3(1, 2, 0);
		mesh->addFace(p1, p2, p3);
		ASSERT_EQ(5, mesh->vertexCount());
		ASSERT_EQ(7, mesh->edgeCount());
		ASSERT_EQ(3, mesh->faceCount());
	}

	TEST(MESH, Mesh_stat_afterAddFaceAndDeleteFace) {
		MESH::MeshPtr mesh = buildMesh();
		GEOM::Point p1(0, 1, 0);
		GEOM::Point p2(1, 1, 0);
		GEOM::Point p3(1, 2, 0);
		mesh->addFace(p1, p2, p3);
		auto& faces = mesh->getFaces();
		auto& face = faces.at(1);
		face->deleteFace();
		ASSERT_EQ(5, mesh->vertexCount());
		ASSERT_EQ(6, mesh->edgeCount());
		ASSERT_EQ(2, mesh->faceCount());
	}

	TEST(MESH, Mesh_stat_afterDeleteFaceAndAddFace) {
		MESH::MeshPtr mesh = buildMesh();
		auto& faces = mesh->getFaces();
		auto& face = faces.at(1);
		face->deleteFace();
		GEOM::Point p1(0, 1, 0);
		GEOM::Point p2(1, 1, 0);
		GEOM::Point p3(1, 2, 0);
		mesh->addFace(p1, p2, p3);
		ASSERT_EQ(5, mesh->vertexCount());
		ASSERT_EQ(6, mesh->edgeCount());
		ASSERT_EQ(2, mesh->faceCount());
	}

	TEST(MESH, Mesh_getverticesAndFaceIndex) {
		MESH::MeshPtr mesh = buildMesh();
		std::vector<MESH::VertexPtr> vertices;
		std::vector<std::array<unsigned int, 3>> faceIndices;
		mesh->getverticesAndFaceIndex(vertices, faceIndices);
		ASSERT_EQ(vertices.size(), mesh->vertexCount());
		ASSERT_EQ(faceIndices.size(), mesh->faceCount());
	}
}