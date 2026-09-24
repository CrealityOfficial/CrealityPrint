#ifndef slic3r_GCode_PathologicalSegmentProbe_hpp_
#define slic3r_GCode_PathologicalSegmentProbe_hpp_

#include "KlipperSim/KlipperSimMain.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Slic3r {

enum class PathologicalProbeStatus {
    Safe,
    RiskDetected,
    Cancelled,
    Unsupported,
    SimulationError
};

enum class PathologicalProbeSource {
    None,
    MainMcu,
    NozzleMcu
};

const char* pathological_probe_status_name(PathologicalProbeStatus status);
const char* pathological_probe_source_name(PathologicalProbeSource source);

struct PathologicalProbeResult {
    PathologicalProbeStatus status = PathologicalProbeStatus::Safe;
    PathologicalProbeSource source = PathologicalProbeSource::None;
    double threshold = 0.80;
    double occupancy = 0.0;
    int occupied_slots = 0;
    int total_slots = 0;
    int line_idx = -1;
    double simulated_time = -1.0;
    uint64_t file_bytes = 0;
    uint64_t bytes_read = 0;
    size_t lines_read = 0;
    double read_time_ms = 0.0;
    double hash_time_ms = 0.0;
    double parse_time_ms = 0.0;
    double simulate_time_ms = 0.0;
    double stream_consume_time_ms = 0.0;
    double gcode_parse_dispatch_time_ms = 0.0;
    double move_callback_time_ms = 0.0;
    double toolhead_time_ms = 0.0;
    double barrier_time_ms = 0.0;
    double position_time_ms = 0.0;
    double aux_event_time_ms = 0.0;
    double aux_resolve_time_ms = 0.0;
    double step_prepare_time_ms = 0.0;
    double step_generate_a_time_ms = 0.0;
    double step_generate_b_time_ms = 0.0;
    double step_generate_e_time_ms = 0.0;
    double step_generation_wall_time_ms = 0.0;
    size_t step_generation_invocations = 0;
    size_t parallel_generation_invocations = 0;
    size_t serial_generation_invocations = 0;
    size_t parallel_phase_total = 0;
    size_t serial_phase_total = 0;
    size_t max_generation_phases = 0;
    double step_mcu_check_time_ms = 0.0;
    double step_main_flush_time_ms = 0.0;
    double step_nozzle_flush_time_ms = 0.0;
    double step_publish_time_ms = 0.0;
    double serial_dispatch_time_ms = 0.0;
    double trapq_release_time_ms = 0.0;
    double finalize_time_ms = 0.0;
    bool parallel_step_generation = false;
    bool microsegment_batching = false;
    size_t move_batch_count = 0;
    size_t batched_move_count = 0;
    size_t forced_batch_flush_count = 0;
    size_t max_moves_per_batch = 0;
    // Incremental process_gcode_line pipeline timeline and internal costs.
    double pipeline_input_time_ms = 0.0;
    double line_handoff_time_ms = 0.0;
    double line_collect_time_ms = 0.0;
    double input_wait_time_ms = 0.0;
    double result_wait_time_ms = 0.0;
    double completion_to_ui_time_ms = 0.0;
    double completion_steady_time_ms = 0.0;
    bool streaming_fast_path = false;
    bool used_reference_fallback = false;
    // Wall-clock time of file reading, hashing, parsing and Boolean Probe execution.
    double wall_time_ms = 0.0;
    size_t lines_consumed = 0;
    size_t main_commands_consumed = 0;
    size_t nozzle_commands_consumed = 0;
    size_t main_commands_total = 0;
    size_t nozzle_commands_total = 0;
    std::string error_message;

    bool needs_protection() const
    {
        return status == PathologicalProbeStatus::RiskDetected;
    }
};

struct PathologicalProbeTicket {
    uint64_t gcode_hash = 0;
    uint64_t sim_config_hash = 0;
    PathologicalProbeResult result;
};

enum class PathologicalAnalysisStatus {
    Complete,
    Cancelled,
    Unsupported,
    SimulationError
};

// Full-provenance result used by the protection filter. Parser, toolhead and
// step generation run continuously; EOF performs the shared authoritative
// auxiliary/serial/provenance pass. Unlike the Boolean probe, this analysis
// never stops at the first threshold crossing.
struct PathologicalSegmentAnalysisResult {
    static constexpr uint32_t schema_version = 2;

    PathologicalAnalysisStatus status = PathologicalAnalysisStatus::SimulationError;
    uint32_t schema = schema_version;
    double threshold = 0.80;
    uint64_t sim_config_hash = 0;
    std::vector<std::string> source_lines;
    std::vector<char> flags;
    std::vector<double> positive_extrusion_cruise_by_line;
    std::vector<double> positive_extrusion_distance_by_line;
    KlipperSim::LineAnalysisState line_state;
    double collect_time_ms = 0.0;
    double simulate_time_ms = 0.0;
    double wall_time_ms = 0.0;
    std::string error_message;
};

// Streams each parsed generated line to a dedicated worker. Parser, toolhead
// and step generation run continuously; EOF drains them and performs the
// shared auxiliary/serial/provenance pass. The analysis never stops at the
// first threshold crossing.
class PathologicalLineAnalysis {
public:
    explicit PathologicalLineAnalysis(
        const KlipperSim::SimConfig& config = {}, double threshold = 0.80);
    ~PathologicalLineAnalysis();

    PathologicalLineAnalysis(const PathologicalLineAnalysis&) = delete;
    PathologicalLineAnalysis& operator=(const PathologicalLineAnalysis&) = delete;

    void consume_line(std::string line);
    void finish_input();
    std::shared_ptr<const PathologicalSegmentAnalysisResult> wait_result(
        const std::function<void()>& cancel = {});
    void cancel();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

// Receives raw G-code lines from GCodeProcessor::process_gcode_line() without
// reopening the generated file. Input collection and probe execution live on
// a dedicated worker so the slicing thread only pays the line handoff cost.
class PathologicalLineProbe {
public:
    explicit PathologicalLineProbe(
        const KlipperSim::SimConfig& config = {},
        double threshold = 0.80,
        const std::function<void()>& result_ready_callback = {});
    ~PathologicalLineProbe();

    PathologicalLineProbe(const PathologicalLineProbe&) = delete;
    PathologicalLineProbe& operator=(const PathologicalLineProbe&) = delete;

    void consume_line(std::string line);
    void finish_input();
    PathologicalProbeResult wait_result(
        const std::function<void()>& cancel = {});
    void cancel();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

class PathologicalSegmentProbe {
public:
    static PathologicalProbeResult probe_lines(
        const std::vector<std::string>& lines,
        const KlipperSim::SimConfig& config = {},
        double threshold = 0.80,
        const std::function<void()>& cancel = {});

    static PathologicalProbeResult probe_file(
        const std::string& path,
        const KlipperSim::SimConfig& config = {},
        double threshold = 0.80,
        const std::function<void()>& cancel = {});

    static PathologicalProbeTicket probe_file_ticket(
        const std::string& path,
        const KlipperSim::SimConfig& config = {},
        double threshold = 0.80,
        const std::function<void()>& cancel = {});

    static bool ticket_matches_file(
        const PathologicalProbeTicket& ticket,
        const std::string& path,
        const KlipperSim::SimConfig& config = {});

    static bool gcode_hash_file(const std::string& path, uint64_t& hash);
    static uint64_t sim_config_hash(const KlipperSim::SimConfig& config = {});
};

} // namespace Slic3r

#endif
