#include "KlipperToolhead.hpp"

#include <algorithm>
#include <cmath>

namespace Slic3r {
namespace KlipperSim {

static constexpr double LOOKAHEAD_FLUSH_TIME = 0.250;

// --- Move.__init__ (1:1) ---
void PlanMove::init(const ToolheadLimits& lim, const double sp[4], const double ep[4], double speed)
{
    for (int i = 0; i < 4; ++i) { start_pos[i]=sp[i]; end_pos[i]=ep[i]; }
    accel = lim.max_accel;
    junction_deviation = lim.junction_deviation;
    double velocity = std::min(speed, lim.max_velocity);
    is_kinematic_move = true;
    for (int i = 0; i < 4; ++i) axes_d[i] = ep[i] - sp[i];
    move_d = std::sqrt(axes_d[0]*axes_d[0] + axes_d[1]*axes_d[1] + axes_d[2]*axes_d[2]);
    double inv_move_d;
    if (move_d < 0.000000001) {
        // extrude-only move
        end_pos[0]=sp[0]; end_pos[1]=sp[1]; end_pos[2]=sp[2];
        axes_d[0]=axes_d[1]=axes_d[2]=0.0;
        move_d = std::fabs(axes_d[3]);
        inv_move_d = 0.0;
        if (move_d) inv_move_d = 1.0/move_d;
        accel = 99999999.9;
        velocity = speed;
        is_kinematic_move = false;
    } else {
        inv_move_d = 1.0/move_d;
    }
    for (int i = 0; i < 4; ++i) axes_r[i] = axes_d[i]*inv_move_d;
    min_move_t = move_d/velocity;
    max_start_v2 = 0.0;
    max_cruise_v2 = velocity*velocity;
    delta_v2 = 2.0*move_d*accel;
    max_smoothed_v2 = 0.0;
    smooth_delta_v2 = 2.0*move_d*lim.max_accel_to_decel;
}

// --- Move.calc_junction (1:1) ---
void PlanMove::calc_junction(const PlanMove& prev, const ToolheadLimits& lim)
{
    if (!is_kinematic_move || !prev.is_kinematic_move)
        return;
    // Extruder junction limit (kin_extruder.calc_junction): instantaneous
    // corner velocity on E axis. extruder_v2 = instant_corner_v^2 unless the
    // E direction change is large. Simplified to firmware's default behavior:
    double diff_r = axes_r[3] - prev.axes_r[3];
    double extruder_v2;
    if (diff_r != 0.0) {
        double icv = lim.instant_corner_v;
        extruder_v2 = (icv*icv) / (diff_r*diff_r);
    } else {
        extruder_v2 = 1e300;
    }

    double jct = -(axes_r[0]*prev.axes_r[0] + axes_r[1]*prev.axes_r[1] + axes_r[2]*prev.axes_r[2]);
    if (jct > 0.999999)
        return;
    jct = std::max(jct, -0.999999);
    double sin_theta_d2 = std::sqrt(0.5*(1.0-jct));
    double R_jd = sin_theta_d2 / (1.0 - sin_theta_d2);
    double tan_theta_d2 = sin_theta_d2 / std::sqrt(0.5*(1.0+jct));
    double move_centripetal_v2 = 0.5 * move_d * tan_theta_d2 * accel;
    double prev_move_centripetal_v2 = 0.5 * prev.move_d * tan_theta_d2 * prev.accel;

    double vals[8] = {
        R_jd * junction_deviation * accel,
        R_jd * prev.junction_deviation * prev.accel,
        move_centripetal_v2, prev_move_centripetal_v2,
        extruder_v2, max_cruise_v2, prev.max_cruise_v2,
        prev.max_start_v2 + prev.delta_v2
    };
    double mn = vals[0];
    for (int i = 1; i < 8; ++i) mn = std::min(mn, vals[i]);
    max_start_v2 = mn;
    max_smoothed_v2 = std::min(max_start_v2, prev.max_smoothed_v2 + prev.smooth_delta_v2);
}

// --- Move.set_junction (1:1) ---
void PlanMove::set_junction(double start_v2, double cruise_v2, double end_v2)
{
    double half_inv_accel = 0.5 / accel;
    double accel_d = (cruise_v2 - start_v2) * half_inv_accel;
    double decel_d = (cruise_v2 - end_v2) * half_inv_accel;
    double cruise_d = move_d - accel_d - decel_d;
    start_v  = std::sqrt(start_v2);
    cruise_v = std::sqrt(cruise_v2);
    end_v    = std::sqrt(end_v2);
    accel_t  = accel_d / ((start_v + cruise_v) * 0.5);
    cruise_t = cruise_d / cruise_v;
    decel_t  = decel_d / ((end_v + cruise_v) * 0.5);
}

// --- MoveQueue.flush (1:1) ---
void KlipperMoveQueue::flush(bool lazy)
{
    m_junction_flush = LOOKAHEAD_FLUSH_TIME;
    bool update_flush_count = lazy;
    int flush_count = (int)m_queue.size();
    if (flush_count == 0) return;

    struct Delayed { PlanMove* m; double ms_v2; double me_v2; };
    std::vector<Delayed> delayed;
    double next_end_v2 = 0.0, next_smoothed_v2 = 0.0, peak_cruise_v2 = 0.0;

    for (int i = flush_count - 1; i >= 0; --i) {
        PlanMove& move = m_queue[i];
        double reachable_start_v2 = next_end_v2 + move.delta_v2;
        double start_v2 = std::min(move.max_start_v2, reachable_start_v2);
        double reachable_smoothed_v2 = next_smoothed_v2 + move.smooth_delta_v2;
        double smoothed_v2 = std::min(move.max_smoothed_v2, reachable_smoothed_v2);
        if (smoothed_v2 < reachable_smoothed_v2) {
            if ((smoothed_v2 + move.smooth_delta_v2 > next_smoothed_v2) || !delayed.empty()) {
                if (update_flush_count && peak_cruise_v2) {
                    flush_count = i;
                    update_flush_count = false;
                }
                peak_cruise_v2 = std::min(move.max_cruise_v2,
                                          (smoothed_v2 + reachable_smoothed_v2) * 0.5);
                if (!delayed.empty()) {
                    if (!update_flush_count && i < flush_count) {
                        double mc_v2 = peak_cruise_v2;
                        for (auto it = delayed.rbegin(); it != delayed.rend(); ++it) {
                            mc_v2 = std::min(mc_v2, it->ms_v2);
                            it->m->set_junction(std::min(it->ms_v2, mc_v2), mc_v2,
                                                std::min(it->me_v2, mc_v2));
                        }
                    }
                    delayed.clear();
                }
            }
            if (!update_flush_count && i < flush_count) {
                double cruise_v2 = std::min(std::min((start_v2 + reachable_start_v2) * 0.5,
                                                     move.max_cruise_v2), peak_cruise_v2);
                move.set_junction(std::min(start_v2, cruise_v2), cruise_v2,
                                  std::min(next_end_v2, cruise_v2));
            }
        } else {
            delayed.push_back({ &move, start_v2, next_end_v2 });
        }
        next_end_v2 = start_v2;
        next_smoothed_v2 = smoothed_v2;
    }
    if (update_flush_count || !flush_count)
        return;

    // Emit the flushed prefix.
    std::vector<PlanMove> batch(m_queue.begin(), m_queue.begin() + flush_count);
    m_process(batch);
    m_queue.erase(m_queue.begin(), m_queue.begin() + flush_count);
}

// --- MoveQueue.add_move (1:1) ---
void KlipperMoveQueue::add_move(const PlanMove& m)
{
    m_queue.push_back(m);
    if (m_queue.size() == 1)
        return;
    m_queue.back().calc_junction(m_queue[m_queue.size()-2], m_lim);
    m_junction_flush -= m_queue.back().min_move_t;
    if (m_junction_flush <= 0.0) {
        flush(true);
    }
}

KlipperHostToolhead::KlipperHostToolhead(const ToolheadLimits& lim,
                                         const ToolheadTiming& timing,
                                         HostProcessMovesCb cb,
                                         EstimatedPrintTimeCb estimated_print_time_cb,
                                         ToolheadFlushCb flush_cb)
    : m_limits(lim)
    , m_timing(timing)
    , m_process(std::move(cb))
    , m_estimated_print_time_cb(std::move(estimated_print_time_cb))
    , m_flush_cb(std::move(flush_cb))
    , m_move_queue(lim, [this](const std::vector<PlanMove>& moves) {
        this->process_moves(moves);
    })
{
    m_move_queue.set_flush_time(m_timing.buffer_time_high);
}

void KlipperHostToolhead::finish()
{
    const bool needs_flush = m_state != QueueState::Flushed || !m_move_queue.empty();
    m_move_queue.finish();
    if (needs_flush)
        flush_step_generation();
}

void KlipperHostToolhead::restore_state(const StateSnapshot& state)
{
    m_state = QueueState::Flushed;
    m_print_time = state.print_time;
    m_host_eventtime = state.host_eventtime;
    m_estimated_print_time = state.estimated_print_time;
    m_last_kin_flush_time = state.last_kin_flush_time;
    m_last_kin_move_time = state.last_kin_move_time;
    m_flush_windows.clear();
}

KlipperHostToolhead::StateSnapshot KlipperHostToolhead::snapshot_state() const
{
    StateSnapshot state;
    state.print_time = m_print_time;
    state.host_eventtime = m_host_eventtime;
    state.estimated_print_time = m_estimated_print_time;
    state.last_kin_flush_time = m_last_kin_flush_time;
    state.last_kin_move_time = m_last_kin_move_time;
    return state;
}

void KlipperHostToolhead::calc_print_time()
{
    if (m_estimated_print_time_cb)
        m_estimated_print_time = m_estimated_print_time_cb(m_host_eventtime);
    double kin_time = std::max(m_estimated_print_time + 0.100, m_last_kin_flush_time);
    kin_time += m_timing.kin_flush_delay;
    double min_print_time = std::max(m_estimated_print_time + m_timing.buffer_time_start,
                                     kin_time);
    if (min_print_time > m_print_time)
        m_print_time = min_print_time;
}

void KlipperHostToolhead::process_moves(const std::vector<PlanMove>& moves)
{
    if (moves.empty())
        return;
    if (m_state != QueueState::Main) {
        calc_print_time();
        m_state = QueueState::Main;
    }

    const double batch_start_time = m_print_time;
    if (m_process)
        m_process(moves, batch_start_time);

    double next_move_time = m_print_time;
    for (const PlanMove& move : moves)
        next_move_time += move.accel_t + move.cruise_t + move.decel_t;

    update_move_time(next_move_time);
    m_last_kin_move_time = next_move_time;
}

void KlipperHostToolhead::update_move_time(double next_print_time)
{
    const double batch_time = m_timing.move_batch_time;
    const double kin_flush_delay = m_timing.kin_flush_delay;
    const double lkft = m_last_kin_flush_time;
    const double host_eventtime = m_host_eventtime;
    double estimated_print_time = m_estimated_print_time;
    if (m_estimated_print_time_cb)
        estimated_print_time = m_estimated_print_time_cb(host_eventtime);
    while (true) {
        m_print_time = std::min(m_print_time + batch_time, next_print_time);
        ToolheadFlushWindow win;
        win.print_time = m_print_time;
        win.sg_flush_time = std::max(lkft, m_print_time - kin_flush_delay);
        win.free_time = std::max(lkft, win.sg_flush_time - kin_flush_delay);
        win.mcu_flush_time = std::max(lkft, win.sg_flush_time - m_timing.move_flush_time);
        win.host_eventtime = host_eventtime;
        win.estimated_print_time = estimated_print_time;
        m_flush_windows.push_back(win);
        if (m_flush_cb)
            m_flush_cb(win);
        if (m_print_time >= next_print_time)
            break;
    }
    m_estimated_print_time = estimated_print_time;
    if (m_estimated_print_time_cb) {
        const double stall_time =
            (m_print_time - estimated_print_time) - m_timing.buffer_time_high;
        if (stall_time > 0.0) {
            m_host_eventtime += stall_time;
            m_estimated_print_time = m_estimated_print_time_cb(m_host_eventtime);
        }
    }
}
void KlipperHostToolhead::flush_step_generation()
{
    m_state = QueueState::Flushed;
    m_move_queue.set_flush_time(m_timing.buffer_time_high);
    double flush_time = m_last_kin_move_time + m_timing.kin_flush_delay;
    flush_time = std::max(flush_time, m_print_time - m_timing.kin_flush_delay);
    m_last_kin_flush_time = std::max(m_last_kin_flush_time, flush_time);
    update_move_time(std::max(m_print_time, m_last_kin_flush_time));
}}} // namespace Slic3r::KlipperSim
