#include "HoverBorderIcon.hpp"
#include "Label.hpp"
#include <algorithm>
#include <cmath>
#include <wx/gdicmn.h>
#include <wx/app.h> // Add this line to include the header file for wxGetApp
#include <wx/wx.h>
#include "slic3r/GUI/GUI_App.hpp"

BEGIN_EVENT_TABLE(HoverBorderIcon, wxPanel)
    EVT_PAINT(HoverBorderIcon::paintEvent)
    EVT_MOTION(HoverBorderIcon::OnMouseMove)
#ifdef  __APPLE__
    EVT_ENTER_WINDOW(HoverBorderIcon::OnMouseEnter)
    EVT_LEAVE_WINDOW(HoverBorderIcon::OnMouseLeave)
#endif //  __APPLE__
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
    SetCornerRadius(FromDIP(m_corner_radius_dip));
    SetLabel(text);

    StaticBox::SetFont(Label::Body_13);
    StaticBox::SetBorderColor(StateColor(std::make_pair(is_dark ? 0x4B4B4D : 0xFFFFFF, (int) StateColor::Disabled),
                                         std::make_pair(0x15BF59, (int) StateColor::Hovered),
                                         std::make_pair(is_dark ?  0x4B4B4D : 0xFFFFFF, (int) StateColor::Normal)));
    StaticBox::SetBackgroundColor(
        StateColor(std::make_pair(is_dark ? 0x4B4B4D : 0xF0F0F1, (int) StateColor::Disabled),
                   std::make_pair(is_dark ? 0x4B4B4D : 0xFFFFFF, (int) StateColor::Normal)));

    state_handler.update_binds();
    const int dip_w   = (size.GetWidth() > 0) ? ToDIP(size.GetWidth()) : -1;
    const int dip_h   = (size.GetHeight() > 0) ? ToDIP(size.GetHeight()) : -1;
    const int dip_min = (dip_w > 0 && dip_h > 0) ? std::min(dip_w, dip_h) : -1;
    if (dip_w > 0 && dip_h > 0)
        m_fixed_size_dip = wxSize(dip_w, dip_h);
    if (dip_min > 0) {
        m_icon_size_or_scale = std::max(1, (int) std::lround(dip_min * m_icon_rel_scale));
    } else {
        m_icon_size_or_scale = 20;
    }

    if (!icon.IsEmpty()) {
        this->m_icon = ScalableBitmap(this, icon.ToStdString(), m_icon_size_or_scale);
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
    this->m_icon = ScalableBitmap(this, icon.ToStdString(), m_icon_size_or_scale);
}

void HoverBorderIcon::SetIconScaleFactor(double factor)
{
    // Basic clamp to avoid degenerate values
    if (factor <= 0.0)
        factor = 0.1;
    m_icon_rel_scale = factor;

    int dip_w  = ToDIP(GetSize().GetWidth());
    int dip_h  = ToDIP(GetSize().GetHeight());
    int dip_min = std::min(dip_w, dip_h);
    if (dip_min > 0) {
        m_icon_size_or_scale = std::max(1, (int)std::lround(dip_min * m_icon_rel_scale));
    }

    if (!m_icon.name().empty()) {
        this->m_icon = ScalableBitmap(this, m_icon.name(), m_icon_size_or_scale);
        Refresh();
    }
}

void HoverBorderIcon::on_sys_color_changed(bool is_dark_mode)
{
    StaticBox::SetBorderColor(StateColor(std::make_pair(is_dark_mode ? 0x4B4B4D : 0xFFFFFF, (int) StateColor::Disabled),
                                         std::make_pair(0x15BF59, (int) StateColor::Hovered),
                                         std::make_pair(is_dark_mode ? 0x4B4B4D : 0xFFFFFF, (int) StateColor::Normal)));

    StaticBox::SetBackgroundColor(
        StateColor(std::make_pair(is_dark_mode ? 0x4B4B4D : 0xF0F0F1, (int) StateColor::Disabled),
                   std::make_pair(is_dark_mode ? 0x4B4B4D : 0xFFFFFF, (int) StateColor::Normal)));
    state_handler.update_binds();

    Refresh();

    m_force_paint = true;
}

