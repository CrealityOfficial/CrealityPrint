#include "MCPChatPanel.hpp"
#include "slic3r/GUI/simple/toolcalls/MCPToolCallsCommon.hpp"
#include "sendWorkflow/AISendWorkflowService.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "slic3r/GUI/NotificationManager.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/PartPlate.hpp"
#include "slic3r/GUI/GLCanvas3D.hpp"
#include "slic3r/GUI/WebModelLibraryView.hpp"
#include "slic3r/GUI/ModelDetailDialog.hpp"
#include "slic3r/GUI/I18N.hpp"
#include "slic3r/GUI/SystemId/SystemId.hpp"
#include "slic3r/GUI/Widgets/WebView.hpp"
#include "slic3r/GUI/print_manage/AppUtils.hpp"
#include "slic3r/GUI/print_manage/data/DataCenter.hpp"
#include "libslic3r/ModelInstance.hpp"
#include "buildinfo.h"

//#ifndef CXAGENT_DEFAULT_API_BASE
//#define CXAGENT_DEFAULT_API_BASE "http://127.0.0.1:8787"
//#endif
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/Utils.hpp"

#include <wx/uri.h>
#include <wx/utils.h>
#include <wx/image.h>
#include <wx/mstream.h>
#include <boost/algorithm/string.hpp>
#include <boost/log/trivial.hpp>
#include <boost/filesystem.hpp>
#include <boost/nowide/convert.hpp>
#include <boost/nowide/fstream.hpp>
#include <algorithm>
#include <initializer_list>
#include <cmath>
#include <ctime>
#include <unordered_set>
#include <vector>
#include <fstream>
#include <sstream>
#include <chrono>
#include <cstdlib>
#include "buildinfo.h"
#include <thread>
#include "cereal/external/base64.hpp"
#include "slic3r/Utils/Http.hpp"
using json = nlohmann::json;
namespace {
WorkflowToolbarState s_workflow_toolbar_state;
std::string thumbnail_to_data_url_256(const Slic3r::ThumbnailData& thumbnail)
{
    if (!thumbnail.is_valid())
        return {};

    wxImage image(thumbnail.width, thumbnail.height);
    image.InitAlpha();
    for (unsigned int row = 0; row < thumbnail.height; ++row) {
        const unsigned int flipped_row = (thumbnail.height - 1 - row) * thumbnail.width;
        for (unsigned int col = 0; col < thumbnail.width; ++col) {
            const unsigned char* px = thumbnail.pixels.data() + 4 * (flipped_row + col);
            image.SetRGB((int)col, (int)row, px[0], px[1], px[2]);
            image.SetAlpha((int)col, (int)row, px[3]);
        }
    }

    if (image.GetWidth() != 256 || image.GetHeight() != 256)
        image.Rescale(256, 256, wxIMAGE_QUALITY_HIGH);

    wxMemoryOutputStream mem_stream;
    if (!image.SaveFile(mem_stream, wxBITMAP_TYPE_PNG))
        return {};

    const size_t png_size = mem_stream.GetSize();
    std::vector<unsigned char> png_bytes(png_size);
    mem_stream.CopyTo(png_bytes.data(), png_size);

    const std::string encoded = cereal::base64::encode(png_bytes.data(), png_bytes.size());
    return "data:image/png;base64," + encoded;
}

json build_current_plate_preview_state()
{
    auto* plater = wxGetApp().plater();
    if (!plater || plater->only_gcode_mode())
        return json::object();

    auto& plate_list = plater->get_partplate_list();
    auto* plate = plate_list.get_curr_plate();
    if (!plate || plate->empty())
        return json::object();

    if (!plate->thumbnail_data.is_valid()) {
        auto* canvas = plater->get_view3D_canvas3D();
        if (canvas && canvas->make_current_for_postinit()) {
            const int plate_index = plate_list.get_curr_plate_index();
            const Slic3r::ThumbnailsParams thumbnail_params = {{}, false, true, true, true, plate_index};
            canvas->render_thumbnail(
                plate->thumbnail_data,
                (unsigned int)plate->plate_thumbnail_width,
                (unsigned int)plate->plate_thumbnail_height,
                thumbnail_params,
                Slic3r::GUI::Camera::EType::Ortho,
                false,
                false,
                false);
        }
    }

    const std::string preview_image = thumbnail_to_data_url_256(plate->thumbnail_data);
    if (preview_image.empty())
        return json::object();

    return {
        {"index", plate->get_index()},
        {"plate_index", plate_list.get_curr_plate_index()},
        {"preview_image", preview_image},
        {"thumbnail", preview_image}
    };
}
std::string normalize_dev_page_url(std::string url)
{
    url = boost::trim_copy(url);
    if (url.empty())
        return {};
    const std::string lower = boost::algorithm::to_lower_copy(url);
    if (!boost::starts_with(lower, "http://") && !boost::starts_with(lower, "https://"))
        url = "http://" + url;
    return url;
}
std::string resolve_chat_dev_server_url()
{
    if (const char* env_url = std::getenv("C3D_AICHATPAGE_URL")) {
        const std::string normalized = normalize_dev_page_url(env_url);
        if (!normalized.empty())
            return normalized;
    }
    auto* cfg = wxGetApp().app_config;
    if (!cfg)
        return {};
    const std::string config_url = normalize_dev_page_url(cfg->get("aichatpage_dev_url"));
    if (!config_url.empty())
        return config_url;
    const std::string config_port = boost::trim_copy(cfg->get("aichatpage_dev_port"));
    if (!config_port.empty())
        return "http://127.0.0.1:" + config_port;
    return {};
}
wxString resolve_chat_page_lang()
{
    auto* cfg = wxGetApp().app_config;
    wxString lang = (cfg && cfg->get("language") != "") ? cfg->get("language") : "";
    if (lang.empty())
        lang = wxGetApp().current_language_code_safe();
    if (lang.empty())
        lang = "en_GB";
    return lang;
}
std::string resolve_chat_page_region(const wxString& lang)
{
    auto* cfg = wxGetApp().app_config;
    std::string region = cfg ? cfg->get("region") : "";
    if (region.empty()) {
        if (lang == "zh_CN")
            region = "China";
        else
            region = "North America";
    }
    std::replace(region.begin(), region.end(), ' ', '_');
    return region;
}
wxString build_chat_page_url(const wxString& base_url)
{
    const wxString lang = resolve_chat_page_lang();
    const std::string region = resolve_chat_page_region(lang);
    // AIChatPage expects the edition as "ai" / "pro"; easy_print_mode == "1" means AI edition.
    const std::string version_mode = wxGetApp().easy_mode() ? "ai" : "pro";
    // Build channel (Dev / Alpha / Beta / Release) drives the tracking endpoint
    // the chat page reports to; mirror how other WebViews pass PROJECT_VERSION_EXTRA.
    std::string type = std::string(PROJECT_VERSION_EXTRA);
    type.erase(std::remove(type.begin(), type.end(), ' '), type.end());
    // os 供 AIChatPage 埋点的 operating_system 字段使用（与 SendToPrinter / DeviceMgr 等 WebView 一致）。
    const std::string os_description = std::string(wxGetOsDescription().ToUTF8().data());
    // device_id 供 AIChatPage 埋点使用，取宿主统一的系统 id（与 AnalyticsDataUploadManager 等一致）。
    const std::string device_id = Slic3r::GUI::SystemId::get_system_id();
    const std::string encoded_lang = Slic3r::Http::url_encode(std::string(lang.ToUTF8().data()));
    const std::string encoded_region = Slic3r::Http::url_encode(region);
    const std::string encoded_version = Slic3r::Http::url_encode(std::string(CREALITYPRINT_VERSION));
    const std::string encoded_version_mode = Slic3r::Http::url_encode(version_mode);
    const std::string encoded_type = Slic3r::Http::url_encode(type);
    const std::string encoded_os = Slic3r::Http::url_encode(os_description);
    const std::string encoded_device_id = Slic3r::Http::url_encode(device_id);
    const wxString separator = base_url.Find('?') == wxNOT_FOUND ? "?" : "&";
    return wxString::Format(
        "%s%slang=%s&region=%s&version=%s&version_mode=%s&type=%s&os=%s&device_id=%s",
        base_url,
        separator,
        wxString::FromUTF8(encoded_lang.c_str()),
        wxString::FromUTF8(encoded_region.c_str()),
        wxString::FromUTF8(encoded_version.c_str()),
        wxString::FromUTF8(encoded_version_mode.c_str()),
        wxString::FromUTF8(encoded_type.c_str()),
        wxString::FromUTF8(encoded_os.c_str()),
        wxString::FromUTF8(encoded_device_id.c_str())
    );
}

// 保存JSON调试数据到CP日志目录
void save_debug_json(const std::string& filename, const std::string& json_data)
{
    try {
        // 使用CP的日志目??? data_dir/log/
        boost::filesystem::path debug_dir;
#ifdef _WIN32
        debug_dir = boost::filesystem::path(boost::nowide::widen(Slic3r::data_dir())) / "log" / "debug_api";
#else
        debug_dir = boost::filesystem::path(Slic3r::data_dir()) / "log" / "debug_api";
#endif
        boost::filesystem::create_directories(debug_dir);
        
        std::string timestamp = std::to_string(std::time(nullptr));
        const boost::filesystem::path debug_file = debug_dir / (filename + "_" + timestamp + ".json");
#ifdef _WIN32
        std::string full_path = boost::nowide::narrow(debug_file.wstring());
#else
        std::string full_path = debug_file.string();
#endif
        
        boost::nowide::ofstream file(full_path);
        if (file.is_open()) {
            file << json_data;
            file.close();
            BOOST_LOG_TRIVIAL(info) << "[MCPChatPanel] Debug JSON saved to: " << full_path;
        }
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "[MCPChatPanel] Failed to save debug JSON: " << e.what();
    }
}

std::string normalize_sse_buffer(std::string value)
{
    boost::replace_all(value, "\r\n", "\n");
    boost::replace_all(value, "\r", "\n");
    return value;
}

bool try_parse_sse_event(std::string& buffer, std::string& event_name, std::string& event_data)
{
    const std::size_t separator = buffer.find("\n\n");
    if (separator == std::string::npos)
        return false;

    const std::string block = buffer.substr(0, separator);
    buffer.erase(0, separator + 2);

    event_name = "message";
    event_data.clear();
    std::istringstream stream(block);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (boost::starts_with(line, "event:")) {
            event_name = boost::trim_copy(line.substr(6));
            continue;
        }
        if (boost::starts_with(line, "data:")) {
            std::string value = line.substr(5);
            boost::trim_left(value);
            if (!event_data.empty())
                event_data += "\n";
            event_data += value;
        }
    }
    return true;
}

bool extract_blocking_slice_message(const json& state, std::string* out_message)
{
    if (!state.is_object())
        return false;

    const json notifications = state.value("ui_notifications", json::array());
    if (!notifications.is_array())
        return false;

    for (const auto& entry : notifications) {
        if (!entry.is_object())
            continue;

        const std::string level = entry.value("level", "");
        if (level != "error" && level != "important")
            continue;

        std::string text = entry.value("text", "");
        if (text.empty())
            text = entry.value("message", "");
        if (text.empty())
            text = entry.value("title", "");
        if (text.empty())
            text = "Slice failed.";

        if (out_message)
            *out_message = text;
        return true;
    }

    return false;
}

json build_ai_send_placeholder_event(
    const json& data,
    const std::string& stage,
    const std::string& message)
{
    return {
        {"version", "1.0"},
        {"request_id", data.value("request_id", std::string())},
        {"card_id", data.value("card_id", std::string())},
        {"timestamp_ms", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()},
        {"stage", stage},
        {"message", message},
        {"data", {
            {"stage", stage},
            {"message", message}
        }}
    };
}

} // namespace

