#include "LoginDialog.hpp"

#include "I18N.hpp"
#include "slic3r/GUI/wxExtensions.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "libslic3r_version.h"
#include "libslic3r/Utils.hpp"
#include "libslic3r/common_header/common_header.h"

#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/panel.h>
#include <wx/msgdlg.h>
#include <wx/webview.h>
#include <wx/utils.h>
#include <wx/display.h>
#include <wx/uri.h>
#include <wx/hyperlink.h>
#include <wx/stattext.h>

#include <slic3r/GUI/Widgets/WebView.hpp>
#include <nlohmann/json.hpp>
#include <boost/log/trivial.hpp>

namespace Slic3r {
    namespace GUI {

        // 事件表
wxBEGIN_EVENT_TABLE(LoginDialog, wxDialog)
    EVT_WEBVIEW_NAVIGATING(wxID_ANY, LoginDialog::OnWebViewNavigating)
    EVT_WEBVIEW_NEWWINDOW(wxID_ANY, LoginDialog::OnWebViewNewWindow)
    EVT_WEBVIEW_LOADED(wxID_ANY, LoginDialog::OnWebViewLoaded)
    EVT_WEBVIEW_ERROR(wxID_ANY, LoginDialog::OnWebViewError)
    EVT_CLOSE(LoginDialog::OnClose)
wxEND_EVENT_TABLE()

        LoginDialog::LoginDialog(wxWindow* parent, const wxString& title)
    : DPIDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m_webView(nullptr)
    , m_panel(nullptr)
    , m_mainSizer(nullptr)
    , m_openSystemBrowserLink(nullptr)
{
    InitializeUI();

    // 设置初始窗口大小（构造完成后再使用 FromDIP，避免基类初始化时空指针导致崩溃）
#ifdef __WXMSW__
    {
        // Windows：获取屏幕可用区域，避免在高 DPI 低分辨率屏（如 1366×768@150%）下窗口超出屏幕
        wxRect displayRect = wxDisplay(wxDisplay::GetFromWindow(this)).GetClientArea();
        const int screenW  = displayRect.GetWidth();
        const int screenH  = displayRect.GetHeight();

        // 期望尺寸（逻辑像素，FromDIP 会按实际 DPI 换算）
        wxSize desired  = FromDIP(wxSize(630, 780));
        wxSize minSz    = FromDIP(wxSize(520, 600));

        // 留 20px 边距，确保不超出可用区域
        const int margin = 20;
        int dlgW = std::min(desired.x, screenW - margin);
        int dlgH = std::min(desired.y, screenH - margin);
        int minW = std::min(minSz.x, screenW - margin);
        int minH = std::min(minSz.y, screenH - margin);

        SetSize(wxSize(dlgW, dlgH));
        SetMinSize(wxSize(minW, minH));
    }
#else
    SetSize(FromDIP(wxSize(630, 780)));
    SetMinSize(FromDIP(wxSize(520, 600)));
#endif

    // 设置对话框图标
    std::string icon_path = (boost::format("%1%/images/%2%.ico") % resources_dir() % Slic3r::CxBuildInfo::getIconName()).str();
    SetIcon(wxIcon(encode_path(icon_path.c_str()), wxBITMAP_TYPE_ICO));

    // 居中显示
    CenterOnParent();
}

        LoginDialog::~LoginDialog()
        {
            if (m_webView) {
                m_webView->Destroy();
                m_webView = nullptr;
            }
        }

