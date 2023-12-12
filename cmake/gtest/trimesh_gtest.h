#ifndef CXSLICE_TESTHELPER_1622639863964_H
#define CXSLICE_TESTHELPER_1622639863964_H
#include "gtest/common_gtest.h"

#include "trimesh2/TriMesh.h"

void test_trimesh_valid(trimesh::TriMesh* mesh)
{
	ASSERT_TRUE(mesh);
	if (mesh)
	{
		GTEST_ASSERT_GT(mesh->vertices.size(), 0);
		GTEST_ASSERT_GT(mesh->faces.size(), 0);
	}
}

void test_trimeshs_valid(trimesh::TriMesh* mesh1,trimesh::TriMesh* mesh2)
{
	ASSERT_TRUE(mesh1);
	ASSERT_TRUE(mesh2);
	if (mesh1 && mesh2)
	{
		GTEST_ASSERT_GT(mesh1->vertices.size(), 0);
		GTEST_ASSERT_GT(mesh1->faces.size(), 0);
		GTEST_ASSERT_GT(mesh2->vertices.size(), 0);
		GTEST_ASSERT_GT(mesh2->faces.size(), 0);

		GTEST_ASSERT_EQ(mesh1->vertices.size(), mesh2->vertices.size());
		GTEST_ASSERT_EQ(mesh1->faces.size(), mesh2->faces.size());
	}
}



#endif // CXSLICE_TESTHELPER_1622639863964_H