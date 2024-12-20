#include "usetting.h"
#include <QtCore/QDebug>

namespace us
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

	QString USetting::key() {
		return m_itemDef ? m_itemDef->name : QString{};
	}

	void USetting::setStr(const QString& str)
	{
		if (m_str == str)
			return;
		m_str = str;
		emit strChanged();
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
		for (auto index = 0; index < m_itemDef->options.size(); ++index) {
			if (m_itemDef->options.at(index).first == m_str) {
				return index;
			}
		}

		return -1;
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
		if (!m_itemDef->minimum.isEmpty()) {
			return m_itemDef->minimum;
		} else if (!m_itemDef->miniwarning.isEmpty()) {
			return m_itemDef->miniwarning;
		}

		return QStringLiteral("0");
	}

	QString USetting::maxStr()
	{
		if (!m_itemDef->maximum.isEmpty()) {
			return m_itemDef->maximum;
		} else if (!m_itemDef->maxwarning.isEmpty()) {
			return m_itemDef->maxwarning;
		}

		return QStringLiteral("99999");
	}

	bool USetting::globalSettable() {
		return m_itemDef->settableGlobally == QStringLiteral("true");
	}

	bool USetting::extruderSettable() {
		return m_itemDef->settablePerExtruder == QStringLiteral("true");
	}

	bool USetting::meshSettable() {
		return m_itemDef->settablePerMesh == QStringLiteral("true");
	}

	bool USetting::meshGroupSettable() {
		return m_itemDef->settablePerMeshGroup == QStringLiteral("true");
	}

	std::vector<trimesh::vec2> USetting::point2s()
	{
		QStringList strs = m_str.split(",");
		std::vector<trimesh::vec2> values;
		for (const QString& str : strs)
		{
			QStringList p = str.split("x");
			if (p.size() == 2)
			{
				trimesh::vec2 v;
				for (int i = 0; i < 2; ++i)
				{
					bool ok = false;
					float f = p.at(i).toFloat(&ok);
					if (ok)
						v[i] = f;
				}
				values.push_back(v);
			}
		}

		return values;
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
			} else {
				for (auto iter = def->options.cbegin(); iter != def->options.cend(); ++iter) {
					datas.append(QVariant::fromValue(QStringLiteral("%1=%2").arg(iter->first, iter->second)));
				}
			}
		}

		return datas;
	}

	QStringList USetting::optionKeys()
	{
		QStringList datas;
		datas.clear();

		const SettingItemDef* def = m_itemDef;
		if (def)
		{
			if (def->type == "enum" || def->type == "coEnum") {
				for (const auto& it : def->options)
					datas.append(it.first);
			}
		}

		return datas;
	}

	QVariantList USetting::options()
	{
		QVariantList datas;
		datas.clear();

		const SettingItemDef* def = m_itemDef;
		if (def)
		{if (def->type == "enum" || def->type == "coEnum") {
				for (const auto& it : def->options)
					datas.append(QVariant::fromValue(it.second));
			}
		}

		return datas;
	}
}