void HoverBorderIcon::msw_rescale()
{
    Slic3r::GUI::wxGetApp().UpdateDarkUI(this);

    // If created with an explicit size, keep that size in DIP and rescale it with the window DPI.
    if (m_fixed_size_dip.GetWidth() > 0 && m_fixed_size_dip.GetHeight() > 0) {
        const wxSize px = FromDIP(m_fixed_size_dip);
        SetMinSize(px);
        SetMaxSize(px);
        SetSize(px);
    }

    SetCornerRadius(FromDIP(m_corner_radius_dip));

    // Recalculate icon DIP size from the current (logical) size and recreate bitmaps.
    wxSize dip_size = m_fixed_size_dip;
    if (dip_size.GetWidth() <= 0 || dip_size.GetHeight() <= 0) {
        const wxSize px = GetSize();
        dip_size = wxSize(ToDIP(px.GetWidth()), ToDIP(px.GetHeight()));
    }
    const int dip_min = (dip_size.GetWidth() > 0 && dip_size.GetHeight() > 0) ? std::min(dip_size.GetWidth(), dip_size.GetHeight()) : -1;
    if (dip_min > 0)
        m_icon_size_or_scale = std::max(1, (int)std::lround(dip_min * m_icon_rel_scale));

    if (!m_icon.name().empty())
        m_icon = ScalableBitmap(this, m_icon.name(), (int)m_icon_size_or_scale);

    if (!m_bmpDiableIcon.name().empty()) {
        const int px_cnt = (m_disable_icon_px_cnt_dip > 0) ? m_disable_icon_px_cnt_dip : m_bmpDiableIcon.px_cnt();
        m_bmpDiableIcon  = ScalableBitmap(this, m_bmpDiableIcon.name(), px_cnt);
    }

    InvalidateBestSize();
    Refresh();
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

void HoverBorderIcon::setDisableIcon(const wxString& disableIconName, int px_cnt/* = 18*/)
{
    m_disable_icon_px_cnt_dip = px_cnt;
    m_bmpDiableIcon = ScalableBitmap(this, disableIconName.ToStdString(), px_cnt);
}

void HoverBorderIcon::OnMouseMove(wxMouseEvent& event)
{
    SetCursor(wxCURSOR_HAND);
    Refresh();
    event.Skip();
}

#ifdef __APPLE__
void HoverBorderIcon::OnMouseEnter(wxMouseEvent& event)
{
    state_handler.set_state(StateHandler::Hovered, StateHandler::Hovered);
    Refresh();
    event.Skip();
}

void HoverBorderIcon::OnMouseLeave(wxMouseEvent& event)
{
    state_handler.set_state(0, StateHandler::Hovered);
    Refresh();
    event.Skip();
}
#endif //__APPLE__

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
            wxPoint iconPt = {(size.x - szIcon.x)/2, (size.y - szIcon.y) / 2};
            m_iconRect     = wxRect(iconPt, szIcon);
            dc.DrawBitmap(m_icon.bmp(), iconPt);
        }
    } else {
        if (m_bmpDiableIcon.bmp().IsOk()) {
            wxSize  szIcon = m_bmpDiableIcon.GetBmpSize();
            wxPoint iconPt = {(size.x - szIcon.x)/2, (size.y - szIcon.y) / 2};
            m_iconRect     = wxRect(iconPt, szIcon);
            dc.DrawBitmap(m_bmpDiableIcon.bmp(), iconPt);
        }
    }
}

ImgBtn::ImgBtn() : HoverBorderIcon()
{

}

ImgBtn::ImgBtn(wxWindow* parent,
    const wxString& text,
    const wxString& icon,
    const wxPoint& pos,
    const wxSize& size,
    long            style)
    /*: HoverBorderIcon(parent, text, icon, pos, size, style)*/
{
    Create(parent, text, icon, pos, size, style);
}
ImgBtn ::~ImgBtn()
{

}

void ImgBtn::SetSelected(bool selected)
{
    state_handler.set_state(selected ? StateHandler::Checked : 0, StateHandler::Checked);
    Refresh();
}

bool ImgBtn::IsSelected()
{
    return (GetState() & StateHandler::Checked) != 0;
}

void ImgBtn::render(wxDC& dc)
{
    wxSize size = GetSize();

    if (m_bSetEnable) {
        StaticBox::render(dc);
        if (m_icon.bmp().IsOk()) {
            wxSize  szIcon = m_icon.GetBmpSize();
            wxPoint iconPt = {(size.x - szIcon.x)/2, (size.y - szIcon.y) / 2};
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

void ImgBtn::Create(wxWindow* parent, const wxString& text, const wxString& icon, const wxPoint& pos, const wxSize& size, long style)
{
    bool is_dark = Slic3r::GUI::wxGetApp().dark_mode();

    StaticBox::Create(parent, wxID_ANY, pos, size, style);
    SetCornerRadius(6);
    SetLabel(text);

    StaticBox::SetFont(Label::Body_13);
    StaticBox::SetBorderColor(StateColor(std::make_pair(parent->GetBackgroundColour(), (int) StateColor::Disabled),
                                         std::make_pair(0x15BF59, (int) StateColor::Checked),
                                         std::make_pair(0x15BF59, (int) StateColor::Hovered),
                                         std::make_pair(parent->GetBackgroundColour(), (int) StateColor::Normal)));
    StaticBox::SetBackgroundColor(
        StateColor(std::make_pair(0xF0F0F1, (int) StateColor::Disabled),
                   std::make_pair(parent->GetBackgroundColour(), (int) StateColor::Focused), // ORCA updated background color for focused item
                   std::make_pair(*wxWHITE, (int) StateColor::Normal)));

    state_handler.update_binds();
    if (!icon.IsEmpty()) {
        int emUnit = Slic3r::GUI::wxGetApp().em_unit();
        m_icon_size_or_scale = FromDIP(91) * (10.0f / emUnit);
        this->m_icon = ScalableBitmap(this, icon.ToStdString(), m_icon_size_or_scale);
    }
}
