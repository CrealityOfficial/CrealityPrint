#include "HoverBorderBox.hpp"
#include "Label.hpp"
#include <sys/stat.h>
#include <wx/gdicmn.h>
#include "../GUI_App.hpp"

BEGIN_EVENT_TABLE(HoverBorderBox, wxPanel)
    EVT_PAINT(HoverBorderBox::paintEvent)
    EVT_MOTION(HoverBorderBox::OnMouseMove)
END_EVENT_TABLE()

HoverBorderBox::HoverBorderBox()
{
}

HoverBorderBox::HoverBorderBox(wxWindow* parent, wxWindow* insideChild, const wxPoint& pos, const wxSize& size, long style)
{
    Create(parent, insideChild, pos, size, style);
}

void HoverBorderBox::Create(wxWindow* parent, wxWindow* insideChild, const wxPoint& pos, const wxSize& size, long style)
{
    bool is_dark = Slic3r::GUI::wxGetApp().dark_mode();

    StaticBox::Create(parent, wxID_ANY, pos, size, style);
    SetCornerRadius(6);

    StaticBox::SetBorderColor(StateColor(std::make_pair(is_dark ? 0x4B4B4D : 0xFFFFFF, (int) StateColor::Disabled),
                                         std::make_pair(0x15BF59, (int) StateColor::Hovered),
                                         std::make_pair(is_dark ? 0x4B4B4D : 0xFFFFFF, (int) StateColor::Normal)));
    StaticBox::SetBackgroundColor(
        StateColor(std::make_pair(0xF0F0F1, (int) StateColor::Disabled),
                   std::make_pair(wxColour("#787565"), (int) StateColor::Focused), // ORCA updated background color for focused item
                   std::make_pair(*wxWHITE, (int) StateColor::Normal)));

    state_handler.update_binds();

    if(insideChild)
    {
        insideChild->Reparent(this);
        insideChild->Bind(wxEVT_MOTION, &HoverBorderBox::OnMouseMove, this);
        insideChild->Bind(wxEVT_PAINT, &HoverBorderBox::OnChildPaint, this);
    }

    wxBoxSizer* tmp_sizer = new wxBoxSizer(wxVERTICAL);
    tmp_sizer->Add(insideChild, 1, wxEXPAND | wxALL, FromDIP(5));
    SetSizer(tmp_sizer);

}

void HoverBorderBox::SetToolTip_( const wxString &tip )
{
    m_hover_tip = tip;
    SetToolTip(tip);
}

void HoverBorderBox::on_change_color_mode(bool is_dark)
{
    StaticBox::SetBorderColor(StateColor(std::make_pair(is_dark ? 0x4B4B4D : 0xFFFFFF, (int) StateColor::Disabled),
                                         std::make_pair(0x15BF59, (int) StateColor::Hovered),
                                         std::make_pair(is_dark ? 0x4B4B4D : 0xFFFFFF, (int) StateColor::Normal)));

    state_handler.update_binds();

    Refresh();
}

void HoverBorderBox::OnMouseMove(wxMouseEvent& event)
{
    SetCursor(wxCURSOR_HAND);
    int cur_state = GetState();
    if(cur_state != 9)  // StateColor::Hovered
    {
        state_handler.set_state(9, 9);
    }
    Refresh();
    event.Skip();
}

void HoverBorderBox::paintEvent(wxPaintEvent& evt)
{
    wxPaintDC dc(this);
    render(dc);
}

void HoverBorderBox::render(wxDC& dc)
{
    StaticBox::render(dc);
}

void HoverBorderBox::OnChildPaint(wxPaintEvent& evt)
{
    // Call the parent's paint event
    paintEvent(evt);
    evt.Skip();
}