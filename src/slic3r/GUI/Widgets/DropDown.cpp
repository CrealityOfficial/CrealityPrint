#include "DropDown.hpp"
#include "Label.hpp"

#include <wx/display.h>
#include <wx/dcbuffer.h>
#include <wx/dcgraph.h>

#include <algorithm>

#ifdef __WXGTK__
#include <gtk/gtk.h>
#endif

wxDEFINE_EVENT(EVT_DISMISS, wxCommandEvent);

BEGIN_EVENT_TABLE(DropDown, PopupWindow)

EVT_LEFT_DOWN(DropDown::mouseDown)
EVT_LEFT_UP(DropDown::mouseReleased)
EVT_MOUSE_CAPTURE_LOST(DropDown::mouseCaptureLost)
EVT_MOTION(DropDown::mouseMove)
EVT_MOUSEWHEEL(DropDown::mouseWheelMoved)

// catch paint events
EVT_PAINT(DropDown::paintEvent)

END_EVENT_TABLE()

/*
 * Called by the system of by wxWidgets when the panel needs
 * to be redrawn. You can also trigger this call by
 * calling Refresh()/Update().
 */

DropDown::DropDown(std::vector<wxString> &texts,
                   std::vector<wxString> &tips,
                   std::vector<wxBitmap> &icons)
    : texts(texts)
    , tips(tips)
    , icons(icons)
    , state_handler(this)
    , border_color(0xDBDBDB)
    , text_color(0x000000)
    , selector_border_color(std::make_pair(0x15BF59, (int) StateColor::Hovered),
        std::make_pair(*wxWHITE, (int) StateColor::Normal))
    , selector_background_color(std::make_pair(0x15BF59, (int) StateColor::Checked), // ORCA updated background color for checked item
        std::make_pair(*wxWHITE, (int) StateColor::Normal))
{
}

DropDown::DropDown(wxWindow *             parent,
                   std::vector<wxString> &texts,
                   std::vector<wxString> &tips,
                   std::vector<wxBitmap> &icons,
                   long           style)
    : DropDown(texts, tips, icons)
{
    Create(parent, style);
}

void DropDown::Create(wxWindow* parent, long style, int flags)
{
    PopupWindow::Create(parent, flags);
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(*wxWHITE);
    state_handler.attach({&border_color, &text_color, &selector_border_color, &selector_background_color});
    state_handler.update_binds();
//     if ((style & DD_NO_CHECK_ICON) == 0)//
//         check_bitmap = ScalableBitmap(this, "checked", 16);
    text_off = style & DD_NO_TEXT;

    // BBS set default font
    SetFont(Label::Body_13);
#ifdef __WXOSX__
    // PopupWindow releases mouse on idle, which may cause various problems,
    //  such as losting mouse move, and dismissing soon on first LEFT_DOWN event.
    Bind(wxEVT_IDLE, [] (wxIdleEvent & evt) {});
#endif
}

void DropDown::Invalidate(bool clear)
{
    if (clear) {
        selection = hover_item = -1;
        offset = wxPoint();
    }
    assert(selection < (int) texts.size());
    need_sync = true;
}

void DropDown::SetSelection(int n)
{
    //assert(n < (int) texts.size());
    if (n >= (int) texts.size())
        n = -1;
    if (selection == n) return;
    selection = n;
    if (need_sync) { // for icon Size
        messureSize();
        need_sync = true;
    }
    paintNow();
}

wxString DropDown::GetValue() const
{
    return selection >= 0 ? texts[selection] : wxString();
}

void DropDown::SetValue(const wxString &value)
{
    auto i = std::find(texts.begin(), texts.end(), value);
    selection = i == texts.end() ? -1 : std::distance(texts.begin(), i);
}
void DropDown::setDrapDownGap(int drapDownGap) { 
    m_drapDownGap = drapDownGap; 
}

void DropDown::SetCornerRadius(double radius)
{
    this->radius = radius;
    paintNow();
}

void DropDown::SetBorderColor(StateColor const &color)
{
    border_color = color;
    state_handler.update_binds();
    paintNow();
}

void DropDown::SetSelectorBorderColor(StateColor const &color)
{
    selector_border_color = color;
    state_handler.update_binds();
    paintNow();
}

