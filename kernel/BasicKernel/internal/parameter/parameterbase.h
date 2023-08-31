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

		Q_INVOKABLE QObject* settingsObject();
		us::USettings* settings();
		us::USettings* userSettings();
		void saveSetting(const QString& fileName);
		//void saveUserSetting(const QString& fileName);
		void setUserSettings(us::USettings* settings);
		void exportSetting(const QString& fileName);
		void exportSettingIni(const QString& fileName);

		//≈‰÷√Œƒº˛∂¡–¥
		Q_INVOKABLE QString readIniByKey(const QString& key, const QString& path);
		Q_INVOKABLE void writeIniByKey(const QString& key, const QString& value, const QString& path);

		bool dirty();
		void setDirty();
		void clearDiry();
		Q_INVOKABLE virtual void save();
		Q_INVOKABLE virtual void reset();
	protected:
		void setSettings(us::USettings* settings);
	protected:
		us::USettings* m_settings = nullptr;
		us::USettings* m_user_settings = nullptr;

		bool m_dirty;
	};
}

#endif // CREATIVE_KERNEL_PARAMETERBASE_1676289222839_H