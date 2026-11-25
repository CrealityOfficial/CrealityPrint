#ifndef slic3r_GUI_WipeTowerPreviewCache_hpp_
#define slic3r_GUI_WipeTowerPreviewCache_hpp_

#include "Jobs/Worker.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/WipeTowerPlacement.hpp"
#include "libslic3r/ClipperUtils.hpp"
#include "libslic3r/Geometry/ConvexHull.hpp"

#include <map>
#include <memory>
#include <stdexcept>
#include <utility>

namespace Slic3r::GUI {

// Only modifies the estimator snapshot. Actual purge amounts depend on elapsed print time.
inline bool reserve_idle_purge_volume(WipeTowerPreviewInput& input)
{
    auto& cfg = input.config;
    bool timed_purge = false;
    auto& minimums = cfg.filament_minimal_purge_on_wipe_tower;
    if (minimums.values.empty())
        return false;
    for (unsigned int filament : input.physical_filament_ids) {
        if (filament == 0 || filament > cfg.filament_diameter.size())
            continue;
        const size_t tool = filament - 1;
        const double volume = cfg.filament_pre_flush_tower_volume.get_at(tool);
        if (cfg.filament_pre_flush_tower_time.get_at(tool) > 0. && volume > minimums.get_at(tool)) {
            minimums.values.resize(cfg.filament_diameter.size(), minimums.get_at(0));
            minimums.values[tool] = volume;
            timed_purge = true;
        }
    }
    return timed_purge;
}

// Requests and publication run on the UI thread. Jobs only read their input
// snapshot. Replacing a slot releases its interest in any queued older job.
class WipeTowerPreviewCache
{
public:
    enum class Use { Scene, Shell, Placement, Validation };

    struct Result {
        std::shared_ptr<const WipeTowerPreview> preview;
        bool ready = false;
        std::exception_ptr error;
    };

    explicit WipeTowerPreviewCache(Worker& worker) : m_worker(worker) {}

    Result request(int plate_index, ObjectID plate_id, Use use, WipeTowerPreviewInput input)
    {
        auto& slot = m_slots[{plate_index, use}];
        if (input.empty()) {
            slot.reset();
            static const auto empty = std::make_shared<const WipeTowerPreview>();
            return {empty, true, {}};
        }
        // Pose is applied to the local mesh by the consumer.
        input.config.wipe_tower_x.values = {0.};
        input.config.wipe_tower_y.values = {0.};
        input.config.wipe_tower_rotation_angle.value = 0.;

        // Scene, placement and validation can share the exact same job. The
        // validator may have different effective filaments or layer heights.
        for (const auto& item : m_slots) {
            const auto& other = item.second;
            if (item.first.first == plate_index && other && other->plate_id == plate_id && other->matches(input)) {
                if (!other->result.ready && !other->result.preview && slot && slot->plate_id == plate_id)
                    other->result.preview = slot->result.preview;
                slot = other;
                return slot->result;
            }
        }

        auto entry = std::make_shared<Entry>(Entry{plate_id, std::move(input), {}});
        if (slot && slot->plate_id == plate_id)
            entry->result.preview = slot->result.preview; // Display only until ready.
        slot = entry;
        std::weak_ptr<Entry> target = entry;
        auto mesh = std::make_shared<WipeTowerPreview>();
        const bool queued = queue_job(m_worker, [target, mesh](Job::Ctl& ctl) {
            if (auto current = target.lock(); current && !ctl.was_canceled()) {
                *mesh = current->input.estimate();
                if (mesh->tower_mesh.empty())
                    throw std::runtime_error("Failed to generate the wipe tower.");
            }
        }, [target, mesh](bool canceled, std::exception_ptr& error) {
            if (auto current = target.lock(); current && !canceled) {
                current->result = {mesh, true, error};
                current->changed = true;
            }
            error = nullptr; // The validator reports a failed calculation on the UI thread.
        });
        if (!queued)
            entry->result = {nullptr, true, std::make_exception_ptr(std::runtime_error("Could not queue the wipe tower preview."))};
        return entry->result;
    }

    void clear() { m_slots.clear(); m_worker.cancel_all(); }
    bool take_changed()
    {
        bool changed = false;
        for (auto& slot : m_slots)
            if (slot.second)
                changed |= std::exchange(slot.second->changed, false);
        return changed;
    }

private:
    struct Entry {
        ObjectID plate_id;
        WipeTowerPreviewInput input;
        Result result;
        bool changed = false;

        bool matches(const WipeTowerPreviewInput& other) const
        {
            return input.filaments_count == other.filaments_count && input.height == other.height &&
                   input.layer_height == other.layer_height && input.physical_filament_ids == other.physical_filament_ids &&
                   input.config == other.config;
        }
    };
    Worker& m_worker;
    std::map<std::pair<int, Use>, std::shared_ptr<Entry>> m_slots;
};

// This one-shot search only runs for a project opened with an out-of-bed tower.
// The UI owns the input snapshot and only publishes a result that still matches it.
class LoadedTowerPlacement
{
public:
    std::shared_ptr<const WipeTowerPreview> preview;
    Polygon bed;
    Polygons obstacles;
    bool allow_rotation = false;
    bool ready = false;
    std::optional<WipeTowerPose> pose;

    static void schedule(Worker& worker, const std::shared_ptr<LoadedTowerPlacement>& request,
                         WipeTowerPreviewInput input, WipeTowerPose saved)
    {
        std::weak_ptr<LoadedTowerPlacement> target = request;
        auto result = std::make_shared<std::optional<WipeTowerPose>>();
        bool queued = queue_job(worker, [target, input = std::move(input), saved, result](Job::Ctl& ctl) mutable {
            auto work = target.lock();
            if (!work || ctl.was_canceled())
                return;
            auto outline = [](const WipeTowerPreview& preview) {
                Points points = preview.tower_mesh.convex_hull().points;
                append(points, preview.brim_mesh.convex_hull().points);
                return Geometry::convex_hull(std::move(points));
            };
            const Polygon nominal = outline(*work->preview);
            Polygon reserved = nominal;
            // Timed purges can enlarge the tower during G-code export. Budget
            // their configured volume only for the placement calculation.
            const bool timed_purge = reserve_idle_purge_volume(input);
            if (timed_purge) {
                auto larger = outline(input.estimate());
                append(larger.points, nominal.points);
                reserved = Geometry::convex_hull(std::move(larger.points));
            }
            Polygon placed = reserved;
            placed.rotate(Geometry::deg2rad(saved.rotation_degrees));
            placed.translate(saved.anchor);
            // The quick rectangular bounds may cross an edge even though the
            // actual brim fits (for example, on a round bed).
            if (!reserved.empty() && diff(placed, work->bed).empty()) {
                *result = saved;
                return;
            }
            auto canceled = [&] { return ctl.was_canceled() || work.use_count() == 1; };
            if (canceled())
                return;
            *result = find_loaded_wipe_tower_pose(reserved, work->bed, work->obstacles, saved, work->allow_rotation, canceled);
            if (!*result && timed_purge && !canceled())
                // If even the reserved tower cannot fit at any allowed angle,
                // keep the saved orientation for a movable nominal preview.
                *result = find_loaded_wipe_tower_pose(nominal, work->bed, work->obstacles, saved, false, canceled);
        }, [target, result](bool canceled, std::exception_ptr& error) {
            if (auto work = target.lock()) {
                if (!canceled && !error)
                    work->pose = *result;
                work->ready = true;
            }
            error = nullptr;
        });
        if (!queued)
            request->ready = true;
    }
};

} // namespace Slic3r::GUI
#endif
