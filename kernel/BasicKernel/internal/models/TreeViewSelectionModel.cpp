#include <QModelIndex>
#include <QModelIndexList>
#include "TreeViewSelectionModel.h"
#include "ObjectTreeItem.h"
#include "external/data/modelgroup.h"
#include "external/data/modeln.h"
#include "interface/selectorinterface.h"
#include "external/interface/printerinterface.h"
#include "kernel/kernel.h"
#include "kernel/reuseablecache.h"
#include "internal/multi_printer/printermanager.h"
#include "ObjectsTreeModel.h"

namespace creative_kernel
{
    TreeViewSelectionModel::TreeViewSelectionModel(QAbstractItemModel* model, QObject* parent):
        QItemSelectionModel(model, parent)
    {
        connect(this, &TreeViewSelectionModel::selectionChanged, this, &TreeViewSelectionModel::onSelectionChanged);
		connect(this, &TreeViewSelectionModel::currentChanged, this, &TreeViewSelectionModel::onCurrentChanged);
	}

    TreeViewSelectionModel::~TreeViewSelectionModel()
    {

    }

	bool TreeViewSelectionModel::isManualSelection()
	{
		return m_IsManualCurrent;
	}

	bool TreeViewSelectionModel::isSceneModels()
	{
		return m_IsSceneModels;
	}

	void TreeViewSelectionModel::setIsSceneModels(bool isScene)
	{
		m_IsSceneModels = isScene;
	}

	void TreeViewSelectionModel::checkOne(const QModelIndex& index)
	{
		ObjectTreeItem* item = static_cast<ObjectTreeItem*>(index.internalPointer());
		QVariant itemData = item->data(0);
		if (!item)
			return;
		switch (item->objectType())
		{
		case ObjectTreeItem::OT_PLATE:
		{
			Printer* pt = itemData.value<Printer*>();
			selectPrinter(pt);
			break;
		}
		case ObjectTreeItem::OT_GROUP:
		{
			ModelGroup* mg = itemData.value<ModelGroup*>();
			selectGroup(mg, true);
			break;
		}
		case ObjectTreeItem::OT_MODEL:
		{
			ModelN* mn = itemData.value<ModelN*>();
			selectModelN(mn, true);
			break;
		}
		default:
			break;
		}
	}

	void TreeViewSelectionModel::unCheck(const QModelIndexList& indexes)
	{
		QList<ModelGroup*> groups;
		QList<ModelN*> models;

		ObjectTreeItem* item;
		for (auto index : indexes)
		{
			item = static_cast<ObjectTreeItem*>(index.internalPointer());
			item->setChecked(false);
			QVariant itemData = item->data(0);

			if (item->objectType() == ObjectTreeItem::OT_PLATE)
			{

			}
			else if (item->objectType() == ObjectTreeItem::OT_GROUP)
			{
				ModelGroup* mg = itemData.value<ModelGroup*>();
				groups << mg;
			}
			else if (item->objectType() == ObjectTreeItem::OT_MODEL)
			{
				ModelN* mn = itemData.value<ModelN*>();
				models << mn;
			}
		}

		if (groups.size() > 0)
		{
			unselectGroups(groups);
		}

		if (models.size() > 0)
		{
			QList<ModelN*> selectModels = selectedParts();
			for (ModelN* model : models)
			{
				if (!selectModels.contains(model))
					models.removeOne(model);
			}
			unselectModels(models);
		}
	}

	void TreeViewSelectionModel::unCheckOne(const QModelIndex& index)
	{

	}

	void TreeViewSelectionModel::emitSelectionChanged(const QItemSelection& newSelection, const QItemSelection& oldSelection)
	{

	}

