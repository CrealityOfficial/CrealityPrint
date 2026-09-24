#include "Fingerprint.hpp"

#if defined(SLIC3R_DIAGNOSTICS_FINGERPRINT_ENABLED)

#include "Report.hpp"
#include "Hasher.hpp"
#include "../ClipperUtils.hpp"
#include "../ExtrusionEntityCollection.hpp"
#include "../Print.hpp"
#include "../support_new/TreeSupport.hpp"

#include <algorithm>
#include <array>
#include <exception>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Slic3r::Diagnostics {
namespace {

std::string calculate_fingerprint(const ConstLayerPtrsAdaptor &layers)
{
    Hasher hasher;
    hasher.add_unsigned(layers.size());
    for (const Layer *layer : layers) {
        hasher.add_unsigned(layer->id());
        hasher.add_floating_point(layer->print_z);

        std::vector<std::string> overhangs;
        overhangs.reserve(layer->loverhangs_with_type.size());
        for (const auto &[expolygon, type] : layer->loverhangs_with_type) {
            Hasher geometry;
            geometry.add_expolygon(expolygon);
            Hasher item;
            item.add_integer(type);
            item.add_string(geometry.digest());
            overhangs.emplace_back(item.digest());
        }
        std::sort(overhangs.begin(), overhangs.end());
        hasher.add_unsigned(overhangs.size());
        for (const std::string &overhang : overhangs)
            hasher.add_string(overhang);
    }
    return hasher.digest();
}

void record_error(const Detail::Target &target,
                  std::string_view group_path,
                  std::string message) noexcept
{
    try {
        target.fingerprint_report->record_error(
            target.scope, std::string(group_path), std::move(message));
    } catch (...) {
        // Diagnostics must never change the slicing result.
    }
}

} // namespace

void fingerprint(std::string_view group_path,
                 const PrintObject &object,
                 const ConstLayerPtrsAdaptor &layers) noexcept
{
    if (!Detail::fingerprint_collection_enabled())
        return;

    const Detail::Target target = Detail::find_target(object);
    if (!target.fingerprint_enabled())
        return;

    try {
        target.fingerprint_report->record(
            target.scope, std::string(group_path), calculate_fingerprint(layers));
    } catch (const std::exception &exception) {
        record_error(target, group_path, exception.what());
    } catch (...) {
        record_error(target, group_path, "unknown fingerprint error");
    }
}

