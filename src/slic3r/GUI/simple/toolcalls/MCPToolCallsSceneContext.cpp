#include "slic3r/GUI/simple/MCPChatPanel.hpp"
#include "slic3r/GUI/simple/toolcalls/MCPToolCallsCommon.hpp"

#include <boost/log/trivial.hpp>

using json = nlohmann::json;

namespace Slic3r {
namespace GUI {

bool MCPChatPanel::TryHandleSceneContextToolCall(const std::string& request_id,
                                                 const std::string& tool,
                                                 const json& args)
{
    if (tool == "get_project_context") {
        m_cxagent_bridge->MarkRequestStarted(request_id);
        m_cxagent_bridge->SendToolProgress(request_id, 5, "Collecting project context", "collecting");

        CallAfter([this, request_id]() {
            json bridge_result = Bridge::SlicerBridge::Instance().Execute(Bridge::ActionID::GET_SLICER_STATE, json::object());
            if (!bridge_result.value("success", false)) {
                m_cxagent_bridge->SendToolResult(request_id, false, {
                    {"code", "PROJECT_CONTEXT_FAILED"},
                    {"message", bridge_result.value("message", std::string("Failed to collect project context"))},
                    {"details", bridge_result}
                });
            } else {
                json result = {
                    {"project_context", bridge_result.value("state", json::object())},
                    {"source_action", Bridge::ActionID::GET_SLICER_STATE}
                };
                if (bridge_result.contains("message"))
                    result["message"] = bridge_result["message"];

                m_cxagent_bridge->SendToolProgress(request_id, 100, "Project context collected", "completed", "completed");
                m_cxagent_bridge->SendToolResult(request_id, true, result);
            }

            m_cxagent_bridge->MarkRequestFinished(request_id);
            NotifyCxAgentStatus();
        });

        return true;
    }

    if (tool == "capture_model_views") {
        m_cxagent_bridge->MarkRequestStarted(request_id);
        m_cxagent_bridge->SendToolProgress(request_id, 10, "Capturing model views", "capturing");

        CallAfter([this, request_id, args]() {
            json bridge_result = Bridge::SlicerBridge::Instance().Execute(Bridge::ActionID::CAPTURE_MODEL_VIEWS, args);
            if (!bridge_result.value("success", false)) {
                m_cxagent_bridge->SendToolResult(request_id, false, {
                    {"code", "CAPTURE_MODEL_VIEWS_FAILED"},
                    {"message", bridge_result.value("message", std::string("Failed to capture model views"))},
                    {"details", bridge_result}
                });
                m_cxagent_bridge->MarkRequestFinished(request_id);
                NotifyCxAgentStatus();
                return;
            }

            json result = {
                {"views", bridge_result.value("views", json::array())},
                {"source_action", Bridge::ActionID::CAPTURE_MODEL_VIEWS}
            };
            if (bridge_result.contains("object_index"))
                result["object_index"] = bridge_result["object_index"];
            if (bridge_result.contains("object_name"))
                result["object_name"] = bridge_result["object_name"];

            m_cxagent_bridge->SendToolProgress(request_id, 100, "Model views captured", "completed", "completed");
            m_cxagent_bridge->SendToolResult(request_id, true, result);
            m_cxagent_bridge->MarkRequestFinished(request_id);
            NotifyCxAgentStatus();
        });

        return true;
    }

    if (tool == "analyze_model_geometry") {
        m_cxagent_bridge->MarkRequestStarted(request_id);
        m_cxagent_bridge->SendToolProgress(request_id, 5, "Collecting model geometry analysis", "collecting");

        CallAfter([this, request_id]() {
            json bridge_result = Bridge::SlicerBridge::Instance().Execute(Bridge::ActionID::GET_SLICER_STATE, json::object());
            if (!bridge_result.value("success", false)) {
                m_cxagent_bridge->SendToolResult(request_id, false, {
                    {"code", "MODEL_GEOMETRY_ANALYSIS_FAILED"},
                    {"message", bridge_result.value("message", std::string("Failed to analyze model geometry"))},
                    {"details", bridge_result}
                });
                m_cxagent_bridge->MarkRequestFinished(request_id);
                NotifyCxAgentStatus();
                return;
            }

            const json state = bridge_result.value("state", json::object());
            json result = {
                {"geometry_analysis", ToolCalls::BuildGeometryAnalysisFromState(state)},
                {"visual_geometry", ToolCalls::BuildVisualRecommendationGeometryFromState(state)},
                {"source_action", Bridge::ActionID::GET_SLICER_STATE}
            };
            if (bridge_result.contains("message"))
                result["message"] = bridge_result["message"];

            m_cxagent_bridge->SendToolProgress(request_id, 100, "Model geometry analysis collected", "completed", "completed");
            m_cxagent_bridge->SendToolResult(request_id, true, result);
            m_cxagent_bridge->MarkRequestFinished(request_id);
            NotifyCxAgentStatus();
        });

        return true;
    }

    return false;
}

} // namespace GUI
} // namespace Slic3r
