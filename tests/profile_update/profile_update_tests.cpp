#include "slic3r/GUI/ProfileUpdatePolicy.hpp"
#include <cstdlib>
#include <iostream>
#include <set>

using namespace Slic3r::GUI::ProfileUpdatePolicy;
using nlohmann::json;

static void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

static json printer(const std::string& name, const std::string& version, json nozzles)
{
    return {{"name", name}, {"showVersion", version}, {"nozzleDiameter", std::move(nozzles)}};
}

static std::set<std::string> pending(const json& remote, const json& receipts,
                                     const std::set<std::string>& files)
{
    const auto local = installed_versions(receipts);
    std::set<std::string> result;
    for (const auto& entry : select_packages(remote)) {
        const auto found = local.find(entry.first);
        if (needs_update(files.count(entry.first) != 0, found == local.end() ? "" : found->second,
                         entry.second.value("showVersion", std::string())))
            result.insert(entry.first);
    }
    return result;
}

static std::set<std::string> automatic_pending(const json& remote,
                                               const std::set<std::string>& files,
                                               const std::set<std::string>& added)
{
    std::set<std::string> result;
    for (const auto& entry : select_automatic_packages(remote, added)) {
        if (needs_install(files.count(entry.first) != 0,
                          entry.second.value("showVersion", std::string())))
            result.insert(entry.first);
    }
    return result;
}

int main()
{
    const auto k3 = printer("K3", "V3.10.0", {"0.4", "0.4", "0.4"});
    const json allowed = json::array({k3});
    const json empty = json::array();
    const std::set<std::string> beta_files{"Creality K3"};

    require(pending(empty, empty, beta_files).empty(), "Hidden K3 must not be installed from local remnants");
    require(pending(allowed, empty, beta_files) == beta_files,
            "Opening the whitelist must repair beta files without a version receipt");
    require(pending(allowed, empty, {}).size() == 1, "A fresh installation must download the package");
    require(pending(allowed, empty, beta_files) == beta_files,
            "A failed installation without a receipt must remain retryable");
    require(pending(allowed, allowed, beta_files).empty(), "A successful install must not download repeatedly");
    require(pending(allowed, allowed, {}).size() == 1, "A receipt alone does not prove the machine file exists");

    require(needs_update(true, "V3.9.0", "V3.10.0"), "Compare numeric versions, not strings");
    require(!needs_update(true, "V3.10.0", "V3.9.0"), "Preserve a newer beta package");
    require(!needs_update(true, "v3.10.0", "3.10.0"), "Version prefixes do not change equality");
    require(needs_update(true, "3.10.0-beta.1", "V3.10.0"), "Promote prerelease to the matching release");
    require(needs_update(true, "invalid", "V3.10.0"), "Repair an unknown local version");
    require(!needs_update(true, "V3.9.0", "invalid"), "Do not install an invalid remote version");
    require(!needs_update(false, "", ""), "Missing remote metadata cannot establish an update");

    // A 0.6 alias and a 0.4 package both write Creality K3.json. Select the
    // standard package before deciding whether it is newer than the receipt.
    const auto alias = printer("Creality K3", "V3.99.0", {"0.6", "0.6", "0.6"});
    const json aliases = json::array({alias, k3});
    require(select_packages(aliases).size() == 1, "Shared profiles must be deduplicated");
    require(select_packages(aliases).at("Creality K3")["showVersion"] == "V3.10.0",
            "The standard package must win over a newer nozzle alias");
    require(pending(aliases, allowed, beta_files).empty(), "Alias versions must not cause repeated downloads");
    require(installed_versions(aliases).at("Creality K3") == "V3.10.0", "Receipts use the same alias selection");
    require(select_packages(json::array({k3, alias})).at("Creality K3")["showVersion"] == "V3.10.0",
            "Alias selection must not depend on response order");

    require(profile_name(printer("Sermoon D3 Pro", "V3.1.0", {"0.4", "0.4"})) ==
            "Creality Sermoon D3 Pro 0.4 nozzle", "Keep the legacy D3 Pro filename");
    require(profile_name(printer("SPARKX i7", "V3.1.0", {"0.4"})) == "SPARKX i7 0.4 nozzle",
            "Do not add a Creality prefix to SPARKX");
    require(select_packages(json::array({printer("K1", "V3.1.0", {"0.4"}),
                                         printer("K1", "V3.1.0", {"0.6"})})).size() == 2,
            "Separate single-extruder nozzle profiles must remain separate");
    require(select_packages(json::array({nullptr, json::object(), printer("K3", "V3.1.0", {})})).empty(),
            "Malformed catalog rows must not produce filenames");

    // Guide installation must leave added profiles pending for confirmation, including
    // migrated beta files without receipts and aliases of shared nozzle profiles.
    const auto k1 = printer("K1", "V3.10.0", {"0.4"});
    const json mixed = json::array({alias, k3, k1});
    const std::set<std::string> k1_name{"Creality K1 0.4 nozzle"};
    require(automatic_pending(mixed, beta_files, beta_files) == k1_name,
            "Only unadded printers may be automatically installed in a mixed catalogue");
    require(automatic_pending(allowed, beta_files, beta_files).empty() &&
            pending(allowed, empty, beta_files) == beta_files,
            "An added beta printer without a receipt must still be eligible for a tip, not auto repair");
    require(automatic_pending(allowed, {}, beta_files).empty() &&
            pending(allowed, empty, {}) == beta_files,
            "A missing file must not bypass confirmation for an added printer");
    const json old_receipt = json::array({printer("K3", "V3.9.0", {"0.4", "0.4", "0.4"})});
    require(automatic_pending(allowed, beta_files, beta_files).empty() &&
            pending(allowed, old_receipt, beta_files) == beta_files,
            "An added printer with an older version must be prompted, not automatically updated");
    require(automatic_pending(allowed, beta_files, {}).empty() &&
            pending(allowed, empty, beta_files) == beta_files &&
            pending(allowed, old_receipt, beta_files) == beta_files,
            "Existing unadded files must not be auto-repaired or upgraded even when a version is missing or old");
    require(automatic_pending(allowed, {}, {}) == beta_files,
            "The guide must still install missing unadded printer files");
    require(!needs_install(true, "V3.10.0") && needs_install(false, "V3.10.0"),
            "Only missing files qualify for automatic installation");
    require(!needs_install(false, "invalid"),
            "Invalid remote versions must not authorize automatic installation");
    const json single_nozzles = json::array({k1, printer("K1", "V3.10.0", {"0.6"})});
    require(automatic_pending(single_nozzles, {}, k1_name) ==
                std::set<std::string>{"Creality K1 0.6 nozzle"},
            "An added independent nozzle must not block automatic installation of another nozzle");
    require(automatic_pending(mixed, beta_files, {}) == k1_name,
            "Removing an added printer must not permit replacement of its existing file");

    // A later check uses current visibility even if an earlier check found work.
    require(pending(allowed, empty, beta_files).size() == 1 && pending(empty, empty, beta_files).empty(),
            "Do not retain a pending package after whitelist removal");
    std::cout << "Profile update policy tests passed\n";
}
