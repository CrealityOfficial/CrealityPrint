#include "KlipperSimMain.hpp"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#endif

using namespace Slic3r::KlipperSim;

static double peak_rss_mb()
{
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS counters{};
    counters.cb = sizeof(counters);
    return GetProcessMemoryInfo(GetCurrentProcess(), &counters,
                                sizeof(counters))
        ? static_cast<double>(counters.PeakWorkingSetSize) / (1024.0 * 1024.0)
        : 0.0;
#else
    rusage usage{};
    getrusage(RUSAGE_SELF, &usage);
    return static_cast<double>(usage.ru_maxrss) / 1024.0;
#endif
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::fprintf(stderr, "usage: streaming_benchmark <gcode>\n");
        return 2;
    }
    std::ifstream input(argv[1], std::ios::binary);
    if (!input) {
        std::fprintf(stderr, "cannot open: %s\n", argv[1]);
        return 2;
    }
    std::vector<std::string> lines;
    std::string line;
    size_t bytes = 0;
    while (std::getline(input, line)) {
        line.push_back('\n');
        bytes += line.size();
        lines.push_back(std::move(line));
    }

    SimConfig cfg;
    LineAnalysisState state;
    LineAnalysisDebug debug;
    const auto wall_start = std::chrono::steady_clock::now();
    const std::clock_t cpu_start = std::clock();
    const std::vector<char> flags = analyze_gcode_lines(
        lines, cfg, 0.80, state, &debug, {}, true);
    const double cpu_s = static_cast<double>(std::clock() - cpu_start)
                       / CLOCKS_PER_SEC;
    const double wall_s = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - wall_start).count();
    size_t flagged = 0;
    for (char hit : flags)
        flagged += hit ? 1u : 0u;
    const double throughput = wall_s > 0.0
        ? static_cast<double>(lines.size()) / wall_s : 0.0;
    std::printf(
        "lines=%zu bytes=%zu wall_s=%.6f cpu_s=%.6f peak_rss_mb=%.3f "
        "lines_per_s=%.1f flagged=%zu mcu_peak=%.6f nozzle_peak=%.6f "
        "nozzle_late_batch_peak=%.6f nozzle_physical_upper=%.6f batches=%zu max_batch=%zu short=%zu "
        "batch_span=%.6f batch_lines=%d..%d\n",
        lines.size(), bytes, wall_s, cpu_s, peak_rss_mb(), throughput, flagged,
        debug.mcu_peak_frac, debug.nozzle_peak_frac,
        debug.nozzle_late_batch_peak_frac,
        debug.nozzle_physical_upper_peak_frac, debug.toolhead_batch_count,
        debug.toolhead_max_batch_moves,
        debug.toolhead_max_batch_short_moves,
        debug.toolhead_max_batch_span,
        debug.toolhead_max_batch_first_line,
        debug.toolhead_max_batch_last_line);
    return 0;
}
