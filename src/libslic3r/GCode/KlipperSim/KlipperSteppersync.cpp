#include "KlipperSteppersync.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <stdexcept>
#if defined(__has_include)
#if __has_include(<boost/log/trivial.hpp>)
#include <boost/log/trivial.hpp>
#else
#include <iostream>
#define BOOST_LOG_TRIVIAL(level) std::cerr
#endif
#else
#include <boost/log/trivial.hpp>
#endif
#include <sstream>

namespace Slic3r { namespace KlipperSim {

// =========================================================================
// McuMovePool 瀹炵幇
// =========================================================================
McuMovePool::McuMovePool(int total_slots) : m_total(std::max(0, total_slots))
{
    m_free_times.reserve((size_t) m_total + 64);
    m_events.reserve((size_t) m_total * 4);
}

void McuMovePool::reset()
{
    m_free_times.clear();
    m_compact_free_times.clear();
    m_compact_ordered_inserts = 0;
    m_heap_ready              = false;
    m_events.clear();
    m_event_prefix.clear();
    m_event_index_ready      = false;
    m_events_finalized       = false;
    m_capture_events         = true;
    m_compact_occupancy      = 0;
    m_compact_high_threshold = 0;
    m_compact_high_start     = -1.0;
    m_compact_high_segments.clear();
    m_peak               = 0;
    m_peak_time          = 0.0;
    m_had_overflow       = false;
    m_failed_alloc_count = 0;
    m_sum_duration       = 0.0;
    m_max_duration       = 0.0;
    m_alloc_count        = 0;
    for (int& c : m_source_counts)
        c = 0;
}

void McuMovePool::record_timeline_event(double time, int delta)
{
    if (m_capture_events) {
        m_events.emplace_back(time, delta);
        m_event_index_ready = false;
    }
    if (m_compact_high_threshold <= 0)
        return;
    m_compact_occupancy += delta;
    if (m_compact_occupancy >= m_compact_high_threshold) {
        if (m_compact_high_start < 0.0)
            m_compact_high_start = time;
    } else if (m_compact_high_start >= 0.0) {
        m_compact_high_segments.push_back({m_compact_high_start, time});
        m_compact_high_start = -1.0;
    }
}

void McuMovePool::set_compact_timeline(int high_threshold)
{
    if (m_alloc_count != 0 || !m_free_times.empty() || !m_compact_free_times.empty())
        throw std::logic_error("McuMovePool compact timeline must be enabled before allocation");
    m_capture_events = false;
    m_events.clear();
    m_event_prefix.clear();
    m_event_index_ready      = false;
    m_events_finalized       = false;
    m_compact_occupancy      = 0;
    m_compact_high_threshold = std::max(0, high_threshold);
    m_compact_high_start     = -1.0;
    m_compact_high_segments.clear();
}

void McuMovePool::finish_compact_timeline()
{
    if (m_compact_high_threshold <= 0)
        return;
    advance_compact_timeline(std::numeric_limits<double>::infinity());
    if (m_compact_high_start >= 0.0) {
        m_compact_high_segments.push_back({m_compact_high_start, m_compact_high_start});
        m_compact_high_start = -1.0;
    }
}

void McuMovePool::advance_compact_timeline(double time)
{
    if (!m_capture_events) {
        while (!m_compact_free_times.empty() && m_compact_free_times.front() <= time) {
            const double released_at = m_compact_free_times.front();
            m_compact_free_times.pop_front();
            record_timeline_event(released_at, -1);
        }
        return;
    }
    if (!m_heap_ready) {
        std::make_heap(m_free_times.begin(), m_free_times.end(), std::greater<double>());
        m_heap_ready = true;
    }
    while (!m_free_times.empty() && m_free_times.front() <= time) {
        const double released_at = m_free_times.front();
        std::pop_heap(m_free_times.begin(), m_free_times.end(), std::greater<double>());
        m_free_times.pop_back();
        record_timeline_event(released_at, -1);
    }
}

bool McuMovePool::alloc(double at_time, double free_time, int line_idx, int source_kind)
{
    (void) line_idx;
    if (m_total <= 0)
        return false;
    // 鎳掗噴鏀撅細寮瑰嚭鎵€鏈?free_time <= at_time 鐨?slot
    advance_compact_timeline(at_time);

    // 妫€鏌ユ槸鍚?overflow锛坆asecmd.c:90锛?
    const size_t active_count = m_capture_events ? m_free_times.size() : m_compact_free_times.size();
    if ((int) active_count >= m_total) {
        m_had_overflow = true;
        ++m_failed_alloc_count;
        return false;
    }

    // 鍒嗛厤 slot
    double effective_free = std::max(at_time, free_time);
    if (m_capture_events) {
        m_free_times.push_back(effective_free);
        std::push_heap(m_free_times.begin(), m_free_times.end(), std::greater<double>());
    } else if (m_compact_free_times.empty() || m_compact_free_times.back() <= effective_free) {
        m_compact_free_times.push_back(effective_free);
    } else {
        const auto pos = std::upper_bound(m_compact_free_times.begin(), m_compact_free_times.end(), effective_free);
        m_compact_free_times.insert(pos, effective_free);
        ++m_compact_ordered_inserts;
    }

    // 璁板綍浜嬩欢
    record_timeline_event(at_time, +1);

    // 鏇存柊宄板€?
    int cur = (int) (m_capture_events ? m_free_times.size() : m_compact_free_times.size());
    if (cur > m_peak) {
        m_peak      = cur;
        m_peak_time = at_time;
    }

    double dur = effective_free - at_time;
    m_sum_duration += dur;
    if (dur > m_max_duration)
        m_max_duration = dur;
    ++m_alloc_count;

    // 鎸?source_kind 鍒嗙被璁℃暟
    int idx = 5;
    switch (source_kind) {
    case HCS_XY_STEP: idx = 0; break;
    case HCS_E_STEP: idx = 1; break;
    case HCS_AUX_FAN: idx = 2; break;
    case HCS_AUX_HEATER: idx = 3; break;
    case HCS_AUX_BED: idx = 4; break;
    default: idx = 5; break;
    }
    if (idx >= 0 && idx < 6)
        ++m_source_counts[idx];

    return true;
}

void McuMovePool::ensure_event_index() const
{
    if (m_event_index_ready)
        return;
    if (!m_events_finalized) {
        std::vector<double> pending_releases = m_free_times;
        std::make_heap(pending_releases.begin(), pending_releases.end(), std::greater<double>());
        while (!pending_releases.empty()) {
            const double released_at = pending_releases.front();
            std::pop_heap(pending_releases.begin(), pending_releases.end(), std::greater<double>());
            pending_releases.pop_back();
            m_events.emplace_back(released_at, -1);
        }
        m_events_finalized = true;
    }
    // recv_time processing keeps this ordered already. Preserve insertion
    // order for equal timestamps because it matches move_alloc/move_free
    // sequencing, including zero-duration allocations.
    if (!std::is_sorted(m_events.begin(), m_events.end(),
                        [](const std::pair<double, int>& a, const std::pair<double, int>& b) { return a.first < b.first; })) {
        std::stable_sort(m_events.begin(), m_events.end(),
                         [](const std::pair<double, int>& a, const std::pair<double, int>& b) { return a.first < b.first; });
    }
    m_event_prefix.resize(m_events.size());
    int occupancy = 0;
    for (size_t i = 0; i < m_events.size(); ++i) {
        occupancy += m_events[i].second;
        m_event_prefix[i] = occupancy;
    }
    m_event_index_ready = true;
}

int McuMovePool::occupancy_at(double print_time) const
{
    ensure_event_index();
    const auto it = std::upper_bound(m_events.begin(), m_events.end(), print_time,
                                     [](double time, const std::pair<double, int>& event) { return time < event.first; });
    if (it == m_events.begin())
        return 0;
    return m_event_prefix[(size_t) (it - m_events.begin() - 1)];
}

void McuMovePool::build_events(double time_offset, std::vector<std::pair<double, int>>& dst) const
{
    ensure_event_index();
    dst.clear();
    dst.reserve(m_events.size());
    for (const auto& e : m_events)
        dst.emplace_back(time_offset + e.first, e.second);
}

double McuMovePool::avg_duration() const { return m_alloc_count > 0 ? m_sum_duration / (double) m_alloc_count : 0.0; }

void McuMovePool::source_counts(int (&counts)[6]) const
{
    for (int i = 0; i < 6; ++i)
        counts[i] = m_source_counts[i];
}

McuMovePool::Stats McuMovePool::analyze(double warn_frac) const
{
    ensure_event_index();
    Stats s;
    s.total     = m_total;
    s.peak      = m_peak + m_baseline_count; // 鍚ǔ鎬佸熀绾?
    s.peak_time = m_peak_time;
    s.avg_dur   = avg_duration();
    s.max_dur   = m_max_duration;
    // 鍥轰欢 move_alloc() 鍦?free_list==NULL 鏃?shutdown锛坆asecmd.c:89锛夈€?
    // 鍗?occupied == move_count 鏃朵笅涓€娆?alloc 鍗虫孩鍑恒€?
    // 鐢?>= 鑰岄潪 >锛屼笌 alloc() 涓殑 heap.size() >= m_total 鍒ゅ畾涓€鑷淬€?
    s.overflow = m_had_overflow || (s.peak >= m_total);

    int warn_thresh = (warn_frac > 0.0) ? (int) (m_total * warn_frac) : (int) (m_total * 0.90);
    // 鎵弿楂樺崰鐢ㄦ
    int    cur       = 0;
    double seg_start = -1.0;
    for (const auto& e : m_events) {
        cur += e.second;
        if (cur >= warn_thresh && seg_start < 0.0)
            seg_start = e.first;
        else if (cur < warn_thresh && seg_start >= 0.0) {
            s.high_segments.push_back({seg_start, e.first});
            seg_start = -1.0;
        }
    }
    if (seg_start >= 0.0 && !m_events.empty())
        s.high_segments.push_back({seg_start, m_events.back().first});

    // 閲囨牱锛堟瘡 100s锛?
    if (!m_events.empty()) {
        cur       = 0;
        size_t ei = 0;
        for (double ts = 0.0; ts <= m_events.back().first; ts += 100.0) {
            while (ei < m_events.size() && m_events[ei].first <= ts) {
                cur += m_events[ei].second;
                ++ei;
            }
            s.samples.push_back({ts, cur});
        }
    }
    return s;
}

namespace {

static const char* source_name(int kind)
{
    switch (kind) {
    case HCS_XY_STEP: return "xy";
    case HCS_E_STEP: return "e";
    case HCS_AUX_FAN: return "fan";
    case HCS_AUX_HEATER: return "heater";
    case HCS_AUX_BED: return "bed";
    default: return "unk";
    }
}

static int source_index(int kind)
{
    switch (kind) {
    case HCS_XY_STEP: return 0;
    case HCS_E_STEP: return 1;
    case HCS_AUX_FAN: return 2;
    case HCS_AUX_HEATER: return 3;
    case HCS_AUX_BED: return 4;
    default: return 5;
    }
}

} // namespace

void KlipperSteppersync::register_stepqueue(KlipperStepCompress* sc)
{
    if (sc == nullptr)
        return;
    m_stepqueues.push_back(StepperState{sc, 0, 0});
}

namespace {

int encoded_step_int_len(uint32_t value)
{
    const int32_t signed_value = static_cast<int32_t>(value);
    if (signed_value < (3L << 5) && signed_value >= -(1L << 5)) return 1;
    if (signed_value < (3L << 12) && signed_value >= -(1L << 12)) return 2;
    if (signed_value < (3L << 19) && signed_value >= -(1L << 19)) return 3;
    if (signed_value < (3L << 26) && signed_value >= -(1L << 26)) return 4;
    return 5;
}

int queue_step_message_length(const StepMoveCmd& command)
{
    return 2 + encoded_step_int_len(command.interval)
             + encoded_step_int_len(command.count)
             + encoded_step_int_len(static_cast<uint32_t>(
                   static_cast<int32_t>(command.add)));
}

void heap_replace_clock(std::vector<uint64_t>& clocks, uint64_t replacement)
{
    size_t position = 0;
    for (;;) {
        const size_t child1 = position * 2 + 1;
        const size_t child2 = child1 + 1;
        const uint64_t clock1 = child1 < clocks.size()
            ? clocks[child1] : std::numeric_limits<uint64_t>::max();
        const uint64_t clock2 = child2 < clocks.size()
            ? clocks[child2] : std::numeric_limits<uint64_t>::max();
        if (replacement <= clock1 && replacement <= clock2) {
            clocks[position] = replacement;
            return;
        }
        if (clock1 < clock2) {
            clocks[position] = clock1;
            position = child1;
        } else {
            clocks[position] = clock2;
            position = child2;
        }
    }
}

} // namespace

void KlipperSteppersync::flush_stepqueues(
    uint64_t move_clock, int source_kind, std::vector<HostDispatchCmd>& out)
{
    for (StepperState& state : m_stepqueues)
        state.sc->flush_to(move_clock);

    if (m_step_move_clocks.size() != static_cast<size_t>(std::max(0, m_total)))
        m_step_move_clocks.assign(static_cast<size_t>(std::max(0, m_total)), 0);
    if (m_step_move_clocks.empty())
        throw std::runtime_error("KlipperSteppersync: no move clocks available");

    struct QueueHead {
        StepperState* state = nullptr;
        const StepMoveCmd* move = nullptr;
        const StepAuxCmd* aux = nullptr;
        uint64_t req_clock = 0;
        bool uses_move_slot = false;
    };

    for (;;) {
        QueueHead best;
        uint64_t best_req_clock = std::numeric_limits<uint64_t>::max();
        for (StepperState& state : m_stepqueues) {
            const auto& moves = state.sc->commands();
            const auto& aux = state.sc->aux_commands();
            const StepMoveCmd* move = state.next_move_index < moves.size()
                ? &moves[state.next_move_index] : nullptr;
            const StepAuxCmd* direction = state.next_aux_index < aux.size()
                ? &aux[state.next_aux_index] : nullptr;
            if (move != nullptr && direction != nullptr) {
                if (direction->output_sequence < move->output_sequence)
                    move = nullptr;
                else
                    direction = nullptr;
            }
            const uint64_t req_clock = move != nullptr
                ? move->req_clock : (direction != nullptr ? direction->req_clock
                                                          : std::numeric_limits<uint64_t>::max());
            if ((move != nullptr || direction != nullptr)
                && req_clock < best_req_clock) {
                best = QueueHead{&state, move, direction, req_clock, move != nullptr};
                best_req_clock = req_clock;
            }
        }
        if (best.state == nullptr
            || (best.uses_move_slot && best.req_clock > move_clock))
            break;

        const uint64_t next_available = m_step_move_clocks.front();
        HostDispatchCmd command;
        command.min_time = command.ready_time = command.recv_time =
            static_cast<double>(next_available) / m_mcu_freq + m_time_offset;
        command.req_time = static_cast<double>(best.req_clock) / m_mcu_freq
                         + m_time_offset;
        command.source_kind = source_kind;
        command.uses_move_slot = best.uses_move_slot;
        if (best.move != nullptr) {
            const StepMoveCmd& move = *best.move;
            command.batch_start_time = move.batch_start_time;
            command.slot_free_time = static_cast<double>(move.min_clock)
                                   / m_mcu_freq + m_time_offset;
            command.exec_start = static_cast<double>(move.first_clock)
                               / m_mcu_freq + m_time_offset;
            command.exec_end = static_cast<double>(move.free_clock)
                             / m_mcu_freq + m_time_offset;
            command.line_idx = move.line_idx;
            command.msg_len = queue_step_message_length(move);
            heap_replace_clock(m_step_move_clocks, move.min_clock);
            ++best.state->next_move_index;
        } else {
            const StepAuxCmd& aux = *best.aux;
            command.batch_start_time = aux.batch_start_time;
            command.slot_free_time = command.exec_start = command.exec_end
                = command.req_time;
            command.line_idx = aux.line_idx;
            command.msg_len = aux.msg_len;
            ++best.state->next_aux_index;
        }
        out.push_back(command);
    }
}

void KlipperSteppersync::set_time(double time_offset, double mcu_freq)
{
    m_time_offset = time_offset;
    m_mcu_freq    = mcu_freq;
    for (StepperState& ss : m_stepqueues)
        ss.sc->sync_time(time_offset, mcu_freq);
}

void KlipperSteppersync::add_cmd(double exec_start, double exec_end, double recv_time, int line_idx, int source_kind)
{
    if (exec_end < exec_start)
        exec_end = exec_start;
    if (std::isnan(recv_time))
        recv_time = exec_start - m_buffer;
    double occupied_until = exec_start;
    if (occupied_until < recv_time)
        occupied_until = recv_time;
    m_cmds.push_back(PoolCmd{recv_time, occupied_until, exec_start, exec_end, line_idx, source_kind});
}

void KlipperSteppersync::add_pending(const PendingSyncCmd& cmd)
{
    PendingSyncCmd in = cmd;
    if (in.exec_end < in.exec_start)
        in.exec_end = in.exec_start;
    if (in.min_time < 0.0)
        in.min_time = 0.0;
    if (in.recv_time < in.min_time)
        in.recv_time = in.min_time;
    if (in.slot_free_time < 0.0)
        in.slot_free_time = 0.0;
    m_pending.push_back(in);
    m_sorted = false;
}

bool KlipperSteppersync::consume_received(const HostDispatchCmd& command)
{
    if (!command.uses_move_slot)
        return true;
    const double recv = std::max(command.recv_time, command.min_time);
    const double occupied_until = std::max(recv, command.slot_free_time);
    const bool allocated = m_pool.alloc(recv, occupied_until,
                                        command.line_idx,
                                        command.source_kind);
    if (!allocated)
        m_overflow_events.push_back(
            {recv, command.line_idx, command.source_kind});
    m_cmds.push_back(PoolCmd{recv, occupied_until, command.exec_start,
                             std::max(command.exec_start, command.exec_end),
                             command.line_idx, command.source_kind});
    return allocated;
}

void KlipperSteppersync::flush_to(double flush_time)
{
    ++m_flush_seq;
    if (!m_sorted) {
        std::sort(m_pending.begin(), m_pending.end(), [](const PendingSyncCmd& a, const PendingSyncCmd& b) {
            if (a.recv_time != b.recv_time)
                return a.recv_time < b.recv_time;
            if (a.req_time != b.req_time)
                return a.req_time < b.req_time;
            if (a.min_time != b.min_time)
                return a.min_time < b.min_time;
            if (a.slot_free_time != b.slot_free_time)
                return a.slot_free_time < b.slot_free_time;
            return a.exec_end < b.exec_end;
        });
        m_sorted = true;
    }
    if (m_move_clocks.size() != (size_t) std::max(0, m_total)) {
        m_move_clocks.assign((size_t) std::max(0, m_total), 0.0);
        m_heap_ready = false;
    }
    if (!m_heap_ready) {
        std::make_heap(m_move_clocks.begin(), m_move_clocks.end(), std::greater<double>());
        m_heap_ready = true;
    }

    const uint64_t move_clock = (uint64_t) std::llround(std::max(0.0, (flush_time - m_time_offset) * m_mcu_freq));
    if (m_use_registered_stepqueues) {
        for (StepperState& ss : m_stepqueues)
            ss.sc->flush_to(move_clock);

        for (;;) {
            StepperState*      best_ss        = nullptr;
            const StepMoveCmd* best_cmd       = nullptr;
            uint64_t           best_req_clock = std::numeric_limits<uint64_t>::max();
            for (StepperState& ss : m_stepqueues) {
                const auto& cmds = ss.sc->commands();
                if (ss.next_move_index >= cmds.size())
                    continue;
                const StepMoveCmd& cmd = cmds[ss.next_move_index];
                if (cmd.req_clock < best_req_clock) {
                    best_req_clock = cmd.req_clock;
                    best_cmd       = &cmd;
                    best_ss        = &ss;
                }
            }
            if (best_cmd == nullptr || best_req_clock > move_clock)
                break;

            // Legacy fallback path: serialqueue is not modeled here.
            // Use flush_time as receive point and min_clock as pool release point.
            double recv = flush_time;
            if (recv < 0.0)
                recv = 0.0;
            double occupied_until = (double) best_cmd->min_clock / m_mcu_freq + m_time_offset;

            double exec_start = (double) best_cmd->first_clock / m_mcu_freq + m_time_offset;
            double exec_end   = (double) best_cmd->free_clock / m_mcu_freq + m_time_offset;
            if (occupied_until < exec_start)
                occupied_until = exec_start;
            // 涔熻蛋鏄惧紡姹犳ā鍨嬶紙淇濇寔涓€鑷存€э級
            if (!m_pool.alloc(recv, occupied_until, best_cmd->line_idx, 99)) {
                m_overflow_events.push_back({recv, best_cmd->line_idx, 99});
            }
            m_cmds.push_back(PoolCmd{recv, occupied_until, exec_start, exec_end, best_cmd->line_idx, 99});
            ++best_ss->next_move_index;
        }
    }

    size_t added_total     = 0;
    size_t added_move_slot = 0;
    int    src_counts[6]   = {0, 0, 0, 0, 0, 0};
    double min_req = 0.0, max_req = 0.0;
    double min_min = 0.0, max_min = 0.0;
    double min_recv = 0.0, max_recv = 0.0;
    double min_slot_free = 0.0, max_slot_free = 0.0;
    double min_exec = 0.0, max_exec = 0.0;
    double min_next_avail = 0.0, max_next_avail = 0.0;
    bool   have_added      = false;
    bool   have_next_avail = false;
    int    first_line      = -1;
    int    last_line       = -1;

    while (m_next_pending < m_pending.size()) {
        const PendingSyncCmd& c = m_pending[m_next_pending];
        // Firmware allocates a move node when the command is received, so the
        // pool timeline must advance in recv_time order (not execution order).
        if (c.recv_time > flush_time)
            break;
        double next_avail = c.min_time;
        if (c.uses_move_slot) {
            double recv = c.recv_time;
            // occupied_until = MCU 渚?move_free 鏃跺埢銆?
            // - Stepper: slot_free_time 鏉ヨ嚜 stepcompress min_clock
            //   (= 涓婃潯鍛戒护 last_clock 鈮?stepper_load_next 瑁呰浇鏃跺埢)
            // - AUX: slot_free_time = waketime + handler overhead
            //   (= pwm_event / digital_load_event 涓?move_queue_pop+move_free 鏃跺埢)
            double occupied_until = c.slot_free_time;
            if (occupied_until < recv)
                occupied_until = recv;

            // 鏄惧紡姹犲垎閰嶏紙瀵瑰簲 basecmd.c move_alloc锛?
            if (!m_pool.alloc(recv, occupied_until, c.line_idx, c.source_kind)) {
                m_overflow_events.push_back({recv, c.line_idx, c.source_kind});
            }
            // 淇濈暀 PoolCmd 鐢ㄤ簬璇婃柇鏃ュ織
            m_cmds.push_back(PoolCmd{recv, occupied_until, c.exec_start, c.exec_end, c.line_idx, c.source_kind});
        }
        ++added_total;
        if (c.uses_move_slot)
            ++added_move_slot;
        ++src_counts[source_index(c.source_kind)];
        if (!have_added) {
            min_req = max_req = c.req_time;
            min_min = max_min = c.min_time;
            min_recv = max_recv = c.recv_time;
            min_slot_free = max_slot_free = c.slot_free_time;
            min_exec                      = c.exec_start;
            max_exec                      = c.exec_end;
            first_line                    = c.line_idx;
            last_line                     = c.line_idx;
            have_added                    = true;
        } else {
            if (c.req_time < min_req)
                min_req = c.req_time;
            if (c.req_time > max_req)
                max_req = c.req_time;
            if (c.min_time < min_min)
                min_min = c.min_time;
            if (c.min_time > max_min)
                max_min = c.min_time;
            if (c.recv_time < min_recv)
                min_recv = c.recv_time;
            if (c.recv_time > max_recv)
                max_recv = c.recv_time;
            if (c.slot_free_time < min_slot_free)
                min_slot_free = c.slot_free_time;
            if (c.slot_free_time > max_slot_free)
                max_slot_free = c.slot_free_time;
            if (c.exec_start < min_exec)
                min_exec = c.exec_start;
            if (c.exec_end > max_exec)
                max_exec = c.exec_end;
            if (first_line < 0 && c.line_idx >= 0)
                first_line = c.line_idx;
            if (c.line_idx >= 0)
                last_line = c.line_idx;
        }
        if (c.uses_move_slot) {
            if (!have_next_avail) {
                min_next_avail = max_next_avail = next_avail;
                have_next_avail                 = true;
            } else {
                if (next_avail < min_next_avail)
                    min_next_avail = next_avail;
                if (next_avail > max_next_avail)
                    max_next_avail = next_avail;
            }
        }
        ++m_next_pending;
    }

#if 0  // 楂橀璇婃柇鏃ュ織锛岄粯璁ゅ叧闂?
    if (added_total > 0 && (src_counts[1] > 0 || (!m_debug_name.empty() && m_debug_name == "nozzle_mcu"))) {
        std::ostringstream oss;
        oss << "[KLSIM] steppersync_flush name="
            << (m_debug_name.empty() ? "unnamed" : m_debug_name)
            << " seq=" << m_flush_seq
            << " flush=" << flush_time
            << " added=" << added_total
            << " added_move_slot=" << added_move_slot
            << " pending_idx=" << m_next_pending
            << "/" << m_pending.size()
            << " occ_after=" << occupancy_at(flush_time)
            << " total=" << m_total
            << " src=["
            << source_name(HCS_XY_STEP) << ':' << src_counts[0] << ','
            << source_name(HCS_E_STEP) << ':' << src_counts[1] << ','
            << source_name(HCS_AUX_FAN) << ':' << src_counts[2] << ','
            << source_name(HCS_AUX_HEATER) << ':' << src_counts[3] << ','
            << source_name(HCS_AUX_BED) << ':' << src_counts[4] << ','
            << source_name(99) << ':' << src_counts[5] << ']';
        if (have_added) {
            oss << " min=[" << min_min << "," << max_min << "]"
                << " req=[" << min_req << "," << max_req << "]"
                << " recv=[" << min_recv << "," << max_recv << "]"
                << " slot_free=[" << min_slot_free << "," << max_slot_free << "]"
                << " exec=[" << min_exec << "," << max_exec << "]"
                << " first_line=" << first_line
                << " last_line=" << last_line;
        }
        if (have_next_avail) {
            oss << " next_avail=[" << min_next_avail << "," << max_next_avail << "]";
        }
        BOOST_LOG_TRIVIAL(error) << oss.str();
    }
#endif // 楂橀璇婃柇鏃ュ織锛岄粯璁ゅ叧闂?
}

int KlipperSteppersync::occupancy_at(double print_time) const { return m_pool.occupancy_at(print_time); }

void KlipperSteppersync::build_events(double time_offset, std::vector<std::pair<double, int>>& dst) const
{
    m_pool.build_events(time_offset, dst);
}

OverflowReport KlipperSteppersync::analyze() const
{
    // 浼樺厛浣跨敤鏄惧紡姹犳ā鍨嬬殑缁熻
    auto           ps = m_pool.analyze(m_warn_frac);
    OverflowReport rep;
    rep.total         = m_total;
    rep.peak          = ps.peak;
    rep.peak_time     = ps.peak_time;
    rep.overflow      = ps.overflow || !m_overflow_events.empty();
    rep.avg_exec_dur  = ps.avg_dur;
    rep.max_exec_dur  = ps.max_dur;
    rep.high_segments = std::move(ps.high_segments);
    rep.samples       = std::move(ps.samples);
    return rep;
}

}} // namespace Slic3r::KlipperSim
