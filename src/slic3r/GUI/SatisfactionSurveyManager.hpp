#ifndef slic3r_GUI_SatisfactionSurveyManager_hpp_
#define slic3r_GUI_SatisfactionSurveyManager_hpp_

#include <memory>
#include <string>
#include <vector>

class wxWindow;

namespace Slic3r {

class AppConfig;

namespace GUI {

// The entry point used by the different network-print workflows.  Keeping the
// context small makes those workflows adapters only; eligibility, de-duplication
// and persistence remain centralized in SatisfactionSurveyManager.
enum class SatisfactionSurveyPrintSource
{
    SendToPrinter,
    DeviceDetail,
    MultiDevice,
    AiAssistant
};

struct SatisfactionSurveyPrintEvent
{
    SatisfactionSurveyPrintSource source {SatisfactionSurveyPrintSource::SendToPrinter};
    std::vector<std::string>       device_addresses;
    std::string                    operation_fingerprint;
};

class SatisfactionSurveyManager final
{
public:
    SatisfactionSurveyManager(AppConfig& app_config,
                              std::string full_version,
                              std::string channel);
    ~SatisfactionSurveyManager();

    SatisfactionSurveyManager(const SatisfactionSurveyManager&) = delete;
    SatisfactionSurveyManager& operator=(const SatisfactionSurveyManager&) = delete;

    // Captures this launch's eligibility before prints from the current process
    // can change it and, when eligible, performs exactly one remote-config check.
    void begin_startup_check();

    // These three methods form a small startup barrier.  The survey remains the
    // lowest-priority startup dialog and is abandoned if the bounded window is
    // missed; it is never shown later in the same session.
    void expect_startup_restore();
    void notify_startup_restore_finished();
    void notify_post_init_finished(wxWindow* parent);

    // Records a successful network print only when at least one target resolves
    // to a Creality LAN/cloud device (deviceType 0/1).
    void record_successful_print(const SatisfactionSurveyPrintEvent& event);

    void shutdown();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace GUI
} // namespace Slic3r

#endif // slic3r_GUI_SatisfactionSurveyManager_hpp_
