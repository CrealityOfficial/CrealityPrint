#ifndef slic3r_MarkdownTip_hpp_
#define slic3r_MarkdownTip_hpp_

#include <wx/popupwin.h>
#include <wx/timer.h>
#include <wx/webview.h>


class StaticBox;
class wxHyperlinkCtrl;
namespace Slic3r { namespace GUI {
    class BitmapCache;
    class ProcessTip : public wxPopupTransientWindow
    {
        public:
        struct ContentS
        {
            wxString CS_Title;
            wxString CS_Content;
            wxString CS_Key;
            wxString CS_Image;
            wxString CS_URL;
            wxString CS_UrlText;
        };

        
        static bool ShowTip(wxString const& tip,
                     wxString const& tooltip_title,
                     wxString const& tooltip_content,
                     wxString const& tooltip_key,
                     wxString const& tooltip_img,
                     wxString const& tooltip_url,
                     wxPoint            pos);

        bool        ShowTip(wxPoint pos);
        void        closeTip();
        static ProcessTip* processTip(bool create = true);

        static void Recreate(wxWindow *parent);
        void OnMouseEvent(wxMouseEvent& event);
        void setLineRect(wxRect lineRect) { m_lineRect = lineRect;}
    protected:
        ~ProcessTip();
        void OnTimer(wxTimerEvent& event);

    private:
        ProcessTip();
        void updateUI();
        void themeChanged();
        void OnPaint(wxPaintEvent& evt);
        wxRect m_lineRect;
        ContentS m_LastContent;
        ContentS m_Content;
        wxTimer* m_Timer = nullptr;
        bool m_Hide = false;
        wxStaticText* m_Title_text = nullptr;
        wxStaticText* m_Content_text = nullptr;
        wxStaticText* m_Url_text = nullptr;
        wxStaticText* m_KeyText = nullptr;
        wxStaticBitmap* m_ProcessImg = nullptr;
        StaticBox* m_ImgBox = nullptr;
        BitmapCache* m_bitmap_cache = nullptr;
        wxPoint m_pos;
    };

    class MarkdownTip : public wxPopupTransientWindow
{
public:
    struct ContentS
    {
        std::string CS_Title;
        std::string CS_Content;
        std::string CS_Image;
        std::string CS_URL;
    };
    static bool ShowTip(std::string const& tip, std::string const& tooltip, wxPoint pos, std::string img = "");

    static bool ShowTip(std::string const& tip,
                        std::string const& tooltip_title,
                        std::string const& tooltip_content,
                        std::string const& tooltip_img,
                        std::string const& tooltip_url, 
                         wxPoint pos);

    static void ExitTip();

    static void Reload();

    static void Recreate(wxWindow *parent);

    static wxWindow* AttachTo(wxWindow * parent);

    static wxWindow* DetachFrom(wxWindow * parent);

    static MarkdownTip* instance();

protected:
    MarkdownTip();
    ~MarkdownTip();

    void RunScript(std::string const& script);

private:
    static MarkdownTip* markdownTip(bool create = true);

    void LoadStyle();

    bool ShowTip(wxPoint pos, std::string const& tip, std::string const& tooltip, std::string img="");
    bool ShowTip(wxPoint pos, std::string const& tip);

    std::string LoadTip(std::string const &tip, std::string const &tooltip);

    

protected:
    wxWebView* CreateTipView(wxWindow* parent);

    void OnLoaded(wxWebViewEvent& event);

    void OnTitleChanged(wxWebViewEvent& event);

    void OnError(wxWebViewEvent& event);

    void OnTimer(wxTimerEvent& event);
    
protected:
    ContentS    m_Content;
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
