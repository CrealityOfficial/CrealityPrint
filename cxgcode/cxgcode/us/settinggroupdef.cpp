#include "cxgcode/us/settinggroupdef.h"

namespace cxgcode
{
	SettingGroupDef::SettingGroupDef(QObject* parent)
		: QObject(parent)
		, order(0)
	{
	}

	SettingGroupDef::~SettingGroupDef()
	{
	}

	void SettingGroupDef::addSettingItemDef(SettingItemDef* settingItemDef)
	{
		items.push_back(settingItemDef);
	}
}
