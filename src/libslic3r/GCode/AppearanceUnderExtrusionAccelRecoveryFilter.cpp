#include "AppearanceUnderExtrusionAccelRecoveryFilter.hpp"
#include "InterestRegion.hpp"

#include "../GCode.hpp"
#include "../ExtrusionEntity.hpp"
#include "../GCodeReader.hpp"
#include "../GCodeWriter.hpp"
#include "../LocalesUtils.hpp"
#include "../Utils.hpp"
#include "../libslic3r.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <optional>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

namespace Slic3r {

namespace {

static inline std::string_view trim_left(std::string_view s)
{
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n'))
        s.remove_prefix(1);
    return s;
}

static inline std::string_view trim(std::string_view s)
{
    s = trim_left(s);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n'))
        s.remove_suffix(1);
    return s;
}

static inline bool starts_with(std::string_view s, std::string_view prefix)
{
    return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
}

static inline bool is_ascii_digit(char c)
{
    return c >= '0' && c <= '9';
}

static inline bool is_ascii_alpha(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static std::vector<std::string> split_lines_keep_newline(const std::string& gcode)
{
    std::vector<std::string> out;
    out.reserve(std::count(gcode.begin(), gcode.end(), '\n') + 1);
    std::size_t start = 0;
    while (start < gcode.size()) {
        const auto pos = gcode.find('\n', start);
        if (pos == std::string::npos) {
            out.emplace_back(gcode.substr(start));
            break;
        }
        out.emplace_back(gcode.substr(start, pos - start + 1)); // keep newline
        start = pos + 1;
    }
    if (gcode.empty())
        out.emplace_back();
    return out;
}

static bool try_parse_unsigned(std::string_view s, unsigned int& out)
{
    s = trim(s);
    if (s.empty())
        return false;
    unsigned long long v = 0;
    for (char c : s) {
        if (c < '0' || c > '9')
            return false;
        v = v * 10ull + static_cast<unsigned long long>(c - '0');
        if (v > std::numeric_limits<unsigned int>::max())
            return false;
    }
    out = static_cast<unsigned int>(v);
    return true;
}

static bool try_parse_double(std::string_view s, double& out)
{
    s = trim(s);
    if (s.empty())
        return false;
    std::string tmp(s);
    char* endptr = nullptr;
    const double v = std::strtod(tmp.c_str(), &endptr);
    if (endptr == tmp.c_str())
        return false;
    out = v;
    return std::isfinite(out);
}

static bool try_parse_named_value(std::string_view line, std::string_view key, double& out)
{
    const std::size_t pos = line.find(key);
    if (pos == std::string_view::npos)
        return false;
    std::size_t p = pos + key.size();
    // consume optional spaces
    while (p < line.size() && (line[p] == ' ' || line[p] == '\t'))
        ++p;
    std::size_t end = p;
    while (end < line.size() && line[end] != ' ' && line[end] != '\t' && line[end] != '\r' && line[end] != '\n' && line[end] != ';')
        ++end;
    return try_parse_double(line.substr(p, end - p), out);
}

static bool try_parse_extrusion_role_marker(std::string_view raw_line, ExtrusionRole& out_role)
{
    std::string_view s = trim_left(raw_line);
    if (s.empty() || s.front() != ';')
        return false;
    s.remove_prefix(1);
    s = trim_left(s);

    // Numeric marker emitted when pressure equalizer is enabled.
    if (starts_with(s, "_EXTRUSION_ROLE:")) {
        s.remove_prefix(std::string_view("_EXTRUSION_ROLE:").size());
        unsigned int v = 0;
        if (!try_parse_unsigned(s, v))
            return false;
        out_role = static_cast<ExtrusionRole>(v);
        return true;
    }

    // Reserved tags (compatible / BBL): TYPE:...,  FEATURE: ...
    // We accept any prefix ending with ':' and try to parse the remainder as a role string.
    // Examples:
    //  ;TYPE:Outer wall
    //  ; FEATURE: Outer wall
    const std::size_t colon = s.find(':');
    if (colon == std::string_view::npos)
        return false;
    const std::string_view key = trim(s.substr(0, colon));
    if (key != "TYPE" && key != "FEATURE")
        return false;
    std::string_view role_str = s.substr(colon + 1);
    role_str                  = trim(role_str);
    out_role                  = ExtrusionEntity::string_to_role(role_str);
    return out_role != erNone;
}

static std::string strip_feedrate_from_motion_cmd(const std::string& raw_line)
{
    // Keep newline as-is.
    std::string_view line(raw_line);
    std::string      newline;
    if (line.size() >= 2 && line.substr(line.size() - 2) == "\r\n") {
        newline = "\r\n";
        line.remove_suffix(2);
    } else if (!line.empty() && line.back() == '\n') {
        newline = "\n";
        line.remove_suffix(1);
    }

    const std::size_t comment_pos = line.find(';');
    std::string_view  main        = (comment_pos == std::string_view::npos) ? line : line.substr(0, comment_pos);
    std::string_view  comment     = (comment_pos == std::string_view::npos) ? std::string_view() : line.substr(comment_pos);

    // Parse a motion command prefix like "G1" even if it is immediately followed by parameters (e.g. "G1F6000").
    std::string_view sv = trim_left(main);
    if (sv.empty())
        return raw_line;
    if (sv.front() != 'G' && sv.front() != 'g')
        return raw_line;
    std::size_t p = 1;
    if (p >= sv.size() || !is_ascii_digit(sv[p]))
        return raw_line;
    unsigned int gcode = 0;
    while (p < sv.size() && is_ascii_digit(sv[p])) {
        gcode = gcode * 10u + static_cast<unsigned int>(sv[p] - '0');
        ++p;
    }
    if (!(gcode == 0u || gcode == 1u || gcode == 2u || gcode == 3u))
        return raw_line;

    std::string_view cmd  = sv.substr(0, p);
    std::string_view rest = sv.substr(p);

    std::vector<std::string> kept;
    kept.reserve(16);
    kept.emplace_back(cmd);

    bool        removed_f = false;
    std::size_t i         = 0;
    while (i < rest.size()) {
        while (i < rest.size() && (rest[i] == ' ' || rest[i] == '\t'))
            ++i;
        if (i >= rest.size())
            break;

        const char axis = rest[i];
        if (!is_ascii_alpha(axis))
            return raw_line;
        ++i;

        while (i < rest.size() && (rest[i] == ' ' || rest[i] == '\t'))
            ++i;

        const std::size_t value_start = i;
        while (i < rest.size()) {
            const char c = rest[i];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == ';')
                break;
            if (is_ascii_alpha(c))
                break;
            ++i;
        }

        std::string_view value = rest.substr(value_start, i - value_start);
        value                  = trim(value);
        if (value.empty())
            return raw_line;

        if (axis == 'F' || axis == 'f') {
            removed_f = true;
            continue;
        }

        std::string tok;
        tok.reserve(1 + value.size());
        tok.push_back(axis);
        tok.append(value.data(), value.size());
        kept.push_back(std::move(tok));
    }

    if (!removed_f)
        return raw_line;

    // If it was a pure feedrate command (e.g. "G1 F..." or "G1F..."), drop it (but keep comment-only line if any).
    if (kept.size() == 1) {
        if (comment.empty())
            return std::string();
        std::string out;
        out.reserve(comment.size() + newline.size());
        out.append(comment.data(), comment.size());
        out += newline;
        return out;
    }

    std::string out;
    out.reserve(raw_line.size());
    for (std::size_t k = 0; k < kept.size(); ++k) {
        if (k > 0)
            out.push_back(' ');
        out.append(kept[k]);
    }
    if (!comment.empty()) {
        if (!out.empty())
            out.push_back(' ');
        out.append(comment.data(), comment.size());
    }
    out += newline;
    return out;
}

static bool is_blank_or_comment_only_line(std::string_view raw_line)
{
    // Strip trailing newline(s)
    while (!raw_line.empty() && (raw_line.back() == '\n' || raw_line.back() == '\r'))
        raw_line.remove_suffix(1);

    // Trim whitespace
    auto is_ws = [](unsigned char c) { return std::isspace(c) != 0; };
    while (!raw_line.empty() && is_ws(static_cast<unsigned char>(raw_line.front())))
        raw_line.remove_prefix(1);
    while (!raw_line.empty() && is_ws(static_cast<unsigned char>(raw_line.back())))
        raw_line.remove_suffix(1);

    if (raw_line.empty())
        return true;
    // Comment-only line (";" or "(...)" style)
    return raw_line.front() == ';' || raw_line.front() == '(';
}

static bool is_pure_feedrate_motion_cmd(std::string_view raw_line)
{
    // Strip trailing newline(s)
    while (!raw_line.empty() && (raw_line.back() == '\n' || raw_line.back() == '\r'))
        raw_line.remove_suffix(1);

    // Trim leading whitespace
    auto is_ws = [](unsigned char c) { return std::isspace(c) != 0; };
    while (!raw_line.empty() && is_ws(static_cast<unsigned char>(raw_line.front())))
        raw_line.remove_prefix(1);

    if (raw_line.empty())
        return false;
    if (raw_line.front() == ';' || raw_line.front() == '(')
        return false;

    // Drop trailing comment part.
    const size_t semi = raw_line.find(';');
    std::string_view code = (semi == std::string_view::npos) ? raw_line : raw_line.substr(0, semi);
    while (!code.empty() && is_ws(static_cast<unsigned char>(code.back())))
        code.remove_suffix(1);
    if (code.empty())
        return false;

    // Optional line number: N123 ...
    if (!code.empty() && (code.front() == 'N' || code.front() == 'n')) {
        size_t i = 1;
        while (i < code.size() && std::isdigit(static_cast<unsigned char>(code[i])))
            ++i;
        while (i < code.size() && is_ws(static_cast<unsigned char>(code[i])))
            ++i;
        code = (i < code.size()) ? code.substr(i) : std::string_view();
        if (code.empty())
            return false;
    }

    if (code.empty() || (code.front() != 'G' && code.front() != 'g'))
        return false;
    size_t i = 1;
    if (i >= code.size() || !std::isdigit(static_cast<unsigned char>(code[i])))
        return false;
    int gcode = 0;
    while (i < code.size() && std::isdigit(static_cast<unsigned char>(code[i]))) {
        gcode = gcode * 10 + (code[i] - '0');
        ++i;
    }
    if (!(gcode == 0 || gcode == 1 || gcode == 2 || gcode == 3))
        return false;

    bool has_f = false;
    bool has_other = false;
    while (i < code.size()) {
        // Skip spaces and non-alpha separators
        while (i < code.size() && !std::isalpha(static_cast<unsigned char>(code[i])))
            ++i;
        if (i >= code.size())
            break;
        const char letter = static_cast<char>(std::toupper(static_cast<unsigned char>(code[i])));
        ++i;
        if (letter == 'F')
            has_f = true;
        else
            has_other = true;
        // Skip value (until next alpha)
        while (i < code.size() && !std::isalpha(static_cast<unsigned char>(code[i])))
            ++i;
    }

    return has_f && !has_other;
}

static size_t extend_safe_begin_line(const std::vector<std::string>& lines, const std::vector<bool>& zaa_lines, size_t defect_line)
{
    // Include any immediately preceding "pure feedrate" commands (often emitted by cooling) and any blank/comment-only
    // lines between them and the first L_safe motion line.
    size_t begin = defect_line;
    while (begin > 0) {
        if (zaa_lines[begin - 1])
            break;
        const std::string& prev = lines[begin - 1];
        if (is_blank_or_comment_only_line(prev) || is_pure_feedrate_motion_cmd(prev))
            --begin;
        else
            break;
    }
    return begin;
}

static bool try_parse_object_id_marker(std::string_view raw, int& out_object_id)
{
    std::string_view sv = trim_left(raw);
    constexpr std::string_view prefix = "; OBJECT_ID:";
    if (sv.size() < prefix.size() || sv.substr(0, prefix.size()) != prefix)
        return false;
    sv.remove_prefix(prefix.size());
    while (!sv.empty() && (sv.front() == ' ' || sv.front() == '\t'))
        sv.remove_prefix(1);
    int  v = 0;
    bool has_digit = false;
    while (!sv.empty()) {
        const unsigned char c = static_cast<unsigned char>(sv.front());
        if (c < '0' || c > '9')
            break;
        has_digit = true;
        v = v * 10 + int(c - '0');
        sv.remove_prefix(1);
    }
    if (!has_digit)
        return false;
    out_object_id = v;
    return true;
}


struct Motion
{
    enum class Type : unsigned char
    {
        G0,
        G1,
        G2,
        G3,
    };

