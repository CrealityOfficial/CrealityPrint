#include "SendToPrinter.hpp"

#include "slic3r/GUI/I18N.hpp"
#include "slic3r/GUI/wxExtensions.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "libslic3r_version.h"

#include <regex>
#include <string>
#include <wx/sizer.h>
#include <wx/string.h>
#include <wx/toolbar.h>
#include <wx/textdlg.h>
#include <locale>
#include <codecvt>

#include <slic3r/GUI/Widgets/WebView.hpp>
#include <wx/webview.h>
#include "slic3r/GUI/print_manage/RemotePrinterManager.hpp"
#include <boost/beast/core/detail/base64.hpp>

#include <wx/stdpaths.h>
#include "slic3r/GUI/print_manage/utils/cxmdns.h"
#include "slic3r/GUI/print_manage/Utils.hpp"
#include "wx/event.h"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/PartPlate.hpp"
#include "slic3r/GUI/AnalyticsDataUploadManager.hpp"
#include "libslic3r/Print.hpp"
#include <wx/variant.h>
#include <wx/datstrm.h>
#include "../data/DataCenter.hpp"
#include "../AppMgr.hpp"
#include "../AppUtils.hpp"
#include "slic3r/GUI/Notebook.hpp"
#include "cereal/external/base64.hpp"
#include "libslic3r/Time.hpp"
#include "wx/stringimpl.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#if defined(__linux__) || defined(__LINUX__)
#include "video/WebRTCDecoder.h"
#endif
#include "libslic3r/common_header/common_header.h"
namespace Slic3r {
namespace GUI {
namespace pt = boost::property_tree;

CxSentToPrinterDialog::CxSentToPrinterDialog(Plater *plater,
                                             SendType sendtype,std::string mapString)
    : DPIDialog(wxGetApp().mainframe,
                wxID_ANY,
                _L("Send to Lan Printer"),
                wxDefaultPosition,
                wxDefaultSize,
                wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER), m_sendtype(sendtype),m_mapString(mapString)
    , m_plater(plater)
{
    #ifdef __WINDOWS__
    SetDoubleBuffered(true);
#endif //__WINDOWS__

    wxGetApp().UpdateDlgDarkUI(this);

    wxSize minSize = wxSize(FromDIP(1170), FromDIP(500)); // 设置最小尺寸
    wxSize initialSize = wxSize(FromDIP(1170), FromDIP(650));

    SetMinSize(minSize);
    SetSize(initialSize);
    // 将窗体移动到屏幕顶部
    Bind(wxEVT_SHOW, [this](wxShowEvent& event) {
        if (event.IsShown()) {
            wxPoint position = GetPosition();
            if (position.y < 0)
            {
                wxSize newSize = wxSize(FromDIP(1170), FromDIP(650) + position.y);
                SetSize(newSize);
                SetPosition(wxPoint(position.x, 0));
            }
                
        }
        event.Skip();
    });

    //SetBackgroundColour(m_colour_def_color);

    // icon
    std::string icon_path = (boost::format("%1%/images/%2%.ico") % resources_dir() % Slic3r::CxBuildInfo::getIconName()).str();
    SetIcon(wxIcon(encode_path(icon_path.c_str()), wxBITMAP_TYPE_ICO));

    wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);
    topsizer->SetMinSize(FromDIP(1170), FromDIP(600));

    // Create the webview
    m_browser = WebView::CreateWebView(this, "");
    if (m_browser == nullptr) {
        wxLogError("Could not init m_browser");
        return;
    }

    bind_events();

    SetSizer(topsizer);

    topsizer->Add(m_browser, 1, wxEXPAND | wxALL, 0);
    std::string version = std::string(CREALITYPRINT_VERSION);
    std::string os      = wxGetOsDescription().ToStdString();
    int port = wxGetApp().get_server_port();
//#define _DEBUG1
#ifdef _DEBUG1
    wxString url = wxString::Format("http://localhost:5174/?version=%s&port=%d&sendtype=%d&map=%s&os=%s", version, port,(int)m_sendtype,m_mapString, os);
    this->load_url(url, wxString());
    m_browser->EnableAccessToDevTools();
#else

    // wxString url = wxString::Format("file://%s/web/sendToPrinterPage/index.html", from_u8(resources_dir()));
    // this->load_url(wxString(url), wxString());
    wxString url = wxString::Format("%s/web/sendToPrinterPage/index.html?version=%s&port=%d&sendtype=%d&os=%s", from_u8(resources_dir()),version, port,(int)m_sendtype, os);
    url.Replace(wxT("\\"), wxT("/"));
    url.Replace(wxT("#"), wxT("%23"));
    wxURI uri(url);
    wxString encodedUrl = uri.BuildURI();
    encodedUrl = wxT("file://")+encodedUrl;
    //encodedUrl = "http://localhost:5173/";
    this->load_url(encodedUrl, wxString());
    m_browser->EnableAccessToDevTools();
#endif

    if (m_plater)
    {
        update_send_page_content();
    }

    wxGetApp().mainframe->get_printer_mgr_view()->RequestDeviceListFromDB();

    if (wxGetApp().mainframe && wxGetApp().mainframe->get_printer_mgr_view()) {
        wxGetApp().mainframe->get_printer_mgr_view()->RegisterHandler("receive_color_match_info",
            [this](const nlohmann::json& json_data) {
                this->handle_receive_color_match_info(json_data);
            });
    }
    CenterOnParent();
    DM::AppMgr::Ins().Register(m_browser);
}

CxSentToPrinterDialog::~CxSentToPrinterDialog() 
{
    DM::AppMgr::Ins().UnRegister(m_browser);
    restore_extruder_colors();
    UnregisterHandler("register_complete");
    UnregisterHandler("send_gcode");
    UnregisterHandler("send_3mf");
    UnregisterHandler("cancel_send");
    UnregisterHandler("request_color_match_info");
    UnregisterHandler("send_start_print_cmd");
    UnregisterHandler("request_update_plate_thumbnail");
    UnregisterHandler("forward_device_detail");
    UnregisterHandler("get_lang");
    UnregisterHandler("get_devices");
    UnregisterHandler("get_user");
    UnregisterHandler("is_dark_theme");
    UnregisterHandler("get_threeMF");
    UnregisterHandler("get_machine_list");
    UnregisterHandler("get_webrtc_local_param");

}
void CxSentToPrinterDialog::OnCloseWindow(wxCloseEvent& event)
{
    // need to reopen the detail-page Video when close the send page
    wxGetApp().mainframe->get_printer_mgr_view()->request_reopen_detail_video();
    m_isClosed = true;
    if(m_uploadingIp!=wxEmptyString)
    {
        RemotePrint::RemotePrinterManager::getInstance().cancelUpload(m_uploadingIp.ToStdString());
        event.Skip(false);
    }else{
        event.Skip();
    }
}
void CxSentToPrinterDialog::bind_events()
{
    Bind(wxEVT_CLOSE_WINDOW, &CxSentToPrinterDialog::OnCloseWindow, this);
    if(m_browser)
    {
        m_browser->Bind(wxEVT_WEBVIEW_ERROR, &CxSentToPrinterDialog::OnError, this);
        m_browser->Bind(wxEVT_WEBVIEW_LOADED, &CxSentToPrinterDialog::OnLoaded, this);
        Bind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED, &CxSentToPrinterDialog::OnScriptMessage, this, m_browser->GetId());
    }

    RegisterHandler("register_complete", [this](const nlohmann::json& json_data) {
        this->handle_register_complete(json_data);
    });

    RegisterHandler("send_gcode", [this](const nlohmann::json& json_data) {
        this->handle_send_gcode(json_data);
    });

    RegisterHandler("send_3mf", [this](const nlohmann::json& json_data) {
        this->handle_send_3mf(json_data);
    });

    RegisterHandler("cancel_send", [this](const nlohmann::json& json_data) { 
        this->handle_cancel_send(json_data); 
    });

    RegisterHandler("request_color_match_info", [this](const nlohmann::json& json_data) {
        this->handle_request_color_match_info(json_data);
    });

