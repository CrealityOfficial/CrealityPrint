#include "cxgcode/us/jsondef.h"
#include <rapidjson/document.h>

#include <QtCore/QFile>
#include <QtCore/QDebug>

#include <functional>

namespace cxgcode
{
	JsonDef::JsonDef(QObject* parent)
		:QObject(parent)
	{
	}

	JsonDef::~JsonDef()
	{
	}

	void JsonDef::parse(const QString& fileName, QMap<QString, SettingGroupDef*>& defs, QObject* parent)
	{
        qDebug() << QString("JsonDef::parse: [%1] start.").arg(fileName);
        QFile file(fileName);
        QString currentClassType = "";
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QByteArray barray = file.readAll();

            rapidjson::Document jsonDocument;
            jsonDocument.ParseInsitu(barray.data());
            if (jsonDocument.HasParseError())
            {
                qDebug() << "Json parse error";
                return;
            }

            int orderIndex = 0;

            std::function<void(const rapidjson::Value& value, SettingItemDef* itemDef, SettingGroupDef* groupDe)> fillItemCall;
            fillItemCall = [&fillItemCall, &orderIndex](const rapidjson::Value& value, SettingItemDef* itemDef, SettingGroupDef* groupDef) {
                if (!value.IsObject()) return;

                itemDef->category = groupDef->key;
                itemDef->order = orderIndex++;
                if (value.HasMember("label")) itemDef->label = (value["label"].GetString());
                if (value.HasMember("description")) itemDef->description = (value["description"].GetString());
                if (value.HasMember("unit")) itemDef->unit = (value["unit"].GetString());
                if (value.HasMember("type")) itemDef->type = (value["type"].GetString());  
				if (value.HasMember("default_value")) itemDef->defaultStr = (value["default_value"].GetString());
                if (value.HasMember("minimum_value")) itemDef->minimum = (value["minimum_value"].GetString());
                if (value.HasMember("maximum_value")) itemDef->maximum = (value["maximum_value"].GetString());
                if (value.HasMember("minimum_value_warning")) itemDef->miniwarning = (value["minimum_value_warning"].GetString());
                if (value.HasMember("maximum_value_warning")) itemDef->maxwarning = (value["maximum_value_warning"].GetString());
                if (value.HasMember("parameter_level"))
                {
                    QString levelStr = (value["parameter_level"].GetString());
                    bool success = false;
                    int level = levelStr.toInt(&success);
                    if (!success)
                        level = 3;
                    itemDef->level = level;
                }
                if (value.HasMember("value")) itemDef->valueStr = (value["value"].GetString());
                if (value.HasMember("enabled"))
                {
                    if (value["enabled"].IsBool())
                    {
                        if (value["enabled"].GetBool())
                        {
                            itemDef->enableStatus = "true";
                        }
                        else
                        {
                            itemDef->enableStatus = "false";
                        }
                    }
                    else
                    {
                        itemDef->enableStatus = (value["enabled"].GetString());
                    }
                }

                if (value.HasMember("children"))
                {
                    const rapidjson::Value& children = value["children"];
                    for (rapidjson::Value::ConstMemberIterator child = children.MemberBegin();
                        child != children.MemberEnd(); child++)
                    {
                        QString name = child->name.GetString();
                        const rapidjson::Value& childValue = child->value;

                        SettingItemDef* settingItemDef = new SettingItemDef(itemDef);
                        itemDef->addSettingItemDef(settingItemDef);
                        settingItemDef->name = name;
                        fillItemCall(childValue, settingItemDef, groupDef);
                    }
                }

                if (value.HasMember("options"))
                {
                    const rapidjson::Value& options = value["options"];
                    for (rapidjson::Value::ConstMemberIterator child = options.MemberBegin();
                        child != options.MemberEnd(); child++)
                    {
                        QString name = child->name.GetString();
                        QString values = child->value.GetString();

                        itemDef->options.insert(name, values);
                    }
                }
                if (itemDef->type == "optional_extruder" || itemDef->type == "extruder")
                {
                    itemDef->options.insert(QString("-1"), QString("Not overridden"));
                    itemDef->options.insert(QString("0"), QString("Extruder 1"));
                    itemDef->options.insert(QString("1"), QString("Extruder 2"));
                }
            };

            auto fillGroup = [&fillItemCall](const rapidjson::Value& value, SettingGroupDef* groupDef) {
                if (!value.IsObject()) return;

                if (value.HasMember("label")) groupDef->label = (value["label"].GetString());
                if (value.HasMember("description")) groupDef->description = (value["description"].GetString());

                if (value.HasMember("children"))
                {
                    const rapidjson::Value& children = value["children"];
                    for (rapidjson::Value::ConstMemberIterator child = children.MemberBegin();
                        child != children.MemberEnd(); child++)
                    {
                        QString name = child->name.GetString();
                        const rapidjson::Value& childValue = child->value;

                        SettingItemDef* settingItemDef = new SettingItemDef(groupDef);
                        settingItemDef->name = name;
                        groupDef->addSettingItemDef(settingItemDef);
                        fillItemCall(childValue, settingItemDef, groupDef);
                    }
                }
            };

            if (jsonDocument.HasMember("settings"))
            {
                const rapidjson::Value& settings = jsonDocument["settings"];
                int index = 0;
                for (rapidjson::Value::ConstMemberIterator settingGroup = settings.MemberBegin();
                    settingGroup != settings.MemberEnd(); settingGroup++)
                {
                    QString name = settingGroup->name.GetString();
                    const rapidjson::Value& value = settingGroup->value;

                    SettingGroupDef* settingGroupDef = new SettingGroupDef(parent);
                    settingGroupDef->order = index;
                    settingGroupDef->key = name;
                    defs.insert(name, settingGroupDef);

                    qDebug() << QString("JsonDef::parse SettingGroup [%1].").arg(name);
                    fillGroup(value, settingGroupDef); 
                    ++index;
                }
            }
        }
        
        qDebug() << QString("JsonDef::parse finished.");
	}
}
