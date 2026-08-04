#include "PrinterPresetConfig.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/Preset.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "3DBed.hpp"

#include "slic3r/GUI/GUI.hpp"
#include "slic3r/GUI/WebViewDialog.hpp"
#include "GUI_App.hpp"
#include "MainFrame.hpp"
#include "slic3r/Utils/ProfileFamilyLoader.hpp"

namespace Slic3r { namespace GUI {
struct AreaInfo
{
    std::string strModelName;
    std::string strAreaInfo;
    std::string strHeightInfo;
};

static void GetPrinterArea(json& pm, std::map<string, AreaInfo>& mapInfo)
{
    AreaInfo areaInfo;
    areaInfo.strModelName  = "";
    areaInfo.strAreaInfo   = "";
    areaInfo.strHeightInfo = "";

    string strInherits = "";
    string strName     = "";

    if (pm.contains("name")) {
        strName = pm["name"];
    }

    if (pm.contains("printer_model")) {
        areaInfo.strModelName = pm["printer_model"];
    }

    if (pm.contains("inherits")) {
        strInherits = pm["inherits"];
    }

    if (pm.contains("printable_height")) {
        areaInfo.strHeightInfo = pm["printable_height"];
    } else {
        auto it = mapInfo.find(strInherits);
        if (it != mapInfo.end()) {
            areaInfo.strHeightInfo = it->second.strHeightInfo;
        }
    }

    if (pm.contains("printable_area")) {
        string pt0 = "";
        string pt2 = "";
        if (pm["printable_area"].is_array()) {
            if (pm["printable_area"].size() < 5) {
                pt0 = pm["printable_area"][0];
                pt2 = pm["printable_area"][2];
            } else {
                std::vector<Vec2d> vecPt;
                int                size = pm["printable_area"].size();
                for (int i = 0; i < size; i++) {
                    std::string point_str = pm["printable_area"][i].get<std::string>();
                    size_t      pos       = point_str.find('x');
                    double      x         = std::stod(point_str.substr(0, pos));
                    double      y         = std::stod(point_str.substr(pos + 1));
                    vecPt.push_back(Vec2d(x, y));
                }

                Geometry::Circled circle = Geometry::circle_ransac(vecPt);
                double            dRad   = scaled<double>(circle.radius);
                int               dDim   = (int) ((2. * unscaled<double>(dRad)) + 0.1);
                areaInfo.strAreaInfo     = std::to_string(dDim) + "*" + std::to_string(dDim);
            }
        } else if (pm["printable_area"].is_string()) {
            string              printable_area_str = pm["printable_area"];
            std::vector<string> points;
            size_t              start = 0;
            size_t              end   = printable_area_str.find(',');
            while (end != std::string::npos) {
                points.push_back(printable_area_str.substr(start, end - start));
                start = end + 1;
                end   = printable_area_str.find(',', start);
            }
            points.push_back(printable_area_str.substr(start));

            if (points.size() < 5) {
                pt0 = points[0];
                pt2 = points[2];
            }
        }

        if (!pt0.empty() && !pt2.empty()) {
            size_t pos0   = pt0.find('x');
            size_t pos2   = pt2.find('x');
            int    pt0_x  = std::stoi(pt0.substr(0, pos0));
            int    pt0_y  = std::stoi(pt0.substr(pos0 + 1));
            int    pt2_x  = std::stoi(pt2.substr(0, pos2));
            int    pt2_y  = std::stoi(pt2.substr(pos2 + 1));
            int    length = pt2_x - pt0_x;
            int    width  = pt2_y - pt0_y;

            if ((length > 0) && (width > 0)) {
                areaInfo.strAreaInfo = std::to_string(length) + "*" + std::to_string(width);
            }
        }
    } else {
        auto it = mapInfo.find(strInherits);
        if (it != mapInfo.end()) {
            areaInfo.strAreaInfo = it->second.strAreaInfo;
        }
    }

    mapInfo[strName] = areaInfo;
}

static void StringReplace(string& strBase, string strSrc, string strDes)
{
    string::size_type pos    = 0;
    string::size_type srcLen = strSrc.size();
    string::size_type desLen = strDes.size();
    pos                      = strBase.find(strSrc, pos);
    while ((pos != string::npos)) {
        strBase.replace(pos, srcLen, strDes);
        pos = strBase.find(strSrc, (pos + desLen));
    }
}

struct PrinterInfo
{
    std::string name;
    std::string seriesNameList;
};

static bool toLowerAndContains(const std::string& str, const std::string& key)
{
    std::string lowerStr = str;
    std::string lowerKey = key;

    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);

