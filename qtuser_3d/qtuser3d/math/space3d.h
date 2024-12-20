#ifndef _qtuser_3d_SPACE3D_1588841647525_H
#define _qtuser_3d_SPACE3D_1588841647525_H
#include "qtuser3d/qtuser3dexport.h"
#include "qtuser3d/math/box3d.h"
#include "qtuser3d/math/ray.h"
#include <QtGui/QVector2D>
#include <QtGui/QMatrix4x4>
#include <Qt3DExtras/QTorusMesh>

namespace qtuser_3d
{
	QTUSER_3D_API QVector3D point3DFromVector2D(const QVector2D& point, const QVector2D& center, const QVector2D& size, bool skipz);

	QTUSER_3D_API QVector3D point3DFromVector2D(const QVector2D& point, const QVector2D& center, float width, float height, bool skipz);

	QTUSER_3D_API bool lineCollidePlane(const QVector3D& planeCenter, const QVector3D& planeDir, const Ray& ray, QVector3D& collide);

	QTUSER_3D_API Qt3DExtras::QTorusMesh* torusMesh();
}
#endif // _qtuser_3d_SPACE3D_1588841647525_H
