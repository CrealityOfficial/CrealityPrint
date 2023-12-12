#include <QSettings>
#include "parameterbase.h"

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

	us::USettings* ParameterBase::settings()
	{
		return m_settings;
	}

	void ParameterBase::setSettings(us::USettings* settings)
	{
		if (m_settings)
			delete m_settings;
		
		m_settings = settings;
		if (m_settings)
			m_settings->setParent(this);
	}

	void ParameterBase::setUserSettings(us::USettings* settings)
	{
		if (m_user_settings)
			delete m_user_settings;

		m_user_settings = settings;
		if (m_user_settings)
			m_user_settings->setParent(this);
	}

	void ParameterBase::exportSetting(const QString& fileName)
	{
		us::USettings* settings = m_settings->copy();
		//剔除overide
		QList<us::USetting*> override_settings = settings->materialParameters("override");
		Q_FOREACH(us::USetting* us,override_settings)
		{
			settings->deleteValueByKey(us->key());
		}

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
		//������Ĭ�ϼ��صģ���Ϊ���������㣬�������ᵼ��ÿ�ζ�ʧ��
		//if (m_settings)
		//	m_settings->saveAsDefault(fileName);
		if(m_user_settings)
			m_user_settings->saveAsDefault(fileName+".cover");
	}

	us::USettings* ParameterBase::userSettings()
	{
		if (m_user_settings == nullptr)
		{
			return nullptr;
		}
		return m_user_settings;
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
	void ParameterBase::save()
	{

	}
	void ParameterBase::reset()
	{
		m_user_settings->clear();
		save();
	}
}
