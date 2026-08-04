#include "WebView.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/Utils/MacDarkMode.hpp"

#include <boost/log/trivial.hpp>

#include <wx/webviewarchivehandler.h>
#include <wx/webviewfshandler.h>
#include <wx/textfile.h>
#include <wx/utils.h>
#if wxUSE_WEBVIEW_EDGE
#include <wx/msw/webview_edge.h>
#elif defined(__WXMAC__)
#include <wx/osx/webview_webkit.h>
#endif
#include <wx/uri.h>
#if defined(__WIN32__) || defined(__WXMAC__)
#include "wx/private/jsscriptwrapper.h"
#endif

#ifdef __WIN32__
#include <WebView2.h>
#include <Shellapi.h>
#include <slic3r/Utils/Http.hpp>
#elif defined __linux__
#include <gtk/gtk.h>
#define WEBKIT_API
struct WebKitWebView;
struct WebKitJavascriptResult;
extern "C" {
WEBKIT_API void
webkit_web_view_run_javascript                       (WebKitWebView             *web_view,
                                                      const gchar               *script,
                                                      GCancellable              *cancellable,
                                                      GAsyncReadyCallback       callback,
                                                      gpointer                  user_data);
WEBKIT_API WebKitJavascriptResult *
webkit_web_view_run_javascript_finish                (WebKitWebView             *web_view,
                                                      GAsyncResult              *result,
						      GError                    **error);
WEBKIT_API void
webkit_javascript_result_unref              (WebKitJavascriptResult *js_result);
}
#endif

#include <cstdlib>
#ifdef __WIN32__
// Run Download and Install in another thread so we don't block the UI thread
DWORD DownloadAndInstallWV2RT() {

  int returnCode = 2; // Download failed
  // Use fwlink to download WebView2 Bootstrapper at runtime and invoke installation
  // Broken/Invalid Https Certificate will fail to download
  // Use of the download link below is governed by the below terms. You may acquire the link
  // for your use at https://developer.microsoft.com/microsoft-edge/webview2/. Microsoft owns
  // all legal right, title, and interest in and to the WebView2 Runtime Bootstrapper
  // ("Software") and related documentation, including any intellectual property in the
  // Software. You must acquire all code, including any code obtained from a Microsoft URL,
  // under a separate license directly from Microsoft, including a Microsoft download site
  // (e.g., https://developer.microsoft.com/microsoft-edge/webview2/).
  // HRESULT hr = URLDownloadToFileW(NULL, L"https://go.microsoft.com/fwlink/p/?LinkId=2124703",
  //                               L".\\plugin\\MicrosoftEdgeWebview2Setup.exe", 0, 0);
  fs::path target_file_path = (fs::path(Slic3r::resources_dir()).parent_path() / "MicrosoftEdgeWebView2RuntimeInstallerX64.exe");
  bool downloaded = true;
  if (downloaded) {
    // Either Package the WebView2 Bootstrapper with your app or download it using fwlink
    // Then invoke install at Runtime.
    SHELLEXECUTEINFOW shExInfo = {0};
    shExInfo.cbSize = sizeof(shExInfo);
    shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    shExInfo.hwnd = 0;
    shExInfo.lpVerb = L"runas";
    shExInfo.lpFile = target_file_path.generic_wstring().c_str();
    shExInfo.lpParameters = L" /install";
    shExInfo.lpDirectory = 0;
    shExInfo.nShow = 0;
    shExInfo.hInstApp = 0;

    if (ShellExecuteExW(&shExInfo)) {
      WaitForSingleObject(shExInfo.hProcess, INFINITE);
      returnCode = 0; // Install successfull
    } else {
      returnCode = 1; // Install failed
    }
  }
  return returnCode;
}

class WebViewEdge : public wxWebViewEdge
{
public:
    WebViewEdge(const wxWebViewConfiguration& config, bool defer_navigation)
        : wxWebViewEdge(config)
        , m_defer_navigation(defer_navigation)
    {
        Bind(wxEVT_WEBVIEW_CREATED, &WebViewEdge::OnWebViewCreated, this);
    }

    void LoadURL(const wxString& url) override
    {
        if (m_defer_navigation && GetNativeBackend() == nullptr) {
            // WebView2 initialization is asynchronous. During GUI recreation,
            // the old controls may still be shutting down while new controls
            // are created. Navigating before wxEVT_WEBVIEW_CREATED can fail
            // with ERROR_INVALID_STATE.
            pendingUrl = url;
            BOOST_LOG_TRIVIAL(warning)
                << __FUNCTION__ << ": WebView2 backend is not ready; deferring URL: "
                << url.ToStdString();
            return;
        }

        pendingUrl.clear();
        wxWebViewEdge::LoadURL(url);
    }

