#include "DeviceMgrRoutes.hpp"
#include "nlohmann/json.hpp"
#include "../AppUtils.hpp"
#include "../DataCenter.hpp"
#include "../../GUI.hpp"
#include "../PrinterMgr.hpp"
#include "Http.hpp"
#include "cereal/external/base64.hpp"

namespace DM{
    DeviceMgrRoutes::DeviceMgrRoutes()
    {
        //get all device
        auto fn_get_device = [](std::string cmd, wxWebView* browse, const std::string& data, const nlohmann::json& json_data) {
            
            return true;
        };

        this->Handler("request_all_device", [](wxWebView* browse, const std::string& data, const nlohmann::json& json_data) {
            nlohmann::json commandJson;
            commandJson["command"] = "request_all_device";
            commandJson["data"] = DataCenter::Ins().GetData();
            AppUtils::PostMsg(browse, wxString::Format("window.handleStudioCmd('%s');", commandJson.dump(-1, ' ', true)).ToStdString());
            return true;
        });

        this->Handler("get_devices", [](wxWebView* browse, const std::string& data, const nlohmann::json& json_data) {
            nlohmann::json commandJson;
            commandJson["command"] = "get_devices";
            commandJson["data"] = DataCenter::Ins().GetData();
            AppUtils::PostMsg(browse, wxString::Format("window.handleStudioCmd('%s');", commandJson.dump(-1, ' ', true)).ToStdString());
            return true;
        });
            //

        //for device module 
        this->Handler("init_device", [](wxWebView* browse, const std::string& data, const nlohmann::json& json_data) {
            nlohmann::json commandJson;
            commandJson["command"] = "init_device";
            commandJson["data"] = Slic3r::GUI::DeviceMgr::Ins().GetData();
            AppUtils::PostMsg(browse, wxString::Format("window.handleStudioCmd('%s');", commandJson.dump(-1, ' ', true)).ToStdString());

            return true;
        });
        
        //for device module 
        this->Handler("get_device_merge_state", [](wxWebView* browse, const std::string& data, const nlohmann::json& json_data) {
            nlohmann::json commandJson;
            commandJson["command"] = "get_device_merge_state";
            commandJson["data"] = Slic3r::GUI::DeviceMgr::Ins().IsMergeState();
            AppUtils::PostMsg(browse, wxString::Format("window.handleStudioCmd('%s');", commandJson.dump(-1, ' ', true)).ToStdString());

            return true;
            });

        //for device module 
        this->Handler("set_device_merge_state", [](wxWebView* browse, const std::string& data, const nlohmann::json& json_data) {
            if(json_data.contains("state"))
            {
                Slic3r::GUI::DeviceMgr::Ins().SetMergeState(json_data["state"].get<bool>());
            }

            return true;
            });

        // web rtc local get 
        this->Handler("get_webrtc_local_param", [](wxWebView* browse, const std::string& data, const nlohmann::json& json_data) {

            std::string url = json_data["url"].get<std::string>();
            std::string  sdp  = json_data["sdp"].get<std::string>();
            Slic3r::Http http = Slic3r::Http::post(url);
         
            nlohmann::json j; 
            j["type"] = "offer";
            j["sdp"]  = sdp;

            std::string d    = j.dump();
            std::string e = cereal::base64::encode((unsigned char const*) d.c_str(), d.length());

            http.header("Content-Type", "application/json")
                .set_post_body(e)
                .on_complete([&](std::string body, unsigned status) {
                    if (status != 200) {
                        return;
                    }
                    
                    nlohmann::json data;
                    data["body"] = body;
                    data["ret"] = true;

                    nlohmann::json commandJson;
                    commandJson["command"] = "get_webrtc_local_param";
                    commandJson["data"]    = data;
                     
                    AppUtils::PostMsg(browse, wxString::Format("window.handleStudioCmd('%s');", commandJson.dump(-1, ' ', true)).ToStdString());

                })
                .on_error([&](std::string body, std::string error, unsigned status) {
                    nlohmann::json data;
                    data["body"] = body;
                    data["ret"]  = false;

                    nlohmann::json commandJson;
                    commandJson["command"] = "get_webrtc_local_param";
                    commandJson["data"]    = data;
                    AppUtils::PostMsg(browse,
                                      wxString::Format("window.handleStudioCmd('%s');", commandJson.dump(-1, ' ', true)).ToStdString());
                 })
                .perform_sync();

            return true;
        });
        
    }
}