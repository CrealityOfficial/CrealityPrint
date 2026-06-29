#ifndef slic3r_MCPChatPanel_hpp_
#define slic3r_MCPChatPanel_hpp_

#include <wx/panel.h>
#include <wx/frame.h>
#include <wx/webview.h>
#include <wx/sizer.h>
#include <wx/timer.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <memory>
#include <atomic>

#include "nlohmann/json.hpp"
#include "slic3r/GUI/GUI_Utils.hpp"
#include "slic3r/GUI/BackgroundSlicingProcess.hpp"
#include "slic3r/Utils/Http.hpp"
#include "bridge/CxAgentClientBridge.hpp"
#include "bridge/SAgentMqttBridge.hpp"
#include "bridge/SlicerBridge.hpp"

namespace Slic3r {
namespace GUI {

class AISendWorkflowService;

struct WorkflowToolbarState
{
    bool available = false;
    bool can_slice = false;
    bool can_send_print = false;
};

class MCPChatPanel : public wxPanel
{
public:
    MCPChatPanel(wxWindow* parent,
                 wxWindowID id = wxID_ANY,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize);
    virtual ~MCPChatPanel();

    void SendAgentEvent(const std::string& event_name, const nlohmann::json& data);
    void NotifyModelImported(const nlohmann::json& data);
    void ReloadChat();
    void SetCxAgentApiBaseAndReload(const std::string& api_base);
    void NotifySceneChanged();
    void NotifyCxAgentStatus();
    void NotifyGatewayUser(bool send_empty_user = false);
    void NotifyThemeChanged();
    void NotifyEditionChanged();
    void NotifyDeviceStatusChanged();
    void NotifyProjectWorkflowResetRequested(const nlohmann::json& data);
    void RefreshAISendMappingForCurrentDevice(bool auto_match);
    bool OpenSendWorkflowFromScene();
    bool OpenSliceWorkflowFromScene();
    void RegisterPendingAsyncToolCall(const std::string& request_id,
                                      const std::string& tool_name,
                                      const nlohmann::json& result_payload,
                                      bool native_request);
    void CompletePendingAsyncToolCall(const std::string& completion_key,
                                      bool success,
                                      const std::string& message,
                                      const nlohmann::json& details = nlohmann::json::object());

private:
    void InitWebView();
    void LoadChatPage();

    void OnScriptMessage(wxWebViewEvent& evt);
    void OnNavigationComplete(wxWebViewEvent& evt);
    void OnError(wxWebViewEvent& evt);
    void OnSceneUpdateTimer(wxTimerEvent& evt);
    void OnScheduledRefreshTimer(wxTimerEvent& evt);
    void OnSliceProcessCompleted(Slic3r::SlicingProcessCompletedEvent& evt);
    void OnExportFinished(wxCommandEvent& evt);

    using CommandHandler = std::function<void(const nlohmann::json&)>;
    void RegisterHandler(const std::string& command, CommandHandler handler);
    void RegisterAllHandlers();

