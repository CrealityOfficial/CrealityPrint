#include <QItemSelection>
#include <QModelIndex>
#include <QItemSelectionModel>
#include "ObjectsDataManager.h"
#include "internal/models/ObjectsTreeModel.h"
#include "internal/models/ObjectTreeItem.h"
#include "interface/selectorinterface.h"
#include "internal/multi_printer/printermanager.h"
#include "internal/multi_printer/printer.h"
#include "interface/spaceinterface.h"
#include "interface/printerinterface.h"
#include "external/data/modelgroup.h"
#include "kernel/kernel.h"
#include "data/modeln.h"
#include "internal/models/TreeViewSelectionModel.h"

namespace creative_kernel
{
    ObjectsDataManager::ObjectsDataManager(QObject* parent)
        :QObject(parent)
    {
        m_ObjectsTreeModel = new ObjectsTreeModel(this);

        QVector<QVariant> datas;
        datas.append(-1);
        appendChild(datas);
    }

    ObjectsDataManager::ObjectsDataManager(const ObjectsDataManager& other)
    {

    }

    ObjectsDataManager::~ObjectsDataManager()
    {

    }

    void ObjectsDataManager::setModelSpace(ModelSpace* model_space)
    {
        m_ModelSpace = m_ModelSpace;
    }

    void ObjectsDataManager::setModelSelector(ModelNSelector* model_selector)
    {
        m_ModelSelector = model_selector;
    }

    void ObjectsDataManager::setPrinterManager(PrinterMangager* printer_manager)
    {
        m_PrinterManager = printer_manager;
    }

    void ObjectsDataManager::addPlate(Printer* p)
    {
        QVector<QVariant> datas;
        datas.append(QVariant::fromValue(p));
        
        ObjectTreeItem* rootItem = m_ObjectsTreeModel->rootItem();
        ObjectTreeItem* item = new ObjectTreeItem(ObjectTreeItem::OT_PLATE, rootItem);
        item->setDatas(datas);

        m_ObjectsTreeModel->appendChild(item, QModelIndex());
    }

    bool ObjectsDataManager::isMoved(const QModelIndex& curParent, const QModelIndex& preParent)
    {
        if (!preParent.isValid())
            return false;

        bool moved = (curParent != preParent);
        return moved;
    }

    void ObjectsDataManager::moveModelGroups(const QModelIndex& curPrinterIndex, ModelGroup* mg)
    {
        if (!curPrinterIndex.isValid())
            return;

        ObjectTreeItem* printerItem = static_cast<ObjectTreeItem*>(curPrinterIndex.internalPointer());
        if (!printerItem)
            return;

        QModelIndex modelGroupIndex = m_ObjectsTreeModel->findModelIndex(QModelIndex(), 0, QVariant::fromValue(mg));
        QModelIndex preParentIndex = modelGroupIndex.parent();

        int sourceIndex = modelGroupIndex.row();
        int destIndex = printerItem->childCount();

        if (preParentIndex == curPrinterIndex)
            return; 

        m_ObjectsTreeModel->moveRowsTo(preParentIndex, sourceIndex, sourceIndex, curPrinterIndex, destIndex);
        emit sigExpandIndex(curPrinterIndex);
    }

    void ObjectsDataManager::insertPlate(Printer* p, int row)
    {
        QVector<QVariant> datas;
        datas.append(QVariant::fromValue(p));

        ObjectTreeItem* rootItem = m_ObjectsTreeModel->rootItem();
        ObjectTreeItem* item = new ObjectTreeItem(ObjectTreeItem::OT_PLATE, rootItem);
        item->setDatas(datas);

        row = rootItem->childCount() - 1;
        m_ObjectsTreeModel->insertChild(item, QModelIndex(), row);
    }

    void ObjectsDataManager::deletePlate(Printer* p)
    {
        QModelIndex printerIndex = m_ObjectsTreeModel->findModelIndex(QModelIndex(), 0, QVariant::fromValue(p));
        ObjectTreeItem* printerItem = static_cast<ObjectTreeItem*>(printerIndex.internalPointer());
        m_ObjectsTreeModel->removeChild(printerItem, QModelIndex());
    }

