#include "PathologicalSegmentProbe.hpp"
#include "KlipperSim/KlipperStreamingEarlyProbe.hpp"

#include <chrono>
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <fstream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace Slic3r {
namespace {

constexpr uint64_t fnv_offset_basis = 14695981039346656037ULL;
constexpr uint64_t fnv_prime = 1099511628211ULL;

void hash_byte(uint64_t& hash, unsigned char byte)
{
    hash ^= byte;
    hash *= fnv_prime;
}

struct FileProbeMetrics {
    uint64_t bytes_read = 0;
    size_t lines_read = 0;
    double read_time_ms = 0.0;
    double hash_time_ms = 0.0;
    double parse_time_ms = 0.0;
};

bool read_and_hash_gcode(
    const std::string& path,
    uint64_t& hash,
    std::vector<std::string>* lines,
    FileProbeMetrics* metrics = nullptr)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;

    hash = fnv_offset_basis;
    std::string line;
    bool any_byte = false;
    bool last_was_newline = false;
    char buffer[64 * 1024];
    while (input) {
        const auto read_started_at = std::chrono::steady_clock::now();
        input.read(buffer, sizeof(buffer));
        const std::streamsize count = input.gcount();
        if (metrics != nullptr) {
            metrics->read_time_ms += std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - read_started_at).count();
            metrics->bytes_read += static_cast<uint64_t>(count);
        }

        const auto hash_started_at = std::chrono::steady_clock::now();
        for (std::streamsize i = 0; i < count; ++i) {
            const unsigned char byte = static_cast<unsigned char>(buffer[i]);
            hash_byte(hash, byte);
        }
        if (metrics != nullptr)
            metrics->hash_time_ms += std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - hash_started_at).count();

        if (count > 0) {
            any_byte = true;
            last_was_newline = buffer[count - 1] == '\n';
        }
        if (lines != nullptr) {
            const auto parse_started_at = std::chrono::steady_clock::now();
            for (std::streamsize i = 0; i < count; ++i) {
                const unsigned char byte = static_cast<unsigned char>(buffer[i]);
                if (byte == '\n') {
                    lines->push_back(std::move(line));
                    line.clear();
                } else {
                    line.push_back(static_cast<char>(byte));
                }
            }
            if (metrics != nullptr)
                metrics->parse_time_ms += std::chrono::duration<double, std::milli>(
                    std::chrono::steady_clock::now() - parse_started_at).count();
        }
    }
    if (input.bad())
        return false;
    if (lines != nullptr && any_byte && !last_was_newline)
        lines->push_back(std::move(line));
    if (metrics != nullptr && lines != nullptr)
        metrics->lines_read = lines->size();
    return true;
}

uint64_t hash_text(const std::string& text)
{
    uint64_t hash = fnv_offset_basis;
    for (unsigned char byte : text)
        hash_byte(hash, byte);
    return hash;
}

PathologicalProbeSource convert_source(KlipperSim::EarlyRiskSource source)
{
    switch (source) {
    case KlipperSim::EarlyRiskSource::MainMcu:
        return PathologicalProbeSource::MainMcu;
    case KlipperSim::EarlyRiskSource::NozzleMcu:
        return PathologicalProbeSource::NozzleMcu;
    default:
        return PathologicalProbeSource::None;
    }
}

