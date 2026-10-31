#include "FillLayerFilamentWipePacking.hpp"

#include "../../ClipperUtils.hpp"
#include "../../Color.hpp"
#include "../Fill.hpp"
#include "../../ExtrusionEntity.hpp"
#include "../../ExtrusionEntityCollection.hpp"
#include "../../Print.hpp"
#include "../../FDM/NoWipeTowerMaterialChange.hpp"
#include "../../GCode/ToolOrdering.hpp"
#include "../../Layer.hpp"
#include "../../PrintConfig.hpp"
#include "../../Surface.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include <boost/log/trivial.hpp>

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/task_arena.h>

namespace Slic3r {
namespace FillTower {

namespace {

constexpr float k_max_packing_density = 100.f;
constexpr int k_max_layer_planning_concurrency = 4;
constexpr size_t k_exact_global_packing_item_limit = 20;
constexpr size_t k_exact_global_assignment_state_limit = 200000;

double area_mm2(const ExPolygon& expolygon)
{
    return std::abs(unscale_(unscale_(area(ExPolygons{expolygon}))));
}

float filament_grayscale(const PrintConfig& print_config, unsigned int extruder)
{
    if (extruder >= print_config.filament_colour.values.size())
        return 0.5f;

    ColorRGB color;
    if (!decode_color(print_config.filament_colour.values[extruder], color))
        return 0.5f;

    // Relative luminance keeps black/white separation strong while retaining
    // a useful ordering for colored filaments.
    return std::clamp(0.2126f * color.r() + 0.7152f * color.g() + 0.0722f * color.b(), 0.f, 1.f);
}

double surface_height_mm(const LayerRegion& layer_region, const Surface& surface)
{
    const Layer* layer = layer_region.layer();
    if (layer == nullptr)
        return 0.;

    double height = surface.thickness > EPSILON ? surface.thickness : layer->height;
    if (surface.is_internal_bridge()) {
        const bool thick_bridge = layer->object()->config().thick_internal_bridges.value;
        const Flow bridge_flow  = layer_region.bridging_flow(frSolidInfill, thick_bridge);
        if (bridge_flow.spacing() > EPSILON)
            height = bridge_flow.mm3_per_mm() / bridge_flow.spacing();
    }
    return std::max(0., height);
}

bool is_layer_filament_wipe_packing_skeleton(const ExtrusionEntity* entity)
{
    if (entity == nullptr || entity->role() != erInternalInfill)
        return false;
    if (const auto* path = dynamic_cast<const ExtrusionPath*>(entity); path != nullptr)
        return path->is_locked_zag_skeleton();
    if (const auto* multipath = dynamic_cast<const ExtrusionMultiPath*>(entity); multipath != nullptr)
        return !multipath->paths.empty() && std::all_of(multipath->paths.begin(), multipath->paths.end(),
            [](const ExtrusionPath& path) { return path.is_locked_zag_skeleton(); });
    if (const auto* loop = dynamic_cast<const ExtrusionLoop*>(entity); loop != nullptr)
        return !loop->paths.empty() && std::all_of(loop->paths.begin(), loop->paths.end(),
            [](const ExtrusionPath& path) { return path.is_locked_zag_skeleton(); });
    if (const auto* collection = dynamic_cast<const ExtrusionEntityCollection*>(entity); collection != nullptr)
        return !collection->entities.empty() && std::all_of(collection->entities.begin(), collection->entities.end(),
            [](const ExtrusionEntity* child) { return is_layer_filament_wipe_packing_skeleton(child); });
    return false;
}

struct PackingBin
{
    const PrintObject*  object          = nullptr;
    const LayerRegion*  layer_region    = nullptr;
    const Surface*      surface         = nullptr;
    unsigned int        extruder        = (unsigned int)-1;
    size_t              object_id       = 0;
    size_t              layer_id        = 0;
    size_t              region_index    = 0;
    size_t              surface_index   = 0;
    float               base_density    = 0.f;
    float               target_density  = 0.f;
    float               skin_depth_mm   = 0.f;
    double              volume_100_mm3  = 0.;
    double              assigned_base_mm3  = 0.;
    double              assigned_extra_mm3 = 0.;
    bool                used_for_wipe    = false;
    bool                keep_for_retry   = false;
    double              measured_capacity_mm3 = 0.;
    bool                has_measured_capacity = false;

    double base_capacity_mm3() const
    {
        return std::max(0., volume_100_mm3 * double(base_density) * 0.01);
    }

    double base_remaining_mm3() const
    {
        return std::max(0., base_capacity_mm3() - assigned_base_mm3);
    }

    double extra_remaining_mm3() const
    {
        return std::max(0., volume_100_mm3 * double(k_max_packing_density - target_density) * 0.01);
    }

    double total_remaining_mm3() const
    {
        return base_remaining_mm3() + extra_remaining_mm3();
    }

    double assigned_total_mm3() const
    {
        return assigned_base_mm3 + assigned_extra_mm3;
    }

    double current_trajectory_capacity_mm3() const
    {
        return has_measured_capacity ? std::max(0., measured_capacity_mm3) : base_capacity_mm3();
    }
};

unsigned int bin_host_extruder(const PackingBin& bin)
{
    if (bin.layer_region != nullptr) {
        const int wall_filament = bin.layer_region->region().config().wall_filament.value;
        if (wall_filament > 0)
            return static_cast<unsigned int>(wall_filament - 1);
    }
    return bin.extruder;
}

float bin_grayscale(const PackingBin& bin)
{
    if (bin.object == nullptr || bin.object->print() == nullptr)
        return 0.5f;

    // The path extruder may already have been reassigned by cross-region
    // packing.  Use the region's wall filament as the host color.
    return filament_grayscale(bin.object->print()->config(), bin_host_extruder(bin));
}

bool has_fixed_protection_skin(const PackingBin& bin)
{
    // The UI/host-color rule is authoritative for every no-tower skin pattern,
    // including zero: transition matrix depths must not recreate disabled skin.
    return bin.object != nullptr && bin.layer_region != nullptr &&
        no_wipe_tower_no_purge_mode_enabled(*bin.object->print());
}

float wipe_protection_skin_depth_mm(const PackingBin& bin)
{
    if (bin.layer_region == nullptr)
        return 0.f;

    if (has_fixed_protection_skin(bin))
        return no_wipe_tower_skin_infill_depth(bin.object->print()->config(), bin.layer_region->region().config());
    // A configured zero means no protective skin, not an automatic nozzle-based depth.
    return std::max(0.f, static_cast<float>(bin.layer_region->region().config().skin_infill_depth));
}

float transition_skin_depth_from_matrix_mm(const PrintConfig& print_config, unsigned int old_extruder, unsigned int new_extruder)
{
    const std::vector<double>& matrix = print_config.transmittance_matrix.values;
    const size_t matrix_size = matrix.size();
    const size_t filament_count = static_cast<size_t>(
        std::lround(std::sqrt(static_cast<double>(matrix_size))));
    if (matrix_size == 0 || filament_count * filament_count != matrix_size ||
        old_extruder >= filament_count || new_extruder >= filament_count ||
        old_extruder == new_extruder)
        return 0.f;

    const double depth_mm = matrix[size_t(old_extruder) * filament_count + new_extruder];
    return std::isfinite(depth_mm) && depth_mm > 0. ? float(depth_mm) : 0.f;
}

struct PackingItem
{
    LayerFilamentWipePackingTransition transition;
    float                              volume_mm3     = 0.f;
    float                              skin_depth_mm = 0.f;
    size_t                             order          = 0;
};

struct PackingPlate
{
    std::vector<PackingBin*> bins;

    double base_capacity_mm3() const
    {
        double capacity = 0.;
        for (const PackingBin* bin : bins)
            capacity += bin->base_capacity_mm3();
        return capacity;
    }

