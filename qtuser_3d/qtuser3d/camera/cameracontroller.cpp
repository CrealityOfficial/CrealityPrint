#include "qtuser3d/camera/cameracontroller.h"
#include "qtuser3d/camera/screencamera.h"
#include "qtuser3d/camera/trackballcameramanipulator.h"
#include "qtuser3d/camera/eularmousemanipulator.h"
#include "qmath.h"
#include <qdebug.h>

#define USE_EULAR_MANIPULATOR
#define ENABLE_ZOOM_AROUND_CURSOR   1
namespace qtuser_3d
{
	CameraController::CameraController(QObject* parent)
		:QObject(parent)
		, m_screenCamera(nullptr)
		, m_cameraManipulator(nullptr)
		, m_mask(7)
		, m_enableZoomAroundCursor(true)
	{

#ifdef USE_EULAR_MANIPULATOR
		m_cameraManipulator = new qtuser_3d::EularMouseManipulator(this);
#else
		m_cameraManipulator = new qtuser_3d::TrackballCameraManipulator(this);
#endif

	}

	CameraController::~CameraController()
	{
	}

	void CameraController::useTrackableCamera()
	{
		if (m_cameraManipulator)
			delete m_cameraManipulator;

		m_cameraManipulator = new qtuser_3d::TrackballCameraManipulator(this);

		setScreenCamera(m_screenCamera);
	}

	void CameraController::useEularCamera()
	{
		if (m_cameraManipulator)
			delete m_cameraManipulator;

		m_cameraManipulator = new qtuser_3d::EularMouseManipulator(this);

		setScreenCamera(m_screenCamera);
	}

	void CameraController::setMouseSensitivity(float sensitivity)
	{
		m_cameraManipulator->setMouseSensitivity(sensitivity);
	}

	void CameraController::setNeed360Rotate(bool is_need)
	{
		m_cameraManipulator->setNeed360Rotate(is_need);
	}

	void CameraController::setNeedAroundRotate(bool is_need)
	{
		if (m_cameraManipulator)
			m_cameraManipulator->setNeedAroundRotate(is_need);
	}

	void CameraController::setRotateSpeedDelta(float hDelta, float vDelta)
	{
		if (m_cameraManipulator)
			m_cameraManipulator->setRotateSpeedDelta(hDelta, vDelta);
	}

	void CameraController::setRotateCenter(const QVector3D& rotateCenter)
	{
		if (m_cameraManipulator)
			m_cameraManipulator->setRotateCenter(rotateCenter);
	}

	QVector3D CameraController::rotateCenter() const
	{
		QVector3D center;
		if (m_cameraManipulator)
			center = m_cameraManipulator->rotateCenter();
		return center;
	}

	void CameraController::setScreenCamera(qtuser_3d::ScreenCamera* camera)
	{
		m_screenCamera = camera;
		//Qt3DRender::QCamera* pickerCamera = m_screenCamera->camera();
		m_cameraManipulator->setCamera(m_screenCamera);
		if (camera)
		{
			m_cursorPos = QPoint(camera->size().width() / 2.0, camera->size().height() / 2.0);
		}
	}

	qtuser_3d::ScreenCamera* CameraController::screenCamera()
	{
		return m_screenCamera;
	}
	void CameraController::viewSwitch()
	{
		Qt3DRender::QCamera* pickerCamera = m_screenCamera->camera();
		if (pickerCamera->projectionType() == Qt3DRender::QCameraLens::OrthographicProjection)
			viewFromPerspective();
		else if (pickerCamera->projectionType() == Qt3DRender::QCameraLens::PerspectiveProjection)
			viewFromOrthographic();
	}

	void CameraController::viewFromOrthographic()
	{
		Qt3DRender::QCamera* pickerCamera = m_screenCamera->camera();
		if (pickerCamera->projectionType() == Qt3DRender::QCameraLens::OrthographicProjection)
			return;

		m_cameraPos = pickerCamera->position();
		QMatrix4x4 m_viewMatrix = pickerCamera->projectionMatrix();

		float fnearPlane = pickerCamera->nearPlane();
		float ffarPlane = pickerCamera->farPlane();

		float r = (ffarPlane - fnearPlane) / 2 + fnearPlane;
		float flen = r * tan(pickerCamera->fieldOfView() * M_PI / 360) * 0.25;
		qDebug() << "flen =" << flen;
		float top = flen;
		float bottom = -flen;

		float fratio = pickerCamera->aspectRatio();
		float left = -fratio * flen;
		float right = fratio * flen;

		pickerCamera->setTop(top);
		pickerCamera->setBottom(bottom);
		pickerCamera->setLeft(left);
		pickerCamera->setRight(right);

		pickerCamera->setProjectionType(Qt3DRender::QCameraLens::ProjectionType::OrthographicProjection);
		emit signalViewChanged(false);
		m_screenCamera->notifyCameraChanged();
	}

