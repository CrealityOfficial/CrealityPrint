#include "KlipperMCU.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace Slic3r {
namespace KlipperSim {

static int calc_host_move_slots(int move_count,
                                double reserved_move_slots_ratio,
                                int reserved_move_slots)
{
    const int reserved_ratio_slots =
        (int)std::floor((double)move_count * reserved_move_slots_ratio);
    return move_count - reserved_ratio_slots - reserved_move_slots;
}

void KlipperMCU::request_move_queue_slot()
{
    ++m_reserved_move_slots;
    m_host_move_slots = calc_host_move_slots(m_firmware_move_count,
                                             m_reserved_move_slots_ratio,
                                             m_reserved_move_slots);
    if (m_host_move_slots < 0)
        throw std::runtime_error("KlipperMCU: negative host move slots");
}

KlipperMCU::KlipperMCU(std::string name, int move_count, double buffer_time_s,
                       double mcu_freq_hz, double reserved_move_slots_ratio,
                       int reserved_move_slots, int physical_move_count)
    : KlipperMCU(std::move(name), move_count, buffer_time_s,
                 std::make_unique<KlipperClockSync>(mcu_freq_hz),
                 reserved_move_slots_ratio, reserved_move_slots,
                 physical_move_count)
{
}

KlipperMCU::KlipperMCU(std::string name, int move_count, double buffer_time_s,
                       std::unique_ptr<KlipperClockSync> clocksync,
                       double reserved_move_slots_ratio,
                       int reserved_move_slots, int physical_move_count)
    : m_name(std::move(name))
    , m_firmware_move_count(move_count)
    , m_physical_move_count(physical_move_count >= 0 ? physical_move_count
                                                     : move_count)
    , m_reserved_move_slots(reserved_move_slots)
    , m_host_move_slots(calc_host_move_slots(move_count,
                                             reserved_move_slots_ratio,
                                             reserved_move_slots))
    , m_reserved_move_slots_ratio(reserved_move_slots_ratio)
    , m_mcu_freq(clocksync ? clocksync->mcu_freq() : 1.0)
    , m_clocksync(std::move(clocksync))
    , m_steppersync(0, buffer_time_s)
{
    if (m_host_move_slots < 0)
        throw std::runtime_error("KlipperMCU: negative host move slots");
    if (m_physical_move_count < m_host_move_slots)
        throw std::runtime_error("KlipperMCU: physical pool smaller than host move slots");
    if (m_clocksync == nullptr)
        m_clocksync = std::make_unique<KlipperClockSync>(m_mcu_freq);
    // Host steppersync reserves slots for non-step command queues, while the
    // MCU move_free_list remains the full firmware move_count shared by all
    // step/PWM/digital handlers.
    m_steppersync = KlipperSteppersync(m_host_move_slots, buffer_time_s,
                                       m_physical_move_count);
    m_steppersync.set_debug_name(m_name);
    m_steppersync.set_time(0.0, m_mcu_freq);
}

void KlipperMCU::set_clocksync(std::unique_ptr<KlipperClockSync> clocksync)
{
    if (clocksync == nullptr)
        return;
    m_mcu_freq = clocksync->mcu_freq();
    m_clocksync = std::move(clocksync);
    m_steppersync.set_time(0.0, m_mcu_freq);
}

uint64_t KlipperMCU::print_time_to_clock(double print_time) const
{
    return m_clocksync->print_time_to_clock(print_time);
}

double KlipperMCU::clock_to_print_time(uint64_t clock) const
{
    return m_clocksync->clock_to_print_time(clock);
}

double KlipperMCU::estimated_print_time(double eventtime) const
{
    return m_clocksync->estimated_print_time(eventtime);
}

void KlipperMCU::check_active(double print_time, double eventtime)
{
    const auto [offset, freq] = m_clocksync->calibrate_clock(print_time,
                                                             eventtime);
    m_steppersync.set_time(offset, freq);
}

void KlipperMCU::flush_moves(double print_time)
{
    m_last_flush_print_time = std::max(m_last_flush_print_time, print_time);
    m_last_flush_clock = print_time_to_clock(m_last_flush_print_time);
    if ((int64_t)m_last_flush_clock < 0)
        return;
    m_steppersync.flush_to(print_time);
}

}} // namespace Slic3r::KlipperSim
