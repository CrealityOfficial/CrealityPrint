#ifndef slic3r_SceneFilamentSourceSnapshotManager_hpp_
#define slic3r_SceneFilamentSourceSnapshotManager_hpp_

#include "nlohmann/json.hpp"

#include <string>

namespace Slic3r {

class DynamicConfig;

namespace GUI {

class SceneFilamentSourceSnapshotManager
{
public:
    void reset();
    bool initialized() const;
    bool capture_from_current_config_if_needed();
    bool capture_from_loaded_3mf_config(const DynamicConfig& loaded_config);
    nlohmann::json export_items() const;

private:
    bool is_easy_mode_active() const;
    bool build_items_from_current_config(nlohmann::json& out_items, std::string& out_fingerprint) const;
    bool build_items_from_loaded_3mf_config(
        const DynamicConfig& loaded_config,
        nlohmann::json& out_items,
        std::string& out_fingerprint) const;

    bool           m_initialized = false;
    nlohmann::json m_items = nlohmann::json::array();
    std::string    m_scene_fingerprint;
};

} // namespace GUI
} // namespace Slic3r

#endif