    void ExecuteBridgeAction(const std::string& action_id, const nlohmann::json& data);
    void StartSliceRequest(const std::string& request_id,
                           const nlohmann::json& args,
                           bool notify_cxagent_bridge);
    void HandleCxAgentMessage(const nlohmann::json& msg);
    void HandleCxAgentToolCall(const nlohmann::json& msg);
    bool ShouldUseSAgentMqttNativePath(const nlohmann::json& msg) const;
    void HandleSAgentMqttNativeToolRequest(const nlohmann::json& msg);
    void HandleCxAgentCancelCall(const nlohmann::json& msg);
    void BindAISendWorkflowCallbacks();
    void ReplayAISendCardsToJS();
    nlohmann::json ExecuteAISendToPrinterAction(const nlohmann::json& data);
    void HandleAISendCardOpen(const nlohmann::json& data);
    void HandleAISendCardSelectPlate(const nlohmann::json& data);
    void HandleAISendCardAutoMatch(const nlohmann::json& data);
    void HandleAISendCardUpdateMapping(const nlohmann::json& data);
    void HandleAISendCardApplyMapping(const nlohmann::json& data);
    void HandleAISendApplyProcessIntent(const nlohmann::json& data);
    void HandleAISendCardSendOnly(const nlohmann::json& data);
    void HandleAISendCardStartPrint(const nlohmann::json& data);
    void HandleAISendCardCancel(const nlohmann::json& data);
    void HandleAISendCardRetry(const nlohmann::json& data);
    void OnAISendSnapshot(const nlohmann::json& envelope);
    void OnAISendProgress(const nlohmann::json& envelope);
    void OnAISendResult(const nlohmann::json& envelope);
    void OnAISendError(const nlohmann::json& envelope);
    void FinishAISendToolCallSuccess(const std::string& card_id, const nlohmann::json& result_payload);
    void FinishAISendToolCallFailure(const std::string& card_id, const nlohmann::json& error_payload);
    void FinishAISendToolCallCanceled(const std::string& card_id, const nlohmann::json& result_payload);
    bool ShouldSuppressAISendCardEvent(const std::string& request_id, const std::string& card_id) const;
    void MarkAISendToolCallSilent(const std::string& request_id, const std::string& card_id);
    void ClearAISendToolCallSilent(const std::string& request_id, const std::string& card_id);
    void HandleCxAgentStatusChanged(const Bridge::CxAgentClientBridge::StatusSnapshot& status);
    void HandleSAgentMqttStatusChanged(const Bridge::SAgentMqttBridge::StatusSnapshot& status);
    void HandleCxAgentChatRequest(const nlohmann::json& data);
    void HandleCxAgentConfirmTaskRequest(const nlohmann::json& data);
    void HandleCxAgentListSessionsRequest(const nlohmann::json& data);
    void HandleCxAgentGetTaskRequest(const nlohmann::json& data);
    void HandleLoadTrendingModels(int page, int page_size);
    void OpenExternalBrowserFromJS(const nlohmann::json& data);
    void GetCxAgentJson(const std::string& response_command,
                        const std::string& error_command,
                        const std::string& request_id,
                        const std::string& path);
    void PostCxAgentJson(const std::string& request_command,
                         const std::string& request_id,
                         const std::string& path,
                         const nlohmann::json& payload);
    nlohmann::json BuildCxAgentStatusJson() const;
    Bridge::SAgentMqttBridge::Config BuildSAgentMqttConfig(const nlohmann::json& data);
    void PublishSAgentMqttMessage(const nlohmann::json& data);
    nlohmann::json BuildCompletedSliceResult(const std::string& exported_gcode_path = std::string()) const;
    std::string ResolveCxAgentBaseUrl(const nlohmann::json& data) const;
    std::string ResolveAISendCardId(const nlohmann::json& data) const;
    std::string EnsureCxAgentSessionId();

    void RunScriptInBrowser(const wxString& javascript);
    void SendCommandToJS(const std::string& command, const nlohmann::json& data);
    void ScheduleSceneRefresh(int max_retries = 5);
    void OnBrowserDestroyed(wxWindowDestroyEvent& evt);

// related tool calls    
private:
    bool TryHandlePrinterToolCall(const std::string& request_id,const std::string& tool,const nlohmann::json& args);
    bool TryHandleObjectToolCall(const std::string& request_id,const std::string& tool,const nlohmann::json& args);
    bool TryHandleLayoutToolCall(const std::string& request_id,const std::string& tool,const nlohmann::json& args);
    bool TryHandleFilamentToolCall(const std::string& request_id,const std::string& tool,const nlohmann::json& args);
    bool HandleAutoMapFilamentsToolCall(const std::string& request_id,const nlohmann::json& args);
    bool TryHandleSceneContextToolCall(const std::string& request_id,const std::string& tool,const nlohmann::json& args);
    bool TryHandleConfigToolCall(const std::string& request_id,const std::string& tool,const nlohmann::json& args);
    bool TryHandleModelSourceToolCall(const std::string& request_id,const std::string& tool,const nlohmann::json& args);
    bool TryHandlePrintToolCall(const std::string& request_id,const std::string& tool,const nlohmann::json& args);
    bool HandleSendToPrinterToolCall(const std::string& request_id,const nlohmann::json& args);

private:
    wxWebView*    m_browser      = nullptr;
    wxTimer       m_scene_update_timer;
    wxTimer       m_scheduled_refresh_timer;
    bool          m_page_loaded  = false;
    bool          m_js_ready     = false;
    std::atomic<bool> m_shutting_down { false };
    std::shared_ptr<int> m_async_lifetime = std::make_shared<int>(0);
    bool          m_cxagent_was_connected = false;
    std::unordered_map<std::string, CommandHandler> m_commandHandlers;
    int           m_pending_refresh_count = 0;
    class ModelDetailDialog* m_model_detail_dlg = nullptr;  // track the open model detail dialog to avoid stacking

