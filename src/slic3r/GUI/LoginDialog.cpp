#include "LoginDialog.hpp"

#include "I18N.hpp"
#include "slic3r/GUI/wxExtensions.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/GUI.hpp"
#include "slic3r/Utils/Http.hpp"
#include "slic3r/Utils/NetworkAgent.hpp"
#include "AppleSignIn.hpp"
#include "FirebaseSignIn.hpp"

// Unbuffered crash-investigation logging (implemented in FirebaseSignIn.mm,
// which is only compiled for the Mac App Store build).
#if defined(__APPLE__) && defined(CREALITYPRINT_APP_STORE)
extern "C" void cp_apple_debug_log(const char *message);
#endif
#include "slic3r/GUI/MainFrame.hpp"
#include "libslic3r_version.h"
#include "libslic3r/Utils.hpp"
#include <boost/nowide/fstream.hpp>
#include <boost/filesystem/path.hpp>
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
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

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
    , m_appleSignInButton(nullptr)
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

            // App Store 版 Apple 入口 = 登录网页里的 apple-icon（点击被注入的拦截脚本接管，走原生 Authentication Services），不再显示独立按钮

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
            const bool is_creality_login_host =
                host == wxT("www.creality.com") || host == wxT("pre.creality.com") ||
                host == wxT("www.creality.cn") || host == wxT("pre.creality.cn") ||
                host == wxT("id.creality.com") || host == wxT("id-dev.creality.com") ||
                host == wxT("id.creality.cn") || host == wxT("id-dev.creality.cn");
            if (is_creality_login_host) {
                urlToLoad = append_param(urlToLoad, wxT("webview"), wxT("1"));
#if defined(__WXOSX__) && defined(CREALITYPRINT_APP_STORE)
                urlToLoad = append_param(urlToLoad, wxT("app_store"), wxT("macos"));
#endif
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

        void LoginDialog::CloseModalOnce(int code)
        {
            if (m_modal_ended)
                return;
            m_modal_ended = true;
            EndModal(code);
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

            // 未注册绑定流程期间：Apple/Firebase 等跳转均留在内嵌 WebView，
            // 否则会被下方三方规则踢到系统浏览器，打断绑定流程
            if (m_bind_flow_web) {
                BOOST_LOG_TRIVIAL(info) << "Bind flow active, keep in-webview: " << host.ToStdString();
                return;
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
#if defined(__WXOSX__) && defined(CREALITYPRINT_APP_STORE)
            // App Store 审核（Guideline 4）：应用未实现原生 Sign in with Apple
            // 流程前，在商店版登录页隐藏 Apple 登录入口。按钮由账号中心登录页
            // 渲染，class 为 apple-icon（Google 图标为 google-icon，不受影响）。
            if (m_webView) {
                const wxString js =
                    "(function(){"
                    "if(window.__cpAppleIntercepted) return;"
                    "window.__cpAppleIntercepted = true;"
                    "document.addEventListener('click', function(e){"
                    "  if(window.__cpAllowAppleWeb) return;"
                    "  var el = e.target;"
                    "  while(el && el !== document){"
                    "    if(el.classList && el.classList.contains('apple-icon')){"
                    "      e.preventDefault(); e.stopPropagation();"
      "      try{"
                    "        var p = JSON.stringify({action:'toNative',message:{command:'appleSignIn'}});"
                    "        if(window.webkit && window.webkit.messageHandlers && window.webkit.messageHandlers.CXSWGroupInterface){"
                    "          window.webkit.messageHandlers.CXSWGroupInterface.postMessage(p);"
                    "        } else if(window.CXSWGroupInterface && window.CXSWGroupInterface.postMessage){"
                    "          window.CXSWGroupInterface.postMessage(p);"
                    "        } else { console.warn('cp: no message channel for apple sign-in'); }"
                    "      }catch(err){ console.warn('cp apple intercept failed', err); }"
                    "      return;"
                    "    }"
                    "    el = el.parentElement;"
                    "  }"
                    "}, true);"
                    "})();";
                m_webView->RunScript(js);
            }
#endif
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

                        // App Store 版：网页 Apple 入口的点击统一走原生 SIWA
                        if (msg.contains("command") && msg["command"].is_string() &&
                            msg["command"].get<std::string>() == "appleSignIn") {
                            BOOST_LOG_TRIVIAL(info) << "LoginDialog: apple-icon click intercepted, starting native sign-in";
                            StartNativeAppleSignIn();
                            return;
                        }

                        // 优先处理 callback 字段（三方登录按钮返回的本地回调地址）
                        if (msg.contains("callback") && msg["callback"].is_string()) {
                            std::string cb = msg["callback"].get<std::string>();
                            if (!cb.empty()) {
#if defined(__WXOSX__) && defined(CREALITYPRINT_APP_STORE)
                                // 双保险：注入的点击拦截未生效时，Apple 相关 callback
                                // 也不跳出系统浏览器，转原生流程
                                if (cb.find("apple") != std::string::npos) {
                                    BOOST_LOG_TRIVIAL(info) << "LoginDialog: apple callback intercepted: " << cb;
                                    StartNativeAppleSignIn();
                                    return;
                                }
#endif
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

        void LoginDialog::StartNativeAppleSignIn()
        {
#if defined(__APPLE__) && defined(CREALITYPRINT_APP_STORE)
            cp_apple_debug_log("StartNativeAppleSignIn: entry (apple-icon)");
            start_apple_sign_in([this](AppleSignInResult apple) {
                cp_apple_debug_log(("OnAppleSignIn: authorization callback, success=" + std::string(apple.success ? "1" : "0")).c_str());
                CallAfter([this, apple]() {
                    if (wxGetApp().get_login_dialog() != this)
                        return; // 对话框已关闭或重建
                    if (!apple.success) {
                        cp_apple_debug_log("OnAppleSignIn: CallAfter entered, success=1");
                        if (m_appleSignInButton)
                            m_appleSignInButton->Enable();
                        wxMessageBox(from_u8(apple.error), _L("Apple Sign in"), wxOK | wxICON_ERROR, this);
                        return;
                    }
                    BOOST_LOG_TRIVIAL(info) << "Apple native authorization ok: identity_token_len=" << apple.identity_token.size()
                        << ", nonce_present=" << !apple.nonce.empty()
                        << ", auth_code_len=" << apple.authorization_code.size()
                        << ", user_present=" << !apple.user.empty();
                    firebase_sign_in_with_apple(apple.identity_token, apple.nonce, apple.authorization_code,
                        [this](FirebaseSignInResult firebase) {
                            CallAfter([this, firebase]() {
                                if (wxGetApp().get_login_dialog() != this)
                                    return; // 对话框已关闭或重建
                                if (!firebase.success) {
                                    if (m_appleSignInButton)
                                        m_appleSignInButton->Enable();
                                    BOOST_LOG_TRIVIAL(error) << "Apple Firebase exchange failed: " << firebase.error;
                                    wxMessageBox(from_u8(firebase.error), _L("Apple Sign in"), wxOK | wxICON_ERROR, this);
                                    return;
                                }
                                send_apple_login_v2(firebase.firebase_id_token);
                            });
                        });
                });
            });
#endif
        }


        // Same contract as the Creality Cloud web login: loginV2 with type 23
        // (Apple via Firebase) carrying the Firebase ID token as accessToken.
        // The resulting session is applied exactly like the WebView login in
        // HttpServer.cpp: fetch the profile with the new token, then hand the
        // same JSON shape to NetworkAgent::change_user.
        void LoginDialog::send_apple_login_v2(const std::string& firebase_id_token, int login_attempt)
        {
#if defined(__APPLE__) && defined(CREALITYPRINT_APP_STORE)
            cp_apple_debug_log("send_apple_login_v2: enter");
            nlohmann::json request;
            request["type"] = 23;
            request["accessToken"] = firebase_id_token;
            const std::string request_id = boost::uuids::to_string(boost::uuids::random_generator()());
            BOOST_LOG_TRIVIAL(info) << "Apple login request: firebase_id_token_len=" << firebase_id_token.size()
                << ", type=23"
                << ", requestId=" << request_id;
            Http::set_extra_headers(wxGetApp().get_extra_header());
            Http http = Http::post(get_cloud_api_url() + "/api/cxy/account/v2/loginV2");
            http.header("Content-Type", "application/json")
                .header("__CXY_REQUESTID_", request_id)
                .timeout_connect(5).timeout_max(15).set_post_body(request.dump())
                    .on_complete([this, request_id, firebase_id_token, login_attempt](std::string body, unsigned status) {
                        // Worker thread, mirroring HttpServer's login callback;
                        // NetworkAgent::change_user is used there the same way.
                        std::string failure;
                        std::string pending_session; // loginV2 pending 会话，供绑定流程接力
                        try {
                            BOOST_LOG_TRIVIAL(error) << "Apple loginV2 response: requestId=" << request_id
                                << ", HTTP " << status << ", body=" << body;
                            nlohmann::json response = nlohmann::json::parse(body);
                            const int response_code = response.value("code", -1);
                            const std::string response_msg = response.value("msg", "");
                            if (status != 200 || response_code != 0) {
                                std::ostringstream details;
                                details << "Apple loginV2 failed (HTTP " << status
                                        << ", code " << response_code << "): "
                                        << (response_msg.empty() ? "no server message" : response_msg)
                                        << "\nrequestId: " << request_id;
                                BOOST_LOG_TRIVIAL(error) << details.str();
                                failure = details.str();
                            } else {
                                cp_apple_debug_log("send_apple_login_v2: loginV2 returned success, token present");
                                const auto& result = response.at("result");
                                const std::string token = result.value("token", "");
                                if (token.empty())
                                    throw std::runtime_error("Apple login response carries no token");
                                auto field_str = [](const nlohmann::json& j, const char* key) {
                                    if (!j.contains(key)) return std::string();
                                    if (j[key].is_string()) return j[key].get<std::string>();
                                    if (j[key].is_number()) return std::to_string(j[key].get<long long>());
                                    return std::string();
                                };
// Apply the session exactly like the working web login (HttpServer.cpp
                                // CX path): write user_info.json and let the file watcher /
                                // post_login_status_cmd flip the global login state. The
                                // NetworkAgent is never created on this branch (on_init_network
                                // is disabled upstream), so change_user is a dead end here.
                                const auto user_info2 = result.value("user_info", nlohmann::json::object());
                                const std::string apple_uid = result.value("userId", "");
                                const std::string apple_nick = user_info2.value("nickName", "");
                                const std::string apple_avatar = user_info2.value("avatar", "");
                                if (token.empty() || apple_uid.empty())
                                    throw std::runtime_error("Apple login response has no Creality Cloud session");
                                // First-time Apple sign-in on this account has not finished
                                // Creality Cloud registration (bind-email) yet: loginV2
                                // answers code=0 with a placeholder userId "0". Do NOT
                                // persist that empty session; guide the user through the
                                // web signup inside the dialog WebView instead. After the
                                // web flow completes (or on the next native attempt) the
                                // server returns the real account.
                                // 与网页端对齐：result.isNeedBindPhone=true 时也需先完成绑定
                                // （已注册但未绑定邮箱的账号会返回真实 userId + 该标志）
                                const bool need_bind = result.value("isNeedBindPhone", false);
                                if (apple_uid == "0" || apple_uid.empty()) { // 与移动端海外版一致：isNeedBindPhone=true 但 userId 非 0 时直接登录
                                    // 与移动端 App 一致：新三方身份先调 createFromThird 自动建号，
                                    // 成功后重跑 loginV2 拿真实会话（无表单、仅一次授权）。
                                    // 失败则回退到网页绑定页（携 pending 会话）。
                                    nlohmann::json pending;
                                    pending["token"] = token;
                                    pending["userId"] = apple_uid;
                                    pending["newUser"] = result.value("newUser", false);
                                    pending["type"] = 23;
                                    pending["isNeedBindPhone"] = true;
                                    pending_session = pending.dump();
                                    if (login_attempt < 2) {
                                        cp_apple_debug_log("send_apple_login_v2: new third-party identity, calling createFromThird");
                                        const std::string pending_json = pending_session;
                                        Http http2 = Http::post(get_cloud_api_url() + "/api/cxy/account/v2/createFromThird");
                                        // Http 构造时注入的全局头含注销后残留的旧 token/uid，且
                                        // curl_slist 不去重会重复发送；先移除再补全新三头（与网页
                                        // axios 拦截器同款约定：TOKEN + UID + APP_ID）
                                        http2.remove_header("__CXY_TOKEN_")
                                            .remove_header("__CXY_UID_")
                                            .header("Content-Type", "application/json")
                                            .header("__CXY_TOKEN_", token)
                                            .header("__CXY_UID_", apple_uid.empty() ? std::string("0") : apple_uid)
                                            .header("__CXY_APP_ID_", "creality_model")
                                            .header("__CXY_REQUESTID_", request_id)
                                            .timeout_connect(5).timeout_max(15).set_post_body(std::string("{}"))
                                            .on_complete([this, request_id, firebase_id_token, login_attempt, pending_json](std::string body2, unsigned status2) {
                                                std::string failure2;
                                                try {
                                                    BOOST_LOG_TRIVIAL(error) << "createFromThird response: requestId=" << request_id
                                                        << ", HTTP " << status2 << ", body=" << body2;
                                                    nlohmann::json r2 = nlohmann::json::parse(body2);
                                                    if (status2 != 200 || r2.value("code", -1) != 0)
                                                        throw std::runtime_error("HTTP " + std::to_string(status2) + ", code " + std::to_string(r2.value("code", -1)) + ": " + r2.value("msg", "no message"));
                                                    // 建号/关联成功：重跑 loginV2 获取真实会话
                                                    std::this_thread::sleep_for(std::chrono::milliseconds(600));
                                                    CallAfter([this, firebase_id_token, login_attempt]() {
                                                        if (wxGetApp().get_login_dialog() != this)
                                                            return; // 对话框已关闭或重建
                                                        send_apple_login_v2(firebase_id_token, login_attempt + 1);
                                                    });
                                                    return;
                                                } catch (const std::exception& e) {
                                                    failure2 = std::string("createFromThird failed: ") + e.what();
                                                    BOOST_LOG_TRIVIAL(error) << failure2 << ", requestId=" << request_id;
                                                }
                                                // 失败回退：带 pending 会话进入网页绑定页
                                                CallAfter([this, failure2, pending_json]() {
                                                    if (wxGetApp().get_login_dialog() != this)
                                                        return;
                                                    if (m_appleSignInButton)
                                                        m_appleSignInButton->Enable();
                                                    if (m_webView) {
                                                        wxString js_safe = wxString::FromUTF8(pending_json);
                                                        js_safe.Replace(wxT("\\"), wxT("\\\\"));
                                                        js_safe.Replace(wxT("'"), wxT("\\'"));
                                                        m_bind_flow_web = true;
                                                        m_webView->RunScript(
                                                            "document.cookie='id-application=' + encodeURIComponent('" + js_safe + "') + ';path=/;max-age=2592000'");
                                                        wxString base = m_loginUrl;
                                                        int q = base.Find(wxT("?"));
                                                        wxString query = (q == wxNOT_FOUND) ? wxString() : base.Mid(q + 1);
                                                        int hash = base.Find(wxT("#"));
                                                        if (hash != wxNOT_FOUND && q > hash) { query = wxString(); }
                                                        wxString tp = wxT("https://id.creality.com/binding/email");
                                                        if (!query.empty()) { tp += wxT("?") + query; }
                                                        m_webView->LoadURL(tp);
                                                    }
                                                    wxMessageBox(from_u8(failure2 + "\n\nContinuing with the web sign-up page below."), _L("Apple Sign in"), wxOK | wxICON_INFORMATION, this);
                                                });
                                            })
                                            .on_error([this](std::string body, std::string err, unsigned status) {
                                                CallAfter([this, err]() {
                                                    if (wxGetApp().get_login_dialog() != this)
                                                        return;
                                                    if (m_appleSignInButton)
                                                        m_appleSignInButton->Enable();
                                                    wxMessageBox(from_u8("createFromThird network failure: " + err), _L("Apple Sign in"), wxOK | wxICON_ERROR, this);
                                                });
                                            })
                                            .perform();
                                        return;
                                    }
                                    cp_apple_debug_log("send_apple_login_v2: binding required, pending session prepared");
                                    throw std::runtime_error("__APPLE_ACCOUNT_NOT_REGISTERED__");
                                }
                                nlohmann::json r;
                                r["token"] = token;
                                r["nickName"] = apple_nick;
                                r["avatar"] = apple_avatar;
                                r["userId"] = apple_uid;
                                boost::filesystem::path user_file = boost::filesystem::path(data_dir()) / "user_info.json";
                                {
                                    boost::nowide::ofstream c;
                                    c.open(user_file.string(), std::ios::out | std::ios::trunc);
                                    c << r.dump(4) << std::endl;
                                    c.close();
                                }
                                cp_apple_debug_log(("send_apple_login_v2: user_info.json written, uid_len="
                                    + std::to_string(apple_uid.size())).c_str());
                                UserInfo user;
                                user.token = token;
                                user.nickName = apple_nick;
                                user.avatar = apple_avatar;
                                user.userId = apple_uid;
                                wxGetApp().app_config->set("cloud", "user_id", apple_uid);
                                wxGetApp().app_config->set("cloud", "token", token);
                                cp_apple_debug_log("send_apple_login_v2: posting login status");
                                CallAfter([user]() { wxGetApp().post_login_status_cmd(true, user); });
                            }
                        } catch (const std::exception& error) {
                            failure = error.what();
                            BOOST_LOG_TRIVIAL(error) << "Apple login failure: " << failure;
                        }
                        CallAfter([this, failure, pending_session]() {
                            if (wxGetApp().get_login_dialog() != this)
                                return; // 对话框已关闭或重建
                            if (m_appleSignInButton)
                                m_appleSignInButton->Enable();
                            if (failure == "__APPLE_ACCOUNT_NOT_REGISTERED__") {
                                // 导航到登录网站的三方登录专用路由（与网页点 Apple 图标
                                // 后进入的页面完全一致）。携带当前登录页的 OAuth 参数，
                                // 用户在那里再次确认 Apple 登录，网页会弹出官方的
                                // 绑定邮箱表单（含 Turnstile/验证码），完成后经原
                                // redirect_uri -> localhost 回调自动登录。
                                if (m_webView) {
                                    wxString base = m_loginUrl;
                                    int q = base.Find(wxT("?"));
                                    wxString query = (q == wxNOT_FOUND) ? wxString() : base.Mid(q + 1);
                                    int hash = base.Find(wxT("#"));
                                    if (hash != wxNOT_FOUND && q > hash) { query = wxString(); }
                                    // 与网页同款会话接力：把 loginV2 返回的 pending 会话
                                    // 写进网站自身使用的 id-application cookie，然后直接
                                    // 进入 /binding/email。绑定页读同一会话即可完成绑定，
                                    // 不再触发第二次 Apple 授权（与网页端完全一致）。
                                    // 用 encodeURIComponent 编码，与 js-cookie 默认行为一致
                                    wxString js_safe = wxString::FromUTF8(pending_session);
                                    js_safe.Replace(wxT("\\"), wxT("\\\\"));
                                    js_safe.Replace(wxT("'"), wxT("\\'"));
                                    m_bind_flow_web = true; // 绑定流程期间所有导航留在内嵌 WebView
                                    m_webView->RunScript(
                                        "document.cookie='id-application=' + encodeURIComponent('" + js_safe + "') + ';path=/;max-age=2592000'");
                                    wxString tp = wxT("https://id.creality.com/binding/email");
                                    if (!query.empty()) { tp += wxT("?") + query; }
                                    BOOST_LOG_TRIVIAL(info) << "Apple login needs binding, navigating WebView to binding/email (pending session injected): " << tp.ToStdString();
                                    m_webView->LoadURL(tp);
                                }
                                return;
                            }
                            if (!failure.empty()) {
                                wxMessageBox(from_u8(failure), _L("Apple Sign in"), wxOK | wxICON_ERROR, this);
                                return;
                            }
                            MarkLoginSucceeded();
                            wxGetApp().request_user_login(1);
                            CloseModalOnce(wxID_OK);
                        });
                    })
                    .on_error([this, request_id](std::string body, std::string error, unsigned status) {
                        std::string diagnostic = "Apple loginV2 network failure (HTTP " + std::to_string(status) + "): " + error
                            + "\nrequestId: " + request_id;
                        BOOST_LOG_TRIVIAL(error) << diagnostic;
                        CallAfter([this, diagnostic = std::move(diagnostic)]() {
                            if (wxGetApp().get_login_dialog() != this)
                                return; // 对话框已关闭或重建
                            if (m_appleSignInButton)
                                m_appleSignInButton->Enable();
                            wxMessageBox(from_u8(diagnostic), _L("Apple Sign in"), wxOK | wxICON_ERROR, this);
                        });
                    }).perform();
#endif
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