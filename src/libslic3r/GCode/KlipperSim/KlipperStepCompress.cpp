#include "KlipperStepCompress.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace Slic3r { namespace KlipperSim {

static inline int32_t div_round_up(int32_t n, int32_t d) { return (n + d - 1) / d; }

static inline int32_t idiv_up(int32_t n, int32_t d) { return (n >= 0) ? div_round_up(n, d) : (n / d); }

static inline int32_t idiv_down(int32_t n, int32_t d) { return (n >= 0) ? (n / d) : (n - d + 1) / d; }

static constexpr int      CHECK_LINES     = 0; // 璋冭瘯楠岃瘉锛岀敓浜х幆澧冨叧闂?static constexpr size_t QUEUE_START_SIZE = 1024;
static constexpr size_t   QUEUE_START_SIZE = 1024;
static constexpr int32_t  QUADRATIC_DEV   = 11;
static constexpr uint64_t CLOCK_DIFF_MAX  = (uint64_t) 3 << 28;
static constexpr double   SDS_FILTER_TIME = 0.000750;

KlipperStepCompress::KlipperStepCompress(uint32_t max_error_ticks) : m_max_error(max_error_ticks)
{
    m_queue.reserve(QUEUE_START_SIZE);
    m_queue_line_idx.reserve(QUEUE_START_SIZE);
    m_queue_batch_start.reserve(QUEUE_START_SIZE);
}

void KlipperStepCompress::reset()
{
    m_queue.clear();
    m_queue_line_idx.clear();
    m_queue_batch_start.clear();
    m_queue_pos            = 0;
    m_mcu_time_offset      = 0.0;
    m_mcu_freq             = 0.0;
    m_last_step_print_time = 0.0;
    m_last_step_clock      = 0;
    m_sdir                 = -1;
    m_invert_sdir          = 0;
    m_next_step_clock      = 0;
    m_next_step_dir        = 0;
    m_next_line_idx        = -1;
    m_next_batch_start     = 0.0;
    m_last_position        = 0;
    m_history.clear();
    m_commands.clear();
    m_aux_cmds.clear();
    m_have_emitted_move = false;
    m_next_output_sequence = 0;
}

void KlipperStepCompress::append_step(uint64_t step_clock)
{
    m_queue.push_back(step_clock);
    m_queue_line_idx.push_back(-1);
    m_queue_batch_start.push_back(0.0);
}

void KlipperStepCompress::free_history(uint64_t end_clock)
{
    while (!m_history.empty()) {
        const HistoryStep& hs = m_history.back();
        if (hs.last_clock > end_clock)
            break;
        m_history.pop_back();
    }
}

void KlipperStepCompress::calc_last_step_print_time()
{
    if (m_mcu_freq <= 0.0) {
        m_last_step_print_time = 0.0;
        return;
    }
    double lsc             = (double) m_last_step_clock;
    m_last_step_print_time = m_mcu_time_offset + (lsc - 0.5) / m_mcu_freq;
    // free_history 浠?find_past_position 浣跨敤锛屾ā鎷熼摼璺湭璋冪敤 鈫?涓嶅啀缁存姢
    // if (lsc > m_mcu_freq * HISTORY_EXPIRE)
    //    free_history((uint64_t)(lsc - m_mcu_freq * HISTORY_EXPIRE));
}

void KlipperStepCompress::set_time(double time_offset, double mcu_freq)
{
    m_mcu_time_offset = time_offset;
    m_mcu_freq        = mcu_freq;
    calc_last_step_print_time();
}

KlipperStepCompress::Points KlipperStepCompress::minmax_point(size_t pos) const
{
    uint32_t lsc       = (uint32_t) m_last_step_clock;
    uint32_t point     = (uint32_t) (m_queue[pos] - lsc);
    uint32_t prevpoint = pos > m_queue_pos ? (uint32_t) (m_queue[pos - 1] - lsc) : 0u;
    uint32_t max_error = (point - prevpoint) / 2;
    if (max_error > m_max_error)
        max_error = m_max_error;
    return Points{(int32_t) (point - max_error), (int32_t) point};
}

