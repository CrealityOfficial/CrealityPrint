#include "PrinterMgrView.hpp"

#include "../../I18N.hpp"
#include "../AccountDeviceMgr.hpp"
#include "slic3r/GUI/wxExtensions.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "slic3r/GUI/Notebook.hpp"
#include "slic3r/GUI/PartPlate.hpp"
#include "libslic3r/Thread.hpp"
#include "libslic3r_version.h"

#include <regex>
#include <string>
#include <wx/sizer.h>
#include <wx/string.h>
#include <wx/toolbar.h>
#include <wx/textdlg.h>
#include "wx/evtloop.h"

#include <slic3r/GUI/Widgets/WebView.hpp>
#include <wx/webview.h>
#include "slic3r/GUI/print_manage/RemotePrinterManager.hpp"
#include <boost/beast/core/detail/base64.hpp>

#include <wx/stdpaths.h>
#include "../utils/cxmdns.h"
#include "slic3r/GUI/print_manage/Utils.hpp"
#include "slic3r/GUI/print_manage/AccountDeviceMgr.hpp"
#include "slic3r/GUI/AnalyticsDataUploadManager.hpp"
#include "wx/event.h"
#include "../data/DataCenter.hpp"
#include "../AppMgr.hpp"
#include "../PrinterMgr.hpp"
#include "../TypeDefine.hpp"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#if defined(__linux__) || defined(__LINUX__)
#include "video/WebRTCDecoder.h"
#endif
#include <slic3r/GUI/print_manage/AppUtils.hpp>
#include "buildinfo.h"

namespace pt = boost::property_tree;

namespace Slic3r {
namespace GUI {

PrinterMgrView::PrinterMgrView(wxWindow *parent)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize)
 {
    BOOST_LOG_TRIVIAL(warning) <<__FUNCTION__ << " Address: " << (void*) this;
    wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);

      // Create the webview
    m_browser = WebView::CreateWebView(this, "");
    if (m_browser == nullptr) {
        BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " m_browser is null!!! ";
        wxLogError("Could not init m_browser");
        return;
    }
    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " m_browser address: " << (void*) m_browser;

    m_browser->Bind(wxEVT_WEBVIEW_ERROR, &PrinterMgrView::OnError, this);
    m_browser->Bind(wxEVT_WEBVIEW_LOADED, &PrinterMgrView::OnLoaded, this);
    Bind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED, &PrinterMgrView::OnScriptMessage, this, m_browser->GetId());

    SetSizer(topsizer);

    topsizer->Add(m_browser, wxSizerFlags().Expand().Proportion(1));

    //Zoom
    m_zoomFactor = 100;

    //Connect the idle events
    Bind(wxEVT_CLOSE_WINDOW, &PrinterMgrView::OnClose, this);

    RegisterHandler("set_device_relate_to_account", [this](const nlohmann::json& json_data) {
        this->handle_set_device_relate_to_account(json_data);
    });

    RegisterHandler("request_update_device_relate_to_account", [this](const nlohmann::json& json_data) {
        this->handle_request_update_device_relate_to_account(json_data);
    });
    std::string version = std::string(CREALITYPRINT_VERSION);
    std::string os = wxGetOsDescription().ToStdString();
    int port = wxGetApp().get_server_port();
    int customized = 0;
    #ifdef CUSTOMIZED
        customized = 1;
    #endif
//#define _DEBUG1 
#ifdef _DEBUG1
     wxString url = wxString::Format("http://localhost:5173/?version=%s&port=%d&os=%s&customized=%d", version, port, os, customized);
        this->load_url(url, wxString());
         m_browser->EnableAccessToDevTools();
     #else
        //wxString url = wxString::Format("http://localhost:%d/deviceMgr/index.html", wxGetApp().get_server_port());
        
        wxString url = wxString::Format("%s/web/deviceMgr/index.html?version=%s&port=%d&os=%s&customized=%d", from_u8(resources_dir()),
                                        version, port, os, customized);
        url.Replace(wxT("\\"), wxT("/"));
        url.Replace(wxT("#"), wxT("%23"));
        wxURI uri(url);
        wxString encodedUrl = uri.BuildURI();
        encodedUrl = wxT("file://")+encodedUrl;
        this->load_url(encodedUrl, wxString());

        //this->load_url(wxString("http://localhost:5173/"), wxString());
        m_browser->EnableAccessToDevTools();

     #endif
    
    #ifdef __WXGTK__
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetDoubleBuffered(true);
    m_freshTimer = new wxTimer();
    m_freshTimer->SetOwner(this);
    m_freshTimer->Start(500);
    Bind(wxEVT_TIMER, [this](wxTimerEvent&){
        this->Refresh();
    });
    #endif
    DM::AppMgr::Ins().Register(m_browser);
    DM::AppMgr::Ins().RegisterEvents(m_browser, std::vector<std::string>{DM::EVENT_SET_CURRENT_DEVICE, DM::EVENT_FORWARD_DEVICE_DETAIL});
 }

PrinterMgrView::~PrinterMgrView()
{
    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " Address: " << (void*) this;

#ifdef __WXGTK__
    m_freshTimer->Stop();
    m_browser->Stop();
    m_browser->RemoveScriptMessageHandler("wx");
#endif
    DM::AppMgr::Ins().UnRegister(m_browser);
    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " Start";
    SetEvtHandlerEnabled(false);
    m_scanExit = true;
    m_scanPoolThread.join();
    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " End";
}

