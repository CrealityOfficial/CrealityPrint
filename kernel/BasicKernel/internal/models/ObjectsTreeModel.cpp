#include "BasicTreeModel.h"
#include "ObjectTreeItem.h"
#include "ObjectsTreeModel.h"
#include "data/modeln.h"
#include "external/data/modelgroup.h"
#include "interface/selectorinterface.h"
#include "TreeViewSelectionModel.h"
#include "kernel/kernel.h"

namespace creative_kernel
{
	ObjectsTreeModel::ObjectsTreeModel(QObject* parent)
	{
		m_TreeSelect = new TreeViewSelectionModel(this, this);
		QAbstractItemModel* im = m_TreeSelect->model();
	}

	ObjectsTreeModel::ObjectsTreeModel(const ObjectsTreeModel& model)
	{
	}

	ObjectsTreeModel::~ObjectsTreeModel()
	{

	}

	void ObjectsTreeModel::setColor(int index, QString color)
	{

	}

	void ObjectsTreeModel::checkOne(const QModelIndex& index)
	{
		ObjectTreeItem* item = static_cast<ObjectTreeItem*>(index.internalPointer());
		QVariant itemData = item->data(0);
		if (!item)
			return;
		switch (item->objectType())
		{
		case ObjectTreeItem::OT_PLATE:
			break;
		case ObjectTreeItem::OT_GROUP:
		{
			ModelGroup* mg = itemData.value<ModelGroup*>();
			selectGroup(mg);
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

	void ObjectsTreeModel::checkOne(const QVariant& index)
	{
		ModelN* mn = index.value<ModelN*>();
		selectModelN(mn);
	}

	QObject* ObjectsTreeModel::treeSelectModel()
	{
		return m_TreeSelect;
	}

	void ObjectsTreeModel::select(const QItemSelection& selection, QItemSelectionModel::SelectionFlags command)
	{
		m_TreeSelect->select(selection, command);
	}

	void ObjectsTreeModel::rightMenuType(QModelIndex index)
	{
		//updateCameraCenter();
		ObjectTreeItem* obj = static_cast<ObjectTreeItem*>(index.internalPointer());
		if (!obj)
			return;
		ModelN* model = nullptr;
		if (obj->objectType() == ObjectTreeItem::OT_GROUP)
		{
			ModelGroup* modelGroup = obj->data(0).value<ModelGroup*>();
			if (modelGroup->models().size() >= 1)
			{
				model = modelGroup->models().at(0);
			}
		}

		if (obj->objectType() == ObjectTreeItem::OT_MODEL)
		{
			model = obj->data(0).value<ModelN*>();
		}

		if (model)
		{
			auto models = selectionms();
			if (models.isEmpty())
				selectGroup(model->getModelGroup(), false);

			models = selectedParts();
			if (models.size() == 1)
			{
				ModelN* sModel = models.first();
				ModelNType type = sModel->modelNType();
				if (type == ModelNType::modifier || type == ModelNType::normal_part)
					getKernel()->setRightClickType(Kernel::ModifierPartMenu);
				else
					getKernel()->setRightClickType(Kernel::NormalPartMenu);
			}
			else
			{
				int count = selectedGroups().count();
				if (count == 1)
					getKernel()->setRightClickType(Kernel::SingleModelMenu);
				else
					getKernel()->setRightClickType(Kernel::MultiModelsMenu);
			}
		}
		else
		{
			getKernel()->setRightClickType(Kernel::NoModelMenu);
		}
	}

	TreeViewSelectionModel* ObjectsTreeModel::selectionModel()
	{
		return m_TreeSelect;
	}

	bool ObjectsTreeModel::moveRowsTo(const QModelIndex& sourceParent, int sourceFirst, int sourceLast, const QModelIndex& destinationParent, int destinationChild)
	{
		ObjectTreeItem* sourceParentItem = static_cast<ObjectTreeItem*>(sourceParent.internalPointer());
		ObjectTreeItem* destParentItem = static_cast<ObjectTreeItem*>(destinationParent.internalPointer());

		if (sourceParent == destinationParent)
		{
			if (destinationChild > sourceFirst)
			{
				destinationChild += sourceLast - sourceFirst + 1;
			}
			else {
				destinationChild += sourceLast - sourceFirst;
			}
			int sourceRow = sourceFirst;
			int count = sourceLast - sourceFirst + 1;

			if (sourceRow < 0
				|| sourceRow + count - 1 >= rowCount(sourceParent)
				|| destinationChild <= 0
				|| destinationChild > rowCount(destinationParent)
				|| sourceRow == destinationChild - 1
				|| count <= 0) {
				return false;
			}
			if (!beginMoveRows(sourceParent, sourceRow, sourceRow + count - 1, destinationParent, destinationChild))
				return false;

			if (destinationChild > sourceFirst)
			{
				destinationChild--;
			}
			//const int fromRow = destinationChild < sourceRow ? (sourceRow + count - 1) : sourceRow;

			int index = 0;
			for (; index < sourceLast - sourceFirst + 1; index++)
			{
				sourceParentItem->moveChild(sourceFirst + index, destinationChild + index);
			}

			endMoveRows();
			return true;
		}
		else {
			if (sourceLast < sourceFirst)
			{
				return false;
			}
			bool res = beginMoveRows(sourceParent, sourceFirst, sourceLast, destinationParent, destinationChild);
			ObjectTreeItem* sourceParentItem = static_cast<ObjectTreeItem*>(sourceParent.internalPointer());
			ObjectTreeItem* destParentItem = static_cast<ObjectTreeItem*>(destinationParent.internalPointer());
			int index = 0;
			for (; index < sourceLast - sourceFirst + 1; index++)
			{
				destParentItem->insert(destinationChild + index, sourceParentItem->child(index + sourceFirst));
				sourceParentItem->removeAtNoDel(index + sourceFirst);
			}
			endMoveRows();
			return true;
		}
	}
}