#ifndef HOVERBORDERICON_HPP
#define HOVERBORDERICON_HPP

#include <wx/wx.h>
#include <wx/statbox.h>
#include <wx/dcbuffer.h>
#include "StaticBox.hpp"

class HoverBorderIcon : public wxNavigationEnabled<StaticBox>
{
public:
    HoverBorderIcon();
    HoverBorderIcon(wxWindow* parent, const wxString& text, const wxString& icon, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0);

    void SetIcon(const wxString& icon);
    bool SetBitmap_(const std::string& bmp_name);
    void SetToolTip_( const wxString &tip );
    void on_sys_color_changed(bool is_dark_mode);
    void msw_rescale();
    void set_force_paint(bool force_paint) { m_force_paint = force_paint; }
    void setEnable(bool enable);
    void setDisableIcon(const wxString& disableIconName);

protected:
    void paintEvent(wxPaintEvent& evt);

    void render(wxDC& dc);

    void messureSize();

    void OnMouseMove(wxMouseEvent& event);

    void Create(wxWindow* parent, const wxString& text, const wxString& icon, const wxPoint& pos, const wxSize& size, long style);

private:
    ScalableBitmap m_icon;
    wxRect m_iconRect;
    wxString m_hover_tip;
    bool m_force_paint;
    bool           m_bSetEnable = true;
    ScalableBitmap m_bmpDiableIcon;


    wxDECLARE_EVENT_TABLE();
};

#endif // TEXTDISPLAY_HPP