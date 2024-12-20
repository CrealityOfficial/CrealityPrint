#include "crslice/base/parametermeta.h"
#include <boost/filesystem.hpp>

#include "jsonhelper.h"
#include "ccglobal/log.h"

#ifdef USE_BINARY_JSON
#include "base.json.h"
#include "blackmagic.json.h"
#include "command_line_settings.json.h"
#include "cooling.json.h"
#include "dual.json.h"
#include "experimental.json.h"
#include "infill.json.h"
#include "machine.json.h"
#include "material.json.h"
#include "meshfix.json.h"
#include "platform_adhesion.json.h"
#include "resolution.json.h"
#include "shell.json.h"
#include "special.json.h"
#include "speed.json.h"
#include "support.json.h"
#include "travel.json.h"

#include "extruder_keys.json.h"
#include "machine_keys.json.h"
#include "material_keys.json.h"
#include "profile_keys.json.h"
#endif

namespace crslice
{
	ParameterMetas::ParameterMetas()
	{
	}

	ParameterMetas::~ParameterMetas()
	{
        clear();
	}

    ParameterMeta* ParameterMetas::find(const std::string& key)
    {
        MetasMapIter iter = mm.find(key);
        if (iter != mm.end())
            return iter->second;
        
        LOGE("ParameterMetas::find key error [%s].", key.c_str());
        return nullptr;
    }

	void ParameterMetas::initializeBase(const std::string& path)
	{
		std::string baseFile = path + "/base.json";

		rapidjson::Document baseDoc;
		if (!openJson(baseDoc, baseFile))
		{
			LOGE("ParameterMetas::initializeBase error. [%s] not contain base.json", path.c_str());
			return;
		}

        if (baseDoc.HasMember("subs") && baseDoc["subs"].IsArray())
        {
            const rapidjson::Value& value = baseDoc["subs"];
            for (rapidjson::Value::ConstValueIterator it = value.Begin(); it != value.End(); ++it)
            {
                std::string sub = it->GetString();
                std::string json = path + "/" + sub;

                rapidjson::Document subDoc;
                if (!openJson(subDoc, json))
                {
                    LOGE("ParameterMetas::initializeBase parse sub. [%s] error.", sub.c_str());
                    continue;
                }

                processSub(subDoc, mm);
            }
        }
        else {
            LOGE("ParameterMetas::initializeBase base.json no subs.");
        }
	}

	ParameterMetas* ParameterMetas::createInherits(const std::string& fileName)
	{
        std::string directory = boost::filesystem::path(fileName).parent_path().string();

        rapidjson::Document machineDoc;
        if (!openJson(machineDoc, fileName))
        {
            LOGE("ParameterMetas::createInherits error. [%s] not valid.", fileName.c_str());
            return nullptr;
        }

        ParameterMetas* newMetas = copy();
        if (machineDoc.HasMember("inherits") && machineDoc["inherits"].IsString())
        {
            std::string inherits = machineDoc["inherits"].GetString();
            processInherit(inherits, fileName, *newMetas);
        }
        return newMetas;
	}

    void ParameterMetas::clear()
    {
        for (MetasMapIter it = mm.begin();
            it != mm.end(); ++it)
            delete it->second;
        mm.clear();
    }

    ParameterMetas* ParameterMetas::copy()
    {
        ParameterMetas* nm = new ParameterMetas();
        for (MetasMapIter it = mm.begin();
            it != mm.end(); ++it)
            nm->mm.insert(MetasMap::value_type(it->first, new ParameterMeta(*it->second)));
        return nm;
    }

    void saveKeysJson(const std::vector<std::string>& keys, const std::string& fileName)
    {
        std::string content = createKeysContent(keys);

        saveJson(fileName, content);
    }

    void parseMetasMap(MetasMap& datas)
    {
#ifdef USE_BINARY_JSON
        rapidjson::Document baseDoc;
        baseDoc.Parse((const char*)base);
        if (baseDoc.HasParseError())
        {
            LOGE("ParameterMetas::parseMetasMap parse base error. [%d].", (int)baseDoc.GetParseError());
            return;
        }

        if (baseDoc.HasMember("subs") && baseDoc["subs"].IsArray())
        {
            const unsigned char* subs[16] = {
                blackmagic,
                command_line_settings,
                cooling,
                dual,
                experimental,
                infill,
                machine,
                material,
                meshfix,
                platform_adhesion,
                resolution,
                shell,
                special,
                speed,
                support,
                travel
            };
            for (int i = 0; i < 16; ++i)
            {
                const unsigned char* str = subs[i];
                rapidjson::Document subDoc;
                subDoc.Parse((const char*)str);
                if (subDoc.HasParseError())
                {
                    LOGE("parseMetasMap parse sub. [%d] error.", (int)subDoc.GetParseError());
                    continue;
                }

                processSub(subDoc, datas);
            }
        }
        else {
            LOGE("parseMetasMap base.json no subs.");
        }
#else
        LOGE("parseMetasMap::Binary not support.");
#endif
    }

    void getMetaKeys(MetaGroup metaGroup, std::vector<std::string>& keys)
    {
        keys.clear();
#ifdef USE_BINARY_JSON
        const unsigned char* data = machine_keys;
        if (metaGroup == MetaGroup::emg_extruder)
            data = extruder_keys;
        else if (metaGroup == MetaGroup::emg_material)
            data = material_keys;
        else if (metaGroup == MetaGroup::emg_profile)
            data = profile_keys;

        const unsigned char* str = data;
        rapidjson::Document keyDoc;
        keyDoc.Parse((const char*)str);
        if (keyDoc.HasParseError())
        {
            LOGE("getMetaKeys parse sub. [%d] error.", (int)keyDoc.GetParseError());
            return;
        }

        processKeys(keyDoc, keys);
#else
        LOGE("getMetaKeys::Binary not support.");
#endif
    }

    std::string engineVersion()
    {
        std::string ver = "cura-5.2.0";

#ifdef USE_BINARY_JSON
        rapidjson::Document baseDoc;
        baseDoc.Parse((const char*)base);
        if (baseDoc.HasParseError())
        {
            LOGE("ParameterMetas::parseMetasMap parse base error. [%d].", (int)baseDoc.GetParseError());
            return ver;
        }

        if (baseDoc.HasMember("version"))
        {
            ver = "cura-" + std::string(baseDoc["version"].GetString());
        }
#endif
        return ver;
    }
}