        void LoginDialog::InitializeUI()
        {
            auto dark = Slic3r::GUI::wxGetApp().dark_mode();
            this->SetBackgroundColour(dark ? wxColour("#1c1e22") :wxColour("#f4f7fb") );
            // 创建主面板
            m_panel = new wxPanel(this, wxID_ANY);

            // 创建主布局
            m_mainSizer = new wxBoxSizer(wxVERTICAL);

            // 创建WebView（不加载任何URL）
            m_webView = WebView::CreateWebView(m_panel, wxEmptyString);
            if (m_webView == nullptr) {
                BOOST_LOG_TRIVIAL(error) << "Could not create WebView for login dialog";

                // 显示错误信息给用户
                wxStaticText* errorText = new wxStaticText(m_panel, wxID_ANY, 
                    _("Failed to initialize web browser component.\nPlease ensure Microsoft Edge WebView2 is installed."));
                errorText->SetForegroundColour(*wxRED);
                m_mainSizer->Add(errorText, 1, wxEXPAND | wxALL | wxALIGN_CENTER, FromDIP(20));
            } else {
                // 启用开发者工具（调试用）
                m_webView->EnableAccessToDevTools();

                // 添加WebView到布局
                m_mainSizer->Add(m_webView, 1, wxEXPAND | wxALL, FromDIP(5));

                // 绑定WebView事件
                m_webView->Bind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED, &LoginDialog::OnWebViewScriptMessage, this, m_webView->GetId());
                // 绑定 CXSWGroupInterface 为脚本消息通道
                m_webView->RemoveScriptMessageHandler("wx");
                CallAfter([this]() {
                    if (!m_webView)
                        return;
                    if (!m_webView->AddScriptMessageHandler("CXSWGroupInterface")) {
                        BOOST_LOG_TRIVIAL(error) << "Failed to add script message handler 'CXSWGroupInterface' for LoginDialog";
                    } else {
                        BOOST_LOG_TRIVIAL(info) << "Successfully added script message handler 'CXSWGroupInterface' for LoginDialog";
                    }
                });
            }

            // 设置面板布局
            m_panel->SetSizer(m_mainSizer);

            // 使用静态文本模拟链接样式：默认无下划线，悬停加下划线
            m_openSystemBrowserLink = new wxStaticText(
                m_panel,
                wxID_ANY,
                _L("System Browser Login"),
                wxDefaultPosition,
                wxDefaultSize);

            {
                wxFont f = GetFont();
                f.SetPointSize(f.GetPointSize() + 2);
                f.SetUnderlined(false);
                m_openSystemBrowserLink->SetFont(f);

                const bool is_dark = Slic3r::GUI::wxGetApp().dark_mode();
                const wxColour normal = wxColour("#00bb4c");//is_dark ? wxColour(0x67, 0xC2, 0x3A) : wxColour(0x19, 0x90, 0xFF);
                m_openSystemBrowserLink->SetForegroundColour(normal);
                m_openSystemBrowserLink->SetCursor(wxCursor(wxCURSOR_HAND));
            }

            m_openSystemBrowserLink->Bind(wxEVT_ENTER_WINDOW, &LoginDialog::OnLinkMouseEnter, this);
            m_openSystemBrowserLink->Bind(wxEVT_LEAVE_WINDOW, &LoginDialog::OnLinkMouseLeave, this);
            m_openSystemBrowserLink->Bind(wxEVT_LEFT_UP, &LoginDialog::OnOpenSystemBrowser, this);
            m_mainSizer->Add(m_openSystemBrowserLink, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM | wxTOP, FromDIP(20));

            // 创建对话框布局
            wxBoxSizer* dialogSizer = new wxBoxSizer(wxVERTICAL);
            dialogSizer->Add(m_panel, 1, wxEXPAND);
            SetSizer(dialogSizer);

            Layout();

        }

