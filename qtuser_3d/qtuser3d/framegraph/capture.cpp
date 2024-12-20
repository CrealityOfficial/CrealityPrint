#include "capture.h"
#include "qtuser3d/framegraph/texturerendertarget.h"
#include <QtGui/QOffscreenSurface>

namespace qtuser_3d
{
	Capture::Capture(QObject* parent)
		:QObject(parent)
	{
		m_clearBuffer = new Qt3DRender::QClearBuffers();
		m_clearBuffer->setClearColor(QColor(255, 0, 0, 255));
		m_clearBuffer->setBuffers(Qt3DRender::QClearBuffers::BufferType::ColorDepthBuffer);

		m_viewPort = new Qt3DRender::QViewport(m_clearBuffer);
		m_viewPort->setNormalizedRect(QRectF(0.0f, 0.0f, 1.0f, 1.0f));

		m_renderSurfaceSelector = new Qt3DRender::QRenderSurfaceSelector(m_viewPort);
		QOffscreenSurface* surface = new QOffscreenSurface();
		surface->create();
		m_renderSurfaceSelector->setSurface(surface);

		m_renderTargetSelector = new Qt3DRender::QRenderTargetSelector(m_renderSurfaceSelector);
		m_textureRenderTarget = new TextureRenderTarget();
		m_renderTargetSelector->setTarget(m_textureRenderTarget);

		m_cameraSelector = new Qt3DRender::QCameraSelector(m_renderTargetSelector);
		m_camera = new Qt3DRender::QCamera(m_cameraSelector);
		m_cameraSelector->setCamera(m_camera);

		m_renderCapture = new Qt3DRender::QRenderCapture(m_cameraSelector);
		m_renderPassFilter = new Qt3DRender::QRenderPassFilter(m_renderCapture);
		m_filterKey = new Qt3DRender::QFilterKey(m_renderPassFilter);
		m_renderPassFilter->addMatch(m_filterKey);

		m_rootEntity = new Qt3DCore::QEntity();
	}

	Capture::~Capture()
	{
		delete m_clearBuffer;
		delete m_rootEntity;
	}

	void Capture::resize(const QSize& size)
	{
		qDebug() << "Capture::resize " << size;
		m_textureRenderTarget->resize(size);
	}

	void Capture::requestCapture()
	{
		m_captureReply.reset(m_renderCapture->requestCapture());
		connect(m_captureReply.data(), &Qt3DRender::QRenderCaptureReply::completed, this, &Capture::captureCompleted);
	}

	void Capture::captureCompleted()
	{
		onCaptureCompleted();
		deleteLater();
	}

	void Capture::onCaptureCompleted()
	{
		QImage image = m_captureReply->image();
		m_captureReply.reset();

		image.save("xxxx.png");
	}

	void Capture::setFilterKey(const QString& name, int value)
	{
		qDebug() << "Capture::setFilterKey " << name << " = " << value;
		m_filterKey->setName(name);
		m_filterKey->setValue(value);
	}

	void Capture::setClearColor(const QColor& color)
	{
		qDebug() << "Capture::setClearColor " << color;
		m_clearBuffer->setClearColor(color);
	}

	Qt3DRender::QFrameGraphNode* Capture::frameGraphNode()
	{
		return m_clearBuffer;
	}

	Qt3DCore::QEntity* Capture::rootEntity()
	{
		return m_rootEntity;
	}
}
