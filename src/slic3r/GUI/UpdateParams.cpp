#include "UpdateParams.hpp"
#include "ProfileUpdatePolicy.hpp"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "GUI.hpp"
#include "GUI_App.hpp"
#include "Plater.hpp"
#include "NotificationManager.hpp"
#include "MsgDialog.hpp"
#include "UpdateDialogs.hpp"
#include "MainFrame.hpp"
#include <chrono>

namespace Slic3r {
namespace GUI {

std::map<std::string, std::string> nsUpdateParams::loadInstalledPrinterVersions()
{
    const auto file = fs::path(data_dir()) / "system" / "Creality" / "profile_version.json";
    try {
        if (fs::exists(file)) {
            json records;
            boost::nowide::ifstream input(file.string());
            input >> records;
            if (records.contains("Creality"))
                return ProfileUpdatePolicy::installed_versions(records["Creality"]);
        }
    } catch (const std::exception& error) {
        // Keep the original receipt for diagnostics. Missing/invalid versions
        // will be repaired only after the corresponding package is installed.
        BOOST_LOG_TRIVIAL(warning) << "Cannot read installed profile versions: " << error.what();
    }
    return {};
}

int nsUpdateParams::CXCloud::getAvailablePackages(json &packages, const std::string &url,
                                                 const std::map<std::string, std::string> &headers,
                                                 const std::atomic<bool> &cancel)
{
    auto fetch = [&](const std::map<std::string, std::string> &request_headers, json &list) {
        bool ok = false;
        Http http = Http::post(url);
        http.clear_header();
        for (const auto &header : request_headers) http.header(header.first, header.second);
        http.header("Content-Type", "application/json")
            .header("__CXY_REQUESTID_", to_string(boost::uuids::random_generator()()))
            .timeout_connect(2).timeout_max(10)
            .on_progress([&cancel](Http::Progress, bool &stop) { stop = cancel.load(); })
            .set_post_body(json({{"engineVersion", "3.0.0"}}).dump())
            .on_complete([&](std::string body, unsigned status) {
                if (status != 200) return;
                try {
                    const auto response = json::parse(body);
                    if (response.value("code", -1) == 0 && response.contains("result") &&
                        response["result"].contains("printerList") && response["result"]["printerList"].is_array()) {
                        list = response["result"]["printerList"];
                        ok = true;
                    }
                } catch (const std::exception &error) {
                    BOOST_LOG_TRIVIAL(error) << "Invalid parameter catalog: " << error.what();
                }
            })
            .on_error([](std::string, std::string error, unsigned status) {
                BOOST_LOG_TRIVIAL(error) << "Parameter catalog request failed: " << status << " " << error;
            }).perform_sync();
        return ok && !cancel;
    };
    auto stable_headers = headers;
    stable_headers["__CXY_OS_LANG_"] = "0";
    if (!fetch(stable_headers, packages)) return -1;
    if (headers != stable_headers) {
        json localized;
        if (fetch(headers, localized)) {
            std::map<std::string, std::string> notes;
            for (const auto &item : localized)
                if (item.is_object() && item.contains("zipUrl") && item["zipUrl"].is_string())
                    notes[item["zipUrl"].get<std::string>()] = ProfileUpdatePolicy::release_notes(item);
            for (auto &item : packages)
                if (item.is_object() && item.contains("zipUrl") && item["zipUrl"].is_string()) {
                    auto found = notes.find(item["zipUrl"].get<std::string>());
                    if (found != notes.end() && !found->second.empty()) item["descriptionI18n"] = found->second;
                }
        }
    }
    return cancel ? -1 : 0;
}

namespace {
int64_t preset_update_now()
{
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}
json read_preset_snoozes()
{
    const auto text = wxGetApp().app_config->get("preset_update", "snoozed");
    const auto parsed = json::parse(text, nullptr, false);
    return parsed.is_object() ? parsed : json::object();
}
std::string snooze_scope(const std::string &url, const std::map<std::string, std::string> &headers)
{
    const auto user = headers.find("__CXY_UID_");
    return url + "|" + (user == headers.end() ? "" : user->second);
}
json current_added_updates(const json &catalog)
{
    const auto added = nsUpdateParams::getAddedPrinterNames();
    std::set<std::string> files;
    for (const auto &name : added)
        if (fs::exists(fs::path(data_dir()) / "system" / "Creality" / "machine" / (name + ".json"))) files.insert(name);
    json pending = json::array();
    for (const auto &item : ProfileUpdatePolicy::select_added_updates(catalog, added, nsUpdateParams::loadInstalledPrinterVersions(), files))
        pending.push_back(item.second);
    return pending;
}
} // namespace

UpdateParams::UpdateParams() {}

UpdateParams& UpdateParams::getInstance()
{
    static UpdateParams instance;
    return instance;
}

void UpdateParams::startPeriodicChecks()
{
    if (m_stopping || m_check_timer)
        return;
    m_check_timer = std::make_unique<wxTimer>();
    m_check_timer->Bind(wxEVT_TIMER, [this](wxTimerEvent &) { checkParamsNeedUpdate(); });
    // Startup retains its existing check; the first periodic check is 24 hours later.
    m_check_timer->Start(24 * 60 * 60 * 1000);
}

void UpdateParams::shutdown()
{
    m_stopping = true;
    m_check_timer.reset();
    m_present_timer.reset();
    if (m_check_thread.joinable()) m_check_thread.join();
    if (m_dialog) {
        auto *dialog = m_dialog;
        m_dialog = nullptr;
        dialog->Destroy();
    }
}

bool UpdateParams::context_matches() const
{
    return m_url == get_cloud_api_url() + "/api/cxy/v2/slice/profile/official/printerList" &&
           m_headers == wxGetApp().get_extra_header();
}

void UpdateParams::ensure_dialog()
{
    if (!m_dialog) {
        m_dialog = new PresetUpdateDialog(wxGetApp().mainframe, [this] { dialog_action(); }, [this](bool remind_later) { dismiss_dialog(remind_later); });
        auto *dialog = m_dialog;
        dialog->Bind(wxEVT_DESTROY, [this, dialog](wxWindowDestroyEvent &event) {
            // Recreating the main window can destroy children without a close event.
            if (event.GetEventObject() == dialog && m_dialog == dialog) m_dialog = nullptr;
            event.Skip();
        });
        m_dialog->Show();
    }
    m_dialog->Raise();
}

void UpdateParams::checkParamsNeedUpdate(bool manual)
{
    if (m_stopping) return;
    if (!wxIsMainThread()) {
        wxGetApp().CallAfter([this, manual] { checkParamsNeedUpdate(manual); });
        return;
    }
    if (m_dialog) { if (manual) m_dialog->Raise(); return; }
    auto &app = wxGetApp();
    if (!app.plater() || !app.preset_bundle || app.native_profile_update_running()) return;
    if (m_check_running) {
        if (manual) { m_manual = true; m_discard_result = false; ensure_dialog(); }
        return;
    }
    begin_check(manual);
}

void UpdateParams::begin_check(bool manual)
{
    if (m_stopping || m_check_running) return;
    m_manual = manual;
    m_discard_result = false;
    m_catalog = json::array();
    m_offered = json::array();
    if (m_present_timer) m_present_timer->Stop();
    if (manual) {
        ensure_dialog();
        m_dialog->set_state(PresetUpdateDialog::State::Checking, m_offered);
    }
    if (nsUpdateParams::getAddedPrinterNames().empty()) {
        if (manual) m_dialog->set_state(PresetUpdateDialog::State::Info, m_offered,
            _L("No printers have been added. Add a printer before checking for updates."));
        return;
    }
    if (m_check_thread.joinable()) m_check_thread.join();
    m_url = get_cloud_api_url() + "/api/cxy/v2/slice/profile/official/printerList";
    m_headers = wxGetApp().get_extra_header();
    const auto generation = m_install_generation;
    m_check_running = true;
    try {
        m_check_thread = std::thread([this, url = m_url, headers = m_headers, generation] {
            json catalog = json::array();
            int result = -1;
            try { result = m_CXCloud.getAvailablePackages(catalog, url, headers, m_stopping); }
            catch (const std::exception &error) { BOOST_LOG_TRIVIAL(error) << "Parameter update check failed: " << error.what(); }
            if (m_stopping) return;
            wxGetApp().CallAfter([this, generation, result, catalog = std::move(catalog)] {
                m_check_running = false;
                if (m_stopping || m_discard_result) return;
                if (generation != m_install_generation || !context_matches() || wxGetApp().native_profile_update_running()) {
                    if (m_manual && m_dialog) m_dialog->set_state(PresetUpdateDialog::State::Error, json::array(),
                        _L("The printer list or account has changed. Please check again."));
                    return;
                }
                if (result != 0) {
                    if (m_manual && m_dialog) m_dialog->set_state(PresetUpdateDialog::State::Error, json::array(),
                        _L("Failed to check preset updates. Please check your network and try again."));
                    return;
                }
                m_catalog = catalog;
                present_pending();
            });
        });
    } catch (const std::exception &error) {
        m_check_running = false;
        BOOST_LOG_TRIVIAL(error) << "Cannot start parameter update check: " << error.what();
        if (m_dialog) m_dialog->set_state(PresetUpdateDialog::State::Error, json::array(),
            _L("Failed to check preset updates. Please check your network and try again."));
    }
}

void UpdateParams::present_pending()
{
    if (m_stopping || !context_matches()) {
        if (m_present_timer) m_present_timer->Stop();
        return;
    }
    auto &app = wxGetApp();
    json pending;
    try { pending = current_added_updates(m_catalog); }
    catch (const std::exception &error) {
        BOOST_LOG_TRIVIAL(error) << "Cannot select parameter updates: " << error.what();
        if (m_present_timer) m_present_timer->Stop();
        if (m_manual && m_dialog) m_dialog->set_state(PresetUpdateDialog::State::Error, json::array(),
            _L("Failed to check preset updates. Please check your network and try again."));
        return;
    }
    if (!m_manual) {
        const auto records = read_preset_snoozes();
        const auto scope = snooze_scope(m_url, m_headers);
        const auto now = preset_update_now();
        json visible = json::array();
        for (const auto &item : pending)
            if (!ProfileUpdatePolicy::is_snoozed(records, scope, ProfileUpdatePolicy::profile_name(item),
                                                 item.value("showVersion", std::string()), now)) visible.push_back(item);
        pending = std::move(visible);
    }
    if (pending.empty()) {
        if (m_present_timer) m_present_timer->Stop();
        if (m_manual && m_dialog) m_dialog->set_state(PresetUpdateDialog::State::Info, pending,
            _L("All added printers are up to date."));
        return;
    }
    if (!m_manual && (m_dialog || app.native_profile_update_running() || !app.mainframe->IsActive() ||
        app.mainframe->IsIconized() || !dialogStack.empty() || app.plater()->is_background_process_slicing())) {
        if (!m_present_timer) {
            m_present_timer = std::make_unique<wxTimer>();
            m_present_timer->Bind(wxEVT_TIMER, [this](wxTimerEvent &) { present_pending(); });
        }
        if (!m_present_timer->IsRunning()) m_present_timer->Start(1000);
        return;
    }
    if (m_present_timer) m_present_timer->Stop();
    m_offered = std::move(pending);
    ensure_dialog();
    m_dialog->set_state(PresetUpdateDialog::State::Available, m_offered);
}

void UpdateParams::dialog_action()
{
    if (!m_dialog || m_stopping) return;
    const auto state = m_dialog->state();
    if (state == PresetUpdateDialog::State::Error) { begin_check(true); return; }
    if (state == PresetUpdateDialog::State::Complete || state == PresetUpdateDialog::State::Info) { m_dialog->Close(); return; }
    if (state != PresetUpdateDialog::State::Available) return;
    if (!context_matches() || current_added_updates(m_offered) != m_offered) {
        m_dialog->set_state(PresetUpdateDialog::State::Error, json::array(),
            _L("The printer list or account has changed. Please check again."));
        return;
    }
    m_dialog->set_state(PresetUpdateDialog::State::Updating, m_offered);
    wxGetApp().update_preset_params_from_notification(m_offered);
}

void UpdateParams::dismiss_dialog(bool remind_later)
{
    if (remind_later && m_dialog && m_dialog->state() == PresetUpdateDialog::State::Available) {
        auto records = read_preset_snoozes();
        ProfileUpdatePolicy::snooze(records, snooze_scope(m_url, m_headers), m_offered, preset_update_now());
        auto *config = wxGetApp().app_config;
        config->set("preset_update", "snoozed", records.dump());
        try { config->save(); }
        catch (const std::exception &error) { BOOST_LOG_TRIVIAL(error) << "Cannot save preset reminder: " << error.what(); }
    }
    m_discard_result = true;
    m_manual = false;
    m_dialog = nullptr;
    m_catalog = json::array();
    m_offered = json::array();
}

void UpdateParams::finishUpdate(const json &installed, const wxString &message, bool failed)
{
    if (m_stopping) return;
    ensure_dialog();
    m_dialog->set_state(failed ? PresetUpdateDialog::State::Error :
        installed.empty() ? PresetUpdateDialog::State::Info : PresetUpdateDialog::State::Complete, installed, message);
}

void UpdateParams::closeParamsUpdateTip() {
    std::string ssTip = _u8L("Detected a new parameter package to update, would you like to update it?");

    wxGetApp().plater()->get_notification_manager()->close_update_params_tip(ssTip);
}

void UpdateParams::hasUpdateParams() {
    ++m_install_generation;
    closeParamsUpdateTip();
}

std::set<std::string> nsUpdateParams::getAddedPrinterNames()
{
    std::set<std::string> printers;

    const std::deque<Preset>& printer_presets = GUI::wxGetApp().preset_bundle->printers.get_presets();
    for (const Preset& printer_preset : printer_presets) {
        std::string preset_name = printer_preset.name;
        if (!printer_preset.is_visible || printer_preset.is_default || printer_preset.is_project_embedded)
            continue;

        if (printer_preset.is_user()) {
            preset_name = printer_preset.inherits();
        }
        if (!preset_name.empty())
            printers.insert(preset_name);
    }
    return printers;
}

}
}