PathologicalProbeResult convert_result(
    const KlipperSim::EarlyRiskResult& low_level, double threshold)
{
    PathologicalProbeResult result;
    result.threshold = threshold;
    result.status = low_level.detected ? PathologicalProbeStatus::RiskDetected
                                       : PathologicalProbeStatus::Safe;
    result.source = convert_source(low_level.source);
    result.occupancy = low_level.occupancy;
    result.occupied_slots = low_level.occupied_slots;
    result.total_slots = low_level.total_slots;
    result.line_idx = low_level.line_idx;
    result.simulated_time = low_level.simulated_time;
    result.lines_consumed = low_level.lines_consumed;
    result.main_commands_consumed = low_level.main_commands_consumed;
    result.nozzle_commands_consumed = low_level.nozzle_commands_consumed;
    result.main_commands_total = low_level.main_commands_total;
    result.nozzle_commands_total = low_level.nozzle_commands_total;
    result.stream_consume_time_ms = low_level.stream_consume_time_ms;
    result.gcode_parse_dispatch_time_ms = low_level.gcode_parse_dispatch_time_ms;
    result.move_callback_time_ms = low_level.move_callback_time_ms;
    result.toolhead_time_ms = low_level.toolhead_time_ms;
    result.barrier_time_ms = low_level.barrier_time_ms;
    result.position_time_ms = low_level.position_time_ms;
    result.aux_event_time_ms = low_level.aux_event_time_ms;
    result.aux_resolve_time_ms = low_level.aux_resolve_time_ms;
    result.step_prepare_time_ms = low_level.step_prepare_time_ms;
    result.step_generate_a_time_ms = low_level.step_generate_a_time_ms;
    result.step_generate_b_time_ms = low_level.step_generate_b_time_ms;
    result.step_generate_e_time_ms = low_level.step_generate_e_time_ms;
    result.step_generation_wall_time_ms = low_level.step_generation_wall_time_ms;
    result.step_generation_invocations = low_level.step_generation_invocations;
    result.parallel_generation_invocations = low_level.parallel_generation_invocations;
    result.serial_generation_invocations = low_level.serial_generation_invocations;
    result.parallel_phase_total = low_level.parallel_phase_total;
    result.serial_phase_total = low_level.serial_phase_total;
    result.max_generation_phases = low_level.max_generation_phases;
    result.step_mcu_check_time_ms = low_level.step_mcu_check_time_ms;
    result.step_main_flush_time_ms = low_level.step_main_flush_time_ms;
    result.step_nozzle_flush_time_ms = low_level.step_nozzle_flush_time_ms;
    result.step_publish_time_ms = low_level.step_publish_time_ms;
    result.serial_dispatch_time_ms = low_level.serial_dispatch_time_ms;
    result.trapq_release_time_ms = low_level.trapq_release_time_ms;
    result.finalize_time_ms = low_level.finalize_time_ms;
    result.parallel_step_generation = low_level.parallel_step_generation;
    result.microsegment_batching = low_level.microsegment_batching;
    result.move_batch_count = low_level.move_batch_count;
    result.batched_move_count = low_level.batched_move_count;
    result.forced_batch_flush_count = low_level.forced_batch_flush_count;
    result.max_moves_per_batch = low_level.max_moves_per_batch;
    result.streaming_fast_path = low_level.streaming_fast_path;
    result.used_reference_fallback = low_level.used_reference_fallback;
    return result;
}
} // namespace

struct PathologicalLineProbe::Impl {
    explicit Impl(const KlipperSim::SimConfig& config, double threshold,
                  std::function<void()> result_ready_callback)
        : config(config), threshold(threshold),
          result_ready_callback(std::move(result_ready_callback)),
          worker([this] { run(); })
    {}

    ~Impl()
    {
        cancel();
        if (worker.joinable())
            worker.join();
    }

