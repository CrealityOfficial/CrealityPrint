#ifndef _CXGCODE_US_SETTINGDEF_1589436632667_H
#define _CXGCODE_US_SETTINGDEF_1589436632667_H
#include "cxgcode/interface.h"
#include "cxgcode/us/settinggroupdef.h"
#include <QtCore/QString>
#include <QtCore/QMap>
#include <QtCore/QHash>

namespace cxgcode
{
	class USetting;
	class USettings;
	class CXGCODE_API SettingDef: public QObject
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
}

#define SETTING(x) cxgcode::SettingDef::instance().create(x)
#define SETTING2(x, y) cxgcode::SettingDef::instance().create(x, y)
#endif // _CXGCODE_US_SETTINGDEF_1589436632667_H
