#include "slic3r/GUI/simple/MCPChatPanel.hpp"

#include <boost/log/trivial.hpp>

#include <algorithm>
#include <cctype>
#include <initializer_list>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace Slic3r {
namespace GUI {
namespace {

struct LayoutToolSpec {
    std::string tool;
    std::string action_id;
    std::string result_key;
    std::string progress_start_message;
    std::string progress_stage;
    std::string progress_done_message;
    std::string error_code;
    std::string error_message;
};

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

std::string ParseStringKey(const json& args, std::initializer_list<const char*> keys)
{
    if (!args.is_object())
        return {};

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
}

std::string ToLowerCopy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

const LayoutToolSpec* FindSimpleLayoutToolSpec(const std::string& tool)
{
    static const LayoutToolSpec specs[] = {
        {
            "fill_bed",
            Bridge::ActionID::FILL_BED,
            "fill_bed",
            "Filling bed",
            "arranging",
            "Bed filled",
            "FILL_BED_FAILED",
            "Failed to fill bed."
        },
        {
            "arrange_current_plate",
            Bridge::ActionID::ARRANGE_SINGLE_PLATE,
            "arrange",
            "Arranging current plate",
            "arranging",
            "Current plate arranged",
            "ARRANGE_CURRENT_PLATE_FAILED",
            "Failed to arrange current plate."
        },
        {
            "arrange_all_plates",
            Bridge::ActionID::ARRANGE_ALL_PLATES,
            "arrange",
            "Arranging all plates",
            "arranging",
            "All plates arranged",
            "ARRANGE_ALL_PLATES_FAILED",
            "Failed to arrange all plates."
        },
    };

    for (const auto& spec : specs) {
        if (tool == spec.tool)
            return &spec;
    }

    return nullptr;
}

} // namespace

bool MCPChatPanel::TryHandleLayoutToolCall(const std::string& request_id,
                                           const std::string& tool,
                                           const json& args)
{
    if (tool == "auto_arrange") {
        if (!m_cxagent_bridge)
            return true;

        m_cxagent_bridge->MarkRequestStarted(request_id);
        m_cxagent_bridge->SendToolProgress(
            request_id,
            5,
            "Preparing auto arrange",
            "preparing",
            "running");

        CallAfter([this, request_id, args, tool]() {
            if (!m_cxagent_bridge)
                return;

            std::string scope = ToLowerCopy(ParseStringKey(args, {"scope", "target_scope", "mode"}));

            const bool has_object_targets =
                args.contains("object_index") || args.contains("object_indices") ||
                args.contains("object_name") || args.contains("object_names");
            const bool has_plate_target =
                args.contains("plate_number") || args.contains("plate") ||
                args.contains("plate_index");

            if (scope.empty()) {
                if (has_object_targets)
                    scope = "object";
                else if (has_plate_target)
                    scope = "plate";
                else
                    scope = "out_of_bounds";
            }

            json selection_result;
            json arrange_result;
            std::string source_action;

            if (scope == "all" || scope == "all_plates" || scope == "global") {
                arrange_result = Bridge::SlicerBridge::Instance().Execute(
                    Bridge::ActionID::ARRANGE_ALL_PLATES,
                    json::object());
                source_action = Bridge::ActionID::ARRANGE_ALL_PLATES;
            } else if (scope == "out_of_bounds" || scope == "outside" || scope == "outside_plate") {
                json state_result = Bridge::SlicerBridge::Instance().Execute(
                    Bridge::ActionID::GET_SLICER_STATE,
                    json::object());

                if (!state_result.value("success", false)) {
                    m_cxagent_bridge->SendToolResult(request_id, false, {
                        {"code", "AUTO_ARRANGE_STATE_FAILED"},
                        {"message", state_result.value("message", std::string("Failed to inspect scene before auto arrange"))},
                        {"details", state_result}
                    });
                    m_cxagent_bridge->MarkRequestFinished(request_id);
                    NotifyCxAgentStatus();
                    return;
                }

                json select_args = json::object();
                select_args["scope"] = "object";
                json target_indices = json::array();

                const json scene_state = state_result.value("state", json::object());
                if (scene_state.contains("objects") && scene_state["objects"].is_array()) {
                    for (const auto& object : scene_state["objects"]) {
                        if (!object.is_object())
                            continue;

                        const int plate_index = object.value("plate_index", -1);
                        if (plate_index < 0)
                            target_indices.push_back(object.value("object_index", -1));
                    }
                }

                if (target_indices.empty()) {
                    select_args = json::object();
                    select_args["scope"] = "plate";
                    if (scene_state.contains("current_plate_index"))
                        select_args["plate_index"] = scene_state.value("current_plate_index", -1);

                    m_cxagent_bridge->SendToolProgress(
                        request_id,
                        30,
                        "No detached out-of-bounds objects found, selecting the current plate instead",
                        "selecting",
                        "running");
                } else {
                    select_args["object_indices"] = target_indices;
                }

                selection_result = Bridge::SlicerBridge::Instance().Execute(
                    Bridge::ActionID::SELECT_OBJECTS,
                    select_args);

                if (!selection_result.value("success", false)) {
                    m_cxagent_bridge->SendToolResult(request_id, false, {
                        {"code", "AUTO_ARRANGE_SELECTION_FAILED"},
                        {"message", selection_result.value("message", std::string("Failed to select auto-arrange targets"))},
                        {"details", selection_result}
                    });
                    m_cxagent_bridge->MarkRequestFinished(request_id);
                    NotifyCxAgentStatus();
                    return;
                }

                m_cxagent_bridge->SendToolProgress(
                    request_id,
                    40,
                    "Out-of-bounds models selected for auto arrange",
                    "selecting",
                    "running");

                arrange_result = Bridge::SlicerBridge::Instance().Execute(
                    Bridge::ActionID::AUTO_ARRANGE,
                    json::object());
                source_action = Bridge::ActionID::AUTO_ARRANGE;
            } else if (scope == "plate" || scope == "current_plate" ||
                       scope == "object" || scope == "selected") {
                json select_args = args.is_object() ? args : json::object();
                select_args["scope"] =
                    (scope == "plate" || scope == "current_plate") ? "plate" : "object";

                selection_result = Bridge::SlicerBridge::Instance().Execute(
                    Bridge::ActionID::SELECT_OBJECTS,
                    select_args);

                if (!selection_result.value("success", false)) {
                    m_cxagent_bridge->SendToolResult(request_id, false, {
                        {"code", "AUTO_ARRANGE_SELECTION_FAILED"},
                        {"message", selection_result.value("message", std::string("Failed to select target objects for auto arrange"))},
                        {"details", selection_result}
                    });
                    m_cxagent_bridge->MarkRequestFinished(request_id);
                    NotifyCxAgentStatus();
                    return;
                }

                m_cxagent_bridge->SendToolProgress(
                    request_id,
                    40,
                    "Targets selected for auto arrange",
                    "selecting",
                    "running");

                arrange_result = Bridge::SlicerBridge::Instance().Execute(
                    Bridge::ActionID::AUTO_ARRANGE,
                    json::object());
                source_action = Bridge::ActionID::AUTO_ARRANGE;
            } else {
                m_cxagent_bridge->SendToolResult(request_id, false, {
                    {"code", "AUTO_ARRANGE_INVALID_ARGS"},
                    {"message", "Unsupported auto_arrange scope: " + scope},
                    {"details", args}
                });
                m_cxagent_bridge->MarkRequestFinished(request_id);
                NotifyCxAgentStatus();
                return;
            }

            if (!arrange_result.value("success", false)) {
                m_cxagent_bridge->SendToolResult(request_id, false, {
                    {"code", "AUTO_ARRANGE_FAILED"},
                    {"message", arrange_result.value("message", std::string("Failed to start auto arrange"))},
                    {"details", arrange_result}
                });
                m_cxagent_bridge->MarkRequestFinished(request_id);
                NotifyCxAgentStatus();
                return;
            }

            json result = {
                {"scope", scope},
                {"selection", selection_result},
                {"arrange", arrange_result},
                {"source_action", source_action},
                {"lifecycle", "async_pending"},
                {"requires_settle", true},
                {"requires_async_completion", true},
                {"async_completion", {
                    {"completion_key", "job:auto_arrange"},
                    {"completion_source", "ArrangeJob::finalize"},
                    {"job_type", "arrange"}
                }},
            };

            json state_result = Bridge::SlicerBridge::Instance().Execute(
                Bridge::ActionID::GET_SLICER_STATE,
                json::object());

            if (state_result.value("success", false))
                result["project_context"] = state_result.value("state", json::object());

            if (1) {
                RegisterPendingAsyncToolCall(request_id, tool, result, false);
                m_cxagent_bridge->SendToolProgress(
                    request_id,
                    65,
                    "Auto arrange is running in slicer",
                    "arranging",
                    "running");
                NotifyCxAgentStatus();
                return;
            }

            m_cxagent_bridge->SendToolProgress(
                request_id,
                100,
                "Auto arrange job started",
                "completed",
                "completed");

            m_cxagent_bridge->SendToolResult(request_id, true, result);
            m_cxagent_bridge->MarkRequestFinished(request_id);
            NotifyCxAgentStatus();
        });

        return true;
    }

    const LayoutToolSpec* spec = FindSimpleLayoutToolSpec(tool);
    if (!spec)
        return false;

    if (!m_cxagent_bridge)
        return true;

    const LayoutToolSpec selected = *spec;

    m_cxagent_bridge->MarkRequestStarted(request_id);
    m_cxagent_bridge->SendToolProgress(
        request_id,
        10,
        selected.progress_start_message,
        selected.progress_stage,
        "running");

    CallAfter([this, request_id, args, selected]() {
        if (!m_cxagent_bridge)
            return;

        json bridge_result = Bridge::SlicerBridge::Instance().Execute(
            selected.action_id,
            args.is_object() ? args : json::object());

        if (!bridge_result.value("success", false)) {
            m_cxagent_bridge->SendToolResult(request_id, false, {
                {"code", JsonStringValue(bridge_result, "code", selected.error_code)},
                {"message", JsonStringValue(bridge_result, "message", selected.error_message)},
                {"details", bridge_result}
            });
            m_cxagent_bridge->MarkRequestFinished(request_id);
            NotifyCxAgentStatus();
            return;
        }

        json result = {
            {selected.result_key, bridge_result},
            {"source_action", selected.action_id}
        };

        json state_result = Bridge::SlicerBridge::Instance().Execute(
            Bridge::ActionID::GET_SLICER_STATE,
            json::object());

        if (state_result.value("success", false))
            result["project_context"] = state_result.value("state", json::object());

        m_cxagent_bridge->SendToolProgress(
            request_id,
            100,
            selected.progress_done_message,
            "completed",
            "completed");

        m_cxagent_bridge->SendToolResult(request_id, true, result);
        m_cxagent_bridge->MarkRequestFinished(request_id);
        NotifyCxAgentStatus();
    });

    return true;
}

} // namespace GUI
} // namespace Slic3r
