#ifndef _QTUSER_3D_COLORPICKER_1589166630506_H
#define _QTUSER_3D_COLORPICKER_1589166630506_H
#include "qtuser3d/qtuser3dexport.h"
#include <Qt3DRender/QRenderTargetSelector>
#include <Qt3DRender/QRenderCapture>
#include <Qt3DRender/QClearBuffers>
#include <Qt3DRender/QFilterKey>
#include <Qt3DRender/QRenderPassFilter>
#include <Qt3DRender/QCameraSelector>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QTexture>

#include "qtuser3d/module/facepicker.h"
#include <QtCore/QTimer>
#include "rendercaptor.h"

namespace qtuser_3d
{
	class ColorPicker;
	typedef std::function<void(ColorPicker*)> selfPickerFunc;
	typedef std::function<void(QImage& image)> requestCallFunc;
	typedef std::function<void(const QImage& image)> namedReplyFunc;

	class QTUSER_3D_API NamedReply : public QObject
	{
	public:
		NamedReply(namedReplyFunc func, QObject* parent = nullptr);
		virtual ~NamedReply();

		void invoke();

		namedReplyFunc callback;
		QScopedPointer<Qt3DRender::QRenderCaptureReply> reply;
	};

	class TextureRenderTarget;
	class QTUSER_3D_API ColorPicker : public Qt3DRender::QFrameGraphNode
		, public FacePicker
	{
		Q_OBJECT
	signals:
		void frameCaptureComplete();
	public:
		ColorPicker(Qt3DCore::QNode* parent = nullptr);
		virtual ~ColorPicker();

		void resize(const QSize& size);

		void requestCapture();
		RenderCaptor* requestCapture(Qt3DRender::QCamera* camera, QObject* receiver, RenderCaptor::ReceiverHandleReplyFunc func);
		void requestSyncCapture(int timeout = 500);
		void requestNamedCapture(namedReplyFunc callback);

		void setFilterKey(const QString& name, int value);
		void setClearColor(const QColor& color);

		void setAllFilterKey(const QString& name, int value);

		bool isPrepared();
		bool pick(int x, int y, int* faceID) override;
		bool pick(const QPoint& point, int* faceID) override;
		void sourceMayChanged() override;

		void setUseDelay(bool delay);

		void setPickerFunc(selfPickerFunc func);
		void setRequestCallback(requestCallFunc func);

		void useSelfCameraSelector(bool use);
		Qt3DRender::QCamera* camera();

		void useSelfCamera();
		void useCamera(Qt3DRender::QCamera* cam);

		void setDebugName(const QString& name);

		bool getImageFinished();

		Qt3DRender::QTexture2D* colorTexture();
		void setTextureRenderTarget(TextureRenderTarget* textureRenderTarget);
		void useSelfTarget();

		void use();
		
		float renderTargetRatio();

	public slots:
		void captureCompleted();
		void namedCaptureCompleted();
		void delayCapture();
		void unUse();
	signals:
		void signalUpdate();
		void updateColorPickerFinished();

	protected:
		Qt3DRender::QClearBuffers* m_clearBuffer;
		Qt3DRender::QRenderTargetSelector* m_renderTargetSelector;
		Qt3DRender::QRenderCapture* m_renderCapture;
		Qt3DRender::QRenderPassFilter* m_renderPassFilter;
		Qt3DRender::QFilterKey* m_filterKey;
		Qt3DRender::QRenderPassFilter* m_renderPassFilter2;
		Qt3DRender::QFilterKey* m_filterKey2;

		Qt3DRender::QCameraSelector* m_cameraSelector;
		Qt3DRender::QCameraSelector* m_cameraSelector2;
		Qt3DRender::QCamera* m_camera;

		TextureRenderTarget* m_textureRenderTarget;

		Qt3DRender::QRenderCaptureReply* m_captureReply;
		QImage m_colorPickerImage;

		QTimer* m_updateTimer;
		QTimer* m_delayTimer;
		bool    m_useDelay;

		QTimer m_unUseTimer;

		bool m_createImageFinished;
		bool m_capturing;
		
#ifdef Q_OS_OSX
		const float m_renderTargetRatio{ 0.5f };
#else
		const float m_renderTargetRatio{ 1.0f };
#endif

		selfPickerFunc m_pickerFunc;
		requestCallFunc m_requestCallback;
#ifdef _DEBUG
		QString m_debugName;
	public:
#endif

		QList<NamedReply*> m_namedReplies;
	};
}
#endif // _QTUSER_3D_COLORPICKER_1589166630506_H
