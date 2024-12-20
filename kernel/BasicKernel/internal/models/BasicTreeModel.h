#pragma once
#include <QAbstractItemModel>
#include <QSet>
#include <QVector>
#include <QItemSelection>

namespace creative_kernel
{
    class ObjectTreeItem;
    class BasicTreeModel : public QAbstractItemModel
    {
        Q_OBJECT
    public:
        enum DataType {
            DT_PLATE,
            DT_GROUP,
            DT_MODEL
        };
        enum RoleType {
            RT_DATA = Qt::UserRole,
            RT_DATATYPE
        };

        explicit BasicTreeModel(QObject* parent = nullptr);
        BasicTreeModel(const BasicTreeModel& model) {}
        ~BasicTreeModel();
        ObjectTreeItem* rootItem() const;

        void appendChild(ObjectTreeItem* data, const QModelIndex& parent);
        void insertChild(ObjectTreeItem* data, const QModelIndex& parent, int row = 0);
        void removeChild(ObjectTreeItem* data, const QModelIndex& parent);
        void appendChildren(const QVector<ObjectTreeItem*>& datas);
        void prependChild(ObjectTreeItem* data);
        void insert(int row, const QVector<ObjectTreeItem*>& datas);
        void clear();
        void removeAt(int row);
        void refresh();

        QModelIndex findModelIndex(const QModelIndex& parent, 
            int role, const QVariant& value);

        QModelIndex findModelIndex(const QModelIndex& parent,
            int role, ObjectTreeItem* data);

        Q_INVOKABLE int findModelIndexTotalRow(const QItemSelection& indexes);
        Q_INVOKABLE int findModelIndexTotalRow(const QModelIndex& index);

        Q_INVOKABLE void setTreeView(QObject* treeView);
    protected:
        QVariant data(const QModelIndex& index, int role) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
        QModelIndex parent(const QModelIndex& index) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QHash<int, QByteArray> roleNames() const override;

    protected:
        bool findModelIndexTotalRow(const QModelIndex& index, const QModelIndex& parent, int& totalRow);
        ObjectTreeItem* m_rootItem = nullptr;
        QObject* m_TreeView = nullptr;
    };
}
