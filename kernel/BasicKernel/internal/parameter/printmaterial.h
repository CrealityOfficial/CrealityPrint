#ifndef CREATIVE_KERNEL_PRINTMATERIAL_1675841555856_H
#define CREATIVE_KERNEL_PRINTMATERIAL_1675841555856_H
#include "internal/parameter/parameterbase.h"
#include "data/header.h"

namespace creative_kernel
{
	class ProfileParameterModel;
	class PrintMaterial : public ParameterBase
	{
		Q_OBJECT
	public:
		PrintMaterial();
		PrintMaterial(const QString& machineName, const MaterialData& data, QObject* parent = nullptr);
		virtual ~PrintMaterial();

		Q_INVOKABLE QString name();
		Q_INVOKABLE QString brand();
		Q_INVOKABLE QString type();
		Q_INVOKABLE bool isUserDef();

		void setName(const QString& materialName); 
		Q_INVOKABLE QString uniqueName();

		void added();
		Q_INVOKABLE void removed();

		void addedUserMaterial();
		Q_INVOKABLE QAbstractListModel* materialParameterModel(const int& extruderIndex,bool professional=true);
		Q_INVOKABLE void exportMaterialFromQml(const QString& urlString);
		Q_INVOKABLE void save();
		Q_INVOKABLE void cancel();
		Q_INVOKABLE void reset();
		us::USettings* getOverrideSettings(const QString& quality);
		int getTime() { return m_data.time; }
	protected:
		MaterialData m_data;
		QString m_machineName;
		ProfileParameterModel* m_materialParameterModel;
		std::map<QString, us::USettings*> m_overrideSettings;
	};
}

#endif // CREATIVE_KERNEL_PRINTMATERIAL_1675841555856_H