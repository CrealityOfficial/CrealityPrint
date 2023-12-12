#include "qtuser3d/framegraph/clearbuffergraph.h"

namespace qtuser_3d
{
	ClearBufferGraph::ClearBufferGraph(Qt3DCore::QNode* parent)
		:QFrameGraphNode(parent)
	{
		m_clearBuffer = new Qt3DRender::QClearBuffers(this);
		m_clearBuffer->setClearColor(QColor(0, 0, 0, 0));
		m_clearBuffer->setBuffers(Qt3DRender::QClearBuffers::BufferType::DepthBuffer);

		m_renderPassFilter = new Qt3DRender::QRenderPassFilter(m_clearBuffer);
		m_filterKey = new Qt3DRender::QFilterKey(m_renderPassFilter);
		m_filterKey->setName("overlayPass");
		m_filterKey->setValue(0);
		m_renderPassFilter->addMatch(m_filterKey);


	}

	ClearBufferGraph::~ClearBufferGraph()
	{
	}

}