    return lowerStr.find(lowerKey) != std::string::npos;
}

static bool customComparator(const PrinterInfo& a, const PrinterInfo& b)
{
    static const std::vector<std::string> order = {"flagship", "ender", "cr", "halot"};

    auto getPriority = [](const std::string& name) {
        for (size_t i = 0; i < order.size(); ++i) {
            if (toLowerAndContains(name, order[i])) {
                return i; // Higher priority for earlier keywords
            }
        }
        return order.size(); // Lowest priority if no keyword matches
    };

    return getPriority(a.name) < getPriority(b.name);
}

PrinterPresetConfig::PrinterPresetConfig() {}

PrinterPresetConfig::~PrinterPresetConfig() {}

std::vector<std::string> PrinterPresetConfig::getFilament(const std::string& printerName, const std::string& nozzle)
{
    std::vector<std::string> vtFileName;
    /*fs::path filamentPath = fs::path(resources_dir()).append("profiles").append("Creality").append("filament");
    for (auto& entry : fs::directory_iterator(filamentPath)) {
        std::string fileName = entry.path().filename().string();
        if (fileName.find(printerName) != std::string::npos) {
            vtFileName.push_back(fileName);
        }
    }
    */
    fs::path filamentPath = fs::path(resources_dir()).append("profiles").append("Creality.json");
    std::string filamentContent;
    LoadFile(filamentPath.string(), filamentContent);
    json jFilament;
    if (!filamentContent.empty())
    {
        jFilament = json::parse(filamentContent);
    }

    fs::path printerPath = fs::path(resources_dir()).append("profiles").append("Creality").append("machine").append(printerName+".json");
    std::string printerContent;
    LoadFile(printerPath.string(), printerContent);
    json jPrinter;
    if (!printerContent.empty())
    {
        jPrinter = json::parse(printerContent);
    }
    std::string ssDefaultFilament;
    if (jPrinter.contains("default_materials") && jPrinter["default_materials"].is_string()) {
        ssDefaultFilament = jPrinter["default_materials"];
    }
    
    std::string ssPrinter = printerName + " " + nozzle + " nozzle";
    if (jFilament.contains("filament_list") && jFilament["filament_list"].is_array()) {
        for (auto& filament : jFilament["filament_list"]) {
            if (filament.contains("name") && filament["name"].is_string()) {
                std::string fName = filament["name"];
                if (fName.find(ssPrinter) == std::string::npos)
                    continue;
                size_t pos   = fName.find(" @");
                if (pos != std::string::npos) {
                    fName = fName.substr(0, pos);
                    if (fName.length() > 0 && fName.c_str()[fName.length() - 1] == ' ') {
                        fName = fName.substr(0, pos - 1);
                    }
                    if (ssDefaultFilament.find(fName) != std::string::npos) {
                        vtFileName.push_back(filament["name"]);
                    }
                }
            }
        }
    }
    return vtFileName; 
}

bool PrinterPresetConfig::getPrinterDefaultMaterials(const std::string& vendor, const std::string& printerName,  std::vector<std::string>& vtPrinterDefaultMaterials)
{
    vtPrinterDefaultMaterials.clear();
    try {
        fs::path printerPath = fs::path(resources_dir()).append("profiles").append(vendor).append("machine").append(printerName + ".json");
        std::string printerContent;
        LoadFile(printerPath.string(), printerContent);
        json        jPrinter = json::parse(printerContent);
        std::string ssDefaultFilament;
        if (printerName.find("nozzle") != std::string::npos) {
            if (jPrinter.contains("printer_model") && jPrinter["printer_model"].is_string()) {
                std::string printer_model = jPrinter["printer_model"];
                if (!printer_model.empty()) {
                    printerPath =
                        fs::path(resources_dir()).append("profiles").append(vendor).append("machine").append(printer_model + ".json");
                    std::string printerContent;
                    LoadFile(printerPath.string(), printerContent);
                    jPrinter = json::parse(printerContent);
                }
            }
        }
        if (jPrinter.contains("default_materials") && jPrinter["default_materials"].is_string()) {
            ssDefaultFilament = jPrinter["default_materials"];
        }
        // size_t count = ssDefaultFilament.size();
        // size_t offset = 0;
        // size_t sublen = 0;
        // for (size_t i = 0; i < count; ++i) {
        //    sublen++;
        //    if (ssDefaultFilament.c_str()[i] == ';') {
        //        vtPrinterDefaultMaterials.push_back(ssDefaultFilament.substr(offset, sublen-1));
        //        offset = i + 1;
        //        sublen = 0;
        //    }
        //}
        boost::split(vtPrinterDefaultMaterials, ssDefaultFilament, boost::is_any_of(";"));
    }
    catch (...) {
        return false;
    }
    return true;
}

bool PrinterPresetConfig::getPrinterDefaultMaterials(const std::string& vendor, const std::string& printerName,  std::vector<std::pair<std::string, std::string>>& vtPrinterDefaultMaterials)
{
    std::vector<std::string> tmpvtPrinterDefaultMaterials;
    getPrinterDefaultMaterials(vendor, printerName, tmpvtPrinterDefaultMaterials);
    vtPrinterDefaultMaterials.clear();
    try {
        std::string newPrinterName = printerName;
        for (auto item : tmpvtPrinterDefaultMaterials) {
            std::string filamentName;
            fs::path filamentPath;
            if (item.find("@") == std::string::npos) {
                //filamentPath = fs::path(resources_dir()).append("profiles").append(vendor).append("filament").append(item + " @" + newPrinterName + ".json");
                filamentPath = fs::path(resources_dir()).append("profiles").append(vendor).append("filament").append(item + ".json");
                if (!fs::exists(filamentPath)) {
                    filamentPath = fs::path(resources_dir()).append("profiles").append(vendor).append("filament").append(item + " @" + newPrinterName + ".json");
                }
            } else {
                filamentPath = fs::path(resources_dir()).append("profiles").append(vendor).append("filament").append(item + ".json");
            }
            json jFilament = {};
            do {
                jFilament = {};
                std::string filamentContent;
                LoadFile(filamentPath.string(), filamentContent);
                try {
                    jFilament = json::parse(filamentContent);
                } catch (...) {
                    break;
                }
                if (jFilament.contains("filament_type")) {
                    vtPrinterDefaultMaterials.push_back(std::make_pair(item, jFilament["filament_type"][0]));
                    break;
                } else if (jFilament.contains("inherits")) {
                    filamentPath = fs::path(resources_dir()).append("profiles").append(vendor).append("filament").append(std::string(jFilament["inherits"]) + ".json");
                }
            } while (jFilament.contains("inherits"));
        }
    }
    catch (...) {
        return false;
    }
    return true;
}

int PrinterPresetConfig::LoadProfile() { 
    try {
        ProfileFamilyLoader::get_instance()->request_and_wait();
        ProfileFamilyLoader::get_instance()->get_result(m_ProfileJson, m_MachineJson, bbl_bundle_rsrc);

        const auto enabled_filaments = wxGetApp().app_config->has_section(AppConfig::SECTION_FILAMENTS) ?
                                                                wxGetApp().app_config->get_section(AppConfig::SECTION_FILAMENTS) :
                                                                std::map<std::string, std::string>();
        m_appconfig_new.set_vendors(*wxGetApp().app_config);
        m_appconfig_new.set_section(AppConfig::SECTION_FILAMENTS, enabled_filaments);

        for (auto it = m_ProfileJson["model"].begin(); it != m_ProfileJson["model"].end(); ++it) {
            if (it.value().is_object()) {
                json&       temp_model      = it.value();
                std::string model_name      = temp_model["model"];
                std::string vendor_name     = temp_model["vendor"];
                std::string nozzle_diameter = temp_model["nozzle_diameter"];
                std::string selected;
                boost::trim(nozzle_diameter);
                std::string nozzle;
                bool        enabled = false, first = true;
                while (nozzle_diameter.size() > 0) {
                    auto pos = nozzle_diameter.find(';');
                    if (pos != std::string::npos) {
                        nozzle  = nozzle_diameter.substr(0, pos);
                        enabled = m_appconfig_new.get_variant(vendor_name, model_name, nozzle);
                        if (enabled) {
                            if (!first)
                                selected += ";";
                            selected += nozzle;
                            first = false;
                        }
                        nozzle_diameter = nozzle_diameter.substr(pos + 1);
                        boost::trim(nozzle_diameter);
                    } else {
                        enabled = m_appconfig_new.get_variant(vendor_name, model_name, nozzle_diameter);
                        if (enabled) {
                            if (!first)
                                selected += ";";
                            selected += nozzle_diameter;
                        }
                        break;
                    }
                }
                temp_model["nozzle_selected"] = selected;
                // m_ProfileJson["model"][a]["nozzle_selected"]
            }
        }

        if (m_ProfileJson["model"].size() == 1) {
            std::string strNozzle                        = m_ProfileJson["model"][0]["nozzle_diameter"];
            m_ProfileJson["model"][0]["nozzle_selected"] = strNozzle;
        }

        for (auto it = m_ProfileJson["filament"].begin(); it != m_ProfileJson["filament"].end(); ++it) {
            // json temp_filament = it.value();
            std::string filament_name = it.key();
            if (enabled_filaments.find(filament_name) != enabled_filaments.end())
                m_ProfileJson["filament"][filament_name]["selected"] = 1;
        }

        //----region
        m_Region = wxGetApp().app_config->get("region");
        if (m_Region.empty()) {
            wxString strlang = wxGetApp().current_language_code_safe();
            if (strlang == "zh_CN") {
                m_Region = "China";
            } else {
                m_Region = "North America";
            }
        }
        m_ProfileJson["region"] = m_Region;

        m_ProfileJson["network_plugin_install"]     = wxGetApp().app_config->get("app", "installed_networking");
        m_ProfileJson["network_plugin_compability"] = wxGetApp().is_compatibility_version() ? "1" : "0";
        network_plugin_ready                        = wxGetApp().is_compatibility_version();
        // write regin into app_config
        wxGetApp().app_config->set("region", m_Region);
        {
            // update the webview region
            std::vector<wxString> prefs;

            wxString    strlang           = wxGetApp().current_language_code_safe();
            std::string country_code      = m_Region;
            std::string use_inches        = wxGetApp().app_config->get("use_inches");
            std::string syncPresetEnabled = wxGetApp().app_config->get("sync_user_preset") == "true" ? "1" : "0";

            prefs.push_back(strlang);
            prefs.push_back(wxString(country_code));
            prefs.push_back(wxString(use_inches));
            prefs.push_back(wxString(syncPresetEnabled));

            if (wxGetApp().mainframe->m_webview) {
                wxGetApp().mainframe->m_webview->sync_preferences(prefs);
            }
        }
    } catch (std::exception& e) {
        // wxLogMessage("GUIDE: load_profile_error  %s ", e.what());
        // wxMessageBox(e.what(), "", MB_OK);
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ", error: " << e.what() << std::endl;
    }

    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ", finished";
    return 0;
}

bool PrinterPresetConfig::LoadFile(std::string jPath, std::string& sContent)
{
    try {
        boost::nowide::ifstream t(jPath);
        std::stringstream       buffer;
        buffer << t.rdbuf();
        sContent = buffer.str();
        BOOST_LOG_TRIVIAL(trace) << __FUNCTION__ << boost::format(", load %1% into buffer") % jPath;
    } catch (std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ",  got exception: " << e.what();
        return false;
    }

    return true;
}

}}
