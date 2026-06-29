#include "slic3r/GUI/simple/MCPChatPanel.hpp"

#include <boost/log/trivial.hpp>

using json = nlohmann::json;

namespace Slic3r {
namespace GUI {

void MCPChatPanel::HandleCxAgentToolCall(const json& msg)
{
    if (!m_cxagent_bridge)
        return;

    const std::string request_id = msg.value("request_id", "");
    const std::string tool = msg.value("tool", "");
    const json args = msg.value("args", json::object());

    BOOST_LOG_TRIVIAL(warning)
        << "[MCPChatPanel] tool_call request_id=" << request_id
        << " tool=" << tool
        << " replay=" << (msg.value("replay", false) ? "true" : "false");

    if (request_id.empty()) {
        BOOST_LOG_TRIVIAL(warning)
            << "[MCPChatPanel] Ignored tool_call without request_id";
        return;
    }

    if(TryHandleSceneContextToolCall(request_id, tool, args))
        return;

    if(TryHandleConfigToolCall(request_id, tool, args))
        return;

    if(TryHandleModelSourceToolCall(request_id, tool, args))
        return;

    if (TryHandleObjectToolCall(request_id, tool, args))
        return;
    
    if(TryHandleLayoutToolCall(request_id, tool, args))
        return;

    if (TryHandleFilamentToolCall(request_id, tool, args))
        return;

    if (TryHandlePrinterToolCall(request_id, tool, args))
        return;

    if (TryHandlePrintToolCall(request_id, tool, args))
        return;

    m_cxagent_bridge->MarkRequestStarted(request_id);
    m_cxagent_bridge->SendToolResult(request_id, false, {
        {"code", "UNSUPPORTED_TOOL"},
        {"message", "Unsupported tool: " + tool}
    });
    m_cxagent_bridge->MarkRequestFinished(request_id);
    NotifyCxAgentStatus();

}

} // namespace GUI
} // namespace Slic3r
