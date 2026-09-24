#include "KlipperClockSync.hpp"

#include <algorithm>
#include <cmath>

namespace Slic3r {
namespace KlipperSim {

KlipperClockSync::KlipperClockSync(double mcu_freq)
    : m_mcu_freq(mcu_freq)
    , m_clock_est(0.0, 0.0, mcu_freq)
{
}

void KlipperClockSync::set_mcu_freq(double mcu_freq)
{
    m_mcu_freq = mcu_freq;
    auto [sample_time, clock, freq] = m_clock_est;
    (void)freq;
    m_clock_est = std::make_tuple(sample_time, clock, mcu_freq);
}

uint64_t KlipperClockSync::print_time_to_clock(double print_time) const
{
    return (uint64_t)std::llround(print_time * m_mcu_freq);
}

double KlipperClockSync::clock_to_print_time(uint64_t clock) const
{
    return (double)clock / m_mcu_freq;
}

uint64_t KlipperClockSync::get_clock(double eventtime) const
{
    auto [sample_time, clock, freq] = m_clock_est;
    return (uint64_t)std::llround(clock + (eventtime - sample_time) * freq);
}

double KlipperClockSync::estimate_clock_systime(uint64_t reqclock) const
{
    auto [sample_time, clock, freq] = m_clock_est;
    return ((double)reqclock - clock) / freq + sample_time;
}

double KlipperClockSync::estimated_print_time(double eventtime) const
{
    return clock_to_print_time(get_clock(eventtime));
}

uint64_t KlipperClockSync::clock32_to_clock64(uint32_t clock32) const
{
    uint64_t clock_diff = (m_last_clock - clock32) & 0xffffffffULL;
    if (clock_diff & 0x80000000ULL)
        return m_last_clock + 0x100000000ULL - clock_diff;
    return m_last_clock - clock_diff;
}

std::pair<double, double> KlipperClockSync::calibrate_clock(double,
                                                            double)
{
    return {0.0, m_mcu_freq};
}

void KlipperClockSync::set_clock_est(double sample_time, double clock, double freq)
{
    m_clock_est = std::make_tuple(sample_time, clock, freq);
}

KlipperSecondarySync::KlipperSecondarySync(KlipperClockSync& main_sync,
                                           double mcu_freq)
    : KlipperClockSync(mcu_freq)
    , m_main_sync(main_sync)
    , m_adjusted_freq(mcu_freq)
{
}

uint64_t KlipperSecondarySync::print_time_to_clock(double print_time) const
{
    return (uint64_t)std::llround((print_time - m_adjusted_offset) * m_adjusted_freq);
}

double KlipperSecondarySync::clock_to_print_time(uint64_t clock) const
{
    return (double)clock / m_adjusted_freq + m_adjusted_offset;
}

std::pair<double, double> KlipperSecondarySync::calibrate_clock(double print_time,
                                                                double eventtime)
{
    double main_est_print_time = m_main_sync.estimated_print_time(eventtime);
    double sync1_print_time = std::max(print_time, main_est_print_time);
    double sync2_print_time = std::max(
        sync1_print_time + 4.0,
        std::max(m_last_sync_time,
                 print_time + 2.5 * (print_time - main_est_print_time)));

    uint64_t sync1_clock = print_time_to_clock(sync1_print_time);
    uint64_t sync2_clock = get_clock(
        m_main_sync.estimate_clock_systime(m_main_sync.print_time_to_clock(sync2_print_time)));
    double adjusted_freq = ((double)sync2_clock - (double)sync1_clock)
                         / (sync2_print_time - sync1_print_time);
    double adjusted_offset = sync1_print_time - (double)sync1_clock / adjusted_freq;

    m_adjusted_offset = adjusted_offset;
    m_adjusted_freq = adjusted_freq;
    m_last_sync_time = sync2_print_time;
    return {m_adjusted_offset, m_adjusted_freq};
}

uint64_t KlipperSecondarySync::get_clock(double eventtime) const
{
    // Klipper SecondarySync does not override get_clock(): it reads the
    // secondary MCU's raw clock estimate and applies clock_adj only in the
    // print-time conversion methods.
    return KlipperClockSync::get_clock(eventtime);
}

double KlipperSecondarySync::estimated_print_time(double eventtime) const
{
    return clock_to_print_time(get_clock(eventtime));
}

}} // namespace Slic3r::KlipperSim
