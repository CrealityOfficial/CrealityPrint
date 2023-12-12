#ifndef _CXGCODE_US_USETTINGS_1589354685439_H
#define _CXGCODE_US_USETTINGS_1589354685439_H
#include "cxgcode/interface.h"
#include "cxgcode/us/usetting.h"
#include "cxgcode/us/settingdef.h"

#include <QtCore/QMap>
#include <QtCore/QHash>
#include <QtCore/QSharedPointer>

namespace cxgcode
{
	class SettingDef;
	class CXGCODE_API USettings: public QObject
	{
		Q_OBJECT
	public:
		USettings(QObject* parent = nullptr);
		virtual ~USettings();

		const QHash<QString, USetting*>& settings() const;
		void print();

		USettings* copy();
		void insert(USetting* setting);
		void add(const QString& key, const QString& value, bool cover = false);
		void merge(const QHash<QString, QString>& kvs);
		void merge(USettings* settings);
		void mergeNewItem(USettings* settings);
		void clear();
		void deleteValueByKey(const QString& key);

		void loadDefault(const QString& file);
		void appendDefault(const QStringList& keys);
		void appendDefault();
		void loadCompleted();

        void saveAsDefault(const QString& file);
        void saveMachineAsDefault(const QString& file);
        void saveExtruderAsDefault(const QString& file, int index = 0);
		void update(const USettings* settings);
		bool isEmpty();
		bool hasKey(const QString& key);

		SettingDef* def();

		Q_INVOKABLE QVariantList parameterGroupList();
		Q_INVOKABLE QVariantList parameterList(const QString& type = QString(""));
		Q_INVOKABLE QVariantList profileParameterList(int index);  // 0 1 -1
		Q_INVOKABLE QObject* settingObject(const QString& key);

		QString value(const QString& key, const QString& defaultValue) const;
		QVariant vvalue(const QString& key, const QVariant& defaultValue = QVariant()) const;

		QList<USetting*> filter(const QString& category, const QString& filter, bool professional);
	//protected:
		USetting* findSetting(const QString& key);
	protected:
		QHash<QString, USetting*> m_hashsettings;
		SettingDef* m_def;
	};

	typedef QSharedPointer<USettings> SettingsPointer;
}

#endif // _CXGCODE_US_USETTINGS_1589354685439_H
