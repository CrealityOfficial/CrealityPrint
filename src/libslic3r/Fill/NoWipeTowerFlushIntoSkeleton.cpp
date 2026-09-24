#include "NoWipeTowerFlushIntoSkeleton.hpp"

#include "../ClipperUtils.hpp"
#include "../ExtrusionEntity.hpp"
#include "../Layer.hpp"
#include "../PrintConfig.hpp"
#include "../Surface.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace Slic3r {

namespace {

struct InternalSurface
{
    ExPolygon expolygon;
    double    height_mm = 0.;
};

struct NoWipeTowerFlushIntoSkeletonCandidate
{
    std::vector<InternalSurface> surfaces;
    double                       available_volume_mm3 = 0.;
};

double area_mm2(const ExPolygons& expolygons) { return std::abs(unscale_(unscale_(area(expolygons)))); }

double receiver_volume_mm3(const std::vector<InternalSurface>& surfaces)
{
    double volume_mm3 = 0.;
    for (const InternalSurface& surface : surfaces)
        volume_mm3 += area_mm2(ExPolygons{surface.expolygon}) * surface.height_mm;
    return volume_mm3;
}

std::optional<NoWipeTowerFlushIntoSkeletonCandidate> collect_no_wipe_tower_flush_into_skeleton_candidate(const LayerRegion& layer_region)
{
    if (layer_region.layer() == nullptr)
        return std::nullopt;

    const Layer* layer = layer_region.layer();
    const bool first_object_layer = layer->id() == 0 && std::abs(layer->bottom_z()) < EPSILON;
    if (first_object_layer)
        return std::nullopt;

    const double default_height = layer->height;
    if (default_height <= EPSILON)
        return std::nullopt;

    NoWipeTowerFlushIntoSkeletonCandidate candidate;
    for (const Surface& surface : layer_region.fill_surfaces.surfaces) {
        if (surface.surface_type != stInternal || surface.expolygon.empty())
            continue;

        double height = surface.thickness > EPSILON ? surface.thickness : default_height;
        if (surface.is_internal_bridge()) {
            const bool thick_bridge = layer_region.layer()->object()->config().thick_internal_bridges.value;
            const Flow bridge_flow  = layer_region.bridging_flow(frSolidInfill, thick_bridge);
            // A dense bridge deposits flow.mm3_per_mm() every spacing() millimetres.
            // Use that effective material volume per square millimetre instead of
            // ordinary layer height when deriving the skeleton area.
            if (bridge_flow.spacing() > EPSILON)
                height = bridge_flow.mm3_per_mm() / bridge_flow.spacing();
        }

        candidate.surfaces.push_back(InternalSurface{surface.expolygon, height});
    }

    if (candidate.surfaces.empty())
        return std::nullopt;

    candidate.available_volume_mm3 = receiver_volume_mm3(candidate.surfaces);
    if (candidate.available_volume_mm3 <= EPSILON)
        return std::nullopt;

    return candidate;
}

} // namespace

NoWipeTowerFlushIntoSkeletonEntrance no_wipe_tower_flush_into_skeleton_entrance(const ExtrusionEntity& first_entity, double scaled_distance)
{
    NoWipeTowerFlushIntoSkeletonEntrance entrance;
    Points points;
    first_entity.collect_points(points);
    if (points.size() < 2 || scaled_distance <= EPSILON)
        return entrance;

    entrance.start = first_entity.first_point();
    double remaining_distance = scaled_distance;
    double traveled_distance  = 0.;
    Point  previous           = points.front();
    for (size_t idx = 1; idx < points.size(); ++idx) {
        const Point& point = points[idx];
        if (point == previous)
            continue;

        const Vec2d segment         = (point - previous).cast<double>();
        const double segment_length = segment.norm();
        if (segment_length <= EPSILON) {
            previous = point;
            continue;
        }

        if (remaining_distance <= segment_length + EPSILON) {
            const double take = std::min(remaining_distance, segment_length);
            entrance.toolchange    = (previous.cast<double>() + segment * (take / segment_length)).cast<coord_t>();
            entrance.trim_distance = traveled_distance + take;
            entrance.valid         = entrance.toolchange != entrance.start && entrance.trim_distance > EPSILON;
            return entrance;
        }

        remaining_distance -= segment_length;
        traveled_distance += segment_length;
        previous = point;
    }

    if (traveled_distance <= EPSILON || previous == entrance.start)
        return entrance;

    entrance.toolchange    = previous;
    entrance.trim_distance = traveled_distance;
    entrance.valid         = true;
    return entrance;
}

std::optional<NoWipeTowerFlushIntoSkeletonPlan> plan_no_wipe_tower_flush_into_skeleton(const LayerRegion& layer_region)
{
    const std::optional<NoWipeTowerFlushIntoSkeletonCandidate> candidate = collect_no_wipe_tower_flush_into_skeleton_candidate(layer_region);
    if (!candidate || candidate->available_volume_mm3 <= EPSILON)
        return std::nullopt;

    NoWipeTowerFlushIntoSkeletonPlan plan;
    plan.skin_depth_mm               = 0.;
    plan.requested_volume_mm3        = candidate->available_volume_mm3;
    plan.skeleton_volume_mm3         = candidate->available_volume_mm3;
    plan.available_region_volume_mm3 = candidate->available_volume_mm3;
    return plan;
}

} // namespace Slic3r