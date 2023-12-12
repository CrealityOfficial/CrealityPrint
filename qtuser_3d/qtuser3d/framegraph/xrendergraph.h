#ifndef QTUSER_3D_XRENDERGRAPH_1679973441799_H
#define QTUSER_3D_XRENDERGRAPH_1679973441799_H
#include "qtuser3d/qtuser3dexport.h"
#include "qtuser3d/framegraph/rendergraph.h"
#include "qtuser3d/event/eventhandlers.h"
#include <Qt3DRender/QClearBuffers>
#include <Qt3DRender/QViewport>
#include <Qt3DRender/QCameraSelector>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QRenderPassFilter>
#include <Qt3DRender/QSortPolicy>
#include <Qt3DRender/QRenderSurfaceSelector>
#include <Qt3DRender/QFilterKey>
#include <QtGui/QOffscreenSurface>

namespace qtuser_3d
{
	class ScreenCamera;
	class CameraController;
	class QTUSER_3D_API XRenderGraph : public qtuser_3d::RenderGraph
		, public qtuser_3d::WheelEventHandler
		, public qtuser_3d::RightMouseEventHandler
		, public qtuser_3d::MidMouseEventHandler
		, public qtuser_3d::LeftMouseEventHandler
		, public qtuser_3d::ResizeEventHandler
		, public qtuser_3d::HoverEventHandler
		, public qtuser_3d::KeyEventHandler
	{
		Q_OBJECT
	public:
		XRenderGraph(Qt3DCore::QNode* parent = nullptr);
		virtual ~XRenderGraph();

		void setClearColor(const QColor& color);
		void setViewport(const QRectF& rect);
		void updateSurfaceSize(const QSize& size);
	protected:
		void updateRenderSize(const QSize& size) override;
		QSize surfaceSize();

		void onRightMouseButtonPress(QMouseEvent* event) override;
		void onRightMouseButtonRelease(QMouseEvent* event) override;
		void onRightMouseButtonMove(QMouseEvent* event) override;
		void onRightMouseButtonClick(QMouseEvent* event) override;

		void onMidMouseButtonPress(QMouseEvent* event) override;
		void onMidMouseButtonRelease(QMouseEvent* event) override;
		void onMidMouseButtonMove(QMouseEvent* event) override;
		void onMidMouseButtonClick(QMouseEvent* event) override;

		void onLeftMouseButtonPress(QMouseEvent* event) override;
		void onLeftMouseButtonRelease(QMouseEvent* event) override;
		void onLeftMouseButtonMove(QMouseEvent* event) override;
		void onLeftMouseButtonClick(QMouseEvent* event) override;

		void onWheelEvent(QWheelEvent* event) override;
		void onResize(const QSize& size) override;

		void onHoverEnter(QHoverEvent* event) override;
		void onHoverLeave(QHoverEvent* event) override;
		void onHoverMove(QHoverEvent* event) override;

		void onKeyPress(QKeyEvent* event) override;
		void onKeyRelease(QKeyEvent* event) override;
	protected:
		Qt3DRender::QRenderSurfaceSelector* m_surfaceSelector;
		QOffscreenSurface* m_offSurface;
		Qt3DRender::QViewport* m_viewPort;
		Qt3DRender::QCameraSelector* m_cameraSelector;
		Qt3DRender::QClearBuffers* m_clearBuffer;
		Qt3DRender::QCamera* m_camera;

		CameraController* m_cameraController;
		ScreenCamera* m_screenCamera;
	};
}

#endif // QTUSER_3D_XRENDERGRAPH_1679973441799_H