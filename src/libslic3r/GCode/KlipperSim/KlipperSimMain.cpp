#include "KlipperSimMain.hpp"
#include "KlipperTrapQ.hpp"
#include "KlipperItersolve.hpp"
#include "KlipperStepCompress.hpp"
#include "KlipperSteppersync.hpp"
#include "KlipperMCU.hpp"
#include "KlipperExtruderPA.hpp"
#include "KlipperStepper.hpp"
#include "KlipperHostDispatch.hpp"
#include "KlipperDispatchAnalysis.hpp"
#include "KlipperGCodeStream.hpp"
#include "KlipperOnlineStepGeneration.hpp"
#include "KlipperStreamingEarlyProbe.hpp"
#include "KlipperSimulationSession.hpp"

#include <fstream>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <stdexcept>
#include <queue>

namespace Slic3r { namespace KlipperSim {
// ---- gcode parsing ----
static bool parse_word(const std::string& l, char key, double& out)
{
    for (size_t i = 0; i < l.size(); ++i) {
        char c = l[i];
        if ((c == key || c == key + 32 || c == key - 32) && (i == 0 || l[i - 1] == ' ' || l[i - 1] == '\t')) {
            char*  e = nullptr;
            double v = std::strtod(l.c_str() + i + 1, &e);
            if (e != l.c_str() + i + 1) {
                out = v;
                return true;
            }
        }
    }
    return false;
}

static double calc_junction_deviation(double square_corner_velocity, double max_accel)
{
    return max_accel > 0.0 ? square_corner_velocity * square_corner_velocity * (std::sqrt(2.0) - 1.0) / max_accel : 0.0;
}

template <class LineGetter>
static ParsedGCode parse_gcode_lines_detailed_impl(size_t                line_count,
                                                   LineGetter&&          get_line,
                                                   bool                  initial_e_relative,
                                                   const ToolheadLimits& default_limits,
                                                   double                initial_pressure_advance,
                                                   double                initial_pa_smooth_time,
                                                   bool                  initial_pa_commands_enabled,
                                                   int                   line_idx_offset)
{
    ParsedGCode parsed;
    double      x = 0.0, y = 0.0, z = 0.0;
    double      F                        = 100.0;
    double      e_abs                    = 0.0;
    bool        e_relative               = initial_e_relative;
    bool        xyz_relative             = false;
    ArcPlane    arc_plane                = ArcPlane::XY;
    bool        have_pos                 = false;
    double      cur_accel                = default_limits.max_accel;
    double      requested_accel_to_decel = default_limits.max_accel_to_decel;
    double      cur_scv                  = default_limits.square_corner_velocity;
    double      cur_accel_to_decel       = std::min(requested_accel_to_decel, cur_accel);
    double      cur_pa                   = initial_pressure_advance;
    double      cur_pa_smooth_time       = initial_pa_smooth_time;
    bool        pressure_advance_enabled = initial_pa_commands_enabled;
    double      pending_step_generation_scan_time = -1.0;

    for (size_t li = 0; li < line_count; ++li) {
        const std::string& line = get_line(li);
        size_t             s    = 0;
        while (s < line.size() && (line[s] == ' ' || line[s] == '	'))
            ++s;
        if (s + 1 >= line.size())
            continue;

        if (line.compare(s, 18, "SET_VELOCITY_LIMIT") == 0 || line.compare(s, 18, "set_velocity_limit") == 0) {
            for (size_t i = s; i + 6 < line.size(); ++i) {
                auto match_ci = [&](const char* kw, size_t len) -> bool {
                    for (size_t j = 0; j < len; ++j) {
                        char a = line[i + j], b = kw[j];
                        if (a >= 'a' && a <= 'z')
                            a -= 32;
                        if (b >= 'a' && b <= 'z')
                            b -= 32;
                        if (a != b)
                            return false;
                    }
                    if (i > s && line[i - 1] != ' ' && line[i - 1] != '	')
                        return false;
                    return true;
                };
                if (match_ci("ACCEL=", 6)) {
                    size_t vs  = i + 6;
                    char*  ep  = nullptr;
                    double val = std::strtod(line.c_str() + vs, &ep);
                    if (ep > line.c_str() + vs && val > 0.0) {
                        cur_accel          = val;
                        cur_accel_to_decel = std::min(requested_accel_to_decel, cur_accel);
                    }
                    i = (size_t) (ep - line.c_str());
                } else if (match_ci("ACCEL_TO_DECEL=", 15)) {
                    size_t vs  = i + 15;
                    char*  ep  = nullptr;
                    double val = std::strtod(line.c_str() + vs, &ep);
                    if (ep > line.c_str() + vs && val > 0.0) {
                        requested_accel_to_decel = val;
                        cur_accel_to_decel       = std::min(requested_accel_to_decel, cur_accel);
                    }
                    i = (size_t) (ep - line.c_str());
                } else if (match_ci("SQUARE_CORNER_VELOCITY=", 23)) {
                    size_t vs  = i + 23;
                    char*  ep  = nullptr;
                    double val = std::strtod(line.c_str() + vs, &ep);
                    if (ep > line.c_str() + vs && val >= 0.0)
                        cur_scv = val;
                    i = (size_t) (ep - line.c_str());
                }
            }
            continue;
        }

        if (line.compare(s, 23, "ENABLE_PRESSURE_ADVANCE") == 0
            || line.compare(s, 23, "enable_pressure_advance") == 0) {
            const size_t value_pos = line.find("VALUE=", s);
            const size_t lower_value_pos = line.find("value=", s);
            const size_t pos = value_pos != std::string::npos
                             ? value_pos : lower_value_pos;
            if (pos != std::string::npos) {
                char* end = nullptr;
                const double value = std::strtod(line.c_str() + pos + 6, &end);
                if (end != line.c_str() + pos + 6)
                    pressure_advance_enabled = value >= 0.5;
            }
            continue;
        }

        if ((line.compare(s, 20, "SET_PRESSURE_ADVANCE") == 0
             || line.compare(s, 20, "set_pressure_advance") == 0)
            && pressure_advance_enabled) {
            for (size_t i = s; i + 2 < line.size(); ++i) {
                auto match_ci = [&](const char* kw, size_t len) -> bool {
                    for (size_t j = 0; j < len; ++j) {
                        char a = line[i + j], b = kw[j];
                        if (a >= 'a' && a <= 'z')
                            a -= 32;
                        if (b >= 'a' && b <= 'z')
                            b -= 32;
                        if (a != b)
                            return false;
                    }
                    if (i > s && line[i - 1] != ' ' && line[i - 1] != '	')
                        return false;
                    return true;
                };
                if (match_ci("ADVANCE=", 8)) {
                    size_t vs  = i + 8;
                    char*  ep  = nullptr;
                    double val = std::strtod(line.c_str() + vs, &ep);
                    if (ep > line.c_str() + vs && val >= 0.0)
                        cur_pa = val;
                    i = (size_t) (ep - line.c_str());
                } else if (match_ci("SMOOTH_TIME=", 12)) {
                    size_t vs  = i + 12;
                    char*  ep  = nullptr;
                    double val = std::strtod(line.c_str() + vs, &ep);
                    if (ep > line.c_str() + vs && val >= 0.0)
                        cur_pa_smooth_time = val;
                    i = (size_t) (ep - line.c_str());
                }
            }
            pending_step_generation_scan_time =
                cur_pa > 0.0 ? cur_pa_smooth_time * 0.5 : 0.0;
            continue;
        }

        if (line[s] == 'M' || line[s] == 'm') {
            if (line.compare(s, 4, "M106") == 0 || line.compare(s, 4, "m106") == 0) {
                double sval = 255.0;
                parse_word(line, 'S', sval);
                double value = std::max(0.0, std::min(1.0, sval / 255.0));
                parsed.aux_events.push_back({GCodeAuxKind::NozzleFan, line_idx_offset + (int) li, value > 0.0, value});
                continue;
            }
            if (line.compare(s, 4, "M107") == 0 || line.compare(s, 4, "m107") == 0) {
                parsed.aux_events.push_back({GCodeAuxKind::NozzleFan, line_idx_offset + (int) li, false, 0.0});
                continue;
            }
            if (line.compare(s, 4, "M104") == 0 || line.compare(s, 4, "m104") == 0 || line.compare(s, 4, "M109") == 0 ||
                line.compare(s, 4, "m109") == 0) {
                double sval = -1.0;
                if (parse_word(line, 'S', sval))
                    parsed.aux_events.push_back({GCodeAuxKind::NozzleHeater, line_idx_offset + (int) li, sval > 0.0, sval});
                continue;
            }
            if (line.compare(s, 4, "M140") == 0 || line.compare(s, 4, "m140") == 0 || line.compare(s, 4, "M190") == 0 ||
                line.compare(s, 4, "m190") == 0) {
                double sval = -1.0;
                if (parse_word(line, 'S', sval))
                    parsed.aux_events.push_back({GCodeAuxKind::BedHeater, line_idx_offset + (int) li, sval > 0.0, sval});
                continue;
            }
        }

        if (line[s] == 'G' || line[s] == 'g') {
            if (line.compare(s, 3, "G17") == 0 || line.compare(s, 3, "g17") == 0) {
                arc_plane = ArcPlane::XY;
                continue;
            }
            if (line.compare(s, 3, "G18") == 0 || line.compare(s, 3, "g18") == 0) {
                arc_plane = ArcPlane::XZ;
                continue;
            }
            if (line.compare(s, 3, "G19") == 0 || line.compare(s, 3, "g19") == 0) {
                arc_plane = ArcPlane::YZ;
                continue;
            }
            if (line.compare(s, 3, "G90") == 0 || line.compare(s, 3, "g90") == 0) {
                xyz_relative = false;
                continue;
            }
            if (line.compare(s, 3, "G91") == 0 || line.compare(s, 3, "g91") == 0) {
                xyz_relative = true;
                continue;
            }
            if (line.compare(s, 3, "G92") == 0 || line.compare(s, 3, "g92") == 0) {
                double rx = x, ry = y, rz = z, re = e_abs;
                parse_word(line, 'X', rx);
                parse_word(line, 'Y', ry);
                parse_word(line, 'Z', rz);
                parse_word(line, 'E', re);
                const bool was_positioned = have_pos;
                x                         = rx;
                y                         = ry;
                z                         = rz;
                e_abs                     = re;
                have_pos                  = true;
                if (!was_positioned) {
                    parsed.initial_position      = {x, y, z, 0.0};
                    parsed.have_initial_position = true;
                }
                continue;
            }
        }

        if (line[s] == 'M' || line[s] == 'm') {
            if (line.compare(s, 3, "M83") == 0) {
                e_relative = true;
                continue;
            }
            if (line.compare(s, 3, "M82") == 0) {
                e_relative = false;
                continue;
            }
            if (line.compare(s, 4, "M204") == 0) {
                double accel = -1.0, p = -1.0, t = -1.0, sval = -1.0;
                bool   has_s = parse_word(line, 'S', sval);
                if (has_s) {
                    int accel_S = (int) std::llround(sval);
                    if (accel_S != -1 && accel_S <= 100)
                        accel = 100.0;
                    else
                        accel = sval;
                } else {
                    bool has_p = parse_word(line, 'P', p);
                    bool has_t = parse_word(line, 'T', t);
                    if (has_p && has_t)
                        accel = std::min(p, t);
                }
                if (accel > 0.0) {
                    cur_accel          = accel;
                    cur_accel_to_decel = std::min(requested_accel_to_decel, cur_accel);
                }
            }
            continue;
        }

        if (!(line[s] == 'G' || line[s] == 'g'))
            continue;
        char d = line[s + 1];
        if ((d == '2' || d == '3') && (s + 2 == line.size() || line[s + 2] < '0' || line[s + 2] > '9')) {
            double offset_first = 0.0, offset_second = 0.0, radius = 0.0;
            const char first_word = arc_plane == ArcPlane::XY ? 'I' : arc_plane == ArcPlane::XZ ? 'I' : 'J';
            const char second_word = arc_plane == ArcPlane::XY ? 'J' : 'K';
            const bool has_first = parse_word(line, first_word, offset_first);
            const bool has_second = parse_word(line, second_word, offset_second);
            if (xyz_relative || parse_word(line, 'R', radius) || !(has_first || has_second) || !have_pos)
                continue;
            double xword = x, yword = y, zword = z, eword = 0.0, fv = 0.0;
            parse_word(line, 'X', xword);
            parse_word(line, 'Y', yword);
            parse_word(line, 'Z', zword);
            const bool has_e = parse_word(line, 'E', eword);
            if (parse_word(line, 'F', fv) && fv > 0.0)
                F = fv / 60.0;
            const auto coords = expand_arc_moves({x, y, z}, {xword, yword, zword},
                                                 offset_first, offset_second, d == '2', arc_plane);
            const double total_de = !has_e ? 0.0 : e_relative ? eword : eword - e_abs;
            const double de_per_move = total_de / coords.size();
            for (const auto& coord : coords)
                parsed.moves.push_back({coord[0], coord[1], coord[2], de_per_move, F,
                                        cur_accel, cur_accel_to_decel, cur_scv, cur_pa,
                                        cur_pa_smooth_time, line_idx_offset + (int)li});
            if (has_e && !e_relative)
                e_abs = eword;
            x = xword;
            y = yword;
            z = zword;
            continue;
        }
        if (d != '0' && d != '1')
            continue;
        if (s + 2 < line.size() && line[s + 2] >= '0' && line[s + 2] <= '9')
            continue;
        double xword = 0.0, yword = 0.0, zword = 0.0, eword = 0.0, fv = 0.0;
        bool   hx = parse_word(line, 'X', xword), hy = parse_word(line, 'Y', yword), hz = parse_word(line, 'Z', zword);
        bool   he = parse_word(line, 'E', eword);
        if (parse_word(line, 'F', fv) && fv > 0.0)
            F = fv / 60.0;
        double nx = hx ? (xyz_relative ? x + xword : xword) : x;
        double ny = hy ? (xyz_relative ? y + yword : yword) : y;
        double nz = hz ? (xyz_relative ? z + zword : zword) : z;
        double de = 0.0;
        if (he) {
            if (e_relative)
                de = eword;
            else {
                de    = eword - e_abs;
                e_abs = eword;
            }
        }
        if (!have_pos) {
            x                            = nx;
            y                            = ny;
            z                            = nz;
            have_pos                     = true;
            parsed.initial_position      = {nx, ny, nz, 0.0};
            parsed.have_initial_position = true;
            continue;
        }
        if (nx == x && ny == y && nz == z && de == 0.0)
            continue;
        parsed.moves.push_back(
            {nx, ny, nz, de, F, cur_accel, cur_accel_to_decel, cur_scv, cur_pa, cur_pa_smooth_time, line_idx_offset + (int) li});
        parsed.moves.back().step_generation_scan_time_before =
            pending_step_generation_scan_time;
        pending_step_generation_scan_time = -1.0;
        x = nx;
        y = ny;
        z = nz;
    }
    return parsed;
}

static ParsedGCode parse_gcode_lines_detailed(const std::vector<std::string>& lines,
                                              bool initial_e_relative,
                                              const ToolheadLimits& default_limits,
                                              double initial_pressure_advance,
                                              double initial_pa_smooth_time,
                                              bool initial_pa_commands_enabled,
                                              int line_idx_offset = 0)
{
    return parse_gcode_lines_detailed_impl(
        lines.size(), [&lines](size_t index) -> const std::string& { return lines[index]; },
        initial_e_relative, default_limits, initial_pressure_advance,
        initial_pa_smooth_time, initial_pa_commands_enabled, line_idx_offset);
}

static ParsedGCode parse_gcode_line_refs_detailed(const std::vector<const std::string*>& lines,
                                                  bool initial_e_relative,
                                                  const ToolheadLimits& default_limits,
                                                  double initial_pressure_advance,
                                                  double initial_pa_smooth_time,
                                                  bool initial_pa_commands_enabled,
                                                  int line_idx_offset = 0)
{
    return parse_gcode_lines_detailed_impl(
        lines.size(), [&lines](size_t index) -> const std::string& { return *lines[index]; },
        initial_e_relative, default_limits, initial_pressure_advance,
        initial_pa_smooth_time, initial_pa_commands_enabled, line_idx_offset);
}

ParsedGCode parse_gcode_detailed(const std::string& path, size_t lo, size_t hi)
{
    std::ifstream f(path);
    ParsedGCode   parsed;
    if (!f)
        return parsed;

    std::vector<std::string> lines;
    std::string              line;
    size_t                   ln = 0;
    while (std::getline(f, line)) {
        ++ln;
        if (ln < lo)
            continue;
        if (ln > hi)
            break;
        lines.push_back(line);
    }
    SimConfig cfg;
    // Standalone file parsing has no Creality print-preparation context, so
    // honor PA commands present in the file. Whole-file protection analysis
    // uses SimConfig's post-preparation default instead.
    cfg.pressure_advance_commands_enabled = true;
    return parse_gcode_lines_detailed(lines, false, ToolheadLimits{}, cfg.pressure_advance,
                                      cfg.pa_smooth_time, cfg.pressure_advance_commands_enabled,
                                      (int) (lo > 0 ? lo : 1));
}

std::vector<GMove> parse_gcode(const std::string& path, size_t lo, size_t hi) { return parse_gcode_detailed(path, lo, hi).moves; }

static constexpr double HEATER_PWM_PERIOD = 0.75 * 5.0;

static HostDispatchConfig dispatch_config(const SimConfig& cfg, bool nozzle)
{
    HostDispatchConfig out;
    out.transport = HostTransportKind::Uart;
    out.wire_frequency = nozzle ? cfg.nozzle_baud : cfg.mcu_baud;
    out.clock_frequency = cfg.mcu_freq;
    out.receive_window = nozzle ? cfg.nozzle_receive_window
                                : cfg.mcu_receive_window;
    return out;
}

static int encoded_aux_int_len(uint32_t v)
{
    int32_t sv = (int32_t) v;
    if (sv < (3L << 5) && sv >= -(1L << 5))
        return 1;
    if (sv < (3L << 12) && sv >= -(1L << 12))
        return 2;
    if (sv < (3L << 19) && sv >= -(1L << 19))
        return 3;
    if (sv < (3L << 26) && sv >= -(1L << 26))
        return 4;
    return 5;
}

static int aux_cmd_msg_len(double exec_t, double mcu_freq, double value)
{
    uint32_t clock         = (uint32_t) std::llround(std::max(0.0, exec_t * mcu_freq));
    uint32_t encoded_value = (uint32_t) std::llround(value);
    return 1 + 1 + encoded_aux_int_len(clock) + encoded_aux_int_len(encoded_value);
}
AuxDispatchState append_aux_dispatch_cmds(
    const std::vector<GCodeAuxEvent>& aux_events,
    double segment_end_t,
    double buffer_time_high,
    double serialqueue_advance_window,
    double mcu_freq,
    const std::function<double(int)>& line_exec_time,
    std::vector<HostDispatchCmd>& mcu_cmds,
    std::vector<HostDispatchCmd>& nozzle_cmds,
    AuxDispatchState state)
{
    // MCU handler ????????????????????pwm_event / digital_load_event ??
    // move_queue_pop + move_free ??ISR ?????
    static constexpr double AUX_MIN_SLOT_DURATION = 0.001;
    // ????PWM ????????????5Hz PWM ??0.020s ??????
    static constexpr double FAN_MIN_SLOT_DURATION = 0.020;

    auto add_aux_cmd = [&](std::vector<HostDispatchCmd>& dst, double exec_t, int line_idx, int source_kind, double value = 0.0) {
        double min_t = std::max(0.0, std::max(exec_t - buffer_time_high, exec_t - serialqueue_advance_window));
        // slot_free_time = MCU ??move_free ?????
        // AUX ?????handler waketime ??pop+free????????????
        double          min_dur = (source_kind == HCS_AUX_FAN) ? FAN_MIN_SLOT_DURATION : AUX_MIN_SLOT_DURATION;
        HostDispatchCmd cmd;
        cmd.batch_start_time    = min_t;
        cmd.ready_time          = min_t;
        cmd.min_time            = min_t;
        cmd.req_time            = exec_t;
        cmd.slot_free_time      = exec_t + min_dur;
        cmd.exec_start          = exec_t;
        cmd.exec_end            = exec_t;
        cmd.recv_time           = min_t;
        cmd.line_idx            = line_idx;
        cmd.source_kind         = source_kind;
        cmd.uses_move_slot      = true;
        cmd.background_priority = false;
        cmd.msg_len             = aux_cmd_msg_len(exec_t, mcu_freq, value);
        cmd.command_queue_id    = source_kind == HCS_AUX_FAN ? 10
                                : source_kind == HCS_AUX_HEATER ? 11 : 12;
        dst.push_back(cmd);
    };
    double seg_start_t         = 0.0;
    auto   emit_heater_segment = [&](std::vector<HostDispatchCmd>& dst, bool active, double& next_t, double seg_end_t) {
        if (next_t < seg_start_t)
            next_t = seg_start_t;
        if (!active)
            return;
        double exec_t = next_t;
        while (exec_t <= seg_end_t) {
            add_aux_cmd(dst, exec_t, -1, &dst == &nozzle_cmds ? HCS_AUX_HEATER : HCS_AUX_BED, 0.0);
            exec_t += HEATER_PWM_PERIOD;
        }
        next_t = exec_t;
    };

    for (const GCodeAuxEvent& ev : aux_events) {
        double exec_t = line_exec_time(ev.line_idx);
        emit_heater_segment(nozzle_cmds, state.nozzle_heater_active, state.next_nozzle_pwm_t, exec_t);
        emit_heater_segment(mcu_cmds, state.bed_heater_active, state.next_bed_pwm_t, exec_t);
        seg_start_t = exec_t;
        switch (ev.kind) {
        case GCodeAuxKind::NozzleFan:
            add_aux_cmd(nozzle_cmds, exec_t, ev.line_idx, HCS_AUX_FAN, ev.value * 255.0);
            if (ev.active && ev.value > 0.0 && ev.value < 1.0)
                add_aux_cmd(nozzle_cmds, exec_t + 0.100, ev.line_idx, HCS_AUX_FAN, ev.value * 255.0);
            break;
        case GCodeAuxKind::NozzleHeater: state.nozzle_heater_active = ev.active; break;
        case GCodeAuxKind::BedHeater: state.bed_heater_active = ev.active; break;
        }
    }
    emit_heater_segment(nozzle_cmds, state.nozzle_heater_active, state.next_nozzle_pwm_t, segment_end_t);
    emit_heater_segment(mcu_cmds, state.bed_heater_active, state.next_bed_pwm_t, segment_end_t);
    return state;
}
// ---- full simulation ----
SimReport simulate_full(const std::vector<GMove>& gmoves, const SimConfig& cfg)
{
    ParsedGCode parsed;
    parsed.moves = gmoves;
    return simulate_full(parsed, cfg);
}

SimReport simulate_full(const ParsedGCode& gcode, const SimConfig& cfg)
{
    const std::vector<GMove>& gmoves = gcode.moves;
    SimReport                 rep;
    rep.total_moves = gmoves.size();

    size_t max_line_idx = 0;
    for (const GMove& g : gmoves)
        max_line_idx = std::max(max_line_idx, (size_t) std::max(g.line_idx, 0));
    for (const GCodeAuxEvent& e : gcode.aux_events)
        max_line_idx = std::max(max_line_idx, (size_t) std::max(e.line_idx, 0));

    uint32_t   maxerr = (uint32_t) std::llround(cfg.max_stepper_error * cfg.mcu_freq);
    KlipperMCU mcu_mcu("mcu", cfg.mcu_total, cfg.buffer_time_high, cfg.mcu_freq,
                       0.0, cfg.mcu_reserved_slots, cfg.mcu_pool_total);
    KlipperMCU nozzle_mcu("nozzle_mcu", cfg.nozzle_total, cfg.buffer_time_high,
                          std::make_unique<KlipperSecondarySync>(mcu_mcu.clocksync(), cfg.mcu_freq),
                          0.0, cfg.nozzle_reserved_slots, cfg.nozzle_pool_total);
    // Match configured background allocations on each MCU.
    mcu_mcu.steppersync().pool().set_baseline_frac(cfg.mcu_baseline_frac);
    nozzle_mcu.steppersync().pool().set_baseline_frac(cfg.nozzle_baseline_frac);

    KlipperOnlineStepGeneration online_steps(cfg, mcu_mcu, nozzle_mcu);
    double last_positive_extrusion_cruise = 0.0;
    KlipperSimulationSession    session(
        cfg, gcode.initial_position, gcode.have_initial_position, max_line_idx + 1,
        [&](double host_eventtime) { return mcu_mcu.estimated_print_time(host_eventtime); },
        {},
        [&](const ToolheadFlushWindow& window, KlipperTrapQ& xy, KlipperTrapQ& e) { online_steps.on_flush(window, xy, e); },
        false, nullptr, nullptr, &last_positive_extrusion_cruise);
    for (const GMove& g : gmoves) {
        if (g.step_generation_scan_time_before >= 0.0) {
            session.consume_barrier({
                SimulationBarrierKind::StepGenerationFlush, g.line_idx, 0.0,
                g.step_generation_scan_time_before, false});
        }
        session.consume_move(g);
    }
    session.finish();
    rep.last_positive_extrusion_cruise = last_positive_extrusion_cruise;
    online_steps.finish(session.toolhead().last_kin_move_time(), session.xy_trapq(), session.e_trapq());

    KlipperTrapQ&              xy_tq            = session.xy_trapq();
    KlipperTrapQ&              e_tq             = session.e_trapq();
    KlipperHostToolhead&       toolhead         = session.toolhead();
    rep.toolhead_batch_count = session.process_batch_count();
    rep.toolhead_max_batch_moves = session.max_process_batch_moves();
    rep.toolhead_max_batch_short_moves = session.max_process_batch_short_moves();
    rep.toolhead_max_batch_span = session.max_process_batch_span();
    rep.toolhead_max_batch_start = session.max_process_batch_start();
    rep.toolhead_max_batch_first_line = session.max_process_batch_first_line();
    rep.toolhead_max_batch_last_line = session.max_process_batch_last_line();
    const std::vector<double>& line_start_times = session.line_start_times();
    auto                 xyact = [](const TrapMove& m) { return m.axes_r.x != 0.0 || m.axes_r.y != 0.0; };
    auto                 eact  = [](const TrapMove& m) { return std::fabs(m.start_v) > 1e-12 || std::fabs(m.half_accel) > 1e-12; };
    const double         serialqueue_advance_window = (double) (1ULL << 31) / cfg.mcu_freq;
    KlipperStepCompress& scA                        = online_steps.a_stepqueue();
    KlipperStepCompress& scB                        = online_steps.b_stepqueue();
    KlipperStepCompress& scE                        = online_steps.e_stepqueue();
    rep.xy_cmds                                     = scA.command_count() + scB.command_count();
    rep.e_cmds                                      = scE.command_count();
    annotate_stepcompress_commands(scA, xy_tq.moves(), xyact, cfg.mcu_freq);
    annotate_stepcompress_commands(scB, xy_tq.moves(), xyact, cfg.mcu_freq);
    annotate_stepcompress_commands(scE, e_tq.moves(), eact, cfg.mcu_freq);
    std::vector<HostDispatchCmd> mcuCmds = online_steps.take_main_commands();
    std::vector<HostDispatchCmd> nozCmds = online_steps.take_nozzle_commands();
    mcuCmds.reserve(scA.commands().size() + scA.aux_commands().size() + scB.commands().size() + scB.aux_commands().size() +
                    gcode.aux_events.size());
    nozCmds.reserve(scE.commands().size() + scE.aux_commands().size() + gcode.aux_events.size());
    scA.release_output_storage();
    scB.release_output_storage();
    scE.release_output_storage();
    xy_tq.release_storage();
    e_tq.release_storage();

    std::vector<double> next_line_exec(line_start_times.size(), toolhead.last_kin_move_time());
    double              next_exec = toolhead.last_kin_move_time();
    for (size_t i = line_start_times.size(); i-- > 0;) {
        if (line_start_times[i] >= 0.0)
            next_exec = line_start_times[i];
        next_line_exec[i] = next_exec;
    }
    auto line_exec_time = [&](int line_idx) {
        if (line_idx >= 0 && (size_t) line_idx < next_line_exec.size())
            return next_line_exec[(size_t) line_idx];
        return toolhead.last_kin_move_time();
    };
    append_aux_dispatch_cmds(gcode.aux_events, toolhead.last_kin_move_time(), cfg.buffer_time_high, serialqueue_advance_window,
                             cfg.mcu_freq, line_exec_time, mcuCmds, nozCmds);

    const LateBatchEnvelope nozzle_late = analyze_late_batch_envelope(
        nozCmds, nozzle_mcu.host_move_slots(), cfg.nozzle_pool_total,
        dispatch_config(cfg, true), 0.250,
        static_cast<size_t>(std::ceil(cfg.buffer_time_high / 0.250)));
    rep.nozzle_late_batch_peak = nozzle_late.peak;
    rep.nozzle_late_batch_overflow = nozzle_late.overflow;
    rep.nozzle_late_batch_peak_t = nozzle_late.peak_time;
    rep.nozzle_late_batch_source_t = nozzle_late.source_batch_start;
    rep.nozzle_late_batch_first_line = nozzle_late.first_line;
    rep.nozzle_late_batch_last_line = nozzle_late.last_line;
    rep.nozzle_late_batch_commands = nozzle_late.move_commands;
    rep.nozzle_physical_upper_peak = nozzle_late.move_commands == 0 ? 0
        : std::min(cfg.nozzle_pool_total,
            nozzle_late.host_step_peak
                + std::max(0, cfg.nozzle_pool_total
                                - nozzle_mcu.host_move_slots()));
    rep.nozzle_physical_upper_saturated =
        rep.nozzle_physical_upper_peak >= cfg.nozzle_pool_total;

    // Stage 6: serialqueue delivery is the authoritative MCU command-handler
    // boundary.  Reset the construction-time steppersync state, then allocate
    // shared pool nodes directly as each complete wire block is received.
    mcu_mcu.steppersync().clear();
    nozzle_mcu.steppersync().clear();
    schedule_host_dispatch(
        mcuCmds, nozCmds, toolhead.flush_windows(), toolhead.host_eventtime(),
        cfg.buffer_time_high, mcu_mcu.host_move_slots(),
        nozzle_mcu.host_move_slots(),
        [&](double eventtime) { return mcu_mcu.estimated_print_time(eventtime); },
        [&](double eventtime) { return nozzle_mcu.estimated_print_time(eventtime); },
        dispatch_config(cfg, false), dispatch_config(cfg, true),
        [&](const HostDispatchCmd& command) {
            mcu_mcu.steppersync().consume_received(command);
        },
        [&](const HostDispatchCmd& command) {
            nozzle_mcu.steppersync().consume_received(command);
        });

    std::vector<HostDispatchCmd>().swap(mcuCmds);
    std::vector<HostDispatchCmd>().swap(nozCmds);

    auto mr              = mcu_mcu.steppersync().analyze();
    auto nr              = nozzle_mcu.steppersync().analyze();
    rep.mcu_peak         = mr.peak;
    rep.mcu_peak_t       = mr.peak_time;
    rep.mcu_overflow     = mr.overflow;
    rep.nozzle_peak      = nr.peak;
    rep.nozzle_peak_t    = nr.peak_time;
    rep.nozzle_overflow  = nr.overflow;
    rep.nozzle_avg_dur   = nr.avg_exec_dur;
    rep.nozzle_max_dur   = nr.max_exec_dur;
    rep.mcu_avg_dur      = mr.avg_exec_dur;
    rep.mcu_max_dur      = mr.max_exec_dur;
    rep.print_time_total = toolhead.last_kin_move_time();
    rep.nozzle_samples   = nr.samples;
    rep.mcu_samples      = mr.samples;
    rep.nozzle_total      = nozzle_mcu.steppersync().pool().total();
    rep.mcu_total         = mcu_mcu.steppersync().pool().total();
    rep.nozzle_host_slots = nozzle_mcu.host_move_slots();
    rep.mcu_host_slots    = mcu_mcu.host_move_slots();
    if (mr.overflow || nr.overflow) {
        if (nr.overflow && (!mr.overflow || nr.peak_time <= mr.peak_time)) {
            rep.first_overflow_time = nr.peak_time;
            rep.first_overflow_mcu  = "nozzle_mcu";
        } else {
            rep.first_overflow_time = mr.peak_time;
            rep.first_overflow_mcu  = "mcu";
        }
    }
    return rep;
}

double compute_peak_occupancy(const std::vector<std::pair<double, int>>& events, int total)
{
    if (events.empty())
        return 0.0;
    int cur  = 0;
    int peak = 0;
    for (const auto& e : events) {
        cur += e.second;
        if (cur > peak)
            peak = cur;
    }
    return total > 0 ? (double) peak / (double) total : 0.0;
}

// ---- line-level analysis for filter integration ----
std::vector<char> analyze_gcode_lines(const std::vector<std::string>& lines,
                                      const SimConfig&                cfg,
                                      double                          threshold,
                                      LineAnalysisState&              st,
                                      LineAnalysisDebug*              debug,
                                      const std::function<void()>&    cancel,
                                      bool                            compact_streaming,
                                      std::vector<double>*            positive_extrusion_cruise_by_line,
                                      std::vector<double>*            positive_extrusion_distance_by_line,
                                      EarlyRiskResult*                early_risk,
                                      bool                            streaming_early_probe)
{
    if (early_risk != nullptr) {
        *early_risk = EarlyRiskResult{};
        early_risk->threshold = threshold;
        early_risk->lines_consumed = lines.size();
        compact_streaming = true;
    }
    if (!st.simulation_config_initialized) {
        st.current_pressure_advance = cfg.pressure_advance;
        st.current_pa_smooth_time = cfg.pa_smooth_time;
        st.pressure_advance_enabled = cfg.pressure_advance_commands_enabled;
        st.simulation_config_initialized = true;
    }
    if (early_risk != nullptr && streaming_early_probe) {
        const bool handled = run_streaming_early_probe(
            lines, cfg, threshold, st, cancel, *early_risk);
        if (handled) {
            early_risk->streaming_fast_path = true;
            return {};
        }
        early_risk->used_reference_fallback = true;
    }
    std::vector<char> flags;
    if (early_risk == nullptr)
        flags.assign(lines.size(), 0);
    if (debug != nullptr)
        *debug = LineAnalysisDebug{};
    if (positive_extrusion_cruise_by_line != nullptr)
        positive_extrusion_cruise_by_line->assign(lines.size(), 0.0);
    if (positive_extrusion_distance_by_line != nullptr)
        positive_extrusion_distance_by_line->assign(lines.size(), 0.0);
    auto check_cancel = [&](size_t counter = 0) {
        if (cancel && (counter & 0x3ffu) == 0)
            cancel();
    };
    check_cancel();
    SimConfig lcfg                     = cfg;
    lcfg.limits.max_accel              = st.current_accel;
    lcfg.limits.max_accel_to_decel     = std::min(st.requested_accel_to_decel, lcfg.limits.max_accel);
    lcfg.limits.square_corner_velocity = st.current_square_corner_velocity;
    lcfg.limits.junction_deviation     = calc_junction_deviation(lcfg.limits.square_corner_velocity, lcfg.limits.max_accel);

    const double start_x        = st.x;
    const double start_y        = st.y;
    const double start_z        = st.z;
    const double start_e        = st.e_abs;
    const bool   start_have_pos = st.have_pos;

    ParsedGCode parsed;
    parsed.aux_events.reserve(lines.size());

    uint32_t   maxerr = (uint32_t) std::llround(lcfg.max_stepper_error * lcfg.mcu_freq);
    KlipperMCU real_layer_mcu("mcu", lcfg.mcu_total, lcfg.buffer_time_high,
                              lcfg.mcu_freq, 0.0, lcfg.mcu_reserved_slots,
                              lcfg.mcu_pool_total);
    KlipperMCU real_layer_nozzle("nozzle_mcu", lcfg.nozzle_total, lcfg.buffer_time_high,
                                 std::make_unique<KlipperSecondarySync>(real_layer_mcu.clocksync(), lcfg.mcu_freq), 0.0,
                                 lcfg.nozzle_reserved_slots, lcfg.nozzle_pool_total);
    real_layer_mcu.steppersync().pool().set_baseline_frac(lcfg.mcu_baseline_frac);
    real_layer_nozzle.steppersync().pool().set_baseline_frac(lcfg.nozzle_baseline_frac);

    KlipperOnlineStepGeneration online_steps(lcfg, real_layer_mcu, real_layer_nozzle);
    PathologicalMotionHistory pathological_motion_history;
    SessionPlannedBatchCb pathological_batch_cb;
    if (early_risk != nullptr) {
        pathological_batch_cb = [&](const std::vector<PlanMove>& batch,
                                    double batch_start) {
            pathological_motion_history.append_planned_batch(batch, batch_start);
        };
    }
    KlipperSimulationSession    session(
        lcfg, {start_x + st.x_offset, start_y + st.y_offset, start_z + st.z_offset, start_e}, start_have_pos, lines.size(),
        [&](double host_eventtime) { return real_layer_mcu.estimated_print_time(host_eventtime); },
        {},
        [&](const ToolheadFlushWindow& window, KlipperTrapQ& xy, KlipperTrapQ& e) { online_steps.on_flush(window, xy, e); },
        false,
        positive_extrusion_cruise_by_line,
        positive_extrusion_distance_by_line,
        nullptr,
        std::move(pathological_batch_cb));

    GCodeStreamCallbacks callbacks;
    callbacks.on_move     = [&](const GMove& move) { session.consume_move(move); };
    callbacks.on_aux      = [&](const GCodeAuxEvent& event) { parsed.aux_events.push_back(event); };
    callbacks.on_barrier  = [&](const SimulationBarrier& barrier) { session.consume_barrier(barrier); };
    callbacks.on_position = [&](const std::array<double, 4>& position, bool valid) { session.set_position(position, valid); };
    KlipperGCodeStream stream(lcfg, st, std::move(callbacks));

    for (size_t li = 0; li < lines.size(); ++li) {
        check_cancel(li);
        stream.consume_line(lines[li], static_cast<int>(li));
    }
    session.finish();
    online_steps.finish(session.toolhead().last_kin_move_time(), session.xy_trapq(), session.e_trapq());

    lcfg.limits.max_accel                       = st.current_accel;
    lcfg.limits.max_accel_to_decel              = st.current_accel_to_decel;
    lcfg.limits.square_corner_velocity          = st.current_square_corner_velocity;
    lcfg.limits.junction_deviation              = calc_junction_deviation(lcfg.limits.square_corner_velocity, lcfg.limits.max_accel);
    const double requested_accel_to_decel       = st.requested_accel_to_decel;
    const double current_square_corner_velocity = st.current_square_corner_velocity;
    const double current_pressure_advance       = st.current_pressure_advance;
    const double current_pa_smooth_time         = st.current_pa_smooth_time;

    KlipperTrapQ&              xy_tq            = session.xy_trapq();
    KlipperTrapQ&              e_tq             = session.e_trapq();
    KlipperHostToolhead&       toolhead         = session.toolhead();
    const std::vector<double>& line_start_times = session.line_start_times();
    const std::vector<double>& line_end_times   = session.line_end_times();

    if (debug != nullptr) {
        debug->parsed_moves = stream.stats().moves;
        debug->aux_events   = stream.stats().aux_events;
        debug->toolhead_batch_count = session.process_batch_count();
        debug->toolhead_max_batch_moves = session.max_process_batch_moves();
        debug->toolhead_max_batch_short_moves = session.max_process_batch_short_moves();
        debug->toolhead_max_batch_span = session.max_process_batch_span();
        debug->toolhead_max_batch_start = session.max_process_batch_start();
        debug->toolhead_max_batch_first_line = session.max_process_batch_first_line();
        debug->toolhead_max_batch_last_line = session.max_process_batch_last_line();
    }
    if (!start_have_pos && stream.stats().moves == 0 && stream.stats().aux_events == 0 && stream.stats().barriers == 0) {
        return flags;
    }

    if (debug != nullptr) {
        debug->xy_trap_moves = online_steps.total_xy_phases();
        debug->e_trap_moves  = online_steps.total_e_phases();
        debug->mcu_cmds      = online_steps.a_stepqueue().command_count() + online_steps.b_stepqueue().command_count();
        debug->nozzle_cmds   = online_steps.e_stepqueue().command_count();
    }

    auto                 xyact = [](const TrapMove& m) { return m.axes_r.x != 0.0 || m.axes_r.y != 0.0; };
    auto                 eact  = [](const TrapMove& m) { return std::fabs(m.start_v) > 1e-12 || std::fabs(m.half_accel) > 1e-12; };
    const double         serialqueue_advance_window = (double) (1ULL << 31) / lcfg.mcu_freq;
    KlipperStepCompress& scA                        = online_steps.a_stepqueue();
    KlipperStepCompress& scB                        = online_steps.b_stepqueue();
    KlipperStepCompress& scE                        = online_steps.e_stepqueue();
    if (compact_streaming && st.global_time == 0.0 && st.mcu_events.empty() && st.noz_events.empty()) {
        struct CompactPending
        {
            double recv_time      = 0.0;
            double req_time       = 0.0;
            double min_time       = 0.0;
            double slot_free_time = 0.0;
            double exec_start     = 0.0;
            double exec_end       = 0.0;
            int    line_idx       = -1;
            int    source_kind    = HCS_UNKNOWN;
        };
        struct PendingStream
        {
            struct EarlierRelease
            {
                bool operator()(const CompactPending& lhs,
                                const CompactPending& rhs) const
                {
                    return std::max(lhs.recv_time, lhs.slot_free_time)
                         > std::max(rhs.recv_time, rhs.slot_free_time);
                }
            };
            std::priority_queue<CompactPending, std::vector<CompactPending>,
                                EarlierRelease> provenance;
            size_t                     count     = 0;
            bool                       have_last = false;
            CompactPending             last{};
        };
        const auto& compact_windows = toolhead.flush_windows();
        std::vector<double> compact_next_line_exec(line_start_times.size(), toolhead.last_kin_move_time());
        double              compact_next_exec = toolhead.last_kin_move_time();
        for (size_t i = line_start_times.size(); i-- > 0;) {
            if (line_start_times[i] >= 0.0)
                compact_next_exec = line_start_times[i];
            compact_next_line_exec[i] = compact_next_exec;
        }
        auto compact_line_exec_time = [&](int line_idx) {
            if (line_idx >= 0 && (size_t) line_idx < compact_next_line_exec.size())
                return compact_next_line_exec[(size_t) line_idx];
            return toolhead.last_kin_move_time();
        };
        AuxDispatchState compact_aux_base;
        compact_aux_base.nozzle_heater_active = st.extruder_heater_active;
        compact_aux_base.bed_heater_active    = st.bed_heater_active;
        compact_aux_base.next_nozzle_pwm_t    = std::max(0.0, st.next_extruder_pwm_time - st.global_time);
        compact_aux_base.next_bed_pwm_t       = std::max(0.0, st.next_bed_pwm_time - st.global_time);
        std::vector<HostDispatchCmd> compact_mcu_dispatch = online_steps.take_main_commands();
        std::vector<HostDispatchCmd> compact_noz_dispatch = online_steps.take_nozzle_commands();
        // Ownership has moved into the dispatch vectors; compact production
        // analysis no longer needs the complete stepcompress/trapq history.
        scA.release_output_storage();
        scB.release_output_storage();
        scE.release_output_storage();
        xy_tq.release_storage();
        e_tq.release_storage();
        AuxDispatchState             compact_aux_state = append_aux_dispatch_cmds(parsed.aux_events, toolhead.last_kin_move_time(),
                                                                                  lcfg.buffer_time_high, serialqueue_advance_window, lcfg.mcu_freq,
                                                                                  compact_line_exec_time, compact_mcu_dispatch, compact_noz_dispatch,
                                                                                  compact_aux_base);

        const int mcu_threshold = (int) (lcfg.mcu_pool_total * threshold);
        const int noz_threshold = (int) (lcfg.nozzle_pool_total * threshold);
        McuMovePool compact_mcu_pool(lcfg.mcu_pool_total);
        McuMovePool compact_noz_pool(lcfg.nozzle_pool_total);
        compact_mcu_pool.set_baseline_frac(lcfg.mcu_baseline_frac);
        compact_noz_pool.set_baseline_frac(lcfg.nozzle_baseline_frac);
        if (early_risk != nullptr) {
            // P1 Boolean reference mode: planning and command generation above
            // are shared with full analysis, but receive-side collection keeps
            // no flags, provenance, high intervals, or event timeline.
            compact_mcu_pool.set_compact_timeline(0);
            compact_noz_pool.set_compact_timeline(0);

            struct Candidate {
                bool hit = false;
                EarlyRiskSource source = EarlyRiskSource::None;
                int occupied = 0;
                int total = 0;
                int line_idx = -1;
                double time = -1.0;
            };
            Candidate main_candidate, nozzle_candidate;
            auto consume_until_risk = [&](McuMovePool& pool,
                                          int threshold_slots,
                                          EarlyRiskSource source,
                                          Candidate& candidate,
                                          const HostDispatchCmd& command) {
                if (!command.uses_move_slot)
                    return SimulationControl::Continue;
                const double recv_time = std::max(command.recv_time,
                                                  command.min_time);
                pool.alloc(recv_time,
                           std::max(recv_time, command.slot_free_time),
                           command.line_idx, command.source_kind);
                // Compare the current modeled allocation count, not the
                // historical peak. A pressure-only hit keeps simulation going.
                const int occupied = pool.current_modeled_occupancy();
                const double hit_time = std::max(0.0, command.exec_start);
                if (threshold_slots > 0 && occupied >= threshold_slots
                    && pathological_motion_history.has_microsequence(hit_time)) {
                    candidate = {true, source, occupied, pool.total(),
                                 command.line_idx, recv_time};
                    return SimulationControl::StopRiskDetected;
                }
                return SimulationControl::Continue;
            };

            early_risk->main_commands_total = compact_mcu_dispatch.size();
            early_risk->nozzle_commands_total = compact_noz_dispatch.size();
            size_t main_probe_receive_count = 0;
            const DualHostDispatchOutcome outcome = schedule_host_dispatch_until(
                compact_mcu_dispatch, compact_noz_dispatch, compact_windows,
                toolhead.host_eventtime(), lcfg.buffer_time_high,
                real_layer_mcu.host_move_slots(), real_layer_nozzle.host_move_slots(),
                [&](double eventtime) { return real_layer_mcu.estimated_print_time(eventtime); },
                [&](double eventtime) { return real_layer_nozzle.estimated_print_time(eventtime); },
                dispatch_config(lcfg, false), dispatch_config(lcfg, true),
                [&](const HostDispatchCmd& command) {
                    check_cancel(main_probe_receive_count++);
                    return consume_until_risk(compact_mcu_pool, mcu_threshold,
                                              EarlyRiskSource::MainMcu,
                                              main_candidate, command);
                },
                [&](const HostDispatchCmd& command) {
                    return consume_until_risk(compact_noz_pool, noz_threshold,
                                              EarlyRiskSource::NozzleMcu,
                                              nozzle_candidate, command);
                });
            early_risk->main_commands_consumed = outcome.primary.commands_consumed;
            early_risk->nozzle_commands_consumed = outcome.secondary.commands_consumed;

            const Candidate* first = nullptr;
            if (main_candidate.hit)
                first = &main_candidate;
            if (nozzle_candidate.hit
                && (first == nullptr || nozzle_candidate.time < first->time))
                first = &nozzle_candidate;
            if (first != nullptr) {
                early_risk->detected = true;
                early_risk->source = first->source;
                early_risk->occupied_slots = first->occupied;
                early_risk->total_slots = first->total;
                early_risk->occupancy = first->total > 0
                    ? static_cast<double>(first->occupied) / first->total : 0.0;
                early_risk->line_idx = first->line_idx;
                early_risk->simulated_time = st.global_time + first->time;
            }
            st.global_time += std::max(0.0, toolhead.last_kin_move_time());
            return {};
        }
        compact_mcu_pool.set_compact_timeline(mcu_threshold);
        compact_noz_pool.set_compact_timeline(noz_threshold);
        PendingStream stream_mcu_step, stream_noz_step;
        std::vector<char> compact_mcu_flags(flags.size(), 0);
        std::vector<char> compact_noz_flags(flags.size(), 0);
        auto flag_provenance = [](std::vector<char>& target_flags,
                                  const CompactPending& pending) {
            if (pending.line_idx >= 0
                && pending.line_idx < static_cast<int>(target_flags.size()))
                target_flags[static_cast<size_t>(pending.line_idx)] = 1;
        };
        auto consume_compact = [&](McuMovePool& pool, PendingStream& stream,
                                   std::vector<char>& target_flags,
                                   const HostDispatchCmd& command) {
            if (!command.uses_move_slot)
                return;
            CompactPending received{
                std::max(command.recv_time, command.min_time), command.req_time,
                command.min_time, std::max(0.0, command.slot_free_time),
                command.exec_start, std::max(command.exec_start, command.exec_end),
                command.line_idx, command.source_kind};
            if (stream.have_last && received.recv_time < stream.last.recv_time)
                throw std::logic_error("KlipperSim serialqueue receive frontier regressed");
            stream.last = received;
            stream.have_last = true;
            ++stream.count;
            pool.alloc(received.recv_time,
                       std::max(received.recv_time, received.slot_free_time),
                       received.line_idx, received.source_kind);
            while (!stream.provenance.empty()
                   && std::max(stream.provenance.top().recv_time,
                               stream.provenance.top().slot_free_time)
                          <= received.recv_time)
                stream.provenance.pop();
            if (pool.compact_open_high_start() >= 0.0) {
                while (!stream.provenance.empty()) {
                    flag_provenance(target_flags, stream.provenance.top());
                    stream.provenance.pop();
                }
                flag_provenance(target_flags, received);
            } else {
                stream.provenance.push(received);
            }
        };
        LateBatchEnvelope compact_nozzle_late;
        if (debug != nullptr) {
            compact_nozzle_late = analyze_late_batch_envelope(
                compact_noz_dispatch, real_layer_nozzle.host_move_slots(),
                lcfg.nozzle_pool_total, dispatch_config(lcfg, true), 0.250,
                static_cast<size_t>(std::ceil(lcfg.buffer_time_high / 0.250)));
        }
        size_t compact_mcu_receive_count = 0;

        schedule_host_dispatch(
            compact_mcu_dispatch, compact_noz_dispatch, compact_windows,
            toolhead.host_eventtime(), lcfg.buffer_time_high,
            real_layer_mcu.host_move_slots(), real_layer_nozzle.host_move_slots(),
            [&](double eventtime) { return real_layer_mcu.estimated_print_time(eventtime); },
            [&](double eventtime) { return real_layer_nozzle.estimated_print_time(eventtime); },
            dispatch_config(lcfg, false), dispatch_config(lcfg, true),
            [&](const HostDispatchCmd& command) {
                check_cancel(compact_mcu_receive_count++);
                consume_compact(compact_mcu_pool, stream_mcu_step,
                                compact_mcu_flags, command);
            },
            [&](const HostDispatchCmd& command) {
                consume_compact(compact_noz_pool, stream_noz_step,
                                compact_noz_flags, command);
            });
        for (size_t i = 0; i < flags.size(); ++i)
            flags[i] = static_cast<char>(flags[i] || compact_mcu_flags[i]
                                       || compact_noz_flags[i]);

        compact_mcu_pool.finish_compact_timeline();
        compact_noz_pool.finish_compact_timeline();
        while (!stream_mcu_step.provenance.empty())
            stream_mcu_step.provenance.pop();
        while (!stream_noz_step.provenance.empty())
            stream_noz_step.provenance.pop();
        const double compact_t0        = st.global_time;
        const double compact_layer_dur = std::max(0.0, toolhead.last_kin_move_time());
        auto         shifted_segments  = [&](const std::vector<std::pair<double, double>>& src) {
            std::vector<std::pair<double, double>> out;
            out.reserve(src.size());
            for (const auto& seg : src)
                out.push_back({compact_t0 + seg.first, compact_t0 + seg.second});
            return out;
        };
        std::vector<std::pair<double, double>> compact_mcu_hi  = shifted_segments(compact_mcu_pool.compact_high_segments());
        std::vector<std::pair<double, double>> compact_noz_hi  = shifted_segments(compact_noz_pool.compact_high_segments());
        const double compact_mcu_peak = lcfg.mcu_pool_total > 0 ? (double) compact_mcu_pool.peak() / (double) lcfg.mcu_pool_total : 0.0;
        const double compact_noz_peak = lcfg.nozzle_pool_total > 0 ? (double) compact_noz_pool.peak() / (double) lcfg.nozzle_pool_total : 0.0;
        if (debug != nullptr) {
            debug->layer_window_start = compact_t0;
            debug->layer_window_end   = compact_t0 + compact_layer_dur;
            debug->mcu_peak_frac      = compact_mcu_peak;
            debug->nozzle_peak_frac   = compact_noz_peak;
            debug->mcu_peak_time      = compact_t0 + compact_mcu_pool.peak_time();
            debug->nozzle_peak_time   = compact_t0 + compact_noz_pool.peak_time();
            debug->nozzle_late_batch_peak_slots = compact_nozzle_late.peak;
            debug->nozzle_late_batch_peak_frac = lcfg.nozzle_pool_total > 0
                ? (double) compact_nozzle_late.peak
                    / (double) lcfg.nozzle_pool_total
                : 0.0;
            debug->nozzle_late_batch_peak_time = compact_nozzle_late.peak_time;
            debug->nozzle_late_batch_first_line = compact_nozzle_late.first_line;
            debug->nozzle_late_batch_last_line = compact_nozzle_late.last_line;
            debug->nozzle_physical_upper_peak_slots =
                compact_nozzle_late.move_commands == 0 ? 0
                : std::min(lcfg.nozzle_pool_total,
                    compact_nozzle_late.host_step_peak
                        + std::max(0, lcfg.nozzle_pool_total
                                        - real_layer_nozzle.host_move_slots()));
            debug->nozzle_physical_upper_peak_frac = lcfg.nozzle_pool_total > 0
                ? static_cast<double>(debug->nozzle_physical_upper_peak_slots)
                    / static_cast<double>(lcfg.nozzle_pool_total)
                : 0.0;
            debug->mcu_cmds           = stream_mcu_step.count;
            debug->nozzle_cmds        = stream_noz_step.count;
            debug->mcu_hi_segments    = compact_mcu_hi;
            debug->nozzle_hi_segments = compact_noz_hi;
        }
        st.global_time += compact_layer_dur;
        st.current_accel                  = lcfg.limits.max_accel;
        st.requested_accel_to_decel       = requested_accel_to_decel;
        st.current_accel_to_decel         = lcfg.limits.max_accel_to_decel;
        st.current_square_corner_velocity = current_square_corner_velocity;
        st.current_pressure_advance       = current_pressure_advance;
        st.current_pa_smooth_time         = current_pa_smooth_time;
        st.extruder_heater_active         = compact_aux_state.nozzle_heater_active;
        st.bed_heater_active              = compact_aux_state.bed_heater_active;
        st.next_extruder_pwm_time         = st.global_time + compact_aux_state.next_nozzle_pwm_t;
        st.next_bed_pwm_time              = st.global_time + compact_aux_state.next_bed_pwm_t;
        return flags;
    }
    annotate_stepcompress_commands(scA, xy_tq.moves(), xyact, lcfg.mcu_freq);
    annotate_stepcompress_commands(scB, xy_tq.moves(), xyact, lcfg.mcu_freq);
    annotate_stepcompress_commands(scE, e_tq.moves(), eact, lcfg.mcu_freq);
    if (debug != nullptr) {
        debug->xy_trap_moves = online_steps.total_xy_phases();
        debug->e_trap_moves  = online_steps.total_e_phases();
        debug->mcu_cmds      = (int) (scA.command_count() + scB.command_count());
        debug->nozzle_cmds   = (int) scE.command_count();
    }
    std::vector<HostDispatchCmd> mcuCmds = online_steps.take_main_commands();
    std::vector<HostDispatchCmd> nozCmds = online_steps.take_nozzle_commands();
    mcuCmds.reserve(scA.commands().size() + scA.aux_commands().size() + scB.commands().size() + scB.aux_commands().size() +
                    parsed.aux_events.size());
    nozCmds.reserve(scE.commands().size() + scE.aux_commands().size() + parsed.aux_events.size());
    scA.release_output_storage();
    scB.release_output_storage();
    scE.release_output_storage();
    xy_tq.release_storage();
    e_tq.release_storage();
    std::vector<double> next_line_exec(line_start_times.size(), toolhead.last_kin_move_time());
    double              next_exec = toolhead.last_kin_move_time();
    for (size_t i = line_start_times.size(); i-- > 0;) {
        if (line_start_times[i] >= 0.0)
            next_exec = line_start_times[i];
        next_line_exec[i] = next_exec;
    }
    auto line_exec_time = [&](int line_idx) {
        if (line_idx >= 0 && (size_t) line_idx < next_line_exec.size())
            return next_line_exec[(size_t) line_idx];
        return toolhead.last_kin_move_time();
    };
    AuxDispatchState aux_state_base;
    aux_state_base.nozzle_heater_active = st.extruder_heater_active;
    aux_state_base.bed_heater_active    = st.bed_heater_active;
    aux_state_base.next_nozzle_pwm_t    = std::max(0.0, st.next_extruder_pwm_time - st.global_time);
    aux_state_base.next_bed_pwm_t       = std::max(0.0, st.next_bed_pwm_time - st.global_time);
    AuxDispatchState aux_state          = append_aux_dispatch_cmds(parsed.aux_events, toolhead.last_kin_move_time(), lcfg.buffer_time_high,
                                                                   serialqueue_advance_window, lcfg.mcu_freq, line_exec_time, mcuCmds, nozCmds,
                                                                   aux_state_base);

    LateBatchEnvelope layer_nozzle_late;
    if (debug != nullptr) {
        layer_nozzle_late = analyze_late_batch_envelope(
            nozCmds, real_layer_nozzle.host_move_slots(),
            lcfg.nozzle_pool_total, dispatch_config(lcfg, true), 0.250,
            static_cast<size_t>(std::ceil(lcfg.buffer_time_high / 0.250)));
    }

    real_layer_mcu.steppersync().clear();
    real_layer_nozzle.steppersync().clear();
    size_t receive_count = 0;
    schedule_host_dispatch(
        mcuCmds, nozCmds, toolhead.flush_windows(), toolhead.host_eventtime(),
        lcfg.buffer_time_high, real_layer_mcu.host_move_slots(),
        real_layer_nozzle.host_move_slots(),
        [&](double eventtime) { return real_layer_mcu.estimated_print_time(eventtime); },
        [&](double eventtime) { return real_layer_nozzle.estimated_print_time(eventtime); },
        dispatch_config(lcfg, false), dispatch_config(lcfg, true),
        [&](const HostDispatchCmd& command) {
            check_cancel(receive_count++);
            real_layer_mcu.steppersync().consume_received(command);
        },
        [&](const HostDispatchCmd& command) {
            check_cancel(receive_count++);
            real_layer_nozzle.steppersync().consume_received(command);
        });

    std::vector<HostDispatchCmd>().swap(mcuCmds);
    std::vector<HostDispatchCmd>().swap(nozCmds);

    // Keep per-layer event timelines monotonic. Host/toolhead semantics already
    // model buffer restart inside flush windows, so layer merge must not
    // subtract buffer_time_start again.
    const double                        t0        = st.global_time;
    const double                        layer_dur = std::max(0.0, toolhead.last_kin_move_time());
    std::vector<std::pair<double, int>> new_mcu, new_noz;
    real_layer_mcu.steppersync().build_events(t0, new_mcu);
    real_layer_nozzle.steppersync().build_events(t0, new_noz);
    if (debug != nullptr) {
        debug->new_mcu_events    = new_mcu.size();
        debug->new_nozzle_events = new_noz.size();
    }

    auto merge_events = [](std::vector<std::pair<double, int>>& dst, const std::vector<std::pair<double, int>>& src) {
        std::vector<std::pair<double, int>> merged;
        merged.reserve(dst.size() + src.size());
        std::merge(dst.begin(), dst.end(), src.begin(), src.end(), std::back_inserter(merged),
                   [](const std::pair<double, int>& a, const std::pair<double, int>& b) { return a.first < b.first; });
        dst = std::move(merged);
    };
    auto collect_hi = [&](const std::vector<std::pair<double, int>>& events, int total, double* peak_frac_out, double* peak_time_out) {
        std::vector<std::pair<double, double>> hi;
        if (peak_frac_out)
            *peak_frac_out = 0.0;
        if (peak_time_out)
            *peak_time_out = -1.0;
        if (events.empty() || total <= 0)
            return hi;
        const int thr = (int) (total * threshold);
        if (thr <= 0)
            return hi;
        const double win_start = t0 - lcfg.buffer_time_high;
        const double win_end   = t0 + layer_dur;
        int          occ       = 0;
        int          peak_occ  = 0;
        double       peak_time = -1.0;
        size_t       evi       = 0;
        while (evi < events.size() && events[evi].first < win_start) {
            occ += events[evi].second;
            ++evi;
        }
        double seg = -1.0;
        while (evi < events.size() && events[evi].first <= win_end) {
            occ += events[evi].second;
            double t = events[evi].first;
            if (occ > peak_occ) {
                peak_occ  = occ;
                peak_time = t;
            }
            if (occ >= thr && seg < 0.0)
                seg = t;
            else if (occ < thr && seg >= 0.0) {
                hi.push_back({seg, t});
                seg = -1.0;
            }
            ++evi;
        }
        if (seg >= 0.0)
            hi.push_back({seg, win_end});
        if (peak_frac_out)
            *peak_frac_out = total > 0 ? (double) peak_occ / (double) total : 0.0;
        if (peak_time_out)
            *peak_time_out = peak_time;
        return hi;
    };

    auto mark_layer = [&](const KlipperSteppersync& sync, const std::vector<std::pair<double, double>>& hi) {
        if (hi.empty())
            return;
        size_t command_index = 0;
        for (const auto& c : sync.commands()) {
            check_cancel(command_index++);
            if (c.line_idx < 0 || c.line_idx >= (int) flags.size())
                continue;
            // Attribute the line whose command actually owns a shared-pool
            // node during the high interval. Physical motion execution can
            // begin later and is not the firmware allocation lifetime.
            double start = t0 + c.recv_time;
            double done  = t0 + c.occupied_until;
            if (done < start)
                done = start;
            const auto hit = std::lower_bound(hi.begin(), hi.end(), start, [](const std::pair<double, double>& interval, double time) {
                return interval.second < time;
            });
            if (hit != hi.end() && done >= hit->first)
                flags[c.line_idx] = 1;
        }
    };

    merge_events(st.mcu_events, new_mcu);
    merge_events(st.noz_events, new_noz);
    double                                 mcu_peak_frac = 0.0, noz_peak_frac = 0.0;
    double                                 mcu_peak_time = -1.0, noz_peak_time = -1.0;
    std::vector<std::pair<double, double>> mcu_hi = collect_hi(st.mcu_events, lcfg.mcu_pool_total, &mcu_peak_frac, &mcu_peak_time);
    std::vector<std::pair<double, double>> noz_hi = collect_hi(st.noz_events, lcfg.nozzle_pool_total, &noz_peak_frac, &noz_peak_time);
    mark_layer(real_layer_mcu.steppersync(), mcu_hi);
    mark_layer(real_layer_nozzle.steppersync(), noz_hi);

    if (debug != nullptr) {
        debug->layer_window_start = t0;
        debug->layer_window_end   = t0 + layer_dur;
        debug->mcu_peak_frac      = mcu_peak_frac;
        debug->nozzle_peak_frac   = noz_peak_frac;
        debug->mcu_peak_time      = mcu_peak_time;
        debug->nozzle_peak_time   = noz_peak_time;
        debug->nozzle_late_batch_peak_slots = layer_nozzle_late.peak;
        debug->nozzle_late_batch_peak_frac = lcfg.nozzle_pool_total > 0
            ? (double) layer_nozzle_late.peak
                / (double) lcfg.nozzle_pool_total
            : 0.0;
        debug->nozzle_late_batch_peak_time = layer_nozzle_late.peak_time;
        debug->nozzle_late_batch_first_line = layer_nozzle_late.first_line;
        debug->nozzle_late_batch_last_line = layer_nozzle_late.last_line;
        debug->nozzle_physical_upper_peak_slots =
            layer_nozzle_late.move_commands == 0 ? 0
            : std::min(lcfg.nozzle_pool_total,
                layer_nozzle_late.host_step_peak
                    + std::max(0, lcfg.nozzle_pool_total
                                    - real_layer_nozzle.host_move_slots()));
        debug->nozzle_physical_upper_peak_frac = lcfg.nozzle_pool_total > 0
            ? static_cast<double>(debug->nozzle_physical_upper_peak_slots)
                / static_cast<double>(lcfg.nozzle_pool_total)
            : 0.0;
        debug->mcu_hi_segments    = std::move(mcu_hi);
        debug->nozzle_hi_segments = std::move(noz_hi);
    }

    st.global_time += layer_dur;
    st.current_accel                  = lcfg.limits.max_accel;
    st.requested_accel_to_decel       = requested_accel_to_decel;
    st.current_accel_to_decel         = lcfg.limits.max_accel_to_decel;
    st.current_square_corner_velocity = current_square_corner_velocity;
    st.current_pressure_advance       = current_pressure_advance;
    st.current_pa_smooth_time         = current_pa_smooth_time;
    st.extruder_heater_active         = aux_state.nozzle_heater_active;
    st.bed_heater_active              = aux_state.bed_heater_active;
    st.next_extruder_pwm_time         = st.global_time + aux_state.next_nozzle_pwm_t;
    st.next_bed_pwm_time              = st.global_time + aux_state.next_bed_pwm_t;

    return flags;
}

// ---- region-level occupancy probe (accel-reduction solver support) ----
OccResult simulate_lines_occupancy(const std::vector<std::string>& lines, const SimConfig& cfg, bool e_relative)
{
    OccResult res;

    ParsedGCode parsed = parse_gcode_lines_detailed(
        lines, e_relative, cfg.limits, cfg.pressure_advance,
        cfg.pa_smooth_time, cfg.pressure_advance_commands_enabled);
    if (parsed.moves.empty() && parsed.aux_events.empty())
        return res;

    // Run the full pipeline with cfg.limits as the default motion state while
    // still honoring any in-region SET_VELOCITY_LIMIT / M204 overrides.
    SimReport rep = simulate_full(parsed, cfg);
    res.mcu_frac        = rep.mcu_total > 0 ? double(rep.mcu_peak) / rep.mcu_total : 0.0;
    res.nozzle_frac     = rep.nozzle_total > 0 ? double(rep.nozzle_peak) / rep.nozzle_total : 0.0;
    res.print_time      = rep.print_time_total;
    res.last_positive_extrusion_cruise = rep.last_positive_extrusion_cruise;
    res.mcu_overflow    = rep.mcu_overflow;
    res.nozzle_overflow = rep.nozzle_overflow;
    return res;
}

OccResult simulate_line_refs_occupancy(const std::vector<const std::string*>& lines,
                                       const SimConfig& cfg, bool e_relative)
{
    OccResult res;

    ParsedGCode parsed = parse_gcode_line_refs_detailed(
        lines, e_relative, cfg.limits, cfg.pressure_advance, cfg.pa_smooth_time,
        cfg.pressure_advance_commands_enabled);
    if (parsed.moves.empty() && parsed.aux_events.empty())
        return res;

    SimReport rep = simulate_full(parsed, cfg);
    res.mcu_frac        = rep.mcu_total > 0 ? double(rep.mcu_peak) / rep.mcu_total : 0.0;
    res.nozzle_frac     = rep.nozzle_total > 0 ? double(rep.nozzle_peak) / rep.nozzle_total : 0.0;
    res.print_time      = rep.print_time_total;
    res.last_positive_extrusion_cruise = rep.last_positive_extrusion_cruise;
    res.mcu_overflow    = rep.mcu_overflow;
    res.nozzle_overflow = rep.nozzle_overflow;
    return res;
}

}} // namespace Slic3r::KlipperSim
