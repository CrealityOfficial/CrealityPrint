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
#include "slic3r/GUI/print_manage/DeviceDB.hpp"
#include "slic3r/GUI/print_manage/Utils.hpp"
#include "wx/event.h"
#include "slic3r/GUI/Plater.hpp"
#include "libslic3r/Print.hpp"
#include <wx/variant.h>
#include <wx/datstrm.h>
#include "DataCenter.hpp"
#include "AppMgr.hpp"
#include "slic3r/GUI/Notebook.hpp"

namespace Slic3r {
namespace GUI {
namespace pt = boost::property_tree;
CxSentToPrinterDialog::CxSentToPrinterDialog(Plater *plater)
    : DPIDialog(wxGetApp().mainframe,
                wxID_ANY,
                _L("Send to Lan Printer"),
                wxDefaultPosition,
                wxDefaultSize,
                wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER)
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

    SetBackgroundColour(m_colour_def_color);

    // icon
    std::string icon_path = (boost::format("%1%/images/Creative3DTitle.ico") % resources_dir()).str();
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
//#define _DEBUG1
#ifdef _DEBUG1
    this->load_url(wxString("http://localhost:5173/"), wxString());
    m_browser->EnableAccessToDevTools();
#else

    // wxString url = wxString::Format("file://%s/web/sendToPrinterPage/index.html", from_u8(resources_dir()));
    // this->load_url(wxString(url), wxString());
    wxString url = wxString::Format("%s/web/sendToPrinterPage/index.html", from_u8(resources_dir()));
    url.Replace(wxT("\\"), wxT("/"));
    url.Replace(wxT("#"), wxT("%23"));
    wxURI uri(url);
    wxString encodedUrl = uri.BuildURI();
    encodedUrl = wxT("file://")+encodedUrl;
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

}
void CxSentToPrinterDialog::OnCloseWindow(wxCloseEvent& event)
{
    if(m_uploadingIp!=wxEmptyString)
        RemotePrint::RemotePrinterManager::getInstance().cancelUpload(m_uploadingIp.ToStdString());
    while (m_uploadingIp!=wxEmptyString)
    {
        wxMilliSleep(100);
    }
    event.Skip();
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

    RegisterHandler("set_error_cmd", [this](const nlohmann::json& json_data) { this->handle_set_error_cmd(json_data); });

    RegisterHandler("request_update_plate_thumbnail", [this](const nlohmann::json& json_data) {
        this->handle_request_update_plate_thumbnail(json_data);
    });

    RegisterHandler("forward_device_detail", [this](const nlohmann::json& json_data) {
        this->handle_forward_device_detail(json_data);
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
        commandJson["data"] = DataCenter::Ins().GetData();

        std::string commandStr = commandJson.dump();
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

        if (DM::AppMgr::Ins().Invoke(m_browser, evt.GetString().ToUTF8().data()))
        {
            return;
        }

        wxString strInput = evt.GetString();
        BOOST_LOG_TRIVIAL(trace) << "DeviceDialog::OnScriptMessage;OnRecv:" << strInput.c_str();
        json     j        = json::parse(strInput);

        wxString strCmd = j["command"];
        BOOST_LOG_TRIVIAL(trace) << "DeviceDialog::OnScriptMessage;Command:" << strCmd;

        if (m_commandHandlers.find(strCmd.ToStdString()) != m_commandHandlers.end()) {
            m_commandHandlers[strCmd.ToStdString()](j);
        } else {
            BOOST_LOG_TRIVIAL(trace) << "CxSentToPrinterDialog::OnScriptMessage;Unknown Command:" << strCmd;
        }

    } catch (std::exception &e) {
       // wxMessageBox(e.what(), "json Exception", MB_OK);
        BOOST_LOG_TRIVIAL(trace) << "DeviceDialog::OnScriptMessage;Error:" << e.what();
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

void CxSentToPrinterDialog::handle_forward_device_detail(const nlohmann::json& json_data)
{
    wxCommandEvent e = wxCommandEvent(wxCUSTOMEVT_NOTEBOOK_SEL_CHANGED);
    e.SetId(9); // printer details page
    wxPostEvent(wxGetApp().mainframe->topbar(), e);

    std::string ipAddress = json_data.contains("ip") ? json_data["ip"].get<std::string>() : "";
    std::string printer_name = json_data.contains("name") ? json_data["name"].get<std::string>() : "";

    // Create top-level JSON object
    nlohmann::json top_level_json = {
        {"printer_ip", ipAddress},
        {"printer_name", printer_name},
    };

    // Create command JSON object
    nlohmann::json commandJson = {
        {"command", "switch_printer_details_page"},
        {"data", top_level_json.dump(-1, ' ', true)}};

    wxGetApp().mainframe->get_printer_mgr_view()->ExecuteScriptCommand(RemotePrint::Utils::url_encode(commandJson.dump(-1, ' ', true)));

    // close the SendToPrinter dialog
    // Show(false);
    this->Close(true);
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
    std::vector<int> plate_extruders = plate->get_extruders(true);
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
     
    boost::filesystem::path temp_path(m_plater->model().get_backup_path("Metadata"));
    temp_path /= (boost::format(".%1%.%2%_upload.3mf") % get_current_pid()%plateIndex).str();

    std::string tmp_3mf_path = temp_path.string();
    int result = m_plater->export_3mf(tmp_3mf_path, SaveStrategy::UploadToPrinter);
    if(result < 0) {
        return;
    }
    m_uploadingIp = ipAddress;
    RemotePrint::RemotePrinterManager::getInstance().pushUploadTasks(
        ipAddress.ToStdString(), upload3mfName, tmp_3mf_path,
        [this](std::string ip, float progress) {
            nlohmann::json top_level_json;
            top_level_json["printer_ip"] = ip;
            top_level_json["progress"]   = progress;

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
        }, [this](std::string ip, std::string body) {

            nlohmann::json commandJson;
            commandJson["command"] = "notify_send_complete";
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
        });
}

void CxSentToPrinterDialog::handle_cancel_send(const nlohmann::json& json_data) {
    int      plateIndex = json_data["plateIndex"];
    wxString ipAddress  = json_data["ipAddress"];
    RemotePrint::RemotePrinterManager::getInstance().cancelUpload(ipAddress.ToStdString());
}

void CxSentToPrinterDialog::handle_register_complete(const nlohmann::json& json_data)
{
    if (m_plater) {
        update_send_page_content();
        update_device_data_on_send_page();
    }
}


void CxSentToPrinterDialog::handle_send_gcode(const nlohmann::json& json_data) 
{
    int      plateIndex = json_data["plateIndex"];
    wxString ipAddress  = json_data["ipAddress"];
    std::string uploadName = json_data["uploadName"];  // convert from wxString to std::string would cause exception
    
    PartPlate* plate = wxGetApp().plater()->get_partplate_list().get_plate(plateIndex);
    if (plate) {
        std::string gcodeFilePath;
        if (m_plater->only_gcode_mode()) {
            boost::filesystem::path filePath = std::string(m_plater->get_last_loaded_gcode().utf8_str());
            //if(boost::filesystem::exists(filePath))
            gcodeFilePath = filePath.string();
        }
        else{
            gcodeFilePath = _L(plate->get_tmp_gcode_path()).ToUTF8();
        }
        
        m_uploadingIp = ipAddress;
        RemotePrint::RemotePrinterManager::getInstance().pushUploadTasks(
            ipAddress.ToStdString(), uploadName, gcodeFilePath,
            [this](std::string ip, float progress) {

                nlohmann::json top_level_json;
                top_level_json["printer_ip"] = ip;
                top_level_json["progress"]  = progress;

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
            [this](std::string ip, std::string body){
                int deviceType = 0;//local device
                int statusCode = 1;
                json jBody = json::parse(body);
                if (jBody.contains("code") && jBody["code"].is_number_integer()) {
                    statusCode = jBody["code"];
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
    GCodeProcessorResult* current_result = m_plater->get_partplate_list().get_current_slice_result();
    m_backup_extruder_colors.clear();
    for(auto color :current_result->creality_default_extruder_colors)
    {
        colors_json.push_back(color);
        m_backup_extruder_colors.push_back(color);
    }
    //filament_types_json
    for(auto filament_type :current_result->creality_extruder_types)
    {
        filament_types_json.push_back(filament_type);
    }
    int size;
    unsigned char* imgdata;
    ThumbnailData data = m_plater->get_gcode_thumbnail();
    size               = data.pixels.size();
    imgdata            = new unsigned char[size];
    memcpy(imgdata, data.pixels.data(), size);
    std::size_t encoded_size = boost::beast::detail::base64::encoded_size(size);
    std::string img_base64_data(encoded_size, '\0');
    boost::beast::detail::base64::encode(&img_base64_data[0], imgdata, size);

    json_data["image"]              = "data:image/png;base64," + std::move(img_base64_data);
    json_data["plate_index"]        = 0;
    boost::filesystem::path default_output_file = boost::filesystem::path(m_plater->get_last_loaded_gcode().c_str());
    if(boost::filesystem::exists(default_output_file))
    {
        json_data["upload_gcode__name"] =default_output_file.filename().string();
    }
    

    nlohmann::json extruders_json = nlohmann::json::array();
    int extruder_index = 1;
    for (const auto& extruder : current_result->creality_default_extruder_colors) {
        if(!extruder.empty())
        {
            extruders_json.push_back(extruder_index);
        }
        extruder_index++;    
    }
    json_data["plate_extruders"] = extruders_json;

    wxString total_weight_str;
    wxString print_time_str;
    get_gcode_display_info(total_weight_str, print_time_str, m_plater->get_partplate_list().get_curr_plate());
    json_data["total_weight"] = total_weight_str.ToStdString();
    json_data["print_time"] = print_time_str.ToStdString();

    json_array.push_back(json_data);

    nlohmann::json top_level_json;
    top_level_json["extruder_colors"] = std::move(colors_json);
    top_level_json["filament_types"]    = std::move(filament_types_json);
    top_level_json["plates"]          = std::move(json_array);

    int cur_plate_index = 0;
    top_level_json["current_plate_index"] = 0;
    std::string printer_name;
    if(current_result->printer_model.empty())
    {
        printer_name = _L("Unknown model").ToUTF8().data();
    }else{
        printer_name = (boost::format("%s %.1f nozzle") % current_result->printer_model % current_result->nozzle_diameter).str();
    }
    
    //std::string presetname = wxGetApp().preset_bundle->prints.get_selected_preset_name();
    top_level_json["preset_name"] = printer_name;

    std::string json_str         = top_level_json.dump(-1, ' ', true);

    // create command to send to the webview
    nlohmann::json commandJson;
    commandJson["command"] = "update_plate_data";
    commandJson["data"]    = RemotePrint::Utils::url_encode(json_str);

    std::string commandStr = commandJson.dump(-1, ' ', true);

    return RemotePrint::Utils::url_encode(commandStr);
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

            std::vector<int> plate_extruders = plate->get_extruders(true);

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

            json_array.push_back(json_data);
        }
    }

    nlohmann::json top_level_json;
    top_level_json["extruder_colors"] = std::move(colors_json);
    top_level_json["filament_types"]    = std::move(filament_types_json);
    top_level_json["plates"]          = std::move(json_array);

    bool is_all_plates = wxGetApp().plater()->get_preview_canvas3D()->is_all_plates_selected();
    int cur_plate_index = is_all_plates ? 0 : m_plater->get_partplate_list().get_curr_plate_index();
    top_level_json["current_plate_index"] = cur_plate_index;
    
    std::string printer_name = Slic3r::GUI::wxGetApp().preset_bundle->printers.get_selected_preset_name();

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

void CxSentToPrinterDialog::on_device_list_updated()
{
    update_device_data_on_send_page();
}

std::string CxSentToPrinterDialog::get_device_data_on_show()
{
    nlohmann::json json_array = nlohmann::json::array();

    auto& deviceDB = RemotePrint::DeviceDB::getInstance();

    for (const auto& device : deviceDB.devices)
    {
        nlohmann::json deviceJson;
        deviceJson["online"] = device.online;
        deviceJson["deviceState"] = device.deviceState;
        deviceJson["model"] = device.model;
        deviceJson["mac"] = device.mac;
        deviceJson["address"] = device.address;
        deviceJson["name"] = device.name;
        deviceJson["group"] = device.group;
        deviceJson["deviceType"] = device.deviceType;
        deviceJson["isCurrentDevice"] = device.isCurrentDevice;
        deviceJson["webrtcSupport"] = device.webrtcSupport;
        deviceJson["deviceType"] = device.deviceType;
        deviceJson["tbId"] = device.tbId;

        nlohmann::json boxColorInfosJson = json::array();
        for (const auto& boxColorInfo : device.boxColorInfos)
        {
            nlohmann::json boxColorInfoJson;
            boxColorInfoJson["boxType"] = boxColorInfo.boxType;
            boxColorInfoJson["color"] = boxColorInfo.color;
            boxColorInfoJson["boxId"] = boxColorInfo.boxId;
            boxColorInfoJson["materialId"] = boxColorInfo.materialId;
            boxColorInfoJson["filamentType"] = boxColorInfo.filamentType;
            boxColorInfoJson["filamentName"] = boxColorInfo.filamentName;
            boxColorInfoJson["cId"] = boxColorInfo.cId;
            boxColorInfosJson.push_back(boxColorInfoJson);
        }
        deviceJson["boxColorInfos"] = boxColorInfosJson;

        json_array.push_back(deviceJson);
    }

    nlohmann::json top_level_json;
    top_level_json["all_devices"]          = std::move(json_array);
    top_level_json["current_device_mac"] = deviceDB.get_current_device_id();

    std::string json_str         = top_level_json.dump();

    return json_str;
}

void CxSentToPrinterDialog::update_device_data_on_send_page()
{
    nlohmann::json commandJson;
    commandJson["command"] = "update_device_list";
    commandJson["data"] = RemotePrint::Utils::url_encode(get_device_data_on_show());

    std::string commandStr = commandJson.dump(-1, ' ', true);
    wxString strJS = wxString::Format("window.handleStudioCmd('%s');", RemotePrint::Utils::url_encode(commandStr));

    run_script(strJS.ToStdString());
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
            double density     = current_result->filament_densities.at(extruder_id);
            // double cost        = current_result->filament_costs.at(extruder_id);
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

} // namespace GUI
} // namespace Slic3r