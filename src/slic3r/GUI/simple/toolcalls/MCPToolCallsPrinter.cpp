#include "slic3r/GUI/simple/MCPChatPanel.hpp"

#include <boost/log/trivial.hpp>

using json = nlohmann::json;

namespace Slic3r {
namespace GUI {
namespace {

bool IsSelectPrinterTool(const std::string& tool)
{
    return tool == Bridge::ActionID::SELECT_PRINTER ||
           tool == "switch_printer" ||
           tool == "change_printer" ||
           tool == "select_device";
}


bool IsListPrintersTool(const std::string& tool)
{
    return tool == Bridge::ActionID::LIST_PRINTERS;
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

} // namespace

bool MCPChatPanel::TryHandlePrinterToolCall(const std::string& request_id,
                                            const std::string& tool,
                                            const json& args)
{
    if (!IsSelectPrinterTool(tool) && !IsListPrintersTool(tool))
        return false;

    if (!m_cxagent_bridge)
        return true;

    BOOST_LOG_TRIVIAL(info)
        << "[MCPChatPanel] handling printer tool_call request_id="
        << request_id
        << " tool="
        << tool
        << " args="
        << args.dump();

    const bool is_list_printers = IsListPrintersTool(tool);

    m_cxagent_bridge->MarkRequestStarted(request_id);
    m_cxagent_bridge->SendToolProgress(
        request_id,
        10,
        is_list_printers ? "Listing printers" : "Selecting printer",
        is_list_printers ? "listing" : "selecting",
        "running");

    CallAfter([this, request_id, args, is_list_printers]() {
        if (!m_cxagent_bridge)
            return;

        json bridge_result = Bridge::SlicerBridge::Instance().Execute(
            is_list_printers ? Bridge::ActionID::LIST_PRINTERS : Bridge::ActionID::SELECT_PRINTER,
            args.is_object() ? args : json::object());

        const bool success = bridge_result.value("success", false);
        if (!success) {
            const std::string code = JsonStringValue(
                bridge_result,
                "code",
                is_list_printers ? "LIST_PRINTERS_FAILED" : "SELECT_PRINTER_FAILED");

            const std::string message = JsonStringValue(
                bridge_result,
                "message",
                is_list_printers ? "Failed to list printers." : "Failed to select printer.");

            m_cxagent_bridge->SendToolResult(request_id, false, {
                {"code", code},
                {"message", message},
                {"details", bridge_result}
            });
            m_cxagent_bridge->MarkRequestFinished(request_id);
            NotifyCxAgentStatus();
            return;
        }

        if (!bridge_result.contains("source_action"))
            bridge_result["source_action"] = is_list_printers ? Bridge::ActionID::LIST_PRINTERS : Bridge::ActionID::SELECT_PRINTER;

        m_cxagent_bridge->SendToolProgress(
            request_id,
            100,
            is_list_printers ? "Printer list ready" : "Printer selected",
            "completed",
            "completed");

        m_cxagent_bridge->SendToolResult(request_id, true, bridge_result);
        m_cxagent_bridge->MarkRequestFinished(request_id);
        NotifyCxAgentStatus();
    });

    return true;
}

} // namespace GUI
} // namespace Slic3r
