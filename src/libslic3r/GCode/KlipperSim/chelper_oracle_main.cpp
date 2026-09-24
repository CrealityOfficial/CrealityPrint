#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

extern "C" {
#include "../../../../klipper_src/creality_klipper/klipper/klippy/chelper/trapq.h"
#include "../../../../klipper_src/creality_klipper/klipper/klippy/chelper/itersolve.h"
#include "../../../../klipper_src/creality_klipper/klipper/klippy/chelper/stepcompress.h"
struct stepper_kinematics *corexy_stepper_alloc(char type);
}

struct Window {
    double sg_flush_time;
    double mcu_flush_time;
};

struct MoveRow {
    double print_time, move_t, start_v, accel;
    double sx, sy, sz, rx, ry, rz;
};

static bool read_meta(const std::string& path, double& mcu_freq,
                      double& max_error, double& step_dist)
{
    std::ifstream in(path);
    return (in >> mcu_freq >> max_error >> step_dist);
}

static std::vector<Window> read_windows(const std::string& path)
{
    std::ifstream in(path);
    std::vector<Window> out;
    Window w{};
    while (in >> w.sg_flush_time >> w.mcu_flush_time)
        out.push_back(w);
    return out;
}

static std::vector<MoveRow> read_moves(const std::string& path)
{
    std::ifstream in(path);
    std::vector<MoveRow> out;
    MoveRow m{};
    while (in >> m.print_time >> m.move_t >> m.start_v >> m.accel
              >> m.sx >> m.sy >> m.sz >> m.rx >> m.ry >> m.rz)
        out.push_back(m);
    return out;
}

static int extract_count(struct stepcompress *sc)
{
    std::vector<pull_history_steps> buf(400000);
    return stepcompress_extract_old(sc, buf.data(), (int)buf.size(), 0, UINT64_MAX);
}

static int extract_single_count(struct stepcompress *sc)
{
    std::vector<pull_history_steps> buf(400000);
    int n = stepcompress_extract_old(sc, buf.data(), (int)buf.size(), 0, UINT64_MAX);
    int single = 0;
    for (int i = 0; i < n; ++i) {
        if (std::abs(buf[i].step_count) == 1)
            ++single;
    }
    return single;
}

int main(int argc, char** argv)
{
    if (argc != 4) {
        std::cerr << "usage: chelper_oracle <meta.txt> <windows.txt> <moves.txt>\n";
        return 2;
    }

    double mcu_freq = 0.0, max_error = 0.0, step_dist = 0.0;
    if (!read_meta(argv[1], mcu_freq, max_error, step_dist)) {
        std::cerr << "failed to read meta\n";
        return 1;
    }
    auto windows = read_windows(argv[2]);
    auto moves = read_moves(argv[3]);

    trapq *tq = trapq_alloc();
    for (const auto& m : moves) {
        double accel_t = 0.0, cruise_t = 0.0, decel_t = 0.0;
        if (m.accel > 0.0) {
            double disc = m.start_v * m.start_v + 2.0 * m.accel * 0.0;
            (void)disc;
        }
        // The exported rows are already trapq phases, so map them back 1:1.
        if (m.accel > 0.0)
            accel_t = m.move_t;
        else if (m.accel < 0.0)
            decel_t = m.move_t;
        else
            cruise_t = m.move_t;
        trapq_append(tq, m.print_time, accel_t, cruise_t, decel_t,
                     m.sx, m.sy, m.sz, m.rx, m.ry, m.rz,
                     m.start_v, m.start_v + m.accel * m.move_t, m.accel);
    }

    stepcompress *sc_a = stepcompress_alloc(1);
    stepcompress *sc_b = stepcompress_alloc(2);
    uint32_t maxerr_ticks = (uint32_t)llround(max_error * mcu_freq);
    stepcompress_fill(sc_a, maxerr_ticks, 0, 0);
    stepcompress_fill(sc_b, maxerr_ticks, 0, 0);

    stepper_kinematics *sk_a = corexy_stepper_alloc('+');
    stepper_kinematics *sk_b = corexy_stepper_alloc('-');
    itersolve_set_trapq(sk_a, tq);
    itersolve_set_trapq(sk_b, tq);
    itersolve_set_stepcompress(sk_a, sc_a, step_dist);
    itersolve_set_stepcompress(sk_b, sc_b, step_dist);

    stepcompress *sc_list[2] = { sc_a, sc_b };
    steppersync *ss = steppersync_alloc((serialqueue*)1, sc_list, 2, 2950);
    steppersync_set_time(ss, 0.0, mcu_freq);

    for (const auto& w : windows) {
        if (itersolve_generate_steps(sk_a, w.sg_flush_time)
            || itersolve_generate_steps(sk_b, w.sg_flush_time)
            || steppersync_flush(ss, (uint64_t)llround(w.mcu_flush_time * mcu_freq))) {
            std::cerr << "oracle execution failed\n";
            return 1;
        }
    }

    int count_a = extract_count(sc_a);
    int count_b = extract_count(sc_b);
    int single_a = extract_single_count(sc_a);
    int single_b = extract_single_count(sc_b);
    std::cout << "oracle_counts " << count_a << " " << count_b << " "
              << (count_a + count_b) << "\n";
    std::cout << "oracle_single " << single_a << " " << single_b << " "
              << (single_a + single_b) << "\n";
    return 0;
}