        void LoginDialog::ShowLoginDialog(const wxString& loginUrl)
        {
            wxString urlToLoad = loginUrl.IsEmpty() ? GetLoginUrl() : loginUrl;
            // 嵌入式 WebView 登录：根据文档要求追加 webview=1 参数
            auto append_param = [](const wxString& url, const wxString& key, const wxString& value) {
                if (url.IsEmpty())
                    return url;
                // 保留 fragment 部分，避免误拼接
                wxString base = url;
                wxString fragment;
                int      fragPos = url.Find('#');
                if (fragPos != wxNOT_FOUND) {
                    base     = url.Left(fragPos);
                    fragment = url.Mid(fragPos);
                }
                wxString lower = base.Lower();
                if (lower.Contains(key.Lower() + "="))
                    return url; // 已存在参数，直接返回原 URL
                wxString sep = base.Contains("?") ? "&" : "?";
                return base + sep + key + "=" + value + fragment;
            };
            wxURI    parsed(urlToLoad);
            wxString host = parsed.GetServer().Lower();
            if (!host.IsEmpty() && (host.Contains("creality.com") || host.Contains("creality.cn")) ) {
                urlToLoad = append_param(urlToLoad, wxT("webview"), wxT("1"));
            }
            m_loginUrl = urlToLoad;
            
            if (m_webView) {
                BOOST_LOG_TRIVIAL(error) << "Loading login URL: " << urlToLoad.ToStdString();
                m_webView->LoadURL(m_loginUrl);
            } else {
                BOOST_LOG_TRIVIAL(error) << "LoginDialog::WebView is not initialized";
                CallAfter([this]() {
                    wxMessageBox(_("Failed to initialize web browser component."), _("Login Error"), wxOK | wxICON_ERROR, this);
                });
            }
            // 静态文本不需同步 URL；点击事件直接读取 m_loginUrl
        }

        void LoginDialog::MarkLoginSucceeded()
        {
            m_login_succeeded = true;
        }

        wxString LoginDialog::GetLoginUrl()
        {
            // 这里返回登录页面的URL
            // 可以根据实际需求修改为正确的登录URL
            return wxT("");
        }

        void LoginDialog::OnWebViewNavigating(wxWebViewEvent& evt)
        {
            wxString url = evt.GetURL();
            BOOST_LOG_TRIVIAL(error) << "WebView navigating to: " << url.ToStdString();

            // 提取并标准化 host
            wxURI    parsed(url);
            wxString host = parsed.GetServer().Lower();
            if (host.IsEmpty()) {
                int schemePos = url.Find("://");
                if (schemePos != wxNOT_FOUND) {
                    wxString rest      = url.Mid(schemePos + 3);
                    int      slashPos  = rest.Find('/');
                    wxString hostGuess = (slashPos == wxNOT_FOUND) ? rest : rest.Left(slashPos);
                    if (!hostGuess.IsEmpty())
                        host = hostGuess.Lower();
                }
            }

            auto is_internal_auth_host = [](const wxString& h) {
                // 内部账号/手机登录域名（不跳转系统浏览器）
                return  h.Contains("id.creality") || h.Contains("id-dev.creality") || h.Contains("www.creality") || h.Contains("pre.creality")  ;
            };

            auto is_third_party_host = [](const wxString& h) {
                // 常见三方登录域名（需跳转系统浏览器）
                return h.Contains("open.weixin") || h.Contains("weixin.qq.com") || h.Contains("connect.qq.com") ||
                       h.Contains("graph.qq.com") || h.Contains("facebook.com") || h.Contains("accounts.google.com") ||
                       h.Contains("google.com") || h.Contains("github.com") || h.Contains("apple.com");
            };

            // 本地回调：仅在 localhost/127.0.0.1 且路径以 /login 开头时识别
            {
                wxString path = parsed.GetPath();
                const bool is_localhost = (host == "localhost" || host == "127.0.0.1");
                // 使用 Find("/login")==0 更兼容旧版 wxWidgets
                if (is_localhost && path.Find(wxT("/login")) == 0) {
                    BOOST_LOG_TRIVIAL(error) << "Local OAuth callback navigating to: host=" << host.ToStdString()
                                             << " path=" << path.ToStdString();
                    // 允许导航继续，确保请求发送到本地回调服务器；不要在此处关闭窗口
                    return;
                }
            }

            // 内部账号/手机登录：允许在内置 WebView 内导航
            if (is_internal_auth_host(host) || ( url.Contains("/oauth?code") && url.Contains("redirect_uri"))) {
                BOOST_LOG_TRIVIAL(error) << "Internal auth host detected, keep in-webview: " << host.ToStdString();
                return; // 允许继续导航
            }

            // 三方登录：跳转系统浏览器，并关闭当前窗口
            if (is_third_party_host(host)) {
                BOOST_LOG_TRIVIAL(error) << "Third-party auth host detected, redirecting to browser and closing: " << host.ToStdString();
                wxLaunchDefaultBrowser(url);
                evt.Veto();
                return;
            }

            // 其他未知域名：默认外部浏览器打开，避免内置 WebView 离开登录流程，并关闭窗口
            BOOST_LOG_TRIVIAL(error) << "Unknown host, default to external browser and close: " << host.ToStdString();
            wxLaunchDefaultBrowser(url);
            evt.Veto();
        }

