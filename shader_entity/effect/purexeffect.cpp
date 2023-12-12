#include "purexeffect.h"
#include "qtuser3d/refactor/xrenderpass.h"
#include "renderpass/purerenderpass.h"

namespace qtuser_3d
{
	PureXEffect::PureXEffect(Qt3DCore::QNode* parent)
		: XEffect(parent)
	{
		m_color = setParameter("color", QVector4D(0.5f, 0.5f, 0.5f, 1.0f));
		addRenderPass(new qtuser_3d::PureRenderPass());
	}

	PureXEffect::~PureXEffect()
	{

	}

	void PureXEffect::setColor(const QVector4D& color)
	{
		m_color->setValue(color);
	}
}