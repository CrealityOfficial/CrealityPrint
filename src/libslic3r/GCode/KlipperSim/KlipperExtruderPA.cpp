#include "KlipperExtruderPA.hpp"

#include <algorithm>

namespace Slic3r {
namespace KlipperSim {

// position(t) = base + t*(start_v + t*half_accel) integrated over [start,end]
double KlipperExtruderPA::integrate(double base, double start_v, double half_accel,
                                    double start, double end)
{
    double half_v = 0.5*start_v, sixth_a = (1.0/3.0)*half_accel;
    double si = start * (base + start * (half_v + start * sixth_a));
    double ei = end   * (base + end   * (half_v + end   * sixth_a));
    return ei - si;
}

double KlipperExtruderPA::integrate_time(double base, double start_v, double half_accel,
                                         double start, double end)
{
    double half_b = 0.5*base, third_v = (1.0/3.0)*start_v;
    double eighth_a = 0.25*half_accel;
    double si = start*start * (half_b + start * (third_v + start * eighth_a));
    double ei = end*end     * (half_b + end   * (third_v + end   * eighth_a));
    return ei - si;
}

// pa_move_integrate (1:1). PA only when move has XY motion: encoded in
// m.axes_r.y != 0 (we set that flag when building the E trapq).
double KlipperExtruderPA::move_integrate(const TrapMove& m, double base,
                                         double start, double end,
                                         double time_offset) const
{
    if (start < 0.0) start = 0.0;
    if (end > m.move_t) end = m.move_t;
    double pa = m_pa;
    int can_pa = (m.axes_r.y != 0.0);
    if (!can_pa) pa = 0.0;
    // base += pa * start_v ; start_v += pa * 2*half_accel
    double b = base + pa * m.start_v;
    double start_v = m.start_v + pa * 2.0 * m.half_accel;
    double ha = m.half_accel;
    double iext = integrate(b, start_v, ha, start, end);
    double wgt_ext = integrate_time(b, start_v, ha, start, end);
    return wgt_ext - time_offset * iext;
}

// pa_range_integrate (1:1): integrate over [move_time-hst, move_time+hst]
// spanning neighbor phases.
double KlipperExtruderPA::range_integrate(const std::vector<TrapMove>& moves,
                                          size_t mi, double move_time) const
{
    double hst = m_hst;
    const TrapMove& m0 = moves[mi];
    double start = move_time - hst, end = move_time + hst;
    double start_base = m0.start_pos.x;
    double res = 0.0;
    // current move
    res += move_integrate(m0, 0.0, start, move_time, start);
    res -= move_integrate(m0, 0.0, move_time, end, end);
    // previous moves (start < 0)
    {
        size_t idx = mi;
        double s = start;
        while (s < 0.0 && idx > 0) {
            --idx;
            const TrapMove& pm = moves[idx];
            s += pm.move_t;
            double base = pm.start_pos.x - start_base;
            res += move_integrate(pm, base, s, pm.move_t, s);
        }
    }
    // future moves (end > move_t)
    {
        size_t idx = mi;
        double e = end;
        double mt = m0.move_t;
        while (e > mt && idx + 1 < moves.size()) {
            e -= mt;
            ++idx;
            const TrapMove& nm = moves[idx];
            double base = nm.start_pos.x - start_base;
            res -= move_integrate(nm, base, 0.0, e, e);
            mt = nm.move_t;
        }
    }
    return res;
}

double KlipperExtruderPA::calc_position(const std::vector<TrapMove>& moves,
                                        size_t mi, double move_time) const
{
    const TrapMove& m = moves[mi];
    if (m_hst <= 0.0)
        // No pressure advance: nominal position = base + distance
        return m.start_pos.x + m.get_distance(move_time);
    double area = range_integrate(moves, mi, move_time);
    return m.start_pos.x + area * m_inv_hst2;
}

}} // namespace Slic3r::KlipperSim