void PrinterMgrView::load_url(const wxString& url, wxString apikey)
{
    if (m_browser == nullptr)
        return;
    m_apikey = apikey;
    m_apikey_sent = false;
    void* backend_before = m_browser->GetNativeBackend();
    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << "[LOAD_URL_ACTION] START. webView=" << (void*) m_browser
                               << ", Backend Ptr BEFORE: " << backend_before
                               << ", URL: " << url.ToStdString();

    m_browser->LoadURL(url);

    void* backend_after = m_browser->GetNativeBackend();
    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << "[LOAD_URL_ACTION] END (call returned). webView=" << (void*) m_browser
                               << ", Backend Ptr AFTER: " << backend_after;
    //m_browser->SetFocus();
    UpdateState();
}

void PrinterMgrView::on_switch_to_device_page()
{
    //update_which_device_is_current();
    forward_init_device_cmd_to_printer_list();
}

void PrinterMgrView::reload()
{
    void* backend_before = m_browser->GetNativeBackend();
    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << "[LOAD_URL_ACTION] START. webView=" << (void*) m_browser
                               << ", Backend Ptr BEFORE: " << backend_before ;
    m_browser->Reload();
    void* backend_after = m_browser->GetNativeBackend();
    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << "[LOAD_URL_ACTION] END (call returned). webView=" << (void*) m_browser
                               << ", Backend Ptr AFTER: " << backend_after;
}
/**
 * Method that retrieves the current state from the web control and updates the
 * GUI the reflect this current state.
 */
void PrinterMgrView::UpdateState() {
  // SetTitle(m_browser->GetCurrentTitle());

}

void PrinterMgrView::OnClose(wxCloseEvent& evt)
{
    this->Hide();
}

void PrinterMgrView::SendAPIKey()
{
    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " start";
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

    void* backend_before = m_browser->GetNativeBackend();
    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << "[LOAD_URL_ACTION] START. webView=" << (void*) m_browser
                               << ", Backend Ptr BEFORE: " << backend_before ;
    m_browser->AddUserScript(script);
    m_browser->Reload();

    void* backend_after = m_browser->GetNativeBackend();
    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << "[LOAD_URL_ACTION] END (call returned). webView=" << (void*) m_browser
                               << ", Backend Ptr AFTER: " << backend_after;
    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " end";
}

void PrinterMgrView::OnError(wxWebViewEvent &evt)
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
    BOOST_LOG_TRIVIAL(error) << __FUNCTION__<< boost::format(": error loading page %1% %2% %3% %4%") %evt.GetURL() %evt.GetTarget() %e %evt.GetString();
}

