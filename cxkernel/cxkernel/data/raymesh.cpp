#include "raymesh.h"

namespace cxkernel
{
	bool rayMeshCheck(trimesh::TriMesh* mesh, const trimesh::fxform& matrix, int primitiveID, const Ray& ray,
		trimesh::vec3& position, trimesh::vec3& normal)
	{
		if (!mesh || primitiveID >= mesh->faces.size())
			return false;

		trimesh::TriMesh::Face f = mesh->faces.at(primitiveID);
		trimesh::vec3 v1 = mesh->vertices.at(f[0]);
		trimesh::vec3 v2 = mesh->vertices.at(f[1]);
		trimesh::vec3 v3 = mesh->vertices.at(f[2]);

		v1 = matrix * v1;
		v2 = matrix * v2;
		v3 = matrix * v3;

		trimesh::vec3 v12 = v2 - v1;
		trimesh::vec3 v13 = v3 - v1;

		trimesh::normalize(v12);
		trimesh::normalize(v13);
		trimesh::vec3 d = v12 TRICROSS v13;

		normal = trimesh::normalized(d);
		return ray.collidePlane(v1, d, position);
	}
}