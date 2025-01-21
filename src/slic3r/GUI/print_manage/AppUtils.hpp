#ifndef RemotePrint_AppUtils_hpp_
#define RemotePrint_AppUtils_hpp_
#include "nlohmann/json.hpp"
#include <string>

class wxWebView;
namespace DM{

class AppUtils{
public:
    static void PostMsg(wxWebView*browse, const std::string&data);
};
}
#endif