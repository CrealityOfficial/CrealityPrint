#include "slic3r/GUI/simple/MCPChatPanel.hpp"
#include "slic3r/GUI/simple/toolcalls/MCPToolCallsCommon.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "slic3r/GUI/NotificationManager.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/WebModelLibraryView.hpp"
#include "slic3r/GUI/ModelDetailDialog.hpp"
#include "slic3r/GUI/print_manage/AppUtils.hpp"
#include "slic3r/GUI/simple/sendWorkflow/EasyPrintSender.hpp"
#include "libslic3r/Utils.hpp"
#include "cereal/external/base64.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>

#include <fstream>
#include <utility>
#include <vector>


using json = nlohmann::json;

namespace Slic3r {
namespace GUI {

namespace {

bool find_jpeg_frame_bounds(const std::string& bytes, size_t& start, size_t& end)
{
    start = std::string::npos;
    end = std::string::npos;
    for (size_t i = 0; i + 1 < bytes.size(); ++i) {
        const unsigned char first = static_cast<unsigned char>(bytes[i]);
        const unsigned char second = static_cast<unsigned char>(bytes[i + 1]);
        if (start == std::string::npos && first == 0xff && second == 0xd8) {
            start = i;
            ++i;
            continue;
        }
        if (start != std::string::npos && first == 0xff && second == 0xd9) {
            end = i + 1;
            return true;
        }
    }
    return false;
}

std::string jpeg_frame_to_data_url(const std::string& frame)
{
    const std::string encoded = cereal::base64::encode(
        reinterpret_cast<const unsigned char*>(frame.data()),
        frame.size());
    return "data:image/jpeg;base64," + encoded;
}

} // namespace
// ---------------------------------------------------------------------------
// Command routing
// ---------------------------------------------------------------------------
void MCPChatPanel::RegisterHandler(const std::string& command, CommandHandler handler)
{
    m_commandHandlers[command] = std::move(handler);
}    

void MCPChatPanel::RegisterAllHandlers()
{
    using namespace Bridge;

    // All slicer actions delegate to the bridge
    for (const auto& spec : ToolCalls::GetBridgeToolRouteSpecs()) {
        if (!spec.register_handler)
            continue;

        const std::string tool_name = spec.tool ? spec.tool : "";
        const std::string action_id = spec.action_id ? spec.action_id : "";
        if (tool_name.empty() || action_id.empty())
            continue;

        if (tool_name == Bridge::ActionID::START_SLICE || tool_name == "run_slice" ||
            tool_name == "PAUSE_PRINTING" || tool_name == "RESUME_PRINTING" || tool_name == "STOP_PRINTING")
            continue;

        if (tool_name == Bridge::ActionID::EDIT_PLATE_NAME) {
            RegisterHandler(tool_name, [this, action_id](const json& d) {
                BOOST_LOG_TRIVIAL(info) << "[MCPToolCalls] EDIT_PLATE_NAME handler invoked payload=" << d.dump();
                ExecuteBridgeAction(action_id, d);
            });
            continue;
        }

        RegisterHandler(tool_name, [this, action_id](const json& d) { ExecuteBridgeAction(action_id, d); });
    }

    RegisterHandler(ActionID::SET_OBJECT_COLOR,  [this](const json& d) { ExecuteBridgeAction(ActionID::SET_OBJECT_COLOR, d); });
    RegisterHandler(ActionID::SELECT_PRESET,     [this](const json& d) { ExecuteBridgeAction(ActionID::SELECT_PRESET, d); });
    RegisterHandler(ActionID::IMPORT_MODEL,      [this](const json& d) { ExecuteBridgeAction(ActionID::IMPORT_MODEL, d); });
    RegisterHandler(ActionID::START_SLICE,       [this](const json& d) { StartSliceRequest(d.value("request_id", std::string()), d, false); });
    RegisterHandler("run_slice",               [this](const json& d) { StartSliceRequest(d.value("request_id", std::string()), d, false); });
    RegisterHandler(ActionID::SEND_TO_PRINTER,   [this](const json& d) { ExecuteBridgeAction(ActionID::SEND_TO_PRINTER, d); });
    RegisterHandler(ActionID::CAPTURE_MODEL_VIEWS, [this](const json& d) { ExecuteBridgeAction(ActionID::CAPTURE_MODEL_VIEWS, d); });
    RegisterHandler(ActionID::SPLIT_MODEL,       [this](const json& d) { ExecuteBridgeAction(ActionID::SPLIT_MODEL, d); });
    RegisterHandler(ActionID::LIST_PRINTERS,     [this](const json& d) { ExecuteBridgeAction(ActionID::LIST_PRINTERS, d); });
    RegisterHandler(ActionID::SELECT_PRINTER,    [this](const json& d) { ExecuteBridgeAction(ActionID::SELECT_PRINTER, d); });
    RegisterHandler(ActionID::MOVE_PRINT_HEAD,   [this](const json& d) { ExecuteBridgeAction(ActionID::MOVE_PRINT_HEAD, d); });
    RegisterHandler("PAUSE_PRINTING", [this](const json& d) {
        json payload = d;
        payload["action"] = "pause";
        ExecuteBridgeAction(ActionID::PRINT_CONTROL, payload);
    });
    RegisterHandler("RESUME_PRINTING", [this](const json& d) {
        json payload = d;
        payload["action"] = "resume";
        ExecuteBridgeAction(ActionID::PRINT_CONTROL, payload);
    });
    RegisterHandler("STOP_PRINTING", [this](const json& d) {
        json payload = d;
        payload["action"] = "stop";
        ExecuteBridgeAction(ActionID::PRINT_CONTROL, payload);
    });

// Meta actions
    RegisterHandler("get_action_list", [this](const json&) {
        auto list = Bridge::SlicerBridge::Instance().GetActionListJSON();
        SendCommandToJS("action_list", list);
    });
    RegisterHandler("get_available_tools", [this](const json&) {
        auto tools = Bridge::SlicerBridge::Instance().GetAvailableToolsJSON();
        SendCommandToJS("available_tools", tools);
    });
    RegisterHandler("get_system_prompt", [this](const json&) {
        auto prompt = Bridge::SlicerBridge::Instance().GenerateSystemPrompt();
        SendCommandToJS("system_prompt", {{"prompt", prompt}});
    });
    RegisterHandler("web_log", [this](const json& d) {
        ToolCalls::AppendWebViewLogLine(d);
    });
    auto open_external_browser = [this](const json& d) { OpenExternalBrowserFromJS(d); };
    RegisterHandler("open_external_browser", open_external_browser);
    RegisterHandler("open_system_browser", open_external_browser);
    RegisterHandler("open_browser", open_external_browser);
    RegisterHandler("chat_ready", [this](const json&) {
        //if (!m_page_loaded || m_js_ready)
        //    return;

        m_js_ready = true;
        BOOST_LOG_TRIVIAL(info) << "[MCPChatPanel] chat_ready received, bootstrap JS state.";
        NotifyThemeChanged();
        NotifyGatewayUser();
        ExecuteBridgeAction(Bridge::ActionID::GET_SLICER_STATE, json::object());
        auto list = Bridge::SlicerBridge::Instance().GetActionListJSON();
        SendCommandToJS("action_list", list);
        NotifyCxAgentStatus();
        ReplayAISendCardsToJS();
        m_scene_update_timer.Stop();
        m_scene_update_timer.StartOnce(300);
    });
    RegisterHandler("workflow_toolbar_state", [this](const json& d) {
        WorkflowToolbarState state;
        state.available = d.value("available", false);
        state.can_slice = d.value("can_slice", false);
        state.can_send_print = d.value("can_send_print", false);
        SetAIWorkflowToolbarState(state);
    });
    RegisterHandler("get_cxagent_status", [this](const json&) {
        NotifyCxAgentStatus();
    });
    RegisterHandler("sagent_mqtt_publish", [this](const json& d) {
        PublishSAgentMqttMessage(d);
    });
    RegisterHandler("get_webrtc_local_param", [this](const json& d) {
        const std::string request_id = d.value("request_id", "");
        const std::string url = d.value("url", "");
        std::string sdp = d.value("sdp", "");
        bool video_encryption = d.value("videoEncryption", false);
        const std::string video_token = d.value("token", "");

        json out_data = {
            {"request_id", request_id},
            {"url", url},
            {"videoEncryption", video_encryption},
            {"transport", "webrtc"}
        };

        if (url.empty() || sdp.empty()) {
            out_data["status"] = 0;
            out_data["error"] = "missing url or sdp";
            SendCommandToJS("get_webrtc_local_param", out_data);
            return;
        }

        std::string local_ip;
        try {
            const std::string domain = DM::AppUtils::extractDomain(url);
            boost::asio::io_context io_context;
            boost::asio::ip::udp::socket socket(io_context);
            socket.connect(boost::asio::ip::udp::endpoint(
                boost::asio::ip::address::from_string(domain), 80));
            local_ip = socket.local_endpoint().address().to_string();
            socket.close();
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(warning)
                << "[MCPChatPanel] get_webrtc_local_param local ip resolve failed: " << e.what();
        }

        if (!local_ip.empty()) {
            std::string mdns_addr;
            std::vector<std::string> lines;
            boost::split(lines, sdp, boost::is_any_of("\n"));
            for (const auto& line : lines) {
                if (line.find("a=candidate") == std::string::npos)
                    continue;
                std::vector<std::string> tokens;
                boost::split(tokens, line, boost::is_any_of(" "));
                if (tokens.size() > 4) {
                    mdns_addr = tokens[4];
                    break;
                }
            }
            if (!mdns_addr.empty())
                boost::replace_first(sdp, mdns_addr, local_ip);
        }

        json offer_json = {
            {"type", "offer"},
            {"sdp", sdp}
        };
        if (!video_token.empty())
            offer_json["token"] = video_token;

        const std::string offer_payload = offer_json.dump();
        const std::string encoded_offer = cereal::base64::encode(
            reinterpret_cast<const unsigned char*>(offer_payload.c_str()),
            offer_payload.length());

        if (!video_encryption && boost::istarts_with(url, "https://"))
            video_encryption = true;

        std::string response_body;
        std::string response_error;
        unsigned response_status = 0;

        try {
            Http http = Http::post(url);
            http.timeout_connect(5)
                .timeout_max(15)
                .header("Content-Type", "plain/text")
                .set_post_body(encoded_offer);
            if (video_encryption) {
                http.ca_file(Slic3r::resources_dir() + "/cert/ca.crt")
                    .ssl_verify_peer(true)
                    .ssl_verify_host(false);
            }
            http.on_complete([&](std::string body, unsigned http_status) {
                    response_body = body;
                    response_status = http_status;
                })
                .on_error([&](std::string body, std::string error, unsigned http_status) {
                    response_body = body;
                    response_error = error;
                    response_status = http_status;
                })
                .perform_sync();
        } catch (const std::exception& ex) {
            response_error = ex.what();
        }

        out_data["videoEncryption"] = video_encryption;
        out_data["signaling_via_host"] = true;
        out_data["status"] = response_status;
        if (!response_body.empty()) {
            out_data["sdp"] = response_body;
        } else {
            out_data["error"] = response_error.empty() ? "empty WebRTC signaling response" : response_error;
        }
        SendCommandToJS("get_webrtc_local_param", out_data);
    });
    RegisterHandler("capture_mjpeg_frame", [this](const json& d) {
        const std::string request_id = d.value("request_id", "");
        std::string url = d.value("url", "");
        const std::string address = d.value("address", "");
        if (url.empty() && !address.empty())
            url = "http://" + address + ":8080/?action=stream";

        json out_data = {
            {"request_id", request_id},
            {"url", url},
            {"transport", "mjpeg"},
            {"mime_type", "image/jpeg"}
        };

        if (url.empty()) {
            out_data["success"] = false;
            out_data["message"] = "missing MJPEG url";
            SendCommandToJS("capture_mjpeg_frame", out_data);
            return;
        }

        std::string captured_frame;
        std::string response_error;
        unsigned response_status = 0;

        try {
            Http::get(url)
                .timeout_connect(5)
                .timeout_max(10)
                .size_limit(2 * 1024 * 1024)
                .on_progress([&](Http::Progress progress, bool& cancel) {
                    size_t start = 0;
                    size_t end = 0;
                    if (find_jpeg_frame_bounds(progress.buffer, start, end)) {
                        captured_frame = progress.buffer.substr(start, end - start + 1);
                        cancel = true;
                    }
                })
                .on_complete([&](std::string body, unsigned http_status) {
                    response_status = http_status;
                    if (captured_frame.empty()) {
                        size_t start = 0;
                        size_t end = 0;
                        if (find_jpeg_frame_bounds(body, start, end))
                            captured_frame = body.substr(start, end - start + 1);
                    }
                })
                .on_error([&](std::string body, std::string error, unsigned http_status) {
                    response_status = http_status;
                    response_error = error;
                    if (captured_frame.empty()) {
                        size_t start = 0;
                        size_t end = 0;
                        if (find_jpeg_frame_bounds(body, start, end))
                            captured_frame = body.substr(start, end - start + 1);
                    }
                })
                .perform_sync();
        } catch (const std::exception& ex) {
            response_error = ex.what();
        }

        out_data["status"] = response_status;
        if (!captured_frame.empty()) {
            out_data["success"] = true;
            out_data["data_url"] = jpeg_frame_to_data_url(captured_frame);
            out_data["bytes"] = captured_frame.size();
        } else {
            out_data["success"] = false;
            out_data["message"] = response_error.empty() ? "MJPEG first frame not found" : response_error;
        }
        SendCommandToJS("capture_mjpeg_frame", out_data);
    });
    RegisterHandler("connect_cxagent", [this](const json& d) {
        m_cxagent_api_base = ResolveCxAgentBaseUrl(d);
        if (m_cxagent_bridge)
            m_cxagent_bridge->Stop();
        if (m_sagent_mqtt_bridge && d.contains("mqtt") && d["mqtt"].is_object())
            m_sagent_mqtt_bridge->Start(BuildSAgentMqttConfig(d));
        NotifyCxAgentStatus();
    });
    RegisterHandler("disconnect_cxagent", [this](const json&) {
        if (m_sagent_mqtt_bridge)
            m_sagent_mqtt_bridge->Stop();
        if (m_cxagent_bridge)
            m_cxagent_bridge->Stop();
        NotifyCxAgentStatus();
    });
    RegisterHandler("ai_send_card_open", [this](const json& d) { HandleAISendCardOpen(d); });
    RegisterHandler("ai_send_card_select_plate", [this](const json& d) { HandleAISendCardSelectPlate(d); });
    RegisterHandler("ai_send_card_auto_match", [this](const json& d) { HandleAISendCardAutoMatch(d); });
    RegisterHandler("ai_send_card_update_mapping", [this](const json& d) { HandleAISendCardUpdateMapping(d); });
    RegisterHandler("ai_send_card_apply_mapping", [this](const json& d) { HandleAISendCardApplyMapping(d); });
    RegisterHandler("ai_send_apply_process_intent", [this](const json& d) { HandleAISendApplyProcessIntent(d); });
    RegisterHandler("ai_send_card_send_only", [this](const json& d) { HandleAISendCardSendOnly(d); });
    RegisterHandler("ai_send_card_start_print", [this](const json& d) { HandleAISendCardStartPrint(d); });
    RegisterHandler("ai_send_card_cancel", [this](const json& d) { HandleAISendCardCancel(d); });
    RegisterHandler("ai_send_card_retry", [this](const json& d) { HandleAISendCardRetry(d); });
    RegisterHandler("open_import_model", [this](const json&) {
        CallAfter([]() {
            auto* plater = wxGetApp().plater();
            if (!plater) {
                BOOST_LOG_TRIVIAL(warning) << "[MCPChatPanel] open_import_model ignored: plater not available";
                return;
            }
            if (!plater->can_add_model()) {
                BOOST_LOG_TRIVIAL(warning) << "[MCPChatPanel] open_import_model ignored: plater cannot add model";
                return;
            }

            plater->add_model();
        });
    });
    RegisterHandler("open_3mf_project", [this](const json&) {
        CallAfter([]() {
            wxGetApp().request_open_project({});
        });
    });
    RegisterHandler("new_project", [this](const json& payload) {
        const std::string request_id = payload.value("request_id", "");
        CallAfter([this, request_id]() {
            auto* plater = wxGetApp().plater();
            if (!plater) {
                SendCommandToJS("new_project_created", {
                    {"request_id", request_id},
                    {"success", false},
                    {"message", "Plater not available"}
                });
                return;
            }

            const int new_project_result = plater->new_project(false, false);
            if (new_project_result != wxID_YES) {
                SendCommandToJS("new_project_created", {
                    {"request_id", request_id},
                    {"success", false},
                    {"message", new_project_result == wxID_CANCEL ? "Create new project cancelled" : "Create new project failed"}
                });
                return;
            }

            SendCommandToJS("new_project_created", {
                {"request_id", request_id},
                {"success", true},
                {"message", "New project created"}
            });
        });
    });
    RegisterHandler("save_and_create_new_project", [this](const json& payload) {
        const std::string request_id = payload.value("request_id", "");
        const bool skip_save = payload.value("skip_save", false);
        CallAfter([this, request_id, skip_save]() {
            auto* plater = wxGetApp().plater();
            if (!plater) {
                SendCommandToJS("new_project_created", {
                    {"request_id", request_id},
                    {"success", false},
                    {"message", "Plater not available"}
                });
                return;
            }

            if (!skip_save) {
                const int save_result = plater->save_project(false, FT_PROJECT);
                if (save_result != wxID_YES) {
                    SendCommandToJS("new_project_created", {
                        {"request_id", request_id},
                        {"success", false},
                        {"message", save_result == wxID_CANCEL ? "Save project cancelled" : "Save project failed"}
                    });
                    return;
                }
            }

            const int new_project_result = plater->new_project(true, false);
            if (new_project_result != wxID_YES) {
                SendCommandToJS("new_project_created", {
                    {"request_id", request_id},
                    {"success", false},
                    {"message", new_project_result == wxID_CANCEL ? "Create new project cancelled" : "Create new project failed"}
                });
                return;
            }

            SendCommandToJS("new_project_created", {
                {"request_id", request_id},
                {"success", true},
                {"message", skip_save ? "New project created" : "Project saved and new project created"}
            });
        });
    });
    RegisterHandler("open_model_library", [this](const json& d) { ExecuteBridgeAction(Bridge::ActionID::OPEN_MODEL_LIBRARY, d); });
    // Open model detail page handler for trending models preview
    RegisterHandler("open_model_preview", [this](const json& payload) {
        const std::string model_id = payload.value("model_id", "");
        const std::string model_name = payload.value("model_name", "");
        BOOST_LOG_TRIVIAL(info) << "[MCPChatPanel] open_model_preview: model_id=" << model_id;

        if (model_id.empty()) {
            BOOST_LOG_TRIVIAL(warning) << "[MCPChatPanel] open_model_preview: missing model_id, ignoring";
            return;
        }

        CallAfter([this, model_id, model_name]() {
            // Close any existing model detail dialog first to avoid window stacking
            if (m_model_detail_dlg) {
                m_model_detail_dlg->Hide();
                m_model_detail_dlg->Destroy();
                m_model_detail_dlg = nullptr;
            }

            auto* mainframe = wxGetApp().mainframe;
            wxWindow* parent = mainframe ? static_cast<wxWindow*>(mainframe) : wxGetApp().GetTopWindow();
            auto* dlg = new ModelDetailDialog(
                parent,
                ModelDetailDialog::BuildDetailUrl(model_id),
                model_name.empty() ? wxString() : wxString::FromUTF8(model_name.c_str()));

            // Track the dialog so we can close it when a new one is opened
            m_model_detail_dlg = dlg;
            // Clear the pointer when the dialog is closed by the user
            dlg->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& evt) {
                m_model_detail_dlg = nullptr;
                evt.Skip();
            });

            dlg->Centre();
            dlg->Show();
        });
    });
    // Direct model search handler for frontend trigger
    RegisterHandler("recommend_model", [this](const json& d) { ExecuteBridgeAction(Bridge::ActionID::RECOMMEND_MODEL, d); });
    RegisterHandler("smart_model_search", [this](const json& d) { ExecuteBridgeAction(Bridge::ActionID::SMART_MODEL_SEARCH, d); });
    RegisterHandler("import_model_from_search", [this](const json& d) { ExecuteBridgeAction(Bridge::ActionID::IMPORT_MODEL_FROM_SEARCH, d); });
    // Load trending models handler
    RegisterHandler("load_trending_models", [this](const json& payload) {
        const int page = payload.value("page", 1);
        const int page_size = payload.value("page_size", 4);
        BOOST_LOG_TRIVIAL(info) << "[MCPChatPanel] Frontend load_trending_models command: page=" << page << ", page_size=" << page_size;
        CallAfter([this, page, page_size]() {
            HandleLoadTrendingModels(page, page_size);
        });
    });
    RegisterHandler("open_login_dialog", [this](const json&) {
        CallAfter([this]() {
            const std::string message = "{\"command\":\"trigger_login_check\"}";
            wxGetApp().handle_web_request(message);
            NotifyGatewayUser();
        });
    });
    RegisterHandler("open_manual_add_device", [this](const json&) {
        CallAfter([this]() {
            auto* mainframe = wxGetApp().mainframe;
            if (!mainframe) {
                SendCommandToJS("error", {{"message", "Main frame not available"}});
                return;
            }
            if (mainframe->topbar())
                mainframe->topbar()->SetSelection(size_t(MainFrame::tpDeviceMgr));
            mainframe->select_tab(MainFrame::tpDeviceMgr);

            auto* printer_mgr_view = mainframe->get_printer_mgr_view();
            if (!printer_mgr_view) {
                SendCommandToJS("error", {{"message", "Printer manager view not available"}});
                return;
            }

            json command_json;
            command_json["command"] = "open_manual_add_device";
            command_json["data"] = json::object();
            printer_mgr_view->ExecuteScriptCommand(wxGetApp().url_encode(command_json.dump(-1, ' ', true)), true);
        });
    });
    RegisterHandler("open_scan_add_device", [this](const json&) {
        CallAfter([this]() {
            auto* mainframe = wxGetApp().mainframe;
            if (!mainframe) {
                SendCommandToJS("error", {{"message", "Main frame not available"}});
                return;
            }
            if (mainframe->topbar())
                mainframe->topbar()->SetSelection(size_t(MainFrame::tpDeviceMgr));
            mainframe->select_tab(MainFrame::tpDeviceMgr);

            auto* printer_mgr_view = mainframe->get_printer_mgr_view();
            if (!printer_mgr_view) {
                SendCommandToJS("error", {{"message", "Printer manager view not available"}});
                return;
            }

            json command_json;
            command_json["command"] = "open_scan_add_device";
            command_json["data"] = json::object();
            printer_mgr_view->ExecuteScriptCommand(wxGetApp().url_encode(command_json.dump(-1, ' ', true)), true);
        });
    });
    RegisterHandler("forward_device_detail", [this](const json& payload) {
        const auto read_string = [&payload](std::initializer_list<const char*> keys) -> std::string {
            for (const char* key : keys) {
                if (payload.contains(key) && payload[key].is_string()) {
                    std::string value = payload[key].get<std::string>();
                    boost::algorithm::trim(value);
                    if (!value.empty())
                        return value;
                }
            }
            return {};
        };

        const std::string ip = read_string({"ip", "printer_ip", "address", "deviceAddress", "device_address"});
        const std::string name = read_string({"name", "printer_name", "deviceName", "device_name"});
        if (ip.empty()) {
            BOOST_LOG_TRIVIAL(warning) << "[MCPChatPanel] forward_device_detail ignored: missing device ip";
            return;
        }

        CallAfter([ip, name]() {
            EasyPrintSender sender;
            sender.jumpToDeviceDetail(ip, name);
        });
    });
    RegisterHandler("dismiss_notifications", [this](const json& d) {
        CallAfter([this, d]() {
            auto* plater = wxGetApp().plater();
            if (!plater) {
                SendCommandToJS("error", {{"message", "Plater not available"}});
                return;
            }

            auto* nm = plater->get_notification_manager();
            if (!nm) {
                SendCommandToJS("error", {{"message", "Notification manager not available"}});
                return;
            }

            if (d.contains("notifications") && d["notifications"].is_array()) {
                for (const auto& item : d["notifications"])
                    ToolCalls::DismissNotificationEntry(nm, item);
            } else {
                ToolCalls::DismissNotificationEntry(nm, d);
            }

            auto result = Bridge::SlicerBridge::Instance().Execute(Bridge::ActionID::GET_SLICER_STATE, json::object());
            SendCommandToJS("slicer_state", result);
            if (result.value("success", false) && result.contains("state") && result["state"].contains("ui_notifications"))
                SendCommandToJS("scene_warnings", result["state"]["ui_notifications"]);
            else
                SendCommandToJS("scene_warnings", json::array());
        });
    });
    RegisterHandler("cxagent_chat", [this](const json& d) {
        HandleCxAgentChatRequest(d);
    });
    RegisterHandler("cxagent_confirm_task", [this](const json& d) {
        HandleCxAgentConfirmTaskRequest(d);
    });
    RegisterHandler("cxagent_list_sessions", [this](const json& d) {
        HandleCxAgentListSessionsRequest(d);
    });
    RegisterHandler("cxagent_get_task", [this](const json& d) {
        HandleCxAgentGetTaskRequest(d);
    });
    RegisterHandler("model_import_confirmation", [this](const json& d) {
        const bool confirmed = d.value("confirmed", false);
        const std::string model_id = d.value("model_id", "");
        CallAfter([this, confirmed, model_id]() {
            auto& bridge = Bridge::SlicerBridge::Instance();
            if (!confirmed) {
                bridge.ClearPendingModelSearchCache();
                SendCommandToJS("model_import_cancelled", {{"model_id", model_id}});
                return;
            }

            json result = bridge.Execute(Bridge::ActionID::IMPORT_MODEL_FROM_SEARCH, {{"model_id", model_id}});
            if (result.value("success", false)) {
                SendCommandToJS("model_import_success", {
                    {"model_id", result.value("model_id", model_id)},
                    {"model_name", result.value("model_name", std::string())}
                });
            } else {
                SendCommandToJS("model_import_error", {
                    {"error", result.value("message", std::string("Failed to import model from search"))}
                });
            }
        });
    });
    RegisterHandler("cxagent_billing_me", [this](const json& d) {
        const std::string request_id = d.value("request_id", "");
        const std::string user_id = d.value("user_id", m_gateway_user_id);
        const std::string token = d.value("token", m_gateway_user_token);
        if (request_id.empty() || user_id.empty()) {
            SendCommandToJS("cxagent_billing_error", {
                {"request_id", request_id},
                {"message", "Missing request_id or user_id for cxagent_billing_me."}
            });
            return;
        }

        const std::string base = ResolveCxAgentBaseUrl(json::object());
        std::string path = "/api/billing/me?user_id=" + Http::url_encode(user_id);
        if (!token.empty())
            path += "&token=" + Http::url_encode(token);

        Http::set_extra_headers(wxGetApp().get_extra_header());
        Http http = Http::get(base + path);
        http.timeout_connect(5)
            .timeout_max(30)
            .on_complete([this, request_id](std::string body, unsigned status) {
                json response = json::object();
                if (!body.empty()) {
                    try {
                        response = json::parse(body);
                    } catch (...) {
                        response = {{"raw", body}};
                    }
                }

                wxGetApp().CallAfter([this, request_id, response, status]() {
                    SendCommandToJS("cxagent_billing_response", {
                        {"request_id", request_id},
                        {"status_code", status},
                        {"response", response}
                    });
                });
            })
            .on_error([this, request_id](std::string body, std::string error, unsigned status) {
                json detail = json::object();
                if (!body.empty()) {
                    try {
                        detail = json::parse(body);
                    } catch (...) {
                        detail = {{"raw", body}};
                    }
                }

                wxGetApp().CallAfter([this, request_id, detail, error, status]() {
                    std::string message = error;
                    if (message.empty()) {
                        if (detail.contains("detail") && detail["detail"].is_string()) {
                            message = detail["detail"].get<std::string>();
                        } else if (detail.contains("message") && detail["message"].is_string()) {
                            message = detail["message"].get<std::string>();
                        } else if (status != 0) {
                            message = "CxAgent billing request failed: HTTP " + std::to_string(status);
                        } else {
                            message = "CxAgent billing request failed.";
                        }
                    }
                    SendCommandToJS("cxagent_billing_error", {
                        {"request_id", request_id},
                        {"status_code", status},
                        {"message", message},
                        {"detail", detail}
                    });
                });
            })
            .perform();
    });
}

} // namespace GUI
} // namespace Slic3r