StepMoveCmd KlipperStepCompress::compress_bisect_add() const
{
    size_t qlast = m_queue.size();
    if (qlast > m_queue_pos + 65535)
        qlast = m_queue_pos + 65535;
    Points  point             = minmax_point(m_queue_pos);
    int32_t outer_mininterval = point.minp;
    int32_t outer_maxinterval = point.maxp;
    int32_t add = 0, minadd = -0x8000, maxadd = 0x7fff;
    int32_t bestinterval = 0, bestcount = 1, bestadd = 1;
    int32_t bestreach    = std::numeric_limits<int32_t>::min();
    int32_t zerointerval = 0, zerocount = 0;

    constexpr int MAX_COMPRESS_ITER = 10000;
    int           iter_guard        = 0;
    for (;;) {
        if (++iter_guard > MAX_COMPRESS_ITER) {
            // 鍘嬬缉涓嶆敹鏁?鈥?杩斿洖褰撳墠鏈€浣充及璁?            if (zerocount + zerocount / 16 >= bestcount)
            return StepMoveCmd{(uint32_t) zerointerval, (uint16_t) zerocount, 0};
            return StepMoveCmd{(uint32_t) bestinterval, (uint16_t) bestcount, (int16_t) bestadd};
        }
        Points  nextpoint{};
        int32_t nextmininterval = outer_mininterval;
        int32_t nextmaxinterval = outer_maxinterval;
        int32_t interval        = nextmaxinterval;
        int32_t nextcount       = 1;
        for (;;) {
            nextcount++;
            if (m_queue_pos + (size_t) (nextcount - 1) >= qlast) {
                int32_t count = nextcount - 1;
                return StepMoveCmd{(uint32_t) interval, (uint16_t) count, (int16_t) add};
            }
            nextpoint             = minmax_point(m_queue_pos + nextcount - 1);
            int32_t nextaddfactor = nextcount * (nextcount - 1) / 2;
            int32_t c             = add * nextaddfactor;
            if (nextmininterval * nextcount < nextpoint.minp - c)
                nextmininterval = idiv_up(nextpoint.minp - c, nextcount);
            if (nextmaxinterval * nextcount > nextpoint.maxp - c)
                nextmaxinterval = idiv_down(nextpoint.maxp - c, nextcount);
            if (nextmininterval > nextmaxinterval)
                break;
            interval = nextmaxinterval;
        }

        int32_t count     = nextcount - 1;
        int32_t addfactor = count * (count - 1) / 2;
        int32_t reach     = add * addfactor + interval * count;
        if (reach > bestreach || (reach == bestreach && interval > bestinterval)) {
            bestinterval = interval;
            bestcount    = count;
            bestadd      = add;
            bestreach    = reach;
            if (!add) {
                zerointerval = interval;
                zerocount    = count;
            }
            if (count > 0x200)
                break;
        }

        int32_t nextaddfactor = nextcount * (nextcount - 1) / 2;
        int32_t nextreach     = add * nextaddfactor + interval * nextcount;
        if (nextreach < nextpoint.minp) {
            minadd            = add + 1;
            outer_maxinterval = nextmaxinterval;
        } else {
            maxadd            = add - 1;
            outer_mininterval = nextmininterval;
        }

        if (count > 1) {
            int32_t errdelta = (int32_t) ((int64_t) m_max_error * QUADRATIC_DEV / ((int64_t) count * count));
            if (minadd < add - errdelta)
                minadd = add - errdelta;
            if (maxadd > add + errdelta)
                maxadd = add + errdelta;
        }

        int32_t c = outer_maxinterval * nextcount;
        if (minadd * nextaddfactor < nextpoint.minp - c)
            minadd = idiv_up(nextpoint.minp - c, nextaddfactor);
        c = outer_mininterval * nextcount;
        if (maxadd * nextaddfactor > nextpoint.maxp - c)
            maxadd = idiv_down(nextpoint.maxp - c, nextaddfactor);

        if (minadd > maxadd)
            break;
        add = maxadd - (maxadd - minadd) / 4;
    }

    if (zerocount + zerocount / 16 >= bestcount)
        return StepMoveCmd{(uint32_t) zerointerval, (uint16_t) zerocount, 0};
    return StepMoveCmd{(uint32_t) bestinterval, (uint16_t) bestcount, (int16_t) bestadd};
}

