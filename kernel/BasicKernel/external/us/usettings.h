#ifndef _US_USETTINGS_1589354685439_H
#define _US_USETTINGS_1589354685439_H
#include "basickernelexport.h"

#include <list>
#include <map>

#include "us/usetting.h"
#include "us/settingdef.h"

#include <QtCore/QMap>
#include <QtCore/QHash>
#include <QtCore/QSharedPointer>
#include <QtCore/QPointer>

namespace us
{
	class SettingDef;
	class BASIC_KERNEL_API USettings: public QObject
	{
		Q_OBJECT
	public:
		USettings(QObject* parent = nullptr);
		virtual ~USettings();

		const QHash<QString, USetting*>& settings() const;
		std::map<std::string, std::string> toStdMaps();

		Q_SIGNAL void settingsChanged(const QString& key, USetting* setting);
		Q_SIGNAL void settingValueChanged(const QString& key, USetting* setting);

		void print();

		USettings* copy();
		void insert(USetting* setting);
		/// @brief Try to find a USetting object with the same key.
		///        Then create and emplace one if it doesnt exist, or try to updating its value.
		/// @return false if the key is invalid, or true.
		bool add(const QString& key, const QString& value, bool cover = false);
		void merge(const QHash<QString, QString>& kvs);
		void merge(USettings* settings);
		void mergeNewItem(USettings* settings);
		void deleteValueByKey(const QString& key);

		QHash<QString, USetting*>::iterator emplace(const QString &key, USetting* setting);
		int erase(const QString& key);
		void clear();

		Q_SIGNAL void settingEmplaced(const QString &key, USetting* setting);
		Q_SIGNAL void settingErased(const QString &key);
		Q_SIGNAL void settingCleared();

		void loadDefault(const QString& file);
		void appendDefault(const QStringList& keys);
		void appendDefault();
		void loadCompleted();

        void saveAsDefault(const QString& file);
		void saveAsDefaultIni(const QString& file);
        void saveMachineAsDefault(const QString& file);
        void saveExtruderAsDefault(const QString& file, int index = 0);
		void update(const USettings* settings);
		bool isEmpty();
		bool hasKey(const QString& key);

		SettingDef* def();

		Q_INVOKABLE QVariantList profileParameterList(int index);  // 0 1 -1
		Q_INVOKABLE QObject* settingObject(const QString& key);

		QString value(const QString& key, const QString& defaultValue) const;
		QVariant vvalue(const QString& key, const QVariant& defaultValue = QVariant()) const;
		QVariantList enums(const QString& key);
		QStringList enumKeys(const QString& key);

		std::vector<trimesh::vec2> point2s(const QString& key);

		QList<USetting*> filter(const QString& category, const QString& filter, bool professional);
	//protected:
		USetting* findSetting(const QString& key);
		USetting* findSettingDelegated(const QString& key);

		std::list<QPointer<USettings>>& delegatedSettingsList();
		const std::list<QPointer<USettings>>& delegatedSettingsList() const;
		const QHash<QString, USetting*>& hashSettings() const {
			return m_hashsettings;
		}

		bool operator==(const USettings& other);

	protected:
		std::list<QPointer<USettings>> delegated_settings_list_{};
		QHash<QString, USetting*> m_hashsettings;
		SettingDef* m_def;
	};
}

typedef QSharedPointer<us::USettings> SettingsPointer;

#endif // _US_USETTINGS_1589354685439_H
