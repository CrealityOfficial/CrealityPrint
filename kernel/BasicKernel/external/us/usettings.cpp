#include "us/usettings.h"
#include "us/defaultloader.h"
#include "us/settingdef.h"
#include <QtCore/QFile>
#include <QtCore/QDebug>
#include <QSettings>
#include "internal/parameter/parameterpath.h"
#include <QJsonObject>
#include <QJsonDocument>

namespace us
{
	USettings::USettings(QObject* parent)
		: QObject(parent)
		, m_def(nullptr)
	{
		m_def = &SettingDef::instance();
	}

	USettings::~USettings()
	{
	}

	USettings* USettings::copy()
	{
		USettings* settings = new USettings();
		settings->merge(this);
		return settings;
	}

	void USettings::insert(USetting* setting)
	{
		if (setting)
		{
			setting->setParent(this);
			QHash<QString, us::USetting*>::iterator insertIt = m_hashsettings.find(setting->key());
			if (insertIt != m_hashsettings.end())
			{
				us::USetting*& current_setting = insertIt.value();
				bool value_changed = current_setting && current_setting->str() != setting->str();

				current_setting = setting;

				if (value_changed)
				{
					settingValueChanged(setting->key(), setting);
				}
			}
			else
			{
				emplace(setting->key(), setting);
			}
		}
	}

	bool USettings::add(const QString& key, const QString& value, bool cover) {
		auto iter = m_hashsettings.find(key);
		if (iter == m_hashsettings.cend()) {
			auto* setting = SETTING2(key, value);
			if (setting) {
				emplace(key, setting);
				return true;
			}
		} else if (cover) {
			iter.value()->setStr(value);
			return true;
		}

		return false;
	}

	void USettings::merge(USettings* settings)
	{
		if (settings)
		{
			const QHash<QString, USetting*>& mergeSettings = settings->m_hashsettings;
			for (const auto& it : mergeSettings.keys())
			{
				if (m_hashsettings.contains(it))
				{
					m_hashsettings[it]->setStr(mergeSettings[it]->str());   // replace value
				}
				else
				{
					USetting* setting = mergeSettings[it]->clone();
					setting->setParent(this);
					emplace(it, setting);     //insert
				}
			}
		}
	}

	void USettings::merge(const QHash<QString, QString>& kvs)
	{
		for (QHash<QString, QString>::const_iterator it = kvs.begin();
			it != kvs.end(); ++it)
		{
			QHash<QString, us::USetting*>::iterator insertIt = m_hashsettings.find(it.key());
			if (insertIt != m_hashsettings.end())
			{
				insertIt.value()->setStr(it.value());   // replace value
			}
			else
			{
				USetting* setting = SETTING2(it.key(), it.value());
				setting->setParent(this);
				emplace(it.key(), setting);     //insert
			}
		}
	}

	void USettings::mergeNewItem(USettings* settings)
	{
		if (settings)
		{
			const QHash<QString, USetting*>& mergeSettings = settings->m_hashsettings;
			for (QHash<QString, us::USetting*>::const_iterator it = mergeSettings.begin(); it != mergeSettings.end(); ++it)
			{
				QHash<QString, us::USetting*>::iterator insertIt = m_hashsettings.find(it.key());
				if (insertIt != m_hashsettings.end())
				{
					//insertIt.value()->setStr(it.value()->value());   // replace value
				}
				else
				{
					USetting* setting = it.value()->clone();
					setting->setParent(this);
					emplace(it.key(), setting);     //insert
				}
			}
		}
	}

	QHash<QString, USetting*>::iterator USettings::emplace(const QString& key, USetting* setting) {
		auto iter = m_hashsettings.insert(key, setting);

		connect(setting, &USetting::strChanged,
						this, std::bind(&USettings::settingValueChanged, this, setting->key(), setting),
						Qt::ConnectionType::UniqueConnection);

		settingEmplaced(key, setting);
		settingsChanged(key, setting);
		return iter;
	}

