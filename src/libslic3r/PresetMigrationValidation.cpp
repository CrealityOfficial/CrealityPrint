#include "DataDirectoryMigration.hpp"
#include "PrintConfig.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <set>

namespace Slic3r::DataMigration {
std::vector<std::string> validate_preset_readonly(const fs::path& file)
{
    std::ifstream in(file); nlohmann::json original; in >> original;
    DynamicPrintConfig config;
    ConfigSubstitutionContext substitutions(ForwardCompatibilitySubstitutionRule::Disable);
    std::map<std::string, std::string> metadata;
    std::string reason;
    const int status = config.load_from_json(file.u8string(), substitutions, true, metadata, reason);
    if (status != 0 || !reason.empty()) throw Error("Preset parameter parsing failed");
    std::vector<std::string> warnings;
    const std::set<std::string> headers{"name", "from", "version", "type", "setting_id", "base_id", "filament_id", "description", "is_custom_defined", "instantiation", "url"};
    for (auto it = original.begin(); it != original.end(); ++it) {
        if (headers.count(it.key()) || print_config_def.has(it.key())) continue;
        warnings.push_back("Legacy or unknown parameter retained without rewriting: " + it.key());
    }
    // Loading may perform legacy substitutions in memory; the on-disk JSON and .info stay untouched.
    return warnings;
}
} // namespace Slic3r::DataMigration
