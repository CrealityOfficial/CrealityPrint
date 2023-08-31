#include "materialcenter.h"
#include "internal/parameter/parameterpath.h"

namespace creative_kernel
{
    MaterialCenter::MaterialCenter(QObject* parent)
		: QObject(parent)
	{

	}

    MaterialCenter::~MaterialCenter()
	{

	}

	void MaterialCenter::intialize()
	{
		loadMaterialMeta(m_materialMetas);
		for (const MaterialMeta& meta : m_materialMetas)
		{
			for (float diameter : meta.supportDiameters)
			{
				if (!m_brands.contains(meta.brand))
					m_brands.append(meta.brand);
				if (!m_types.contains(meta.type))
					m_types.append(meta.type);
			}
		}
	}

	QStringList MaterialCenter::types()
	{
		return m_types;
	}

	QStringList MaterialCenter::brands()
	{
		return m_brands;
	}

	QStringList MaterialCenter::materials()
	{
		QStringList matrials;
		Q_FOREACH(MaterialMeta material, m_materialMetas)
		{
			matrials.append(material.name);
		}
		return matrials;
	}

	const QList<MaterialMeta>& MaterialCenter::materialMetas()
	{
		return m_materialMetas;
	}

	 void MaterialCenter::addFilter(QString filter)
	 {
		 emit materialsChanged();
	 }
}