    RegisterHandler("send_start_print_cmd", [this](const nlohmann::json& json_data) {
        this->handle_send_start_print_cmd(json_data);
    });
    RegisterHandler("start_heartbeat_cmd", [this](const nlohmann::json& json_data) {
        this->handle_start_heartbeat_cmd(json_data);
    });
    RegisterHandler("stop_heartbeat_cmd", [this](const nlohmann::json& json_data) {
        this->handle_stop_heartbeat_cmd(json_data);
    });

    RegisterHandler("set_error_cmd", [this](const nlohmann::json& json_data) { this->handle_set_error_cmd(json_data); });

    RegisterHandler("request_update_plate_thumbnail", [this](const nlohmann::json& json_data) {
        this->handle_request_update_plate_thumbnail(json_data);
    });

    RegisterHandler("get_lang", [this](const nlohmann::json& json_data) {
        wxString       lan = wxGetApp().app_config->get("language");
        nlohmann::json commandJson;
        commandJson["command"] = "get_lang";
        commandJson["data"] = lan.ToStdString();

        wxString strJS = wxString::Format("window.handleStudioCmd('%s');", RemotePrint::Utils::url_encode(commandJson.dump()));
        run_script(strJS.ToStdString());
    });

    RegisterHandler("get_devices", [this](const nlohmann::json& json_data) {
        nlohmann::json commandJson;
        commandJson["command"] = "get_devices";
        commandJson["data"] = DM::DataCenter::Ins().GetData();

        std::string commandStr = commandJson.dump(-1,' ',true);
        // wxString strJS = wxString::Format("window.handleStudioCmd('%s');", RemotePrint::Utils::url_encode(commandStr));
        wxString strJS = wxString::Format("window.handleStudioCmd('%s');", commandStr);
        run_script(strJS.ToUTF8().data());
    });

    RegisterHandler("get_user", [this](const nlohmann::json& json_data) {
        auto user = wxGetApp().get_user();
        std::string country_code = wxGetApp().app_config->get("region");
        nlohmann::json top_level_json = {
            {"bLogin", user.bLogin ? 1 : 0},
            {"token", user.token},
            {"userId", user.userId},
            {"region", country_code},
        };

        nlohmann::json commandJson = {
            {"command", "get_user"},
            {"data", top_level_json.dump()}
        };

wxString strJS = wxString::Format("window.handleStudioCmd('%s');", RemotePrint::Utils::url_encode(commandJson.dump()));

run_script(strJS.ToStdString());


        });

    RegisterHandler("is_dark_theme", [this](const nlohmann::json& json_data) {
        wxString lan = wxGetApp().current_language_code_safe();
        nlohmann::json commandJson;
        commandJson["command"] = "is_dark_theme";
        commandJson["data"] =  wxGetApp().dark_mode();

        wxString strJS = wxString::Format("window.handleStudioCmd('%s');", RemotePrint::Utils::url_encode(commandJson.dump()));
        run_script(strJS.ToStdString());
    });

    RegisterHandler("get_threeMF", [this](const nlohmann::json& json_data) {
        wxString project_name = Slic3r::GUI::wxGetApp().plater()->get_project_name();

        boost::property_tree::wptree req;
        req.put(L"command", L"get_threeMF");

        boost::property_tree::wptree item;
        item.put(L"project_name", project_name);

        req.put_child(L"data", item);

        std::wostringstream oss;
        pt::write_json(oss, req, false);
        WebView::RunScript(m_browser, wxString::Format("window.handleStudioCmd(%s)", oss.str()));
        });

    RegisterHandler("get_machine_list", [this](const nlohmann::json& json_data) {
        load_machine_preset_data();
    });

    RegisterHandler("get_webrtc_local_param", [this](const nlohmann::json& json_data) {
        this->handle_get_webrtc_local_param(json_data);
    });
}

void CxSentToPrinterDialog::RegisterHandler(const std::string& command, std::function<void(const nlohmann::json&)> handler)
{
    m_commandHandlers[command] = handler;
}
void CxSentToPrinterDialog::UnregisterHandler(const std::string& command)
{
    m_commandHandlers.erase(command);
}

void CxSentToPrinterDialog::OnScriptMessage(wxWebViewEvent& evt)
{
    try {
        wxString strInput = evt.GetString();
        BOOST_LOG_TRIVIAL(trace) << "DeviceDialog::OnScriptMessage;OnRecv:" << strInput.c_str();
        json     j = json::parse(strInput);
        wxString strCmd = j["command"];
        BOOST_LOG_TRIVIAL(trace) << "DeviceDialog::OnScriptMessage;Command:" << strCmd;
        if(strCmd == "forward_device_detail"){
            wxPostEvent(this, wxCloseEvent(wxEVT_CLOSE_WINDOW));
        }else if(strCmd == "switch_webrtc_source")
        {
#if defined(__linux__) || defined(__LINUX__)
            std::string ip = j["ip"];
            std::string video_url = (boost::format("http://%1%:8000/call/webrtc_local") % ip).str();
            WebRTCDecoder::GetInstance()->startPlay(video_url); 
#endif
        } else if (strCmd == "common_openurl") {
            wxLaunchDefaultBrowser(j["url"]);
            wxPostEvent(this, wxCloseEvent(wxEVT_CLOSE_WINDOW));
        }
        
        if (DM::AppMgr::Ins().Invoke(m_browser, evt.GetString().ToUTF8().data()))
        {
            return;
        }

        

        

        if (m_commandHandlers.find(strCmd.ToStdString()) != m_commandHandlers.end()) {
            m_commandHandlers[strCmd.ToStdString()](j);
        } else {
            BOOST_LOG_TRIVIAL(trace) << "CxSentToPrinterDialog::OnScriptMessage;Unknown Command:" << strCmd;
        }

    } catch (std::exception &e) {
       // wxMessageBox(e.what(), "json Exception", MB_OK);
        BOOST_LOG_TRIVIAL(trace) << "DeviceDialog::OnScriptMessage;Error:" << e.what();
        m_uploadingIp = wxEmptyString;
    }
    

}

/**
 *  Handle the "request_color_match_info" command from the sendPage browser, 
 *  and then send the request to the printerMgrView
 */
void CxSentToPrinterDialog::handle_request_color_match_info(const nlohmann::json& json_data)
{
    m_request_color_match_plateIndex = json_data["plateIndex"];
    wxString ipAddress  = json_data["ipAddress"];

    wxGetApp().mainframe->get_printer_mgr_view()->ExecuteScriptCommand(build_match_color_cmd_info(m_request_color_match_plateIndex, ipAddress.ToStdString()));

}

/**
 *  Handle the "send_start_print_cmd" command from the sendPage browser, 
 *  and then send the request to the printerMgrView
 */
void CxSentToPrinterDialog::handle_send_start_print_cmd(const nlohmann::json& json_data)
{
    // create command to send to the webview
    nlohmann::json commandJson;
    commandJson["command"] = "send_print_cmd";
    commandJson["data"]    = json_data["data"].dump(-1, ' ', true);

    wxGetApp().mainframe->get_printer_mgr_view()->ExecuteScriptCommand(RemotePrint::Utils::url_encode(commandJson.dump(-1, ' ', true)));
}
void CxSentToPrinterDialog::handle_set_error_cmd(const nlohmann::json& json_data)
{
    // create command to send to the webview
    nlohmann::json commandJson;
    commandJson["command"] = "set_error_cmd";
    commandJson["data"]    = json_data["data"].dump();

    wxGetApp().mainframe->get_printer_mgr_view()->ExecuteScriptCommand(RemotePrint::Utils::url_encode(commandJson.dump()));
}



void CxSentToPrinterDialog::post_notify_event(const std::vector<int>& plate_extruders, const std::vector<std::string>& extruder_match_colors, bool bUpdateSelf)
{
    wxVariantList str_variantList;
    for (const auto& str : extruder_match_colors) {
        str_variantList.Append(new wxVariant(str));
    }
    wxVariant str_variant(str_variantList);

    wxVariantList int_variantList;
    for (const auto& i : plate_extruders) {
        int_variantList.Append(new wxVariant(i));
    }
    wxVariant int_variant(int_variantList);

    wxVariantList variantList;
    variantList.Append(&str_variant);
    variantList.Append(&int_variant);

    wxCommandEvent notifyEvent(EVT_NOTIFY_PLATE_THUMBNAIL_UPDATE);
    notifyEvent.SetClientData(new wxVariant(variantList));

    //wxPostEvent(wxGetApp().plater(), notifyEvent);
    wxGetApp().plater()->update_plate_thumbnail(notifyEvent);//Optimize this code in the future

    if(bUpdateSelf)
        update_plate_preview_img_on_send_page();
}

