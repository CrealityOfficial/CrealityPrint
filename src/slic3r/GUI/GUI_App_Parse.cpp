#include "GUI_App.hpp"
#include "GUI_Init.hpp"
#include "Tab.hpp"

#include "slic3r/Utils/ExportMetas.hpp"
#include <boost/algorithm/string.hpp>
#include <string>

namespace Slic3r{
namespace GUI {

    void app_export_meta(TabPrint *tab_print, TabFilament *tab_filament, TabPrinter *tab_printer, const std::string& version)
    {
        std::map<std::string, Utils::GroupInfo> group_infos;

        auto add_group = [&group_infos](const std::vector<PageShp>& pages, const std::string& filed) {            
            for (auto& page : pages) {
                for (auto& group : page->m_optgroups) {
                    for (auto& opt : group->opt_map()) {
                        std::string key = opt.first;
                        int pos = key.find_last_of("#");
                        if (pos < key.length() && pos >= 0) {
                            key = key.substr(0, pos);
                        }

                        auto iter = group_infos.find(key);
                        if(iter == group_infos.end())
                        {
                            Utils::GroupInfo info;
                            info.filed = filed;
                            info.main_group = page->title().ToStdString();
                            info.sub_group = group->title.ToStdString();
                            group_infos.insert(std::pair(key, info));
                        }else{
                            continue;
                        }
                    }
                }
            }
        };

        add_group(tab_print->get_pages(), "Profile");
        add_group(tab_filament->get_pages(), "Filament");
        add_group(tab_printer->get_pages(), "Printer");

        Utils::ExportParam param;
        param.translate = false;
        param.version = version;
        Utils::export_metas(group_infos, param);
    }

    void GUI_App::parse_args()
    {
        if(init_params && init_params->argc > 1)
        {
            std::string _arg1 = init_params->argv[1];
            if(boost::algorithm::starts_with(_arg1, "export_meta"))
            {
                std::vector<std::string> strs;
                boost::split(strs, _arg1, boost::is_any_of(";"), boost::token_compress_on);

                std::string version = "error";
                if(strs.size() > 1)
                    version = strs[1];
                
                TabPrint *tab_print = dynamic_cast<TabPrint*>(tabs_list.at(0));
                TabFilament *tab_filament = dynamic_cast<TabFilament*>(tabs_list.at(1));
                TabPrinter *tab_printer = dynamic_cast<TabPrinter*>(tabs_list.at(2));
                app_export_meta(tab_print, tab_filament, tab_printer, version);
            }
        }
    }
}
}