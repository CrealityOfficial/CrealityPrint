#ifndef slic3r_GCode_KlipperSim_KlipperToolhead_hpp_
#define slic3r_GCode_KlipperSim_KlipperToolhead_hpp_

// ---------------------------------------------------------------------------
// KlipperToolhead  (1:1 port of firmware toolhead.py Move + MoveQueue)
// ---------------------------------------------------------------------------
// Reproduces the host-side motion planner lookahead exactly:
//   - Move: junction speed limits (calc_junction), trapezoid split (set_junction)
//   - MoveQueue: reverse-pass lookahead flush (lazy + depth cap)
// Output: for each flushed move, its accel/cruise/decel phases (start_v/cruise_v
// /end_v, times) — fed to the trapq for step generation.
//
// This is the SAME code path Klipper runs; velocities/timings match firmware,
// which is what earlier approximations got wrong.
// ---------------------------------------------------------------------------

#include <algorithm>
#include <vector>
#include <functional>
#include <cmath>

namespace Slic3r {
namespace KlipperSim {

struct ToolheadLimits
{
    double max_velocity   = 500.0;
    double max_accel      = 10000.0;
    double max_accel_to_decel = 5000.0;
    double square_corner_velocity = 8.0;
    double junction_deviation = 0.002650;
    // extruder instantaneous corner velocity (for E junction limit)
    double instant_corner_v = 1.0;
};

struct ToolheadTiming
{
    double buffer_time_high = 3.000;
    double buffer_time_start = 0.250;
    double move_flush_time = 0.050;
    double kin_flush_delay = 0.001;
    double move_batch_time = 0.500;
};

// One planner move (mirrors firmware Move).
struct PlanMove
{
    double start_pos[4]{0,0,0,0};
    double end_pos[4]{0,0,0,0};
    double axes_d[4]{0,0,0,0};
    double axes_r[4]{0,0,0,0};
    double move_d = 0.0;
    double accel = 0.0;
    double junction_deviation = 0.0;
    bool   is_kinematic_move = true;
    int    line_idx = -1;      // source gcode line index
    // Runtime PA state captured when this G-code move is parsed.
    double pressure_advance = 0.0;
    double pa_smooth_time = 0.040;

    double min_move_t = 0.0;
    double max_start_v2 = 0.0;
    double max_cruise_v2 = 0.0;
    double delta_v2 = 0.0;
    double max_smoothed_v2 = 0.0;
    double smooth_delta_v2 = 0.0;

    // set_junction output (trapezoid)
    double start_v = 0.0, cruise_v = 0.0, end_v = 0.0;
    double accel_t = 0.0, cruise_t = 0.0, decel_t = 0.0;

    void init(const ToolheadLimits& lim, const double sp[4], const double ep[4], double speed);
    void calc_junction(const PlanMove& prev, const ToolheadLimits& lim);
    void set_junction(double start_v2, double cruise_v2, double end_v2);
};

// Callback receiving a batch of fully-planned moves ready to flush.
using ProcessMovesCb = std::function<void(const std::vector<PlanMove>&)>;
using HostProcessMovesCb = std::function<void(const std::vector<PlanMove>&,
                                              double batch_start_time)>;
using EstimatedPrintTimeCb = std::function<double(double)>;

struct ToolheadFlushWindow
{
    double print_time = 0.0;
    double estimated_print_time = 0.0;
    double sg_flush_time = 0.0;
    double free_time = 0.0;
    double mcu_flush_time = 0.0;
    double host_eventtime = 0.0;
};

using ToolheadFlushCb = std::function<void(const ToolheadFlushWindow&)>;

class KlipperMoveQueue
{
public:
    KlipperMoveQueue(const ToolheadLimits& lim, ProcessMovesCb cb)
        : m_lim(lim), m_process(std::move(cb)) {}

    void add_move(const PlanMove& m);
    void flush(bool lazy);
    void finish() { flush(false); } // final drain
    bool empty() const { return m_queue.empty(); }
    void update_limits(const ToolheadLimits& lim) { m_lim = lim; }
    void set_flush_time(double flush_time) { m_junction_flush = flush_time; }

private:
    ToolheadLimits m_lim;
    ProcessMovesCb m_process;
    std::vector<PlanMove> m_queue;
    double m_junction_flush = 0.250; // LOOKAHEAD_FLUSH_TIME
};

// Offline host-side toolhead scheduler that mirrors the main timing chain in
// toolhead.py: MoveQueue.flush() -> _process_moves() -> _update_move_time().
// This isolates print_time / batch flush cadence from KlipperSimMain so the
// later 1:1 mcu.py port has a stable integration point.
class KlipperHostToolhead
{
public:
    struct StateSnapshot
    {
        double print_time = 0.0;
        double host_eventtime = 0.0;
        double estimated_print_time = 0.0;
        double last_kin_flush_time = 0.0;
        double last_kin_move_time = 0.0;
    };

    KlipperHostToolhead(const ToolheadLimits& lim,
                        const ToolheadTiming& timing,
                        HostProcessMovesCb cb,
                        EstimatedPrintTimeCb estimated_print_time_cb = {},
                        ToolheadFlushCb flush_cb = {});

    void update_limits(const ToolheadLimits& lim)
    {
        m_limits = lim;
        m_move_queue.update_limits(lim);
    }

    void add_move(const PlanMove& move) { m_move_queue.add_move(move); }
    void finish();
    void note_step_generation_scan_time(double delay)
    {
        // toolhead.py flushes with the old scan delay before installing the
        // new one.
        m_move_queue.flush(false);
        flush_step_generation();
        m_timing.kin_flush_delay = std::max(0.001, delay);
    }
    void synchronize(double dwell_seconds = 0.0)
    {
        finish();
        double estimated_print_time = m_estimated_print_time;
        if (m_estimated_print_time_cb)
            estimated_print_time = m_estimated_print_time_cb(m_host_eventtime);
        if (m_print_time > estimated_print_time)
            m_host_eventtime += m_print_time - estimated_print_time;
        if (dwell_seconds > 0.0)
            m_host_eventtime += dwell_seconds;
        if (m_estimated_print_time_cb)
            m_estimated_print_time = m_estimated_print_time_cb(m_host_eventtime);
        else
            m_estimated_print_time = m_host_eventtime;
    }
    void restore_state(const StateSnapshot& state);
    StateSnapshot snapshot_state() const;

    double print_time() const { return m_print_time; }
    double host_eventtime() const { return m_host_eventtime; }
    double last_kin_move_time() const { return m_last_kin_move_time; }
    double last_kin_flush_time() const { return m_last_kin_flush_time; }

    const std::vector<ToolheadFlushWindow>& flush_windows() const
    {
        return m_flush_windows;
    }

private:
    enum class QueueState { Flushed, Main };

    void calc_print_time();
    void process_moves(const std::vector<PlanMove>& moves);
    void update_move_time(double next_print_time);
    void flush_step_generation();

    ToolheadLimits m_limits;
    ToolheadTiming m_timing;
    HostProcessMovesCb m_process;
    EstimatedPrintTimeCb m_estimated_print_time_cb;
    ToolheadFlushCb m_flush_cb;
    KlipperMoveQueue m_move_queue;
    QueueState m_state = QueueState::Flushed;
    double m_print_time = 0.0;
    double m_host_eventtime = 0.0;
    double m_estimated_print_time = 0.0;
    double m_last_kin_flush_time = 0.0;
    double m_last_kin_move_time = 0.0;
    std::vector<ToolheadFlushWindow> m_flush_windows;
};

}} // namespace Slic3r::KlipperSim

#endif
