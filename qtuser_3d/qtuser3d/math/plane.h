#ifndef _qtuser_3d_PLANE_1591247271330_H
#define _qtuser_3d_PLANE_1591247271330_H
#include "qtuser3d/qtuser3dexport.h"
#include <QtGui/QVector3D>

namespace qtuser_3d
{
	class QTUSER_3D_API Plane
	{
	public:
		Plane();
		Plane(const QVector3D& center);
		Plane(const QVector3D& center, const QVector3D& ndir);

		~Plane();

		QVector3D center;
		QVector3D dir;
	};
}
#endif // _qtuser_3d_PLANE_1591247271330_H
