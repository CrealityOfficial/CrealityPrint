#include "KlipperStreamingEarlyProbe.hpp"

#include "KlipperGCodeStream.hpp"
#include "KlipperHostDispatch.hpp"
#include "KlipperOnlineStepGeneration.hpp"
#include "KlipperSimulationSession.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <deque>
#include <limits>
#include <memory>
#include <queue>
#include <stdexcept>

namespace Slic3r { namespace KlipperSim {
namespace {

double elapsed_ms(const std::chrono::steady_clock::time_point started_at)
{
    return std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - started_at).count();
}


HostDispatchConfig dispatch_config(const SimConfig& config, bool nozzle)
{
    HostDispatchConfig result;
    result.transport = HostTransportKind::Uart;
    result.wire_frequency = nozzle ? config.nozzle_baud : config.mcu_baud;
    result.clock_frequency = config.mcu_freq;
    result.receive_window = nozzle ? config.nozzle_receive_window
                                   : config.mcu_receive_window;
    return result;
}

double junction_deviation(double square_corner_velocity, double max_accel)
{
    return max_accel > 0.0
        ? square_corner_velocity * square_corner_velocity
            * (std::sqrt(2.0) - 1.0) / max_accel
        : 0.0;
}

struct Candidate {
    bool hit = false;
    EarlyRiskSource source = EarlyRiskSource::None;
    int occupied = 0;
    int total = 0;
    int line_idx = -1;
    double time = -1.0;
};

} // namespace

void PathologicalMotionHistory::append_planned_batch(
    const std::vector<PlanMove>& batch, double batch_start)
{
    static constexpr double spatial_epsilon = 1.0e-12;
    static constexpr double micro_length = 0.09;
    static constexpr double window_seconds = 2.5;
    static constexpr size_t required_consecutive = 100;

    double move_start = batch_start;
    for (const PlanMove& move : batch) {
        const double move_end = move_start + move.accel_t
                              + move.cruise_t + move.decel_t;
        const double spatial_length = std::sqrt(
            move.axes_d[0] * move.axes_d[0]
            + move.axes_d[1] * move.axes_d[1]
            + move.axes_d[2] * move.axes_d[2]);
        if (spatial_length > spatial_epsilon) {
            if (spatial_length <= micro_length + spatial_epsilon) {
                m_trailing_micro_moves.push_back({move_start, move_end});
                if (m_trailing_micro_moves.size() > required_consecutive)
                    m_trailing_micro_moves.pop_front();
                if (m_trailing_micro_moves.size() == required_consecutive) {
                    const double interval_start = move_start;
                    const double interval_end =
                        m_trailing_micro_moves.front().end_time + window_seconds;
                    if (interval_start <= interval_end + spatial_epsilon) {
                        if (!m_hit_intervals.empty()
                            && interval_start <= m_hit_intervals.back().end_time
                                                   + spatial_epsilon) {
                            m_hit_intervals.back().end_time = std::max(
                                m_hit_intervals.back().end_time, interval_end);
                        } else {
                            m_hit_intervals.push_back({interval_start,
                                                       interval_end});
                        }
                    }
                }
            } else {
                m_trailing_micro_moves.clear();
            }
        }
        move_start = move_end;
    }
}

bool PathologicalMotionHistory::has_microsequence(double hit_time) const
{
    static constexpr double epsilon = 1.0e-12;
    const auto it = std::upper_bound(
        m_hit_intervals.begin(), m_hit_intervals.end(), hit_time,
        [](double time, const HitInterval& interval) {
            return time < interval.start_time;
        });
    if (it == m_hit_intervals.begin())
        return false;
    const HitInterval& interval = *std::prev(it);
    return hit_time <= interval.end_time + epsilon;
}

struct StreamingEarlyProbeSession::Impl {
    Impl(const SimConfig& config, double threshold,
         const LineAnalysisState& initial_state,
         std::function<void()> cancel)
        : local(config), threshold(threshold), cancel(std::move(cancel)),
          state(initial_state)
    {
        result.threshold = threshold;
        result.streaming_fast_path = true;
        if (state.global_time != 0.0 || !state.mcu_events.empty()
            || !state.noz_events.empty()) {
            fallback = true;
            return;
        }

        local.limits.max_accel = state.current_accel;
        local.limits.max_accel_to_decel = std::min(
            state.requested_accel_to_decel, local.limits.max_accel);
        local.limits.square_corner_velocity = state.current_square_corner_velocity;
        local.limits.junction_deviation = junction_deviation(
            local.limits.square_corner_velocity, local.limits.max_accel);

        main_mcu = std::make_unique<KlipperMCU>(
            "mcu", local.mcu_total, local.buffer_time_high, local.mcu_freq,
            0.0, local.mcu_reserved_slots, local.mcu_pool_total);
        nozzle_mcu = std::make_unique<KlipperMCU>(
            "nozzle_mcu", local.nozzle_total, local.buffer_time_high,
            std::make_unique<KlipperSecondarySync>(main_mcu->clocksync(),
                                                   local.mcu_freq),
            0.0, local.nozzle_reserved_slots, local.nozzle_pool_total);
        main_pool = std::make_unique<McuMovePool>(local.mcu_pool_total);
        nozzle_pool = std::make_unique<McuMovePool>(local.nozzle_pool_total);
        main_pool->set_baseline_frac(local.mcu_baseline_frac);
        nozzle_pool->set_baseline_frac(local.nozzle_baseline_frac);
        main_pool->set_compact_timeline(0);
        nozzle_pool->set_compact_timeline(0);
        main_threshold = static_cast<int>(local.mcu_pool_total * threshold);
        nozzle_threshold = static_cast<int>(local.nozzle_pool_total * threshold);

        main_serial = std::make_unique<DeterministicSerialQueueSession>(
            dispatch_config(local, false),
            [this](double eventtime) {
                return main_mcu->estimated_print_time(eventtime);
            },
            [this](const HostDispatchCmd& command) {
                return receive(*main_pool, main_threshold,
                               EarlyRiskSource::MainMcu, main_candidate,
                               command);
            }, false);
        nozzle_serial = std::make_unique<DeterministicSerialQueueSession>(
            dispatch_config(local, true),
            [this](double eventtime) {
                return nozzle_mcu->estimated_print_time(eventtime);
            },
            [this](const HostDispatchCmd& command) {
                return receive(*nozzle_pool, nozzle_threshold,
                               EarlyRiskSource::NozzleMcu, nozzle_candidate,
                               command);
            }, false);

        online_steps = std::make_unique<KlipperOnlineStepGeneration>(
            local, *main_mcu, *nozzle_mcu, true, OnlineCommandBatchCb{},
            [this](std::vector<HostDispatchCmd>& main_commands,
                   std::vector<HostDispatchCmd>& nozzle_commands,
                   double host_eventtime) {
                return publish(main_commands, nozzle_commands, host_eventtime);
            }, false);
        const std::array<double, 4> start_position{
            state.x + state.x_offset, state.y + state.y_offset,
            state.z + state.z_offset, state.e_abs};
        simulation = std::make_unique<KlipperSimulationSession>(
            local, start_position, state.have_pos, 0,
            [this](double eventtime) {
                return main_mcu->estimated_print_time(eventtime);
            }, ToolheadFlushCb{},
            [this](const ToolheadFlushWindow& window, KlipperTrapQ& xy,
                   KlipperTrapQ& e) {
                online_steps->on_flush(window, xy, e);
            }, false, nullptr, nullptr, nullptr,
            [this](const std::vector<PlanMove>& batch, double batch_start) {
                resolve_aux_for_batch(batch, batch_start);
            });

        GCodeStreamCallbacks callbacks;
        auto consume_moves = [this](const GMove* moves, size_t count) {
            const double step_started_ms = step_pipeline_time_ms();
            const double aux_started_ms = aux_resolve_time_ms;
            const auto started_at = std::chrono::steady_clock::now();
            size_t consumed = 0;
            for (; consumed < count && !online_steps->stopped(); ++consumed)
                simulation->consume_move(moves[consumed]);
            const double callback_ms = elapsed_ms(started_at);
            move_callback_time_ms += callback_ms;
            toolhead_time_ms += std::max(0.0, callback_ms
                - (step_pipeline_time_ms() - step_started_ms)
                - (aux_resolve_time_ms - aux_started_ms));
            return consumed == count && !online_steps->stopped();
        };
        callbacks.on_move = [consume_moves](const GMove& move) {
            (void) consume_moves(&move, 1);
        };
        if (local.microsegment_batching) {
            callbacks.on_move_batch = [consume_moves](
                const std::vector<GMove>& moves) {
                return consume_moves(moves.data(), moves.size());
            };
        }
        callbacks.on_barrier = [this](const SimulationBarrier& barrier) {
            const double step_started_ms = step_pipeline_time_ms();
            const double aux_started_ms = aux_resolve_time_ms;
            const auto started_at = std::chrono::steady_clock::now();
            simulation->consume_barrier(barrier);
            const double callback_ms = elapsed_ms(started_at);
            barrier_callback_time_ms += callback_ms;
            barrier_time_ms += std::max(0.0, callback_ms
                - (step_pipeline_time_ms() - step_started_ms)
                - (aux_resolve_time_ms - aux_started_ms));
        };
        callbacks.on_position = [this](const std::array<double, 4>& position,
                                       bool valid) {
            const auto started_at = std::chrono::steady_clock::now();
            simulation->set_position(position, valid);
            position_time_ms += elapsed_ms(started_at);
        };
        callbacks.on_aux = [this](const GCodeAuxEvent& event) {
            const auto started_at = std::chrono::steady_clock::now();
            unresolved_aux.push_back(event);
            aux_event_time_ms += elapsed_ms(started_at);
        };
        stream = std::make_unique<KlipperGCodeStream>(
            local, state, std::move(callbacks));
    }

