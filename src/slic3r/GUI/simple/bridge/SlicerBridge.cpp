#include "SlicerBridge.hpp"

#include <boost/log/trivial.hpp>
#include <cctype>
#include <sstream>
#include <stdexcept>

using json = nlohmann::json;

namespace {

std::string trim_copy(const std::string& value)
{
    size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin])))
        ++begin;

    size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])))
        --end;

    return value.substr(begin, end - begin);
}

json BuildTypeSchema(const std::string& raw_type)
{
    const std::string type = trim_copy(raw_type);
    if (type.empty())
        return { {"type", "string"} };

    const size_t pipe_pos = type.find('|');
    if (pipe_pos != std::string::npos) {
        json any_of = json::array();
        size_t start = 0;
        while (start <= type.size()) {
            const size_t next = type.find('|', start);
            const std::string part = trim_copy(type.substr(start, next == std::string::npos ? std::string::npos : next - start));
            if (!part.empty())
                any_of.push_back(BuildTypeSchema(part));
            if (next == std::string::npos)
                break;
            start = next + 1;
        }
        if (any_of.size() == 1)
            return any_of.front();
        return { {"anyOf", any_of} };
    }

    if (type.size() > 2 && type.compare(type.size() - 2, 2, "[]") == 0) {
        return {
            {"type", "array"},
            {"items", BuildTypeSchema(type.substr(0, type.size() - 2))}
        };
    }

    if (type.size() > 7 && type.compare(0, 6, "array[") == 0 && type.back() == ']') {
        return {
            {"type", "array"},
            {"items", BuildTypeSchema(type.substr(6, type.size() - 7))}
        };
    }

    if (type == "string")
        return { {"type", "string"} };
    if (type == "number" || type == "integer" || type == "int")
        return { {"type", "number"} };
    if (type == "bool" || type == "boolean")
        return { {"type", "boolean"} };
    if (type == "object")
        return { {"type", "object"} };

    return {
        {"type", "string"},
        {"x-original-type", type}
    };
}

json ParseDefaultValue(const Slic3r::GUI::Bridge::ParamDef& param)
{
    if (param.default_value.empty())
        return nullptr;

    const std::string type = trim_copy(param.type);
    const std::string value = trim_copy(param.default_value);
    if (type == "number" || type == "integer" || type == "int") {
        try {
            return std::stod(value);
        } catch (...) {
            return nullptr;
        }
    }

    if (type == "bool" || type == "boolean") {
        if (value == "true") return true;
        if (value == "false") return false;
        return nullptr;
    }

    if (type == "string")
        return value;

    return nullptr;
}

json BuildParamSchema(const Slic3r::GUI::Bridge::ParamDef& param)
{
    json schema = BuildTypeSchema(param.type);
    if (!param.description.empty())
        schema["description"] = param.description;

    json default_value = ParseDefaultValue(param);
    if (!default_value.is_null())
        schema["default"] = default_value;
    else if (!param.default_value.empty())
        schema["default_text"] = param.default_value;

    return schema;
}

bool IsDynamicParam(const Slic3r::GUI::Bridge::ParamDef& param)
{
    return param.name.size() >= 2 && param.name.front() == '<' && param.name.back() == '>';
}

json BuildInputSchema(const Slic3r::GUI::Bridge::ActionDef& action)
{
    json schema = {
        {"type", "object"},
        {"properties", json::object()}
    };

    json required = json::array();
    bool has_dynamic_properties = false;

    for (const auto& param : action.params) {
        if (IsDynamicParam(param)) {
            has_dynamic_properties = true;
            schema["additionalProperties"] = BuildParamSchema(param);
            if (!param.description.empty())
                schema["description"] = param.description;
            continue;
        }

        schema["properties"][param.name] = BuildParamSchema(param);
        if (param.required)
            required.push_back(param.name);
    }

    if (!required.empty())
        schema["required"] = required;

    if (!has_dynamic_properties)
        schema["additionalProperties"] = false;

    return schema;
}

json BuildToolItem(const Slic3r::GUI::Bridge::ActionDef& action)
{
    json item = {
        {"name", action.id},
        {"title", action.name_en.empty() ? action.id : action.name_en},
        {"description", action.description},
        {"requires_confirm", action.requires_confirm},
        {"inputSchema", BuildInputSchema(action)}
    };

    if (!action.name_zh.empty())
        item["title_zh"] = action.name_zh;

    return item;
}

} // namespace

