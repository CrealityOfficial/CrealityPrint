#include "KlipperSimulationSession.hpp"
#include "../PathologicalSegmentProbe.hpp"

#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace Slic3r;
using namespace Slic3r::KlipperSim;

namespace {

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

template<class F> void expect_error(F action, const char* message_fragment)
{
    try {
        action();
    } catch (const std::runtime_error& error) {
        require(std::string(error.what()).find(message_fragment) != std::string::npos,
                "unexpected simulation error");
        return;
    }
    throw std::runtime_error("invalid simulation unexpectedly succeeded");
}

PlanMove normal_move()
{
    PlanMove move;
    const double start[4] = {0, 0, 0, 0};
    const double end[4] = {10, 0, 0, 0};
    move.init(ToolheadLimits{}, start, end, 10.0);
    return move;
}

void test_invalid_planned_times()
{
    // Inject corrupted planned durations at the queue/host boundary, as seen
    // in the dump. A delayed move with zero lookahead deltas keeps these times.
    for (double invalid : {std::numeric_limits<double>::quiet_NaN(),
                           std::numeric_limits<double>::infinity()}) {
        for (int phase = 0; phase < 3; ++phase) {
            size_t processed = 0, flushed = 0;
            KlipperHostToolhead host({}, {},
                [&](const auto&, double) { ++processed; }, {},
                [&](const auto&) {
                    if (++flushed == 8)
                        throw std::runtime_error("regression guard: invalid time kept flushing");
                });
            PlanMove move;
            move.move_d = 1.0;
            move.line_idx = 4981;
            (phase == 0 ? move.accel_t : phase == 1 ? move.cruise_t : move.decel_t) = invalid;
            host.add_move(move);
            expect_error([&] { host.finish(); }, "G-code line index 4981");
            require(processed == 0 && flushed == 0 && host.flush_windows().empty(),
                    "invalid time reached trapq or allocated flush windows");
        }
    }

    KlipperHostToolhead host({}, {}, {});
    PlanMove overflow;
    overflow.move_d = 1.0;
    overflow.accel_t = overflow.cruise_t = std::numeric_limits<double>::max();
    host.add_move(overflow);
    expect_error([&] { host.finish(); }, "accumulated move time");
    require(host.flush_windows().empty(), "overflow allocated flush windows");
}

void test_invalid_inputs()
{
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    for (double invalid : {nan, inf, 0.0, -1.0}) {
        PlanMove move;
        const double start[4] = {0, 0, 0, 0}, end[4] = {1, 0, 0, 0};
        expect_error([&] { move.init({}, start, end, invalid); }, "speed or acceleration");
        ToolheadLimits limits;
        limits.max_accel = invalid;
        expect_error([&] { move.init(limits, start, end, 10.0); }, "speed or acceleration");

        ToolheadTiming timing;
        timing.move_batch_time = invalid;
        size_t processed = 0;
        KlipperHostToolhead host({}, timing, [&](const auto&, double) { ++processed; });
        host.add_move(normal_move());
        expect_error([&] { host.finish(); }, "invalid toolhead flush time");
        require(processed == 0 && host.flush_windows().empty(), "invalid cadence published motion");
    }
    for (double invalid : {nan, inf}) {
        PlanMove move = normal_move();
        expect_error([&] { move.set_junction(0, invalid, 0); }, "junction");
        KlipperHostToolhead host({}, {}, {}, [=](double) { return invalid; });
        host.add_move(normal_move());
        expect_error([&] { host.finish(); }, "time");
        require(host.flush_windows().empty(), "invalid clock allocated windows");
    }
    PlanMove stopped = normal_move();
    expect_error([&] { stopped.set_junction(0, 0, 0); }, "junction");
}

void test_bounded_flush()
{
    size_t processed = 0;
    KlipperHostToolhead host({}, {}, [&](const auto&, double) { ++processed; });
    PlanMove enormous;
    enormous.move_d = 1.0;
    enormous.cruise_t = 1e12;
    host.add_move(enormous);
    expect_error([&] { host.finish(); }, "flush window limit");
    require(processed == 0 && host.flush_windows().empty(), "huge span was processed before rejection");

    // A representable target can still be unreachable when adding batch_time
    // to a very large current time rounds back to that same time.
    KlipperHostToolhead stalled({}, {}, {});
    KlipperHostToolhead::StateSnapshot state;
    state.print_time = 1e16;
    stalled.restore_state(state);
    PlanMove move;
    move.move_d = 1.0;
    move.cruise_t = 2.0;
    stalled.add_move(move);
    expect_error([&] { stalled.finish(); }, "cannot advance");
    require(stalled.flush_windows().empty(), "stalled clock allocated windows");
}

void test_normal_and_zero_length_moves()
{
    size_t processed = 0;
    double duration = 0;
    KlipperHostToolhead host({}, {}, [&](const auto& moves, double) {
        processed += moves.size();
        for (const auto& m : moves)
            duration += m.accel_t + m.cruise_t + m.decel_t;
    });
    PlanMove empty;
    const double position[4] = {0, 0, 0, 0};
    empty.init({}, position, position, 10.0);
    host.add_move(empty);
    host.add_move(normal_move());
    host.add_move(empty);
    host.finish();
    require(processed == 1 && std::fabs(duration - 1.001) < 1e-9,
            "normal trapezoid changed or zero-length move was published");
    const size_t count = host.flush_windows().size();
    require(count > 0 && count < 10, "normal move has unexpected flush count");
    host.finish();
    require(host.flush_windows().size() == count, "repeated finish appended history");
}

void test_probe_error_propagation()
{
    const std::vector<std::string> lines = {"G90", "G1 X0 Y0 Z0.2 F6000",
                                           "G1 Xnan Y1 E0.1", "M400"};
    const auto sync = PathologicalSegmentProbe::probe_lines(lines);
    require(sync.status == PathologicalProbeStatus::SimulationError,
            "invalid G-code was reported safe by synchronous probe");
    require(sync.error_message.find("line index 2") != std::string::npos,
            "simulation failure lost the source line");

    PathologicalLineProbe probe;
    for (const auto& line : lines)
        probe.consume_line(line);
    probe.finish_input();
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    const auto result = probe.wait_result([&] {
        if (std::chrono::steady_clock::now() > deadline)
            throw std::runtime_error("probe did not terminate on invalid motion");
    });
    require(result.status == PathologicalProbeStatus::SimulationError,
            "invalid G-code was reported safe by background probe");
    require(result.error_message.find("line index 2") != std::string::npos,
            "background simulation failure lost the source line");
}

} // namespace

int main()
{
    try {
        test_invalid_planned_times();
        test_invalid_inputs();
        test_bounded_flush();
        test_normal_and_zero_length_moves();
        test_probe_error_propagation();
        std::cout << "Klipper toolhead safety tests: PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Klipper toolhead safety tests: FAIL: " << error.what() << '\n';
        return 1;
    }
}
