#pragma once

#include <filesystem>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace Slic3r::DataMigration {
namespace fs = std::filesystem;
class Error : public std::runtime_error { public: using std::runtime_error::runtime_error; };
struct Options {
    fs::path target;
    fs::path resources;
    std::string data_version = "7.3";
    std::string application_version;
    bool alpha = false;
    // Read-only validation. A failure isolates the preset; it never rewrites JSON or .info.
    std::function<std::vector<std::string>(const fs::path&)> validate_preset;
};
struct Result {
    bool migrated = false;
    std::string source_version;
    size_t presets = 0;
    size_t quarantined = 0;
};
Result initialize(const Options& options);
bool preserves_user_presets(const fs::path& target);
// Uses the same parser as the application, without saving or changing global data_dir.
std::vector<std::string> validate_preset_readonly(const fs::path& file);
} // namespace Slic3r::DataMigration
