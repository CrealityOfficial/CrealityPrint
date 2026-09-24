#ifndef slic3r_WebSocketProxy_hpp_
#define slic3r_WebSocketProxy_hpp_

#include "DeviceMessageFilter.h"

#include <functional>
#include <memory>
#include <string>
#include <wx/string.h>
#include "nlohmann/json_fwd.hpp"

namespace Slic3r {
namespace GUI {

namespace WebSocketProxy {

/*
bool ShouldProxyAllWebSockets();
*/
wxString GetWebSocketProxyScript();

using ScriptSink = std::function<void(const std::string&)>;

class Manager final
{
public:
    Manager(ScriptSink script_sink);
    ~Manager();

    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;

    void HandleCommand(const nlohmann::json& payload);
    void SetDeviceDetailState(DeviceDetailState state);
    void Shutdown() noexcept;

private:
    struct Impl;
    std::shared_ptr<Impl> m_impl;
};

} // namespace WebSocketProxy

} // namespace GUI
} // namespace Slic3r

#endif /* slic3r_WebSocketProxy_hpp_ */
