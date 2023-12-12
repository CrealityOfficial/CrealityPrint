#include "selector.h"
#include <QRect>

namespace qtuser_3d
{
	Selector::Selector(QObject* parent)
		:QObject(parent)
		, m_pickSource(nullptr)
		, m_hoverPickable(nullptr)
		, m_currentFaceBase(0)
		, selectNotifying(false)
		, m_disableReverseSelect(false)
		, m_enabled(true)
	{
	}

	Selector::~Selector()
	{
	}

	void Selector::setPickerSource(FacePicker* picker)
	{
		m_pickSource = picker;
	}

	void Selector::disableReverseSelect(bool disable)
	{
		m_disableReverseSelect = disable;
	}

	void Selector::setEnabled(bool enabled)
	{
		m_enabled = enabled;
	}

	void Selector::changed(qtuser_3d::Pickable* pickable)
	{
		if (pickable && pickable->selected())
		{
			for (SelectorTracer* tracer : m_selectorTracers)
			{
				tracer->selectChanged(pickable);
			}
		}
	}

	void Selector::addPickable(Pickable* pickable)
	{
		if (pickable)
		{
			if (m_pickables.indexOf(pickable) < 0)
			{
				_add(pickable);
				updateFaceBases();
			}

			if (m_pickables.size() == 1)
			{
				selectPickable(pickable);
			}

			if (pickable->enableSelect())
				notifyTracers();
		}
	}

	void Selector::removePickable(Pickable* pickable)
	{
		if (pickable)
		{
			_remove(pickable);
			updateFaceBases(); //prevent m_currentFaceBase from changing into nagetive value, modified by wys, 2023-3-21

			pickable->setState(ControlState::none);
			if (m_hoverPickable && (m_hoverPickable == pickable))
			{
				clearHover();
			}

			m_selectedPickables.removeOne(pickable);
			if (pickable->enableSelect())
				notifyTracers();
		}
	}

	void Selector::addTracer(SelectorTracer* tracer)
	{
		if (tracer)
		{
			m_selectorTracers.push_back(tracer);
			notifyTracers(tracer);
		}
	}

	void Selector::removeTracer(SelectorTracer* tracer)
	{
		if (tracer) m_selectorTracers.removeOne(tracer);
	}

	QList<qtuser_3d::Pickable*> Selector::selectionmPickables()
	{
		return m_selectedPickables;
	}

	bool Selector::facePick(const QPoint& p, int* faceID)
	{
		return checkPickerColor(m_pickSource, p, faceID);
	}

	Pickable* Selector::check(const QPoint& p, int* primitiveID)
	{
		Pickable* pickable = checkPickableFromList(m_pickSource, p, m_pickables, primitiveID);

		return pickable;
	}

	//void Selector::_add(Pickable* pickable)
	//{
	//	m_pickables.push_back(pickable);

	//	bool noPrimitive = pickable->noPrimitive();
	//	int primitiveNum = pickable->primitiveNum();
	//	if (noPrimitive) primitiveNum = 1;

	//	pickable->setFaceBase(m_currentFaceBase);
	//	m_currentFaceBase += primitiveNum;
	//}

	void Selector::_add(Pickable* pickable)
	{
		m_pickables.push_back(pickable);
	}

	//void Selector::_remove(Pickable* pickable)
	//{
	//	//rebase 
	//	bool noPrimitive = pickable->noPrimitive();
	//	int primitiveNum = pickable->primitiveNum();
	//	if (noPrimitive) primitiveNum = 1;

	//	int offset = primitiveNum;

	//	int size = m_pickables.size();
	//	int index = m_pickables.indexOf(pickable);
	//	if (index >= 0)
	//	{
	//		for (int i = index + 1; i < size; ++i)
	//		{
	//			Pickable* p = m_pickables.at(i);
	//			int fBase = p->faceBase();
	//			fBase -= offset;
	//			p->setFaceBase(fBase);
	//		}

