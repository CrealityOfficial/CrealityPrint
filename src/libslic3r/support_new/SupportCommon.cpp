// [FORMATTED BY CLANG-FORMAT 2026-05-19 19:45:11]
///|/ Copyright (c) Prusa Research 2023 VojtÄ›ch BubnÃ­k @bubnikv, Pavel MikuÅ¡ @Godrak
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "../AABBTreeLines.hpp"
#include "../KDTreeIndirect.hpp"
#include "../ClipperUtils.hpp"
// #include "../ClipperZUtils.hpp"
#include "../ExtrusionEntityCollection.hpp"
#include "../Layer.hpp"
#include "../Print.hpp"
#include "../Fill/FillBase.hpp"
#include "../MutablePolygon.hpp"
#include "../Geometry.hpp"
#include "../Point.hpp"
#include "clipper/clipper_z.hpp"

#include <cmath>
#include <deque>
#include <limits>
#include <numeric>
#include <boost/container/static_vector.hpp>
#include <boost/log/trivial.hpp>

#include <tbb/parallel_for.h>

#include <fstream>
#include <iomanip>
#include <sstream>
#include <boost/filesystem.hpp>
#include "../Utils.hpp"

#include "SupportCommon.hpp"
#include "SupportLayer.hpp"
#include "SupportParameters.hpp"

// #define SLIC3R_DEBUG

// Make assert active if SLIC3R_DEBUG
#ifdef SLIC3R_DEBUG
#define DEBUG
#define _DEBUG
#undef NDEBUG
#include "../SVG.hpp"
#endif

#include <cassert>

