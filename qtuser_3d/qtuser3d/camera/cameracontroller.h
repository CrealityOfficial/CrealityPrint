#ifndef _NULLSPACE_CAMERACONTROLLER_1588862127357_H
#define _NULLSPACE_CAMERACONTROLLER_1588862127357_H
#include "qtuser3d/qtuser3dexport.h"
#include "qtuser3d/event/eventhandlers.h"
#include "qtuser3d/math/box3d.h"
#include <QVector3D>

namespace qtuser_3d
{
	class ScreenCamera;
	class CameraMouseManipulator;
	class QTUSER_3D_API CameraController : public QObject
		, public qtuser_3d::WheelEventHandler
		, public qtuser_3d::RightMouseEventHandler
		, public qtuser_3d::MidMouseEventHandler
		, public qtuser_3d::ResizeEventHandler
		, public qtuser_3d::HoverEventHandler
	{
		Q_OBJECT
	public:
		CameraController(QObject* parent = nullptr);
		virtual ~CameraController();

		void useTrackableCamera();
		void useEularCamera();

		void setRotateCenter(const QVector3D& rotateCenter);
		QVector3D rotateCenter() const;
		void setScreenCamera(qtuser_3d::ScreenCamera* camera);
		qtuser_3d::ScreenCamera* screenCamera();

		void setMouseSensitivity(float sensitivity);
		void setNeed360Rotate(bool is_need);
		void setNeedAroundRotate(bool is_need);
		void setRotateSpeedDelta(float hDelta, float vDelta);

		void setEnableZoomAroundCursor(bool enable);

		Q_INVOKABLE void viewFromBottom(QVector3D* specificCenter = nullptr);
		Q_INVOKABLE void viewFromTop(QVector3D* specificCenter = nullptr);
		Q_INVOKABLE void viewFromLeft(QVector3D* specificCenter = nullptr);
		Q_INVOKABLE void viewFromRight(QVector3D* specificCenter = nullptr);
		Q_INVOKABLE void viewFromFront(QVector3D* specificCenter = nullptr);
		Q_INVOKABLE void viewFromBack(QVector3D* specificCenter = nullptr);
		Q_INVOKABLE void viewFromUserSetting(QVector3D posion, QVector3D viewCenter, QVector3D upVector, QVector3D* specificCenter = nullptr);

		void view(const QVector3D& dir, const QVector3D& right, QVector3D* specificCenter = nullptr);

		void viewEx(const QVector3D& newDir, const QVector3D& newUp, const QVector3D& homePosition, const QVector3D& homeViewCenter, const QVector3D& newCenter);

		// �л�������͸����ͼ
		Q_INVOKABLE void viewSwitch();

		Q_INVOKABLE void viewFromOrthographic();
		Q_INVOKABLE void viewFromPerspective();

		Q_INVOKABLE void showSelf();

		void sendViewUpdateSingle(bool capture);
		void fittingBoundingBox(const qtuser_3d::Box3D& box);
		void updateNearFar(const qtuser_3d::Box3D& box);
		void home(const qtuser_3d::Box3D& box, int type = 0);

		QVector3D getViewPosition() const;
		QVector3D getViewupVector() const;
		QVector3D getviewCenter() const;
		QVector3D getViewDir() const;

		void setViewPosition(const QVector3D position);
		void setViewupVector(const QVector3D upVector);
		void setviewCenter(const QVector3D viewCenter);

		void enableRotate(bool enabled);
		void enableScale(bool enabled);
		void enableTranslate(bool enabled);
	public:
		void onResize(const QSize& size) override;

		void onRightMouseButtonPress(QMouseEvent* event) override;
		void onRightMouseButtonRelease(QMouseEvent* event) override;
		void onRightMouseButtonMove(QMouseEvent* event) override;
		void onRightMouseButtonClick(QMouseEvent* event) override;

		void onMidMouseButtonPress(QMouseEvent* event) override;
		void onMidMouseButtonRelease(QMouseEvent* event) override;
		void onMidMouseButtonMove(QMouseEvent* event) override;
		void onMidMouseButtonClick(QMouseEvent* event) override;

		void onWheelEvent(QWheelEvent* event) override;

	public:
		void onHoverEnter(QHoverEvent* event) override;
		void onHoverLeave(QHoverEvent* event) override;
		void onHoverMove(QHoverEvent* event) override;

	signals:
		void signalViewChanged(bool capture);
		void signalCameraChaged(QVector3D position, const QVector3D upVector);
		void signalMidMousePressed(bool bPressed);
		void signalRightMousePressed(bool bPressed);
	protected:
		qtuser_3d::CameraMouseManipulator* m_cameraManipulator;
		qtuser_3d::ScreenCamera* m_screenCamera;
		QVector3D m_cameraPos;
		QPoint m_cursorPos = QPoint(-1, -1);
		unsigned m_mask;
		bool m_enableZoomAroundCursor;
	};
}
#endif // _NULLSPACE_CAMERACONTROLLER_1588862127357_H
