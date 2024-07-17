#include "modelnselector.h"
#include "qtuser3d/module/pickable.h"
#include "qtuser3d/module/facepicker.h"
#include "qtuser3d/module/pickableselecttracer.h"
#include "qtusercore/plugin/toolcommandcenter.h"
#include "internal/tool/scalemode.h"
#include "data/modeln.h"
#include "kernel/kernelui.h"
#include "internal/render/wipetowerpickable.h"

using namespace qtuser_3d;
namespace creative_kernel
{
	ModelNSelector::ModelNSelector(QObject* parent)
		: Selector(parent)
	{
		addTracer(this);
	}
	
	ModelNSelector::~ModelNSelector()
	{
	}

	void ModelNSelector::sizeChanged()
	{
		emit selectedNumChanged();
	}

	QList<ModelN*> ModelNSelector::selectionms()
	{
		QList<ModelN*> models;
		QList<qtuser_3d::Pickable*> selections = selectionmPickables();
		for (Pickable* pickable : selections)
		{
			ModelN* m = qobject_cast<ModelN*>(pickable);
			if (m)
				models << m;
		}
		return models;
	}

	int ModelNSelector::selectedNum()
	{
		return selectionms().size();
	}

	QVector3D ModelNSelector::selectionmsSize()
	{
		QList<ModelN*> models = selectionms();
		qtuser_3d::Box3D box;
		for(ModelN* model : models)
			box += model->globalSpaceBox();
		return box.size();
	}

	void ModelNSelector::onSelectionsChanged()
	{
		emit selectedNumChanged();

		bool isSelectWipeTower = false;
		if (m_selectedPickables.count() == 1)
		{
			WipeTowerPickable* wipeTower = dynamic_cast<WipeTowerPickable*>(m_selectedPickables[0]);
			isSelectWipeTower = wipeTower;
		}

		if (m_wipeTowerSelected != isSelectWipeTower)
		{
			m_wipeTowerSelected = isSelectWipeTower;
			emit wipeTowerSelectedChanged();
		}

	}

	void ModelNSelector::selectChanged(qtuser_3d::Pickable* pickable)
	{
	}

	void ModelNSelector::selectRect(const QRect& rect, bool exclude)
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
					WipeTowerPickable* m = qobject_cast<WipeTowerPickable*>(obj);
					if (m == nullptr)
					{
						list << obj;
					}
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


	void ModelNSelector::selectGroup(qtuser_3d::Pickable* pickable)
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
		{
			for (Pickable* i : onList)
			{
				WipeTowerPickable* m = qobject_cast<WipeTowerPickable*>(i);
				if (m != nullptr)
				{
					offList.push_back(m);
					onList.removeOne(m);
				}
			}

			if (m_selectedPickables.isEmpty())
			{
				onList << pickable;
			}
			else {
				WipeTowerPickable* m = qobject_cast<WipeTowerPickable*>(pickable);
				if (!onList.isEmpty() && m != nullptr)
				{
				}
				else {
					onList << pickable;
				}
			}
		}

		selectPickables(onList, offList);
	
	}

	QList<QObject*> ModelNSelector::selectionmObjects()
	{
		QList<QObject*> models;
		QList<qtuser_3d::Pickable*> selections = selectionmPickables();
		for (Pickable* pickable : selections)
		{
			QObject* m = qobject_cast<QObject*>(pickable);
			if (m)
				models << m;
		}
		return models;
	}
}