#include "xrendergraph.h"

#include "qtuser3d/camera/cameracontroller.h"
#include "qtuser3d/camera/screencamera.h"

namespace qtuser_3d
{
	XRenderGraph::XRenderGraph(Qt3DCore::QNode* parent)
		: RenderGraph(parent)
	{
		m_surfaceSelector = new Qt3DRender::QRenderSurfaceSelector(this);
		m_offSurface = new QOffscreenSurface(nullptr);
		m_offSurface->create();
		m_surfaceSelector->setSurface(m_offSurface);

		m_viewPort = new Qt3DRender::QViewport(m_surfaceSelector);
		m_viewPort->setNormalizedRect(QRectF(0.0f, 0.0f, 1.0f, 1.0f));

		m_cameraSelector = new Qt3DRender::QCameraSelector(m_viewPort);

		m_clearBuffer = new Qt3DRender::QClearBuffers(m_cameraSelector);
		m_clearBuffer->setClearColor(QColor(60, 60, 60, 255));
		m_clearBuffer->setClearStencilValue(0x0);
		m_clearBuffer->setBuffers(Qt3DRender::QClearBuffers::BufferType::ColorDepthStencilBuffer);

		m_camera = new Qt3DRender::QCamera(this);
		m_cameraSelector->setCamera(m_camera);

		m_cameraController = new CameraController(this);
		m_screenCamera = new ScreenCamera(this, m_camera);
		m_cameraController->setScreenCamera(m_screenCamera);

		Qt3DRender::QRenderPassFilter* filter = new Qt3DRender::QRenderPassFilter(m_clearBuffer);
		Qt3DRender::QFilterKey* filterKey = new Qt3DRender::QFilterKey();
		filterKey->setName("view");
		filterKey->setValue(0);
		filter->addMatch(filterKey);
	}

	XRenderGraph::~XRenderGraph()
	{

	}

	void XRenderGraph::setClearColor(const QColor& color)
	{
		m_clearBuffer->setClearColor(color);
	}

	void XRenderGraph::setViewport(const QRectF& rect)
	{
		m_viewPort->setNormalizedRect(rect);
	}

	void XRenderGraph::updateSurfaceSize(const QSize& size)
	{
		m_surfaceSelector->setExternalRenderTargetSize(size);
	}

	void XRenderGraph::updateRenderSize(const QSize& size)
	{
		updateSurfaceSize(size);
		m_screenCamera->setScreenSize(size);
	}

	QSize XRenderGraph::surfaceSize()
	{
		return m_surfaceSelector->externalRenderTargetSize();
	}

	void XRenderGraph::onRightMouseButtonPress(QMouseEvent* event)
	{
		m_cameraController->onRightMouseButtonPress(event);
	}

	void XRenderGraph::onRightMouseButtonRelease(QMouseEvent* event)
	{
		m_cameraController->onRightMouseButtonRelease(event);
	}

	void XRenderGraph::onRightMouseButtonMove(QMouseEvent* event)
	{
		m_cameraController->onRightMouseButtonMove(event);
	}

	void XRenderGraph::onRightMouseButtonClick(QMouseEvent* event)
	{
	}

	void XRenderGraph::onLeftMouseButtonPress(QMouseEvent* event)
	{
	}

	void XRenderGraph::onLeftMouseButtonRelease(QMouseEvent* event)
	{
	}

	void XRenderGraph::onLeftMouseButtonMove(QMouseEvent* event)
	{
	}

	void XRenderGraph::onLeftMouseButtonClick(QMouseEvent* event)
	{
	}

	void XRenderGraph::onMidMouseButtonPress(QMouseEvent* event)
	{
		m_cameraController->onMidMouseButtonPress(event);
	}

	void XRenderGraph::onMidMouseButtonRelease(QMouseEvent* event)
	{
		m_cameraController->onMidMouseButtonRelease(event);
	}

	void XRenderGraph::onMidMouseButtonMove(QMouseEvent* event)
	{
		m_cameraController->onMidMouseButtonMove(event);
	}

	void XRenderGraph::onMidMouseButtonClick(QMouseEvent* event)
	{
	}

	void XRenderGraph::onWheelEvent(QWheelEvent* event)
	{
		m_cameraController->onWheelEvent(event);
	}

	void XRenderGraph::onResize(const QSize& size)
	{
	}

	void XRenderGraph::onHoverEnter(QHoverEvent* event)
	{
	}

	void XRenderGraph::onHoverLeave(QHoverEvent* event)
	{
	}

	void XRenderGraph::onHoverMove(QHoverEvent* event)
	{
		m_cameraController->onHoverMove(event);
	}

	void XRenderGraph::onKeyPress(QKeyEvent* event)
	{
	}

	void XRenderGraph::onKeyRelease(QKeyEvent* event)
	{
	}
}