    QVariant ObjectsDataManager::objectsTreeModel()
    {
        return QVariant::fromValue(m_ObjectsTreeModel);
    }

    QList<ModelGroup*> ObjectsDataManager::generlatePrinterModelGroupList(Printer* printer) const
    {
        auto* root_item = m_ObjectsTreeModel->rootItem();
        if (!root_item)
        {
            return {};
        }

        QList<ModelGroup*> group_list{};
        for (int printer_index = 0; printer_index < root_item->childCount(); ++printer_index)
        {
            auto* printer_item = root_item->child(printer_index);
            if (!printer_item)
            {
                continue;
            }

            auto* temp_printer = static_cast<Printer*>(printer_item->data(0).value<QObject*>());
            if (printer != temp_printer)
            {
                continue;
            }

            for (int group_index = 0; group_index < printer_item->childCount(); ++group_index)
            {
                auto* group_item = printer_item->child(group_index);
                if (!group_item)
                {
                    continue;
                }

                auto* group = static_cast<ModelGroup*>(group_item->data(0).value<QObject*>());
                if (!group)
                {
                    continue;
                }

                group_list.push_back(group);
            }

            break;
        }

        return group_list;
    }

    void ObjectsDataManager::onModelGroupAdded(ModelGroup* _model_group)
    {
      
    }

    void ObjectsDataManager::onModelGroupRemoved(ModelGroup* _model_group)
    {
        QModelIndex groupIndex = m_ObjectsTreeModel->findModelIndex(QModelIndex(), 0, QVariant::fromValue(_model_group));
        if (!groupIndex.isValid())
        {
            return;
        }
        QModelIndex parentIndex = groupIndex.parent();

        deleteGroup(_model_group, parentIndex);
   }

    void ObjectsDataManager::onModelGroupModified(ModelGroup* _model_group, const QList<ModelN*>& removes, const QList<ModelN*>& adds)
    {
        Q_ASSERT(_model_group);
        QModelIndex parentIndex = m_ObjectsTreeModel->findModelIndex(QModelIndex(), 0, QVariant::fromValue(_model_group));
        if (!parentIndex.isValid())
            return;

        ObjectTreeItem* item = static_cast<ObjectTreeItem*>(parentIndex.internalPointer());
        if (!item)
            return;

        if (adds.size() > 0)
        {
            QList<ModelN*> models;
            if (_model_group->models().count() == 2)
            {
                models = _model_group->models();
            }
            else
            {
                models = adds;
            }

            for (auto model : models)
            {
                addModel(model, parentIndex);
            }
        }
        if(removes.size() > 0)
        {
            QList<ModelN*> models = removes;
            if (_model_group->models().count() == 1)
            {
                models << _model_group->models();
            }
          
            for (auto model : models)
            {   
                deleteModel(model, parentIndex);
            }
        }
    }

    void ObjectsDataManager::onModelGroupNameChanged(ModelGroup* _model_group)
    {

    }

    void ObjectsDataManager::onModelNameChanged(ModelN* model)
    {

    }

    void ObjectsDataManager::onModelTypeChanged(ModelN* model)
    {
        QModelIndex moveIndex = m_ObjectsTreeModel->findModelIndex(QModelIndex(), 0, QVariant::fromValue(model));
        QModelIndex parent = moveIndex.parent();

        int beginRow = moveIndex.row();
        int endRow = moveIndex.row();

        ObjectTreeItem* moveItem = static_cast<ObjectTreeItem*>(moveIndex.internalPointer());
        ObjectTreeItem* parentItem = static_cast<ObjectTreeItem*>(parent.internalPointer());
        if (!parentItem || !moveItem)
            return;
        int srcBeginIndex = parentItem->childIndex(moveItem);
        
        ModelN* modelN = moveItem->data(0).value<ModelN*>();
        ModelGroup* modelGroup = parentItem->data(0).value<ModelGroup*>();

        QList<ModelN*> models = modelGroup->models();
        int dstBeginIndex = models.indexOf(modelN);


        //for (auto model : models)
        //{
        //    deleteModel(model, parent);
        //}

        //for (auto model : models)
        //{
        //    addModel(model, parent);
        //}
        m_ObjectsTreeModel->moveRowsTo(parent, beginRow, endRow, parent, dstBeginIndex);
        //emit model->modelNTypeChanged();
    }

