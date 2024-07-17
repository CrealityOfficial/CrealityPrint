#include "gcodevieweffect.h"

namespace qtuser_3d
{
	GCodeViewEffect::GCodeViewEffect(Qt3DCore::QNode* parent)
		: XEffect(parent)
	{
		XRenderPass* pass = new XRenderPass("gcodepreviewG", this);
		pass->addFilterKeyMask("alpha2nd", 0);
		pass->setPassDepthTest();
		pass->setPassCullFace(Qt3DRender::QCullFace::Back);
		addRenderPass(pass);
	}

	GCodeViewEffect::~GCodeViewEffect()
	{

	}
}