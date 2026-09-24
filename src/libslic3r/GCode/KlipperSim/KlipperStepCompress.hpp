#ifndef slic3r_GCode_KlipperSim_KlipperStepCompress_hpp_
#define slic3r_GCode_KlipperSim_KlipperStepCompress_hpp_

// ---------------------------------------------------------------------------
// KlipperStepCompress
// ---------------------------------------------------------------------------
// A faithful (1:1) port of Klipper firmware's stepcompress.c core:
//   - compress_bisect_add(): the integer bisection compressor that turns a
//     series of absolute step clock times into a minimal set of "queue_step"
//     commands {interval, count, add}.
//   - add_move()/queue accounting: how many queue_step commands a run of steps
//     produces (this count IS the compression ratio C we could never estimate).
//
// Why this matters for the slicer:
//   The MCU shared move-node pool stores queue_step *commands*. Each command
//   allocates one node when received, and that node is freed in stepper.c
//   stepper_load_next() when the command is popped from the stepper queue and
//   loaded into the active stepper state. To predict overflow we therefore
//   need the REAL queue_step command stream and its true pop/load timing, i.e.
//   the real compression this port computes, instead of the old C=1 guess.
//
// Units: all times here are MCU clock ticks (uint32/uint64). mcu_freq converts
// seconds<->ticks. max_error is in ticks (Klipper: max_stepper_error * freq).
// ---------------------------------------------------------------------------

#include <cstdint>
#include <vector>

namespace Slic3r {
namespace KlipperSim {

// One emitted queue_step command (mirrors firmware struct step_move).
struct StepMoveCmd
{
    uint32_t interval; // ticks to first step (and base of the arithmetic seq)
    uint16_t count;    // number of step pulses this command emits
    int16_t  add;      // per-step interval increment (acceleration)
    uint64_t first_clock = 0;     // first step time of this command
    uint64_t last_clock = 0;      // last step time of this command
    uint64_t min_clock = 0;       // original qm->min_clock (shared move-slot free time)
    uint64_t req_clock = 0;       // host-side req_clock used by steppersync
    uint64_t free_clock = 0;      // physical execution end (last step clock)
    double   batch_start_time = 0.0;
    int      line_idx = -1;
    uint64_t output_sequence = 0; // position in firmware msg_queue
};

struct StepAuxCmd
{
    uint64_t req_clock = 0;
    double   batch_start_time = 0.0;
    int      line_idx = -1;
    int      msg_len = 3;
    uint64_t output_sequence = 0;
};

// Faithful port of firmware stepcompress for a single stepper (oid).
// Feed absolute step clock times in increasing order; it emits StepMoveCmd.
class KlipperStepCompress
{
public:
    // max_error_ticks: Klipper max_stepper_error * mcu_freq (default 0.000025 s).
    explicit KlipperStepCompress(uint32_t max_error_ticks);

    // Reset internal state for a fresh run (keeps configuration).
    void reset();

    // Append one scheduled step at absolute clock time `step_clock` (ticks).
    // Steps must be provided in non-decreasing clock order.
    void append_step(uint64_t step_clock);
    void append_step_time(int sdir, double print_time, double step_time,
                          double mcu_freq, double time_offset = 0.0,
                          int line_idx = -1,
                          double batch_start_time = 0.0);
    void sync_time(double time_offset, double mcu_freq);
    void commit();
    void reset_to_clock(uint64_t last_step_clock);
    void set_last_position(uint64_t clock, int64_t last_position);
    int64_t find_past_position(uint64_t clock) const;

    // Flush any buffered steps into commands (call once after the last step).
    void finish();
    void flush_to(uint64_t move_clock);

    // The emitted command stream after finish().
    const std::vector<StepMoveCmd>& commands() const { return m_commands; }
    std::vector<StepMoveCmd>& commands_mut() { return m_commands; }
    const std::vector<StepAuxCmd>& aux_commands() const { return m_aux_cmds; }
    void drain_output(std::vector<StepMoveCmd>& moves,
                      std::vector<StepAuxCmd>& aux)
    {
        moves.clear();
        aux.clear();
        moves.swap(m_commands);
        aux.swap(m_aux_cmds);
    }

    // Convenience: number of queue_step commands emitted so far.
    size_t command_count() const { return m_commands.size(); }
    int current_step_dir() const { return m_next_step_dir; }
    void release_output_storage()
    {
        std::vector<StepMoveCmd>().swap(m_commands);
        std::vector<StepAuxCmd>().swap(m_aux_cmds);
    }

private:
    struct HistoryStep {
        uint64_t first_clock{ 0 };
        uint64_t last_clock{ 0 };
        int64_t  start_position{ 0 };
        int      step_count{ 0 };
        int      interval{ 0 };
        int      add{ 0 };
    };

    std::vector<uint64_t>    m_queue;
    std::vector<int>         m_queue_line_idx;
    std::vector<double>      m_queue_batch_start;
    size_t                   m_queue_pos{ 0 };
    uint32_t                 m_max_error{ 0 };
    double                   m_mcu_time_offset{ 0.0 };
    double                   m_mcu_freq{ 0.0 };
    double                   m_last_step_print_time{ 0.0 };
    uint64_t                 m_last_step_clock{ 0 };
    int                      m_sdir{ -1 };
    int                      m_invert_sdir{ 0 };
    uint64_t                 m_next_step_clock{ 0 };
    int                      m_next_step_dir{ 0 };
    int                      m_next_line_idx{ -1 };
    double                   m_next_batch_start{ 0.0 };
    int64_t                  m_last_position{ 0 };
    std::vector<HistoryStep> m_history;
    std::vector<StepMoveCmd> m_commands;
    std::vector<StepAuxCmd>  m_aux_cmds;
    bool                     m_have_emitted_move{ false };
    uint64_t                 m_next_output_sequence{ 0 };

    // Ported from firmware stepcompress.c
    struct Points { int32_t minp, maxp; };
    static constexpr double HISTORY_EXPIRE = 30.0;

    void        free_history(uint64_t end_clock);
    void        calc_last_step_print_time();
    Points minmax_point(size_t pos) const;
    StepMoveCmd compress_bisect_add() const;
    void        emit_move(const StepMoveCmd& mv);
    void        set_time(double time_offset, double mcu_freq);
    void        set_next_step_dir(int sdir);
    void        queue_flush(uint64_t move_clock);
    void        queue_append_far();
    void        queue_append_extend();
    void        flush(uint64_t move_clock);
    void        flush_far(uint64_t abs_step_clock);
    void        queue_append();
};

}} // namespace Slic3r::KlipperSim

#endif