    size_t       line_idx{ 0 };
    Type         type{ Type::G1 };
    bool         extruding{ false };
    ExtrusionRole role{ erNone };
    Vec3d        start{ Vec3d::Zero() };
    Vec3d        end{ Vec3d::Zero() };
    double       length_mm{ 0.0 }; // Path length in mm. For arcs (G2/G3 with I/J), this is the 3D arc length (includes Z delta).
    double       feedrate_mm_s{ 0.0 };
    double       feedrate_mm_min{ 0.0 };
    double       accel_mm_s2{ 0.0 };
    int          object_id{ -1 };
    bool         zaa_protected{ false };
    size_t       zaa_epoch{ 0 };

    // For extrusion split
    double e_start_abs{ 0.0 };
    double e_end_abs{ 0.0 };
    double delta_e{ 0.0 };

    // Arc specific (I/J only)
    bool   is_arc{ false };
    bool   arc_ccw{ true };
    Vec2d  arc_center{ Vec2d::Zero() };
    double arc_sweep_rad{ 0.0 }; // signed, matches direction
};

struct ObjectSpan
{
    size_t trigger_begin{ 0 };
    size_t trigger_end{ 0 };
    size_t defect_begin{ 0 };
    size_t defect_end{ 0 };
    int    object_id{ -1 };
};

struct Plan
{
    ObjectSpan span;

    // Where to start applying velocity_safe (end of L_safe_transition).
    bool   has_velocity_safe{ false };
    size_t velocity_safe_motion{ 0 };
    double velocity_safe_fraction{ 0.0 }; // within velocity_safe_motion, [0,1]

    // Where to recover accel/feedrate back to the "real" target.
    size_t boundary_motion{ 0 };
    double boundary_fraction{ 1.0 }; // within boundary_motion, (0,1]