void PrinterMgrView::OnLoaded(wxWebViewEvent &evt)
{
    if (evt.GetURL().IsEmpty())
        return;
    SendAPIKey();

    DM::DeviceMgr::Ins().Load();
    AccountDeviceMgr::getInstance().load();
    json groupData = DM::DeviceMgr::Ins().GetData();
    for (auto it = groupData["groups"].begin(); it != groupData["groups"].end(); it++)
        {
            auto& group = it.value();
            if (group.contains("list"))
            {
                for (auto jt = group["list"].begin(); jt != group["list"].end(); jt++)
                {
                    std::string address = jt.value()["address"].get<std::string>();
                    std::string mac = jt.value()["mac"].get<std::string>();
                    m_devicePool[mac] = address;
                }
            }
        }
    correct_device();
}
void PrinterMgrView::sendProgressWithRateLimit(std::string ip,float progress,double speed)
{
    std::lock_guard<std::mutex> lock(sendMutex);
    auto now = std::chrono::steady_clock::now();
    if (now - lastSendTime >= std::chrono::milliseconds(500)) {
        lastSendTime = now;
    }else{
        return;
    }
    std::cout << "Progress: " << progress << "% for IP: " << ip << std::endl;
                            nlohmann::json top_level_json;
                            top_level_json["ip"] = ip;
                            top_level_json["progress"]  = progress;
                            top_level_json["speed"]  = round(speed);

                            std::string json_str = top_level_json.dump();
                            nlohmann::json commandJson;
                            commandJson["command"] = "upload_progress";
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
}
void PrinterMgrView::OnScriptMessage(wxWebViewEvent& evt)
{
    try
    {

        if (DM::AppMgr::Ins().Invoke(m_browser, evt.GetString().ToUTF8().data()))
        {
            return;
        }

        std::string strInput = evt.GetString().ToStdString();
        BOOST_LOG_TRIVIAL(trace) << "DeviceDialog::OnScriptMessage;OnRecv:" << strInput.c_str();
        json     j = json::parse(strInput);

        std::string strCmd = j["command"];
        BOOST_LOG_TRIVIAL(trace) << "DeviceDialog::OnScriptMessage;Command:" << strCmd;

        if (strCmd == "send_gcode")
        {
            int plateIndex = j["plateIndex"];
            std::string ipAddress = j["ipAddress"];
            std::string uploadName = j["uploadName"];
            bool oldPrinter = j["oldPrinter"];
            int  moonrakerPort = j["moonrakerPort"];

            if (oldPrinter)
            {
                std::string strIpAddr = ipAddress;
                RemotePrint::RemotePrinterManager::getInstance().setOldPrinterMap(strIpAddr);
            }
            if (moonrakerPort > 0)
            {
                std::string strIpAddr = ipAddress;
                if (strIpAddr.find('(') != std::string::npos)
                {
                    RemotePrint::RemotePrinterManager::getInstance().setKlipperPrinterMap(strIpAddr, moonrakerPort);
                }
            }

            PartPlate* plate = wxGetApp().plater()->get_partplate_list().get_plate(plateIndex);
            if (plate)
            {
                // upload analytics data here
                AnalyticsDataUploadManager::getInstance().triggerUploadTasks(AnalyticsUploadTiming::ON_CLICK_START_PRINT_CMD,
                                                                             {AnalyticsDataEventType::ANALYTICS_GLOBAL_PRINT_PARAMS,
                                                                              AnalyticsDataEventType::ANALYTICS_OBJECT_PRINT_PARAMS}, plateIndex);

                std::string gcodeFilePath = plate->get_tmp_gcode_path();
                if (wxGetApp().plater()->only_gcode_mode())
                {
                    gcodeFilePath = wxGetApp().plater()->get_last_loaded_gcode().ToUTF8();

                }

                RemotePrint::RemotePrinterManager::getInstance().pushUploadMultTasks(ipAddress, uploadName, gcodeFilePath,
                    [this](std::string ip, float progress, double speed) {
                        // BOOST_LOG_TRIVIAL(info) << "Progress: " << progress << "% for IP: " << ip;
                        //进度发送太快会导致浏览器卡死，所以这里限制一下
                        
                        if (progress >= 1.0f)
                        {
                            sendProgressWithRateLimit(ip, progress,speed);
                        }
                    },
                    [this](std::string ip, int statusCode) {
                        nlohmann::json top_level_json;
                        top_level_json["ip"] = ip;
                        top_level_json["statusCode"]  = statusCode;
                        std::string json_str = top_level_json.dump();

                        nlohmann::json commandJson;
                        commandJson["command"] = "upload_status";
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
                    },
                    [this](std::string ip, std::string body){
                        int deviceType = 0;//local device
                        int statusCode = 1;
                        json jBody = json::parse(body);
                        if (jBody.contains("code") && jBody["code"].is_number_integer()) {
                            statusCode = jBody["code"];
                        }

                        nlohmann::json top_level_json;
                        top_level_json["ip"] = ip;
                        top_level_json["statusCode"] = statusCode;
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
                        json_str = top_level_json.dump(-1, ' ', true);

                        nlohmann::json commandJson;
                        commandJson["command"] = "upload_complete";
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

              });
            }
        }else if(strCmd == "down_files")
        {
            
            json urls = j["urls"];
            std::string path_type = j["file_type"];
            
            wxDirDialog dlg(this, _L("Please Select"), "", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
            
            if (dlg.ShowModal() != wxID_OK) {
                return;
            }
            wxString path = dlg.GetPath();
            std::vector<std::string> download_infos;
            for (auto iter = urls.begin(); iter != urls.end(); iter++)
            {
                std::string url = iter->get<std::string>();
                download_infos.push_back(url);
            }
            this->down_files(download_infos, path.ToUTF8().data(), path_type);
            

        }
        else if (strCmd == "down_file")
        {
            std::string url = j["url"];
            std::string name = j["name"];
            std::string file_type = j["file_type"];

            this->down_file(url, name, file_type);
        }
        else if (strCmd.compare("common_openurl") == 0)
        {
            boost::optional<std::string> path = j["url"];
            if (path.has_value())
            {
                wxLaunchDefaultBrowser(path.value());
            }
        }
        else if (strCmd == "scan_device")
        {
            scan_device();
        }

        else if (m_commandHandlers.find(strCmd) != m_commandHandlers.end())
        {
            m_commandHandlers[strCmd](j);
        }
        else if (strCmd == "req_device_move_direction")
        {
            std::string presetName = j["preset_name"];
            std::string address = j["address"];
            int direction = 0;
            int machine_LED_light_exist = 0;
            int auxiliary_fan = 0;
            int support_air_filtration = 0;
            int machine_ptc_exist = 0;
            Preset*     preset = Slic3r::GUI::wxGetApp().preset_bundle->printers.find_preset(presetName);
            if (preset != nullptr) {
                if(preset->config.has("machine_platform_motion_enable"))
                {
                    const ConfigOption* option = preset->config.option("machine_platform_motion_enable");
                    bool b = option->getBool();
                    direction = b ? 1 : 0;
                }
                if (preset->config.has("support_air_filtration"))
                {
                    const ConfigOption* option = preset->config.option("support_air_filtration");
                    bool b = option->getBool();
                    support_air_filtration = b ? 1 : 0;
                }
                if (preset->config.has("machine_LED_light_exist"))
                {
                    const ConfigOption* option = preset->config.option("machine_LED_light_exist");
                    bool b = option->getBool();
                    machine_LED_light_exist = b ? 1 : 0;
                }
                if (preset->config.has("auxiliary_fan"))
                {
                    const ConfigOption* option = preset->config.option("auxiliary_fan");
                    bool b = option->getBool();
                    auxiliary_fan = b ? 1 : 0;
                }
                if(preset->config.has("machine_ptc_exist"))
                {
                    const ConfigOption* option = preset->config.option("machine_ptc_exist");
                    bool b = option->getBool();
                    machine_ptc_exist     = b ? 1 : 0;
                }
                nlohmann::json commandJson;
                nlohmann::json  dataJson;
                commandJson["command"] = "req_device_move_direction";
                dataJson["direction"]  = direction;
                dataJson["machine_LED_light_exist"]  = machine_LED_light_exist;
                dataJson["auxiliary_fan"]  = auxiliary_fan;
                dataJson["support_air_filtration"]  = support_air_filtration;
                dataJson["machine_ptc_exist"]  = machine_ptc_exist;
                
                dataJson["address"]    = address;
                commandJson["data"]    = dataJson;
                wxString strJS = wxString::Format("window.handleStudioCmd('%s');", commandJson.dump());
                run_script(strJS.ToStdString());
            }
        }
        else if (strCmd == "get_machine_list")
        {
            load_machine_preset_data();
        }else if(strCmd == "switch_webrtc_source")
        {
            std::string ip = j["ip"];
            std::string video_url = (boost::format("http://%1%:8000/call/webrtc_local") % ip).str();
#if defined(__linux__) || defined(__LINUX__)
            WebRTCDecoder::GetInstance()->startPlay(video_url); 
#endif
        }
        else if (strCmd == "get_file_List_from_lan_device")
        {
            std::string strIp = j["url"];
            getFileListFromLanDevice(strIp);
        }
        else if (strCmd == "delete_file_from_lan_device")
        {
            std::string strIp = j["url"];
            std::string strName = j["name"];

            deleteFileListFromLanDevice(strIp,strName);
        }
        else if (strCmd == "uploade_file_oldPrinter")
        {
            std::string strIp = j["url"];

            int num = uploadeFileLanDevice(strIp);
            nlohmann::json commandJson;
            commandJson["data"] = num;
            commandJson["command"] = "uploade_file_oldPrinter";
            wxString strJS = wxString::Format("window.handleStudioCmd('%s');", commandJson.dump());
            run_script(strJS.ToStdString());
        }
        else if (strCmd == "cancel_upload")
        {
            wxString ipAddress  = j["ipAddress"];
            RemotePrint::RemotePrinterManager::getInstance().cancelUpload(ipAddress.ToStdString());
        }
        else if (strCmd == "diagnosis_lan_connect")
        {
            std::string ip = j["ip"];
            Slic3r::create_thread([this, ip] {
                int result = DM::LANConnectCheck::checkLan(ip, _ctrl);
                nlohmann::json commandJson;
                nlohmann::json resultJson;
                commandJson["command"] = "diagnosis_lan_connect_result";
                resultJson["ip"] = ip;
                resultJson["errorcode"] = result;
                commandJson["data"] = resultJson;
                wxString strJS = wxString::Format("window.handleStudioCmd('%s');", commandJson.dump());
                wxGetApp().CallAfter([this, strJS] { run_script(strJS.ToStdString()); });
            });
        }
        else if (strCmd == "diagnosis_close_cmd")
        {
            _ctrl.requestStop();
            _ctrl.reset();
        }
        
        else {
            BOOST_LOG_TRIVIAL(trace) << "PrinterMgrView::OnScriptMessage;Unknown Command:" << strCmd;
        }
    }
    catch (std::exception& e)
    {
        // wxMessageBox(e.what(), "json Exception", MB_OK);
        BOOST_LOG_TRIVIAL(trace) << "DeviceDialog::OnScriptMessage;Error:" << e.what();
    }

}

std::string PrinterMgrView::get_plate_data_on_show()
{
    nlohmann::json json_array = nlohmann::json::array();

    std::vector<std::string> extruder_colors = Slic3r::GUI::wxGetApp().plater()->get_extruder_colors_from_plater_config();
    nlohmann::json           colors_json     = nlohmann::json::array();
    for (const auto& color : extruder_colors) {
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
        if (plate && !plate->empty() && plate->is_slice_result_valid() && plate->is_slice_result_ready_for_print() && plate->thumbnail_data.is_valid()) {
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

            wxImage resized_image = image.Rescale(50, 50, wxIMAGE_QUALITY_HIGH);

            wxMemoryOutputStream mem_stream;
            if (!resized_image.SaveFile(mem_stream, wxBITMAP_TYPE_PNG)) {
                wxLogError("Failed to save image to memory stream.");
            }

            auto size = mem_stream.GetSize();
            // wxMemoryBuffer buffer(size);
            auto imgdata = new unsigned char[size];
            mem_stream.CopyTo(imgdata, size);

            std::size_t encoded_size = boost::beast::detail::base64::encoded_size(size);
            std::string img_base64_data(encoded_size, '\0');
            boost::beast::detail::base64::encode(&img_base64_data[0], imgdata, size);

            std::string default_gcode_name = "";

            std::vector<int> plate_extruders = plate->get_extruders(true);

            if (Slic3r::GUI::wxGetApp().plater()->only_gcode_mode()) {
                wxString   last_loaded_gcode = Slic3r::GUI::wxGetApp().plater()->get_last_loaded_gcode();
                wxFileName fileName(last_loaded_gcode);
                default_gcode_name = std::string(fileName.GetName().ToUTF8().data()) + ".gcode";
                plate_extruders    = Slic3r::GUI::wxGetApp().plater()->get_gcode_extruders_in_only_gcode_mode();
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
                                         get_bbl_time_dhms(plate_time_mode.time) + ".gcode";
                } else {
                    default_gcode_name = "plate" + std::to_string(i + 1) + ".gcode";
                }
            }

            nlohmann::json json_data;
            json_data["image"]              = "data:image/png;base64," + std::move(img_base64_data);
            json_data["plate_index"]        = plate->get_index();
            json_data["upload_gcode__name"] = std::move(default_gcode_name);

            nlohmann::json extruders_json = nlohmann::json::array();
            for (const auto& extruder : plate_extruders) {
                extruders_json.push_back(extruder);
            }
            json_data["plate_extruders"] = extruders_json;

            json_array.push_back(json_data);
        }
    }

    nlohmann::json top_level_json;
    top_level_json["extruder_colors"] = std::move(colors_json);
    top_level_json["filament_types"]    = std::move(filament_types_json);
    top_level_json["plates"]          = std::move(json_array);

    std::string json_str         = top_level_json.dump(-1, ' ', true);

    // create command to send to the webview
    nlohmann::json commandJson;
    commandJson["command"] = "update_plate_data";
    commandJson["data"]    = RemotePrint::Utils::url_encode(json_str);

    std::string commandStr = commandJson.dump(-1, ' ', true);

    return RemotePrint::Utils::url_encode(commandStr);

}

