#include "NoWipeTowerMaterialChange.hpp"

#include "../Color.hpp"
#include "../ExtrusionEntityCollection.hpp"
#include "../Print.hpp"
#include "../Layer.hpp"
#include "../PrintConfig.hpp"
#include "../ShortestPath.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <set>

namespace Slic3r {

float no_wipe_tower_skin_infill_depth(float configured_depth_mm, float grayscale)
{
    return grayscale > 0.85f ? std::max(0.f, configured_depth_mm) : 0.f;
}

float no_wipe_tower_skin_infill_depth(const PrintConfig& print_config, const PrintRegionConfig& region_config)
{
    const float configured_depth_mm = std::max(0.f, float(region_config.skin_infill_depth.value));
    if (configured_depth_mm == 0.f)
        return 0.f;

    // Always use the model's wall color, not an infill tool reassigned for purge.
    const int wall_filament = region_config.wall_filament.value;
    if (wall_filament <= 0 || size_t(wall_filament) > print_config.filament_colour.values.size())
        return 0.f;
    ColorRGB color;
    if (!decode_color(print_config.filament_colour.values[size_t(wall_filament - 1)], color))
        return 0.f;

    const float grayscale = std::clamp(0.2126f * color.r() + 0.7152f * color.g() + 0.0722f * color.b(), 0.f, 1.f);
    return no_wipe_tower_skin_infill_depth(configured_depth_mm, grayscale);
}

static size_t no_wipe_tower_no_purge_extruder_count(const Print& print)
{
    const auto& config = print.config();
    if (config.enable_prime_tower.value ||
        config.flush_into_solid_skeleton.value ||
        config.print_sequence.value != PrintSequence::ByLayer ||
        config.spiral_mode.value ||
        print.has_prime_volume_solid_skeleton())
        return 0;

    const std::set<unsigned int> physical_extruders = print.used_physical_extruders();
    if (physical_extruders.empty())
        return 0;

    for (unsigned int filament_id : physical_extruders) {
        if (filament_id == 0 ||
            config.filament_soluble.get_at(filament_id - 1) ||
            config.filament_is_support.get_at(filament_id - 1))
            return 0;
    }

    for (const PrintObject* object : print.objects()) {
        const PrintObjectConfig& object_config = object->config();
        if (object_config.flush_into_objects.value ||
            object_config.flush_into_infill.value ||
            object_config.flush_into_support.value ||
            object_config.flush_into_skeleton.value)
            return 0;
    }

    return physical_extruders.size();
}

bool no_wipe_tower_no_purge_mode_enabled(const Print& print)
{
    return no_wipe_tower_no_purge_extruder_count(print) > 1;
}

bool no_wipe_tower_single_filament_skin_disabled(const Print& print)
{
    return no_wipe_tower_no_purge_extruder_count(print) == 1;
}

bool flush_into_skeleton_packing_mode_enabled(const Print& print)
{
    const auto& config = print.config();
    if (!config.enable_prime_tower.value ||
        config.print_sequence.value != PrintSequence::ByLayer ||
        config.spiral_mode.value)
        return false;

    const std::set<unsigned int> physical_extruders = print.used_physical_extruders();
    if (physical_extruders.size() <= 1)
        return false;

    for (unsigned int filament_id : physical_extruders) {
        if (filament_id == 0 ||
            config.filament_soluble.get_at(filament_id - 1) ||
            config.filament_is_support.get_at(filament_id - 1))
            return false;
    }

    for (const PrintObject* object : print.objects())
        if (object != nullptr && object->config().flush_into_skeleton.value)
            return true;

    return false;
}

bool flush_into_skeleton_force_external_fill_flush(const Print& print, coordf_t print_z)
{
    // Reserve external-purge layers before either packing mode assigns model paths.
    if (print.config().enable_prime_tower.value &&
        !flush_into_skeleton_packing_mode_enabled(print))
        return false;

    const int bottom_flush_layers = print.config().bottom_fill_flush_layers.value;
    const int top_flush_layers = print.config().top_fill_flush_layers.value;
    if (bottom_flush_layers <= 0 && top_flush_layers <= 0)
        return false;

    auto non_negative_layer_count = [](int value) -> size_t {
        return value > 0 ? size_t(value) : size_t(0);
    };

    for (const PrintObject* object : print.objects()) {
        const Layer* layer = object != nullptr ? object->get_layer_at_printz(print_z, EPSILON) : nullptr;
        if (layer == nullptr)
            continue;

        const size_t layer_id = layer->id();
        const size_t layer_count = object->layer_count();
        if (layer_count == 0)
            continue;

        size_t bottom_shell_layers = non_negative_layer_count(print.default_region_config().bottom_shell_layers.value);
        size_t top_shell_layers = non_negative_layer_count(print.default_region_config().top_shell_layers.value);
        for (const LayerRegion* layer_region : layer->regions()) {
            if (layer_region == nullptr)
                continue;
            bottom_shell_layers = std::max(bottom_shell_layers,
                non_negative_layer_count(layer_region->region().config().bottom_shell_layers.value));
            top_shell_layers = std::max(top_shell_layers,
                non_negative_layer_count(layer_region->region().config().top_shell_layers.value));
        }

        const size_t fill_begin = std::min(bottom_shell_layers, layer_count);
        const size_t top_shell_begin = top_shell_layers >= layer_count ? size_t(0) : layer_count - top_shell_layers;
        if (top_shell_begin <= fill_begin)
            continue;
        const size_t fill_end = top_shell_begin;

        if (bottom_flush_layers > 0) {
            const size_t bottom_flush_end = std::min(fill_end, fill_begin + size_t(bottom_flush_layers));
            if (layer_id >= fill_begin && layer_id < bottom_flush_end)
                return true;
        }
        if (top_flush_layers > 0) {
            const size_t fill_layer_count = fill_end - fill_begin;
            const size_t top_flush_start = size_t(top_flush_layers) >= fill_layer_count ?
                fill_begin : fill_end - size_t(top_flush_layers);
            if (layer_id >= top_flush_start && layer_id < fill_end)
                return true;
        }
    }

    return false;
}

static bool supports_reverse_wipe(const ExtrusionEntity& entity)
{
    if (const auto* path = dynamic_cast<const ExtrusionPath*>(&entity))
        return !path->has_path3();

    const ExtrusionPaths* paths = nullptr;
    if (const auto* multipath = dynamic_cast<const ExtrusionMultiPath*>(&entity))
        paths = &multipath->paths;
    else if (const auto* loop = dynamic_cast<const ExtrusionLoop*>(&entity))
        paths = &loop->paths;
    if (paths == nullptr || paths->empty())
        return false;

    const ExtrusionPath* previous = nullptr;
    for (const ExtrusionPath& path : *paths) {
        if (path.has_path3() || path.empty() ||
            (previous != nullptr && previous->last_point() != path.first_point()))
            return false;
        previous = &path;
    }
    return true;
}

static NoWipeTowerMaterialChangeEntrance entrance_from_entity(const ExtrusionEntity& first_entity, double scaled_distance)
{
    if (const auto* collection = dynamic_cast<const ExtrusionEntityCollection*>(&first_entity)) {
        // Search only within the selected infill island. Do not count travel
        // between separate paths toward the requested distance.
        for (const ExtrusionEntity* entity : collection->entities) {
            NoWipeTowerMaterialChangeEntrance entrance = entrance_from_entity(*entity, scaled_distance);
            if (entrance.valid) {
                // Later paths supply the parking point and their own local
                // reverse wipe only. Retain the original normal print start;
                // the caller uses safe travel for the gap after wiping.
                entrance.start = collection->first_point();
                return entrance;
            }
        }
        return {};
    }

    NoWipeTowerMaterialChangeEntrance entrance;
    Points points;
    first_entity.collect_points(points);
    if (points.size() < 2 || scaled_distance <= EPSILON)
        return entrance;

    entrance.start = first_entity.first_point();
    double remaining_distance = scaled_distance;
    Point  previous           = points.front();

    for (size_t idx = 1; idx < points.size(); ++idx) {
        const Point& point = points[idx];
        if (point == previous)
            continue;

        const Vec2d segment        = (point - previous).cast<double>();
        const double segment_length = segment.norm();
        if (segment_length <= EPSILON) {
            previous = point;
            continue;
        }

        if (remaining_distance <= segment_length + EPSILON) {
            const double take = std::min(remaining_distance, segment_length);
            entrance.toolchange = (previous.cast<double>() + segment * (take / segment_length)).cast<coord_t>();
            entrance.valid      = entrance.toolchange != entrance.start;
            if (entrance.valid && supports_reverse_wipe(first_entity)) {
                // Retain every bend. A straight P-to-start shortcut could leave
                // the skeleton or cross a wall. Do not mutate the print path.
                for (size_t prefix_idx = 0; prefix_idx < idx; ++prefix_idx)
                    entrance.reverse_wipe.append(points[prefix_idx]);
                entrance.reverse_wipe.append(entrance.toolchange);
                entrance.reverse_wipe.reverse();
            }
            return entrance;
        }

        remaining_distance -= segment_length;
        previous = point;
    }

    return entrance;
}

NoWipeTowerMaterialChangeEntrance no_wipe_tower_material_change_entrance(
    ExtrusionEntitiesPtr candidates,
    const Point&         start_near,
    double               scaled_distance)
{
    if (candidates.empty() || scaled_distance <= EPSILON)
        return {};

    // Ordered skin is emitted after the core. Choose the entrance from that
    // first group too; nearest-neighbor chaining across both groups can otherwise
    // select a skin ring which will not be the first printed trajectory.
    const bool ordered_skin = std::any_of(candidates.begin(), candidates.end(),
        [](const ExtrusionEntity* entity) { return entity->role() == erSkinInfill && !entity->can_sort(); });
    const bool has_core = std::any_of(candidates.begin(), candidates.end(),
        [](const ExtrusionEntity* entity) { return entity->role() == erInternalInfill; });
    if (ordered_skin && has_core)
        candidates.erase(std::remove_if(candidates.begin(), candidates.end(),
            [](const ExtrusionEntity* entity) { return entity->role() == erSkinInfill; }), candidates.end());

    const std::vector<std::pair<size_t, bool>> chain = chain_extrusion_entities(candidates, &start_near);
    if (chain.empty())
        return {};

    std::unique_ptr<ExtrusionEntity> first_entity(candidates[chain.front().first]->clone());
    if (chain.front().second)
        first_entity->reverse();

    if (const auto* collection = dynamic_cast<const ExtrusionEntityCollection*>(first_entity.get())) {
        ExtrusionEntityCollection chained = collection->chained_path_from(start_near);
        if (chained.entities.empty())
            return {};
        return entrance_from_entity(chained, scaled_distance);
    }

    return entrance_from_entity(*first_entity, scaled_distance);
}

} // namespace Slic3r
