#include "us/settingdef.h"
#include "us/jsondef.h"
#include "us/usetting.h"

#include <functional>
#include "internal/parameter/parameterpath.h"
#include "crslice/base/parametermeta.h"

#include <QtCore/QDebug>
#include "external/kernel/crsliceadaptor.h"
namespace us
{
	SettingDef SettingDef::s_settingDef;
	SettingDef::SettingDef(QObject* parent)
		:QObject(parent)
	{
	}

	SettingDef::~SettingDef()
	{
	}

	SettingDef& SettingDef::instance()
	{
		return s_settingDef;
	}

	USetting* SettingDef::create(const QString& key)
	{
		SettingItemDef* def = findDef(key);
		if (def)
		{
			USetting* setting = new USetting(def);
			return setting;
		}
		return nullptr;
	}

	USetting* SettingDef::create(const QString& key, const QString& value)
	{
		USetting* setting = create(key);
		if (setting) setting->setValue(value);
		return setting;
	}

	void SettingDef::initialize(const QString& file)
	{
		m_hashItemDef.clear();
		for (SettingGroupDef* def : m_settingGroupDefs)
			delete def;
		m_settingGroupDefs.clear();
		creative_kernel::CrsliceAdaptor::getInstance().getSettingGroupDefs(m_settingGroupDefs);
		buildHash();
	}

	void SettingDef::buildHash()
	{
		std::function<void(SettingItemDef* itemDef)> buildFunc;
		buildFunc = [this, &buildFunc](SettingItemDef* itemDef) {
#ifdef _DEBUG
			QHash<QString, SettingItemDef*>::iterator it = m_hashItemDef.find(itemDef->name);
			if (it != m_hashItemDef.end())
			{
				qDebug() << QString("SettingDef::buildHash Same Key [%1].").arg(it.key());
			}
#endif
			m_hashItemDef.insert(itemDef->name, itemDef);

			for (SettingItemDef* childItemDef : itemDef->items)
				buildFunc(childItemDef);
		};

		for(SettingGroupDef* groupDef : m_settingGroupDefs)
		{
			for (SettingItemDef* itemDef : groupDef->items)
			{
				buildFunc(itemDef);
			}
		}

		for (SettingItemDef* itemSetting : m_hashItemDef)
		{
			itemSetting->process();
		}
	}

	SettingItemDef* SettingDef::findDef(const QString& key)
	{
		QHash<QString, SettingItemDef*>::iterator it = m_hashItemDef.find(key);
		if (it != m_hashItemDef.end())
			return it.value();

		//qDebug() << QString("Unkown setting key [%1]").arg(key);
		return nullptr;
	}

	QList<SettingItemDef*> SettingDef::collectItems(SettingGroupDef* groupDef)
	{
		QList<us::SettingItemDef*> defs;

		std::function<void(const QList<us::SettingItemDef*>&)> buildFunc;
		buildFunc = [&defs, &buildFunc](const QList<us::SettingItemDef*>& items) {
			if (items.count() == 0)
				return;

			QList<us::SettingItemDef*> next;
			defs += items;
			for (us::SettingItemDef* item : items)
				next += item->items;
			buildFunc(next);
		};

		if (groupDef)
			buildFunc(groupDef->items);
		return defs;
	}

	QStringList SettingDef::collectKeys(const QList<SettingItemDef*>& itemDefs)
	{
		QStringList keys;
		for (SettingItemDef* itemDef : itemDefs)
			keys.append(itemDef->name);
		return keys;
	}

	const QHash<QString, SettingItemDef*>& SettingDef::getHashItemDef() const
	{
		return m_hashItemDef;
	}

	const QMap<QString, SettingGroupDef*>& SettingDef::getGroupDef() const
	{
		return m_settingGroupDefs;
	}

	void SettingDef::writeKeys(const QString& fileName)
	{
		QStringList keys;
		for(QHash<QString, SettingItemDef*>::iterator it = m_hashItemDef.begin();
			it != m_hashItemDef.end(); ++it)
		{
			keys << it.key();
		}
		saveKeys(keys, fileName);
	}

	void SettingDef::writeAllKeys()
	{
		for (QMap<QString, us::SettingGroupDef*>::const_iterator it = m_settingGroupDefs.begin(); it != m_settingGroupDefs.end(); ++it)
		{
			QList<us::SettingItemDef*> defs = collectItems(it.value());

			QList<QString> keys = collectKeys(defs);
			saveKeys(keys, it.key());
		}
	}

