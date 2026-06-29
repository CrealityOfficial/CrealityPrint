#ifndef slic3r_WebModelLibraryView_hpp_
#define slic3r_WebModelLibraryView_hpp_

#include "GUI_Utils.hpp"
#include "Event.hpp"
#include "wxExtensions.hpp"

#include <wx/panel.h>
#include <wx/webview.h>
#include <wx/timer.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <unordered_set>

namespace Slic3r {
namespace GUI {

class MainFrame;

// Web Model Library View for embedding online model interface
class WebModelLibraryView : public wxPanel
{
public:
    WebModelLibraryView(wxWindow* parent, const wxString& url = wxEmptyString);
    virtual ~WebModelLibraryView();

    // Web functions
    void load_url(const wxString& url);
    void open_default_page();
    void search(const wxString& query);
    bool UpdateUserAgent();
    void LoadURL(const wxString& url) { load_url(url); }  // Alias for compatibility
    void SetStartPage(const wxString& url);
    void RunScript(const wxString& javascript);
    void Reload();
    void GoBack();
    void GoForward();
    bool CanGoBack();
    bool CanGoForward();
    // 标记是否已完成首次加载（用于避免重复首载并在重建 GUI 后正常触发）
    bool IsInitialized() const;
    
    // Show/Hide functions
    bool Show(bool show = true) override;
    void Hide();
    bool IsShown() const;
    
    // Get webview object
    wxWebView* GetWebView() const { return m_browser; }
    
    // Event handlers
    void OnNavigating(wxWebViewEvent& evt);
    void OnNavigated(wxWebViewEvent& evt);
    void OnLoaded(wxWebViewEvent& evt);
    void OnError(wxWebViewEvent& evt);
    void OnNewWindow(wxWebViewEvent& evt);
    void OnTitleChanged(wxWebViewEvent& evt);
    void OnScriptMessage(wxWebViewEvent& evt);
    
    // Button event handlers
    void OnBackButton(wxCommandEvent& evt);
    void OnForwardButton(wxCommandEvent& evt);
    void OnRefreshButton(wxCommandEvent& evt);
    void OnHomeButton(wxCommandEvent& evt);
    
    bool needLogin(std::string cmd);
    bool handleLinkType403(std::string cmd);
    bool handleLinkType404(std::string cmd);

private:
    void CreateControls();
    void CreateWebView();
    void BindEvents();
    // Set cookies for the target URL before navigation
    bool SetCookiesForUrl(const wxString& url);
    
    // UI components
    wxWebView*      m_browser;
    wxPanel*        m_toolbar_panel;
    wxButton*       m_back_btn;
    wxButton*       m_forward_btn;
    wxButton*       m_refresh_btn;
    wxButton*       m_home_btn;
    
    // Configuration
    wxString        m_start_url;
    wxString        m_current_url;
    
    // Layout
    wxBoxSizer*     m_main_sizer;
    wxBoxSizer*     m_toolbar_sizer;

    // 记录已为其注入过 Cookie 的域名，避免无限重载
    std::unordered_set<std::string> m_cookie_injected_domains;
    // UA 是否已初始化（仅首次设置 UA，后续调用只刷新 Cookies）
    bool m_ua_initialized = false;

    DECLARE_EVENT_TABLE()
};

} // namespace GUI
} // namespace Slic3r

#endif // slic3r_WebModelLibraryView_hpp_