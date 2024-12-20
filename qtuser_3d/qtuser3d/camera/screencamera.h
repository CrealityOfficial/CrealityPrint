#ifndef _QTUSER_3D_SCREENCAMERA_1588839492087_H
#define _QTUSER_3D_SCREENCAMERA_1588839492087_H
#include "qtuser3d/qtuser3dexport.h"
#include "qtuser3d/math/box3d.h"
#include "qtuser3d/math/ray.h"
#include <Qt3DRender/QCamera>

namespace qtuser_3d
{
	class ScreenCamera;
	class ScreenCameraObserver
	{
	public:
		virtual ~ScreenCameraObserver() {}

		virtual void onCameraChanged(ScreenCamera* camera) = 0;
	};

	class QTUSER_3D_API ScreenCamera: public QObject
	{
	public:
		ScreenCamera(QObject* parent = nullptr, Qt3DRender::QCamera* camera = nullptr);
		virtual ~ScreenCamera();

		Qt3DRender::QCamera* camera();
		void setScreenSize(const QSize& size);
		QSize size();

		void setFovyLimit(float maxFovy = 50.0f, float minFovy = 0.1f);

		void fittingBoundingBox(const qtuser_3d::Box3D& box);

		// for halotBox
		void fittingBoundingBoxEx(const qtuser_3d::Box3D& box, const QVector3D& homeDir, const QVector3D& homeUp, const QVector3D& homePosition, const QVector3D& homeViewCenter);
		void viewFromEx(const QVector3D& newDir, const QVector3D& newUp, const QVector3D& homePosition, const QVector3D& homeViewCenter, const QVector3D& newCenter);

		void updateNearFar(const qtuser_3d::Box3D& box);
		void updateNearFar();

		bool checkUpState();

		qtuser_3d::Ray screenRay(const QPoint& point);
		qtuser_3d::Ray screenRay(const QPointF& point);
		qtuser_3d::Ray screenRayOrthographic(const QPointF& point);
		float screenSpaceRatio(const QVector3D& position);
		float viewAllLen(float r);

		void setMaxLimitDistance(float dmax);
		void setMinLimitDistance(float dmin);

		bool zoom(float scale);
		bool translate(const QVector3D& trans);
		bool rotate(const QVector3D& axis, float angle);
		bool rotate(const QQuaternion& q);

        void home(const qtuser_3d::Box3D& box, int type = 0);
        qtuser_3d::Box3D box();

		QMatrix4x4 projectionMatrix() const;
		QMatrix4x4 viewMatrix() const;

		void showSelf() const;

		bool testCameraValid();
		QPoint flipY(const QPoint pos);

		QVector3D orignCenter() const;

		void viewFrom(const QVector3D& dir, const QVector3D& right, QVector3D* specificCenter);

		QVector3D horizontal();
		QVector3D vertical();
		float verticalAngle();
		float verticalAngle360();     // scope [0, 360)

		void addCameraObserver(ScreenCameraObserver* observer);
		void removeCameraObserver(ScreenCameraObserver* observer);
		void clearCameraObservers();
		void notifyCameraChanged();

		void setUpdateNearFarRuntime(bool update);

		QPoint mapToScreen(const QVector3D& position);
		
	protected:
		void _updateNearFar(const qtuser_3d::Box3D& box);
	protected:
		Qt3DRender::QCamera* m_camera;
		bool m_externalCamera;

		QSize m_size;

		float m_minDistance;
		float m_maxDistance;
		float m_maxFovy;
		float m_minFovy;

		qtuser_3d::Box3D m_box;

		QVector3D m_orignCenter;

		QList<ScreenCameraObserver*> m_cameraObservers;
		
		bool m_updateNearFarRuntime;
	};

	QTUSER_3D_API bool cameraRayPoint(ScreenCamera* camera, QPoint point, QVector3D& planePosition, QVector3D& planeNormal, QVector3D& collide);
}
#endif // _QTUSER_3D_SCREENCAMERA_1588839492087_H
