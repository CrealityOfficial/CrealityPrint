#include "eularmousemanipulator.h"
#include "qtuser3d/camera/screencamera.h"
#include "qtuser3d/math/space3d.h"
#include "qtuser3d/math/const.h"
#include <QtCore/QDebug>

namespace qtuser_3d
{
	EularMouseManipulator::EularMouseManipulator(QObject* parent)
		:CameraMouseManipulator(parent)
	{
		m_revertY = false;

		m_need360Rotate = false;
		m_needAroundRotate = false;
	}

	EularMouseManipulator::~EularMouseManipulator()
	{
	}

	void EularMouseManipulator::setNeed360Rotate(bool is_need)
	{
		m_need360Rotate = is_need;
	}

	void EularMouseManipulator::setNeedAroundRotate(bool is_need)
	{
		m_needAroundRotate = is_need;
	}

	void EularMouseManipulator::onRightMouseButtonPress(QMouseEvent* event)
	{
		m_savePoint = event->pos();
		CameraMouseManipulator::onRightMouseButtonPress(event);
	}

	void EularMouseManipulator::onRightMouseButtonMove(QMouseEvent* event)
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

		if (m_needAroundRotate)
		{
			performAynRotate(p);
		}
		else if (m_need360Rotate)
		{
			performRotate360(p);
		}
		else
		{
			performRotate(p);
		}

		m_savePoint = p;