namespace Slic3r {
namespace GUI {
namespace Bridge {

// ===========================================================================
// Singleton
// ===========================================================================

SlicerBridge& SlicerBridge::Instance()
{
    static SlicerBridge inst;
    return inst;
}

SlicerBridge::SlicerBridge()
{
    RegisterAllActions();
}

void SlicerBridge::SetSendToPrinterDelegate(void* owner, ActionExecutor delegate)
{
    m_send_to_printer_delegate_owner = owner;
    m_send_to_printer_delegate = std::move(delegate);
}

void SlicerBridge::ClearSendToPrinterDelegate(void* owner)
{
    if (m_send_to_printer_delegate_owner != owner)
        return;

    m_send_to_printer_delegate_owner = nullptr;
    m_send_to_printer_delegate = nullptr;
}

// ===========================================================================
// Registration helpers
// ===========================================================================

void SlicerBridge::RegisterAction(ActionDef def, ActionExecutor executor)
{
    m_executors[def.id] = std::move(executor);
    m_actions.push_back(std::move(def));
}


// ===========================================================================
// Registry queries
// ===========================================================================

const ActionDef* SlicerBridge::FindAction(const std::string& id) const
{
    for (const auto& a : m_actions)
        if (a.id == id) return &a;
    return nullptr;
}

json SlicerBridge::GetActionListJSON() const
{
    json arr = json::array();
    for (const auto& a : m_actions) {
        json item;
        item["id"]               = a.id;
        item["name_zh"]          = a.name_zh;
        item["name_en"]          = a.name_en;
        item["description"]      = a.description;
        item["requires_confirm"] = a.requires_confirm;

        json params = json::array();
        for (const auto& p : a.params) {
            params.push_back({
                {"name", p.name}, {"type", p.type},
                {"description", p.description},
                {"required", p.required},
                {"default", p.default_value}
            });
        }
        item["params"] = params;
        arr.push_back(item);
    }
    return arr;
}

json SlicerBridge::GetAvailableToolsJSON() const
{
    json tools = json::array();
    for (const auto& action : m_actions)
        tools.push_back(BuildToolItem(action));
    return tools;
}

std::string SlicerBridge::GenerateSystemPrompt() const
{
    std::ostringstream ss;
    ss << "You are an AI assistant embedded in CrealityPrint 3D slicer software.\n"
       << "You can query slicer state and execute slicer operations.\n\n"
       << "## Available Actions\n\n"
       << "When you decide to execute a slicer operation, output an action block in your reply:\n"
       << "[ACTION]{\"command\":\"<action_id>\", \"data\":{...}}[/ACTION]\n\n"
       << "IMPORTANT RULES:\n"
       << "- Only output an [ACTION] block when the user explicitly requests an operation.\n"
       << "- For queries (get_presets, get_slicer_state, get_edited_config), output the action block and wait for the result.\n"
       << "- Always explain what you are doing before the action block.\n"
       << "- You may output multiple action blocks in a single reply if needed.\n\n"
       << "### Action Catalogue\n\n";

    for (const auto& a : m_actions) {
        ss << "**" << a.id << "** - " << a.description << "\n";
        if (!a.params.empty()) {
            ss << "  Parameters:\n";
            for (const auto& p : a.params) {
                ss << "  - `" << p.name << "` (" << p.type << ")"
                   << (p.required ? " [required]" : " [optional]")
                   << ": " << p.description;
                if (!p.default_value.empty())
                    ss << " (default: " << p.default_value << ")";
                ss << "\n";
            }
        }
        ss << "\n";
    }

    ss << "## Response Guidelines\n\n"
       << "- Reply in the same language as the user (Chinese or English).\n"
       << "- Be concise and practical for 3D printing.\n"
       << "- When recommending parameters, explain why.\n"
       << "- If the user asks to slice but no model is loaded, ask for a local model file path to import or direct them to the model library.\n"
       << "- `get_slicer_state` returns rich scene analysis including per-model geometry, overhang detection, "
       << "filament info, wipe tower status, inter-model distances, settings summary, and auto-generated warnings.\n"
       << "- When the scene state contains a `warnings` array, PROACTIVELY mention these warnings to the user "
       << "and suggest corrective actions (e.g. enable support for high overhang, enable wipe tower for multi-material).\n"
       << "- `overhang.unsupported_ratio` is a lightweight pre-analysis signal for unsupported downward-facing area; use it only for initial parameter, adhesion, and orientation suggestions.\n"
       << "- `inter_model.too_close` is true when models are within 2mm of each other (collision risk).\n";

    return ss.str();
}

// ===========================================================================
// Execution dispatcher
// ===========================================================================

json SlicerBridge::Execute(const std::string& action_id, const json& params)
{
    BOOST_LOG_TRIVIAL(info) << "[SlicerBridge] Execute: action_id=" << action_id << " params=" << params.dump();
    auto it = m_executors.find(action_id);
    if (it == m_executors.end()) {
        BOOST_LOG_TRIVIAL(warning) << "[SlicerBridge] Unknown action: " << action_id;
        return {{"success", false}, {"action", action_id},
                {"message", "Unknown action: " + action_id}};
    }

    try {
        BOOST_LOG_TRIVIAL(info) << "[SlicerBridge] Execute: " << action_id
                                << " params=" << params.dump();
        json result = it->second(params);
        result["action"] = action_id;
        return result;
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "[SlicerBridge] Execute " << action_id
                                 << " error: " << e.what();
        return {{"success", false}, {"action", action_id}, {"message", e.what()}};
    }
}


} // namespace Bridge
} // namespace GUI
} // namespace Slic3r


