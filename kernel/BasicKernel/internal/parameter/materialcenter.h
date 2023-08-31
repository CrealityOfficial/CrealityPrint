#ifndef CREATIVE_KERNEL_PRINTMATERIALCENTER_1675841555856_H
#define CREATIVE_KERNEL_PRINTMATERIALCENTER_1675841555856_H
#include "data/header.h"

namespace creative_kernel
{
	class MaterialCenter : public QObject
	{
		Q_OBJECT
		Q_PROPERTY(QStringList materials READ materials NOTIFY materialsChanged)
	public:
		MaterialCenter(QObject* parent = nullptr);
		virtual ~MaterialCenter();

		void intialize();
		Q_INVOKABLE QStringList types();
		Q_INVOKABLE QStringList brands();
		Q_INVOKABLE QStringList materials();

		Q_INVOKABLE void addFilter(QString filter);
		const QList<MaterialMeta>& materialMetas();
	protected:
		QList<MaterialMeta> m_materialMetas;
		
		QStringList m_types;
		QStringList m_brands;

	signals:
		void materialsChanged();
	};
}

#endif // CREATIVE_KERNEL_PRINTMATERIAL_1675841555856_H