    void consume_line(std::string line)
    {
        const auto handoff_started_at = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(mutex);
        if (input_finished || result_ready
            || canceled.load(std::memory_order_relaxed))
            return;
        pending.emplace_back(std::move(line));
        line_handoff_ns.fetch_add(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now() - handoff_started_at).count(),
            std::memory_order_relaxed);
        condition.notify_one();
    }

    void finish_input()
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (!input_finished)
            pipeline_input_time_ms = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - created_at).count();
        input_finished = true;
        condition.notify_all();
    }

    void cancel()
    {
        canceled.store(true, std::memory_order_relaxed);
        std::lock_guard<std::mutex> lock(mutex);
        input_finished = true;
        condition.notify_all();
    }

    PathologicalProbeResult wait_result(const std::function<void()>& cancel_check)
    {
        std::unique_lock<std::mutex> lock(mutex);
        while (!result_ready) {
            condition.wait_for(lock, std::chrono::milliseconds(50));
            if (result_ready)
                break;
            if (cancel_check) {
                lock.unlock();
                try {
                    cancel_check();
                } catch (...) {
                    cancel();
                    throw;
                }
                lock.lock();
            }
        }
        return result;
    }

    void run()
    {
        std::vector<std::string> lines;
        uint64_t bytes_read = 0;
        size_t lines_read = 0;
        double input_wait_time_ms = 0.0;
        double line_collect_time_ms = 0.0;
        double simulate_time_ms = 0.0;
        bool reference_fallback = false;
        KlipperSim::ProbeConsumeState streaming_state =
            KlipperSim::ProbeConsumeState::Continue;

        std::string capability_error;
        if (!KlipperSim::validate_pathological_probe_config(config, &capability_error)
            || !(threshold > 0.0 && threshold <= 1.0)
            || config.mcu_pool_total <= 0 || config.nozzle_pool_total <= 0) {
            result.status = PathologicalProbeStatus::Unsupported;
            result.threshold = threshold;
            result.error_message = capability_error.empty()
                ? "invalid probe threshold or MCU pool size" : capability_error;
        } else {
            try {
                KlipperSim::StreamingEarlyProbeSession streaming(
                    config, threshold, {}, [this] {
                        if (canceled.load(std::memory_order_relaxed))
                            throw std::runtime_error("line probe cancelled");
                    });

                for (;;) {
                    std::deque<std::string> batch;
                    bool input_done = false;
                    {
                        std::unique_lock<std::mutex> lock(mutex);
                        const auto wait_started_at = std::chrono::steady_clock::now();
                        condition.wait(lock, [&] {
                            return !pending.empty() || input_finished
                                || canceled.load(std::memory_order_relaxed);
                        });
                        input_wait_time_ms += std::chrono::duration<double, std::milli>(
                            std::chrono::steady_clock::now() - wait_started_at).count();
                        batch.swap(pending);
                        input_done = input_finished
                            || canceled.load(std::memory_order_relaxed);
                    }

                    for (std::string& line : batch) {
                        bytes_read += line.size() + 1;
                        ++lines_read;
                        if (!reference_fallback) {
                            const auto simulate_started_at = std::chrono::steady_clock::now();
                            streaming_state = streaming.consume_line(
                                line, static_cast<int>(lines_read - 1));
                            simulate_time_ms += std::chrono::duration<double, std::milli>(
                                std::chrono::steady_clock::now() - simulate_started_at).count();
                            if (streaming_state
                                == KlipperSim::ProbeConsumeState::NeedReferenceFallback)
                                reference_fallback = true;
                            else if (streaming_state
                                     == KlipperSim::ProbeConsumeState::RiskDetected)
                                break;
                        }
                        const auto collect_started_at = std::chrono::steady_clock::now();
                        lines.emplace_back(std::move(line));
                        line_collect_time_ms += std::chrono::duration<double, std::milli>(
                            std::chrono::steady_clock::now() - collect_started_at).count();
                    }

                    if (streaming_state
                        == KlipperSim::ProbeConsumeState::RiskDetected) {
                        result = convert_result(streaming.result(), threshold);
                        lines.clear();
                        std::lock_guard<std::mutex> lock(mutex);
                        pending.clear();
                        break;
                    }
                    if (input_done)
                        break;
                }

                if (canceled.load(std::memory_order_relaxed)) {
                    result.status = PathologicalProbeStatus::Cancelled;
                    result.threshold = threshold;
                } else if (streaming_state
                           != KlipperSim::ProbeConsumeState::RiskDetected) {
                    if (reference_fallback) {
                        const auto simulate_started_at = std::chrono::steady_clock::now();
                        result = PathologicalSegmentProbe::probe_lines(
                            lines, config, threshold, [this] {
                                if (canceled.load(std::memory_order_relaxed))
                                    throw std::runtime_error("line probe cancelled");
                            });
                        simulate_time_ms += std::chrono::duration<double, std::milli>(
                            std::chrono::steady_clock::now() - simulate_started_at).count();
                        result.used_reference_fallback = true;
                    } else {
                        const auto simulate_started_at = std::chrono::steady_clock::now();
                        streaming_state = streaming.finish();
                        simulate_time_ms += std::chrono::duration<double, std::milli>(
                            std::chrono::steady_clock::now() - simulate_started_at).count();
                        result = convert_result(streaming.result(), threshold);
                    }
                }
            } catch (const std::exception& error) {
                result.status = canceled.load(std::memory_order_relaxed)
                    ? PathologicalProbeStatus::Cancelled
                    : PathologicalProbeStatus::SimulationError;
                result.threshold = threshold;
                result.error_message = error.what();
            } catch (...) {
                result.status = canceled.load(std::memory_order_relaxed)
                    ? PathologicalProbeStatus::Cancelled
                    : PathologicalProbeStatus::SimulationError;
                result.threshold = threshold;
                result.error_message = "unknown line probe failure";
            }
        }

        const auto completed_at = std::chrono::steady_clock::now();
        result.simulate_time_ms = simulate_time_ms;
        result.line_handoff_time_ms = static_cast<double>(
            line_handoff_ns.load(std::memory_order_relaxed)) / 1000000.0;
        result.line_collect_time_ms = line_collect_time_ms;
        result.input_wait_time_ms = input_wait_time_ms;
        result.wall_time_ms = std::chrono::duration<double, std::milli>(
            completed_at - created_at).count();
        result.completion_steady_time_ms =
            std::chrono::duration<double, std::milli>(
                completed_at.time_since_epoch()).count();
        result.file_bytes = bytes_read;
        result.bytes_read = bytes_read;
        result.lines_read = lines_read;

        {
            std::lock_guard<std::mutex> lock(mutex);
            if (pipeline_input_time_ms <= 0.0)
                pipeline_input_time_ms = result.wall_time_ms;
            result.pipeline_input_time_ms = pipeline_input_time_ms;
            result_ready = true;
            condition.notify_all();
        }
        if (result_ready_callback) {
            try {
                result_ready_callback();
            } catch (...) {
                // Completion publication must never change the probe result or
                // terminate its worker thread.
            }
        }
    }

    KlipperSim::SimConfig config;
    double threshold = 0.80;
    std::mutex mutex;
    std::condition_variable condition;
    std::deque<std::string> pending;
    bool input_finished = false;
    bool result_ready = false;
    std::atomic<bool> canceled{false};
    std::atomic<int64_t> line_handoff_ns{0};
    PathologicalProbeResult result;
    const std::chrono::steady_clock::time_point created_at =
        std::chrono::steady_clock::now();
    double pipeline_input_time_ms = 0.0;
    std::function<void()> result_ready_callback;
    std::thread worker;
};