namespace {

void add_extrusion_path(Hasher &hasher, const ExtrusionPath &path)
{
    hasher.add_unsigned(static_cast<unsigned int>(path.role()));
    hasher.add_floating_point(path.mm3_per_mm);
    hasher.add_floating_point(path.width);
    hasher.add_floating_point(path.height);
    hasher.add_floating_point(path.overhang_degree);
    hasher.add_integer(path.curve_degree);
    hasher.add_boolean(path.can_reverse());
    hasher.add_polyline(path.polyline);
}

void add_extrusion_entity(Hasher &hasher, const ExtrusionEntity &entity)
{
    if (const auto *collection = dynamic_cast<const ExtrusionEntityCollection *>(&entity)) {
        hasher.add_string("collection");
        hasher.add_boolean(collection->no_sort);
        hasher.add_unsigned(collection->entities.size());
        for (const ExtrusionEntity *child : collection->entities)
            add_extrusion_entity(hasher, *child);
    } else if (const auto *loop = dynamic_cast<const ExtrusionLoop *>(&entity)) {
        hasher.add_string("loop");
        hasher.add_unsigned(static_cast<unsigned int>(loop->loop_role()));
        hasher.add_unsigned(loop->paths.size());
        for (const ExtrusionPath &path : loop->paths)
            add_extrusion_path(hasher, path);
    } else if (const auto *multi_path = dynamic_cast<const ExtrusionMultiPath *>(&entity)) {
        hasher.add_string("multi_path");
        hasher.add_unsigned(multi_path->paths.size());
        for (const ExtrusionPath &path : multi_path->paths)
            add_extrusion_path(hasher, path);
    } else if (const auto *path = dynamic_cast<const ExtrusionPath *>(&entity)) {
        hasher.add_string("path");
        add_extrusion_path(hasher, *path);
    } else {
        hasher.add_string("entity");
        hasher.add_unsigned(static_cast<unsigned int>(entity.role()));
        hasher.add_floating_point(entity.length());
        hasher.add_floating_point(entity.total_volume());
        const Polylines polylines = entity.as_polylines();
        hasher.add_unsigned(polylines.size());
        for (const Polyline &polyline : polylines)
            hasher.add_polyline(polyline);
    }
}

std::string fingerprint_unoriented_polyline(const Polyline &polyline)
{
    Hasher forward;
    forward.add_unsigned(polyline.points.size());
    for (const Point &point : polyline.points)
        forward.add_point(point);

    Hasher reverse;
    reverse.add_unsigned(polyline.points.size());
    for (auto it = polyline.points.rbegin(); it != polyline.points.rend(); ++it)
        reverse.add_point(*it);

    return std::min(forward.digest(), reverse.digest());
}

void collect_support_perimeter_paths(const ExtrusionEntity &entity,
                                     std::vector<std::string> &paths,
                                     bool bridging_only)
{
    if (const auto *collection = dynamic_cast<const ExtrusionEntityCollection *>(&entity)) {
        for (const ExtrusionEntity *child : collection->entities)
            collect_support_perimeter_paths(*child, paths, bridging_only);
    } else if (const auto *loop = dynamic_cast<const ExtrusionLoop *>(&entity)) {
        for (const ExtrusionPath &path : loop->paths) {
            if (bridging_only && path.role() != erOverhangPerimeter)
                continue;
            Hasher item;
            item.add_unsigned(static_cast<unsigned int>(path.role()));
            item.add_string(fingerprint_unoriented_polyline(path.polyline));
            paths.emplace_back(item.digest());
        }
    } else if (const auto *multi_path = dynamic_cast<const ExtrusionMultiPath *>(&entity)) {
        for (const ExtrusionPath &path : multi_path->paths) {
            if (bridging_only && path.role() != erOverhangPerimeter)
                continue;
            Hasher item;
            item.add_unsigned(static_cast<unsigned int>(path.role()));
            item.add_string(fingerprint_unoriented_polyline(path.polyline));
            paths.emplace_back(item.digest());
        }
    } else if (const auto *path = dynamic_cast<const ExtrusionPath *>(&entity)) {
        if (bridging_only && path->role() != erOverhangPerimeter)
            return;
        Hasher item;
        item.add_unsigned(static_cast<unsigned int>(path->role()));
        item.add_string(fingerprint_unoriented_polyline(path->polyline));
        paths.emplace_back(item.digest());
    } else {
        if (bridging_only && entity.role() != erOverhangPerimeter)
            return;
        for (const Polyline &polyline : entity.as_polylines()) {
            Hasher item;
            item.add_unsigned(static_cast<unsigned int>(entity.role()));
            item.add_string(fingerprint_unoriented_polyline(polyline));
            paths.emplace_back(item.digest());
        }
    }
}

bool contains_bridge_infill(const ExtrusionEntity &entity)
{
    if (const auto *collection = dynamic_cast<const ExtrusionEntityCollection *>(&entity)) {
        return std::any_of(collection->entities.begin(), collection->entities.end(),
                           [](const ExtrusionEntity *child) {
                               return contains_bridge_infill(*child);
                           });
    }
    return entity.role() == erBridgeInfill || entity.role() == erInternalBridgeInfill;
}

void add_sorted_strings(Hasher &hasher, std::vector<std::string> values)
{
    std::sort(values.begin(), values.end());
    hasher.add_unsigned(values.size());
    for (const std::string &value : values)
        hasher.add_string(value);
}

void add_config_options(Hasher &hasher,
                        const ConfigBase &config,
                        const std::vector<std::string> &keys)
{
    hasher.add_unsigned(keys.size());
    for (const std::string &key : keys) {
        hasher.add_string(key);
        hasher.add_string(config.option(key)->serialize());
    }
}

template<size_t N>
void append_existing_config_keys(std::vector<std::string> &result,
                                 const ConfigBase &config,
                                 const std::array<std::string_view, N> &wanted)
{
    for (std::string_view key : wanted)
        if (config.option(std::string(key)) != nullptr)
            result.emplace_back(key);
}

std::vector<std::string> support_object_config_keys(const PrintObjectConfig &config,
                                                    bool tree_support)
{
    static constexpr std::array common_keys{
        std::string_view{"enable_support"},
        std::string_view{"thick_bridges"},
        std::string_view{"overhang_optimization"},
        std::string_view{"line_width"},
        std::string_view{"layer_height"},
        std::string_view{"support_type"},
        std::string_view{"support_style"},
        std::string_view{"support_filament"},
        std::string_view{"support_interface_filament"},
        std::string_view{"support_line_width"},
        std::string_view{"support_object_xy_distance"},
        std::string_view{"support_object_first_layer_gap"},
        std::string_view{"support_top_z_distance"},
        std::string_view{"support_bottom_z_distance"},
        std::string_view{"support_xy_overrides_z"},
        std::string_view{"support_angle"},
        std::string_view{"support_interface_top_layers"},
        std::string_view{"support_interface_bottom_layers"},
        std::string_view{"support_interface_spacing"},
        std::string_view{"support_bottom_interface_spacing"},
        std::string_view{"support_interface_pattern"},
        std::string_view{"support_interface_loop_pattern"},
        std::string_view{"support_base_pattern"},
        std::string_view{"support_base_pattern_spacing"},
        std::string_view{"support_expansion"},
        std::string_view{"support_closing_radius"},
        std::string_view{"support_ironing"},
        std::string_view{"support_ironing_pattern"},
        std::string_view{"support_ironing_flow"},
        std::string_view{"support_ironing_spacing"},
        std::string_view{"raft_layers"},
        std::string_view{"raft_expansion"},
        std::string_view{"raft_first_layer_density"},
        std::string_view{"raft_first_layer_expansion"},
        std::string_view{"brim_type"},
        std::string_view{"brim_width"},
        std::string_view{"brim_object_gap"}
    };

    std::vector<std::string> result;
    append_existing_config_keys(result, config, common_keys);

    if (tree_support) {
        static constexpr std::array tree_keys{
            std::string_view{"enforce_support_layers"},
            std::string_view{"bridge_no_support"},
            std::string_view{"max_bridge_length"},
            std::string_view{"minimum_support_area"},
            std::string_view{"min_feature_size"},
            std::string_view{"support_on_build_plate_only"},
            std::string_view{"support_critical_regions_only"},
            std::string_view{"support_remove_small_overhang"},
            std::string_view{"support_threshold_angle"},
            std::string_view{"support_interface_min_area"},
            std::string_view{"support_base_pattern_tree"},
            std::string_view{"tree_support_branch_angle"},
            std::string_view{"tree_support_branch_angle_organic"},
            std::string_view{"tree_support_angle_slow"},
            std::string_view{"tree_support_branch_distance"},
            std::string_view{"tree_support_branch_distance_organic"},
            std::string_view{"tree_support_tip_diameter"},
            std::string_view{"tree_support_branch_diameter"},
            std::string_view{"tree_support_branch_diameter_organic"},
            std::string_view{"tree_support_branch_diameter_angle"},
            std::string_view{"tree_support_branch_diameter_double_wall"},
            std::string_view{"tree_support_wall_count"},
            std::string_view{"tree_support_wall_count_tree"},
            std::string_view{"tree_support_organic_validate_repair"},
            std::string_view{"tree_support_adaptive_layer_height"},
            std::string_view{"tree_support_auto_brim"},
            std::string_view{"tree_support_brim_width"},
            std::string_view{"tree_support_with_infill"}
        };
        append_existing_config_keys(result, config, tree_keys);
    } else {
        static constexpr std::array normal_keys{
            std::string_view{"enforce_support_layers"},
            std::string_view{"bridge_no_support"},
            std::string_view{"max_bridge_length"},
            std::string_view{"minimum_support_area"},
            std::string_view{"support_on_build_plate_only"},
            std::string_view{"support_remove_small_overhang"},
            std::string_view{"support_threshold_angle"},
            std::string_view{"ironing_support_layer"}
        };
        append_existing_config_keys(result, config, normal_keys);
    }

    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

template<size_t N>
std::vector<std::string> existing_config_keys(
    const ConfigBase &config,
    const std::array<std::string_view, N> &wanted)
{
    std::vector<std::string> result;
    result.reserve(wanted.size());
    for (std::string_view key : wanted)
        if (config.option(std::string(key)) != nullptr)
            result.emplace_back(key);
    std::sort(result.begin(), result.end());
    return result;
}

std::string fingerprint_support_region_config(const PrintRegionConfig &config)
{
    static constexpr std::array region_keys{
        std::string_view{"bridge_flow"},
        std::string_view{"ironing_flow"},
        std::string_view{"ironing_spacing"},
        std::string_view{"wall_filament"},
        std::string_view{"detect_overhang_wall"},
        std::string_view{"outer_wall_line_width"},
        std::string_view{"inner_wall_line_width"},
        std::string_view{"line_width"},
        std::string_view{"sparse_infill_density"},
        std::string_view{"sparse_infill_line_width"}
    };

    Hasher hasher;
    add_config_options(hasher, config, existing_config_keys(config, region_keys));
    return hasher.digest();
}

std::string fingerprint_support_region_parameters(const PrintRegion &region)
{
    Hasher hasher;
    hasher.add_integer(region.print_region_id());
    hasher.add_integer(region.print_object_region_id());
    hasher.add_string(fingerprint_support_region_config(region.config()));
    return hasher.digest();
}

void add_support_configuration(Hasher &hasher, const PrintObject &object, bool tree_support)
{
    const PrintObjectConfig &object_config = object.config();
    const PrintConfig &print_config = object.print()->config();

    add_config_options(hasher, object_config,
                       support_object_config_keys(object_config, tree_support));

    static constexpr std::array common_print_keys{
        std::string_view{"filament_soluble"},
        std::string_view{"min_layer_height"},
        std::string_view{"nozzle_diameter"},
        std::string_view{"independent_support_layer_height"},
        std::string_view{"initial_layer_line_width"},
        std::string_view{"initial_layer_print_height"}
    };
    std::vector<std::string> print_keys = existing_config_keys(print_config, common_print_keys);
    if (tree_support) {
        static constexpr std::array tree_print_keys{
            std::string_view{"printable_area"},
            std::string_view{"bed_exclude_area"},
            std::string_view{"printable_height"},
            std::string_view{"machine_is_belt"},
            std::string_view{"resolution"},
            std::string_view{"skirt_height"},
            std::string_view{"skirt_loops"},
            std::string_view{"draft_shield"}
        };
        std::vector<std::string> tree_keys = existing_config_keys(print_config, tree_print_keys);
        print_keys.insert(print_keys.end(), tree_keys.begin(), tree_keys.end());
        std::sort(print_keys.begin(), print_keys.end());
    }
    add_config_options(hasher, print_config, print_keys);
    hasher.add_string(fingerprint_support_region_config(
        object.print()->default_region_config()));

    std::vector<std::string> regions;
    regions.reserve(object.num_printing_regions());
    for (size_t region_id = 0; region_id < object.num_printing_regions(); ++region_id)
        regions.emplace_back(fingerprint_support_region_parameters(
            object.printing_region(region_id)));
    add_sorted_strings(hasher, std::move(regions));
}

void add_flow(Hasher &hasher, const Flow &flow)
{
    hasher.add_floating_point(flow.width());
    hasher.add_floating_point(flow.height());
    hasher.add_floating_point(flow.spacing());
    hasher.add_floating_point(flow.nozzle_diameter());
    hasher.add_boolean(flow.bridge());
}

void add_slicing_parameters(Hasher &hasher, const SlicingParameters &parameters)
{
    hasher.add_boolean(parameters.valid);
    hasher.add_unsigned(parameters.base_raft_layers);
    hasher.add_unsigned(parameters.interface_raft_layers);
    hasher.add_floating_point(parameters.base_raft_layer_height);
    hasher.add_floating_point(parameters.interface_raft_layer_height);
    hasher.add_floating_point(parameters.contact_raft_layer_height);
    hasher.add_floating_point(parameters.layer_height);
    hasher.add_floating_point(parameters.min_layer_height);
    hasher.add_floating_point(parameters.max_layer_height);
    hasher.add_floating_point(parameters.max_suport_layer_height);
    hasher.add_floating_point(parameters.first_print_layer_height);
    hasher.add_floating_point(parameters.first_object_layer_height);
    hasher.add_boolean(parameters.first_object_layer_bridging);
    hasher.add_boolean(parameters.soluble_interface);
    hasher.add_floating_point(parameters.gap_raft_object);
    hasher.add_floating_point(parameters.gap_object_support);
    hasher.add_floating_point(parameters.gap_support_object);
    hasher.add_floating_point(parameters.xy_distance_overhang);
    hasher.add_floating_point(parameters.threshold_rad);
    hasher.add_boolean(parameters.use_xy_distance_overhang);
    hasher.add_floating_point(parameters.raft_base_top_z);
    hasher.add_floating_point(parameters.raft_interface_top_z);
    hasher.add_floating_point(parameters.raft_contact_top_z);
    hasher.add_floating_point(parameters.object_print_z_min);
    hasher.add_floating_point(parameters.object_print_z_max);
    hasher.add_floating_point(parameters.object_print_z_uncompensated_max);
    hasher.add_floating_point(parameters.object_shrinkage_compensation_z);
}

std::string fingerprint_support_surfaces(const SurfaceCollection &surfaces,
                                         bool include_internal_solids)
{
    std::vector<std::string> selected;
    for (const Surface &surface : surfaces) {
        if (surface.surface_type != stBottomBridge &&
            !(include_internal_solids && surface.surface_type == stInternalSolid))
            continue;
        Hasher item;
        item.add_unsigned(static_cast<unsigned int>(surface.surface_type));
        item.add_expolygon(surface.expolygon);
        selected.emplace_back(item.digest());
    }
    Hasher hasher;
    add_sorted_strings(hasher, std::move(selected));
    return hasher.digest();
}

std::string fingerprint_region_slices(const SurfaceCollection &surfaces)
{
    std::vector<std::string> slices;
    slices.reserve(surfaces.size());
    for (const Surface &surface : surfaces) {
        Hasher slice;
        slice.add_unsigned(static_cast<unsigned int>(surface.surface_type));
        slice.add_expolygon(surface.expolygon);
        slices.emplace_back(slice.digest());
    }
    Hasher hasher;
    add_sorted_strings(hasher, std::move(slices));
    return hasher.digest();
}

struct SupportRegionUsage
{
    bool tree_support;
    bool all_perimeters;
    bool bridging_perimeters;
    bool bridge_surfaces;
    bool bridge_presence;
    bool unsupported_bridge_edges;
};

std::string fingerprint_support_region(const LayerRegion &region,
                                       const SupportRegionUsage &usage)
{
    Hasher hasher;
    hasher.add_integer(region.region().print_region_id());
    hasher.add_integer(region.region().print_object_region_id());
    if (!usage.tree_support) {
        hasher.add_string(fingerprint_region_slices(region.slices));
        hasher.add_expolygons(region.raw_slices);
    }

    if (usage.bridge_surfaces)
        hasher.add_string(fingerprint_support_surfaces(
            region.fill_surfaces, usage.tree_support));
    if (usage.bridge_presence) {
        hasher.add_boolean(region.fill_surfaces.has(stBottomBridge));
        hasher.add_boolean(contains_bridge_infill(region.fills));
    }

    if (usage.unsupported_bridge_edges) {
        std::vector<std::string> bridge_edges;
        bridge_edges.reserve(region.unsupported_bridge_edges.size());
        for (const Polyline &edge : region.unsupported_bridge_edges)
            bridge_edges.emplace_back(fingerprint_unoriented_polyline(edge));
        add_sorted_strings(hasher, std::move(bridge_edges));
    }

    if (usage.all_perimeters ||
        (usage.bridging_perimeters && region.region().config().detect_overhang_wall.value)) {
        std::vector<std::string> perimeter_paths;
        collect_support_perimeter_paths(
            region.perimeters, perimeter_paths, !usage.all_perimeters);
        add_sorted_strings(hasher, std::move(perimeter_paths));
    }
    return hasher.digest();
}

SupportRegionUsage support_region_usage(const PrintObject &object, bool tree_support)
{
    const bool thick_bridge_processing =
        !object.slicing_parameters().soluble_interface && object.config().thick_bridges.value;
    return SupportRegionUsage{
        tree_support,
        tree_support ? object.config().max_bridge_length.value > 0.
                     : object.config().bridge_no_support.value,
        tree_support || thick_bridge_processing,
        tree_support || thick_bridge_processing || object.config().bridge_no_support.value,
        !tree_support && object.config().thick_bridges.value &&
            object.print()->config().independent_support_layer_height.value,
        tree_support ? object.config().max_bridge_length.value > 0.
                     : object.config().bridge_no_support.value
    };
}

std::string fingerprint_support_parameters(const PrintObject &object, bool tree_support)
{
    Hasher hasher;

    hasher.add_boolean(tree_support);
    add_support_configuration(hasher, object, tree_support);
    add_slicing_parameters(hasher, object.slicing_parameters());
    if (tree_support)
        hasher.add_boolean(object.has_variable_layer_heights);

    const std::vector<unsigned int> extruders = object.object_extruders();
    hasher.add_unsigned(extruders.size());
    for (unsigned int extruder : extruders)
        hasher.add_unsigned(extruder);

    if (tree_support) {
        hasher.add_boolean(object.print()->has_brim());
        hasher.add_boolean(object.print()->has_infinite_skirt());
        add_flow(hasher, object.print()->brim_flow());

        hasher.add_boolean(!object.instances().empty());
        if (!object.instances().empty())
            hasher.add_point(object.instances().front().shift);

        const Vec3d plate_origin = object.print()->get_plate_origin();
        hasher.add_floating_point(plate_origin.x());
        hasher.add_floating_point(plate_origin.y());
        hasher.add_floating_point(plate_origin.z());
    }

    const ConstLayerPtrsAdaptor layers = object.layers();
    std::string first_layer_active_region;
    if (layers.size() > 0) {
        for (const LayerRegion *layer_region : layers[0]->regions()) {
            if (!layer_region->slices.empty()) {
                first_layer_active_region = fingerprint_support_region_parameters(
                    layer_region->region());
                break;
            }
        }
    }
    hasher.add_boolean(!first_layer_active_region.empty());
    if (!first_layer_active_region.empty())
        hasher.add_string(first_layer_active_region);

    return hasher.digest();
}

std::string fingerprint_support_geometry(const PrintObject &object,
                                         const SupportRegionUsage &region_usage)
{
    Hasher hasher;
    const ConstLayerPtrsAdaptor layers = object.layers();

    hasher.add_unsigned(layers.size());
    for (const Layer *layer : layers) {
        hasher.add_unsigned(layer->id());
        hasher.add_floating_point(layer->slice_z);
        hasher.add_floating_point(layer->print_z);
        hasher.add_floating_point(layer->height);
        hasher.add_expolygons(layer->lslices);

        std::vector<std::string> regions;
        regions.reserve(layer->regions().size());
        for (const LayerRegion *layer_region : layer->regions())
            regions.emplace_back(fingerprint_support_region(*layer_region, region_usage));
        add_sorted_strings(hasher, std::move(regions));
    }
    return hasher.digest();
}

std::string calculate_fingerprint(const SupportModuleInput &input)
{
    const PrintObject &object = input.object;
    const bool tree_support = is_tree(object.config().support_type.value);
    const SupportRegionUsage region_usage = support_region_usage(object, tree_support);

    Hasher hasher;
    hasher.add_string("parameters");
    hasher.add_string(fingerprint_support_parameters(object, tree_support));
    hasher.add_string("geometry");
    hasher.add_string(fingerprint_support_geometry(object, region_usage));
    return hasher.digest();
}

std::string fingerprint_parent(const SupportNode &node)
{
    Hasher hasher;
    hasher.add_integer(node.obj_layer_nr);
    hasher.add_point(node.position);
    hasher.add_integer(node.distance_to_top);
    hasher.add_floating_point(node.print_z);
    return hasher.digest();
}

std::string fingerprint_node(const SupportNode &node)
{
    Hasher hasher;
    hasher.add_integer(node.obj_layer_nr);
    hasher.add_point(node.position);
    hasher.add_integer(node.distance_to_top);
    hasher.add_floating_point(node.dist_mm_to_top);
    hasher.add_floating_point(node.radius);
    hasher.add_floating_point(node.max_move_dist);
    hasher.add_unsigned(static_cast<unsigned int>(node.type));
    hasher.add_boolean(node.is_corner);
    hasher.add_boolean(node.is_processed);
    hasher.add_boolean(node.need_extra_wall);
    hasher.add_boolean(node.is_sharp_tail);
    hasher.add_boolean(node.is_manual_enforcer);
    hasher.add_boolean(node.valid);
    hasher.add_boolean(node.fading);
    hasher.add_boolean(node.to_buildplate);
    hasher.add_integer(node.support_roof_layers_below);
    hasher.add_floating_point(node.overhang_degree);
    hasher.add_floating_point(node.print_z);
    hasher.add_floating_point(node.height);
    hasher.add_point(node.movement);
    hasher.add_point(node.skin_direction);
    hasher.add_expolygon(node.overhang);
    hasher.add_floating_point(node.origin_area);

    hasher.add_boolean(node.parent != nullptr);
    if (node.parent != nullptr)
        hasher.add_string(fingerprint_parent(*node.parent));
    hasher.add_boolean(node.child != nullptr);
    if (node.child != nullptr)
        hasher.add_string(fingerprint_parent(*node.child));

    std::vector<std::string> parents;
    parents.reserve(node.parents.size());
    for (const SupportNode *parent : node.parents)
        if (parent != nullptr)
            parents.emplace_back(fingerprint_parent(*parent));
    std::sort(parents.begin(), parents.end());
    hasher.add_unsigned(parents.size());
    for (const std::string &parent : parents)
        hasher.add_string(parent);

    std::vector<std::string> merged_neighbours;
    merged_neighbours.reserve(node.merged_neighbours.size());
    for (const SupportNode *neighbour : node.merged_neighbours)
        if (neighbour != nullptr)
            merged_neighbours.emplace_back(fingerprint_parent(*neighbour));
    std::sort(merged_neighbours.begin(), merged_neighbours.end());
    hasher.add_unsigned(merged_neighbours.size());
    for (const std::string &neighbour : merged_neighbours)
        hasher.add_string(neighbour);
    return hasher.digest();
}

std::string calculate_fingerprint(const ConstSupportLayerPtrsAdaptor &layers)
{
    Hasher hasher;
    hasher.add_unsigned(layers.size());
    for (const SupportLayer *layer : layers) {
        hasher.add_unsigned(layer->id());
        hasher.add_unsigned(layer->interface_id());
        hasher.add_floating_point(layer->slice_z);
        hasher.add_floating_point(layer->print_z);
        hasher.add_floating_point(layer->height);
        hasher.add_unsigned(static_cast<unsigned int>(layer->support_type));
        hasher.add_expolygons(layer->support_islands);
        hasher.add_expolygons(layer->base_areas);
        hasher.add_expolygons(layer->overhang_areas);
        hasher.add_expolygons(layer->lslices);
        add_extrusion_entity(hasher, layer->support_fills);
    }
    return hasher.digest();
}

std::string calculate_fingerprint(const std::vector<std::vector<SupportNode *>> &nodes)
{
    Hasher hasher;
    hasher.add_unsigned(nodes.size());
    for (std::size_t layer_id = 0; layer_id < nodes.size(); ++layer_id) {
        std::vector<std::string> layer_nodes;
        layer_nodes.reserve(nodes[layer_id].size());
        for (const SupportNode *node : nodes[layer_id])
            if (node != nullptr)
                layer_nodes.emplace_back(fingerprint_node(*node));
        std::sort(layer_nodes.begin(), layer_nodes.end());

        hasher.add_unsigned(layer_id);
        hasher.add_unsigned(layer_nodes.size());
        for (const std::string &node : layer_nodes)
            hasher.add_string(node);
    }
    return hasher.digest();
}

std::string calculate_fingerprint(const std::vector<TreeSupport3D::SupportElements> &layers)
{
    Hasher hasher;
    hasher.add_unsigned(layers.size());
    for (std::size_t layer_id = 0; layer_id < layers.size(); ++layer_id) {
        std::vector<std::string> elements;
        elements.reserve(layers[layer_id].size());
        for (const TreeSupport3D::SupportElement &element : layers[layer_id]) {
            const TreeSupport3D::SupportElementState &state = element.state;
            Hasher item;
            item.add_integer(state.target_height);
            item.add_point(state.target_position);
            item.add_point(state.next_position);
            item.add_integer(state.layer_idx);
            item.add_unsigned(state.effective_radius_height);
            item.add_unsigned(state.distance_to_top);
            item.add_boolean(state.result_on_layer_is_set());
            if (state.result_on_layer_is_set())
                item.add_point(state.result_on_layer);
            item.add_integer(state.increased_to_model_radius);
            item.add_floating_point(state.elephant_foot_increases);
            item.add_unsigned(state.dont_move_until);
            item.add_unsigned(state.missing_roof_layers);
            item.add_boolean(state.to_buildplate);
            item.add_boolean(state.to_model_gracious);
            item.add_boolean(state.non_gracious_model_landing_confirmed);
            item.add_boolean(state.use_min_xy_dist);
            item.add_boolean(state.supports_roof);
            item.add_boolean(state.can_use_safe_radius);
            item.add_boolean(state.skip_ovalisation);

            std::vector<std::string> parents;
            parents.reserve(element.parents.size());
            if (layer_id + 1 < layers.size()) {
                for (int32_t parent_id : element.parents) {
                    if (parent_id < 0 || static_cast<std::size_t>(parent_id) >= layers[layer_id + 1].size())
                        continue;
                    const TreeSupport3D::SupportElementState &parent = layers[layer_id + 1][parent_id].state;
                    Hasher parent_hasher;
                    parent_hasher.add_integer(parent.type);
                    parent_hasher.add_integer(parent.target_height);
                    parent_hasher.add_point(parent.target_position);
                    parent_hasher.add_point(parent.next_position);
                    parent_hasher.add_integer(parent.layer_idx);
                    parent_hasher.add_unsigned(parent.distance_to_top);
                    parent_hasher.add_boolean(parent.result_on_layer_is_set());
                    if (parent.result_on_layer_is_set())
                        parent_hasher.add_point(parent.result_on_layer);
                    parents.emplace_back(parent_hasher.digest());
                }
            }
            std::sort(parents.begin(), parents.end());
            item.add_unsigned(parents.size());
            for (const std::string &parent : parents)
                item.add_string(parent);

            item.add_expolygons(union_ex(element.influence_area));

            std::vector<std::pair<coord_t, coord_t>> cores;
            cores.reserve(element.multi_core.core_positions.size());
            for (const Point &core : element.multi_core.core_positions)
                cores.emplace_back(core.x(), core.y());
            std::sort(cores.begin(), cores.end());
            item.add_boolean(element.multi_core.is_multi_core);
            item.add_unsigned(cores.size());
            for (const auto &[x, y] : cores) {
                item.add_integer(x);
                item.add_integer(y);
            }
            elements.emplace_back(item.digest());
        }
        std::sort(elements.begin(), elements.end());
        hasher.add_unsigned(layer_id);
        hasher.add_unsigned(elements.size());
        for (const std::string &element : elements)
            hasher.add_string(element);
    }
    return hasher.digest();
}

template<typename Factory>
void record_fingerprint(std::string_view group_path,
                        const PrintObject &object,
                        Factory &&factory) noexcept
{
    if (!Detail::fingerprint_collection_enabled())
        return;

    const Detail::Target target = Detail::find_target(object);
    if (!target.fingerprint_enabled())
        return;

    try {
        target.fingerprint_report->record(
            target.scope, std::string(group_path), factory());
    } catch (const std::exception &exception) {
        try {
            target.fingerprint_report->record_error(
                target.scope, std::string(group_path), exception.what());
        } catch (...) {
        }
    } catch (...) {
        try {
            target.fingerprint_report->record_error(
                target.scope, std::string(group_path), "unknown fingerprint error");
        } catch (...) {
        }
    }
}

} // namespace

void fingerprint(std::string_view group_path,
                 const PrintObject &object,
                 const SupportModuleInput &input) noexcept
{
    record_fingerprint(group_path, object,
                       [&input] { return calculate_fingerprint(input); });
}

void fingerprint(std::string_view group_path,
                 const PrintObject &object,
                 const ConstSupportLayerPtrsAdaptor &layers) noexcept
{
    record_fingerprint(group_path, object,
                       [&layers] { return calculate_fingerprint(layers); });
}

void fingerprint(std::string_view group_path,
                 const PrintObject &object,
                 const std::vector<std::vector<SupportNode *>> &nodes) noexcept
{
    record_fingerprint(group_path, object,
                       [&nodes] { return calculate_fingerprint(nodes); });
}

void fingerprint(std::string_view group_path,
                 const PrintObject &object,
                 const std::vector<TreeSupport3D::SupportElements> &elements) noexcept
{
    record_fingerprint(group_path, object,
                       [&elements] { return calculate_fingerprint(elements); });
}

} // namespace Slic3r::Diagnostics

#endif
