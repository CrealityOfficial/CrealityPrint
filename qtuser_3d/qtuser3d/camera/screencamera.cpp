#include "qtuser3d/camera/screencamera.h"
#include "qtuser3d/math/rectutil.h"
#include "qtuser3d/math/angles.h"
#include "qtuser3d/math/space3d.h"
#include <cmath>
#include <QtMath>

namespace qtuser_3d
{
	ScreenCamera::ScreenCamera(QObject* parent, Qt3DRender::QCamera* camera)
		:QObject(parent)
		,m_camera(camera)
		, m_minDistance(1.0f)
		, m_maxDistance(4000.0f)
		, m_updateNearFarRuntime(true)
		, m_externalCamera(true)
		, m_maxFovy(50.0f)
		, m_minFovy(0.1f)
	{
		if (!m_camera)
		{
			m_camera = new Qt3DRender::QCamera();
			m_externalCamera = false;
		}
		m_camera->lens()->setPerspectiveProjection(30.0f, 16.0f / 9.0f, 0.1f, 3000.0f);
		m_camera->setPosition(QVector3D(0.0f, -40.0f, 0.0f));
		m_camera->setViewCenter(QVector3D(0.0f, 0.0f, 0.0f));
		m_camera->setUpVector(QVector3D(0.0f, 0.0f, 1.0f));
	}

	ScreenCamera::~ScreenCamera()
	{
		if (!m_externalCamera && !m_camera->parent())
		{
			delete m_camera;
			m_camera = nullptr;
		}
	}

	Qt3DRender::QCamera* ScreenCamera::camera()
	{
		return m_camera;
	}

	void ScreenCamera::setScreenSize(const QSize& size)
	{
		m_size = size;

		if (m_size.height() != 0)
		{
			float aspectRatio = (float)m_size.width() / (float)m_size.height();
			m_camera->setAspectRatio(aspectRatio);
		}
	}

	QSize ScreenCamera::size()
	{
		return m_size;
	}

	void ScreenCamera::setFovyLimit(float maxFovy, float minFovy)
	{
		m_maxFovy = maxFovy;
		m_minFovy = minFovy;
	}

	void ScreenCamera::fittingBoundingBox(const qtuser_3d::Box3D& box)
	{
		QVector3D bsize = box.size();
        float len = qMax(bsize.x(), qMax(bsize.y(), bsize.z()));
		QVector3D center = box.center();
//		center.setZ(center.z() - box.size().z() * 0.4);
		center.setZ(0);

		//float nearPlane = 0.2f * len;
		//float farPlane = 10.0f * len;

		//osg::Vec3f delta = m_position - m_focus;
		float newDistance = 2.0f * len;

		QVector3D cameraPosition = m_camera->position();
		QVector3D cameraCenter = m_camera->viewCenter();
		QVector3D cameraView = cameraCenter - cameraPosition;
		cameraView.normalize();

		QVector3D newPosition = center - cameraView * newDistance;
		m_camera->setPosition(newPosition);
		m_camera->setViewCenter(center);
		//m_camera->setNearPlane(nearPlane);
		//m_camera->setFarPlane(farPlane);

		m_orignCenter = center;

		m_box = box;
		float dmax = viewAllLen(m_box.size().length() / 2.0f) * 1.5;
		setMaxLimitDistance(dmax);

		_updateNearFar(m_box);
	}

	void ScreenCamera::fittingBoundingBoxEx(const qtuser_3d::Box3D& box, const QVector3D& homeDir, const QVector3D& homeUp, const QVector3D& homePosition, const QVector3D& homeViewCenter)
	{
		QVector3D bsize = box.size();
		float len = qMax(bsize.x(), qMax(bsize.y(), bsize.z()));
		QVector3D center = box.center();

		QVector3D newUp = homeUp;
		newUp.normalize();

		float d = (homeViewCenter - homePosition).length() * (len / 8.0f);
		QVector3D newPosition = center - homeDir.normalized() * d;

		m_camera->setViewCenter(center);
		m_camera->setUpVector(newUp);
		m_camera->setPosition(newPosition);

		m_orignCenter = center;

		m_box = box;
		float dmax = viewAllLen(m_box.size().length() / 2.0f) * 1.5;
		setMaxLimitDistance(dmax);

		_updateNearFar(m_box);
	}

