#include "PathologicalSegmentAccelFilter.hpp"
#include "../Exception.hpp"
#include "../I18N.hpp"
#include "../Utils.hpp"

#include <cmath>
#include <fstream>
#include <cctype>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <stdexcept>
#include <thread>

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/task_arena.h>

#include "KlipperSim/KlipperSimMain.hpp"

namespace Slic3r {

// ---------------------------------------------------------------------------
// Detection is delegated to the ported Klipper motion pipeline (KlipperSim):
//   gcode -> lookahead -> itersolve (CoreXY A/B, Extruder+PA) -> stepcompress
//         -> per-MCU shared move-pool occupancy.
// A line is "pathological" when its motion contributes to an MCU move-queue
// occupancy at/above OCCUPANCY_THRESHOLD (fraction of pool). Such regions get
// SET_VELOCITY_LIMIT ACCEL reductions to lower the command production rate.
//
// Align this threshold with the host-side backpressure start point added in
// 119_klipper/toolhead.py (_MQ_BACKPRESSURE_START = 70%). For slicer-side preemptive mitigation we keep an additional margin above that runtime backpressure start, because the successful and ball samples both climb into the 70% range without overflowing. Using 50% here causes
// safe-but-busy models (for example the ball sample) to be throttled even
// though Klipper itself would not yet apply queue protection.
// ---------------------------------------------------------------------------

// Occupancy fraction at/above which a region is treated as pathological.
static constexpr double OCCUPANCY_THRESHOLD = 0.80;
// Merge nearby flagged regions while allowing at most this many executable
// G-code commands between them. Blank and comment-only lines do not consume the budget.
static constexpr int    MERGE_GAP_COMMANDS  = 20;
static constexpr double AUE_MAX_RAMP_SEGMENT_MM = 1.0;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static inline std::string_view trim_left(std::string_view s)
{
    size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i;
    return s.substr(i);
}

static bool is_executable_gcode(std::string_view sv)
{
    sv = trim_left(sv);
    return !(sv.empty() || sv.front() == ';' || sv.front() == '('
             || sv.front() == '\r' || sv.front() == '\n' || sv.front() == '%');
}

static std::string float_to_string(double v)
{
    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss.precision(1);
    oss << v;
    return oss.str();
}

static std::string coordinate_to_string(double v)
{
    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss.precision(6);
    oss << v;
    std::string result = oss.str();
    while (result.size() > 2 && result.back() == '0')
        result.pop_back();
    if (!result.empty() && result.back() == '.')
        result.push_back('0');
    return result;
}

static std::string set_motion_feedrate(std::string_view sv, double speed_mm_s)
{
    std::string result(sv);
    size_t code_end = result.size();
    for (char terminator : {';', '\r', '\n'}) {
        const size_t pos = result.find(terminator);
        if (pos != std::string::npos)
            code_end = std::min(code_end, pos);
    }

    for (size_t i = 0; i < code_end; ++i) {
        char c = result[i];
        if (c >= 'a' && c <= 'z')
            c -= 32;
        if (c != 'F' || (i > 0 && result[i - 1] != ' ' && result[i - 1] != '\t'))
            continue;
        const size_t value_begin = i + 1;
        char* value_end = nullptr;
        std::strtod(result.c_str() + value_begin, &value_end);
        if (value_end == result.c_str() + value_begin)
            continue;
        const size_t value_end_pos = static_cast<size_t>(value_end - result.c_str());
        result.replace(value_begin, value_end_pos - value_begin,
                       float_to_string(speed_mm_s * 60.0));
        return result;
    }

    size_t insert_pos = code_end;
    while (insert_pos > 0
           && (result[insert_pos - 1] == ' ' || result[insert_pos - 1] == '\t'))
        --insert_pos;
    std::string feed_word;
    if (insert_pos > 0 && result[insert_pos - 1] != ' ' && result[insert_pos - 1] != '\t')
        feed_word += ' ';
    feed_word += 'F';
    feed_word += float_to_string(speed_mm_s * 60.0);
    result.insert(insert_pos, feed_word);
    return result;
}

static std::string clamp_velocity_limit_line(std::string_view sv,
                                               double safe_accel,
                                               double safe_decel)
{
    std::string result(sv);
    auto clamp_param = [&](const char* key, double limit) {
        for (int casing = 0; casing < 2; ++casing) {
            std::string k(key);
            if (casing == 1)
                for (char& c : k) c = char(tolower((unsigned char)c));
            size_t pos = 0;
            while (pos < result.size()) {
                pos = result.find(k, pos);
                if (pos == std::string::npos) break;
                if (pos > 0 && result[pos - 1] != ' ' && result[pos - 1] != '\t') {
                    ++pos;
                    continue;
                }
                size_t val_start = pos + k.size();
                if (val_start < result.size()) {
                    size_t val_end = val_start;
                    while (val_end < result.size() &&
                           result[val_end] != ' ' && result[val_end] != '\t' &&
                           result[val_end] != ';' && result[val_end] != '\r' &&
                           result[val_end] != '\n')
                        ++val_end;
                    if (val_end > val_start) {
                        char* endp = nullptr;
                        double v = std::strtod(result.c_str() + val_start, &endp);
                        if (endp > result.c_str() + val_start && v > limit) {
                            char buf[64];
                            std::snprintf(buf, sizeof(buf), "%.1f", limit);
                            result.replace(val_start, val_end - val_start, buf);
                        }
                    }
                    return;
                }
                pos += k.size();
            }
        }
    };

    clamp_param("ACCEL=", safe_accel);
    clamp_param("ACCEL_TO_DECEL=", safe_decel);
    return result;
}
static std::vector<std::string> split_lines(const std::string& gcode)
{
    std::vector<std::string> lines;
    lines.reserve(std::count(gcode.begin(), gcode.end(), '\n') + 1);
    size_t start = 0;
    while (start < gcode.size()) {
        size_t pos = gcode.find('\n', start);
        if (pos == std::string::npos) { lines.emplace_back(gcode.substr(start)); break; }
        lines.emplace_back(gcode.substr(start, pos - start + 1));
        start = pos + 1;
    }
    return lines;
}

static bool line_has_command(std::string_view line, std::string_view command)
{
    line = trim_left(line);
    if (line.size() < command.size())
        return false;
    for (size_t i = 0; i < command.size(); ++i) {
        char lhs = line[i];
        char rhs = command[i];
        if (lhs >= 'a' && lhs <= 'z') lhs -= 32;
        if (rhs >= 'a' && rhs <= 'z') rhs -= 32;
        if (lhs != rhs)
            return false;
    }
    return line.size() == command.size()
        || line[command.size()] == ' ' || line[command.size()] == '\t'
        || line[command.size()] == ';' || line[command.size()] == '\r'
        || line[command.size()] == '\n';
}

static std::string_view line_without_eol(std::string_view line)
{
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
        line.remove_suffix(1);
    return line;
}

static bool whitespace_only(std::string_view line)
{
    for (const char ch : line) {
        if (ch != ' ' && ch != '\t')
            return false;
    }
    return true;
}

static bool comment_only_line(std::string_view line)
{
    line = trim_left(line_without_eol(line));
    while (!line.empty()) {
        if (line.front() == ';')
            return true;
        if (line.front() != '(')
            return false;
        const size_t close = line.find(')');
        if (close == std::string_view::npos)
            return false;
        line.remove_prefix(close + 1);
        line = trim_left(line);
    }
    return true;
}

static bool prescan_ignorable_line(std::string_view line)
{
    line = trim_left(line_without_eol(line));
    return line.empty() || comment_only_line(line)
        || (line.front() == '%' && whitespace_only(line.substr(1)))
        || line_has_command(line, "M73");
}

static bool has_analysis_payload(
    const PathologicalSegmentAnalysisResult& result, size_t index)
{
    return (index < result.flags.size() && result.flags[index] != 0)
        || (index < result.positive_extrusion_cruise_by_line.size()
            && std::abs(result.positive_extrusion_cruise_by_line[index]) > 1e-12)
        || (index < result.positive_extrusion_distance_by_line.size()
            && std::abs(result.positive_extrusion_distance_by_line[index]) > 1e-12);
}

static bool map_prescan_analysis(
    const PathologicalSegmentAnalysisResult& source,
    const std::vector<std::string>& final_lines,
    std::vector<char>& flags,
    std::vector<double>& cruise,
    std::vector<double>& distance,
    std::string& reason,
    const std::function<void()>& cancel)
{
    const size_t source_count = source.source_lines.size();
    if (source.flags.size() != source_count
        || source.positive_extrusion_cruise_by_line.size() != source_count
        || source.positive_extrusion_distance_by_line.size() != source_count) {
        reason = "vector_size_mismatch";
        return false;
    }

    flags.assign(final_lines.size(), 0);
    cruise.assign(final_lines.size(), 0.0);
    distance.assign(final_lines.size(), 0.0);
    size_t source_index = 0;
    size_t final_index = 0;
    size_t mapping_steps = 0;
    while (source_index < source_count && final_index < final_lines.size()) {
        if (cancel && ((mapping_steps++ & 0xfffu) == 0))
            cancel();
        const std::string_view source_line =
            line_without_eol(source.source_lines[source_index]);
        const std::string_view final_line =
            line_without_eol(final_lines[final_index]);
        if (source_line == final_line) {
            flags[final_index] = source.flags[source_index];
            cruise[final_index] =
                source.positive_extrusion_cruise_by_line[source_index];
            distance[final_index] =
                source.positive_extrusion_distance_by_line[source_index];
            ++source_index;
            ++final_index;
        } else if (prescan_ignorable_line(source_line)) {
            if (has_analysis_payload(source, source_index)) {
                reason = "ignored_source_has_payload";
                return false;
            }
            ++source_index;
        } else if (prescan_ignorable_line(final_line)) {
            ++final_index;
        } else {
            reason = "substantive_line_mismatch";
            return false;
        }
    }
    while (source_index < source_count) {
        if (cancel && ((mapping_steps++ & 0xfffu) == 0))
            cancel();
        if (!prescan_ignorable_line(source.source_lines[source_index])
            || has_analysis_payload(source, source_index)) {
            reason = "unmapped_source_line";
            return false;
        }
        ++source_index;
    }
    while (final_index < final_lines.size()) {
        if (cancel && ((mapping_steps++ & 0xfffu) == 0))
            cancel();
        if (!prescan_ignorable_line(final_lines[final_index])) {
            reason = "unmapped_final_line";
            return false;
        }
        ++final_index;
    }
    return true;
}

// ---------------------------------------------------------------------------
// State: carries the simulator's cross-layer parse state + accel bookkeeping.
// ---------------------------------------------------------------------------
struct PathologicalSegmentAccelFilter::State
{
    KlipperSim::LineAnalysisState sim_state;
    bool   accel_known       = false;
    std::string buffered_gcode;
    bool   aue_pending = false;
    bool   aue_active = false;
    double aue_v_low = 0.0;
    double aue_v_target = 0.0;
    double aue_accel = 0.0;
    double aue_distance = 0.0;
};

// ---------------------------------------------------------------------------
PathologicalSegmentAccelFilter::PathologicalSegmentAccelFilter(
    bool enabled, GCodeFlavor flavor,
    std::function<void()> cancel_callback,
    std::shared_ptr<PathologicalLineAnalysis> prescan,
    KlipperSim::SimConfig sim_config,
    std::function<void(int, const std::string&)> status_callback)
    : m_flavor(flavor)
    , m_cancel(std::move(cancel_callback))
    , m_prescan(std::move(prescan))
    , m_sim_config(std::move(sim_config))
    , m_status_callback(std::move(status_callback))
    , m_state(std::make_unique<State>())
{
    m_config.enabled = enabled;
}

PathologicalSegmentAccelFilter::~PathologicalSegmentAccelFilter() = default;

std::string PathologicalSegmentAccelFilter::apply_with_prescan(
    std::string&& gcode, bool compact_streaming)
{
    if (!m_state)
        m_state = std::make_unique<State>();
    std::shared_ptr<const PathologicalSegmentAnalysisResult> prescan_result;
    if (compact_streaming && m_status_callback)
        m_status_callback(82, _u8L("Pathological protection: preparing motion scan data"));
    if (m_prescan)
        prescan_result = m_prescan->wait_result(m_cancel);
    return apply(
        std::move(gcode), m_config, m_flavor, *m_state, m_cancel,
        compact_streaming, m_sim_config, prescan_result,
        m_status_callback);
}

std::string PathologicalSegmentAccelFilter::process_layer(std::string&& gcode)
{
    if (!m_config.enabled)      return std::move(gcode);
    if (m_flavor != gcfKlipper) return std::move(gcode);
    if (gcode.empty())          return std::move(gcode);
    return apply_with_prescan(std::move(gcode), false);
}

std::string PathologicalSegmentAccelFilter::process_gcode(std::string&& gcode, bool flush)
{
    if (!m_config.enabled || m_flavor != gcfKlipper) {
        if (flush && !m_state) return std::move(gcode);
        return std::move(gcode);
    }
    if (!m_state) m_state = std::make_unique<State>();
    if (!gcode.empty()) {
        if (m_state->buffered_gcode.empty())
            m_state->buffered_gcode = std::move(gcode);
        else
            m_state->buffered_gcode += gcode;
    }
    if (!flush)
        return {};

    std::string full_gcode = std::move(m_state->buffered_gcode);
    m_state->buffered_gcode.clear();
    m_state->sim_state = {};
    m_state->accel_known = false;
    m_state->aue_pending = false;
    m_state->aue_active = false;
    m_state->aue_v_low = 0.0;
    m_state->aue_v_target = 0.0;
    m_state->aue_accel = 0.0;
    m_state->aue_distance = 0.0;
    if (full_gcode.empty())
        return {};
    return apply_with_prescan(std::move(full_gcode), true);
}

bool PathologicalSegmentAccelFilter::process_file(const std::string& path)
{
    if (!m_config.enabled || m_flavor != gcfKlipper)
        return false;

    std::ifstream in(path, std::ios::binary);
    if (!in)
        return false;
    std::string gcode;
    if (m_status_callback)
        m_status_callback(80, _u8L("Pathological protection: reading input"));
    std::array<char, 64 * 1024> read_buffer{};
    while (in) {
        if (m_cancel)
            m_cancel();
        in.read(read_buffer.data(), read_buffer.size());
        const std::streamsize count = in.gcount();
        if (count > 0)
            gcode.append(read_buffer.data(), static_cast<size_t>(count));
    }
    if (!in.eof())
        return false;
    in.close();

    if (m_status_callback)
        m_status_callback(81, _u8L("Pathological protection: preparing motion scan"));
    std::string rewritten = process_gcode(std::move(gcode), true);
    const std::string output_path = path + ".pathological.tmp";
    {
        if (m_status_callback)
            m_status_callback(93, _u8L("Pathological protection: writing output"));
        std::ofstream out(output_path, std::ios::binary | std::ios::trunc);
        if (!out)
            return false;
        out.write(rewritten.data(), (std::streamsize)rewritten.size());
        out.flush();
        if (!out) {
            out.close();
            boost::nowide::remove(output_path.c_str());
            return false;
        }
    }
    const std::error_code rename_error = rename_file(output_path, path);
    if (rename_error) {
        boost::nowide::remove(output_path.c_str());
        return false;
    }
    if (m_cancel)
        m_cancel();
    if (m_status_callback)
        m_status_callback(94, _u8L("Pathological protection: complete"));
    return true;
}

// ---------------------------------------------------------------------------
std::string PathologicalSegmentAccelFilter::apply(
    std::string&& gcode, const Config& cfg, GCodeFlavor flavor, State& state,
    const std::function<void()>& cancel, bool compact_streaming,
    const KlipperSim::SimConfig& sim_config,
    const std::shared_ptr<const PathologicalSegmentAnalysisResult>& prescan_result,
    const std::function<void(int, const std::string&)>& status_callback)
{
    if (cancel)
        cancel();
    std::vector<std::string> lines = split_lines(gcode);
    const int line_count = (int)lines.size();

    if (compact_streaming && status_callback)
        status_callback(83, _u8L("Pathological protection: scanning motion trajectory"));

    // Analyze the whole exported file in one pass so the filter sees the same
    // continuous occupancy curve as the main simulator path.
    const double initial_accel = state.sim_state.current_accel;
    const double initial_requested_atd = state.sim_state.requested_accel_to_decel;
    const double initial_atd = state.sim_state.current_accel_to_decel;
    const bool initial_e_relative = state.sim_state.e_relative;
    const double initial_feed = state.sim_state.feed;
    const double initial_x = state.sim_state.x;
    const double initial_y = state.sim_state.y;
    const double initial_z = state.sim_state.z;
    const double initial_e = state.sim_state.e_abs;
    const bool initial_xyz_relative = state.sim_state.xyz_relative;
    KlipperSim::SimConfig scfg = sim_config;
    scfg.limits.max_accel = state.sim_state.current_accel;
    scfg.limits.max_accel_to_decel = state.sim_state.current_accel_to_decel;
    std::vector<double> positive_extrusion_cruise_by_line;
    std::vector<double> positive_extrusion_distance_by_line;
    std::vector<char> flags;
    bool use_prescan = false;
    std::string prescan_fallback_reason;
    if (prescan_result) {
        if (prescan_result->status == PathologicalAnalysisStatus::Cancelled)
            throw std::runtime_error("pathological protection analysis cancelled");
        if (prescan_result->status != PathologicalAnalysisStatus::Complete) {
            prescan_fallback_reason = "analysis_";
            switch (prescan_result->status) {
            case PathologicalAnalysisStatus::Unsupported:
                prescan_fallback_reason += "unsupported";
                break;
            case PathologicalAnalysisStatus::SimulationError:
                prescan_fallback_reason += "simulation_error";
                break;
            case PathologicalAnalysisStatus::Complete:
            case PathologicalAnalysisStatus::Cancelled:
                prescan_fallback_reason += "not_complete";
                break;
            }
            if (!prescan_result->error_message.empty())
                prescan_fallback_reason += ":" + prescan_result->error_message;
        } else if (prescan_result->schema
                   != PathologicalSegmentAnalysisResult::schema_version) {
            prescan_fallback_reason = "schema_mismatch";
        } else if (std::abs(prescan_result->threshold
                            - OCCUPANCY_THRESHOLD) > 1e-12) {
            prescan_fallback_reason = "threshold_mismatch";
        } else if (prescan_result->sim_config_hash
                   != PathologicalSegmentProbe::sim_config_hash(scfg)) {
            prescan_fallback_reason = "sim_config_mismatch";
        } else {
            use_prescan = map_prescan_analysis(
                *prescan_result, lines, flags,
                positive_extrusion_cruise_by_line,
                positive_extrusion_distance_by_line,
                prescan_fallback_reason, cancel);
        }
    }

    if (use_prescan) {
        state.sim_state = prescan_result->line_state;
    } else {
        flags = KlipperSim::analyze_gcode_lines(
            lines, scfg, OCCUPANCY_THRESHOLD, state.sim_state, nullptr, cancel,
            compact_streaming, &positive_extrusion_cruise_by_line,
            &positive_extrusion_distance_by_line);
    }
    if (compact_streaming && status_callback) {
        status_callback(84, _u8L("Pathological protection: processing motion scan results"));
        status_callback(85, _u8L("Pathological protection: locating risk regions"));
    }

    // --- Merge flagged lines into regions (with small gap tolerance) ---
    struct Region {
        int begin;
        int end;
        double selected_accel = 0.0;
        double selected_decel = 0.0;
        double approximate_exit_cruise = 0.0;
        double recovery_accel = 0.0;
    };
    std::vector<Region> regions;
    {
        int i = 0;
        while (i < line_count) {
            if (!flags[i]) { ++i; continue; }
            int begin = i;
            int last  = i;
            int j = i + 1;
            int gap_commands = 0;
            while (j < line_count) {
                if (flags[j]) {
                    last = j;
                    gap_commands = 0;
                } else if (is_executable_gcode(lines[j])
                           && ++gap_commands > MERGE_GAP_COMMANDS) {
                    break;
                }
                ++j;
            }
            regions.push_back({ begin, last });
            i = j;
        }
    }

    if (compact_streaming && status_callback) {
        status_callback(86, regions.empty()
            ? _u8L("Pathological protection: no risk regions found")
            : _u8L("Pathological protection: preparing protection parameters"));
    }

    // --- Emit gcode with SET_VELOCITY_LIMIT around each region ---
    // Track ACCEL seen in stream to restore correctly.
    auto parse_named = [](std::string_view sv, std::string_view key, double& out)->bool{
        for (size_t i=0;i+key.size()<=sv.size();++i){
            bool m=true;
            for(size_t j=0;j<key.size();++j){ char a=sv[i+j],b=key[j];
                if(a>='a'&&a<='z')a-=32; if(b>='a'&&b<='z')b-=32; if(a!=b){m=false;break;} }
            if(m){ size_t st=i+key.size(); if(st>=sv.size())return false;
                char* e=nullptr; double v=std::strtod(sv.data()+st,&e);
                if(e==sv.data()+st)return false; out=v; return true; } }
        return false;
    };

    auto parse_word = [](std::string_view sv, char key, double& out)->bool {
        if (key >= 'a' && key <= 'z') key -= 32;
        for (size_t i = 0; i < sv.size() && sv[i] != ';'; ++i) {
            char c = sv[i];
            if (c >= 'a' && c <= 'z') c -= 32;
            if (c != key)
                continue;
            if (i > 0 && sv[i - 1] != ' ' && sv[i - 1] != '\t')
                continue;
            const char* begin = sv.data() + i + 1;
            char* end = nullptr;
            const double value = std::strtod(begin, &end);
            if (end != begin) {
                out = value;
                return true;
            }
        }
        return false;
    };
    auto is_command = [](std::string_view sv, std::string_view command)->bool {
        sv = trim_left(sv);
        if (sv.size() < command.size())
            return false;
        for (size_t i = 0; i < command.size(); ++i) {
            char a = sv[i], b = command[i];
            if (a >= 'a' && a <= 'z') a -= 32;
            if (b >= 'a' && b <= 'z') b -= 32;
            if (a != b) return false;
        }
        return sv.size() == command.size() || sv[command.size()] == ' '
            || sv[command.size()] == '\t' || sv[command.size()] == ';'
            || sv[command.size()] == '\r' || sv[command.size()] == '\n';
    };

    auto is_simple_absolute_g1 = [&](std::string_view sv)->bool {
        if (!is_command(sv, "G1"))
            return false;
        size_t end = sv.find_first_of(";\r\n");
        if (end == std::string_view::npos)
            end = sv.size();
        size_t pos = 0;
        bool first = true;
        while (pos < end) {
            while (pos < end && (sv[pos] == ' ' || sv[pos] == '\t'))
                ++pos;
            if (pos >= end)
                break;
            const size_t begin = pos;
            while (pos < end && sv[pos] != ' ' && sv[pos] != '\t')
                ++pos;
            std::string_view token = sv.substr(begin, pos - begin);
            if (first) {
                first = false;
                continue;
            }
            char key = token.front();
            if (key >= 'a' && key <= 'z')
                key -= 32;
            if (key != 'X' && key != 'Y' && key != 'Z'
                && key != 'E' && key != 'F')
                return false;
            std::string value(token.substr(1));
            char* value_end = nullptr;
            std::strtod(value.c_str(), &value_end);
            if (value_end == value.c_str() || *value_end != '\0')
                return false;
        }
        return true;
    };

    auto emit_split_aue_g1 = [&](std::string_view original,
                                  double start_x, double start_y, double start_z,
                                  double start_e, bool e_relative,
                                  double x_word, double y_word, double z_word,
                                  double e_word, bool has_x, bool has_y,
                                  bool has_z, bool has_e, double move_distance,
                                  double distance_before, double v_low,
                                  double v_target, double recovery_accel,
                                  double original_feed, std::string& destination) {
        const double required_total = std::max(0.0,
            (v_target * v_target - v_low * v_low) / (2.0 * recovery_accel));
        const double ramp_distance = std::min(move_distance,
            std::max(0.0, required_total - distance_before));
        const int ramp_segments = std::max(1, static_cast<int>(
            std::ceil(ramp_distance / AUE_MAX_RAMP_SEGMENT_MM)));
        std::vector<double> endpoints;
        endpoints.reserve(static_cast<size_t>(ramp_segments) + 1);
        for (int segment = 1; segment <= ramp_segments; ++segment)
            endpoints.push_back(ramp_distance * segment / ramp_segments);
        if (move_distance - ramp_distance > 1e-9)
            endpoints.push_back(move_distance);

        const size_t comment_pos = original.find(';');
        const std::string_view comment = comment_pos == std::string_view::npos
            ? std::string_view{} : original.substr(comment_pos);
        double previous_fraction = 0.0;
        for (size_t segment = 0; segment < endpoints.size(); ++segment) {
            const double fraction = endpoints[segment] / move_distance;
            const double delta_fraction = fraction - previous_fraction;
            destination += "G1";
            if (has_x) destination += " X" + coordinate_to_string(start_x + (x_word - start_x) * fraction);
            if (has_y) destination += " Y" + coordinate_to_string(start_y + (y_word - start_y) * fraction);
            if (has_z) destination += " Z" + coordinate_to_string(start_z + (z_word - start_z) * fraction);
            if (has_e) destination += " E" + coordinate_to_string(
                e_relative ? e_word * delta_fraction : start_e + (e_word - start_e) * fraction);
            const double speed = endpoints[segment] > ramp_distance + 1e-9
                ? original_feed
                : std::min(v_target, std::sqrt(v_low * v_low
                    + 2.0 * recovery_accel * (distance_before + endpoints[segment])));
            destination += " F" + float_to_string(speed * 60.0);
            if (segment + 1 == endpoints.size() && !comment.empty()) {
                destination += ' ';
                destination.append(comment.data(), comment.size());
                if (destination.back() != '\n')
                    destination += '\n';
            } else {
                destination += "\n";
            }
            previous_fraction = fraction;
        }
    };


    struct RegionProbeContext {
        double entry_accel = 0.0;
        double entry_requested_atd = 0.0;
        bool   e_relative = false;
    };
    struct CandidateSpec {
        double accel = 0.0;
        double accel_to_decel = 0.0;
    };
    struct CandidateAttempt {
        bool                  executed = false;
        bool                  safe = false;

        KlipperSim::OccResult occupancy;
        CandidateSpec         spec;
    };
    struct RegionProbeWork {
        RegionProbeContext              context;
        std::array<CandidateSpec, 4>    candidates{};
        size_t                          candidate_count = 0;
        std::array<CandidateAttempt, 4> attempts{};
        bool                            resolved = false;
    };

    std::vector<RegionProbeWork> probe_work(regions.size());

    // Capture the original modal state at every region entry before running
    // any candidate simulation. Protected regions restore these values, so
    // one region's selected candidate cannot alter another probe context.
    {
        size_t region_index = 0;
        bool   scan_in_region = false;
        double scan_accel = initial_accel;
        double scan_requested_atd = initial_requested_atd;
        double saved_scan_accel = initial_accel;
        double saved_scan_requested_atd = initial_requested_atd;
        bool   scan_e_relative = initial_e_relative;

        for (int i = 0; i < line_count; ++i) {
            const std::string_view line(lines[i]);
            const std::string_view t = trim_left(line);
            if (is_command(t, "M82"))
                scan_e_relative = false;
            else if (is_command(t, "M83"))
                scan_e_relative = true;

            const bool start_region = !scan_in_region
                && region_index < regions.size()
                && i == regions[region_index].begin;
            if (start_region) {
                probe_work[region_index].context = {
                    scan_accel, scan_requested_atd, scan_e_relative};
                saved_scan_accel = scan_accel;
                saved_scan_requested_atd = scan_requested_atd;
                scan_in_region = true;
            }

            const bool is_set_vel = t.size() >= 18
                && (t.compare(0, 18, "SET_VELOCITY_LIMIT") == 0
                    || t.compare(0, 18, "set_velocity_limit") == 0);
            if (is_set_vel && !scan_in_region) {
                double parsed = scan_accel;
                if (parse_named(line, "ACCEL=", parsed) && parsed > 0.0)
                    scan_accel = parsed;
                parsed = scan_requested_atd;
                if (parse_named(line, "ACCEL_TO_DECEL=", parsed)
                    && parsed > 0.0)
                    scan_requested_atd = parsed;
            }

            if (scan_in_region && region_index < regions.size()
                && i == regions[region_index].end) {
                scan_accel = saved_scan_accel;
                scan_requested_atd = saved_scan_requested_atd;
                scan_in_region = false;
                ++region_index;
            }
        }
    }

    constexpr std::array<double, 3> accel_factors{{0.8, 0.5, 0.2}};
    constexpr double fallback_accel = 800.0;
    const KlipperSim::ToolheadLimits default_limits;
    for (RegionProbeWork& work : probe_work) {
        const double base_accel = work.context.entry_accel > 0.0
            ? work.context.entry_accel : default_limits.max_accel;
        const double base_atd = work.context.entry_requested_atd > 0.0
            ? work.context.entry_requested_atd
            : default_limits.max_accel_to_decel;
        const double minimum_candidate_accel =
            std::max(1.0, std::min(base_accel, fallback_accel));
        const std::array<double, 4> raw_candidates{{
            std::max(minimum_candidate_accel, base_accel * accel_factors[0]),
            std::max(minimum_candidate_accel, base_accel * accel_factors[1]),
            std::max(minimum_candidate_accel, base_accel * accel_factors[2]),
            minimum_candidate_accel
        }};
        double previous_candidate = -1.0;
        for (double candidate_accel : raw_candidates) {
            if (std::abs(candidate_accel - previous_candidate) <= 1e-9)
                continue;
            previous_candidate = candidate_accel;
            const double ratio = candidate_accel / base_accel;
            work.candidates[work.candidate_count++] = {
                candidate_accel, std::max(1.0, base_atd * ratio)};
        }
    }

    auto run_candidate = [&](const Region& region,
                             const RegionProbeContext& context,
                             const CandidateSpec& candidate) {
        CandidateAttempt attempt;
        attempt.executed = true;
        attempt.spec = candidate;
        KlipperSim::SimConfig probe_cfg = scfg;
        probe_cfg.limits.max_accel = candidate.accel;
        probe_cfg.limits.max_accel_to_decel = candidate.accel_to_decel;

        std::string candidate_limit_line = "SET_VELOCITY_LIMIT ACCEL="
            + float_to_string(candidate.accel) + " ACCEL_TO_DECEL="
            + float_to_string(candidate.accel_to_decel) + "\n";
        std::deque<std::string> rewritten_lines;
        std::vector<const std::string*> probe_lines;
        probe_lines.reserve(static_cast<size_t>(region.end - region.begin + 2));
        probe_lines.push_back(&candidate_limit_line);
        for (int li = region.begin; li <= region.end; ++li) {
            const std::string_view line = lines[li];
            const std::string_view t = trim_left(line);
            const bool is_set_vel = t.size() >= 18
                && (t.compare(0, 18, "SET_VELOCITY_LIMIT") == 0
                    || t.compare(0, 18, "set_velocity_limit") == 0);
            if (is_set_vel) {
                rewritten_lines.push_back(clamp_velocity_limit_line(
                    line, candidate.accel, candidate.accel_to_decel));
                probe_lines.push_back(&rewritten_lines.back());
            } else {
                probe_lines.push_back(&lines[li]);
            }
        }
        attempt.occupancy = KlipperSim::simulate_line_refs_occupancy(
            probe_lines, probe_cfg, context.e_relative);
        const KlipperSim::OccResult& occ = attempt.occupancy;
        attempt.safe = !occ.mcu_overflow && !occ.nozzle_overflow
            && occ.mcu_frac < OCCUPANCY_THRESHOLD
            && occ.nozzle_frac < OCCUPANCY_THRESHOLD;
        return attempt;
    };

    const unsigned hardware_threads = std::thread::hardware_concurrency();
    const int available_workers = hardware_threads > 1
        ? static_cast<int>(hardware_threads - 1) : 1;
    const int region_workers = regions.empty()
        ? 1 : static_cast<int>(std::min<size_t>(regions.size(), 8));
    const int worker_count = std::max(1,
        std::min(available_workers, region_workers));
    tbb::task_arena probe_arena(worker_count);

    // Candidate priority is preserved by executing one candidate rank per
    // wave. Only unresolved regions advance to the next lower acceleration.
    // Workers write distinct attempt slots; Region is committed afterward on
    // the caller thread in deterministic region order.
    for (size_t candidate_index = 0; candidate_index < 4; ++candidate_index) {
        if (cancel)
            cancel();
        if (compact_streaming && status_callback && !regions.empty()) {
            static const char* const candidate_messages[] = {
                L("Pathological protection: evaluating protection parameters (1/4)"),
                L("Pathological protection: evaluating protection parameters (2/4)"),
                L("Pathological protection: evaluating protection parameters (3/4)"),
                L("Pathological protection: evaluating protection parameters (4/4)")
            };
            status_callback(87 + static_cast<int>(candidate_index),
                            I18N::translate(candidate_messages[candidate_index]));
        }

        probe_arena.execute([&] {
            tbb::parallel_for(
                tbb::blocked_range<size_t>(0, regions.size(), 1),
                [&](const tbb::blocked_range<size_t>& range) {
                    for (size_t region_index = range.begin();
                         region_index != range.end(); ++region_index) {
                        RegionProbeWork& work = probe_work[region_index];
                        if (work.resolved
                            || candidate_index >= work.candidate_count)
                            continue;
                        work.attempts[candidate_index] = run_candidate(
                            regions[region_index], work.context,
                            work.candidates[candidate_index]);
                    }
                });
        });

        for (size_t region_index = 0; region_index < regions.size();
             ++region_index) {
            Region& region = regions[region_index];
            RegionProbeWork& work = probe_work[region_index];
            if (work.resolved || candidate_index >= work.candidate_count)
                continue;
            const CandidateAttempt& attempt = work.attempts[candidate_index];
            if (!attempt.executed)
                throw std::logic_error(
                    "pathological candidate worker did not produce a result");

            if (attempt.occupancy.last_positive_extrusion_cruise > 1e-9)
                region.approximate_exit_cruise =
                    attempt.occupancy.last_positive_extrusion_cruise;
            region.recovery_accel = attempt.spec.accel;
            region.selected_accel = attempt.spec.accel;
            region.selected_decel = attempt.spec.accel_to_decel;

            if (attempt.safe)
                work.resolved = true;
        }

        if (cancel)
            cancel();
    }

    if (compact_streaming && status_callback)
        status_callback(91, _u8L("Pathological protection: rewriting motion"));

    std::string out;
    out.reserve(gcode.size() + regions.size() * 128);

    size_t ri = 0;
    bool in_region = false;
    double saved_accel = initial_accel;
    double saved_requested_atd = initial_requested_atd;
    double runtime_accel = initial_accel;
    double runtime_requested_atd = initial_requested_atd;
    double runtime_atd = initial_atd;
    bool runtime_e_relative = initial_e_relative;
    double runtime_feed = initial_feed;
    bool   runtime_known = true;
    bool   aue_pending = state.aue_pending;
    bool   aue_active = state.aue_active;
    double aue_v_low = state.aue_v_low;
    double aue_v_target = state.aue_v_target;
    double aue_accel = state.aue_accel;
    double aue_distance = state.aue_distance;
    double runtime_x = initial_x;
    double runtime_y = initial_y;
    double runtime_z = initial_z;
    double runtime_e = initial_e;
    bool runtime_xyz_relative = initial_xyz_relative;
    auto effective_atd = [](double accel, double requested) {
        return std::min(accel, requested);
    };

    for (int i = 0; i < line_count; ++i) {
        std::string_view sv(lines[i]);
        bool is_set_vel = false;
        double parsed_accel = runtime_accel;
        double parsed_requested_atd   = runtime_requested_atd;

        std::string_view t = trim_left(sv);
        if (is_command(t, "G90"))
            runtime_xyz_relative = false;
        else if (is_command(t, "G91"))
            runtime_xyz_relative = true;
        if (is_command(t, "M82"))
            runtime_e_relative = false;
        else if (is_command(t, "M83"))
            runtime_e_relative = true;

        const bool is_linear = is_command(t, "G0") || is_command(t, "G1");
        const bool is_arc = is_command(t, "G2") || is_command(t, "G3");
        const bool is_motion = is_linear || is_arc;
        const double motion_start_x = runtime_x;
        const double motion_start_y = runtime_y;
        const double motion_start_z = runtime_z;
        const double motion_start_e = runtime_e;
        double x_word = runtime_x, y_word = runtime_y, z_word = runtime_z;
        double e_word = runtime_e;
        const bool has_x = is_motion && parse_word(sv, 'X', x_word);
        const bool has_y = is_motion && parse_word(sv, 'Y', y_word);
        const bool has_z = is_motion && parse_word(sv, 'Z', z_word);
        const bool has_e = is_motion && parse_word(sv, 'E', e_word);
        if (is_motion) {
            if (runtime_xyz_relative) {
                if (has_x) x_word += runtime_x;
                if (has_y) y_word += runtime_y;
                if (has_z) z_word += runtime_z;
            }
            if (runtime_e_relative && has_e)
                runtime_e += e_word;
            else if (has_e)
                runtime_e = e_word;
            runtime_x = x_word;
            runtime_y = y_word;
            runtime_z = z_word;
        } else if (is_command(t, "G92")) {
            parse_word(sv, 'X', runtime_x);
            parse_word(sv, 'Y', runtime_y);
            parse_word(sv, 'Z', runtime_z);
            parse_word(sv, 'E', runtime_e);
        }
        double feed_word = 0.0;
        const bool updates_feed = is_motion && parse_word(sv, 'F', feed_word)
            && feed_word > 0.0;
        const double line_feed = updates_feed ? feed_word / 60.0 : runtime_feed;
        if (t.size()>=18 &&
            (t.compare(0,18,"SET_VELOCITY_LIMIT")==0 ||
             t.compare(0,18,"set_velocity_limit")==0)) {
            is_set_vel = true;
            if (parse_named(sv, "ACCEL=", parsed_accel) && parsed_accel > 0)
                runtime_known = true;
            if (parse_named(sv, "ACCEL_TO_DECEL=", parsed_requested_atd) && parsed_requested_atd > 0) {
                // parsed_atd already updated
            }
        }

        bool start_region = (!in_region && ri < regions.size() && i == regions[ri].begin);
        // Region start: before the first line of a region, inject safe accel.
        if (start_region) {
            // A new pathological region supersedes any unfinished recovery.
            aue_pending = false;
            aue_active = false;
            const Region& selected = regions[ri];
            saved_accel = runtime_accel;
            saved_requested_atd = runtime_requested_atd;
            out += "SET_VELOCITY_LIMIT ACCEL=";
            out += float_to_string(selected.selected_accel);
            out += " ACCEL_TO_DECEL=";
            out += float_to_string(selected.selected_decel);
            out += " ; pathological_segment_safe\n";
            in_region = true;
            runtime_accel = selected.selected_accel;
            runtime_requested_atd = selected.selected_decel;
            runtime_atd = effective_atd(runtime_accel, runtime_requested_atd);
            runtime_known  = true;
        }

        // Emit the line, clamping any native SET_VELOCITY_LIMIT inside a
        // pathological region so it cannot override our safe accel values.
        // Post-region AUE feed caps remain local to one extrusion line and
        // restore the original modal feed immediately. Recovery distance
        // accumulates only across positive spatial extrusion.
        const size_t line_index = static_cast<size_t>(i);
        const double planned_cruise = line_index < positive_extrusion_cruise_by_line.size()
            ? positive_extrusion_cruise_by_line[line_index] : 0.0;
        const double extrusion_distance = line_index < positive_extrusion_distance_by_line.size()
            ? positive_extrusion_distance_by_line[line_index] : 0.0;
        const bool positive_spatial_extrusion = planned_cruise > 1e-9
            && extrusion_distance > 1e-9;
        double aue_speed_cap = 0.0;
        bool aue_profile_applies_to_line = false;
        const double aue_distance_before_line = aue_distance;
        if (!in_region && positive_spatial_extrusion) {
            if (aue_pending) {
                aue_pending = false;
                if (aue_v_low > 1e-9
                    && planned_cruise > aue_v_low + 1e-9) {
                    aue_active = true;
                    aue_v_target = planned_cruise;
                    aue_distance = 0.0;
                }
            }
            if (aue_active) {
                aue_profile_applies_to_line = true;
                aue_distance += extrusion_distance;
                const double envelope = std::sqrt(
                    aue_v_low * aue_v_low + 2.0 * aue_accel * aue_distance);
                aue_speed_cap = std::min({planned_cruise, aue_v_target, envelope});
                if (envelope >= aue_v_target - 1e-9)
                    aue_active = false;
            }
        }
        const bool cap_aue_speed = !in_region && aue_profile_applies_to_line
            && aue_speed_cap > 1e-9 && line_feed > 1e-9;
        if (in_region && is_set_vel) {
            out += clamp_velocity_limit_line(lines[i],
                                             regions[ri].selected_accel,
                                             regions[ri].selected_decel);
        } else if (cap_aue_speed) {
            const bool can_split = !runtime_xyz_relative && is_command(t, "G1")
                && is_simple_absolute_g1(sv) && extrusion_distance > AUE_MAX_RAMP_SEGMENT_MM
                && has_e;
            if (can_split)
                emit_split_aue_g1(lines[i], motion_start_x, motion_start_y, motion_start_z,
                                  motion_start_e, runtime_e_relative, x_word, y_word, z_word,
                                  e_word, has_x, has_y, has_z, has_e, extrusion_distance,
                                  aue_distance_before_line, aue_v_low, aue_v_target,
                                  aue_accel, line_feed, out);
            else
                out += set_motion_feedrate(lines[i], aue_speed_cap);
        } else {
            out += lines[i];
        }
        if (cap_aue_speed && line_feed > 1e-9) {
            out += "G1 F";
            out += float_to_string(line_feed * 60.0);
            out += " ; pathological_segment_aue_restore\n";
        }

        if (updates_feed)
            runtime_feed = feed_word / 60.0;
        // Update the running stream state only when the original line was not
        // forced through the safe clamp.  Clamped SET_VELOCITY_LIMIT commands
        // must not overwrite the pre-region restoration point.
        if (is_set_vel && !in_region) {
            if (parse_named(sv, "ACCEL=", runtime_accel) && runtime_accel > 0)
                runtime_known = true;
            if (parse_named(sv, "ACCEL_TO_DECEL=", runtime_requested_atd) && runtime_requested_atd > 0) {
                // no-op; runtime_atd already updated
            }
            runtime_atd = effective_atd(runtime_accel, runtime_requested_atd);
        }

        // Region end: after the last line, restore accel.
        if (in_region && ri < regions.size() && i == regions[ri].end) {
            out += "SET_VELOCITY_LIMIT ACCEL=";
            out += float_to_string(saved_accel);
            out += " ACCEL_TO_DECEL=";
            out += float_to_string(saved_requested_atd);
            out += " ; pathological_segment_recover\n";
            in_region = false;
            runtime_accel = saved_accel;
            runtime_requested_atd = saved_requested_atd;
            runtime_atd   = effective_atd(runtime_accel, runtime_requested_atd);
            runtime_known  = true;
            const Region& completed = regions[ri];
            aue_v_low = completed.approximate_exit_cruise;
            aue_accel = std::max(1.0, completed.recovery_accel);
            aue_v_target = 0.0;
            aue_distance = 0.0;
            aue_active = false;
            aue_pending = aue_v_low > 1e-9;
            ++ri;
        }
    }
    state.aue_pending = aue_pending;
    state.aue_active = aue_active;
    state.aue_v_low = aue_v_low;
    state.aue_v_target = aue_v_target;
    state.aue_accel = aue_accel;
    state.aue_distance = aue_distance;
    // Close a region left open at end of layer.
    if (in_region) {
        out += "SET_VELOCITY_LIMIT ACCEL=";
        out += float_to_string(saved_accel);
        out += " ACCEL_TO_DECEL=";
        out += float_to_string(saved_requested_atd);
        out += " ; pathological_segment_recover\n";
        runtime_accel = saved_accel;
        runtime_requested_atd = saved_requested_atd;
        runtime_atd   = effective_atd(runtime_accel, runtime_requested_atd);
        runtime_known  = true;
    }

    state.sim_state.current_accel = runtime_accel;
    state.sim_state.requested_accel_to_decel = runtime_requested_atd;
    state.sim_state.current_accel_to_decel = runtime_atd;
    state.accel_known = runtime_known;

    // Verify the rewritten whole-file stream with a fresh simulation session.
    // Candidate scanning owns acceleration selection; residual hits are
    // reported and never trigger another rewrite of already protected G-code.
    if (compact_streaming && !regions.empty()) {
        if (cancel)
            cancel();
        if (status_callback)
            status_callback(92, _u8L("Pathological protection: verifying protected motion"));
        std::vector<std::string> verification_lines = split_lines(out);
        KlipperSim::LineAnalysisState verification_state;
        KlipperSim::LineAnalysisDebug verification_debug;
        std::vector<char> residual = KlipperSim::analyze_gcode_lines(
            verification_lines, scfg, OCCUPANCY_THRESHOLD,
            verification_state, &verification_debug, cancel, true);
        size_t residual_count = 0;
        for (char hit : residual)
            residual_count += hit ? 1u : 0u;

        out += "; pathological_segment_verification mcu_peak=";
        out += float_to_string(verification_debug.mcu_peak_frac);
        out += " nozzle_peak=";
        out += float_to_string(verification_debug.nozzle_peak_frac);
        out += " unresolved=";
        out += residual_count == 0 ? "0\n" : "1\n";
    }
    return out;
}

} // namespace Slic3r
