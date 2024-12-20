#ifndef ROTATE_HELPER_ENTITY_T_H
#define ROTATE_HELPER_ENTITY_T_H
#include "shader_entity_export.h"
#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DExtras/QText2DEntity>
#include "qtuser3d/math/box3d.h"
#include "qtuser3d/event/eventhandlers.h"
#include "qtuser3d/refactor/pxentity.h"


namespace qtuser_3d
{
	class ManipulateEntity;
	class PieFadeEntity;
	class Pickable;
	class ManipulatePickable;
	class FacePicker;
	class ScreenCamera;
	class RotateCallback;

	enum class RotateHelperType
	{
		RH_NORMAL,
		RH_SIMPLE,
		RH_CUSTOM
	};

	class SHADER_ENTITY_API RotateHelperEntity_T : public XEntity
		,public LeftMouseEventHandler
	{
		Q_OBJECT
	public:
		RotateHelperEntity_T(Qt3DCore::QNode* parent = nullptr, RotateHelperType helperType = RotateHelperType::RH_NORMAL);
		virtual ~RotateHelperEntity_T();

		Pickable* getPickable();

		void setRotateAxis(QVector3D axis, double initAngle = 0);
		QVector3D getRotateAxis() { return m_rotateAxis; }

		double getLastestRotAngle() { return m_lastestRotAngles; }

		void setPickSource(FacePicker* pickSource);
		void setScreenCamera(ScreenCamera* camera);
		void setRotateCallback(RotateCallback* callback);

		void setScale(float scale);
		void setInitScale(float initScale);

		void setVisibility(bool visibility);
		void setHandlerVisibility(bool visibility);
		void setDialVisibility(bool visibility);
		void setColor(QVector4D v4);
		void setRingColor(QVector4D color);
		void setDialColor(QVector4D color);
		void setHandlerColor(QVector4D color);
		void setHandlerPickedColor(QVector4D color);

		QVector3D center();
		bool isRotating() { return m_rotatingFlag; }

		void onCameraChanged();
		void setAlwaysFaceCamera(bool faceCamera);

	public slots:
		void onBoxChanged(Box3D box);

	private:
		void initRing();
		void initHandler();
		void initDial(QVector3D scale);
		void initTip();
		void initManipulateSimpleRing();
		void resetSimpleRingRotateInitAngle();

	protected:
		void onLeftMouseButtonClick(QMouseEvent* event) override {}
		void onLeftMouseButtonPress(QMouseEvent* event) override;
		void onLeftMouseButtonRelease(QMouseEvent* event) override;
		void onLeftMouseButtonMove(QMouseEvent* event) override;

		QVector3D calculateSpacePoint(QPoint point);
		QQuaternion process(QPoint point);
		void perform(QPoint point, bool release);

	protected:
		RotateHelperType m_helperType;
		bool m_rotatingFlag; 
		double m_lastestRotAngles; // < 0: ˳ʱ�룬= 0��0��> 0����ʱ��
		double m_initRotateDirAngles;
		double m_initScaleRate;
		QVector3D m_rotateAxis;
		QVector3D m_originRotateAxis;
		QVector3D m_initRotateDir;
		QVector3D m_originInitRotateDir;
		QQuaternion m_initQuaternion;

		QVector3D m_scale;
		QVector3D m_center;
		QVector3D m_spacePoint;
		QVector3D m_simpleRingScale;

		double m_ringRadius;
		double m_ringMinorRadius;
		QVector4D m_ringColor;
		PieFadeEntity* m_pRingEntity;
		//ManipulatePickable* m_pRingPickable;

		double m_handlerOffset;
		QVector4D m_handlerColor;
		QVector4D m_handlerPickedColor;
		ManipulateEntity* m_pHandlerEntity;
		ManipulatePickable* m_pHandlerPickable;

		float m_camViewRingRadius;
		float m_camViewRingMinorRadius;
		QVector4D m_camViewRingColor;
		QVector4D m_camViewRingPickedColor;
		ManipulateEntity* m_manipulateRingEntity;
		bool m_alwaysFaceCamera;
		QVector3D m_camRight;

		// only apply to m_manipulateRingEntity,   to set difference from the dial entity
		Qt3DCore::QTransform* m_pManipulateRingTransform;
		QEntity* m_pManipulateRingGroup;

		double m_dialRadius;
		double m_degreeRadius;
		double m_markOffset;
		QVector4D m_dialColor;
		QVector4D m_degreeColor;
		PieFadeEntity* m_pDialEntity;
		XEntity* m_pDegreeEntity;
		//ManipulatePickable* m_pDialPickable;

		Qt3DExtras::QText2DEntity* m_pTipEntity;

		Qt3DCore::QTransform* m_pGlobalTransform;
		Qt3DCore::QTransform* m_pRotateTransform;
		Qt3DCore::QTransform* m_pNoRotateTransform;
		QEntity* m_pRotateGroup;
		QEntity* m_pNoRotateGroup;

		FacePicker* m_pPickSource;
		ScreenCamera* m_pScreenCamera;
		RotateCallback* m_pRotateCallback;
	};
}

#endif // ROTATE_HELPER_ENTITY_T_H