bool PrinterMgrView::Show(bool show) 
{
    bool result = wxPanel::Show(show);

    if (show) {
        wxString strJS = wxString::Format("window.handleStudioCmd('%s');", get_plate_data_on_show());
        run_script(strJS.ToStdString());
    }

    return result;
}

void PrinterMgrView::run_script(std::string content)
{
    void* backend_after = m_browser->GetNativeBackend();
    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " this Address: " << (void*) this << " wxWebView address: " << (void*) m_browser
                               << ", Backend Ptr AFTER: " << backend_after;   
    boost::log::core::get()->flush();
    WebView::RunScript(m_browser, content);
}
std::string getFileNameFromURL(const std::string& url) {
    // ???std::istringstream?????URL
    std::istringstream iss(url);
    std::string segment;
    std::string fileName;
 
    // ??????????'/'?????��??
    size_t lastIndex = url.find_last_of("/\\");
    if (lastIndex != std::string::npos) {
        fileName = url.substr(lastIndex + 1); // ????????'/'??'\\'????????????
    } else {
        // ???URL?????'/'????????URL???????????
        fileName = url;
    }
 
    return fileName;
}
std::string filterInvalidFileNameChars(const std::string& input) {
    std::string result = input;
    for (char& c : result) {
        if (c == '\\' || c == '/' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            c = '-';
        }
    }
    return result;
}
void PrinterMgrView::down_files(std::vector<std::string> download_infos, std::string savePath, std::string path_type )
{        
        boost::thread import_thread = Slic3r::create_thread([savePath, download_infos, this] {
            for(auto& download_info : download_infos)
            {
                boost::filesystem::path  target_path = savePath; 
                target_path = target_path / filterInvalidFileNameChars(getFileNameFromURL(Http::url_decode(download_info)));   
                wxString download_url(download_info.c_str());
                fs::path tmp_path = target_path;
                tmp_path += wxSecretString::Format("%s",".download");

                auto filesize = 0;
                bool size_limit = false;
                auto http = Http::get(download_url.ToStdString());

                http.on_progress([filesize, size_limit, this](Http::Progress progress, bool& cancel) {
                        if (progress.dltotal != 0) {
                            int percent = progress.dlnow * 100 / progress.dltotal;

                            nlohmann::json commandJson;
                            commandJson["command"] = "update_download_progress";
                            commandJson["data"]    = percent;

                            wxString strJS = wxString::Format("window.handleStudioCmd('%s');", RemotePrint::Utils::url_encode(commandJson.dump()));
                            wxGetApp().CallAfter([this, strJS] { run_script(strJS.ToStdString()); });
                        }
                    })
                    .on_error([this](std::string body, std::string error, unsigned http_status) {
                        (void) body;
                        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << boost::format("Error getting: HTTP %1%, %2%")%http_status%error;
                        nlohmann::json commandJson;
                        commandJson["command"] = "update_download_progress";
                    
                        commandJson["data"]    = -2;

                        wxString strJS = wxString::Format("window.handleStudioCmd('%s');", RemotePrint::Utils::url_encode(commandJson.dump()));
                        wxGetApp().CallAfter([this, strJS] { run_script(strJS.ToStdString()); });
                        return;
                    }).on_complete([tmp_path, target_path](std::string body, unsigned http_status) {
                        if(http_status!=200){
                            return ;
                        }
                        fs::fstream file(tmp_path, std::ios::out | std::ios::binary | std::ios::trunc);
                        file.write(body.c_str(), body.size());
                        file.close();

                        fs::rename(tmp_path, target_path);
                    })
                    .perform_sync();
            }
        });
    
}
void PrinterMgrView::down_file(std::string download_info, std::string filename, std::string path_type )
{
    FileType file_type = FT_GCODE;
    wxStandardPaths& stdPaths = wxStandardPaths::Get();
    wxString docsDir = stdPaths.GetDocumentsDir();
    if(path_type == "Videos"){
        docsDir = stdPaths.GetUserDir(wxStandardPaths::Dir_Videos);
        file_type  =FT_VIDEO;
    }

    fs::path output_file(filename);
    fs::path output_path;
    wxString extension = output_file.extension().string();
    
    std::string ext = "";
    
    wxFileDialog dlg(this, _L(""), docsDir, from_path(output_file.filename()),
        GUI::file_wildcards(file_type, ext), wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxPD_APP_MODAL);

//     wxFileDialog dlg(this, _L(""),
//         docsDir.ToStdString(),
//         from_path(output_file.filename()),
//         GUI::file_wildcards(file_type, ext),
//         wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxPD_APP_MODAL
//     );

    if (dlg.ShowModal() != wxID_OK) {
        return;
    }

    output_path = into_path(dlg.GetPath());
    output_file = into_path(dlg.GetFilename());

    wxString download_url(download_info.c_str());

    boost::filesystem::path target_path = output_path;
    boost::thread import_thread = Slic3r::create_thread([extension, target_path, download_url, this] {

        fs::path tmp_path = target_path;
        tmp_path += wxSecretString::Format("%s", ".download");

        auto filesize = 0;
        bool size_limit = false;
        auto http = Http::get(download_url.ToStdString());

        http.on_progress([filesize, size_limit, this](Http::Progress progress, bool& cancel) {
                if (progress.dltotal != 0) {
                    int percent = progress.dlnow * 100 / progress.dltotal;

                    nlohmann::json commandJson;
                    commandJson["command"] = "update_download_progress";
                    commandJson["data"]    = percent;

                    wxString strJS = wxString::Format("window.handleStudioCmd('%s');", RemotePrint::Utils::url_encode(commandJson.dump()));
                    wxGetApp().CallAfter([this, strJS] { run_script(strJS.ToStdString()); });
                }
            })
            .on_error([this](std::string body, std::string error, unsigned http_status) {
                (void) body;
                BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << boost::format("Error getting: HTTP %1%, %2%")%http_status%error;
                nlohmann::json commandJson;
                    commandJson["command"] = "update_download_progress";
                    commandJson["data"]    = -2;

                    wxString strJS = wxString::Format("window.handleStudioCmd('%s');", RemotePrint::Utils::url_encode(commandJson.dump()));
                    wxGetApp().CallAfter([this, strJS] { run_script(strJS.ToStdString()); });
                return;
            })
            .on_complete([tmp_path, target_path, extension](std::string body, unsigned http_status) {
                if(http_status!=200){
                    return ;
                }
                fs::fstream file(tmp_path, std::ios::out | std::ios::binary | std::ios::trunc);
                file.write(body.c_str(), body.size());
                file.close();

                fs::rename(tmp_path, target_path);
            })
            .perform_sync();

        });

}
void PrinterMgrView::correct_device()
{
    
    if (m_scanPoolThread.joinable() && !m_scanPoolThread.try_join_for(boost::chrono::seconds(0))) {
        return;
    }
    m_scanPoolThread = Slic3r::create_thread(
        [this] {
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));
            std::vector<std::string> prefix;
            prefix.push_back("CXSWBox");
            prefix.push_back("creality");
            prefix.push_back("Creality");
            std::vector<std::string> vtIp,vtBoxIp;
            if(m_scanExit)
            {
                    return 0;
                }
            auto vtDevice = cxnet::syncDiscoveryService(prefix);
            //cxnet::machine_info info;
            //info.answer = "1";
            //info.machineIp = "172.23.215.56";
            //vtDevice.push_back(info);
            for (auto& item : vtDevice) {
                if(m_scanExit)
                {
                    return 0;
                }
                std::string answer = item.answer;
                if (answer.substr(0, 8) == "_CXSWBox")
                {
                    continue; // Skip the box devices for correction
                }
                else
                {
                std::string url = (boost::format("http://%1%/info") % item.machineIp).str();
                Slic3r::Http http_url = Slic3r::Http::get(url);    
                http_url.timeout_connect(1)
                .timeout_max(5)
                .on_complete(
                [this,item](std::string body, unsigned status) {
                    try {
                        json j = json::parse(body);
                        std::string mac = j["mac"].get<std::string>();
                        std::string vtIp = item.machineIp;
                        if(m_devicePool.count(mac))
                        {
                            if(m_devicePool[mac] == vtIp)
                            {
                                return; // Device already exists with the same IP
                            }
                            else
                            {
                                
                                nlohmann::json dataJson;
                                dataJson["mac"] = mac;  
                                dataJson["ip"] = vtIp;   

                                nlohmann::json commandJson;
                                commandJson["command"] = "correct_device";
                                commandJson["data"]    = dataJson;

                                wxString strJS = wxString::Format("window.handleStudioCmd('%s');", RemotePrint::Utils::url_encode(commandJson.dump()));           
                                wxGetApp().CallAfter([this, strJS] { run_script(strJS.ToStdString()); });            
                            }
                        }
                        m_devicePool.emplace(
                            std::make_pair(mac, vtIp)
                        );
                    }
                    catch (std::exception& e) {
                        BOOST_LOG_TRIVIAL(error) << "Error parsing JSON: " << e.what();
                        return;
                    }
                }).on_error(
                [this](std::string body, std::string error, unsigned int status) {
                    
                    }).perform_sync();
                }
            }

            return 0;
        });
}
void PrinterMgrView::scan_device() 
{
    boost::thread _thread = Slic3r::create_thread(
        [this] {
             
            std::vector<std::string> prefix;
            prefix.push_back("CXSWBox");
            prefix.push_back("creality");
            prefix.push_back("Creality");
            std::vector<std::string> vtIp,vtBoxIp,vtKlipperIp;
            auto vtDevice = cxnet::syncDiscoveryService(prefix);
            for (auto& item : vtDevice) {

                std::string answer = item.answer;
                if (answer.substr(0, 8) == "_CXSWBox")
                {
                    vtBoxIp.push_back(item.machineIp);
                }
                else
                {
                    std::regex answerRegex("_creality(\\d{2})(\\d{4}).+");
                    std::smatch matches;
                    if (std::regex_match(answer, matches, answerRegex))
                    {
                        string strMachineType = matches[1];
                        string strMoonrakerPort = matches[2];
                        if (strMachineType == "00")
                        {
                            //音速屏
                            vtKlipperIp.push_back(item.machineIp + ":"+strMoonrakerPort);
                        }
                    }
                    else
                    {
                        //if (vtIp.size() < 50)
                            vtIp.push_back(item.machineIp);
                    }
                }
            }

            nlohmann::json dataJson;
            dataJson["servers"] = vtIp;  
            dataJson["boxs"] = vtBoxIp;   
            dataJson["klipper"] = vtKlipperIp;   

            nlohmann::json commandJson;
            commandJson["command"] = "scan_device";
            commandJson["data"]    = dataJson;

            wxString strJS = wxString::Format("window.handleStudioCmd('%s');", RemotePrint::Utils::url_encode(commandJson.dump()));           
            wxGetApp().CallAfter([this, strJS] { run_script(strJS.ToStdString()); });            
        });
 

   /* if (_thread.joinable())
        _thread.join();*/
}

