#include "ProfileFamilyLoader.hpp"

#include <thread>
#include <condition_variable>

#include <wx/wx.h>
#include <boost/filesystem/path.hpp>
#include <tbb/parallel_for_each.h>

#include "nlohmann/json.hpp"

#include "libslic3r/Utils.hpp"
#include "libslic3r/Preset.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/AppConfig.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/3DBed.hpp"

using namespace Slic3r;
using namespace GUI;


class ThreadPool
{
public:
    class Worker
    {
    public:
        Worker(ThreadPool* p, bool runonce = true) 
            : parent(p)
            , runonce_after_create(runonce)
        {
            m_ret = std::async([this]() { run(); });
        }

        void stop_and_wait() 
        { 
            stop_signal();  
            wait_finished();
        }
        void stop_signal() { m_stop.store(true); }
        void wait_finished() { m_ret.get(); }
        bool idle() { return m_idle.load(); }

    private:
        void run()
        {
            {
                std::unique_lock lk(parent->active_mutex);
                while (1) {
                    if (!runonce_after_create)
                        parent->active_signal.wait(lk);
                    runonce_after_create = false;
                    if (m_stop.load())
                        return;
                    if (parent->m_target_active_count.load() != parent->m_cur_active_count.load()) {
                        parent->m_cur_active_count.fetch_add(1);
                        break;
                    }
                }
            }

            while (!m_stop.load()) 
            {
                m_idle.store(true);        
                auto f = parent->get_task();
                m_idle.store(false);
                f();
            }
        }

        bool                   runonce_after_create = false;
        std::future<void>      m_ret;
        std::atomic_bool m_stop  = false;
        std::atomic_bool m_idle = true;
        std::condition_variable actived_signal;
        std::mutex actived_mutex;
        ThreadPool*      parent = nullptr;
    };
    struct WorkerQueue
    {
        void wait_all_idle()
        {
            for (auto work : m_works)
            {
                while (!work->idle()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
        }
        void terminate_all()
        {
            for (auto work : m_works) 
            {
                work->stop_signal();
            }
        }
        void wait_all()
        {
            for (auto work : m_works) {
                work->wait_finished();
            }
        }

        std::list<Worker*> m_works;
    };

    struct TaskQueue
    {
        using Func_Type = std::function<void()>;
        Func_Type pop()
        {
            std::unique_lock lk(data_mutex);
            if (m_tasks.empty()) {
                empty_signal.notify_all();

                non_empty_signal.wait(lk);
                if (m_tasks.empty()) {
                    return [](){}; // empty function
                }
            }

            auto t = m_tasks.front();
            m_tasks.pop_front();
            return t;
        }

        void push(std::list<Func_Type>& block)
        {
            std::unique_lock lk(data_mutex);
            m_tasks = block;
            non_empty_signal.notify_all();
        }
        
        void wait_empty()
        {
            std::unique_lock lk(data_mutex);
            empty_signal.wait(lk, [&]() { return m_tasks.empty(); });
        }

        std::list<Func_Type> m_tasks;

        std::condition_variable empty_signal;
        std::condition_variable non_empty_signal;
        std::mutex data_mutex;
    };

public:
    ThreadPool()
    {
        auto hc = std::thread::hardware_concurrency();
        m_max_count = hc == 0 ? 4 : hc>4? 4:hc; // maybe 4 thread enough
        min_fire_power();
        for (auto i = 0; i < m_max_count; ++i) 
        {
            m_work_queue.m_works.push_back(new Worker(this));
        }
    }
    ~ThreadPool()
    {
        m_work_queue.terminate_all();
        active_signal.notify_all();
        m_task_queue.non_empty_signal.notify_all();
        m_work_queue.wait_all();

        for (auto work : m_work_queue.m_works)
        {
            delete work;
        }
    }

    template<typename Vec, typename Func> 
    void parallel_for(Vec&& vec, Func&& f)
    { 
        if (vec.size() == 0)
            return;
        add_tasks(std::forward<Vec>(vec), std::forward<Func>(f));
        wait_cur_tasks_finished();
    }

    template<typename Func> 
    void parallel_for(int i1, int i2, Func&& f)
    {
        std::vector<int> v;
        for (auto i = i1; i != i2; ++i)
            v.push_back(i);
        parallel_for(v, std::forward<Func>(f));
    }
    
    void max_fire_power()
    { change_active_worker_count(m_max_count); }

private:
    void min_fire_power() { change_active_worker_count(1); }

    TaskQueue::Func_Type get_task() { return m_task_queue.pop(); }

    template<typename Vec, typename Func> 
    void add_tasks(Vec&& vec, Func&& f)
    {
        std::list<TaskQueue::Func_Type> block;
        for (auto it = vec.begin(); it != vec.end(); ++it) {
            block.push_back(std::bind(f, *it));
        }
        m_task_queue.push(block);
    }

    void wait_cur_tasks_finished()
    {
        m_task_queue.wait_empty();
        m_work_queue.wait_all_idle();
    }
    void change_active_worker_count(int c)
    {
        c = c > m_max_count ? m_max_count : c;
        if (m_target_active_count.load() == c)
            return;

        m_target_active_count.store(c);
        active_signal.notify_all();
    }

    int              m_max_count;
    std::atomic_char m_target_active_count = 0;
    std::atomic_char m_cur_active_count = 0;
    std::condition_variable active_signal;
    std::mutex active_mutex;

    WorkerQueue m_work_queue;
    friend WorkerQueue;
    TaskQueue   m_task_queue;
    friend TaskQueue;
};

static std::string w2s(wxString sSrc) { return std::string(sSrc.mb_str()); }

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

static struct AreaInfo
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

static struct PrinterInfo
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
                return i; // 返回优先级索引
            }
        }
        return order.size(); // 如果都不包含，返回最低优先级
    };

    return getPriority(a.name) < getPriority(b.name);
}

