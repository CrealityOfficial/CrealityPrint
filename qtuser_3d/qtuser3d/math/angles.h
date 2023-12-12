#ifndef _qtuser_3d_ANGLES_1588841647525_H
#define _qtuser_3d_ANGLES_1588841647525_H
#include "qtuser3d/qtuser3dexport.h"
#include <QtGui/QVector3D>
#include <QtGui/QQuaternion>

namespace qtuser_3d
{
	QTUSER_3D_API float angleOfVector3D2(const QVector3D& v1, const QVector3D& v2);
	QTUSER_3D_API float radianOfVector3D2(const QVector3D& v1, const QVector3D& v2);

	QTUSER_3D_API QQuaternion quaternionFromVector3D2(const QVector3D& v1, const QVector3D& v2, bool normalized = true, bool needFan = false);
}
#endif // _qtuser_3d_ANGLES_1588841647525_H
