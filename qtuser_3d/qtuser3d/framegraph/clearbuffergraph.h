#ifndef _QTUSER_3D_CLEARBUFFERGRAPH_H
#define _QTUSER_3D_CLEARBUFFERGRAPH_H
#include "qtuser3d/qtuser3dexport.h"
#include <Qt3DRender/QClearBuffers>
#include <Qt3DRender/QLayerFilter>
#include <Qt3DRender/QRenderPassFilter>
#include <Qt3DRender/QFilterKey>

namespace qtuser_3d
{

	class QTUSER_3D_API ClearBufferGraph : public Qt3DRender::QFrameGraphNode
	{
	public:
		ClearBufferGraph(Qt3DCore::QNode* parent = nullptr);
		virtual ~ClearBufferGraph();

	protected:
		Qt3DRender::QClearBuffers* m_clearBuffer;
		Qt3DRender::QRenderPassFilter* m_renderPassFilter;
		Qt3DRender::QFilterKey* m_filterKey;

	};
}
#endif // _QTUSER_3D_CLEARBUFFERGRAPH_H
