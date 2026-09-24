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
#include "../Device/LanDeviceProbe.hpp"
#include "slic3r/GUI/print_manage/RemotePrinterManager.hpp"
#include "slic3r/GUI/print_manage/Utils.hpp"
#include "libslic3r/GCode/GCodeProcessor.hpp"
#include "slic3r/GUI/PartPlate.hpp"
#include "libslic3r/Preset.hpp"
#include "libslic3r/AppConfig.hpp"
#include "libslic3r/Thread.hpp"
#include "slic3r/GUI/AnalyticsDataUploadManager.hpp"
#include "libslic3r/Time.hpp"
#include "slic3r/GUI/PhysicalPrinter.hpp"
#include "slic3r/GUI/SystemId/SystemId.hpp"
#include "libslic3r/Utils.hpp"
#include "slic3r/GUI/simple/MCPChatPanel.hpp"
#include <wx/window.h>
#include <wx/thread.h>
#include <boost/algorithm/string.hpp>
#include <boost/log/trivial.hpp>
#include <cstdint>
#include <limits>

using namespace Slic3r;

namespace {
    std::unordered_map<std::string, int> g_last_device_states;

    // wxWeakRef mutates wxTrackable's unprotected linked list, so background
    // tasks carry only value data and resolve the window back on the UI thread.
    struct WebViewHandle {
        wxWindowID id {wxID_NONE};
        std::uintptr_t address {0};
    };

    WebViewHandle MakeWebViewHandle(wxWebView* browse)
    {
        if (!wxIsMainThread() || browse == nullptr)
            return {};

        return {browse->GetId(), reinterpret_cast<std::uintptr_t>(static_cast<wxWindow*>(browse))};
    }

    wxWebView* ResolveWebView(const WebViewHandle& handle)
    {
        if (!wxIsMainThread() || handle.address == 0)
            return nullptr;

        wxWindow* window = wxWindow::FindWindowById(handle.id);
        if (window == nullptr || reinterpret_cast<std::uintptr_t>(window) != handle.address)
            return nullptr;

        wxWebView* browse = dynamic_cast<wxWebView*>(window);
        return browse != nullptr && !browse->IsBeingDeleted() ? browse : nullptr;
    }

    void PostDeviceRouteResult(const WebViewHandle& weak_browse,
                               const std::string& command,
                               nlohmann::json data)
    {
        if (wxTheApp == nullptr)
            return;

        nlohmann::json commandJson;
        commandJson["command"] = command;
        commandJson["data"] = std::move(data);
        const auto encodedJS = RemotePrint::Utils::url_encode(commandJson.dump(-1, ' ', true));
        wxTheApp->CallAfter([weak_browse, encodedJS] {
            wxWebView* browse = ResolveWebView(weak_browse);
            if (browse == nullptr)
                return;
            DM::AppUtils::PostMsg(browse, wxString::Format("window.handleStudioCmd('%s');", encodedJS).ToStdString());
        });
    }

    const char* UpdateDeviceCode(DM::DeviceMgr::UpdateDeviceOutcome outcome)
    {
        using Outcome = DM::DeviceMgr::UpdateDeviceOutcome;
        switch (outcome) {
        case Outcome::Updated:              return "updated";
        case Outcome::AlreadyApplied:       return "already_applied";
        case Outcome::StaleSource:          return "stale_source";
        case Outcome::SourceNotFound:       return "source_not_found";
        case Outcome::DuplicateMacConflict: return "duplicate_mac_conflict";
        case Outcome::AddressConflict:      return "address_conflict";
        case Outcome::PersistenceFailed:    return "persistence_failed";
        case Outcome::InvalidRequest:       return "invalid_request";
        }
        return "invalid_request";
    }

    bool UpdateDeviceSucceeded(DM::DeviceMgr::UpdateDeviceOutcome outcome)
    {
        using Outcome = DM::DeviceMgr::UpdateDeviceOutcome;
        return outcome == Outcome::Updated || outcome == Outcome::AlreadyApplied;
    }