    SimulationControl receive(McuMovePool& pool, int threshold_slots,
                              EarlyRiskSource source, Candidate& candidate,
                              const HostDispatchCmd& command)
    {
        if (cancel && ((receive_counter++ & 0x3ffu) == 0))
            cancel();
        if (!command.uses_move_slot)
            return SimulationControl::Continue;
        const double recv_time = std::max(command.recv_time, command.min_time);
        pool.alloc(recv_time, std::max(recv_time, command.slot_free_time),
                   command.line_idx, command.source_kind);
        const int occupied = pool.current_modeled_occupancy();
        const double hit_time = std::max(0.0, command.exec_start);
        if (!candidate.hit && threshold_slots > 0
            && occupied >= threshold_slots
            && pathological_motion_history.has_microsequence(hit_time)) {
            candidate = {true, source, occupied, pool.total(),
                         command.line_idx, recv_time};
            return SimulationControl::StopRiskDetected;
        }
        return SimulationControl::Continue;
    }

    SimulationControl advance_pending()
    {
        const auto started_at = std::chrono::steady_clock::now();
        const SimulationControl main_control = main_serial->command_count() > 0
            ? main_serial->advance_to(pending_host_eventtime)
            : SimulationControl::Continue;
        const SimulationControl nozzle_control = nozzle_serial->command_count() > 0
            ? nozzle_serial->advance_to(pending_host_eventtime)
            : SimulationControl::Continue;
        serial_dispatch_time_ms += elapsed_ms(started_at);
        return main_control == SimulationControl::StopRiskDetected
                || nozzle_control == SimulationControl::StopRiskDetected
            ? SimulationControl::StopRiskDetected
            : SimulationControl::Continue;
    }