namespace Slic3r {

static double support_debug_area_mm2(double scaled_area) { return std::abs(scaled_area) * sqr(SCALING_FACTOR); }

// Relative path keeps the optional repair trace local to the running process
// instead of writing into a developer-specific checkout directory.
static void append_footprint_repair_debug(const std::string& msg, bool clear_first = false)
{
    std::ofstream ofs("footprint_repair_debug.txt", clear_first ? std::ios::trunc : std::ios::app);
    if (ofs)
        ofs << msg << '\n';
}

static void append_footprint_repair_island_debug(const char*        result,
                                                 int                upper_idx,
                                                 size_t             lower_idx,
                                                 double             upper_area,
                                                 double             missing_area,
                                                 double             repair_area,
                                                 double             generated_overlap,
                                                 double             tol_masked_area,
                                                 const BoundingBox& bbox)
{
    std::ostringstream ss;
    ss << "[FOOT_REPAIR_ISLAND] result=" << result << " upper=" << upper_idx << " lower=" << lower_idx
       << " upper_area_mm2=" << support_debug_area_mm2(upper_area) << " missing_mm2=" << support_debug_area_mm2(missing_area)
       << " repair_mm2=" << support_debug_area_mm2(repair_area) << " generated_overlap_mm2=" << support_debug_area_mm2(generated_overlap)
       << " tol_masked_mm2=" << support_debug_area_mm2(tol_masked_area) << " bbox_min=(" << unscale<double>(bbox.min.x()) << ","
       << unscale<double>(bbox.min.y()) << ")"
       << " bbox_max=(" << unscale<double>(bbox.max.x()) << "," << unscale<double>(bbox.max.y()) << ")";
    append_footprint_repair_debug(ss.str());
}

static Polygons support_extrusions_covered_by_width(const ExtrusionEntitiesPtr& extrusions)
{
    Polygons covered;
    for (const ExtrusionEntity* entity : extrusions)
        entity->polygons_covered_by_width(covered, float(SCALED_EPSILON));
    return covered;
}

// how much we extend support around the actual contact area
// FIXME this should be dependent on the nozzle diameter!
#define SUPPORT_MATERIAL_MARGIN 1.5

// #define SUPPORT_SURFACES_OFFSET_PARAMETERS ClipperLib::jtMiter, 3.
// #define SUPPORT_SURFACES_OFFSET_PARAMETERS ClipperLib::jtMiter, 1.5
#define SUPPORT_SURFACES_OFFSET_PARAMETERS ClipperLib::jtSquare, 0.
static const double area_ironing_interface_supported = scale_(10); // min: 10x10=100mm^2

// Convert some of the intermediate layers into top/bottom interface layers as well as base interface layers.
std::pair<SupportGeneratorLayersPtr, SupportGeneratorLayersPtr> generate_interface_layers(
    const PrintObjectConfig&         config,
    const SupportParameters&         support_params,
    const SupportGeneratorLayersPtr& bottom_contacts,
    const SupportGeneratorLayersPtr& top_contacts,
    // Input / output, will be merged with output. Only provided for Organic supports.
    SupportGeneratorLayersPtr& top_interface_layers,
    SupportGeneratorLayersPtr& top_base_interface_layers,
    // Input, will be trimmed with the newly created interface layers.
    SupportGeneratorLayersPtr&    intermediate_layers,
    SupportGeneratorLayerStorage& layer_storage)
{
    std::pair<SupportGeneratorLayersPtr, SupportGeneratorLayersPtr> base_and_interface_layers;

    if (!intermediate_layers.empty() && support_params.has_interfaces()) {
        // For all intermediate layers, collect top contact surfaces, which are not further than support_material_interface_layers.
        BOOST_LOG_TRIVIAL(debug) << "PrintObjectSupportMaterial::generate_interface_layers() in parallel - start";
        const bool                 snug_supports         = support_params.support_style == smsSnug;
        const bool                 smooth_supports       = support_params.support_style != smsGrid;
        SupportGeneratorLayersPtr& interface_layers      = base_and_interface_layers.first;
        SupportGeneratorLayersPtr& base_interface_layers = base_and_interface_layers.second;

        interface_layers.assign(intermediate_layers.size(), nullptr);
        if (support_params.has_base_interfaces())
            base_interface_layers.assign(intermediate_layers.size(), nullptr);
        const auto smoothing_distance    = support_params.support_material_interface_flow.scaled_spacing() * 1.5;
        const auto minimum_island_radius = support_params.support_material_interface_flow.scaled_spacing() /
                                           support_params.interface_density;
        const auto closing_distance = smoothing_distance; // scaled<float>(config.support_material_closing_radius.value);
        // Insert a new layer into base_interface_layers, if intersection with base exists.
        auto insert_layer = [&layer_storage, smooth_supports, closing_distance, smoothing_distance,
                             minimum_island_radius](SupportGeneratorLayer& intermediate_layer, Polygons& bottom, Polygons&& top,
                                                    SupportGeneratorLayer* top_interface_layer, const Polygons* subtract,
                                                    SupporLayerType type) -> SupportGeneratorLayer* {
            bool has_top_interface = top_interface_layer && !top_interface_layer->polygons.empty();
            assert(!bottom.empty() || !top.empty() || has_top_interface);
            // Merge top into bottom, unite them with a safety offset.
            append(bottom, std::move(top));
            // Merge top / bottom interfaces. For snug supports, merge using closing distance and regularize (close concave corners).
            bottom = intersection(smooth_supports ? smooth_outward(closing(std::move(bottom), closing_distance + minimum_island_radius,
                                                                           closing_distance, SUPPORT_SURFACES_OFFSET_PARAMETERS),
                                                                   smoothing_distance) :
                                                    union_safety_offset(std::move(bottom)),
                                  intermediate_layer.polygons);
            if (has_top_interface) {
                // Don't trim the precomputed Organic supports top interface with base layer
                // as the precomputed top interface likely expands over multiple tree tips.
                bottom = union_(std::move(top_interface_layer->polygons), bottom);
                top_interface_layer->polygons.clear();
            }
            if (!bottom.empty()) {
                // FIXME Remove non-printable tiny islands, let them be printed using the base support.
                // bottom = opening(std::move(bottom), minimum_island_radius);
                if (!bottom.empty()) {
                    SupportGeneratorLayer& layer_new = top_interface_layer ? *top_interface_layer : layer_storage.allocate(type);
                    layer_new.polygons               = std::move(bottom);
                    layer_new.print_z                = intermediate_layer.print_z;
                    layer_new.bottom_z               = intermediate_layer.bottom_z;
                    layer_new.height                 = intermediate_layer.height;
                    layer_new.bridging               = intermediate_layer.bridging;
                    // Subtract the interface from the base regions.
                    intermediate_layer.polygons = diff(intermediate_layer.polygons, layer_new.polygons);
                    if (subtract)
                        // Trim the base interface layer with the interface layer.
                        layer_new.polygons = diff(std::move(layer_new.polygons), *subtract);
                    // FIXME filter layer_new.polygons islands by a minimum area?
                    //                  $interface_area = [ grep abs($_->area) >= $area_threshold, @$interface_area ];
                    return &layer_new;
                }
            }
            return nullptr;
        };
        tbb::parallel_for(tbb::blocked_range<int>(0, int(intermediate_layers.size())), [&bottom_contacts, &top_contacts,
                                                                                        &top_interface_layers, &top_base_interface_layers,
                                                                                        &intermediate_layers, &insert_layer,
                                                                                        &support_params, snug_supports, &interface_layers,
                                                                                        &base_interface_layers](
                                                                                           const tbb::blocked_range<int>& range) {
            // Gather the top / bottom contact layers intersecting with num_interface_layers resp. num_interface_layers_only intermediate layers
            // above / below this intermediate layer. Index of the first top contact layer intersecting the current intermediate layer.
            auto idx_top_contact_first = -1;
            // Index of the first bottom contact layer intersecting the current intermediate layer.
            auto idx_bottom_contact_first = -1;
            // Index of the first top interface layer intersecting the current intermediate layer.
            auto idx_top_interface_first = -1;
            // Index of the first top contact interface layer intersecting the current intermediate layer.
            auto idx_top_base_interface_first = -1;
            auto num_intermediate             = int(intermediate_layers.size());
            for (int idx_intermediate_layer = range.begin(); idx_intermediate_layer < range.end(); ++idx_intermediate_layer) {
                SupportGeneratorLayer& intermediate_layer = *intermediate_layers[idx_intermediate_layer];
                Polygons               polygons_top_contact_projected_interface;
                Polygons               polygons_top_contact_projected_base;
                Polygons               polygons_bottom_contact_projected_interface;
                Polygons               polygons_bottom_contact_projected_base;
                if (support_params.num_top_interface_layers > 0) {
                    // Top Z coordinate of a slab, over which we are collecting the top / bottom contact surfaces
                    coordf_t top_z = intermediate_layers[std::min(num_intermediate - 1,
                                                                  idx_intermediate_layer + int(support_params.num_top_interface_layers) - 1)]
                                         ->print_z;
                    coordf_t top_inteface_z = std::numeric_limits<coordf_t>::max();
                    if (support_params.num_top_base_interface_layers > 0)
                        // Some top base interface layers will be generated.
                        top_inteface_z = support_params.num_top_interface_layers_only() == 0 ?
                                             // Only base interface layers to generate.
                                             -std::numeric_limits<coordf_t>::max() :
                                             intermediate_layers[std::min(num_intermediate - 1,
                                                                          idx_intermediate_layer +
                                                                              int(support_params.num_top_interface_layers_only()) - 1)]
                                                 ->print_z;
                    // Move idx_top_contact_first up until above the current print_z.
                    idx_top_contact_first = idx_higher_or_equal(top_contacts, idx_top_contact_first,
                                                                [&intermediate_layer](const SupportGeneratorLayer* layer) {
                                                                    return layer->print_z >= intermediate_layer.print_z;
                                                                }); //  - EPSILON
                    // Collect the top contact areas above this intermediate layer, below top_z.
                    for (int idx_top_contact = idx_top_contact_first; idx_top_contact < int(top_contacts.size()); ++idx_top_contact) {
                        const SupportGeneratorLayer& top_contact_layer = *top_contacts[idx_top_contact];
                        // FIXME maybe this adds one interface layer in excess?
                        if (top_contact_layer.bottom_z - EPSILON > top_z)
                            break;
                        polygons_append(top_contact_layer.bottom_z - EPSILON > top_inteface_z ? polygons_top_contact_projected_base :
                                                                                                polygons_top_contact_projected_interface,
                                        // For snug supports, project the overhang polygons covering the whole overhang, so that they will
                                        // merge without a gap with support polygons of the other layers. For grid supports, merging of
                                        // support regions will be performed by the projection into grid.
                                        snug_supports ? *top_contact_layer.overhang_polygons : top_contact_layer.polygons);
                    }
                }
                if (support_params.num_bottom_interface_layers > 0) {
                    // Bottom Z coordinate of a slab, over which we are collecting the top / bottom contact surfaces
                    coordf_t bottom_z =
                        intermediate_layers[std::max(0, idx_intermediate_layer - int(support_params.num_bottom_interface_layers) + 1)]
                            ->bottom_z;
                    coordf_t bottom_interface_z = -std::numeric_limits<coordf_t>::max();
                    if (support_params.num_bottom_base_interface_layers > 0)
                        // Some bottom base interface layers will be generated.
                        bottom_interface_z = support_params.num_bottom_interface_layers_only() == 0 ?
                                                 // Only base interface layers to generate.
                                                 std::numeric_limits<coordf_t>::max() :
                                                 intermediate_layers[std::max(0, idx_intermediate_layer -
                                                                                     int(support_params.num_bottom_interface_layers_only()))]
                                                     ->bottom_z;
                    // Move idx_bottom_contact_first up until touching bottom_z.
                    idx_bottom_contact_first = idx_higher_or_equal(bottom_contacts, idx_bottom_contact_first,
                                                                   [bottom_z](const SupportGeneratorLayer* layer) {
                                                                       return layer->print_z >= bottom_z - EPSILON;
                                                                   });
                    // Collect the top contact areas above this intermediate layer, below top_z.
                    for (int idx_bottom_contact = idx_bottom_contact_first; idx_bottom_contact < int(bottom_contacts.size());
                         ++idx_bottom_contact) {
                        const SupportGeneratorLayer& bottom_contact_layer = *bottom_contacts[idx_bottom_contact];
                        if (bottom_contact_layer.print_z - EPSILON > intermediate_layer.bottom_z)
                            break;
                        polygons_append(bottom_contact_layer.print_z - EPSILON > bottom_interface_z ?
                                            polygons_bottom_contact_projected_interface :
                                            polygons_bottom_contact_projected_base,
                                        bottom_contact_layer.polygons);
                    }
                }
                auto resolve_same_layer = [](SupportGeneratorLayersPtr& layers, int& idx, coordf_t print_z) -> SupportGeneratorLayer* {
                    if (!layers.empty()) {
                        idx = idx_higher_or_equal(layers, idx, [print_z](const SupportGeneratorLayer* layer) {
                            return layer->print_z > print_z - EPSILON;
                        });
                        if (idx < int(layers.size()) && layers[idx]->print_z < print_z + EPSILON)
                            return layers[idx];
                    }
                    return nullptr;
                };
                SupportGeneratorLayer* top_interface_layer      = resolve_same_layer(top_interface_layers, idx_top_interface_first,
                                                                                     intermediate_layer.print_z);
                SupportGeneratorLayer* top_base_interface_layer = resolve_same_layer(top_base_interface_layers,
                                                                                     idx_top_base_interface_first,
                                                                                     intermediate_layer.print_z);
                SupportGeneratorLayer* interface_layer          = nullptr;
                if (!polygons_bottom_contact_projected_interface.empty() || !polygons_top_contact_projected_interface.empty() ||
                    (top_interface_layer && !top_interface_layer->polygons.empty())) {
                    interface_layer                          = insert_layer(intermediate_layer, polygons_bottom_contact_projected_interface,
                                                                            std::move(polygons_top_contact_projected_interface), top_interface_layer, nullptr,
                                                   polygons_top_contact_projected_interface.empty() ? sltBottomInterface : sltTopInterface);
                    interface_layers[idx_intermediate_layer] = interface_layer;
                }
                if (!polygons_bottom_contact_projected_base.empty() || !polygons_top_contact_projected_base.empty() ||
                    (top_base_interface_layer && !top_base_interface_layer->polygons.empty()))
                    base_interface_layers[idx_intermediate_layer] = insert_layer(intermediate_layer, polygons_bottom_contact_projected_base,
                                                                                 std::move(polygons_top_contact_projected_base),
                                                                                 top_base_interface_layer,
                                                                                 interface_layer ? &interface_layer->polygons : nullptr,
                                                                                 sltBase);
            }
        });

        // Compress contact_out, remove the nullptr items.
        // The parallel_for above may not have merged all the interface and base_interface layers
        // generated by the Organic supports code, do it here.
        auto merge_remove_empty = [](SupportGeneratorLayersPtr& in1, SupportGeneratorLayersPtr& in2) {
            auto remove_empty = [](SupportGeneratorLayersPtr& vec) {
                vec.erase(std::remove_if(vec.begin(), vec.end(),
                                         [](const SupportGeneratorLayer* ptr) { return ptr == nullptr || ptr->polygons.empty(); }),
                          vec.end());
            };
            remove_empty(in1);
            remove_empty(in2);
            if (in2.empty())
                return std::move(in1);
            else if (in1.empty())
                return std::move(in2);
            else {
                SupportGeneratorLayersPtr out(in1.size() + in2.size(), nullptr);
                std::merge(in1.begin(), in1.end(), in2.begin(), in2.end(), out.begin(),
                           [](auto* l, auto* r) { return l->print_z < r->print_z; });
                return out;
            }
        };
        interface_layers      = merge_remove_empty(interface_layers, top_interface_layers);
        base_interface_layers = merge_remove_empty(base_interface_layers, top_base_interface_layers);
        BOOST_LOG_TRIVIAL(debug) << "PrintObjectSupportMaterial::generate_interface_layers() in parallel - end";
    }

    return base_and_interface_layers;
}

SupportGeneratorLayersPtr generate_raft_base(const PrintObject&               object,
                                             const SupportParameters&         support_params,
                                             const SlicingParameters&         slicing_params,
                                             const SupportGeneratorLayersPtr& top_contacts,
                                             const SupportGeneratorLayersPtr& interface_layers,
                                             const SupportGeneratorLayersPtr& base_interface_layers,
                                             const SupportGeneratorLayersPtr& base_layers,
                                             SupportGeneratorLayerStorage&    layer_storage,
                                             std::vector<Polygons>&           buildplate_covered)
{
    // If there is brim to be generated, calculate the trimming regions.
    Polygons brim;
    if (object.has_brim()) {
        // The object does not have a raft.
        // Calculate the area covered by the brim.
        const BrimType brim_type  = object.config().brim_type;
        const bool     brim_outer = brim_type == btOuterOnly || brim_type == btOuterAndInner;
        const bool     brim_inner = brim_type == btInnerOnly || brim_type == btOuterAndInner;
        // BBS: the pattern of raft and brim are the same, thus the brim can be serpated by support raft.
        const auto brim_object_gap = scaled<float>(object.config().brim_object_gap.value);
        // const auto     brim_object_gap = scaled<float>(object.config().brim_object_gap.value + object.config().brim_width.value);
        for (const ExPolygon& ex : object.layers().front()->lslices) {
            if (brim_outer && brim_inner)
                polygons_append(brim, offset(ex, brim_object_gap));
            else {
                if (brim_outer)
                    polygons_append(brim, offset(ex.contour, brim_object_gap, ClipperLib::jtRound, float(scale_(0.1))));
                else
                    brim.emplace_back(ex.contour);
                if (brim_inner) {
                    Polygons holes = ex.holes;
                    polygons_reverse(holes);
                    holes = shrink(holes, brim_object_gap, ClipperLib::jtRound, float(scale_(0.1)));
                    polygons_reverse(holes);
                    polygons_append(brim, std::move(holes));
                } else
                    polygons_append(brim, ex.holes);
            }
        }
        brim = union_(brim);
    }

    // How much to inflate the support columns to be stable. This also applies to the 1st layer, if no raft layers are to be printed.
    const float            inflate_factor_fine       = float(scale_((slicing_params.raft_layers() > 1) ? 0.5 : EPSILON));
    const float            inflate_factor_1st_layer  = std::max(0.f,
                                                                float(scale_(object.config().raft_first_layer_expansion)) - inflate_factor_fine);
    SupportGeneratorLayer* contacts                  = top_contacts.empty() ? nullptr : top_contacts.front();
    SupportGeneratorLayer* interfaces                = interface_layers.empty() ? nullptr : interface_layers.front();
    SupportGeneratorLayer* base_interfaces           = base_interface_layers.empty() ? nullptr : base_interface_layers.front();
    SupportGeneratorLayer* columns_base              = base_layers.empty() ? nullptr : base_layers.front();
    if (contacts != nullptr &&
        contacts->print_z > std::max(slicing_params.first_print_layer_height, slicing_params.raft_contact_top_z) + EPSILON)
        // This is not the raft contact layer.
        contacts = nullptr;
    if (interfaces != nullptr && interfaces->bottom_print_z() > slicing_params.raft_interface_top_z + EPSILON)
        // This is not the raft column base layer.
        interfaces = nullptr;
    if (base_interfaces != nullptr && base_interfaces->bottom_print_z() > slicing_params.raft_interface_top_z + EPSILON)
        // This is not the raft column base layer.
        base_interfaces = nullptr;
    if (columns_base != nullptr && columns_base->bottom_print_z() > slicing_params.raft_interface_top_z + EPSILON)
        // This is not the raft interface layer.
        columns_base = nullptr;

    Polygons interface_polygons;
    if (contacts != nullptr && !contacts->polygons.empty())
        polygons_append(interface_polygons, expand(contacts->polygons, inflate_factor_fine, SUPPORT_SURFACES_OFFSET_PARAMETERS));
    if (interfaces != nullptr && !interfaces->polygons.empty())
        polygons_append(interface_polygons, expand(interfaces->polygons, inflate_factor_fine, SUPPORT_SURFACES_OFFSET_PARAMETERS));
    if (base_interfaces != nullptr && !base_interfaces->polygons.empty())
        polygons_append(interface_polygons, expand(base_interfaces->polygons, inflate_factor_fine, SUPPORT_SURFACES_OFFSET_PARAMETERS));

    // Output vector.
    SupportGeneratorLayersPtr raft_layers;

    if (slicing_params.raft_layers() > 1) {
        Polygons base;
        Polygons columns;
        Polygons first_layer;
        if (columns_base != nullptr) {
            if (columns_base->bottom_print_z() > slicing_params.raft_interface_top_z - EPSILON) {
                // Classic supports with colums above the raft interface.
                base    = columns_base->polygons;
                columns = base;
                if (!interface_polygons.empty())
                    // Trim the 1st layer columns with the inflated interface polygons.
                    columns = diff(columns, interface_polygons);
            } else {
                // Organic supports with raft on print bed.
                assert(is_approx(columns_base->print_z, slicing_params.first_print_layer_height));
                first_layer = columns_base->polygons;
            }
        }
        if (!interface_polygons.empty()) {
            // Merge the untrimmed columns base with the expanded raft interface, to be used for the support base and interface.
            base = union_(base, interface_polygons);
        }
        // Do not add the raft contact layer, only add the raft layers below the contact layer.
        // Insert the 1st layer.
        {
            SupportGeneratorLayer& new_layer = layer_storage.allocate((slicing_params.base_raft_layers > 0) ? sltRaftBase :
                                                                                                              sltRaftInterface);
            raft_layers.push_back(&new_layer);
            new_layer.print_z  = slicing_params.first_print_layer_height;
            new_layer.height   = slicing_params.first_print_layer_height;
            new_layer.bottom_z = 0.;
            first_layer        = union_(std::move(first_layer), base);
            new_layer.polygons = inflate_factor_1st_layer > 0 ? expand(first_layer, inflate_factor_1st_layer) : first_layer;
        }
        // Insert the base layers.
        for (size_t i = 1; i < slicing_params.base_raft_layers; ++i) {
            coordf_t               print_z   = raft_layers.back()->print_z;
            SupportGeneratorLayer& new_layer = layer_storage.allocate_unguarded(SupporLayerType::sltRaftBase);
            raft_layers.push_back(&new_layer);
            new_layer.print_z  = print_z + slicing_params.base_raft_layer_height;
            new_layer.height   = slicing_params.base_raft_layer_height;
            new_layer.bottom_z = print_z;
            new_layer.polygons = base;
        }
        // Insert the interface layers.
        for (size_t i = 1; i < slicing_params.interface_raft_layers; ++i) {
            coordf_t               print_z   = raft_layers.back()->print_z;
            SupportGeneratorLayer& new_layer = layer_storage.allocate_unguarded(SupporLayerType::sltRaftInterface);
            raft_layers.push_back(&new_layer);
            new_layer.print_z  = print_z + slicing_params.interface_raft_layer_height;
            new_layer.height   = slicing_params.interface_raft_layer_height;
            new_layer.bottom_z = print_z;
            new_layer.polygons = interface_polygons;
            // FIXME misusing contact_polygons for support columns.
            new_layer.contact_polygons = std::make_unique<Polygons>(columns);
        }
    } else {
        if (columns_base != nullptr) {
            // Expand the bases of the support columns in the 1st layer.
            Polygons& raft = columns_base->polygons;
            Polygons  trimming;
            // BBS: if first layer of support is intersected with object island, it must have the same function as brim unless in nobrim
            // mode. brim_object_gap is changed to 0 by default, it's no longer appropriate to use it to determine the gap of first layer
            // support. if (object.has_brim())
            //    trimming = offset(object.layers().front()->lslices, (float)scale_(object.config().brim_object_gap.value),
            //    SUPPORT_SURFACES_OFFSET_PARAMETERS);
            // else
            trimming = offset(object.layers().front()->lslices, (float) scale_(support_params.gap_xy_first_layer),
                              SUPPORT_SURFACES_OFFSET_PARAMETERS);
            // todo : The support is out of the platform issue
            // if (!buildplate_covered.empty())
            //{
            //     append(trimming, buildplate_covered[0]);
            //     buildplate_covered.clear();
            // }
            if (inflate_factor_1st_layer > SCALED_EPSILON) {
                // Inflate in multiple steps to avoid leaking of the support 1st layer through object walls.
                auto  nsteps = std::max(5, int(ceil(inflate_factor_1st_layer / support_params.first_layer_flow.scaled_width())));
                float step   = inflate_factor_1st_layer / nsteps;
                for (int i = 0; i < nsteps; ++i)
                    raft = diff(expand(raft, step), trimming);
            } else
                raft = diff(raft, trimming);
            if (!interface_polygons.empty())
                columns_base->polygons = diff(columns_base->polygons, interface_polygons);
        }
        if (!brim.empty()) {
            if (columns_base)
                columns_base->polygons = diff(columns_base->polygons, brim);
            if (contacts)
                contacts->polygons = diff(contacts->polygons, brim);
            if (interfaces)
                interfaces->polygons = diff(interfaces->polygons, brim);
            if (base_interfaces)
                base_interfaces->polygons = diff(base_interfaces->polygons, brim);
        }
    }

    /* if (object.print()) {
         Polygons bed_shape_polygons;
         const auto& pa = object.print()->config().printable_area.values;
         if (!pa.empty()) {
             BoundingBoxf bboxf(pa);
             Polygon bed_poly;
             bed_poly.points.emplace_back(scale_(bboxf.min.x()), scale_(bboxf.min.y()));
             bed_poly.points.emplace_back(scale_(bboxf.max.x()), scale_(bboxf.min.y()));
             bed_poly.points.emplace_back(scale_(bboxf.max.x()), scale_(bboxf.max.y()));
             bed_poly.points.emplace_back(scale_(bboxf.min.x()), scale_(bboxf.max.y()));

             if (!object.instances().empty()) {
                 Point shift = object.instances().front().shift;
                 bed_poly.translate(-shift.x(), -shift.y());
             }
             bed_shape_polygons.push_back(std::move(bed_poly));

             if (slicing_params.raft_layers() > 1) {
                 if (!raft_layers.empty()) {
                     SupportGeneratorLayer* first_raft_layer = raft_layers.front();
                     if (first_raft_layer && !first_raft_layer->polygons.empty()) {
                         first_raft_layer->polygons = intersection(first_raft_layer->polygons, bed_shape_polygons);
                     }
                 }
             }
             else {
                 if (columns_base != nullptr && !columns_base->polygons.empty()) {
                     columns_base->polygons = intersection(columns_base->polygons, bed_shape_polygons);
                 }
             }
         }
     }*/

    return raft_layers;
}

static inline void fill_expolygon_generate_paths(ExtrusionEntitiesPtr& dst,
                                                 ExPolygon&&           expolygon,
                                                 Fill*                 filler,
                                                 const FillParams&     fill_params,
                                                 float                 density,
                                                 ExtrusionRole         role,
                                                 const Flow&           flow)
{
    Surface   surface(stInternal, std::move(expolygon));
    Polylines polylines;
    try {
        assert(!fill_params.use_arachne);
        polylines = filler->fill_surface(&surface, fill_params);
    } catch (InfillFailedException&) {}
    extrusion_entities_append_paths(dst, std::move(polylines), role, flow.mm3_per_mm(), flow.width(), flow.height());
}

static inline void fill_expolygons_generate_paths(ExtrusionEntitiesPtr& dst,
                                                  ExPolygons&&          expolygons,
                                                  Fill*                 filler,
                                                  const FillParams&     fill_params,
                                                  float                 density,
                                                  ExtrusionRole         role,
                                                  const Flow&           flow)
{
    for (ExPolygon& expoly : expolygons)
        fill_expolygon_generate_paths(dst, std::move(expoly), filler, fill_params, density, role, flow);
}

static inline void fill_expolygons_generate_paths(ExtrusionEntitiesPtr&                  dst,
                                                  ExPolygons&&                           expolygons,
                                                  Fill*                                  filler,
                                                  float                                  density,
                                                  ExtrusionRole                          role,
                                                  const Flow&                            flow,
                                                  const SupportMaterialInterfacePattern& interface_pattern = smipAuto)
{
    FillParams fill_params;
    fill_params.density     = density;
    fill_params.dont_adjust = true;
    if (role == ExtrusionRole ::erSupportMaterialInterface && interface_pattern == smipMonotonicLine) {
        fill_params.monotonic = true;
        fill_params.dont_sort = false;
    }
    fill_expolygons_generate_paths(dst, std::move(expolygons), filler, fill_params, density, role, flow);
}

static inline void fill_expolygon_generate_paths_ironing(
    ExtrusionEntitiesPtr& dst, ExPolygons&& expolygons, Fill* filler, const FillParams& fill_params, ExtrusionRole role, const Flow& flow)
{
    for (ExPolygon& expolygon : expolygons) {
        Surface   surface(stInternal, std::move(expolygon));
        Polylines polylines;
        try {
            polylines = filler->fill_surface(&surface, fill_params);
        } catch (InfillFailedException&) {}

        if (!polylines.empty()) {
            ExtrusionEntityCollection* eec = nullptr;
            dst.push_back(eec = new ExtrusionEntityCollection());
            double flow_mm3_per_mm  = flow.width() * flow.height();
            float  extrusion_width  = flow.width();
            double extrusion_height = flow.height();
            eec->no_sort            = true;
            extrusion_entities_append_paths(eec->entities, std::move(polylines), erIroning, flow_mm3_per_mm, extrusion_width,
                                            float(extrusion_height));
        }
    }
}

static Polylines draw_perimeters(const ExPolygon& expoly, double clip_length, bool include_holes = true)
{
    // Draw the perimeters.
    Polylines polylines;
    const size_t num_contours = include_holes ? expoly.holes.size() + 1 : 1;
    polylines.reserve(num_contours);
    for (size_t i = 0; i < num_contours; ++i) {
        Polyline pl(i == 0 ? expoly.contour.points : expoly.holes[i - 1].points);
        pl.points.emplace_back(pl.points.front());
        if (i > 0)
            // It is a hole, reverse it.
            pl.reverse();
        // so that all contours are CCW oriented.
        pl.clip_end(clip_length);
        polylines.emplace_back(std::move(pl));
    }
    return polylines;
}

void tree_supports_generate_paths(ExtrusionEntitiesPtr&    dst,
                                  const Polygons&          polygons,
                                  const Flow&              flow,
                                  const SupportParameters& support_params)
{
    // Offset expolygon inside, returns number of expolygons collected (0 or 1).
    // Vertices of output paths are marked with Z = source contour index of the expoly.
    // Vertices at the intersection of source contours are marked with Z = -1.
    auto shrink_expolygon_with_contour_idx = [](const Slic3r::ExPolygon& expoly, const float delta, ClipperLib::JoinType joinType,
                                                double miterLimit, ClipperLib_Z::Paths& out) -> int {
        assert(delta > 0);
        auto append_paths_with_z = [](ClipperLib::Paths& src, coord_t contour_idx, ClipperLib_Z::Paths& dst) {
            dst.reserve(next_highest_power_of_2(dst.size() + src.size()));
            for (const ClipperLib::Path& contour : src) {
                ClipperLib_Z::Path tmp;
                tmp.reserve(contour.size());
                for (const Point& p : contour)
                    tmp.emplace_back(p.x(), p.y(), contour_idx);
                dst.emplace_back(std::move(tmp));
            }
        };

        // 1) Offset the outer contour.
        ClipperLib_Z::Paths contours;
        {
            ClipperLib::ClipperOffset co;
            if (joinType == jtRound)
                co.ArcTolerance = miterLimit;
            else
                co.MiterLimit = miterLimit;
            co.ShortestEdgeLength = double(delta * 0.005);
            co.AddPath(expoly.contour.points, joinType, ClipperLib::etClosedPolygon);
            ClipperLib::Paths contours_raw;
            co.Execute(contours_raw, -delta);
            if (contours_raw.empty())
                // No need to try to offset the holes.
                return 0;
            append_paths_with_z(contours_raw, 0, contours);
        }

        if (expoly.holes.empty()) {
            // No need to subtract holes from the offsetted expolygon, we are done.
            append(out, std::move(contours));
        } else {
            // 2) Offset the holes one by one, collect the offsetted holes.
            ClipperLib_Z::Paths holes;
            {
                for (const Polygon& hole : expoly.holes) {
                    ClipperLib::ClipperOffset co;
                    if (joinType == jtRound)
                        co.ArcTolerance = miterLimit;
                    else
                        co.MiterLimit = miterLimit;
                    co.ShortestEdgeLength = double(delta * 0.005);
                    co.AddPath(hole.points, joinType, ClipperLib::etClosedPolygon);
                    ClipperLib::Paths out2;
                    // Execute reorients the contours so that the outer most contour has a positive area. Thus the output
                    // contours will be CCW oriented even though the input paths are CW oriented.
                    // Offset is applied after contour reorientation, thus the signum of the offset value is reversed.
                    co.Execute(out2, delta);
                    append_paths_with_z(out2, 1 + (&hole - expoly.holes.data()), holes);
                }
            }

            // 3) Subtract holes from the contours.
            if (holes.empty()) {
                // No hole remaining after an offset. Just copy the outer contour.
                append(out, std::move(contours));
            } else {
                // Negative offset. There is a chance, that the offsetted hole intersects the outer contour.
                // Subtract the offsetted holes from the offsetted contours.
                ClipperLib_Z::Clipper clipper;
                clipper.ZFillFunction([](const ClipperLib_Z::IntPoint& e1bot, const ClipperLib_Z::IntPoint& e1top,
                                         const ClipperLib_Z::IntPoint& e2bot, const ClipperLib_Z::IntPoint& e2top,
                                         ClipperLib_Z::IntPoint& pt) {
                    // pt.z() = std::max(std::max(e1bot.z(), e1top.z()), std::max(e2bot.z(), e2top.z()));
                    //  Just mark the intersection.
                    pt.z() = -1;
                });
                clipper.AddPaths(contours, ClipperLib_Z::ptSubject, true);
                clipper.AddPaths(holes, ClipperLib_Z::ptClip, true);
                ClipperLib_Z::Paths output;
                clipper.Execute(ClipperLib_Z::ctDifference, output, ClipperLib_Z::pftNonZero, ClipperLib_Z::pftNonZero);
                if (!output.empty()) {
                    append(out, std::move(output));
                } else {
                    // The offsetted holes have eaten up the offsetted outer contour.
                    return 0;
                }
            }
        }

        return 1;
    };

    const double spacing = flow.scaled_spacing();
    // Clip the sheath path to avoid the extruder to get exactly on the first point of the loop.
    const double        clip_length   = spacing * 0.15;
    // Disable the seam anchor by default: it creates visible protruding scars on organic tree walls.
    const double        anchor_length = spacing * 6.;
    constexpr bool      emit_inner_tree_walls = true;
    ClipperLib_Z::Paths anchor_candidates;
    for (ExPolygon& expoly : closing_ex(polygons, float(SCALED_EPSILON), float(SCALED_EPSILON + 0.5 * flow.scaled_width()))) {
        std::unique_ptr<ExtrusionEntityCollection> eec;
        ExPolygons                                 regions_to_draw{expoly};
        if (emit_inner_tree_walls && support_params.tree_branch_diameter_double_wall_area_scaled > 0)
            if (double area = expoly.area(); area > support_params.tree_branch_diameter_double_wall_area_scaled) {
                BOOST_LOG_TRIVIAL(debug) << "TreeSupports: double wall area: " << area << " > "
                                         << support_params.tree_branch_diameter_double_wall_area_scaled;
                eec = std::make_unique<ExtrusionEntityCollection>();
                // Don't reorder internal / external loops of the same island, always start with the internal loop.
                eec->no_sort = true;
                // Make the tree branch stable by adding another perimeter.
                ExPolygons level2 = offset2_ex({expoly}, -1.5 * flow.scaled_width(), 0.5 * flow.scaled_width());
                if (level2.size() > 0) {
                    regions_to_draw = level2;
                    extrusion_entities_append_paths(eec->entities, draw_perimeters(expoly, clip_length, false),
                                                    ExtrusionRole::erSupportMaterial,
                                                    flow.mm3_per_mm(), flow.width(), flow.height(),
                                                    // Disable reversal of the path, always start with the anchor, always print CCW.
                                                    false);
                    expoly = level2.front();
                }
            }
        for (ExPolygon& expoly : regions_to_draw) {
            // Try to produce one more perimeter to place the seam anchor.
            // First genrate a 2nd perimeter loop as a source for anchor candidates.
            // The anchor candidate points are annotated with an index of the source contour or with -1 if on intersection.
            anchor_candidates.clear();
            shrink_expolygon_with_contour_idx(expoly, flow.scaled_width(), DefaultJoinType, 1.2, anchor_candidates);
            // Orient all contours CW.
            for (auto& path : anchor_candidates)
                if (ClipperLib_Z::Area(path) > 0)
                    std::reverse(path.begin(), path.end());

            // Draw the perimeters.
            Polylines polylines;
            polylines.reserve(1);
            for (int idx_loop = 0; idx_loop < std::min<int>(1, int(expoly.num_contours())); ++idx_loop) {
                // Open the loop with a seam.
                const Polygon& loop = expoly.contour_or_hole(idx_loop);
                Polyline       pl(loop.points);
                // Orient all contours CW, because the anchor will be added to the end of polyline while we want to start a loop with the anchor.
                if (idx_loop == 0)
                    // It is an outer contour.
                    pl.reverse();
                pl.points.emplace_back(pl.points.front());
                pl.clip_end(clip_length);
                if (pl.size() < 2)
                    continue;
                // Find the foot of the seam point on anchor_candidates. Only pick an anchor point that was created by offsetting the source contour.
                ClipperLib_Z::Path* closest_contour = nullptr;
                Vec2d               closest_point;
                int                 closest_point_idx = -1;
                double              closest_point_t   = 0.;
                double              d2min             = std::numeric_limits<double>::max();
                Vec2d               seam_pt           = pl.back().cast<double>();
                for (ClipperLib_Z::Path& path : anchor_candidates)
                    for (int i = 0; i < int(path.size()); ++i) {
                        int j = next_idx_modulo(i, path);
                        if (path[i].z() == idx_loop || path[j].z() == idx_loop) {
                            Vec2d pi(path[i].x(), path[i].y());
                            Vec2d pj(path[j].x(), path[j].y());
                            Vec2d v  = pj - pi;
                            Vec2d w  = seam_pt - pi;
                            auto  l2 = v.squaredNorm();
                            auto  t  = std::clamp((l2 == 0) ? 0 : v.dot(w) / l2, 0., 1.);
                            if ((path[i].z() == idx_loop || t > EPSILON) && (path[j].z() == idx_loop || t < 1. - EPSILON)) {
                                // Closest point.
                                Vec2d  fp = pi + v * t;
                                double d2 = (fp - seam_pt).squaredNorm();
                                if (d2 < d2min) {
                                    d2min             = d2;
                                    closest_contour   = &path;
                                    closest_point     = fp;
                                    closest_point_idx = i;
                                    closest_point_t   = t;
                                }
                            }
                        }
                    }
                if (anchor_length > 0. && d2min < sqr(flow.scaled_width() * 3.)) {
                    // Try to cut an anchor from the closest_contour.
                    // Both closest_contour and pl are CW oriented.
                    pl.points.emplace_back(closest_point.cast<coord_t>());
                    const ClipperLib_Z::Path& path             = *closest_contour;
                    double                    remaining_length = anchor_length - (seam_pt - closest_point).norm();
                    int                       i                = closest_point_idx;
                    int                       j                = next_idx_modulo(i, *closest_contour);
                    Vec2d                     pi(path[i].x(), path[i].y());
                    Vec2d                     pj(path[j].x(), path[j].y());
                    Vec2d                     v = pj - pi;
                    double                    l = v.norm();
                    if (remaining_length < (1. - closest_point_t) * l) {
                        // Just trim the current line.
                        pl.points.emplace_back((closest_point + v * (remaining_length / l)).cast<coord_t>());
                    } else {
                        // Take the rest of the current line, continue with the other lines.
                        pl.points.emplace_back(path[j].x(), path[j].y());
                        pi = pj;
                        for (i = j; path[i].z() == idx_loop && remaining_length > 0; i = j, pi = pj) {
                            j  = next_idx_modulo(i, path);
                            pj = Vec2d(path[j].x(), path[j].y());
                            v  = pj - pi;
                            l  = v.norm();
                            if (i == closest_point_idx) {
                                // Back at the first segment. Most likely this should not happen and we may end the anchor.
                                break;
                            }
                            if (remaining_length <= l) {
                                pl.points.emplace_back((pi + v * (remaining_length / l)).cast<coord_t>());
                                break;
                            }
                            pl.points.emplace_back(path[j].x(), path[j].y());
                            remaining_length -= l;
                        }
                    }
                }
                // Keep path orientation stable; when enabled, the optional anchor stays at the start.
                pl.reverse();
                polylines.emplace_back(std::move(pl));
            }

            ExtrusionEntitiesPtr& out = eec ? eec->entities : dst;
            extrusion_entities_append_paths(out, std::move(polylines), ExtrusionRole::erSupportMaterial, flow.mm3_per_mm(), flow.width(),
                                            flow.height(),
                                            // Disable reversal of the path to keep organic wall seams deterministic.
                                            false);
        }
        if (eec) {
            std::reverse(eec->entities.begin(), eec->entities.end());
            if (!eec->entities.empty())
                dst.emplace_back(eec.release());
        }
    }
}

static void append_outer_contours_for_support_plan(Polylines& dst, const Polygons& polygons)
{
    for (const ExPolygon& expoly : union_ex(polygons)) {
        if (expoly.contour.empty())
            continue;
        Polyline pl(expoly.contour.points);
        pl.points.emplace_back(pl.points.front());
        dst.emplace_back(std::move(pl));
    }
}

static void append_centerline_contours_for_support_plan(Polylines& dst, const Polygons& polygons, coord_t line_width)
{
    if (polygons.empty())
        return;

    Polygons centerline_regions;
    if (line_width > 0)
        centerline_regions = offset(polygons, -0.5f * float(line_width));

    // If an island cannot survive the half-line-width inset, it cannot provide a
    // stable centerline for this contour support propagation pass.
    if (centerline_regions.empty())
        return;

    append_outer_contours_for_support_plan(dst, centerline_regions);
}

static void filter_short_contour_support_paths(Polylines& polylines, double min_length)
{
    polylines.erase(std::remove_if(polylines.begin(), polylines.end(),
                                   [min_length](const Polyline& pl) { return pl.size() < 2 || pl.length() < min_length; }),
                    polylines.end());
}

static void remove_overlapping_contour_support_paths(Polylines& polylines, coord_t overlap_distance)
{
    if (polylines.size() < 2 || overlap_distance <= 0)
        return;

    std::sort(polylines.begin(), polylines.end(),
              [](const Polyline& a, const Polyline& b) { return a.length() > b.length(); });

    struct SegmentBox {
        Point   a;
        Point   b;
        coord_t min_x {0};
        coord_t min_y {0};
        coord_t max_x {0};
        coord_t max_y {0};
        size_t  source_segment_idx {size_t(-1)};
    };

    auto make_segment_box = [overlap_distance](const Point& a, const Point& b, size_t source_segment_idx = size_t(-1)) {
        SegmentBox box;
        box.a     = a;
        box.b     = b;
        box.min_x = std::min(a.x(), b.x()) - overlap_distance;
        box.min_y = std::min(a.y(), b.y()) - overlap_distance;
        box.max_x = std::max(a.x(), b.x()) + overlap_distance;
        box.max_y = std::max(a.y(), b.y()) + overlap_distance;
        box.source_segment_idx = source_segment_idx;
        return box;
    };

    auto boxes_overlap = [](coord_t min_x, coord_t min_y, coord_t max_x, coord_t max_y, const SegmentBox& box) {
        return !(max_x < box.min_x || min_x > box.max_x || max_y < box.min_y || min_y > box.max_y);
    };

    auto segment_distance2 = [](const Point& p1, const Point& q1, const Point& p2, const Point& q2, double& s, double& t) {
        const Vec2d d1 = q1.cast<double>() - p1.cast<double>();
        const Vec2d d2 = q2.cast<double>() - p2.cast<double>();
        const Vec2d r  = p1.cast<double>() - p2.cast<double>();
        const double a = d1.squaredNorm();
        const double e = d2.squaredNorm();
        const double f = d2.dot(r);
        constexpr double eps = 1e-12;

        if (a <= eps && e <= eps) {
            s = 0.;
            t = 0.;
            return r.squaredNorm();
        }
        if (a <= eps) {
            s = 0.;
            t = std::clamp(f / e, 0., 1.);
        } else {
            const double c = d1.dot(r);
            if (e <= eps) {
                t = 0.;
                s = std::clamp(-c / a, 0., 1.);
            } else {
                const double b = d1.dot(d2);
                const double denom = a * e - b * b;
                s = denom != 0. ? std::clamp((b * f - c * e) / denom, 0., 1.) : 0.;
                t = (b * s + f) / e;
                if (t < 0.) {
                    t = 0.;
                    s = std::clamp(-c / a, 0., 1.);
                } else if (t > 1.) {
                    t = 1.;
                    s = std::clamp((b - c) / a, 0., 1.);
                }
            }
        }

        const Vec2d c1 = p1.cast<double>() + d1 * s;
        const Vec2d c2 = p2.cast<double>() + d2 * t;
        return (c1 - c2).squaredNorm();
    };

    auto cut_interval_near_segment = [overlap_distance, &segment_distance2](const Point& a,
                                                                            const Point& b,
                                                                            const SegmentBox& box,
                                                                            double& cut_begin,
                                                                            double& cut_end) {
        double closest_on_candidate = 0.;
        double closest_on_kept      = 0.;
        if (segment_distance2(a, b, box.a, box.b, closest_on_candidate, closest_on_kept) >
            sqr(double(overlap_distance)))
            return false;

        const Vec2d ab   = b.cast<double>() - a.cast<double>();
        const double len2 = ab.squaredNorm();
        if (len2 <= 1e-12)
            return false;

        const double inv_len2 = 1. / len2;
        const double kept_a_t = (box.a.cast<double>() - a.cast<double>()).dot(ab) * inv_len2;
        const double kept_b_t = (box.b.cast<double>() - a.cast<double>()).dot(ab) * inv_len2;
        const double half_t   = double(overlap_distance) / std::sqrt(len2);

        cut_begin = std::min(kept_a_t, kept_b_t);
        cut_end   = std::max(kept_a_t, kept_b_t);

        if (cut_end < 0. || cut_begin > 1. || cut_end - cut_begin < half_t) {
            cut_begin = closest_on_candidate - half_t;
            cut_end   = closest_on_candidate + half_t;
        } else {
            cut_begin -= half_t;
            cut_end   += half_t;
        }

        cut_begin = std::clamp(cut_begin, 0., 1.);
        cut_end   = std::clamp(cut_end,   0., 1.);
        return cut_end > cut_begin;
    };

    auto point_at_t = [](const Point& a, const Point& b, double t) {
        const Vec2d p = a.cast<double>() + std::clamp(t, 0., 1.) * (b - a).cast<double>();
        return Point(coord_t(std::llround(p.x())), coord_t(std::llround(p.y())));
    };

    std::vector<SegmentBox> kept_segments;
    Polylines kept;
    for (Polyline& polyline : polylines) {
        if (polyline.points.size() < 2)
            continue;

        Polylines              clipped;
        std::vector<SegmentBox> clipped_segments;
        Polyline               current;
        auto flush_current = [&current, &clipped]() {
            if (current.points.size() >= 2)
                clipped.emplace_back(std::move(current));
            current = Polyline{};
        };

        for (size_t point_idx = 1; point_idx < polyline.points.size(); ++point_idx) {
            const Point& a = polyline.points[point_idx - 1];
            const Point& b = polyline.points[point_idx];
            if (a == b)
                continue;

            const coord_t seg_min_x = std::min(a.x(), b.x());
            const coord_t seg_min_y = std::min(a.y(), b.y());
            const coord_t seg_max_x = std::max(a.x(), b.x());
            const coord_t seg_max_y = std::max(a.y(), b.y());

            std::vector<std::pair<double, double>> remaining_intervals {{0., 1.}};
            for (const SegmentBox& box : kept_segments) {
                if (!boxes_overlap(seg_min_x, seg_min_y, seg_max_x, seg_max_y, box))
                    continue;

                double cut_begin = 0.;
                double cut_end   = 1.;
                if (!cut_interval_near_segment(a, b, box, cut_begin, cut_end))
                    continue;

                std::vector<std::pair<double, double>> next_intervals;
                next_intervals.reserve(remaining_intervals.size() + 1);
                for (const auto& interval : remaining_intervals) {
                    if (cut_end <= interval.first || cut_begin >= interval.second) {
                        next_intervals.emplace_back(interval);
                        continue;
                    }
                    if (cut_begin > interval.first)
                        next_intervals.emplace_back(interval.first, cut_begin);
                    if (cut_end < interval.second)
                        next_intervals.emplace_back(cut_end, interval.second);
                }
                remaining_intervals = std::move(next_intervals);
                if (remaining_intervals.empty())
                    break;
            }
            for (size_t box_idx = 0; box_idx < clipped_segments.size() && !remaining_intervals.empty(); ++box_idx) {
                const SegmentBox& box = clipped_segments[box_idx];
                const size_t current_segment_idx = point_idx;
                if (box_idx + 1 == clipped_segments.size() && box.b == a)
                    continue;
                if (box.a == a || box.a == b || box.b == a || box.b == b)
                    continue;
                if (box.source_segment_idx != size_t(-1) &&
                    std::max(box.source_segment_idx, current_segment_idx) -
                    std::min(box.source_segment_idx, current_segment_idx) <= 3)
                    continue;
                if (!boxes_overlap(seg_min_x, seg_min_y, seg_max_x, seg_max_y, box))
                    continue;

                double cut_begin = 0.;
                double cut_end   = 1.;
                if (!cut_interval_near_segment(a, b, box, cut_begin, cut_end))
                    continue;

                std::vector<std::pair<double, double>> next_intervals;
                next_intervals.reserve(remaining_intervals.size() + 1);
                for (const auto& interval : remaining_intervals) {
                    if (cut_end <= interval.first || cut_begin >= interval.second) {
                        next_intervals.emplace_back(interval);
                        continue;
                    }
                    if (cut_begin > interval.first)
                        next_intervals.emplace_back(interval.first, cut_begin);
                    if (cut_end < interval.second)
                        next_intervals.emplace_back(cut_end, interval.second);
                }
                remaining_intervals = std::move(next_intervals);
            }

            if (remaining_intervals.empty()) {
                flush_current();
                continue;
            }

            for (const auto& interval : remaining_intervals) {
                if (interval.second - interval.first <= 1e-6)
                    continue;

                const Point out_a = point_at_t(a, b, interval.first);
                const Point out_b = point_at_t(a, b, interval.second);
                if (out_a == out_b)
                    continue;

                clipped_segments.emplace_back(make_segment_box(out_a, out_b, point_idx));
                if (interval.first > 1e-6)
                    flush_current();
                if (current.points.empty()) {
                    current.points.emplace_back(out_a);
                    current.points.emplace_back(out_b);
                } else if (current.points.back() == out_a) {
                    current.points.emplace_back(out_b);
                } else {
                    flush_current();
                    current.points.emplace_back(out_a);
                    current.points.emplace_back(out_b);
                }
                if (interval.second < 1. - 1e-6)
                    flush_current();
            }
        }
        flush_current();

        if (clipped.empty())
            continue;

        kept_segments.insert(kept_segments.end(), clipped_segments.begin(), clipped_segments.end());
        append(kept, std::move(clipped));
    }
    polylines = std::move(kept);
}

static double contour_support_anchor_segment_distance2(const Point& p1,
                                                       const Point& q1,
                                                       const Point& p2,
                                                       const Point& q2)
{
    const Vec2d d1 = q1.cast<double>() - p1.cast<double>();
    const Vec2d d2 = q2.cast<double>() - p2.cast<double>();
    const Vec2d r  = p1.cast<double>() - p2.cast<double>();
    const double a = d1.squaredNorm();
    const double e = d2.squaredNorm();
    const double f = d2.dot(r);
    constexpr double eps = 1e-12;

    double s = 0.;
    double t = 0.;
    if (a <= eps && e <= eps) {
        return r.squaredNorm();
    } else if (a <= eps) {
        t = std::clamp(f / e, 0., 1.);
    } else {
        const double c = d1.dot(r);
        if (e <= eps) {
            s = std::clamp(-c / a, 0., 1.);
        } else {
            const double b = d1.dot(d2);
            const double denom = a * e - b * b;
            s = denom != 0. ? std::clamp((b * f - c * e) / denom, 0., 1.) : 0.;
            t = (b * s + f) / e;
            if (t < 0.) {
                t = 0.;
                s = std::clamp(-c / a, 0., 1.);
            } else if (t > 1.) {
                t = 1.;
                s = std::clamp((b - c) / a, 0., 1.);
            }
        }
    }

    const Vec2d c1 = p1.cast<double>() + d1 * s;
    const Vec2d c2 = p2.cast<double>() + d2 * t;
    return (c1 - c2).squaredNorm();
}

static bool contour_support_anchor_paths_connected(const Polyline& lhs,
                                                   const Polyline& rhs,
                                                   coord_t attach_radius,
                                                   double attach_radius2)
{
    if (lhs.points.size() < 2 || rhs.points.size() < 2)
        return false;

    for (size_t lhs_idx = 1; lhs_idx < lhs.points.size(); ++lhs_idx) {
        const Point& a = lhs.points[lhs_idx - 1];
        const Point& b = lhs.points[lhs_idx];
        const coord_t lhs_min_x = std::min(a.x(), b.x()) - attach_radius;
        const coord_t lhs_max_x = std::max(a.x(), b.x()) + attach_radius;
        const coord_t lhs_min_y = std::min(a.y(), b.y()) - attach_radius;
        const coord_t lhs_max_y = std::max(a.y(), b.y()) + attach_radius;

        for (size_t rhs_idx = 1; rhs_idx < rhs.points.size(); ++rhs_idx) {
            const Point& c = rhs.points[rhs_idx - 1];
            const Point& d = rhs.points[rhs_idx];
            if (lhs_max_x < std::min(c.x(), d.x()) || lhs_min_x > std::max(c.x(), d.x()) ||
                lhs_max_y < std::min(c.y(), d.y()) || lhs_min_y > std::max(c.y(), d.y()))
                continue;

            if (contour_support_anchor_segment_distance2(a, b, c, d) <= attach_radius2)
                return true;
        }
    }
    return false;
}

static void filter_contour_support_paths_connected_to_anchors(Polylines& polylines,
                                                              const Polylines& anchored_paths,
                                                              double attach_distance)
{
    if (polylines.empty() || anchored_paths.empty() || attach_distance <= 0.)
        return;

    const coord_t attach_radius = coord_t(std::ceil(attach_distance));
    const double  attach_radius2 = sqr(double(attach_radius));

    std::vector<size_t> parent(polylines.size());
    std::iota(parent.begin(), parent.end(), size_t(0));
    auto find_root = [&parent](size_t idx) {
        while (parent[idx] != idx) {
            parent[idx] = parent[parent[idx]];
            idx = parent[idx];
        }
        return idx;
    };
    auto unite = [&parent, &find_root](size_t a, size_t b) {
        a = find_root(a);
        b = find_root(b);
        if (a != b)
            parent[b] = a;
    };

    for (size_t i = 0; i < polylines.size(); ++i) {
        if (polylines[i].points.size() < 2)
            continue;
        for (size_t j = i + 1; j < polylines.size(); ++j)
            if (contour_support_anchor_paths_connected(polylines[i], polylines[j], attach_radius, attach_radius2))
                unite(i, j);
    }

    std::vector<char> anchor_connected(polylines.size(), false);
    for (size_t i = 0; i < polylines.size(); ++i) {
        if (polylines[i].points.size() < 2)
            continue;
        for (const Polyline& anchored : anchored_paths) {
            if (contour_support_anchor_paths_connected(polylines[i], anchored, attach_radius, attach_radius2)) {
                anchor_connected[find_root(i)] = true;
                break;
            }
        }
    }

    size_t dst = 0;
    for (size_t src = 0; src < polylines.size(); ++src)
        if (!anchor_connected[find_root(src)])
            polylines[dst++] = std::move(polylines[src]);
    polylines.resize(dst);
}

static void filter_short_independent_contour_support_paths(Polylines& polylines,
                                                           const Polylines& anchored_paths,
                                                           double max_independent_length,
                                                           double attach_distance)
{
    if (polylines.empty() || max_independent_length <= 0. || attach_distance <= 0.)
        return;

    const coord_t attach_radius = coord_t(std::ceil(attach_distance));
    const double  attach_radius2 = sqr(double(attach_radius));

    std::vector<size_t> parent(polylines.size());
    std::iota(parent.begin(), parent.end(), size_t(0));
    auto find_root = [&parent](size_t idx) {
        while (parent[idx] != idx) {
            parent[idx] = parent[parent[idx]];
            idx = parent[idx];
        }
        return idx;
    };
    auto unite = [&parent, &find_root](size_t a, size_t b) {
        a = find_root(a);
        b = find_root(b);
        if (a != b)
            parent[b] = a;
    };

    for (size_t i = 0; i < polylines.size(); ++i) {
        if (polylines[i].points.size() < 2)
            continue;
        for (size_t j = i + 1; j < polylines.size(); ++j)
            if (contour_support_anchor_paths_connected(polylines[i], polylines[j], attach_radius, attach_radius2))
                unite(i, j);
    }

    std::vector<double> component_length(polylines.size(), 0.);
    std::vector<char>   component_anchored(polylines.size(), false);
    for (size_t i = 0; i < polylines.size(); ++i) {
        if (polylines[i].points.size() < 2)
            continue;

        const size_t root = find_root(i);
        component_length[root] += polylines[i].length();
        for (const Polyline& anchored : anchored_paths) {
            if (contour_support_anchor_paths_connected(polylines[i], anchored, attach_radius, attach_radius2)) {
                component_anchored[root] = true;
                break;
            }
        }
    }

    size_t dst = 0;
    for (size_t src = 0; src < polylines.size(); ++src) {
        const size_t root = find_root(src);
        if (!component_anchored[root] && component_length[root] <= max_independent_length)
            continue;
        polylines[dst++] = std::move(polylines[src]);
    }
    polylines.resize(dst);
}

static void filter_short_free_contour_support_paths(Polylines& polylines,
                                                    double max_free_length,
                                                    double attach_distance)
{
    if (polylines.empty() || max_free_length <= 0. || attach_distance <= 0.)
        return;

    const coord_t attach_radius = coord_t(std::ceil(attach_distance > 0. ? attach_distance : 0.));
    const double  attach_radius2 = sqr(double(attach_radius));
    auto segment_distance2 = [](const Point& p1, const Point& q1, const Point& p2, const Point& q2, double& s, double& t) {
        const Vec2d d1 = q1.cast<double>() - p1.cast<double>();
        const Vec2d d2 = q2.cast<double>() - p2.cast<double>();
        const Vec2d r  = p1.cast<double>() - p2.cast<double>();
        const double a = d1.squaredNorm();
        const double e = d2.squaredNorm();
        const double f = d2.dot(r);
        constexpr double eps = 1e-12;

        if (a <= eps && e <= eps) {
            s = 0.;
            t = 0.;
            return r.squaredNorm();
        }
        if (a <= eps) {
            s = 0.;
            t = std::clamp(f / e, 0., 1.);
        } else {
            const double c = d1.dot(r);
            if (e <= eps) {
                t = 0.;
                s = std::clamp(-c / a, 0., 1.);
            } else {
                const double b = d1.dot(d2);
                const double denom = a * e - b * b;
                s = denom != 0. ? std::clamp((b * f - c * e) / denom, 0., 1.) : 0.;
                t = (b * s + f) / e;
                if (t < 0.) {
                    t = 0.;
                    s = std::clamp(-c / a, 0., 1.);
                } else if (t > 1.) {
                    t = 1.;
                    s = std::clamp((b - c) / a, 0., 1.);
                }
            }
        }

        const Vec2d c1 = p1.cast<double>() + d1 * s;
        const Vec2d c2 = p2.cast<double>() + d2 * t;
        return (c1 - c2).squaredNorm();
    };

    bool changed = true;
    while (changed) {
        changed = false;
        std::vector<char> remove(polylines.size(), false);

        auto endpoint_near_polyline = [attach_radius, attach_radius2](const Point& point, const Polyline& other) {
            if (other.points.size() < 2)
                return false;

            for (size_t point_idx = 1; point_idx < other.points.size(); ++point_idx) {
                const Point& a = other.points[point_idx - 1];
                const Point& b = other.points[point_idx];
                if (point.x() < std::min(a.x(), b.x()) - attach_radius ||
                    point.x() > std::max(a.x(), b.x()) + attach_radius ||
                    point.y() < std::min(a.y(), b.y()) - attach_radius ||
                    point.y() > std::max(a.y(), b.y()) + attach_radius)
                    continue;

                if (Line(a, b).distance_to_squared(point) <= attach_radius2)
                    return true;
            }
            return false;
        };
        auto segment_near_polyline = [attach_radius, attach_radius2, &segment_distance2](const Point& a,
                                                                                         const Point& b,
                                                                                         const Polyline& other) {
            if (other.points.size() < 2)
                return false;

            const coord_t seg_min_x = std::min(a.x(), b.x()) - attach_radius;
            const coord_t seg_max_x = std::max(a.x(), b.x()) + attach_radius;
            const coord_t seg_min_y = std::min(a.y(), b.y()) - attach_radius;
            const coord_t seg_max_y = std::max(a.y(), b.y()) + attach_radius;
            for (size_t point_idx = 1; point_idx < other.points.size(); ++point_idx) {
                const Point& c = other.points[point_idx - 1];
                const Point& d = other.points[point_idx];
                if (seg_max_x < std::min(c.x(), d.x()) || seg_min_x > std::max(c.x(), d.x()) ||
                    seg_max_y < std::min(c.y(), d.y()) || seg_min_y > std::max(c.y(), d.y()))
                    continue;

                double s = 0.;
                double t = 0.;
                if (segment_distance2(a, b, c, d, s, t) <= attach_radius2)
                    return true;
            }
            return false;
        };
        auto segment_hits_polyline_interior = [attach_radius, attach_radius2, &segment_distance2](const Point& a,
                                                                                                  const Point& b,
                                                                                                  const Polyline& other) {
            if (other.points.size() < 2)
                return false;

            const Vec2d ab = b.cast<double>() - a.cast<double>();
            const double len = std::sqrt(ab.squaredNorm());
            if (len <= 1e-6)
                return false;
            const double end_tolerance = std::min(0.45, double(attach_radius) / len);

            const coord_t seg_min_x = std::min(a.x(), b.x()) - attach_radius;
            const coord_t seg_max_x = std::max(a.x(), b.x()) + attach_radius;
            const coord_t seg_min_y = std::min(a.y(), b.y()) - attach_radius;
            const coord_t seg_max_y = std::max(a.y(), b.y()) + attach_radius;
            for (size_t point_idx = 1; point_idx < other.points.size(); ++point_idx) {
                const Point& c = other.points[point_idx - 1];
                const Point& d = other.points[point_idx];
                if (seg_max_x < std::min(c.x(), d.x()) || seg_min_x > std::max(c.x(), d.x()) ||
                    seg_max_y < std::min(c.y(), d.y()) || seg_min_y > std::max(c.y(), d.y()))
                    continue;

                double s = 0.;
                double t = 0.;
                if (segment_distance2(a, b, c, d, s, t) <= attach_radius2 && s > end_tolerance && s < 1. - end_tolerance)
                    return true;
            }
            return false;
        };
        auto endpoint_hits_polyline_interior = [attach_radius, attach_radius2](const Point& point, const Polyline& other) {
            if (other.points.size() < 2)
                return false;

            for (size_t point_idx = 1; point_idx < other.points.size(); ++point_idx) {
                const Point& a = other.points[point_idx - 1];
                const Point& b = other.points[point_idx];
                if (point.x() < std::min(a.x(), b.x()) - attach_radius ||
                    point.x() > std::max(a.x(), b.x()) + attach_radius ||
                    point.y() < std::min(a.y(), b.y()) - attach_radius ||
                    point.y() > std::max(a.y(), b.y()) + attach_radius)
                    continue;
                if (Line(a, b).distance_to_squared(point) > attach_radius2)
                    continue;

                const bool near_a = (point - a).cast<double>().squaredNorm() <= attach_radius2;
                const bool near_b = (point - b).cast<double>().squaredNorm() <= attach_radius2;
                if (!near_a && !near_b)
                    return true;
            }
            return false;
        };

        auto endpoint_is_connected = [&polylines, &endpoint_near_polyline](size_t current_idx, const Point& point) {
            for (size_t other_idx = 0; other_idx < polylines.size(); ++other_idx) {
                if (other_idx == current_idx)
                    continue;

                if (endpoint_near_polyline(point, polylines[other_idx]))
                    return true;
            }
            return false;
        };
        auto path_has_interior_attachment = [&polylines, &endpoint_hits_polyline_interior](size_t current_idx) {
            const Polyline& current = polylines[current_idx];
            if (current.points.size() < 2)
                return false;

            for (size_t other_idx = 0; other_idx < polylines.size(); ++other_idx) {
                if (other_idx == current_idx || polylines[other_idx].points.size() < 2)
                    continue;
                if (endpoint_hits_polyline_interior(polylines[other_idx].points.front(), current) ||
                    endpoint_hits_polyline_interior(polylines[other_idx].points.back(), current))
                    return true;
            }
            return false;
        };
        auto path_has_segment_interior_attachment = [&polylines, &segment_hits_polyline_interior](size_t current_idx) {
            const Polyline& current = polylines[current_idx];
            if (current.points.size() < 2)
                return false;

            for (size_t point_idx = 1; point_idx < current.points.size(); ++point_idx) {
                const Point& a = current.points[point_idx - 1];
                const Point& b = current.points[point_idx];
                if (a == b)
                    continue;
                for (size_t other_idx = 0; other_idx < polylines.size(); ++other_idx) {
                    if (other_idx == current_idx)
                        continue;
                    if (segment_hits_polyline_interior(a, b, polylines[other_idx]))
                        return true;
                }
            }
            return false;
        };

        for (size_t polyline_idx = 0; polyline_idx < polylines.size(); ++polyline_idx) {
            const Polyline& pl = polylines[polyline_idx];
            if (pl.size() < 2) {
                remove[polyline_idx] = true;
                changed = true;
                continue;
            }

            if (pl.length() > max_free_length)
                continue;
            if (path_has_interior_attachment(polyline_idx) || path_has_segment_interior_attachment(polyline_idx))
                continue;

            const bool front_connected = endpoint_is_connected(polyline_idx, pl.points.front());
            const bool back_connected  = endpoint_is_connected(polyline_idx, pl.points.back());
            if (front_connected != back_connected) {
                remove[polyline_idx] = true;
                changed = true;
            }
        }

        if (changed) {
            std::vector<size_t> parent(polylines.size());
            std::iota(parent.begin(), parent.end(), size_t(0));
            auto find_root = [&parent](size_t idx) {
                while (parent[idx] != idx) {
                    parent[idx] = parent[parent[idx]];
                    idx = parent[idx];
                }
                return idx;
            };
            auto unite = [&parent, &find_root](size_t a, size_t b) {
                a = find_root(a);
                b = find_root(b);
                if (a != b)
                    parent[b] = a;
            };
            for (size_t i = 0; i < polylines.size(); ++i) {
                if (polylines[i].points.size() < 2)
                    continue;
                for (size_t j = i + 1; j < polylines.size(); ++j) {
                    if (polylines[j].points.size() < 2)
                        continue;
                    bool connected = endpoint_near_polyline(polylines[i].points.front(), polylines[j]) ||
                        endpoint_near_polyline(polylines[i].points.back(), polylines[j]) ||
                        endpoint_near_polyline(polylines[j].points.front(), polylines[i]) ||
                        endpoint_near_polyline(polylines[j].points.back(), polylines[i]);
                    if (!connected) {
                        for (size_t pi = 1; pi < polylines[i].points.size() && !connected; ++pi)
                            if (segment_near_polyline(polylines[i].points[pi - 1], polylines[i].points[pi], polylines[j]))
                                connected = true;
                    }
                    if (connected)
                        unite(i, j);
                }
            }

            std::vector<size_t> component_path_count(polylines.size(), 0);
            std::vector<size_t> component_kept_count(polylines.size(), 0);
            std::vector<std::vector<size_t>> component_removed_candidates(polylines.size());
            auto better_survivor = [attach_radius2, &polylines, &path_has_interior_attachment, &path_has_segment_interior_attachment](size_t candidate_idx, size_t current_idx) {
                const Polyline& candidate = polylines[candidate_idx];
                const Polyline& current = polylines[current_idx];
                const bool candidate_through = path_has_interior_attachment(candidate_idx) || path_has_segment_interior_attachment(candidate_idx);
                const bool current_through = path_has_interior_attachment(current_idx) || path_has_segment_interior_attachment(current_idx);
                if (candidate_through != current_through)
                    return candidate_through;
                const bool candidate_closed = candidate.points.size() >= 2 &&
                    (candidate.points.front() - candidate.points.back()).cast<double>().squaredNorm() <= attach_radius2;
                const bool current_closed = current.points.size() >= 2 &&
                    (current.points.front() - current.points.back()).cast<double>().squaredNorm() <= attach_radius2;
                if (candidate_closed != current_closed)
                    return candidate_closed;
                const bool candidate_native = candidate.points.size() > 2;
                const bool current_native = current.points.size() > 2;
                if (candidate_native != current_native)
                    return candidate_native;
                return candidate.length() > current.length();
            };

            for (size_t i = 0; i < polylines.size(); ++i) {
                if (polylines[i].points.size() < 2)
                    continue;
                const size_t root = find_root(i);
                ++component_path_count[root];
                if (!remove[i])
                    ++component_kept_count[root];
                else
                    component_removed_candidates[root].push_back(i);
            }

            for (size_t root = 0; root < component_removed_candidates.size(); ++root) {
                if (component_removed_candidates[root].empty())
                    continue;
                const size_t min_kept = component_path_count[root] > 3 ? 2 : 1;
                if (component_kept_count[root] >= min_kept)
                    continue;

                std::vector<size_t>& candidates = component_removed_candidates[root];
                std::stable_sort(candidates.begin(), candidates.end(), better_survivor);
                for (size_t survivor : candidates) {
                    remove[survivor] = false;
                    if (++component_kept_count[root] >= min_kept)
                        break;
                }
            }
        }

        if (changed) {
            size_t dst = 0;
            for (size_t src = 0; src < polylines.size(); ++src)
                if (!remove[src])
                    polylines[dst++] = std::move(polylines[src]);
            polylines.resize(dst);
        }
    }
}

static void resolve_contour_support_edge_graph(Polylines& polylines,
                                               const Polylines& source_contours,
                                               double max_bridge_length,
                                               double merge_supported_gap,
                                               Polylines* bridged_paths = nullptr,
                                               std::vector<size_t>* bridge_round_counts = nullptr)
{
    if (max_bridge_length <= 0. || polylines.empty() || source_contours.empty())
        return;

    struct Projection {
        size_t contour_idx {0};
        double distance {0.};
        double dist2 {std::numeric_limits<double>::max()};
        bool   valid {false};
    };
    struct UnsupportedInterval {
        size_t patch_idx {0};
        double begin {0.};
        double end {0.};
    };
    struct MergedUnsupportedInterval {
        size_t begin_idx {0};
        size_t end_idx {0};
        double begin {0.};
        double end {0.};
    };
    struct BridgeNode {
        Polyline path;
        double   length {0.};
        bool     has_supported_seed {false};
    };

    std::vector<Line>                indexed_lines;
    std::vector<size_t>              line_to_contour;
    std::vector<double>              line_to_prefix;
    std::vector<std::vector<double>> contour_prefix(source_contours.size());
    std::vector<double>              contour_length_cache(source_contours.size(), 0.);
    std::vector<char>                contour_closed_cache(source_contours.size(), 0);
    {
        size_t total_segments = 0;
        for (const Polyline& contour : source_contours)
            if (contour.points.size() > 1)
                total_segments += contour.points.size() - 1;
        indexed_lines.reserve(total_segments);
        line_to_contour.reserve(total_segments);
        line_to_prefix.reserve(total_segments);
        for (size_t contour_idx = 0; contour_idx < source_contours.size(); ++contour_idx) {
            const Polyline& contour = source_contours[contour_idx];
            std::vector<double>& pref = contour_prefix[contour_idx];
            pref.reserve(contour.points.size());
            if (!contour.points.empty())
                pref.push_back(0.);
            double prefix = 0.;
            for (size_t i = 1; i < contour.points.size(); ++i) {
                const Point& a = contour.points[i - 1];
                const Point& b = contour.points[i];
                const double len = (b - a).cast<double>().norm();
                if (len > 1e-6) {
                    indexed_lines.emplace_back(a, b);
                    line_to_contour.push_back(contour_idx);
                    line_to_prefix.push_back(prefix);
                    prefix += len;
                }
                pref.push_back(prefix);
            }
            contour_length_cache[contour_idx] = prefix;
            contour_closed_cache[contour_idx] = contour.points.size() > 2 && contour.points.front() == contour.points.back();
        }
    }

    const bool have_index = !indexed_lines.empty();
    AABBTreeLines::LinesDistancer<Line> distancer = have_index ?
        AABBTreeLines::LinesDistancer<Line>(std::move(indexed_lines)) :
        AABBTreeLines::LinesDistancer<Line>();

    auto project_to_source_contours = [&distancer, &line_to_contour, &line_to_prefix, have_index](const Point& point) -> Projection {
        Projection best;
        if (!have_index)
            return best;
        auto [dist, line_idx, np] = distancer.distance_from_lines_extra<false>(point);
        if (line_idx == size_t(-1) || !std::isfinite(dist))
            return best;
        const Line& seg = distancer.get_line(line_idx);
        best.contour_idx = line_to_contour[line_idx];
        best.distance = line_to_prefix[line_idx] + (np - seg.a.cast<double>()).norm();
        best.dist2 = dist * dist;
        best.valid = true;
        return best;
    };

    auto point_at_distance = [&source_contours, &contour_prefix, &contour_length_cache](size_t ci, double distance) -> Point {
        const Polyline& contour = source_contours[ci];
        if (contour.points.empty())
            return Point(0, 0);
        if (distance <= 0.)
            return contour.points.front();
        const std::vector<double>& pref = contour_prefix[ci];
        const double total = contour_length_cache[ci];
        if (distance >= total)
            return contour.points.back();
        auto it = std::upper_bound(pref.begin(), pref.end(), distance);
        if (it == pref.begin())
            return contour.points.front();
        if (it == pref.end())
            return contour.points.back();
        const size_t i = size_t(it - pref.begin());
        const double prefix_before = pref[i - 1];
        const double seg_len = pref[i] - prefix_before;
        if (seg_len <= 1e-6)
            return contour.points[i];
        const Vec2d a = contour.points[i - 1].cast<double>();
        const Vec2d b = contour.points[i].cast<double>();
        const Vec2d p = a + std::clamp((distance - prefix_before) / seg_len, 0., 1.) * (b - a);
        return Point(coord_t(std::llround(p.x())), coord_t(std::llround(p.y())));
    };

    auto copy_contour_interval = [&source_contours, &contour_prefix, &contour_length_cache, &point_at_distance]
                                 (size_t ci, double begin, double end) -> Polyline {
        Polyline copied;
        const Polyline& contour = source_contours[ci];
        const double contour_length = contour_length_cache[ci];
        if (contour.points.size() < 2 || contour_length <= 1e-6 || end <= begin)
            return copied;

        auto add_point = [&copied](const Point& point) {
            if (copied.points.empty() || copied.points.back() != point)
                copied.points.emplace_back(point);
        };

        const std::vector<double>& pref = contour_prefix[ci];
        auto append_range = [&](double range_begin, double range_end) {
            range_begin = std::clamp(range_begin, 0., contour_length);
            range_end = std::clamp(range_end, 0., contour_length);
            if (range_end <= range_begin)
                return;
            add_point(point_at_distance(ci, range_begin));
            auto it = std::upper_bound(pref.begin(), pref.end(), range_begin);
            for (size_t i = size_t(it - pref.begin()); i < pref.size(); ++i) {
                if (pref[i] >= range_end)
                    break;
                if (pref[i] > range_begin)
                    add_point(contour.points[i]);
            }
            add_point(point_at_distance(ci, range_end));
        };
        append_range(begin, std::min(end, contour_length));
        if (end > contour_length)
            append_range(0., end - contour_length);
        return copied;
    };

    std::vector<std::vector<UnsupportedInterval>> intervals_by_contour(source_contours.size());
    std::vector<BridgeNode> bridge_nodes;
    auto add_bridge_node = [&bridge_nodes](Polyline path, bool has_supported_seed) {
        if (path.points.size() < 2)
            return;
        const double length = path.length();
        if (length <= 1e-6)
            return;
        bridge_nodes.push_back({std::move(path), length, has_supported_seed});
    };
    for (size_t patch_idx = 0; patch_idx < polylines.size(); ++patch_idx) {
        const Polyline& patch = polylines[patch_idx];
        if (patch.points.size() < 2)
            continue;

        bool any_projected_segment = false;
        for (size_t point_idx = 1; point_idx < patch.points.size(); ++point_idx) {
            if (patch.points[point_idx - 1] == patch.points[point_idx])
                continue;

            const Projection first = project_to_source_contours(patch.points[point_idx - 1]);
            const Projection last = project_to_source_contours(patch.points[point_idx]);
            if (!first.valid || !last.valid || first.contour_idx != last.contour_idx) {
                const double seg_len = (patch.points[point_idx] - patch.points[point_idx - 1]).cast<double>().norm();
                if (seg_len > max_bridge_length * 0.5) {
                    Polyline fallback;
                    fallback.points.emplace_back(patch.points[point_idx - 1]);
                    fallback.points.emplace_back(patch.points[point_idx]);
                    add_bridge_node(std::move(fallback), false);
                }
                continue;
            }

            double begin = std::min(first.distance, last.distance);
            double end = std::max(first.distance, last.distance);
            const double seg_source_length = contour_length_cache[first.contour_idx];
            const bool seg_source_closed = bool(contour_closed_cache[first.contour_idx]);
            if (seg_source_closed && seg_source_length > 1e-6 && end > begin) {
                const double forward_len = end - begin;
                const double wrap_len = seg_source_length - forward_len;
                if (wrap_len > 1e-6 && forward_len > 1e-6) {
                    const double forward_mid_d = 0.5 * (begin + end);
                    double wrap_mid_d = end + 0.5 * wrap_len;
                    if (wrap_mid_d >= seg_source_length)
                        wrap_mid_d -= seg_source_length;
                    const Point forward_mid = point_at_distance(first.contour_idx, forward_mid_d);
                    const Point wrap_mid = point_at_distance(first.contour_idx, wrap_mid_d);
                    const Vec2d patch_mid = 0.5 * (patch.points[point_idx - 1].cast<double>() + patch.points[point_idx].cast<double>());
                    if ((wrap_mid.cast<double>() - patch_mid).squaredNorm() < (forward_mid.cast<double>() - patch_mid).squaredNorm()) {
                        begin = std::max(first.distance, last.distance);
                        end = std::min(first.distance, last.distance) + seg_source_length;
                    }
                }
            }
            if (end - begin <= 1e-6)
                continue;
            intervals_by_contour[first.contour_idx].push_back({patch_idx, begin, end});
            any_projected_segment = true;
        }
        if (!any_projected_segment && patch.length() > max_bridge_length)
            add_bridge_node(patch, false);
    }

    for (size_t contour_idx = 0; contour_idx < intervals_by_contour.size(); ++contour_idx) {
        std::vector<UnsupportedInterval>& intervals = intervals_by_contour[contour_idx];
        if (intervals.empty())
            continue;
        std::sort(intervals.begin(), intervals.end(),
                  [](const UnsupportedInterval& a, const UnsupportedInterval& b) { return a.begin < b.begin; });

        std::vector<MergedUnsupportedInterval> merged_intervals;
        size_t merged_begin_idx = 0;
        double merged_begin = intervals.front().begin;
        double merged_end = intervals.front().end;
        auto flush_merged_interval = [&](size_t end_idx) { merged_intervals.push_back({merged_begin_idx, end_idx, merged_begin, merged_end}); };
        for (size_t i = 1; i < intervals.size(); ++i) {
            if (intervals[i].begin - merged_end <= merge_supported_gap) {
                merged_end = std::max(merged_end, intervals[i].end);
                continue;
            }
            flush_merged_interval(i);
            merged_begin_idx = i;
            merged_begin = intervals[i].begin;
            merged_end = intervals[i].end;
        }
        flush_merged_interval(intervals.size());

        const double source_length = contour_length_cache[contour_idx];
        const bool closed_contour = bool(contour_closed_cache[contour_idx]);
        if (closed_contour && merged_intervals.size() > 1) {
            const double wrap_gap = source_length - merged_intervals.back().end + merged_intervals.front().begin;
            if (wrap_gap <= merge_supported_gap) {
                MergedUnsupportedInterval wrapped = merged_intervals.back();
                wrapped.end_idx = merged_intervals.front().end_idx;
                wrapped.end = merged_intervals.front().end + source_length;
                merged_intervals.back() = wrapped;
                merged_intervals.erase(merged_intervals.begin());
            }
        }

        for (size_t merged_idx = 0; merged_idx < merged_intervals.size(); ++merged_idx) {
            const MergedUnsupportedInterval& merged = merged_intervals[merged_idx];
            const double unsupported_length = merged.end - merged.begin;
            const bool covers_whole_contour = source_length <= 1e-6 || unsupported_length >= source_length - merge_supported_gap;
            double prev_supported_gap = 0.;
            double next_supported_gap = 0.;
            if (!covers_whole_contour) {
                if (closed_contour) {
                    const MergedUnsupportedInterval& prev = merged_intervals[(merged_idx + merged_intervals.size() - 1) % merged_intervals.size()];
                    const MergedUnsupportedInterval& next = merged_intervals[(merged_idx + 1) % merged_intervals.size()];
                    prev_supported_gap = merged.begin - prev.end;
                    if (prev_supported_gap < 0.)
                        prev_supported_gap += source_length;
                    next_supported_gap = next.begin - merged.end;
                    if (next_supported_gap < 0.)
                        next_supported_gap += source_length;
                } else {
                    prev_supported_gap = merged.begin;
                    next_supported_gap = source_length - merged.end;
                }
            }

            const bool touches_supported_seed = !covers_whole_contour &&
                (prev_supported_gap > merge_supported_gap || next_supported_gap > merge_supported_gap);
            add_bridge_node(copy_contour_interval(contour_idx, merged.begin, merged.end), touches_supported_seed);
        }
    }

    if (bridge_nodes.empty()) {
        polylines.clear();
        return;
    }

    struct GraphEdge {
        Line line;
        size_t path_idx {size_t(-1)};
    };

    std::vector<GraphEdge> graph_edges;
    graph_edges.reserve(bridge_nodes.size());
    for (size_t path_idx = 0; path_idx < bridge_nodes.size(); ++path_idx) {
        const Polyline& path = bridge_nodes[path_idx].path;
        for (size_t point_idx = 1; point_idx < path.points.size(); ++point_idx) {
            if (path.points[point_idx - 1] == path.points[point_idx])
                continue;
            graph_edges.push_back({Line(path.points[point_idx - 1], path.points[point_idx]), path_idx});
        }
    }

    std::vector<Line> graph_lines;
    graph_lines.reserve(graph_edges.size());
    for (const GraphEdge& edge : graph_edges)
        graph_lines.emplace_back(edge.line);

    std::vector<std::vector<size_t>> path_adjacency(bridge_nodes.size());
    if (!graph_lines.empty()) {
        AABBTreeLines::LinesDistancer<Line> graph_distancer(std::move(graph_lines));
        const double connect_radius = merge_supported_gap;
        auto connect_paths = [&path_adjacency](size_t a, size_t b) {
            if (a == b)
                return;
            path_adjacency[a].push_back(b);
            path_adjacency[b].push_back(a);
        };

        for (size_t edge_idx = 0; edge_idx < graph_edges.size(); ++edge_idx) {
            const Line& line = graph_edges[edge_idx].line;
            for (const auto& hit : graph_distancer.intersections_with_line<false>(line))
                connect_paths(graph_edges[edge_idx].path_idx, graph_edges[hit.second].path_idx);
            for (size_t hit_idx : graph_distancer.all_lines_in_radius(line.a, connect_radius))
                connect_paths(graph_edges[edge_idx].path_idx, graph_edges[hit_idx].path_idx);
            for (size_t hit_idx : graph_distancer.all_lines_in_radius(line.midpoint(), connect_radius))
                connect_paths(graph_edges[edge_idx].path_idx, graph_edges[hit_idx].path_idx);
            for (size_t hit_idx : graph_distancer.all_lines_in_radius(line.b, connect_radius))
                connect_paths(graph_edges[edge_idx].path_idx, graph_edges[hit_idx].path_idx);
        }
    }

    std::vector<char> closed(bridge_nodes.size(), false);
    std::vector<char> bridged(bridge_nodes.size(), false);
    std::vector<int>  path_bridge_round(bridge_nodes.size(), -1);

    for (size_t seed_idx = 0; seed_idx < bridge_nodes.size(); ++seed_idx) {
        if (!bridge_nodes[seed_idx].has_supported_seed || closed[seed_idx] || bridged[seed_idx])
            continue;

        std::vector<char> local_seen(bridge_nodes.size(), false);
        std::vector<size_t> local_visited;
        std::vector<int> local_round(bridge_nodes.size(), -1);
        std::deque<size_t> queue;
        queue.push_back(seed_idx);
        local_round[seed_idx] = 0;

        double bridged_length = 0.;
        bool   exceeded_bridge_length = false;
        while (!queue.empty()) {
            const size_t path_idx = queue.front();
            queue.pop_front();
            if (closed[path_idx] || bridged[path_idx] || local_seen[path_idx])
                continue;

            local_seen[path_idx] = true;
            local_visited.push_back(path_idx);
            bridged_length += bridge_nodes[path_idx].length;
            if (bridged_length > max_bridge_length) {
                exceeded_bridge_length = true;
                break;
            }

            const int next_round = local_round[path_idx] + 1;
            for (size_t neighbor_idx : path_adjacency[path_idx]) {
                if (closed[neighbor_idx] || bridged[neighbor_idx] || local_seen[neighbor_idx])
                    continue;
                if (local_round[neighbor_idx] < 0)
                    local_round[neighbor_idx] = next_round;
                queue.push_back(neighbor_idx);
            }
        }

        // A bridge seed that grows past max_bridge_length is not a valid bridge.
        // Paths already reached by that seed are closed so a later seed cannot
        // reuse them as a stepping stone and accidentally bridge the same chain.
        if (exceeded_bridge_length) {
            for (size_t path_idx : local_visited)
                closed[path_idx] = true;
        } else {
            for (size_t path_idx : local_visited) {
                bridged[path_idx] = true;
                path_bridge_round[path_idx] = local_round[path_idx];
            }
        }
    }

    if (bridge_round_counts != nullptr) {
        bridge_round_counts->clear();
        for (int round : path_bridge_round) {
            if (round < 0)
                continue;
            if (bridge_round_counts->size() <= size_t(round))
                bridge_round_counts->resize(size_t(round) + 1, 0);
            ++(*bridge_round_counts)[size_t(round)];
        }
    }

    Polylines kept_unsupported;
    Polylines absorbed_unsupported;
    kept_unsupported.reserve(bridge_nodes.size());
    absorbed_unsupported.reserve(bridge_nodes.size());
    for (size_t path_idx = 0; path_idx < bridge_nodes.size(); ++path_idx) {
        if (bridged[path_idx])
            absorbed_unsupported.emplace_back(std::move(bridge_nodes[path_idx].path));
        else
            kept_unsupported.emplace_back(std::move(bridge_nodes[path_idx].path));
    }

    if (bridged_paths != nullptr)
        append(*bridged_paths, std::move(absorbed_unsupported));
    polylines = std::move(kept_unsupported);
}

static Polylines simplify_contour_support_patch_paths(Polylines patch_paths,
                                                      const Polylines& upper_contours,
                                                      double min_patch_length,
                                                      double max_bridge_length,
                                                      double merge_supported_gap,
                                                      coord_t overlap_merge_distance,
                                                      Polylines* bridged_paths = nullptr,
                                                      std::vector<size_t>* bridge_round_counts = nullptr)
{
    resolve_contour_support_edge_graph(patch_paths, upper_contours, max_bridge_length, merge_supported_gap,
                                       bridged_paths, bridge_round_counts);
    remove_overlapping_contour_support_paths(patch_paths, overlap_merge_distance);
    filter_short_contour_support_paths(patch_paths, min_patch_length);
    if (patch_paths.empty())
        return patch_paths;
    return patch_paths;
}

static void filter_long_contour_paths_near_supported_paths(Polylines& print_paths,
                                                           const Polylines& supported_paths,
                                                           coord_t line_width)
{
    if (print_paths.empty() || supported_paths.empty() || line_width <= 0)
        return;

    const double  line_width_d = double(line_width);
    const coord_t near_distance = std::max<coord_t>(1, coord_t(std::ceil(1.6 * line_width_d)));
    const coord_t removal_radius = std::max<coord_t>(1, line_width / 4);
    const double  min_near_length = 40.0 * line_width_d;

    const Polygons supported_envelope = offset(supported_paths, near_distance, ClipperLib::jtRound, scaled<float>(0.01));
    if (supported_envelope.empty())
        return;

    Polylines long_near_paths = intersection_pl(print_paths, supported_envelope);
    filter_short_contour_support_paths(long_near_paths, min_near_length);
    if (long_near_paths.empty())
        return;

    const Polygons removal_envelope = offset(long_near_paths, removal_radius, ClipperLib::jtRound, scaled<float>(0.01));
    if (removal_envelope.empty())
        return;

    print_paths = diff_pl(print_paths, removal_envelope);
    filter_short_contour_support_paths(print_paths, 2.0 * line_width_d);
}
static void clip_contour_support_paths_from_model_region(Polylines& polylines, const ExPolygons* model_region)
{
    if (polylines.empty() || model_region == nullptr || model_region->empty())
        return;

    polylines = diff_pl(polylines, *model_region);
}

ContourSupportPlan plan_contour_support_paths(std::vector<ContourSupportLayerInput> layers,
                                               size_t n_raft_layers,
                                               double max_bridge_length,
                                               bool include_layer_contours_as_demand,
                                               bool write_debug)
{
    ContourSupportPlan plan;
    plan.patch_paths_by_layer.resize(layers.size());
    plan.paths_to_support_by_layer.resize(layers.size());
    if (layers.empty())
        return plan;

    for (ContourSupportLayerInput& layer : layers) {
        if (!layer.legal_region.empty()) {
            layer.layer_contours.clear();
            append_centerline_contours_for_support_plan(layer.layer_contours, layer.legal_region, layer.line_width);
        }
    }

    struct PatchDebugEntry {
        int      upper_idx {-1};
        int      lower_idx {-1};
        coordf_t z {0.};
        bool     skip_legal {false};
        bool     skip_upper {false};
        bool     skip_unsupported {false};
        bool     skip_simplified {false};
        bool     propagation_break {false};
        Polygons legal_lower;
        Polylines lower_contours_snap;
        Polygons  lower_supported_snap;
        Polylines upper_contours_snap;
        Polylines input_paths_snap;
        Polygons  unsupported_region_snap;
        Polylines raw_patches_snap;
        Polylines edge_graph_print_snap;
        Polylines edge_graph_bridged_snap;
        std::vector<size_t> edge_graph_round_counts;
        Polylines after_overlap_remove_snap;
        Polylines simplified_patches_snap;
        Polylines to_support_snap;
    };
    std::vector<PatchDebugEntry> dbg_entries;
    if (write_debug)
        dbg_entries.reserve(layers.size());

    for (int upper_idx = int(layers.size()) - 1; upper_idx > int(n_raft_layers); --upper_idx) {
        const size_t lower_idx = size_t(upper_idx - 1);
        ContourSupportLayerInput& upper = layers[size_t(upper_idx)];
        ContourSupportLayerInput& lower = layers[lower_idx];

        PatchDebugEntry dbg;
        if (write_debug) {
            dbg.upper_idx = upper_idx;
            dbg.lower_idx = int(lower_idx);
            dbg.z         = lower.print_z;
            dbg.legal_lower = lower.legal_region;
            append(dbg.lower_contours_snap, lower.layer_contours);
            append(dbg.lower_contours_snap, lower.printed_paths);
            if (include_layer_contours_as_demand)
                append(dbg.upper_contours_snap, upper.layer_contours);
            append(dbg.upper_contours_snap, upper.paths_to_support);
            append(dbg.upper_contours_snap, upper.input_paths);
            dbg.input_paths_snap = upper.input_paths;
        }

        if (!upper.valid || !lower.valid || lower.line_width <= 0) {
            if (write_debug) {
                dbg.skip_legal = true;
                dbg_entries.push_back(std::move(dbg));
            }
            continue;
        }

        Polylines lower_contours;
        append(lower_contours, lower.layer_contours);
        append(lower_contours, lower.printed_paths);
        // lower_contours are centerlines of already printed support paths.  Use half
        // a line width to model their physical coverage; bridge/free-length filters
        // below decide which longer unsupported contour spans may be left unpropagated.
        //const coordf_t z_gap   = std::max<coordf_t>(EPSILON, upper.print_z - lower.print_z);
        //const coord_t  reach   = lower.line_width + coord_t(scale_(z_gap));
        const coord_t reach = lower.line_width / 2;
        Polygons lower_supported = lower_contours.empty() ? Polygons{} :
            offset(lower_contours, reach, ClipperLib::jtRound, scaled<float>(0.01));

        Polylines upper_contours;
        if (include_layer_contours_as_demand)
            append(upper_contours, upper.layer_contours);
        append(upper_contours, upper.paths_to_support);
        append(upper_contours, upper.input_paths);
        if (upper_contours.empty()) {
            if (write_debug) {
                dbg.skip_upper           = true;
                dbg.lower_supported_snap = lower_supported;
                dbg_entries.push_back(std::move(dbg));
            }
            continue;
        }

        const coord_t demand_region_radius = lower.line_width / 2;
        const Polygons upper_demand_region = offset(upper_contours, demand_region_radius, ClipperLib::jtRound, scaled<float>(0.01));
        Polygons unsupported_region = lower_supported.empty() ? upper_demand_region : diff(upper_demand_region, lower_supported);

        const double min_unsupported_area = sqr(double(lower.line_width));
        if (!unsupported_region.empty() && area(unsupported_region) < min_unsupported_area) {
            if (write_debug) {
                dbg.skip_unsupported        = true;
                dbg.lower_supported_snap    = lower_supported;
                dbg.unsupported_region_snap = std::move(unsupported_region);
                dbg_entries.push_back(std::move(dbg));
            }
            continue;
        }

        Polylines raw_patches = unsupported_region.empty() ? Polylines{} : intersection_pl(upper_contours, unsupported_region);
        Polylines dbg_edge_graph_print;
        Polylines dbg_edge_graph_bridged;
        std::vector<size_t> dbg_edge_graph_round_counts;
        Polylines dbg_after_overlap;
        if (write_debug) {
            dbg_edge_graph_print = Polylines(raw_patches);
            resolve_contour_support_edge_graph(dbg_edge_graph_print, upper_contours, max_bridge_length,
                                               double(lower.line_width), &dbg_edge_graph_bridged, &dbg_edge_graph_round_counts);
            dbg_after_overlap = dbg_edge_graph_print;
            remove_overlapping_contour_support_paths(dbg_after_overlap, lower.line_width / 2);
        }

        Polylines actual_edge_graph_bridged;
        Polylines patch_paths = simplify_contour_support_patch_paths(
            Polylines(raw_patches), upper_contours, 2.0 * double(lower.line_width), max_bridge_length,
            double(lower.line_width), lower.line_width / 2, &actual_edge_graph_bridged, nullptr);
        const double max_independent_short_length = 4.0 * double(lower.line_width);
        filter_short_independent_contour_support_paths(patch_paths, lower_contours, max_independent_short_length,
                                                       double(lower.line_width));
        Polylines supported_upper_paths = lower_supported.empty() ? Polylines{} : intersection_pl(upper_contours, lower_supported);
        append(supported_upper_paths, std::move(actual_edge_graph_bridged));
        filter_long_contour_paths_near_supported_paths(patch_paths, supported_upper_paths, lower.line_width);
        if (patch_paths.empty()) {
            if (write_debug) {
                dbg.skip_unsupported         = unsupported_region.empty();
                dbg.skip_simplified          = !unsupported_region.empty();
                dbg.lower_supported_snap     = lower_supported;
                dbg.unsupported_region_snap  = unsupported_region;
                dbg.raw_patches_snap         = std::move(raw_patches);
                dbg.edge_graph_print_snap    = std::move(dbg_edge_graph_print);
                dbg.edge_graph_bridged_snap  = std::move(dbg_edge_graph_bridged);
                dbg.edge_graph_round_counts  = std::move(dbg_edge_graph_round_counts);
                dbg.after_overlap_remove_snap = std::move(dbg_after_overlap);
                dbg_entries.push_back(std::move(dbg));
            }
            continue;
        }

        Polylines patch_paths_to_support = patch_paths;
        const double max_nonpropagating_free_length = std::max(3.0 * double(lower.line_width), 0.4 * max_bridge_length);
        if (!upper.supported_anchor_paths.empty() && !lower_supported.empty()) {
            Polylines anchored_input_paths = intersection_pl(upper.supported_anchor_paths, lower_supported);
            filter_contour_support_paths_connected_to_anchors(patch_paths_to_support, anchored_input_paths, double(lower.line_width));
        }
        filter_short_free_contour_support_paths(patch_paths_to_support, max_nonpropagating_free_length, double(lower.line_width));
        // legal_region is only the original organic tree footprint for this layer.  Extra
        // contour patches may legitimately be outside that footprint, so using it as the
        // propagation bound would stop valid model-outside repairs.  For propagation we only
        // need to prevent paths from continuing into the model itself.
        clip_contour_support_paths_from_model_region(patch_paths_to_support, lower.model_region);
        filter_short_contour_support_paths(patch_paths_to_support, 2.0 * double(lower.line_width));
        clip_contour_support_paths_from_model_region(patch_paths, lower.model_region);
        filter_short_contour_support_paths(patch_paths, 2.0 * double(lower.line_width));

        if (write_debug) {
            dbg.propagation_break        = !patch_paths.empty() && patch_paths_to_support.empty();
            dbg.lower_supported_snap     = lower_supported;
            dbg.unsupported_region_snap  = unsupported_region;
            dbg.raw_patches_snap         = std::move(raw_patches);
            dbg.edge_graph_print_snap    = std::move(dbg_edge_graph_print);
            dbg.edge_graph_bridged_snap  = std::move(dbg_edge_graph_bridged);
            dbg.edge_graph_round_counts  = std::move(dbg_edge_graph_round_counts);
            dbg.after_overlap_remove_snap = std::move(dbg_after_overlap);
            dbg.simplified_patches_snap  = patch_paths;
            dbg.to_support_snap          = patch_paths_to_support;
            dbg_entries.push_back(std::move(dbg));
        }

        append(plan.patch_paths_by_layer[lower_idx], patch_paths);
        append(lower.printed_paths, std::move(patch_paths));
        append(plan.paths_to_support_by_layer[lower_idx], patch_paths_to_support);
        append(lower.paths_to_support, std::move(patch_paths_to_support));
    }

    if (write_debug) {
        fprintf(stderr, "[patch_debug] entries=%zu\n", dbg_entries.size());
        boost::filesystem::create_directories("C:/temp");
        const std::string json_path = "C:/temp/patch_debug.json";
        std::ofstream f(json_path, std::ios::trunc);
        if (!f.is_open()) {
            fprintf(stderr, "[patch_debug] ERROR: cannot open %s\n", json_path.c_str());
        } else {
            fprintf(stderr, "[patch_debug] writing %s\n", json_path.c_str());
            auto write_pts = [&](const Points& pts, bool close) {
                f << "[";
                for (size_t i = 0; i < pts.size(); ++i) {
                    if (i)
                        f << ",";
                    f << "[" << std::fixed << std::setprecision(3) << unscale<double>(pts[i].x()) << ","
                      << unscale<double>(pts[i].y()) << "]";
                }
                if (close && !pts.empty())
                    f << ",[" << unscale<double>(pts[0].x()) << "," << unscale<double>(pts[0].y()) << "]";
                f << "]";
            };
            auto write_polylines = [&](const Polylines& pls) {
                f << "[";
                for (size_t i = 0; i < pls.size(); ++i) {
                    if (i)
                        f << ",";
                    write_pts(pls[i].points, false);
                }
                f << "]";
            };
            auto write_size_values = [&](const std::vector<size_t>& values) {
                f << "[";
                for (size_t i = 0; i < values.size(); ++i) {
                    if (i)
                        f << ",";
                    f << values[i];
                }
                f << "]";
            };
            auto write_polygons = [&](const Polygons& polys) {
                f << "[";
                for (size_t i = 0; i < polys.size(); ++i) {
                    if (i)
                        f << ",";
                    write_pts(polys[i].points, true);
                }
                f << "]";
            };

            f << "{\"layers\":[\n";
            for (size_t ei = 0; ei < dbg_entries.size(); ++ei) {
                const PatchDebugEntry& e = dbg_entries[ei];
                if (ei)
                    f << ",\n";
                const char* st = e.skip_legal        ? "skip_legal" :
                                 e.skip_upper        ? "skip_upper" :
                                 e.skip_unsupported  ? "skip_unsupported" :
                                 e.skip_simplified   ? "skip_simplified" :
                                 e.propagation_break ? "break" : "ok";
                f << "{\"upper_idx\":" << e.upper_idx << ",\"lower_idx\":" << e.lower_idx << ",\"z\":" << std::fixed
                  << std::setprecision(3) << e.z << ",\"status\":\"" << st << "\",\"stages\":{"
                  << "\"legal_lower\":";
                write_polygons(e.legal_lower);
                f << ",\"lower_contours\":";
                write_polylines(e.lower_contours_snap);
                f << ",\"lower_supported\":";
                write_polygons(e.lower_supported_snap);
                f << ",\"upper_contours\":";
                write_polylines(e.upper_contours_snap);
                f << ",\"input_paths\":";
                write_polylines(e.input_paths_snap);
                f << ",\"unsupported_region\":";
                write_polygons(e.unsupported_region_snap);
                f << ",\"raw_patches\":";
                write_polylines(e.raw_patches_snap);
                f << ",\"edge_graph_print\":";
                write_polylines(e.edge_graph_print_snap);
                f << ",\"edge_graph_bridged\":";
                write_polylines(e.edge_graph_bridged_snap);
                f << ",\"edge_graph_rounds\":";
                write_size_values(e.edge_graph_round_counts);
                f << ",\"after_overlap_remove\":";
                write_polylines(e.after_overlap_remove_snap);
                f << ",\"simplified_patches\":";
                write_polylines(e.simplified_patches_snap);
                f << ",\"to_support\":";
                write_polylines(e.to_support_snap);
                f << "}}";
            }
            f << "\n]}\n";
            fprintf(stderr, "[patch_debug] done\n");
        }
    }

    return plan;
}

void fill_expolygons_with_sheath_generate_paths(ExtrusionEntitiesPtr&    dst,
                                                const Polygons&          polygons,
                                                Fill*                    filler,
                                                float                    density,
                                                ExtrusionRole            role,
                                                const Flow&              flow,
                                                const SupportParameters& support_params,
                                                bool                     with_sheath,
                                                bool                     no_sort)
{
    if (polygons.empty())
        return;

    if (with_sheath) {
        if (density == 0) {
            tree_supports_generate_paths(dst, polygons, flow, support_params);
            return;
        }
    } else {
        fill_expolygons_generate_paths(dst, closing_ex(polygons, float(SCALED_EPSILON)), filler, density, role, flow);
        return;
    }

    FillParams fill_params;
    fill_params.density     = density;
    fill_params.dont_adjust = true;

    const double spacing = flow.scaled_spacing();
    // Clip the sheath path to avoid the extruder to get exactly on the first point of the loop.
    const double clip_length = spacing * 0.15;

    for (ExPolygon& expoly : closing_ex(polygons, float(SCALED_EPSILON), float(SCALED_EPSILON + 0.5 * flow.scaled_width()))) {
        // Don't reorder the skirt and its infills.
        std::unique_ptr<ExtrusionEntityCollection> eec;
        if (no_sort) {
            eec          = std::make_unique<ExtrusionEntityCollection>();
            eec->no_sort = true;
        }
        ExtrusionEntitiesPtr& out = no_sort ? eec->entities : dst;
        extrusion_entities_append_paths(out, draw_perimeters(expoly, clip_length), ExtrusionRole::erSupportMaterial, flow.mm3_per_mm(),
                                        flow.width(), flow.height());
        // Fill in the rest.
        fill_expolygons_generate_paths(out, offset_ex(expoly, float(-0.4 * spacing)), filler, fill_params, density, role, flow);
        if (no_sort && !eec->empty())
            dst.emplace_back(eec.release());
    }
}

// Support layers, partially processed.
struct SupportGeneratorLayerExtruded
{
    SupportGeneratorLayerExtruded& operator=(SupportGeneratorLayerExtruded&& rhs)
    {
        this->layer           = rhs.layer;
        this->extrusions      = std::move(rhs.extrusions);
        m_polygons_to_extrude = std::move(rhs.m_polygons_to_extrude);
        rhs.layer             = nullptr;
        return *this;
    }