		m_screenCamera->notifyCameraChanged();
	}


	void EularMouseManipulator::onMidMouseButtonPress(QMouseEvent* event)
	{
		m_savePoint = event->pos();
		CameraMouseManipulator::onRightMouseButtonPress(event);
	}

	void EularMouseManipulator::onMidMouseButtonMove(QMouseEvent* event)
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


	void EularMouseManipulator::performTranslate(const QPoint& pos)
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

	void EularMouseManipulator::performRotate(const QPoint& pos)
	{
		QVector3D viewCenter = m_camera->viewCenter();
		QVector3D position = m_camera->position();
		QVector3D horizontal = m_screenCamera->horizontal();

		QPoint delta = pos - m_savePoint;
		delta *= m_mouseSensitivity;

		float hangle = -0.1f * (float)delta.x();
		QQuaternion hq = QQuaternion::fromAxisAndAngle(QVector3D(0.0f, 0.0f, 1.0f), hangle);
		QVector3D h = hq * horizontal;
		h.normalize();

		float vangle = m_screenCamera->verticalAngle() + 0.003f * (float)delta.y();
		if (vangle < 0.0f) vangle = 0.0f;
		if (vangle > M_PI) vangle = (float)M_PI;

		QVector3D dir;
		QVector3D right = h;
		if (vangle > 0.0f && vangle < M_PI)
		{
			dir = QVector3D(-h.y(), h.x(), 0.0f);
			float z = dir.length() / (tanf(vangle));
			dir.setZ(z);
			dir.normalize();
		}
		else if (vangle <= 0.0f)
		{
			dir = QVector3D(0.0f, 0.0f, 1.0f);
		}
		else if (vangle >= M_PI)
		{
			dir = QVector3D(0.0f, 0.0f, -1.0f);
		}

		QVector3D saveDir = viewCenter - position;
		float distance = saveDir.length();

		QVector3D newPosition = viewCenter - dir * distance;
		QVector3D up = QVector3D::crossProduct(right, dir);
		m_camera->setUpVector(up);
		m_camera->setPosition(newPosition);

		if (up.x() == 0.0f && up.y() == 0.0f && up.z() == 0.0f)
		{
			qDebug() << "error";
		}

		m_screenCamera->updateNearFar();
	}

	QMatrix4x4 EularMouseManipulator::rotMatrix(const QPoint& pos)
	{
		QVector3D viewCenter = m_camera->viewCenter();
		QVector3D position = m_camera->position();
		QVector3D up = m_camera->upVector();

		QPoint delta = pos - m_savePoint;
		delta *= m_mouseSensitivity;
		float hangle = - m_hangleDelta * (float)delta.x();
		QQuaternion hq = QQuaternion::fromAxisAndAngle(QVector3D(0.0f, 0.0f, 1.0f), hangle);
		
		float vangle = -m_vangleDelta * (float)delta.y();
		QVector3D dir = viewCenter - position;
		dir.normalize();
		QVector3D h = QVector3D::crossProduct(dir, up);
		h.setZ(0.0f);
		h.normalize();
		QQuaternion vq = QQuaternion::fromAxisAndAngle(h, vangle);

		QMatrix4x4 m;
		m.setToIdentity();
		m.rotate(hq * vq);
		return m;
	}

	void EularMouseManipulator::performAynRotate(const QPoint& pos)
	{
		QMatrix4x4 rot = rotMatrix(pos);

		QVector3D viewCenter = m_camera->viewCenter();
		QVector3D position = m_camera->position();
		QVector3D up = m_camera->upVector();

		QVector3D newUp = rot * up;
		newUp.normalize();

		QVector3D delta = viewCenter - m_rotateCenter;
		float D = delta.length();
		delta.normalize();
		delta = rot * delta;
		delta.normalize();

		QVector3D saveDir = viewCenter - position;
		float distance = saveDir.length();

		saveDir.normalize();
		QVector3D newDir = rot * saveDir;
		newDir.normalize();
		QVector3D newPosition = m_rotateCenter - newDir * distance + delta * D;

		QVector3D newViewCenter = m_rotateCenter + delta * D;

		m_camera->setViewCenter(newViewCenter);
		m_camera->setUpVector(newUp);
		m_camera->setPosition(newPosition);

		m_screenCamera->updateNearFar();
	}

	void EularMouseManipulator::performRotate360(const QPoint& pos)
	{
		QVector3D viewCenter = m_camera->viewCenter();
		QVector3D position = m_camera->position();
		QVector3D horizontal = m_screenCamera->horizontal();

		QPoint delta = pos - m_savePoint;
		delta *= m_mouseSensitivity;

		float hangle = -0.1f * (float)delta.x();
		QQuaternion hq = QQuaternion::fromAxisAndAngle(QVector3D(0.0f, 0.0f, 1.0f), hangle);
		QVector3D h = hq * horizontal;
		h.normalize();

		float o_angle = m_screenCamera->verticalAngle360();

		float vangle = o_angle + 0.003f * (float)delta.y();
//		if (vangle < 0.0f) vangle = 0.0f;
		if (vangle > 2 * M_PI) vangle -= (float)(2 * M_PI);

		QVector3D dir;
		QVector3D right = h;

		if (vangle < 0.001f && vangle > -0.001f)
		{
			dir = QVector3D(0.0f, 0.0f, 1.0f);
		}
		else if (vangle > M_PI - 0.001 && vangle < M_PI + 0.001)
		{
			dir = QVector3D(0.0f, 0.0f, -1.0f);
		}
		else
		{
			dir = QVector3D(-h.y(), h.x(), 0.0f);
			float z = dir.length() / (tanf(vangle));
			dir.setZ(z);
			dir.normalize();

			if (vangle > M_PI || vangle < 0)
			{
				dir.setX(-dir.x());
				dir.setY(-dir.y());
				dir.setZ(-dir.z());
			}
		}

		QVector3D saveDir = viewCenter - position;
		float distance = saveDir.length();

		QVector3D newPosition = viewCenter - dir * distance;
		QVector3D up = QVector3D::crossProduct(right, dir);
		m_camera->setUpVector(up);
		m_camera->setPosition(newPosition);

		if (up.x() == 0.0f && up.y() == 0.0f && up.z() == 0.0f)
		{
			qDebug() << "error";
		}

		m_screenCamera->updateNearFar();
	}
}
