#ifndef slic3r_GCode_PathologicalSegmentAccelFilter_hpp_
#define slic3r_GCode_PathologicalSegmentAccelFilter_hpp_

#include "../libslic3r.h"
#include "../PrintConfig.hpp"
#include "PathologicalSegmentProbe.hpp"

#include <string>
#include <string_view>
#include <functional>
#include <utility>
#include <vector>

namespace Slic3r {

// ---------------------------------------------------------------------------
// PathologicalSegmentAccelFilter
// ---------------------------------------------------------------------------
// A whole-file G-code post-processing filter that detects "pathological" dense
// short-segment sequences and injects SET_VELOCITY_LIMIT commands to limit
// acceleration during those regions.
//
// Detection runs the ported Klipper motion pipeline and marks commands whose
// MCU move-pool occupancy reaches the configured protection threshold.
//
// Whole-file analysis uses an incremental single-pass command pipeline. Per-stepqueue
// ordering frontiers feed the MCU pool online, while a rolling provenance
// window marks G-code lines as soon as their execution intervals are finalized.
// It does not retain the complete simulator command/event timeline.
// ---------------------------------------------------------------------------

class PathologicalSegmentAccelFilter
{
public:
    struct Config
    {
        bool  enabled{ true };
    };

    PathologicalSegmentAccelFilter() = delete;
    PathologicalSegmentAccelFilter(
        bool enabled, GCodeFlavor flavor,
        std::function<void()> cancel_callback = {},
        std::shared_ptr<PathologicalLineAnalysis> prescan = {},
        KlipperSim::SimConfig sim_config = {},
        std::function<void(int, const std::string&)> status_callback = {});
    ~PathologicalSegmentAccelFilter();

    std::string process_layer(std::string&& gcode);
    std::string process_gcode(std::string&& gcode, bool flush);
    bool        process_file(const std::string& path);
    // Called after an unchanged source line is emitted, and once at EOF.
    // Consumers may append metadata using the final output byte positions.
    using OutputCallback = std::function<void(std::string_view, std::string&, bool)>;
    void set_output_callback(OutputCallback callback) { m_output_callback = std::move(callback); }

private:
    struct State;

    std::string apply(
        std::string&& gcode, const Config& cfg, GCodeFlavor flavor,
        State& state, const std::function<void()>& cancel,
        bool compact_streaming,
        const KlipperSim::SimConfig& sim_config,
        const std::shared_ptr<const PathologicalSegmentAnalysisResult>& prescan_result,
        const std::function<void(int, const std::string&)>& status_callback);
    std::string apply_with_prescan(std::string&& gcode, bool compact_streaming);

    Config                  m_config;
    GCodeFlavor             m_flavor;
    std::function<void()>    m_cancel;
    std::shared_ptr<PathologicalLineAnalysis> m_prescan;
    KlipperSim::SimConfig m_sim_config;
    std::function<void(int, const std::string&)> m_status_callback;
    OutputCallback m_output_callback;
    std::unique_ptr<State>  m_state;
};

} // namespace Slic3r

#endif /* slic3r_GCode_PathologicalSegmentAccelFilter_hpp_ */