void KlipperStepCompress::emit_move(const StepMoveCmd& mv)
{
    int32_t  addfactor   = mv.count * (mv.count - 1) / 2;
    uint32_t ticks       = (uint32_t) ((int32_t) mv.add * addfactor + (int32_t) mv.interval * (mv.count - 1));
    uint64_t first_clock = m_last_step_clock + mv.interval;
    uint64_t last_clock  = first_clock + ticks;

    StepMoveCmd out = mv;
    if (out.line_idx < 0 && m_queue_pos < m_queue_line_idx.size()) {
        out.line_idx         = m_queue_line_idx[m_queue_pos];
        out.batch_start_time = m_queue_batch_start[m_queue_pos];
    }
    out.first_clock = first_clock;
    out.last_clock  = last_clock;
    // stepcompress.c:add_move() stores the previous last_step_clock in both
    // fields, including zero for the first command.  steppersync later
    // overloads min_clock with the earliest transmit clock.
    out.min_clock = m_last_step_clock;
    out.req_clock = m_last_step_clock;
    // min_clock = 鍏变韩姹?node 閲婃斁鏃跺埢銆?    // 瀵瑰簲鍥轰欢 stepper.c: stepper_load_next() 鍦ㄨ杞芥湰鏉″懡浠ゆ椂 move_free
    // 璇?node锛?    // 瑁呰浇鏃跺埢 鈮?涓婁竴鏉″懡浠ょ殑 last_clock 鈮?first_clock锛堝樊鍊?= interval锛屽井绉掔骇锛夈€?    //
    // 棣栧懡浠ゆ棤"涓婁竴鏉?锛屾晠浣跨敤鑷韩鐨?first_clock 浣滀负閲婃斁鏃跺埢銆?    out.min_clock = !m_have_emitted_move ? first_clock :
    // m_last_step_clock; req_clock = 鏈熸湜鎵ц鏃跺埢銆傞鍛戒护鍚屾牱浣跨敤鑷韩鐨?first_clock銆?    out.req_clock =
    // !m_have_emitted_move ? first_clock : m_last_step_clock;
    out.free_clock      = last_clock;
    out.output_sequence = m_next_output_sequence++;
    if (mv.count == 1 && first_clock >= m_last_step_clock + CLOCK_DIFF_MAX)
        out.req_clock = first_clock;
    m_commands.push_back(out);
    m_have_emitted_move = true;
    m_last_step_clock   = last_clock;
    m_last_position += m_sdir ? (int) mv.count : -(int) mv.count;
    // m_history 浠?find_past_position 浣跨敤锛屾ā鎷熼摼璺湭璋冪敤 鈫?涓嶅啀缁存姢
    // HistoryStep hs{first_clock, last_clock, m_last_position, mv.interval, mv.add, ...};
    // m_history.insert(m_history.begin(), hs);
}

