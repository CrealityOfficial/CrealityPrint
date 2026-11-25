#ifndef SLIC3R_WIPE_TOWER_IDLE_PURGE_HPP
#define SLIC3R_WIPE_TOWER_IDLE_PURGE_HPP

#include <algorithm>
#include <cstddef>
#include <vector>

namespace Slic3r {
// Estimated G-code times are in seconds. The filament setting is in minutes.
class WipeTowerIdlePurgeClock {
public:
    float change(size_t old_tool, size_t new_tool, double now,
                 double minutes_to_maximum, float minimum_volume, double maximum_volume)
    {
        const size_t count = std::max(old_tool, new_tool) + 1;
        if (m_channels.size() < count)
            m_channels.resize(count, Channel{0., m_all_flushed});

        const Channel target = m_channels[new_tool];
        m_channels[old_tool] = {now, false};
        m_channels[new_tool] = {now, false};
        if (old_tool == new_tool || !(minutes_to_maximum > 0.) || !(maximum_volume > 0.))
            return 0.f;
        if (target.flushed_since_use)
            return minimum_volume;

        const double factor = std::clamp((now - target.last_use) / (minutes_to_maximum * 60.), 0., 1.);
        const double maximum = std::max(double(minimum_volume), maximum_volume);
        return float(minimum_volume + (maximum - minimum_volume) * factor);
    }

    void flush_all()
    {
        m_all_flushed = true;
        for (Channel& channel : m_channels)
            channel.flushed_since_use = true;
    }

private:
    struct Channel {
        double last_use;
        bool flushed_since_use;
    };
    bool m_all_flushed = false;
    std::vector<Channel> m_channels;
};
} // namespace Slic3r
#endif
