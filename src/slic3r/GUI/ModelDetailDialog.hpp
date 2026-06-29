#ifndef slic3r_ModelDetailDialog_hpp_
#define slic3r_ModelDetailDialog_hpp_

#include <wx/dialog.h>
#include <string>

namespace Slic3r {
namespace GUI {

class WebModelLibraryView;

// A dialog that shows a model detail page in an embedded WebView.
// Internally reuses WebModelLibraryView so UA, cookies, script message
// handlers and dev-tools are identical to the main model library tab.
//
// Show() is overridden: the dialog stays hidden until the second
// wxEVT_WEBVIEW_LOADED fires (i.e. cookies & UA have been re-injected
// and the page is truly ready).  This prevents the "参数非法" error
// caused by clicking download before auth headers are available.
class ModelDetailDialog : public wxDialog
{
public:
    // url   - full detail page URL to load
    // title - dialog title (model name); falls back to "Model Detail"
    ModelDetailDialog(wxWindow*       parent,
                      const wxString& url,
                      const wxString& title = wxEmptyString);
    ~ModelDetailDialog() = default;

    // Build the detail URL from a model_id using the same cloud base address
    // as the rest of the application.  Returns empty string if model_id is empty.
    static wxString BuildDetailUrl(const std::string& model_id);

    // Override: block Show(true) until the page is fully loaded
    bool Show(bool show = true) override;

private:
    WebModelLibraryView* m_view             = nullptr;
    bool                 m_cookies_injected = false;
    bool                 m_page_ready       = false;  // set true after second load
    int                  m_load_count       = 0;
};

} // namespace GUI
} // namespace Slic3r

#endif // slic3r_ModelDetailDialog_hpp_