void     PrinterMgrView::RequestDeviceListFromDB() 
{
    // create command to send to the webview
    nlohmann::json commandJson;
    commandJson["command"] = "req_device_list";
    commandJson["data"]    = "";

    wxString strJS = wxString::Format("window.handleStudioCmd('%s');", commandJson.dump());

    run_script(strJS.ToStdString());
}

void PrinterMgrView::RegisterHandler(const std::string& command, std::function<void(const nlohmann::json&)> handler)
{
    m_commandHandlers[command] = handler;
}
void PrinterMgrView::UnregisterHandler(const std::string& command)
{
    m_commandHandlers.erase(command);
}
void PrinterMgrView::ExecuteScriptCommand(const std::string& commandInfo, bool async)
{
    if (commandInfo.empty())
        return;
    
    wxString strJS = wxString::Format("window.handleStudioCmd('%s');", commandInfo);

    if (async)
    {
        wxGetApp().CallAfter([this, strJS] { run_script(strJS.ToStdString()); });
    }
    else 
    {
        run_script(strJS.ToStdString());
    }

}

void PrinterMgrView::handle_request_update_device_relate_to_account(const nlohmann::json& json_data)
{
    if (!Slic3r::GUI::wxGetApp().is_login()) {
        return;
    }

    // the account_device_info is possibly empty, if the account is not binded to any printer
    std::string account_device_info = AccountDeviceMgr::getInstance().get_account_device_bind_json_info();

    // Create top-level JSON object
    nlohmann::json top_level_json = {
        {"accout_binded_devices", account_device_info}
    };

    // Create command JSON object
    nlohmann::json commandJson = {
        {"command", "update_device_relate_account_state"},
        {"data", top_level_json.dump()}
    };

    ExecuteScriptCommand(RemotePrint::Utils::url_encode(commandJson.dump()));
}

