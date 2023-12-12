#include "rendercenter.h"
//#include "qtuser3d/module/glquickitem.h"
#include "qtuser3d/module/quickscene3dwrapper.h"
#include "qtuser3d/event/eventsubdivide.h"

namespace qtuser_3d
{
	RenderCenter::RenderCenter(QObject* parent)
		:QObject(parent)
		//,m_glQuickItem(nullptr)
		, m_eventSubdivide(nullptr)
		, m_scene3DWrapper(nullptr)
	{
		//setScene3DWrapper(new qtuser_qml::QuickScene3DWrapper());
	}
	
	RenderCenter::~RenderCenter()
	{
	}

	/*void RenderCenter::setGLQuickItem(GLQuickItem* item)
	{
		m_glQuickItem = item;
		if (m_glQuickItem)
		{
			m_eventSubdivide = m_glQuickItem->eventSubdivide();
		}
	}

	GLQuickItem* RenderCenter::glQuickItem()
	{
		return m_glQuickItem;
	}*/

	void RenderCenter::setScene3DWrapper(qtuser_3d::QuickScene3DWrapper* item)
	{
		m_scene3DWrapper = item;
		if (m_scene3DWrapper)
		{
			m_eventSubdivide = m_scene3DWrapper->eventSubdivide();
		}
	}
	
	qtuser_3d::QuickScene3DWrapper* RenderCenter::scene3DWrapper()
	{
		return m_scene3DWrapper;
	}

	qtuser_3d::RawOGL* RenderCenter::rawOGL()
	{
		return m_scene3DWrapper->rawOGL();
	}

	qtuser_3d::EventSubdivide* RenderCenter::eventSubdivide()
	{
		return m_eventSubdivide;
	}

	void RenderCenter::uninitialize()
	{
		qDebug() << "RenderCenter uninitialize";
		if (m_scene3DWrapper)
			m_scene3DWrapper->unRegisterAll();
 		if (m_eventSubdivide)
			m_eventSubdivide->closeHandlers();
	}

	void RenderCenter::registerResidentNode(Qt3DCore::QNode* node)
	{
		/*if (m_glQuickItem)
			m_glQuickItem->registerResidentNode(node);*/
	}

	void RenderCenter::unRegisterResidentNode(Qt3DCore::QNode* node)
	{
		/*if (m_glQuickItem)
			m_glQuickItem->unRegisterResidentNode(node);*/
	}

	void RenderCenter::renderRenderGraph(qtuser_3d::RenderGraph* graph)
	{
		if (m_scene3DWrapper)
			m_scene3DWrapper->renderRenderGraph(graph);
	}

	bool RenderCenter::isRenderRenderGraph(qtuser_3d::RenderGraph* graph)
	{
		if (m_scene3DWrapper)
			return m_scene3DWrapper->isRenderRenderGraph(graph);
		return false;
	}

	qtuser_3d::RenderGraph* RenderCenter::currentRenderGraph()
	{
		if (m_scene3DWrapper)
			return m_scene3DWrapper->currentRenderGraph();
		return nullptr;
	}

	void RenderCenter::registerRenderGraph(qtuser_3d::RenderGraph* graph)
	{
		if (m_scene3DWrapper)
			m_scene3DWrapper->registerRenderGraph(graph);
	}

	void RenderCenter::unRegisterRenderGraph(qtuser_3d::RenderGraph* graph)
	{
		if (m_scene3DWrapper)
			m_scene3DWrapper->unRegisterRenderGraph(graph);
	}

	void RenderCenter::renderOneFrame()
	{
		if (m_scene3DWrapper)
			m_scene3DWrapper->requestUpdate();
	}

	void RenderCenter::exposureScene3D(QObject* scene3d)
	{
		if (scene3d && m_scene3DWrapper)
		{
			m_scene3DWrapper->bindScene3D(scene3d);
		}
	}
}
