#include "ExportMetas.hpp"
#include "libslic3r/Preset.hpp"
#include "../GUI/I18N.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/Time.hpp"

#include <string>
#include <vector>
#include <set>

#include "../../buildinfo.h"

namespace Slic3r {
namespace Utils {

struct mata_category
{
    std::string type1;
    std::string type2;
    std::string key;
    std::string key_type;
    std::string description;
    std::string label;
};

void loadBaseMetas(std::string file_name, std::set<std::string>& baseKeys)
{
    boost::nowide::ifstream file(file_name.c_str());
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            baseKeys.insert(line);
        }
        file.close();
    }
}

void writeBaseMetas(std::string file_name, std::vector<std::string> baseKeys)
{
    boost::nowide::ofstream c;
    c.open(file_name.c_str(), std::ios::out | std::ios::trunc);

    for (auto& key : baseKeys) {
        c << key << std::endl;
    }
    c.close();
}

void export_metas_impl(std::map<std::string, std::pair<std::string, std::string>>& optionsm,const EnumExportTypes type, const std::string& cur_version, const std::string& pref_version)
{
    boost::filesystem::path path_to_binary = __FILE__; 
    path_to_binary                         = path_to_binary.parent_path().parent_path().parent_path().parent_path();
    path_to_binary += "/parameter_history/";

    std::string _pref_version = "";
    std::string _cur_version  = "";
    if (!pref_version.empty())
        _pref_version = path_to_binary.string() + pref_version;

    _cur_version = path_to_binary.string() + cur_version;

    std::set<std::string> baseKeys;
    if (!pref_version.empty()) {
        std::string file_name = "";
        switch (type) {
        case EnumExportTypes::eprofile_keys: file_name = _pref_version + "/profile_keys.txt"; break;
        case EnumExportTypes::emachine_keys: file_name = _pref_version + "/machine_keys.txt"; break;
        case EnumExportTypes::ematerial_keys: file_name = _pref_version + "/material_keys.txt"; break;
        default: file_name = _pref_version + "extruder_keys.txt"; break;
        }
        loadBaseMetas(file_name, baseKeys);
    } 
    
    {
        switch (type) {
        case EnumExportTypes::eprofile_keys: writeBaseMetas(_cur_version + "/profile_keys.txt", Preset::print_options()); break;
        case EnumExportTypes::emachine_keys: writeBaseMetas(_cur_version + "/machine_keys.txt", Preset::printer_options()); break;
        case EnumExportTypes::ematerial_keys: writeBaseMetas(_cur_version + "/material_keys.txt", Preset::filament_options()); break;
        default: writeBaseMetas(_cur_version + "extruder_keys.txt", print_config_def.extruder_option_keys()); break;
        }
    }

    std::map<std::string, mata_category> full_configs;
    auto export_metas_keys = [&](std::string file_name, std::vector<std::string> keys, std::set<std::string>& baseKeys) {
        boost::nowide::ofstream c;
        c.open(file_name.c_str(), std::ios::out | std::ios::trunc);

        for (auto& key : keys) {
            auto iter = full_configs.find(key);
            if (iter != full_configs.end()) {
                auto _iter = baseKeys.find(key);
                bool hasKey = _iter == baseKeys.end() ? false : true;

                wxString wstr = _L(iter->second.description);
                wstr.Replace("\n", ";");
                wstr.Replace(",", ";");

                wxString wstr2 = _L(iter->second.label);
                wstr2.Replace(",", ";");
                c << _L(iter->second.type1) << "," << _L(iter->second.type2) << "," << iter->second.key << "," << wstr2
                  << "," << iter->second.key_type << "," << wstr << "," << (hasKey ?_L("No") : _L("Yes")) << std::endl;
            } else {
                c << _L(key) << std::endl;
            }
        }

        c.close();
    };

    DynamicPrintConfig new_full_config;
    const ConfigDef*   _def = new_full_config.def();
    {
        PrintRegionConfig        printRegionConfig;
        PrintObjectConfig        printObjectConfig;
        PrintConfig              printConfig;

        // record all the key-values
        for (const std::string& opt_key : _def->keys()) {
            const ConfigOptionDef* optDef = _def->get(opt_key);
            if (!optDef || (optDef->printer_technology == Slic3r::ptSLA))
                continue;

            std::string type = "coNone";
            switch (optDef->type) {
            case coFloat: type = "coFloat"; break;
            case coFloats: type = "coFloats"; break;
            case coInt: type = "coInt"; break;
            case coInts: type = "coInts"; break;
            case coString: type = "coString"; break;
            case coStrings: type = "coStrings"; break;
            case coPercent: type = "coPercent"; break;
            case coPercents: type = "coPercents"; break;
            case coFloatOrPercent: type = "coFloatOrPercent"; break;
            case coFloatsOrPercents: type = "coFloatsOrPercents"; break;
            case coPoint: type = "coPoint"; break;
            case coPoints: type = "coPoints"; break;
            case coPoint3: type = "coPoint3"; break;
            case coBool: type = "coBool"; break;
            case coBools: type = "coBools"; break;
            case coEnum: type = "coEnum"; break;
            case coEnums: type = "coEnums"; break;
            };

            mata_category _diff_category;

            auto iter = optionsm.find(opt_key);
            if (iter != optionsm.end()) {
                _diff_category.type1 = iter->second.first;
                _diff_category.type2 = iter->second.second;
            }
            if (_diff_category.type1.empty()) 
                _diff_category.type1      = optDef->category;
            _diff_category.key         = opt_key;
            _diff_category.key_type    = type;
            _diff_category.description = optDef->tooltip;
            _diff_category.label       = optDef->label;
            full_configs.insert(std::pair<std::string, mata_category>(opt_key, _diff_category));
        }
    }

    switch (type) {
    case EnumExportTypes::eprofile_keys: export_metas_keys(_cur_version + "/profile_keys.csv", Preset::print_options(), baseKeys); break;
    case EnumExportTypes::emachine_keys: export_metas_keys(_cur_version + "/machine_keys.csv", Preset::printer_options(), baseKeys); break;
    case EnumExportTypes::ematerial_keys: export_metas_keys(_cur_version + "/material_keys.csv", Preset::filament_options(), baseKeys); break;
    default: export_metas_keys(_cur_version + "/extruder_keys.csv", print_config_def.extruder_option_keys(), baseKeys); break;
    }
}



