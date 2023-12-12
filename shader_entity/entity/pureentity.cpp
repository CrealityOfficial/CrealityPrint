#include "pureentity.h"
#include "effect/purexeffect.h"

namespace qtuser_3d
{
	PureEntity::PureEntity(Qt3DCore::QNode* parent)
		:XEntity(parent)
		, m_color(nullptr)
	{
		setEffect(new PureXEffect());
	}

	PureEntity::~PureEntity()
	{

	}

	void PureEntity::setColor(const QVector4D& color)
	{
		if(!m_color)
			m_color = setParameter("color", QVector4D(1.0f, 0.0f, 0.0f, 1.0f));
		m_color->setValue(color);
	}
}