#ifndef PROFILEFAMILYLOADER_HPP
#define PROFILEFAMILYLOADER_HPP

#include <mutex>
#include <map>
#include <string>
#include <future>

#include <boost/filesystem/path.hpp>

#include "nlohmann/json.hpp"

using namespace nlohmann;

namespace Slic3r {

// ProfileFamilyLoader can load all profile json, such as: machine,filament,process
// non-thread-safe
class ProfileFamilyLoader
{
public:
    static ProfileFamilyLoader* get_instance() {
        static ProfileFamilyLoader* ins = new ProfileFamilyLoader;
        return ins;
    }
    static void init() { ProfileFamilyLoader::get_instance(); }

    void request_and_wait();
    void wait_until_loaded();

    // Reload only machineList.json (the series / printer grouping used by the
    // "add printer" navigation tree) and replace the cached machine json.
    //
    // This exists because machineList.json is re-downloaded per login region while
    // the full profile family is loaded once per process (see request()). Without a
    // targeted reload, a freshly downloaded list is only picked up after a restart.
    // Reparsing this single file is cheap compared to request_and_wait(), which
    // rescans every vendor machine/filament/process preset.
    //
    // Returns true only when m_machine_json was actually replaced. The previous
    // list is kept on parse failure or when the new file yields no series at all.
    bool reload_machine_list();

    void get_result(json& profile_json, json& machine_json, bool& load_custome_from_bundle)
    {
        std::lock_guard<std::mutex> lock(m_state_mutex);
        profile_json = m_profile_json;
        machine_json = m_machine_json;
        load_custome_from_bundle = m_load_curstom_from_bundle;
    }
    void get_filament_result(json& filament_json)
    {
        std::lock_guard<std::mutex> lock(m_state_mutex);
        filament_json = m_profile_json.contains("filament") ? m_profile_json["filament"] : json::object();
    }
    bool data_empty();

private:
    ProfileFamilyLoader();
    ~ProfileFamilyLoader() = default;

    void request(bool force_reload = false); // start a load request
    void wait();    // wait for request finished

    int LoadProfile(json&       output_profile,
                    json&       output_machine,
                    bool&       bbl_bundle_rsrc);
    int LoadMachineJson(
        json& output_machine,
        std::map<std::string, std::string>& mapMachineThumbnail,
        const boost::filesystem::path& file_path);
    // Resolves which machineList.json wins: the per-user one under
    // <data_dir>/system/<vendor>/ when present, otherwise the bundled
    // <resources>/profiles/<vendor>/ fallback.
    static boost::filesystem::path machine_list_path();
    int LoadProfileFamily(
        const std::string& vendor,
        const boost::filesystem::path& file_path,
        json& outputJson);
    bool LoadFile(const boost::filesystem::path& path, std::string& content);
    int  GetFilamentInfo(const boost::filesystem::path& vendor_directory,
                         json& pFilaList,
                         const boost::filesystem::path& file_path,
                         std::string& sVendor,
                         std::string& sType);


private:
    mutable std::mutex m_state_mutex;
    bool              m_first_frame_loading = false;
    bool              m_first_frame_loaded = false;
    json              m_machine_json; // machine json 
    json              m_profile_json; // all profile json, output json 

    json              m_creality_profile_json; // creality profile json
    json              m_resources_profile_json; // resource profile json
    std::map<std::string, std::string> m_map_machine_thumbnail;
    bool m_load_curstom_from_bundle = false;
    std::shared_future<int> m_ret;
};

} // namespace Slic3r

#endif // PROFILEFAMILYLOADER_HPP
