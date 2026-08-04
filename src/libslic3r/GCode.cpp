#include "BoundingBox.hpp"
#include "Config.hpp"
#include "Polygon.hpp"
#include "PrintConfig.hpp"
#include "libslic3r.h"
#include "I18N.hpp"
#include "GCode.hpp"
#include "Exception.hpp"
#include "ExtrusionEntity.hpp"
#include "EdgeGrid.hpp"
#include "Geometry/ConvexHull.hpp"
#include "GCode/PrintExtents.hpp"
#include "GCode/Thumbnails.hpp"
#include "GCode/WipeTower.hpp"
#include "libslic3r/FDM/MachineVender.hpp"
#include "libslic3r/ModelInstance.hpp"
#include "libslic3r/ModelVolume.hpp"
#include "ShortestPath.hpp"
#include "Print.hpp"
#include "Utils.hpp"
#include "ClipperUtils.hpp"
#include "libslic3r.h"
#include "LocalesUtils.hpp"
#include "libslic3r/format.hpp"
#include "Time.hpp"
#include "GCode/ExtrusionProcessor.hpp"
#include "GCode/AppearanceUnderExtrusionAccelRecoveryFilter.hpp"
#include "GCode/InterestRegion.hpp"
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <chrono>
#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <string>
#include <utility>
#include <unordered_map>
#include <string_view>

#include <regex>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>
#include <boost/beast/core/detail/base64.hpp>

#include <boost/nowide/iostream.hpp>
#include <boost/nowide/cstdio.hpp>
#include <boost/nowide/cstdlib.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#ifdef SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
#include <cctype>
#include <nlohmann/json.hpp>
#include <cstdio>
#include <ctime>
#include <boost/nowide/fstream.hpp>
#endif

#include "SVG.hpp"

#include <tbb/parallel_for.h>
#include "calib.hpp"
// Intel redesigned some TBB interface considerably when merging TBB with their oneAPI set of libraries, see GH #7332.
// We are using quite an old TBB 2017 U7. Before we update our build servers, let's use the old API, which is deprecated in up to date TBB.
#if ! defined(TBB_VERSION_MAJOR)
    #include <tbb/version.h>
#endif
#if ! defined(TBB_VERSION_MAJOR)
    static_assert(false, "TBB_VERSION_MAJOR not defined");
#endif
#if TBB_VERSION_MAJOR >= 2021
    #include <tbb/parallel_pipeline.h>
    using slic3r_tbb_filtermode = tbb::filter_mode;
#else
    #include <tbb/pipeline.h>
    using slic3r_tbb_filtermode = tbb::filter;
#endif

#include <Shiny/Shiny.h>

#include "miniz_extension.hpp"

using namespace std::literals::string_view_literals;

#if 0
// Enable debugging and asserts, even in the release build.
#define DEBUG
#define _DEBUG
#undef NDEBUG
#endif

#include <assert.h>
#include "libslic3r/GCode/Smoothing.hpp"

namespace Slic3r {

    //debug
    void bench_debug_seamplacer(Print* print, SeamPlacer* placer);
    void bench_debug_generate(Print* print, int layer, const std::string& code, bool by_object = false);
    void bench_debug_cooling(Print* print, int layer, const std::string& code, bool by_object = false);
    void bench_debug_fanmove(Print* print, int layer, const std::string& code, bool by_object = false);
    void bench_debug_output(Print* print, int layer, const std::string& code, bool by_object = false);
    //debug

    //! macro used to mark string used at localization,
    //! return same string
#define L(s) (s)
#define _(s) Slic3r::I18N::translate(s)

static const float g_min_purge_volume = 100.f;
static const float g_purge_volume_one_time = 135.f;
static const int g_max_flush_count = 5;


// static const size_t g_max_label_object = 64;
static const double smooth_speed_step = 10;
#ifdef SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
static bool g_enable_layer_time_auto_export = true;
#ifdef _WIN32
// Manual path interface (no env dependency):
// - g_slicer_layer_time_export_root: root for <job_id>.slicer_layer_time.jsonl and job meta.
// - g_prepare_compare_export_root: root for <job_id>.slicer_prepare_compare.json.
static fs::path g_slicer_layer_time_export_root("C:\\Users\\118388\\Desktop\\i7-dev\\output");
static fs::path g_prepare_compare_export_root("C:\\Users\\118388\\Desktop\\i7-dev\\output");
#else
static fs::path g_slicer_layer_time_export_root("/tmp/k2plus-dev/output");
static fs::path g_prepare_compare_export_root("/tmp/k2plus-dev/output");
#endif

static std::string analytics_role_key(ExtrusionRole role)
{
    switch (role) {
        case erPerimeter:                return "inner_wall";
        case erExternalPerimeter:        return "outer_wall";
        case erOverhangPerimeter:        return "overhang_wall";
        case erInternalInfill:           return "sparse_infill";
        case erSolidInfill:              return "internal_solid_infill";
        case erTopSolidInfill:           return "top_surface";
        case erBottomSurface:            return "bottom_surface";
        case erIroning:                  return "ironing";
        case erBridgeInfill:             return "bridge";
        case erInternalBridgeInfill:     return "internal_bridge";
        case erGapFill:                  return "gap_infill";
        case erSkirt:                    return "skirt";
        case erBrim:                     return "brim";
        case erSupportMaterial:          return "support";
        case erSupportMaterialInterface: return "support_interface";
        case erSupportTransition:        return "support_transition";
        case erWipeTower:                return "wipe_tower";
        case erCustom:                   return "custom";
        case erNone:
        case erMixed:
        default:                         return "other";
    }
}

static std::string analytics_move_key(EMoveType move_type)
{
    switch (move_type) {
        case EMoveType::Travel:    return "travel";
        case EMoveType::Retract:   return "retract";
        case EMoveType::Unretract: return "unretract";
        case EMoveType::Wipe:      return "wipe";
        default:                                   return "other";
    }
}

static std::string layer_time_seconds_to_hms(double seconds)
{
    const long long total_seconds = static_cast<long long>(std::max(0.0, std::floor(seconds)));
    const int hours = static_cast<int>(total_seconds / 3600);
    const int minutes = static_cast<int>((total_seconds % 3600) / 60);
    const int remain_seconds = static_cast<int>(total_seconds % 60);
    char time_str[16] = { 0 };
    std::snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", hours, minutes, remain_seconds);
    return time_str;
}

static std::string layer_time_now_local_string()
{
    const std::time_t now = std::time(nullptr);
    std::tm tm_now{};
#ifdef _WIN32
    localtime_s(&tm_now, &now);
#else
    localtime_r(&now, &tm_now);
#endif
    char time_str[20] = { 0 };
    std::snprintf(time_str, sizeof(time_str), "%04d-%02d-%02d %02d:%02d:%02d",
                  tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
                  tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);
    return time_str;
}

static std::string layer_time_new_run_batch_id()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t now_seconds = std::chrono::system_clock::to_time_t(now);
    const long long millis = static_cast<long long>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000);
    std::tm tm_now{};
#ifdef _WIN32
    localtime_s(&tm_now, &now_seconds);
#else
    localtime_r(&now_seconds, &tm_now);
#endif
    char batch_id[32] = { 0 };
    std::snprintf(batch_id, sizeof(batch_id), "%04d%02d%02d_%02d%02d%02d_%03lld",
                  tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
                  tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec, millis);
    return batch_id;
}

static double round_layer_time_ms(double seconds)
{
    return std::round(seconds * 1000.0) / 1000.0;
}

static const std::vector<ExtrusionRole>& analytics_role_order()
{
    static const std::vector<ExtrusionRole> order = {
        erPerimeter,
        erExternalPerimeter,
        erOverhangPerimeter,
        erInternalInfill,
        erSolidInfill,
        erTopSolidInfill,
        erBottomSurface,
        erIroning,
        erBridgeInfill,
        erInternalBridgeInfill,
        erGapFill,
        erSkirt,
        erBrim,
        erSupportMaterial,
        erSupportMaterialInterface,
        erSupportTransition,
        erWipeTower,
        erCustom,
        erNone
    };
    return order;
}

static const std::vector<EMoveType>& analytics_move_order()
{
    static const std::vector<EMoveType> order = {
        EMoveType::Travel,
        EMoveType::Retract,
        EMoveType::Unretract,
        EMoveType::Wipe
    };
    return order;
}

static nlohmann::json build_role_time_json(
    const std::vector<std::pair<ExtrusionRole, float>>& role_items,
    double custom_adjust_s,
    double* total_time_s = nullptr)
{
    using json = nlohmann::json;
    std::map<ExtrusionRole, double> role_time_map;
    for (const auto& item : role_items)
        role_time_map[item.first] += std::max(0.0, static_cast<double>(item.second));
    if (custom_adjust_s > 0.0)
        role_time_map[erCustom] = std::max(0.0, role_time_map[erCustom] - custom_adjust_s);

    double total = 0.0;
    json payload = json::object();
    for (const ExtrusionRole role : analytics_role_order()) {
        const double value = std::max(0.0, role_time_map[role]);
        payload[analytics_role_key(role)] = round_layer_time_ms(value);
        total += value;
    }
    if (total_time_s != nullptr)
        *total_time_s = total;
    return payload;
}

static nlohmann::json build_move_time_json(
    const std::vector<std::pair<EMoveType, float>>& move_items,
    double* total_time_s = nullptr)
{
    using json = nlohmann::json;
    std::map<EMoveType, double> move_time_map;
    for (const auto& item : move_items)
        move_time_map[item.first] += std::max(0.0, static_cast<double>(item.second));

    double total = 0.0;
    json payload = json::object();
    for (const auto move_type : analytics_move_order()) {
        const double value = std::max(0.0, move_time_map[move_type]);
        payload[analytics_move_key(move_type)] = round_layer_time_ms(value);
        total += value;
    }
    if (total_time_s != nullptr)
        *total_time_s = total;
    return payload;
}


static fs::path layer_time_output_root_path()
{
    return g_slicer_layer_time_export_root;
}

static fs::path prepare_compare_output_root_path()
{
    return g_prepare_compare_export_root;
}

static bool extract_creality_uuid_hex_lower(const fs::path& file_path, std::string& out_job_id)
{
    if (!fs::exists(file_path) || fs::is_directory(file_path))
        return false;

    boost::filesystem::ifstream ifs(file_path.string(), std::ios::binary);
    if (!ifs.is_open())
        return false;

    std::string line;
    for (int line_idx = 0; line_idx < 20 && std::getline(ifs, line); ++line_idx) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        const std::size_t non_space_pos = line.find_first_not_of(" 	");
        const std::string_view trimmed =
            (non_space_pos == std::string::npos) ? std::string_view() : std::string_view(line).substr(non_space_pos);
        static constexpr std::string_view prefix = "; creality_uuid:";
        if (trimmed.rfind(prefix, 0) != 0)
            continue;

        std::string raw_job_id(trimmed.substr(prefix.size()));
        const std::size_t value_begin = raw_job_id.find_first_not_of(" 	");
        if (value_begin == std::string::npos)
            return false;
        const std::size_t value_end = raw_job_id.find_last_not_of(" 	");
        raw_job_id = raw_job_id.substr(value_begin, value_end - value_begin + 1);

        std::string normalized_job_id;
        normalized_job_id.reserve(raw_job_id.size());
        for (char ch : raw_job_id) {
            if (ch == '-')
                continue;
            normalized_job_id.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        }

        if (normalized_job_id.size() != 32)
            return false;
        for (char ch : normalized_job_id) {
            if (!std::isxdigit(static_cast<unsigned char>(ch)))
                return false;
        }
        out_job_id = std::move(normalized_job_id);
        return true;
    }
    return false;
}

static bool is_layer_time_job_id_hex(const std::string& job_id)
{
    if (job_id.size() != 32)
        return false;
    for (const char ch : job_id) {
        if (!std::isxdigit(static_cast<unsigned char>(ch)))
            return false;
    }
    return true;
}

static bool parse_layer_time_job_dir_name(const std::string& dir_name, int& seq, std::string& job_id)
{
    const std::size_t split_pos = dir_name.find('_');
    if (split_pos == std::string::npos || split_pos == 0 || split_pos + 1 >= dir_name.size())
        return false;

    const std::string seq_str = dir_name.substr(0, split_pos);
    if (!std::all_of(seq_str.begin(), seq_str.end(), [](char ch) { return std::isdigit(static_cast<unsigned char>(ch)); }))
        return false;

    try {
        seq = std::stoi(seq_str);
    } catch (...) {
        return false;
    }
    if (seq <= 0)
        return false;

    job_id = dir_name.substr(split_pos + 1);
    std::transform(job_id.begin(), job_id.end(), job_id.begin(),
        [](char ch) { return static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); });
    if (!is_layer_time_job_id_hex(job_id))
        return false;
    return true;
}

struct LayerTimeRunContext
{
    fs::path    run_dir;
    std::string run_name;
    int         run_seq { 0 };
    std::string run_tag;
    std::string run_batch_id;
    int         run_batch_seq { 1 };
    int         run_batch_size { 1 };
    std::string export_mode;
};

static bool is_layer_time_run_tag_char(char ch)
{
    const unsigned char uch = static_cast<unsigned char>(ch);
    return std::isalnum(uch) || ch == '_' || ch == '-' || ch == '.';
}

static std::string sanitize_layer_time_run_tag(const std::string& raw_tag)
{
    std::string tag;
    tag.reserve(raw_tag.size());
    bool last_was_sep = false;
    for (const char ch : raw_tag) {
        if (is_layer_time_run_tag_char(ch)) {
            tag.push_back(ch);
            last_was_sep = false;
        } else if (!last_was_sep && !tag.empty()) {
            tag.push_back('_');
            last_was_sep = true;
        }
    }
    while (!tag.empty() && tag.back() == '_')
        tag.pop_back();
    return tag.empty() ? std::string("default") : tag;
}

static std::string format_layer_time_run_dir_name(int seq, const std::string& tag)
{
    char seq_str[16] = { 0 };
    std::snprintf(seq_str, sizeof(seq_str), "%03d", std::max(1, seq));
    return std::string(seq_str) + "_" + sanitize_layer_time_run_tag(tag);
}

static bool parse_layer_time_run_dir_name(const std::string& dir_name, int& seq, std::string& tag)
{
    const std::size_t split_pos = dir_name.find('_');
    if (split_pos == std::string::npos || split_pos == 0 || split_pos + 1 >= dir_name.size())
        return false;

    const std::string seq_str = dir_name.substr(0, split_pos);
    if (!std::all_of(seq_str.begin(), seq_str.end(), [](char ch) { return std::isdigit(static_cast<unsigned char>(ch)); }))
        return false;

    try {
        seq = std::stoi(seq_str);
    } catch (...) {
        return false;
    }
    if (seq <= 0)
        return false;

    tag = dir_name.substr(split_pos + 1);
    if (tag.empty() || !std::all_of(tag.begin(), tag.end(), is_layer_time_run_tag_char))
        return false;
    return true;
}

static bool resolve_next_layer_time_run_dir(
    const fs::path& job_dir,
    const std::string& raw_tag,
    const std::string& run_batch_id,
    int run_batch_seq,
    int run_batch_size,
    const std::string& export_mode,
    LayerTimeRunContext& out_run)
{
    if (job_dir.empty())
        return false;

    const std::string run_tag = sanitize_layer_time_run_tag(raw_tag);
    try {
        if (!fs::exists(job_dir))
            fs::create_directories(job_dir);
        else if (!fs::is_directory(job_dir))
            return false;

        int max_seq = 0;
        for (fs::directory_iterator it(job_dir), end; it != end; ++it) {
            const fs::path entry_path = it->path();
            if (!fs::is_directory(entry_path))
                continue;
            int entry_seq = 0;
            std::string entry_tag;
            if (!parse_layer_time_run_dir_name(entry_path.filename().string(), entry_seq, entry_tag))
                continue;
            max_seq = std::max(max_seq, entry_seq);
        }

        int run_seq = std::max(1, max_seq + 1);
        fs::path run_dir;
        std::string run_name;
        do {
            run_name = format_layer_time_run_dir_name(run_seq, run_tag);
            run_dir = job_dir / run_name;
            if (!fs::exists(run_dir))
                break;
            ++run_seq;
        } while (true);

        fs::create_directories(run_dir);
        if (!fs::is_directory(run_dir))
            return false;

        out_run.run_dir = run_dir;
        out_run.run_name = run_name;
        out_run.run_seq = run_seq;
        out_run.run_tag = run_tag;
        out_run.run_batch_id = run_batch_id.empty() ? layer_time_new_run_batch_id() : run_batch_id;
        out_run.run_batch_seq = std::max(1, run_batch_seq);
        out_run.run_batch_size = std::max(1, run_batch_size);
        out_run.export_mode = export_mode.empty() ? std::string("default") : export_mode;
        return true;
    } catch (...) {
        return false;
    }
}

static void append_layer_time_run_metadata(nlohmann::json& item, const LayerTimeRunContext& run_context)
{
    item["run_name"] = run_context.run_name;
    item["run_seq"] = run_context.run_seq;
    item["run_tag"] = run_context.run_tag;
    item["run_batch_id"] = run_context.run_batch_id;
    item["run_batch_seq"] = run_context.run_batch_seq;
    item["run_batch_size"] = run_context.run_batch_size;
    item["export_mode"] = run_context.export_mode;
}

static bool resolve_layer_time_job_dir(
    const fs::path& output_root,
    const std::string& raw_job_id,
    fs::path& job_dir,
    std::string& job_dir_name,
    int& job_seq)
{
    if (output_root.empty() || raw_job_id.empty())
        return false;

    std::string job_id = raw_job_id;
    std::transform(job_id.begin(), job_id.end(), job_id.begin(),
        [](char ch) { return static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); });
    if (!is_layer_time_job_id_hex(job_id))
        return false;

    try {
        if (!fs::exists(output_root))
            fs::create_directories(output_root);
        else if (!fs::is_directory(output_root))
            return false;

        int      max_seq = 0;
        int      found_seq = 0;
        fs::path found_dir;
        for (fs::directory_iterator it(output_root), end; it != end; ++it) {
            const fs::path entry_path = it->path();
            if (!fs::is_directory(entry_path))
                continue;
            int         entry_seq = 0;
            std::string entry_job_id;
            if (!parse_layer_time_job_dir_name(entry_path.filename().string(), entry_seq, entry_job_id))
                continue;
            max_seq = std::max(max_seq, entry_seq);
            if (entry_job_id == job_id) {
                if (found_dir.empty() || entry_seq < found_seq) {
                    found_dir = entry_path;
                    found_seq = entry_seq;
                }
            }
        }

        if (!found_dir.empty()) {
            job_dir = found_dir;
            job_dir_name = found_dir.filename().string();
            job_seq = found_seq;
            return true;
        }

        job_seq = std::max(1, max_seq + 1);
        job_dir_name = std::to_string(job_seq) + "_" + job_id;
        job_dir = output_root / job_dir_name;
        if (!fs::exists(job_dir))
            fs::create_directories(job_dir);
        return true;
    } catch (...) {
        return false;
    }
}

static fs::path ensure_job_dir_in_root(const fs::path& output_root, const std::string& job_dir_name)
{
    if (output_root.empty() || job_dir_name.empty())
        return fs::path();

    try {
        if (!fs::exists(output_root))
            fs::create_directories(output_root);
        else if (!fs::is_directory(output_root))
            return fs::path();

        const fs::path job_dir = output_root / job_dir_name;
        if (!fs::exists(job_dir))
            fs::create_directories(job_dir);
        else if (!fs::is_directory(job_dir))
            return fs::path();
        return job_dir;
    } catch (...) {
        return fs::path();
    }
}

static bool build_layer_time_job_context(
    const std::string& gcode_path,
    const std::string& gcode_tmp_path,
    const std::string& preferred_job_id,
    std::string& job_id,
    std::string& gcode_file_name,
    std::string& job_dir_name,
    int& job_seq,
    fs::path& job_dir)
{
    fs::path uuid_source_path;
    if (!gcode_tmp_path.empty() && fs::exists(gcode_tmp_path))
        uuid_source_path = fs::path(gcode_tmp_path);
    else if (!gcode_path.empty() && fs::exists(gcode_path))
        uuid_source_path = fs::path(gcode_path);
    else
        return false;

    if (!preferred_job_id.empty()) {
        std::string normalized_job_id;
        normalized_job_id.reserve(preferred_job_id.size());
        for (char ch : preferred_job_id) {
            if (ch == '-')
                continue;
            normalized_job_id.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        }
        if (!is_layer_time_job_id_hex(normalized_job_id))
            return false;
        job_id = std::move(normalized_job_id);
    } else if (!extract_creality_uuid_hex_lower(uuid_source_path, job_id)) {
        return false;
    }

    const fs::path gcode_file_path(gcode_path);
    gcode_file_name = gcode_file_path.filename().string();
    return resolve_layer_time_job_dir(layer_time_output_root_path(), job_id, job_dir, job_dir_name, job_seq);
}

static fs::path export_slicer_layer_time_jsonl(
    const LayerTimeRunContext& run_context,
    const std::string& job_id,
    const std::string& job_dir_name,
    int job_seq,
    const std::string& gcode_file_name,
    const GCodeProcessorResult& result,
    const std::string& export_reason)
{
    using json = nlohmann::json;
    if (run_context.run_dir.empty() || job_id.empty())
        return fs::path();

    const auto& normal_mode = result.print_statistics.modes[static_cast<size_t>(PrintEstimatedStatistics::ETimeMode::Normal)];
    const size_t layer_count = std::max(
        std::max(normal_mode.layers_times.size(), normal_mode.layer_roles_times.size()),
        normal_mode.layer_moves_times.size());
    if (layer_count == 0) {
        BOOST_LOG_TRIVIAL(info) << "skip slicer layer_time export: empty layer analytics";
        return fs::path();
    }

    const fs::path layer_time_path = run_context.run_dir / (job_id + ".slicer_layer_time.jsonl");
    boost::nowide::ofstream layer_time_file(layer_time_path.string(), std::ios::out | std::ios::trunc);
    if (!layer_time_file.is_open()) {
        BOOST_LOG_TRIVIAL(error) << boost::format("open slicer layer time file failed: %1%") % layer_time_path.string();
        return fs::path();
    }

    static const std::vector<std::pair<ExtrusionRole, float>> kEmptyRoleItems;
    static const std::vector<std::pair<EMoveType, float>> kEmptyMoveItems;
    double cumulative_time = 0.0;
    for (size_t idx = 0; idx < layer_count; ++idx) {
        const double layer_time_s = idx < normal_mode.layers_times.size()
            ? std::max(0.0, static_cast<double>(normal_mode.layers_times[idx]))
            : 0.0;
        const auto& role_items = idx < normal_mode.layer_roles_times.size()
            ? normal_mode.layer_roles_times[idx]
            : kEmptyRoleItems;
        const auto& move_items = idx < normal_mode.layer_moves_times.size()
            ? normal_mode.layer_moves_times[idx]
            : kEmptyMoveItems;

        double role_total_s = 0.0;
        double move_total_s = 0.0;
        json role_time = build_role_time_json(role_items, 0.0, &role_total_s);
        json move_time = build_move_time_json(move_items, &move_total_s);
        const double unattributed_time_s = std::max(0.0, layer_time_s - role_total_s - move_total_s);
        cumulative_time += layer_time_s;

        json item = {
            {"schema_version", 1},
            {"source", "slicer"},
            {"timing_origin", "estimated_total_time"},
            {"print_id", "slicer_" + job_id},
            {"job_id", job_id},
            {"job_dir", job_dir_name},
            {"job_seq", job_seq},
            {"gcode_file", gcode_file_name},
            {"layer", static_cast<int>(idx + 1)},
            {"layer_time_s", round_layer_time_ms(layer_time_s)},
            {"layer_time_hms", layer_time_seconds_to_hms(layer_time_s)},
            {"print_duration_s", round_layer_time_ms(cumulative_time)},
            {"print_duration_hms", layer_time_seconds_to_hms(cumulative_time)},
            {"role_time_s", std::move(role_time)},
            {"move_type_time_s", std::move(move_time)},
            {"unattributed_time_s", round_layer_time_ms(unattributed_time_s)},
            {"reason", export_reason.empty() ? "slice_export" : export_reason},
            {"record_time", layer_time_now_local_string()}
        };
        append_layer_time_run_metadata(item, run_context);
        layer_time_file << item.dump() << '\n';
    }
    layer_time_file.flush();
    BOOST_LOG_TRIVIAL(info) << boost::format("slicer_layer_time_file path:%1% lines:%2% source:slicer.estimated_total_time")
                                % layer_time_path.string() % layer_count;
    return layer_time_path;
}

static fs::path export_custom_role_and_prepare_time(
    const fs::path& job_dir,
    const std::string& job_id,
    const std::string& gcode_path,
    const GCodeProcessorResult& result)
{
    using json = nlohmann::json;
    if (job_dir.empty() || job_id.empty())
        return fs::path();

    const auto& normal_mode = result.print_statistics.modes[static_cast<size_t>(PrintEstimatedStatistics::ETimeMode::Normal)];

    double custom_role_time_s = 0.0;
    for (const auto& role_item : normal_mode.roles_times) {
        if (role_item.first == erCustom) {
            custom_role_time_s = std::max(0.0, static_cast<double>(role_item.second));
            break;
        }
    }
    const double prepare_time_s = std::max(0.0, static_cast<double>(normal_mode.prepare_time));
    const double delta_s = custom_role_time_s - prepare_time_s;

    const fs::path gcode_file_path(gcode_path);
    const fs::path export_file = job_dir / (job_id + ".slicer_prepare_compare.json");

    json item = {
        {"job_id", job_id},
        {"record_time", layer_time_now_local_string()},
        {"gcode_file", gcode_file_path.filename().string()},
        {"gcode_path", gcode_path},
        {"roles_time_custom_s", round_layer_time_ms(custom_role_time_s)},
        {"roles_time_custom_hms", layer_time_seconds_to_hms(custom_role_time_s)},
        {"prepare_time_s", round_layer_time_ms(prepare_time_s)},
        {"prepare_time_hms", layer_time_seconds_to_hms(prepare_time_s)},
        {"delta_custom_minus_prepare_s", round_layer_time_ms(delta_s)}
    };

    boost::nowide::ofstream out_file(export_file.string(), std::ios::out | std::ios::trunc);
    if (!out_file.is_open()) {
        BOOST_LOG_TRIVIAL(error) << boost::format("open custom/prepare export file failed: %1%") % export_file.string();
        return fs::path();
    }
    out_file << item.dump(2) << '\n';
    out_file.flush();

    BOOST_LOG_TRIVIAL(info) << boost::format("custom_prepare_time_file path:%1% custom:%2% prepare:%3%")
                                % export_file.string()
                                % round_layer_time_ms(custom_role_time_s)
                                % round_layer_time_ms(prepare_time_s);
    return export_file;
}

static fs::path export_slicer_time_summary_json(
    const LayerTimeRunContext& run_context,
    const std::string& job_id,
    const std::string& job_dir_name,
    int job_seq,
    const std::string& gcode_file_name,
    const GCodeProcessorResult& result,
    const std::string& export_reason)
{
    using json = nlohmann::json;
    if (run_context.run_dir.empty() || job_id.empty())
        return fs::path();

    const auto& normal_mode = result.print_statistics.modes[static_cast<size_t>(PrintEstimatedStatistics::ETimeMode::Normal)];
    const size_t layer_count = std::max(
        std::max(normal_mode.layers_times.size(), normal_mode.layer_roles_times.size()),
        normal_mode.layer_moves_times.size());
    if (layer_count == 0)
        return fs::path();

    const fs::path summary_path = run_context.run_dir / (job_id + ".slicer_time_summary.json");
    double total_print_duration_s = 0.0;
    for (const double layer_time : normal_mode.layers_times)
        total_print_duration_s += std::max(0.0, static_cast<double>(layer_time));

    double total_role_time_s = 0.0;
    double total_move_time_s = 0.0;
    json total_role_time = build_role_time_json(normal_mode.roles_times, 0.0, &total_role_time_s);
    json total_move_time = build_move_time_json(normal_mode.moves_times, &total_move_time_s);
    const double total_unattributed_time_s = std::max(0.0, total_print_duration_s - total_role_time_s - total_move_time_s);

    json item = {
        {"schema_version", 1},
        {"source", "slicer"},
        {"timing_origin", "estimated_total_time"},
        {"print_id", "slicer_" + job_id},
        {"job_id", job_id},
        {"job_dir", job_dir_name},
        {"job_seq", job_seq},
        {"gcode_file", gcode_file_name},
        {"total_layers", static_cast<int>(layer_count)},
        {"total_print_duration_s", round_layer_time_ms(total_print_duration_s)},
        {"total_print_duration_hms", layer_time_seconds_to_hms(total_print_duration_s)},
        {"total_role_time_s", std::move(total_role_time)},
        {"total_move_type_time_s", std::move(total_move_time)},
        {"total_unattributed_time_s", round_layer_time_ms(total_unattributed_time_s)},
        {"prepare_time_s", round_layer_time_ms(std::max(0.0, static_cast<double>(normal_mode.prepare_time)))},
        {"prepare_time_hms", layer_time_seconds_to_hms(std::max(0.0, static_cast<double>(normal_mode.prepare_time)))},
        {"reason", export_reason.empty() ? "slice_export" : export_reason},
        {"record_time", layer_time_now_local_string()}
    };
    append_layer_time_run_metadata(item, run_context);

    boost::nowide::ofstream out_file(summary_path.string(), std::ios::out | std::ios::trunc);
    if (!out_file.is_open()) {
        BOOST_LOG_TRIVIAL(error) << boost::format("open slicer time summary file failed: %1%") % summary_path.string();
        return fs::path();
    }
    out_file << item.dump(2) << '\n';
    out_file.flush();
    BOOST_LOG_TRIVIAL(info) << boost::format("slicer_time_summary_file path:%1% total:%2%")
                                % summary_path.string()
                                % round_layer_time_ms(total_print_duration_s);
    return summary_path;
}

static fs::path export_slicer_thumbnail_png_from_gcode_result(
    const fs::path& job_dir,
    const std::string& job_id,
    const GCodeProcessorResult& result)
{
    if (job_dir.empty() || job_id.empty() || result.image_data.empty())
        return fs::path();

    for (auto rit = result.image_data.rbegin(); rit != result.image_data.rend(); ++rit) {
        const int width = rit->first[0];
        const int height = rit->first[1];
        const auto& image_bytes = rit->second;
        if (width <= 0 || height <= 0 || image_bytes.empty())
            continue;

        const fs::path thumbnail_file = job_dir / (job_id + ".slicer_thumbnail.png");
        boost::nowide::ofstream out(thumbnail_file.string(), std::ios::out | std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            BOOST_LOG_TRIVIAL(error) << boost::format("open slicer thumbnail file failed: %1%") % thumbnail_file.string();
            return fs::path();
        }
        out.write(reinterpret_cast<const char*>(image_bytes.data()), static_cast<std::streamsize>(image_bytes.size()));
        out.flush();
        BOOST_LOG_TRIVIAL(info) << boost::format("slicer_thumbnail_file path:%1% size:%2%x%3% source:gcode.image_data")
                                    % thumbnail_file.string() % width % height;
        return thumbnail_file;
    }

    BOOST_LOG_TRIVIAL(info) << "skip slicer thumbnail export: no valid gcode image_data";
    return fs::path();
}

static void export_slicer_job_meta(
    const fs::path& job_dir,
    const std::string& job_id,
    const std::string& job_dir_name,
    int job_seq,
    const std::string& gcode_path,
    const fs::path& slicer_layer_time_file,
    const fs::path& slicer_time_summary_file,
    const fs::path& prepare_compare_file,
    const fs::path& thumbnail_file,
    const LayerTimeRunContext& run_context)
{
    using json = nlohmann::json;
    if (job_dir.empty() || job_id.empty())
        return;

    const fs::path meta_file = job_dir / (job_id + ".job_meta.json");
    json item = {
        {"job_id", job_id},
        {"job_dir", job_dir_name},
        {"job_seq", job_seq},
        {"record_time", layer_time_now_local_string()},
        {"gcode_path", gcode_path},
        {"gcode_file", fs::path(gcode_path).filename().string()},
        {"layer_time_auto_export_enabled", g_enable_layer_time_auto_export},
        {"slicer_layer_time_file", slicer_layer_time_file.empty() ? "" : slicer_layer_time_file.filename().string()},
        {"slicer_layer_time_path", slicer_layer_time_file.empty() ? "" : slicer_layer_time_file.string()},
        {"slicer_time_summary_file", slicer_time_summary_file.empty() ? "" : slicer_time_summary_file.filename().string()},
        {"slicer_time_summary_path", slicer_time_summary_file.empty() ? "" : slicer_time_summary_file.string()},
        {"prepare_compare_file", prepare_compare_file.empty() ? "" : prepare_compare_file.filename().string()},
        {"prepare_compare_path", prepare_compare_file.empty() ? "" : prepare_compare_file.string()},
        {"thumbnail_file", thumbnail_file.empty() ? "" : thumbnail_file.filename().string()},
        {"thumbnail_path", thumbnail_file.empty() ? "" : thumbnail_file.string()},
        {"latest_run_name", run_context.run_name},
        {"latest_run_dir", run_context.run_dir.empty() ? "" : run_context.run_dir.string()},
        {"latest_run_seq", run_context.run_seq},
        {"latest_run_tag", run_context.run_tag},
        {"latest_export_mode", run_context.export_mode},
        {"latest_run_batch_id", run_context.run_batch_id},
        {"latest_run_batch_seq", run_context.run_batch_seq},
        {"latest_run_batch_size", run_context.run_batch_size},
        {"schema_version", 5}
    };

    boost::nowide::ofstream out(meta_file.string(), std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        BOOST_LOG_TRIVIAL(error) << boost::format("open slicer job meta file failed: %1%") % meta_file.string();
        return;
    }
    out << item.dump(2) << '\n';
    out.flush();
    BOOST_LOG_TRIVIAL(info) << boost::format("slicer_job_meta_file path:%1%") % meta_file.string();
}

bool export_layer_time_analytics_for_loaded_gcode(const std::string& gcode_path, const GCodeProcessorResult& result)
{
    if (!g_enable_layer_time_auto_export) {
        BOOST_LOG_TRIVIAL(info) << "skip layer-time auto export on gcode load: g_enable_layer_time_auto_export=false";
        return false;
    }

    std::string job_id;
    std::string gcode_file_name;
    std::string job_dir_name;
    int         job_seq = 0;
    fs::path    job_dir;
    if (!build_layer_time_job_context(gcode_path, "", result.gcode_uuid, job_id, gcode_file_name, job_dir_name, job_seq, job_dir)) {
        BOOST_LOG_TRIVIAL(error) << boost::format("skip layer-time auto export on gcode load: build job context failed for %1%") % gcode_path;
        return false;
    }

    LayerTimeRunContext run_context;
    if (!resolve_next_layer_time_run_dir(
            job_dir,
            "default",
            layer_time_new_run_batch_id(),
            1,
            1,
            "default",
            run_context)) {
        BOOST_LOG_TRIVIAL(error) << boost::format("skip layer-time auto export on gcode load: create run dir failed for %1%") % job_dir.string();
        return false;
    }

    const fs::path prepare_root = prepare_compare_output_root_path();
    fs::path       prepare_job_dir = ensure_job_dir_in_root(prepare_root, job_dir_name);
    if (prepare_job_dir.empty())
        prepare_job_dir = job_dir;

    BOOST_LOG_TRIVIAL(info) << boost::format(
        "time_analytics_export_root layer_time:%1% prepare_compare:%2%")
        % layer_time_output_root_path().string()
        % prepare_root.string();

    const fs::path slicer_layer_time_file = export_slicer_layer_time_jsonl(
        run_context, job_id, job_dir_name, job_seq, gcode_file_name, result, "gcode_load");
    const fs::path slicer_time_summary_file = export_slicer_time_summary_json(
        run_context, job_id, job_dir_name, job_seq, gcode_file_name, result, "gcode_load");
    const fs::path prepare_compare_file = export_custom_role_and_prepare_time(
        prepare_job_dir, job_id, gcode_path, result);
    const fs::path thumbnail_file = export_slicer_thumbnail_png_from_gcode_result(
        job_dir, job_id, result);
    export_slicer_job_meta(
        job_dir, job_id, job_dir_name, job_seq, gcode_path, slicer_layer_time_file, slicer_time_summary_file, prepare_compare_file, thumbnail_file, run_context);
    return !slicer_layer_time_file.empty();
}
#else
bool export_layer_time_analytics_for_loaded_gcode(const std::string& gcode_path, const GCodeProcessorResult& result)
{
    (void)gcode_path;
    (void)result;
    return false;
}
#endif // SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT

Vec2d travel_point_1;
Vec2d travel_point_2;
Vec2d travel_point_3;

static std::vector<unsigned int> collect_rendered_extruder_ids(const GCodeProcessorResult &result)
{
    std::vector<unsigned int> extruder_ids;
    for (size_t i = 0; i < result.rendered_extruder_used.size(); ++i) {
        if (result.rendered_extruder_used[i])
            extruder_ids.push_back(static_cast<unsigned int>(i));
    }
    return extruder_ids;
}

// === Mixed Filament Helper Functions ===

static std::vector<unsigned int> decode_manual_pattern_sequence_for_gcode(const MixedFilament& mf, size_t num_physical)
{
    std::vector<unsigned int> sequence;
    if (mf.manual_pattern.empty())
        return sequence;
    sequence.reserve(mf.manual_pattern.size());
    for (const char token : mf.manual_pattern) {
        unsigned int extruder_id = 0;
        if (token == '1')
            extruder_id = mf.component_a;
        else if (token == '2')
            extruder_id = mf.component_b;
        else if (token >= '3' && token <= '9')
            extruder_id = unsigned(token - '0');
        if (extruder_id >= 1 && extruder_id <= num_physical)
            sequence.emplace_back(extruder_id);
    }
    return sequence;
}

static std::vector<unsigned int> decode_gradient_component_ids_for_gcode(const MixedFilament& mf, size_t num_physical)
{
    std::vector<unsigned int> ids;
    if (mf.gradient_component_ids.empty() || num_physical == 0)
        return ids;
    bool seen[10] = { false };
    ids.reserve(mf.gradient_component_ids.size());
    for (const char c : mf.gradient_component_ids) {
        if (c < '1' || c > '9')
            continue;
        const unsigned int id = unsigned(c - '0');
        if (id == 0 || id > num_physical || seen[id])
            continue;
        seen[id] = true;
        ids.emplace_back(id);
    }
    return ids;
}

static std::vector<int> decode_gradient_component_weights_for_gcode(const MixedFilament& mf, size_t expected_components)
{
    std::vector<int> out;
    if (mf.gradient_component_weights.empty() || expected_components == 0)
        return out;
    std::string token;
    for (const char c : mf.gradient_component_weights) {
        if (c >= '0' && c <= '9') {
            token.push_back(c);
            continue;
        }
        if (!token.empty()) {
            out.emplace_back(std::max(0, std::atoi(token.c_str())));
            token.clear();
        }
    }
    if (!token.empty())
        out.emplace_back(std::max(0, std::atoi(token.c_str())));
    if (out.size() != expected_components)
        return {};
    return out;
}

static std::vector<unsigned int> build_weighted_gradient_sequence_for_gcode(const std::vector<unsigned int>& ids,
                                                                            const std::vector<int>&          weights)
{
    if (ids.empty())
        return {};

    std::vector<unsigned int> filtered_ids;
    std::vector<int>          counts;
    filtered_ids.reserve(ids.size());
    counts.reserve(ids.size());
    for (size_t i = 0; i < ids.size(); ++i) {
        const int w = (i < weights.size()) ? std::max(0, weights[i]) : 0;
        if (w <= 0)
            continue;
        filtered_ids.emplace_back(ids[i]);
        counts.emplace_back(w);
    }
    if (filtered_ids.empty()) {
        filtered_ids = ids;
        counts.assign(ids.size(), 1);
    }

    int g = 0;
    for (const int c : counts)
        g = std::gcd(g, std::max(1, c));
    if (g > 1) {
        for (int &c : counts)
            c = std::max(1, c / g);
    }

    int cycle = std::accumulate(counts.begin(), counts.end(), 0);
    constexpr int k_max_cycle = 48;
    if (cycle > k_max_cycle) {
        const double scale = double(k_max_cycle) / double(cycle);
        for (int &c : counts)
            c = std::max(1, int(std::round(double(c) * scale)));
        cycle = std::accumulate(counts.begin(), counts.end(), 0);
        while (cycle > k_max_cycle) {
            auto it = std::max_element(counts.begin(), counts.end());
            *it = std::max(1, *it - 1);
            cycle = std::accumulate(counts.begin(), counts.end(), 0);
        }
    }

    std::vector<unsigned int> sequence;
    sequence.reserve(cycle);
    for (size_t i = 0; i < filtered_ids.size(); ++i) {
        for (int k = 0; k < counts[i]; ++k)
            sequence.emplace_back(filtered_ids[i]);
    }
    return sequence;
}

static std::vector<unsigned int> pointillism_sequence_for_row_for_gcode(const MixedFilament& mf, size_t num_physical)
{
    if (!mf.enabled || num_physical == 0 || mf.distribution_mode != int(MixedFilament::SameLayerPointillisme))
        return {};
    std::vector<unsigned int> out;
    if (mf.manual_pattern.empty())
        return out;
    out.reserve(mf.manual_pattern.size());
    for (const char token : mf.manual_pattern) {
        unsigned int extruder_id = 0;
        if (token == '1')
            extruder_id = mf.component_a;
        else if (token == '2')
            extruder_id = mf.component_b;
        else if (token >= '3' && token <= '9')
            extruder_id = unsigned(token - '0');
        if (extruder_id >= 1 && extruder_id <= num_physical)
            out.emplace_back(extruder_id);
    }
    return out;
}

// === End Mixed Filament Helper Functions ===

static std::vector<std::string> build_render_aligned_default_string_values(
    const ConfigBase &cfg, const std::vector<unsigned int> &rendered_extruders, const std::string &source_key)
{
    const auto *values_opt = cfg.option<ConfigOptionStrings>(source_key);
    if (values_opt == nullptr)
        return {};

    std::vector<std::string> aligned_values(values_opt->values.size());
    for (const unsigned int extruder_id : rendered_extruders) {
        if (extruder_id < values_opt->values.size())
            aligned_values[extruder_id] = values_opt->values[extruder_id];
    }
    return aligned_values;
}

static std::string serialize_string_values(const std::string &key, const std::vector<std::string> &values)
{
    DynamicPrintConfig temp_config;
    temp_config.set_key_value(key, new ConfigOptionStrings(values));
    return temp_config.opt_serialize(key);
}

static bool find_config_block_start_from_tail(std::istream &input, std::streamoff &offset)
{
    static const std::string marker = "; CONFIG_BLOCK_START";
    static constexpr std::streamoff block_size = 64 * 1024;
    static constexpr size_t max_tail_search_size = 1024 * 1024;

    input.clear();
    input.seekg(0, std::ios::end);
    std::streamoff read_end = input.tellg();
    if (read_end <= 0)
        return false;
    std::string tail_buffer;
    while (read_end > 0 && tail_buffer.size() < max_tail_search_size) {
        const std::streamoff remaining_limit =
            static_cast<std::streamoff>(max_tail_search_size - tail_buffer.size());
        const std::streamoff read_size =
            std::min(block_size, std::min(read_end, remaining_limit));

        read_end -= read_size;

        std::string chunk(static_cast<size_t>(read_size), '\0');
        input.clear();
        input.seekg(read_end, std::ios::beg);
        input.read(&chunk[0], static_cast<std::streamsize>(read_size));
        if (input.gcount() != static_cast<std::streamsize>(read_size))
            return false;

        tail_buffer.insert(0, chunk);

        const size_t pos = tail_buffer.rfind(marker);
        if (pos != std::string::npos) {
            offset = read_end + static_cast<std::streamoff>(pos);
            return true;
        }
    }

    return false;
}

static bool rewrite_config_block_tail_values(
    const std::string &path, const std::vector<std::pair<std::string, std::string>> &replacements)
{
    if (replacements.empty())
        return true;

    boost::nowide::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        BOOST_LOG_TRIVIAL(warning) << "Failed to open G-code file for default filament metadata rewrite, skip rewrite";
        return true;
    }

    std::streamoff config_block_offset = -1;
    if (!find_config_block_start_from_tail(input, config_block_offset)) {
        BOOST_LOG_TRIVIAL(warning) << "CONFIG_BLOCK_START not found in tail, skip default filament metadata rewrite";
        return true;
    }

    input.clear();
    input.seekg(config_block_offset, std::ios::beg);
    std::string tail_text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    input.close();

    std::istringstream tail_stream(tail_text);
    std::string rewritten_tail;
    std::string line;
    bool inside_config_block = false;
    while (std::getline(tail_stream, line)) {
        std::string trimmed = line;
        if (!trimmed.empty() && trimmed.back() == '\r')
            trimmed.pop_back();

        if (trimmed == "; CONFIG_BLOCK_START")
            inside_config_block = true;

        if (inside_config_block) {
            for (const auto &replacement : replacements) {
                const std::string prefix = "; " + replacement.first + " = ";
                if (boost::starts_with(trimmed, prefix)) {
                    line = prefix + replacement.second;
                    break;
                }
            }
        }

        rewritten_tail += line;
        rewritten_tail += '\n';

        if (trimmed == "; CONFIG_BLOCK_END")
            inside_config_block = false;
    }

    boost::filesystem::resize_file(path, static_cast<uintmax_t>(config_block_offset));

    boost::nowide::ofstream output(path, std::ios::binary | std::ios::app);
    if (!output.is_open())
        return false;

    output.write(rewritten_tail.data(), static_cast<std::streamsize>(rewritten_tail.size()));
    output.flush();
    return !output.fail();
}

static void sync_default_filament_metadata_with_rendered_tools(
    const ConfigBase &cfg, const std::string &path, GCodeProcessorResult &result)
{
    const std::vector<unsigned int> rendered_extruders = collect_rendered_extruder_ids(result);
    const std::vector<std::string> render_aligned_colours =
        build_render_aligned_default_string_values(cfg, rendered_extruders, "filament_colour");
    const std::vector<std::string> render_aligned_types =
        build_render_aligned_default_string_values(cfg, rendered_extruders, "filament_type");

    std::vector<std::pair<std::string, std::string>> replacements;
    if (!render_aligned_colours.empty())
        replacements.emplace_back("default_filament_colour", serialize_string_values("default_filament_colour", render_aligned_colours));
    if (!render_aligned_types.empty())
        replacements.emplace_back("default_filament_type", serialize_string_values("default_filament_type", render_aligned_types));

    if (!rewrite_config_block_tail_values(path, replacements))
        throw RuntimeError("Failed to rewrite CONFIG_BLOCK tail for default filament metadata.");

    if (!render_aligned_colours.empty())
        result.creality_extruder_colors = render_aligned_colours;
}
static std::vector<Vec2d> get_path_of_change_filament(const Print& print)
{
    // give safe value in case there is no start_end_points in config
    std::vector<Vec2d> out_points;
    out_points.emplace_back(Vec2d(54, 0));
    out_points.emplace_back(Vec2d(54, 0));
    out_points.emplace_back(Vec2d(54, 245));

    // get the start_end_points from config (20, -3) (54, 245)
    Pointfs points = print.config().start_end_points.values;
    if (points.size() != 2)
        return out_points;

    Vec2d start_point  = points[0];
    Vec2d end_point    = points[1];

    // the cutter area size(18, 28)
    Pointfs excluse_area = print.config().bed_exclude_area.values;
    if (excluse_area.size() != 4)
        return out_points;

    double cutter_area_x = excluse_area[2].x() + 2;
    double cutter_area_y = excluse_area[2].y() + 2;

    double start_x_position = start_point.x();
    double end_x_position   = end_point.x();
    double end_y_position   = end_point.y();

    bool can_travel_form_left = true;

    // step 1: get the x-range intervals of all objects
    std::vector<std::pair<double, double>> object_intervals;
    for (PrintObject *print_object : print.objects()) {
        const PrintInstances &print_instances = print_object->instances();
        BoundingBoxf3 bounding_box = print_instances[0].model_instance->get_object()->bounding_box_exact();

        if (bounding_box.min.x() < start_x_position && bounding_box.min.y() < cutter_area_y)
            can_travel_form_left = false;

        std::pair<double, double> object_scope = std::make_pair(bounding_box.min.x() - 2, bounding_box.max.x() + 2);
        if (object_intervals.empty())
            object_intervals.push_back(object_scope);
        else {
            std::vector<std::pair<double, double>> new_object_intervals;
            bool intervals_intersect = false;
            std::pair<double, double>              new_merged_scope;
            for (auto object_interval : object_intervals) {
                if (object_interval.second >= object_scope.first && object_interval.first <= object_scope.second) {
                    if (intervals_intersect) {
                        new_merged_scope = std::make_pair(std::min(object_interval.first, new_merged_scope.first), std::max(object_interval.second, new_merged_scope.second));
                    } else { // it is the first intersection
                        new_merged_scope = std::make_pair(std::min(object_interval.first, object_scope.first), std::max(object_interval.second, object_scope.second));
                    }
                    intervals_intersect = true;
                } else {
                    new_object_intervals.push_back(object_interval);
                }
            }

            if (intervals_intersect) {
                new_object_intervals.push_back(new_merged_scope);
                object_intervals = new_object_intervals;
            } else
                object_intervals.push_back(object_scope);
        }
    }

    // step 2: get the available x-range
    std::sort(object_intervals.begin(), object_intervals.end(),
              [](const std::pair<double, double> &left, const std::pair<double, double> &right) {
            return left.first < right.first;
    });
    std::vector<std::pair<double, double>> available_intervals;
    double                                 start_position = 0;
    for (auto object_interval : object_intervals) {
        if (object_interval.first > start_position)
            available_intervals.push_back(std::make_pair(start_position, object_interval.first));
        start_position = object_interval.second;
    }
    available_intervals.push_back(std::make_pair(start_position, 255));

    // step 3: get the nearest path
    double new_path     = 255;
    for (auto available_interval : available_intervals) {
        if (available_interval.first > end_x_position) {
            double distance = available_interval.first - end_x_position;
            new_path        = abs(end_x_position - new_path) < distance ? new_path : available_interval.first;
            break;
        } else {
            if (available_interval.second >= end_x_position) {
                new_path = end_x_position;
                break;
            } else if (!can_travel_form_left && available_interval.second < start_x_position) {
                continue;
            } else {
                new_path     = available_interval.second;
            }
        }
    }

    // step 4: generate path points  (new_path == start_x_position means not need to change path)
    Vec2d out_point_1;
    Vec2d out_point_2;
    Vec2d out_point_3;
    if (new_path < start_x_position) {
        out_point_1 = Vec2d(start_x_position, cutter_area_y);
        out_point_2 = Vec2d(new_path, cutter_area_y);
        out_point_3 = Vec2d(new_path, end_y_position);
    } else {
        out_point_1 = Vec2d(new_path, 0);
        out_point_2 = Vec2d(new_path, 0);
        out_point_3 = Vec2d(new_path, end_y_position);
    }

    out_points.clear();
    out_points.emplace_back(out_point_1);
    out_points.emplace_back(out_point_2);
    out_points.emplace_back(out_point_3);

    return out_points;
}

// Only add a newline in case the current G-code does not end with a newline.
    static inline void check_add_eol(std::string& gcode)
    {
        if (!gcode.empty() && gcode.back() != '\n')
            gcode += '\n';
    }


    // Return true if tch_prefix is found in custom_gcode
    static bool custom_gcode_changes_tool(const std::string& custom_gcode, const std::string& tch_prefix, unsigned next_extruder)
    {
        bool ok = false;
        size_t from_pos = 0;
        size_t pos = 0;
        while ((pos = custom_gcode.find(tch_prefix, from_pos)) != std::string::npos) {
            if (pos + 1 == custom_gcode.size())
                break;
            from_pos = pos + 1;
            // only whitespace is allowed before the command
            while (--pos < custom_gcode.size() && custom_gcode[pos] != '\n') {
                if (!std::isspace(custom_gcode[pos]))
                    goto NEXT;
            }
            {
                // we should also check that the extruder changes to what was expected
                std::istringstream ss(custom_gcode.substr(from_pos, std::string::npos));
                unsigned num = 0;
                if (ss >> num)
                    ok = (num == next_extruder);
            }
        NEXT:;
        }
        return ok;
    }

    std::string OozePrevention::pre_toolchange(GCode& gcodegen)
    {
        std::string gcode;

        unsigned int extruder_id        = gcodegen.writer().extruder()->id();
        const auto&  filament_idle_temp = gcodegen.config().idle_temperature;
        if (filament_idle_temp.get_at(extruder_id) == 0) {
            // There is no idle temperature defined in filament settings.
            // Use the delta value from print config.
            if (gcodegen.config().standby_temperature_delta.value != 0) {
                // we assume that heating is always slower than cooling, so no need to block
                gcode += gcodegen.writer().set_temperature(this->_get_temp(gcodegen) + gcodegen.config().standby_temperature_delta.value, false, extruder_id);
                gcode.pop_back();
                gcode += " ;cooldown\n"; // this is a marker for GCodeProcessor, so it can supress the commands when needed
            }
        } else {
            // Use the value from filament settings. That one is absolute, not delta.
            gcode += gcodegen.writer().set_temperature(filament_idle_temp.get_at(extruder_id), false, extruder_id);
            gcode.pop_back();
            gcode += " ;cooldown\n"; // this is a marker for GCodeProcessor, so it can supress the commands when needed
        }

        return gcode;
    }

    std::string OozePrevention::post_toolchange(GCode& gcodegen)
    {
        // Keep the restore condition aligned with pre_toolchange():
        // if an extruder may be cooled down by either idle temperature (absolute)
        // or standby delta (relative), restore it to print temperature after toolchange.
        const unsigned int extruder_id = gcodegen.writer().extruder()->id();
        const auto&        filament_idle_temp = gcodegen.config().idle_temperature;
        const bool         has_idle_temp = filament_idle_temp.get_at(extruder_id) != 0;
        const bool         has_standby_delta = gcodegen.config().standby_temperature_delta.value != 0;

        if (! (has_idle_temp || has_standby_delta))
            return std::string();

        std::string gcode = gcodegen.writer().set_temperature(this->_get_temp(gcodegen), true, extruder_id);
        const bool is_f039 = boost::algorithm::icontains(gcodegen.config().printer_model.value, "F039");
        if (is_f039 && gcode.rfind("M109 ", 0) == 0 &&
            gcode.find("set nozzle temperature and wait for it to be reached") != std::string::npos)
            gcode.insert(gcode.begin(), ';');
        return gcode;
    }

    int OozePrevention::_get_temp(const GCode& gcodegen) const
    {
        // First layer temperature should be used when on the first layer (obviously) and when
        // "other layers" is set to zero (which means it should not be used).
        return (gcodegen.layer() == nullptr || gcodegen.layer()->id() == 0 ||
                gcodegen.config().nozzle_temperature.get_at(gcodegen.writer().extruder()->id()) == 0) ?
                    gcodegen.config().nozzle_temperature_initial_layer.get_at(gcodegen.writer().extruder()->id()) :
                    gcodegen.config().nozzle_temperature.get_at(gcodegen.writer().extruder()->id());

    };

    // Orca:
    // Function to calculate the excess retraction length that should be retracted either before or after wiping
    // in order for the wipe operation to respect the filament retraction speed
    Wipe::RetractionValues Wipe::calculateWipeRetractionLengths(GCode& gcodegen, bool toolchange) {
        auto& writer = gcodegen.writer();
        auto& config = gcodegen.config();
        auto extruder = writer.extruder();
        auto extruder_id = extruder->id();
        auto last_pos = gcodegen.last_pos();

        // Declare & initialize retraction lengths
        double retraction_length_remaining = 0,
                retractionBeforeWipe = 0,
                retractionDuringWipe = 0;

        // initialise the remaining retraction amount with the full retraction amount.
        retraction_length_remaining = toolchange ? std::max(extruder->retraction_length(), extruder->retract_length_toolchange()) :extruder->retraction_length();
        /*if (toolchange)
        {
            retraction_length_remaining = std::max(extruder->retraction_length(), extruder->retract_length_toolchange());
        } else {
            retraction_length_remaining = extruder->retraction_length();
        }*/

        // nothing to retract - return early
        if(retraction_length_remaining <=EPSILON) return {0.f,0.f};

        // calculate retraction before wipe distance from the user setting. Keep adding to this variable any excess retraction needed
        // to be performed before the wipe.
        retractionBeforeWipe = retraction_length_remaining * extruder->retract_before_wipe();
        retraction_length_remaining -= retractionBeforeWipe; // subtract it from the remaining retraction length

        // all of the retraction is to be done before the wipe
        if(retraction_length_remaining <=EPSILON) return {retractionBeforeWipe,0.f};

        return {retractionBeforeWipe, retraction_length_remaining};

        // Calculate wipe speed
        double wipe_speed = config.role_based_wipe_speed ? writer.get_current_speed() / 60.0 : config.get_abs_value("wipe_speed");
        wipe_speed = std::max(wipe_speed, 10.0);

        // Process wipe path & calculate wipe path length
        double wipe_dist = scale_(config.wipe_distance.get_at(extruder_id));
        Polyline wipe_path = {last_pos};
        wipe_path.append(this->path.points.begin() + 1, this->path.points.end());
        double wipe_path_length = std::min(wipe_path.length(), wipe_dist);

        // Calculate the maximum retraction amount during wipe
        retractionDuringWipe = config.retraction_speed.get_at(extruder_id) * unscale_(wipe_path_length) / wipe_speed;
        // If the maximum retraction amount during wipe is too small, return 0 and retract everything prior to the wipe.
        if(retractionDuringWipe <= EPSILON) return {retractionBeforeWipe,0.f};

        // If the maximum retraction amount during wipe is greater than any remaining retraction length
        // return the remaining retraction length to be retracted during the wipe
        if (retractionDuringWipe - retraction_length_remaining > EPSILON) return {retractionBeforeWipe,retraction_length_remaining};

        // We will always proceed with incrementing the retraction amount before wiping with the difference
        // and return the maximum allowed wipe amount to be retracted during the wipe move
        retractionBeforeWipe += retraction_length_remaining - retractionDuringWipe;
        return {retractionBeforeWipe, retractionDuringWipe};
    }

    std::string Wipe::wipe(GCode& gcodegen,double length, bool toolchange, bool is_last)
    {
        std::string gcode;

        /*  Reduce feedrate a bit; travel speed is often too high to move on existing material.
            Too fast = ripping of existing material; too slow = short wipe path, thus more blob.  */
        double _wipe_speed = gcodegen.config().get_abs_value("wipe_speed");// gcodegen.writer().config.travel_speed.value * 0.8;
        if(gcodegen.config().role_based_wipe_speed)
            _wipe_speed = gcodegen.writer().get_current_speed() / 60.0;
        if(_wipe_speed < 10)
            _wipe_speed = 10;


        //SoftFever: allow 100% retract before wipe
        if (length >= 0)
        {
            /*  Calculate how long we need to travel in order to consume the required
                amount of retraction. In other words, how far do we move in XY at wipe_speed
                for the time needed to consume retraction_length at retraction_speed?  */
            // BBS
            double wipe_dist = scale_(gcodegen.config().wipe_distance.get_at(gcodegen.writer().extruder()->id()));

            /*  Take the stored wipe path and replace first point with the current actual position
                (they might be different, for example, in case of loop clipping).  */
            Polyline wipe_path;
            wipe_path.append(gcodegen.last_pos());
            wipe_path.append(
                this->path.points.begin() + 1,
                this->path.points.end()
            );

            // wangwenbin: check if the wipe path is empty
            for (Point& apoint : wipe_path.points) {
                if (apoint.x() < 0 || apoint.y() < 0) {
                    int a = 0;
                }
            }

            wipe_path.clip_end(wipe_path.length() - wipe_dist);

            // subdivide the retraction in segments
            if (!wipe_path.empty()) {
                // BBS. Handle short path case.
                if (wipe_path.length() < wipe_dist) {
                    wipe_dist = wipe_path.length();
                    //BBS: avoid to divide 0
                    wipe_dist = wipe_dist < EPSILON ? EPSILON : wipe_dist;
                }

                // add tag for processor
                gcode += ";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Wipe_Start) + "\n";
                //BBS: don't need to enable cooling makers when this is the last wipe. Because no more cooling layer will clean this "_WIPE"
                //Softfever:
                std::string cooling_mark = "";
                if (gcodegen.enable_cooling_markers() && !is_last)
                    cooling_mark = /*gcodegen.config().role_based_wipe_speed ? ";_EXTERNAL_PERIMETER" : */";_WIPE";

                gcode += gcodegen.writer().set_speed(_wipe_speed * 60, "", cooling_mark);
                for (const Line& line : wipe_path.lines()) {
                    double segment_length = line.length();
                    double dE = length * (segment_length / wipe_dist)* 0.95;
                    //BBS: fix this FIXME
                    //FIXME one shall not generate the unnecessary G1 Fxxx commands, here wipe_speed is a constant inside this cycle.
                    // Is it here for the cooling markers? Or should it be outside of the cycle?
                    //gcode += gcodegen.writer().set_speed(wipe_speed * 60, "", gcodegen.enable_cooling_markers() ? ";_WIPE" : "");
                    gcode += gcodegen.writer().extrude_to_xy(
                        gcodegen.point_to_gcode(line.b),
                        -dE,
                        "wipe and retract"
                    );
                }
                // add tag for processor
                gcode += ";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Wipe_End) + "\n";
                gcodegen.set_last_pos(wipe_path.points.back());
            }

            // prevent wiping again on same path
            this->reset_path();
        }

        return gcode;
    }

    static inline Point wipe_tower_point_to_object_point(GCode& gcodegen, const Vec2f& wipe_tower_pt)
    {
        return Point(scale_(wipe_tower_pt.x() - gcodegen.origin()(0)), scale_(wipe_tower_pt.y() - gcodegen.origin()(1)));
    }

    void cal_flush_list(float src_length, std::vector<float>& cal_length, const FlushConfig& cfg)
    {
        cal_length.clear(); // ?????
        int remain_length = src_length;

        int first_len = cfg.box_first_clean_length;
        int std_len   = cfg.box_need_clean_length;
        int max_len   = cfg.box_need_clean_length_max;

        if (remain_length <= max_len) {
            if (remain_length >= first_len) {
                cal_length.push_back(remain_length);
            } else {
                cal_length.push_back(first_len);
            }
            return;
        }

        // Step 1: Add first section
        cal_length.push_back(first_len);
        remain_length -= first_len;
        int count = 1;

        // Step 2: Add standard-length sections
        while (remain_length >= std_len) {
            cal_length.push_back(std_len);
            remain_length -= std_len;
            count++;
        }

        // Step 3: Handle remaining part
        if (remain_length > 0) {
            if (count == 1) {
                cal_length.push_back(std_len);
            } else {
                float average_remain = static_cast<double>(remain_length - (max_len - first_len)) / (count - 1);

                if (remain_length <= max_len - std_len) {
                    cal_length[count - 1] += remain_length;
                } else if (average_remain > (max_len - std_len)) {
                    cal_length.push_back(std_len);
                } else {
                    cal_length[0] = max_len;
                    for (int i = 1; i < count; ++i) {
                        cal_length[i] += static_cast<int>(std::round(average_remain));
                    }
                }
            }
        }

        if (cal_length.size() > 5) {
            cal_length.clear();
            for (int i = 0; i < 5; i++) {
                cal_length.push_back(src_length / 5);
            }
        }

    }

    void consume_flush_lengths(std::vector<float>& cal_length, float consumed_length)
    {
        for (float& length : cal_length) {
            if (consumed_length <= EPSILON)
                break;
            const float used_length = std::min(std::max(0.f, length), consumed_length);
            length = std::max(0.f, length - used_length);
            consumed_length -= used_length;
        }
    }

    struct RemainingFlushSegments
    {
        float total_length = 0.f;
        int   first_segment = -1;

        bool has_outside_flush() const { return first_segment >= 0; }
    };

    RemainingFlushSegments normalize_remaining_flush_segments(std::vector<float>& cal_length)
    {
        RemainingFlushSegments result;
        for (size_t i = 0; i < cal_length.size(); ++i) {
            float& length = cal_length[i];
            length = std::max(0.f, length);
            // The change-filament template emits a segment only for flush_length_n > 1.
            // A consumed segment is therefore this segment and the already-consumed prefix.
            if (length <= 1.f) {
                length = 0.f;
                continue;
            }
            if (result.first_segment < 0)
                result.first_segment = static_cast<int>(i);
            result.total_length += length;
        }
        return result;
    }

    std::string WipeTowerIntegration::append_tcr_creality(GCode &gcodegen, const WipeTower::ToolChangeResult &tcr, int new_extruder_id, double z) const
    {

        if (gcodegen.config().purge_in_prime_tower || (gcodegen.config().timelapse_type == TimelapseType::tlSmooth) /*||
            (gcodegen.config().timelapse_type == TimelapseType::tlTraditional)*/)
            return append_tcr_creality_cfs(gcodegen, tcr, new_extruder_id, z);

        auto extractXY = [](const std::string& text) -> Slic3r::Vec2f {
            std::regex  pattern(R"(G1\s+[^;]*X([-+]?\d*\.?\d*)\s+Y([-+]?\d*\.?\d*))");
            std::smatch match;
            if (std::regex_search(text, match, pattern)) {
                return {std::stof(match[1].str()), std::stof(match[2].str())};
            }
            return {0.f, 0.f}; // ?????????
        };

        /* if (new_extruder_id != -1 && new_extruder_id != tcr.new_tool)
            throw Slic3r::InvalidArgument("Error: WipeTowerIntegration::append_tcr was asked to do a toolchange it didn't expect.");*/

        // add for CFS
        bool hasToolChange = (tcr.initial_tool != tcr.new_tool) ? true : false;

        std::string gcode;
#if ORCA_CHECK_GCODE_PLACEHOLDERS
        gcode = "\n;creality tool change start ########################################\n";
#endif

        // Toolchangeresult.gcode assumes the wipe tower corner is at the origin (except for priming lines)
        // We want to rotate and shift all extrusions (gcode postprocessing) and starting and ending position
        float alpha = m_wipe_tower_rotation / 180.f * float(M_PI);

        auto transform_wt_pt = [&alpha, this](const Vec2f& pt) -> Vec2f {
            Vec2f out = Eigen::Rotation2Df(alpha) * pt;
            out += m_wipe_tower_pos;
            return out;
        };

        // Vec2f start_pos = transform_wt_pt(tcr.start_pos);
        // Vec2f end_pos   = transform_wt_pt(tcr.end_pos);
        Vec2f start_pos             = tcr.start_pos;
        Vec2f end_pos               = tcr.end_pos;
        Vec2f m_wipe_max_pos        = transform_wt_pt(Vec2f(tcr.m_wipe_max_x, tcr.m_wipe_max_y));

        Vec2f tool_change_start_pos = start_pos;
        if (tcr.is_tool_change)
            tool_change_start_pos = tcr.tool_change_start_pos;
        if (!tcr.priming) {
            start_pos             = transform_wt_pt(start_pos);
            end_pos               = transform_wt_pt(end_pos);
            tool_change_start_pos = transform_wt_pt(tool_change_start_pos);
        }

        Vec2f wipe_tower_offset   = m_wipe_tower_pos;
        float wipe_tower_rotation = alpha;
        Vec2f plate_origin_2d(m_plate_origin(0), m_plate_origin(1));

        // std::string tcr_rotated_gcode = post_process_wipe_tower_moves(tcr, wipe_tower_offset, wipe_tower_rotation,z);
        std::string wipe_path_before_change_tool;
        Vec2f       end_wipe_pos;

        //block_type == 1 ???????
        int block_type = -1;
        std::string tcr_rotated_gcode = post_process_wipe_tower_moves_wipe_head(tcr, wipe_tower_offset, end_wipe_pos,
                                                                                wipe_path_before_change_tool, block_type,
                                                                                wipe_tower_rotation, z);


        ZHopType z_hope_type    = ZHopType(gcodegen.config().z_hop_types.get_at(gcodegen.writer().extruder()->id()));
        LiftType auto_lift_type = LiftType::NormalLift;
        switch (z_hope_type) {
        case ZHopType::zhtNormal: {
            auto_lift_type = LiftType::NormalLift;
            break;
        }
        case ZHopType::zhtSlope: {
            auto_lift_type = LiftType::LazyLift;
            break;
        }
        case ZHopType::zhtSpiral: {
            auto_lift_type = LiftType::SpiralLift;
            break;
        }
        default:
            // if no corresponding lift type, use normal lift
            auto_lift_type = LiftType::NormalLift;
        }

        if (!wipe_path_before_change_tool.empty())
            gcode += gcodegen.retract(false, false, auto_lift_type);
        else
            gcode += gcodegen.retract(hasToolChange, false, auto_lift_type);

       if (!wipe_path_before_change_tool.empty() || !hasToolChange)
       {
           gcode += gcodegen.travel_to(wipe_tower_point_to_object_point(gcodegen, start_pos + plate_origin_2d), erMixed, "Travel to a Wipe Tower");

           //????????/??????????? ??????????,???????
           if (!gcodegen.writer().is_object_start_str_empty() || block_type < 0)
           {
               if (!gcodegen.config().wipe_tower_no_sparse_layers)
               {
                   // compensate retraction
                   gcode += gcodegen.unretract();
               }
           }
       }



       auto wipe_indexs = [&](const std::string wipe_path, int& indexs)
       {
           std::istringstream gcode_str(wipe_path);
           std::string        line;
           while (std::getline(gcode_str, line))
           {
               if (line.find(";wipe_finish_path") == 0)
                   indexs++;
           }
       };


       int index_i = 0, index = 0;
       wipe_indexs(wipe_path_before_change_tool, index);

        std::istringstream gcode_str(wipe_path_before_change_tool);
        std::string        wall_tail_wipe, line;
        while (std::getline(gcode_str, line)) {
            if (line.find(";wipe_finish_path") == 0) {
                gcodegen.m_wipe.reset_path();
                gcodegen.set_last_pos(
                    wipe_tower_point_to_object_point(gcodegen, transform_wt_pt(tcr.wipe_paths[index_i][0]) + plate_origin_2d));

                for (const Vec2f& wipe_pt : tcr.wipe_paths[index_i]) {
                    gcodegen.m_wipe.path.points.emplace_back(
                        wipe_tower_point_to_object_point(gcodegen, transform_wt_pt(wipe_pt) + plate_origin_2d));
                }
                // compensate retraction
                gcodegen.unretract();
                if (index_i < index - 1)
                    wall_tail_wipe += gcodegen.retract(false, false, auto_lift_type) + "\n";
                else
                    wall_tail_wipe += gcodegen.retract(hasToolChange /*true*/, false, auto_lift_type) + "\n";
                // wall_tail_wipe += line + "\n";
                index_i++;

            }
            else if (line.find(";deretraction_from_wipe_tower_zhop") == 0)
            {
                wall_tail_wipe += gcodegen.writer().unretract();
            }
            else
            {
                wall_tail_wipe += line;
                wall_tail_wipe += "\n";
            }
        }

        if (!wall_tail_wipe.empty())
            gcode += gcodegen.set_wipe_tower_print_acceleration();
        gcode += wall_tail_wipe;

        gcode += gcodegen.writer().unlift(); // Make sure there is no z-hop (in most cases, there isn't).

        double current_z = gcodegen.writer().get_position().z();
        if (z == -1.) // in case no specific z was provided, print at current_z pos
            z = current_z;

        /* bool changeZ = false;
         if (!is_approx((double) tcr.print_z, current_z)) {
             changeZ = true;
             z = tcr.print_z;
             current_z = z;
         }*/

        const bool needs_toolchange = gcodegen.writer().need_toolchange(new_extruder_id);
        const bool will_go_down     = !is_approx(z, current_z);

        {
            // FIXME: It would be better if the wipe tower set the force_travel flag for all toolchanges,
            // then we could simplify the condition and make it more readable.



            //gcode += gcodegen.retract(false, false, auto_lift_type);

            // gcode += tcr.muti_lapse;
            bool is_time_lapse = !gcodegen.config().time_lapse_gcode.value.empty() && gcodegen.config().enable_prime_tower &&
                                 m_tool_change_idx == 1 && gcodegen.config().timelapse_type != TimelapseType::tlClose;
            // add for CFS
            if (!hasToolChange) {
                if (is_time_lapse) {
                    Vec2f time_lapse_pos = extractXY(gcodegen.config().time_lapse_gcode.value);
                    gcodegen.m_avoid_crossing_perimeters.use_external_mp_once();
                    gcode += gcodegen.travel_to(wipe_tower_point_to_object_point(gcodegen, time_lapse_pos + plate_origin_2d), erMixed,
                                                "Travel to is_time_lapse");

                } else {
                   /* gcodegen.m_avoid_crossing_perimeters.use_external_mp_once();
                    gcode += gcodegen.travel_to(wipe_tower_point_to_object_point(gcodegen, start_pos + plate_origin_2d), erMixed,
                                                "Travel to a Wipe Tower");
                    gcode += gcodegen.unretract();*/
                }

            } else {
                // for cfs 'EXCLUDE_OBJECT_END'
                gcodegen.m_writer.add_object_change_labels(gcode);
            }

            if (is_time_lapse) {
                DynamicConfig config;
                config.set_key_value("layer_num", new ConfigOptionInt(gcodegen.m_layer_index));
                config.set_key_value("layer_z", new ConfigOptionFloat(current_z));
                config.set_key_value("max_layer_z", new ConfigOptionFloat(gcodegen.m_max_layer_z));
                config.set_key_value("enable_prime_tower", new ConfigOptionBool(gcodegen.config().enable_prime_tower));
                gcode += gcodegen.placeholder_parser_process("timelapse_gcode", gcodegen.config().time_lapse_gcode.value,
                                                             gcodegen.writer().extruder()->id(), &config) +
                         "\n";

                if (!hasToolChange) {
                    gcode += gcodegen.writer().travel_to_xy((start_pos + plate_origin_2d).cast<double>());
                    gcode += gcodegen.unretract();
                }
            }
        }

        bool skeleton_flush_wipe_wall_printed = false;
        auto prepare_skeleton_flush_wipe_wall = [&]() -> bool {
            gcodegen.m_pending_skeleton_flush_wipe_wall_box_valid = false;
            gcodegen.m_pending_skeleton_flush_wipe_wall_print_z = -1.;
            gcodegen.m_pending_skeleton_flush_wipe_wall_layer_height = 0.f;
            gcodegen.m_pending_skeleton_flush_wipe_wall_avoid_box_valid = false;
            gcodegen.m_pending_skeleton_flush_wipe_wall_start_pos_valid = false;
            gcodegen.m_pending_skeleton_flush_wipe_wall_approach_pos_valid = false;
            if (!tcr.wipe_tower_inner_wall_box_valid)
                return false;

            gcodegen.m_pending_skeleton_flush_wipe_wall_print_z = tcr.print_z;
            gcodegen.m_pending_skeleton_flush_wipe_wall_layer_height = tcr.layer_height;
            for (int i = 0; i < 4; ++i)
                gcodegen.m_pending_skeleton_flush_wipe_wall_box[i] =
                    (transform_wt_pt(tcr.wipe_tower_inner_wall_box[i]) + plate_origin_2d).cast<double>();
            gcodegen.m_pending_skeleton_flush_wipe_wall_start_pos_valid = tcr.wipe_tower_inner_wall_start_pos_valid;
            if (tcr.wipe_tower_inner_wall_start_pos_valid)
                gcodegen.m_pending_skeleton_flush_wipe_wall_start_pos =
                    (transform_wt_pt(tcr.wipe_tower_inner_wall_start_pos) + plate_origin_2d).cast<double>();
            gcodegen.m_pending_skeleton_flush_wipe_wall_approach_pos_valid = tcr.wipe_tower_inner_wall_approach_pos_valid;
            if (tcr.wipe_tower_inner_wall_approach_pos_valid)
                gcodegen.m_pending_skeleton_flush_wipe_wall_approach_pos =
                    (transform_wt_pt(tcr.wipe_tower_inner_wall_approach_pos) + plate_origin_2d).cast<double>();
            {
                Vec2d      avoid_min(std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
                Vec2d      avoid_max(-std::numeric_limits<double>::max(), -std::numeric_limits<double>::max());
                BoundingBox avoid_bbx = scaled(m_wipe_tower_bbx);
                Polygon     avoid_points = avoid_bbx.polygon();
                for (const Point& p : avoid_points.points) {
                    const Vec2d gp = (transform_wt_pt(unscale(p).cast<float>()) + plate_origin_2d).cast<double>();
                    avoid_min.x() = std::min(avoid_min.x(), gp.x());
                    avoid_min.y() = std::min(avoid_min.y(), gp.y());
                    avoid_max.x() = std::max(avoid_max.x(), gp.x());
                    avoid_max.y() = std::max(avoid_max.y(), gp.y());
                }
                if (avoid_min.x() <= avoid_max.x() && avoid_min.y() <= avoid_max.y()) {
                    gcodegen.m_pending_skeleton_flush_wipe_wall_avoid_box[0] = Vec2d(avoid_min.x(), avoid_min.y());
                    gcodegen.m_pending_skeleton_flush_wipe_wall_avoid_box[1] = Vec2d(avoid_max.x(), avoid_min.y());
                    gcodegen.m_pending_skeleton_flush_wipe_wall_avoid_box[2] = Vec2d(avoid_max.x(), avoid_max.y());
                    gcodegen.m_pending_skeleton_flush_wipe_wall_avoid_box[3] = Vec2d(avoid_min.x(), avoid_max.y());
                    gcodegen.m_pending_skeleton_flush_wipe_wall_avoid_box_valid = true;
                }
            }
            gcodegen.m_pending_skeleton_flush_wipe_wall_box_valid = true;
            return true;
        };
        std::string toolchange_gcode_str;
        std::string deretraction_str;
        float       purge_volume = tcr.purge_volume < EPSILON ? 0 : std::max(tcr.purge_volume, g_min_purge_volume);
        if (new_extruder_id >= 0 && needs_toolchange)
        {
            gcodegen.m_wipe.reset_path(); // We don't want wiping on the ramming lines.
            const bool prepared_skeleton_flush_wipe_wall = prepare_skeleton_flush_wipe_wall();

            if (hasToolChange)
            {
                toolchange_gcode_str = gcodegen.writer().retract();
                if (!m_single_extruder_multi_material)
                {
                    Vec2d _pos(m_wipe_tower_pos.x(), m_wipe_tower_pos.y());
                    gcodegen.set_tower_pos((_pos));
                }
                //toolchange_gcode_str += gcodegen.set_extruder(new_extruder_id, z); // TODO: toolchange_z vs print_z
                toolchange_gcode_str += gcodegen.set_extruder_new(new_extruder_id, z, purge_volume, (start_pos + plate_origin_2d)[0],
                                                                  (start_pos + plate_origin_2d)[1], m_wipe_max_pos[0],
                                                                  m_wipe_max_pos[1]); // TODO: toolchange_z vs print_z
            }
            else
            {
                //toolchange_gcode_str = gcodegen.set_extruder(new_extruder_id, tcr.print_z); // TODO: toolchange_z vs print_z
                toolchange_gcode_str = gcodegen.set_extruder_new(new_extruder_id, tcr.print_z, purge_volume,
                                                                 (start_pos + plate_origin_2d)[0], (start_pos + plate_origin_2d)[1],
                                                                 m_wipe_max_pos[0],
                                                                 m_wipe_max_pos[1]); // TODO: toolchange_z vs print_z
            }

            skeleton_flush_wipe_wall_printed = prepared_skeleton_flush_wipe_wall && !gcodegen.m_pending_skeleton_flush_wipe_wall_box_valid && !gcodegen.m_pending_skeleton_flush_gcode_generator;
            gcodegen.m_pending_skeleton_flush_wipe_wall_box_valid = false;
            gcodegen.m_pending_skeleton_flush_wipe_wall_print_z = -1.;
            gcodegen.m_pending_skeleton_flush_wipe_wall_layer_height = 0.f;
            gcodegen.m_pending_skeleton_flush_wipe_wall_avoid_box_valid = false;
            gcodegen.m_pending_skeleton_flush_wipe_wall_start_pos_valid = false;
            gcodegen.m_pending_skeleton_flush_wipe_wall_approach_pos_valid = false;
            if (gcodegen.config().enable_prime_tower)
                deretraction_str = gcodegen.unretract();
        }


       {
            //has change filement over.
            std::istringstream gcode_str(tcr_rotated_gcode);
            std::string        wall_tail_wipe, line;
            while (std::getline(gcode_str, line)) {
                if (line.find(";wipe_finish_path") == 0) {
                    gcodegen.m_wipe.reset_path();
                    gcodegen.set_last_pos(
                        wipe_tower_point_to_object_point(gcodegen, transform_wt_pt(tcr.wipe_paths[index_i][0]) + plate_origin_2d));

                    for (const Vec2f& wipe_pt : tcr.wipe_paths[index_i]) {
                        gcodegen.m_wipe.path.points.emplace_back(
                            wipe_tower_point_to_object_point(gcodegen, transform_wt_pt(wipe_pt) + plate_origin_2d));
                    }
                    // compensate retraction
                    gcodegen.unretract();
                    wall_tail_wipe += gcodegen.retract(false, false, auto_lift_type) + "\n";
                    /* wall_tail_wipe += gcodegen.writer().travel_to_xy(
                         (transform_wt_pt(tcr.wipe_paths[path_i][0]) + plate_origin_2d).cast<double>());*/
                    index_i++;

                } else if (line.find(";deretraction_from_wipe_tower_zhop") == 0) {
                    wall_tail_wipe += gcodegen.writer().unretract();
                }
                else {
                    wall_tail_wipe += line;
                    wall_tail_wipe += "\n";
                }
            }
            tcr_rotated_gcode = "";
            tcr_rotated_gcode += wall_tail_wipe;
        }


        std::string around_wipe_tower_str;
        // move to start_pos for wiping after toolchange
        {
        const bool auto_travel_acceleration_was_suppressed = gcodegen.writer().auto_travel_acceleration_suppressed();
        gcodegen.writer().set_auto_travel_acceleration_suppressed(true);
        if (!gcodegen.m_config.prime_tower_skip_points.value) {
        /*    std::string start_pos_str = gcodegen.travel_to(wipe_tower_point_to_object_point(gcodegen,
                                                                                            tool_change_start_pos + plate_origin_2d),
                                                           erMixed, "Move to start pos");*/
            std::string start_pos_str = gcodegen.writer().travel_to_xy(gcodegen.point_to_gcode(wipe_tower_point_to_object_point(gcodegen,
                                                                                            tool_change_start_pos + plate_origin_2d)));

            check_add_eol(start_pos_str);
            around_wipe_tower_str += start_pos_str;
        } else {
            // BBS:change travel_path
            Vec3f gcode_last_pos;
            GCodeProcessor::get_last_position_from_gcode(toolchange_gcode_str, gcode_last_pos);
            Vec2f       gcode_last_pos2d{gcode_last_pos[0], gcode_last_pos[1]};
            Point       gcode_last_pos2d_object = gcodegen.gcode_to_point(gcode_last_pos2d.cast<double>() + plate_origin_2d.cast<double>());
            Point       start_wipe_pos          = wipe_tower_point_to_object_point(gcodegen, tool_change_start_pos + plate_origin_2d);
            BoundingBox avoid_bbx, printer_bbx;
            {
                // set printer_bbx
                Pointfs bed_pointsf = gcodegen.m_config.printable_area.values;
                Points  bed_points;
                for (auto p : bed_pointsf) {
                    bed_points.push_back(wipe_tower_point_to_object_point(gcodegen, p.cast<float>() + plate_origin_2d));
                }
                printer_bbx = BoundingBox(bed_points);
            }
            {
                // set avoid_bbx
                avoid_bbx            = scaled(m_wipe_tower_bbx);
                Polygon avoid_points = avoid_bbx.polygon();
                for (auto& p : avoid_points.points) {
                    Vec2f pp = transform_wt_pt(unscale(p).cast<float>());
                    p        = wipe_tower_point_to_object_point(gcodegen, pp + plate_origin_2d);
                }
                avoid_bbx = BoundingBox(avoid_points.points);
            }
            std::string travel_to_wipe_tower_gcode;
            Polyline    travel_polyline = generate_path_to_wipe_tower(gcode_last_pos2d_object, start_wipe_pos, avoid_bbx, printer_bbx);

            /* for (const auto& p : travel_polyline.points) {

                 travel_to_wipe_tower_gcode += gcodegen.travel_to(p, erNone, "Move to start pos");
                 //travel_to_wipe_tower_gcode += gcodegen.writer().unlift();
                 check_add_eol(travel_to_wipe_tower_gcode);
             }*/
            for (auto it = travel_polyline.points.begin(); it != travel_polyline.points.end() - 1; ++it) {
                //travel_to_wipe_tower_gcode += gcodegen.travel_to(*it, erMixed, "Move to start pos");
                travel_to_wipe_tower_gcode +=gcodegen.writer().travel_to_xy(gcodegen.point_to_gcode(*it));
                check_add_eol(travel_to_wipe_tower_gcode);
            }
            around_wipe_tower_str += travel_to_wipe_tower_gcode;
            gcodegen.set_last_pos(start_wipe_pos);
        }
        gcodegen.writer().set_auto_travel_acceleration_suppressed(auto_travel_acceleration_was_suppressed);
        }

        // add for CFS
        if (will_go_down) {
            // if (changeZ)
            {
                // toolchange_gcode_str += gcodegen.writer().retract();
                // toolchange_gcode_str += gcodegen.writer().travel_to_z(z, "Travel down to the last wipe tower layer.");
                // toolchange_gcode_str += gcodegen.writer().unretract();
            }
            // else
            {
                if (!gcodegen.config().wipe_tower_no_sparse_layers) {
                    toolchange_gcode_str += gcodegen.writer().retract();
                    toolchange_gcode_str += gcodegen.writer().travel_to_z(z, "Travel down to the last wipe tower layer.");
                    toolchange_gcode_str += gcodegen.writer().unretract();
                }
                current_z = z;
            }
        }

        // Insert the toolchange and deretraction gcode into the generated gcode.

        DynamicConfig config;
        config.set_key_value("change_filament_gcode", new ConfigOptionString(toolchange_gcode_str));
        config.set_key_value("move_around_wipe_tower", new ConfigOptionString(around_wipe_tower_str));
        config.set_key_value("deretraction_from_wipe_tower_generator", new ConfigOptionString(deretraction_str));

        int previous_extruder_id = gcodegen.writer().extruder() ? (int) gcodegen.writer().extruder()->id() : -1;
        config.set_key_value("previous_extruder", new ConfigOptionInt(previous_extruder_id));
        config.set_key_value("next_extruder", new ConfigOptionInt((int) new_extruder_id));
        config.set_key_value("layer_num", new ConfigOptionInt(gcodegen.m_layer_index));
        config.set_key_value("layer_z", new ConfigOptionFloat(tcr.print_z));
        config.set_key_value("toolchange_z", new ConfigOptionFloat(z));
        GCodeWriter&     gcode_writer = gcodegen.m_writer;
        FullPrintConfig& full_config  = gcodegen.m_config;
        float old_retract_length      = gcode_writer.extruder() != nullptr ? full_config.retraction_length.get_at(previous_extruder_id) : 0;
        float new_retract_length      = full_config.retraction_length.get_at(new_extruder_id);
        float old_retract_length_toolchange = gcode_writer.extruder() != nullptr ?
                                                  full_config.retract_length_toolchange.get_at(previous_extruder_id) :
                                                  0;
        float new_retract_length_toolchange = full_config.retract_length_toolchange.get_at(new_extruder_id);
        int   old_filament_temp             = gcode_writer.extruder() != nullptr ?
                                                  (gcodegen.on_first_layer() ? full_config.nozzle_temperature_initial_layer.get_at(previous_extruder_id) :
                                                                               full_config.nozzle_temperature.get_at(previous_extruder_id)) :
                                                  210;
        int   new_filament_temp = gcodegen.on_first_layer() ? full_config.nozzle_temperature_initial_layer.get_at(new_extruder_id) :
                                                              full_config.nozzle_temperature.get_at(new_extruder_id);
        Vec3d nozzle_pos        = gcode_writer.get_position();

        //float purge_volume  = tcr.purge_volume < EPSILON ? 0 : std::max(tcr.purge_volume, g_min_purge_volume);
        float filament_area = float((M_PI / 4.f) * pow(full_config.filament_diameter.get_at(new_extruder_id), 2));
        float purge_length  = purge_volume / filament_area;

        int old_filament_e_feedrate = gcode_writer.extruder() != nullptr ?
                                          (int) (60.0 * full_config.filament_max_volumetric_speed.get_at(previous_extruder_id) /
                                                 filament_area) :
                                          200;
        old_filament_e_feedrate     = old_filament_e_feedrate == 0 ? 100 : old_filament_e_feedrate;
        int new_filament_e_feedrate = (int) (60.0 * full_config.filament_max_volumetric_speed.get_at(new_extruder_id) / filament_area);
        new_filament_e_feedrate     = new_filament_e_feedrate == 0 ? 100 : new_filament_e_feedrate;

        config.set_key_value("max_layer_z", new ConfigOptionFloat(gcodegen.m_max_layer_z));
        config.set_key_value("relative_e_axis", new ConfigOptionBool(full_config.use_relative_e_distances));
        config.set_key_value("toolchange_count", new ConfigOptionInt((int) gcodegen.m_toolchange_count));
        config.set_key_value("fan_speed", new ConfigOptionInt((int) 0));
        config.set_key_value("flush_into_skeleton", new ConfigOptionBool(skeleton_flush_wipe_wall_printed));

        config.set_key_value("old_retract_length", new ConfigOptionFloat(old_retract_length));
        config.set_key_value("new_retract_length", new ConfigOptionFloat(new_retract_length));
        config.set_key_value("old_retract_length_toolchange", new ConfigOptionFloat(old_retract_length_toolchange));
        config.set_key_value("new_retract_length_toolchange", new ConfigOptionFloat(new_retract_length_toolchange));
        config.set_key_value("old_filament_temp", new ConfigOptionInt(old_filament_temp));
        config.set_key_value("new_filament_temp", new ConfigOptionInt(new_filament_temp));
        config.set_key_value("x_after_toolchange", new ConfigOptionFloat(start_pos(0)));
        config.set_key_value("y_after_toolchange", new ConfigOptionFloat(start_pos(1)));
        config.set_key_value("z_after_toolchange", new ConfigOptionFloat(nozzle_pos(2)));
        config.set_key_value("first_flush_volume", new ConfigOptionFloat(purge_length / 2.f));
        config.set_key_value("second_flush_volume", new ConfigOptionFloat(purge_length / 2.f));
        config.set_key_value("old_filament_e_feedrate", new ConfigOptionInt(old_filament_e_feedrate));
        config.set_key_value("new_filament_e_feedrate", new ConfigOptionInt(new_filament_e_feedrate));
        config.set_key_value("travel_point_1_x", new ConfigOptionFloat(float(travel_point_1.x())));
        config.set_key_value("travel_point_1_y", new ConfigOptionFloat(float(travel_point_1.y())));
        config.set_key_value("travel_point_2_x", new ConfigOptionFloat(float(travel_point_2.x())));
        config.set_key_value("travel_point_2_y", new ConfigOptionFloat(float(travel_point_2.y())));
        config.set_key_value("travel_point_3_x", new ConfigOptionFloat(float(travel_point_3.x())));
        config.set_key_value("travel_point_3_y", new ConfigOptionFloat(float(travel_point_3.y())));

        int   flush_count = std::min(g_max_flush_count, (int) std::round(purge_volume / g_purge_volume_one_time));
        float flush_unit  = purge_length / flush_count;
        int   flush_idx   = 0;
        for (; flush_idx < flush_count; flush_idx++) {
            char key_value[64] = {0};
            snprintf(key_value, sizeof(key_value), "flush_length_%d", flush_idx + 1);
            config.set_key_value(key_value, new ConfigOptionFloat(flush_unit));
        }

        for (; flush_idx < g_max_flush_count; flush_idx++) {
            char key_value[64] = {0};
            snprintf(key_value, sizeof(key_value), "flush_length_%d", flush_idx + 1);
            config.set_key_value(key_value, new ConfigOptionFloat(0.f));
        }

        std::string tcr_gcode,
            tcr_escaped_gcode = gcodegen.placeholder_parser_process("tcr_rotated_gcode", tcr_rotated_gcode, new_extruder_id, &config);
        unescape_string_cstyle(tcr_escaped_gcode, tcr_gcode);
        gcode += gcodegen.inject_wipe_tower_print_acceleration(tcr_gcode);
        check_add_eol(toolchange_gcode_str);

        // SoftFever: set new PA for new filament
        if (new_extruder_id != -1 && gcodegen.config().enable_pressure_advance.get_at(new_extruder_id)) {
            gcode += gcodegen.writer().set_pressure_advance(gcodegen.config().pressure_advance.get_at(new_extruder_id));
        }

        //??
        const bool auto_travel_acceleration_was_suppressed = gcodegen.writer().auto_travel_acceleration_suppressed();
        gcodegen.writer().set_auto_travel_acceleration_suppressed(true);
        if (!gcodegen.config().wipe_tower_no_sparse_layers && !is_approx(z, current_z))
            /*gcode += */ gcodegen.writer().travel_to_z(current_z + gcodegen.config().z_hop.get_at(new_extruder_id) /*0.4*/,
                                                        "Travel back up to the topmost object layer.");

        // A phony move to the end position at the wipe tower.
        Vec3f end_pos_;
        GCodeProcessor::get_last_position_from_gcode(gcode, end_pos_);
        Vec2f end_pos2d_{end_pos_[0], end_pos_[1]};
        gcodegen.writer().travel_to_xy((end_pos2d_ + plate_origin_2d).cast<double>());
        gcodegen.set_last_pos(wipe_tower_point_to_object_point(gcodegen, end_pos2d_ + plate_origin_2d));

        //??
        if (!gcodegen.config().wipe_tower_no_sparse_layers && !is_approx(z, current_z))
        /*gcode += */gcodegen.writer().travel_to_z(current_z, "Travel back up to the topmost object layer.");

        gcodegen.writer().set_auto_travel_acceleration_suppressed(auto_travel_acceleration_was_suppressed);

        // current_z = gcodegen.writer().get_position().z();
        if (!gcodegen.config().wipe_tower_no_sparse_layers && !is_approx(z, current_z)) {
            gcode += gcodegen.writer().retract();
            gcode += gcodegen.writer().travel_to_z(current_z, "Travel back up to the topmost object layer.");
            gcode += gcodegen.writer().unretract();
        }

        //else {
        //    // Prepare a future wipe.
        //    gcodegen.m_wipe.reset_path();
        //    for (const Vec2f& wipe_pt : tcr.wipe_path)
        //        gcodegen.m_wipe.path.points.emplace_back(
        //            wipe_tower_point_to_object_point(gcodegen, transform_wt_pt(wipe_pt) + plate_origin_2d));
        //}

        // Let the planner know we are traveling between objects.
        gcodegen.m_avoid_crossing_perimeters.use_external_mp_once();

#if ORCA_CHECK_GCODE_PLACEHOLDERS
        gcode += ";creality tool change end ########################################\n\n";
#endif
        return gcode;


    }

    std::string WipeTowerIntegration::append_tcr_creality_cfs(GCode& gcodegen,const WipeTower::ToolChangeResult& tcr,int new_extruder_id,double z) const
    {

        static int recode_num = 0;
        recode_num++;
        auto extractXY = [](const std::string& text) -> Slic3r::Vec2f {
            std::regex  pattern(R"(G1\s+[^;]*X([-+]?\d*\.?\d*)\s+Y([-+]?\d*\.?\d*))");
            std::smatch match;
            if (std::regex_search(text, match, pattern)) {
                return {std::stof(match[1].str()), std::stof(match[2].str())};
            }
            return {0.f, 0.f}; // ?????????
        };

        if (new_extruder_id != -1 && new_extruder_id != tcr.new_tool)
            throw Slic3r::InvalidArgument("Error: WipeTowerIntegration::append_tcr was asked to do a toolchange it didn't expect.");

        // add for CFS
        bool hasToolChange = (tcr.initial_tool != tcr.new_tool) ? true : false;

        std::string gcode;
    #if ORCA_CHECK_GCODE_PLACEHOLDERS
        gcode = "\n;creality tool change start ########################################\n";
    #endif

        // Toolchangeresult.gcode assumes the wipe tower corner is at the origin (except for priming lines)
        // We want to rotate and shift all extrusions (gcode postprocessing) and starting and ending position
        float alpha = m_wipe_tower_rotation / 180.f * float(M_PI);

        auto transform_wt_pt = [&alpha, this](const Vec2f& pt) -> Vec2f {
            Vec2f out = Eigen::Rotation2Df(alpha) * pt;
            out += m_wipe_tower_pos;
            return out;
        };

        Vec2f start_pos = transform_wt_pt(tcr.start_pos);
        Vec2f end_pos   = transform_wt_pt(tcr.end_pos);

        Vec2f m_wipe_max_pos = transform_wt_pt(Vec2f(tcr.m_wipe_max_x, tcr.m_wipe_max_y));

        Vec2f wipe_tower_offset   = m_wipe_tower_pos;
        float wipe_tower_rotation = alpha;
        Vec2f plate_origin_2d(m_plate_origin(0), m_plate_origin(1));

        std::string tcr_rotated_gcode = post_process_wipe_tower_moves_wipe(tcr, wipe_tower_offset, wipe_tower_rotation, z, hasToolChange);

        gcode += gcodegen.writer().unlift(); // Make sure there is no z-hop (in most cases, there isn't).

        double current_z = gcodegen.writer().get_position().z();
        if (z == -1.) // in case no specific z was provided, print at current_z pos
            z = current_z;

        /* bool changeZ = false;
            if (!is_approx((double) tcr.print_z, current_z)) {
                changeZ = true;
                z = tcr.print_z;
                current_z = z;
            }*/

        const bool needs_toolchange = gcodegen.writer().need_toolchange(new_extruder_id);
        const bool will_go_down     = !is_approx(z, current_z);

        {
            // FIXME: It would be better if the wipe tower set the force_travel flag for all toolchanges,
            // then we could simplify the condition and make it more readable.

            ZHopType z_hope_type    = ZHopType(gcodegen.config().z_hop_types.get_at(gcodegen.writer().extruder()->id()));
            LiftType auto_lift_type = LiftType::NormalLift;
            switch (z_hope_type) {
            case ZHopType::zhtNormal: {
                auto_lift_type = LiftType::NormalLift;
                break;
            }
            case ZHopType::zhtSlope: {
                auto_lift_type = LiftType::LazyLift;
                break;
            }
            case ZHopType::zhtSpiral: {
                auto_lift_type = LiftType::SpiralLift;
                break;
            }
            default:
                // if no corresponding lift type, use normal lift
                auto_lift_type = LiftType::NormalLift;
            }
            if (gcodegen.m_layer_index == 0 && !hasToolChange) {
                gcode += gcodegen.retract(hasToolChange, false);

            } else {
                gcode += gcodegen.retract(hasToolChange, false, auto_lift_type);
            }

            // gcode += tcr.muti_lapse;

            bool is_time_lapse = !gcodegen.config().time_lapse_gcode.value.empty() && gcodegen.config().enable_prime_tower &&
                                    m_tool_change_idx == 1 && gcodegen.config().timelapse_type != TimelapseType::tlClose;
            // add for CFS
            if (!hasToolChange) {
                if (is_time_lapse) {
                    Vec2f time_lapse_pos = extractXY(gcodegen.config().time_lapse_gcode.value);
                    gcodegen.m_avoid_crossing_perimeters.use_external_mp_once();
                    gcode += gcodegen.travel_to(wipe_tower_point_to_object_point(gcodegen, time_lapse_pos + plate_origin_2d), erMixed,
                                                "Travel to is_time_lapse");

                } else {
                    gcodegen.m_avoid_crossing_perimeters.use_external_mp_once();
                    gcode += gcodegen.travel_to(wipe_tower_point_to_object_point(gcodegen, start_pos + plate_origin_2d), erMixed,
                                                "Travel to a Wipe Tower");
                    gcode += gcodegen.unretract();
                }

            } else {
                // for cfs 'EXCLUDE_OBJECT_END'
                gcodegen.m_writer.add_object_change_labels(gcode);
            }

            if (is_time_lapse) {
                DynamicConfig config;
                config.set_key_value("layer_num", new ConfigOptionInt(gcodegen.m_layer_index));
                config.set_key_value("layer_z", new ConfigOptionFloat(current_z));
                config.set_key_value("max_layer_z", new ConfigOptionFloat(gcodegen.m_max_layer_z));
                config.set_key_value("enable_prime_tower", new ConfigOptionBool(gcodegen.config().enable_prime_tower));
                gcode += gcodegen.placeholder_parser_process("timelapse_gcode", gcodegen.config().time_lapse_gcode.value,
                                                                gcodegen.writer().extruder()->id(), &config) +
                            "\n";

                if (!hasToolChange) {
                    gcode += gcodegen.writer().travel_to_xy((start_pos + plate_origin_2d).cast<double>());
                    gcode += gcodegen.unretract();
                }
            }
        }

        bool skeleton_flush_wipe_wall_printed = false;
        auto prepare_skeleton_flush_wipe_wall = [&]() -> bool {
            gcodegen.m_pending_skeleton_flush_wipe_wall_box_valid = false;
            gcodegen.m_pending_skeleton_flush_wipe_wall_print_z = -1.;
            gcodegen.m_pending_skeleton_flush_wipe_wall_layer_height = 0.f;
            gcodegen.m_pending_skeleton_flush_wipe_wall_avoid_box_valid = false;
            gcodegen.m_pending_skeleton_flush_wipe_wall_start_pos_valid = false;
            gcodegen.m_pending_skeleton_flush_wipe_wall_approach_pos_valid = false;
            if (!tcr.wipe_tower_inner_wall_box_valid)
                return false;

            gcodegen.m_pending_skeleton_flush_wipe_wall_print_z = tcr.print_z;
            gcodegen.m_pending_skeleton_flush_wipe_wall_layer_height = tcr.layer_height;
            for (int i = 0; i < 4; ++i)
                gcodegen.m_pending_skeleton_flush_wipe_wall_box[i] =
                    (transform_wt_pt(tcr.wipe_tower_inner_wall_box[i]) + plate_origin_2d).cast<double>();
            gcodegen.m_pending_skeleton_flush_wipe_wall_start_pos_valid = tcr.wipe_tower_inner_wall_start_pos_valid;
            if (tcr.wipe_tower_inner_wall_start_pos_valid)
                gcodegen.m_pending_skeleton_flush_wipe_wall_start_pos =
                    (transform_wt_pt(tcr.wipe_tower_inner_wall_start_pos) + plate_origin_2d).cast<double>();
            gcodegen.m_pending_skeleton_flush_wipe_wall_approach_pos_valid = tcr.wipe_tower_inner_wall_approach_pos_valid;
            if (tcr.wipe_tower_inner_wall_approach_pos_valid)
                gcodegen.m_pending_skeleton_flush_wipe_wall_approach_pos =
                    (transform_wt_pt(tcr.wipe_tower_inner_wall_approach_pos) + plate_origin_2d).cast<double>();
            {
                Vec2d      avoid_min(std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
                Vec2d      avoid_max(-std::numeric_limits<double>::max(), -std::numeric_limits<double>::max());
                BoundingBox avoid_bbx = scaled(m_wipe_tower_bbx);
                Polygon     avoid_points = avoid_bbx.polygon();
                for (const Point& p : avoid_points.points) {
                    const Vec2d gp = (transform_wt_pt(unscale(p).cast<float>()) + plate_origin_2d).cast<double>();
                    avoid_min.x() = std::min(avoid_min.x(), gp.x());
                    avoid_min.y() = std::min(avoid_min.y(), gp.y());
                    avoid_max.x() = std::max(avoid_max.x(), gp.x());
                    avoid_max.y() = std::max(avoid_max.y(), gp.y());
                }
                if (avoid_min.x() <= avoid_max.x() && avoid_min.y() <= avoid_max.y()) {
                    gcodegen.m_pending_skeleton_flush_wipe_wall_avoid_box[0] = Vec2d(avoid_min.x(), avoid_min.y());
                    gcodegen.m_pending_skeleton_flush_wipe_wall_avoid_box[1] = Vec2d(avoid_max.x(), avoid_min.y());
                    gcodegen.m_pending_skeleton_flush_wipe_wall_avoid_box[2] = Vec2d(avoid_max.x(), avoid_max.y());
                    gcodegen.m_pending_skeleton_flush_wipe_wall_avoid_box[3] = Vec2d(avoid_min.x(), avoid_max.y());
                    gcodegen.m_pending_skeleton_flush_wipe_wall_avoid_box_valid = true;
                }
            }
            gcodegen.m_pending_skeleton_flush_wipe_wall_box_valid = true;
            return true;
        };
        std::string toolchange_gcode_str;
        std::string deretraction_str;
        float       purge_volume = tcr.purge_volume < EPSILON ? 0 : std::max(tcr.purge_volume, g_min_purge_volume);
        if (new_extruder_id >= 0 && needs_toolchange) {
            gcodegen.m_wipe.reset_path(); // We don't want wiping on the ramming lines.
            const bool prepared_skeleton_flush_wipe_wall = prepare_skeleton_flush_wipe_wall();

            // add for CFS
            if (hasToolChange) {
                toolchange_gcode_str = gcodegen.writer().retract();
                if (!m_single_extruder_multi_material) {
                    Vec2d _pos(m_wipe_tower_pos.x(), m_wipe_tower_pos.y());
                    gcodegen.set_tower_pos((_pos));
                }
                toolchange_gcode_str += gcodegen.set_extruder_new(new_extruder_id, z, purge_volume, (start_pos + plate_origin_2d)[0],
                                                                    (start_pos + plate_origin_2d)[1], m_wipe_max_pos[0],
                                                                    m_wipe_max_pos[1]); // TODO: toolchange_z vs print_z
            } else {
                toolchange_gcode_str = gcodegen.set_extruder_new(new_extruder_id, tcr.print_z, purge_volume,
                                                                    (start_pos + plate_origin_2d)[0], (start_pos + plate_origin_2d)[1],
                                                                    m_wipe_max_pos[0],
                                                                    m_wipe_max_pos[1]); // TODO: toolchange_z vs print_z
            }
            skeleton_flush_wipe_wall_printed = prepared_skeleton_flush_wipe_wall && !gcodegen.m_pending_skeleton_flush_wipe_wall_box_valid && !gcodegen.m_pending_skeleton_flush_gcode_generator;
            gcodegen.m_pending_skeleton_flush_wipe_wall_box_valid = false;
            gcodegen.m_pending_skeleton_flush_wipe_wall_print_z = -1.;
            gcodegen.m_pending_skeleton_flush_wipe_wall_layer_height = 0.f;
            gcodegen.m_pending_skeleton_flush_wipe_wall_avoid_box_valid = false;
            gcodegen.m_pending_skeleton_flush_wipe_wall_start_pos_valid = false;
            gcodegen.m_pending_skeleton_flush_wipe_wall_approach_pos_valid = false;
            if (gcodegen.config().enable_prime_tower)
                deretraction_str = gcodegen.unretract();
        }

        // add for CFS
        if (will_go_down) {
            // if (changeZ)
            {
                // toolchange_gcode_str += gcodegen.writer().retract();
                // toolchange_gcode_str += gcodegen.writer().travel_to_z(z, "Travel down to the last wipe tower layer.");
                // toolchange_gcode_str += gcodegen.writer().unretract();
            }
            // else
            {
                if (!gcodegen.config().wipe_tower_no_sparse_layers) {
                    toolchange_gcode_str += gcodegen.writer().retract();
                    toolchange_gcode_str += gcodegen.writer().travel_to_z(z, "Travel down to the last wipe tower layer.");
                    toolchange_gcode_str += gcodegen.writer().unretract();
                }
                current_z = z;
            }
        }

        // Insert the toolchange and deretraction gcode into the generated gcode.

        DynamicConfig config;
        config.set_key_value("change_filament_gcode", new ConfigOptionString(toolchange_gcode_str));
        config.set_key_value("deretraction_from_wipe_tower_generator", new ConfigOptionString(deretraction_str));

        int previous_extruder_id = gcodegen.writer().extruder() ? (int) gcodegen.writer().extruder()->id() : -1;
        config.set_key_value("previous_extruder", new ConfigOptionInt(previous_extruder_id));
        config.set_key_value("next_extruder", new ConfigOptionInt((int) new_extruder_id));
        config.set_key_value("layer_num", new ConfigOptionInt(gcodegen.m_layer_index));
        config.set_key_value("layer_z", new ConfigOptionFloat(tcr.print_z));
        config.set_key_value("toolchange_z", new ConfigOptionFloat(z));
        GCodeWriter&     gcode_writer = gcodegen.m_writer;
        FullPrintConfig& full_config  = gcodegen.m_config;
        float old_retract_length      = gcode_writer.extruder() != nullptr ? full_config.retraction_length.get_at(previous_extruder_id) : 0;
        float new_retract_length      = full_config.retraction_length.get_at(new_extruder_id);
        float old_retract_length_toolchange = gcode_writer.extruder() != nullptr ?
                                                    full_config.retract_length_toolchange.get_at(previous_extruder_id) :
                                                    0;
        float new_retract_length_toolchange = full_config.retract_length_toolchange.get_at(new_extruder_id);
        int   old_filament_temp             = gcode_writer.extruder() != nullptr ?
                                                    (gcodegen.on_first_layer() ? full_config.nozzle_temperature_initial_layer.get_at(previous_extruder_id) :
                                                                                full_config.nozzle_temperature.get_at(previous_extruder_id)) :
                                                    210;
        int   new_filament_temp = gcodegen.on_first_layer() ? full_config.nozzle_temperature_initial_layer.get_at(new_extruder_id) :
                                                                full_config.nozzle_temperature.get_at(new_extruder_id);
        Vec3d nozzle_pos        = gcode_writer.get_position();

        // float purge_volume  = tcr.purge_volume < EPSILON ? 0 : std::max(tcr.purge_volume, g_min_purge_volume);
        float filament_area = float((M_PI / 4.f) * pow(full_config.filament_diameter.get_at(new_extruder_id), 2));
        float purge_length  = purge_volume / filament_area;

        int old_filament_e_feedrate = gcode_writer.extruder() != nullptr ?
                                            (int) (60.0 * full_config.filament_max_volumetric_speed.get_at(previous_extruder_id) /
                                                    filament_area) :
                                            200;
        old_filament_e_feedrate     = old_filament_e_feedrate == 0 ? 100 : old_filament_e_feedrate;
        int new_filament_e_feedrate = (int) (60.0 * full_config.filament_max_volumetric_speed.get_at(new_extruder_id) / filament_area);
        new_filament_e_feedrate     = new_filament_e_feedrate == 0 ? 100 : new_filament_e_feedrate;

        config.set_key_value("max_layer_z", new ConfigOptionFloat(gcodegen.m_max_layer_z));
        config.set_key_value("relative_e_axis", new ConfigOptionBool(full_config.use_relative_e_distances));
        config.set_key_value("toolchange_count", new ConfigOptionInt((int) gcodegen.m_toolchange_count));
        config.set_key_value("fan_speed", new ConfigOptionInt((int) 0));
        config.set_key_value("flush_into_skeleton", new ConfigOptionBool(skeleton_flush_wipe_wall_printed));

        config.set_key_value("old_retract_length", new ConfigOptionFloat(old_retract_length));
        config.set_key_value("new_retract_length", new ConfigOptionFloat(new_retract_length));
        config.set_key_value("old_retract_length_toolchange", new ConfigOptionFloat(old_retract_length_toolchange));
        config.set_key_value("new_retract_length_toolchange", new ConfigOptionFloat(new_retract_length_toolchange));
        config.set_key_value("old_filament_temp", new ConfigOptionInt(old_filament_temp));
        config.set_key_value("new_filament_temp", new ConfigOptionInt(new_filament_temp));
        config.set_key_value("x_after_toolchange", new ConfigOptionFloat(start_pos(0)));
        config.set_key_value("y_after_toolchange", new ConfigOptionFloat(start_pos(1)));
        config.set_key_value("z_after_toolchange", new ConfigOptionFloat(nozzle_pos(2)));
        config.set_key_value("first_flush_volume", new ConfigOptionFloat(purge_length / 2.f));
        config.set_key_value("second_flush_volume", new ConfigOptionFloat(purge_length / 2.f));
        config.set_key_value("old_filament_e_feedrate", new ConfigOptionInt(old_filament_e_feedrate));
        config.set_key_value("new_filament_e_feedrate", new ConfigOptionInt(new_filament_e_feedrate));
        config.set_key_value("travel_point_1_x", new ConfigOptionFloat(float(travel_point_1.x())));
        config.set_key_value("travel_point_1_y", new ConfigOptionFloat(float(travel_point_1.y())));
        config.set_key_value("travel_point_2_x", new ConfigOptionFloat(float(travel_point_2.x())));
        config.set_key_value("travel_point_2_y", new ConfigOptionFloat(float(travel_point_2.y())));
        config.set_key_value("travel_point_3_x", new ConfigOptionFloat(float(travel_point_3.x())));
        config.set_key_value("travel_point_3_y", new ConfigOptionFloat(float(travel_point_3.y())));
        config.set_key_value("wipe_tower_start_position_x", new ConfigOptionFloat(float((start_pos + plate_origin_2d)[0])));
        config.set_key_value("wipe_tower_start_position_y", new ConfigOptionFloat(float((start_pos + plate_origin_2d)[1])));
        // config.set_key_value("wipe_tower_random_num", new ConfigOptionFloat(float((start_pos + plate_origin_2d)[1])));
        int   flush_count = std::min(g_max_flush_count, (int) std::round(purge_volume / g_purge_volume_one_time));
        float flush_unit  = purge_length / flush_count;
        int   flush_idx   = 0;
        for (; flush_idx < flush_count; flush_idx++) {
            char key_value[64] = {0};
            snprintf(key_value, sizeof(key_value), "flush_length_%d", flush_idx + 1);
            config.set_key_value(key_value, new ConfigOptionFloat(flush_unit));
        }

        for (; flush_idx < g_max_flush_count; flush_idx++) {
            char key_value[64] = {0};
            snprintf(key_value, sizeof(key_value), "flush_length_%d", flush_idx + 1);
            config.set_key_value(key_value, new ConfigOptionFloat(0.f));
        }

        std::string tcr_gcode,
            tcr_escaped_gcode = gcodegen.placeholder_parser_process("tcr_rotated_gcode", tcr_rotated_gcode, new_extruder_id, &config);
        unescape_string_cstyle(tcr_escaped_gcode, tcr_gcode);
        gcode += gcodegen.inject_wipe_tower_print_acceleration(tcr_gcode);
        check_add_eol(toolchange_gcode_str);

        // SoftFever: set new PA for new filament
        if (new_extruder_id != -1 && gcodegen.config().enable_pressure_advance.get_at(new_extruder_id)) {
            gcode += gcodegen.writer().set_pressure_advance(gcodegen.config().pressure_advance.get_at(new_extruder_id));
        }

        // A phony move to the end position at the wipe tower.
        {
            const bool auto_travel_acceleration_was_suppressed = gcodegen.writer().auto_travel_acceleration_suppressed();
            gcodegen.writer().set_auto_travel_acceleration_suppressed(true);
            gcodegen.writer().travel_to_xy((end_pos + plate_origin_2d).cast<double>());
            gcodegen.writer().set_auto_travel_acceleration_suppressed(auto_travel_acceleration_was_suppressed);
        }
        gcodegen.set_last_pos(wipe_tower_point_to_object_point(gcodegen, end_pos + plate_origin_2d));

        // current_z = gcodegen.writer().get_position().z();
        if (!gcodegen.config().wipe_tower_no_sparse_layers && !is_approx(z, current_z)) {
            gcode += gcodegen.writer().retract();
            gcode += gcodegen.writer().travel_to_z(current_z, "Travel back up to the topmost object layer.");
            gcode += gcodegen.writer().unretract();
        }

        else {
            // Prepare a future wipe.
            gcodegen.m_wipe.reset_path();
            for (const Vec2f& wipe_pt : tcr.wipe_path)
                gcodegen.m_wipe.path.points.emplace_back(
                    wipe_tower_point_to_object_point(gcodegen, transform_wt_pt(wipe_pt) + plate_origin_2d));
        }

        // Let the planner know we are traveling between objects.
        gcodegen.m_avoid_crossing_perimeters.use_external_mp_once();

    #if ORCA_CHECK_GCODE_PLACEHOLDERS
        gcode += ";creality tool change end ########################################\n\n";
    #endif
        return gcode;


    }

    std::string WipeTowerIntegration::append_tcr(GCode &gcodegen, const WipeTower::ToolChangeResult &tcr, int new_extruder_id, double z) const
    {
        if (new_extruder_id != -1 && new_extruder_id != tcr.new_tool)
            throw Slic3r::InvalidArgument("Error: WipeTowerIntegration::append_tcr was asked to do a toolchange it didn't expect.");

        std::string gcode;

        // Toolchangeresult.gcode assumes the wipe tower corner is at the origin (except for priming lines)
        // We want to rotate and shift all extrusions (gcode postprocessing) and starting and ending position
        float alpha = m_wipe_tower_rotation / 180.f * float(M_PI);

        auto transform_wt_pt = [&alpha, this](const Vec2f &pt) -> Vec2f {
            Vec2f out = Eigen::Rotation2Df(alpha) * pt;
            out += m_wipe_tower_pos;
            return out;
        };

        Vec2f start_pos = tcr.start_pos;
        Vec2f end_pos   = tcr.end_pos;
        if (!tcr.priming) {
            start_pos = transform_wt_pt(start_pos);
            end_pos   = transform_wt_pt(end_pos);
        }

        Vec2f wipe_tower_offset   = tcr.priming ? Vec2f::Zero() : m_wipe_tower_pos;
        float wipe_tower_rotation = tcr.priming ? 0.f : alpha;

        std::string tcr_rotated_gcode = post_process_wipe_tower_moves(tcr, wipe_tower_offset, wipe_tower_rotation);

        // BBS: add partplate logic
        Vec2f plate_origin_2d(m_plate_origin(0), m_plate_origin(1));

        // BBS: toolchange gcode will move to start_pos,
        // so only perform movement when printing sparse partition to support upper layer.
        // start_pos is the position in plate coordinate.
        if (!tcr.priming && tcr.is_finish_first) {
            // Move over the wipe tower.
            gcode += gcodegen.retract();
            gcodegen.m_avoid_crossing_perimeters.use_external_mp_once();
            gcode += gcodegen.travel_to(wipe_tower_point_to_object_point(gcodegen, start_pos + plate_origin_2d), erMixed,
                                        "Travel to a Wipe Tower");
            gcode += gcodegen.unretract();
        }

        // BBS: if needed, write the gcode_label_objects_end then priming tower, if the retract, didn't did it.
        gcodegen.m_writer.add_object_end_labels(gcode);

        double current_z = gcodegen.writer().get_position().z();
        if (z == -1.) // in case no specific z was provided, print at current_z pos
            z = current_z;
        if (!is_approx(z, current_z)) {
            gcode += gcodegen.writer().retract();
            gcode += gcodegen.writer().travel_to_z(z, "Travel down to the last wipe tower layer.");
            gcode += gcodegen.writer().unretract();
        }

        // Process the end filament gcode.
        std::string end_filament_gcode_str;
        if (gcodegen.writer().extruder() != nullptr) {
            // Process the custom filament_end_gcode in case of single_extruder_multi_material.
            unsigned int       old_extruder_id    = gcodegen.writer().extruder()->id();
            const std::string &filament_end_gcode = gcodegen.config().filament_end_gcode.get_at(old_extruder_id);
            if (gcodegen.writer().extruder() != nullptr && !filament_end_gcode.empty()) {
                end_filament_gcode_str = gcodegen.placeholder_parser_process("filament_end_gcode", filament_end_gcode, old_extruder_id);
                check_add_eol(end_filament_gcode_str);
            }
        }
        // BBS: increase toolchange count
        gcodegen.m_toolchange_count++;

        // BBS: should be placed before toolchange parsing
        std::string toolchange_retract_str = gcodegen.retract(true, false);
        check_add_eol(toolchange_retract_str);

        // Process the custom change_filament_gcode. If it is empty, provide a simple Tn command to change the filament.
        // Otherwise, leave control to the user completely.
        std::string        toolchange_gcode_str;
        const std::string &change_filament_gcode = gcodegen.config().change_filament_gcode.value;
        //        m_max_layer_z = std::max(m_max_layer_z, tcr.print_z);
        if (!change_filament_gcode.empty()) {
            DynamicConfig config;
            int           previous_extruder_id = gcodegen.writer().extruder() ? (int) gcodegen.writer().extruder()->id() : -1;
            config.set_key_value("previous_extruder", new ConfigOptionInt(previous_extruder_id));
            config.set_key_value("next_extruder", new ConfigOptionInt((int) new_extruder_id));
            config.set_key_value("layer_num", new ConfigOptionInt(gcodegen.m_layer_index));
            config.set_key_value("layer_z", new ConfigOptionFloat(tcr.print_z));
            config.set_key_value("toolchange_z", new ConfigOptionFloat(z));
            //            config.set_key_value("max_layer_z", new ConfigOptionFloat(m_max_layer_z));
            // BBS
            {
                GCodeWriter     &gcode_writer = gcodegen.m_writer;
                FullPrintConfig &full_config  = gcodegen.m_config;
                float old_retract_length = gcode_writer.extruder() != nullptr ? full_config.retraction_length.get_at(previous_extruder_id) :
                                                                                0;
                float new_retract_length = full_config.retraction_length.get_at(new_extruder_id);
                float old_retract_length_toolchange = gcode_writer.extruder() != nullptr ?
                                                          full_config.retract_length_toolchange.get_at(previous_extruder_id) :
                                                          0;
                float new_retract_length_toolchange = full_config.retract_length_toolchange.get_at(new_extruder_id);
                int   old_filament_temp             = gcode_writer.extruder() != nullptr ?
                                                          (gcodegen.on_first_layer() ?
                                                               full_config.nozzle_temperature_initial_layer.get_at(previous_extruder_id) :
                                                               full_config.nozzle_temperature.get_at(previous_extruder_id)) :
                                                          210;
                int   new_filament_temp = gcodegen.on_first_layer() ? full_config.nozzle_temperature_initial_layer.get_at(new_extruder_id) :
                                                                      full_config.nozzle_temperature.get_at(new_extruder_id);
                Vec3d nozzle_pos        = gcode_writer.get_position();

                float purge_volume  = tcr.purge_volume < EPSILON ? 0 : std::max(tcr.purge_volume, g_min_purge_volume);
                float filament_area = float((M_PI / 4.f) * pow(full_config.filament_diameter.get_at(new_extruder_id), 2));
                float purge_length  = purge_volume / filament_area;

                int old_filament_e_feedrate = gcode_writer.extruder() != nullptr ?
                                                  (int) (60.0 * full_config.filament_max_volumetric_speed.get_at(previous_extruder_id) /
                                                         filament_area) :
                                                  200;
                old_filament_e_feedrate     = old_filament_e_feedrate == 0 ? 100 : old_filament_e_feedrate;
                int new_filament_e_feedrate = (int) (60.0 * full_config.filament_max_volumetric_speed.get_at(new_extruder_id) /
                                                     filament_area);
                new_filament_e_feedrate     = new_filament_e_feedrate == 0 ? 100 : new_filament_e_feedrate;

                config.set_key_value("max_layer_z", new ConfigOptionFloat(gcodegen.m_max_layer_z));
                config.set_key_value("relative_e_axis", new ConfigOptionBool(full_config.use_relative_e_distances));
                config.set_key_value("toolchange_count", new ConfigOptionInt((int) gcodegen.m_toolchange_count));
                // BBS: fan speed is useless placeholer now, but we don't remove it to avoid
                // slicing error in old change_filament_gcode in old 3MF
                config.set_key_value("fan_speed", new ConfigOptionInt((int) 0));
                config.set_key_value("flush_into_skeleton", new ConfigOptionBool(false));

                config.set_key_value("old_retract_length", new ConfigOptionFloat(old_retract_length));
                config.set_key_value("new_retract_length", new ConfigOptionFloat(new_retract_length));
                config.set_key_value("old_retract_length_toolchange", new ConfigOptionFloat(old_retract_length_toolchange));
                config.set_key_value("new_retract_length_toolchange", new ConfigOptionFloat(new_retract_length_toolchange));
                config.set_key_value("old_filament_temp", new ConfigOptionInt(old_filament_temp));
                config.set_key_value("new_filament_temp", new ConfigOptionInt(new_filament_temp));
                config.set_key_value("x_after_toolchange", new ConfigOptionFloat(start_pos(0)));
                config.set_key_value("y_after_toolchange", new ConfigOptionFloat(start_pos(1)));
                config.set_key_value("z_after_toolchange", new ConfigOptionFloat(nozzle_pos(2)));
                config.set_key_value("first_flush_volume", new ConfigOptionFloat(purge_length / 2.f));
                config.set_key_value("second_flush_volume", new ConfigOptionFloat(purge_length / 2.f));
                config.set_key_value("old_filament_e_feedrate", new ConfigOptionInt(old_filament_e_feedrate));
                config.set_key_value("new_filament_e_feedrate", new ConfigOptionInt(new_filament_e_feedrate));
                config.set_key_value("travel_point_1_x", new ConfigOptionFloat(float(travel_point_1.x())));
                config.set_key_value("travel_point_1_y", new ConfigOptionFloat(float(travel_point_1.y())));
                config.set_key_value("travel_point_2_x", new ConfigOptionFloat(float(travel_point_2.x())));
                config.set_key_value("travel_point_2_y", new ConfigOptionFloat(float(travel_point_2.y())));
                config.set_key_value("travel_point_3_x", new ConfigOptionFloat(float(travel_point_3.x())));
                config.set_key_value("travel_point_3_y", new ConfigOptionFloat(float(travel_point_3.y())));

                config.set_key_value("flush_length", new ConfigOptionFloat(purge_length));

                int   flush_count = std::min(g_max_flush_count, (int) std::round(purge_volume / g_purge_volume_one_time));
                float flush_unit  = purge_length / flush_count;
                int   flush_idx   = 0;
                for (; flush_idx < flush_count; flush_idx++) {
                    char key_value[64] = {0};
                    snprintf(key_value, sizeof(key_value), "flush_length_%d", flush_idx + 1);
                    config.set_key_value(key_value, new ConfigOptionFloat(flush_unit));
                }

                for (; flush_idx < g_max_flush_count; flush_idx++) {
                    char key_value[64] = {0};
                    snprintf(key_value, sizeof(key_value), "flush_length_%d", flush_idx + 1);
                    config.set_key_value(key_value, new ConfigOptionFloat(0.f));
                }
            }
            toolchange_gcode_str = gcodegen.placeholder_parser_process("change_filament_gcode", change_filament_gcode, new_extruder_id,
                                                                       &config);
            check_add_eol(toolchange_gcode_str);

            // retract before toolchange
            toolchange_gcode_str = toolchange_retract_str + toolchange_gcode_str;
            // BBS
            {
                // BBS: current position and fan_speed is unclear after interting change_filament_gcode
                check_add_eol(toolchange_gcode_str);
                toolchange_gcode_str += ";_FORCE_RESUME_FAN_SPEED\n";
                gcodegen.writer().set_current_position_clear(false);
                // BBS: check whether custom gcode changes the z position. Update if changed
                double temp_z_after_tool_change;
                if (GCodeProcessor::get_last_z_from_gcode(toolchange_gcode_str, temp_z_after_tool_change)) {
                    Vec3d pos = gcodegen.writer().get_position();
                    pos(2)    = temp_z_after_tool_change;
                    gcodegen.writer().set_position(pos);
                }
            }

            // move to start_pos for wiping after toolchange
            std::string start_pos_str;
            start_pos_str = gcodegen.travel_to(wipe_tower_point_to_object_point(gcodegen, start_pos + plate_origin_2d), erMixed,
                                               "Move to start pos");
            check_add_eol(start_pos_str);
            toolchange_gcode_str += start_pos_str;

            // unretract before wiping
            toolchange_gcode_str += gcodegen.unretract();
            check_add_eol(toolchange_gcode_str);
        }

        std::string toolchange_command;
        if (tcr.priming || (new_extruder_id >= 0 && gcodegen.writer().need_toolchange(new_extruder_id)))
            toolchange_command = gcodegen.writer().toolchange(new_extruder_id);
        if (!custom_gcode_changes_tool(toolchange_gcode_str, gcodegen.writer().toolchange_prefix(), new_extruder_id))
            toolchange_gcode_str += toolchange_command;
        else {
            // We have informed the m_writer about the current extruder_id, we can ignore the generated G-code.
        }

        gcodegen.placeholder_parser().set("current_extruder", new_extruder_id);
        gcodegen.placeholder_parser().set("retraction_distance_when_cut", gcodegen.m_config.retraction_distances_when_cut.get_at(new_extruder_id));
        gcodegen.placeholder_parser().set("long_retraction_when_cut", gcodegen.m_config.long_retractions_when_cut.get_at(new_extruder_id));

        // Process the start filament gcode.
        std::string        start_filament_gcode_str;
        const std::string &filament_start_gcode = gcodegen.config().filament_start_gcode.get_at(new_extruder_id);
        if (!filament_start_gcode.empty()) {
            // Process the filament_start_gcode for the active filament only.
            DynamicConfig config;
            config.set_key_value("filament_extruder_id", new ConfigOptionInt(new_extruder_id));

            //std::vector<double> _position(3, 0);
            //_position[2] = current_z;
            //config.set_key_value("position", new ConfigOptionFloats(_position));
            start_filament_gcode_str = gcodegen.placeholder_parser_process("filament_start_gcode", filament_start_gcode, new_extruder_id,
                                                                           &config);
            check_add_eol(start_filament_gcode_str);
        }

        // Insert the end filament, toolchange, and start filament gcode into the generated gcode.
        DynamicConfig config;
        config.set_key_value("filament_end_gcode", new ConfigOptionString(end_filament_gcode_str));
        config.set_key_value("change_filament_gcode", new ConfigOptionString(toolchange_gcode_str));
        config.set_key_value("filament_start_gcode", new ConfigOptionString(start_filament_gcode_str));
        std::string tcr_gcode,
            tcr_escaped_gcode = gcodegen.placeholder_parser_process("tcr_rotated_gcode", tcr_rotated_gcode, new_extruder_id, &config);
        unescape_string_cstyle(tcr_escaped_gcode, tcr_gcode);
        gcode += gcodegen.inject_wipe_tower_print_acceleration(tcr_gcode);
        check_add_eol(toolchange_gcode_str);

        // SoftFever: set new PA for new filament
        if (gcodegen.config().enable_pressure_advance.get_at(new_extruder_id)) {
            gcode += gcodegen.writer().set_pressure_advance(gcodegen.config().pressure_advance.get_at(new_extruder_id));
        }

        // A phony move to the end position at the wipe tower.
        {
            const bool auto_travel_acceleration_was_suppressed = gcodegen.writer().auto_travel_acceleration_suppressed();
            gcodegen.writer().set_auto_travel_acceleration_suppressed(true);
            gcodegen.writer().travel_to_xy((end_pos + plate_origin_2d).cast<double>());
            gcodegen.writer().set_auto_travel_acceleration_suppressed(auto_travel_acceleration_was_suppressed);
        }
        gcodegen.set_last_pos(wipe_tower_point_to_object_point(gcodegen, end_pos + plate_origin_2d));
        if (!is_approx(z, current_z)) {
            gcode += gcodegen.writer().retract();
            gcode += gcodegen.writer().travel_to_z(current_z, "Travel back up to the topmost object layer.");
            gcode += gcodegen.writer().unretract();
        }

        else {
            // Prepare a future wipe.
            gcodegen.m_wipe.reset_path();
            for (const Vec2f &wipe_pt : tcr.wipe_path)
                gcodegen.m_wipe.path.points.emplace_back(wipe_tower_point_to_object_point(gcodegen, transform_wt_pt(wipe_pt)));
        }

        // Let the planner know we are traveling between objects.
        gcodegen.m_avoid_crossing_perimeters.use_external_mp_once();
        return gcode;
    }

    std::string WipeTowerIntegration::append_tcr2(GCode                             &gcodegen,
                                                  const WipeTower::ToolChangeResult &tcr,
                                                  int                                new_extruder_id,
                                                  double                             z) const
    {
        if (new_extruder_id != -1 && new_extruder_id != tcr.new_tool)
            throw Slic3r::InvalidArgument("Error: WipeTowerIntegration::append_tcr was asked to do a toolchange it didn't expect.");

        //add for CFS
        bool hasToolChange = (tcr.initial_tool != tcr.new_tool) ? true : false;

        std::string gcode;

        // Toolchangeresult.gcode assumes the wipe tower corner is at the origin (except for priming lines)
        // We want to rotate and shift all extrusions (gcode postprocessing) and starting and ending position
        float alpha = m_wipe_tower_rotation / 180.f * float(M_PI);

        auto transform_wt_pt = [&alpha, this](const Vec2f &pt) -> Vec2f {
            Vec2f out = Eigen::Rotation2Df(alpha) * pt;
            out += m_wipe_tower_pos;
            return out;
        };

        Vec2f start_pos = tcr.start_pos;
        Vec2f end_pos   = tcr.end_pos;
        if (!tcr.priming) {
            start_pos = transform_wt_pt(start_pos);
            end_pos   = transform_wt_pt(end_pos);
        }

        Vec2f wipe_tower_offset   = tcr.priming ? Vec2f::Zero() : m_wipe_tower_pos;
        float wipe_tower_rotation = tcr.priming ? 0.f : alpha;
        Vec2f plate_origin_2d(m_plate_origin(0), m_plate_origin(1));


        std::string tcr_rotated_gcode = post_process_wipe_tower_moves(tcr, wipe_tower_offset, wipe_tower_rotation);

        gcode += gcodegen.writer().unlift(); // Make sure there is no z-hop (in most cases, there isn't).

        double current_z = gcodegen.writer().get_position().z();
        if (z == -1.) // in case no specific z was provided, print at current_z pos
            z = current_z;

        const bool needs_toolchange = gcodegen.writer().need_toolchange(new_extruder_id);
        const bool will_go_down     = !is_approx(z, current_z);
        const bool is_ramming       = (gcodegen.config().single_extruder_multi_material) ||
                                (!gcodegen.config().single_extruder_multi_material &&
                                 gcodegen.config().filament_multitool_ramming.get_at(tcr.initial_tool));
        const bool should_travel_to_tower = !tcr.priming && (tcr.force_travel     // wipe tower says so
                                                             || !needs_toolchange // this is just finishing the tower with no toolchange
                                                             || is_ramming);

        if (should_travel_to_tower) {
            // FIXME: It would be better if the wipe tower set the force_travel flag for all toolchanges,
            // then we could simplify the condition and make it more readable.
            gcode += gcodegen.retract();
            // add for CFS
            if (!is_creality_cfs() || !hasToolChange)
            {
                gcodegen.m_avoid_crossing_perimeters.use_external_mp_once();
                gcode += gcodegen.travel_to(wipe_tower_point_to_object_point(gcodegen, start_pos + plate_origin_2d), erMixed, "Travel to a Wipe Tower");
                gcode += gcodegen.unretract();
            } else {
                // for cfs 'EXCLUDE_OBJECT_END'
                gcodegen.m_writer.add_object_change_labels(gcode);
            }
        } else {
            // When this is multiextruder printer without any ramming, we can just change
            // the tool without travelling to the tower.
        }

        // add for CFS
        if (!is_creality_cfs() && will_go_down) {
            gcode += gcodegen.writer().retract();
            gcode += gcodegen.writer().travel_to_z(z, "Travel down to the last wipe tower layer.");
            gcode += gcodegen.writer().unretract();
        }

        std::string toolchange_gcode_str;
        std::string deretraction_str;
        if (tcr.priming || (new_extruder_id >= 0 && needs_toolchange)) {
            if (is_ramming)
                gcodegen.m_wipe.reset_path();                                           // We don't want wiping on the ramming lines.

            // add for CFS
            if (is_creality_cfs() && hasToolChange) {
                toolchange_gcode_str = gcodegen.writer().retract();
                toolchange_gcode_str += gcodegen.set_extruder(new_extruder_id, tcr.print_z); // TODO: toolchange_z vs print_z
            } else {

                Vec2d _pos(m_wipe_tower_pos.x(), m_wipe_tower_pos.y());
                gcodegen.set_tower_pos((_pos));
                toolchange_gcode_str = gcodegen.set_extruder(new_extruder_id, tcr.print_z); // TODO: toolchange_z vs print_z
            }
            if (gcodegen.config().enable_prime_tower)
                deretraction_str = gcodegen.unretract();
        }

        // add for CFS
        if (is_creality_cfs() && will_go_down) {
            toolchange_gcode_str += gcodegen.writer().retract();
            toolchange_gcode_str += gcodegen.writer().travel_to_z(z, "Travel down to the last wipe tower layer.");
            toolchange_gcode_str += gcodegen.writer().unretract();
        }

        // Insert the toolchange and deretraction gcode into the generated gcode.

        DynamicConfig config;
        config.set_key_value("change_filament_gcode", new ConfigOptionString(toolchange_gcode_str));
        config.set_key_value("deretraction_from_wipe_tower_generator", new ConfigOptionString(deretraction_str));

        int previous_extruder_id = gcodegen.writer().extruder() ? (int) gcodegen.writer().extruder()->id() : -1;
        config.set_key_value("previous_extruder", new ConfigOptionInt(previous_extruder_id));
        config.set_key_value("next_extruder", new ConfigOptionInt((int) new_extruder_id));
        config.set_key_value("layer_num", new ConfigOptionInt(gcodegen.m_layer_index));
        config.set_key_value("layer_z", new ConfigOptionFloat(tcr.print_z));
        config.set_key_value("toolchange_z", new ConfigOptionFloat(z));
        GCodeWriter     &gcode_writer = gcodegen.m_writer;
        FullPrintConfig &full_config  = gcodegen.m_config;
        float old_retract_length      = gcode_writer.extruder() != nullptr ? full_config.retraction_length.get_at(previous_extruder_id) : 0;
        float new_retract_length      = full_config.retraction_length.get_at(new_extruder_id);
        float old_retract_length_toolchange = gcode_writer.extruder() != nullptr ?
                                                  full_config.retract_length_toolchange.get_at(previous_extruder_id) :
                                                  0;
        float new_retract_length_toolchange = full_config.retract_length_toolchange.get_at(new_extruder_id);
        int   old_filament_temp             = gcode_writer.extruder() != nullptr ?
                                                  (gcodegen.on_first_layer() ? full_config.nozzle_temperature_initial_layer.get_at(previous_extruder_id) :
                                                                               full_config.nozzle_temperature.get_at(previous_extruder_id)) :
                                                  210;
        int   new_filament_temp = gcodegen.on_first_layer() ? full_config.nozzle_temperature_initial_layer.get_at(new_extruder_id) :
                                                              full_config.nozzle_temperature.get_at(new_extruder_id);
        Vec3d nozzle_pos        = gcode_writer.get_position();

        float purge_volume  = tcr.purge_volume < EPSILON ? 0 : std::max(tcr.purge_volume, g_min_purge_volume);
        float filament_area = float((M_PI / 4.f) * pow(full_config.filament_diameter.get_at(new_extruder_id), 2));
        float purge_length  = purge_volume / filament_area;

        int old_filament_e_feedrate = gcode_writer.extruder() != nullptr ?
                                          (int) (60.0 * full_config.filament_max_volumetric_speed.get_at(previous_extruder_id) /
                                                 filament_area) :
                                          200;
        old_filament_e_feedrate     = old_filament_e_feedrate == 0 ? 100 : old_filament_e_feedrate;
        int new_filament_e_feedrate = (int) (60.0 * full_config.filament_max_volumetric_speed.get_at(new_extruder_id) / filament_area);
        new_filament_e_feedrate     = new_filament_e_feedrate == 0 ? 100 : new_filament_e_feedrate;

        config.set_key_value("max_layer_z", new ConfigOptionFloat(gcodegen.m_max_layer_z));
        config.set_key_value("relative_e_axis", new ConfigOptionBool(full_config.use_relative_e_distances));
        config.set_key_value("toolchange_count", new ConfigOptionInt((int) gcodegen.m_toolchange_count));
        config.set_key_value("fan_speed", new ConfigOptionInt((int) 0));
        config.set_key_value("flush_into_skeleton", new ConfigOptionBool(false));

        config.set_key_value("old_retract_length", new ConfigOptionFloat(old_retract_length));
        config.set_key_value("new_retract_length", new ConfigOptionFloat(new_retract_length));
        config.set_key_value("old_retract_length_toolchange", new ConfigOptionFloat(old_retract_length_toolchange));
        config.set_key_value("new_retract_length_toolchange", new ConfigOptionFloat(new_retract_length_toolchange));
        config.set_key_value("old_filament_temp", new ConfigOptionInt(old_filament_temp));
        config.set_key_value("new_filament_temp", new ConfigOptionInt(new_filament_temp));
        config.set_key_value("x_after_toolchange", new ConfigOptionFloat(start_pos(0)));
        config.set_key_value("y_after_toolchange", new ConfigOptionFloat(start_pos(1)));
        config.set_key_value("z_after_toolchange", new ConfigOptionFloat(nozzle_pos(2)));
        config.set_key_value("first_flush_volume", new ConfigOptionFloat(purge_length / 2.f));
        config.set_key_value("second_flush_volume", new ConfigOptionFloat(purge_length / 2.f));
        config.set_key_value("old_filament_e_feedrate", new ConfigOptionInt(old_filament_e_feedrate));
        config.set_key_value("new_filament_e_feedrate", new ConfigOptionInt(new_filament_e_feedrate));
        config.set_key_value("travel_point_1_x", new ConfigOptionFloat(float(travel_point_1.x())));
        config.set_key_value("travel_point_1_y", new ConfigOptionFloat(float(travel_point_1.y())));
        config.set_key_value("travel_point_2_x", new ConfigOptionFloat(float(travel_point_2.x())));
        config.set_key_value("travel_point_2_y", new ConfigOptionFloat(float(travel_point_2.y())));
        config.set_key_value("travel_point_3_x", new ConfigOptionFloat(float(travel_point_3.x())));
        config.set_key_value("travel_point_3_y", new ConfigOptionFloat(float(travel_point_3.y())));

        int   flush_count = std::min(g_max_flush_count, (int) std::round(purge_volume / g_purge_volume_one_time));
        float flush_unit  = purge_length / flush_count;
        int   flush_idx   = 0;
        for (; flush_idx < flush_count; flush_idx++) {
            char key_value[64] = {0};
            snprintf(key_value, sizeof(key_value), "flush_length_%d", flush_idx + 1);
            config.set_key_value(key_value, new ConfigOptionFloat(flush_unit));
        }

        for (; flush_idx < g_max_flush_count; flush_idx++) {
            char key_value[64] = {0};
            snprintf(key_value, sizeof(key_value), "flush_length_%d", flush_idx + 1);
            config.set_key_value(key_value, new ConfigOptionFloat(0.f));
        }

        std::string tcr_gcode,
            tcr_escaped_gcode = gcodegen.placeholder_parser_process("tcr_rotated_gcode", tcr_rotated_gcode, new_extruder_id, &config);
        unescape_string_cstyle(tcr_escaped_gcode, tcr_gcode);
        gcode += gcodegen.inject_wipe_tower_print_acceleration(tcr_gcode);
        check_add_eol(toolchange_gcode_str);

        // SoftFever: set new PA for new filament
        if (new_extruder_id != -1 && gcodegen.config().enable_pressure_advance.get_at(new_extruder_id)) {
            gcode += gcodegen.writer().set_pressure_advance(gcodegen.config().pressure_advance.get_at(new_extruder_id));
        }

        // A phony move to the end position at the wipe tower.
        {
            const bool auto_travel_acceleration_was_suppressed = gcodegen.writer().auto_travel_acceleration_suppressed();
            gcodegen.writer().set_auto_travel_acceleration_suppressed(true);
            gcodegen.writer().travel_to_xy((end_pos + plate_origin_2d).cast<double>());
            gcodegen.writer().set_auto_travel_acceleration_suppressed(auto_travel_acceleration_was_suppressed);
        }
        gcodegen.set_last_pos(wipe_tower_point_to_object_point(gcodegen, end_pos + plate_origin_2d));
        if (!is_approx(z, current_z)) {
            gcode += gcodegen.writer().retract();
            gcode += gcodegen.writer().travel_to_z(current_z, "Travel back up to the topmost object layer.");
            gcode += gcodegen.writer().unretract();
        }

        else {
            // Prepare a future wipe.
            gcodegen.m_wipe.reset_path();
            for (const Vec2f &wipe_pt : tcr.wipe_path)
                gcodegen.m_wipe.path.points.emplace_back(wipe_tower_point_to_object_point(gcodegen, transform_wt_pt(wipe_pt)));
        }

        // Let the planner know we are traveling between objects.
        gcodegen.m_avoid_crossing_perimeters.use_external_mp_once();
        return gcode;
    }

    Polyline WipeTowerIntegration::generate_path_to_wipe_tower(const Point&       start_pos,
                                                               const Point&       end_pos,
                                                               const BoundingBox& avoid_polygon,
                                                               const BoundingBox& printer_bbx) const
    {
        Polyline    res;
        coord_t     alpha               = scaled(2.f); // offset distance
        BoundingBox avoid_polygon_inner = avoid_polygon;
        avoid_polygon_inner.offset(alpha);
        coord_t width       = avoid_polygon_inner.max[0] - avoid_polygon_inner.min[0];
        Polygon bed_polygon = printer_bbx.polygon();
        Vec2f   v(1, 0); // the first print direction of end_pos.
        if (abs(end_pos[0] - avoid_polygon_inner.min[0]) < width / 2)
            v = -v; // judge whether the wipe tower's infill goes to the left or right.
        // Judge whether the avoid_polygon_inner is outside the printer_bbx.
        // If so, do nothing and just go directly to the end_pos.
        bool   is_bbx_in_bed = true;
        Points avoid_points  = avoid_polygon_inner.polygon().points;
        for (auto& wipe_tower_bbx_p : avoid_points) {
            if (ClipperLib::PointInPolygon(wipe_tower_bbx_p, bed_polygon.points) != 1) {
                is_bbx_in_bed = false;
                break;
            }
        }
        if (!is_bbx_in_bed) {
            res.points.push_back(end_pos);
            return res;
        }
        // Ray-Line Segment Intersection
        auto ray_intersetion_line = [](const Vec2d& a, const Vec2d& v1, const Vec2d& b, const Vec2d& c) -> std::pair<bool, Point> {
            const Vec2d v2    = c - b;
            double      denom = cross2(v1, v2);
            if (fabs(denom) < EPSILON)
                return {false, Point(0, 0)};
            const Vec2d v12    = (a - b);
            double      nume_a = cross2(v2, v12);
            double      nume_b = cross2(v1, v12);
            double      t1     = nume_a / denom;
            double      t2     = nume_b / denom;
            if (t1 >= 0 && t2 >= 0 && t2 <= 1.) {
                // Get the intersection point.
                Vec2d res = a + t1 * v1;
                return std::pair<bool, Point>(true, scaled(res));
            }
            return std::pair<bool, Point>(false, {0, 0});
        };
        struct Inter_info
        {
            int   inter_idx0 = -1;
            Point inter_p;
        };
        auto calc_path_len = [](Points& points, Inter_info& beg_info, Inter_info& end_info,
                                bool is_add) -> std::pair<std::vector<Point>, double> {
            int                beg = is_add ? (beg_info.inter_idx0 + 1) % points.size() : beg_info.inter_idx0;
            int                end = is_add ? end_info.inter_idx0 : (end_info.inter_idx0 + 1) % points.size();
            int                i   = beg;
            double             len = 0;
            std::vector<Point> path;
            path.push_back(beg_info.inter_p);
            len += (unscale(beg_info.inter_p) - unscale(points[beg])).squaredNorm();
            while (i != end) {
                int  ni = is_add ? (i + 1) % points.size() : (i - 1 + points.size()) % points.size();
                auto a  = unscale(points[i]);
                auto b  = unscale(points[ni]);
                len += (a - b).squaredNorm();
                path.push_back(points[i]);
                i = ni;
            }
            path.push_back(points[end]);
            path.push_back(end_info.inter_p);
            len += (unscale(end_info.inter_p) - unscale(points[end])).squaredNorm();
            return {path, len};
        };
        // calculate the intersection point of end_pos along vector v with the avoid_polygon.
        // store in inter_info.
        // represent this intersection by 'p'.
        Inter_info inter_info;
        for (size_t i = 0; i < avoid_points.size(); i++) {
            const auto& a            = avoid_points[i];
            const auto& b            = avoid_points[(i + 1) % avoid_points.size()];
            auto [is_inter, inter_p] = ray_intersetion_line(unscale(end_pos), v.cast<double>(), unscale(a), unscale(b));
            if (is_inter) {
                inter_info.inter_idx0 = i;
                inter_info.inter_p    = inter_p;
                break;
            }
        }
        if (inter_info.inter_idx0 == -1) {
            res.points.push_back(end_pos);
            return res;
        }
        // calculate the other intersection of start_to_p with the avoid_polygon.
        // represent this intersection by 'p_'.
        Inter_info inter_info2;
        Linef      start_to_p(unscale(start_pos), unscale(inter_info.inter_p));
        for (size_t i = 0; i < avoid_points.size(); i++) {
            if (i == inter_info.inter_idx0)
                continue;
            Vec2d a = unscale(avoid_points[i]);
            Vec2d b = unscale(avoid_points[(i + 1) % avoid_points.size()]);
            Linef tower_edge(a, b);
            Vec2d inter;
            if (line_alg::intersection(start_to_p, tower_edge, &inter)) {
                inter_info2.inter_p    = scaled(inter);
                inter_info2.inter_idx0 = i;
                break;
            }
        }
        // if p_ does not exist, go directly to p.
        // else p travels along the shorter path on the wipe_tower_offset_polygon to p_
        if (inter_info2.inter_idx0 == -1) {
            res.points.push_back(inter_info.inter_p);
        } else {
            std::vector<Point> path;
            auto [path1, len1] = calc_path_len(avoid_points, inter_info2, inter_info, true);
            auto [path2, len2] = calc_path_len(avoid_points, inter_info2, inter_info, false);
            path               = len1 < len2 ? path1 : path2;
            for (size_t i = 0; i < path.size(); i++) {
                res.points.push_back(path[i]);
            }
        }
        res.points.push_back(end_pos);
        return res;
    }

     std::string tranGCode(const std::string& input, const std::string& keyword, double z)
    {
        // std::string keyword = "relative_zhop_up_for_firmware ";
        size_t pos = input.find(keyword);

        // ?? G-code ????
        std::string gcode = (pos != std::string::npos) ? input.substr(pos + keyword.length()) : input;

        // ????????? Z ?
        std::regex  zRegex(R"(Z([-+]?\d*\.\d+|\d+))");
        std::smatch match;

        if (std::regex_search(gcode, match, zRegex)) {
            double zValue    = std::stod(match[1]); // ???? Z ?
            double newZValue = zValue + z;          // ???? Z ?

            // ???? Z ??
            std::string newZCmd = "Z" + Slic3r::float_to_string_decimal_point(newZValue, 3); // std::to_string(newZValue);

            // ?? G-code ?? Z ?
            gcode = std::regex_replace(gcode, zRegex, newZCmd);
        }

        return gcode;
    }


    std::string WipeTowerIntegration::post_process_wipe_tower_moves_wipe_head(const WipeTower::ToolChangeResult& tcr,
                                                                              const Vec2f&                       translation,
                                                                              Vec2f&                             end_wipe_pos,
                                                                              std::string&                       wipe_head_path,
                                                                              int&                               wipe_block_type,
                                                                              float                              angle,
                                                                              double                             wipe_tower_z) const
    {
        const double physical_wipe_tower_z = wipe_tower_z == -1. ? tcr.print_z : wipe_tower_z;
        auto extruder_offset_at = [this](int tool_id) -> Vec2f {
            if (tool_id >= 0 && static_cast<size_t>(tool_id) < m_extruder_offsets.size())
                return Vec2f(m_extruder_offsets[static_cast<size_t>(tool_id)].cast<float>());
            return Vec2f::Zero();
        };
        Vec2f extruder_offset = m_single_extruder_multi_material ? extruder_offset_at(0) : extruder_offset_at(tcr.initial_tool);
        std::string        line;
        std::istringstream gcode_str(tcr.gcode);
        std::string        gcode_out;
        Vec2f              pos             = tcr.start_pos;
        Vec2f              transformed_pos = pos;
        Vec2f              old_pos(-1000.1f, -1000.1f);

        Vec2f last_arc_end_pos = pos;
        struct MoveLineInfo
        {
            bool   is_move = false;
            bool   has_xy  = false;
            bool   has_z   = false;
            bool   has_e   = false;
            bool   has_arc = false;
            double e       = 0.;
        };
        auto get_move_line_info = [](const std::string& move_line) {
            MoveLineInfo info;
            if (move_line.find("G1 ") != 0 && move_line.find("G2 ") != 0 && move_line.find("G3 ") != 0)
                return info;

            info.is_move = true;
            std::istringstream line_stream(move_line);
            line_stream >> std::noskipws;
            char ch = 0;
            while (line_stream >> ch) {
                double value = 0.;
                if (ch == 'X' || ch == 'Y') {
                    if (line_stream >> value)
                        info.has_xy = true;
                } else if (ch == 'Z') {
                    if (line_stream >> value)
                        info.has_z = true;
                } else if (ch == 'I' || ch == 'J' || ch == 'R') {
                    if (line_stream >> value)
                        info.has_arc = true;
                } else if (ch == 'E') {
                    if (line_stream >> value) {
                        info.has_e = true;
                        info.e     = value;
                    }
                }
            }
            return info;
        };
        bool        defer_repeat_unretract = false;
        std::string delayed_repeat_unretract;

        while (std::getline(gcode_str, line)) { // we read the gcode line by line

            if (line.find(";avoiding_repeat_unretract") == 0)
            {
                wipe_block_type = 1;
                defer_repeat_unretract = true;
            }

            if (line.find(";will_change_tool") == 0) {
                // isapp = true;
                if (!delayed_repeat_unretract.empty()) {
                    gcode_out += delayed_repeat_unretract + "\n";
                    delayed_repeat_unretract.clear();
                }
                wipe_head_path = gcode_out;
                gcode_out      = "";
                end_wipe_pos   = transformed_pos;
                continue;
            }

            if (line.find("relative_zhop_up_for_firmware ") == 0) {
                // The lift follows custom tool-change G-code, so its base is the current object layer.
                line = tranGCode(line, "relative_zhop_up_for_firmware ", tcr.print_z);
            } else if (line.find("relative_zhop_recovery_for_firmware ") == 0) {
                // Recovery returns to the physical tower layer, which may be compressed by no-sparse-layers mode.
                line = tranGCode(line, "relative_zhop_recovery_for_firmware ", physical_wipe_tower_z);
                gcode_out += line + "\n";
                continue;
            }

            MoveLineInfo move_info = get_move_line_info(line);
            if (defer_repeat_unretract && delayed_repeat_unretract.empty() && move_info.is_move && move_info.has_e && move_info.e > 0. &&
                !move_info.has_xy && !move_info.has_z && !move_info.has_arc) {
                delayed_repeat_unretract = line;
                continue;
            }
            if (!delayed_repeat_unretract.empty() && move_info.is_move && move_info.has_e && move_info.e > 0. &&
                (move_info.has_xy || move_info.has_arc)) {
                gcode_out += delayed_repeat_unretract + "\n";
                delayed_repeat_unretract.clear();
                defer_repeat_unretract = false;
            }

            // All G1 commands should be translated and rotated. X and Y coords are
            // only pushed to the output when they differ from last time.
            // WT generator can override this by appending the never_skip_tag
            if (line.find("G1 ") == 0 || line.find("G2 ") == 0 || line.find("G3 ") == 0) {
                std::string cur_gcode_start = line.find("G1 ") == 0 ? "G1 " : (line.find("G2 ") == 0 ? "G2 " : "G3 ");
                bool        never_skip      = false;
                auto        it              = line.find(WipeTower::never_skip_tag());
                if (it != std::string::npos) {
                    // remove the tag and remember we saw it
                    never_skip = true;
                    line.erase(it, it + WipeTower::never_skip_tag().size());
                }
                std::ostringstream line_out;
                std::istringstream line_str(line);
                line_str >> std::noskipws; // don't skip whitespace

                Vec2f target_pos = pos;
                Vec2f center_offset(0, 0);
                float radius = 0;
                bool  has_IJ = false;
                bool  has_R  = false;

                char ch = 0;
                while (line_str >> ch) {
                    if (ch == 'X' || ch == 'Y')
                        line_str >> (ch == 'X' ? pos.x() : pos.y());
                    else if (ch == 'I')
                        line_str >> center_offset.x(), has_IJ = true;
                    else if (ch == 'J')
                        line_str >> center_offset.y(), has_IJ = true;
                    else if (ch == 'R')
                        line_str >> radius, has_R = true;
                    else
                        line_out << ch;
                }

                transformed_pos = Eigen::Rotation2Df(angle) * pos + translation;
                if (cur_gcode_start == "G2 " || cur_gcode_start == "G3 ") {
                    if (has_IJ) {
                        center_offset = Eigen::Rotation2Df(angle) * center_offset;
                    } else if (has_R) {
                        Vec2f rotated_prev_pos = Eigen::Rotation2Df(angle) * last_arc_end_pos + translation;
                    }
                }

                if (transformed_pos != old_pos || never_skip) {
                    line = line_out.str();
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(3) << cur_gcode_start;
                    if (transformed_pos.x() != old_pos.x() || never_skip)
                        oss << " X" << transformed_pos.x() - extruder_offset.x();
                    if (transformed_pos.y() != old_pos.y() || never_skip)
                        oss << " Y" << transformed_pos.y() - extruder_offset.y();

                    if (cur_gcode_start == "G2 " || cur_gcode_start == "G3 ") {
                        if (has_IJ) {
                            oss << " I" << center_offset.x() << " J" << center_offset.y();
                        } else if (has_R) {
                            oss << " R" << radius;
                        }
                    }

                    oss << " ";
                    line.replace(line.find(cur_gcode_start), 3, oss.str());
                    old_pos = transformed_pos;
                    if (cur_gcode_start == "G2 " || cur_gcode_start == "G3 ") {
                        last_arc_end_pos = target_pos;
                    }
                }
            }

            gcode_out += line + "\n";

            // If this was a toolchange command, we should change current extruder offset
            if (line == "[change_filament_gcode]") {
                // BBS
                if (!m_single_extruder_multi_material) {
                    extruder_offset = extruder_offset_at(tcr.new_tool);

                    // If the extruder offset changed, add an extra move so everything is continuous
                    if (extruder_offset != extruder_offset_at(tcr.initial_tool)) {
                        std::ostringstream oss;
                        oss << std::fixed << std::setprecision(3) << "G1 X" << transformed_pos.x() - extruder_offset.x() << " Y"
                            << transformed_pos.y() - extruder_offset.y() << "\n";
                        gcode_out += oss.str();
                    }
                }

                old_pos          = Vec2f{-1000.1f, -1000.1f};
                pos              = tcr.tool_change_start_pos;
                transformed_pos  = pos;
                last_arc_end_pos = pos;
            }
        }
        if (!delayed_repeat_unretract.empty())
            gcode_out += delayed_repeat_unretract + "\n";
        return gcode_out;
    }




    std::string WipeTowerIntegration::post_process_wipe_tower_moves_wipe(        const WipeTower::ToolChangeResult& tcr, const Vec2f& translation, float angle, float z, bool change_m) const
    {
        if (z == -1.) {
            z = tcr.print_z;
        }
        auto extruder_offset_at = [this](int tool_id) -> Vec2f {
            if (tool_id >= 0 && static_cast<size_t>(tool_id) < m_extruder_offsets.size())
                return Vec2f(m_extruder_offsets[static_cast<size_t>(tool_id)].cast<float>());
            return Vec2f::Zero();
        };
        Vec2f extruder_offset = m_single_extruder_multi_material ? extruder_offset_at(0) : extruder_offset_at(tcr.initial_tool);

        std::istringstream gcode_str(tcr.gcode);
        std::string        gcode_out;
        std::string        line;
        Vec2f              pos             = tcr.start_pos;
        Vec2f              transformed_pos = pos;
        Vec2f              old_pos(-1000.1f, -1000.1f);
        const std::string  prefix = "custom_set_tmp";
        while (std::getline(gcode_str, line)) { // we read the gcode line by line
            if (line.find("relative_zhop_up_for_firmware ") == 0) {
                line = tranGCode(line, "relative_zhop_up_for_firmware ", z);
            } else if (line.find("relative_zhop_recovery_for_firmware ") == 0) {
                line = tranGCode(line, "relative_zhop_recovery_for_firmware ", z);
            } else if (line.find(prefix) == 0) {
                if (change_m) {
                    continue;
                } else {
                    line = line.substr(prefix.length()); // ????
                }
            }
            // All G1 commands should be translated and rotated. X and Y coords are
            // only pushed to the output when they differ from last time.
            // WT generator can override this by appending the never_skip_tag
            if (line.find("G1 ") == 0) {
                bool never_skip = false;
                auto it         = line.find(WipeTower::never_skip_tag());
                if (it != std::string::npos) {
                    // remove the tag and remember we saw it
                    never_skip = true;
                    line.erase(it, it + WipeTower::never_skip_tag().size());
                }
                std::ostringstream line_out;
                std::istringstream line_str(line);
                line_str >> std::noskipws; // don't skip whitespace
                char ch = 0;
                while (line_str >> ch) {
                    if (ch == 'X' || ch == 'Y')
                        line_str >> (ch == 'X' ? pos.x() : pos.y());
                    else
                        line_out << ch;
                }

                transformed_pos = Eigen::Rotation2Df(angle) * pos + translation;

                if (transformed_pos != old_pos || never_skip) {
                    line = line_out.str();
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(3) << "G1 ";
                    if (transformed_pos.x() != old_pos.x() || never_skip)
                        oss << " X" << transformed_pos.x() - extruder_offset.x();
                    if (transformed_pos.y() != old_pos.y() || never_skip)
                        oss << " Y" << transformed_pos.y() - extruder_offset.y();
                    oss << " ";
                    line.replace(line.find("G1 "), 3, oss.str());
                    old_pos = transformed_pos;
                }
            }

            gcode_out += line + "\n";

            // If this was a toolchange command, we should change current extruder offset
            if (line == "[change_filament_gcode]") {
                // BBS
                if (!m_single_extruder_multi_material) {
                    extruder_offset = extruder_offset_at(tcr.new_tool);

                    // If the extruder offset changed, add an extra move so everything is continuous
                    if (extruder_offset != extruder_offset_at(tcr.initial_tool)) {
                        std::ostringstream oss;
                        oss << std::fixed << std::setprecision(3) << "G1 X" << transformed_pos.x() - extruder_offset.x() << " Y"
                            << transformed_pos.y() - extruder_offset.y() << "\n";
                        gcode_out += oss.str();
                    }
                }
            }
        }
        return gcode_out;
    }
    // This function postprocesses gcode_original, rotates and moves all G1 extrusions and returns resulting gcode
    // Starting position has to be supplied explicitely (otherwise it would fail in case first G1 command only contained one coordinate)
    std::string WipeTowerIntegration::post_process_wipe_tower_moves(const WipeTower::ToolChangeResult& tcr, const Vec2f& translation, float angle,float z,bool change_m) const
    {
        if (z == -1.)
        {
            z = tcr.print_z;
        }
        auto extruder_offset_at = [this](int tool_id) -> Vec2f {
            if (tool_id >= 0 && static_cast<size_t>(tool_id) < m_extruder_offsets.size())
                return Vec2f(m_extruder_offsets[static_cast<size_t>(tool_id)].cast<float>());
            return Vec2f::Zero();
        };
        Vec2f extruder_offset = m_single_extruder_multi_material ? extruder_offset_at(0) : extruder_offset_at(tcr.initial_tool);

        std::istringstream gcode_str(tcr.gcode);
        std::string gcode_out;
        std::string line;
        Vec2f pos = tcr.start_pos;
        Vec2f transformed_pos = pos;
        Vec2f old_pos(-1000.1f, -1000.1f);
        const std::string  prefix = "custom_set_tmp";
        while (std::getline(gcode_str, line)) {  // we read the gcode line by line
            if (line.find("relative_zhop_up_for_firmware ")== 0)
            {
                line = tranGCode(line ,"relative_zhop_up_for_firmware ", z);
            }
            else if (line.find("relative_zhop_recovery_for_firmware ")==0)
            {
                line = tranGCode(line, "relative_zhop_recovery_for_firmware ", z);
            } else if (line.find(prefix) == 0)
            {
                if (change_m) {
                    continue;
                } else {
                    line = line.substr(prefix.length()); // ????

                }
            }
            // All G1 commands should be translated and rotated. X and Y coords are
            // only pushed to the output when they differ from last time.
            // WT generator can override this by appending the never_skip_tag
            if (line.find("G1 ") == 0) {
                bool never_skip = false;
                auto it = line.find(WipeTower::never_skip_tag());
                if (it != std::string::npos) {
                    // remove the tag and remember we saw it
                    never_skip = true;
                    line.erase(it, it + WipeTower::never_skip_tag().size());
                }
                std::ostringstream line_out;
                std::istringstream line_str(line);
                line_str >> std::noskipws;  // don't skip whitespace
                char ch = 0;
                while (line_str >> ch) {
                    if (ch == 'X' || ch == 'Y')
                        line_str >> (ch == 'X' ? pos.x() : pos.y());
                    else
                        line_out << ch;
                }

                transformed_pos = Eigen::Rotation2Df(angle) * pos + translation;

                if (transformed_pos != old_pos || never_skip) {
                    line = line_out.str();
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(3) << "G1 ";
                    if (transformed_pos.x() != old_pos.x() || never_skip)
                        oss << " X" << transformed_pos.x() - extruder_offset.x();
                    if (transformed_pos.y() != old_pos.y() || never_skip)
                        oss << " Y" << transformed_pos.y() - extruder_offset.y();
                    oss << " ";
                    line.replace(line.find("G1 "), 3, oss.str());
                    old_pos = transformed_pos;
                }
            }

            gcode_out += line + "\n";

            // If this was a toolchange command, we should change current extruder offset
            if (line == "[change_filament_gcode]") {
                // BBS
                if (!m_single_extruder_multi_material && m_extruder_is_offset) {
                    extruder_offset = extruder_offset_at(tcr.new_tool);

                    // If the extruder offset changed, add an extra move so everything is continuous
                    if (extruder_offset != extruder_offset_at(tcr.initial_tool)) {
                        std::ostringstream oss;
                        oss << std::fixed << std::setprecision(3)
                            << "G1 X" << transformed_pos.x() - extruder_offset.x()
                            << " Y" << transformed_pos.y() - extruder_offset.y()
                            << "\n";
                        gcode_out += oss.str();
                    }
                }
            }
        }
        return gcode_out;
    }

    std::string WipeTowerIntegration::prime(GCode &gcodegen)
    {
        std::string gcode;
        if (!gcodegen.is_BBL_Printer()) {
            for (const WipeTower::ToolChangeResult &tcr : m_priming) {
                if (!tcr.extrusions.empty())
                    gcode += append_tcr2(gcodegen, tcr, tcr.new_tool);
            }
        }
        return gcode;
    }

    std::string WipeTowerIntegration::tool_change(GCode &gcodegen, int extruder_id, bool finish_layer)
    {
        std::string gcode;

        assert(m_layer_idx >= 0);
        if (m_layer_idx >= (int) m_tool_changes.size())
            return gcode;

        if(m_print->m_machine_vender->is_firmwaresoft_mm_printer()) {
            if (gcodegen.writer().need_toolchange(extruder_id) || finish_layer) {
                if (m_layer_idx < (int) m_tool_changes.size()) {
                    if (!(size_t(m_tool_change_idx) < m_tool_changes[m_layer_idx].size()))
                        throw Slic3r::RuntimeError("Wipe tower generation failed, possibly due to empty first layer.");

                    // Calculate where the wipe tower layer will be printed. -1 means that print z will not change,
                    // resulting in a wipe tower with sparse layers.
                    double wipe_tower_z  = -1;
                    bool   ignore_sparse = false;
                    if (gcodegen.config().wipe_tower_no_sparse_layers.value) {
                        wipe_tower_z  = m_last_wipe_tower_print_z;
                        ignore_sparse = (m_tool_changes[m_layer_idx].size() == 1 &&
                                         m_tool_changes[m_layer_idx].front().initial_tool == m_tool_changes[m_layer_idx].front().new_tool &&
                                         m_layer_idx != 0);
                        if (m_tool_change_idx == 0 && !ignore_sparse)
                            wipe_tower_z = m_last_wipe_tower_print_z + m_tool_changes[m_layer_idx].front().layer_height;
                    }

                    if (!ignore_sparse) {
                        gcode += append_tcr_creality(gcodegen, m_tool_changes[m_layer_idx][m_tool_change_idx++], extruder_id, wipe_tower_z);
                        m_last_wipe_tower_print_z = wipe_tower_z;
                    }
                }
            }
        } else if (!gcodegen.is_BBL_Printer()) {
            if (gcodegen.writer().need_toolchange(extruder_id) || finish_layer) {
                if (m_layer_idx < (int) m_tool_changes.size()) {
                    if (!(size_t(m_tool_change_idx) < m_tool_changes[m_layer_idx].size()))
                        throw Slic3r::RuntimeError("Wipe tower generation failed, possibly due to empty first layer.");

                    // Calculate where the wipe tower layer will be printed. -1 means that print z will not change,
                    // resulting in a wipe tower with sparse layers.
                    double wipe_tower_z  = -1;
                    bool   ignore_sparse = false;
                    if (gcodegen.config().wipe_tower_no_sparse_layers.value) {
                        wipe_tower_z  = m_last_wipe_tower_print_z;
                        ignore_sparse = (m_tool_changes[m_layer_idx].size() == 1 &&
                                         m_tool_changes[m_layer_idx].front().initial_tool == m_tool_changes[m_layer_idx].front().new_tool &&
                                         m_layer_idx != 0);
                        if (m_tool_change_idx == 0 && !ignore_sparse)
                        wipe_tower_z = m_last_wipe_tower_print_z + m_tool_changes[m_layer_idx].front().layer_height;
                    }

                    if (!ignore_sparse) {
                        gcode += append_tcr2(gcodegen, m_tool_changes[m_layer_idx][m_tool_change_idx++], extruder_id, wipe_tower_z);
                        m_last_wipe_tower_print_z = wipe_tower_z;
                    }
                }
            }
        }else {
            // Calculate where the wipe tower layer will be printed. -1 means that print z will not change,
            // resulting in a wipe tower with sparse layers.
            double wipe_tower_z  = -1;
            bool   ignore_sparse = false;
            if (gcodegen.config().wipe_tower_no_sparse_layers.value) {
                wipe_tower_z  = m_last_wipe_tower_print_z;
                ignore_sparse = (m_tool_changes[m_layer_idx].size() == 1 &&
                                 m_tool_changes[m_layer_idx].front().initial_tool == m_tool_changes[m_layer_idx].front().new_tool);
                if (m_tool_change_idx == 0 && !ignore_sparse)
                    wipe_tower_z = m_last_wipe_tower_print_z + m_tool_changes[m_layer_idx].front().layer_height;
            }

            if (m_enable_timelapse_print && m_is_first_print) {
                gcode += append_tcr(gcodegen, m_tool_changes[m_layer_idx][0], m_tool_changes[m_layer_idx][0].new_tool, wipe_tower_z);
                m_tool_change_idx++;
                m_is_first_print = false;
            }

            if (gcodegen.writer().need_toolchange(extruder_id) || finish_layer) {
                if (!(size_t(m_tool_change_idx) < m_tool_changes[m_layer_idx].size()))
                    throw Slic3r::RuntimeError("Wipe tower generation failed, possibly due to empty first layer.");

                if (!ignore_sparse) {
                    gcode += append_tcr(gcodegen, m_tool_changes[m_layer_idx][m_tool_change_idx++], extruder_id, wipe_tower_z);
                    m_last_wipe_tower_print_z = wipe_tower_z;
                }
            }
        }

        return gcode;
    }

    bool WipeTowerIntegration::is_empty_wipe_tower_gcode(GCode &gcodegen, int extruder_id, bool finish_layer)
    {
        assert(m_layer_idx >= 0);
        if (m_layer_idx >= (int) m_tool_changes.size())
            return true;

        bool   ignore_sparse = false;
        if (gcodegen.config().wipe_tower_no_sparse_layers.value) {
            ignore_sparse = (m_tool_changes[m_layer_idx].size() == 1 && m_tool_changes[m_layer_idx].front().initial_tool == m_tool_changes[m_layer_idx].front().new_tool);
        }

        if (m_enable_timelapse_print && m_is_first_print) {
            return false;
        }

        if (gcodegen.writer().need_toolchange(extruder_id) || finish_layer) {
            if (!(size_t(m_tool_change_idx) < m_tool_changes[m_layer_idx].size()))
                throw Slic3r::RuntimeError("Wipe tower generation failed, possibly due to empty first layer.");

            if (!ignore_sparse) {
                return false;
            }
        }

        return true;
    }

    // Print is finished. Now it remains to unload the filament safely with ramming over the wipe tower.
    std::string WipeTowerIntegration::finalize(GCode &gcodegen)
    {
        std::string gcode;
        if (!gcodegen.is_BBL_Printer()) {
            if (std::abs(gcodegen.writer().get_position().z() - m_final_purge.print_z) > EPSILON)
                gcode += gcodegen.change_layer(m_final_purge.print_z);
            gcode += append_tcr2(gcodegen, m_final_purge, -1);
        }

        return gcode;
    }

    const std::vector<std::string> ColorPrintColors::Colors = { "#C0392B", "#E67E22", "#F1C40F", "#27AE60", "#1ABC9C", "#2980B9", "#9B59B6" };

#define EXTRUDER_CONFIG(OPT) m_config.OPT.get_at(m_writer.extruder()->id())

void GCode::PlaceholderParserIntegration::reset()
{
    this->failed_templates.clear();
    this->output_config.clear();
    this->opt_position = nullptr;
    this->opt_zhop      = nullptr;
    this->opt_e_position = nullptr;
    this->opt_e_retracted = nullptr;
    this->opt_e_restart_extra = nullptr;
    this->opt_extruded_volume = nullptr;
    this->opt_extruded_weight = nullptr;
    this->opt_extruded_volume_total = nullptr;
    this->opt_extruded_weight_total = nullptr;
    this->num_extruders = 0;
    this->position.clear();
    this->e_position.clear();
    this->e_retracted.clear();
    this->e_restart_extra.clear();
}

void GCode::PlaceholderParserIntegration::init(const GCodeWriter &writer)
{
    this->reset();
    const std::vector<Extruder> &extruders = writer.extruders();
    if (! extruders.empty()) {
        this->num_extruders = extruders.back().id() + 1;
        this->e_retracted.assign(MAXIMUM_EXTRUDER_NUMBER, 0);
        this->e_restart_extra.assign(MAXIMUM_EXTRUDER_NUMBER, 0);
        this->opt_e_retracted = new ConfigOptionFloats(e_retracted);
        this->opt_e_restart_extra = new ConfigOptionFloats(e_restart_extra);
        this->output_config.set_key_value("e_retracted", this->opt_e_retracted);
        this->output_config.set_key_value("e_restart_extra", this->opt_e_restart_extra);
        if (! writer.config.use_relative_e_distances) {
            e_position.assign(MAXIMUM_EXTRUDER_NUMBER, 0);
            opt_e_position = new ConfigOptionFloats(e_position);
            this->output_config.set_key_value("e_position", opt_e_position);
        }
    }
    this->opt_extruded_volume = new ConfigOptionFloats(this->num_extruders, 0.f);
    this->opt_extruded_weight = new ConfigOptionFloats(this->num_extruders, 0.f);
    this->opt_extruded_volume_total = new ConfigOptionFloat(0.f);
    this->opt_extruded_weight_total = new ConfigOptionFloat(0.f);
    this->parser.set("extruded_volume", this->opt_extruded_volume);
    this->parser.set("extruded_weight", this->opt_extruded_weight);
    this->parser.set("extruded_volume_total", this->opt_extruded_volume_total);
    this->parser.set("extruded_weight_total", this->opt_extruded_weight_total);

    // Reserve buffer for current position.
    this->position.assign(3, 0);
    this->opt_position = new ConfigOptionFloats(this->position);
    this->output_config.set_key_value("position", this->opt_position);
    // Store zhop variable into the parser itself, it is a read-only variable to the script.
    this->opt_zhop = new ConfigOptionFloat(writer.get_zhop());
    this->parser.set("zhop", this->opt_zhop);
}

void GCode::PlaceholderParserIntegration::update_from_gcodewriter(const GCodeWriter &writer)
{
    memcpy(this->position.data(), writer.get_position().data(), sizeof(double) * 3);
    this->opt_position->values = this->position;
    this->opt_zhop->value = writer.get_zhop();

    if (this->num_extruders > 0) {
        const std::vector<Extruder> &extruders = writer.extruders();
        assert(! extruders.empty() && num_extruders == extruders.back().id() + 1);
        this->e_retracted.assign(MAXIMUM_EXTRUDER_NUMBER, 0);
        this->e_restart_extra.assign(MAXIMUM_EXTRUDER_NUMBER, 0);
        this->opt_extruded_volume->values.assign(num_extruders, 0);
        this->opt_extruded_weight->values.assign(num_extruders, 0);
        double total_volume = 0.;
        double total_weight = 0.;
        for (const Extruder &e : extruders) {
            this->e_retracted[e.id()]     = e.retracted();
            this->e_restart_extra[e.id()] = e.restart_extra();
            double v = e.extruded_volume();
            double w = v * e.filament_density() * 0.001;
            this->opt_extruded_volume->values[e.id()] = v;
            this->opt_extruded_weight->values[e.id()] = w;
            total_volume += v;
            total_weight += w;
        }
        opt_extruded_volume_total->value = total_volume;
        opt_extruded_weight_total->value = total_weight;
        opt_e_retracted->values = this->e_retracted;
        opt_e_restart_extra->values = this->e_restart_extra;
        if (! writer.config.use_relative_e_distances) {
            this->e_position.assign(MAXIMUM_EXTRUDER_NUMBER, 0);
            for (const Extruder &e : extruders)
                this->e_position[e.id()] = e.position();
            this->opt_e_position->values = this->e_position;
        }
    }
}

// Throw if any of the output vector variables were resized by the script.
void GCode::PlaceholderParserIntegration::validate_output_vector_variables()
{
    if (this->opt_position->values.size() != 3)
        throw Slic3r::RuntimeError("\"position\" output variable must not be resized by the script.");
    if (this->num_extruders > 0) {
        if (this->opt_e_position && this->opt_e_position->values.size() != MAXIMUM_EXTRUDER_NUMBER)
            throw Slic3r::RuntimeError("\"e_position\" output variable must not be resized by the script.");
        if (this->opt_e_retracted->values.size() != MAXIMUM_EXTRUDER_NUMBER)
            throw Slic3r::RuntimeError("\"e_retracted\" output variable must not be resized by the script.");
        if (this->opt_e_restart_extra->values.size() != MAXIMUM_EXTRUDER_NUMBER)
            throw Slic3r::RuntimeError("\"e_restart_extra\" output variable must not be resized by the script.");
    }
}

// Collect pairs of object_layer + support_layer sorted by print_z.
// object_layer & support_layer are considered to be on the same print_z, if they are not further than EPSILON.
std::vector<GCode::LayerToPrint> GCode::collect_layers_to_print(const PrintObject& object)
{
    const bool belt_machine = object.print()->config().machine_is_belt.value;
    std::vector<GCode::LayerToPrint> layers_to_print;
    layers_to_print.reserve(object.layers().size() + object.support_layers().size());

    /*
    // Calculate a minimum support layer height as a minimum over all extruders, but not smaller than 10um.
    // This is the same logic as in support generator.
    //FIXME should we use the printing extruders instead?
    double gap_over_supports = object.config().support_top_z_distance;
    // FIXME should we test object.config().support_material_synchronize_layers ? Currently the support layers are synchronized with object layers iff soluble supports.
    assert(!object.has_support() || gap_over_supports != 0. || object.config().support_material_synchronize_layers);
    if (gap_over_supports != 0.) {
        gap_over_supports = std::max(0., gap_over_supports);
        // Not a soluble support,
        double support_layer_height_min = 1000000.;
        for (auto lh : object.print()->config().min_layer_height.values)
            support_layer_height_min = std::min(support_layer_height_min, std::max(0.01, lh));
        gap_over_supports += support_layer_height_min;
    }*/

    std::vector<std::pair<double, double>> warning_ranges;

    // Pair the object layers with the support layers by z.
    size_t idx_object_layer = 0;
    size_t idx_support_layer = 0;
    const LayerToPrint* last_extrusion_layer = nullptr;
    while (idx_object_layer < object.layers().size() || idx_support_layer < object.support_layers().size()) {
        LayerToPrint layer_to_print;
        double print_z_min = std::numeric_limits<double>::max();
        if (idx_object_layer < object.layers().size()) {
            layer_to_print.object_layer = object.layers()[idx_object_layer++];
            print_z_min = std::min(print_z_min, layer_to_print.object_layer->print_z);
        }

        if (idx_support_layer < object.support_layers().size()) {
            layer_to_print.support_layer = object.support_layers()[idx_support_layer++];
            print_z_min = std::min(print_z_min, layer_to_print.support_layer->print_z);
        }

        if (layer_to_print.object_layer && layer_to_print.object_layer->print_z > print_z_min + EPSILON) {
            layer_to_print.object_layer = nullptr;
            --idx_object_layer;
        }

        if (layer_to_print.support_layer && layer_to_print.support_layer->print_z > print_z_min + EPSILON) {
            layer_to_print.support_layer = nullptr;
            --idx_support_layer;
        }

        layer_to_print.original_object = &object;
        layers_to_print.push_back(layer_to_print);

        bool has_extrusions = (layer_to_print.object_layer && layer_to_print.object_layer->has_extrusions())
            || (layer_to_print.support_layer && layer_to_print.support_layer->has_extrusions());

        // Check that there are extrusions on the very first layer. The case with empty
        // first layer may result in skirt/brim in the air and maybe other issues.
        if (layers_to_print.size() == 1u) {
            if (!has_extrusions && !object.belt())
                throw Slic3r::SlicingError(_(L("One object has empty initial layer and can't be printed. Please Cut the bottom or enable supports.")), object.id().id);
        }

        // In case there are extrusions on this layer, check there is a layer to lay it on.
        if ((layer_to_print.object_layer && layer_to_print.object_layer->has_extrusions())
            // Allow empty support layers, as the support generator may produce no extrusions for non-empty support regions.
            || (layer_to_print.support_layer /* && layer_to_print.support_layer->has_extrusions() */)) {
            double top_cd    = object.config().support_top_z_distance;
            double bottom_cd = object.config().support_bottom_z_distance == 0. ? top_cd : object.config().support_bottom_z_distance;
            // if (!object.print()->config().independent_support_layer_height)
            { // the actual support gap may be larger than the configured one due to rounding to layer height for organic support,
              // regardless of independent support layer height
                top_cd    = std::ceil(top_cd / object.config().layer_height) * object.config().layer_height;
                bottom_cd = std::ceil(bottom_cd / object.config().layer_height) * object.config().layer_height;
            }

            double extra_gap = (layer_to_print.support_layer ? bottom_cd : top_cd);

            // raft contact distance should not trigger any warning
            if(last_extrusion_layer && last_extrusion_layer->support_layer)
                extra_gap = std::max(extra_gap, object.config().raft_contact_distance.value);

            double maximal_print_z = (last_extrusion_layer ? last_extrusion_layer->print_z() : 0.)
                + layer_to_print.layer()->height
                + std::max(0., extra_gap);
            // Negative support_contact_z is not taken into account, it can result in false positives in cases

            if (has_extrusions && layer_to_print.print_z() > maximal_print_z + 2. * EPSILON)
                warning_ranges.emplace_back(std::make_pair((last_extrusion_layer ? last_extrusion_layer->print_z() : 0.), layers_to_print.back().print_z()));
        }
        // Remember last layer with extrusions.
        if (has_extrusions)
            last_extrusion_layer = &layers_to_print.back();
    }

    if (!warning_ranges.empty() && !object.belt()) {
        std::string warning;
        size_t i = 0;
        for (i = 0; i < std::min(warning_ranges.size(), size_t(5)); ++i)
            warning += Slic3r::format(_(L("Object can't be printed for empty layer between %1% and %2%.")),
                                      warning_ranges[i].first, warning_ranges[i].second) + "\n";
        warning += Slic3r::format(_(L("Object: %1%")), object.model_object()->name) + "\n"
            + _(L("Maybe parts of the object at these height are too thin, or the object has faulty mesh"));

        const_cast<Print*>(object.print())->active_step_add_warning(
            PrintStateBase::WarningLevel::CRITICAL, warning, PrintStateBase::SlicingEmptyGcodeLayers);
    }

    return layers_to_print;
}

// Prepare for non-sequential printing of multiple objects: Support resp. object layers with nearly identical print_z
// will be printed for  all objects at once.
// Return a list of <print_z, per object LayerToPrint> items.
std::vector<std::pair<coordf_t, std::vector<GCode::LayerToPrint>>> GCode::collect_layers_to_print(const Print& print)
{
    struct OrderingItem {
        coordf_t    print_z;
        size_t      object_idx;
        size_t      layer_idx;
    };

    std::vector<std::vector<LayerToPrint>>  per_object(print.objects().size(), std::vector<LayerToPrint>());
    std::vector<OrderingItem>               ordering;

    std::vector<Slic3r::SlicingError> errors;

    for (size_t i = 0; i < print.objects().size(); ++i) {
        try {
            per_object[i] = collect_layers_to_print(*print.objects()[i]);
        } catch (const Slic3r::SlicingError &e) {
            errors.push_back(e);
            continue;
        }
        OrderingItem ordering_item;
        ordering_item.object_idx = i;
        ordering.reserve(ordering.size() + per_object[i].size());
        const LayerToPrint& front = per_object[i].front();
        for (const LayerToPrint& ltp : per_object[i]) {
            ordering_item.print_z = ltp.print_z();
            ordering_item.layer_idx = &ltp - &front;
            ordering.emplace_back(ordering_item);
        }
    }

    if (!errors.empty()) { throw Slic3r::SlicingErrors(errors); }

    std::sort(ordering.begin(), ordering.end(), [](const OrderingItem& oi1, const OrderingItem& oi2) { return oi1.print_z < oi2.print_z; });

    std::vector<std::pair<coordf_t, std::vector<LayerToPrint>>> layers_to_print;

    // Merge numerically very close Z values.
    for (size_t i = 0; i < ordering.size();) {
        // Find the last layer with roughly the same print_z.
        size_t j = i + 1;
        coordf_t zmax = ordering[i].print_z + EPSILON;
        for (; j < ordering.size() && ordering[j].print_z <= zmax; ++j);
        // Merge into layers_to_print.
        std::pair<coordf_t, std::vector<LayerToPrint>> merged;
        // Assign an average print_z to the set of layers with nearly equal print_z.
        merged.first = 0.5 * (ordering[i].print_z + ordering[j - 1].print_z);
        merged.second.assign(print.objects().size(), LayerToPrint());
        for (; i < j; ++i) {
            const OrderingItem& oi = ordering[i];
            assert(merged.second[oi.object_idx].layer() == nullptr);
            merged.second[oi.object_idx] = std::move(per_object[oi.object_idx][oi.layer_idx]);
        }
        layers_to_print.emplace_back(std::move(merged));
    }

    return layers_to_print;
}

// free functions called by GCode::do_export()
namespace DoExport {
//    static void update_print_estimated_times_stats(const GCodeProcessor& processor, PrintStatistics& print_statistics)
//    {
//        const GCodeProcessorResult& result = processor.get_result();
//        print_statistics.estimated_normal_print_time = get_time_dhms(result.print_statistics.modes[static_cast<size_t>(PrintEstimatedStatistics::ETimeMode::Normal)].time);
//        print_statistics.estimated_silent_print_time = processor.is_stealth_time_estimator_enabled() ?
//            get_time_dhms(result.print_statistics.modes[static_cast<size_t>(PrintEstimatedStatistics::ETimeMode::Stealth)].time) : "N/A";
//    }

    static void update_print_estimated_stats(const GCodeProcessor& processor, const std::vector<Extruder>& extruders, PrintStatistics& print_statistics, const PrintConfig& config)
    {
        const GCodeProcessorResult& result = processor.get_result();
        const PrintEstimatedStatistics::Mode& normal_mode = result.print_statistics.modes[static_cast<size_t>(PrintEstimatedStatistics::ETimeMode::Normal)];
        print_statistics.estimated_normal_print_time = get_time_dhms(normal_mode.model_time_s());
        print_statistics.estimated_silent_print_time = processor.is_stealth_time_estimator_enabled() ?
            get_time_dhms(result.print_statistics.modes[static_cast<size_t>(PrintEstimatedStatistics::ETimeMode::Stealth)].model_time_s()) : "N/A";

        // update filament statictics
        double total_extruded_volume = 0.0;
        double total_used_filament   = 0.0;
        double total_weight          = 0.0;
        double total_cost            = 0.0;

        auto calc_statistics = [&](const std::map<size_t, double>& total_volumes_per_extruder) {
            for (auto volume : total_volumes_per_extruder) {
                total_extruded_volume += volume.second;

                size_t extruder_id = volume.first;
                auto   extruder    = std::find_if(extruders.begin(), extruders.end(),
                                                  [extruder_id](const Extruder& extr) { return extr.id() == extruder_id; });
                if (extruder == extruders.end())
                    continue;

                double s      = PI * sqr(0.5 * extruder->filament_diameter());
                double weight = volume.second * extruder->filament_density() * 0.001;
                total_used_filament += volume.second / s;
                total_weight += weight;
                total_cost += weight * extruder->filament_cost() * 0.001;
            }
        };

        calc_statistics(result.print_statistics.total_volumes_per_extruder);

        print_statistics.total_extruded_volume = total_extruded_volume;
        print_statistics.total_used_filament   = total_used_filament;
        print_statistics.total_weight          = round(total_weight * 100) / 100.00;
        print_statistics.total_cost            = round(total_cost * 100) / 100.00;

        print_statistics.filament_stats = result.print_statistics.model_volumes_per_extruder;
    }

    static double update_total_weight(const std::vector<Extruder>& extruders)
    {
        double total_weight = 0.0;
        for (auto& extruder : extruders) {
            total_weight += extruder.extruded_volume() * extruder.filament_density() / 1000.0f;
        }
        return total_weight;
    }

    // if any reserved keyword is found, returns a std::vector containing the first MAX_COUNT keywords found
    // into pairs containing:
    // first: source
    // second: keyword
    // to be shown in the warning notification
    // The returned vector is empty if no keyword has been found
    static std::vector<std::pair<std::string, std::string>> validate_custom_gcode(const Print& print) {
        static const unsigned int MAX_TAGS_COUNT = 5;
        std::vector<std::pair<std::string, std::string>> ret;

        auto check = [&ret](const std::string& source, const std::string& gcode) {
            std::vector<std::string> tags;
            if (GCodeProcessor::contains_reserved_tags(gcode, MAX_TAGS_COUNT, tags)) {
                if (!tags.empty()) {
                    size_t i = 0;
                    while (ret.size() < MAX_TAGS_COUNT && i < tags.size()) {
                        ret.push_back({ source, tags[i] });
                        ++i;
                    }
                }
            }
        };

        const GCodeConfig& config = print.config();
        check(_(L("Machine start G-code")), config.machine_start_gcode.value);
        if (ret.size() < MAX_TAGS_COUNT) check(_(L("Machine end G-code")), config.machine_end_gcode.value);
        if (ret.size() < MAX_TAGS_COUNT) check(_(L("Before layer change G-code")), config.before_layer_change_gcode.value);
        if (ret.size() < MAX_TAGS_COUNT) check(_(L("Layer change G-code")), config.layer_change_gcode.value);
        if (ret.size() < MAX_TAGS_COUNT) check(_(L("Time lapse G-code")), config.time_lapse_gcode.value);
        if (ret.size() < MAX_TAGS_COUNT) check(_(L("Change filament G-code")), config.change_filament_gcode.value);
        if (ret.size() < MAX_TAGS_COUNT) check(_(L("Printing by object G-code")), config.printing_by_object_gcode.value);
        //if (ret.size() < MAX_TAGS_COUNT) check(_(L("Color Change G-code")), config.color_change_gcode.value);
        //Orca
        if (ret.size() < MAX_TAGS_COUNT) check(_(L("Change extrusion role G-code")), config.change_extrusion_role_gcode.value);
        if (ret.size() < MAX_TAGS_COUNT) check(_(L("Pause G-code")), config.machine_pause_gcode.value);
        if (ret.size() < MAX_TAGS_COUNT) check(_(L("Template Custom G-code")), config.template_custom_gcode.value);
        if (ret.size() < MAX_TAGS_COUNT) {
            for (const std::string& value : config.filament_start_gcode.values) {
                check(_(L("Filament start G-code")), value);
                if (ret.size() == MAX_TAGS_COUNT)
                    break;
            }
        }
        if (ret.size() < MAX_TAGS_COUNT) {
            for (const std::string& value : config.filament_end_gcode.values) {
                check(_(L("Filament end G-code")), value);
                if (ret.size() == MAX_TAGS_COUNT)
                    break;
            }
        }
        //BBS: no custom_gcode_per_print_z, don't need to check
        //if (ret.size() < MAX_TAGS_COUNT) {
        //    const CustomGCode::Info& custom_gcode_per_print_z = print.model().custom_gcode_per_print_z;
        //    for (const auto& gcode : custom_gcode_per_print_z.gcodes) {
        //        check(_(L("Custom G-code")), gcode.extra);
        //        if (ret.size() == MAX_TAGS_COUNT)
        //            break;
        //    }
        //}

        return ret;
    }
} // namespace DoExport

bool GCode::is_BBL_Printer()
{
    if (m_curr_print)
        return m_curr_print->is_BBL_printer();
    return false;
}

bool GCode::is_CX_printer()
{
    if (m_curr_print)
        return m_curr_print->is_CX_printer();
    return false;
}

double GCode::getLimitSpeed()
{
    double limitSpeed = 0.0f;
    if (m_config.acceleration_limit_mess_enable
        || m_config.speed_limit_to_height_enable)
    {
        double weight = 0.0f;
        weight = DoExport::update_total_weight(m_writer.extruders());
        if (m_config.acceleration_limit_mess_enable
            || m_config.speed_limit_to_height_enable)
        {
            double F = std::numeric_limits<int>::max();
            m_smoothSpeedAcc->detect_speed(F, weight, m_last_layer_z);
            if (F < std::numeric_limits<int>::max() && F > 0.0f)
            {
                limitSpeed = F;
            }
        }
    }
    return limitSpeed;
}

void GCode::do_export(Print* print, const char* path, GCodeProcessorResult* result, ThumbnailsGeneratorCallback thumbnail_cb)
{
    BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << " start " << " export path " << path;

    PROFILE_CLEAR();

    // BBS
    m_curr_print = print;

    GCodeWriter::full_gcode_comment = print->config().gcode_comments;
    CNumericLocalesSetter locales_setter;

    // Does the file exist? If so, we hope that it is still valid.
    if (print->is_step_done(psGCodeExport) && boost::filesystem::exists(boost::filesystem::path(path)))
        return;

    BOOST_LOG_TRIVIAL(error) << boost::format("Will export G-code to %1% soon")%path;
    GCodeProcessor::s_IsBBLPrinter = print->is_BBL_printer();
    print->set_started(psGCodeExport);

    // check if any custom gcode contains keywords used by the gcode processor to
    // produce time estimation and gcode toolpaths
    std::vector<std::pair<std::string, std::string>> validation_res = DoExport::validate_custom_gcode(*print);
    if (!validation_res.empty()) {
        std::string reports;
        for (const auto& [source, keyword] : validation_res) {
            reports += source + ": \"" + keyword + "\"\n";
        }
        //print->active_step_add_warning(PrintStateBase::WarningLevel::NON_CRITICAL,
        //    _(L("In the custom G-code were found reserved keywords:")) + "\n" +
        //    reports +
        //    _(L("This may cause problems in g-code visualization and printing time estimation.")));
        std::string temp = "Dangerous keywords in custom Gcode: " + reports + "\nThis may cause problems in g-code visualization and printing time estimation.";
        BOOST_LOG_TRIVIAL(warning) << temp;
    }

    BOOST_LOG_TRIVIAL(error) << "Exporting G-code..." << log_memory_info();

    // Remove the old g-code if it exists.

    boost::nowide::remove(path);

    fs::path file_path(path);
    fs::path folder = file_path.parent_path();
    if (!fs::exists(folder)) {
        fs::create_directory(folder);
        BOOST_LOG_TRIVIAL(error) << "[WARNING]: the parent path " + folder.string() +" is not there, create it!" << std::endl;
    }

    std::string path_tmp(path);
    path_tmp += ".tmp";

    m_processor.initialize(path_tmp);
    GCodeOutputStream file(boost::nowide::fopen(path_tmp.c_str(), "wb"), m_processor);
    if (! file.is_open()) {
        BOOST_LOG_TRIVIAL(error) << std::string("G-code export to ") + path + " failed.\nCannot open the file for writing.\n" << std::endl;
        if (!fs::exists(folder)) {
            //fs::create_directory(folder);
            BOOST_LOG_TRIVIAL(error) << "the parent path " + folder.string() +" is not there!!!" << std::endl;
        }
        throw Slic3r::RuntimeError(std::string("G-code export to ") + path + " failed.\nCannot open the file for writing.\n");
    }

    m_processor.s_IsCFSPrinter = print->getCrealityCFS();
    m_processor.s_creality_flush_time = 0;// print->config().creality_flush_time;
    BOOST_LOG_TRIVIAL(error) << "[PERF_TIMING] GCODE_GENERATE START";
    auto _perf_generate_start = std::chrono::steady_clock::now();
    try {
        this->_do_export(*print, file, thumbnail_cb);
        file.flush();
        if (file.is_error()) {
            file.close();
            boost::nowide::remove(path_tmp.c_str());
            throw Slic3r::RuntimeError(std::string("G-code export to ") + path + " failed\nIs the disk full?\n");
        }
    } catch (std::exception & /* ex */) {
        // Rethrow on any exception. std::runtime_exception and CanceledException are expected to be thrown.
        // Close and remove the file.
        file.close();
        boost::nowide::remove(path_tmp.c_str());
        throw;
    }
    file.close();
    //file.write_md5(path_tmp);
    check_placeholder_parser_failed();

#if ORCA_CHECK_GCODE_PLACEHOLDERS
    if (!m_placeholder_error_messages.empty()){
        std::ostringstream message;
        message << "Some EditGcodeDialog defs were not specified properly. Do so in PrintConfig under SlicingStatesConfigDef:" << std::endl;
        for (const auto& error : m_placeholder_error_messages) {
            message << std::endl << error.first << ": " << std::endl;
            for (const auto& str : error.second)
                message << str << ", ";
            message.seekp(-2, std::ios_base::end);
            message << std::endl;
        }
        throw Slic3r::PlaceholderParserError(message.str());
    }
#endif

    BOOST_LOG_TRIVIAL(error) << "Start processing gcode, " << log_memory_info();
    { auto _ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - _perf_generate_start).count();
      BOOST_LOG_TRIVIAL(error) << "[PERF_TIMING] GCODE_GENERATE END elapsed=" << _ms << "ms"; }
    BOOST_LOG_TRIVIAL(error) << "[PERF_TIMING] GCODE_PROCESSING START";
    auto _perf_processing_start = std::chrono::steady_clock::now();
    // Post-process the G-code to update time stamps.

    m_timelapse_warning_code = 0;
    if (m_config.printer_structure.value == PrinterStructure::psI3 && m_spiral_vase) {
        m_timelapse_warning_code += 1;
    }
    if (m_config.printer_structure.value == PrinterStructure::psI3 && print->config().print_sequence == PrintSequence::ByObject) {
        m_timelapse_warning_code += (1 << 1);
    }
    m_processor.result().timelapse_warning_code = m_timelapse_warning_code;
    m_processor.result().support_traditional_timelapse = m_support_traditional_timelapse;

    bool activate_long_retraction_when_cut = false;
    for (const auto& extruder : m_writer.extruders())
        activate_long_retraction_when_cut |= (
            m_config.long_retractions_when_cut.get_at(extruder.id())
         && m_config.retraction_distances_when_cut.get_at(extruder.id()) > 0
            );

    m_processor.result().long_retraction_when_cut = activate_long_retraction_when_cut;

    {   //BBS:check bed and filament compatible
        const ConfigOptionDef *bed_type_def = print_config_def.get("curr_bed_type");
        assert(bed_type_def != nullptr);
        const t_config_enum_values *bed_type_keys_map = bed_type_def->enum_keys_map;
        const ConfigOptionInts *bed_temp_opt = m_config.option<ConfigOptionInts>(get_bed_temp_key(m_config.curr_bed_type));
        for(auto extruder_id : m_initial_layer_extruders){
            int cur_bed_temp = bed_temp_opt->get_at(extruder_id);
            if (cur_bed_temp == 0 && bed_type_keys_map != nullptr) {
                for (auto item : *bed_type_keys_map) {
                    if (item.second == m_config.curr_bed_type) {
                        m_processor.result().bed_match_result = BedMatchResult(false, item.first, extruder_id);
                        break;
                    }
                }
            }
            if (m_processor.result().bed_match_result.match == false)
                break;
        }
    }

    m_processor.finalize(true, print->m_print_statistics.total_used_filament, print->config().creality_flush_time,print->config().multicolor_method);
    try {
        sync_default_filament_metadata_with_rendered_tools(print->full_print_config(), path_tmp, m_processor.result());
    } catch (...) {
        boost::nowide::remove(path_tmp.c_str());
        throw;
    }
    //file.write_md5(path_tmp);
    //    DoExport::update_print_estimated_times_stats(m_processor, print->m_print_statistics);
    DoExport::update_print_estimated_stats(m_processor, m_writer.extruders(), print->m_print_statistics, print->config());

#ifdef SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
    BOOST_LOG_TRIVIAL(info) << "skip layer-time auto export on slice export: only gcode load is supported";
#endif // SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
    if (result != nullptr) {
        // Cache interest regions for GUI preview (available only for sliced 3mf workflow).
        {
            const auto& moves = m_processor.get_result().moves;

            std::vector<size_t> ssid_to_moveid_map;
            ssid_to_moveid_map.reserve(moves.size());
            for (size_t move_id = 0; move_id < moves.size(); ++move_id) {
                if (moves[move_id].type != EMoveType::Seam)
                    ssid_to_moveid_map.push_back(move_id);
            }

            const InterestRegion::AppearanceUnderExtrusionDefinition def;
            const auto& object_ids = m_processor.get_result().object_id_by_move_id;

            // Build per-object safe params (label object id -> accel_safe / velocity_safe) for dynamic defect span cap.
            std::unordered_map<int, std::pair<float, float>> safe_params_by_object_id;
            safe_params_by_object_id.reserve(print->num_object_instances());
            for (const PrintObject* obj : print->objects()) {
                const PrintObjectConfig& ocfg = obj->config();
                const float accel_safe = static_cast<float>(ocfg.msao_safe_accel.value);
                const float v_safe     = static_cast<float>(ocfg.msao_safe_velocity.value);
                for (const PrintInstance& inst : obj->instances()) {
                    const int label_id = inst.model_instance->get_labeled_id();
                    safe_params_by_object_id[label_id] = { accel_safe, v_safe };
                }
            }

            const PrintObjectConfig& default_ocfg  = print->default_object_config();
            const float             default_accel = static_cast<float>(default_ocfg.msao_safe_accel.value);
            const float             default_v_safe = static_cast<float>(default_ocfg.msao_safe_velocity.value);

            const AppearanceUnderExtrusionAccelRecoveryConfig aue_params;
            auto defect_cap_base_mm = [&](float v_low_mm_s, int object_id) -> double {
                float accel_safe = default_accel;
                float v_safe     = default_v_safe;
                if (auto it = safe_params_by_object_id.find(object_id); it != safe_params_by_object_id.end()) {
                    accel_safe = it->second.first;
                    v_safe     = it->second.second;
                }
                if (!(accel_safe > 0.0f) || !(v_safe > 0.0f))
                    return 0.0;
                return InterestRegion::compute_aue_L_safe_total_mm(v_low_mm_s, v_safe, accel_safe,
                                                                   aue_params.L_safe_transition_mm, aue_params.L_safe_cruise_mm);
            };

            const InterestRegion::InterestRegion region =
                InterestRegion::detect_appearance_under_extrusion_interest_region(moves, ssid_to_moveid_map, def, &object_ids, defect_cap_base_mm);

            if (region.empty()) {
                m_processor.result().custom_interest_by_move_id.clear();
            } else {
                std::vector<unsigned char>& cache = m_processor.result().custom_interest_by_move_id;
                cache.assign(moves.size(), 0);

                for (const auto& obj : region.objects) {
                    if (!obj)
                        continue;
                    for (const InterestRegion::TaggedSpan& ts : obj->spans()) {
                        const size_t first = std::max<size_t>(ts.span.first_end_ssid, 1);
                        const size_t last  = std::min(ts.span.last_end_ssid, ssid_to_moveid_map.size() - 1);
                        if (last < first)
                            continue;

                        const unsigned char v = static_cast<unsigned char>(ts.tag);
                        for (size_t end_ssid = first; end_ssid <= last; ++end_ssid) {
                            const size_t move_id_end = ssid_to_moveid_map[end_ssid];
                            if (move_id_end < cache.size())
                                cache[move_id_end] = std::max(cache[move_id_end], v);
                        }
                    }
                }
            }
        }
        //*result = std::move(m_processor.extract_result());
        result->take(m_processor.extract_result());   //optimize:replace copy assign
        // set the filename to the correct value
        result->filename = path;
    }

    //BBS: add some log for error output
    BOOST_LOG_TRIVIAL(error) << boost::format("Finished processing gcode to %1% ") % path_tmp;

    std::error_code ret = rename_file(path_tmp, path);
    if (ret) {
        BOOST_LOG_TRIVIAL(error) << (std::string("Failed to rename the output G-code file from ") + path_tmp + " to " + path + '\n' +
                                     "error code " + ret.message() + '\n' + "Is " + path_tmp + " locked?" + '\n');
        throw Slic3r::RuntimeError(
            std::string("Failed to rename the output G-code file from ") + path_tmp + " to " + path + '\n' + "error code " + ret.message() + '\n' +
            "Is " + path_tmp + " locked?" + '\n');
    }
    else {
        BOOST_LOG_TRIVIAL(error) << boost::format("rename_file from %1% to %2% successfully")% path_tmp % path;
    }

        { auto _ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - _perf_processing_start).count();
      BOOST_LOG_TRIVIAL(error) << "[PERF_TIMING] GCODE_PROCESSING END elapsed=" << _ms << "ms"; }
BOOST_LOG_TRIVIAL(error) << "Exporting G-code finished" << log_memory_info();
    print->set_done(psGCodeExport);

    if(is_BBL_Printer())
        result->label_object_enabled = m_enable_exclude_object;

    // Write the profiler measurements to file
    PROFILE_UPDATE();
    PROFILE_OUTPUT(debug_out_path("gcode-export-profile.txt").c_str());
    BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << " end";
}

// free functions called by GCode::_do_export()
namespace DoExport {
    static void init_gcode_processor(const PrintConfig& config, GCodeProcessor& processor, bool& silent_time_estimator_enabled)
    {
        silent_time_estimator_enabled = (config.gcode_flavor == gcfMarlinLegacy || config.gcode_flavor == gcfMarlinFirmware)
                                        && config.silent_mode;
        processor.reset();
        processor.apply_config(config);
        processor.enable_stealth_time_estimator(silent_time_estimator_enabled);
    }

#if 0
	static double autospeed_volumetric_limit(const Print &print)
	{
	    // get the minimum cross-section used in the print
	    std::vector<double> mm3_per_mm;
	    for (auto object : print.objects()) {
	        for (size_t region_id = 0; region_id < object->num_printing_regions(); ++ region_id) {
	            const PrintRegion &region = object->printing_region(region_id);
	            for (auto layer : object->layers()) {
	                const LayerRegion* layerm = layer->regions()[region_id];
	                if (region.config().get_abs_value("inner_wall_speed") == 0 ||
                        // BBS: remove small small_perimeter_speed config, and will absolutely
                        // remove related code if no other issue in the coming release.
	                    //region.config().get_abs_value("small_perimeter_speed") == 0 ||
	                    region.config().outer_wall_speed.value == 0 ||
	                    region.config().get_abs_value("bridge_speed") == 0)
	                    mm3_per_mm.push_back(layerm->perimeters.min_mm3_per_mm());
	                if (region.config().get_abs_value("sparse_infill_speed") == 0 ||
	                    region.config().get_abs_value("internal_solid_infill_speed") == 0 ||
	                    region.config().get_abs_value("top_surface_speed") == 0 ||
                        region.config().get_abs_value("bridge_speed") == 0)
                    {
                        // Minimal volumetric flow should not be calculated over ironing extrusions.
                        // Use following lambda instead of the built-it method.
                        auto min_mm3_per_mm_no_ironing = [](const ExtrusionEntityCollection& eec) -> double {
                            double min = std::numeric_limits<double>::max();
                            for (const ExtrusionEntity* ee : eec.entities)
                                if (ee->role() != erIroning)
                                    min = std::min(min, ee->min_mm3_per_mm());
                            return min;
                        };

                        mm3_per_mm.push_back(min_mm3_per_mm_no_ironing(layerm->fills));
                    }
	            }
	        }
	        if (object->config().get_abs_value("support_speed") == 0 ||
	            object->config().get_abs_value("support_interface_speed") == 0)
	            for (auto layer : object->support_layers())
	                mm3_per_mm.push_back(layer->support_fills.min_mm3_per_mm());
	    }
	    // filter out 0-width segments
	    mm3_per_mm.erase(std::remove_if(mm3_per_mm.begin(), mm3_per_mm.end(), [](double v) { return v < 0.000001; }), mm3_per_mm.end());
	    double volumetric_speed = 0.;
	    if (! mm3_per_mm.empty()) {
	        // In order to honor max_print_speed we need to find a target volumetric
	        // speed that we can use throughout the print. So we define this target
	        // volumetric speed as the volumetric speed produced by printing the
	        // smallest cross-section at the maximum speed: any larger cross-section
	        // will need slower feedrates.
	        volumetric_speed = *std::min_element(mm3_per_mm.begin(), mm3_per_mm.end()) * print.config().max_print_speed.value;
	        // limit such volumetric speed with max_volumetric_speed if set
            //BBS
	        //if (print.config().max_volumetric_speed.value > 0)
	        //    volumetric_speed = std::min(volumetric_speed, print.config().max_volumetric_speed.value);
	    }
	    return volumetric_speed;
	}
#endif

    static void init_ooze_prevention(const Print &print, OozePrevention &ooze_prevention)
	{
        ooze_prevention.enable = print.config().ooze_prevention.value && !print.config().single_extruder_multi_material;
	}

	// Fill in print_statistics and return formatted string containing filament statistics to be inserted into G-code comment section.
    static std::string update_print_stats_and_format_filament_stats(
        const bool                   has_wipe_tower,
	    const WipeTowerData         &wipe_tower_data,
	    const std::vector<Extruder> &extruders,
		PrintStatistics 		    &print_statistics)
    {
		std::string filament_stats_string_out;

	    print_statistics.clear();
        print_statistics.total_toolchanges = std::max(0, wipe_tower_data.number_of_toolchanges);
	    if (! extruders.empty()) {
	        std::pair<std::string, unsigned int> out_filament_used_mm ("; filament used [mm] = ", 0);
	        std::pair<std::string, unsigned int> out_filament_used_cm3("; filament used [cm3] = ", 0);
	        std::pair<std::string, unsigned int> out_filament_used_g  ("; filament used [g] = ", 0);
	        std::pair<std::string, unsigned int> out_filament_cost    ("; filament cost = ", 0);
	        for (const Extruder &extruder : extruders) {
	            double used_filament   = extruder.used_filament() + (has_wipe_tower ? wipe_tower_data.used_filament[extruder.id()] : 0.f);
	            double extruded_volume = extruder.extruded_volume() + (has_wipe_tower ? wipe_tower_data.used_filament[extruder.id()] * extruder.filament_crossection() : 0.f);
	            double filament_weight = extruded_volume * extruder.filament_density() * 0.001;
	            double filament_cost   = filament_weight * extruder.filament_cost()    * 0.001;
                auto append = [&extruder](std::pair<std::string, unsigned int> &dst, const char *tmpl, double value) {
                    assert(is_decimal_separator_point());
	                while (dst.second < extruder.id()) {
	                    // Fill in the non-printing extruders with zeros.
	                    dst.first += (dst.second > 0) ? ", 0" : "0";
	                    ++ dst.second;
	                }
	                if (dst.second > 0)
	                    dst.first += ", ";
	                char buf[64];
					sprintf(buf, tmpl, value);
	                dst.first += buf;
	                ++ dst.second;
	            };
	            append(out_filament_used_mm,  "%.2lf", used_filament);
	            append(out_filament_used_cm3, "%.2lf", extruded_volume * 0.001);
	            if (filament_weight > 0.) {
	                print_statistics.total_weight = print_statistics.total_weight + filament_weight;
	                append(out_filament_used_g, "%.2lf", filament_weight);
	                if (filament_cost > 0.) {
	                    print_statistics.total_cost = print_statistics.total_cost + filament_cost;
	                    append(out_filament_cost, "%.2lf", filament_cost);
	                }
	            }
	            print_statistics.total_used_filament += used_filament;
	            print_statistics.total_extruded_volume += extruded_volume;
	            print_statistics.total_wipe_tower_filament += has_wipe_tower ? used_filament - extruder.used_filament() : 0.;
	            print_statistics.total_wipe_tower_cost += has_wipe_tower ? (extruded_volume - extruder.extruded_volume())* extruder.filament_density() * 0.001 * extruder.filament_cost() * 0.001 : 0.;
	        }
	        filament_stats_string_out += out_filament_used_mm.first;
            filament_stats_string_out += "\n" + out_filament_used_cm3.first;
            if (out_filament_used_g.second)
                filament_stats_string_out += "\n" + out_filament_used_g.first;
            if (out_filament_cost.second)
               filament_stats_string_out += "\n" + out_filament_cost.first;
        }
        return filament_stats_string_out;
    }
}

#if 0
// Sort the PrintObjects by their increasing Z, likely useful for avoiding colisions on Deltas during sequential prints.
static inline std::vector<const PrintInstance*> sort_object_instances_by_max_z(const Print &print)
{
    std::vector<const PrintObject*> objects(print.objects().begin(), print.objects().end());
    std::sort(objects.begin(), objects.end(), [](const PrintObject *po1, const PrintObject *po2) { return po1->height() < po2->height(); });
    std::vector<const PrintInstance*> instances;
    instances.reserve(objects.size());
    for (const PrintObject *object : objects)
        for (size_t i = 0; i < object->instances().size(); ++ i)
            instances.emplace_back(&object->instances()[i]);
    return instances;
}
#endif

// Produce a vector of PrintObjects in the order of their respective ModelObjects in print.model().
//BBS: add sort logic for seq-print
std::vector<const PrintInstance*> sort_object_instances_by_model_order(const Print& print, bool init_order)
{
    auto find_object_index = [](const Model& model, const ModelObject* obj) {
        for (int index = 0; index < model.objects.size(); index++)
        {
            if (model.objects[index] == obj)
                return index;
        }
        return -1;
    };

    // Build up map from ModelInstance* to PrintInstance*
    std::vector<std::pair<const ModelInstance*, const PrintInstance*>> model_instance_to_print_instance;
    model_instance_to_print_instance.reserve(print.num_object_instances());
    for (const PrintObject *print_object : print.objects())
        for (const PrintInstance &print_instance : print_object->instances())
        {
            if (init_order)
                const_cast<ModelInstance*>(print_instance.model_instance)->arrange_order = find_object_index(print.model(), print_object->model_object());
            model_instance_to_print_instance.emplace_back(print_instance.model_instance, &print_instance);
        }
    std::sort(model_instance_to_print_instance.begin(), model_instance_to_print_instance.end(), [](auto &l, auto &r) { return l.first->arrange_order < r.first->arrange_order; });
    if (init_order) {
        // Re-assign the arrange_order so each instance has a unique order number
        for (int k = 0; k < model_instance_to_print_instance.size(); k++) {
            const_cast<ModelInstance*>(model_instance_to_print_instance[k].first)->arrange_order = k + 1;
        }
    }

    std::vector<const PrintInstance*> instances;
    instances.reserve(model_instance_to_print_instance.size());
    for (const ModelObject *model_object : print.model().objects)
        for (const ModelInstance *model_instance : model_object->instances) {
            auto it = std::lower_bound(model_instance_to_print_instance.begin(), model_instance_to_print_instance.end(), std::make_pair(model_instance, nullptr), [](auto &l, auto &r) { return l.first->arrange_order < r.first->arrange_order; });
            if (it != model_instance_to_print_instance.end() && it->first == model_instance)
                instances.emplace_back(it->second);
        }
    std::sort(instances.begin(), instances.end(), [](auto& l, auto& r) { return l->model_instance->arrange_order < r->model_instance->arrange_order; });
    return instances;
}

enum BambuBedType {
    bbtUnknown = 0,
    bbtCoolPlate = 1,
    bbtEngineeringPlate = 2,
    bbtHighTemperaturePlate = 3,
    bbtTexturedPEIPlate         = 4,
};

static BambuBedType to_bambu_bed_type(BedType type)
{
    BambuBedType bambu_bed_type = bbtUnknown;
    if (type == btPC)
        bambu_bed_type = bbtCoolPlate;
    else if (type == btEP)
        bambu_bed_type = bbtEngineeringPlate;
    else if (type == btPEI)
        bambu_bed_type = bbtHighTemperaturePlate;
    else if (type == btPTE)
        bambu_bed_type = bbtTexturedPEIPlate;

    return bambu_bed_type;
}

void GCode::_do_export(Print& print, GCodeOutputStream &file, ThumbnailsGeneratorCallback thumbnail_cb)
{
    BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << " start";
    PROFILE_FUNC();

    // modifies m_silent_time_estimator_enabled
    DoExport::init_gcode_processor(print.config(), m_processor, m_silent_time_estimator_enabled);
    const bool is_bbl_printers = print.is_BBL_printer();
    m_calib_config.clear();
    // resets analyzer's tracking data
    m_last_height  = 0.f;
    m_last_layer_z = 0.f;
    m_max_layer_z  = 0.f;
    m_last_width = 0.f;
    m_is_overhang_fan_on = false;
    m_is_supp_interface_fan_on = false;
#if ENABLE_GCODE_VIEWER_DATA_CHECKING
    m_last_mm3_per_mm = 0.;
#endif // ENABLE_GCODE_VIEWER_DATA_CHECKING

    m_fan_mover.release();

    m_writer.set_is_bbl_machine(is_bbl_printers);

    // How many times will be change_layer() called?
    // change_layer() in turn increments the progress bar status.
    m_layer_count = 0;
    if (print.config().print_sequence == PrintSequence::ByObject) {
        // Add each of the object's layers separately.
        for (auto object : print.objects()) {
            std::vector<coordf_t> zs;
            zs.reserve(object->layers().size() + object->support_layers().size());
            for (auto layer : object->layers())
                zs.push_back(layer->print_z);
            for (auto layer : object->support_layers())
                zs.push_back(layer->print_z);
            std::sort(zs.begin(), zs.end());
            //BBS: merge numerically very close Z values.
            auto end_it = std::unique(zs.begin(), zs.end());
            unsigned int temp_layer_count = (unsigned int)(end_it - zs.begin());
            for (auto it = zs.begin(); it != end_it - 1; it++) {
                if (abs(*it - *(it + 1)) < EPSILON)
                    temp_layer_count--;
            }
            m_layer_count += (unsigned int)(object->instances().size() * temp_layer_count);
        }
    } else {
        // Print all objects with the same print_z together.
        std::vector<coordf_t> zs;
        for (auto object : print.objects()) {
            zs.reserve(zs.size() + object->layers().size() + object->support_layers().size());
            for (auto layer : object->layers())
                zs.push_back(layer->print_z);
            for (auto layer : object->support_layers())
                zs.push_back(layer->print_z);
        }
        if (!zs.empty())
        {
            std::sort(zs.begin(), zs.end());
            //BBS: merge numerically very close Z values.
            auto end_it = std::unique(zs.begin(), zs.end());
            m_layer_count = (unsigned int)(end_it - zs.begin());
            for (auto it = zs.begin(); it != end_it - 1; it++) {
                if (abs(*it - *(it + 1)) < EPSILON)
                    m_layer_count--;
            }
        }
    }
    print.throw_if_canceled();

    m_enable_cooling_markers = true;
    this->apply_print_config(print.config());

    //limit speed and acc
    if (print.config().acceleration_limit_mess_enable|| print.config().speed_limit_to_height_enable)
        m_smoothSpeedAcc->init_limit(print.full_print_config());

    //limit temperature
    int extruders_index = print.extruders().size()>0 ? print.extruders()[0] : 0;
    if (print.config().material_flow_dependent_temperature.get_at(extruders_index) && !print.getMultiColor()){
        m_smoothTemp->init_limit(print.config().material_flow_temp_graph.value);
    }

    //m_volumetric_speed = DoExport::autospeed_volumetric_limit(print);
    print.throw_if_canceled();

    if (print.config().spiral_mode.value)
        m_spiral_vase = make_unique<SpiralVase>(print.config());

    if (print.config().max_volumetric_extrusion_rate_slope.value > 0){
    		m_pressure_equalizer = make_unique<PressureEqualizer>(print.config());
    		m_enable_extrusion_role_markers = (bool)m_pressure_equalizer;
    } else
	    m_enable_extrusion_role_markers = false;

    if (!print.config().small_area_infill_flow_compensation_model.empty())
        m_small_area_infill_flow_compensator = make_unique<SmallAreaInfillFlowCompensator>(print.config());

    if (!is_bbl_printers && print.config().gcode_flavor == gcfMarlinFirmware) {
        file.write_format(";%s\n", GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Time_Filament_Used).c_str());
        file.write_format(";Layer height:%.3f\n", print.config().initial_layer_print_height.value);
    }

    file.write_format("; HEADER_BLOCK_START\n");

     const ConfigOptionBool* is_cloud_slicer = print.full_print_config().option<ConfigOptionBool>("Is_Cloud_Slicer");
    if (is_cloud_slicer != nullptr) {
         if (is_cloud_slicer->value) {
            file.write_format("; generated by %s on %s\n", Slic3r::header_slic3r_generated_cloud().c_str(),
                              Slic3r::Utils::local_timestamp().c_str());

         } else {
             file.write_format("; generated by %s on %s\n", Slic3r::header_slic3r_generated2().c_str(),
                               Slic3r::Utils::local_timestamp().c_str());

         }
    } else {
        file.write_format("; generated by %s on %s\n", Slic3r::header_slic3r_generated2().c_str(), Slic3r::Utils::local_timestamp().c_str());
    }
    // Write information on the generator.

    if (is_bbl_printers)
        file.write_format(";%s\n", GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Estimated_Printing_Time_Placeholder).c_str());
    //BBS: total layer number
    file.write_format(";%s\n", GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Total_Layer_Number_Placeholder).c_str());
    m_enable_exclude_object = config().exclude_object;
    //Orca: extra check for bbl printer
    if (is_bbl_printers) {
        if (print.calib_params().mode == CalibMode::Calib_None) { // Don't support skipping in cali mode
            // list all label_object_id with sorted order here
            m_enable_exclude_object = true;
            m_label_objects_ids.clear();
            m_label_objects_ids.reserve(print.num_object_instances());
            for (const PrintObject *print_object : print.objects())
                for (const PrintInstance &print_instance : print_object->instances())
                    m_label_objects_ids.push_back(print_instance.model_instance->get_labeled_id());

            std::sort(m_label_objects_ids.begin(), m_label_objects_ids.end());

            std::string objects_id_list = "; model label id: ";
            for (auto it = m_label_objects_ids.begin(); it != m_label_objects_ids.end(); it++)
                objects_id_list += (std::to_string(*it) + (it != m_label_objects_ids.end() - 1 ? "," : "\n"));
            file.writeln(objects_id_list);
        } else {
            m_enable_exclude_object = false;
            m_label_objects_ids.clear();
        }
    }

    {
        std::string filament_density_list = "; filament_density: ";
        (filament_density_list+=m_config.filament_density.serialize()) +='\n';
        file.writeln(filament_density_list);

        std::string filament_diameter_list = "; filament_diameter: ";
        (filament_diameter_list += m_config.filament_diameter.serialize()) += '\n';
        file.writeln(filament_diameter_list);

        coordf_t max_height_z = -1;
        for (const auto& object : print.objects())
            max_height_z = std::max(object->layers().back()->print_z, max_height_z);

        std::ostringstream max_height_z_tip;
        max_height_z_tip<<"; max_z_height: " << std::fixed << std::setprecision(2) << max_height_z << '\n';
        file.writeln(max_height_z_tip.str());

        std::string creality_uuid_str = "; creality_uuid: ";
        boost::uuids::uuid uuid = boost::uuids::random_generator()();
        m_gcode_uuid = to_string(uuid);
        (creality_uuid_str += m_gcode_uuid) += '\n';
        file.writeln(creality_uuid_str);

        // Write creality_task_id_pending placeholder (_pending suffix marks unsent; replaced with field name creality_task_id on send)
        // Skip placeholder writing if report conditions are not met (model modified, non-3MF file, etc.)
        if (!print.get_write_task_id_placeholder()) {
            // Skip placeholder writing, gcode will not be reported
        } else {
            int plate_id = print.get_plate_index();
            if (plate_id >= 0) {
                // Placeholder: 56 zeros + _ + 13 zeros + _ + 2-digit plate = 73 bytes (8 less than final task_id)
                // The 8 saved bytes offset the _pending suffix in field name, total line 101 bytes unchanged
                char placeholder[74];
                memset(placeholder, '0', 73);
                placeholder[56] = '_';
                placeholder[70] = '_';
                int display_plate_id = plate_id + 1;
                placeholder[71] = '0' + (display_plate_id / 10);
                placeholder[72] = '0' + (display_plate_id % 10);
                placeholder[73] = '\0';
                file.write_format("; creality_task_id_pending: %s\n", placeholder);
            }
        }

        if (!print.get_creality_task_id().empty()) {
            std::string creality_task_id_str = "; creality_task_id: ";
            (creality_task_id_str += print.get_creality_task_id()) += '\n';
            file.writeln(creality_task_id_str);
        }
    }

    file.write_format("; HEADER_BLOCK_END\n\n");


   {
        BoundingBoxf3 _bbox;
        for (PrintObject* object : print.objects()) {
            for (PrintInstance& inst : object->instances()) {
                _bbox.merge(inst.get_bounding_box());
            }
        }
        // _bbox ???????

        Vec3d plate_origin = print.get_plate_origin();
        if (print.has_wipe_tower())
        {
            FakeWipeTower& towerdata = print.m_fake_wipe_tower;
            stl_vertex     WipeTowerMini(towerdata.pos.x(), towerdata.pos.y(), 0);
            stl_vertex     WipeTowerMax(towerdata.pos.x() + towerdata.width, towerdata.pos.y() + towerdata.depth, towerdata.height);
            _bbox.merge(WipeTowerMini.cast<double>());
            _bbox.merge(WipeTowerMax.cast<double>());
        }
        _bbox.translate(-plate_origin);

        BoundingBoxf bbox_bed(m_config.printable_area.values);
        float box_max_z = m_config.printable_height.value;

        float minx = (_bbox.min.x()>bbox_bed.min.x() && _bbox.min.x()<bbox_bed.max.x())?_bbox.min.x():bbox_bed.min.x();
        file.write_format("; MINX = %0.2f\n", minx);

        float miny = (_bbox.min.y()>bbox_bed.min.y() && _bbox.min.y()<bbox_bed.max.y())?_bbox.min.y():bbox_bed.min.y();
        file.write_format("; MINY = %0.2f\n", miny);

        float minz = (_bbox.min.z()>0 && _bbox.min.z()<box_max_z)?_bbox.min.z():0;
        file.write_format("; MINZ = %0.2f\n", minz);

        float maxx = (_bbox.max.x()>bbox_bed.min.x() && _bbox.max.x()<bbox_bed.max.x())?_bbox.max.x():bbox_bed.max.x();
        file.write_format("; MAXX = %0.2f\n", maxx);

        float maxy = (_bbox.max.y()>bbox_bed.min.y() && _bbox.max.x()<bbox_bed.max.y())?_bbox.max.y():bbox_bed.max.y();
        file.write_format("; MAXY = %0.2f\n", maxy);

        float maxz = (_bbox.max.z()>0 && _bbox.max.z()<box_max_z)?_bbox.max.z():box_max_z;
        file.write_format("; MAXZ = %0.2f\n\n", maxz);
    }

   //The firmware provides an interface to determine whether a new color change scheme is supported
    const ConfigOptionBool* is_multicolor_method = config().option<ConfigOptionBool>("multicolor_method");
    if (is_multicolor_method != nullptr) {
        if (is_multicolor_method->value) {
            file.write("; multicolor_method = 1 \n");
        } else {
            file.write("; multicolor_method = 0 \n");
        }

    }
   //

      // BBS: write global config at the beginning of gcode file because printer
      // need these config information
      // Append full config, delimited by two 'phony' configuration keys
      // CONFIG_BLOCK_START and CONFIG_BLOCK_END. The delimiters are structured
      // as configuration key / value pairs to be parsable by older versions of
      // PrusaSlicer G-code viewer.
    {
        if (is_bbl_printers) {
            file.write("; CONFIG_BLOCK_START\n");
            std::string full_config;
            append_full_config(print, full_config);
            if (!full_config.empty())
                file.write(full_config);

            // SoftFever: write compatiple image
            const std::vector<unsigned int> printing_extruders = print.extruders();
            const int first_printing_extruder_id = printing_extruders.empty() ? 0 : int(printing_extruders.front());
            int first_layer_bed_temperature = get_bed_temperature(first_printing_extruder_id, true, print.config().curr_bed_type, &printing_extruders);
            file.write_format("; first_layer_bed_temperature = %d\n",
                                first_layer_bed_temperature);
            file.write_format(
                "; first_layer_temperature = %d\n",
                print.config().nozzle_temperature_initial_layer.get_at(0));
            file.write("; CONFIG_BLOCK_END\n\n");
        } else if (thumbnail_cb != nullptr) {
            // generate the thumbnails
            auto [thumbnails, errors] = GCodeThumbnails::make_and_check_thumbnail_list(print.full_print_config());

            if (errors != enum_bitmask<ThumbnailError>()) {
                std::string error_str = format("Invalid thumbnails value:");
                error_str += GCodeThumbnails::get_error_string(errors);
                throw Slic3r::ExportError(error_str);
            }

            if (!thumbnails.empty())
                GCodeThumbnails::export_thumbnails_to_file(
                    thumbnail_cb, print.get_plate_index(), thumbnails,
                    [&file](const char* sz) { file.write(sz); },
                    [&print]() { print.throw_if_canceled(); }, m_layer_count);
        }
    }


    // Write some terse information on the slicing parameters.
    const PrintObject *first_object         = print.objects().front();
    const double       layer_height         = first_object->config().layer_height.value;
    const double       initial_layer_print_height   = print.config().initial_layer_print_height.value;
    for (size_t region_id = 0; region_id < print.num_print_regions(); ++ region_id) {
        const PrintRegion &region = print.get_print_region(region_id);
        file.write_format("; external perimeters extrusion width = %.2fmm\n", region.flow(*first_object, frExternalPerimeter, layer_height).width());
        file.write_format("; perimeters extrusion width = %.2fmm\n",          region.flow(*first_object, frPerimeter,         layer_height).width());
        file.write_format("; infill extrusion width = %.2fmm\n",              region.flow(*first_object, frInfill,            layer_height).width());
        file.write_format("; solid infill extrusion width = %.2fmm\n",        region.flow(*first_object, frSolidInfill,       layer_height).width());
        file.write_format("; top infill extrusion width = %.2fmm\n",          region.flow(*first_object, frTopSolidInfill,    layer_height).width());
        if (print.has_support_material())
            file.write_format("; support material extrusion width = %.2fmm\n", support_material_flow(first_object).width());
        if (print.config().initial_layer_line_width.value > 0)
            file.write_format("; first layer extrusion width = %.2fmm\n",   region.flow(*first_object, frPerimeter, initial_layer_print_height, true).width());
        file.write_format("\n");
    }

    file.write_format("; EXECUTABLE_BLOCK_START\n");

    // SoftFever
    if( m_enable_exclude_object)
        file.write(set_object_info(&print));

    // adds tags for time estimators
    file.write_format(";%s\n", GCodeProcessor::reserved_tag(GCodeProcessor::ETags::First_Line_M73_Placeholder).c_str());

    // Prepare the helper object for replacing placeholders in custom G-code and output filename.
    m_placeholder_parser_integration.parser = print.placeholder_parser();
    m_placeholder_parser_integration.parser.update_timestamp();
    m_placeholder_parser_integration.context.rng = std::mt19937(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    // Enable passing global variables between PlaceholderParser invocations.
    //m_placeholder_parser_integration.context.global_config = std::make_unique<DynamicConfig>();
    print.update_object_placeholders(m_placeholder_parser_integration.parser.config_writable(), ".gcode");

    // Creality: expose per-plate values as scalar placeholders for custom G-code.
    // This allows expressions like {wipe_tower_x + prime_tower_width*0.5} to work on the current plate.
    {
        auto rebind_plate_placeholders = [&print](PlaceholderParser& parser) {
            const int         plate_idx = print.get_plate_index();
            const PrintConfig& cfg       = print.config();

            parser.set("wipe_tower_x", cfg.wipe_tower_x.get_at(plate_idx));
            parser.set("wipe_tower_y", cfg.wipe_tower_y.get_at(plate_idx));
        };
        rebind_plate_placeholders(m_placeholder_parser_integration.parser);
    }

    // Get optimal tool ordering to minimize tool switches of a multi-exruder print.
    // For a print by objects, find the 1st printing object.
    ToolOrdering tool_ordering;
    unsigned int initial_extruder_id = (unsigned int)-1;
    //BBS: first non-support filament extruder
    unsigned int initial_non_support_extruder_id;
    unsigned int final_extruder_id   = (unsigned int)-1;
    bool         has_wipe_tower      = false;
    const bool   enable_all_extruder_priming = false;
    std::vector<const PrintInstance*> 					print_object_instances_ordering;
    m_remaining_extruder_segment_uses.clear();
    std::vector<const PrintInstance*>::const_iterator 	print_object_instance_sequential_active;
    if (print.config().print_sequence == PrintSequence::ByObject && print.objects().size() > 1) {
        // Order object instances for sequential print.
        print_object_instances_ordering = sort_object_instances_by_model_order(print);
//        print_object_instances_ordering = sort_object_instances_by_max_z(print);
        // Find the 1st printing object, find its tool ordering and the initial extruder ID.
        print_object_instance_sequential_active = print_object_instances_ordering.begin();
        for (; print_object_instance_sequential_active != print_object_instances_ordering.end(); ++ print_object_instance_sequential_active) {
            tool_ordering = ToolOrdering(*(*print_object_instance_sequential_active)->print_object, initial_extruder_id);
            if ((initial_extruder_id = tool_ordering.first_extruder()) != static_cast<unsigned int>(-1)) {
                //BBS: try to find the non-support filament extruder if is multi color and initial_extruder is support filament
                initial_non_support_extruder_id = initial_extruder_id;
                if (tool_ordering.all_extruders().size() > 1 && print.config().filament_is_support.get_at(initial_extruder_id)) {
                    bool has_non_support_filament = false;
                    for (unsigned int extruder : tool_ordering.all_extruders()) {
                        if (!print.config().filament_is_support.get_at(extruder)) {
                            has_non_support_filament = true;
                            break;
                        }
                    }
                    //BBS: find the non-support filament extruder of object
                    if (has_non_support_filament)
                        for (LayerTools layer_tools : tool_ordering.layer_tools()) {
                            if (!layer_tools.has_object)
                                continue;
                            for (unsigned int extruder : layer_tools.extruders) {
                                if (print.config().filament_is_support.get_at(extruder))
                                    continue;
                                initial_non_support_extruder_id = extruder;
                                break;
                            }
                        }
                }

                break;
            }
        }
        if (initial_extruder_id == static_cast<unsigned int>(-1))
            // No object to print was found, cancel the G-code export.
            throw Slic3r::SlicingError(_(L("No object can be printed. Maybe too small")));
        // We don't allow switching of extruders per layer by Model::custom_gcode_per_print_z in sequential mode.
        // Use the extruder IDs collected from Regions.
        this->set_extruders(print.extruders());

        has_wipe_tower = print.has_wipe_tower() && tool_ordering.has_wipe_tower();
    } else {
        // Find tool ordering for all the objects at once, and the initial extruder ID.
        // If the tool ordering has been pre-calculated by Print class for wipe tower already, reuse it.
        tool_ordering = print.tool_ordering();
        tool_ordering.assign_custom_gcodes(print);
        if (tool_ordering.all_extruders().empty())
            // No object to print was found, cancel the G-code export.
            throw Slic3r::SlicingError(_(L("No object can be printed. Maybe too small")));
        has_wipe_tower = print.has_wipe_tower() && tool_ordering.has_wipe_tower();

        // Orca: support all extruder priming
        initial_extruder_id = (!is_bbl_printers && has_wipe_tower && !enable_all_extruder_priming) ?
            // The priming towers will be skipped.
            tool_ordering.all_extruders().back() :
            // Don't skip the priming towers.
            tool_ordering.first_extruder();

        //BBS: try to find the non-support filament extruder if is multi color and initial_extruder is support filament
        if (initial_extruder_id != static_cast<unsigned int>(-1)) {
            initial_non_support_extruder_id = initial_extruder_id;
            if (tool_ordering.all_extruders().size() > 1 && print.config().filament_is_support.get_at(initial_extruder_id)) {
                bool has_non_support_filament = false;
                for (unsigned int extruder : tool_ordering.all_extruders()) {
                    if (!print.config().filament_is_support.get_at(extruder)) {
                        has_non_support_filament = true;
                        break;
                    }
                }
                //BBS: find the non-support filament extruder of object
                if (has_non_support_filament)
                    for (LayerTools layer_tools : tool_ordering.layer_tools()) {
                        if (!layer_tools.has_object)
                            continue;
                        for (unsigned int extruder : layer_tools.extruders) {
                            if (print.config().filament_is_support.get_at(extruder))
                                continue;
                            initial_non_support_extruder_id = extruder;
                            break;
                        }
                    }
            }
        }

        // In non-sequential print, the printing extruders may have been modified by the extruder switches stored in Model::custom_gcode_per_print_z.
        // Therefore initialize the printing extruders from there.
        this->set_extruders(tool_ordering.all_extruders());
        print_object_instances_ordering =
            // By default, order object instances using a nearest neighbor search.
            print.config().print_order == PrintOrder::Default ? chain_print_object_instances(print)
            // Otherwise same order as the object list
            : sort_object_instances_by_model_order(print);
    }
    if (initial_extruder_id == (unsigned int)-1) {
        // Nothing to print!
        initial_extruder_id = 0;
        initial_non_support_extruder_id = 0;
        final_extruder_id   = 0;
    } else {
        final_extruder_id = tool_ordering.last_extruder();
        assert(final_extruder_id != (unsigned int)-1);
    }
    print.throw_if_canceled();

    m_gcode_editor = make_unique<GCodeEditor>(*this);
    m_gcode_editor->set_current_extruder(initial_extruder_id);

    // Emit machine envelope limits for the Marlin firmware.
    this->print_machine_envelope(file, print);

    // Disable fan.
    if (m_config.auxiliary_fan.value && print.config().close_fan_the_first_x_layers.get_at(initial_extruder_id)) {
        file.write(m_writer.set_fan(0));
        //BBS: disable additional fan
        file.write(m_writer.set_additional_fan(0));
    }

    // Update output variables after the extruders were initialized.
    m_placeholder_parser_integration.init(m_writer);
    // Let the start-up script prime the 1st printing tool.
    this->placeholder_parser().set("initial_tool", initial_extruder_id);
    this->placeholder_parser().set("initial_extruder", initial_extruder_id);
    //BBS
    this->placeholder_parser().set("initial_no_support_tool", initial_non_support_extruder_id);
    this->placeholder_parser().set("initial_no_support_extruder", initial_non_support_extruder_id);
    this->placeholder_parser().set("current_extruder", initial_extruder_id);
    //set the key for compatibilty
    this->placeholder_parser().set("retraction_distance_when_cut", m_config.retraction_distances_when_cut.get_at(initial_extruder_id));
    this->placeholder_parser().set("long_retraction_when_cut", m_config.long_retractions_when_cut.get_at(initial_extruder_id));

    this->placeholder_parser().set("retraction_distances_when_cut", new ConfigOptionFloats(m_config.retraction_distances_when_cut));
    this->placeholder_parser().set("long_retractions_when_cut",new ConfigOptionBools(m_config.long_retractions_when_cut));
    //Set variable for total layer count so it can be used in custom gcode.
    this->placeholder_parser().set("total_layer_count", m_layer_count);
    // Useful for sequential prints.
    this->placeholder_parser().set("current_object_idx", 0);
    // For the start / end G-code to do the priming and final filament pull in case there is no wipe tower provided.
    this->placeholder_parser().set("has_wipe_tower", has_wipe_tower);
    this->placeholder_parser().set("has_single_extruder_multi_material_priming", !is_bbl_printers && has_wipe_tower && enable_all_extruder_priming);
    this->placeholder_parser().set("total_toolchanges", std::max(0, print.wipe_tower_data().number_of_toolchanges)); // Check for negative toolchanges (single extruder mode) and set to 0 (no tool change).
    this->placeholder_parser().set("num_extruders", int(print.config().nozzle_diameter.values.size()));
    this->placeholder_parser().set("retract_length", new ConfigOptionFloats(print.config().retraction_length));

    // PlaceholderParser currently substitues non-existent vector values with the zero'th value, which is harmful in the
    // case of "is_extruder_used[]" as Slicer may lie about availability of such non-existent extruder. We rather
    // sacrifice 256B of memory before we change the behavior of the PlaceholderParser, which should really only fill in
    // the non-existent vector elements for filament parameters.
    std::vector<unsigned char> is_extruder_used(std::max(size_t(255), print.config().filament_diameter.size()), 0);
    for (unsigned int extruder : tool_ordering.all_extruders())
        is_extruder_used[extruder] = true;
    this->placeholder_parser().set("is_extruder_used", new ConfigOptionBools(is_extruder_used));

    {
        BoundingBoxf bbox_bed(print.config().printable_area.values);
        Vec2f plate_offset = m_writer.get_xy_offset();
        this->placeholder_parser().set("print_bed_min", new ConfigOptionFloats({ bbox_bed.min.x(), bbox_bed.min.y()}));
        this->placeholder_parser().set("print_bed_max", new ConfigOptionFloats({ bbox_bed.max.x(), bbox_bed.max.y()}));
        this->placeholder_parser().set("print_bed_size", new ConfigOptionFloats({ bbox_bed.size().x(), bbox_bed.size().y() }));

        BoundingBoxf bbox;
        auto pts = std::make_unique<ConfigOptionPoints>();
        if (print.calib_mode() == CalibMode::Calib_PA_Line || print.calib_mode() == CalibMode::Calib_PA_Pattern) {
            bbox = bbox_bed;
            bbox.offset(-25.0);
            // add 4 corner points of bbox into pts
            pts->values.reserve(4);
            pts->values.emplace_back(bbox.min.x(), bbox.min.y());
            pts->values.emplace_back(bbox.max.x(), bbox.min.y());
            pts->values.emplace_back(bbox.max.x(), bbox.max.y());
            pts->values.emplace_back(bbox.min.x(), bbox.max.y());

        } else {
            // Convex hull of the 1st layer extrusions, for bed leveling and placing the initial purge line.
            // It encompasses the object extrusions, support extrusions, skirt, brim, wipe tower.
            // It does NOT encompass user extrusions generated by custom G-code,
            // therefore it does NOT encompass the initial purge line.
            // It does NOT encompass MMU/MMU2 starting (wipe) areas.
            pts->values.reserve(print.first_layer_convex_hull().size());
            for (const Point &pt : print.first_layer_convex_hull().points)
                pts->values.emplace_back(print.translate_to_print_space(pt));
            bbox = BoundingBoxf((pts->values));
        }
        this->placeholder_parser().set("first_layer_print_convex_hull", pts.release());
        this->placeholder_parser().set("first_layer_print_min", new ConfigOptionFloats({bbox.min.x(), bbox.min.y()}));
        this->placeholder_parser().set("first_layer_print_max", new ConfigOptionFloats({bbox.max.x(), bbox.max.y()}));
        this->placeholder_parser().set("first_layer_print_size", new ConfigOptionFloats({ bbox.size().x(), bbox.size().y() }));

        {
            // use first layer convex_hull union with each object's bbox to check whether in head detect zone
            Polygons object_projections;
            for (auto& obj : print.objects()) {
                for (auto& instance : obj->instances()) {
                    const auto& bbox = instance.get_bounding_box();
                    Point min_p{ coord_t(scale_(bbox.min.x())),coord_t(scale_(bbox.min.y())) };
                    Point max_p{ coord_t(scale_(bbox.max.x())),coord_t(scale_(bbox.max.y())) };
                    Polygon instance_projection = {
                        {min_p.x(),min_p.y()},
                        {max_p.x(),min_p.y()},
                        {max_p.x(),max_p.y()},
                        {min_p.x(),max_p.y()}
                    };
                    object_projections.emplace_back(std::move(instance_projection));
                }
            }
            object_projections.emplace_back(print.first_layer_convex_hull());

            Polygons project_polys = union_(object_projections);
            Polygon  head_wrap_detect_zone;
            for (auto& point : print.config().head_wrap_detect_zone.values)
                head_wrap_detect_zone.append(scale_(point).cast<coord_t>() + scale_(plate_offset).cast<coord_t>());

            this->placeholder_parser().set("in_head_wrap_detect_zone", !intersection_pl(project_polys, {head_wrap_detect_zone}).empty());
        }

        BoundingBoxf mesh_bbox(m_config.bed_mesh_min, m_config.bed_mesh_max);
        auto         mesh_margin = m_config.adaptive_bed_mesh_margin.value;
        mesh_bbox.min            = mesh_bbox.min.cwiseMax((bbox.min.array() - mesh_margin).matrix());
        mesh_bbox.max            = mesh_bbox.max.cwiseMin((bbox.max.array() + mesh_margin).matrix());
        this->placeholder_parser().set("adaptive_bed_mesh_min", new ConfigOptionFloats({mesh_bbox.min.x(), mesh_bbox.min.y()}));
        this->placeholder_parser().set("adaptive_bed_mesh_max", new ConfigOptionFloats({mesh_bbox.max.x(), mesh_bbox.max.y()}));

        auto probe_dist_x  = std::max(1., m_config.bed_mesh_probe_distance.value.x());
        auto probe_dist_y  = std::max(1., m_config.bed_mesh_probe_distance.value.y());
        int  probe_count_x = std::max(3, (int) std::ceil(mesh_bbox.size().x() / probe_dist_x));
        int  probe_count_y = std::max(3, (int) std::ceil(mesh_bbox.size().y() / probe_dist_y));
        auto bed_mesh_algo = "bicubic";
        if (probe_count_x * probe_count_y <= 6) { // lagrange needs up to a total of 6 mesh points
            bed_mesh_algo = "lagrange";
        }
        else
            if(print.config().gcode_flavor == gcfKlipper){
              // bicubic needs 4 probe points per axis
              probe_count_x = std::max(probe_count_x,4);
              probe_count_y = std::max(probe_count_y,4);
            }
        this->placeholder_parser().set("bed_mesh_probe_count", new ConfigOptionInts({probe_count_x, probe_count_y}));
        this->placeholder_parser().set("bed_mesh_algo", bed_mesh_algo);
        // get center without wipe tower
        BoundingBoxf bbox_wo_wt; // bounding box without wipe tower
        for (auto &objPtr : print.objects()) {
            BBoxData data;
            bbox_wo_wt.merge(unscaled(objPtr->get_first_layer_bbox(data.area, data.layer_height, data.name)));
        }
        auto center = bbox_wo_wt.center();
        this->placeholder_parser().set("first_layer_center_no_wipe_tower", new ConfigOptionFloats{ {center.x(),center.y()}});
    }
    //bool activate_chamber_temp_control = false;
    //auto max_chamber_temp              = 0;
    //for (const auto &extruder : m_writer.extruders()) {
    //    activate_chamber_temp_control |= m_config.activate_chamber_temp_control.get_at(extruder.id());
    //    max_chamber_temp = std::max(max_chamber_temp, m_config.chamber_temperature.get_at(extruder.id()));
    //}
    bool activate_chamber_temp_control = false;
    int max_chamber_temp = 0;
    int max_extruder_id  = -1;
    for (const auto& extruder : m_writer.extruders()) {
        activate_chamber_temp_control |= m_config.activate_chamber_temp_control.get_at(extruder.id());
        int current_value = m_config.chamber_temperature.get_at(extruder.id());
        // ?????????????,?????????ID
        if (current_value > max_chamber_temp) {
            max_chamber_temp = current_value;
            max_extruder_id  = extruder.id();
        }
    }

    int   max_bed_temp   = 0;
    int   max_nozzle_temperature     = 0;
    int                     curr_bed_type          = m_config.option("curr_bed_type")->getInt();
    const ConfigOptionInts* bed_temp_1st_layer_opt = m_config.option<ConfigOptionInts>(get_bed_temp_1st_layer_key((BedType) curr_bed_type));
    const ConfigOptionInts* bed_temp_layer_opt = m_config.option<ConfigOptionInts>(get_bed_temp_key((BedType) curr_bed_type));
    for (auto extruder : m_writer.extruders()) {
        int temperature = m_config.nozzle_temperature.get_at(extruder.id() - 1);
        int temperature_first = m_config.nozzle_temperature_initial_layer.get_at(extruder.id() - 1);
        int bedTemp_first     = bed_temp_1st_layer_opt->get_at(extruder.id() - 1);
        int bedTemp           = bed_temp_layer_opt->get_at(extruder.id() - 1);
        if (bedTemp > max_bed_temp) {
            max_bed_temp = bedTemp;
        }
        if (bedTemp_first > max_bed_temp) {
            max_bed_temp = bedTemp_first;
        }
        if (temperature > max_nozzle_temperature) {
            max_nozzle_temperature = temperature;
        }
        if (temperature_first > max_nozzle_temperature) {
            max_nozzle_temperature = temperature_first;
        }
    }
    file.write_format("; max_print_temp = %d,%d\n", max_bed_temp, max_nozzle_temperature);

    float outer_wall_volumetric_speed = 0.0f;
    {
        int curr_bed_type = m_config.curr_bed_type.getInt();

        std::string first_layer_bed_temp_str;
        const ConfigOptionInts* first_bed_temp_opt = m_config.option<ConfigOptionInts>(get_bed_temp_1st_layer_key((BedType)curr_bed_type));
        const ConfigOptionInts* bed_temp_opt = m_config.option<ConfigOptionInts>(get_bed_temp_key((BedType)curr_bed_type));
        this->placeholder_parser().set("bbl_bed_temperature_gcode", new ConfigOptionBool(false));
        this->placeholder_parser().set("bed_temperature_initial_layer", new ConfigOptionInts(*first_bed_temp_opt));
        this->placeholder_parser().set("bed_temperature", new ConfigOptionInts(*bed_temp_opt));
        this->placeholder_parser().set("bed_temperature_initial_layer_single", new ConfigOptionInt(get_bed_temperature(initial_extruder_id, true, (BedType) curr_bed_type)));
        this->placeholder_parser().set("bed_temperature_initial_layer_vector", new ConfigOptionString(""));
        this->placeholder_parser().set("chamber_temperature",new ConfigOptionInts(m_config.chamber_temperature));
        this->placeholder_parser().set("overall_chamber_temperature", new ConfigOptionInt(max_chamber_temp));

        // SoftFever: support variables `first_layer_temperature` and `first_layer_bed_temperature`
        this->placeholder_parser().set("first_layer_bed_temperature", new ConfigOptionInts(*first_bed_temp_opt));
        this->placeholder_parser().set("first_layer_temperature", new ConfigOptionInts(m_config.nozzle_temperature_initial_layer));
        this->placeholder_parser().set("max_print_height",new ConfigOptionInt(m_config.printable_height));
        this->placeholder_parser().set("z_offset", new ConfigOptionFloat(m_config.z_offset));
        this->placeholder_parser().set("model_name", new ConfigOptionString(print.get_model_name()));
        this->placeholder_parser().set("plate_number", new ConfigOptionString(print.get_plate_number_formatted()));
        this->placeholder_parser().set("plate_name", new ConfigOptionString(print.get_plate_name()));
        this->placeholder_parser().set("first_layer_height", new ConfigOptionFloat(m_config.initial_layer_print_height.value));

        //add during_print_exhaust_fan_speed
        std::vector<int> during_print_exhaust_fan_speed_num;
        during_print_exhaust_fan_speed_num.reserve(m_config.during_print_exhaust_fan_speed.size());
        for (const auto& item : m_config.during_print_exhaust_fan_speed.values)
            during_print_exhaust_fan_speed_num.emplace_back((int)(item / 100.0 * 255));
        this->placeholder_parser().set("during_print_exhaust_fan_speed_num",new ConfigOptionInts(during_print_exhaust_fan_speed_num));

        // calculate the volumetric speed of outer wall. Ignore per-object setting and multi-filament, and just use the default setting
        {

            float filament_max_volumetric_speed = m_config.option<ConfigOptionFloats>("filament_max_volumetric_speed")->get_at(initial_non_support_extruder_id);
            const double nozzle_diameter = m_config.nozzle_diameter.get_at(initial_non_support_extruder_id);
            float outer_wall_line_width = print.default_region_config().get_abs_value("outer_wall_line_width", nozzle_diameter);
            if (outer_wall_line_width == 0.0) {
                float default_line_width =  print.default_object_config().get_abs_value("line_width", nozzle_diameter);
                outer_wall_line_width = default_line_width == 0.0 ? nozzle_diameter : default_line_width;
            }
            Flow outer_wall_flow = Flow(outer_wall_line_width, m_config.layer_height, m_config.nozzle_diameter.get_at(initial_non_support_extruder_id));
            float outer_wall_speed = print.default_region_config().outer_wall_speed.value;
            outer_wall_volumetric_speed = outer_wall_speed * outer_wall_flow.mm3_per_mm();
            if (outer_wall_volumetric_speed > filament_max_volumetric_speed)
                outer_wall_volumetric_speed = filament_max_volumetric_speed;
            this->placeholder_parser().set("outer_wall_volumetric_speed", new ConfigOptionFloat(outer_wall_volumetric_speed));
        }

        if (print.calib_params().mode == CalibMode::Calib_PA_Line) {
            this->placeholder_parser().set("scan_first_layer", new ConfigOptionBool(false));
        }
    }
    std::string machine_start_template = print.config().machine_start_gcode.value;
    if (print.calib_mode() == CalibMode::Calib_Vol_speed_Tower) {
        // ???????,??????,??????200??????
        boost::replace_all(machine_start_template, "{filament_max_volumetric_speed[initial_extruder]/0.360}", "6000");
        boost::replace_all(machine_start_template, "{filament_max_volumetric_speed[initial_extruder]/0.3*60}", "6000");
    }
    std::string machine_start_gcode = this->placeholder_parser_process("machine_start_gcode", machine_start_template, initial_extruder_id);
    if (print.config().gcode_flavor != gcfKlipper || (m_writer.multiple_extruders && !print.config().single_extruder_multi_material)) {
        // Set bed temperature if the start G-code does not contain any bed temp control G-codes.
        this->_print_first_layer_bed_temperature(file, print, machine_start_gcode, initial_extruder_id, false);
        // Set extruder(s) temperature before and after start G-code.
        this->_print_first_layer_extruder_temperatures(file, print, machine_start_gcode, initial_extruder_id, true);
    }

    // adds tag for processor
    file.write_format(";%s%s\n", GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Role).c_str(), ExtrusionEntity::role_to_string(erCustom).c_str());

    // Orca: set chamber temperature at the beginning of gcode file
    int max_activate_chamber_layer = m_config.activate_chamber_layer.get_at(max_extruder_id);

    if (activate_chamber_temp_control && max_chamber_temp > 0 && max_activate_chamber_layer == 1)
    {
        if (print.is_CX_printer())
            file.write(m_writer.set_chamber_temperature(max_chamber_temp, false)); // for creality
        else
            file.write(m_writer.set_chamber_temperature(max_chamber_temp, true)); // set chamber_temperature
    }
    // Write the custom start G-code
    file.writeln(machine_start_gcode);

    //BBS: gcode writer doesn't know where the real position of extruder is after inserting custom gcode
    m_writer.set_current_position_clear(false);
    m_start_gcode_filament = GCodeProcessor::get_gcode_last_filament(machine_start_gcode);

    //flush FanMover buffer to avoid modifying the start gcode if it's manual.
    if (!machine_start_gcode.empty() && this->m_fan_mover.get() != nullptr)
        file.write(this->m_fan_mover.get()->process_gcode("", true));

    // Process filament-specific gcode.
   /* if (has_wipe_tower) {
        // Wipe tower will control the extruder switching, it will call the filament_start_gcode.
    } else {
            DynamicConfig config;
            config.set_key_value("filament_extruder_id", new ConfigOptionInt(int(initial_extruder_id)));
            file.writeln(this->placeholder_parser_process("filament_start_gcode", print.config().filament_start_gcode.values[initial_extruder_id], initial_extruder_id, &config));
    }
*/
    if (is_bbl_printers) {
        this->_print_first_layer_extruder_temperatures(file, print, machine_start_gcode, initial_extruder_id, true);
    }
    // Orca: when activate_air_filtration is set on any extruder, find and set the highest during_print_exhaust_fan_speed
    bool activate_air_filtration        = false;
    int  during_print_exhaust_fan_speed = 0;
    for (const auto &extruder : m_writer.extruders()) {
        activate_air_filtration |= m_config.activate_air_filtration.get_at(extruder.id());
        if (m_config.activate_air_filtration.get_at(extruder.id()))
            during_print_exhaust_fan_speed = std::max(during_print_exhaust_fan_speed,
                                                      m_config.during_print_exhaust_fan_speed.get_at(extruder.id()));
    }
    if (activate_air_filtration)
        file.write(m_writer.set_exhaust_fan(during_print_exhaust_fan_speed, true, print.is_CX_printer()));

    print.throw_if_canceled();

    // Set other general things.
    file.write(this->preamble());

    // Calculate wiping points if needed
    DoExport::init_ooze_prevention(print, m_ooze_prevention);
    print.throw_if_canceled();

    // Collect custom seam data from all objects.
    std::function<void(void)> throw_if_canceled_func = [&print]() { print.throw_if_canceled(); };
    m_seam_placer.init(print, throw_if_canceled_func);

    // BBS: get path for change filament
    if (m_writer.multiple_extruders) {
        std::vector<Vec2d> points = get_path_of_change_filament(print);
        if (points.size() == 3) {
            travel_point_1 = points[0];
            travel_point_2 = points[1];
            travel_point_3 = points[2];
        }
    }

    const bool start_gcode_selected_initial_extruder =
        !is_bbl_printers && m_start_gcode_filament == static_cast<int>(initial_extruder_id);
     // Orca: support extruder priming
    if (is_bbl_printers || !(has_wipe_tower && enable_all_extruder_priming))
    {
        // Set initial extruder only after custom start G-code.
        // Ugly hack: Do not set the initial extruder if the extruder is primed using the MMU priming towers at the edge of the print bed.
        file.write(this->set_extruder(initial_extruder_id, 0., false, !start_gcode_selected_initial_extruder));
    }
    // BBS: set that indicates objs with brim
    for (auto iter = print.m_brimMap.begin(); iter != print.m_brimMap.end(); ++iter) {
        if (!iter->second.empty())
            this->m_objsWithBrim.insert(iter->first);
    }
    for (auto iter = print.m_supportBrimMap.begin(); iter != print.m_supportBrimMap.end(); ++iter) {
        if (!iter->second.empty())
            this->m_objSupportsWithBrim.insert(iter->first);
    }
    if (this->m_objsWithBrim.empty() && this->m_objSupportsWithBrim.empty()) m_brim_done = true;

    if (m_config.travel_acceleration.value > 0) {
        m_writer.set_travel_acceleration((unsigned int) floor(m_config.travel_acceleration.value + 0.5));
    }
    std::vector<unsigned int> first_layer_travel_accelerations;
    for (size_t i = 0; i < m_config.initial_layer_travel_acceleration.values.size(); i++) {
        if (!m_config.initial_layer_travel_acceleration.is_nil(i)) {
            double value = m_config.initial_layer_travel_acceleration.values[i];
            first_layer_travel_accelerations.emplace_back((unsigned int) floor(value + 0.5));
        } else {
            first_layer_travel_accelerations.emplace_back(0); // nil ?? 0
        }
    }
    m_writer.set_first_layer_travel_acceleration(first_layer_travel_accelerations);

    if(!is_bbl_printers)
    {
        file.write(this->retract(false, false));
    }

    // SoftFever: calib
    if (print.calib_params().mode == CalibMode::Calib_PA_Line) {
        std::string gcode;
        if ((print.default_object_config().outer_wall_acceleration.value > 0 && print.default_object_config().outer_wall_acceleration.value > 0)) {
            gcode += m_writer.set_print_acceleration((unsigned int)floor(print.default_object_config().outer_wall_acceleration.value + 0.5));
        }

        if (print.default_object_config().outer_wall_jerk.value > 0) {
            double jerk = print.default_object_config().outer_wall_jerk.value;
            gcode += m_writer.set_jerk_xy(jerk);
        }

        auto params = print.calib_params();

        CalibPressureAdvanceLine pa_test(this);

        auto fast_speed = CalibPressureAdvance::find_optimal_PA_speed(print.full_print_config(), pa_test.line_width(), pa_test.height_layer());
        auto slow_speed = std::max(10.0, fast_speed / 10.0);
        if (fast_speed < slow_speed + 5)
            fast_speed = slow_speed + 5;

        pa_test.set_speed(fast_speed, slow_speed);
        pa_test.draw_numbers() = print.calib_params().print_numbers;
        gcode += pa_test.generate_test(params.start, params.step, std::llround(std::ceil((params.end - params.start) / params.step)) + 1);

        file.write(gcode);
    } else {
        //BBS: open spaghetti detector
        if (is_bbl_printers) {
            // if (print.config().spaghetti_detector.value)
            file.write("M981 S1 P20000 ;open spaghetti detector\n");
        }

        // Do all objects for each layer.
        if (print.config().print_sequence == PrintSequence::ByObject && !has_wipe_tower && print.objects().size() > 1) {
            size_t finished_objects = 0;
            const PrintObject *prev_object = (*print_object_instance_sequential_active)->print_object;
            m_remaining_extruder_segment_uses.assign(print.config().nozzle_diameter.values.size(), 0);
            ToolOrdering       simulated_tool_ordering  = tool_ordering;
            unsigned int       simulated_final_extruder = final_extruder_id;
            const PrintObject *simulated_prev_object    = (*print_object_instance_sequential_active)->print_object;
            for (auto it = print_object_instance_sequential_active; it != print_object_instances_ordering.end(); ++it) {
                const PrintObject &simulated_object = *(*it)->print_object;
                if (&simulated_object != simulated_prev_object || simulated_tool_ordering.first_extruder() != simulated_final_extruder) {
                    simulated_tool_ordering = ToolOrdering(simulated_object, simulated_final_extruder);
                    unsigned int new_extruder_id = simulated_tool_ordering.first_extruder();
                    if (new_extruder_id == static_cast<unsigned int>(-1))
                        continue;
                    simulated_final_extruder = simulated_tool_ordering.last_extruder();
                }
                for (const LayerToPrint &layer_to_print : collect_layers_to_print(simulated_object)) {
                    const LayerTools &object_layer_tools = simulated_tool_ordering.tools_for_layer(layer_to_print.print_z());
                    for (unsigned int layer_extruder_id : object_layer_tools.extruders)
                        if (layer_extruder_id < m_remaining_extruder_segment_uses.size())
                            ++m_remaining_extruder_segment_uses[layer_extruder_id];
                }
                simulated_prev_object = &simulated_object;
            }
            for (; print_object_instance_sequential_active != print_object_instances_ordering.end(); ++ print_object_instance_sequential_active) {
                const PrintObject &object = *(*print_object_instance_sequential_active)->print_object;
                if (&object != prev_object || tool_ordering.first_extruder() != final_extruder_id) {
                    tool_ordering = ToolOrdering(object, final_extruder_id);
                    unsigned int new_extruder_id = tool_ordering.first_extruder();
                    if (new_extruder_id == (unsigned int)-1)
                        // Skip this object.
                        continue;
                    initial_extruder_id = new_extruder_id;
                    final_extruder_id   = tool_ordering.last_extruder();
                    assert(final_extruder_id != (unsigned int)-1);
                }
                print.throw_if_canceled();
                this->set_origin(unscale((*print_object_instance_sequential_active)->shift));

                // BBS: prime extruder if extruder change happens before this object instance
                bool prime_extruder = false;
                if (finished_objects > 0) {
                    // Move to the origin position for the copy we're going to print.
                    // This happens before Z goes down to layer 0 again, so that no collision happens hopefully.
                    m_enable_cooling_markers = false; // we're not filtering these moves through GCodeEditor
                    m_avoid_crossing_perimeters.use_external_mp_once();
                    // BBS. change tool before moving to origin point.
                    if (m_writer.need_toolchange(initial_extruder_id)) {
                        coordf_t initial_layer_print_height = print.config().initial_layer_print_height.value;
#ifdef SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
                        this->set_toolchange_source_object(m_last_obj_copy.first);
                        this->set_toolchange_target_object(&object);
#endif // SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
                        file.write(this->set_extruder(initial_extruder_id, initial_layer_print_height, true));
#ifdef SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
                        this->set_toolchange_source_object(nullptr);
                        this->set_toolchange_target_object(nullptr);
#endif // SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
                        prime_extruder = true;
                    }
                    else {
                        file.write(this->retract());
                    }
                    file.write(m_writer.travel_to_z(m_max_layer_z));
                    file.write(this->travel_to(Point(0, 0), erNone, "move to origin position for next object"));
                    m_enable_cooling_markers = true;
                    // Disable motion planner when traveling to first object point.
                    m_avoid_crossing_perimeters.disable_once();
                    // Ff we are printing the bottom layer of an object, and we have already finished
                    // another one, set first layer temperatures. This happens before the Z move
                    // is triggered, so machine has more time to reach such temperatures.
                    this->placeholder_parser().set("current_object_idx", int(finished_objects));
                    std::string printing_by_object_gcode = this->placeholder_parser_process("printing_by_object_gcode", print.config().printing_by_object_gcode.value, initial_extruder_id);
                    // Set first layer bed and extruder temperatures, don't wait for it to reach the temperature.
                    this->_print_first_layer_bed_temperature(file, print, printing_by_object_gcode, initial_extruder_id, false);
                    this->_print_first_layer_extruder_temperatures(file, print, printing_by_object_gcode, initial_extruder_id, false);
                    file.writeln(printing_by_object_gcode);
                }
                // Reset the cooling buffer internal state (the current position, feed rate, accelerations).
                m_gcode_editor->reset(this->writer().get_position());
                m_gcode_editor->set_current_extruder(initial_extruder_id);
                // Process all layers of a single object instance (sequential mode) with a parallel pipeline:
                // Generate G-code, run the filters (vase mode, cooling buffer), run the G-code analyser
                // and export G-code into file.
                this->process_layers(print, tool_ordering, collect_layers_to_print(object), *print_object_instance_sequential_active - object.instances().data(), file, prime_extruder);
                //BBS: close powerlost recovery
                {
                    if (is_bbl_printers && m_second_layer_things_done) {
                        file.write("; close powerlost recovery\n");
                        file.write("M1003 S0\n");
                    }
                }
                ++ finished_objects;
                // Flag indicating whether the nozzle temperature changes from 1st to 2nd layer were performed.
                // Reset it when starting another object from 1st layer.
                m_second_layer_things_done = false;
                prev_object = &object;
            }
        } else {
            // Sort layers by Z.
            // All extrusion moves with the same top layer height are extruded uninterrupted.
            std::vector<std::pair<coordf_t, std::vector<LayerToPrint>>> layers_to_print = collect_layers_to_print(print);
            m_remaining_extruder_segment_uses.assign(print.config().nozzle_diameter.values.size(), 0);
            for (const auto &layer_to_print : layers_to_print) {
                const LayerTools &current_layer_tools = tool_ordering.tools_for_layer(layer_to_print.first);
                for (unsigned int layer_extruder_id : current_layer_tools.extruders)
                    if (layer_extruder_id < m_remaining_extruder_segment_uses.size())
                        ++m_remaining_extruder_segment_uses[layer_extruder_id];
            }
            if (!layers_to_print.empty()) {
                // Start-gcode may leave Z lifted; prime lines must run at first-layer height.
                file.write(m_writer.travel_to_z(initial_layer_print_height + m_config.z_offset.value, "Move to the first layer height"));
            }
            // Prusa Multi-Material wipe tower.
            if (has_wipe_tower && ! layers_to_print.empty()) {
                m_wipe_tower.reset(new WipeTowerIntegration(&print, print.config(), print.get_plate_index(), print.get_plate_origin(), * print.wipe_tower_data().priming.get(), print.wipe_tower_data().tool_changes, *print.wipe_tower_data().final_purge.get()));
                m_wipe_tower->set_creality_cfs(print.getCrealityCFS());

                m_wipe_tower->set_wipe_tower_depth(print.get_wipe_tower_depth());
                m_wipe_tower->set_wipe_tower_bbx(print.get_wipe_tower_bbx());

                if (!is_bbl_printers && enable_all_extruder_priming) {
                    file.write(m_wipe_tower->prime(*this));
                    // Verify, whether the print overaps the priming extrusions.
                    BoundingBoxf bbox_print(get_print_extrusions_extents(print));
                    coordf_t twolayers_printz = ((layers_to_print.size() == 1) ? layers_to_print.front() : layers_to_print[1]).first + EPSILON;
                    for (const PrintObject *print_object : print.objects())
                        bbox_print.merge(get_print_object_extrusions_extents(*print_object, twolayers_printz));
                    bbox_print.merge(get_wipe_tower_extrusions_extents(print, twolayers_printz));
                    BoundingBoxf bbox_prime(get_wipe_tower_priming_extrusions_extents(print));
                    bbox_prime.offset(0.5f);
                    bool overlap = bbox_prime.overlap(bbox_print);

                    if (print.config().gcode_flavor == gcfMarlinLegacy || print.config().gcode_flavor == gcfMarlinFirmware) {
                        file.write(this->retract());
                        file.write("M300 S800 P500\n"); // Beep for 500ms, tone 800Hz.
                        if (overlap) {
                            // Wait for the user to remove the priming extrusions.
                            file.write("M1 Remove priming towers and click button.\n");
                        } else {
                            // Just wait for a bit to let the user check, that the priming succeeded.
                            //TODO Add a message explaining what the printer is waiting for. This needs a firmware fix.
                            file.write("M1 S10\n");
                        }
                    }
                    else
                    {
                        // This is not Marlin, M1 command is probably not supported.
                        if (overlap) {
                            print.active_step_add_warning(PrintStateBase::WarningLevel::CRITICAL,
                                                          _(L("Your print is very close to the priming regions. "
                                                              "Make sure there is no collision.")));
                        } else {
                            // Just continue printing, no action necessary.
                        }
                    }
                }
                print.throw_if_canceled();
            }
            // Process all layers of all objects (non-sequential mode) with a parallel pipeline:
            // Generate G-code, run the filters (vase mode, cooling buffer), run the G-code analyser
            // and export G-code into file.
            //
            // wwb: if the first layer is not done, we need to set the extruder to the initial extruder
            if (print.objects().at(0)->belt()) {
                this->set_belt(true);
            }
            this->process_layers(print, tool_ordering, print_object_instances_ordering, layers_to_print, file);
            //BBS: close powerlost recovery
            {
                if (is_bbl_printers && m_second_layer_things_done) {
                    file.write("; close powerlost recovery\n");
                    file.write("M1003 S0\n");
                }
            }
            if (m_wipe_tower)
                // Purge the extruder, pull out the active filament.
                file.write(m_wipe_tower->finalize(*this));
        }
    }
    //BBS: the last retraction
    // Write end commands to file.
    file.write(this->retract(false, true));

    // if needed, write the gcode_label_objects_end
    {
        std::string gcode;
        m_writer.add_object_change_labels(gcode);
        file.write(gcode);
    }

    file.write(m_writer.set_fan(0));
    //BBS: make sure the additional fan is closed when end
    if(m_config.auxiliary_fan.value)
        file.write(m_writer.set_additional_fan(0));
    if (is_bbl_printers) {
        //BBS: close spaghetti detector
        //Note: M981 is also used to tell xcam the last layer is finished, so we need always send it even if spaghetti option is disabled.
        //if (print.config().spaghetti_detector.value)
        file.write("M981 S0 P20000 ; close spaghetti detector\n");
    }

    // adds tag for processor
    file.write_format(";%s%s\n", GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Role).c_str(), ExtrusionEntity::role_to_string(erCustom).c_str());

    // Process filament-specific gcode in extruder order.
    {
        DynamicConfig config;
        config.set_key_value("layer_num", new ConfigOptionInt(m_layer_index));
        //BBS
        config.set_key_value("layer_z",   new ConfigOptionFloat(m_writer.get_position()(2) - m_config.z_offset.value));
        config.set_key_value("max_layer_z", new ConfigOptionFloat(m_max_layer_z));
        if (print.config().single_extruder_multi_material) {
            // Process the filament_end_gcode for the active filament only.
            int extruder_id = m_writer.extruder()->id();
            config.set_key_value("filament_extruder_id", new ConfigOptionInt(extruder_id));
            file.writeln(this->placeholder_parser_process("filament_end_gcode", print.config().filament_end_gcode.get_at(extruder_id), extruder_id, &config));
        } else {
            for (const std::string &end_gcode : print.config().filament_end_gcode.values) {
                int extruder_id = (unsigned int)(&end_gcode - &print.config().filament_end_gcode.values.front());
                config.set_key_value("filament_extruder_id", new ConfigOptionInt(extruder_id));
                file.writeln(this->placeholder_parser_process("filament_end_gcode", end_gcode, extruder_id, &config));
            }
        }
        int active_extruder_id = m_writer.extruder()->id();
        config.set_key_value("filament_extruder_id", new ConfigOptionInt(active_extruder_id));
        file.writeln(this->placeholder_parser_process("machine_end_gcode", print.config().machine_end_gcode, active_extruder_id, &config));
    }
    file.write(m_writer.update_progress(m_layer_count, m_layer_count, true)); // 100%
    file.write(m_writer.postamble());

    if (activate_chamber_temp_control && max_chamber_temp > 0)
        file.write(m_writer.set_chamber_temperature(0, false));  //close chamber_temperature

    if (activate_air_filtration) {
        int complete_print_exhaust_fan_speed = 0;
        for (const auto& extruder : m_writer.extruders())
            if (m_config.activate_air_filtration.get_at(extruder.id()))
                complete_print_exhaust_fan_speed = std::max(complete_print_exhaust_fan_speed, m_config.complete_print_exhaust_fan_speed.get_at(extruder.id()));
        file.write(m_writer.set_exhaust_fan(complete_print_exhaust_fan_speed, true, print.is_CX_printer()));
    }
    // adds tags for time estimators
    file.write_format(";%s\n", GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Last_Line_M73_Placeholder).c_str());
    //TIME_ELAPSED
    file.write(";TIME_ELAPSED:" + std::to_string(m_processor.get_time(PrintEstimatedStatistics::ETimeMode::Normal)) + "\n\n");
    file.write_format("; EXECUTABLE_BLOCK_END\n\n");

    print.throw_if_canceled();

    // Get filament stats.
    file.write(DoExport::update_print_stats_and_format_filament_stats(
    	// Const inputs
        has_wipe_tower, print.wipe_tower_data(),
        m_writer.extruders(),
        // Modifies
        print.m_print_statistics));
    print.m_print_statistics.initial_tool = initial_extruder_id;
    if (!is_bbl_printers) {
        file.write_format("; total filament used [g] = %.2lf\n",
            print.m_print_statistics.total_weight);
        file.write_format("; total filament cost = %.2lf\n",
            print.m_print_statistics.total_cost);
        if (print.m_print_statistics.total_toolchanges > 0)
            file.write_format("; total filament change = %i\n",
                print.m_print_statistics.total_toolchanges);
        file.write_format("; total layers count = %i\n", m_layer_count);
        file.write_format(
            ";%s\n",
            GCodeProcessor::reserved_tag(
                GCodeProcessor::ETags::Estimated_Printing_Time_Placeholder)
            .c_str());
      file.write("\n");
      file.write("; CONFIG_BLOCK_START\n");
      std::string full_config;
      append_full_config(print, full_config);
      if (!full_config.empty())
        file.write(full_config);
      // SoftFever: write compatiple info
      int first_layer_bed_temperature = get_bed_temperature(initial_extruder_id, true, print.config().curr_bed_type);
      file.write_format("; first_layer_bed_temperature = %d\n", first_layer_bed_temperature);
      file.write_format("; bed_shape = %s\n", print.full_print_config().opt_serialize("printable_area").c_str());
      file.write_format("; first_layer_temperature = %d\n", print.config().nozzle_temperature_initial_layer.get_at(0));
      file.write_format("; first_layer_height = %.3f\n", print.config().initial_layer_print_height.value);

        //SF TODO
//      file.write_format("; variable_layer_height = %d\n", print.ad.adaptive_layer_height ? 1 : 0);

	// use for gcode preview lite mode to judge whether all print object has shell
	{
		auto dynamic_config_value_greather_zero = [](const DynamicPrintConfig& global_config, const t_config_option_key& key) {
            bool result = true;
            if (global_config.has(key)) {
                int value = global_config.opt_int(key);
                result    = value > 0;
            }
            return result;
        };

		auto model_config_value_greather_zero = [](const ModelConfig& config, const t_config_option_key& key) {
            bool result = true;
            if (config.has(key)) {
                int value = config.opt_int(key);
                result    = value > 0;
            }
            return result;
        };

		bool has_surface_layers = true;
        for (const PrintObject* print_object : print.objects()) {
            const PrintInstances& print_instances = print_object->instances();
            for (const auto& instance : print_instances) {
                ModelObject*     obj     = instance.model_instance->get_object();

				//volumes
                for (ModelVolume* vol : obj->volumes) {
                    ModelConfigObject& vol_config = vol->config;
                    has_surface_layers &= model_config_value_greather_zero(vol_config, "top_shell_layers");
                    has_surface_layers &= model_config_value_greather_zero(vol_config, "bottom_shell_layers");
                    has_surface_layers &= model_config_value_greather_zero(vol_config, "wall_loops");
                    if (has_surface_layers == false)
                        break;
                }
                if (has_surface_layers == false)
                    break;

				// current object
                const ModelConfigObject& config = obj->config;
                has_surface_layers &= model_config_value_greather_zero(config, "top_shell_layers");
                has_surface_layers &= model_config_value_greather_zero(config, "bottom_shell_layers");
                has_surface_layers &= model_config_value_greather_zero(config, "wall_loops");
                if (has_surface_layers == false)
                    break;

				//object`s range profile
                t_layer_config_ranges& range_config = obj->layer_config_ranges;
                for (auto it = range_config.begin(); it != range_config.end(); ++it) {
                    const ModelConfig& range_config = it->second;
                    has_surface_layers &= model_config_value_greather_zero(range_config, "top_shell_layers");
                    has_surface_layers &= model_config_value_greather_zero(range_config, "bottom_shell_layers");
                    has_surface_layers &= model_config_value_greather_zero(range_config, "wall_loops");
                    if (has_surface_layers == false)
                        break;
				}
				if (has_surface_layers == false)
                    break;

            }
            if (has_surface_layers == false)
                break;
        }


        if (has_surface_layers) {
            const DynamicPrintConfig& global_cfg = print.full_print_config();
            has_surface_layers &= dynamic_config_value_greather_zero(global_cfg, "top_shell_layers");
            has_surface_layers &= dynamic_config_value_greather_zero(global_cfg, "bottom_shell_layers");
            has_surface_layers &= dynamic_config_value_greather_zero(global_cfg, "wall_loops");
		}

        file.write_format("; all_surface_with_shell = %d\n", has_surface_layers ? 1 : 0);
	}

	// use for gcode preview lod, find special layers
	{

		const auto &tool_changes = print.wipe_tower_data().tool_changes;
        if (!tool_changes.empty()) {
            std::vector<size_t> layers;
			layers.push_back(0);

			int last_tool_change = tool_changes.front().size();

            for (size_t i = 1; i < tool_changes.size(); i++) {
                int changes = tool_changes.at(i).size();
                if (changes != last_tool_change) {
                    layers.push_back(i-1);
                    last_tool_change = changes;
                }
            }

            layers.push_back(tool_changes.size()-1);

			std::string layers_string;
            for (size_t i = 0; i < layers.size(); i++) {
                if (i != 0)
                    layers_string += ",";
                layers_string += std::to_string(layers[i]);
			}
            file.write_format("; wipe_tower_tool_changes_layers = %s\n", layers_string.c_str());
		}
	}

	// use for gcode preview lod, if wipe_tower_no_sparse_layers == true, to disable lod
    {
        const bool no_sparse = print.config().wipe_tower_no_sparse_layers.value;

		bool has_support = false;
        bool has_adaptive_layer_height = false;
		for (const PrintObject* print_object : print.objects())
		{
			if (print_object->has_support()) {
                has_support = true;
                break;
            }

            if (print_object->model_object()->layer_height_profile.empty() == false) {
                has_adaptive_layer_height = true;
                break;
            }
		}

		bool enable = (!no_sparse) && (!has_support) && (!has_adaptive_layer_height);

		file.write_format("; should_enable_preview_lod = %d\n", enable ? 1 : 0);
	}

      file.write("; CONFIG_BLOCK_END\n\n");

    }
    file.write("\n");

    print.throw_if_canceled();

    BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << " end";
}

//BBS
void GCode::check_placeholder_parser_failed()
{
    if (! m_placeholder_parser_integration.failed_templates.empty()) {
        // G-code export proceeded, but some of the PlaceholderParser substitutions failed.
        std::string msg = Slic3r::format(_(L("Failed to generate gcode for invalid custom G-code.\n\n")));
        for (const auto &name_and_error : m_placeholder_parser_integration.failed_templates)
            msg += name_and_error.first + " " + name_and_error.second + "\n";
        msg += Slic3r::format(_(L("Please check the custom G-code or use the default custom G-code.")));
        throw Slic3r::PlaceholderParserError(msg);
    }
}

// Process all layers of all objects (non-sequential mode) with a parallel pipeline:
// Generate G-code, run the filters (vase mode, cooling buffer), run the G-code analyser
// and export G-code into file.
void GCode::process_layers(
    const Print                                                         &print,
    const ToolOrdering                                                  &tool_ordering,
    const std::vector<const PrintInstance*>                             &print_object_instances_ordering,
    const std::vector<std::pair<coordf_t, std::vector<LayerToPrint>>>   &layers_to_print,
    GCodeOutputStream                                                   &output_stream)
{
    bool first_layer = true;
    // BBS: get object label id
    size_t layer_to_print_idx = 0;
    std::vector<int> object_label;

    for (const PrintInstance* instance : print_object_instances_ordering)
        object_label.push_back(instance->model_instance->get_labeled_id());

    std::vector<LayerResult> layers_results;
    layers_results.resize(layers_to_print.size());

    // The pipeline is variable: The vase mode filter is optional.
    const auto generator = tbb::make_filter<void, LayerResult>(slic3r_tbb_filtermode::serial_in_order,
        [this, &print, &tool_ordering, &print_object_instances_ordering, &layers_to_print, &layer_to_print_idx](tbb::flow_control& fc) -> LayerResult {
            if (layer_to_print_idx >= layers_to_print.size()) {
                if (layer_to_print_idx == layers_to_print.size() + (m_pressure_equalizer ? 1 : 0)) {
                    fc.stop();
                    return {};
                } else {
                    // Pressure equalizer need insert empty input. Because it returns one layer back.
                    // Insert NOP (no operation) layer;
                    ++layer_to_print_idx;
                    return LayerResult::make_nop_layer_result();
                }
            } else {
                const std::pair<coordf_t, std::vector<LayerToPrint>>& layer = layers_to_print[layer_to_print_idx++];
                const LayerTools& layer_tools = tool_ordering.tools_for_layer(layer.first);
                print.set_status(80, Slic3r::format(_(L("Generating G-code: layer %1%")), std::to_string(layer_to_print_idx)));
                if (m_wipe_tower && layer_tools.has_wipe_tower)
                    m_wipe_tower->next_layer();
                //BBS
                check_placeholder_parser_failed();
                print.throw_if_canceled();
                LayerResult res = this->process_layer(print, layer.second, layer_tools, &layer == &layers_to_print.back(), &print_object_instances_ordering, size_t(-1));
                res.gcode_store_pos = layer_to_print_idx - 1;
                return std::move(res);
            }
        });
    if (m_spiral_vase) {
        float nozzle_diameter  = EXTRUDER_CONFIG(nozzle_diameter);
        float max_xy_smoothing = m_config.get_abs_value("spiral_mode_max_xy_smoothing", nozzle_diameter);
        this->m_spiral_vase->set_max_xy_smoothing(max_xy_smoothing);
    }
    const auto spiral_mode = tbb::make_filter<LayerResult, LayerResult>(slic3r_tbb_filtermode::serial_in_order,
        [&spiral_mode = *this->m_spiral_vase.get(), &layers_to_print](LayerResult in) -> LayerResult {
        	if (in.nop_layer_result)
                return in;

            spiral_mode.enable(in.spiral_vase_enable);
            bool last_layer = in.layer_id == layers_to_print.size() - 1;
            return { spiral_mode.process_layer(std::move(in.gcode), last_layer), in.layer_id, in.spiral_vase_enable, in.cooling_buffer_flush, in.gcode_store_pos};
        });
    const auto pressure_equalizer = tbb::make_filter<LayerResult, LayerResult>(slic3r_tbb_filtermode::serial_in_order,
        [pressure_equalizer = this->m_pressure_equalizer.get()](LayerResult in) -> LayerResult {
            return pressure_equalizer->process_layer(std::move(in));
        });

    std::unordered_map<int, AppearanceUnderExtrusionAccelRecoveryFilter::ObjectParams> aue_object_params;
    aue_object_params.reserve(print_object_instances_ordering.size());
    for (const PrintInstance* instance : print_object_instances_ordering) {
        const int               label_id = instance->model_instance->get_labeled_id();
        const PrintObjectConfig& ocfg     = instance->print_object->config();
        aue_object_params[label_id]      = { ocfg.msao_recovery_enable.value, static_cast<float>(ocfg.msao_safe_accel.value), static_cast<float>(ocfg.msao_safe_velocity.value) };
    }

    AppearanceUnderExtrusionAccelRecoveryFilter aue_accel_filter(print.config(), print.default_object_config(), std::move(aue_object_params), m_writer.get_gcode_flavor());
    const auto aue_accel_recovery = tbb::make_filter<std::string, std::string>(slic3r_tbb_filtermode::serial_in_order,
        [&aue_accel_filter](std::string in) -> std::string {
            LayerResult layer{ std::move(in), size_t(0), false, false, size_t(0), 0.0f, false };
            return aue_accel_filter.process_layer(std::move(layer)).gcode;
        });


    std::vector<std::vector<PerExtruderAdjustments>> layers_extruder_adjustments(layers_to_print.size());

    const auto parsing = tbb::make_filter<LayerResult, LayerResult>(slic3r_tbb_filtermode::serial_in_order,
        [&gcode_editor = *this->m_gcode_editor.get(), &layers_extruder_adjustments, object_label](LayerResult in) -> LayerResult {
            // record gcode
            in.gcode = gcode_editor.process_layer(std::move(in.gcode), in.layer_id, layers_extruder_adjustments[in.gcode_store_pos],
                                                  object_label, in.cooling_buffer_flush, false);
            return std::move(in);
        });

    // step2: cooling
    std::vector<std::vector<OutwallCollection>> layers_wall_collection(layers_to_print.size());

    CoolingBuffer cooling_processor;

    const auto cooling = tbb::make_filter<LayerResult, LayerResult>(slic3r_tbb_filtermode::serial_in_order,
        [&cooling_processor, &layers_extruder_adjustments](LayerResult in) -> LayerResult {
            in.layer_time = cooling_processor.calculate_layer_slowdown(layers_extruder_adjustments[in.gcode_store_pos]);
            return std::move(in);
        });

    // step 4.1: record node data
    SmoothCalculator smooth_calculator(object_label.size());

    const auto build_node = tbb::make_filter<LayerResult, void>(slic3r_tbb_filtermode::serial_in_order,
        [&smooth_calculator, &layers_wall_collection, &layers_extruder_adjustments, object_label, &layers_results](LayerResult in) {
            smooth_calculator.build_node(layers_wall_collection[in.gcode_store_pos], object_label, layers_extruder_adjustments[in.gcode_store_pos]);
            layers_results[in.gcode_store_pos] = std::move(in);
            return;
        });

    // step 5: rewrite
    const auto write_gcode = tbb::make_filter<LayerResult, std::string>(slic3r_tbb_filtermode::serial_in_order,
        [&gcode_editor = *this->m_gcode_editor.get(), &layers_extruder_adjustments](LayerResult in) -> std::string {
            return gcode_editor.write_layer_gcode(std::move(in.gcode), in.layer_id, in.layer_time, layers_extruder_adjustments[in.gcode_store_pos]);
        });

    std::vector<LayerResult> gcode_res;

    // BBS: apply new feedrate of outwall and recalculate layer time
    int        layer_idx            = 0;
    const auto calculate_layer_time = tbb::make_filter<void, LayerResult>(slic3r_tbb_filtermode::serial_in_order,
        [&layer_idx, &smooth_calculator, &layers_extruder_adjustments, &gcode_res](tbb::flow_control& fc) -> LayerResult {
            if (layer_idx == gcode_res.size()) {
                fc.stop();
                return {};
            } else {
                if (layer_idx > 0) {
                    gcode_res[layer_idx].layer_time =
                        smooth_calculator.recaculate_layer_time(layer_idx,
                                                                layers_extruder_adjustments[gcode_res[layer_idx].gcode_store_pos]);
                }
                return gcode_res[layer_idx++];
            }
        });

    const auto output = tbb::make_filter<std::string, void>(slic3r_tbb_filtermode::serial_in_order,
        [this, &first_layer,&print,&output_stream,&processor = this->m_processor](std::string s) {

		float layerTime = processor.layer_time();
		std::string strLayerTemp = "";

		if (print.config().material_flow_dependent_temperature.get_at(m_currentExtruder) && !print.getMultiColor() && !first_layer) {
			if (m_temperature <= 0)
			{
				m_temperature = m_config.nozzle_temperature.get_at(m_currentExtruder);
			}
			if (layerTime - m_last_time > 0.0f)
			{
				double avg_flow = (processor.layer_flow() - m_last_flow) / (layerTime - m_last_time);
				double _temperature = m_smoothTemp->getTemp(avg_flow, m_temperature);

				if (_temperature != m_temperature)
				{
					strLayerTemp = m_writer.set_temperatured((float)_temperature, false, m_currentExtruder);
					m_temperature = _temperature;
					s = strLayerTemp + s;
				}
			}
		}
		m_last_flow = processor.layer_flow();
		m_last_time = layerTime;
		first_layer = false;
        output_stream.process_gcode(s);
        float layerTime_new = processor.layer_time();
        float current_layer_time = layerTime_new - layerTime;
        if(current_layer_time > 60.0f)
        {
            int n = current_layer_time / 60.0f +1; //split to n segments, each segment time less than 60s
            n = std::max(1, n);
            float segment_time = current_layer_time / n;

            std::vector<std::string> lines;
            lines.reserve(std::count(s.begin(), s.end(), '\n') + 1);
            std::size_t start = 0;
            while (start < s.size()) {
                const auto pos = s.find('\n', start);
                if (pos == std::string::npos) {
                    lines.emplace_back(s.substr(start));
                    break;
                }
                lines.emplace_back(s.substr(start, pos - start + 1)); // keep newline
                start = pos + 1;
            }

            const std::size_t total_lines = lines.size();
            const std::size_t base_lines_per_segment = total_lines / static_cast<std::size_t>(n);
            std::size_t extra_lines = total_lines % static_cast<std::size_t>(n);
            std::size_t line_index = 0;

            for(int i=0; i<n; i++)
            {
                const std::size_t lines_this_segment = base_lines_per_segment + (static_cast<std::size_t>(i) < extra_lines ? 1 : 0);
                std::vector<std::string> segment_lines;
                segment_lines.reserve(lines_this_segment);
                for (std::size_t j = 0; j < lines_this_segment && line_index < total_lines; ++j, ++line_index) {
                    segment_lines.emplace_back(lines[line_index]);
                }
                if (segment_lines.empty() && line_index < total_lines) {
                    continue;
                }
                if(i < n -1) {
                    const std::string time_line = ";TIME_ELAPSED:" + std::to_string(layerTime + segment_time*(i+1)) + "\n";
                    bool inserted = false;
                    for (auto it = segment_lines.rbegin(); it != segment_lines.rend(); ++it) {
                        bool blank = true;
                        for (char c : *it) {
                            if (c != '\n' && c != '\r' && c != ' ' && c != '\t') {
                                blank = false;
                                break;
                            }
                        }
                        if (blank) {
                            *it = time_line;
                            inserted = true;
                            break;
                        }
                    }
                    if (!inserted) {
                        // No blank line available in this segment, skip inserting to keep line numbers stable.
                    }
                }
                std::string segment_content;
                for (const auto& l : segment_lines)
                    segment_content += l;
                output_stream.write_with_noprocess(segment_content);
            }
        }else{
            output_stream.write_with_noprocess(s);
        }

		//output_stream.write(s);
        layerTime_new = processor.layer_time();
		if (!s.empty())
		{
			std::string strTime = ";TIME_ELAPSED:" + std::to_string(layerTime_new) + "\n\n";
			output_stream.write(strTime);
		}
        }
    );

    const auto fan_mover = tbb::make_filter<std::string, std::string>(slic3r_tbb_filtermode::serial_in_order,
            [&fan_mover = this->m_fan_mover, &config = this->config(), &writer = this->m_writer](std::string in)->std::string {

        CNumericLocalesSetter locales_setter;

        if (config.fan_speedup_time.value != 0 || config.fan_kickstart.value > 0) {
            if (fan_mover.get() == nullptr)
                fan_mover.reset(new Slic3r::FanMover(
                    writer,
                    std::abs((float)config.fan_speedup_time.value),
                    config.fan_speedup_time.value > 0,
                    config.use_relative_e_distances.value,
                    config.fan_speedup_overhangs.value,
                    (float)config.fan_kickstart.value));
            //flush as it's a whole layer
            return fan_mover->process_gcode(in, true);
        }
        return in;
    });

    // BBS: apply cooling
    // The pipeline elements are joined using const references, thus no copying is performed.
    if (m_spiral_vase && m_pressure_equalizer)
        tbb::parallel_pipeline(12, generator & spiral_mode & pressure_equalizer & parsing & cooling & write_gcode & aue_accel_recovery & fan_mover & output);
    else if (m_spiral_vase)
        tbb::parallel_pipeline(12, generator & spiral_mode & parsing & cooling & write_gcode & aue_accel_recovery & fan_mover & output);
    else if (!m_config.z_direction_outwall_speed_continuous) {
        if (m_pressure_equalizer)
            tbb::parallel_pipeline(12, generator & pressure_equalizer & parsing & cooling & write_gcode & aue_accel_recovery & fan_mover & output);
        else
            tbb::parallel_pipeline(12, generator & parsing & cooling & write_gcode & aue_accel_recovery & fan_mover & output);
    } else {
        if (m_pressure_equalizer)
            tbb::parallel_pipeline(12, generator & pressure_equalizer & parsing & cooling & build_node);
        else
            tbb::parallel_pipeline(12, generator & parsing & cooling & build_node);

        // append data
        for (const LayerResult& res : layers_results) {
            // remove empty gcode layer caused by support independent layers
            if (res.cooling_buffer_flush) {
                smooth_calculator.append_data(layers_wall_collection[res.gcode_store_pos]);
                gcode_res.push_back(std::move(res));
            }
        }

        smooth_calculator.smooth_layer_speed();

        tbb::parallel_pipeline(12, calculate_layer_time & write_gcode & aue_accel_recovery & fan_mover & output);
    }
}

// Process all layers of a single object instance (sequential mode) with a parallel pipeline:
// Generate G-code, run the filters (vase mode, cooling buffer), run the G-code analyser
// and export G-code into file.
void GCode::process_layers(
    const Print                             &print,
    const ToolOrdering                      &tool_ordering,
    std::vector<LayerToPrint>                layers_to_print,
    const size_t                             single_object_idx,
    GCodeOutputStream                       &output_stream,
    // BBS
    const bool                               prime_extruder)
{
    bool first_layer = true;

    // The pipeline should be
    // generator + (spiral) + (pressure_equalizer) + parse + cooling + (smoothing) + rewrite
    // rewrite pipeline to get better schu

    // BBS: get object label id
    size_t           layer_to_print_idx = 0;
    std::vector<int> object_label;
    for (LayerToPrint layer : layers_to_print)
        object_label.push_back(layer.original_object->instances()[single_object_idx].model_instance->get_labeled_id());

    std::vector<LayerResult> layers_results;
    layers_results.resize(layers_to_print.size());

    // step 1: generator
    // The pipeline is variable: The vase mode filter is optional.
    const auto generator = tbb::make_filter<void, LayerResult>(slic3r_tbb_filtermode::serial_in_order,
        [this, &print, &tool_ordering, &layers_to_print, &layer_to_print_idx, single_object_idx, prime_extruder](tbb::flow_control& fc) -> LayerResult {
            if (layer_to_print_idx >= layers_to_print.size()) {
                if (layer_to_print_idx == layers_to_print.size() + (m_pressure_equalizer ? 1 : 0)) {
                    fc.stop();
                    return {};
                } else {
                    // Pressure equalizer need insert empty input. Because it returns one layer back.
                    // Insert NOP (no operation) layer;
                    ++layer_to_print_idx;
                    return LayerResult::make_nop_layer_result();
                }
            } else {
                LayerToPrint &layer = layers_to_print[layer_to_print_idx ++];
                print.set_status(80, Slic3r::format(_(L("Generating G-code: layer %1%")), std::to_string(layer_to_print_idx)));
                //BBS
                check_placeholder_parser_failed();
                print.throw_if_canceled();
                LayerResult res     = this->process_layer(print, {std::move(layer)}, tool_ordering.tools_for_layer(layer.print_z()),
                                                          &layer == &layers_to_print.back(), nullptr, single_object_idx, prime_extruder);
                res.gcode_store_pos = layer_to_print_idx - 1;
                return std::move(res);
            }
        });
    if (m_spiral_vase) {
        float nozzle_diameter  = EXTRUDER_CONFIG(nozzle_diameter);
        float max_xy_smoothing = m_config.get_abs_value("spiral_mode_max_xy_smoothing", nozzle_diameter);
        this->m_spiral_vase->set_max_xy_smoothing(max_xy_smoothing);
    }
    const auto spiral_mode = tbb::make_filter<LayerResult, LayerResult>(slic3r_tbb_filtermode::serial_in_order,
        [&spiral_mode = *this->m_spiral_vase.get(), &layers_to_print](LayerResult in)->LayerResult {
            if (in.nop_layer_result)
                return in;
            spiral_mode.enable(in.spiral_vase_enable);
            bool last_layer = in.layer_id == layers_to_print.size() - 1;
            return { spiral_mode.process_layer(std::move(in.gcode), last_layer), in.layer_id, in.spiral_vase_enable, in.cooling_buffer_flush, in.gcode_store_pos };
        });
    const auto pressure_equalizer = tbb::make_filter<LayerResult, LayerResult>(slic3r_tbb_filtermode::serial_in_order,
        [pressure_equalizer = this->m_pressure_equalizer.get()](LayerResult in) -> LayerResult {
             return pressure_equalizer->process_layer(std::move(in));
        });

    std::unordered_map<int, AppearanceUnderExtrusionAccelRecoveryFilter::ObjectParams> aue_object_params;
    if (!layers_to_print.empty() && layers_to_print.front().original_object != nullptr) {
        const PrintObject*      aue_object = layers_to_print.front().original_object;
        const int               label_id   = aue_object->instances()[single_object_idx].model_instance->get_labeled_id();
        const PrintObjectConfig& ocfg      = aue_object->config();
        aue_object_params[label_id]       = { ocfg.msao_recovery_enable.value, static_cast<float>(ocfg.msao_safe_accel.value), static_cast<float>(ocfg.msao_safe_velocity.value) };
    }

    AppearanceUnderExtrusionAccelRecoveryFilter aue_accel_filter(print.config(), print.default_object_config(), std::move(aue_object_params), m_writer.get_gcode_flavor());
    const auto aue_accel_recovery = tbb::make_filter<std::string, std::string>(slic3r_tbb_filtermode::serial_in_order,
        [&aue_accel_filter](std::string in) -> std::string {
            LayerResult layer{ std::move(in), size_t(0), false, false, size_t(0), 0.0f, false };
            return aue_accel_filter.process_layer(std::move(layer)).gcode;
        });


    // BBS: get objects and nodes info, for better arrange
    const ConstPrintObjectPtrsAdaptor& objects = print.objects();

    // step 2: parse
    std::vector<std::vector<PerExtruderAdjustments>> layers_extruder_adjustments(layers_to_print.size());

    const auto parsing = tbb::make_filter<LayerResult, LayerResult>(slic3r_tbb_filtermode::serial_in_order,
        [&gcode_editor = *this->m_gcode_editor.get(), &layers_extruder_adjustments, object_label](LayerResult in) -> LayerResult {
            // record gcode
            in.gcode = gcode_editor.process_layer(std::move(in.gcode), in.layer_id, layers_extruder_adjustments[in.gcode_store_pos],
                                                  object_label, in.cooling_buffer_flush, false);
            return std::move(in);
        });

    // step 3: cooling
    std::vector<std::vector<OutwallCollection>> layers_wall_collection(layers_to_print.size());
    CoolingBuffer                               cooling_processor;

    const auto cooling = tbb::make_filter<LayerResult, LayerResult>(slic3r_tbb_filtermode::serial_in_order,
        [&cooling_processor, &layers_extruder_adjustments](LayerResult in) -> LayerResult {
            in.layer_time = cooling_processor.calculate_layer_slowdown(layers_extruder_adjustments[in.gcode_store_pos]);
            return std::move(in);
        });

    // step 4.1: record node data
    SmoothCalculator smooth_calculator(object_label.size());

    const auto build_node = tbb::make_filter<LayerResult, void>(slic3r_tbb_filtermode::serial_in_order,
        [&smooth_calculator, &layers_wall_collection, &layers_extruder_adjustments, object_label, &layers_results](LayerResult in) {
            smooth_calculator.build_node(layers_wall_collection[in.gcode_store_pos], object_label, layers_extruder_adjustments[in.gcode_store_pos]);
            layers_results[in.gcode_store_pos] = std::move(in);
            return;
        });

    // step 5: rewrite
    const auto write_gcode = tbb::make_filter<LayerResult, std::string>(slic3r_tbb_filtermode::serial_in_order,
        [&gcode_editor = *this->m_gcode_editor.get(), &layers_extruder_adjustments](LayerResult in) -> std::string {
            return gcode_editor.write_layer_gcode(std::move(in.gcode), in.layer_id, in.layer_time, layers_extruder_adjustments[in.gcode_store_pos]);
        });

    std::vector<LayerResult> gcode_res;

    // BBS: apply new feedrate of outwall and recalculate layer time
    int        layer_idx            = 0;
    // restart pipeline
    const auto calculate_layer_time = tbb::make_filter<void, LayerResult>(slic3r_tbb_filtermode::serial_in_order,
        [&layer_idx, &gcode_res, &smooth_calculator, &layers_extruder_adjustments](tbb::flow_control& fc) -> LayerResult {
            if (layer_idx == gcode_res.size()) {
                fc.stop();
                return {};
            } else {
                if (layer_idx > 0) {
                    gcode_res[layer_idx].layer_time =
                        smooth_calculator.recaculate_layer_time(layer_idx,
                                                                layers_extruder_adjustments[gcode_res[layer_idx].gcode_store_pos]);
                }
                return gcode_res[layer_idx++];
            }
        });

    const auto output = tbb::make_filter<std::string, void>(slic3r_tbb_filtermode::serial_in_order,
        [this, &first_layer, &print, &output_stream, &processor = this->m_processor](std::string s) {

		float layerTime = processor.layer_time();
		std::string strLayerTemp = "";

		if (print.config().material_flow_dependent_temperature.get_at(m_currentExtruder) && !print.getMultiColor() && !first_layer) {
			if (m_temperature <= 0)
			{
				m_temperature = m_config.nozzle_temperature.get_at(m_currentExtruder);
			}
			if (layerTime - m_last_time > 0.0f)
			{
				double avg_flow = (processor.layer_flow() - m_last_flow) / (layerTime - m_last_time);
				double _temperature = m_smoothTemp->getTemp(avg_flow, m_temperature);

				if (_temperature != m_temperature)
				{
					strLayerTemp = m_writer.set_temperatured((float)_temperature, false, m_currentExtruder);
					m_temperature = _temperature;
					s = strLayerTemp + s;
				}
			}
		}
		m_last_flow = processor.layer_flow();
		m_last_time = layerTime;
		first_layer = false;
        output_stream.write(s);


        float layerTime_new = processor.layer_time();
		if (!s.empty())
		{
			std::string strTime = ";TIME_ELAPSED:" + std::to_string(layerTime_new) + "\n\n";
			output_stream.write(strTime);
		}
		    }
    );

    const auto fan_mover = tbb::make_filter<std::string, std::string>(slic3r_tbb_filtermode::serial_in_order,
        [&fan_mover = this->m_fan_mover, &config = this->config(), &writer = this->m_writer](std::string in)->std::string {

        if (config.fan_speedup_time.value != 0 || config.fan_kickstart.value > 0) {
            if (fan_mover.get() == nullptr)
                fan_mover.reset(new Slic3r::FanMover(
                    writer,
                    std::abs((float)config.fan_speedup_time.value),
                    config.fan_speedup_time.value > 0,
                    config.use_relative_e_distances.value,
                    config.fan_speedup_overhangs.value,
                    (float)config.fan_kickstart.value));
            //flush as it's a whole layer
            return fan_mover->process_gcode(in, true);
        }
        return in;
    });

    // BBS: apply cooling
    // The pipeline elements are joined using const references, thus no copying is performed.
    if (m_spiral_vase && m_pressure_equalizer)
        tbb::parallel_pipeline(12, generator & spiral_mode & pressure_equalizer & parsing & cooling & write_gcode & aue_accel_recovery & fan_mover & output);
    else if (m_spiral_vase)
        tbb::parallel_pipeline(12, generator & spiral_mode & parsing & cooling & write_gcode & aue_accel_recovery & fan_mover & output);
    else if (!m_config.z_direction_outwall_speed_continuous) {
        if (m_pressure_equalizer)
            tbb::parallel_pipeline(12, generator & pressure_equalizer & parsing & cooling & write_gcode & aue_accel_recovery & fan_mover & output);
        else
            tbb::parallel_pipeline(12, generator & parsing & cooling & write_gcode & aue_accel_recovery & fan_mover & output);
    } else {
        if (m_pressure_equalizer)
            tbb::parallel_pipeline(12, generator & pressure_equalizer & parsing & cooling & build_node);
        else
            tbb::parallel_pipeline(12, generator & parsing & cooling & build_node);
        // step 4.2: smoothing
        // break pipeline and do z smoothing
        // append data
        for (const LayerResult& res : layers_results) {
            // remove empty gcode layer caused by support independent layers
            if (res.cooling_buffer_flush) {
                smooth_calculator.append_data(layers_wall_collection[res.gcode_store_pos]);
                gcode_res.push_back(res);
            }
        }

        smooth_calculator.smooth_layer_speed();

        tbb::parallel_pipeline(12, calculate_layer_time & write_gcode & aue_accel_recovery & fan_mover & output);
    }
}

std::string GCode::placeholder_parser_process(const std::string &name, const std::string &templ, unsigned int current_extruder_id, const DynamicConfig *config_override)
{
    // Orca: Added CMake config option since debug is rarely used in current workflow.
    // Also changed from throwing error immediately to storing messages till slicing is completed
    // to raise all errors at the same time.
#if ORCA_CHECK_GCODE_PLACEHOLDERS
    if (config_override) {
        const auto& custom_gcode_placeholders = custom_gcode_specific_placeholders();

        // 1-st check: custom G-code "name" have to be present in s_CustomGcodeSpecificOptions;
        //if (custom_gcode_placeholders.count(name) > 0) {
        //    const auto& placeholders = custom_gcode_placeholders.at(name);
        if (auto it = custom_gcode_placeholders.find(name); it != custom_gcode_placeholders.end()) {
            const auto& placeholders = it->second;

            for (const std::string& key : config_override->keys()) {
                // 2-nd check: "key" have to be present in s_CustomGcodeSpecificOptions for "name" custom G-code ;
                if (std::find(placeholders.begin(), placeholders.end(), key) == placeholders.end()) {
                    auto& vector = m_placeholder_error_messages[name + " - option not specified for custom gcode type (s_CustomGcodeSpecificOptions)"];
                    if (std::find(vector.begin(), vector.end(), key) == vector.end())
                        vector.emplace_back(key);
                }
                // 3-rd check: "key" have to be present in CustomGcodeSpecificConfigDef for "key" placeholder;
                if (!custom_gcode_specific_config_def.has(key)) {
                    auto& vector = m_placeholder_error_messages[name + " - option has no definition (CustomGcodeSpecificConfigDef)"];
                    if (std::find(vector.begin(), vector.end(), key) == vector.end())
                        vector.emplace_back(key);
                }
            }
        }
        else {
            auto& vector = m_placeholder_error_messages[name + " - gcode type not found in s_CustomGcodeSpecificOptions"];
            if (vector.empty())
                vector.emplace_back("");
        }
    }
#endif

PlaceholderParserIntegration &ppi = m_placeholder_parser_integration;
    try {
        ppi.update_from_gcodewriter(m_writer);
        std::string output = ppi.parser.process(templ, current_extruder_id, config_override, &ppi.output_config, &ppi.context);
        ppi.validate_output_vector_variables();

        if (const std::vector<double> &pos = ppi.opt_position->values; ppi.position != pos) {
            // Update G-code writer.
            m_writer.set_position({ pos[0], pos[1], pos[2] });
            this->set_last_pos(this->gcode_to_point({ pos[0], pos[1] }));
        }

        for (const Extruder &e : m_writer.extruders()) {
            unsigned int eid = e.id();
            assert(eid < ppi.num_extruders);
            if ( eid < ppi.num_extruders) {
                if (! m_writer.config.use_relative_e_distances && ! is_approx(ppi.e_position[eid], ppi.opt_e_position->values[eid]))
                    const_cast<Extruder&>(e).set_position(ppi.opt_e_position->values[eid]);
                if (! is_approx(ppi.e_retracted[eid], ppi.opt_e_retracted->values[eid]) ||
                    ! is_approx(ppi.e_restart_extra[eid], ppi.opt_e_restart_extra->values[eid]))
                    const_cast<Extruder&>(e).set_retracted(ppi.opt_e_retracted->values[eid], ppi.opt_e_restart_extra->values[eid]);
            }
        }

        return output;
    }
    catch (std::runtime_error &err)
    {
        // Collect the names of failed template substitutions for error reporting.
        auto it = ppi.failed_templates.find(name);
        if (it == ppi.failed_templates.end())
            // Only if there was no error reported for this template, store the first error message into the map to be reported.
            // We don't want to collect error message for each and every occurence of a single custom G-code section.
            ppi.failed_templates.insert(it, std::make_pair(name, std::string(err.what())));
        // Insert the macro error message into the G-code.
        return
            std::string("\n!!!!! Failed to process the custom G-code template ") + name + "\n" +
            err.what() +
            "!!!!! End of an error report for the custom G-code template " + name + "\n\n";
    }
}

// Parse the custom G-code, try to find mcode_set_temp_dont_wait and mcode_set_temp_and_wait or optionally G10 with temperature inside the custom G-code.
// Returns true if one of the temp commands are found, and try to parse the target temperature value into temp_out.
static bool custom_gcode_sets_temperature(const std::string &gcode, const int mcode_set_temp_dont_wait, const int mcode_set_temp_and_wait, const bool include_g10, int &temp_out)
{
    temp_out = -1;
    if (gcode.empty())
        return false;

    const char *ptr = gcode.data();
    bool temp_set_by_gcode = false;
    while (*ptr != 0) {
        // Skip whitespaces.
        for (; *ptr == ' ' || *ptr == '\t'; ++ ptr);
        if (*ptr == 'M' || // Line starts with 'M'. It is a machine command.
            (*ptr == 'G' && include_g10)) { // Only check for G10 if requested
            bool is_gcode = *ptr == 'G';
            ++ ptr;
            // Parse the M or G code value.
            char *endptr = nullptr;
            int mgcode = int(strtol(ptr, &endptr, 10));
            if (endptr != nullptr && endptr != ptr &&
                is_gcode ?
                    // G10 found
                    mgcode == 10 :
                    // M104/M109 or M140/M190 found.
                    (mgcode == mcode_set_temp_dont_wait || mgcode == mcode_set_temp_and_wait)) {
                ptr = endptr;
                if (! is_gcode)
                    // Let the caller know that the custom M-code sets the temperature.
                    temp_set_by_gcode = true;
                // Now try to parse the temperature value.
                // While not at the end of the line:
                while (strchr(";\r\n\0", *ptr) == nullptr) {
                    // Skip whitespaces.
                    for (; *ptr == ' ' || *ptr == '\t'; ++ ptr);
                    if (*ptr == 'S') {
                        // Skip whitespaces.
                        for (++ ptr; *ptr == ' ' || *ptr == '\t'; ++ ptr);
                        // Parse an int.
                        endptr = nullptr;
                        long temp_parsed = strtol(ptr, &endptr, 10);
                        if (endptr > ptr) {
                            ptr = endptr;
                            temp_out = temp_parsed;
                            // Let the caller know that the custom G-code sets the temperature
                            // Only do this after successfully parsing temperature since G10
                            // can be used for other reasons
                            temp_set_by_gcode = true;
                        }
                    } else {
                        // Skip this word.
                        for (; strchr(" \t;\r\n\0", *ptr) == nullptr; ++ ptr);
                    }
                }
            }
        }
        // Skip the rest of the line.
        for (; *ptr != 0 && *ptr != '\r' && *ptr != '\n'; ++ ptr);
        // Skip the end of line indicators.
        for (; *ptr == '\r' || *ptr == '\n'; ++ ptr);
    }
    return temp_set_by_gcode;
}

// Print the machine envelope G-code for the Marlin firmware based on the "machine_max_xxx" parameters.
// Do not process this piece of G-code by the time estimator, it already knows the values through another sources.
void GCode::print_machine_envelope(GCodeOutputStream &file, Print &print)
{
    const auto flavor = print.config().gcode_flavor.value;
    if ((flavor == gcfMarlinLegacy || flavor == gcfMarlinFirmware || flavor == gcfRepRapFirmware) &&
        print.config().emit_machine_limits_to_gcode.value == true) {
        int factor = flavor == gcfRepRapFirmware ? 60 : 1; // RRF M203 and M566 are in mm/min
        file.write_format("M201 X%d Y%d Z%d E%d\n",
            int(print.config().machine_max_acceleration_x.values.front() + 0.5),
            int(print.config().machine_max_acceleration_y.values.front() + 0.5),
            int(print.config().machine_max_acceleration_z.values.front() + 0.5),
            int(print.config().machine_max_acceleration_e.values.front() + 0.5));
        file.write_format("M203 X%d Y%d Z%d E%d\n",
            int(print.config().machine_max_speed_x.values.front() * factor + 0.5),
            int(print.config().machine_max_speed_y.values.front() * factor + 0.5),
            int(print.config().machine_max_speed_z.values.front() * factor + 0.5),
            int(print.config().machine_max_speed_e.values.front() * factor + 0.5));

        // Now M204 - acceleration. This one is quite hairy thanks to how Marlin guys care about
        // Legacy Marlin should export travel acceleration the same as printing acceleration.
        // MarlinFirmware has the two separated.
        int travel_acc = flavor == gcfMarlinLegacy
                       ? int(print.config().machine_max_acceleration_extruding.values.front() + 0.5)
                       : int(print.config().machine_max_acceleration_travel.values.front() + 0.5);
        if (flavor == gcfRepRapFirmware)
            file.write_format("M204 P%d T%d ; sets acceleration (P, T), mm/sec^2\n",
                int(print.config().machine_max_acceleration_extruding.values.front() + 0.5),
                travel_acc);
        else if (flavor == gcfMarlinFirmware)
            // New Marlin uses M204 P[print] R[retract] T[travel]
            file.write_format("M204 P%d R%d T%d ; sets acceleration (P, T) and retract acceleration (R), mm/sec^2\n",
                int(print.config().machine_max_acceleration_extruding.values.front() + 0.5),
                int(print.config().machine_max_acceleration_retracting.values.front() + 0.5),
                int(print.config().machine_max_acceleration_travel.values.front() + 0.5));
        else
            file.write_format("M204 P%d R%d T%d\n",
                int(print.config().machine_max_acceleration_extruding.values.front() + 0.5),
                int(print.config().machine_max_acceleration_retracting.values.front() + 0.5),
                travel_acc);

        assert(is_decimal_separator_point());
        file.write_format(flavor == gcfRepRapFirmware
            ? "M566 X%.2lf Y%.2lf Z%.2lf E%.2lf ; sets the jerk limits, mm/min\n"
            : "M205 X%.2lf Y%.2lf Z%.2lf E%.2lf ; sets the jerk limits, mm/sec\n",
            print.config().machine_max_jerk_x.values.front() * factor,
            print.config().machine_max_jerk_y.values.front() * factor,
            print.config().machine_max_jerk_z.values.front() * factor,
            print.config().machine_max_jerk_e.values.front() * factor);
    }
}

// BBS
int GCode::get_bed_temperature(const int extruder_id, const bool is_first_layer, const BedType bed_type,
    const std::vector<unsigned int>* extruder_ids) const
{
    
    std::string bed_temp_key = is_first_layer ? get_bed_temp_1st_layer_key(bed_type) : get_bed_temp_key(bed_type);
    const ConfigOptionInts* bed_temp_opt = m_config.option<ConfigOptionInts>(bed_temp_key);
    const bool is_multi_nozzle_printer = !m_config.single_extruder_multi_material.value && m_config.nozzle_diameter.values.size() > 1;
    if (!is_multi_nozzle_printer)
        return bed_temp_opt->get_at(extruder_id);

    if (m_config.bed_temperature_mode.value == BedTemperatureMode::UseMaxTemperature) {
        int max_temp = 0;
        bool has_printing_temp = false;
        auto collect_temp = [&](unsigned int idx) {
            if (idx < bed_temp_opt->size()) {
                max_temp = std::max(max_temp, bed_temp_opt->get_at(int(idx)));
                has_printing_temp = true;
            }
        };

        if (extruder_ids != nullptr && !extruder_ids->empty()) {
            for (unsigned int idx : *extruder_ids)
                collect_temp(idx);
        } else if (!m_writer.extruders().empty()) {
            for (const Extruder& extruder : m_writer.extruders())
                collect_temp(extruder.id());
        }

        if (!has_printing_temp)
            for (size_t idx = 0; idx < bed_temp_opt->size(); ++idx)
                collect_temp((unsigned int)idx);

        return max_temp;
    }
    return bed_temp_opt->get_at(extruder_id);
}


// Write 1st layer bed temperatures into the G-code.
// Only do that if the start G-code does not already contain any M-code controlling an extruder temperature.
// M140 - Set Extruder Temperature
// M190 - Set Extruder Temperature and Wait
void GCode::_print_first_layer_bed_temperature(GCodeOutputStream &file, Print &print, const std::string &gcode, unsigned int first_printing_extruder_id, bool wait)
{
    // Initial bed temperature based on the first extruder.
    // BBS
    std::vector<int> temps_per_bed;
    int bed_temp = get_bed_temperature(first_printing_extruder_id, true, print.config().curr_bed_type);

    // Is the bed temperature set by the provided custom G-code?
    int  temp_by_gcode     = -1;
    bool temp_set_by_gcode = custom_gcode_sets_temperature(gcode, 140, 190, false, temp_by_gcode);
    // BBS
#if 0
    if (temp_set_by_gcode && temp_by_gcode >= 0 && temp_by_gcode < 1000)
        temp = temp_by_gcode;
#endif

    // Always call m_writer.set_bed_temperature() so it will set the internal "current" state of the bed temp as if
    // the custom start G-code emited these.
    std::string set_temp_gcode = m_writer.set_bed_temperature(bed_temp, wait);
    if (! temp_set_by_gcode)
        file.write(set_temp_gcode);
}

// Write 1st layer extruder temperatures into the G-code.
// Only do that if the start G-code does not already contain any M-code controlling an extruder temperature.
// M104 - Set Extruder Temperature
// M109 - Set Extruder Temperature and Wait
// RepRapFirmware: G10 Sxx
void GCode::_print_first_layer_extruder_temperatures(GCodeOutputStream &file, Print &print, const std::string &gcode, unsigned int first_printing_extruder_id, bool wait)
{
    // Is the bed temperature set by the provided custom G-code?
    int  temp_by_gcode = -1;
    bool include_g10   = print.config().gcode_flavor == gcfRepRapFirmware;
    if (custom_gcode_sets_temperature(gcode, 104, 109, include_g10, temp_by_gcode)) {
        // Set the extruder temperature at m_writer, but throw away the generated G-code as it will be written with the custom G-code.
        int temp = print.config().nozzle_temperature_initial_layer.get_at(first_printing_extruder_id);
        if (temp_by_gcode >= 0 && temp_by_gcode < 1000)
            temp = temp_by_gcode;

        if (print.config().material_flow_dependent_temperature.get_at(first_printing_extruder_id) && !print.getMultiColor()) {
            m_temperature = temp;
        }
        m_writer.set_temperature(temp, wait, first_printing_extruder_id);
    } else {
        // Custom G-code does not set the extruder temperature. Do it now.
        if (print.config().single_extruder_multi_material.value) {
            // Set temperature of the first printing extruder only.
            int temp = print.config().nozzle_temperature_initial_layer.get_at(first_printing_extruder_id);
            if (temp > 0)
            {
                if (print.config().material_flow_dependent_temperature.get_at(first_printing_extruder_id) && !print.getMultiColor()) {
                    m_temperature = temp;
                }
                file.write(m_writer.set_temperature(temp, wait, first_printing_extruder_id));
            }
        } else {
            // Set temperatures of all the printing extruders.
            for (unsigned int tool_id : print.extruders()) {
                int temp = print.config().nozzle_temperature_initial_layer.get_at(tool_id);
                if (print.config().ooze_prevention.value)
                {
                    if (print.config().idle_temperature.get_at(tool_id) == 0)
                        temp += print.config().standby_temperature_delta.value;
                    else
                        temp = print.config().idle_temperature.get_at(tool_id);
                }
                if (temp > 0)
                {
                    if (print.config().material_flow_dependent_temperature.get_at(tool_id) && !print.getMultiColor()) {
                        m_temperature = temp;
                    }

                    if (tool_id != first_printing_extruder_id){
                        file.write(m_writer.set_temperature(temp, wait, tool_id));
                    }
                }
            }
        }
    }
}

inline GCode::ObjectByExtruder& object_by_extruder(
    std::map<unsigned int, std::vector<GCode::ObjectByExtruder>> &by_extruder,
    unsigned int                                                  extruder_id,
    size_t                                                        object_idx,
    size_t                                                        num_objects)
{
    std::vector<GCode::ObjectByExtruder> &objects_by_extruder = by_extruder[extruder_id];
    if (objects_by_extruder.empty())
        objects_by_extruder.assign(num_objects, GCode::ObjectByExtruder());
    return objects_by_extruder[object_idx];
}

inline std::vector<GCode::ObjectByExtruder::Island>& object_islands_by_extruder(
    std::map<unsigned int, std::vector<GCode::ObjectByExtruder>>  &by_extruder,
    unsigned int                                                   extruder_id,
    size_t                                                         object_idx,
    size_t                                                         num_objects,
    size_t                                                         num_islands)
{
    std::vector<GCode::ObjectByExtruder::Island> &islands = object_by_extruder(by_extruder, extruder_id, object_idx, num_objects).islands;
    if (islands.empty())
        islands.assign(num_islands, GCode::ObjectByExtruder::Island());
    return islands;
}

std::vector<GCode::InstanceToPrint> GCode::sort_print_object_instances(
    std::vector<GCode::ObjectByExtruder> 		&objects_by_extruder,
    const std::vector<LayerToPrint> 			&layers,
    // Ordering must be defined for normal (non-sequential print).
    const std::vector<const PrintInstance*> 	*ordering,
    // For sequential print, the instance of the object to be printing has to be defined.
    const size_t                     		 	 single_object_instance_idx)
{
    std::vector<InstanceToPrint> out;

    if (ordering == nullptr) {
        // Sequential print, single object is being printed.
        for (ObjectByExtruder &object_by_extruder : objects_by_extruder) {
            const size_t       layer_id     = &object_by_extruder - objects_by_extruder.data();
            //BBS:add the support of shared print object
            const PrintObject *print_object = layers[layer_id].original_object;
            //const PrintObject *print_object = layers[layer_id].object();
            if (print_object)
                out.emplace_back(object_by_extruder, layer_id, *print_object, single_object_instance_idx, print_object->instances()[single_object_instance_idx].model_instance->get_labeled_id());
        }
    } else {
        // Create mapping from PrintObject* to ObjectByExtruder*.
        std::vector<std::pair<const PrintObject*, ObjectByExtruder*>> sorted;
        sorted.reserve(objects_by_extruder.size());
        for (ObjectByExtruder &object_by_extruder : objects_by_extruder) {
            const size_t       layer_id     = &object_by_extruder - objects_by_extruder.data();
            //BBS:add the support of shared print object
            const PrintObject *print_object = layers[layer_id].original_object;
            //const PrintObject *print_object = layers[layer_id].object();
            if (print_object)
                sorted.emplace_back(print_object, &object_by_extruder);
        }
        std::sort(sorted.begin(), sorted.end());

        if (! sorted.empty()) {
            out.reserve(sorted.size());
            for (const PrintInstance *instance : *ordering) {
                const PrintObject &print_object = *instance->print_object;
                //BBS:add the support of shared print object
                //const PrintObject* print_obj_ptr = &print_object;
                //if (print_object.get_shared_object())
                //    print_obj_ptr = print_object.get_shared_object();
                std::pair<const PrintObject*, ObjectByExtruder*> key(&print_object, nullptr);
                auto it = std::lower_bound(sorted.begin(), sorted.end(), key);
                if (it != sorted.end() && it->first == &print_object)
                    // ObjectByExtruder for this PrintObject was found.
                    out.emplace_back(*it->second, it->second - objects_by_extruder.data(), print_object, instance - print_object.instances().data(), instance->model_instance->get_labeled_id());
            }
        }
    }
    return out;
}

namespace ProcessLayer
{

    static std::string emit_custom_gcode_per_print_z(
        GCode                                                   &gcodegen,
        const CustomGCode::Item 								*custom_gcode,
        unsigned int                                             current_extruder_id,
        // ID of the first extruder printing this layer.
        unsigned int                                             first_extruder_id,
        const PrintConfig                                       &config)
    {
        std::string gcode;
        // BBS
        bool single_filament_print = config.filament_diameter.size() == 1;

        if (custom_gcode != nullptr) {
            // Extruder switches are processed by LayerTools, they should be filtered out.
            assert(custom_gcode->type != CustomGCode::ToolChange);

            CustomGCode::Type   gcode_type = custom_gcode->type;
            bool  				color_change = gcode_type == CustomGCode::ColorChange;
            bool 				tool_change = gcode_type == CustomGCode::ToolChange;
            // Tool Change is applied as Color Change for a single extruder printer only.
            assert(!tool_change || single_filament_print);

            std::string pause_print_msg;
            int m600_extruder_before_layer = -1;
            if (color_change && custom_gcode->extruder > 0)
                m600_extruder_before_layer = custom_gcode->extruder - 1;
            else if (gcode_type == CustomGCode::PausePrint)
                pause_print_msg = custom_gcode->extra;
            //BBS: inserting color gcode is removed
#if 0
            // we should add or not colorprint_change in respect to nozzle_diameter count instead of really used extruders count
            if (color_change || tool_change)
            {
                assert(m600_extruder_before_layer >= 0);
                // Color Change or Tool Change as Color Change.
                // add tag for processor
                gcode += ";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Color_Change) + ",T" + std::to_string(m600_extruder_before_layer) + "," + custom_gcode->color + "\n";

                if (!single_filament_print && m600_extruder_before_layer >= 0 && first_extruder_id != (unsigned)m600_extruder_before_layer
                    // && !MMU1
                    ) {
                    //! FIXME_in_fw show message during print pause
                    DynamicConfig cfg;
                    cfg.set_key_value("color_change_extruder", new ConfigOptionInt(m600_extruder_before_layer));
                    gcode += gcodegen.placeholder_parser_process("machine_pause_gcode", config.machine_pause_gcode, current_extruder_id, &cfg);
                    gcode += "\n";
                    gcode += "M117 Change filament for Extruder " + std::to_string(m600_extruder_before_layer) + "\n";
                }
                else {
                    gcode += gcodegen.placeholder_parser_process("color_change_gcode", config.color_change_gcode, current_extruder_id);
                    gcode += "\n";
                    //FIXME Tell G-code writer that M600 filled the extruder, thus the G-code writer shall reset the extruder to unretracted state after
                    // return from M600. Thus the G-code generated by the following line is ignored.
                    // see GH issue #6362
                    gcodegen.writer().unretract();
                }
            }
            else {
#endif
                if (gcode_type == CustomGCode::PausePrint) // Pause print
                {
                    // add tag for processor
                    gcode += ";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Pause_Print) + "\n";
                    //! FIXME_in_fw show message during print pause
                    //if (!pause_print_msg.empty())
                    //    gcode += "M117 " + pause_print_msg + "\n";
                    gcode += gcodegen.placeholder_parser_process("machine_pause_gcode", config.machine_pause_gcode, current_extruder_id) + "\n";
                }
                else {
                    // add tag for processor
                    gcode += ";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Custom_Code) + "\n";
                    if (gcode_type == CustomGCode::Template)    // Template Custom Gcode
                        gcode += gcodegen.placeholder_parser_process("template_custom_gcode", config.template_custom_gcode, current_extruder_id);
                    else                                        // custom Gcode
                        gcode += custom_gcode->extra;

                }
                gcode += "\n";
#if 0
            }
#endif
        }

        return gcode;
    }
} // namespace ProcessLayer

namespace Skirt {
    static void skirt_loops_per_extruder_all_printing(const Print &print, const ExtrusionEntityCollection &skirt, const LayerTools &layer_tools, std::map<unsigned int, std::pair<size_t, size_t>> &skirt_loops_per_extruder_out,unsigned int& _currentExtruder)
    {
        // Prime all extruders printing over the 1st layer over the skirt lines.
        size_t n_loops = skirt.entities.size();
        size_t n_tools = layer_tools.extruders.size();
        size_t lines_per_extruder = (n_loops + n_tools - 1) / n_tools;

        // BBS. Extrude skirt with first extruder if min_skirt_length is zero
        //ORCA: Always extrude skirt with first extruder, independantly of if the minimum skirt length is zero or not. The code below
        // is left as a placeholder for when a multiextruder support is implemented. Then we will need to extrude the skirt loops for each extruder.
        //const PrintConfig &config = print.config();
        //if (config.min_skirt_length.value < EPSILON) {
        skirt_loops_per_extruder_out[(int) _currentExtruder /*layer_tools.extruders.front()*/] = std::pair<size_t, size_t>(0, n_loops);
        //} else {
        //    for (size_t i = 0; i < n_loops; i += lines_per_extruder)
        //        skirt_loops_per_extruder_out[layer_tools.extruders[i / lines_per_extruder]] = std::pair<size_t, size_t>(i, std::min(i + lines_per_extruder, n_loops));
        //}
    }

    static std::map<unsigned int, std::pair<size_t, size_t>> make_skirt_loops_per_extruder_1st_layer(
        const Print&                     print,
        const ExtrusionEntityCollection& skirt,
        const LayerTools&                layer_tools,
        // Heights (print_z) at which the skirt has already been extruded.
        std::vector<coordf_t>& skirt_done,
        unsigned int& extruder_id)
    {
        // Extrude skirt at the print_z of the raft layers and normal object layers
        // not at the print_z of the interlaced support material layers.
        std::map<unsigned int, std::pair<size_t, size_t>> skirt_loops_per_extruder_out;
        // For sequential print, the following test may fail when extruding the 2nd and other objects.
        // assert(skirt_done.empty());
        if (skirt_done.empty() && print.has_skirt() && !skirt.entities.empty() && layer_tools.has_skirt) {
            skirt_loops_per_extruder_all_printing(print, skirt, layer_tools, skirt_loops_per_extruder_out, extruder_id);
            skirt_done.emplace_back(layer_tools.print_z);
        }
        return skirt_loops_per_extruder_out;
    }

    static std::map<unsigned int, std::pair<size_t, size_t>> make_skirt_loops_per_extruder_other_layers(
        const Print&                     print,
        const ExtrusionEntityCollection& skirt,
        const LayerTools&                layer_tools,
        // Heights (print_z) at which the skirt has already been extruded.
        std::vector<coordf_t>& skirt_done,
        unsigned int& extruder_id)
    {
        // Extrude skirt at the print_z of the raft layers and normal object layers
        // not at the print_z of the interlaced support material layers.
        std::map<unsigned int, std::pair<size_t, size_t>> skirt_loops_per_extruder_out;
        if (print.has_skirt() && !skirt.entities.empty() && layer_tools.has_skirt &&
            // Not enough skirt layers printed yet.
            // FIXME infinite or high skirt does not make sense for sequential print!
            (skirt_done.size() < (size_t) print.config().skirt_height.value || print.has_infinite_skirt())) {
            bool valid = !skirt_done.empty() && skirt_done.back() < layer_tools.print_z - EPSILON;
            assert(valid);
            // This print_z has not been extruded yet (sequential print)
            // FIXME: The skirt_done should not be empty at this point. The check is a workaround
            if (valid) {
#if 0
                // Prime just the first printing extruder. This is original Slic3r's implementation.
                skirt_loops_per_extruder_out[layer_tools.extruders.front()] = std::pair<size_t, size_t>(0, print.config().skirt_loops.value);
#else
                // Prime all extruders planned for this layer, see
                skirt_loops_per_extruder_all_printing(print, skirt, layer_tools, skirt_loops_per_extruder_out,extruder_id);
#endif
                assert(!skirt_done.empty());
                skirt_done.emplace_back(layer_tools.print_z);
            }

            if (!valid && print.config().draft_shield == DraftShield::dsEnabled && print.config().skirt_type == SkirtType::stPerObject) {
                // Prime all extruders planned for this layer, see
                skirt_loops_per_extruder_all_printing(print, skirt, layer_tools, skirt_loops_per_extruder_out, extruder_id);
                assert(!skirt_done.empty());
                // skirt_done.emplace_back(layer_tools.print_z);
            }
        }
        return skirt_loops_per_extruder_out;
    }


    static Point find_start_point(ExtrusionLoop& loop, float start_angle)
    {
        coord_t min_x = std::numeric_limits<coord_t>::max();
        coord_t max_x = std::numeric_limits<coord_t>::min();
        coord_t min_y = min_x;
        coord_t max_y = max_x;

        Points pts;
        loop.collect_points(pts);
        for (Point pt : pts) {
            if (pt.x() < min_x)
                min_x = pt.x();
            else if (pt.x() > max_x)
                max_x = pt.x();
            if (pt.y() < min_y)
                min_y = pt.y();
            else if (pt.y() > max_y)
                max_y = pt.y();
        }

        Point  center((min_x + max_x) / 2., (min_y + max_y) / 2.);
        double r       = center.distance_to(Point(min_x, min_y));
        double deg     = start_angle * PI / 180;
        double shift_x = r * std::cos(deg);
        double shift_y = r * std::sin(deg);
        return Point(center.x() + shift_x, center.y() + shift_y);
    }

 } // namespace Skirt

// Orca: Klipper can't parse object names with spaces and other spetical characters
std::string sanitize_instance_name(const std::string& name) {
    // Replace sequences of non-word characters with an underscore
    std::string result = std::regex_replace(name, std::regex("[ !@#$%^&*()=+\\[\\]{};:\",']+"), "_");
    // Remove leading and trailing underscores
    if (!result.empty() && result.front() == '_') {
        result.erase(result.begin());
    }
    if (!result.empty() && result.back() == '_') {
        result.erase(result.end() - 1);
    }

    return result;
}

inline std::string get_instance_name(const PrintObject *object, size_t inst_id) {
    auto obj_name = sanitize_instance_name(object->model_object()->name);
    auto name = (boost::format("%1%_id_%2%_copy_%3%") % obj_name % object->get_id() % inst_id).str();
    return sanitize_instance_name(name);
}

inline std::string get_instance_name(const PrintObject *object, const PrintInstance &inst) {
    return get_instance_name(object, inst.id);
}

std::string GCode::generate_skirt(const Print&                     print,
                                  const ExtrusionEntityCollection& skirt,
                                  const Point&                     offset,
                                  const LayerTools&                layer_tools,
                                  const Layer&                     layer,
                                  unsigned int                     extruder_id,
                                  const PrintObject*               object_for_brim)
{
    bool        first_layer = (layer.id() == 0 && abs(layer.bottom_z()) < EPSILON);
    std::string gcode;
    // Under draft shield, trim first-layer skirt where it overlaps brim so we do not double-extrude.
    // For per-object skirt trim against this object's brim; for combined skirt trim against all brims.
    const bool  trim_first_layer = first_layer &&
        print.config().draft_shield != DraftShield::dsDisabled;
    Polygons    brim_polys;
    if (trim_first_layer) {
        auto append_brim_polys = [&brim_polys](const ExtrusionEntityCollection& brim) {
            brim.polygons_covered_by_width(brim_polys, float(SCALED_EPSILON));
        };
        if (object_for_brim != nullptr) {
            if (auto it = print.m_brimMap.find(object_for_brim->id()); it != print.m_brimMap.end() && !it->second.empty())
                append_brim_polys(it->second);
            if (auto it = print.m_supportBrimMap.find(object_for_brim->id()); it != print.m_supportBrimMap.end() && !it->second.empty())
                append_brim_polys(it->second);
        } else {
            for (const auto &it : print.m_brimMap)
                if (!it.second.empty())
                    append_brim_polys(it.second);
            for (const auto &it : print.m_supportBrimMap)
                if (!it.second.empty())
                    append_brim_polys(it.second);
        }
        if (!brim_polys.empty()) {
            ExPolygons brim_clip_ex = union_ex(brim_polys);
            for (ExPolygon &ex : brim_clip_ex)
                ex.holes.clear();
            const float clip_margin = 0.25f * float(std::max(print.brim_flow().scaled_spacing(), print.skirt_flow().scaled_spacing()));
            if (clip_margin > 0.f)
                brim_clip_ex = offset_ex(brim_clip_ex, clip_margin, jtRound, SCALED_RESOLUTION);
            brim_polys = to_polygons(brim_clip_ex);

            for (Polygon &poly : brim_polys)
                poly.translate(-offset.x(), -offset.y());
        }
    }
    // Extrude skirt at the print_z of the raft layers and normal object layers
    // not at the print_z of the interlaced support material layers.
    // Map from extruder ID to <begin, end> index of skirt loops to be extruded with that extruder.
    std::map<unsigned int, std::pair<size_t, size_t>> skirt_loops_per_extruder;
    skirt_loops_per_extruder = first_layer ? Skirt::make_skirt_loops_per_extruder_1st_layer(print, skirt, layer_tools, m_skirt_done,extruder_id) :
                                   Skirt::make_skirt_loops_per_extruder_other_layers(print, skirt, layer_tools, m_skirt_done, extruder_id);

    if (auto loops_it = skirt_loops_per_extruder.find(extruder_id); loops_it != skirt_loops_per_extruder.end()) {
        const std::pair<size_t, size_t> loops = loops_it->second;

        set_origin(unscaled(offset));

        m_avoid_crossing_perimeters.use_external_mp();
        Flow layer_skirt_flow = print.skirt_flow().with_height(
            float(m_skirt_done.back() - (m_skirt_done.size() == 1 ? 0. : m_skirt_done[m_skirt_done.size() - 2])));
        double mm3_per_mm = layer_skirt_flow.mm3_per_mm();
        for (size_t i = first_layer ? loops.first : loops.second - 1; i < loops.second; ++i) {
            // Adjust flow according to this layer's layer height.
            ExtrusionLoop loop = *dynamic_cast<const ExtrusionLoop*>(skirt.entities[i]);
            for (ExtrusionPath& path : loop.paths) {
                path.height     = layer_skirt_flow.height();
                path.mm3_per_mm = mm3_per_mm;
            }

            //// set skirt start point location
            //if (first_layer && i == loops.first)
            //    this->set_last_pos(Skirt::find_start_point(loop, layer.object()->config().skirt_start_angle));

            // FIXME using the support_speed of the 1st object printed.
            if (trim_first_layer && !brim_polys.empty()) {
                Polylines clipped = diff_pl(loop.as_polyline(), brim_polys);
                for (Polyline &polyline : clipped) {
                    if (polyline.size() < 2)
                        continue;
                    ExtrusionPath path = loop.paths.front();
                    path.polyline = std::move(polyline);
                    gcode += this->extrude_path(std::move(path), "skirt", m_config.support_speed.value);
                }
            } else {
                gcode += this->extrude_loop(loop, "skirt", m_config.support_speed.value);
            }
            if (!first_layer)
                break;
        }
        m_avoid_crossing_perimeters.use_external_mp(false);
        // Allow a straight travel move to the first object point if this is the first layer (but don't in next layers).
        if (first_layer && loops.first == 0)
            m_avoid_crossing_perimeters.disable_once();
    }
    return gcode;
}


// In sequential mode, process_layer is called once per each object and its copy,
// therefore layers will contain a single entry and single_object_instance_idx will point to the copy of the object.
// In non-sequential mode, process_layer is called per each print_z height with all object and support layers accumulated.
// For multi-material prints, this routine minimizes extruder switches by gathering extruder specific extrusion paths
// and performing the extruder specific extrusions together.
LayerResult GCode::process_layer(
    const Print                    			&print,
    // Set of object & print layers of the same PrintObject and with the same print_z.
    const std::vector<LayerToPrint> 		&layers,
    const LayerTools        		        &layer_tools,
    const bool                               last_layer,
    // Pairs of PrintObject index and its instance index.
    const std::vector<const PrintInstance*> *ordering,
    // If set to size_t(-1), then print all copies of all objects.
    // Otherwise print a single copy of a single object.
    const size_t                     		 single_object_instance_idx,
    // BBS
    const bool                               prime_extruder)
{
    assert(! layers.empty());
    // Either printing all copies of all objects, or just a single copy of a single object.
    assert(single_object_instance_idx == size_t(-1) || layers.size() == 1);

    // First object, support and raft layer, if available.
    const Layer         *object_layer  = nullptr;
    const SupportLayer  *support_layer = nullptr;
    const SupportLayer  *raft_layer    = nullptr;
    for (const LayerToPrint &l : layers) {
        if (l.object_layer && ! object_layer)
            object_layer = l.object_layer;
        if (l.support_layer) {
            if (! support_layer)
                support_layer = l.support_layer;
            if (! raft_layer && support_layer->id() < support_layer->object()->slicing_parameters().raft_layers())
                raft_layer = support_layer;
        }
    }

    const Layer* layer_ptr = nullptr;
    if (object_layer != nullptr)
        layer_ptr = object_layer;
    else if (support_layer != nullptr)
        layer_ptr = support_layer;
    const Layer& layer = *layer_ptr;
    LayerResult   result { {}, layer.id(), false, last_layer };
    if (layer_tools.extruders.empty())
        // Nothing to extrude.
        return result;

    // Extract 1st object_layer and support_layer of this set of layers with an equal print_z.
    coordf_t             print_z       = layer.print_z;
    //BBS: using layer id to judge whether the layer is first layer is wrong. Because if the normal
    //support is attached above the object, and support layers has independent layer height, then the lowest support
    //interface layer id is 0.
    bool                 first_layer   = (layer.id() == 0 && abs(layer.bottom_z()) < EPSILON);
    m_writer.set_is_first_layer(first_layer);
    unsigned int         first_extruder_id = layer_tools.extruders.front();
    auto current_extruder_or_first = [this, first_extruder_id]() -> unsigned int {
        return m_writer.extruder() != nullptr ? m_writer.extruder()->id() : first_extruder_id;
    };

    // Initialize config with the 1st object to be printed at this layer.
    m_config.apply(layer.object()->config(), true);

    // Check whether it is possible to apply the spiral vase logic for this layer.
    // Just a reminder: A spiral vase mode is allowed for a single object, single material print only.
    m_enable_loop_clipping = true;
    if (m_spiral_vase && layers.size() == 1 && support_layer == nullptr) {
        bool enable = (layer.id() > 0 || !print.has_brim()) && (layer.id() >= (size_t)print.config().skirt_height.value && ! print.has_infinite_skirt());
        if (enable) {
            for (const LayerRegion *layer_region : layer.regions())
                if (size_t(layer_region->region().config().bottom_shell_layers.value) > layer.id() ||
                    layer_region->perimeters.items_count() > 1u ||
                    layer_region->fills.items_count() > 0) {
                    enable = false;
                    break;
                }
        }
        result.spiral_vase_enable = enable;
        // If we're going to apply spiralvase to this layer, disable loop clipping.
        m_enable_loop_clipping = !enable;
    }

    std::string gcode;
    assert(is_decimal_separator_point()); // for the sprintfs

    if (m_wipe_tower)
        m_wipe_tower->set_creality_cfs(print.getCrealityCFS());


    int max_chamber_temp = 0;
    int max_extruder_id            = -1;
    for (const auto& extruder : m_writer.extruders()) {
        int current_value = m_config.chamber_temperature.get_at(extruder.id());
        // ?????????????,?????????ID
        if (current_value > max_chamber_temp) {
            max_chamber_temp = current_value;
            max_extruder_id            = extruder.id();
        }
    }
    int max_activate_chamber_layer = m_config.activate_chamber_layer.get_at(max_extruder_id);


    if (max_activate_chamber_layer > 1 && max_activate_chamber_layer == m_layer_index + 2)
    {
        bool activate_chamber_temp_control = false;
        auto max_chamber_temp = 0;
        for (const auto& extruder : m_writer.extruders()) {
            activate_chamber_temp_control |= m_config.activate_chamber_temp_control.get_at(extruder.id());
            max_chamber_temp = std::max(max_chamber_temp, m_config.chamber_temperature.get_at(extruder.id()));
        }

        //
        if (activate_chamber_temp_control)
        {
            if (print.is_CX_printer())
                gcode += m_writer.set_chamber_temperature(max_chamber_temp, false); // for creality
            else
                gcode +=m_writer.set_chamber_temperature(max_chamber_temp, true); // set chamber_temperature
        }
    }


    // add tag for processor
    gcode += ";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Layer_Change) + "\n";
    // export layer z
    char buf[64];
    sprintf(buf, print.is_BBL_printer() ? "; Z_HEIGHT: %g\n" : ";Z:%g\n", print_z);
    gcode += buf;
    // export layer height
    float height = first_layer ? static_cast<float>(print_z) : static_cast<float>(print_z) - m_last_layer_z;
    sprintf(buf, ";%s%g\n", GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Height).c_str(), height);
    gcode += buf;
    // update caches
    m_last_layer_z = static_cast<float>(print_z);
    m_max_layer_z  = std::max(m_max_layer_z, m_last_layer_z);
    m_last_height = height;

    // Set new layer - this will change Z and force a retraction if retract_when_changing_layer is enabled.
    if (! m_config.before_layer_change_gcode.value.empty()) {
        DynamicConfig config;
        config.set_key_value("layer_num",   new ConfigOptionInt(m_layer_index + 1));
        config.set_key_value("layer_z",     new ConfigOptionFloat(print_z));
        config.set_key_value("max_layer_z", new ConfigOptionFloat(m_max_layer_z));
        gcode += this->placeholder_parser_process("before_layer_change_gcode",
            print.config().before_layer_change_gcode.value, current_extruder_or_first(), &config)
            + "\n";
    }

    PrinterStructure printer_structure           = m_config.printer_structure.value;
    bool need_insert_timelapse_gcode_for_traditional = false;
    if (printer_structure == PrinterStructure::psI3 &&
        !m_spiral_vase &&
        (!m_wipe_tower || !m_wipe_tower->enable_timelapse_print()) &&
        print.config().print_sequence == PrintSequence::ByLayer) {
        need_insert_timelapse_gcode_for_traditional = true;
    }
    bool has_insert_timelapse_gcode = false;
    bool has_wipe_tower             = (layer_tools.has_wipe_tower && m_wipe_tower);

    auto insert_timelapse_gcode = [this, print_z, &print, &current_extruder_or_first]() -> std::string {
        std::string gcode_res;
        if (!m_config.time_lapse_gcode.value.empty() && m_config.timelapse_type != TimelapseType::tlClose) {
            DynamicConfig config;
            config.set_key_value("layer_num", new ConfigOptionInt(m_layer_index));
            config.set_key_value("layer_z", new ConfigOptionFloat(print_z));
            config.set_key_value("max_layer_z", new ConfigOptionFloat(m_max_layer_z));
            gcode_res = this->placeholder_parser_process("timelapse_gcode", print.config().time_lapse_gcode.value, current_extruder_or_first(), &config) + "\n";
        }
        return gcode_res;
    };

    // BBS: don't use lazy_raise when enable spiral vase
    gcode += this->change_layer(print_z);  // this will increase m_layer_index

    m_layer = &layer;
    m_object_layer_over_raft = false;
    if(is_BBL_Printer()){
        if (printer_structure == PrinterStructure::psI3 && !need_insert_timelapse_gcode_for_traditional && !m_spiral_vase && print.config().print_sequence == PrintSequence::ByLayer) {
            std::string timepals_gcode = insert_timelapse_gcode();
            gcode += timepals_gcode;
            m_writer.set_current_position_clear(false);
            //BBS: check whether custom gcode changes the z position. Update if changed
            double temp_z_after_timepals_gcode;
            if (GCodeProcessor::get_last_z_from_gcode(timepals_gcode, temp_z_after_timepals_gcode)) {
                Vec3d pos = m_writer.get_position();
                pos(2) = temp_z_after_timepals_gcode;
                m_writer.set_position(pos);
            }
        }
    } else {
        if (!m_config.time_lapse_gcode.value.empty() && !m_config.enable_prime_tower && m_config.timelapse_type != TimelapseType::tlClose) {
            DynamicConfig config;
            config.set_key_value("layer_num", new ConfigOptionInt(m_layer_index));
            config.set_key_value("layer_z", new ConfigOptionFloat(print_z));
            config.set_key_value("max_layer_z", new ConfigOptionFloat(m_max_layer_z));
            config.set_key_value("enable_prime_tower", new ConfigOptionBool(m_config.enable_prime_tower));
            gcode += this->placeholder_parser_process("timelapse_gcode", print.config().time_lapse_gcode.value, current_extruder_or_first(),
                                                      &config) +
                     "\n";
        }
    }
    if (! m_config.layer_change_gcode.value.empty()) {
        DynamicConfig config;
        config.set_key_value("layer_num", new ConfigOptionInt(m_layer_index));
        config.set_key_value("layer_z",   new ConfigOptionFloat(print_z));
        gcode += this->placeholder_parser_process("layer_change_gcode",
            print.config().layer_change_gcode.value, current_extruder_or_first(), &config)
            + "\n";
        config.set_key_value("max_layer_z", new ConfigOptionFloat(m_max_layer_z));
    }
    //BBS: set layer time fan speed after layer change gcode
    gcode += ";_SET_FAN_SPEED_CHANGING_LAYER\n";

    m_writer.set_first_layer(this->on_first_layer());

    if (print.calib_mode() == CalibMode::Calib_PA_Tower) {
        gcode += writer().set_pressure_advance(print.calib_params().start + static_cast<int>(print_z) * print.calib_params().step);
    } else if (print.calib_mode() == CalibMode::Calib_Temp_Tower) {
        auto offset = static_cast<unsigned int>(print_z / 10.001) * 5;
        gcode += writer().set_temperature(print.calib_params().start - offset);
    } else if (print.calib_mode() == CalibMode::Calib_VFA_Tower) {
        auto _speed = print.calib_params().start + std::floor(print_z / 5.0) * print.calib_params().step;
        m_calib_config.set_key_value("outer_wall_speed", new ConfigOptionFloat(std::round(_speed)));
    } else if (print.calib_mode() == CalibMode::Calib_Vol_speed_Tower) {
        auto _speed = print.calib_params().start + print_z * print.calib_params().step;
        m_calib_config.set_key_value("outer_wall_speed", new ConfigOptionFloat(std::round(_speed)));
    }else if (print.calib_mode() == CalibMode::Calib_Retraction_tower) {
        auto _length = print.calib_params().start + std::floor(std::max(0.0, print_z - 0.2 + 0.001 -0.4)) * print.calib_params().step;
        DynamicConfig _cfg;
        _cfg.set_key_value("retraction_length", new ConfigOptionFloats{_length});
        writer().config.apply(_cfg);
        sprintf(buf, "; Calib_Retraction_tower: Z_HEIGHT: %g, length:%g\n", print_z, _length);
        gcode += buf;
    } else if (print.calib_mode() == CalibMode::Calib_Retraction_tower_speed) {
        auto _speed = print.calib_params().start + std::floor(std::max(0.0, print_z - 0.2 + 0.001 - 0.4)) * print.calib_params().step;
        DynamicConfig _cfg;
        _cfg.set_key_value("retraction_speed", new ConfigOptionFloats{_speed});
        _cfg.set_key_value("deretraction_speed", new ConfigOptionFloats{_speed});
        writer().config.apply(_cfg);
        sprintf(buf, "; Calib_Retraction_tower: Z_HEIGHT: %g, speed:%g\n", print_z, _speed);
        gcode += buf;
    } else if (print.calib_mode() == CalibMode::Calib_Limit_Speed || print.calib_mode() == CalibMode::Calib_Speed_Tower) {
        auto _speed = print.calib_params().start + std::floor(print_z / 5.0) * print.calib_params().step;
        m_calib_config.set_key_value("inner_wall_speed", new ConfigOptionFloat(std::round(_speed)));
        m_calib_config.set_key_value("outer_wall_speed", new ConfigOptionFloat(std::round(_speed)));
        m_calib_config.set_key_value("sparse_infill_speed", new ConfigOptionFloat(std::round(_speed)));
        m_calib_config.set_key_value("gap_infill_speed", new ConfigOptionFloat(std::round(_speed)));
    } else if (print.calib_mode() == CalibMode::Calib_X_Y_Jerk) {
        auto _speed = print.calib_params().start + std::floor(print_z / 5.0) * print.calib_params().step;
        m_calib_config.set_key_value("outer_wall_jerk", new ConfigOptionFloat(std::round(_speed)));
        m_calib_config.set_key_value("inner_wall_jerk", new ConfigOptionFloat(std::round(_speed)));
        m_calib_config.set_key_value("top_surface_jerk", new ConfigOptionFloat(std::round(_speed)));
        m_calib_config.set_key_value("infill_jerk", new ConfigOptionFloat(std::round(_speed)));
    } else if (print.calib_mode() == CalibMode::Calib_Fan_Speed) {
        int _speed = print.calib_params().start + std::floor(print_z / 7.8) * print.calib_params().step;
        m_calib_config.set_key_value("fan_min_speed", new ConfigOptionFloats(2, std::min(_speed,100)));
    } else if (print.calib_mode() == CalibMode::Calib_Limit_Acceleration || print.calib_mode() == CalibMode::Calib_Acceleration_Tower) {
        auto _speed = print.calib_params().start + std::floor(print_z / 5.0) * print.calib_params().step;
        m_calib_config.set_key_value("inner_wall_acceleration", new ConfigOptionFloat(std::round(_speed)));
        m_calib_config.set_key_value("outer_wall_acceleration", new ConfigOptionFloat(std::round(_speed)));
        m_calib_config.set_key_value("default_acceleration", new ConfigOptionFloat(std::round(_speed)));
        m_calib_config.set_key_value("sparse_infill_acceleration", new ConfigOptionFloatOrPercent(100, true));
    } else if (print.calib_mode() == CalibMode::Calib_Accel2Decel) {
        int _speed = print.calib_params().start - std::floor(print_z / print.calib_params().high_step) * print.calib_params().step;
        m_calib_config.set_key_value("accel_to_decel_factor", new ConfigOptionPercent(std::max(_speed, (int) print.calib_params().end)));
    }

    //BBS
    if (first_layer) {
        // Orca: we don't need to optimize the Klipper as only set once
        if (m_config.default_acceleration.value > 0 && m_config.initial_layer_acceleration.value > 0) {
            gcode += m_writer.set_print_acceleration((unsigned int)floor(m_config.initial_layer_acceleration.value + 0.5));
        }

        if (m_config.default_jerk.value > 0 && m_config.initial_layer_jerk.value > 0) {
            gcode += m_writer.set_jerk_xy(m_config.initial_layer_jerk.value);
        }

    }

    if (! first_layer && ! m_second_layer_things_done) {
      if (print.is_BBL_printer()) {
        // BBS: open powerlost recovery
        {
          gcode += "; open powerlost recovery\n";
          gcode += "M1003 S1\n";
        }
        // BBS: open first layer inspection at second layer
        if (print.config().scan_first_layer.value) {
          // BBS: retract first to avoid droping when scan model
          gcode += this->retract();
          gcode += "M976 S1 P1 ; scan model before printing 2nd layer\n";
          gcode += "M400 P100\n";
          gcode += this->unretract();
        }
      }
      // Reset acceleration at sencond layer
      // Orca: only set once, don't need to call set_accel_and_jerk
      if (m_config.default_acceleration.value > 0 && m_config.initial_layer_acceleration.value > 0) {
        gcode += m_writer.set_print_acceleration((unsigned int) floor(m_config.default_acceleration.value + 0.5));
      }

      if (m_config.default_jerk.value > 0 && m_config.initial_layer_jerk.value > 0) {
        gcode += m_writer.set_jerk_xy(m_config.default_jerk.value);
      }

        // Transition from 1st to 2nd layer. Adjust nozzle temperatures as prescribed by the nozzle dependent
        // nozzle_temperature_initial_layer vs. temperature settings.
        for (const Extruder &extruder : m_writer.extruders()) {
          if ((print.config().single_extruder_multi_material.value || m_ooze_prevention.enable) && extruder.id() != current_extruder_or_first())
                // In single extruder multi material mode, set the temperature for the current extruder only.
                continue;
            int temperature = print.config().nozzle_temperature.get_at(extruder.id());
            if (temperature > 0 && temperature != print.config().nozzle_temperature_initial_layer.get_at(extruder.id()))
            {
                if (print.config().material_flow_dependent_temperature.get_at(extruder.id()) && !print.getMultiColor()) {
                    m_temperature = temperature;
                }
                gcode += m_writer.set_temperature(temperature, false, extruder.id());
            }
        }

        // BBS
        int bed_temp = get_bed_temperature(first_extruder_id, false, print.config().curr_bed_type);
        gcode += m_writer.set_bed_temperature(bed_temp);
        // Mark the temperature transition from 1st to 2nd layer to be finished.
        m_second_layer_things_done = true;
    }

    // Map from extruder ID to <begin, end> index of skirt loops to be extruded with that extruder.
    std::map<unsigned int, std::pair<size_t, size_t>> skirt_loops_per_extruder;

    if (single_object_instance_idx == size_t(-1)) {
        // Normal (non-sequential) print.
        gcode += ProcessLayer::emit_custom_gcode_per_print_z(*this, layer_tools.custom_gcode, current_extruder_or_first(), first_extruder_id, print.config());
    }


    // BBS: get next extruder according to flush and soluble
    auto get_next_extruder = [&](int current_extruder,const std::vector<unsigned int>&extruders) {
        const auto& flush_matrix = m_config.flush_volumes_matrix.values;
        const unsigned int number_of_extruders = (unsigned int)(sqrt(flush_matrix.size()) + EPSILON);
        // Extract purging volumes for each extruder pair:
        std::vector<std::vector<float>> wipe_volumes;
        for (unsigned int i = 0; i < number_of_extruders; ++i)
            wipe_volumes.push_back(std::vector<float>(flush_matrix.begin() + i * number_of_extruders, flush_matrix.begin() + (i + 1) * number_of_extruders));
        unsigned int next_extruder = current_extruder;
        float min_flush = std::numeric_limits<float>::max();
        for (auto extruder_id : extruders) {
            if (print.config().filament_soluble.get_at(extruder_id) || extruder_id == current_extruder)
                continue;
            if (wipe_volumes[current_extruder][extruder_id] < min_flush) {
                next_extruder = extruder_id;
                min_flush = wipe_volumes[current_extruder][extruder_id];
            }
        }
        return next_extruder;
    };

    for (const auto& layer_to_print : layers) {
        if (layer_to_print.object_layer) {
            const auto& regions               = layer_to_print.object_layer->regions();
            const bool  enable_overhang_speed = std::any_of(regions.begin(), regions.end(), [](const LayerRegion* r) {
                return r->has_extrusions() && r->region().config().enable_overhang_speed;
            });
            if (enable_overhang_speed) {
                m_extrusion_quality_estimator.prepare_for_new_layer(layer_to_print.original_object, layer_to_print.object_layer);
            }
        }
    }

    // Group extrusions by an extruder, then by an object, an island and a region.
    std::map<unsigned int, std::vector<ObjectByExtruder>> by_extruder;
    bool is_anything_overridden = const_cast<LayerTools&>(layer_tools).wiping_extrusions().is_anything_overridden();
    for (const LayerToPrint &layer_to_print : layers) {
        if (layer_to_print.support_layer != nullptr) {
            const SupportLayer &support_layer = *layer_to_print.support_layer;
            const PrintObject& object = *layer_to_print.original_object;
            if (! support_layer.support_fills.entities.empty()) {
                ExtrusionRole   role               = support_layer.support_fills.role();
                bool            has_support        = role == erMixed || role == erSupportMaterial || role == erSupportTransition;
                bool            has_interface      = role == erMixed || role == erSupportMaterialInterface;
                // Extruder ID of the support base. -1 if "don't care".
                unsigned int    support_extruder   = object.config().support_filament.value - 1;
                // Shall the support be printed with the active extruder, preferably with non-soluble, to avoid tool changes?
                bool            support_dontcare   = object.config().support_filament.value == 0;
                // Extruder ID of the support interface. -1 if "don't care".
                unsigned int    interface_extruder = object.config().support_interface_filament.value - 1;
                // Shall the support interface be printed with the active extruder, preferably with non-soluble, to avoid tool changes?
                bool            interface_dontcare = object.config().support_interface_filament.value == 0;

                // BBS: apply wiping overridden extruders
                WipingExtrusions& wiping_extrusions = const_cast<LayerTools&>(layer_tools).wiping_extrusions();
                if (support_dontcare) {
                    int extruder_override = wiping_extrusions.get_support_extruder_overrides(&object);
                    if (extruder_override >= 0) {
                        support_extruder = extruder_override;
                        support_dontcare = false;
                    }
                }

                if (interface_dontcare) {
                    int extruder_override = wiping_extrusions.get_support_interface_extruder_overrides(&object);
                    if (extruder_override >= 0) {
                        interface_extruder = extruder_override;
                        interface_dontcare = false;
                    }
                }

                // BBS: try to print support base with a filament other than interface filament
                if (support_dontcare && !interface_dontcare) {
                    unsigned int dontcare_extruder = first_extruder_id;
                    for (unsigned int extruder_id : layer_tools.extruders) {
                        if (print.config().filament_soluble.get_at(extruder_id))
                            continue;

                        //BBS: now we don't consider interface filament used in other object
                        if (extruder_id == interface_extruder && object.config().support_interface_not_for_body)
                            continue;

                        dontcare_extruder = extruder_id;
                        break;
                    }
                #if 0
                    //BBS: not found a suitable extruder in current layer ,dontcare_extruider==first_extruder_id==interface_extruder
                    if (dontcare_extruder == interface_extruder && (object.config().support_interface_not_for_body && object.config().support_interface_filament.value!=0)) {
                        // BBS : get a suitable extruder from other layer
                        auto all_extruders = print.extruders();
                        dontcare_extruder = get_next_extruder(dontcare_extruder, all_extruders);
                    }
                #endif

                    if (support_dontcare)
                        support_extruder = dontcare_extruder;
                }
                else if (support_dontcare || interface_dontcare) {
                    // Some support will be printed with "don't care" material, preferably non-soluble.
                    // Is the current extruder assigned a soluble filament?
                    unsigned int dontcare_extruder = first_extruder_id;
                    if (print.config().filament_soluble.get_at(dontcare_extruder)) {
                        // The last extruder printed on the previous layer extrudes soluble filament.
                        // Try to find a non-soluble extruder on the same layer.
                        for (unsigned int extruder_id : layer_tools.extruders)
                            if (! print.config().filament_soluble.get_at(extruder_id)) {
                                dontcare_extruder = extruder_id;
                                break;
                            }
                    }
                    if (support_dontcare)
                        support_extruder = dontcare_extruder;
                    if (interface_dontcare)
                        interface_extruder = dontcare_extruder;
                }
                // Both the support and the support interface are printed with the same extruder, therefore
                // the interface may be interleaved with the support base.
                bool single_extruder = ! has_support || support_extruder == interface_extruder;
                // Assign an extruder to the base.
                ObjectByExtruder &obj = object_by_extruder(by_extruder, has_support ? support_extruder : interface_extruder, &layer_to_print - layers.data(), layers.size());
                obj.support = &support_layer.support_fills;
                obj.support_extrusion_role = single_extruder ? erMixed : erSupportMaterial;
                if (! single_extruder && has_interface) {
                    ObjectByExtruder &obj_interface = object_by_extruder(by_extruder, interface_extruder, &layer_to_print - layers.data(), layers.size());
                    obj_interface.support = &support_layer.support_fills;
                    obj_interface.support_extrusion_role = erSupportMaterialInterface;

                    if (object.config().ironing_support_layer && !object.config().support_interface_not_for_body)
                        obj_interface.support_extrusion_role = erMixed;
                }
            }
        }

        if (layer_to_print.object_layer != nullptr) {
            const Layer &layer = *layer_to_print.object_layer;
            // We now define a strategy for building perimeters and fills. The separation
            // between regions doesn't matter in terms of printing order, as we follow
            // another logic instead:
            // - we group all extrusions by extruder so that we minimize toolchanges
            // - we start from the last used extruder
            // - for each extruder, we group extrusions by island
            // - for each island, we extrude perimeters first, unless user set the infill_first
            //   option
            // (Still, we have to keep track of regions because we need to apply their config)
            size_t n_slices = layer.lslices.size();
            const std::vector<BoundingBox> &layer_surface_bboxes = layer.lslices_bboxes;
            // Traverse the slices in an increasing order of bounding box size, so that the islands inside another islands are tested first,
            // so we can just test a point inside ExPolygon::contour and we may skip testing the holes.
            std::vector<size_t> slices_test_order;
            slices_test_order.reserve(n_slices);
            for (size_t i = 0; i < n_slices; ++ i)
                slices_test_order.emplace_back(i);
            std::sort(slices_test_order.begin(), slices_test_order.end(), [&layer_surface_bboxes](size_t i, size_t j) {
                const Vec2d s1 = layer_surface_bboxes[i].size().cast<double>();
                const Vec2d s2 = layer_surface_bboxes[j].size().cast<double>();
                return s1.x() * s1.y() < s2.x() * s2.y();
            });
            auto point_inside_surface = [&layer, &layer_surface_bboxes](const size_t i, const Point &point) {
                const BoundingBox &bbox = layer_surface_bboxes[i];
                return point(0) >= bbox.min(0) && point(0) < bbox.max(0) &&
                       point(1) >= bbox.min(1) && point(1) < bbox.max(1) &&
                       layer.lslices[i].contour.contains(point);
            };

            for (size_t region_id = 0; region_id < layer.regions().size(); ++ region_id) {
                const LayerRegion *layerm = layer.regions()[region_id];
                if (layerm == nullptr)
                    continue;
                // PrintObjects own the PrintRegions, thus the pointer to PrintRegion would be unique to a PrintObject, they would not
                // identify the content of PrintRegion accross the whole print uniquely. Translate to a Print specific PrintRegion.
                const PrintRegion &region = print.get_print_region(layerm->region().print_region_id());

                // Now we must process perimeters and infills and create islands of extrusions in by_region std::map.
                // It is also necessary to save which extrusions are part of MM wiping and which are not.
                // The process is almost the same for perimeters and infills - we will do it in a cycle that repeats twice:
                std::vector<unsigned int> printing_extruders;
                for (const ObjectByExtruder::Island::Region::Type entity_type : { ObjectByExtruder::Island::Region::INFILL, ObjectByExtruder::Island::Region::PERIMETERS }) {
                    bool is_infill = entity_type == ObjectByExtruder::Island::Region::INFILL;
                    for (const ExtrusionEntity *ee : is_infill ? layerm->fills.entities : layerm->perimeters.entities) {
                        // extrusions represents infill or perimeter extrusions of a single island.
                        assert(dynamic_cast<const ExtrusionEntityCollection*>(ee) != nullptr);
                        const auto *extrusions = static_cast<const ExtrusionEntityCollection*>(ee);
                        if (extrusions->entities.empty()) // This shouldn't happen but first_point() would fail.
                            continue;

                        // This extrusion is part of certain Region, which tells us which extruder should be used for it:
                        int correct_extruder_id = layer_tools.extruder(*extrusions, region);

                        // Let's recover vector of extruder overrides:
                        const WipingExtrusions::ExtruderPerCopy *entity_overrides = nullptr;
                        if (! layer_tools.has_extruder(correct_extruder_id)) {
                            // this entity is not overridden, but its extruder is not in layer_tools - we'll print it
                            // by last extruder on this layer (could happen e.g. when a wiping object is taller than others - dontcare extruders are eradicated from layer_tools)
                            correct_extruder_id = layer_tools.extruders.back();
                        }
                        printing_extruders.clear();
                        if (is_anything_overridden) {
                            entity_overrides = const_cast<LayerTools&>(layer_tools).wiping_extrusions().get_extruder_overrides(extrusions, layer_to_print.original_object, correct_extruder_id, layer_to_print.object()->instances().size());
                            if (entity_overrides == nullptr) {
                                printing_extruders.emplace_back(correct_extruder_id);
                            } else {
                                printing_extruders.reserve(entity_overrides->size());
                                for (int extruder : *entity_overrides)
                                    printing_extruders.emplace_back(extruder >= 0 ?
                                        // at least one copy is overridden to use this extruder
                                        extruder :
                                        // at least one copy would normally be printed with this extruder (see get_extruder_overrides function for explanation)
                                        static_cast<unsigned int>(- extruder - 1));
                                Slic3r::sort_remove_duplicates(printing_extruders);
                            }
                        } else
                            printing_extruders.emplace_back(correct_extruder_id);

                        // Now we must add this extrusion into the by_extruder map, once for each extruder that will print it:
                        for (unsigned int extruder : printing_extruders)
                        {
                            std::vector<ObjectByExtruder::Island> &islands = object_islands_by_extruder(
                                by_extruder,
                                extruder,
                                &layer_to_print - layers.data(),
                                layers.size(), n_slices+1);
                            for (size_t i = 0; i <= n_slices; ++ i) {
                                bool   last = i == n_slices;
                                size_t island_idx = last ? n_slices : slices_test_order[i];
                                if (// extrusions->first_point does not fit inside any slice
                                    last ||
                                    // extrusions->first_point fits inside ith slice
                                    point_inside_surface(island_idx, extrusions->first_point())) {
                                    if (islands[island_idx].by_region.empty())
                                        islands[island_idx].by_region.assign(print.num_print_regions(), ObjectByExtruder::Island::Region());
                                    islands[island_idx].by_region[region.print_region_id()].append(entity_type, extrusions, entity_overrides);
                                    break;
                                }
                            }
                        }
                    }
                }
            } // for regions
        }
    } // for objects

    if (m_wipe_tower)
        m_wipe_tower->set_is_first_print(true);

    // Extrude the skirt, brim, support, perimeters, infill ordered by the extruders.
    unsigned int skip_wipe_tower_toolchange_extruder = (unsigned int)-1;
    for (size_t layer_extruder_idx = 0; layer_extruder_idx < layer_tools.extruders.size(); ++layer_extruder_idx)
    {
        const unsigned int extruder_id = layer_tools.extruders[layer_extruder_idx];
        const unsigned int next_extruder_id = layer_extruder_idx + 1 < layer_tools.extruders.size() ?
            layer_tools.extruders[layer_extruder_idx + 1] : (unsigned int)-1;
        // Same-layer successor only; do not carry skin across a layer boundary.
        auto objects_by_extruder_it = by_extruder.find(extruder_id);

        // BBS: ordering instances by extruder
        std::vector<InstanceToPrint> instances_to_print;
        if (objects_by_extruder_it != by_extruder.end()) {
            bool has_prime_tower = print.config().enable_prime_tower
                && print.extruders().size() > 1
                && ((print.config().print_sequence == PrintSequence::ByLayer && print.config().print_order == PrintOrder::Default)
                    || (print.config().print_sequence == PrintSequence::ByObject && print.objects().size() == 1));
            if (has_prime_tower) {
                int plate_idx = print.get_plate_index();
                Point wt_pos(print.config().wipe_tower_x.get_at(plate_idx), print.config().wipe_tower_y.get_at(plate_idx));

                std::vector<GCode::ObjectByExtruder>& objects_by_extruder = objects_by_extruder_it->second;
                std::vector<const PrintObject*> print_objects;
                for (int obj_idx = 0; obj_idx < objects_by_extruder.size(); obj_idx++) {
                    auto& object_by_extruder = objects_by_extruder[obj_idx];
                    if (object_by_extruder.islands.empty() && (object_by_extruder.support == nullptr || object_by_extruder.support->empty()))
                        continue;

                    print_objects.push_back(print.get_object(obj_idx));
                }

                std::vector<const PrintInstance*> new_ordering = chain_print_object_instances(print_objects, &wt_pos);
                std::reverse(new_ordering.begin(), new_ordering.end());
                instances_to_print = sort_print_object_instances(objects_by_extruder_it->second, layers, &new_ordering, single_object_instance_idx);
            }
            else {
                instances_to_print = sort_print_object_instances(objects_by_extruder_it->second, layers, ordering, single_object_instance_idx);
            }
        }

#ifdef SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
        const PrintObject *toolchange_source_object = m_last_obj_copy.first;
        const PrintObject *toolchange_target_object = instances_to_print.empty() ? nullptr : &instances_to_print.front().print_object;
        this->set_toolchange_source_object(toolchange_source_object);
        this->set_toolchange_target_object(toolchange_target_object);
#endif // SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
        if (extruder_id < m_remaining_extruder_segment_uses.size() && m_remaining_extruder_segment_uses[extruder_id] > 0)
            --m_remaining_extruder_segment_uses[extruder_id];

        if (has_wipe_tower) {
            if (extruder_id == skip_wipe_tower_toolchange_extruder) {
                skip_wipe_tower_toolchange_extruder = (unsigned int)-1;
            } else if (!m_wipe_tower->is_empty_wipe_tower_gcode(*this, extruder_id, extruder_id == layer_tools.extruders.back())) {
                if (need_insert_timelapse_gcode_for_traditional && !has_insert_timelapse_gcode) {
                    gcode += this->retract(false, false, LiftType::NormalLift);
                    m_writer.add_object_change_labels(gcode);

                    std::string timepals_gcode = insert_timelapse_gcode();
                    gcode += timepals_gcode;
                    m_writer.set_current_position_clear(false);
                    //BBS: check whether custom gcode changes the z position. Update if changed
                    double temp_z_after_timepals_gcode;
                    if (GCodeProcessor::get_last_z_from_gcode(timepals_gcode, temp_z_after_timepals_gcode)) {
                        Vec3d pos = m_writer.get_position();
                        pos(2) = temp_z_after_timepals_gcode;
                        m_writer.set_position(pos);
                    }
                    has_insert_timelapse_gcode = true;
                }
                gcode += m_wipe_tower->tool_change(*this, extruder_id, extruder_id == layer_tools.extruders.back());
            }
        } else {
            gcode += this->set_extruder(extruder_id, print_z);
        }
#ifdef SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
        this->set_toolchange_source_object(nullptr);
        this->set_toolchange_target_object(nullptr);
#endif // SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT

        // let analyzer tag generator aware of a role type change
        if (layer_tools.has_wipe_tower && m_wipe_tower)
            m_last_processor_extrusion_role = erWipeTower;

        if (print.config().skirt_type == stCombined && !print.skirt().empty())
            gcode += generate_skirt(print, print.skirt(), Point(0, 0), layer_tools, layer, extruder_id);

        if (auto loops_it = skirt_loops_per_extruder.find(extruder_id); loops_it != skirt_loops_per_extruder.end()) {
            const std::pair<size_t, size_t> loops = loops_it->second;
            this->set_origin(0., 0.);
            m_avoid_crossing_perimeters.use_external_mp();
            Flow layer_skirt_flow = print.skirt_flow().with_height(float(m_skirt_done.back() - (m_skirt_done.size() == 1 ? 0. : m_skirt_done[m_skirt_done.size() - 2])));
            double mm3_per_mm = layer_skirt_flow.mm3_per_mm();
            for (size_t i = loops.first; i < loops.second; ++i) {
                // Adjust flow according to this layer's layer height.
                ExtrusionLoop loop = *dynamic_cast<const ExtrusionLoop*>(print.skirt().entities[i]);
                for (ExtrusionPath &path : loop.paths) {
                    path.height = layer_skirt_flow.height();
                    path.mm3_per_mm = mm3_per_mm;
                }
                //FIXME using the support_speed of the 1st object printed.
                gcode += this->extrude_loop(loop, "skirt", m_config.support_speed.value);
            }
            m_avoid_crossing_perimeters.use_external_mp(false);
            // Allow a straight travel move to the first object point if this is the first layer (but don't in next layers).
            if (first_layer && loops.first == 0)
                m_avoid_crossing_perimeters.disable_once();
        }

        if (objects_by_extruder_it == by_extruder.end())
            continue;

        // BBS
        if (print.config().skirt_type == stPerObject && print.config().print_sequence == PrintSequence::ByObject &&
            !layer.object()->object_skirt().empty() &&
            ((layer.id() < print.config().skirt_height || print.config().draft_shield == DraftShield::dsEnabled))) {
            for (InstanceToPrint& instance_to_print : instances_to_print) {
                if (instance_to_print.print_object.object_skirt().empty())
                    continue;

                if (this->m_objSupportsWithBrim.find(instance_to_print.print_object.id()) != this->m_objSupportsWithBrim.end() &&
                    print.m_supportBrimMap.at(instance_to_print.print_object.id()).entities.size() > 0)
                    continue;

                if (this->m_objsWithBrim.find(instance_to_print.print_object.id()) != this->m_objsWithBrim.end() &&
                    print.m_brimMap.at(instance_to_print.print_object.id()).entities.size() > 0)
                    continue;
                if (first_layer)
                    m_skirt_done.clear();

                if (layer.id() == 1 && m_skirt_done.size() > 1)
                    m_skirt_done.erase(m_skirt_done.begin() + 1, m_skirt_done.end());

                const Point& offset = instance_to_print.print_object.instances()[instance_to_print.instance_id].shift;
                gcode += generate_skirt(print, instance_to_print.print_object.object_skirt(), offset, layer_tools, layer, extruder_id,
                                        &instance_to_print.print_object);
            }
        }

        // We are almost ready to print. However, we must go through all the objects twice to print the the overridden extrusions first (infill/perimeter wiping feature):
        std::vector<ObjectByExtruder::Island::Region> by_region_per_copy_cache;
        struct DeferredSkeletonFlush {
            const InstanceToPrint* instance_to_print = nullptr;
            const LayerToPrint* layer_to_print = nullptr;
            bool object_layer_over_raft = false;
            std::vector<ObjectByExtruder::Island::Region> by_region;
        };
        std::vector<DeferredSkeletonFlush> deferred_skeleton_flushes;
        float deferred_skeleton_flush_volume = 0.f;
        for (int print_wipe_extrusions = is_anything_overridden; print_wipe_extrusions>=0; --print_wipe_extrusions) {
            if (is_anything_overridden && print_wipe_extrusions == 0)
                gcode+="; PURGING FINISHED\n";
            for (InstanceToPrint &instance_to_print : instances_to_print) {
                if (print.config().skirt_type == stPerObject && !instance_to_print.print_object.object_skirt().empty() &&
                    print.config().print_sequence == PrintSequence::ByLayer &&
                    (layer.id() < print.config().skirt_height || print.config().draft_shield == DraftShield::dsEnabled)) {
                    if (first_layer)
                        m_skirt_done.clear();
                    const Point& offset = instance_to_print.print_object.instances()[instance_to_print.instance_id].shift;
                    gcode += generate_skirt(print, instance_to_print.print_object.object_skirt(), offset, layer_tools, layer, extruder_id,
                                            &instance_to_print.print_object);
                    if (instances_to_print.size() > 1 && &instance_to_print != &*(instances_to_print.end() - 1))
                        m_skirt_done.pop_back();
                }

                const auto&         inst           = instance_to_print.print_object.instances()[instance_to_print.instance_id];
                const LayerToPrint& layer_to_print = layers[instance_to_print.layer_id];
                // To control print speed of the 1st object layer printed over raft interface.
                bool object_layer_over_raft = layer_to_print.object_layer && layer_to_print.object_layer->id() > 0 &&
                                              instance_to_print.print_object.slicing_parameters().raft_layers() ==
                                                  layer_to_print.object_layer->id();
                m_config.apply(instance_to_print.print_object.config(), true);
                // Update first layer travel accelerations whenever object config changes
                {
                    std::vector<unsigned int> fta;
                    for (size_t i = 0; i < m_config.initial_layer_travel_acceleration.values.size(); i++) {
                        if (!m_config.initial_layer_travel_acceleration.is_nil(i) &&
                            m_config.initial_layer_travel_acceleration.values[i] > 0) {
                            fta.emplace_back((unsigned int) floor(m_config.initial_layer_travel_acceleration.values[i] + 0.5));
                        } else {
                            fta.emplace_back(0);
                        }
                    }
                    m_writer.set_first_layer_travel_acceleration(fta);
                }
                m_layer                  = layer_to_print.layer();
                m_object_layer_over_raft = object_layer_over_raft;
                if (m_config.reduce_crossing_wall)
                    m_avoid_crossing_perimeters.init_layer(*m_layer);

                // BBS: label object id, prepare for cooling
                gcode += "; OBJECT_ID: " + std::to_string(instance_to_print.label_object_id) + "\n";
                if (this->config().gcode_label_objects) {
                    gcode += std::string("; printing object ") + instance_to_print.print_object.model_object()->name +
                             " id:" + std::to_string(instance_to_print.print_object.get_id()) + " copy " + std::to_string(inst.id) + "\n";
                }
                // exclude objects
                if (m_enable_exclude_object) {
                    if (is_BBL_Printer()) {
                        m_writer.set_object_start_str(
                            std::string("; start printing object, unique label id: ") +
                            std::to_string(instance_to_print.label_object_id) + "\n" + "M624 " +
                            _encode_label_ids_to_base64({instance_to_print.label_object_id}) + "\n");
                    } else {
                        const auto gflavor = print.config().gcode_flavor.value;
                        if (gflavor == gcfKlipper) {
                            m_writer.set_object_start_str(std::string("EXCLUDE_OBJECT_START NAME=") +
                                                          get_instance_name(&instance_to_print.print_object, inst.id) + "\n");
                        }
                        else if (gflavor == gcfMarlinLegacy || gflavor == gcfMarlinFirmware || gflavor == gcfRepRapFirmware) {
                            std::string str = std::string("M486 S") + std::to_string(inst.unique_id) + "\n";
                            m_writer.set_object_start_str(str);
                        }
                    }
                }

                // Orca(#7946): set current obj regardless of the `enable_overhang_speed` value, because
                // `enable_overhang_speed` is a PrintRegionConfig and here we don't have a region yet.
                // And no side effect doing this even if `enable_overhang_speed` is off, so don't bother
                // checking anything here.
                m_extrusion_quality_estimator.set_current_object(&instance_to_print.print_object);

                // When starting a new object, use the external motion planner for the first travel move.
                const Point &offset = instance_to_print.print_object.instances()[instance_to_print.instance_id].shift;

                const Transform3d& trafo  = instance_to_print.print_object.instances()[instance_to_print.instance_id].print_object->trafo();
                std::pair<const PrintObject*, Point> this_object_copy(&instance_to_print.print_object, offset);
                if (m_last_obj_copy != this_object_copy)
                    m_avoid_crossing_perimeters.use_external_mp_once();
                m_last_obj_copy = this_object_copy;
                this->set_origin(unscale(offset));
                if (instance_to_print.object_by_extruder.support != nullptr) {
                    m_layer = layers[instance_to_print.layer_id].support_layer;
                    m_object_layer_over_raft = false;

                    //BBS: print supports' brims first
                    if (this->m_objSupportsWithBrim.find(instance_to_print.print_object.id()) != this->m_objSupportsWithBrim.end() && !print_wipe_extrusions) {
                        this->set_origin(0., 0.);
                        m_avoid_crossing_perimeters.use_external_mp();
                        for (const ExtrusionEntity* ee : print.m_supportBrimMap.at(instance_to_print.print_object.id()).entities) {
                            gcode += this->extrude_entity(*ee, "brim", m_config.support_speed.value);
                        }
                        m_avoid_crossing_perimeters.use_external_mp(false);
                        // Allow a straight travel move to the first object point.
                        m_avoid_crossing_perimeters.disable_once();
                        this->m_objSupportsWithBrim.erase(instance_to_print.print_object.id());
                    }
                    // When starting a new object, use the external motion planner for the first travel move.
                    const Point& offset = instance_to_print.print_object.instances()[instance_to_print.instance_id].shift;
                    std::pair<const PrintObject*, Point> this_object_copy(&instance_to_print.print_object, offset);
                    if (m_last_obj_copy != this_object_copy)
                        m_avoid_crossing_perimeters.use_external_mp_once();
                    m_last_obj_copy = this_object_copy;
                    this->set_origin(unscale(offset));
                    ExtrusionEntityCollection support_eec;

                    // BBS
                    WipingExtrusions& wiping_extrusions = const_cast<LayerTools&>(layer_tools).wiping_extrusions();
                    bool support_overridden = wiping_extrusions.is_support_overridden(layer_to_print.original_object);
                    bool support_intf_overridden = wiping_extrusions.is_support_interface_overridden(layer_to_print.original_object);

                    ExtrusionRole support_extrusion_role = instance_to_print.object_by_extruder.support_extrusion_role;
                    bool is_overridden = support_extrusion_role == erSupportMaterialInterface ? support_intf_overridden : support_overridden;
                    if (is_overridden == (print_wipe_extrusions != 0)) {
                        gcode += this->extrude_support(
                            // support_extrusion_role is erSupportMaterial, erSupportTransition, erSupportMaterialInterface or erMixed for
                            // all extrusion paths.
                            /*instance_to_print.object_by_extruder.support->chained_path_from(m_last_pos, support_extrusion_role) */
                            *instance_to_print.object_by_extruder.support, support_extrusion_role);

                        // Make sure ironing is the last
                        if (support_extrusion_role == erMixed || support_extrusion_role == erSupportMaterialInterface) {
                            gcode += this->extrude_support(*instance_to_print.object_by_extruder.support, erIroning);
                        }
                    }
                    m_layer = layer_to_print.layer();
                    m_object_layer_over_raft = object_layer_over_raft;
                }
                //FIXME order islands?
                // Sequential tool path ordering of multiple parts within the same object, aka. perimeter tracking (#5511)
                for (ObjectByExtruder::Island &island : instance_to_print.object_by_extruder.islands) {
                    const auto& by_region_specific = is_anything_overridden ? island.by_region_per_copy(by_region_per_copy_cache, static_cast<unsigned int>(instance_to_print.instance_id), extruder_id, print_wipe_extrusions != 0) : island.by_region;
                    //BBS: add brim by obj by extruder
                    if (this->m_objsWithBrim.find(instance_to_print.print_object.id()) != this->m_objsWithBrim.end() && !print_wipe_extrusions) {
                        this->set_origin(0., 0.);
                        m_avoid_crossing_perimeters.use_external_mp();
                        for (const ExtrusionEntity* ee : print.m_brimMap.at(instance_to_print.print_object.id()).entities) {
                            gcode += this->extrude_entity(*ee, "brim", m_config.support_speed.value);
                        }
                        m_avoid_crossing_perimeters.use_external_mp(false);
                        // Allow a straight travel move to the first object point.
                        m_avoid_crossing_perimeters.disable_once();
                        this->m_objsWithBrim.erase(instance_to_print.print_object.id());
                    }
                    // When starting a new object, use the external motion planner for the first travel move.
                    const Point& offset = instance_to_print.print_object.instances()[instance_to_print.instance_id].shift;
                    std::pair<const PrintObject*, Point> this_object_copy(&instance_to_print.print_object, offset);
                    if (m_last_obj_copy != this_object_copy)
                        m_avoid_crossing_perimeters.use_external_mp_once();
                    m_last_obj_copy = this_object_copy;
                    this->set_origin(unscale(offset));
                    //FIXME the following code prints regions in the order they are defined, the path is not optimized in any way.
                    //bool is_infill_first =m_config.is_infill_first;

                    auto has_infill = [](const std::vector<ObjectByExtruder::Island::Region> &by_region) {
                        for (auto region : by_region) {
                            if (!region.infills.empty())
                                return true;
                        }
                        return false;
                    };
                    auto has_skeleton = [](const std::vector<ObjectByExtruder::Island::Region> &by_region) {
                        for (const auto& region : by_region)
                            for (const ExtrusionEntity* ee : region.infills)
                                if (ee->role() == erInternalInfill)
                                    return true;
                        return false;
                    };

                    auto skeleton_volume = [](const std::vector<ObjectByExtruder::Island::Region> &by_region) {
                        float volume = 0.f;
                        for (const auto& region : by_region)
                            for (const ExtrusionEntity* ee : region.infills)
                                if (ee->role() == erInternalInfill)
                                    volume += float(ee->total_volume());
                        return volume;
                    };

                    auto filter_skeleton_infill = [](const std::vector<ObjectByExtruder::Island::Region> &by_region, bool skeleton_only) {
                        std::vector<ObjectByExtruder::Island::Region> filtered(by_region.size());
                        for (size_t region_idx = 0; region_idx < by_region.size(); ++region_idx) {
                            const ObjectByExtruder::Island::Region& src = by_region[region_idx];
                            ObjectByExtruder::Island::Region&       dst = filtered[region_idx];
                            for (ExtrusionEntity* ee : src.infills) {
                                const bool is_skeleton = ee->role() == erInternalInfill;
                                if (is_skeleton == skeleton_only)
                                    dst.infills.emplace_back(ee);
                            }
                        }
                        return filtered;
                    };

                    {
                        auto maybe_insert_timelapse_before_infill = [&]() {
                            if (!has_wipe_tower && need_insert_timelapse_gcode_for_traditional && !has_insert_timelapse_gcode && has_infill(by_region_specific)) {
                                gcode += this->retract(false, false, LiftType::NormalLift);

                                std::string timepals_gcode = insert_timelapse_gcode();
                                gcode += timepals_gcode;
                                m_writer.set_current_position_clear(false);
                                //BBS: check whether custom gcode changes the z position. Update if changed
                                double temp_z_after_timepals_gcode;
                                if (GCodeProcessor::get_last_z_from_gcode(timepals_gcode, temp_z_after_timepals_gcode)) {
                                    Vec3d pos = m_writer.get_position();
                                    pos(2) = temp_z_after_timepals_gcode;
                                    m_writer.set_position(pos);
                                }

                                has_insert_timelapse_gcode = true;
                            }
                        };
                        auto extrude_perimeters_before_infill = [&]() {
                            gcode += this->extrude_perimeters(print, by_region_specific, first_layer, false);
                        };
                        auto extrude_perimeters_after_infill = [&]() {
                            gcode += this->extrude_perimeters(print, by_region_specific, first_layer, true);
                        };

                        const bool flush_into_skeleton_order =
                            creality::is_k2_series_printer_from_string(m_config.printer_model.value) &&
                            instance_to_print.print_object.config().flush_into_skeleton.value;
                        const float skeleton_flush_volume_for_color =
                            (flush_into_skeleton_order && next_extruder_id != (unsigned int)-1 && print_wipe_extrusions == 0) ?
                                layer_tools.wiping_extrusions().skeleton_flush_volume(extruder_id, next_extruder_id) :
                                0.f;
                        const bool skeleton_flush_for_color = skeleton_flush_volume_for_color > EPSILON;
                        const bool can_defer_skeleton_flush = skeleton_flush_for_color && has_skeleton(by_region_specific);

                        const bool saved_flush_into_skeleton_tail_wipe_enabled = m_flush_into_skeleton_tail_wipe_enabled;
                        m_flush_into_skeleton_tail_wipe_enabled = flush_into_skeleton_order;
                        extrude_perimeters_before_infill();
                        if (flush_into_skeleton_order) {
                            // Keep clean skin before skeleton so first-flush material is spent on hidden skeleton paths.
                            gcode += this->extrude_skin(print, by_region_specific);
                            maybe_insert_timelapse_before_infill();

                            if (can_defer_skeleton_flush) {
                                gcode += this->extrude_infill(print, filter_skeleton_infill(by_region_specific, false), false);
                                deferred_skeleton_flushes.push_back(DeferredSkeletonFlush{
                                    &instance_to_print,
                                    &layer_to_print,
                                    object_layer_over_raft,
                                    filter_skeleton_infill(by_region_specific, true)
                                });
                                deferred_skeleton_flush_volume += skeleton_volume(by_region_specific);
                            } else {
                                gcode += this->extrude_infill(print, by_region_specific, false);
                            }
                        } else {
                            // No skeleton flush: emit the normal infill stage between the two perimeter passes.
                            maybe_insert_timelapse_before_infill();
                            gcode += this->extrude_infill(print, by_region_specific, false);
                            gcode += this->extrude_skin(print, by_region_specific);
                        }
                        extrude_perimeters_after_infill();
                        m_flush_into_skeleton_tail_wipe_enabled = saved_flush_into_skeleton_tail_wipe_enabled;
                    }
                    // ironing
                    gcode += this->extrude_infill(print,by_region_specific, true);

                    //CP: analysis large area
                    auto slowdown_logic = CoolingSlowdownLogicType(m_config.cooling_slowdown_logic.get_at(extruder_id));
                    if (slowdown_logic == CoolingSlowdownLogicType::SmartCoolingZones) {
                        if (by_region_specific.size() == 0) {
                            continue;
                        }
                        bool large_area = true;
                        //CP: check if the layer is thin wall
                        if (!has_infill(by_region_specific)) {
                            large_area = false;
                            continue;
                        }
                        double_t total_area = 0.0;
                        const std::string filament_type = m_config.filament_type.get_at(extruder_id);
                        double_t slice_threshold = (filament_type == "PETG") ? 20.0 : 10.0;
                        for (const auto& lslice : layer.lslices) {
                            double slice_area = unscale_(unscale_(area(lslice)));
                            //CP: small island should coolling slow down, slice_area must be larger than 1 to ignore the print area of ??a single point
                            if (slice_area > 1 && slice_area < slice_threshold) {
                                large_area = false;
                            }
                            total_area += slice_area;
                        }
                        if (total_area < 100) {
                            large_area = false;
                        }
                        // CP: During the secondary slicing process, layer.loverhangs is cleared, so the bbox determination method is used to improve stability.
                        double_t overhangs_bbox_area = 0.0;
                        double_t overhangs_bbo_x     = unscale_(layer.loverhangs_bbox.max(0) - layer.loverhangs_bbox.min(0));
                        double_t overhangs_bbo_y     = unscale_(layer.loverhangs_bbox.max(1) - layer.loverhangs_bbox.min(1));
                        overhangs_bbox_area          = unscale_(unscale_(layer.loverhangs_bbox.area()));
                        if (overhangs_bbox_area > 100 || (overhangs_bbo_x > 1.5 && overhangs_bbo_y > 1.5)) {
                            large_area = false;
                        }
                        double_t overhang_area = 0.0;
                        for (const auto& loverhang : layer.loverhangs) {
                            overhang_area += unscale_(unscale_(area(loverhang)));
                        }
                        if (overhang_area > 0.40) {
                            large_area = false;
                        }
                        double_t ratio = overhang_area / (total_area + 1e-6);
                        if (ratio > 0.05) {
                            large_area = false;
                        }
                        if (boost::algorithm::contains(gcode, ";TYPE:Overhang wall") || boost::algorithm::contains(gcode, ";TYPE:Bridge")) {
                            large_area = false;
                        }
                        if (large_area) {
                            boost::replace_all(gcode, ";_EXTRUDE_SET_SPEED", ";_EXTRUDE_SET_SPEED;_LARGE_RANGE");
                        }
                    }
                }

                if (this->config().gcode_label_objects) {
                    gcode += std::string("; stop printing object ") +
                             instance_to_print.print_object.model_object()->name +
                             " id:" + std::to_string(instance_to_print.print_object.get_id()) + " copy " +
                             std::to_string(inst.id) + "\n";
                }
                // exclude objects
                // Don't set m_gcode_label_objects_end if you don't had to write the m_gcode_label_objects_start.
                if (!m_writer.is_object_start_str_empty()) {
                    m_writer.set_object_start_str("");
                } else if (m_enable_exclude_object) {
                    if (is_BBL_Printer()) {
                        m_writer.set_object_end_str(std::string("; stop printing object, unique label id: ") +
                                                    std::to_string(instance_to_print.label_object_id) + "\n" +
                                                    "M625\n");
                    } else {
                        const auto gflavor = print.config().gcode_flavor.value;
                        if (gflavor == gcfKlipper) {
                            m_writer.set_object_end_str(std::string("EXCLUDE_OBJECT_END NAME=") +
                                                        get_instance_name(&instance_to_print.print_object, inst.id) + "\n");
                        } else if (gflavor == gcfMarlinLegacy || gflavor == gcfMarlinFirmware || gflavor == gcfRepRapFirmware) {
                            m_writer.set_object_end_str(std::string("M486 S-1\n"));
                        }
                    }
                }
            }
        }
        if (!deferred_skeleton_flushes.empty()) {
            auto emit_deferred_skeleton_flushes = [&]() -> std::string {
                std::string skeleton_gcode;
                const bool saved_flush_into_skeleton_tail_wipe_enabled = m_flush_into_skeleton_tail_wipe_enabled;
                const bool saved_flush_into_skeleton_center_start_enabled = m_flush_into_skeleton_center_start_enabled;
                m_flush_into_skeleton_tail_wipe_enabled = true;
                m_flush_into_skeleton_center_start_enabled = true;
                for (const DeferredSkeletonFlush& deferred : deferred_skeleton_flushes) {
                    if (deferred.instance_to_print == nullptr || deferred.layer_to_print == nullptr)
                        continue;

                    const InstanceToPrint& deferred_instance = *deferred.instance_to_print;
                    const auto& deferred_inst = deferred_instance.print_object.instances()[deferred_instance.instance_id];
                    m_config.apply(deferred_instance.print_object.config(), true);
                    {
                        std::vector<unsigned int> fta;
                        for (size_t i = 0; i < m_config.initial_layer_travel_acceleration.values.size(); i++) {
                            if (!m_config.initial_layer_travel_acceleration.is_nil(i) &&
                                m_config.initial_layer_travel_acceleration.values[i] > 0) {
                                fta.emplace_back((unsigned int) floor(m_config.initial_layer_travel_acceleration.values[i] + 0.5));
                            } else {
                                fta.emplace_back(0);
                            }
                        }
                        m_writer.set_first_layer_travel_acceleration(fta);
                    }

                    m_layer = deferred.layer_to_print->layer();
                    m_object_layer_over_raft = deferred.object_layer_over_raft;
                    if (m_config.reduce_crossing_wall && m_layer != nullptr)
                        m_avoid_crossing_perimeters.init_layer(*m_layer);
                    m_extrusion_quality_estimator.set_current_object(&deferred_instance.print_object);

                    const Point& deferred_offset = deferred_inst.shift;
                    std::pair<const PrintObject*, Point> this_object_copy(&deferred_instance.print_object, deferred_offset);
                    if (m_last_obj_copy != this_object_copy)
                        m_avoid_crossing_perimeters.use_external_mp_once();
                    m_last_obj_copy = this_object_copy;
                    this->set_origin(unscale(deferred_offset));

                    skeleton_gcode += "; OBJECT_ID: " + std::to_string(deferred_instance.label_object_id) + "\n";
                    if (this->config().gcode_label_objects) {
                        skeleton_gcode += std::string("; printing object ") + deferred_instance.print_object.model_object()->name +
                                         " id:" + std::to_string(deferred_instance.print_object.get_id()) + " copy " +
                                         std::to_string(deferred_inst.id) + " skeleton flush\n";
                    }
                    skeleton_gcode += this->extrude_infill(print, deferred.by_region, false);
                    if (this->config().gcode_label_objects) {
                        skeleton_gcode += std::string("; stop printing object ") +
                                         deferred_instance.print_object.model_object()->name +
                                         " id:" + std::to_string(deferred_instance.print_object.get_id()) + " copy " +
                                         std::to_string(deferred_inst.id) + " skeleton flush\n";
                    }
                }
                m_flush_into_skeleton_tail_wipe_enabled = saved_flush_into_skeleton_tail_wipe_enabled;
                m_flush_into_skeleton_center_start_enabled = saved_flush_into_skeleton_center_start_enabled;
                return skeleton_gcode;
            };

            const float skeleton_flush_volume = next_extruder_id != (unsigned int)-1 ?
                layer_tools.wiping_extrusions().skeleton_flush_volume(extruder_id, next_extruder_id) : 0.f;
            const float skeleton_model_volume = std::max(skeleton_flush_volume, deferred_skeleton_flush_volume);
            if (next_extruder_id == (unsigned int)-1) {
                gcode += emit_deferred_skeleton_flushes();
            } else if (has_wipe_tower) {
                m_pending_skeleton_flush_gcode_generator = emit_deferred_skeleton_flushes;
                m_pending_skeleton_flush_generator_volume = skeleton_model_volume;
                gcode += m_wipe_tower->tool_change(*this, next_extruder_id, next_extruder_id == layer_tools.extruders.back());
                const bool skeleton_printed_by_early_toolchange = !m_pending_skeleton_flush_gcode_generator;
                if (skeleton_printed_by_early_toolchange)
                    skip_wipe_tower_toolchange_extruder = next_extruder_id;
                else {
                    m_pending_skeleton_flush_gcode_generator = nullptr;
                    m_pending_skeleton_flush_generator_volume = 0.f;
                    gcode += emit_deferred_skeleton_flushes();
                }
            } else {
                gcode += this->start_skeleton_flush_toolchange(next_extruder_id, print_z, skeleton_model_volume);
                gcode += emit_deferred_skeleton_flushes();
            }
        }
    }

    if (first_layer) {
        for (auto iter = by_extruder.begin(); iter != by_extruder.end(); ++iter) {
            if (!iter->second.empty())
                m_initial_layer_extruders.insert(iter->first);
        }
    }

#if 0
    // Apply spiral vase post-processing if this layer contains suitable geometry
    // (we must feed all the G-code into the post-processor, including the first
    // bottom non-spiral layers otherwise it will mess with positions)
    // we apply spiral vase at this stage because it requires a full layer.
    // Just a reminder: A spiral vase mode is allowed for a single object per layer, single material print only.
    if (m_spiral_vase)
        gcode = m_spiral_vase->process_layer(std::move(gcode));

    // Apply cooling logic; this may alter speeds.
    if (m_gcode_editor)
        gcode = m_gcode_editor->process_layer(std::move(gcode), layer.id(),
            // Flush the cooling buffer at each object layer or possibly at the last layer, even if it contains just supports (This should not happen).
            object_layer || last_layer);

    file.write(gcode);
#endif

    BOOST_LOG_TRIVIAL(trace) << "Exported layer " << layer.id() << " print_z " << print_z <<
    log_memory_info();

    if (!has_wipe_tower && need_insert_timelapse_gcode_for_traditional && !has_insert_timelapse_gcode) {
        if (m_support_traditional_timelapse)
            m_support_traditional_timelapse = false;

        gcode += this->retract(false, false, LiftType::NormalLift);
        m_writer.add_object_change_labels(gcode);

        std::string timepals_gcode = insert_timelapse_gcode();
        gcode += timepals_gcode;
        m_writer.set_current_position_clear(false);
        //BBS: check whether custom gcode changes the z position. Update if changed
        double temp_z_after_timepals_gcode;
        if (GCodeProcessor::get_last_z_from_gcode(timepals_gcode, temp_z_after_timepals_gcode)) {
            Vec3d pos = m_writer.get_position();
            pos(2) = temp_z_after_timepals_gcode;
            m_writer.set_position(pos);
        }
    }

    //gcode += this->unretract();
    result.gcode = std::move(gcode);
    result.cooling_buffer_flush = object_layer || raft_layer || last_layer;
    return result;
}

void GCode::apply_print_config(const PrintConfig &print_config)
{
    m_writer.apply_print_config(print_config);
    m_config.apply(print_config);
    m_scaled_resolution = scaled<double>(print_config.resolution.value);

#if ORCA_CHECK_GCODE_PLACEHOLDERS
    // If the gcode value is empty, set a value so that the check code within the parser is run
    for (auto opt : std::initializer_list<ConfigOptionString*>{
             &m_config.machine_start_gcode,
             &m_config.machine_end_gcode,
             &m_config.before_layer_change_gcode,
             &m_config.layer_change_gcode,
             &m_config.time_lapse_gcode,
             &m_config.change_filament_gcode,
             &m_config.change_extrusion_role_gcode,
             &m_config.printing_by_object_gcode,
             &m_config.machine_pause_gcode,
             &m_config.template_custom_gcode,
         }) {
        if (opt->empty())
            opt->set(new ConfigOptionString(";VALUE FOR TESTING "));
    }
    for (auto opt : std::initializer_list<ConfigOptionStrings*>{
             &m_config.filament_start_gcode,
             &m_config.filament_end_gcode
         }) {
        if (opt->empty())
            for (int i = 0; i < opt->size(); ++i)
                opt->set_at(new ConfigOptionString(";VALUE FOR TESTING "), i, 0);
    }
#endif
}

void GCode::append_full_config(const Print &print, std::string &str)
{
    const DynamicPrintConfig &cfg = print.full_print_config();
    // Sorted list of config keys, which shall not be stored into the G-code. Initializer list.
    static const std::set<std::string_view> banned_keys( {
        "compatible_printers"sv,
        "compatible_prints"sv,
        "print_host"sv,
        "print_host_webui"sv,
        "printhost_apikey"sv,
        "printhost_cafile"sv,
        "printhost_user"sv,
        "printhost_password"sv,
        "printhost_port"sv,
        "different_settings_to_system"sv
    });
    auto is_banned = [](const std::string &key) {
        return banned_keys.find(key) != banned_keys.end();
    };
    std::ostringstream ss;
    for (const std::string& key : cfg.keys()) {
        if (!is_banned(key) && !cfg.option(key)->is_nil()) {
            if (key == "wipe_tower_x" || key == "wipe_tower_y") {
                std::vector<std::string> default_filament_colour = (cfg.option<ConfigOptionStrings>("filament_colour"))->values;
                int color_count = std::count_if(default_filament_colour.begin(), default_filament_colour.end(),
                        [](const std::string& str) {
                            return !str.empty();
                        });
                //??????????
                if(color_count>1)
                    ss << std::fixed << std::setprecision(3) << "; " << key << " = " << dynamic_cast<const ConfigOptionFloats*>(cfg.option(key))->get_at(print.get_plate_index()) << "\n";
                else
                   ss << std::fixed << std::setprecision(3) << "; " << key << " = " << 0.0 << "\n";
            } else if (key == "flush_volumes_matrix")
            {
                std::vector<double> m_matrix = (cfg.option<ConfigOptionFloats>("flush_volumes_matrix"))->values;
                for (double& value : m_matrix) {
                    value = static_cast<int>(std::ceil(value));
                }
                DynamicPrintConfig tempconfig;
                tempconfig.set_key_value("flush_volumes_matrix", new ConfigOptionFloats(m_matrix));
                ss << "; " << key << " = " << tempconfig.opt_serialize(key) << "\n";
                tempconfig.clear();
            }
            else
                ss << "; " << key << " = " << cfg.opt_serialize(key) << "\n";
        }
    }
    str += ss.str();
}

void GCode::set_extruders(const std::vector<unsigned int> &extruder_ids)
{
    m_writer.set_extruders(extruder_ids);

    // enable wipe path generation if any extruder has wipe enabled
    m_wipe.enable = false;
    for (auto id : extruder_ids)
        if (m_config.wipe.get_at(id)) {
            m_wipe.enable = true;
            break;
        }
}

void GCode::set_origin(const Vec2d& pointf, const Transform3d& trafo)
{
    // if origin increases (goes towards right), last_pos decreases because it goes towards left

    {
        // if origin increases (goes towards right), last_pos decreases because it goes towards left
        const Point translate(scale_(m_origin(0) - pointf(0)), scale_(m_origin(1) - pointf(1)));
        m_last_pos += translate;
        m_wipe.path.translate(translate);
        m_origin = pointf;
    }
}

std::string GCode::preamble()
{
    std::string gcode = m_writer.preamble();

    /*  Perform a *silent* move to z_offset: we need this to initialize the Z
        position of our writer object so that any initial lift taking place
        before the first layer change will raise the extruder from the correct
        initial Z instead of 0.  */
    const bool auto_travel_acceleration_was_suppressed = m_writer.auto_travel_acceleration_suppressed();
    m_writer.set_auto_travel_acceleration_suppressed(true);
    m_writer.travel_to_z(m_config.z_offset.value);
    m_writer.set_auto_travel_acceleration_suppressed(auto_travel_acceleration_was_suppressed);

    return gcode;
}

// called by GCode::process_layer()
std::string GCode::change_layer(coordf_t print_z, bool toolchange)
{
    double limitSpeed = getLimitSpeed();
    std::string gcode;
    if (m_layer_count > 0)
        // Increment a progress bar indicator.
        gcode += m_writer.update_progress(++ m_layer_index, m_layer_count);
    //BBS
    coordf_t z = print_z + m_config.z_offset.value;  // in unscaled coordinates
    if (EXTRUDER_CONFIG(retract_when_changing_layer) && m_writer.will_move_z(z)) {
        LiftType lift_type = this->to_lift_type(ZHopType(EXTRUDER_CONFIG(z_hop_types)));
        //BBS: force to use SpiralLift when change layer if lift type is auto
        gcode += this->retract(toolchange, false, ZHopType(EXTRUDER_CONFIG(z_hop_types)) == ZHopType::zhtAuto ? LiftType::SpiralLift : lift_type);
    }

    m_writer.add_object_change_labels(gcode);

    if (m_spiral_vase) {
        //BBS: force to normal lift immediately in spiral vase mode
        std::ostringstream comment;
        comment << "move to next layer (" << m_layer_index << ")";
        gcode += m_writer.travel_to_z(z, comment.str(), limitSpeed);
    }
    else {
        //BBS: set m_need_change_layer_lift_z to be true so that z lift can be done in travel_to() function
        m_need_change_layer_lift_z = true;
    }

    m_nominal_z = z;

    // forget last wiping path as wiping after raising Z is pointless
    // BBS. Dont forget wiping path to reduce stringing.
    //m_wipe.reset_path();

    return gcode;
}



static std::unique_ptr<EdgeGrid::Grid> calculate_layer_edge_grid(const Layer& layer)
{
    auto out = make_unique<EdgeGrid::Grid>();

    // Create the distance field for a layer below.
    const coord_t distance_field_resolution = coord_t(scale_(1.) + 0.5);
    out->create(layer.lslices, distance_field_resolution);
    out->calculate_sdf();
#if 0
        {
            static int iRun = 0;
            BoundingBox bbox = (*lower_layer_edge_grid)->bbox();
            bbox.min(0) -= scale_(5.f);
            bbox.min(1) -= scale_(5.f);
            bbox.max(0) += scale_(5.f);
            bbox.max(1) += scale_(5.f);
            EdgeGrid::save_png(*(*lower_layer_edge_grid), bbox, scale_(0.1f), debug_out_path("GCode_extrude_loop_edge_grid-%d.png", iRun++));
        }
#endif
    return out;
}

std::string GCode::extrude_loop(ExtrusionLoop loop, std::string description, double speed, const ExtrusionEntitiesPtr& region_perimeters)
{
    // get a copy; don't modify the orientation of the original loop object otherwise
    // next copies (if any) would not detect the correct orientation

    bool is_hole = (loop.loop_role() & elrHole) == elrHole;

    if (m_config.spiral_mode && !is_hole) {
        // if spiral vase, we have to ensure that all contour are in the same orientation.
        loop.make_counter_clockwise();
    }

    // find the point of the loop that is closest to the current extruder position
    // or randomize if requested
    Point last_pos = this->last_pos();
    float seam_overhang = std::numeric_limits<float>::lowest();
    if (!m_config.spiral_mode && description == "perimeter") {
        assert(m_layer != nullptr);
        bool is_outer_wall_first = m_config.wall_sequence == WallSequence::OuterInner;
        m_seam_placer.place_seam(m_layer, loop, is_outer_wall_first, this->last_pos(), seam_overhang);
    } else
        loop.split_at(last_pos, false);

    const auto seam_scarf_type = m_config.seam_slope_type.value;
    bool enable_seam_slope = ((seam_scarf_type == SeamScarfType::External && !is_hole) || seam_scarf_type == SeamScarfType::All) &&
        !m_config.spiral_mode &&
        (loop.role() == erExternalPerimeter || (loop.role() == erPerimeter && m_config.seam_slope_inner_walls)) &&
        layer_id() > 0;
    const auto nozzle_diameter = EXTRUDER_CONFIG(nozzle_diameter);
    if (enable_seam_slope && m_config.seam_slope_conditional.value) {
        enable_seam_slope = loop.is_smooth(m_config.scarf_angle_threshold.value * M_PI / 180., nozzle_diameter);
    }

    if (enable_seam_slope && m_config.seam_slope_conditional.value && m_config.scarf_overhang_threshold.value > 0.0f) {
        const auto _line_width = loop.role() == erExternalPerimeter ? m_config.outer_wall_line_width.get_abs_value(nozzle_diameter) :
                                                                      m_config.inner_wall_line_width.get_abs_value(nozzle_diameter);
        enable_seam_slope      = seam_overhang < m_config.scarf_overhang_threshold.value * 0.01f * _line_width;
    }

    // clip the path to avoid the extruder to get exactly on the first point of the loop;
    // if polyline was shorter than the clipping distance we'd get a null polyline, so
    // we discard it in that case
    const double seam_gap = scale_(m_config.seam_gap.get_abs_value(nozzle_diameter));
    const double clip_length = m_enable_loop_clipping && !enable_seam_slope ? seam_gap : 0;

    // get paths
    ExtrusionPaths paths;
    loop.clip_end(clip_length, &paths);
    if (paths.empty()) return "";

    // SoftFever: check loop lenght for small perimeter.
    double small_peri_speed = -1;
    if (speed == -1 && loop.length() <= SMALL_PERIMETER_LENGTH(m_config.small_perimeter_threshold.value)) {
        if(m_config.small_perimeter_speed == 0)
            small_peri_speed = m_config.outer_wall_speed * 0.5;
        else
            small_peri_speed = m_config.small_perimeter_speed.get_abs_value(m_config.outer_wall_speed);
    }

    // extrude along the path
    std::string gcode;

    // Orca:
    // Port of "wipe inside before extruding an external perimeter" feature from super slicer
    // If region perimeters size not greater than or equal to 2, then skip the wipe inside move as we will extrude in mid air
    // as no neighbouring perimeter exists. If an internal perimeter exists, we should find 2 perimeters touching the de-retraction point
    // 1 - the currently printed external perimeter and 2 - the neighbouring internal perimeter.
    if (m_config.wipe_before_external_loop.value && !paths.empty() && paths.front().size() > 1 && paths.back().size() > 1 && paths.front().role() == erExternalPerimeter && region_perimeters.size() > 1) {
        const bool is_full_loop_ccw = loop.polygon().is_counter_clockwise();
        bool is_hole_loop = (loop.loop_role() & ExtrusionLoopRole::elrHole) != 0; // loop.make_counter_clockwise();
        const double nozzle_diam = nozzle_diameter;

        // note: previous & next are inverted to extrude "in the opposite direction, and we are "rewinding"
        Point previous_point = paths.front().polyline.points[1];
        Point current_point = paths.front().polyline.points.front();
        Point next_point = paths.back().polyline.points.back();

        // can happen if seam_gap is null
        if (next_point == current_point) {
            next_point = paths.back().polyline.points[paths.back().polyline.points.size() - 2];
        }

        Point a = next_point;  // second point
        Point b = previous_point;  // second to last point
        if ((is_hole_loop ? !is_full_loop_ccw : is_full_loop_ccw)) {
            // swap points
            std::swap(a, b);
        }

        double angle = current_point.ccw_angle(a, b) / 3;

        // turn outwards if contour, turn inwwards if hole
        if (is_hole_loop ? !is_full_loop_ccw : is_full_loop_ccw) angle *= -1;

        Vec2d current_pos = current_point.cast<double>();
        Vec2d next_pos = next_point.cast<double>();
        Vec2d vec_dist = next_pos - current_pos;
        double vec_norm = vec_dist.norm();
        // Offset distance is the minimum between half the nozzle diameter or half the line width for the upcomming perimeter
        // This is to mimimize potential instances where the de-retraction is performed on top of a neighbouring
        // thin perimeter due to arachne reducing line width.
        coordf_t dist = std::min(scaled(nozzle_diam) * 0.5, scaled(paths.front().width) * 0.5);

        // FIXME Hiding the seams will not work nicely for very densely discretized contours!
        Point pt = (current_pos + vec_dist * (2 * dist / vec_norm)).cast<coord_t>();
        pt.rotate(angle, current_point);
        pt = (current_pos + vec_dist * (2 * dist / vec_norm)).cast<coord_t>();
        pt.rotate(angle, current_point);

        // Search region perimeters for lines that are touching the de-retraction point.
        // If an internal perimeter exists, we should find 2 perimeters touching the de-retraction point
        // 1: the currently printed external perimeter and 2: the neighbouring internal perimeter.
        int discoveredTouchingLines = 0;
        for (const ExtrusionEntity* ee : region_perimeters){
            auto potential_touching_line = ee->as_polyline();
            AABBTreeLines::LinesDistancer<Line> potential_touching_line_distancer{potential_touching_line.lines()};
            auto touching_line = potential_touching_line_distancer.all_lines_in_radius(pt, scale_(nozzle_diam));
            if(touching_line.size()){
                discoveredTouchingLines ++;
                if(discoveredTouchingLines > 1) break; // found 2 touching lines. End the search early.
            }
        }
        // found 2 perimeters touching the de-retraction point. Its safe to deretract as the point will be
        // inside the model
        if(discoveredTouchingLines > 1){
            // use extrude instead of travel_to_xy to trigger the unretract
            ExtrusionPath fake_path_wipe(Polyline{pt, current_point}, paths.front());
            fake_path_wipe.mm3_per_mm = 0;
            gcode += extrude_path(fake_path_wipe, "move inwards before retraction/seam", speed);
        }
    }


    const auto speed_for_path = [&speed, &small_peri_speed](const ExtrusionPath& path) {
        // don't apply small perimeter setting for overhangs/bridges/non-perimeters
        const bool is_small_peri = is_perimeter(path.role()) && !is_bridge(path.role()) && small_peri_speed > 0 && (path.get_overhang_degree() == 0 || path.get_overhang_degree() > 5);
        return is_small_peri ? small_peri_speed : speed;
    };

    if (!enable_seam_slope) {
        // BBS: smooth speed of discontinuity areas
        if (m_config.detect_overhang_wall && m_config.overhang_speed_classic.value && m_config.smooth_speed_discontinuity_area &&
            (loop.role() == erExternalPerimeter || loop.role() == erPerimeter))
            smooth_speed_discontinuity_area(paths);
        for (ExtrusionPaths::iterator path = paths.begin(); path != paths.end(); ++path) {
            gcode += this->_extrude(*path, description, speed_for_path(*path));
        }
    } else {
        // Create seam slope
        double start_slope_ratio;
        if (m_config.seam_slope_start_height.percent) {
            start_slope_ratio = m_config.seam_slope_start_height.value / 100.;
        } else {
            // Get the ratio against current layer height
            double h = paths.front().height;
            start_slope_ratio = m_config.seam_slope_start_height.value / h;
        }
        if (start_slope_ratio >= 1)
            start_slope_ratio = 0.99;

        double loop_length = 0.;
        for (const auto & path : paths) {
            loop_length += unscale_(path.length());
        }

        const bool   slope_entire_loop        = m_config.seam_slope_entire_loop;
        const double slope_min_length         = slope_entire_loop ? loop_length : std::min(m_config.seam_slope_min_length.value, loop_length);
        const int    slope_steps              = m_config.seam_slope_steps;
        const double slope_max_segment_length = scale_(slope_min_length / slope_steps);

        // Calculate the sloped loop
        ExtrusionLoopSloped new_loop(paths, seam_gap, slope_min_length, slope_max_segment_length, start_slope_ratio, loop.loop_role());
        new_loop.clip_slope(seam_gap);

        if (m_config.detect_overhang_wall && m_config.overhang_speed_classic && m_config.smooth_speed_discontinuity_area &&
            (loop.role() == erExternalPerimeter || loop.role() == erPerimeter))
            smooth_speed_discontinuity_area(new_loop.paths);

        // Then extrude it
        for (const auto& p : new_loop.get_all_paths()) {
            gcode += this->_extrude(*p, description, speed_for_path(*p));
        }

        // Fix path for wipe
        if (!new_loop.ends.empty()) {
            paths.clear();
            // The start slope part is ignored as it overlaps with the end part
            paths.reserve(new_loop.paths.size() + new_loop.ends.size());
            paths.insert(paths.end(), new_loop.paths.begin(), new_loop.paths.end());
            paths.insert(paths.end(), new_loop.ends.begin(), new_loop.ends.end());
        }
    }

    // BBS
    if (m_wipe.enable) {
        m_wipe.path = Polyline();
        for (ExtrusionPath &path : paths) {
            //BBS: Don't need to save duplicated point into wipe path
            if (!m_wipe.path.empty() && !path.empty() &&
                m_wipe.path.last_point() == path.first_point())
                m_wipe.path.append(path.polyline.points.begin() + 1, path.polyline.points.end());
            else
                m_wipe.path.append(path.polyline);  // TODO: don't limit wipe to last path
        }
    }

    // make a little move inwards before leaving loop
    if (m_config.wipe_on_loops.value && paths.back().role() == erExternalPerimeter && m_layer != NULL && m_config.wall_loops.value > 1 && paths.front().size() >= 2 && paths.back().polyline.points.size() >= 3) {
        // detect angle between last and first segment
        // the side depends on the original winding order of the polygon (inwards for contours, outwards for holes)
        //FIXME improve the algorithm in case the loop is tiny.
        //FIXME improve the algorithm in case the loop is split into segments with a low number of points (see the Point b query).
        Point a = paths.front().polyline.points[1];  // second point
        Point b = *(paths.back().polyline.points.end()-3);       // second to last point
        if (is_hole == loop.is_counter_clockwise()) {
            // swap points
            Point c = a; a = b; b = c;
        }

        double angle = paths.front().first_point().ccw_angle(a, b) / 3;

        // turn inwards if contour, turn outwards if hole
        if (is_hole == loop.is_counter_clockwise()) angle *= -1;

        // create the destination point along the first segment and rotate it
        // we make sure we don't exceed the segment length because we don't know
        // the rotation of the second segment so we might cross the object boundary
        Vec2d  p1 = paths.front().polyline.points.front().cast<double>();
        Vec2d  p2 = paths.front().polyline.points[1].cast<double>();
        Vec2d  v  = p2 - p1;
        double nd = scale_(EXTRUDER_CONFIG(nozzle_diameter));
        double l2 = v.squaredNorm();
        // Shift by no more than a nozzle diameter.
        //FIXME Hiding the seams will not work nicely for very densely discretized contours!
        //BBS. shorten the travel distant before the wipe path
        double threshold = 0.2;
        Point  pt = (p1 + v * threshold).cast<coord_t>();
        if (nd * nd < l2)
            pt = (p1 + threshold * v * (nd / sqrt(l2))).cast<coord_t>();
        //Point pt = ((nd * nd >= l2) ? (p1+v*0.4): (p1 + 0.2 * v * (nd / sqrt(l2)))).cast<coord_t>();
        pt.rotate(angle, paths.front().polyline.points.front());
        // generate the travel move
        gcode += m_writer.extrude_to_xy(this->point_to_gcode(pt), 0,"move inwards before travel",true);
    }

    return gcode;
}

std::string GCode::extrude_multi_path(ExtrusionMultiPath multipath, std::string description, double speed)
{
    // extrude along the path
    std::string gcode;
    for (ExtrusionPath path : multipath.paths)
        // CP: The path may be empty for unknown reasons.
        if (!path.polyline.points.empty()) {
            gcode += this->_extrude(path, description, speed);
        }

    // BBS
    if (m_wipe.enable) {
        m_wipe.path = Polyline();
        for (ExtrusionPath &path : multipath.paths) {
            //BBS: Don't need to save duplicated point into wipe path
            if (!m_wipe.path.empty() && !path.empty() &&
                m_wipe.path.last_point() == path.first_point())
                m_wipe.path.append(path.polyline.points.begin() + 1, path.polyline.points.end());
            else
                m_wipe.path.append(path.polyline); // TODO: don't limit wipe to last path
        }
        m_wipe.path.reverse();
    }

    return gcode;
}

std::string GCode::extrude_entity(const ExtrusionEntity &entity, std::string description, double speed, const ExtrusionEntitiesPtr& region_perimeters)
{
    if (const ExtrusionPath* path = dynamic_cast<const ExtrusionPath*>(&entity))
        return this->extrude_path(*path, description, speed);
    else if (const ExtrusionMultiPath* multipath = dynamic_cast<const ExtrusionMultiPath*>(&entity))
        return this->extrude_multi_path(*multipath, description, speed);
    else if (const ExtrusionLoop* loop = dynamic_cast<const ExtrusionLoop*>(&entity))
        return this->extrude_loop(*loop, description, speed, region_perimeters);
    else
        throw Slic3r::InvalidArgument("Invalid argument supplied to extrude()");
    return "";
}

std::string GCode::extrude_path(ExtrusionPath path, std::string description, double speed)
{
//    description += ExtrusionEntity::role_to_string(path.role());
    std::string gcode = this->_extrude(path, description, speed);
    if (m_wipe.enable) {
        m_wipe.path = std::move(path.polyline);
        m_wipe.path.reverse();
    }

    return gcode;
}

// Extrude perimeters: Decide where to put seams (hide or align seams).
std::string GCode::extrude_perimeters(const Print &print, const std::vector<ObjectByExtruder::Island::Region> &by_region, bool is_first_layer, bool is_infill_first)
{
    std::string gcode;
    for (const ObjectByExtruder::Island::Region &region : by_region)
        if (! region.perimeters.empty()) {
            m_config.apply(print.get_print_region(&region - &by_region.front()).config());
            // BBS: for first layer, we always print wall firstly to get better bed adhesive force
            // This behaviour is same with cura
            const bool should_print = is_first_layer ? !is_infill_first : (m_config.is_infill_first == is_infill_first);
            if (!should_print)
                continue;

            // BBS: output merged node id
            int curr_node    = 0;
            int cooling_node = -1;
            for (size_t perimeter_idx = 0; perimeter_idx < region.perimeters.size(); ++perimeter_idx) {
                const ExtrusionEntity* ee         = region.perimeters[perimeter_idx];
                int                    ee_node_id = ee->get_cooling_node();
                if (ee_node_id != cooling_node) {
                    gcode += "; COOLING_NODE: " + std::to_string(ee_node_id) + "\n";
                }

                gcode += this->extrude_entity(*ee, "perimeter", -1., region.perimeters);
            }
        }
    return gcode;
}

// Extrude skin (erSkinInfill): keep it as a distinct group so it can use the first flush segment after a toolchange.
std::string GCode::extrude_skin(const Print &print, const std::vector<ObjectByExtruder::Island::Region> &by_region)
{
    std::string gcode;
    for (const ObjectByExtruder::Island::Region &region : by_region) {
        if (! region.infills.empty()) {
            ExtrusionEntitiesPtr skin_extrusions;
            skin_extrusions.reserve(region.infills.size());
            // Collect only erSkinInfill entities
            for (ExtrusionEntity *ee : region.infills) {
                if (ee->role() == erSkinInfill)
                    skin_extrusions.emplace_back(ee);
            }
            // Print skin extrusions
            if (!skin_extrusions.empty()) {
                m_config.apply(print.get_print_region(&region - &by_region.front()).config());
                chain_and_reorder_extrusion_entities(skin_extrusions, &m_last_pos);
                for (const ExtrusionEntity *fill : skin_extrusions) {
                    auto *eec = dynamic_cast<const ExtrusionEntityCollection*>(fill);
                    if (eec) {
                        for (ExtrusionEntity *ee : eec->chained_path_from(m_last_pos).entities)
                            gcode += this->extrude_entity(*ee, "skin");
                    } else
                        gcode += this->extrude_entity(*fill, "skin");
                }
            }
        }
    }
    return gcode;
}

// Chain the paths hierarchically by a greedy algorithm to minimize a travel distance.
std::string GCode::extrude_infill(const Print &print, const std::vector<ObjectByExtruder::Island::Region> &by_region, bool ironing)
{
    std::string 		 gcode;
    ExtrusionEntitiesPtr extrusions;
    const char*          extrusion_name = ironing ? "ironing" : "infill";
    for (const ObjectByExtruder::Island::Region &region : by_region)
        if (! region.infills.empty()) {
            extrusions.clear();
            extrusions.reserve(region.infills.size());
            for (ExtrusionEntity *ee : region.infills)
                if ((ee->role() == erIroning) == ironing)
                    extrusions.emplace_back(ee);
            if (! extrusions.empty()) {
                m_config.apply(print.get_print_region(&region - &by_region.front()).config());

                // Skin (erSkinInfill) is printed separately after walls and skeleton.
                // Only print skeleton (erInternalInfill and others) now.
                ExtrusionEntitiesPtr skeleton_extrusions;
                for (ExtrusionEntity *ee : extrusions) {
                    if (ee->role() != erSkinInfill)
                        skeleton_extrusions.emplace_back(ee);
                }

                // Print skeleton (flush/color-change may happen before this group)
                if (!skeleton_extrusions.empty()) {
                    Point skeleton_start_reference = m_last_pos;
                    if (m_flush_into_skeleton_center_start_enabled) {
                        BoundingBox skeleton_bbox;
                        Points extrusion_points;
                        for (const ExtrusionEntity *ee : skeleton_extrusions) {
                            extrusion_points.clear();
                            ee->collect_points(extrusion_points);
                            for (const Point &point : extrusion_points)
                                skeleton_bbox.merge(point);
                        }
                        if (skeleton_bbox.defined)
                            skeleton_start_reference = skeleton_bbox.center();
                    }

                    chain_and_reorder_extrusion_entities(skeleton_extrusions, &skeleton_start_reference);
                    for (const ExtrusionEntity *fill : skeleton_extrusions) {
                        auto *eec = dynamic_cast<const ExtrusionEntityCollection*>(fill);
                        if (eec) {
                            for (ExtrusionEntity *ee : eec->chained_path_from(skeleton_start_reference).entities)
                                gcode += this->extrude_entity(*ee, extrusion_name);
                        } else
                            gcode += this->extrude_entity(*fill, extrusion_name);
                    }
                }
            }
        }
    return gcode;
}

std::string GCode::extrude_support(const ExtrusionEntityCollection& support_fills, const ExtrusionRole support_extrusion_role)
{
    static constexpr const char* support_label            = "support material";
    static constexpr const char* support_interface_label  = "support material interface";
    static constexpr const char* support_transition_label = "support transition";
    static constexpr const char* support_ironing_label    = "support ironing";

    std::string gcode;
    if (!support_fills.entities.empty()) {
        ExtrusionEntitiesPtr extrusions;
        extrusions.reserve(support_fills.entities.size());
        for (ExtrusionEntity* ee : support_fills.entities) {
            const auto role = ee->role();
            if ((role == support_extrusion_role) || (support_extrusion_role == erMixed && role != erIroning)) {
                extrusions.emplace_back(ee);
            }
        }
        if (extrusions.empty())
            return gcode;

        //chain_and_reorder_extrusion_entities(extrusions, &m_last_pos);

        const double support_speed           = m_config.support_speed.value;
        const double support_interface_speed = m_config.get_abs_value("support_interface_speed");
        for (const ExtrusionEntity* ee : extrusions) {
            ExtrusionRole role = ee->role();
            assert(role == erSupportMaterial || role == erSupportMaterialInterface || role == erSupportTransition || role == erIroning);
            const char* label = (role == erSupportMaterial) ?
                                    support_label :
                                    ((role == erSupportMaterialInterface) ?
                                         support_interface_label :
                                         ((role == erIroning) ? support_ironing_label : support_transition_label));
            // BBS
            // const double speed = (role == erSupportMaterial) ? support_speed : support_interface_speed;
            const double                     speed      = -1.0;
            const ExtrusionPath*             path       = dynamic_cast<const ExtrusionPath*>(ee);
            const ExtrusionMultiPath*        multipath  = dynamic_cast<const ExtrusionMultiPath*>(ee);
            const ExtrusionLoop*             loop       = dynamic_cast<const ExtrusionLoop*>(ee);
            const ExtrusionEntityCollection* collection = dynamic_cast<const ExtrusionEntityCollection*>(ee);
            if (path)
                gcode += this->extrude_path(*path, label, speed);
            else if (multipath) {
                gcode += this->extrude_multi_path(*multipath, label, speed);
            } else if (loop) {
                gcode += this->extrude_loop(*loop, label, speed);
            } else if (collection) {
                gcode += extrude_support(*collection, support_extrusion_role);
            } else {
                throw Slic3r::InvalidArgument("Unknown extrusion type");
            }
        }
    }
    return gcode;
}

bool GCode::GCodeOutputStream::is_error() const
{
    return ::ferror(this->f);
}

void GCode::GCodeOutputStream::flush()
{
    ::fflush(this->f);
}

void GCode::GCodeOutputStream::close()
{
    if (this->f) {
        ::fclose(this->f);
        this->f = nullptr;
    }
}
void GCode::GCodeOutputStream::write_md5(std::string src_gcode_file)
{
    unsigned char digest[16];
    MD5_CTX       ctx;
    MD5_Init(&ctx);
    boost::filesystem::ifstream ifs(src_gcode_file, std::ios::binary);
    std::string                 buf(64 * 1024, 0);
    const std::size_t &         size      = boost::filesystem::file_size(src_gcode_file);
    std::size_t                 left_size = size;
    while (ifs) {
        ifs.read(buf.data(), buf.size());
        int read_bytes = ifs.gcount();
        MD5_Update(&ctx, (unsigned char *) buf.data(), read_bytes);
    }
    MD5_Final(digest, &ctx);
    char md5_str[33];
    for (int j = 0; j < 16; j++) { sprintf(&md5_str[j * 2], "%02X", (unsigned int) digest[j]); }
    std::string gcode_file_md5 = std::string(md5_str);
    ifs.close();
    boost::nowide::ofstream file(src_gcode_file, std::ios::in | std::ios::out | std::ios::ate);
    file << "; MD5 = "<<gcode_file_md5 << std::endl;
    file.close();
}
void GCode::GCodeOutputStream::write_with_noprocess(const std::string& what)
{
    if (!what.empty()) {
        const char* gcode = what.c_str();
        // writes string to file
        size_t byteSize = ::strlen(gcode);
        size_t byteWrited = fwrite(gcode, 1, byteSize, this->f);
        if (byteWrited != byteSize)
        {
            char buf[256];
            sprintf(buf, "G-code export failed: code(%d)", errno);
            throw std::runtime_error(buf);
        }
    }
}

void GCode::GCodeOutputStream::process_gcode(const std::string& gcode)
{
    if (!gcode.empty()) {
        //FIXME don't allocate a string, maybe process a batch of lines?
        m_processor.process_buffer(gcode);
    }
}
void GCode::GCodeOutputStream::write(const char *what)
{
    if (what != nullptr) {
        const char* gcode = what;
        // writes string to file
        size_t byteSize = ::strlen(gcode);
        size_t byteWrited = fwrite(gcode, 1, byteSize, this->f);
        if (byteWrited != byteSize)
        {
            char buf[256];
            sprintf(buf, "G-code export failed: code(%d)", errno);
            throw std::runtime_error(buf);
        }
        //FIXME don't allocate a string, maybe process a batch of lines?
        m_processor.process_buffer(std::string(gcode));
    }
}

void GCode::GCodeOutputStream::writeln(const std::string &what)
{
    if (! what.empty())
        this->write(what.back() == '\n' ? what : what + '\n');
}

void GCode::GCodeOutputStream::write_format(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    int buflen;
    {
        va_list args2;
        va_copy(args2, args);
        buflen =
    #ifdef _MSC_VER
            ::_vscprintf(format, args2)
    #else
            ::vsnprintf(nullptr, 0, format, args2)
    #endif
            + 1;
        va_end(args2);
    }

    char buffer[1024];
    bool buffer_dynamic = buflen > 1024;
    char *bufptr = buffer_dynamic ? (char*)malloc(buflen) : buffer;
    int res = ::vsnprintf(bufptr, buflen, format, args);
    if (res > 0)
        this->write(bufptr);

    if (buffer_dynamic)
        free(bufptr);

    va_end(args);
}

static std::map<int, std::string> overhang_speed_key_map =
{
    {1, "overhang_1_4_speed"},
    {2, "overhang_2_4_speed"},
    {3, "overhang_3_4_speed"},
    {4, "overhang_4_4_speed"},
    {5, "overhang_totally_speed"},
    {6, "bridge_speed"},
};

double GCode::get_path_speed(const ExtrusionPath& path)
{
    double min_speed = double(m_config.slow_down_min_speed.get_at(m_writer.extruder()->id()));
    // set speed
    double speed = 0;
    if (path.role() == erPerimeter) {
        speed = m_config.get_abs_value("inner_wall_speed");
        if (m_config.enable_overhang_speed.value) {
            double new_speed = 0;
            new_speed        = get_overhang_degree_corr_speed(speed, path.overhang_degree);
            speed            = new_speed == 0.0 ? speed : new_speed;
        }
    } else if (path.role() == erExternalPerimeter) {
        speed = m_config.get_abs_value("outer_wall_speed");
        if (m_config.enable_overhang_speed.value) {
            double new_speed = 0;
            new_speed        = get_overhang_degree_corr_speed(speed, path.overhang_degree);
            speed            = new_speed == 0.0 ? speed : new_speed;
        }
    } else if (path.role() == erOverhangPerimeter && path.overhang_degree == 5)
        speed = m_config.get_abs_value("overhang_totally_speed");
    else if (path.role() == erOverhangPerimeter || path.role() == erBridgeInfill || path.role() == erSupportTransition) {
        speed = m_config.get_abs_value("bridge_speed");
    }
    auto _mm3_per_mm = path.mm3_per_mm *
                       double(m_curr_print->calib_mode() == CalibMode::Calib_Flow_Rate ? this->config().print_flow_ratio.value : 1);

    // BBS: if not set the speed, then use the filament_max_volumetric_speed directly
    double filament_max_volumetric_speed = EXTRUDER_CONFIG(filament_max_volumetric_speed);
    if (speed == 0) {
        if (_mm3_per_mm > 0)
            speed = filament_max_volumetric_speed / _mm3_per_mm;
        else
            speed = filament_max_volumetric_speed / path.mm3_per_mm;
    }
    if (this->on_first_layer()) {
        // BBS: for solid infill of initial layer, speed can be higher as long as
        // wall lines have be attached
        if (path.role() != erBottomSurface)
            speed = m_config.get_abs_value("initial_layer_speed");
    }

    if (filament_max_volumetric_speed > 0) {
        double extrude_speed = filament_max_volumetric_speed / path.mm3_per_mm;
        if (_mm3_per_mm > 0)
            extrude_speed = filament_max_volumetric_speed / _mm3_per_mm;

        // cap speed with max_volumetric_speed anyway (even if user is not using autospeed)
        speed = std::min(speed, extrude_speed);
    }

    return speed;
}

// BBS: f(x)=2x^2
double GCode::mapping_speed(double dist)
{
    if (dist <= 0)
        return 0;
    return this->config().smooth_coefficient * pow(dist, 2);
}

double GCode::get_speed_coor_x(double speed)
{
    double temp = speed / this->config().smooth_coefficient;
    return sqrt(temp);
}

double GCode::get_overhang_degree_corr_speed(float normal_speed, double path_degree) {

    //BBS: protection: overhang degree is float, make sure it not excess degree range
    if (path_degree <= 0)
        return normal_speed;

    int lower_degree_bound = int(path_degree);
    if (path_degree >= 5 || path_degree == lower_degree_bound)
        return m_config.get_abs_value(overhang_speed_key_map[lower_degree_bound].c_str());

    int upper_degree_bound = lower_degree_bound + 1;

    double lower_speed_bound = lower_degree_bound == 0 ? normal_speed : m_config.get_abs_value(overhang_speed_key_map[lower_degree_bound].c_str());
    double upper_speed_bound = upper_degree_bound == 0 ? normal_speed : m_config.get_abs_value(overhang_speed_key_map[upper_degree_bound].c_str());

    lower_speed_bound = lower_speed_bound == 0 ? normal_speed : lower_speed_bound;
    upper_speed_bound = upper_speed_bound == 0 ? normal_speed : upper_speed_bound;

    double speed_out = lower_speed_bound + (upper_speed_bound - lower_speed_bound) * (path_degree - lower_degree_bound);
    return speed_out;
}

static bool need_smooth_speed(const ExtrusionPath& other_path, const ExtrusionPath& this_path)
{
    if (this_path.smooth_speed - other_path.smooth_speed > smooth_speed_step)
        return true;

    return false;
}

ExtrusionPaths GCode::split_and_mapping_speed(
    double& other_path_v, double& final_v, ExtrusionPath& this_path, double max_smooth_length, bool split_from_left)
{
    ExtrusionPaths splited_path;
    if (this_path.length() <= 0 || this_path.polyline.points.size() < 2) {
        return splited_path;
    }

    // reverse if this slowdown the speed
    Polyline input_polyline = this_path.polyline;
    if (!split_from_left)
        std::reverse(input_polyline.begin(), input_polyline.end());

    double this_path_x = scale_(get_speed_coor_x(final_v));
    double x_base      = scale_(get_speed_coor_x(other_path_v));

    double smooth_length = this_path_x - x_base;

    // this length not support to get final v, adjust final v
    if (smooth_length > max_smooth_length)
        final_v = mapping_speed(unscale_(x_base + max_smooth_length));

    double max_step_length = scale_(1.0); // cut path if the path too long
    double min_step_length = scale_(0.4); // cut step

    double smooth_length_count = 0;
    double split_line_speed    = 0;
    Point  line_start_pt       = input_polyline.points.front();
    Point  line_end_pt         = input_polyline.points[1];
    bool   get_next_line       = false;
    size_t end_pt_idx          = 1;

    auto insert_speed = [this](double line_lenght, double& pos_x, double& smooth_length_count, double target_v) {
        pos_x += line_lenght;
        double pos_x_speed = mapping_speed(unscale_(pos_x));
        smooth_length_count += line_lenght;

        if (pos_x_speed > target_v)
            pos_x_speed = target_v;

        return pos_x_speed;
    };

    while (split_line_speed < final_v && end_pt_idx < input_polyline.size()) {
        // move to next line
        if (get_next_line) {
            line_start_pt = input_polyline.points[end_pt_idx - 1];
            line_end_pt   = input_polyline.points[end_pt_idx];
        }
        // This line is cut off as a speed transition area
        Polyline cuted_polyline;
        Line     line(line_start_pt, line_end_pt);

        cuted_polyline.append(line_start_pt);
        // split polyline and set speed
        if (line.length() < max_step_length || line.length() - min_step_length < min_step_length / 2) {
            split_line_speed = insert_speed(line.length(), x_base, smooth_length_count, final_v);
            end_pt_idx++;
            get_next_line = true;
            cuted_polyline.append(line.b);
        } else {
            // path is too long, split it
            double rate     = min_step_length / line.length();
            Point  insert_p = line.a + (line.b - line.a) * rate;

            split_line_speed = insert_speed(min_step_length, x_base, smooth_length_count, final_v);
            line_start_pt    = insert_p;
            get_next_line    = false;
            cuted_polyline.append(insert_p);
        }

        ExtrusionPath path_step(cuted_polyline, this_path);
        path_step.smooth_speed = split_line_speed;
        splited_path.push_back(std::move(path_step));
    }

    // reverse path back
    Polyline p1, p2;
    Point&   split_point = splited_path.back().polyline.points.back();
    this_path.polyline.split_at(split_point, &p1, &p2);
    if (!split_from_left) {
        this_path.polyline = p1;
        std::reverse(splited_path.begin(), splited_path.end());
        for (ExtrusionPath& path : splited_path) {
            std::reverse(path.polyline.points.begin(), path.polyline.points.end());
        }
    } else {
        this_path.polyline = p2;
    }

    return splited_path;
}

ExtrusionPaths GCode::merge_same_speed_paths(const ExtrusionPaths& paths)
{
    ExtrusionPaths               output_paths;
    std::optional<ExtrusionPath> merged_path;

    for (size_t path_idx = 0; path_idx < paths.size(); ++path_idx) {
        ExtrusionPath path = paths[path_idx];
        path.smooth_speed  = get_path_speed(path);

        if (path.role() == erOverhangPerimeter) {
            if (merged_path.has_value()) {
                output_paths.push_back(std::move(*merged_path));
                merged_path = std::nullopt;
            }
            output_paths.emplace_back(path);
            continue;
        }

        if (!merged_path.has_value()) {
            merged_path = path;
            continue;
        }

        if (merged_path->can_merge(path)) {
            merged_path->polyline.append(path.polyline);
        } else {
            output_paths.push_back(std::move(*merged_path));
            merged_path = path;
        }
    }

    if (merged_path.has_value())
        output_paths.push_back(std::move(*merged_path));

    return output_paths;
}

ExtrusionPaths GCode::set_speed_transition(ExtrusionPaths& paths)
{
    ExtrusionPaths interpolated_paths;
    for (int path_idx = 0; path_idx < paths.size(); path_idx++) {
        // update path
        ExtrusionPath& path = paths[path_idx];

        double this_path_speed = 0;
        // 100% overhang speed will not to set smooth speed
        if (path.role() == erOverhangPerimeter) {
            interpolated_paths.push_back(path);
            continue;
        }

        bool smooth_left_path  = false;
        bool smooth_right_path = false;
        // first line do not need to smooth speed on left
        // prev line speed may change
        if (path_idx > 0)
            smooth_left_path = need_smooth_speed(paths[path_idx - 1], path);

        // first line do not need to smooth speed on right
        if (path_idx < paths.size() - 1)
            smooth_right_path = need_smooth_speed(paths[path_idx + 1], path);

        // get smooth length
        double max_smooth_path_length = path.length();
        if (smooth_right_path && smooth_left_path)
            max_smooth_path_length /= 2;

        // smooth left
        ExtrusionPaths left_split_paths;
        if (smooth_left_path) {
            left_split_paths = split_and_mapping_speed(paths[path_idx - 1].smooth_speed, path.smooth_speed, path, max_smooth_path_length);
            if (!left_split_paths.empty())
                interpolated_paths.insert(interpolated_paths.end(), left_split_paths.begin(), left_split_paths.end());
            max_smooth_path_length = path.length();
        }

        // smooth right
        ExtrusionPaths right_split_paths;
        if (smooth_right_path) {
            right_split_paths = split_and_mapping_speed(paths[path_idx + 1].smooth_speed, path.smooth_speed, path, max_smooth_path_length,
                                                        false);
        }

        if (!path.empty())
            interpolated_paths.push_back(path);

        if (!right_split_paths.empty())
            interpolated_paths.insert(interpolated_paths.end(), right_split_paths.begin(), right_split_paths.end());
    }

    return interpolated_paths;
}

void GCode::smooth_speed_discontinuity_area(ExtrusionPaths& paths)
{
    if (paths.size() <= 1 || this->config().smooth_coefficient == 0)
        return;

    // step 1 merge same speed path
    size_t         path_tail_pos = 0;
    ExtrusionPaths prepare_paths = merge_same_speed_paths(paths);

    // step 2 split path
    ExtrusionPaths inter_paths;
    inter_paths = set_speed_transition(prepare_paths);
    paths       = std::move(inter_paths);
}

std::string GCode::_extrude(const ExtrusionPath &path, std::string description, double speed)
{
    double limitSpeed = getLimitSpeed();

    std::string gcode;

    if (is_bridge(path.role()))
        description += " (bridge)";

    const ExtrusionPathSloped* sloped = dynamic_cast<const ExtrusionPathSloped*>(&path);

    const auto get_sloped_z = [&sloped, this](double z_ratio) {
        const auto height = sloped->height;
        return lerp(m_nominal_z - height, m_nominal_z, z_ratio);
    };

    // go to first point of extrusion path
    //BBS: path.first_point is 2D point. But in lazy raise case, lift z is done in travel_to function.
    //Add m_need_change_layer_lift_z when change_layer in case of no lift if m_last_pos is equal to path.first_point() by chance
    if (!m_last_pos_defined || m_last_pos != path.first_point() || m_need_change_layer_lift_z || (sloped != nullptr && !sloped->is_flat())) {
        gcode += this->travel_to(
            path.first_point(),
            path.role(),
            "move to first " + description + " point",
            sloped == nullptr ? DBL_MAX : get_sloped_z(sloped->slope_begin.z_ratio)
        );
        m_need_change_layer_lift_z = false;
    }

    // if needed, write the gcode_label_objects_end then gcode_label_objects_start
    // should be already done by travel_to, but just in case
    m_writer.add_object_change_labels(gcode);

    // compensate retraction
    gcode += this->unretract(limitSpeed);
    m_config.apply(m_calib_config);
    m_writer.config.apply(m_calib_config, true);

    double weight = 0.0f;
    if (m_config.acceleration_limit_mess_enable
        || m_config.speed_limit_to_height_enable)
             weight = DoExport::update_total_weight(m_writer.extruders());

    // Orca: optimize for Klipper, set acceleration and jerk in one command
    unsigned int acceleration_i = 0;
    double jerk = 0;
    // adjust acceleration
    if (m_config.default_acceleration.value > 0) {
        double acceleration;
        if (this->on_first_layer() && m_config.initial_layer_acceleration.value > 0) {
            acceleration = m_config.initial_layer_acceleration.value;
#if 0
        } else if (this->object_layer_over_raft() && m_config.first_layer_acceleration_over_raft.value > 0) {
            acceleration = m_config.first_layer_acceleration_over_raft.value;
#endif
        } else if (m_config.get_abs_value("bridge_acceleration") > 0 && is_bridge(path.role())) {
            acceleration = m_config.get_abs_value("bridge_acceleration");
        } else if (m_config.get_abs_value("sparse_infill_acceleration") > 0 && (path.role() == erInternalInfill || path.role() == erSkinInfill)) {
            acceleration = m_config.get_abs_value("sparse_infill_acceleration");
        } else if (m_config.get_abs_value("internal_solid_infill_acceleration") > 0 && (path.role() == erSolidInfill)) {
            acceleration = m_config.get_abs_value("internal_solid_infill_acceleration");
        } else if (m_config.outer_wall_acceleration.value > 0 && is_external_perimeter(path.role())) {
            acceleration = m_config.outer_wall_acceleration.value;
        } else if (m_config.inner_wall_acceleration.value > 0 && is_internal_perimeter(path.role())) {
            acceleration = m_config.inner_wall_acceleration.value;
        } else if (m_config.top_surface_acceleration.value > 0 && is_top_surface(path.role())) {
            acceleration = m_config.top_surface_acceleration.value;
        } else {
            acceleration = m_config.default_acceleration.value;
        }

        if (m_config.acceleration_limit_mess_enable
            || m_config.speed_limit_to_height_enable)
                m_smoothSpeedAcc->detect_acc(acceleration, weight, m_last_layer_z);

        acceleration_i = (unsigned int)floor(acceleration + 0.5);
    }

    // adjust X Y jerk
    if (m_config.default_jerk.value > 0) {
        if (this->on_first_layer() && m_config.initial_layer_jerk.value > 0) {
            jerk = m_config.initial_layer_jerk.value;
        } else if (m_config.outer_wall_jerk.value > 0 && is_external_perimeter(path.role())) {
             jerk = m_config.outer_wall_jerk.value;
        } else if (m_config.inner_wall_jerk.value > 0 && is_internal_perimeter(path.role())) {
            jerk = m_config.inner_wall_jerk.value;
        } else if (m_config.top_surface_jerk.value > 0 && is_top_surface(path.role())) {
            jerk = m_config.top_surface_jerk.value;
        } else if (m_config.infill_jerk.value > 0 && is_infill(path.role())) {
            jerk = m_config.infill_jerk.value;
        }
        else {
            jerk = m_config.default_jerk.value;
        }
    }

    if (m_writer.get_gcode_flavor() == gcfKlipper) {
        gcode += m_writer.set_accel_and_jerk(acceleration_i, jerk);

    } else {
        gcode += m_writer.set_print_acceleration(acceleration_i);
        gcode += m_writer.set_jerk_xy(jerk);
    }

    // calculate extrusion length per distance unit
    auto _mm3_per_mm = path.mm3_per_mm * this->config().print_flow_ratio;
    if (path.role() == erTopSolidInfill)
        _mm3_per_mm *= m_config.top_solid_infill_flow_ratio;
    else if (path.role() == erBottomSurface)
        _mm3_per_mm *= m_config.bottom_solid_infill_flow_ratio;
    else if (path.role() == erInternalBridgeInfill)
    {
        if(m_config.internal_bridge_flow>0)
        {
            _mm3_per_mm *= m_config.internal_bridge_flow;
        }
        else{
            throw FlowErrorNegativeFlow();
        }
    }
    else if(sloped)
        _mm3_per_mm *= m_config.scarf_joint_flow_ratio;


    double e_per_mm = m_writer.extruder()->e_per_mm3() * _mm3_per_mm;

    double min_speed = double(m_config.slow_down_min_speed.get_at(m_writer.extruder()->id()));
    // set speed
    if (speed == -1) {
        if (path.role() == erPerimeter) {
            speed = m_config.get_abs_value("inner_wall_speed");
            if (m_config.detect_overhang_wall && m_config.overhang_speed_classic.value && m_config.smooth_speed_discontinuity_area &&
                path.smooth_speed != 0)
                speed = path.smooth_speed;
            else if (m_config.overhang_speed_classic.value && m_config.enable_overhang_speed.value) {
                double new_speed = 0;
                new_speed = get_overhang_degree_corr_speed(speed, path.overhang_degree);
                speed = new_speed == 0.0 ? speed : new_speed;
            }

            if (sloped) {
                speed = std::min(speed, m_config.scarf_joint_speed.get_abs_value(m_config.get_abs_value("inner_wall_speed")));
            }
        } else if (path.role() == erExternalPerimeter) {
            speed = m_config.get_abs_value("outer_wall_speed");
            if (m_config.detect_overhang_wall && m_config.overhang_speed_classic.value && m_config.smooth_speed_discontinuity_area &&
                path.smooth_speed != 0)
                speed = path.smooth_speed;
            else if (m_config.overhang_speed_classic.value && m_config.enable_overhang_speed.value ) {
                double new_speed = 0;
                new_speed = get_overhang_degree_corr_speed(speed, path.overhang_degree);
                speed = new_speed == 0.0 ? speed : new_speed;
            }
            if (sloped) {
                speed = std::min(speed, m_config.scarf_joint_speed.get_abs_value(m_config.get_abs_value("outer_wall_speed")));
            }
        }
        else if (path.role() == erOverhangPerimeter && path.overhang_degree == 5) {
            speed = m_config.get_abs_value("overhang_totally_speed");
        } else if (path.role() == erInternalBridgeInfill) {
            speed = m_config.get_abs_value("internal_bridge_speed");
        } else if (path.role() == erOverhangPerimeter || path.role() == erSupportTransition || path.role() == erBridgeInfill) {
            speed = m_config.get_abs_value("bridge_speed");
        } else if (path.role() == erInternalInfill || path.role() == erSkinInfill) {
            speed = m_config.get_abs_value("sparse_infill_speed");
        } else if (path.role() == erSolidInfill) {
            speed = m_config.get_abs_value("internal_solid_infill_speed");
        } else if (path.role() == erTopSolidInfill) {
            speed = m_config.get_abs_value("top_surface_speed");
        } else if (path.role() == erIroning) {
            speed = m_config.get_abs_value("ironing_speed");
        } else if (path.role() == erBottomSurface) {
            speed = m_config.get_abs_value("initial_layer_infill_speed");
        } else if (path.role() == erGapFill) {
            speed = m_config.get_abs_value("gap_infill_speed");
        }
        else if (path.role() == erSupportMaterial ||
                 path.role() == erSupportMaterialInterface) {
            const double  support_speed = m_config.support_speed.value;
            const double  support_interface_speed = m_config.get_abs_value("support_interface_speed");
            speed = (path.role() == erSupportMaterial) ? support_speed : support_interface_speed;
        } else {
            throw Slic3r::InvalidArgument("Invalid speed");
        }
    }
    //BBS: if not set the speed, then use the filament_max_volumetric_speed directly
    if (speed == 0)
        speed = EXTRUDER_CONFIG(filament_max_volumetric_speed) / _mm3_per_mm;
    if (this->on_first_layer()) {
        //BBS: for solid infill of initial layer, speed can be higher as long as
        //wall lines have be attached
        if (path.role() != erBottomSurface)
            speed = m_config.get_abs_value("initial_layer_speed");
    }
    else if(m_config.slow_down_layers > 1){
        const auto _layer = layer_id();
        if (_layer > 0 && _layer < m_config.slow_down_layers) {
            const auto first_layer_speed =
                is_perimeter(path.role())
                    ? m_config.get_abs_value("initial_layer_speed")
                    : m_config.get_abs_value("initial_layer_infill_speed");
            if (first_layer_speed < speed) {
                speed = std::min(
                    speed,
                    Slic3r::lerp(first_layer_speed, speed,
                                 (double)_layer / m_config.slow_down_layers));
            }
        }
    }
    // Override skirt speed if set
    if (path.role() == erSkirt) {
        const double skirt_speed = m_config.get_abs_value("skirt_speed");
        if (skirt_speed > 0.0)
        speed = skirt_speed;
    }
    //BBS: remove this config
    //else if (this->object_layer_over_raft())
    //    speed = m_config.get_abs_value("first_layer_speed_over_raft", speed);
    //if (m_config.max_volumetric_speed.value > 0) {
    //    // cap speed with max_volumetric_speed anyway (even if user is not using autospeed)
    //    speed = std::min(
    //        speed,
    //        m_config.max_volumetric_speed.value / _mm3_per_mm
    //    );
    //}
    if (EXTRUDER_CONFIG(filament_max_volumetric_speed) > 0) {
        // cap speed with max_volumetric_speed anyway (even if user is not using autospeed)
        speed = std::min(
            speed,
            EXTRUDER_CONFIG(filament_max_volumetric_speed) / _mm3_per_mm
        );
    }

    if (is_support(path.role())) {
        if (EXTRUDER_CONFIG(filament_max_volumetric_speed) > 0) {
            speed = std::min(speed, EXTRUDER_CONFIG(filament_max_volumetric_speed) / path.mm3_per_mm);
        }
    }

    bool variable_speed = false;
    std::vector<ProcessedPoint> new_points {};

    if (m_config.enable_overhang_speed && !m_config.overhang_speed_classic && !this->on_first_layer() &&
        (is_bridge(path.role()) || is_perimeter(path.role()))) {
            bool is_external = is_external_perimeter(path.role());
            double ref_speed   = is_external ? m_config.get_abs_value("outer_wall_speed") : m_config.get_abs_value("inner_wall_speed");
            if (ref_speed == 0)
                ref_speed = EXTRUDER_CONFIG(filament_max_volumetric_speed) / _mm3_per_mm;

            if (EXTRUDER_CONFIG(filament_max_volumetric_speed) > 0) {
                ref_speed = std::min(ref_speed, EXTRUDER_CONFIG(filament_max_volumetric_speed) / path.mm3_per_mm);
            }
            if (sloped) {
                ref_speed = std::min(ref_speed, m_config.scarf_joint_speed.get_abs_value(ref_speed));
            }

            ConfigOptionPercents         overhang_overlap_levels({81, 67.5, 45, 22.5, 0.1, 0});

            ConfigOptionFloatsOrPercents dynamic_overhang_speeds(
                {(m_config.get_abs_value("overhang_1_4_speed", ref_speed) < 0.5) ?
                     FloatOrPercent{100, true} :
                     FloatOrPercent{std::min(m_config.get_abs_value("overhang_1_4_speed", ref_speed), ref_speed) * 100 / ref_speed, true},
                 (m_config.get_abs_value("overhang_2_4_speed", ref_speed) < 0.5) ?
                     FloatOrPercent{100, true} :
                     FloatOrPercent{std::min(m_config.get_abs_value("overhang_2_4_speed", ref_speed), ref_speed) * 100 / ref_speed, true},
                 (m_config.get_abs_value("overhang_3_4_speed", ref_speed) < 0.5) ?
                     FloatOrPercent{100, true} :
                     FloatOrPercent{std::min(m_config.get_abs_value("overhang_3_4_speed", ref_speed), ref_speed) * 100 / ref_speed, true},
                 (m_config.get_abs_value("overhang_4_4_speed", ref_speed) < 0.5) ?
                     FloatOrPercent{100, true} :
                     FloatOrPercent{std::min(m_config.get_abs_value("overhang_4_4_speed", ref_speed), ref_speed) * 100 / ref_speed, true},
                 (m_config.get_abs_value("overhang_totally_speed", ref_speed) < 0.5) ?
                     FloatOrPercent{100, true} :
                     FloatOrPercent{std::min(m_config.get_abs_value("overhang_totally_speed", ref_speed), ref_speed) * 100 / ref_speed,
                                    true},
                 (m_config.get_abs_value("overhang_totally_speed", ref_speed) < 0.5) ?
                     FloatOrPercent{100, true} :
                     FloatOrPercent{std::min(m_config.get_abs_value("overhang_totally_speed", ref_speed), ref_speed) * 100 / ref_speed,
                                    true}});

            new_points = m_extrusion_quality_estimator.estimate_extrusion_quality(path, overhang_overlap_levels, dynamic_overhang_speeds,
                                                                            ref_speed, speed, m_config.slowdown_for_curled_perimeters);

            variable_speed = std::any_of(new_points.begin(), new_points.end(),
                                         [speed](const ProcessedPoint &p) { return fabs(double(p.speed) - speed) > 1; });// Ignore small speed variations (under 1mm/sec)

    }
    // check if the line is straight line, which mean if the wall is bridge
    if (path.role() == erOverhangPerimeter) {
        Line line(path.first_point(), path.last_point());
        if (line.length() >= path.length()) {
            speed          = m_config.get_abs_value("bridge_speed");
            variable_speed = false;
        }
    }

    double F = speed * 60;  // convert mm/sec to mm/min

    //Orca: process custom gcode for extrusion role change
    if (path.role() != m_last_extrusion_role && !m_config.change_extrusion_role_gcode.value.empty()) {
            DynamicConfig config;
            config.set_key_value("extrusion_role", new ConfigOptionString(extrusion_role_to_string_for_parser(path.role())));
            config.set_key_value("last_extrusion_role", new ConfigOptionString(extrusion_role_to_string_for_parser(m_last_extrusion_role)));
            config.set_key_value("layer_num", new ConfigOptionInt(m_layer_index + 1));
            config.set_key_value("layer_z", new ConfigOptionFloat(m_layer == nullptr ? m_last_height : m_layer->print_z));
            gcode += this->placeholder_parser_process("change_extrusion_role_gcode",
                                                      m_config.change_extrusion_role_gcode.value, m_writer.extruder()->id(), &config)
                     + "\n";
    }

    // extrude arc or line
    if (m_enable_extrusion_role_markers) {
        if (path.role() != m_last_extrusion_role) {
            char buf[32];
            sprintf(buf, ";_EXTRUSION_ROLE:%d\n", int(path.role()));
            gcode += buf;
      }
    }

    m_last_extrusion_role = path.role();

    // adds processor tags and updates processor tracking data
    // PrusaMultiMaterial::Writer may generate GCodeProcessor::Height_Tag lines without updating m_last_height
    // so, if the last role was erWipeTower we force export of GCodeProcessor::Height_Tag lines
    bool last_was_wipe_tower = (m_last_processor_extrusion_role == erWipeTower);
    char buf[64];
    assert(is_decimal_separator_point());

    if (path.role() != m_last_processor_extrusion_role) {
        m_last_processor_extrusion_role = path.role();
        sprintf(buf, ";%s%s\n", GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Role).c_str(), ExtrusionEntity::role_to_string(m_last_processor_extrusion_role).c_str());
        gcode += buf;
    }

    if (last_was_wipe_tower || m_last_width != path.width) {
        m_last_width = path.width;
        sprintf(buf, ";%s%g\n", GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Width).c_str(), m_last_width);
        gcode += buf;
    }

#if ENABLE_GCODE_VIEWER_DATA_CHECKING
    if (last_was_wipe_tower || (m_last_mm3_per_mm != path.mm3_per_mm)) {
        m_last_mm3_per_mm = path.mm3_per_mm;
        sprintf(buf, ";%s%f\n", GCodeProcessor::Mm3_Per_Mm_Tag.c_str(), m_last_mm3_per_mm);
        gcode += buf;
    }
#endif // ENABLE_GCODE_VIEWER_DATA_CHECKING

    if (last_was_wipe_tower || std::abs(m_last_height - path.height) > EPSILON) {
        m_last_height = path.height;
        sprintf(buf, ";%s%g\n", GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Height).c_str(), m_last_height);
        gcode += buf;
    }

    auto overhang_fan_threshold = EXTRUDER_CONFIG(overhang_fan_threshold);
    auto enable_overhang_bridge_fan = EXTRUDER_CONFIG(enable_overhang_bridge_fan);

    auto supp_interface_fan_speed = EXTRUDER_CONFIG(support_material_interface_fan_speed);


    //    { "0%", Overhang_threshold_none },
    //    { "10%", Overhang_threshold_1_4 },
    //    { "25%", Overhang_threshold_2_4 },
    //    { "50%", Overhang_threshold_3_4 },
    //    { "75%", Overhang_threshold_4_4 },
    //    { "95%", Overhang_threshold_bridge }
    auto check_overhang_fan = [&overhang_fan_threshold](float overlap, ExtrusionRole role) {
      switch (overhang_fan_threshold) {
      case (int)Overhang_threshold_1_4:
        return overlap <= 0.9f;
        break;
      case (int)Overhang_threshold_2_4:
        return overlap <= 0.75f;
        break;
      case (int)Overhang_threshold_3_4:
        return overlap <= 0.5f;
        break;
      case (int)Overhang_threshold_4_4:
        return overlap <= 0.25f;
        break;
      case (int)Overhang_threshold_bridge:
        return overlap <= 0.05f;
        break;
      case (int)Overhang_threshold_none:
        return is_external_perimeter(role);
        break;
      default:
        return false;
      }
    };

    std::string cooling_marker_setspeed_comments;
    if (m_enable_cooling_markers) {
        cooling_marker_setspeed_comments = ";_EXTRUDE_SET_SPEED";
        if (is_external_perimeter(path.role()))
            cooling_marker_setspeed_comments += ";_EXTERNAL_PERIMETER";
        else if (is_perimeter(path.role()))
        {
            if (path.perimeter_index.has_value())
            {
                cooling_marker_setspeed_comments += ";_INTERNAL_PERIMETER" + std::to_string(path.perimeter_index.value());
            }
        }
    }

    if (!variable_speed) {
        if (m_config.acceleration_limit_mess_enable
            || m_config.speed_limit_to_height_enable)
            m_smoothSpeedAcc->detect_speed_min(F, weight, m_last_layer_z);

        // F is mm per minute.
        gcode += m_writer.set_speed(F, "", cooling_marker_setspeed_comments);
        {
            if (m_enable_cooling_markers) {
                if (enable_overhang_bridge_fan) {
                    // BBS: Overhang_threshold_none means Overhang_threshold_1_4 and forcing cooling for all external
                    // perimeter
                    int overhang_threshold = overhang_fan_threshold == Overhang_threshold_none ? Overhang_threshold_none
                    : overhang_fan_threshold - 1;
                    if ((overhang_fan_threshold == Overhang_threshold_none && is_external_perimeter(path.role())) ||
                        (path.get_overhang_degree() > overhang_threshold || is_bridge(path.role()))) {
                        if (!m_is_overhang_fan_on) {
                            gcode += ";_OVERHANG_FAN_START\n";
                            m_is_overhang_fan_on = true;
                        }
                    } else {
                        if (m_is_overhang_fan_on) {
                            m_is_overhang_fan_on = false;
                            gcode += ";_OVERHANG_FAN_END\n";
                        }
                    }
                }
                if (supp_interface_fan_speed >= 0 && path.role() == erSupportMaterialInterface) {
                    if (!m_is_supp_interface_fan_on) {
                        gcode += ";_SUPP_INTERFACE_FAN_START\n";
                        m_is_supp_interface_fan_on = true;
                    }
                } else {
                    if (m_is_supp_interface_fan_on) {
                        gcode += ";_SUPP_INTERFACE_FAN_END\n";
                        m_is_supp_interface_fan_on = false;
                    }
                }
            }
            // BBS: use G1 if not enable arc fitting or has no arc fitting result or in spiral_mode mode or we are doing sloped extrusion
            // Attention: G2 and G3 is not supported in spiral_mode mode
            if (!m_config.enable_arc_fitting || path.polyline.fitting_result.empty() || m_config.spiral_mode || sloped != nullptr) {
                double path_length = 0.;
                double total_length = sloped == nullptr ? 0. : path.polyline.length() * SCALING_FACTOR;
                for (const Line& line : path.polyline.lines()) {
                    std::string tempDescription = description;
                    const double line_length = line.length() * SCALING_FACTOR;
                    if (line_length < EPSILON)
                        continue;
                    path_length += line_length;
                    auto dE = e_per_mm * line_length;
                    if (m_small_area_infill_flow_compensator && m_config.small_area_infill_flow_compensation.value) {
                        auto oldE = dE;
                        dE = m_small_area_infill_flow_compensator->modify_flow(line_length, dE, path.role());

                        if (m_config.gcode_comments && oldE > 0 && oldE != dE) {
                            tempDescription += Slic3r::format(" | Old Flow Value: %0.5f Length: %0.5f",oldE, line_length);
                        }
                    }
                    if (sloped == nullptr) {
                        // Normal extrusion
                        gcode += m_writer.extrude_to_xy(
                            this->point_to_gcode(line.b),
                            dE,
                            GCodeWriter::full_gcode_comment ? tempDescription : "", path.is_force_no_extrusion());
                    } else {
                        // Sloped extrusion
                        const auto [z_ratio, e_ratio] = sloped->interpolate(path_length / total_length);
                        Vec2d dest2d = this->point_to_gcode(line.b);
                        Vec3d dest3d(dest2d(0), dest2d(1), get_sloped_z(z_ratio));
                        gcode += m_writer.extrude_to_xyz(
                            dest3d,
                            dE * e_ratio,
                            GCodeWriter::full_gcode_comment ? tempDescription : "", path.is_force_no_extrusion());
                    }
                }
            } else {
                // BBS: start to generate gcode from arc fitting data which includes line and arc
                const std::vector<PathFittingData>& fitting_result = path.polyline.fitting_result;
                for (size_t fitting_index = 0; fitting_index < fitting_result.size(); fitting_index++) {
                    std::string tempDescription = description;
                    switch (fitting_result[fitting_index].path_type) {
                    case EMovePathType::Linear_move: {
                        size_t start_index = fitting_result[fitting_index].start_point_index;
                        size_t end_index = fitting_result[fitting_index].end_point_index;
                        for (size_t point_index = start_index + 1; point_index < end_index + 1; point_index++) {
                            const Line line = Line(path.polyline.points[point_index - 1], path.polyline.points[point_index]);
                            const double line_length = line.length() * SCALING_FACTOR;
                            if (line_length < EPSILON)
                                continue;
                            auto dE = e_per_mm * line_length;
                            if (m_small_area_infill_flow_compensator  && m_config.small_area_infill_flow_compensation.value) {
                                auto oldE = dE;
                                dE = m_small_area_infill_flow_compensator->modify_flow(line_length, dE, path.role());

                                if (m_config.gcode_comments && oldE > 0 && oldE != dE) {
                                    tempDescription += Slic3r::format(" | Old Flow Value: %0.5f Length: %0.5f",oldE, line_length);
                                }
                            }
                            gcode += m_writer.extrude_to_xy(
                                this->point_to_gcode(line.b),
                                dE,
                                GCodeWriter::full_gcode_comment ? tempDescription : "", path.is_force_no_extrusion());
                        }
                        break;
                    }
                    case EMovePathType::Arc_move_cw:
                    case EMovePathType::Arc_move_ccw: {
                        const ArcSegment& arc = fitting_result[fitting_index].arc_data;
                        const double arc_length = fitting_result[fitting_index].arc_data.length * SCALING_FACTOR;
                        if (arc_length < EPSILON)
                            continue;
                        const Vec2d center_offset = this->point_to_gcode(arc.center) - this->point_to_gcode(arc.start_point);
                        auto dE = e_per_mm * arc_length;
                        if (m_small_area_infill_flow_compensator && m_config.small_area_infill_flow_compensation.value) {
                            auto oldE = dE;
                            dE = m_small_area_infill_flow_compensator->modify_flow(arc_length, dE, path.role());

                            if (m_config.gcode_comments && oldE > 0 && oldE != dE) {
                                tempDescription += Slic3r::format(" | Old Flow Value: %0.5f Length: %0.5f",oldE, arc_length);
                            }
                        }
                        gcode += m_writer.extrude_arc_to_xy(
                            this->point_to_gcode(arc.end_point),
                            center_offset,
                            dE,
                            arc.direction == ArcDirection::Arc_Dir_CCW,
                            GCodeWriter::full_gcode_comment ? tempDescription : "", path.is_force_no_extrusion());
                        break;
                    }
                    default:
                        // BBS: should never happen that a empty path_type has been stored
                        assert(0);
                        break;
                    }
                }
            }
        }
    } else {
        double last_set_speed = F/*new_points[0].speed * 60.0*/;

        double total_length = 0;
        if (sloped != nullptr) {
            // Calculate total extrusion length
            Points p;
            p.reserve(new_points.size());
            std::transform(new_points.begin(), new_points.end(), std::back_inserter(p), [](const ProcessedPoint& pp) { return pp.p; });
            Polyline l(p);
            total_length = l.length() * SCALING_FACTOR;
        }
        //gcode += m_writer.set_speed(last_set_speed, "", cooling_marker_setspeed_comments);
        Vec2d prev = this->point_to_gcode_quantized(new_points[0].p);
        const bool path_fan_enabled = m_enable_cooling_markers && enable_overhang_bridge_fan && (is_bridge(path.role()) || (overhang_fan_threshold == Overhang_threshold_none && is_external_perimeter(path.role())));
        std::vector<char> overhang_fan_segments;

        if (m_enable_cooling_markers && enable_overhang_bridge_fan && !path_fan_enabled && new_points.size() > 1) {
            std::vector<double> segment_lengths(new_points.size() - 1, 0.);
            overhang_fan_segments.assign(new_points.size() - 1, false);

            const double max_gap_merge_length       = std::max(2.0, 4.0 * double(path.width));

            bool prev_point_fan = check_overhang_fan(new_points.front().overlap, path.role());
            Vec2d prev_point = this->point_to_gcode_quantized(new_points.front().p);
            for (size_t i = 1; i < new_points.size(); ++i) {
                const bool cur_point_fan = check_overhang_fan(new_points[i].overlap, path.role());
                const Vec2d cur_point = this->point_to_gcode_quantized(new_points[i].p);
                segment_lengths[i - 1] = (cur_point - prev_point).norm();
                overhang_fan_segments[i - 1] = prev_point_fan && cur_point_fan && segment_lengths[i - 1] >= EPSILON;
                prev_point = cur_point;
                prev_point_fan = cur_point_fan;
            }

            auto run_length = [&segment_lengths](size_t begin, size_t end) {
                double length = 0.;
                for (size_t i = begin; i < end; ++i)
                    length += segment_lengths[i];
                return length;
            };

            for (size_t begin = 0; begin < overhang_fan_segments.size();) {
                const bool enabled = overhang_fan_segments[begin] != 0;
                size_t end = begin + 1;
                while (end < overhang_fan_segments.size() && (overhang_fan_segments[end] != 0) == enabled)
                    ++end;
                if (!enabled && begin > 0 && end < overhang_fan_segments.size() && run_length(begin, end) <= max_gap_merge_length)
                    std::fill(overhang_fan_segments.begin() + begin, overhang_fan_segments.begin() + end, 1);
                begin = end;
            }
        }

        double path_length = 0.;
        for (size_t i = 1; i < new_points.size(); i++)
        {
            std::string tempDescription = description;
            const ProcessedPoint &processed_point = new_points[i];
            const ProcessedPoint &pre_processed_point = new_points[i-1];
            Vec2d p = this->point_to_gcode_quantized(processed_point.p);
            const double line_length = (p - prev).norm();
            if(line_length < EPSILON)
                continue;

            if (m_enable_cooling_markers) {
                if (enable_overhang_bridge_fan) {
                    const bool fan_enabled = path_fan_enabled || (!overhang_fan_segments.empty() && overhang_fan_segments[i - 1] != 0);
                    if (fan_enabled) {
                        if (!m_is_overhang_fan_on) {
                            gcode += ";_OVERHANG_FAN_START\n";
                            m_is_overhang_fan_on = true;
                        }
                    } else {
                        if (m_is_overhang_fan_on) {
                            m_is_overhang_fan_on = false;
                            gcode += ";_OVERHANG_FAN_END\n";
                        }
                    }
                }
                if (supp_interface_fan_speed >= 0 && path.role() == erSupportMaterialInterface) {
                    if (!m_is_supp_interface_fan_on) {
                        gcode += ";_SUPP_INTERFACE_FAN_START\n";
                        m_is_supp_interface_fan_on = true;
                    }
                } else {
                    if (m_is_supp_interface_fan_on) {
                        gcode += ";_SUPP_INTERFACE_FAN_END\n";
                        m_is_supp_interface_fan_on = false;
                    }
                }
            }

            path_length += line_length;
            double new_speed = pre_processed_point.speed * 60.0;
            // Ignore small speed variations - emit speed change if the delta between current and new is greater than 60mm/min / 1mm/sec
            // Reset speed to F if delta to F is less than 1mm/sec
            if ((std::abs(last_set_speed - new_speed) > 60)) {
                gcode += m_writer.set_speed(new_speed, "", cooling_marker_setspeed_comments);
                last_set_speed = new_speed;
            } else if ((std::abs(F - new_speed) <= 60)) {
                gcode += m_writer.set_speed(F, "", cooling_marker_setspeed_comments);
                last_set_speed = F;
            }

            auto dE = e_per_mm * line_length;
            if (m_small_area_infill_flow_compensator  && m_config.small_area_infill_flow_compensation.value) {
                auto oldE = dE;
                dE = m_small_area_infill_flow_compensator->modify_flow(line_length, dE, path.role());

                if (m_config.gcode_comments && oldE > 0 && oldE != dE) {
                    tempDescription += Slic3r::format(" | Old Flow Value: %0.5f Length: %0.5f",oldE, line_length);
                }
            }
            if (sloped == nullptr) {
                // Normal extrusion
                gcode += m_writer.extrude_to_xy(p, dE, GCodeWriter::full_gcode_comment ? tempDescription : "");
            } else {
                // Sloped extrusion
                const auto [z_ratio, e_ratio] = sloped->interpolate(path_length / total_length);
                Vec3d dest3d(p(0), p(1), get_sloped_z(z_ratio));
                gcode += m_writer.extrude_to_xyz(dest3d, dE * e_ratio, GCodeWriter::full_gcode_comment ? tempDescription : "");
            }

            prev = p;

        }
    }
    if (m_enable_cooling_markers) {
            gcode += ";_EXTRUDE_END\n";
    }

    if (path.role() != ExtrusionRole::erGapFill) {
      m_last_notgapfill_extrusion_role = path.role();
    }

    this->set_last_pos(path.last_point());
    return gcode;
}

//Orca: get string name of extrusion role. used for change_extruder_role_gcode
std::string GCode::extrusion_role_to_string_for_parser(const ExtrusionRole & role)
{
    switch (role) {
        case erPerimeter: return "Perimeter";
        case erExternalPerimeter: return "ExternalPerimeter";
        case erOverhangPerimeter: return "OverhangPerimeter";
        case erInternalInfill: return "InternalInfill";
        case erSolidInfill: return "SolidInfill";
        case erTopSolidInfill: return "TopSolidInfill";
        case erBottomSurface: return "BottomSurface";
        case erBridgeInfill:
        case erInternalBridgeInfill: return "BridgeInfill";
        case erGapFill: return "GapFill";
        case erIroning: return "Ironing";
        case erSkirt: return "Skirt";
        case erBrim: return "Brim";
        case erSupportMaterial: return "SupportMaterial";
        case erSupportMaterialInterface: return "SupportMaterialInterface";
        case erSupportTransition: return "SupportTransition";
        case erWipeTower: return "WipeTower";
        case erSkinInfill: return "SkinInfill";
        case erCustom:
        case erMixed:
        case erCount:
        case erNone:
        default: return "Mixed";
    }
}

std::string encodeBase64(uint64_t value)
{
    //Always use big endian mode
    uint8_t src[8];
    for (size_t i = 0; i < 8; i++)
        src[i] = (value >> (8 * i)) & 0xff;

    std::string dest;
    dest.resize(boost::beast::detail::base64::encoded_size(sizeof(src)));
    dest.resize(boost::beast::detail::base64::encode(&dest[0], src, sizeof(src)));
    return dest;
}

std::string GCode::_encode_label_ids_to_base64(std::vector<size_t> ids)
{
    assert(m_label_objects_ids.size() < 64);

    uint64_t bitset = 0;
    for (size_t id : ids) {
        auto index = std::lower_bound(m_label_objects_ids.begin(), m_label_objects_ids.end(), id);
        if (index != m_label_objects_ids.end() && *index == id)
            bitset |= (1ull << (index - m_label_objects_ids.begin()));
        else
            throw Slic3r::LogicError("Unknown label object id!");
    }
    if (bitset == 0)
        throw Slic3r::LogicError("Label object id error!");

    return encodeBase64(bitset);
}

std::string GCode::set_wipe_tower_print_acceleration()
{
    if (m_config.default_acceleration.value <= 0)
        return std::string();

    double acceleration = m_config.default_acceleration.value;
    if (this->on_first_layer() && m_config.initial_layer_acceleration.value > 0) {
        acceleration = m_config.initial_layer_acceleration.value;
    } else if (m_config.top_surface_acceleration.value > 0 && is_top_surface(m_last_notgapfill_extrusion_role)) {
        acceleration = m_config.top_surface_acceleration.value;
    }

    if (m_config.acceleration_limit_mess_enable || m_config.speed_limit_to_height_enable) {
        double weight = DoExport::update_total_weight(m_writer.extruders());
        m_smoothSpeedAcc->detect_acc(acceleration, weight, m_last_layer_z);
    }

    return m_writer.set_print_acceleration((unsigned int) floor(acceleration + 0.5));
}

std::string GCode::inject_wipe_tower_print_acceleration(std::string tower_gcode)
{
    std::string acceleration_gcode = this->set_wipe_tower_print_acceleration();
    if (acceleration_gcode.empty())
        return tower_gcode;

    const std::string marker     = "; CP TOOLCHANGE WIPE";
    size_t            marker_pos = tower_gcode.find(marker);
    if (marker_pos != std::string::npos)
        tower_gcode.insert(marker_pos, acceleration_gcode);

    return tower_gcode;
}

// This method accepts &point in print coordinates.
std::string GCode::travel_to(const Point& point, ExtrusionRole role, std::string comment, double z/* = DBL_MAX*/)
{
    /*  Define the travel move as a line between current position and the taget point.
        This is expressed in print coordinates, so it will need to be translated by
        this->origin in order to get G-code coordinates.  */
    Polyline travel { this->last_pos(), point };

    // check whether a straight travel move would need retraction
    LiftType lift_type = LiftType::SpiralLift;
    bool needs_retraction = this->needs_retraction(travel, role, lift_type);
    // check whether wipe could be disabled without causing visible stringing
    bool could_be_wipe_disabled       = false;
    // Save state of use_external_mp_once for the case that will be needed to call twice m_avoid_crossing_perimeters.travel_to.
    const bool used_external_mp_once  = m_avoid_crossing_perimeters.used_external_mp_once();
    std::string gcode;

    // Orca: we don't need to optimize the Klipper as only set once
    double jerk_to_set = 0.0;
    unsigned int acceleration_to_set = 0;
    if (this->on_first_layer()) {
        unsigned int initial_layer_travel_acceleration = 0;
        if (!m_config.initial_layer_travel_acceleration.values.empty()) {
            const size_t extruder_id = m_writer.extruder() ? m_writer.extruder()->id() : 0;
            const size_t acceleration_idx = std::min(extruder_id, m_config.initial_layer_travel_acceleration.values.size() - 1);
            if (!m_config.initial_layer_travel_acceleration.is_nil(acceleration_idx) &&
                m_config.initial_layer_travel_acceleration.values[acceleration_idx] > 0) {
                initial_layer_travel_acceleration =
                    (unsigned int) floor(m_config.initial_layer_travel_acceleration.values[acceleration_idx] + 0.5);
            }
        }

        if (m_config.default_acceleration.value > 0) {
            if (initial_layer_travel_acceleration > 0) {
                acceleration_to_set = initial_layer_travel_acceleration;
            } else if (m_config.initial_layer_acceleration.value > 0) {
                acceleration_to_set = (unsigned int) floor(m_config.initial_layer_acceleration.value + 0.5);
            }
        }
        if (m_config.default_jerk.value > 0 && m_config.initial_layer_jerk.value > 0) {
            jerk_to_set = m_config.initial_layer_jerk.value;
        }
    } else {
        if (m_config.default_acceleration.value > 0 && m_config.travel_acceleration.value > 0) {
            acceleration_to_set = (unsigned int) floor(m_config.travel_acceleration.value + 0.5);
        }
        if (m_config.default_jerk.value > 0 && m_config.travel_jerk.value > 0) {
            jerk_to_set = m_config.travel_jerk.value;
        }
    }


    if (m_config.acceleration_limit_mess_enable
        || m_config.speed_limit_to_height_enable)
    {
        double weight = 0.0f;
        weight = DoExport::update_total_weight(m_writer.extruders());
        if (m_config.acceleration_limit_mess_enable
            || m_config.speed_limit_to_height_enable)
        {
            double acc = acceleration_to_set;
            m_smoothSpeedAcc->detect_acc(acc, weight, m_last_layer_z);
            acceleration_to_set = acc;
        }
    }
    double limitSpeed = getLimitSpeed();

    // if a retraction would be needed, try to use reduce_crossing_wall to plan a
    // multi-hop travel path inside the configuration space
    if (needs_retraction
        && m_config.reduce_crossing_wall
        && ! m_avoid_crossing_perimeters.disabled_once()
        //BBS: don't generate detour travel paths when current position is unclear
        && m_writer.is_current_position_clear()) {
        travel = m_avoid_crossing_perimeters.travel_to(*this, point, &could_be_wipe_disabled);
        // check again whether the new travel path still needs a retraction
        needs_retraction = this->needs_retraction(travel, role, lift_type);
        //if (needs_retraction && m_layer_index > 1) exit(0);
    }

    // Re-allow reduce_crossing_wall for the next travel moves
    m_avoid_crossing_perimeters.reset_once_modifiers();

    std::string wipe_retract_gcode{};
    // generate G-code for the travel move
    if (needs_retraction) {
        // ORCA: Fix scenario where wipe is disabled when avoid crossing perimeters was enabled even though a retraction move was performed.
        // This replicates the existing behaviour of always wiping when retracting
        /*if (m_config.reduce_crossing_wall && could_be_wipe_disabled)
            m_wipe.reset_path();*/

        if ((m_last_extrusion_role == erInternalInfill || m_last_extrusion_role == erSkinInfill) &&
            (is_perimeter(role) || role == erWipeTower || role == erMixed || role == erNone) &&
            !m_flush_into_skeleton_tail_wipe_enabled)
            m_wipe.reset_path();

        Point last_post_before_retract = this->last_pos();
        gcode += this->retract(false, false, lift_type);
        //wipe_retract_gcode = this->retract(false, false, lift_type);
        // When "Wipe while retracting" is enabled, then extruder moves to another position, and travel from this position can cross perimeters.
        // Because of it, it is necessary to call avoid crossing perimeters again with new starting point after calling retraction()
        // FIXME Lukas H.: Try to predict if this second calling of avoid crossing perimeters will be needed or not. It could save computations.
        if (last_post_before_retract != this->last_pos() && m_config.reduce_crossing_wall) {
            // If in the previous call of m_avoid_crossing_perimeters.travel_to was use_external_mp_once set to true restore this value for next call.
            if (used_external_mp_once)
                m_avoid_crossing_perimeters.use_external_mp_once();
            travel = m_avoid_crossing_perimeters.travel_to(*this, point);
            // If state of use_external_mp_once was changed reset it to right value.
            if (used_external_mp_once)
                m_avoid_crossing_perimeters.reset_once_modifiers();
        }
    } else
        // Reset the wipe path when traveling, so one would not wipe along an old path.
        m_wipe.reset_path();

    // if needed, write the gcode_label_objects_end then gcode_label_objects_start
    m_writer.add_object_change_labels(gcode);

    bool use_short_distance_acceleration =
        is_external_perimeter(role) &&
        travel.length() < scaled<double>(m_config.travel_short_distance_threshold.value); // Prusa used VFA, -travel
                                                                                                       // short distance acceleration
    const unsigned travel_acceleration                = static_cast<unsigned>(m_config.travel_acceleration.value + 0.5);
    const unsigned travel_short_distance_acceleration = static_cast<unsigned>(m_config.travel_short_distance_acceleration.value + 0.5);

    if (this->config().travel_short_distance_acceleration > 0. &&
        use_short_distance_acceleration) // Prusa used VFA, -travel short distance acceleration
        acceleration_to_set = travel_short_distance_acceleration;

    const bool auto_travel_acceleration_was_suppressed = m_writer.auto_travel_acceleration_suppressed();
    m_writer.set_auto_travel_acceleration_override(acceleration_to_set);
    if (!auto_travel_acceleration_was_suppressed) {
        if (m_writer.get_gcode_flavor() == gcfKlipper) {
            m_writer.set_auto_travel_acceleration_suppressed(true);
            gcode += m_writer.set_accel_and_jerk(acceleration_to_set, jerk_to_set);
        } else {
            gcode += m_writer.set_jerk_xy(jerk_to_set);
        }
    }

    // use G1 because we rely on paths being straight (G0 may make round paths)
    if (travel.size() >= 2) {
        if (m_spiral_vase) {
            // No lazy z lift for spiral vase mode
            for (size_t i = 1; i < travel.size(); ++i) {
                gcode += m_writer.travel_to_xy(this->point_to_gcode(travel.points[i]), comment + " travel_to_xy", limitSpeed);
            }
        } else {
            if (travel.size() == 2) {
                // No extra movements emitted by avoid_crossing_perimeters, simply move to the end point with z change
                const auto& dest2d = this->point_to_gcode(travel.points.back());
                Vec3d dest3d(dest2d(0), dest2d(1), z == DBL_MAX ? m_nominal_z : z);
                gcode += m_writer.travel_to_xyz(dest3d, comment + " travel_to_xyz", limitSpeed);
            } else {
                // Extra movements emitted by avoid_crossing_perimeters, lift the z to normal height at the beginning, then apply the z
                // ratio at the last point
                for (size_t i = 1; i < travel.size(); ++i) {
                    if (i == 1) {
                        // Lift to normal z at beginning
                        Vec2d dest2d = this->point_to_gcode(travel.points[i]);
                        Vec3d dest3d(dest2d(0), dest2d(1), m_nominal_z);
                        gcode += m_writer.travel_to_xyz(dest3d, comment + " travel_to_xyz", limitSpeed);
                    } else if (z != DBL_MAX && i == travel.size() - 1) {
                        // Apply z_ratio for the very last point
                        Vec2d dest2d = this->point_to_gcode(travel.points[i]);
                        Vec3d dest3d(dest2d(0), dest2d(1), z);
                        gcode += m_writer.travel_to_xyz(dest3d, comment + " travel_to_xyz", limitSpeed);
                    } else {
                        // For all points in between, no z change
                        gcode += m_writer.travel_to_xy(this->point_to_gcode(travel.points[i]), comment + " travel_to_xy", limitSpeed);
                    }
                }
            }
        }
        this->set_last_pos(travel.points.back());
    }

    //// Prusa used VFA, -travel short distance acceleration
    //if (this->config().travel_short_distance_acceleration > 0.) {
    //    // This is mainly for parts of the G-code export that don't take into account that travel acceleration could change during printing.
    //    // Those parts of the G-code export always use the travel acceleration that was set last.
    //    if (use_short_distance_acceleration && travel_short_distance_acceleration != travel_acceleration) {
    //        gcode += this->m_writer.set_travel_acceleration(travel_acceleration);
    //    }

    //    if (!GCodeWriter::supports_separate_travel_acceleration(config().gcode_flavor)) {
    //        // In case that this flavor does not support separate print and travel acceleration,
    //        // reset acceleration to default.
    //        // TODO: This doesn't seem to perform what the comment describes.
    //        gcode += this->m_writer.set_travel_acceleration(travel_acceleration);
    //    }
    //}


    m_writer.set_auto_travel_acceleration_suppressed(auto_travel_acceleration_was_suppressed);
    m_writer.clear_auto_travel_acceleration_override();

    return gcode;
}

//BBS
LiftType GCode::to_lift_type(ZHopType z_hop_types) {
    switch (z_hop_types)
    {
    case ZHopType::zhtNormal:
        return LiftType::NormalLift;
    case ZHopType::zhtSlope:
        return LiftType::LazyLift;
    case ZHopType::zhtSpiral:
        return LiftType::SpiralLift;
    default:
        // if no corresponding lift type, use normal lift
        return LiftType::NormalLift;
    }
};

bool GCode::needs_retraction(const Polyline &travel, ExtrusionRole role, LiftType& lift_type)
{
    if (travel.length() < scale_(EXTRUDER_CONFIG(retraction_minimum_travel))) {
        // skip retraction if the move is shorter than the configured threshold
        return false;
    }

    //BBS: input travel polyline must be in current plate coordinate system
    auto is_through_overhang = [this](const Polyline& travel) {
        BoundingBox travel_bbox = get_extents(travel);
        travel_bbox.inflated(1);
        travel_bbox.defined = true;

        // do not scale for z
        const float protect_z = 0.4;
        std::pair<float, float> z_range;
        z_range.second = m_layer ? m_layer->print_z : 0.f;
        z_range.first = std::max(0.f, z_range.second - protect_z);
        std::vector<LayerPtrs> layers_of_objects;
        std::vector<BoundingBox> boundingBox_for_objects;
        std::vector<Points> objects_instances_shift;
        std::vector<size_t> idx_of_object_sorted = m_curr_print->layers_sorted_for_object(z_range.first, z_range.second, layers_of_objects, boundingBox_for_objects, objects_instances_shift);

        std::vector<bool> is_layers_of_objects_sorted(layers_of_objects.size(), false);

        for (size_t idx : idx_of_object_sorted) {
            for (const Point & instance_shift : objects_instances_shift[idx]) {
                BoundingBox instance_bbox = boundingBox_for_objects[idx];
                if (!instance_bbox.defined)  //BBS: Don't need to check when bounding box of overhang area is empty(undefined)
                    continue;

                instance_bbox.offset(scale_(EPSILON));
                instance_bbox.translate(instance_shift.x(), instance_shift.y());
                if (!instance_bbox.overlap(travel_bbox))
                    continue;

                Polygons temp;
                temp.emplace_back(std::move(instance_bbox.polygon()));
                if (intersection_pl(travel, temp).empty())
                    continue;

                if (!is_layers_of_objects_sorted[idx]) {
                    std::sort(layers_of_objects[idx].begin(), layers_of_objects[idx].end(), [](auto left, auto right) { return left->loverhangs_bbox.area() > right->loverhangs_bbox.area();});
                    is_layers_of_objects_sorted[idx] = true;
                }

                for (const auto& layer : layers_of_objects[idx]) {
                    for (ExPolygon overhang : layer->loverhangs) {
                        overhang.translate(instance_shift);
                        BoundingBox bbox1 = get_extents(overhang);

                        if (!bbox1.overlap(travel_bbox))
                            continue;

                        if (intersection_pl(travel, overhang).empty())
                            continue;

                        return true;
                    }
                }
            }
        }
        return false;
    };

    float max_z_hop = 0.f;
    for (int i = 0; i < m_config.z_hop.size(); i++)
        max_z_hop = std::max(max_z_hop, (float)m_config.z_hop.get_at(i));
    float travel_len_thresh = scale_(max_z_hop / tan(this->writer().extruder()->travel_slope()));
    float accum_len = 0.f;
    Polyline clipped_travel;

    clipped_travel.append(Polyline(travel.points[0], travel.points[1]));
    if (clipped_travel.length() > travel_len_thresh)
        clipped_travel.points.back() = clipped_travel.points.front()+(clipped_travel.points.back() - clipped_travel.points.front()) * (travel_len_thresh / clipped_travel.length());
    //BBS: translate to current plate coordinate system
    clipped_travel.translate(Point::new_scale(double(m_origin.x() - m_writer.get_xy_offset().x()), double(m_origin.y() - m_writer.get_xy_offset().y())));

    //BBS: force to retract when leave from external perimeter for a long travel
    //Better way is judging whether the travel move direction is same with last extrusion move.
    if (is_perimeter(m_last_processor_extrusion_role) && m_last_processor_extrusion_role != erPerimeter) {
        if (ZHopType(EXTRUDER_CONFIG(z_hop_types)) == ZHopType::zhtAuto) {
            lift_type = is_through_overhang(clipped_travel) ? LiftType::SpiralLift : LiftType::LazyLift;
        }
        else {
            lift_type = to_lift_type(ZHopType(EXTRUDER_CONFIG(z_hop_types)));
        }
        return true;
    }

    if (role == erSupportMaterial || role == erSupportTransition) {
        const SupportLayer* support_layer = dynamic_cast<const SupportLayer*>(m_layer);
        //FIXME support_layer->support_islands.contains should use some search structure!
        if (support_layer != NULL)
            // skip retraction if this is a travel move inside a support material island
            //FIXME not retracting over a long path may cause oozing, which in turn may result in missing material
            // at the end of the extrusion path!
            for (const ExPolygon& support_island : support_layer->support_islands)
                if (support_island.contains(travel))
                    return false;
        //reduce the retractions in lightning infills for tree support
        if (support_layer != NULL && support_layer->support_type==stInnerTree)
            for (auto &area : support_layer->base_areas)
                if (area.contains(travel))
                    return false;
    }
    //BBS: need retract when long moving to print perimeter to avoid dropping of material
    if (!is_perimeter(role) && m_config.reduce_infill_retraction && m_layer != nullptr &&
        m_config.sparse_infill_density.value > 0 && m_retract_when_crossing_perimeters.travel_inside_internal_regions(*m_layer, travel))
        // Skip retraction if travel is contained in an internal slice *and*
        // internal infill is enabled (so that stringing is entirely not visible).
        //FIXME any_internal_region_slice_contains() is potentionally very slow, it shall test for the bounding boxes first.
        return false;

    // retract if reduce_infill_retraction is disabled or doesn't apply when role is perimeter
    if (ZHopType(EXTRUDER_CONFIG(z_hop_types)) == ZHopType::zhtAuto) {
        lift_type = is_through_overhang(clipped_travel) ? LiftType::SpiralLift : LiftType::LazyLift;
    }
    else {
        lift_type = to_lift_type(ZHopType(EXTRUDER_CONFIG(z_hop_types)));
    }
    return true;
}

std::string GCode::retract(bool toolchange, bool is_last_retraction, LiftType lift_type)
{
    double limitSpeed = getLimitSpeed();

    std::string gcode;

    if (m_writer.extruder() == nullptr)
        return gcode;

    // wipe (if it's enabled for this extruder and we have a stored wipe path and no-zero wipe distance)
    if (EXTRUDER_CONFIG(wipe) && m_wipe.has_path() && scale_(EXTRUDER_CONFIG(wipe_distance)) > SCALED_EPSILON) {
        Wipe::RetractionValues wipeRetractions = m_wipe.calculateWipeRetractionLengths(*this, toolchange);
        gcode += toolchange ? m_writer.retract_for_toolchange(true,wipeRetractions.retractLengthBeforeWipe) : m_writer.retract(true, wipeRetractions.retractLengthBeforeWipe);
        gcode += m_wipe.wipe(*this,wipeRetractions.retractLengthDuringWipe, toolchange, is_last_retraction);
    }

    /*  The parent class will decide whether we need to perform an actual retraction
        (the extruder might be already retracted fully or partially). We call these
        methods even if we performed wipe, since this will ensure the entire retraction
        length is honored in case wipe path was too short.  */
    gcode += toolchange ? m_writer.retract_for_toolchange() : m_writer.retract();

    gcode += m_writer.reset_e();
    // Orca: check if should + can lift (roughly from SuperSlicer)
    RetractLiftEnforceType retract_lift_type = RetractLiftEnforceType(EXTRUDER_CONFIG(retract_lift_enforce));

    bool needs_lift = toolchange
        || m_writer.extruder()->retraction_length() > 0
        || m_config.use_firmware_retraction;

    bool last_fill_extrusion_role_top_infill = (this->m_last_notgapfill_extrusion_role == ExtrusionRole::erTopSolidInfill || this->m_last_notgapfill_extrusion_role == ExtrusionRole::erIroning);

    // assume we can lift on retraction; conditions left explicit
    bool can_lift = true;

    if (retract_lift_type == RetractLiftEnforceType::rletAllSurfaces) {
        can_lift = true;
    }
    else if (this->m_layer_index == 0 && (retract_lift_type == RetractLiftEnforceType::rletBottomOnly || retract_lift_type == RetractLiftEnforceType::rletTopAndBottom)) {
        can_lift = true;
    }
    else if (retract_lift_type == RetractLiftEnforceType::rletTopOnly || retract_lift_type == RetractLiftEnforceType::rletTopAndBottom) {
        can_lift = last_fill_extrusion_role_top_infill;
    }
    else {
        can_lift = false;
    }

    if (needs_lift && can_lift) {
        size_t extruder_id = m_writer.extruder()->id();
        gcode += m_writer.lift(!m_spiral_vase ? lift_type : LiftType::NormalLift,false, limitSpeed);
    }

    return gcode;
}

void GCode::set_tower_pos(Vec2d _pos)
{
     m_powerPos = _pos;
}

#ifdef SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
void GCode::set_toolchange_source_object(const PrintObject *print_object)
{
    m_toolchange_source_object = print_object;
}

void GCode::set_toolchange_target_object(const PrintObject *print_object)
{
    m_toolchange_target_object = print_object;
}

float GCode::effective_retraction_distance_when_cut(unsigned int fallback_extruder_id) const
{
    const float fallback = m_config.retraction_distances_when_cut.get_at(fallback_extruder_id);
    if (m_toolchange_source_object == nullptr)
        return fallback;

    const PrintObjectConfig &object_config = m_toolchange_source_object->config();
    if (!object_config.enable_retraction_distance_when_cut_override.value)
        return fallback;

    return float(object_config.retraction_distance_when_cut_override.value);
}

ConfigOptionFloats GCode::effective_retraction_distances_when_cut(unsigned int fallback_extruder_id) const
{
    ConfigOptionFloats values(m_config.retraction_distances_when_cut);
    if (fallback_extruder_id < values.values.size())
        values.values[fallback_extruder_id] = this->effective_retraction_distance_when_cut(fallback_extruder_id);
    return values;
}
#endif // SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT

int get_random_unique_0_80()
{
    static std::vector<int> numbers;
    static size_t           index = 0;
    static std::mt19937     g(std::random_device{}()); // ?????

    if (numbers.empty()) {
        numbers.reserve(81);
        for (int i = 0; i <= 80; ++i) {
            numbers.push_back(i);
        }
        std::shuffle(numbers.begin(), numbers.end(), g);
    }

    // ?????????????,????
    int value = numbers[index % numbers.size()];
    ++index;
    return value;
}

static size_t find_flush_start_line_start(const std::string& gcode);
static size_t find_flush_end_line_after(const std::string& gcode);
static size_t find_skeleton_flush_remaining_flush_line_start(const std::string& gcode);
static bool parse_axis_value(const std::string& line, size_t end, char axis, double& value);
static std::string last_primary_fan_command(const std::string& gcode);
static bool split_last_toolchange_line_with_suffix(const std::string& gcode, unsigned int extruder_id, std::string& before_toolchange, std::string& toolchange_line, std::string& after_toolchange);
static double retracted_after_custom_gcode(const std::string& gcode, double e_position, double retracted, bool relative_e, bool* saw_e_move = nullptr);
static void sync_writer_extruder_retraction_after_custom_gcode(GCodeWriter& writer, const std::string& custom_gcode, bool reset_retracted = false);

static Vec3d position_after_custom_gcode(const std::string& gcode, Vec3d position, const Vec2d& xy_offset = Vec2d::Zero());

std::string GCode::set_extruder_new(unsigned int extruder_id,
                                    double       print_z,
                                    float        trc_wipe_volume,
                                    float        wipe_pos_x,
                                    float        wipe_pos_y,
                                    float        max_wipe_x,
                                    float        max_wipe_y,
                                    bool         by_object  ,
                                    bool         change_tool)
{
    if (!m_writer.need_toolchange(extruder_id))
        return "";

    // if we are running a single-extruder setup, just set the extruder and return nothing
    if (!m_writer.multiple_extruders) {
        this->placeholder_parser().set("current_extruder", extruder_id);
        this->placeholder_parser().set("retraction_distance_when_cut", m_config.retraction_distances_when_cut.get_at(extruder_id));
        this->placeholder_parser().set("long_retraction_when_cut", m_config.long_retractions_when_cut.get_at(extruder_id));

        std::string gcode;
        // Append the filament start G-code.
        const std::string& filament_start_gcode = m_config.filament_start_gcode.get_at(extruder_id);
        if (!filament_start_gcode.empty()) {
            // Process the filament_start_gcode for the filament.
            DynamicConfig config;
            config.set_key_value("layer_num", new ConfigOptionInt(m_layer_index));
            config.set_key_value("layer_z", new ConfigOptionFloat(this->writer().get_position().z() - m_config.z_offset.value));
            config.set_key_value("max_layer_z", new ConfigOptionFloat(m_max_layer_z));
            config.set_key_value("filament_extruder_id", new ConfigOptionInt(int(extruder_id)));
            config.set_key_value("retraction_distance_when_cut",
                                 new ConfigOptionFloat(m_config.retraction_distances_when_cut.get_at(extruder_id)));
            config.set_key_value("long_retraction_when_cut", new ConfigOptionBool(m_config.long_retractions_when_cut.get_at(extruder_id)));

            // std::vector<double> _position(3,0);
            //_position[2] = print_z;
            // config.set_key_value("position", new ConfigOptionFloats(_position));
            gcode += this->placeholder_parser_process("filament_start_gcode", filament_start_gcode, extruder_id, &config);
            check_add_eol(gcode);
        }
        if (m_config.enable_pressure_advance.get_at(extruder_id)) {
            gcode += m_writer.set_pressure_advance(m_config.pressure_advance.get_at(extruder_id));
        }

        gcode += m_writer.toolchange(extruder_id, change_tool);
        m_pending_skeleton_flush_wipe_wall_box_valid = false;
        m_pending_skeleton_flush_wipe_wall_avoid_box_valid = false;
        return gcode;
    }

    // BBS. Should be placed before retract.
    m_toolchange_count++;

    // prepend retraction on the current extruder
    this->retract(true, false);
    std::string gcode;

    // Always reset the extrusion path, even if the tool change retract is set to zero.
    m_wipe.reset_path();

    // BBS: insert skip object label before change filament while by object
    if (by_object)
        m_writer.add_object_change_labels(gcode);

    if (m_writer.extruder() != nullptr) {
        // Process the custom filament_end_gcode. set_extruder() is only called if there is no wipe tower
        // so it should not be injected twice.
        unsigned int       old_extruder_id    = m_writer.extruder()->id();
        const std::string& filament_end_gcode = m_config.filament_end_gcode.get_at(old_extruder_id);
        if (!filament_end_gcode.empty()) {
            DynamicConfig config;
            config.set_key_value("layer_num", new ConfigOptionInt(m_layer_index));
            config.set_key_value("layer_z", new ConfigOptionFloat(m_writer.get_position().z() - m_config.z_offset.value));
            config.set_key_value("max_layer_z", new ConfigOptionFloat(m_max_layer_z));
            config.set_key_value("filament_extruder_id", new ConfigOptionInt(int(old_extruder_id)));
            gcode += placeholder_parser_process("filament_end_gcode", filament_end_gcode, old_extruder_id, &config);
            check_add_eol(gcode);
        }
    }

    // If ooze prevention is enabled, park current extruder in the nearest
    // standby point and set it to the standby temperature.
    if (m_ooze_prevention.enable && m_writer.extruder() != nullptr)
        gcode += m_ooze_prevention.pre_toolchange(*this);

    // BBS
    float new_retract_length            = m_config.retraction_length.get_at(extruder_id);
    float new_retract_length_toolchange = m_config.retract_length_toolchange.get_at(extruder_id);
    int   new_filament_temp             = this->on_first_layer() ? m_config.nozzle_temperature_initial_layer.get_at(extruder_id) :
                                                                   m_config.nozzle_temperature.get_at(extruder_id);
    // BBS: if print_z == 0 use first layer temperature
    if (abs(print_z) < EPSILON)
        new_filament_temp = m_config.nozzle_temperature_initial_layer.get_at(extruder_id);

    Vec3d nozzle_pos = m_writer.get_position();
    float old_retract_length, old_retract_length_toolchange, wipe_volume;
    int   old_filament_temp, old_filament_e_feedrate;

    const float filament_diameter = m_config.filament_diameter.get_at(extruder_id);
    float filament_area = float((M_PI / 4.f) * filament_diameter * filament_diameter);
    // BBS: add handling for filament change in start gcode
    int previous_extruder_id = -1;
    if (m_writer.extruder() != nullptr || m_start_gcode_filament != -1) {
        const auto& flush_matrix = m_config.flush_volumes_matrix.values;
        const unsigned int number_of_extruders = (unsigned int) (sqrt(flush_matrix.size()) + EPSILON);
        if (m_writer.extruder() != nullptr)
            assert(m_writer.extruder()->id() < number_of_extruders);
        else
            assert(m_start_gcode_filament < number_of_extruders);

        previous_extruder_id          = m_writer.extruder() != nullptr ? m_writer.extruder()->id() : m_start_gcode_filament;
        old_retract_length            = m_config.retraction_length.get_at(previous_extruder_id);
        old_retract_length_toolchange = m_config.retract_length_toolchange.get_at(previous_extruder_id);
        old_filament_temp             = this->on_first_layer() ? m_config.nozzle_temperature_initial_layer.get_at(previous_extruder_id) :
                                                                 m_config.nozzle_temperature.get_at(previous_extruder_id);
        if (!m_config.purge_in_prime_tower || is_BBL_Printer()) {
            //wipe_volume = flush_matrix[previous_extruder_id * number_of_extruders + extruder_id];
            //wipe_volume *= m_config.flush_multiplier;
            wipe_volume = trc_wipe_volume;
        } else {
            wipe_volume = 0;
            // std::max<float>(m_config.prime_volume, m_config.filament_minimal_purge_on_wipe_tower.get_at(previous_extruder_id));
        }

        //wipe_volume             = trc_wipe_volume;
        old_filament_e_feedrate = (int) (60.0 * m_config.filament_max_volumetric_speed.get_at(previous_extruder_id) / filament_area);
        old_filament_e_feedrate = old_filament_e_feedrate == 0 ? 100 : old_filament_e_feedrate;
        // BBS: must clean m_start_gcode_filament
        m_start_gcode_filament = -1;
    } else {
        old_retract_length            = 0.f;
        old_retract_length_toolchange = 0.f;
        old_filament_temp             = 0;
        wipe_volume                   = 0.f;
        old_filament_e_feedrate       = 200;
    }
#ifdef SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
    const unsigned int cut_retraction_extruder_id = previous_extruder_id >= 0 ? (unsigned int)previous_extruder_id : extruder_id;
    const float effective_cut_retraction_distance = this->effective_retraction_distance_when_cut(cut_retraction_extruder_id);
    ConfigOptionFloats effective_cut_retraction_distances = this->effective_retraction_distances_when_cut(cut_retraction_extruder_id);
#endif // SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
    // wipe_volume       = 960;
    /*if (wipe_volume > 1500)
    {
        wipe_volume = 1500;
    }*/
    float wipe_length = wipe_volume / filament_area;

    int new_filament_e_feedrate = (int) (60.0 * m_config.filament_max_volumetric_speed.get_at(extruder_id) / filament_area);
    new_filament_e_feedrate     = new_filament_e_feedrate == 0 ? 100 : new_filament_e_feedrate;

    const int parser_previous_extruder_id = m_writer.extruder() != nullptr ? int(m_writer.extruder()->id()) : -1;
    const bool is_previous_extruder_last_use = parser_previous_extruder_id >= 0 &&
                                               parser_previous_extruder_id != int(extruder_id) &&
                                               static_cast<size_t>(parser_previous_extruder_id) < m_remaining_extruder_segment_uses.size() &&
                                               m_remaining_extruder_segment_uses[static_cast<size_t>(parser_previous_extruder_id)] == 0;
    DynamicConfig dyn_config;
    dyn_config.set_key_value("previous_extruder", new ConfigOptionInt(previous_extruder_id));
    dyn_config.set_key_value("next_extruder", new ConfigOptionInt((int) extruder_id));
    dyn_config.set_key_value("is_previous_extruder_last_use", new ConfigOptionBool(is_previous_extruder_last_use));
    dyn_config.set_key_value("layer_num", new ConfigOptionInt(m_layer_index));
    dyn_config.set_key_value("layer_z", new ConfigOptionFloat(print_z));
    dyn_config.set_key_value("max_layer_z", new ConfigOptionFloat(m_max_layer_z));
    dyn_config.set_key_value("relative_e_axis", new ConfigOptionBool(m_config.use_relative_e_distances));
    dyn_config.set_key_value("toolchange_count", new ConfigOptionInt((int) m_toolchange_count));
    // BBS: fan speed is useless placeholer now, but we don't remove it to avoid
    // slicing error in old change_filament_gcode in old 3MF
    dyn_config.set_key_value("fan_speed", new ConfigOptionInt((int) 0));
    dyn_config.set_key_value("flush_into_skeleton", new ConfigOptionBool(false));

    dyn_config.set_key_value("old_retract_length", new ConfigOptionFloat(old_retract_length));
    dyn_config.set_key_value("new_retract_length", new ConfigOptionFloat(new_retract_length));
    dyn_config.set_key_value("old_retract_length_toolchange", new ConfigOptionFloat(old_retract_length_toolchange));
    dyn_config.set_key_value("new_retract_length_toolchange", new ConfigOptionFloat(new_retract_length_toolchange));
    dyn_config.set_key_value("old_filament_temp", new ConfigOptionInt(old_filament_temp));
    dyn_config.set_key_value("new_filament_temp", new ConfigOptionInt(new_filament_temp));
    dyn_config.set_key_value("x_after_toolchange", new ConfigOptionFloat(nozzle_pos(0)));
    dyn_config.set_key_value("y_after_toolchange", new ConfigOptionFloat(nozzle_pos(1)));
    dyn_config.set_key_value("z_after_toolchange", new ConfigOptionFloat(nozzle_pos(2)));
    dyn_config.set_key_value("first_flush_volume", new ConfigOptionFloat(wipe_length / 2.f));
    dyn_config.set_key_value("second_flush_volume", new ConfigOptionFloat(wipe_length / 2.f));
    dyn_config.set_key_value("old_filament_e_feedrate", new ConfigOptionInt(old_filament_e_feedrate));
    dyn_config.set_key_value("new_filament_e_feedrate", new ConfigOptionInt(new_filament_e_feedrate));
    dyn_config.set_key_value("travel_point_1_x", new ConfigOptionFloat(float(travel_point_1.x())));
    dyn_config.set_key_value("travel_point_1_y", new ConfigOptionFloat(float(travel_point_1.y())));
    dyn_config.set_key_value("travel_point_2_x", new ConfigOptionFloat(float(travel_point_2.x())));
    dyn_config.set_key_value("travel_point_2_y", new ConfigOptionFloat(float(travel_point_2.y())));
    dyn_config.set_key_value("travel_point_3_x", new ConfigOptionFloat(float(travel_point_3.x())));
    dyn_config.set_key_value("travel_point_3_y", new ConfigOptionFloat(float(travel_point_3.y())));
#ifdef SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
    dyn_config.set_key_value("retraction_distance_when_cut", new ConfigOptionFloat(effective_cut_retraction_distance));
    dyn_config.set_key_value("retraction_distances_when_cut", new ConfigOptionFloats(effective_cut_retraction_distances));
    dyn_config.set_key_value("long_retraction_when_cut", new ConfigOptionBool(m_config.long_retractions_when_cut.get_at(cut_retraction_extruder_id)));
#endif // SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
    dyn_config.set_key_value("wipe_tower_start_position_x", new ConfigOptionFloat(float(wipe_pos_x)));
    dyn_config.set_key_value("wipe_tower_start_position_y", new ConfigOptionFloat(float(wipe_pos_y)));
    int num = get_random_unique_0_80();
    dyn_config.set_key_value("wipe_tower_random_num", new ConfigOptionFloat(float(num / 80.0)));
    dyn_config.set_key_value("wipe_tower_outer_wall_x", new ConfigOptionFloat(max_wipe_x));
    dyn_config.set_key_value("wipe_tower_outer_wall_y", new ConfigOptionFloat(max_wipe_y));


    dyn_config.set_key_value("flush_length", new ConfigOptionFloat(wipe_length));
    /*
    int flush_count = std::min(g_max_flush_count, (int)std::round(wipe_volume / g_purge_volume_one_time));
    float flush_unit = wipe_length / flush_count;
    int flush_idx = 0;
    for (; flush_idx < flush_count; flush_idx++) {
        char key_value[64] = { 0 };
        snprintf(key_value, sizeof(key_value), "flush_length_%d", flush_idx + 1);
        dyn_config.set_key_value(key_value, new ConfigOptionFloat(flush_unit));
    }

    for (; flush_idx < g_max_flush_count; flush_idx++) {
        char key_value[64] = { 0 };
        snprintf(key_value, sizeof(key_value), "flush_length_%d", flush_idx + 1);
        dyn_config.set_key_value(key_value, new ConfigOptionFloat(0.f));
    }*/
    FlushConfig fig;
    fig.box_first_clean_length    = std::max(1, m_config.flush_box_first_clean_length.value);
    fig.box_need_clean_length     = std::max(1, m_config.flush_box_need_clean_length.value);
    fig.box_need_clean_length_max = std::max(1, m_config.flush_box_need_clean_length_max.value);

    std::vector<float> cal_length;
    if (wipe_length > 0) {
        cal_flush_list(wipe_length, cal_length, fig);
    }
    const bool has_pending_skeleton_flush = m_pending_skeleton_flush_gcode_generator && m_pending_skeleton_flush_generator_volume > EPSILON;
    if (has_pending_skeleton_flush && !cal_length.empty()) {
        const float skeleton_flush_length = std::max(0.f, m_pending_skeleton_flush_generator_volume) / filament_area;
        consume_flush_lengths(cal_length, skeleton_flush_length);
    }
    const RemainingFlushSegments remaining_flush_segments = normalize_remaining_flush_segments(cal_length);
    const float remaining_flush_length = remaining_flush_segments.total_length;
    if (has_pending_skeleton_flush) {
        dyn_config.set_key_value("flush_length", new ConfigOptionFloat(remaining_flush_length));
        dyn_config.set_key_value("first_flush_volume", new ConfigOptionFloat(remaining_flush_length / 2.f));
        dyn_config.set_key_value("second_flush_volume", new ConfigOptionFloat(remaining_flush_length / 2.f));
    }
    int flush_idx = 0;
    for (; flush_idx < cal_length.size(); flush_idx++) {
        char key_value[64] = {0};
        snprintf(key_value, sizeof(key_value), "flush_length_%d", flush_idx + 1);
        dyn_config.set_key_value(key_value, new ConfigOptionFloat(cal_length[flush_idx]));
    }
    for (; flush_idx < g_max_flush_count; flush_idx++) {
        char key_value[64] = {0};
        snprintf(key_value, sizeof(key_value), "flush_length_%d", flush_idx + 1);
        dyn_config.set_key_value(key_value, new ConfigOptionFloat(0.f));
    }
    // Process the custom change_filament_gcode.
    const std::string& change_filament_gcode = m_config.change_filament_gcode.value;
    std::string        toolchange_gcode_parsed;
    const bool         flush_into_skeleton_toolchange = has_pending_skeleton_flush;
    const bool         has_remaining_outside_flush = flush_into_skeleton_toolchange && remaining_flush_segments.has_outside_flush();
    bool               skeleton_flush_toolchange_registered = false;
    std::string        skeleton_flush_wipe_wall_restore_fan_gcode;
    auto emit_pending_skeleton_flush_wipe_wall = [&]() -> std::string {
        if (!m_pending_skeleton_flush_wipe_wall_box_valid)
            return "";

        const double wipe_wall_print_z = m_pending_skeleton_flush_wipe_wall_print_z > EPSILON ?
                                               m_pending_skeleton_flush_wipe_wall_print_z : print_z;
        const float wipe_wall_layer_height = m_pending_skeleton_flush_wipe_wall_layer_height;
        m_pending_skeleton_flush_wipe_wall_print_z = -1.;
        m_pending_skeleton_flush_wipe_wall_layer_height = 0.f;

        Vec2d corners[4] = {
            m_pending_skeleton_flush_wipe_wall_box[0],
            m_pending_skeleton_flush_wipe_wall_box[1],
            m_pending_skeleton_flush_wipe_wall_box[2],
            m_pending_skeleton_flush_wipe_wall_box[3]
        };
        for (const Vec2d& corner : corners)
            if (!std::isfinite(corner.x()) || !std::isfinite(corner.y()))
                return "";

        Vec2d raw_avoid_corners[4];
        bool  has_avoid_box = m_pending_skeleton_flush_wipe_wall_avoid_box_valid;
        if (has_avoid_box) {
            for (int i = 0; i < 4; ++i) {
                raw_avoid_corners[i] = m_pending_skeleton_flush_wipe_wall_avoid_box[i];
                if (!std::isfinite(raw_avoid_corners[i].x()) || !std::isfinite(raw_avoid_corners[i].y())) {
                    has_avoid_box = false;
                    break;
                }
            }
        }
        m_pending_skeleton_flush_wipe_wall_avoid_box_valid = false;

        const double min_width  = std::min((corners[1] - corners[0]).norm(), (corners[2] - corners[3]).norm());
        const double min_height = std::min((corners[3] - corners[0]).norm(), (corners[2] - corners[1]).norm());
        if (min_width <= EPSILON || min_height <= EPSILON)
            return "";

        m_pending_skeleton_flush_wipe_wall_box_valid = false;

        const float nozzle_diameter = float(m_config.nozzle_diameter.get_at(extruder_id));
        float layer_height = wipe_wall_layer_height > EPSILON ?
                                 wipe_wall_layer_height :
                                 float(m_layer != nullptr && m_layer->height > EPSILON ? m_layer->height : m_config.layer_height.value);
        if (layer_height <= EPSILON)
            layer_height = 0.2f;
        const Flow wall_flow(nozzle_diameter * 1.25f, layer_height, nozzle_diameter);
        const double e_per_mm = wall_flow.mm3_per_mm() / std::max<double>(filament_area, EPSILON);

        const bool first_layer = m_layer_index <= 0;
        double target_speed = first_layer ? m_config.get_abs_value("initial_layer_speed") :
                                            m_config.get_abs_value("sparse_infill_speed");
        if (target_speed <= EPSILON)
            target_speed = first_layer ? 30. : 80.;
        if (!first_layer && m_config.wipe_tower_max_purge_speed.value > EPSILON)
            target_speed = std::min(target_speed, double(m_config.wipe_tower_max_purge_speed.value));
        const double wall_speed = 0.33 * target_speed;

        auto closest_point_on_segment = [](const Vec2d& point, const Vec2d& a, const Vec2d& b) -> Vec2d {
            const Vec2d ab = b - a;
            const double len2 = ab.squaredNorm();
            if (len2 <= EPSILON)
                return a;
            const double t = std::max(0., std::min(1., (point - a).dot(ab) / len2));
            return a + t * ab;
        };

        constexpr double entrance_wipe_length = 3.;
        Vec2d start_pos = corners[0];
        int start_corner_idx = 0;
        bool traverse_forward = true;
        auto set_start_corner = [&](int corner_idx) {
            start_corner_idx = corner_idx & 3;
            start_pos = corners[start_corner_idx];
            traverse_forward = start_corner_idx == 0 || start_corner_idx == 2;
        };
        auto set_horizontal_start_from_reference = [&](const Vec2d& reference) {
            double best_distance = std::numeric_limits<double>::max();
            int best_corner_idx = 0;
            for (int corner_idx : { 0, 1, 2, 3 }) {
                const double distance = (corners[corner_idx] - reference).squaredNorm();
                if (distance < best_distance) {
                    best_distance = distance;
                    best_corner_idx = corner_idx;
                }
            }
            set_start_corner(best_corner_idx);
        };
        if (m_pending_skeleton_flush_wipe_wall_start_pos_valid &&
            std::isfinite(m_pending_skeleton_flush_wipe_wall_start_pos.x()) &&
            std::isfinite(m_pending_skeleton_flush_wipe_wall_start_pos.y())) {
            set_horizontal_start_from_reference(m_pending_skeleton_flush_wipe_wall_start_pos);
        } else {
            set_start_corner((std::max(0, m_layer_index) + int(m_toolchange_count)) & 3);
        }
        m_pending_skeleton_flush_wipe_wall_start_pos_valid = false;
        Vec2d approach_pos = start_pos;
        const bool has_approach_pos = m_pending_skeleton_flush_wipe_wall_approach_pos_valid &&
            std::isfinite(m_pending_skeleton_flush_wipe_wall_approach_pos.x()) &&
            std::isfinite(m_pending_skeleton_flush_wipe_wall_approach_pos.y());
        if (has_approach_pos)
            approach_pos = m_pending_skeleton_flush_wipe_wall_approach_pos;
        m_pending_skeleton_flush_wipe_wall_approach_pos_valid = false;

        std::string wall_gcode;
        // Restore the layer cooling state before printing the wipe wall, matching the following wipe-tower path.
        wall_gcode += ";_FORCE_RESUME_FAN_SPEED\n";
        wall_gcode += ";TYPE:Prime tower\n";
        wall_gcode += "; CP SKELETON FLUSH WIPE WALL\n";
        wall_gcode += ";WIDTH:" + Slic3r::float_to_string_decimal_point(wall_flow.width(), 3) + "\n";
        wall_gcode += ";HEIGHT:" + Slic3r::float_to_string_decimal_point(layer_height, 3) + "\n";
        auto travel_to_if_needed = [&](const Vec2d& pos, const char* comment) {
            const Vec3d current = m_writer.get_position();
            if ((pos - Vec2d(current.x(), current.y())).norm() > EPSILON)
                wall_gcode += m_writer.travel_to_xy(pos, comment);
        };
        auto travel_to_gap_like_wipe = [&](const Vec2d& gap_pos, const Vec2d& entry_pos) {
            Vec2d route_avoid_corners[4];
            if (has_avoid_box) {
                const double avoid_min_x = std::min(std::min(raw_avoid_corners[0].x(), raw_avoid_corners[1].x()), std::min(raw_avoid_corners[2].x(), raw_avoid_corners[3].x()));
                const double avoid_max_x = std::max(std::max(raw_avoid_corners[0].x(), raw_avoid_corners[1].x()), std::max(raw_avoid_corners[2].x(), raw_avoid_corners[3].x()));
                const double avoid_min_y = std::min(std::min(raw_avoid_corners[0].y(), raw_avoid_corners[1].y()), std::min(raw_avoid_corners[2].y(), raw_avoid_corners[3].y()));
                const double avoid_max_y = std::max(std::max(raw_avoid_corners[0].y(), raw_avoid_corners[1].y()), std::max(raw_avoid_corners[2].y(), raw_avoid_corners[3].y()));
                constexpr double wipe_tower_avoid_offset = 2.;
                route_avoid_corners[0] = Vec2d(avoid_min_x - wipe_tower_avoid_offset, avoid_min_y - wipe_tower_avoid_offset);
                route_avoid_corners[1] = Vec2d(avoid_max_x + wipe_tower_avoid_offset, avoid_min_y - wipe_tower_avoid_offset);
                route_avoid_corners[2] = Vec2d(avoid_max_x + wipe_tower_avoid_offset, avoid_max_y + wipe_tower_avoid_offset);
                route_avoid_corners[3] = Vec2d(avoid_min_x - wipe_tower_avoid_offset, avoid_max_y + wipe_tower_avoid_offset);
            } else {
                const double box_min_x = std::min(std::min(corners[0].x(), corners[1].x()), std::min(corners[2].x(), corners[3].x()));
                const double box_max_x = std::max(std::max(corners[0].x(), corners[1].x()), std::max(corners[2].x(), corners[3].x()));
                const double box_min_y = std::min(std::min(corners[0].y(), corners[1].y()), std::min(corners[2].y(), corners[3].y()));
                const double box_max_y = std::max(std::max(corners[0].y(), corners[1].y()), std::max(corners[2].y(), corners[3].y()));
                const double clearance = std::max((gap_pos - entry_pos).norm(), double(wall_flow.width()));
                route_avoid_corners[0] = Vec2d(box_min_x - clearance, box_min_y - clearance);
                route_avoid_corners[1] = Vec2d(box_max_x + clearance, box_min_y - clearance);
                route_avoid_corners[2] = Vec2d(box_max_x + clearance, box_max_y + clearance);
                route_avoid_corners[3] = Vec2d(box_min_x - clearance, box_max_y + clearance);
            }

            auto ray_intersection = [](const Vec2d& a, const Vec2d& v, const Vec2d& b, const Vec2d& c, Vec2d& out) -> bool {
                const Vec2d edge = c - b;
                const double denom = cross2(v, edge);
                if (std::abs(denom) < EPSILON)
                    return false;
                const Vec2d ba = a - b;
                const double t = cross2(edge, ba) / denom;
                const double u = cross2(v, ba) / denom;
                if (t >= -EPSILON && u >= -EPSILON && u <= 1. + EPSILON) {
                    out = a + t * v;
                    return true;
                }
                return false;
            };
            auto segment_intersection = [](const Vec2d& a, const Vec2d& b, const Vec2d& c, const Vec2d& d, Vec2d& out) -> bool {
                const Vec2d r = b - a;
                const Vec2d s = d - c;
                const double denom = cross2(r, s);
                if (std::abs(denom) < EPSILON)
                    return false;
                const Vec2d ca = c - a;
                const double t = cross2(ca, s) / denom;
                const double u = cross2(ca, r) / denom;
                if (t >= -EPSILON && t <= 1. + EPSILON && u >= -EPSILON && u <= 1. + EPSILON) {
                    out = a + t * r;
                    return true;
                }
                return false;
            };
            auto boundary_path = [&](int from_edge, const Vec2d& from, int to_edge, const Vec2d& to, bool forward) {
                std::vector<Vec2d> path;
                path.push_back(from);
                int idx = forward ? (from_edge + 1) % 4 : from_edge;
                const int end = forward ? to_edge : (to_edge + 1) % 4;
                double len = 0.;
                Vec2d prev = from;
                while (idx != end) {
                    path.push_back(route_avoid_corners[idx]);
                    len += (route_avoid_corners[idx] - prev).norm();
                    prev = route_avoid_corners[idx];
                    idx = forward ? (idx + 1) % 4 : (idx + 3) % 4;
                }
                path.push_back(route_avoid_corners[end]);
                len += (route_avoid_corners[end] - prev).norm();
                len += (to - route_avoid_corners[end]).norm();
                path.push_back(to);
                return std::pair<std::vector<Vec2d>, double>(path, len);
            };

            Vec2d exit_dir(1., 0.);
            const double avoid_width = route_avoid_corners[1].x() - route_avoid_corners[0].x();
            if (std::abs(entry_pos.x() - route_avoid_corners[0].x()) < avoid_width / 2.)
                exit_dir = Vec2d(-1., 0.);

            int gap_edge = -1;
            Vec2d gap_on_avoid = gap_pos;
            for (int i = 0; i < 4; ++i) {
                if (ray_intersection(entry_pos, exit_dir, route_avoid_corners[i], route_avoid_corners[(i + 1) & 3], gap_on_avoid)) {
                    gap_edge = i;
                    break;
                }
            }
            if (gap_edge < 0) {
                travel_to_if_needed(gap_pos, "Travel to skeleton flush wipe wall gap");
                travel_to_if_needed(entry_pos, "Travel from gap to skeleton flush wipe wall");
                return;
            }

            const Vec3d current3 = m_writer.get_position();
            const Vec2d current_xy(current3.x(), current3.y());
            int enter_edge = -1;
            Vec2d enter_on_avoid = gap_on_avoid;
            for (int i = 0; i < 4; ++i) {
                if (i == gap_edge)
                    continue;
                if (segment_intersection(current_xy, gap_on_avoid, route_avoid_corners[i], route_avoid_corners[(i + 1) & 3], enter_on_avoid)) {
                    enter_edge = i;
                    break;
                }
            }

            if (enter_edge >= 0) {
                auto [path_forward, len_forward] = boundary_path(enter_edge, enter_on_avoid, gap_edge, gap_on_avoid, true);
                auto [path_backward, len_backward] = boundary_path(enter_edge, enter_on_avoid, gap_edge, gap_on_avoid, false);
                const std::vector<Vec2d>& path = len_forward < len_backward ? path_forward : path_backward;
                for (const Vec2d& pos : path)
                    travel_to_if_needed(pos, "Travel around skeleton flush wipe wall gap");
            } else {
                travel_to_if_needed(gap_on_avoid, "Travel to skeleton flush wipe wall gap");
            }
            travel_to_if_needed(entry_pos, "Travel from gap to skeleton flush wipe wall");
        };
        if ((has_approach_pos && (approach_pos - start_pos).norm() > EPSILON) || has_avoid_box)
            travel_to_gap_like_wipe(approach_pos, start_pos);
        else
            travel_to_if_needed(start_pos, "Travel from gap to skeleton flush wipe wall");
        wall_gcode += m_writer.travel_to_z(wipe_wall_print_z, "Move to skeleton flush wipe wall layer");
        wall_gcode += m_writer.unlift();
        // Keep the writer retraction/E state, but suppress the standalone E-only priming line.
        (void) m_writer.unretract();
        wall_gcode += "G4 S0\n";
        wall_gcode += m_writer.set_speed(wall_speed * 60.);

        Vec2d current = start_pos;
        const int traversal_step = traverse_forward ? 1 : 3;
        int next_corner_idx = (start_corner_idx + traversal_step) & 3;
        {
            const Vec2d edge = corners[next_corner_idx] - current;
            const double edge_len = edge.norm();
            if (edge_len > EPSILON) {
                const Vec2d edge_dir = edge / edge_len;
                const double entry_len = std::min(entrance_wipe_length, edge_len);
                const Vec2d entry_to = current + edge_dir * entry_len;
                wall_gcode += m_writer.extrude_to_xy(entry_to, entry_len * e_per_mm, "Skeleton flush wipe wall entrance");
                wall_gcode += m_writer.retract();
                const double retract_speed = m_writer.extruder() != nullptr ? m_writer.extruder()->retract_speed() : 40.;
                wall_gcode += m_writer.travel_to_xy(current, "Skeleton flush wipe wall entrance wipe back", std::max(1., retract_speed * 0.25));
                wall_gcode += m_writer.travel_to_xy(entry_to, "Skeleton flush wipe wall entrance wipe forward", std::max(1., retract_speed * 0.10));
                wall_gcode += m_writer.unretract();
                wall_gcode += m_writer.set_speed(wall_speed * 60.);
                current = entry_to;
            }
        }
        Vec2d last_extrude_from = current;
        for (int step = 0; step < 3; ++step) {
            const Vec2d next = corners[next_corner_idx];
            const double distance = (next - current).norm();
            if (distance > EPSILON) {
                last_extrude_from = current;
                wall_gcode += m_writer.extrude_to_xy(next, distance * e_per_mm, "Skeleton flush wipe wall");
            }
            current = next;
            next_corner_idx = (next_corner_idx + traversal_step) & 3;
        }
        {
            const Vec2d start_corner = corners[start_corner_idx];
            const Vec2d closing_edge = start_corner - current;
            const double closing_len = closing_edge.norm();
            if (closing_len > EPSILON) {
                const Vec2d closing_dir = closing_edge / closing_len;
                const double entry_gap_len = std::min(2.5 * double(wall_flow.width()), closing_len);
                const Vec2d gap_stop = start_corner - closing_dir * entry_gap_len;
                const double distance = (gap_stop - current).norm();
                if (distance > EPSILON) {
                    last_extrude_from = current;
                    wall_gcode += m_writer.extrude_to_xy(gap_stop, distance * e_per_mm, "Skeleton flush wipe wall");
                }
                current = gap_stop;
            }
        }
        const Vec2d wipe_back_dir = last_extrude_from - current;
        const double wipe_back_len = wipe_back_dir.norm();
        const Vec2d wipe_back_to = wipe_back_len > EPSILON ?
            current + wipe_back_dir * (std::min(2., wipe_back_len) / wipe_back_len) :
            current;
        this->set_last_pos(this->gcode_to_point(current));
        m_wipe.reset_path();
        m_wipe.path.points.emplace_back(this->gcode_to_point(current));
        m_wipe.path.points.emplace_back(this->gcode_to_point(wipe_back_to));
        const LiftType wipe_wall_lift_type = m_writer.extruder() != nullptr ?
            this->to_lift_type(ZHopType(EXTRUDER_CONFIG(z_hop_types))) :
            LiftType::NormalLift;
        wall_gcode += this->retract(false, false, wipe_wall_lift_type);
        m_wipe.reset_path();
        wall_gcode += skeleton_flush_wipe_wall_restore_fan_gcode;
        return wall_gcode;
    };
    auto sync_current_extruder_retraction_after_custom_gcode = [&](const std::string& custom_gcode, bool reset_retracted = false) {
        sync_writer_extruder_retraction_after_custom_gcode(m_writer, custom_gcode, reset_retracted);
    };
    // Orca: Ignore change_filament_gcode if is the first call for a tool change and manual_filament_change is enabled
    if (!change_filament_gcode.empty() && !(m_config.manual_filament_change.value && m_toolchange_count == 1) && change_tool) {
        dyn_config.set_key_value("previous_extruder", new ConfigOptionInt(parser_previous_extruder_id));
        dyn_config.set_key_value("next_extruder", new ConfigOptionInt((int) extruder_id));
        dyn_config.set_key_value("layer_num", new ConfigOptionInt(m_layer_index));
        dyn_config.set_key_value("layer_z", new ConfigOptionFloat(print_z));
        dyn_config.set_key_value("toolchange_z", new ConfigOptionFloat(print_z));
        dyn_config.set_key_value("max_layer_z", new ConfigOptionFloat(m_max_layer_z));
        dyn_config.set_key_value("flush_into_skeleton", new ConfigOptionBool(flush_into_skeleton_toolchange));

        toolchange_gcode_parsed = placeholder_parser_process("change_filament_gcode", change_filament_gcode, extruder_id, &dyn_config);
        check_add_eol(toolchange_gcode_parsed);

        // if (!m_config.single_extruder_multi_material && m_placeholder_parser_integration.num_extruders > 1 && m_powerPos != Vec2d::Zero()) {
        //     gcode += m_writer.travel_to_xy(m_powerPos);
        // }
        // else
        // if (!m_config.single_extruder_multi_material && m_config.print_sequence.value == PrintSequence::ByObject && change_tool) {
        //     gcode += m_writer.travel_to_xy(Vec2d::Zero());
        // }

        if (m_pending_skeleton_flush_gcode_generator && m_pending_skeleton_flush_generator_volume > EPSILON) {
            std::string prefix;
            std::string suffix;
            bool split_for_skeleton_flush = false;
            if (!has_remaining_outside_flush) {
                // Close the CFS feeding/statistics window before a long skeleton consumes all remaining flush.
                prefix = toolchange_gcode_parsed;
                split_for_skeleton_flush = true;
            } else {
                const size_t remaining_flush_pos = find_skeleton_flush_remaining_flush_line_start(toolchange_gcode_parsed);
                if (remaining_flush_pos != std::string::npos && remaining_flush_pos > 0) {
                    prefix = toolchange_gcode_parsed.substr(0, remaining_flush_pos);
                    suffix = toolchange_gcode_parsed.substr(remaining_flush_pos);
                    split_for_skeleton_flush = true;
                } else {
                    const size_t flush_start_pos = find_flush_start_line_start(toolchange_gcode_parsed);
                    if (flush_start_pos != std::string::npos && flush_start_pos > 0) {
                        prefix = toolchange_gcode_parsed.substr(0, flush_start_pos);
                        suffix = toolchange_gcode_parsed.substr(flush_start_pos);
                        split_for_skeleton_flush = true;
                    } else {
                        const size_t flush_end_after = find_flush_end_line_after(toolchange_gcode_parsed);
                        if (flush_end_after != std::string::npos && flush_end_after > 0) {
                            prefix = toolchange_gcode_parsed.substr(0, flush_end_after);
                            suffix = toolchange_gcode_parsed.substr(flush_end_after);
                            split_for_skeleton_flush = true;
                        } else {
                            std::string before_toolchange;
                            std::string toolchange_line;
                            std::string after_toolchange;
                            if (split_last_toolchange_line_with_suffix(toolchange_gcode_parsed, extruder_id, before_toolchange, toolchange_line, after_toolchange)) {
                                prefix = before_toolchange + toolchange_line;
                                suffix = after_toolchange;
                                split_for_skeleton_flush = true;
                            }
                        }
                    }
                }
            }
            if (split_for_skeleton_flush) {
                const int previous_extruder_id = m_writer.extruder() != nullptr ? static_cast<int>(m_writer.extruder()->id()) : -1;
                skeleton_flush_wipe_wall_restore_fan_gcode = last_primary_fan_command(prefix);
                const bool prefix_changes_tool = custom_gcode_changes_tool(prefix, m_writer.toolchange_prefix(), extruder_id);
                gcode += prefix;
                // The prefix may already emit T[next]. Still sync the writer before generating first-flush skeleton.
                std::string toolchange_command = m_writer.toolchange(extruder_id, !prefix_changes_tool && change_tool);
                if (!prefix_changes_tool)
                    gcode += toolchange_command;
                if (prefix_changes_tool) {
                    std::string before_toolchange;
                    std::string toolchange_line;
                    std::string after_toolchange;
                    sync_current_extruder_retraction_after_custom_gcode(split_last_toolchange_line_with_suffix(prefix, extruder_id, before_toolchange, toolchange_line, after_toolchange) ? after_toolchange : prefix, true);
                }

                const Vec3d skeleton_flush_return_position = position_after_custom_gcode(prefix, m_writer.get_position(), m_writer.get_xy_offset().cast<double>());
                m_writer.set_position(skeleton_flush_return_position);
                m_writer.sync_lifted_to_nominal_z(m_nominal_z);
                m_writer.set_current_position_clear(false);
                if (previous_extruder_id >= 0)
                    gcode += ";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Skeleton_Flush_Preview_Start) + std::to_string(previous_extruder_id) + "\n";
                gcode += emit_pending_skeleton_flush_wipe_wall();
                gcode += ";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Wipe_Tower_End) + "\n";

                gcode += m_pending_skeleton_flush_gcode_generator();
                if (previous_extruder_id >= 0)
                    gcode += ";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Skeleton_Flush_Preview_End) + "\n";
                gcode += this->retract(false, false, LiftType::NormalLift);
                if (has_remaining_outside_flush)
                    gcode += m_writer.travel_to_xyz(skeleton_flush_return_position, "Travel to first flush position after skeleton");
                gcode += ";TYPE:Prime tower\n";
                gcode += ";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Wipe_Tower_Start) + "\n";
                skeleton_flush_toolchange_registered = true;

                this->placeholder_parser().set("current_extruder", extruder_id);
                this->placeholder_parser().set("retraction_distance_when_cut", m_config.retraction_distances_when_cut.get_at(extruder_id));
                this->placeholder_parser().set("long_retraction_when_cut", m_config.long_retractions_when_cut.get_at(extruder_id));

                m_pending_skeleton_flush_gcode_generator = nullptr;
                m_pending_skeleton_flush_generator_volume = 0.f;
                gcode += suffix;
                sync_current_extruder_retraction_after_custom_gcode(suffix, true);
            }
        }
        if (!skeleton_flush_toolchange_registered)
            gcode += toolchange_gcode_parsed;

        // BBS
        {
            // BBS: gcode writer doesn't know where the extruder is and whether fan speed is changed after inserting tool change gcode
            // Set this flag so that normal lift will be used the first time after tool change.
            gcode += ";_FORCE_RESUME_FAN_SPEED\n";
            m_writer.set_current_position_clear(false);
            // BBS: check whether custom gcode changes the z position. Update if changed
            double temp_z_after_tool_change;
            if (GCodeProcessor::get_last_z_from_gcode(toolchange_gcode_parsed, temp_z_after_tool_change)) {
                Vec3d pos = m_writer.get_position();
                pos(2)    = temp_z_after_tool_change;
                m_writer.set_position(pos);
            }
        }
    }

    // BBS. Reset old extruder E-value.
    // Keep retract length because Custom GCode will guarantee retract length be the same as toolchange
    if (m_config.single_extruder_multi_material) {
        m_writer.reset_e();
    }

    // We inform the writer about what is happening, but we may not use the resulting gcode.
    if (!skeleton_flush_toolchange_registered) {
        const int previous_extruder_id = m_writer.extruder() != nullptr ? static_cast<int>(m_writer.extruder()->id()) : -1;
        const bool has_pending_skeleton_flush = m_pending_skeleton_flush_gcode_generator && m_pending_skeleton_flush_generator_volume > EPSILON;
        skeleton_flush_wipe_wall_restore_fan_gcode = last_primary_fan_command(toolchange_gcode_parsed);
        Vec3d skeleton_flush_return_position = m_writer.get_position();
        if (has_pending_skeleton_flush && !toolchange_gcode_parsed.empty())
            skeleton_flush_return_position = position_after_custom_gcode(toolchange_gcode_parsed, skeleton_flush_return_position, m_writer.get_xy_offset().cast<double>());

        std::string toolchange_command = m_writer.toolchange(extruder_id, change_tool);
        if (!custom_gcode_changes_tool(toolchange_gcode_parsed, m_writer.toolchange_prefix(), extruder_id))
            gcode += toolchange_command;
        else {
            // user provided his own toolchange gcode, no need to do anything
        }

        if (has_pending_skeleton_flush) {
            m_writer.set_position(skeleton_flush_return_position);
            m_writer.sync_lifted_to_nominal_z(m_nominal_z);
            m_writer.set_current_position_clear(false);
            if (previous_extruder_id >= 0)
                gcode += ";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Skeleton_Flush_Preview_Start) + std::to_string(previous_extruder_id) + "\n";
            gcode += emit_pending_skeleton_flush_wipe_wall();
            gcode += ";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Wipe_Tower_End) + "\n";
            gcode += m_pending_skeleton_flush_gcode_generator();
            if (previous_extruder_id >= 0)
                gcode += ";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Skeleton_Flush_Preview_End) + "\n";
            gcode += this->retract(false, false, LiftType::NormalLift);
            gcode += m_writer.travel_to_xyz(skeleton_flush_return_position, "Travel to first flush position after skeleton");
            gcode += ";TYPE:Prime tower\n";
            gcode += ";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Wipe_Tower_Start) + "\n";
            m_pending_skeleton_flush_gcode_generator = nullptr;
            m_pending_skeleton_flush_generator_volume = 0.f;
        } else {
            gcode += emit_pending_skeleton_flush_wipe_wall();
        }
    }

    bool bchange = m_writer.need_toolchange(extruder_id);

    // Set the temperature if the wipe tower didn't (not needed for non-single extruder MM)
    if (m_config.single_extruder_multi_material && !m_config.enable_prime_tower) {
        int temp = (m_layer_index <= 0 ? m_config.nozzle_temperature_initial_layer.get_at(extruder_id) :
                                         m_config.nozzle_temperature.get_at(extruder_id));

        gcode += m_writer.set_temperature(temp, false);
    }

    this->placeholder_parser().set("current_extruder", extruder_id);
#ifdef SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
    this->placeholder_parser().set("retraction_distance_when_cut", effective_cut_retraction_distance);
    this->placeholder_parser().set("retraction_distances_when_cut", new ConfigOptionFloats(effective_cut_retraction_distances));
    this->placeholder_parser().set("long_retraction_when_cut", m_config.long_retractions_when_cut.get_at(cut_retraction_extruder_id));
#else
    this->placeholder_parser().set("retraction_distance_when_cut", m_config.retraction_distances_when_cut.get_at(extruder_id));
    this->placeholder_parser().set("long_retraction_when_cut", m_config.long_retractions_when_cut.get_at(extruder_id));
#endif // SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT

    // Append the filament start G-code.
    const std::string& filament_start_gcode = m_config.filament_start_gcode.get_at(extruder_id);
    if (!filament_start_gcode.empty()) {
        // Process the filament_start_gcode for the new filament.
        DynamicConfig config;
        config.set_key_value("layer_num", new ConfigOptionInt(m_layer_index));
        config.set_key_value("layer_z", new ConfigOptionFloat(this->writer().get_position().z() - m_config.z_offset.value));
        config.set_key_value("max_layer_z", new ConfigOptionFloat(m_max_layer_z));
        config.set_key_value("filament_extruder_id", new ConfigOptionInt(int(extruder_id)));
#ifdef SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
        config.set_key_value("retraction_distance_when_cut", new ConfigOptionFloat(effective_cut_retraction_distance));
        config.set_key_value("retraction_distances_when_cut", new ConfigOptionFloats(effective_cut_retraction_distances));
        config.set_key_value("long_retraction_when_cut", new ConfigOptionBool(m_config.long_retractions_when_cut.get_at(cut_retraction_extruder_id)));
#endif // SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT

        // std::vector<double> _position(3, 0);
        //_position[2] = print_z;
        // config.set_key_value("position", new ConfigOptionFloats(_position));

        gcode += this->placeholder_parser_process("filament_start_gcode", filament_start_gcode, extruder_id, &config);

        check_add_eol(gcode);
    }
    // Set the new extruder to the operating temperature.
    if (m_ooze_prevention.enable)
        gcode += m_ooze_prevention.post_toolchange(*this);

    if (m_config.enable_pressure_advance.get_at(extruder_id)) {
        gcode += m_writer.set_pressure_advance(m_config.pressure_advance.get_at(extruder_id));
    }

    m_pending_skeleton_flush_wipe_wall_box_valid = false;
    m_pending_skeleton_flush_wipe_wall_avoid_box_valid = false;
    m_pending_skeleton_flush_wipe_wall_start_pos_valid = false;
    m_pending_skeleton_flush_wipe_wall_approach_pos_valid = false;
    return gcode;
}



static size_t find_comment_line_start(const std::string& gcode, const char* marker)
{
    const size_t marker_length = std::char_traits<char>::length(marker);
    size_t pos = 0;
    while (pos < gcode.size()) {
        const size_t line_end = gcode.find('\n', pos);
        const size_t end = line_end == std::string::npos ? gcode.size() : line_end;
        size_t scan = pos;
        while (scan < end && (gcode[scan] == ' ' || gcode[scan] == '\t'))
            ++scan;

        if (scan < end && gcode[scan] == ';') {
            ++scan;
            while (scan < end && (gcode[scan] == ' ' || gcode[scan] == '\t'))
                ++scan;
            if (end - scan >= marker_length && gcode.compare(scan, marker_length, marker) == 0)
                return pos;
        }

        if (line_end == std::string::npos)
            break;
        pos = line_end + 1;
    }
    return std::string::npos;
}

static size_t find_flush_start_line_start(const std::string& gcode)
{
    return find_comment_line_start(gcode, "FLUSH_START");
}

static size_t find_skeleton_flush_remaining_flush_line_start(const std::string& gcode)
{
    return find_comment_line_start(gcode, "SKELETON_FLUSH_REMAINING_FLUSH");
}

static size_t find_flush_end_line_after(const std::string& gcode)
{
    size_t pos = 0;
    while (pos < gcode.size()) {
        const size_t line_end = gcode.find('\n', pos);
        const size_t end = line_end == std::string::npos ? gcode.size() : line_end;
        size_t scan = pos;
        while (scan < end && (gcode[scan] == ' ' || gcode[scan] == '\t'))
            ++scan;

        if (scan < end && gcode[scan] == ';') {
            ++scan;
            while (scan < end && (gcode[scan] == ' ' || gcode[scan] == '\t'))
                ++scan;
            if (end - scan >= 9 && gcode.compare(scan, 9, "FLUSH_END") == 0)
                return line_end == std::string::npos ? gcode.size() : line_end + 1;
        }

        if (line_end == std::string::npos)
            break;
        pos = line_end + 1;
    }
    return std::string::npos;
}

static bool parse_axis_value(const std::string& line, size_t end, char axis, double& value)
{
    const char lower_axis = char(axis - 'A' + 'a');
    bool found = false;
    for (size_t i = 0; i < end; ++i) {
        if (line[i] != axis && line[i] != lower_axis)
            continue;
        const char* number_begin = line.c_str() + i + 1;
        char* number_end = nullptr;
        const double parsed = std::strtod(number_begin, &number_end);
        if (number_end != number_begin) {
            value = parsed;
            found = true;
        }
    }
    return found;
}

static std::string last_primary_fan_command(const std::string& gcode)
{
    std::string result;
    size_t pos = 0;
    while (pos < gcode.size()) {
        const size_t line_end = gcode.find('\n', pos);
        size_t end = line_end == std::string::npos ? gcode.size() : line_end;
        if (end > pos && gcode[end - 1] == '\r')
            --end;
        const size_t comment = gcode.find(';', pos);
        if (comment != std::string::npos && comment < end)
            end = comment;

        size_t scan = pos;
        while (scan < end && (gcode[scan] == ' ' || gcode[scan] == '\t'))
            ++scan;
        auto matches_command = [&](const char* command) {
            constexpr size_t command_len = 4;
            if (end - scan < command_len)
                return false;
            for (size_t i = 0; i < command_len; ++i)
                if (std::toupper(static_cast<unsigned char>(gcode[scan + i])) != command[i])
                    return false;
            return scan + command_len == end || gcode[scan + command_len] == ' ' || gcode[scan + command_len] == '\t';
        };

        bool primary_fan_command = matches_command("M107");
        if (matches_command("M106")) {
            double fan_index = 0.;
            const bool has_fan_index = parse_axis_value(gcode.substr(pos, end - pos), end - pos, 'P', fan_index);
            primary_fan_command = !has_fan_index || std::lround(fan_index) == 0 || std::lround(fan_index) == 1;
        }
        if (primary_fan_command) {
            result.assign(gcode, scan, end - scan);
            result.push_back('\n');
        }

        if (line_end == std::string::npos)
            break;
        pos = line_end + 1;
    }
    return result;
}

static bool is_toolchange_line_for_extruder(const std::string& line, size_t end, unsigned int extruder_id)
{
    size_t scan = 0;
    while (scan < end && (line[scan] == ' ' || line[scan] == '\t'))
        ++scan;
    if (scan >= end || line[scan] != 'T')
        return false;

    const char* number_begin = line.c_str() + scan + 1;
    char* number_end = nullptr;
    const unsigned long parsed = std::strtoul(number_begin, &number_end, 10);
    if (number_end == number_begin || parsed != extruder_id)
        return false;

    scan = size_t(number_end - line.c_str());
    while (scan < end && (line[scan] == ' ' || line[scan] == '\t' || line[scan] == '\r'))
        ++scan;
    return scan >= end || line[scan] == ';';
}

static bool split_last_toolchange_line_with_suffix(const std::string& gcode, unsigned int extruder_id, std::string& before_toolchange, std::string& toolchange_line, std::string& after_toolchange)
{
    size_t pos = 0;
    size_t found_start = std::string::npos;
    size_t found_after = std::string::npos;

    while (pos < gcode.size()) {
        const size_t line_end = gcode.find('\n', pos);
        const size_t end = line_end == std::string::npos ? gcode.size() : line_end;
        const std::string line = gcode.substr(pos, end - pos);
        if (is_toolchange_line_for_extruder(line, line.size(), extruder_id)) {
            found_start = pos;
            found_after = line_end == std::string::npos ? gcode.size() : line_end + 1;
        }
        if (line_end == std::string::npos)
            break;
        pos = line_end + 1;
    }

    if (found_start == std::string::npos)
        return false;

    before_toolchange = gcode.substr(0, found_start);
    toolchange_line = gcode.substr(found_start, found_after - found_start);
    after_toolchange = gcode.substr(found_after);
    check_add_eol(toolchange_line);
    return true;
}
static int parse_g_command(const std::string& line, size_t end)
{
    size_t scan = 0;
    while (scan < end && (line[scan] == ' ' || line[scan] == '\t'))
        ++scan;
    if (scan >= end || (line[scan] != 'G' && line[scan] != 'g'))
        return -1;

    const char* number_begin = line.c_str() + scan + 1;
    char* number_end = nullptr;
    const long parsed = std::strtol(number_begin, &number_end, 10);
    return number_end == number_begin ? -1 : int(parsed);
}

static double retracted_after_custom_gcode(const std::string& gcode, double e_position, double retracted, bool relative_e, bool* saw_e_move)
{
    bool saw_e_motion = false;
    size_t pos = 0;
    while (pos < gcode.size()) {
        const size_t line_end = gcode.find('\n', pos);
        const size_t end = line_end == std::string::npos ? gcode.size() : line_end;
        std::string line = gcode.substr(pos, end - pos);
        size_t comment = line.find(';');
        if (comment == std::string::npos)
            comment = line.size();

        const int g_command = parse_g_command(line, comment);
        double e_value = 0.;
        if ((g_command == 0 || g_command == 1 || g_command == 2 || g_command == 3) && parse_axis_value(line, comment, 'E', e_value)) {
            saw_e_motion = true;
            const double dE = relative_e ? e_value : e_value - e_position;
            if (!relative_e)
                e_position = e_value;
            if (dE < -EPSILON)
                retracted += -dE;
            else if (dE > EPSILON)
                retracted = std::max(0., retracted - dE);
        } else if (g_command == 92 && parse_axis_value(line, comment, 'E', e_value)) {
            e_position = e_value;
        }

        if (line_end == std::string::npos)
            break;
        pos = line_end + 1;
    }
    if (saw_e_move != nullptr)
        *saw_e_move = saw_e_motion;
    return retracted;
}

static void sync_writer_extruder_retraction_after_custom_gcode(GCodeWriter& writer, const std::string& custom_gcode, bool reset_retracted)
{
    if (custom_gcode.empty() || writer.extruder() == nullptr)
        return;

    bool saw_e_move = false;
    const double initial_retracted = reset_retracted ? 0. : writer.extruder()->retracted();
    const double retracted = retracted_after_custom_gcode(custom_gcode, writer.extruder()->E(), initial_retracted, writer.config.use_relative_e_distances, &saw_e_move);
    if (!saw_e_move)
        return;
    if (!is_approx(retracted, writer.extruder()->retracted()))
        writer.extruder()->set_retracted(retracted, retracted > EPSILON ? writer.extruder()->restart_extra() : 0.);
}
static Vec3d position_after_custom_gcode(const std::string& gcode, Vec3d position, const Vec2d& xy_offset)
{
    size_t pos = 0;
    while (pos < gcode.size()) {
        const size_t line_end = gcode.find('\n', pos);
        const size_t end = line_end == std::string::npos ? gcode.size() : line_end;
        std::string line = gcode.substr(pos, end - pos);
        size_t comment = line.find(';');
        if (comment == std::string::npos)
            comment = line.size();

        double value = 0.;
        if (parse_axis_value(line, comment, 'X', value))
            position(0) = value + xy_offset(0);
        if (parse_axis_value(line, comment, 'Y', value))
            position(1) = value + xy_offset(1);
        if (parse_axis_value(line, comment, 'Z', value))
            position(2) = value;

        if (line_end == std::string::npos)
            break;
        pos = line_end + 1;
    }
    return position;
}
std::string GCode::start_skeleton_flush_toolchange(unsigned int new_extruder_id, double print_z, float skeleton_flush_volume)
{
    if (m_pending_skeleton_flush_toolchange || skeleton_flush_volume <= EPSILON || !m_config.multicolor_method.value ||
        m_config.change_filament_gcode.value.empty() || m_writer.extruder() == nullptr || !m_writer.need_toolchange(new_extruder_id))
        return "";

    const unsigned int old_extruder_id = m_writer.extruder()->id();
    std::vector<float> flush_matrix(cast<float>(m_config.flush_volumes_matrix.values));
    const unsigned int number_of_extruders = (unsigned int)(sqrt(flush_matrix.size()) + EPSILON);
    if (old_extruder_id >= number_of_extruders || new_extruder_id >= number_of_extruders ||
        flush_matrix.size() < size_t(number_of_extruders) * number_of_extruders)
        return "";

    float wipe_volume = flush_matrix[old_extruder_id * number_of_extruders + new_extruder_id];
    wipe_volume *= m_config.flush_multiplier;
    if (wipe_volume <= EPSILON)
        return "";

    const float filament_area = float((M_PI / 4.f) * pow(m_config.filament_diameter.get_at(new_extruder_id), 2));
    float wipe_length = wipe_volume / filament_area;
    const float skeleton_flush_length = std::max(0.f, skeleton_flush_volume) / filament_area;

    FlushConfig fig;
    fig.box_first_clean_length    = std::max(1, m_config.flush_box_first_clean_length.value);
    fig.box_need_clean_length     = std::max(1, m_config.flush_box_need_clean_length.value);
    fig.box_need_clean_length_max = std::max(1, m_config.flush_box_need_clean_length_max.value);

    std::vector<float> cal_length;
    if (wipe_length > 0)
        cal_flush_list(wipe_length, cal_length, fig);
    if (!cal_length.empty())
        consume_flush_lengths(cal_length, skeleton_flush_length);

    const RemainingFlushSegments remaining_flush_segments = normalize_remaining_flush_segments(cal_length);
    const float remaining_flush_length = remaining_flush_segments.total_length;
    const bool has_remaining_outside_flush = remaining_flush_segments.has_outside_flush();

    const float new_retract_length = m_config.retraction_length.get_at(new_extruder_id);
    const float new_retract_length_toolchange = m_config.retract_length_toolchange.get_at(new_extruder_id);
    int new_filament_temp = this->on_first_layer() ? m_config.nozzle_temperature_initial_layer.get_at(new_extruder_id) :
                                                     m_config.nozzle_temperature.get_at(new_extruder_id);
    if (abs(print_z) < EPSILON)
        new_filament_temp = m_config.nozzle_temperature_initial_layer.get_at(new_extruder_id);

    const float old_retract_length = m_config.retraction_length.get_at(old_extruder_id);
    const float old_retract_length_toolchange = m_config.retract_length_toolchange.get_at(old_extruder_id);
    const int old_filament_temp = this->on_first_layer() ? m_config.nozzle_temperature_initial_layer.get_at(old_extruder_id) :
                                                           m_config.nozzle_temperature.get_at(old_extruder_id);
    int old_filament_e_feedrate = int(60.0 * m_config.filament_max_volumetric_speed.get_at(old_extruder_id) / filament_area);
    old_filament_e_feedrate = old_filament_e_feedrate == 0 ? 100 : old_filament_e_feedrate;
    int new_filament_e_feedrate = int(60.0 * m_config.filament_max_volumetric_speed.get_at(new_extruder_id) / filament_area);
    new_filament_e_feedrate = new_filament_e_feedrate == 0 ? 100 : new_filament_e_feedrate;

    Vec3d nozzle_pos = m_writer.get_position();
    DynamicConfig dyn_config;
    dyn_config.set_key_value("previous_extruder", new ConfigOptionInt((int)old_extruder_id));
    dyn_config.set_key_value("next_extruder", new ConfigOptionInt((int)new_extruder_id));
    dyn_config.set_key_value("layer_num", new ConfigOptionInt(m_layer_index));
    dyn_config.set_key_value("layer_z", new ConfigOptionFloat(print_z));
    dyn_config.set_key_value("toolchange_z", new ConfigOptionFloat(print_z));
    dyn_config.set_key_value("max_layer_z", new ConfigOptionFloat(m_max_layer_z));
    dyn_config.set_key_value("relative_e_axis", new ConfigOptionBool(m_config.use_relative_e_distances));
    dyn_config.set_key_value("toolchange_count", new ConfigOptionInt((int)m_toolchange_count + 1));
    dyn_config.set_key_value("fan_speed", new ConfigOptionInt(0));
    dyn_config.set_key_value("flush_into_skeleton", new ConfigOptionBool(true));
    dyn_config.set_key_value("old_retract_length", new ConfigOptionFloat(old_retract_length));
    dyn_config.set_key_value("new_retract_length", new ConfigOptionFloat(new_retract_length));
    dyn_config.set_key_value("old_retract_length_toolchange", new ConfigOptionFloat(old_retract_length_toolchange));
    dyn_config.set_key_value("new_retract_length_toolchange", new ConfigOptionFloat(new_retract_length_toolchange));
    dyn_config.set_key_value("old_filament_temp", new ConfigOptionInt(old_filament_temp));
    dyn_config.set_key_value("new_filament_temp", new ConfigOptionInt(new_filament_temp));
    dyn_config.set_key_value("x_after_toolchange", new ConfigOptionFloat(nozzle_pos(0)));
    dyn_config.set_key_value("y_after_toolchange", new ConfigOptionFloat(nozzle_pos(1)));
    dyn_config.set_key_value("z_after_toolchange", new ConfigOptionFloat(nozzle_pos(2)));
    dyn_config.set_key_value("first_flush_volume", new ConfigOptionFloat(remaining_flush_length / 2.f));
    dyn_config.set_key_value("second_flush_volume", new ConfigOptionFloat(remaining_flush_length / 2.f));
    dyn_config.set_key_value("old_filament_e_feedrate", new ConfigOptionInt(old_filament_e_feedrate));
    dyn_config.set_key_value("new_filament_e_feedrate", new ConfigOptionInt(new_filament_e_feedrate));
    dyn_config.set_key_value("travel_point_1_x", new ConfigOptionFloat(float(travel_point_1.x())));
    dyn_config.set_key_value("travel_point_1_y", new ConfigOptionFloat(float(travel_point_1.y())));
    dyn_config.set_key_value("travel_point_2_x", new ConfigOptionFloat(float(travel_point_2.x())));
    dyn_config.set_key_value("travel_point_2_y", new ConfigOptionFloat(float(travel_point_2.y())));
    dyn_config.set_key_value("travel_point_3_x", new ConfigOptionFloat(float(travel_point_3.x())));
    dyn_config.set_key_value("travel_point_3_y", new ConfigOptionFloat(float(travel_point_3.y())));
    dyn_config.set_key_value("wipe_tower_start_position_x", new ConfigOptionFloat(nozzle_pos(0)));
    dyn_config.set_key_value("wipe_tower_start_position_y", new ConfigOptionFloat(nozzle_pos(1)));
    dyn_config.set_key_value("wipe_tower_random_num", new ConfigOptionFloat(0.f));
    dyn_config.set_key_value("wipe_tower_outer_wall_x", new ConfigOptionFloat(nozzle_pos(0)));
    dyn_config.set_key_value("wipe_tower_outer_wall_y", new ConfigOptionFloat(nozzle_pos(1)));
    dyn_config.set_key_value("flush_length", new ConfigOptionFloat(remaining_flush_length));

    int flush_idx = 0;
    for (; flush_idx < (int)cal_length.size() && flush_idx < g_max_flush_count; ++flush_idx) {
        char key_value[64] = {0};
        snprintf(key_value, sizeof(key_value), "flush_length_%d", flush_idx + 1);
        dyn_config.set_key_value(key_value, new ConfigOptionFloat(std::max(0.f, cal_length[flush_idx])));
    }
    for (; flush_idx < g_max_flush_count; ++flush_idx) {
        char key_value[64] = {0};
        snprintf(key_value, sizeof(key_value), "flush_length_%d", flush_idx + 1);
        dyn_config.set_key_value(key_value, new ConfigOptionFloat(0.f));
    }

    std::string parsed = placeholder_parser_process("change_filament_gcode", m_config.change_filament_gcode.value, new_extruder_id, &dyn_config);
    check_add_eol(parsed);
    const size_t flush_start_pos = find_flush_start_line_start(parsed);
    std::string prefix;
    std::string suffix;
    if (!has_remaining_outside_flush) {
        // Do not defer the template tail after an all-consuming skeleton.
        prefix = parsed;
    } else {
        const size_t remaining_flush_pos = find_skeleton_flush_remaining_flush_line_start(parsed);
        if (remaining_flush_pos != std::string::npos && remaining_flush_pos > 0) {
            prefix = parsed.substr(0, remaining_flush_pos);
            suffix = parsed.substr(remaining_flush_pos);
        } else {
            if (flush_start_pos != std::string::npos && flush_start_pos > 0) {
                prefix = parsed.substr(0, flush_start_pos);
                suffix = parsed.substr(flush_start_pos);
            } else {
                const size_t flush_end_after = find_flush_end_line_after(parsed);
                if (flush_end_after != std::string::npos && flush_end_after > 0) {
                    prefix = parsed.substr(0, flush_end_after);
                    suffix = parsed.substr(flush_end_after);
                } else {
                    std::string before_toolchange;
                    std::string toolchange_line;
                    std::string after_toolchange;
                    if (!split_last_toolchange_line_with_suffix(parsed, new_extruder_id, before_toolchange, toolchange_line, after_toolchange))
                        return "";
                    prefix = before_toolchange + toolchange_line;
                    suffix = after_toolchange;
                }
            }
        }
    }
    m_wipe.reset_path();
    std::string gcode = this->retract(true, false);

    const std::string& filament_end_gcode = m_config.filament_end_gcode.get_at(old_extruder_id);
    if (!filament_end_gcode.empty()) {
        DynamicConfig config;
        config.set_key_value("layer_num", new ConfigOptionInt(m_layer_index));
        config.set_key_value("layer_z", new ConfigOptionFloat(m_writer.get_position().z() - m_config.z_offset.value));
        config.set_key_value("max_layer_z", new ConfigOptionFloat(m_max_layer_z));
        config.set_key_value("filament_extruder_id", new ConfigOptionInt(int(old_extruder_id)));
        gcode += placeholder_parser_process("filament_end_gcode", filament_end_gcode, old_extruder_id, &config);
        check_add_eol(gcode);
    }

    if (m_ooze_prevention.enable)
        gcode += m_ooze_prevention.pre_toolchange(*this);

    const bool prefix_changes_tool = custom_gcode_changes_tool(prefix, m_writer.toolchange_prefix(), new_extruder_id);
    gcode += prefix;
    std::string toolchange_command = m_writer.toolchange(new_extruder_id, !prefix_changes_tool);
    if (!prefix_changes_tool)
        gcode += toolchange_command;
    if (prefix_changes_tool) {
        std::string before_toolchange;
        std::string toolchange_line;
        std::string after_toolchange;
        sync_writer_extruder_retraction_after_custom_gcode(m_writer, split_last_toolchange_line_with_suffix(prefix, new_extruder_id, before_toolchange, toolchange_line, after_toolchange) ? after_toolchange : prefix, true);
    }
    gcode += ";_FORCE_RESUME_FAN_SPEED\n";
    m_pending_skeleton_flush_return_position = position_after_custom_gcode(prefix, m_writer.get_position(), m_writer.get_xy_offset().cast<double>());
    m_pending_skeleton_flush_return_position_valid = true;
    m_writer.set_position(m_pending_skeleton_flush_return_position);
    m_writer.set_current_position_clear(false);

    ++m_toolchange_count;
    m_pending_skeleton_flush_toolchange = true;
    m_pending_skeleton_flush_old_extruder = old_extruder_id;
    m_pending_skeleton_flush_new_extruder = new_extruder_id;
    m_pending_skeleton_flush_toolchange_gcode.clear();
    m_pending_skeleton_flush_suffix_gcode = std::move(suffix);
    return gcode;
}

std::string GCode::finish_pending_skeleton_flush_toolchange(unsigned int extruder_id, double print_z, bool by_object, bool change_tool)
{
    if (!m_pending_skeleton_flush_toolchange || extruder_id != m_pending_skeleton_flush_new_extruder)
        return "";

    std::string toolchange_gcode_parsed = std::move(m_pending_skeleton_flush_suffix_gcode);
    m_pending_skeleton_flush_suffix_gcode.clear();
    m_pending_skeleton_flush_toolchange = false;

    std::string gcode;
    if (by_object)
        m_writer.add_object_change_labels(gcode);

    if (m_pending_skeleton_flush_return_position_valid && !toolchange_gcode_parsed.empty()) {
        m_wipe.reset_path();
        gcode += this->retract(false, false, LiftType::NormalLift);
        gcode += m_writer.travel_to_xyz(m_pending_skeleton_flush_return_position, "Travel to first flush position after skeleton");
    }
    m_pending_skeleton_flush_return_position_valid = false;

    const bool has_delayed_toolchange_gcode = !m_pending_skeleton_flush_toolchange_gcode.empty();
    if (has_delayed_toolchange_gcode)
        gcode += m_pending_skeleton_flush_toolchange_gcode;
    const bool needs_toolchange = m_writer.need_toolchange(extruder_id);
    std::string toolchange_command;
    if (needs_toolchange)
        toolchange_command = m_writer.toolchange(extruder_id, change_tool);
    if (!has_delayed_toolchange_gcode && needs_toolchange && !custom_gcode_changes_tool(toolchange_gcode_parsed, m_writer.toolchange_prefix(), extruder_id))
        gcode += toolchange_command;
    m_pending_skeleton_flush_toolchange_gcode.clear();

    gcode += toolchange_gcode_parsed;
    sync_writer_extruder_retraction_after_custom_gcode(m_writer, toolchange_gcode_parsed, true);
    gcode += ";_FORCE_RESUME_FAN_SPEED\n";
    m_writer.set_current_position_clear(false);
    double temp_z_after_tool_change;
    if (GCodeProcessor::get_last_z_from_gcode(toolchange_gcode_parsed, temp_z_after_tool_change)) {
        Vec3d pos = m_writer.get_position();
        pos(2) = temp_z_after_tool_change;
        m_writer.set_position(pos);
    }

    if (m_config.single_extruder_multi_material)
        m_writer.reset_e();

    if (m_config.single_extruder_multi_material && !m_config.enable_prime_tower) {
        int temp = (m_layer_index <= 0 ? m_config.nozzle_temperature_initial_layer.get_at(extruder_id) :
                                         m_config.nozzle_temperature.get_at(extruder_id));
        gcode += m_writer.set_temperature(temp, false);
    }

    this->placeholder_parser().set("current_extruder", extruder_id);
    this->placeholder_parser().set("retraction_distance_when_cut", m_config.retraction_distances_when_cut.get_at(extruder_id));
    this->placeholder_parser().set("long_retraction_when_cut", m_config.long_retractions_when_cut.get_at(extruder_id));

    const std::string& filament_start_gcode = m_config.filament_start_gcode.get_at(extruder_id);
    if (!filament_start_gcode.empty() && change_tool) {
        DynamicConfig config;
        config.set_key_value("layer_num", new ConfigOptionInt(m_layer_index));
        config.set_key_value("layer_z", new ConfigOptionFloat(this->writer().get_position().z() - m_config.z_offset.value));
        config.set_key_value("max_layer_z", new ConfigOptionFloat(m_max_layer_z));
        config.set_key_value("filament_extruder_id", new ConfigOptionInt(int(extruder_id)));
        gcode += this->placeholder_parser_process("filament_start_gcode", filament_start_gcode, extruder_id, &config);
        check_add_eol(gcode);
    }

    if (m_ooze_prevention.enable)
        gcode += m_ooze_prevention.post_toolchange(*this);

    if (m_config.enable_pressure_advance.get_at(extruder_id))
        gcode += m_writer.set_pressure_advance(m_config.pressure_advance.get_at(extruder_id));

    m_pending_skeleton_flush_old_extruder = (unsigned int)-1;
    m_pending_skeleton_flush_new_extruder = (unsigned int)-1;
    m_pending_skeleton_flush_toolchange_gcode.clear();
    m_pending_skeleton_flush_return_position_valid = false;
    m_pending_skeleton_flush_return_position = Vec3d::Zero();
    return gcode;
}

std::string GCode::set_extruder(unsigned int extruder_id, double print_z, bool by_object, bool change_tool)
{
    if (m_pending_skeleton_flush_toolchange && extruder_id == m_pending_skeleton_flush_new_extruder)
        return finish_pending_skeleton_flush_toolchange(extruder_id, print_z, by_object, change_tool);

    if (!m_writer.need_toolchange(extruder_id))
        return "";

    // if we are running a single-extruder setup, just set the extruder and return nothing
    if (!m_writer.multiple_extruders) {
        this->placeholder_parser().set("current_extruder", extruder_id);
        this->placeholder_parser().set("retraction_distance_when_cut", m_config.retraction_distances_when_cut.get_at(extruder_id));
        this->placeholder_parser().set("long_retraction_when_cut", m_config.long_retractions_when_cut.get_at(extruder_id));

        std::string gcode;
        // Append the filament start G-code.
        const std::string &filament_start_gcode = m_config.filament_start_gcode.get_at(extruder_id);
        if (! filament_start_gcode.empty()) {
            // Process the filament_start_gcode for the filament.
            DynamicConfig config;
            config.set_key_value("layer_num", new ConfigOptionInt(m_layer_index));
            config.set_key_value("layer_z", new ConfigOptionFloat(this->writer().get_position().z() - m_config.z_offset.value));
            config.set_key_value("max_layer_z", new ConfigOptionFloat(m_max_layer_z));
            config.set_key_value("filament_extruder_id", new ConfigOptionInt(int(extruder_id)));
            config.set_key_value("retraction_distance_when_cut",
                                 new ConfigOptionFloat(m_config.retraction_distances_when_cut.get_at(extruder_id)));
            config.set_key_value("long_retraction_when_cut", new ConfigOptionBool(m_config.long_retractions_when_cut.get_at(extruder_id)));

            //std::vector<double> _position(3,0);
            //_position[2] = print_z;
            //config.set_key_value("position", new ConfigOptionFloats(_position));
            gcode += this->placeholder_parser_process("filament_start_gcode", filament_start_gcode, extruder_id, &config);
            check_add_eol(gcode);
        }
        if (m_config.enable_pressure_advance.get_at(extruder_id)) {
            gcode += m_writer.set_pressure_advance(m_config.pressure_advance.get_at(extruder_id));
        }

        gcode += m_writer.toolchange(extruder_id, change_tool);
        return gcode;
    }

    // BBS. Should be placed before retract.
    m_toolchange_count++;

    // prepend retraction on the current extruder
    std::string gcode = this->retract(true, false);

    // Always reset the extrusion path, even if the tool change retract is set to zero.
    m_wipe.reset_path();

    // BBS: insert skip object label before change filament while by object
    if (by_object)
        m_writer.add_object_change_labels(gcode);

    if (m_writer.extruder() != nullptr) {
        // Process the custom filament_end_gcode. set_extruder() is only called if there is no wipe tower
        // so it should not be injected twice.
        unsigned int        old_extruder_id     = m_writer.extruder()->id();
        const std::string  &filament_end_gcode  = m_config.filament_end_gcode.get_at(old_extruder_id);
        if (! filament_end_gcode.empty()) {
            DynamicConfig config;
            config.set_key_value("layer_num", new ConfigOptionInt(m_layer_index));
            config.set_key_value("layer_z", new ConfigOptionFloat(m_writer.get_position().z() - m_config.z_offset.value));
            config.set_key_value("max_layer_z", new ConfigOptionFloat(m_max_layer_z));
            config.set_key_value("filament_extruder_id", new ConfigOptionInt(int(old_extruder_id)));
            gcode += placeholder_parser_process("filament_end_gcode", filament_end_gcode, old_extruder_id, &config);
            check_add_eol(gcode);
        }
    }


    // If ooze prevention is enabled, park current extruder in the nearest
    // standby point and set it to the standby temperature.
    if (m_ooze_prevention.enable && m_writer.extruder() != nullptr)
        gcode += m_ooze_prevention.pre_toolchange(*this);

    // BBS
    float new_retract_length = m_config.retraction_length.get_at(extruder_id);
    float new_retract_length_toolchange = m_config.retract_length_toolchange.get_at(extruder_id);
    int new_filament_temp = this->on_first_layer() ? m_config.nozzle_temperature_initial_layer.get_at(extruder_id): m_config.nozzle_temperature.get_at(extruder_id);
    // BBS: if print_z == 0 use first layer temperature
    if (abs(print_z) < EPSILON)
        new_filament_temp = m_config.nozzle_temperature_initial_layer.get_at(extruder_id);

    Vec3d nozzle_pos = m_writer.get_position();
    float old_retract_length, old_retract_length_toolchange, wipe_volume;
    int old_filament_temp, old_filament_e_feedrate;

    const float filament_diameter = m_config.filament_diameter.get_at(extruder_id);
    float filament_area = float((M_PI / 4.f) * filament_diameter * filament_diameter);
    //BBS: add handling for filament change in start gcode
    int previous_extruder_id = -1;
    if (m_writer.extruder() != nullptr || m_start_gcode_filament != -1) {
        const auto& flush_matrix = m_config.flush_volumes_matrix.values;
        const unsigned int number_of_extruders = (unsigned int)(sqrt(flush_matrix.size()) + EPSILON);
        if (m_writer.extruder() != nullptr)
            assert(m_writer.extruder()->id() < number_of_extruders);
        else
            assert(m_start_gcode_filament < number_of_extruders);

        previous_extruder_id = m_writer.extruder() != nullptr ? m_writer.extruder()->id() : m_start_gcode_filament;
        old_retract_length = m_config.retraction_length.get_at(previous_extruder_id);
        old_retract_length_toolchange = m_config.retract_length_toolchange.get_at(previous_extruder_id);
        old_filament_temp = this->on_first_layer()? m_config.nozzle_temperature_initial_layer.get_at(previous_extruder_id) : m_config.nozzle_temperature.get_at(previous_extruder_id);
        if (!m_config.purge_in_prime_tower || is_BBL_Printer() ) {
            wipe_volume = flush_matrix[previous_extruder_id * number_of_extruders + extruder_id];
            wipe_volume *= m_config.flush_multiplier;
        } else {
            wipe_volume = 0;
            //std::max<float>(m_config.prime_volume, m_config.filament_minimal_purge_on_wipe_tower.get_at(previous_extruder_id));
        }
        old_filament_e_feedrate = (int)(60.0 * m_config.filament_max_volumetric_speed.get_at(previous_extruder_id) / filament_area);
        old_filament_e_feedrate = old_filament_e_feedrate == 0 ? 100 : old_filament_e_feedrate;
        //BBS: must clean m_start_gcode_filament
        m_start_gcode_filament = -1;
    } else {
        old_retract_length = 0.f;
        old_retract_length_toolchange = 0.f;
        old_filament_temp = 0;
        wipe_volume = 0.f;
        old_filament_e_feedrate = 200;
    }
#ifdef SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
    const unsigned int cut_retraction_extruder_id = previous_extruder_id >= 0 ? (unsigned int)previous_extruder_id : extruder_id;
    const float effective_cut_retraction_distance = this->effective_retraction_distance_when_cut(cut_retraction_extruder_id);
    ConfigOptionFloats effective_cut_retraction_distances = this->effective_retraction_distances_when_cut(cut_retraction_extruder_id);
#endif // SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
   // wipe_volume       = 960;
    /*if (wipe_volume > 1500)
    {
        wipe_volume = 1500;
    }*/
    float wipe_length = wipe_volume / filament_area;

    int new_filament_e_feedrate = (int)(60.0 * m_config.filament_max_volumetric_speed.get_at(extruder_id) / filament_area);
    new_filament_e_feedrate = new_filament_e_feedrate == 0 ? 100 : new_filament_e_feedrate;

    const int parser_previous_extruder_id = m_writer.extruder() != nullptr ? int(m_writer.extruder()->id()) : -1;
    const bool is_previous_extruder_last_use = parser_previous_extruder_id >= 0 &&
                                               parser_previous_extruder_id != int(extruder_id) &&
                                               static_cast<size_t>(parser_previous_extruder_id) < m_remaining_extruder_segment_uses.size() &&
                                               m_remaining_extruder_segment_uses[static_cast<size_t>(parser_previous_extruder_id)] == 0;
    DynamicConfig dyn_config;
    dyn_config.set_key_value("previous_extruder", new ConfigOptionInt(previous_extruder_id));
    dyn_config.set_key_value("next_extruder", new ConfigOptionInt((int)extruder_id));
    dyn_config.set_key_value("is_previous_extruder_last_use", new ConfigOptionBool(is_previous_extruder_last_use));
    dyn_config.set_key_value("layer_num", new ConfigOptionInt(m_layer_index));
    dyn_config.set_key_value("layer_z", new ConfigOptionFloat(print_z));
    dyn_config.set_key_value("max_layer_z", new ConfigOptionFloat(m_max_layer_z));
    dyn_config.set_key_value("relative_e_axis", new ConfigOptionBool(m_config.use_relative_e_distances));
    dyn_config.set_key_value("toolchange_count", new ConfigOptionInt((int)m_toolchange_count));
    //BBS: fan speed is useless placeholer now, but we don't remove it to avoid
    //slicing error in old change_filament_gcode in old 3MF
    dyn_config.set_key_value("fan_speed", new ConfigOptionInt((int)0));
    dyn_config.set_key_value("flush_into_skeleton", new ConfigOptionBool(false));
    dyn_config.set_key_value("old_retract_length", new ConfigOptionFloat(old_retract_length));
    dyn_config.set_key_value("new_retract_length", new ConfigOptionFloat(new_retract_length));
    dyn_config.set_key_value("old_retract_length_toolchange", new ConfigOptionFloat(old_retract_length_toolchange));
    dyn_config.set_key_value("new_retract_length_toolchange", new ConfigOptionFloat(new_retract_length_toolchange));
    dyn_config.set_key_value("old_filament_temp", new ConfigOptionInt(old_filament_temp));
    dyn_config.set_key_value("new_filament_temp", new ConfigOptionInt(new_filament_temp));
    dyn_config.set_key_value("x_after_toolchange", new ConfigOptionFloat(nozzle_pos(0)));
    dyn_config.set_key_value("y_after_toolchange", new ConfigOptionFloat(nozzle_pos(1)));
    dyn_config.set_key_value("z_after_toolchange", new ConfigOptionFloat(nozzle_pos(2)));
    dyn_config.set_key_value("first_flush_volume", new ConfigOptionFloat(wipe_length / 2.f));
    dyn_config.set_key_value("second_flush_volume", new ConfigOptionFloat(wipe_length / 2.f));
    dyn_config.set_key_value("old_filament_e_feedrate", new ConfigOptionInt(old_filament_e_feedrate));
    dyn_config.set_key_value("new_filament_e_feedrate", new ConfigOptionInt(new_filament_e_feedrate));
    dyn_config.set_key_value("travel_point_1_x", new ConfigOptionFloat(float(travel_point_1.x())));
    dyn_config.set_key_value("travel_point_1_y", new ConfigOptionFloat(float(travel_point_1.y())));
    dyn_config.set_key_value("travel_point_2_x", new ConfigOptionFloat(float(travel_point_2.x())));
    dyn_config.set_key_value("travel_point_2_y", new ConfigOptionFloat(float(travel_point_2.y())));
    dyn_config.set_key_value("travel_point_3_x", new ConfigOptionFloat(float(travel_point_3.x())));
    dyn_config.set_key_value("travel_point_3_y", new ConfigOptionFloat(float(travel_point_3.y())));
#ifdef SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
    dyn_config.set_key_value("retraction_distance_when_cut", new ConfigOptionFloat(effective_cut_retraction_distance));
    dyn_config.set_key_value("retraction_distances_when_cut", new ConfigOptionFloats(effective_cut_retraction_distances));
    dyn_config.set_key_value("long_retraction_when_cut", new ConfigOptionBool(m_config.long_retractions_when_cut.get_at(cut_retraction_extruder_id)));
#endif // SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
    // Provide sane defaults for wipe tower placeholders even when no wipe tower is generated.
    dyn_config.set_key_value("wipe_tower_start_position_x", new ConfigOptionFloat(nozzle_pos(0)));
    dyn_config.set_key_value("wipe_tower_start_position_y", new ConfigOptionFloat(nozzle_pos(1)));
    dyn_config.set_key_value("wipe_tower_random_num", new ConfigOptionFloat(0.f));
    dyn_config.set_key_value("wipe_tower_outer_wall_x", new ConfigOptionFloat(nozzle_pos(0)));
    dyn_config.set_key_value("wipe_tower_outer_wall_y", new ConfigOptionFloat(nozzle_pos(1)));
    dyn_config.set_key_value("flush_length", new ConfigOptionFloat(wipe_length));
    /*
    int flush_count = std::min(g_max_flush_count, (int)std::round(wipe_volume / g_purge_volume_one_time));
    float flush_unit = wipe_length / flush_count;
    int flush_idx = 0;
    for (; flush_idx < flush_count; flush_idx++) {
        char key_value[64] = { 0 };
        snprintf(key_value, sizeof(key_value), "flush_length_%d", flush_idx + 1);
        dyn_config.set_key_value(key_value, new ConfigOptionFloat(flush_unit));
    }

    for (; flush_idx < g_max_flush_count; flush_idx++) {
        char key_value[64] = { 0 };
        snprintf(key_value, sizeof(key_value), "flush_length_%d", flush_idx + 1);
        dyn_config.set_key_value(key_value, new ConfigOptionFloat(0.f));
    }*/
    FlushConfig fig;
    fig.box_first_clean_length    = std::max(1, m_config.flush_box_first_clean_length.value);
    fig.box_need_clean_length     = std::max(1, m_config.flush_box_need_clean_length.value);
    fig.box_need_clean_length_max = std::max(1, m_config.flush_box_need_clean_length_max.value);

    std::vector<float> cal_length;
    if (wipe_length > 0) {
        cal_flush_list(wipe_length, cal_length, fig);
    }
    int flush_idx = 0;
    for (; flush_idx < cal_length.size(); flush_idx++) {
        char key_value[64] = {0};
        snprintf(key_value, sizeof(key_value), "flush_length_%d", flush_idx + 1);
        dyn_config.set_key_value(key_value, new ConfigOptionFloat(cal_length[flush_idx]));
    }
    for (; flush_idx < g_max_flush_count; flush_idx++) {
        char key_value[64] = {0};
        snprintf(key_value, sizeof(key_value), "flush_length_%d", flush_idx + 1);
        dyn_config.set_key_value(key_value, new ConfigOptionFloat(0.f));
    }
    // Process the custom change_filament_gcode.
    const std::string& change_filament_gcode = m_config.change_filament_gcode.value;
    std::string toolchange_gcode_parsed;
    //Orca: Ignore change_filament_gcode if is the first call for a tool change and manual_filament_change is enabled
    if (!change_filament_gcode.empty() && !(m_config.manual_filament_change.value && m_toolchange_count == 1) && change_tool) {
        dyn_config.set_key_value("previous_extruder", new ConfigOptionInt(parser_previous_extruder_id));
        dyn_config.set_key_value("next_extruder", new ConfigOptionInt((int) extruder_id));
        dyn_config.set_key_value("layer_num", new ConfigOptionInt(m_layer_index));
        dyn_config.set_key_value("layer_z", new ConfigOptionFloat(print_z));
        dyn_config.set_key_value("toolchange_z", new ConfigOptionFloat(print_z));
        dyn_config.set_key_value("max_layer_z", new ConfigOptionFloat(m_max_layer_z));

        toolchange_gcode_parsed = placeholder_parser_process("change_filament_gcode", change_filament_gcode, extruder_id, &dyn_config);
        check_add_eol(toolchange_gcode_parsed);


        //if (!m_config.single_extruder_multi_material && m_placeholder_parser_integration.num_extruders > 1 && m_powerPos != Vec2d::Zero()) {
        //    gcode += m_writer.travel_to_xy(m_powerPos);
        //}
        //else
        //if (!m_config.single_extruder_multi_material && m_config.print_sequence.value == PrintSequence::ByObject && change_tool) {
        //    gcode += m_writer.travel_to_xy(Vec2d::Zero());
        //}

        gcode += toolchange_gcode_parsed;

        //BBS
        {
            //BBS: gcode writer doesn't know where the extruder is and whether fan speed is changed after inserting tool change gcode
            //Set this flag so that normal lift will be used the first time after tool change.
            gcode += ";_FORCE_RESUME_FAN_SPEED\n";
            m_writer.set_current_position_clear(false);
            //BBS: check whether custom gcode changes the z position. Update if changed
            double temp_z_after_tool_change;
            if (GCodeProcessor::get_last_z_from_gcode(toolchange_gcode_parsed, temp_z_after_tool_change)) {
                Vec3d pos = m_writer.get_position();
                pos(2) = temp_z_after_tool_change;
                m_writer.set_position(pos);
            }
        }
    }

    // BBS. Reset old extruder E-value.
    // Keep retract length because Custom GCode will guarantee retract length be the same as toolchange
    if (m_config.single_extruder_multi_material) {
        m_writer.reset_e();
    }

    // We inform the writer about what is happening, but we may not use the resulting gcode.
    std::string toolchange_command = m_writer.toolchange(extruder_id, change_tool);
    if (!custom_gcode_changes_tool(toolchange_gcode_parsed, m_writer.toolchange_prefix(), extruder_id))
        gcode += toolchange_command;
    else {
        // user provided his own toolchange gcode, no need to do anything
    }



   bool bchange = m_writer.need_toolchange(extruder_id);

    // Set the temperature if the wipe tower didn't (not needed for non-single extruder MM)
    if (m_config.single_extruder_multi_material && !m_config.enable_prime_tower) {
        int temp = (m_layer_index <= 0 ? m_config.nozzle_temperature_initial_layer.get_at(extruder_id) :
                                         m_config.nozzle_temperature.get_at(extruder_id));

        gcode += m_writer.set_temperature(temp, false);
    }

    this->placeholder_parser().set("current_extruder", extruder_id);
#ifdef SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
    this->placeholder_parser().set("retraction_distance_when_cut", effective_cut_retraction_distance);
    this->placeholder_parser().set("retraction_distances_when_cut", new ConfigOptionFloats(effective_cut_retraction_distances));
    this->placeholder_parser().set("long_retraction_when_cut", m_config.long_retractions_when_cut.get_at(cut_retraction_extruder_id));
#else
    this->placeholder_parser().set("retraction_distance_when_cut", m_config.retraction_distances_when_cut.get_at(extruder_id));
    this->placeholder_parser().set("long_retraction_when_cut", m_config.long_retractions_when_cut.get_at(extruder_id));
#endif // SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT

    // Append the filament start G-code.
    const std::string &filament_start_gcode = m_config.filament_start_gcode.get_at(extruder_id);
    if (! filament_start_gcode.empty()) {
        // Process the filament_start_gcode for the new filament.
        DynamicConfig config;
        config.set_key_value("layer_num", new ConfigOptionInt(m_layer_index));
        config.set_key_value("layer_z", new ConfigOptionFloat(this->writer().get_position().z() - m_config.z_offset.value));
        config.set_key_value("max_layer_z", new ConfigOptionFloat(m_max_layer_z));
        config.set_key_value("filament_extruder_id", new ConfigOptionInt(int(extruder_id)));
#ifdef SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
        config.set_key_value("retraction_distance_when_cut", new ConfigOptionFloat(effective_cut_retraction_distance));
        config.set_key_value("retraction_distances_when_cut", new ConfigOptionFloats(effective_cut_retraction_distances));
        config.set_key_value("long_retraction_when_cut", new ConfigOptionBool(m_config.long_retractions_when_cut.get_at(cut_retraction_extruder_id)));
#endif // SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT

        //std::vector<double> _position(3, 0);
        //_position[2] = print_z;
        //config.set_key_value("position", new ConfigOptionFloats(_position));

        gcode += this->placeholder_parser_process("filament_start_gcode", filament_start_gcode, extruder_id, &config);

        check_add_eol(gcode);
    }
    // Set the new extruder to the operating temperature.
    if (m_ooze_prevention.enable)
        gcode += m_ooze_prevention.post_toolchange(*this);

    if (m_config.enable_pressure_advance.get_at(extruder_id)) {
        gcode += m_writer.set_pressure_advance(m_config.pressure_advance.get_at(extruder_id));
    }

    return gcode;
}



void reducePolygonPoints(Polygon& poly, int maxPoints, float tolerance)
{
    while (poly.size() > maxPoints && tolerance < 100000) { // ??????
        Polygons pls;
        pls = poly.simplify(tolerance);
        tolerance *= 1.2; // ??????
        if (pls.size() == 1)
        {
            poly = pls[0];
        }
        else
        {
            break;
        }
    }
}

inline std::string polygon_to_string(const Polygon &polygon, Print* print, bool is_convex_hull = false,bool is_print_space = false)
{
    Polygon simply_poly = polygon;
    if (is_convex_hull)
    {
        reducePolygonPoints(simply_poly, 100, 1.0); // 简化输出多边形凸包 控制少于100个点
    }

    std::ostringstream gcode;
    gcode << "[";
    for (const Point& p : simply_poly.points) {
        const auto v = is_print_space ? Vec2d(p.x(), p.y()) : print->translate_to_print_space(p);
        gcode << "[" << v.x() << "," << v.y() << "],";
    }
    if (!simply_poly.points.empty()) {
        const auto first_v = is_print_space ? Vec2d(simply_poly.points.front().x(), simply_poly.points.front().y()) :
                                              print->translate_to_print_space(simply_poly.points.front());
        gcode << "[" << first_v.x() << "," << first_v.y() << "]";
    }
    gcode << "]";
    return gcode.str();
}
// this function iterator PrintObject and assign a seqential id to each object.
// this id is used to generate unique object id for each object.
std::string GCode::set_object_info(Print *print) {
    const auto gflavor = print->config().gcode_flavor.value;
    if (print->is_BBL_printer() ||
        (gflavor != gcfKlipper && gflavor != gcfMarlinLegacy && gflavor != gcfMarlinFirmware && gflavor != gcfRepRapFirmware))
        return "";
    std::ostringstream gcode;
    size_t object_id = 0;
    // Orca: check if we are in pa calib mode
    if (print->calib_mode() == CalibMode::Calib_PA_Line || print->calib_mode() == CalibMode::Calib_PA_Pattern) {
        BoundingBoxf bbox_bed(print->config().printable_area.values);
        bbox_bed.offset(-25.0);
        Polygon polygon_bed;
        polygon_bed.append(Point(bbox_bed.min.x(), bbox_bed.min.y()));
        polygon_bed.append(Point(bbox_bed.max.x(), bbox_bed.min.y()));
        polygon_bed.append(Point(bbox_bed.max.x(), bbox_bed.max.y()));
        polygon_bed.append(Point(bbox_bed.min.x(), bbox_bed.max.y()));
        gcode << "EXCLUDE_OBJECT_DEFINE NAME="
              << "Orca-PA-Calibration-Test"
              << " CENTER=" << 0 << "," << 0 << " POLYGON=" << polygon_to_string(polygon_bed, print, false,true) << "\n";
    } else {
        size_t unique_id = 0;
        for (PrintObject* object : print->objects()) {
            object->set_id(object_id++);
            size_t inst_id = 0;
            for (PrintInstance& inst : object->instances()) {
                inst.unique_id = unique_id++;
                inst.id        = inst_id++;
                auto bbox      = inst.get_bounding_box();
                auto center    = print->translate_to_print_space(Vec2d(bbox.center().x(), bbox.center().y()));
                auto inst_name = get_instance_name(object, inst);
                if (gflavor == gcfKlipper) {
                    gcode << "EXCLUDE_OBJECT_DEFINE NAME=" << inst_name << " CENTER=" << center.x() << "," << center.y()
                          << " POLYGON=" << polygon_to_string(inst.get_convex_hull_2d(), print,true,false) << "\n";
                } else if (gflavor == gcfMarlinLegacy || gflavor == gcfMarlinFirmware || gflavor == gcfRepRapFirmware) {
                    gcode << "M486 S" << std::to_string(inst.unique_id);
                    if (gflavor == gcfRepRapFirmware)
                        gcode << " A"
                              << "\"" << inst_name << "\"";
                    else
                        gcode << "\nM486 A" << inst_name;
                    gcode << "\nM486 S-1\n";
                }
            }
        }
    }

    return gcode.str();
}

// convert a model-space scaled point into G-code coordinates
Vec2d GCode::point_to_gcode(const Point &point) const
{
    Vec2d extruder_offset = EXTRUDER_CONFIG(extruder_offset);
    return unscale(point) + m_origin - extruder_offset;
}

// convert a model-space scaled point into G-code coordinates
Point GCode::gcode_to_point(const Vec2d &point) const
{
    Vec2d extruder_offset = EXTRUDER_CONFIG(extruder_offset);
    return Point(
        scale_(point(0) - m_origin(0) + extruder_offset(0)),
        scale_(point(1) - m_origin(1) + extruder_offset(1)));
}

Vec2d GCode::point_to_gcode_quantized(const Point& point) const
{
    Vec2d p = this->point_to_gcode(point);
    return { GCodeFormatter::quantize_xyzf(p.x()), GCodeFormatter::quantize_xyzf(p.y()) };
}


// Goes through by_region std::vector and returns reference to a subvector of entities, that are to be printed
// during infill/perimeter wiping, or normally (depends on wiping_entities parameter)
// Fills in by_region_per_copy_cache and returns its reference.
const std::vector<GCode::ObjectByExtruder::Island::Region>& GCode::ObjectByExtruder::Island::by_region_per_copy(std::vector<Region> &by_region_per_copy_cache, unsigned int copy, unsigned int extruder, bool wiping_entities) const
{
    bool has_overrides = false;
    for (const auto& reg : by_region)
        if (! reg.infills_overrides.empty() || ! reg.perimeters_overrides.empty()) {
            has_overrides = true;
            break;
        }

    // Data is cleared, but the memory is not.
    by_region_per_copy_cache.clear();

    if (! has_overrides)
        // Simple case. No need to copy the regions.
        return wiping_entities ? by_region_per_copy_cache : this->by_region;

    // Complex case. Some extrusions are used for wiping, while first-flush skeleton stays with
    // the old extruder normal pass so it is printed after that object's walls and skin.
    for (const auto& reg : by_region) {
        by_region_per_copy_cache.emplace_back();

        for (int iter = 0; iter < 2; ++iter) {
            const ExtrusionEntitiesPtr& entities   = iter ? reg.infills : reg.perimeters;
            ExtrusionEntitiesPtr&       target_eec = iter ? by_region_per_copy_cache.back().infills : by_region_per_copy_cache.back().perimeters;
            const std::vector<const WipingExtrusions::ExtruderPerCopy*>& overrides = iter ? reg.infills_overrides : reg.perimeters_overrides;

            if (wiping_entities) {
                for (unsigned int i = 0; i < overrides.size(); ++ i) {
                    const WipingExtrusions::ExtruderPerCopy *this_override = overrides[i];
                    const bool skeleton_flush = entities[i]->role() == erInternalInfill;
                    if (this_override != nullptr && (*this_override)[copy] == int(extruder) && !skeleton_flush)
                        target_eec.emplace_back(entities[i]);
                }
            } else {
                unsigned int i = 0;
                for (; i < overrides.size(); ++ i) {
                    const WipingExtrusions::ExtruderPerCopy *this_override = overrides[i];
                    const bool skeleton_flush = entities[i]->role() == erInternalInfill;
                    if (this_override == nullptr || (*this_override)[copy] == -int(extruder)-1 ||
                        (skeleton_flush && (*this_override)[copy] == int(extruder)))
                        target_eec.emplace_back(entities[i]);
                }
                for (; i < entities.size(); ++ i)
                    target_eec.emplace_back(entities[i]);
            }
        }
    }
    return by_region_per_copy_cache;
}
// This function takes the eec and appends its entities to either perimeters or infills of this Region (depending on the first parameter)
// It also saves pointer to ExtruderPerCopy struct (for each entity), that holds information about which extruders should be used for which copy.
void GCode::ObjectByExtruder::Island::Region::append(const Type type, const ExtrusionEntityCollection* eec, const WipingExtrusions::ExtruderPerCopy* copies_extruder)
{
    // We are going to manipulate either perimeters or infills, exactly in the same way. Let's create pointers to the proper structure to not repeat ourselves:
    ExtrusionEntitiesPtr*									perimeters_or_infills;
    std::vector<const WipingExtrusions::ExtruderPerCopy*>* 	perimeters_or_infills_overrides;

    switch (type) {
    case PERIMETERS:
        perimeters_or_infills 			= &perimeters;
        perimeters_or_infills_overrides = &perimeters_overrides;
        break;
    case INFILL:
        perimeters_or_infills 			= &infills;
        perimeters_or_infills_overrides = &infills_overrides;
        break;
    default:
    	throw Slic3r::InvalidArgument("Unknown parameter!");
    }

    // First we append the entities, there are eec->entities.size() of them:
    size_t old_size = perimeters_or_infills->size();
    size_t new_size = old_size + (eec->can_sort() ? eec->entities.size() : 1);
    perimeters_or_infills->reserve(new_size);
    if (eec->can_sort()) {
        for (auto* ee : eec->entities)
            perimeters_or_infills->emplace_back(ee);
    } else
        perimeters_or_infills->emplace_back(const_cast<ExtrusionEntityCollection*>(eec));

    if (copies_extruder != nullptr) {
        // Don't reallocate overrides if not needed.
        // Missing overrides are implicitely considered non-overridden.
        perimeters_or_infills_overrides->reserve(new_size);
        perimeters_or_infills_overrides->resize(old_size, nullptr);
        perimeters_or_infills_overrides->resize(new_size, copies_extruder);
    }
}


// Index into std::vector<LayerToPrint>, which contains Object and Support layers for the current print_z, collected for
// a single object, or for possibly multiple objects with multiple instances.

} // namespace Slic3r
