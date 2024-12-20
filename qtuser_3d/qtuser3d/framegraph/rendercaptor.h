#ifndef _QTUSER_3D_RENDERCAPTOR_1589166630506_H
#define _QTUSER_3D_RENDERCAPTOR_1589166630506_H
#include "qtuser3d/qtuser3dexport.h"
#include <Qt3DRender/QRenderTargetSelector>
#include <Qt3DRender/QRenderCapture>
#include <Qt3DRender/QClearBuffers>
#include <Qt3DRender/QFilterKey>
#include <Qt3DRender/QRenderPassFilter>
#include <Qt3DRender/QCameraSelector>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QTexture>
#include <QObject>

namespace qtuser_3d
{
	class QTUSER_3D_API RenderCaptor : public QObject
	{
		Q_OBJECT
	public:
		typedef std::function<void(QObject* receiver, const QImage& image)> ReceiverHandleReplyFunc;
		RenderCaptor(Qt3DCore::QNode* parent);
		~RenderCaptor();

		void setDisposable(bool enabled) { m_disposable = enabled; }
		void capture(Qt3DRender::QCamera* camera, QObject* receiver, ReceiverHandleReplyFunc func);
		void setEnabled(bool enabled);

	private:
		Qt3DRender::QCameraSelector* m_cameraSelector;
		Qt3DRender::QRenderCapture* m_renderCapture;
		QScopedPointer<Qt3DRender::QCamera> m_camera;
		QScopedPointer<Qt3DRender::QRenderCaptureReply> m_reply;
		ReceiverHandleReplyFunc m_callback;
		QObject* m_receiver;
		bool m_disposable { true };

	private slots:
		void onCaptureCompleted();
	};
}
#endif // _QTUSER_3D_RENDERCAPTOR_1589166630506_H
