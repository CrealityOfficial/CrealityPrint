#ifndef slic3r_GCode_KlipperSim_KlipperItersolve_hpp_
#define slic3r_GCode_KlipperSim_KlipperItersolve_hpp_

// ---------------------------------------------------------------------------
// KlipperItersolve  (port of firmware itersolve.c)
// ---------------------------------------------------------------------------
// The iterative (secant + bisection) solver that converts a trapezoid move
// stream into exact step crossing times for ONE stepper. Each stepper defines
// how a move's XYZ coordinate maps to its own axis position:
//   CoreXY '+' : pos = x + y      (motor A)
//   CoreXY '-' : pos = x - y      (motor B)
//   Extruder   : pos = distance along move (E)
// The solver finds every time the stepper position crosses a half-step
// boundary and emits that time. This is the SAME algorithm the firmware uses,
// so the produced step timing (and thus stepcompress output) matches exactly.
// ---------------------------------------------------------------------------

#include <functional>
#include <vector>
#include "KlipperTrapQ.hpp"
#include "KlipperStepCompress.hpp"

namespace Slic3r {
namespace KlipperSim {

// Position callback: given a move and a time within it, return this stepper's
// scalar axis position (mm).
using CalcPositionCb = std::function<double(const TrapMove& m, double move_time)>;

class KlipperItersolve
{
public:
    enum ActiveFlags {
        AF_X = 1 << 0,
        AF_Y = 1 << 1,
        AF_Z = 1 << 2,
    };

    KlipperItersolve(CalcPositionCb cb, double step_dist, int active_flags = 0)
        : m_calc(std::move(cb)), m_step_dist(step_dist), m_active_flags(active_flags) {}

    double commanded_position() const { return m_commanded_pos; }
    double step_dist() const { return m_step_dist; }
    void set_step_dist(double step_dist) { m_step_dist = step_dist; }
    void set_position(double x, double y, double z);
    double calc_position_from_coord(double x, double y, double z) const;
    bool is_active_axis(char axis) const;
    void reset_stream()
    {
        m_last_flush_time = 0.0;
        m_last_move_time = 0.0;
        m_next_move_idx = 0;
        m_stream_seeded = false;
    }
    void set_active_window(double pre_active, double post_active)
    {
        m_gen_steps_pre_active = pre_active;
        m_gen_steps_post_active = post_active;
    }
    void discard_phase_prefix(size_t count)
    {
        m_next_move_idx = count >= m_next_move_idx ? 0 : m_next_move_idx - count;
    }

    // Generate step times (seconds, absolute) for all moves in tq that affect
    // this stepper. Returns absolute step clock times in seconds.
    // active_pred: whether a move produces motion on this stepper (axes check).
    std::vector<double> generate_steps(const KlipperTrapQ& tq,
                                       const std::function<bool(const TrapMove&)>& active_pred);
    size_t generate_stepcompress(const KlipperTrapQ& tq,
                                 const std::function<bool(const TrapMove&)>& active_pred,
                                 KlipperStepCompress& sc, double mcu_freq);
    size_t generate_stepcompress_to(const KlipperTrapQ& tq, double flush_time,
                                    const std::function<bool(const TrapMove&)>& active_pred,
                                    KlipperStepCompress& sc, double mcu_freq);

    // Extruder-with-pressure-advance variant. The position of a phase depends
    // on neighboring phases (smooth-time window), so the callback receives the
    // whole phase vector + current index. Generates E steps over all phases.
    std::vector<double> generate_steps_indexed(
        const std::vector<TrapMove>& moves,
        const std::function<double(const std::vector<TrapMove>&, size_t, double)>& pos_cb,
        const std::function<bool(const TrapMove&)>& active_pred);
    size_t generate_stepcompress_indexed(
        const std::vector<TrapMove>& moves,
        const std::function<double(const std::vector<TrapMove>&, size_t, double)>& pos_cb,
        const std::function<bool(const TrapMove&)>& active_pred,
        KlipperStepCompress& sc, double mcu_freq);
    size_t generate_stepcompress_indexed_to(
        const std::vector<TrapMove>& moves, double flush_time,
        const std::function<double(const std::vector<TrapMove>&, size_t, double)>& pos_cb,
        const std::function<bool(const TrapMove&)>& active_pred,
        KlipperStepCompress& sc, double mcu_freq);

private:
    CalcPositionCb m_calc;
    double m_step_dist{ 1.0 };
    int    m_active_flags{ 0 };
    double m_commanded_pos{ 0.0 }; // current known stepper position (mm)
    int    m_sdir{ 0 };            // current step direction (1=+, 0=-)
    double m_last_flush_time{ 0.0 };
    double m_last_move_time{ 0.0 };
    double m_gen_steps_pre_active{ 0.0 };
    double m_gen_steps_post_active{ 0.0 };
    size_t m_next_move_idx{ 0 };
    bool   m_stream_seeded{ false };

    struct TimePos { double time, position; };

    // Port of itersolve_gen_steps_range for a single move phase.
    void gen_steps_range(const TrapMove& m, double abs_start, double abs_end,
                         std::vector<double>& out_steps);
    void gen_steps_range_sc(const TrapMove& m, double abs_start, double abs_end,
                            KlipperStepCompress& sc, double mcu_freq,
                            size_t& out_count);
    bool check_active(const TrapMove& m,
                      const std::function<bool(const TrapMove&)>& active_pred) const
    {
        return active_pred(m);
    }

    void gen_steps_range_indexed(
        const std::vector<TrapMove>& moves, size_t mi,
        double abs_start, double abs_end,
        const std::function<double(const std::vector<TrapMove>&, size_t, double)>& pos_cb,
        std::vector<double>& out_steps);
    void gen_steps_range_indexed_sc(
        const std::vector<TrapMove>& moves, size_t mi,
        double abs_start, double abs_end,
        const std::function<double(const std::vector<TrapMove>&, size_t, double)>& pos_cb,
        KlipperStepCompress& sc, double mcu_freq,
        size_t& out_count);
};

}} // namespace Slic3r::KlipperSim

#endif
