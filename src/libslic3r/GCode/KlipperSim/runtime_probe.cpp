#include "KlipperSimMain.hpp"
#include "KlipperTrapQ.hpp"
#include "KlipperItersolve.hpp"
#include "KlipperExtruderPA.hpp"
#include <cstdio>
#include <cstdlib>
#include <vector>
using namespace Slic3r::KlipperSim;
static double calc_jd(double scv, double accel) {
    return accel > 0.0 ? scv * scv * (std::sqrt(2.0) - 1.0) / accel : 0.0;
}
int main(int argc, char** argv) {
    setvbuf(stdout, nullptr, _IONBF, 0);
    if (argc < 4) {
        std::fprintf(stderr, "usage: runtime_probe <gcode> <lo> <hi>\n");
        return 2;
    }
    size_t lo = (size_t)std::strtoull(argv[2], nullptr, 10);
    size_t hi = (size_t)std::strtoull(argv[3], nullptr, 10);
    std::vector<GMove> gmoves = parse_gcode(argv[1], lo, hi);
    std::printf("parsed moves=%zu\n", gmoves.size());
    SimConfig cfg;
    KlipperTrapQ xy_tq, e_tq;
    Coord xy_pos{0,0,0}, e_pos{0,0,0};
    size_t plan_moves = 0;
    auto process = [&](const std::vector<PlanMove>& batch, double batch_start){
        std::printf("process batch size=%zu batch_start=%.6f\n", batch.size(), batch_start);
        double move_time = batch_start;
        for (const PlanMove& m : batch) {
            double at=m.accel_t, ct=m.cruise_t, dt=m.decel_t;
            double accel = m.accel;
            Coord xr{ m.axes_r[0], m.axes_r[1], m.axes_r[2] };
            if (!(at>=0)) at=0; if(!(ct>=0)) ct=0; if(!(dt>=0)) dt=0;
            xy_tq.append(move_time, batch_start, at, ct, dt, xy_pos, xr, m.start_v, m.cruise_v, accel);
            xy_pos.x += m.axes_d[0]; xy_pos.y += m.axes_d[1]; xy_pos.z += m.axes_d[2];
            double de = m.axes_d[3];
            double escale = (m.move_d>1e-12) ? (de / m.move_d) : 0.0;
            double has_xy = m.is_kinematic_move ? 1.0 : 0.0;
            e_tq.append(move_time, batch_start, at, ct, dt, e_pos, Coord{1, has_xy, 0},
                        m.start_v*escale, m.cruise_v*escale, accel*escale);
            e_pos.x += de;
            move_time += at+ct+dt;
            ++plan_moves;
        }
        std::printf("process batch done plan_moves=%zu move_time=%.6f\n", plan_moves, move_time);
    };
    KlipperHostToolhead toolhead(cfg.limits, ToolheadTiming{}, process);
    double cur[4]={0,0,0,0};
    bool has_seed=false;
    size_t add_count = 0;
    for (const GMove& g : gmoves) {
        if (!has_seed) {
            cur[0] = g.x; cur[1] = g.y; cur[2] = g.z; cur[3] = 0.0;
            has_seed = true;
            std::printf("seed x=%.6f y=%.6f z=%.6f\n", g.x, g.y, g.z);
            continue;
        }
        double ep[4] = { g.x, g.y, g.z, cur[3] + g.e };
        ToolheadLimits ml = cfg.limits;
        ml.max_accel = g.accel;
        ml.max_accel_to_decel = g.accel_to_decel;
        ml.square_corner_velocity = g.square_corner_velocity;
        ml.junction_deviation = calc_jd(ml.square_corner_velocity, ml.max_accel);
        PlanMove pm;
        pm.init(ml, cur, ep, g.v);
        toolhead.update_limits(ml);
        toolhead.add_move(pm);
        ++add_count;
        std::printf("added=%zu move_d=%.6f de=%.6f v=%.6f\n", add_count, pm.move_d, pm.axes_d[3], g.v);
        cur[0]=g.x; cur[1]=g.y; cur[2]=g.z; cur[3]=ep[3];
    }
    std::printf("before finish add_count=%zu\n", add_count);
    toolhead.finish();
    std::printf("after finish\n");
    std::printf("plan_moves=%zu xy_trap=%zu e_trap=%zu flush_windows=%zu print_time=%.6f\n",
                plan_moves, xy_tq.moves().size(), e_tq.moves().size(),
                toolhead.flush_windows().size(), toolhead.last_kin_move_time());
    auto cxp=[](const TrapMove&m,double t){Coord c=m.get_coord(t);return c.x+c.y;};
    auto cxm=[](const TrapMove&m,double t){Coord c=m.get_coord(t);return c.x-c.y;};
    auto xyact=[](const TrapMove&m){return m.axes_r.x!=0.0||m.axes_r.y!=0.0;};
    KlipperItersolve sA(cxp,cfg.xy_step_dist), sB(cxm,cfg.xy_step_dist);
    auto stA=sA.generate_steps(xy_tq,xyact);
    std::printf("steps A=%zu\n", stA.size());
    auto stB=sB.generate_steps(xy_tq,xyact);
    std::printf("steps B=%zu\n", stB.size());
    KlipperExtruderPA pa(cfg.pressure_advance, cfg.pa_smooth_time);
    KlipperItersolve sE([](const TrapMove&m,double t){return m.start_pos.x+m.get_distance(t);}, cfg.e_step_dist);
    std::vector<double> stE;
    auto eact = [](const TrapMove& m){ return std::fabs(m.start_v) > 1e-12 || std::fabs(m.half_accel) > 1e-12; };
    sE.set_active_window(0.5 * cfg.pa_smooth_time, 0.5 * cfg.pa_smooth_time);
    if (cfg.pressure_advance > 0.0 && cfg.pa_smooth_time > 0.0) {
        stE = sE.generate_steps_indexed(
            e_tq.moves(),
            [&pa](const std::vector<TrapMove>& mv, size_t mi, double t){
                return pa.calc_position(mv, mi, t);
            },
            eact);
    } else {
        stE = sE.generate_steps(e_tq,[](const TrapMove&){return true;});
    }
    std::printf("steps E=%zu\n", stE.size());
    return 0;
}