	int USettings::erase(const QString& key) {
		auto count = m_hashsettings.remove(key);
		settingErased(key);
		settingsChanged(key, nullptr);
		return count;
	}

	void USettings::clear() {
		settingCleared();
		m_hashsettings.clear();
		settingsChanged({}, nullptr);
	}

	void USettings::deleteValueByKey(const QString& key)
	{
		erase(key);
	}

    const QHash<QString, USetting*>& USettings::settings() const
	{
		return m_hashsettings;
	}

	std::map<std::string, std::string> USettings::toStdMaps()
	{
		std::map<std::string, std::string> ss;
		for (QHash<QString, us::USetting*>::const_iterator it = m_hashsettings.begin();
			it != m_hashsettings.end(); ++it)
		{
			ss.insert(std::pair<std::string, std::string>(it.key().toLocal8Bit().data(), 
				it.value()->str().toLocal8Bit().data()));
		}

		return ss;
	}

	void USettings::print()
	{
		for (QHash<QString, us::USetting*>::const_iterator it = m_hashsettings.begin();
			it != m_hashsettings.end(); ++it)
		{
			qDebug() << it.key() << " : " << it.value()->str();
		}
	}

	void USettings::loadDefault(const QString& file)
	{
		DefaultLoader loader;
		loader.loadDefault(file, this);
	}

	void USettings::appendDefault(const QStringList& keys)
	{
		for (const QString& key : keys)
		{
			if (!m_hashsettings.contains(key))
			{
				insert(us::SettingDef::instance().create(key));
			}
		}
	}

	void USettings::appendDefault()
	{
		QHash<QString, us::SettingItemDef*> hashItemDef = us::SettingDef::instance().getHashItemDef();
		for (QHash<QString, us::SettingItemDef*>::iterator it = hashItemDef.begin();
			it != hashItemDef.end(); ++it)
		{
			if (!m_hashsettings.contains(it.key()))
				emplace(it.key(), us::SettingDef::instance().create(it.key()));
		}
	}

	void USettings::loadCompleted()
	{
		QHash<QString, us::SettingItemDef*> hashItemDef = us::SettingDef::instance().getHashItemDef();
		for (QHash<QString, us::SettingItemDef*>::iterator it = hashItemDef.begin();
			it != hashItemDef.end(); ++it)
		{
			emplace(it.key(), us::SettingDef::instance().create(it.key()));
		}
	}

	QString USettings::value(const QString& key, const QString& defaultValue) const
	{
		QString valueStr = defaultValue;
		auto it = m_hashsettings.find(key);
		if (it != m_hashsettings.end())
		{
			valueStr = it.value()->str();
		}

		return valueStr;
	}

	QVariant USettings::vvalue(const QString& key, const QVariant& defaultValue) const
	{
		QVariant value = defaultValue;
		auto it = m_hashsettings.find(key);
		if (it != m_hashsettings.end())
		{
			value = QVariant(it.value()->str());
		}

		return value;
	}

	QVariantList USettings::enums(const QString& key)
	{
		auto it = m_hashsettings.find(key);
		if (it != m_hashsettings.end())
		{
			return it.value()->options();
		}
		return QVariantList();
	}

	QStringList USettings::enumKeys(const QString& key)
	{
		auto it = m_hashsettings.find(key);
		if (it != m_hashsettings.end())
		{
			return it.value()->optionKeys();
		}
		return QStringList();
	}

	std::vector<trimesh::vec2> USettings::point2s(const QString& key)
	{
		std::vector<trimesh::vec2> values;
		auto it = m_hashsettings.find(key);
		if (it != m_hashsettings.end())
		{
			values = it.value()->point2s();
		}
		return values;
	}