    std::string AddressFromLegacyInfoUrl(const std::string& url)
    {
        static const std::string prefix = "https://";
        if (url.rfind(prefix, 0) != 0)
            return {};

        const std::size_t begin = prefix.size();
        const std::size_t end = url.find_first_of(":/", begin);
        return url.substr(begin, end == std::string::npos ? std::string::npos : end - begin);
    }

    nlohmann::json InvalidProbeRequest(const std::string& request_id, const std::string& detail)
    {
        return {
            {"requestId", request_id},
            {"status", 1},
            {"errorType", "invalid_request"},
            {"error", detail},
            {"httpStatus", 0},
            {"attempts", nlohmann::json::array()}
        };
    }

    const char* DeviceOwnershipBatchCode(DM::DeviceMgr::DeviceOwnershipBatchOutcome outcome)
    {
        using Outcome = DM::DeviceMgr::DeviceOwnershipBatchOutcome;
        switch (outcome) {
        case Outcome::Reconciled:     return "reconciled";
        case Outcome::AlreadyApplied: return "already_applied";
        case Outcome::StaleSource:    return "stale_source";
        case Outcome::PersistenceFailed: return "persistence_failed";
        case Outcome::InvalidRequest: return "invalid_request";
        }
        return "invalid_request";
    }

    bool DeviceOwnershipBatchSucceeded(DM::DeviceMgr::DeviceOwnershipBatchOutcome outcome)
    {
        using Outcome = DM::DeviceMgr::DeviceOwnershipBatchOutcome;
        return outcome == Outcome::Reconciled || outcome == Outcome::AlreadyApplied;
    }

    nlohmann::json ModernLanSnapshotToJson(const DM::DeviceMgr::ModernLanDeviceSnapshot& snapshot)
    {
        const auto& data = snapshot.data;
        return {
            {"rawIndex", snapshot.rawIndex},
            {"group", snapshot.group},
            {"address", data.address},
            {"addressRevision", data.addressRevision},
            {"mac", data.mac},
            {"model", data.model},
            {"name", data.name},
            {"deviceUI", data.deviceUI},
            {"connectType", data.connectType},
            {"type", data.connectType},
            {"oldPrinter", data.oldPrinter},
            {"secureConnection", data.secureConnection},
            {"wssPort", data.wssPort},
            {"videoPort", data.videoPort},
            {"moonrakerPort", data.moonrakerPort},
            {"fluiddPort", data.fluiddPort},
            {"mainsailPort", data.mainsailPort},
            {"apiKey", data.apiKey},
            {"hostType", data.hostType},
            {"caFile", data.caFile},
            {"ignoreCertRevocation", data.ignoreCertRevocation}
        };
    }
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
            nlohmann::json device_data = DM::DeviceMgr::Ins().GetData();
            std::string    selected_mac;

            if (!wxGetApp().easy_mode() && wxGetApp().preset_bundle != nullptr) {
                selected_mac = get_preset_bound_device_mac(
                    wxGetApp().preset_bundle->printers.get_selected_preset());
            }

            // The preset-specific binding is more precise than the global last device.
            if (!selected_mac.empty()) {
                if (!device_data.contains("current_device") || !device_data["current_device"].is_object())
                    device_data["current_device"] = nlohmann::json::object();
                device_data["current_device"]["mac"] = selected_mac;
            }

            nlohmann::json commandJson;
            commandJson["command"] = "init_device";
            commandJson["data"]    = std::move(device_data);
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
            const WebViewHandle weak_browse = MakeWebViewHandle(browse);

            std::string url = json_data["url"].get<std::string>();
            std::string  sdp = json_data["sdp"].get<std::string>();
            bool videoEncryption = false;
            auto videoEncryptionIt = json_data.find("videoEncryption");
            if (videoEncryptionIt != json_data.end() && videoEncryptionIt->is_boolean()) {
                videoEncryption = videoEncryptionIt->get<bool>();
            }
            