//////////////////////////////////////////////////
    void export_metas(const std::map<std::string, GroupInfo>& group_info, const ExportParam& param)
    {
        MetaDatas metas;
        metas.build_date   = "Build Time : " + Slic3r::Utils::utc_timestamp();
        metas.current_commit = std::string("Commit :") + MAIN_GIT_HASH;
        std::string version = param.version;

#define _EL(x) std::string(_L(x.c_str()).c_str())
        DynamicPrintConfig full_config  ;
        const ConfigDef*   _def = full_config.def();
        {
            for (const std::string& opt_key : _def->keys()) {
                const ConfigOptionDef* optDef = _def->get(opt_key);
                if (!optDef || (optDef->printer_technology == Slic3r::ptSLA))
                    continue;

                ParameterMeta meta;
                std::string type = "coNone";
                switch (optDef->type) {
                    case coFloat: type = "coFloat"; break;
                    case coFloats: type = "coFloats"; break;
                    case coInt: type = "coInt"; break;
                    case coInts: type = "coInts"; break;
                    case coString: type = "coString"; break;
                    case coStrings: type = "coStrings"; break;
                    case coPercent: type = "coPercent"; break;
                    case coPercents: type = "coPercents"; break;
                    case coFloatOrPercent: type = "coFloatOrPercent"; break;
                    case coFloatsOrPercents: type = "coFloatsOrPercents"; break;
                    case coPoint: type = "coPoint"; break;
                    case coPoints: type = "coPoints"; break;
                    case coPoint3: type = "coPoint3"; break;
                    case coBool: type = "coBool"; break;
                    case coBools: type = "coBools"; break;
                    case coEnum: type = "coEnum"; break;
                    case coEnums: type = "coEnums"; break;
                };

                meta.type        = type;
                meta.key         = opt_key;
                meta.name        = param.translate ? _EL(optDef->label) : optDef->label;
                meta.description = param.translate ? _EL(optDef->tooltip) : optDef->tooltip;

                auto iter = group_info.find(opt_key);
                if(iter != group_info.end())
                {
                    meta.filed       = param.translate ? _EL(iter->second.filed) : iter->second.filed;
                    meta.main_group = param.translate ? _EL(iter->second.main_group) : iter->second.main_group;
                    meta.sub_group   = param.translate ? _EL(iter->second.sub_group) : iter->second.sub_group;
                }

                metas.metas.push_back(meta);
            }
        }
#undef _EL(x)

        std::sort(metas.metas.begin(), metas.metas.end(), [](const ParameterMeta& a, const ParameterMeta& b) {
            return std::tie(a.filed, a.main_group, a.sub_group, a.key) 
                < std::tie(b.filed, b.main_group, b.sub_group, b.key);
        });
        export_metas(metas, version);
    }

    void export_metas(const MetaDatas& metas, const std::string& version)
    {
        std::vector<std::vector<std::string>> datas;
        int line = (int)metas.metas.size() + 2;
        datas.resize(line);

        for(int i = 0; i < line; i++) {
            std::vector<std::string>& data = datas[i];
            data.resize(8);

            if(i == 0)
            {//meta data
                data[0] = metas.build_date;
                data[1] = metas.current_commit;
            } else if(i == 1)
            {//header
                data[0] = "Field";
                data[1] = "Main Group";
                data[2] = "Sub Group";
                data[3] = "Key";
                data[4] = "Name";
                data[5] = "Type";
                data[6] = "Description";
                data[7] = "Change Info";
            } else {
                const ParameterMeta& meta = metas.metas[i - 2];
                data[0] = meta.filed;
                data[1] = meta.main_group;
                data[2] = meta.sub_group;
                data[3] = meta.key;
                data[4] = meta.name;
                data[5] = meta.type;
                data[6] = meta.description;
                data[7] = meta.change_info;
            }
        }

        boost::filesystem::path path_to_binary = __FILE__; 
        path_to_binary                         = path_to_binary.parent_path().parent_path().parent_path().parent_path();
        path_to_binary += "/parameter_history/";

        std::string _version = version;
        if(_version.empty())
            _version = "error";
        std::string file_name = path_to_binary.string() + _version + ".metas.csv";
        export_csv(datas, file_name);
    }

    void export_csv(const std::vector<std::vector<std::string>>& datas, const std::string& file_name)
    {
        boost::nowide::ofstream c;
        c.open(file_name.c_str(), std::ios::out | std::ios::trunc);

        for(const std::vector<std::string>& data : datas) {
            for(const std::string& str : data) {

                std::string _str = str;
                std::replace(_str.begin(), _str.end(), '\n', ';');
                std::replace(_str.begin(), _str.end(), ',', ';');

                c << _str << ",";
            }
            c << std::endl;
        }

        c.close(); 
    }
} // namespace Utils
} // namespace Slic3r
