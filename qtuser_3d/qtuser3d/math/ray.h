#ifndef _qtuser_3d_RAY_1588946909315_H
#define _qtuser_3d_RAY_1588946909315_H
#include "qtuser3d/qtuser3dexport.h"
#include <QtGui/QVector3D>

namespace qtuser_3d
{
	class QTUSER_3D_API Ray
	{
	public:
		Ray();
		Ray(const QVector3D& vstart);
		Ray(const QVector3D& vstart, const QVector3D& ndir);

		~Ray();

		QVector3D start;
		QVector3D dir;
	};
}
#endif // _qtuser_3d_RAY_1588946909315_H
