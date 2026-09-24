#include "NoWipeTowerMaterialChange.hpp"

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
    if (!flush_into_skeleton_packing_mode_enabled(print))
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

static NoWipeTowerMaterialChangeEntrance entrance_from_entity(const ExtrusionEntity& first_entity, double scaled_distance)
{
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

    const std::vector<std::pair<size_t, bool>> chain = chain_extrusion_entities(candidates, &start_near);
    if (chain.empty())
        return {};

    std::unique_ptr<ExtrusionEntity> first_entity(candidates[chain.front().first]->clone());
    if (chain.front().second)
        first_entity->reverse();

    if (const auto* collection = dynamic_cast<const ExtrusionEntityCollection*>(first_entity.get())) {
        ExtrusionEntityCollection chained = ExtrusionEntityCollection::chained_path_from(collection->entities, start_near);
        if (chained.entities.empty())
            return {};
        return entrance_from_entity(*chained.entities.front(), scaled_distance);
    }

    return entrance_from_entity(*first_entity, scaled_distance);
}

} // namespace Slic3r