    double accel_safe{ 0.0 };
    double accel_recovered{ 0.0 };
    double velocity_safe_mm_s{ 0.0 };
    double velocity_recovered_mm_s{ 0.0 };
    double L_safe_transition_mm{ 0.0 };
    double L_safe_accel_mm{ 0.0 };
    double L_safe_cruise_mm{ 0.0 };
    double L_safe_total_mm{ 0.0 };
};

static double clamp_non_negative(double v)
{
    return (std::isfinite(v) && v > 0.0) ? v : 0.0;
}

static double compute_length_mm(const Motion& m)
{
    if (m.is_arc) {
        // Arc case: Motion::length_mm already contains the 3D arc length (including Z delta).
        return m.length_mm;
    }
    return m.length_mm;
}

// 为一个检测到的 ROI 对象（Trigger + Defect）构建 AUE 缓冲计划（Plan）。
//
// span 语义：
// - span.* 是 motions[] 的下标，均为闭区间 [begin, end]（两端都包含）。
//
// 关键速度/加速度：
// - v_low  ：Trigger 末端 motion 的进给（trigger_last.feedrate_mm_s）。
// - v_safe ：用户设定的安全速度上限（params.velocity_safe_mm_s）。
// - a_safe ：用户设定的安全加速度上限（params.accel_safe）。
// - v_rec  ：Defect 末端“真实要恢复到的速度”（defect_last.feedrate_mm_s），即该段最后一次生效的设速。
//           注意：standalone 的 "G1 F..." 不会生成 Motion，但会更新解析 state.feedrate，
//           因此会影响后续 motion 的 feedrate，并最终反映到 defect_last.feedrate_mm_s。
// - accel_recovered：恢复点要恢复的加速度，优先 defect_first.accel_mm_s2，失败则回退 trigger_last.accel_mm_s2。
//
// safe 段长度模型：
// - L_transition：在 defect_begin 后先保持 v_low 的过渡距离（params.L_safe_transition_mm）。
//   目的：稍微延长拐角/降速后的低速段，避免太早触发加速/提速。
// - L_accel：在常加速度 a_safe 下，从 v_low 加速到 v_safe 的理论距离：
//     L_accel = (v_safe^2 - v_low^2) / (2*a_safe)（仅 v_safe > v_low 时）
//   该值只用于确定“理想需要多长距离”，并不会把 safe 段拆成独立的 accel/cruise 两相；
//   固件 planner 是否能在 safe 段内真正达到 v_safe，取决于 defect 段实际可用路径长度。
// - L_total = L_transition + L_accel + L_safe_cruise_mm：希望在 Defect 路径上维持 safe 限制的总长度。
//
// boundary（恢复点）的确定：
// - 沿 defect span 累积 motions 的几何长度，第一次达到 L_total 的位置即为恢复边界：
//   - boundary_motion   ：包含恢复边界的那条 Motion（motions[] 下标）
//   - boundary_fraction ：边界在该 Motion 内的位置比例 (0,1]，1.0 表示落在 motion 末端（无需 split）
// - 若 Defect 总长度 < L_total，则 boundary 会被夹到 defect_end（fraction=1.0），safe 段提前结束并在 defect 末端恢复。
//   这包含三类距离不足：
//   - Defect 总长度 < L_transition：连 transition 都跑不完，不会插入 v_safe（整段保持 v_low），并在 defect_end recover。
//   - L_transition <= Defect 总长度 < L_transition + L_accel：理论加速距离不够，planner 可能达不到 v_safe（不存在 cruise）
//   - L_transition + L_accel <= Defect 总长度 < L_total：能达到 v_safe，但 cruise 长度达不到设定值
//   当前实现不区分后两者，统一在 defect_end 处 recover。

static std::optional<Plan> build_plan(const ObjectSpan& span, const std::vector<Motion>& motions, const AppearanceUnderExtrusionAccelRecoveryConfig& params)
{
    if (span.trigger_end >= motions.size() || span.defect_begin >= motions.size() || span.defect_end >= motions.size())
        return std::nullopt;
    if (!(params.accel_safe > 0.0f) || params.velocity_safe_mm_s <= 0.0f)
        return std::nullopt;

    const Motion& trigger_last = motions[span.trigger_end];
    const Motion& defect_first = motions[span.defect_begin];
    const Motion& defect_last  = motions[span.defect_end];

    const double v_low  = clamp_non_negative(trigger_last.feedrate_mm_s);
    const double v_safe = clamp_non_negative(params.velocity_safe_mm_s);
    const double a_safe = clamp_non_negative(static_cast<double>(params.accel_safe));
    if (a_safe <= 0.0)
        return std::nullopt;

    // Recover to the "real" speed at the end of the defect segment (last set feedrate in effect).
    const double v_rec = clamp_non_negative(defect_last.feedrate_mm_s);
    if (v_rec <= 0.0)
        return std::nullopt;

    const double accel_recovered = clamp_non_negative(defect_first.accel_mm_s2) > 0.0 ? defect_first.accel_mm_s2 : trigger_last.accel_mm_s2;
    if (!(accel_recovered > 0.0))
        return std::nullopt;

    // Acceleration distance from v_low -> v_safe, under constant accel a_safe.
    double L_accel = 0.0;
    if (v_safe > v_low) {
        L_accel = (v_safe * v_safe - v_low * v_low) / (2.0 * a_safe);
        if (!std::isfinite(L_accel) || L_accel < 0.0)
            L_accel = 0.0;
    }

    const double L_transition = clamp_non_negative(params.L_safe_transition_mm);

    // Find where transition ends (where we start applying velocity_safe).
    bool   has_velocity_safe = false;
    size_t velocity_safe_motion = span.defect_begin;
    double velocity_safe_fraction = 0.0;
    if (L_transition <= 0.0) {
        has_velocity_safe = true;
        velocity_safe_motion = span.defect_begin;
        velocity_safe_fraction = 0.0;
    } else {
        double acc = 0.0;
        for (size_t i = span.defect_begin; i <= span.defect_end && i < motions.size(); ++i) {
            const double len = clamp_non_negative(compute_length_mm(motions[i]));
            if (len <= 0.0)
                continue;
            if (acc + len >= L_transition) {
                has_velocity_safe = true;
                velocity_safe_motion = i;
                const double remain = L_transition - acc;
                velocity_safe_fraction = std::clamp(remain / len, 0.0, 1.0);
                break;
            }
            acc += len;
        }
    }

    const double L_cruise = clamp_non_negative(params.L_safe_cruise_mm);
    const double L_total  = L_transition + L_accel + L_cruise;
    if (!(L_total > 0.0))
        return std::nullopt;

    // Find recover boundary within defect span.
    double acc = 0.0;
    size_t boundary = span.defect_begin;
    double fraction = 1.0;
    for (size_t i = span.defect_begin; i <= span.defect_end && i < motions.size(); ++i) {
        const double len = clamp_non_negative(compute_length_mm(motions[i]));
        if (len <= 0.0)
            continue;
        if (acc + len >= L_total) {
            boundary = i;
            const double remain = L_total - acc;
            fraction            = std::clamp(remain / len, 0.0, 1.0);
            break;
        }
        acc += len;
        boundary = i;
        fraction = 1.0;
    }

    Plan p;
    p.span                   = span;
    p.has_velocity_safe       = has_velocity_safe;
    p.velocity_safe_motion    = velocity_safe_motion;
    p.velocity_safe_fraction  = velocity_safe_fraction;
    p.boundary_motion         = boundary;
    p.boundary_fraction       = fraction;
    p.accel_safe              = a_safe;
    p.accel_recovered         = accel_recovered;
    p.velocity_safe_mm_s      = v_safe;
    p.velocity_recovered_mm_s = v_rec;
    p.L_safe_transition_mm     = L_transition;
    p.L_safe_accel_mm          = L_accel;
    p.L_safe_cruise_mm         = L_cruise;
    p.L_safe_total_mm          = L_total;
    return p;
}

static std::string make_set_velocity_limit_accel(double accel, const char* comment)
{
    std::ostringstream oss;
    oss << "SET_VELOCITY_LIMIT ACCEL=" << float_to_string_decimal_point(accel);
    if (comment && *comment)
        oss << " ; " << comment;
    oss << "\n";
    return oss.str();
}

static std::string make_set_feedrate_mm_s(double v_mm_s, const char* comment)
{
    const double f_mm_min = v_mm_s * 60.0;
    GCodeG1Formatter f;
    f.emit_f(f_mm_min);
    f.emit_comment(true, comment ? comment : "");
    return f.string();
}

static std::string format_linear_move(const Vec3d& end, double e_value, bool use_relative_e, const char* comment)
{
    GCodeG1Formatter g1;
    g1.emit_xyz(end);
    if (use_relative_e)
        g1.emit_e(e_value);
    else
        g1.emit_e(e_value);
    g1.emit_comment(true, comment ? comment : "");
    return g1.string();
}

static std::string format_arc_move(bool ccw, const Vec3d& end, const Vec2d& ij, double e_value, bool use_relative_e, const char* comment)
{
    GCodeG2G3Formatter g23(ccw);
    g23.emit_xyz(end);
    g23.emit_ij(ij);
    if (use_relative_e)
        g23.emit_e(e_value);
    else
        g23.emit_e(e_value);
    g23.emit_comment(true, comment ? comment : "");
    return g23.string();
}

} // namespace

struct AppearanceUnderExtrusionAccelRecoveryFilter::State
{
    Vec3d         pos{ Vec3d::Zero() };
    double        e_abs{ 0.0 };
    double        feedrate_mm_min{ 0.0 };
    double        accel_mm_s2{ 0.0 };
    ExtrusionRole role{ erNone };
    bool          use_relative_e{ true };
    int           object_id{ -1 };
    ZaaIntervalTracker zaa_interval;
};

AppearanceUnderExtrusionAccelRecoveryFilter::AppearanceUnderExtrusionAccelRecoveryFilter(const GCodeConfig& config,
                                                                                       const PrintObjectConfig& default_object_config,
                                                                                       std::unordered_map<int, ObjectParams> object_params_by_id,
                                                                                       GCodeFlavor flavor)
    : m_object_params_by_id(std::move(object_params_by_id))
    , m_config(config)
    , m_flavor(flavor)
    , m_state(std::make_unique<State>())
{
    m_params.enabled    = default_object_config.msao_recovery_enable.value;
    m_params.accel_safe = static_cast<float>(default_object_config.msao_safe_accel.value);
    m_params.velocity_safe_mm_s = static_cast<float>(default_object_config.msao_safe_velocity.value);
    m_state->use_relative_e = config.use_relative_e_distances.value;
}

AppearanceUnderExtrusionAccelRecoveryFilter::~AppearanceUnderExtrusionAccelRecoveryFilter() = default;

void AppearanceUnderExtrusionAccelRecoveryFilter::reset()
{
    if (!m_state)
        m_state = std::make_unique<State>();
    *m_state = State{};
    m_state->use_relative_e = m_config.use_relative_e_distances.value;
}

LayerResult AppearanceUnderExtrusionAccelRecoveryFilter::process_layer(LayerResult&& input)
{
    return process_layer(std::move(input), true);
}

LayerResult AppearanceUnderExtrusionAccelRecoveryFilter::process_layer(LayerResult&& input, bool layer_end)
{
    if (!m_state)
        m_state = std::make_unique<State>();

    // Preserve the entry state for the actual line parser, then advance the persistent
    // tracker exactly once so protocol validation also covers all early-return paths.
    const ZaaIntervalTracker zaa_interval_at_chunk_start = m_state->zaa_interval;
    m_state->zaa_interval.consume_gcode(input.gcode);
    if (layer_end)
        m_state->zaa_interval.finish_layer();

    if (input.nop_layer_result)
        return std::move(input);
    if (m_flavor != gcfKlipper)
        return std::move(input);
    if (!m_params.enabled) {
        bool any_enabled = false;
        for (const auto& kv : m_object_params_by_id) {
            if (kv.second.enabled) {
                any_enabled = true;
                break;
            }
        }
        if (!any_enabled)
            return std::move(input);
    }
    input.gcode = apply_to_gcode_layer(std::move(input.gcode), m_params, m_config, m_flavor, m_object_params_by_id, *m_state,
                                       zaa_interval_at_chunk_start);
    return std::move(input);
}

// AUE（Appearance Under-Extrusion）加速度/速度恢复缓冲（单层 G-code 重写）。
//
// 背景问题：
// - 典型场景是悬垂/过桥：悬垂段触发冷却/悬垂降速，连续低速打印一段时间；
// - 当恢复到正常（相对更高）速度时，常见外观缺料（under-extrusion appearance）。
//   这类缺料更像“速度/加速度瞬态不匹配导致的挤出跟不上”，而不是持续的流量上限不足。
//
// 本函数做的事情（核心思路）：
// - 先在 layer 内定位一段“Trigger（低速触发段）”以及其后的“Defect（疑似缺料段）”，统称 ROI；
// - 然后在 Trigger/Defect 周围插入一组 Klipper 命令，短距离内把加速度/速度限制在“安全值”，
//   再在 Defect 段的某个边界处恢复为“真实的目标速度/加速度”，从而缓解低速→高速切换瞬态。
//
// 1) 解析阶段：从原始 layer gcode 构建 Motion 列表（用于距离、速度、E 分割等）
// - 逐行解析 G0/G1/G2/G3，跟踪：
//   - XYZ/E 位置（用于计算 move 长度、以及在边界处 split move 时拆分 E）
//   - 当前生效的进给 feedrate（F，mm/min）与 accel（来自 SET_VELOCITY_LIMIT ACCEL=...）
//   - extrusion role（来自注释标记，如 ";_EXTERNAL_PERIMETER"），用于 ROI 的角色筛选
// - 注意：像 "G1 F..." 这种“纯设速指令”不记为 Motion，但会更新 state.feedrate_mm_min，
//   这样后续 defect_last.feedrate_mm_s 才能代表“末端真实速度（最后一次设置过的进给）”。
//
// 2) ROI 检测：直接在 motions[] 上复用 InterestRegion 的检测逻辑（不再二次跑 GCodeProcessor）
// - ROI 检测参数取自 InterestRegion::AppearanceUnderExtrusionDefinition 的默认值（与 GUI 预览缓存一致），避免两处默认值漂移。
// - InterestRegion::detect_appearance_under_extrusion_interest_region(motions, ...) 会产出
//   "AppearanceUnderExtrusion" 对象，包含两段 span：
//   - Trigger：持续低速挤出段（速度 <= max_trigger_speed_mm_s 且持续时间 >= min_trigger_time_s）
//   - Defect：Trigger 之后、满足 defect_roles（默认外墙 + 悬垂外墙）的挤出段
// - SegmentSpan 使用 end-ssid 索引（表示“从 end_ssid-1 到 end_ssid 的那条段”）。对于 motions[] 视图：
//   end_ssid = N 直接对应 motions[N-1]，因此 span → motion 索引的映射是 O(1) 的减一操作。
//
// 3) 计划（Plan）生成：把“安全缓冲”的边界落到 Defect 路径上的某个位置
// - v_low：Trigger 末端 motion 的速度（trigger_last.feedrate_mm_s）
// - v_safe/a_safe：用户参数（velocity_safe_mm_s / accel_safe）
// - v_recovered：Defect 末端 motion 的“真实速度”（defect_last.feedrate_mm_s，即最近一次设置过的进给）
// - L_safe_transition_mm：在 defect_begin 后先保持 v_low 的过渡距离（默认 1mm），用于稍微延长拐角/降速后的低速段。
// - 计算理论加速距离（从 v_low 加到 v_safe，常加速度 a_safe）：
//     L_safe_accel = (v_safe^2 - v_low^2) / (2*a_safe)  (当 v_safe > v_low)
//   然后定义总安全长度：
//     L_safe_total = L_safe_transition_mm + L_safe_accel + L_safe_cruise_mm
// - 在 Defect span 内累积 motions 的几何长度，找到首次达到 L_safe_total 的位置作为 boundary：
//   - 若 boundary 落在某条 motion 内部，则记录 boundary_fraction（后面会 split 该 motion）
//   - 若 Defect 的总可用长度 < L_safe_total，则 boundary 会被“夹到 defect_end”，意味着 safe 段提前结束
//     （此时固件 planner 可能根本加不到 v_safe，表现为 L_safe_accel 被距离不足“截断”）
//
// 4) G-code 重写：插入 safe/recover，并保证 L_safe 只受 accel_safe 与 velocity_safe 影响
// - 在 trigger_begin 行前插入：SET_VELOCITY_LIMIT ACCEL=accel_safe
// - 在 transition_end 处插入：G1 F(v_safe)   （只设置进给，不带 XYZ/E）
// - 对处于 L_safe 段内的所有行：
//   - 移除任何遗留/上游注入的 F token（包括 "G1 X.. F.."、"G1F.."、以及独立的 "G1 F.."）
//   - 目的：保证 L_safe 这一段只由我们插入的 accel_safe 与 v_safe 控制，避免 CoolingBuffer 等模块
//     在同一区间内插入额外设速指令导致“safe 段不纯粹”
//   - 额外处理：Cooling 有时会在 safe 段第一条运动指令之前插入独立的 "G1 F..."，
//     因此 safe 段起点会向前扩展，连同紧邻的纯设速/空行/注释行一起清理
// - 在 boundary 处插入恢复指令：
//     SET_VELOCITY_LIMIT ACCEL=accel_recovered
//     G1 F(v_recovered)
//   - 若 transition_end / boundary 在 motion 中间：split 该 G1/G2/G3（可能两次 split：transition_end + boundary），
//     并在 split 点插入 v_safe / recover；输出的分段 move 均不带 F。
//
// 注意事项：
// - 当前实现只对 Klipper 生效（依赖 SET_VELOCITY_LIMIT）。
// - 本模块是“文本重写 + 插入控制命令”，实际的加减速曲线由固件 planner 决定。
// - 之所以推荐把该 filter 放在 write_gcode 之后、fan_mover 之前，是为了避免后续模块再次注入 F/限速指令，
//   破坏 L_safe 段“只受 accel_safe/velocity_safe 控制”的约束。
std::string AppearanceUnderExtrusionAccelRecoveryFilter::apply_to_gcode_layer(std::string&& gcode,
                                                                              const AppearanceUnderExtrusionAccelRecoveryConfig& params,
                                                                              const GCodeConfig& config,
                                                                              GCodeFlavor flavor,
                                                                              const std::unordered_map<int, ObjectParams>& object_params_by_id,
                                                                              State& state,
                                                                              ZaaIntervalTracker zaa_interval)
{
    if (flavor != gcfKlipper)
        return std::move(gcode);
    if (gcode.empty())
        return std::move(gcode);
    if (!params.enabled) {
        bool any_enabled = false;
        for (const auto& kv : object_params_by_id) {
            if (kv.second.enabled) {
                any_enabled = true;
                break;
            }
        }
        if (!any_enabled)
            return std::move(gcode);
    }
    if (params.velocity_safe_mm_s <= 0.0f) {
        bool any_has_velocity = false;
        for (const auto& kv : object_params_by_id) {
            if (kv.second.enabled && kv.second.velocity_safe_mm_s > 0.0f) {
                any_has_velocity = true;
                break;
            }
        }
        if (!any_has_velocity)
            return std::move(gcode);
    }

    state.object_id = -1;


    // Parse lines and build motion list.
    std::vector<std::string> lines = split_lines_keep_newline(gcode);
    std::vector<int>         line_to_motion(lines.size(), -1);
    std::vector<bool>        zaa_lines(lines.size(), false);
    std::vector<Motion>      motions;
    motions.reserve(lines.size() / 2);

    size_t zaa_epoch = 0;
    GCodeReader parser;
    parser.apply_config(config);

    for (size_t li = 0; li < lines.size(); ++li) {
        const std::string& raw = lines[li];
        const ZaaIntervalMarker zaa_marker = zaa_interval.consume_line(raw);
        if (zaa_marker != ZaaIntervalMarker::None)
            ++zaa_epoch;
        const bool zaa_protected = zaa_marker != ZaaIntervalMarker::None || zaa_interval.inside();
        zaa_lines[li] = zaa_protected;

        // Update role markers from comment-only lines.
        int object_id_marker = -1;
        if (try_parse_object_id_marker(raw, object_id_marker)) {
            state.object_id = object_id_marker;
            continue;
        }

        ExtrusionRole role_marker = erNone;
        if (try_parse_extrusion_role_marker(raw, role_marker))
            state.role = role_marker;

        // Extract cmd quickly.
        std::string_view sv(raw);
        sv = trim_left(sv);
        if (sv.empty() || sv.front() == ';')
            continue;

        const std::size_t cmd_end = sv.find_first_of(" \t;\r\n");
        const std::string_view cmd = (cmd_end == std::string_view::npos) ? sv : sv.substr(0, cmd_end);

        // Acceleration tracking (Klipper).
        if (cmd == "SET_VELOCITY_LIMIT") {
            double accel = 0.0;
            if (try_parse_named_value(sv, "ACCEL=", accel) && accel > 0.0 && std::isfinite(accel))
                state.accel_mm_s2 = accel;
            continue;
        }

        // Parse G-code axes for motion / G92, update state positions for length estimation.
        parser.parse_line(raw, [&](GCodeReader&, const GCodeReader::GCodeLine& line) {
            const std::string_view lcmd = line.cmd();

            if (lcmd == "G92") {
                if (line.has(X))
                    state.pos.x() = line.value(X);
                if (line.has(Y))
                    state.pos.y() = line.value(Y);
                if (line.has(Z))
                    state.pos.z() = line.value(Z);
                if (line.has(E))
                    state.e_abs = state.use_relative_e ? 0.0 : static_cast<double>(line.value(E));
                return;
            }

            const bool is_g0 = (lcmd == "G0");
            const bool is_g1 = (lcmd == "G1");
            const bool is_g2 = (lcmd == "G2");
            const bool is_g3 = (lcmd == "G3");
            if (!is_g0 && !is_g1 && !is_g2 && !is_g3)
                return;

            Vec3d start = state.pos;
            Vec3d end   = state.pos;
            if (line.has(X))
                end.x() = line.value(X);
            if (line.has(Y))
                end.y() = line.value(Y);
            if (line.has(Z))
                end.z() = line.value(Z);

            double feedrate_mm_min = state.feedrate_mm_min;
            if (line.has(F))
                feedrate_mm_min = static_cast<double>(line.value(F));
            const double feedrate_mm_s = feedrate_mm_min / 60.0;

            double delta_e = 0.0;
            double e_start = state.e_abs;
            double e_end   = state.e_abs;
            if (line.has(E)) {
                const double e_val = static_cast<double>(line.value(E));
                if (state.use_relative_e) {
                    delta_e = e_val;
                    e_end   = e_start + delta_e;
                } else {
                    e_end   = e_val;
                    delta_e = e_end - e_start;
                }
            }

            const bool has_geom_motion = ((end - start).norm() > 1e-9);
            const bool has_e_motion    = (std::fabs(delta_e) > 0.0);
            if (!has_geom_motion && !has_e_motion) {
                // e.g. "G1 F..." only.
                if (line.has(F))
                    state.feedrate_mm_min = feedrate_mm_min;
                return;
            }

            Motion m;
            m.line_idx          = li;
            m.type              = is_g0 ? Motion::Type::G0 : is_g1 ? Motion::Type::G1 : is_g2 ? Motion::Type::G2 : Motion::Type::G3;
            m.role              = state.role;
            m.start             = start;
            m.end               = end;
            m.feedrate_mm_min   = feedrate_mm_min;
            m.feedrate_mm_s     = feedrate_mm_s;
            m.accel_mm_s2       = state.accel_mm_s2;
            m.object_id         = state.object_id;
            m.zaa_protected     = zaa_protected;
            m.zaa_epoch         = zaa_epoch;
            m.e_start_abs       = e_start;
            m.e_end_abs         = e_end;
            m.delta_e           = delta_e;
            m.extruding         = delta_e > 0.0 && !zaa_protected;
            if (zaa_protected)
                m.role = erNone;

            if (is_g2 || is_g3) {
                m.is_arc  = true;
                m.arc_ccw = is_g3;
                if (line.has(I) && line.has(J)) {
                    const Vec2d ij(static_cast<double>(line.value(I)), static_cast<double>(line.value(J)));
                    m.arc_center = Vec2d(start.x(), start.y()) + ij;
                    const Vec2d sxy(start.x(), start.y());
                    const Vec2d exy(end.x(), end.y());
                    const double r = (sxy - m.arc_center).norm();
                    if (r > 0.0) {
                        const double a0 = std::atan2(sxy.y() - m.arc_center.y(), sxy.x() - m.arc_center.x());
                        const double a1 = std::atan2(exy.y() - m.arc_center.y(), exy.x() - m.arc_center.x());
                        double sweep     = a1 - a0;
                        if (m.arc_ccw) {
                            if (sweep < 0.0)
                                sweep += 2.0 * PI;
                        } else {
                            if (sweep > 0.0)
                                sweep -= 2.0 * PI;
                        }
                        m.arc_sweep_rad = sweep;
                        const double arc_len_xy = std::abs(sweep) * r;
                        const double dz         = end.z() - start.z();
                        m.length_mm             = std::sqrt(arc_len_xy * arc_len_xy + dz * dz);
                    } else {
                        m.length_mm = (end - start).norm();
                    }
                } else {
                    // Fallback: treat as straight chord.
                    m.length_mm = (end - start).norm();
                }
            } else {
                m.length_mm = (end - start).norm();
            }

            line_to_motion[li] = static_cast<int>(motions.size());
            motions.push_back(m);

            // Update state after move.
            if (line.has(F))
                state.feedrate_mm_min = feedrate_mm_min;
            state.pos = end;
            if (line.has(E))
                state.e_abs = e_end;
        });
    }

    if (motions.empty())
        return std::move(gcode);

    // Detect objects and build plans, driven by ROI (InterestRegion::InterestObject).
    std::vector<ObjectSpan> spans;
    {
        // Use InterestRegion defaults (same as GUI preview ROI cache).
        // Defect span cap is derived dynamically from the effective safe params (L_safe_total + margin),
        // so that GUI preview and post-processing stay consistent when msao_safe_accel / msao_safe_velocity changes.
        const InterestRegion::AppearanceUnderExtrusionDefinition def;

        auto defect_cap_base_mm = [&](float v_low_mm_s, int object_id) -> double {
            const auto  it         = object_params_by_id.find(object_id);
            const float accel_safe = (it != object_params_by_id.end()) ? it->second.accel_safe : params.accel_safe;
            const float v_safe     = (it != object_params_by_id.end()) ? it->second.velocity_safe_mm_s : params.velocity_safe_mm_s;
            if (!(accel_safe > 0.0f) || !(v_safe > 0.0f))
                return 0.0;
            return InterestRegion::compute_aue_L_safe_total_mm(v_low_mm_s, v_safe, accel_safe, params.L_safe_transition_mm, params.L_safe_cruise_mm);
        };

        const InterestRegion::InterestRegion region =
            InterestRegion::detect_appearance_under_extrusion_interest_region(motions, def, defect_cap_base_mm);

        auto end_ssid_to_motion_idx = [&](size_t end_ssid) -> std::optional<size_t> {
            if (end_ssid == 0)
                return std::nullopt;
            const size_t idx = end_ssid - 1;
            if (idx >= motions.size())
                return std::nullopt;
            return idx;
        };

        spans.reserve(region.objects.size());
        for (const std::unique_ptr<InterestRegion::InterestObject>& obj : region.objects) {
            if (!obj)
                continue;
            if (std::string_view(obj->type_name()) != "AppearanceUnderExtrusion")
                continue;

            std::optional<InterestRegion::SegmentSpan> trigger_span;
            std::optional<InterestRegion::SegmentSpan> defect_span;
            for (const InterestRegion::TaggedSpan& ts : obj->spans()) {
                if (ts.tag == InterestRegion::SegmentTag::Trigger)
                    trigger_span = ts.span;
                else if (ts.tag == InterestRegion::SegmentTag::Defect)
                    defect_span = ts.span;
            }
            if (!trigger_span || !defect_span)
                continue;

            // SegmentSpan is expressed in end-ssid indices (inclusive). For the MotionSegments view,
            // end_ssid N maps directly to motions[N-1].
            const std::optional<size_t> trigger_begin = end_ssid_to_motion_idx(trigger_span->first_end_ssid);
            const std::optional<size_t> trigger_end   = end_ssid_to_motion_idx(trigger_span->last_end_ssid);
            const std::optional<size_t> defect_begin  = end_ssid_to_motion_idx(defect_span->first_end_ssid);
            const std::optional<size_t> defect_end    = end_ssid_to_motion_idx(defect_span->last_end_ssid);
            if (!trigger_begin || !trigger_end || !defect_begin || !defect_end)
                continue;

            if (*trigger_begin > *trigger_end)
                continue;
            if (*defect_begin > *defect_end)
                continue;

            const size_t span_epoch = motions[*trigger_begin].zaa_epoch;
            bool intersects_zaa = false;
            for (size_t mi = *trigger_begin; mi <= *defect_end; ++mi) {
                if (motions[mi].zaa_protected || motions[mi].zaa_epoch != span_epoch) {
                    intersects_zaa = true;
                    break;
                }
            }
            if (intersects_zaa)
                continue;

            int span_object_id = motions[*defect_begin].object_id;
            if (span_object_id < 0)
                span_object_id = motions[*trigger_begin].object_id;
            spans.push_back({ *trigger_begin, *trigger_end, *defect_begin, *defect_end, span_object_id });
        }
    }
    if (spans.empty())
        return std::move(gcode);

    std::vector<Plan> plans;
    plans.reserve(spans.size());
    for (const ObjectSpan& s : spans) {
        const auto it = object_params_by_id.find(s.object_id);
        const bool  enabled    = (it != object_params_by_id.end()) ? it->second.enabled : params.enabled;
        const float accel_safe = (it != object_params_by_id.end()) ? it->second.accel_safe : params.accel_safe;
        const float velocity_safe_mm_s = (it != object_params_by_id.end()) ? it->second.velocity_safe_mm_s : params.velocity_safe_mm_s;
        if (!enabled)
            continue;

        AppearanceUnderExtrusionAccelRecoveryConfig effective = params;
        effective.accel_safe = accel_safe;
        effective.velocity_safe_mm_s = velocity_safe_mm_s;
        auto p = build_plan(s, motions, effective);
        if (p)
            plans.push_back(*p);
    }
    if (plans.empty())
        return std::move(gcode);

    // Drop overlapping plans to avoid nested state.
    // 这一段好像可以优化，因为我已经调整过不会overlap了，TODO
    std::sort(plans.begin(), plans.end(), [&](const Plan& a, const Plan& b) { return motions[a.span.trigger_begin].line_idx < motions[b.span.trigger_begin].line_idx; });
    std::vector<Plan> non_overlapping;
    non_overlapping.reserve(plans.size());
    size_t last_end_line = 0;
    bool   has_last_end  = false;
    for (const Plan& p : plans) {
        const size_t begin_line = motions[p.span.trigger_begin].line_idx;
        const size_t end_line   = motions[p.boundary_motion].line_idx;
        if (has_last_end && begin_line <= last_end_line)
            continue;
        non_overlapping.push_back(p);
        last_end_line  = end_line;
        has_last_end   = true;
    }
    plans.swap(non_overlapping);
    if (plans.empty())
        return std::move(gcode);

    // Build quick lookup by line index for insertions / splits.
    struct LineAction
    {
        enum class Kind : unsigned char
        {
            InsertAccelSafe,
            InsertSpeedSafe,
            InsertRecover,
            InsertDefectSpanEnd,
            SplitSpeedSafe,
            SplitRecover,
        };
        Kind   kind;
        size_t plan_idx;
    };

    // Lookup tables by original layer G-code line index (li):
    //  - before[li] / after[li]: actions inserted immediately before/after line li.
    //  - split[li]: actions that require splitting the motion line li at a plan-defined fraction.
    //              (For one plan, we may split twice on the same line: transition-end and recover boundary.)
    //  - safe_plan_of_line[li]: marks L_safe lines where we must strip any legacy feedrate (F) tokens,
    //                           including standalone commands like "G1 F...".
    std::vector<std::vector<LineAction>> before(lines.size());
    std::vector<std::vector<LineAction>> split(lines.size());
    std::vector<std::vector<LineAction>> after(lines.size());
    std::vector<int>                     safe_plan_of_line(lines.size(), -1);

    for (size_t pi = 0; pi < plans.size(); ++pi) {
        const Plan& p = plans[pi];

        const size_t trigger_line    = motions[p.span.trigger_begin].line_idx;
        const size_t defect_line     = motions[p.span.defect_begin].line_idx;
        const size_t boundary_line   = motions[p.boundary_motion].line_idx;
        const size_t defect_end_line = motions[p.span.defect_end].line_idx;

        if (trigger_line < lines.size())
            before[trigger_line].push_back({ LineAction::Kind::InsertAccelSafe, pi });

        // velocity_safe is applied only after transition ends (or immediately if transition length is 0).
        if (p.has_velocity_safe) {
            const size_t velocity_safe_line = motions[p.velocity_safe_motion].line_idx;
            constexpr double begin_eps = 1e-6;
            constexpr double end_eps   = 1e-6;
            if (p.velocity_safe_fraction <= begin_eps) {
                if (velocity_safe_line < lines.size())
                    before[velocity_safe_line].push_back({ LineAction::Kind::InsertSpeedSafe, pi });
            } else if (p.velocity_safe_fraction < 1.0 - end_eps) {
                if (velocity_safe_line < lines.size())
                    split[velocity_safe_line].push_back({ LineAction::Kind::SplitSpeedSafe, pi });
            } else {
                if (velocity_safe_line < lines.size())
                    after[velocity_safe_line].push_back({ LineAction::Kind::InsertSpeedSafe, pi });
            }
        }

        // Mark lines within the safe segment [defect_line, boundary_line] for feedrate cleanup.
        // Note: cooling may emit standalone "G1 F..." right before the first safe-segment motion line, so extend
        // the begin line backwards to include those pure-feedrate commands as well.
        if (defect_line < lines.size()) {
            const size_t begin = extend_safe_begin_line(lines, zaa_lines, defect_line);
            const size_t last  = std::min(boundary_line, lines.size() - 1);
            for (size_t l = begin; l <= last; ++l) {
                if (!zaa_lines[l])
                    safe_plan_of_line[l] = static_cast<int>(pi);
            }
        }

        constexpr double end_eps = 1e-6;
        if (p.boundary_fraction < 1.0 - end_eps) {
            if (boundary_line < lines.size())
                split[boundary_line].push_back({ LineAction::Kind::SplitRecover, pi });
        } else {
            if (boundary_line < lines.size())
                after[boundary_line].push_back({ LineAction::Kind::InsertRecover, pi });
        }

        if (defect_end_line < lines.size())
            after[defect_end_line].push_back({ LineAction::Kind::InsertDefectSpanEnd, pi });
    }

    const bool use_relative_e = config.use_relative_e_distances.value;
    const bool emit_aue_cmt = params.emit_aue_comments;
    auto aue_comment = [emit_aue_cmt](const char* s) -> const char* { return emit_aue_cmt ? s : ""; };

    std::string out;
    out.reserve(gcode.size() + plans.size() * 256);

    for (size_t li = 0; li < lines.size(); ++li) {
        // Before-line insertions.
        for (const LineAction& a : before[li]) {
            const Plan& p = plans[a.plan_idx];
            if (a.kind == LineAction::Kind::InsertAccelSafe) {
                out += make_set_velocity_limit_accel(p.accel_safe, aue_comment("AUE accel_safe"));
            } else if (a.kind == LineAction::Kind::InsertSpeedSafe) {
                out += make_set_feedrate_mm_s(p.velocity_safe_mm_s, aue_comment("AUE velocity_safe"));
            }
        }

        const int mi = line_to_motion[li];
        if (!split[li].empty() && mi >= 0) {
            const Motion& m = motions[static_cast<size_t>(mi)];

            struct SplitEvent
            {
                LineAction::Kind kind;
                size_t           plan_idx;
                double           fraction;
            };

            std::vector<SplitEvent> events;
            events.reserve(split[li].size());
            for (const LineAction& a : split[li]) {
                const Plan& p = plans[a.plan_idx];
                double      f = 1.0;
                if (a.kind == LineAction::Kind::SplitSpeedSafe)
                    f = p.velocity_safe_fraction;
                else if (a.kind == LineAction::Kind::SplitRecover)
                    f = p.boundary_fraction;
                else
                    continue;
                f = std::clamp(f, 0.0, 1.0);
                events.push_back({ a.kind, a.plan_idx, f });
            }

            if (!events.empty()) {
                const auto kind_priority = [](LineAction::Kind k) -> int {
                    if (k == LineAction::Kind::SplitSpeedSafe)
                        return 0;
                    if (k == LineAction::Kind::SplitRecover)
                        return 1;
                    return 2;
                };
                std::sort(events.begin(), events.end(), [&](const SplitEvent& a, const SplitEvent& b) {
                    if (a.fraction != b.fraction)
                        return a.fraction < b.fraction;
                    return kind_priority(a.kind) < kind_priority(b.kind);
                });

                // Split this motion at one or two fractions (transition end / recover boundary).
                const Vec3d start = m.start;
                const Vec3d end   = m.end;

                auto point_at = [&](double f) -> Vec3d {
                    f = std::clamp(f, 0.0, 1.0);
                    Vec3d pt = end;
                    if (f > 0.0 && f < 1.0) {
                        if (!m.is_arc) {
                            pt = start + (end - start) * f;
                        } else if (m.is_arc && std::fabs(m.arc_sweep_rad) > 0.0) {
                            const Vec2d c = m.arc_center;
                            const Vec2d sxy(start.x(), start.y());
                            const double r = (sxy - c).norm();
                            const double a0 = std::atan2(sxy.y() - c.y(), sxy.x() - c.x());
                            const double sweep = m.arc_sweep_rad * f;
                            const double a = a0 + sweep;
                            const Vec2d pxy(c.x() + r * std::cos(a), c.y() + r * std::sin(a));
                            pt.x() = pxy.x();
                            pt.y() = pxy.y();
                            pt.z() = start.z() + (end.z() - start.z()) * f;
                        } else {
                            pt = start + (end - start) * f;
                        }
                    }
                    return pt;
                };

                const double e_total = m.e_end_abs - m.e_start_abs;
                auto e_abs_at = [&](double f) -> double {
                    f = std::clamp(f, 0.0, 1.0);
                    return m.e_start_abs + e_total * f;
                };

                // Determine the active stage at the beginning of this motion.
                // Overlapping plans are dropped earlier, so split events are expected to belong to a single plan.
                const Plan&   p0 = plans[events.front().plan_idx];
                const size_t  motion_idx = static_cast<size_t>(mi);
                constexpr double begin_eps = 1e-6;

                bool speed_safe_active = false;
                if (p0.has_velocity_safe) {
                    if (motion_idx > p0.velocity_safe_motion)
                        speed_safe_active = true;
                    else if (motion_idx == p0.velocity_safe_motion && p0.velocity_safe_fraction <= begin_eps)
                        speed_safe_active = true;
                }
                bool recovered_active = false;

                auto segment_comment = [&](bool arc) -> const char* {
                    if (recovered_active)
                        return nullptr;
                    if (speed_safe_active)
                        return aue_comment(arc ? "AUE safe(arc)" : "AUE safe(part)");
                    return aue_comment(arc ? "AUE transition(arc)" : "AUE transition(part)");
                };

                double prev_f = 0.0;
                Vec3d  prev_pt = start;
                for (const SplitEvent& ev : events) {
                    const double f = std::clamp(ev.fraction, 0.0, 1.0);
                    if (f < prev_f)
                        continue;

                    if (f > prev_f) {
                        const Vec3d pt = point_at(f);
                        const double e_val = use_relative_e ? (m.delta_e * (f - prev_f)) : e_abs_at(f);

                        // Output segment up to the split point without any F token.
                        if (!m.is_arc) {
                            out += format_linear_move(pt, e_val, use_relative_e, segment_comment(false));
                        } else {
                            const Vec2d c = m.arc_center;
                            const Vec2d ij(c.x() - prev_pt.x(), c.y() - prev_pt.y());
                            out += format_arc_move(m.arc_ccw, pt, ij, e_val, use_relative_e, segment_comment(true));
                        }

                        prev_f  = f;
                        prev_pt = pt;
                    }

                    // Insert commands at split point.
                    const Plan& p = plans[ev.plan_idx];
                    if (ev.kind == LineAction::Kind::SplitSpeedSafe) {
                        out += make_set_feedrate_mm_s(p.velocity_safe_mm_s, aue_comment("AUE velocity_safe"));
                        speed_safe_active = true;
                    } else if (ev.kind == LineAction::Kind::SplitRecover) {
                        out += make_set_velocity_limit_accel(p.accel_recovered, aue_comment("AUE accel_recovered"));
                        out += make_set_feedrate_mm_s(p.velocity_recovered_mm_s, aue_comment("AUE velocity_recovered"));
                        recovered_active = true;
                    }
                }

                // Output the remaining segment to the original end point.
                const double e_last = use_relative_e ? (m.delta_e * (1.0 - prev_f)) : m.e_end_abs;
                if (!m.is_arc) {
                    out += format_linear_move(end, e_last, use_relative_e, segment_comment(false));
                } else {
                    const Vec2d c = m.arc_center;
                    const Vec2d ij(c.x() - prev_pt.x(), c.y() - prev_pt.y());
                    out += format_arc_move(m.arc_ccw, end, ij, e_last, use_relative_e, segment_comment(true));
                }

                // After-line insertions for this line (recover-at-end, defect span end marker, etc.).
                for (const LineAction& a : after[li]) {
                    const Plan& ap = plans[a.plan_idx];
                    if (a.kind == LineAction::Kind::InsertSpeedSafe) {
                        out += make_set_feedrate_mm_s(ap.velocity_safe_mm_s, aue_comment("AUE velocity_safe"));
                    } else if (a.kind == LineAction::Kind::InsertRecover) {
                        out += make_set_velocity_limit_accel(ap.accel_recovered, aue_comment("AUE accel_recovered"));
                        out += make_set_feedrate_mm_s(ap.velocity_recovered_mm_s, aue_comment("AUE velocity_recovered"));
                    } else if (a.kind == LineAction::Kind::InsertDefectSpanEnd) {
                        if (emit_aue_cmt)
                            out += "; defect span end\n";
                    }
                }

                continue; // skip original line
            }
        }

        // If this line is within the safe segment (L_safe), remove any legacy feedrate (F) tokens,
        // including standalone commands like "G1 F...".
        if (safe_plan_of_line[li] >= 0)
            out += strip_feedrate_from_motion_cmd(lines[li]);
        else
            out += lines[li];

        // After-line insertions (recover-at-end).
        for (const LineAction& a : after[li]) {
            const Plan& p = plans[a.plan_idx];
            if (a.kind == LineAction::Kind::InsertSpeedSafe) {
                out += make_set_feedrate_mm_s(p.velocity_safe_mm_s, aue_comment("AUE velocity_safe"));
            } else if (a.kind == LineAction::Kind::InsertRecover) {
                out += make_set_velocity_limit_accel(p.accel_recovered, aue_comment("AUE accel_recovered"));
                out += make_set_feedrate_mm_s(p.velocity_recovered_mm_s, aue_comment("AUE velocity_recovered"));
            } else if (a.kind == LineAction::Kind::InsertDefectSpanEnd) {
                if (emit_aue_cmt)
                    out += "; defect span end\n";
            }
        }
    }

    return out;
}

} // namespace Slic3r
