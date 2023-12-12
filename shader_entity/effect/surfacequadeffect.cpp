#include "surfacequadeffect.h"
#include "qtuser3d/refactor/xrenderpass.h"

namespace qtuser_3d
{
	SurfaceQuadEffect::SurfaceQuadEffect(Qt3DCore::QNode* parent)
		: XEffect(parent)
	{
		m_color = setParameter("color", QVector4D(0.5f, 0.5f, 0.5f, 1.0f));
		addRenderPass(new qtuser_3d::XRenderPass("surfacequad"));
	}

	SurfaceQuadEffect::~SurfaceQuadEffect()
	{

	}

	void SurfaceQuadEffect::setColor(const QVector4D& color)
	{
		m_color->setValue(color);
	}
}