// Verify move planning: extract trapezoidal params from both simulate_full and
// analyze_gcode_lines for the same gcode window and compare per-move data.
//
// Build: g++ -O2 -std=c++17 -I. verify_moves.cpp KlipperToolhead.cpp \
//        KlipperTrapQ.cpp KlipperItersolve.cpp KlipperExtruderPA.cpp \
//        KlipperStepCompress.cpp KlipperSteppersync.cpp KlipperSimMain.cpp \
//        -o verify_moves.exe

#include "KlipperSimMain.hpp"
#include "KlipperTrapQ.hpp"
#include "KlipperItersolve.hpp"
#include "KlipperStepCompress.hpp"
#include "KlipperSteppersync.hpp"
#include "KlipperExtruderPA.hpp"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>

using namespace Slic3r::KlipperSim;

static std::vector<std::string> read_window(const std::string& path, size_t lo, size_t hi)
{
    std::vector<std::string> out;
    std::ifstream f(path);
    std::string line; size_t ln = 0;
    while (std::getline(f, line)) {
        ++ln;
        if (ln < lo) continue;
        if (ln > hi) break;
        out.push_back(line + "\n");
    }
    return out;
}

int main(int argc, char** argv)
{
    if (argc < 4) {
        fprintf(stderr, "usage: verify_moves <gcode> <lo> <hi> [n_moves]\n");
        return 2;
    }
    const char* path = argv[1];
    size_t lo = (size_t)strtoull(argv[2], nullptr, 10);
    size_t hi = (size_t)strtoull(argv[3], nullptr, 10);
    int n_dump = (argc > 4) ? atoi(argv[4]) : 30;

    // ---- Path A: simulate_full (continuous lookahead) ----
    printf("=== Path A: simulate_full (continuous) ===\n");
    printf("%-4s %-10s %-10s %-9s %-9s %-9s %-9s %-9s %-9s\n",
           "N", "move_d", "vel", "start_v", "cruise_v", "end_v", "accel_t", "cruise_t", "decel_t");

    {
        std::vector<GMove> moves = parse_gcode(path, lo, hi);
        SimConfig cfg;

        KlipperTrapQ xy_tq, e_tq;
        double print_time = 0.0;
        Coord xy_pos{0,0,0}, e_pos{0,0,0};
        int move_num = 0;

        auto process = [&](const std::vector<PlanMove>& batch){
            for (const PlanMove& m : batch) {
                double at=m.accel_t, ct=m.cruise_t, dt=m.decel_t;
                if(!(at>=0))at=0; if(!(ct>=0))ct=0; if(!(dt>=0))dt=0;
                Coord xr{ m.axes_r[0], m.axes_r[1], m.axes_r[2] };
                xy_tq.append(print_time,at,ct,dt,xy_pos,xr,m.start_v,m.cruise_v,m.accel);
                xy_pos.x+=m.axes_d[0]; xy_pos.y+=m.axes_d[1]; xy_pos.z+=m.axes_d[2];
                double de=m.axes_d[3];
                double escale=(m.move_d>1e-12)?(de/m.move_d):0.0;
                double has_xy=m.is_kinematic_move?1.0:0.0;
                e_tq.append(print_time,at,ct,dt,e_pos,Coord{1,has_xy,0},
                            m.start_v*escale,m.cruise_v*escale,m.accel*escale);
                e_pos.x+=de;
                print_time+=at+ct+dt;

                if (move_num < n_dump) {
                    printf("%-4d %-10.4f %-10.1f %-9.3f %-9.3f %-9.3f %-9.5f %-9.5f %-9.5f\n",
                           move_num, m.move_d, (m.move_d/(at+ct+dt+1e-9)),
                           m.start_v, m.cruise_v, m.end_v, at, ct, dt);
                }
                ++move_num;
            }
        };

        KlipperMoveQueue mq(cfg.limits, process);
        double cur[4]={0,0,0,0};
        double first_x=0, first_y=0, first_z=0, first_e=0;
        bool got_first=false;
        for (size_t i = 0; i < moves.size(); ++i) {
            const GMove& g = moves[i];
            if (!got_first) {
                first_x = g.x; first_y = g.y; first_z = g.z; first_e = 0;
                got_first = true;
                cur[0]=first_x; cur[1]=first_y; cur[2]=first_z; cur[3]=first_e;
            }
            double ep[4] = { g.x, g.y, g.z, cur[3] + g.e };
            PlanMove pm; pm.init(cfg.limits, cur, ep, g.v);
            mq.add_move(pm);
            cur[0]=g.x; cur[1]=g.y; cur[2]=g.z; cur[3]=ep[3];
        }
        mq.finish();
        printf("... total %d moves, print_time=%.3fs\n", move_num, print_time);
    }

    // ---- Path B: analyze_gcode_lines (per-layer) ----
    printf("\n=== Path B: analyze_gcode_lines ===\n");
    {
        std::vector<std::string> lines = read_window(path, lo, hi);
        LineAnalysisState st;
        SimConfig cfg;
        analyze_gcode_lines(lines, cfg, 0.50, st);
        printf("print_time=%.3fs\n", st.global_time);
    }

    return 0;
}
