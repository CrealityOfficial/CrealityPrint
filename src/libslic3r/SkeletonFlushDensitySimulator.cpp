#include "SkeletonFlushDensitySimulator.hpp"

#include "ExtrusionEntity.hpp"
#include "Fill/FillRectilinear.hpp"
#include "Geometry.hpp"
#include "Layer.hpp"
#include "Print.hpp"
#include "PrintConfig.hpp"
#include "Surface.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace Slic3r {

namespace {

static float lockedzag_skin_depth(const LayerRegion& layer_region)
{
    const PrintObject& object = *layer_region.layer()->object();
    const PrintRegionConfig& region_config = layer_region.region().config();
    if (const float* resolved = object.print()->lockedzag_skin_infill_depth(layer_region))
        return *resolved;
    return float(region_config.skin_infill_depth);
}

static std::unique_ptr<ExtrusionEntityCollection> generate_skeleton_at_density(
    const LayerRegion& layer_region,
    float density)
{
    const Layer* layer = layer_region.layer();
    if (layer == nullptr || layer_region.region().config().sparse_infill_pattern.value != ipLockedZag)
        return nullptr;

    const PrintObject& object = *layer->object();
    const PrintRegionConfig& region_config = layer_region.region().config();
    const float layer_height = float(layer->height);
    const float skin_depth = lockedzag_skin_depth(layer_region);

    auto output = std::make_unique<ExtrusionEntityCollection>();
    bool have_source_collection = false;
    for (const ExtrusionEntity* entity : layer_region.fills.entities) {
        if (const auto* collection = dynamic_cast<const ExtrusionEntityCollection*>(entity);
            collection != nullptr && collection->role() == erInternalInfill) {
            if (!have_source_collection) {
                output->no_sort = collection->no_sort;
                if (!collection->can_reverse())
                    output->set_reverse();
                have_source_collection = true;
            }
        }
    }

    if (!have_source_collection)
        return nullptr;

    const Flow spacing_flow = layer_region.region().flow(
        object,
        frInfill,
        object.config().layer_height,
        false);

    for (const Surface& source_surface : layer_region.fill_surfaces.surfaces) {
        if (source_surface.surface_type != stInternal || source_surface.expolygon.empty())
            continue;

        const float surface_height = source_surface.thickness == -1.
            ? layer_height
            : float(source_surface.thickness);
        const Flow params_flow = layer_region.flow(frInfill, surface_height);
        const Flow skeleton_flow = resolve_infill_detail_flow_width(object, layer_region.region(), frInfill, false).flow(surface_height);
        const Flow skin_flow = resolve_infill_detail_flow_width(object, layer_region.region(), frInfill, true).flow(surface_height);

        FillLockedZag fill;
        fill.set_bounding_box(object.bounding_box());
        fill.set_bounding_box_height(object.height());
        fill.layer_id = layer->id();
        fill.z = layer->print_z;
        fill.angle = float(Geometry::deg2rad(region_config.infill_direction.value));
        fill.rotate_angle = true;
        fill.spacing = spacing_flow.spacing();
        fill.link_max_length = region_config.sparse_infill_density > 80.f
            ? coord_t(scale_(3.f * fill.spacing))
            : 0;
        fill.loop_clipping = coord_t(scale_(
            layer_region.region().config().seam_gap.get_abs_value(skeleton_flow.nozzle_diameter())));
        fill.print_config = &object.print()->config();
        fill.print_object_config = &object.config();
        // Keep simulation identical to the flush-enabled production path:
        // configured skin pattern with a Grid skeleton.
        fill.set_skin_and_skeleton_pattern(
            region_config.locked_skin_infill_pattern.value,
            ipGrid);
        LockRegionParam lock_param;
        lock_param.skeleton_density_params[density].push_back(source_surface.expolygon);
        lock_param.skin_density_params[1.f].push_back(source_surface.expolygon);
        lock_param.skeleton_flow_params[skeleton_flow].push_back(source_surface.expolygon);
        lock_param.skin_flow_params[skin_flow].push_back(source_surface.expolygon);
        fill.set_lock_region_param(lock_param);

        FillParams params;
        params.density = float(0.01 * region_config.sparse_infill_density);
        params.dont_adjust = false;
        params.resolution = object.print()->config().resolution.value;
        params.anchor_length = float(region_config.infill_anchor);
        if (region_config.infill_anchor.percent)
            params.anchor_length = float(params.anchor_length * 0.01 * fill.spacing);
        params.anchor_length_max = float(region_config.infill_anchor_max);
        if (region_config.infill_anchor_max.percent)
            params.anchor_length_max = float(params.anchor_length_max * 0.01 * fill.spacing);
        params.anchor_length = std::min(params.anchor_length, params.anchor_length_max);
        params.flow = params_flow;
        params.extrusion_role = erInternalInfill;
        params.using_internal_flow = true;
        params.config = &region_config;
        params.pattern = ipLockedZag;
        params.locked_zag = true;
        params.infill_lock_depth = scale_(region_config.infill_lock_depth);
        params.skin_infill_depth = scale_(skin_depth);
        params.infill_overhang_angle = region_config.infill_overhang_angle;

        if (layer->id() % 2 == 0)
            params.horiz_move -= region_config.infill_shift_step * float(layer->id() / 2);
        else
            params.horiz_move += region_config.infill_shift_step * float(layer->id() / 2);
        params.symmetric_infill_y_axis = region_config.symmetric_infill_y_axis;

        Surface surface = source_surface;
        if (params.symmetric_infill_y_axis) {
            params.symmetric_y_axis = static_cast<const Fill&>(fill).extended_object_bounding_box().center().x();
            surface.expolygon.symmetric_y(params.symmetric_y_axis);
        }

        std::vector<std::pair<Polylines, Flow>> skeleton_polylines;
        std::vector<std::pair<Polylines, Flow>> unused_skin_polylines;
        fill.fill_surface_locked_zag(
            &surface,
            params,
            skeleton_polylines,
            unused_skin_polylines);

        for (auto& polyline_with_flow : skeleton_polylines) {
            double mm3_per_mm = polyline_with_flow.second.mm3_per_mm();
            float width = polyline_with_flow.second.width();
            if (!params.using_internal_flow) {
                const Flow adjusted = polyline_with_flow.second.with_spacing(fill.spacing);
                mm3_per_mm = adjusted.mm3_per_mm();
                width = adjusted.width();
            }
            extrusion_entities_append_paths(
                output->entities,
                std::move(polyline_with_flow.first),
                erInternalInfill,
                mm3_per_mm,
                width,
                polyline_with_flow.second.height(),
                true);
        }
    }

    return output;
}

static float current_skeleton_volume(const LayerRegion& layer_region)
{
    double volume = 0.;
    for (const ExtrusionEntity* entity : layer_region.fills.entities)
        if (const auto* collection = dynamic_cast<const ExtrusionEntityCollection*>(entity);
            collection != nullptr && collection->role() == erInternalInfill)
            volume += collection->total_volume();
    return float(volume);
}

static bool replace_skeleton_collection(
    LayerRegion& layer_region,
    std::unique_ptr<ExtrusionEntityCollection> replacement)
{
    if (replacement == nullptr || replacement->entities.empty())
        return false;

    size_t first_index = size_t(-1);
    for (size_t i = 0; i < layer_region.fills.entities.size(); ++i) {
        const auto* collection = dynamic_cast<const ExtrusionEntityCollection*>(layer_region.fills.entities[i]);
        if (collection != nullptr && collection->role() == erInternalInfill) {
            first_index = i;
            break;
        }
    }
    if (first_index == size_t(-1))
        return false;

    delete layer_region.fills.entities[first_index];
    layer_region.fills.entities[first_index] = replacement.release();

    for (size_t i = layer_region.fills.entities.size(); i-- > 0;) {
        if (i == first_index)
            continue;
        const auto* collection = dynamic_cast<const ExtrusionEntityCollection*>(layer_region.fills.entities[i]);
        if (collection != nullptr && collection->role() == erInternalInfill) {
            delete layer_region.fills.entities[i];
            layer_region.fills.entities.erase(layer_region.fills.entities.begin() + i);
        }
    }
    return true;
}

} // namespace

