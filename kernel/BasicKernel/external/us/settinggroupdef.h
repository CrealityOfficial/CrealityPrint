#ifndef _US_SETTINGGROUPDEF_1589438667491_H
#define _US_SETTINGGROUPDEF_1589438667491_H
#include "basickernelexport.h"
#include "us/settingitemdef.h"

namespace us
{
	class BASIC_KERNEL_API SettingGroupDef: public QObject
	{
	public:
		SettingGroupDef(QObject* parent = nullptr);
		virtual ~SettingGroupDef();

		void addSettingItemDef(SettingItemDef* settingItemDef);

		QList<SettingItemDef*> items;
		QString key;
		QString label;
		QString description;
		QString type;
		int order;
	};
}
#endif // _US_SETTINGGROUPDEF_1589438667491_H
