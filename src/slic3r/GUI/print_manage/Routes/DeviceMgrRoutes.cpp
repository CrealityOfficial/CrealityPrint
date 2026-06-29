#include "DeviceMgrRoutes.hpp"
#include "nlohmann/json.hpp"
#include "../AppUtils.hpp"
#include "../data/DataCenter.hpp"
#include "../../GUI.hpp"
#include "../PrinterMgr.hpp"
#include "Http.hpp"
#include "cereal/external/base64.hpp"
#include "slic3r/GUI/Notebook.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "../AccountDeviceMgr.hpp"
#include "slic3r/GUI/print_manage/RemotePrinterManager.hpp"
#include "slic3r/GUI/print_manage/Utils.hpp"
#include "libslic3r/GCode/GCodeProcessor.hpp"
#include "slic3r/GUI/PartPlate.hpp"
#include "libslic3r/Preset.hpp"
#include "libslic3r/Thread.hpp"
#include "slic3r/GUI/AnalyticsDataUploadManager.hpp"
#include "libslic3r/Time.hpp"
#include "slic3r/GUI/PhysicalPrinter.hpp"
#include "slic3r/GUI/SystemId/SystemId.hpp"
#include "libslic3r/Utils.hpp"
#include "slic3r/GUI/simple/MCPChatPanel.hpp"
#include <wx/weakref.h>
#include <boost/log/trivial.hpp>

using namespace Slic3r;

namespace {
    std::unordered_map<std::string, int> g_last_device_states;
    
    /**
     * @brief Check if the state is a failure state (for future extension)
     */
    inline bool IsFailureState(int state) {
        return (state == 3);  // TODO: change to (state == 3 || state == 4) when needed
    }
}

namespace DM {
    DeviceMgrRoutes::DeviceMgrRoutes()
    {
        this->Handler({ "init_device" }, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            nlohmann::json commandJson;
            commandJson["command"] = "init_device";
            commandJson["data"] = DM::DeviceMgr::Ins().GetData();
            std::string commandStr = commandJson.dump(-1,' ',true);
            wxString strJS = wxString::Format("window.handleStudioCmd('%s');", RemotePrint::Utils::url_encode(commandStr));
            AppUtils::PostMsg(browse, strJS.ToStdString());
            return true;
            });

