#include "overlayrenderpass.h"

namespace qtuser_3d
{
	OverlayRenderPass::OverlayRenderPass(Qt3DCore::QNode* parent)
		: XRenderPass("overlay", parent)
	{
		m_color = setParameter("color", QVector4D(0.5f, 0.5f, 0.5f, 1.0f));
	}

	OverlayRenderPass::~OverlayRenderPass()
	{

	}

	void OverlayRenderPass::setColor(const QVector4D& color)
	{
		m_color->setValue(color);
	}
}