#ifndef slic3r_GCode_KlipperSim_KlipperStreamingEarlyProbe_hpp_
#define slic3r_GCode_KlipperSim_KlipperStreamingEarlyProbe_hpp_

#include "KlipperSimMain.hpp"

#include <deque>
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

namespace Slic3r { namespace KlipperSim {

enum class ProbeConsumeState {
    Continue,
    RiskDetected,
    Safe,
    NeedReferenceFallback,
    Error
};

// Records when a 2.5-second execution-time window contains at least 100
// consecutive spatial moves no longer than 0.09 mm. Extrusion and travel are
// both included; pure-E and zero-XYZ moves are not spatial evidence.
class PathologicalMotionHistory {
public:
    void append_planned_batch(const std::vector<PlanMove>& batch,
                              double batch_start);
    bool has_microsequence(double hit_time) const;

private:
    struct TimedMove {
        double start_time = 0.0;
        double end_time = 0.0;
    };
    struct HitInterval {
        double start_time = 0.0;
        double end_time = 0.0;
    };

    std::deque<TimedMove> m_trailing_micro_moves;
    std::vector<HitInterval> m_hit_intervals;
};

// Stateful, bounded-memory form of the early probe. It preserves parser,
// lookahead, step-generation and serial-queue state across individual lines.
class StreamingEarlyProbeSession {
public:
    StreamingEarlyProbeSession(
        const SimConfig& config,
        double threshold,
        const LineAnalysisState& initial_state = {},
        const std::function<void()>& cancel = {});
    ~StreamingEarlyProbeSession();

    StreamingEarlyProbeSession(const StreamingEarlyProbeSession&) = delete;
    StreamingEarlyProbeSession& operator=(const StreamingEarlyProbeSession&) = delete;

    ProbeConsumeState consume_line(const std::string& line, int line_idx);
    ProbeConsumeState finish();
    const EarlyRiskResult& result() const;
    const LineAnalysisState& line_state() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

// Full-provenance generation-time analysis. It preserves parser, toolhead and
// step-generation state across individual lines, then runs auxiliary dispatch,
// serial queues and occupancy provenance through the authoritative whole-file
// ordering path at EOF. It never stops at the first threshold crossing.
struct StreamingLineAnalysisResult {
    std::vector<char> flags;
    std::vector<double> positive_extrusion_cruise_by_line;
    std::vector<double> positive_extrusion_distance_by_line;
    LineAnalysisState line_state;
};

class StreamingLineAnalysisSession {
public:
    StreamingLineAnalysisSession(
        const SimConfig& config,
        double threshold,
        const LineAnalysisState& initial_state = {},
        const std::function<void()>& cancel = {});
    ~StreamingLineAnalysisSession();

    StreamingLineAnalysisSession(const StreamingLineAnalysisSession&) = delete;
    StreamingLineAnalysisSession& operator=(const StreamingLineAnalysisSession&) = delete;

    void consume_line(const std::string& line, int line_idx);
    void finish();
    bool supported() const;
    const StreamingLineAnalysisResult& result() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

// Returns false when the input requires the P1 reference collector (currently
// auxiliary PWM/digital commands whose execution time is finalized at EOF).
bool run_streaming_early_probe(
    const std::vector<std::string>& lines,
    const SimConfig& config,
    double threshold,
    LineAnalysisState& state,
    const std::function<void()>& cancel,
    EarlyRiskResult& result);

}} // namespace Slic3r::KlipperSim

#endif