// void CxSentToPrinterDialog::handle_request_update_plate_thumbnail(const nlohmann::json& json_data)
// {
//     std::vector<int> plate_extruders;
//     std::vector<std::string> extruder_match_colors;

//     m_request_color_match_plateIndex = json_data["plateIndex"];
//     int extruderId = json_data["extruderId"];

//     plate_extruders.emplace_back(extruderId);
//     wxString matchColor = json_data["matchColor"];
//     extruder_match_colors.emplace_back(matchColor.ToStdString());
    
    
//     post_notify_event(plate_extruders, extruder_match_colors);
// }

void CxSentToPrinterDialog::handle_request_update_plate_thumbnail(const nlohmann::json& json_data)
{
    std::vector<int> plate_extruders;
    std::vector<std::string> extruder_match_colors;

    if (m_plater->model().objects.size() == 0)
        return;

    m_request_color_match_plateIndex = json_data["plateIndex"];

    // 遍历 matchInfo 数组，解析 extruderId 和 matchColor
    for (const auto& matchInfo : json_data["matchInfo"])
    {
        int extruderId = matchInfo["extruderId"];
        std::string matchColor = matchInfo["matchColor"];

        plate_extruders.emplace_back(extruderId);
        extruder_match_colors.emplace_back(matchColor);
    }


    post_notify_event(plate_extruders, extruder_match_colors);
}

void CxSentToPrinterDialog::handle_get_webrtc_local_param(const nlohmann::json& json_data){
    std::string url = json_data["url"].get<std::string>();
    std::string  sdp = json_data["sdp"].get<std::string>();
    Slic3r::Http http = Slic3r::Http::post(url);

    std::string localip = "";
    try {
        // 提取域名部分
        std::string domain = DM::AppUtils::extractDomain(url);
        // 创建一个 Boost.Asio 的 io_context 对象
        boost::asio::io_context io_context;
        // 创建一个 UDP 套接字
        boost::asio::ip::udp::socket socket(io_context);
        // 连接到一个公共的 UDP 地址和端口（Google 的公共 DNS 服务器）
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

    std::string d = j.dump();
    std::string e = cereal::base64::encode((unsigned char const*)d.c_str(), d.length());

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
        commandJson["data"] = data;

        // AppUtils::PostMsg(browse, wxString::Format("window.handleStudioCmd('%s');", commandJson.dump(-1, ' ', true)).ToStdString());
        wxString strJS = wxString::Format("window.handleStudioCmd('%s');", commandJson.dump(-1, ' ', true));
        run_script(strJS.ToStdString());

        })
        .on_error([&](std::string body, std::string error, unsigned status) {
                nlohmann::json data;
                data["body"] = body;
                data["ret"] = false;

                nlohmann::json commandJson;
                commandJson["command"] = "get_webrtc_local_param";
                commandJson["data"] = data;
                // AppUtils::PostMsg(browse,
                //     wxString::Format("window.handleStudioCmd('%s');", commandJson.dump(-1, ' ', true)).ToStdString());
                wxString strJS = wxString::Format("window.handleStudioCmd('%s');", commandJson.dump(-1, ' ', true));
                run_script(strJS.ToStdString());
            })
                .perform_sync();

}

std::string CxSentToPrinterDialog::build_match_color_cmd_info(int plateIndex, const std::string& ipAddress)
{
    PartPlate*     plate = wxGetApp().plater()->get_partplate_list().get_plate(plateIndex);
    if(!plate)
        return "";

    if(m_backup_extruder_colors.size() <= 0) {
        return "";
    }

    // get filament types
    std::vector<std::string> filament_presets = wxGetApp().preset_bundle->filament_presets;
    std::vector<std::string> filament_types;

    if(m_plater->only_gcode_mode()) {
        GCodeProcessorResult* current_result = m_plater->get_partplate_list().get_current_slice_result();
        if(current_result) {
            for (auto f_type : current_result->creality_extruder_types) {
                filament_types.emplace_back(f_type);
            }
        }

    }
    else {
        for (const auto& preset_name : filament_presets) {
            std::string     filament_type;
            Slic3r::Preset* preset = wxGetApp().preset_bundle->filaments.find_preset(preset_name);
            if (preset) {
                preset->get_filament_type(filament_type);
                filament_types.emplace_back(filament_type);
            }
        }
    }

    nlohmann::json           plate_extruder_colors_json     = nlohmann::json::array();
    std::vector<int> plate_extruders = plate->get_used_extruders();
    if(m_plater->only_gcode_mode()) {
        plate_extruders = m_plater->get_gcode_extruders_in_only_gcode_mode();
    }
    if (plate_extruders.size() > 0) {
        for (const auto& extruder : plate_extruders) {
            if(m_backup_extruder_colors.size() > (extruder-1)) {
                nlohmann::json extruder_info = {
                    {"extruder_id", extruder}, 
                    {"extruder_color", m_backup_extruder_colors[extruder - 1]},
                    {"filament_type", filament_types[extruder - 1]}
                };
                plate_extruder_colors_json.push_back(extruder_info);
            }
        }
    }

    // Create top-level JSON object
    nlohmann::json top_level_json = {
        {"printer_ip", ipAddress},
        {"plate_extruder_colors", plate_extruder_colors_json}
    };

    // Create command JSON object
    nlohmann::json commandJson = {
        {"command", "req_match_color_info"},
        {"data", top_level_json.dump()}
    };

    // Encode the command string
    return RemotePrint::Utils::url_encode(commandJson.dump());
}

void CxSentToPrinterDialog::notify_update_plate_thumbnail_data(const nlohmann::json& json_data)
{
    if (m_plater->model().objects.size() == 0)
        return;
    const auto& dataInfo = json_data["data"];
    std::vector<int> plate_extruders;
    std::vector<std::string> extruder_match_colors;

    for (const auto& matchInfo : dataInfo) {
        plate_extruders.emplace_back(matchInfo["extruderId"]);
        extruder_match_colors.emplace_back(matchInfo["matchColor"]);
    }

    post_notify_event(plate_extruders, extruder_match_colors);

}

void CxSentToPrinterDialog::update_plate_preview_img_on_send_page()
{
    std::string str_val = get_updated_plate_preview_img(m_request_color_match_plateIndex);
    if(str_val.empty())
        return;

    wxString strJS = wxString::Format("window.handleStudioCmd('%s');", str_val);

    run_script(strJS.ToStdString());
}

/**
 * Processes the received color match information from the printerMgrView
 * and prepares it to be sent to the send page.
 */
void CxSentToPrinterDialog::handle_receive_color_match_info(const nlohmann::json& json_data)
{
    notify_update_plate_thumbnail_data(json_data);

    boost::property_tree::wptree req;
    req.put(L"command", L"update_color_match_info");
    req.put(L"data", from_u8(json_data["data"].dump()));

    std::wostringstream oss;
    pt::write_json(oss, req, false);
    BOOST_LOG_TRIVIAL(warning) << oss.str();

    WebView::RunScript(m_browser, wxString::Format("window.handleStudioCmd(%s)", oss.str()));

}

