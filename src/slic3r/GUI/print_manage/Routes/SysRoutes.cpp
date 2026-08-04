#include "SysRoutes.hpp"
#include "nlohmann/json.hpp"
#include "../AppUtils.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/LoginDialog.hpp"
#include "slic3r/GUI/SystemId/SystemId.hpp"

using namespace Slic3r::GUI;

namespace DM{
    SysRoutes::SysRoutes()
    {
        //
        this->Handler({"is_dark_theme"}, [](wxWebView*browse, const std::string& data, const nlohmann::json& json_data, const std::string cmd) {
            wxString lan = wxGetApp().current_language_code_safe();
            nlohmann::json commandJson;
            commandJson["command"] = "is_dark_theme";
            commandJson["data"] = wxGetApp().dark_mode();

            AppUtils::PostMsg(browse, commandJson);
            return true;
        });

        this->Handler({"get_system_id"}, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            nlohmann::json systemId;
            systemId["command"] = "get_system_id";
            systemId["data"]    = SystemId::get_system_id();
            AppUtils::PostMsg(browse, systemId);
            return true;
        });

        //
        this->Handler({"get_user"}, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            auto user = wxGetApp().get_user();
            std::string country_code = wxGetApp().app_config->get("region");
            unsigned pid = Slic3r::get_current_pid();

            nlohmann::json top_level_json = {
                {"bLogin", user.bLogin ? 1 : 0},
                {"token", user.token},
                {"userId", user.userId},
                {"region", country_code},
                {"pid", pid}
            };

            nlohmann::json commandJson = {
                {"command", "get_user"},
                {"data", top_level_json}
            };

            AppUtils::PostMsg(browse, commandJson);
            return true;
            });

        //
        this->Handler({ "common_openurl" }, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            wxLaunchDefaultBrowser(json_data["url"]);

            return true;
            });

        this->Handler({ "open_login_dialog" }, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            std::string url = json_data["url"];
            wxGetApp().ShowUserLogin(true,url);

            return true;
            });
        this->Handler({ "switch_to_tab" }, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            std::string tabname = json_data["tabName"];
            std::string pageName = json_data["pageName"];
            wxGetApp().switch_to_tab(tabname);
            if (!pageName.empty()) {
                wxGetApp().swith_community_sub_page(pageName);
            }
            return true;
            });
    
        

        this->Handler({ "get_lang" }, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            wxString lan = wxGetApp().current_frontend_language_code();
            nlohmann::json commandJson;
            commandJson["command"] = "get_lang";
            commandJson["data"] = lan.ToStdString();

            AppUtils::PostMsg(browse, commandJson);
            return true;
            });


        this->Handler({ "get_region" }, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            std::string country_code = wxGetApp().app_config->get("region");
            nlohmann::json commandJson;
            commandJson["command"] = "get_region";
            commandJson["data"] = country_code;

            AppUtils::PostMsg(browse, commandJson);
            return true;
            });

    }

}