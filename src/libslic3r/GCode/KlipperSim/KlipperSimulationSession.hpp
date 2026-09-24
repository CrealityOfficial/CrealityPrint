#ifndef slic3r_GCode_KlipperSim_KlipperSimulationSession_hpp_
#define slic3r_GCode_KlipperSim_KlipperSimulationSession_hpp_

#include "KlipperSimMain.hpp"
#include "KlipperTrapQ.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

namespace Slic3r {
namespace KlipperSim {

enum class SimulationBarrierKind {
    StepGenerationFlush,
    Synchronize,
    Dwell,
    Homing,
    TemperatureWait,
    PauseOrToolChange,
    UnknownExecutable
};

struct SimulationBarrier {
    SimulationBarrierKind kind = SimulationBarrierKind::Synchronize;
    int line_idx = -1;
    double dwell_seconds = 0.0;
    double step_generation_scan_time = 0.0;
    bool invalidates_position = false;
};

using SessionStepGenerationCb = std::function<void(
    const ToolheadFlushWindow&, KlipperTrapQ&, KlipperTrapQ&)>;
using SessionPlannedBatchCb = std::function<void(
    const std::vector<PlanMove>&, double)>;

class KlipperSimulationSession
{
public:
    KlipperSimulationSession(const SimConfig& cfg,
                             const std::array<double, 4>& start_position,
                             bool have_position,
                             size_t line_count,
                             EstimatedPrintTimeCb estimated_print_time_cb = {},
                             ToolheadFlushCb flush_cb = {},
                             SessionStepGenerationCb step_generation_cb = {},
                             bool capture_planned_moves = false,
                             std::vector<double>* positive_extrusion_cruise_by_line = nullptr,
                             std::vector<double>* positive_extrusion_distance_by_line = nullptr,
                             double* last_positive_extrusion_cruise = nullptr,
                             SessionPlannedBatchCb planned_batch_cb = {});
    ~KlipperSimulationSession();

    KlipperSimulationSession(const KlipperSimulationSession&) = delete;
    KlipperSimulationSession& operator=(const KlipperSimulationSession&) = delete;

    void consume_move(const GMove& move);
    void set_position(const std::array<double, 4>& position, bool have_position = true);
    void consume_barrier(const SimulationBarrier& barrier);
    void finish();

    KlipperTrapQ& xy_trapq();
    KlipperTrapQ& e_trapq();
    const KlipperTrapQ& xy_trapq() const;
    const KlipperTrapQ& e_trapq() const;
    KlipperHostToolhead& toolhead();
    const KlipperHostToolhead& toolhead() const;
    const std::vector<double>& line_start_times() const;
    const std::vector<double>& line_end_times() const;
    const std::vector<PlanMove>& planned_moves() const;
    size_t move_count() const;
    size_t barrier_count() const;
    size_t process_batch_count() const;
    size_t max_process_batch_moves() const;
    size_t max_process_batch_short_moves() const;
    double max_process_batch_span() const;
    double max_process_batch_start() const;
    int max_process_batch_first_line() const;
    int max_process_batch_last_line() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}} // namespace Slic3r::KlipperSim

#endif