PathologicalLineProbe::PathologicalLineProbe(
    const KlipperSim::SimConfig& config, double threshold,
    const std::function<void()>& result_ready_callback)
    : m_impl(std::make_unique<Impl>(config, threshold,
                                    result_ready_callback))
{}

PathologicalLineProbe::~PathologicalLineProbe() = default;

void PathologicalLineProbe::consume_line(std::string line)
{
    m_impl->consume_line(std::move(line));
}

void PathologicalLineProbe::finish_input()
{
    m_impl->finish_input();
}

PathologicalProbeResult PathologicalLineProbe::wait_result(
    const std::function<void()>& cancel)
{
    return m_impl->wait_result(cancel);
}

void PathologicalLineProbe::cancel()
{
    m_impl->cancel();
}

struct PathologicalLineAnalysis::Impl {
    Impl(const KlipperSim::SimConfig& config, double threshold)
        : config(config), threshold(threshold), worker([this] { run(); })
    {}

    ~Impl()
    {
        cancel();
        if (worker.joinable())
            worker.join();
    }

    void consume_line(std::string line)
    {
        const auto started_at = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(mutex);
        if (input_finished || result_ready
            || canceled.load(std::memory_order_relaxed))
            return;
        pending.emplace_back(std::move(line));
        handoff_time_ms += std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - started_at).count();
        condition.notify_one();
    }

    void finish_input()
    {
        std::lock_guard<std::mutex> lock(mutex);
        input_finished = true;
        condition.notify_all();
    }

    void cancel()
    {
        canceled.store(true, std::memory_order_relaxed);
        std::lock_guard<std::mutex> lock(mutex);
        input_finished = true;
        condition.notify_all();
    }

    std::shared_ptr<const PathologicalSegmentAnalysisResult> wait_result(
        const std::function<void()>& cancel_check)
    {
        std::unique_lock<std::mutex> lock(mutex);
        while (!result_ready) {
            condition.wait_for(lock, std::chrono::milliseconds(50));
            if (result_ready)
                break;
            if (cancel_check) {
                lock.unlock();
                try {
                    cancel_check();
                } catch (...) {
                    cancel();
                    throw;
                }
                lock.lock();
            }
        }
        return result;
    }

    void run()
    {
        const auto started_at = std::chrono::steady_clock::now();
        auto mutable_result = std::make_shared<PathologicalSegmentAnalysisResult>();
        mutable_result->threshold = threshold;
        mutable_result->sim_config_hash =
            PathologicalSegmentProbe::sim_config_hash(config);

        try {
            std::string capability_error;
            if (!KlipperSim::validate_pathological_probe_config(config, &capability_error)
                || !(threshold > 0.0 && threshold <= 1.0)) {
                mutable_result->status = PathologicalAnalysisStatus::Unsupported;
                mutable_result->error_message = capability_error.empty()
                    ? "invalid analysis threshold or MCU pool size" : capability_error;
            } else {
                KlipperSim::StreamingLineAnalysisSession streaming(
                    config, threshold, {}, [this] {
                        if (canceled.load(std::memory_order_relaxed))
                            throw std::runtime_error(
                                "pathological protection analysis cancelled");
                    });
                if (!streaming.supported()) {
                    mutable_result->status = PathologicalAnalysisStatus::Unsupported;
                    mutable_result->error_message =
                        "streaming protection analysis is unsupported";
                } else {
                    size_t line_index = 0;
                    for (;;) {
                        std::deque<std::string> batch;
                        bool done = false;
                        {
                            std::unique_lock<std::mutex> lock(mutex);
                            condition.wait(lock, [&] {
                                return !pending.empty() || input_finished
                                    || canceled.load(std::memory_order_relaxed);
                            });
                            batch.swap(pending);
                            done = input_finished
                                || canceled.load(std::memory_order_relaxed);
                        }
                        for (std::string& line : batch) {
                            const auto collect_started_at =
                                std::chrono::steady_clock::now();
                            mutable_result->source_lines.emplace_back(line);
                            mutable_result->collect_time_ms +=
                                std::chrono::duration<double, std::milli>(
                                    std::chrono::steady_clock::now()
                                    - collect_started_at).count();
                            const auto simulate_started_at =
                                std::chrono::steady_clock::now();
                            streaming.consume_line(
                                line, static_cast<int>(line_index++));
                            mutable_result->simulate_time_ms +=
                                std::chrono::duration<double, std::milli>(
                                    std::chrono::steady_clock::now()
                                    - simulate_started_at).count();
                        }
                        if (done)
                            break;
                    }

                    if (canceled.load(std::memory_order_relaxed)) {
                        mutable_result->status =
                            PathologicalAnalysisStatus::Cancelled;
                    } else {
                        const auto finish_started_at =
                            std::chrono::steady_clock::now();
                        streaming.finish();
                        mutable_result->simulate_time_ms +=
                            std::chrono::duration<double, std::milli>(
                                std::chrono::steady_clock::now()
                                - finish_started_at).count();
                        const KlipperSim::StreamingLineAnalysisResult& streamed =
                            streaming.result();
                        mutable_result->flags = streamed.flags;
                        mutable_result->positive_extrusion_cruise_by_line =
                            streamed.positive_extrusion_cruise_by_line;
                        mutable_result->positive_extrusion_distance_by_line =
                            streamed.positive_extrusion_distance_by_line;
                        mutable_result->line_state = streamed.line_state;
                        mutable_result->status =
                            PathologicalAnalysisStatus::Complete;
                    }
                }
            }
        } catch (const std::exception& error) {
            mutable_result->status = canceled.load(std::memory_order_relaxed)
                ? PathologicalAnalysisStatus::Cancelled
                : PathologicalAnalysisStatus::SimulationError;
            mutable_result->error_message = error.what();
        } catch (...) {
            mutable_result->status = canceled.load(std::memory_order_relaxed)
                ? PathologicalAnalysisStatus::Cancelled
                : PathologicalAnalysisStatus::SimulationError;
            mutable_result->error_message =
                "unknown streaming protection analysis failure";
        }

        mutable_result->collect_time_ms += handoff_time_ms;
        mutable_result->wall_time_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - started_at).count();
        {
            std::lock_guard<std::mutex> lock(mutex);
            result = std::move(mutable_result);
            result_ready = true;
            condition.notify_all();
        }
    }

    KlipperSim::SimConfig config;
    double threshold = 0.80;
    std::mutex mutex;
    std::condition_variable condition;
    std::deque<std::string> pending;
    bool input_finished = false;
    bool result_ready = false;
    std::atomic<bool> canceled{false};
    double handoff_time_ms = 0.0;
    std::shared_ptr<const PathologicalSegmentAnalysisResult> result;
    std::thread worker;
};

