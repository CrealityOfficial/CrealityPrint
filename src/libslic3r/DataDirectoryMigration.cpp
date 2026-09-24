#include "DataDirectoryMigration.hpp"
#include <nlohmann/json.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <thread>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tlhelp32.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#ifdef __APPLE__
#include <libproc.h>
#endif
#endif

namespace Slic3r::DataMigration {
using Json = nlohmann::json;
namespace {
std::string read(const fs::path& p)
{
    std::ifstream in(p, std::ios::binary);
    if (!in) throw Error("Cannot read: " + p.u8string());
    std::ostringstream out; out << in.rdbuf();
    if (in.bad()) throw Error("Read failed: " + p.u8string());
    return out.str();
}
void write(const fs::path& p, const std::string& content)
{
    fs::create_directories(p.parent_path());
    auto temporary = p; temporary += ".tmp";
    {
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        out << content; out.flush();
        if (!out) throw Error("Write failed: " + p.u8string());
    }
#ifdef _WIN32
    auto handle = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) throw Error("Cannot flush migration state");
    const bool flushed = FlushFileBuffers(handle) != 0; CloseHandle(handle);
    if (!flushed || !MoveFileExW(temporary.c_str(), p.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        throw Error("Cannot commit migration state: " + p.u8string());
#else
    const int handle = ::open(temporary.c_str(), O_WRONLY);
    if (handle < 0) throw Error("Cannot flush migration state");
    const int flushed = ::fsync(handle); ::close(handle);
    if (flushed != 0) throw Error("Cannot flush migration state");
    fs::rename(temporary, p);
#endif
}
Json parse(const fs::path& p, bool config = false)
{
    auto bytes = read(p);
    // AppConfig appends an MD5 comment on Windows.
    if (config) {
        const auto end = bytes.rfind('}');
        if (end != std::string::npos) bytes.resize(end + 1);
    }
    return Json::parse(bytes);
}
std::string digest(const std::string& bytes)
{
    boost::uuids::detail::md5 md5;
    boost::uuids::detail::md5::digest_type result;
    md5.process_bytes(bytes.data(), bytes.size()); md5.get_digest(result);
    std::ostringstream out;
    for (auto word : result) out << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << word;
    return out.str();
}
void check_path(const fs::path& p)
{
    if (fs::is_symlink(fs::symlink_status(p))) throw Error("Linked paths are not migrated: " + p.u8string());
#ifdef _WIN32
    auto attr = GetFileAttributesW(p.c_str());
    if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_REPARSE_POINT))
        throw Error("Reparse points are not migrated: " + p.u8string());
#endif
}
std::vector<fs::path> files(const fs::path& root)
{
    std::vector<fs::path> result;
    if (!fs::exists(root)) return result;
    check_path(root);
    for (const auto& e : fs::recursive_directory_iterator(root)) {
        check_path(e.path());
        if (e.is_regular_file()) result.push_back(e.path());
        else if (!e.is_directory()) throw Error("Unsupported file type: " + e.path().u8string());
    }
    std::sort(result.begin(), result.end());
    return result;
}
class Lock {
#ifdef _WIN32
    HANDLE handle = INVALID_HANDLE_VALUE;
#else
    int handle = -1;
#endif
public:
    explicit Lock(const fs::path& path) {
#ifdef _WIN32
        handle = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle == INVALID_HANDLE_VALUE) throw Error("Migration is locked: " + path.u8string());
#else
        handle = ::open(path.c_str(), O_CREAT | O_RDWR, 0600);
        if (handle < 0) throw Error("Cannot open migration lock");
        if (::flock(handle, LOCK_EX | LOCK_NB) != 0) { ::close(handle); handle = -1; throw Error("Migration is locked"); }
#endif
    }
    ~Lock() {
#ifdef _WIN32
        if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
#else
        if (handle >= 0) { ::flock(handle, LOCK_UN); ::close(handle); }
#endif
    }
    Lock(const Lock&) = delete;
    Lock& operator=(const Lock&) = delete;
};
bool other_slicer_running()
{
#ifdef _WIN32
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) throw Error("Cannot inspect source processes");
    PROCESSENTRY32W entry{}; entry.dwSize = sizeof(entry);
    bool found = false;
    if (!Process32FirstW(snap, &entry)) { CloseHandle(snap); throw Error("Cannot enumerate source processes"); }
    do {
        if (entry.th32ProcessID != GetCurrentProcessId() &&
            (_wcsicmp(entry.szExeFile, L"CrealityPrint.exe") == 0 ||
             _wcsicmp(entry.szExeFile, L"CrealityPrint_app_gui.exe") == 0)) { found = true; break; }
    } while (Process32NextW(snap, &entry));
    CloseHandle(snap); return found;
