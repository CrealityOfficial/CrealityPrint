#include "PrinterCover.hpp"
#include "libslic3r/PrinterCover.hpp"
#include "ProfileFamilyLoader.hpp"

#include <thread>
#include <wx/wx.h>
#include <boost/filesystem/path.hpp>
#include <tbb/parallel_for.h>
#include <tbb/parallel_for_each.h>

#include "nlohmann/json.hpp"

#include "libslic3r/Utils.hpp"
#include "libslic3r/Preset.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/AppConfig.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/3DBed.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/GLCanvas3D.hpp"

using namespace Slic3r;
using namespace GUI;


// NOTE: must encode as UTF-8, not via mb_str().
// mb_str() encodes using the current locale (GBK on Simplified Chinese Windows).
// Paths and vendor names in this file originate from from_u8()/UTF-8 std::strings and
// are fed back into boost::filesystem::path, which has a UTF-8 codecvt installed by
// boost::nowide::nowide_filesystem() at startup. Handing it locale-encoded bytes makes
// the path constructor throw "codecvt to wstring: error [codecvt:2]" for any non-ASCII
// path, which silently aborts loading a whole vendor's profile family.
static std::string w2s(wxString sSrc) { return std::string(sSrc.utf8_str()); }

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

ProfileFamilyLoader::ProfileFamilyLoader()
{
    request();
}

void ProfileFamilyLoader::request(bool force_reload)
{
    std::lock_guard<std::mutex> lock(m_state_mutex);
    if (m_first_frame_loading) {
        return;
    }
    if (m_first_frame_loaded && !force_reload)
        return;

    m_first_frame_loading = true;
    m_ret = std::async(std::launch::async, [this]()->int {
        auto ret = LoadProfile(m_profile_json, m_machine_json, m_load_curstom_from_bundle);
        std::lock_guard<std::mutex> lock(m_state_mutex);
        m_first_frame_loading = false;
        m_first_frame_loaded  = true;
        return ret;
    }).share();
}

void ProfileFamilyLoader::wait()
{
    std::shared_future<int> result;
    {
        std::lock_guard<std::mutex> lock(m_state_mutex);
        result = m_ret;
    }
    if (result.valid())
        result.get();
}

void ProfileFamilyLoader::request_and_wait()
{
    request(true);
    wait();
}

void ProfileFamilyLoader::wait_until_loaded()
{
    request(false);
    wait();
}

bool ProfileFamilyLoader::data_empty() 
{ 
    return m_machine_json.empty() || m_profile_json.empty();
}

// Vendor folder that owns machineList.json and the machine presets derived from it.
static std::string profile_vendor_name()
{
#ifdef CUSTOMIZED
    return std::string(SLIC3R_APP_KEY);
#else
    return std::string("Creality");
#endif
}

boost::filesystem::path ProfileFamilyLoader::machine_list_path()
{
    const boost::filesystem::path data_system_dir =
        (boost::filesystem::path(Slic3r::data_dir()) / PRESET_SYSTEM_DIR).make_preferred();
    const boost::filesystem::path resources_profiles_dir =
        (boost::filesystem::path(resources_dir()) / "profiles").make_preferred();

    const std::string vendor_name = profile_vendor_name();

    boost::filesystem::path machinepath = data_system_dir;
    if (!boost::filesystem::exists((data_system_dir / vendor_name / "machineList").replace_extension(".json"))) {
        machinepath = resources_profiles_dir;
    }
    return (machinepath / vendor_name / "machineList.json").make_preferred();
}

bool ProfileFamilyLoader::reload_machine_list()
{
    {
        std::lock_guard<std::mutex> lock(m_state_mutex);
        // A full load is in flight, or has not run yet. It reads machineList.json from
        // disk on its own, so patching the cached copy here would be pointless and
        // would race with the loader thread writing m_machine_json.
        if (m_first_frame_loading || !m_first_frame_loaded)
            return false;
    }

    const boost::filesystem::path file_path = machine_list_path();

    json output_machine       = json::object();
    output_machine["machine"] = json::array();
    std::map<std::string, std::string> thumbnails;

    if (LoadMachineJson(output_machine, thumbnails, file_path) != 0) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": failed to reload " << file_path.string()
                                 << ", keeping the previously loaded machine list";
        return false;
    }

    // An empty result means the file parsed but produced no usable series (for
    // example a truncated download). Keeping the old list is better than blanking
    // the add-printer navigation tree.
    if (output_machine["machine"].empty()) {
        BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << ": " << file_path.string()
                                   << " produced an empty machine list, keeping the previous one";
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_state_mutex);
        m_machine_json = std::move(output_machine);
        // Merge instead of replace: covers resolved during the full load may refer to
        // thumbnails that are no longer listed, and dropping them would break images.
        for (const auto& item : thumbnails)
            m_map_machine_thumbnail[item.first] = item.second;
    }

    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ": machine list reloaded from " << file_path.string();
    return true;
}

