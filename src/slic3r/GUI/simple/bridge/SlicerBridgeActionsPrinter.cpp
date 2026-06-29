#include "SlicerBridge.hpp"

#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/GUI_ObjectList.hpp"
#include "slic3r/GUI/simple/DeviceListSimple.hpp"
#include "slic3r/GUI/format.hpp"
#include "slic3r/GUI/print_manage/data/DataCenter.hpp"
#include "libslic3r/PresetBundle.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace Slic3r {
namespace GUI {
namespace Bridge {
namespace {

std::string trim_copy(std::string value)
{
    auto is_ws = [](unsigned char ch) { return std::isspace(ch) != 0; };
    while (!value.empty() && is_ws(static_cast<unsigned char>(value.front())))
        value.erase(value.begin());
    while (!value.empty() && is_ws(static_cast<unsigned char>(value.back())))
        value.pop_back();
    return value;
}

std::string lower_copy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string normalized_id(std::string value)
{
    value = lower_copy(trim_copy(value));
    std::string out;
    out.reserve(value.size());
    for (unsigned char ch : value) {
        if (ch == ':' || ch == '-' || std::isspace(ch))
            continue;
        out.push_back(static_cast<char>(ch));
    }
    return out;
}

bool contains_ci(const std::string& haystack, const std::string& needle)
{
    const std::string h = lower_copy(haystack);
    const std::string n = lower_copy(trim_copy(needle));
    return !n.empty() && h.find(n) != std::string::npos;
}

bool contains_normalized(const std::string& haystack, const std::string& needle)
{
    const std::string h = normalized_id(haystack);
    const std::string n = normalized_id(needle);
    return !n.empty() && h.find(n) != std::string::npos;
}


bool matches_device_identity(const std::string& key,
                             const Simple_Device_List_Item_Data& item,
                             const std::string& target_device_id)
{
    if (target_device_id.empty())
        return true;

    const std::string target = normalized_id(target_device_id);
    return normalized_id(key) == target ||
           normalized_id(item.mac) == target ||
           normalized_id(item.address) == target ||
           normalized_id(item.name) == target;
}

bool matches_device_name(const std::string& key,
                         const Simple_Device_List_Item_Data& item,
                         const std::string& target_name)
{
    if (target_name.empty())
        return true;

    const std::string model_with_vendor = "Creality " + item.model_name;
    return contains_ci(item.name, target_name) ||
           contains_ci(key, target_name) ||
           contains_ci(item.model_name, target_name) ||
           contains_ci(model_with_vendor, target_name) ||
           contains_normalized(item.name, target_name) ||
           contains_normalized(key, target_name) ||
           contains_normalized(item.model_name, target_name) ||
           contains_normalized(model_with_vendor, target_name);
}

bool matches_device_model(const Simple_Device_List_Item_Data& item,
                          const std::string& target_model)
{
    if (target_model.empty())
        return true;

    const std::string model_with_vendor = "Creality " + item.model_name;
    return contains_ci(item.model_name, target_model) ||
           contains_ci(model_with_vendor, target_model) ||
           contains_normalized(item.model_name, target_model) ||
           contains_normalized(model_with_vendor, target_model);
}

std::string get_string_arg(const json& params, std::initializer_list<const char*> keys)
{
    for (const char* key : keys) {
        if (!params.contains(key))
            continue;
        const auto& value = params[key];
        if (value.is_string())
            return trim_copy(value.get<std::string>());
        if (value.is_number_integer())
            return std::to_string(value.get<long long>());
        if (value.is_number_unsigned())
            return std::to_string(value.get<unsigned long long>());
    }
    return "";
}

bool get_bool_arg(const json& params, const char* key, bool fallback)
{
    if (!params.contains(key))
        return fallback;

    const auto& value = params[key];
    if (value.is_boolean())
        return value.get<bool>();

    if (value.is_number_integer())
        return value.get<int>() != 0;

    if (value.is_string()) {
        const std::string text = lower_copy(trim_copy(value.get<std::string>()));
        if (text == "true" || text == "1" || text == "yes" || text == "on")
            return true;
        if (text == "false" || text == "0" || text == "no" || text == "off")
            return false;
    }

    return fallback;
}

json serialize_device_candidate(const std::string& key, const Simple_Device_List_Item_Data& item)
{
    return {
        {"device_key", key},
        {"device_id", key},
        {"name", item.name},
        {"model", item.model_name},
        {"address", item.address},
        {"ip", item.address},
        {"mac", item.mac},
        {"online", item.online},
        {"state", item.state},
        {"idle", item.online && item.state == 0},
        {"is_current", item.isCurrent},
        {"device_type", item.device_type},
        {"visible", item.visible}
    };
}

bool preset_exists(const std::string& preset_name)
{
    auto* bundle = wxGetApp().preset_bundle;
    if (!bundle || preset_name.empty())
        return false;

    for (const auto& preset : bundle->printers.get_presets()) {
        if (preset.name == preset_name)
            return true;
    }
    return false;
}

void refresh_printer_config_ui()
{
    auto* bundle = wxGetApp().preset_bundle;
    auto* plater = wxGetApp().plater();
    if (bundle && plater)
        plater->on_config_change(bundle->full_config());

    if (wxGetApp().obj_list())
        wxGetApp().obj_list()->update_object_list_by_printer_technology();
}

} // namespace

json SlicerBridge::DoMovePrintHead(const json& params)
{
    auto* mainframe = wxGetApp().mainframe;
    if (!mainframe)
        return {{"success", false}, {"code", "MOVE_PRINT_HEAD_MAINFRAME_UNAVAILABLE"}, {"message", "MainFrame is not available."}};
    auto* printer_mgr_view = mainframe->get_printer_mgr_view();
    if (!printer_mgr_view)
        return {{"success", false}, {"code", "MOVE_PRINT_HEAD_MANAGER_UNAVAILABLE"}, {"message", "Printer manager view is not available."}};
    const bool has_x = params.contains("x") && params["x"].is_number();
    const bool has_y = params.contains("y") && params["y"].is_number();
    const bool has_z = params.contains("z") && params["z"].is_number();
    if (!has_x && !has_y && !has_z)
        return {{"success", false}, {"code", "MOVE_PRINT_HEAD_MISSING_TARGET"}, {"message", "move_print_head requires at least one axis target: x, y, or z."}};
    const DM::Device& current_device = DM::DataCenter::Ins().get_current_device_data();
    if (!current_device.valid || current_device.address.empty())
        return {{"success", false}, {"code", "MOVE_PRINT_HEAD_DEVICE_UNAVAILABLE"}, {"message", "Current printer is not available."}};
    json target_pose = json::object();
    if (has_x) target_pose["x"] = params["x"].get<double>();
    if (has_y) target_pose["y"] = params["y"].get<double>();
    if (has_z) target_pose["z"] = params["z"].get<double>();
    json payload = {
        {"command", ActionID::MOVE_PRINT_HEAD},
        {"coordinate_mode", "absolute"},
        {"target_pose", target_pose},
        {"device", {
            {"name", current_device.name},
            {"address", current_device.address},
            {"mac", current_device.mac},
            {"tb_id", current_device.tbId},
            {"device_type", current_device.deviceType},
            {"old_printer", current_device.oldPrinter},
            {"model_name", current_device.modelName},
            {"online", current_device.online}
        }}
    };
    if (params.contains("request_id") && params["request_id"].is_string()) payload["request_id"] = params["request_id"].get<std::string>();
    if (params.contains("speed_mm_per_min") && params["speed_mm_per_min"].is_number()) payload["speed_mm_per_min"] = params["speed_mm_per_min"].get<double>();
    if (!printer_mgr_view->request_move_print_head(payload))
        return {{"success", false}, {"code", "MOVE_PRINT_HEAD_DISPATCH_FAILED"}, {"message", "Failed to dispatch move_print_head to device manager."}};
    json result = {
        {"success", true},
        {"accepted", true},
        {"source_action", ActionID::MOVE_PRINT_HEAD},
        {"message", "move_print_head request accepted."},
        {"target_pose", target_pose}
    };
    if (payload.contains("request_id")) result["request_id"] = payload["request_id"];
    if (payload.contains("speed_mm_per_min")) result["speed_mm_per_min"] = payload["speed_mm_per_min"];
    return result;
}

json SlicerBridge::DoPrintControl(const json& params)
{
    auto* mainframe = wxGetApp().mainframe;
    if (!mainframe)
        return {{"success", false}, {"code", "PRINT_CONTROL_MAINFRAME_UNAVAILABLE"}, {"message", "MainFrame is not available."}};
    auto* printer_mgr_view = mainframe->get_printer_mgr_view();
    if (!printer_mgr_view)
        return {{"success", false}, {"code", "PRINT_CONTROL_MANAGER_UNAVAILABLE"}, {"message", "Printer manager view is not available."}};

    const std::string action = lower_copy(get_string_arg(params, {"action", "control_action"}));
    if (action != "pause" && action != "resume" && action != "stop")
        return {{"success", false}, {"code", "PRINT_CONTROL_INVALID_ACTION"}, {"message", "print_control action must be pause, resume, or stop."}};

    const DM::Device& current_device = DM::DataCenter::Ins().get_current_device_data();
    if (!current_device.valid || current_device.address.empty())
        return {{"success", false}, {"code", "PRINT_CONTROL_DEVICE_UNAVAILABLE"}, {"message", "Current printer is not available."}};

    json payload = {
        {"command", ActionID::PRINT_CONTROL},
        {"action", action},
        {"device", {
            {"name", current_device.name},
            {"address", current_device.address},
            {"mac", current_device.mac},
            {"tb_id", current_device.tbId},
            {"tbId", current_device.tbId},
            {"device_type", current_device.deviceType},
            {"deviceType", current_device.deviceType},
            {"old_printer", current_device.oldPrinter},
            {"oldPrinter", current_device.oldPrinter},
            {"model_name", current_device.modelName},
            {"modelName", current_device.modelName},
            {"online", current_device.online}
        }}
    };
    if (params.contains("request_id") && params["request_id"].is_string())
        payload["request_id"] = params["request_id"].get<std::string>();
    else if (params.contains("requestId") && params["requestId"].is_string())
        payload["request_id"] = params["requestId"].get<std::string>();

    if (!printer_mgr_view->request_print_control(payload))
        return {{"success", false}, {"code", "PRINT_CONTROL_DISPATCH_FAILED"}, {"message", "Failed to dispatch print_control to device manager."}};

    json result = {
        {"success", true},
        {"accepted", true},
        {"source_action", ActionID::PRINT_CONTROL},
        {"action", action},
        {"message", "print_control request accepted."}
    };
    if (payload.contains("request_id")) result["request_id"] = payload["request_id"];
    return result;
}
json SlicerBridge::DoListPrinters(const json& params)
{
    const std::string target_mac = get_string_arg(params, {"mac", "printer_mac"});
    const std::string target_ip = get_string_arg(params, {"ip", "address", "printer_ip"});
    const std::string target_device_id = get_string_arg(params, {"device_id", "deviceId", "id"});
    const std::string target_name = get_string_arg(params, {"printer_name", "device_name", "name"});
    const std::string target_model = get_string_arg(params, {"printer_model", "model", "model_name"});

    const bool online_only = get_bool_arg(params, "online_only", false);
    const bool idle_only = get_bool_arg(params, "idle_only", false);
    const bool visible_only = get_bool_arg(params, "visible_only", true);

    std::size_t limit = 20;
    if (params.contains("limit")) {
        const auto& raw_limit = params["limit"];
        if (raw_limit.is_number_unsigned()) {
            limit = static_cast<std::size_t>(raw_limit.get<unsigned long long>());
        } else if (raw_limit.is_number_integer()) {
            const long long value = raw_limit.get<long long>();
            if (value > 0)
                limit = static_cast<std::size_t>(value);
        }
    }
    if (limit == 0)
        limit = 20;

    const auto& device_list = SimpleDeviceMgr::instance().get_device_list_data_simple(true);
    json items = json::array();
    std::size_t total_matches = 0;

    for (const auto& entry : device_list.datas) {
        const std::string& key = entry.first;
        const Simple_Device_List_Item_Data& item = entry.second;

        if (visible_only && !item.visible)
            continue;
        if (online_only && !item.online)
            continue;
        if (idle_only && !(item.online && item.state == 0))
            continue;
        if (!target_mac.empty() && normalized_id(item.mac) != normalized_id(target_mac))
            continue;
        if (!target_ip.empty() && trim_copy(item.address) != trim_copy(target_ip))
            continue;
        if (!matches_device_identity(key, item, target_device_id))
            continue;
        if (!matches_device_name(key, item, target_name))
            continue;
        if (!matches_device_model(item, target_model))
            continue;

        ++total_matches;
        if (items.size() < limit)
            items.push_back(serialize_device_candidate(key, item));
    }

    return {
        {"success", true},
        {"message", total_matches == 0 ? _u8L("No printers matched the requested filters.") : _u8L("Listed matching printers.")},
        {"source_action", ActionID::LIST_PRINTERS},
        {"items", items},
        {"total", total_matches},
        {"returned", items.size()},
        {"has_more", total_matches > items.size()},
        {"filters", {
            {"printer_model", target_model},
            {"printer_name", target_name},
            {"device_id", target_device_id},
            {"ip", target_ip},
            {"online_only", online_only},
            {"idle_only", idle_only},
            {"visible_only", visible_only},
            {"limit", limit}
        }}
    };
}

json SlicerBridge::DoSelectPrinter(const json& params)
{
    auto* mainframe = wxGetApp().mainframe;
    if (!mainframe)
        return {
            {"success", false},
            {"code", "SELECT_PRINTER_MAINFRAME_UNAVAILABLE"},
            {"message", "MainFrame is not available."}
        };

    auto* printer_mgr_view = mainframe->get_printer_mgr_view();
    if (!printer_mgr_view)
        return {
            {"success", false},
            {"code", "SELECT_PRINTER_MANAGER_UNAVAILABLE"},
            {"message", "Printer manager view is not available."}
        };

    const std::string target_mac = get_string_arg(params, {"mac", "printer_mac"});
    const std::string target_ip = get_string_arg(params, {"ip", "address", "printer_ip"});
    const std::string target_device_id = get_string_arg(params, {"device_id", "deviceId", "id"});
    const std::string target_name = get_string_arg(params, {"printer_name", "device_name", "name"});
    const std::string target_model = get_string_arg(params, {"printer_model", "model", "model_name"});
    const std::string requested_preset = get_string_arg(params, {"preset_name", "printer_preset", "printer_profile"});

    const bool sync_printer_preset = get_bool_arg(params, "sync_printer_preset", true);
    const bool require_online = get_bool_arg(params, "require_online", false);
    const bool prefer_online = get_bool_arg(params, "prefer_online", true);
    const bool pick_first_match =
        get_bool_arg(params, "pick_first_match", false) ||
        get_bool_arg(params, "select_first_match", false) ||
        get_bool_arg(params, "first_match", false);

    const bool has_device_selector =
        !target_mac.empty() ||
        !target_ip.empty() ||
        !target_device_id.empty() ||
        !target_name.empty() ||
        !target_model.empty();

    if (!has_device_selector && requested_preset.empty()) {
        return {
            {"success", false},
            {"code", "SELECT_PRINTER_MISSING_TARGET"},
            {"message", "select_printer requires at least one target: mac, ip, device_id, printer_name, printer_model, or preset_name."}
        };
    }

    if (!requested_preset.empty() && !preset_exists(requested_preset)) {
        return {
            {"success", false},
            {"code", "SELECT_PRINTER_PRESET_NOT_FOUND"},
            {"message", "Printer preset not found: " + requested_preset},
            {"preset_name", requested_preset}
        };
    }

    std::string selected_key;
    Simple_Device_List_Item_Data selected_item;
    bool selected_item_valid = false;

    if (has_device_selector) {
        const auto& device_list = SimpleDeviceMgr::instance().get_device_list_data_simple(true);
        std::vector<std::pair<std::string, Simple_Device_List_Item_Data>> matches;

        for (const auto& entry : device_list.datas) {
            const std::string& key = entry.first;
            const Simple_Device_List_Item_Data& item = entry.second;

            if (!item.visible)
                continue;

            bool matched = true;

            if (!target_mac.empty())
                matched = matched && normalized_id(item.mac) == normalized_id(target_mac);

            if (!target_ip.empty())
                matched = matched && trim_copy(item.address) == trim_copy(target_ip);

            matched = matched && matches_device_identity(key, item, target_device_id);
            matched = matched && matches_device_name(key, item, target_name);
            matched = matched && matches_device_model(item, target_model);

            if (matched)
                matches.push_back(entry);
        }

        if (matches.empty()) {
            return {
                {"success", false},
                {"code", "SELECT_PRINTER_NOT_FOUND"},
                {"message", "No printer matched the requested target."},
                {"target", params}
            };
        }

        if (prefer_online) {
            std::vector<std::pair<std::string, Simple_Device_List_Item_Data>> online_matches;
            for (const auto& match : matches) {
                if (match.second.online)
                    online_matches.push_back(match);
            }
            if (!online_matches.empty())
                matches = online_matches;
        }

        if (matches.size() > 1 && !pick_first_match) {
            json candidates = json::array();
            for (const auto& match : matches)
                candidates.push_back(serialize_device_candidate(match.first, match.second));

            return {
                {"success", false},
                {"code", "SELECT_PRINTER_AMBIGUOUS"},
                {"message", "Multiple printers matched the requested target."},
                {"candidates", candidates}
            };
        }

        selected_key = matches.front().first;
        selected_item = matches.front().second;
        selected_item_valid = true;

        if (require_online && !selected_item.online) {
            return {
                {"success", false},
                {"code", "SELECT_PRINTER_OFFLINE"},
                {"message", "The matched printer is offline: " + selected_item.name},
                {"selected_device", serialize_device_candidate(selected_key, selected_item)}
            };
        }

        if (!SimpleDeviceMgr::instance().switch_current_device_simple(selected_item.mac, sync_printer_preset)) {
            return {
                {"success", false},
                {"code", "SELECT_PRINTER_SWITCH_FAILED"},
                {"message", "Failed to switch current printer: " + selected_item.name},
                {"selected_device", serialize_device_candidate(selected_key, selected_item)}
            };
        }
    }

    std::string selected_preset = requested_preset;

    if (!requested_preset.empty()) {
        json preset_result = DoSelectPreset({
            {"type", "printer"},
            {"name", requested_preset}
        });

        if (!preset_result.value("success", false)) {
            return {
                {"success", false},
                {"code", "SELECT_PRINTER_PRESET_SELECT_FAILED"},
                {"message", preset_result.value("message", std::string("Failed to select printer preset."))},
                {"preset_name", requested_preset},
                {"details", preset_result}
            };
        }

        refresh_printer_config_ui();
        selected_preset = requested_preset;
    }

    json state_result = DoGetSlicerState(json::object());

    auto* bundle = wxGetApp().preset_bundle;
    std::string current_printer_preset;
    if (bundle)
        current_printer_preset = bundle->printers.get_edited_preset().name;
    if (selected_preset.empty())
        selected_preset = current_printer_preset;

    json result = {
        {"success", true},
        {"message", selected_item_valid ? format(_u8L("Selected printer: %1%"), selected_item.name) : format(_u8L("Selected printer preset: %1%"), selected_preset)},
        {"source_action", ActionID::SELECT_PRINTER},
        {"selected_printer_preset", selected_preset},
        {"invalidates", json::array({
            "plate.current.slice_ready_for_print",
            "plate.current.gcode_available",
            "scene.filament_mapping.valid"
        })}
    };

    if (selected_item_valid)
        result["selected_device"] = serialize_device_candidate(selected_key, selected_item);

    if (!current_printer_preset.empty())
        result["current_printer_preset"] = current_printer_preset;

    if (state_result.value("success", false))
        result["project_context"] = state_result.value("state", json::object());

    return result;
}

} // namespace Bridge
} // namespace GUI
} // namespace Slic3r