            std::string request_id;
            auto requestIdIt = json_data.find("requestId");
            if (requestIdIt != json_data.end() && requestIdIt->is_string()) {
                request_id = requestIdIt->get<std::string>();
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

                auto post_result = [weak_browse, request_id](nlohmann::json out_data) {
                    out_data["requestId"] = request_id;
                    nlohmann::json commandJson;
                    commandJson["command"] = "get_webrtc_local_param";
                    commandJson["data"] = std::move(out_data);
                    auto strJS = commandJson.dump(-1, ' ', true);
                    auto encodedJS = RemotePrint::Utils::url_encode(strJS);
                    wxGetApp().CallAfter([weak_browse, encodedJS] {
                        wxWebView* browse = ResolveWebView(weak_browse);
                        if (browse == nullptr)
                            return;

                        AppUtils::PostMsg(browse, wxString::Format("window.handleStudioCmd('%s');", encodedJS).ToStdString());
                    });
                };

                auto retry_without_encryption = [post_result, e, url] {
                    const std::string address = DM::AppUtils::extractDomain(url);
                    if (address.empty()) {
                        nlohmann::json out_data;
                        out_data["url"] = url;
                        out_data["videoEncryption"] = false;
                        out_data["status"] = 0;
                        out_data["error"] = "Unable to determine the device address for the unencrypted retry";
                        post_result(std::move(out_data));
                        return;
                    }

                    const std::string fallback_url = "http://" + address + ":8000/call/webrtc_local";
                    BOOST_LOG_TRIVIAL(warning) << "Encrypted WebRTC signaling failed; retrying without encryption: "
                                               << fallback_url;
                    try {
                        Http::post(fallback_url)
                            .timeout_connect(10)
                            .timeout_max(15)
                            .header("Content-Type", "plain/text")
                            .set_post_body(e)
                            .on_complete([post_result, fallback_url](std::string body, unsigned http_status) {
                                nlohmann::json out_data;
                                out_data["sdp"] = std::move(body);
                                out_data["url"] = fallback_url;
                                out_data["videoEncryption"] = false;
                                out_data["status"] = http_status;
                                post_result(std::move(out_data));
                            })
                            .on_error([post_result, fallback_url](std::string body, std::string error, unsigned http_status) {
                                nlohmann::json out_data;
                                if (!body.empty()) {
                                    out_data["sdp"] = std::move(body);
                                } else {
                                    out_data["error"] = std::move(error);
                                }
                                out_data["url"] = fallback_url;
                                out_data["videoEncryption"] = false;
                                out_data["status"] = http_status;
                                post_result(std::move(out_data));
                            })
                            .perform();
                    } catch (const std::exception& ex) {
                        nlohmann::json out_data;
                        out_data["url"] = fallback_url;
                        out_data["videoEncryption"] = false;
                        out_data["status"] = 0;
                        out_data["error"] = ex.what();
                        post_result(std::move(out_data));
                    }
                };

                try {
                    Http http = Http::post(url);
                    http.timeout_connect(10)
                        .timeout_max(15)
                        .header("Content-Type", "plain/text")
                        .set_post_body(e);
                    if (videoEncryption) {
                        http.ca_file(Slic3r::resources_dir() + "/cert/ca.crt")
                            .ssl_verify_peer(true)
                            .ssl_verify_host(false).ssl_ignore_certificate_time(true);
                    }
                    http.on_complete([post_result, url, videoEncryption](std::string body, unsigned http_status) {
                            nlohmann::json out_data;
                            out_data["sdp"] = std::move(body);
                            out_data["url"] = url;
                            out_data["videoEncryption"] = videoEncryption;
                            out_data["status"] = http_status;
                            post_result(std::move(out_data));
                        })
                        .on_error([post_result, retry_without_encryption, url, videoEncryption](std::string body, std::string error, unsigned http_status) {
                            if (videoEncryption) {
                                retry_without_encryption();
                                return;
                            }

                            nlohmann::json out_data;
                            if (!body.empty()) {
                                out_data["sdp"] = std::move(body);
                            } else {
                                out_data["error"] = std::move(error);
                            }
                            out_data["url"] = url;
                            out_data["videoEncryption"] = videoEncryption;
                            out_data["status"] = http_status;
                            post_result(std::move(out_data));
                        })
                        .perform();
                } catch (const std::exception& ex) {
                    if (videoEncryption) {
                        retry_without_encryption();
                        return true;
                    }

                    nlohmann::json out_data;
                    out_data["url"] = url;
                    out_data["videoEncryption"] = videoEncryption;
                    out_data["status"] = 0;
                    out_data["error"] = ex.what();
                    post_result(std::move(out_data));
                }

                return true;
            });

        this->Handler({ "get_printer_https_info" }, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            const std::string address = AddressFromLegacyInfoUrl(json_data.value("url", std::string()));
            const WebViewHandle weak_browse = MakeWebViewHandle(browse);
            RemotePrint::ProbeOptions options;
            options.https_timeout_seconds = 6;
            const auto result = RemotePrint::LanDeviceProbe().probe(
                address, RemotePrint::ProbePolicy::SecureOnly, options);
            PostDeviceRouteResult(weak_browse, "get_printer_https_info", result.to_json());
            return true;
        });

        this->Handler({ "probe_lan_device" }, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            const std::string request_id = json_data.value("requestId", std::string());
            const WebViewHandle weak_browse = MakeWebViewHandle(browse);
            if (request_id.empty() || request_id.size() > 128) {
                PostDeviceRouteResult(weak_browse, "probe_lan_device_result",
                                      InvalidProbeRequest(request_id, "requestId is required"));
                return true;
            }

            const std::string address = json_data.value("address", std::string());
            const std::string policy_name = json_data.value("policy", std::string("prefer_secure"));
            RemotePrint::ProbePolicy policy;
            if (policy_name == "prefer_secure") {
                policy = RemotePrint::ProbePolicy::PreferSecure;
            } else if (policy_name == "secure_only") {
                policy = RemotePrint::ProbePolicy::SecureOnly;
            } else if (policy_name == "http_only") {
                policy = RemotePrint::ProbePolicy::HttpOnly;
            } else {
                PostDeviceRouteResult(weak_browse, "probe_lan_device_result",
                                      InvalidProbeRequest(request_id, "unknown probe policy"));
                return true;
            }

            RemotePrint::submit_lan_device_probe_task([weak_browse, request_id, address, policy] {
                const auto result = RemotePrint::LanDeviceProbe().probe(address, policy);
                nlohmann::json response = result.to_json();
                response["requestId"] = request_id;
                PostDeviceRouteResult(weak_browse, "probe_lan_device_result", std::move(response));
            });
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
            const WebViewHandle weak_browse = MakeWebViewHandle(browse);
            std::string request_id;
            std::string session_id;
            const bool valid_request_id = json_data.contains("requestId") &&
                json_data.at("requestId").is_string() &&
                !json_data.at("requestId").get_ref<const std::string&>().empty() &&
                json_data.at("requestId").get_ref<const std::string&>().size() <= 128;
            if (valid_request_id)
                request_id = json_data.at("requestId").get<std::string>();

            auto post_response = [&](nlohmann::json response) {
                if (!valid_request_id)
                    return;
                response["requestId"] = request_id;
                if (!session_id.empty())
                    response["sessionId"] = session_id;
                PostDeviceRouteResult(weak_browse, "update_device_result", std::move(response));
            };
            auto post_invalid = [&](const std::string& detail, const std::string& code = "invalid_request") {
                post_response({
                    {"status", 1},
                    {"ok", false},
                    {"code", code},
                    {"changed", false},
                    {"errorType", code},
                    {"error", detail}
                });
            };

            const bool has_expected_address = json_data.contains("expectedAddress");
            const bool has_expected_revision = json_data.contains("expectedAddressRevision");
            const bool cas_requested = has_expected_address || has_expected_revision;
            if (cas_requested && !valid_request_id) {
                BOOST_LOG_TRIVIAL(warning) << "update_device CAS rejected: valid requestId is required";
                return true;
            }

            bool allow_secure_downgrade = false;
            if (json_data.contains("allowSecureDowngrade")) {
                if (!json_data.at("allowSecureDowngrade").is_boolean()) {
                    post_invalid("allowSecureDowngrade must be a boolean");
                    return true;
                }
                allow_secure_downgrade = json_data.at("allowSecureDowngrade").get<bool>();
            }

            if (json_data.contains("sessionId")) {
                if (!json_data.at("sessionId").is_string() ||
                    json_data.at("sessionId").get_ref<const std::string&>().size() > 128) {
                    post_invalid("sessionId must be a string of at most 128 characters");
                    return true;
                }
                session_id = json_data.at("sessionId").get<std::string>();
            }

            try {
                DM::DeviceMgr::UpdatePatch patch;
                std::string new_mac;
                std::string lookup_mac;
                auto read_int = [&](const char* field, int& value) {
                    if (!json_data.contains(field) || !json_data.at(field).is_number_integer())
                        return false;
                    const auto& number = json_data.at(field);
                    if (number.is_number_unsigned()) {
                        const std::uint64_t unsigned_value = number.get<std::uint64_t>();
                        if (unsigned_value > static_cast<std::uint64_t>(std::numeric_limits<int>::max()))
                            return false;
                        value = static_cast<int>(unsigned_value);
                        return true;
                    }
                    const std::int64_t signed_value = number.get<std::int64_t>();
                    if (signed_value < std::numeric_limits<int>::min() ||
                        signed_value > std::numeric_limits<int>::max())
                        return false;
                    value = static_cast<int>(signed_value);
                    return true;
                };

                if (json_data.contains("mac")) {
                    if (!json_data.at("mac").is_string()) {
                        post_invalid("mac must be a string");
                        return true;
                    }
                    new_mac = json_data.at("mac").get<std::string>();
                    if (!new_mac.empty())
                        patch.mac = new_mac;
                }
                lookup_mac = new_mac;
                if (json_data.contains("lookupMac")) {
                    if (!json_data.at("lookupMac").is_string()) {
                        post_invalid("lookupMac must be a string");
                        return true;
                    }
                    lookup_mac = json_data.at("lookupMac").get<std::string>();
                }
                if (json_data.contains("lookupAddress")) {
                    if (!json_data.at("lookupAddress").is_string()) {
                        post_invalid("lookupAddress must be a string");
                        return true;
                    }
                    patch.lookupAddress = json_data.at("lookupAddress").get<std::string>();
                }
                if (json_data.contains("address")) {
                    if (!json_data.at("address").is_string()) {
                        post_invalid("address must be a string");
                        return true;
                    }
                    patch.address = json_data.at("address").get<std::string>();
                }
                if (json_data.contains("model")) {
                    if (!json_data.at("model").is_string()) {
                        post_invalid("model must be a string");
                        return true;
                    }
                    patch.model = json_data.at("model").get<std::string>();
                }
                if (json_data.contains("type")) {
                    int connect_type = 0;
                    if (!read_int("type", connect_type)) {
                        post_invalid("type must be an integer");
                        return true;
                    }
                    patch.connectType = connect_type;
                }
                if (json_data.contains("secureConnection")) {
                    if (!json_data.at("secureConnection").is_boolean()) {
                        post_invalid("secureConnection must be a boolean");
                        return true;
                    }
                    patch.connection.secureConnection = json_data.at("secureConnection").get<bool>();
                }
                if (json_data.contains("wssPort")) {
                    int wss_port = 0;
                    if (!read_int("wssPort", wss_port)) {
                        post_invalid("wssPort must be an integer");
                        return true;
                    }
                    patch.connection.wssPort = wss_port;
                }
                if (json_data.contains("videoPort")) {
                    int video_port = 0;
                    if (!read_int("videoPort", video_port)) {
                        post_invalid("videoPort must be an integer");
                        return true;
                    }
                    patch.connection.videoPort = video_port;
                }

                if (cas_requested) {
                    if (!has_expected_address || !has_expected_revision) {
                        post_invalid("expectedAddress and expectedAddressRevision must be provided together");
                        return true;
                    }
                    if (!json_data.at("expectedAddress").is_string()) {
                        post_invalid("expectedAddress must be a string");
                        return true;
                    }
                    if (!json_data.at("expectedAddressRevision").is_number_integer()) {
                        post_invalid("expectedAddressRevision must be a non-negative integer");
                        return true;
                    }
                    if (!patch.address) {
                        post_invalid("address is required for a CAS update");
                        return true;
                    }

                    DM::DeviceMgr::AddressUpdateExpectation expectation;
                    expectation.expectedAddress = json_data.at("expectedAddress").get<std::string>();
                    const auto& revision_json = json_data.at("expectedAddressRevision");
                    if (revision_json.is_number_unsigned()) {
                        expectation.expectedAddressRevision = revision_json.get<std::uint64_t>();
                    } else {
                        const std::int64_t signed_revision = revision_json.get<std::int64_t>();
                        if (signed_revision < 0) {
                            post_invalid("expectedAddressRevision must be non-negative");
                            return true;
                        }
                        expectation.expectedAddressRevision =
                            static_cast<std::uint64_t>(signed_revision);
                    }
                    const auto result = DM::DeviceMgr::Ins().UpdateDevicePatchCas(
                        lookup_mac,
                        patch,
                        expectation,
                        allow_secure_downgrade);
                    const bool ok = UpdateDeviceSucceeded(result.outcome);
                    const std::string code = UpdateDeviceCode(result.outcome);
                    nlohmann::json response = {
                        {"status", ok ? 0 : 1},
                        {"ok", ok},
                        {"code", code},
                        {"changed", result.changed},
                        {"expectedAddress", expectation.expectedAddress},
                        {"expectedAddressRevision", expectation.expectedAddressRevision}
                    };
                    if (result.device) {
                        response["mac"] = result.device->mac;
                        response["address"] = result.device->address;
                        response["addressRevision"] = result.device->addressRevision;
                    }
                    if (!result.error.empty()) {
                        response["errorType"] = code;
                        response["error"] = result.error;
                    }
                    post_response(std::move(response));
                    return true;
                }

                const bool updated = DM::DeviceMgr::Ins().UpdateDevicePatch(
                    lookup_mac, patch, allow_secure_downgrade);
                if (!updated)
                    BOOST_LOG_TRIVIAL(warning) << "update_device rejected for lookup mac: " << lookup_mac;

                if (valid_request_id) {
                    std::optional<DM::DeviceMgr::Data> current_device;
                    if (updated) {
                        const std::string result_mac = patch.mac.value_or(lookup_mac);
                        if (!result_mac.empty())
                            current_device = DM::DeviceMgr::Ins().FindByMac(result_mac);
                        if (!current_device) {
                            const std::string result_address = patch.address.value_or(
                                patch.lookupAddress.value_or(std::string()));
                            if (!result_address.empty())
                                current_device = DM::DeviceMgr::Ins().FindByAddress(result_address);
                        }
                    }

                    nlohmann::json response = {
                        {"status", updated ? 0 : 1},
                        {"ok", updated},
                        {"code", updated ? "updated" : "rejected"},
                        {"changed", updated}
                    };
                    if (current_device) {
                        response["mac"] = current_device->mac;
                        response["address"] = current_device->address;
                        response["addressRevision"] = current_device->addressRevision;
                    }
                    post_response(std::move(response));
                }
            } catch (const std::exception& e) {
                post_invalid(e.what(), cas_requested ? "invalid_request" : "rejected");
            } catch (...) {
                post_invalid("unexpected update_device failure", "internal_error");
            }
            return true;
            });
        this->Handler({ "reconcile_device_ownership_batch" }, [](wxWebView* browse, const std::string& data, nlohmann::json& json_data, const std::string cmd) {
            const WebViewHandle weak_browse = MakeWebViewHandle(browse);
            std::string request_id;
            std::string session_id;

            auto post_invalid = [&](const std::string& detail, const std::string& code = "invalid_request") {
                nlohmann::json response = {
                    {"requestId", request_id},
                    {"status", 1},
                    {"ok", false},
                    {"code", code},
                    {"changed", false},
                    {"snapshot", nlohmann::json::array()},
                    {"errorType", code},
                    {"error", detail}
                };
                if (!session_id.empty())
                    response["sessionId"] = session_id;
                PostDeviceRouteResult(
                    weak_browse,
                    "reconcile_device_ownership_batch_result",
                    std::move(response));
            };

            try {
                if (!json_data.is_object()) {
                    post_invalid("request data must be an object");
                    return true;
                }
                if (!json_data.contains("requestId") || !json_data.at("requestId").is_string()) {
                    post_invalid("requestId is required and must be a string");
                    return true;
                }
                request_id = json_data.at("requestId").get<std::string>();
                if (request_id.empty() || request_id.size() > 128) {
                    post_invalid("requestId must contain 1 to 128 characters");
                    return true;
                }

                if (json_data.contains("sessionId")) {
                    if (!json_data.at("sessionId").is_string()) {
                        post_invalid("sessionId must be a string");
                        return true;
                    }
                    session_id = json_data.at("sessionId").get<std::string>();
                    if (session_id.size() > 128) {
                        post_invalid("sessionId must not exceed 128 characters");
                        return true;
                    }
                }

                if (!json_data.contains("expectedRecords") ||
                    !json_data.at("expectedRecords").is_array() ||
                    json_data.at("expectedRecords").size() > 4096) {
                    post_invalid("expectedRecords must be an array with at most 4096 records");
                    return true;
                }
                if (!json_data.contains("probes") ||
                    !json_data.at("probes").is_array() ||
                    json_data.at("probes").size() > 1024) {
                    post_invalid("probes must be an array with at most 1024 records");
                    return true;
                }

                auto read_non_negative = [](const nlohmann::json& value, std::uint64_t& result) {
                    if (!value.is_number_integer())
                        return false;
                    if (value.is_number_unsigned()) {
                        result = value.get<std::uint64_t>();
                        return true;
                    }
                    const std::int64_t signed_value = value.get<std::int64_t>();
                    if (signed_value < 0)
                        return false;
                    result = static_cast<std::uint64_t>(signed_value);
                    return true;
                };
                auto read_port = [&](const nlohmann::json& value, int& port) {
                    std::uint64_t parsed = 0;
                    if (!read_non_negative(value, parsed) || parsed > 65535)
                        return false;
                    port = static_cast<int>(parsed);
                    return true;
                };

                DM::DeviceMgr::DeviceOwnershipBatchRequest request;
                for (const auto& record_json : json_data.at("expectedRecords")) {
                    if (!record_json.is_object() ||
                        !record_json.contains("rawIndex") ||
                        !record_json.contains("group") || !record_json.at("group").is_string() ||
                        !record_json.contains("mac") || !record_json.at("mac").is_string() ||
                        !record_json.contains("address") || !record_json.at("address").is_string() ||
                        !record_json.contains("addressRevision")) {
                        post_invalid(
                            "each expected record requires rawIndex, group, mac, address and addressRevision");
                        return true;
                    }

                    std::uint64_t raw_index = 0;
                    std::uint64_t address_revision = 0;
                    if (!read_non_negative(record_json.at("rawIndex"), raw_index) ||
                        raw_index > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
                        post_invalid("rawIndex must be a non-negative integer");
                        return true;
                    }
                    if (!read_non_negative(record_json.at("addressRevision"), address_revision)) {
                        post_invalid("addressRevision must be a non-negative integer");
                        return true;
                    }

                    DM::DeviceMgr::DeviceOwnershipExpectedRecord expected;
                    expected.rawIndex = static_cast<std::size_t>(raw_index);
                    expected.group = record_json.at("group").get<std::string>();
                    expected.mac = record_json.at("mac").get<std::string>();
                    expected.address = record_json.at("address").get<std::string>();
                    expected.addressRevision = address_revision;
                    request.expectedRecords.push_back(std::move(expected));
                }

                for (const auto& probe_json : json_data.at("probes")) {
                    if (!probe_json.is_object() ||
                        !probe_json.contains("address") || !probe_json.at("address").is_string() ||
                        !probe_json.contains("ok") || !probe_json.at("ok").is_boolean()) {
                        post_invalid("each probe requires address and ok");
                        return true;
                    }

                    DM::DeviceMgr::DeviceOwnershipProbe probe;
                    probe.address = probe_json.at("address").get<std::string>();
                    probe.ok = probe_json.at("ok").get<bool>();
                    if (probe.ok) {
                        if (!probe_json.contains("observedMac") ||
                            !probe_json.at("observedMac").is_string() ||
                            !probe_json.contains("secureConnection") ||
                            !probe_json.at("secureConnection").is_boolean() ||
                            !probe_json.contains("wssPort") ||
                            !probe_json.contains("videoPort")) {
                            post_invalid(
                                "successful probes require observedMac, secureConnection, wssPort and videoPort");
                            return true;
                        }
                        if (probe_json.contains("model") && !probe_json.at("model").is_string()) {
                            post_invalid("probe model must be a string");
                            return true;
                        }
                        if (probe_json.contains("allowSecureDowngrade") &&
                            !probe_json.at("allowSecureDowngrade").is_boolean()) {
                            post_invalid("allowSecureDowngrade must be a boolean");
                            return true;
                        }
                        probe.observedMac = probe_json.at("observedMac").get<std::string>();
                        probe.model = probe_json.value("model", std::string());
                        probe.secureConnection = probe_json.at("secureConnection").get<bool>();
                        probe.allowSecureDowngrade =
                            probe_json.value("allowSecureDowngrade", false);
                        if (!read_port(probe_json.at("wssPort"), probe.wssPort) ||
                            !read_port(probe_json.at("videoPort"), probe.videoPort)) {
                            post_invalid("probe ports must be between 0 and 65535");
                            return true;
                        }
                    }
                    request.probes.push_back(std::move(probe));
                }

                const auto result = DM::DeviceMgr::Ins().ReconcileDeviceOwnershipBatch(request);
                const bool ok = DeviceOwnershipBatchSucceeded(result.outcome);
                const std::string code = DeviceOwnershipBatchCode(result.outcome);
                nlohmann::json response = {
                    {"requestId", request_id},
                    {"status", ok ? 0 : 1},
                    {"ok", ok},
                    {"code", code},
                    {"changed", result.changed},
                    {"snapshot", nlohmann::json::array()}
                };
                if (!session_id.empty())
                    response["sessionId"] = session_id;
                for (const auto& device : result.snapshot)
                    response["snapshot"].push_back(ModernLanSnapshotToJson(device));
                if (!result.error.empty()) {
                    response["errorType"] = code;
                    response["error"] = result.error;
                }
                PostDeviceRouteResult(
                    weak_browse,
                    "reconcile_device_ownership_batch_result",
                    std::move(response));
            } catch (const std::exception& e) {
                BOOST_LOG_TRIVIAL(error) << "reconcile_device_ownership_batch failed: " << e.what();
                post_invalid(e.what(), "internal_error");
            } catch (...) {
                BOOST_LOG_TRIVIAL(error) <<
                    "reconcile_device_ownership_batch failed with unknown error";
                post_invalid("unknown device ownership batch failure", "internal_error");
            }
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
            device.secureConnection = json_data.value("secureConnection", false);
            device.wssPort = json_data.value("wssPort", 0);
            device.videoPort = json_data.value("videoPort", 0);
            

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
            device.secureConnection =  false;


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
            const std::string fallback_model_a = json_data.value("printerModelA", std::string());
            const std::string fallback_model_b = json_data.value("printerModelB", std::string());
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
                    auto normalize_model = [](std::string model) {
                        boost::algorithm::trim(model);
                        if (!model.empty()
                            && !boost::algorithm::istarts_with(model, "Creality")
                            && !boost::algorithm::istarts_with(model, "SPARKX")) {
                            model = "Creality " + model;
                        }
                        return model;
                    };

                    std::string printer_model_a = fallback_model_a;
                    std::string printer_model_b = fallback_model_b;
                    if (preset && preset->config.has("printer_model")) {
                        printer_model_a = preset->config.opt_string("printer_model", true);
                    }
                    if (preset2 && preset2->config.has("printer_model")) {
                        printer_model_b = preset2->config.opt_string("printer_model", true);
                    }

                    printer_model_a = normalize_model(printer_model_a);
                    printer_model_b = normalize_model(printer_model_b);

                    match_result = !printer_model_a.empty()
                        && !printer_model_b.empty()
                        && boost::algorithm::iequals(printer_model_a, printer_model_b)
                        ? 0
                        : 1;
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
