#ifndef slic3r_GCode_KlipperSim_KlipperSteppersync_hpp_
#define slic3r_GCode_KlipperSim_KlipperSteppersync_hpp_

// ---------------------------------------------------------------------------
// KlipperSteppersync  (port of host steppersync + MCU basecmd shared move pool)
// ---------------------------------------------------------------------------
// Models ONE mcu's shared move-node pool (basecmd.c move_free_list, size=total).
// Every occupying command (queue_step / queue_pwm_out / queue_digital_out) is
// move_alloc'd when the MCU receives it and move_free'd when the command is
// consumed from the per-device move queue.
//
// Current authoritative simulation path:
//   HostDispatchCmd -> PendingSyncCmd -> KlipperSteppersync::flush_to()
// The older registered-stepqueue path is retained only as a fallback/legacy
// hook and is disabled by default.
//
// For stepper queue_step commands this is subtle: stepper.c frees the move node
// in stepper_load_next() when the command is popped from the stepper queue and
// loaded into the active stepper state, not when the final pulse executes.
// Therefore the host-side occupancy interval is [recv_time, occupied_until],
// while exec_start/exec_end still describe the actual physical motion interval.
//
// overflow 閳?at some instant the number of occupied nodes reaches `total`
// (move_alloc fails -> shutdown("Move queue overflow")).
// ---------------------------------------------------------------------------

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <deque>
#include <limits>
#include <vector>
#include <string>

#include "KlipperHostDispatch.hpp"
#include "KlipperStepCompress.hpp"

namespace Slic3r {
namespace KlipperSim {

// =========================================================================
// McuMovePool 鈥?鏄惧紡 MCU 鍏变韩 move-node 姹犳ā鍨?// =========================================================================
// 1:1 瀵瑰簲鍥轰欢 basecmd.c 鐨?move_free_list + move_alloc/move_free 璇箟銆?// 鎵€鏈夎澶囩被鍨嬶紙stepper/pwm/digital锛夊叡浜悓涓€涓叏灞€姹犮€?// overflow 鍒ゅ畾锛氫换鎰忔椂鍒诲凡鍒嗛厤 node 鏁拌揪鍒?total 鈫?move_alloc 澶辫触銆?// =========================================================================
class McuMovePool
{
public:
    explicit McuMovePool(int total_slots);
    void reset();
    bool alloc(double at_time, double free_time, int line_idx, int source_kind);

    // move_alloc: 鍦?at_time 鏃跺埢鍒嗛厤涓€涓?slot锛岃 slot 鍦?free_time 鏃跺埢閲婃斁銆?    // 杩斿洖 false 琛ㄧず overflow锛? basecmd.c:90 shutdown锛夈€?    bool alloc(double at_time, double free_time, int line_idx, int source_kind);
    // Compact streaming mode: update occupancy/high-water state online without
    // retaining the full +1/-1 event timeline.
    void set_compact_timeline(int high_threshold);
    void advance_compact_timeline(double time);
    void finish_compact_timeline();
    const std::vector<std::pair<double,double>>& compact_high_segments() const
    {
        return m_compact_high_segments;
    }
    double compact_open_high_start() const { return m_compact_high_start; }
    int occupancy_at(double print_time) const;
    void build_events(double time_offset,
                      std::vector<std::pair<double,int>>& dst) const;
    // 鏌ヨ鎸囧畾鏃跺埢鐨勫崰鐢ㄦ暟銆?    int  occupancy_at(double print_time) const;

    // 浜х敓 +1/-1 浜嬩欢娴侊紝渚涜法灞傚崰鐢ㄦ洸绾垮悎骞躲€?    void build_events(double time_offset,
    int  total()     const { return m_total; }
    // Current modeled allocation count after releases at the latest alloc time.
    // The diagnostic baseline is intentionally excluded.
    int  current_modeled_occupancy() const
    {
        return static_cast<int>(m_capture_events ? m_free_times.size()
                                                 : m_compact_free_times.size());
    }
    int  peak()      const { return m_peak + m_baseline_count; }
    double peak_time() const { return m_peak_time; }
    bool  had_overflow() const { return m_had_overflow; }
    int   failed_alloc_count() const { return m_failed_alloc_count; }
    uint64_t compact_ordered_inserts() const { return m_compact_ordered_inserts; }
    double avg_duration() const;
    double max_duration() const { return m_max_duration; }
    int   alloc_count() const { return m_alloc_count; }
    void set_baseline_frac(double frac) { m_baseline_frac = std::max(0.0, std::min(1.0, frac)); m_baseline_count = (int)std::llround(m_total * m_baseline_frac); }

