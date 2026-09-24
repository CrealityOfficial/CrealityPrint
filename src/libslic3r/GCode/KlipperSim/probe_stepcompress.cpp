#include "KlipperSimMain.hpp"
#include "KlipperTrapQ.hpp"
#include "KlipperItersolve.hpp"
#include "KlipperStepCompress.hpp"
#include "KlipperToolhead.hpp"

#include <cstdio>

using namespace Slic3r::KlipperSim;

static double probe_calc_junction_deviation(double square_corner_velocity,
                                            double max_accel)
{
    return max_accel > 0.0
        ? square_corner_velocity * square_corner_velocity
          * (std::sqrt(2.0) - 1.0) / max_accel
        : 0.0;
}

static std::vector<ToolheadFlushWindow>
build_trapqs(const std::vector<GMove>& gmoves, const SimConfig& cfg,
             KlipperTrapQ& xy_tq, KlipperTrapQ& e_tq)
{
    ToolheadTiming timing;
    timing.buffer_time_high = cfg.buffer_time_high;
    timing.buffer_time_start = 0.250;
    timing.move_flush_time = 0.050;
    timing.kin_flush_delay = 0.001;
    timing.move_batch_time = 0.500;

    Coord xy_pos{0,0,0}, e_pos{0,0,0};
    KlipperHostToolhead toolhead(cfg.limits, timing,
        [&](const std::vector<PlanMove>& batch, double batch_start_time) {
            double move_time = batch_start_time;
            for (const PlanMove& pm : batch) {
                Coord xyzr{ pm.axes_r[0], pm.axes_r[1], pm.axes_r[2] };
                xy_tq.append(move_time, batch_start_time,
                             pm.accel_t, pm.cruise_t, pm.decel_t,
                             xy_pos, xyzr, pm.start_v, pm.cruise_v, pm.accel,
                             pm.line_idx);
                xy_pos.x += pm.axes_d[0];
                xy_pos.y += pm.axes_d[1];
                xy_pos.z += pm.axes_d[2];

                double de = pm.axes_d[3];
                double escale = (pm.move_d > 1e-12) ? (de / pm.move_d) : 0.0;
                double has_xy = pm.is_kinematic_move ? 1.0 : 0.0;
                e_tq.append(move_time, batch_start_time,
                            pm.accel_t, pm.cruise_t, pm.decel_t,
                            e_pos, Coord{1, has_xy, 0},
                            pm.start_v * escale, pm.cruise_v * escale,
                            pm.accel * escale, pm.line_idx);
                e_pos.x += de;
                move_time += pm.accel_t + pm.cruise_t + pm.decel_t;
            }
        });

    double cur[4]{0,0,0,0};
    bool has_seed = false;
    for (const auto& gm : gmoves) {
        if (!has_seed) {
            cur[0] = gm.x;
            cur[1] = gm.y;
            cur[2] = gm.z;
            cur[3] = 0.0;
            has_seed = true;
        }
        PlanMove pm;
        double next[4]{gm.x, gm.y, gm.z, cur[3] + gm.e};
        ToolheadLimits lim = cfg.limits;
        lim.max_accel = gm.accel;
        lim.max_accel_to_decel = gm.accel_to_decel;
        lim.square_corner_velocity = gm.square_corner_velocity;
        lim.junction_deviation = probe_calc_junction_deviation(
            lim.square_corner_velocity, lim.max_accel);
        pm.line_idx = gm.line_idx;
        pm.init(lim, cur, next, gm.v);
        toolhead.update_limits(lim);
        toolhead.add_move(pm);
        for (int i = 0; i < 4; ++i) cur[i] = next[i];
    }
    toolhead.finish();
    return toolhead.flush_windows();
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::fprintf(stderr, "usage: probe_stepcompress <gcode>\n");
        return 2;
    }
    SimConfig cfg;
    ParsedGCode gcode = parse_gcode_detailed(argv[1]);
    KlipperTrapQ xy_tq, e_tq;
    auto flush_windows = build_trapqs(gcode.moves, cfg, xy_tq, e_tq);

    uint32_t maxerr = (uint32_t)std::llround(cfg.max_stepper_error * cfg.mcu_freq);
    KlipperItersolve sA([](const TrapMove& m, double t){ return m.get_coord(t).x + m.get_coord(t).y; },
                        cfg.xy_step_dist);
    auto xyact = [](const TrapMove& m) {
        return m.axes_r.x != 0.0 || m.axes_r.y != 0.0 || m.axes_r.z != 0.0;
    };

    auto offline_steps = sA.generate_steps(xy_tq, xyact);
    KlipperStepCompress off(maxerr);
    for (double t : offline_steps)
        off.append_step((uint64_t)std::llround(t * cfg.mcu_freq));
    off.finish();

    KlipperStepCompress on(maxerr);
    size_t on_steps = sA.generate_stepcompress(xy_tq, xyact, on, cfg.mcu_freq);
    on.finish();

    KlipperItersolve sWin([](const TrapMove& m, double t){ return m.get_coord(t).x + m.get_coord(t).y; },
                          cfg.xy_step_dist);
    KlipperStepCompress win(maxerr);
    size_t win_steps = 0;
    for (const auto& fw : flush_windows) {
        win_steps += sWin.generate_stepcompress_to(xy_tq, fw.sg_flush_time, xyact, win, cfg.mcu_freq);
        win.flush_to((uint64_t)std::llround(fw.mcu_flush_time * cfg.mcu_freq));
    }
    win.finish();

    KlipperItersolve sB([](const TrapMove& m, double t){ return m.get_coord(t).x - m.get_coord(t).y; },
                        cfg.xy_step_dist);
    KlipperStepCompress winB(maxerr);
    size_t win_b_steps = 0;
    for (const auto& fw : flush_windows) {
        win_b_steps += sB.generate_stepcompress_to(xy_tq, fw.sg_flush_time, xyact, winB, cfg.mcu_freq);
        winB.flush_to((uint64_t)std::llround(fw.mcu_flush_time * cfg.mcu_freq));
    }
    winB.finish();

    std::printf("offline_steps=%zu offline_cmds=%zu online_steps=%zu online_cmds=%zu window_steps=%zu window_cmds=%zu window_b_steps=%zu window_b_cmds=%zu\n",
                offline_steps.size(), off.command_count(), on_steps, on.command_count(),
                win_steps, win.command_count(), win_b_steps, winB.command_count());
    const auto& cmds = on.commands();
    size_t single = 0;
    for (const auto& c : cmds)
        if (c.count == 1) ++single;
    std::printf("online_single_cmds=%zu\n", single);
    return 0;
}