    bool SetUserAgent(const wxString &userAgent)
    {
        bool dark = userAgent.Contains("dark");
        SetColorScheme(dark ? COREWEBVIEW2_PREFERRED_COLOR_SCHEME_DARK : COREWEBVIEW2_PREFERRED_COLOR_SCHEME_LIGHT);

        ICoreWebView2 *webView2 = (ICoreWebView2 *) GetNativeBackend();
        if (webView2) {
            ICoreWebView2Settings *settings;
            HRESULT                hr = webView2->get_Settings(&settings);
            if (hr == S_OK) {
                ICoreWebView2Settings2 *settings2;
                hr = settings->QueryInterface(&settings2);
                if (hr == S_OK) {
                    settings2->put_UserAgent(userAgent.wc_str());
                    settings2->Release();
                    return true;
                }
            }
            settings->Release();
            return false;
        }
        pendingUserAgent = userAgent;
        return true;
    }

    bool SetColorScheme(COREWEBVIEW2_PREFERRED_COLOR_SCHEME colorScheme)
    {
        ICoreWebView2 *webView2 = (ICoreWebView2 *) GetNativeBackend();
        if (webView2) {
            ICoreWebView2_13 * webView2_13;
            HRESULT           hr = webView2->QueryInterface(&webView2_13);
            if (hr == S_OK) {
                ICoreWebView2Profile *profile;
                hr = webView2_13->get_Profile(&profile);
                if (hr == S_OK) {
                    profile->put_PreferredColorScheme(colorScheme);
                    profile->Release();
                    return true;
                }
                webView2_13->Release();
            }
            return false;
        }
        pendingColorScheme = colorScheme;
        return true;
    }

    void DoGetClientSize(int *x, int *y) const override
    {
        if (!pendingUserAgent.empty()) {
            auto thiz = const_cast<WebViewEdge *>(this);
            auto userAgent = std::move(thiz->pendingUserAgent);
            thiz->pendingUserAgent.clear();
            thiz->SetUserAgent(userAgent);
        }
        if (pendingColorScheme) {
            auto thiz      = const_cast<WebViewEdge *>(this);
            auto colorScheme = pendingColorScheme;
            thiz->pendingColorScheme = COREWEBVIEW2_PREFERRED_COLOR_SCHEME_AUTO;
            thiz->SetColorScheme(colorScheme);
        }
        wxWebViewEdge::DoGetClientSize(x, y);
    };
private:
    void OnWebViewCreated(wxWebViewEvent& event)
    {
        void* backend = GetNativeBackend();
        if (backend == nullptr) {
            BOOST_LOG_TRIVIAL(warning)
                << __FUNCTION__ << ": WebView2 created event received without a backend. webView="
                << (void*) this;
            event.Skip();
            return;
        }

        BOOST_LOG_TRIVIAL(warning)
            << __FUNCTION__ << ": WebView2 backend ready. webView=" << (void*) this
            << ", backend=" << backend;

        if (!pendingUrl.empty()) {
            const wxString url = std::move(pendingUrl);
            pendingUrl.clear();
            BOOST_LOG_TRIVIAL(warning)
                << __FUNCTION__ << ": loading deferred URL: " << url.ToStdString();
            wxWebViewEdge::LoadURL(url);
        }

        event.Skip();
    }

    const bool m_defer_navigation;
    wxString pendingUrl;
    wxString pendingUserAgent;
    COREWEBVIEW2_PREFERRED_COLOR_SCHEME pendingColorScheme = COREWEBVIEW2_PREFERRED_COLOR_SCHEME_AUTO;
};

#elif defined __WXOSX__

class WebViewWebKit : public wxWebViewWebKit
{
public:
#if wxCHECK_VERSION(3, 3, 0)
    WebViewWebKit() : wxWebViewWebKit(wxWebView::NewConfiguration(wxWebViewBackendWebKit)) {}
#endif

    ~WebViewWebKit() override
    {
        RemoveScriptMessageHandler("wx");
    }
};

#elif defined __linux__

    // TODO: release webview script message handle in Destructor
    // class WebViewWebGTK : public wxWebview
    // {
    //     ~WebViewWebGTK() override
    //     {
    //         RemoveScriptMessageHandler("wx");
    //     }
    // };

#endif