        void LoginDialog::OnWebViewNewWindow(wxWebViewEvent& evt)
        {
            // 政策页面（隐私政策/用户协议）：始终在系统浏览器中打开
            {
                wxString newUrl  = evt.GetURL();
                wxURI    newUri(newUrl);
                wxString newPath = newUri.GetPath().Lower();
                if (newPath.StartsWith(wxT("/policy/")) || newPath == wxT("/privacy-policy") || newPath == wxT("/terms")) {
                    BOOST_LOG_TRIVIAL(info) << "New-window policy page, opening in system browser: " << newUrl.ToStdString();
                    wxLaunchDefaultBrowser(newUrl);
                    evt.Veto();
                    return;
                }
            }

            // 三方登录已通过 CXSWGroupInterface 的 JSON 消息在点击时直接转到系统浏览器，
            // 此处不再做额外处理，统一阻止在 WebView 内弹出新窗口以保持登录流程简洁。
            BOOST_LOG_TRIVIAL(info) << "New-window requested, veto under external-login flow: " << evt.GetURL().ToStdString();
            evt.Veto();
            return;
            wxString url = evt.GetURL();
            BOOST_LOG_TRIVIAL(error) << "WebView new-window requested: " << url.ToStdString();

            // Normalize host
            wxURI    parsed(url);
            wxString host = parsed.GetServer().Lower();
            if (host.IsEmpty()) {
                int schemePos = url.Find("://");
                if (schemePos != wxNOT_FOUND) {
                    wxString rest      = url.Mid(schemePos + 3);
                    int      slashPos  = rest.Find('/');
                    wxString hostGuess = (slashPos == wxNOT_FOUND) ? rest : rest.Left(slashPos);
                    if (!hostGuess.IsEmpty())
                        host = hostGuess.Lower();
                }
            }

            // 本地 OAuth 回调：如果目标是 localhost/127.0.0.1 且路径以 /login 开头，
            // 则在当前对话框内加载以确保回调抵达本地服务器，避免外部浏览器拦截导致状态不更新。
            {
                wxString path = parsed.GetPath();
                const bool is_localhost = (host == "localhost" || host == "127.0.0.1");
                if (is_localhost && path.Find(wxT("/login")) == 0) {
                    BOOST_LOG_TRIVIAL(error) << "New-window to local callback detected, loading in-webview: host="
                                              << host.ToStdString() << " path=" << path.ToStdString();
                    if (m_webView) {
                        m_webView->LoadURL(url);
                    }
                    evt.Veto();
                    return;
                }
            }

            auto is_internal_auth_host = [](const wxString& h) {
                return h.Contains("id.creality") || h.Contains("id-dev.creality") || h.Contains("www.creality");
             };
            auto is_third_party_host = [](const wxString& h) {
                return h.Contains("open.weixin") || h.Contains("weixin.qq.com") || h.Contains("connect.qq.com") ||
                       h.Contains("graph.qq.com") || h.Contains("facebook.com") || h.Contains("accounts.google.com") ||
                       h.Contains("google.com") || h.Contains("github.com") || h.Contains("apple.com");
            };

            // Internal auth flows: load inside dialog
            if (is_internal_auth_host(host) || ( url.Contains("/oauth?code") && url.Contains("redirect_uri") )) {
                m_webView->LoadURL(url);
                //evt.Veto();
                return;
            }

            // Third-party auth: launch external browser
            if (is_third_party_host(host)) {
                wxLaunchDefaultBrowser(url);
                evt.Veto();
                return;
            }

            // Unknown: default to external browser to avoid breaking login flow
            wxLaunchDefaultBrowser(url);
            evt.Veto();
        }