    SimulationControl publish(std::vector<HostDispatchCmd>& main_commands,
                              std::vector<HostDispatchCmd>& nozzle_commands,
                              double host_eventtime)
    {
        if (have_pending_host
            && host_eventtime > pending_host_eventtime + 1.0e-12
            && advance_pending() == SimulationControl::StopRiskDetected)
            return SimulationControl::StopRiskDetected;
        pending_host_eventtime = host_eventtime;
        have_pending_host = true;
        const auto append_started_at = std::chrono::steady_clock::now();
        for (HostDispatchCmd& command : main_commands)
            main_serial->append(std::move(command));
        for (HostDispatchCmd& command : pending_main_aux) {
            command.enqueue_time = host_eventtime;
            main_serial->append(std::move(command));
        }
        pending_main_aux.clear();
        for (HostDispatchCmd& command : nozzle_commands)
            nozzle_serial->append(std::move(command));
        for (HostDispatchCmd& command : pending_nozzle_aux) {
            command.enqueue_time = host_eventtime;
            nozzle_serial->append(std::move(command));
        }
        pending_nozzle_aux.clear();
        serial_dispatch_time_ms += elapsed_ms(append_started_at);
        return SimulationControl::Continue;
    }

    static int encoded_aux_int_len(uint32_t value)
    {
        const int32_t signed_value = static_cast<int32_t>(value);
        if (signed_value < (3L << 5) && signed_value >= -(1L << 5)) return 1;
        if (signed_value < (3L << 12) && signed_value >= -(1L << 12)) return 2;
        if (signed_value < (3L << 19) && signed_value >= -(1L << 19)) return 3;
        if (signed_value < (3L << 26) && signed_value >= -(1L << 26)) return 4;
        return 5;
    }

    void add_aux_command(std::vector<HostDispatchCmd>& destination,
                         double exec_time, int line_idx, int source_kind,
                         double value = 0.0)
    {
        static constexpr double fan_slot_duration = 0.020;
        static constexpr double aux_slot_duration = 0.001;
        const double advance_window = static_cast<double>(1ULL << 31)
                                    / local.mcu_freq;
        const double min_time = std::max(
            0.0, std::max(exec_time - local.buffer_time_high,
                          exec_time - advance_window));
        HostDispatchCmd command;
        command.batch_start_time = min_time;
        command.ready_time = min_time;
        command.min_time = min_time;
        command.req_time = exec_time;
        command.slot_free_time = exec_time
            + (source_kind == HCS_AUX_FAN ? fan_slot_duration
                                          : aux_slot_duration);
        command.exec_start = command.exec_end = exec_time;
        command.recv_time = min_time;
        command.line_idx = line_idx;
        command.source_kind = source_kind;
        command.uses_move_slot = true;
        command.background_priority = false;
        const uint32_t clock = static_cast<uint32_t>(std::llround(
            std::max(0.0, exec_time * local.mcu_freq)));
        const uint32_t encoded_value = static_cast<uint32_t>(
            std::llround(value));
        command.msg_len = 2 + encoded_aux_int_len(clock)
                            + encoded_aux_int_len(encoded_value);
        command.command_queue_id = source_kind == HCS_AUX_FAN ? 10
            : source_kind == HCS_AUX_HEATER ? 11 : 12;
        destination.push_back(command);
    }