namespace Slic3r {
namespace GUI {

namespace {
MCPChatPanel* get_floating_chat_panel();
}

MCPChatPanel::MCPChatPanel(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size)
    : wxPanel(parent, id, pos, size)
    , m_scene_update_timer(this)
    , m_scheduled_refresh_timer(this)
{
    const auto& user = wxGetApp().get_user();
    m_gateway_user_id = user.userId;
    m_gateway_user_token = user.token;

    InitWebView();
    RegisterAllHandlers();
    

    // Start 300ms auto-push timer
    Bind(wxEVT_TIMER, &MCPChatPanel::OnSceneUpdateTimer, this, m_scene_update_timer.GetId());
    auto* cfg = wxGetApp().app_config;
    if (cfg) {
        //m_cxagent_api_base = cfg->get("cxagent_api_base");
    }
    if (m_cxagent_api_base.empty()) {
        std::string region = cfg ? cfg->get("region") : std::string();
        m_cxagent_api_base = region == "China" ? "https://cxagent.crealitycloud.cn" : "https://cxagent.crealitycloud.com";
    }
    // #define _DEBUG1
    #ifdef _DEBUG1
        // DEV MODE: Load from Vite dev server for hot reload
        m_cxagent_api_base = "http://localhost:5175/";
    #endif
    LoadChatPage();
    m_cxagent_client_id = "c3dslicer-" + std::to_string(static_cast<long long>(std::time(nullptr)));
    std::weak_ptr<int> async_lifetime = m_async_lifetime;

    m_cxagent_bridge = std::make_unique<Bridge::CxAgentClientBridge>();
    m_cxagent_bridge->SetMessageHandler([this, async_lifetime](const json& msg) {
        if (async_lifetime.expired())
            return;
        wxGetApp().CallAfter([this, async_lifetime, msg]() {
            if (async_lifetime.expired())
                return;
            HandleCxAgentMessage(msg);
        });
    });
    m_cxagent_bridge->SetStatusHandler(
        [this, async_lifetime](const Bridge::CxAgentClientBridge::StatusSnapshot& status) {
            if (async_lifetime.expired())
                return;
            wxGetApp().CallAfter([this, async_lifetime, status]() {
                if (async_lifetime.expired())
                    return;
                HandleCxAgentStatusChanged(status);
            });
        }

    );

    m_sagent_mqtt_bridge = std::make_unique<Bridge::SAgentMqttBridge>();
    m_sagent_mqtt_bridge->SetMessageHandler([this, async_lifetime](const json& msg) {
        if (async_lifetime.expired())
            return;
        wxGetApp().CallAfter([this, async_lifetime, msg]() {
            if (async_lifetime.expired())
                return;
            const std::string message_kind = msg.value("message_kind", std::string());
            if (message_kind == "workflow_update") {
                SendCommandToJS("sagent_mqtt_workflow_update", msg);
                return;
            }
            if (ShouldUseSAgentMqttNativePath(msg)) {
                HandleSAgentMqttNativeToolRequest(msg);
                return;
            }
            SendCommandToJS("sagent_mqtt_tool_request", msg);
        });
    });
    m_sagent_mqtt_bridge->SetStatusHandler(
        [this, async_lifetime](const Bridge::SAgentMqttBridge::StatusSnapshot& status) {
            if (async_lifetime.expired())
                return;
            wxGetApp().CallAfter([this, async_lifetime, status]() {
                if (async_lifetime.expired())
                    return;
                HandleSAgentMqttStatusChanged(status);
            });
        }
    );

    m_ai_send_workflow = std::make_unique<AISendWorkflowService>();
    BindAISendWorkflowCallbacks();
    Bridge::SlicerBridge::Instance().SetSendToPrinterDelegate(
        this,
        [this](const json& params) { return ExecuteAISendToPrinterAction(params); });

    if (auto* plater = wxGetApp().plater())
    {
        plater->Bind(Slic3r::GUI::EVT_EXPORT_GCODE_FINISHED, &MCPChatPanel::OnExportFinished, this);
    }
}

MCPChatPanel::~MCPChatPanel()
{
    m_shutting_down = true;
    m_page_loaded = false;
    m_js_ready = false;
    // Stop the WebView before nulling the pointer so that any in-flight
    // navigation or script execution is cancelled.  DestroyAll() in
    // GUI_App::OnExit() will handle the msedgewebview2.exe child processes;
    // we must not call Destroy() here because the wx window tree owns the
    // lifetime of m_browser.
    if (m_browser) {
        m_browser->Stop();
    }
    m_browser = nullptr;
    m_async_lifetime.reset();

    Bridge::SlicerBridge::Instance().ClearSendToPrinterDelegate(this);
    UnregisterEmbeddedAIChatPanel(this);

    if (auto* plater = wxGetApp().plater())
    {
        plater->Unbind(Slic3r::GUI::EVT_EXPORT_GCODE_FINISHED, &MCPChatPanel::OnExportFinished, this);
    }
    m_scene_update_timer.Stop();
    m_scheduled_refresh_timer.Stop();
    if (m_sagent_mqtt_bridge) {
        m_sagent_mqtt_bridge->SetMessageHandler({});
        m_sagent_mqtt_bridge->SetStatusHandler({});
        m_sagent_mqtt_bridge->Stop();
    }
    if (m_cxagent_bridge) {
        m_cxagent_bridge->SetMessageHandler({});
        m_cxagent_bridge->SetStatusHandler({});
        m_cxagent_bridge->Stop();
    }
    m_commandHandlers.clear();

    // Clean up model detail dialog if still open
    if (m_model_detail_dlg) {
        m_model_detail_dlg->Destroy();
        m_model_detail_dlg = nullptr;
    }
}

// ---------------------------------------------------------------------------
// WebView initialization
// ---------------------------------------------------------------------------

void MCPChatPanel::InitWebView()
{
    ToolCalls::ClearWebViewLogFile();
    m_browser = WebView::CreateWebView(this, "");
    if (!m_browser) {
        BOOST_LOG_TRIVIAL(error) << "[MCPChatPanel] Failed to create wxWebView";
        return;
    }

    // User-Agent已在WebView::CreateWebView中设置，包含主题信息(dark/light)
    // 格式: Creality-Slicer/v{version} (dark/light) Mozilla/5.0 ...
    // 前端通过解析UA中的dark/light关键字来判断主题

    m_browser->Bind(wxEVT_DESTROY, &MCPChatPanel::OnBrowserDestroyed, this);
    m_browser->Bind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED, &MCPChatPanel::OnScriptMessage, this);
    // 拦截页面内导航：只允许停留在初始页面???origin，其他外部链接用系统浏览器打开
    m_browser->Bind(wxEVT_WEBVIEW_NAVIGATING, [this](wxWebViewEvent& evt) {
        const wxString url = evt.GetURL();
        // ???http/https（如 file://、about:blank、blob: 等）直接放行
        if (!url.StartsWith("http://") && !url.StartsWith("https://")) {
            return;
        }
        // 第一次导航（初始页面加载）：记录 origin 并放???
        if (m_chat_page_origin.IsEmpty()) {
            wxURI uri(url);
            m_chat_page_origin = uri.GetScheme() + "://" + uri.GetServer();
            const wxString port = uri.GetPort();
            if (!port.IsEmpty())
                m_chat_page_origin += ":" + port;
            return;
        }
        // ???origin 的导航放行（SPA 内部路由???
        if (url.StartsWith(m_chat_page_origin)) {
            return;
        }
        // 其他外部链接：用系统浏览器打开，阻???WebView 内嵌导航
        wxLaunchDefaultBrowser(url);
        evt.Veto();
    });
    // target="_blank" 新窗口请求也用系统浏览器打开
    m_browser->Bind(wxEVT_WEBVIEW_NEWWINDOW, [](wxWebViewEvent& evt) {
        const wxString url = evt.GetURL();
        if (!url.IsEmpty())
            wxLaunchDefaultBrowser(url);
        evt.Veto();
    });
    m_browser->Bind(wxEVT_WEBVIEW_LOADED, &MCPChatPanel::OnNavigationComplete, this);
    m_browser->Bind(wxEVT_WEBVIEW_ERROR, &MCPChatPanel::OnError, this);
    m_browser->EnableAccessToDevTools();

    // Bind scheduled refresh timer
    Bind(wxEVT_TIMER, &MCPChatPanel::OnScheduledRefreshTimer, this, m_scheduled_refresh_timer.GetId());

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_browser, 1, wxEXPAND);
    SetSizer(sizer);
    Layout();
}

void MCPChatPanel::OnBrowserDestroyed(wxWindowDestroyEvent& evt)
{
    if (evt.GetEventObject() == m_browser) {
        m_browser = nullptr;
        m_page_loaded = false;
        m_js_ready = false;
    }
    evt.Skip();
}

void MCPChatPanel::LoadChatPage()
{
    if (!m_browser) return;
    m_page_loaded = false;
    m_js_ready = false;
    m_chat_page_origin.Clear();

    wxString url = build_chat_page_url(m_cxagent_api_base);
    BOOST_LOG_TRIVIAL(info) << "[MCPChatPanel] Loading chat page: "
                            << url.ToUTF8().data();
    CallAfter([this, url]() {
        if (!m_browser) return;
        m_browser->LoadURL(url);
        m_browser->EnableAccessToDevTools();
    });

}

// ---------------------------------------------------------------------------
// JS Bridge: incoming messages from WebView
// ---------------------------------------------------------------------------

void MCPChatPanel::OnScriptMessage(wxWebViewEvent& evt)
{
    wxString strInput = evt.GetString();
    BOOST_LOG_TRIVIAL(trace) << "[MCPChatPanel] OnScriptMessage: " << strInput.ToUTF8().data();

    const std::string utf8_input = std::string(strInput.ToUTF8().data());
    json j = json::parse(utf8_input, nullptr, false);
    if (j.is_discarded() || !j.is_object()) {
        BOOST_LOG_TRIVIAL(error) << "[MCPChatPanel] Invalid script message payload: " << utf8_input;
        SendCommandToJS("error", {{"message", "Failed to parse JS bridge message: payload is not a JSON object"}});
        return;
    }

    try {
        std::string command = j.value("command", "");
        json data = j.value("data", json::object());

        if (command.empty() && j.value("action", std::string()) == "toNative" &&
            j.contains("message") && j["message"].is_object()) {
            OpenExternalBrowserFromJS(j["message"]);
            return;
        }

        BOOST_LOG_TRIVIAL(info)
            << "[MCPChatPanel] JS bridge command=" << command
            << " request_id=" << data.value("request_id", std::string())
            << " keyword=" << data.value("keyword", std::string())
            << " payload=" << data.dump();

        if (command.empty()) {
            BOOST_LOG_TRIVIAL(warning) << "[MCPChatPanel] Received message with no command";
            return;
        }

        auto it = m_commandHandlers.find(command);
        if (it != m_commandHandlers.end()) {
            BOOST_LOG_TRIVIAL(info) << "[MCPChatPanel] JS bridge DISPATCH: command=" << command << " payload=" << data.dump();
            it->second(data);
        } else {
            BOOST_LOG_TRIVIAL(warning) << "[MCPChatPanel] Unknown command: " << command;
            SendCommandToJS("error", {{"message", "Unknown command from chat.js: " + command}});
        }
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "[MCPChatPanel] Error handling script message: " << e.what();
        SendCommandToJS("error", {{"message", std::string("Failed to handle JS bridge message: ") + e.what()}});
    }
}

void MCPChatPanel::HandleOpenDeviceList(const nlohmann::json& data)
{
    bool ok = false;
    if (auto* plater = wxGetApp().plater()) {
        if (auto* canvas = plater->get_view3D_canvas3D()) {
            canvas->open_device_list_popup();
            ok = true;
        }
    }
    BOOST_LOG_TRIVIAL(info) << "[MCPChatPanel] open_device_list handled success=" << ok;
    SendCommandToJS("open_device_list_result",
                    {{"success", ok}, {"request_id", data.value("request_id", std::string())}});
}

void MCPChatPanel::OnNavigationComplete(wxWebViewEvent& evt)
{
    BOOST_LOG_TRIVIAL(info) << "[MCPChatPanel] Page loaded: " << evt.GetURL().ToUTF8().data();
    m_page_loaded = true;

    // Sync the current app theme on first load so the web card does not stay in light mode.
    NotifyThemeChanged();

    // Send current slicer state
    ExecuteBridgeAction(Bridge::ActionID::GET_SLICER_STATE, json::object());

    // Send available action list to front-end
    auto& bridge = Bridge::SlicerBridge::Instance();
    auto list = bridge.GetActionListJSON();
    SendCommandToJS("action_list", list);
	SendCommandToJS("available_tools", bridge.GetAvailableToolsJSON());
    NotifyCxAgentStatus();
    
}
void MCPChatPanel::OnError(wxWebViewEvent& evt)
{
    BOOST_LOG_TRIVIAL(error) << "[MCPChatPanel] WebView error: " << evt.GetString().ToUTF8().data();
}

void MCPChatPanel::OpenExternalBrowserFromJS(const json& data)
{
    auto read_url = [&data](std::initializer_list<const char*> keys) -> std::string {
        for (const char* key : keys) {
            if (data.contains(key) && data[key].is_string()) {
                std::string value = data[key].get<std::string>();
                boost::algorithm::trim(value);
                if (!value.empty())
                    return value;
            }
        }
        return {};
    };

    const std::string request_id = data.value("request_id", data.value("requestId", std::string()));
    const std::string url = read_url({"url", "href", "callback", "link"});
    const std::string lower_url = boost::algorithm::to_lower_copy(url);
    json result = {
        {"request_id", request_id},
        {"url", url},
        {"success", false}
    };

    if (url.empty()) {
        result["message"] = "Missing URL for external browser.";
        SendCommandToJS("open_external_browser_result", result);
        return;
    }

    if (!boost::starts_with(lower_url, "http://") && !boost::starts_with(lower_url, "https://")) {
        result["message"] = "Only http and https URLs can be opened in the external browser.";
        SendCommandToJS("open_external_browser_result", result);
        return;
    }

    BOOST_LOG_TRIVIAL(info) << "[MCPChatPanel] Opening URL in external browser: " << url;
    const bool opened = wxLaunchDefaultBrowser(wxString::FromUTF8(url.c_str()));
    result["success"] = opened;
    if (!opened)
        result["message"] = "Failed to open external browser.";
    SendCommandToJS("open_external_browser_result", result);
}
// ---------------------------------------------------------------------------
// Timer: auto-push scene state to JS every 300ms
// ---------------------------------------------------------------------------

void MCPChatPanel::OnSceneUpdateTimer(wxTimerEvent& /*evt*/)
{
    if (!m_js_ready) return;
    auto& bridge = Bridge::SlicerBridge::Instance();
    json result = bridge.Execute(Bridge::ActionID::GET_SLICER_STATE, json::object());
    SendCommandToJS("slicer_state", result);
    if (result.value("success", false)) {
        if (m_ai_send_workflow)
            m_ai_send_workflow->RefreshActiveSnapshots();

        if (result.contains("state") && result["state"].contains("ui_notifications")) {
            SendCommandToJS("scene_warnings", result["state"]["ui_notifications"]);
        } else {
            SendCommandToJS("scene_warnings", json::array());
        }

        if (m_cxagent_bridge && m_cxagent_bridge->GetStatus().connected) {
            json context_update = {
                {"project_context", result.value("state", json::object())}
            };

            json edited_config = bridge.Execute(Bridge::ActionID::GET_EDITED_CONFIG, json::object());
            if (edited_config.value("success", false)) {
                json current_slice_params = json::object();
                if (edited_config.contains("config") && edited_config["config"].is_object())
                    current_slice_params = edited_config["config"];
                else {
                    if (edited_config.contains("print"))
                        current_slice_params["print"] = edited_config["print"];
                    if (edited_config.contains("filament"))
                        current_slice_params["filament"] = edited_config["filament"];
                    if (edited_config.contains("printer"))
                        current_slice_params["printer"] = edited_config["printer"];
                }
                context_update["current_slice_params"] = current_slice_params;
            }

            const json state = result.value("state", json::object());
            context_update["facts"] = ToolCalls::BuildExplicitFactsFromState(state);
            context_update["scene"] = {
                {"blocking_errors", ToolCalls::BuildBlockingErrorsPayload(state)},
            };
            context_update["geometry_analysis"] = ToolCalls::BuildGeometryAnalysisFromState(state);
            context_update["visual_geometry"] = ToolCalls::BuildVisualRecommendationGeometryFromState(state);

            auto* plater = wxGetApp().plater();
            if (plater) {
                auto& plate_list = plater->get_partplate_list();
                auto* current_plate = plate_list.get_curr_plate();
                auto* current_result = plate_list.get_current_slice_result();
                if (current_plate && current_result && current_plate->is_slice_result_valid())
                    context_update["slice_result"] = BuildCompletedSliceResult();
            }

            m_cxagent_bridge->SendContextUpdate(context_update);
        }

        if (m_pending_slice_request.active &&
            !m_pending_slice_request.awaiting_export &&
            !m_pending_slice_request.request_id.empty()) {
            const std::string request_id = m_pending_slice_request.request_id;
            const bool notify_cxagent_bridge = m_pending_slice_request.notify_cxagent_bridge && m_cxagent_bridge;
            const bool notify_sagent_mqtt_bridge = m_pending_slice_request.notify_sagent_mqtt_bridge && m_sagent_mqtt_bridge;
            auto& observed = m_observed_slice_requests;
            auto* plater = wxGetApp().plater();
            if (plater && plater->is_background_process_slicing()) {
                observed.insert(request_id);
            } else if (observed.count(request_id) > 0) {
                const json state = result.value("state", json::object());
                const json current_plate = state.value("current_plate", json::object());
                const bool slice_result_valid = current_plate.value("slice_result_valid", false);

                if (slice_result_valid) {
                    if (plater &&
                        !m_pending_slice_request.output_path.empty() &&
                        m_pending_slice_request.export_strategy != "slice_only") {
                        const bool export_started = plater->export_gcode_to_path(
                            boost::filesystem::path(m_pending_slice_request.output_path), false);
                        if (!export_started) {
                            const json error = {
                                {"code", "EXPORT_START_FAILED"},
                                {"message", "Slice completed but automatic G-code export could not be started."}
                            };
                            if (notify_cxagent_bridge) {
                                m_cxagent_bridge->SendToolResult(request_id, false, error);
                                m_cxagent_bridge->MarkRequestFinished(request_id);
                            }
                            if (notify_sagent_mqtt_bridge)
                                PublishSAgentMqttToolResult(request_id, false, json::object(), error);
                            SendCommandToJS("slice_completed", {
                                {"request_id", request_id},
                                {"success", false},
                                {"cancelled", false},
                                {"error", true},
                                {"message", "Slice completed but automatic G-code export could not be started."}
                            });
                            m_pending_slice_request = {};
                            observed.erase(request_id);
                            NotifyCxAgentStatus();
                        } else {
                            m_pending_slice_request.awaiting_export = true;
                            if (notify_cxagent_bridge)
                                m_cxagent_bridge->SendToolProgress(request_id, 90, "Slice completed, exporting G-code", "exporting");
                            if (notify_sagent_mqtt_bridge)
                                PublishSAgentMqttToolProgress(
                                    request_id,
                                    90,
                                    "exporting",
                                    "Slice completed, exporting G-code",
                                    "running",
                                    {{"tool", Bridge::ActionID::START_SLICE}, {"source_action", Bridge::ActionID::START_SLICE}});
                            NotifyCxAgentStatus();
                        }
                    } else {
                        const json slice_result = BuildCompletedSliceResult();
                        if (notify_cxagent_bridge) {
                            m_cxagent_bridge->SendToolProgress(request_id, 100, "Slice completed", "completed", "completed");
                            m_cxagent_bridge->SendToolResult(request_id, true, slice_result);
                            m_cxagent_bridge->MarkRequestFinished(request_id);
                        }
                        if (notify_sagent_mqtt_bridge) {
                            PublishSAgentMqttToolProgress(
                                request_id,
                                100,
                                "completed",
                                "Slice completed",
                                "completed",
                                {{"tool", Bridge::ActionID::START_SLICE}, {"source_action", Bridge::ActionID::START_SLICE}});
                            PublishSAgentMqttToolResult(request_id, true, slice_result, json::object());
                        }
                        SendCommandToJS("slice_completed", {
                            {"request_id", request_id},
                            {"success", true},
                            {"cancelled", false},
                            {"error", false},
                            {"message", "Slice completed"},
                            {"result", slice_result}
                        });
                        m_pending_slice_request = {};
                        observed.erase(request_id);
                        NotifyCxAgentStatus();
                    }
                } else {
                    std::string message;
                    if (extract_blocking_slice_message(state, &message)) {
                        const json error = {
                            {"code", "SLICE_PROCESS_FAILED"},
                            {"message", message}
                        };
                        if (notify_cxagent_bridge) {
                            m_cxagent_bridge->SendToolResult(request_id, false, error);
                            m_cxagent_bridge->MarkRequestFinished(request_id);
                        }
                        if (notify_sagent_mqtt_bridge)
                            PublishSAgentMqttToolResult(request_id, false, json::object(), error);
                        SendCommandToJS("slice_completed", {
                            {"request_id", request_id},
                            {"success", false},
                            {"cancelled", false},
                            {"error", true},
                            {"message", message}
                        });
                        m_pending_slice_request = {};
                        observed.erase(request_id);
                        NotifyCxAgentStatus();
                    } else {
                        // ????????????????????????????
                        BOOST_LOG_TRIVIAL(warning)
                            << "[MCPChatPanel] slice completed but slice_result_valid=false, no error message found";
                        const json warning_result = {
                            {"code", "SLICE_COMPLETED_INVALID_RESULT"},
                            {"message", "Slice completed but result is not valid for print"},
                            {"await_context_update", true}
                        };
                        if (notify_cxagent_bridge) {
                            m_cxagent_bridge->SendToolResult(request_id, true, warning_result);
                            m_cxagent_bridge->MarkRequestFinished(request_id);
                        }
                        if (notify_sagent_mqtt_bridge)
                            PublishSAgentMqttToolResult(request_id, true, warning_result, json::object());
                        SendCommandToJS("slice_completed", {
                            {"request_id", request_id},
                            {"success", true},
                            {"cancelled", false},
                            {"error", false},
                            {"message", "Slice completed with warnings"}
                        });
                        m_pending_slice_request = {};
                        observed.erase(request_id);
                        NotifyCxAgentStatus();
                    }
                }
            }
        }
    }

}

