#include "KlipperHostDispatch.hpp"
#include "KlipperSteppersync.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <limits>
#include <map>
#include <queue>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#if !defined(KLSIM_DISABLE_TBB)
#include <tbb/parallel_invoke.h>
#endif

namespace Slic3r { namespace KlipperSim {
namespace {

constexpr double PR_NOW = -1.0;
constexpr double PR_NEVER = std::numeric_limits<double>::infinity();
constexpr double MIN_REQTIME_DELTA = 0.250;
constexpr double MIN_BACKGROUND_DELTA = 0.005;
constexpr int MESSAGE_MAX = 64;
constexpr int MESSAGE_TRAILER_SIZE = 3;
constexpr int MESSAGE_HEADER_SIZE = 2;
constexpr int MESSAGE_MIN = MESSAGE_HEADER_SIZE + MESSAGE_TRAILER_SIZE;
constexpr int MESSAGE_PAYLOAD_MAX = MESSAGE_MAX - MESSAGE_MIN;
constexpr int MAX_PENDING_BLOCKS = 12;
constexpr double CLOCK_ADVANCE_TICKS = 2147483648.0;

struct CommandQueueState {
    int id = 0;
    bool active = false;
    std::deque<HostDispatchCmd*> stalled;
    std::deque<HostDispatchCmd*> ready;
};

struct SentBlock {
    uint64_t sequence = 0;
    int length = 0;
    double ack_eventtime = 0.0;
};

double calculate_bittime(const HostDispatchConfig& config, uint32_t bytes)
{
    if (config.wire_frequency <= 0.0)
        throw std::invalid_argument("KlipperHostDispatch: wire frequency must be positive");
    if (config.transport == HostTransportKind::Can) {
        constexpr uint32_t packet_bits = (1 + 11 + 3 + 4) + (16 + 2 + 7 + 3);
        constexpr uint32_t ifs_bits = 4;
        const uint32_t packets = (bytes + 7) / 8;
        const uint32_t bits = bytes * 8 + packets * packet_bits
                            - (packets ? ifs_bits : 0);
        return static_cast<double>(bits) / config.wire_frequency;
    }
    return static_cast<double>(bytes) * 10.0 / config.wire_frequency;
}

double effective_req_time(const HostDispatchCmd& command,
                          double block_min_end, double serial_idle_time,
                          bool pending)
{
    if (!command.background_priority)
        return command.req_time;
    const double background_time = pending ? block_min_end : serial_idle_time;
    return background_time + MIN_REQTIME_DELTA + MIN_BACKGROUND_DELTA;
}

class DeterministicSerialQueue {
public:
    DeterministicSerialQueue(HostDispatchConfig config,
                             std::function<double(double)> estimated_print_time,
                             HostReceiveControlCallback receive_callback,
                             bool retain_command_trace = true)
        : m_config(config)
        , m_estimated_print_time(std::move(estimated_print_time))
        , m_receive_callback(std::move(receive_callback))
        , m_retain_command_trace(retain_command_trace)
    {
        m_queues.reserve(16);
        m_active_order.reserve(16);
        m_block_commands.reserve(MESSAGE_PAYLOAD_MAX);
    }

    void append(const HostDispatchCmd& command)
    {
        HostDispatchCmd copy = command;
        append(std::move(copy));
    }

    void append(HostDispatchCmd&& command)
    {
        if (m_finished || m_control != SimulationControl::Continue)
            throw std::logic_error("KlipperHostDispatch: append after terminal state");
        command.min_time = std::max(0.0, command.min_time);
        command.enqueue_time = std::max(0.0, command.enqueue_time);
        if ((m_have_enqueue_time
             && command.enqueue_time + 1e-12 < m_last_enqueue_time)
            || command.enqueue_time + 1e-12 < m_eventtime
            || (m_have_advanced
                && command.enqueue_time <= m_eventtime + 1e-12))
            throw std::logic_error("KlipperHostDispatch: append frontier regressed enqueue=" + std::to_string(command.enqueue_time) + " event=" + std::to_string(m_eventtime) + " last=" + std::to_string(m_last_enqueue_time) + " advanced=" + std::to_string(m_have_advanced));
        command.enqueue_sequence = m_total_appended++;
        m_last_enqueue_time = command.enqueue_time;
        m_have_enqueue_time = true;
        command.recv_time = command.min_time;
        command.send_time = command.block_end_time = command.ack_time = 0.0;
        command.block_sequence = 0;
        command.block_index = -1;
        m_commands.push_back(std::move(command));
        m_incoming.push_back(&m_commands.back());
    }

