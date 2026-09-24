#include "UpdateParams.hpp"

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

static std::string update_profile_model_name(const json& printer)
{
    std::string model = printer.value("name", std::string());
    if (model.find("Creality") == std::string::npos && model.find("SPARKX") == std::string::npos)
        model = "Creality " + model;
    return model;
}

static bool update_profile_uses_bare_multi_extruder_name(const json& printer)
{
    if (!printer.contains("nozzleDiameter") || !printer["nozzleDiameter"].is_array())
        return false;
    return printer["nozzleDiameter"].size() > 1 &&
        printer.value("printerIntName", std::string()) != "Sermoon D3 Pro" &&
        update_profile_model_name(printer) != "Creality Sermoon D3 Pro";
}

static std::string update_profile_name(const json& printer, const std::string& nozzle)
{
    const std::string model = update_profile_model_name(printer);
    return update_profile_uses_bare_multi_extruder_name(printer) ? model : model + " " + nozzle + " nozzle";
}

int nsUpdateParams::CXCloud::getNeedUpdatePrintes(std::vector<std::string>& vtNeedUpdatePrinter)
{
    vtNeedUpdatePrinter.clear();

    if (m_mapLocalPrinterVersion.empty()) {
        loadLocalPrinter();
    }
    if (m_bHasRequestedCXCloud) {
        for (auto& item : m_mapRemotePrinterVersion) {
            auto iter = m_mapLocalPrinterVersion.find(item.first);
            if (iter != m_mapLocalPrinterVersion.end() && item.second > iter->second) {
                vtNeedUpdatePrinter.push_back(item.first);
            }
        }
        return 0;
    }

    m_mapRemotePrinterVersion.clear();
    int         nRet                  = -1;
    std::string version               = "3.0.0"; // preset_bundle->get_vendor_profile_version(PresetBundle::BBL_BUNDLE).to_string();
    std::string version_type          = get_vertion_type();
    std::string base_url              = get_cloud_api_url();
    auto        preupload_profile_url = "/api/cxy/v2/slice/profile/official/printerList";
    Http::set_extra_headers(wxGetApp().get_extra_header());
    Http http = Http::post(base_url + preupload_profile_url);

    BOOST_LOG_TRIVIAL(warning) << "CXCloud getNeedUpdatePrintes [" << base_url << preupload_profile_url << "]";

    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    json               j;
    j["version"] = version;

    http.header("Content-Type", "application/json")
        .header("__CXY_REQUESTID_", to_string(uuid))
        .timeout_connect(2)
        .timeout_max(10)
        .set_post_body(j.dump())
        .on_complete([&](std::string body, unsigned status) {
            if(status != 200)
            {
                BOOST_LOG_TRIVIAL(error) << "CXCloud getNeedUpdatePrintes fail.status=" << status;
                nRet = 1; // error code
                return;
            }
            try{
                json j = json::parse(body);
                if (j["code"] != 0) {
                    nRet = 1; // error code

                    BOOST_LOG_TRIVIAL(error) << "CXCloud getNeedUpdatePrintes fail.code=" << j["code"];
                    return;
                }
                if (j.contains("result") && j["result"].contains("printerList") && j["result"]["printerList"].is_array()) {
                    std::map<std::string, bool> selected_standard_packages;
                    for (auto& printer : j["result"]["printerList"]) {
                        if (printer.contains("nozzleDiameter") && printer["nozzleDiameter"].is_array()) {
                            for (auto& nozzle : printer["nozzleDiameter"]) {
                                const std::string nozzle_name = nozzle.get<std::string>();
                                const std::string printerName = update_profile_name(printer, nozzle_name);
                                const std::string version = printer["showVersion"].get<std::string>();
                                const bool standard_package = nozzle_name == "0.4";
                                const auto selected = selected_standard_packages.find(printerName);
                                if (selected != selected_standard_packages.end() &&
                                    (selected->second || !standard_package))
                                    continue;

                                m_mapRemotePrinterVersion[printerName] = version;
                                selected_standard_packages[printerName] = standard_package;
                                if (update_profile_uses_bare_multi_extruder_name(printer))
                                    break;
                            }
                        }
                    }
                    for (const auto& [printer_name, version] : m_mapRemotePrinterVersion) {
                        const auto local = m_mapLocalPrinterVersion.find(printer_name);
                        if (local != m_mapLocalPrinterVersion.end() && version > local->second)
                            vtNeedUpdatePrinter.push_back(printer_name);
                    }
                }
            } catch (std::exception& e) {
                BOOST_LOG_TRIVIAL(error) << "CXCloud getNeedUpdatePrintes parse error: " << e.what();
                nRet = 1; // error code
                return;
            }
            m_bHasRequestedCXCloud = true;
            nRet = 0;
        })
        .on_error([&](std::string body, std::string error, unsigned status) {
            BOOST_LOG_TRIVIAL(error) << "CXCloud getNeedUpdatePrintes fail.err=" << error << ",status=" << status;
        })
        .on_progress([&](Http::Progress progress, bool& cancel) {

        })
        .perform_sync();

    return nRet;
}