void MCPChatPanel::HandleCxAgentMessage(const json& msg)
{
    const std::string type = msg.value("type", "");
    if (type == "planner_update") {
        BOOST_LOG_TRIVIAL(warning)
            << "[MCPChatPanel] planner_update received task_id="
            << msg.value("task_id", std::string())
            << " tool=" << msg.value("planner_tool_name", std::string())
            << " reason=" << msg.value("planner_reason_code", std::string());
    }
    if (type == "tool_call") {
        HandleCxAgentToolCall(msg);
        return;
    }
    if (type == "cancel_call") {
        HandleCxAgentCancelCall(msg);
        return;
    }
    if (type == "ack") {
        SendCommandToJS("cxagent_ack", msg);
        return;
    }

    SendCommandToJS("cxagent_message", msg);
}


void MCPChatPanel::HandleCxAgentCancelCall(const json& msg)
{
    if (!m_cxagent_bridge)
        return;

    const std::string request_id = msg.value("request_id", "");
    if (request_id.empty())
        return;

    SendCommandToJS("cxagent_cancel_call", msg);

    auto ai_send_it = m_pending_ai_send_calls_by_request.find(request_id);
    if (ai_send_it != m_pending_ai_send_calls_by_request.end()) {
        if (!m_ai_send_workflow || !m_ai_send_workflow->Cancel(ai_send_it->second.card_id)) {
            ClearAISendToolCallSilent(request_id, ai_send_it->second.card_id);
            m_cxagent_bridge->SendToolResult(request_id, false, {
                {"code", "AI_SEND_CANCEL_FAILED"},
                {"message", "Failed to cancel active AI send workflow."}
            });
            m_cxagent_bridge->MarkRequestFinished(request_id);
            m_ai_send_request_by_card.erase(ai_send_it->second.card_id);
            m_pending_ai_send_calls_by_request.erase(ai_send_it);
            NotifyCxAgentStatus();
        }
        return;
    }

    if (!m_pending_slice_request.active || m_pending_slice_request.request_id != request_id) {
        m_cxagent_bridge->SendToolResult(request_id, false, {
            {"code", "REQUEST_NOT_ACTIVE"},
            {"message", "No active slice request matches the cancel request."}
        });
        m_cxagent_bridge->MarkRequestFinished(request_id);
        NotifyCxAgentStatus();
        return;
    }

    auto* plater = wxGetApp().plater();
    if (!plater) {
        m_cxagent_bridge->SendToolResult(request_id, false, {
            {"code", "PLATER_NOT_AVAILABLE"},
            {"message", "Plater is not available for canceling the slice."}
        });
        m_cxagent_bridge->MarkRequestFinished(request_id);
        m_pending_slice_request = {};
        m_observed_slice_requests.erase(request_id);
        NotifyCxAgentStatus();
        return;
    }

    if (!plater->cancel_background_slicing()) {
        m_cxagent_bridge->SendToolResult(request_id, false, {
            {"code", "SLICE_CANCEL_FAILED"},
            {"message", "No running background slicing task could be canceled."}
        });
        m_cxagent_bridge->MarkRequestFinished(request_id);
        m_pending_slice_request = {};
        m_observed_slice_requests.erase(request_id);
        NotifyCxAgentStatus();
        return;
    }

    m_cxagent_bridge->SendToolProgress(request_id, 25, "Canceling slice", "canceling");
    NotifyCxAgentStatus();
}

void MCPChatPanel::BindAISendWorkflowCallbacks()
{
    if (!m_ai_send_workflow)
        return;

    m_ai_send_workflow->SetSnapshotCallback([this](const json& envelope) {
        wxGetApp().CallAfter([this, envelope]() { OnAISendSnapshot(envelope); });
    });
    m_ai_send_workflow->SetProgressCallback([this](const json& envelope) {
        wxGetApp().CallAfter([this, envelope]() { OnAISendProgress(envelope); });
    });
    m_ai_send_workflow->SetResultCallback([this](const json& envelope) {
        wxGetApp().CallAfter([this, envelope]() { OnAISendResult(envelope); });
    });
    m_ai_send_workflow->SetErrorCallback([this](const json& envelope) {
        wxGetApp().CallAfter([this, envelope]() { OnAISendError(envelope); });
    });
}

void MCPChatPanel::ReplayAISendCardsToJS()
{
    if (!m_ai_send_workflow)
        return;

    for (const auto& snapshot : m_ai_send_workflow->GetActiveSnapshots())
        OnAISendSnapshot(snapshot);
}

json MCPChatPanel::ExecuteAISendToPrinterAction(const json& data)
{
    if (!m_ai_send_workflow) {
        return {
            {"success", false},
            {"code", "AI_SEND_WORKFLOW_UNAVAILABLE"},
            {"message", "AI send workflow service is not available."}
        };
    }

    json open_args = data.is_object() ? data : json::object();
    std::string request_id = open_args.value("request_id", std::string());
    if (request_id.empty()) {
        const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        request_id = "bridge-send-" + std::to_string(static_cast<long long>(now_ms));
        open_args["request_id"] = request_id;
    }

    if (!open_args.contains("entry_mode"))
        open_args["entry_mode"] = "send_workflow";

    const bool direct_start_print = open_args.value("direct_start_print", true);
    const auto open_result = m_ai_send_workflow->OpenCard(request_id, open_args);
    ToolCalls::LogAISendPanelStage(
        open_result.success ? "bridge_send_open_card_success" : "bridge_send_open_card_failed",
        open_result.card_id,
        request_id,
        open_result.success
            ? (std::string("open_args=") + ToolCalls::SafeJsonDumpForLog(open_args))
            : (std::string("code=") + open_result.code + ", message=" + open_result.message));

    if (!open_result.success) {
        return {
            {"success", false},
            {"code", open_result.code.empty() ? std::string("AI_SEND_CARD_OPEN_FAILED") : open_result.code},
            {"message", open_result.message.empty() ? std::string("Failed to open AI send workflow.") : open_result.message}
        };
    }

    if (direct_start_print && !m_ai_send_workflow->StartSendAndPrint(open_result.card_id)) {
        return {
            {"success", false},
            {"code", "AI_SEND_DIRECT_START_FAILED"},
            {"message", into_u8(_L("start print failed."))},
            {"request_id", request_id},
            {"card_id", open_result.card_id}
        };
    }

    return {
        {"success", true},
        {"message", direct_start_print ? into_u8(_L("print started")) : into_u8(_L("Uploading"))},
        {"request_id", request_id},
        {"card_id", open_result.card_id},
        {"action", Bridge::ActionID::SEND_TO_PRINTER},
        {"source_action", Bridge::ActionID::SEND_TO_PRINTER}
    };
}

void MCPChatPanel::HandleAISendCardOpen(const json& data)
{
    BOOST_LOG_TRIVIAL(info) << "[MCPChatPanel] ai_send_card_open payload=" << data.dump();
    if (!m_ai_send_workflow) {
        OnAISendError(build_ai_send_placeholder_event(data, "open", "AI send workflow service is not available."));
        return;
    }

    const std::string request_id = data.value("request_id", std::string());
    json open_args = data;
    if (!open_args.contains("entry_mode"))
        open_args["entry_mode"] = "mapping_only";

    const auto result = m_ai_send_workflow->OpenCard(request_id, open_args);
    if (!result.success) {
        json error = build_ai_send_placeholder_event(
            {{"request_id", request_id}, {"card_id", result.card_id}},
            "open",
            result.message.empty() ? std::string("Failed to open AI send card.") : result.message);
        if (!result.code.empty())
            error["code"] = result.code;
        OnAISendError(error);
    }
}

void MCPChatPanel::HandleAISendCardSelectPlate(const json& data)
{
    BOOST_LOG_TRIVIAL(info) << "[MCPChatPanel] ai_send_card_select_plate payload=" << data.dump();
    const std::string card_id = ResolveAISendCardId(data);
    int plate_index = data.value("plate_index", -1);
    if (plate_index < 0 && data.contains("selected_plate_index"))
        plate_index = data.value("selected_plate_index", -1);
    if (plate_index < 0 && data.contains("plate_number"))
        plate_index = data.value("plate_number", 0) - 1;
    if (plate_index < 0 && data.contains("plateNumber"))
        plate_index = data.value("plateNumber", 0) - 1;

    if (!m_ai_send_workflow || !m_ai_send_workflow->SelectPlate(card_id, plate_index)) {
        json error = build_ai_send_placeholder_event(data, "select_plate", "Failed to switch AI send card plate.");
        error["finish_tool_call"] = false;
        error["data"]["finish_tool_call"] = false;
        OnAISendError(error);
    }
}

void MCPChatPanel::HandleAISendCardAutoMatch(const json& data)
{
    BOOST_LOG_TRIVIAL(info) << "[MCPChatPanel] ai_send_card_auto_match payload=" << data.dump();
    const std::string card_id = ResolveAISendCardId(data);
    if (!m_ai_send_workflow || !m_ai_send_workflow->AutoMatch(card_id)) {
        json error = build_ai_send_placeholder_event(data, "auto_match", "Failed to auto match consumables for AI send card.");
        error["finish_tool_call"] = false;
        error["data"]["finish_tool_call"] = false;
        OnAISendError(error);
    }
}

void MCPChatPanel::HandleAISendCardUpdateMapping(const json& data)
{
    BOOST_LOG_TRIVIAL(info) << "[MCPChatPanel] ai_send_card_update_mapping payload=" << data.dump();
    const std::string card_id = ResolveAISendCardId(data);
    if (!m_ai_send_workflow || !m_ai_send_workflow->UpdateMapping(card_id, data)) {
        json error = build_ai_send_placeholder_event(data, "update_mapping", "Failed to update consumable mapping for AI send card.");
        error["finish_tool_call"] = false;
        error["data"]["finish_tool_call"] = false;
        OnAISendError(error);
    }
}

void MCPChatPanel::HandleAISendCardApplyMapping(const json& data)
{
    BOOST_LOG_TRIVIAL(info) << "[MCPChatPanel] ai_send_card_apply_mapping payload=" << data.dump();
    const std::string card_id = ResolveAISendCardId(data);
    if (!m_ai_send_workflow || !m_ai_send_workflow->ApplyMapping(card_id)) {
        json error = build_ai_send_placeholder_event(data, "apply_mapping", "Failed to apply consumable mapping for AI send card.");
        error["finish_tool_call"] = false;
        error["data"]["finish_tool_call"] = false;
        OnAISendError(error);
    }
}

void MCPChatPanel::HandleAISendApplyProcessIntent(const json& data)
{
    BOOST_LOG_TRIVIAL(info) << "[MCPChatPanel] ai_send_apply_process_intent payload=" << data.dump();
    const std::string card_id = ResolveAISendCardId(data);
    std::string intent_key = data.value("intent", std::string());
    if (intent_key.empty())
        intent_key = data.value("intent_key", std::string());

    if (!m_ai_send_workflow || !m_ai_send_workflow->ApplyProcessIntent(card_id, intent_key)) {
        json error = build_ai_send_placeholder_event(data, "apply_process_intent", "Failed to apply process intent for AI send card.");
        error["finish_tool_call"] = false;
        error["data"]["finish_tool_call"] = false;
        OnAISendError(error);
    }
}