    SimulationControl advance_to(double host_eventtime)
    {
        if (host_eventtime + 1e-12 < m_eventtime)
            throw std::logic_error("KlipperHostDispatch: host frontier regressed");
        m_have_advanced = true;
        size_t guard = 0;
        while (m_control == SimulationControl::Continue) {
            if (++guard > m_total_appended * 64 + 100000)
                throw std::runtime_error("KlipperHostDispatch: event loop did not converge");
            ingest(m_eventtime);
            acknowledge(m_eventtime);
            const double wake = command_event(m_eventtime);
            if (m_control != SimulationControl::Continue)
                break;
            double next_event = wake;
            if (!m_incoming.empty())
                next_event = std::min(next_event, m_incoming.front()->enqueue_time);
            if (!m_sent.empty())
                next_event = std::min(next_event, m_sent.front().ack_eventtime);
            if (!std::isfinite(next_event) || next_event > host_eventtime + 1e-12) {
                m_eventtime = host_eventtime;
                break;
            }
            if (next_event <= m_eventtime + 1e-12)
                next_event = m_eventtime + 1e-9;
            if (next_event > host_eventtime + 1e-12) {
                m_eventtime = host_eventtime;
                break;
            }
            m_eventtime = next_event;
        }
        return m_control;
    }

    SimulationControl drain_ready() { return advance_to(m_eventtime); }

    HostDispatchOutcome finish(double final_host_eventtime)
    {
        size_t guard = 0;
        while (m_dispatched < m_total_appended
               && m_control == SimulationControl::Continue) {
            if (++guard > m_total_appended * 64 + 100000)
                throw std::runtime_error("KlipperHostDispatch: event loop did not converge");
            ingest(m_eventtime);
            acknowledge(m_eventtime);
            const double wake = command_event(m_eventtime);
            if (m_control != SimulationControl::Continue)
                break;
            if (m_dispatched == m_total_appended)
                break;
            double next_event = wake;
            if (!m_incoming.empty())
                next_event = std::min(next_event, m_incoming.front()->enqueue_time);
            if (!m_sent.empty())
                next_event = std::min(next_event, m_sent.front().ack_eventtime);
            if (!std::isfinite(next_event)) {
                const double estimated = estimate(m_eventtime);
                double next_min = PR_NEVER;
                for (int id : m_active_order) {
                    const auto& queue = m_queues.at(id);
                    if (!queue.stalled.empty())
                        next_min = std::min(next_min, queue.stalled.front()->min_time);
                }
                if (std::isfinite(next_min))
                    next_event = m_eventtime + std::max(0.0, next_min - estimated);
            }
            if (!std::isfinite(next_event))
                throw std::runtime_error("KlipperHostDispatch: commands remain with no wake event");
            if (next_event <= m_eventtime + 1e-12)
                next_event = m_eventtime + 1e-9;
            m_eventtime = next_event;
        }
        if (m_control == SimulationControl::Continue) {
            while (!m_sent.empty())
                acknowledge(std::max(final_host_eventtime,
                                     m_sent.front().ack_eventtime));
        }
        m_finished = true;
        return {m_control, m_dispatched};
    }

    std::vector<HostDispatchCmd> take_commands() const
    {
        if (!m_retain_command_trace)
            throw std::logic_error("KlipperHostDispatch: trace retention disabled");
        return {m_commands.begin(), m_commands.end()};
    }

    size_t command_count() const { return m_total_appended; }
    size_t commands_consumed() const { return m_dispatched; }
    size_t retained_command_count() const { return m_commands.size(); }

private:
    void release_consumed_prefix()
    {
        if (m_retain_command_trace)
            return;
        while (!m_commands.empty()
               && m_commands.front().block_index >= 0)
            m_commands.pop_front();
    }

    double estimate(double eventtime) const
    {
        return m_estimated_print_time ? m_estimated_print_time(eventtime) : eventtime;
    }

    CommandQueueState& queue_for(int id)
    {
        auto inserted = m_queues.emplace(id, CommandQueueState{});
        inserted.first->second.id = id;
        return inserted.first->second;
    }

    void activate(CommandQueueState& queue)
    {
        if (!queue.active) {
            queue.active = true;
            m_active_order.push_back(queue.id);
        }
    }

