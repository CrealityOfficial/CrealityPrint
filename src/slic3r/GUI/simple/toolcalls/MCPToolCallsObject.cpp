#include "slic3r/GUI/simple/MCPChatPanel.hpp"
#include "slic3r/GUI/simple/toolcalls/MCPToolCallsCommon.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "slic3r/GUI/Notebook.hpp"
#include "slic3r/GUI/GUI_App.hpp"

#include <boost/log/trivial.hpp>

#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_set>
#include <vector>

using json = nlohmann::json;

namespace Slic3r {
namespace GUI {
namespace {

struct ObjectToolSpec {
    std::string tool;
    std::string action_id;
    std::string result_key;
    std::string progress_start_message;
    std::string progress_stage;
    std::string progress_done_message;
    std::string error_code;
    std::string error_message;
    bool attach_facts = false;
};

std::string ToLowerCopy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
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

const ObjectToolSpec* FindObjectToolSpec(const std::string& tool)
{
    static const ObjectToolSpec specs[] = {
        {
            "select_objects",
            Bridge::ActionID::SELECT_OBJECTS,
            "selection",
            "Selecting objects",
            "selecting",
            "Objects selected",
            "SELECT_OBJECTS_FAILED",
            "Failed to select objects.",
            false
        },
        {
            "move_object",
            Bridge::ActionID::MOVE_OBJECT,
            "movement",
            "Moving model",
            "moving",
            "Model moved",
            "MOVE_MODEL_FAILED",
            "Failed to move model.",
            true
        },
        {
            "rotate_object",
            Bridge::ActionID::ROTATE_OBJECT,
            "rotation",
            "Rotating model",
            "rotating",
            "Model rotated",
            "ROTATE_MODEL_FAILED",
            "Failed to rotate model.",
            true
        },
        {
            "scale_object",
            Bridge::ActionID::SCALE_OBJECT,
            "scale",
            "Scaling model",
            "scaling",
            "Model scaled",
            "SCALE_MODEL_FAILED",
            "Failed to scale model.",
            false
        },
        {
            "delete_model",
            Bridge::ActionID::DELETE_MODEL,
            "deletion",
            "Deleting model",
            "deleting",
            "Model deleted",
            "DELETE_MODEL_FAILED",
            "Failed to delete model.",
            false
        },
        {
            "clone_model",
            Bridge::ActionID::CLONE_MODEL,
            "clone",
            "Cloning model",
            "cloning",
            "Model cloned",
            "CLONE_MODEL_FAILED",
            "Failed to clone model.",
            false
        },
        {
            "auto_orient_model",
            Bridge::ActionID::AUTO_ORIENT,
            "orient",
            "Starting auto orient",
            "orienting",
            "Auto orient completed",
            "AUTO_ORIENT_FAILED",
            "Failed to auto orient model.",
            false
        },
        {
            "repair_mesh",
            Bridge::ActionID::REPAIR_MESH,
            "repair",
            "Starting mesh repair",
            "repairing",
            "Mesh repair completed",
            "REPAIR_MESH_FAILED",
            "Failed to repair mesh.",
            false
        }

    };

    for (const auto& spec : specs) {
        if (tool == spec.tool)
            return &spec;
    }

    return nullptr;
}

} // namespace

bool MCPChatPanel::TryHandleObjectToolCall(const std::string& request_id,
                                           const std::string& tool,
                                           const json& args)
{
    const ObjectToolSpec* spec = FindObjectToolSpec(tool);
    if (!spec)
        return false;

    if (!m_cxagent_bridge)
        return true;

    // Check if we are on the prepare (3D editor) tab before executing model operations.
    // tp3DEditor corresponds to the "Prepare" page in the UI (tab index 2).
    // If mainframe/tabpanel is unavailable, skip the check and allow the operation.
    {
        auto* mainframe = wxGetApp().mainframe;
        if (mainframe && mainframe->m_tabpanel) {
            const int current_tab = static_cast<int>(mainframe->m_tabpanel->GetSelection());
            if (current_tab != MainFrame::tp3DEditor) {
                std::string current_tab_name;
                if (current_tab == MainFrame::tpPreview)          current_tab_name = "preview";
                else if (current_tab == MainFrame::tpOnlineModel) current_tab_name = "online_model";
                else if (current_tab == MainFrame::tpMonitor)     current_tab_name = "monitor";
                else if (current_tab == MainFrame::tpDeviceMgr)   current_tab_name = "device_mgr";
                else                                               current_tab_name = "unknown";

                BOOST_LOG_TRIVIAL(warning)
                    << "[MCPChatPanel] object tool_call blocked: not on prepare tab"
                    << " current_tab=" << current_tab
                    << " current_tab_name=" << current_tab_name
                    << " tool=" << tool;

                m_cxagent_bridge->MarkRequestStarted(request_id);
                m_cxagent_bridge->SendToolResult(request_id, false, {
                    {"code", "NOT_ON_PREPARE_TAB"},
                    {"current_tab", current_tab_name},
                    {"message", "Model operations are only available on the prepare page. Current page: " + current_tab_name}
                });
                m_cxagent_bridge->MarkRequestFinished(request_id);
                return true;
            }
        }
    }

    const ObjectToolSpec selected = *spec;

    BOOST_LOG_TRIVIAL(info)
        << "[MCPChatPanel] handling object tool_call request_id="
        << request_id
        << " tool="
        << tool
        << " args="
        << args.dump();

    m_cxagent_bridge->MarkRequestStarted(request_id);
    m_cxagent_bridge->SendToolProgress(
        request_id,
        10,
        selected.progress_start_message,
        selected.progress_stage,
        "running");

    CallAfter([this, request_id, args, selected, tool]() {
        if (!m_cxagent_bridge)
            return;

        json bridge_result = Bridge::SlicerBridge::Instance().Execute(
            selected.action_id,
            args.is_object() ? args : json::object());

        const bool success = bridge_result.value("success", false);
        if (!success) {
            const std::string code = JsonStringValue(
                bridge_result,
                "code",
                selected.error_code);

            const std::string message = JsonStringValue(
                bridge_result,
                "message",
                selected.error_message);

            m_cxagent_bridge->SendToolResult(request_id, false, {
                {"code", code},
                {"message", message},
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

        if (state_result.value("success", false)) {
            const json state = state_result.value("state", json::object());
            result["project_context"] = state;

            if (selected.attach_facts)
                ToolCalls::AttachExplicitFactsFromState(result, state);
        }
        const std::string lifecycle = ToLowerCopy(JsonStringValue(
            bridge_result,
            "lifecycle",
            std::string()));
        const bool is_async_completion =
            (bridge_result.contains("async_completion") || lifecycle == "async_pending" || bridge_result.value("requires_settle", false));
        if (is_async_completion) {
            result["lifecycle"] = bridge_result.value("lifecycle", std::string("async_pending"));
            if (bridge_result.contains("requires_settle"))
                result["requires_settle"] = bridge_result["requires_settle"];
            if (bridge_result.contains("requires_async_completion"))
                result["requires_async_completion"] = bridge_result["requires_async_completion"];
            if (bridge_result.contains("async_completion"))
                result["async_completion"] = bridge_result["async_completion"];
            RegisterPendingAsyncToolCall(request_id, tool, result, false);
            m_cxagent_bridge->SendToolProgress(
                request_id,
                65,
                selected.progress_done_message,
                selected.progress_stage,
                "running");
            NotifyCxAgentStatus();
            return;
        }

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