int ProfileFamilyLoader::LoadProfile(
    json& output_profile, 
    json& output_machine, 
    bool& bbl_bundle_rsrc)
{
    auto gen_profile_json = []() -> json {
        json ret;
        ret             = json::parse("{}");
        ret["filament"] = json::object();
        ret["model"]    = json::array();
        ret["machine"]  = json::object();
        ret["process"]  = json::array();
        return ret;
    };

    std::mutex profile_json_mutex;
    output_profile = gen_profile_json();
    m_creality_profile_json = gen_profile_json();
    if (!m_first_frame_loaded)
        m_resources_profile_json = gen_profile_json();

    try {
        auto merger_profile_data = [&profile_json_mutex, this](json& target, json& patch) {
            std::lock_guard<std::mutex> lk(profile_json_mutex);
            // model
            if (patch.contains("model")) {
                for (const auto& item : patch["model"]) {
                    target["model"].push_back(item);
                }
            }
            // machine
            if (patch.contains("machine")) {
                target["machine"].merge_patch(patch["machine"]);
            }
            // filament
            if (patch.contains("filament")) {
                target["filament"].merge_patch(patch["filament"]);
            }
            // process
            if (patch.contains("process")) {
                for (const auto& item : patch["process"]) {
                    target["process"].push_back(item);
                }
            }
        };

        boost::filesystem::path data_system_dir = 
            (boost::filesystem::path(Slic3r::data_dir()) / PRESET_SYSTEM_DIR).make_preferred();
        boost::filesystem::path resources_profiles_dir = 
            (boost::filesystem::path(resources_dir()) / "profiles").make_preferred();

        // load machine lists
        output_machine                      = json::parse("{}");
        output_machine["machine"]           = json::array();
        LoadMachineJson(
            output_machine,
            m_map_machine_thumbnail,
            machine_list_path());

        const std::string vendor_name = profile_vendor_name();

        // Backfill covers for already installed models, even when their parameter version is current.
        bool covers_changed = false;
        for (const auto& entry : m_map_machine_thumbnail) {
            if (!valid_printer_cover_component(entry.first))
                continue;
            const auto model_file = data_system_dir / vendor_name / "machine" / (entry.first + "_model.json");
            boost::system::error_code ec;
            if (boost::filesystem::is_regular_file(model_file, ec))
                covers_changed |= sync_printer_cover(vendor_name, entry.first, entry.second);
        }
        if (covers_changed) {
            wxGetApp().CallAfter([] {
                if (auto* plater = wxGetApp().plater()) {
                    if (auto* canvas = plater->get_current_canvas3D())
                        canvas->set_as_dirty();
                    plater->Refresh();
                }
            });
        }

        auto traversal_get_files = [](const fs::path& dir, std::vector<fs::path>& output) {
            boost::filesystem::directory_iterator endIter;
            for (boost::filesystem::directory_iterator iter(dir); iter != endIter; ++iter) {
                const fs::path& path = iter->path();
                if (!boost::filesystem::is_directory(path))
                    output.push_back(path);
            }
        };

        // load curstom json
        auto customdir  = data_system_dir;
        bbl_bundle_rsrc = false;
        if (!boost::filesystem::exists((data_system_dir / PresetBundle::BBL_BUNDLE).replace_extension(".json"))) {
            customdir       = resources_profiles_dir;
            bbl_bundle_rsrc = true;
        }
        std::vector<fs::path> paths;
        traversal_get_files(customdir, paths);
        for (const fs::path& path : paths)
        {
            const std::string vendor = path.stem().string();
            const std::string extension = path.extension().string();
            if (boost::iequals(extension, ".json") && vendor == PresetBundle::BBL_BUNDLE) {
                json output = gen_profile_json();
                LoadProfileFamily(vendor, path, output);
                merger_profile_data(output_profile, output);
            }
        }
        
        // load all profile json
        std::vector<fs::path> resources_profiles_files;
        traversal_get_files(resources_profiles_dir, resources_profiles_files);
        BOOST_LOG_TRIVIAL(info) << "ProfileScan: resources profiles dir=" << resources_profiles_dir.string()
                                << " exists=" << boost::filesystem::exists(resources_profiles_dir)
                                << " files found=" << resources_profiles_files.size();
        for (const fs::path& path : resources_profiles_files)
        {
            const std::string vendor = path.stem().string();
            const std::string extension = path.extension().string();
            if (!boost::iequals(extension, ".json") || vendor == PresetBundle::BBL_BUNDLE)
                continue;

            auto target_path = path;
            bool load        = false;
            bool is_creality = false;
            if (vendor == "Creality") {
                boost::filesystem::path user_vendor_path = (data_system_dir / "Creality.json").make_preferred();
                if (boost::filesystem::exists(user_vendor_path))
                    target_path = user_vendor_path;
                is_creality = true;
                load = true;
            } else if (!m_first_frame_loaded) {
                load = true;
            }

            if (load) {
                json output = gen_profile_json();
                BOOST_LOG_TRIVIAL(info) << "ProfileScan: loading vendor '" << vendor
                                        << "' from " << target_path.string();
                LoadProfileFamily(vendor, target_path, output);
                merger_profile_data(is_creality ? m_creality_profile_json : m_resources_profile_json, output);
                BOOST_LOG_TRIVIAL(info) << "ProfileScan: vendor '" << vendor << "' done";
            }
        }
        merger_profile_data(output_profile, m_creality_profile_json);
        merger_profile_data(output_profile, m_resources_profile_json);

    } catch (std::exception& e) {
        // This handler used to be completely empty. Any failure while scanning the
        // vendor profile directories silently dropped whole vendors (notably
        // Creality), which surfaced much later as "add printer failed" with nothing
        // in the log to explain it.
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": profile scan aborted by exception: " << e.what();
    }
    return 0;
}

