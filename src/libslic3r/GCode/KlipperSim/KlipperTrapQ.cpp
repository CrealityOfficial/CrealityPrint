#include "KlipperTrapQ.hpp"

namespace Slic3r {
namespace KlipperSim {

size_t KlipperTrapQ::release_before(double print_time, size_t min_prefix)
{
    size_t count = 0;
    while (count < m_moves.size()
           && m_moves[count].print_time + m_moves[count].move_t <= print_time)
        ++count;
    if (count < min_prefix)
        return 0;
    m_moves.erase(m_moves.begin(),
                  m_moves.begin() + static_cast<ptrdiff_t>(count));
    return count;
}
// Port of firmware trapq_append(): split a move into accel/cruise/decel phases,
// each a separate TrapMove with constant half_accel. Chains start_pos through.
void KlipperTrapQ::append(double print_time,
                          double batch_start_time,
                          double accel_t, double cruise_t, double decel_t,
                          const Coord& start_pos_in, const Coord& axes_r,
                          double start_v, double cruise_v, double accel,
                          int line_idx,
                          double pressure_advance,
                          double pa_smooth_time)
{
    Coord start_pos = start_pos_in;

    if (accel_t) {
        TrapMove m;
        m.print_time = print_time;
        m.batch_start_time = batch_start_time;
        m.move_t     = accel_t;
        m.start_v    = start_v;
        m.half_accel = 0.5 * accel;
        m.start_pos  = start_pos;
        m.axes_r     = axes_r;
        m.line_idx   = line_idx;
        m.pressure_advance = pressure_advance;
        m.pa_smooth_time = pa_smooth_time;
        m_moves.push_back(m);

        print_time += accel_t;
        start_pos   = m.get_coord(accel_t);
    }
    if (cruise_t) {
        TrapMove m;
        m.print_time = print_time;
        m.batch_start_time = batch_start_time;
        m.move_t     = cruise_t;
        m.start_v    = cruise_v;
        m.half_accel = 0.0;
        m.start_pos  = start_pos;
        m.axes_r     = axes_r;
        m.line_idx   = line_idx;
        m.pressure_advance = pressure_advance;
        m.pa_smooth_time = pa_smooth_time;
        m_moves.push_back(m);

        print_time += cruise_t;
        start_pos   = m.get_coord(cruise_t);
    }
    if (decel_t) {
        TrapMove m;
        m.print_time = print_time;
        m.batch_start_time = batch_start_time;
        m.move_t     = decel_t;
        m.start_v    = cruise_v;
        m.half_accel = -0.5 * accel;
        m.start_pos  = start_pos;
        m.axes_r     = axes_r;
        m.line_idx   = line_idx;
        m.pressure_advance = pressure_advance;
        m.pa_smooth_time = pa_smooth_time;
        m_moves.push_back(m);
    }
}

}} // namespace Slic3r::KlipperSim