class FakeWebView : public wxWebView
{
    virtual bool Create(wxWindow* parent, wxWindowID id, const wxString& url, const wxPoint& pos, const wxSize& size, long style, const wxString& name) override { return false; }
    virtual wxString GetCurrentTitle() const override { return wxString(); }
    virtual wxString GetCurrentURL() const override { return wxString(); }
    virtual bool IsBusy() const override { return false; }
    virtual bool IsEditable() const override { return false; }
    virtual void LoadURL(const wxString& url) override { }
    virtual void Print() override { }
    virtual void RegisterHandler(wxSharedPtr<wxWebViewHandler> handler) override { }
    virtual void Reload(wxWebViewReloadFlags flags = wxWEBVIEW_RELOAD_DEFAULT) override { }
    virtual bool RunScript(const wxString& javascript, wxString* output = NULL) const override { return false; }
    virtual void SetEditable(bool enable = true) override { }
    virtual void Stop() override { }
    virtual bool CanGoBack() const override { return false; }
    virtual bool CanGoForward() const override { return false; }
    virtual void GoBack() override { }
    virtual void GoForward() override { }
    virtual void ClearHistory() override { }
    virtual void EnableHistory(bool enable = true) override { }
    virtual wxVector<wxSharedPtr<wxWebViewHistoryItem>> GetBackwardHistory() override { return {}; }
    virtual wxVector<wxSharedPtr<wxWebViewHistoryItem>> GetForwardHistory() override { return {}; }
    virtual void LoadHistoryItem(wxSharedPtr<wxWebViewHistoryItem> item) override { }
    virtual bool CanSetZoomType(wxWebViewZoomType type) const override { return false; }
    virtual float GetZoomFactor() const override { return 0.0f; }
    virtual wxWebViewZoomType GetZoomType() const override { return wxWebViewZoomType(); }
    virtual void SetZoomFactor(float zoom) override { }
    virtual void SetZoomType(wxWebViewZoomType zoomType) override { }
    virtual bool CanUndo() const override { return false; }
    virtual bool CanRedo() const override { return false; }
    virtual void Undo() override { }
    virtual void Redo() override { }
    virtual void* GetNativeBackend() const override { return nullptr; }
    virtual void DoSetPage(const wxString& html, const wxString& baseUrl) override { }
};

wxDEFINE_EVENT(EVT_WEBVIEW_RECREATED, wxCommandEvent);

static std::vector<wxWebView*> g_webviews;
static std::vector<wxWebView*> g_delay_webviews;
#ifdef __WIN32__
static bool g_webview_atexit_registered = false;
static wxString g_webview_userdata_dir;
static wxString g_webview_userdata_marker;
static bool g_webview_userdata_ephemeral = false;
static std::unique_ptr<wxWebViewConfiguration> g_webview_configuration;
static bool g_force_single_process = false;
#endif
#if defined __linux__
static bool g_linux_webview_schemes_registered = false;
#endif

class WebViewRef : public wxObjectRefData
{
public:
    WebViewRef(wxWebView *webView) : m_webView(webView) {}
    ~WebViewRef() {
        BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " wxWebView address: " << (void*) m_webView;
        auto iter = std::find(g_webviews.begin(), g_webviews.end(), m_webView);
        assert(iter != g_webviews.end());
        if (iter != g_webviews.end())
            g_webviews.erase(iter);
    }
    wxWebView *m_webView;
};

#ifdef __WIN32__
static bool is_valid_webview_profile_name(const wxString& name)
{
    return name.StartsWith("profile") && name != "." && name != ".." &&
           !name.Contains("/") && !name.Contains("\\") && !name.Contains(":");
}