    // 璁剧疆绋虫€佸熀绾垮崰鐢紙fraction 0-1锛夛紝鐢ㄤ簬琛ュ伩鏈缓妯＄殑 PWM/digital 鍛戒护銆?    void set_baseline_frac(double frac) { m_baseline_frac = std::max(0.0, std::min(1.0, frac)); m_baseline_count = (int)std::llround(m_total * m_baseline_frac); }
    double baseline_frac() const { return m_baseline_frac; }
    int    baseline_count() const { return m_baseline_count; }
    void source_counts(int (&counts)[6]) const;

    // 鎸?source_kind 鍒嗙被鐨勫垎閰嶈鏁般€?    void source_counts(int (&counts)[6]) const;

    // 璁＄畻宄板€煎崰鐢ㄧ瓑缁熻銆傜粨鏋滀笌 KlipperSteppersync::analyze() 鍏煎銆?    struct Stats {
    struct Stats {
        int    peak = 0;
        double peak_time = 0.0;
        int    total = 0;
        double avg_dur = 0.0;
        double max_dur = 0.0;
        bool   overflow = false;
        std::vector<std::pair<double,double>> high_segments;
        std::vector<std::pair<double,int>>    samples;
    };
    Stats analyze(double warn_frac = 0.90) const;

private:
    int  m_total;
    int  m_peak = 0;
    double m_peak_time = 0.0;
    bool m_had_overflow = false;
    int  m_failed_alloc_count = 0;
    std::vector<double> m_free_times;
    // min-heap: 褰撳墠宸插垎閰?slot 鐨?free_time锛坒ront = 鏈€鏃╅噴鏀剧殑 slot锛?    std::vector<double> m_free_times;
    std::deque<double> m_compact_free_times;
    uint64_t m_compact_ordered_inserts = 0;
    bool m_heap_ready = false;
    // 浜嬩欢鏃ュ織: (time, delta) 鐢ㄤ簬鍗犵敤鏇茬嚎鎵弿
    mutable std::vector<std::pair<double,int>> m_events;
    double m_sum_duration = 0.0;
    double m_max_duration = 0.0;
    int    m_alloc_count = 0;
    int    m_source_counts[6] = {0,0,0,0,0,0};
    double m_baseline_frac = 0.0;
    int    m_baseline_count = 0;
    mutable bool m_event_index_ready = false;
    mutable bool m_events_finalized = false;
    mutable std::vector<int> m_event_prefix;
    bool   m_capture_events = true;
    int    m_compact_occupancy = 0;
    int    m_compact_high_threshold = 0;
    double m_compact_high_start = -1.0;
    std::vector<std::pair<double,double>> m_compact_high_segments;

    void ensure_event_index() const;
    void record_timeline_event(double time, int delta);
};

// ---------------------------------------------------------------------------
// PoolCmd / PendingSyncCmd 鈥?淇濈暀鐢ㄤ簬璇婃柇鏃ュ織鍜屼腑闂翠紶杈?// ---------------------------------------------------------------------------
// One occupying command:
//   recv_time       = when MCU receives / allocates the move node
//   occupied_until  = when MCU frees that node back to move_free_list
//   exec_start/end  = actual physical execution interval
struct PoolCmd
{
    double recv_time;
    double occupied_until;
    double exec_start;
    double exec_end;
    int    line_idx = -1;
    int    source_kind = 99;
};

struct PendingSyncCmd
{
    double min_time = 0.0;
    double req_time = 0.0;
    double recv_time = 0.0;
    double slot_free_time = 0.0;
    double exec_start = 0.0;
    double exec_end = 0.0;
    int    line_idx = -1;
    int    source_kind = 99;
    bool   uses_move_slot = true;
};

struct OverflowReport
{
    bool   overflow = false;
    int    peak = 0;
    double peak_time = 0.0;   // absolute print time (s) of peak occupancy
    int    total = 0;
    double avg_exec_dur = 0.0;
    double max_exec_dur = 0.0;
    std::vector<std::pair<double,double>> high_segments; // [start,end] >=90%
    std::vector<std::pair<double,int>> samples; // (print_time, occupancy) every 100s
};

class KlipperSteppersync
{
public:
    KlipperSteppersync(int total, double buffer_time_s, int pool_total = -1)
        : m_total(total), m_buffer(buffer_time_s),
          m_pool(pool_total >= 0 ? pool_total : total),
          m_move_clocks((size_t)std::max(0, total), 0.0) {}