void CxSentToPrinterDialog::handle_send_3mf(const nlohmann::json& json_data)
{
    if(!m_plater)
        return;

    int      plateIndex = json_data["printPlateIndex"];  // which plate to print
    wxString ipAddress  = json_data["ipAddress"];
    std::string upload3mfName = json_data["upload3mfName"];

    std::string tmp_3mf_path = "";
    if (m_plater->only_gcode_mode())
    {
        tmp_3mf_path = m_plater->get_last_loaded_3mf().string();
    } 
    else 
    {
        boost::filesystem::path temp_path(m_plater->model().get_backup_path("Metadata"));
        temp_path /= (boost::format(".%1%.%2%_upload.3mf") % get_current_pid() % plateIndex).str();

        tmp_3mf_path = temp_path.string();
        int         result       = m_plater->export_3mf(tmp_3mf_path, SaveStrategy::UploadToPrinter);
        if (result < 0) {
            return;
        }
    }

    if (tmp_3mf_path.empty())
        return;

    {
        // upload analytics data here
        auto device = DM::DataCenter::Ins().get_printer_data(ipAddress.ToStdString());
        check_upload_analytics_data(device.mac);
    }
    

    m_uploadingIp = ipAddress;
    RemotePrint::RemotePrinterManager::getInstance().pushUploadTasks(
        ipAddress.ToStdString(), upload3mfName, tmp_3mf_path,
        [this](std::string ip, float progress,double speed) {
            nlohmann::json top_level_json;
            top_level_json["printer_ip"] = ip;
            top_level_json["progress"]   = progress;
            top_level_json["speed"]   = speed;
            std::string json_str = top_level_json.dump();

            // create command to send to the webview
            nlohmann::json commandJson;
            commandJson["command"] = "display_upload_progress";
            commandJson["data"]    = RemotePrint::Utils::url_encode(json_str);

            wxString strJS = wxString::Format("window.handleStudioCmd('%s');", RemotePrint::Utils::url_encode(commandJson.dump(-1, ' ', true)));

            wxTheApp->CallAfter([this, strJS]() {
                try
                {
                    if (!m_browser->IsBusy()) {
                        run_script(strJS.ToStdString());
                    }
                }
                catch (...)
                {
                }
            });
        },
        [this](std::string ip, int statusCode) {
            nlohmann::json top_level_json;
            top_level_json["printer_ip"]  = ip;
            top_level_json["status_code"] = statusCode;
            if(m_isClosed)
            {
                m_uploadingIp = wxEmptyString;
                wxTheApp->CallAfter([this]() {
                        Close(false);
                    });
                    return;
                }
            std::string json_str = top_level_json.dump();

            // create command to send to the webview
            nlohmann::json commandJson;
            commandJson["command"] = "notify_upload_status";
            commandJson["data"]    = RemotePrint::Utils::url_encode(json_str);

            wxString strJS = wxString::Format("window.handleStudioCmd('%s');", RemotePrint::Utils::url_encode(commandJson.dump(-1, ' ', true)));
            
            wxTheApp->CallAfter([this, strJS]() {
                try
                {
                    if (!m_browser->IsBusy()) {
                        run_script(strJS.ToStdString());
                    }
                }
                catch (...)
                {
                }
            });
            m_uploadingIp = wxEmptyString;
        }, [this,tmp_3mf_path](std::string ip, std::string body) {

            int statusCode = 1;
            std::string status_msg = "";
            json jBody = json::parse(body);
            if (jBody.contains("code") && jBody["code"].is_number_integer()) {
                statusCode = jBody["code"];
            }
            if(jBody.contains("message") && jBody["message"].is_string()) {
                status_msg = jBody["message"];
            }

            nlohmann::json commandJson;
            commandJson["command"] = "notify_send_complete";
            wxString strJS = wxString::Format("window.handleStudioCmd('%s');", RemotePrint::Utils::url_encode(commandJson.dump(-1, ' ', true)));

            wxTheApp->CallAfter([this, strJS,tmp_3mf_path, statusCode, status_msg]() {
                try
                    {
                        if (!m_browser->IsBusy()) {
                            run_script(strJS.ToStdString());
                        }

                        if(wxGetApp().is_privacy_checked()) {
                            json js;
                            js["type_code"] = "slice813";
                            js["client_id"] = wxGetApp().get_client_id();
                            js["file_format"] = "3mf";

                            std::uintmax_t size_bytes = 0;
                            size_bytes = boost::filesystem::file_size(tmp_3mf_path);
                            double size_mb = size_bytes / (1024.0 * 1024.0);  // MB
                            std::ostringstream oss;
                            oss << std::fixed << std::setprecision(2) << size_mb;
                            js["file_size"] = oss.str();

                            js["status_code"] = statusCode;
                            js["message"] = status_msg;
                            js["operation_date"] = Slic3r::Utils::utc_timestamp(Slic3r::Utils::get_current_time_utc());
                            js["app_version"] = GUI_App::format_display_version().c_str();

                            wxGetApp().track_event("send_file_complete", js.dump());
                        }
                    }
                    catch (...)
                    {
                    }
                });
                m_uploadingIp = wxEmptyString;
        });
}
void CxSentToPrinterDialog::handle_cancel_send(const nlohmann::json& json_data) {
    int      plateIndex = json_data["plateIndex"];
    wxString ipAddress  = json_data["ipAddress"];
    RemotePrint::RemotePrinterManager::getInstance().cancelUpload(ipAddress.ToStdString());
}
void CxSentToPrinterDialog::handle_start_heartbeat_cmd(const nlohmann::json& json_data) {
    // create command to send to the webview
    nlohmann::json commandJson;
    commandJson["command"] = "start_heartbeat_cmd";
    commandJson["data"]    = json_data["data"].dump(-1, ' ', true);

    wxGetApp().mainframe->get_printer_mgr_view()->ExecuteScriptCommand(RemotePrint::Utils::url_encode(commandJson.dump(-1, ' ', true)));
}
void CxSentToPrinterDialog::handle_stop_heartbeat_cmd(const nlohmann::json& json_data) {
     // create command to send to the webview
    nlohmann::json commandJson;
    commandJson["command"] = "stop_heartbeat_cmd";
    commandJson["data"]    = json_data["data"].dump(-1, ' ', true);

    wxGetApp().mainframe->get_printer_mgr_view()->ExecuteScriptCommand(RemotePrint::Utils::url_encode(commandJson.dump(-1, ' ', true)));
}

void CxSentToPrinterDialog::handle_register_complete(const nlohmann::json& json_data)
{
    if (m_plater) {
        update_send_page_content();
    }
}