int ProfileFamilyLoader::LoadMachineJson(
    json& output_machine,
    std::map<std::string, std::string>& mapMachineThumbnail,
    const boost::filesystem::path& file_path)
{
    try {
        std::string contents;
        if (!LoadFile(file_path, contents))
            return -1;
        json jLocal = json::parse(contents);

        json pmodels = jLocal["printerList"];
        json series  = jLocal["series"];

        std::map<std::string,std::vector<PrinterInfo>> mapPrinters;  //key = 品牌  Creality SparkX

         //wxString strJS = wxString::Format("handleStudioCmd(%s)", series.dump(-1, ' ', true));
        for (const auto& item : series) {
            int         id   = item["id"];
            std::string name = item["name"];
            std::string brandName = "";
            if (item.contains("brandName") && item["brandName"].is_string()) {
                brandName = item["brandName"];
            }else{
                brandName = "Creality";
            }

            if (name.empty() || brandName.empty())
                continue;

            PrinterInfo printerInfo;
            printerInfo.name = name;

            for (const auto& printer : pmodels) {
                int seriesId = printer["seriesId"];
                if (seriesId == id) {
                    std::string str1 = printer["name"];
                    if (str1.find("Creality") == std::string::npos) {
                        if ((str1.find("SPARKX") == std::string::npos))
                        {
                            str1 = "Creality " + str1;
                        }
                    }
                    std::string str2 = printer["printerIntName"];
                    printerInfo.seriesNameList += (str1 + ";" + str2 + ";");
                    std::string printerName = printer["name"];
                    if (printerName.find("Creality") == std::string::npos) {
                        if ((printerName.find("SPARKX") == std::string::npos))
                        {
                            printerName = "Creality " + printerName;
                        }
                        
                    }
                    mapMachineThumbnail[printerName] = printer["thumbnail"];
                }
            }

            if (printerInfo.name.empty() || printerInfo.seriesNameList.empty()) 
            {
                continue;
            }

            mapPrinters[brandName].push_back(printerInfo);
        }

        //std::sort(printers.begin(), printers.end(), customComparator);
        auto it = mapPrinters.find("Creality");
        if (it != mapPrinters.end()) {
            std::sort(it->second.begin(), it->second.end(), customComparator);
        }

        for (const auto& [brandName, printerList] : mapPrinters) 
        {
            for (const auto& info : printerList) 
            {
                json childList = json::object();
                std::string fullName  = "";
                if (brandName.find("Creality") == std::string::npos) {
                    fullName = brandName + "|" + info.name;  // 其他品牌 如:sparkX 
                } else {
                    fullName = info.name;
                }
                fullName = trim(fullName);
                wxString sName = _L(fullName);
                childList["name"] = sName.utf8_str();
                childList["printers"] = info.seriesNameList;
                output_machine["machine"].push_back(childList);
            }
        }

    } catch (nlohmann::detail::parse_error& err) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": parse " << file_path.string()
                                 << " got a nlohmann::detail::parse_error, reason = " << err.what();
        return -1;
    } catch (std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": parse " << file_path.string() << " got exception: " << e.what();
        return -1;
    }

    return 0;
}

