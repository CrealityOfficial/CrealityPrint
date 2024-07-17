#ifndef CREATIVE_KERNEL_PARAMETERBASE_1676289222839_H
#define CREATIVE_KERNEL_PARAMETERBASE_1676289222839_H
#include "us/usettings.h"
#include "internal/parameter/parameterpath.h"
#include <QtCore/QAbstractListModel>

namespace creative_kernel
{
	class ParameterBase : public QObject
	{
		Q_OBJECT
	public:
		enum SettingType
		{
			PRINTER_TYPE,
			MATERIAL_TYPE,
			PROFILE_TYPE
		};
		ParameterBase(QObject* parent = nullptr);
		virtual ~ParameterBase();

	public:
		us::USettings* settings();
		Q_INVOKABLE QObject* settingsObject();
		Q_SIGNAL void settingsChanged();
		Q_PROPERTY(QObject* settings READ settingsObject NOTIFY settingsChanged);

		us::USettings* userSettings();
		Q_INVOKABLE QObject* userSettingsObject();
		Q_SIGNAL void userSettingsChanged();
		Q_PROPERTY(QObject* userSettings READ userSettingsObject NOTIFY userSettingsChanged);

	public:
		QString settingsValue(const QString &key, const QString& defaultValue = QString()) const;

		void saveSetting(const QString& fileName);
		//void saveUserSetting(const QString& fileName);
		void setUserSettings(us::USettings* settings);
		virtual void exportSetting(const QString& fileName);
		void exportSettingIni(const QString& fileName);

		//比较两个不同配置，返回value不同的key的列表
		QStringList compareSettings(ParameterBase* const param);

		//配置文件读写
		Q_INVOKABLE QString readIniByKey(const QString& key, const QString& path);
		Q_INVOKABLE void writeIniByKey(const QString& key, const QString& value, const QString& path);

		bool dirty();
		void setDirty();
		void clearDiry();
		virtual void enableChanged(const QString& key, bool enabled);
		virtual void strChanged(const QString& key, const QString& str);
		Q_INVOKABLE virtual void save();
		Q_INVOKABLE virtual void reset();
		virtual void mergeAndSave();
		QString inheritsFrom() const;
		virtual void setInheritFrom(const QString& factoryName);
		QStringList differentKeysToFactory() const;
		virtual void setDifferentKeysToFactory(const QStringList& keys);
	protected:
		void setSettings(us::USettings* settings);
	protected:
		us::USettings* m_settings = nullptr;
		us::USettings* m_user_settings = nullptr;

		bool m_dirty;
		QString m_inherits_from;
		QStringList m_different_keys_to_factory;
	};
}

#endif // CREATIVE_KERNEL_PARAMETERBASE_1676289222839_H