void CxSentToPrinterDialog::handle_send_gcode(const nlohmann::json& json_data) 
{
    int      plateIndex = json_data["plateIndex"];
    wxString ipAddress  = json_data["ipAddress"];
    std::string uploadName = json_data["uploadName"];  // convert from wxString to std::string would cause exception
    bool oldPrinter = json_data["oldPrinter"];
    int  moonrakerPort = json_data["moonrakerPort"];

    if (oldPrinter)
    {
        std::string strIpAddr = ipAddress.ToStdString();
        RemotePrint::RemotePrinterManager::getInstance().setOldPrinterMap(strIpAddr);
    }

    if (moonrakerPort > 0)
    {
        std::string strIpAddr = ipAddress.ToStdString();
        if (strIpAddr.find('(') != std::string::npos)
        {
           RemotePrint::RemotePrinterManager::getInstance().setKlipperPrinterMap(strIpAddr, moonrakerPort);
        }
    }

    PartPlate* plate = wxGetApp().plater()->get_partplate_list().get_plate(plateIndex);
    if (plate) {

        {
            // upload analytics data here
            auto device = DM::DataCenter::Ins().get_printer_data(ipAddress.ToStdString());
            AnalyticsDataUploadManager::getInstance().triggerUploadTasks(AnalyticsUploadTiming::ON_CLICK_START_PRINT_CMD,
                                                                            {AnalyticsDataEventType::ANALYTICS_GLOBAL_PRINT_PARAMS,
                                                                             AnalyticsDataEventType::ANALYTICS_OBJECT_PRINT_PARAMS}, plateIndex,device.mac);
        }

        std::string gcodeFilePath;
        if (m_plater->only_gcode_mode()) {

            GCodeProcessorResult* plate_gcode_result = plate->get_slice_result();
            if (plate_gcode_result)
            {
                gcodeFilePath = plate_gcode_result->filename;
            }

            if (gcodeFilePath.empty())
                return;

        }
        else{
            gcodeFilePath = _L(plate->get_tmp_gcode_path()).ToUTF8();
        }
        
        m_uploadingIp = ipAddress;
        RemotePrint::RemotePrinterManager::getInstance().pushUploadTasks(
            ipAddress.ToStdString(), uploadName, gcodeFilePath,
            [this](std::string ip, float progress,double speed) {

                nlohmann::json top_level_json;
                top_level_json["printer_ip"] = ip;
                top_level_json["progress"]  = progress;
                top_level_json["speed"]  = round(speed);

                std::string json_str = top_level_json.dump();

                // create command to send to the webview
                nlohmann::json commandJson;
                commandJson["command"] = "display_upload_progress";
                commandJson["data"]    = RemotePrint::Utils::url_encode(json_str);

                wxString strJS = wxString::Format("window.handleStudioCmd('%s');", RemotePrint::Utils::url_encode(commandJson.dump(-1, ' ', true)));

                wxTheApp->CallAfter([this, strJS]() {
                    try
                    {
                        if (!m_browser->IsBusy()) {
                            run_script(strJS.ToStdString());
                        }
                    }
                    catch (...)
                    {
                    }
                });
            },
            [this](std::string ip, int statusCode) {
                if(m_isClosed)
                {
                    m_uploadingIp = wxEmptyString;
                    wxTheApp->CallAfter([this]() {
                        Close(false);
                    });
                    return;
                }
                nlohmann::json top_level_json;
                top_level_json["status_code"]  = statusCode;
                std::string json_str = top_level_json.dump();
                
                nlohmann::json commandJson;
                commandJson["command"] = "notify_upload_status";
                commandJson["data"] = RemotePrint::Utils::url_encode(json_str);
                wxString strJS = wxString::Format("window.handleStudioCmd('%s');", RemotePrint::Utils::url_encode(commandJson.dump(-1, ' ', true)));
                
                wxTheApp->CallAfter([this, strJS]() {
                    try
                    {
                        if (!m_browser->IsBusy()) {
                            run_script(strJS.ToStdString());
                        }
                    }
                    catch (...)
                    {
                    }
                });
                m_uploadingIp = wxEmptyString;
            },
            [this, gcodeFilePath](std::string ip, std::string body){
                int deviceType = 0;//local device
                int statusCode = 1;
                std::string status_msg = "";
                json jBody = json::parse(body);
                if (jBody.contains("code") && jBody["code"].is_number_integer()) {
                    statusCode = jBody["code"];
                }
                if(jBody.contains("message") && jBody["message"].is_string()) {
                    status_msg = jBody["message"];
                }

                nlohmann::json top_level_json;
                top_level_json["status_code"] = statusCode;
                top_level_json["id"] = "";
                top_level_json["name"] = "";
                top_level_json["type"] = "";
                top_level_json["filekey"] = "";
                if(jBody.contains("result") && jBody["result"].contains("list") &&jBody["result"]["list"].size()>=0){
                    deviceType = 1;//CX device
                    if(jBody["result"]["list"][0].contains("id"))top_level_json["id"]=jBody["result"]["list"][0]["id"];
                    if(jBody["result"]["list"][0].contains("name"))top_level_json["name"]=jBody["result"]["list"][0]["name"];
                    if(jBody["result"]["list"][0].contains("type"))top_level_json["type"]=jBody["result"]["list"][0]["type"];
                    if(jBody["result"]["list"][0].contains("filekey"))top_level_json["filekey"]=jBody["result"]["list"][0]["filekey"];
                }
                

                std::string json_str;
                if(1 == deviceType)
                { json_str = top_level_json.dump(-1, ' ', true);}

                nlohmann::json commandJson;
                commandJson["command"] = "notify_send_complete";
                commandJson["data"] = RemotePrint::Utils::url_encode(json_str);

                wxString strJS = wxString::Format("window.handleStudioCmd('%s');", RemotePrint::Utils::url_encode(commandJson.dump(-1, ' ', true)));

                wxTheApp->CallAfter([this, strJS, statusCode, status_msg, gcodeFilePath]() {
                    try
                    {
                        if (!m_browser->IsBusy()) {
                            run_script(strJS.ToStdString());
                        }

                        if(wxGetApp().is_privacy_checked()) {
                            json js;
                            js["type_code"] = "slice813";
                            js["client_id"] = wxGetApp().get_client_id();
                            js["file_format"] = "gcode";

                            std::uintmax_t size_bytes = 0;
                            size_bytes = boost::filesystem::file_size(gcodeFilePath);
                            double size_mb = size_bytes / (1024.0 * 1024.0);  // MB
                            std::ostringstream oss;
                            oss << std::fixed << std::setprecision(2) << size_mb;
                            js["file_size"] = oss.str();

                            js["status_code"] = statusCode;
                            js["message"] = status_msg;
                            js["operation_date"] = Slic3r::Utils::utc_timestamp(Slic3r::Utils::get_current_time_utc());
                            js["app_version"] = GUI_App::format_display_version().c_str();

                            wxGetApp().track_event("send_file_complete", js.dump());
                        }
                    }
                    catch (...)
                    {
                    }
                });
                m_uploadingIp = wxEmptyString;
            });
    }
}

void CxSentToPrinterDialog::load_url(const wxString& url, const wxString& apikey)
{
    if (m_browser == nullptr)
        return;
    m_apikey = apikey;
    m_apikey_sent = false;
    m_browser->LoadURL(url);
    //m_browser->SetFocus();
    UpdateState();
}

void CxSentToPrinterDialog::restore_extruder_colors()
{
    if (m_plater->model().objects.size() == 0)
        return;
    if(m_backup_extruder_colors.size() == 0)
        return;
    
    std::vector<int> plate_extruders;
    for(int i = 0; i < m_backup_extruder_colors.size(); i++)
    {
        plate_extruders.emplace_back(i+1);
    }
    
    post_notify_event(plate_extruders, m_backup_extruder_colors, false);
}

bool CxSentToPrinterDialog::Show(bool show)
{
    return wxDialog::Show(show);
}

