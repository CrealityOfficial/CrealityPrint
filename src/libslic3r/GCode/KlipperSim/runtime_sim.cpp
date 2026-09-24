#include "KlipperSimMain.hpp"
#include <cstdio>
#include <cstdlib>
#include <vector>
using namespace Slic3r::KlipperSim;
int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: runtime_sim <gcode> [lo] [hi]\n");
        return 2;
    }
    size_t lo = argc > 2 ? (size_t)std::strtoull(argv[2], nullptr, 10) : 1;
    size_t hi = argc > 3 ? (size_t)std::strtoull(argv[3], nullptr, 10) : (size_t)-1;
    ParsedGCode gcode = parse_gcode_detailed(argv[1], lo, hi);
    SimConfig cfg;
    SimReport rep = simulate_full(gcode, cfg);
    std::printf("window=[%zu,%zu] moves=%zu print_time=%.6f xy_cmds=%zu e_cmds=%zu\n",
                lo, hi, gcode.moves.size(), rep.print_time_total, rep.xy_cmds, rep.e_cmds);
    std::printf("toolhead batches=%zu max_moves=%zu short=%zu span=%.4f source_t=%.6f lines=%d..%d\n",
                rep.toolhead_batch_count, rep.toolhead_max_batch_moves,
                rep.toolhead_max_batch_short_moves, rep.toolhead_max_batch_span,
                rep.toolhead_max_batch_start, rep.toolhead_max_batch_first_line,
                rep.toolhead_max_batch_last_line);
    std::printf("mcu pool_total=%d host_slots=%d peak=%d frac=%.4f overflow=%s peak_t=%.6f\n",
                rep.mcu_total, rep.mcu_host_slots, rep.mcu_peak,
                rep.mcu_total ? (double)rep.mcu_peak / rep.mcu_total : 0.0,
                rep.mcu_overflow ? "YES" : "no", rep.mcu_peak_t);
    std::printf("nozzle pool_total=%d host_slots=%d peak=%d frac=%.4f overflow=%s peak_t=%.6f\n",
                rep.nozzle_total, rep.nozzle_host_slots, rep.nozzle_peak,
                rep.nozzle_total ? (double)rep.nozzle_peak / rep.nozzle_total : 0.0,
                rep.nozzle_overflow ? "YES" : "no", rep.nozzle_peak_t);
    std::printf("nozzle_late_batch peak=%d frac=%.4f overflow=%s peak_t=%.6f "
                "source_t=%.6f lines=%d..%d commands=%zu\n",
                rep.nozzle_late_batch_peak,
                rep.nozzle_total ? (double)rep.nozzle_late_batch_peak / rep.nozzle_total : 0.0,
                rep.nozzle_late_batch_overflow ? "YES" : "no",
                rep.nozzle_late_batch_peak_t,
                rep.nozzle_late_batch_source_t,
                rep.nozzle_late_batch_first_line,
                rep.nozzle_late_batch_last_line,
                rep.nozzle_late_batch_commands);
    if (!rep.first_overflow_mcu.empty())
        std::printf("first_overflow=%s time=%.6f\n",
                    rep.first_overflow_mcu.c_str(), rep.first_overflow_time);
    std::printf("mcu_samples=%zu nozzle_samples=%zu\n",
                rep.mcu_samples.size(), rep.nozzle_samples.size());
    for (size_t i = 0; i < rep.mcu_samples.size() && i < 8; ++i)
        std::printf("mcu_sample[%zu] t=%.3f occ=%d\n",
                    i, rep.mcu_samples[i].first, rep.mcu_samples[i].second);
    for (size_t i = 0; i < rep.nozzle_samples.size() && i < 8; ++i)
        std::printf("noz_sample[%zu] t=%.3f occ=%d\n",
                    i, rep.nozzle_samples[i].first, rep.nozzle_samples[i].second);
    return 0;
}
