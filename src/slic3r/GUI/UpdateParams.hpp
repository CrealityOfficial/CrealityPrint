#ifndef slic3r_UpdateParams_hpp_
#define slic3r_UpdateParams_hpp_
#include <atomic>
#include <memory>
#include <thread>
#include <wx/timer.h>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <json_diff.hpp>

namespace Slic3r {
namespace GUI {

class PresetUpdateDialog;

namespace nsUpdateParams {
// Read installation receipts afresh; catalog downloads are not installations.
std::map<std::string, std::string> loadInstalledPrinterVersions();
// Share the same definition of added printers between tips and automatic updates.
std::set<std::string> getAddedPrinterNames();

class CXCloud
{
public:
    CXCloud()  = default;
    ~CXCloud() = default;

    int getAvailablePackages(nlohmann::json &packages, const std::string &url,
                            const std::map<std::string, std::string> &headers, const std::atomic<bool> &cancel);
};
} // namespace nsUpdateParams

class UpdateParams
{
public:
    static UpdateParams& getInstance();

    void checkParamsNeedUpdate(bool manual = false);
    void finishUpdate(const nlohmann::json &installed, const wxString &message, bool failed = false);
    // Called on the UI thread after startup and before application teardown.
    void startPeriodicChecks();
    void shutdown();
    void closeParamsUpdateTip();
    void hasUpdateParams();

private:
    UpdateParams();
    void begin_check(bool manual);
    void ensure_dialog();
    void present_pending();
    void dialog_action();
    void dismiss_dialog(bool remind_later);
    bool context_matches() const;

private:
    nsUpdateParams::CXCloud m_CXCloud;
    std::unique_ptr<wxTimer> m_check_timer;
    std::thread m_check_thread;
    std::unique_ptr<wxTimer> m_present_timer;
    PresetUpdateDialog *m_dialog{nullptr}; // Parent-owned; cleared before Destroy().
    nlohmann::json m_catalog = nlohmann::json::array();
    nlohmann::json m_offered = nlohmann::json::array();
    std::map<std::string, std::string> m_headers;
    std::string m_url;
    bool m_manual{false};
    bool m_discard_result{false};
    std::atomic<bool> m_stopping{false};
    bool m_check_running{false}; // UI thread only.
    size_t m_install_generation{0}; // Discard replies from before an installation.
};

}
}

#endif
