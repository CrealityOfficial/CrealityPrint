#ifndef PLATESDATAMANAGER_H
#define PLATESDATAMANAGER_H
#include "qtuser3d/module/pickableselecttracer.h"
#include <QtCore/QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QColor>
#include <QVector>

namespace creative_kernel
{
    class Printer;
    class ModelN;
    class CusModelListModel;
    class CusModelListModelProxy;

    class PlateInfo : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(bool isExpend READ isExpend WRITE setIsExpend NOTIFY isExpendChanged)

    public:
        PlateInfo(CusModelListModelProxy* modelData = nullptr, QObject* parent = nullptr, Printer* printer = nullptr);
        ~PlateInfo();

        int plateIndex();
        void setPlateIndex(int index);

        bool isExpend();
        void setIsExpend(bool expend);

        QString plateName();
        void setPlateName(QString name);

        Printer* printer();
        CusModelListModelProxy* modelsData();

    signals:
        void isExpendChanged();
            
    private:
        bool m_isExpend = true;
        Printer* m_Printer = nullptr;
        CusModelListModelProxy* m_ModelData = nullptr;
    };
    class PlatesDataManager : public QAbstractListModel
    {
        Q_OBJECT
        Q_PROPERTY(int plateCount READ plateCount NOTIFY plateCountChanged)
        Q_PROPERTY(int currentPlateIndex READ currentPlateIndex WRITE setCurrentPlateIndex NOTIFY currentPlateIndexChanged)

    public:
        enum PlateRoles 
        {
            PR_PlateIndex,
            PR_PlateModels,
            PR_PLateName,
            PR_PLATEINFO,
            PR_PLateObj
        };

        PlatesDataManager(QObject* parent = nullptr);
        ~PlatesDataManager();

        int plateCount();

        int currentPlateIndex();
        void setCurrentPlateIndex(int index);
        
        void registerModelList(CusModelListModel* modelList);

        void addPlate(Printer* p);
        void deletePlate(Printer* p);

        Q_INVOKABLE void selectPlate(int index);

    protected:
        int rowCount(const QModelIndex& parent = QModelIndex()) const;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
        virtual QHash<int, QByteArray> roleNames() const;
        bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

    signals:
        void plateCountChanged();
        void currentPlateIndexChanged();
        void plateModelListChanged(int index, const QList<ModelN*>& modelList);

    public slots:
        //接收多盘信息变动相关的参数
        void slotAddPlate(Printer* p);
        void slotDeletePlate(Printer* p);
        void slotModelsOutPlateChange(const QList<ModelN*>& list);

    private:
        void addPlateDataModelProxy(Printer* pt = nullptr);
        void refreshPlatesIndex();

    private:
        CusModelListModel* m_SourceModelList = nullptr;
        QMap<int, PlateInfo*> m_PlatesInfo;
    };
}
#endif // PLATESDATAMANAGER_H
