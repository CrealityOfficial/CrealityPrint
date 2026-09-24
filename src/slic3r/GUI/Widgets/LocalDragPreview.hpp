#pragma once

#include <wx/dcbuffer.h>
#include <wx/weakref.h>
#include <wx/window.h>
#include <algorithm>

namespace Slic3r::GUI {

// A child of the drag dialog, not a separately positioned native window.
// This also works when Wayland does not permit moving top-level windows.
// It never captures the mouse or accepts keyboard focus.
class LocalDragPreview final : public wxWindow
{
public:
    LocalDragPreview(wxWindow* host, const wxBitmap& bitmap, const wxSize& size)
        : wxWindow(host, wxID_ANY, wxDefaultPosition, size, wxBORDER_NONE), m_bitmap(bitmap)
    {
        Hide();
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT, [this](wxPaintEvent&) {
            wxAutoBufferedPaintDC dc(this);
            dc.DrawBitmap(m_bitmap, 0, 0, false);
        });
    }

    bool AcceptsFocus() const override { return false; }
    bool AcceptsFocusFromKeyboard() const override { return false; }

    void Follow(const wxPoint& screen_position)
    {
        wxWindow* host = GetParent();
        const wxPoint cursor = host->ScreenToClient(screen_position);
        const wxSize bounds = host->GetClientSize();
        if (!wxRect(wxPoint(0, 0), bounds).Contains(cursor)) {
            Hide();
            return;
        }

        const int gap = host->FromDIP(12);
        const wxSize size = GetSize();
        wxPoint position = cursor + wxPoint(gap, gap);
        // Near the right/bottom edge put the preview on the other side of
        // the pointer, so it remains visible without covering the drop point.
        if (position.x + size.x > bounds.x)
            position.x = cursor.x - gap - size.x;
        if (position.y + size.y > bounds.y)
            position.y = cursor.y - gap - size.y;
        position.x = std::clamp(position.x, 0, std::max(0, bounds.x - size.x));
        position.y = std::clamp(position.y, 0, std::max(0, bounds.y - size.y));
        Move(position);
        Raise();
        Show();
    }

private:
    wxBitmap m_bitmap;
};

} // namespace Slic3r::GUI