std::string CxSentToPrinterDialog::get_updated_plate_preview_img(int plateIndex)
{
    if (wxGetApp().plater()->only_gcode_mode())
        return "";

    PartPlate* plate = wxGetApp().plater()->get_partplate_list().get_plate(plateIndex);
    if (!plate)
        return "";

    if (plate && plate->thumbnail_data.is_valid()) 
    {
        wxImage image(plate->thumbnail_data.width, plate->thumbnail_data.height);
        image.InitAlpha();
        for (unsigned int r = 0; r < plate->thumbnail_data.height; ++r) {
            unsigned int rr = (plate->thumbnail_data.height - 1 - r) * plate->thumbnail_data.width;
            for (unsigned int c = 0; c < plate->thumbnail_data.width; ++c) {
                unsigned char* px = (unsigned char*) plate->thumbnail_data.pixels.data() + 4 * (rr + c);
                image.SetRGB((int) c, (int) r, px[0], px[1], px[2]);
                image.SetAlpha((int) c, (int) r, px[3]);
            }
        }

        wxImage resized_image = image.Rescale(256, 256, wxIMAGE_QUALITY_HIGH);

        wxMemoryOutputStream mem_stream;
        if (!resized_image.SaveFile(mem_stream, wxBITMAP_TYPE_PNG)) {
            wxLogError("Failed to save image to memory stream.");
        }

        auto size = mem_stream.GetSize();
        auto imgdata = new unsigned char[size];
        mem_stream.CopyTo(imgdata, size);

        std::size_t encoded_size = boost::beast::detail::base64::encoded_size(size);
        std::string img_base64_data(encoded_size, '\0');
        boost::beast::detail::base64::encode(&img_base64_data[0], imgdata, size);

        nlohmann::json top_level_json;
        top_level_json["image"]       = "data:image/png;base64," + std::move(img_base64_data);
        top_level_json["plate_index"] = plateIndex;

        std::string json_str = top_level_json.dump();

        // create command to send to the webview
        nlohmann::json commandJson;
        commandJson["command"] = "update_plate_preview_img";
        commandJson["data"]    = RemotePrint::Utils::url_encode(json_str);

        std::string commandStr = commandJson.dump();

        return RemotePrint::Utils::url_encode(commandStr);
    }

    return "";
}
std::string CxSentToPrinterDialog::get_onlygcode_plate_data_on_show()
{

    nlohmann::json           colors_json     = nlohmann::json::array();
    nlohmann::json filament_types_json = nlohmann::json::array();
    nlohmann::json json_array = nlohmann::json::array();
    nlohmann::json json_data;

    std::string printer_model = "";
    std::string printer_name = "";
    float       nozzle_diameter = 0.0f;

    m_backup_extruder_colors.clear();
    std::string project_name = wxGetApp().plater()->get_preview_only_filename();
    std::regex  pattern(R"(.*\.(gcode))", std::regex::icase);
    m_is_only_gcode_mode = std::regex_match(project_name, pattern);

    for (int i = 0; i < wxGetApp().plater()->get_partplate_list().get_plate_count(); i++) 
    {
        PartPlate* plate = wxGetApp().plater()->get_partplate_list().get_plate(i);
        if (!plate) 
        {
            continue;
        }

        GCodeProcessorResult* current_result = plate->get_slice_result();
        
        if (!current_result->filename.empty() && current_result->image_data.size() > 0) 
        {
            printer_model = current_result->printer_model;
            printer_name = current_result->printer_settings_id;
            nozzle_diameter = current_result->nozzle_diameter;
            int color_index = 0;
            for (auto color : current_result->creality_default_extruder_colors) {
                if(colors_json.size()<=color_index)
                {
                    colors_json.push_back(color);
                    m_backup_extruder_colors.push_back(color);
                }
                else
                {
                    if(color!="")
                    {
                        colors_json[color_index] = color;
                        m_backup_extruder_colors[color_index] = color;
                    }
                        
                }
                    
                color_index++;
            }
            // filament_types_json
            if(filament_types_json.size() ==0)
            {
                for (auto filament_type : current_result->creality_extruder_types) {
                    filament_types_json.push_back(filament_type);
                }
            }
            int            size    = 0;
            unsigned char* imgdata = nullptr;
            ThumbnailData  data;
            int            maxIndex = current_result->image_data.size() - 1;
            if (maxIndex >= 0) {
                data.width  = current_result->image_data[maxIndex].first[0];
                data.height = current_result->image_data[maxIndex].first[1];
                data.pixels = current_result->image_data[maxIndex].second;
            }

            size    = data.pixels.size();
            imgdata = new unsigned char[size];
            memcpy(imgdata, data.pixels.data(), size);
            std::size_t encoded_size = boost::beast::detail::base64::encoded_size(size);
            std::string img_base64_data(encoded_size, '\0');
            boost::beast::detail::base64::encode(&img_base64_data[0], imgdata, size);

            json_data["image"]       = "data:image/png;base64," + std::move(img_base64_data);
            json_data["plate_index"] = plate->get_index();

            boost::filesystem::path gcode_path(current_result->filename);
            json_data["upload_gcode__name"] = gcode_path.filename().string();

            nlohmann::json extruders_json = nlohmann::json::array();
            int            extruder_index = 1;
            for (const auto& extruder : current_result->creality_default_extruder_colors) {
                if (!extruder.empty()) {
                    extruders_json.push_back(extruder_index);
                }
                extruder_index++;
            }
            json_data["plate_extruders"] = extruders_json;

            wxString total_weight_str;
            wxString print_time_str;
            get_gcode_display_info(total_weight_str, print_time_str, m_plater->get_partplate_list().get_curr_plate());
            json_data["total_weight"] = total_weight_str.ToStdString();
            json_data["print_time"]   = print_time_str.ToStdString();

            json_array.push_back(json_data);
        }else if (!current_result->filename.empty()){
            if (plate && plate->thumbnail_data.is_valid()) {
                wxImage image(plate->thumbnail_data.width, plate->thumbnail_data.height);
                image.InitAlpha();
                for (unsigned int r = 0; r < plate->thumbnail_data.height; ++r) {
                    unsigned int rr = (plate->thumbnail_data.height - 1 - r) * plate->thumbnail_data.width;
                    for (unsigned int c = 0; c < plate->thumbnail_data.width; ++c) {
                        unsigned char* px = (unsigned char*) plate->thumbnail_data.pixels.data() + 4 * (rr + c);
                        image.SetRGB((int) c, (int) r, px[0], px[1], px[2]);
                        image.SetAlpha((int) c, (int) r, px[3]);
                    }
                }
                wxImage resized_image = image.Rescale(256, 256, wxIMAGE_QUALITY_HIGH);

                wxMemoryOutputStream mem_stream;
                if (!resized_image.SaveFile(mem_stream, wxBITMAP_TYPE_PNG)) {
                    wxLogError("Failed to save image to memory stream.");
                }

                auto size = mem_stream.GetSize();
                auto imgdata = new unsigned char[size];
                mem_stream.CopyTo(imgdata, size);

                std::size_t encoded_size = boost::beast::detail::base64::encoded_size(size);
                std::string img_base64_data(encoded_size, '\0');
                boost::beast::detail::base64::encode(&img_base64_data[0], imgdata, size);

                json_data["image"]       = "data:image/png;base64," + std::move(img_base64_data);
            }
           
            json_data["plate_index"] = plate->get_index();
            json_data["plate_extruders"] = nlohmann::json::array();
            json_data["total_weight"] = "";
            json_data["print_time"]   = "";
            json_array.push_back(json_data);
        }
    }

    nlohmann::json top_level_json;
    top_level_json["extruder_colors"] = std::move(colors_json);
    top_level_json["filament_types"]    = std::move(filament_types_json);
    top_level_json["plates"]          = std::move(json_array);
    top_level_json["is_only_gcode_mode"] = m_is_only_gcode_mode;

    int cur_plate_index                   = m_plater->get_partplate_list().get_curr_plate_index();
    top_level_json["current_plate_index"] = cur_plate_index;
    if(printer_name.empty())
    {
        if (printer_model.empty())
        {
            printer_name = _L("Unknown model").ToUTF8().data();
        }
        else
        {
            printer_name = (boost::format("%s %.1f nozzle") % printer_model % nozzle_diameter).str();
        }
    }
    
    //std::string presetname = wxGetApp().preset_bundle->prints.get_selected_preset_name();
    top_level_json["printer_model"] = printer_model;
    top_level_json["preset_name"] = printer_name;

    std::string json_str         = top_level_json.dump(-1, ' ', true);

    // create command to send to the webview
    nlohmann::json commandJson;
    commandJson["command"] = "update_plate_data";
    commandJson["data"]    = RemotePrint::Utils::url_encode(json_str);

    std::string commandStr = commandJson.dump(-1, ' ', true);

    return RemotePrint::Utils::url_encode(commandStr);
}

void CxSentToPrinterDialog::replaceIllegalChars(std::string& str)
{
    const std::unordered_set<char> illegalChars = { '/', '\\', ':', '*', '?', '"', '<', '>', '|' };
    for (char& ch : str)
    {
        if (illegalChars.find(ch) != illegalChars.end())
        {
            ch = '_';
        }
    }
}

void CxSentToPrinterDialog::check_upload_analytics_data(const std::string& device_ip)
{
    for (int i = 0; i < wxGetApp().plater()->get_partplate_list().get_plate_count(); i++) {
        PartPlate* plate = wxGetApp().plater()->get_partplate_list().get_plate(i);
        if (!plate) {
            continue;
        }

        if ((plate->is_slice_result_ready_for_print() || m_plater->only_gcode_mode()) && plate->thumbnail_data.is_valid())
        {
            // upload analytics data here
            AnalyticsDataUploadManager::getInstance().triggerUploadTasks(AnalyticsUploadTiming::ON_CLICK_START_PRINT_CMD,
                                                                        {AnalyticsDataEventType::ANALYTICS_GLOBAL_PRINT_PARAMS,
                                                                         AnalyticsDataEventType::ANALYTICS_OBJECT_PRINT_PARAMS}, plate->get_index(), device_ip);
        }
    }
}