SkeletonFlushDensityResult replace_skeleton_with_flush_density(
    LayerRegion& layer_region,
    float current_flush_volume,
    float density_step,
    float max_density)
{
    SkeletonFlushDensityResult result;
    const PrintObject* object = layer_region.layer() == nullptr ? nullptr : layer_region.layer()->object();
    if (object == nullptr || !object->config().flush_into_skeleton.value ||
        layer_region.region().config().sparse_infill_pattern.value != ipLockedZag ||
        current_flush_volume <= EPSILON ||
        layer_region.skeleton_flush_density_replaced)
        return result;

    result.base_volume = current_skeleton_volume(layer_region);
    result.required_volume = result.base_volume + kSkeletonFlushShare * current_flush_volume;

    const float configured_density =
        std::clamp(float(0.01 * layer_region.region().config().skeleton_infill_density), 0.01f, max_density);
    density_step = std::max(0.001f, density_step);
    max_density = std::clamp(max_density, configured_density, 0.99f);

    // Search discrete density candidates instead of scanning every 1% step.
    // Using an index avoids accumulated floating-point error in the candidates.
    const float density_range = std::max(0.f, max_density - configured_density);
    const int max_step_index = int(std::ceil(density_range / density_step - 1e-5f));
    auto density_at = [&](int index) {
        return index >= max_step_index
            ? max_density
            : std::min(configured_density + float(index) * density_step, max_density);
    };

    struct Candidate
    {
        float density = 0.f;
        float volume  = 0.f;
        std::unique_ptr<ExtrusionEntityCollection> paths;

        bool valid() const { return paths != nullptr && !paths->entities.empty(); }
    };

    std::vector<float> tested_volumes(size_t(max_step_index + 1), -1.f);
    auto simulate = [&](int index) {
        Candidate candidate;
        candidate.density = density_at(index);
        candidate.paths = generate_skeleton_at_density(layer_region, candidate.density);
        ++result.simulation_count;
        if (candidate.valid())
            candidate.volume = float(candidate.paths->total_volume());
        tested_volumes[size_t(index)] = candidate.valid() ? candidate.volume : 0.f;
        return candidate;
    };

    // First establish whether the requested capacity is physically reachable.
    // If 99% is insufficient, it is also the best available fallback and the
    // search completes after this single simulation.
    Candidate best = simulate(max_step_index);
    if (!best.valid())
        return result;

    if (best.volume + EPSILON < result.required_volume) {
        if (best.volume + EPSILON >= result.base_volume) {
            result.selected_density = best.density;
            result.simulated_volume = best.volume;
            result.replaced = replace_skeleton_collection(layer_region, std::move(best.paths));
            if (result.replaced)
                layer_region.skeleton_flush_density_replaced = true;
        }
        return result;
    }

    // The virtual index -1 is known to be insufficient: required_volume is
    // base_volume plus a positive share of the purge. Find the first passing
    // candidate in [0, max_step_index].
    int low = -1;
    int high = max_step_index;
    while (high - low > 1) {
        const int middle = low + (high - low) / 2;
        Candidate candidate = simulate(middle);
        if (candidate.valid() && candidate.volume + EPSILON >= result.required_volume) {
            high = middle;
            best = std::move(candidate);
        } else {
            low = middle;
        }
    }

    // Grid clipping is discrete and may create tiny local volume fluctuations.
    // Verify up to two untested candidates immediately below the binary result.
    for (int index = high - 1, checks = 0; index >= 0 && checks < 2; --index, ++checks) {
        if (tested_volumes[size_t(index)] >= 0.f)
            continue;
        Candidate candidate = simulate(index);
        if (candidate.valid() && candidate.volume + EPSILON >= result.required_volume) {
            high = index;
            best = std::move(candidate);
        }
    }

    result.selected_density = best.density;
    result.simulated_volume = best.volume;
    result.target_met = true;
    result.replaced = replace_skeleton_collection(layer_region, std::move(best.paths));
    if (result.replaced)
        layer_region.skeleton_flush_density_replaced = true;
    return result;

}

} // namespace Slic3r
