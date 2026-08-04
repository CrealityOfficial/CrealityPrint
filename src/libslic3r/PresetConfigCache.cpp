#include "PresetConfigCache.hpp"

#include "PrintConfig.hpp"
#include "Utils.hpp"
#include "libslic3r.h"
#include "libslic3r_version.h"

#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>
#include <boost/nowide/fstream.hpp>

#include <cereal/archives/binary.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
#include <filesystem>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <thread>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#endif

namespace Slic3r {
namespace {

constexpr uint32_t PRESET_CONFIG_CACHE_FORMAT_VERSION = 4;

uint64_t fnv1a_64(const std::string& value)
{
    uint64_t hash = 14695981039346656037ULL;
    for (unsigned char ch : value) {
        hash ^= ch;
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::string safe_filename(std::string value)
{
    for (char& ch : value) {
        const unsigned char uch = static_cast<unsigned char>(ch);
        if (!std::isalnum(uch) && ch != '-' && ch != '_')
            ch = '_';
    }
    return value.empty() ? "vendor" : value;
}

std::string normalized_absolute_path(const boost::filesystem::path& path)
{
    try {
        return boost::filesystem::absolute(path).make_preferred().string();
    } catch (...) {
        boost::filesystem::path fallback = path;
        return fallback.make_preferred().string();
    }
}

std::string file_stamp(const boost::filesystem::path& path)
{
    try {
        if (!boost::filesystem::is_regular_file(path))
            return "missing";
#ifdef _WIN32
        const std::filesystem::path timestamp_path(path.wstring());
#else
        const std::filesystem::path timestamp_path(path.string());
#endif
        const auto modified_at = static_cast<std::int64_t>(
            std::filesystem::last_write_time(timestamp_path).time_since_epoch().count());
        std::ostringstream stamp;
        stamp << boost::filesystem::file_size(path) << ':' << modified_at;
        return stamp.str();
    } catch (...) {
        return "unavailable";
    }
}

std::mutex                     s_cache_writers_mutex;
std::vector<std::future<void>> s_cache_writers;

void enqueue_cache_write(std::function<void()> write_task)
{
    std::lock_guard<std::mutex> lock(s_cache_writers_mutex);
    s_cache_writers.erase(
        std::remove_if(s_cache_writers.begin(), s_cache_writers.end(), [](std::future<void>& writer) {
            return writer.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
        }),
        s_cache_writers.end());
    s_cache_writers.emplace_back(std::async(std::launch::async, [task = std::move(write_task)]() mutable {
#ifdef _WIN32
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#endif
        task();
    }));
}

} // namespace

struct PresetConfigCache::Impl
{
    struct Entry {
        std::map<std::string, std::string> config_values;
        DynamicPrintConfig                 parsed_config;
        bool                               has_parsed_config = false;
        std::map<std::string, std::string> key_values;
        std::vector<std::string>           unrecognized_keys;

        template<class Archive> void serialize(Archive& archive)
        {
            archive(config_values, key_values, unrecognized_keys);
        }

        void prepare_for_save()
        {
            if (!config_values.empty() || !has_parsed_config)
                return;
            for (auto option = parsed_config.cbegin(); option != parsed_config.cend(); ++option)
                config_values.emplace(option->first, option->second->serialize());
        }
    };

    Impl(const boost::filesystem::path& source_root_path,
         const std::string& vendor,
         ForwardCompatibilitySubstitutionRule rule,
         bool cache_enabled,
         const boost::filesystem::path& requested_cache_root)
        : source_root(normalized_absolute_path(source_root_path))
        , vendor_name(vendor)
        , substitution_rule(static_cast<int>(rule))
        , enabled(cache_enabled)
    {
        if (!enabled)
            return;

        boost::filesystem::path root = requested_cache_root;
        if (root.empty()) {
            if (data_dir().empty()) {
                enabled = false;
                return;
            }
            root = boost::filesystem::path(data_dir()) / "cache" / "system_presets";
        }

        std::ostringstream identity;
        identity << source_root << '\0' << vendor_name << '\0' << substitution_rule;
        const boost::filesystem::path normalized_source_root(source_root);
        source_stamp = file_stamp(normalized_source_root.parent_path() / (vendor_name + ".json")) + '|' +
                       file_stamp(normalized_source_root / "profile_version.json");
        std::ostringstream hash;
        hash << std::hex << std::setfill('0') << std::setw(16) << fnv1a_64(identity.str());
        cache_file = root / (safe_filename(vendor_name) + "-" + hash.str() + ".cereal");
        load_document();
    }

    void load_document()
    {
        try {
            if (!boost::filesystem::is_regular_file(cache_file))
                return;
            boost::nowide::ifstream stream(cache_file.string(), std::ios::binary);
            if (!stream)
                return;

            uint32_t                     format_version = 0;
            std::string                  app_version;
            std::string                  cached_source_root;
            std::string                  cached_vendor;
            std::string                  cached_source_stamp;
            int                          cached_rule = -1;
            std::map<std::string, Entry> cached_entries;

            cereal::BinaryInputArchive archive(stream);
            archive(format_version);
            if (format_version != PRESET_CONFIG_CACHE_FORMAT_VERSION) {
                dirty = true;
                return;
            }

            archive(app_version, cached_source_root, cached_vendor, cached_source_stamp, cached_rule);
            if (app_version != SLIC3R_VERSION ||
                cached_source_root != source_root ||
                cached_vendor != vendor_name ||
                cached_source_stamp != source_stamp ||
                cached_rule != substitution_rule) {
                dirty = true;
                return;
            }

            archive(cached_entries);
            entries = std::move(cached_entries);
        } catch (const std::exception& error) {
            entries.clear();
            dirty = true;
            BOOST_LOG_TRIVIAL(warning) << "PresetConfigCache: ignoring invalid cache " << cache_file.string()
                                       << ": " << error.what();
        }
    }

    static void write_document(const boost::filesystem::path& cache_file,
                               const std::string& source_root,
                               const std::string& vendor_name,
                               const std::string& source_stamp,
                               int substitution_rule,
                               std::map<std::string, Entry> entries)
    {
        boost::filesystem::path temporary_file;
        try {
            for (auto& item : entries)
                item.second.prepare_for_save();

            boost::filesystem::create_directories(cache_file.parent_path());
            temporary_file = cache_file.parent_path() /
                boost::filesystem::unique_path(cache_file.filename().string() + ".%%%%-%%%%.tmp");
            {
                boost::nowide::ofstream stream(temporary_file.string(), std::ios::binary | std::ios::trunc);
                if (!stream)
                    throw std::runtime_error("could not create temporary cache file");
                cereal::BinaryOutputArchive archive(stream);
                const uint32_t format_version = PRESET_CONFIG_CACHE_FORMAT_VERSION;
                const std::string app_version = SLIC3R_VERSION;
                archive(format_version, app_version, source_root, vendor_name, source_stamp,
                        substitution_rule, entries);
            }

            if (boost::filesystem::exists(cache_file))
                boost::filesystem::remove(cache_file);
            boost::filesystem::rename(temporary_file, cache_file);
            BOOST_LOG_TRIVIAL(info) << "PresetConfigCache: saved " << entries.size()
                                    << " entries to " << cache_file.string();
        } catch (const std::exception& error) {
            if (!temporary_file.empty()) {
                boost::system::error_code ignored;
                boost::filesystem::remove(temporary_file, ignored);
            }
            BOOST_LOG_TRIVIAL(warning) << "PresetConfigCache: failed to save " << cache_file.string()
                                       << ": " << error.what();
        }
    }

    std::string                  source_root;
    std::string                  vendor_name;
    std::string                  source_stamp;
    int                          substitution_rule = -1;
    bool                         enabled = false;
    bool                         dirty = false;
    boost::filesystem::path      cache_file;
    std::map<std::string, Entry> entries;
    size_t                       hit_count = 0;
    size_t                       miss_count = 0;
};

PresetConfigCache::PresetConfigCache(const boost::filesystem::path& source_root,
                                     const std::string& vendor_name,
                                     ForwardCompatibilitySubstitutionRule substitution_rule,
                                     bool enabled,
                                     const boost::filesystem::path& cache_root)
    : m_impl(std::make_unique<Impl>(source_root, vendor_name, substitution_rule, enabled, cache_root))
{
}

PresetConfigCache::~PresetConfigCache() = default;
PresetConfigCache::PresetConfigCache(PresetConfigCache&&) noexcept = default;
PresetConfigCache& PresetConfigCache::operator=(PresetConfigCache&&) noexcept = default;

bool PresetConfigCache::load(const std::string& relative_path,
                             DynamicPrintConfig& config,
                             std::map<std::string, std::string>& key_values,
                             std::vector<std::string>& unrecognized_keys)
{
    if (!m_impl->enabled)
        return false;

    const auto found = m_impl->entries.find(relative_path);
    if (found == m_impl->entries.end()) {
        ++m_impl->miss_count;
        return false;
    }

    try {
        if (found->second.has_parsed_config) {
            config = found->second.parsed_config;
        } else {
            DynamicPrintConfig restored;
            ConfigSubstitutionContext context(ForwardCompatibilitySubstitutionRule::Disable);
            for (const auto& [key, value] : found->second.config_values)
                restored.set_deserialize(key, value, context);
            if (!context.empty())
                throw std::runtime_error("cached canonical values required compatibility substitutions");
            config = std::move(restored);
        }
        key_values = found->second.key_values;
        unrecognized_keys = found->second.unrecognized_keys;
    } catch (const std::exception& error) {
        BOOST_LOG_TRIVIAL(warning) << "PresetConfigCache: failed to restore " << relative_path
                                   << ": " << error.what();
        m_impl->entries.erase(found);
        m_impl->dirty = true;
        ++m_impl->miss_count;
        return false;
    }
    ++m_impl->hit_count;
    return true;
}

void PresetConfigCache::store(const std::string& relative_path,
                              const DynamicPrintConfig& config,
                              const std::map<std::string, std::string>& key_values,
                              const std::vector<std::string>& unrecognized_keys)
{
    if (!m_impl->enabled)
        return;

    Impl::Entry entry;
    entry.parsed_config = config;
    entry.has_parsed_config = true;
    entry.key_values = key_values;
    entry.unrecognized_keys = unrecognized_keys;
    m_impl->entries[relative_path] = std::move(entry);
    m_impl->dirty = true;
}

void PresetConfigCache::erase(const std::string& relative_path)
{
    if (m_impl->entries.erase(relative_path) > 0)
        m_impl->dirty = true;
}

void PresetConfigCache::save()
{
    if (!m_impl->enabled || !m_impl->dirty)
        return;
    Impl::write_document(m_impl->cache_file, m_impl->source_root, m_impl->vendor_name, m_impl->source_stamp,
                         m_impl->substitution_rule, std::move(m_impl->entries));
    m_impl->dirty = false;
}

void PresetConfigCache::save_async()
{
    if (!m_impl->enabled || !m_impl->dirty)
        return;
    const boost::filesystem::path cache_file = m_impl->cache_file;
    const std::string source_root = m_impl->source_root;
    const std::string vendor_name = m_impl->vendor_name;
    const std::string source_stamp = m_impl->source_stamp;
    const int substitution_rule = m_impl->substitution_rule;
    auto entries = std::move(m_impl->entries);
    m_impl->dirty = false;
    enqueue_cache_write([cache_file, source_root, vendor_name, source_stamp, substitution_rule,
                         entries = std::move(entries)]() mutable {
        Impl::write_document(cache_file, source_root, vendor_name, source_stamp,
                             substitution_rule, std::move(entries));
    });
}

size_t PresetConfigCache::hits() const { return m_impl->hit_count; }
size_t PresetConfigCache::misses() const { return m_impl->miss_count; }

} // namespace Slic3r
