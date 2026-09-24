#pragma once

#include <wx/window.h>
#include <wx/utils.h>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <utility>

namespace Slic3r::GUI {

// A drag confined to one dialog. No native drag loop is entered: callbacks
// finish before queued layout changes can destroy the source or target.
class LocalDragHandler final
{
public:
    using Begin = std::function<bool()>;
    using Move = std::function<void(const wxPoint&)>;
    using End = std::function<void(const wxPoint&, bool)>;

    LocalDragHandler(wxWindow* window, Begin begin, Move move, End end)
        : m_window(window), m_begin(std::move(begin)), m_move(std::move(move)), m_end(std::move(end))
    {
        m_window->Bind(wxEVT_LEFT_DOWN, &LocalDragHandler::on_down, this);
        m_window->Bind(wxEVT_LEFT_UP, &LocalDragHandler::on_up, this);
        m_window->Bind(wxEVT_MOTION, &LocalDragHandler::on_motion, this);
        m_window->Bind(wxEVT_MOUSE_CAPTURE_LOST, &LocalDragHandler::on_capture_lost, this);
    }

    ~LocalDragHandler()
    {
        m_window->Unbind(wxEVT_LEFT_DOWN, &LocalDragHandler::on_down, this);
        m_window->Unbind(wxEVT_LEFT_UP, &LocalDragHandler::on_up, this);
        m_window->Unbind(wxEVT_MOTION, &LocalDragHandler::on_motion, this);
        m_window->Unbind(wxEVT_MOUSE_CAPTURE_LOST, &LocalDragHandler::on_capture_lost, this);
        Cancel();
    }

    LocalDragHandler(const LocalDragHandler&) = delete;
    LocalDragHandler& operator=(const LocalDragHandler&) = delete;

    void Cancel() { finish(false); }

private:
    void on_down(wxMouseEvent& event)
    {
        if (m_tracking || wxWindow::GetCapture() || !m_begin())
            return;
        m_tracking = true;
        m_dragged = false;
        m_origin = m_position = m_window->ClientToScreen(event.GetPosition());
        m_window->CaptureMouse();
        if (!m_window->HasCapture())
            finish(false);
    }

    void on_motion(wxMouseEvent& event)
    {
        if (!m_tracking) {
            event.Skip();
            return;
        }
        if (!event.LeftIsDown()) {
            finish(false);
            return;
        }
        m_position = m_window->ClientToScreen(event.GetPosition());
        const int threshold = std::max(1, m_window->FromDIP(4));
        if (!m_dragged && std::abs(m_position.x - m_origin.x) < threshold &&
            std::abs(m_position.y - m_origin.y) < threshold)
            return;
        m_dragged = true;
        m_move(m_position);
    }

    void on_up(wxMouseEvent& event)
    {
        if (!m_tracking) {
            event.Skip();
            return;
        }
        m_position = m_window->ClientToScreen(event.GetPosition());
        finish(m_dragged);
    }

    void on_capture_lost(wxMouseCaptureLostEvent&) { finish(false); }

    void finish(bool dropped)
    {
        if (!m_tracking)
            return;
        m_tracking = false;
        m_dragged = false;
        if (m_window->HasCapture())
            m_window->ReleaseMouse();
        m_end(m_position, dropped);
    }

    wxWindow* m_window;
    Begin m_begin;
    Move m_move;
    End m_end;
    bool m_tracking = false;
    bool m_dragged = false;
    wxPoint m_origin;
    wxPoint m_position;
};

} // namespace Slic3r::GUI
