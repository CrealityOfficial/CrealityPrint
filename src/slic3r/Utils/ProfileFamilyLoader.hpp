#ifndef PROFILEFAMILYLOADER_HPP
#define PROFILEFAMILYLOADER_HPP

#include <mutex>
#include <map>
#include <string>
#include <future>

#include <boost/filesystem/path.hpp>

#include "nlohmann/json.hpp"

using namespace nlohmann;

class ThreadPool;


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
    void get_result(json& profile_json, json& machine_json, bool& load_custome_from_bundle)
    {
        profile_json = m_profile_json;
        machine_json = m_machine_json;
        load_custome_from_bundle = m_load_curstom_from_bundle;
    }
    bool data_empty();

private:
    ProfileFamilyLoader();
    ~ProfileFamilyLoader() = default;

    void request(); // start a load request
    void wait();    // wait for request finished

    int LoadProfile(json&       output_profile,
                    json&       output_machine,
                    bool&       bbl_bundle_rsrc);
    int LoadMachineJson(
        json& output_machine, 
        std::map<std::string, std::string> &mapMachineThumbnail,
        std::string strFilePath);
    int LoadProfileFamily(
        std::string strVendor, 
        std::string strFilePath,
        json& outputJson);
    bool LoadFile(std::string jPath, std::string& sContent);
    int  GetFilamentInfo(std::string VendorDirectory, json& pFilaList, 
        std::string filepath, std::string& sVendor, std::string& sType);


private:
    bool              m_first_frame_loading = false;
    bool              m_first_frame_loaded = false;
    json              m_machine_json; // machine json 
    json              m_profile_json; // all profile json, output json 

    json              m_creality_profile_json; // creality profile json
    json              m_resources_profile_json; // resource profile json
    std::map<std::string, std::string> m_map_machine_thumbnail;
    bool m_load_curstom_from_bundle = false;
    std::future<int> m_ret;
    ThreadPool* tp;
};

} // namespace Slic3r

#endif // PROFILEFAMILYLOADER_HPP