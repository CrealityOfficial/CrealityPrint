#include "cameramousemanipulator.h"
#include "qtuser3d/math/rectutil.h"
#include "qtuser3d/camera/screencamera.h"
#include <qdebug.h>

namespace qtuser_3d
{
	CameraMouseManipulator::CameraMouseManipulator(QObject* parent)
		:QObject(parent)
		, m_screenCamera(nullptr)
		, m_camera(nullptr)
		, m_savePoint(QPoint(-1, -1))
		, m_revertY(true)
		, m_rotateCenter(0.0f, 0.0f, 0.0f)
		, m_hangleDelta(0.1f)
		, m_vangleDelta(0.1f)
	{
	}

	CameraMouseManipulator::~CameraMouseManipulator()
	{
	}

	void CameraMouseManipulator::setMouseSensitivity(float sensitivity)
	{
		m_mouseSensitivity = sensitivity;
	}

	void CameraMouseManipulator::setNeed360Rotate(bool is_need)
	{
		//
	}

	void CameraMouseManipulator::setNeedAroundRotate(bool is_need)
	{
	}

	void CameraMouseManipulator::setCamera(ScreenCamera* screenCamera)
	{
		if (m_screenCamera == screenCamera) return;
		m_screenCamera = screenCamera;
		if (m_screenCamera)
		{
			m_camera = m_screenCamera->camera();
		}
		onCameraChanged();
	}

	void CameraMouseManipulator::setRotateCenter(const QVector3D& rotateCenter)
	{
		m_rotateCenter = rotateCenter;
	}

	QVector3D CameraMouseManipulator::rotateCenter() const
	{
		return m_rotateCenter;
	}

	void CameraMouseManipulator::setRotateSpeedDelta(float hDelta, float vDelta)
	{
		m_hangleDelta = hDelta;
		m_vangleDelta = vDelta;
	}
	
	void CameraMouseManipulator::onRightMouseButtonPress(QMouseEvent* event)
	{
		if (m_screenCamera)
		{
			if (m_revertY)
				m_savePoint = m_screenCamera->flipY(event->pos());
			else
				m_savePoint = event->pos();
			m_saveUp = m_camera->upVector();
			m_saveCenter = m_camera->viewCenter();
			m_savePosition = m_camera->position();
		}
	}

	void CameraMouseManipulator::onRightMouseButtonRelease(QMouseEvent* event)
	{
		
	}

	void CameraMouseManipulator::onRightMouseButtonMove(QMouseEvent* event)
	{

	}

	void CameraMouseManipulator::onRightMouseButtonClick(QMouseEvent* event)
	{

	}


	void CameraMouseManipulator::onMidMouseButtonPress(QMouseEvent* event)
	{
		if (m_screenCamera)
		{
			if (m_revertY)
				m_savePoint = m_screenCamera->flipY(event->pos());
			else
				m_savePoint = event->pos();
			m_saveUp = m_camera->upVector();
			m_saveCenter = m_camera->viewCenter();
			m_savePosition = m_camera->position();
		}
	}

	void CameraMouseManipulator::onMidMouseButtonRelease(QMouseEvent* event)
	{

	}

	void CameraMouseManipulator::onMidMouseButtonMove(QMouseEvent* event)
	{

	}

	void CameraMouseManipulator::onMidMouseButtonClick(QMouseEvent* event)
	{

	}



	void CameraMouseManipulator::onCameraChanged()
	{

	}

	void CameraMouseManipulator::restore()
	{
		if (m_camera)
		{
			m_camera->setPosition(m_savePosition);
			m_camera->setUpVector(m_saveUp);
			m_camera->setViewCenter(m_saveCenter);
		}
	}
}