	QList<USetting*> USettings::filter(const QString& category, const QString& filter, bool professional)
	{
		QList<USetting*> settings;
		for (QHash<QString, us::USetting*>::const_iterator it = m_hashsettings.begin();
			it != m_hashsettings.end(); ++it)
		{
			us::SettingItemDef* def = it.value()->def();
			if (def->category == category || category=="")
			{
				bool levelOK = false;
				//if ((professional && def->level <= 4)
				//	|| (!professional && def->level <= 2))
					levelOK = true;
				if (levelOK)
				{
					bool filterOK = true;
					if (!filter.isEmpty())
					{
						QString lKey = def->name.toLower();
						QString lFilter = filter.toLower();
						if (!lKey.contains(lFilter))
							filterOK = false;
					}
					if (filterOK
#if 0
						&& settings.count() < 4
#endif
						)
						settings.append(it.value());
				}
			}
		}

		auto variantLessThanByOrder = [](us::USetting* v1, us::USetting* v2)
		{
			return v1->def()->order < v2->def()->order;
		};
		std::sort(settings.begin(), settings.end(), variantLessThanByOrder);
		return settings;
	}

	USetting* USettings::findSetting(const QString& key)
	{
		QHash<QString, USetting*>::iterator it = m_hashsettings.find(key);
		if (it != m_hashsettings.end())
		{
			return it.value();
		}

		return nullptr;
	}

	USetting* USettings::findSettingDelegated(const QString& key) {
		auto* setting = findSetting(key);
		if (setting) {
			return setting;
		}

		for (const auto& sub_settings : delegated_settings_list_) {
			if (!sub_settings) {
				continue;
			}

			setting = sub_settings->findSettingDelegated(key);
			if (setting) {
				return setting;
			}
		}

		return nullptr;
	}

	std::list<QPointer<USettings>>& USettings::delegatedSettingsList() {
		return delegated_settings_list_;
	}

	const std::list<QPointer<USettings>>& USettings::delegatedSettingsList() const {
		return delegated_settings_list_;
	}

	bool USettings::operator==(const USettings& other)
	{
		if (m_hashsettings.size() != other.m_hashsettings.size())
			return false;

		auto it = m_hashsettings.begin(), end = m_hashsettings.end();
		for (; it != end; ++it)
		{
			if (!other.m_hashsettings.contains(it.key()) || other.m_hashsettings[it.key()] != it.value())
			{
				return false;
			}
		}
		return true;
	}

	bool  USettings::isEmpty()
	{
		return m_hashsettings.empty();
	}

	bool USettings::hasKey(const QString& key)
	{
		return m_hashsettings.constFind(key) != m_hashsettings.constEnd();
	}

	void USettings::update(const USettings* settings)
	{
		if (!settings) return;
		for (auto& ms : m_hashsettings)
		{
			QString val = settings->value(ms->key(), "");
			if (val != "")
			{
				ms->setStr(std::move(val));
			}
		}
	}

    void USettings::saveAsDefault(const QString& fileName)
    {
		{
			QFile file(fileName);
			file.remove();
		}

		QFile file(fileName);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		{
			qDebug() << QString("USettings::saveAsDefault [%1] failed.").arg(fileName);
			return;
		}

		QList<us::USetting*> orderSettings;
		for (QHash<QString, us::USetting*>::const_iterator it = m_hashsettings.begin();
			it != m_hashsettings.end(); ++it)
			orderSettings.append(it.value());

		auto f = [](us::USetting* s1, us::USetting* s2)->bool {
			return s1->def()->order < s2->def()->order;
		};

		std::sort(orderSettings.begin(), orderSettings.end(), f);

		//�汾��Ϣ
		QJsonObject rootObj;
		rootObj.insert("engine_version", "");

		QJsonObject engine_data;
		for (us::USetting* setting : orderSettings)
		{
			QString v = setting->str();
			QString key = setting->key();

			if ("gcode_start" == key
				|| "inter_layer" == key
				|| "gcode_end" == key
				|| "machine_start_gcode" == key
				|| "machine_extruder_start_code" == key
				|| "machine_extruder_end_code" == key
				|| "machine_end_gcode" == key)
			{
				v = v.replace("\n", "\\n");
			}
			engine_data.insert(key, v);
		}
		rootObj.insert("engine_data", engine_data);

		QJsonDocument document(rootObj);
		QByteArray byteArray = document.toJson();
		file.write(byteArray);
		file.close();
    }

