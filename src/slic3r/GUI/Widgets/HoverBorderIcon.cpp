#include "HoverBorderIcon.hpp"
#include "Label.hpp"
#include <wx/gdicmn.h>
#include <wx/app.h> // Add this line to include the header file for wxGetApp
#include <wx/wx.h>
#include "slic3r/GUI/GUI_App.hpp"

BEGIN_EVENT_TABLE(HoverBorderIcon, wxPanel)
    EVT_PAINT(HoverBorderIcon::paintEvent)
    EVT_MOTION(HoverBorderIcon::OnMouseMove)
END_EVENT_TABLE()

HoverBorderIcon::HoverBorderIcon()
    :m_force_paint(true)
{
}

HoverBorderIcon::HoverBorderIcon(wxWindow* parent, const wxString& text, const wxString& icon, const wxPoint& pos, const wxSize& size, long style)
    :m_force_paint(true)
{
    Create(parent, text, icon, pos, size, style);
}

void HoverBorderIcon::Create(wxWindow* parent, const wxString& text, const wxString& icon, const wxPoint& pos, const wxSize& size, long style)
{
    bool is_dark = Slic3r::GUI::wxGetApp().dark_mode();
    
    StaticBox::Create(parent, wxID_ANY, pos, size, style);
    SetCornerRadius(6);
    SetLabel(text);

    StaticBox::SetFont(Label::Body_13);
    StaticBox::SetBorderColor(StateColor(std::make_pair(is_dark ? 0x4B4B4D : 0xFFFFFF, (int) StateColor::Disabled),
                                         std::make_pair(0x15BF59, (int) StateColor::Hovered),
                                         std::make_pair(is_dark ?  0x4B4B4D : 0xFFFFFF, (int) StateColor::Normal)));
    StaticBox::SetBackgroundColor(
        StateColor(std::make_pair(0xF0F0F1, (int) StateColor::Disabled),
                   std::make_pair(wxColour("#787565"), (int) StateColor::Focused), // ORCA updated background color for focused item
                   std::make_pair(*wxWHITE, (int) StateColor::Normal)));

    state_handler.update_binds();
    if (!icon.IsEmpty()) {
        int emUnit = Slic3r::GUI::wxGetApp().em_unit();
        this->m_icon = ScalableBitmap(this, icon.ToStdString(), FromDIP(13) * (10.0f /emUnit));
    }
}

bool HoverBorderIcon::SetBitmap_(const std::string& bmp_name)
{
    SetIcon(bmp_name);
    return true;
}

void HoverBorderIcon::SetToolTip_( const wxString &tip )
{
    m_hover_tip = tip;
    SetToolTip(tip);
}

void HoverBorderIcon::SetIcon(const wxString& icon)
{
    if (this->m_icon.name() == icon.ToStdString())
        return;
    this->m_icon = ScalableBitmap(this, icon.ToStdString(), FromDIP(14));
}

void HoverBorderIcon::on_sys_color_changed(bool is_dark_mode)
{
    StaticBox::SetBorderColor(StateColor(std::make_pair(is_dark_mode ? 0x4B4B4D : 0xFFFFFF, (int) StateColor::Disabled),
                                         std::make_pair(0x15BF59, (int) StateColor::Hovered),
                                         std::make_pair(is_dark_mode ? 0x4B4B4D : 0xFFFFFF, (int) StateColor::Normal)));

    state_handler.update_binds();

    Refresh();

    m_force_paint = true;
}

void HoverBorderIcon::msw_rescale()
{
    Slic3r::GUI::wxGetApp().UpdateDarkUI(this); 
}

void HoverBorderIcon::setEnable(bool enable)
{
    m_bSetEnable = enable;
    if (m_bSetEnable) {
        this->SetToolTip(m_hover_tip);
    } else {
        this->SetToolTip("");
    }
    Refresh();
}

void HoverBorderIcon::setDisableIcon(const wxString& disableIconName)
{
    m_bmpDiableIcon = ScalableBitmap(this, disableIconName.ToStdString(), FromDIP(18));
}

void HoverBorderIcon::OnMouseMove(wxMouseEvent& event)
{
    SetCursor(wxCURSOR_HAND);
    Refresh();
    event.Skip();
}

void HoverBorderIcon::paintEvent(wxPaintEvent& evt)
{
    if(!m_force_paint)
    {
        if(!IsEnabled() || !IsShown())
            return;
    }

    wxPaintDC dc(this);
    render(dc);

}

void HoverBorderIcon::render(wxDC& dc)
{
    wxSize size        = GetSize();

    if (m_bSetEnable) {
        StaticBox::render(dc);
        if (m_icon.bmp().IsOk()) {
            wxSize  szIcon = m_icon.GetBmpSize();
            wxPoint iconPt = {size.x - szIcon.x - 5, (size.y - szIcon.y) / 2};
            m_iconRect     = wxRect(iconPt, szIcon);
            dc.DrawBitmap(m_icon.bmp(), iconPt);
        }
    } else {
        if (m_bmpDiableIcon.bmp().IsOk()) {
            wxSize  szIcon = m_bmpDiableIcon.GetBmpSize();
            wxPoint iconPt = {size.x - szIcon.x - 5, (size.y - szIcon.y) / 2};
            m_iconRect     = wxRect(iconPt, szIcon);
            dc.DrawBitmap(m_bmpDiableIcon.bmp(), iconPt);
        }
    }
}