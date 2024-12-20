#include "pureentity.h"
#include "effect/purexeffect.h"

namespace qtuser_3d
{
	PureEntity::PureEntity(Qt3DCore::QNode* parent)
		:XEntity(parent)
	{
		m_effect = new PureXEffect();
		setEffect(m_effect);
	}

	PureEntity::~PureEntity()
	{

	}

	void PureEntity::setColor(const QVector4D& color)
	{
		m_effect->setColor(color);
	}
}