	QVariantList SettingDef::parameterGroupList()
	{
		auto variantLessThanByOrder = [](us::SettingGroupDef* v1, us::SettingGroupDef* v2)
		{
			return v1->order < v2->order;
		};

		QList<us::SettingGroupDef*> defs;
		for (QMap<QString, us::SettingGroupDef*>::const_iterator it = m_settingGroupDefs.begin(); it != m_settingGroupDefs.end(); ++it)
			defs.append(it.value());

		std::sort(defs.begin(), defs.end(), variantLessThanByOrder);

		QVariantList values;
		for (us::SettingGroupDef* def : defs)
			values.push_back(QVariant::fromValue(def->label));

		qDebug() << QString("SettingDef::parameterGroupList : ");
		qDebug() << values;
		return values;
	}

	QVariantList SettingDef::parameterList(const QString& type)
	{
		auto variantLessThanByOrder = [](us::SettingItemDef* v1, us::SettingItemDef* v2)
		{
			return v1->order < v2->order;
		};

		QList<us::SettingItemDef*> defs;
		if (type.isEmpty())
		{
			for (QHash<QString, us::SettingItemDef*>::const_iterator it = m_hashItemDef.begin();
				it != m_hashItemDef.end(); ++it)
			{
				defs.append(it.value());
			}
		}
		else
		{
			for (QMap<QString, us::SettingGroupDef*>::const_iterator it = m_settingGroupDefs.begin(); it != m_settingGroupDefs.end(); ++it)
			{
				if (it.value()->label == type)
				{
					defs = collectItems(it.value());
					break;
				}
			}
		}

		std::sort(defs.begin(), defs.end(), variantLessThanByOrder);

		QVariantList values;
		for (us::SettingItemDef* def : defs)
			values.push_back(QVariant::fromValue(def->name));

		return values;
	}

	QVariantList SettingDef::profileParameterList(int index)
	{
		QStringList names0;
		names0 << "layer_height"
			<< "line_width"
			<< "wall_line_count"
			<< "infill_sparse_density"
			<< "adhesion_type"
			<< "support_enable"
			<< "support_angle"
			<< "support_infill_rate"
			<< "small_feature_max_length"
			;

		QStringList names1;
		names1 << "material_print_temperature"
			<< "material_bed_temperature"
			<< "speed_print"
			<< "speed_infill"
			<< "speed_wall"
			<< "support_material_flow"
			<< "infill_material_flow"
			<< "wall_material_flow"
			<< "small_feature_speed_factor"
			;

		QVariantList values;
		if (index == -1 || index == 0)
		{
			for (const QString& name : names0)
				values.push_back(QVariant::fromValue(name));
		}
		if (index == -1 || index == 1)
		{
			for (const QString& name : names1)
				values.push_back(QVariant::fromValue(name));
		}

		return values;
	}

	void convert(const std::vector<std::string>& inKeys, QStringList& keys)
	{
		keys.clear();
		for (const std::string& k : inKeys)
			keys.append(QString(k.c_str()));
	}

	void loadMetaMachineKeys(QStringList& keys)
	{
		std::vector<std::string> inKeys;
		creative_kernel::CrsliceAdaptor::getInstance().getMetaKeys(creative_kernel::MetaGroup::emg_machine, inKeys);
		convert(inKeys, keys);
	}

	void loadMetaExtruderKeys(QStringList& keys)
	{
		std::vector<std::string> inKeys;
		creative_kernel::CrsliceAdaptor::getInstance().getMetaKeys(creative_kernel::MetaGroup::emg_extruder, inKeys);
		convert(inKeys, keys);
	}

	void loadMetaMaterialKeys(QStringList& keys)
	{
		std::vector<std::string> inKeys;
		creative_kernel::CrsliceAdaptor::getInstance().getMetaKeys(creative_kernel::MetaGroup::emg_material, inKeys);
		convert(inKeys, keys);
	}

	void loadMetaProfileKeys(QStringList& keys)
	{
		std::vector<std::string> inKeys;
		creative_kernel::CrsliceAdaptor::getInstance().getMetaKeys(creative_kernel::MetaGroup::emg_profile, inKeys);
		convert(inKeys, keys);
	}
}
