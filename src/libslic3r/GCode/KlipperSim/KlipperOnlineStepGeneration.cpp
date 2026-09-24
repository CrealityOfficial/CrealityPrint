#include "KlipperOnlineStepGeneration.hpp"

#include "KlipperExtruderPA.hpp"
#include "KlipperDispatchAnalysis.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>

#if !defined(KLSIM_DISABLE_TBB)
#include <tbb/parallel_invoke.h>
#endif

namespace Slic3r { namespace KlipperSim {

namespace {

double elapsed_ms(const std::chrono::steady_clock::time_point started_at)
{
    return std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - started_at).count();
}

uint32_t max_error_ticks(const SimConfig& cfg) { return static_cast<uint32_t>(std::llround(cfg.max_stepper_error * cfg.mcu_freq)); }

bool xy_active(const TrapMove& move) { return move.axes_r.x != 0.0 || move.axes_r.y != 0.0; }

bool e_active(const TrapMove& move) { return std::fabs(move.start_v) > 1e-12 || std::fabs(move.half_accel) > 1e-12; }

double pa_position(const std::vector<TrapMove>& moves, size_t index, double time)
{
    const TrapMove& move = moves[index];
    if (move.pressure_advance <= 0.0 || move.pa_smooth_time <= 0.0)
        return move.start_pos.x + move.get_distance(time);
    KlipperExtruderPA pressure_advance(move.pressure_advance, move.pa_smooth_time);
    return pressure_advance.calc_position(moves, index, time);
}

} // namespace

KlipperOnlineStepGeneration::KlipperOnlineStepGeneration(const SimConfig& cfg,
                                                         KlipperMCU&      main_mcu,
                                                         KlipperMCU&      nozzle_mcu,
                                                         bool             release_trapq,
                                                         OnlineCommandBatchCb command_batch_cb,
                                                         OnlineBorrowedCommandBatchCb borrowed_batch_cb,
                                                         bool retain_windows)
    : m_cfg(cfg)
    , m_main_mcu(main_mcu)
    , m_nozzle_mcu(nozzle_mcu)
    , m_stepper_a("stepper_a", main_mcu, 1, cfg.xy_step_dist, 1.0, false, max_error_ticks(cfg))
    , m_stepper_b("stepper_b", main_mcu, 2, cfg.xy_step_dist, 1.0, false, max_error_ticks(cfg))
    , m_stepper_e("extruder", nozzle_mcu, 3, cfg.e_step_dist, 1.0, false, max_error_ticks(cfg))
    , m_solver_a(
          [](const TrapMove& move, double time) {
              const Coord coord = move.get_coord(time);
              return coord.x + coord.y;
          },
          cfg.xy_step_dist)
    , m_solver_b(
          [](const TrapMove& move, double time) {
              const Coord coord = move.get_coord(time);
              return coord.x - coord.y;
          },
          cfg.xy_step_dist)
    , m_solver_e([](const TrapMove& move, double time) { return move.start_pos.x + move.get_distance(time); }, cfg.e_step_dist)
    , m_max_pa_half_window(0.5 * std::max(0.0, cfg.pa_smooth_time))
    , m_release_trapq(release_trapq)
    , m_parallel_step_generation(cfg.parallel_step_generation)
    , m_command_batch_cb(std::move(command_batch_cb))
    , m_borrowed_batch_cb(std::move(borrowed_batch_cb))
    , m_retain_windows(retain_windows)
{
#if defined(KLSIM_DISABLE_TBB)
    m_parallel_step_generation = false;
#endif
    m_solver_e.set_active_window(m_max_pa_half_window, m_max_pa_half_window);
}

SimulationControl KlipperOnlineStepGeneration::publish_commands(double host_eventtime)
{
    if (m_borrowed_batch_cb) {
        const SimulationControl control = m_borrowed_batch_cb(
            m_main_commands, m_nozzle_commands, host_eventtime);
        m_main_commands.clear();
        m_nozzle_commands.clear();
        return control;
    }
    if (!m_command_batch_cb)
        return SimulationControl::Continue;
    std::vector<HostDispatchCmd> main_batch = std::move(m_main_commands);
    std::vector<HostDispatchCmd> nozzle_batch = std::move(m_nozzle_commands);
    return m_command_batch_cb(std::move(main_batch),
                              std::move(nozzle_batch),
                              host_eventtime);
}

void KlipperOnlineStepGeneration::generate_to(double flush_time, const KlipperTrapQ& xy_trapq, const KlipperTrapQ& e_trapq)
{
    if (flush_time <= m_last_flush_time)
        return;

    const auto prepare_started_at = std::chrono::steady_clock::now();
    const std::vector<TrapMove>& e_moves = e_trapq.moves();
    const size_t new_xy_phases = xy_trapq.moves().size() > m_known_xy_phases
        ? xy_trapq.moves().size() - m_known_xy_phases : 0;
    const size_t new_e_phases = e_moves.size() > m_known_e_phases
        ? e_moves.size() - m_known_e_phases : 0;
    if (xy_trapq.moves().size() > m_known_xy_phases)
        m_total_xy_phases += xy_trapq.moves().size() - m_known_xy_phases;
    if (e_moves.size() > m_known_e_phases)
        m_total_e_phases += e_moves.size() - m_known_e_phases;
    m_known_xy_phases = xy_trapq.moves().size();
    m_known_e_phases  = e_moves.size();
    for (size_t i = m_scanned_e_phases; i < e_moves.size(); ++i)
        m_max_pa_half_window = std::max(m_max_pa_half_window, 0.5 * std::max(0.0, e_moves[i].pa_smooth_time));
    m_scanned_e_phases = e_moves.size();
    m_solver_e.set_active_window(m_max_pa_half_window, m_max_pa_half_window);
    m_prepare_time_ms += elapsed_ms(prepare_started_at);

    const auto generation_started_at = std::chrono::steady_clock::now();
    auto generate_a = [&] {
        const auto started_at = std::chrono::steady_clock::now();
        m_a_steps += m_solver_a.generate_stepcompress_to(
            xy_trapq, flush_time, xy_active, m_stepper_a.stepqueue(), m_cfg.mcu_freq);
        m_generate_a_time_ms += elapsed_ms(started_at);
    };
    auto generate_b = [&] {
        const auto started_at = std::chrono::steady_clock::now();
        m_b_steps += m_solver_b.generate_stepcompress_to(
            xy_trapq, flush_time, xy_active, m_stepper_b.stepqueue(), m_cfg.mcu_freq);
        m_generate_b_time_ms += elapsed_ms(started_at);
    };
    auto generate_e = [&] {
        const auto started_at = std::chrono::steady_clock::now();
        m_e_steps += m_solver_e.generate_stepcompress_indexed_to(
            e_moves, flush_time, pa_position, e_active, m_stepper_e.stepqueue(),
            m_cfg.mcu_freq);
        m_generate_e_time_ms += elapsed_ms(started_at);
    };
#if !defined(KLSIM_DISABLE_TBB)
    if (m_parallel_step_generation)
        tbb::parallel_invoke(generate_a, generate_b, generate_e);
    else
#endif
    {
        generate_a();
        generate_b();
        generate_e();
    }
    m_generation_wall_time_ms += elapsed_ms(generation_started_at);
#if !defined(KLSIM_DISABLE_TBB)
    const bool used_parallel_generation = m_parallel_step_generation;
#else
    const bool used_parallel_generation = false;
#endif
    m_max_generation_phases = std::max(m_max_generation_phases, new_xy_phases + new_e_phases);
    if (used_parallel_generation) {
        ++m_parallel_generation_invocations;
        m_parallel_phase_total += new_xy_phases + new_e_phases;
    } else {
        ++m_serial_generation_invocations;
        m_serial_phase_total += new_xy_phases + new_e_phases;
    }
    ++m_generation_invocations;
    m_last_flush_time = flush_time;
}

void KlipperOnlineStepGeneration::on_flush(const ToolheadFlushWindow& window, KlipperTrapQ& xy_trapq, KlipperTrapQ& e_trapq)
{
    if (m_stopped)
        return;
    m_finished = false;
    generate_to(window.sg_flush_time, xy_trapq, e_trapq);
    const auto mcu_check_started_at = std::chrono::steady_clock::now();
    m_main_mcu.check_active(window.print_time, window.host_eventtime);
    m_nozzle_mcu.check_active(window.print_time, window.host_eventtime);
    m_mcu_check_time_ms += elapsed_ms(mcu_check_started_at);
    const size_t main_begin = m_main_commands.size();
    const size_t nozzle_begin = m_nozzle_commands.size();
    const auto main_flush_started_at = std::chrono::steady_clock::now();
    m_main_mcu.steppersync().flush_stepqueues(
        m_main_mcu.print_time_to_clock(window.mcu_flush_time), HCS_XY_STEP,
        m_main_commands);
    m_main_flush_time_ms += elapsed_ms(main_flush_started_at);
    const auto nozzle_flush_started_at = std::chrono::steady_clock::now();
    m_nozzle_mcu.steppersync().flush_stepqueues(
        m_nozzle_mcu.print_time_to_clock(window.mcu_flush_time), HCS_E_STEP,
        m_nozzle_commands);
    m_nozzle_flush_time_ms += elapsed_ms(nozzle_flush_started_at);
    for (size_t i = main_begin; i < m_main_commands.size(); ++i)
        m_main_commands[i].enqueue_time = window.host_eventtime;
    for (size_t i = nozzle_begin; i < m_nozzle_commands.size(); ++i)
        m_nozzle_commands[i].enqueue_time = window.host_eventtime;
    m_last_host_eventtime = std::max(m_last_host_eventtime,
                                     window.host_eventtime);
    if (m_retain_windows) {
        m_windows.push_back({window.sg_flush_time, m_a_steps, m_b_steps, m_e_steps,
                             m_stepper_a.stepqueue().command_count(),
                             m_stepper_b.stepqueue().command_count(),
                             m_stepper_e.stepqueue().command_count(),
                             m_main_commands.size(), m_nozzle_commands.size()});
    }

    const auto publish_started_at = std::chrono::steady_clock::now();
    const SimulationControl publish_control = publish_commands(window.host_eventtime);
    m_publish_time_ms += elapsed_ms(publish_started_at);
    if (publish_control == SimulationControl::StopRiskDetected) {
        m_stopped = true;
        return;
    }

    if (!m_release_trapq)
        return;
    const auto release_started_at = std::chrono::steady_clock::now();
    const size_t xy_released      = xy_trapq.release_before(window.free_time);
    const double e_history_cutoff = window.free_time - m_max_pa_half_window;
    const size_t e_released       = e_trapq.release_before(e_history_cutoff);
    m_solver_a.discard_phase_prefix(xy_released);
    m_solver_b.discard_phase_prefix(xy_released);
    m_solver_e.discard_phase_prefix(e_released);
    m_known_xy_phases -= std::min(m_known_xy_phases, xy_released);
    m_known_e_phases -= std::min(m_known_e_phases, e_released);
    m_scanned_e_phases -= std::min(m_scanned_e_phases, e_released);
    m_trapq_release_time_ms += elapsed_ms(release_started_at);
}

void KlipperOnlineStepGeneration::finish(double last_kin_move_time, const KlipperTrapQ& xy_trapq, const KlipperTrapQ& e_trapq)
{
    if (m_finished || m_stopped)
        return;
    generate_to(last_kin_move_time, xy_trapq, e_trapq);
    m_stepper_a.stepqueue().finish();
    m_stepper_b.stepqueue().finish();
    m_stepper_e.stepqueue().finish();
    const size_t main_begin = m_main_commands.size();
    const size_t nozzle_begin = m_nozzle_commands.size();
    m_main_mcu.steppersync().flush_stepqueues(
        std::numeric_limits<uint64_t>::max(), HCS_XY_STEP, m_main_commands);
    m_nozzle_mcu.steppersync().flush_stepqueues(
        std::numeric_limits<uint64_t>::max(), HCS_E_STEP, m_nozzle_commands);
    for (size_t i = main_begin; i < m_main_commands.size(); ++i)
        m_main_commands[i].enqueue_time = m_last_host_eventtime;
    for (size_t i = nozzle_begin; i < m_nozzle_commands.size(); ++i)
        m_nozzle_commands[i].enqueue_time = m_last_host_eventtime;
    annotate_stepcompress_commands(m_stepper_a.stepqueue(), xy_trapq.moves(), xy_active, m_cfg.mcu_freq);
    annotate_stepcompress_commands(m_stepper_b.stepqueue(), xy_trapq.moves(), xy_active, m_cfg.mcu_freq);
    annotate_stepcompress_commands(m_stepper_e.stepqueue(), e_trapq.moves(), e_active, m_cfg.mcu_freq);
    if (publish_commands(m_last_host_eventtime)
        == SimulationControl::StopRiskDetected)
        m_stopped = true;
    m_finished = true;
}

}} // namespace Slic3r::KlipperSim
