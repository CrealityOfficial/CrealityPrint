#include "translateop.h"
#include <QQmlProperty>
#include "qtuser3d/math/space3d.h"
#include "qtuser3d/camera/cameracontroller.h"
#include <QMetaObject>
#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
#include "interface/camerainterface.h"
#include "interface/modelinterface.h"
#include "interface/eventinterface.h"


using namespace creative_kernel;
MoveOperateMode::MoveOperateMode(QObject* parent)
	:SceneOperateMode(parent)
	, m_mode(TMode::null)
{
}

MoveOperateMode::~MoveOperateMode()
{
	//unTraceSpace(this);
}


void MoveOperateMode::onAttach()
{
	addLeftMouseEventHandler(this);
	requestVisPickUpdate(true);
}

void MoveOperateMode::onDettach()
{
	removeLeftMouseEventHandler(this);
}

void MoveOperateMode::onLeftMouseButtonPress(QMouseEvent* event)
{
	m_mode = TMode::null;
	m_tempLocal = QVector3D();

	auto models = creative_kernel::selectionms();
	if (models.isEmpty())
		return;

	qtuser_3d::Pickable* pickable = checkPickable(event->pos(), nullptr);
	ModelN* model = dynamic_cast<ModelN*>(pickable);
	if (models.contains(model))
	{
		setMode(TMode::sp); 
		resetSpacePoint(event->pos());
	}
}

void MoveOperateMode::onLeftMouseButtonRelease(QMouseEvent* event)
{
	if (m_mode == TMode::null || m_tempLocal.isNull())
		return;

	auto models = creative_kernel::selectionms();
	if (models.isEmpty())
		return;

	if (!m_tempLocal.isNull())
	{
		QList<QVector3D> starts;
		QList<QVector3D> ends;

		for (auto m : models)
		{
			QVector3D originLocalPosition = m->localPosition() - m_tempLocal;

			starts << originLocalPosition;

			ends << m->localPosition();
		}

		moveModels(models, starts, ends, true);
	}

	m_mode = TMode::null;
}

void MoveOperateMode::onLeftMouseButtonMove(QMouseEvent* event)
{
	if (m_mode == TMode::null)
		return;

	auto models = creative_kernel::selectionms();
	if (models.isEmpty())
		return;

	QVector3D p = process(event->pos());
	QVector3D delta = clip(p - m_spacePoint);
	QVector3D moveDelta = delta - m_tempLocal;
	m_tempLocal = delta;

	QList<QVector3D> starts;
	QList<QVector3D> ends;
	for (auto m : models)
	{
		QVector3D newLocalPosition = m->localPosition() + moveDelta;
		//moveModel(m, m->localPosition(), newLocalPosition, false);	
		starts.push_back(m->localPosition());
		ends.push_back(newLocalPosition);
	}
	//yy
	moveModels(models, starts, ends,false);
	requestVisUpdate(true);
	emit moving();
}

void MoveOperateMode::onLeftMouseButtonClick(QMouseEvent* event)
{
}

bool MoveOperateMode::shouldMultipleSelect()
{
	return true;
}

QVector3D MoveOperateMode::process(const QPoint& point)
{
	qtuser_3d::Ray ray = visRay(point);

	QVector3D planeCenter;
	QVector3D planeDir;
	getProperPlane(planeCenter, planeDir, ray);

	QVector3D c;
	qtuser_3d::lineCollidePlane(planeCenter, planeDir, ray, c);
    if(c.x() > PosMax) { c.setX(PosMax); }
    if(c.y() > PosMax) { c.setY(PosMax);}
    if(c.z() > PosMax) { c.setZ(PosMax);}
	return c;
}

void MoveOperateMode::getProperPlane(QVector3D& planeCenter, QVector3D& planeDir, qtuser_3d::Ray& ray)
{
	planeCenter = QVector3D(0.0f, 0.0f, 0.0f);
	auto models = selectionms();
	qtuser_3d::Box3D box = models[models.size()-1]->globalSpaceBox();
	planeCenter = box.center();

	QList<QVector3D> dirs;
	if (m_mode == TMode::x)  // x
	{
		dirs << QVector3D(0.0f, 1.0f, 0.0f);
		dirs << QVector3D(0.0f, 0.0f, 1.0f);
	}
	else if (m_mode == TMode::y)
	{
		dirs << QVector3D(1.0f, 0.0f, 0.0f);
		dirs << QVector3D(0.0f, 0.0f, 1.0f);
	}
	else if (m_mode == TMode::z)
	{
		dirs << QVector3D(1.0f, 0.0f, 0.0f);
		dirs << QVector3D(0.0f, 1.0f, 0.0f);
	}
	else if (m_mode == TMode::xy)
	{
		planeDir = QVector3D(0.0, 0.0, 1.0);
	}
	else if (m_mode == TMode::yz)
	{
		planeDir = QVector3D(1.0, 0.0, 0.0);
	}
	else if (m_mode == TMode::zx)
	{
		planeDir = QVector3D(0.0, 1.0, 0.0);
	}
	else if (m_mode == TMode::xyz)
	{
		planeDir = ray.dir;
	}
	else if (m_mode == TMode::sp)
	{
		dirs << QVector3D(0.0f, 0.0f, 1.0f);
		dirs << QVector3D(0.0f, 0.0f, 1.0f);
	}

	float d = 0.0f;
	for (QVector3D& n : dirs)
	{
		float dd = QVector3D::dotProduct(n, ray.dir);
		if (qAbs(dd) > d)
		{
			d = qAbs(dd);
			planeDir = n;
		}
	}
}

QVector3D MoveOperateMode::clip(const QVector3D& delta)
{
	QVector3D clipDelta = delta;
	if (m_mode == TMode::x)  // x
	{
		clipDelta.setY(0.0f);
		clipDelta.setZ(0.0f);
	}
	else if (m_mode == TMode::y)
	{
		clipDelta.setX(0.0f);
		clipDelta.setZ(0.0f);
	}
	else if (m_mode == TMode::z)
	{
		clipDelta.setY(0.0f);
		clipDelta.setX(0.0f);
	}

	return clipDelta;
}

void MoveOperateMode::setMode(TMode mode)
{
	m_mode = mode;
}

void MoveOperateMode::resetSpacePoint(QPoint pos)
{
	m_spacePoint = process(pos);
}