void KlipperStepCompress::queue_flush(uint64_t move_clock)
{
    if (m_queue_pos >= m_queue.size())
        return;
    while (m_last_step_clock < move_clock) {
        StepMoveCmd move = compress_bisect_add();
        if (!move.count)
            break;
        if (CHECK_LINES) {
            uint32_t interval = move.interval;
            uint32_t p        = 0;
            for (uint16_t i = 0; i < move.count; ++i) {
                Points point = minmax_point(m_queue_pos + i);
                p += interval;
                if (p < (uint32_t) point.minp || p > (uint32_t) point.maxp)
                    break;
                interval += move.add;
            }
        }
        emit_move(move);
        if (m_queue_pos + move.count >= m_queue.size()) {
            m_queue.clear();
            m_queue_line_idx.clear();
            m_queue_batch_start.clear();
            m_queue_pos = 0;
            break;
        }
        m_queue_pos += move.count;
    }
    if (m_queue_pos > 0 && m_queue_pos == m_queue.size()) {
        m_queue.clear();
        m_queue_line_idx.clear();
        m_queue_batch_start.clear();
        m_queue_pos = 0;
    } else if (m_queue_pos > 0 && m_queue_pos >= QUEUE_START_SIZE) {
        m_queue.erase(m_queue.begin(), m_queue.begin() + (ptrdiff_t) m_queue_pos);
        m_queue_line_idx.erase(m_queue_line_idx.begin(), m_queue_line_idx.begin() + (ptrdiff_t) m_queue_pos);
        m_queue_batch_start.erase(m_queue_batch_start.begin(), m_queue_batch_start.begin() + (ptrdiff_t) m_queue_pos);
        m_queue_pos = 0;
    }
    calc_last_step_print_time();
}

void KlipperStepCompress::flush_far(uint64_t abs_step_clock)
{
    StepMoveCmd move{(uint32_t) (abs_step_clock - m_last_step_clock), 1, 0};
    emit_move(move);
    calc_last_step_print_time();
}

void KlipperStepCompress::set_next_step_dir(int sdir)
{
    if (m_sdir == sdir)
        return;
    queue_flush(UINT64_MAX);
    m_sdir = sdir;
    StepAuxCmd cmd;
    cmd.req_clock        = m_last_step_clock;
    // The direction command is emitted for m_next_step_clock and therefore
    // belongs to that step's lookahead batch.  last_step_print_time is a
    // request clock, not batch ownership.
    cmd.batch_start_time = m_next_batch_start;
    cmd.line_idx         = m_next_line_idx;
    cmd.msg_len          = 3;
    cmd.output_sequence  = m_next_output_sequence++;
    m_aux_cmds.push_back(cmd);
}

void KlipperStepCompress::queue_append_far()
{
    uint64_t     step_clock  = m_next_step_clock;
    const int    line_idx    = m_next_line_idx;
    const double batch_start = m_next_batch_start;
    m_next_step_clock        = 0;
    queue_flush(step_clock - CLOCK_DIFF_MAX + 1);
    if (step_clock >= m_last_step_clock + CLOCK_DIFF_MAX) {
        StepMoveCmd far_move{(uint32_t) (step_clock - m_last_step_clock), 1, 0};
        far_move.line_idx         = line_idx;
        far_move.batch_start_time = batch_start;
        emit_move(far_move);
        calc_last_step_print_time();
        return;
    }
    m_queue.push_back(step_clock);
    m_queue_line_idx.push_back(line_idx);
    m_queue_batch_start.push_back(batch_start);
}

void KlipperStepCompress::queue_append_extend()
{
    if (m_queue.size() - m_queue_pos > 65535 + 2000) {
        uint32_t flush = (uint32_t) (m_queue[m_queue.size() - 65535] - m_last_step_clock);
        queue_flush(m_last_step_clock + flush);
    }
    if (m_queue.capacity() == 0) {
        m_queue.reserve(QUEUE_START_SIZE);
        m_queue_line_idx.reserve(QUEUE_START_SIZE);
        m_queue_batch_start.reserve(QUEUE_START_SIZE);
    }
    m_queue.push_back(m_next_step_clock);
    m_queue_line_idx.push_back(m_next_line_idx);
    m_queue_batch_start.push_back(m_next_batch_start);
    m_next_step_clock = 0;
}

