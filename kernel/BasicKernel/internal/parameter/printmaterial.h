#ifndef CREATIVE_KERNEL_PRINTMATERIAL_1675841555856_H
#define CREATIVE_KERNEL_PRINTMATERIAL_1675841555856_H
#include "internal/parameter/parameterbase.h"
#include "data/header.h"
#include <set>

namespace creative_kernel
{
	class ParameterDataModel;
	class PrintMaterial : public ParameterBase
	{
		Q_OBJECT
		Q_PROPERTY(QString extruderTemp READ extruderTemp WRITE setExtruderTemp NOTIFY extruderTempChanged)
		Q_PROPERTY(QString bedTemp READ bedTemp WRITE setBedTemp NOTIFY bedTempChanged)
		Q_PROPERTY(QString materialType READ materialType WRITE setMaterialType NOTIFY materialTypeChanged)

	public:
		PrintMaterial();
		PrintMaterial(const QString& machineName, const MaterialData& data, QObject* parent = nullptr);
		virtual ~PrintMaterial();

		void refreshChangedValue();

		QString materialType();
		void setMaterialType(const QString& type);

		QString extruderTemp();
		void setExtruderTemp(const QString& extruderTemp);

		QString bedTemp();
		void setBedTemp(const QString& extruderTemp);

		Q_INVOKABLE QString name();
		Q_INVOKABLE QString brand();
		Q_INVOKABLE QString type();
		Q_INVOKABLE QString rfid();
		Q_INVOKABLE float diameter();
		int rank() const;
		Q_INVOKABLE bool isUserDef();
		Q_INVOKABLE bool isMachineDef();

		void setName(const QString& materialName);
		Q_INVOKABLE QString uniqueName();

		void added();
		void addedUserMaterial();

		Q_INVOKABLE void removed();
		Q_INVOKABLE QObject* basicDataModel();
		ParameterDataModel* getDataModel();
		Q_INVOKABLE void exportMaterialFromQml(const QString& urlString);
		Q_INVOKABLE void save();
		Q_INVOKABLE void cancel();
		Q_INVOKABLE void reset();

		Q_INVOKABLE QObject* extruderDataModel();
		Q_INVOKABLE QString addExtruderOverrideKey(const QString& key);
		Q_INVOKABLE void removeExtruderOverrideKey(const QString& key);
		Q_INVOKABLE QObject* processDataModel(const QString& processName);
		Q_INVOKABLE QString addProcessOverrideKey(const QString& processName, const QString& key);
		Q_INVOKABLE void removeProcessOverrideKey(const QString& processName, const QString& key);
		Q_INVOKABLE QStringList getExtruderOverrideKeys();
		Q_INVOKABLE QStringList getProcessOverrideKeys(const QString& processName);
		Q_INVOKABLE QStringList getAllOverrideKeys(const QString& processName);
		us::USettings* getProcessOverrideSettings(const QString& processName);
		us::USettings* getExtruderOverrideSettings() const { return m_extruderOverrideSettings; };

		//�ĲĲ�ͬ�����ĶԱ�
		Q_INVOKABLE QStringList materialDiffKeys(QObject* obj);
	private:
		void createConnect();
	signals:
		void extruderTempChanged();
		void bedTempChanged();
		void materialTypeChanged();

	public slots:
		void onCurrentBedTypeChanged();

	protected:
		MaterialData m_data;
		QString m_machineName;
		ParameterDataModel* m_BasicDataModel = nullptr;
		ParameterDataModel* m_extruderDataModel = nullptr;
		std::map<QString, us::USettings*> m_processOverrideSettings;
		std::map<QString, ParameterDataModel*> m_processDataModels;
		us::USettings* m_extruderOverrideSettings;

	private:
		bool m_extruderOverrideChanged = false;
		std::set<QString> m_processOverrideChanged;
	};
}

#endif // CREATIVE_KERNEL_PRINTMATERIAL_1675841555856_H
