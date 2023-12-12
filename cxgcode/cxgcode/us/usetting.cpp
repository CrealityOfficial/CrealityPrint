#include "usetting.h"
#include <QtCore/QDebug>

namespace cxgcode
{
	USetting::USetting(SettingItemDef* def, QObject* parent)
		:QObject(parent)
		, m_itemDef(def)
	{
		
		m_str = m_itemDef->defaultStr;
		//assert(m_str != "default");
	}

	USetting::~USetting()
	{
	}

	SettingItemDef* USetting::def()
	{
		return m_itemDef;
	}

	USetting* USetting::clone()
	{
		USetting* setting = new USetting(m_itemDef, nullptr);
		setting->m_str = m_str;
		//assert(m_str != "default");
		return setting;
	}

	void USetting::setValue(const QString& str)
	{
		
		m_str = str;
		//assert(m_str != "default");
	}

	QString USetting::key()
	{
		return m_itemDef->name;
	}

	float USetting::toFloat()
	{
		bool success = false;
		float value = m_str.toFloat(&success);
		if (!success)
			value = 0.0f;
		return value;
	}

	int USetting::toInt()
	{
		bool success = false;
		int value = m_str.toInt(&success);
		if (!success)
			value = 0;
		return value;
	}

	QString USetting::str()
	{
		if ("gcode_start" == key()
			|| "inter_layer" == key()
			|| "gcode_end" == key()
			|| "machine_start_gcode" == key()
			|| "machine_extruder_start_code" == key()
			|| "machine_extruder_end_code" == key()
			|| "machine_end_gcode" == key())
		{
			m_str = m_str.replace("\\n", "\n");
		}
		return m_str;
	}

	int USetting::enumIndex()
	{
		int index = -1;

		QMap<QString, QString> options = m_itemDef->options;
		for (QMap<QString, QString>::const_iterator it = options.begin();
			it != options.end(); ++it)
		{
			++index;
			if (it.key() == m_str)
				break;
		}
#if _DEBUG
		//qDebug() << QString("USetting::enumIndex [%1] in: ").arg(m_value.toString());
		//qDebug() << options;
#endif
		if (index == options.count())
			index = -1;
		return index;
	}

	bool USetting::enabled()
	{
		return checkBool(m_str);
	}

	QString USetting::description()
	{
		const SettingItemDef* def = m_itemDef;
		return def->description;
	}

	QString USetting::label()
	{
		const SettingItemDef* def = m_itemDef;
		return def->label;
	}

	QString USetting::type()
	{
		const SettingItemDef* def = m_itemDef;
		return def->type;
	}

	int USetting::level()
	{
		const SettingItemDef* def = m_itemDef;
		return def->level;
	}

	QString USetting::unit()
	{
		const SettingItemDef* def = m_itemDef;
		return def->unit;
	}

	QString USetting::minStr()
	{
		const SettingItemDef* def = m_itemDef;
		if (def->minimum != "")
			return def->minimum;

		return def->miniwarning;
	}

	QString USetting::maxStr()
	{
		const SettingItemDef* def = m_itemDef;
		if (def->maximum != "")
			return def->maximum;

		return def->maxwarning;
	}

	QVariantList USetting::enums()
	{
		QVariantList datas;
		datas.clear();

		const SettingItemDef* def = m_itemDef;
		if (def)
		{
			if (def->type == "optional_extruder" || def->type == "extruder")
			{
				datas.append(QVariant::fromValue(QString("-1=")+QString("Not overridden")));
				datas.append(QVariant::fromValue(QString("0=")+ QString("Extruder 1")));
				datas.append(QVariant::fromValue(QString("1=")+QString("Extruder 2")));
			}
			else if (def->type == "enum") {
				for (QMap<QString, QString>::const_iterator it = def->options.constBegin(); it != def->options.constEnd(); ++it)
					datas.append(QVariant::fromValue(it.key()+"="+ it.value()));
			}
		}

		return datas;
	}
}