	void TreeViewSelectionModel::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
	{
		//if (m_IsManualSelection)
		//	return;

		QItemSelection sln = selection();
		QModelIndexList list = selectedIndexes();
		QModelIndexList selectedML = selected.indexes();
		QModelIndexList deSelectedML = deselected.indexes();
		QModelIndex curIndex = currentIndex();

		QModelIndexList slnIndexs = sln.indexes();
		QModelIndex firstIndex;
		QModelIndex lastIndex;
		if(slnIndexs.size() > 0)
			firstIndex = slnIndexs.first();
		if (selectedML.size() > 0)
			lastIndex = selectedML.last();

		QItemSelection preSelection = sln;
		preSelection.merge(selected, QItemSelectionModel::Deselect);
		preSelection.merge(deselected, QItemSelectionModel::Select);

		//选择空盘
		if (selectedML.size() > 0)
		{
			ObjectTreeItem* obj = static_cast<ObjectTreeItem*>(selectedML.at(0).internalPointer());
			if (obj->data(0) == -1)
			{
				QItemSelection itemSelection;
				return;
			}
		}

		if (list.size() > 0)
		{
			//设置当前选中
			m_IsManualCurrent = true;
			setCurrentIndex(lastIndex, QItemSelectionModel::Current);
			m_IsManualCurrent = false;
		}
		if (list.size() > 1)
		{
			//1.判断是否同类型
			bool sameType = isSameType(firstIndex, lastIndex);

			//2.判断是否同组别
			bool sameGroup = isSameGroup(firstIndex, lastIndex);

			if (!m_IsSameType)
			{
				emit sigSelectionError(preSelection, SE_DIFFERENTTYPE);
				//m_IsSameType = true;
				m_IsManualSelection = true;
				select(selected, QItemSelectionModel::Deselect);
				m_IsManualSelection = false;
			}
			else if (!m_IsSameGroup)
			{
				emit sigSelectionError(preSelection, SE_DIFFERENTGROUP);
				//m_IsSameGroup = true;
				m_IsManualSelection = true;
				select(selected, QItemSelectionModel::Deselect);
				m_IsManualSelection = false;
			}
		}

		if (list.size() == 1)
		{
			m_IsSameType = true;
			m_IsSameGroup = true;
		}

		if (!m_IsManualSelection && m_IsSameType && m_IsSameGroup)
		{
			m_IsSceneModels = false;
			unCheck(deSelectedML);

			for (const QModelIndex& index : selectedML)
			{
				ObjectTreeItem* obj = static_cast<ObjectTreeItem*>(index.internalPointer());
				obj->setChecked(true);
				checkOne(index);
			}
			m_IsSceneModels = true;
		}
	}

	void TreeViewSelectionModel::onCurrentChanged(const QModelIndex& current, const QModelIndex& previous)
	{
		/*if (m_IsManualCurrent)
			return;

		ObjectTreeItem* currentItem = static_cast<ObjectTreeItem*>(current.internalPointer());
		ObjectTreeItem* previousItem = static_cast<ObjectTreeItem*>(previous.internalPointer());
		if (!currentItem || !previousItem)
		{
			return;
		}
		ObjectTreeItem::ObjType curType = currentItem->objectType();
		ObjectTreeItem::ObjType preType = previousItem->objectType();
		if (preType != curType)
		{
			m_IsSameType = false;
			m_IsManualCurrent = true;
			setCurrentIndex(previous, QItemSelectionModel::Current);
			m_IsManualCurrent = false;
		}
		else if (currentItem->parentItem() != previousItem->parentItem())
		{
			m_IsSameGroup = false;
			m_IsManualCurrent = true;
			setCurrentIndex(previous, QItemSelectionModel::Current);
			m_IsManualCurrent = false;
		}*/
	}

	bool TreeViewSelectionModel::isSameGroup(const QModelIndex& index1, const QModelIndex& index2)
	{
		if (!index1.isValid() || !index2.isValid())
		{
			m_IsSameGroup = true;
			return m_IsSameGroup;
		}
			
		ObjectTreeItem* currentItem = static_cast<ObjectTreeItem*>(index2.internalPointer());
		ObjectTreeItem* previousItem = static_cast<ObjectTreeItem*>(index1.internalPointer());
		if (!currentItem || !previousItem)
		{
			m_IsSameGroup = false;
			return m_IsSameGroup;
		}

		if (currentItem->objectType() == ObjectTreeItem::OT_GROUP
			&& previousItem->objectType() == ObjectTreeItem::OT_GROUP)
		{//模型组类型可以忽略不同盘
			m_IsSameGroup = true;
			return m_IsSameGroup;
		}

		if (currentItem->parentItem() != previousItem->parentItem())
		{
			m_IsSameGroup = false;
		}
		else 
		{
			m_IsSameGroup = true;
		}

		return m_IsSameGroup;
	}

	bool TreeViewSelectionModel::isSameType(const QModelIndex& index1, const QModelIndex& index2)
	{
		if (!index1.isValid() || !index2.isValid())
			return true;

		ObjectTreeItem* currentItem = static_cast<ObjectTreeItem*>(index2.internalPointer());
		ObjectTreeItem* previousItem = static_cast<ObjectTreeItem*>(index1.internalPointer());
		if (!currentItem || !previousItem)
		{
			return false;
		}
		ObjectTreeItem::ObjType curType = currentItem->objectType();
		ObjectTreeItem::ObjType preType = previousItem->objectType();
		if (preType != curType)
		{
			m_IsSameType = false;
		}
		else {
			m_IsSameType = true;
		}

		return m_IsSameType;
	}

	QString TreeViewSelectionModel::getModelName(ObjectTreeItem* obj)
	{
		ObjectTreeItem::ObjType preType1 = obj->objectType();
		if (preType1 = ObjectTreeItem::OT_MODEL)
		{
			ModelN* mn = obj->data(0).value<ModelN*>();
			if (!mn)
				return QString();
			QString name = mn->name();
			return name;
		}
		return QString();
	}

}