void PrinterMgrView::handle_set_device_relate_to_account(const nlohmann::json& json_data)
{
    try {
        std::string ipAddress   = json_data["ipAddress"];
        std::string device_mac  = json_data["device_mac"];
        std::string device_name = json_data["device_name"];
        bool        relateState = json_data["relateState"];

        std::string printerGroup = json_data["device_group"];
        std::string device_model = json_data["device_model"];

        int connectType = 0;
        if(json_data.contains("type"))
        {
            connectType = json_data["type"].get<int>();
        }

        if(relateState) {
            AccountDeviceMgr::DeviceInfo device_info;
            device_info.device_unique_id = device_mac;
            device_info.address          = ipAddress;
            device_info.mac              = device_mac;
            device_info.model            = device_model;
            device_info.connectType      = connectType;
            device_info.group            = printerGroup;

            AccountDeviceMgr::getInstance().add_to_my_devices(device_info);
        }
        else {
            AccountDeviceMgr::getInstance().unbind_device(device_mac);
        }
    } catch (std::exception& e) {

    }
}

void PrinterMgrView::request_refresh_all_device()
{
    // Create command JSON object
    nlohmann::json commandJson = {
        {"command", "refresh_all_device"},
        {"data", ""}
    };

    ExecuteScriptCommand(commandJson.dump());
}

