#include "simplepickable.h"
#include "qtuser3d/refactor/pxentity.h"

namespace qtuser_3d
{
	SimplePickable::SimplePickable(QObject* parent)
		:Pickable(parent)
		, m_pickableEntity(nullptr)
	{
		m_faceBase = 0;
		m_noPrimitive = true;
	}
	
	SimplePickable::~SimplePickable()
	{
	}

	void SimplePickable::setPickableEntity(PickXEntity* entity)
	{
		m_pickableEntity = entity;
		if (m_pickableEntity)
		{
			m_pickableEntity->setVisualState(m_state);

			QPoint vertexBase;
			vertexBase.setX(m_faceBase * 3);
			vertexBase.setY(m_noPrimitive ? 1 : 0);
			m_pickableEntity->setVisualVertexBase(vertexBase);

			pickableEntityChanged();
		}
	}

	void SimplePickable::pickableEntityChanged()
	{

	}

	void SimplePickable::onStateChanged(ControlState state)
	{
		if (m_pickableEntity)
			m_pickableEntity->setVisualState(state);
	}

	void SimplePickable::faceBaseChanged(int faceBase)
	{
		if (m_pickableEntity)
		{
			QPoint vertexBase;
			vertexBase.setX(faceBase * 3);
			vertexBase.setY(m_noPrimitive ? 1 : 0);
			m_pickableEntity->setVisualVertexBase(vertexBase);
		}
	}
}