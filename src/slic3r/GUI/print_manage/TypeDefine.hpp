#ifndef RemotePrint_TypeDefine
#define RemotePrint_TypeDefine

#include "vector"
#include "string"

namespace DM{
    //
    constexpr const char* EVENT_SET_ACTIVE_DEVICE = "set_active_device";
    
    // system event
    constexpr const char* EVENT_SET_SYS_THEME = "is_dark_theme";
    constexpr const char* EVENT_SET_USER_THEME = "get_user";
}
#endif