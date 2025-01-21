#ifndef RemotePrint_Routes_hpp_
#define RemotePrint_Routes_hpp_
#include "nlohmann/json.hpp"
#include <string>

class wxWebView;
namespace DM{

    class Routes
    {
    public:
        Routes();
        bool Invoke(wxWebView* browse, const std::string& data);

    protected:
        std::unordered_map<std::string, std::function<bool(wxWebView*, const std::string&, const nlohmann::json&)>> m_handlers;
        void Handler(const std::string& command, std::function<bool(wxWebView*, const std::string&, const nlohmann::json&)> handler);
    };
}
#endif /* RemotePrint_DeviceDB_hpp_ */