int ProfileFamilyLoader::LoadProfileFamily(
    const std::string& vendor,
    const boost::filesystem::path& file_path,
    json& outputJson)
{
    boost::filesystem::path vendor_dir = boost::filesystem::absolute(file_path.parent_path() / vendor).make_preferred();
    // judge if user has copy vendor dir to data dir
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(",  vendor path %1%.") % vendor_dir.string();
    try {
        std::string contents;
        if (!LoadFile(file_path, contents))
            return -1;

        json jLocal = json::parse(contents);

        // BBS:Machine
        std::map<string, AreaInfo> mapInfo;
        std::mutex                 mapInfoMutex;
        std::mutex                 outputJsonMutex;
        json                       pmachine = jLocal["machine_list"];
        int                        nsize    = pmachine.size();
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(",  got %1% machines") % nsize;
        tbb::parallel_for(
                        0, nsize,
                        [this, &pmachine, vendor_dir, &mapInfo, &outputJson, &mapInfoMutex,
                        &outputJsonMutex](int n) {
                                json OneMachine = pmachine.at(n);

                                std::string s1 = OneMachine["name"];
                                std::string s2 = OneMachine["sub_path"];

                                const boost::filesystem::path sub_path = boost::filesystem::absolute(vendor_dir / s2).make_preferred();
                                std::string contents;
                                if (!boost::filesystem::exists(sub_path) || !LoadFile(sub_path, contents))
                                    return;
                                try {
                                    json pm = json::parse(contents);

                                    std::map<string, AreaInfo> mapInfoTmp;
                                    GetPrinterArea(pm, mapInfoTmp);
                                    {
                                        std::lock_guard lk(mapInfoMutex);
                                        mapInfo.merge(mapInfoTmp);
                                    }

                                    std::string strInstant = pm["instantiation"];
                                    if (strInstant.compare("true") == 0) {
                                        OneMachine["model"]  = pm["printer_model"];
                                        OneMachine["nozzle"] = pm["nozzle_diameter"][0];

                                        // GetPrinterArea(pm, vecAre);
                                        {
                                            std::lock_guard lk(outputJsonMutex);
                                            outputJson["machine"][s1] = OneMachine;
                                        }
                                    }
                                } catch (nlohmann::detail::parse_error& err) {
                                } catch (std::exception& e) {}
                            }
                        );
        // BBS:models
        json pmodels = jLocal["machine_model_list"];
        nsize   = pmodels.size();

        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(",  got %1% machine models") % nsize;
        tbb::parallel_for(
            0, nsize,
            [this, &pmodels, vendor_dir, &outputJsonMutex,
            vendor, &mapInfo, &outputJson](int n) {
                json OneModel = pmodels.at(n);

                OneModel["model"] = OneModel["name"];
                OneModel.erase("name");

                std::string             s1       = OneModel["model"];
                std::string             s2       = OneModel["sub_path"];
                const boost::filesystem::path sub_path = boost::filesystem::absolute(vendor_dir / s2).make_preferred();
                std::string contents;
                if (!boost::filesystem::exists(sub_path) || !LoadFile(sub_path, contents))
                    return;
                json pm;
                try {
                    pm = json::parse(contents);
                } catch (nlohmann::detail::parse_error& err) {
                    BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << "Load machine_model_list error::" << sub_path.string() << std::endl;
                    return;
                }
                OneModel["vendor"]    = vendor;
                std::string NozzleOpt = pm["nozzle_diameter"];
                StringReplace(NozzleOpt, " ", "");
                OneModel["nozzle_diameter"] = NozzleOpt;
                OneModel["materials"]       = pm["default_materials"];

                boost::filesystem::path cover_path = find_printer_cover(data_dir(), resources_dir(), vendor, s1);
                if (cover_path.empty())
                    cover_path = boost::filesystem::path(resources_dir()) / "images" / "printer_default.png";
                std::string url = cover_path.string();
                std::regex  pattern("\\\\");
                std::string replacement = "/";
                std::string output      = std::regex_replace(url, pattern, replacement);
                std::regex  pattern2("#");
                std::string replacement2 = "%23";
                output                   = std::regex_replace(output, pattern2, replacement2);
                OneModel["cover"]        = output;

                OneModel["nozzle_selected"] = "";

                for (const auto& pair : mapInfo) {
                    // Special-case K1 Max: always use 0.4 mm nozzle area info
                    if ("K1 Max" == s1) {
                        if ((pair.second.strModelName == s1) && (pair.first.find("0.4") != string::npos)) {
                            OneModel["area"] = pair.second.strAreaInfo + "*" + pair.second.strHeightInfo;
                            break;
                        }
                    } else {
                        if (pair.second.strModelName == s1) {
                            OneModel["area"] = pair.second.strAreaInfo + "*" + pair.second.strHeightInfo;
                            break;
                        }
                    }
                }
                {
                    std::lock_guard lk(outputJsonMutex);
                    outputJson["model"].push_back(OneModel);
                }
        });
        // BBS:Filament
        json pFilament = jLocal["filament_list"];
        json tFilaList = json::object();
        nsize     = pFilament.size();

        for (int n = 0; n < nsize; n++) {
            json OneFF = pFilament.at(n);

            std::string s1 = OneFF["name"];
            std::string s2 = OneFF["sub_path"];

            tFilaList[s1] = OneFF;
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "Vendor: " << vendor << ", tFilaList Add: " << s1;
        }

        int nFalse  = 0;
        int nModel  = 0;
        int nFinish = 0;
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(",  got %1% filaments") % nsize;
        tbb::parallel_for(
            0, nsize, 
            [this, &pFilament, &outputJson, 
            &outputJsonMutex, vendor_dir,&tFilaList](int n) 
            {
                json OneFF = pFilament.at(n);

                std::string s1 = OneFF["name"];
                std::string s2 = OneFF["sub_path"];

                outputJsonMutex.lock();
                auto elem_exists = outputJson["filament"].contains(s1);
                outputJsonMutex.unlock();
                if (!elem_exists) {
                    const boost::filesystem::path sub_path = boost::filesystem::absolute(vendor_dir / s2).make_preferred();
                    std::string contents;
                    if (!boost::filesystem::exists(sub_path) || !LoadFile(sub_path, contents))
                        return;
                    json pm;
                    try {
                        pm = json::parse(contents);
                    } catch (nlohmann::detail::parse_error& err) {
                        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << "Load Filament error::" << sub_path.string() << ",reason:" << err.what() << std::endl;
                        return;
                    }
                    std::string strInstant = pm["instantiation"];
                    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "Load Filament:" << s1 << ",Path:" << sub_path.string() << ",instantiation?"
                                            << strInstant;

                    if (strInstant == "true") {
                        std::string sV;
                        std::string sT;

                        int nRet = GetFilamentInfo(vendor_dir, tFilaList, sub_path, sV, sT);
                        if (nRet != 0) {
                            BOOST_LOG_TRIVIAL(info)
                                << __FUNCTION__ << "Load Filament:" << s1 << ",GetFilamentInfo Failed, Vendor:" << sV << ",Type:" << sT;
                            return;
                        }

                        OneFF["vendor"] = sV;
                        OneFF["type"]   = sT;

                        OneFF["models"] = "";

                        json        pPrinters = pm["compatible_printers"];
                        int         nPrinter  = pPrinters.size();
                        std::string ModelList = "";
                        for (int i = 0; i < nPrinter; i++) {
                            std::string sP = pPrinters.at(i);
                            if (outputJson["machine"].contains(sP)) {
                                std::string mModel   = outputJson["machine"][sP]["model"];
                                std::string mNozzle  = outputJson["machine"][sP]["nozzle"];
                                std::string NewModel = mModel + "++" + mNozzle;

                                ModelList = (boost::format("%1%[%2%]") % ModelList % NewModel).str();
                            }
                        }

                        OneFF["models"]   = ModelList;
                        OneFF["selected"] = 0;

                        {
                            std::lock_guard lk(outputJsonMutex);
                            outputJson["filament"][s1] = OneFF;
                        }
                    }
                }
            
        });
        // process
        json pProcess = jLocal["process_list"];
        nsize    = pProcess.size();
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(",  got %1% processes") % nsize;
        tbb::parallel_for(
            0, nsize, 
            [this, &pProcess, vendor_dir, &outputJson, 
            &outputJsonMutex](int n) 
            {
                json OneProcess = pProcess.at(n);

                std::string             s2       = OneProcess["sub_path"];
                const boost::filesystem::path sub_path = boost::filesystem::absolute(vendor_dir / s2).make_preferred();
                std::string contents;
                if (!boost::filesystem::exists(sub_path) || !LoadFile(sub_path, contents))
                    return;
                json pm;
                try {
                    pm = json::parse(contents);
                } catch (nlohmann::detail::parse_error& err) {
                    BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << "Load process error::" << sub_path.string() << std::endl;
                    return;
                }

                std::string bInstall = pm["instantiation"];
                if (bInstall == "true") {
                    std::lock_guard lk(outputJsonMutex);
                    outputJson["process"].push_back(OneProcess);
                }
            
        });
    } catch (nlohmann::detail::parse_error& err) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": parse " << file_path.string()
                                 << " got a nlohmann::detail::parse_error, reason = " << err.what();
        return -1;
    } catch (std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": parse " << file_path.string() << " got exception: " << e.what();
        return -1;
    }

    return 0;
}