        void LoginDialog::OnWebViewLoaded(wxWebViewEvent& evt)
        {
            // 页面加载完成
            BOOST_LOG_TRIVIAL(error) << "WebView page loaded successfully";
        }

        void LoginDialog::OnWebViewError(wxWebViewEvent& evt)
        {
            wxString technicalError = evt.GetString();
            wxLogError("WebView error: %s", technicalError);


            //alpha + dev 版本会需要二次登录内部账号，所以屏蔽这个
            //// 提供用户友好的错误信息
            //wxString userFriendlyMsg = _("Failed to load the login page. Please check your internet connection and try again.");
            //
            //// 使用CallAfter避免在WebView2事件处理器中创建模态对话框导致重入问题
            //CallAfter([this, userFriendlyMsg, technicalError]() {
            //    wxString fullMsg = userFriendlyMsg + "\n\n" + _("Technical details: ") + technicalError;
            //    wxMessageBox(fullMsg, _("Login Error"), wxOK | wxICON_ERROR, this);
            //    EndModal(wxID_CANCEL);
            //});
        }

        void LoginDialog::OnWebViewScriptMessage(wxWebViewEvent& evt)
        {
            wxString message = evt.GetString();

            // 使用 CallAfter 避免 WebView 事件重入
            CallAfter([this, message]() {
                try {
                    auto j = nlohmann::json::parse(message.ToStdString());

                    // 仅处理 { action: "toNative", message: { ... } } 格式
                    if (j.contains("action") && j["action"].is_string() && j["action"].get<std::string>() == "toNative" &&
                        j.contains("message") && j["message"].is_object()) {
                        const auto& msg = j["message"];

                        // 优先处理 callback 字段（三方登录按钮返回的本地回调地址）
                        if (msg.contains("callback") && msg["callback"].is_string()) {
                            std::string cb = msg["callback"].get<std::string>();
                            if (!cb.empty()) {
                                BOOST_LOG_TRIVIAL(info) << "LoginDialog: opening callback URL in browser: " << cb;
                                wxLaunchDefaultBrowser(wxString::FromUTF8(cb));
                                return;
                            }
                        }
                    }

                    // 未匹配到目标消息，记录日志
                    BOOST_LOG_TRIVIAL(error) << "LoginDialog: script message ignored: " << message.ToStdString();
                } catch (const std::exception& e) {
                    BOOST_LOG_TRIVIAL(error) << "LoginDialog::OnWebViewScriptMessage JSON parse error: " << e.what();
                    BOOST_LOG_TRIVIAL(error) << "Raw message: " << message.ToStdString();
                }
            });
        }

        void LoginDialog::OnClose(wxCloseEvent& evt)
        {
            if (!m_login_succeeded && !m_close_event_sent) {
                m_close_event_sent = true;
                nlohmann::json event_json = {
                    {"command", "login_dialog_event"},
                    {"event", "close"}
                };
                wxString strJS = wxString::Format("window.handleStudioCmd(%s)", event_json.dump(-1, ' ', true, nlohmann::json::error_handler_t::ignore));
                GUI::wxGetApp().run_script(strJS);
            }
            evt.Skip();
        }

