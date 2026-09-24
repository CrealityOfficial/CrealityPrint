#ifndef slic3r_GCode_KlipperSim_KlipperExtruderPA_hpp_
#define slic3r_GCode_KlipperSim_KlipperExtruderPA_hpp_

// ---------------------------------------------------------------------------
// KlipperExtruderPA  (1:1 port of firmware kin_extruder.c pressure advance)
// ---------------------------------------------------------------------------
// Extruder stepper position with pressure advance + smooth-time averaging.
// PA pushes extra filament during acceleration and retracts during
// deceleration -> in accel/decel-heavy (dense micro-segment) regions this
// greatly increases E step count and thus queue_step command count.
//
//   pa_position(t) = nominal_position(t) + pressure_advance * nominal_velocity(t)
//   smooth_position(t) = weighted average of pa_position over [t-hst, t+hst]
//
// Operates over a flat list of trapezoid phases (TrapMove) representing the E
// motion timeline. Each phase: base = start_pos.x, start_v, half_accel.
// ---------------------------------------------------------------------------

#include <vector>
#include "KlipperTrapQ.hpp"

namespace Slic3r {
namespace KlipperSim {

class KlipperExtruderPA
{
public:
    // pressure_advance (s), smooth_time (s). If smooth_time<=0 -> no PA.
    KlipperExtruderPA(double pressure_advance, double smooth_time)
        : m_pa(pressure_advance), m_hst(0.5*smooth_time)
    {
        if (m_hst > 0.0) m_inv_hst2 = 1.0 / (m_hst * m_hst);
    }

    double half_smooth_time() const { return m_hst; }

    // E position at absolute time within move index `mi` at local move_time.
    // Needs the full phase list to integrate across neighbors.
    double calc_position(const std::vector<TrapMove>& moves, size_t mi,
                         double move_time) const;

private:
    double m_pa;
    double m_hst;
    double m_inv_hst2 = 0.0;

    // integrate helpers (1:1 firmware)
    static double integrate(double base, double start_v, double half_accel,
                            double start, double end);
    static double integrate_time(double base, double start_v, double half_accel,
                                 double start, double end);
    double move_integrate(const TrapMove& m, double base, double start,
                          double end, double time_offset) const;
    double range_integrate(const std::vector<TrapMove>& moves, size_t mi,
                           double move_time) const;
};

}} // namespace Slic3r::KlipperSim

#endif
