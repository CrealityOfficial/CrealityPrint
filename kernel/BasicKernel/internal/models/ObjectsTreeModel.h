#pragma once
#include <QAbstractItemModel>
#include <QSet>
#include <QVector>
#include <QItemSelectionModel>
#include "BasicTreeModel.h"

namespace creative_kernel
{
    class ModelN;
    class BasicTreeModel;
    class TreeViewSelectionModel;
    class ObjectsTreeModel : public BasicTreeModel
    {
        Q_OBJECT
        Q_PROPERTY(QObject* treeSelectModel READ treeSelectModel CONSTANT)
    public:
        explicit ObjectsTreeModel(QObject* parent = nullptr);
        ObjectsTreeModel(const ObjectsTreeModel& model);
        ~ObjectsTreeModel();

        Q_INVOKABLE void setColor(int index, QString color);
        Q_INVOKABLE void checkOne(const QModelIndex& index);
        Q_INVOKABLE void checkOne(const QVariant& index);
        Q_INVOKABLE void rightMenuType(QModelIndex index);

        TreeViewSelectionModel* selectionModel();

        bool moveRowsTo(const QModelIndex& sourceParent, int sourceFirst, int sourceLast, const QModelIndex& destinationParent, int destinationChild);
        QObject* treeSelectModel();
        void select(const QItemSelection& selection, QItemSelectionModel::SelectionFlags command);

    protected:
        //int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    private:
        TreeViewSelectionModel* m_TreeSelect = nullptr;
    };
}