	void USettings::saveAsDefaultIni(const QString& fileName)
	{
		QSettings iniSetting(fileName, QSettings::IniFormat);

		QList<us::USetting*> orderSettings;
		for (QHash<QString, us::USetting*>::const_iterator it = m_hashsettings.begin();
			it != m_hashsettings.end(); ++it)
			orderSettings.append(it.value());

		auto f = [](us::USetting* s1, us::USetting* s2)->bool {
			return s1->def()->order < s2->def()->order;
		};

		std::sort(orderSettings.begin(), orderSettings.end(), f);
		for (us::USetting* setting : orderSettings)
		{
			QString v = setting->str();
			QString key = setting->key();

			if ("gcode_start" == key
				|| "inter_layer" == key
				|| "gcode_end" == key
				|| "machine_start_gcode" == key
				|| "machine_extruder_start_code" == key
				|| "machine_extruder_end_code" == key
				|| "machine_end_gcode" == key)
			{
				v = v.replace("\n", "\\n");
			}
			iniSetting.setValue(QString("/Default/%1").arg(key), v);
		}
	}

	void USettings::saveMachineAsDefault(const QString& fileName)
	{
		{
			QFile file(fileName);
			file.remove();
		}

		QFile file(fileName);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		{
			qDebug() << QString("USettings::saveAsDefault [%1] failed.").arg(fileName);
			return;
		}

		QList<us::USetting*> orderSettings;
		for (QHash<QString, us::USetting*>::const_iterator it = m_hashsettings.begin();
			it != m_hashsettings.end(); ++it)
			orderSettings.append(it.value());

		auto f = [](us::USetting* s1, us::USetting* s2)->bool {
			return s1->def()->order < s2->def()->order;
		};

		std::sort(orderSettings.begin(), orderSettings.end(), f);
		file.write("[machine]\n");
		for (us::USetting* setting : orderSettings)
		{
			QString v = setting->str();
			QString key = setting->key();

			if ("gcode_start" == key
				|| "inter_layer" == key
				|| "gcode_end" == key
				|| "machine_start_gcode" == key
				|| "machine_extruder_start_code" == key
				|| "machine_extruder_end_code" == key
				|| "machine_end_gcode" == key)
			{
				v = v.replace("\n", "\\n");
			}

			QString str = QString("%1=%2\n").arg(key, v);
			file.write(str.toUtf8());
		}
		file.close();
	}

	void USettings::saveExtruderAsDefault(const QString& fileName, int index)
	{
		QFile file(fileName);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Append))
		{
			qDebug() << QString("USettings::saveAsDefault [%1] failed.").arg(fileName);
			return;
		}

		QList<us::USetting*> orderSettings;
		for (QHash<QString, us::USetting*>::const_iterator it = m_hashsettings.begin();
			it != m_hashsettings.end(); ++it)
			orderSettings.append(it.value());

		auto f = [](us::USetting* s1, us::USetting* s2)->bool {
			return s1->def()->order < s2->def()->order;
		};

		std::sort(orderSettings.begin(), orderSettings.end(), f);
		file.write(QString("[Extruder_%1]\n").arg(index).toUtf8());
		for (us::USetting* setting : orderSettings)
		{
			QString v = setting->str();
			QString key = setting->key();

			if ("gcode_start" == key
				|| "inter_layer" == key
				|| "gcode_end" == key
				|| "machine_start_gcode" == key
				|| "machine_extruder_start_code" == key
				|| "machine_extruder_end_code" == key
				|| "machine_end_gcode" == key)
			{
				v = v.replace("\n", "\\n");
			}

			QString str = QString("%1=%2\n").arg(key, v);
			file.write(str.toUtf8());
		}
		file.close();
	}

	SettingDef* USettings::def()
	{
		return m_def;
	}

	QVariantList USettings::profileParameterList(int index)
	{
		return m_def->profileParameterList(index);
	}

	QObject* USettings::settingObject(const QString& key)
	{
		USetting* setting = findSetting(key);
		return setting;
	}
}