std::string CxSentToPrinterDialog::get_plate_data_on_show()
{
    m_backup_extruder_colors.clear();

    nlohmann::json json_array = nlohmann::json::array();

    m_backup_extruder_colors = Slic3r::GUI::wxGetApp().plater()->get_extruder_colors_from_plater_config();
    nlohmann::json           colors_json     = nlohmann::json::array();
    for (const auto& color : m_backup_extruder_colors) {
        colors_json.push_back(color);
    }

    std::vector<std::string> filament_presets = wxGetApp().preset_bundle->filament_presets;

    std::vector<std::string> filament_types;
    nlohmann::json filament_types_json = nlohmann::json::array();
    nlohmann::json filament_maps_json = nlohmann::json::array();
    std::vector<std::string> filament_maps;
    boost::split(filament_maps, m_mapString, boost::is_any_of(";"));
    for (const auto& substr : filament_maps) {
        filament_maps_json.emplace_back(substr);
    }

    for (const auto& preset_name : filament_presets) {
        std::string     filament_type;
        Slic3r::Preset* preset = wxGetApp().preset_bundle->filaments.find_preset(preset_name);
        if (preset) {
            preset->get_filament_type(filament_type);
            filament_types_json.push_back(filament_type);
            filament_types.emplace_back(filament_type);
        }
    }

    for (int i = 0; i < wxGetApp().plater()->get_partplate_list().get_plate_count(); i++) {
        PartPlate* plate = wxGetApp().plater()->get_partplate_list().get_plate(i);
        if(!plate) {
            continue;
        }

        if ( ( plate->is_slice_result_ready_for_print() || m_plater->only_gcode_mode() ) &&  plate->thumbnail_data.is_valid() ) 
        {
            int size;
            unsigned char* imgdata;
            if (!m_plater->only_gcode_mode()) {
                ThumbnailData& thumbnail = plate->thumbnail_data;
                wxImage image(thumbnail.width, thumbnail.height);
                image.InitAlpha();
                for (unsigned int r = 0; r < thumbnail.height; ++r) {
                    unsigned int rr = (thumbnail.height - 1 - r) * thumbnail.width;
                    for (unsigned int c = 0; c < thumbnail.width; ++c) {
                        unsigned char* px = (unsigned char*) thumbnail.pixels.data() + 4 * (rr + c);
                        image.SetRGB((int) c, (int) r, px[0], px[1], px[2]);
                        image.SetAlpha((int) c, (int) r, px[3]);
                    }
                }
                wxImage resized_image = image.Rescale(256, 256, wxIMAGE_QUALITY_HIGH);

                wxMemoryOutputStream mem_stream;
                if (!resized_image.SaveFile(mem_stream, wxBITMAP_TYPE_PNG)) {
                    wxLogError("Failed to save image to memory stream.");
                }
                size = mem_stream.GetSize();
                // wxMemoryBuffer buffer(size);
                imgdata = new unsigned char[size];
                mem_stream.CopyTo(imgdata, size);
            }
            else
            {   // picture from gcode
                ThumbnailData data = m_plater->get_gcode_thumbnail();
                size               = data.pixels.size();
                imgdata            = new unsigned char[size];
                memcpy(imgdata, data.pixels.data(), size);
            }

            std::size_t encoded_size = boost::beast::detail::base64::encoded_size(size);
            std::string img_base64_data(encoded_size, '\0');
            boost::beast::detail::base64::encode(&img_base64_data[0], imgdata, size);

            std::string default_gcode_name = "";

            std::vector<int> plate_extruders = plate->get_used_extruders();

            if (m_plater->only_gcode_mode()) {
                wxString   last_loaded_gcode = m_plater->get_last_loaded_gcode();
                wxFileName fileName(last_loaded_gcode);
                default_gcode_name = std::string(fileName.GetName().ToUTF8().data());
                plate_extruders = m_plater->get_gcode_extruders_in_only_gcode_mode();
            }
            else {
                // {m_pathname=L"Cone_PLA_10m45s.gcode" }
                ModelObjectPtrs plate_objects = plate->get_objects_on_this_plate();
                std::string     obj0_name     = ""; // the first object's name
                if (plate_objects.size() > 0 && nullptr != plate_objects[0]) {
                    obj0_name = plate_objects[0]->name;
                }

                auto                                  plate_print_statistics = plate->get_slice_result()->print_statistics;
                const PrintEstimatedStatistics::Mode& plate_time_mode =
                    plate_print_statistics.modes[static_cast<size_t>(PrintEstimatedStatistics::ETimeMode::Normal)];

                if (wxGetApp().plater()->has_illegal_filename_characters(obj0_name))
                {
                    replaceIllegalChars(obj0_name);
                }
                
                if (plate_extruders.size() > 0) {
                    default_gcode_name = obj0_name + "_" + filament_types[plate_extruders[0] - 1] + "_" +
                                         get_bbl_time_dhms(plate_time_mode.time);
                } else {
                    default_gcode_name = "plate" + std::to_string(i + 1);
                }
            }

            nlohmann::json json_data;
            json_data["image"]              = "data:image/png;base64," + std::move(img_base64_data);
            json_data["plate_index"]        = plate->get_index();
            json_data["upload_gcode__name"] = default_gcode_name;

            nlohmann::json extruders_json = nlohmann::json::array();
            for (const auto& extruder : plate_extruders) {
                extruders_json.push_back(extruder);
            }
            json_data["plate_extruders"] = extruders_json;

            wxString total_weight_str;
            wxString print_time_str;
            get_gcode_display_info(total_weight_str, print_time_str, plate);
            json_data["total_weight"] = total_weight_str.ToStdString();
            json_data["print_time"] = print_time_str.ToStdString();
            m_is_only_gcode_mode = false;

            json_array.push_back(json_data);
        }
    }

    nlohmann::json top_level_json;
    top_level_json["extruder_colors"] = std::move(colors_json);
    top_level_json["filament_types"]    = std::move(filament_types_json);
    top_level_json["filament_maps"]    = std::move(filament_maps_json);
    top_level_json["plates"]          = std::move(json_array);
    top_level_json["is_only_gcode_mode"] = m_is_only_gcode_mode;


    bool is_all_plates = wxGetApp().plater()->get_preview_canvas3D()->is_all_plates_selected();
    int cur_plate_index = m_plater->get_partplate_list().get_curr_plate_index();
    top_level_json["current_plate_index"] = cur_plate_index;
    
    std::string printer_name = Slic3r::GUI::wxGetApp().preset_bundle->printers.get_selected_preset_name();
    Preset&  presetSelect = Slic3r::GUI::wxGetApp().preset_bundle->printers.get_selected_preset();
    ConfigOptionString* printer_model = presetSelect.config.option<ConfigOptionString>("printer_model");
    if (printer_model) {
        top_level_json["printer_model"] = printer_model->value;
    }else{
        top_level_json["printer_model"] = printer_name;
    }

    top_level_json["preset_name"] = printer_name;
    top_level_json["slice_type"]  = m_plater->isSliceAll() ? 2 : 1; // 1 - 切单盘 2 - 切所有盘

    std::string json_str         = top_level_json.dump(-1, ' ', true);

    // create command to send to the webview
    nlohmann::json commandJson;
    commandJson["command"] = "update_plate_data";
    commandJson["data"]    = RemotePrint::Utils::url_encode(json_str);


    std::string commandStr = commandJson.dump(-1, ' ', true);

    return RemotePrint::Utils::url_encode(commandStr);

}

void CxSentToPrinterDialog::update_send_page_content()
{
    if(m_plater->only_gcode_mode())
    {
        wxString strJS = wxString::Format("window.handleStudioCmd('%s');", from_u8(get_onlygcode_plate_data_on_show()));
        run_script(strJS.ToStdString());
    }else{
        wxString strJS = wxString::Format("window.handleStudioCmd('%s');", from_u8(get_plate_data_on_show()));
        run_script(strJS.ToStdString());
    }
}

void CxSentToPrinterDialog::run_script(std::string content)
{
    WebView::RunScript(m_browser, from_u8(content));
}

void CxSentToPrinterDialog::reload()
{
    m_browser->Reload();
}
/**
 * Method that retrieves the current state from the web control and updates the
 * GUI the reflect this current state.
 */
void CxSentToPrinterDialog::UpdateState() {
  // SetTitle(m_browser->GetCurrentTitle());

}

void CxSentToPrinterDialog::OnClose(wxCloseEvent& evt)
{
    this->Hide();
}

