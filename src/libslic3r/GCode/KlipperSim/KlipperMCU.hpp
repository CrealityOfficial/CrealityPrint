#ifndef slic3r_GCode_KlipperSim_KlipperMCU_hpp_
#define slic3r_GCode_KlipperSim_KlipperMCU_hpp_

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>

#include "KlipperClockSync.hpp"
#include "KlipperSteppersync.hpp"

namespace Slic3r {
namespace KlipperSim {

// Offline host-side MCU model mirroring the parts of klippy/mcu.py that are
// relevant to motion scheduling: move_count, reserved move slots,
// print_time<->clock conversion, and flush_moves() forwarding into steppersync.
class KlipperMCU
{
public:
    KlipperMCU(std::string name, int move_count, double buffer_time_s,
               double mcu_freq_hz, double reserved_move_slots_ratio = 0.0,
               int reserved_move_slots = 0, int physical_move_count = -1);
    KlipperMCU(std::string name, int move_count, double buffer_time_s,
               std::unique_ptr<KlipperClockSync> clocksync,
               double reserved_move_slots_ratio = 0.0,
               int reserved_move_slots = 0, int physical_move_count = -1);

    void request_move_queue_slot();

    int firmware_move_count() const { return m_firmware_move_count; }
    int physical_move_count() const { return m_physical_move_count; }
    int reserved_move_slots() const { return m_reserved_move_slots; }
    int host_move_slots() const { return m_host_move_slots; }
    void set_clocksync(std::unique_ptr<KlipperClockSync> clocksync);
    KlipperClockSync& clocksync() { return *m_clocksync; }
    const KlipperClockSync& clocksync() const { return *m_clocksync; }

    uint64_t print_time_to_clock(double print_time) const;
    double clock_to_print_time(uint64_t clock) const;
    double estimated_print_time(double eventtime) const;

    void set_time_offset(double time_offset)
    {
        m_clocksync->set_clock_est(0.0,
                                   (double)m_clocksync->print_time_to_clock(time_offset),
                                   m_clocksync->mcu_freq());
    }
    void set_estimated_print_time(double print_time)
    {
        m_last_flush_print_time = std::max(m_last_flush_print_time, print_time);
    }
    void check_active(double print_time, double eventtime);

    void flush_moves(double print_time);

    KlipperSteppersync& steppersync() { return m_steppersync; }
    const KlipperSteppersync& steppersync() const { return m_steppersync; }

    double last_flush_print_time() const { return m_last_flush_print_time; }
    uint64_t last_flush_clock() const { return m_last_flush_clock; }
    const std::string& name() const { return m_name; }

private:
    void rebuild_steppersync();

    std::string m_name;
    int m_firmware_move_count;
    int m_physical_move_count;
    int m_reserved_move_slots = 0;
    int m_host_move_slots;
    double m_reserved_move_slots_ratio;
    double m_mcu_freq;
    double m_last_flush_print_time = 0.0;
    uint64_t m_last_flush_clock = 0;
    std::unique_ptr<KlipperClockSync> m_clocksync;
    KlipperSteppersync m_steppersync;
};

}} // namespace Slic3r::KlipperSim

#endif
