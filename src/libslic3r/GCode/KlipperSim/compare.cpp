#include "KlipperSimMain.hpp"
#include "KlipperTrapQ.hpp"
#include <cstdio>
#include <cstdlib>
#include <cmath>
using namespace Slic3r::KlipperSim;

int main(int argc, char** argv) {
    if (argc < 3) { fprintf(stderr,"usage: compare <gcode> <move_idx>\n"); return 2; }
    const char* path = argv[1];
    int target_idx = atoi(argv[2]);

    std::vector<GMove> moves = parse_gcode(path, 1, ~size_t(0));
    if (moves.empty()) { fprintf(stderr,"no moves\n"); return 1; }
    printf("parsed %zu moves\n", moves.size());

    SimConfig cfg;
    int move_num = 0;
    bool found = false;

    auto process = [&](const std::vector<PlanMove>& batch){
        for (const PlanMove& m : batch) {
            double at=m.accel_t, ct=m.cruise_t, dt=m.decel_t;
            if(!(at>=0))at=0; if(!(ct>=0))ct=0; if(!(dt>=0))dt=0;
            if (move_num == target_idx) {
                found = true;
                printf("SIM idx=%d move_d=%.6f accel=%.1f sv=%.3f cv=%.3f ev=%.3f at=%.6f ct=%.6f dt=%.6f de=%.6f\n",
                    move_num, m.move_d, m.accel, m.start_v, m.cruise_v, m.end_v, at, ct, dt, m.axes_d[3]);
            }
            ++move_num;
        }
    };

    KlipperMoveQueue mq(cfg.limits, process);
    double cur[4]={0,0,0,0};
    bool got_first=false;
    for (const GMove& g : moves) {
        if (!got_first) { got_first=true; cur[0]=g.x;cur[1]=g.y;cur[2]=g.z;cur[3]=0; }
        double ep[4]={g.x,g.y,g.z,cur[3]+g.e};
        ToolheadLimits ml = cfg.limits;
        ml.max_accel = g.accel;
        ml.max_accel_to_decel = g.accel_to_decel;
        double scv2 = cfg.limits.junction_deviation * cfg.limits.max_accel;
        ml.junction_deviation = scv2 / g.accel;
        PlanMove pm; pm.init(ml,cur,ep,g.v);
        mq.update_limits(ml);
        mq.add_move(pm);
        cur[0]=g.x;cur[1]=g.y;cur[2]=g.z;cur[3]=ep[3];
    }
    mq.finish();
    if (!found) printf("move %d not in range (total=%d)\n", target_idx, move_num);
    return 0;
}
