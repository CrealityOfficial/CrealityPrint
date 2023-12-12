#ifndef CXND_RAYMESH_1642582336569_H
#define CXND_RAYMESH_1642582336569_H
#include "cxkernel/cxkernelinterface.h"
#include "trimesh2/TriMesh.h"
#include "trimesh2/XForm.h"
#include "cxkernel/data/ray.h"

namespace cxkernel
{
	CXKERNEL_API bool rayMeshCheck(trimesh::TriMesh* mesh, const trimesh::fxform& matrix, int primitiveID, const Ray& ray,
		trimesh::vec3& position, trimesh::vec3& normal);
}

#endif // CXND_RAYMESH_1642582336569_H