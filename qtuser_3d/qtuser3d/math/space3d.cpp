#include "space3d.h"
#include <QtCore/QtMath>
#include <QtCore/QDebug>

namespace qtuser_3d
{
	QVector3D point3DFromVector2D(const QVector2D& point, const QVector2D& center, const QVector2D& size, bool skipz)
	{
		return point3DFromVector2D(point, center, size.x(), size.y(), skipz);
	}

	QVector3D point3DFromVector2D(const QVector2D& point, const QVector2D& center, float width, float height, bool skipz)
	{
		QVector3D pt3D;

		float dx = point.x() - center.x();
		float dy = point.y() - center.y();

        float width2 = qMax(qFabs(dx), qMin(qFabs(width - center.x()), qFabs(center.x())));
        float height2 = qMax(qFabs(dy), qMin(qFabs(height - center.y()), qFabs(center.y())));

        float maxDim = qMax(width2, height2);

		pt3D[0] = dx / maxDim;
		pt3D[1] = dy / maxDim;

		if (skipz)
		{
			pt3D[2] = 0.0f;
			pt3D.normalize();
		}
		else
		{
			/// Sphere equation : (x-x_o)^2 + (y-y_o)^2 + (z-z_o)^2 = R^2
			/// we take Center O(x_o=0, y_o=0, z_o=0) and R = 1

			float x2_plus_y2 = (pt3D.x() * pt3D.x() + pt3D.y() * pt3D.y());
			if (x2_plus_y2 > 1.0f)
			{
				pt3D.setZ(0.0f);

				// fast normalization
				float length = sqrt(x2_plus_y2);
				pt3D[0] /= length;
				pt3D[1] /= length;
			}
			else
			{
				pt3D[2] = sqrt(1.0f - x2_plus_y2);
			}
		}

		return pt3D;
	}

	bool lineCollidePlane(const QVector3D& planeCenter, const QVector3D& planeDir, const Ray& ray, QVector3D& collide)
	{
		float l = QVector3D::dotProduct(ray.dir, planeDir);
		if (l == 0.0f) return false;

		float t = QVector3D::dotProduct(planeDir, (planeCenter - ray.start)) / l;
		collide = ray.start + ray.dir * t;

		return true;
	}

	Qt3DExtras::QTorusMesh* torusMesh()
	{
		Qt3DExtras::QTorusMesh* torusMesh = new Qt3DExtras::QTorusMesh(nullptr);
		torusMesh->setRadius(2.0);
		torusMesh->setMinorRadius(0.02);
		torusMesh->setRings(100);
		return torusMesh;
	}

}
