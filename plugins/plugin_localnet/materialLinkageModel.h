#ifndef MATERIALLINKAGEMODEL_H
#define MATERIALLINKAGEMODEL_H

#include <QObject>
#include <QAbstractListModel>
#include <qchar.h>
#include "localnetworkinterface/RemotePrinter.h"
class MaterialLinkageModel :public QAbstractListModel
{
        Q_OBJECT
    public:
        explicit MaterialLinkageModel(QList<RemoteMaterial> materials,QObject* parent = nullptr);

        enum CustomRoles {
            NameRole = Qt::UserRole + 1,
            MaxTempRole
        };

        using SubItems = QMap<QString, QList<RemoteMaterial>>;
        using MaterialInfo = QMap<QString, SubItems>;
   

        Q_INVOKABLE QStringList getBrands();
        Q_INVOKABLE QStringList getTypes(const QString& brand);
        Q_INVOKABLE QList<RemoteMaterial> getMaterials(const QString& brand, const QString& Type);
        Q_INVOKABLE int materialIndex(QString name);
        Q_INVOKABLE void setMaterials(const QString& brand, const QString& Type);
        Q_INVOKABLE int getMaxTemp(int index);
        Q_INVOKABLE int getMinTemp(int index);
        Q_INVOKABLE QString getPressAdv(int index);
        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        QHash<int, QByteArray> roleNames() const override;
    private:
        QString m_brand="CREALITY";
        QString m_type="PLA";
        MaterialInfo m_data;

        QObject* materialLinkageModelObject();

    };



#endif