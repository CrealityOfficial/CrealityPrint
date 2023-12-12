#include "faces.h"
#include "qtuser3d/refactor/xrenderpass.h"
#include "qtuser3d/refactor/xeffect.h"
#include "renderpass/purerenderpass.h"

namespace qtuser_3d
{
	Faces::Faces(Qt3DCore::QNode* parent)
		:XEntity(parent)
	{
		m_colorParameter = setParameter("color", QVector4D(0.180f, 0.541f, 0.968f, 0.3f));

		m_renderPass = new PureRenderPass(this);
		m_renderPass->addFilterKeyMask("alpha", 0);
		m_renderPass->setPassBlend();
		m_renderPass->setPassNoDepthMask();
		m_renderPass->setPassCullFace();

		XEffect* effect = new XEffect(this);
		effect->addRenderPass(m_renderPass);

		setEffect(effect);
	}
	
	Faces::~Faces()
	{
	}

	void Faces::setColor(const QVector4D& color)
	{
		m_colorParameter->setValue(color);
	}
}