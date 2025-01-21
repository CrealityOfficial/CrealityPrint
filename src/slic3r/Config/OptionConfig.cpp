#include "OptionConfig.h"
#include "nlohmann/json.hpp"
#include "libslic3r/Utils.hpp"
#include "boost/filesystem/path.hpp"
#include "slic3r/GUI/MainFrame.hpp"

using json = nlohmann::json;

struct OptionConfig::priv
{
    nlohmann::json m_jsonData;
    std::string    m_version;
    std::string    m_url;
    
    priv()
    {
        std::string config_path = Slic3r::var("\\process\\ProcessConfig.json");
        boost::filesystem::path ph(config_path);
        if (boost::filesystem::exists(ph))
        {
            std::ifstream file(config_path);
            if (file.is_open())
            {
                file >> m_jsonData;
                file.close();

                if (m_jsonData.contains("version"))
                {
                    m_version = m_jsonData["version"];
                    return;
                }
            }
        }
        
//        assert(false);
    }

    std::string replaceFirstSubstring(std::string str, const std::string& from, const std::string& to)
    {
        size_t start_pos = str.find(from);
        if (start_pos != std::string::npos) {
            str.replace(start_pos, from.length(), to);
        }
        return str;
    }

    OptionConfig::Option get_option(std::string key)
    {
        OptionConfig::Option data;
        if (m_jsonData.contains("config_options_group")) {
            std::string url = m_jsonData["config_options_group"]["url"];
            std::string lan = wxGetApp().app_config->get("language");
            if (lan != "zh_CN" && lan != "zh_TW") {
                url = replaceFirstSubstring(url, "/zh/", "/en/");
            }
            for (const auto& page : m_jsonData["config_options_group"]["data"]) {
                if (page.contains("option_group")) {
                    for (const auto& group : page["option_group"]) {
                        if (group.contains("options")) {
                            for (const auto& option : group["options"]) {
                                if (option.contains("key") && key == option["key"]) {
                                    data.key = key;
                                    data.tooltip     = option.contains("tooltip") ? option["tooltip"] : "";
                                    data.tooltip_img = option.contains("tooltip_img") ? option["tooltip_img"] : "";
                                    data.url         = url;
                                    data.url        += option.contains("url") ? option["url"] : "";

                                    return data;
                                }
                            }
                        }
                    }
                }
            }
        }

        return data;
    }

    std::string get_tooltip_img(std::string key)
    {
        if (m_jsonData.contains("config_options_group")) {
            for (const auto& page : m_jsonData["config_options_group"]["data"]) {
                if (page.contains("option_group")) {
                    for (const auto& group : page["option_group"]) {
                        if (group.contains("options")) {
                            for (const auto& option : group["options"]) {
                                if (option.contains("key") && key == option["key"]) {
                                    return option.contains("tooltip_img") ? option["tooltip_img"].get<std::string>() : std::string();
                                }
                            }
                        }
                    }
                }
            }
        }

        return std::string();
    }

    std::string get_group_icon(std::string group_name)
    {
        if (m_jsonData.contains("config_options_group")) {
            for (const auto& page : m_jsonData["config_options_group"]) {
                if (page.contains("option_group")) {
                    for (const auto& group : page["option_group"]) {
                        if (group.contains("name") && group.contains("icon")) {
                            if (group["name"] == group_name)
                            {
                                return group["icon"];
                            }
                        }
                    }
                }
            }
        }

        return std::string();
    }
};

OptionConfig::OptionConfig() : p(new priv())
{
}

OptionConfig::~OptionConfig()
{
}

OptionConfig& OptionConfig::Ins()
{   
    static OptionConfig obj;
    return obj;
}

OptionConfig::Option OptionConfig::get_option(std::string key) { return p->get_option(key); }

std::string OptionConfig::get_group_icon(std::string group) { return p->get_group_icon(group); }
