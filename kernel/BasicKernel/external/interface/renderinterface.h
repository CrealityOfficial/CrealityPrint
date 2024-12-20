#ifndef CREATIVE_KERNEL_RENDERINTERFACE_1594447238062_H
#define CREATIVE_KERNEL_RENDERINTERFACE_1594447238062_H
#include "basickernelexport.h"
#include "data/kernelenum.h"
#include "qtuser3d/framegraph/rendergraph.h"

namespace creative_kernel
{
	BASIC_KERNEL_API void registerResidentNode(Qt3DCore::QNode* node);
	BASIC_KERNEL_API void unRegisterResidentNode(Qt3DCore::QNode* node);
	BASIC_KERNEL_API void renderRenderGraph(qtuser_3d::RenderGraph* graph);
	BASIC_KERNEL_API void registerRenderGraph(qtuser_3d::RenderGraph* graph);
	BASIC_KERNEL_API void renderOneFrame();

	BASIC_KERNEL_API bool isRenderRenderGraph(qtuser_3d::RenderGraph* graph);
	BASIC_KERNEL_API bool isRenderDefaultRenderGraph();

	BASIC_KERNEL_API void renderDefaultRenderGraph();

	BASIC_KERNEL_API void setContinousRender();
	BASIC_KERNEL_API void setCommandRender();
	// BASIC_KERNEL_API void setCommandRenderEx();
}
#endif // CREATIVE_KERNEL_RENDERINTERFACE_1594447238062_H