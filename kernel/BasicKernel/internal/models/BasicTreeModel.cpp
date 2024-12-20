#include "BasicTreeModel.h"
#include "ObjectTreeItem.h"
#include "kernel/kernelui.h"
#include "external/data/modelgroup.h"

namespace creative_kernel
{
    BasicTreeModel::BasicTreeModel(QObject* parent)
        : QAbstractItemModel(parent)
    {
        m_rootItem = new ObjectTreeItem(ObjectTreeItem::OT_ROOT, nullptr);
        QVector<QVariant> datas;
        datas.append("");
        datas.append("");
        m_rootItem->setDatas(datas);
    }

    BasicTreeModel::~BasicTreeModel()
    {
        delete m_rootItem;
    }

    ObjectTreeItem* BasicTreeModel::rootItem() const
    {
        return m_rootItem;
    }

    QVariant BasicTreeModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
            return QVariant();

  /*      if (role != Qt::DisplayRole)
            return QVariant();*/

        ObjectTreeItem* item = static_cast<ObjectTreeItem*>(index.internalPointer());
        return QVariant::fromValue(item);
        //switch (role)
        //{
        //case RT_DATATYPE:
        //    return item->objectType();
        //    break;
        //case RT_DATA:
        //    return QVariant::fromValue(item);
        //    break;
        //default:
        //    break;
        //}
    }

    Qt::ItemFlags BasicTreeModel::flags(const QModelIndex& index) const
    {
        if (!index.isValid())
            return Qt::NoItemFlags;

        return QAbstractItemModel::flags(index);
    }

    QVariant BasicTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
            return m_rootItem->data(section);

        return QVariant();
    }

    QModelIndex BasicTreeModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (!hasIndex(row, column, parent))
            return QModelIndex();

        ObjectTreeItem* parentItem;

        if (!parent.isValid())
            parentItem = m_rootItem;
        else
            parentItem = static_cast<ObjectTreeItem*>(parent.internalPointer());

        ObjectTreeItem* childItem = parentItem->child(row);
        if (childItem)
            return createIndex(row, column, childItem);
        return QModelIndex();
    }

    QModelIndex BasicTreeModel::parent(const QModelIndex& index) const
    {
        if (!index.isValid())
            return QModelIndex();

        ObjectTreeItem* childItem = static_cast<ObjectTreeItem*>(index.internalPointer());
        ObjectTreeItem* parentItem = childItem->parentItem();

        if (parentItem == m_rootItem)
            return QModelIndex();

        return createIndex(parentItem->row(), 0, parentItem);
    }

    int BasicTreeModel::rowCount(const QModelIndex& parent) const
    {
        ObjectTreeItem* parentItem;
        if (parent.column() > 0)
            return 0;

        if (!parent.isValid())
            parentItem = m_rootItem;
        else
            parentItem = static_cast<ObjectTreeItem*>(parent.internalPointer());

        return parentItem->childCount();
    }

    int BasicTreeModel::columnCount(const QModelIndex& parent) const
    {
        if (parent.isValid())
            return static_cast<ObjectTreeItem*>(parent.internalPointer())->columnCount();
        return m_rootItem->columnCount();
    }

    QHash<int, QByteArray> BasicTreeModel::roleNames() const
    {
        QHash<int, QByteArray> hash;
        hash[RT_DATATYPE] = "itemDataType";
        hash[RT_DATA] = "itemData";

        return hash;
    }

    void BasicTreeModel::appendChild(ObjectTreeItem* data, const QModelIndex& parent)
    {
        ObjectTreeItem* parentItem = static_cast<ObjectTreeItem*>(parent.internalPointer());
        QModelIndex parentIndex = parent;

        if (!parent.isValid())
        {
            parentItem = m_rootItem;
            parentIndex = QModelIndex();
        }
            
        int childCount = parentItem->childCount();
        beginInsertRows(parentIndex, childCount, childCount);
        parentItem->appendChild(data);
        endInsertRows();
    }

    void BasicTreeModel::insertChild(ObjectTreeItem* data, const QModelIndex& parent, int row)
    {
        ObjectTreeItem* parentItem = static_cast<ObjectTreeItem*>(parent.internalPointer());
        QModelIndex parentIndex = parent;

        if (!parent.isValid())
        {
            parentItem = m_rootItem;
            parentIndex = QModelIndex();
        }

        beginInsertRows(parentIndex, row, row);
        parentItem->insert(row, data);
        endInsertRows();
    }

    void BasicTreeModel::removeChild(ObjectTreeItem* data, const QModelIndex& parent)
    {
        ObjectTreeItem* parentItem = static_cast<ObjectTreeItem*>(parent.internalPointer());
        QModelIndex parentIndex = parent;

        if (!parent.isValid())
        {
            parentItem = m_rootItem;
            parentIndex = QModelIndex();
        }

        int childRow = parentItem->childIndex(data);
        if (childRow == -1)
            return;

        beginRemoveRows(parentIndex, childRow, childRow);
        parentItem->remove(data);
        endRemoveRows();
    }

    void BasicTreeModel::appendChildren(const QVector<ObjectTreeItem*>& datas)
    {
        beginInsertRows({}, m_rootItem->childCount(), m_rootItem->childCount() + datas.size() - 1);
        m_rootItem->appendChildren(datas);
        endInsertRows();
    }
    void BasicTreeModel::prependChild(ObjectTreeItem* data)
    {
        beginInsertRows({}, 0, 0);
        m_rootItem->prependChild(data);
        endInsertRows();
    }
    void BasicTreeModel::insert(int row, const QVector<ObjectTreeItem*>& datas)
    {
        if (row < 0 || row > m_rootItem->childCount())
        {
            return;
        }
        beginInsertRows({}, m_rootItem->childCount(), m_rootItem->childCount() + datas.size() - 1);
        m_rootItem->insert(row, datas);
        endInsertRows();
    }

    void BasicTreeModel::clear()
    {
        if (m_rootItem->childCount() > 0)
        {
            beginRemoveRows({}, 0, m_rootItem->childCount() - 1);
            m_rootItem->clear();
            endRemoveRows();
        }
    }
    void BasicTreeModel::removeAt(int row)
    {
        if (row < 0 || row >= m_rootItem->childCount())
        {
            return;
        }
        beginRemoveRows({}, row, row);
        m_rootItem->removeAt(row);
        endRemoveRows();
    }

    void BasicTreeModel::refresh()
    {
        beginResetModel();
        endResetModel();
    }

    QModelIndex BasicTreeModel::findModelIndex(const QModelIndex& parent, int role, const QVariant& value) 
    {
        int rowCount = this->rowCount(parent);
        int columnCount = this->columnCount(parent);

        for (int row = 0; row < rowCount; ++row) {
            //for (int column = 0; column < columnCount; ++column) {
                QModelIndex index = this->index(row, 0, parent);
                ObjectTreeItem* item = static_cast<ObjectTreeItem*>(index.internalPointer());
                //ObjectTreeItem* item = this->data(index, role).value<ObjectTreeItem*>();
                Q_ASSERT(item);
                QVariant temp = item->data(0);
                ModelGroup*  mg = temp.value<ModelGroup*>();
                QString gname;
                if (mg)
                {
                    gname = mg->groupName();
                }
                if (item->data(0) == value) {
                    return index;
                }
                int rowCount = index.row();
                if (this->hasChildren(index)) {
                    QModelIndex result = findModelIndex(index, role, value);
                    if (result.isValid()) {
                        return result;
                    }
                }
            //}
        }

        return QModelIndex();
    }

    QModelIndex BasicTreeModel::findModelIndex(const QModelIndex& parent,
        int role, ObjectTreeItem* data)
    {
        int rowCount = this->rowCount(parent);
        int columnCount = this->columnCount(parent);

        for (int row = 0; row < rowCount; ++row) {
            for (int column = 0; column < columnCount; ++column) {
                QModelIndex index = this->index(row, column, parent);
                ObjectTreeItem* item = this->data(index, role).value<ObjectTreeItem*>();
                Q_ASSERT(item);
                if (item == data) {
                    return index;
                }

                if (this->hasChildren(index)) {
                    QModelIndex result = findModelIndex(index, role, data);
                    if (result.isValid()) {
                        return result;
                    }
                }
            }
        }

        return QModelIndex();
    }

    int BasicTreeModel::findModelIndexTotalRow(const QItemSelection& indexes)
    {
        QModelIndexList list = indexes.indexes();
        if (list.size() == 0)
            return 0;
        QModelIndex lastIndex = list.last();
        int totalRow = 0;
        findModelIndexTotalRow(lastIndex, QModelIndex(), totalRow);
        return totalRow;
    }

    int BasicTreeModel::findModelIndexTotalRow(const QModelIndex& index)
    {
        int totalRow = 0;
        findModelIndexTotalRow(index, QModelIndex(), totalRow);
        return totalRow;
    }

    bool BasicTreeModel::findModelIndexTotalRow(const QModelIndex& curIndex, const QModelIndex& parent, int& totalRow)
    {
        if (!curIndex.isValid())
            return false;

        int rowCount = this->rowCount(parent);
        int columnCount = this->columnCount(parent);

        for (int row = 0; row < rowCount; ++row)
        {
            ++totalRow;

            QModelIndex index = this->index(row, 0, parent);
            if (index == curIndex)
            {
                return true;
            }


            QObject* pinfoshowObj = getKernelUI()->getUI("uiappwindow")->findChild<QObject*>("cusTreeViewObj");
            QVariant isExpandedRes = false;
            bool res = QMetaObject::invokeMethod(m_TreeView, "isIndexExpand",
                Q_RETURN_ARG(QVariant, isExpandedRes),
                Q_ARG(QVariant, index));

            if (this->hasChildren(index) && isExpandedRes.value<bool>()) {
                if (findModelIndexTotalRow(curIndex, index, totalRow)) {
                    return true;
                }
            }
        }

        return false;
    }

    void BasicTreeModel::setTreeView(QObject* treeView)
    {
        if (!treeView || m_TreeView == treeView)
        {
            return;
        }

        m_TreeView = treeView;
    }
}