#ifndef slic3r_GCode_KlipperSim_KlipperClockSync_hpp_
#define slic3r_GCode_KlipperSim_KlipperClockSync_hpp_

#include <cstdint>
#include <tuple>

namespace Slic3r {
namespace KlipperSim {

class KlipperClockSync
{
public:
    explicit KlipperClockSync(double mcu_freq = 1.0);
    virtual ~KlipperClockSync() = default;

    void set_mcu_freq(double mcu_freq);
    double mcu_freq() const { return m_mcu_freq; }

    virtual uint64_t print_time_to_clock(double print_time) const;
    virtual double clock_to_print_time(uint64_t clock) const;

    virtual uint64_t get_clock(double eventtime) const;
    double estimate_clock_systime(uint64_t reqclock) const;
    virtual double estimated_print_time(double eventtime) const;
    uint64_t clock32_to_clock64(uint32_t clock32) const;

    bool is_active() const { return m_queries_pending <= 4; }
    virtual std::pair<double, double> calibrate_clock(double print_time,
                                                      double eventtime);

    void set_last_clock(uint64_t last_clock) { m_last_clock = last_clock; }
    void set_clock_est(double sample_time, double clock, double freq);
    void set_queries_pending(int queries_pending)
    {
        m_queries_pending = queries_pending;
    }

protected:
    double m_mcu_freq = 1.0;
    uint64_t m_last_clock = 0;
    std::tuple<double, double, double> m_clock_est{0.0, 0.0, 1.0};
    int m_queries_pending = 0;
};

class KlipperSecondarySync : public KlipperClockSync
{
public:
    explicit KlipperSecondarySync(KlipperClockSync& main_sync,
                                  double mcu_freq = 1.0);

    uint64_t print_time_to_clock(double print_time) const;
    double clock_to_print_time(uint64_t clock) const;
    std::pair<double, double> calibrate_clock(double print_time,
                                              double eventtime) override;
    uint64_t get_clock(double eventtime) const override;
    double estimated_print_time(double eventtime) const override;

private:
    KlipperClockSync& m_main_sync;
    double m_adjusted_offset = 0.0;
    double m_adjusted_freq = 1.0;
    double m_last_sync_time = 0.0;
};

}} // namespace Slic3r::KlipperSim

#endif
