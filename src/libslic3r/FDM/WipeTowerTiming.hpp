#ifndef SLIC3R_WIPE_TOWER_TIMING_HPP
#define SLIC3R_WIPE_TOWER_TIMING_HPP

#include "../Point.hpp"
#include <optional>
#include <vector>

namespace Slic3r {
class Print;
struct GCodeProcessorResult;

// Local to one export. Absolute times come from the normal motion planner.
class WipeTowerTiming {
public:
    struct TowerSegment {
        size_t id;
        int old_tool, new_tool;
        double print_z;
        double tower_time; // Cumulative tower motion at the change marker.
        Vec2d entry, exit;
    };
    using Layout = std::vector<TowerSegment>;
    static Layout layout(const Print& print);
    static bool same_tower_plan(const std::vector<float>& planned, const std::vector<float>& measured,
                                const std::vector<float>& normal_volumes);
    static std::optional<double> purge_timing_error(float planned, float measured, float base,
                                                  double minutes, float minimum, float maximum);
    static constexpr double idle_tolerance_seconds = 30.;
    // Maximum equivalent idle-time error, or no value if an endpoint or tool change disagrees.
    std::optional<double> tower_timing_error(const std::vector<float>& planned, const std::vector<float>& measured,
                                             const std::vector<float>& normal_volumes) const;
    static std::optional<std::vector<float>> saturated_plan(const Print& print, const Layout& baseline);

    void change(size_t id, size_t old_tool, size_t new_tool, double time, double minutes,
                float minimum, float maximum, const GCodeProcessorResult& result);
    void start_periodic(double time);
    enum class SiteKind { ToolChange, TowerEntry, Tower, Infill };
    bool periodic_site(double time, double interval, size_t pending_change, SiteKind kind, unsigned tool);
    void flushed(double end_time, double next_interval);
    // Prefer the same safe nodes as a measured pass while the actual deadline permits it.
    void prefer_periodic_sites(const WipeTowerTiming* reference) { m_reference = reference; }
    bool accepted_schedule() const { return !m_missed_window && !m_waiting; }
    // Negative when every flush used the first eligible node.
    double max_deferred_lateness() const { return m_max_deferred_lateness; }

    // A prediction only proposes a tower. The full motion planner checks it before export.
    std::optional<std::vector<float>> predict(const Layout& baseline, const Layout& next,
                                              const Print& print) const;
private:
    struct Change {
        size_t id, old_tool, new_tool;
        double time, minutes;
        float minimum, maximum;
        std::optional<Vec2d> first_model, last_model;
    };
    struct Site {
        size_t changes_before, pending_change;
        double time, interval;
        SiteKind kind;
        unsigned tool;
        std::optional<double> duration;
    };
    std::vector<Change> m_changes;
    std::vector<Site> m_sites;
    bool can_wait(size_t index, const Site& current, double overdue) const;
    const WipeTowerTiming* m_reference = nullptr;
    size_t m_move_index = 1;
    double m_periodic_start = 0.;
    double m_periodic_origin = 0.;
    bool m_waiting = false;
    bool m_missed_window = false;
    double m_max_deferred_lateness = -1.;
    bool m_dynamic_interval = false;
};
}
#endif
