#include "supportposeffect.h"
#include "qtuser3d/refactor/xrenderpass.h"
#include "renderpass/phongrenderpass.h"

namespace qtuser_3d
{
	SupportPosEffect::SupportPosEffect(Qt3DCore::QNode* parent)
		: XEffect(parent)
	{
		addRenderPass(new qtuser_3d::PhongRenderPass());
	}

	SupportPosEffect::~SupportPosEffect()
	{

	}

}