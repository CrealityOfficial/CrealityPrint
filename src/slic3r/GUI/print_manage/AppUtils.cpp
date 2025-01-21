#include "AppUtils.hpp"
#include "../Widgets/WebView.hpp"
#include "../GUI.hpp"

using namespace Slic3r::GUI;

namespace DM{

    void AppUtils::PostMsg(wxWebView* browse, const std::string& data)
    {
        WebView::RunScript(browse, from_u8(data));
    }

}