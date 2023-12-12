#include "qtuser3d/math/angles.h"
#include "qtuser3d/math/const.h"

namespace qtuser_3d
{
	float angleOfVector3D2(const QVector3D& v1, const QVector3D& v2)
	{
		float radian = radianOfVector3D2(v1, v2);
		return radian * 180.0f / (float)M_PI;
	}

	float radianOfVector3D2(const QVector3D& v1, const QVector3D& v2)
	{
		double denominator = sqrt((double)v1.lengthSquared() * (double)v2.lengthSquared());
		double cosinus = 0.0;

		if (denominator < INT_MIN)
			cosinus = 1.0; // cos (1)  = 0 degrees

		cosinus = (double)(QVector3D::dotProduct(v1, v2)) / denominator;
		cosinus = cosinus > 1.0 ? 1.0 : (cosinus < -1.0 ? -1.0 : cosinus);
		double angle = acos(cosinus);
#ifdef WIN32
		if (!_finite(angle) || angle > M_PI)
			angle = 0.0;
#elif __APPLE__
		if (!finite(angle) || angle > M_PI)
			angle = 0.0;
#else
		if (!__finite(angle) || angle > M_PI)
			angle = 0.0;
#endif
		return (float)angle;
	}

	QQuaternion quaternionFromVector3D2(const QVector3D& v1, const QVector3D& v2, bool normalized, bool needFan)
	{
		QVector3D nv1 = normalized ? v1 : v1.normalized();
		QVector3D nv2 = normalized ? v2 : v2.normalized();

		float angle = qtuser_3d::angleOfVector3D2(nv1, nv2);
		if (needFan)
		{
			angle = -angle;
		}
		QVector3D axis = QVector3D(1.0f, 0.0f, 0.0f);
		if (angle < 180.0f)
		{
			axis = QVector3D::crossProduct(nv1, nv2);
			axis.normalize();
		}

		QQuaternion q = QQuaternion::fromAxisAndAngle(axis, angle);

#ifdef _DEBUG
		if (q.isNull())
		{
			q = QQuaternion();
			qDebug() << "QQuaterian error.";
		}
#endif // 

		return q;
	}
}
