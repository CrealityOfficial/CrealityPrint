#ifndef ROTATE3D_HELPER_ENTITY_H
#define ROTATE3D_HELPER_ENTITY_H

#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>

#include "shader_entity_export.h"
#include "qtuser3d/math/box3d.h"
#include "qtuser3d/event/eventhandlers.h"
#include "qtuser3d/module/manipulatecallback.h"
#include "qtuser3d/refactor/xentity.h"
#include "entity/rotatehelperentity_T.h"
#include "qtuser3d/camera/screencamera.h"

namespace qtuser_3d
{
	class RotateHelperEntity_T;
	class Pickable;
	class FacePicker;
	class ScreenCamera;
	class RotateCallback;
	class CameraController;

	class SHADER_ENTITY_API Rotate3DHelperEntity : public XEntity,
		public RotateCallback, public qtuser_3d::ScreenCameraObserver
	{
		Q_OBJECT
	public:
		Rotate3DHelperEntity(Qt3DCore::QNode* parent = nullptr, RotateHelperType rotateHelperType = RotateHelperType::RH_NORMAL);
		virtual ~Rotate3DHelperEntity();

		QList<Pickable*> getPickables();
		QList<LeftMouseEventHandler*> getLeftMouseEventHandlers();

		void setPickSource(FacePicker* pickSource);
		void setScreenCamera(ScreenCamera* camera);
		void setRotateCallback(RotateCallback* callback);

		void setXVisibility(bool visibility);
		void setYVisibility(bool visibility);
		void setZVisibility(bool visibility);

		QVector3D getCurrentRotateAxis();
		double getCurrentRotAngle();
		
		Pickable* xPickable();
		Pickable* yPickable();
		Pickable* zPickable();
		Pickable* camViewPickable();
		Pickable* customPickable();

		void setCustomRotateAxis(QVector3D axis);

		void setCameraController(CameraController* controller);
		void attach();
		void detach();

	public slots:
		void onBoxChanged(Box3D box);
		//void slotCameraChanged(QVector3D position, const QVector3D upVector);

	protected:
		bool shouldStartRotate();
		void onStartRotate() override;
		void onRotate(QQuaternion q) override;
		void onEndRotate(QQuaternion q) override;
		void setRotateAngle(QVector3D axis, float angle) override;

		void onCameraChanged(ScreenCamera* camera) override;

		void setScale(float scaleRate);
		void adaptScale();
	private:
		RotateHelperEntity_T* m_pXRotHelper;
		RotateHelperEntity_T* m_pYRotHelper;
		RotateHelperEntity_T* m_pZRotHelper;
		RotateHelperEntity_T* m_pCamViewRotHelper;
		RotateHelperEntity_T* m_pCustomRotHelper;

		RotateCallback* m_pRotateCallback;
		CameraController* m_pCameraController;

		QVector3D m_center;
	};
}

#endif // ROTATE3D_HELPER_ENTITY_H