#ifndef slic3r_LoginDialog_hpp_
#define slic3r_LoginDialog_hpp_

#include <wx/dialog.h>
#include <wx/webview.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/hyperlink.h>
#include <wx/stattext.h>
#include <wx/panel.h>

#include "I18N.hpp"
#include "GUI_Utils.hpp"

namespace Slic3r {
namespace GUI {

class LoginDialog : public DPIDialog
{
public:
    LoginDialog(wxWindow* parent, const wxString& title = _L("Login"));
    virtual ~LoginDialog();

    // 显示登录对话框
    void ShowLoginDialog(const wxString& loginUrl = wxEmptyString);
    void MarkLoginSucceeded();

    // 关闭对话框，多次调用只生效一次（登录成功路径与 post_login_status_cmd
    // 都可能触发关闭，重复 EndModal 会导致 AppKit 窗口事务断言崩溃）
    void CloseModalOnce(int code);

private:
    // 事件处理函数
    void OnWebViewNavigating(wxWebViewEvent& evt);
    void OnWebViewNewWindow(wxWebViewEvent& evt);
    void OnWebViewLoaded(wxWebViewEvent& evt);
    void OnWebViewError(wxWebViewEvent& evt);
    void OnWebViewScriptMessage(wxWebViewEvent& evt);
    void OnClose(wxCloseEvent& evt);
    void OnOpenSystemBrowser(wxMouseEvent& evt);
    void StartNativeAppleSignIn(); // 网页 apple-icon 点击/拦截回调入口，走原生 SIWA
        void send_apple_login_v2(const std::string& firebase_id_token, int login_attempt = 1);
    void OnLinkMouseEnter(wxMouseEvent& evt);
    void OnLinkMouseLeave(wxMouseEvent& evt);
    
    // 初始化UI
    void InitializeUI();
    
    // 获取登录URL
    wxString GetLoginUrl();

protected:
    // 实现DPIAware的纯虚函数
    void on_dpi_changed(const wxRect &suggested_rect) override;

private:
    wxWebView* m_webView;
    wxPanel* m_panel;
    wxBoxSizer* m_mainSizer;
    wxStaticText* m_openSystemBrowserLink;
    wxButton*     m_appleSignInButton;

    
    wxString m_loginUrl;
    bool     m_login_succeeded { false };
    bool     m_close_event_sent { false };
    bool     m_modal_ended { false };
    // 未注册绑定流程进行中：期间 WebView 所有导航留在对话框内
    bool     m_bind_flow_web { false };
    
    // 声明事件表
    wxDECLARE_EVENT_TABLE();
};

} // namespace GUI
} // namespace Slic3r

#endif // slic3r_LoginDialog_hpp_