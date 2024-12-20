#include "purexeffect.h"
#include "qtuser3d/refactor/xrenderpass.h"
#include "renderpass/purerenderpass.h"

namespace qtuser_3d
{
	PureXEffect::PureXEffect(Qt3DCore::QNode* parent)
		: XEffect(parent)
	{
		// m_color = setParameter("color", QVector4D(0.5f, 0.5f, 0.5f, 1.0f));
		m_pass = new qtuser_3d::PureRenderPass();
		addRenderPass(m_pass);
		m_pass->setColor(QVector4D(0.5f, 0.5f, 0.5f, 1.0f));
	}

	PureXEffect::~PureXEffect()
	{

	}

	void PureXEffect::setColor(const QVector4D& color)
	{
		m_pass->setColor(color);
	}
}