    void deactivate_if_empty(CommandQueueState& queue)
    {
        if (!queue.active || !queue.ready.empty() || !queue.stalled.empty())
            return;
        queue.active = false;
        auto found = std::find(m_active_order.begin(), m_active_order.end(), queue.id);
        if (found != m_active_order.end())
            m_active_order.erase(found);
    }

    void ingest(double eventtime)
    {
        while (!m_incoming.empty()
               && m_incoming.front()->enqueue_time <= eventtime + 1e-12) {
            HostDispatchCmd* command = m_incoming.front();
            m_incoming.pop_front();
            if (!command->background_priority && m_config.clock_frequency > 0.0) {
                const double advance = CLOCK_ADVANCE_TICKS / m_config.clock_frequency;
                if (command->min_time + advance < command->req_time)
                    command->min_time = command->req_time - advance;
            }
            CommandQueueState& queue = queue_for(command->command_queue_id);
            activate(queue);
            queue.stalled.push_back(command);
            m_stalled_bytes += command->msg_len;
        }
    }

    void acknowledge(double eventtime)
    {
        while (!m_sent.empty() && m_sent.front().ack_eventtime <= eventtime + 1e-12) {
            const SentBlock block = m_sent.front();
            m_sent.pop_front();
            m_need_ack_bytes = std::max(0, m_need_ack_bytes - block.length);
            m_receive_seq = block.sequence + 1;
            m_last_ack_seq = m_receive_seq;
            m_last_ack_bytes = block.length;
        }
    }

    bool transport_blocked() const
    {
        if (m_send_seq - m_receive_seq >= MAX_PENDING_BLOCKS)
            return true;
        if (m_send_seq > m_receive_seq && m_config.receive_window > 0) {
            int bytes = m_need_ack_bytes + MESSAGE_MAX;
            if (m_last_ack_seq < m_receive_seq)
                bytes += m_last_ack_bytes;
            if (bytes > m_config.receive_window)
                return true;
        }
        return false;
    }

    double check_send_command(int pending, double eventtime)
    {
        if (transport_blocked())
            return PR_NEVER;
        const double idletime = std::max(eventtime, m_idle_time)
                              + calculate_bittime(m_config, pending + MESSAGE_MIN);
        const double ack_clock = estimate(idletime);
        double min_stalled_clock = PR_NEVER;
        double min_ready_clock = PR_NEVER;

        for (int id : m_active_order) {
            CommandQueueState& queue = m_queues.at(id);
            while (!queue.stalled.empty()) {
                HostDispatchCmd* command = queue.stalled.front();
                if (ack_clock + 1e-12 < command->min_time) {
                    min_stalled_clock = std::min(min_stalled_clock, command->min_time);
                    break;
                }
                queue.stalled.pop_front();
                queue.ready.push_back(command);
                m_stalled_bytes -= command->msg_len;
                m_ready_bytes += command->msg_len;
            }
            if (!queue.ready.empty())
                min_ready_clock = std::min(min_ready_clock,
                    effective_req_time(*queue.ready.front(), idletime,
                                       m_idle_time, pending != 0));
        }
        if (m_ready_bytes >= MESSAGE_PAYLOAD_MAX)
            return PR_NOW;
        if (m_ready_bytes == 0) {
            if (!std::isfinite(min_stalled_clock))
                return PR_NEVER;
            return idletime + std::max(0.0, min_stalled_clock - ack_clock);
        }
        if (min_ready_clock <= ack_clock + MIN_REQTIME_DELTA)
            return PR_NOW;
        double want_clock = std::min(min_ready_clock - MIN_REQTIME_DELTA,
                                     min_stalled_clock);
        return idletime + std::max(0.0, want_clock - ack_clock);
    }

