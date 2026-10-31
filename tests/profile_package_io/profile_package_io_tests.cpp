#include "slic3r/GUI/ProfilePackageIO.hpp"

#include <chrono>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;
using nlohmann::json;
using namespace Slic3r::GUI::ProfilePackageIO;

static void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

static void write(const fs::path& path, const std::string& text)
{
    std::ofstream output(path, std::ios::binary);
    output.exceptions(std::ios::failbit | std::ios::badbit);
    output << text;
    output.close();
}

int main()
{
    const auto root = fs::temp_directory_path() /
        ("profile-package-io-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directory(root);
    const auto legacy = root / fs::u8path(u8"旧机型.json");
    const auto target = root / fs::u8path(u8"机型_model.json");
    int result = 0;
    try {
        const json model = {{"type", "machine_model"}, {"name", "Example"}};
        require(!migrate_machine_model(legacy, target), "Missing legacy file must not migrate");
        require(!matches(legacy, model), "Missing cache must need a write");

        write(legacy, "{\n  \"name\": \"Example\", \"type\": \"machine_model\"\n}\n");
        const auto before = fs::last_write_time(legacy);
        require(matches(legacy, model), "Whitespace/key order must not count as a change");
        require(fs::last_write_time(legacy) == before, "Comparison must not rewrite the file");
        require(!matches(legacy, {{"type", "machine_model"}, {"name", "Changed"}}),
                "Changed values must require a write");

        // Exercises the real read/close/rename sequence on Windows, including UTF-8 paths.
        require(migrate_machine_model(legacy, target), "Legacy model must migrate without sharing violation");
        require(!fs::exists(legacy) && matches(target, model), "Migration must preserve model contents");
        require(fs::last_write_time(target) == before, "Migration must preserve the file timestamp");
        require(!migrate_machine_model(legacy, target), "Migration must be idempotent");

        write(legacy, R"({"type":"machine_model","name":"Old"})");
        require(!migrate_machine_model(legacy, target), "An existing model must not be overwritten");
        require(matches(target, model) && fs::exists(legacy), "Both files must survive an existing target");
        fs::remove(target);

        write(legacy, R"({"type":"machine","name":"User preset"})");
        require(!migrate_machine_model(legacy, target), "A machine preset must not be renamed as a model");
        require(fs::exists(legacy) && !fs::exists(target), "Non-model file must be preserved");

        write(legacy, "{truncated");
        require(!matches(legacy, model), "Corrupt content must require repair");
        bool rejected = false;
        try { migrate_machine_model(legacy, target); }
        catch (const json::exception&) { rejected = true; }
        require(rejected && fs::exists(legacy) && !fs::exists(target), "Invalid legacy JSON must be preserved");
        const auto machine_dir = root / "machine";
        fs::create_directory(machine_dir);
        const json manifest = {
            {"machine_model_list", {{{"name", "Example"}, {"sub_path", "machine/model.json"}}}},
            {"machine_list", {{{"name", "Printer"}, {"sub_path", "machine/printer.json"}}}}
        };
        write(machine_dir / "model.json", R"({"type":"machine_model","name":"Example"})");
        write(machine_dir / "printer.json", R"({"type":"machine","name":"Printer"})");
        std::string reason;
        require(validate_manifest(root, manifest, &reason), "A complete manifest must validate");
        write(machine_dir / "printer.json", "");
        require(!validate_manifest(root, manifest, &reason) && reason.find("invalid profile JSON") != std::string::npos,
                "An empty generated profile must reject the installation");
        write(machine_dir / "printer.json", R"({"type":"machine_model","name":"Printer"})");
        require(!validate_manifest(root, manifest, &reason) && reason.find("unexpected profile type") != std::string::npos,
                "A machine-model/machine filename collision must reject the installation");
        std::cout << "Profile package IO tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        result = 1;
    }
    fs::remove(legacy);
    fs::remove(target);
    fs::remove_all(root);
    return result;
}
