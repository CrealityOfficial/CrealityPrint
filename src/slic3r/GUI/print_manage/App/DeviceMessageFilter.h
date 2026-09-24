#ifndef slic3r_DeviceMessageFilter_h_
#define slic3r_DeviceMessageFilter_h_

#include <string>
#include <string_view>

namespace Slic3r {
namespace GUI {
namespace WebSocketProxy {

struct DeviceDetailState
{
    bool        visible{false};
    std::string source{"none"};
    std::string address;
    std::string mac;
};

struct DeviceIdentity
{
    std::string address;
    std::string mac;
};

enum class DeviceMessageFilterAction
{
    KeepOriginal,
    UseModified,
    Drop
};

struct DeviceMessageFilterResult
{
    DeviceMessageFilterAction action{DeviceMessageFilterAction::KeepOriginal};
    std::string               modified_payload;
};

DeviceMessageFilterResult FilterDeviceMessage(
    const DeviceDetailState& detail_state,
    const DeviceIdentity& source_device,
    std::string_view payload);

} // namespace WebSocketProxy
} // namespace GUI
} // namespace Slic3r

#endif /* slic3r_DeviceMessageFilter_h_ */
