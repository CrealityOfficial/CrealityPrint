#include "KlipperItersolve.hpp"

#include <cmath>
#if defined(__has_include)
#  if __has_include(<boost/log/trivial.hpp>)
#    include <boost/log/trivial.hpp>
#  else
#    include <iostream>
#    define BOOST_LOG_TRIVIAL(level) std::cerr
#  endif
#else
#  include <boost/log/trivial.hpp>
#endif
#include <sstream>

namespace Slic3r {
namespace KlipperSim {

static constexpr double SEEK_TIME_RESET = 0.000100;

namespace {

static void annotate_new_stepcompress_cmds(KlipperStepCompress& sc,
                                           size_t prev_count,
                                           const TrapMove& move)
{
    std::vector<StepMoveCmd>& cmds = sc.commands_mut();
    if (prev_count >= cmds.size())
        return;
    for (size_t i = prev_count; i < cmds.size(); ++i) {
        cmds[i].line_idx = move.line_idx;
        cmds[i].batch_start_time = move.batch_start_time;
    }
}

} // namespace

double KlipperItersolve::calc_position_from_coord(double x, double y, double z) const
{
    TrapMove m;
    m.start_pos = Coord{ x, y, z };
    m.move_t = 1000.0;
    return m_calc(m, 500.0);
}

void KlipperItersolve::set_position(double x, double y, double z)
{
    m_commanded_pos = calc_position_from_coord(x, y, z);
}

bool KlipperItersolve::is_active_axis(char axis) const
{
    if (axis < 'x' || axis > 'z')
        return false;
    return (m_active_flags & (AF_X << (axis - 'x'))) != 0;
}

// Port of firmware itersolve_gen_steps_range for one trapezoid phase.
// Emits ABSOLUTE step times (seconds) = m.print_time + guess.time.
void KlipperItersolve::gen_steps_range(const TrapMove& m,
                                       double abs_start, double abs_end,
                                       std::vector<double>& out_steps)
{
    double half_step = 0.5 * m_step_dist;
    double start = abs_start - m.print_time;
    double end   = abs_end   - m.print_time;
    if (start < 0.0) start = 0.0;
    if (end > m.move_t) end = m.move_t;
    if (end <= start) {
        // still advance commanded_pos to end-of-considered-range position?
        // firmware only sets commanded_pos when steps found; nothing to do.
        return;
    }

    TimePos old_guess{ start, m_commanded_pos }, guess = old_guess;
    // Determine initial step direction from local state.
    int sdir = m_sdir;
    int is_dir_change = 0, have_bracket = 0, check_oscillate = 0;
    double target = m_commanded_pos + (sdir ? half_step : -half_step);
    double last_time = start, low_time = start, high_time = start + SEEK_TIME_RESET;
    if (high_time > end) high_time = end;

    for (;;) {
        double guess_dist = guess.position - target;
        double og_dist    = old_guess.position - target;
        double denom      = (guess_dist - og_dist);
        double next_time  = (denom != 0.0)
            ? ((old_guess.time * guess_dist - guess.time * og_dist) / denom)
            : std::nan("");

        if (!(next_time > low_time && next_time < high_time)) { // or NaN
            if (have_bracket) {
                next_time = (low_time + high_time) * 0.5;
                check_oscillate = 0;
            } else if (guess.time >= end) {
                break;
            } else {
                next_time = high_time;
                high_time = 2.0 * high_time - last_time;
                if (high_time > end) high_time = end;
            }
        }

        old_guess = guess;
        guess.time = next_time;
        guess.position = m_calc(m, next_time);
        guess_dist = guess.position - target;

        if (std::fabs(guess_dist) > 0.000000001) {
            double rel_dist = sdir ? guess_dist : -guess_dist;
            if (rel_dist > 0.0) {
                if (have_bracket && old_guess.time <= low_time) {
                    if (check_oscillate)
                        old_guess = guess;
                    check_oscillate = 1;
                }
                high_time = guess.time;
                have_bracket = 1;
            } else if (rel_dist < -(half_step + half_step + 0.000000010)) {
                // direction change
                sdir = !sdir;
                target = (sdir ? target + half_step + half_step
                               : target - half_step - half_step);
                low_time = last_time;
                high_time = guess.time;
                is_dir_change = have_bracket = 1;
                check_oscillate = 0;
            } else {
                low_time = guess.time;
            }
            if (!have_bracket || high_time - low_time > 0.000000001) {
                // (firmware commits stepcompress here; we don't need rollback)
                continue;
            }
        }

        // Found next step — emit absolute time.
        out_steps.push_back(m.print_time + guess.time);

        target = sdir ? target + half_step + half_step
                      : target - half_step - half_step;
        double seek_time_delta = 1.5 * (guess.time - last_time);
        if (seek_time_delta < 0.000000001) seek_time_delta = 0.000000001;
        if (is_dir_change && seek_time_delta > SEEK_TIME_RESET)
            seek_time_delta = SEEK_TIME_RESET;
        last_time = low_time = guess.time;
        high_time = guess.time + seek_time_delta;
        if (high_time > end) high_time = end;
        is_dir_change = have_bracket = check_oscillate = 0;
    }

    m_commanded_pos = target - (sdir ? half_step : -half_step);
    m_sdir = sdir;
}

void KlipperItersolve::gen_steps_range_sc(const TrapMove& m,
                                          double abs_start, double abs_end,
                                          KlipperStepCompress& sc,
                                          double mcu_freq,
                                          size_t& out_count)
{
    double half_step = 0.5 * m_step_dist;
    double start = abs_start - m.print_time;
    double end   = abs_end   - m.print_time;
    if (start < 0.0) start = 0.0;
    if (end > m.move_t) end = m.move_t;
    if (end <= start)
        return;

    TimePos old_guess{ start, m_commanded_pos }, guess = old_guess;
    int sdir = sc.current_step_dir();
    int is_dir_change = 0, have_bracket = 0, check_oscillate = 0;
    double target = m_commanded_pos + (sdir ? half_step : -half_step);
    double last_time = start, low_time = start, high_time = start + SEEK_TIME_RESET;
    if (high_time > end) high_time = end;

    static constexpr int MAX_ITERSOLVE_ITER = 1000000;
    int iter_guard = 0;
    for (;;) {
        if (++iter_guard > MAX_ITERSOLVE_ITER) {
            BOOST_LOG_TRIVIAL(error)
                << "[KLSIM] ERROR itersolve SC diverged: start=" << start
                << " end=" << end << " move_t=" << m.move_t
                << " low=" << low_time << " high=" << high_time
                << " guess.t=" << guess.time << " guess.p=" << guess.position
                << " target=" << target << " step_dist=" << m_step_dist
                << " line=" << m.line_idx << " — aborting loop";
            break;
        }
        double guess_dist = guess.position - target;
        double og_dist    = old_guess.position - target;
        double denom      = (guess_dist - og_dist);
        double next_time  = (denom != 0.0)
            ? ((old_guess.time * guess_dist - guess.time * og_dist) / denom)
            : std::nan("");

        if (!(next_time > low_time && next_time < high_time)) {
            if (have_bracket) {
                next_time = (low_time + high_time) * 0.5;
                check_oscillate = 0;
            } else if (guess.time >= end) {
                break;
            } else {
                next_time = high_time;
                high_time = 2.0 * high_time - last_time;
                if (high_time > end) high_time = end;
            }
        }

        old_guess = guess;
        guess.time = next_time;
        guess.position = m_calc(m, next_time);
        guess_dist = guess.position - target;

        if (std::fabs(guess_dist) > 0.000000001) {
            double rel_dist = sdir ? guess_dist : -guess_dist;
            if (rel_dist > 0.0) {
                if (have_bracket && old_guess.time <= low_time) {
                    if (check_oscillate)
                        old_guess = guess;
                    check_oscillate = 1;
                }
                high_time = guess.time;
                have_bracket = 1;
            } else if (rel_dist < -(half_step + half_step + 0.000000010)) {
                sdir = !sdir;
                target = (sdir ? target + half_step + half_step
                               : target - half_step - half_step);
                low_time = last_time;
                high_time = guess.time;
                is_dir_change = have_bracket = 1;
                check_oscillate = 0;
            } else {
                low_time = guess.time;
            }
            if (!have_bracket || high_time - low_time > 0.000000001) {
                if (!is_dir_change && rel_dist >= -half_step)
                    sc.commit();
                continue;
            }
        }

        sc.append_step_time(sdir, m.print_time, guess.time, mcu_freq, 0.0,
                            m.line_idx, m.batch_start_time);
        ++out_count;

        target = sdir ? target + half_step + half_step
                      : target - half_step - half_step;
        double seek_time_delta = 1.5 * (guess.time - last_time);
        if (seek_time_delta < 0.000000001) seek_time_delta = 0.000000001;
        if (is_dir_change && seek_time_delta > SEEK_TIME_RESET)
            seek_time_delta = SEEK_TIME_RESET;
        last_time = low_time = guess.time;
        high_time = guess.time + seek_time_delta;
        if (high_time > end) high_time = end;
        is_dir_change = have_bracket = check_oscillate = 0;
    }

    m_commanded_pos = target - (sdir ? half_step : -half_step);
}

std::vector<double>
KlipperItersolve::generate_steps(const KlipperTrapQ& tq,
                                 const std::function<bool(const TrapMove&)>& active_pred)
{
    std::vector<double> steps;
    const auto& moves = tq.moves();
    if (moves.empty())
        return steps;

    m_commanded_pos = m_calc(moves.front(), 0.0);
    m_last_flush_time = 0.0;
    m_last_move_time = 0.0;

    double flush_time = moves.back().print_time + moves.back().move_t;
    size_t mi = 0;
    while (mi < moves.size() && m_last_flush_time >= moves[mi].print_time + moves[mi].move_t)
        ++mi;
    double force_steps_time = m_last_move_time + m_gen_steps_post_active;
    int skip_count = 0;
    for (; mi < moves.size(); ++mi) {
        const TrapMove& m = moves[mi];
        double move_start = m.print_time;
        double move_end = move_start + m.move_t;
        if (check_active(m, active_pred)) {
            if (skip_count && m_gen_steps_pre_active > 0.0) {
                double abs_start = move_start - m_gen_steps_pre_active;
                if (abs_start < m_last_flush_time)
                    abs_start = m_last_flush_time;
                if (abs_start < force_steps_time)
                    abs_start = force_steps_time;
                size_t pmi = mi - 1;
                while (--skip_count && moves[pmi].print_time > abs_start)
                    --pmi;
                do {
                    gen_steps_range(moves[pmi], abs_start, flush_time, steps);
                    ++pmi;
                } while (pmi != mi);
            }
            gen_steps_range(m, m_last_flush_time, flush_time, steps);
            if (move_end >= flush_time) {
                m_last_move_time = flush_time;
                m_last_flush_time = flush_time;
                return steps;
            }
            skip_count = 0;
            m_last_move_time = move_end;
            force_steps_time = m_last_move_time + m_gen_steps_post_active;
        } else {
            if (move_start < force_steps_time) {
                double abs_end = force_steps_time;
                if (abs_end > flush_time)
                    abs_end = flush_time;
                gen_steps_range(m, m_last_flush_time, abs_end, steps);
                skip_count = 1;
            } else {
                ++skip_count;
            }
            if (flush_time + m_gen_steps_pre_active <= move_end) {
                m_last_flush_time = flush_time;
                return steps;
            }
        }
    }
    m_last_flush_time = flush_time;
    return steps;
}

size_t KlipperItersolve::generate_stepcompress(
    const KlipperTrapQ& tq,
    const std::function<bool(const TrapMove&)>& active_pred,
    KlipperStepCompress& sc, double mcu_freq)
{
    size_t out_count = 0;
    const auto& moves = tq.moves();
    if (!moves.empty())
        m_commanded_pos = m_calc(moves.front(), 0.0);

    for (const TrapMove& m : moves) {
        if (!active_pred(m))
            continue;
        gen_steps_range_sc(m, m.print_time, m.print_time + m.move_t,
                           sc, mcu_freq, out_count);
    }
    return out_count;
}

size_t KlipperItersolve::generate_stepcompress_to(
    const KlipperTrapQ& tq, double flush_time,
    const std::function<bool(const TrapMove&)>& active_pred,
    KlipperStepCompress& sc, double mcu_freq)
{
    const auto& moves = tq.moves();
    if (moves.empty())
        return 0;
    size_t out_count = 0;
    if (!m_stream_seeded) {
        m_commanded_pos = m_calc(moves.front(), 0.0);
        m_last_flush_time = 0.0;
        m_last_move_time = 0.0;
        m_next_move_idx = 0;
        m_stream_seeded = true;
    }
    while (m_next_move_idx < moves.size()
           && m_last_flush_time >= moves[m_next_move_idx].print_time + moves[m_next_move_idx].move_t)
        ++m_next_move_idx;

    const size_t start_idx = m_next_move_idx;
    size_t end_idx = start_idx;
    size_t active_moves = 0;
    int first_line = -1;
    int last_line = -1;
    double force_steps_time = m_last_move_time + m_gen_steps_post_active;
    int skip_count = 0;
    for (size_t mi = m_next_move_idx; mi < moves.size(); ++mi) {
        const TrapMove& m = moves[mi];
        double move_start = m.print_time;
        double move_end = m.print_time + m.move_t;
        if (flush_time <= move_start)
            break;
        end_idx = mi;
        if (check_active(m, active_pred)) {
            ++active_moves;
            if (first_line < 0)
                first_line = m.line_idx;
            last_line = m.line_idx;
            if (skip_count && m_gen_steps_pre_active > 0.0) {
                double abs_start = move_start - m_gen_steps_pre_active;
                if (abs_start < m_last_flush_time)
                    abs_start = m_last_flush_time;
                if (abs_start < force_steps_time)
                    abs_start = force_steps_time;
                size_t pmi = mi - 1;
                while (--skip_count && moves[pmi].print_time > abs_start)
                    --pmi;
                do {
                    size_t cmd_count = sc.command_count();
                    gen_steps_range_sc(moves[pmi], abs_start, flush_time, sc, mcu_freq, out_count);
                    annotate_new_stepcompress_cmds(sc, cmd_count, moves[pmi]);
                    ++pmi;
                } while (pmi != mi);
            }
            size_t cmd_count = sc.command_count();
            gen_steps_range_sc(m, m_last_flush_time, flush_time, sc, mcu_freq, out_count);
            annotate_new_stepcompress_cmds(sc, cmd_count, m);
            if (move_end >= flush_time) {
                m_last_move_time = flush_time;
                m_last_flush_time = flush_time;
                return out_count;
            }
            skip_count = 0;
            m_last_move_time = move_end;
            force_steps_time = m_last_move_time + m_gen_steps_post_active;
        } else {
            if (move_start < force_steps_time) {
                double abs_end = force_steps_time;
                if (abs_end > flush_time)
                    abs_end = flush_time;
                size_t cmd_count = sc.command_count();
                gen_steps_range_sc(m, m_last_flush_time, abs_end, sc, mcu_freq, out_count);
                annotate_new_stepcompress_cmds(sc, cmd_count, m);
                skip_count = 1;
            } else {
                ++skip_count;
            }
            if (flush_time + m_gen_steps_pre_active <= move_end)
                break;
        }
        if (flush_time < move_end) {
            m_next_move_idx = mi;
            break;
        }
        m_next_move_idx = mi + 1;
    }
    m_last_flush_time = std::max(m_last_flush_time, flush_time);
    return out_count;
}

// Indexed (pressure-advance) variant: position depends on neighboring phases.
void KlipperItersolve::gen_steps_range_indexed(
    const std::vector<TrapMove>& moves, size_t mi,
    double abs_start, double abs_end,
    const std::function<double(const std::vector<TrapMove>&, size_t, double)>& pos_cb,
    std::vector<double>& out_steps)
{
    const TrapMove& m = moves[mi];
    double half_step = 0.5 * m_step_dist;
    double start = abs_start - m.print_time;
    double end   = abs_end   - m.print_time;
    if (start < 0.0) start = 0.0;
    if (end > m.move_t) end = m.move_t;
    if (end <= start) return;

    TimePos old_guess{ start, m_commanded_pos }, guess = old_guess;
    int sdir = m_sdir;
    int is_dir_change = 0, have_bracket = 0, check_oscillate = 0;
    double target = m_commanded_pos + (sdir ? half_step : -half_step);
    double last_time = start, low_time = start, high_time = start + SEEK_TIME_RESET;
    if (high_time > end) high_time = end;

    for (;;) {
        double guess_dist = guess.position - target;
        double og_dist    = old_guess.position - target;
        double denom      = (guess_dist - og_dist);
        double next_time  = (denom != 0.0)
            ? ((old_guess.time * guess_dist - guess.time * og_dist) / denom)
            : std::nan("");
        if (!(next_time > low_time && next_time < high_time)) {
            if (have_bracket) { next_time = (low_time + high_time) * 0.5; check_oscillate = 0; }
            else if (guess.time >= end) break;
            else { next_time = high_time; high_time = 2.0*high_time - last_time; if (high_time > end) high_time = end; }
        }
        old_guess = guess;
        guess.time = next_time;
        guess.position = pos_cb(moves, mi, next_time);
        guess_dist = guess.position - target;
        if (std::fabs(guess_dist) > 0.000000001) {
            double rel_dist = sdir ? guess_dist : -guess_dist;
            if (rel_dist > 0.0) {
                if (have_bracket && old_guess.time <= low_time) { if (check_oscillate) old_guess = guess; check_oscillate = 1; }
                high_time = guess.time; have_bracket = 1;
            } else if (rel_dist < -(half_step + half_step + 0.000000010)) {
                sdir = !sdir;
                target = (sdir ? target + half_step + half_step : target - half_step - half_step);
                low_time = last_time; high_time = guess.time; is_dir_change = have_bracket = 1; check_oscillate = 0;
            } else { low_time = guess.time; }
            if (!have_bracket || high_time - low_time > 0.000000001) continue;
        }
        out_steps.push_back(m.print_time + guess.time);
        target = sdir ? target + half_step + half_step : target - half_step - half_step;
        double seek_time_delta = 1.5 * (guess.time - last_time);
        if (seek_time_delta < 0.000000001) seek_time_delta = 0.000000001;
        if (is_dir_change && seek_time_delta > SEEK_TIME_RESET) seek_time_delta = SEEK_TIME_RESET;
        last_time = low_time = guess.time;
        high_time = guess.time + seek_time_delta;
        if (high_time > end) high_time = end;
        is_dir_change = have_bracket = check_oscillate = 0;
    }
    m_commanded_pos = target - (sdir ? half_step : -half_step);
    m_sdir = sdir;
}

void KlipperItersolve::gen_steps_range_indexed_sc(
    const std::vector<TrapMove>& moves, size_t mi,
    double abs_start, double abs_end,
    const std::function<double(const std::vector<TrapMove>&, size_t, double)>& pos_cb,
    KlipperStepCompress& sc, double mcu_freq,
    size_t& out_count)
{
    const TrapMove& m = moves[mi];
    double half_step = 0.5 * m_step_dist;
    double start = abs_start - m.print_time;
    double end   = abs_end   - m.print_time;
    if (start < 0.0) start = 0.0;
    if (end > m.move_t) end = m.move_t;
    if (end <= start) return;

    TimePos old_guess{ start, m_commanded_pos }, guess = old_guess;
    int sdir = sc.current_step_dir();
    int is_dir_change = 0, have_bracket = 0, check_oscillate = 0;
    double target = m_commanded_pos + (sdir ? half_step : -half_step);
    double last_time = start, low_time = start, high_time = start + SEEK_TIME_RESET;
    if (high_time > end) high_time = end;

    static constexpr int MAX_ITERSOLVE_ITER = 1000000;
    int iter_guard = 0;
    for (;;) {
        if (++iter_guard > MAX_ITERSOLVE_ITER) {
            BOOST_LOG_TRIVIAL(error)
                << "[KLSIM] ERROR itersolve indexed diverged: start=" << start
                << " end=" << end << " move_t=" << m.move_t
                << " low=" << low_time << " high=" << high_time
                << " guess.t=" << guess.time << " guess.p=" << guess.position
                << " target=" << target << " step_dist=" << m_step_dist
                << " line=" << m.line_idx << " — aborting loop";
            break;
        }
        double guess_dist = guess.position - target;
        double og_dist    = old_guess.position - target;
        double denom      = (guess_dist - og_dist);
        double next_time  = (denom != 0.0)
            ? ((old_guess.time * guess_dist - guess.time * og_dist) / denom)
            : std::nan("");
        if (!(next_time > low_time && next_time < high_time)) {
            if (have_bracket) { next_time = (low_time + high_time) * 0.5; check_oscillate = 0; }
            else if (guess.time >= end) break;
            else { next_time = high_time; high_time = 2.0*high_time - last_time; if (high_time > end) high_time = end; }
        }
        old_guess = guess;
        guess.time = next_time;
        guess.position = pos_cb(moves, mi, next_time);
        guess_dist = guess.position - target;
        if (std::fabs(guess_dist) > 0.000000001) {
            double rel_dist = sdir ? guess_dist : -guess_dist;
            if (rel_dist > 0.0) {
                if (have_bracket && old_guess.time <= low_time) { if (check_oscillate) old_guess = guess; check_oscillate = 1; }
                high_time = guess.time; have_bracket = 1;
            } else if (rel_dist < -(half_step + half_step + 0.000000010)) {
                sdir = !sdir;
                target = (sdir ? target + half_step + half_step : target - half_step - half_step);
                low_time = last_time; high_time = guess.time; is_dir_change = have_bracket = 1; check_oscillate = 0;
            } else { low_time = guess.time; }
            if (!have_bracket || high_time - low_time > 0.000000001) {
                if (!is_dir_change && rel_dist >= -half_step)
                    sc.commit();
                continue;
            }
        }
        sc.append_step_time(sdir, m.print_time, guess.time, mcu_freq, 0.0,
                            m.line_idx, m.batch_start_time);
        ++out_count;
        target = sdir ? target + half_step + half_step : target - half_step - half_step;
        double seek_time_delta = 1.5 * (guess.time - last_time);
        if (seek_time_delta < 0.000000001) seek_time_delta = 0.000000001;
        if (is_dir_change && seek_time_delta > SEEK_TIME_RESET) seek_time_delta = SEEK_TIME_RESET;
        last_time = low_time = guess.time;
        high_time = guess.time + seek_time_delta;
        if (high_time > end) high_time = end;
        is_dir_change = have_bracket = check_oscillate = 0;
    }
    m_commanded_pos = target - (sdir ? half_step : -half_step);
}

std::vector<double>
KlipperItersolve::generate_steps_indexed(
    const std::vector<TrapMove>& moves,
    const std::function<double(const std::vector<TrapMove>&, size_t, double)>& pos_cb,
    const std::function<bool(const TrapMove&)>& active_pred)
{
    std::vector<double> steps;
    if (moves.empty())
        return steps;
    if (!moves.empty())
        m_commanded_pos = pos_cb(moves, 0, 0.0);

    m_last_flush_time = 0.0;
    m_last_move_time = 0.0;
    double force_steps_time = m_last_move_time + m_gen_steps_post_active;
    int skip_count = 0;
    for (size_t mi = 0; mi < moves.size(); ++mi) {
        const TrapMove& m = moves[mi];
        double move_start = m.print_time;
        double move_end = move_start + m.move_t;
        if (check_active(m, active_pred)) {
            if (skip_count && m_gen_steps_pre_active > 0.0) {
                double abs_start = move_start - m_gen_steps_pre_active;
                if (abs_start < m_last_flush_time)
                    abs_start = m_last_flush_time;
                if (abs_start < force_steps_time)
                    abs_start = force_steps_time;
                size_t pmi = mi - 1;
                while (--skip_count && moves[pmi].print_time > abs_start)
                    --pmi;
                do {
                    gen_steps_range_indexed(moves, pmi, abs_start, move_end, pos_cb, steps);
                    ++pmi;
                } while (pmi != mi);
            }
            gen_steps_range_indexed(moves, mi, m_last_flush_time, move_end, pos_cb, steps);
            skip_count = 0;
            m_last_move_time = move_end;
            force_steps_time = m_last_move_time + m_gen_steps_post_active;
        } else {
            if (move_start < force_steps_time) {
                double abs_end = force_steps_time;
                if (abs_end > move_end)
                    abs_end = move_end;
                gen_steps_range_indexed(moves, mi, m_last_flush_time, abs_end, pos_cb, steps);
                skip_count = 1;
            } else {
                ++skip_count;
            }
        }
        m_last_flush_time = move_end;
    }
    return steps;
}

size_t KlipperItersolve::generate_stepcompress_indexed(
    const std::vector<TrapMove>& moves,
    const std::function<double(const std::vector<TrapMove>&, size_t, double)>& pos_cb,
    const std::function<bool(const TrapMove&)>& active_pred,
    KlipperStepCompress& sc, double mcu_freq)
{
    size_t out_count = 0;
    if (!moves.empty())
        m_commanded_pos = pos_cb(moves, 0, 0.0);
    for (size_t mi = 0; mi < moves.size(); ++mi) {
        if (!active_pred(moves[mi]))
            continue;
        gen_steps_range_indexed_sc(moves, mi, moves[mi].print_time,
                                   moves[mi].print_time + moves[mi].move_t,
                                   pos_cb, sc, mcu_freq, out_count);
    }
    return out_count;
}

size_t KlipperItersolve::generate_stepcompress_indexed_to(
    const std::vector<TrapMove>& moves, double flush_time,
    const std::function<double(const std::vector<TrapMove>&, size_t, double)>& pos_cb,
    const std::function<bool(const TrapMove&)>& active_pred,
    KlipperStepCompress& sc, double mcu_freq)
{
    size_t out_count = 0;
    if (moves.empty())
        return 0;
    if (!m_stream_seeded) {
        m_commanded_pos = pos_cb(moves, 0, 0.0);
        m_last_flush_time = 0.0;
        m_last_move_time = 0.0;
        m_next_move_idx = 0;
        m_stream_seeded = true;
    }
    while (m_next_move_idx < moves.size()
           && m_last_flush_time >= moves[m_next_move_idx].print_time + moves[m_next_move_idx].move_t)
        ++m_next_move_idx;
    const size_t start_idx = m_next_move_idx;
    size_t end_idx = start_idx;
    size_t active_moves = 0;
    int first_line = -1;
    int last_line = -1;
    double force_steps_time = m_last_move_time + m_gen_steps_post_active;
    int skip_count = 0;
    for (size_t mi = m_next_move_idx; mi < moves.size(); ++mi) {
        const TrapMove& m = moves[mi];
        double move_start = m.print_time;
        double move_end = m.print_time + m.move_t;
        if (flush_time <= move_start)
            break;
        end_idx = mi;
        if (check_active(m, active_pred)) {
            ++active_moves;
            if (first_line < 0)
                first_line = m.line_idx;
            last_line = m.line_idx;
            if (skip_count && m_gen_steps_pre_active > 0.0) {
                double abs_start = move_start - m_gen_steps_pre_active;
                if (abs_start < m_last_flush_time)
                    abs_start = m_last_flush_time;
                if (abs_start < force_steps_time)
                    abs_start = force_steps_time;
                size_t pmi = mi - 1;
                while (--skip_count && moves[pmi].print_time > abs_start)
                    --pmi;
                do {
                    size_t cmd_count = sc.command_count();
                    gen_steps_range_indexed_sc(moves, pmi, abs_start, flush_time,
                                               pos_cb, sc, mcu_freq, out_count);
                    annotate_new_stepcompress_cmds(sc, cmd_count, moves[pmi]);
                    ++pmi;
                } while (pmi != mi);
            }
            size_t cmd_count = sc.command_count();
            gen_steps_range_indexed_sc(moves, mi, m_last_flush_time, flush_time,
                                       pos_cb, sc, mcu_freq, out_count);
            annotate_new_stepcompress_cmds(sc, cmd_count, m);
            if (move_end >= flush_time) {
                m_last_move_time = flush_time;
                m_last_flush_time = flush_time;
                return out_count;
            }
            skip_count = 0;
            m_last_move_time = move_end;
            force_steps_time = m_last_move_time + m_gen_steps_post_active;
        } else {
            if (move_start < force_steps_time) {
                double abs_end = force_steps_time;
                if (abs_end > flush_time)
                    abs_end = flush_time;
                size_t cmd_count = sc.command_count();
                gen_steps_range_indexed_sc(moves, mi, m_last_flush_time, abs_end,
                                           pos_cb, sc, mcu_freq, out_count);
                annotate_new_stepcompress_cmds(sc, cmd_count, m);
                skip_count = 1;
            } else {
                ++skip_count;
            }
            if (flush_time + m_gen_steps_pre_active <= move_end)
                break;
        }
        if (flush_time < move_end) {
            m_next_move_idx = mi;
            break;
        }
        m_next_move_idx = mi + 1;
    }
    m_last_flush_time = std::max(m_last_flush_time, flush_time);
    return out_count;
}

}} // namespace Slic3r::KlipperSim