void nsUpdateParams::CXCloud::clearLocalPrinter() {
    m_mapLocalPrinterVersion.clear();
}

int nsUpdateParams::CXCloud::loadLocalPrinter() {

    m_mapLocalPrinterVersion.clear();

    //auto cache_file = fs::path(data_dir()).append("profile_version.json");
    //if (!fs::exists(cache_file)) {
    //    auto src = fs::path(resources_dir()).append("profiles").append("Creality").append("profile_version.json");
    //    fs::copy_file(src, cache_file);
    //}
    auto cache_file = fs::path(data_dir()).append("system").append("Creality").append("profile_version.json");
    json cache_json;
    try{
    if (fs::exists(cache_file)) {
        boost::nowide::ifstream ifs(cache_file.string());
        ifs >> cache_json;
    }
    if (cache_json.contains("Creality") && cache_json["Creality"].is_array()) {
        std::map<std::string, bool> selected_standard_packages;
        for (auto& item : cache_json["Creality"]) {
            if (!item.contains("nozzleDiameter") || !item["nozzleDiameter"].is_array() || item["nozzleDiameter"].empty())
                continue;
            const std::string nozzle = item["nozzleDiameter"][0].get<std::string>();
            const std::string printer_name = update_profile_name(item, nozzle);
            const bool standard_package = nozzle == "0.4";
            const auto selected = selected_standard_packages.find(printer_name);
            if (selected != selected_standard_packages.end() && (selected->second || !standard_package))
                continue;
            m_mapLocalPrinterVersion[printer_name] = item["showVersion"].get<std::string>();
            selected_standard_packages[printer_name] = standard_package;
        }
    }
    }catch(std::exception& e) {
        fs::remove(cache_file);
        BOOST_LOG_TRIVIAL(error) << "CXCloud loadLocalPrinter parse error: " << e.what();
    }

    return 0;
}

UpdateParams::UpdateParams() {}

UpdateParams& UpdateParams::getInstance()
{
    static UpdateParams instance;
    return instance;
}

void UpdateParams::checkParamsNeedUpdate()
{
    if (m_bHasUpdateParams) {
        m_CXCloud.clearLocalPrinter();
        m_vtNeedUpdatePrinter.clear();
        m_bHasUpdateParams = false;
    }

    if (m_vtNeedUpdatePrinter.empty()) {
        m_CXCloud.getNeedUpdatePrintes(m_vtNeedUpdatePrinter);
    }

    std::vector<std::string> vtCurPrinter;
    getCurrentPrinter(vtCurPrinter);

    bool hasPrinterNeedUpdate = false;
    for (auto item : vtCurPrinter) {
        if (std::find(m_vtNeedUpdatePrinter.begin(), m_vtNeedUpdatePrinter.end(), item) != m_vtNeedUpdatePrinter.end()) {
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
    m_bHasUpdateParams = true;
}

void UpdateParams::getCurrentPrinter(std::vector<std::string>& vtCurPrinter) 
{
    vtCurPrinter.clear();

    const std::deque<Preset>& printer_presets = GUI::wxGetApp().preset_bundle->printers.get_presets();
    for (const Preset& printer_preset : printer_presets) {
        std::string preset_name = printer_preset.name;
        if (!printer_preset.is_visible || printer_preset.is_default || printer_preset.is_project_embedded)
            continue;

        if (printer_preset.is_user()) {
            preset_name = printer_preset.inherits();
        }
        vtCurPrinter.push_back(preset_name);
    }
}

}
}