	void CameraController::viewFromPerspective()
	{
		Qt3DRender::QCamera* pickerCamera = m_screenCamera->camera();
		if (pickerCamera->projectionType() == Qt3DRender::QCameraLens::PerspectiveProjection)
			return;

		pickerCamera->setProjectionType(Qt3DRender::QCameraLens::ProjectionType::PerspectiveProjection);
		emit signalViewChanged(false);
		m_screenCamera->notifyCameraChanged();
	}

	void CameraController::showSelf()
	{
		qDebug() << "show CameraController:";
		m_screenCamera->showSelf();
	}

	void CameraController::sendViewUpdateSingle(bool capture)
	{
		emit signalViewChanged(capture);
	}

	void CameraController::fittingBoundingBox(const qtuser_3d::Box3D& box)
	{
		if (m_screenCamera)
			m_screenCamera->fittingBoundingBox(box);
	}

	void CameraController::updateNearFar(const qtuser_3d::Box3D& box)
	{
		if (m_screenCamera)
			m_screenCamera->updateNearFar(box);
	}

	void CameraController::home(const qtuser_3d::Box3D& box, int type)
	{
		if (m_screenCamera)
			m_screenCamera->home(box, type);
	}

	void CameraController::onResize(const QSize& size)
	{
		if (m_screenCamera)
		{
			m_screenCamera->setScreenSize(size);

			//������������ : �����ӽ�����£�resize�¼��� �������������
			Qt3DRender::QCamera* pickerCamera = m_screenCamera->camera();
			if (pickerCamera && pickerCamera->projectionType() == Qt3DRender::QCameraLens::OrthographicProjection)
				m_screenCamera->zoom(1.0f);
		}
	}

	QVector3D CameraController::getViewPosition() const
	{
		return m_screenCamera->camera()->position();
	}
	QVector3D CameraController::getViewupVector() const
	{
		return m_screenCamera->camera()->upVector();
	}
	QVector3D CameraController::getviewCenter() const
	{
		return m_screenCamera->camera()->viewCenter();
	}

	QVector3D CameraController::getViewDir() const
	{
		QVector3D v = -m_screenCamera->camera()->viewVector();
		return v.normalized();
	}

	void CameraController::setViewPosition(const QVector3D position)
	{
		m_screenCamera->camera()->setPosition(position);
	}
	void CameraController::setViewupVector(const QVector3D upVector)
	{
		m_screenCamera->camera()->setUpVector(upVector);
	}
	void CameraController::setviewCenter(const QVector3D viewCenter)
	{
		m_screenCamera->camera()->setViewCenter(viewCenter);
	}

	void CameraController::onRightMouseButtonPress(QMouseEvent* event)
	{
		if (!(m_mask & 1))
			return;

		m_cameraManipulator->onRightMouseButtonPress(event);
		
		emit signalRightMousePressed(true);
	}

	void CameraController::onRightMouseButtonMove(QMouseEvent* event)
	{
		if (!(m_mask & 1))
			return;

		m_cameraManipulator->onRightMouseButtonMove(event);
		emit signalViewChanged(false);

		QVector3D position = getViewPosition();
		QVector3D upview = getViewupVector();
		QVector3D center = getviewCenter();
		QVector3D diff = position - center;
		emit signalCameraChaged(position - center, getViewupVector());
	}

	void CameraController::onRightMouseButtonRelease(QMouseEvent* event)
	{
		if (!(m_mask & 1))
			return;

		emit signalViewChanged(true);
		emit signalRightMousePressed(false);
	}

	void CameraController::onRightMouseButtonClick(QMouseEvent* event)
	{
		(void)(event);
	}


	void CameraController::onMidMouseButtonPress(QMouseEvent* event)
	{
		if (!(m_mask & 4))
			return;

		m_cameraManipulator->onMidMouseButtonPress(event);
		emit signalMidMousePressed(true);
	}

	void CameraController::onMidMouseButtonMove(QMouseEvent* event)
	{
		if (!(m_mask & 4))
			return;

		m_cameraManipulator->onMidMouseButtonMove(event);
		emit signalViewChanged(false);
	}

	void CameraController::onMidMouseButtonRelease(QMouseEvent* event)
	{
		if (!(m_mask & 4))
			return;

		emit signalViewChanged(true);
		emit signalMidMousePressed(false);
	}

	void CameraController::onMidMouseButtonClick(QMouseEvent* event)
	{
		(void)(event);
	}