    bool empty() const { return layer == nullptr || layer->polygons.empty(); }

    void set_polygons_to_extrude(Polygons&& polygons)
    {
        if (m_polygons_to_extrude == nullptr)
            m_polygons_to_extrude = std::make_unique<Polygons>(std::move(polygons));
        else
            *m_polygons_to_extrude = std::move(polygons);
    }
    Polygons&       polygons_to_extrude() { return (m_polygons_to_extrude == nullptr) ? layer->polygons : *m_polygons_to_extrude; }
    const Polygons& polygons_to_extrude() const { return (m_polygons_to_extrude == nullptr) ? layer->polygons : *m_polygons_to_extrude; }

    bool could_merge(const SupportGeneratorLayerExtruded& other) const
    {
        return !this->empty() && !other.empty() && std::abs(this->layer->height - other.layer->height) < EPSILON &&
               this->layer->bridging == other.layer->bridging;
    }

    // Merge regions, perform boolean union over the merged polygons.
    void merge(SupportGeneratorLayerExtruded&& other)
    {
        assert(this->could_merge(other));
        // 1) Merge the rest polygons to extrude, if there are any.
        if (other.m_polygons_to_extrude != nullptr) {
            if (m_polygons_to_extrude == nullptr) {
                // This layer has no extrusions generated yet, if it has no m_polygons_to_extrude (its area to extrude was not reduced yet).
                assert(this->extrusions.empty());
                m_polygons_to_extrude = std::make_unique<Polygons>(this->layer->polygons);
            }
            Slic3r::polygons_append(*m_polygons_to_extrude, std::move(*other.m_polygons_to_extrude));
            *m_polygons_to_extrude = union_safety_offset(*m_polygons_to_extrude);
            other.m_polygons_to_extrude.reset();
        } else if (m_polygons_to_extrude != nullptr) {
            assert(other.m_polygons_to_extrude == nullptr);
            // The other layer has no extrusions generated yet, if it has no m_polygons_to_extrude (its area to extrude was not reduced yet).
            assert(other.extrusions.empty());
            Slic3r::polygons_append(*m_polygons_to_extrude, other.layer->polygons);
            *m_polygons_to_extrude = union_safety_offset(*m_polygons_to_extrude);
        }
        // 2) Merge the extrusions.
        this->extrusions.insert(this->extrusions.end(), other.extrusions.begin(), other.extrusions.end());
        other.extrusions.clear();
        // 3) Merge the infill polygons.
        Slic3r::polygons_append(this->layer->polygons, std::move(other.layer->polygons));
        this->layer->polygons = union_safety_offset(this->layer->polygons);
        other.layer->polygons.clear();
    }

