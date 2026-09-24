#include "../../../src/slic3r/GUI/Widgets/LocalDragHandler.hpp"
#include "../../../src/slic3r/GUI/Widgets/LocalDragPreview.hpp"
#include <wx/wx.h>
#include <cstdio>
#include <memory>
#include <stdexcept>

class TestApp : public wxApp { public: bool OnInit() override { return true; } };
wxIMPLEMENT_APP_NO_MAIN(TestApp);

static void require(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

class DragPanel : public wxPanel
{
public:
    explicit DragPanel(wxWindow* parent) : wxPanel(parent, wxID_ANY, {}, {100, 100}) {}
    std::unique_ptr<Slic3r::GUI::LocalDragHandler> handler;
};

static void mouse(wxWindow* window, wxEventType type, int x, bool down)
{
    wxMouseEvent event(type);
    event.SetEventObject(window);
    event.SetPosition({x, 10});
    event.m_leftDown = down;
    window->GetEventHandler()->ProcessEvent(event);
}

int main(int argc, char** argv)
{
    if (!wxEntryStart(argc, argv) || !wxTheApp->CallOnInit()) return 1;
    int result = 0;
    auto* frame = new wxFrame(nullptr, wxID_ANY, "Local drag regression", {}, {300, 200});
    auto* source = new DragPanel(frame);
    auto* second = new DragPanel(frame);
    second->Move(120, 0);
    int begins = 0, ends = 0, drops = 0, moves = 0;
    bool active = false;
    wxWeakRef<Slic3r::GUI::LocalDragPreview> preview;
    wxBitmap bitmap(90, 49);
    { wxMemoryDC dc(bitmap); dc.SetBackground(*wxBLUE_BRUSH); dc.Clear(); }
    const auto move = [&](const wxPoint& position) {
        ++moves;
        if (!preview)
            preview = new Slic3r::GUI::LocalDragPreview(frame, bitmap, bitmap.GetSize());
        preview->Follow(position);
    };
    const auto begin = [&] { if (active) return false; active = true; ++begins; return true; };
    const auto end = [&](const wxPoint&, bool dropped) {
        require(wxWindow::GetCapture() == nullptr, "capture must be released before the end callback");
        if (preview) { preview->Hide(); delete preview.get(); }
        active = false; ++ends; if (dropped) ++drops;
    };
    source->handler = std::make_unique<Slic3r::GUI::LocalDragHandler>(source, begin,
        move, end);
    second->handler = std::make_unique<Slic3r::GUI::LocalDragHandler>(second, begin,
        move, end);
    frame->Show();
    wxYield();
    try {
        mouse(source, wxEVT_LEFT_DOWN, 10, true);
        mouse(source, wxEVT_MOTION, 11, true);
        mouse(source, wxEVT_LEFT_UP, 11, false);
        require(begins == 1 && ends == 1 && drops == 0 && moves == 0, "a click must not drag");

        mouse(source, wxEVT_LEFT_DOWN, 10, true);
        mouse(second, wxEVT_LEFT_DOWN, 10, true);
        require(begins == 2 && source->HasCapture(), "a second press must not reenter a drag");
        wxWindow* focus = wxWindow::FindFocus();
        mouse(source, wxEVT_MOTION, 60, true);
        require(preview && preview->IsShown(), "drag must display the chip preview");
        require(!preview->AcceptsFocus() && wxWindow::FindFocus() == focus && source->HasCapture(),
                "preview must not steal focus or mouse capture");
        preview->Follow(frame->ClientToScreen(wxPoint(-5, -5)));
        require(!preview->IsShown() && source->HasCapture(), "leaving the dialog hides only the preview");
        const wxSize bounds = frame->GetClientSize();
        preview->Follow(frame->ClientToScreen(wxPoint(bounds.x - 1, bounds.y - 1)));
        require(preview->IsShown() && wxRect(wxPoint(0, 0), bounds).Contains(preview->GetRect()),
                "preview must stay inside the dialog at its bottom/right edge");
        mouse(source, wxEVT_LEFT_UP, 60, false);
        require(!preview, "drop must clear the preview");
        require(ends == 2 && drops == 1 && moves == 1, "a completed drag must drop exactly once");

        mouse(source, wxEVT_LEFT_DOWN, 10, true);
        mouse(source, wxEVT_MOTION, 60, true);
        source->handler->Cancel();
        mouse(source, wxEVT_LEFT_UP, 60, false);
        require(ends == 3 && drops == 1 && !active && !preview, "Esc/close cancellation must not drop");

        mouse(source, wxEVT_LEFT_DOWN, 10, true);
        mouse(source, wxEVT_MOTION, 60, true);
        source->ReleaseMouse();
        wxMouseCaptureLostEvent lost(source->GetId());
        source->GetEventHandler()->ProcessEvent(lost);
        require(ends == 4 && !active && !preview, "capture loss must cancel the drag");

        mouse(source, wxEVT_LEFT_DOWN, 10, true);
        mouse(source, wxEVT_MOTION, 60, false);
        require(ends == 5 && drops == 1 && !active, "a missed release must cancel on motion");

        mouse(source, wxEVT_LEFT_DOWN, 10, true);
        mouse(source, wxEVT_MOTION, 60, true);
        source->Destroy();
        require(ends == 6 && !active && !preview && !wxWindow::GetCapture(), "destroying a source must release capture");
        std::puts("PASS: click threshold, drag, reentry prevention, cancellation, capture loss, source destruction");
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        result = 1;
    }
    frame->Destroy();
    wxTheApp->ProcessPendingEvents();
    wxTheApp->OnExit();
    wxEntryCleanup();
    return result;
}