void PrinterMgrView::forward_init_device_cmd_to_printer_list()
{
    if (!Slic3r::GUI::wxGetApp().is_login()) {
        return;
    }

    try {
        std::string reload_printer_info = AccountDeviceMgr::getInstance().get_account_device_info_for_printers_init();

        std::string account_device_info = AccountDeviceMgr::getInstance().get_account_device_bind_json_info();

        // Create top-level JSON object
        nlohmann::json top_level_json = {
            {"related_device_info", reload_printer_info},
            {"accout_binded_devices", account_device_info}
        };

        // Create command JSON object
        nlohmann::json commandJson = {
            {"command", "reinit_related_to_account_device"},
            {"data", top_level_json.dump()}
        };

        WebView::RunScript(m_browser, wxString::Format("window.handleStudioCmd('%s')", RemotePrint::Utils::url_encode(commandJson.dump())));
    }
    catch (const std::exception& e) {
        std::cout << "forward_init_device_cmd_to_printer_list failed" << std::endl;
    }

}

bool PrinterMgrView::LoadFile(std::string jPath, std::string &sContent)
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

void PrinterMgrView::request_close_detail_page()
{
    nlohmann::json commandJson;
    commandJson["command"] = "req_close_detail_page_video";
    commandJson["data"] = "";

    wxString strJS = wxString::Format("window.handleStudioCmd('%s');", commandJson.dump());

    run_script(strJS.ToStdString());
}

void PrinterMgrView::request_reopen_detail_video()
{
    nlohmann::json commandJson;
    commandJson["command"] = "req_reopen_detail_page_video";
    commandJson["data"] = "";

    wxString strJS = wxString::Format("window.handleStudioCmd('%s');", commandJson.dump());

    run_script(strJS.ToStdString());
}

int PrinterMgrView::load_machine_preset_data()
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
            wxString strTmp = wxString::Format("handleStudioCmd('%s')",pmodels.dump(-1,' ',true));
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

