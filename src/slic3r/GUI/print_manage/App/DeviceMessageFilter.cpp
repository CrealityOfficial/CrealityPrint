#include "DeviceMessageFilter.h"

#include "nlohmann/json.hpp"

#include <array>

namespace Slic3r {
namespace GUI {
namespace WebSocketProxy {

namespace {

constexpr std::array<const char*, 3> ALWAYS_FILTER_KEYS{
    "realTimeSpeed",
    "realTimeFlow",
    "usedMaterialLength"
};

constexpr std::array<const char*, 4> BACKGROUND_TELEMETRY_KEYS{
    "curPosition",
    "nozzleTemp",
    "bedTemp0",
    "boxTemp"
};

bool should_keep_all_messages(const DeviceDetailState& detail_state,
                              const DeviceIdentity& source_device)
{
    if (!detail_state.visible || detail_state.source != "lan")
        return false;

    if (detail_state.address.empty() || source_device.address.empty())
        return true;

    return detail_state.address == source_device.address;
}

} // namespace

DeviceMessageFilterResult FilterDeviceMessage(
    const DeviceDetailState& detail_state,
    const DeviceIdentity& source_device,
    std::string_view payload)
{
    try {
        nlohmann::json message = nlohmann::json::parse(
            payload.begin(), payload.end(), nullptr, false);
        if (!message.is_object())
            return {};

        bool changed = false;
        for (const char* key : ALWAYS_FILTER_KEYS) {
            if (message.contains(key)) {
                message.erase(key);
                changed = true;
            }
        }

        if (!should_keep_all_messages(detail_state, source_device)) {
            for (const char* key : BACKGROUND_TELEMETRY_KEYS) {
                if (message.contains(key)) {
                    message.erase(key);
                    changed = true;
                }
            }
        }

        if (!changed)
            return {};
        if (message.empty())
            return {DeviceMessageFilterAction::Drop, {}};

        return {
            DeviceMessageFilterAction::UseModified,
            message.dump()
        };
    } catch (...) {
        return {};
    }
}

} // namespace WebSocketProxy
} // namespace GUI
} // namespace Slic3r
