// Standalone diagnostic: exercise BOTH the continuous full-sim path and the
// filter's per-layer analyze_gcode_lines path over a gcode line window, to find
// why the dense variable-width wall region is not flagged pathological.
//
// Build (g++):
//   g++ -O2 -std=c++17 -I. diag_main.cpp KlipperToolhead.cpp KlipperTrapQ.cpp \
//       KlipperItersolve.cpp KlipperExtruderPA.cpp KlipperStepCompress.cpp \
//       KlipperSteppersync.cpp KlipperSimMain.cpp -o diag.exe

#include "KlipperSimMain.hpp"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

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
    if (argc < 2) {
        std::fprintf(stderr, "usage:\n");
        std::fprintf(stderr, "  diag <gcode> <lo> <hi> [target_abs_line] [threshold]   Ã¢â‚¬?window\n");
        std::fprintf(stderr, "  diag <gcode> full [target_abs_line]                     Ã¢â‚¬?end-to-end\n");
        return 2;
    }
    std::string path = argv[1];
    std::string mode = (argc > 2) ? argv[2] : "";

    if (mode == "full") {
        // ---- End-to-end audit using engine's own code path ----
        // Reads gcode, splits by LAYER_CHANGE, calls analyze_gcode_lines
        // per layer (same as PathologicalSegmentAccelFilter::apply).
        // After all layers, peaks are read from the accumulated events.
        long target = (argc > 3) ? std::atol(argv[3]) : -1;
        (void)target;

        SimConfig cfg;
        LineAnalysisState state;
        double threshold = 0.50;

        std::ifstream f(path);
        if (!f) { std::fprintf(stderr,"cannot open %s\n",path.c_str()); return 1; }

        std::vector<std::string> batch;
        std::string l;
        size_t lineno=0, total_layers=0, total_lines=0;

        auto flush_layer = [&](){
            if (batch.empty()) return;
            ++total_layers;
            total_lines += batch.size();
            analyze_gcode_lines(batch, cfg, threshold, state);
            cfg.limits.max_accel = state.current_accel;
            cfg.limits.max_accel_to_decel = state.current_accel_to_decel;

            batch.clear();
        };

        while (std::getline(f, l)) {
            // Preserve trailing newline to match filter's line format
            if (l.find(";LAYER_CHANGE") != std::string::npos ||
                l.find("; layer change") != std::string::npos) {
                flush_layer();
            }
            batch.push_back(l + "\n");
        }
        flush_layer(); // final layer
        f.close();

        // Compute peaks from the accumulated cross-layer events
        double mcu_pk = compute_peak_occupancy(state.mcu_events,
                                                cfg.mcu_pool_total);
        double noz_pk = compute_peak_occupancy(state.noz_events,
                                                cfg.nozzle_pool_total);

        std::printf("=== end-to-end audit (engine path) ===\n");
        std::printf("  total_layers=%zu  total_lines=%zu\n", total_layers, total_lines);
        std::printf("  print_time=%.1fs (%.1fmin)\n",
                    state.global_time, state.global_time/60.0);
        std::printf("  mcu_events=%zu  nozzle_events=%zu\n",
                    state.mcu_events.size(), state.noz_events.size());
        std::printf("  mcu    peak=%.1f%%  overflow=%s\n",
                    mcu_pk*100.0, mcu_pk>=1.0?"YES":"no");
        std::printf("  nozzle peak=%.1f%%  overflow=%s\n",
                    noz_pk*100.0, noz_pk>=1.0?"YES":"no");

        // Sample first/last 5 events for sanity check
        std::printf("  first 5 mcu events:\n");
        for (size_t i=0; i<5 && i<state.mcu_events.size(); ++i)
            std::printf("    t=%.6f d=%d\n", state.mcu_events[i].first, state.mcu_events[i].second);
        std::printf("  last 5 mcu events:\n");
        for (size_t i=state.mcu_events.size()>5?state.mcu_events.size()-5:0;
             i<state.mcu_events.size(); ++i)
            std::printf("    t=%.6f d=%d\n", state.mcu_events[i].first, state.mcu_events[i].second);
        return 0;
    }

    size_t lo = (size_t)std::strtoull(argv[2], nullptr, 10);
    size_t hi = (size_t)std::strtoull(argv[3], nullptr, 10);
    long target = (argc > 4) ? std::atol(argv[4]) : -1;
    double threshold = (argc > 5) ? std::atof(argv[5]) : 0.50;

    // ---- Path A: continuous full simulation over the window ----
    {
        std::vector<GMove> moves = parse_gcode(path, lo, hi);
        SimConfig cfg;
        SimReport rep = simulate_full(moves, cfg);
        double dur = rep.print_time_total;
        std::printf("=== full-sim window [%zu,%zu] moves=%zu time=%.3fs ===\n",
                    lo, hi, moves.size(), dur);
        std::printf("  xy_cmds=%zu (%.1f/s)  e_cmds=%zu (%.1f/s)\n",
                    rep.xy_cmds, dur>0?rep.xy_cmds/dur:0.0,
                    rep.e_cmds, dur>0?rep.e_cmds/dur:0.0);
        std::printf("  mcu peak=%.1f%% (t=%.3f)  nozzle peak=%.1f%% (t=%.3f)\n",
                    rep.mcu_total?100.0*rep.mcu_peak/rep.mcu_total:0.0, rep.mcu_peak_t,
                    rep.nozzle_total?100.0*rep.nozzle_peak/rep.nozzle_total:0.0, rep.nozzle_peak_t);
    }

    // ---- Path B: filter's analyze_gcode_lines over the same window ----
    {
        std::vector<std::string> lines = read_window(path, lo, hi);
        SimConfig cfg;
        LineAnalysisState st;
        std::vector<char> flags = analyze_gcode_lines(lines, cfg, threshold, st);
        size_t nflag = 0; for (char c : flags) if (c) ++nflag;
        std::printf("=== analyze_gcode_lines (threshold=%.2f) ===\n", threshold);
        std::printf("  window lines=%zu  flagged=%zu (%.1f%%)\n",
                    lines.size(), nflag, lines.size()?100.0*nflag/lines.size():0.0);
        if (target >= 0) {
            long rel = target - (long)lo;
            if (rel >= 0 && rel < (long)flags.size()) {
                // report flag status in a +/-15 line neighborhood of target
                long a = rel-15<0?0:rel-15, b = rel+15>=(long)flags.size()?(long)flags.size()-1:rel+15;
                std::printf("  target abs line %ld (rel %ld): flag=%d\n",
                            target, rel, (int)flags[rel]);
                std::printf("  neighborhood flags [%ld..%ld]: ", a+(long)lo, b+(long)lo);
                for (long i=a;i<=b;++i) std::printf("%d", flags[i]?1:0);
                std::printf("\n");
            } else {
                std::printf("  target line %ld out of window\n", target);
            }
        }
    }
    return 0;
}