void DropDown::SetTextColor(StateColor const &color)
{
    text_color = color;
    state_handler.update_binds();
    paintNow();
}

void DropDown::SetSelectorBackgroundColor(StateColor const &color)
{
    selector_background_color = color;
    state_handler.update_binds();
    paintNow();
}

void DropDown::SetUseContentWidth(bool use, bool limit_max_content_width)
{
    if (use_content_width == use)
        return;
    use_content_width = use;
    this->limit_max_content_width = limit_max_content_width;
    need_sync = true;
    messureSize();
}

void DropDown::SetAlignIcon(bool align) { align_icon = align; }

void DropDown::SetMaxVisibleItems(size_t max_items)
{
    if (m_max_visible_items == max_items)
        return;
    m_max_visible_items = max_items;
    need_sync = true;
}

void DropDown::SetPopupDirection(PopupDirection direction)
{
    m_popup_direction = direction;
}

void DropDown::Rescale()
{
    need_sync = true;
}

bool DropDown::HasDismissLongTime()
{
    auto now = boost::posix_time::microsec_clock::universal_time();
    return !IsShown() &&
        (now - dismissTime).total_milliseconds() >= 20;
}

void DropDown::paintEvent(wxPaintEvent& evt)
{
    // depending on your system you may need to look at double-buffered dcs
    wxBufferedPaintDC dc(this);
    render(dc);
}

/*
 * Alternatively, you can use a clientDC to paint on the panel
 * at any time. Using this generally does not free you from
 * catching paint events, since it is possible that e.g. the window
 * manager throws away your drawing when the window comes to the
 * background, and expects you will redraw it when the window comes
 * back (by sending a paint event).
 */
void DropDown::paintNow()
{
    // depending on your system you may need to look at double-buffered dcs
    //wxClientDC dc(this);
    //render(dc);
    Refresh();
}

static wxSize GetBmpSize(wxBitmap & bmp)
{
#ifdef __APPLE__
    return bmp.GetScaledSize();
#else
    return bmp.GetSize();
#endif
}

/*
 * Here we do the actual rendering. I put it in a separate
 * method so that it can work no matter what type of DC
 * (e.g. wxPaintDC or wxClientDC) is used.
 */
