#include "KlipperDispatchAnalysis.hpp"

#include <algorithm>
#include <cstdlib>

namespace Slic3r {
namespace KlipperSim {

namespace {

static int encoded_int_len(uint32_t v)
{
    int32_t sv = (int32_t)v;
    if (sv < (3L << 5) && sv >= -(1L << 5))
        return 1;
    if (sv < (3L << 12) && sv >= -(1L << 12))
        return 2;
    if (sv < (3L << 19) && sv >= -(1L << 19))
        return 3;
    if (sv < (3L << 26) && sv >= -(1L << 26))
        return 4;
    return 5;
}

static int queue_step_msg_len(const StepMoveCmd& c)
{
    return 1 + 1
         + encoded_int_len(c.interval)
         + encoded_int_len((uint32_t)c.count)
         + encoded_int_len((uint32_t)(int32_t)c.add);
}
} // namespace

template <class Fn>
static void map_stepcompress_metadata(std::vector<StepMoveCmd>& cmds,
                                      const std::vector<TrapMove>& phases,
                                      const std::function<bool(const TrapMove&)>& phase_active,
                                      double mcu_freq,
                                      Fn&& fn)
{
    size_t ph = 0;
    while (ph < phases.size() && !phase_active(phases[ph]))
        ++ph;
    for (auto& c : cmds) {
        double start_t = (double)c.first_clock / mcu_freq;
        while (ph + 1 < phases.size()) {
            size_t next_ph = ph + 1;
            while (next_ph < phases.size() && !phase_active(phases[next_ph]))
                ++next_ph;
            if (next_ph >= phases.size()
                || phases[ph].print_time + phases[ph].move_t >= start_t)
                break;
            ph = next_ph;
        }
        const TrapMove* tm = ph < phases.size() ? &phases[ph] : nullptr;
        fn(c, start_t, tm);
    }
}

void append_stepcompress_dispatch_cmds(const KlipperStepCompress& sc,
                                       const std::vector<TrapMove>& phases,
                                       const std::function<bool(const TrapMove&)>& phase_active,
                                       double mcu_freq,
                                       int source_kind,
                                       std::vector<HostDispatchCmd>& out)
{
    (void)phases;
    (void)phase_active;
    const double serialqueue_advance_window = (double)(1ULL << 31) / mcu_freq;
    const std::vector<StepMoveCmd>& cmds = sc.commands();
    for (const StepMoveCmd& c : cmds) {
        const double start_t = (double)c.first_clock / mcu_freq;
        const double done_t = (double)c.free_clock / mcu_freq;
        const double move_slot_release_t = (double)c.min_clock / mcu_freq;
        double min_t = (double)c.req_clock / mcu_freq - serialqueue_advance_window;
        if (min_t < 0.0)
            min_t = 0.0;
        const double batch_start = c.batch_start_time > 0.0
            ? c.batch_start_time
            : start_t;
        out.push_back({
            batch_start,
            min_t,
            min_t,
            (double)c.req_clock / mcu_freq,
            move_slot_release_t,
            start_t,
            done_t,
            min_t,
            c.line_idx,
            source_kind,
            true,
            false,
            queue_step_msg_len(c)
        });
    }
    for (const StepAuxCmd& c : sc.aux_commands()) {
        double min_t = (double)c.req_clock / mcu_freq - serialqueue_advance_window;
        if (min_t < 0.0)
            min_t = 0.0;
        const double req_t = (double)c.req_clock / mcu_freq;
        out.push_back({
            c.batch_start_time,
            min_t,
            min_t,
            req_t,
            req_t,
            req_t,
            req_t,
            min_t,
            c.line_idx,
            source_kind,
            false,
            false,
            c.msg_len
        });
    }
}

void append_stepcompress_dispatch_cmds(const std::vector<StepMoveCmd>& cmds,
                                       const std::vector<StepAuxCmd>& aux_cmds,
                                       double mcu_freq,
                                       int source_kind,
                                       std::vector<HostDispatchCmd>& out)
{
    const double serialqueue_advance_window = (double)(1ULL << 31) / mcu_freq;
    for (const StepMoveCmd& c : cmds) {
        const double start_t = (double)c.first_clock / mcu_freq;
        const double done_t = (double)c.free_clock / mcu_freq;
        const double move_slot_release_t = (double)c.min_clock / mcu_freq;
        double min_t = (double)c.req_clock / mcu_freq - serialqueue_advance_window;
        if (min_t < 0.0)
            min_t = 0.0;
        const double batch_start = c.batch_start_time > 0.0
            ? c.batch_start_time : start_t;
        out.push_back({
            batch_start, min_t, min_t, (double)c.req_clock / mcu_freq,
            move_slot_release_t, start_t, done_t, min_t,
            c.line_idx, source_kind, true, false, queue_step_msg_len(c)
        });
    }
    for (const StepAuxCmd& c : aux_cmds) {
        double min_t = (double)c.req_clock / mcu_freq - serialqueue_advance_window;
        if (min_t < 0.0)
            min_t = 0.0;
        const double req_t = (double)c.req_clock / mcu_freq;
        out.push_back({
            c.batch_start_time, min_t, min_t, req_t, req_t, req_t, req_t,
            min_t, c.line_idx, source_kind, false, false, c.msg_len
        });
    }
}
void annotate_stepcompress_commands(KlipperStepCompress& sc,
                                    const std::vector<TrapMove>& phases,
                                    const std::function<bool(const TrapMove&)>& phase_active,
                                    double mcu_freq)
{
    map_stepcompress_metadata(sc.commands_mut(), phases, phase_active, mcu_freq,
                              [](StepMoveCmd& c, double start_t, const TrapMove* tm) {
        if (c.batch_start_time <= 0.0)
            c.batch_start_time = tm != nullptr ? tm->batch_start_time : start_t;
        if (c.line_idx < 0)
            c.line_idx = tm != nullptr ? tm->line_idx : -1;
    });
}

void build_dispatch_events(const std::vector<HostDispatchCmd>& cmds,
                           double time_offset,
                           std::vector<std::pair<double, int>>& dst)
{
    dst.clear();
    dst.reserve(cmds.size() * 2);
    for (const auto& c : cmds) {
        if (!c.uses_move_slot)
            continue;
        double recv = time_offset + c.recv_time;
        double done = time_offset + (c.slot_free_time < c.recv_time ? c.recv_time
                                                                    : c.slot_free_time);
        dst.emplace_back(recv, +1);
        dst.emplace_back(done, -1);
    }
    std::sort(dst.begin(), dst.end(),
              [](const std::pair<double, int>& a,
                 const std::pair<double, int>& b) {
                  if (a.first != b.first) return a.first < b.first;
                  return a.second < b.second;
              });
}

OverflowReport analyze_dispatch_cmds(const std::vector<HostDispatchCmd>& cmds,
                                     int total_slots)
{
    OverflowReport rep;
    rep.total = total_slots;
    if (cmds.empty())
        return rep;

    std::vector<std::pair<double, int>> events;
    build_dispatch_events(cmds, 0.0, events);

    double sum_dur = 0.0;
    int dur_count = 0;
    for (const auto& c : cmds) {
        if (!c.uses_move_slot)
            continue;
        double dur = c.slot_free_time - c.recv_time;
        sum_dur += dur;
        ++dur_count;
        if (dur > rep.max_exec_dur)
            rep.max_exec_dur = dur;
    }

    int cur = 0;
    for (const auto& e : events) {
        cur += e.second;
        if (cur > rep.peak) {
            rep.peak = cur;
            rep.peak_time = e.first;
        }
    }
    rep.overflow = rep.peak > total_slots;
    rep.avg_exec_dur = dur_count == 0 ? 0.0 : sum_dur / (double)dur_count;

    cur = 0;
    size_t ei = 0;
    for (double ts = 0.0; !events.empty() && ts <= events.back().first; ts += 100.0) {
        while (ei < events.size() && events[ei].first <= ts) {
            cur += events[ei].second;
            ++ei;
        }
        rep.samples.push_back({ts, cur});
    }
    return rep;
}

}} // namespace Slic3r::KlipperSim
