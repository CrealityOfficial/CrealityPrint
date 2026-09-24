#ifndef slic3r_GCode_KlipperSim_KlipperOnlineStepGeneration_hpp_
#define slic3r_GCode_KlipperSim_KlipperOnlineStepGeneration_hpp_

#include "KlipperItersolve.hpp"
#include "KlipperMCU.hpp"
#include "KlipperSimMain.hpp"
#include "KlipperStepper.hpp"
#include "KlipperToolhead.hpp"
#include "KlipperTrapQ.hpp"

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

namespace Slic3r {
namespace KlipperSim {

using OnlineCommandBatchCb = std::function<SimulationControl(
    std::vector<HostDispatchCmd>&&, std::vector<HostDispatchCmd>&&, double)>;
// Synchronous streaming sink. The callback may move elements out, but must not
// retain references; the vectors are cleared after it returns so capacity is
// reused by the next firmware window.
using OnlineBorrowedCommandBatchCb = std::function<SimulationControl(
    std::vector<HostDispatchCmd>&, std::vector<HostDispatchCmd>&, double)>;

struct OnlineStepWindow
{
    double flush_time = 0.0;
    size_t a_steps = 0;
    size_t b_steps = 0;
    size_t e_steps = 0;
    size_t a_commands = 0;
    size_t b_commands = 0;
    size_t e_commands = 0;
    size_t main_scheduled = 0;
    size_t nozzle_scheduled = 0;
};

// Persistent trapq -> itersolve -> stepcompress front end.  The same object
// consumes every toolhead flush window, so solver cursors and step direction
// survive across windows exactly as they do in Klipper's step generators.
class KlipperOnlineStepGeneration
{
public:
    KlipperOnlineStepGeneration(const SimConfig& cfg,
                                KlipperMCU& main_mcu,
                                KlipperMCU& nozzle_mcu,
                                bool release_trapq = true,
                                OnlineCommandBatchCb command_batch_cb = {},
                                OnlineBorrowedCommandBatchCb borrowed_batch_cb = {},
                                bool retain_windows = true);

    void on_flush(const ToolheadFlushWindow& window,
                  KlipperTrapQ& xy_trapq,
                  KlipperTrapQ& e_trapq);
    void finish(double last_kin_move_time,
                const KlipperTrapQ& xy_trapq,
                const KlipperTrapQ& e_trapq);

    KlipperStepCompress& a_stepqueue() { return m_stepper_a.stepqueue(); }
    KlipperStepCompress& b_stepqueue() { return m_stepper_b.stepqueue(); }
    KlipperStepCompress& e_stepqueue() { return m_stepper_e.stepqueue(); }
    const KlipperStepCompress& a_stepqueue() const { return m_stepper_a.stepqueue(); }
    const KlipperStepCompress& b_stepqueue() const { return m_stepper_b.stepqueue(); }
    const KlipperStepCompress& e_stepqueue() const { return m_stepper_e.stepqueue(); }
    const std::vector<OnlineStepWindow>& windows() const { return m_windows; }
    const std::vector<HostDispatchCmd>& main_commands() const { return m_main_commands; }
    const std::vector<HostDispatchCmd>& nozzle_commands() const { return m_nozzle_commands; }
    std::vector<HostDispatchCmd> take_main_commands() { return std::move(m_main_commands); }
    std::vector<HostDispatchCmd> take_nozzle_commands() { return std::move(m_nozzle_commands); }
    size_t total_xy_phases() const { return m_total_xy_phases; }
    size_t total_e_phases() const { return m_total_e_phases; }
    double prepare_time_ms() const { return m_prepare_time_ms; }
    double generate_a_time_ms() const { return m_generate_a_time_ms; }
    double generate_b_time_ms() const { return m_generate_b_time_ms; }
    double generate_e_time_ms() const { return m_generate_e_time_ms; }
    double generation_wall_time_ms() const { return m_generation_wall_time_ms; }
    size_t generation_invocations() const { return m_generation_invocations; }
    size_t parallel_generation_invocations() const { return m_parallel_generation_invocations; }
    size_t serial_generation_invocations() const { return m_serial_generation_invocations; }
    size_t parallel_phase_total() const { return m_parallel_phase_total; }
    size_t serial_phase_total() const { return m_serial_phase_total; }
    size_t max_generation_phases() const { return m_max_generation_phases; }
    double mcu_check_time_ms() const { return m_mcu_check_time_ms; }
    double main_flush_time_ms() const { return m_main_flush_time_ms; }
    double nozzle_flush_time_ms() const { return m_nozzle_flush_time_ms; }
    double publish_time_ms() const { return m_publish_time_ms; }
    double trapq_release_time_ms() const { return m_trapq_release_time_ms; }
    bool parallel_step_generation() const { return m_parallel_step_generation; }
    bool stopped() const { return m_stopped; }
    size_t retained_dispatch_commands() const
    {
        return m_main_commands.size() + m_nozzle_commands.size();
    }

private:
    void generate_to(double flush_time,
                     const KlipperTrapQ& xy_trapq,
                     const KlipperTrapQ& e_trapq);
    SimulationControl publish_commands(double host_eventtime);

    SimConfig m_cfg;
    KlipperMCU& m_main_mcu;
    KlipperMCU& m_nozzle_mcu;
    KlipperStepper m_stepper_a;
    KlipperStepper m_stepper_b;
    KlipperStepper m_stepper_e;
    KlipperItersolve m_solver_a;
    KlipperItersolve m_solver_b;
    KlipperItersolve m_solver_e;
    double m_last_flush_time = -1.0;
    double m_last_host_eventtime = 0.0;
    size_t m_scanned_e_phases = 0;
    size_t m_known_xy_phases = 0;
    size_t m_known_e_phases = 0;
    size_t m_total_xy_phases = 0;
    size_t m_total_e_phases = 0;
    double m_max_pa_half_window = 0.0;
    size_t m_a_steps = 0;
    size_t m_b_steps = 0;
    size_t m_e_steps = 0;
    bool m_finished = false;
    bool m_stopped = false;
    bool m_release_trapq = true;
    bool m_parallel_step_generation = false;
    OnlineCommandBatchCb m_command_batch_cb;
    OnlineBorrowedCommandBatchCb m_borrowed_batch_cb;
    bool m_retain_windows = true;
    std::vector<OnlineStepWindow> m_windows;
    std::vector<HostDispatchCmd> m_main_commands;
    std::vector<HostDispatchCmd> m_nozzle_commands;
    double m_prepare_time_ms = 0.0;
    double m_generate_a_time_ms = 0.0;
    double m_generate_b_time_ms = 0.0;
    double m_generate_e_time_ms = 0.0;
    double m_generation_wall_time_ms = 0.0;
    size_t m_generation_invocations = 0;
    size_t m_parallel_generation_invocations = 0;
    size_t m_serial_generation_invocations = 0;
    size_t m_parallel_phase_total = 0;
    size_t m_serial_phase_total = 0;
    size_t m_max_generation_phases = 0;
    double m_mcu_check_time_ms = 0.0;
    double m_main_flush_time_ms = 0.0;
    double m_nozzle_flush_time_ms = 0.0;
    double m_publish_time_ms = 0.0;
    double m_trapq_release_time_ms = 0.0;
};

}} // namespace Slic3r::KlipperSim

#endif