void DropDown::render(wxDC &dc)
{
    if (texts.size() == 0) return;
    int states = state_handler.states();
    dc.SetPen(wxPen(border_color.colorForStates(states)));
    dc.SetBrush(wxBrush(StateColor::darkModeColorFor(GetBackgroundColour())));
    // if (GetWindowStyle() & wxBORDER_NONE)
    //    dc.SetPen(wxNullPen);

    // draw background
    wxSize size = GetSize();
    if (radius == 0)
        dc.DrawRectangle(0, 0, size.x, size.y);
    else
        dc.DrawRoundedRectangle(0, 0, size.x, size.y, radius);

    // draw hover rectangle
    wxRect rcContent = {{0, offset.y}, rowSize};
    if (hover_item >= 0 && (states & StateColor::Hovered)) {
        rcContent.y += rowSize.y * hover_item;
        if (rcContent.GetBottom() > 0 && rcContent.y < size.y) {
            if (selection == hover_item)
                dc.SetBrush(wxBrush(selector_background_color.colorForStates(states | StateColor::Checked)));
            dc.SetPen(wxPen(selector_border_color.colorForStates(states)));
            rcContent.Deflate(4, 1);
            dc.DrawRectangle(rcContent);
            rcContent.Inflate(4, 1);
        }
        rcContent.y = offset.y;
    }
    // draw checked rectangle
    if (selection >= 0 && (selection != hover_item || (states & StateColor::Hovered) == 0)) {
        rcContent.y += rowSize.y * selection;
        if (rcContent.GetBottom() > 0 && rcContent.y < size.y) {
            dc.SetBrush(wxBrush(selector_background_color.colorForStates(states | StateColor::Checked)));
            dc.SetPen(wxPen(selector_background_color.colorForStates(states)));
            rcContent.Deflate(4, 1);
            dc.DrawRectangle(rcContent);
            rcContent.Inflate(4, 1);
        }
        rcContent.y = offset.y;
    }
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    {
        wxSize offset = (rowSize - textSize) / 2;
        rcContent.Deflate(0, offset.y);
    }

    // draw position bar
    if (rowSize.y * texts.size() > size.y) {
        int    height = rowSize.y * texts.size();
        wxRect rect = {size.x - 6, -offset.y * size.y / height, 4,
                       size.y * size.y / height};
        dc.SetPen(wxPen(border_color.defaultColor()));
        dc.SetBrush(wxBrush(*wxLIGHT_GREY));
        dc.DrawRoundedRectangle(rect, 2);
        rcContent.width -= 6;
    }

    // draw check icon
    rcContent.x += 5;
    rcContent.width -= 5;
    if (check_bitmap.bmp().IsOk()) {
        auto szBmp = check_bitmap.GetBmpSize();
        if (selection >= 0) {
            wxPoint pt = rcContent.GetLeftTop();
            pt.y += (rcContent.height - szBmp.y) / 2;
            pt.y += rowSize.y * selection;
            if (pt.y + szBmp.y > 0 && pt.y < size.y)
                dc.DrawBitmap(check_bitmap.bmp(), pt);
        }
        rcContent.x += szBmp.x + 5;
        rcContent.width -= szBmp.x + 5;
    }
    // draw texts & icons
    dc.SetTextForeground(text_color.colorForStates(states));
    for (int i = 0; i < texts.size(); ++i) {
        if (rcContent.GetBottom() < 0) {
            rcContent.y += rowSize.y;
            continue;
        }
        if (rcContent.y > size.y) break;
        wxPoint pt   = rcContent.GetLeftTop();
        auto & icon = icons[i];
        auto size2 = GetBmpSize(icon);
        if (iconSize.x > 0) {
            if (icon.IsOk()) {
                pt.y += (rcContent.height - size2.y) / 2;
                dc.DrawBitmap(icon, pt);
            }
            pt.x += iconSize.x + 5;
            pt.y = rcContent.y;
        } else if (icon.IsOk()) {
            pt.y += (rcContent.height - size2.y) / 2;
            dc.DrawBitmap(icon, pt);
            pt.x += size2.x + 5;
            pt.y = rcContent.y;
        }
        auto text = texts[i];
        if (!text_off && !text.IsEmpty()) {
            wxSize tSize = dc.GetMultiLineTextExtent(text);
            if (pt.x + tSize.x > rcContent.GetRight()) {
                if (i == hover_item && tips[i].IsEmpty())
                    SetToolTip(text);
                text = wxControl::Ellipsize(text, dc, wxELLIPSIZE_END,
                                            rcContent.GetRight() - pt.x);
            }
            pt.y += (rcContent.height - textSize.y) / 2;
            dc.SetFont(GetFont());
            dc.DrawText(text, pt);
        }
        rcContent.y += rowSize.y;
    }
}

void DropDown::messureSize()
{
    if (!need_sync) return;
    textSize = wxSize();
    iconSize = wxSize();
    wxClientDC dc(GetParent() ? GetParent() : this);
    for (size_t i = 0; i < texts.size(); ++i) {
        wxSize size1 = text_off ? wxSize() : dc.GetMultiLineTextExtent(texts[i]);
        if (icons[i].IsOk()) {
            wxSize size2 = GetBmpSize(icons[i]);
            if (size2.x > iconSize.x) iconSize = size2;
            if (!align_icon) {
                size1.x += size2.x + (text_off ? 0 : 5);
            }
        }
        if (size1.x > textSize.x) textSize = size1;
    }
    if (!align_icon) iconSize.x = 0;
    wxSize szContent = textSize;
    szContent.x += 10;
    if (check_bitmap.bmp().IsOk()) {
        auto szBmp = check_bitmap.GetBmpSize();
        szContent.x += szBmp.x + 5;
    }
    if (iconSize.x > 0) szContent.x += iconSize.x + (text_off ? 0 : 5);
    if (iconSize.y > szContent.y) szContent.y = iconSize.y;
    szContent.y += 10;
    if (texts.size() > m_max_visible_items) szContent.x += 6;
    if (GetParent()) {
        auto x = GetParent()->GetSize().x;
        if (!use_content_width || x > szContent.x)
            szContent.x = x;
    }
    rowSize = szContent;
    if (limit_max_content_width) {
        wxSize parent_size = GetParent()->GetParent()->GetSize();
        if (rowSize.x > parent_size.x) {
            rowSize.x = parent_size.x;
            szContent = rowSize;
        }
    }
    szContent.y *= std::min(m_max_visible_items, texts.size());
    szContent.y += texts.size() > m_max_visible_items ? rowSize.y / 2 : 0;
    wxWindow::SetSize(szContent);
#ifdef __WXGTK__
    // Gtk has a wrapper window for popup widget
    gtk_window_resize (GTK_WINDOW (m_widget), szContent.x, szContent.y);
#endif
    need_sync = false;
}

