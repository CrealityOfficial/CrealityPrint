#include "quickscene3dwrapper.h"
#include <QOpenGLContext>
#include <QtCore/QThread>

#include "qtuser3d/framegraph/xrendergraph.h"
#include "qtuser3d/module/glcompatibility.h"
#include "qtuser3d/module/rawogl.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QDebug>
#include <QPointF>
#include <QQuickWindow>

#include "qtuser3d/macro.h"
#include <Qt3DInput/QInputSettings>
#include <QTimer>

namespace qtuser_3d
{
	QTUSER_3D_REG(QuickScene3DWrapper)
	QuickScene3DWrapper::QuickScene3DWrapper(QQuickItem* parent)
		: QQuickItem(parent)
		, m_scene3D(nullptr)
		, m_always(false)
		, m_sharedContext(nullptr)
	{
		connect(this, &QQuickItem::windowChanged, this, &QuickScene3DWrapper::handleWindowChanged);
		m_root = new Qt3DCore::QEntity();
		m_renderSettings = new Qt3DRender::QRenderSettings(m_root);
		m_renderSettings->setRenderPolicy(Qt3DRender::QRenderSettings::OnDemand);
		m_root->addComponent(m_renderSettings);

		m_root->addComponent(new Qt3DInput::QInputSettings(m_root));

		m_defaultFG = new Qt3DRender::QFrameGraphNode(m_root);
		m_renderSettings->setActiveFrameGraph(m_defaultFG);

		m_eventSubdivide = new qtuser_3d::EventSubdivide(this);

		m_rawOGL = new qtuser_3d::RawOGL(this);
		qDebug() << "QuickScene3DWrapper -->" << QThread::currentThread();

		setAcceptHoverEvents(true);
		setAcceptedMouseButtons(Qt::AllButtons);
		forceActiveFocus(Qt::MouseFocusReason);

#ifdef DEBUG
		installEventFilter(this);
#endif

	}

	QuickScene3DWrapper::~QuickScene3DWrapper()
	{
		m_eventSubdivide->closeHandlers();
	}
	/*
	void QuickScene3DWrapper::setRatio(float ratio)
	{
		m_ratio = ratio;
	}*/

	void QuickScene3DWrapper::bindScene3D(QObject* scene3d)
	{
		m_scene3D = scene3d;
		if (m_scene3D)
		{
			QMetaObject::invokeMethod(m_scene3D, "setEntity", Qt::AutoConnection, Q_ARG(Qt3DCore::QEntity*, m_root));
			QQuickWindow* w = window();
			if(w)
				w->installEventFilter(this);
		}

		QTimer::singleShot(2000, this, &QuickScene3DWrapper::releaseAlwaysRender);
	}

