#ifndef CUSMODELLISTMODEL_H
#define CUSMODELLISTMODEL_H
#include "qtuser3d/module/pickableselecttracer.h"
#include <QtCore/QAbstractListModel>

namespace creative_kernel
{
    class ModelN;
    class ModelInfo : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(bool isChosed READ isChosed NOTIFY isChosedChanged)
        Q_PROPERTY(bool isVisible READ isVisible NOTIFY isVisibleChanged)
    public:
        ModelInfo(ModelN* model, QObject* parent = nullptr);
        virtual ~ModelInfo();

        ModelN* model();

        QString fileName() const;

        bool isChosed() const;
        void setIsChosed_cpp(bool newIsChosed);

        bool isVisible() const;
        void setIsVisible_cpp(bool newIsVisible);
    signals:
        void isChosedChanged();
        void isVisibleChanged();
    private:
        ModelN* m_model;

        bool m_isChosed;
        bool m_isVisible;
    };

    class CusModelListModel : public QAbstractListModel
        , public qtuser_3d::SelectorTracer
    {
        Q_OBJECT
            Q_PROPERTY(bool isCheckedAll READ isCheckedAll NOTIFY isCheckedAllChanged)
    public:
        enum FileInfoRoles {
            File_Name = Qt::UserRole + 1,
            File_Visible,
            File_Checked,
            File_Size,
            File_Data
        };

        CusModelListModel(QObject* parent = nullptr);
        virtual ~CusModelListModel();

        void addModel(ModelN* model);
        void removeModel(ModelN* model);

        Q_INVOKABLE void addModel();
        Q_INVOKABLE void deleteSelections();
        Q_INVOKABLE void checkAll(bool on);
        Q_INVOKABLE void checkOne(int index);
        Q_INVOKABLE void checkAppendOne(int index);
        Q_INVOKABLE void checkRange(int begin, int index);
        Q_INVOKABLE void setItemVisible(int index, bool visible);
        Q_INVOKABLE bool hasSupport();

        bool isCheckedAll();
    signals:
        void isCheckedAllChanged();
    protected:
        int rowCount(const QModelIndex& parent = QModelIndex()) const;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
        virtual QHash<int, QByteArray> roleNames() const;
        bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

        void onSelectionsChanged() override;
        void selectChanged(qtuser_3d::Pickable* pickable) override;
        ModelInfo* _index(ModelN* model);
    protected:
        QList<ModelInfo*> m_modelsInfoList;
        bool m_isCheckedAll = false;
    };
}
#endif // CUSMODELLISTMODEL_H