	//		m_pickables.removeAt(index);
	//		m_currentFaceBase -= offset;
	//	}
	//}

	void Selector::_remove(Pickable* pickable)
	{
		int index = m_pickables.indexOf(pickable);
		if (index >= 0)
		{
			m_pickables.removeAt(index);
		}
	}

	void Selector::updateFaceBases()
	{
		int currentFaceBase = 0;

		for (qtuser_3d::Pickable* pickable : m_pickables)
		{
			if (pickable->faceBase() >= 40000000)
			{
				// supportData faceBase no need to update
				continue;
			}

			bool noPrimitive = pickable->noPrimitive();
			int primitiveNum = pickable->primitiveNum();
			if (noPrimitive) primitiveNum = 1;

			pickable->setFaceBase(currentFaceBase);
			currentFaceBase += primitiveNum;
		}
	}

	void Selector::clearHover()
	{
		if (m_hoverPickable)
		{
			m_hoverPickable->setState(qtuser_3d::ControlState::none);
			m_hoverPickable = nullptr;
		}
	}

	bool Selector::hover(const QPoint& p)
	{
		if (!m_enabled)
			return false;

		Pickable* hoverEntity = nullptr;
		int primitiveID = -1;
		hoverEntity = check(p, &primitiveID);
		
		if (m_hoverPickable == hoverEntity)
		{
			if (hoverEntity && hoverEntity->isGroup())
			{
				bool changed = hoverEntity->hoverPrimitive(primitiveID);
				return changed;
			}

			return false;
		}

		if (m_hoverPickable)
		{
			if (m_hoverPickable->isGroup())
			{
				m_hoverPickable->unHoverPrimitive();
				m_hoverPickable = nullptr;
			}
			else if (m_hoverPickable->state() != ControlState::selected)
			{
				m_hoverPickable->setState(ControlState::none);
				m_hoverPickable = nullptr;
			}
		}

		if (hoverEntity && hoverEntity->selected())
			return true;

		m_hoverPickable = hoverEntity;

		if (m_hoverPickable && !m_hoverPickable->isGroup())
		{
			m_hoverPickable->setState(ControlState::hover);
		}
		return true;
	}

	void Selector::select(const QPoint& p, bool sGroup)
	{
		if (!m_enabled)
			return;

		if (selectNotifying)
			return;
		int primitiveID = -1;
		Pickable* pickable = check(p, &primitiveID);

		if (pickable)
		{
			pickable->pick(primitiveID);
		}

		if (pickable && pickable->enableSelect() == false)
		{
			return;
		}

		if (sGroup)
			selectGroup(pickable);
		else selectPickable(pickable);
	}

	void Selector::selectCtrl(const QPoint& p, bool sGroup)
	{
		if (!m_enabled)
			return;

		if (selectNotifying)
			return;
		Pickable* pickable = check(p, nullptr);
		selectGroup(pickable);
		

	}
	void Selector::selectGroup(qtuser_3d::Pickable* pickable)
	{
		if (!m_enabled)
			return;

		QList<Pickable*> offList;
		QList<Pickable*> onList = m_selectedPickables;
		for (size_t i = 0; i < onList.size(); i++)
		{
			if (pickable == onList[i])
			{
				offList << onList[i];//by TCJ -ctrl uncheck
				onList.removeAt(i);
				selectPickables(onList, offList);
				return;
			}
		}
		if (pickable && pickable->enableSelect())
			onList << pickable;
		selectPickables(onList, offList);
	}

	void Selector::selectPickable(Pickable* pickable)
	{
		if (!m_enabled)
			return;

		if (selectNotifying)
			return;
		
		QList<Pickable*> offList = m_selectedPickables;
		QList<Pickable*> onList;
		if (m_disableReverseSelect && !pickable)
			return;

		if (pickable && pickable->enableSelect())
			onList << pickable;

		selectPickables(onList, offList);
	}