    std::string m_gateway_user_id;
    std::string m_gateway_user_token;
    std::string m_cxagent_api_base;
    std::string m_cxagent_client_id;
    std::string m_cxagent_session_id;
    wxString    m_chat_page_origin;  // Initial page origin used to detect external links
    std::unique_ptr<AISendWorkflowService> m_ai_send_workflow;
    std::unique_ptr<Bridge::CxAgentClientBridge> m_cxagent_bridge;
    std::unique_ptr<Bridge::SAgentMqttBridge> m_sagent_mqtt_bridge;

    struct PendingAISendToolCall {
        std::string request_id;
        std::string card_id;
        std::string tool_name = "send_to_printer";
        nlohmann::json tool_args = nlohmann::json::object();
        bool from_agent = true;
        bool waiting_user_action = true;
        bool terminal_reported = false;
    };

    std::unordered_map<std::string, PendingAISendToolCall> m_pending_ai_send_calls_by_request;
    std::unordered_map<std::string, std::string> m_ai_send_request_by_card;
    std::unordered_set<std::string> m_silent_ai_send_requests;
    std::unordered_set<std::string> m_silent_ai_send_cards;

    struct PendingSliceRequest {
        bool active = false;
        bool awaiting_export = false;
        bool notify_cxagent_bridge = false;
        std::string request_id;
        std::string export_strategy;
        std::string output_path;
    } m_pending_slice_request;

    struct PendingAsyncToolCall {
        bool native_request = false;
        std::string request_id;
        std::string tool;
        std::string lifecycle;
        std::string completion_key;
        std::string completion_source;
        nlohmann::json result_payload = nlohmann::json::object();
    };

    std::unordered_map<std::string, PendingAsyncToolCall> m_pending_async_tool_calls;
};

class MCPChatWindow : public DPIFrame
{
public:
    static void Toggle(wxWindow* parent = nullptr);
    static void Show(wxWindow* parent = nullptr);
    static void Hide();
    static MCPChatPanel* TakeChatPanelForEmbedding(wxWindow* new_parent, wxSizer* new_sizer);
    static MCPChatWindow* Get();
    static void DestroyForGUIRecreate();
    MCPChatPanel* GetChatPanel() { return m_chat_panel; }

private:
    MCPChatWindow(wxWindow* parent);
    ~MCPChatWindow();
    bool AttachChatPanel();
    void RestoreEmbeddedPanel();
    void OnClose(wxCloseEvent& evt);
    void OnIconize(wxIconizeEvent& evt);
    void OnParentIconize(wxIconizeEvent& evt);
    void on_dpi_changed(const wxRect& suggested_rect) override;

    MCPChatPanel* m_chat_panel = nullptr;
    wxWindow* m_chat_panel_original_parent = nullptr;
    wxSizer* m_chat_panel_original_sizer = nullptr;
    bool m_owns_chat_panel = true;
    wxWindow* m_iconize_parent = nullptr;   // parent watched for minimize/restore
    bool m_user_wants_visible = false;      // authoritative: should the window be visible?
    static MCPChatWindow* s_instance;
};

void RegisterEmbeddedAIChatPanel(MCPChatPanel* panel);
void UnregisterEmbeddedAIChatPanel(MCPChatPanel* panel);
MCPChatPanel* GetEmbeddedAIChatPanel();
MCPChatPanel* GetActiveAIChatPanel();
bool OpenActiveAISendWorkflowCardFromScene();
bool OpenActiveAISliceWorkflowFromScene();
bool ShowProAISliceAssistantWithEmbeddedSession();
WorkflowToolbarState GetAIWorkflowToolbarState();
void SetAIWorkflowToolbarState(const WorkflowToolbarState& state);

void NotifyAIChatSceneChanged();
void NotifyAIChatModelImported(const nlohmann::json& data);
void NotifyAIChatLoginStatusChanged(bool send_empty_user = false);
void NotifyAIChatThemeChanged();
void NotifyAIChatEditionChanged();
void NotifyAIChatDeviceStatusChanged();
void NotifyAIChatProjectWorkflowResetRequested(const nlohmann::json& data);
void DestroyAIChatPanelsForGUIRecreate();
} // namespace GUI
} // namespace Slic3r

#endif // slic3r_MCPChatPanel_hpp_