    void ObjectsDataManager::nameChanged(Printer* p, QString newName)
    {

    }

    void ObjectsDataManager::printerIndexChanged(Printer* p, int newIndex)
    {

    }

    void ObjectsDataManager::printersCountChanged(int count)
    {

    }

    void ObjectsDataManager::modelGroupsOutOfPrinterChange(const QList<ModelGroup*>& list)
    {
        QModelIndex printerIndex = m_ObjectsTreeModel->findModelIndex(QModelIndex(), 0, QVariant::fromValue(-1));
        if (!printerIndex.isValid())
           return;

        for (ModelGroup* mg : list)
        {
            QModelIndex modelGroupIndex = m_ObjectsTreeModel->findModelIndex(QModelIndex(), 0, QVariant::fromValue(mg));
            ObjectTreeItem* modelGroupIndexItem = static_cast<ObjectTreeItem*>(modelGroupIndex.internalPointer());

            QModelIndex preParentIndex = modelGroupIndex.parent();
            bool moved = isMoved(printerIndex, preParentIndex);
            if (moved)
            {
                moveModelGroups(printerIndex, mg);
            }
            else {
                if (preParentIndex.isValid())
                {
                    continue;
                }

                QModelIndex addGroupIndex = m_ObjectsTreeModel->findModelIndex(printerIndex, 0, QVariant::fromValue(mg));
                if (!addGroupIndex.isValid())
                {
                    addGroup(mg, printerIndex);
                }
            }
        }
    }

    void ObjectsDataManager::didAddPrinter(Printer* p)
    {
        //addPlate(p);
        insertPlate(p);
    }

    void ObjectsDataManager::didDeletePrinter(Printer* p)
    {
        deletePlate(p);
    }

    void ObjectsDataManager::didSelectPrinter(Printer* p)
    {

    }

    void ObjectsDataManager::printerNameChanged(Printer* p, QString newName)
    {

    }

    void ObjectsDataManager::printerModelGroupsInsideChange(Printer* pt, const QList<ModelGroup*>& list)
    {
        Q_ASSERT(pt);
        QModelIndex curPrinterIndex = m_ObjectsTreeModel->findModelIndex(QModelIndex(), 0, QVariant::fromValue(pt));
        if (!curPrinterIndex.isValid())
            return;

        ObjectTreeItem* printerItem = static_cast<ObjectTreeItem*>(curPrinterIndex.internalPointer());
        if (!printerItem)
            return;

        for (ModelGroup* mg : list)
        {
            QModelIndex modelGroupIndex = m_ObjectsTreeModel->findModelIndex(QModelIndex(), 0, QVariant::fromValue(mg));
            ObjectTreeItem* modelGroupIndexItem = static_cast<ObjectTreeItem*>(modelGroupIndex.internalPointer());
            
            QModelIndex preParentIndex = modelGroupIndex.parent();
            bool moved = isMoved(curPrinterIndex, preParentIndex);
            if (moved)
            {
                moveModelGroups(curPrinterIndex, mg);
            }
            else {
                if (preParentIndex.isValid())
                {
                    continue;
                }

                QModelIndex addGroupIndex = m_ObjectsTreeModel->findModelIndex(curPrinterIndex, 0, QVariant::fromValue(mg));
                if (!addGroupIndex.isValid())
                {
                    addGroup(mg, curPrinterIndex);
                }
            }
        }
    }

    void ObjectsDataManager::addGroup(ModelGroup* mg, const QModelIndex& parent)
    {
        Q_ASSERT(mg);
        ObjectTreeItem* parentItem = static_cast<ObjectTreeItem *>(parent.internalPointer());
        Q_ASSERT(parentItem);
        ObjectTreeItem* groupItem = new ObjectTreeItem(ObjectTreeItem::OT_GROUP, parentItem);
        QVector<QVariant> datas;
        datas << QVariant::fromValue(mg);
        groupItem->setDatas(datas);

        QVector<ObjectTreeItem*> groups;
        groups << groupItem;

        m_ObjectsTreeModel->appendChild(groupItem, parent);

        QList<ModelN*> models = mg->models();
        QModelIndex modelGroupIndex = m_ObjectsTreeModel->findModelIndex(QModelIndex(), 0, groupItem);

        if (models.count() > 1)
        {
            for (ModelN* model : models)
            {
                addModel(model, modelGroupIndex);
            }
        }

        emit sigExpandIndex(parent);
    }

