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

int nsUpdateParams::CXCloud::getNeedUpdatePrintes(std::vector<std::string>& vtNeedUpdatePrinter)
{
    vtNeedUpdatePrinter.clear();
    const auto installed = loadInstalledPrinterVersions();
    const auto machine_dir = fs::path(data_dir()) / "system" / "Creality" / "machine";
    int result = -1;
    const std::string url = get_cloud_api_url() + "/api/cxy/v2/slice/profile/official/printerList";
    Http::set_extra_headers(wxGetApp().get_extra_header());
    Http http = Http::post(url);
    const auto uuid = boost::uuids::random_generator()();
    const json request = {{"engineVersion", "3.0.0"}};

    // Visibility may change during this process (whitelist, login or region).
    // Never reuse a successful request forever or infer visibility from disk.
    http.header("Content-Type", "application/json")
        .header("__CXY_REQUESTID_", to_string(uuid))
        .timeout_connect(2)
        .timeout_max(10)
        .set_post_body(request.dump())
        .on_complete([&](std::string body, unsigned status) {
            if (status != 200)
                return;
            try {
                const json response = json::parse(body);
                if (response.value("code", -1) != 0 || !response.contains("result") ||
                    !response["result"].contains("printerList") ||
                    !response["result"]["printerList"].is_array())
                    return;
                std::vector<std::string> pending;
                for (const auto& entry : ProfileUpdatePolicy::select_packages(response["result"]["printerList"])) {
                    const auto local = installed.find(entry.first);
                    const std::string version = local == installed.end() ? "" : local->second;
                    if (ProfileUpdatePolicy::needs_update(fs::exists(machine_dir / (entry.first + ".json")),
                                                          version, entry.second.value("showVersion", std::string())))
                        pending.push_back(entry.first);
                }
                vtNeedUpdatePrinter = std::move(pending);
                result = 0;
            } catch (const std::exception& error) {
                BOOST_LOG_TRIVIAL(error) << "CXCloud getNeedUpdatePrintes parse error: " << error.what();
            }
        })
        .on_error([&](std::string, std::string error, unsigned status) {
            BOOST_LOG_TRIVIAL(error) << "CXCloud getNeedUpdatePrintes fail: " << error << ", status=" << status;
        })
        .perform_sync();
    return result;
}

UpdateParams::UpdateParams() {}

UpdateParams& UpdateParams::getInstance()
{
    static UpdateParams instance;
    return instance;
}

void UpdateParams::checkParamsNeedUpdate()
{
    std::vector<std::string> need_update;
    if (m_CXCloud.getNeedUpdatePrintes(need_update) != 0)
        return;

    const auto vtCurPrinter = nsUpdateParams::getAddedPrinterNames();

    bool hasPrinterNeedUpdate = false;
    for (auto item : vtCurPrinter) {
        if (std::find(need_update.begin(), need_update.end(), item) != need_update.end()) {
            hasPrinterNeedUpdate = true;
            break;
        }
    }
    if (hasPrinterNeedUpdate) {
        wxGetApp().CallAfter([this]() {
            std::string ssTip = _u8L("Detected a new parameter package to update, would you like to update it?");
            wxGetApp().plater()->get_notification_manager()->push_update_params_tip(ssTip);
        });
    } else {
        wxGetApp().CallAfter([this]() {
            closeParamsUpdateTip();
        });
    }
}

void UpdateParams::closeParamsUpdateTip() {
    std::string ssTip = _u8L("Detected a new parameter package to update, would you like to update it?");

    wxGetApp().plater()->get_notification_manager()->close_update_params_tip(ssTip);
}

void UpdateParams::hasUpdateParams() {
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