    // Fraction of pool defining a "high occupancy" segment (default 0.90).
    void set_warn_fraction(double f) { m_warn_frac = f; }

    void clear()
    {
        m_cmds.clear();
        m_pending.clear();
        m_pool.reset();
        m_overflow_events.clear();
        for (StepperState& ss : m_stepqueues) {
            ss.next_move_index = 0;
            ss.next_aux_index  = 0;
        }
        m_move_clocks.assign((size_t)std::max(0, m_total), 0.0);
        m_step_move_clocks.assign((size_t)std::max(0, m_total), 0);
        m_sorted = true;
        m_heap_ready = false;
        m_next_pending = 0;
    }
    int total_slots() const { return m_total; }
    void register_stepqueue(KlipperStepCompress* sc);
    void set_time(double time_offset, double mcu_freq);
    void add_cmd(double exec_start, double exec_end,
                 double recv_time = std::numeric_limits<double>::quiet_NaN(),
                 int line_idx = -1,
                 int source_kind = 99);
    void add_pending(const PendingSyncCmd& cmd);
    // MCU receive-side command handler. Commands arrive here directly from
    // serialqueue in receive order and allocate the shared move pool online.
    bool consume_received(const HostDispatchCmd& cmd);
    void reserve_pending(size_t count)
    {
        m_pending.reserve(count);
        m_cmds.reserve(count);
    }
    void set_use_registered_stepqueues(bool enabled) { m_use_registered_stepqueues = enabled; }
    void flush_stepqueues(uint64_t move_clock, int source_kind,
                          std::vector<HostDispatchCmd>& out);
    void flush_to(double flush_time);
    size_t command_count() const { return m_cmds.size(); }
    size_t pending_count() const { return m_pending.size(); }
    const std::vector<PoolCmd>& commands() const { return m_cmds; }
    int occupancy_at(double print_time) const;
    void build_events(double time_offset,
                      std::vector<std::pair<double,int>>& dst) const;
    void set_debug_name(std::string name) { m_debug_name = std::move(name); }
    McuMovePool& pool() { return m_pool; }

    // 鏄惧紡 MCU 姹犳ā鍨嬶紙浠ｆ浛 flush_to 閲岄殣寮忓崰鐢ㄨ拷韪級銆?    McuMovePool& pool() { return m_pool; }
    const McuMovePool& pool() const { return m_pool; }

    // 璁板綍 overflow 鍙戠敓鐨勬椂鍒诲拰鏉ユ簮銆?    struct OverflowEvent {
    struct OverflowEvent {
        double time;
        int    line_idx;
        int    source_kind;
    };
    const std::vector<OverflowEvent>& overflow_events() const { return m_overflow_events; }

    // Compute peak pool occupancy over [recv_time, occupied_until].
    OverflowReport analyze() const;

private:
    struct StepperState
    {
        KlipperStepCompress* sc = nullptr;
        size_t next_move_index = 0;
        size_t next_aux_index = 0;
    };

    int    m_total;
    double m_buffer;
    double m_warn_frac = 0.90;
    double m_time_offset = 0.0;
    double m_mcu_freq = 1.0;
    std::vector<PoolCmd> m_cmds;
    std::vector<PendingSyncCmd> m_pending;
    std::vector<StepperState> m_stepqueues;
    std::vector<double> m_move_clocks;
    // Exact integer heap used by the online firmware-order path.
    std::vector<uint64_t> m_step_move_clocks;
    bool   m_use_registered_stepqueues = false;
    bool   m_sorted = true;
    bool   m_heap_ready = false;
    size_t m_next_pending = 0;
    std::string m_debug_name;
    size_t m_flush_seq = 0;
    McuMovePool m_pool;
    std::vector<OverflowEvent> m_overflow_events;
};

}} // namespace Slic3r::KlipperSim

#endif
