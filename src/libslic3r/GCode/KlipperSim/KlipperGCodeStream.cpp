#include "KlipperGCodeStream.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string_view>

namespace Slic3r {
namespace KlipperSim {

namespace {

char upper_ascii(char c)
{
    return c >= 'a' && c <= 'z' ? static_cast<char>(c - ('a' - 'A')) : c;
}

bool starts_command(const std::string& line, size_t start, std::string_view command)
{
    if (start + command.size() > line.size())
        return false;
    for (size_t i = 0; i < command.size(); ++i) {
        if (upper_ascii(line[start + i]) != upper_ascii(command[i]))
            return false;
    }
    const size_t end = start + command.size();
    return end == line.size() || line[end] == ' ' || line[end] == '\t'
        || line[end] == ';' || line[end] == '\r' || line[end] == '\n';
}

bool parse_word_value(const std::string& line, char key, double& value)
{
    key = upper_ascii(key);
    for (size_t i = 0; i < line.size(); ++i) {
        if (upper_ascii(line[i]) != key
            || (i != 0 && line[i - 1] != ' ' && line[i - 1] != '\t'))
            continue;
        char* end = nullptr;
        const double parsed = std::strtod(line.c_str() + i + 1, &end);
        if (end != line.c_str() + i + 1) {
            value = parsed;
            return true;
        }
    }
    return false;
}

bool parse_named_value(const std::string& line, size_t start,
                       std::string_view key, double& value)
{
    for (size_t i = start; i + key.size() < line.size(); ++i) {
        if (i > start && line[i - 1] != ' ' && line[i - 1] != '\t')
            continue;
        bool match = true;
        for (size_t j = 0; j < key.size(); ++j) {
            if (upper_ascii(line[i + j]) != upper_ascii(key[j])) {
                match = false;
                break;
            }
        }
        if (!match)
            continue;
        char* end = nullptr;
        const double parsed = std::strtod(line.c_str() + i + key.size(), &end);
        if (end != line.c_str() + i + key.size()) {
            value = parsed;
            return true;
        }
    }
    return false;
}

double calc_junction_deviation(double scv, double accel)
{
    return accel > 0.0
        ? scv * scv * (std::sqrt(2.0) - 1.0) / accel
        : 0.0;
}

} // namespace

KlipperGCodeStream::KlipperGCodeStream(const SimConfig& cfg,
                                       LineAnalysisState& state,
                                       GCodeStreamCallbacks callbacks)
    : m_cfg(cfg), m_state(state), m_callbacks(std::move(callbacks))
{
    if (m_cfg.microsegment_batching && m_callbacks.on_move_batch) {
        m_move_batch_size = std::clamp<size_t>(m_cfg.microsegment_batch_size,
                                               1, 64);
        m_move_batch.reserve(m_move_batch_size);
    }
    if (!m_state.simulation_config_initialized) {
        m_state.current_pressure_advance = m_cfg.pressure_advance;
        m_state.current_pa_smooth_time = m_cfg.pa_smooth_time;
        m_state.pressure_advance_enabled = m_cfg.pressure_advance_commands_enabled;
        m_state.simulation_config_initialized = true;
    }
    m_cfg.limits.max_accel = state.current_accel;
    m_cfg.limits.max_accel_to_decel = std::min(
        state.requested_accel_to_decel, state.current_accel);
    m_cfg.limits.square_corner_velocity = state.current_square_corner_velocity;
    m_cfg.limits.junction_deviation = calc_junction_deviation(
        state.current_square_corner_velocity, state.current_accel);
}

bool KlipperGCodeStream::flush_move_batch(bool forced)
{
    if (m_move_batch.empty() || m_stopped)
        return !m_stopped;
    if (forced)
        ++m_stats.forced_batch_flushes;
    ++m_stats.move_batches;
    m_stats.batched_moves += m_move_batch.size();
    m_stats.max_moves_per_batch = std::max(
        m_stats.max_moves_per_batch, m_move_batch.size());
    const bool keep_going = m_callbacks.on_move_batch(m_move_batch);
    m_move_batch.clear();
    m_stopped = !keep_going;
    return keep_going;
}

bool KlipperGCodeStream::flush_pending_moves()
{
    return flush_move_batch(true);
}

bool KlipperGCodeStream::emit_move(const GMove& move)
{
    ++m_stats.moves;
    if (m_stopped)
        return false;
    if (m_move_batch_size == 0) {
        if (m_callbacks.on_move)
            m_callbacks.on_move(move);
        return true;
    }
    m_move_batch.push_back(move);
    return m_move_batch.size() < m_move_batch_size
        || flush_move_batch(false);
}

void KlipperGCodeStream::consume_line(const std::string& line, int line_idx)
{
    if (m_stopped)
        return;
    ++m_stats.lines;
    size_t s = 0;
    while (s < line.size() && (line[s] == ' ' || line[s] == '\t'))
        ++s;
    if (s >= line.size() || line[s] == ';' || line[s] == '\r' || line[s] == '\n') {
        ++m_stats.ignored;
        return;
    }

    const bool line_is_move = starts_command(line, s, "G0")
        || starts_command(line, s, "G1")
        || starts_command(line, s, "G2")
        || starts_command(line, s, "G3");
    if (!line_is_move && !flush_move_batch(true))
        return;

    auto emit_barrier = [&](SimulationBarrierKind kind, double dwell,
                            bool invalidates_position,
                            double step_generation_scan_time = 0.0) {
        // A PA scan-time change flushes queued step generation but does not
        // wait for motion, so keep it out of the blocking-barrier statistic.
        if (kind != SimulationBarrierKind::StepGenerationFlush)
            ++m_stats.barriers;
        if (m_callbacks.on_barrier)
            m_callbacks.on_barrier({kind, line_idx, dwell,
                                    step_generation_scan_time,
                                    invalidates_position});
    };
    auto emit_aux = [&](GCodeAuxKind kind, bool active, double value) {
        ++m_stats.aux_events;
        if (m_callbacks.on_aux)
            m_callbacks.on_aux({kind, line_idx, active, value});
    };

    if (starts_command(line, s, "SET_VELOCITY_LIMIT")) {
        double accel = m_cfg.limits.max_accel;
        double requested_atd = m_state.requested_accel_to_decel;
        double scv = m_state.current_square_corner_velocity;
        parse_named_value(line, s, "ACCEL=", accel);
        parse_named_value(line, s, "ACCEL_TO_DECEL=", requested_atd);
        parse_named_value(line, s, "SQUARE_CORNER_VELOCITY=", scv);
        if (accel > 0.0)
            m_cfg.limits.max_accel = accel;
        if (requested_atd > 0.0)
            m_state.requested_accel_to_decel = requested_atd;
        if (scv >= 0.0)
            m_state.current_square_corner_velocity = scv;
        m_cfg.limits.max_accel_to_decel = std::min(
            m_state.requested_accel_to_decel, m_cfg.limits.max_accel);
        m_cfg.limits.square_corner_velocity = m_state.current_square_corner_velocity;
        m_cfg.limits.junction_deviation = calc_junction_deviation(
            m_cfg.limits.square_corner_velocity, m_cfg.limits.max_accel);
        m_state.current_accel = m_cfg.limits.max_accel;
        m_state.current_accel_to_decel = m_cfg.limits.max_accel_to_decel;
        return;
    }

    if (starts_command(line, s, "ENABLE_PRESSURE_ADVANCE")) {
        double enabled = m_state.pressure_advance_enabled ? 1.0 : 0.0;
        if (parse_named_value(line, s, "VALUE=", enabled))
            m_state.pressure_advance_enabled = enabled >= 0.5;
        return;
    }

    if (starts_command(line, s, "SET_PRESSURE_ADVANCE")) {
        // Creality extruder.py keeps the current PA value while disabled and
        // ignores SET_PRESSURE_ADVANCE until it is enabled again.
        if (!m_state.pressure_advance_enabled)
            return;
        double pa = m_state.current_pressure_advance;
        double smooth = m_state.current_pa_smooth_time;
        parse_named_value(line, s, "ADVANCE=", pa);
        parse_named_value(line, s, "SMOOTH_TIME=", smooth);
        // ExtruderStepper._set_pressure_advance() flushes step generation
        // before installing the new PA and smoothing window.
        emit_barrier(SimulationBarrierKind::StepGenerationFlush, 0.0, false,
                     pa > 0.0 ? smooth * 0.5 : 0.0);
        if (pa >= 0.0)
            m_state.current_pressure_advance = pa;
        if (smooth >= 0.0)
            m_state.current_pa_smooth_time = smooth;
        return;
    }

    if (starts_command(line, s, "M400")) {
        emit_barrier(SimulationBarrierKind::Synchronize, 0.0, false);
        return;
    }
    if (starts_command(line, s, "G4")) {
        double p = 0.0, seconds = 0.0;
        if (parse_word_value(line, 'P', p) && p > 0.0)
            seconds = p * 0.001;
        parse_word_value(line, 'S', seconds);
        emit_barrier(SimulationBarrierKind::Dwell, std::max(0.0, seconds), false);
        return;
    }
    if (starts_command(line, s, "G28")) {
        emit_barrier(SimulationBarrierKind::Homing, 0.0, true);
        m_state.have_pos = false;
        m_state.x_offset = m_state.y_offset = m_state.z_offset = 0.0;
        return;
    }

    if (starts_command(line, s, "M106")) {
        double raw = 255.0;
        parse_word_value(line, 'S', raw);
        const double value = std::max(0.0, std::min(1.0, raw / 255.0));
        emit_aux(GCodeAuxKind::NozzleFan, value > 0.0, value);
        return;
    }
    if (starts_command(line, s, "M107")) {
        emit_aux(GCodeAuxKind::NozzleFan, false, 0.0);
        return;
    }
    if (starts_command(line, s, "M104") || starts_command(line, s, "M109")) {
        double target = -1.0;
        if (parse_word_value(line, 'S', target))
            emit_aux(GCodeAuxKind::NozzleHeater, target > 0.0, target);
        if (starts_command(line, s, "M109"))
            emit_barrier(SimulationBarrierKind::TemperatureWait, 0.0, false);
        return;
    }
    if (starts_command(line, s, "M140") || starts_command(line, s, "M190")) {
        double target = -1.0;
        if (parse_word_value(line, 'S', target))
            emit_aux(GCodeAuxKind::BedHeater, target > 0.0, target);
        if (starts_command(line, s, "M190"))
            emit_barrier(SimulationBarrierKind::TemperatureWait, 0.0, false);
        return;
    }

    if (starts_command(line, s, "M82")) {
        m_state.e_relative = false;
        return;
    }
    if (starts_command(line, s, "M83")) {
        m_state.e_relative = true;
        return;
    }
    if (starts_command(line, s, "M204")) {
        double accel = -1.0, p = -1.0, t = -1.0, value = -1.0;
        if (parse_word_value(line, 'S', value)) {
            const int rounded = static_cast<int>(std::llround(value));
            accel = rounded != -1 && rounded <= 100 ? 100.0 : value;
        } else if (parse_word_value(line, 'P', p)
                   && parse_word_value(line, 'T', t)) {
            accel = std::min(p, t);
        }
        if (accel > 0.0) {
            m_cfg.limits.max_accel = accel;
            m_cfg.limits.max_accel_to_decel = std::min(
                m_state.requested_accel_to_decel, accel);
            m_cfg.limits.junction_deviation = calc_junction_deviation(
                m_cfg.limits.square_corner_velocity, accel);
            m_state.current_accel = accel;
            m_state.current_accel_to_decel = m_cfg.limits.max_accel_to_decel;
        }
        return;
    }

    if (starts_command(line, s, "G17")) {
        m_state.arc_plane = ArcPlane::XY;
        return;
    }
    if (starts_command(line, s, "G18")) {
        m_state.arc_plane = ArcPlane::XZ;
        return;
    }
    if (starts_command(line, s, "G19")) {
        m_state.arc_plane = ArcPlane::YZ;
        return;
    }
    if (starts_command(line, s, "G90")) {
        m_state.xyz_relative = false;
        return;
    }
    if (starts_command(line, s, "G91")) {
        m_state.xyz_relative = true;
        return;
    }
    if (starts_command(line, s, "G92")) {
        const bool was_positioned = m_state.have_pos;
        double x = m_state.x, y = m_state.y, z = m_state.z, e = m_state.e_abs;
        if (parse_word_value(line, 'X', x))
            m_state.x_offset += m_state.x - x;
        if (parse_word_value(line, 'Y', y))
            m_state.y_offset += m_state.y - y;
        if (parse_word_value(line, 'Z', z))
            m_state.z_offset += m_state.z - z;
        parse_word_value(line, 'E', e);
        m_state.x = x;
        m_state.y = y;
        m_state.z = z;
        m_state.e_abs = e;
        m_state.have_pos = true;
        if (!was_positioned && m_callbacks.on_position) {
            m_callbacks.on_position({m_state.x + m_state.x_offset,
                                     m_state.y + m_state.y_offset,
                                     m_state.z + m_state.z_offset, 0.0}, true);
        }
        return;
    }

    const bool is_g0 = starts_command(line, s, "G0");
    const bool is_g1 = starts_command(line, s, "G1");
    const bool is_g2 = starts_command(line, s, "G2");
    const bool is_g3 = starts_command(line, s, "G3");
    if (is_g2 || is_g3) {
        double offset_first = 0.0, offset_second = 0.0, radius = 0.0;
        const char first_word = m_state.arc_plane == ArcPlane::XY ? 'I'
            : m_state.arc_plane == ArcPlane::XZ ? 'I' : 'J';
        const char second_word = m_state.arc_plane == ArcPlane::XY ? 'J' : 'K';
        const bool has_first = parse_word_value(line, first_word, offset_first);
        const bool has_second = parse_word_value(line, second_word, offset_second);
        if (m_state.xyz_relative || parse_word_value(line, 'R', radius)
            || !(has_first || has_second) || !m_state.have_pos) {
            ++m_stats.ignored;
            return;
        }
        double xword = m_state.x, yword = m_state.y, zword = m_state.z;
        double eword = 0.0, feed = 0.0;
        parse_word_value(line, 'X', xword);
        parse_word_value(line, 'Y', yword);
        parse_word_value(line, 'Z', zword);
        const bool has_e = parse_word_value(line, 'E', eword);
        if (parse_word_value(line, 'F', feed) && feed > 0.0)
            m_state.feed = feed / 60.0;
        const auto coords = expand_arc_moves({m_state.x, m_state.y, m_state.z},
                                             {xword, yword, zword}, offset_first,
                                             offset_second, is_g2, m_state.arc_plane);
        const double total_de = !has_e ? 0.0 : m_state.e_relative ? eword : eword - m_state.e_abs;
        const double de_per_move = total_de / coords.size();
        for (const auto& coord : coords) {
            const GMove move{coord[0] + m_state.x_offset,
                             coord[1] + m_state.y_offset,
                             coord[2] + m_state.z_offset,
                             de_per_move, m_state.feed,
                             m_cfg.limits.max_accel,
                             m_cfg.limits.max_accel_to_decel,
                             m_cfg.limits.square_corner_velocity,
                             m_state.current_pressure_advance,
                             m_state.current_pa_smooth_time,
                             line_idx};
            if (!emit_move(move))
                return;
        }
        if (has_e && !m_state.e_relative)
            m_state.e_abs = eword;
        m_state.x = xword;
        m_state.y = yword;
        m_state.z = zword;
        return;
    }
    if (is_g0 || is_g1) {
        double xword = 0.0, yword = 0.0, zword = 0.0, eword = 0.0, feed = 0.0;
        const bool has_x = parse_word_value(line, 'X', xword);
        const bool has_y = parse_word_value(line, 'Y', yword);
        const bool has_z = parse_word_value(line, 'Z', zword);
        const bool has_e = parse_word_value(line, 'E', eword);
        if (parse_word_value(line, 'F', feed) && feed > 0.0)
            m_state.feed = feed / 60.0;

        const double nx = has_x
            ? (m_state.xyz_relative ? m_state.x + xword : xword) : m_state.x;
        const double ny = has_y
            ? (m_state.xyz_relative ? m_state.y + yword : yword) : m_state.y;
        const double nz = has_z
            ? (m_state.xyz_relative ? m_state.z + zword : zword) : m_state.z;
        double de = 0.0;
        if (has_e) {
            if (m_state.e_relative)
                de = eword;
            else {
                de = eword - m_state.e_abs;
                m_state.e_abs = eword;
            }
        }

        if (!m_state.have_pos) {
            m_state.x = nx;
            m_state.y = ny;
            m_state.z = nz;
            m_state.have_pos = true;
            if (m_callbacks.on_position)
                m_callbacks.on_position({nx + m_state.x_offset,
                                         ny + m_state.y_offset,
                                         nz + m_state.z_offset, 0.0}, true);
            return;
        }
        if (nx == m_state.x && ny == m_state.y && nz == m_state.z && de == 0.0)
            return;

        GMove move{nx + m_state.x_offset,
                   ny + m_state.y_offset,
                   nz + m_state.z_offset, de, m_state.feed,
                   m_cfg.limits.max_accel,
                   m_cfg.limits.max_accel_to_decel,
                   m_cfg.limits.square_corner_velocity,
                   m_state.current_pressure_advance,
                   m_state.current_pa_smooth_time,
                   line_idx};
        if (!emit_move(move))
            return;
        m_state.x = nx;
        m_state.y = ny;
        m_state.z = nz;
        return;
    }

    const bool tool_change = upper_ascii(line[s]) == 'T'
        && s + 1 < line.size() && line[s + 1] >= '0' && line[s + 1] <= '9';
    const bool changes_tool = tool_change && line[s + 1] != '0';
    const bool activates_other_extruder = starts_command(line, s, "ACTIVATE_EXTRUDER")
        && line.find("EXTRUDER=extruder", s) == std::string::npos;
    if (starts_command(line, s, "PAUSE") || starts_command(line, s, "M600")
        || activates_other_extruder || changes_tool) {
        emit_barrier(SimulationBarrierKind::PauseOrToolChange, 0.0, false);
        return;
    }
    if (tool_change || starts_command(line, s, "ACTIVATE_EXTRUDER"))
        return;

    // Common metadata commands do not alter motion continuity.
    if (starts_command(line, s, "M73") || starts_command(line, s, "M117")
        || starts_command(line, s, "M118")
        || starts_command(line, s, "SET_PRINT_STATS_INFO")
        || starts_command(line, s, "EXCLUDE_OBJECT_DEFINE")
        || starts_command(line, s, "EXCLUDE_OBJECT_START")
        || starts_command(line, s, "EXCLUDE_OBJECT_END")) {
        ++m_stats.ignored;
        return;
    }

    // Unknown named commands may be macros with hidden wait_moves()/motion.
    // Treat those as a hard boundary instead of silently joining both sides.
    size_t token_end = s;
    while (token_end < line.size() && line[token_end] != ' '
           && line[token_end] != '\t' && line[token_end] != ';'
           && line[token_end] != '\r' && line[token_end] != '\n')
        ++token_end;
    if (token_end > s
        && std::string_view(line.data() + s, token_end - s).find('_')
           != std::string_view::npos) {
        emit_barrier(SimulationBarrierKind::UnknownExecutable, 0.0, false);
        return;
    }

    ++m_stats.ignored;
}

}} // namespace Slic3r::KlipperSim
