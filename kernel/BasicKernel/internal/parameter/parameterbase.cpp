#include <QSettings>
#include "parameterbase.h"
#include <QFile>

namespace creative_kernel
{
	ParameterBase::ParameterBase(QObject* parent)
		: QObject(parent)
		, m_settings(nullptr)
		, m_dirty(true)
	{

	}

	ParameterBase::~ParameterBase()
	{

	}

	QObject* ParameterBase::settingsObject()
	{
		return m_settings;
	}

	QObject* ParameterBase::userSettingsObject()
	{
		return m_user_settings;
	}

	us::USettings* ParameterBase::settings()
	{
		return m_settings;
	}

	void ParameterBase::setSettings(us::USettings* settings)
	{
		if (m_settings) {
			delete m_settings;
		}

		m_settings = settings;
		if (m_settings) {
			m_settings->setParent(this);
			QString v = settings->vvalue("acceleration_infill").toString();
			int a = 5;
		}

		settingsChanged();
	}

	void ParameterBase::setUserSettings(us::USettings* settings)
	{
		if (m_user_settings) {
			delete m_user_settings;
		}

		m_user_settings = settings;
		if (m_user_settings) {
			m_user_settings->setParent(this);
		}

		userSettingsChanged();
	}

	void ParameterBase::exportSetting(const QString& fileName)
	{
		us::USettings* settings = m_settings->copy();
		if (m_user_settings)
			settings->merge(m_user_settings);
		settings->saveAsDefault(fileName);
		delete settings;
	}

	void ParameterBase::exportSettingIni(const QString& fileName)
	{
		us::USettings* settings = m_settings->copy();
		if (m_user_settings)
			settings->merge(m_user_settings);
		settings->saveAsDefaultIni(fileName);
		delete settings;
	}

	//比较两个不同配置，返回value不同的key的列表
	QStringList ParameterBase::compareSettings(ParameterBase* const param)
	{
		if (!param)
			return QStringList();

		QStringList diffKeys;

		us::USettings* compare1 = m_settings->copy();
		compare1->merge(m_user_settings);

		us::USettings* compare2 = param->settings()->copy();
		compare2->merge(param->userSettings());

		const QHash<QString, us::USetting*>& settings = compare1->settings();
		auto it = settings.begin();
		while (it != settings.end())
		{
			QString value1 = it.value()->str();
			QString value2 = compare2->value(it.key(), "");
			if (it.key() == "acceleration_infill")
			{
				int a = 5;
			}
			if (it.key() == "bridge_skin_speed")
			{
				int a = 5;
			}
			if (it.value()->str() != compare2->value(it.key(), ""))
			{
				diffKeys << it.key();
			}
			it++;
		}

		return diffKeys;
	}

	QString ParameterBase::readIniByKey(const QString& key, const QString& path)
	{
		QSettings setting(path, QSettings::IniFormat);
		QString res = setting.value(QString("//%1").arg(key)).toString();
		return res;
	}

	void ParameterBase::writeIniByKey(const QString& key, const QString& value, const QString& path)
	{
		QSettings setting(path, QSettings::IniFormat);
		setting.setValue(QString("//%1").arg(key), value);
	}

	void ParameterBase::saveSetting(const QString& fileName)
	{
		//不保存默认加载的，因为会联动计算，计算错误会导致每次都失败
		//if (m_settings)
		//	m_settings->saveAsDefault(fileName);
		if (m_user_settings)
		{
			if (m_user_settings->hashSettings().isEmpty())
			{
				QFile(fileName).remove();
			}
			else
			{
				m_user_settings->saveAsDefault(fileName);
			}
		}
	}

	us::USettings* ParameterBase::userSettings()
	{
		if (m_user_settings == nullptr)
		{
			return nullptr;
		}
		return m_user_settings;
	}

	QString ParameterBase::settingsValue(const QString &key, const QString& defaultValue) const
	{
		if (m_user_settings->hasKey(key))
			return m_user_settings->value(key, defaultValue);

		return m_settings->value(key, defaultValue);
	}

	bool ParameterBase::dirty()
	{
		return m_dirty;
	}

	void ParameterBase::setDirty()
	{
		m_dirty = true;
	}

	void ParameterBase::clearDiry()
	{
		m_dirty = false;
	}
	void ParameterBase::enableChanged(const QString& key, bool enabled)
	{
	}
	void ParameterBase::strChanged(const QString& key, const QString& str)
	{
	}
	void ParameterBase::save()
	{

	}
	void ParameterBase::reset()
	{
		if (m_user_settings) {
			m_user_settings->clear();
		}
		save();
	}

	void ParameterBase::mergeAndSave()
	{
		m_settings->merge(m_user_settings);
		reset();
	}

	QString ParameterBase::inheritsFrom() const
	{
		return m_inherits_from;
	}

	void ParameterBase::setInheritFrom(const QString& factoryName)
	{
		if (m_inherits_from != factoryName)
		{
			m_inherits_from = factoryName;
		}
	}

	QStringList ParameterBase::differentKeysToFactory() const
	{
		return m_different_keys_to_factory;
	}

	void ParameterBase::setDifferentKeysToFactory(const QStringList& keys)
	{
		if (m_different_keys_to_factory != keys)
		{
			m_different_keys_to_factory = keys;
		}
	}
}
