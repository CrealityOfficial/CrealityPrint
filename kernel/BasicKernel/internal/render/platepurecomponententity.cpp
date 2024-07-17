#include "platepurecomponententity.h"
#include "renderpass/purerenderpass.h"
#include "qtuser3d/refactor/xeffect.h"

using namespace qtuser_3d;

namespace creative_kernel {

	PlatePureComponentEntity::PlatePureComponentEntity(Qt3DCore::QNode* parent)
		:XEntity(parent)
	{
		XRenderPass* pass = new XRenderPass("pure", parent);
		pass->addFilterKeyMask("view", 0);
		pass->setPassCullFace(Qt3DRender::QCullFace::Back);
		pass->setPassDepthTest();

		XEffect* effect = new XEffect(this);
		effect->addRenderPass(pass);
		setEffect(effect);

		m_colorParameter = setParameter("color", QVector4D(0.27f, 0.27f, 0.27f, 1.0f));
	}

	PlatePureComponentEntity::~PlatePureComponentEntity()
	{
	}

	void PlatePureComponentEntity::setColor(const QVector4D& color)
	{
		m_colorParameter->setValue(color);
	}
}