    double current_trajectory_capacity_mm3() const
    {
        double capacity = 0.;
        for (const PackingBin* bin : bins)
            capacity += bin->current_trajectory_capacity_mm3();
        return capacity;
    }

    double capacity_at_density(float density) const
    {
        double capacity = 0.;
        for (const PackingBin* bin : bins) {
            const double effective_density = std::max(double(bin->base_density), double(density));
            capacity += bin->volume_100_mm3 * effective_density * 0.01;
        }
        return capacity;
    }
};

struct PackingSolution
{
    std::vector<std::vector<const PackingItem*>> items_by_plate;
    std::vector<const PackingItem*>              unplaced;
    double                                       packed_volume_mm3 = 0.;
    size_t                                       packed_count      = 0;

    bool all_placed() const { return unplaced.empty(); }
};

double items_total_volume(const std::vector<const PackingItem*>& items)
{
    double total_volume = 0.;
    for (const PackingItem* item : items)
        total_volume += item->volume_mm3;
    return total_volume;
}

float density_needed_for_plate_volume(const PackingPlate& plate, double volume_mm3)
{
    if (volume_mm3 <= plate.base_capacity_mm3() + EPSILON)
        return 0.f;

    // The binary search must never probe below the configured skeleton
    // density floor of any bin on this plate.
    float low = 0.f;
    for (const PackingBin* bin : plate.bins)
        if (bin != nullptr)
            low = std::max(low, bin->base_density);
    float high = k_max_packing_density;
    for (size_t iter = 0; iter < 18; ++iter) {
        const float mid = 0.5f * (low + high);
        if (plate.capacity_at_density(mid) + EPSILON >= volume_mm3)
            high = mid;
        else
            low = mid;
    }
    return high;
}

std::vector<size_t> solution_assignment_signature(const PackingSolution& solution)
{
    size_t max_order = 0;
    for (const std::vector<const PackingItem*>& plate_items : solution.items_by_plate)
        for (const PackingItem* item : plate_items)
            max_order = std::max(max_order, item->order);

    std::vector<size_t> signature(max_order + 1, std::numeric_limits<size_t>::max());
    for (size_t plate_idx = 0; plate_idx < solution.items_by_plate.size(); ++plate_idx)
        for (const PackingItem* item : solution.items_by_plate[plate_idx])
            signature[item->order] = plate_idx;
    return signature;
}

double solution_density_cost(const PackingSolution& solution, const std::vector<PackingPlate>& plates)
{
    double cost = 0.;
    for (size_t plate_idx = 0; plate_idx < solution.items_by_plate.size(); ++plate_idx) {
        double assigned_volume = 0.;
        for (const PackingItem* item : solution.items_by_plate[plate_idx])
            assigned_volume += item->volume_mm3;
        if (assigned_volume <= EPSILON)
            continue;

        const float target_density = density_needed_for_plate_volume(plates[plate_idx], assigned_volume);
        cost += std::max(0., plates[plate_idx].capacity_at_density(target_density) -
                             plates[plate_idx].base_capacity_mm3());
    }
    return cost;
}

float solution_max_target_density(const PackingSolution& solution, const std::vector<PackingPlate>& plates)
{
    float max_density = 0.f;
    for (size_t plate_idx = 0; plate_idx < solution.items_by_plate.size(); ++plate_idx) {
        double assigned_volume = 0.;
        for (const PackingItem* item : solution.items_by_plate[plate_idx])
            assigned_volume += item->volume_mm3;
        if (assigned_volume > EPSILON)
            max_density = std::max(max_density, density_needed_for_plate_volume(plates[plate_idx], assigned_volume));
    }
    return max_density;
}

size_t solution_used_plate_count(const PackingSolution& solution)
{
    size_t used = 0;
    for (const std::vector<const PackingItem*>& plate_items : solution.items_by_plate)
        if (!plate_items.empty())
            ++used;
    return used;
}

bool is_better_full_solution(const PackingSolution& candidate, const PackingSolution& best,
                             const std::vector<PackingPlate>& plates, bool has_best)
{
    if (!candidate.all_placed())
        return false;
    if (!has_best)
        return true;

    const float candidate_max_density = solution_max_target_density(candidate, plates);
    const float best_max_density = solution_max_target_density(best, plates);
    if (candidate_max_density < best_max_density - EPSILON)
        return true;
    if (candidate_max_density > best_max_density + EPSILON)
        return false;

    const double candidate_cost = solution_density_cost(candidate, plates);
    const double best_cost = solution_density_cost(best, plates);
    if (candidate_cost < best_cost - EPSILON)
        return true;
    if (candidate_cost > best_cost + EPSILON)
        return false;

    const size_t candidate_used = solution_used_plate_count(candidate);
    const size_t best_used = solution_used_plate_count(best);
    if (candidate_used != best_used)
        return candidate_used < best_used;

    return solution_assignment_signature(candidate) < solution_assignment_signature(best);
}

void sort_solution_items_by_order(PackingSolution& solution)
{
    for (std::vector<const PackingItem*>& plate_items : solution.items_by_plate)
        std::stable_sort(plate_items.begin(), plate_items.end(), [](const PackingItem* lhs, const PackingItem* rhs) {
            return lhs->order < rhs->order;
        });
}

PackingSolution make_solution_from_assignment(const std::vector<const PackingItem*>&              items,
                                              std::vector<std::vector<const PackingItem*>>&&      assignment)
{
    PackingSolution solution;
    solution.items_by_plate = std::move(assignment);
    sort_solution_items_by_order(solution);

    std::set<size_t> placed_orders;
    for (const std::vector<const PackingItem*>& plate_items : solution.items_by_plate) {
        for (const PackingItem* item : plate_items) {
            solution.packed_volume_mm3 += item->volume_mm3;
            ++solution.packed_count;
            placed_orders.insert(item->order);
        }
    }

    for (const PackingItem* item : items)
        if (placed_orders.find(item->order) == placed_orders.end())
            solution.unplaced.emplace_back(item);

    return solution;
}

bool try_assign_all_items_to_plates(const std::vector<const PackingItem*>& items,
                                    const std::vector<PackingPlate>&       plates,
                                    const std::vector<double>&             plate_capacities,
                                    std::vector<std::vector<const PackingItem*>>& assignment)
{
    assignment.assign(plate_capacities.size(), {});
    if (items.empty())
        return true;
    if (plate_capacities.empty())
        return false;

    double total_capacity = 0.;
    double max_capacity = 0.;
    for (double capacity : plate_capacities) {
        total_capacity += capacity;
        max_capacity = std::max(max_capacity, capacity);
    }
    if (items_total_volume(items) > total_capacity + EPSILON)
        return false;
    for (const PackingItem* item : items)
        if (item->volume_mm3 > max_capacity + EPSILON)
            return false;

    std::vector<const PackingItem*> ordered = items;
    std::stable_sort(ordered.begin(), ordered.end(), [](const PackingItem* lhs, const PackingItem* rhs) {
        if (std::abs(lhs->volume_mm3 - rhs->volume_mm3) > EPSILON)
            return lhs->volume_mm3 > rhs->volume_mm3;
        return lhs->order < rhs->order;
    });

    std::vector<double> remaining_capacity = plate_capacities;
    std::vector<std::vector<const PackingItem*>> working_assignment(plate_capacities.size());
    std::vector<double> suffix_volume(ordered.size() + 1, 0.);
    for (size_t i = ordered.size(); i > 0; --i)
        suffix_volume[i - 1] = suffix_volume[i] + ordered[i - 1]->volume_mm3;

    PackingSolution best_solution;
    bool has_best = false;
    bool aborted = false;
    size_t visited_states = 0;

    auto search = [&](auto&& self, size_t item_idx) -> void {
        if (aborted)
            return;
        if (++visited_states > k_exact_global_assignment_state_limit) {
            aborted = true;
            return;
        }

        if (item_idx == ordered.size()) {
            std::vector<std::vector<const PackingItem*>> candidate_assignment = working_assignment;
            PackingSolution candidate = make_solution_from_assignment(items, std::move(candidate_assignment));
            if (is_better_full_solution(candidate, best_solution, plates, has_best)) {
                best_solution = std::move(candidate);
                has_best = true;
            }
            return;
        }

        double remaining_total_capacity = 0.;
        double remaining_max_capacity = 0.;
        for (double capacity : remaining_capacity) {
            remaining_total_capacity += capacity;
            remaining_max_capacity = std::max(remaining_max_capacity, capacity);
        }
        if (suffix_volume[item_idx] > remaining_total_capacity + EPSILON)
            return;

        const PackingItem* item = ordered[item_idx];
        if (item->volume_mm3 > remaining_max_capacity + EPSILON)
            return;

        for (size_t plate_idx = 0; plate_idx < remaining_capacity.size(); ++plate_idx) {
            if (remaining_capacity[plate_idx] + EPSILON < item->volume_mm3)
                continue;

            remaining_capacity[plate_idx] -= item->volume_mm3;
            working_assignment[plate_idx].emplace_back(item);
            self(self, item_idx + 1);
            working_assignment[plate_idx].pop_back();
            remaining_capacity[plate_idx] += item->volume_mm3;
        }
    };

    search(search, 0);
    if (aborted || !has_best)
        return false;

    assignment = std::move(best_solution.items_by_plate);
    return true;
}
void sort_bins(std::vector<PackingBin*>& bins)
{
    std::stable_sort(bins.begin(), bins.end(), [](const PackingBin* lhs, const PackingBin* rhs) {
        const double lhs_capacity = lhs->current_trajectory_capacity_mm3();
        const double rhs_capacity = rhs->current_trajectory_capacity_mm3();
        if (std::abs(lhs_capacity - rhs_capacity) > EPSILON)
            return lhs_capacity > rhs_capacity;
        if (lhs->object_id != rhs->object_id)
            return lhs->object_id < rhs->object_id;
        if (lhs->layer_id != rhs->layer_id)
            return lhs->layer_id < rhs->layer_id;
        if (lhs->region_index != rhs->region_index)
            return lhs->region_index < rhs->region_index;
        return lhs->surface_index < rhs->surface_index;
    });
}

std::vector<const PackingItem*> choose_packable_items(const std::vector<const PackingItem*>& items, double capacity_mm3)
{
    std::vector<const PackingItem*> selected;
    if (items.empty() || capacity_mm3 <= EPSILON)
        return selected;

    double total_volume = 0.;
    for (const PackingItem* item : items)
        total_volume += item->volume_mm3;
    if (total_volume <= capacity_mm3 + EPSILON) {
        selected.reserve(items.size());
        for (const PackingItem* item : items)
            selected.emplace_back(item);
        return selected;
    }

    // A partially covered transition still triggers the external flush penalty.
    // Select complete transitions first by count, then keep the required volume
    // as low as possible. With one capacity, lower volume means lower density.
    if (items.size() <= 20) {
        const size_t masks = size_t(1) << items.size();
        size_t best_mask = 0;
        double best_volume = std::numeric_limits<double>::max();
        size_t best_count = 0;
        for (size_t mask = 1; mask < masks; ++mask) {
            double volume = 0.;
            size_t count = 0;
            for (size_t i = 0; i < items.size(); ++i) {
                if ((mask & (size_t(1) << i)) == 0)
                    continue;
                volume += items[i]->volume_mm3;
                ++count;
            }
            if (volume > capacity_mm3 + EPSILON)
                continue;
            if (count > best_count ||
                (count == best_count && volume < best_volume - EPSILON)) {
                best_volume = volume;
                best_count  = count;
                best_mask   = mask;
            }
        }
        for (size_t i = 0; i < items.size(); ++i)
            if ((best_mask & (size_t(1) << i)) != 0)
                selected.emplace_back(items[i]);
        return selected;
    }

    std::vector<const PackingItem*> ordered;
    ordered.reserve(items.size());
    for (const PackingItem* item : items)
        ordered.emplace_back(item);
    std::stable_sort(ordered.begin(), ordered.end(), [](const PackingItem* lhs, const PackingItem* rhs) {
        if (std::abs(lhs->volume_mm3 - rhs->volume_mm3) > EPSILON)
            return lhs->volume_mm3 < rhs->volume_mm3;
        return lhs->order < rhs->order;
    });

    double used = 0.;
    for (const PackingItem* item : ordered) {
        if (used + item->volume_mm3 <= capacity_mm3 + EPSILON) {
            selected.emplace_back(item);
            used += item->volume_mm3;
        }
    }
    std::stable_sort(selected.begin(), selected.end(), [](const PackingItem* lhs, const PackingItem* rhs) {
        return lhs->order < rhs->order;
    });
    return selected;
}

PackingSolution pack_items_into_plates_greedy(const std::vector<const PackingItem*>& items,
                                              const std::vector<PackingPlate>&       plates,
                                              const std::vector<double>&             plate_capacities)
{
    PackingSolution solution;
    solution.items_by_plate.resize(plates.size());
    solution.unplaced = items;

    for (size_t plate_idx = 0; plate_idx < plates.size() && !solution.unplaced.empty(); ++plate_idx) {
        if (plate_idx >= plate_capacities.size() || plate_capacities[plate_idx] <= EPSILON)
            continue;

        const std::vector<const PackingItem*> selected =
            choose_packable_items(solution.unplaced, plate_capacities[plate_idx]);
        if (selected.empty())
            continue;

        std::set<size_t> selected_orders;
        for (const PackingItem* item : selected) {
            solution.items_by_plate[plate_idx].emplace_back(item);
            solution.packed_volume_mm3 += item->volume_mm3;
            ++solution.packed_count;
            selected_orders.insert(item->order);
        }

        std::vector<const PackingItem*> next_unplaced;
        next_unplaced.reserve(solution.unplaced.size() - selected.size());
        for (const PackingItem* item : solution.unplaced)
            if (selected_orders.find(item->order) == selected_orders.end())
                next_unplaced.emplace_back(item);
        solution.unplaced = std::move(next_unplaced);
    }

    return solution;
}

PackingSolution pack_items_into_plates(const std::vector<const PackingItem*>& items,
                                       const std::vector<PackingPlate>&       plates,
                                       const std::vector<double>&             plate_capacities)
{
    PackingSolution solution;
    solution.items_by_plate.resize(plates.size());
    solution.unplaced = items;

    if (items.empty() || plates.empty())
        return solution;

    if (items.size() <= k_exact_global_packing_item_limit) {
        std::vector<std::vector<const PackingItem*>> assignment;
        if (try_assign_all_items_to_plates(items, plates, plate_capacities, assignment))
            return make_solution_from_assignment(items, std::move(assignment));
    }

    return pack_items_into_plates_greedy(items, plates, plate_capacities);
}

PackingSolution solve_items_at_current_capacity(const std::vector<const PackingItem*>& items,
                                                const std::vector<PackingPlate>&       plates)
{
    std::vector<double> plate_capacities;
    plate_capacities.reserve(plates.size());
    for (const PackingPlate& plate : plates)
        plate_capacities.emplace_back(plate.current_trajectory_capacity_mm3());
    return pack_items_into_plates(items, plates, plate_capacities);
}

PackingSolution solve_items_by_expanding_plates(const std::vector<const PackingItem*>& items,
                                                const std::vector<PackingPlate>&       plates)
{
    std::vector<double> plate_capacities;
    plate_capacities.reserve(plates.size());
    for (const PackingPlate& plate : plates)
        plate_capacities.emplace_back(plate.base_capacity_mm3());

    PackingSolution last_solution = pack_items_into_plates(items, plates, plate_capacities);
    PackingSolution best_all_solution;
    bool has_best_all_solution = false;
    if (is_better_full_solution(last_solution, best_all_solution, plates, has_best_all_solution)) {
        best_all_solution = last_solution;
        has_best_all_solution = true;
    }

    for (size_t plate_idx = 0; plate_idx < plates.size(); ++plate_idx) {
        plate_capacities[plate_idx] = plates[plate_idx].capacity_at_density(k_max_packing_density);
        PackingSolution candidate = pack_items_into_plates(items, plates, plate_capacities);
        if (is_better_full_solution(candidate, best_all_solution, plates, has_best_all_solution)) {
            best_all_solution = candidate;
            has_best_all_solution = true;
        }
        last_solution = std::move(candidate);
    }

    return has_best_all_solution ? best_all_solution : last_solution;
}

void record_transition_take(LayerFilamentWipePackingPlan& plan, const PackingItem& item, const PackingBin& bin, double volume_mm3)
{
    if (bin.layer_region == nullptr || volume_mm3 <= EPSILON)
        return;

    plan.transition_region_volumes[item.transition][bin.layer_region][bin.extruder] += float(volume_mm3);
}

double consume_plate_capacity(std::vector<PackingBin*>& bins, const PackingItem& item, float target_density,
                              bool use_actual_trajectory_capacity, LayerFilamentWipePackingPlan& plan)
{
    double remaining = item.volume_mm3;

    auto available_capacity = [&](const PackingBin& bin) {
        const double capacity = use_actual_trajectory_capacity ?
                                    bin.current_trajectory_capacity_mm3() :
                                    bin.volume_100_mm3 * std::max(double(bin.base_density), double(target_density)) * 0.01;
        return std::max(0., capacity - bin.assigned_total_mm3());
    };

    // Group the logical backpack by the host filament first. The physical
    // paths remain separate bins, but same-color regions are accumulated before
    // a transition is allowed to spill into another color.
    std::map<unsigned int, std::vector<PackingBin*>> bins_by_host;
    for (PackingBin* bin : bins)
        if (bin != nullptr)
            bins_by_host[bin_host_extruder(*bin)].push_back(bin);

    std::vector<PackingBin*> ordered_bins;
    unsigned int selected_host = (unsigned int)-1;
    double selected_excess = std::numeric_limits<double>::max();
    for (const auto& entry : bins_by_host) {
        double group_capacity = 0.;
        for (PackingBin* bin : entry.second) {
            group_capacity += available_capacity(*bin);
        }
        if (group_capacity + EPSILON < remaining)
            continue;
        const double excess = group_capacity - remaining;
        if (excess < selected_excess - EPSILON) {
            selected_host = entry.first;
            selected_excess = excess;
        }
    }
    if (selected_host != (unsigned int)-1)
        ordered_bins = bins_by_host[selected_host];
    else
        ordered_bins = bins;

    // No grayscale preference: retain capacity and stable host order.
    for (PackingBin* bin : ordered_bins) {
        if (remaining <= EPSILON)
            break;
        if (bin == nullptr)
            continue;

        const double take = std::min(remaining, available_capacity(*bin));
        if (take <= EPSILON)
            continue;

        if (use_actual_trajectory_capacity) {
            // Measured trajectories are already printed. Keep their capacity
            // separate from the density target used for the next rebuild.
            bin->assigned_base_mm3 += take;
        } else {
            const double base_take = std::min(take, bin->base_remaining_mm3());
            const double extra_take = take - base_take;
            bin->assigned_base_mm3 += base_take;
            bin->assigned_extra_mm3 += extra_take;
            if (extra_take > EPSILON)
                bin->target_density = std::max(bin->target_density, target_density);
        }
        bin->used_for_wipe = true;
        const float protection_depth_mm = !has_fixed_protection_skin(*bin) && item.skin_depth_mm > EPSILON ?
                                              item.skin_depth_mm : wipe_protection_skin_depth_mm(*bin);
        bin->skin_depth_mm = has_fixed_protection_skin(*bin) ? protection_depth_mm :
                             std::max(bin->skin_depth_mm, protection_depth_mm);
        record_transition_take(plan, item, *bin, take);
        remaining -= take;
    }
    return remaining;
}


PackingSolution choose_best_packable_subset_for_plates(const std::vector<const PackingItem*>& items,
                                                       const std::vector<PackingPlate>&       plates,
                                                       bool allow_density_expansion)
{
    PackingSolution best;
    best.unplaced = items;
    if (items.empty() || plates.empty())
        return best;

    if (items.size() <= 20) {
        const size_t masks = size_t(1) << items.size();
        double best_volume = std::numeric_limits<double>::max();
        double best_density_cost = std::numeric_limits<double>::max();
        float best_target_density = std::numeric_limits<float>::max();
        size_t best_count = 0;

        // The order is intentional: eliminate the most external flushes first,
        // then minimize the density needed by that many complete transitions.
        for (size_t mask = 1; mask < masks; ++mask) {
            std::vector<const PackingItem*> subset;
            subset.reserve(items.size());
            for (size_t i = 0; i < items.size(); ++i)
                if ((mask & (size_t(1) << i)) != 0)
                    subset.emplace_back(items[i]);

            PackingSolution candidate = allow_density_expansion ?
                                                solve_items_by_expanding_plates(subset, plates) :
                                                solve_items_at_current_capacity(subset, plates);
            if (!candidate.all_placed())
                continue;

            const float target_density = allow_density_expansion ?
                solution_max_target_density(candidate, plates) : 0.f;
            const double density_cost = allow_density_expansion ?
                solution_density_cost(candidate, plates) : 0.;
            if (candidate.packed_count > best_count ||
                (candidate.packed_count == best_count &&
                 target_density < best_target_density - EPSILON) ||
                (candidate.packed_count == best_count &&
                 std::abs(target_density - best_target_density) <= EPSILON &&
                 density_cost < best_density_cost - EPSILON) ||
                (candidate.packed_count == best_count &&
                 std::abs(target_density - best_target_density) <= EPSILON &&
                 std::abs(density_cost - best_density_cost) <= EPSILON &&
                 candidate.packed_volume_mm3 < best_volume - EPSILON) ||
                (candidate.packed_count == best_count &&
                 std::abs(target_density - best_target_density) <= EPSILON &&
                 std::abs(density_cost - best_density_cost) <= EPSILON &&
                 std::abs(candidate.packed_volume_mm3 - best_volume) <= EPSILON &&
                 solution_assignment_signature(candidate) < solution_assignment_signature(best))) {
                best = std::move(candidate);
                best_volume = best.packed_volume_mm3;
                best_density_cost = density_cost;
                best_target_density = target_density;
                best_count = best.packed_count;
            }
        }

        return best;
    }

    std::vector<const PackingItem*> remaining = items;
    std::stable_sort(remaining.begin(), remaining.end(), [](const PackingItem* lhs, const PackingItem* rhs) {
        if (std::abs(lhs->volume_mm3 - rhs->volume_mm3) > EPSILON)
            return lhs->volume_mm3 < rhs->volume_mm3;
        return lhs->order < rhs->order;
    });
    while (!remaining.empty()) {
        PackingSolution candidate = allow_density_expansion ?
                                            solve_items_by_expanding_plates(remaining, plates) :
                                            solve_items_at_current_capacity(remaining, plates);
        if (candidate.all_placed())
            return candidate;
        remaining.pop_back();
    }
    return best;
}

void apply_solution_to_plan(const PackingSolution& solution, const std::vector<PackingPlate>& plates,
                            bool use_actual_trajectory_capacity, LayerFilamentWipePackingPlan& plan)
{
    for (size_t plate_idx = 0; plate_idx < solution.items_by_plate.size(); ++plate_idx) {
        const std::vector<const PackingItem*>& plate_items = solution.items_by_plate[plate_idx];
        if (plate_items.empty())
            continue;

        double assigned_volume = 0.;
        for (const PackingItem* item : plate_items)
            assigned_volume += item->volume_mm3;

        std::vector<PackingBin*> bins = plates[plate_idx].bins;
        sort_bins(bins);

        const float target_density = use_actual_trajectory_capacity ? 0.f :
            density_needed_for_plate_volume(plates[plate_idx], assigned_volume);
        for (const PackingItem* item : plate_items) {
            struct BinState {
                PackingBin* bin;
                double      assigned_base_mm3;
                double      assigned_extra_mm3;
                float       target_density;
                float       skin_depth_mm;
                bool        used_for_wipe;
            };
            std::vector<BinState> previous_states;
            previous_states.reserve(bins.size());
            for (PackingBin* bin : bins)
                previous_states.push_back(BinState{bin, bin->assigned_base_mm3, bin->assigned_extra_mm3,
                                                   bin->target_density, bin->skin_depth_mm, bin->used_for_wipe});

            const double remaining = consume_plate_capacity(bins, *item, target_density,
                                                             use_actual_trajectory_capacity, plan);
            const double consumed = item->volume_mm3 - remaining;
            if (remaining <= EPSILON || (use_actual_trajectory_capacity && consumed > EPSILON)) {
                plan.infill_transitions.insert(item->transition);
            } else {
                // Density-expanding plans keep transitions atomic. Fixed-capacity
                // plans commit any usable prefix in the branch above.
                for (const BinState& state : previous_states) {
                    state.bin->assigned_base_mm3  = state.assigned_base_mm3;
                    state.bin->assigned_extra_mm3 = state.assigned_extra_mm3;
                    state.bin->target_density     = state.target_density;
                    state.bin->skin_depth_mm      = state.skin_depth_mm;
                    state.bin->used_for_wipe      = state.used_for_wipe;
                }
                plan.transition_region_volumes.erase(item->transition);
            }
        }
    }
}

} // namespace

LayerFilamentWipePackingTransition layer_filament_wipe_packing_transition_key(
    coordf_t print_z, unsigned int old_extruder, unsigned int new_extruder)
{
    return LayerFilamentWipePackingTransition{scaled<coord_t>(print_z), old_extruder, new_extruder};
}

LayerFilamentWipePackingPlan plan_layer_filament_wipe_packing(
    const Print& print, const ToolOrdering& tool_ordering,
    const std::set<LayerFilamentWipePackingTransition>& blocked_transitions,
    bool use_actual_trajectory_capacity,
    bool perform_density_search)
{

    LayerFilamentWipePackingPlan plan;
    BOOST_LOG_TRIVIAL(warning) << "[NO_WIPE_TRACE] planner begin density_search=" << perform_density_search
                               << " actual_capacity=" << use_actual_trajectory_capacity;
    if (tool_ordering.empty())
        return plan;

    if (!perform_density_search && no_wipe_tower_no_purge_mode_enabled(print)) {
        // Include regions without bins or transitions as explicit zeroes too.
        // Only a positive final assignment below may restore protective skin.
        for (const PrintObject* object : print.objects())
            for (const Layer* layer : object->layers())
                for (const LayerRegion* layer_region : layer->regions())
                    for (const Surface& surface : layer_region->fill_surfaces.surfaces)
                        if (surface.surface_type == stInternal)
                            plan.skin_depths[&surface] = 0.f;
    }

    std::vector<PackingBin> bins;
    std::map<coord_t, std::vector<size_t>> bins_by_layer;
    std::map<coord_t, std::vector<std::pair<const Layer*, size_t>>> simulation_layers_by_layer;
    std::set<const Layer*> simulation_layers;
    std::set<const LayerRegion*> simulation_regions;

    auto surface_density = [&](const LayerRegion& layer_region, const Surface& surface) {
        const PrintRegionConfig& region_config = layer_region.region().config();
        const float configured_density = configured_skeleton_infill_density_percent(print.config(), region_config);
        if (const float* planned_density =
                print.layer_filament_wipe_packing_sparse_infill_density(layer_region, surface)) {
            // Planned values may increase the skeleton density, but never lower
            // it below the user's skeleton setting.
            return std::min(k_max_packing_density,
                            std::max(configured_density, *planned_density));
        }
        return configured_density;
    };

    auto layer_region_density = [&](const LayerRegion& layer_region) {
        float density = 0.f;
        for (const Surface& surface : layer_region.fill_surfaces.surfaces)
            if (surface.surface_type == stInternal)
                density = std::max(density, surface_density(layer_region, surface));
        return density;
    };

    for (const PrintObject* object : print.objects()) {
        const size_t object_id = object->id().id;
        const size_t instance_count = std::max<size_t>(size_t(1), object->instances().size());
        for (const Layer* layer : object->layers()) {
            if (layer == nullptr || layer->id() == 0)
                continue;

            const LayerTools& layer_tools = tool_ordering.tools_for_layer(layer->print_z);
            const coord_t layer_key = scaled<coord_t>(layer_tools.print_z);
            for (size_t region_index = 0; region_index < layer->regions().size(); ++region_index) {
                const LayerRegion* layer_region = layer->regions()[region_index];
                const PrintRegionConfig& region_config = layer_region->region().config();
                const bool packing_mode =
                    no_wipe_tower_no_purge_mode_enabled(print) ||
                    flush_into_skeleton_packing_mode_enabled(print);
                const float skeleton_density = configured_skeleton_infill_density_percent(print.config(), region_config);
                if ((!packing_mode && region_config.sparse_infill_filament.value == 0) ||
                    (packing_mode && skeleton_density <= 0.f) ||
                    (!packing_mode && region_config.sparse_infill_density.value <= 0))
                    continue;

                const unsigned int extruder = region_config.sparse_infill_filament.value == 0 ?
                    0u : layer_tools.sparse_infill_filament(layer_region->region());
                if (!layer_tools.has_extruder(extruder))
                    continue;

                if (use_actual_trajectory_capacity) {
                    simulation_regions.insert(layer_region);
                    if (simulation_layers.insert(layer).second)
                        simulation_layers_by_layer[layer_key].emplace_back(layer, instance_count);
                }

                if (use_actual_trajectory_capacity) {
                    const float current_density = layer_region_density(*layer_region);
                    const float configured_density = configured_skeleton_infill_density_percent(print.config(), region_config);

                    std::map<unsigned int, double> actual_volume_by_extruder;
                    for (const ExtrusionEntity* entity : layer_region->fills.entities) {
                        const auto* fill = dynamic_cast<const ExtrusionEntityCollection*>(entity);
                        if (!is_layer_filament_wipe_packing_skeleton(fill))
                            continue;

                        const unsigned int fill_extruder = layer_tools.extruder(*fill, layer_region->region());
                        if (!layer_tools.has_extruder(fill_extruder))
                            continue;

                        actual_volume_by_extruder[fill_extruder] +=
                            skeleton_wipe_volume(*fill, 0.f) * double(instance_count);
                    }

                    auto add_actual_bin = [&](unsigned int fill_extruder, double actual_volume_mm3) {
                        bins_by_layer[layer_key].emplace_back(bins.size());
                        bins.push_back(PackingBin{
                            object,
                            layer_region,
                            nullptr,
                            fill_extruder,
                            object_id,
                            layer->id(),
                            region_index,
                            0,
                            configured_density,
                            current_density,
                            0.f,
                            0.,
                            0.,
                            0.,
                            false,
                            false,
                            actual_volume_mm3,
                            true
                        });
                    };

                    bool added_actual_bin = false;
                    for (const auto& actual_volume : actual_volume_by_extruder) {
                        if (actual_volume.second <= EPSILON)
                            continue;
                        add_actual_bin(actual_volume.first, actual_volume.second);
                        added_actual_bin = true;
                    }
                    if (!added_actual_bin)
                        add_actual_bin(extruder, 0.);
                    continue;
                }

                for (size_t surface_index = 0; surface_index < layer_region->fill_surfaces.surfaces.size(); ++surface_index) {
                    const Surface& surface = layer_region->fill_surfaces.surfaces[surface_index];
                    if (surface.surface_type != stInternal || surface.expolygon.empty())
                        continue;

                    const double v100 = area_mm2(surface.expolygon) * surface_height_mm(*layer_region, surface) *
                                        double(instance_count);
                    if (v100 <= EPSILON)
                        continue;

                    const float base_density = configured_skeleton_infill_density_percent(print.config(), region_config);

                    bins_by_layer[layer_key].emplace_back(bins.size());
                    bins.push_back(PackingBin{
                        object,
                        layer_region,
                        &surface,
                        extruder,
                        object_id,
                        layer->id(),
                        region_index,
                        surface_index,
                        base_density,
                        base_density,
                        0.f,
                        v100,
                        0.
                    });
                }
            }
        }
    }

    struct LayerPackingWork {
        coord_t                              layer_key = 0;
        std::vector<PackingItem>             items;
        std::vector<size_t>                  bin_indices;
        LayerFilamentWipePackingPlan         plan;
    };

    // Build transitions serially because the first transition of a layer
    // depends on the last extruder of the preceding layer.
    std::vector<LayerPackingWork> layer_work;
    unsigned int previous_last_extruder = (unsigned int)-1;
    size_t transition_order = 0;
    for (auto layer_tools_it = tool_ordering.begin(); layer_tools_it != tool_ordering.end(); ++layer_tools_it) {
        const LayerTools& layer_tools = *layer_tools_it;
        if (layer_tools.extruders.empty())
            continue;

        LayerPackingWork work;
        const bool force_external_fill_flush =
            flush_into_skeleton_force_external_fill_flush(print, layer_tools.print_z);
        auto add_transition = [&](unsigned int old_extruder, unsigned int new_extruder) {
            if (force_external_fill_flush || old_extruder == (unsigned int)-1 ||
                new_extruder == (unsigned int)-1 || old_extruder == new_extruder)
                return;
            const float required_volume = flush_volume_from_matrix(print.config(),
                                                                   old_extruder,
                                                                   new_extruder);
            const bool no_wipe_mode = no_wipe_tower_no_purge_mode_enabled(print);
            const float mandatory_tower_volume = no_wipe_mode ? 0.f : std::max(
                float(print.config().prime_volume.value),
                float(print.config().filament_minimal_purge_on_wipe_tower.get_at(new_extruder)));
            // Preserve the printer preference; it only takes effect with an enabled tower.
            const bool purge_in_tower = print.config().enable_prime_tower.value &&
                                        print.config().purge_in_prime_tower.value;
            const float volume = purge_in_tower ?
                                     0.f : std::max(required_volume - mandatory_tower_volume, 0.f);
            if (volume <= EPSILON)
                return;

            LayerFilamentWipePackingTransition transition =
                layer_filament_wipe_packing_transition_key(layer_tools.print_z, old_extruder, new_extruder);
            if (blocked_transitions.find(transition) != blocked_transitions.end())
                return;

            const float skin_depth_mm = transition_skin_depth_from_matrix_mm(
                print.config(), old_extruder, new_extruder);
            work.items.push_back(PackingItem{transition, volume, skin_depth_mm, transition_order++});
        };

        add_transition(previous_last_extruder, layer_tools.extruders.front());
        for (size_t extruder_idx = 0; extruder_idx + 1 < layer_tools.extruders.size(); ++extruder_idx)
            add_transition(layer_tools.extruders[extruder_idx], layer_tools.extruders[extruder_idx + 1]);
        previous_last_extruder = layer_tools.extruders.back();

        if (work.items.empty())
            continue;

        work.layer_key = scaled<coord_t>(layer_tools.print_z);
        auto layer_bins_it = bins_by_layer.find(work.layer_key);
        if (layer_bins_it != bins_by_layer.end())
            work.bin_indices = layer_bins_it->second;
        layer_work.emplace_back(std::move(work));
    }

    if (use_actual_trajectory_capacity && perform_density_search && no_wipe_tower_no_purge_mode_enabled(print)) {
        // Each region starts at its structural floor. Skin retains its fixed
        // protection thickness and never contributes to purge capacity.
        // Solid/bridge surfaces are never virtualized into spendable capacity.
        for (const PackingBin& bin : bins) {
            if (bin.layer_region == nullptr)
                continue;
            for (const Surface& surface : bin.layer_region->fill_surfaces.surfaces) {
                if (surface.surface_type != stInternal)
                    continue;
                plan.sparse_infill_densities[&surface] = bin.base_density;
                if (has_fixed_protection_skin(bin))
                    plan.skin_depths[&surface] = wipe_protection_skin_depth_mm(bin);
            }
        }

        auto optimize_layer = [&](LayerPackingWork& work) {
            struct Candidate {
                float density;
                double volume;
            };
            struct Region {
                const PackingBin* bin = nullptr;
                size_t copies = 1;
                Candidate maximum{0.f, 0.};
                Candidate selected{0.f, 0.};
            };
            std::vector<Region> regions;
            std::set<const LayerRegion*> seen;
            std::map<const Layer*, std::unique_ptr<LockedZagSkeletonSimulation>> simulations;
            for (size_t idx : work.bin_indices) {
                const PackingBin& bin = bins[idx];
                if (bin.layer_region == nullptr || !seen.insert(bin.layer_region).second)
                    continue;
                const auto& surfaces = bin.layer_region->fill_surfaces.surfaces;
                if (std::none_of(surfaces.begin(), surfaces.end(), [](const Surface& surface) {
                        return surface.surface_type == stInternal;
                    }))
                    continue;
                const Layer* layer = bin.layer_region->layer();
                if (simulations.find(layer) == simulations.end())
                    simulations.emplace(layer, std::make_unique<LockedZagSkeletonSimulation>(*layer, false));
                Region region;
                region.bin = &bin;
                region.copies = std::max<size_t>(1, bin.object->instances().size());
                regions.emplace_back(std::move(region));
            }
            auto measure = [&](const Region& region, float density) {
                if (print.canceled())
                    throw CanceledException();
                const LayerRegion* layer_region = region.bin->layer_region;
                const auto metrics = simulations.at(layer_region->layer())->simulate(density, layer_region);
                const auto found = metrics.find(layer_region);
                if (found == metrics.end())
                    return 0.;
                return found->second.extrusion_volume_mm3 * double(region.copies);
            };
            double capacity = 0.;
            for (Region& region : regions) {
                const float base = region.bin->base_density;
                region.selected = Candidate{base, measure(region, base)};

                capacity += region.selected.volume;
            }
            double required = 0.;
            for (const PackingItem& item : work.items)
                required += item.volume_mm3;

            if (capacity + EPSILON < required) {
                // Find the required density with endpoint measurements and
                // bisection. The former 0.5% scan plus 0.01% refinement could
                // trigger roughly 300 geometry simulations per region.
                constexpr int density_binary_iterations = 8;
                double maximum_capacity = capacity;
                for (Region& region : regions) {
                    region.maximum = Candidate{k_max_packing_density,
                                               measure(region, k_max_packing_density)};
                    maximum_capacity += std::max(0., region.maximum.volume - region.selected.volume);
                }

                // If the requested transition volume exceeds the actual
                // maximum, only the available portion can be assigned to the
                // skeleton; the remainder will use the normal external path.
                required = std::min(required, maximum_capacity);
                while (capacity + EPSILON < required) {
                    const double deficit = required - capacity;
                    Region* chosen = nullptr;
                    Candidate next{0.f, 0.};
                    double best_gain = 0.;
                    bool chosen_completes = false;

                    for (Region& region : regions) {
                        const double maximum_gain = region.maximum.volume - region.selected.volume;
                        if (maximum_gain <= EPSILON)
                            continue;

                        Candidate candidate = region.maximum;
                        const bool can_complete = maximum_gain + EPSILON >= deficit;
                        if (can_complete) {
                            // Find the smallest density increase that covers
                            // the remaining deficit for this region.
                            float low = region.selected.density;
                            float high = region.maximum.density;
                            for (int iteration = 0; iteration < density_binary_iterations; ++iteration) {
                                const float mid = 0.5f * (low + high);
                                const double volume = measure(region, mid);
                                if (volume - region.selected.volume + EPSILON >= deficit) {
                                    high = mid;
                                    candidate = Candidate{mid, volume};
                                } else {
                                    low = mid;
                                }
                            }
                        }

                        const double gain = candidate.volume - region.selected.volume;
                        if (gain <= EPSILON)
                            continue;
                        const bool completes = gain + EPSILON >= deficit;
                        if (chosen == nullptr ||
                            (completes && !chosen_completes) ||
                            (completes == chosen_completes &&
                             (completes ? gain < best_gain - EPSILON : gain > best_gain + EPSILON))) {
                            chosen = &region;
                            next = candidate;
                            best_gain = gain;
                            chosen_completes = completes;
                        }
                    }

                    if (chosen == nullptr)
                        break;
                    capacity += next.volume - chosen->selected.volume;
                    chosen->selected = next;
                }
            }
            for (const Region& region : regions) {
                const PackingBin& bin = *region.bin;
                for (const Surface& surface : bin.layer_region->fill_surfaces.surfaces) {
                    if (surface.surface_type != stInternal)
                        continue;
                    work.plan.sparse_infill_densities[&surface] = region.selected.density;
                    work.plan.skin_depths[&surface] = wipe_protection_skin_depth_mm(bin);
                }
            }
        };
        const int concurrency = int(std::min(layer_work.size(), size_t(k_max_layer_planning_concurrency)));
        if (concurrency > 1) {
            tbb::task_arena arena(concurrency);
            arena.execute([&] {
                tbb::parallel_for(tbb::blocked_range<size_t>(0, layer_work.size(), 1),
                    [&](const tbb::blocked_range<size_t>& range) {
                        for (size_t idx = range.begin(); idx < range.end(); ++idx)
                            optimize_layer(layer_work[idx]);
                    });
            });
        } else {
            for (LayerPackingWork& work : layer_work)
                optimize_layer(work);
        }
        for (const LayerPackingWork& work : layer_work) {
            for (const auto& entry : work.plan.sparse_infill_densities)
                plan.sparse_infill_densities[entry.first] = entry.second;
            for (const auto& entry : work.plan.skin_depths)
                plan.skin_depths[entry.first] = entry.second;
        }
        return plan;
    }

    std::map<const Layer*, std::shared_ptr<const LockedZagSkeletonSimulation>> simulation_contexts;
    if (use_actual_trajectory_capacity && perform_density_search) {
        for (const Layer* simulation_layer : simulation_layers) {
            if (simulation_layer == nullptr)
                continue;
            simulation_contexts.emplace(
                simulation_layer,
                std::make_shared<const LockedZagSkeletonSimulation>(*simulation_layer, true));
        }
    }

    std::map<std::pair<coord_t, float>, LockedZagSkeletonMetrics> planning_capacity_cache;
    auto planning_layer_capacity = [&](coord_t layer_key, float density) -> const LockedZagSkeletonMetrics& {
        const std::pair<coord_t, float> cache_key{layer_key, density};
        auto cached = planning_capacity_cache.find(cache_key);
        if (cached != planning_capacity_cache.end())
            return cached->second;

        LockedZagSkeletonMetrics total;
        auto layers_it = simulation_layers_by_layer.find(layer_key);
        if (layers_it != simulation_layers_by_layer.end()) {
            for (const auto& layer_entry : layers_it->second) {
                const auto context_it = simulation_contexts.find(layer_entry.first);
                if (context_it == simulation_contexts.end() || !context_it->second)
                    continue;
                const LockedZagSkeletonMetricsByRegion metrics_by_region =
                    context_it->second->simulate(density);
                for (const auto& region_metrics : metrics_by_region) {
                    if (simulation_regions.find(region_metrics.first) == simulation_regions.end())
                        continue;
                    total.trajectory_length_mm +=
                        region_metrics.second.trajectory_length_mm *
                        double(layer_entry.second);
                    total.extrusion_volume_mm3 +=
                        region_metrics.second.extrusion_volume_mm3 *
                        double(layer_entry.second);
                }
            }
        }
        return planning_capacity_cache.emplace(cache_key, total).first->second;
    };

    bool layer_work_is_independent = true;
    std::set<coord_t> claimed_layer_keys;
    std::set<size_t> claimed_bin_indices;
    for (const LayerPackingWork& work : layer_work) {
        if (!claimed_layer_keys.insert(work.layer_key).second)
            layer_work_is_independent = false;
        for (size_t bin_idx : work.bin_indices)
            if (!claimed_bin_indices.insert(bin_idx).second)
                layer_work_is_independent = false;
    }

    auto solve_layer = [&](LayerPackingWork& work, LayerFilamentWipePackingPlan& layer_plan) {
        const std::vector<PackingItem>& layer_items = work.items;
        std::vector<PackingBin*> layer_bins;
        layer_bins.reserve(work.bin_indices.size());
        for (size_t bin_idx : work.bin_indices) {
            PackingBin& bin = bins[bin_idx];
            layer_bins.emplace_back(&bin);
        }

        if (layer_bins.empty()) {
            if (use_actual_trajectory_capacity && !perform_density_search)
                for (const PackingItem& item : layer_items)
                    layer_plan.external_flush_transitions.insert(item.transition);
            return;
        }

        // Hidden sparse skeleton is a distributed wipe tower. Its original
        // filament does not own the capacity: a transition may consume paths
        // across objects, regions and colors as one shared layer pool.
        PackingPlate shared_plate;
        shared_plate.bins = layer_bins;
        sort_bins(shared_plate.bins);
        std::vector<PackingPlate> plates;
        plates.emplace_back(std::move(shared_plate));

        std::vector<const PackingItem*> items;
        items.reserve(layer_items.size());
        for (const PackingItem& item : layer_items)
            items.emplace_back(&item);

        // Density-expanding planning keeps transitions atomic. Actual-trajectory
        // packing below may consume a prefix and leave its tail on the tower.
        PackingSolution solution;

        if (!use_actual_trajectory_capacity) {
            solution = solve_items_by_expanding_plates(items, plates);
            if (!solution.all_placed())
                solution = choose_best_packable_subset_for_plates(items, plates, true);
            apply_solution_to_plan(solution, plates, false, layer_plan);
            return;
        }

        if (perform_density_search) {
            // Select density independently for this layer work item. A dense
            // layer must not raise the skeleton density of other layers.
            float density_floor = k_max_packing_density;
            for (const PackingBin* bin : layer_bins) {
                if (bin != nullptr)
                    density_floor = std::min(density_floor, bin->base_density);
            }

            auto evaluate_density = [&](float density) {
                const LockedZagSkeletonMetrics& metrics =
                    planning_layer_capacity(work.layer_key, density);
                std::vector<double> capacities(plates.size(), 0.);
                capacities.front() = metrics.extrusion_volume_mm3;
                return pack_items_into_plates(items, plates, capacities);
            };

            float selected_density = density_floor;
            const PackingSolution default_solution = evaluate_density(density_floor);
            if (!default_solution.all_placed() &&
                density_floor < k_max_packing_density - EPSILON) {
                const PackingSolution max_solution = evaluate_density(k_max_packing_density);
                const size_t target_packed_count = max_solution.packed_count;
                if (target_packed_count > default_solution.packed_count) {
                    float low = density_floor;
                    float high = k_max_packing_density;
                    for (size_t iteration = 0; iteration < 18; ++iteration) {
                        const float mid = 0.5f * (low + high);
                        if (evaluate_density(mid).packed_count >= target_packed_count)
                            high = mid;
                        else
                            low = mid;
                    }
                    selected_density = high;
                }
            }

            for (PackingBin* bin : layer_bins) {
                if (bin == nullptr)
                    continue;
                bin->keep_for_retry = true;
                bin->target_density = std::max(bin->base_density, selected_density);
                bin->skin_depth_mm = std::max(bin->skin_depth_mm, wipe_protection_skin_depth_mm(*bin));
            }
            return;
        }
        // Use the UI-density trajectories as a fixed shared pool. Assign
        // transitions in print order and allow the final one to use a partial
        // prefix; G-code leaves the unassigned tail on the wipe tower.
        solution.items_by_plate.resize(1);
        solution.items_by_plate.front() = items;
        apply_solution_to_plan(solution, plates, true, layer_plan);
        for (const PackingItem& item : layer_items)
            if (layer_plan.infill_transitions.find(item.transition) == layer_plan.infill_transitions.end())
                layer_plan.external_flush_transitions.insert(item.transition);
        return;
    };

    const bool run_layer_planning_in_parallel =
        use_actual_trajectory_capacity && layer_work.size() > 1 && layer_work_is_independent;
    if (run_layer_planning_in_parallel) {
        const int concurrency = static_cast<int>(
            std::min(layer_work.size(), static_cast<size_t>(k_max_layer_planning_concurrency)));
        tbb::task_arena planning_arena(concurrency);
        planning_arena.execute([&]() {
            tbb::parallel_for(
                tbb::blocked_range<size_t>(0, layer_work.size(), 1),
                [&](const tbb::blocked_range<size_t>& range) {
                    for (size_t work_idx = range.begin(); work_idx < range.end(); ++work_idx)
                        solve_layer(layer_work[work_idx], layer_work[work_idx].plan);
                });
        });

        // Merge in original layer order to keep deterministic map/set updates.
        for (const LayerPackingWork& work : layer_work) {
            const LayerFilamentWipePackingPlan& layer_plan = work.plan;
            for (const auto& entry : layer_plan.sparse_infill_densities)
                plan.sparse_infill_densities[entry.first] = entry.second;
            for (const auto& entry : layer_plan.skin_depths)
                plan.skin_depths[entry.first] = entry.second;
            plan.plain_infill_surfaces.insert(
                layer_plan.plain_infill_surfaces.begin(), layer_plan.plain_infill_surfaces.end());
            plan.infill_transitions.insert(
                layer_plan.infill_transitions.begin(), layer_plan.infill_transitions.end());
            plan.external_flush_transitions.insert(
                layer_plan.external_flush_transitions.begin(), layer_plan.external_flush_transitions.end());

            for (const auto& transition_entry : layer_plan.transition_region_volumes) {
                LayerFilamentWipePackingTransitionRegionVolumes& target_regions =
                    plan.transition_region_volumes[transition_entry.first];
                for (const auto& region_entry : transition_entry.second) {
                    std::map<unsigned int, float>& target_extruders = target_regions[region_entry.first];
                    for (const auto& extruder_entry : region_entry.second)
                        target_extruders[extruder_entry.first] += extruder_entry.second;
                }
            }
        }
    } else {
        // Preserve the original serial behavior for non-simulation passes or
        // unexpected overlapping layer work.
        for (LayerPackingWork& work : layer_work)
            solve_layer(work, plan);
    }


    auto final_skin_depth = [&](const PackingBin& bin) {
        if (!has_fixed_protection_skin(bin))
            return bin.skin_depth_mm;
        // Base-density paths can receive purge too. Density growth and retry
        // flags are not evidence that a region actually received an assignment.
        return bin.assigned_total_mm3() > EPSILON ? wipe_protection_skin_depth_mm(bin) : 0.f;
    };

    auto set_bin_surfaces_density = [&](const PackingBin& bin) {
        if (bin.surface != nullptr) {
            plan.sparse_infill_densities[bin.surface] = bin.target_density;
            plan.skin_depths[bin.surface] = final_skin_depth(bin);
            return;
        }

        if (bin.layer_region == nullptr)
            return;

        for (const Surface& surface : bin.layer_region->fill_surfaces.surfaces) {
            if (surface.surface_type != stInternal)
                continue;
            plan.sparse_infill_densities[&surface] = bin.target_density;
            plan.skin_depths[&surface] = final_skin_depth(bin);
        }
    };

    auto force_bin_surfaces_plain = [&](const PackingBin& bin) {
        if (bin.surface != nullptr) {
            plan.plain_infill_surfaces.insert(bin.surface);
            return;
        }

        if (bin.layer_region == nullptr)
            return;

        for (const Surface& surface : bin.layer_region->fill_surfaces.surfaces)
            if (surface.surface_type == stInternal)
                plan.plain_infill_surfaces.insert(&surface);
    };

    std::map<const LayerRegion*, std::vector<const PackingBin*>> actual_bins_by_region;
    for (const PackingBin& bin : bins) {
        if (bin.surface == nullptr && bin.layer_region != nullptr) {
            actual_bins_by_region[bin.layer_region].emplace_back(&bin);
        } else if (bin.used_for_wipe || bin.keep_for_retry) {
            set_bin_surfaces_density(bin);
        } else {
            force_bin_surfaces_plain(bin);
        }
    }

    // Actual trajectories may be split into several extruder groups while all
    // groups still originate from the same fill surfaces. Resolve the surface
    // state once per region: one unused group must not turn the surfaces plain
    // when another group uses them for wiping or needs a density retry.
    for (const auto& region_entry : actual_bins_by_region) {
        const LayerRegion* layer_region = region_entry.first;
        const std::vector<const PackingBin*>& region_bins = region_entry.second;
        const bool keep_skeleton = std::any_of(region_bins.begin(), region_bins.end(), [](const PackingBin* bin) {
            return bin != nullptr && (bin->used_for_wipe || bin->keep_for_retry);
        });

        if (!keep_skeleton) {
            force_bin_surfaces_plain(*region_bins.front());
            continue;
        }

        float target_density = 0.f;
        float skin_depth_mm = 0.f;
        for (const PackingBin* bin : region_bins) {
            if (bin == nullptr)
                continue;
            target_density = std::max(target_density, bin->target_density);
            skin_depth_mm = std::max(skin_depth_mm, final_skin_depth(*bin));
        }
        for (const Surface& surface : layer_region->fill_surfaces.surfaces) {
            if (surface.surface_type != stInternal)
                continue;
            plan.sparse_infill_densities[&surface] = target_density;
            plan.skin_depths[&surface] = skin_depth_mm;
        }
    }

    BOOST_LOG_TRIVIAL(warning) << "[NO_WIPE_TRACE] planner end bins=" << bins.size()
                               << " works=" << layer_work.size()
                               << " infill_transitions=" << plan.infill_transitions.size()
                               << " external_transitions=" << plan.external_flush_transitions.size()
                               << " region_volumes=" << plan.transition_region_volumes.size()
                               << " densities=" << plan.sparse_infill_densities.size();
    return plan;
}

} // namespace FillTower
} // namespace Slic3r