void KlipperStepCompress::queue_append()
{
    if (m_next_step_dir != m_sdir)
        set_next_step_dir(m_next_step_dir);
    if (m_next_step_clock >= m_last_step_clock + CLOCK_DIFF_MAX) {
        queue_append_far();
        return;
    }
    if (m_queue.size() == m_queue.capacity()) {
        queue_append_extend();
        return;
    }
    m_queue.push_back(m_next_step_clock);
    m_queue_line_idx.push_back(m_next_line_idx);
    m_queue_batch_start.push_back(m_next_batch_start);
    m_next_step_clock = 0;
}

void KlipperStepCompress::append_step_time(
    int sdir, double print_time, double step_time, double mcu_freq, double time_offset, int line_idx, double batch_start_time)
{
    set_time(time_offset, mcu_freq);
    double   offset     = print_time - m_last_step_print_time;
    double   rel_sc     = (step_time + offset) * m_mcu_freq;
    uint64_t step_clock = m_last_step_clock + (uint64_t) rel_sc;
    if (m_next_step_clock) {
        if (sdir != m_next_step_dir) {
            double diff = (double) (int64_t) (step_clock - m_next_step_clock);
            if (diff < SDS_FILTER_TIME * m_mcu_freq) {
                m_next_step_clock = 0;
                m_next_step_dir   = sdir;
                return;
            }
        }
        queue_append();
    }
    m_next_step_clock  = step_clock;
    m_next_step_dir    = sdir;
    m_next_line_idx    = line_idx;
    m_next_batch_start = batch_start_time;
}

void KlipperStepCompress::sync_time(double time_offset, double mcu_freq) { set_time(time_offset, mcu_freq); }

void KlipperStepCompress::commit()
{
    if (m_next_step_clock)
        queue_append();
}

void KlipperStepCompress::reset_to_clock(uint64_t last_step_clock)
{
    flush(UINT64_MAX);
    m_last_step_clock = last_step_clock;
    m_sdir            = -1;
    calc_last_step_print_time();
}

void KlipperStepCompress::set_last_position(uint64_t clock, int64_t last_position)
{
    flush(UINT64_MAX);
    m_last_position = last_position;
    // m_history 浠?find_past_position 浣跨敤锛屾ā鎷熼摼璺湭璋冪敤 鈫?涓嶅啀缁存姢
    // HistoryStep hs{clock, clock, last_position};
    // m_history.insert(m_history.begin(), hs);
}

int64_t KlipperStepCompress::find_past_position(uint64_t clock) const
{
    int64_t last_position = m_last_position;
    for (const HistoryStep& hs : m_history) {
        if (clock < hs.first_clock) {
            last_position = hs.start_position;
            continue;
        }
        if (clock >= hs.last_clock)
            return hs.start_position + hs.step_count;
        int32_t interval = hs.interval;
        int32_t add      = hs.add;
        int32_t ticks    = (int32_t) (clock - hs.first_clock) + interval;
        int32_t offset   = 0;
        if (!add) {
            offset = ticks / interval;
        } else {
            double a = .5 * add, b = interval - .5 * add, c = -ticks;
            offset = (int32_t) ((std::sqrt(b * b - 4 * a * c) - b) / (2. * a));
        }
        if (hs.step_count < 0)
            return hs.start_position - offset;
        return hs.start_position + offset;
    }
    return last_position;
}

void KlipperStepCompress::flush(uint64_t move_clock)
{
    if (m_next_step_clock && move_clock >= m_next_step_clock)
        queue_append();
    queue_flush(move_clock);
}

void KlipperStepCompress::flush_to(uint64_t move_clock) { flush(move_clock); }

void KlipperStepCompress::finish()
{
    commit();
    flush(UINT64_MAX);
}

}} // namespace Slic3r::KlipperSim