    void ObjectsDataManager::deleteGroup(ModelGroup* mg, const QModelIndex& parent)
    {
        Q_ASSERT(mg);
        ObjectTreeItem* parentItem = static_cast<ObjectTreeItem *>(parent.internalPointer());
        QVector<ObjectTreeItem*> items = parentItem->children();

        for (ObjectTreeItem* groupItem : items)
        {
            Q_ASSERT(groupItem);
            ModelGroup* value = groupItem->itemData().value<ModelGroup*>();
            if (value == mg)
            {
                m_ObjectsTreeModel->removeChild(groupItem, parent);
                return;
            }
        }
    }

    void ObjectsDataManager::addModel(ModelN* mn, const QModelIndex& parent)
    {
        ObjectTreeItem* parentItem = static_cast<ObjectTreeItem*>(parent.internalPointer());
        Q_ASSERT(mn&& parentItem);
        QVector<QVariant> datas;
        datas << QVariant::fromValue(mn);

        ObjectTreeItem* modelItem = new ObjectTreeItem(ObjectTreeItem::OT_MODEL, parentItem);
        modelItem->setDatas(datas);

        ModelGroup* mg = parentItem->data(0).value<ModelGroup*>();
        QList<ModelN*> models = mg->models();
        int dstIndex = models.indexOf(mn);

        QVector<ObjectTreeItem*> items = parentItem->children();
        int row = 0;
        for (auto item : items)
        {
            ModelN* sourceMN = item->data(0).value<ModelN*>();
            if (!sourceMN)
                continue;

            int sourceIndex = models.indexOf(sourceMN);
            if (sourceIndex > dstIndex)
            {
                break;
            }
            row++;
        }

        emit sigExpandIndex(parent);
        m_ObjectsTreeModel->insertChild(modelItem, parent, row);
    }

    void ObjectsDataManager::deleteModel(ModelN* mn, const QModelIndex& parent)
    {
        Q_ASSERT(mn);
        ObjectTreeItem* parentItem = static_cast<ObjectTreeItem*>(parent.internalPointer());
        if (!parentItem)
            return;
        QModelIndex modelIndex = m_ObjectsTreeModel->findModelIndex(QModelIndex(), 0, QVariant::fromValue(mn));
        if (!modelIndex.isValid())
            return;

        ObjectTreeItem* modelItem = static_cast<ObjectTreeItem*>(modelIndex.internalPointer());
        m_ObjectsTreeModel->removeChild(modelItem, parent);
    }

    void ObjectsDataManager::appendChild(const QVector<QVariant>& datas)
    {
        ObjectTreeItem* rootItem = m_ObjectsTreeModel->rootItem();
        ObjectTreeItem* item = new ObjectTreeItem(ObjectTreeItem::OT_PLATE, rootItem);
        item->setDatas(datas);

        m_ObjectsTreeModel->appendChild(item, QModelIndex());
    }