void MCPChatPanel::HandleAISendCardSendOnly(const json& data)
{
    BOOST_LOG_TRIVIAL(info) << "[MCPChatPanel] ai_send_card_send_only payload=" << data.dump();
    const std::string card_id = ResolveAISendCardId(data);
    if (!m_ai_send_workflow || !m_ai_send_workflow->StartSendOnly(card_id)) {
        OnAISendError(build_ai_send_placeholder_event(data, "send_only", "Failed to start AI send-only workflow."));
        return;
    }

    auto request_it = m_ai_send_request_by_card.find(card_id);
    if (request_it != m_ai_send_request_by_card.end()) {
        auto pending_it = m_pending_ai_send_calls_by_request.find(request_it->second);
        if (pending_it != m_pending_ai_send_calls_by_request.end())
            pending_it->second.waiting_user_action = false;
    }
}

void MCPChatPanel::HandleAISendCardStartPrint(const json& data)
{
    BOOST_LOG_TRIVIAL(info) << "[MCPChatPanel] ai_send_card_start_print payload=" << data.dump();
    const std::string card_id = ResolveAISendCardId(data);
    if (!m_ai_send_workflow || !m_ai_send_workflow->StartSendAndPrint(card_id)) {
        OnAISendError(build_ai_send_placeholder_event(data, "start_print", "Failed to start AI send-and-print workflow."));
        return;
    }

    auto request_it = m_ai_send_request_by_card.find(card_id);
    if (request_it != m_ai_send_request_by_card.end()) {
        auto pending_it = m_pending_ai_send_calls_by_request.find(request_it->second);
        if (pending_it != m_pending_ai_send_calls_by_request.end())
            pending_it->second.waiting_user_action = false;
    }
}

void MCPChatPanel::HandleAISendCardCancel(const json& data)
{
    BOOST_LOG_TRIVIAL(info) << "[MCPChatPanel] ai_send_card_cancel payload=" << data.dump();
    const std::string card_id = ResolveAISendCardId(data);
    if (!m_ai_send_workflow || !m_ai_send_workflow->Cancel(card_id)) {
        json error = build_ai_send_placeholder_event(data, "cancel", "Failed to cancel AI send workflow.");
        error["finish_tool_call"] = false;
        error["data"]["finish_tool_call"] = false;
        OnAISendError(error);
    }
}

void MCPChatPanel::HandleAISendCardRetry(const json& data)
{
    BOOST_LOG_TRIVIAL(info) << "[MCPChatPanel] ai_send_card_retry payload=" << data.dump();
    json error = build_ai_send_placeholder_event(data, "retry", "Retry is not implemented in this build.");
    error["finish_tool_call"] = false;
    error["data"]["finish_tool_call"] = false;
    OnAISendError(error);
}

void MCPChatPanel::OnAISendSnapshot(const json& envelope)
{
    const std::string card_id = envelope.value("card_id", std::string());
    const std::string request_id = envelope.value("request_id", std::string());
    const json data = envelope.value("data", json::object());
    ToolCalls::LogAISendPanelStage(
        "snapshot",
        card_id,
        request_id,
        std::string("status=") + data.value("status", std::string()) +
        ", status_text=" + data.value("status_text", std::string()));
    if (!ShouldSuppressAISendCardEvent(request_id, card_id))
        SendAgentEvent("ai_send_card_snapshot", envelope);
}

void MCPChatPanel::OnAISendProgress(const json& envelope)
{
    const std::string card_id = envelope.value("card_id", std::string());
    const std::string request_id_for_log = envelope.value("request_id", std::string());
    ToolCalls::LogAISendPanelStage(
        "progress",
        card_id,
        request_id_for_log,
        std::string("progress=") + std::to_string(envelope.value("progress", 0)) +
        ", stage=" + envelope.value("stage", std::string()) +
        ", message=" + envelope.value("message", std::string()));
    if (!ShouldSuppressAISendCardEvent(request_id_for_log, card_id))
        SendAgentEvent("ai_send_card_progress", envelope);
    if (!m_cxagent_bridge && !m_sagent_mqtt_bridge)
        return;

    auto request_it = m_ai_send_request_by_card.find(card_id);
    if (request_it == m_ai_send_request_by_card.end())
        return;

    const std::string request_id = request_it->second;
    auto pending_it = m_pending_ai_send_calls_by_request.find(request_id);
    const bool is_mqtt_native = pending_it != m_pending_ai_send_calls_by_request.end() && pending_it->second.native_mqtt_request;

    if (is_mqtt_native && m_sagent_mqtt_bridge) {
        const std::string workflow_id = pending_it->second.workflow_id;
        json progress_payload = {
            {"request_id", request_id},
            {"status", envelope.value("state", std::string("running"))},
            {"stage", envelope.value("stage", std::string("running"))},
            {"progress", envelope.value("progress", 0)},
            {"message", envelope.value("message", std::string("Processing AI send workflow"))},
            {"data", {{"tool", "send_print"}, {"card_id", card_id}}}
        };
        if (!workflow_id.empty())
            progress_payload["workflow_id"] = workflow_id;
        m_sagent_mqtt_bridge->PublishToolResponse("progress", progress_payload);
    } else if (m_cxagent_bridge) {
        m_cxagent_bridge->SendToolProgress(
            request_id,
            envelope.value("progress", 0),
            envelope.value("message", std::string("Processing AI send workflow")),
            envelope.value("stage", std::string("running")),
            envelope.value("state", std::string("running")));
    }
    NotifyCxAgentStatus();
}

void MCPChatPanel::OnAISendResult(const json& envelope)
{
    const std::string card_id = envelope.value("card_id", std::string());
    const std::string request_id = envelope.value("request_id", std::string());
    ToolCalls::LogAISendPanelStage(
        "result",
        card_id,
        request_id,
        std::string("result_type=") + envelope.value("result_type", std::string()) +
        ", message=" + envelope.value("message", std::string()));
    // AI-driven "direct start print" marks the send request silent so the
    // intermediate send-card UI is not rendered (the tool result flows to
    // CxAgent over MQTT, which owns the workflow cards). However the
    // "print_started" result is the signal the AIChatPage print-progress
    // monitor needs to start tracking the device in real time — exactly like a
    // LAN send does. Let this terminal "print_started" event through even when
    // the request is otherwise silent, so cloud print progress can update live.
    const std::string result_type = envelope.value("result_type", std::string());
    const bool is_print_started_result = result_type == "print_started";
    if (is_print_started_result || !ShouldSuppressAISendCardEvent(request_id, card_id))
        SendAgentEvent("ai_send_card_result", envelope);
    if (card_id.empty())
        return;

    json payload = {
        {"result_type", envelope.value("result_type", std::string())},
        {"message", envelope.value("message", std::string())},
        {"details", envelope.value("details", json::object())}
    };
    if (payload.value("result_type", std::string()) == "canceled")
        FinishAISendToolCallCanceled(card_id, payload);
    else
        FinishAISendToolCallSuccess(card_id, payload);
}

void MCPChatPanel::OnAISendError(const json& envelope)
{
    const std::string card_id = envelope.value("card_id", std::string());
    const std::string request_id = envelope.value("request_id", std::string());
    ToolCalls::LogAISendPanelStage(
        "error",
        card_id,
        request_id,
        std::string("code=") + envelope.value("code", std::string()) +
        ", message=" + envelope.value("message", std::string()) +
        ", finish_tool_call=" + (envelope.value("finish_tool_call", true) ? "true" : "false"));
    if (!ShouldSuppressAISendCardEvent(request_id, card_id))
        SendAgentEvent("ai_send_card_error", envelope);
    const bool finish_tool_call = envelope.value("finish_tool_call", true);
    if (!finish_tool_call)
        return;

    if (card_id.empty())
        return;

    json payload = {
        {"code", envelope.value("code", std::string("AI_SEND_WORKFLOW_FAILED"))},
        {"message", envelope.value("message", std::string("AI send workflow failed."))},
        {"details", envelope.value("details", json::object())}
    };
    FinishAISendToolCallFailure(card_id, payload);
}

void MCPChatPanel::FinishAISendToolCallSuccess(const std::string& card_id, const json& result_payload)
{
    auto request_it = m_ai_send_request_by_card.find(card_id);
    if (request_it == m_ai_send_request_by_card.end())
        return;

    const std::string request_id = request_it->second;
    ToolCalls::LogAISendPanelStage("finish_success", card_id, request_id, std::string("result_payload=") + ToolCalls::SafeJsonDumpForLog(result_payload));
    ClearAISendToolCallSilent(request_id, card_id);

    auto pending_it = m_pending_ai_send_calls_by_request.find(request_id);
    const bool is_mqtt_native = pending_it != m_pending_ai_send_calls_by_request.end() && pending_it->second.native_mqtt_request;
    const std::string workflow_id = pending_it != m_pending_ai_send_calls_by_request.end() ? pending_it->second.workflow_id : std::string();

    if (is_mqtt_native && m_sagent_mqtt_bridge) {
        json progress_payload = {
            {"request_id", request_id},
            {"status", "completed"},
            {"stage", "completed"},
            {"progress", 100},
            {"message", "Print sent"},
            {"data", {{"tool", "send_print"}, {"card_id", card_id}}}
        };
        if (!workflow_id.empty())
            progress_payload["workflow_id"] = workflow_id;
        m_sagent_mqtt_bridge->PublishToolResponse("progress", progress_payload);

        json mqtt_result = {
            {"request_id", request_id},
            {"ok", true},
            {"result", result_payload},
            {"error", json::object()}
        };
        if (!workflow_id.empty())
            mqtt_result["workflow_id"] = workflow_id;
        m_sagent_mqtt_bridge->PublishToolResponse("result", mqtt_result);
    } else if (m_cxagent_bridge) {
        m_cxagent_bridge->SendToolResult(request_id, true, result_payload);
        m_cxagent_bridge->MarkRequestFinished(request_id);
    }

    m_pending_ai_send_calls_by_request.erase(request_id);
    m_ai_send_request_by_card.erase(request_it);
    NotifyCxAgentStatus();
}

void MCPChatPanel::FinishAISendToolCallFailure(const std::string& card_id, const json& error_payload)
{
    auto request_it = m_ai_send_request_by_card.find(card_id);
    if (request_it == m_ai_send_request_by_card.end())
        return;

    const std::string request_id = request_it->second;
    ToolCalls::LogAISendPanelStage("finish_failure", card_id, request_id, std::string("error_payload=") + ToolCalls::SafeJsonDumpForLog(error_payload));
    ClearAISendToolCallSilent(request_id, card_id);

    auto pending_it = m_pending_ai_send_calls_by_request.find(request_id);
    const bool is_mqtt_native = pending_it != m_pending_ai_send_calls_by_request.end() && pending_it->second.native_mqtt_request;
    const std::string workflow_id = pending_it != m_pending_ai_send_calls_by_request.end() ? pending_it->second.workflow_id : std::string();

    if (is_mqtt_native && m_sagent_mqtt_bridge) {
        json mqtt_result = {
            {"request_id", request_id},
            {"ok", false},
            {"result", json::object()},
            {"error", error_payload}
        };
        if (!workflow_id.empty())
            mqtt_result["workflow_id"] = workflow_id;
        m_sagent_mqtt_bridge->PublishToolResponse("result", mqtt_result);
    } else if (m_cxagent_bridge) {
        m_cxagent_bridge->SendToolResult(request_id, false, error_payload);
        m_cxagent_bridge->MarkRequestFinished(request_id);
    }

    m_pending_ai_send_calls_by_request.erase(request_id);
    m_ai_send_request_by_card.erase(request_it);
    NotifyCxAgentStatus();
}

void MCPChatPanel::FinishAISendToolCallCanceled(const std::string& card_id, const json& result_payload)
{
    auto request_it = m_ai_send_request_by_card.find(card_id);
    if (request_it == m_ai_send_request_by_card.end())
        return;

    const std::string request_id = request_it->second;
    ToolCalls::LogAISendPanelStage("finish_canceled", card_id, request_id, std::string("result_payload=") + ToolCalls::SafeJsonDumpForLog(result_payload));
    ClearAISendToolCallSilent(request_id, card_id);
    json payload = result_payload;
    payload["canceled"] = true;

    auto pending_it = m_pending_ai_send_calls_by_request.find(request_id);
    const bool is_mqtt_native = pending_it != m_pending_ai_send_calls_by_request.end() && pending_it->second.native_mqtt_request;
    const std::string workflow_id = pending_it != m_pending_ai_send_calls_by_request.end() ? pending_it->second.workflow_id : std::string();

    if (is_mqtt_native && m_sagent_mqtt_bridge) {
        json mqtt_result = {
            {"request_id", request_id},
            {"ok", true},
            {"result", payload},
            {"error", json::object()}
        };
        if (!workflow_id.empty())
            mqtt_result["workflow_id"] = workflow_id;
        m_sagent_mqtt_bridge->PublishToolResponse("result", mqtt_result);
    } else if (m_cxagent_bridge) {
        m_cxagent_bridge->SendToolResult(request_id, true, payload);
        m_cxagent_bridge->MarkRequestFinished(request_id);
    }

    m_pending_ai_send_calls_by_request.erase(request_id);
    m_ai_send_request_by_card.erase(request_it);
    NotifyCxAgentStatus();
}

bool MCPChatPanel::ShouldSuppressAISendCardEvent(const std::string& request_id, const std::string& card_id) const
{
    if (!request_id.empty() && m_silent_ai_send_requests.find(request_id) != m_silent_ai_send_requests.end())
        return true;

    if (!card_id.empty() && m_silent_ai_send_cards.find(card_id) != m_silent_ai_send_cards.end())
        return true;

    return false;
}

void MCPChatPanel::MarkAISendToolCallSilent(const std::string& request_id, const std::string& card_id)
{
    if (!request_id.empty())
        m_silent_ai_send_requests.insert(request_id);
    if (!card_id.empty())
        m_silent_ai_send_cards.insert(card_id);
}

void MCPChatPanel::ClearAISendToolCallSilent(const std::string& request_id, const std::string& card_id)
{
    if (!request_id.empty())
        m_silent_ai_send_requests.erase(request_id);
    if (!card_id.empty())
        m_silent_ai_send_cards.erase(card_id);
}

void MCPChatPanel::HandleCxAgentStatusChanged(const Bridge::CxAgentClientBridge::StatusSnapshot& status)
{
    const bool became_connected = status.connected && !m_cxagent_was_connected;
    m_cxagent_was_connected = status.connected;

    if (became_connected) {
        m_scene_update_timer.Stop();
        m_scene_update_timer.StartOnce(10);
    }
    NotifyCxAgentStatus();
}

void MCPChatPanel::HandleSAgentMqttStatusChanged(const Bridge::SAgentMqttBridge::StatusSnapshot& status)
{
    const bool became_connected = status.connected && !m_cxagent_was_connected;
    m_cxagent_was_connected = status.connected;

    if (became_connected) {
        m_scene_update_timer.Stop();
        m_scene_update_timer.StartOnce(10);
    }
    NotifyCxAgentStatus();
}

