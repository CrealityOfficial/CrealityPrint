#include "gcodevieweffect.h"

namespace cxgcode
{
	GCodeViewEffect::GCodeViewEffect(Qt3DCore::QNode* parent)
		: XEffect(parent)
	{
		addRenderPass(new qtuser_3d::XRenderPass("gcodepreviewG", this));
	}

	GCodeViewEffect::~GCodeViewEffect()
	{

	}
}