    void emit_heater_until(double end_time)
    {
        static constexpr double heater_pwm_period = 0.75 * 5.0;
        auto emit = [&](std::vector<HostDispatchCmd>& destination,
                        bool active, double& next_time, int source_kind) {
            if (next_time < aux_segment_start)
                next_time = aux_segment_start;
            if (!active)
                return;
            while (next_time <= end_time + 1.0e-12) {
                add_aux_command(destination, next_time, -1, source_kind);
                next_time += heater_pwm_period;
            }
        };
        emit(pending_nozzle_aux, nozzle_heater_active,
             next_nozzle_pwm_time, HCS_AUX_HEATER);
        emit(pending_main_aux, bed_heater_active,
             next_bed_pwm_time, HCS_AUX_BED);
    }

    void resolve_aux_event(const GCodeAuxEvent& event, double exec_time)
    {
        emit_heater_until(exec_time);
        aux_segment_start = exec_time;
        switch (event.kind) {
        case GCodeAuxKind::NozzleFan:
            add_aux_command(pending_nozzle_aux, exec_time, event.line_idx,
                            HCS_AUX_FAN, event.value * 255.0);
            if (event.active && event.value > 0.0 && event.value < 1.0)
                add_aux_command(pending_nozzle_aux, exec_time + 0.100,
                                event.line_idx, HCS_AUX_FAN,
                                event.value * 255.0);
            break;
        case GCodeAuxKind::NozzleHeater:
            nozzle_heater_active = event.active;
            break;
        case GCodeAuxKind::BedHeater:
            bed_heater_active = event.active;
            break;
        }
    }

    void resolve_aux_for_batch(const std::vector<PlanMove>& batch,
                               double batch_start)
    {
        const auto started_at = std::chrono::steady_clock::now();
        double move_time = batch_start;
        for (const PlanMove& move : batch) {
            while (!unresolved_aux.empty()
                   && unresolved_aux.front().line_idx <= move.line_idx) {
                resolve_aux_event(unresolved_aux.front(), move_time);
                unresolved_aux.pop_front();
            }
            move_time += move.accel_t + move.cruise_t + move.decel_t;
        }
        pathological_motion_history.append_planned_batch(batch, batch_start);
        aux_resolve_time_ms += elapsed_ms(started_at);
    }

    void finish_aux(double end_time)
    {
        while (!unresolved_aux.empty()) {
            resolve_aux_event(unresolved_aux.front(), end_time);
            unresolved_aux.pop_front();
        }
        emit_heater_until(end_time);
    }

    double step_pipeline_time_ms() const
    {
        return online_steps->prepare_time_ms()
            + online_steps->generate_a_time_ms()
            + online_steps->generate_b_time_ms()
            + online_steps->generate_e_time_ms()
            + online_steps->mcu_check_time_ms()
            + online_steps->main_flush_time_ms()
            + online_steps->nozzle_flush_time_ms()
            + online_steps->publish_time_ms()
            + online_steps->trapq_release_time_ms();
    }

    void complete_result()
    {
        if (result_completed)
            return;
        result.main_commands_consumed = main_serial->commands_consumed();
        result.nozzle_commands_consumed = nozzle_serial->commands_consumed();
        result.main_commands_total = main_serial->command_count();
        result.nozzle_commands_total = nozzle_serial->command_count();
        result.stream_consume_time_ms = stream_consume_time_ms;
        result.move_callback_time_ms = move_callback_time_ms;
        result.gcode_parse_dispatch_time_ms = std::max(0.0,
            stream_consume_time_ms - move_callback_time_ms - barrier_callback_time_ms
                - position_time_ms - aux_event_time_ms);
        result.toolhead_time_ms = toolhead_time_ms;
        result.barrier_time_ms = barrier_time_ms;
        result.position_time_ms = position_time_ms;
        result.aux_event_time_ms = aux_event_time_ms;
        result.aux_resolve_time_ms = aux_resolve_time_ms;
        result.step_prepare_time_ms = online_steps->prepare_time_ms();
        result.step_generate_a_time_ms = online_steps->generate_a_time_ms();
        result.step_generate_b_time_ms = online_steps->generate_b_time_ms();
        result.step_generate_e_time_ms = online_steps->generate_e_time_ms();
        result.step_generation_wall_time_ms = online_steps->generation_wall_time_ms();
        result.step_generation_invocations = online_steps->generation_invocations();
        result.parallel_generation_invocations = online_steps->parallel_generation_invocations();
        result.serial_generation_invocations = online_steps->serial_generation_invocations();
        result.parallel_phase_total = online_steps->parallel_phase_total();
        result.serial_phase_total = online_steps->serial_phase_total();
        result.max_generation_phases = online_steps->max_generation_phases();
        result.step_mcu_check_time_ms = online_steps->mcu_check_time_ms();
        result.step_main_flush_time_ms = online_steps->main_flush_time_ms();
        result.step_nozzle_flush_time_ms = online_steps->nozzle_flush_time_ms();
        result.step_publish_time_ms = online_steps->publish_time_ms();
        result.serial_dispatch_time_ms = serial_dispatch_time_ms;
        result.trapq_release_time_ms = online_steps->trapq_release_time_ms();
        result.finalize_time_ms = finalize_time_ms;
        result.parallel_step_generation = online_steps->parallel_step_generation();
        result.microsegment_batching = local.microsegment_batching;
        result.move_batch_count = stream->stats().move_batches;
        result.batched_move_count = stream->stats().batched_moves;
        result.forced_batch_flush_count = stream->stats().forced_batch_flushes;
        result.max_moves_per_batch = stream->stats().max_moves_per_batch;
        const Candidate* first = nullptr;
        if (main_candidate.hit)
            first = &main_candidate;
        if (nozzle_candidate.hit
            && (first == nullptr || nozzle_candidate.time < first->time))
            first = &nozzle_candidate;
        if (first != nullptr) {
            result.detected = true;
            result.source = first->source;
            result.occupied_slots = first->occupied;
            result.total_slots = first->total;
            result.occupancy = first->total > 0
                ? static_cast<double>(first->occupied) / first->total : 0.0;
            result.line_idx = first->line_idx;
            result.simulated_time = state.global_time + first->time;
        }
        state.global_time += std::max(
            0.0, simulation->toolhead().last_kin_move_time());
        result_completed = true;
    }