bool ProfileFamilyLoader::LoadFile(const boost::filesystem::path& path, std::string& content)
{
    content.clear();
    try {
        // boost::nowide::ifstream expects an UTF-8 narrow path on Windows. Keep the
        // path native until this I/O boundary so no local-code-page conversion can
        // consume a path separator (for example UTF-8 "新建文件夹\\" under CP936).
        const std::string utf8_path = path.string();
        boost::nowide::ifstream input(utf8_path);
        if (!input.is_open()) {
            BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": failed to open " << utf8_path;
            return false;
        }

        std::stringstream buffer;
        buffer << input.rdbuf();
        if (input.bad()) {
            BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": failed while reading " << utf8_path;
            return false;
        }

        content = buffer.str();
        if (content.empty()) {
            BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": file is empty " << utf8_path;
            return false;
        }

        BOOST_LOG_TRIVIAL(trace) << __FUNCTION__ << boost::format(", load %1% into buffer") % utf8_path;
        return true;
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": exception for " << path.string() << ": " << e.what();
        return false;
    }
}

int ProfileFamilyLoader::GetFilamentInfo(
    const boost::filesystem::path& vendor_directory,
    json& pFilaList,
    const boost::filesystem::path& file_path,
    std::string& sVendor,
    std::string& sType)
{
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " GetFilamentInfo:VendorDirectory - " << vendor_directory.string()
                            << ", Filepath - " << file_path.string();

    try {
        std::string contents;
        if (!LoadFile(file_path, contents))
            return -1;
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ": Json Contents: " << contents;
        json jLocal = json::parse(contents);

        if (sVendor.empty()) {
            if (jLocal.contains("filament_vendor"))
                sVendor = jLocal["filament_vendor"][0];
            else
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << file_path.string() << " - Not Contains filament_vendor";
        }

        if (sType.empty()) {
            if (jLocal.contains("filament_type"))
                sType = jLocal["filament_type"][0];
            else
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << file_path.string() << " - Not Contains filament_type";
        }

        if (sVendor.empty() || sType.empty()) {
            if (jLocal.contains("inherits")) {
                const std::string inherited_name = jLocal["inherits"];
                if (!pFilaList.contains(inherited_name)) {
                    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "pFilaList - Not Contains inherits filaments: " << inherited_name;
                    return -1;
                }

                const std::string relative_path = pFilaList[inherited_name]["sub_path"];
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " Before Format Inherits Path: VendorDirectory - "
                                        << vendor_directory.string() << ", sub_path - " << relative_path;
                const boost::filesystem::path inherits_path =
                    (vendor_directory / boost::filesystem::path(relative_path)).make_preferred();

                if (boost::filesystem::exists(inherits_path))
                    return GetFilamentInfo(vendor_directory, pFilaList, inherits_path, sVendor, sType);

                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " inherits File Not Exist: " << inherits_path.string();
                return -1;
            }

            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << file_path.string() << " - Not Contains inherits";
            if (sType.empty()) {
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "sType is Empty";
                return -1;
            }
            sVendor = "Generic";
        }

        return 0;
    } catch (const nlohmann::detail::parse_error& err) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": parse " << file_path.string()
                                 << " got a nlohmann::detail::parse_error, reason = " << err.what();
        return -1;
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": parse " << file_path.string() << " got exception: " << e.what();
        return -1;
    }
}
