#ifndef MSBASE_MMESH_CREATECYLINDER_1619866821989_H
#define MSBASE_MMESH_CREATECYLINDER_1619866821989_H
#include "msbase/interface.h"
#include "trimesh2/TriMesh.h"

namespace msbase
{
	MSBASE_API trimesh::TriMesh* createCylinder(float _radius, float _height);
	MSBASE_API trimesh::TriMesh* createSoupObliqueCylinder(int count, float _radius, float _height);
	MSBASE_API trimesh::TriMesh* createSoupObliqueCylinder(int count, const trimesh::vec2& v1, const trimesh::vec2& v2, float _radius);
	MSBASE_API trimesh::TriMesh* createSoupCylinder(int count, float _radius, float _height, bool offsetOnZero = false);
	MSBASE_API trimesh::TriMesh* createSoupCylinder(int count, float _radius, float _height,
		const trimesh::vec3& centerPoint, const trimesh::vec3& normal);
	MSBASE_API trimesh::TriMesh* createCylinderBasepos(int count, float radius, float height, const trimesh::vec3& sp, const trimesh::vec3& normal);
	MSBASE_API trimesh::TriMesh* createCylinderWallBasepos(int count, float radius, float height, const trimesh::vec3& sp);
	MSBASE_API trimesh::TriMesh* createHollowCylinder(trimesh::TriMesh* wallOutter, trimesh::TriMesh* wallInner, int sectionCount, const trimesh::vec3& normal);
}

#endif // MSBASE_MMESH_CREATECYLINDER_1619866821989_H