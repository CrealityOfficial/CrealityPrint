#include "materialLinkageModel.h"
#include "internal/parameter/materialcenter.h"
#include <external/kernel/kernel.h>
namespace creative_kernel {
    MaterialLinkageModel::MaterialLinkageModel(QObject* parent) : QObject(parent)
    {
        m_materialMetas = getKernel()->materialCenter()->materialMetas();
        for (MaterialData item : m_materialMetas)
        { 
            m_data[item.brand][item.type].push_back(item.name);
            
        }
    }

    QStringList MaterialLinkageModel::getBrands()
    {
        return m_data.keys();
    }

    QStringList MaterialLinkageModel::getTypes(const QString& brand)
    {
        return  m_data[brand].keys();
    }

    QStringList MaterialLinkageModel::getMaterials(const QString& brand, const QString& type)
    {
        return m_data[brand][type];
    }
}