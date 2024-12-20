#ifndef CXKERNEL_WORLDINDICATORGEOMETRY_H
#define CXKERNEL_WORLDINDICATORGEOMETRY_H

#include <QUrl>
#include <QPropertyAnimation>
#include <QPointer>

#include "shader_entity_export.h"
#include "qtuser3d/refactor/pxentity.h"
#include "qtuser3d/camera/screencamera.h"
#include "qtuser3d/camera/cameracontroller.h"

namespace qtuser_3d
{
	class CameraController;
	class Pickable;
}

namespace qtuser_3d
{
	class WorldIndicatorEntity;

	class SHADER_ENTITY_API WorldIndicatorTracer
	{
	public:
		virtual ~WorldIndicatorTracer() {}

		virtual qtuser_3d::Box3D resetCameraWithNewBox(WorldIndicatorEntity *entity) = 0;
	};


	class SHADER_ENTITY_API WorldIndicatorEntity : public qtuser_3d::PickXEntity, public qtuser_3d::ScreenCameraObserver
	{
		Q_OBJECT
			Q_PROPERTY(float lambda READ lambda WRITE setLambda NOTIFY lambdaChanged)
			Q_PROPERTY(float lambdax READ lambdax WRITE setLambdax NOTIFY lambdaxChanged)
	public:
		WorldIndicatorEntity(Qt3DCore::QNode* parent = nullptr);

		virtual ~WorldIndicatorEntity();

		//qtuser_3d::Pickable* pickable() const;

		void setCameraController(qtuser_3d::CameraController* cc);

		void setWorldIndicatorTracer(WorldIndicatorTracer * t);

		void setHighlightDirections(int dirs);

		void setSelectedDirections(int dirs);

		void setLength(float length);
		void setScreenPos(const QPoint& p);

		void resetCameraAngle(const int angle);
		void resetCameraAngle();
		void topCameraAngle();
		void bottomCameraAngle();
		void leftCameraAngle();
		void rightCameraAngle();
		void frontCameraAngle();
		void backCameraAngle();
		
		void setTheme(int theme);
		void setupLightTexture(const QUrl& url);
		void setupDarkTexture(const QUrl& url);

		void freshTextures();

		void resetHomeCamera(const QVector3D& homeDir, const QVector3D& homeUp, const QVector3D& homePosition, const QVector3D& homeViewCenter);

	signals:
		void lambdaChanged();
		void lambdaxChanged();

	protected:
		void onCameraChanged(qtuser_3d::ScreenCamera* camera) override;

		void setViewport(float x, float y, float w, float h);

	public Q_SLOTS:
		void aspectRatioChanged(float aspectRatio);

	private:

		void adaptCamera(int dirs);

		void setLambda(float lambda);
		float lambda() const;

		void setLambdax(float lambdax);
		float lambdax() const;

		void updateBasicTexture();

	private:
		
		//IndicatorPickable* m_pickable;
		QPointer<qtuser_3d::CameraController> m_cameraController;

		WorldIndicatorTracer* m_tracer;

		// for animate
		QSharedPointer<QPropertyAnimation> m_animation;
		float m_lambda;
		float m_lambdax;
		QVector3D m_startDir;
		QVector3D m_startUp;
		QVector3D m_endDir;
		QVector3D m_endUp;

		QPoint m_showOnPoint;
        QVector3D m_startCenter;
        QVector3D m_endCenter;

		float m_length = 100.0;//������ı߳�
		int m_theme;
		QUrl m_lightTextureUrl;
		QUrl m_darkTextureUrl;
		QUrl m_selectTextureUrl;

		QVector3D m_homePosition;
		QVector3D m_homeViewCenter;
	};
}
#endif // CXKERNEL_WORLDINDICATORGEOMETRY_H