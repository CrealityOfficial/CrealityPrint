#ifndef TEXTDISPLAY_HPP
#define TEXTDISPLAY_HPP

#include <wx/wx.h>
#include <wx/statbox.h>
#include <wx/dcbuffer.h>
#include "StaticBox.hpp"

class TextDisplay : public wxNavigationEnabled<StaticBox>
{
public:
    TextDisplay();
    TextDisplay(wxWindow* parent, const wxString& text, const wxString& icon, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0);

    void SetIcon(const wxString& icon);
    void SetLabel(const wxString& label);
    void SetTextLeft(bool text_left);
    void SetLabelColor(StateColor const &color);

protected:
    void paintEvent(wxPaintEvent& evt);

    void render(wxDC& dc);

    void messureSize();

    void OnMouseMove(wxMouseEvent& event);

    void Create(wxWindow* parent, const wxString& text, const wxString& icon, const wxPoint& pos, const wxSize& size, long style);

private:
    ScalableBitmap m_icon;
    bool m_text_left;
    wxSize labelSize;
    StateColor     label_color;
    wxRect m_iconRect;

    wxDECLARE_EVENT_TABLE();
};

#endif // TEXTDISPLAY_HPP