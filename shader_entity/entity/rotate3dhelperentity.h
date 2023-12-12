#ifndef ROTATE3D_HELPER_ENTITY_H
#define ROTATE3D_HELPER_ENTITY_H

#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>

#include "shader_entity_export.h"
#include "qtuser3d/math/box3d.h"
#include "qtuser3d/event/eventhandlers.h"
#include "qtuser3d/module/manipulatecallback.h"
#include "qtuser3d/refactor/xentity.h"


namespace qtuser_3d
{
	class RotateHelperEntity_T;
	class Pickable;
	class FacePicker;
	class ScreenCamera;
	class RotateCallback;

	class SHADER_ENTITY_API Rotate3DHelperEntity : public XEntity,
		public RotateCallback
	{
		Q_OBJECT
	public:
		Rotate3DHelperEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~Rotate3DHelperEntity();

		QList<Pickable*> getPickables();
		QList<LeftMouseEventHandler*> getLeftMouseEventHandlers();

		void setPickSource(FacePicker* pickSource);
		void setScreenCamera(ScreenCamera* camera);
		void setRotateCallback(RotateCallback* callback);

		inline void setXVisibility(bool visibility);
		inline void setYVisibility(bool visibility);
		inline void setZVisibility(bool visibility);
		
		void setScale(float scaleRate);
		void setInitScale(float initScale);

		QVector3D getCurrentRotateAxis();
		double getCurrentRotAngle();
		
		Pickable* xPickable();
		Pickable* yPickable();
		Pickable* zPickable();

	public slots:
		void onBoxChanged(Box3D box);

	protected:
		bool shouldStartRotate();
		void onStartRotate() override;
		void onRotate(QQuaternion q) override;
		void onEndRotate(QQuaternion q) override;
		void setRotateAngle(QVector3D axis, float angle) override;

	private:
		RotateHelperEntity_T* m_pXRotHelper;
		RotateHelperEntity_T* m_pYRotHelper;
		RotateHelperEntity_T* m_pZRotHelper;

		RotateCallback* m_pRotateCallback;
	};
}

#endif // ROTATE3D_HELPER_ENTITY_H