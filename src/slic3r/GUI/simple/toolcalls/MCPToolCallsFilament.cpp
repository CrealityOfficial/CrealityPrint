#include "slic3r/GUI/simple/MCPChatPanel.hpp"

#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/Plater.hpp"

#include <boost/log/trivial.hpp>

#include <wx/event.h>

using json = nlohmann::json;

namespace Slic3r {
namespace GUI {
namespace {

bool IsFilamentTool(const std::string& tool)
{
    return tool == "open_filament_mapping" ||
           tool == Bridge::ActionID::AUTO_MAP_FILAMENTS ||
           tool == "auto_match_filaments" ||
           tool == "map_filaments" ||
           tool == "auto_map_materials" ||
           tool == "auto_match_materials" ||
           tool == "map_materials";
}

} // namespace

bool MCPChatPanel::TryHandleFilamentToolCall(const std::string& request_id,
                                             const std::string& tool,
                                             const json& args)
{
    if (!IsFilamentTool(tool))
        return false;

    if (tool != "open_filament_mapping")
        return HandleAutoMapFilamentsToolCall(request_id, args);

    if (!m_cxagent_bridge)
        return true;

    BOOST_LOG_TRIVIAL(info)
        << "[MCPChatPanel] handling filament tool_call request_id="
        << request_id
        << " tool="
        << tool
        << " args="
        << args.dump();

    m_cxagent_bridge->MarkRequestStarted(request_id);
    m_cxagent_bridge->SendToolProgress(
        request_id,
        5,
        "Opening filament mapping",
        "opening",
        "running");

    CallAfter([this, request_id]() {
        if (!m_cxagent_bridge)
            return;

        auto* plater = wxGetApp().plater();
        if (!plater) {
            m_cxagent_bridge->SendToolResult(request_id, false, {
                {"code", "FILAMENT_MAPPING_UNAVAILABLE"},
                {"message", "Plater is not available for filament mapping."}
            });
            m_cxagent_bridge->MarkRequestFinished(request_id);
            NotifyCxAgentStatus();
            return;
        }

        wxPostEvent(plater, wxCommandEvent(Slic3r::GUI::EVT_ON_MAPPING_DEVICE_FILAMENT));

        json result = {
            {"mapping_dialog_opened", true},
            {"source_action", "open_filament_mapping"}
        };

        json state_result = Bridge::SlicerBridge::Instance().Execute(
            Bridge::ActionID::GET_SLICER_STATE,
            json::object());

        if (state_result.value("success", false))
            result["project_context"] = state_result.value("state", json::object());

        m_cxagent_bridge->SendToolProgress(
            request_id,
            100,
            "Filament mapping opened",
            "completed",
            "waiting");

        m_cxagent_bridge->SendToolResult(request_id, true, result);
        m_cxagent_bridge->MarkRequestFinished(request_id);
        NotifyCxAgentStatus();
    });

    return true;
}

} // namespace GUI
} // namespace Slic3r