    ProbeConsumeState consume_line(const std::string& line, int line_idx)
    {
        if (fallback)
            return ProbeConsumeState::NeedReferenceFallback;
        if (terminal)
            return result.detected ? ProbeConsumeState::RiskDetected
                                   : ProbeConsumeState::Safe;

        if (cancel && ((result.lines_consumed & 0x3ffu) == 0))
            cancel();
        const auto consume_started_at = std::chrono::steady_clock::now();
        stream->consume_line(line, line_idx);
        stream_consume_time_ms += elapsed_ms(consume_started_at);
        ++result.lines_consumed;
        if (online_steps->stopped()) {
            complete_result();
            terminal = true;
            return ProbeConsumeState::RiskDetected;
        }
        return ProbeConsumeState::Continue;
    }

    ProbeConsumeState finish()
    {
        if (fallback)
            return ProbeConsumeState::NeedReferenceFallback;
        if (terminal)
            return result.detected ? ProbeConsumeState::RiskDetected
                                   : ProbeConsumeState::Safe;
        const auto finalize_started_at = std::chrono::steady_clock::now();
        if (!stream->flush_pending_moves() || online_steps->stopped()) {
            finalize_time_ms += elapsed_ms(finalize_started_at);
            complete_result();
            terminal = true;
            return ProbeConsumeState::RiskDetected;
        }
        simulation->finish();
        finish_aux(simulation->toolhead().last_kin_move_time());
        online_steps->finish(simulation->toolhead().last_kin_move_time(),
                             simulation->xy_trapq(), simulation->e_trapq());
        if (!online_steps->stopped()) {
            (void) main_serial->finish(simulation->toolhead().host_eventtime());
            (void) nozzle_serial->finish(simulation->toolhead().host_eventtime());
        }
        finalize_time_ms += elapsed_ms(finalize_started_at);
        complete_result();
        terminal = true;
        return result.detected ? ProbeConsumeState::RiskDetected
                               : ProbeConsumeState::Safe;
    }

    SimConfig local;
    double threshold = 0.80;
    std::function<void()> cancel;
    LineAnalysisState state;
    EarlyRiskResult result;
    Candidate main_candidate;
    Candidate nozzle_candidate;
    PathologicalMotionHistory pathological_motion_history;
    size_t receive_counter = 0;
    int main_threshold = 0;
    int nozzle_threshold = 0;
    bool have_pending_host = false;
    double pending_host_eventtime = 0.0;
    bool fallback = false;
    bool terminal = false;
    bool result_completed = false;
    double stream_consume_time_ms = 0.0;
    double move_callback_time_ms = 0.0;
    double toolhead_time_ms = 0.0;
    double barrier_callback_time_ms = 0.0;
    double barrier_time_ms = 0.0;
    double position_time_ms = 0.0;
    double aux_event_time_ms = 0.0;
    double aux_resolve_time_ms = 0.0;
    double serial_dispatch_time_ms = 0.0;
    double finalize_time_ms = 0.0;
    bool nozzle_heater_active = false;
    bool bed_heater_active = false;
    double next_nozzle_pwm_time = 0.0;
    double next_bed_pwm_time = 0.0;
    double aux_segment_start = 0.0;
    std::deque<GCodeAuxEvent> unresolved_aux;
    std::vector<HostDispatchCmd> pending_main_aux;
    std::vector<HostDispatchCmd> pending_nozzle_aux;
    std::unique_ptr<KlipperMCU> main_mcu;
    std::unique_ptr<KlipperMCU> nozzle_mcu;
    std::unique_ptr<McuMovePool> main_pool;
    std::unique_ptr<McuMovePool> nozzle_pool;
    std::unique_ptr<DeterministicSerialQueueSession> main_serial;
    std::unique_ptr<DeterministicSerialQueueSession> nozzle_serial;
    std::unique_ptr<KlipperOnlineStepGeneration> online_steps;
    std::unique_ptr<KlipperSimulationSession> simulation;
    std::unique_ptr<KlipperGCodeStream> stream;
};

struct StreamingLineAnalysisSession::Impl {
    struct PendingCommand {
        double recv_time = 0.0;
        double slot_free_time = 0.0;
        int line_idx = -1;
        int source_kind = HCS_UNKNOWN;
    };
    struct EarlierRelease {
        bool operator()(const PendingCommand& lhs,
                        const PendingCommand& rhs) const
        {
            return std::max(lhs.recv_time, lhs.slot_free_time)
                 > std::max(rhs.recv_time, rhs.slot_free_time);
        }
    };
    struct PendingStream {
        std::priority_queue<PendingCommand, std::vector<PendingCommand>,
                            EarlierRelease> provenance;
        bool have_last = false;
        double last_recv_time = 0.0;
    };