    void polygons_append(Polygons& dst) const
    {
        if (layer != NULL && !layer->polygons.empty())
            Slic3r::polygons_append(dst, layer->polygons);
    }

    // The source layer. It carries the height and extrusion type (bridging / non bridging, extrusion height).
    SupportGeneratorLayer* layer{nullptr};
    // Collect extrusions. They will be exported sorted by the bottom height.
    ExtrusionEntitiesPtr extrusions;

private:
    // In case the extrusions are non-empty, m_polygons_to_extrude may contain the rest areas yet to be filled by additional support.
    // This is useful mainly for the loop interfaces, which are generated before the zig-zag infills.
    std::unique_ptr<Polygons> m_polygons_to_extrude;
};

typedef std::vector<SupportGeneratorLayerExtruded*> SupportGeneratorLayerExtrudedPtrs;

struct LoopInterfaceProcessor
{
    LoopInterfaceProcessor(coordf_t circle_r) : n_contact_loops(0), circle_radius(circle_r), circle_distance(circle_r * 3.)
    {
        // Shape of the top contact area.
        circle.points.reserve(6);
        for (size_t i = 0; i < 6; ++i) {
            double angle = double(i) * M_PI / 3.;
            circle.points.push_back(Point(circle_radius * cos(angle), circle_radius * sin(angle)));
        }
    }

