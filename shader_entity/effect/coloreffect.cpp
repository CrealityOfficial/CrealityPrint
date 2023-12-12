#include "coloreffect.h"
#include "qtuser3d/refactor/xrenderpass.h"
#include "renderpass/colorrenderpass.h"

namespace qtuser_3d
{
	ColorEffect::ColorEffect(Qt3DCore::QNode* parent)
		: XEffect(parent)
	{
		addRenderPass(new qtuser_3d::ColorRenderPass());
	}

	ColorEffect::~ColorEffect()
	{

	}

}