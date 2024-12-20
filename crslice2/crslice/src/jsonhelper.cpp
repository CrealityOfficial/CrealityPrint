#include "jsonhelper.h"
#include <fstream>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/error/en.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/writer.h>

#include "ccglobal/log.h"

#define META_LABEL "label"
#define META_DESCRIPTION "description"
#define META_TYPE "type"
#define META_DEFAULT_VALUE "default_value"
#define META_VALUE "value"
#define META_ENABLED "enabled"

namespace crslice2
{
    bool openJson(rapidjson::Document& doc, const std::string& fileName)
	{
        LOGI("try to open JSON file: %s", fileName.c_str());

        std::ifstream ifs(fileName.c_str());
        if (!ifs.is_open())
        {
            LOGE("Couldn't open JSON file: %s", fileName.c_str());
            return false;
        }

        rapidjson::IStreamWrapper isw(ifs);
        doc.ParseStream(isw);
        if (doc.HasParseError())
        {
            LOGE("Error parsing JSON (offset %d): %s", (int)doc.GetErrorOffset(), GetParseError_En(doc.GetParseError()));
            return false;
        }

        return true;
	}

    void processSub(const rapidjson::Document& doc, MetasMap& metas)
    {
        for (rapidjson::Value::ConstMemberIterator child = doc.MemberBegin();
            child != doc.MemberEnd(); child++)
        {
            std::string name = child->name.GetString();
            const rapidjson::Value& childValue = child->value;

            MetasMap::iterator it = metas.find(name);
            if (it != metas.end())
            {
                LOGE("processSub dumplicate key [%s]", name.c_str());
                continue;
            }

            ParameterMeta meta;

            meta.name = name;
            processMeta(childValue, meta);

            metas.insert(MetasMap::value_type(name, new ParameterMeta(meta)));
        }
    }

    void processMeta(const rapidjson::Value& value, ParameterMeta& meta)
    {
        if (!value.IsObject())
            return;

        if (value.HasMember(META_LABEL))
            meta.label = (value[META_LABEL].GetString());

        if (value.HasMember(META_DESCRIPTION))
            meta.description = (value[META_DESCRIPTION].GetString());

        if (value.HasMember("unit"))
            meta.unit = value["unit"].GetString();

        if (value.HasMember(META_TYPE))
            meta.type = (value[META_TYPE].GetString());

        if (value.HasMember(META_DEFAULT_VALUE))
            meta.default_value = (value[META_DEFAULT_VALUE].GetString());

        if (value.HasMember("minimum_value"))
            meta.minimum_value = (value["minimum_value"].GetString());
        if (value.HasMember("maximum_value"))
            meta.maximum_value = (value["maximum_value"].GetString());
        if (value.HasMember("minimum_value_warning"))
            meta.minimum_value_warning = (value["minimum_value_warning"].GetString());
        if (value.HasMember("maximum_value_warning"))
            meta.maximum_value_warning = (value["maximum_value_warning"].GetString());

        if (value.HasMember("settable_globally"))
            meta.settable_globally = value["settable_globally"].GetString();
        if (value.HasMember("settable_per_extruder"))
            meta.settable_per_extruder = value["settable_per_extruder"].GetString();
        if (value.HasMember("settable_per_mesh"))
            meta.settable_per_mesh = value["settable_per_mesh"].GetString();
        if (value.HasMember("settable_per_meshgroup"))
            meta.settable_per_meshgroup = value["settable_per_meshgroup"].GetString();

        if (value.HasMember(META_VALUE))
            meta.value = (value[META_VALUE].GetString());

        if (value.HasMember(META_ENABLED))
        {
            const rapidjson::Value& enabledValue = value[META_ENABLED];
            if (enabledValue.IsBool())
            {
                if (enabledValue.GetBool())
                {
                    meta.enabled = "true";
                }
                else
                {
                    meta.enabled = "false";
                }
            }
            else
            {
                meta.enabled = (enabledValue.GetString());
            }
        }

        if (value.HasMember("options"))
        {
            const rapidjson::Value& options = value["options"];
            for (rapidjson::Value::ConstMemberIterator child = options.MemberBegin();
                child != options.MemberEnd(); child++)
            {
                std::string name = child->name.GetString();
                std::string values = child->value.GetString();

                meta.options.push_back(OptionValue(name, values));
            }
        }
        if (meta.type == "optional_extruder" || meta.type == "extruder")
        {
            meta.options.push_back(OptionValue(std::string("-1"), std::string("Not overridden")));
            meta.options.push_back(OptionValue(std::string("0"), std::string("Extruder 1")));
            meta.options.push_back(OptionValue(std::string("1"), std::string("Extruder 2")));
        }
    }

    void processKeys(const rapidjson::Document& doc, std::vector<std::string>& keys)
    {
        for (rapidjson::Value::ConstMemberIterator child = doc.MemberBegin();
            child != doc.MemberEnd(); child++)
        {
            std::string name = child->name.GetString();
            const rapidjson::Value& childValue = child->value;

            if (name == "keys")
            {
                if (!childValue.IsArray())
                    return;

                int size = childValue.Size();
                keys.reserve(size);

                for (int i = 0; i < size; i++)
                {
                    keys.push_back(childValue[i].GetString());
                }
                return;
            }
        }
    }

    void processInherit(const std::string& fileName, const std::string& directory, ParameterMetas& metas)
    {

    }

    std::string createKeysContent(const std::vector<std::string>& keys)
    {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

        writer.StartObject();
        writer.Key("keys");
        writer.StartArray();

        for (const std::string& key : keys)
            writer.String(key.c_str());
        writer.EndArray();
        writer.EndObject();
        return std::string(buffer.GetString());
    }

    void saveJson(const std::string& fileName, const std::string& content)
    {
        rapidjson::Document doc;

        std::string writeStr = content;
        doc.Parse(writeStr.c_str());
        FILE* f = fopen(fileName.c_str(), "wb");
        if (f)
        {
            char buffer[65535];
            rapidjson::FileWriteStream os(f, buffer, sizeof(buffer));
            rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(os);
            doc.Accept(writer);
        }
        fclose(f);
    }
}
