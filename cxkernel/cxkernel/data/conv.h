#ifndef QCXUTIL_VEC2QVECTOR_1645445878366_H
#define QCXUTIL_VEC2QVECTOR_1645445878366_H
#include "cxkernel/data/header.h"

#include <QtCore/QVector>
#include <QtGui/QMatrix4x4>
#include <QtGui/QVector3D>
#include <QtGui/QQuaternion>

namespace cxkernel
{
	CXKERNEL_API QVector3D vec2qvector(const trimesh::vec3& vertex);
	CXKERNEL_API QVector<QVector3D> vecs2qvectors(const std::vector<trimesh::vec3>& vertices);
	CXKERNEL_API QMatrix4x4 xform2QMatrix(const trimesh::fxform& xf);

	CXKERNEL_API trimesh::fxform qMatrix2Xform(const QMatrix4x4& matrix);
	CXKERNEL_API trimesh::vec3 qVector3D2Vec3(const QVector3D& vec);
	CXKERNEL_API trimesh::quaternion qQuaternion2tQuaternion(const QQuaternion& q);
	CXKERNEL_API QQuaternion tqua2qqua(trimesh::quaternion q);
}

#endif // QCXUTIL_VEC2QVECTOR_1645445878366_H