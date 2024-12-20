#ifndef _qtuser_3d_UTIL_H
#define _qtuser_3d_UTIL_H
#include "qtuser3d/qtuser3dexport.h"
#include <QtGui/QVector3D>
#include <QtGui/QQuaternion>

namespace qtuser_3d
{
	QTUSER_3D_API inline void checkQVector3D(QVector3D& v, const QVector3D& minscope, const QVector3D& maxscope)
	{
		for (int i = 0; i < 3; i++)
		{
			if (v[i] > maxscope[i])
			{
				v[i] = maxscope[i];
			}
			if (v[i] < minscope[i])
			{
				v[i] = minscope[i];
			}
		}

	}

}
#endif // _qtuser_3d_UTIL_H