    Impl(const SimConfig& config, double threshold,
         const LineAnalysisState& initial_state,
         std::function<void()> cancel)
        : local(config), threshold(threshold), cancel(std::move(cancel)),
          state(initial_state)
    {
        if (state.global_time != 0.0 || !state.mcu_events.empty()
            || !state.noz_events.empty()
            || !(threshold > 0.0 && threshold <= 1.0)
            || local.mcu_pool_total <= 0 || local.nozzle_pool_total <= 0) {
            is_supported = false;
            return;
        }

        local.limits.max_accel = state.current_accel;
        local.limits.max_accel_to_decel = std::min(
            state.requested_accel_to_decel, local.limits.max_accel);
        local.limits.square_corner_velocity = state.current_square_corner_velocity;
        local.limits.junction_deviation = junction_deviation(
            local.limits.square_corner_velocity, local.limits.max_accel);

        main_mcu = std::make_unique<KlipperMCU>(
            "mcu", local.mcu_total, local.buffer_time_high, local.mcu_freq,
            0.0, local.mcu_reserved_slots, local.mcu_pool_total);
        nozzle_mcu = std::make_unique<KlipperMCU>(
            "nozzle_mcu", local.nozzle_total, local.buffer_time_high,
            std::make_unique<KlipperSecondarySync>(main_mcu->clocksync(),
                                                   local.mcu_freq),
            0.0, local.nozzle_reserved_slots, local.nozzle_pool_total);
        main_pool = std::make_unique<McuMovePool>(local.mcu_pool_total);
        nozzle_pool = std::make_unique<McuMovePool>(local.nozzle_pool_total);
        main_pool->set_baseline_frac(local.mcu_baseline_frac);
        nozzle_pool->set_baseline_frac(local.nozzle_baseline_frac);
        main_pool->set_compact_timeline(
            static_cast<int>(local.mcu_pool_total * threshold));
        nozzle_pool->set_compact_timeline(
            static_cast<int>(local.nozzle_pool_total * threshold));

        online_steps = std::make_unique<KlipperOnlineStepGeneration>(
            local, *main_mcu, *nozzle_mcu, true, OnlineCommandBatchCb{},
            [this](std::vector<HostDispatchCmd>& main_commands,
                   std::vector<HostDispatchCmd>& nozzle_commands,
                   double) {
                for (HostDispatchCmd& command : main_commands)
                    main_dispatch.emplace_back(std::move(command));
                for (HostDispatchCmd& command : nozzle_commands)
                    nozzle_dispatch.emplace_back(std::move(command));
                return SimulationControl::Continue;
            }, false);
        const std::array<double, 4> start_position{
            state.x + state.x_offset, state.y + state.y_offset,
            state.z + state.z_offset, state.e_abs};
        simulation = std::make_unique<KlipperSimulationSession>(
            local, start_position, state.have_pos, 0,
            [this](double eventtime) {
                return main_mcu->estimated_print_time(eventtime);
            }, ToolheadFlushCb{},
            [this](const ToolheadFlushWindow& window, KlipperTrapQ& xy,
                   KlipperTrapQ& e) {
                online_steps->on_flush(window, xy, e);
            }, false,
            &analysis.positive_extrusion_cruise_by_line,
            &analysis.positive_extrusion_distance_by_line, nullptr,
            [this](const std::vector<PlanMove>& batch, double batch_start) {
                double move_time = batch_start;
                for (const PlanMove& move : batch) {
                    if (move.line_idx >= 0) {
                        const size_t line_idx = static_cast<size_t>(move.line_idx);
                        if (line_start_times.size() <= line_idx)
                            line_start_times.resize(line_idx + 1, -1.0);
                        if (line_start_times[line_idx] < 0.0)
                            line_start_times[line_idx] = move_time;
                    }
                    move_time += move.accel_t + move.cruise_t + move.decel_t;
                }
            });

        GCodeStreamCallbacks callbacks;
        auto consume_moves = [this](const GMove* moves, size_t count) {
            for (size_t i = 0; i < count; ++i)
                simulation->consume_move(moves[i]);
            return true;
        };
        callbacks.on_move = [consume_moves](const GMove& move) {
            (void) consume_moves(&move, 1);
        };
        if (local.microsegment_batching) {
            callbacks.on_move_batch = [consume_moves](
                const std::vector<GMove>& moves) {
                return consume_moves(moves.data(), moves.size());
            };
        }
        callbacks.on_barrier = [this](const SimulationBarrier& barrier) {
            simulation->consume_barrier(barrier);
        };
        callbacks.on_position = [this](const std::array<double, 4>& position,
                                       bool valid) {
            simulation->set_position(position, valid);
        };
        callbacks.on_aux = [this](const GCodeAuxEvent& event) {
            aux_events.push_back(event);
        };
        stream = std::make_unique<KlipperGCodeStream>(
            local, state, std::move(callbacks));
    }

