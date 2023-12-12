#include "gcodevieweffect.h"

namespace qtuser_3d
{
	GCodeViewEffect::GCodeViewEffect(Qt3DCore::QNode* parent)
		: XEffect(parent)
	{
		addRenderPass(new XRenderPass("gcodepreviewG", this));
	}

	GCodeViewEffect::~GCodeViewEffect()
	{

	}
}