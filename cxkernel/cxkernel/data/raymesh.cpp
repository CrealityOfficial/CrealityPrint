#include "raymesh.h"

namespace cxkernel
{
	bool rayMeshCheck(trimesh::TriMesh* mesh, const trimesh::fxform& matrix, int primitiveID, const Ray& ray,
		trimesh::vec3& position, trimesh::vec3& normal, bool accurate)
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
		
		if (!ray.collidePlane(v1, normal, position))
			return false;

		if (!accurate)
			return true;

		/* Check if position is on the triangle*/
		trimesh::vec3 e1 = v1 - v3;
		trimesh::vec3 e2 = v2 - v3;
		trimesh::vec3 e3;
		float totalArea = 0.5 * trimesh::len(e1 TRICROSS e2);

		e1 = v1 - position;
		e2 = v2 - position;
		e3 = v3 - position;
		float area1 = 0.5 * trimesh::len(e1 TRICROSS e2);
		if (area1 >= totalArea)
			return false;

		float area2 = 0.5 * trimesh::len(e1 TRICROSS e3);
		if (area2 >= totalArea)
			return false;

		float area3 = 0.5 * trimesh::len(e2 TRICROSS e3);
		if (area3 >= totalArea)
			return false;

		return std::abs(area1 + area2 + area3 - totalArea) < 1e-4;
	}

	bool rayMeshCheckEx(trimesh::TriMesh* mesh, const trimesh::fxform& matrix, const trimesh::fxform& normalMatrix, int primitiveID, const Ray& ray,
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

		trimesh::vec3 v12 = mesh->vertices.at(f[1]) - mesh->vertices.at(f[0]);
		trimesh::vec3 v13 = mesh->vertices.at(f[2]) - mesh->vertices.at(f[0]);

		trimesh::normalize(v12);
		trimesh::normalize(v13);
		trimesh::vec3 d = normalMatrix * (v12 TRICROSS v13);

		normal = trimesh::normalized(d);
		return ray.collidePlane(v1, d, position);
	}
}