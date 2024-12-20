#include "overlayeffect.h"
#include "qtuser3d/refactor/xrenderpass.h"
#include "renderpass/overlayrenderpass.h"

namespace qtuser_3d
{
	OverlayEffect::OverlayEffect(Qt3DCore::QNode* parent)
		: XEffect(parent)
	{
		addRenderPass(new qtuser_3d::OverlayRenderPass());
		addPassFilter(0, QStringLiteral("view"));
	}

	OverlayEffect::~OverlayEffect()
	{

	}

}