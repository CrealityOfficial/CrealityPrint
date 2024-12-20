// 测试代码

#include <gtest/gtest.h>
#include "MESHTOOL/MeshOutputer.hpp"

namespace test_modelTool_MESHTOOL_OutputTest
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

	MESH::MeshPtr buildMeshCube()
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
		std::vector<unsigned int> faceIndexes = { 
			0, 2, 1,
			0, 3, 2,
			4, 5, 6,
			4, 6, 7,
			1, 2, 6,
			1, 6, 5,
			0, 1, 5,
			0, 5, 4,
			0, 4, 3,
			3, 4, 7,
			2, 7, 6,
			2, 3, 7 };
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

	TEST(MESHTOOL, MeshOutputer_writeSTL_fromMesh) {
		MESH::MeshPtr mesh = buildMeshCube();
		MESHTOOL::MeshOutputer meshOutputer;
		meshOutputer.writeSTLPlain(mesh, "MeshOutputerTestplain.stl");
		meshOutputer.writeSTLBinary(mesh, "MeshOutputerTestbinary.stl");
		GTEST_SUCCEED();
	}

	TEST(MESHTOOL, MeshOutputer_writeSTL_fromSimpleMesh) {
		MESH::SimpleMeshPtr mesh = buildSimpleMesh();
		MESHTOOL::MeshOutputer meshOutputer;
		meshOutputer.writeSTLBinary(mesh, "MeshOutputerTestbinaryFromSimpleMesh.stl");
		GTEST_SUCCEED();
	}

	TEST(MESHTOOL, MeshOutputer_writeOFF_fromMesh) {
		MESH::MeshPtr mesh = buildMeshCube();
		MESHTOOL::MeshOutputer meshOutputer;
		meshOutputer.writeOFF(mesh, "MeshOutputerTestCube.off");
		GTEST_SUCCEED();
	}

	TEST(MESHTOOL, MeshOutputer_writeOFF_fromSimpleMesh) {
		MESH::SimpleMeshPtr mesh = buildSimpleMesh();
		MESHTOOL::MeshOutputer meshOutputer;
		meshOutputer.writeOFF(mesh, "MeshOutputerTestSImpleMesh.off");
		GTEST_SUCCEED();
	}
}