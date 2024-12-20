#ifndef QTUSER_QML_QUICKSCENE3DWRAPPER_1691221434441_H
#define QTUSER_QML_QUICKSCENE3DWRAPPER_1691221434441_H
#include "qtuser3d/qtuser3dexport.h"
#include "qtuser3d/framegraph/rendergraph.h"
#include "qtuser3d/event/eventsubdivide.h"

#include <Qt3DCore/QEntity>
#include <Qt3DRender/QRenderSettings>
#include <Qt3DRender/QFrameGraphNode>

#include <QQuickItem>

class QOpenGLContext;

namespace qtuser_3d
{
	class RawOGL;
	class QTUSER_3D_API QuickScene3DWrapper : public QQuickItem
	{
		Q_OBJECT
	public:
		QuickScene3DWrapper(QQuickItem* parent = nullptr);
		virtual ~QuickScene3DWrapper();

		Q_INVOKABLE void bindScene3D(QObject* scene3d);
		
		void renderRenderGraph(qtuser_3d::RenderGraph* graph);
		void registerRenderGraph(qtuser_3d::RenderGraph* graph);
		void unRegisterRenderGraph(qtuser_3d::RenderGraph* graph);
		void unRegisterAll();

		bool isRenderRenderGraph(qtuser_3d::RenderGraph* graph);
		qtuser_3d::RenderGraph* currentRenderGraph();

		qtuser_3d::EventSubdivide* eventSubdivide();

		void setSharedContext(QOpenGLContext* context);
		QOpenGLContext* sharedContext();
		qtuser_3d::RawOGL* rawOGL();

		void setAlways(bool always);
		bool always();

	public slots:
		void requestUpdate();
		void sync();
		void cleanup();
	private slots:
		void handleWindowChanged(QQuickWindow* win);
		void releaseAlwaysRender();
	protected:
		Q_INVOKABLE void _geometry(int width, int height);
		virtual void geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry) Q_DECL_OVERRIDE;
		virtual void mousePressEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
		virtual void mouseMoveEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
		virtual void mouseReleaseEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
		virtual void wheelEvent(QWheelEvent* event) Q_DECL_OVERRIDE;
		virtual void hoverEnterEvent(QHoverEvent* event) Q_DECL_OVERRIDE;
		virtual void hoverMoveEvent(QHoverEvent* event) Q_DECL_OVERRIDE;
		virtual void hoverLeaveEvent(QHoverEvent* event) Q_DECL_OVERRIDE;
		//virtual void keyPressEvent(QKeyEvent* event) Q_DECL_OVERRIDE;
		//virtual void keyReleaseEvent(QKeyEvent* event) Q_DECL_OVERRIDE;
		virtual bool eventFilter(QObject *o, QEvent *e) Q_DECL_OVERRIDE;

	protected:
		QObject* m_scene3D;
		QQuickItem* m_topItem { NULL };

		QOpenGLContext* m_sharedContext;
		qtuser_3d::RawOGL* m_rawOGL;

		QSize m_size;
		qtuser_3d::EventSubdivide* m_eventSubdivide;
		float m_ratio{ 1 };
		bool isHovered{ false };

		Qt3DCore::QEntity* m_root;
		Qt3DRender::QRenderSettings* m_renderSettings;
		Qt3DRender::QFrameGraphNode* m_defaultFG;

		QList<qtuser_3d::RenderGraph*> m_registerRenderGraph;
		bool m_always;
		
#ifdef DEBUG
		bool m_forceAlways { false };
#endif

	};
}

#endif // QTUSER_QML_QUICKSCENE3DWRAPPER_1691221434441_H