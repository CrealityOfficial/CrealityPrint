#include "KlipperGCodeStream.hpp"
#include "KlipperOnlineStepGeneration.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <queue>
#include <stdexcept>
#include <string>
#include <vector>

using namespace Slic3r::KlipperSim;

namespace {

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::vector<std::string> read_lines(const std::string& path)
{
    std::ifstream input(path);
    require(input.good(), "cannot open regression gcode");
    std::vector<std::string> lines;
    for (std::string line; std::getline(input, line);)
        lines.push_back(line);
    return lines;
}

struct RunResult
{
    LineAnalysisState              state;
    GCodeStreamStats               stats;
    std::vector<GCodeAuxEvent>     aux;
    std::vector<SimulationBarrier> barriers;
    std::vector<GMove>             moves;
    std::array<double, 4>          initial_position{0.0, 0.0, 0.0, 0.0};
    bool                           have_initial_position = false;
    std::vector<PlanMove>          planned;
    std::vector<double>            starts;
    std::vector<double>            ends;
};

RunResult run(const std::vector<std::string>& lines,
              const SimConfig& cfg = SimConfig{})
{
    RunResult                result;
    KlipperSimulationSession session(cfg, {0.0, 0.0, 0.0, 0.0}, false, lines.size(), {}, {}, {}, true);
    GCodeStreamCallbacks     callbacks;
    callbacks.on_move = [&](const GMove& move) {
        result.moves.push_back(move);
        session.consume_move(move);
        require(session.move_count() > 0, "move was not delivered to the planner immediately");
    };
    callbacks.on_aux     = [&](const GCodeAuxEvent& event) { result.aux.push_back(event); };
    callbacks.on_barrier = [&](const SimulationBarrier& barrier) {
        result.barriers.push_back(barrier);
        session.consume_barrier(barrier);
    };
    callbacks.on_position = [&](const std::array<double, 4>& position, bool valid) {
        result.initial_position      = position;
        result.have_initial_position = valid;
        session.set_position(position, valid);
    };
    KlipperGCodeStream stream(cfg, result.state, std::move(callbacks));
    for (size_t i = 0; i < lines.size(); ++i)
        stream.consume_line(lines[i], static_cast<int>(i));
    session.finish();
    result.stats   = stream.stats();
    result.planned = session.planned_moves();
    result.starts  = session.line_start_times();
    result.ends    = session.line_end_times();
    return result;
}

void test_continuous(const std::string& root)
{
    RunResult result = run(read_lines(root + "/continuous.gcode"));
    require(result.stats.moves == 4, "continuous: wrong move count");
    require(result.stats.barriers == 0, "continuous: unexpected barrier");
    require(result.planned.size() == 4, "continuous: planner lost moves");
    require(result.planned[0].end_v > 0.0, "continuous: lookahead did not join adjacent moves");
    require(std::fabs(result.state.current_accel - 5000.0) < 1e-9, "continuous: acceleration state was not retained");
    require(std::fabs(result.state.current_pressure_advance - 0.031) < 1e-9, "continuous: pressure advance state was not retained");

    SimConfig                cfg;
    KlipperSimulationSession batch(cfg, result.initial_position, result.have_initial_position, 16, {}, {}, {}, true);
    for (const GMove& move : result.moves)
        batch.consume_move(move);
    batch.finish();
    require(batch.planned_moves().size() == result.planned.size(), "continuous: batch adapter changed move count");
    for (size_t i = 0; i < result.planned.size(); ++i) {
        const PlanMove& streamed = result.planned[i];
        const PlanMove& adapted  = batch.planned_moves()[i];
        require(std::fabs(streamed.start_v - adapted.start_v) < 1e-9 && std::fabs(streamed.cruise_v - adapted.cruise_v) < 1e-9 &&
                    std::fabs(streamed.end_v - adapted.end_v) < 1e-9,
                "continuous: batch adapter changed lookahead output");
    }
}

void test_microsegments(const std::string& root)
{
    RunResult result = run(read_lines(root + "/microsegments.gcode"));
    require(result.stats.moves == 10, "microsegments: wrong move count");
    require(result.planned.size() == 10, "microsegments: planner lost moves");
}

void test_arcs(const std::string& root)
{
    RunResult result = run(read_lines(root + "/arcs.gcode"));
    require(result.stats.moves == 52, "arcs: wrong expanded move count");
    require(result.planned.size() == 52, "arcs: planner lost expanded moves");
    require(std::fabs(result.state.x) < 1e-9 && std::fabs(result.state.y - 10.0) < 1e-9
                && std::fabs(result.state.z - 3.0) < 1e-9,
            "arcs: final XYZ state did not follow arc endpoints");
    require(std::fabs(result.state.e_abs - 1.0) < 1e-9,
            "arcs: absolute extrusion state was not retained");
    for (const GMove& move : result.moves)
        require(move.line_idx == 3 || move.line_idx == 5 || move.line_idx == 7 || move.line_idx == 9,
                "arcs: expanded move lost source line mapping");

    const ParsedGCode offline = parse_gcode_detailed(root + "/arcs.gcode");
    require(offline.moves.size() == result.moves.size(),
            "arcs: offline and streaming expansion disagree on count");
    require(std::fabs(offline.moves.back().x) < 1e-9
                && std::fabs(offline.moves.back().y - 10.0) < 1e-9
                && std::fabs(offline.moves.back().z - 3.0) < 1e-9,
            "arcs: offline expansion final endpoint is wrong");
}

void test_barriers(const std::string& root)
{
    RunResult result = run(read_lines(root + "/barriers.gcode"));
    require(result.stats.moves == 5, "barriers: wrong move count");
    require(result.stats.barriers == 4, "barriers: wrong barrier count");
    require(result.planned.size() == 5, "barriers: planner lost moves");
    for (const PlanMove& move : result.planned) {
        require(std::fabs(move.start_v) < 1e-9, "barriers: move crossed a synchronization boundary");
        require(std::fabs(move.end_v) < 1e-9, "barriers: move crossed a synchronization boundary");
    }
    require(result.starts[7] - result.ends[5] >= 0.249, "barriers: G4 dwell was not reflected in the timeline");
}

void test_aux(const std::string& root)
{
    RunResult result = run(read_lines(root + "/aux_interleave.gcode"));
    require(result.stats.moves == 3, "aux: wrong move count");
    require(result.stats.aux_events == 7, "aux: wrong event count");
    require(result.stats.barriers == 0, "aux: non-waiting commands became barriers");
}

void test_state_changes(const std::string& root)
{
    SimConfig cfg;
    cfg.pressure_advance_commands_enabled = true;
    RunResult result = run(read_lines(root + "/state_changes.gcode"), cfg);
    require(result.planned.size() == 2, "state changes: wrong move count");
    require(std::fabs(result.planned[0].accel - 3000.0) < 1e-9 && std::fabs(result.planned[0].pressure_advance) < 1e-9,
            "state changes: first move used future limits");
    require(std::fabs(result.planned[1].accel - 6000.0) < 1e-9 && std::fabs(result.planned[1].pressure_advance - 0.050) < 1e-9,
            "state changes: updated limits were not applied in place");

    const ParsedGCode offline = parse_gcode_detailed(root + "/state_changes.gcode");
    require(offline.moves.size() == 2
                && std::fabs(offline.moves[0].step_generation_scan_time_before) < 1e-12
                && std::fabs(offline.moves[1].step_generation_scan_time_before - 0.010) < 1e-12,
            "state changes: offline adapter lost PA scan-time flush boundaries");
}

void test_creality_pa_enable_and_single_tool()
{
    SimConfig cfg;
    cfg.pressure_advance_commands_enabled = true;
    RunResult result = run({
        "G90", "G1 X1 F6000",
        "SET_PRESSURE_ADVANCE ADVANCE=0.040",
        "ENABLE_PRESSURE_ADVANCE VALUE=0",
        "SET_PRESSURE_ADVANCE ADVANCE=0.090",
        "G1 X2", "T0", "ACTIVATE_EXTRUDER EXTRUDER=extruder",
        "ENABLE_PRESSURE_ADVANCE VALUE=1", "G1 X3"}, cfg);
    require(result.planned.size() == 2,
            "PA enable: single-extruder no-op commands changed move count");
    require(result.stats.barriers == 0,
            "PA enable: PA flush was incorrectly counted as a blocking barrier");
    require(std::fabs(result.planned[0].pressure_advance - 0.040) < 1e-9
                && std::fabs(result.planned[1].pressure_advance - 0.040) < 1e-9,
            "PA enable: disabled SET changed or cleared retained PA");
}

void test_initial_pa_command_gate()
{
    SimConfig cfg;
    cfg.pressure_advance = 0.034;
    cfg.pressure_advance_commands_enabled = false;
    RunResult result = run({
        "G90", "M83", "G92 X0 Y0 Z0 E0",
        "SET_PRESSURE_ADVANCE ADVANCE=0.090",
        "G1 X1 E0.01 F6000", "ENABLE_PRESSURE_ADVANCE VALUE=1",
        "SET_PRESSURE_ADVANCE ADVANCE=0.050", "G1 X2 E0.01"}, cfg);
    require(result.planned.size() == 2, "PA gate: wrong move count");
    require(std::fabs(result.planned[0].pressure_advance - 0.034) < 1e-9,
            "PA gate: disabled SET changed the active initial PA");
    require(std::fabs(result.planned[1].pressure_advance - 0.050) < 1e-9,
            "PA gate: enabling SET did not apply the new PA");
}

void test_extruder_limits_and_pa_semantics()
{
    SimConfig cfg;
    const double expected_max_e_accel =
        10000.0 * (4.0 * 0.4 * 0.4)
        / (3.14159265358979323846 * 0.875 * 0.875);
    require(std::fabs(cfg.max_extrude_only_accel - expected_max_e_accel) < 1e-6,
            "extruder limits: max_e_accel does not match Creality startup derivation");

    KlipperSimulationSession session(cfg, {0.0, 0.0, 0.0, 0.0}, true, 8,
                                     {}, {}, {}, true);
    session.consume_move({0.0, 0.0, 0.0, 1.0, 30.0, 10000.0, 5000.0, 8.0,
                          0.031, 0.040, 1});
    session.consume_move({10.0, 0.0, 0.0, 0.5, 60.0, 10000.0, 5000.0, 8.0,
                          0.031, 0.040, 2});
    session.consume_move({20.0, 0.0, 0.0, -0.5, 60.0, 10000.0, 5000.0, 8.0,
                          0.031, 0.040, 3});
    session.consume_move({20.0, 0.0, 1.0, 0.5, 20.0, 10000.0, 5000.0, 8.0,
                          0.031, 0.040, 4});
    session.finish();

    require(!session.planned_moves().empty()
                && std::fabs(session.planned_moves().front().accel
                             - expected_max_e_accel) < 1e-6,
            "extruder limits: pure E move did not use max_e_accel");
    require(session.toolhead().last_kin_flush_time()
                    - session.toolhead().last_kin_move_time() >= 0.019999,
            "PA scan time: initial smoothing window was not registered");

    bool positive_xy_pa = false;
    bool retract_pa = false;
    bool z_only_pa = false;
    for (const TrapMove& phase : session.e_trapq().moves()) {
        if (phase.line_idx == 2 && phase.axes_r.y != 0.0)
            positive_xy_pa = true;
        if (phase.line_idx == 3 && phase.axes_r.y != 0.0)
            retract_pa = true;
        if (phase.line_idx == 4 && phase.axes_r.y != 0.0)
            z_only_pa = true;
    }
    require(positive_xy_pa, "PA eligibility: positive XY extrusion lost PA");
    require(!retract_pa, "PA eligibility: XY retraction incorrectly received PA");
    require(!z_only_pa, "PA eligibility: Z-only extrusion incorrectly received PA");
}

void test_pressure_advance_flush_boundary()
{
    SimConfig cfg;
    cfg.pressure_advance_commands_enabled = true;
    RunResult result = run({
        "G90", "M83", "G92 X0 Y0 Z0 E0",
        "G1 X1 E0.01 F6000", "G1 X2 E0.01",
        "SET_PRESSURE_ADVANCE ADVANCE=0.050 SMOOTH_TIME=0.060",
        "G1 X3 E0.01"}, cfg);
    require(result.planned.size() == 3,
            "PA flush: planner lost a move");
    require(result.stats.barriers == 0,
            "PA flush: scan-time flush was counted as a blocking barrier");
    require(std::fabs(result.planned[1].end_v) < 1e-9
                && std::fabs(result.planned[2].start_v) < 1e-9,
            "PA flush: lookahead crossed the PA change boundary");
    require(std::fabs(result.planned[1].pressure_advance - 0.031) < 1e-9
                && std::fabs(result.planned[2].pressure_advance - 0.050) < 1e-9,
            "PA flush: old/new PA states were applied to the wrong side");
}

void test_homing(const std::string& root)
{
    RunResult result = run(read_lines(root + "/homing.gcode"));
    require(result.stats.barriers == 1, "homing: G28 was not a barrier");
    require(result.planned.size() == 2, "homing: position validity was not reset");
    require(std::fabs(result.planned[0].end_v) < 1e-9 && std::fabs(result.planned[1].start_v) < 1e-9, "homing: lookahead crossed G28");
}

void test_g92_machine_coordinates()
{
    const std::vector<std::string> lines{"G90", "G1 X100 Y20 Z0.2 F6000", "G92 X0", "G1 X10 Y20"};
    RunResult                      result = run(lines);
    require(result.planned.size() == 1, "G92: wrong move count");
    require(std::fabs(result.planned[0].axes_d[0] - 10.0) < 1e-9, "G92: logical reset changed the physical travel distance");

    RunResult initial_reset = run({"G90", "G92 E0", "G1 X10 F6000"});
    require(initial_reset.planned.size() == 1 && std::fabs(initial_reset.planned[0].axes_d[0] - 10.0) < 1e-9,
            "G92: initial coordinate reset caused the first move to be dropped");
}

void require_same_commands(const std::vector<StepMoveCmd>& online, const std::vector<StepMoveCmd>& replay, const char* axis)
{
    require(online.size() == replay.size(), "online stepgen: command count differs");
    for (size_t i = 0; i < online.size(); ++i) {
        const StepMoveCmd& a = online[i];
        const StepMoveCmd& b = replay[i];
        if (a.interval != b.interval || a.count != b.count || a.add != b.add || a.first_clock != b.first_clock ||
            a.last_clock != b.last_clock || a.req_clock != b.req_clock || a.min_clock != b.min_clock || a.free_clock != b.free_clock ||
            a.line_idx != b.line_idx || a.output_sequence != b.output_sequence) {
            std::cerr << "online stepgen mismatch axis=" << axis << " command=" << i << '\n';
            require(false, "online stepgen: command content differs");
        }
    }
}

void require_firmware_steppersync_order(
    const std::vector<const KlipperStepCompress*>& stepqueues,
    const std::vector<HostDispatchCmd>& actual, int move_slots,
    double mcu_freq, int source_kind)
{
    struct Message {
        uint64_t sequence = 0;
        uint64_t req_clock = 0;
        uint64_t free_clock = 0;
        bool uses_move_slot = false;
    };
    std::vector<std::vector<Message>> queues;
    queues.reserve(stepqueues.size());
    for (const KlipperStepCompress* stepqueue : stepqueues) {
        std::vector<Message> queue;
        queue.reserve(stepqueue->commands().size() + stepqueue->aux_commands().size());
        for (const StepMoveCmd& move : stepqueue->commands())
            queue.push_back({move.output_sequence, move.req_clock, move.min_clock, true});
        for (const StepAuxCmd& aux : stepqueue->aux_commands())
            queue.push_back({aux.output_sequence, aux.req_clock, 0, false});
        std::sort(queue.begin(), queue.end(), [](const Message& a, const Message& b) {
            return a.sequence < b.sequence;
        });
        queues.push_back(std::move(queue));
    }

    std::vector<size_t> positions(queues.size(), 0);
    std::priority_queue<uint64_t, std::vector<uint64_t>, std::greater<uint64_t>> slots;
    for (int i = 0; i < move_slots; ++i)
        slots.push(0);
    require(!slots.empty(), "steppersync reference: empty move clock heap");

    size_t output = 0;
    for (;;) {
        size_t best = queues.size();
        uint64_t best_req = std::numeric_limits<uint64_t>::max();
        for (size_t i = 0; i < queues.size(); ++i) {
            if (positions[i] == queues[i].size())
                continue;
            const uint64_t req = queues[i][positions[i]].req_clock;
            if (req < best_req) {
                best = i;
                best_req = req;
            }
        }
        if (best == queues.size())
            break;
        require(output < actual.size(), "steppersync reference: missing output command");
        const Message& expected = queues[best][positions[best]++];
        const HostDispatchCmd& command = actual[output++];
        const uint64_t actual_req = static_cast<uint64_t>(std::llround(command.req_time * mcu_freq));
        const uint64_t actual_min = static_cast<uint64_t>(std::llround(command.min_time * mcu_freq));
        require(actual_req == expected.req_clock,
                "steppersync reference: req_clock order differs");
        require(actual_min == slots.top(),
                "steppersync reference: move_clocks minimum differs");
        require(command.uses_move_slot == expected.uses_move_slot
                    && command.source_kind == source_kind,
                "steppersync reference: command type differs");
        if (expected.uses_move_slot) {
            slots.pop();
            slots.push(expected.free_clock);
        }
    }
    require(output == actual.size(), "steppersync reference: extra output command");
}

void require_same_dispatch(const std::vector<HostDispatchCmd>& online,
                           const std::vector<HostDispatchCmd>& replay,
                           const char* mcu)
{
    require(online.size() == replay.size(), "online steppersync: command count differs");
    for (size_t i = 0; i < online.size(); ++i) {
        const HostDispatchCmd& a = online[i];
        const HostDispatchCmd& b = replay[i];
        if (a.min_time != b.min_time || a.req_time != b.req_time || a.slot_free_time != b.slot_free_time ||
            a.exec_start != b.exec_start || a.exec_end != b.exec_end || a.line_idx != b.line_idx ||
            a.source_kind != b.source_kind || a.uses_move_slot != b.uses_move_slot || a.msg_len != b.msg_len) {
            std::cerr << "online steppersync mismatch mcu=" << mcu << " command=" << i << '\n';
            require(false, "online steppersync: command content differs");
        }
    }
}

void require_same_dispatch_trace(const std::vector<HostDispatchCmd>& batch,
                                 const std::vector<HostDispatchCmd>& incremental,
                                 const char* mcu)
{
    require(batch.size() == incremental.size(),
            "serialqueue session: command count differs");
    for (size_t i = 0; i < batch.size(); ++i) {
        const HostDispatchCmd& a = batch[i];
        const HostDispatchCmd& b = incremental[i];
        if (a.send_time != b.send_time || a.recv_time != b.recv_time
            || a.ack_time != b.ack_time
            || a.block_end_time != b.block_end_time
            || a.block_sequence != b.block_sequence
            || a.block_index != b.block_index
            || a.enqueue_sequence != b.enqueue_sequence) {
            std::cerr << "serialqueue trace mismatch mcu=" << mcu
                      << " command=" << i << '\n';
            require(false, "serialqueue session: per-command trace differs");
        }
    }
}

void test_online_step_generation(const std::string& root)
{
    const std::vector<std::string> lines = read_lines(root + "/microsegments.gcode");
    SimConfig                      cfg;
    LineAnalysisState              state;
    KlipperMCU                     main_mcu("mcu", cfg.mcu_total, cfg.buffer_time_high, cfg.mcu_freq, 0.0,
                                            cfg.mcu_reserved_slots, cfg.mcu_pool_total);
    KlipperMCU                     nozzle_mcu("nozzle_mcu", cfg.nozzle_total, cfg.buffer_time_high,
                                              std::make_unique<KlipperSecondarySync>(main_mcu.clocksync(), cfg.mcu_freq), 0.0,
                                              cfg.nozzle_reserved_slots, cfg.nozzle_pool_total);
    KlipperOnlineStepGeneration    online(cfg, main_mcu, nozzle_mcu);
    KlipperSimulationSession       session(cfg, {0.0, 0.0, 0.0, 0.0}, false, lines.size(), {}, {},
                                           [&](const ToolheadFlushWindow& window, KlipperTrapQ& xy, KlipperTrapQ& e) {
                                         online.on_flush(window, xy, e);
                                     });
    GCodeStreamCallbacks           callbacks;
    callbacks.on_move     = [&](const GMove& move) { session.consume_move(move); };
    callbacks.on_barrier  = [&](const SimulationBarrier& barrier) { session.consume_barrier(barrier); };
    callbacks.on_position = [&](const std::array<double, 4>& position, bool valid) { session.set_position(position, valid); };
    KlipperGCodeStream stream(cfg, state, std::move(callbacks));
    for (size_t i = 0; i < lines.size(); ++i)
        stream.consume_line(lines[i], static_cast<int>(i));
    session.finish();
    online.finish(session.toolhead().last_kin_move_time(), session.xy_trapq(), session.e_trapq());

    KlipperMCU                  replay_main("mcu", cfg.mcu_total, cfg.buffer_time_high, cfg.mcu_freq, 0.0,
                                            cfg.mcu_reserved_slots, cfg.mcu_pool_total);
    KlipperMCU                  replay_nozzle("nozzle_mcu", cfg.nozzle_total, cfg.buffer_time_high,
                                              std::make_unique<KlipperSecondarySync>(replay_main.clocksync(), cfg.mcu_freq), 0.0,
                                              cfg.nozzle_reserved_slots, cfg.nozzle_pool_total);
    KlipperOnlineStepGeneration replay(cfg, replay_main, replay_nozzle);
    for (const ToolheadFlushWindow& window : session.toolhead().flush_windows())
        replay.on_flush(window, session.xy_trapq(), session.e_trapq());
    replay.finish(session.toolhead().last_kin_move_time(), session.xy_trapq(), session.e_trapq());

    KlipperMCU sink_main("mcu", cfg.mcu_total, cfg.buffer_time_high,
                         cfg.mcu_freq, 0.0, cfg.mcu_reserved_slots,
                         cfg.mcu_pool_total);
    KlipperMCU sink_nozzle(
        "nozzle_mcu", cfg.nozzle_total, cfg.buffer_time_high,
        std::make_unique<KlipperSecondarySync>(sink_main.clocksync(), cfg.mcu_freq),
        0.0, cfg.nozzle_reserved_slots, cfg.nozzle_pool_total);
    std::vector<HostDispatchCmd> sink_main_commands, sink_nozzle_commands;
    KlipperOnlineStepGeneration sink(
        cfg, sink_main, sink_nozzle, false,
        [&](std::vector<HostDispatchCmd>&& main_batch,
            std::vector<HostDispatchCmd>&& nozzle_batch, double) {
            sink_main_commands.insert(
                sink_main_commands.end(),
                std::make_move_iterator(main_batch.begin()),
                std::make_move_iterator(main_batch.end()));
            sink_nozzle_commands.insert(
                sink_nozzle_commands.end(),
                std::make_move_iterator(nozzle_batch.begin()),
                std::make_move_iterator(nozzle_batch.end()));
            return SimulationControl::Continue;
        });
    for (const ToolheadFlushWindow& window : session.toolhead().flush_windows())
        sink.on_flush(window, session.xy_trapq(), session.e_trapq());
    sink.finish(session.toolhead().last_kin_move_time(), session.xy_trapq(),
                session.e_trapq());


    require(online.windows().size() == replay.windows().size(), "online stepgen: flush window count differs");
    for (size_t i = 0; i < online.windows().size(); ++i) {
        const OnlineStepWindow& a = online.windows()[i];
        const OnlineStepWindow& b = replay.windows()[i];
        require(a.a_steps == b.a_steps && a.b_steps == b.b_steps && a.e_steps == b.e_steps && a.a_commands == b.a_commands &&
                    a.b_commands == b.b_commands && a.e_commands == b.e_commands && a.main_scheduled == b.main_scheduled &&
                    a.nozzle_scheduled == b.nozzle_scheduled,
                "online stepgen: per-window frontier differs");
    }
    require_same_commands(online.a_stepqueue().commands(), replay.a_stepqueue().commands(), "A");
    require_same_commands(online.b_stepqueue().commands(), replay.b_stepqueue().commands(), "B");
    require_same_commands(online.e_stepqueue().commands(), replay.e_stepqueue().commands(), "E");
    require_same_dispatch(online.main_commands(), replay.main_commands(), "mcu");
    require_same_dispatch(online.nozzle_commands(), replay.nozzle_commands(), "nozzle_mcu");
    require_same_dispatch(online.main_commands(), sink_main_commands, "mcu-sink");
    require_same_dispatch(online.nozzle_commands(), sink_nozzle_commands,
                          "nozzle_mcu-sink");
    require(sink.retained_dispatch_commands() == 0,
            "online command sink retained published dispatch commands");
    require_firmware_steppersync_order(
        {&online.a_stepqueue(), &online.b_stepqueue()}, online.main_commands(),
        main_mcu.host_move_slots(), cfg.mcu_freq, HCS_XY_STEP);
    require_firmware_steppersync_order(
        {&online.e_stepqueue()}, online.nozzle_commands(),
        nozzle_mcu.host_move_slots(), cfg.mcu_freq, HCS_E_STEP);

    auto compare_persistent = [&](const std::vector<HostDispatchCmd>& input,
                                  const HostDispatchConfig& config,
                                  const std::function<double(double)>& estimate,
                                  const char* name) {
        std::vector<HostDispatchCmd> batch = input;
        std::vector<HostDispatchCmd> empty;
        schedule_host_dispatch(batch, empty, {}, session.toolhead().host_eventtime(),
                               cfg.buffer_time_high, 10000, 10000, estimate, {},
                               config, config);

        DeterministicSerialQueueSession incremental(config, estimate);
        size_t command_index = 0;
        while (command_index < input.size()) {
            const double enqueue_time = input[command_index].enqueue_time;
            size_t batch_end = command_index;
            while (batch_end < input.size()
                   && input[batch_end].enqueue_time == enqueue_time) {
                incremental.append(input[batch_end]);
                ++batch_end;
            }
            require(incremental.advance_to(enqueue_time)
                        == SimulationControl::Continue,
                    "serialqueue session stopped while advancing safe trace");
            command_index = batch_end;
        }
        const HostDispatchOutcome outcome = incremental.finish(
            session.toolhead().host_eventtime());
        require(outcome.control == SimulationControl::Continue
                    && outcome.commands_consumed == input.size(),
                "serialqueue session did not finish complete trace");
        require_same_dispatch_trace(batch, incremental.take_commands(), name);
    };
    HostDispatchConfig main_dispatch;
    main_dispatch.wire_frequency = cfg.mcu_baud;
    main_dispatch.clock_frequency = cfg.mcu_freq;
    main_dispatch.receive_window = cfg.mcu_receive_window;
    HostDispatchConfig nozzle_dispatch;
    nozzle_dispatch.wire_frequency = cfg.nozzle_baud;
    nozzle_dispatch.clock_frequency = cfg.mcu_freq;
    nozzle_dispatch.receive_window = cfg.nozzle_receive_window;
    compare_persistent(
        online.main_commands(), main_dispatch,
        [&](double eventtime) { return main_mcu.estimated_print_time(eventtime); },
        "mcu-persistent");
    compare_persistent(
        online.nozzle_commands(), nozzle_dispatch,
        [&](double eventtime) { return nozzle_mcu.estimated_print_time(eventtime); },
        "nozzle-persistent");
}

void test_analyze_online_path(const std::string& root)
{
    const std::vector<std::string> lines = read_lines(root + "/microsegments.gcode");
    SimConfig                      cfg;
    cfg.mcu_total    = 100000;
    cfg.nozzle_total = 100000;
    cfg.mcu_pool_total = 100010;
    cfg.nozzle_pool_total = 100010;
    LineAnalysisState       compact_state;
    LineAnalysisState       diagnostic_state;
    LineAnalysisDebug       compact_debug;
    LineAnalysisDebug       diagnostic_debug;
    const std::vector<char> compact    = analyze_gcode_lines(lines, cfg, 0.80, compact_state, &compact_debug, {}, true);
    const std::vector<char> diagnostic = analyze_gcode_lines(lines, cfg, 0.80, diagnostic_state, &diagnostic_debug, {}, false);
    require(compact == diagnostic, "analyze online path: compact and diagnostic flags differ");
    require(compact_debug.parsed_moves == 10 && compact_debug.xy_trap_moves == diagnostic_debug.xy_trap_moves &&
                compact_debug.e_trap_moves == diagnostic_debug.e_trap_moves,
            "analyze online path: production diagnostics differ");
}

void test_simulate_full_online_steppersync(const std::string& root)
{
    SimConfig cfg;
    const ParsedGCode parsed = parse_gcode_detailed(root + "/microsegments.gcode");
    const SimReport report = simulate_full(parsed, cfg);
    require(report.xy_cmds > 0 && report.e_cmds > 0,
            "simulate_full: online steppersync produced no motion commands");
    require(!report.mcu_overflow && !report.nozzle_overflow,
            "simulate_full: smoke input unexpectedly overflowed");
}

HostDispatchCmd dispatch_test_command(double req_time, int queue_id,
                                      int message_length,
                                      double enqueue_time = 0.0)
{
    HostDispatchCmd command;
    command.min_time = 0.0;
    command.req_time = req_time;
    command.msg_len = message_length;
    command.command_queue_id = queue_id;
    command.enqueue_time = enqueue_time;
    command.uses_move_slot = false;
    return command;
}

void test_host_dispatch_serialqueue()
{
    HostDispatchConfig config;
    config.wire_frequency = 230400.0;
    config.receive_window = 1000;

    // Queue 0 deliberately has a lower-priority command behind its head.
    // Firmware may compare queue heads only, so queue 1 wins the first block.
    std::vector<HostDispatchCmd> commands = {
        dispatch_test_command(10.0, 0, 30),
        dispatch_test_command(1.0, 0, 30),
        dispatch_test_command(5.0, 1, 30)
    };
    std::vector<HostDispatchCmd> empty;
    schedule_host_dispatch(commands, empty, {}, 0.0, 3.0, 10, 10,
                           [](double eventtime) { return eventtime; }, {},
                           config, config);
    require(commands[2].block_index == 0 && commands[0].block_index == 1
                && commands[1].block_index == 2,
            "serialqueue: command queue FIFO/head competition differs");
    for (const HostDispatchCmd& command : commands)
        require(command.block_index >= 0 && command.recv_time > 0.0
                    && command.ack_time > command.block_end_time,
                "serialqueue: command lacks send/receive/ack trace");

    std::vector<HostDispatchCmd> packed;
    for (int i = 0; i < 5; ++i)
        packed.push_back(dispatch_test_command(0.0, 0, 10));
    schedule_host_dispatch(packed, empty, {}, 0.0, 3.0, 10, 10,
                           [](double eventtime) { return eventtime; }, {},
                           config, config);
    for (const HostDispatchCmd& command : packed)
        require(command.block_index == 0,
                "serialqueue: 64-byte block packing differs");
    const double expected_end = 55.0 * 10.0 / config.wire_frequency;
    require(std::fabs(packed.front().block_end_time - expected_end) < 1e-12,
            "serialqueue: UART 8N1 wire time differs");

    HostDispatchConfig gated = config;
    gated.receive_window = 100;
    std::vector<HostDispatchCmd> gated_commands = {
        dispatch_test_command(0.0, 0, 50),
        dispatch_test_command(0.0, 0, 50)
    };
    schedule_host_dispatch(gated_commands, empty, {}, 0.0, 3.0, 10, 10,
                           [](double eventtime) { return eventtime; }, {},
                           gated, gated);
    require(gated_commands[1].send_time + 1e-12
                >= gated_commands[0].ack_time,
            "serialqueue: receive-window ACK gate was bypassed");

    std::vector<HostDispatchCmd> delayed = {
        dispatch_test_command(1.0, 0, 5, 1.0)
    };
    schedule_host_dispatch(delayed, empty, {}, 0.0, 3.0, 10, 10,
                           [](double eventtime) { return eventtime; }, {},
                           config, config);
    require(delayed.front().send_time >= 1.0,
            "serialqueue: command sent before serialqueue_send_batch");

    std::vector<HostDispatchCmd> unsorted = {
        dispatch_test_command(0.0, 0, 30, 2.0),
        dispatch_test_command(0.0, 0, 30, 0.0),
        dispatch_test_command(0.0, 0, 30, 1.0)
    };
    schedule_host_dispatch(unsorted, empty, {}, 0.0, 3.0, 10, 10,
                           [](double eventtime) { return eventtime; }, {},
                           config, config);
    require(unsorted[1].block_index == 0 && unsorted[2].block_index == 1
                && unsorted[0].block_index == 2,
            "serialqueue: unsorted enqueue fallback changed dispatch order");

}

void test_persistent_serialqueue_batches()
{
    HostDispatchConfig config;
    config.wire_frequency = 230400.0;
    config.receive_window = 1000;
    std::vector<HostDispatchCmd> source;
    for (int batch_index = 0; batch_index < 4; ++batch_index) {
        for (int i = 0; i < 3; ++i) {
            HostDispatchCmd command = dispatch_test_command(
                1.0 + batch_index + 0.1 * i, i & 1, 8 + i,
                static_cast<double>(batch_index));
            command.min_time = 0.25 * batch_index;
            command.line_idx = static_cast<int>(source.size());
            source.push_back(command);
        }
    }

    std::vector<HostDispatchCmd> batch = source;
    std::vector<HostDispatchCmd> empty;
    std::vector<int> batch_receive_order;
    schedule_host_dispatch(batch, empty, {}, 4.0, 3.0, 100, 100,
                           [](double eventtime) { return eventtime; }, {},
                           config, config,
                           [&](const HostDispatchCmd& command) {
                               batch_receive_order.push_back(command.line_idx);
                           });

    std::vector<int> incremental_receive_order;
    DeterministicSerialQueueSession incremental(
        config, [](double eventtime) { return eventtime; },
        [&](const HostDispatchCmd& command) {
            incremental_receive_order.push_back(command.line_idx);
            return SimulationControl::Continue;
        });
    size_t index = 0;
    for (int batch_index = 0; batch_index < 4; ++batch_index) {
        while (index < source.size()
               && source[index].enqueue_time == batch_index)
            incremental.append(source[index++]);
        require(incremental.advance_to(batch_index)
                    == SimulationControl::Continue,
                "persistent serialqueue stopped during append batch");
    }
    bool rejected_late_append = false;
    try {
        incremental.append(dispatch_test_command(4.0, 0, 8, 3.0));
    } catch (const std::logic_error&) {
        rejected_late_append = true;
    }
    require(rejected_late_append,
            "persistent serialqueue accepted an already-advanced batch");
    const HostDispatchOutcome outcome = incremental.finish(4.0);
    require(outcome.control == SimulationControl::Continue
                && outcome.commands_consumed == source.size(),
            "persistent serialqueue failed to finish append batches");
    require_same_dispatch_trace(batch, incremental.take_commands(),
                                "synthetic-persistent");
    require(batch_receive_order == incremental_receive_order,
            "persistent serialqueue changed MCU receive/allocation order");

    DeterministicSerialQueueSession nonretaining(
        config, [](double eventtime) { return eventtime; }, {}, false);
    index = 0;
    size_t max_retained = 0;
    for (int batch_index = 0; batch_index < 4; ++batch_index) {
        while (index < source.size()
               && source[index].enqueue_time == batch_index)
            nonretaining.append(source[index++]);
        require(nonretaining.advance_to(batch_index)
                    == SimulationControl::Continue,
                "non-retaining serialqueue stopped during append batch");
        max_retained = std::max(max_retained,
                                nonretaining.retained_command_count());
    }
    const HostDispatchOutcome nonretaining_outcome = nonretaining.finish(4.0);
    require(nonretaining_outcome.commands_consumed == source.size()
                && nonretaining.retained_command_count() == 0,
            "non-retaining serialqueue kept consumed command history");
    require(max_retained < source.size(),
            "non-retaining serialqueue retention grew to the full stream");
}

void test_host_dispatch_normal_early_stop()
{
    HostDispatchConfig config;
    config.wire_frequency = 230400.0;
    config.receive_window = 1000;
    std::vector<HostDispatchCmd> commands;
    for (int i = 0; i < 5; ++i)
        commands.push_back(dispatch_test_command(0.0, 0, 10));
    std::vector<HostDispatchCmd> empty;
    const DualHostDispatchOutcome outcome = schedule_host_dispatch_until(
        commands, empty, {}, 0.0, 3.0, 10, 10,
        [](double eventtime) { return eventtime; }, {}, config, config,
        [](const HostDispatchCmd&) {
            return SimulationControl::StopRiskDetected;
        });
    require(outcome.primary.control == SimulationControl::StopRiskDetected
                && outcome.primary.commands_consumed == 1,
            "serialqueue early stop was not a normal one-command outcome");
    require(commands.front().block_index == 0 && commands[1].block_index == -1,
            "serialqueue consumed commands after normal early stop");
}

void test_online_mcu_receive_and_pool_lifecycle()
{
    HostDispatchConfig config;
    config.wire_frequency = 230400.0;
    config.receive_window = 1000;

    std::vector<HostDispatchCmd> commands;
    for (int i = 0; i < 3; ++i) {
        HostDispatchCmd command = dispatch_test_command(0.0, 0, 10);
        command.uses_move_slot = true;
        command.line_idx = i;
        command.source_kind = HCS_XY_STEP;
        command.slot_free_time = 1.0;
        command.exec_start = 0.5;
        command.exec_end = 1.5;
        commands.push_back(command);
    }
    std::vector<HostDispatchCmd> empty;
    KlipperSteppersync consumer(2, 3.0);
    std::vector<int> receive_order;
    schedule_host_dispatch(
        commands, empty, {}, 0.0, 3.0, 2, 2,
        [](double eventtime) { return eventtime; }, {}, config, config,
        [&](const HostDispatchCmd& command) {
            receive_order.push_back(command.line_idx);
            consumer.consume_received(command);
        });

    require(receive_order == std::vector<int>({0, 1, 2}),
            "MCU online consume: receive order differs from serialqueue");
    require(consumer.command_count() == 3,
            "MCU online consume: command handler did not see every command");
    require(consumer.pool().alloc_count() == 2
                && consumer.pool().failed_alloc_count() == 1
                && consumer.overflow_events().size() == 1
                && consumer.overflow_events().front().line_idx == 2,
            "MCU online consume: first failed move_alloc provenance differs");

    KlipperMCU mcu("mcu", 100, 3.0, 120e6, 0.0, 7, 110);
    require(mcu.firmware_move_count() == 100
                && mcu.physical_move_count() == 110
                && mcu.host_move_slots() == 93
                && mcu.steppersync().total_slots() == 93
                && mcu.steppersync().pool().total() == 110,
            "MCU pool: advertised, host, and physical capacities were not separated");

    SimConfig machine;
    KlipperMCU configured_main("mcu", machine.mcu_total, machine.buffer_time_high,
                               machine.mcu_freq, 0.0, machine.mcu_reserved_slots,
                               machine.mcu_pool_total);
    KlipperMCU configured_nozzle("nozzle_mcu", machine.nozzle_total,
                                 machine.buffer_time_high, machine.mcu_freq, 0.0,
                                 machine.nozzle_reserved_slots,
                                 machine.nozzle_pool_total);
    require(configured_main.host_move_slots() == 2947
                && configured_main.steppersync().pool().total() == 2960
                && configured_nozzle.host_move_slots() == 1693
                && configured_nozzle.steppersync().pool().total() == 1710,
            "MCU pool: i7 host and physical capacities differ from MQDIAG");
}

void test_late_batch_envelope()
{
    HostDispatchConfig config;
    config.wire_frequency = 1000000000.0;
    config.receive_window = 1000;

    std::vector<HostDispatchCmd> commands;
    for (int i = 0; i < 5; ++i) {
        HostDispatchCmd command = dispatch_test_command(10.0 + i, 0, 5);
        command.uses_move_slot = true;
        command.source_kind = HCS_E_STEP;
        command.line_idx = 100 + i;
        command.batch_start_time = 10.0;
        command.enqueue_time = 1.0;
        command.slot_free_time = 11.0 + i;
        command.exec_start = command.slot_free_time;
        command.exec_end = command.slot_free_time + 0.1;
        commands.push_back(command);
    }
    HostDispatchCmd heater = dispatch_test_command(10.5, 1, 5);
    heater.uses_move_slot = true;
    heater.source_kind = HCS_AUX_HEATER;
    heater.slot_free_time = 20.0;
    commands.push_back(heater);
    // A smaller second batch must not replace the first batch's envelope.
    for (int i = 0; i < 2; ++i) {
        HostDispatchCmd command = dispatch_test_command(30.0 + i, 0, 5);
        command.uses_move_slot = true;
        command.source_kind = HCS_E_STEP;
        command.line_idx = 200 + i;
        command.batch_start_time = 30.0;
        command.enqueue_time = 2.0;
        command.slot_free_time = 31.0 + i;
        commands.push_back(command);
    }

    const LateBatchEnvelope envelope = analyze_late_batch_envelope(
        commands, 3, 10, config);
    require(envelope.host_step_peak == 3 && envelope.peak == 4
                && !envelope.overflow,
            "late batch: step slots or shared auxiliary pool semantics differ");
    require(envelope.source_batch_start == 10.0
                && envelope.first_line == 100
                && envelope.last_line == 104
                && envelope.move_commands == 5,
            "late batch: source batch provenance differs");
}

void test_online_trapq_release()
{
    SimConfig          cfg;
    std::vector<GMove> moves;
    moves.reserve(1400);
    double x = 0.0;
    for (int i = 0; i < 1400; ++i) {
        x += 0.05;
        moves.push_back({x, (i & 1) ? 0.01 : 0.0, 0.2, 0.002, 60.0, 5000.0, 2500.0, 5.0, 0.031, 0.040, i + 1});
    }

    KlipperMCU                  main_mcu("mcu", cfg.mcu_total, cfg.buffer_time_high, cfg.mcu_freq, 0.0,
                                         cfg.mcu_reserved_slots, cfg.mcu_pool_total);
    KlipperMCU                  nozzle_mcu("nozzle_mcu", cfg.nozzle_total, cfg.buffer_time_high,
                                           std::make_unique<KlipperSecondarySync>(main_mcu.clocksync(), cfg.mcu_freq), 0.0,
                                           cfg.nozzle_reserved_slots, cfg.nozzle_pool_total);
    KlipperOnlineStepGeneration online(cfg, main_mcu, nozzle_mcu, true);
    KlipperSimulationSession    session(cfg, {0.0, 0.0, 0.2, 0.0}, true, moves.size() + 1, {}, {},
                                        [&](const ToolheadFlushWindow& window, KlipperTrapQ& xy, KlipperTrapQ& e) {
                                         online.on_flush(window, xy, e);
                                     });
    for (const GMove& move : moves)
        session.consume_move(move);
    session.finish();
    online.finish(session.toolhead().last_kin_move_time(), session.xy_trapq(), session.e_trapq());
    require(online.total_xy_phases() > session.xy_trapq().moves().size() + 1024, "online trapq: completed XY history was not released");
    require(online.total_e_phases() > session.e_trapq().moves().size() + 1024, "online trapq: completed E history was not released");

    KlipperMCU               ref_main("mcu", cfg.mcu_total, cfg.buffer_time_high, cfg.mcu_freq, 0.0,
                                      cfg.mcu_reserved_slots, cfg.mcu_pool_total);
    KlipperMCU               ref_nozzle("nozzle_mcu", cfg.nozzle_total, cfg.buffer_time_high,
                                        std::make_unique<KlipperSecondarySync>(ref_main.clocksync(), cfg.mcu_freq), 0.0,
                                        cfg.nozzle_reserved_slots, cfg.nozzle_pool_total);
    KlipperSimulationSession reference(cfg, {0.0, 0.0, 0.2, 0.0}, true, moves.size() + 1);
    for (const GMove& move : moves)
        reference.consume_move(move);
    reference.finish();
    KlipperOnlineStepGeneration replay(cfg, ref_main, ref_nozzle, false);
    for (const ToolheadFlushWindow& window : reference.toolhead().flush_windows())
        replay.on_flush(window, reference.xy_trapq(), reference.e_trapq());
    replay.finish(reference.toolhead().last_kin_move_time(), reference.xy_trapq(), reference.e_trapq());

    require_same_commands(online.a_stepqueue().commands(), replay.a_stepqueue().commands(), "A-release");
    require_same_commands(online.b_stepqueue().commands(), replay.b_stepqueue().commands(), "B-release");
    require_same_commands(online.e_stepqueue().commands(), replay.e_stepqueue().commands(), "E-release");
}

void test_borrowed_line_probe_equivalence(const std::string& root)
{
    const std::vector<std::string> lines = read_lines(root + "/microsegments.gcode");
    std::vector<const std::string*> refs;
    refs.reserve(lines.size());
    for (const std::string& line : lines)
        refs.push_back(&line);

    SimConfig cfg;
    const OccResult owned = simulate_lines_occupancy(lines, cfg, false);
    const OccResult borrowed = simulate_line_refs_occupancy(refs, cfg, false);
    require(owned.mcu_frac == borrowed.mcu_frac
                && owned.nozzle_frac == borrowed.nozzle_frac
                && owned.print_time == borrowed.print_time
                && owned.last_positive_extrusion_cruise
                    == borrowed.last_positive_extrusion_cruise
                && owned.mcu_overflow == borrowed.mcu_overflow
                && owned.nozzle_overflow == borrowed.nozzle_overflow,
            "borrowed line probe changed simulation result");
}

} // namespace

int main(int argc, char** argv)
{
    try {
        const std::string root = argc > 1 ? argv[1] : "../src/libslic3r/GCode/KlipperSim/testdata/streaming_phase0";
        test_continuous(root);
        test_microsegments(root);
        test_arcs(root);
        test_barriers(root);
        test_aux(root);
        test_state_changes(root);
        test_creality_pa_enable_and_single_tool();
        test_initial_pa_command_gate();
        test_extruder_limits_and_pa_semantics();
        test_pressure_advance_flush_boundary();
        test_homing(root);
        test_g92_machine_coordinates();
        test_online_step_generation(root);
        test_analyze_online_path(root);
        test_simulate_full_online_steppersync(root);
        test_host_dispatch_serialqueue();
        test_persistent_serialqueue_batches();
        test_host_dispatch_normal_early_stop();
        test_online_mcu_receive_and_pool_lifecycle();
        test_late_batch_envelope();
        test_online_trapq_release();
        test_borrowed_line_probe_equivalence(root);
        std::cout << "streaming phase 0-8 tests: PASS\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "streaming phase 0-8 tests: FAIL: " << e.what() << '\n';
        return 1;
    }
}