	void CameraController::enableRotate(bool enabled)
	{
		if (enabled)
			m_mask |= 1;
		else
			m_mask &= ~1;
	}

	void CameraController::enableScale(bool enabled)
	{
		if (enabled)
			m_mask |= 2;
		else
			m_mask &= ~2;
	}

	void CameraController::enableTranslate(bool enabled)
	{
		if (enabled)
			m_mask |= 4;
		else
			m_mask &= ~4;
	}

	void CameraController::onWheelEvent(QWheelEvent* event)
	{
		if (!(m_mask & 2))
			return;

//#if ENABLE_ZOOM_AROUND_CURSOR
if (m_enableZoomAroundCursor) {
		QVector3D oldPosition = getViewPosition();
		QVector3D oldViewCenter = getviewCenter();
		QVector3D planeCenter = oldViewCenter;
		
		QVector3D cursorPosition;

		QVector3D planeXDir = QVector3D(1.0, 0.0, 0.0);
		QVector3D planeYDir = QVector3D(0.0, 1.0, 0.0);
		QVector3D planeZDir = QVector3D(0.0, 0.0, 1.0);

		Ray ray = m_screenCamera->screenRay(event->pos());
		bool collide = cameraRayPoint(m_screenCamera, m_cursorPos, planeCenter, ray.dir, cursorPosition);
		if (!collide)
		{
			QVector3D testingPos;
			collide = cameraRayPoint(m_screenCamera, m_cursorPos, planeCenter, planeXDir, testingPos);
			if (collide)
			{
				cursorPosition = testingPos;
			}

			collide = cameraRayPoint(m_screenCamera, m_cursorPos, planeCenter, planeYDir, testingPos);
			if (collide)
			{
				if (planeCenter.distanceToPoint(testingPos) < planeCenter.distanceToPoint(cursorPosition))
				{
					cursorPosition = testingPos;
				}
			}

			collide = cameraRayPoint(m_screenCamera, m_cursorPos, planeCenter, planeZDir, testingPos);
			if (collide)
			{
				if (planeCenter.distanceToPoint(testingPos) < planeCenter.distanceToPoint(cursorPosition))
				{
					cursorPosition = testingPos;
				}
			}
		}
		 
		if (true)
		{
			bool zoomIn = event->delta() > 0; //�Ƿ�Ŵ�

			if (m_screenCamera && m_screenCamera->zoom(zoomIn ? 1.0f / 1.1f : 1.1f))
			{
				QSize half = m_screenCamera->size() / 2.0;
				QPoint screenCenter = QPoint(half.width(), half.height());

				QPoint b = m_cursorPos - screenCenter;
				float refLength = qSqrt(b.x() * b.x() + b.y() * b.y());

				float itrRate = 0.5;
				float accRate = itrRate;

				float k = zoomIn ? 1.0 : -1.0;

				for (size_t i = 0; i < 10; i++)
				{
					QVector3D offset = (cursorPosition - oldViewCenter) * accRate * k;
					QVector3D tempViewCenter = oldViewCenter + offset;
					QVector3D tempPosition = oldPosition + offset;

					setviewCenter(tempViewCenter);
					setViewPosition(tempPosition);
					QMatrix4x4 viewMatrix1 = m_screenCamera->viewMatrix();
					QMatrix4x4 projectionMatrix1 = m_screenCamera->projectionMatrix();

					QVector3D p = projectionMatrix1 * viewMatrix1 * cursorPosition;
					float x = (p.x() + 1.0) / 2.0 * m_screenCamera->size().width();
					float y = (1.0 - (p.y() + 1.0) / 2.0) * m_screenCamera->size().height();
					QPoint temCursorPos = QPoint(x, y);

					QPoint a = temCursorPos - screenCenter;
					float tmpLength = qSqrt(a.x() * a.x() + a.y() * a.y());

					//qDebug() << "rate b=" << (float)b.x() / (float)b.y() << "a=" << (float)a.x() / (float)a.y();
					//qDebug() << "(x, y) =" << x << y;

					if (abs(tmpLength - refLength) < 1.5)
					{
						break;
					}
					else
					{
						itrRate *= 0.5;

						if (tmpLength > refLength)
						{
							accRate += itrRate * k;
						}
						else {
							accRate -= itrRate * k;
						}

					}
				}

				emit signalViewChanged(true);
			}
		}

//#else
}else {
		if (m_screenCamera && m_screenCamera->zoom(event->delta() > 0 ? 1.0f / 1.1f : 1.1f))
		{
			emit signalViewChanged(true);
		}
//#endif // ENABLE_ZOOM_AROUND_CURSOR
}
	}

