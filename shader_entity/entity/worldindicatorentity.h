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
	class IndicatorPickable;

	class SHADER_ENTITY_API WorldIndicatorEntity : public qtuser_3d::PickXEntity, public qtuser_3d::ScreenCameraObserver
	{
		Q_OBJECT
			Q_PROPERTY(float lambda READ lambda WRITE setLambda NOTIFY lambdaChanged)
	public:
		WorldIndicatorEntity(Qt3DCore::QNode* parent = nullptr);

		virtual ~WorldIndicatorEntity();

		//qtuser_3d::Pickable* pickable() const;

		void setCameraController(qtuser_3d::CameraController* cc);

		void setHighlightDirections(int dirs);

		void setSelectedDirections(int dirs);

		void setLength(float length);
		void setScreenPos(const QPoint& p);

		void resetCameraAngle(const int angle);
		void resetCameraAngle();

		void setTheme(int theme);
		void setupLightTexture(const QUrl& url);
		void setupDarkTexture(const QUrl& url);

		void freshTextures();

	signals:
		void lambdaChanged();

	protected:
		void onCameraChanged(qtuser_3d::ScreenCamera* camera) override;

		void setViewport(float x, float y, float w, float h);

	public Q_SLOTS:
		void aspectRatioChanged(float aspectRatio);

	private:

		void adaptCamera(int dirs);

		void setLambda(float lambda);
		float lambda() const;

		void updateBasicTexture();

	private:
		
		//IndicatorPickable* m_pickable;
		QPointer<qtuser_3d::CameraController> m_cameraController;

		// for animate
		QSharedPointer<QPropertyAnimation> m_animation;
		float m_lambda;
		QVector3D m_startDir;
		QVector3D m_startUp;
		QVector3D m_endDir;
		QVector3D m_endUp;

		QPoint m_showOnPoint;
        QVector3D m_startCenter;
        QVector3D m_endCenter;

		float m_length = 100.0;//正方体的边长
		int m_theme;
		QUrl m_lightTextureUrl;
		QUrl m_darkTextureUrl;
		QUrl m_selectTextureUrl;
	};
}
#endif // CXKERNEL_WORLDINDICATORGEOMETRY_H