	void ScreenCamera::viewFromEx(const QVector3D& newDir, const QVector3D& newUp, const QVector3D& homePosition, const QVector3D& homeViewCenter, const QVector3D& newCenter)
	{
		QVector3D bsize = m_box.size();
		float len = qMax(bsize.x(), qMax(bsize.y(), bsize.z()));

		float d = (homeViewCenter - homePosition).length() * (len / 8.0f);
		QVector3D newPosition = newCenter - newDir.normalized() * d;

		QVector3D up = newUp;
		up.normalize();

		m_camera->setViewCenter(newCenter);
		m_camera->setUpVector(up);
		m_camera->setPosition(newPosition);
	}

	void ScreenCamera::updateNearFar(const qtuser_3d::Box3D& box)
	{
		m_box = box;

		_updateNearFar(m_box);
	}

	void ScreenCamera::updateNearFar()
	{
		_updateNearFar(m_box);
	}

	bool ScreenCamera::checkUpState()
	{
		QVector3D viewDir = camera()->viewVector();
		viewDir = viewDir.normalized();

		QVector3D normal = QVector3D(0.0f, 0.0f, -1.0f);
		float product = viewDir.dotProduct(viewDir, normal);
		return product > 0.5f;
	}

	void ScreenCamera::_updateNearFar(const qtuser_3d::Box3D& box)
	{
		if (!m_updateNearFarRuntime)
			return;

		QVector3D cameraPosition = m_camera->position();
		QVector3D cameraCenter = m_camera->viewCenter();
		QVector3D cameraView = cameraCenter - cameraPosition;
		cameraView.normalize();

		QVector3D center = box.center();
		float times = 1.5f;
		float r = box.size().length() / 2.0f;
		float d = QVector3D::dotProduct(cameraView, center - cameraPosition);
		float dmin = d - times * r;
		float dmax = d + times * r;

		//float fieldofview = m_camera->lens()->fieldOfView();
		//float aspect = m_camera->lens()->aspectRatio();

		float nearpos = dmin < 1.0f ? (box.size().length() > 1.0f ? 1.0f : dmin) : dmin;
		float farpos = dmax > 0.0f ? dmax : 3000.0f;

		m_camera->lens()->setNearPlane(nearpos);
		m_camera->lens()->setFarPlane(farpos + 3000.0f);
	}

	qtuser_3d::Ray ScreenCamera::screenRay(const QPoint& point)
	{
		QPointF p(0.0,0.0);
		p = qtuser_3d::viewportRatio(point, m_size);

		if (m_camera->projectionType() == Qt3DRender::QCameraLens::PerspectiveProjection)
			return screenRay(p);
		else if(m_camera->projectionType() == Qt3DRender::QCameraLens::OrthographicProjection)
			return screenRayOrthographic(p);
		else
			return screenRay(p);
	}

	qtuser_3d::Ray ScreenCamera::screenRay(const QPointF& point)
	{
		QVector3D position = m_camera->position();
		QVector3D center = m_camera->viewCenter();
		QVector3D dir = (center - position).normalized();
		QVector3D nearCenter = position + dir * m_camera->nearPlane();
		QVector3D left = QVector3D::crossProduct(dir, m_camera->upVector());
		left.normalize();
        float near_height = 2.0f * m_camera->nearPlane() * qTan(m_camera->fieldOfView() * 3.1415926f / 180.0f / 2.0f);
		float near_width = near_height * m_camera->aspectRatio();
		QVector3D nearPoint = nearCenter + m_camera->upVector() * point.y() * near_height + left * point.x() * near_width;

		qtuser_3d::Ray ray;
		ray.start = m_camera->position();
		ray.dir = nearPoint - ray.start;
		ray.dir.normalize();
		return ray;
	}