void MCPChatPanel::HandleCxAgentChatRequest(const json& data)
{
    const std::string request_id = data.value("request_id", "");
    if (request_id.empty()) {
        SendCommandToJS("cxagent_chat_error", {
            {"request_id", request_id},
            {"message", "Missing request_id for cxagent_chat."}
        });
        return;
    }

    const std::string session_id = data.value("session_id", EnsureCxAgentSessionId());
    const std::string client_id = data.value("client_id", m_cxagent_client_id);
    const std::string user_id = data.value("user_id", m_gateway_user_id);
    const std::string token = data.value("token", m_gateway_user_token);
    const std::string message = data.value("message", "");
    const json context = data.value("context", json::object());
    if (context.is_object() && context.contains("scene_error_event")) {
        std::string scene_error_preview;
        try {
            const json& scene_error_event = context["scene_error_event"];
            if (scene_error_event.is_object() && scene_error_event.contains("raw_errors") && scene_error_event["raw_errors"].is_array()) {
                for (const auto& item : scene_error_event["raw_errors"]) {
                    if (!item.is_string())
                        continue;
                    const std::string text = item.get<std::string>();
                    if (text.empty())
                        continue;
                    if (!scene_error_preview.empty())
                        scene_error_preview += " | ";
                    scene_error_preview += text;
                }
            }
        } catch (...) {
        }
        BOOST_LOG_TRIVIAL(info) << "[CxAgentChat] scene_error_event request_id=" << request_id
                                << " session_id=" << session_id
                                << " preview=" << scene_error_preview;
    }
    BOOST_LOG_TRIVIAL(warning)
        << "[CxAgentChat] request request_id=" << request_id
        << " session_id=" << session_id
        << " client_id=" << client_id;

    json payload = {
        {"session_id", session_id},
        {"client_id", client_id},
        {"user_id", user_id},
        {"token", token},
        {"message", message},
        {"context", context}
    };

    PostCxAgentJson("cxagent_chat", request_id, "/api/chat", payload);
}

void MCPChatPanel::HandleCxAgentConfirmTaskRequest(const json& data)
{
    const std::string request_id = data.value("request_id", "");
    const std::string task_id = data.value("task_id", "");
    BOOST_LOG_TRIVIAL(warning)
        << "[CxAgentChat] confirm_task request request_id=" << request_id
        << " task_id=" << task_id
        << " approved=" << (data.value("approved", true) ? "true" : "false");
    if (request_id.empty() || task_id.empty()) {
        SendCommandToJS("cxagent_confirm_error", {
            {"request_id", request_id},
            {"task_id", task_id},
            {"message", "Missing request_id or task_id for cxagent_confirm_task."}
        });
        return;
    }

    json payload = {
        {"task_id", task_id},
        {"approved", data.value("approved", true)}
    };
    PostCxAgentJson("cxagent_confirm_task", request_id, "/api/tasks/" + task_id + "/confirm", payload);
}

void MCPChatPanel::HandleCxAgentListSessionsRequest(const json& data)
{
    const std::string request_id = data.value("request_id", "");
    if (request_id.empty()) {
        SendCommandToJS("cxagent_sessions_error", {
            {"request_id", request_id},
            {"message", "Missing request_id for cxagent_list_sessions."}
        });
        return;
    }

    GetCxAgentJson("cxagent_sessions_response", "cxagent_sessions_error", request_id, "/api/sessions");
}

void MCPChatPanel::HandleCxAgentGetTaskRequest(const json& data)
{
    const std::string request_id = data.value("request_id", "");
    const std::string task_id = data.value("task_id", "");
    if (request_id.empty() || task_id.empty()) {
        SendCommandToJS("cxagent_task_error", {
            {"request_id", request_id},
            {"task_id", task_id},
            {"message", "Missing request_id or task_id for cxagent_get_task."}
        });
        return;
    }

    GetCxAgentJson("cxagent_task_response", "cxagent_task_error", request_id, "/api/tasks/" + task_id);
}

void MCPChatPanel::GetCxAgentJson(const std::string& response_command,
                                  const std::string& error_command,
                                  const std::string& request_id,
                                  const std::string& path)
{
    const std::string base = ResolveCxAgentBaseUrl(json::object());
    Http::set_extra_headers(wxGetApp().get_extra_header());
    Http http = Http::get(base + path);
    http.timeout_connect(5)
        .timeout_max(30)
        .on_complete([this, response_command, request_id](std::string body, unsigned status) {
            BOOST_LOG_TRIVIAL(warning)
                << "[CxAgentChat] http get complete command=" << response_command
                << " request_id=" << request_id
                << " status=" << status;
            json response = json::object();
            if (!body.empty()) {
                try {
                    response = json::parse(body);
                } catch (...) {
                    response = {
                        {"raw", body}
                    };
                }
            }

            wxGetApp().CallAfter([this, response_command, request_id, response, status]() {
                SendCommandToJS(response_command, {
                    {"request_id", request_id},
                    {"status_code", status},
                    {"response", response}
                });
            });
        })
        .on_error([this, error_command, request_id](std::string body, std::string error, unsigned status) {
            json detail = json::object();
            if (!body.empty()) {
                try {
                    detail = json::parse(body);
                } catch (...) {
                    detail = {
                        {"raw", body}
                    };
                }
            }

            wxGetApp().CallAfter([this, error_command, request_id, detail, error, status]() {
                SendCommandToJS(error_command, {
                    {"request_id", request_id},
                    {"status_code", status},
                    {"message", error},
                    {"detail", detail}
                });
            });
        })
        .perform();
}

void MCPChatPanel::PostCxAgentJson(const std::string& request_command,
                                   const std::string& request_id,
                                   const std::string& path,
                                   const json& payload)
{
    const std::string base = ResolveCxAgentBaseUrl(json::object());
    Http::set_extra_headers(wxGetApp().get_extra_header());
    const bool is_streaming_chat = request_command == "cxagent_chat";
    Http http = Http::post(base + (is_streaming_chat ? "/api/chat/stream" : path));
    auto sse_buffer = std::make_shared<std::string>();
    auto sse_consumed = std::make_shared<std::size_t>(0);
    auto sse_completed = std::make_shared<bool>(false);
    auto sse_start_count = std::make_shared<std::size_t>(0);
    auto sse_delta_count = std::make_shared<std::size_t>(0);
    auto sse_total_delta_bytes = std::make_shared<std::size_t>(0);
    auto sse_pending_js_delta = std::make_shared<std::string>();
    auto sse_last_js_flush = std::make_shared<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
    auto sse_first_js_flush_sent = std::make_shared<bool>(false);

    http.header("Content-Type", "application/json")
        .header("Accept", is_streaming_chat ? "text/event-stream" : "application/json")
        .timeout_connect(5)
        .timeout_max(is_streaming_chat ? 120 : 60)
        .set_post_body(payload.dump());

    if (is_streaming_chat) {
        http.on_progress([this, request_id, sse_buffer, sse_consumed, sse_completed, sse_start_count, sse_delta_count, sse_total_delta_bytes, sse_pending_js_delta, sse_last_js_flush, sse_first_js_flush_sent](Http::Progress progress, bool& cancel) {
            auto flush_js_delta = [this, &request_id, sse_pending_js_delta, sse_last_js_flush, sse_first_js_flush_sent](bool force) {
                if (sse_pending_js_delta->empty())
                    return;
                const auto now = std::chrono::steady_clock::now();
                const auto elapsed = now - *sse_last_js_flush;
                const bool first_flush_ready = !*sse_first_js_flush_sent &&
                    (elapsed >= std::chrono::milliseconds(300) || sse_pending_js_delta->size() >= 120);
                const bool regular_flush_ready = *sse_first_js_flush_sent && elapsed >= std::chrono::seconds(2);
                if (!force && !first_flush_ready && !regular_flush_ready)
                    return;
                std::string merged_delta = std::move(*sse_pending_js_delta);
                sse_pending_js_delta->clear();
                *sse_last_js_flush = now;
                *sse_first_js_flush_sent = true;
                wxGetApp().CallAfter([this, request_id, merged_delta]() {
                    SendCommandToJS("cxagent_chat_stream_delta", {
                        {"request_id", request_id},
                        {"delta", merged_delta}
                    });
                });
            };
            (void) cancel;
            if (progress.buffer.size() < *sse_consumed)
                *sse_consumed = 0;
            if (progress.buffer.size() > *sse_consumed) {
                sse_buffer->append(normalize_sse_buffer(progress.buffer.substr(*sse_consumed)));
                *sse_consumed = progress.buffer.size();
            }

            std::string event_name;
            std::string event_data;
            while (try_parse_sse_event(*sse_buffer, event_name, event_data)) {
                json event_payload = json::object();
                if (!event_data.empty()) {
                    try {
                        event_payload = json::parse(event_data);
                    } catch (...) {
                        event_payload = {{"raw", event_data}};
                    }
                }

                if (event_name == "start") {
                    ++(*sse_start_count);
                    BOOST_LOG_TRIVIAL(info)
                        << "[CxAgentChat][stream] start request_id=" << request_id
                        << " start_count=" << *sse_start_count
                        << " buffered_bytes=" << progress.buffer.size();
                    ToolCalls::AppendWebViewLogLine({
                        {"level", "info"},
                        {"args", json::array({
                            "[CxAgentChat][stream] start",
                            json{{"request_id", request_id}, {"start_count", *sse_start_count}, {"buffered_bytes", progress.buffer.size()}}
                        })}
                    });
                    *sse_last_js_flush = std::chrono::steady_clock::now();
                    *sse_first_js_flush_sent = false;
                    wxGetApp().CallAfter([this, request_id]() {
                        SendCommandToJS("cxagent_chat_stream_start", {{"request_id", request_id}});
                    });
                    continue;
                }

                if (event_name == "delta") {
                    const std::string delta = event_payload.value("delta", std::string());
                    if (delta.empty())
                        continue;
                    ++(*sse_delta_count);
                    *sse_total_delta_bytes += delta.size();
                    BOOST_LOG_TRIVIAL(info)
                        << "[CxAgentChat][stream] delta request_id=" << request_id
                        << " delta_count=" << *sse_delta_count
                        << " chunk_bytes=" << delta.size()
                        << " total_delta_bytes=" << *sse_total_delta_bytes
                        << " buffered_bytes=" << progress.buffer.size();
                    ToolCalls::AppendWebViewLogLine({
                        {"level", "info"},
                        {"args", json::array({
                            "[CxAgentChat][stream] delta",
                            json{{"request_id", request_id}, {"delta_count", *sse_delta_count}, {"chunk_bytes", delta.size()}, {"total_delta_bytes", *sse_total_delta_bytes}, {"buffered_bytes", progress.buffer.size()}}
                        })}
                    });
                    sse_pending_js_delta->append(delta);
                    flush_js_delta(false);
                    continue;
                }

                if (event_name == "completed") {
                    *sse_completed = true;
                    BOOST_LOG_TRIVIAL(info)
                        << "[CxAgentChat][stream] completed request_id=" << request_id
                        << " start_count=" << *sse_start_count
                        << " delta_count=" << *sse_delta_count
                        << " total_delta_bytes=" << *sse_total_delta_bytes
                        << " buffered_bytes=" << progress.buffer.size();
                    ToolCalls::AppendWebViewLogLine({
                        {"level", "info"},
                        {"args", json::array({
                            "[CxAgentChat][stream] completed",
                            json{{"request_id", request_id}, {"start_count", *sse_start_count}, {"delta_count", *sse_delta_count}, {"total_delta_bytes", *sse_total_delta_bytes}, {"buffered_bytes", progress.buffer.size()}}
                        })}
                    });
                    flush_js_delta(true);
                    const json response = event_payload.value("response", json::object());
                    wxGetApp().CallAfter([this, request_id, response]() {
                        SendCommandToJS("cxagent_chat_response", {
                            {"request_id", request_id},
                            {"status_code", 200},
                            {"response", response}
                        });
                    });
                    continue;
                }

                if (event_name == "error") {
                    *sse_completed = true;
                    BOOST_LOG_TRIVIAL(error)
                        << "[CxAgentChat][stream] error request_id=" << request_id
                        << " start_count=" << *sse_start_count
                        << " delta_count=" << *sse_delta_count
                        << " total_delta_bytes=" << *sse_total_delta_bytes
                        << " payload=" << event_payload.dump();
                    ToolCalls::AppendWebViewLogLine({
                        {"level", "error"},
                        {"args", json::array({
                            "[CxAgentChat][stream] error",
                            json{{"request_id", request_id}, {"start_count", *sse_start_count}, {"delta_count", *sse_delta_count}, {"total_delta_bytes", *sse_total_delta_bytes}, {"payload", event_payload}}
                        })}
                    });
                    flush_js_delta(true);
                    wxGetApp().CallAfter([this, request_id, event_payload]() {
                        std::string message = event_payload.value("message", std::string());
                        if (message.empty())
                            message = event_payload.value("message_key", std::string("CxAgent streaming request failed."));
                        SendCommandToJS("cxagent_chat_error", {
                            {"request_id", request_id},
                            {"status_code", event_payload.value("status_code", 500)},
                            {"message", message},
                            {"detail", event_payload}
                        });
                    });
                }
            }
        });
    }

    http.on_complete([this, request_command, request_id, sse_completed](std::string body, unsigned status) {
            BOOST_LOG_TRIVIAL(warning)
                << "[CxAgentChat] http complete command=" << request_command
                << " request_id=" << request_id
                << " status=" << status;
            if (request_command == "cxagent_chat" && *sse_completed)
                return;

            json response = json::object();
            if (!body.empty()) {
                try {
                    response = json::parse(body);
                } catch (...) {
                    response = {
                        {"raw", body}
                    };
                }
            }

            wxGetApp().CallAfter([this, request_command, request_id, response, status]() {
                const std::string event_name = request_command == "cxagent_chat"
                    ? "cxagent_chat_response"
                    : "cxagent_confirm_response";
                SendCommandToJS(event_name, {
                    {"request_id", request_id},
                    {"status_code", status},
                    {"response", response}
                });
            });
        })
        .on_error([this, request_command, request_id](std::string body, std::string error, unsigned status) {
            json detail = json::object();
            if (!body.empty()) {
                try {
                    detail = json::parse(body);
                } catch (...) {
                    detail = {
                        {"raw", body}
                    };
                }
            }

            wxGetApp().CallAfter([this, request_command, request_id, detail, error, status]() {
                const std::string event_name = request_command == "cxagent_chat"
                    ? "cxagent_chat_error"
                    : "cxagent_confirm_error";
                std::string message = error;
                if (message.empty()) {
                    if (detail.contains("detail") && detail["detail"].is_string()) {
                        message = detail["detail"].get<std::string>();
                    } else if (detail.contains("message") && detail["message"].is_string()) {
                        message = detail["message"].get<std::string>();
                    } else if (detail.contains("raw") && detail["raw"].is_string()) {
                        message = detail["raw"].get<std::string>();
                    } else if (status != 0) {
                        message = "CxAgent request failed: HTTP " + std::to_string(status);
                    } else {
                        message = "CxAgent request failed.";
                    }
                }
                SendCommandToJS(event_name, {
                    {"request_id", request_id},
                    {"status_code", status},
                    {"message", message},
                    {"detail", detail}
                });
            });
        })
        .perform();
}

