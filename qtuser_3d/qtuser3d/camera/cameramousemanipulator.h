#ifndef _QTUSER_3D_CAMERAMOUSEMANIPULATOR_1590801985759_H
#define _QTUSER_3D_CAMERAMOUSEMANIPULATOR_1590801985759_H
#include "qtuser3d/qtuser3dexport.h"
#include <Qt3DRender/QCamera>
#include "qtuser3d/event/eventhandlers.h"
#include <QtGui/QVector3D>

namespace qtuser_3d
{
	class ScreenCamera;
	class QTUSER_3D_API CameraMouseManipulator: public QObject, public RightMouseEventHandler, public MidMouseEventHandler
	{
	public:
		CameraMouseManipulator(QObject* parent = nullptr);
		virtual ~CameraMouseManipulator();

		void setCamera(ScreenCamera* screenCamera);
		void setRotateCenter(const QVector3D& rotateCenter);

		void onRightMouseButtonPress(QMouseEvent* event) override;
		void onRightMouseButtonRelease(QMouseEvent* event) override;
		void onRightMouseButtonMove(QMouseEvent* event) override;
		void onRightMouseButtonClick(QMouseEvent* event) override;

		void onMidMouseButtonPress(QMouseEvent* event) override;
		void onMidMouseButtonRelease(QMouseEvent* event) override;
		void onMidMouseButtonMove(QMouseEvent* event) override;
		void onMidMouseButtonClick(QMouseEvent* event) override;

		virtual void setNeed360Rotate(bool is_need);

	protected:
		virtual void onCameraChanged();

		void restore();
	protected:
		ScreenCamera* m_screenCamera;
		Qt3DRender::QCamera* m_camera;

		bool m_revertY;
		QPoint m_savePoint;
		QVector3D m_saveUp;
		QVector3D m_saveCenter;
		QVector3D m_savePosition;

		QVector3D m_rotateCenter;
	};
}
#endif // _QTUSER_3D_CAMERAMOUSEMANIPULATOR_1590801985759_H
