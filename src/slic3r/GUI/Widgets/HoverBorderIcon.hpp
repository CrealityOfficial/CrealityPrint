#ifndef HOVERBORDERICON_HPP
#define HOVERBORDERICON_HPP

#include <wx/wx.h>
#include <wx/statbox.h>
#include <wx/dcbuffer.h>
#include <vector>
#include "StaticBox.hpp"

class HoverBorderIcon : public wxNavigationEnabled<StaticBox>
{
public:
    HoverBorderIcon();
    HoverBorderIcon(wxWindow* parent, const wxString& text, const wxString& icon, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0);
    ~HoverBorderIcon() override;

    void SetIcon(const wxString& icon);
    bool SetBitmap_(const std::string& bmp_name);
    void SetToolTip_( const wxString &tip );
    void on_sys_color_changed(bool is_dark_mode);
    void msw_rescale();
    void set_force_paint(bool force_paint) { m_force_paint = force_paint; }
    void setEnable(bool enable);
    void setDisableIcon(const wxString& disableIconName, int px_cnt = 18);

    void SetIconScaleFactor(double factor);
    void StartBusyAnimation(const wxString& tooltip = wxEmptyString);
    void StopBusyAnimation();
    bool IsBusy() const { return m_busy; }
protected:
    void paintEvent(wxPaintEvent& evt);

    virtual void render(wxDC& dc);

    void messureSize();

    void OnMouseMove(wxMouseEvent& event);
    void OnBusyTimer(wxTimerEvent& event);
#ifdef __APPLE__
    void OnMouseEnter(wxMouseEvent& event);
    void OnMouseLeave(wxMouseEvent& event);
#endif //__APPLE__

    void Create(wxWindow* parent, const wxString& text, const wxString& icon, const wxPoint& pos, const wxSize& size, long style);
    void rebuild_busy_frames();

    ScalableBitmap m_icon;
    wxRect m_iconRect;
    wxString m_hover_tip;
    bool m_force_paint;
    bool           m_bSetEnable = true;
    ScalableBitmap m_bmpDiableIcon;
    wxSize         m_fixed_size_dip     = wxDefaultSize; // logical (DIP) size when created with explicit size
    int            m_corner_radius_dip  = 6;
    int            m_disable_icon_px_cnt_dip = 0;
    double         m_icon_size_or_scale = 13.0;
    double         m_icon_rel_scale     = 0.6; // default: 60% of min DIP size
    wxTimer        m_busy_timer;
    std::vector<ScalableBitmap> m_busy_frames;
    size_t         m_busy_frame = 0;
    bool           m_busy       = false;


    wxDECLARE_EVENT_TABLE();
};

class ImgBtn : public HoverBorderIcon
{
public:
    ImgBtn();
    ImgBtn(wxWindow*       parent,
           const wxString& text,
           const wxString& icon,
           const wxPoint&  pos   = wxDefaultPosition,
           const wxSize&   size  = wxDefaultSize,
           long            style = 0);
    ~ImgBtn();
    void SetSelected(bool selected);
    bool IsSelected();

protected:
    void render(wxDC& dc) override;
    void Create(wxWindow* parent, const wxString& text, const wxString& icon, const wxPoint& pos, const wxSize& size, long style);
};

#endif // TEXTDISPLAY_HPP