#elif defined(__linux__)
    for (const auto& e : fs::directory_iterator("/proc")) {
        const auto id = e.path().filename().string();
        if (id.empty() || id.find_first_not_of("0123456789") != std::string::npos || std::stol(id) == getpid()) continue;
        std::ifstream in(e.path() / "comm"); std::string name; std::getline(in, name);
        if (name == "CrealityPrint" || name == "crealityprint") return true;
    }
    return false;
#elif defined(__APPLE__)
    std::vector<pid_t> ids(4096);
    const int size = proc_listpids(PROC_ALL_PIDS, 0, ids.data(), int(ids.size() * sizeof(pid_t)));
    if (size <= 0 || size >= int(ids.size() * sizeof(pid_t))) throw Error("Cannot inspect source processes");
    for (int i = 0; i < size / int(sizeof(pid_t)); ++i) {
        if (ids[i] <= 0 || ids[i] == getpid()) continue;
        char name[1024]{}; proc_name(ids[i], name, sizeof(name));
        if (std::string(name) == "CrealityPrint") return true;
    }
    return false;
#else
    throw Error("Source process detection is not implemented on this platform");
#endif
}
struct Snapshot {
    std::map<fs::path, std::string> hashes;
#ifdef _WIN32
    std::vector<HANDLE> handles;
#endif
    ~Snapshot() {
#ifdef _WIN32
        for (auto h : handles) CloseHandle(h);
#endif
    }
    void add(const fs::path& path) {
        check_path(path);
#ifdef _WIN32
        // Keep selected source files read-only for the entire transaction.
        auto h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h == INVALID_HANDLE_VALUE) throw Error("Source is being written or is unreadable: " + path.u8string());
        handles.push_back(h);
#endif
        hashes.emplace(path, digest(read(path)));
    }
    void verify() const {
        for (const auto& item : hashes)
            if (digest(read(item.first)) != item.second) throw Error("Source changed during migration: " + item.first.u8string());
    }
};
bool valid_config(const Json& j)
{
    return j.is_object() && j.contains("app") && j["app"].is_object() &&
        j["app"].contains("version") && j["app"]["version"].is_string();
}
bool runtime_only(const fs::path& target)
{
    if (!fs::exists(target)) return true;
    check_path(target);
    for (const auto& e : fs::directory_iterator(target)) {
        const auto n = e.path().filename().u8string();
        check_path(e.path());
        if (n != "log" && n != "cache" && n != "applock.data") return false;
    }
    return true;
}
void copy_checked(const fs::path& from, const fs::path& to)
{
    check_path(from); fs::create_directories(to.parent_path());
    fs::copy_file(from, to, fs::copy_options::none);
    if (digest(read(from)) != digest(read(to))) throw Error("Copy verification failed: " + from.u8string());
}
void copy_tree(const fs::path& from, const fs::path& to)
{
    if (!fs::is_directory(from)) throw Error("Missing bundled resources: " + from.u8string());
    fs::create_directories(to);
    for (const auto& p : files(from)) copy_checked(p, to / p.lexically_relative(from));
}
// Cleanup is best-effort after commit. Never recursively delete runtime data or a lock file.
void cleanup_empty_runtime_backup(const fs::path& target, const Json& manifest) noexcept
{
    try {
        const auto it = manifest.find("transaction");
        if (it == manifest.end() || !it->is_string()) return;
        const auto transaction = it->get<std::string>();
        // Only reconstruct this transaction's sibling; never accept paths from the manifest.
        if (transaction.empty() || transaction.find_first_not_of("0123456789") != std::string::npos) return;
        const auto backup = target.parent_path() /
            fs::u8path("." + target.filename().u8string() + ".runtime-backup-" + transaction);
        check_path(backup);
        std::error_code error;
        if (!fs::is_directory(backup, error) || error) return;
        if (!fs::is_empty(backup, error) || error) return;
        // Non-recursive removal also protects files added between the emptiness check and removal.
        fs::remove(backup, error);
    } catch (...) {
        // An inaccessible backup must not turn a committed migration into a startup failure.
        // The next launch retries cleanup using the committed manifest.
    }
}
std::string preset_key(const std::string& type, const std::string& name) { return type + "\n" + name; }
struct Preset { fs::path path; Json json; std::string account, type, name, parent, problem; };
void validate_presets(const Options& o, const fs::path& stage, Json& report, Result& result)
{
    std::map<std::string, Json> system;
    // Only index profiles actually declared by installed vendor manifests.
    for (const auto& e : fs::directory_iterator(stage / "system")) {
        if (!e.is_regular_file() || e.path().extension() != ".json") continue;
        const auto manifest = parse(e.path());
        const auto vendor = e.path().stem();
        for (const auto& type : {"machine", "filament", "process"}) {
            const std::string list = std::string(type) + "_list";
            if (!manifest.contains(list)) continue;
            for (const auto& entry : manifest.at(list)) {
                const auto relative = fs::u8path(entry.at("sub_path").get<std::string>());
                if (relative.is_absolute() || relative.lexically_normal().u8string().find("..") != std::string::npos)
                    throw Error("Unsafe resource manifest path");
                const auto data = parse(stage / "system" / vendor / relative);
                system[preset_key(type, data.at("name").get<std::string>())] = data;
            }
        }
    }
    std::vector<Preset> presets;
    for (const auto& p : files(stage / "user")) {
        auto rel = p.lexically_relative(stage / "user");
        auto it = rel.begin(); const std::string account = (it++)->u8string();
        if (it == rel.end()) continue;
        const std::string type = (it++)->u8string();
        if ((type != "machine" && type != "filament" && type != "process") || p.extension() != ".json") continue;
        Preset item; item.path = p; item.account = account; item.type = type;
        try {
            item.json = parse(p); item.name = item.json.at("name").get<std::string>();
            item.parent = item.json.value("inherits", std::string());
            if (item.name.empty()) throw Error("Empty preset name");
            if (!o.validate_preset) throw Error("Native preset validator is required");
            for (const auto& warning : o.validate_preset(p))
                report["warnings"].push_back({{"file", p.lexically_relative(stage).u8string()}, {"reason", warning}});
        } catch (const Json::exception&) { item.problem = "Invalid preset JSON structure"; }
        catch (const std::exception& e) { item.problem = e.what(); }
        presets.push_back(std::move(item));
    }
    std::map<std::string, size_t> users;
    auto user_key = [](const Preset& p, const std::string& name) { return p.account + "\n" + preset_key(p.type, name); };
    for (size_t i = 0; i < presets.size(); ++i) {
        auto& p = presets[i];
        const auto key = user_key(p, p.name);
        if (users.count(key) || system.count(preset_key(p.type, p.name))) {
            p.problem = "Duplicate preset name";
            if (users.count(key)) presets[users.at(key)].problem = p.problem;
        } else users[key] = i;
    }
    // Resolve complete inheritance chains, including user roots in base/; detect cycles.
    std::function<bool(const Preset&, const std::string&, std::set<std::string>&)> resolves;
    resolves = [&](const Preset& context, const std::string& name, std::set<std::string>& visiting) -> bool {
        if (name.empty()) return true;
        const auto key = preset_key(context.type, name);
        if (!visiting.insert(key).second) return false;
        bool ok = false;
        const auto u = users.find(user_key(context, name));
        if (u != users.end()) {
            const auto& parent = presets[u->second];
            ok = parent.problem.empty() && resolves(context, parent.parent, visiting);
        } else if (const auto s = system.find(key); s != system.end()) {
            ok = resolves(context, s->second.value("inherits", std::string()), visiting);
        }
        visiting.erase(key); return ok;
    };
    // Iterate because isolating a parent must also isolate dependent presets.
    bool changed;
    do {
        changed = false;
        for (auto& p : presets) if (p.problem.empty()) {
            std::set<std::string> visiting{preset_key(p.type, p.name)};
            if (!resolves(p, p.parent, visiting)) { p.problem = "Missing, incompatible or cyclic parent: " + p.parent; changed = true; }
        }
    } while (changed);
    for (const auto& p : presets) {
        const auto relative = p.path.lexically_relative(stage);
        if (p.problem.empty()) {
            if (p.json.contains("compatible_printers") && p.json["compatible_printers"].is_array()) {
                for (const auto& printer : p.json["compatible_printers"]) {
                    if (!printer.is_string()) continue;
                    const auto name = printer.get<std::string>();
                    if (!system.count(preset_key("machine", name)) &&
                        !users.count(p.account + "\n" + preset_key("machine", name)))
                        report["warnings"].push_back({{"file", relative.u8string()}, {"reason", "Compatible printer absent from target; reference preserved: " + name}});
                }
            }
            ++result.presets; continue;
        }
        const auto destination = stage / "migration" / "quarantine" / relative;
        fs::create_directories(destination.parent_path()); fs::rename(p.path, destination);
        auto info = p.path; info.replace_extension(".info");
        if (fs::exists(info)) { auto target = destination; target.replace_extension(".info"); fs::rename(info, target); }
        report["quarantined"].push_back({{"file", relative.u8string()}, {"reason", p.problem}});
        ++result.quarantined;
    }
}
} // namespace