	void CameraController::viewFromBottom(QVector3D* specificCenter)
	{
		QVector3D dir(0.0f, 0.0f, 1.0f);
		QVector3D right(1.0f, 0.0f, 0.0f);
		view(dir, right, specificCenter);

		QVector3D position = getViewPosition();
		QVector3D upview = getViewupVector();
		QVector3D center = getviewCenter();
		QVector3D diff = position - center;
		emit signalCameraChaged(position - center, getViewupVector());

		if (m_screenCamera)
			m_screenCamera->notifyCameraChanged();
	}

	void CameraController::viewFromTop(QVector3D* specificCenter)
	{
		QVector3D dir(0.0f, 0.0f, -1.0f);
		QVector3D right(1.0f, 0.0f, 0.0f);
		view(dir, right, specificCenter);

		QVector3D position = getViewPosition();
		QVector3D upview = getViewupVector();
		QVector3D center = getviewCenter();
		QVector3D diff = position - center;
		emit signalCameraChaged(position - center, getViewupVector());

		if (m_screenCamera)
			m_screenCamera->notifyCameraChanged();
	}

	void CameraController::viewFromLeft(QVector3D* specificCenter)
	{
		QVector3D dir(1.0f, 0.0f, 0.0f);
		QVector3D right(0.0f, -1.0f, 0.0f);
		view(dir, right, specificCenter);

		QVector3D position = getViewPosition();
		QVector3D upview = getViewupVector();
		QVector3D center = getviewCenter();
		QVector3D diff = position - center;
		emit signalCameraChaged(position - center, getViewupVector());

		if (m_screenCamera)
			m_screenCamera->notifyCameraChanged();
	}

	void CameraController::viewFromRight(QVector3D* specificCenter)
	{
		QVector3D dir(-1.0f, 0.0f, 0.0f);
		QVector3D right(0.0f, 1.0f, 0.0f);
		view(dir, right, specificCenter);

		QVector3D position = getViewPosition();
		QVector3D upview = getViewupVector();
		QVector3D center = getviewCenter();
		QVector3D diff = position - center;
		emit signalCameraChaged(position - center, getViewupVector());

		if (m_screenCamera)
			m_screenCamera->notifyCameraChanged();
	}

	void CameraController::viewFromFront(QVector3D* specificCenter)
	{
		QVector3D dir(0.0f, 1.0f, 0.0f);
		QVector3D right(1.0f, 0.0f, 0.0f);
		view(dir, right, specificCenter);

		QVector3D position = getViewPosition();
		QVector3D upview = getViewupVector();
		QVector3D center = getviewCenter();
		QVector3D diff = position - center;
		emit signalCameraChaged(position - center, getViewupVector());

		if(m_screenCamera)
			m_screenCamera->notifyCameraChanged();
	}

	void CameraController::viewFromBack(QVector3D* specificCenter)
	{
		QVector3D dir(0.0f, -1.0f, 0.0f);
		QVector3D right(-1.0f, 0.0f, 0.0f);
		view(dir, right, specificCenter);

		QVector3D position = getViewPosition();
		QVector3D upview = getViewupVector();
		QVector3D center = getviewCenter();
		QVector3D diff = position - center;
		emit signalCameraChaged(position - center, getViewupVector());

		if (m_screenCamera)
			m_screenCamera->notifyCameraChanged();
	}

	void CameraController::viewFromUserSetting(QVector3D posion, QVector3D viewCenter, QVector3D upVector, QVector3D* specificCenter)
	{
		QVector3D dir = viewCenter - posion;
		QVector3D right = QVector3D::crossProduct(dir, upVector);
		view(dir, right, specificCenter);
	}

	void CameraController::view(const QVector3D& dir, const QVector3D& right, QVector3D* specificCenter)
	{
		if (m_screenCamera)
		{
			m_screenCamera->viewFrom(dir, right, specificCenter);
			emit signalViewChanged(true);
		}
	}

	void CameraController::viewEx(const QVector3D& newDir, const QVector3D& newUp, const QVector3D& homePosition, const QVector3D& homeViewCenter, const QVector3D& newCenter)
	{
		if (m_screenCamera)
		{
			m_screenCamera->viewFromEx(newDir, newUp, homePosition, homeViewCenter, newCenter);
			emit signalViewChanged(true);
		}
	}

	void CameraController::onHoverEnter(QHoverEvent* event)
	{

	}

	void CameraController::onHoverMove(QHoverEvent* event)
	{
		m_cursorPos = event->pos();
	}

	void CameraController::onHoverLeave(QHoverEvent* event)
	{

	}

	void CameraController::setEnableZoomAroundCursor(bool enable)
	{
		m_enableZoomAroundCursor = enable;
	}
}