void MCPChatPanel::OnExportFinished(wxCommandEvent& evt)
{
    if (!m_pending_slice_request.active || !m_pending_slice_request.awaiting_export)
        return;

    const std::string request_id = m_pending_slice_request.request_id;
    const bool notify_cxagent_bridge = m_pending_slice_request.notify_cxagent_bridge && m_cxagent_bridge;
    const bool notify_sagent_mqtt_bridge = m_pending_slice_request.notify_sagent_mqtt_bridge && m_sagent_mqtt_bridge;
    const std::string exported_path = evt.GetString().ToUTF8().data();
    const json result = BuildCompletedSliceResult(exported_path);
    if (notify_cxagent_bridge) {
        m_cxagent_bridge->SendToolProgress(request_id, 100, "G-code exported", "completed", "completed");
        m_cxagent_bridge->SendToolResult(request_id, true, result);
    }
    if (notify_sagent_mqtt_bridge) {
        PublishSAgentMqttToolProgress(
            request_id,
            100,
            "completed",
            "G-code exported",
            "completed",
            {{"tool", Bridge::ActionID::START_SLICE}, {"source_action", Bridge::ActionID::START_SLICE}});
        PublishSAgentMqttToolResult(request_id, true, result, json::object());
    }
    SendCommandToJS("slice_completed", {
        {"request_id", request_id},
        {"success", true},
        {"cancelled", false},
        {"error", false},
        {"message", "G-code exported"},
        {"result", result}
    });
    if (notify_cxagent_bridge)
        m_cxagent_bridge->MarkRequestFinished(request_id);
    m_pending_slice_request = {};
    m_observed_slice_requests.erase(request_id);
    NotifyCxAgentStatus();
}

nlohmann::json MCPChatPanel::BuildCxAgentStatusJson() const
{
    json status_json = {
        {"connected", false},
        {"connecting", false},
        {"transport", "sagent-mqtt"},
        {"api_base", m_cxagent_api_base},
        {"client_id", m_cxagent_client_id},
        {"session_id", m_cxagent_session_id},
        {"tenant_id", "default"},
        {"topic_prefix", "sagent"},
        {"in_flight_request_ids", json::array()}
    };

    if (m_sagent_mqtt_bridge) {
        const auto status = m_sagent_mqtt_bridge->GetStatus();
        status_json["connected"] = status.connected;
        status_json["connecting"] = status.connecting;
        status_json["broker_url"] = status.broker_url;
        status_json["request_topic"] = status.request_topic;
        status_json["topic_prefix"] = status.topic_prefix;
        status_json["tenant_id"] = status.tenant_id;
        if (!status.client_id.empty())
            status_json["client_id"] = status.client_id;
        if (!status.session_id.empty())
            status_json["session_id"] = status.session_id;
        status_json["last_error"] = status.last_error;
        return status_json;
    }

    if (!m_cxagent_bridge)
        return status_json;

    const auto status = m_cxagent_bridge->GetStatus();
    status_json["transport"] = "websocket";
    status_json["connected"] = status.connected;
    status_json["connecting"] = status.connecting;
    if (!status.http_base_url.empty())
        status_json["api_base"] = status.http_base_url;
    status_json["ws_url"] = status.ws_url;
    if (!status.client_id.empty())
        status_json["client_id"] = status.client_id;
    if (!status.session_id.empty())
        status_json["session_id"] = status.session_id;
    status_json["last_error"] = status.last_error;
    status_json["last_client_seq"] = status.last_client_seq;
    status_json["last_seen_server_seq"] = status.last_seen_server_seq;
    status_json["in_flight_request_ids"] = status.in_flight_request_ids;
    return status_json;
}

Bridge::SAgentMqttBridge::Config MCPChatPanel::BuildSAgentMqttConfig(const json& data)
{
    auto* cfg = wxGetApp().app_config;
    auto read_value = [&](const std::string& data_key,
                          const std::string& config_key,
                          std::initializer_list<const char*> env_names,
                          const std::string& fallback = std::string()) -> std::string {
        if (data.contains(data_key)) {
            const json& value = data[data_key];
            if (value.is_string()) {
                const std::string text = boost::trim_copy(value.get<std::string>());
                if (!text.empty())
                    return text;
            } else if (value.is_number_integer()) {
                return std::to_string(value.get<int>());
            }
        }
        for (const char* env_name : env_names) {
            if (const char* env_value = std::getenv(env_name)) {
                const std::string text = boost::trim_copy(std::string(env_value));
                if (!text.empty())
                    return text;
            }
        }
        if (cfg) {
            const std::string text = boost::trim_copy(cfg->get(config_key));
            if (!text.empty())
                return text;
        }
        return fallback;
    };

    std::string region = cfg ? cfg->get("region") : std::string();
    const std::string default_host = region == "China" ? "cxagent.crealitycloud.cn" : "cxagent.crealitycloud.com";

    Bridge::SAgentMqttBridge::Config config;
    if (data.contains("mqtt") && data["mqtt"].is_object()) {
        const json& mqtt = data["mqtt"];
        config.host = mqtt.value("host", std::string());
        config.port = mqtt.value("port", 1883);
        config.username = mqtt.value("username", std::string());
        config.password = mqtt.value("password", std::string());
        config.topic_prefix = mqtt.value("topic_prefix", std::string("sagent"));
        config.tenant_id = mqtt.value("tenant_id", std::string("default"));
        config.client_id = mqtt.value("client_id", data.value("client_id", m_cxagent_client_id));
        config.session_id = data.value("session_id", EnsureCxAgentSessionId());
        return config;
    }

    config.host = read_value("host", "sagent_mqtt_host", {"SAGENT_MQTT_HOST", "MQTT_HOST"}, default_host);
    const std::string port = read_value("port", "sagent_mqtt_port", {"SAGENT_MQTT_PORT", "MQTT_PORT"}, "1883");
    try {
        config.port = std::stoi(port);
    } catch (...) {
        config.port = 1883;
    }
    config.username = read_value("username", "sagent_mqtt_username", {"SAGENT_MQTT_USERNAME", "MQTT_USERNAME"});
    config.password = read_value("password", "sagent_mqtt_password", {"SAGENT_MQTT_PASSWORD", "MQTT_PASSWORD"});
    config.topic_prefix = read_value("topic_prefix", "sagent_mqtt_topic_prefix", {"SAGENT_MQTT_TOPIC_PREFIX"}, "sagent");
    config.tenant_id = read_value("tenant_id", "sagent_mqtt_tenant_id", {"SAGENT_MQTT_TENANT_ID", "SAGENT_TENANT_ID"}, "default");
    config.client_id = data.value("client_id", m_cxagent_client_id);
    if (config.client_id.empty())
        config.client_id = m_cxagent_client_id;
    config.session_id = data.value("session_id", EnsureCxAgentSessionId());
    return config;
}

void MCPChatPanel::PublishSAgentMqttMessage(const json& data)
{
    const std::string topic = data.value("topic", std::string());
    const int qos = data.value("qos", 1);
    json payload = json::object();
    if (data.contains("payload"))
        payload = data["payload"];

    if (!m_sagent_mqtt_bridge) {
        SendCommandToJS("sagent_mqtt_publish_error", {{"topic", topic}, {"message", "SAgent MQTT bridge is not initialized"}});
        return;
    }

    const bool ok = m_sagent_mqtt_bridge->Publish(topic, payload, qos);
    if (ok) {
        SendCommandToJS("sagent_mqtt_publish_result", {{"topic", topic}, {"ok", true}});
    } else {
        const auto status = m_sagent_mqtt_bridge->GetStatus();
        SendCommandToJS("sagent_mqtt_publish_error", {{"topic", topic}, {"message", status.last_error}});
    }
    NotifyCxAgentStatus();
}

nlohmann::json MCPChatPanel::BuildCompletedSliceResult(const std::string& exported_gcode_path) const
{
    json result = {
        {"accepted", true},
        {"completed", true}
    };

    auto* plater = wxGetApp().plater();
    if (!plater)
        return result;

    auto& plate_list = plater->get_partplate_list();
    auto* current_plate = plate_list.get_curr_plate();
    auto* current_result = plate_list.get_current_slice_result();
    bool has_critical_warning = false;
    try {
        has_critical_warning = current_plate ? current_plate->get_print().has_critical_warning_status() : false;
    } catch (...) {
        has_critical_warning = false;
    }
    json state_result = Bridge::SlicerBridge::Instance().Execute(Bridge::ActionID::GET_SLICER_STATE, json::object());
    const json current_state = state_result.value("state", json::object());
    const json current_device = current_state.value("current_device", json::object());
    const json current_settings = current_state.value("settings", json::object());
    const auto& current_print_statistics = plate_list.get_current_fff_print().print_statistics();

    result["plate_index"] = plate_list.get_curr_plate_index();
    result["plate_number"] = plate_list.get_curr_plate_index() + 1;
    result["plate_count"] = plate_list.get_plate_count();
    result["slice_result_valid"] = current_plate ? current_plate->is_slice_result_valid() : false;
    result["export_strategy"] = m_pending_slice_request.export_strategy;
    result["exported_gcode_path"] = exported_gcode_path;
    result["facts"] = {
        {"project.has_model", !plater->model().objects.empty()},
        {"plate.current.has_objects", current_plate ? !current_plate->empty() : false},
        {"plate.current.has_printable_instances", current_plate ? current_plate->has_printable_instances() : false},
        {"plate.current.slice_completed", current_plate ? current_plate->is_slice_result_valid() : false},
        {"plate.current.slice_ready_for_print", current_plate ? current_plate->is_slice_result_ready_for_print() : false},
        {"plate.current.gcode_available",
            (current_plate ? current_plate->is_valid_gcode_file() : false) ||
            (current_plate ? current_plate->is_slice_result_ready_for_print() : false)},
        {"plate.current.no_critical_warnings", !has_critical_warning},
        {"device.current.valid", current_device.value("valid", false)},
        {"device.current.online", current_device.value("online", false)},
        {"device.current.idle",
            current_device.value("valid", false) && current_device.value("is_idle", false)},
        {"scene.no_blocking_errors", true},
        {"scene.layout.valid", !(current_result && current_result->toolpath_outside)},
        {"scene.filament_mapping.valid", true},
    };
    result["support_enabled"] = current_settings.value("support_enabled", "");
    result["support_type"] = current_settings.value("support_type", "");
    result["layer_height"] = current_settings.value("layer_height", "");
    result["infill_density"] = current_settings.value("infill_density", "");

    if (!current_result)
        return result;

    double filament_used_mm = current_print_statistics.total_used_filament;
    double filament_weight_g = current_print_statistics.total_weight;
    if (filament_used_mm <= 0.0 || filament_weight_g <= 0.0) {
        filament_used_mm = 0.0;
        filament_weight_g = 0.0;
        for (const auto& role_entry : current_result->print_statistics.used_filaments_per_role) {
            filament_used_mm += role_entry.second.first;
            filament_weight_g += role_entry.second.second;
        }
    }

    result["filename"] = current_result->filename;
    double estimated_time_s = 0.0;
    if (!current_result->print_statistics.modes.empty())
        estimated_time_s = current_result->print_statistics.modes[0].model_time_s();
    if (estimated_time_s <= 0.0)
        estimated_time_s = current_result->print_statistics.total_estimated_time;

    result["estimated_time_s"] = estimated_time_s;
    result["filament_cost"] = current_print_statistics.total_cost > 0.0 ? current_print_statistics.total_cost : current_result->print_statistics.total_filament_cost;
    result["filament_used_mm"] = filament_used_mm;
    result["filament_weight_g"] = filament_weight_g;
    result["total_layer_count"] = current_print_statistics.total_layer_count;
    result["toolpath_outside"] = current_result->toolpath_outside;
    result["warnings"] = json::array();
    for (const auto& warning : current_result->warnings) {
        result["warnings"].push_back({
            {"level", warning.level},
            {"message", warning.msg},
            {"error_code", warning.error_code}
        });
    }

    // 附加 UI 通知中的警告（包含几何风险分析等完整切片警告???
    result["ui_notifications"] = current_state.value("ui_notifications", json::array());

    return result;
}
std::string MCPChatPanel::ResolveCxAgentBaseUrl(const json& data) const
{
    const std::string from_data = data.value("api_base", "");
    if (!from_data.empty())
        return from_data;
    if (!m_cxagent_api_base.empty())
        return m_cxagent_api_base;
    return m_cxagent_api_base;
}

std::string MCPChatPanel::ResolveAISendCardId(const json& data) const
{
    const std::string card_id = data.value("card_id", std::string());
    if (!card_id.empty())
        return card_id;

    std::string request_id = data.value("request_id", std::string());
    if (request_id.empty())
        request_id = data.value("requestId", std::string());
    if (request_id.empty())
        return {};

    const auto pending_it = m_pending_ai_send_calls_by_request.find(request_id);
    if (pending_it != m_pending_ai_send_calls_by_request.end() && !pending_it->second.card_id.empty())
        return pending_it->second.card_id;

    if (m_ai_send_workflow) {
        const std::string resolved_card_id = m_ai_send_workflow->FindCardIdByRequestId(request_id);
        if (!resolved_card_id.empty())
            return resolved_card_id;
    }

    return {};
}
std::string MCPChatPanel::EnsureCxAgentSessionId()
{
    if (!m_cxagent_session_id.empty())
        return m_cxagent_session_id;

    m_cxagent_session_id = "c3d-session-" + std::to_string(static_cast<long long>(std::time(nullptr)));
    return m_cxagent_session_id;
}

// ---------------------------------------------------------------------------
// Utility: C++ -> JS communication
// ---------------------------------------------------------------------------

void MCPChatPanel::RunScriptInBrowser(const wxString& javascript)
{
    if (m_shutting_down)
        return;

    wxWebView* browser = m_browser;
    if (!browser)
        return;

    WebView::RunScript(browser, javascript);
}
void MCPChatPanel::SendCommandToJS(const std::string& command, const json& data)
{
    json msg;
    msg["command"] = command;
    msg["data"] = data;

    wxString js = "window.handleSlicerEvent(" + from_u8(msg.dump()) + ");";
    RunScriptInBrowser(js);
}

void MCPChatPanel::SendAgentEvent(const std::string& event_name, const json& data)
{
    json event_data;
    event_data["event"] = event_name;
    event_data["data"] = data;
    SendCommandToJS("agent_event", event_data);
}