    int build_block(int pending, double eventtime)
    {
        int length = MESSAGE_HEADER_SIZE;
        m_block_commands.clear();
        while (m_ready_bytes > 0) {
            CommandQueueState* best_queue = nullptr;
            double best_req = PR_NEVER;
            const double min_end = std::max(eventtime, m_idle_time)
                                 + calculate_bittime(m_config, pending + MESSAGE_MIN);
            for (int id : m_active_order) {
                CommandQueueState& queue = m_queues.at(id);
                if (queue.ready.empty())
                    continue;
                const double req = effective_req_time(
                    *queue.ready.front(), min_end, m_idle_time,
                    length > MESSAGE_HEADER_SIZE);
                if (req < best_req) {
                    best_req = req;
                    best_queue = &queue;
                }
            }
            if (best_queue == nullptr)
                break;
            HostDispatchCmd* command = best_queue->ready.front();
            if (length + command->msg_len > MESSAGE_MAX - MESSAGE_TRAILER_SIZE)
                break;
            best_queue->ready.pop_front();
            length += command->msg_len;
            m_ready_bytes -= command->msg_len;
            m_block_commands.push_back(command);
            deactivate_if_empty(*best_queue);
        }
        if (length == MESSAGE_HEADER_SIZE)
            return 0;
        length += MESSAGE_TRAILER_SIZE;
        const double block_start = std::max(eventtime, m_idle_time);
        const double block_end = block_start
                               + calculate_bittime(m_config, pending + length);
        const double receive_print_time = estimate(block_end);
        const double ack_eventtime = block_end + calculate_bittime(
            m_config, static_cast<uint32_t>(std::max(MESSAGE_MIN,
                                                     m_config.ack_message_bytes)));
        const uint64_t sequence = m_send_seq++;
        const int block_index = m_next_block_index++;
        for (HostDispatchCmd* command : m_block_commands) {
            command->send_time = block_start;
            command->block_end_time = block_end;
            command->ack_time = ack_eventtime;
            command->recv_time = receive_print_time;
            command->block_sequence = sequence;
            command->block_index = block_index;
            ++m_dispatched;
            if (m_receive_callback
                && m_receive_callback(*command)
                       == SimulationControl::StopRiskDetected) {
                m_control = SimulationControl::StopRiskDetected;
                break;
            }
        }

        m_block_commands.clear();
        release_consumed_prefix();
        m_need_ack_bytes += length;
        m_sent.push_back({sequence, length, ack_eventtime});
        return length;
    }

    double command_event(double eventtime)
    {
        int pending = 0;
        for (;;) {
            const double wake = check_send_command(pending, eventtime);
            if (wake != PR_NOW || pending + MESSAGE_MAX > MESSAGE_MAX * MAX_PENDING_BLOCKS) {
                if (pending != 0) {
                    m_idle_time = std::max(eventtime, m_idle_time)
                                + calculate_bittime(m_config, static_cast<uint32_t>(pending));
                    pending = 0;
                }
                if (wake != PR_NOW)
                    return wake;
            }
            const int length = build_block(pending, eventtime);
            if (m_control != SimulationControl::Continue)
                return PR_NEVER;
            if (length <= 0)
                return PR_NEVER;
            pending += length;
        }
    }

    std::deque<HostDispatchCmd> m_commands;
    HostDispatchConfig m_config;
    std::function<double(double)> m_estimated_print_time;
    HostReceiveControlCallback m_receive_callback;
    std::deque<HostDispatchCmd*> m_incoming;
    std::unordered_map<int, CommandQueueState> m_queues;
    std::vector<int> m_active_order;
    std::vector<HostDispatchCmd*> m_block_commands;
    std::deque<SentBlock> m_sent;
    double m_idle_time = 0.0;
    int m_ready_bytes = 0;
    int m_stalled_bytes = 0;
    int m_need_ack_bytes = 0;
    int m_last_ack_bytes = 0;
    uint64_t m_send_seq = 1;
    uint64_t m_receive_seq = 1;
    uint64_t m_last_ack_seq = 0;
    size_t m_dispatched = 0;
    int m_next_block_index = 0;
    SimulationControl m_control = SimulationControl::Continue;
    uint64_t m_total_appended = 0;
    double m_last_enqueue_time = 0.0;
    bool m_have_enqueue_time = false;
    double m_eventtime = 0.0;
    bool m_finished = false;
    bool m_have_advanced = false;
    bool m_retain_command_trace = true;
};

HostDispatchOutcome schedule_one(
    std::vector<HostDispatchCmd>& commands,
    double final_host_eventtime,
    const std::function<double(double)>& estimated_print_time,
    const HostDispatchConfig& config,
    const HostReceiveControlCallback& receive_callback)
{
    std::vector<size_t> order(commands.size());
    for (size_t i = 0; i < order.size(); ++i)
        order[i] = i;
    std::stable_sort(order.begin(), order.end(), [&](size_t lhs, size_t rhs) {
        return std::max(0.0, commands[lhs].enqueue_time)
             < std::max(0.0, commands[rhs].enqueue_time);
    });

    DeterministicSerialQueue session(config, estimated_print_time,
                                     receive_callback);
    for (size_t index : order)
        session.append(commands[index]);
    const HostDispatchOutcome outcome = session.finish(final_host_eventtime);
    std::vector<HostDispatchCmd> scheduled = session.take_commands();
    for (size_t i = 0; i < order.size(); ++i) {
        commands[order[i]] = std::move(scheduled[i]);
        commands[order[i]].enqueue_sequence = order[i];
    }
    return outcome;
}

} // namespace

struct DeterministicSerialQueueSession::Impl
{
    Impl(const HostDispatchConfig& config,
         const std::function<double(double)>& estimated_print_time,
         const HostReceiveControlCallback& receive_callback,
         bool retain_command_trace)
        : engine(config, estimated_print_time, receive_callback,
                 retain_command_trace) {}