void CxSentToPrinterDialog::SendAPIKey()
{
    if (m_apikey_sent || m_apikey.IsEmpty())
        return;
    m_apikey_sent   = true;
    wxString script = wxString::Format(R"(
    // Check if window.fetch exists before overriding
    if (window.fetch) {
        const originalFetch = window.fetch;
        window.fetch = function(input, init = {}) {
            init.headers = init.headers || {};
            init.headers['X-API-Key'] = '%s';
            return originalFetch(input, init);
        };
    }
)",
                                       m_apikey);
    m_browser->RemoveAllUserScripts();

    m_browser->AddUserScript(script);
    m_browser->Reload();
}

void CxSentToPrinterDialog::OnError(wxWebViewEvent &evt)
{
    auto e = "unknown error";
    switch (evt.GetInt()) {
      case wxWEBVIEW_NAV_ERR_CONNECTION:
        e = "wxWEBVIEW_NAV_ERR_CONNECTION";
        break;
      case wxWEBVIEW_NAV_ERR_CERTIFICATE:
        e = "wxWEBVIEW_NAV_ERR_CERTIFICATE";
        break;
      case wxWEBVIEW_NAV_ERR_AUTH:
        e = "wxWEBVIEW_NAV_ERR_AUTH";
        break;
      case wxWEBVIEW_NAV_ERR_SECURITY:
        e = "wxWEBVIEW_NAV_ERR_SECURITY";
        break;
      case wxWEBVIEW_NAV_ERR_NOT_FOUND:
        e = "wxWEBVIEW_NAV_ERR_NOT_FOUND";
        break;
      case wxWEBVIEW_NAV_ERR_REQUEST:
        e = "wxWEBVIEW_NAV_ERR_REQUEST";
        break;
      case wxWEBVIEW_NAV_ERR_USER_CANCELLED:
        e = "wxWEBVIEW_NAV_ERR_USER_CANCELLED";
        break;
      case wxWEBVIEW_NAV_ERR_OTHER:
        e = "wxWEBVIEW_NAV_ERR_OTHER";
        break;
      }
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__<< boost::format(": error loading page %1% %2% %3% %4%") %evt.GetURL() %evt.GetTarget() %e %evt.GetString();
}

void CxSentToPrinterDialog::OnLoaded(wxWebViewEvent &evt)
{
    if (evt.GetURL().IsEmpty())
        return;
    SendAPIKey();

}

void CxSentToPrinterDialog::on_dpi_changed(const wxRect& suggested_rect)
{
    
}

double CxSentToPrinterDialog::get_gcode_total_weight()
{
    double total_weight = 0.0;
    GCodeProcessorResult* current_result = m_plater->get_partplate_list().get_current_slice_result();
    if (current_result) {
        auto&  ps         = current_result->print_statistics;
        for (auto volume : ps.total_volumes_per_extruder) {
            size_t extruder_id = volume.first;
            double density = 1.24;
            if(extruder_id<current_result->filament_densities.size())
                density     = current_result->filament_densities.at(extruder_id);
            double weight      = volume.second * density * 0.001;
            total_weight += weight;
        }
    }

    return total_weight;
}

void CxSentToPrinterDialog::get_gcode_display_info(wxString& total_weight_str, wxString& print_time, Slic3r::GUI::PartPlate* plate)
{
    if(!m_plater || !plate) return;

	Slic3r::Print* print = nullptr;
	plate->get_print((Slic3r::PrintBase **)&print, nullptr, nullptr);
    if(!print) return;

    // basic info
    auto       aprint_stats = print->print_statistics();

    if(m_plater->only_gcode_mode())
    {
        GCodeProcessorResult* gcode_process_result = m_plater->get_partplate_list().get_current_slice_result();
        if (gcode_process_result) {
            print_time = wxString::Format("%s", short_time(get_time_dhms(gcode_process_result->print_statistics.modes[0].time)));
        }
    }
    else {
        if (plate->get_slice_result()) { 
            print_time = wxString::Format("%s", short_time(get_time_dhms(plate->get_slice_result()->print_statistics.modes[0].time))); 
        }
    }

    double total_weight = aprint_stats.total_weight;
    if(m_plater->only_gcode_mode())
    {
        total_weight = get_gcode_total_weight();
    }

    char weight[64];
    if (wxGetApp().app_config->get("use_inches") == "1") {
        ::sprintf(weight, "  %.2f oz", total_weight*0.035274);
    }else{
        ::sprintf(weight, "  %.2f g", total_weight);
    }

    total_weight_str = wxString(weight);

}

bool CxSentToPrinterDialog::LoadFile(std::string jPath, std::string &sContent)
{
    try {
        boost::nowide::ifstream t(jPath);
        std::stringstream buffer;
        buffer << t.rdbuf();
        sContent=buffer.str();
        BOOST_LOG_TRIVIAL(trace) << __FUNCTION__ << boost::format(", load %1% into buffer")% jPath;
    }
    catch (std::exception &e)
    {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ",  got exception: "<<e.what();
        return false;
    }

    return true;
}

int CxSentToPrinterDialog::load_machine_preset_data()
{
    boost::filesystem::path vendor_dir = (boost::filesystem::path(Slic3r::data_dir()) / PRESET_SYSTEM_DIR ).make_preferred();
    if (boost::filesystem::exists((vendor_dir /"Creality"/"machineList").replace_extension(".json")))
    {
        std::string vendor_preset_path = vendor_dir.string() + "/Creality/machineList.json";
        boost::filesystem::path file_path(vendor_preset_path);

        boost::filesystem::path vendor_dir = boost::filesystem::absolute(file_path.parent_path() / "machineList").make_preferred();
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(",  vendor path %1%.") % vendor_dir.string();
        try{
            std::string contents;
            LoadFile(vendor_preset_path, contents);
            json jLocal = json::parse(contents);
            json pmodels = jLocal["printerList"];

            nlohmann::json commandJson;
            commandJson["command"] = "get_machine_list";
            commandJson["data"]    = pmodels;
            wxString strJS = wxString::Format("window.handleStudioCmd('%s');", RemotePrint::Utils::url_encode(commandJson.dump(-1, ' ', true)));
            run_script(strJS.ToStdString());
        }catch (nlohmann::detail::parse_error &err) {
            BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": parse " << vendor_preset_path
                                     << " got a nlohmann::detail::parse_error, reason = " << err.what();
            return -1;
        } catch (std::exception &e) {
            BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": parse " << vendor_preset_path << " got exception: " << e.what();
            return -1;
        }
    }else{
        auto printer_list_file = fs::path(data_dir()).append("system").append("Creality").append("machineList.json").string();
        std::string base_url   = get_cloud_api_url();
        auto preupload_profile_url = "/api/cxy/v2/slice/profile/official/printerList";
        Http::set_extra_headers(Slic3r::GUI::wxGetApp().get_extra_header());
                            Http http = Http::post(base_url + preupload_profile_url);
                            json        j;
                            j["engineVersion"]  = "3.0.0";
                            boost::uuids::uuid uuid = boost::uuids::random_generator()();
                                http.header("Content-Type", "application/json")
                                    .header("__CXY_REQUESTID_", to_string(uuid))
                                    .set_post_body(j.dump())
                                    .on_complete([&](std::string body, unsigned status) {
                                        if(status!=200){
                                            return -1;
                                        }
                                        try{
                                            json j = json::parse(body);
                                            json printer_list = j["result"];
                                            json list = printer_list["printerList"];

                                            nlohmann::json commandJson;
                                            commandJson["command"] = "get_machine_list";
                                            commandJson["data"] =  list;
                                            wxString strJS = wxString::Format("window.handleStudioCmd('%s');", RemotePrint::Utils::url_encode(commandJson.dump(-1, ' ', true)));
                                            run_script(strJS.ToStdString());

                                            if(list.empty()){
                                                return -1;
                                            }

                                            auto out_printer_list_file = fs::path(data_dir()).append("system")
                                                                            .append("Creality")
                                                                            .append("machineList.json")
                                                                            .string();
                                            boost::nowide::ofstream c;
                                            c.open(out_printer_list_file, std::ios::out | std::ios::trunc);
                                            c << std::setw(4) << printer_list << std::endl;
                                            
                                        }catch(...){
                                            return -1;
                                        }
                                        return 0;
                                    }).perform_sync();
    }

    return 0;
}

} // namespace GUI
} // namespace Slic3r