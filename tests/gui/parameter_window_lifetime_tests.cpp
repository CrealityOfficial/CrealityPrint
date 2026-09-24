#include "../../src/slic3r/GUI/DeferredWindowDestroy.hpp"
#include <wx/wx.h>
#include <cstdlib>
#include <iostream>

class LifetimeTestApp : public wxApp {
public:
    bool OnInit() override { return true; }
};
wxIMPLEMENT_APP_NO_MAIN(LifetimeTestApp);

static void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

int main(int argc, char** argv)
{
    if (!wxEntryStart(argc, argv))
        return 1;
    wxTheApp->CallOnInit();
    wxTheApp->SetExitOnFrameDelete(false);
    using Slic3r::GUI::detach_and_defer_destroy;

    // Match the parameter reset path: clear the old layout from inside its
    // receiver's mouse callback, then let wxWidgets continue event dispatch.
    for (int iteration = 0; iteration < 50; ++iteration) {
        auto* frame = new wxFrame(nullptr, wxID_ANY, "hidden lifetime regression");
        auto* layout = new wxBoxSizer(wxVERTICAL);
        frame->SetSizer(layout);
        auto* panel = new wxPanel(frame);
        layout->Add(panel);
        wxWeakRef<wxWindow> retired(panel);
        bool callback_completed = false;
        panel->Bind(wxEVT_LEFT_DOWN, [&](wxMouseEvent& event) {
            detach_and_defer_destroy(panel);
            layout->Clear(true);
            require(retired.get() != nullptr, "event receiver destroyed inside its callback");
            layout->Add(new wxPanel(frame));
            callback_completed = true;
            event.Skip();
        });
        wxMouseEvent event(wxEVT_LEFT_DOWN);
        event.SetEventObject(panel);
        panel->GetEventHandler()->ProcessEvent(event);
        require(callback_completed && retired.get(), "receiver did not survive mouse dispatch");
        require(panel->GetContainingSizer() == nullptr, "retired panel still owned by old layout");
        wxTheApp->ProcessPendingEvents();
        require(!retired.get(), "retired receiver leaked after deferred destruction");
        delete frame;
    }

    // A page/window can close before the pending cleanup is delivered.
    auto* frame = new wxFrame(nullptr, wxID_ANY, "hidden close regression");
    auto* panel = new wxPanel(frame);
    wxWeakRef<wxWindow> retired(panel);
    detach_and_defer_destroy(panel);
    detach_and_defer_destroy(panel);
    delete frame;
    require(!retired.get(), "parent did not destroy its child");
    wxTheApp->ProcessPendingEvents(); // Must not touch/delete the old address.

    wxTheApp->OnExit();
    wxEntryCleanup();
    std::cout << "parameter window lifetime tests passed\n";
    return 0;
}
