#pragma once
#include <QItemSelectionModel>
#include <QSet>
#include <QVector>

namespace creative_kernel
{
    class ObjectTreeItem;
    class TreeViewSelectionModel : public QItemSelectionModel 
    {
        Q_OBJECT
    public:
        enum SelectionError 
        {
            SE_DIFFERENTTYPE,
            SE_DIFFERENTGROUP
        };

        TreeViewSelectionModel(QAbstractItemModel* model = nullptr, QObject* parent = nullptr);
        ~TreeViewSelectionModel();

        bool isManualSelection();
        bool isSceneModels();
        void setIsSceneModels(bool isScene);
    private:
        void checkOne(const QModelIndex& index);
        void unCheck(const QModelIndexList& indexes);
        void unCheckOne(const QModelIndex& index);
        void emitSelectionChanged(const QItemSelection& newSelection, const QItemSelection& oldSelection);

    public slots:
       void onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
       void onCurrentChanged(const QModelIndex& current, const QModelIndex& previous);

    private:
        bool isSameGroup(const QModelIndex& index1, const QModelIndex& index2);
        bool isSameType(const QModelIndex& index1, const QModelIndex& index2);
        QString getModelName(ObjectTreeItem* obj);
    signals:
        void sigSelectionError(QItemSelection selectModelIndexs, int errCode);
        void sigSelectionChanged(const QModelIndex& lastIndex);//0: µã»÷ 1:select½Ó¿Ú

    public:
        bool m_IsManualSelection = false;
        bool m_IsManualCurrent = false;
        bool m_IsSceneModels = true;
        bool m_IsSameType = true;
        bool m_IsSameGroup = true;
    };
}
