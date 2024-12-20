#ifndef CREATIVE_KERNEL_MODELGROUP_SPACEUTILS_1592037009137_H
#define CREATIVE_KERNEL_MODELGROUP_SPACEUTILS_1592037009137_H
#include "basickernelexport.h"
#include "data/modelgroup.h"
#include "data/modeln.h"

#include "qtuser3d/math/box3d.h"
namespace creative_kernel
{
	BASIC_KERNEL_API trimesh::dbox3 groupsGlobalBoundingBox(const QList<ModelGroup*>& groups);
	BASIC_KERNEL_API trimesh::dbox3 modelsBox(const QList<ModelN*>& models);

	BASIC_KERNEL_API qtuser_3d::Box3D convert(const trimesh::dbox3& box);

	BASIC_KERNEL_API QList<ModelGroup*> findRelativeGroups(const QList<ModelN*>& models);

	BASIC_KERNEL_API bool fuzzyCompareDvec3(const trimesh::dvec3& v1, const trimesh::dvec3& v2);

	BASIC_KERNEL_API void changeMeshFaceNormal(trimesh::TriMesh* oriMesh, trimesh::TriMesh* outMesh, const trimesh::fxform& normalMatrix);

}
#endif // CREATIVE_KERNEL_MODELGROUP_1592037009137_H