#ifndef slic3r_MarkdownTip_hpp_
#define slic3r_MarkdownTip_hpp_

#include <wx/popupwin.h>
#include <wx/timer.h>
#include <wx/webview.h>


namespace Slic3r { namespace GUI {

class MarkdownTip : public wxPopupTransientWindow
{
public:
    static bool ShowTip(std::string const& tip, std::string const& tooltip, wxPoint pos, std::string img = "");

    static void ExitTip();

    static void Reload();

    static void Recreate(wxWindow *parent);

    static wxWindow* AttachTo(wxWindow * parent);

    static wxWindow* DetachFrom(wxWindow * parent);

protected:
    MarkdownTip();
    ~MarkdownTip();

    void RunScript(std::string const& script);

private:
    static MarkdownTip* markdownTip(bool create = true);

    void LoadStyle();

    bool ShowTip(wxPoint pos, std::string const& tip, std::string const& tooltip, std::string img="");

    std::string LoadTip(std::string const &tip, std::string const &tooltip);

    

protected:
    wxWebView* CreateTipView(wxWindow* parent);

    void OnLoaded(wxWebViewEvent& event);

    void OnTitleChanged(wxWebViewEvent& event);

    void OnError(wxWebViewEvent& event);

    void OnTimer(wxTimerEvent& event);
    
protected:
    wxWebView * _tipView = nullptr;
    std::string _lastTip;
    std::string _pendingScript = " ";
    std::string _language;
    wxPoint _requestPos;
    double _lastHeight = 0;
    wxTimer* _timer = nullptr;
    bool _hide = false;
    bool _data_dir = false;
};

}
}

#endif