void MCPChatPanel::NotifyModelImported(const json& data)
{
    BOOST_LOG_TRIVIAL(warning)
        << "[MCPChatPanel] NotifyModelImported data=" << data.dump();
    SendCommandToJS("model_import", data);

    if (!m_cxagent_bridge || !m_cxagent_bridge->GetStatus().connected) {
        BOOST_LOG_TRIVIAL(warning)
            << "[MCPChatPanel] NotifyModelImported skipped context_update connected=false";
        return;
    }

    auto& bridge = Bridge::SlicerBridge::Instance();
    json result = bridge.Execute(Bridge::ActionID::GET_SLICER_STATE, json::object());
    BOOST_LOG_TRIVIAL(warning)
        << "[MCPChatPanel] NotifyModelImported slicer_state success="
        << (result.value("success", false) ? "true" : "false");
    if (!result.value("success", false))
        return;

    const json state = result.value("state", json::object());
    json context_update = {
        {"project_context", state},
        {"facts", ToolCalls::BuildExplicitFactsFromState(state)},
        {"scene", {
            {"blocking_errors", ToolCalls::BuildBlockingErrorsPayload(state)}
        }}
    };

    json edited_config = bridge.Execute(Bridge::ActionID::GET_EDITED_CONFIG, json::object());
    if (edited_config.value("success", false)) {
        json current_slice_params = json::object();
        if (edited_config.contains("config") && edited_config["config"].is_object())
            current_slice_params = edited_config["config"];
        else {
            if (edited_config.contains("print"))
                current_slice_params["print"] = edited_config["print"];
            if (edited_config.contains("filament"))
                current_slice_params["filament"] = edited_config["filament"];
            if (edited_config.contains("printer"))
                current_slice_params["printer"] = edited_config["printer"];
        }
        context_update["current_slice_params"] = current_slice_params;
    }

    context_update["geometry_analysis"] = ToolCalls::BuildGeometryAnalysisFromState(state);
    context_update["visual_geometry"] = ToolCalls::BuildVisualRecommendationGeometryFromState(state);

    auto* plater = wxGetApp().plater();
    if (plater) {
        auto& plate_list = plater->get_partplate_list();
        auto* current_plate = plate_list.get_curr_plate();
        auto* current_result = plate_list.get_current_slice_result();
        if (current_plate && current_result && current_plate->is_slice_result_valid())
            context_update["slice_result"] = BuildCompletedSliceResult();
    }

    BOOST_LOG_TRIVIAL(warning)
        << "[MCPChatPanel] NotifyModelImported sending context_update has_model="
        << (state.value("has_model", false) ? "true" : "false")
        << " object_count=" << state.value("model_count", 0)
        << " facts=" << context_update["facts"].dump();
    m_cxagent_bridge->SendContextUpdate(context_update);
}
void MCPChatPanel::ReloadChat()
{
    if (auto* cfg = wxGetApp().app_config) {
        const std::string api_base = cfg->get("cxagent_api_base");
        if (!api_base.empty())
        {
            std::string region = cfg ? cfg->get("region") : std::string();
            m_cxagent_api_base = region == "China" ? "https://cxagent.crealitycloud.cn" : "https://cxagent.crealitycloud.com";
            cfg->set("cxagent_api_base", m_cxagent_api_base);
        }
            //m_cxagent_api_base = api_base;
    }
    //if (m_cxagent_api_base.empty())
    //    m_cxagent_api_base = “”;

    LoadChatPage();
}
void MCPChatPanel::SetCxAgentApiBaseAndReload(const std::string& api_base)
{
    m_cxagent_api_base = api_base;
    if (auto* cfg = wxGetApp().app_config) {
        cfg->set("cxagent_api_base", m_cxagent_api_base);
        cfg->save();
    }

    LoadChatPage();
}

void MCPChatPanel::NotifySceneChanged()
{
    // Restart the one-shot timer (300ms debounce).
    // If the timer is already running, Stop+StartOnce resets the countdown.
    m_scene_update_timer.Stop();

    // reload_scene() in GLCanvas3D.cpp  would trigger schedule_background_process(), then background_process_timer.Start(500), so here the time value should adjust ?
    m_scene_update_timer.StartOnce(300);

    // Manual scene edits can surface validation notifications slightly later.
    // Schedule lightweight retries so chat diagnostics can catch those async errors.
    ScheduleSceneRefresh(5);
}

void MCPChatPanel::NotifyCxAgentStatus()
{
    const json status = BuildCxAgentStatusJson();
    const bool connected = status.value("connected", false);
    SendCommandToJS(connected ? "cxagent_connected" : "cxagent_status", status);
}

void MCPChatPanel::NotifyGatewayUser(bool send_empty_user)
{
    const auto& user = wxGetApp().get_user();
    if (send_empty_user) {
        m_gateway_user_id.clear();
        m_gateway_user_token.clear();
    } else {
        m_gateway_user_id = user.userId;
        m_gateway_user_token = user.token;
    }

    // 获取区域配置
    auto* cfg = wxGetApp().app_config;
    std::string region = cfg ? cfg->get("region") : "";
    if (region.empty()) {
        region = "China";  // 默认中国
    }
    
    // 直接使用现成的函数获取创想云URL
    std::string crealitycloud_url = Slic3r::GUI::get_cloud_api_url();

    if (!m_js_ready)
        return;

    SendCommandToJS("gateway_user", {
        {"user_id", m_gateway_user_id},
        {"user_token", m_gateway_user_token},
        {"region", region},
        {"crealitycloud_url", crealitycloud_url}
    });
}

void MCPChatPanel::NotifyThemeChanged()
{
    if (!m_browser)
        return;
    
    // 直接通过JavaScript通知前端更新主题
    // 不依赖m_js_ready，因为主题切换可能在页面加载后任何时刻发???
    bool isDark = wxGetApp().dark_mode();
    wxString themeStr = isDark ? "dark" : "light";
    wxString js = wxString::Format(
        "if(window.applyThemeFromCpp) window.applyThemeFromCpp('%s');",
        themeStr
    );
    RunScriptInBrowser(js);
    BOOST_LOG_TRIVIAL(info) << "[MCPChatPanel] Theme changed notification sent via JS: " << themeStr.ToUTF8().data();
}

void MCPChatPanel::NotifyEditionChanged()
{
    if (!m_browser)
        return;

    // AIChatPage expects the edition as "ai" / "pro"; easy_print_mode == "1" means AI edition.
    // version_mode 只在加载 URL 时拼接一次，AI/专业版切换后不会刷新页面，
    // 这里通过 JS 实时推送当前版本，保证埋点 mode 与宿主一致。
    const wxString edition = wxGetApp().easy_mode() ? "ai" : "pro";
    wxString js = wxString::Format(
        "if(window.applyEditionFromCpp) window.applyEditionFromCpp('%s');",
        edition
    );
    RunScriptInBrowser(js);
    BOOST_LOG_TRIVIAL(info) << "[MCPChatPanel] Edition changed notification sent via JS: " << edition.ToUTF8().data();
}

void MCPChatPanel::NotifyDeviceStatusChanged()
{
    json payload = DM::DataCenter::Ins().GetData();
    if (!payload.is_object())
        payload = json::object();

    const json current_plate = build_current_plate_preview_state();
    if (!current_plate.empty()) {
        payload["slicer_state"]["current_plate"] = current_plate;
        payload["current_plate"] = current_plate;
        if (payload.contains("data") && payload["data"].is_object())
            payload["data"]["slicer_state"]["current_plate"] = current_plate;
    }

    SendCommandToJS("update_devices", payload);
}

void MCPChatPanel::RefreshAISendMappingForCurrentDevice(bool auto_match)
{
    if (m_ai_send_workflow)
        m_ai_send_workflow->RefreshMappingForCurrentDevice(auto_match);
}

void MCPChatPanel::NotifyProjectWorkflowResetRequested(const json& data)
{
    SendCommandToJS("project_workflow_reset_requested", data);
}

bool MCPChatPanel::OpenSliceWorkflowFromScene()
{
    BOOST_LOG_TRIVIAL(info) << "[MCPChatPanel] scene_slice_button send open_slice_workflow";
    json payload = {
        {"source_action", "scene_slice_button"},
        {"source_surface", "simple_scene"}
    };
    SendCommandToJS("open_slice_workflow", payload);
    return true;
}

bool MCPChatPanel::OpenSendWorkflowFromScene()
{
    BOOST_LOG_TRIVIAL(info) << "[MCPChatPanel] scene_send_button send open_print_workflow";
    json payload = {
        {"source_action", "scene_send_button"},
        {"source_surface", "simple_scene"}
    };
    SendCommandToJS("open_print_workflow", payload);
    return true;
}

// ---------------------------------------------------------------------------
// Scheduled scene refresh with retry logic for async validation
// ---------------------------------------------------------------------------

void MCPChatPanel::ScheduleSceneRefresh(int max_retries)
{
    m_pending_refresh_count = max_retries;
    m_scheduled_refresh_timer.Stop();
    m_scheduled_refresh_timer.StartOnce(500);  // First check after 500ms
}

void MCPChatPanel::OnScheduledRefreshTimer(wxTimerEvent& /*evt*/)
{
    if (m_pending_refresh_count <= 0) {
        m_scheduled_refresh_timer.Stop();
        return;
    }

    // Re-fetch the full slicer state so late-arriving warnings land in the
    // same context path as the initial refresh.
    auto& bridge = Bridge::SlicerBridge::Instance();
    json state_result = bridge.Execute(Bridge::ActionID::GET_SLICER_STATE, json::object());
    json notifs = json::array();
    if (state_result.value("success", false)) {
        SendCommandToJS("slicer_state", state_result);
        const json state = state_result.value("state", json::object());
        if (state.contains("ui_notifications"))
            notifs = state["ui_notifications"];
    } else {
        json warnings_result = bridge.Execute(Bridge::ActionID::GET_SCENE_WARNINGS, json::object());
        notifs = warnings_result.value("warnings", json::array());
    }
    SendCommandToJS("scene_warnings", notifs);

    // Check if there are still warnings present
    bool has_warnings = !notifs.empty();

    if (has_warnings && m_pending_refresh_count > 1) {
        // Warnings still present, schedule another check in 300ms
        m_pending_refresh_count--;
        m_scheduled_refresh_timer.StartOnce(300);
    } else {
        // No more warnings or max retries reached, stop refreshing
        m_pending_refresh_count = 0;
        m_scheduled_refresh_timer.Stop();
    }

}


// ---------------------------------------------------------------------------
// Free function: lightweight notification for external callers (e.g. Plater)
// ---------------------------------------------------------------------------

namespace {

MCPChatPanel* s_embedded_instance = nullptr;
MCPChatPanel* s_last_active_instance = nullptr;

MCPChatPanel* remember_active_panel(MCPChatPanel* panel)
{
    if (panel != nullptr)
        s_last_active_instance = panel;
    return panel;
}

bool activate_embedded_chat_panel_if_available()
{
    if (!wxGetApp().easy_mode() || s_embedded_instance == nullptr || !s_embedded_instance->IsShown())
        return false;

    remember_active_panel(s_embedded_instance);
    s_embedded_instance->NotifyGatewayUser();
    return true;
}

MCPChatPanel* get_floating_chat_panel()
{
    if (auto* win = MCPChatWindow::Get())
        return win->GetChatPanel();
    return nullptr;
}

template<typename Fn>
void for_each_chat_panel(Fn&& fn)
{
    MCPChatPanel* embedded = s_embedded_instance;
    MCPChatPanel* floating = get_floating_chat_panel();

    if (embedded != nullptr)
        fn(embedded);
    if (floating != nullptr && floating != embedded)
        fn(floating);
}

} // namespace

void RegisterEmbeddedAIChatPanel(MCPChatPanel* panel)
{
    s_embedded_instance = panel;
    remember_active_panel(panel);
}

void UnregisterEmbeddedAIChatPanel(MCPChatPanel* panel)
{
    if (s_embedded_instance == panel)
        s_embedded_instance = nullptr;
    if (s_last_active_instance == panel)
        s_last_active_instance = nullptr;
}

MCPChatPanel* GetEmbeddedAIChatPanel()
{
    return s_embedded_instance;
}

MCPChatPanel* GetActiveAIChatPanel()
{
    if (wxGetApp().easy_mode() && s_embedded_instance != nullptr)
        return remember_active_panel(s_embedded_instance);

    if (auto* panel = get_floating_chat_panel())
        return remember_active_panel(panel);

    if (s_embedded_instance != nullptr && s_embedded_instance->IsShown())
        return remember_active_panel(s_embedded_instance);

    return s_last_active_instance;
}


bool OpenActiveAISendWorkflowCardFromScene()
{
    if (auto* panel = GetActiveAIChatPanel())
        return panel->OpenSendWorkflowFromScene();
    return false;
}

bool OpenActiveAISliceWorkflowFromScene()
{
    if (auto* panel = GetActiveAIChatPanel())
        return panel->OpenSliceWorkflowFromScene();
    return false;
}

bool ShowProAISliceAssistantWithEmbeddedSession()
{
    wxGetApp().CallAfter([]() {
        MCPChatWindow::Show();
    });

    return true;
}

WorkflowToolbarState GetAIWorkflowToolbarState()
{
    return s_workflow_toolbar_state;
}

void SetAIWorkflowToolbarState(const WorkflowToolbarState& state)
{
    s_workflow_toolbar_state = state;
}

void NotifyAIChatSceneChanged()
{
    if (!wxGetApp().easy_mode()) {
        MCPChatPanel* panel = get_floating_chat_panel();
        if (panel == nullptr || !panel->IsShownOnScreen())
            return;

        panel->NotifySceneChanged();
        return;
    }

    if (auto* panel = GetActiveAIChatPanel())
        panel->NotifySceneChanged();
}

void NotifyAIChatModelImported(const json& data)
{
    if (auto* panel = GetActiveAIChatPanel())
        panel->NotifyModelImported(data);
}

void NotifyAIChatLoginStatusChanged(bool send_empty_user)
{
    if (auto* panel = GetActiveAIChatPanel()) {
        if (send_empty_user)
            panel->NotifyGatewayUser(true);
        else
            panel->ReloadChat();
    }
}

void NotifyAIChatThemeChanged()
{
    if (auto* panel = GetActiveAIChatPanel())
        panel->NotifyThemeChanged();
}

void NotifyAIChatEditionChanged()
{
    // AI / 专业版切换时，向所有存在的聊天面板（内嵌 + 浮动）推送当前版本，
    // 保证无论哪个面板处于活动状态，埋点 mode 都与宿主当前版本一致。
    for_each_chat_panel([](MCPChatPanel* panel) {
        panel->NotifyEditionChanged();
    });
}

void NotifyAIChatDeviceStatusChanged()
{
    if (auto* panel = GetActiveAIChatPanel())
        panel->NotifyDeviceStatusChanged();
}

void NotifyAIChatProjectWorkflowResetRequested(const json& data)
{
    if (!wxGetApp().easy_mode()) {
        MCPChatPanel* panel = get_floating_chat_panel();
        if (panel == nullptr || !panel->IsShownOnScreen())
            return;

        panel->NotifyProjectWorkflowResetRequested(data);
        return;
    }

    if (auto* panel = GetActiveAIChatPanel())
        panel->NotifyProjectWorkflowResetRequested(data);
}

// ===========================================================================
// MCPChatWindow - Floating window wrapper
// ===========================================================================

MCPChatWindow* MCPChatWindow::s_instance = nullptr;

MCPChatWindow::MCPChatWindow(wxWindow* parent)
    : DPIFrame(parent, wxID_ANY, _L("AI Chat Assistant"),
             wxDefaultPosition, wxDefaultSize,
             wxCAPTION | wxSYSTEM_MENU | wxCLOSE_BOX | wxMINIMIZE_BOX | wxRESIZE_BORDER | wxFRAME_FLOAT_ON_PARENT | wxFRAME_NO_TASKBAR)
{
    ApplyDisplaySizeLimits(wxSize(FromDIP(420), FromDIP(900)));

    // m_chat_panel = new MCPChatPanel(this);

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    // sizer->Add(m_chat_panel, 1, wxEXPAND);
    SetSizer(sizer);
    AttachChatPanel();

    Bind(wxEVT_CLOSE_WINDOW, &MCPChatWindow::OnClose, this);

    Bind(wxEVT_ICONIZE, &MCPChatWindow::OnIconize, this);
    if (parent != nullptr) {
        m_iconize_parent = parent;
        m_iconize_parent->Bind(wxEVT_ICONIZE, &MCPChatWindow::OnParentIconize, this);
    }

    if (parent) {
        CentreOnParent(wxBOTH);
    } else {
        CentreOnScreen(wxBOTH);
    }

    BOOST_LOG_TRIVIAL(info) << "[MCPChatWindow] Created";
}

