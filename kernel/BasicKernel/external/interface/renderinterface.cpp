#include "renderinterface.h"

#include "qtuser3d/module/rendercenter.h"
#include "qtuser3d/module/quickscene3dwrapper.h"

#include "kernel/kernel.h"
#include "kernel/visualscene.h"

namespace creative_kernel
{
	void registerResidentNode(Qt3DCore::QNode* node)
	{
		getKernel()->renderCenter()->registerResidentNode(node);
	}

	void unRegisterResidentNode(Qt3DCore::QNode* node)
	{
		getKernel()->renderCenter()->unRegisterResidentNode(node);
	}

	void renderRenderGraph(qtuser_3d::RenderGraph* graph)
	{
		getKernel()->renderCenter()->renderRenderGraph(graph);
	}

	bool isRenderRenderGraph(qtuser_3d::RenderGraph* graph)
	{
		return getKernel()->renderCenter()->isRenderRenderGraph(graph);
	}

	void registerRenderGraph(qtuser_3d::RenderGraph* graph)
	{
		getKernel()->renderCenter()->registerRenderGraph(graph);
	}

	void renderOneFrame()
	{
		getKernel()->renderCenter()->renderOneFrame();
	}

	bool isRenderDefaultRenderGraph()
	{
		return isRenderRenderGraph(getKernel()->visScene());
	}

	void renderDefaultRenderGraph()
	{
		renderRenderGraph(getKernel()->visScene());
	}

	void setContinousRender()
	{
		getKernel()->renderCenter()->scene3DWrapper()->setAlways(true);
	}

	void setCommandRender()
	{
		getKernel()->renderCenter()->scene3DWrapper()->setAlways(false);
	}

	// void setCommandRenderEx()
	// {
	// 	getKernel()->renderCenter()->scene3DWrapper()->setAlways(false);
	// }
}