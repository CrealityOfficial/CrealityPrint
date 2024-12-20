#include <QUrl>
#include <QFile>
#include<QtQml/QQmlEngine>
#include "printmaterial.h"
#include "printmachine.h"
#include "printextruder.h"
#include "printprofile.h"
#include "kernel/kernel.h"
#include "parametermanager.h"
#include "qtusercore/string/resourcesfinder.h"
#include "internal/models/parameterdatamodel.h"
//#include "internal/parameter/printmachine.cpp"

namespace creative_kernel
{
	PrintMaterial::PrintMaterial()
	{

	}

	PrintMaterial::PrintMaterial(const QString& machineName, const MaterialData& data, QObject* parent)
		: ParameterBase(parent)
		, m_machineName(machineName)
		, m_data(data)
		, m_extruderOverrideSettings(nullptr)
	{
		//m_name = m_fileName.mid(0, m_fileName.indexOf(".default"));

	}

	PrintMaterial::~PrintMaterial()
	{

	}

	void PrintMaterial::refreshChangedValue()
	{
		emit extruderTempChanged();
		emit bedTempChanged();
		emit materialTypeChanged();
	}

	QString PrintMaterial::materialType()
	{
		QString temp = settings()->vvalue("filament_type").toString();
		return temp;
	}

	void PrintMaterial::setMaterialType(const QString& type)
	{
		emit materialTypeChanged();
	}

	QString PrintMaterial::extruderTemp()
	{
		QString temp = userSettings()->vvalue("nozzle_temperature").toString();
		if (temp.isEmpty())
		{
			temp = settings()->vvalue("nozzle_temperature").toString();
		}

		return temp;
	}

	void PrintMaterial::setExtruderTemp(const QString& extruderTemp)
	{
		emit extruderTempChanged();
		emit bedTempChanged();
	}

	QString PrintMaterial::bedTemp()
	{
		ParameterManager* pm = creative_kernel::getKernel()->parameterManager();
		QString type = pm->currBedType();
		QString key = QString();
		if (!type.compare(QString("Cool plate"), Qt::CaseInsensitive))
		{
			key = "cool_plate_temp";
		}
		else if (!type.compare(QString("Engineering plate"), Qt::CaseInsensitive))
		{
			key = "eng_plate_temp";
		}
		else if (!type.compare(QString("Smooth PEI Plate / High Temp Plate"), Qt::CaseInsensitive) ||
			!type.compare(QString("High Temp Plate"), Qt::CaseInsensitive))
		{
			key = "hot_plate_temp";
		}
		else if (!type.compare(QString("Textured PEI plate"), Qt::CaseInsensitive))
		{
			key = "textured_plate_temp";
		}
		else if (!type.compare(QString("Epoxy Resin Plate"), Qt::CaseInsensitive))
		{
			key = "epoxy_resin_plate_temp";
		}
		QString value = userSettings()->vvalue(key).toString();
		if (value.isEmpty())
		{
			value = settings()->vvalue(key).toString();
		}

		return value;
	}

	void PrintMaterial::setBedTemp(const QString& extruderTemp)
	{

	}

	QString PrintMaterial::name()
	{
		return m_data.name;
		//return m_data.uniqueName();
	}

	QString PrintMaterial::brand()
	{
		return m_data.brand;
	}

	QString PrintMaterial::type()
	{
		return m_data.type;
	}

	QString PrintMaterial::rfid()
	{
		QString rfid = m_settings->vvalue("filament_ids").toString();
		return rfid;
	}

	int PrintMaterial::rank() const
	{
		return m_data.rank;
	}

	Q_INVOKABLE bool PrintMaterial::isUserDef()
	{
		return m_data.isUserDef;
	}

	Q_INVOKABLE bool PrintMaterial::isMachineDef()
	{
		return m_data.isMachineDef;
	}

	void PrintMaterial::setName(const QString& materialName)
	{
		m_data.name = materialName;
	}

    QString PrintMaterial::uniqueName()
    {
        return m_data.uniqueName();
    }
	float PrintMaterial::diameter()
	{
		return m_data.diameter;
	}

	void PrintMaterial::added()
	{
		auto machine = qobject_cast<PrintMachine*>(parent());
		setSettings(createMaterialSettings(m_machineName, m_data, m_data.isUserDef || machine->inheritsFrom().isEmpty()));
		setUserSettings(createMaterialCoverSettings(m_machineName, m_data));

		QStringList process_names = fetchOfficialProfileNames(m_machineName);
		process_names += fetchUserProfileNames(m_machineName);

		for (const auto& process : process_names)
		{
			auto process_name = process.mid(0, process.indexOf(".json"));
			us::USettings* settings = createProcessOverrideSettings(m_machineName, m_data, process_name);
			if (settings)
			{
				m_processOverrideSettings[process_name] = settings;
			}
		}
		m_extruderOverrideSettings = createExtruderOverrideSettings(m_machineName, m_data.uniqueName());
	}