PathologicalLineAnalysis::PathologicalLineAnalysis(
    const KlipperSim::SimConfig& config, double threshold)
    : m_impl(std::make_unique<Impl>(config, threshold))
{}

PathologicalLineAnalysis::~PathologicalLineAnalysis() = default;

void PathologicalLineAnalysis::consume_line(std::string line)
{
    m_impl->consume_line(std::move(line));
}

void PathologicalLineAnalysis::finish_input()
{
    m_impl->finish_input();
}

std::shared_ptr<const PathologicalSegmentAnalysisResult>
PathologicalLineAnalysis::wait_result(const std::function<void()>& cancel)
{
    return m_impl->wait_result(cancel);
}

void PathologicalLineAnalysis::cancel()
{
    m_impl->cancel();
}

const char* pathological_probe_status_name(PathologicalProbeStatus status)
{
    switch (status) {
    case PathologicalProbeStatus::Safe: return "safe";
    case PathologicalProbeStatus::RiskDetected: return "risk_detected";
    case PathologicalProbeStatus::Cancelled: return "cancelled";
    case PathologicalProbeStatus::Unsupported: return "unsupported";
    case PathologicalProbeStatus::SimulationError: return "simulation_error";
    }
    return "simulation_error";
}

const char* pathological_probe_source_name(PathologicalProbeSource source)
{
    switch (source) {
    case PathologicalProbeSource::None: return "none";
    case PathologicalProbeSource::MainMcu: return "main_mcu";
    case PathologicalProbeSource::NozzleMcu: return "nozzle_mcu";
    }
    return "none";
}

