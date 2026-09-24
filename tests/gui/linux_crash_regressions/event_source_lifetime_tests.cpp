#include <wx/event.h>
#include <wx/weakref.h>
#include <cstdio>
#include <memory>
#include <stdexcept>

wxDEFINE_EVENT(EVT_TEST_EXPORT_FINISHED, wxCommandEvent);

// Exercise wx's actual event connection and object tracking during both
// destruction orders, including a source outlived by a floating listener.
class ExportListener : public wxEvtHandler
{
public:
    explicit ExportListener(wxEvtHandler& source) : m_source(&source)
    {
        source.Bind(EVT_TEST_EXPORT_FINISHED, &ExportListener::OnExport, this);
    }
    ~ExportListener() override
    {
        if (m_source)
            m_source->Unbind(EVT_TEST_EXPORT_FINISHED, &ExportListener::OnExport, this);
    }
    bool HasSource() const { return m_source.get() != nullptr; }
    int deliveries = 0;

private:
    void OnExport(wxCommandEvent&) { ++deliveries; }
    wxWeakRef<wxEvtHandler> m_source;
};

static void require(bool value, const char* message)
{
    if (!value)
        throw std::runtime_error(message);
}

int main()
{
    try {
        for (int iteration = 0; iteration != 1000; ++iteration) {
            auto source = std::make_unique<wxEvtHandler>();
            auto listener = std::make_unique<ExportListener>(*source);
            wxCommandEvent event(EVT_TEST_EXPORT_FINISHED);
            require(source->ProcessEvent(event) && listener->deliveries == 1,
                    "live export events must still reach the listener");
            source.reset();
            require(!listener->HasSource(), "destroyed source must invalidate the weak reference");

            // Allocations for a replacement frame must not revive the old link.
            source = std::make_unique<wxEvtHandler>();
            require(!listener->HasSource(), "a replacement source must not revive a stale subscription");
            listener.reset();
            require(!source->ProcessEvent(event), "replacement source must not inherit an old listener");

            listener = std::make_unique<ExportListener>(*source);
            listener.reset();
            require(!source->ProcessEvent(event), "listener-first destruction must disconnect its callback");
        }
        std::puts("PASS: export delivery, source-first and listener-first destruction, source replacement (1000 cycles)");
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        return 1;
    }
}
