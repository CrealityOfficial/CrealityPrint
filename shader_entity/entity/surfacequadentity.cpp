#include "surfacequadentity.h"
#include "effect/surfacequadeffect.h"

namespace qtuser_3d
{
	SurfaceQuadEntity::SurfaceQuadEntity(Qt3DCore::QNode* parent)
		:XEntity(parent)
		, m_color(nullptr)
	{
		setEffect(new SurfaceQuadEffect());
	}

	SurfaceQuadEntity::~SurfaceQuadEntity()
	{

	}

	void SurfaceQuadEntity::setColor(const QVector4D& color)
	{
		if (!m_color)
			m_color = setParameter("color", QVector4D(1.0f, 0.0f, 0.0f, 1.0f));
		m_color->setValue(color);
	}
}