ProfileFamilyLoader::ProfileFamilyLoader()
{
    tp = new ThreadPool;
    request();
    m_first_frame_loading = true;
}

void ProfileFamilyLoader::request()
{
    if (m_first_frame_loading)
    {
        tp->max_fire_power();
        return;
    }
    if (m_first_frame_loaded)
        tp->max_fire_power();

    m_ret = std::async([this]()->int { 
        auto ret = LoadProfile(m_profile_json, m_machine_json, m_load_curstom_from_bundle); 
        m_first_frame_loading = false;
        m_first_frame_loaded  = true;
        return ret;
    });
}

void ProfileFamilyLoader::wait() { m_ret.get(); }

void ProfileFamilyLoader::request_and_wait()
{
    request();
    wait();
}

bool ProfileFamilyLoader::data_empty() 
{ 
    return m_machine_json.empty() || m_profile_json.empty();
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
        boost::filesystem::path machinepath = data_system_dir;
#ifdef CUSTOMIZED
        std::string vendor_name = std::string(SLIC3R_APP_KEY);
#else
        std::string vendor_name = "Creality";
#endif
        if (!boost::filesystem::exists((data_system_dir / vendor_name / "machineList").replace_extension(".json"))) {
            machinepath = resources_profiles_dir;
        }
        LoadMachineJson(
            output_machine, 
            m_map_machine_thumbnail, 
            (machinepath / vendor_name / "machineList.json").string());

        auto traversal_get_files = [](std::string dir, std::vector<fs::path>& output) {
            boost::filesystem::directory_iterator endIter;
            for (boost::filesystem::directory_iterator iter(dir); iter != endIter; iter++) {
                auto path = iter->path();
                if (boost::filesystem::is_directory(path))
                    continue;
                output.push_back(path);
            }
        };
        auto parser_ext_vendor_by_path = [](std::string path, wxString& vendor, wxString& ext) {
            vendor                = from_u8(path).BeforeLast('.');
            vendor                = vendor.AfterLast('\\');
            vendor                = vendor.AfterLast('\/');
            ext                   = from_u8(path).AfterLast('.').Lower();
        };

        // load curstom json
        auto customdir  = data_system_dir;
        bbl_bundle_rsrc = false;
        if (!boost::filesystem::exists((data_system_dir / PresetBundle::BBL_BUNDLE).replace_extension(".json"))) {
            customdir       = resources_profiles_dir;
            bbl_bundle_rsrc = true;
        }
        std::vector<fs::path> paths;
        traversal_get_files(customdir.string(), paths);
        for (auto path : paths)
        {
            auto f = [this, &output_profile, parser_ext_vendor_by_path, gen_profile_json,
                      merger_profile_data](const boost::filesystem::path& path) {
                wxString strVendor;
                wxString strExtension;
                parser_ext_vendor_by_path(path.string(), strVendor, strExtension);
                if (strExtension.CmpNoCase("json") == 0) {
                    if (w2s(strVendor) == PresetBundle::BBL_BUNDLE) {
                        json output = gen_profile_json();
                        LoadProfileFamily(w2s(strVendor), path.string(), output);
                        merger_profile_data(output_profile, output);
                    }
                }
            };
            f(path);
        }
        
        // load all profile json
        std::vector<fs::path> resources_profiles_files;
        traversal_get_files(resources_profiles_dir.string(), resources_profiles_files);
        for (auto path : resources_profiles_files)
        {
            auto f = [this, data_system_dir, parser_ext_vendor_by_path, gen_profile_json,
                      merger_profile_data](const boost::filesystem::path& path) {
                wxString strVendor;
                wxString strExtension;
                parser_ext_vendor_by_path(path.string(), strVendor, strExtension);

                if (strExtension.CmpNoCase("json") == 0) {
                    if (w2s(strVendor) != PresetBundle::BBL_BUNDLE) {
                        auto target_path = path;
                        bool load        = false;
                        bool is_creality = false;
                        if (strVendor == "Creality") {
                            boost::filesystem::path user_vendor_path = (data_system_dir / "Creality.json").make_preferred();
                            if (boost::filesystem::exists(user_vendor_path)) {
                                target_path = user_vendor_path;
                            }
                            is_creality = true;
                            load = true;
                        }
                        else
                        {
                            if (!m_first_frame_loaded)
                                load = true;
                        }
                        if (load) {
                            json output = gen_profile_json();
                            LoadProfileFamily(w2s(strVendor), target_path.string(), output);
                            merger_profile_data(is_creality ? m_creality_profile_json : m_resources_profile_json, output);
                        }
                    }
                }
            };
            f(path);
        }
        merger_profile_data(output_profile, m_creality_profile_json);
        merger_profile_data(output_profile, m_resources_profile_json);

    } catch (std::exception& e) {
    }
    return 0;
}

