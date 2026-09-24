#include "libslic3r/PresetSyncUtils.hpp"
#include <boost/nowide/fstream.hpp>
#include <boost/nowide/convert.hpp>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <set>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace fs = std::filesystem;
namespace policy = Slic3r::PresetSyncUtils;
using json = nlohmann::json;

void require(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

bool equivalent(const std::string& a, const std::string& b)
{
#ifdef _WIN32
    return CompareStringOrdinal(boost::nowide::widen(a).c_str(), -1,
                                boost::nowide::widen(b).c_str(), -1, TRUE) == CSTR_EQUAL;
#else
    return a == b;
#endif
}

void version_selection()
{
    // The three cloud records observed in the user's log, in every arrival order.
    const std::vector<policy::Values> variants = {
        {{"updated_time", "1789633362"}, {"setting_id", "6aaba352297739ae0467e4cd"}},
        {{"updated_time", "1789633359"}, {"setting_id", "6aaba34f297739ae0467d6db"}},
        {{"updated_time", "1789633346"}, {"setting_id", "6aaba342297739ae0467a13f"}}
    };
    std::vector<int> order{0, 1, 2};
    do {
        auto selected = variants[order.front()];
        for (int index : order)
            if (policy::prefer_incoming(selected, variants[index])) selected = variants[index];
        require(selected == variants[0], "cloud list order selected an older preset");
    } while (std::next_permutation(order.begin(), order.end()));

    const policy::Values a{{"updated_time", "100"}, {"setting_id", "a"}};
    const policy::Values b{{"updated_time", "100"}, {"setting_id", "b"}};
    require(policy::prefer_incoming(a, b) && !policy::prefer_incoming(b, a), "unstable timestamp tie");
    for (const std::string time : {"", "abc", "-1", "100suffix", "99999999999999999999999"}) {
        const policy::Values invalid{{"updated_time", time}, {"setting_id", "z"}};
        require(!policy::prefer_incoming(a, invalid), "invalid time displaced a valid record");
    }
}

json read(const fs::path& path)
{
    boost::nowide::ifstream stream(path.u8string());
    json result;
    stream >> result;
    return result;
}

void write(const fs::path& path, const std::string& name, const std::string& id)
{
    boost::nowide::ofstream stream(path.u8string());
    stream << json{{"name", name}, {"setting_id", id}}.dump();
    stream.close();
    require(!stream.fail(), "test could not write file");
    fs::path info = path;
    info.replace_extension(".info");
    boost::nowide::ofstream metadata(info.u8string());
    metadata << id;
}

fs::path choose(const fs::path& directory, const std::string& name)
{
    return directory / fs::u8path(policy::available_filename(name, [&](const std::string& filename) {
        const fs::path path = directory / fs::u8path(filename);
        return !fs::exists(path) || read(path).at("name").get<std::string>() == name;
    }));
}

void filename_collisions(const fs::path& root)
{
    const std::string upper = "SUNLU PETG WHITE @Creality K2 Plus 0.6 nozzle";
    const std::string mixed = "SUNLU PETG White @Creality K2 Plus 0.6 nozzle";
    const fs::path directory = root / fs::u8path("用户预设");
    fs::create_directories(directory);

    // Reproduce the existing damaged state: uppercase filename, mixed-case owner.
    const fs::path original = directory / fs::u8path(policy::filename(upper));
    write(original, mixed, "mixed-id");
    const std::string recovered = policy::loaded_name(original.filename().u8string(), mixed, equivalent);
#ifdef _WIN32
    require(recovered == mixed, "legacy overwritten file recovered the wrong name");
#endif
    const fs::path restored_upper = choose(directory, upper);
    write(restored_upper, upper, "upper-id");
    require(read(original).at("name") == mixed, "restoring uppercase preset overwrote mixed-case preset");
    require(read(restored_upper).at("name") == upper, "uppercase preset was not restored");
    require(choose(directory, upper) == restored_upper, "repeat save changed the collision path");

    // Simulate restart: recover logical names, preserving independent .info files.
    std::set<std::string> names;
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.path().extension() != ".json") continue;
        const json data = read(entry.path());
        names.insert(policy::loaded_name(entry.path().filename().u8string(), data.at("name"), equivalent));
        fs::path info = entry.path();
        info.replace_extension(".info");
        boost::nowide::ifstream metadata(info.u8string());
        std::string id;
        metadata >> id;
        require(id == data.at("setting_id"), "JSON and info files have different owners");
    }
#ifdef _WIN32
    require(names == std::set<std::string>{upper, mixed}, "restart did not preserve both preset names");
#endif

    // A deliberate hash-filename collision must also be handled without overwrite.
    const std::string collision_name = "Other preset";
    write(directory / fs::u8path(policy::filename(collision_name)), "occupied", "one");
    write(directory / fs::u8path(policy::collision_stem(collision_name) + ".json"), "occupied", "two");
    const fs::path numbered = choose(directory, collision_name);
    require(numbered.filename().u8string() == policy::collision_stem(collision_name) + "_1.json", "fallback collision unresolved");
    require(policy::loaded_name(numbered.filename().u8string(), collision_name, equivalent) == collision_name,
            "numbered collision path changed the display name");
    require(policy::loaded_name("Manually renamed.json", "Original", equivalent) == "Manually renamed",
            "legacy manual rename behavior changed");
    const std::string long_name(240, 'x');
    require(policy::collision_stem(long_name).size() < 220, "collision filename is too long");
}

int main()
{
#ifdef _WIN32
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif
    const fs::path root = fs::temp_directory_path() /
        ("preset-sync-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    try {
        version_selection();
        filename_collisions(root);
        fs::remove_all(root);
        std::cout << "PASS cloud version ordering, filename collisions, metadata and restart names\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        fs::remove_all(root);
        return 1;
    }
}
