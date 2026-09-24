#pragma once

#include <wx/app.h>
#include <wx/sizer.h>
#include <wx/weakref.h>
#include <wx/window.h>

namespace Slic3r { namespace GUI {

// The caller must disconnect callbacks into the retired model first. Child
// windows are destroyed synchronously by wxSizer::Clear(true), even when their
// mouse event is still being dispatched. Remove that ownership before clearing
// the layout and finish destruction on the application event queue.
inline void detach_and_defer_destroy(wxWindow* window)
{
    if (wxSizer* owner = window->GetContainingSizer())
        owner->Detach(window);
    window->Hide();
    wxWeakRef<wxWindow> retired(window);
    wxTheApp->CallAfter([retired]() {
        // The parent may have destroyed the window while closing the page/app.
        if (retired && !retired->IsBeingDeleted())
            retired->Destroy();
    });
}

}}