	qtuser_3d::Ray ScreenCamera::screenRayOrthographic(const QPointF& point)
	{
		QVector3D position = m_camera->position();
		QVector3D center = m_camera->viewCenter();
		QVector3D dir = (center - position).normalized();
		QVector3D nearCenter = position + dir * m_camera->nearPlane();

		float near_width = qAbs(m_camera->right() - m_camera->left()) * point.x();
		float near_height = qAbs(m_camera->top() - m_camera->bottom()) * point.y();
		QVector3D right = QVector3D::crossProduct(dir, m_camera->upVector());
		right.normalize();
		QVector3D nearPoint = nearCenter + m_camera->upVector() * near_height + right * near_width;

		qtuser_3d::Ray ray;
		ray.dir = dir;// direction vector
		ray.start = nearPoint - dir * m_camera->nearPlane();//start vector
		return ray;
	}

	float ScreenCamera::screenSpaceRatio(const QVector3D& position)
	{
		float ratio = 1.0f;
		if (m_camera)
		{
			float nearPlane = m_camera->nearPlane();
			float positionPlane = nearPlane;

			float h = 2.0f * nearPlane * tanf(m_camera->fieldOfView() * M_PI / 2.0f / 180.0f);
			QVector3D cameraCenter = m_camera->position();
			QVector3D cameraView = m_camera->viewCenter() - cameraCenter;
			cameraView.normalize();
			positionPlane = QVector3D::dotProduct(position - cameraCenter, cameraView);

			ratio = positionPlane * h / nearPlane / (float)m_size.height();
		}

		return ratio;
	}

	float ScreenCamera::viewAllLen(float r)
	{
		float len = r / (qSin(m_camera->fieldOfView() * 3.1415926f / 180.0f / 2.0f));
		return len;
	}

	void ScreenCamera::setMaxLimitDistance(float dmax)
	{
		m_maxDistance = dmax;
	}

	void ScreenCamera::setMinLimitDistance(float dmin)
	{
		m_minDistance = dmin;
	}

	bool ScreenCamera::zoom(float scale)
	{
		float factor = scale;

		float maxFovy = m_maxFovy;
		float minFovy = m_minFovy;
		float fovy = m_camera->fieldOfView();
		fovy *= factor;
		if (fovy >= minFovy && fovy <= maxFovy)
		{
			m_camera->setFieldOfView(fovy);
			if (m_camera->projectionType() == Qt3DRender::QCameraLens::OrthographicProjection)
			{
				float fnearPlane = m_camera->nearPlane();
				float ffarPlane = m_camera->farPlane();

				float r = (ffarPlane - fnearPlane) / 2 + fnearPlane;
				float flen = r * tan(m_camera->fieldOfView() * M_PI / 360) * 0.25;
				float top = flen;
				float bottom = -flen;

				float fratio = m_camera->aspectRatio();
				float left = -fratio * flen;
				float right = fratio * flen;

				m_camera->setTop(top);
				m_camera->setBottom(bottom);
				m_camera->setLeft(left);
				m_camera->setRight(right);
			}
			notifyCameraChanged();
			return true;
		}
		return false;
		//QVector3D cameraPosition = m_camera->position();
		//QVector3D cameraCenter = m_camera->viewCenter();
		//QVector3D cameraView = cameraCenter - cameraPosition;
		//float distance = cameraView.length();
		//float newDistance = factor * distance;
		//
		//cameraView.normalize();
		//if ((newDistance > m_minDistance) && (newDistance < m_maxDistance))
		//{
		//	QVector3D newPosition = cameraCenter - cameraView * newDistance;
		//	m_camera->setPosition(newPosition);
		//
		//	notifyCameraChanged();
		//
		//	_updateNearFar(m_box);
		//	return true;
		//}
		//return false;
	}