        void LoginDialog::OnOpenSystemBrowser(wxMouseEvent& evt)
        {
            evt.Skip(false);
            wxString urlToOpen = m_loginUrl.IsEmpty() ? GetLoginUrl() : m_loginUrl;
            // 系统浏览器登录不需要 webview=1，移除该参数
            auto remove_param = [](const wxString& url, const wxString& key) {
                if (url.IsEmpty())
                    return url;
                // 拆分 fragment，防止污染
                wxString base = url;
                wxString fragment;
                int      fragPos = url.Find('#');
                if (fragPos != wxNOT_FOUND) {
                    base     = url.Left(fragPos);
                    fragment = url.Mid(fragPos);
                }
                int qPos = base.Find('?');
                if (qPos == wxNOT_FOUND)
                    return url; // 无查询参数
                wxString      path  = base.Left(qPos);
                wxString      query = base.Mid(qPos + 1);
                wxArrayString parts = wxSplit(query, '&');
                wxString      newQuery;
                for (auto& p : parts) {
                    wxString lower = p.Lower();
                    if (lower.StartsWith(key.Lower() + "="))
                        continue; // 过滤掉目标参数
                    if (!newQuery.IsEmpty())
                        newQuery += "&";
                    newQuery += p;
                }
                wxString rebuilt = path;
                if (!newQuery.IsEmpty())
                    rebuilt += "?" + newQuery;
                return rebuilt + fragment;
            };
            urlToOpen = remove_param(urlToOpen, wxT("webview"));
            if (!urlToOpen.IsEmpty())
                wxLaunchDefaultBrowser(urlToOpen);
        }

        void LoginDialog::OnLinkMouseEnter(wxMouseEvent& evt)
        {
            if (!m_openSystemBrowserLink) return;
            wxFont f = m_openSystemBrowserLink->GetFont();
            f.SetUnderlined(true);
            m_openSystemBrowserLink->SetFont(f);

            evt.Skip();
        }

        void LoginDialog::OnLinkMouseLeave(wxMouseEvent& evt)
        {
            if (!m_openSystemBrowserLink) return;
            wxFont f = m_openSystemBrowserLink->GetFont();
            f.SetUnderlined(false);
            m_openSystemBrowserLink->SetFont(f);

            evt.Skip();
        }

        void LoginDialog::on_dpi_changed(const wxRect &suggested_rect)
        {
            if (m_mainSizer) {
                m_mainSizer->Layout();
            }
            if (m_openSystemBrowserLink) {
                wxFont f = GetFont();
                f.SetPointSize(f.GetPointSize() + 2);
                f.SetUnderlined(false);
                m_openSystemBrowserLink->SetFont(f);
            }

            // DPI 变化后确保窗口不超出屏幕可用区域
#ifdef __WXMSW__
            // Windows：统一用 FromDIP 重算期望尺寸，避免单位混用
            {
                wxRect displayRect = wxDisplay(wxDisplay::GetFromWindow(this)).GetClientArea();
                const int margin   = 20;

                wxSize desired = FromDIP(wxSize(630, 780));
                wxSize minSz   = FromDIP(wxSize(520, 600));

                int dlgW = std::min(desired.x, displayRect.GetWidth()  - margin);
                int dlgH = std::min(desired.y, displayRect.GetHeight() - margin);
                int minW = std::min(minSz.x,   displayRect.GetWidth()  - margin);
                int minH = std::min(minSz.y,   displayRect.GetHeight() - margin);

                SetMinSize(wxSize(minW, minH));
                SetSize(wxSize(dlgW, dlgH));
            }
#else
            // Mac：直接使用 suggested_rect，wxWidgets 已正确处理 Retina 缩放
            SetSize(suggested_rect.GetSize());
#endif

            Refresh();
        }
    }
}// namespace Slic3r::GU