PathologicalProbeResult PathologicalSegmentProbe::probe_lines(
    const std::vector<std::string>& lines,
    const KlipperSim::SimConfig& config,
    double threshold,
    const std::function<void()>& cancel)
{
    PathologicalProbeResult result;
    result.threshold = threshold;
    if (!(threshold > 0.0 && threshold <= 1.0)
        || config.mcu_pool_total <= 0 || config.nozzle_pool_total <= 0) {
        result.status = PathologicalProbeStatus::Unsupported;
        result.error_message = "invalid probe threshold or MCU pool size";
        return result;
    }

    bool cancellation_thrown = false;
    const auto guarded_cancel = [&] {
        if (!cancel)
            return;
        try {
            cancel();
        } catch (...) {
            cancellation_thrown = true;
            throw;
        }
    };

    try {
        KlipperSim::LineAnalysisState state;
        KlipperSim::EarlyRiskResult low_level;
        (void) KlipperSim::analyze_gcode_lines(
            lines, config, threshold, state, nullptr, guarded_cancel, true,
            nullptr, nullptr, &low_level);
        result = convert_result(low_level, threshold);
    } catch (const std::exception& error) {
        result.status = cancellation_thrown ? PathologicalProbeStatus::Cancelled
                                            : PathologicalProbeStatus::SimulationError;
        result.error_message = error.what();
    } catch (...) {
        result.status = cancellation_thrown ? PathologicalProbeStatus::Cancelled
                                            : PathologicalProbeStatus::SimulationError;
        result.error_message = "unknown probe failure";
    }
    return result;
}

