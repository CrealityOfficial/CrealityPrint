#ifndef slic3r_GCode_KlipperSim_KlipperTrapQ_hpp_
#define slic3r_GCode_KlipperSim_KlipperTrapQ_hpp_

// ---------------------------------------------------------------------------
// KlipperTrapQ  (port of firmware trapq.c)
// ---------------------------------------------------------------------------
// A trapezoid-velocity move: a kinematic segment split into up to three phases
// (accel / cruise / decel). Klipper stores each phase as a separate `move`
// with constant half_accel. Position within a phase is:
//     dist(t)  = (start_v + half_accel * t) * t
//     coord(t) = start_pos + axes_r * dist(t)
//
// itersolve queries coord(t) to find step crossings; the extruder kinematic
// queries distance(t). We port the exact same math so step timing matches.
// ---------------------------------------------------------------------------

#include <vector>

namespace Slic3r {
namespace KlipperSim {

struct Coord { double x, y, z; };

// One trapezoid phase (firmware struct move). Times are in seconds, positions
// in mm, velocities mm/s, accel mm/s^2 (half_accel = 0.5*accel).
struct TrapMove
{
    double print_time{ 0.0 }; // absolute start time of this phase (s)
    double batch_start_time{ 0.0 }; // flush-batch start time that emitted this phase
    double move_t{ 0.0 };     // duration of this phase (s)
    double start_v{ 0.0 };    // velocity at phase start (mm/s)
    double half_accel{ 0.0 }; // 0.5 * accel (signed: +accel, 0 cruise, -decel)
    Coord  start_pos{ 0, 0, 0 };
    Coord  axes_r{ 1, 0, 0 };  // unit direction
    int    line_idx{ -1 };     // source gcode line index (for filter mapping)
    // PA state belongs to the move, so E itersolve never has to search G-code.
    double pressure_advance{ 0.0 };
    double pa_smooth_time{ 0.040 };

    // dist(t) within this phase
    double get_distance(double t) const { return (start_v + half_accel * t) * t; }
    Coord  get_coord(double t) const
    {
        double d = get_distance(t);
        return Coord{ start_pos.x + axes_r.x * d,
                      start_pos.y + axes_r.y * d,
                      start_pos.z + axes_r.z * d };
    }
};

// A queue of trapezoid phases in execution order.
class KlipperTrapQ
{
public:
    void clear() { m_moves.clear(); }

    // Append a full kinematic move (accel/cruise/decel) — mirrors trapq_append.
    // start_pos: position at move start. axes_r: unit direction of travel.
    void append(double print_time,
                double batch_start_time,
                double accel_t, double cruise_t, double decel_t,
                const Coord& start_pos, const Coord& axes_r,
                double start_v, double cruise_v, double accel,
                int line_idx = -1,
                double pressure_advance = 0.0,
                double pa_smooth_time = 0.040);

    const std::vector<TrapMove>& moves() const { return m_moves; }
    size_t release_before(double print_time, size_t min_prefix = 1024);
    void release_storage() { std::vector<TrapMove>().swap(m_moves); }

private:
    std::vector<TrapMove> m_moves;
};

}} // namespace Slic3r::KlipperSim

#endif
