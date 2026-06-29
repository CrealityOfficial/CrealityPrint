#include "slic3r/GUI/simple/MCPChatPanel.hpp"
#include "slic3r/GUI/simple/toolcalls/MCPToolCallsCommon.hpp"

#include <boost/log/trivial.hpp>

#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

using json = nlohmann::json;

namespace Slic3r {
namespace GUI {
namespace {

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

} // namespace

bool MCPChatPanel::TryHandleConfigToolCall(const std::string& request_id,
                                           const std::string& tool,
                                           const json& args)
{
    if (tool == "get_current_slice_params") {
        m_cxagent_bridge->MarkRequestStarted(request_id);
        m_cxagent_bridge->SendToolProgress(request_id, 5, "Collecting current slice parameters", "collecting");

        CallAfter([this, request_id, args]() {
            json bridge_result = Bridge::SlicerBridge::Instance().Execute(Bridge::ActionID::GET_EDITED_CONFIG, args);
            const bool success = bridge_result.value("success", false);
            if (!success) {
                m_cxagent_bridge->SendToolResult(request_id, false, {
                    {"code", "CURRENT_SLICE_PARAMS_FAILED"},
                    {"message", bridge_result.value("message", std::string("Failed to collect current slice parameters"))},
                    {"details", bridge_result}
                });
            } else {
                json slice_params = json::object();
                if (bridge_result.contains("config") && bridge_result["config"].is_object()) {
                    slice_params = bridge_result["config"];
                } else {
                    if (bridge_result.contains("print"))
                        slice_params["print"] = bridge_result["print"];
                    if (bridge_result.contains("filament"))
                        slice_params["filament"] = bridge_result["filament"];
                    if (bridge_result.contains("printer"))
                        slice_params["printer"] = bridge_result["printer"];
                }

                json result = {
                    {"current_slice_params", slice_params},
                    {"source_action", Bridge::ActionID::GET_EDITED_CONFIG}
                };
                if (bridge_result.contains("message"))
                    result["message"] = bridge_result["message"];

                m_cxagent_bridge->SendToolProgress(request_id, 100, "Current slice parameters collected", "completed", "completed");
                m_cxagent_bridge->SendToolResult(request_id, true, result);
            }

            m_cxagent_bridge->MarkRequestFinished(request_id);
            NotifyCxAgentStatus();
        });

        return true;
    }

    if (tool == "list_presets") {
        m_cxagent_bridge->MarkRequestStarted(request_id);
        m_cxagent_bridge->SendToolProgress(request_id, 5, "Collecting presets", "collecting");

        CallAfter([this, request_id, args]() {
            json bridge_result = Bridge::SlicerBridge::Instance().Execute(Bridge::ActionID::GET_PRESETS, args);
            const bool success = bridge_result.value("success", false);
            if (!success) {
                m_cxagent_bridge->SendToolResult(request_id, false, {
                    {"code", "LIST_PRESETS_FAILED"},
                    {"message", bridge_result.value("message", std::string("Failed to collect presets"))},
                    {"details", bridge_result}
                });
            } else {
                json presets = {
                    {"print_presets", bridge_result.value("print_presets", json::array())},
                    {"filament_presets", bridge_result.value("filament_presets", json::array())},
                    {"printer_presets", bridge_result.value("printer_presets", json::array())},
                    {"current_print", bridge_result.value("current_print", std::string())},
                    {"current_filament", bridge_result.value("current_filament", std::string())},
                    {"current_printer", bridge_result.value("current_printer", std::string())},
                };

                json result = {
                    {"presets", presets},
                    {"source_action", Bridge::ActionID::GET_PRESETS}
                };
                if (bridge_result.contains("message"))
                    result["message"] = bridge_result["message"];

                m_cxagent_bridge->SendToolProgress(request_id, 100, "Presets collected", "completed", "completed");
                m_cxagent_bridge->SendToolResult(request_id, true, result);
            }

            m_cxagent_bridge->MarkRequestFinished(request_id);
            NotifyCxAgentStatus();
        });

        return true;
    }

    if (tool == "get_config_schema") {
        m_cxagent_bridge->MarkRequestStarted(request_id);
        m_cxagent_bridge->SendToolProgress(request_id, 10, "Collecting config schema", "collecting");

        CallAfter([this, request_id, args]() {
            json bridge_result = Bridge::SlicerBridge::Instance().Execute(Bridge::ActionID::GET_CONFIG_OPTIONS, args);
            const bool success = bridge_result.value("success", false);
            if (!success) {
                m_cxagent_bridge->SendToolResult(request_id, false, {
                    {"code", "GET_CONFIG_SCHEMA_FAILED"},
                    {"message", bridge_result.value("message", std::string("Failed to collect config schema"))},
                    {"details", bridge_result}
                });
                m_cxagent_bridge->MarkRequestFinished(request_id);
                NotifyCxAgentStatus();
                return;
            }

            json result = {
                {"config_schema", bridge_result},
                {"source_action", Bridge::ActionID::GET_CONFIG_OPTIONS}
            };

            m_cxagent_bridge->SendToolProgress(request_id, 100, "Config schema collected", "completed", "completed");
            m_cxagent_bridge->SendToolResult(request_id, true, result);
            m_cxagent_bridge->MarkRequestFinished(request_id);
            NotifyCxAgentStatus();
        });

        return true;
    }

    if (tool == "apply_preset") {
        m_cxagent_bridge->MarkRequestStarted(request_id);
        m_cxagent_bridge->SendToolProgress(request_id, 5, "Applying preset", "applying");

        CallAfter([this, request_id, args]() {
            std::vector<std::pair<std::string, std::string>> preset_requests;

            const std::string direct_type = ParseStringKey(args, {"type", "preset_type"});
            const std::string direct_name = ParseStringKey(args, {"name", "preset_name"});
            if (!direct_type.empty() && !direct_name.empty())
                preset_requests.emplace_back(direct_type, direct_name);

            const std::string process_profile = ParseStringKey(args, {"process_profile", "print_preset", "print_profile", "process_preset"});
            if (!process_profile.empty())
                preset_requests.emplace_back("print", process_profile);

            const std::string material_profile = ParseStringKey(args, {"material_profile", "filament_preset", "filament_profile", "material_preset"});
            if (!material_profile.empty())
                preset_requests.emplace_back("filament", material_profile);

            const std::string printer_profile = ParseStringKey(args, {"printer_profile", "printer_preset", "machine_preset", "machine_profile"});
            if (!printer_profile.empty())
                preset_requests.emplace_back("printer", printer_profile);

            if (preset_requests.empty()) {
                m_cxagent_bridge->SendToolResult(request_id, false, {
                    {"code", "APPLY_PRESET_INVALID_ARGS"},
                    {"message", "No preset target was provided."},
                    {"details", args}
                });
                m_cxagent_bridge->MarkRequestFinished(request_id);
                NotifyCxAgentStatus();
                return;
            }

            json applied = json::array();
            for (const auto& preset_request : preset_requests) {
                json select_args = {
                    {"type", preset_request.first},
                    {"name", preset_request.second}
                };

                json select_result = Bridge::SlicerBridge::Instance().Execute(Bridge::ActionID::SELECT_PRESET, select_args);
                if (!select_result.value("success", false)) {
                    m_cxagent_bridge->SendToolResult(request_id, false, {
                        {"code", "APPLY_PRESET_FAILED"},
                        {"message", select_result.value("message", std::string("Failed to apply preset"))},
                        {"details", select_result}
                    });
                    m_cxagent_bridge->MarkRequestFinished(request_id);
                    NotifyCxAgentStatus();
                    return;
                }

                applied.push_back({
                    {"type", preset_request.first},
                    {"name", preset_request.second}
                });
            }

            json result = {
                {"applied_presets", applied},
                {"source_action", Bridge::ActionID::SELECT_PRESET}
            };

            json state_result = Bridge::SlicerBridge::Instance().Execute(Bridge::ActionID::GET_SLICER_STATE, json::object());
            if (state_result.value("success", false))
                result["project_context"] = state_result.value("state", json::object());

            m_cxagent_bridge->SendToolProgress(request_id, 100, "Preset applied", "completed", "completed");
            m_cxagent_bridge->SendToolResult(request_id, true, result);
            m_cxagent_bridge->MarkRequestFinished(request_id);
            NotifyCxAgentStatus();
        });

        return true;
    }

    if (tool == "apply_param_patch" || tool == "apply_config") {
        m_cxagent_bridge->MarkRequestStarted(request_id);
        m_cxagent_bridge->SendToolProgress(request_id, 5, "Applying parameter patch", "applying");

        CallAfter([this, request_id, args]() {
            const json normalized_args = ToolCalls::NormalizeApplyParamPatchArgs(args);
            if (normalized_args.empty()) {
                m_cxagent_bridge->SendToolResult(request_id, false, {
                    {"code", "APPLY_PARAM_PATCH_INVALID_ARGS"},
                    {"message", "No valid parameter patch entries were provided."},
                    {"details", args}
                });
                m_cxagent_bridge->MarkRequestFinished(request_id);
                NotifyCxAgentStatus();
                return;
            }

            BOOST_LOG_TRIVIAL(info)
                << "[MCPChatPanel] apply_param_patch request_id=" << request_id
                << " raw_args=" << args.dump()
                << " normalized_args=" << normalized_args.dump();

            json bridge_result = Bridge::SlicerBridge::Instance().Execute(Bridge::ActionID::APPLY_CONFIG, normalized_args);
            if (!bridge_result.value("success", false)) {
                m_cxagent_bridge->SendToolResult(request_id, false, {
                    {"code", "APPLY_PARAM_PATCH_FAILED"},
                    {"message", bridge_result.value("message", std::string("Failed to apply parameter patch"))},
                    {"details", bridge_result}
                });
                m_cxagent_bridge->MarkRequestFinished(request_id);
                NotifyCxAgentStatus();
                return;
            }

            json result = {
                {"applied_patch", normalized_args},
                {"raw_patch", args},
                {"changes", bridge_result.value("changes", json::array())},
                {"warnings", bridge_result.value("warnings", json::array())},
                {"source_action", Bridge::ActionID::APPLY_CONFIG}
            };

            json state_result = Bridge::SlicerBridge::Instance().Execute(Bridge::ActionID::GET_SLICER_STATE, json::object());
            if (state_result.value("success", false))
                result["project_context"] = state_result.value("state", json::object());

            m_cxagent_bridge->SendToolProgress(request_id, 100, "Parameter patch applied", "completed", "completed");
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