    void ObjectsDataManager::onSelectionsChanged()
    {
        TreeViewSelectionModel* ts = m_ObjectsTreeModel->selectionModel();
        bool ms = ts->isSceneModels();
        if (!ms)
            return;

        TreeViewSelectionModel* tModel = m_ObjectsTreeModel->selectionModel();
        bool isManual = tModel->isManualSelection();

        QList<ModelN*> selectModels = selectedParts();
        QList<ModelGroup*> groups = selectedGroups();

        Printer* printer = selectedPlate();

        QItemSelection modelsSelection;
        QItemSelection gourpsSelection;
        QItemSelection printersSelection;

        for (ModelN* model : selectModels)
        {
            QModelIndex modelIndex = m_ObjectsTreeModel->findModelIndex(QModelIndex(), 0, QVariant::fromValue(model));
            modelsSelection.select(modelIndex, modelIndex);

            QModelIndex parentIndex = modelIndex.parent();
            if (parentIndex.isValid())
            {
                emit sigExpandIndex(parentIndex);
            }
        }

        for (ModelGroup* group : groups)
        {
            QModelIndex groupIndex = m_ObjectsTreeModel->findModelIndex(QModelIndex(), 0, QVariant::fromValue(group));
            gourpsSelection.select(groupIndex, groupIndex);

            QModelIndex parentIndex = groupIndex.parent();
            if (parentIndex.isValid())
            {
                emit sigExpandIndex(parentIndex);
            }
        }

        if (printer && selectModels.size() == 0 && groups.size() == 0)
        {
            QModelIndex printerIndex = m_ObjectsTreeModel->findModelIndex(QModelIndex(), 0, QVariant::fromValue(printer));
            printersSelection.select(printerIndex, printerIndex);
        }

        QItemSelection selects;
        selects.merge(modelsSelection, QItemSelectionModel::Toggle);
        selects.merge(gourpsSelection, QItemSelectionModel::Toggle);
        selects.merge(printersSelection, QItemSelectionModel::Toggle);

        m_ObjectsTreeModel->select(selects, QItemSelectionModel::ClearAndSelect);

        QModelIndexList indexList = selects.indexes();
        if (indexList.isEmpty())
            return;
        const QModelIndex& lastIndex = indexList.last();
        emit ts->sigSelectionChanged(lastIndex);
    }

    void ObjectsDataManager::onSelectionsObjectsChanged(const QList<Printer*>& printersOn, const QList<Printer*>& printersOff,
        const QList<ModelGroup*>& modelGroupsOn, const QList<ModelGroup*>& modelGroupsOff,
        const QList<ModelN*>& modelsOn, const QList<ModelN*>& modelsOff)
    {
        QItemSelection selects;
        QItemSelection printersSelects;
        QItemSelection printersUnSelects;
        QItemSelection modelGroupsSelects;
        QItemSelection modelGroupsUnSelects;
        QItemSelection modelNsSelects;
        QItemSelection modelNsUnSelects;

        for (const Printer* printer : printersOn)
        {
            QModelIndex index = m_ObjectsTreeModel->findModelIndex(QModelIndex(), 0, printer);
            printersSelects.select(index, index);
        }

        for (const Printer* printer : printersOff)
        {
            QModelIndex index = m_ObjectsTreeModel->findModelIndex(QModelIndex(), 0, printer);
            printersUnSelects.select(index, index);
        }
        printersSelects.merge(printersUnSelects, QItemSelectionModel::Deselect);
        selects.merge(printersSelects, QItemSelectionModel::Toggle);

        for (const ModelGroup* modelGroup : modelGroupsOn)
        {
            QModelIndex index = m_ObjectsTreeModel->findModelIndex(QModelIndex(), 0, modelGroup);
            modelGroupsSelects.select(index, index);
        }

        for (const ModelGroup* modelGroup : modelGroupsOff)
        {
            QModelIndex index = m_ObjectsTreeModel->findModelIndex(QModelIndex(), 0, modelGroup);
            modelGroupsUnSelects.select(index, index);
        }
        modelGroupsSelects.merge(modelGroupsUnSelects, QItemSelectionModel::Deselect);
        selects.merge(modelGroupsSelects, QItemSelectionModel::Toggle);

        for (const ModelN* model : modelsOn)
        {
            QModelIndex index = m_ObjectsTreeModel->findModelIndex(QModelIndex(), 0, model);
            modelNsSelects.select(index, index);
        }

        for (const ModelN* model : modelsOff)
        {
            QModelIndex index = m_ObjectsTreeModel->findModelIndex(QModelIndex(), 0, model);
            modelNsUnSelects.select(index, index);
        }
        modelNsSelects.merge(modelNsUnSelects, QItemSelectionModel::Deselect);
        selects.merge(modelNsSelects, QItemSelectionModel::Toggle);

        m_ObjectsTreeModel->select(selects, QItemSelectionModel::Toggle);
    }
}