void DropDown::autoPosition()
{
    need_sync = true;
    messureSize();

    if (m_popup_direction == PopupDirection::Down) {
        wxPoint pos = GetParent()->ClientToScreen(wxPoint(0, -6));
        wxPoint old = GetPosition();
        wxSize size = GetSize();
        Position(pos, {0, GetParent()->GetSize().y + 12 - 6 + m_drapDownGap});
        if (old != GetPosition()) {
            size = rowSize;
            size.y *= std::min(m_max_visible_items, texts.size());
            size.y += texts.size() > m_max_visible_items ? rowSize.y / 2 : 0;
            if (size != GetSize()) {
                wxWindow::SetSize(size);
                offset = wxPoint();
                Position(pos, {0, GetParent()->GetSize().y + 12 - 6 + m_drapDownGap});
            }
        }
        if (GetPosition().y > pos.y) {
            // may exceed
            auto drect = wxDisplay(GetParent()).GetGeometry();
            if (GetPosition().y + size.y + 10 > drect.GetBottom()) {
                if (use_content_width && texts.size() <= m_max_visible_items) size.x += 6;
                size.y = drect.GetBottom() - GetPosition().y - 10;
                wxWindow::SetSize(size);
                if (selection >= 0) {
                    if (offset.y + rowSize.y * (selection + 1) > size.y)
                        offset.y = size.y - rowSize.y * (selection + 1);
                    else if (offset.y + rowSize.y * selection < 0)
                        offset.y = -rowSize.y * selection;
                }
            }
        }
        return;
    }

    wxWindow *parent = GetParent();
    if (!parent)
        return;

    wxSize size = GetSize();
    const wxPoint parent_pos = parent->ClientToScreen(wxPoint(0, 0));
    const wxSize  parent_size = parent->GetSize();
    wxRect boundary = wxDisplay(parent).GetClientArea();

    if (wxWindow *top_window = wxGetTopLevelParent(parent)) {
        const wxRect top_rect(top_window->ClientToScreen(wxPoint(0, 0)), top_window->GetClientSize());
        const int left   = std::max(boundary.GetLeft(), top_rect.GetLeft());
        const int top    = std::max(boundary.GetTop(), top_rect.GetTop());
        const int right  = std::min(boundary.GetRight(), top_rect.GetRight());
        const int bottom = std::min(boundary.GetBottom(), top_rect.GetBottom());
        if (right >= left && bottom >= top)
            boundary = wxRect(wxPoint(left, top), wxSize(right - left + 1, bottom - top + 1));
    }

    const int margin = 10;
    const int below_y = parent_pos.y + parent_size.y + m_drapDownGap;
    const int available_below = boundary.GetBottom() - below_y - margin + 1;
    const int available_above = parent_pos.y - boundary.GetTop() - m_drapDownGap - margin;
    const bool open_up = m_popup_direction == PopupDirection::Up ||
        (m_popup_direction == PopupDirection::Auto && available_below < size.y && available_above > available_below);
    const int available_height = open_up ? available_above : available_below;
    const bool height_limited = available_height > 0 && available_height < size.y;

    if (height_limited) {
        if (use_content_width && texts.size() <= m_max_visible_items)
            size.x += 6;
        size.y = available_height;
    }

    if (size != GetSize()) {
        wxWindow::SetSize(size);
#ifdef __WXGTK__
        gtk_window_resize(GTK_WINDOW(m_widget), size.x, size.y);
#endif
    }

    int x = parent_pos.x;
    if (x + size.x + margin > boundary.GetRight())
        x = boundary.GetRight() - size.x - margin + 1;
    if (x < boundary.GetLeft() + margin)
        x = boundary.GetLeft() + margin;

    int y = open_up ? parent_pos.y - m_drapDownGap - size.y : below_y;
    if (y < boundary.GetTop() + margin)
        y = boundary.GetTop() + margin;
    else if (y + size.y + margin > boundary.GetBottom())
        y = boundary.GetBottom() - size.y - margin + 1;

    SetPosition(wxPoint(x, y));

    if (selection >= 0 && rowSize.y > 0 && rowSize.y * int(texts.size()) > size.y) {
        if (offset.y + rowSize.y * (selection + 1) > size.y)
            offset.y = size.y - rowSize.y * (selection + 1);
        else if (offset.y + rowSize.y * selection < 0)
            offset.y = -rowSize.y * selection;
    }
}