        this->Handler({"get_system_id"}, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            nlohmann::json commandJson;
            commandJson["command"] = "get_system_id";
            commandJson["data"]    = SystemId::get_system_id();
            std::string commandStr = commandJson.dump(-1, ' ', true);
            wxString    strJS      = wxString::Format("window.handleStudioCmd('%s');", RemotePrint::Utils::url_encode(commandStr));
            AppUtils::PostMsg(browse, strJS.ToStdString());
            return true;
        });

        this->Handler({ "request_all_device", "get_devices" }, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            nlohmann::json commandJson;
            commandJson["command"] = cmd;
            commandJson["data"] = DataCenter::Ins().GetData();
            std::string commandStr = commandJson.dump(-1,' ',true);
            wxString strJS = wxString::Format("window.handleStudioCmd('%s');", RemotePrint::Utils::url_encode(commandStr));
            AppUtils::PostMsg(browse, strJS.ToStdString());
            return true;
            });

        // form device, real time update of the data
        this->Handler({ "update_devices" }, [this](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            DM::DataCenter::Ins().update_data(json_data);
            Slic3r::GUI::NotifyAIChatDeviceStatusChanged();
            
            // 7.1.0版本先屏蔽此事件上报
            //check_and_send_print_failure_events(json_data);
            
            if (DM::DataCenter::Ins().is_current_device_changed()) {
                wxPostEvent(wxGetApp().plater(), wxCommandEvent(EVT_CURRENT_DEVICE_CHANGED));
                Slic3r::GUI::NotifyAIChatSceneChanged();
            }
            if (wxGetApp().obj_list()) {
                wxPostEvent(wxGetApp().obj_list(), wxCommandEvent(EVT_UPDATE_DEVICES));
            }
            if(wxGetApp().mainframe->get_printer_mgr_view()->should_upload_device_info()) {
                // upload analytics data here
                AnalyticsDataUploadManager::getInstance().triggerUploadTasks(AnalyticsUploadTiming::ON_SOFTWARE_LAUNCH,
                                                                        {AnalyticsDataEventType::ANALYTICS_DEVICE_INFO});
                AnalyticsEventPayload payload;
                payload.type = AnalyticsDataEventType::ANALYTICS_DEVICE_INFO;
                AnalyticsDataUploadManager::getInstance().triggerUploadTasksWithPayload(payload);
            }

            return true;
            });

        //for device module 
        this->Handler({ "get_device_merge_state" }, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            nlohmann::json commandJson;
            commandJson["command"] = "get_device_merge_state";
            commandJson["data"] = DM::DeviceMgr::Ins().IsMergeState();
            AppUtils::PostMsg(browse, commandJson);

            return true;
            });

        this->Handler({"sync_mappinp_cfs_filament"},
                      [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
                          wxPostEvent(Slic3r::GUI::wxGetApp().plater(), wxCommandEvent(Slic3r::GUI::EVT_AUTO_SYNC_CURRENT_DEVICE_FILAMENT));
                return true;
            });

        this->Handler({ "set_current_plate_index" }, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            int index = json_data["plateIndex"].get<int>();
            if(wxGetApp().plater()->get_partplate_list().get_curr_plate_index()!=index)
            {
                wxGetApp().plater()->select_sliced_plate(index);
            }
            
            Slic3r::GCodeProcessorResult* current_result = wxGetApp().plater()->get_partplate_list().get_current_slice_result();
            bool is_plate_loaded = current_result && !current_result->filename.empty() && current_result->image_data.size() > 0;
            nlohmann::json commandJson;
            commandJson["command"] = "set_current_plate_index";
            commandJson["result"] = is_plate_loaded ? 1 : 0;
            commandJson["plateIndex"] = index;
            commandJson["isPlateLoaded"] = is_plate_loaded;
            commandJson["isOnlyGcodeMode"] = wxGetApp().plater()->only_gcode_mode();
            AppUtils::PostMsg(browse, commandJson);
            
            return true;
            });
            

        //for device module 
        this->Handler({ "set_device_merge_state" }, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            if (json_data.contains("state"))
            {
                DM::DeviceMgr::Ins().SetMergeState(json_data["state"].get<bool>());
            }

            return true;
            });

        // web rtc local get 
        this->Handler({ "get_webrtc_local_param" }, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            wxWeakRef<wxWebView> weak_browse(browse);

            std::string url = json_data["url"].get<std::string>();
            std::string  sdp = json_data["sdp"].get<std::string>();
            bool videoEncryption = false;
            auto videoEncryptionIt = json_data.find("videoEncryption");
            if (videoEncryptionIt != json_data.end() && videoEncryptionIt->is_boolean()) {
                videoEncryption = videoEncryptionIt->get<bool>();
            }
            
            std::string videoToken;
            auto tokenIt = json_data.find("token");
            if (tokenIt != json_data.end() && tokenIt->is_string()) {
                videoToken = tokenIt->get<std::string>();
            }

                std::string localip = "";
                try {
                    // 提取域名部分
                    std::string domain = DM::AppUtils::extractDomain(url);
                    // 创建一个 Boost.Asio 的 io_context 对象
                    boost::asio::io_context io_context;
                    // 创建一个 UDP 套接字
                    boost::asio::ip::udp::socket socket(io_context);
                    //socket.non_blocking(true); 
                    // 连接到一个公共的 UDP 地址和端口（Google 的公共 DNS 服务器）
                    //boost::system::error_code ec;
                    socket.connect(boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(domain), 80));
                   
                    // 获取本地端点信息
                    boost::asio::ip::udp::endpoint local_endpoint = socket.local_endpoint();
                    // 关闭套接字
                    socket.close();
                    // 返回本地 IP 地址的字符串表示
                    localip = local_endpoint.address().to_string();
                }
                catch (const std::exception& e) {
                    // 若出现异常，输出错误信息并返回空字符串
                    std::cerr << "Error: " << e.what() << std::endl;
                }
                if (!localip.empty())
                {
                    std::string mdns_addr = "";
                    std::vector<std::string> tokens;
                    boost::split(tokens, sdp, boost::is_any_of("\n"));
                    for (const auto& token : tokens) {
                        if (token.find("a=candidate") != std::string::npos) {
                            std::vector<std::string> sub_tokens;
                            boost::split(sub_tokens, token, boost::is_any_of(" "));
                            mdns_addr = sub_tokens[4];
                            break;
                            //sdp = sdp.replace("a=candidate", "a=candidate" + " " + "raddr=" + localip);
                        }
                    }
                    if (!mdns_addr.empty())
                    {
                        boost::algorithm::replace_first(sdp, mdns_addr, localip);
                    }

                    //sdp = sdp.replace("
                }


                nlohmann::json j;
                j["type"] = "offer";
                j["sdp"] = sdp;

                if (!videoToken.empty()) {
                    j["token"] = videoToken;
                }

                std::string d = j.dump();
                std::string e = cereal::base64::encode((unsigned char const*)d.c_str(), d.length());

                if (!videoEncryption && url.rfind("https://", 0) == 0) {
                    videoEncryption = true;
                }

                auto post_result = [weak_browse](nlohmann::json out_data) {
                    nlohmann::json commandJson;
                    commandJson["command"] = "get_webrtc_local_param";
                    commandJson["data"] = std::move(out_data);
                    auto strJS = commandJson.dump(-1, ' ', true);
                    auto encodedJS = RemotePrint::Utils::url_encode(strJS);
                    wxGetApp().CallAfter([weak_browse, encodedJS] {
                        wxWebView* browse = weak_browse.get();
                        if (browse == nullptr || browse->IsBeingDeleted())
                            return;

                        AppUtils::PostMsg(browse, wxString::Format("window.handleStudioCmd('%s');", encodedJS).ToStdString());
                    });
                };

                if (videoEncryption) {
                    try {
                        Http::post(url)
                            .timeout_connect(10)
                            .timeout_max(15)
                            .header("Content-Type", "plain/text")
                            .set_post_body(e)
                            .ca_file(Slic3r::resources_dir() + "/cert/ca.crt")
                            .ssl_verify_peer(true)   //校验证书链
                            .ssl_verify_host(false)  //不校验域名
                            .on_complete([post_result, url, videoEncryption](std::string body, unsigned http_status) {
                                nlohmann::json out_data;
                                out_data["sdp"] = std::move(body);
                                out_data["url"] = url;
                                out_data["videoEncryption"] = videoEncryption;
                                out_data["status"] = http_status;
                                post_result(std::move(out_data));
                            })
                            .on_error([post_result, url, videoEncryption](std::string body, std::string error, unsigned http_status) {
                                nlohmann::json out_data;
                                if (!body.empty()) {
                                    out_data["sdp"] = std::move(body);
                                } else {
                                    out_data["status"] = http_status;
                                    out_data["error"] = std::move(error);
                                }
                                out_data["url"] = url;
                                out_data["videoEncryption"] = videoEncryption;
                                out_data["status"] = http_status;
                                post_result(std::move(out_data));
                            })
                            .perform();
                    } catch (const std::exception& ex) {
                        nlohmann::json out_data;
                        out_data["url"] = url;
                        out_data["videoEncryption"] = videoEncryption;
                        out_data["status"] = 0;
                        out_data["error"] = ex.what();
                        post_result(std::move(out_data));
                    }
                    return true;
                }

                nlohmann::json out_data;
                out_data["sdp"] = e;
                out_data["url"] = url;
                out_data["videoEncryption"] = videoEncryption;
                out_data["status"] = 0;
                post_result(std::move(out_data));

                return true;
            });

        // set active device
        this->Handler({ "set_current_device" }, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            std::string mac = json_data.value("device_id", json_data.value("mac", std::string()));
            if (mac.empty()) {
                return true;
            }

            DM::DeviceMgr::Ins().SetCurrentDevice(mac);
            DM::DataCenter::Ins().set_current_device(mac, json_data);
            wxPostEvent(wxGetApp().plater(), wxCommandEvent(EVT_CURRENT_DEVICE_CHANGED));
            Slic3r::GUI::NotifyAIChatSceneChanged();
            return true;
            }); 

        this->Handler({ "get_current_device" }, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {

            DM::Device device= DM::DataCenter::Ins().get_current_device_data();
            if (!device.valid) {
                return true;
            }

            // Create top-level JSON object
            nlohmann::json top_level_json = {
                {"mac", device.mac}
            };

            // Create command JSON object
            nlohmann::json commandJson = {
                {"command", "get_current_device"},
                {"data", top_level_json}
            };

            AppUtils::PostMsg(browse, commandJson);

            return true;
            });

        this->Handler({ "edit_device_group_name" }, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            std::string oldName = json_data["oldName"];
            std::string newName = json_data["newName"];
            DM::DeviceMgr::Ins().EditGroupName(oldName, newName);
            return true;
            });

        this->Handler({ "remove_group" }, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            std::string name = json_data["name"];
            DM::DeviceMgr::Ins().RemoveGroup(name);
            AccountDeviceMgr::getInstance().unbind_device_by_group(name);
            return true;
            });

        this->Handler({ "edit_device_name" }, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            std::string address = json_data["address"];
            std::string name = json_data["name"];
            DM::DeviceMgr::Ins().EditDeiveName(address, name);
            return true;
            });

        // Handle print command from device detail page
        this->Handler({ "device_detail_print" }, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            try {
                // Create analytics event payload
                AnalyticsEventPayload payload;
                payload.type = AnalyticsDataEventType::ANALYTICS_PRINT_BEGIN;
                
                // Extract parameters from json_data["data"]
                nlohmann::json analytics_data;
                
                // Get the "data" field first (where actual parameters are located)
                nlohmann::json data_field;
                if (json_data.contains("data") && !json_data["data"].is_null()) {
                    if (json_data["data"].is_string()) {
                        // If data is a string, parse it
                        try {
                            data_field = nlohmann::json::parse(json_data["data"].get<std::string>());
                        } catch (...) {
                            BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": Failed to parse data string";
                        }
                    } else {
                        // If data is already an object
                        data_field = json_data["data"];
                    }
                }
                
                // Build event data from frontend JSON (empty string if field not provided)
                analytics_data["printer"] = data_field.value("printer", "");           // Printer model from frontend
                analytics_data["calibration"] = data_field.value("calibration", "");   // "0" or "1" from frontend
                analytics_data["time_lapse"] = data_field.value("time_lapse", "");     // "0" or "1" from frontend
                analytics_data["format"] = data_field.value("format", "");             // "GCode" or "3MF" from frontend
                analytics_data["network"] = data_field.value("network", "");           // "Local" or "Global" from frontend
                analytics_data["filament_device"] = data_field.value("filament_device", ""); // From frontend
                analytics_data["entry"] = data_field.value("entry", "");               // Entry point from frontend
                analytics_data["error_code"] = data_field.value("error_code", "");     // Error code from frontend
                
                payload.data = analytics_data;

                // Trigger analytics event upload
                AnalyticsDataUploadManager::getInstance().triggerUploadTasksWithPayload(payload);
                
            } catch (const std::exception& e) {
                BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": Failed to process device_detail_print command: " << e.what();
            } catch (...) {
                BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": Failed to process device_detail_print command with unknown error";
            }
            
            return true;
        });

        // Handle print video connect event
        this->Handler({ "print_video_connect" }, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            try {
                AnalyticsEventPayload payload;
                payload.type = AnalyticsDataEventType::ANALYTICS_PRINT_VIDEO_CONNECT;
                payload.data = json_data;

                AnalyticsDataUploadManager::getInstance().triggerUploadTasksWithPayload(payload);
            } catch (const std::exception& e) {
                BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": Failed to process print_video_connect command: " << e.what();
            } catch (...) {
                BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": Failed to process print_video_connect command with unknown error";
            }

            return true;
        });

        this->Handler({ "remove_device" }, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            std::string address = json_data["address"];
            DM::DeviceMgr::Ins().RemoveDevice(address);
            AccountDeviceMgr::getInstance().unbind_device_by_address(address);
            return true;
            });
        this->Handler({ "update_device" }, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            DM::DeviceMgr::Data device;
            device.address = json_data["address"];
            device.mac = json_data["mac"];
            device.model = json_data["model"];
            device.connectType = json_data["type"];

            DM::DeviceMgr::Ins().UpdateDevice(device.mac, device);
            return true;
            });
        this->Handler({ "add_device" }, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {

            wxString strJS = wxString::Format("handleStudioCmd(%s)", json_data.dump(-1, ' ', true));

            DM::DeviceMgr::Data device;
            device.address = json_data["address"];
            device.mac = json_data["mac"];
            device.model = json_data["model"];
            device.connectType = json_data["type"];
            device.oldPrinter = json_data["oldPrinter"];
            

            std::string group = json_data["group"];
            DM::DeviceMgr::Ins().AddDevice(group, device);
            return true;
            });
        this->Handler({ "add_device_klipper" }, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {

            wxString strJS = wxString::Format("handleStudioCmd(%s)", json_data.dump(-1, ' ', true));

            DM::DeviceMgr::Data device;
            device.address = json_data["address"];
            device.mac = json_data["mac"];
            device.model = json_data["model"];
            device.connectType = json_data["type"];
            device.oldPrinter = json_data["oldPrinter"];
            device.moonrakerPort = json_data["moonrakerPort"];
            device.fluiddPort = json_data["fluiddPort"];
            device.mainsailPort = json_data["mainsailPort"];

            std::string group = json_data["group"];
            DM::DeviceMgr::Ins().AddDevice(group, device);
            return true;
            });

        this->Handler({ "add_device_fluidd" }, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            wxString strJS = wxString::Format("handleStudioCmd(%s)", json_data.dump(-1, ' ', true));
            DM::DeviceMgr::Data device;
            device.address = json_data["address"];
            device.mac = json_data["mac"];
            device.model = json_data["model"];
            device.connectType = json_data["type"];
            device.oldPrinter = false;
            device.deviceUI = json_data["deviceUI"];
            device.apiKey = json_data["apiKey"];
            device.hostType = json_data["hostType"];
            device.caFile = json_data["caFile"];
            device.ignoreCertRevocation = json_data["ignoreCertRevocation"];
            std::string group = json_data["group"];

            DM::DeviceMgr::Ins().AddDevice(group, device);
            return true;
            });
        this->Handler({"remove_to_first"}, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            DM::DeviceMgr::Data device;
            std::string         name = json_data["name"];
            DM::DeviceMgr::Ins().remove2FirstGroup(name);
            return true;
        });

        this->Handler({"move_to_group"}, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            DM::DeviceMgr::Data device;
            std::string         originGroup = json_data["originGroup"];
            std::string         targetGroup = json_data["targetGroup"];
            std::string         address     = json_data["address"];
            DM::DeviceMgr::Ins().move2Group(originGroup, targetGroup, address);
            return true;
        });

        this->Handler({"sort_group"}, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            DM::DeviceMgr::Data device;
            std::string              jsonString = json_data["groupMap"];
            json                     groupMap   = json::parse(jsonString);
            std::vector<std::string> groupVector;
            for (const auto& elem : groupMap) {
                groupVector.push_back(elem.get<std::string>());
            }
            DM::DeviceMgr::Ins().sortGroup(groupVector);
            return true;
        });

        this->Handler({ "add_group" }, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            std::string group = json_data["group"];
            DM::DeviceMgr::Ins().AddGroup(group);
            return true;
            });

        this->Handler({ "forward_device_detail" }, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            wxCommandEvent e = wxCommandEvent(wxCUSTOMEVT_NOTEBOOK_SEL_CHANGED);
            e.SetId(MainFrame::TabPosition::tpDeviceMgr); // printer details page
            wxPostEvent(wxGetApp().mainframe->topbar(), e);

            return true;
            });

        this->Handler({"switch_to_tab"},[](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) { 
            std::string tabName = json_data["tabName"];
            std::string pageName = json_data["pageName"];
            wxGetApp().switch_to_tab(tabName);
            if (!pageName.empty()) {
                wxGetApp().swith_community_sub_page(pageName);
            }

            return true;
            });
        this->Handler({"buy_filament_cmd"},[](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            string color = json_data["filamentColor"].get<std::string>();
            string type  = json_data["filamentType"].get<std::string>();
            string name  = json_data["filamentName"].get<std::string>();
            wxGetApp().OpenEshopRecommendedGoods(color, type, name);

            try
            {
                json js;
                js["type_code"] = "slice822";
                js["event_type"]      = "click_event";
                js["function_module"] = "buy_filament";
                js["module_id"]       = 1;
                js["app_version"]     = GUI_App::format_display_version().c_str();
                js["operating_system"] = wxGetOsDescription().ToStdString().c_str();
                js["timestamp"]       = Slic3r::Utils::utc_timestamp(Slic3r::Utils::get_current_time_utc());
                 wxGetApp().track_event("click_event", js.dump());
            }
            catch (...){}
        
            return true;
        }); 
        this->Handler({"track_learn_about_cfs"},[](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            try
            {
                json js;
                js["type_code"] = "slice822";
                js["event_type"]      = "click_event";
                js["function_module"] = "learn_about_CFS";
                js["module_id"]       = 1;
                js["app_version"]     = GUI_App::format_display_version().c_str();
                js["operating_system"] = wxGetOsDescription().ToStdString().c_str();
                js["timestamp"]       = Slic3r::Utils::utc_timestamp(Slic3r::Utils::get_current_time_utc());
                 wxGetApp().track_event("click_event", js.dump());
            }
            catch (...){}
        
            return true;
        }); 

        this->Handler({"test_fluidd_device"},[](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            
            int deviceType = json_data["type"].get<int>();
            std::string deviceUrl  = json_data["url"].get<std::string>();
            std::string deviceApi  = json_data["api"].get<std::string>();
            std::string caFile  = json_data["caFile"].get<std::string>();
            bool ignoreCertRevocation = json_data["ignoreCertRevocation"].get<bool>();

            GUI::PhysicalPrinter PhyPrinter(deviceType, deviceUrl, deviceApi, caFile,ignoreCertRevocation);
            string               info      = "";
            bool isSuccess =  PhyPrinter.TestConnection(info);
            nlohmann::json commandJson;
            commandJson["command"] = "test_fluidd_device_status";
            commandJson["result"] = (isSuccess ? 1 : 0);
            commandJson["info"] = info;

            wxString strJS = wxString::Format("window.handleStudioCmd('%s');", RemotePrint::Utils::url_encode(commandJson.dump(-1, ' ', true)));
            wxGetApp().CallAfter([browse,strJS]{ AppUtils::PostMsg(browse,strJS.ToStdString());});

            return true;
        });

         this->Handler({ "get_model_match" }, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            std::string printmodelA = json_data["modelA"];
            std::string printmodelB = json_data["modelB"];
            int match_result = 0;
            if(printmodelA != printmodelB)
            {
                Preset*     preset = Slic3r::GUI::wxGetApp().preset_bundle->printers.find_preset(printmodelA);
                Preset*     preset2 = Slic3r::GUI::wxGetApp().preset_bundle->printers.find_preset(printmodelB);
                if(preset &&preset2 )
                {
                    if(preset->config.has("printer_model") && preset2->config.has("printer_model"))
                    {
                        if(preset->config.opt_string("printer_model", true)== preset2->config.opt_string("printer_model", true))
                        {
                            match_result = 0;
                        }else if(preset->config.has("gcode_flavor") && preset2->config.has("gcode_flavor"))
                        {
                            auto        flavor1 = preset->config.option<ConfigOptionEnum<GCodeFlavor>>("gcode_flavor")->value;
                            auto        flavor2 = preset2->config.option<ConfigOptionEnum<GCodeFlavor>>("gcode_flavor")->value;
                            if(flavor1 != flavor2)
                            {
                                match_result = 2;

                            }else{
                                
                                match_result = 1;
                            }
                        }else{
                                match_result = 1;
                            }
                        }
                    
                }else{
                    match_result = 1;
                }
                
                
            }
            nlohmann::json commandJson;
                commandJson["command"] = "get_model_match";
                commandJson["result"] = match_result;
                AppUtils::PostMsg(browse, commandJson);
            return true;
            });
    }

    void DeviceMgrRoutes::check_and_send_print_failure_events(const nlohmann::json& json_data)
    {
        try {
            if (!json_data.contains("data") || !json_data["data"].contains("printerList")) {
                return;
            }
            
            for (const auto& group : json_data["data"]["printerList"]) {
                if (!group.contains("list")) {
                    continue;
                }
                
                for (const auto& printer : group["list"]) {
                    // ✅ Safe field access using .value()
                    std::string mac = printer.value("mac", "");
                    if (mac.empty()) {
                        continue;
                    }
                    
                    // ✅ Use 'state' field instead of 'deviceState' (actual field name in JSON)
                    int current_state = printer.value("state", 0);
                    
                    // ✅ Get last state
                    auto it = g_last_device_states.find(mac);
                    int last_state = (it != g_last_device_states.end()) ? it->second : -1;
                    
                    // ✅ Core logic: report only when transitioning from non-failure to failure state
                    // Note: First detection (-1 → 3) will also report, which is reasonable (detect failure at startup)
                    if (!IsFailureState(last_state) && IsFailureState(current_state)) {
                        BOOST_LOG_TRIVIAL(info) << "Print error detected for " 
                            << mac << " (state: " << last_state << " -> " 
                            << current_state << ")";
                        
                        // ✅ Build event data - only keep error_code field
                        nlohmann::json event_data;
                        
                        // ✅ Add error code if available
                        if (printer.contains("err") && printer["err"].is_object()) {
                            int errcode = printer["err"].value("errcode", 0);
                            if (errcode != 0) {
                                event_data["error_code"] = std::to_string(errcode);
                            }
                        }
                        
                        // ✅ Report using Analytics framework (instead of sending to frontend)
                        AnalyticsEventPayload payload;
                        payload.type = AnalyticsDataEventType::ANALYTICS_PRINT_ERROR;  // Need to define new event type
                        payload.data = event_data;

                        AnalyticsDataUploadManager::getInstance().triggerUploadTasksWithPayload(payload);
                    } else if (IsFailureState(current_state)) {
                        // ✅ Continuing failure, output trace level log (usually not shown)
                        BOOST_LOG_TRIVIAL(trace) << "Continuing failure for " << mac 
                            << " (state: " << current_state << ")";
                    }
                    
                    // ✅ Update last state
                    g_last_device_states[mac] = current_state;
                }
            }
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(error) << "check_and_send_print_failure_events failed: " << e.what();
        }
    }
}