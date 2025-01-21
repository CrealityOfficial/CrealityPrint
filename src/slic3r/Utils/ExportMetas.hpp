#ifndef slic3r_GUI_Utils_Export_Metas_hpp_
#define slic3r_GUI_Utils_Export_Metas_hpp_
#include <map>

namespace Slic3r {
namespace Utils {

typedef enum {
    eprofile_keys,    
    extruder_keys,       
    emachine_keys,        
    ematerial_keys, 
    eCount
} EnumExportTypes;

void export_metas_impl(std::map<std::string, std::pair<std::string, std::string>>& optionsm,const EnumExportTypes type, 
        const std::string& cur_version,const std::string& pref_version = "");


    struct GroupInfo{
        std::string filed;
        std::string main_group;
        std::string sub_group;
    };

    struct ParameterMeta{
        std::string filed;
        std::string main_group;
        std::string sub_group;

        std::string key;
        std::string name;
        std::string type;
        std::string description;
        std::string change_info;
    };

    struct MetaDatas{
        std::string build_date;
        std::string current_commit;
        std::vector<ParameterMeta> metas;
    }; 

    struct ExportParam{
        bool translate = false;
        std::string version;
    };

    void export_metas(const std::map<std::string, GroupInfo>& group_info, const ExportParam& param);
    void export_metas(const MetaDatas& metas, const std::string& version);
    void export_csv(const std::vector<std::vector<std::string>>& datas, const std::string& file_name);
} // Utils
} // Slic3r

#endif /* slic3r_GUI_Utils_Export_Metas_hpp_ */