int ProfileFamilyLoader::LoadMachineJson(
    json& output_machine, 
    std::map<std::string, std::string> &mapMachineThumbnail,
    std::string strFilePath)
{
    boost::filesystem::path file_path(strFilePath);
    try {
        std::string contents;
        LoadFile(strFilePath, contents);
        json jLocal = json::parse(contents);

        json pmodels = jLocal["printerList"];
        json series  = jLocal["series"];

        std::vector<PrinterInfo> printers;

        for (const auto& item : series) {
            int         id   = item["id"];
            std::string name = item["name"];

            if (name.empty())
                continue;

            PrinterInfo printerInfo;
            printerInfo.name = name;

            for (const auto& printer : pmodels) {
                int seriesId = printer["seriesId"];
                if (seriesId == id) {
                    std::string str1 = printer["name"];
                    if (str1.find("Creality") == std::string::npos) {
                        str1 = "Creality " + str1;
                    }
                    std::string str2 = printer["printerIntName"];
                    printerInfo.seriesNameList += (str1 + ";" + str2 + ";");
                    std::string printerName = printer["name"];
                    if (printerName.find("Creality") == std::string::npos) {
                        printerName = "Creality " + printerName;
                    }
                    mapMachineThumbnail[printerName] = printer["thumbnail"];
                }
            }

            printers.push_back(printerInfo);
        }

        std::sort(printers.begin(), printers.end(), customComparator);

        for (const auto& info : printers) {
            json childList = json::object();

            std::string name  = info.name;
            wxString    sName = _L(name);
            childList["name"] = sName.utf8_str();

            childList["printers"] = info.seriesNameList;
            output_machine["machine"].push_back(childList);
        }
    } catch (nlohmann::detail::parse_error& err) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": parse " << strFilePath
                                 << " got a nlohmann::detail::parse_error, reason = " << err.what();
        return -1;
    } catch (std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": parse " << strFilePath << " got exception: " << e.what();
        return -1;
    }

    return 0;
}

