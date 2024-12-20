#ifndef QTUSER_3D_VEC2QVECTOR_1645445878366_H
#define QTUSER_3D_VEC2QVECTOR_1645445878366_H
#include "qtuser3d/qtuser3dexport.h"
#include "trimesh2/Vec.h"
#include "trimesh2/XForm.h"
#include "trimesh2/Box.h"
#include "trimesh2/quaternion.h"

#include <QtCore/QVector>
#include <QtGui/QMatrix4x4>
#include <QtGui/QVector3D>
#include <QtGui/QQuaternion>

#include "qtuser3d/math/box3d.h"
#include "qtuser3d/math/ray.h"

namespace qtuser_3d
{
	QTUSER_3D_API QVector3D vec2qvector(const trimesh::vec3& vertex);
	QTUSER_3D_API QVector3D vec2qvector(const trimesh::dvec3& vertex);
	QTUSER_3D_API QVector<QVector3D> vecs2qvectors(const std::vector<trimesh::vec3>& vertices);
	QTUSER_3D_API QMatrix4x4 xform2QMatrix(const trimesh::fxform& xf);
	QTUSER_3D_API qtuser_3d::Box3D triBox2Box3D(const trimesh::box3& box);
	QTUSER_3D_API qtuser_3d::Box3D triBox2Box3D(const trimesh::dbox3& box);

	QTUSER_3D_API trimesh::fxform qMatrix2Xform(const QMatrix4x4& matrix);
	QTUSER_3D_API trimesh::vec3 qVector3D2Vec3(const QVector3D& vec);
	QTUSER_3D_API trimesh::box3 qBox32box3(const qtuser_3d::Box3D& box);
	QTUSER_3D_API trimesh::quaternion qQuaternion2tQuaternion(const QQuaternion& q);
	QTUSER_3D_API QQuaternion tqua2qqua(trimesh::quaternion q);
}

#endif // QTUSER_3D_VEC2QVECTOR_1645445878366_H