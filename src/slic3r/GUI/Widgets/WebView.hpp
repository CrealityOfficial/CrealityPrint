#ifndef slic3r_GUI_WebView_hpp_
#define slic3r_GUI_WebView_hpp_

#include <wx/webview.h>

class WebView
{
public:
    static wxWebView *CreateWebView(wxWindow *parent, wxString const &url);
    static bool ConfigureHardwareAccelerationForMjpeg(wxWebView *webView);
#if wxUSE_WEBVIEW_EDGE
    static bool CheckWebViewRuntime();
    static bool DownloadAndInstallWebViewRuntime();
    static bool ReInstallWebViewRuntime();
    static void SetForceSingleProcess(bool force_single_process);
    static void ReleaseConfiguration();
#endif
    static void LoadUrl(wxWebView * webView, wxString const &url);

    // Normal exit is split so wxWidgets can destroy every real WebView before
    // the shared WebView2 environment and browser processes are finalized.
    static void BeginShutdown();
    static void FinalizeShutdown();
    static bool IsShuttingDown();

    static bool RunScript(wxWebView * webView, wxString const & msg);

    static void RecreateAll();
};

#endif // !slic3r_GUI_WebView_hpp_
