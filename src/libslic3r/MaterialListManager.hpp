#ifndef slic3r_MaterialListManager_hpp_
#define slic3r_MaterialListManager_hpp_

#include <ctime>
#include <mutex>
#include <string>
#include <unordered_map>

#include <boost/filesystem/path.hpp>
#include <nlohmann/json_fwd.hpp>

namespace Slic3r {

class Preset;

class MaterialListManager
{
public:
    static MaterialListManager& instance();

    boost::filesystem::path material_list_path() const;
    std::string base_name_from_preset_name(std::string material_name) const;
    std::string display_name_from_preset_name(const std::string& preset_name);
    std::string display_name_with_material_alias(const Preset& preset, bool keep_printer_suffix = true);
    std::string display_name_with_material_alias(const std::string& preset_name, bool keep_printer_suffix = true);
    bool material_alias_matches_preset_name(const std::string& preset_name, const std::string& alias);
    bool save_official_material_list(const nlohmann::json& materials);

private:
    MaterialListManager() = default;
    MaterialListManager(const MaterialListManager&) = delete;
    MaterialListManager& operator=(const MaterialListManager&) = delete;

    void refresh_alias_cache_locked();

    boost::filesystem::path m_alias_cache_path;
    std::time_t m_alias_cache_last_write_time = 0;
    bool m_alias_cache_initialized = false;
    std::unordered_map<std::string, std::string> m_aliases;
    std::mutex m_alias_cache_mutex;
};

} // namespace Slic3r

#endif
