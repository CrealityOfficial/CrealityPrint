#ifndef QCXUTIL_Q3DRENDER_1645445878366_H
#define QCXUTIL_Q3DRENDER_1645445878366_H
#include "qtuser3d/qtuser3dexport.h"
#include "trimesh2/Vec.h"
#include "trimesh2/XForm.h"
#include "trimesh2/Box.h"
#include "trimesh2/TriMesh.h"

#include "qtuser3d/geometry/attribute.h"
#include <Qt3DRender/QGeometry>
#include <QtCore/QByteArray>
#include <qtuser3d/geometry/supportattribute.h>

namespace qtuser_3d
{
	QTUSER_3D_API Qt3DRender::QGeometry* trimeshes2Geometry(const std::vector<trimesh::TriMesh*>& meshes, Qt3DCore::QNode* parent = nullptr);
	QTUSER_3D_API Qt3DRender::QGeometry* trimesh2Geometry(trimesh::TriMesh* mesh, Qt3DCore::QNode* parent = nullptr);

	QTUSER_3D_API QByteArray createFlagArray(int faceNum);
	QTUSER_3D_API QByteArray createPositionArray(trimesh::TriMesh* mesh);
	QTUSER_3D_API void trimesh2SupportAttributeInfo(trimesh::TriMesh* mesh, qtuser_3d::SupportAttributeShade& supportChunkAttributeInfo);

	QTUSER_3D_API void trimeshes2AttributeShade(const std::vector<trimesh::TriMesh*>& meshes, qtuser_3d::AttributeShade& position, qtuser_3d::AttributeShade& normal);
	QTUSER_3D_API void trimesh2AttributeShade(trimesh::TriMesh* mesh, qtuser_3d::AttributeShade& position, qtuser_3d::AttributeShade& normal);

}

#endif // QCXUTIL_Q3DRENDER_1645445878366_H