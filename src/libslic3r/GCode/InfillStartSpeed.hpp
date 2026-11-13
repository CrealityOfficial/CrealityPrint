#ifndef slic3r_InfillStartSpeed_hpp_
#define slic3r_InfillStartSpeed_hpp_

#include <algorithm>
#include <cmath>

namespace Slic3r {

// Limit initial model infill by the requested extrusion volume.
// This counter is independent of the tower purge budget.
class InfillStartSpeed
{
public:
    struct Segment {
        double fraction = 0.;
        double speed = 0.;
    };

    void reset(double flush_volume, double max_volumetric_speed, double speed_percent)
    {
        m_remaining_volume = 0.;
        m_flow_limit = 0.;
        if (std::isfinite(flush_volume) && flush_volume > 0. &&
            std::isfinite(max_volumetric_speed) && max_volumetric_speed > 0. &&
            std::isfinite(speed_percent) && speed_percent > 0. && speed_percent <= 100.) {
            m_remaining_volume = flush_volume;
            m_flow_limit = max_volumetric_speed * (speed_percent / 100.);
        }
    }

    bool active() const { return m_remaining_volume > 1e-9; }
    double remaining_volume() const { return m_remaining_volume; }
    double flow_limit() const { return m_flow_limit; }

    Segment consume(double volume, double distance, double speed)
    {
        if (!active() || !std::isfinite(volume) || volume <= 0. ||
            !std::isfinite(distance) || distance <= 0.)
            return {};
        const double fraction = std::min(1., m_remaining_volume / volume);
        m_remaining_volume = std::max(0., m_remaining_volume - volume);
        return {fraction, std::min(speed, m_flow_limit * distance / volume)};
    }

private:
    double m_remaining_volume = 0.;
    double m_flow_limit = 0.;
};

} // namespace Slic3r
#endif
