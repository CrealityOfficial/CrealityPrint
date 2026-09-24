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

namespace {
// Scrollbar geometry, in DIP.
constexpr int SCROLLBAR_WIDTH_DIP     = 6;
constexpr int SCROLLBAR_MARGIN_DIP    = 2;
constexpr int SCROLLBAR_MIN_THUMB_DIP = 24;
// Extra hit area on each side of the bar so it is easy to grab.
constexpr int SCROLLBAR_GRAB_PAD_DIP  = 4;
} // namespace

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

DropDown::DropDown(std::vector<wxString> &texts,
                   std::vector<wxString> &tips,
                   std::vector<wxBitmap> &icons,
                   std::vector<bool> *group_headers)
    : DropDown(texts, tips, icons)
{
    this->group_headers = group_headers;
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

bool DropDown::IsGroupHeader(int index) const
{
    if (index < 0 || !group_headers)
        return false;
    return static_cast<size_t>(index) < group_headers->size() && (*group_headers)[index];
}

void DropDown::SetIndents(const std::vector<int> *levels, int step_dip)
{
    indents         = levels;
    indent_step_dip = std::max(0, step_dip);
    indent_step     = levels ? dip(indent_step_dip) : 0;
    need_sync       = true;
}

void DropDown::SetAnchor(const wxRect &rect, int width, wxWindow* dpi_reference)
{
    m_anchor_rect   = rect;
    m_anchor_width  = width;
    m_dpi_reference = rect.IsEmpty() ? nullptr : dpi_reference;
    // Recomputed by autoPosition() for the new anchor.
    m_anchor_max_height = 0;
    m_anchor_target     = wxRect();
    need_sync           = true;
}

void DropDown::SetBolds(const std::vector<char> *flags)
{
    bolds     = flags;
    need_sync = true;
}

int DropDown::indentFor(size_t i) const
{
    if (indents == nullptr || indent_step == 0 || i >= indents->size())
        return 0;
    return (*indents)[i] * indent_step;
}

bool DropDown::isBold(size_t i) const
{
    return bolds != nullptr && i < bolds->size() && (*bolds)[i] != 0;
}

int DropDown::dip(int value) const
{
    return wxWindow::FromDIP(value, m_dpi_reference ? m_dpi_reference : this);
}

int DropDown::contentHeight() const
{
    return GetContentHeight();
}

bool DropDown::scrollbarRects(wxRect &track, wxRect &thumb) const
{
    const wxSize size   = GetSize();
    const int    height = contentHeight();
    if (height <= size.y || height <= 0 || size.y <= 0)
        return false;

    const int margin = dip(SCROLLBAR_MARGIN_DIP);
    // Clamp the width so the bar always lands inside the window: the strip is
    // only reserved in messureSize() when the item count exceeds the visible
    // limit, but a scrollbar also appears when autoPosition() clips the height
    // near a screen edge, and with an explicit anchor width nothing is added.
    const int bar_w = std::min(dip(SCROLLBAR_WIDTH_DIP), std::max(1, size.x - 2 * margin));
    track = wxRect(size.x - bar_w - margin, margin, bar_w, size.y - 2 * margin);
    if (track.height <= 0 || track.width <= 0 || track.x < 0)
        return false;

    int thumb_h = std::max(dip(SCROLLBAR_MIN_THUMB_DIP), track.height * size.y / height);
    thumb_h     = std::min(thumb_h, track.height);

    // offset.y is <= 0; map the scrolled range onto the free travel of the thumb.
    const int max_scroll = height - size.y;
    const int travel     = track.height - thumb_h;
    const int thumb_y    = max_scroll > 0 ? track.y + (-offset.y) * travel / max_scroll : track.y;

    thumb = wxRect(track.x, thumb_y, track.width, thumb_h);
    return true;
}

bool DropDown::setScrollOffset(int y)
{
    const wxSize size   = GetSize();
    const int    height = contentHeight();
    if (y > 0)
        y = 0;
    else if (y + height < size.y)
        y = size.y - height;
    if (height <= size.y)
        y = 0;
    if (y == offset.y)
        return false;
    offset.y = y;
    return true;
}

void DropDown::scrollToThumbTop(int thumb_top)
{
    wxRect track, thumb;
    if (!scrollbarRects(track, thumb))
        return;

    const int travel = track.height - thumb.height;
    if (travel <= 0)
        return;

    const int max_scroll = contentHeight() - GetSize().y;
    int rel = thumb_top - track.y;
    rel     = std::max(0, std::min(rel, travel));
    if (setScrollOffset(-(rel * max_scroll / travel)))
        paintNow();
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
    indent_step = indents ? dip(indent_step_dip) : 0;
    need_sync = true;
}

bool DropDown::HasDismissLongTime()
{
    auto now = boost::posix_time::microsec_clock::universal_time();
    return !IsShown() &&
        (now - dismissTime).total_milliseconds() >= 20;
}

void DropDown::Popup(wxWindow *focus)
{
#ifdef __WXGTK__
    if (m_widget) {
        GtkWindow *transient_parent = nullptr;
        // Filament lists are owned by a combobox in a never-shown popup.
        // Use the visible anchor's toplevel, not that hidden GTK window.
        wxWindow *parent = m_dpi_reference ? m_dpi_reference : GetParent();
        for (wxWindow *win = parent; win; win = win->GetParent()) {
            GtkWidget *widget = static_cast<GtkWidget *>(win->GetHandle());
            if (!widget)
                continue;
            GtkWidget *top = gtk_widget_get_toplevel(widget);
            if (GTK_IS_WINDOW(top) && gtk_widget_get_mapped(top)) {
                transient_parent = GTK_WINDOW(top);
                break;
            }
        }
        if (transient_parent)
            gtk_window_set_transient_for(GTK_WINDOW(m_widget), transient_parent);
        if (!m_anchor_target.IsEmpty())
            gtk_window_move(GTK_WINDOW(m_widget), m_anchor_target.x, m_anchor_target.y);
    }
#endif
    PopupWindow::Popup(focus);

    // A hidden popup keeps the DPI metrics from the display where it was last
    // shown. Rebuild its font and geometry after it becomes visible on the target
    // display, otherwise the first open after a monitor switch uses stale sizes.
    SetFont(Label::Body_13);
    Rescale();
    autoPosition();

    // With an explicit anchor, re-apply the final geometry now that the window is
    // visible. SetSize()/SetPosition() do not reliably stick while it is hidden.
    if (!m_anchor_target.IsEmpty()) {
        SetSize(m_anchor_target);
#ifdef __WXGTK__
        if (m_widget && m_anchor_target.width > 0 && m_anchor_target.height > 0) {
            // wxGTK skips the native move when its cached coordinates match,
            // even if GTK placed the popup elsewhere while mapping it.
            gtk_window_move(GTK_WINDOW(m_widget), m_anchor_target.x, m_anchor_target.y);
            gtk_window_resize(GTK_WINDOW(m_widget), m_anchor_target.width, m_anchor_target.height);
        }
#endif
    }
    Layout();
    Refresh();
    Update();
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

int DropDown::GetItemY(int index) const
{
    if (index <= 0) return 0;
    int y = 0;
    int count = std::min(index, static_cast<int>(texts.size()));
    for (int i = 0; i < count; ++i) {
        y += rowSize.y;
        if (IsGroupHeader(i))
            y += dip(2);
    }
    return y;
}

int DropDown::GetContentHeight() const
{
    return GetItemY(static_cast<int>(texts.size()));
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

    // Reserve the scrollbar strip up front, so the hover / checked rectangles
    // drawn below stop short of it instead of being covered by it.
    wxRect scroll_track, scroll_thumb;
    const bool has_scrollbar = scrollbarRects(scroll_track, scroll_thumb);
    const int  scroll_reserved = has_scrollbar ? scroll_track.width + 2 * dip(SCROLLBAR_MARGIN_DIP) : 0;

    // draw hover rectangle
    wxRect rcContent = {{0, offset.y}, rowSize};
    rcContent.width -= scroll_reserved;
    if (hover_item >= 0 && (states & StateColor::Hovered) && !IsGroupHeader(hover_item)) {
        rcContent.y += GetItemY(hover_item);
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
    if (selection >= 0 && (selection != hover_item || (states & StateColor::Hovered) == 0) && !IsGroupHeader(selection)) {
        rcContent.y += GetItemY(selection);
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

    // draw scrollbar
    if (has_scrollbar) {
        const double r = scroll_thumb.width / 2.0;
        const wxColour track_colour(0xF0, 0xF0, 0xF0);
        const wxColour thumb_colour = dragging_scrollbar ? wxColour(0x60, 0x60, 0x60) : wxColour(0x9A, 0x9A, 0x9A);

        auto draw_bar = [&](wxDC &target) {
            target.SetPen(*wxTRANSPARENT_PEN);
            target.SetBrush(wxBrush(track_colour));
            target.DrawRoundedRectangle(scroll_track, r);
            target.SetBrush(wxBrush(thumb_colour));
            target.DrawRoundedRectangle(scroll_thumb, r);
        };

        // Prefer a wxGCDC: the plain (buffered) DC has no anti-aliasing, which
        // makes the rounded caps look chipped. wxGCDC has no generic wxDC
        // constructor, so go through the concrete type when possible.
        if (auto *mem_dc = dynamic_cast<wxMemoryDC *>(&dc)) {
            wxGCDC gdc(*mem_dc);
            draw_bar(gdc);
        } else if (auto *win_dc = dynamic_cast<wxWindowDC *>(&dc)) {
            wxGCDC gdc(*win_dc);
            draw_bar(gdc);
        } else {
            draw_bar(dc);
        }
    }

    // draw check icon
    rcContent.x += 5;
    rcContent.width -= 5;
    if (check_bitmap.bmp().IsOk()) {
        auto szBmp = check_bitmap.GetBmpSize();
        if (selection >= 0) {
            wxPoint pt = rcContent.GetLeftTop();
            pt.y += (rcContent.height - szBmp.y) / 2;
            pt.y += GetItemY(selection);
            if (pt.y + szBmp.y > 0 && pt.y < size.y)
                dc.DrawBitmap(check_bitmap.bmp(), pt);
        }
        rcContent.x += szBmp.x + 5;
        rcContent.width -= szBmp.x + 5;
    }
    // draw texts & icons
    dc.SetTextForeground(text_color.colorForStates(states));
    for (int i = 0; i < texts.size(); ++i) {
        int item_y = offset.y + GetItemY(i);
        if (item_y + rowSize.y < 0) continue;
        if (item_y > size.y) break;

        // Group header: draw as separator line with text
        if (IsGroupHeader(i) && !isBold(i)) {
            if (item_y + rowSize.y > 0 && item_y < size.y) {
                auto pre_clr = dc.GetTextForeground();
                auto pre_pen = dc.GetPen();
                auto pre_font = dc.GetFont();
                dc.SetTextForeground(StateColor::darkModeColorFor(wxColour(190, 190, 190)));
                dc.SetPen(wxPen(StateColor::darkModeColorFor(wxColour(166, 169, 170)), 1));
                auto font = GetFont();
                font.SetPointSize(font.GetPointSize() - 2);
                dc.SetFont(font);

                int spacing = dip(8);
                int text_y = item_y + (rowSize.y - GetFont().GetPixelSize().y) / 2;
                wxPoint pt(rcContent.x, text_y);

                if (!texts[i].IsEmpty()) {
                    wxSize tSize = dc.GetMultiLineTextExtent(texts[i]);
                    dc.DrawText(texts[i], pt);
                    int line_y = pt.y + tSize.y / 2;
                    int line_start = pt.x + tSize.x + spacing;
                    int line_end = size.x - spacing;
                    if (line_end > line_start)
                        dc.DrawLine(line_start, line_y, line_end, line_y);
                }

                dc.SetTextForeground(pre_clr);
                dc.SetPen(pre_pen);
                dc.SetFont(pre_font);
            }
            continue;
        }

        wxPoint pt   = wxPoint(rcContent.x, item_y);
        pt.x += indentFor(i);
        auto & icon = icons[i];
        auto size2 = GetBmpSize(icon);
        if (iconSize.x > 0) {
            if (icon.IsOk()) {
                pt.y += (rowSize.y - size2.y) / 2;
                dc.DrawBitmap(icon, pt);
            }
            pt.x += iconSize.x + 5;
        } else if (icon.IsOk()) {
            pt.y += (rowSize.y - size2.y) / 2;
            dc.DrawBitmap(icon, pt);
            pt.x += size2.x + 5;
        }
        auto text = texts[i];
        if (!text_off && !text.IsEmpty()) {
            // Set the font before measuring, so ellipsizing matches what is drawn.
            wxFont font = GetFont();
            if (isBold(i))
                font.MakeBold();
            dc.SetFont(font);

            wxSize tSize = dc.GetMultiLineTextExtent(text);
            if (pt.x + tSize.x > rcContent.GetRight()) {
                if (i == hover_item && tips[i].IsEmpty())
                    SetToolTip(text);
                text = wxControl::Ellipsize(text, dc, wxELLIPSIZE_END,
                                            rcContent.GetRight() - pt.x);
            }
            // Center text from the row origin, independently of the icon offset.
            pt.y = item_y + (rowSize.y - tSize.y) / 2;
            dc.DrawText(text, pt);
        }
    }
}

void DropDown::messureSize()
{
    if (!need_sync) return;
    textSize = wxSize();
    iconSize = wxSize();
    wxClientDC dc(m_dpi_reference ? m_dpi_reference : (GetParent() ? GetParent() : this));
    const wxFont base_font = GetFont();
    wxFont bold_font = base_font;
    bold_font.MakeBold();
    for (size_t i = 0; i < texts.size(); ++i) {
        // Bold rows are wider, so measure them with the font they are drawn with.
        dc.SetFont(isBold(i) ? bold_font : base_font);
        wxSize size1 = text_off ? wxSize() : dc.GetMultiLineTextExtent(texts[i]);
        if (icons[i].IsOk()) {
            wxSize size2 = GetBmpSize(icons[i]);
            if (size2.x > iconSize.x) iconSize = size2;
            if (!align_icon) {
                size1.x += size2.x + (text_off ? 0 : 5);
            }
        }
        size1.x += indentFor(i);
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
    // Reserve room for the scrollbar so it never overlaps the longest label. A
    // scrollbar can also appear when autoPosition() has to clip the height near a
    // screen edge, so reserve the strip whenever the list may scroll at all.
    if (texts.size() > m_max_visible_items)
        szContent.x += dip(SCROLLBAR_WIDTH_DIP) + 2 * dip(SCROLLBAR_MARGIN_DIP);
    if (m_anchor_width > 0) {
        // Explicit width wins over both the content width and the parent width.
        szContent.x = m_anchor_width;
        rowSize     = szContent;
    } else {
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
    }
    // Calculate total height accounting for group header extra spacing
    size_t visible_count = std::min(m_max_visible_items, texts.size());
    int total_row_height = 0;
    for (size_t i = 0; i < visible_count; ++i) {
        total_row_height += rowSize.y;
        if (IsGroupHeader(i))
            total_row_height += dip(2);
    }
    szContent.y = total_row_height;
    szContent.y += texts.size() > m_max_visible_items ? rowSize.y / 2 : 0;
    if (m_anchor_max_height > 0 && szContent.y > m_anchor_max_height)
        szContent.y = m_anchor_max_height;
    wxWindow::SetSize(szContent);
#ifdef __WXGTK__
    // Gtk has a wrapper window for popup widget. Avoid GTK assertions on invalid sizes.
    if (m_widget && szContent.x > 0 && szContent.y > 0)
        gtk_window_resize(GTK_WINDOW(m_widget), szContent.x, szContent.y);
#endif
    need_sync = false;
}

void DropDown::autoPosition()
{
    // Always decide from the unclipped height, so calling this repeatedly (which
    // the first open does, see ComboBox::ForceDropdownOpen) is idempotent and
    // cannot flip the open-up decision on the second pass.
    m_anchor_max_height = 0;
    need_sync = true;
    messureSize();

    if (!m_anchor_rect.IsEmpty()) {
        // Explicit anchor: position relative to the given screen rect. Used when
        // the owning combobox is not the visible trigger, so its own screen
        // position must not be consulted.
        wxSize size = GetSize();
        int display_idx = wxDisplay::GetFromPoint(m_anchor_rect.GetTopLeft());
        if (display_idx == wxNOT_FOUND)
            display_idx = 0;
        wxRect boundary = wxDisplay(static_cast<unsigned>(display_idx)).GetClientArea();

        // Always open downwards, never flip above the anchor: when there is not
        // enough room below, clip the height and let the user scroll instead.
        const int margin = 2;
        const int y = m_anchor_rect.GetBottom() + 1 + m_drapDownGap;
        const int available_height = boundary.GetBottom() - y - margin + 1;

        // Remember the cap so a later messureSize() does not restore the full
        // height; that would push the bottom of the list off-screen.
        m_anchor_max_height = std::max(0, available_height);
        if (available_height > 0 && available_height < size.y)
            size.y = available_height;
        if (size != GetSize()) {
            wxWindow::SetSize(size);
#ifdef __WXGTK__
            if (m_widget && size.x > 0 && size.y > 0)
                gtk_window_resize(GTK_WINDOW(m_widget), size.x, size.y);
#endif
        }

        int x = m_anchor_rect.GetLeft();
        if (x + size.x > boundary.GetRight())
            x = boundary.GetRight() - size.x + 1;
        x = std::max(x, boundary.GetLeft());

        // Remember the target rect: autoPosition() runs while the window is still
        // hidden, where SetSize()/SetPosition() are unreliable on a
        // wxPopupTransientWindow, so Popup() re-applies it once shown.
        m_anchor_target = wxRect(wxPoint(x, y), size);
        SetSize(m_anchor_target);

        if (selection >= 0 && rowSize.y > 0 && GetContentHeight() > size.y) {
            if (offset.y + GetItemY(selection + 1) > size.y)
                offset.y = size.y - GetItemY(selection + 1);
            else if (offset.y + GetItemY(selection) < 0)
                offset.y = -GetItemY(selection);
        }
        return;
    }

    if (m_popup_direction == PopupDirection::Down) {
        wxPoint pos = GetParent()->ClientToScreen(wxPoint(0, -6));
        wxPoint old = GetPosition();
        wxSize size = GetSize();
        Position(pos, {0, GetParent()->GetSize().y + 12 - 6 + m_drapDownGap});
        if (old != GetPosition()) {
            size = rowSize;
            size.y = 0;
            size_t count = std::min(m_max_visible_items, texts.size());
            for (size_t i = 0; i < count; ++i) {
                size.y += rowSize.y;
                if (IsGroupHeader(i))
                    size.y += dip(2);
            }
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
                int available_height = drect.GetBottom() - GetPosition().y - 10;
                if (available_height < rowSize.y * 2)
                    return;
                if (use_content_width && texts.size() <= m_max_visible_items)
                    size.x += dip(SCROLLBAR_WIDTH_DIP) + 2 * dip(SCROLLBAR_MARGIN_DIP);
                size.y = available_height;
                wxWindow::SetSize(size);
                if (selection >= 0) {
                    if (offset.y + GetItemY(selection + 1) > size.y)
                        offset.y = size.y - GetItemY(selection + 1);
                    else if (offset.y + GetItemY(selection) < 0)
                        offset.y = -GetItemY(selection);
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
            size.x += dip(SCROLLBAR_WIDTH_DIP) + 2 * dip(SCROLLBAR_MARGIN_DIP);
        size.y = available_height;
    }

    if (size != GetSize()) {
        wxWindow::SetSize(size);
#ifdef __WXGTK__
        if (m_widget && size.x > 0 && size.y > 0)
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

    if (selection >= 0 && rowSize.y > 0 && GetContentHeight() > size.y) {
        if (offset.y + GetItemY(selection + 1) > size.y)
            offset.y = size.y - GetItemY(selection + 1);
        else if (offset.y + GetItemY(selection) < 0)
            offset.y = -GetItemY(selection);
    }
}

void DropDown::mouseDown(wxMouseEvent& event)
{
    // Receivce unexcepted LEFT_DOWN on Mac after OnDismiss
    if (!IsShown())
        return;

    // Grabbing the scrollbar starts a drag instead of selecting an item.
    wxRect track, thumb;
    if (scrollbarRects(track, thumb)) {
        wxRect hit = track;
        hit.Inflate(dip(SCROLLBAR_GRAB_PAD_DIP), 0);
        const wxPoint pt = event.GetPosition();
        if (hit.Contains(pt)) {
            if (thumb.Contains(wxPoint(thumb.x, pt.y))) {
                // Grabbed the thumb: keep the point under the cursor fixed.
                scrollbar_grab_dy = pt.y - thumb.y;
            } else {
                // Clicked the track: jump so the thumb is centred on the cursor.
                scrollbar_grab_dy = thumb.height / 2;
                scrollToThumbTop(pt.y - scrollbar_grab_dy);
            }
            dragging_scrollbar = true;
            hover_item         = -1;
            if (!HasCapture())
                CaptureMouse();
            paintNow();
            return;
        }
    }

    // force calc hover item again
    mouseMove(event);
    pressedDown = true;
    CaptureMouse();
    dragStart   = event.GetPosition();
}

void DropDown::mouseReleased(wxMouseEvent& event)
{
    if (dragging_scrollbar) {
        dragging_scrollbar = false;
        scrollbar_grab_dy  = 0;
        if (HasCapture())
            ReleaseMouse();
        paintNow();
        return;
    }
    if (pressedDown) {
        dragStart = wxPoint();
        pressedDown = false;
        if (HasCapture())
            ReleaseMouse();
        if (hover_item >= 0 && !IsGroupHeader(hover_item)) { // not moved and not a group header
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
    if (dragging_scrollbar) {
        scrollToThumbTop(pt.y - scrollbar_grab_dy);
        return;
    }
    if (pressedDown) {
        // Drag the list content itself.
        const int target = offset.y + pt.y - dragStart.y;
        dragStart        = pt;
        if (setScrollOffset(target))
            hover_item = -1; // moved
        else
            return;
    }
    if (rowSize.y <= 0)
        return;
    if (!pressedDown || hover_item >= 0) {
        // Find which item contains the mouse position, accounting for group header extra spacing
        int hover = -1;
        int rel_y = pt.y - offset.y;
        for (int i = 0; i < (int) texts.size(); ++i) {
            int item_top = GetItemY(i);
            int item_h = rowSize.y + (IsGroupHeader(i) ? dip(2) : 0);
            if (rel_y >= item_top && rel_y < item_top + item_h) {
                hover = i;
                break;
            }
        }
        if (hover >= 0 && IsGroupHeader(hover)) hover = -1;
        if (hover == hover_item) return;
        hover_item = hover;
        if (hover >= 0) SetToolTip(tips[hover]);
    }
    paintNow();
}

void DropDown::mouseWheelMoved(wxMouseEvent &event)
{
    if (rowSize.y <= 0)
        return;
    if (!setScrollOffset(offset.y + event.GetWheelRotation()))
        return;
    // Find which item contains the mouse position, accounting for group header extra spacing
    int hover = -1;
    int rel_y = event.GetPosition().y - offset.y;
    for (int i = 0; i < (int) texts.size(); ++i) {
        int item_top = GetItemY(i);
        int item_h = rowSize.y + (IsGroupHeader(i) ? FromDIP(2) : 0);
        if (rel_y >= item_top && rel_y < item_top + item_h) {
            hover = i;
            break;
        }
    }
    if (hover >= 0 && IsGroupHeader(hover)) hover = -1;
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
