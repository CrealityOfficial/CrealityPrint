#ifndef _CXGCODE_US_SETTINGGROUPDEF_1589438667491_H
#define _CXGCODE_US_SETTINGGROUPDEF_1589438667491_H
#include "cxgcode/interface.h"
#include "cxgcode/us/settingitemdef.h"

namespace cxgcode
{
	class CXGCODE_API SettingGroupDef: public QObject
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
#endif // _CXGCODE_US_SETTINGGROUPDEF_1589438667491_H
