#pragma once

#include "BaseColorSpectraCache.hpp"
#include "GUI.hpp"
#include "GUI_App.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r_version.h"
#include "slic3r/Utils/Http.hpp"
#include <boost/filesystem.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/algorithm/string.hpp>
#include <nlohmann/json.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/log/trivial.hpp>
#include <vector>
#include <mutex>
#include <future>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

namespace Slic3r { namespace GUI {
namespace OfficialMaterialColors {
using Json = nlohmann::json;

inline boost::filesystem::path cache_path()
{
    return boost::filesystem::path(data_dir()) / "system" / "Creality" / "filament" / "filaments_color.json";
}

inline std::mutex& cache_mutex()
{
    static std::mutex mutex;
    return mutex;
}

inline std::string text(const Json& item, const char* key)
{
    return item.contains(key) && item[key].is_string() ? item[key].get<std::string>() : std::string();
}

inline std::string localized_color_name(const Json& color, const std::string& language)
{
    std::string locale = language;
    boost::trim(locale);
    boost::replace_all(locale, "-", "_");
    boost::to_lower(locale);
    const auto separator = locale.find('_');
    const std::string code = locale.substr(0, separator);

    // The two Chinese locales have separate backend fields; all other supported
    // locales use their two-letter language code. Missing names fall back to English.
    static const std::map<std::string, const char*> fields = {
        {"zh_cn", "colorCn"}, {"zh_tw", "colorZh"},
        {"en", "colorEn"}, {"de", "colorDe"}, {"fr", "colorFr"},
        {"it", "colorIt"}, {"es", "colorEs"}, {"pt", "colorPt"},
        {"ja", "colorJa"}, {"ko", "colorKo"}, {"nl", "colorNl"},
        {"sv", "colorSv"}, {"hu", "colorHu"}
    };
    const auto it = fields.find(code == "zh" ? locale : code);
    if (it != fields.end()) {
        std::string value = text(color, it->second);
        boost::trim(value);
        if (!value.empty()) return value;
    }
    std::string english = text(color, "colorEn");
    boost::trim(english);
    return english;
}

inline std::string normalize_hex(std::string value)
{
    boost::algorithm::trim(value);
    boost::algorithm::to_upper(value);
    if (value.size() != 9 || value[0] != '#' ||
        value.find_first_not_of("0123456789ABCDEF", 1) != std::string::npos)
        throw std::runtime_error("Invalid official RGBA color");
    return value;
}

inline Json normalize_colors(const Json& input)
{
    Json out = Json::array();
    const auto append = [&](const std::string& value) {
        std::vector<std::string> parts;
        boost::split(parts, value, boost::is_any_of(","));
        for (auto part : parts) out.push_back(normalize_hex(part));
    };
    if (input.is_string()) append(input.get<std::string>());
    else if (input.is_array()) {
        for (const auto& value : input) {
            if (!value.is_string()) throw std::runtime_error("Invalid color array");
            append(value.get<std::string>());
        }
    } else throw std::runtime_error("Missing color values");
    if (out.empty()) throw std::runtime_error("Empty color values");
    return out;
}

// Validate structure and colour values, independently of release versions.
inline void validate_cache(const Json& cache)
{
    if (!cache.is_object() || !cache.contains("data") || !cache["data"].is_array() || cache["data"].empty())
        throw std::runtime_error("Missing color cache data");
    for (const auto& item : cache["data"]) {
        if (!item.is_object() || text(item, "id").empty())
            throw std::runtime_error("Missing material ID in color cache");
        normalize_colors(item.at("colors"));
    }
}

inline bool initialize_from_resources()
{
    std::lock_guard<std::mutex> lock(cache_mutex());

    const auto target = cache_path();
    // Keep installation bookkeeping outside system/Creality: preset updates replace that directory.
    const auto marker = boost::filesystem::path(data_dir()) / "filaments_color_initialized_version.txt";
    static bool initialized_this_session = false;
    const bool cache_exists = boost::filesystem::exists(target);
    if (cache_exists && initialized_this_session) return true;
    std::string initialized_version;
    {
        boost::nowide::ifstream input(marker.string());
        std::getline(input, initialized_version);
    }
    if (cache_exists && initialized_version == SLIC3R_VERSION) {
        initialized_this_session = true;
        return true;
    }

    const auto bundled = boost::filesystem::path(resources_dir()) / "profiles" / "Creality" /
        "filament" / "filaments_color.json";
    const auto temporary = target.string() + ".tmp";
    const auto marker_temporary = marker.string() + ".tmp";
    try {
        boost::nowide::ifstream input(bundled.string());
        if (!input) throw std::runtime_error("Bundled color cache not found");
        Json cache;
        input >> cache;
        validate_cache(cache);

        boost::filesystem::create_directories(target.parent_path());
        {
            boost::nowide::ofstream output(temporary, std::ios::binary | std::ios::trunc);
            output << cache.dump(2);
            output.flush();
            if (!output) throw std::runtime_error("Cannot write bundled color cache");
            output.close();
            if (!output) throw std::runtime_error("Cannot close bundled color cache");
        }
        const auto error = Slic3r::rename_file(temporary, target.string());
        if (error) throw std::runtime_error(error.message());
        initialized_this_session = true;
        // Commit the marker only after successfully replacing the validated cache.
        {
            boost::nowide::ofstream output(marker_temporary, std::ios::binary | std::ios::trunc);
            output << SLIC3R_VERSION << '\n';
            output.close();
            if (!output) throw std::runtime_error("Cannot write color cache initialization version");
        }
        const auto marker_error = Slic3r::rename_file(marker_temporary, marker.string());
        if (marker_error) throw std::runtime_error(marker_error.message());
        BOOST_LOG_TRIVIAL(info) << "Official material color cache initialized from resources: "
                                << cache["data"].size();
        return true;
    } catch (const std::exception& e) {
        boost::system::error_code error;
        boost::filesystem::remove(temporary, error);
        boost::filesystem::remove(marker_temporary, error);
        BOOST_LOG_TRIVIAL(warning) << "Bundled official color cache unavailable: " << e.what();
        return false;
    }
}

// Identity is material ID plus the complete, ordered RGBA list. No SKU is invented.
inline Json convert(const Json& materials)
{
    if (!materials.is_array() || materials.empty()) throw std::runtime_error("Empty material list");
    Json entries = Json::array();
    std::set<std::string> keys;
    for (const auto& material : materials) {
        if (!material.is_object()) throw std::runtime_error("Invalid material");
        if (!boost::iequals(text(material, "brand"), "Creality")) continue;
        const std::string id = text(material, "id");
        if (id.empty()) throw std::runtime_error("Missing material ID");
        const auto add = [&](const Json& color) {
            const Json* raw_colors = color.contains("hexValue") ? &color.at("hexValue") :
                                     color.contains("colors") ? &color.at("colors") : nullptr;
            bool has_color = false;
            if (raw_colors && raw_colors->is_string()) {
                std::string value = raw_colors->get<std::string>();
                boost::trim(value);
                has_color = !value.empty();
            } else if (raw_colors && raw_colors->is_array()) {
                for (const auto& value : *raw_colors) {
                    if (!value.is_string()) continue;
                    std::string candidate = value.get<std::string>();
                    boost::trim(candidate);
                    if (!candidate.empty()) { has_color = true; break; }
                }
            }
            if (!has_color) {
                BOOST_LOG_TRIVIAL(warning) << "Skipping official material color without HEX value: "
                                           << id << " (" << text(material, "name") << ")";
                return;
            }
            Json rgb;
            try {
                rgb = normalize_colors(*raw_colors);
            } catch (const std::exception& e) {
                throw std::runtime_error("Material " + id + " (" + text(material, "name") +
                    "), color " + text(color, "colorEn") + ": " + e.what());
            }
            const std::string key = id + ":" + rgb.dump();
            if (!keys.insert(key).second) return;
            Json entry = {{"id", id}, {"name", text(material, "name")}, {"colors", rgb},
                {"mixData", text(color, "mixData")},
                {"mixSpecialAttr", text(color, "mixSpecialAttr")}};
            for (const char* field : {"colorCn", "colorEn", "colorDe", "colorFr", "colorIt",
                                      "colorEs", "colorPt", "colorJa", "colorKo", "colorNl",
                                      "colorSv", "colorHu", "colorZh"}) {
                std::string value = text(color, field);
                boost::trim(value);
                if (!value.empty()) entry[field] = std::move(value);
            }
            entries.push_back(std::move(entry));
        };
        if (material.contains("mixColorInfoList") && material["mixColorInfoList"].is_array() &&
            !material["mixColorInfoList"].empty()) {
            for (const auto& color : material["mixColorInfoList"]) add(color);
        } else if (material.contains("colorList") && material["colorList"].is_array()) {
            if (material["colorList"].empty()) throw std::runtime_error("Empty color list");
            for (const auto& color : material["colorList"]) add(color);
        } else if (material.contains("colors") && material["colors"].is_array() &&
                   !material["colors"].empty() && material["colors"].front().is_object()) {
            for (const auto& color : material["colors"]) add(color);
        } else add(material);
    }
    if (entries.empty()) throw std::runtime_error("No Creality colors");
    return {{"source", "official/materialList"}, {"data", entries}};
}

inline std::string api_base_url()
{
    return get_cloud_api_url();
}

inline void refresh(const std::string& base_url, const std::map<std::string, std::string>& headers)
{
    // Serialize requests without blocking readers while waiting for the network.
    static std::mutex request_mutex;
    std::lock_guard<std::mutex> request_lock(request_mutex);
    // Keep bundled data available if the startup request fails.
    initialize_from_resources();
    try {
        constexpr size_t page_size = 1000;
        Json materials = Json::array();
        std::set<std::string> material_ids;
        int64_t expected_count = -1;
        for (size_t page = 1; ; ++page) {
            Json response;
            std::string request_error;
            Http request = Http::post(base_url + "/api/cxy/v2/slice/profile/official/materialList");
            for (const auto& header : headers) request.header(header.first, header.second);
            request
                .header("Content-Type", "application/json")
                .header("__CXY_REQUESTID_", boost::uuids::to_string(boost::uuids::random_generator()()))
                .timeout_connect(3).timeout_max(15)
                .set_post_body(Json({{"engineVersion", "3.0.0"}, {"pageSize", page_size}, {"page", page}}).dump())
                .on_complete([&](std::string body, unsigned status) {
                    if (status != 200) {
                        request_error = "HTTP " + std::to_string(status);
                        return;
                    }
                    try { response = Json::parse(body); }
                    catch (const std::exception& e) { request_error = e.what(); }
                })
                .on_error([&](std::string, std::string error, unsigned status) {
                    request_error = "HTTP " + std::to_string(status) + ": " + error;
                }).perform_sync();

            if (!request_error.empty()) throw std::runtime_error(request_error);
            if (!response.is_object() || response.value("code", -1) != 0)
                throw std::runtime_error("Invalid material list response");
            const auto& result = response.at("result");
            const auto& list = result.at("list");
            const auto& count = result.at("count");
            if (!list.is_array() || !count.is_number_integer())
                throw std::runtime_error("Invalid material list pagination");
            const int64_t current_count = count.get<int64_t>();
            if (current_count <= 0 || (expected_count >= 0 && current_count != expected_count) || list.empty())
                throw std::runtime_error("Incomplete material list");
            expected_count = current_count;
            for (const auto& material : list) {
                if (!material.is_object() || text(material, "id").empty() ||
                    !material_ids.insert(text(material, "id")).second)
                    throw std::runtime_error("Duplicate or invalid material page");
                materials.push_back(material);
            }
            if (materials.size() > static_cast<uint64_t>(expected_count))
                throw std::runtime_error("Material list exceeds count");
            if (materials.size() == static_cast<uint64_t>(expected_count)) break;
        }

        const Json cache = convert(materials);
        validate_cache(cache);
        {
            std::lock_guard<std::mutex> write_lock(cache_mutex());
            const auto target = cache_path();
            const auto temporary = target.string() + ".tmp";
            boost::filesystem::create_directories(target.parent_path());
            {
                boost::nowide::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
                stream << cache.dump(2);
                stream.flush();
                if (!stream) throw std::runtime_error("Cannot write color cache");
                stream.close();
                if (!stream) throw std::runtime_error("Cannot close color cache");
            }
            const auto error = Slic3r::rename_file(temporary, target.string());
            if (error) throw std::runtime_error(error.message());
        }
        BOOST_LOG_TRIVIAL(info) << "Official material color cache updated: " << cache["data"].size();
        BaseColorSpectraCache::update_from_material_cache(cache);
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(warning) << "Keeping previous official color cache: " << e.what();
    }
}
inline void refresh_async()
{
    // Startup is the only caller; guard against a second launch path in this process.
    static std::future<void> request;
    static std::once_flag once;
    std::call_once(once, [] {
        // Snapshot application settings on the UI thread. Workers never retain a dialog.
        const auto url = api_base_url();
        const auto headers = wxGetApp().get_extra_header();
        request = std::async(std::launch::async, [url, headers] {
            refresh(url, headers);
        });
    });
}
}}
}
