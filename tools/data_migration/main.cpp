#include "libslic3r/DataDirectoryMigration.hpp"
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#endif
int run(int argc, const char* const* argv)
{
    if (argc < 4 || argc > 5) { std::cerr << "Usage: data_directory_migrate TARGET RESOURCES TARGET_APP_VERSION [alpha]\n"; return 2; }
    try {
        Slic3r::DataMigration::Options o;
        o.target = std::filesystem::u8path(argv[1]); o.resources = std::filesystem::u8path(argv[2]);
        o.application_version = argv[3]; o.alpha = argc == 5 && std::string(argv[4]) == "alpha";
        o.validate_preset = Slic3r::DataMigration::validate_preset_readonly;
        const auto result = Slic3r::DataMigration::initialize(o);
        std::cout << "migrated=" << result.migrated << " source_version=" << result.source_version
                  << " valid_presets=" << result.presets << " quarantined=" << result.quarantined << '\n';
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
#ifdef _WIN32
int wmain(int argc, wchar_t** argv)
{
    std::vector<std::string> args; std::vector<const char*> pointers;
    for (int i = 0; i < argc; ++i) {
        int size = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, nullptr, 0, nullptr, nullptr);
        std::string value(size, '\0'); WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, value.data(), size, nullptr, nullptr);
        value.pop_back(); args.push_back(std::move(value));
    }
    for (const auto& value : args) pointers.push_back(value.c_str());
    return run(argc, pointers.data());
}
#else
int main(int argc, char** argv) { return run(argc, argv); }
#endif