	void QuickScene3DWrapper::geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry)
	{
		QQuickItem::geometryChanged(newGeometry, oldGeometry);

		QSize size = newGeometry.size().toSize();
		m_size = size;
		if (m_ratio < 0.01)
		{
			m_ratio = 1;
		}
		size *= m_ratio;
		m_eventSubdivide->geometryChanged(size);

		qtuser_3d::RenderGraph* graph = currentRenderGraph();
		if (graph)
			graph->updateRenderSize(size);

		//requestUpdate();

		setAlways(true);
	}

	void QuickScene3DWrapper::mousePressEvent(QMouseEvent* event)
	{
		QPoint pt = event->pos();
		pt.rx() *= m_ratio;
		pt.ry() *= m_ratio;
		event->setLocalPos(pt);
		m_eventSubdivide->mousePressEvent(event);
		setFocus(true);
	}

	void QuickScene3DWrapper::mouseMoveEvent(QMouseEvent* event)
	{
		QPoint pt = event->pos();
		pt.rx() *= m_ratio;
		pt.ry() *= m_ratio;
		event->setLocalPos(pt);
		m_eventSubdivide->mouseMoveEvent(event);
	}

	void QuickScene3DWrapper::mouseReleaseEvent(QMouseEvent* event)
	{
		QPoint pt = event->pos();
		pt.rx() *= m_ratio;
		pt.ry() *= m_ratio;
		event->setLocalPos(pt);
		m_eventSubdivide->mouseReleaseEvent(event);
	}

	void QuickScene3DWrapper::wheelEvent(QWheelEvent* event)
	{
		QWheelEvent new_event(event->posF() * m_ratio, event->globalPosF(), event->delta() * m_ratio, event->buttons(), event->modifiers(), event->orientation());
		m_eventSubdivide->wheelEvent(&new_event);
	}

	void QuickScene3DWrapper::hoverEnterEvent(QHoverEvent* event)
	{
		QHoverEvent new_event(event->type(), event->posF() * m_ratio, event->oldPosF() * m_ratio, event->modifiers());
		m_eventSubdivide->hoverEnterEvent(&new_event);
	}

	void QuickScene3DWrapper::hoverMoveEvent(QHoverEvent* event)
	{
		QHoverEvent new_event(event->type(), event->posF() * m_ratio, event->oldPosF() * m_ratio, event->modifiers());
		m_eventSubdivide->hoverMoveEvent(&new_event);
	}

	void QuickScene3DWrapper::hoverLeaveEvent(QHoverEvent* event)
	{
		QHoverEvent new_event(event->type(), event->posF() * m_ratio, event->oldPosF() * m_ratio, event->modifiers());
		m_eventSubdivide->hoverLeaveEvent(&new_event);
	}

	bool QuickScene3DWrapper::eventFilter(QObject *o, QEvent *e)
	{
		int type = e->type();
#ifdef DEBUG
		if (this == o)
		{
			if (m_scene3D &&
				type != QEvent::KeyPress &&
				type != QEvent::KeyRelease)
			{
				QCoreApplication::sendEvent(m_scene3D, e);
			}
			return false;
		}
#endif
		if (!hasFocus())
			return false;

		if (type == QEvent::KeyPress)
		{
			QKeyEvent* keyEvent = reinterpret_cast<QKeyEvent*>(e);
			m_eventSubdivide->keyPressEvent(keyEvent);

#ifdef DEBUG
			if (keyEvent->key() == Qt::Key_F11)
			{
				m_forceAlways = true;
				setAlways(m_always);
				currentRenderGraph()->setShowDebugOverlay(true);
			}
			if (keyEvent->key() == Qt::Key_F12)
			{
				m_forceAlways = false;
				setAlways(m_always);
				currentRenderGraph()->setShowDebugOverlay(false);
			}
#endif
		}
		else if (type == QEvent::KeyRelease)
		{
			QKeyEvent* keyEvent = reinterpret_cast<QKeyEvent*>(e);
			m_eventSubdivide->keyReleaseEvent(keyEvent);
		}

		return false;
	}

	void QuickScene3DWrapper::renderRenderGraph(qtuser_3d::RenderGraph* graph)
	{
		if (isRenderRenderGraph(graph))
			return;

		qtuser_3d::RenderGraph* currentGraph = currentRenderGraph();
		if (currentGraph)
		{
			currentGraph->endRender();
			currentGraph->sceneGraph()->setEnabled(false);
			//currentGraph->setParent((Qt3DCore::QNode*)nullptr);
			currentGraph->setParent(m_defaultFG);
			disconnect(currentGraph, SIGNAL(signalUpdate()), this, SLOT(requestUpdate()));

			qtuser_3d::XRenderGraph* xRender = qobject_cast<qtuser_3d::XRenderGraph*>(currentGraph);
			if (xRender)
			{
				m_eventSubdivide->removeHoverEventHandler(xRender);
				m_eventSubdivide->removeKeyEventHandler(xRender);
				m_eventSubdivide->removeLeftMouseEventHandler(xRender);
				m_eventSubdivide->removeRightMouseEventHandler(xRender);
				m_eventSubdivide->removeMidMouseEventHandler(xRender);
				m_eventSubdivide->removeResizeEventHandler(xRender);
				m_eventSubdivide->removeWheelEventHandler(xRender);
			}
		}

		if (graph)
		{
			connect(graph, SIGNAL(signalUpdate()), this, SLOT(requestUpdate()));

			QSize itemSize = m_size;
			if (m_ratio > 0)
			{
				itemSize *= m_ratio;
			}
			graph->updateRenderSize(itemSize);
			graph->sceneGraph()->setEnabled(true);
			graph->setParent(m_renderSettings);
			graph->begineRender();

			qtuser_3d::XRenderGraph* xRender = qobject_cast<qtuser_3d::XRenderGraph*>(graph);
			if (xRender)
			{
				m_eventSubdivide->addHoverEventHandler(xRender);
				m_eventSubdivide->addKeyEventHandler(xRender);
				m_eventSubdivide->addLeftMouseEventHandler(xRender);
				m_eventSubdivide->addRightMouseEventHandler(xRender);
				m_eventSubdivide->addMidMouseEventHandler(xRender);
				m_eventSubdivide->addResizeEventHandler(xRender);
				m_eventSubdivide->addWheelEventHandler(xRender);
			}
		}

		m_renderSettings->setActiveFrameGraph(graph);
	}

	void QuickScene3DWrapper::registerRenderGraph(qtuser_3d::RenderGraph* graph)
	{
		if (graph && (m_registerRenderGraph.indexOf(graph) == -1))
		{
			m_registerRenderGraph.push_back(graph);
		}
	}

	void QuickScene3DWrapper::unRegisterRenderGraph(qtuser_3d::RenderGraph* graph)
	{
	}

	void QuickScene3DWrapper::unRegisterAll()
	{
		for (qtuser_3d::RenderGraph* graph : m_registerRenderGraph)
			graph->setParent(m_renderSettings);
	}

	bool QuickScene3DWrapper::isRenderRenderGraph(qtuser_3d::RenderGraph* graph)
	{
		return graph && (currentRenderGraph() == graph);
	}

	qtuser_3d::RenderGraph* QuickScene3DWrapper::currentRenderGraph()
	{
		return qobject_cast<qtuser_3d::RenderGraph*>(m_renderSettings->activeFrameGraph());
	}

	qtuser_3d::EventSubdivide* QuickScene3DWrapper::eventSubdivide()
	{
		return m_eventSubdivide;
	}

	void QuickScene3DWrapper::requestUpdate()
	{
		if (m_scene3D)
		{
			bool originState = always();
			if (originState == false)
			{
				setAlways(true);
				setAlways(false);
			}
//#if Q515
//			QMetaObject::invokeMethod(m_scene3D, "requestUpdate", Qt::AutoConnection);
//#else
//			QMetaObject::invokeMethod(m_scene3D, "update", Qt::AutoConnection);
//#endif
		}
	}

	void QuickScene3DWrapper::releaseAlwaysRender()
	{
		setAlways(false);
	}

	void QuickScene3DWrapper::_geometry(int width, int height)
	{
		QSize size(width, height);
		m_eventSubdivide->geometryChanged(size);

		qtuser_3d::RenderGraph* graph = currentRenderGraph();
		if (graph)
			graph->updateRenderSize(size);

		requestUpdate();
	}

	void QuickScene3DWrapper::setAlways(bool always)
	{
		m_always = always;
#ifdef DEBUG
		if (m_forceAlways)
		{
			m_renderSettings->setRenderPolicy(Qt3DRender::QRenderSettings::Always);
			return;
		}
#endif
		m_renderSettings->setRenderPolicy(always ? Qt3DRender::QRenderSettings::Always : Qt3DRender::QRenderSettings::OnDemand);
	}

	bool QuickScene3DWrapper::always()
	{
		return m_always;
	}

	void QuickScene3DWrapper::sync()
	{
		if(!m_sharedContext)
			setSharedContext(QOpenGLContext::currentContext());
	}

	void QuickScene3DWrapper::cleanup()
	{
		qDebug() << Q_FUNC_INFO;
	}


	void QuickScene3DWrapper::setSharedContext(QOpenGLContext* context)
	{
		m_sharedContext = context;
		m_rawOGL->init(m_sharedContext);
		assert(m_sharedContext);

		checkVendor();
	}

	QOpenGLContext* QuickScene3DWrapper::sharedContext()
	{
		return m_sharedContext;
	}

	qtuser_3d::RawOGL* QuickScene3DWrapper::rawOGL()
	{
		return m_rawOGL;
	}

	void QuickScene3DWrapper::handleWindowChanged(QQuickWindow* win)
	{
		if (win) {
			connect(win, &QQuickWindow::beforeSynchronizing, this, &QuickScene3DWrapper::sync, Qt::DirectConnection);
			connect(win, &QQuickWindow::sceneGraphInvalidated, this, &QuickScene3DWrapper::cleanup, Qt::DirectConnection);
		}
	}
}