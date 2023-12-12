#include "pxentity.h"

namespace qtuser_3d
{
	PickXEntity::PickXEntity(Qt3DCore::QNode* parent)
		: XEntity(parent)
		, m_pickable(nullptr)
	{
		m_vertexBaseParameter = setParameter("vertexBase", QPoint(0, 0));
		m_stateParameter = setParameter("state", 0);

		Pickable* pickable = new Pickable(this);
		pickable->setNoPrimitive(true);
		bindPickable(pickable);
	}
	
	PickXEntity::~PickXEntity()
	{
	}

	Pickable* PickXEntity::pickable()
	{
		return m_pickable;
	}

	void PickXEntity::bindPickable(Pickable* pickable)
	{
		if (!pickable || (m_pickable == pickable))
			return;

		if (m_pickable)
		{
			disconnect(m_pickable, SIGNAL(signalStateChanged(ControlState)), this, SLOT(slotStateChanged(ControlState)));
			disconnect(m_pickable, SIGNAL(signalFaceBaseChanged(int)), this, SLOT(slotFaceBaseChanged(int)));
			delete m_pickable;
		}

		m_pickable = pickable;
		if (m_pickable)
		{
			connect(m_pickable, SIGNAL(signalStateChanged(ControlState)), this, SLOT(slotStateChanged(ControlState)));
			connect(m_pickable, SIGNAL(signalFaceBaseChanged(int)), this, SLOT(slotFaceBaseChanged(int)));

			m_pickable->signalStateChanged(m_pickable->state());
			m_pickable->signalFaceBaseChanged(m_pickable->faceBase());
		}
	}

	void PickXEntity::setVisualState(qtuser_3d::ControlState state)
	{
		m_stateParameter->setValue((int)state);
	}

	void PickXEntity::setVisualVertexBase(const QPoint& vertexBase)
	{
		m_vertexBaseParameter->setValue(vertexBase);
	}

	void PickXEntity::slotStateChanged(ControlState state)
	{
		onStateChanged(state);
	}

	void PickXEntity::onStateChanged(ControlState state)
	{

	}

	void PickXEntity::slotFaceBaseChanged(int faceBase)
	{
		QPoint vertexBase;
		vertexBase.setX(faceBase * 3);
		vertexBase.setY(m_pickable->noPrimitive() ? 1 : 0);
		m_vertexBaseParameter->setValue(vertexBase);
	}
}