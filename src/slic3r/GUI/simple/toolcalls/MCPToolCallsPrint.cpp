#include "slic3r/GUI/simple/MCPChatPanel.hpp"
#include "slic3r/GUI/simple/toolcalls/MCPToolCallsCommon.hpp"
#include "slic3r/GUI/simple/sendWorkflow/AISendWorkflowService.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/Plater.hpp"

#include <boost/log/trivial.hpp>

using json = nlohmann::json;

namespace Slic3r {
namespace GUI {

bool MCPChatPanel::TryHandlePrintToolCall(const std::string& request_id,
                                          const std::string& tool,
                                          const json& args)
{
    if (tool == "run_slice") {
        StartSliceRequest(request_id, args, true);
        return true;
    }

    if (tool == "get_slice_result") {
        m_cxagent_bridge->MarkRequestStarted(request_id);
        m_cxagent_bridge->SendToolProgress(request_id, 5, "Collecting slice result", "collecting");

        CallAfter([this, request_id]() {
            auto* plater = wxGetApp().plater();
            if (!plater) {
                m_cxagent_bridge->SendToolResult(request_id, false, {
                    {"code", "SLICE_RESULT_UNAVAILABLE"},
                    {"message", "Plater not available."}
                });
                m_cxagent_bridge->MarkRequestFinished(request_id);
                NotifyCxAgentStatus();
                return;
            }

            auto& plate_list = plater->get_partplate_list();
            auto* current_plate = plate_list.get_curr_plate();
            auto* current_result = plate_list.get_current_slice_result();
            if (!current_plate || !current_result || !current_plate->is_slice_result_valid()) {
                m_cxagent_bridge->SendToolResult(request_id, false, {
                    {"code", "SLICE_RESULT_UNAVAILABLE"},
                    {"message", "No valid slice result is available for the current plate."}
                });
                m_cxagent_bridge->MarkRequestFinished(request_id);
                NotifyCxAgentStatus();
                return;
            }

            m_cxagent_bridge->SendToolProgress(request_id, 100, "Slice result collected", "completed", "completed");
            m_cxagent_bridge->SendToolResult(request_id, true, BuildCompletedSliceResult());
            m_cxagent_bridge->MarkRequestFinished(request_id);
            NotifyCxAgentStatus();
        });

        return true;
    }

    if (tool == "send_to_printer")
        return HandleSendToPrinterToolCall(request_id, args);

    return false;

}

bool MCPChatPanel::HandleSendToPrinterToolCall(const std::string& request_id,
                                               const json& args)
{
    const bool direct_start_print =
        args.is_object() && args.value("direct_start_print", false);

    auto existing_pending_it = m_pending_ai_send_calls_by_request.find(request_id);
    if (existing_pending_it != m_pending_ai_send_calls_by_request.end()) {
        ToolCalls::LogAISendPanelStage(
            "tool_call_reuse_existing",
            existing_pending_it->second.card_id,
            request_id,
            std::string("waiting_user_action=") +
                (existing_pending_it->second.waiting_user_action ? "true" : "false"));

        m_cxagent_bridge->MarkRequestStarted(request_id);

        if (direct_start_print &&
            existing_pending_it->second.waiting_user_action &&
            !existing_pending_it->second.card_id.empty()) {
            existing_pending_it->second.waiting_user_action = false;
            MarkAISendToolCallSilent(request_id, existing_pending_it->second.card_id);

            if (!m_ai_send_workflow || !m_ai_send_workflow->StartSendAndPrint(existing_pending_it->second.card_id)) {
                ClearAISendToolCallSilent(request_id, existing_pending_it->second.card_id);
                m_cxagent_bridge->SendToolResult(request_id, false, {
                    {"code", "AI_SEND_DIRECT_START_FAILED"},
                    {"message", "Failed to start direct send and print workflow."}
                });
                m_cxagent_bridge->MarkRequestFinished(request_id);
                m_ai_send_request_by_card.erase(existing_pending_it->second.card_id);
                m_pending_ai_send_calls_by_request.erase(existing_pending_it);
                NotifyCxAgentStatus();
                return true;
            }

            m_cxagent_bridge->SendToolProgress(
                request_id,
                20,
                "Starting send and print",
                "uploading",
                "running");
        } else {
            if (!ShouldSuppressAISendCardEvent(request_id, existing_pending_it->second.card_id))
                ReplayAISendCardsToJS();

            m_cxagent_bridge->SendToolProgress(
                request_id,
                existing_pending_it->second.waiting_user_action ? 15 : 30,
                existing_pending_it->second.waiting_user_action
                    ? "Send card already open, waiting for user confirmation"
                    : "AI send workflow already started",
                existing_pending_it->second.waiting_user_action
                    ? "awaiting_user_confirmation"
                    : "uploading",
                existing_pending_it->second.waiting_user_action ? "waiting" : "running");
        }

        NotifyCxAgentStatus();
        return true;
    }

    m_cxagent_bridge->MarkRequestStarted(request_id);
    m_cxagent_bridge->SendToolProgress(
        request_id,
        5,
        direct_start_print ? "Preparing direct send and print" : "Preparing AI send card",
        "building_snapshot",
        "waiting");

    CallAfter([this, request_id, args, direct_start_print]() {
        if (!m_ai_send_workflow) {
            m_cxagent_bridge->SendToolResult(request_id, false, {
                {"code", "AI_SEND_WORKFLOW_UNAVAILABLE"},
                {"message", "AI send workflow service is not available."}
            });
            m_cxagent_bridge->MarkRequestFinished(request_id);
            NotifyCxAgentStatus();
            return;
        }

        json open_args = args.is_object() ? args : json::object();
        if (!open_args.contains("entry_mode"))
            open_args["entry_mode"] = "send_workflow";

        const auto open_result = m_ai_send_workflow->OpenCard(request_id, open_args);
        ToolCalls::LogAISendPanelStage(
            open_result.success ? "tool_call_open_card_success" : "tool_call_open_card_failed",
            open_result.card_id,
            request_id,
            open_result.success
                ? (std::string("open_args=") + ToolCalls::SafeJsonDumpForLog(open_args))
                : (std::string("code=") + open_result.code + ", message=" + open_result.message));

        if (!open_result.success) {
            m_cxagent_bridge->SendToolResult(request_id, false, {
                {"code", open_result.code.empty() ? std::string("AI_SEND_CARD_OPEN_FAILED") : open_result.code},
                {"message", open_result.message.empty() ? std::string("Failed to open AI send card") : open_result.message}
            });
            m_cxagent_bridge->MarkRequestFinished(request_id);
            NotifyCxAgentStatus();
            return;
        }

        PendingAISendToolCall pending;
        pending.request_id = request_id;
        pending.card_id = open_result.card_id;
        pending.tool_args = open_args;
        pending.waiting_user_action = !direct_start_print;
        m_pending_ai_send_calls_by_request[request_id] = pending;
        m_ai_send_request_by_card[open_result.card_id] = request_id;

        if (direct_start_print) {
            MarkAISendToolCallSilent(request_id, open_result.card_id);

            if (!m_ai_send_workflow->StartSendAndPrint(open_result.card_id)) {
                ClearAISendToolCallSilent(request_id, open_result.card_id);
                m_ai_send_request_by_card.erase(open_result.card_id);
                m_pending_ai_send_calls_by_request.erase(request_id);
                m_cxagent_bridge->SendToolResult(request_id, false, {
                    {"code", "AI_SEND_DIRECT_START_FAILED"},
                    {"message", "Failed to start direct send and print workflow."}
                });
                m_cxagent_bridge->MarkRequestFinished(request_id);
                NotifyCxAgentStatus();
                return;
            }

            m_cxagent_bridge->SendToolProgress(
                request_id,
                20,
                "Starting send and print",
                "uploading",
                "running");
        } else {
            m_cxagent_bridge->SendToolProgress(
                request_id,
                15,
                "Send card ready, waiting for user confirmation",
                "awaiting_user_confirmation",
                "waiting");
        }

        NotifyCxAgentStatus();
    });

    return true;
}


} // namespace GUI
} // namespace Slic3r