void DropDown::mouseDown(wxMouseEvent& event)
{
    // Receivce unexcepted LEFT_DOWN on Mac after OnDismiss
    if (!IsShown())
        return;
    // force calc hover item again
    mouseMove(event);
    pressedDown = true;
    CaptureMouse();
    dragStart   = event.GetPosition();
}

void DropDown::mouseReleased(wxMouseEvent& event)
{
    if (pressedDown) {
        dragStart = wxPoint();
        pressedDown = false;
        if (HasCapture())
            ReleaseMouse();
        if (hover_item >= 0) { // not moved
            sendDropDownEvent();
            DismissAndNotify();
        }
    }
}

void DropDown::mouseCaptureLost(wxMouseCaptureLostEvent &event)
{
    wxMouseEvent evt;
    mouseReleased(evt);
}

void DropDown::mouseMove(wxMouseEvent &event)
{
    wxPoint pt  = event.GetPosition();
    if (pressedDown) {
        wxPoint pt2 = offset + pt - dragStart;
        wxSize  size = GetSize();
        dragStart    = pt;
        if (pt2.y > 0)
            pt2.y = 0;
        else if (pt2.y + rowSize.y * int(texts.size()) < size.y)
            pt2.y = size.y - rowSize.y * int(texts.size());
        if (pt2.y != offset.y) {
            offset = pt2;
            hover_item = -1; // moved
        } else {
            return;
        }
    }
    if (!pressedDown || hover_item >= 0) {
        int hover = (pt.y - offset.y) / rowSize.y;
        if (hover >= (int) texts.size()) hover = -1;
        if (hover == hover_item) return;
        hover_item = hover;
        if (hover >= 0) SetToolTip(tips[hover]);
    }
    paintNow();
}

void DropDown::mouseWheelMoved(wxMouseEvent &event)
{
    auto delta = event.GetWheelRotation();
    wxSize  size  = GetSize();
    wxPoint pt2   = offset + wxPoint{0, delta};
    if (pt2.y > 0)
        pt2.y = 0;
    else if (pt2.y + rowSize.y * int(texts.size()) < size.y)
        pt2.y = size.y - rowSize.y * int(texts.size());
    if (pt2.y != offset.y) {
        offset = pt2;
    } else {
        return;
    }
    int hover = (event.GetPosition().y - offset.y) / rowSize.y;
    if (hover >= (int) texts.size()) hover = -1;
    if (hover != hover_item) {
        hover_item = hover;
        if (hover >= 0) SetToolTip(tips[hover]);
    }
    paintNow();
}

// currently unused events
void DropDown::sendDropDownEvent()
{
    wxCommandEvent event(wxEVT_COMBOBOX, GetId());
    event.SetEventObject(this);
    event.SetInt(hover_item);
    event.SetString(texts[hover_item]);
    GetEventHandler()->ProcessEvent(event);
}

void DropDown::OnDismiss()
{
    dismissTime = boost::posix_time::microsec_clock::universal_time();
    hover_item  = -1;
    wxCommandEvent e(EVT_DISMISS);
    GetEventHandler()->ProcessEvent(e);
}
