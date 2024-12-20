#include "qtuser3d/framegraph/surface.h"
#include "Qt3DRender/QFilterKey"
#include <QtCore/QDebug>

namespace qtuser_3d
{
	Surface::Surface(Qt3DCore::QNode* parent)
		:QRenderSurfaceSelector(parent)
	{
		m_offSurface = new QOffscreenSurface();
		m_offSurface->create();
		setSurface(m_offSurface);

		m_viewPort = new Qt3DRender::QViewport(this);
        m_viewPort->setNormalizedRect(QRectF(0.0f, 0.0f, 1.0f, 1.0f));
		
		m_cameraSelector = new Qt3DRender::QCameraSelector(m_viewPort);

		m_clearBuffer = new Qt3DRender::QClearBuffers(m_cameraSelector);
		m_clearBuffer->setClearColor(QColor(120, 120, 120, 255));
		m_clearBuffer->setClearStencilValue(0x0);
		m_clearBuffer->setBuffers(Qt3DRender::QClearBuffers::BufferType::ColorDepthStencilBuffer);

		m_renderPassFilter = new Qt3DRender::QRenderPassFilter(m_clearBuffer);
		Qt3DRender::QFilterKey* filterKey = new Qt3DRender::QFilterKey();
		filterKey->setName("view");
		filterKey->setValue(0);
		m_renderPassFilter->addMatch(filterKey);

		m_alphaPassFilter = new Qt3DRender::QRenderPassFilter(m_cameraSelector);
		Qt3DRender::QFilterKey* alphaKey = new Qt3DRender::QFilterKey();
		alphaKey->setName("alpha");
		alphaKey->setValue(0);
		m_alphaPassFilter->addMatch(alphaKey);

		
		m_alphaPassFilter2 = new Qt3DRender::QRenderPassFilter(m_cameraSelector);
		Qt3DRender::QFilterKey* alphaKey2 = new Qt3DRender::QFilterKey();
		alphaKey2->setName("alpha2nd");
		alphaKey2->setValue(0);
		m_alphaPassFilter2->addMatch(alphaKey2);


		{
			Qt3DRender::QClearBuffers *clearBuffer = new Qt3DRender::QClearBuffers(m_cameraSelector);
			clearBuffer->setClearStencilValue(0x0);
			clearBuffer->setBuffers(Qt3DRender::QClearBuffers::BufferType::DepthStencilBuffer);

			Qt3DRender::QRenderPassFilter *renderPassFilter = new Qt3DRender::QRenderPassFilter(clearBuffer);
			Qt3DRender::QFilterKey* filterKey = new Qt3DRender::QFilterKey();
			filterKey->setName("overlay");
			filterKey->setValue(0);
			renderPassFilter->addMatch(filterKey);

#ifdef ENABLE_DEBUG_OVERLAY
			m_debugOverlay = new Qt3DRender::QDebugOverlay(renderPassFilter);
			m_debugOverlay->setEnabled(false);
#endif
		}
	}

	Surface::~Surface()
	{
		//m_surfaceSelector->setSurface(nullptr);
		setSurface(nullptr);
#ifndef Q515
		delete m_offSurface;
#endif
	}

	Qt3DRender::QFrameGraphNode* Surface::getCameraViewportFrameGraphNode()
	{
		return m_cameraSelector;
	}

	void Surface::setClearColor(const QColor& color)
	{
		//qDebug() << "Surface setClearColor " << color;
		m_clearBuffer->setClearColor(color);
	}

	void Surface::setViewport(const QRectF& rect)
	{
        //qDebug() << "Surface viewport " << rect;
		m_viewPort->setNormalizedRect(rect);
	}

	void Surface::setCamera(Qt3DRender::QCamera* camera)
	{
		m_cameraSelector->setCamera(camera);
	}

	void Surface::updateSurfaceSize(const QSize& size)
	{
        //qDebug() << "Surface Set External Target Size : " << size;
		setExternalRenderTargetSize(size);
	}

#ifdef ENABLE_DEBUG_OVERLAY
	bool Surface::showDebugOverlay()
	{
		return m_debugOverlay->isEnabled();
	}

	void Surface::setShowDebugOverlay(bool showDebugOverlay)
	{
		m_debugOverlay->setEnabled(showDebugOverlay);
	}
#endif
}