	void PrintMaterial::addedUserMaterial()
	{
		setSettings(createMaterialSettings(m_machineName, m_data, true));
		setUserSettings(createMaterialCoverSettings(m_machineName, m_data));
	}

	void PrintMaterial::removed()
	{
		//1.�Ƴ��ļ�
		QString filePath = materialCoverFile(m_machineName, m_data.uniqueName());
		QFile file(filePath);
		if (file.exists())
			file.remove();

		filePath = defaultMaterialFile(m_machineName, m_data.uniqueName(),true);
		QFile file2(filePath);
		if (file2.exists())
			file2.remove();

	}

	Q_INVOKABLE QObject* PrintMaterial::basicDataModel()
	{
		return getDataModel();
	}

	ParameterDataModel* PrintMaterial::getDataModel() {
		if (!m_BasicDataModel)
		{
			m_BasicDataModel = new ParameterDataModel(m_settings, m_user_settings, this);
			QQmlEngine::setObjectOwnership(m_BasicDataModel, QQmlEngine::QQmlEngine::CppOwnership);
			createConnect();
		}

		if (!m_settings)
			return nullptr;

		QString materialName1 = m_settings->vvalue("material_name").toString();
		QString materialName2 = this->m_data.name;

		qDebug() << QString("[ParamCheck] _settings material_name = %1").arg(materialName1);
		qDebug() << QString("[ParamCheck] _metaData material_name = %1").arg(materialName2);
		return m_BasicDataModel;
	}

	Q_INVOKABLE void PrintMaterial::exportMaterialFromQml(const QString& urlString)
	{
		QUrl url(urlString);
		this->exportSetting(url.toLocalFile());
	}

	void PrintMaterial::save()
	{
		saveSetting(materialCoverFile(m_machineName, m_data.uniqueName()));
		if (m_extruderOverrideChanged)
		{
			m_extruderOverrideSettings->saveAsDefault(userExtruderOverrideFile(m_machineName, m_data.uniqueName()));
		}
		for (const auto& process : m_processOverrideChanged)
		{
			auto* const process_settings = getProcessOverrideSettings(process);
			if (process_settings) {
				process_settings->saveAsDefault(userProcessOverrideFile(m_machineName, m_data.uniqueName(), process));
			}
		}
	}

	void PrintMaterial::cancel()
	{
	}

	void PrintMaterial::reset()
	{
		if (m_BasicDataModel)
		{
			QList<QString> keys;
			QHash<QString, us::USetting* >::const_iterator it = m_user_settings->settings().constBegin();
			while (it != m_user_settings->settings().constEnd())
			{
				keys.append(it.key());
				it++;
			}
			for (int i = 0; i < keys.size(); i++)
			{
				//m_materialParameterModel->resetValue(keys[i]);
				m_BasicDataModel->resetValue(keys[i]);
			}
		}
		ParameterBase::reset();
	}

	QObject* PrintMaterial::extruderDataModel()
	{
		if (!m_extruderDataModel)
		{
			m_extruderDataModel = new ParameterDataModel(getExtruderOverrideSettings(), new us::USettings(), this);
			QQmlEngine::setObjectOwnership(m_extruderDataModel, QQmlEngine::QQmlEngine::CppOwnership);
		}
		return m_extruderDataModel;
	}

	QString PrintMaterial::addExtruderOverrideKey(const QString& key)
	{
		QString value;
		PrintMachine* machine = qobject_cast<PrintMachine*>(parent());
		if (machine)
		{
			value = machine->getExtruderValue(key);
		}
		auto* const settings = getExtruderOverrideSettings();
		if (settings) {
			settings->add(key, value);
			if (!m_extruderOverrideChanged)
			{
				m_extruderOverrideChanged = true;
			}
		}
		return value;
	}

	void PrintMaterial::removeExtruderOverrideKey(const QString& key)
	{
		auto* const settings = getExtruderOverrideSettings();
		if (settings) {
			settings->deleteValueByKey(key);
			if (!m_extruderOverrideChanged)
			{
				m_extruderOverrideChanged = true;
			}
		}
	}

