#include "slic3r/GUI/simple/MCPChatPanel.hpp"
#include "slic3r/GUI/simple/bridge/SlicerBridge.hpp"

#include <boost/log/trivial.hpp>

using json = nlohmann::json;

namespace Slic3r {
namespace GUI {

bool MCPChatPanel::HandleAutoMapFilamentsToolCall(const std::string& request_id,
                                                  const json& args)
{
    if (!m_cxagent_bridge)
        return true;

    BOOST_LOG_TRIVIAL(warning)
        << "[MCPChatPanel] handling auto_map_filaments tool_call request_id="
        << request_id
        << " args="
        << args.dump();

    m_cxagent_bridge->MarkRequestStarted(request_id);
    m_cxagent_bridge->SendToolProgress(
        request_id,
        10,
        "Auto mapping filaments",
        "mapping",
        "running");

    CallAfter([this, request_id, args]() {
        if (!m_cxagent_bridge)
            return;

        json payload = args.is_object() ? args : json::object();
        json result = Bridge::SlicerBridge::Instance().Execute(
            Bridge::ActionID::AUTO_MAP_FILAMENTS,
            payload);

        const bool include_project_context =
            !payload.is_object() ||
            !payload.contains("include_project_context") ||
            payload.value("include_project_context", true);

        if (include_project_context) {
            json state_result = Bridge::SlicerBridge::Instance().Execute(
                Bridge::ActionID::GET_SLICER_STATE,
                json::object());
            if (state_result.value("success", false))
                result["project_context"] = state_result.value("state", json::object());
        }

        const bool success = result.value("success", false);
        const json summary = result.contains("summary") ? result["summary"] : json::object();
        BOOST_LOG_TRIVIAL(warning)
            << "[MCPChatPanel] auto_map_filaments result request_id="
            << request_id
            << " success="
            << success
            << " code="
            << result.value("code", std::string())
            << " summary="
            << summary.dump();

        if (success) {
            if (result.value("applied", false))
                RefreshAISendMappingForCurrentDevice(true);

            m_cxagent_bridge->SendToolProgress(
                request_id,
                100,
                "Filament mapping completed",
                "completed",
                "completed");
            m_cxagent_bridge->SendToolResult(request_id, true, result);
        } else {
            if (!result.contains("code"))
                result["code"] = "FILAMENT_MAPPING_FAILED";
            if (!result.contains("message"))
                result["message"] = "Automatic filament mapping failed.";

            m_cxagent_bridge->SendToolProgress(
                request_id,
                100,
                result.value("message", std::string("Filament mapping needs review")),
                result.value("mapping_panel_opened", false) ? "waiting_user_mapping" : "failed",
                result.value("mapping_panel_opened", false) ? "waiting" : "failed");
            m_cxagent_bridge->SendToolResult(request_id, false, result);
        }

        m_cxagent_bridge->MarkRequestFinished(request_id);
        NotifyCxAgentStatus();
    });

    return true;
}

} // namespace GUI
} // namespace Slic3r
