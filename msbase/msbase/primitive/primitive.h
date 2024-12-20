#ifndef MSBASE_PRIMITIVE_1695439721999_H
#define MSBASE_PRIMITIVE_1695439721999_H
#include "msbase/interface.h"
#include "trimesh2/TriMesh.h"

namespace msbase
{
	MSBASE_API trimesh::TriMesh* createSphere(float radius, int num_iter);

	MSBASE_API trimesh::TriMesh* createTorusMesh(float middler, float tuber, int sides, int slices);

	MSBASE_API trimesh::TriMesh* createCylinder(float radius, float height, int num_iter);
	MSBASE_API trimesh::TriMesh* createCCylinder(float radiusu, float radiusb, float height, int num_iter);

	MSBASE_API trimesh::TriMesh* createCylinderMesh(const trimesh::vec3& top, const trimesh::vec3& bottom, float radius, int num = 20, float theta = 0.0f);
	MSBASE_API trimesh::TriMesh* createCylinderMeshFromCenter(const trimesh::vec3& position, const trimesh::vec3& normal,
		float depth, float radius, int num = 20, float theta = 0.0f);

	MSBASE_API void fillCylinderSoup(trimesh::vec3* data, float radius1, const trimesh::vec3& center1,
		float radius2, const trimesh::vec3& center2, int num, float theta);

	MSBASE_API trimesh::TriMesh* createCuboid(float x, float y, float z);

	MSBASE_API trimesh::TriMesh* createCone(size_t num_iter, float radius, float height);
}

#endif // MSBASE_PRIMITIVE_1695439721999_H