bool MCPChatWindow::AttachChatPanel()
{
    if (m_chat_panel != nullptr) {
        if (!m_owns_chat_panel || s_embedded_instance == nullptr || m_chat_panel == s_embedded_instance)
            return true;

        wxSizer* old_sizer = GetSizer();
        if (old_sizer != nullptr)
            old_sizer->Detach(m_chat_panel);
        m_chat_panel->Destroy();
        m_chat_panel = nullptr;
    }

    wxSizer* sizer = GetSizer();
    if (sizer == nullptr)
        return false;

    if (s_embedded_instance != nullptr) {
        m_chat_panel = s_embedded_instance;
        m_owns_chat_panel = false;
        m_chat_panel_original_parent = m_chat_panel->GetParent();
        m_chat_panel_original_sizer = m_chat_panel_original_parent != nullptr ? m_chat_panel_original_parent->GetSizer() : nullptr;

        if (m_chat_panel_original_sizer != nullptr)
            m_chat_panel_original_sizer->Detach(m_chat_panel);
        m_chat_panel->Reparent(this);
    } else {
        m_owns_chat_panel = true;
        m_chat_panel = new MCPChatPanel(this);
    }

    sizer->Add(m_chat_panel, 1, wxEXPAND);
    m_chat_panel->Show();
    Layout();
    return true;
}

void MCPChatWindow::RestoreEmbeddedPanel()
{
    if (m_chat_panel == nullptr || m_owns_chat_panel)
        return;

    wxSizer* sizer = GetSizer();
    if (sizer != nullptr)
        sizer->Detach(m_chat_panel);

    if (m_chat_panel_original_parent != nullptr) {
        m_chat_panel->Reparent(m_chat_panel_original_parent);
        if (m_chat_panel_original_sizer != nullptr)
            m_chat_panel_original_sizer->Add(m_chat_panel, 1, wxEXPAND | wxALL, 0);
        m_chat_panel->Show();
        m_chat_panel_original_parent->Layout();
    }

    m_chat_panel = nullptr;
    m_chat_panel_original_parent = nullptr;
    m_chat_panel_original_sizer = nullptr;
    m_owns_chat_panel = true;
    Layout();
}

MCPChatWindow::~MCPChatWindow()
{
    if (m_iconize_parent != nullptr) {
        m_iconize_parent->Unbind(wxEVT_ICONIZE, &MCPChatWindow::OnParentIconize, this);
        m_iconize_parent = nullptr;
    }
    // Do not reparent the borrowed panel from a destructor. During application
    // shutdown its original Plater host may already be in wxWindow teardown,
    // and Reparent() against that stale parent triggers a pure-virtual call.
    // The regular hide path restores the embedded panel; teardown paths let the
    // current wx parent destroy it together with the floating window.
    if (s_last_active_instance == m_chat_panel)
        s_last_active_instance = nullptr;
    s_instance = nullptr;
    BOOST_LOG_TRIVIAL(info) << "[MCPChatWindow] Destroyed";
}

void MCPChatWindow::ApplyDisplaySizeLimits(const wxSize& preferred_size)
{
    wxWindow* anchor = GetParent() != nullptr ? GetParent() : this;
    int display_index = wxDisplay::GetFromWindow(anchor);
    if (display_index == wxNOT_FOUND && wxDisplay::GetCount() > 0)
        display_index = 0;

    if (display_index == wxNOT_FOUND) {
        SetMinSize(wxSize(FromDIP(420), FromDIP(660)));
        SetSize(preferred_size);
        return;
    }

    const wxSize work_size = wxDisplay(static_cast<unsigned>(display_index)).GetClientArea().GetSize();
    const int margin = FromDIP(10);
    const int max_width = std::max(1, work_size.GetWidth() - 2 * margin);
    const int max_height = std::max(1, work_size.GetHeight() - 2 * margin);
    const wxSize min_size(std::min(FromDIP(420), max_width),
                          std::min(FromDIP(660), max_height));
    const wxSize window_size(std::max(min_size.GetWidth(), std::min(preferred_size.GetWidth(), max_width)),
                             std::max(min_size.GetHeight(), std::min(preferred_size.GetHeight(), max_height)));

    SetMinSize(min_size);
    SetSize(window_size);
}

void MCPChatWindow::on_dpi_changed(const wxRect& suggested_rect)
{
    const wxSize preferred_size = suggested_rect.IsEmpty() ? GetSize() : suggested_rect.GetSize();
    ApplyDisplaySizeLimits(preferred_size);
    if (!suggested_rect.IsEmpty())
        SetPosition(suggested_rect.GetPosition());
    Layout();
}

void MCPChatWindow::OnClose(wxCloseEvent& evt)
{
    // Just hide, don't destroy (to preserve state)
    // wxFrame::Hide();
    MCPChatWindow::Hide();
    evt.Veto();
}

void MCPChatWindow::OnIconize(wxIconizeEvent& evt)
{
    evt.Skip();

    if (!evt.IsIconized())
        return;

    // Determine why we are being iconized:
    //  - owner (main window) is also iconized  -> we are being minimized together
    //    with the main window. Keep the user's visibility intent so we can come
    //    back when the owner is restored.
    //  - owner is NOT iconized                 -> the user clicked THIS window's
    //    own minimize button; treat it as "dismiss" (intent = hidden).
    wxWindow* owner = GetParent();
    const bool owner_minimized = owner != nullptr &&
                                 owner->IsKindOf(CLASSINFO(wxTopLevelWindow)) &&
                                 static_cast<wxTopLevelWindow*>(owner)->IsIconized();

    if (!owner_minimized)
        m_user_wants_visible = false;

    // Never remain minimized (would collapse to a title bar and suspend the
    // WebView2 surface). Undo the iconize and hide instead. Deferred so we don't
    // fight the in-progress iconize transition; clearing the iconized flag
    // BEFORE hiding keeps the WebView render surface alive.
    CallAfter([this]() {
        if (IsIconized())
            Iconize(false);
        if (IsShown())
            wxFrame::Show(false);
    });
}

void MCPChatWindow::OnParentIconize(wxIconizeEvent& evt)
{
    evt.Skip();

    if (evt.IsIconized())
        return; // handled via our own OnIconize

    // Owner restored. Bring the window back only if the user still wants it
    // visible (i.e. it was open, not dismissed before/while minimized).
    if (!m_user_wants_visible)
        return;

    CallAfter([this]() {
        if (!m_user_wants_visible)
            return;
        if (IsIconized())
            Iconize(false);
        if (!IsShown())
            wxFrame::Show();
        Raise();
    });
}

void MCPChatWindow::Toggle(wxWindow* parent)
{
    // if (activate_embedded_chat_panel_if_available())
    //     return;

    if (s_instance && s_instance->IsShown()) {
        // s_instance->wxFrame::Hide();
        MCPChatWindow::Hide();
    } else {
        Show(parent);
    }
}

void MCPChatWindow::Show(wxWindow* parent)
{
    // if (activate_embedded_chat_panel_if_available())
    //     return;

    if (!s_instance) {
        if (!parent) {
            parent = wxGetApp().mainframe;
        }
        s_instance = new MCPChatWindow(parent);
    }
    s_instance->AttachChatPanel();
    s_instance->m_user_wants_visible = true;
    if (s_instance->IsIconized())
        s_instance->Iconize(false);
    s_instance->wxFrame::Show();
    if (auto* panel = s_instance->GetChatPanel()) {
        remember_active_panel(panel);
        panel->NotifyGatewayUser();
    }
    s_instance->Raise();
}

void MCPChatWindow::Hide()
{
    if (s_instance) {
        s_instance->m_user_wants_visible = false;
        s_instance->wxFrame::Hide();
        s_instance->RestoreEmbeddedPanel();
    }
}

void MCPChatWindow::DestroyForGUIRecreate()
{
    MCPChatWindow* win = s_instance;
    if (win == nullptr)
        return;

    s_instance = nullptr;

    if (win->m_iconize_parent != nullptr) {
        win->m_iconize_parent->Unbind(wxEVT_ICONIZE, &MCPChatWindow::OnParentIconize, win);
        win->m_iconize_parent = nullptr;
    }

    if (win->m_chat_panel != nullptr) {
        wxSizer* sizer = win->GetSizer();
        if (sizer != nullptr)
            sizer->Detach(win->m_chat_panel);
        if (s_embedded_instance == win->m_chat_panel)
            s_embedded_instance = nullptr;
        if (s_last_active_instance == win->m_chat_panel)
            s_last_active_instance = nullptr;
        win->m_chat_panel = nullptr;
    }

    win->m_chat_panel_original_parent = nullptr;
    win->m_chat_panel_original_sizer = nullptr;
    win->m_owns_chat_panel = true;
    win->m_user_wants_visible = false;
    win->Destroy();
}

MCPChatPanel* MCPChatWindow::TakeChatPanelForEmbedding(wxWindow* new_parent, wxSizer* new_sizer)
{
    if (s_instance == nullptr || s_instance->m_chat_panel == nullptr || new_parent == nullptr || new_sizer == nullptr)
        return nullptr;

    MCPChatPanel* panel = s_instance->m_chat_panel;
    wxSizer* old_sizer = s_instance->GetSizer();
    if (old_sizer != nullptr)
        old_sizer->Detach(panel);

    s_instance->wxFrame::Hide();
    s_instance->m_user_wants_visible = false;
    panel->Reparent(new_parent);
    new_sizer->Add(panel, 1, wxEXPAND | wxALL, 0);
    panel->Show();

    s_instance->m_chat_panel = nullptr;
    s_instance->m_chat_panel_original_parent = nullptr;
    s_instance->m_chat_panel_original_sizer = nullptr;
    s_instance->m_owns_chat_panel = true;
    s_instance->Layout();
    new_parent->Layout();
    return panel;
}
MCPChatWindow* MCPChatWindow::Get()
{
    return s_instance;
}

void DestroyAIChatPanelsForGUIRecreate()
{
    MCPChatWindow::DestroyForGUIRecreate();

    MCPChatPanel* embedded = s_embedded_instance;
    s_embedded_instance = nullptr;
    s_last_active_instance = nullptr;

    if (embedded == nullptr)
        return;

    if (wxWindow* parent = embedded->GetParent()) {
        if (wxSizer* sizer = parent->GetSizer())
            sizer->Detach(embedded);
    }
    embedded->Destroy();
}

void MCPChatPanel::HandleLoadTrendingModels(int page, int page_size)
{
    BOOST_LOG_TRIVIAL(info) << "[MCPChatPanel] Load trending models: page=" << page << ", page_size=" << page_size;

    // 异步执行获取热门模型 - 一次性获???0???
    std::thread([this]() {
        try {
            // 调用模型库热门API
            std::string api_base = Slic3r::GUI::get_cloud_api_url();
            std::string trending_url = api_base + "/api/cxy/v3/model/listTrend";
            
            // 一次性请???0个模???
            json search_body = {
                {"page", 1},
                {"pageSize", 30},
                {"filterType", 10},
                {"printers", json::array()},
                {"displayVersions", json::array({"cxy-gen2"})},
                {"promoType", 0},
                {"isVip", 0},
                {"trendType", 1},
                {"multiMark", 0},
                {"isPay", 0},
                {"hasCubeMeModel", 1}
            };
            
            BOOST_LOG_TRIVIAL(info) << "[MCPChatPanel] Trending models API URL: " << trending_url;
            
            // 设置认证???
            auto extra_headers = wxGetApp().get_extra_header();
            Http::set_extra_headers(extra_headers);
            
            std::vector<json> models;
            int total = 0;
            bool success = false;
            std::string error_msg;
            
            Http http_trending = Http::post(trending_url);
            http_trending.header("Content-Type", "application/json")
                .timeout_connect(10)
                .timeout_max(30)
                .set_post_body(search_body.dump())
                .on_complete([&](std::string body, unsigned status) {
                    BOOST_LOG_TRIVIAL(info) << "[MCPChatPanel] Trending models API status: " << status;
                    if (status == 200) {
                        try {
                            json response = json::parse(body);
                            if (response["code"] == 0 && response.contains("result")) {
                                auto& result = response["result"];
                                total = result.value("count", 0);
                                
                                if (result.contains("list") && result["list"].is_array()) {
                                    auto& list = result["list"];
                                    for (const auto& item : list) {
                                        json model;
                                        model["model_id"] = item.value("id", "");
                                        model["model_group_id"] = item.value("id", "");
                                        model["model_name"] = item.value("groupName", item.value("name", "未知模型"));
                                        
                                        // 获取封面???
                                        std::string cover_image;
                                        if (item.contains("covers") && item["covers"].is_array() && !item["covers"].empty()) {
                                            cover_image = item["covers"][0].value("url", "");
                                        }
                                        model["cover_image"] = cover_image;
                                        
                                        model["likes"] = item.value("likeCount", item.value("like", 0));  // 优先使用 likeCount
                                        model["downloads"] = item.value("downloadCount", 0);
                                        
                                        // 获取作者信???
                                        if (item.contains("userInfo")) {
                                            model["author"] = item["userInfo"].value("nickname", "");
                                        } else {
                                            model["author"] = "";
                                        }
                                        
                                        models.push_back(model);
                                    }
                                    success = true;
                                    BOOST_LOG_TRIVIAL(info) << "[MCPChatPanel] Loaded " << models.size() << " trending models";
                                }
                            } else {
                                error_msg = response.value("msg", "获取热门模型失败");
                            }
                        } catch (const std::exception& e) {
                            error_msg = std::string("Parse failed: ") + e.what();
                        }
                    } else {
                        error_msg = "HTTP error: " + std::to_string(status);
                    }
                })
                .on_error([&](std::string body, std::string error, unsigned status) {
                    error_msg = "Request error: " + error;
                })
                .perform_sync();
            
            if (!success) {
                CallAfter([this, error_msg]() {
                    json error_result;
                    error_result["success"] = false;
                    error_result["error"] = error_msg.empty() ? "获取热门模型失败" : error_msg;
                    SendCommandToJS("trending_models_result", error_result);
                });
                return;
            }
            
            // 发送成功结???
            CallAfter([this, models, total]() {
                json result;
                result["success"] = true;
                result["models"] = models;
                result["total"] = total > 30 ? 30 : total; // 最???0???
                result["sort_label"] = "综合排序";
                SendCommandToJS("trending_models_result", result);
            });
            
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(error) << "[MCPChatPanel] Load trending models failed: " << e.what();
            
            json error_result;
            error_result["success"] = false;
            error_result["error"] = std::string("获取热门模型失败: ") + e.what();
            
            CallAfter([this, error_result]() {
                SendCommandToJS("trending_models_result", error_result);
            });
        }
    }).detach();
}

} // namespace GUI
} // namespace Slic3r