bool preserves_user_presets(const fs::path& target)
{
    return fs::exists(target / "migration" / "preserve-user-presets");
}
Result initialize(const Options& o)
{
    Result result;
    const auto target = fs::absolute(o.target).lexically_normal();
    const auto parent = target.parent_path();
    if (o.data_version.empty() || o.data_version.find_first_not_of("0123456789.") != std::string::npos)
        throw Error("Invalid data directory version");
    const auto suffix = o.alpha ? " Alpha" : "";
    if (target.filename().u8string() != o.data_version + suffix) throw Error("Unexpected target directory");
    check_path(parent); check_path(target);
    fs::create_directories(parent);
    Lock lock(parent / fs::u8path(".migration-" + target.filename().u8string() + ".lock"));
    const auto diagnostics = parent / fs::u8path(".migration-" + target.filename().u8string());
    fs::create_directories(diagnostics);
    try {
        if (fs::exists(target / "migration" / "manifest.json")) {
            auto manifest = parse(target / "migration" / "manifest.json");
            if (manifest.at("target_data_version") != o.data_version) throw Error("Target manifest version mismatch");
            // READY_TO_COMMIT inside the final path means the directory rename completed.
            if (manifest.at("state") != "COMMITTED" && manifest.at("state") != "READY_TO_COMMIT") throw Error("Unknown target migration state");
            if (manifest.at("state") != "COMMITTED") {
                manifest["state"] = "COMMITTED";
                write(target / "migration" / "manifest.json", manifest.dump(2));
            }
            cleanup_empty_runtime_backup(target, manifest);
            return result; // Never re-import into a previously initialized target, even if config later breaks.
        }
        if (fs::exists(target / "Creality.conf")) {
            try { if (valid_config(parse(target / "Creality.conf", true))) return result; }
            catch (const Json::exception&) {}
            throw Error("Existing target configuration needs recovery; refusing to overwrite it");
        }
        if (!runtime_only(target)) throw Error("Target contains existing user data; refusing to overwrite it");
        if (o.data_version != "7.3") throw Error("No migration rules registered for data version " + o.data_version);
        Json report = {{"warnings", Json::array()}, {"quarantined", Json::array()}, {"skipped_sources", Json::array()}, {"copied_files", Json::array()}};
        fs::path source, config_file; Json config;
        for (const auto& version : {"7.0", "6.0"}) {
            const auto candidate = parent / fs::u8path(std::string(version) + suffix);
            if (!fs::exists(candidate)) continue;
            check_path(candidate);
            Json attempts = Json::array();
            for (const auto& filename : {"Creality.conf", "Creality.conf.bak"}) {
                const auto file = candidate / filename;
                if (!fs::exists(file)) {
                    attempts.push_back({{"file", filename}, {"reason", "Configuration file is missing"}});
                    continue;
                }
                check_path(file);
                try {
                    auto j = parse(file, true);
                    if (!valid_config(j)) {
                        attempts.push_back({{"file", filename}, {"reason", "Expected an app object with a string version field"}});
                        continue;
                    }
                    // The allowlisted directory identifies the data epoch. app.version is only
                    // provenance: pre-isolation 7.3 builds wrote 7.0, and 6.0 used 01.09.03.50.
                    // JSON structure and read-only native preset validation decide compatibility.
                    source = candidate; config_file = file;
                    result.source_version = j["app"]["version"].get<std::string>();
                    config = std::move(j); break;
                } catch (const Json::exception& error) {
                    // Do not include parser excerpts: the source can contain login credentials.
                    attempts.push_back({{"file", filename}, {"reason", "Invalid configuration JSON"}, {"json_error_id", error.id}});
                    // Try the backup; I/O errors remain fatal and retryable.
                }
            }
            if (!source.empty()) break;
            report["skipped_sources"].push_back({{"directory", candidate.filename().u8string()},
                {"reason", "No readable configuration with the required structure"}, {"attempts", std::move(attempts)}});
        }
        if (source.empty() && !report["skipped_sources"].empty()) {
            // Existing legacy data must not be hidden behind a successfully initialized empty
            // target. Leave no committed manifest so a corrected source can be retried next launch.
            report["state"] = "SOURCE_UNAVAILABLE";
            write(diagnostics / "report.json", report.dump(2));
            throw Error("Existing source directories have no valid configuration; see " +
                (diagnostics / "report.json").u8string());
        }
        if (!source.empty()) {
            for (int attempt = 0; other_slicer_running(); ++attempt) {
                if (attempt == 10) throw Error("Another Creality Print process is running; retry after it exits");
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }
        const auto transaction = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        const auto stage = parent / fs::u8path("." + target.filename().u8string() + ".staging-" + transaction);
        fs::create_directory(stage);
        Json manifest = {{"transaction", transaction}, {"target_data_version", o.data_version}, {"target_application_version", o.application_version},
            {"source_directory", source.empty() ? "" : source.filename().u8string()}, {"source_application_version", result.source_version},
            {"rule_revision", 2}, {"presets_policy", "byte-preserving"}, {"state", "PREPARING"}};
        auto state = [&](const char* value) { manifest["state"] = value; write(diagnostics / "transaction.json", manifest.dump(2)); };
        state("COPYING");
        Snapshot snapshot;
        std::vector<fs::path> source_user_files;
        auto copy_source = [&](const fs::path& from, const fs::path& relative) {
            snapshot.add(from); copy_checked(from, stage / relative);
            report["copied_files"].push_back({{"source", from.lexically_relative(source).u8string()}, {"target", relative.u8string()}, {"md5", snapshot.hashes.at(from)}});
        };
        if (!source.empty()) {
            copy_source(config_file, "Creality.conf");
            copy_checked(config_file, stage / "Creality.conf.bak");
            for (const auto& name : {"extra_config.json", "deviceInfo.json", "user_info.json", "privacyInfo.json"}) {
                const auto p = source / name;
                if (!fs::exists(p)) continue;
                try { parse(p); copy_source(p, name); }
                catch (const Json::exception&) {
                    copy_source(p, fs::path("migration/quarantine") / name);
                    report["warnings"].push_back({{"file", name}, {"reason", "Invalid JSON; original preserved outside active configuration"}});
                }
            }
            source_user_files = files(source / "user");
            for (const auto& p : source_user_files) {
                const auto relative = p.lexically_relative(source);
                auto it = relative.begin(); ++it; if (it == relative.end()) continue; ++it;
                const std::string category = it == relative.end() ? "" : it->u8string();
                if (p.extension() == ".cereal") continue;
                if (category == "machine" || category == "filament" || category == "process" || category == "local_device")
                    copy_source(p, relative);
                else // Preserve sync metadata for audit, but do not replay old sync queues.
                    copy_source(p, fs::path("migration/legacy-sync") / relative);
            }
            for (const auto& p : files(source / "filament_mixing")) copy_source(p, p.lexically_relative(source));
            write(stage / "migration/preserve-user-presets", "Do not rewrite migrated preset names or parameters.\n");
        }
        state("INSTALLING_RESOURCES");
        std::set<std::string> vendors{"Creality"};
        if (config.contains("models") && config["models"].is_array())
            for (const auto& model : config["models"]) if (model.contains("vendor") && model["vendor"].is_string()) vendors.insert(model["vendor"].get<std::string>());
        fs::create_directories(stage / "system");
        for (const auto& vendor : vendors) {
            if (vendor.empty() || vendor.find_first_of("/\\:") != std::string::npos || vendor == "..") throw Error("Invalid vendor identifier");
            const auto root = o.resources / "profiles";
            if (!fs::exists(root / fs::u8path(vendor + ".json"))) {
                if (vendor == "Creality") throw Error("Missing Creality resource manifest");
                report["warnings"].push_back({{"vendor", vendor}, {"reason", "Vendor absent from bundled resources"}}); continue;
            }
            copy_checked(root / fs::u8path(vendor + ".json"), stage / "system" / fs::u8path(vendor + ".json"));
            copy_tree(root / fs::u8path(vendor), stage / "system" / fs::u8path(vendor));
        }
        copy_tree(o.resources / "printers", stage / "printers");
        state("VALIDATING");
        validate_presets(o, stage, report, result);
        snapshot.verify();
        if (!source.empty() && (files(source / "user") != source_user_files || other_slicer_running())) throw Error("Source changed during migration; retry required");
        // Every imported preset and .info must still match the source, whether active or isolated.
        for (const auto& item : report["copied_files"]) {
            const auto rel = fs::u8path(item.at("target").get<std::string>());
            auto destination = stage / rel;
            if (!fs::exists(destination)) destination = stage / "migration/quarantine" / rel;
            if (digest(read(destination)) != item.at("md5").get<std::string>()) throw Error("Imported file was changed during validation");
        }
        report["source_application_version"] = result.source_version;
        report["valid_presets"] = result.presets; report["quarantined_presets"] = result.quarantined;
        report["cloud_preset_sync"] = source.empty() ? "unchanged" : "held";
        write(stage / "migration/report.json", report.dump(2));
        state("READY_TO_COMMIT");
        write(stage / "migration/manifest.json", manifest.dump(2));
        if (!runtime_only(target)) throw Error("Target changed before commit");
        if (fs::exists(target)) fs::rename(target, parent / fs::u8path("." + target.filename().u8string() + ".runtime-backup-" + transaction));
        fs::rename(stage, target);
        // The manifest is already durable in the renamed directory. Recovery accepts READY_TO_COMMIT.
        state("COMMITTED");
        write(target / "migration/manifest.json", manifest.dump(2));
        cleanup_empty_runtime_backup(target, manifest);
        result.migrated = !source.empty();
        return result;
    } catch (const std::exception& e) {
        try { write(diagnostics / "last-error.txt", e.what()); } catch (...) {}
        throw Error(e.what());
    }
}
} // namespace Slic3r::DataMigration
