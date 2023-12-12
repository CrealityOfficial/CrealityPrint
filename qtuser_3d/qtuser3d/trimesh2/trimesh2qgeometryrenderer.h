#ifndef CXKERNEL_TRIMESH_2_GEOMETRY_RENDERER_H
#define CXKERNEL_TRIMESH_2_GEOMETRY_RENDERER_H
#include "qtuser3d/qtuser3dexport.h"
#include "qtuser3d/geometry/attribute.h"
#include <Qt3DRender/QGeometry>

namespace qtuser_3d
{
	QTUSER_3D_API Qt3DRender::QGeometry* trimeshes2Geometry(std::vector<trimesh::TriMesh*>& meshes, int vflag = -1, Qt3DCore::QNode* parent = nullptr);
	QTUSER_3D_API Qt3DRender::QGeometry* trimesh2Geometry(trimesh::TriMesh* mesh, int vflag = -1, Qt3DCore::QNode* parent = nullptr);
}

#endif // Q_UTIL_TRIMESH_2_GEOMETRY_RENDERER_H