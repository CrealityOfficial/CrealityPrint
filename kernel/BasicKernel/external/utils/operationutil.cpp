#include "operationutil.h"
#include "interface/camerainterface.h"
#include "data/modeln.h"
#include "qtuser3d/math/space3d.h"

float VALUE_MAX = 600;

using namespace creative_kernel;

void getTSProperPlane(QVector3D& planeCenter, QVector3D& planeDir, qtuser_3d::Ray& ray, qtuser_3d::Box3D box, TMode m)
{
	planeCenter = QVector3D(0.0f, 0.0f, 0.0f);
	planeCenter = box.center();

	QList<QVector3D> dirs;
	if (m == TMode::x)  // x
	{
		dirs << QVector3D(0.0f, 1.0f, 0.0f);
		dirs << QVector3D(0.0f, 0.0f, 1.0f);
	}
	else if (m == TMode::y)
	{
		dirs << QVector3D(1.0f, 0.0f, 0.0f);
		dirs << QVector3D(0.0f, 0.0f, 1.0f);
	}
	else if (m == TMode::z)
	{
		dirs << QVector3D(1.0f, 0.0f, 0.0f);
		dirs << QVector3D(0.0f, 1.0f, 0.0f);
	}
	else if (m == TMode::xy)
	{
		planeDir = QVector3D(0.0, 0.0, 1.0);
	}
	else if (m == TMode::yz)
	{
		planeDir = QVector3D(1.0, 0.0, 0.0);
	}
	else if (m == TMode::zx)
	{
		planeDir = QVector3D(0.0, 1.0, 0.0);
	}
	else if (m == TMode::xyz)
	{
		planeDir = ray.dir;
	}
	else if (m == TMode::sp)
	{
		dirs << QVector3D(0.0f, 0.0f, 1.0f);
		dirs << QVector3D(0.0f, 0.0f, 1.0f);
	}

	float d = -1.0f;
	for (QVector3D& n : dirs)
	{
		float dd = QVector3D::dotProduct(n, ray.dir);
		if (qAbs(dd) > d)
		{
			planeDir = n;
//			d = dd;
		}
	}
}

void getRProperPlane(QVector3D& planeCenter, QVector3D& planeDir, qtuser_3d::Ray& ray, qtuser_3d::Box3D box, TMode m)
{
	planeCenter = box.center();
	if (m == TMode::x)  // x
	{
		planeDir = QVector3D(1.0f, 0.0f, 0.0f);
	}
	else if (m == TMode::y)
	{
		planeDir = QVector3D(0.0f, 1.0f, 0.0f);
	}
	else if (m == TMode::z)
	{
		planeDir = QVector3D(0.0f, 0.0f, 1.0f);
	}
	else if (m == TMode::xy)
	{
		planeDir = QVector3D(0.0, 0.0, 1.0);
	}
	else if (m == TMode::yz)
	{
		planeDir = QVector3D(1.0, 0.0, 0.0);
	}
	else if (m == TMode::zx)
	{
		planeDir = QVector3D(0.0, 1.0, 0.0);
	}
	else if (m == TMode::xyz)
	{
		planeDir = ray.dir;
	}
}

QVector3D operationProcessCoord(const QPoint& point, ModelN* model, int op_type, TMode m)
{
	qtuser_3d::Ray ray = visRay(point);
	QVector3D planeCenter;
	QVector3D planeDir;

	qtuser_3d::Box3D box;
	if (model != nullptr)
	{
		box = model->globalSpaceBox();
	}

	if (op_type == 0)
	{
		getTSProperPlane(planeCenter, planeDir, ray, box, m);
	}
	else if (op_type == 1)
	{
		getRProperPlane(planeCenter, planeDir, ray, box, m);
	}

	QVector3D c;
	qtuser_3d::lineCollidePlane(planeCenter, planeDir, ray, c);
	if (c.x() > VALUE_MAX) { c.setX(VALUE_MAX); }
	if (c.y() > VALUE_MAX) { c.setY(VALUE_MAX); }
	if (c.z() > VALUE_MAX) { c.setZ(VALUE_MAX); }
	if (c.x() < -VALUE_MAX) { c.setX(-VALUE_MAX); }
	if (c.y() < -VALUE_MAX) { c.setY(-VALUE_MAX); }
	if (c.z() < -VALUE_MAX) { c.setZ(-VALUE_MAX); }
	return c;
}


