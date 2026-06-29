// [FORMATTED BY CLANG-FORMAT 2026-05-13 14:19:40]
#include "OrganicSupportValidation.hpp"
#include "TreeModelVolumes.hpp"
#include "TreeSupportCommon.hpp"
#include "SupportLayer.hpp"

#include "../Print.hpp"
#include "../ClipperUtils.hpp"
#include "../Polygon.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <mutex>
#include <utility>
#include <vector>

namespace Slic3r { namespace TreeSupport3D {

static constexpr coord_t OVERLAP_TOLERANCE = 150; // 0.15 mm in microns

static double get_min_floating_area()
{
    static const double a = Slic3r::sqr(Slic3r::scaled<double>(0.5));
    return a;
}

static double scaled_area_to_mm2(double scaled_area) { return std::abs(scaled_area) * sqr(SCALING_FACTOR); }

// Debug files are intentionally relative to the process working directory so
// developer-only traces do not depend on a local source checkout path.
static void append_debug(const std::string& msg, bool clear_first = false)
{
    try {
        std::ofstream ofs("support_debug.txt", clear_first ? std::ios::trunc : std::ios::app);
        if (ofs.is_open())
            ofs << msg << '\n';
    } catch (...) {}
}

static void append_floating_debug(const std::string& msg, bool clear_first = false)
{
    static std::mutex           mtx;
    std::lock_guard<std::mutex> lk(mtx);
    try {
        std::ofstream ofs("organic_floating_debug.txt", clear_first ? std::ios::trunc : std::ios::app);
        if (ofs.is_open())
            ofs << msg << '\n';
    } catch (...) {}
}

static size_t max_stack_size(const SupportGeneratorLayersPtr& bc, const SupportGeneratorLayersPtr& tc, const SupportGeneratorLayersPtr& im)
{
    return std::max({bc.size(), tc.size(), im.size()});
}

static void append_layer_polygons(Polygons& dst, const SupportGeneratorLayersPtr& layers, size_t idx)
{
    if (idx < layers.size() && layers[idx] != nullptr && !layers[idx]->polygons.empty())
        append(dst, layers[idx]->polygons);
}

static Polygons combined_support_polygons_at(const SupportGeneratorLayersPtr& bc,
                                             const SupportGeneratorLayersPtr& tc,
                                             const SupportGeneratorLayersPtr& im,
                                             size_t                           idx)
{
    // A support island may be split across bottom contact, intermediate support,
    // and top contact containers. Connectivity diagnostics must see the merged
    // layer footprint, otherwise a supported island can look floating.
    Polygons out;
    append_layer_polygons(out, bc, idx);
    append_layer_polygons(out, im, idx);
    append_layer_polygons(out, tc, idx);
    return out.empty() ? out : union_(out);
}

static void debug_detect_combined_stack_connectivity(const TreeModelVolumes&          volumes,
                                                     const TreeSupportSettings&       config,
                                                     const SupportGeneratorLayersPtr& bc,
                                                     const SupportGeneratorLayersPtr& tc,
                                                     const SupportGeneratorLayersPtr& im,
                                                     const std::string&               tag)
{
    // This diagnostic is deliberately file-only. The candidate list can be large
    // on dense organic supports and would make normal application logs noisy.
    const size_t num_layers = max_stack_size(bc, tc, im);
    if (num_layers == 0)
        return;

    const coord_t tol             = config.xy_distance > 0 ? config.xy_distance / 2 : OVERLAP_TOLERANCE;
    const double  min_contact     = get_min_floating_area();
    const double  min_report_area = std::max(get_min_floating_area(), sqr(scaled<double>(1.0)));
    int           n_reported      = 0;
    int           n_skipped       = 0;
    int           n_no_below      = 0;
    int           n_no_above      = 0;

    append_floating_debug("");
    append_floating_debug("=== Combined Stack Connectivity: " + tag + " ===");
    append_floating_debug("  overlap_tol_mm: " + std::to_string(tol * SCALING_FACTOR));
    append_floating_debug("  min_report_area_mm2: " + std::to_string(scaled_area_to_mm2(min_report_area)));

    for (size_t li = 0; li < num_layers; ++li) {
        Polygons cur = combined_support_polygons_at(bc, tc, im, li);
        if (cur.empty())
            continue;

        Polygons       below     = li > 0 ? combined_support_polygons_at(bc, tc, im, li - 1) : Polygons{};
        Polygons       above     = li + 1 < num_layers ? combined_support_polygons_at(bc, tc, im, li + 1) : Polygons{};
        Polygons       below_exp = below.empty() ? Polygons{} : offset(below, tol);
        Polygons       above_exp = above.empty() ? Polygons{} : offset(above, tol);
        const Polygons placeable = config.support_rests_on_model ? volumes.getPlaceableAreas(0, LayerIndex(li), [] {}) : Polygons{};

        int iidx = 0;
        for (const ExPolygon& isl : union_ex(cur)) {
            Polygons     ip       = to_polygons(isl);
            const double isl_area = area(ip);
            if (isl_area < min_report_area) {
                ++n_skipped;
                continue;
            }

            const double below_ov  = li == 0 ? isl_area : (below_exp.empty() ? 0. : area(intersection(ip, below_exp)));
            const double model_ov  = placeable.empty() ? 0. : area(intersection(ip, placeable));
            const double above_ov  = above_exp.empty() ? 0. : area(intersection(ip, above_exp));
            const bool   has_below = li == 0 || below_ov >= min_contact || model_ov >= min_contact;
            const bool   has_above = above_ov >= min_contact || li + 1 == num_layers;

            if (!has_below)
                ++n_no_below;
            if (!has_above)
                ++n_no_above;

            if (!has_below || !has_above) {
                ++n_reported;
                BoundingBox bbox = get_extents(ip);
                append_floating_debug(
                    "  [COMBINED_FLOAT_CANDIDATE] tag=" + tag + " L" + std::to_string(li) + " island=" + std::to_string(iidx) +
                    " area_mm2=" + std::to_string(scaled_area_to_mm2(isl_area)) + " below_ov_mm2=" +
                    std::to_string(scaled_area_to_mm2(below_ov)) + " model_ov_mm2=" + std::to_string(scaled_area_to_mm2(model_ov)) +
                    " above_ov_mm2=" + std::to_string(scaled_area_to_mm2(above_ov)) + " has_below=" + std::to_string(has_below) +
                    " has_above=" + std::to_string(has_above) + " bbox_min=(" + std::to_string(bbox.min.x() * SCALING_FACTOR) + "," +
                    std::to_string(bbox.min.y() * SCALING_FACTOR) + ")" + " bbox_max=(" + std::to_string(bbox.max.x() * SCALING_FACTOR) +
                    "," + std::to_string(bbox.max.y() * SCALING_FACTOR) + ")");
            }
            ++iidx;
        }
    }

    append_floating_debug("  summary: reported=" + std::to_string(n_reported) + " no_below=" + std::to_string(n_no_below) +
                          " no_above=" + std::to_string(n_no_above) + " skipped_small=" + std::to_string(n_skipped));
    append_floating_debug("=== Combined Stack Connectivity Complete: " + tag + " ===");
}

// Commits pending layer writes produced by a successful try_grow_downward call.
// Also accumulates each write into grown_additions_by_layer and grounded_cache,
// because every committed grow-path polygon is by construction connected to ground.
static void commit_pending_writes(const std::vector<std::pair<LayerIndex, Polygons>>& pending,
                                  PrintObject&                                        print_object,
                                  const TreeSupportSettings&                          config,
                                  SupportGeneratorLayersPtr&                          im,
                                  SupportGeneratorLayerStorage&                       layer_storage,
                                  std::vector<Polygons>&                              grown_additions_by_layer,
                                  std::vector<Polygons>&                              grounded_cache)
{
    for (const auto& [l, polys] : pending) {
        if (l < 0 || size_t(l) >= im.size())
            continue;
        if (im[l] == nullptr)
            im[l] = &layer_allocate(layer_storage, SupporLayerType::sltBase, print_object.slicing_parameters(), config, size_t(l));
        im[l]->polygons = union_(im[l]->polygons, polys);

        if (size_t(l) < grown_additions_by_layer.size())
            grown_additions_by_layer[l] = union_(grown_additions_by_layer[l], polys);

        // Committed grow-path polygons are proven grounded; make them visible to
        // subsequent try_grow_downward calls so they can use grounded stop conditions.
        if (size_t(l) < grounded_cache.size())
            grounded_cache[l] = union_(grounded_cache[l], polys);
    }
}

enum class GrowResult { kConnected, kBlocked };

// Simulates downward growth from start_layer without touching intermediate_layers.
// On kConnected the caller commits pending_writes; on kBlocked pending_writes is left empty,
// so no partial state is ever written — ensuring atomicity.
// Uses grounded_cache (updated after each commit) for stop-condition checks so that a
// floating island at l-1 never causes a premature kConnected return.
static GrowResult try_grow_downward(const Polygons&                               island,
                                    LayerIndex                                    start_layer,
                                    const TreeModelVolumes&                       volumes,
                                    const TreeSupportSettings&                    config,
                                    const std::vector<Polygons>&                  grounded_cache,
                                    std::vector<std::pair<LayerIndex, Polygons>>& pending_writes,
                                    const std::function<void()>&                  throw_on_cancel)
{
    const double   min_contact = get_min_floating_area();
    const coord_t  tol         = config.xy_distance > 0 ? config.xy_distance / 2 : OVERLAP_TOLERANCE;
    const Polygons bed         = {volumes.m_bed_area};

    pending_writes.clear();
    Polygons cur = island;

    for (LayerIndex l = start_layer; l >= 0; --l) {
        throw_on_cancel();

        // Clip by model body
        const Polygons& coll = volumes.getCollision(0, l, false);
        if (!coll.empty())
            cur = diff(cur, coll);

        // Clip to printable bed area
        cur = intersection(cur, bed);

        // Drop tiny fragments
        cur.erase(std::remove_if(cur.begin(), cur.end(), [&](const Polygon& p) { return std::abs(p.area()) < min_contact; }), cur.end());

        if (cur.empty()) {
            // Growth is blocked: discard all pending writes — nothing is committed.
            append_debug("    [BLOCKED] L" + std::to_string(l) + " area zeroed by clipping");
            pending_writes.clear();
            return GrowResult::kBlocked;
        }

        // Buffer this layer's contribution; commit only after kConnected is confirmed.
        // This keeps the original support stack unchanged if a later layer clips the
        // grow path to nothing.
        pending_writes.emplace_back(l, cur);

        // Stop: reached build plate
        if (l == 0)
            return GrowResult::kConnected;

        // Stop: rests on model surface
        if (config.support_rests_on_model) {
            const Polygons& pl = volumes.getPlaceableAreas(0, l, throw_on_cancel);
            if (!pl.empty() && area(intersection(cur, pl)) >= min_contact)
                return GrowResult::kConnected;
        }

        // Stop: overlaps grounded support one layer below.
        // grounded_cache is updated by commit_pending_writes so newly-committed grow chains
        // are visible here; using raw combined would allow a floating island at l-1 to
        // cause a premature kConnected that loses its root when l-1 is later blocked.
        if (size_t(l - 1) < grounded_cache.size() && !grounded_cache[l - 1].empty() &&
            area(intersection(cur, offset(grounded_cache[l - 1], tol))) >= min_contact)
            return GrowResult::kConnected;
    }

    return GrowResult::kConnected;
}

void validate_and_fix_floating_supports(PrintObject&                  print_object,
                                        const TreeModelVolumes&       volumes,
                                        const TreeSupportSettings&    config,
                                        SupportGeneratorLayersPtr&    bottom_contacts,
                                        SupportGeneratorLayersPtr&    top_contacts,
                                        SupportGeneratorLayersPtr&    intermediate_layers,
                                        SupportGeneratorLayerStorage& layer_storage,
                                        std::function<void()>         throw_on_cancel)
{
    // Conservative repair strategy:
    // 1. Build immutable snapshots of the original support stack.
    // 2. Identify islands that are not connected to grounded material below.
    // 3. Grow only those islands downward, committing the whole path atomically.
    // This avoids cascading decisions based on support that was just generated in
    // the same pass, while still allowing committed repair paths to ground later
    // islands through grounded_cache.
    append_floating_debug("=== Organic Floating Debug: validation ===", false);
    append_debug("=== Starting Validation (single-pass dual-snapshot) ===", true);
    debug_detect_combined_stack_connectivity(volumes, config, bottom_contacts, top_contacts, intermediate_layers, "before_fix");

    const size_t interm_size = intermediate_layers.size();
    if (interm_size < 2) {
        debug_detect_combined_stack_connectivity(volumes, config, bottom_contacts, top_contacts, intermediate_layers, "after_fix");
        append_debug("=== Validation Complete (nothing to process) ===");
        return;
    }

    const size_t  num_layers  = max_stack_size(bottom_contacts, top_contacts, intermediate_layers);
    const coord_t tol         = config.xy_distance > 0 ? config.xy_distance / 2 : OVERLAP_TOLERANCE;
    const double  min_contact = get_min_floating_area();

    // Step 1: Build snapshots — all immutable during the entire pass.
    //
    // combined_cache[l]: union of all three stacks at layer l (used by grow-path stop
    //                    conditions and the debug logger).
    //
    // grounded_cache[l]: subset of combined_cache[l] that is provably connected to the
    //                    build plate or model surface.  Computed bottom-up with island
    //                    granularity: if ANY part of an island overlaps with grounded
    //                    material one layer below (>= min_contact), the WHOLE island is
    //                    added — this avoids the progressive area loss that would occur if
    //                    we only kept the intersection strip.
    //                    Used for has_below detection so that a floating island at L is not
    //                    falsely considered grounded just because combined_cache[L-1] is
    //                    non-empty (the material at L-1 might itself be floating).
    //
    // orig_im[l]: copy of intermediate_layers[l]->polygons before any modification.
    std::vector<Polygons> combined_cache(num_layers);
    std::vector<Polygons> grounded_cache(num_layers);
    std::vector<Polygons> orig_im(interm_size);

    // combined_cache is a read-only snapshot for the whole pass. Mutations go to
    // intermediate_layers only after a grow path is proven connected.
    for (size_t l = 0; l < num_layers; ++l)
        combined_cache[l] = combined_support_polygons_at(bottom_contacts, top_contacts, intermediate_layers, l);

    // L=0: directly on the build plate — everything there is grounded.
    grounded_cache[0] = combined_cache[0];

    for (size_t l = 1; l < num_layers; ++l) {
        if (combined_cache[l].empty())
            continue;

        const Polygons& prev_grounded = grounded_cache[l - 1];
        Polygons        below_exp     = prev_grounded.empty() ? Polygons{} : offset(prev_grounded, tol);

        const Polygons* placeable_ptr = nullptr;
        if (config.support_rests_on_model)
            placeable_ptr = &volumes.getPlaceableAreas(0, LayerIndex(l), throw_on_cancel);

        Polygons grounded_here;
        for (const ExPolygon& isl : union_ex(combined_cache[l])) {
            Polygons ip          = to_polygons(isl);
            // Groundedness is island-based rather than overlap-strip based. Keeping
            // the whole island prevents support footprints from shrinking layer by
            // layer during validation.
            bool     is_grounded = (!below_exp.empty() && area(intersection(ip, below_exp)) >= min_contact) ||
                               (placeable_ptr != nullptr && !placeable_ptr->empty() &&
                                area(intersection(ip, *placeable_ptr)) >= min_contact);
            if (is_grounded)
                append(grounded_here, ip);
        }
        grounded_cache[l] = grounded_here.empty() ? Polygons{} : union_(grounded_here);
    }

    for (size_t l = 0; l < interm_size; ++l)
        orig_im[l] = (intermediate_layers[l] != nullptr) ? intermediate_layers[l]->polygons : Polygons{};

    // Explicit per-layer accounting of grow-path polygons committed from higher layers.
    // Used at write-back to avoid the lossy diff(live, orig) approach.
    std::vector<Polygons> grown_additions_by_layer(interm_size);

    int n_processed = 0, n_kept = 0, n_grown = 0, n_blocked = 0;

    // Reusable buffer for transactional pending writes.
    std::vector<std::pair<LayerIndex, Polygons>> pending;

    // Step 2: Single top-down pass (L = interm_size-1 downto 1).
    // L=0 is never floating — it sits on the build plate by definition.
    for (LayerIndex L = LayerIndex(interm_size) - 1; L >= 1; --L) {
        throw_on_cancel();

        if (orig_im[L].empty())
            continue;

        // Enumerate islands from the immutable snapshot so that grow-path polygons
        // committed to lower layers by earlier iterations are never re-processed.
        ExPolygons islands = union_ex(orig_im[L]);

        const Polygons* placeable_L = nullptr;
        if (config.support_rests_on_model)
            placeable_L = &volumes.getPlaceableAreas(0, L, throw_on_cancel);

        Polygons kept;
        int      n_blocked_here = 0;

        for (const ExPolygon& isl : islands) {
            ++n_processed;
            Polygons ip = to_polygons(isl);

            // Small islands: keep unconditionally — conservative behavior, no grow/discard.
            if (area(ip) < min_contact) {
                append(kept, ip);
                ++n_kept;
                continue;
            }

            // Check groundedness via snapshot to prevent cascade false-positives.
            // Check groundedness against grounded_cache: only material proven connected to
            // the build plate / model surface counts.  combined_cache would give false
            // has_below=true whenever L-1 material is itself floating.
            bool            has_below  = false;
            const Polygons& below_snap = grounded_cache[size_t(L) - 1];
            if (!below_snap.empty())
                has_below = area(intersection(ip, offset(below_snap, tol))) >= min_contact;
            if (!has_below && placeable_L != nullptr && !placeable_L->empty())
                has_below = area(intersection(ip, *placeable_L)) >= min_contact;

            if (has_below) {
                append(kept, ip);
                ++n_kept;
                continue;
            }

            // Only unsupported islands reach this point. The downward grow attempt
            // either creates a complete grounded path or discards all pending writes.
            append_debug("  [FLOAT] L" + std::to_string(L) + " area=" + std::to_string(scaled_area_to_mm2(area(ip))) + "mm2");

            // Simulate downward growth — no writes to intermediate_layers yet.
            GrowResult r = try_grow_downward(ip, L - 1, volumes, config, grounded_cache, pending, throw_on_cancel);

            if (r == GrowResult::kConnected) {
                // Atomically commit all buffered layer writes; also updates grounded_cache
                // so subsequent grow operations see newly-rooted material.
                commit_pending_writes(pending, print_object, config, intermediate_layers, layer_storage, grown_additions_by_layer,
                                      grounded_cache);
                append(kept, ip);
                ++n_grown;
            } else {
                // kBlocked: pending is already cleared — no partial writes occur.
                ++n_blocked;
                ++n_blocked_here;
                append_debug("  [DISCARD] L" + std::to_string(L) + " island blocked, removed");
            }
        }

        if (n_blocked_here > 0) {
            // Some original islands were discarded. Reconstruct this layer's content
            // from the grounded originals plus explicitly-tracked grow-path contributions
            // from higher layers. Avoids the lossy diff(live, orig) approach which drops
            // any grow-path area that geometrically overlaps with orig_im[L].
            intermediate_layers[L]->polygons = union_(kept, grown_additions_by_layer[L]);
        }
        // When n_blocked_here == 0: intermediate_layers[L]->polygons already equals
        // union(orig_im[L], grow-path additions), which is the correct final state.
    }

    append_debug("  Summary: processed=" + std::to_string(n_processed) + " kept=" + std::to_string(n_kept) +
                 " grown=" + std::to_string(n_grown) + " blocked=" + std::to_string(n_blocked));
    debug_detect_combined_stack_connectivity(volumes, config, bottom_contacts, top_contacts, intermediate_layers, "after_fix");
    append_debug("=== Validation Complete ===");
}

}} // namespace Slic3r::TreeSupport3D