int ProfileFamilyLoader::LoadProfileFamily(
    std::string strVendor, 
    std::string strFilePath,
    json& outputJson)
{
    boost::filesystem::path file_path(strFilePath);
    boost::filesystem::path vendor_dir = boost::filesystem::absolute(file_path.parent_path() / strVendor).make_preferred();
    // judge if user has copy vendor dir to data dir
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(",  vendor path %1%.") % vendor_dir.string();
    try {
        std::string contents;
        LoadFile(strFilePath, contents);

        json jLocal = json::parse(contents);

        // BBS:Machine
        std::map<string, AreaInfo> mapInfo;
        std::mutex                 mapInfoMutex;
        std::mutex                 outputJsonMutex;
        json                       pmachine = jLocal["machine_list"];
        int                        nsize    = pmachine.size();
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(",  got %1% machines") % nsize;
        tp->parallel_for(
                        0, nsize,
                        [this, &pmachine, vendor_dir, &mapInfo, &outputJson, &mapInfoMutex,
                        &outputJsonMutex](int n) {
                                json OneMachine = pmachine.at(n);

                                std::string s1 = OneMachine["name"];
                                std::string s2 = OneMachine["sub_path"];

                                boost::filesystem::path sub_path = boost::filesystem::absolute(vendor_dir / s2).make_preferred();
                                std::string sub_file = sub_path.string();
                                std::string contents;
                                if (!boost::filesystem::exists(sub_file))
                                    return;
                                LoadFile(sub_file, contents);
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
        tp->parallel_for(
            0, nsize,
            [this, &pmodels, vendor_dir, &outputJsonMutex,
            strVendor, &mapInfo, &outputJson](int n) {
                json OneModel = pmodels.at(n);

                OneModel["model"] = OneModel["name"];
                OneModel.erase("name");

                std::string             s1       = OneModel["model"];
                std::string             s2       = OneModel["sub_path"];
                boost::filesystem::path sub_path = boost::filesystem::absolute(vendor_dir / s2).make_preferred();
                std::string sub_file = sub_path.string();
                std::string contents;
                if (!boost::filesystem::exists(sub_file))
                    return;
                LoadFile(sub_file, contents);
                json pm;
                try {
                    pm = json::parse(contents);
                } catch (nlohmann::detail::parse_error& err) {
                    BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << "Load machine_model_list error::" << sub_file << std::endl;
                    return;
                }
                OneModel["vendor"]    = strVendor;
                std::string NozzleOpt = pm["nozzle_diameter"];
                StringReplace(NozzleOpt, " ", "");
                OneModel["nozzle_diameter"] = NozzleOpt;
                OneModel["materials"]       = pm["default_materials"];

                std::string             cover_file = s1 + "_cover.png";
                boost::filesystem::path cover_path = boost::filesystem::absolute(boost::filesystem::path(resources_dir()) / "/profiles/" /
                                                                                    strVendor / cover_file).make_preferred();
                if (!boost::filesystem::exists(cover_path)) {
                    m_map_machine_thumbnail.count(s1) > 0 ?
                        cover_path = m_map_machine_thumbnail[s1] :
                        cover_path = (boost::filesystem::absolute(boost::filesystem::path(resources_dir()) / "/web/image/printer/") /
                                        cover_file).make_preferred();
                }
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
                    // k1 max 特殊处理，显示0.4喷嘴的打印区域
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
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "Vendor: " << strVendor << ", tFilaList Add: " << s1;
        }

        int nFalse  = 0;
        int nModel  = 0;
        int nFinish = 0;
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(",  got %1% filaments") % nsize;
        tp->parallel_for(
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
                    boost::filesystem::path sub_path = boost::filesystem::absolute(vendor_dir / s2).make_preferred();
                    std::string sub_file = sub_path.string();
                    std::string contents;
                    if (!boost::filesystem::exists(sub_file))
                        return;
                    LoadFile(sub_file, contents);
                    json pm;
                    try {
                        pm = json::parse(contents);
                    } catch (nlohmann::detail::parse_error& err) {
                        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << "Load Filament error::" << sub_file << ",reason:" << err.what() << std::endl;
                        return;
                    }
                    std::string strInstant = pm["instantiation"];
                    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "Load Filament:" << s1 << ",Path:" << sub_file << ",instantiation?"
                                            << strInstant;

                    if (strInstant == "true") {
                        std::string sV;
                        std::string sT;

                        int nRet = GetFilamentInfo(vendor_dir.string(), tFilaList, sub_file, sV, sT);
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
        tp->parallel_for(
            0, nsize, 
            [this, &pProcess, vendor_dir, &outputJson, 
            &outputJsonMutex](int n) 
            {
                json OneProcess = pProcess.at(n);

                std::string             s2       = OneProcess["sub_path"];
                boost::filesystem::path sub_path = boost::filesystem::absolute(vendor_dir / s2).make_preferred();
                std::string sub_file = sub_path.string();
                std::string contents;
                if (!boost::filesystem::exists(sub_file))
                    return;
                LoadFile(sub_file, contents);
                json pm;
                try {
                    pm = json::parse(contents);
                } catch (nlohmann::detail::parse_error& err) {
                    BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << "Load process error::" << sub_file << std::endl;
                    return;
                }

                std::string bInstall = pm["instantiation"];
                if (bInstall == "true") {
                    std::lock_guard lk(outputJsonMutex);
                    outputJson["process"].push_back(OneProcess);
                }
            
        });
    } catch (nlohmann::detail::parse_error& err) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": parse " << strFilePath
                                 << " got a nlohmann::detail::parse_error, reason = " << err.what();
        return -1;
    } catch (std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": parse " << strFilePath << " got exception: " << e.what();
        return -1;
    }

    return 0;
}

bool ProfileFamilyLoader::LoadFile(std::string jPath, std::string& sContent)
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

int ProfileFamilyLoader::GetFilamentInfo(
    std::string VendorDirectory, json& pFilaList, std::string filepath, std::string& sVendor, std::string& sType)
{
    // GetStardardFilePath(filepath);
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " GetFilamentInfo:VendorDirectory - " << VendorDirectory << ", Filepath - " << filepath;

    try {
        std::string contents;
        LoadFile(filepath, contents);
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ": Json Contents: " << contents;
        json jLocal = json::parse(contents);

        if (sVendor == "") {
            if (jLocal.contains("filament_vendor"))
                sVendor = jLocal["filament_vendor"][0];
            else {
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << filepath << " - Not Contains filament_vendor";
            }
        }

        if (sType == "") {
            if (jLocal.contains("filament_type"))
                sType = jLocal["filament_type"][0];
            else {
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << filepath << " - Not Contains filament_type";
            }
        }

        if (sVendor == "" || sType == "") {
            if (jLocal.contains("inherits")) {
                std::string FName = jLocal["inherits"];

                if (!pFilaList.contains(FName)) {
                    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "pFilaList - Not Contains inherits filaments: " << FName;
                    return -1;
                }

                std::string FPath = pFilaList[FName]["sub_path"];
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " Before Format Inherits Path: VendorDirectory - " << VendorDirectory
                                        << ", sub_path - " << FPath;
                wxString                strNewFile = wxString::Format("%s%c%s", wxString(VendorDirectory.c_str(), wxConvUTF8),
                                                                      boost::filesystem::path::preferred_separator, FPath);
                boost::filesystem::path inherits_path(w2s(strNewFile));

                // boost::filesystem::path nf(strNewFile.c_str());
                if (boost::filesystem::exists(inherits_path))
                    return GetFilamentInfo(VendorDirectory, pFilaList, inherits_path.string(), sVendor, sType);
                else {
                    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " inherits File Not Exist: " << inherits_path;
                    return -1;
                }
            } else {
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << filepath << " - Not Contains inherits";
                if (sType == "") {
                    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "sType is Empty";
                    return -1;
                } else
                    sVendor = "Generic";
                return 0;
            }
        } else
            return 0;
    } catch (nlohmann::detail::parse_error& err) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": parse " << filepath
                                 << " got a nlohmann::detail::parse_error, reason = " << err.what();
        return -1;
    } catch (std::exception& e) {
        // wxLogMessage("GUIDE: load_profile_error  %s ", e.what());
        // wxMessageBox(e.what(), "", MB_OK);
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": parse " << filepath << " got exception: " << e.what();
        return -1;
    }

    return 0;
}
