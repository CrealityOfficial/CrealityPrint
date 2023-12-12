#include "cxgcode/us/settingitemdef.h"
#include "cxgcode/us/metatype.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>

namespace cxgcode
{
	bool checkBool(const QString& str)
	{
		return str == "true" || str == "True";
	}

	QStringList loadKeys(const QString& fileName)
	{
		QStringList keys;
		QFile file(fileName);
		if (file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			while (!file.atEnd())
			{
				QString line = file.readLine();
				line = line.replace("\n", "");
				if (line.size() > 0 && !keys.contains(line))
					keys.append(line);
			}
		}
		file.close();
		return keys;
	}

	void saveKeys(const QStringList& keys, const QString& fileName)
	{
		QFile file(fileName);
		if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
		{
			for (const QString& key : keys)
			{
				if (!key.isEmpty())
				{
					file.write(key.toLocal8Bit());
					file.write("\n");
				}
			}
		}
		file.close();
	}

	SettingItemDef::SettingItemDef(QObject* parent)
		:QObject(parent)
		, m_metaType(nullptr)
		, level(3)
	{
		name = "";
		label = "";
		unit = "";
		description = "";
		type = "";
		defaultStr = "";
		minimum = "";
		maximum = "";
		valueStr = "";
		showValueStr = "";
		miniwarning = "";
		maxwarning = "";
		isCustomSetting = false;
		enableStatus = "true";
		order = 0;
	}

	SettingItemDef::~SettingItemDef()
	{
	}

	void SettingItemDef::process()
	{
		if (type == "str")
			m_metaType = &sStrType;
		else if (type == "float")
			m_metaType = &sFloatType;
		else if (type == "int")
			m_metaType = &sIntType;
		else if (type == "bool")
			m_metaType = &sBoolType;
		else if (type == "enum")
			m_metaType = &sEnumType;
		else if (type == "polygon")
			m_metaType = &sPolygonType;
		else if (type == "polygons")
			m_metaType = &sPolygonsType;
		else if (type == "optional_extruder")
			m_metaType = &sOptionalExtruderType;
		else if (type == "[int]")
			m_metaType = &sIntArrayType;
		else if (type == "extruder")
			m_metaType = &sExtruderType;
	}

	void SettingItemDef::addSettingItemDef(SettingItemDef* settingItemDef)
	{
		items.push_back(settingItemDef);
	}
}