    // Generate loop contacts at the top_contact_layer,
    // trim the top_contact_layer->polygons with the areas covered by the loops.
    void generate(SupportGeneratorLayerExtruded& top_contact_layer, const Flow& interface_flow_src) const;

    int      n_contact_loops;
    coordf_t circle_radius;
    coordf_t circle_distance;
    Polygon  circle;
};

void LoopInterfaceProcessor::generate(SupportGeneratorLayerExtruded& top_contact_layer, const Flow& interface_flow_src) const
{
    if (n_contact_loops == 0 || top_contact_layer.empty())
        return;

    Flow flow = interface_flow_src.with_height(top_contact_layer.layer->height);

    Polygons overhang_polygons;
    if (top_contact_layer.layer->overhang_polygons != nullptr)
        overhang_polygons = std::move(*top_contact_layer.layer->overhang_polygons);

    // Generate the outermost loop.
    // Find centerline of the external loop (or any other kind of extrusions should the loop be skipped)
    ExPolygons top_contact_expolygons = offset_ex(union_ex(top_contact_layer.layer->polygons), -0.5f * flow.scaled_width());

    // Grid size and bit shifts for quick and exact to/from grid coordinates manipulation.
    coord_t circle_grid_resolution = 1;
    coord_t circle_grid_powerof2   = 0;
    {
        // epsilon to account for rounding errors
        coord_t circle_grid_resolution_non_powerof2 = coord_t(2. * circle_distance + 3.);
        while (circle_grid_resolution < circle_grid_resolution_non_powerof2) {
            circle_grid_resolution <<= 1;
            ++circle_grid_powerof2;
        }
    }

    struct PointAccessor
    {
        const Point* operator()(const Point& pt) const { return &pt; }
    };
    typedef ClosestPointInRadiusLookup<Point, PointAccessor> ClosestPointLookupType;

    Polygons loops0;
    {
        // find centerline of the external loop of the contours
        // Only consider the loops facing the overhang.
        Polygons external_loops;
        // Holes in the external loops.
        Polygons circles;
        Polygons overhang_with_margin = offset(union_ex(overhang_polygons), 0.5f * flow.scaled_width());
        for (ExPolygons::iterator it_contact_expoly = top_contact_expolygons.begin(); it_contact_expoly != top_contact_expolygons.end();
             ++it_contact_expoly) {
            // Store the circle centers placed for an expolygon into a regular grid, hashed by the circle centers.
            ClosestPointLookupType circle_centers_lookup(coord_t(circle_distance - SCALED_EPSILON));
            Points                 circle_centers;
            Point                  center_last;
            // For each contour of the expolygon, start with the outer contour, continue with the holes.
            for (size_t i_contour = 0; i_contour <= it_contact_expoly->holes.size(); ++i_contour) {
                Polygon&     contour        = (i_contour == 0) ? it_contact_expoly->contour : it_contact_expoly->holes[i_contour - 1];
                const Point* seg_current_pt = nullptr;
                coordf_t     seg_current_t  = 0.;
                if (!intersection_pl(contour.split_at_first_point(), overhang_with_margin).empty()) {
                    // The contour is below the overhang at least to some extent.
                    // FIXME ideally one would place the circles below the overhang only.
                    // Walk around the contour and place circles so their centers are not closer than circle_distance from each other.
                    if (circle_centers.empty()) {
                        // Place the first circle.
                        seg_current_pt = &contour.points.front();
                        seg_current_t  = 0.;
                        center_last    = *seg_current_pt;
                        circle_centers_lookup.insert(center_last);
                        circle_centers.push_back(center_last);
                    }
                    for (Points::const_iterator it = contour.points.begin() + 1; it != contour.points.end(); ++it) {
                        // Is it possible to place a circle on this segment? Is it not too close to any of the circles already placed on
                        // this contour?
                        const Point& p1 = *(it - 1);
                        const Point& p2 = *it;
                        // Intersection of a ray (p1, p2) with a circle placed at center_last, with radius of circle_distance.
                        const Vec2d v_seg(coordf_t(p2(0)) - coordf_t(p1(0)), coordf_t(p2(1)) - coordf_t(p1(1)));
                        const Vec2d v_cntr(coordf_t(p1(0) - center_last(0)), coordf_t(p1(1) - center_last(1)));
                        coordf_t    a    = v_seg.squaredNorm();
                        coordf_t    b    = 2. * v_seg.dot(v_cntr);
                        coordf_t    c    = v_cntr.squaredNorm() - circle_distance * circle_distance;
                        coordf_t    disc = b * b - 4. * a * c;
                        if (disc > 0.) {
                            // The circle intersects a ray. Avoid the parts of the segment inside the circle.
                            coordf_t t1 = (-b - sqrt(disc)) / (2. * a);
                            coordf_t t2 = (-b + sqrt(disc)) / (2. * a);
                            coordf_t t0 = (seg_current_pt == &p1) ? seg_current_t : 0.;
                            // Take the lowest t in <t0, 1.>, excluding <t1, t2>.
                            coordf_t t;
                            if (t0 <= t1)
                                t = t0;
                            else if (t2 <= 1.)
                                t = t2;
                            else {
                                // Try the following segment.
                                seg_current_pt = nullptr;
                                continue;
                            }
                            seg_current_pt = &p1;
                            seg_current_t  = t;
                            center_last    = Point(p1(0) + coord_t(v_seg(0) * t), p1(1) + coord_t(v_seg(1) * t));
                            // It has been verified that the new point is far enough from center_last.
                            // Ensure, that it is far enough from all the centers.
                            std::pair<const Point*, coordf_t> circle_closest = circle_centers_lookup.find(center_last);
                            if (circle_closest.first != nullptr) {
                                --it;
                                continue;
                            }
                        } else {
                            // All of the segment is outside the circle. Take the first point.
                            seg_current_pt = &p1;
                            seg_current_t  = 0.;
                            center_last    = p1;
                        }
                        // Place the first circle.
                        circle_centers_lookup.insert(center_last);
                        circle_centers.push_back(center_last);
                    }
                    external_loops.push_back(std::move(contour));
                    for (const Point& center : circle_centers) {
                        circles.push_back(circle);
                        circles.back().translate(center);
                    }
                }
            }
        }
        // Apply a pattern to the external loops.
        loops0 = diff(external_loops, circles);
    }

    Polylines loop_lines;
    {
        // make more loops
        Polygons loop_polygons = loops0;
        for (int i = 1; i < n_contact_loops; ++i)
            polygons_append(loop_polygons,
                            opening(loops0, i * flow.scaled_spacing() + 0.5f * flow.scaled_spacing(), 0.5f * flow.scaled_spacing()));
        // Clip such loops to the side oriented towards the object.
        // Collect split points, so they will be recognized after the clipping.
        // At the split points the clipped pieces will be stitched back together.
        loop_lines.reserve(loop_polygons.size());
        std::unordered_map<Point, int, PointHash> map_split_points;
        for (Polygons::const_iterator it = loop_polygons.begin(); it != loop_polygons.end(); ++it) {
            assert(map_split_points.find(it->first_point()) == map_split_points.end());
            map_split_points[it->first_point()] = -1;
            loop_lines.push_back(it->split_at_first_point());
        }
        loop_lines = intersection_pl(loop_lines, expand(overhang_polygons, scale_(SUPPORT_MATERIAL_MARGIN)));
        // Because a closed loop has been split to a line, loop_lines may contain continuous segments split to 2 pieces.
        // Try to connect them.
        for (int i_line = 0; i_line < int(loop_lines.size()); ++i_line) {
            Polyline& polyline = loop_lines[i_line];
            auto      it       = map_split_points.find(polyline.first_point());
            if (it != map_split_points.end()) {
                // This is a stitching point.
                // If this assert triggers, multiple source polygons likely intersected at this point.
                assert(it->second != -2);
                if (it->second < 0) {
                    // First occurence.
                    it->second = i_line;
                } else {
                    // Second occurence. Join the lines.
                    Polyline& polyline_1st = loop_lines[it->second];
                    assert(polyline_1st.first_point() == it->first || polyline_1st.last_point() == it->first);
                    if (polyline_1st.first_point() == it->first)
                        polyline_1st.reverse();
                    polyline_1st.append(std::move(polyline));
                    it->second = -2;
                }
                continue;
            }
            it = map_split_points.find(polyline.last_point());
            if (it != map_split_points.end()) {
                // This is a stitching point.
                // If this assert triggers, multiple source polygons likely intersected at this point.
                assert(it->second != -2);
                if (it->second < 0) {
                    // First occurence.
                    it->second = i_line;
                } else {
                    // Second occurence. Join the lines.
                    Polyline& polyline_1st = loop_lines[it->second];
                    assert(polyline_1st.first_point() == it->first || polyline_1st.last_point() == it->first);
                    if (polyline_1st.first_point() == it->first)
                        polyline_1st.reverse();
                    polyline.reverse();
                    polyline_1st.append(std::move(polyline));
                    it->second = -2;
                }
            }
        }
        // Remove empty lines.
        remove_degenerate(loop_lines);
    }

    // add the contact infill area to the interface area
    // note that growing loops by $circle_radius ensures no tiny
    // extrusions are left inside the circles; however it creates
    // a very large gap between loops and contact_infill_polygons, so maybe another
    // solution should be found to achieve both goals
    // Store the trimmed polygons into a separate polygon set, so the original infill area remains intact for
    // "modulate by layer thickness".
    top_contact_layer.set_polygons_to_extrude(diff(top_contact_layer.layer->polygons, offset(loop_lines, float(circle_radius * 1.1))));

    // Transform loops into ExtrusionPath objects.
    extrusion_entities_append_paths(top_contact_layer.extrusions, std::move(loop_lines), ExtrusionRole::erSupportMaterialInterface,
                                    flow.mm3_per_mm(), flow.width(), flow.height());
}

#ifdef SLIC3R_DEBUG
static std::string dbg_index_to_color(int idx)
{
    if (idx < 0)
        return "yellow";
    idx = idx % 3;
    switch (idx) {
    case 0: return "red";
    case 1: return "green";
    default: return "blue";
    }
}
#endif /* SLIC3R_DEBUG */

// When extruding a bottom interface layer over an object, the bottom interface layer is extruded in a thin air, therefore
// it is being extruded with a bridging flow to not shrink excessively (the die swell effect).
// Tiny extrusions are better avoided and it is always better to anchor the thread to an existing support structure if possible.
// Therefore the bottom interface spots are expanded a bit. The expanded regions may overlap with another bottom interface layers,
// leading to over extrusion, where they overlap. The over extrusion is better avoided as it often makes the interface layers
// to stick too firmly to the object.
//
// Modulate thickness (increase bottom_z) of extrusions_in_out generated for this_layer
// if they overlap with overlapping_layers, whose print_z is above this_layer.bottom_z() and below this_layer.print_z.
static void modulate_extrusion_by_overlapping_layers(
    // Extrusions generated for this_layer.
    ExtrusionEntitiesPtr&        extrusions_in_out,
    const SupportGeneratorLayer& this_layer,
    // Multiple layers overlapping with this_layer, sorted bottom up.
    const SupportGeneratorLayersPtr& overlapping_layers)
{
    size_t n_overlapping_layers = overlapping_layers.size();
    if (n_overlapping_layers == 0 || extrusions_in_out.empty())
        // The extrusions do not overlap with any other extrusion.
        return;

    // Get the initial extrusion parameters.
    ExtrusionPath* extrusion_path_template = dynamic_cast<ExtrusionPath*>(extrusions_in_out.front());
    assert(extrusion_path_template != nullptr);
    if (extrusion_path_template == nullptr)
        return;
    ExtrusionRole extrusion_role  = extrusion_path_template->role();
    float         extrusion_width = extrusion_path_template->width;

    struct ExtrusionPathFragment
    {
        ExtrusionPathFragment() : mm3_per_mm(-1), width(-1), height(-1) {};
        ExtrusionPathFragment(double mm3_per_mm, float width, float height) : mm3_per_mm(mm3_per_mm), width(width), height(height) {};

        Polylines polylines;
        double    mm3_per_mm;
        float     width;
        float     height;
    };

    // Split the extrusions by the overlapping layers, reduce their extrusion rate.
    // The last path_fragment is from this_layer.
    std::vector<ExtrusionPathFragment> path_fragments(n_overlapping_layers + 1, ExtrusionPathFragment(extrusion_path_template->mm3_per_mm,
                                                                                                      extrusion_path_template->width,
                                                                                                      extrusion_path_template->height));
    // Don't use it, it will be released.
    extrusion_path_template = nullptr;

#ifdef SLIC3R_DEBUG
    static int iRun = 0;
    ++iRun;
    BoundingBox bbox;
    for (size_t i_overlapping_layer = 0; i_overlapping_layer < n_overlapping_layers; ++i_overlapping_layer) {
        const SupportGeneratorLayer& overlapping_layer = *overlapping_layers[i_overlapping_layer];
        bbox.merge(get_extents(overlapping_layer.polygons));
    }
    for (ExtrusionEntitiesPtr::const_iterator it = extrusions_in_out.begin(); it != extrusions_in_out.end(); ++it) {
        ExtrusionPath* path = dynamic_cast<ExtrusionPath*>(*it);
        assert(path != nullptr);
        bbox.merge(get_extents(path->polyline));
    }
    SVG         svg(debug_out_path("support-fragments-%d-%lf.svg", iRun, this_layer.print_z).c_str(), bbox);
    const float transparency = 0.5f;
    // Filled polygons for the overlapping regions.
    svg.draw(union_ex(this_layer.polygons), dbg_index_to_color(-1), transparency);
    for (size_t i_overlapping_layer = 0; i_overlapping_layer < n_overlapping_layers; ++i_overlapping_layer) {
        const SupportGeneratorLayer& overlapping_layer = *overlapping_layers[i_overlapping_layer];
        svg.draw(union_ex(overlapping_layer.polygons), dbg_index_to_color(int(i_overlapping_layer)), transparency);
    }
    // Contours of the overlapping regions.
    svg.draw(to_polylines(this_layer.polygons), dbg_index_to_color(-1), scale_(0.2));
    for (size_t i_overlapping_layer = 0; i_overlapping_layer < n_overlapping_layers; ++i_overlapping_layer) {
        const SupportGeneratorLayer& overlapping_layer = *overlapping_layers[i_overlapping_layer];
        svg.draw(to_polylines(overlapping_layer.polygons), dbg_index_to_color(int(i_overlapping_layer)), scale_(0.1));
    }
    // Fill extrusion, the source.
    for (ExtrusionEntitiesPtr::const_iterator it = extrusions_in_out.begin(); it != extrusions_in_out.end(); ++it) {
        ExtrusionPath* path = dynamic_cast<ExtrusionPath*>(*it);
        std::string    color_name;
        switch ((it - extrusions_in_out.begin()) % 9) {
        case 0: color_name = "magenta"; break;
        case 1: color_name = "deepskyblue"; break;
        case 2: color_name = "coral"; break;
        case 3: color_name = "goldenrod"; break;
        case 4: color_name = "orange"; break;
        case 5: color_name = "olivedrab"; break;
        case 6: color_name = "blueviolet"; break;
        case 7: color_name = "brown"; break;
        default: color_name = "orchid"; break;
        }
        svg.draw(path->polyline, color_name, scale_(0.2));
    }
#endif /* SLIC3R_DEBUG */

    // End points of the original paths.
    std::vector<std::pair<Point, Point>> path_ends;
    // Collect the paths of this_layer.
    {
        Polylines& polylines = path_fragments.back().polylines;
        for (ExtrusionEntity* ee : extrusions_in_out) {
            if (ExtrusionEntityCollection* collection = dynamic_cast<ExtrusionEntityCollection*>(ee)) {
                for (size_t i = 0; i < collection->entities.size(); i++) {
                    if (ExtrusionPath* path = dynamic_cast<ExtrusionPath*>(collection->entities[i])) {
                        polylines.emplace_back(Polyline(std::move(path->polyline)));
                        path_ends.emplace_back(std::pair<Point, Point>(polylines.back().points.front(), polylines.back().points.back()));
                        delete path;
                    }
                }
            } else if (const ExtrusionPath* path = dynamic_cast<ExtrusionPath*>(ee)) {
                polylines.emplace_back(Polyline(std::move(path->polyline)));
                path_ends.emplace_back(std::pair<Point, Point>(polylines.back().points.front(), polylines.back().points.back()));
                delete path;
            } else if (const ExtrusionMultiPath* multipath = dynamic_cast<ExtrusionMultiPath*>(ee)) {
                for (const ExtrusionPath& path : multipath->paths) {
                    polylines.emplace_back(Polyline(std::move(path.polyline)));
                    path_ends.emplace_back(std::pair<Point, Point>(polylines.back().points.front(), polylines.back().points.back()));
                }
            }
        }
    }
    // Destroy the original extrusion paths, their polylines were moved to path_fragments already.
    // This will be the destination for the new paths.
    extrusions_in_out.clear();

    // Fragment the path segments by overlapping layers. The overlapping layers are sorted by an increasing print_z.
    // Trim by the highest overlapping layer first.
    for (int i_overlapping_layer = int(n_overlapping_layers) - 1; i_overlapping_layer >= 0; --i_overlapping_layer) {
        const SupportGeneratorLayer& overlapping_layer = *overlapping_layers[i_overlapping_layer];
        ExtrusionPathFragment&       frag              = path_fragments[i_overlapping_layer];
        Polygons                     polygons_trimming = offset(union_ex(overlapping_layer.polygons), float(scale_(0.5 * extrusion_width)));
        frag.polylines                                 = intersection_pl(path_fragments.back().polylines, polygons_trimming);
        path_fragments.back().polylines                = diff_pl(path_fragments.back().polylines, polygons_trimming);
        // Adjust the extrusion parameters for a reduced layer height and a non-bridging flow (nozzle_dmr = -1, does not matter).
        assert(this_layer.print_z > overlapping_layer.print_z);
        frag.height     = float(this_layer.print_z - overlapping_layer.print_z);
        frag.mm3_per_mm = Flow(frag.width, frag.height, -1.f).mm3_per_mm();
#ifdef SLIC3R_DEBUG
        svg.draw(frag.polylines, dbg_index_to_color(i_overlapping_layer), scale_(0.1));
#endif /* SLIC3R_DEBUG */
    }

#ifdef SLIC3R_DEBUG
    svg.draw(path_fragments.back().polylines, dbg_index_to_color(-1), scale_(0.1));
    svg.Close();
#endif /* SLIC3R_DEBUG */

    // Now chain the split segments using hashing and a nearly exact match, maintaining the order of segments.
    // Create a single ExtrusionPath or ExtrusionEntityCollection per source ExtrusionPath.
    // Map of fragment start/end points to a pair of <i_overlapping_layer, i_polyline_in_layer>
    // Because a non-exact matching is used for the end points, a multi-map is used.
    // As the clipper library may reverse the order of some clipped paths, store both ends into the map.
    struct ExtrusionPathFragmentEnd
    {
        ExtrusionPathFragmentEnd(size_t alayer_idx, size_t apolyline_idx, bool ais_start)
            : layer_idx(alayer_idx), polyline_idx(apolyline_idx), is_start(ais_start)
        {}
        size_t layer_idx;
        size_t polyline_idx;
        bool   is_start;
    };
    class ExtrusionPathFragmentEndPointAccessor
    {
    public:
        ExtrusionPathFragmentEndPointAccessor(const std::vector<ExtrusionPathFragment>& path_fragments) : m_path_fragments(path_fragments)
        {}
        // Return an end point of a fragment, or nullptr if the fragment has been consumed already.
        const Point* operator()(const ExtrusionPathFragmentEnd& fragment_end) const
        {
            const Polyline& polyline = m_path_fragments[fragment_end.layer_idx].polylines[fragment_end.polyline_idx];
            return polyline.points.empty() ? nullptr : (fragment_end.is_start ? &polyline.points.front() : &polyline.points.back());
        }

    private:
        ExtrusionPathFragmentEndPointAccessor& operator=(const ExtrusionPathFragmentEndPointAccessor&) { return *this; }

        const std::vector<ExtrusionPathFragment>& m_path_fragments;
    };
    const coord_t search_radius = 7;
    ClosestPointInRadiusLookup<ExtrusionPathFragmentEnd, ExtrusionPathFragmentEndPointAccessor>
        map_fragment_starts(search_radius, ExtrusionPathFragmentEndPointAccessor(path_fragments));
    for (size_t i_overlapping_layer = 0; i_overlapping_layer <= n_overlapping_layers; ++i_overlapping_layer) {
        const Polylines& polylines = path_fragments[i_overlapping_layer].polylines;
        for (size_t i_polyline = 0; i_polyline < polylines.size(); ++i_polyline) {
            // Map a starting point of a polyline to a pair of <layer, polyline>
            if (polylines[i_polyline].points.size() >= 2) {
                map_fragment_starts.insert(ExtrusionPathFragmentEnd(i_overlapping_layer, i_polyline, true));
                map_fragment_starts.insert(ExtrusionPathFragmentEnd(i_overlapping_layer, i_polyline, false));
            }
        }
    }

    // For each source path:
    for (size_t i_path = 0; i_path < path_ends.size(); ++i_path) {
        const Point& pt_start   = path_ends[i_path].first;
        const Point& pt_end     = path_ends[i_path].second;
        Point        pt_current = pt_start;
        // Find a chain of fragments with the original / reduced print height.
        ExtrusionMultiPath multipath;
        for (;;) {
            // Find a closest end point to pt_current.
            std::pair<const ExtrusionPathFragmentEnd*, coordf_t> end_and_dist2 = map_fragment_starts.find(pt_current);
            // There may be a bug in Clipper flipping the order of two last points in a fragment?
            // assert(end_and_dist2.first != nullptr);
            assert(end_and_dist2.first == nullptr || end_and_dist2.second < search_radius * search_radius);
            if (end_and_dist2.first == nullptr) {
                // New fragment connecting to pt_current was not found.
                // Verify that the last point found is close to the original end point of the unfragmented path.
                // const double d2 = (pt_end - pt_current).cast<double>.squaredNorm();
                // assert(d2 < coordf_t(search_radius * search_radius));
                // End of the path.
                break;
            }
            const ExtrusionPathFragmentEnd& fragment_end_min = *end_and_dist2.first;
            // Fragment to consume.
            ExtrusionPathFragment& frag          = path_fragments[fragment_end_min.layer_idx];
            Polyline&              frag_polyline = frag.polylines[fragment_end_min.polyline_idx];
            // Path to append the fragment to.
            ExtrusionPath* path = multipath.paths.empty() ? nullptr : &multipath.paths.back();
            if (path != nullptr) {
                // Verify whether the path is compatible with the current fragment.
                assert(this_layer.layer_type == sltBottomContact || path->height != frag.height || path->mm3_per_mm != frag.mm3_per_mm);
                if (path->height != frag.height || path->mm3_per_mm != frag.mm3_per_mm) {
                    path = nullptr;
                }
                // Merging with the previous path. This can only happen if the current layer was reduced by a base layer, which was split
                // into a base and interface layer.
            }
            if (path == nullptr) {
                // Allocate a new path.
                multipath.paths.push_back(ExtrusionPath(extrusion_role, frag.mm3_per_mm, frag.width, frag.height));
                path = &multipath.paths.back();
            }
            // The Clipper library may flip the order of the clipped polylines arbitrarily.
            // Reverse the source polyline, if connecting to the end.
            if (!fragment_end_min.is_start)
                frag_polyline.reverse();
            // Enforce exact overlap of the end points of successive fragments.
            assert(frag_polyline.points.front() == pt_current);
            frag_polyline.points.front() = pt_current;
            // Don't repeat the first point.
            if (!path->polyline.points.empty())
                path->polyline.points.pop_back();
            // Consume the fragment's polyline, remove it from the input fragments, so it will be ignored the next time.
            path->polyline.append(std::move(frag_polyline));
            frag_polyline.points.clear();
            pt_current = path->polyline.points.back();
            if (pt_current == pt_end) {
                // End of the path.
                break;
            }
        }
        if (!multipath.paths.empty()) {
            if (multipath.paths.size() == 1) {
                // This path was not fragmented.
                extrusions_in_out.push_back(new ExtrusionPath(std::move(multipath.paths.front())));
            } else {
                // This path was fragmented. Copy the collection as a whole object, so the order inside the collection will not be changed
                // during the chaining of extrusions_in_out.
                extrusions_in_out.push_back(new ExtrusionMultiPath(std::move(multipath)));
            }
        }
    }
    // If there are any non-consumed fragments, add them separately.
    // FIXME this shall not happen, if the Clipper works as expected and all paths split to fragments could be re-connected.
    for (auto it_fragment = path_fragments.begin(); it_fragment != path_fragments.end(); ++it_fragment)
        extrusion_entities_append_paths(extrusions_in_out, std::move(it_fragment->polylines), extrusion_role, it_fragment->mm3_per_mm,
                                        it_fragment->width, it_fragment->height);
}

// Support layer that is covered by some form of dense interface.
static constexpr const std::initializer_list<SupporLayerType> support_types_interface{SupporLayerType::sltRaftInterface,
                                                                                      SupporLayerType::sltBottomContact,
                                                                                      SupporLayerType::sltBottomInterface,
                                                                                      SupporLayerType::sltTopContact,
                                                                                      SupporLayerType::sltTopInterface};

SupportGeneratorLayersPtr generate_support_layers(PrintObject&                     object,
                                                  const SupportGeneratorLayersPtr& raft_layers,
                                                  const SupportGeneratorLayersPtr& bottom_contacts,
                                                  const SupportGeneratorLayersPtr& top_contacts,
                                                  const SupportGeneratorLayersPtr& intermediate_layers,
                                                  const SupportGeneratorLayersPtr& interface_layers,
                                                  const SupportGeneratorLayersPtr& base_interface_layers)
{
    // Install support layers into the object.
    // A support layer installed on a PrintObject has a unique print_z.
    SupportGeneratorLayersPtr layers_sorted;
    layers_sorted.reserve(raft_layers.size() + bottom_contacts.size() + top_contacts.size() + intermediate_layers.size() +
                          interface_layers.size() + base_interface_layers.size());
    append(layers_sorted, raft_layers);
    append(layers_sorted, bottom_contacts);
    append(layers_sorted, top_contacts);
    append(layers_sorted, intermediate_layers);
    append(layers_sorted, interface_layers);
    append(layers_sorted, base_interface_layers);
    // remove dupliated layers
    std::sort(layers_sorted.begin(), layers_sorted.end());
    layers_sorted.erase(std::unique(layers_sorted.begin(), layers_sorted.end()), layers_sorted.end());

    // Sort the layers lexicographically by a raising print_z and a decreasing height.
    std::sort(layers_sorted.begin(), layers_sorted.end(), [](auto* l1, auto* l2) { return *l1 < *l2; });
    int layer_id           = 0;
    int layer_id_interface = 0;
    assert(object.support_layers().empty());
    for (size_t i = 0; i < layers_sorted.size();) {
        // Find the last layer with roughly the same print_z, find the minimum layer height of all.
        // Due to the floating point inaccuracies, the print_z may not be the same even if in theory they should.
        size_t   j    = i + 1;
        coordf_t zmax = layers_sorted[i]->print_z + EPSILON;
        for (; j < layers_sorted.size() && layers_sorted[j]->print_z <= zmax; ++j)
            ;
        // Assign an average print_z to the set of layers with nearly equal print_z.
        coordf_t zavg       = 0.5 * (layers_sorted[i]->print_z + layers_sorted[j - 1]->print_z);
        coordf_t height_min = layers_sorted[i]->height;
        bool     empty      = true;
        // For snug supports, layers where the direction of the support interface shall change are accounted for.
        size_t num_interfaces       = 0;
        size_t num_top_contacts     = 0;
        double top_contact_bottom_z = 0;
        for (size_t u = i; u < j; ++u) {
            SupportGeneratorLayer& layer = *layers_sorted[u];
            if (!layer.polygons.empty()) {
                empty = false;
                num_interfaces += one_of(layer.layer_type, support_types_interface);
                if (layer.layer_type == SupporLayerType::sltTopContact) {
                    ++num_top_contacts;
                    assert(num_top_contacts <= 1);
                    // All top contact layers sharing this print_z shall also share bottom_z.
                    // assert(num_top_contacts == 1 || (top_contact_bottom_z - layer.bottom_z) < EPSILON);
                    top_contact_bottom_z = layer.bottom_z;
                }
            }
            layer.print_z = zavg;
            height_min    = std::min(height_min, layer.height);
        }
        if (!empty) {
            // Here the upper_layer and lower_layer pointers are left to null at the support layers,
            // as they are never used. These pointers are candidates for removal.
            bool   this_layer_contacts_only = num_top_contacts > 0 && num_top_contacts == num_interfaces;
            size_t this_layer_id_interface  = layer_id_interface;
            if (this_layer_contacts_only) {
                // Find a supporting layer for its interface ID.
                for (auto it = object.support_layers().rbegin(); it != object.support_layers().rend(); ++it)
                    if (const SupportLayer& other_layer = **it; std::abs(other_layer.print_z - top_contact_bottom_z) < EPSILON) {
                        // other_layer supports this top contact layer. Assign a different support interface direction to this layer
                        // from the layer that supports it.
                        this_layer_id_interface = other_layer.interface_id() + 1;
                    }
            }
            object.add_support_layer(layer_id++, this_layer_id_interface, height_min, zavg);
            if (num_interfaces && !this_layer_contacts_only)
                ++layer_id_interface;
        }
        i = j;
    }
    return layers_sorted;
}

void generate_support_toolpaths(SupportLayerPtrs&                support_layers,
                                const PrintObjectConfig&         config,
                                const SupportParameters&         support_params,
                                const SlicingParameters&         slicing_params,
                                const SupportGeneratorLayersPtr& raft_layers,
                                const SupportGeneratorLayersPtr& bottom_contacts,
                                const SupportGeneratorLayersPtr& top_contacts,
                                const SupportGeneratorLayersPtr& intermediate_layers,
                                const SupportGeneratorLayersPtr& interface_layers,
                                const SupportGeneratorLayersPtr& base_interface_layers)
{
    // loop_interface_processor with a given circle radius.
    LoopInterfaceProcessor loop_interface_processor(1.5 * support_params.support_material_interface_flow.scaled_width());
    loop_interface_processor.n_contact_loops = config.support_interface_loop_pattern.value ? 1 : 0;

    std::vector<float> angles{support_params.base_angle};
    // if ((is_tree(config.support_type) ? config.support_base_pattern_tree : config.support_base_pattern) == smpRectilinearGrid)
    if (config.support_base_pattern == smpRectilinearGrid)
        angles.push_back(support_params.interface_angle);

    bool have_infill = config.support_base_pattern != smpNone;

    BoundingBox bbox_object(Point(-scale_(1.), -scale_(1.0)), Point(scale_(1.), scale_(1.)));

    //    const coordf_t link_max_length_factor = 3.;
    const coordf_t link_max_length_factor = 0.;

    // Insert the raft base layers.
    auto n_raft_layers = std::min<size_t>(support_layers.size(), std::max(0, int(slicing_params.raft_layers()) - 1));

    tbb::parallel_for(tbb::blocked_range<size_t>(0, n_raft_layers), [&support_layers, &raft_layers, &intermediate_layers, &config,
                                                                     &support_params, &slicing_params, &bbox_object,
                                                                     link_max_length_factor](const tbb::blocked_range<size_t>& range) {
        for (size_t support_layer_id = range.begin(); support_layer_id < range.end(); ++support_layer_id) {
            assert(support_layer_id < raft_layers.size());
            SupportLayer& support_layer = *support_layers[support_layer_id];
            assert(support_layer.support_fills.entities.empty());
            SupportGeneratorLayer& raft_layer = *raft_layers[support_layer_id];

            std::unique_ptr<Fill> filler_interface = std::unique_ptr<Fill>(Fill::new_from_type(support_params.raft_interface_fill_pattern));
            std::unique_ptr<Fill> filler_support   = std::unique_ptr<Fill>(Fill::new_from_type(support_params.base_fill_pattern));
            filler_interface->set_bounding_box(bbox_object);
            filler_support->set_bounding_box(bbox_object);

            // Print the tree supports cutting through the raft with the exception of the 1st layer, where a full support layer will be
            // printed below both the raft and the trees. Trim the raft layers with the tree polygons.
            const Polygons& tree_polygons = support_layer_id > 0 && support_layer_id < intermediate_layers.size() &&
                                                    is_approx(intermediate_layers[support_layer_id]->print_z, support_layer.print_z) ?
                                                intermediate_layers[support_layer_id]->polygons :
                                                Polygons();

            // Print the support base below the support columns, or the support base for the support columns plus the contacts.
            if (support_layer_id > 0) {
                const Polygons& to_infill_polygons = (support_layer_id < slicing_params.base_raft_layers) ?
                                                         raft_layer.polygons :
                                                         // FIXME misusing contact_polygons for support columns.
                                                         ((raft_layer.contact_polygons == nullptr) ? Polygons() :
                                                                                                     *raft_layer.contact_polygons);
                // Trees may cut through the raft layers down to a print bed.
                Flow flow(float(support_params.support_material_flow.width()), float(raft_layer.height),
                          support_params.support_material_flow.nozzle_diameter());
                assert(!raft_layer.bridging);
                if (!to_infill_polygons.empty()) {
                    Fill* filler            = filler_support.get();
                    filler->angle           = support_params.raft_angle_base;
                    filler->spacing         = support_params.support_material_flow.spacing();
                    filler->link_max_length = coord_t(scale_(filler->spacing * link_max_length_factor / support_params.support_density));
                    fill_expolygons_with_sheath_generate_paths(
                        // Destination
                        support_layer.support_fills.entities,
                        // Regions to fill
                        tree_polygons.empty() ? to_infill_polygons : diff(to_infill_polygons, tree_polygons),
                        // Filler and its parameters
                        filler, float(support_params.support_density),
                        // Extrusion parameters
                        ExtrusionRole::erSupportMaterial, flow, support_params, support_params.with_sheath, false);
                }
                if (!tree_polygons.empty())
                    tree_supports_generate_paths(support_layer.support_fills.entities, tree_polygons, flow, support_params);
            }

            Fill* filler  = filler_interface.get();
            Flow  flow    = support_params.first_layer_flow;
            float density = 0.f;
            if (support_layer_id == 0) {
                // Base flange.
                filler->angle   = support_params.raft_angle_1st_layer;
                filler->spacing = support_params.first_layer_flow.spacing();
                density         = float(config.raft_first_layer_density.value * 0.01);
            } else if (support_layer_id >= slicing_params.base_raft_layers) {
                filler->angle = support_params.raft_interface_angle(support_layer.interface_id());
                // We don't use $base_flow->spacing because we need a constant spacing
                // value that guarantees that all layers are correctly aligned.
                filler->spacing = support_params.support_material_flow.spacing();
                assert(!raft_layer.bridging);
                flow    = Flow(float(support_params.raft_interface_flow.width()), float(raft_layer.height),
                               support_params.raft_interface_flow.nozzle_diameter());
                density = float(support_params.raft_interface_density);
            } else
                continue;
            filler->link_max_length = coord_t(scale_(filler->spacing * link_max_length_factor / density));
            fill_expolygons_with_sheath_generate_paths(
                // Destination
                support_layer.support_fills.entities,
                // Regions to fill
                tree_polygons.empty() ? raft_layer.polygons : diff(raft_layer.polygons, tree_polygons),
                // Filler and its parameters
                filler, density,
                // Extrusion parameters
                (support_layer_id < slicing_params.base_raft_layers) ? ExtrusionRole::erSupportMaterial :
                                                                       ExtrusionRole::erSupportMaterialInterface,
                flow,
                // sheath at first layer
                support_params, support_layer_id == 0, support_layer_id == 0);
        }
    });

    struct LayerCacheItem
    {
        LayerCacheItem(SupportGeneratorLayerExtruded* layer_extruded = nullptr) : layer_extruded(layer_extruded) {}
        SupportGeneratorLayerExtruded*      layer_extruded;
        std::vector<SupportGeneratorLayer*> overlapping;
    };
    struct LayerCache
    {
        SupportGeneratorLayerExtruded                      bottom_contact_layer;
        SupportGeneratorLayerExtruded                      top_contact_layer;
        SupportGeneratorLayerExtruded                      base_layer;
        SupportGeneratorLayerExtruded                      interface_layer;
        SupportGeneratorLayerExtruded                      base_interface_layer;
        boost::container::static_vector<LayerCacheItem, 5> nonempty;

        float    ironing_angle;
        Polygons polys_to_iron;

        void add_nonempty_and_sort()
        {
            for (SupportGeneratorLayerExtruded* item :
                 {&bottom_contact_layer, &top_contact_layer, &interface_layer, &base_interface_layer, &base_layer})
                if (!item->empty())
                    this->nonempty.emplace_back(item);
            // Sort the layers with the same print_z coordinate by their heights, thickest first.
            std::stable_sort(this->nonempty.begin(), this->nonempty.end(), [](const LayerCacheItem& lc1, const LayerCacheItem& lc2) {
                return lc1.layer_extruded->layer->height > lc2.layer_extruded->layer->height;
            });
        }
    };
    std::vector<LayerCache> layer_caches(support_layers.size());
    const bool is_organic_tree_support = support_params.support_style == SupportMaterialStyle::smsTreeOrganic;
    const bool organic_contour_repair_enabled = config.tree_support_organic_validate_repair.value;

    tbb::parallel_for(tbb::blocked_range<size_t>(n_raft_layers, support_layers.size()), [&config, &slicing_params, &support_params,
                                                                                         &support_layers, &bottom_contacts, &top_contacts,
                                                                                         &intermediate_layers, &interface_layers,
                                                                                         &base_interface_layers, &layer_caches,
                                                                                         &loop_interface_processor, &bbox_object, &angles,
                                                                                         &have_infill, n_raft_layers,
                                                                                         link_max_length_factor, is_organic_tree_support,
                                                                                         organic_contour_repair_enabled](
                                                                                            const tbb::blocked_range<size_t>& range) {
        // Indices of the 1st layer in their respective container at the support layer height.
        size_t     idx_layer_bottom_contact = size_t(-1);
        size_t     idx_layer_top_contact    = size_t(-1);
        size_t     idx_layer_intermediate   = size_t(-1);
        size_t     idx_layer_interface      = size_t(-1);
        size_t     idx_layer_base_interface = size_t(-1);
        const auto fill_type_first_layer    = ipRectilinear;
        auto       filler_interface         = std::unique_ptr<Fill>(Fill::new_from_type(support_params.contact_fill_pattern));
        // Filler for the 1st layer interface, if different from filler_interface.
        auto filler_first_layer_ptr = std::unique_ptr<Fill>(range.begin() == 0 &&
                                                                    support_params.contact_fill_pattern != fill_type_first_layer ?
                                                                Fill::new_from_type(fill_type_first_layer) :
                                                                nullptr);
        // Pointer to the 1st layer interface filler.
        auto filler_first_layer = filler_first_layer_ptr ? filler_first_layer_ptr.get() : filler_interface.get();
        // Filler for the 1st layer interface, if different from filler_interface.
        auto filler_raft_contact_ptr = std::unique_ptr<Fill>(range.begin() == n_raft_layers &&
                                                                     config.support_interface_top_layers.value == 0 ?
                                                                 Fill::new_from_type(support_params.raft_interface_fill_pattern) :
                                                                 nullptr);
        // Pointer to the 1st layer interface filler.
        auto filler_raft_contact = filler_raft_contact_ptr ? filler_raft_contact_ptr.get() : filler_interface.get();
        // Filler for the base interface (to be used for soluble interface / non soluble base, to produce non soluble interface layer below
        // soluble interface layer).
        auto filler_base_interface = std::unique_ptr<Fill>(
            base_interface_layers.empty() ?
                nullptr :
                Fill::new_from_type(support_params.interface_density > 0.95 || support_params.with_sheath ? ipRectilinear : ipSupportBase));
        auto filler_support = std::unique_ptr<Fill>(Fill::new_from_type(support_params.base_fill_pattern));
        filler_interface->set_bounding_box(bbox_object);
        if (filler_first_layer_ptr)
            filler_first_layer_ptr->set_bounding_box(bbox_object);
        if (filler_raft_contact_ptr)
            filler_raft_contact_ptr->set_bounding_box(bbox_object);
        if (filler_base_interface)
            filler_base_interface->set_bounding_box(bbox_object);
        filler_support->set_bounding_box(bbox_object);
        for (size_t support_layer_id = range.begin(); support_layer_id < range.end(); ++support_layer_id) {
            SupportLayer& support_layer = *support_layers[support_layer_id];
            LayerCache&   layer_cache   = layer_caches[support_layer_id];
            //             const float   support_interface_angle = ((support_params.support_style == smsGrid && config.support_interface_pattern
            //             != smipRectilinearInterlaced) || config.support_interface_pattern == smipRectilinear) ?
            //                 support_params.interface_angle : support_params.raft_interface_angle(support_layer.interface_id());

            /*   float   support_interface_angle = 0.0;
               if ((support_params.support_style == smsGrid && config.support_interface_pattern != smipRectilinearInterlaced &&
               config.support_interface_pattern != smipAuto))
               {
                   support_interface_angle = support_params.interface_angle;
               }
               else if (config.support_interface_pattern == smipRectilinear)
               {
                   support_interface_angle = support_params.interface_angle;
               }
               else
               {
                   support_interface_angle = support_params.raft_interface_angle(support_layer.interface_id());
               }*/
            const float support_interface_angle = (support_params.support_style == smsGrid ||
                                                   config.support_interface_pattern == smipRectilinear) ?
                                                      support_params.interface_angle :
                                                      support_params.raft_interface_angle(support_layer.interface_id());

            // Find polygons with the same print_z.
            SupportGeneratorLayerExtruded& bottom_contact_layer = layer_cache.bottom_contact_layer;
            SupportGeneratorLayerExtruded& top_contact_layer    = layer_cache.top_contact_layer;
            SupportGeneratorLayerExtruded& base_layer           = layer_cache.base_layer;
            SupportGeneratorLayerExtruded& interface_layer      = layer_cache.interface_layer;
            SupportGeneratorLayerExtruded& base_interface_layer = layer_cache.base_interface_layer;
            // Increment the layer indices to find a layer at support_layer.print_z.
            {
                auto fun = [&support_layer](const SupportGeneratorLayer* l) { return l->print_z >= support_layer.print_z - EPSILON; };
                idx_layer_bottom_contact = idx_higher_or_equal(bottom_contacts, idx_layer_bottom_contact, fun);
                idx_layer_top_contact    = idx_higher_or_equal(top_contacts, idx_layer_top_contact, fun);
                idx_layer_intermediate   = idx_higher_or_equal(intermediate_layers, idx_layer_intermediate, fun);
                idx_layer_interface      = idx_higher_or_equal(interface_layers, idx_layer_interface, fun);
                idx_layer_base_interface = idx_higher_or_equal(base_interface_layers, idx_layer_base_interface, fun);
            }
            // Copy polygons from the layers.
            if (idx_layer_bottom_contact < bottom_contacts.size() &&
                bottom_contacts[idx_layer_bottom_contact]->print_z < support_layer.print_z + EPSILON)
                bottom_contact_layer.layer = bottom_contacts[idx_layer_bottom_contact];
            if (idx_layer_top_contact < top_contacts.size() &&
                top_contacts[idx_layer_top_contact]->print_z < support_layer.print_z + EPSILON)
                top_contact_layer.layer = top_contacts[idx_layer_top_contact];
            if (idx_layer_interface < interface_layers.size() &&
                interface_layers[idx_layer_interface]->print_z < support_layer.print_z + EPSILON)
                interface_layer.layer = interface_layers[idx_layer_interface];
            if (idx_layer_base_interface < base_interface_layers.size() &&
                base_interface_layers[idx_layer_base_interface]->print_z < support_layer.print_z + EPSILON)
                base_interface_layer.layer = base_interface_layers[idx_layer_base_interface];
            if (idx_layer_intermediate < intermediate_layers.size() &&
                intermediate_layers[idx_layer_intermediate]->print_z < support_layer.print_z + EPSILON)
                base_layer.layer = intermediate_layers[idx_layer_intermediate];

            // This layer is a raft contact layer. Any contact polygons at this layer are raft contacts.
            bool raft_layer = slicing_params.interface_raft_layers && top_contact_layer.layer &&
                              is_approx(top_contact_layer.layer->print_z, slicing_params.raft_contact_top_z);
            if (config.support_interface_top_layers == 0) {
                // If no top interface layers were requested, we treat the contact layer exactly as a generic base layer.
                // Don't merge the raft contact layer though.
                if (support_params.can_merge_support_regions && !raft_layer) {
                    if (base_layer.could_merge(top_contact_layer))
                        base_layer.merge(std::move(top_contact_layer));
                    else if (base_layer.empty())
                        base_layer = std::move(top_contact_layer);
                }
            } else {
                if (support_params.ironing && !top_contact_layer.empty()) {
                    // Orca: save the top surface to be ironed later
                    layer_cache.ironing_angle = support_interface_angle; // TODO: should we rotate 90 degrees?
                    layer_cache.polys_to_iron = top_contact_layer.polygons_to_extrude();
                }
                loop_interface_processor.generate(top_contact_layer, support_params.support_material_interface_flow);
                // If no loops are allowed, we treat the contact layer exactly as a generic interface layer.
                // Merge interface_layer into top_contact_layer, as the top_contact_layer is not synchronized and therefore it will be used
                // to trim other layers.
                if (top_contact_layer.could_merge(interface_layer) && !raft_layer)
                    top_contact_layer.merge(std::move(interface_layer));
            }
            if ((config.support_interface_top_layers == 0 || config.support_interface_bottom_layers == 0) &&
                support_params.can_merge_support_regions) {
                if (base_layer.could_merge(bottom_contact_layer))
                    base_layer.merge(std::move(bottom_contact_layer));
                else if (base_layer.empty() && !bottom_contact_layer.empty() && !bottom_contact_layer.layer->bridging)
                    base_layer = std::move(bottom_contact_layer);
            } else if (bottom_contact_layer.could_merge(top_contact_layer) && !raft_layer)
                top_contact_layer.merge(std::move(bottom_contact_layer));
            else if (bottom_contact_layer.could_merge(interface_layer))
                bottom_contact_layer.merge(std::move(interface_layer));

#if 0
            if ( ! interface_layer.empty() && ! base_layer.empty()) {
                // turn base support into interface when it's contained in our holes
                // (this way we get wider interface anchoring)
                //FIXME The intention of the code below is unclear. One likely wanted to just merge small islands of base layers filling in the holes
                // inside interface layers, but the code below fills just too much, see GH #4570
                Polygons islands = top_level_islands(interface_layer.layer->polygons);
                polygons_append(interface_layer.layer->polygons, intersection(base_layer.layer->polygons, islands));
                base_layer.layer->polygons = diff(base_layer.layer->polygons, islands);
            }
#endif

            // Top and bottom contacts, interface layers.
            enum class InterfaceLayerType { TopContact, BottomContact, RaftContact, Interface, InterfaceAsBase };
            auto extrude_interface = [&](SupportGeneratorLayerExtruded& layer_ex, InterfaceLayerType interface_layer_type) {
                if (!layer_ex.empty() && !layer_ex.polygons_to_extrude().empty()) {
                    bool interface_as_base = interface_layer_type == InterfaceLayerType::InterfaceAsBase;
                    bool raft_contact      = interface_layer_type == InterfaceLayerType::RaftContact;
                    bool top_contact       = interface_layer_type == InterfaceLayerType::TopContact;
                    // FIXME Bottom interfaces are extruded with the briding flow. Some bridging layers have its height slightly reduced,
                    // therefore
                    //  the bridging flow does not quite apply. Reduce the flow to area of an ellipse? (A = pi * a * b)
                    auto* filler            = raft_contact ? filler_raft_contact : filler_interface.get();
                    auto  interface_flow    = layer_ex.layer->bridging ?
                                                  Flow::bridging_flow(layer_ex.layer->height,
                                                                      support_params.support_material_bottom_interface_flow.nozzle_diameter()) :
                                                  (raft_contact      ? &support_params.raft_interface_flow :
                                                   interface_as_base ? &support_params.support_material_flow :
                                                                       &support_params.support_material_interface_flow)
                                                  ->with_height(float(layer_ex.layer->height));
                    filler->angle           = interface_as_base ?
                                                  // If zero interface layers are configured, use the same angle as for the base layers.
                                        angles[support_layer_id % angles.size()] :
                                                  // Use interface angle for the interface layers.
                                        raft_contact ? support_params.raft_interface_angle(support_layer.interface_id()) :
                                                                 support_interface_angle;
                    double density          = raft_contact      ? support_params.raft_interface_density :
                                              interface_as_base ? support_params.support_density :
                                                                  support_params.interface_density;
                    filler->spacing         = raft_contact      ? support_params.raft_interface_flow.spacing() :
                                              interface_as_base ? support_params.support_material_flow.spacing() :
                                                                  support_params.support_material_interface_flow.spacing();
                    filler->link_max_length = coord_t(scale_(filler->spacing * link_max_length_factor / density));
                    fill_expolygons_generate_paths(
                        // Destination
                        layer_ex.extrusions,
                        // Regions to fill
                        union_safety_offset_ex(layer_ex.polygons_to_extrude()),
                        // Filler and its parameters
                        filler, float(density),
                        // Extrusion parameters
                        interface_as_base ? ExtrusionRole::erSupportMaterial : ExtrusionRole::erSupportMaterialInterface, interface_flow);

                    // if (!is_tree(config.support_type) && interface_layer_type == InterfaceLayerType::TopContact) {
                    //     if (config.ironing_support_layer && slicing_params.gap_support_object >
                    //     support_params.support_material_ironing_flow.height())
                    //     {
                    //         interface_flow                    = support_params.support_material_ironing_flow;
                    //         filler_interface->angle           = Geometry::deg2rad(support_params.base_angle + 45);
                    //         filler_interface->spacing         = support_params.support_material_ironing_flow.spacing();
                    //         filler_interface->link_max_length = coord_t(scale_(3. * filler_interface->spacing));
                    //         FillParams fill_params;
                    //         fill_params.density     = 0.98;
                    //         fill_params.dont_adjust = true;
                    //         fill_params.monotonic   = true;
                    //         fill_params.dont_sort   = false;

                    //        Polygons ironing        = offset(layer_ex.polygons_to_extrude(),-(float)
                    //        scale_(support_params.support_material_ironing_flow.width()), SUPPORT_SURFACES_OFFSET_PARAMETERS); BoundingBox
                    //        bbox           = get_extents(ironing); if (bbox.size().x() >= area_ironing_interface_supported &&
                    //        bbox.size().y() >= area_ironing_interface_supported) {
                    //            fill_expolygon_generate_paths_ironing(
                    //                // Destination
                    //                layer_ex.extrusions,
                    //                // Regions to fill
                    //                union_safety_offset_ex(ironing),
                    //                // Filler and its parameters
                    //                filler_interface.get(), fill_params,
                    //                // Extrusion parameters
                    //                erIroning, interface_flow);
                    //        }
                    //    }
                    //}
                }
            };
            const bool top_interfaces    = config.support_interface_top_layers.value != 0;
            const bool bottom_interfaces = top_interfaces && config.support_interface_bottom_layers != 0;
            extrude_interface(top_contact_layer, raft_layer     ? InterfaceLayerType::RaftContact :
                                                 top_interfaces ? InterfaceLayerType::TopContact :
                                                                  InterfaceLayerType::InterfaceAsBase);
            extrude_interface(bottom_contact_layer,
                              bottom_interfaces ? InterfaceLayerType::BottomContact : InterfaceLayerType::InterfaceAsBase);
            extrude_interface(interface_layer, top_interfaces ? InterfaceLayerType::Interface : InterfaceLayerType::InterfaceAsBase);

            // Base interface layers under soluble interfaces
            if (!base_interface_layer.empty() && !base_interface_layer.polygons_to_extrude().empty()) {
                Fill* filler = filler_base_interface.get();
                // FIXME Bottom interfaces are extruded with the briding flow. Some bridging layers have its height slightly reduced, therefore
                //  the bridging flow does not quite apply. Reduce the flow to area of an ellipse? (A = pi * a * b)
                assert(!base_interface_layer.layer->bridging);
                Flow interface_flow     = support_params.support_material_flow.with_height(float(base_interface_layer.layer->height));
                filler->angle           = support_interface_angle;
                filler->spacing         = support_params.support_material_interface_flow.spacing();
                filler->link_max_length = coord_t(scale_(filler->spacing * link_max_length_factor / support_params.interface_density));
                fill_expolygons_generate_paths(
                    // Destination
                    base_interface_layer.extrusions,
                    // base_layer_interface.extrusions,
                    //  Regions to fill
                    union_safety_offset_ex(base_interface_layer.polygons_to_extrude()),
                    // Filler and its parameters
                    filler, float(support_params.interface_density),
                    // Extrusion parameters
                    ExtrusionRole::erSupportMaterial, interface_flow);
            }

            // Base support or flange.
            if (!base_layer.empty() && !base_layer.polygons_to_extrude().empty()) {
                Fill* filler  = filler_support.get();
                filler->angle = angles[support_layer_id % angles.size()];
                // We don't use $base_flow->spacing because we need a constant spacing
                // value that guarantees that all layers are correctly aligned.
                assert(!base_layer.layer->bridging);
                auto flow               = support_params.support_material_flow.with_height(float(base_layer.layer->height));
                filler->spacing         = support_params.support_material_flow.spacing();
                filler->link_max_length = coord_t(scale_(filler->spacing * link_max_length_factor / support_params.support_density));
                float density           = float(support_params.support_density);
                bool  sheath            = support_params.with_sheath;
                if (support_layer_id > 0 && !have_infill) {
                    density = 0;
                    sheath  = true;
                }
                bool no_sort = false;
                bool done    = false;
                if (base_layer.layer->bottom_z < EPSILON) {
                    // Base flange (the 1st layer).
                    filler        = filler_first_layer;
                    filler->angle = Geometry::deg2rad(float(config.support_angle.value + 90.));
                    density       = float(config.raft_first_layer_density.value * 0.01);
                    flow          = support_params.first_layer_flow;
                    // use the proper spacing for first layer as we don't need to align
                    // its pattern to the other layers
                    // FIXME When paralellizing, each thread shall have its own copy of the fillers.
                    filler->spacing         = flow.spacing();
                    filler->link_max_length = coord_t(scale_(filler->spacing * link_max_length_factor / density));
                    sheath                  = true;
                    no_sort                 = true;
                } else if (is_organic_tree_support) {
                    if (!organic_contour_repair_enabled) {
                        // Original organic tree path generation.
                        tree_supports_generate_paths(base_layer.extrusions, base_layer.polygons_to_extrude(), flow, support_params);
                    }
                    // When contour repair is enabled, organic tree contours are
                    // planned after all base layers are known.
                    done = true;
                }
                if (!done)
                    fill_expolygons_with_sheath_generate_paths(
                        // Destination
                        base_layer.extrusions,
                        // Regions to fill
                        base_layer.polygons_to_extrude(),
                        // Filler and its parameters
                        filler, density,
                        // Extrusion parameters
                        ExtrusionRole::erSupportMaterial, flow, support_params, sheath, no_sort);
            }

            // Merge base_interface_layers to base_layers to avoid unneccessary retractions
            if (!base_layer.empty() && !base_interface_layer.empty() && !base_layer.polygons_to_extrude().empty() &&
                !base_interface_layer.polygons_to_extrude().empty() && base_layer.could_merge(base_interface_layer))
                base_layer.merge(std::move(base_interface_layer));

            layer_cache.add_nonempty_and_sort();

            // Collect the support areas with this print_z into islands, as there is no need
            // for retraction over these islands.
            Polygons polys;
            // Collect the extrusions, sorted by the bottom extrusion height.
            for (LayerCacheItem& layer_cache_item : layer_cache.nonempty) {
                // Collect islands to polys.
                layer_cache_item.layer_extruded->polygons_append(polys);
                // The print_z of the top contact surfaces and bottom_z of the bottom contact surfaces are "free"
                // in a sense that they are not synchronized with other support layers. As the top and bottom contact surfaces
                // are inflated to achieve a better anchoring, it may happen, that these surfaces will at least partially
                // overlap in Z with another support layers, leading to over-extrusion.
                // Mitigate the over-extrusion by modulating the extrusion rate over these regions.
                // The print head will follow the same print_z, but the layer thickness will be reduced
                // where it overlaps with another support layer.
                // FIXME When printing a briging path, what is an equivalent height of the squished extrudate of the same width?
                // Collect overlapping top/bottom surfaces.
                layer_cache_item.overlapping.reserve(20);
                coordf_t bottom_z        = layer_cache_item.layer_extruded->layer->bottom_print_z() + EPSILON;
                auto     add_overlapping = [&layer_cache_item, bottom_z](const SupportGeneratorLayersPtr& layers, size_t idx_top) {
                    for (int i = int(idx_top) - 1; i >= 0 && layers[i]->print_z > bottom_z; --i)
                        layer_cache_item.overlapping.push_back(layers[i]);
                };
                add_overlapping(top_contacts, idx_layer_top_contact);
                if (layer_cache_item.layer_extruded->layer->layer_type == SupporLayerType::sltBottomContact) {
                    // Bottom contact layer may overlap with a base layer, which may be changed to interface layer.
                    add_overlapping(intermediate_layers, idx_layer_intermediate);
                    add_overlapping(interface_layers, idx_layer_interface);
                    add_overlapping(base_interface_layers, idx_layer_base_interface);
                }
                // Order the layers by lexicographically by an increasing print_z and a decreasing layer height.
                std::stable_sort(layer_cache_item.overlapping.begin(), layer_cache_item.overlapping.end(),
                                 [](auto* l1, auto* l2) { return *l1 < *l2; });
            }
            assert(support_layer.support_islands.empty());
            if (!polys.empty()) {
                support_layer.support_islands = union_ex(polys);
                // support_layer.support_islands_bboxes.reserve(support_layer.support_islands.size());
                // for (const ExPolygon &expoly : support_layer.support_islands)
                //     support_layer.support_islands_bboxes.emplace_back(get_extents(expoly).inflated(SCALED_EPSILON));
            }
        } // for each support_layer_id
    });

    if (is_organic_tree_support && organic_contour_repair_enabled && layer_caches.size() > 1) {
        // Flip to false to disable per-layer patch debug snapshot recording,
        // the duplicated intermediate filter runs that feed those snapshots,
        // and the patch_debug.json dump. Kept enabled for now while the new
        // pipeline is still being validated.
        static constexpr bool kEnablePatchDebug = false;

        // Stage 1: collect unchanged legal organic regions. These are the source
        // polygons that bound where contour-only toolpaths may be emitted.
        std::vector<Polygons>  legal_regions(layer_caches.size());
        std::vector<Polylines> legal_contours(layer_caches.size());
        std::vector<Polylines> patch_contours(layer_caches.size());
        std::vector<Polylines> patch_contours_to_support(layer_caches.size());

        auto append_outer_contours = [](Polylines& dst, const Polygons& polygons) {
            for (const ExPolygon& expoly : union_ex(polygons)) {
                if (expoly.contour.empty())
                    continue;
                Polyline pl(expoly.contour.points);
                pl.points.emplace_back(pl.points.front());
                dst.emplace_back(std::move(pl));
            }
        };

        auto append_printed_contours = [&legal_contours, &patch_contours](Polylines& dst, size_t layer_idx) {
            dst.reserve(dst.size() + legal_contours[layer_idx].size() + patch_contours[layer_idx].size());
            for (const Polyline& contour : legal_contours[layer_idx])
                dst.emplace_back(contour);
            for (const Polyline& contour : patch_contours[layer_idx])
                dst.emplace_back(contour);
        };

        auto append_contours_needing_support = [&legal_contours, &patch_contours_to_support](Polylines& dst, size_t layer_idx) {
            dst.reserve(dst.size() + legal_contours[layer_idx].size() + patch_contours_to_support[layer_idx].size());
            for (const Polyline& contour : legal_contours[layer_idx])
                dst.emplace_back(contour);
            for (const Polyline& contour : patch_contours_to_support[layer_idx])
                dst.emplace_back(contour);
        };

        auto filter_short_contours = [](Polylines& polylines, double min_length) {
            polylines.erase(std::remove_if(polylines.begin(), polylines.end(),
                                           [min_length](const Polyline& pl) { return pl.size() < 2 || pl.length() < min_length; }),
                            polylines.end());
        };

        auto remove_overlapping_contours = [](Polylines& polylines, coord_t overlap_distance) {
            if (polylines.size() < 2 || overlap_distance <= 0)
                return;

            std::sort(polylines.begin(), polylines.end(),
                      [](const Polyline& a, const Polyline& b) { return a.length() > b.length(); });

            // Maintain an incremental union of the offset envelope of kept
            // polylines instead of re-offsetting the entire `kept` vector on
            // every iteration. The previous version did O(N) work per polyline
            // (offset of a growing list), making the loop O(NÂ²) in Clipper
            // calls; this version does O(1) Clipper ops per polyline.
            Polylines kept;
            Polygons  kept_envelope;
            for (Polyline& polyline : polylines) {
                if (polyline.points.size() < 2)
                    continue;

                Polylines remaining{std::move(polyline)};
                if (!kept_envelope.empty())
                    remaining = diff_pl(remaining, kept_envelope);
                if (remaining.empty())
                    continue;

                Polygons added_envelope = offset(remaining, overlap_distance,
                                                 ClipperLib::jtRound, scaled<float>(0.01));
                if (kept_envelope.empty())
                    kept_envelope = std::move(added_envelope);
                else if (!added_envelope.empty())
                    kept_envelope = union_(kept_envelope, added_envelope);

                append(kept, std::move(remaining));
            }
            polylines = std::move(kept);
        };

        auto filter_short_free_contours_for_support = [](Polylines& polylines, double max_free_length,
                                                         double attach_distance,
                                                         const Points& cut_points = {}) {
            if (polylines.empty() || max_free_length <= 0. || attach_distance <= 0.)
                return;

            struct EndpointRef {
                size_t polyline_idx {0};
                bool   front {false};
                Point  point;
            };

            std::vector<EndpointRef> endpoints;
            endpoints.reserve(2 * polylines.size());
            for (size_t polyline_idx = 0; polyline_idx < polylines.size(); ++polyline_idx) {
                const Polyline& pl = polylines[polyline_idx];
                if (pl.points.empty())
                    continue;
                endpoints.push_back({polyline_idx, true,  pl.points.front()});
                endpoints.push_back({polyline_idx, false, pl.points.back()});
            }

            // Build KD-trees over endpoint XY coords for O(log N) radius
            // queries. Use plain struct functors (not capturing lambdas) as the
            // coordinate function so the tree's template type stays a normal
            // assignable/copyable class â€capturing lambdas leak a deleted
            // move-assign into the tree type and trip MSVC when any other
            // lambda captures the tree by reference.
            struct EndpointCoord {
                const std::vector<EndpointRef>* eps;
                coord_t operator()(size_t idx, size_t dim) const {
                    return dim == 0 ? (*eps)[idx].point.x() : (*eps)[idx].point.y();
                }
            };
            EndpointCoord endpoint_coord{&endpoints};
            KDTreeIndirect<2, coord_t, EndpointCoord>
                endpoint_tree(endpoint_coord, endpoints.size());

            struct CutCoord {
                const Points* pts;
                coord_t operator()(size_t idx, size_t dim) const {
                    return dim == 0 ? (*pts)[idx].x() : (*pts)[idx].y();
                }
            };
            CutCoord cut_coord{&cut_points};
            KDTreeIndirect<2, coord_t, CutCoord>
                cut_tree(cut_coord, cut_points.size());

            const coord_t attach_radius =
                coord_t(std::ceil(attach_distance > 0. ? attach_distance : 0.));

            // Inline the connectivity checks directly into the remove_if
            // predicate with an explicit capture list (no auto-typed helper
            // lambdas captured by [&]). MSVC otherwise fails to deduce the
            // captured closure types â€endpoint_tree/cut_tree contain a
            // capturing lambda member, and capturing a lambda that captures a
            // lambda by reference confuses its type deduction.
            size_t polyline_idx = 0;
            polylines.erase(std::remove_if(polylines.begin(), polylines.end(),
                [&polyline_idx, &endpoints, &cut_points, &endpoint_tree, &cut_tree, attach_radius, max_free_length]
                (const Polyline& pl) -> bool {
                    const size_t current_idx = polyline_idx++;
                    if (pl.size() < 2)
                        return true;

                    const double length = pl.length();
                    if (length > max_free_length)
                        return false;

                    // Any cut endpoint means this is a deduplication
                    // artifact â€remove from propagation entirely.
                    if (!cut_points.empty()) {
                        if (!find_nearby_points(cut_tree, pl.points.front(), attach_radius).empty() ||
                            !find_nearby_points(cut_tree, pl.points.back(),  attach_radius).empty())
                            return true;
                    }

                    // Dangling check for non-cut polylines: only one end is
                    // connected to another polyline endpoint.
                    auto endpoint_is_connected = [&endpoints, &endpoint_tree, attach_radius, current_idx]
                                                 (bool front, const Point& point) -> bool {
                        if (endpoints.empty())
                            return false;
                        for (size_t idx : find_nearby_points(endpoint_tree, point, attach_radius)) {
                            const EndpointRef& e = endpoints[idx];
                            if (e.polyline_idx == current_idx && e.front == front)
                                continue;
                            return true;
                        }
                        return false;
                    };
                    const bool front_connected = endpoint_is_connected(true,  pl.points.front());
                    const bool back_connected  = endpoint_is_connected(false, pl.points.back());
                    return front_connected != back_connected;
                }),
                polylines.end());
        };

        auto filter_bridgeable_unsupported_contours = [](Polylines& polylines, const Polylines& source_contours,
                                                         double max_bridge_length, double merge_supported_gap) {
            if (max_bridge_length <= 0.)
                return;
            if (polylines.empty() || source_contours.empty())
                return;

            struct Projection {
                size_t contour_idx {0};
                double distance {0.};
                double dist2 {std::numeric_limits<double>::max()};
                bool   valid {false};
            };
            struct UnsupportedInterval {
                size_t patch_idx {0};
                double begin {0.};
                double end {0.};
            };
            struct MergedUnsupportedInterval {
                size_t begin_idx {0};
                size_t end_idx {0};
                double begin {0.};
                double end {0.};
            };

            // Flatten all source_contour segments into a single Line vector and
            // index them with an AABB tree. Each segment carries a side-table
            // entry so we can recover (contour_idx, parametric distance along
            // its contour) from a tree query. Replaces an O(N segments) linear
            // scan per projection with O(log N). At the same pass, build a
            // per-contour prefix-length array (one entry per polyline point),
            // total length and closed-flag â€point_at_distance and the merge
            // loop reuse these instead of recomputing each time.
            std::vector<Line>                indexed_lines;
            std::vector<size_t>              line_to_contour;
            std::vector<double>              line_to_prefix;
            std::vector<std::vector<double>> contour_prefix(source_contours.size());
            std::vector<double>              contour_length_cache(source_contours.size(), 0.);
            std::vector<char>                contour_closed_cache(source_contours.size(), 0);
            {
                size_t total_segments = 0;
                for (const Polyline& contour : source_contours)
                    if (contour.points.size() > 1)
                        total_segments += contour.points.size() - 1;
                indexed_lines.reserve(total_segments);
                line_to_contour.reserve(total_segments);
                line_to_prefix.reserve(total_segments);
                for (size_t contour_idx = 0; contour_idx < source_contours.size(); ++contour_idx) {
                    const Polyline& contour = source_contours[contour_idx];
                    std::vector<double>& pref = contour_prefix[contour_idx];
                    pref.reserve(contour.points.size());
                    if (!contour.points.empty())
                        pref.push_back(0.);
                    double prefix = 0.;
                    for (size_t i = 1; i < contour.points.size(); ++i) {
                        const Point& a = contour.points[i - 1];
                        const Point& b = contour.points[i];
                        const double len = (b - a).cast<double>().norm();
                        if (len > 1e-6) {
                            indexed_lines.emplace_back(a, b);
                            line_to_contour.push_back(contour_idx);
                            line_to_prefix.push_back(prefix);
                            prefix += len;
                        }
                        pref.push_back(prefix);
                    }
                    contour_length_cache[contour_idx] = prefix;
                    contour_closed_cache[contour_idx] =
                        contour.points.size() > 2 && contour.points.front() == contour.points.back();
                }
            }
            const bool have_index = !indexed_lines.empty();
            AABBTreeLines::LinesDistancer<Line> distancer = have_index ?
                AABBTreeLines::LinesDistancer<Line>(std::move(indexed_lines)) :
                AABBTreeLines::LinesDistancer<Line>();

            auto project_to_source_contours = [&distancer, &line_to_contour, &line_to_prefix, have_index]
                                              (const Point& point) -> Projection {
                Projection best;
                if (!have_index)
                    return best;
                auto [dist, line_idx, np] = distancer.distance_from_lines_extra<false>(point);
                if (line_idx == size_t(-1) || !std::isfinite(dist))
                    return best;
                const Line&  seg   = distancer.get_line(line_idx);
                const double along = (np - seg.a.cast<double>()).norm();
                best.contour_idx = line_to_contour[line_idx];
                best.distance    = line_to_prefix[line_idx] + along;
                best.dist2       = dist * dist;
                best.valid       = true;
                return best;
            };

            // Binary search over the cached prefix array. Each entry of pref
            // matches one polyline point, so `upper_bound(pref, d) - 1` gives
            // the start of the segment that contains parametric distance d.
            auto point_at_distance = [&source_contours, &contour_prefix, &contour_length_cache]
                                     (size_t ci, double distance) -> Point {
                const Polyline& contour = source_contours[ci];
                if (contour.points.empty())
                    return Point(0, 0);
                if (distance <= 0.)
                    return contour.points.front();
                const std::vector<double>& pref = contour_prefix[ci];
                const double total = contour_length_cache[ci];
                if (distance >= total)
                    return contour.points.back();
                auto it = std::upper_bound(pref.begin(), pref.end(), distance);
                if (it == pref.begin())
                    return contour.points.front();
                if (it == pref.end())
                    return contour.points.back();
                const size_t i = size_t(it - pref.begin());
                const double prefix_before = pref[i - 1];
                const double seg_len       = pref[i] - prefix_before;
                if (seg_len <= 1e-6)
                    return contour.points[i];
                const Vec2d a = contour.points[i - 1].cast<double>();
                const Vec2d b = contour.points[i].cast<double>();
                const double t = std::clamp((distance - prefix_before) / seg_len, 0., 1.);
                const Vec2d p = a + t * (b - a);
                return Point(coord_t(std::llround(p.x())), coord_t(std::llround(p.y())));
            };

            auto copy_contour_interval = [&source_contours, &contour_prefix, &contour_length_cache, &point_at_distance]
                                         (size_t ci, double begin, double end) -> Polyline {
                Polyline copied;
                const Polyline& contour = source_contours[ci];
                const double contour_length = contour_length_cache[ci];
                if (contour.points.size() < 2 || contour_length <= 1e-6 || end <= begin)
                    return copied;

                auto add_point = [&copied](const Point& point) {
                    if (copied.points.empty() || copied.points.back() != point)
                        copied.points.emplace_back(point);
                };

                const std::vector<double>& pref = contour_prefix[ci];

                auto append_range = [&](double range_begin, double range_end) {
                    range_begin = std::clamp(range_begin, 0., contour_length);
                    range_end   = std::clamp(range_end,   0., contour_length);
                    if (range_end <= range_begin)
                        return;

                    add_point(point_at_distance(ci, range_begin));

                    // Skip directly to the first polyline point strictly inside
                    // (range_begin, range_end) using binary search on the
                    // cached prefix array.
                    auto it = std::upper_bound(pref.begin(), pref.end(), range_begin);
                    for (size_t i = size_t(it - pref.begin()); i < pref.size(); ++i) {
                        if (pref[i] >= range_end)
                            break;
                        if (pref[i] > range_begin)
                            add_point(contour.points[i]);
                    }

                    add_point(point_at_distance(ci, range_end));
                };

                append_range(begin, std::min(end, contour_length));
                if (end > contour_length)
                    append_range(0., end - contour_length);
                return copied;
            };

            std::vector<std::vector<UnsupportedInterval>> intervals_by_contour(source_contours.size());
            Polylines copied_unsupported_contours;

            for (size_t patch_idx = 0; patch_idx < polylines.size(); ++patch_idx) {
                const Polyline& patch = polylines[patch_idx];
                if (patch.points.size() < 2)
                    continue;

                bool any_projected_segment = false;
                for (size_t point_idx = 1; point_idx < patch.points.size(); ++point_idx) {
                    if (patch.points[point_idx - 1] == patch.points[point_idx])
                        continue;

                    const Projection first = project_to_source_contours(patch.points[point_idx - 1]);
                    const Projection last  = project_to_source_contours(patch.points[point_idx]);
                    if (!first.valid || !last.valid || first.contour_idx != last.contour_idx) {
                        const double seg_len = (patch.points[point_idx] - patch.points[point_idx - 1])
                                                   .cast<double>().norm();
                        if (seg_len > max_bridge_length * 0.5) {
                            Polyline fallback;
                            fallback.points.emplace_back(patch.points[point_idx - 1]);
                            fallback.points.emplace_back(patch.points[point_idx]);
                            copied_unsupported_contours.emplace_back(std::move(fallback));
                        }
                        continue;
                    }

                    double begin = std::min(first.distance, last.distance);
                    double end   = std::max(first.distance, last.distance);
                    // On closed source contours, the two projections split the
                    // ring into two arcs (the "forward" one [min, max] and the
                    // "wrap" one going through the seam). Picking by parametric
                    // min/max alone fails when a small patch segment straddles
                    // the seam: a 4mm arc from 98mmâ†mm would be read as the
                    // 96mm arc on the far side of the ring. Decide by which
                    // arc's midpoint actually lies near the patch segment.
                    const double seg_source_length = contour_length_cache[first.contour_idx];
                    const bool   seg_source_closed = bool(contour_closed_cache[first.contour_idx]);
                    if (seg_source_closed && seg_source_length > 1e-6 && end > begin) {
                        const double forward_len = end - begin;
                        const double wrap_len    = seg_source_length - forward_len;
                        if (wrap_len > 1e-6 && forward_len > 1e-6) {
                            const double forward_mid_d = 0.5 * (begin + end);
                            double       wrap_mid_d    = end + 0.5 * wrap_len;
                            if (wrap_mid_d >= seg_source_length)
                                wrap_mid_d -= seg_source_length;

                            const Point forward_mid = point_at_distance(first.contour_idx, forward_mid_d);
                            const Point wrap_mid    = point_at_distance(first.contour_idx, wrap_mid_d);
                            const Vec2d patch_mid   = 0.5 * (patch.points[point_idx - 1].cast<double>() +
                                                             patch.points[point_idx].cast<double>());
                            const double forward_dist2 = (forward_mid.cast<double>() - patch_mid).squaredNorm();
                            const double wrap_dist2    = (wrap_mid.cast<double>()    - patch_mid).squaredNorm();

                            if (wrap_dist2 < forward_dist2) {
                                // Express the seam-crossing arc as [max, min + L]
                                // so copy_contour_interval's existing
                                // end > contour_length branch emits the wrapped
                                // piece.
                                const double new_begin = std::max(first.distance, last.distance);
                                const double new_end   = std::min(first.distance, last.distance) + seg_source_length;
                                begin = new_begin;
                                end   = new_end;
                            }
                        }
                    }
                    if (end - begin <= 1e-6)
                        continue;

                    intervals_by_contour[first.contour_idx].push_back({patch_idx, begin, end});
                    any_projected_segment = true;
                }

                if (!any_projected_segment && patch.length() > max_bridge_length)
                    copied_unsupported_contours.emplace_back(patch);
            }

            for (size_t contour_idx = 0; contour_idx < intervals_by_contour.size(); ++contour_idx) {
                std::vector<UnsupportedInterval>& intervals = intervals_by_contour[contour_idx];
                if (intervals.empty())
                    continue;

                std::sort(intervals.begin(), intervals.end(),
                          [](const UnsupportedInterval& a, const UnsupportedInterval& b) { return a.begin < b.begin; });

                std::vector<MergedUnsupportedInterval> merged_intervals;
                size_t merged_begin_idx = 0;
                double merged_begin = intervals.front().begin;
                double merged_end   = intervals.front().end;
                auto flush_merged_interval = [&](size_t end_idx) {
                    merged_intervals.push_back({merged_begin_idx, end_idx, merged_begin, merged_end});
                };

                for (size_t i = 1; i < intervals.size(); ++i) {
                    if (intervals[i].begin - merged_end <= merge_supported_gap) {
                        merged_end = std::max(merged_end, intervals[i].end);
                        continue;
                    }

                    flush_merged_interval(i);
                    merged_begin_idx = i;
                    merged_begin     = intervals[i].begin;
                    merged_end       = intervals[i].end;
                }
                flush_merged_interval(intervals.size());

                const double source_length  = contour_length_cache[contour_idx];
                const bool   closed_contour = bool(contour_closed_cache[contour_idx]);

                if (closed_contour && merged_intervals.size() > 1) {
                    const double wrap_gap = source_length - merged_intervals.back().end + merged_intervals.front().begin;
                    if (wrap_gap <= merge_supported_gap) {
                        MergedUnsupportedInterval wrapped = merged_intervals.back();
                        wrapped.end_idx = merged_intervals.front().end_idx;
                        wrapped.end     = merged_intervals.front().end + source_length;
                        merged_intervals.back() = wrapped;
                        merged_intervals.erase(merged_intervals.begin());
                    }
                }

                for (size_t merged_idx = 0; merged_idx < merged_intervals.size(); ++merged_idx) {
                    const MergedUnsupportedInterval& merged = merged_intervals[merged_idx];
                    const double unsupported_length = merged.end - merged.begin;
                    const bool covers_whole_contour = source_length <= 1e-6 ||
                        unsupported_length >= source_length - merge_supported_gap;

                    double prev_supported_gap = 0.;
                    double next_supported_gap = 0.;
                    if (!covers_whole_contour) {
                        if (closed_contour) {
                            const MergedUnsupportedInterval& prev = merged_intervals[(merged_idx + merged_intervals.size() - 1) % merged_intervals.size()];
                            const MergedUnsupportedInterval& next = merged_intervals[(merged_idx + 1) % merged_intervals.size()];
                            prev_supported_gap = merged.begin - prev.end;
                            if (prev_supported_gap < 0.)
                                prev_supported_gap += source_length;
                            next_supported_gap = next.begin - merged.end;
                            if (next_supported_gap < 0.)
                                next_supported_gap += source_length;
                        } else {
                            prev_supported_gap = merged.begin;
                            next_supported_gap = source_length - merged.end;
                        }
                    }

                    const bool has_reliable_anchor = !covers_whole_contour &&
                        (prev_supported_gap > merge_supported_gap || next_supported_gap > merge_supported_gap);
                    if (!has_reliable_anchor || unsupported_length > max_bridge_length) {
                        Polyline copied = copy_contour_interval(contour_idx, merged.begin, merged.end);
                        if (copied.points.size() >= 2)
                            copied_unsupported_contours.emplace_back(std::move(copied));
                    }
                }
            }

            polylines = std::move(copied_unsupported_contours);
        };

        auto simplify_printed_patch_paths = [&](Polylines patch_paths,
                                                const Polylines& upper_contours,
                                                double           min_patch_length,
                                                double           max_bridge_length,
                                                double           merge_supported_gap,
                                                coord_t          overlap_merge_distance) {
            filter_bridgeable_unsupported_contours(patch_paths, upper_contours, max_bridge_length, merge_supported_gap);
            remove_overlapping_contours(patch_paths, overlap_merge_distance);

            filter_short_contours(patch_paths, min_patch_length);
            if (patch_paths.empty())
                return patch_paths;

            remove_overlapping_contours(patch_paths, overlap_merge_distance);
            filter_short_contours(patch_paths, min_patch_length);
            return patch_paths;
        };

        // Stage 2: initialize each organic base layer from its existing extrusion
        // region. This does not alter the region; it only records the legal bound.
        // Initial exterior contours are independent per layer, so extract them in
        // parallel before the sequential top-down patch propagation starts.
        tbb::parallel_for(tbb::blocked_range<size_t>(n_raft_layers, layer_caches.size()),
                          [&layer_caches, &legal_regions, &legal_contours, &append_outer_contours](
                              const tbb::blocked_range<size_t>& range) {
                              for (size_t layer_idx = range.begin(); layer_idx < range.end(); ++layer_idx) {
                                  const SupportGeneratorLayerExtruded& base_layer = layer_caches[layer_idx].base_layer;
                                  if (!base_layer.empty() && !base_layer.polygons_to_extrude().empty() &&
                                      base_layer.layer->bottom_z >= EPSILON) {
                                      legal_regions[layer_idx] = union_safety_offset(base_layer.polygons_to_extrude());
                                      append_outer_contours(legal_contours[layer_idx], legal_regions[layer_idx]);
                                  }
                              }
                          });

        const coord_t line_width       = coord_t(support_params.support_material_flow.scaled_width());
        const double  min_patch_length = 2.0 * double(line_width);
        const double  max_bridge_length = scale_(config.max_bridge_length.value);
        const double  max_nonpropagating_free_length = std::max(3.0 * double(line_width), 0.4 * max_bridge_length);
        const double  merge_supported_gap = double(line_width);
        const coord_t demand_region_radius = line_width / 2;
        const coord_t overlap_merge_distance = line_width / 4;

        std::vector<ContourSupportLayerInput> contour_support_layers(layer_caches.size());
        const PrintObject* object = support_layers.empty() ? nullptr : support_layers.front()->object();
        size_t object_layer_idx = 0;
        auto append_special_surface_paths = [](const SupportGeneratorLayerExtruded& layer_ex, Polylines& dst) {
            for (const ExtrusionEntity* entity : layer_ex.extrusions)
                if (entity != nullptr)
                    entity->collect_polylines(dst);
        };
        for (size_t layer_idx = n_raft_layers; layer_idx < layer_caches.size(); ++layer_idx) {
            LayerCache& layer_cache = layer_caches[layer_idx];
            SupportGeneratorLayerExtruded& base_layer = layer_cache.base_layer;
            Polylines special_surface_paths;
            append_special_surface_paths(layer_cache.bottom_contact_layer, special_surface_paths);
            append_special_surface_paths(layer_cache.top_contact_layer, special_surface_paths);
            append_special_surface_paths(layer_cache.interface_layer, special_surface_paths);
            append_special_surface_paths(layer_cache.base_interface_layer, special_surface_paths);
            if ((legal_regions[layer_idx].empty() || base_layer.empty()) && special_surface_paths.empty())
                continue;

            const coordf_t support_print_z = support_layers[layer_idx]->print_z;
            if (object != nullptr && object->layer_count() > 0) {
                while (object_layer_idx + 1 < object->layer_count() &&
                       object->get_layer(int(object_layer_idx + 1))->print_z <= support_print_z + EPSILON)
                    ++object_layer_idx;
            }

            ContourSupportLayerInput& input = contour_support_layers[layer_idx];
            input.layer_contours = legal_contours[layer_idx];
            input.legal_region   = legal_regions[layer_idx];
            input.printed_paths  = std::move(special_surface_paths);
            // The legal region is the existing organic tree footprint, not the whole
            // model-outside printable area.  Cache the model slice separately so contour
            // patch propagation can stop at model material without being clipped back to
            // the original tree footprint.
            if (object != nullptr && object->layer_count() > 0 &&
                object->get_layer(int(object_layer_idx))->print_z <= support_print_z + EPSILON)
                input.model_region = &object->get_layer(int(object_layer_idx))->lslices;
            input.print_z        = support_print_z;
            input.line_width     = line_width;
            input.valid          = true;
        }

        ContourSupportPlan contour_support_plan =
            plan_contour_support_paths(std::move(contour_support_layers), n_raft_layers, max_bridge_length, true, false);
        patch_contours            = std::move(contour_support_plan.patch_paths_by_layer);
        patch_contours_to_support = std::move(contour_support_plan.paths_to_support_by_layer);

        // Stage 3: top-down contour support planning. Coverage is computed from
        // exterior contour centerlines only; holes and inner walls are ignored.
        // The complete planned path set of a layer is:
        //   original exterior contours + extra patch contours.
        // Patches are not validated in isolation; once added to layer l, they are
        // simply part of layer l's contour set, and their support need naturally
        // propagates to layer l - 1 through the same gap detection loop.
        if constexpr (kEnablePatchDebug) {

        // Debug: per-layer data collected for HTML visualisation.
        struct PatchDebugEntry {
            int      upper_idx {-1};
            int      lower_idx {-1};
            coordf_t z         {0.};
            // skip reason (mutually exclusive)
            bool skip_legal      {false};
            bool skip_upper      {false};
            bool skip_unsupported{false};
            bool skip_simplified {false};
            bool propagation_break{false}; // patch emitted but NOT tracked
            // geometry snapshots
            Polygons  legal_lower;
            Polylines lower_contours_snap;
            Polygons  lower_supported_snap;
            Polylines upper_contours_snap;
            Polygons  unsupported_region_snap;
            Polylines raw_patches_snap;
            Polylines after_bridge_filter_snap;
            Polylines after_overlap_remove_snap;
            Polylines simplified_patches_snap;
            Polylines to_support_snap;
        };
        std::vector<PatchDebugEntry> dbg_entries;
        dbg_entries.reserve(layer_caches.size());

        for (int upper_idx = int(layer_caches.size()) - 1; upper_idx > int(n_raft_layers); --upper_idx) {
            const size_t lower_idx = size_t(upper_idx - 1);

            PatchDebugEntry dbg;
            if constexpr (kEnablePatchDebug) {
                dbg.upper_idx = upper_idx;
                dbg.lower_idx = int(lower_idx);
                dbg.z         = support_layers[lower_idx]->print_z;
            }

            if (legal_regions[size_t(upper_idx)].empty() || legal_regions[lower_idx].empty()) {
                if constexpr (kEnablePatchDebug) {
                    dbg.skip_legal = true;
                    dbg_entries.push_back(std::move(dbg));
                }
                continue;
            }

            //const coordf_t z_gap = std::max<coordf_t>(EPSILON,support_layers[size_t(upper_idx)]->print_z - support_layers[lower_idx]->print_z);
            //const coord_t reach = line_width + coord_t(scale_(z_gap));
            const coord_t reach = line_width / 2;
            Polylines lower_contours;
            append_printed_contours(lower_contours, lower_idx);
            Polygons lower_supported = lower_contours.empty() ? Polygons{} :
                offset(lower_contours, reach, ClipperLib::jtRound, scaled<float>(0.01));
            Polylines upper_contours;
            append_contours_needing_support(upper_contours, size_t(upper_idx));
            if (upper_contours.empty()) {
                if constexpr (kEnablePatchDebug) {
                    dbg.skip_upper = true;
                    dbg.legal_lower           = legal_regions[lower_idx];
                    dbg.lower_contours_snap   = lower_contours;
                    dbg.lower_supported_snap  = lower_supported;
                    dbg_entries.push_back(std::move(dbg));
                }
                continue;
            }

            const Polygons upper_demand_region =
                offset(upper_contours, demand_region_radius, ClipperLib::jtRound, scaled<float>(0.01));
            Polygons unsupported_region = lower_supported.empty() ?
                upper_demand_region :
                diff(upper_demand_region, lower_supported);
            // Do not clip unsupported_region to legal_regions[lower_idx]: upper_contours
            // already originates from legal_regions[upper_idx], so patches are inherently
            // bounded. Clipping to the lower legal region falsely truncates ring contours
            // where the lower polygon boundary does not exactly match the upper one.

            // Early exit: a surviving patch needs at minimum a contour strip of
            // length min_patch_length and width ~line_width, i.e. area â‰
            // 2 * sqr(line_width). Anything well below that cannot produce a
            // patch that survives filter_short_contours, so skip the
            // intersection_pl + simplify chain entirely.
            const double min_unsupported_area = sqr(double(line_width));
            if (!unsupported_region.empty() && area(unsupported_region) < min_unsupported_area) {
                if constexpr (kEnablePatchDebug) {
                    dbg.skip_unsupported            = true;
                    dbg.legal_lower                 = legal_regions[lower_idx];
                    dbg.lower_contours_snap         = lower_contours;
                    dbg.lower_supported_snap        = lower_supported;
                    dbg.upper_contours_snap         = upper_contours;
                    dbg.unsupported_region_snap     = std::move(unsupported_region);
                    dbg_entries.push_back(std::move(dbg));
                }
                continue;
            }

            Polylines raw_patches = unsupported_region.empty() ? Polylines{} :
                intersection_pl(upper_contours, unsupported_region);

            // Debug: run each simplification step separately to capture intermediate
            // states. This duplicates work done by simplify_printed_patch_paths below,
            // so it is only executed when the debug flag is on.
            Polylines dbg_after_bridge;
            Polylines dbg_after_overlap;
            if constexpr (kEnablePatchDebug) {
                dbg_after_bridge = Polylines(raw_patches);
                filter_bridgeable_unsupported_contours(dbg_after_bridge, upper_contours,
                                                       max_bridge_length, merge_supported_gap);
                dbg_after_overlap = dbg_after_bridge;
                remove_overlapping_contours(dbg_after_overlap, overlap_merge_distance);
            }

            Polylines patch_paths = simplify_printed_patch_paths(
                Polylines(raw_patches), upper_contours, min_patch_length,
                max_bridge_length, merge_supported_gap, overlap_merge_distance);

            if (patch_paths.empty()) {
                if constexpr (kEnablePatchDebug) {
                    dbg.skip_unsupported            = unsupported_region.empty();
                    dbg.skip_simplified             = !unsupported_region.empty();
                    dbg.legal_lower                 = legal_regions[lower_idx];
                    dbg.lower_contours_snap         = lower_contours;
                    dbg.lower_supported_snap        = lower_supported;
                    dbg.upper_contours_snap         = upper_contours;
                    dbg.unsupported_region_snap     = unsupported_region;
                    dbg.raw_patches_snap            = std::move(raw_patches);
                    dbg.after_bridge_filter_snap    = std::move(dbg_after_bridge);
                    dbg.after_overlap_remove_snap   = std::move(dbg_after_overlap);
                    dbg_entries.push_back(std::move(dbg));
                }
                continue;
            }

            // Collect endpoints from raw_patches before simplification.
            // Any endpoint in patch_paths that is NOT in this original set is a
            // cut point produced by remove_overlapping_contours. Treat these as
            // "connected" so that stubs terminating at a cut point are correctly
            // identified as dangling by filter_short_free_contours_for_support.
            Points orig_endpoints;
            for (const Polyline& pl : raw_patches)
                if (pl.size() >= 2) {
                    orig_endpoints.emplace_back(pl.first_point());
                    orig_endpoints.emplace_back(pl.last_point());
                }
            // Index orig_endpoints with a KD-tree so the "is this an original
            // endpoint?" check is O(log N) per query instead of O(N).
            struct OrigCoord {
                const Points* pts;
                coord_t operator()(size_t idx, size_t dim) const {
                    return dim == 0 ? (*pts)[idx].x() : (*pts)[idx].y();
                }
            };
            OrigCoord orig_coord{&orig_endpoints};
            KDTreeIndirect<2, coord_t, OrigCoord>
                orig_tree(orig_coord, orig_endpoints.size());
            const coord_t attach_radius = coord_t(std::ceil(double(line_width)));

            Points cut_points;
            for (const Polyline& pl : patch_paths) {
                if (pl.size() < 2) continue;
                for (const Point& pt : {pl.first_point(), pl.last_point()}) {
                    const bool is_orig = !orig_endpoints.empty() &&
                        !find_nearby_points(orig_tree, pt, attach_radius).empty();
                    if (!is_orig)
                        cut_points.emplace_back(pt);
                }
            }

            Polylines patch_paths_to_support = patch_paths;
            filter_short_free_contours_for_support(patch_paths_to_support, max_nonpropagating_free_length,
                                                   double(line_width), cut_points);

            if constexpr (kEnablePatchDebug) {
                dbg.propagation_break       = !patch_paths.empty() && patch_paths_to_support.empty();
                dbg.legal_lower             = legal_regions[lower_idx];
                dbg.lower_contours_snap     = lower_contours;
                dbg.lower_supported_snap    = lower_supported;
                dbg.upper_contours_snap     = upper_contours;
                dbg.unsupported_region_snap = unsupported_region;
                dbg.raw_patches_snap            = std::move(raw_patches);
                dbg.after_bridge_filter_snap    = std::move(dbg_after_bridge);
                dbg.after_overlap_remove_snap   = std::move(dbg_after_overlap);
                dbg.simplified_patches_snap     = patch_paths;
                dbg.to_support_snap             = patch_paths_to_support;
                dbg_entries.push_back(std::move(dbg));
            }

            append(patch_contours[lower_idx], std::move(patch_paths));
            append(patch_contours_to_support[lower_idx], std::move(patch_paths_to_support));
        }

        // Write JSON data for external visualisation (see æ£€æŸ¥è„šæœview_patch_debug.py).
        if constexpr (kEnablePatchDebug) {
            fprintf(stderr, "[patch_debug] entries=%zu\n", dbg_entries.size());
            boost::filesystem::create_directories("C:/temp");
            const std::string json_path = "C:/temp/patch_debug.json";
            std::ofstream f(json_path);
            if (!f.is_open()) {
                fprintf(stderr, "[patch_debug] ERROR: cannot open %s\n", json_path.c_str());
            } else {
                fprintf(stderr, "[patch_debug] writing %s\n", json_path.c_str());

                // helpers --------------------------------------------------
                auto write_pts = [&](const Points& pts, bool close) {
                    f << "[";
                    for (size_t i = 0; i < pts.size(); ++i) {
                        if (i) f << ",";
                        f << "[" << std::fixed << std::setprecision(3)
                          << unscale<double>(pts[i].x()) << ","
                          << unscale<double>(pts[i].y()) << "]";
                    }
                    if (close && !pts.empty())
                        f << ",[" << unscale<double>(pts[0].x()) << ","
                                  << unscale<double>(pts[0].y()) << "]";
                    f << "]";
                };
                auto write_polylines = [&](const Polylines& pls) {
                    f << "[";
                    for (size_t i = 0; i < pls.size(); ++i) {
                        if (i) f << ",";
                        write_pts(pls[i].points, false);
                    }
                    f << "]";
                };
                auto write_polygons = [&](const Polygons& polys) {
                    f << "[";
                    for (size_t i = 0; i < polys.size(); ++i) {
                        if (i) f << ",";
                        write_pts(polys[i].points, true);
                    }
                    f << "]";
                };
                // ----------------------------------------------------------

                f << "{\"layers\":[\n";
                for (size_t ei = 0; ei < dbg_entries.size(); ++ei) {
                    const PatchDebugEntry& e = dbg_entries[ei];
                    if (ei) f << ",\n";

                    const char* st  = e.skip_legal        ? "skip_legal"       :
                                      e.skip_upper        ? "skip_upper"       :
                                      e.skip_unsupported  ? "skip_unsupported" :
                                      e.skip_simplified   ? "skip_simplified"  :
                                      e.propagation_break ? "break"            : "ok";
                    f << "{\"upper_idx\":" << e.upper_idx
                      << ",\"lower_idx\":" << e.lower_idx
                      << ",\"z\":"         << std::fixed << std::setprecision(3) << e.z
                      << ",\"status\":\""  << st << "\""
                      << ",\"stages\":{"
                      << "\"legal_lower\":";      write_polygons(e.legal_lower);
                    f << ",\"lower_contours\":";  write_polylines(e.lower_contours_snap);
                    f << ",\"lower_supported\":"; write_polygons(e.lower_supported_snap);
                    f << ",\"upper_contours\":";  write_polylines(e.upper_contours_snap);
                    f << ",\"unsupported_region\":"; write_polygons(e.unsupported_region_snap);
                    f << ",\"raw_patches\":";          write_polylines(e.raw_patches_snap);
                    f << ",\"after_bridge_filter\":";  write_polylines(e.after_bridge_filter_snap);
                    f << ",\"after_overlap_remove\":"; write_polylines(e.after_overlap_remove_snap);
                    f << ",\"simplified_patches\":";   write_polylines(e.simplified_patches_snap);
                    f << ",\"to_support\":";      write_polylines(e.to_support_snap);
                    f << "}}";
                }
                f << "\n]}\n";
                fprintf(stderr, "[patch_debug] done\n");
            }
        }
        }

        // Stage 4: emit the original exterior contours and any extra patch
        // contours. They are emitted separately only to preserve the planned
        // contour set; semantically they are one complete layer path set.
        for (size_t layer_idx = n_raft_layers; layer_idx < layer_caches.size(); ++layer_idx) {
            SupportGeneratorLayerExtruded& base_layer = layer_caches[layer_idx].base_layer;
            if (legal_regions[layer_idx].empty() || base_layer.empty())
                continue;

            SupportParameters support_params2 = support_params;
            Flow flow = support_params.support_material_flow.with_height(float(base_layer.layer->height));
            tree_supports_generate_paths(base_layer.extrusions, legal_regions[layer_idx], flow, support_params2);
            if (!patch_contours[layer_idx].empty())
                extrusion_entities_append_paths(base_layer.extrusions, std::move(patch_contours[layer_idx]),
                                                ExtrusionRole::erSupportMaterial, flow.mm3_per_mm(), flow.width(), flow.height(), false);
        }
    }

    // Now modulate the support layer height in parallel.
    tbb::parallel_for(tbb::blocked_range<size_t>(n_raft_layers, support_layers.size()),
                      [&support_layers, &layer_caches, &support_params, &bbox_object](const tbb::blocked_range<size_t>& range) {
                          for (size_t support_layer_id = range.begin(); support_layer_id < range.end(); ++support_layer_id) {
                              SupportLayer& support_layer = *support_layers[support_layer_id];
                              LayerCache&   layer_cache   = layer_caches[support_layer_id];
                              // For all extrusion types at this print_z, ordered by decreasing layer height:
                              for (LayerCacheItem& layer_cache_item : layer_cache.nonempty) {
                                  // Trim the extrusion height from the bottom by the overlapping layers.
                                  modulate_extrusion_by_overlapping_layers(layer_cache_item.layer_extruded->extrusions,
                                                                           *layer_cache_item.layer_extruded->layer,
                                                                           layer_cache_item.overlapping);
                                  support_layer.support_fills.append(std::move(layer_cache_item.layer_extruded->extrusions));
                              }

                              // Orca: Generate iron toolpath for contact layer
                              if (!layer_cache.polys_to_iron.empty()) {
                                  auto f = std::unique_ptr<Fill>(Fill::new_from_type(support_params.ironing_pattern));
                                  f->set_bounding_box(bbox_object);
                                  f->layer_id        = support_layer.id();
                                  f->z               = support_layer.print_z;
                                  f->overlap         = 0;
                                  f->angle           = layer_cache.ironing_angle;
                                  f->spacing         = support_params.ironing_spacing;
                                  f->link_max_length = (coord_t) scale_(3. * f->spacing);

                                  ExPolygons polys_to_iron = union_safety_offset_ex(layer_cache.polys_to_iron);
                                  layer_cache.polys_to_iron.clear();

                                  // Find the layer above that directly overlaps current layer, clip the overlapped part
                                  if (support_layer_id < support_layers.size() - 1) {
                                      const auto& upper_layer = support_layers[support_layer_id + 1];
                                      if (!upper_layer->support_islands.empty() &&
                                          upper_layer->bottom_z() <= support_layer.print_z + EPSILON) {
                                          polys_to_iron = diff_ex(polys_to_iron, upper_layer->support_islands);
                                      }
                                  }

                                  fill_expolygons_generate_paths(
                                      // Destination
                                      support_layer.support_fills.entities,
                                      // Regions to fill
                                      std::move(polys_to_iron),
                                      // Filler and its parameters
                                      f.get(), 1.f,
                                      // Extrusion parameters
                                      ExtrusionRole::erIroning, support_params.ironing_flow);
                              }
                          }
                      });

#ifndef NDEBUG
    struct Test
    {
        static bool verify_nonempty(const ExtrusionEntityCollection* collection)
        {
            for (const ExtrusionEntity* ee : collection->entities) {
                if (const ExtrusionPath* path = dynamic_cast<const ExtrusionPath*>(ee))
                    assert(!path->empty());
                else if (const ExtrusionMultiPath* multipath = dynamic_cast<const ExtrusionMultiPath*>(ee))
                    assert(!multipath->empty());
                else if (const ExtrusionEntityCollection* eecol = dynamic_cast<const ExtrusionEntityCollection*>(ee)) {
                    assert(!eecol->empty());
                    return verify_nonempty(eecol);
                } else
                    assert(false);
            }
            return true;
        }
    };
    for (const SupportLayer* support_layer : support_layers)
        assert(Test::verify_nonempty(&support_layer->support_fills));
#endif // NDEBUG
}

/*
void PrintObjectSupportMaterial::clip_by_pillars(
    const PrintObject   &object,
    LayersPtr           &bottom_contacts,
    LayersPtr           &top_contacts,
    LayersPtr           &intermediate_contacts);

{
    // this prevents supplying an empty point set to BoundingBox constructor
    if (top_contacts.empty())
        return;

    coord_t pillar_size    = scale_(PILLAR_SIZE);
    coord_t pillar_spacing = scale_(PILLAR_SPACING);

    // A regular grid of pillars, filling the 2D bounding box.
    Polygons grid;
    {
        // Rectangle with a side of 2.5x2.5mm.
        Polygon pillar;
        pillar.points.push_back(Point(0, 0));
        pillar.points.push_back(Point(pillar_size, 0));
        pillar.points.push_back(Point(pillar_size, pillar_size));
        pillar.points.push_back(Point(0, pillar_size));

        // 2D bounding box of the projection of all contact polygons.
        BoundingBox bbox;
        for (LayersPtr::const_iterator it = top_contacts.begin(); it != top_contacts.end(); ++ it)
            bbox.merge(get_extents((*it)->polygons));
        grid.reserve(size_t(ceil(bb.size()(0) / pillar_spacing)) * size_t(ceil(bb.size()(1) / pillar_spacing)));
        for (coord_t x = bb.min(0); x <= bb.max(0) - pillar_size; x += pillar_spacing) {
            for (coord_t y = bb.min(1); y <= bb.max(1) - pillar_size; y += pillar_spacing) {
                grid.push_back(pillar);
                for (size_t i = 0; i < pillar.points.size(); ++ i)
                    grid.back().points[i].translate(Point(x, y));
            }
        }
    }

    // add pillars to every layer
    for my $i (0..n_support_z) {
        $shape->[$i] = [ @$grid ];
    }

    // build capitals
    for my $i (0..n_support_z) {
        my $z = $support_z->[$i];

        my $capitals = intersection(
            $grid,
            $contact->{$z} // [],
        );

        // work on one pillar at time (if any) to prevent the capitals from being merged
        // but store the contact area supported by the capital because we need to make
        // sure nothing is left
        my $contact_supported_by_capitals = [];
        foreach my $capital (@$capitals) {
            // enlarge capital tops
            $capital = offset([$capital], +($pillar_spacing - $pillar_size)/2);
            push @$contact_supported_by_capitals, @$capital;

            for (my $j = $i-1; $j >= 0; $j--) {
                my $jz = $support_z->[$j];
                $capital = offset($capital, -$self->interface_flow->scaled_width/2);
                last if !@$capitals;
                push @{ $shape->[$j] }, @$capital;
            }
        }

        // Capitals will not generally cover the whole contact area because there will be
        // remainders. For now we handle this situation by projecting such unsupported
        // areas to the ground, just like we would do with a normal support.
        my $contact_not_supported_by_capitals = diff(
            $contact->{$z} // [],
            $contact_supported_by_capitals,
        );
        if (@$contact_not_supported_by_capitals) {
            for (my $j = $i-1; $j >= 0; $j--) {
                push @{ $shape->[$j] }, @$contact_not_supported_by_capitals;
            }
        }
    }
}

sub clip_with_shape {
    my ($self, $support, $shape) = @_;

    foreach my $i (keys %$support) {
        // don't clip bottom layer with shape so that we
        // can generate a continuous base flange
        // also don't clip raft layers
        next if $i == 0;
        next if $i < $self->object_config->raft_layers;
        $support->{$i} = intersection(
            $support->{$i},
            $shape->[$i],
        );
    }
}
*/

/*!
 * \brief Unions two Polygons. Ensures that if the input is non empty that the output also will be non empty.
 * \param first[in] The first Polygon.
 * \param second[in] The second Polygon.
 * \return The union of both Polygons
 */
[[nodiscard]] Polygons safe_union(const Polygons& first, const Polygons& second)
{
    // unionPolygons can slowly remove Polygons under certain circumstances, because of rounding issues (Polygons that have a thin area).
    // This does not cause a problem when actually using it on large areas, but as influence areas (representing centerpoints) can be very
    // thin, this does occur so this ugly workaround is needed Here is an example of a Polygons object that will loose vertices when
    // unioning, and will be gone after a few times unionPolygons was called:
    /*
    Polygons example;
    Polygon exampleInner;
    exampleInner.add(Point(120410,83599));//A
    exampleInner.add(Point(120384,83643));//B
    exampleInner.add(Point(120399,83618));//C
    exampleInner.add(Point(120414,83591));//D
    exampleInner.add(Point(120423,83570));//E
    exampleInner.add(Point(120419,83580));//F
    example.add(exampleInner);
    for(int i=0;i<10;i++){
         log("Iteration %d Example area: %f\n",i,area(example));
         example=example.unionPolygons();
    }
*/

    Polygons result;
    if (!first.empty() || !second.empty()) {
        result = first.empty() ? union_(second) : second.empty() ? union_(first) : union_(first, second);
        if (result.empty()) {
            BOOST_LOG_TRIVIAL(debug) << "Caught an area destroying union, enlarging areas a bit.";
            // just take the few lines we have, and offset them a tiny bit. Needs to be offsetPolylines, as offset may aleady have problems
            // with the area.
            result = union_(offset(to_polylines(first), scaled<float>(0.002), jtMiter, 1.2),
                            offset(to_polylines(second), scaled<float>(0.002), jtMiter, 1.2));
        }
    }

    return result;
}
[[nodiscard]] ExPolygons safe_union(const ExPolygons& first, const ExPolygons& second)
{
    ExPolygons result;
    if (!first.empty() || !second.empty()) {
        result = first.empty() ? union_ex(second) : second.empty() ? union_ex(first) : union_ex(first, second);
        if (result.empty()) {
            BOOST_LOG_TRIVIAL(debug) << "Caught an area destroying union, enlarging areas a bit.";
            // just take the few lines we have, and offset them a tiny bit. Needs to be offsetPolylines, as offset may aleady have problems
            // with the area.
            Polygons result_polys = union_(offset(to_polylines(first), scaled<float>(0.002), jtMiter, 1.2),
                                           offset(to_polylines(second), scaled<float>(0.002), jtMiter, 1.2));
            for (auto& poly : result_polys)
                result.emplace_back(ExPolygon(poly));
        }
    }

    return result;
}

/*!
 * \brief Offsets (increases the area of) a polygons object in multiple steps to ensure that it does not lag through over a given obstacle.
 * \param me[in] Polygons object that has to be offset.
 * \param distance[in] The distance by which me should be offset. Expects values >=0.
 * \param collision[in] The area representing obstacles.
 * \param last_step_offset_without_check[in] The most it is allowed to offset in one step.
 * \param min_amount_offset[in] How many steps have to be done at least. As this uses round offset this increases the amount of vertices,
 * which may be required if Polygons get very small. Required as arcTolerance is not exposed in offset, which should result with a similar
 * result. \return The resulting Polygons object.
 */
[[nodiscard]] Polygons safe_offset_inc(const Polygons& me,
                                       coord_t         distance,
                                       const Polygons& collision,
                                       coord_t         safe_step_size,
                                       coord_t         last_step_offset_without_check,
                                       size_t          min_amount_offset)
{
    bool     do_final_difference = last_step_offset_without_check == 0;
    Polygons ret                 = safe_union(me); // ensure sane input

    // Trim the collision polygons with the region of interest for diff() efficiency.
    Polygons collision_trimmed_buffer;
    auto     collision_trimmed = [&collision_trimmed_buffer, &collision, &ret, distance]() -> const Polygons& {
        if (collision_trimmed_buffer.empty() && !collision.empty()) {
            coord_t _distance        = distance < 0 ? 0 : distance;
            collision_trimmed_buffer = ClipperUtils::clip_clipper_polygons_with_subject_bbox(collision, get_extents(ret).inflated(
                                                                                                            _distance + SCALED_EPSILON));
        }

        return collision_trimmed_buffer;
    };

    if (distance == 0)
        return do_final_difference ? diff(ret, collision_trimmed()) : union_(ret);
    if (safe_step_size < 0 || last_step_offset_without_check < 0) {
        BOOST_LOG_TRIVIAL(error) << "Offset increase got invalid parameter!";
        return do_final_difference ? diff(ret, collision_trimmed()) : union_(ret);
    }

    coord_t step_size = safe_step_size;
    int     steps     = distance > last_step_offset_without_check ? (distance - last_step_offset_without_check) / step_size : 0;
    if (distance - steps * step_size > last_step_offset_without_check) {
        if ((steps + 1) * step_size <= distance)
            // This will be the case when last_step_offset_without_check >= safe_step_size
            ++steps;
        else
            do_final_difference = true;
    }
    if (steps + (distance < last_step_offset_without_check || (distance % step_size) != 0) < int(min_amount_offset) &&
        min_amount_offset > 1) {
        // yes one can add a bool as the standard specifies that a result from compare operators has to be 0 or 1
        // reduce the stepsize to ensure it is offset the required amount of times
        step_size = distance / min_amount_offset;
        if (step_size >= safe_step_size) {
            // effectivly reduce last_step_offset_without_check
            step_size = safe_step_size;
            steps     = min_amount_offset;
        } else
            steps = distance / step_size;
    }
    // offset in steps
    for (int i = 0; i < steps; ++i) {
        ret = diff(offset(ret, step_size, ClipperLib::jtRound, scaled<float>(0.01)), collision_trimmed());
        // ensure that if many offsets are done the performance does not suffer extremely by the new vertices of jtRound.
        if (i % 10 == 7)
            ret = polygons_simplify(ret, scaled<double>(0.015));
    }
    // offset the remainder
    float last_offset = distance - steps * step_size;
    if (last_offset > SCALED_EPSILON)
        ret = offset(ret, distance - steps * step_size, ClipperLib::jtRound, scaled<float>(0.01));
    ret = polygons_simplify(ret, scaled<double>(0.015));

    if (do_final_difference)
        ret = diff(ret, collision_trimmed());
    return union_(ret);
}

} // namespace Slic3r
