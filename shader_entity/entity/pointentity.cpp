#include "pointentity.h"
#include "effect/purexeffect.h"
#include "qtuser3d/trimesh2/renderprimitive.h"

namespace qtuser_3d
{
	PointEntity::PointEntity(Qt3DCore::QNode* parent)
		:XEntity(parent)
		, m_color(nullptr)
	{
		PureXEffect* effect = new PureXEffect();
		m_pointSizeState = new Qt3DRender::QPointSize(this);
		m_pointSizeState->setSizeMode(Qt3DRender::QPointSize::Fixed);
		effect->addRenderState(0, m_pointSizeState);

		setEffect(effect);
	}

	PointEntity::~PointEntity()
	{

	}

	void PointEntity::setPointSize(float size)
	{
		m_pointSizeState->setValue(size);
	}

	void PointEntity::setPoints(const std::vector<trimesh::vec3>& points)
	{
		setGeometry(createLinesGeometry(points), Qt3DRender::QGeometryRenderer::Points);
	}

	void PointEntity::setColor(const QVector4D& color)
	{
		if (!m_color)
			m_color = setParameter("color", QVector4D(1.0f, 0.0f, 0.0f, 1.0f));
		m_color->setValue(color);
	}
}