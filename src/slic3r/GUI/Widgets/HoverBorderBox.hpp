#ifndef HOVERBORDERBOX_HPP
#define HOVERBORDERBOX_HPP

#include <wx/window.h>
#include <wx/wx.h>
#include <wx/statbox.h>
#include <wx/dcbuffer.h>
#include "StaticBox.hpp"

class HoverBorderBox : public wxNavigationEnabled<StaticBox>
{
public:
    HoverBorderBox();
    HoverBorderBox(wxWindow* parent, wxWindow* insideChild, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0);

    void SetToolTip_( const wxString &tip );
    void on_change_color_mode(bool is_dark);

protected:
    void paintEvent(wxPaintEvent& evt);

    void render(wxDC& dc);

    void OnChildPaint(wxPaintEvent& evt);

    void messureSize();

    void OnMouseMove(wxMouseEvent& event);

    void Create(wxWindow* parent, wxWindow* insideChild, const wxPoint& pos, const wxSize& size, long style);

private:
    wxRect m_iconRect;
    wxString m_hover_tip;

    wxDECLARE_EVENT_TABLE();
};

#endif // TEXTDISPLAY_HPP