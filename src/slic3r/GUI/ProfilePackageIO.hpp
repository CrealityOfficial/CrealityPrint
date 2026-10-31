#pragma once

#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <nlohmann/json.hpp>

namespace Slic3r::GUI::ProfilePackageIO {

inline bool matches(const std::filesystem::path& path, const nlohmann::json& content)
{
    std::ifstream input(path, std::ios::binary);
    return input && nlohmann::json::parse(input, nullptr, false) == content;
}

inline bool migrate_machine_model(const std::filesystem::path& legacy,
                                  const std::filesystem::path& target)
{
    if (std::filesystem::exists(target) || !std::filesystem::exists(legacy))
        return false;

    nlohmann::json model;
    {
        std::ifstream input(legacy, std::ios::binary);
        model = nlohmann::json::parse(input);
    } // Windows requires the read handle to be closed before rename.
    if (model.value("type", std::string()) != "machine_model")
        return false;
    std::filesystem::rename(legacy, target);
    return true;
}

inline bool validate_manifest(const std::filesystem::path& vendor_root,
                              const nlohmann::json& manifest,
                              std::string* reason = nullptr)
{
    const std::map<std::string, std::string> lists {
        {"machine_model_list", "machine_model"},
        {"machine_list", "machine"},
        {"filament_list", "filament"},
        {"process_list", "process"}
    };
    auto fail = [reason](const std::string& message) {
        if (reason != nullptr)
            *reason = message;
        return false;
    };
    if (!manifest.is_object())
        return fail("profile manifest is not an object");

    for (const auto& [list_name, expected_type] : lists) {
        const auto list = manifest.find(list_name);
        if (list == manifest.end())
            continue;
        if (!list->is_array())
            return fail(list_name + " is not an array");
        for (const auto& entry : *list) {
            if (!entry.is_object() || !entry.contains("sub_path") || !entry["sub_path"].is_string())
                return fail(list_name + " contains an entry without sub_path");
            const std::string sub_path = entry["sub_path"].get<std::string>();
            const auto file = vendor_root / std::filesystem::u8path(sub_path);
            std::ifstream input(file, std::ios::binary);
            if (!input)
                return fail("missing profile file: " + sub_path);
            const auto profile = nlohmann::json::parse(input, nullptr, false);
            if (profile.is_discarded() || !profile.is_object())
                return fail("invalid profile JSON: " + sub_path);
            if (profile.value("type", std::string()) != expected_type)
                return fail("unexpected profile type in " + sub_path + ": expected " + expected_type);
        }
    }
    return true;
}
} // namespace Slic3r::GUI::ProfilePackageIO