	void Selector::appendSelect(qtuser_3d::Pickable* pickable)
	{
		if (!m_enabled)
			return;

		if (selectNotifying)
			return;

		if (pickable && !pickable->selected())
		{
			pickable->setSelected(true);
			m_selectedPickables << pickable;
			notifyTracers();
		}
	}

	void Selector::appendSelects(const QList<qtuser_3d::Pickable*> pickables)
	{
		if (!m_enabled)
			return;

		if (selectNotifying)
			return;

		for (qtuser_3d::Pickable* pickable : pickables)
		{
			if (pickable && !pickable->selected() && pickable->enableSelect())
			{
				pickable->setSelected(true);
				m_selectedPickables << pickable;
			}
		}
		notifyTracers();
	}

	void Selector::selectAll()
	{
		if (!m_enabled)
			return;

		if (selectNotifying)
			return;

		QList<Pickable*> onLists;
		QList<Pickable*> offlist;
		for (Pickable* pickable : m_pickables)
		{
			onLists << pickable;
		}
		selectPickables(onLists, offlist);
	}

	void Selector::selectNone()
	{
		selectPickable(nullptr);
	}

	void Selector::selectOne(qtuser_3d::Pickable* pickable)
	{
		selectPickable(pickable);
	}

	void Selector::selectMore(const QList<qtuser_3d::Pickable*>& pickables)
	{
		QList<qtuser_3d::Pickable*> offLists = selectionmPickables();
		selectPickables(pickables, offLists);
	}

	void Selector::unselectOne(qtuser_3d::Pickable* pickable)
	{
		if (!pickable || !pickable->selected())
			return;

		QList<qtuser_3d::Pickable*> pickables;
		pickables << pickable;
		unselectMore(pickables);
	}

	void Selector::unselectMore(const QList<qtuser_3d::Pickable*>& pickables)
	{
		QList<qtuser_3d::Pickable*> onList;
		selectPickables(onList, pickables);
	}

	void Selector::selectPickables(const QList<Pickable*>& onList, const QList<Pickable*>& offList)
	{
		if (!m_enabled)
			return;

		if (selectNotifying)
			return;

		if ((onList.size() == 0) && (offList.size() == 0))
			return;

		for (Pickable* pickable : offList)
		{
			pickable->setSelected(false);
		}

		for (Pickable* pickable : onList)
		{
			pickable->setSelected(true);
		}

		m_selectedPickables = onList;
		notifyTracers();
	}

	void Selector::notifyTracers(qtuser_3d::SelectorTracer* tracer)
	{
		selectNotifying = true;

		if (tracer)
		{
			tracer->onSelectionsChanged();
		}
		else
		{
			for (SelectorTracer* tracer1 : m_selectorTracers)
			{
				tracer1->onSelectionsChanged();
			}
		}

		selectNotifying = false;
		onChanged();
	}

	void Selector::onChanged()
	{

	}

	void Selector::selectRect(const QRect& rect, bool exclude)
	{
		if (rect.size().isNull()) return;

		int width = abs(rect.width()), height = abs(rect.height());
		if (width <= 1 || height <= 1) return;
	
		int x = std::min(rect.left(), rect.right()), y = std::min(rect.top(), rect.bottom());

		QList<qtuser_3d::Pickable*> list;

		for (size_t i = 0; i < height; i++)
		{
			for (size_t j = 0; j < width; j++)
			{
				QPoint p(x + j, y + i);
				Pickable* obj = check(p);
				if (obj && obj->enableSelect() && list.contains(obj) == false)
				{
					list << obj;
				}
			}
		}

		if (exclude)
		{
			QList<Pickable*> offlist;
			for (Pickable* pickable : m_pickables)
			{
				if (!list.contains(pickable)) 
				{
					offlist << pickable;
				}
			}
			selectPickables(list, offlist);
		}
		else {
			appendSelects(list);
		}
	}

}