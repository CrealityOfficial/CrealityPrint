#ifndef RemotePrint_AppRoutes_hpp_
#define RemotePrint_AppRoutes_hpp_
#include "nlohmann/json.hpp"
#include <string>

class wxWebView;
namespace DM{

    class AppRoutes
    {
    public:
        AppRoutes();
        bool Invoke(wxWebView* browse, const std::string& data);
    };

#endif /* RemotePrint_DeviceDB_hpp_ */
}