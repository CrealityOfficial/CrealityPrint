#ifndef _US_SETTINGDEF_1589436632667_H
#define _US_SETTINGDEF_1589436632667_H
#include "basickernelexport.h"
#include "us/settinggroupdef.h"
#include <QtCore/QString>
#include <QtCore/QMap>
#include <QtCore/QHash>

namespace us
{
	class USetting;
	class USettings;
	class BASIC_KERNEL_API SettingDef: public QObject
	{
		Q_OBJECT

		SettingDef(QObject* parent = nullptr);
	public:
		virtual ~SettingDef();

		static SettingDef& instance();

		USetting* create(const QString& key);
		USetting* create(const QString& key, const QString& value);

		void initialize(const QString& file);

		const QHash<QString, SettingItemDef*>& getHashItemDef() const;
		const QMap<QString, SettingGroupDef*>& getGroupDef() const;

		void writeKeys(const QString& fileName);
		void writeAllKeys();

		QVariantList parameterGroupList();
		QVariantList parameterList(const QString& type = QString(""));
		QVariantList profileParameterList(int index);  // 0 1 -1
	protected:
		void buildHash();
		SettingItemDef* findDef(const QString& key);
		QList<SettingItemDef*> collectItems(SettingGroupDef* groupDef);
		QStringList collectKeys(const QList<SettingItemDef*>& itemDefs);
	protected:
		static SettingDef s_settingDef;

		QMap<QString, SettingGroupDef*> m_settingGroupDefs;
		QHash<QString, SettingItemDef*> m_hashItemDef;
	};

	BASIC_KERNEL_API void loadMetaMachineKeys(QStringList& keys);
	BASIC_KERNEL_API void loadMetaExtruderKeys(QStringList& keys);
	BASIC_KERNEL_API void loadMetaMaterialKeys(QStringList& keys);
	BASIC_KERNEL_API void loadMetaProfileKeys(QStringList& keys);
}

#define SETTING(x) us::SettingDef::instance().create(x)
#define SETTING2(x, y) us::SettingDef::instance().create(x, y)
#endif // _US_SETTINGDEF_1589436632667_H
