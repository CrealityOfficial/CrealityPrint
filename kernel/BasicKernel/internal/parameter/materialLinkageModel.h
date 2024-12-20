#ifndef MATERIALLINKAGEMODEL_H
#define MATERIALLINKAGEMODEL_H

#include <QObject>
#include <QAbstractListModel>
#include "data/header.h"
namespace creative_kernel
{
    class MaterialLinkageModel :public QObject
    {
        Q_OBJECT
    public:
        explicit MaterialLinkageModel(QObject* parent = nullptr);

        struct Item
        {
            QString brand;
            QString type;
            QString name;
        };

        using SubItems = QMap<QString, QList<QString>>;
        using MaterialInfo = QMap<QString, SubItems>;
   

        Q_INVOKABLE QStringList getBrands();
        Q_INVOKABLE QStringList getTypes(const QString& brand);
        Q_INVOKABLE QStringList getMaterials(const QString& brand, const QString& Type);

    private:
        QStringList m_brands;
        QStringList m_types;
        QStringList m_materials;
        QList<MaterialData> m_materialMetas;
        MaterialInfo m_data;

        QObject* materialLinkageModelObject();

    };
}


#endif