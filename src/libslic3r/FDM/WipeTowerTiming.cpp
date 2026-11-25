#include "WipeTowerTiming.hpp"
#include "WipeTowerIdlePurge.hpp"
#include "../GCode/GCodeProcessor.hpp"
#include "../GCodeReader.hpp"
#include "../Print.hpp"
#include "../Layer.hpp"
#include <charconv>

namespace Slic3r {

bool WipeTowerTiming::same_tower_plan(const std::vector<float>& planned, const std::vector<float>& measured,
                                     const std::vector<float>& normal_volumes)
{
    if (planned.empty())
        return std::all_of(measured.begin(), measured.end(), [](float v) { return v == 0.f; });
    if (planned.size() != measured.size() || planned.size() != normal_volumes.size())
        return false;
    for (size_t i = 0; i < planned.size(); ++i) {
        // A positive time-based request enables the timed row layout, even when
        // the normal color-change volume already covers the whole request.
        if ((planned[i] > 0.f) != (measured[i] > 0.f) ||
            !(std::abs(std::max(planned[i], normal_volumes[i]) - std::max(measured[i], normal_volumes[i])) <= 0.01f))
            return false;
    }
    return true;
}

std::optional<double> WipeTowerTiming::purge_timing_error(float planned, float measured, float base,
                                                        double minutes, float minimum, float maximum)
{
    const double delta = std::abs(double(std::max(planned, base)) - double(std::max(measured, base)));
    if (delta == 0.) return 0.;
    // A reset and a saturated tool both require the exact effective budget.
    if (measured <= minimum || measured >= maximum || minutes <= 0.) return {};
    return delta * (60. * minutes) / (maximum - minimum);
}

std::optional<double> WipeTowerTiming::tower_timing_error(const std::vector<float>& planned,
                                                          const std::vector<float>& measured,
                                                          const std::vector<float>& normal_volumes) const
{
    if (planned.empty())
        return same_tower_plan(planned, measured, normal_volumes) ? std::optional<double>(0.) : std::nullopt;
    if (planned.size() != measured.size() || planned.size() != normal_volumes.size() || planned.size() != m_changes.size())
        return {};

    double worst_seconds = 0.;
    for (size_t i = 0; i < planned.size(); ++i) {
        const Change& change = m_changes[i];
        if (change.id != i || (planned[i] > 0.f) != (measured[i] > 0.f))
            return {};
        const auto seconds = purge_timing_error(planned[i], measured[i], normal_volumes[i],
                                               change.minutes, change.minimum, change.maximum);
        if (!seconds) return {};
        worst_seconds = std::max(worst_seconds, *seconds);
    }
    return worst_seconds;
}

WipeTowerTiming::Layout WipeTowerTiming::layout(const Print& print)
{
    const auto& config = print.config();
    const auto plate = print.get_plate_index();
    const Vec2d origin = print.get_plate_origin().head<2>() +
        Vec2d(config.wipe_tower_x.get_at(plate), config.wipe_tower_y.get_at(plate));
    const Eigen::Rotation2Dd rotation(Geometry::deg2rad(config.wipe_tower_rotation_angle.value));
    Layout segments;
    double elapsed = 0.;
    for (const auto& layer : print.wipe_tower_data().tool_changes) {
        if (print.canceled()) throw CanceledException();
        for (const auto& block : layer) {
            GCodeReader reader;
            reader.x() = block.start_pos.x();
            reader.y() = block.start_pos.y();
            // Tower writer commands use mm/min and relative extrusion. Parsing the
            // finished block includes walls joined before or after the change marker.
            reader.f() = 60.f * float(config.travel_speed.get_at(0));
            reader.parse_buffer(block.gcode, [&](GCodeReader& state, const GCodeReader::GCodeLine& line) {
                constexpr std::string_view tag = "IDLE_TOWER_CHANGE: ";
                const auto comment = line.comment();
                if (comment.substr(0, tag.size()) == tag) {
                    size_t id;
                    const char* begin = comment.data() + tag.size();
                    if (std::from_chars(begin, comment.data() + comment.size(), id).ec == std::errc{})
                        segments.push_back({id, block.initial_tool, block.new_tool, block.print_z, elapsed,
                            origin + rotation * block.start_pos.cast<double>(),
                            origin + rotation * block.end_pos.cast<double>()});
                }
                if (line.cmd_is("G1") || line.cmd_is("G0")) {
                    const double speed = line.new_F(state);
                    const double xy = line.dist_XY(state);
                    if (speed > 0.)
                        elapsed += 60. * (xy > 0. ? xy : line.has_e() ? std::abs(line.e()) : 0.) / speed;
                }
            });
        }
    }
    return segments;
}

std::optional<std::vector<float>> WipeTowerTiming::saturated_plan(const Print& print, const Layout& baseline)
{
    if (baseline.empty())
        return {};
    // Estimate a model segment from material already assigned to it. If returning
    // tools are already at their maximum idle volume, try the plateau directly.
    // Startup scripts, painted regions and cooling are checked by the exact pass.
    const auto& config = print.config();
    using Key = std::pair<int64_t, unsigned>;
    auto layer_key = [](double z) { return std::llround(z * 10000.); };
    std::map<Key, double> model_time;
    for (const PrintObject* object : print.objects()) {
        for (const Layer* layer : object->layers()) {
            if (print.canceled()) throw CanceledException();
            for (const LayerRegion* region : layer->regions()) {
                auto add = [&](unsigned filament, const ExtrusionEntityCollection& paths) {
                    if (!filament || paths.empty()) return;
                    const double flow = config.filament_max_volumetric_speed.get_at(filament - 1);
                    if (flow > 0.)
                        model_time[{layer_key(layer->print_z), filament - 1}] +=
                            paths.total_volume() * object->instances().size() / flow;
                };
                add(region->region().extruder(frPerimeter), region->perimeters);
                const unsigned infill = region->region().extruder(frInfill);
                if (infill == region->region().extruder(frSolidInfill))
                    add(infill, region->fills);
            }
        }
    }
    std::map<int, double> last;
    std::vector<float> volumes;
    double model_seconds = 0.;
    bool returning = false;
    for (const auto& s : baseline) {
        if (s.id != volumes.size() || s.old_tool < 0 || s.new_tool < 0) return {};
        auto it = model_time.find({layer_key(s.print_z), unsigned(s.old_tool)});
        if (it != model_time.end()) {
            model_seconds += it->second;
            model_time.erase(it);
        }
        const double time = model_seconds + s.tower_time;
        const double minutes = config.filament_pre_flush_tower_time.get_at(s.new_tool);
        const double maximum = config.filament_pre_flush_tower_volume.get_at(s.new_tool);
        float volume = 0.f;
        if (minutes > 0. && maximum > 0.) {
            const double minimum = config.filament_minimal_purge_on_wipe_tower.get_at(s.new_tool);
            volume = float(std::max(minimum, maximum));
            const auto previous = last.find(s.new_tool);
            if (previous != last.end()) {
                if (maximum > minimum && time - previous->second < 60. * minutes) return {};
                returning = true;
            }
        }
        volumes.push_back(volume);
        last[s.old_tool] = last[s.new_tool] = time;
    }
    if (!returning) return {};
    return volumes;
}

void WipeTowerTiming::change(size_t id, size_t old_tool, size_t new_tool, double time, double minutes,
                            float minimum, float maximum, const GCodeProcessorResult& result)
{
    Change change{id, old_tool, new_tool, time, minutes, minimum, maximum, {}, {}};
    // These two anchors bound the model moves between successive visits to the tower.
    for (; m_move_index < result.moves.size(); ++m_move_index) {
        const auto& move = result.moves[m_move_index];
        if (move.type != EMoveType::Extrude || move.extrusion_role == erWipeTower ||
            move.extrusion_role == erCustom || move.extrusion_role == erNone)
            continue;
        if (!change.first_model)
            change.first_model = result.moves[m_move_index - 1].position.head<2>().cast<double>();
        change.last_model = move.position.head<2>().cast<double>();
    }
    m_changes.push_back(std::move(change));
}

void WipeTowerTiming::start_periodic(double time) { m_periodic_start = m_periodic_origin = time; }

static constexpr double max_flush_delay = 30.; // Seconds after the configured minimum interval.

bool WipeTowerTiming::can_wait(size_t index, const Site& current, double overdue) const
{
    if (m_dynamic_interval || index >= m_sites.size() || overdue >= max_flush_delay)
        return false;
    const auto& site = m_sites[index];
    if (site.duration || site.kind != current.kind || site.tool != current.tool ||
        site.changes_before != current.changes_before || site.pending_change != current.pending_change ||
        site.interval != current.interval)
        return false;
    for (size_t i = index + 1; i < m_sites.size(); ++i) {
        if (overdue + (m_sites[i].time - site.time) > max_flush_delay) break;
        if (m_sites[i].duration) return true;
    }
    return false;
}

bool WipeTowerTiming::periodic_site(double time, double interval, size_t pending_change, SiteKind kind, unsigned tool)
{
    m_sites.push_back({m_changes.size(), pending_change, time, interval, kind, tool, {}});
    const double overdue = (time - m_periodic_origin) - interval;
    if (overdue < 0.) return false;
    if (m_waiting && overdue > max_flush_delay) m_missed_window = true;
    if (m_reference && !m_dynamic_interval && !m_missed_window && m_reference->can_wait(m_sites.size() - 1, m_sites.back(), overdue)) {
        m_waiting = true;
        return false;
    }
    return true;
}

void WipeTowerTiming::flushed(double end_time, double next_interval)
{
    auto& site = m_sites.back();
    site.duration = end_time - site.time;
    m_dynamic_interval |= next_interval != site.interval;
    if (m_waiting)
        m_max_deferred_lateness = std::max(m_max_deferred_lateness, (site.time - m_periodic_origin) - site.interval);
    m_periodic_origin = end_time;
    m_waiting = false;
}

std::optional<std::vector<float>> WipeTowerTiming::predict(const Layout& baseline, const Layout& next,
                                                          const Print& print) const
{
    if (m_dynamic_interval || baseline.size() != m_changes.size() || next.size() != baseline.size() || baseline.empty())
        return {};
    std::vector<double> offsets;
    offsets.reserve(next.size());
    double travel_delta = 0.;
    size_t first_changed = next.size();
    for (size_t i = 0; i < next.size(); ++i) {
        const Change& change = m_changes[i];
        const auto& a = baseline[i];
        const auto& b = next[i];
        if (change.id != i || a.id != i || b.id != i || a.old_tool != int(change.old_tool) ||
            a.new_tool != int(change.new_tool) || b.old_tool != a.old_tool || b.new_tool != a.new_tool)
            return {};
        if (first_changed == next.size() && (b.tower_time != a.tower_time || b.entry != a.entry || b.exit != a.exit))
            first_changed = i;
        const double speed = print.config().travel_speed.get_at(get_physical_nozzle_index(print.config(), change.old_tool));
        if (!(speed > 0.))
            return {};
        if (change.last_model) {
            travel_delta += ((b.entry - *change.last_model).norm() - (a.entry - *change.last_model).norm()) / speed;
            if (i > 0)
                travel_delta += ((next[i-1].exit - *change.first_model).norm() -
                                 (baseline[i-1].exit - *change.first_model).norm()) / speed;
        } else if (i > 0) {
            travel_delta += ((b.entry - next[i-1].exit).norm() - (a.entry - baseline[i-1].exit).norm()) / speed;
        }
        const double offset = b.tower_time - a.tower_time + travel_delta;
        if (!std::isfinite(offset) || (i > 0 && (change.time < m_changes[i-1].time ||
            change.time + offset < m_changes[i-1].time + offsets.back())))
            return {};
        offsets.push_back(offset);
    }

    // Locate deadlines within the measured model spans. A shifted flush uses
    // a duration observed for the same kind of operation and tool. Its actual
    // script and route are checked by normal G-code generation before publication.
    auto offset_at = [&](double time) {
        auto it = std::lower_bound(m_changes.begin(), m_changes.end(), time,
            [](const Change& c, double t) { return c.time < t; });
        const size_t i = size_t(it - m_changes.begin());
        if (it == m_changes.end()) return offsets.back();
        const double start = i == 0 ? 0. : m_changes[i-1].time;
        const double before = i == 0 ? 0. : offsets[i-1];
        return it->time > start ? before + (offsets[i] - before) * (time - start) / (it->time - start) : offsets[i];
    };

    std::map<std::pair<SiteKind, unsigned>, double> durations;
    for (const auto& site : m_sites)
        if (site.duration)
            durations[{site.kind, site.tool}] = *site.duration;

    WipeTowerIdlePurgeClock clock;
    std::vector<float> volumes(next.size());
    double periodic_origin = m_periodic_start + offset_at(m_periodic_start);
    double flush_delta = 0.;
    bool waiting = false;
    size_t site_idx = 0;
    // Replay the measured schedule verbatim up to the earliest changed tower
    // segment. Operations after the final tool change cannot affect tower volumes.
    for (size_t i = 0; i < next.size(); ++i) {
        while (site_idx < m_sites.size() && m_sites[site_idx].changes_before == i) {
            const auto& site = m_sites[site_idx++];
            if (i < first_changed) {
                if (site.duration) periodic_origin = site.time + *site.duration;
                continue;
            }
            const double now = site.time + offset_at(site.time) + flush_delta;
            const double old_duration = site.duration.value_or(0.);
            const double overdue = (now - periodic_origin) - site.interval;
            if (waiting && overdue > max_flush_delay) return {};
            bool due = overdue >= 0.;
            if (due && can_wait(site_idx - 1, site, overdue)) {
                waiting = true;
                due = false;
            }
            if (due) {
                const auto known = durations.find({site.kind, site.tool});
                if (known == durations.end()) return {};
                const double duration = site.duration.value_or(known->second);
                periodic_origin = now + duration;
                waiting = false;
                flush_delta += duration - old_duration;
            } else {
                flush_delta -= old_duration;
            }
        }
        const auto& c = m_changes[i];
        volumes[i] = clock.change(c.old_tool, c.new_tool, c.time + offsets[i] + flush_delta,
                                  c.minutes, c.minimum, c.maximum);
    }
    return volumes;
}
}