	bool ScreenCamera::translate(const QVector3D& trans)
	{
		QVector3D cameraPosition = m_camera->position();
		QVector3D viewCenter = m_camera->viewCenter();
		cameraPosition += trans;
		viewCenter += trans;

		m_camera->setPosition(cameraPosition);
		m_camera->setViewCenter(viewCenter);

		_updateNearFar(m_box);
		return true;
	}

	bool ScreenCamera::rotate(const QVector3D& axis, float angle)
	{
		QQuaternion q = QQuaternion::fromAxisAndAngle(axis, angle);
		return rotate(q);
	}

	bool ScreenCamera::rotate(const QQuaternion& q)
	{
		QVector3D up = m_camera->upVector();
		QVector3D viewCenter = m_camera->viewCenter();
		QVector3D position = m_camera->position();
		QVector3D dir = viewCenter - position;
		float distance = dir.length();

		dir.normalize();
		QVector3D newDir = q * dir;
		newDir.normalize();
		QVector3D newPosition = viewCenter - newDir * distance;

		QVector3D newUp = q * up;
		newUp.normalize();
		m_camera->setUpVector(newUp);
		m_camera->setPosition(newPosition);

		_updateNearFar(m_box);
		return true;
	}

	bool ScreenCamera::testCameraValid()
	{
		return (m_size.width() != 0) && (m_size.height() != 0);
	}

	QPoint ScreenCamera::flipY(const QPoint pos)
	{
		QPoint point(-1, -1);
		if (testCameraValid())
		{
			point.setX(pos.x());
			point.setY(qtuser_3d::adjustY(pos.y(), m_size));
		}
		return point;
	}

	QVector3D ScreenCamera::orignCenter() const
	{
		return m_orignCenter;
	}

	void ScreenCamera::home(const qtuser_3d::Box3D& box, int type)
	{
		QVector3D size = box.size();
		QVector3D center = box.center();
		center.setZ(0);

		QVector3D dir = QVector3D(1.0f, 1.0f, -1.0f);
		dir.normalize();
		QVector3D right = QVector3D(1.0f, -1.0f, 0.0f);
		right.normalize();
		QVector3D up = QVector3D::crossProduct(right, dir);

		if (type == 1)
		{
			dir = QVector3D(0.0f, 1.0f, -1.0f);
			dir.normalize();
			right = QVector3D(1.0f, 0.0f, 0.0f);
			right.normalize();
			up = QVector3D::crossProduct(right, dir);
		}

		float len = viewAllLen(size.length() / 2.0f);
		float dmax = len*1.5;
		QVector3D position = center - len * dir;
		m_camera->setViewCenter(center);
		m_camera->setUpVector(up);
		m_camera->setPosition(position);

		m_orignCenter = center;

		m_box = box;
		setMaxLimitDistance(dmax);

		_updateNearFar(m_box);
		notifyCameraChanged();
	}

    qtuser_3d::Box3D ScreenCamera::box()
    {
        return m_box;
    }

	QMatrix4x4 ScreenCamera::projectionMatrix() const
	{
		return m_camera->projectionMatrix();
	}

	QMatrix4x4 ScreenCamera::viewMatrix() const
	{
		return m_camera->viewMatrix();
	}

	void ScreenCamera::showSelf() const
	{
		if (m_camera != nullptr)
		{
			qDebug() << "show ScreenCamera:";
			qDebug() << m_camera->position() << " * " << m_camera->viewCenter() << " * " << m_camera->upVector();
		}
	}

	void ScreenCamera::viewFrom(const QVector3D& dir, const QVector3D& right, QVector3D* specificCenter)
	{
		QVector3D newUp = QVector3D::crossProduct(right, dir);
		newUp.normalize();

		QVector3D viewCenter = m_camera->viewCenter();
		if (specificCenter != NULL)
		{
			viewCenter = *specificCenter;
		}
		QVector3D position = m_camera->position();
		float d = (m_camera->viewCenter() - position).length();
		QVector3D newPosition = viewCenter - dir.normalized() * d;

		m_camera->setViewCenter(viewCenter);
		m_camera->setUpVector(newUp);
		m_camera->setPosition(newPosition);

		notifyCameraChanged();
	}