    void ensure_line_capacity(size_t count)
    {
        if (analysis.flags.size() >= count)
            return;
        analysis.flags.resize(count, 0);
        main_flags.resize(count, 0);
        nozzle_flags.resize(count, 0);
        line_start_times.resize(count, -1.0);
        analysis.positive_extrusion_cruise_by_line.resize(count, 0.0);
        analysis.positive_extrusion_distance_by_line.resize(count, 0.0);
    }

    static void flag_command(std::vector<char>& flags,
                             const PendingCommand& command)
    {
        if (command.line_idx >= 0
            && static_cast<size_t>(command.line_idx) < flags.size())
            flags[static_cast<size_t>(command.line_idx)] = 1;
    }

    void consume_command(McuMovePool& pool, PendingStream& pending,
                         std::vector<char>& flags,
                         const HostDispatchCmd& command, bool check_cancel)
    {
        if (cancel && check_cancel
            && ((main_receive_counter++ & 0x3ffu) == 0))
            cancel();
        if (!command.uses_move_slot)
            return;
        PendingCommand received{
            std::max(command.recv_time, command.min_time),
            std::max(0.0, command.slot_free_time),
            command.line_idx, command.source_kind};
        if (pending.have_last
            && received.recv_time < pending.last_recv_time)
            throw std::logic_error(
                "KlipperSim streaming analysis receive frontier regressed");
        pending.have_last = true;
        pending.last_recv_time = received.recv_time;
        pool.alloc(received.recv_time,
                   std::max(received.recv_time, received.slot_free_time),
                   received.line_idx, received.source_kind);
        while (!pending.provenance.empty()
               && std::max(pending.provenance.top().recv_time,
                           pending.provenance.top().slot_free_time)
                      <= received.recv_time)
            pending.provenance.pop();
        if (pool.compact_open_high_start() >= 0.0) {
            while (!pending.provenance.empty()) {
                flag_command(flags, pending.provenance.top());
                pending.provenance.pop();
            }
            flag_command(flags, received);
        } else {
            pending.provenance.push(received);
        }
    }

    void consume_line(const std::string& line, int line_idx)
    {
        if (!is_supported || finished)
            return;
        if (cancel && ((line_count & 0x3ffu) == 0))
            cancel();
        if (line_idx < 0 || static_cast<size_t>(line_idx) != line_count)
            throw std::logic_error(
                "streaming analysis line indexes must be contiguous");
        ++line_count;
        ensure_line_capacity(static_cast<size_t>(line_idx) + 1);
        stream->consume_line(line, line_idx);
    }

    void finish()
    {
        if (!is_supported || finished)
            return;
        if (!stream->flush_pending_moves())
            throw std::logic_error("streaming analysis move flush stopped");
        simulation->finish();
        const double end_time = simulation->toolhead().last_kin_move_time();
        online_steps->finish(end_time, simulation->xy_trapq(),
                             simulation->e_trapq());

        std::vector<double> next_line_exec(line_start_times.size(), end_time);
        double next_exec = end_time;
        for (size_t i = line_start_times.size(); i-- > 0;) {
            if (line_start_times[i] >= 0.0)
                next_exec = line_start_times[i];
            next_line_exec[i] = next_exec;
        }
        const auto line_exec_time = [&next_line_exec, end_time](int line_idx) {
            return line_idx >= 0
                    && static_cast<size_t>(line_idx) < next_line_exec.size()
                ? next_line_exec[static_cast<size_t>(line_idx)] : end_time;
        };
        AuxDispatchState aux_state;
        aux_state.nozzle_heater_active = state.extruder_heater_active;
        aux_state.bed_heater_active = state.bed_heater_active;
        aux_state.next_nozzle_pwm_t = std::max(
            0.0, state.next_extruder_pwm_time - state.global_time);
        aux_state.next_bed_pwm_t = std::max(
            0.0, state.next_bed_pwm_time - state.global_time);
        const double advance_window = static_cast<double>(1ULL << 31)
                                    / local.mcu_freq;
        aux_state = append_aux_dispatch_cmds(
            aux_events, end_time, local.buffer_time_high, advance_window,
            local.mcu_freq, line_exec_time, main_dispatch, nozzle_dispatch,
            aux_state);

        schedule_host_dispatch(
            main_dispatch, nozzle_dispatch, simulation->toolhead().flush_windows(),
            simulation->toolhead().host_eventtime(), local.buffer_time_high,
            main_mcu->host_move_slots(), nozzle_mcu->host_move_slots(),
            [this](double eventtime) {
                return main_mcu->estimated_print_time(eventtime);
            },
            [this](double eventtime) {
                return nozzle_mcu->estimated_print_time(eventtime);
            },
            dispatch_config(local, false), dispatch_config(local, true),
            [this](const HostDispatchCmd& command) {
                consume_command(*main_pool, main_pending, main_flags, command,
                                true);
            },
            [this](const HostDispatchCmd& command) {
                consume_command(*nozzle_pool, nozzle_pending, nozzle_flags,
                                command, false);
            });
        main_pool->finish_compact_timeline();
        nozzle_pool->finish_compact_timeline();
        for (size_t i = 0; i < analysis.flags.size(); ++i)
            analysis.flags[i] = static_cast<char>(main_flags[i]
                                                  || nozzle_flags[i]);
        state.global_time += std::max(0.0, end_time);
        state.extruder_heater_active = aux_state.nozzle_heater_active;
        state.bed_heater_active = aux_state.bed_heater_active;
        state.next_extruder_pwm_time = state.global_time
                                     + aux_state.next_nozzle_pwm_t;
        state.next_bed_pwm_time = state.global_time
                                + aux_state.next_bed_pwm_t;
        analysis.line_state = state;
        finished = true;
    }

