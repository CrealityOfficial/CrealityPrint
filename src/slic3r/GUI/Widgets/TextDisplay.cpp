#include "TextDisplay.hpp"
#include "Label.hpp"
#include <wx/gdicmn.h>

BEGIN_EVENT_TABLE(TextDisplay, wxPanel)
    EVT_PAINT(TextDisplay::paintEvent)
    EVT_MOTION(TextDisplay::OnMouseMove)
END_EVENT_TABLE()

TextDisplay::TextDisplay()
    : m_text_left(true)
{
}

TextDisplay::TextDisplay(wxWindow* parent, const wxString& text, const wxString& icon, const wxPoint& pos, const wxSize& size, long style)
    : m_text_left(true)
{
    Create(parent, text, icon, pos, size, style);
}

void TextDisplay::Create(wxWindow* parent, const wxString& text, const wxString& icon, const wxPoint& pos, const wxSize& size, long style)
{
    StaticBox::Create(parent, wxID_ANY, pos, size, style);
    SetCornerRadius(4);
    SetLabel(text);

    StaticBox::SetFont(Label::Body_13);
    StaticBox::SetBorderColor(StateColor(std::make_pair(0xDBDBDB, (int) StateColor::Disabled),
                                         std::make_pair(0x15BF59, (int) StateColor::Hovered),
                                         std::make_pair(0xDBDBDB, (int) StateColor::Normal)));
    StaticBox::SetBackgroundColor(
        StateColor(std::make_pair(0xF0F0F1, (int) StateColor::Disabled),
                   std::make_pair(wxColour("#787565"), (int) StateColor::Focused), // ORCA updated background color for focused item
                   std::make_pair(*wxWHITE, (int) StateColor::Normal)));
    SetLabelColor(StateColor(
        std::make_pair(wxColour("#ACACAC"), (int) StateColor::Disabled), // ORCA: Use same color for disabled text on combo boxes
        std::make_pair(0x000000, (int) StateColor::Normal)));

    state_handler.update_binds();
    if (!icon.IsEmpty()) {
        this->m_icon = ScalableBitmap(this, icon.ToStdString(), 16);
    }
}

void TextDisplay::SetLabel(const wxString& label)
{
    wxWindow::SetLabel(label);
    messureSize();
    Refresh();
}

void TextDisplay::SetTextLeft(bool text_left)
{
    m_text_left = text_left;
    Refresh();
}

void TextDisplay::SetIcon(const wxString& icon)
{
    if (this->m_icon.name() == icon.ToStdString())
        return;
    this->m_icon = ScalableBitmap(this, icon.ToStdString(), 16);
    // Rescale();
}

void TextDisplay::SetLabelColor(StateColor const &color)
{
    label_color = color;
    state_handler.update_binds();
}

void TextDisplay::messureSize()
{
    wxSize size = GetSize();
    wxClientDC dc(this);
    bool   align_right = GetWindowStyle() & wxRIGHT;
    if (align_right)
        dc.SetFont(GetFont());
    else
        dc.SetFont(Label::Body_12);
    labelSize = dc.GetTextExtent(wxWindow::GetLabel());
    wxSize textSize = wxSize(0, 0);
    int h = textSize.y + 8;
    if (size.y < h) {
        size.y = h;
    }
    wxSize minSize = size;
    minSize.x = GetMinWidth();
    SetMinSize(minSize);
    SetSize(size);
}

void TextDisplay::OnMouseMove(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();
    if (m_iconRect.Contains(pos)) {
        SetCursor(wxCURSOR_HAND);
    } else {
        SetCursor(wxCURSOR_ARROW);
    }
    Refresh();
    event.Skip();
}

void TextDisplay::paintEvent(wxPaintEvent& evt)
{
    wxPaintDC dc(this);
    render(dc);
}

void TextDisplay::render(wxDC& dc)
{
    StaticBox::render(dc);
    int states = state_handler.states();
    wxSize size = GetSize();
    bool align_right = GetWindowStyle() & wxRIGHT;
    // start draw
    wxPoint pt = {5, 0};

    if (m_text_left) {
        // Draw text first
        auto text = wxWindow::GetLabel();
        if (!text.IsEmpty()) {
            wxSize textSize = wxSize(0, 0);
            if (align_right) {
                if (pt.x + labelSize.x > size.x)
                    text = wxControl::Ellipsize(text, dc, wxELLIPSIZE_END, size.x - pt.x);
                pt.y = (size.y - labelSize.y) / 2;
            } else {
                pt.x += textSize.x;
                pt.y = (size.y + labelSize.y) / 2 - labelSize.y;
            }
            dc.SetTextForeground(label_color.colorForStates(states));
            if (align_right)
                dc.SetFont(GetFont());
            else
                dc.SetFont(Label::Body_12);
            dc.DrawText(text, pt);
            pt.x += labelSize.x + 5; // Adjust the x position to be after the text
        }
        // Draw the icon after the text
        if (m_icon.bmp().IsOk()) {
            wxSize szIcon = m_icon.GetBmpSize();
            // pt.y = (size.y - szIcon.y) / 2;
            wxPoint iconPt = {size.x - szIcon.x - 5, (size.y - szIcon.y) / 2}; // Adjust the x position to be at the rightmost
            m_iconRect = wxRect(iconPt, szIcon); 
            dc.DrawBitmap(m_icon.bmp(), iconPt);
        }
    } else {
        // Draw the icon first
        if (m_icon.bmp().IsOk()) {
            wxSize szIcon = m_icon.GetBmpSize();
            pt.y = (size.y - szIcon.y) / 2;
            m_iconRect = wxRect(pt, szIcon); 
            dc.DrawBitmap(m_icon.bmp(), pt);
            pt.x += szIcon.x + 5; // Adjust the x position to be after the icon
        }
        // Draw text after the icon
        auto text = wxWindow::GetLabel();
        if (!text.IsEmpty()) {
            wxSize textSize = wxSize(0, 0);
            if (align_right) {
                if (pt.x + labelSize.x > size.x)
                    text = wxControl::Ellipsize(text, dc, wxELLIPSIZE_END, size.x - pt.x);
                pt.y = (size.y - labelSize.y) / 2;
            } else {
                pt.x += textSize.x;
                pt.y = (size.y + labelSize.y) / 2 - labelSize.y;
            }
            dc.SetTextForeground(label_color.colorForStates(states));
            if (align_right)
                dc.SetFont(GetFont());
            else
                dc.SetFont(Label::Body_12);
            dc.DrawText(text, pt);
        }
    }
}