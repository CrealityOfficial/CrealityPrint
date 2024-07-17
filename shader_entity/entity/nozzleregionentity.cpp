#include "nozzleregionentity.h"
#include "renderpass/purerenderpass.h"
#include "qtuser3d/refactor/xeffect.h"

namespace qtuser_3d {

	NozzleRegionEntity::NozzleRegionEntity(Qt3DCore::QNode* parent)
		:XEntity(parent)
	{
		PureRenderPass *viewPass = new qtuser_3d::PureRenderPass();
		viewPass->setColor(QVector4D(1.0f, 0.0f, 1.f, 0.5f));
		viewPass->addFilterKeyMask("alpha", 0);
		viewPass->setPassBlend();
		viewPass->setPassDepthTest();
		viewPass->setPassNoDepthMask();
		viewPass->setPassCullFace(Qt3DRender::QCullFace::Back);

		XEffect* effect = new XEffect();
		effect->addRenderPass(viewPass);

		setEffect(effect);
	}

	NozzleRegionEntity::~NozzleRegionEntity()
	{
	}



}