#ifndef slic3r_GCode_KlipperSim_KlipperHostDispatch_hpp_
#define slic3r_GCode_KlipperSim_KlipperHostDispatch_hpp_

#include <functional>
#include <cstdint>
#include <memory>
#include <vector>

#include "KlipperToolhead.hpp"

namespace Slic3r {
namespace KlipperSim {

enum HostCmdSourceKind {
    HCS_XY_STEP = 0,
    HCS_E_STEP = 1,
    HCS_AUX_FAN = 2,
    HCS_AUX_HEATER = 3,
    HCS_AUX_BED = 4,
    HCS_UNKNOWN = 99
};

struct HostDispatchCmd
{
    // batch_start_time: source trap move / aux segment start on host side.
    double batch_start_time = 0.0;
    // ready_time: compatibility field kept for older diagnostics.
    double ready_time = 0.0;
    // min_time: host-side earliest transmit time after steppersync move-slot heap.
    double min_time = 0.0;
    // req_time: target MCU print_time / waketime requested by command.
    double req_time = 0.0;
    // slot_free_time: MCU-side shared move-node release time.
    double slot_free_time = 0.0;
    // exec_start/end: physical execution interval, not shared-pool occupancy interval.
    double exec_start = 0.0;
    double exec_end = 0.0;
    // recv_time: real receive time after serialqueue block transmission completes.
    double recv_time = 0.0;
    int    line_idx = -1;
    int    source_kind = HCS_UNKNOWN;
    // uses_move_slot: command consumes basecmd.c shared move node pool.
    bool   uses_move_slot = true;
    // background_priority: legacy compatibility shell; mainline path now keeps false.
    bool   background_priority = false;
    int    msg_len = 5;
    // Host eventtime at which serialqueue_send_batch() receives this command.
    double enqueue_time = 0.0;
    // Commands in one command_queue are strict FIFO. Queue 0 is steppersync;
    // auxiliary devices use separate queues and compete only via queue heads.
    int    command_queue_id = 0;
    uint64_t enqueue_sequence = 0;
    // Deterministic serialqueue trace populated by schedule_host_dispatch().
    double send_time = 0.0;
    double block_end_time = 0.0;
    double ack_time = 0.0;
    uint64_t block_sequence = 0;
    int block_index = -1;
};

enum class HostTransportKind {
    Uart,
    Can
};

struct HostDispatchConfig
{
    HostTransportKind transport = HostTransportKind::Uart;
    double wire_frequency = 230400.0;
    double clock_frequency = 120000000.0;
    int receive_window = 192;
    int ack_message_bytes = 5;
};

// A firmware-semantic upper envelope for a legal host-starvation recovery:
// the previous MCU queue has drained, then one already-planned lookahead batch
// becomes available and Klippy resynchronizes its print-time origin.  This is
// deliberately separate from the nominal continuous-supply dispatch result.
struct LateBatchEnvelope
{
    int host_step_peak = 0;
    int peak = 0;
    bool overflow = false;
    double peak_time = 0.0;
    double source_batch_start = 0.0;
    int first_line = -1;
    int last_line = -1;
    size_t batch_commands = 0;
    size_t move_commands = 0;
};

LateBatchEnvelope analyze_late_batch_envelope(
    const std::vector<HostDispatchCmd>& commands,
    int host_move_slots,
    int physical_pool_slots,
    const HostDispatchConfig& config,
    double resync_lead_time = 0.250,
    size_t source_batch_window = 1);

// Invoked in serialqueue receive order after a complete block has reached the
// MCU.  The callback is the MCU command-handler boundary: move_alloc() must
// happen here, not in a later flush-window replay.
using HostReceiveCallback = std::function<void(const HostDispatchCmd&)>;

enum class SimulationControl { Continue, StopRiskDetected };

using HostReceiveControlCallback =
    std::function<SimulationControl(const HostDispatchCmd&)>;

struct HostDispatchOutcome {
    SimulationControl control = SimulationControl::Continue;
    size_t commands_consumed = 0;
};

struct DualHostDispatchOutcome {
    HostDispatchOutcome primary;
    HostDispatchOutcome secondary;
};

// Persistent single-MCU serialqueue. Commands appended at one host eventtime
// must be appended as a complete batch before advance_to() reaches that time.
class DeterministicSerialQueueSession {
public:
    DeterministicSerialQueueSession(
        const HostDispatchConfig& config = {},
        const std::function<double(double)>& estimated_print_time = {},
        const HostReceiveControlCallback& receive_callback = {},
        bool retain_command_trace = true);
    ~DeterministicSerialQueueSession();

    DeterministicSerialQueueSession(const DeterministicSerialQueueSession&) = delete;
    DeterministicSerialQueueSession& operator=(const DeterministicSerialQueueSession&) = delete;

    void append(const HostDispatchCmd& command);
    void append(HostDispatchCmd&& command);
    SimulationControl advance_to(double host_eventtime);
    SimulationControl drain_ready();
    HostDispatchOutcome finish(double final_host_eventtime);
    std::vector<HostDispatchCmd> take_commands();
    size_t command_count() const;
    size_t commands_consumed() const;
    size_t retained_command_count() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

DualHostDispatchOutcome schedule_host_dispatch_until(
    std::vector<HostDispatchCmd>& primary_cmds,
    std::vector<HostDispatchCmd>& secondary_cmds,
    const std::vector<ToolheadFlushWindow>& windows,
    double final_host_eventtime,
    double buffer_time_high,
    int primary_slots,
    int secondary_slots,
    const std::function<double(double)>& primary_estimated_print_time_cb = {},
    const std::function<double(double)>& secondary_estimated_print_time_cb = {},
    const HostDispatchConfig& primary_config = {},
    const HostDispatchConfig& secondary_config = {},
    const HostReceiveControlCallback& primary_receive_cb = {},
    const HostReceiveControlCallback& secondary_receive_cb = {});

void schedule_host_dispatch(std::vector<HostDispatchCmd>& primary_cmds,
                            std::vector<HostDispatchCmd>& secondary_cmds,
                            const std::vector<ToolheadFlushWindow>& windows,
                            double final_host_eventtime,
                            double buffer_time_high,
                            int primary_slots,
                            int secondary_slots,
                            const std::function<double(double)>& primary_estimated_print_time_cb = {},
                            const std::function<double(double)>& secondary_estimated_print_time_cb = {},
                            const HostDispatchConfig& primary_config = {},
                            const HostDispatchConfig& secondary_config = {},
                            const HostReceiveCallback& primary_receive_cb = {},
                            const HostReceiveCallback& secondary_receive_cb = {});

}} // namespace Slic3r::KlipperSim

#endif