    DeterministicSerialQueue engine;
};

DeterministicSerialQueueSession::DeterministicSerialQueueSession(
    const HostDispatchConfig& config,
    const std::function<double(double)>& estimated_print_time,
    const HostReceiveControlCallback& receive_callback,
    bool retain_command_trace)
    : m_impl(std::make_unique<Impl>(config, estimated_print_time,
                                    receive_callback,
                                    retain_command_trace))
{}

DeterministicSerialQueueSession::~DeterministicSerialQueueSession() = default;

void DeterministicSerialQueueSession::append(const HostDispatchCmd& command)
{
    m_impl->engine.append(command);
}

void DeterministicSerialQueueSession::append(HostDispatchCmd&& command)
{
    m_impl->engine.append(std::move(command));
}

SimulationControl DeterministicSerialQueueSession::advance_to(
    double host_eventtime)
{
    return m_impl->engine.advance_to(host_eventtime);
}

SimulationControl DeterministicSerialQueueSession::drain_ready()
{
    return m_impl->engine.drain_ready();
}

HostDispatchOutcome DeterministicSerialQueueSession::finish(
    double final_host_eventtime)
{
    return m_impl->engine.finish(final_host_eventtime);
}

std::vector<HostDispatchCmd>
DeterministicSerialQueueSession::take_commands()
{
    return m_impl->engine.take_commands();
}

size_t DeterministicSerialQueueSession::command_count() const
{
    return m_impl->engine.command_count();
}

size_t DeterministicSerialQueueSession::commands_consumed() const
{
    return m_impl->engine.commands_consumed();
}

size_t DeterministicSerialQueueSession::retained_command_count() const
{
    return m_impl->engine.retained_command_count();
}

LateBatchEnvelope analyze_late_batch_envelope(
    const std::vector<HostDispatchCmd>& commands,
    int host_move_slots,
    int physical_pool_slots,
    const HostDispatchConfig& config,
    double resync_lead_time,
    size_t source_batch_window)
{
    LateBatchEnvelope best;
    if (commands.empty() || host_move_slots <= 0 || physical_pool_slots <= 0)
        return best;

    struct BatchCandidate {
        size_t first_batch = 0;
        size_t last_batch = 0;
        size_t move_count = 0;
    };
    std::map<double, size_t> batch_counts;
    for (const HostDispatchCmd& command : commands) {
        // Define lookahead batches from queue_step commands only.  Periodic
        // PWM/digital commands have their own waketimes and must not consume
        // one of the buffer_time_high / LOOKAHEAD_FLUSH_TIME batch slots.
        if (command.uses_move_slot && command.source_kind == HCS_E_STEP
            && std::isfinite(command.batch_start_time))
            ++batch_counts[command.batch_start_time];
    }
    struct SourceBatch {
        double start = 0.0;
        size_t move_count = 0;
    };
    std::vector<SourceBatch> source_batches;
    source_batches.reserve(batch_counts.size());
    for (const auto& entry : batch_counts)
        source_batches.push_back({entry.first, entry.second});
    source_batch_window = std::max<size_t>(1, source_batch_window);
    std::vector<BatchCandidate> candidates;
    candidates.reserve(source_batches.size());
    size_t rolling_moves = 0;
    for (size_t i = 0; i < source_batches.size(); ++i) {
        rolling_moves += source_batches[i].move_count;
        if (i >= source_batch_window)
            rolling_moves -= source_batches[i - source_batch_window].move_count;
        const size_t first = i + 1 > source_batch_window
                           ? i + 1 - source_batch_window : 0;
        candidates.push_back({first, i, rolling_moves});
    }
    std::sort(candidates.begin(), candidates.end(),
              [](const BatchCandidate& lhs, const BatchCandidate& rhs) {
                  if (lhs.move_count != rhs.move_count)
                      return lhs.move_count > rhs.move_count;
                  return lhs.first_batch < rhs.first_batch;
              });

    // Stepcompress may retain commands over several flushes, so commands from
    // one lookahead batch are not necessarily contiguous in the output vector.
    // Examine batches in descending command count.  Once the remaining command
    // count cannot beat the observed peak, no later candidate can improve it.
    for (const BatchCandidate& candidate : candidates) {
        if (best.host_step_peak >= host_move_slots)
            break;
        if (best.overflow && best.peak >= physical_pool_slots)
            break;
        if (candidate.move_count <= static_cast<size_t>(best.host_step_peak))
            break;
        const double source_batch_start =
            source_batches[candidate.first_batch].start;
        const double source_batch_end =
            source_batches[candidate.last_batch].start;
        std::vector<HostDispatchCmd> replay;
        std::vector<size_t> replay_indices;
        replay_indices.reserve(candidate.move_count + 128);
        double selected_req_min = std::numeric_limits<double>::infinity();
        double selected_req_max = -std::numeric_limits<double>::infinity();
        for (size_t command_index = 0; command_index < commands.size();
             ++command_index) {
            const HostDispatchCmd& command = commands[command_index];
            if (command.source_kind != HCS_E_STEP
                || command.batch_start_time < source_batch_start
                || command.batch_start_time > source_batch_end)
                continue;
            replay_indices.push_back(command_index);
            selected_req_min = std::min(selected_req_min, command.req_time);
            selected_req_max = std::max(selected_req_max, command.req_time);
        }
        // PWM/digital commands use the same physical move pool, but they are
        // not governed by the stepper-sync move_clocks heap.  Include the
        // auxiliary traffic whose firmware waketime overlaps this candidate.
        for (size_t command_index = 0; command_index < commands.size();
             ++command_index) {
            const HostDispatchCmd& command = commands[command_index];
            if (command.source_kind == HCS_E_STEP)
                continue;
            if (command.req_time >= selected_req_min
                && command.req_time <= selected_req_max)
                replay_indices.push_back(command_index);
        }
        std::sort(replay_indices.begin(), replay_indices.end());
        replay.reserve(replay_indices.size());
        double first_req_time = std::numeric_limits<double>::infinity();
        for (size_t command_index : replay_indices) {
            const HostDispatchCmd& command = commands[command_index];
            replay.push_back(command);
            if (command.uses_move_slot)
                first_req_time = std::min(first_req_time, command.req_time);
        }
        const size_t move_count = candidate.move_count;
        if (!replay.empty() && std::isfinite(first_req_time)) {
            const double shift = std::max(0.0, resync_lead_time)
                               - first_req_time;
            std::priority_queue<double, std::vector<double>,
                                std::greater<double>> move_clocks;
            for (int i = 0; i < host_move_slots; ++i)
                move_clocks.push(0.0);

            int first_line = -1;
            int last_line = -1;
            for (HostDispatchCmd& command : replay) {
                command.enqueue_time = 0.0;
                command.req_time = std::max(0.0, command.req_time + shift);
                command.slot_free_time = std::max(
                    0.0, command.slot_free_time + shift);
                command.exec_start = std::max(0.0, command.exec_start + shift);
                command.exec_end = std::max(0.0, command.exec_end + shift);
                command.batch_start_time = std::max(0.0, resync_lead_time);
                const bool step_move_slot = command.uses_move_slot
                                         && command.source_kind == HCS_E_STEP;
                if (step_move_slot) {
                    command.min_time = move_clocks.top();
                    move_clocks.pop();
                    move_clocks.push(command.slot_free_time);
                    if (command.line_idx >= 0) {
                        if (first_line < 0)
                            first_line = command.line_idx;
                        last_line = command.line_idx;
                    }
                } else {
                    command.min_time = std::max(0.0, command.min_time + shift);
                }
                command.ready_time = command.min_time;
                command.recv_time = command.min_time;
                command.send_time = command.block_end_time = command.ack_time = 0.0;
                command.block_sequence = 0;
                command.block_index = -1;
            }

            McuMovePool pool(physical_pool_slots);
            McuMovePool step_pool(host_move_slots);
            std::vector<HostDispatchCmd> empty;
            (void) schedule_one(
                replay, 0.0, [](double eventtime) { return eventtime; },
                config, [&](const HostDispatchCmd& command) {
                    if (command.uses_move_slot) {
                        pool.alloc(command.recv_time,
                                   std::max(command.recv_time,
                                            command.slot_free_time),
                                   command.line_idx, command.source_kind);
                        if (command.source_kind == HCS_E_STEP)
                            step_pool.alloc(command.recv_time,
                                std::max(command.recv_time,
                                         command.slot_free_time),
                                command.line_idx, command.source_kind);
                    }
                    return SimulationControl::Continue;
                });
            const McuMovePool::Stats stats = pool.analyze();
            const McuMovePool::Stats step_stats = step_pool.analyze();
            if (step_stats.peak > best.host_step_peak
                || (step_stats.peak == best.host_step_peak
                    && (stats.peak > best.peak
                        || (stats.peak == best.peak && stats.overflow
                            && !best.overflow)))) {
                best.peak = stats.peak;
                best.host_step_peak = step_stats.peak;
                best.overflow = stats.overflow;
                best.peak_time = stats.peak_time;
                best.source_batch_start = source_batch_start;
                best.first_line = first_line;
                best.last_line = last_line;
                best.batch_commands = replay.size();
                best.move_commands = move_count;
            }
        }
    }
    return best;
}

DualHostDispatchOutcome schedule_host_dispatch_until(
    std::vector<HostDispatchCmd>& primary_cmds,
    std::vector<HostDispatchCmd>& secondary_cmds,
    const std::vector<ToolheadFlushWindow>& windows,
    double final_host_eventtime,
    double buffer_time_high,
    int primary_slots,
    int secondary_slots,
    const EstimatedPrintTimeCb& primary_estimated_print_time_cb,
    const EstimatedPrintTimeCb& secondary_estimated_print_time_cb,
    const HostDispatchConfig& primary_config,
    const HostDispatchConfig& secondary_config,
    const HostReceiveControlCallback& primary_receive_cb,
    const HostReceiveControlCallback& secondary_receive_cb)
{
    (void)windows;
    (void)buffer_time_high;
    (void)primary_slots;
    (void)secondary_slots;
    DualHostDispatchOutcome outcome;
    auto run_primary = [&] {
        outcome.primary = schedule_one(
            primary_cmds, final_host_eventtime,
            primary_estimated_print_time_cb, primary_config,
            primary_receive_cb);
    };
    auto run_secondary = [&] {
        outcome.secondary = schedule_one(
            secondary_cmds, final_host_eventtime,
            secondary_estimated_print_time_cb, secondary_config,
            secondary_receive_cb);
    };
#if !defined(KLSIM_DISABLE_TBB)
    tbb::parallel_invoke(run_primary, run_secondary);
#else
    run_primary();
    run_secondary();
#endif
    return outcome;
}

void schedule_host_dispatch(
    std::vector<HostDispatchCmd>& primary_cmds,
    std::vector<HostDispatchCmd>& secondary_cmds,
    const std::vector<ToolheadFlushWindow>& windows,
    double final_host_eventtime,
    double buffer_time_high,
    int primary_slots,
    int secondary_slots,
    const EstimatedPrintTimeCb& primary_estimated_print_time_cb,
    const EstimatedPrintTimeCb& secondary_estimated_print_time_cb,
    const HostDispatchConfig& primary_config,
    const HostDispatchConfig& secondary_config,
    const HostReceiveCallback& primary_receive_cb,
    const HostReceiveCallback& secondary_receive_cb)
{
    auto adapt = [](const HostReceiveCallback& callback) {
        return [callback](const HostDispatchCmd& command) {
            if (callback)
                callback(command);
            return SimulationControl::Continue;
        };
    };
    (void) schedule_host_dispatch_until(
        primary_cmds, secondary_cmds, windows, final_host_eventtime,
        buffer_time_high, primary_slots, secondary_slots,
        primary_estimated_print_time_cb, secondary_estimated_print_time_cb,
        primary_config, secondary_config, adapt(primary_receive_cb),
        adapt(secondary_receive_cb));
}

}} // namespace Slic3r::KlipperSim
