#include "pickable.h"
#include "qtuser3d/module/facepicker.h"
#include <QtCore/QDebug>

namespace qtuser_3d
{
	Pickable::Pickable(QObject* parent)
		:QObject(parent)
		, m_state(ControlState::none)
		, m_enableSelect(true)
		, m_faceBase(0)
		, m_noPrimitive(false)
		, m_visible(true)
	{
	}

	Pickable::~Pickable()
	{
	}

	bool Pickable::hoverPrimitive(int primitiveID)
	{
		return false;
	}

	void Pickable::selectPrimitive(int chunk)
	{

	}

	void Pickable::unHoverPrimitive()
	{
		hoverPrimitive(-1);
	}

	void Pickable::setSelected(bool selected)
	{
		if (selected && !m_enableSelect) 
			return;
		setState(selected ? ControlState::selected : ControlState::none);
	}

	ControlState Pickable::state()
	{
		return m_state;
	}

	void Pickable::setState(ControlState state)
	{
		m_state = state;
		onStateChanged(m_state);
	}

	bool Pickable::selected()
	{
		return m_state == ControlState::selected;
	}

	void Pickable::onStateChanged(ControlState state)
	{
		emit signalStateChanged(state);
	}

	bool Pickable::enableSelect()
	{
		return m_enableSelect;
	}

	void Pickable::setEnableSelect(bool enable)
	{
		m_enableSelect = enable;
	}

	void Pickable::faceBaseChanged(int faceBase)
	{
		emit signalFaceBaseChanged(faceBase);
	}

	void Pickable::noPrimitiveChanged(bool noPrimitive)
	{

	}

	int Pickable::primitiveNum()
	{
		return 0;
	}

	int Pickable::chunk(int primitiveID)
	{
		return -1;
	}

	bool Pickable::isGroup()
	{
		return false;
	}

	void Pickable::setFaceBase(int faceBase)
	{
		m_faceBase = faceBase;
		faceBaseChanged(m_faceBase);
	}

	int Pickable::faceBase()
	{
		return m_faceBase;
	}

	bool Pickable::noPrimitive()
	{
		return m_noPrimitive;
	}

	void Pickable::setNoPrimitive(bool noPrimitive)
	{
		m_noPrimitive = noPrimitive;
		noPrimitiveChanged(m_noPrimitive);
	}

	void Pickable::setVisible(bool visible)
	{
		m_visible = visible;
	}

	bool Pickable::isVisible()
	{
		return m_visible;
	}

	Pickable* checkPickableFromList(FacePicker* picker, QPoint point, QList<Pickable*>& list, int* primitiveID)
	{
		Pickable* pickable = nullptr;
		if (picker)
		{
			int faceID = -1;
			int _primitiveID = -1;
			bool picked = picker->pick(point, &faceID);
			if (picked)
			{
				int size = list.size();
				for (int i = 0; i < size; ++i)
				{
					Pickable* p = list.at(i);
					int faceBase = p->faceBase();
					int faceEnd = faceBase + (p->noPrimitive() ? 1 : p->primitiveNum());

					if (faceID >= faceBase && faceID < faceEnd && p->isVisible())
					{
						pickable = p;
						break;
					}
				}

				if (pickable)
				{
					int faceBase = pickable->faceBase();
					_primitiveID = faceID - faceBase;
				}
				else
					_primitiveID = faceID;
			}

			if (primitiveID)
				*primitiveID = _primitiveID;

#if 0 //_DEBUG
			if(pickable)
				qDebug() << "Face ID "<< faceID <<" Primitive ID " << _primitiveID;
#endif
		}

		return pickable;
	}

	bool checkPickerColor(FacePicker* picker, QPoint point, int* faceID)
	{
		bool check = false;
		if (picker)
			check = picker->pick(point, faceID);	

		return check;
	}

	void Pickable::pick(int primitiveID)
	{

	}
}