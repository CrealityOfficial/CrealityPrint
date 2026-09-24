#include "BaseColorSpectraCache.hpp"

#include "libslic3r/ColorDecomposeKM.hpp"
#include "libslic3r/Utils.hpp"
#include "cr_km_recipe.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>
#include <boost/nowide/fstream.hpp>
#include <nlohmann/json.hpp>

#include <array>
#include <cmath>
#include <map>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace Slic3r { namespace GUI { namespace BaseColorSpectraCache {
namespace {

using Json = nlohmann::json;

constexpr const char* kComment =
    "Base color reflectance spectra for K-M mixing. Generated from the "
    "official/materialList Hyper PLA mixData; bundled data is used until a "
    "complete validated network response is available.";
constexpr const char* kGrid =
    "360,370,380,390,400,410,420,430,440,450,460,470,480,490,500,510,520,"
    "530,540,550,560,570,580,590,600,610,620,630,640,650,660,670,680,690,"
    "700,710,720,730,740,750,760,770,780";

const std::array<std::pair<const char*, const char*>, 8> kRecipeColors{{
    {"CMYW", "Cyan"}, {"CMYW", "Magenta"}, {"CMYW", "Yellow"}, {"CMYW", "White"},
    {"RYBW", "Red"}, {"RYBW", "Yellow"}, {"RYBW", "Blue"}, {"RYBW", "White"}
}};

const std::map<std::string, std::string> kSourceColors{
    {"Cyan", "Blue"}, {"Magenta", "Viva Magenta"}, {"Yellow", "Yellow"},
    {"White", "White"}, {"Red", "Red"}, {"Blue", "Blue"}
};

const std::map<std::string, std::string> kFilamentIds{
    {"White", "3301010335"}, {"Yellow", "3301010379"},
    {"Magenta", "3301010413"}, {"Cyan", "3301010341"},
    {"Blue", "3301010341"}, {"Red", "3301010342"}
};

std::mutex& cache_mutex()
{
    static std::mutex mutex;
    return mutex;
}

boost::filesystem::path cache_root()
{
    return boost::filesystem::path(data_dir()) / "filament_mixing";
}

boost::filesystem::path cache_path()
{
    return cache_root() / "base_color_spectra.json";
}

boost::filesystem::path bundled_path()
{
    return boost::filesystem::path(resources_dir()) / "filament_mixing" / "base_color_spectra.json";
}

std::string text(const Json& item, const char* key)
{
    return item.contains(key) && item[key].is_string() ? item[key].get<std::string>() : std::string();
}

void validate(const Json& root)
{
    if (!root.is_object() || !root.contains("entries") || !root["entries"].is_array() ||
        root["entries"].size() != kRecipeColors.size())
        throw std::runtime_error("Base spectrum cache must contain exactly eight entries");

    std::set<std::pair<std::string, std::string>> found;
    for (const Json& entry : root["entries"]) {
        if (!entry.is_object() || text(entry, "material") != "Hyper PLA")
            throw std::runtime_error("Invalid base spectrum material");
        const std::string mode = text(entry, "mode");
        const std::string base_color = text(entry, "base_color");
        if (!found.emplace(mode, base_color).second)
            throw std::runtime_error("Duplicate base spectrum entry");
        if (!entry.contains("spectrum") || !entry["spectrum"].is_array() ||
            entry["spectrum"].size() != KM_SPECTRUM_POINTS)
            throw std::runtime_error("Base spectrum must contain 43 points");
        for (const Json& value : entry["spectrum"]) {
            if (!value.is_number()) throw std::runtime_error("Non-numeric base spectrum value");
            const double number = value.get<double>();
            if (!std::isfinite(number) || number < 0.0 || number > 100.0)
                throw std::runtime_error("Base spectrum value out of range");
        }
        const std::string hex = text(entry, "official_hex");
        if (hex.size() != 7 || hex.front() != '#' ||
            hex.find_first_not_of("0123456789ABCDEFabcdef", 1) != std::string::npos)
            throw std::runtime_error("Invalid base spectrum official_hex");
    }
    for (const auto& required : kRecipeColors) {
        if (!found.count({required.first, required.second}))
            throw std::runtime_error("Missing base spectrum entry");
    }
}

Json read_validated(const boost::filesystem::path& path)
{
    boost::nowide::ifstream stream(path.string());
    if (!stream) throw std::runtime_error("Cannot open " + path.string());
    Json root;
    stream >> root;
    validate(root);
    return root;
}

void write_atomic(const Json& root, const boost::filesystem::path& target)
{
    validate(root);
    boost::filesystem::create_directories(target.parent_path());
    const std::string temporary = target.string() + ".tmp";
    try {
        boost::nowide::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        if (!stream) throw std::runtime_error("Cannot create base spectrum cache temporary file");
        stream << root.dump(2) << '\n';
        stream.flush();
        if (!stream) throw std::runtime_error("Cannot write base spectrum cache");
        stream.close();
        if (!stream) throw std::runtime_error("Cannot close base spectrum cache");
        const auto error = Slic3r::rename_file(temporary, target.string());
        if (error) throw std::runtime_error(error.message());
    } catch (...) {
        boost::system::error_code error;
        boost::filesystem::remove(temporary, error);
        throw;
    }
}

ReflectanceSpectrum parse_mix_data(const Json& color, const std::string& expected_source)
{
    std::vector<std::string> fields;
    boost::split(fields, text(color, "mixData"), boost::is_any_of(","));
    if (fields.size() != 6 + KM_SPECTRUM_POINTS)
        throw std::runtime_error("Invalid mixData field count for " + expected_source);
    for (std::string& field : fields) boost::trim(field);
    if (fields[1] != expected_source)
        throw std::runtime_error("mixData color mismatch for " + expected_source);
    const std::string material_id = text(color, "id");
    if (material_id.empty() || fields[0] != material_id)
        throw std::runtime_error("mixData material ID mismatch for " + expected_source);

    const auto parse_number = [&](size_t index) {
        size_t consumed = 0;
        const double value = std::stod(fields[index], &consumed);
        if (consumed != fields[index].size() || !std::isfinite(value))
            throw std::runtime_error("Invalid mixData number for " + expected_source);
        return value;
    };
    if (std::abs(parse_number(2) - 1.0) > 1e-9)
        throw std::runtime_error("Unsupported mixData scale for " + expected_source);
    for (size_t i = 3; i < 6; ++i) parse_number(i); // Validate Lab metadata.

    ReflectanceSpectrum spectrum{};
    for (size_t i = 0; i < spectrum.size(); ++i) {
        spectrum[i] = parse_number(6 + i);
        if (spectrum[i] < 0.0 || spectrum[i] > 100.0)
            throw std::runtime_error("mixData spectrum value out of range for " + expected_source);
    }
    return spectrum;
}

std::string spectrum_hex(const ReflectanceSpectrum& spectrum)
{
    return rgb_to_hex(lab_to_srgb(xyz_to_lab(reflectance_to_xyz(spectrum))));
}

Json convert(const Json& material_cache)
{
    if (!material_cache.is_object() || !material_cache.contains("data") ||
        !material_cache["data"].is_array())
        throw std::runtime_error("Invalid official material color cache");

    std::map<std::string, ReflectanceSpectrum> spectra;
    for (const Json& color : material_cache["data"]) {
        if (!color.is_object() || text(color, "name") != "Hyper PLA") continue;
        const std::string color_name = text(color, "colorEn");
        bool required = false;
        for (const auto& mapping : kSourceColors) required = required || mapping.second == color_name;
        if (!required || text(color, "mixData").empty()) continue;
        if (spectra.count(color_name))
            throw std::runtime_error("Duplicate Hyper PLA mixData for " + color_name);
        spectra.emplace(color_name, parse_mix_data(color, color_name));
    }

    Json entries = Json::array();
    for (const auto& recipe_color : kRecipeColors) {
        const std::string mode = recipe_color.first;
        const std::string base_color = recipe_color.second;
        const std::string source_color = kSourceColors.at(base_color);
        const auto spectrum_it = spectra.find(source_color);
        if (spectrum_it == spectra.end())
            throw std::runtime_error("Missing Hyper PLA mixData for " + source_color);
        const ReflectanceSpectrum& spectrum = spectrum_it->second;
        entries.push_back({
            {"mode", mode}, {"material", "Hyper PLA"}, {"base_color", base_color},
            {"official_hex", spectrum_hex(spectrum)},
            {"filament_id", kFilamentIds.at(base_color)},
            {"spectrum", std::vector<double>(spectrum.begin(), spectrum.end())}
        });
    }
    return {{"_comment", kComment}, {"_units", "percent (0-100)"},
            {"_grid", kGrid}, {"source", "official/materialList"},
            {"entries", std::move(entries)}};
}

void activate_cache_root()
{
    const std::string root = data_dir();
    cr_close_set_resources_dir(root.c_str());
}

} // namespace

bool initialize()
{
    std::lock_guard<std::mutex> lock(cache_mutex());
    try {
        try {
            read_validated(cache_path());
        } catch (const std::exception&) {
            const Json bundled = read_validated(bundled_path());
            write_atomic(bundled, cache_path());
            BOOST_LOG_TRIVIAL(info) << "Base color spectrum cache initialized from resources";
        }
        activate_cache_root();
        return true;
    } catch (const std::exception& e) {
        // Keep the original resources root installed by CrealityPrint.cpp.
        BOOST_LOG_TRIVIAL(warning) << "Base color spectrum cache initialization failed: " << e.what();
        return false;
    }
}

bool update_from_material_cache(const nlohmann::json& material_cache)
{
    try {
        const Json updated = convert(material_cache);
        std::lock_guard<std::mutex> lock(cache_mutex());
        write_atomic(updated, cache_path());
        activate_cache_root();
        BOOST_LOG_TRIVIAL(info) << "Base color spectrum cache updated from official material list";
        return true;
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(warning) << "Keeping previous base color spectrum cache: " << e.what();
        return false;
    }
}

}}} // namespace Slic3r::GUI::BaseColorSpectraCache
