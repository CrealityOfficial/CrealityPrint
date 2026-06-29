#include "slic3r/GUI/simple/MCPChatPanel.hpp"

#include <boost/log/trivial.hpp>

using json = nlohmann::json;

namespace Slic3r {
namespace GUI {
namespace {

bool IsModelSourceTool(const std::string& tool)
{
    return tool == "import_model" ||
           tool == "open_model_library" ||
           tool == "recommend_model" ||
           tool == "smart_model_search" ||
           tool == "import_model_from_search";
}

} // namespace

bool MCPChatPanel::TryHandleModelSourceToolCall(const std::string& request_id,
                                                const std::string& tool,
                                                const json& args)
{
    if (!IsModelSourceTool(tool))
        return false;

    if (!m_cxagent_bridge)
        return true;

    if (tool == "import_model") {
        m_cxagent_bridge->MarkRequestStarted(request_id);
        m_cxagent_bridge->SendToolProgress(request_id, 10, "Importing model", "importing");

        CallAfter([this, request_id, args]() {
            json bridge_result = Bridge::SlicerBridge::Instance().Execute(Bridge::ActionID::IMPORT_MODEL, args);
            if (!bridge_result.value("success", false)) {
                m_cxagent_bridge->SendToolResult(request_id, false, {
                    {"code", "IMPORT_MODEL_FAILED"},
                    {"message", bridge_result.value("message", std::string("Failed to import model"))},
                    {"details", bridge_result}
                });
                m_cxagent_bridge->MarkRequestFinished(request_id);
                NotifyCxAgentStatus();
                return;
            }

            json result = {
                {"import", bridge_result},
                {"source_action", Bridge::ActionID::IMPORT_MODEL}
            };

            json state_result = Bridge::SlicerBridge::Instance().Execute(Bridge::ActionID::GET_SLICER_STATE, json::object());
            if (state_result.value("success", false))
                result["project_context"] = state_result.value("state", json::object());

            m_cxagent_bridge->SendToolProgress(request_id, 100, "Model imported", "completed", "completed");
            m_cxagent_bridge->SendToolResult(request_id, true, result);
            m_cxagent_bridge->MarkRequestFinished(request_id);
            NotifyCxAgentStatus();
        });

        return true;
    }

    if (tool == "open_model_library") {
        m_cxagent_bridge->MarkRequestStarted(request_id);
        m_cxagent_bridge->SendToolProgress(request_id, 10, "Opening model library", "opening");

        CallAfter([this, request_id, args]() {
            json bridge_result = Bridge::SlicerBridge::Instance().Execute(Bridge::ActionID::OPEN_MODEL_LIBRARY, args);
            const bool success = bridge_result.value("success", false);
            if (success)
                m_cxagent_bridge->SendToolProgress(request_id, 100, "Model library opened", "completed", "completed");

            m_cxagent_bridge->SendToolResult(
                request_id,
                success,
                success ? bridge_result : json{
                    {"code", "OPEN_MODEL_LIBRARY_FAILED"},
                    {"message", bridge_result.value("message", std::string("Failed to open model library"))},
                    {"details", bridge_result}
                });

            m_cxagent_bridge->MarkRequestFinished(request_id);
            NotifyCxAgentStatus();
        });

        return true;
    }

    if (tool == "recommend_model") {
        m_cxagent_bridge->MarkRequestStarted(request_id);
        m_cxagent_bridge->SendToolProgress(request_id, 5, "Recommending online model", "recommending");

        CallAfter([this, request_id, args]() {
            json bridge_result = Bridge::SlicerBridge::Instance().Execute(Bridge::ActionID::RECOMMEND_MODEL, args);
            const bool success = bridge_result.value("success", false);
            if (success) {
                m_cxagent_bridge->SendToolProgress(
                    request_id,
                    100,
                    "Model recommendation completed",
                    "completed",
                    bridge_result.value("await_user_confirmation", false) ? "waiting" : "completed");
            }

            m_cxagent_bridge->SendToolResult(
                request_id,
                success,
                success ? bridge_result : json{
                    {"code", "RECOMMEND_MODEL_FAILED"},
                    {"message", bridge_result.value("message", std::string("Failed to recommend online model"))},
                    {"details", bridge_result}
                });

            m_cxagent_bridge->MarkRequestFinished(request_id);
            NotifyCxAgentStatus();
        });

        return true;
    }

    if (tool == "smart_model_search") {
        m_cxagent_bridge->MarkRequestStarted(request_id);
        m_cxagent_bridge->SendToolProgress(request_id, 5, "Searching model library", "searching");

        CallAfter([this, request_id, args]() {
            json bridge_result = Bridge::SlicerBridge::Instance().Execute(Bridge::ActionID::SMART_MODEL_SEARCH, args);
            const bool success = bridge_result.value("success", false);
            if (success) {
                m_cxagent_bridge->SendToolProgress(
                    request_id,
                    100,
                    "Model search completed",
                    "completed",
                    bridge_result.value("await_user_confirmation", false) ? "waiting" : "completed");
            }

            m_cxagent_bridge->SendToolResult(
                request_id,
                success,
                success ? bridge_result : json{
                    {"code", "SMART_MODEL_SEARCH_FAILED"},
                    {"message", bridge_result.value("message", std::string("Failed to search model library"))},
                    {"details", bridge_result}
                });

            m_cxagent_bridge->MarkRequestFinished(request_id);
            NotifyCxAgentStatus();
        });

        return true;
    }

    if (tool == "import_model_from_search") {
        m_cxagent_bridge->MarkRequestStarted(request_id);
        m_cxagent_bridge->SendToolProgress(request_id, 10, "Importing model from search", "importing");

        CallAfter([this, request_id, args]() {
            json bridge_result = Bridge::SlicerBridge::Instance().Execute(Bridge::ActionID::IMPORT_MODEL_FROM_SEARCH, args);
            const bool success = bridge_result.value("success", false);
            if (success)
                m_cxagent_bridge->SendToolProgress(request_id, 100, "Model import started", "completed", "completed");

            m_cxagent_bridge->SendToolResult(
                request_id,
                success,
                success ? bridge_result : json{
                    {"code", "IMPORT_MODEL_FROM_SEARCH_FAILED"},
                    {"message", bridge_result.value("message", std::string("Failed to import model from search"))},
                    {"details", bridge_result}
                });

            m_cxagent_bridge->MarkRequestFinished(request_id);
            NotifyCxAgentStatus();
        });

        return true;
    }

    return false;
}

} // namespace GUI
} // namespace Slic3r