    SimConfig local;
    double threshold = 0.80;
    std::function<void()> cancel;
    LineAnalysisState state;
    StreamingLineAnalysisResult analysis;
    bool is_supported = true;
    bool finished = false;
    size_t line_count = 0;
    size_t main_receive_counter = 0;
    PendingStream main_pending;
    PendingStream nozzle_pending;
    std::vector<char> main_flags;
    std::vector<char> nozzle_flags;
    std::vector<double> line_start_times;
    std::vector<GCodeAuxEvent> aux_events;
    std::vector<HostDispatchCmd> main_dispatch;
    std::vector<HostDispatchCmd> nozzle_dispatch;
    std::unique_ptr<KlipperMCU> main_mcu;
    std::unique_ptr<KlipperMCU> nozzle_mcu;
    std::unique_ptr<McuMovePool> main_pool;
    std::unique_ptr<McuMovePool> nozzle_pool;
    std::unique_ptr<KlipperOnlineStepGeneration> online_steps;
    std::unique_ptr<KlipperSimulationSession> simulation;
    std::unique_ptr<KlipperGCodeStream> stream;
};

StreamingLineAnalysisSession::StreamingLineAnalysisSession(
    const SimConfig& config, double threshold,
    const LineAnalysisState& initial_state,
    const std::function<void()>& cancel)
    : m_impl(std::make_unique<Impl>(config, threshold, initial_state, cancel))
{}

StreamingLineAnalysisSession::~StreamingLineAnalysisSession() = default;

void StreamingLineAnalysisSession::consume_line(
    const std::string& line, int line_idx)
{
    m_impl->consume_line(line, line_idx);
}

void StreamingLineAnalysisSession::finish()
{
    m_impl->finish();
}

bool StreamingLineAnalysisSession::supported() const
{
    return m_impl->is_supported;
}

const StreamingLineAnalysisResult& StreamingLineAnalysisSession::result() const
{
    return m_impl->analysis;
}

StreamingEarlyProbeSession::StreamingEarlyProbeSession(
    const SimConfig& config, double threshold,
    const LineAnalysisState& initial_state,
    const std::function<void()>& cancel)
    : m_impl(std::make_unique<Impl>(config, threshold, initial_state, cancel))
{}

StreamingEarlyProbeSession::~StreamingEarlyProbeSession() = default;

ProbeConsumeState StreamingEarlyProbeSession::consume_line(
    const std::string& line, int line_idx)
{
    return m_impl->consume_line(line, line_idx);
}

ProbeConsumeState StreamingEarlyProbeSession::finish()
{
    return m_impl->finish();
}

const EarlyRiskResult& StreamingEarlyProbeSession::result() const
{
    return m_impl->result;
}

const LineAnalysisState& StreamingEarlyProbeSession::line_state() const
{
    return m_impl->state;
}

bool run_streaming_early_probe(
    const std::vector<std::string>& lines,
    const SimConfig& config,
    double threshold,
    LineAnalysisState& state,
    const std::function<void()>& cancel,
    EarlyRiskResult& result)
{
    if (state.global_time != 0.0 || !state.mcu_events.empty()
        || !state.noz_events.empty())
        return false;

    StreamingEarlyProbeSession session(config, threshold, state, cancel);
    ProbeConsumeState consume_state = ProbeConsumeState::Continue;
    for (size_t line_index = 0;
         line_index < lines.size()
            && consume_state == ProbeConsumeState::Continue;
         ++line_index) {
        consume_state = session.consume_line(
            lines[line_index], static_cast<int>(line_index));
    }
    if (consume_state == ProbeConsumeState::Continue)
        consume_state = session.finish();
    if (consume_state == ProbeConsumeState::NeedReferenceFallback
        || consume_state == ProbeConsumeState::Error)
        return false;
    result = session.result();
    state = session.line_state();
    return true;
}

}} // namespace Slic3r::KlipperSim
