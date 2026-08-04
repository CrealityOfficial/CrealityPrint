#include "slic3r/GUI/simple/MCPChatPanel.hpp"
#include "slic3r/GUI/simple/toolcalls/MCPToolCallsCommon.hpp"
#include "slic3r/GUI/simple/sendWorkflow/AISendWorkflowService.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/Plater.hpp"

#include <boost/log/trivial.hpp>

#include <algorithm>
#include <cctype>
#include <initializer_list>

using json = nlohmann::json;

namespace Slic3r {
namespace GUI {
namespace {

bool ShouldDirectStartPrint(const json& args)
{
    if (!args.is_object())
        return false;

    const std::string dispatch_mode = args.value("dispatch_mode", std::string());
    return args.value("direct_start_print", false) ||
           args.value("skip_local_confirmation", false) ||
           dispatch_mode == "host_direct_start";
}

} // namespace

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
    const bool direct_start_print = ShouldDirectStartPrint(args);

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

void MCPChatPanel::StartSliceRequest(const std::string& request_id,
                                     const json& args,
                                     bool notify_cxagent_bridge,
                                     bool notify_sagent_mqtt_bridge)
{
    auto parse_string_key = [&args](std::initializer_list<const char*> keys) -> std::string {
        for (const char* key : keys) {
            try {
                if (args.contains(key) && args[key].is_string()) {
                    const std::string value = args[key].get<std::string>();
                    if (!value.empty())
                        return value;
                }
            } catch (...) {
            }
        }
        return {};
    };

    const std::string output_path = parse_string_key({"output_path", "export_path", "gcode_path"});
    const std::string workflow_id = parse_string_key({"workflow_id", "workflowId"});
    std::string export_strategy = parse_string_key({"export_strategy", "export_mode"});
    std::transform(export_strategy.begin(), export_strategy.end(), export_strategy.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    if (export_strategy.empty())
        export_strategy = output_path.empty() ? "slice_only" : "auto_export";

    if (notify_cxagent_bridge && m_cxagent_bridge) {
        m_cxagent_bridge->MarkRequestStarted(request_id);
        m_cxagent_bridge->SendToolProgress(request_id, 5, "Slice request accepted", "starting");
    }

    m_pending_slice_request.active = true;
    m_pending_slice_request.awaiting_export = false;
    m_pending_slice_request.notify_cxagent_bridge = notify_cxagent_bridge;
    m_pending_slice_request.notify_sagent_mqtt_bridge = notify_sagent_mqtt_bridge;
    m_pending_slice_request.request_id = request_id;
    m_pending_slice_request.workflow_id = workflow_id;
    m_pending_slice_request.export_strategy = export_strategy;
    m_pending_slice_request.output_path = output_path;
    m_observed_slice_requests.erase(request_id);

    CallAfter([this, request_id, args]() {
        json bridge_result = Bridge::SlicerBridge::Instance().Execute(Bridge::ActionID::START_SLICE, args);
        if (!request_id.empty() && !bridge_result.contains("request_id"))
            bridge_result["request_id"] = request_id;

        const bool success = bridge_result.value("success", false);
        SendCommandToJS("start_slice", bridge_result);
        SendCommandToJS("slice_started", bridge_result);

        if (success) {
            if (bridge_result.contains("export_strategy") && bridge_result["export_strategy"].is_string())
                m_pending_slice_request.export_strategy = bridge_result["export_strategy"].get<std::string>();
            if (bridge_result.contains("output_path") && bridge_result["output_path"].is_string())
                m_pending_slice_request.output_path = bridge_result["output_path"].get<std::string>();
            m_observed_slice_requests.insert(request_id);
            if (m_pending_slice_request.notify_cxagent_bridge && m_cxagent_bridge)
                m_cxagent_bridge->SendToolProgress(request_id, 15, "Slice action started", "slicing");
            if (m_pending_slice_request.notify_sagent_mqtt_bridge)
                PublishSAgentMqttToolProgress(
                    request_id,
                    15,
                    "slicing",
                    "Slice action started",
                    "running",
                    {{"tool", Bridge::ActionID::START_SLICE}, {"source_action", Bridge::ActionID::START_SLICE}});
            NotifyCxAgentStatus();
            return;
        }

        const json error = {
            {"code", "SLICE_START_FAILED"},
            {"message", bridge_result.value("message", std::string("Failed to start slice"))},
            {"details", bridge_result}
        };
        if (m_pending_slice_request.notify_cxagent_bridge && m_cxagent_bridge) {
            m_cxagent_bridge->SendToolResult(request_id, false, error);
            m_cxagent_bridge->MarkRequestFinished(request_id);
        }
        if (m_pending_slice_request.notify_sagent_mqtt_bridge)
            PublishSAgentMqttToolResult(request_id, false, json::object(), error);
        m_pending_slice_request = {};
        m_observed_slice_requests.erase(request_id);
        NotifyCxAgentStatus();
    });
}

void MCPChatPanel::PublishSAgentMqttToolProgress(const std::string& request_id,
                                                 int progress,
                                                 const std::string& stage,
                                                 const std::string& message,
                                                 const std::string& status,
                                                 const json& data)
{
    if (!m_sagent_mqtt_bridge || request_id.empty())
        return;

    json payload = {
        {"request_id", request_id},
        {"status", status},
        {"stage", stage},
        {"progress", progress},
        {"message", message},
        {"data", data}
    };
    if (m_pending_slice_request.request_id == request_id && !m_pending_slice_request.workflow_id.empty())
        payload["workflow_id"] = m_pending_slice_request.workflow_id;

    if (!m_sagent_mqtt_bridge->PublishToolResponse("progress", payload))
        BOOST_LOG_TRIVIAL(warning) << "[SAgentMQTT][Slice] failed to publish progress request_id=" << request_id;
}

void MCPChatPanel::PublishSAgentMqttToolResult(const std::string& request_id,
                                               bool ok,
                                               const json& result,
                                               const json& error)
{
    if (!m_sagent_mqtt_bridge || request_id.empty())
        return;

    json payload = {
        {"request_id", request_id},
        {"ok", ok},
        {"result", ok ? result : json::object()},
        {"error", ok ? json::object() : error}
    };
    if (m_pending_slice_request.request_id == request_id && !m_pending_slice_request.workflow_id.empty())
        payload["workflow_id"] = m_pending_slice_request.workflow_id;

    if (!m_sagent_mqtt_bridge->PublishToolResponse("result", payload))
        BOOST_LOG_TRIVIAL(warning) << "[SAgentMQTT][Slice] failed to publish result request_id=" << request_id;
}


void MCPChatPanel::HandleSendToPrinterMqttNative(const std::string& request_id,
                                                   const std::string& tool,
                                                   const json& args,
                                                   const std::string& workflow_id)
{
    const bool direct_start_print = ShouldDirectStartPrint(args);

    CallAfter([this, request_id, tool, args, workflow_id, direct_start_print]() {
        if (!m_ai_send_workflow) {
            if (m_sagent_mqtt_bridge) {
                json error_payload = {
                    {"request_id", request_id},
                    {"ok", false},
                    {"result", json::object()},
                    {"error", {{"code", "AI_SEND_WORKFLOW_UNAVAILABLE"}, {"message", "AI send workflow service is not available."}}}
                };
                if (!workflow_id.empty())
                    error_payload["workflow_id"] = workflow_id;
                m_sagent_mqtt_bridge->PublishToolResponse("result", error_payload);
            }
            return;
        }

        json open_args = args.is_object() ? args : json::object();
        if (!open_args.contains("entry_mode"))
            open_args["entry_mode"] = "send_workflow";

        const auto open_result = m_ai_send_workflow->OpenCard(request_id, open_args);
        ToolCalls::LogAISendPanelStage(
            open_result.success ? "mqtt_native_send_open_success" : "mqtt_native_send_open_failed",
            open_result.card_id,
            request_id,
            open_result.success
                ? (std::string("open_args=") + ToolCalls::SafeJsonDumpForLog(open_args))
                : (std::string("code=") + open_result.code + ", message=" + open_result.message));

        if (!open_result.success) {
            if (m_sagent_mqtt_bridge) {
                json error_payload = {
                    {"request_id", request_id},
                    {"ok", false},
                    {"result", json::object()},
                    {"error", {
                        {"code", open_result.code.empty() ? std::string("AI_SEND_CARD_OPEN_FAILED") : open_result.code},
                        {"message", open_result.message.empty() ? std::string("Failed to open AI send card") : open_result.message}
                    }}
                };
                if (!workflow_id.empty())
                    error_payload["workflow_id"] = workflow_id;
                m_sagent_mqtt_bridge->PublishToolResponse("result", error_payload);
            }
            return;
        }

        PendingAISendToolCall pending;
        pending.request_id = request_id;
        pending.card_id = open_result.card_id;
        pending.workflow_id = workflow_id;
        pending.tool_args = open_args;
        pending.waiting_user_action = !direct_start_print;
        pending.native_mqtt_request = true;
        m_pending_ai_send_calls_by_request[request_id] = pending;
        m_ai_send_request_by_card[open_result.card_id] = request_id;

        if (direct_start_print) {
            // Keep the existing C++ -> WebView AI send progress events enabled.
            // MQTT remains responsible for the remote tool lifecycle only.

            if (!m_ai_send_workflow->StartSendAndPrint(open_result.card_id)) {
                ClearAISendToolCallSilent(request_id, open_result.card_id);
                m_ai_send_request_by_card.erase(open_result.card_id);
                m_pending_ai_send_calls_by_request.erase(request_id);
                if (m_sagent_mqtt_bridge) {
                    json error_payload = {
                        {"request_id", request_id},
                        {"ok", false},
                        {"result", json::object()},
                        {"error", {{"code", "AI_SEND_DIRECT_START_FAILED"}, {"message", "Failed to start direct send and print workflow."}}}
                    };
                    if (!workflow_id.empty())
                        error_payload["workflow_id"] = workflow_id;
                    m_sagent_mqtt_bridge->PublishToolResponse("result", error_payload);
                }
                return;
            }
        }

        NotifyCxAgentStatus();
    });
}


} // namespace GUI
} // namespace Slic3r
