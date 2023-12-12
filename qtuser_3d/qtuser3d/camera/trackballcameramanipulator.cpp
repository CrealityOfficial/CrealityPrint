#include "qtuser3d/camera/trackballcameramanipulator.h"
#include "qtuser3d/camera/screencamera.h"
#include "qtuser3d/math/angles.h"
#include "qtuser3d/math/space3d.h"
#include "qtuser3d/math/rectutil.h"
#include "qtuser3d/math/const.h"

namespace qtuser_3d
{
	TrackballCameraManipulator::TrackballCameraManipulator(QObject* parent)
		:CameraMouseManipulator(parent)
		, m_RightHorizontal(true)
	{

	}

	TrackballCameraManipulator::~TrackballCameraManipulator()
	{

	}

	void TrackballCameraManipulator::setNeed360Rotate(bool is_need)
	{
		//
	}

	void TrackballCameraManipulator::onCameraChanged()
	{
		m_savePoint = QPoint(-1, -1);
	}

	void TrackballCameraManipulator::onRightMouseButtonPress(QMouseEvent* event)
	{
		m_savePoint = event->pos();
		CameraMouseManipulator::onRightMouseButtonPress(event);
	}

	void TrackballCameraManipulator::onRightMouseButtonMove(QMouseEvent* event)
	{
		if (m_screenCamera)
		{
			QPoint pos = m_screenCamera->flipY(event->pos());
			performRotate(pos);
		}
	}

	void TrackballCameraManipulator::onRightMouseButtonRelease(QMouseEvent* event)
	{
		m_savePoint = event->pos();
	}

	void TrackballCameraManipulator::onMidMouseButtonPress(QMouseEvent* event)
	{
		m_savePoint = event->pos();
		CameraMouseManipulator::onRightMouseButtonPress(event);
	}

	void TrackballCameraManipulator::onMidMouseButtonMove(QMouseEvent* event)
	{
		if (!m_screenCamera)
			return;

		QPoint p = event->pos();

		//qDebug() << p;
		if (m_savePoint == p)
		{
			//restore();
			return;
		}

		performTranslate(p);

		m_savePoint = p;

		m_screenCamera->notifyCameraChanged();
	}


	void TrackballCameraManipulator::performTranslate(const QPoint& pos)
	{
		QVector3D viewCenter = m_camera->viewCenter();
		QVector3D position = m_camera->position();

		QVector3D dir = viewCenter - position;
		dir.normalize();

		QPoint savePoint = QPoint(m_savePoint.x(), m_savePoint.y());
		qtuser_3d::Ray ray0 = m_screenCamera->screenRay(m_savePoint);
		qtuser_3d::Ray ray = m_screenCamera->screenRay(pos);

		QVector3D c0, c;
		qtuser_3d::lineCollidePlane(viewCenter, dir, ray0, c0);
		qtuser_3d::lineCollidePlane(viewCenter, dir, ray, c);

		QVector3D delta = c0 - c;
		QVector3D newPosition = position + delta;
		QVector3D newViewCenter = viewCenter + delta;

		m_camera->setPosition(newPosition);
		m_camera->setViewCenter(newViewCenter);

		m_screenCamera->updateNearFar();
	}

	void TrackballCameraManipulator::setRightHorizontal(bool horizontal)
	{
		m_RightHorizontal = horizontal;
	}

	void TrackballCameraManipulator::performRotate(const QPoint& p)
	{
		if ((m_savePoint == p))
		{
			restore();
			return;
		}

		QVector2D xy0((float)p.x(), (float)p.y());
		QVector2D xy1((float)m_savePoint.x(), (float)m_savePoint.y());

		QSize size = m_screenCamera->size();
		QVector2D vsize((float)size.width(), (float)size.height());
		QVector2D center = vsize / 2.0f;

		QVector3D p0 = qtuser_3d::point3DFromVector2D(xy0, center, vsize, false);
		QVector3D p1 = qtuser_3d::point3DFromVector2D(xy1, center, vsize, false);
		float angle = qtuser_3d::angleOfVector3D2(p0, p1);

		if (angle == 0.0f)
		{
			restore();
			return;
		}

		QVector3D axis = QVector3D::crossProduct(p0, p1);
		axis.normalize();

		QMatrix4x4 viewMatrix;
		viewMatrix.lookAt(m_savePosition, m_saveCenter, m_saveUp);

		QMatrix4x4 invViewMatrix = viewMatrix.inverted();
		//
		QVector4D axisCC = invViewMatrix * QVector4D(axis, 0.0f);
		QVector3D axisC = axisCC.toVector3D();
		float angleC = 1.6f * angle;
		//QVector3D axisC = QVector3D(1.0f, 0.0f, 0.0f);
		//float angleC = 10.0f;

		QMatrix4x4 rot;
		rot.rotate(angleC, axisC);

		QVector3D newUp = rot * m_saveUp;
		newUp.normalize();

		QVector3D delta = m_saveCenter - m_rotateCenter;
		float D = delta.length();
		delta.normalize();
		delta = rot * delta;
		delta.normalize();
		
		QVector3D saveDir = m_saveCenter - m_savePosition;
		float distance = saveDir.length();

		saveDir.normalize();
		QVector3D newDir = rot * saveDir;
		newDir.normalize();
		QVector3D newPosition = m_rotateCenter - newDir * distance + delta * D;

		QVector3D newViewCenter = m_rotateCenter + delta * D;
		if (m_RightHorizontal)
		{
			//QVector3D oldRight = QVector3D::crossProduct(saveDir, m_saveUp);
			//oldRight = rot * oldRight;
			//oldRight.setZ(0.0f);
			//oldRight.normalize();
			//newUp = QVector3D::crossProduct(oldRight, newDir);
		}

		m_camera->setViewCenter(newViewCenter);
		m_camera->setUpVector(newUp);
		m_camera->setPosition(newPosition);
	}
}
