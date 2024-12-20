#include "materialLinkageModel.h"
#include <QDebug>
MaterialLinkageModel::MaterialLinkageModel(QList<RemoteMaterial> materials,QObject* parent) : QAbstractListModel(parent)
{
        for (RemoteMaterial item : materials)
        { 
            m_data[item.brand][item.meterialType].push_back(item); 
        }
}

QStringList MaterialLinkageModel::getBrands()
{
       QList<QString> keys = m_data.keys();
       keys.removeOne("");
       return keys;
    }

QStringList MaterialLinkageModel::getTypes(const QString& brand)
{
        return  m_data[brand].keys();
}

QList<RemoteMaterial> MaterialLinkageModel::getMaterials(const QString& brand, const QString& type)
{
        m_brand = brand;
        m_type = type;
        return m_data[m_brand][m_type];
}
int MaterialLinkageModel::materialIndex(QString name)
{
        QList<RemoteMaterial> materials = m_data[m_brand][m_type];
        for (int i = 0; i < materials.count(); i++)
        {
                if (materials[i].name == name)
                {
                        return i;
                }
        }
        return 0;
}
void MaterialLinkageModel::setMaterials(const QString& brand, const QString& type)
{
        m_brand = brand;
        m_type = type;
        beginResetModel();
        endResetModel();
}
int MaterialLinkageModel::rowCount(const QModelIndex &parent) const
{
        if (parent.isValid())
        return 0;
        int size = m_data[m_brand][m_type].count();
        return size;
}
QVariant MaterialLinkageModel::data(const QModelIndex &index, int role) const
{
        if (!index.isValid())
                return QVariant();

        if (index.row() >= m_data[m_brand][m_type].count())
                return QVariant();

        if (role == NameRole)
        {
                RemoteMaterial m = m_data[m_brand][m_type][index.row()];
                qDebug()<<m.name<<index.row();
                return m.name;
        }
        if(role == MaxTempRole)
        {
                RemoteMaterial m = m_data[m_brand][m_type][index.row()];
                return m.maxTemp;
        }
                

        return QVariant();

}
Q_INVOKABLE int MaterialLinkageModel::getMaxTemp(int index)
{
        if(index<m_data[m_brand][m_type].length())
        {
                RemoteMaterial m = m_data[m_brand][m_type][index];
                return m.maxTemp;
        }else{
                return 0;
        }
        
}
Q_INVOKABLE int MaterialLinkageModel::getMinTemp(int index)
{
         if(index<m_data[m_brand][m_type].length())
        {
                RemoteMaterial m = m_data[m_brand][m_type][index];
                return m.minTemp;
        }else{
                return 0;
        }
}
Q_INVOKABLE QString MaterialLinkageModel::getPressAdv(int index)
{
        if(index<m_data[m_brand][m_type].length())
        {
                RemoteMaterial m = m_data[m_brand][m_type][index];
                return m.pressure_advance;
        }else{
                return "";
        }
}
QHash<int, QByteArray> MaterialLinkageModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[MaxTempRole] = "maxTemp";
    
    return roles;
}