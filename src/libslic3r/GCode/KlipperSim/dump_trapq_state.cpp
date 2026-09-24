#include "KlipperSimMain.hpp"
#include "KlipperTrapQ.hpp"
#include "KlipperToolhead.hpp"

#include <cstdio>

using namespace Slic3r::KlipperSim;

static double calc_jd(double scv, double accel)
{
    return accel > 0.0
        ? scv * scv * (std::sqrt(2.0) - 1.0) / accel
        : 0.0;
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::fprintf(stderr, "usage: dump_trapq_state <gcode>\n");
        return 2;
    }

    SimConfig cfg;
    ParsedGCode gcode = parse_gcode_detailed(argv[1]);
    KlipperTrapQ xy_tq, e_tq;
    Coord xy_pos{0,0,0}, e_pos{0,0,0};

    ToolheadTiming timing;
    timing.buffer_time_high = cfg.buffer_time_high;
    timing.buffer_time_start = 0.250;
    timing.move_flush_time = 0.050;
    timing.kin_flush_delay = 0.001;
    timing.move_batch_time = 0.500;

    KlipperHostToolhead toolhead(cfg.limits, timing,
        [&](const std::vector<PlanMove>& batch, double batch_start) {
            double move_time = batch_start;
            for (const PlanMove& m : batch) {
                Coord xr{ m.axes_r[0], m.axes_r[1], m.axes_r[2] };
                xy_tq.append(move_time, batch_start, m.accel_t, m.cruise_t, m.decel_t,
                             xy_pos, xr, m.start_v, m.cruise_v, m.accel, m.line_idx);
                xy_pos.x += m.axes_d[0];
                xy_pos.y += m.axes_d[1];
                xy_pos.z += m.axes_d[2];

                double de = m.axes_d[3];
                double escale = (m.move_d > 1e-12) ? (de / m.move_d) : 0.0;
                double has_xy = m.is_kinematic_move ? 1.0 : 0.0;
                e_tq.append(move_time, batch_start, m.accel_t, m.cruise_t, m.decel_t,
                            e_pos, Coord{1, has_xy, 0},
                            m.start_v * escale, m.cruise_v * escale, m.accel * escale,
                            m.line_idx);
                e_pos.x += de;
                move_time += m.accel_t + m.cruise_t + m.decel_t;
            }
        });

    double cur[4]{0,0,0,0};
    bool has_seed = false;
    for (const GMove& g : gcode.moves) {
        if (!has_seed) {
            cur[0] = g.x;
            cur[1] = g.y;
            cur[2] = g.z;
            cur[3] = 0.0;
            has_seed = true;
        }
        double ep[4] = { g.x, g.y, g.z, cur[3] + g.e };
        ToolheadLimits ml = cfg.limits;
        ml.max_accel = g.accel;
        ml.max_accel_to_decel = g.accel_to_decel;
        ml.square_corner_velocity = g.square_corner_velocity;
        ml.junction_deviation = calc_jd(ml.square_corner_velocity, ml.max_accel);
        PlanMove pm;
        pm.init(ml, cur, ep, g.v);
        pm.line_idx = g.line_idx;
        toolhead.update_limits(ml);
        toolhead.add_move(pm);
        cur[0] = g.x; cur[1] = g.y; cur[2] = g.z; cur[3] = ep[3];
    }
    toolhead.finish();
    const auto& windows = toolhead.flush_windows();
    const auto& moves = xy_tq.moves();

    std::printf("{\"mcu_freq\":%.17g,\"max_error\":%.17g,\"xy_step_dist\":%.17g,"
                "\"window_count\":%zu,\"move_count\":%zu,\n",
                cfg.mcu_freq, cfg.max_stepper_error, cfg.xy_step_dist,
                windows.size(), moves.size());
    std::printf("\"windows\":[");
    for (size_t i = 0; i < windows.size(); ++i) {
        const auto& w = windows[i];
        if (i) std::printf(",");
        std::printf("[%.17g,%.17g]", w.sg_flush_time, w.mcu_flush_time);
    }
    std::printf("],\n\"xy_moves\":[");
    for (size_t i = 0; i < moves.size(); ++i) {
        const auto& m = moves[i];
        if (i) std::printf(",");
        std::printf("[%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g]",
                    m.print_time, m.move_t, m.start_v, m.half_accel * 2.0,
                    m.start_pos.x, m.start_pos.y, m.start_pos.z,
                    m.axes_r.x, m.axes_r.y, m.axes_r.z);
    }
    std::printf("]}\n");
    return 0;
}