int PrinterMgrView::getFileListFromLanDevice(const std::string strIp)
{
    if (strIp.empty())
    {
        return -1;
    }
    Slic3r::create_thread([this,strIp]{
        CURL* curl = curl_easy_init();
        if (!curl)
        {
            return -1;
        }

        // RAII����CURL���
        struct CurlGuard
        {
            CURL* handle;
            ~CurlGuard()
            {
                if (handle) curl_easy_cleanup(handle);
            }
        } guard{ curl };

        std::string ftpUrl = "ftp://" + strIp + "/mmcblk0p1/creality/gztemp/";
        curl_easy_setopt(curl, CURLOPT_URL, ftpUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_USERPWD, "anonymous:");
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_NONE);

        // ������ʱ�ļ��洢FTP��Ӧ
        std::string tempFilePath = "/tmp/ftp_listing_temp"; // Linux/Mac��ʱ·��
    #if defined(_WIN32)
        tempFilePath = std::tmpnam(nullptr); // Windows��ʱ�ļ�
    #endif

        FILE* fd = nullptr;
    #if defined(_MSC_VER) || defined(__MINGW64__)
        fd = boost::nowide::fopen(tempFilePath.c_str(), "wb+");
    #elif defined(__GNUC__) && defined(_LARGEFILE64_SOURCE)
        fd = fopen64(tempFilePath.c_str(), "wb+");
    #else
        fd = fopen(tempFilePath.c_str(), "wb+");
    #endif

        if (fd == nullptr)
        {
            return -1;
        }

        // RAII�����ļ����
        struct FileGuard
        {
            FILE* fd;
            std::string filePath;
            ~FileGuard()
            {
                if (fd) fclose(fd);
                // ɾ����ʱ�ļ�
                remove(filePath.c_str());
            }
        } fileGuard{ fd, tempFilePath };

        //������д���ļ�
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fd);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);  // 连接超时5秒
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);       // 数据传输超时15秒

        // ͬ��ִ������
        CURLcode res = curl_easy_perform(curl);

        std::vector<std::string> FileInfoList;

        if (res == CURLE_OK)
        {
            // �����ļ�ָ�뵽��ͷ
            rewind(fd);

            //��ȡ�ļ����ݲ�����
            char buffer[1024];
            while (fgets(buffer, sizeof(buffer), fd))
            {
                std::string line(buffer);

                // �Ƴ����л��з��ͻس���
                line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
                line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());

                FileInfoList.push_back(line);
            }

            nlohmann::json commandJson;
            nlohmann::json dataJson;

            dataJson["address"] = strIp;
            if(FileInfoList.empty())
                dataJson["fileList"] = nlohmann::json::array();
            else
                dataJson["fileList"] = FileInfoList;
            commandJson["command"] = "get_file_List_from_lan_device";
            commandJson["data"] = dataJson;

            wxString strJS = wxString::Format("window.handleStudioCmd('%s');", RemotePrint::Utils::url_encode(commandJson.dump(-1, ' ', true)));
            wxGetApp().CallAfter([this, strJS] { 
                    if (this == nullptr || this->IsBeingDeleted())
                        return;
                    run_script(strJS.ToStdString()); 
                });
            //run_script(strJS.ToStdString());
        }
        return 0;
    });
    return 0;
}

int PrinterMgrView::deleteFileListFromLanDevice(const std::string strIp, const std::string strName)
{
    if (strIp.empty() || strName.empty())
    {
        return -1;
    }

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        return -1;
    }

    // RAII����CURL���
    struct CurlGuard
    {
        CURL* handle;
        ~CurlGuard()
        {
            if (handle) curl_easy_cleanup(handle);
        }
    } guard{ curl };

    std::string fullPath = "ftp://" + strIp + "/mmcblk0p1/creality/gztemp/";
    curl_easy_setopt(curl, CURLOPT_URL, fullPath.c_str());

    std::string deleteCmd = "DELE /mmcblk0p1/creality/gztemp/" + strName;
    struct curl_slist *CMDlist = nullptr;
    CMDlist = curl_slist_append(CMDlist, deleteCmd.c_str()); 
    curl_easy_setopt(curl, CURLOPT_POSTQUOTE, CMDlist);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);  // 连接超时5秒
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);       // 数据传输超时15秒

    // ͬ��ִ��FTPɾ������
    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(CMDlist);

    if (res != CURLE_OK)
    {
       
        return -1;
    }


    return 0;
}

int PrinterMgrView::uploadeFileLanDevice(const std::string strIp)
{
    wxString input_file;

    input_file.Clear();
    wxFileDialog dialog(this,
        _L("Choose files"),
        "", "",
        file_wildcards(FT_ONLY_GCODE), wxFD_OPEN);

    if (dialog.ShowModal() == wxID_OK)
        input_file = dialog.GetPath();
    else
        return 0;

    //�Ƿ�Ϊgcode�ļ���
    if (!is_gcode_file(into_u8(input_file)))
    {
        return -1;
    }

    {
        //�������
        nlohmann::json commandJson;
        commandJson["data"] = 30;
        commandJson["command"] = "uploade_file_oldPrinter_progress";
        wxString strJS = wxString::Format("window.handleStudioCmd('%s');", commandJson.dump());
        run_script(strJS.ToStdString());
    }

    fs::path path = into_u8(input_file);
    fs::path name = path.filename();

   float outProgress = 0;
    RemotePrint::RemotePrinterManager::getInstance().uploadFileByLan(strIp,name.string(),path.string(),
        [&outProgress](float progress,double speed) {
                outProgress  = progress;
            });

    if ((outProgress<101) && (outProgress>99))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

std::vector<std::string> PrinterMgrView::get_all_device_macs() const 
{
    std::vector<std::string> macs;
    for (const auto& pair : m_devicePool) {
        macs.push_back(pair.first);
    }
    return macs;
}

bool PrinterMgrView::should_upload_device_info() const
{
    return !m_finish_upload_device_state && get_all_device_macs().size() > 0;
}


} // GUI
} // Slic3r
