#ifndef slic3r_PresetConfigCache_hpp_
#define slic3r_PresetConfigCache_hpp_

#include "Config.hpp"

#include <boost/filesystem/path.hpp>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Slic3r {

class DynamicPrintConfig;

// Persistent cache for the expensive JSON-to-DynamicPrintConfig parsing step.
// Preset inheritance and collection construction intentionally remain outside
// the cache so their behavior stays identical to an uncached startup. System
// bundle manifests provide coarse-grained invalidation for the managed files.
class PresetConfigCache
{
public:
    PresetConfigCache(const boost::filesystem::path& source_root,
                      const std::string& vendor_name,
                      ForwardCompatibilitySubstitutionRule substitution_rule,
                      bool enabled = true,
                      const boost::filesystem::path& cache_root = {});
    ~PresetConfigCache();

    PresetConfigCache(const PresetConfigCache&) = delete;
    PresetConfigCache& operator=(const PresetConfigCache&) = delete;
    PresetConfigCache(PresetConfigCache&&) noexcept;
    PresetConfigCache& operator=(PresetConfigCache&&) noexcept;

    bool load(const std::string& relative_path,
              DynamicPrintConfig& config,
              std::map<std::string, std::string>& key_values,
              std::vector<std::string>& unrecognized_keys);

    void store(const std::string& relative_path,
               const DynamicPrintConfig& config,
               const std::map<std::string, std::string>& key_values,
               const std::vector<std::string>& unrecognized_keys);

    void erase(const std::string& relative_path);
    void save();
    void save_async();

    size_t hits() const;
    size_t misses() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace Slic3r

#endif // slic3r_PresetConfigCache_hpp_
