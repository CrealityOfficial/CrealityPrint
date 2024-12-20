#ifndef _QTUSER_3D_CAPTURE_1589166630506_H
#define _QTUSER_3D_CAPTURE_1589166630506_H
#include "qtuser3d/qtuser3dexport.h"
#include <Qt3DRender/QRenderTargetSelector>
#include <Qt3DRender/QRenderSurfaceSelector>
#include <Qt3DRender/QRenderCapture>
#include <Qt3DRender/QClearBuffers>
#include <Qt3DRender/QFilterKey>
#include <Qt3DRender/QRenderPassFilter>
#include <Qt3DRender/QCameraSelector>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QTexture>
#include <Qt3DRender/QViewport>

namespace qtuser_3d
{
	class TextureRenderTarget;
	class QTUSER_3D_API Capture : public QObject
	{
		Q_OBJECT
	public:
		Capture(QObject* parent = nullptr);
		virtual ~Capture();

		void setClearColor(const QColor& color);
		void resize(const QSize& size);

		void requestCapture();
		void setFilterKey(const QString& name, int value);

		Qt3DCore::QEntity* rootEntity();
		Qt3DRender::QFrameGraphNode* frameGraphNode();
	public slots:
		void captureCompleted();
	protected:
		virtual void onCaptureCompleted();
	protected:
		Qt3DRender::QClearBuffers* m_clearBuffer;
		Qt3DRender::QRenderTargetSelector* m_renderTargetSelector;
		Qt3DRender::QRenderSurfaceSelector* m_renderSurfaceSelector;
		Qt3DRender::QRenderCapture* m_renderCapture;
		Qt3DRender::QRenderPassFilter* m_renderPassFilter;
		Qt3DRender::QFilterKey* m_filterKey;
		Qt3DRender::QCameraSelector* m_cameraSelector;
		Qt3DRender::QCamera* m_camera;
		Qt3DRender::QViewport* m_viewPort;
		TextureRenderTarget* m_textureRenderTarget;

		QScopedPointer<Qt3DRender::QRenderCaptureReply> m_captureReply;

		Qt3DCore::QEntity* m_rootEntity;
	};
}

#endif // _QTUSER_3D_CAPTURE_1589166630506_H