	QVector3D ScreenCamera::horizontal()
	{
		QVector3D viewCenter = m_camera->viewCenter();
		QVector3D position = m_camera->position();
		QVector3D dir = viewCenter - position;
		dir.normalize();
		QVector3D up = m_camera->upVector();

		QVector3D h = QVector3D::crossProduct(dir, up);
		h.setZ(0.0f);
		if (h.length() > 0.0f)
		{
			h.normalize();
		}
		if (h.length() == 0.0f)
		{
			qDebug() << "error";
		}
		return h;
	}

	QVector3D ScreenCamera::vertical()
	{
		QVector3D v = QVector3D(0.0f, 1.0f, 0.0f);
		QVector3D viewCenter = m_camera->viewCenter();
		QVector3D position = m_camera->position();
		QVector3D dir = viewCenter - position;
		float y = QVector3D::dotProduct(v, dir);
		float z = qSqrt(dir.lengthSquared() - y * y);
		QVector3D d(0.0f, y, z);
		if (d.lengthSquared() > 0.0f)
			v = d.normalized();
		return v;
	}

	float ScreenCamera::verticalAngle()
	{
		QVector3D v = QVector3D(0.0f, 0.0f, 1.0f);
		QVector3D viewCenter = m_camera->viewCenter();
		QVector3D position = m_camera->position();
		QVector3D dir = viewCenter - position;
		dir.normalize();
		float angle = acosf(QVector3D::dotProduct(v, dir));
		return angle;
	}

	float ScreenCamera::verticalAngle360()
	{
		QVector3D v = QVector3D(0.0f, 0.0f, 1.0f);
		QVector3D viewCenter = m_camera->viewCenter();
		QVector3D position = m_camera->position();
		QVector3D dir = viewCenter - position;
		dir.normalize();
		float angle = acosf(QVector3D::dotProduct(v, dir));

		QVector3D t = QVector3D::crossProduct(v, dir);
		QVector3D h = ScreenCamera::horizontal();

		float vv = QVector3D::dotProduct(h, t);

		float result = angle;

		if (vv > 0)
		{
			result = 2 * M_PI - angle;
		}
		return result;
	}

	void ScreenCamera::addCameraObserver(ScreenCameraObserver* observer)
	{
		if (observer)
		{
			m_cameraObservers.push_back(observer);
		}
	}

	void ScreenCamera::removeCameraObserver(ScreenCameraObserver* observer)
	{
		m_cameraObservers.removeOne(observer);
	}

	void ScreenCamera::clearCameraObservers()
	{
		m_cameraObservers.clear();
	}

	void ScreenCamera::notifyCameraChanged()
	{
		for (ScreenCameraObserver* observer : m_cameraObservers)
			observer->onCameraChanged(this);
	}

	bool cameraRayPoint(ScreenCamera* camera, QPoint point, QVector3D& planePosition, QVector3D& planeNormal, QVector3D& collide)
	{
		if (camera)
		{
			Ray ray = camera->screenRay(point);
			return lineCollidePlane(planePosition, planeNormal, ray, collide);
		}

		return false;
	}

	void ScreenCamera::setUpdateNearFarRuntime(bool update)
	{
		m_updateNearFarRuntime = update;
	}

	QPoint ScreenCamera::mapToScreen(const QVector3D& position)
	{
		const QMatrix4x4 vp = projectionMatrix() * viewMatrix();
		QVector4D clip = vp * QVector4D(position, 1.0);
		QVector3D ndc = QVector3D(clip) / clip.w();
		QVector2D uv(ndc);
		uv = uv * 0.5 + QVector2D( 0.5, 0.5);
		uv = QVector2D(uv.x(), 1.0 - uv.y());
		QSize screenSize = size();
		uv = uv * QVector2D(screenSize.width(), screenSize.height());
		QPoint screenPos(uv.x(), uv.y());
	
		return screenPos;
	}
}
