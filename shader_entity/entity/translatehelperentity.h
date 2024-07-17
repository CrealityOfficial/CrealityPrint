#ifndef TRANSLATE_HELPER_ENTITY_H
#define TRANSLATE_HELPER_ENTITY_H

#include "qtuser3d/math/box3d.h"
#include "shader_entity_export.h"
#include "qtuser3d/refactor/xentity.h"
#include <Qt3DCore/QTransform>
#include <Qt3DCore/QEntity>
#include "qtuser3d/camera/screencamera.h"

#define HELPERTYPE_AXIS_X 1
#define HELPERTYPE_AXIS_Y 2
#define HELPERTYPE_AXIS_Z 4
#define HELPERTYPE_PLANE_XY 8
#define HELPERTYPE_PLANE_YZ 16
#define HELPERTYPE_PLANE_ZX 32
#define HELPERTYPE_CUBE_XYZ 64
#define HELPERTYPE_AXIS_ALL 7
#define HELPERTYPE_PLANE_ALL 63
#define HELPERTYPE__ALL 127


namespace qtuser_3d
{
	class Pickable;
	class SimplePickable;
	class ManipulateEntity;
	class CameraController;
	class SHADER_ENTITY_API TranslateHelperEntity : public XEntity , public qtuser_3d::ScreenCameraObserver
	{
		Q_OBJECT
	public:
		enum IndicatorType
		{
			ARROW,
			CUBE
		};
	public:
		TranslateHelperEntity(Qt3DCore::QNode* parent = nullptr, int type = 7, IndicatorType shapetype = ARROW);
		virtual ~TranslateHelperEntity();

		void setCameraController(CameraController* controller);
		void attach();
		void detach();

		Pickable* xArrowPickable();
		Pickable* yArrowPickable();
		Pickable* zArrowPickable();
		Pickable* xyPlanePickable();
		Pickable* yzPlanePickable();
		Pickable* zxPlanePickable();
		Pickable* xyzCubePickable();

		void setXVisibility(bool visibility);
		void setYVisibility(bool visibility);
		void setZVisibility(bool visibility);

		void setRotation(QQuaternion rotQ);
		Qt3DCore::QTransform* getTransform() { return m_transform; }

		void updateBox(const Box3D& box);
		QVector3D center();
		void setDirColor(QVector4D v4, int nDir);
		void setZArrowEntityPickable(bool enablePick);

	protected:
		void initAxis(int helperType, IndicatorType shapeType);
		void initPlane(int helperType);
		void initCenterCube(int helperType);

		void onCameraChanged(ScreenCamera* camera) override;

		void adaptHelperEntityDir(QVector3D dir);
		void adaptScale();
	protected:
		Qt3DCore::QTransform* m_transform;

		double m_scale;
		QVector3D m_center;
		QVector3D m_initAxisDir;
		QQuaternion m_initRotationQ;
		QQuaternion m_initXYPlaneQ;
		QQuaternion m_initYZPlaneQ;
		QQuaternion m_initZXPlaneQ;

		ManipulateEntity* m_pXArrowEntity;
		ManipulateEntity* m_pYArrowEntity;
		ManipulateEntity* m_pZArrowEntity;
		SimplePickable* m_pXArrowPickable;
		SimplePickable* m_pYArrowPickable;
		SimplePickable* m_pZArrowPickable;

		ManipulateEntity* m_pXYPlaneEntity;
		ManipulateEntity* m_pYZPlaneEntity;
		ManipulateEntity* m_pZXPlaneEntity;
		SimplePickable* m_pXYPlanePickable;
		SimplePickable* m_pYZPlanePickable;
		SimplePickable* m_pZXPlanePickable;

		ManipulateEntity* m_pXYZCubeEntity;
		SimplePickable* m_pXYZCubePickable;

		CameraController* m_pCameraController;
	};
}

#endif // TRANSLATE_HELPER_ENTITY_H