PathologicalProbeResult PathologicalSegmentProbe::probe_file(
    const std::string& path,
    const KlipperSim::SimConfig& config,
    double threshold,
    const std::function<void()>& cancel)
{
    return probe_file_ticket(path, config, threshold, cancel).result;
}

PathologicalProbeTicket PathologicalSegmentProbe::probe_file_ticket(
    const std::string& path,
    const KlipperSim::SimConfig& config,
    double threshold,
    const std::function<void()>& cancel)
{
    const auto started_at = std::chrono::steady_clock::now();
    PathologicalProbeTicket ticket;
    ticket.sim_config_hash = sim_config_hash(config);
    std::vector<std::string> lines;
    FileProbeMetrics metrics;
    if (!read_and_hash_gcode(path, ticket.gcode_hash, &lines, &metrics)) {
        ticket.result.status = PathologicalProbeStatus::SimulationError;
        ticket.result.threshold = threshold;
        ticket.result.error_message = "cannot read G-code file: " + path;
        ticket.result.file_bytes = metrics.bytes_read;
        ticket.result.bytes_read = metrics.bytes_read;
        ticket.result.lines_read = metrics.lines_read;
        ticket.result.read_time_ms = metrics.read_time_ms;
        ticket.result.hash_time_ms = metrics.hash_time_ms;
        ticket.result.parse_time_ms = metrics.parse_time_ms;
        ticket.result.wall_time_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - started_at).count();
        return ticket;
    }
    const auto simulation_started_at = std::chrono::steady_clock::now();
    ticket.result = probe_lines(lines, config, threshold, cancel);
    ticket.result.simulate_time_ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - simulation_started_at).count();
    ticket.result.file_bytes = metrics.bytes_read;
    ticket.result.bytes_read = metrics.bytes_read;
    ticket.result.lines_read = metrics.lines_read;
    ticket.result.read_time_ms = metrics.read_time_ms;
    ticket.result.hash_time_ms = metrics.hash_time_ms;
    ticket.result.parse_time_ms = metrics.parse_time_ms;
    ticket.result.wall_time_ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - started_at).count();
    return ticket;
}

bool PathologicalSegmentProbe::ticket_matches_file(
    const PathologicalProbeTicket& ticket,
    const std::string& path,
    const KlipperSim::SimConfig& config)
{
    uint64_t current_gcode_hash = 0;
    return gcode_hash_file(path, current_gcode_hash)
        && ticket.gcode_hash == current_gcode_hash
        && ticket.sim_config_hash == sim_config_hash(config);
}

bool PathologicalSegmentProbe::gcode_hash_file(const std::string& path, uint64_t& hash)
{
    return read_and_hash_gcode(path, hash, nullptr);
}

uint64_t PathologicalSegmentProbe::sim_config_hash(const KlipperSim::SimConfig& c)
{
    std::ostringstream values;
    values << std::hexfloat
           << c.xy_step_dist << ',' << c.e_step_dist << ','
           << c.mcu_total << ',' << c.nozzle_total << ','
           << c.mcu_pool_total << ',' << c.nozzle_pool_total << ','
           << c.mcu_reserved_slots << ',' << c.nozzle_reserved_slots << ','
           << c.mcu_baseline_frac << ',' << c.nozzle_baseline_frac << ','
           << c.pressure_advance << ',' << c.pressure_advance_commands_enabled << ','
           << c.mcu_freq << ',' << c.mcu_baud << ',' << c.nozzle_baud << ','
           << c.mcu_receive_window << ',' << c.nozzle_receive_window << ','
           << c.max_stepper_error << ',' << c.buffer_time_high << ','
           << c.pa_smooth_time << ',' << c.max_extrude_only_velocity << ','
           << c.max_extrude_only_accel << ',' << c.max_z_velocity << ','
           << c.max_z_accel << ',' << c.parallel_step_generation << ','
           << c.microsegment_batching << ',' << c.microsegment_batch_size << ','
           << c.limits.max_velocity << ','
           << c.limits.max_accel << ',' << c.limits.max_accel_to_decel << ','
           << c.limits.square_corner_velocity << ',' << c.limits.junction_deviation << ','
           << c.limits.instant_corner_v;
    return hash_text(values.str());
}

} // namespace Slic3r
