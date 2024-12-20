#include "crslice2/base/parametermeta.h"
#include <boost/filesystem.hpp>

#include "jsonhelper.h"
#include "ccglobal/log.h"

#ifdef USE_BINARY_JSON
#include "base.json.h"
#include "fdm_filament_common.json.h"
#include "fdm_machine_common.json.h"
#include "fdm_process_common.json.h"

#include "extruder_keys.json.h"
#include "machine_keys.json.h"
#include "material_keys.json.h"
#include "profile_keys.json.h"
#endif

#include "wrapper/orcaslicewrapper.h"

namespace crslice2
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
#if 0
        return parse_metas_map_impl(datas);
#else
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
#ifdef USE_CURA_META
            int count = 16;
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
#else
            int count = 3;
            const unsigned char* subs[3] = {
                fdm_filament_common,
                fdm_machine_common,
                fdm_process_common
            };
#endif
            for (int i = 0; i < count; ++i)
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
#endif
    }

    void getMetaKeys(MetaGroup metaGroup, std::vector<std::string>& keys)
    {
        keys.clear();
#if 0
        get_meta_keys_impl(metaGroup, keys);
#else
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
#endif
    }

    std::string engineVersion()
    {
        std::string ver = "orca-1.8.0";

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
            ver = "orca-" + std::string(baseDoc["version"].GetString());
        }
#endif
        return ver;
    }

    void exportParameterMeta()
    {
        export_metas_impl();
    }
}