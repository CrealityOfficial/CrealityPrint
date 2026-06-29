#include "slic3r/GUI/simple/MCPChatPanel.hpp"
#include "slic3r/GUI/simple/toolcalls/MCPToolCallsCommon.hpp"
#include "slic3r/GUI/GUI_App.hpp"

#include <boost/log/trivial.hpp>

#include <algorithm>
#include <cctype>
#include <exception>
#include <initializer_list>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace Slic3r {
namespace GUI {
namespace {

std::string ToLowerCopy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string TrimCopy(const std::string& value)
{
    auto begin = value.begin();
    while (begin != value.end() && std::isspace(static_cast<unsigned char>(*begin)))
        ++begin;

    auto end = value.end();
    while (end != begin && std::isspace(static_cast<unsigned char>(*(end - 1))))
        --end;

    return std::string(begin, end);
}

std::string ReadStringKey(const json& source, const char* key)
{
    if (!source.is_object() || !source.contains(key))
        return {};

    const auto& value = source[key];
    if (value.is_string())
        return TrimCopy(value.get<std::string>());
    if (value.is_boolean())
        return value.get<bool>() ? "true" : "false";
    return {};
}

void CollectRouteMarkers(const json& source,
                         std::initializer_list<const char*> keys,
                         std::vector<std::string>& out)
{
    if (!source.is_object())
        return;

    for (const char* key : keys) {
        std::string value = ReadStringKey(source, key);
        if (!value.empty())
            out.push_back(value);
    }
}

bool IsNativeRouteMarker(const std::string& value)
{
    const std::string normalized = ToLowerCopy(TrimCopy(value));
    return normalized == "native" ||
           normalized == "slicer_native" ||
           normalized == "slicer-native" ||
           normalized == "cxx_native" ||
           normalized == "cxx-native" ||
           normalized == "cpp_native" ||
           normalized == "cpp-native" ||
           normalized == "c++_native" ||
           normalized == "true" ||
           normalized == "1";
}

std::string ExtractRequestId(const json& msg)
{
    std::string request_id = ReadStringKey(msg, "request_id");
    if (!request_id.empty())
        return request_id;

    const json args = msg.value("args", json::object());
    return ReadStringKey(args, "request_id");
}

json BuildNativeArgs(const json& msg)
{
    json args = msg.value("args", json::object());
    if (!args.is_object())
        args = json::object();

    args.erase("_sagent_executor");
    args.erase("_sagent_route");
    args.erase("_sagent_native");

    return args;
}

std::string JsonStringValue(const json& source,
                            const char* key,
                            const std::string& fallback)
{
    if (!source.is_object() || !source.contains(key))
        return fallback;

    const auto& value = source[key];
    if (value.is_string())
        return value.get<std::string>();

    return fallback;
}

bool JsonSuccessValue(const json& source)
{
    if (!source.is_object())
        return false;
    if (source.contains("success") && source["success"].is_boolean())
        return source["success"].get<bool>();
    if (source.contains("ok") && source["ok"].is_boolean())
        return source["ok"].get<bool>();
    return false;
}

std::string ExtractAsyncCompletionKey(const json& source)
{
    if (!source.is_object())
        return {};

    const json async_completion = source.value("async_completion", json::object());
    if (!async_completion.is_object())
        return {};

    return JsonStringValue(async_completion, "completion_key", std::string());
}

std::string ExtractAsyncCompletionSource(const json& source)
{
    if (!source.is_object())
        return {};

    const json async_completion = source.value("async_completion", json::object());
    if (!async_completion.is_object())
        return {};

    return JsonStringValue(async_completion, "completion_source", std::string());
}

json BuildError(const std::string& code,
                const std::string& message,
                const json& details = json::object())
{
    json error = {
        {"code", code},
        {"message", message}
    };
    if (!details.empty())
        error["details"] = details;
    return error;
}

} // namespace

bool MCPChatPanel::ShouldUseSAgentMqttNativePath(const json& msg) const
{
    if (!msg.is_object())
        return false;

    if (msg.value("message_kind", std::string()) != "tool_request")
        return false;

    std::vector<std::string> markers;
    CollectRouteMarkers(msg, {"executor", "route", "_sagent_executor", "_sagent_route", "_sagent_native"}, markers);

    const json args = msg.value("args", json::object());
    CollectRouteMarkers(args, {"_sagent_executor", "_sagent_route", "_sagent_native"}, markers);

    for (const auto& marker : markers) {
        if (IsNativeRouteMarker(marker))
            return true;
    }

    return false;
}

void MCPChatPanel::HandleSAgentMqttNativeToolRequest(const json& msg)
{
    if (!m_sagent_mqtt_bridge) {
        BOOST_LOG_TRIVIAL(warning)
            << "[SAgentMQTT][Native] ignored request because bridge is not initialized";
        return;
    }

    const std::string request_id = ExtractRequestId(msg);
    const std::string tool = ReadStringKey(msg, "tool");

    if (request_id.empty()) {
        BOOST_LOG_TRIVIAL(warning)
            << "[SAgentMQTT][Native] ignored request without request_id tool=" << tool;
        return;
    }

    auto publish_ack = [this, &request_id](bool accepted, const std::string& message) {
        json payload = {
            {"request_id", request_id},
            {"accepted", accepted},
            {"message", message}
        };
        if (!m_sagent_mqtt_bridge->PublishToolResponse("ack", payload))
            BOOST_LOG_TRIVIAL(warning) << "[SAgentMQTT][Native] failed to publish ack request_id=" << request_id;
    };

    auto publish_progress = [this, &request_id](int progress,
                                                const std::string& stage,
                                                const std::string& message,
                                                const std::string& status,
                                                const json& data) {
        json payload = {
            {"request_id", request_id},
            {"status", status},
            {"stage", stage},
            {"progress", progress},
            {"message", message},
            {"data", data}
        };
        if (!m_sagent_mqtt_bridge->PublishToolResponse("progress", payload))
            BOOST_LOG_TRIVIAL(warning) << "[SAgentMQTT][Native] failed to publish progress request_id=" << request_id;
    };

    auto publish_result = [this, &request_id](bool ok, const json& result, const json& error) {
        json payload = {
            {"request_id", request_id},
            {"ok", ok},
            {"result", ok ? result : json::object()},
            {"error", ok ? json::object() : error}
        };
        if (!m_sagent_mqtt_bridge->PublishToolResponse("result", payload))
            BOOST_LOG_TRIVIAL(warning) << "[SAgentMQTT][Native] failed to publish result request_id=" << request_id;
    };

    if (tool.empty()) {
        publish_ack(false, "native tool request missing tool name");
        publish_result(false, json::object(), BuildError("invalid_request", "native tool request missing tool name", msg));
        return;
    }

    const ToolCalls::BridgeToolRouteSpec* spec = ToolCalls::FindBridgeToolRouteSpec(tool);
    if (!spec || !spec->native_supported) {
        publish_ack(false, "tool is not supported by slicer native path");
        publish_result(
            false,
            json::object(),
            BuildError(
                "unsupported_tool",
                "tool is not supported by slicer native path: " + tool,
                {{"tool", tool}}));
        return;
    }

    publish_ack(true, "native tool accepted");
    publish_progress(
        5,
        spec->progress_stage,
        spec->progress_start_message,
        "running",
        {{"tool", tool}, {"source_action", spec->action_id}});

    json args = BuildNativeArgs(msg);
    if (spec->normalize_apply_config) {
        args = ToolCalls::NormalizeApplyParamPatchArgs(args);
        if (args.empty()) {
            publish_result(
                false,
                json::object(),
                BuildError("invalid_arguments", "No valid config patch entries were provided.", msg.value("args", json::object())));
            return;
        }
    }

    try {
        BOOST_LOG_TRIVIAL(info)
            << "[SAgentMQTT][Native] executing request_id=" << request_id
            << " tool=" << tool
            << " action=" << spec->action_id
            << " args=" << args.dump();

        json bridge_result = Bridge::SlicerBridge::Instance().Execute(spec->action_id, args);
        const bool success = JsonSuccessValue(bridge_result);

        if (!success) {
            const std::string code = JsonStringValue(bridge_result, "code", spec->error_code);
            const std::string message = JsonStringValue(bridge_result, "message", spec->error_message);
            publish_result(false, json::object(), BuildError(code, message, bridge_result));
            return;
        }

        if (!bridge_result.contains("tool"))
            bridge_result["tool"] = tool;
        if (!bridge_result.contains("source_action"))
            bridge_result["source_action"] = spec->action_id;

        if (spec->attach_project_context && !bridge_result.contains("project_context")) {
            json state_result = Bridge::SlicerBridge::Instance().Execute(Bridge::ActionID::GET_SLICER_STATE, json::object());
            if (JsonSuccessValue(state_result))
                bridge_result["project_context"] = state_result.value("state", json::object());
        }

        const std::string lifecycle = ToLowerCopy(JsonStringValue(bridge_result, "lifecycle", std::string()));
        const std::string completion_key = ExtractAsyncCompletionKey(bridge_result);
        const bool defer_until_completion =
            wxGetApp().easy_mode() &&
            GetEmbeddedAIChatPanel() == this &&
            (!completion_key.empty() || lifecycle == "async_pending" || bridge_result.value("requires_settle", false));
        if (defer_until_completion) {
            RegisterPendingAsyncToolCall(request_id, tool, bridge_result, true);
            publish_progress(
                65,
                spec->progress_stage,
                spec->progress_done_message,
                "running",
                {{"tool", tool}, {"source_action", spec->action_id}, {"lifecycle", bridge_result.value("lifecycle", std::string("async_pending"))}, {"completion_key", completion_key}});
            return;
        }

        publish_progress(
            100,
            "completed",
            spec->progress_done_message,
            "completed",
            {{"tool", tool}, {"source_action", spec->action_id}});
        publish_result(true, bridge_result, json::object());
    } catch (const std::exception& e) {
        publish_result(
            false,
            json::object(),
            BuildError("execution_failed", e.what(), {{"tool", tool}, {"source_action", spec->action_id}}));
    } catch (...) {
        publish_result(
            false,
            json::object(),
            BuildError("execution_failed", "Unknown native tool execution failure.", {{"tool", tool}, {"source_action", spec->action_id}}));
    }
}

void MCPChatPanel::RegisterPendingAsyncToolCall(const std::string& request_id,
                                                const std::string& tool_name,
                                                const json& result_payload,
                                                bool native_request)
{
    json payload = result_payload.is_object() ? result_payload : json::object();
    if (!payload.contains("request_id"))
        payload["request_id"] = request_id;

    PendingAsyncToolCall pending;
    pending.native_request = native_request;
    pending.request_id = request_id;
    pending.tool = tool_name.empty() ? JsonStringValue(payload, "tool", std::string("async_tool")) : tool_name;
    pending.lifecycle = JsonStringValue(payload, "lifecycle", std::string("async_pending"));
    pending.completion_key = ExtractAsyncCompletionKey(payload);
    pending.completion_source = ExtractAsyncCompletionSource(payload);
    pending.result_payload = payload;

    if (pending.completion_key.empty()) {
        const std::string normalized_tool = ToLowerCopy(pending.tool);
        pending.completion_key = "job:" + normalized_tool;
        if (pending.completion_source.empty())
            pending.completion_source = "job_finalize";
    }

    m_pending_async_tool_calls[pending.completion_key] = pending;
}

void MCPChatPanel::CompletePendingAsyncToolCall(const std::string& completion_key,
                                                bool success,
                                                const std::string& message,
                                                const json& details)
{
    auto pending_it = m_pending_async_tool_calls.find(completion_key);
    if (pending_it == m_pending_async_tool_calls.end())
        return;

    PendingAsyncToolCall pending = pending_it->second;
    m_pending_async_tool_calls.erase(pending_it);

    if (success) {
        json result = pending.result_payload.is_object() ? pending.result_payload : json::object();
        result["tool"] = pending.tool;
        result["message"] = message.empty() ? (pending.tool + std::string(" completed")) : message;
        result["lifecycle"] = "completed";
        result["effect_status"] = "completed";
        result["requires_settle"] = false;
        result["requires_async_completion"] = false;
        result["completion_key"] = pending.completion_key;

        if (!pending.completion_source.empty())
            result["completed_via"] = pending.completion_source;
        if (details.is_object() && !details.empty()) {
            result["completion_details"] = details;
            const std::string normalized_tool = ToLowerCopy(pending.tool);
            if (normalized_tool.find("arrange") != std::string::npos)
                result["arrange_job"] = details;
            else if (normalized_tool.find("orient") != std::string::npos)
                result["orient_job"] = details;
        }

        json state_result = Bridge::SlicerBridge::Instance().Execute(
            Bridge::ActionID::GET_SLICER_STATE,
            json::object());
        if (state_result.value("success", false))
            result["project_context"] = state_result.value("state", json::object());

        if (pending.native_request) {
            if (!m_sagent_mqtt_bridge)
                return;

            m_sagent_mqtt_bridge->PublishToolResponse("progress", {
                {"request_id", pending.request_id},
                {"status", "completed"},
                {"stage", "completed"},
                {"progress", 100},
                {"message", result["message"]},
                {"data", {{"tool", pending.tool}, {"completed_via", result.value("completed_via", std::string())}, {"completion_key", pending.completion_key}}}
            });
            m_sagent_mqtt_bridge->PublishToolResponse("result", {
                {"request_id", pending.request_id},
                {"ok", true},
                {"result", result},
                {"error", json::object()}
            });
            return;
        }

        if (!m_cxagent_bridge)
            return;

        m_cxagent_bridge->SendToolProgress(
            pending.request_id,
            100,
            result["message"],
            "completed",
            "completed");
        m_cxagent_bridge->SendToolResult(pending.request_id, true, result);
        m_cxagent_bridge->MarkRequestFinished(pending.request_id);
        NotifyCxAgentStatus();
        return;
    }

    const std::string normalized_tool = ToLowerCopy(pending.tool);
    std::string default_error_code = "ASYNC_TOOL_FINALIZE_FAILED";
    if (normalized_tool.find("arrange") != std::string::npos)
        default_error_code = "AUTO_ARRANGE_FINALIZE_FAILED";
    else if (normalized_tool.find("orient") != std::string::npos)
        default_error_code = "AUTO_ORIENT_FINALIZE_FAILED";

    json error = {
        {"code", details.value("code", default_error_code)},
        {"message", message.empty() ? (pending.tool + std::string(" failed")) : message}
    };
    if (details.is_object() && !details.empty())
        error["details"] = details;

    if (pending.native_request) {
        if (!m_sagent_mqtt_bridge)
            return;
        m_sagent_mqtt_bridge->PublishToolResponse("result", {
            {"request_id", pending.request_id},
            {"ok", false},
            {"result", json::object()},
            {"error", error}
        });
        return;
    }

    if (!m_cxagent_bridge)
        return;

    m_cxagent_bridge->SendToolResult(pending.request_id, false, error);
    m_cxagent_bridge->MarkRequestFinished(pending.request_id);
    NotifyCxAgentStatus();
}
} // namespace GUI
} // namespace Slic3r