static bool claim_webview_profile(const wxString& profile_dir, wxString& marker_path)
{
    if (!wxFileName::Mkdir(profile_dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL) &&
        !wxFileName::DirExists(profile_dir)) {
        return false;
    }

    wxFileName marker(profile_dir, ".running");
    HANDLE marker_handle = CreateFileW(marker.GetFullPath().wc_str(), GENERIC_WRITE, 0, nullptr,
                                       CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (marker_handle == INVALID_HANDLE_VALUE)
        return false;

    const wxString pid = wxString::Format("%lu", static_cast<unsigned long>(wxGetProcessId()));
    const wxScopedCharBuffer pid_utf8 = pid.utf8_str();
    DWORD written = 0;
    WriteFile(marker_handle, pid_utf8.data(), static_cast<DWORD>(pid_utf8.length()), &written, nullptr);
    CloseHandle(marker_handle);
    marker_path = marker.GetFullPath();
    return true;
}

static void save_active_webview_profile(const wxString& active_file_path, const wxString& profile_name)
{
    wxTextFile active_file;
    if (wxFileName::FileExists(active_file_path)) {
        if (!active_file.Open(active_file_path))
            return;
        active_file.Clear();
    } else if (!active_file.Create(active_file_path)) {
        return;
    }

    active_file.AddLine(profile_name);
    active_file.Write();
    active_file.Close();
}

static void initialize_webview_userdata_dir()
{
    wxFileName root(wxStandardPaths::Get().GetTempDir(), "");
    root.AppendDir("slicer_webview_userdata");

    const wxString root_path = root.GetFullPath();
    if (!wxFileName::Mkdir(root_path, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL) &&
        !wxFileName::DirExists(root_path)) {
        wxFileName fallback(wxStandardPaths::Get().GetTempDir(), "");
        fallback.AppendDir(wxString::Format("slicer_webview_userdata_%lu",
                                           static_cast<unsigned long>(wxGetProcessId())));
        wxFileName::Mkdir(fallback.GetFullPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
        g_webview_userdata_dir = fallback.GetFullPath();
        g_webview_userdata_ephemeral = true;
        BOOST_LOG_TRIVIAL(warning) << __FUNCTION__
                                   << ": failed to create reusable profile root; using per-run folder: "
                                   << g_webview_userdata_dir.ToStdString();
        wxSetEnv("WEBVIEW2_USER_DATA_FOLDER", g_webview_userdata_dir);
        return;
    }

    const wxString active_file_path = wxFileName(root_path, "active_profile.txt").GetFullPath();
    wxString profile_name = "profile";
    if (wxFileName::FileExists(active_file_path)) {
        wxTextFile active_file;
        if (active_file.Open(active_file_path) && active_file.GetLineCount() > 0 &&
            is_valid_webview_profile_name(active_file.GetLine(0))) {
            profile_name = active_file.GetLine(0);
        }
        if (active_file.IsOpened())
            active_file.Close();
    }

    wxFileName profile(root_path, "");
    profile.AppendDir(profile_name);
    const bool reused_profile = claim_webview_profile(profile.GetFullPath(), g_webview_userdata_marker);
    if (!reused_profile) {
        profile_name = wxString::Format("profile_%lu_%llu",
                                        static_cast<unsigned long>(wxGetProcessId()),
                                        static_cast<unsigned long long>(GetTickCount64()));
        profile.AssignDir(root_path);
        profile.AppendDir(profile_name);
        if (!claim_webview_profile(profile.GetFullPath(), g_webview_userdata_marker)) {
            g_webview_userdata_ephemeral = true;
            BOOST_LOG_TRIVIAL(warning) << __FUNCTION__
                                       << ": failed to claim reusable and fallback profiles; folder will be removed on exit.";
        }
    }

    g_webview_userdata_dir = profile.GetFullPath();
    save_active_webview_profile(active_file_path, profile_name);
    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__
                               << (reused_profile ? ": using reusable WebView2 profile: "
                                                  : ": active profile is occupied; using fallback profile: ")
                               << g_webview_userdata_dir.ToStdString();
    wxSetEnv("WEBVIEW2_USER_DATA_FOLDER", g_webview_userdata_dir);
}
#endif

wxWebView* WebView::CreateWebView(wxWindow * parent, wxString const & url)
{
    bool using_edge_fixed = false;
#if wxUSE_WEBVIEW_EDGE
    // Check if a fixed version of edge is present in
    // $executable_path/edge_fixed and use it
    wxFileName edgeFixedDir(wxStandardPaths::Get().GetExecutablePath());
    edgeFixedDir.SetFullName("");
    edgeFixedDir.AppendDir("edge_fixed");
    using_edge_fixed = edgeFixedDir.DirExists();
    if (using_edge_fixed) {
        wxWebViewEdge::MSWSetBrowserExecutableDir(edgeFixedDir.GetFullPath());
        wxLogMessage("Using fixed edge version");
    }
    // parepare for single process mode, but edge_fixed2 is not used now, so we don't set it.
    wxFileName edgeFixedDir2(wxStandardPaths::Get().GetExecutablePath());
    edgeFixedDir2.SetFullName("");
    edgeFixedDir2.AppendDir("edge_fixed2");
    if (edgeFixedDir2.DirExists()) {
        wxWebViewEdge::MSWSetBrowserExecutableDir(edgeFixedDir2.GetFullPath());
        wxLogMessage("Using fixed edge version");
    }
    if (g_webview_userdata_dir.empty())
        initialize_webview_userdata_dir();
#endif
    auto url2  = url;
#ifdef __WIN32__
    url2.Replace("\\", "/");
#endif
    if (!url2.empty()) { url2 = wxURI(url2).BuildURI(); }
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__ << ": " << url2.ToUTF8();

#ifdef __WIN32__
    wxString previous_browser_args;
    const bool had_previous_args = wxGetEnv("WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS", &previous_browser_args);
    const bool use_single_process = g_force_single_process && !using_edge_fixed;
    if (g_force_single_process && using_edge_fixed) {
        BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << ": edge_fixed detected; ignoring WebView2 single-process mode.";
    }
    if (use_single_process) {
        wxSetEnv("WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS", wxS("--single-process"));
        BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << ": enabling WebView2 single-process mode for creation.";
    }
    // Keep one Edge configuration alive for the whole process. Reusing its
    // underlying WebView2 environment prevents rapid GUI recreation from
    // closing one environment while the next controls are being initialized.
    if (!g_webview_configuration) {
        wxWebViewConfiguration config = wxWebView::NewConfiguration(wxWebViewBackendEdge);
        config.SetDataPath(g_webview_userdata_dir);
        g_webview_configuration = std::make_unique<wxWebViewConfiguration>(config);
    }
    // Initial startup keeps the old direct-navigation behaviour. GUI recreation
    // snapshots a deferred-navigation policy because m_is_recreating_gui may be
    // reset before the asynchronous WebView2 creation event is delivered.
    const bool defer_navigation = Slic3r::GUI::wxGetApp().is_recreating_gui();
    wxWebView* webView = new WebViewEdge(*g_webview_configuration, defer_navigation);
    BOOST_LOG_TRIVIAL(warning)
        << __FUNCTION__ << ": WebView2 navigation mode="
        << (defer_navigation ? "deferred (GUI recreation)" : "direct (initial startup)");
#elif defined(__WXOSX__)
    wxWebView *webView = new WebViewWebKit;
#else
    auto webView = wxWebView::New();
#endif
    if (webView) {
        webView->SetBackgroundColour(StateColor::darkModeColorFor(*wxWHITE));
#ifdef __WIN32__
        webView->SetUserAgent(wxString::Format("Creality-Slicer/v%s (%s) Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
            "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/107.0.0.0 Safari/537.36 Edg/107.0.1418.52", SLIC3R_VERSION, 
            Slic3r::GUI::wxGetApp().dark_mode() ? "dark" : "light"));
        if (defer_navigation) {
            // During GUI recreation, wait for wxEVT_WEBVIEW_CREATED before
            // navigating to avoid ERROR_INVALID_STATE while old controls close.
            webView->Create(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
            if (!url2.empty())
                webView->LoadURL(url2);
        } else {
            // On initial startup let wxWebView2 handle the initial URL as part
            // of Create(), matching the pre-10ece62d62 behaviour.
            webView->Create(parent, wxID_ANY, url2, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
        }
        // We register the wxfs:// protocol for testing purposes
        webView->RegisterHandler(wxSharedPtr<wxWebViewHandler>(new wxWebViewArchiveHandler("bbl")));
        // And the memory: file system
        webView->RegisterHandler(wxSharedPtr<wxWebViewHandler>(new wxWebViewFSHandler("memory")));
#elif defined __linux__
        if (webView) {
            std::ifstream osRelease("/etc/os-release");
            std::string   line;
            bool          isUOS = false;
            while (std::getline(osRelease, line)) {
                if (line.find("UOS") != std::string::npos || line.find("UnionTech") != std::string::npos) {
                    isUOS = true;
                    break;
                }
            }
            if (isUOS) {
                webView->SetUserAgent(
                    wxString::Format("Creality-Slicer/v%s (%s; UOS)", SLIC3R_VERSION, Slic3r::GUI::wxGetApp().dark_mode() ? "dark" : "light"));
                setenv("WEBKIT_DISABLE_TLS_VERIFICATION", "0", 0);
                setenv("G_TLS_GNUTLS_PRIORITY", "NORMAL:-VERS-ALL:+VERS-TLS1.2", 1);
                setenv("WEBKIT_TLS_ERRORS_POLICY", "ignore-tls-errors", 0);
                BOOST_LOG_TRIVIAL(info) << "Set TLS 1.2 as default for WebView on Linux";
            } else {
                webView->SetUserAgent(
                    wxString::Format("Creality-Slicer/v%s (%s; Linux) Mozilla/5.0 (X11; Linux) AppleWebKit/537.36 (KHTML, like Gecko)",
                                     SLIC3R_VERSION, Slic3r::GUI::wxGetApp().dark_mode() ? "dark" : "light"));
            }
            if (!g_linux_webview_schemes_registered) {
                webView->RegisterHandler(wxSharedPtr<wxWebViewHandler>(new wxWebViewArchiveHandler("wxfs")));
                webView->RegisterHandler(wxSharedPtr<wxWebViewHandler>(new wxWebViewFSHandler("memory")));
                g_linux_webview_schemes_registered = true;
            }
            webView->Create(parent, wxID_ANY, url2, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
        }
#else
        // With WKWebView handlers need to be registered before creation
        webView->RegisterHandler(wxSharedPtr<wxWebViewHandler>(new wxWebViewArchiveHandler("wxfs")));
        // And the memory: file system
        webView->RegisterHandler(wxSharedPtr<wxWebViewHandler>(new wxWebViewFSHandler("memory")));
        webView->Create(parent, wxID_ANY, url2, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
        webView->SetUserAgent(wxString::Format("Creality-Slicer/v%s (%s) Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko)", SLIC3R_VERSION,
                                               Slic3r::GUI::wxGetApp().dark_mode() ? "dark" : "light"));
#endif
#ifdef __WXMAC__
        WKWebView * wkWebView = (WKWebView *) webView->GetNativeBackend();
        Slic3r::GUI::WKWebView_setTransparentBackground(wkWebView);
#endif

        auto addScriptMessageHandler = [] (wxWebView *webView) {
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ": begin to add script message handler for wx.";
            Slic3r::GUI::wxGetApp().set_adding_script_handler(true);
            if (!webView->AddScriptMessageHandler("wx"))
                wxLogError("Could not add script message handler");
            Slic3r::GUI::wxGetApp().set_adding_script_handler(false);
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ": finished add script message handler for wx.";
        };
#ifndef __WIN32__
        webView->CallAfter([webView, addScriptMessageHandler] {
#endif
            if (Slic3r::GUI::wxGetApp().is_adding_script_handler()) {
                g_delay_webviews.push_back(webView);
            } else {
                addScriptMessageHandler(webView);
                while (!g_delay_webviews.empty()) {
                    auto views = std::move(g_delay_webviews);
                    for (auto wv : views)
                        addScriptMessageHandler(wv);
                }
            }
#ifndef __WIN32__
        });
#endif
        webView->EnableContextMenu(true);
    } else {
        BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << ": failed. Use fake web view.";
        webView = new FakeWebView;
    }
    webView->SetRefData(new WebViewRef(webView));
    g_webviews.push_back(webView);
#ifdef __WIN32__
    if (use_single_process) {
        if (had_previous_args)
            wxSetEnv("WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS", previous_browser_args);
        else
            wxUnsetEnv("WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS");
    }
    if (!g_webview_atexit_registered) {
        g_webview_atexit_registered = true;
        std::atexit([] {
            // [诊断日志] 记录 atexit 触发时 g_webviews 的大小。
            // 若为 0，说明 wx 对象树已在 atexit 之前析构完毕，DestroyAll 将无法
            // 收集到 WebView2 子进程 pid，子进程可能残留在内存中。
            // 若不为 0，说明 atexit 路径可正常回收子进程。
            BOOST_LOG_TRIVIAL(warning)
                << "[WebView][atexit] triggered, g_webviews.size()=" << g_webviews.size()
                << " (0 means wx already destroyed webviews before atexit fired"
                << " — orphan msedgewebview2.exe processes may remain)";
            WebView::DestroyAll();
        });
    }
#endif
    return webView;
}
#if wxUSE_WEBVIEW_EDGE
void WebView::SetForceSingleProcess(bool force_single_process)
{
#ifdef __WIN32__
    g_force_single_process = force_single_process;
#else
    (void)force_single_process;
#endif
}
#endif
#if wxUSE_WEBVIEW_EDGE
bool WebView::CheckWebViewRuntime()
{
    wxWebViewFactoryEdge factory;
    auto wxVersion = factory.GetVersionInfo(wxVersionContext::RunTime);
    return wxVersion.GetMajor() != 0;
}
bool WebView::ReInstallWebViewRuntime()
{
    int returnCode = 2; // Download failed
    SHELLEXECUTEINFOW shExInfo = {0};
    //移除edge
    fs::path remove_edge_path = (fs::path(Slic3r::resources_dir()).parent_path() / "Remove-Edge.exe");
    //SHELLEXECUTEINFOW shExInfo = {0};
    shExInfo.cbSize = sizeof(shExInfo);
    shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    shExInfo.hwnd = 0;
    shExInfo.lpVerb = L"runas";
    shExInfo.lpFile = remove_edge_path.generic_wstring().c_str();
    shExInfo.lpParameters = L" /install";
    shExInfo.lpDirectory = 0;
    shExInfo.nShow = 0;
    shExInfo.hInstApp = 0;

    if (ShellExecuteExW(&shExInfo)) {
      WaitForSingleObject(shExInfo.hProcess, INFINITE);
      returnCode = 0; // Install successfull
    } else {
      returnCode = 1; // remove failed
      return false;
    }
    //重新安装webview2 runtime
    fs::path target_file_path = (fs::path(Slic3r::resources_dir()).parent_path() / "MicrosoftEdgeWebView2RuntimeInstallerX64.exe");
    // Either Package the WebView2 Bootstrapper with your app or download it using fwlink
    // Then invoke install at Runtime.
    //SHELLEXECUTEINFOW shExInfo = {0};
    shExInfo.cbSize = sizeof(shExInfo);
    shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    shExInfo.hwnd = 0;
    shExInfo.lpVerb = L"runas";
    shExInfo.lpFile = target_file_path.generic_wstring().c_str();
    shExInfo.lpParameters = L" /install";
    shExInfo.lpDirectory = 0;
    shExInfo.nShow = 0;
    shExInfo.hInstApp = 0;

    if (ShellExecuteExW(&shExInfo)) {
      WaitForSingleObject(shExInfo.hProcess, INFINITE);
      returnCode = 0; // Install successfull
    } else {
      returnCode = 1; // Install failed
    }
    //重新安装edge
    fs::path edge_file_path = (fs::path(Slic3r::resources_dir()).parent_path() / "MicrosoftEdgeSetup.exe");
    // Either Package the WebView2 Bootstrapper with your app or download it using fwlink
    // Then invoke install at Runtime.
    // 判断edge安装文件是否存在
    if (fs::exists(edge_file_path)) {
        shExInfo.cbSize = sizeof(shExInfo);
        shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        shExInfo.hwnd = 0;
        shExInfo.lpVerb = L"runas";
        shExInfo.lpFile = edge_file_path.generic_wstring().c_str();
        shExInfo.lpParameters = L" /install";
        shExInfo.lpDirectory = 0;
        shExInfo.nShow = 0;
        shExInfo.hInstApp = 0;

        if (ShellExecuteExW(&shExInfo)) {
        WaitForSingleObject(shExInfo.hProcess, INFINITE);
        returnCode = 0; // Install successfull
        } else {
        returnCode = 1; // Install failed
        }
    }
    return returnCode == 0;
}
bool WebView::DownloadAndInstallWebViewRuntime()
{
    return DownloadAndInstallWV2RT() == 0;
}
#endif
void WebView::LoadUrl(wxWebView * webView, wxString const &url)
{
    auto url2  = url;
#ifdef __WIN32__
    url2.Replace("\\", "/");
#endif
    if (!url2.empty()) { url2 = wxURI(url2).BuildURI(); }
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__ << url2.ToUTF8();
    webView->LoadURL(url2);
}

bool WebView::RunScript(wxWebView *webView, wxString const &javascript)
{
    if (Slic3r::GUI::wxGetApp().app_config->get("internal_developer_mode") == "true"
            && javascript.find("studio_userlogin") == wxString::npos)
        wxLogMessage("Running JavaScript:\n%s\n", javascript);

    try {
#ifdef __WIN32__
        ICoreWebView2 *   webView2 = (ICoreWebView2 *) webView->GetNativeBackend();
        if (webView2 == nullptr)
            return false;
        return webView2->ExecuteScript(javascript, NULL) == 0;
#elif defined __WXMAC__
        WKWebView * wkWebView = (WKWebView *) webView->GetNativeBackend();
        Slic3r::GUI::WKWebView_evaluateJavaScript(wkWebView, javascript, nullptr);
        return true;
#else
        WebKitWebView *wkWebView = (WebKitWebView *) webView->GetNativeBackend();
        webkit_web_view_run_javascript(
            wkWebView, javascript.utf8_str(), NULL,
            [](GObject *wkWebView, GAsyncResult *res, void *) {
                GError * error = NULL;
                auto result = webkit_web_view_run_javascript_finish((WebKitWebView*)wkWebView, res, &error);
                if (!result)
                    g_error_free (error);
                else
                    webkit_javascript_result_unref (result);
        }, NULL);
        return true;
#endif
    } catch (std::exception &e) {
        return false;
    }
}

void WebView::DestroyAll()
{
    // caller 字段用于区分是从 GUI_App::OnExit（主动调用，时机可控）
    // 还是从 atexit 回调（被动兜底，时机不可控）进入的。
    // 结合上方 atexit 日志里的 g_webviews.size() 可以判断本次调用是否有效。
    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " destroying " << g_webviews.size() << " webviews";

#ifdef __WIN32__
    // Before destroying wxWebView controls, collect the browser process IDs
    // that belong to *our* WebView2 instances via ICoreWebView2.
    // This is the only reliable way to identify our own msedgewebview2.exe
    // processes without accidentally touching those of other applications
    // (e.g. VS Code, Teams) that also use WebView2 on the same machine.
    std::vector<DWORD> our_browser_pids;
    for (auto *webView : g_webviews) {
        if (!webView) continue;
        ICoreWebView2 *wv2 = reinterpret_cast<ICoreWebView2 *>(webView->GetNativeBackend());
        if (wv2) {
            // get_BrowserProcessId is a member of ICoreWebView2 directly.
            UINT32 pid = 0;
            if (SUCCEEDED(wv2->get_BrowserProcessId(&pid)) && pid != 0) {
                our_browser_pids.push_back(static_cast<DWORD>(pid));
                BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " found our WebView2 browser pid: " << pid;
            }
        }
        webView->Stop();
        webView->LoadURL("about:blank");
    }
#endif
    g_webviews.clear();

#ifdef __WIN32__
    // Wait for our browser processes to exit, then clean up the user data folder.
    if (!our_browser_pids.empty()) {
        BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " waiting for " << our_browser_pids.size() << " WebView2 browser process(es) to exit";

        std::vector<HANDLE> handles;
        std::vector<DWORD> opened_pids; // parallel: only PIDs whose OpenProcess succeeded
        handles.reserve(our_browser_pids.size());
        opened_pids.reserve(our_browser_pids.size());
        for (DWORD pid : our_browser_pids) {
            HANDLE h = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pid);
            if (h) {
                handles.push_back(h);
                opened_pids.push_back(pid);
            } else {
                BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " failed to open WebView2 browser process " << pid;
            }
        }

        if (!handles.empty()) {
            // Wait up to 5 seconds for graceful exit.
            DWORD waitResult = WaitForMultipleObjects(
                static_cast<DWORD>(handles.size()),
                handles.data(), TRUE, 5000);

            if (waitResult == WAIT_TIMEOUT) {
                BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " WebView2 browser process(es) did not exit, terminating";
                for (size_t i = 0; i < handles.size(); ++i) {
                    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " terminating WebView2 browser process " << opened_pids[i];
                    TerminateProcess(handles[i], 1);
                }
                WaitForMultipleObjects(
                    static_cast<DWORD>(handles.size()),
                    handles.data(), TRUE, 2000);
            }

            for (HANDLE h : handles)
                CloseHandle(h);
        } else {
            // All OpenProcess calls failed but child processes may still be alive
            // (e.g. running under a different privilege level). Give them time to
            // exit naturally before we try to remove the user data folder.
            BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " could not open any WebView2 process, sleeping 5s as fallback";
            Sleep(5000);
        }
        BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " WebView2 browser process(es) finished";
    }


#endif
    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " done";
}

#if wxUSE_WEBVIEW_EDGE
void WebView::ReleaseConfiguration()
{
#ifdef __WIN32__
    // Release the shared WebView2 environment before wxWidgets unloads its
    // WebView module. Leaving this to the CRT static destructors is too late.
    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " releasing shared WebView2 configuration";
    g_webview_configuration.reset();

    // Only mark the profile reusable after the shared WebView2 environment is released.
    if (!g_webview_userdata_marker.empty()) {
        if (!wxRemoveFile(g_webview_userdata_marker))
            BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " failed to remove WebView2 profile marker: "
                                       << g_webview_userdata_marker.ToStdString();
        g_webview_userdata_marker.clear();
    }
    if (g_webview_userdata_ephemeral && !g_webview_userdata_dir.empty())
        wxFileName::Rmdir(g_webview_userdata_dir, wxPATH_RMDIR_RECURSIVE);
    g_webview_userdata_dir.clear();
    g_webview_userdata_ephemeral = false;
#endif
}
#endif

void WebView::RecreateAll()
{
    BOOST_LOG_TRIVIAL(warning) <<__FUNCTION__ << " start";
    auto dark = Slic3r::GUI::wxGetApp().dark_mode();
    for (auto webView : g_webviews) {
        void* backend_before = webView->GetNativeBackend();
        BOOST_LOG_TRIVIAL(warning) << "[RECREATE_ALL] Reloading webView " << (void*) webView << ". Backend BEFORE: " << backend_before;

        webView->SetUserAgent(wxString::Format("Creality-Slicer/v%s (%s) Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko)", SLIC3R_VERSION,
                                               dark ? "dark" : "light"));
        webView->Reload();

        void* backend_after = webView->GetNativeBackend();
        BOOST_LOG_TRIVIAL(warning) << "[RECREATE_ALL] Reloaded webView " << (void*) webView << ". Backend AFTER: " << backend_after;

    }
    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " end";
    boost::log::core::get()->flush();
}