	QObject* PrintMaterial::processDataModel(const QString& processName)
	{
		if (m_processDataModels.find(processName) == m_processDataModels.cend())
		{
			ParameterDataModel* basicDataModel = new ParameterDataModel(getProcessOverrideSettings(processName), new us::USettings(), this);
			QQmlEngine::setObjectOwnership(basicDataModel, QQmlEngine::QQmlEngine::CppOwnership);
			m_processDataModels[processName] = basicDataModel;
		}
		return m_processDataModels[processName];
	}

	QString PrintMaterial::addProcessOverrideKey(const QString& processName, const QString& key)
	{
		QString value;
		PrintMachine* machine = qobject_cast<PrintMachine*>(parent());
		if (machine)
		{
			value = machine->getProcessValue(processName, key);
		}
		auto* const settings = getProcessOverrideSettings(processName);
		if (settings) {
			settings->add(key, value);
			if (m_processOverrideChanged.find(processName) == m_processOverrideChanged.cend())
			{
				m_processOverrideChanged.insert(processName);
			}
		}
		return value;
	}

	void PrintMaterial::removeProcessOverrideKey(const QString& processName, const QString& key)
	{
		auto* const settings = getProcessOverrideSettings(processName);
		if (settings) {
			settings->deleteValueByKey(key);
			if (m_processOverrideChanged.find(processName) == m_processOverrideChanged.cend())
			{
				m_processOverrideChanged.insert(processName);
			}
		}
	}

	QStringList PrintMaterial::getExtruderOverrideKeys()
	{
		QStringList extruder_keys;
		if (m_extruderOverrideSettings)
		{
			extruder_keys = m_extruderOverrideSettings->settings().keys();
		}
		return extruder_keys;
	}

	QStringList PrintMaterial::getProcessOverrideKeys(const QString& processName)
	{
		QStringList process_keys;
		auto processOverrideSettings = getProcessOverrideSettings(processName);
		if (processOverrideSettings)
		{
			process_keys = processOverrideSettings->settings().keys();
		}
		return process_keys;
	}

	QStringList PrintMaterial::getAllOverrideKeys(const QString& processName)
	{
		QStringList process_keys, extruder_keys;
		auto processOverrideSettings = getProcessOverrideSettings(processName);
		if (processOverrideSettings)
		{
			process_keys = processOverrideSettings->settings().keys();
		}
		if (m_extruderOverrideSettings)
		{
			extruder_keys = m_extruderOverrideSettings->settings().keys();
		}
		return process_keys + extruder_keys;
	}

	us::USettings* PrintMaterial::getProcessOverrideSettings(const QString& processName)
	{
		if (m_processOverrideSettings.find(processName) != m_processOverrideSettings.cend())
		{
			return m_processOverrideSettings[processName];
		}
		return nullptr;
	}

	QStringList PrintMaterial::materialDiffKeys(QObject* obj)
	{
		PrintMaterial* material = qobject_cast<PrintMaterial*>(obj);
		QStringList diffKeys = this->compareSettings(material);
		return diffKeys;
	}

	void PrintMaterial::createConnect()
	{
		if (!m_BasicDataModel)
			return;

		ParameterDataItem* cptItem = m_BasicDataModel->findOrMakeDataItem("cool_plate_temp");
		ParameterDataItem* eptItem = m_BasicDataModel->findOrMakeDataItem("eng_plate_temp");
		ParameterDataItem* hptItem = m_BasicDataModel->findOrMakeDataItem("hot_plate_temp");
		ParameterDataItem* tptItem = m_BasicDataModel->findOrMakeDataItem("textured_plate_temp");
		ParameterDataItem* ntItem = m_BasicDataModel->findOrMakeDataItem("nozzle_temperature");
		ParameterDataItem* ftItem = m_BasicDataModel->findOrMakeDataItem("filament_type");

		connect(cptItem, &ParameterDataItem::valueChanged, this, &PrintMaterial::bedTempChanged);
		connect(eptItem, &ParameterDataItem::valueChanged, this, &PrintMaterial::bedTempChanged);
		connect(hptItem, &ParameterDataItem::valueChanged, this, &PrintMaterial::bedTempChanged);
		connect(tptItem, &ParameterDataItem::valueChanged, this, &PrintMaterial::bedTempChanged);
		connect(ntItem, &ParameterDataItem::valueChanged, this, &PrintMaterial::extruderTempChanged);
		connect(ftItem, &ParameterDataItem::valueChanged, this, &PrintMaterial::materialTypeChanged);
	}

	void PrintMaterial::onCurrentBedTypeChanged()
	{
		emit extruderTempChanged();
		emit bedTempChanged();
	}
}
