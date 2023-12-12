#ifndef _qtuser_3d_RECTUTIL_1588842280407_H
#define _qtuser_3d_RECTUTIL_1588842280407_H
#include "qtuser3d/qtuser3dexport.h"
#include <QtCore/QSize>
#include <QtCore/QPointF>
#include <QtCore/QPoint>

namespace qtuser_3d
{
	QTUSER_3D_API int adjustY(int y, const QSize& size);

	QTUSER_3D_API float viewportRatio(int x, int w);
	QTUSER_3D_API QPointF viewportRatio(const QPoint p, const QSize& size);
}
#endif // _qtuser_3d_RECTUTIL_1588842280407_H
