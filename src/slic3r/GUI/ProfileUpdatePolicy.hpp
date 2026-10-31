#ifndef slic3r_ProfileUpdatePolicy_hpp_
#define slic3r_ProfileUpdatePolicy_hpp_

#include <map>
#include <set>
#include <string>
#include <nlohmann/json.hpp>
#include "libslic3r/Semver.hpp"

namespace Slic3r::GUI::ProfileUpdatePolicy {

using Versions = std::map<std::string, std::string>;
using Packages = std::map<std::string, nlohmann::json>;

// Use the same installed filename for catalog entries and version records.
// Multiple extruders share one profile, except for the legacy D3 Pro format.
inline std::string profile_name(const nlohmann::json& printer)
{
    if (!printer.is_object() || !printer.contains("name") || !printer["name"].is_string() ||
        printer["name"].get<std::string>().empty() || !printer.contains("nozzleDiameter") ||
        !printer["nozzleDiameter"].is_array() || printer["nozzleDiameter"].empty())
        return {};
    for (const auto& nozzle : printer["nozzleDiameter"])
        if (!nozzle.is_string() || nozzle.get<std::string>().empty())
            return {};

    std::string model = printer["name"].get<std::string>();
    if (model.find("Creality") == std::string::npos && model.find("SPARKX") == std::string::npos)
        model = "Creality " + model;
    if (printer["nozzleDiameter"].size() > 1 && model != "Creality Sermoon D3 Pro" &&
        printer.value("printerIntName", std::string()) != "Sermoon D3 Pro")
        return model;
    return model + " " + printer["nozzleDiameter"][0].get<std::string>() + " nozzle";
}

// Select before comparing versions: an alias for a shared multi-extruder
// profile must not override the standard (0.4 mm) package's installed version.
inline Packages select_packages(const nlohmann::json& printers)
{
    Packages selected;
    if (!printers.is_array())
        return selected;
    for (const auto& printer : printers) {
        const std::string name = profile_name(printer);
        if (name.empty())
            continue;
        auto current = selected.find(name);
        if (current == selected.end() ||
            (current->second["nozzleDiameter"][0] != "0.4" && printer["nozzleDiameter"][0] == "0.4"))
            selected[name] = printer;
    }
    return selected;
}

// Added printers require confirmation even if their files or receipts are missing.
// Filtering by profile name also protects all aliases of shared nozzle profiles.
inline Packages select_automatic_packages(const nlohmann::json& printers,
                                         const std::set<std::string>& added_printers)
{
    auto selected = select_packages(printers);
    for (const auto& name : added_printers)
        selected.erase(name);
    return selected;
}

inline Versions installed_versions(const nlohmann::json& records)
{
    Versions versions;
    for (const auto& entry : select_packages(records))
        if (entry.second.contains("showVersion") && entry.second["showVersion"].is_string())
            versions[entry.first] = entry.second["showVersion"].get<std::string>();
    return versions;
}

inline boost::optional<Semver> parse_version(std::string version)
{
    if (!version.empty() && (version.front() == 'V' || version.front() == 'v'))
        version.erase(0, 1);
    if (version.empty() || version.front() < '0' || version.front() > '9')
        return boost::none;
    return Semver::parse(version);
}

// Automatic installation fills missing files only, regardless of the local version.
inline bool needs_install(bool file_exists, const std::string& available)
{
    return !file_exists && bool(parse_version(available));
}

inline bool needs_update(bool file_exists, const std::string& installed, const std::string& available)
{
    const auto remote = parse_version(available);
    if (!remote)
        return false; // A malformed server version cannot establish an update.
    if (!file_exists)
        return true;
    const auto local = parse_version(installed);
    // A beta profile without an installed package version needs reconciliation.
    return !local || *remote > *local;
}

} // namespace Slic3r::GUI::ProfileUpdatePolicy

#endif
