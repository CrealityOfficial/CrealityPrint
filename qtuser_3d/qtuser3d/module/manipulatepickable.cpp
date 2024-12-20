#include "manipulatepickable.h"
#include "qtuser3d/refactor/pxentity.h"

namespace qtuser_3d
{
	ManipulatePickable::ManipulatePickable(QObject* parent)
		:SimplePickable(parent)
		, m_showEntity(nullptr)
	{
		setNoPrimitive(true);
	}
	
	ManipulatePickable::~ManipulatePickable()
	{
	}

	void ManipulatePickable::onStateChanged(ControlState state)
	{
		if (!enableSelect() && state == ControlState::selected)
			return;

		int index = (int)state;
		if (index < 0) index = 0;
		if (index > 2) index = 2;
		if (m_showEntity)
			m_showEntity->setVisualState(m_stateFactor[index]);
		else if (m_pickableEntity)
			m_pickableEntity->setVisualState(m_stateFactor[index]);
	}

	void ManipulatePickable::setStateFactor(ControlState sf[3])
	{
		for (int i = 0; i < 3; i++)
		{
			m_stateFactor[i] = sf[i];
		}
	}
	
	void ManipulatePickable::setShowEntity(PickXEntity* entity)
	{
		m_showEntity = entity;
	}

	void ManipulatePickable::pickableEntityChanged()
	{
		onStateChanged(m_state);
	}
}