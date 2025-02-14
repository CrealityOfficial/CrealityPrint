#include "AppUtils.hpp"
#include "../Widgets/WebView.hpp"
#include "../GUI.hpp"

#include <boost/uuid/detail/md5.hpp>
#include "libslic3r/Utils.hpp"

using namespace Slic3r::GUI;
using namespace boost::uuids::detail;

namespace DM{

    void AppUtils::PostMsg(wxWebView* browse, const std::string& data)
    {
        WebView::RunScript(browse, from_u8(data));
    }

    std::string AppUtils::MD5(const std::string& file)
    {
        std::string ret;
        std::string filePath = std::string(file);
        Slic3r::bbl_calc_md5(filePath, ret);
        return ret;
    }

}