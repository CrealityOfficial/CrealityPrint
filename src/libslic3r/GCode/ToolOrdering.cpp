#include "Print.hpp"
#include "ToolOrdering.hpp"
#include "Layer.hpp"
#include "ClipperUtils.hpp"
#include "ParameterUtils.hpp"
#include "ExtrusionEntity.hpp"
#include "ShortestPath.hpp"
#include "libslic3r/FDM/MachineVender.hpp"

// #define SLIC3R_DEBUG

// Make assert active if SLIC3R_DEBUG
#ifdef SLIC3R_DEBUG
    #define DEBUG
    #define _DEBUG
    #undef NDEBUG
#endif

#include <cassert>
#include <limits>
#include <algorithm>
#include <unordered_set>

#include <libslic3r.h>
#include <tbb/parallel_for.h>

namespace Slic3r {

const static bool g_wipe_into_objects = false;

// Maximum number of extruders for exact TSP bitmask DP (2^n states).
// Beyond this threshold, fall back to O(n^2) greedy nearest-neighbor to avoid std::bad_alloc.
// n=16: 2^16 * 16 * 4B = 4MB, acceptable; n=17 would be 8MB+.
static constexpr int k_tsp_exact_max_extruders = 20;

// Helper to resolve a mixed filament ID to a physical extruder, considering
// optional layer-height cadence overrides from the print settings.
static unsigned int resolve_mixed_with_layer_heights(const MixedFilamentManager *mixed_mgr,
                                              size_t                      num_physical,
                                              unsigned int                filament_id_1based,
                                              int                         layer_index,
                                              float                       layer_print_z,
                                              float                       layer_height,
                                              float                       layer_height_a,
                                              float                       layer_height_b,
                                              float                       base_layer_height)
{
    if (mixed_mgr == nullptr || filament_id_1based <= num_physical)
        return filament_id_1based;
    if (!mixed_mgr->is_mixed(filament_id_1based, num_physical))
        return filament_id_1based;

    const MixedFilament *mixed_row = mixed_mgr->mixed_filament_from_id(filament_id_1based, num_physical);
    const bool is_custom_mixed = mixed_row != nullptr && mixed_row->custom;

    if (!is_custom_mixed && (layer_height_a > 0.f || layer_height_b > 0.f)) {
        const float safe_base = std::max<float>(0.01f, base_layer_height);
        const int ratio_a = std::max(1, int(std::lround((layer_height_a > 0.f ? layer_height_a : safe_base) / safe_base)));
        const int ratio_b = std::max(1, int(std::lround((layer_height_b > 0.f ? layer_height_b : safe_base) / safe_base)));
        const int cycle   = ratio_a + ratio_b;

        if (cycle > 0) {
            if (mixed_row != nullptr) {
                const int pos = ((layer_index % cycle) + cycle) % cycle;
                return pos < ratio_a ? mixed_row->component_a : mixed_row->component_b;
            }
        }
    }

    return mixed_mgr->resolve(filament_id_1based, num_physical, layer_index, layer_print_z, layer_height);
}
static bool can_flush_into_skeleton(const ExtrusionEntityCollection& eec, const PrintConfig& print_config, const PrintObject& object)
{
    return creality::is_k2_series_printer_from_string(print_config.printer_model.value)
        && object.config().flush_into_skeleton.value
        && eec.role() == erInternalInfill;
}
static ExtrusionEntityCollection* new_collection_like(const ExtrusionEntityCollection& source)
{
    auto* out = new ExtrusionEntityCollection();
    out->no_sort = source.no_sort;
    out->loop_node_range = source.loop_node_range;
    if (!source.can_reverse())
        out->set_reverse();
    return out;
}

static ExtrusionPath* clone_path_with_polyline(const ExtrusionPath& source, Polyline&& polyline)
{
    auto* path = dynamic_cast<ExtrusionPath*>(source.clone());
    if (path != nullptr)
        path->polyline = std::move(polyline);
    return path;
}

static bool split_skeleton_collection_for_wipe(ExtrusionEntityCollection& fills, size_t fill_idx, float volume_limit,
                                           ExtrusionEntityCollection*& wipe_collection, float& wipe_volume)
{
    wipe_collection = nullptr;
    wipe_volume = 0.f;

    if (volume_limit <= EPSILON || fill_idx >= fills.entities.size())
        return false;

    auto* source = dynamic_cast<ExtrusionEntityCollection*>(fills.entities[fill_idx]);
    if (source == nullptr)
        return false;

    const double source_volume = source->total_volume();
    if (source_volume <= double(volume_limit) + EPSILON) {
        wipe_collection = source;
        wipe_volume = float(source_volume);
        return true;
    }

    auto* wipe_part = new_collection_like(*source);
    auto* rest_part = new_collection_like(*source);
    bool put_remaining_to_rest = false;

    for (ExtrusionEntity* entity : source->entities) {
        if (put_remaining_to_rest) {
            rest_part->entities.emplace_back(entity);
            continue;
        }

        const double entity_volume = entity->total_volume();
        if (double(wipe_volume) + entity_volume <= double(volume_limit) + EPSILON) {
            wipe_part->entities.emplace_back(entity);
            wipe_volume += float(entity_volume);
            continue;
        }

        const double remaining_volume = double(volume_limit) - double(wipe_volume);
        if (remaining_volume > EPSILON) {
            if (auto* path = dynamic_cast<ExtrusionPath*>(entity); path != nullptr && path->mm3_per_mm > 0.) {
                Polyline wipe_polyline;
                Polyline rest_polyline;
                const double split_length = scale_(remaining_volume / path->mm3_per_mm);
                if (path->polyline.split_at_length(split_length, &wipe_polyline, &rest_polyline)) {
                    if (wipe_polyline.is_valid()) {
                        if (ExtrusionPath* wipe_path = clone_path_with_polyline(*path, std::move(wipe_polyline)); wipe_path != nullptr) {
                            wipe_part->entities.emplace_back(wipe_path);
                            wipe_volume += float(wipe_path->total_volume());
                        }
                    }
                    if (rest_polyline.is_valid()) {
                        if (ExtrusionPath* rest_path = clone_path_with_polyline(*path, std::move(rest_polyline)); rest_path != nullptr)
                            rest_part->entities.emplace_back(rest_path);
                    }
                    delete entity;
                    put_remaining_to_rest = true;
                    continue;
                }
            }
        }

        rest_part->entities.emplace_back(entity);
        put_remaining_to_rest = true;
    }

    source->entities.clear();

    if (wipe_part->empty()) {
        delete wipe_part;
        if (rest_part->empty()) {
            delete rest_part;
            return false;
        }
        fills.entities[fill_idx] = rest_part;
        delete source;
        return false;
    }

    fills.entities[fill_idx] = wipe_part;
    if (rest_part->empty()) {
        delete rest_part;
    } else {
        fills.entities.insert(fills.entities.begin() + fill_idx + 1, rest_part);
    }
    delete source;

    wipe_collection = wipe_part;
    return true;
}

static unsigned int resolve_matrix_extruder_1based(const MixedFilamentManager *mixed_mgr,
                                                   size_t                      num_physical,
                                                   unsigned int                filament_id_1based,
                                                   int                         layer_index,
                                                   float                       layer_print_z,
                                                   float                       layer_height,
                                                   float                       layer_height_a,
                                                   float                       layer_height_b,
                                                   float                       base_layer_height,
                                                   unsigned int                matrix_extruder_count)
{
    if (matrix_extruder_count == 0)
        return 0;

    const unsigned int resolved = resolve_mixed_with_layer_heights(mixed_mgr,
                                                                   num_physical,
                                                                   filament_id_1based,
                                                                   layer_index,
                                                                   layer_print_z,
                                                                   layer_height,
                                                                   layer_height_a,
                                                                   layer_height_b,
                                                                   base_layer_height);
    return (resolved >= 1 && resolved <= matrix_extruder_count) ? resolved : 1;
}


// Shortest hamilton path problem
static std::vector<unsigned int> solve_extruder_order(const std::vector<std::vector<float>>& wipe_volumes, std::vector<unsigned int> all_extruders, std::optional<unsigned int> start_extruder_id) 
{
    if (all_extruders.empty())
        return all_extruders;

    bool add_start_extruder_flag = false;

    if (start_extruder_id) {
        auto start_iter = std::find(all_extruders.begin(), all_extruders.end(), *start_extruder_id);
        if (start_iter == all_extruders.end())
            all_extruders.insert(all_extruders.begin(), *start_extruder_id), add_start_extruder_flag = true;
        else
            std::swap(*all_extruders.begin(), *start_iter);
    }
    // When start_extruder_id is nullopt, just treat the first element as start (no swap needed).

    const int n          = (int) all_extruders.size();
    const float INF_COST = std::numeric_limits<float>::max();

    // Greedy nearest-neighbor fallback for large n to avoid 2^n memory explosion.
    if (n > k_tsp_exact_max_extruders) {
        std::vector<bool>        visited(n, false);
        std::vector<unsigned int> path;
        path.reserve(n);
        // Start from index 0 (already placed as first element above).
        int cur = 0;
        visited[cur] = true;
        path.push_back(all_extruders[cur]);
        for (int step = 1; step < n; ++step) {
            int   best_next = -1;
            float best_w    = INF_COST;
            for (int j = 0; j < n; ++j) {
                if (!visited[j]) {
                    float w = wipe_volumes[all_extruders[cur]][all_extruders[j]];
                    if (w < best_w) { best_w = w; best_next = j; }
                }
            }
            if (best_next == -1) break;
            visited[best_next] = true;
            path.push_back(all_extruders[best_next]);
            cur = best_next;
        }
        if (add_start_extruder_flag && !path.empty())
            path.erase(path.begin()); // remove the virtual start node
        return path;
    }

    // Flat 1-D DP arrays: single allocation, cache-friendly.
    const int states = 1 << n;
    std::vector<float>  cache(states * n, INF_COST);
    std::vector<int8_t> dp_prev(states * n, -1);
    cache[1 * n + 0] = 0.f; // start at index 0 with mask=1

    for (int state = 1; state < states; ++state) {
        if (!(state & 1))
            continue; // must include start node (index 0)
        for (int target = 0; target < n; ++target) {
            if (!(state >> target & 1))
                continue;
            float c = cache[state * n + target];
            if (c == INF_COST)
                continue;
            for (int next = 0; next < n; ++next) {
                if (state & (1 << next))
                    continue;
                float        tmp       = c + wipe_volumes[all_extruders[target]][all_extruders[next]];
                int          nxt_state = state | (1 << next);
                float&       slot      = cache[nxt_state * n + next];
                if (tmp < slot) {
                    slot                         = tmp;
                    dp_prev[nxt_state * n + next] = (int8_t) target;
                }
            }
        }
    }

    // Find best end node (prefer not ending on start node to avoid trivial paths).
    float cost      = INF_COST;
    int   final_dst = 0;
    for (int dst = 0; dst < n; ++dst) {
        float c = cache[(states - 1) * n + dst];
        if (c < cost) {
            cost      = c;
            final_dst = dst;
        }
    }

    std::vector<unsigned int> path;
    int curr_state = states - 1;
    int curr_point = final_dst;
    while (curr_point != -1) {
        path.emplace_back(all_extruders[curr_point]);
        int8_t mid = dp_prev[curr_state * n + curr_point];
        curr_state &= ~(1 << curr_point);
        curr_point = (int) mid; // int8_t -1 stays -1 as int
    }

    if (add_start_extruder_flag && !path.empty())
        path.pop_back(); // remove the virtual start node (it was pushed last, reversed to front)

    std::reverse(path.begin(), path.end());
    return path;
}

std::vector<unsigned int> get_extruders_order(const std::vector<std::vector<float>> &wipe_volumes, std::vector<unsigned int> all_extruders, std::optional<unsigned int>start_extruder_id)
{
#define USE_DP_OPTIMIZE
#ifdef USE_DP_OPTIMIZE
    return solve_extruder_order(wipe_volumes, all_extruders, start_extruder_id);
#else
if (all_extruders.size() > 1) {
        int begin_index = 0;
        auto iter = std::find(all_extruders.begin(), all_extruders.end(), start_extruder_id);
        if (iter != all_extruders.end()) {
            for (int i = 0; i < all_extruders.size(); ++i) {
                if (all_extruders[i] == start_extruder_id) {
                    std::swap(all_extruders[i], all_extruders[0]);
                }
            }
            begin_index = 1;
        }

        std::pair<float, std::vector<unsigned int>> volumes_to_extruder_order;
        volumes_to_extruder_order.first = 10000 * all_extruders.size();
        std::sort(all_extruders.begin() + begin_index, all_extruders.end());
        do {
            float flush_volume = 0;
            for (int i = 0; i < all_extruders.size() - 1; ++i) {
                flush_volume += wipe_volumes[all_extruders[i]][all_extruders[i + 1]];
            }
            if (flush_volume < volumes_to_extruder_order.first) {
                volumes_to_extruder_order = std::pair(flush_volume, all_extruders);
            }
        } while (std::next_permutation(all_extruders.begin() + begin_index, all_extruders.end()));

        if (volumes_to_extruder_order.second.size() > 0)
            return volumes_to_extruder_order.second;
    }
    return all_extruders;

#endif // OPTIMIZE
}

// Returns true in case that extruder a comes before b (b does not have to be present). False otherwise.
bool LayerTools::is_extruder_order(unsigned int a, unsigned int b) const
{
    if (a == b)
        return false;

    for (auto extruder : extruders) {
        if (extruder == a)
            return true;
        if (extruder == b)
            return false;
    }

    return false;
}

// Returns a zero based extruder this eec should be printed with, according to PrintRegion config or extruder_override if overriden.
unsigned int LayerTools::extruder(const ExtrusionEntityCollection& extrusions, const PrintRegion& region) const
{
    assert(region.config().wall_filament.value > 0);
    assert(region.config().sparse_infill_filament.value > 0);
    assert(region.config().solid_infill_filament.value > 0);
    // 1 based extruder ID.
    unsigned int extruder = 1;
    if (this->extruder_override == 0) {
        if (extrusions.has_infill()) {
            if (extrusions.has_solid_infill())
                extruder = region.config().solid_infill_filament;
            else
                extruder = region.config().sparse_infill_filament;
        } else
            extruder = region.config().wall_filament.value;
    } else
        extruder = this->extruder_override;

    // BBS: Resolve mixed filament ID to actual extruder.
    return (extruder == 0) ? 0 : resolve_mixed_1based(extruder) - 1;
}

static double calc_max_layer_height(const PrintConfig &config, double max_object_layer_height)
{
    double max_layer_height = std::numeric_limits<double>::max();
    for (size_t i = 0; i < config.nozzle_diameter.values.size(); ++ i) {
        double mlh = config.max_layer_height.values[i];
        if (mlh == 0.)
            mlh = 0.75 * config.nozzle_diameter.values[i];
        max_layer_height = std::min(max_layer_height, mlh);
    }
    // The Prusa3D Fast (0.35mm layer height) print profile sets a higher layer height than what is normally allowed
    // by the nozzle. This is a hack and it works by increasing extrusion width. See GH #3919.
    return std::max(max_layer_height, max_object_layer_height);
}

// For the use case when each object is printed separately
// (print->config().print_sequence == PrintSequence::ByObject is true).
ToolOrdering::ToolOrdering(const PrintObject &object, unsigned int first_extruder, bool prime_multi_material)
{
    m_is_BBL_printer = object.print()->is_BBL_printer();
    m_print_full_config = &object.print()->full_print_config();
    m_print_object_ptr = &object;
    // Mixed filament support.
    m_mixed_mgr   = &object.print()->mixed_filament_manager();
    m_num_physical = object.print()->config().filament_diameter.size();
    m_has_mixed_filaments = m_mixed_mgr != nullptr && m_mixed_mgr->enabled_count() > 0;
    update_mixed_layer_height_settings();
    if (object.layers().empty())
        return;

    // Initialize the print layers for just a single object.
    {
        std::vector<coordf_t> zs;
        zs.reserve(zs.size() + object.layers().size() + object.support_layers().size());
        for (auto layer : object.layers())
            zs.emplace_back(layer->print_z);
        for (auto layer : object.support_layers())
            zs.emplace_back(layer->print_z);
        this->initialize_layers(zs);
        this->initialize_mixed_context();
    }
    double max_layer_height = calc_max_layer_height(object.print()->config(), object.config().layer_height);

    // Collect extruders reuqired to print the layers.
    this->collect_extruders(object, std::vector<std::pair<double, unsigned int>>());

    // BBS
    // Reorder the extruders to minimize tool switches.
    std::vector<unsigned int> first_layer_tool_order;
    if (first_extruder == (unsigned int) -1) {
        first_layer_tool_order = generate_first_layer_tool_order(object);
    }

    if (!first_layer_tool_order.empty()) {
        this->reorder_extruders(first_layer_tool_order);
    } else {
        this->reorder_extruders(first_extruder);
    }

    this->fill_wipe_tower_partitions(object.print()->config(), object.layers().front()->print_z - object.layers().front()->height, max_layer_height);

    this->collect_extruder_statistics(prime_multi_material);

    this->mark_skirt_layers(object.print()->config(), max_layer_height);
}

// For the use case when all objects are printed at once.
// (print->config().print_sequence == PrintSequence::ByObject is false).
ToolOrdering::ToolOrdering(const Print &print, unsigned int first_extruder, bool prime_multi_material)
{
    m_is_BBL_printer = print.is_BBL_printer();
    m_print_full_config = &print.full_print_config();
    m_print_config_ptr = &print.config();
    // Mixed filament support.
    m_mixed_mgr   = &print.mixed_filament_manager();
    m_num_physical = print.config().filament_diameter.size();
    m_has_mixed_filaments = m_mixed_mgr != nullptr && m_mixed_mgr->enabled_count() > 0;
    update_mixed_layer_height_settings();

    // Initialize the print layers for all objects and all layers.
    coordf_t object_bottom_z = 0.;
    coordf_t max_layer_height = 0.;
    {
        std::vector<coordf_t> zs;
        for (auto object : print.objects()) {
            zs.reserve(zs.size() + object->layers().size() + object->support_layers().size());
            for (auto layer : object->layers())
                zs.emplace_back(layer->print_z);
            for (auto layer : object->support_layers())
                zs.emplace_back(layer->print_z);

            // Find first object layer that is not empty and save its print_z
            for (const Layer* layer : object->layers())
                if (layer->has_extrusions()) {
                    object_bottom_z = layer->print_z - layer->height;
                    break;
                }

            max_layer_height = std::max(max_layer_height, object->config().layer_height.value);
        }
        this->initialize_layers(zs);
        this->initialize_mixed_context();
    }
    max_layer_height = calc_max_layer_height(print.config(), max_layer_height);

	// Use the extruder switches from Model::custom_gcode_per_print_z to override the extruder to print the object.
	// Do it only if all the objects were configured to be printed with a single extruder.
	std::vector<std::pair<double, unsigned int>> per_layer_extruder_switches;

    // BBS
	if (auto num_filaments = unsigned(m_num_physical + (m_mixed_mgr != nullptr ? m_mixed_mgr->enabled_count() : 0));
		num_filaments > 1 && print.object_extruders().size() == 1 
        // the current Print's configuration is CustomGCode::MultiAsSingle
        //BBS: replace model custom gcode with current plate custom gcode
        // && print.model().get_curr_plate_custom_gcodes().mode == CustomGCode::MultiAsSingle
        ) {
		// Printing a single extruder platter on a printer with more than 1 extruder (or single-extruder multi-material).
		// There may be custom per-layer tool changes available at the model.
        per_layer_extruder_switches = custom_tool_changes(print.model().get_curr_plate_custom_gcodes(), num_filaments);
	}

    // Collect extruders reuqired to print the layers.
    for (auto object : print.objects())
        this->collect_extruders(*object, per_layer_extruder_switches);

    // Reorder the extruders to minimize tool switches.
    std::vector<unsigned int> first_layer_tool_order;
    if (first_extruder == (unsigned int)-1) {
        first_layer_tool_order = generate_first_layer_tool_order(print);
    }

    if (!first_layer_tool_order.empty()) {
        this->reorder_extruders(first_layer_tool_order);
    }
    else {
        this->reorder_extruders(first_extruder);
    }

    this->fill_wipe_tower_partitions(print.config(), object_bottom_z, max_layer_height);

    this->collect_extruder_statistics(prime_multi_material);

    this->mark_skirt_layers(print.config(), max_layer_height);
}

// BBS
std::vector<unsigned int> ToolOrdering::generate_first_layer_tool_order(const Print& print)
{
    std::vector<unsigned int> tool_order;
    int initial_extruder_id = -1;
    std::map<int, double> min_areas_per_extruder;

    for (auto object : print.objects()) {
        auto first_layer = object->get_layer(0);
        for (auto layerm : first_layer->regions()) {
            int extruder_id = layerm->region().config().option("wall_filament")->getInt();
            
            for (auto expoly : layerm->raw_slices) {
                const double nozzle_diameter = print.config().nozzle_diameter.get_at(0);
                const coordf_t initial_layer_line_width = print.config().get_abs_value("initial_layer_line_width", nozzle_diameter);

                if (offset_ex(expoly, -0.2 * scale_(initial_layer_line_width)).empty())
                    continue;

                double contour_area = expoly.contour.area();
                auto iter = min_areas_per_extruder.find(extruder_id);
                if (iter == min_areas_per_extruder.end()) {
                    min_areas_per_extruder.insert({ extruder_id, contour_area });
                }
                else {
                    if (contour_area < min_areas_per_extruder.at(extruder_id)) {
                        min_areas_per_extruder[extruder_id] = contour_area;
                    }
                }
            }
        }
    }

    double max_minimal_area = 0.;
    for (auto ape : min_areas_per_extruder) {
        auto iter = tool_order.begin();
        for (; iter != tool_order.end(); iter++) {
            if (min_areas_per_extruder.at(*iter) < min_areas_per_extruder.at(ape.first))
                break;
        }

        tool_order.insert(iter, ape.first);
    }

    const ConfigOptionInts* first_layer_print_sequence_op = print.full_print_config().option<ConfigOptionInts>("first_layer_print_sequence");
    if (first_layer_print_sequence_op) {
        const std::vector<int>& print_sequence_1st = first_layer_print_sequence_op->values;
        if (print_sequence_1st.size() >= tool_order.size()) {
            std::sort(tool_order.begin(), tool_order.end(), [&print_sequence_1st](int lh, int rh) {
                auto lh_it = std::find(print_sequence_1st.begin(), print_sequence_1st.end(), lh);
                auto rh_it = std::find(print_sequence_1st.begin(), print_sequence_1st.end(), rh);

                if (lh_it == print_sequence_1st.end() || rh_it == print_sequence_1st.end())
                    return false;

                return lh_it < rh_it;
            });
        }
    }

    return tool_order;
}

std::vector<unsigned int> ToolOrdering::generate_first_layer_tool_order(const PrintObject& object)
{
    std::vector<unsigned int> tool_order;
    int initial_extruder_id = -1;
    std::map<int, double> min_areas_per_extruder;
    auto first_layer = object.get_layer(0);
    for (auto layerm : first_layer->regions()) {
        int extruder_id = layerm->region().config().option("wall_filament")->getInt();
        for (auto expoly : layerm->raw_slices) {
            const double nozzle_diameter = object.print()->config().nozzle_diameter.get_at(0);
            const coordf_t line_width = object.config().get_abs_value("line_width", nozzle_diameter);

            if (offset_ex(expoly, -0.2 * scale_(line_width)).empty())
                continue;

            double contour_area = expoly.contour.area();
            auto iter = min_areas_per_extruder.find(extruder_id);
            if (iter == min_areas_per_extruder.end()) {
                min_areas_per_extruder.insert({ extruder_id, contour_area });
            }
            else {
                if (contour_area < min_areas_per_extruder.at(extruder_id)) {
                    min_areas_per_extruder[extruder_id] = contour_area;
                }
            }
        }
    }

    double max_minimal_area = 0.;
    for (auto ape : min_areas_per_extruder) {
        auto iter = tool_order.begin();
        for (; iter != tool_order.end(); iter++) {
            if (min_areas_per_extruder.at(*iter) < min_areas_per_extruder.at(ape.first))
                break;
        }

        tool_order.insert(iter, ape.first);
    }

    const ConfigOptionInts* first_layer_print_sequence_op = object.print()->full_print_config().option<ConfigOptionInts>("first_layer_print_sequence");
    if (first_layer_print_sequence_op) {
        const std::vector<int>& print_sequence_1st = first_layer_print_sequence_op->values;
        if (print_sequence_1st.size() >= tool_order.size()) {
            std::sort(tool_order.begin(), tool_order.end(), [&print_sequence_1st](int lh, int rh) {
                auto lh_it = std::find(print_sequence_1st.begin(), print_sequence_1st.end(), lh);
                auto rh_it = std::find(print_sequence_1st.begin(), print_sequence_1st.end(), rh);

                if (lh_it == print_sequence_1st.end() || rh_it == print_sequence_1st.end())
                    return false;

                return lh_it < rh_it;
            });
        }
    }

    return tool_order;
}

void ToolOrdering::initialize_layers(std::vector<coordf_t> &zs)
{
    sort_remove_duplicates(zs);
    // Merge numerically very close Z values.
    for (size_t i = 0; i < zs.size();) {
        // Find the last layer with roughly the same print_z.
        size_t j = i + 1;
        coordf_t zmax = zs[i] + EPSILON;
        for (; j < zs.size() && zs[j] <= zmax; ++ j) ;
        // Assign an average print_z to the set of layers with nearly equal print_z.
        m_layer_tools.emplace_back(LayerTools(0.5 * (zs[i] + zs[j-1])));
        i = j;
    }
}

void ToolOrdering::initialize_mixed_context()
{
    for (LayerTools &layer_tools : m_layer_tools) {
        layer_tools.mixed_mgr                = m_mixed_mgr;
        layer_tools.num_physical             = m_num_physical;
        layer_tools.has_mixed_filaments      = m_has_mixed_filaments;
        layer_tools.mixed_layer_height_a     = m_mixed_layer_height_a;
        layer_tools.mixed_layer_height_b     = m_mixed_layer_height_b;
        layer_tools.mixed_base_layer_height  = m_mixed_base_layer_height;
    }
}

// Collect extruders reuqired to print layers.
void ToolOrdering::collect_extruders(const PrintObject &object, const std::vector<std::pair<double, unsigned int>> &per_layer_extruder_switches)
{

    // Collect the support extruders.
    for (auto support_layer : object.support_layers()) 
    {
        LayerTools   &layer_tools = this->tools_for_layer(support_layer->print_z);
        layer_tools.layer_height = support_layer->height;
        ExtrusionRole role = support_layer->support_fills.role();

        bool          has_support = false;
        bool          has_interface = false;
        for (const ExtrusionEntity* ee : support_layer->support_fills.entities) 
        {
            ExtrusionRole er = ee->role();
            if (er == erSupportMaterial || er == erSupportTransition) has_support = true;
            if (er == erSupportMaterialInterface) has_interface = true;
            if (has_support && has_interface) break;
        }

        unsigned int extruder_support = 0;
        unsigned int extruder_interface = 0;
        if (has_support)
        {
            extruder_support = resolve_mixed(object.config().support_filament.value,
                                             layer_tools.layer_index,
                                             float(support_layer->print_z),
                                             float(support_layer->height));
            if (has_interface)
                extruder_interface = resolve_mixed(object.config().support_interface_filament.value,
                                                   layer_tools.layer_index,
                                                   float(support_layer->print_z),
                                                   float(support_layer->height));
            if (extruder_support > 0 || !has_interface || extruder_interface == 0 || layer_tools.has_object)
            {
                layer_tools.extruders.push_back(extruder_support);
            }
            else 
            {
                auto all_extruders = object.print()->extruders();
                const PrintConfig& print_config = object.print()->config();
                const std::vector<double> flush_matrix =
                    get_flush_volumes_matrix(print_config.flush_volumes_matrix.values, 0, print_config.nozzle_diameter.values.size());

                const unsigned int number_of_extruders = (unsigned int)(sqrt(flush_matrix.size()) + EPSILON);

                const unsigned int interface_extruder = resolve_matrix_extruder_1based(m_mixed_mgr,
                                                                                       m_num_physical,
                                                                                       extruder_interface,
                                                                                       layer_tools.layer_index,
                                                                                       float(support_layer->print_z),
                                                                                       float(support_layer->height),
                                                                                       m_mixed_layer_height_a,
                                                                                       m_mixed_layer_height_b,
                                                                                       m_mixed_base_layer_height,
                                                                                       number_of_extruders);
                auto get_next_extruder = [&](int current_extruder, const std::vector<unsigned int>& extruders) 
                {
                    if (interface_extruder == 0)
                        return current_extruder;

                    int   next_extruder = current_extruder;
                    float min_flush = std::numeric_limits<float>::max();
                    for (auto extruder_id : extruders)
                    {
                        const unsigned int candidate_extruder_1based = resolve_matrix_extruder_1based(m_mixed_mgr,
                                                                                                      m_num_physical,
                                                                                                      extruder_id + 1,
                                                                                                      layer_tools.layer_index,
                                                                                                      float(support_layer->print_z),
                                                                                                      float(support_layer->height),
                                                                                                      m_mixed_layer_height_a,
                                                                                                      m_mixed_layer_height_b,
                                                                                                      m_mixed_base_layer_height,
                                                                                                      number_of_extruders);
                        if (candidate_extruder_1based == 0)
                            continue;

                        const unsigned int candidate_extruder = candidate_extruder_1based - 1;
                        if (print_config.filament_soluble.get_at(candidate_extruder) || int(candidate_extruder) == current_extruder) continue;

                        const float candidate_flush = float(flush_matrix[(interface_extruder - 1) * number_of_extruders + candidate_extruder]);
                        if (candidate_flush < min_flush)
                        {
                            next_extruder = candidate_extruder;
                            min_flush = candidate_flush;
                        }
                    }
                    return next_extruder;
                };

                bool interface_not_for_body = object.config().support_interface_not_for_body;
                layer_tools.extruders.push_back(get_next_extruder(interface_not_for_body && interface_extruder > 0 ? int(interface_extruder - 1) : -1, all_extruders) + 1);
            }
        }
        
        if (has_interface)
        {
            if (extruder_interface == 0)
                extruder_interface = resolve_mixed(object.config().support_interface_filament.value,
                                                   layer_tools.layer_index,
                                                   float(support_layer->print_z),
                                                   float(support_layer->height));
            layer_tools.extruders.push_back(extruder_interface);
        }
        
        if (has_support || has_interface) 
        {
            layer_tools.has_support = true;
            layer_tools.wiping_extrusions().is_support_overriddable_and_mark(role, object);
        }
    }

    // Extruder overrides are ordered by print_z.
    std::vector<std::pair<double, unsigned int>>::const_iterator it_per_layer_extruder_override;
	it_per_layer_extruder_override = per_layer_extruder_switches.begin();
    unsigned int extruder_override = 0;

    // BBS: collect first layer extruders of an object's wall, which will be used by brim generator
    int layerCount = 0;
    std::vector<int> firstLayerExtruders;
    firstLayerExtruders.clear();

    bool ignore_inner_color = object.print()->config().ignore_inner_color.value;

    // Collect the object extruders.
    for (auto layer : object.layers()) {
        LayerTools &layer_tools = this->tools_for_layer(layer->print_z);
        // Store the sequential layer index and mixed-filament context for resolution.
        layer_tools.layer_index  = layerCount;
        layer_tools.layer_height = layer->height;

        // Override extruder with the next 
    	for (; it_per_layer_extruder_override != per_layer_extruder_switches.end() && it_per_layer_extruder_override->first < layer->print_z + EPSILON; ++ it_per_layer_extruder_override)
    		extruder_override = (int)it_per_layer_extruder_override->second;

        // Store the current extruder override (set to zero if no overriden), so that layer_tools.wiping_extrusions().is_overridable_and_mark() will use it.
        layer_tools.extruder_override = extruder_override;

        // What extruders are required to print this object layer?
        for (const LayerRegion *layerm : layer->regions()) {
            const PrintRegion &region = layerm->region();

            if(ignore_inner_color && layerm->all_inner)
                continue;
                
            if (! layerm->perimeters.entities.empty()) {
                bool something_nonoverriddable = true;

                if (m_print_config_ptr) { // in this case print->config().print_sequence != PrintSequence::ByObject (see ToolOrdering constructors)
                    something_nonoverriddable = false;
                    for (const auto& eec : layerm->perimeters.entities) // let's check if there are nonoverriddable entities
                        if (!layer_tools.wiping_extrusions().is_overriddable_and_mark(dynamic_cast<const ExtrusionEntityCollection&>(*eec), *m_print_config_ptr, object, region))
                            something_nonoverriddable = true;
                } else {
                    something_nonoverriddable = false;
                    for (const auto &eec : layerm->perimeters.entities) // let's check if there are nonoverriddable entities
                        if (!layer_tools.wiping_extrusions().is_obj_overriddable_and_mark(dynamic_cast<const ExtrusionEntityCollection &>(*eec), object))
                            something_nonoverriddable = true;
                }

                if (something_nonoverriddable){
                    const unsigned int configured_wall = (extruder_override == 0) ? region.config().wall_filament.value : extruder_override;
                    unsigned int       wall_ext        = resolve_mixed(configured_wall, layerCount, float(layer->print_z), float(layer->height));
               		layer_tools.extruders.emplace_back(wall_ext);
                    if (layerCount == 0) {
                        firstLayerExtruders.emplace_back(wall_ext);
                    }
                }

                layer_tools.has_object = true;
            }

            bool has_infill       = false;
            bool has_solid_infill = false;
            bool something_nonoverriddable = false;
            for (const ExtrusionEntity *ee : layerm->fills.entities) {
                // fill represents infill extrusions of a single island.
                const auto *fill = dynamic_cast<const ExtrusionEntityCollection*>(ee);
                ExtrusionRole role = fill->entities.empty() ? erNone : fill->entities.front()->role();
                if (is_solid_infill(role))
                    has_solid_infill = true;
                else if (is_infill(role))
                    has_infill = true;

                if (m_print_config_ptr) {
                    bool overridable = layer_tools.wiping_extrusions().is_overriddable_and_mark(*fill, *m_print_config_ptr, object, region);
                    if (!overridable || can_flush_into_skeleton(*fill, *m_print_config_ptr, object))
                        something_nonoverriddable = true;
                } else {
                    bool overridable = layer_tools.wiping_extrusions().is_obj_overriddable_and_mark(*fill, object);
                    if (!overridable || can_flush_into_skeleton(*fill, object.print()->config(), object))
                        something_nonoverriddable = true;
                }
            }

            if (something_nonoverriddable) {
            	if (extruder_override == 0) {
		                if (has_solid_infill)
		                    layer_tools.extruders.emplace_back(resolve_mixed(region.config().solid_infill_filament, layerCount, float(layer->print_z), float(layer->height)));
		                if (has_infill)
	                    layer_tools.extruders.emplace_back(resolve_mixed(region.config().sparse_infill_filament, layerCount, float(layer->print_z), float(layer->height)));
            	} else if (has_solid_infill || has_infill)
            		layer_tools.extruders.emplace_back(resolve_mixed(extruder_override, layerCount, float(layer->print_z), float(layer->height)));
            }
            if (has_solid_infill || has_infill)
                layer_tools.has_object = true;
        }
        layerCount++;
    }

    sort_remove_duplicates(firstLayerExtruders);
    const_cast<PrintObject&>(object).object_first_layer_wall_extruders = firstLayerExtruders;
    
    for (auto& layer : m_layer_tools) {
        // Sort and remove duplicates
        sort_remove_duplicates(layer.extruders);

        // make sure that there are some tools for each object layer (e.g. tall wiping object will result in empty extruders vector)
        if (layer.extruders.empty() && layer.has_object)
            layer.extruders.emplace_back(0); // 0="dontcare" extruder - it will be taken care of in reorder_extruders
    }
}

// Reorder extruders to minimize layer changes.
void ToolOrdering::reorder_extruders(unsigned int last_extruder_id)
{
    if (m_layer_tools.empty())
        return;

    if (last_extruder_id == (unsigned int)-1) {
        // The initial print extruder has not been decided yet.
        // Initialize the last_extruder_id with the first non-zero extruder id used for the print.
        last_extruder_id = 0;
        for (size_t i = 0; i < m_layer_tools.size() && last_extruder_id == 0; ++ i) {
            const LayerTools &lt = m_layer_tools[i];
            for (unsigned int extruder_id : lt.extruders)
                if (extruder_id > 0) {
                    last_extruder_id = extruder_id;
                    break;
                }
        }
        if (last_extruder_id == 0)
            // Nothing to extrude.
            return;
    } else
        // 1 based index
        ++ last_extruder_id;

    for (LayerTools &lt : m_layer_tools) {
        if (lt.extruders.empty())
            continue;
        if (lt.extruders.size() == 1 && lt.extruders.front() == 0)
            lt.extruders.front() = last_extruder_id;
        else {
            if (lt.extruders.front() == 0)
                // Pop the "don't care" extruder, the "don't care" region will be merged with the next one.
                lt.extruders.erase(lt.extruders.begin());
            // Reorder the extruders to start with the last one.
            for (size_t i = 1; i < lt.extruders.size(); ++ i)
                if (lt.extruders[i] == last_extruder_id) {
                    // Move the last extruder to the front.
                    memmove(lt.extruders.data() + 1, lt.extruders.data(), i * sizeof(unsigned int));
                    lt.extruders.front() = last_extruder_id;
                    break;
                }

            // On first layer with wipe tower, prefer a soluble extruder
            // at the beginning, so it is not wiped on the first layer.
            if (lt == m_layer_tools[0] && m_print_config_ptr && m_print_config_ptr->enable_prime_tower) {
                for (size_t i = 0; i<lt.extruders.size(); ++i)
                    if (m_print_config_ptr->filament_soluble.get_at(lt.extruders[i]-1)) { // 1-based...
                        std::swap(lt.extruders[i], lt.extruders.front());
                        break;
                    }
            }
        }
        last_extruder_id = lt.extruders.back();
    }

    // Reindex the extruders, so they are zero based, not 1 based.
    for (LayerTools &lt : m_layer_tools)
        for (unsigned int &extruder_id : lt.extruders) {
            assert(extruder_id > 0);
            -- extruder_id;
        }

    // reorder the extruders for minimum flush volume
    reorder_extruders_for_minimum_flush_volume();
}

// BBS
void ToolOrdering::reorder_extruders(std::vector<unsigned int> tool_order_layer0)
{
    if (m_layer_tools.empty())
        return;

    if (tool_order_layer0.empty())
        return;

    // Reorder the extruders of first layer
    {
        LayerTools& lt = m_layer_tools[0];
        std::vector<unsigned int> layer0_extruders = lt.extruders;
        lt.extruders.clear();
        for (unsigned int extruder_id : tool_order_layer0) {
            auto iter = std::find(layer0_extruders.begin(), layer0_extruders.end(), extruder_id);
            if (iter != layer0_extruders.end()) {
                lt.extruders.push_back(extruder_id);
                *iter = (unsigned int)-1;
            }
        }

        for (unsigned int extruder_id : layer0_extruders) {
            if (extruder_id == 0)
                continue;

            if (extruder_id != (unsigned int)-1)
                lt.extruders.push_back(extruder_id);
        }

        // all extruders are zero
        if (lt.extruders.empty()) {
            lt.extruders.push_back(tool_order_layer0[0]);
        }
    }

    int last_extruder_id = m_layer_tools[0].extruders.back();
    for (int i = 1; i < m_layer_tools.size(); i++) {
        LayerTools& lt = m_layer_tools[i];

        if (lt.extruders.empty())
            continue;
        if (lt.extruders.size() == 1 && lt.extruders.front() == 0)
            lt.extruders.front() = last_extruder_id;
        else {
            if (lt.extruders.front() == 0)
                // Pop the "don't care" extruder, the "don't care" region will be merged with the next one.
                lt.extruders.erase(lt.extruders.begin());
            // Reorder the extruders to start with the last one.
            for (size_t i = 1; i < lt.extruders.size(); ++i)
                if (lt.extruders[i] == last_extruder_id) {
                    // Move the last extruder to the front.
                    memmove(lt.extruders.data() + 1, lt.extruders.data(), i * sizeof(unsigned int));
                    lt.extruders.front() = last_extruder_id;
                    break;
                }
        }
        last_extruder_id = lt.extruders.back();
    }

    // Reindex the extruders, so they are zero based, not 1 based.
    for (LayerTools& lt : m_layer_tools)
        for (unsigned int& extruder_id : lt.extruders) {
            assert(extruder_id > 0);
            --extruder_id;
        }

    // reorder the extruders for minimum flush volume
    reorder_extruders_for_minimum_flush_volume();
}

void ToolOrdering::fill_wipe_tower_partitions(const PrintConfig &config, coordf_t object_bottom_z, coordf_t max_layer_height)
{
    if (m_layer_tools.empty())
        return;

    // Count the minimum number of tool changes per layer.
    size_t last_extruder = size_t(-1);
    for (LayerTools &lt : m_layer_tools) {
        lt.wipe_tower_partitions = lt.extruders.size();
        if (! lt.extruders.empty()) {
            if (last_extruder == size_t(-1) || last_extruder == lt.extruders.front())
                // The first extruder on this layer is equal to the current one, no need to do an initial tool change.
                -- lt.wipe_tower_partitions;
            last_extruder = lt.extruders.back();
        }
    }

    // Propagate the wipe tower partitions down to support the upper partitions by the lower partitions.
    for (int i = int(m_layer_tools.size()) - 2; i >= 0; -- i)
        m_layer_tools[i].wipe_tower_partitions = std::max(m_layer_tools[i + 1].wipe_tower_partitions, m_layer_tools[i].wipe_tower_partitions);

    //FIXME this is a hack to get the ball rolling.
    for (LayerTools &lt : m_layer_tools)
        lt.has_wipe_tower = (lt.has_object && (config.timelapse_type == TimelapseType::tlSmooth || lt.wipe_tower_partitions > 0))
            || lt.print_z < object_bottom_z + EPSILON;

    // Test for a raft, insert additional wipe tower layer to fill in the raft separation gap.
    for (size_t i = 0; i + 1 < m_layer_tools.size(); ++ i) {
        const LayerTools &lt      = m_layer_tools[i];
        const LayerTools &lt_next = m_layer_tools[i + 1];
        if (lt.print_z < object_bottom_z + EPSILON && lt_next.print_z >= object_bottom_z + EPSILON) {
            // lt is the last raft layer. Find the 1st object layer.
            size_t j = i + 1;
            for (; j < m_layer_tools.size() && ! m_layer_tools[j].has_wipe_tower; ++ j);
            if (j < m_layer_tools.size()) {
                const LayerTools &lt_object = m_layer_tools[j];
                coordf_t gap = lt_object.print_z - lt.print_z;
                assert(gap > 0.f);
                if (gap > max_layer_height + EPSILON) {
                    // Insert one additional wipe tower layer between lh.print_z and lt_object.print_z.
                    LayerTools lt_new(0.5f * (lt.print_z + lt_object.print_z));
                    // Find the 1st layer above lt_new.
                    for (j = i + 1; j < m_layer_tools.size() && m_layer_tools[j].print_z < lt_new.print_z - EPSILON; ++ j);
                    if (std::abs(m_layer_tools[j].print_z - lt_new.print_z) < EPSILON) {
						m_layer_tools[j].has_wipe_tower = true;
					} else {
						LayerTools &lt_extra = *m_layer_tools.insert(m_layer_tools.begin() + j, lt_new);
                        //LayerTools &lt_prev  = m_layer_tools[j];
                        LayerTools &lt_next  = m_layer_tools[j + 1];
                        assert(! m_layer_tools[j - 1].extruders.empty() && ! lt_next.extruders.empty());
                        // FIXME: Following assert tripped when running combine_infill.t. I decided to comment it out for now.
                        // If it is a bug, it's likely not critical, because this code is unchanged for a long time. It might
                        // still be worth looking into it more and decide if it is a bug or an obsolete assert.
                        //assert(lt_prev.extruders.back() == lt_next.extruders.front());
                        lt_extra.has_wipe_tower = true;
                        lt_extra.extruders.push_back(lt_next.extruders.front());
                        lt_extra.wipe_tower_partitions = lt_next.wipe_tower_partitions;
                    }
                }
            }
            break;
        }
    }

    // If the model contains empty layers (such as https://github.com/prusa3d/Slic3r/issues/1266), there might be layers
    // that were not marked as has_wipe_tower, even when they should have been. This produces a crash with soluble supports
    // and maybe other problems. We will therefore go through layer_tools and detect and fix this.
    // So, if there is a non-object layer starting with different extruder than the last one ended with (or containing more than one extruder),
    // we'll mark it with has_wipe tower.
    for (unsigned int i=0; i+1<m_layer_tools.size(); ++i) {
        LayerTools& lt = m_layer_tools[i];
        LayerTools& lt_next = m_layer_tools[i+1];
        if (lt.extruders.empty() || lt_next.extruders.empty())
            break;
        if (!lt_next.has_wipe_tower && (lt_next.extruders.front() != lt.extruders.back() || lt_next.extruders.size() > 1))
            lt_next.has_wipe_tower = true;
        // We should also check that the next wipe tower layer is no further than max_layer_height:
        unsigned int j = i+1;
        double last_wipe_tower_print_z = lt_next.print_z;
        while (++j < m_layer_tools.size()-1 && !m_layer_tools[j].has_wipe_tower)
            if (m_layer_tools[j+1].print_z - last_wipe_tower_print_z > max_layer_height + EPSILON) {
                m_layer_tools[j].has_wipe_tower = true;
                last_wipe_tower_print_z = m_layer_tools[j].print_z;
            }
    }

    // Calculate the wipe_tower_layer_height values.
    coordf_t wipe_tower_print_z_last = 0.;
    for (LayerTools &lt : m_layer_tools)
        if (lt.has_wipe_tower) {
            lt.wipe_tower_layer_height = lt.print_z - wipe_tower_print_z_last;
            wipe_tower_print_z_last = lt.print_z;
        }
}

void ToolOrdering::collect_extruder_statistics(bool prime_multi_material)
{
    m_first_printing_extruder = (unsigned int)-1;
    for (const auto &lt : m_layer_tools)
        if (! lt.extruders.empty()) {
            m_first_printing_extruder = lt.extruders.front();
            break;
        }

    m_last_printing_extruder = (unsigned int)-1;
    for (auto lt_it = m_layer_tools.rbegin(); lt_it != m_layer_tools.rend(); ++ lt_it)
        if (! lt_it->extruders.empty()) {
            m_last_printing_extruder = lt_it->extruders.back();
            break;
        }

    m_all_printing_extruders.clear();
    for (const auto &lt : m_layer_tools) {
        append(m_all_printing_extruders, lt.extruders);
        sort_remove_duplicates(m_all_printing_extruders);
    }

    if (prime_multi_material && ! m_all_printing_extruders.empty()) {
        // Reorder m_all_printing_extruders in the sequence they will be primed, the last one will be m_first_printing_extruder.
        // Then set m_first_printing_extruder to the 1st extruder primed.
        m_all_printing_extruders.erase(
            std::remove_if(m_all_printing_extruders.begin(), m_all_printing_extruders.end(),
                [ this ](const unsigned int eid) { return eid == m_first_printing_extruder; }),
            m_all_printing_extruders.end());
        m_all_printing_extruders.emplace_back(m_first_printing_extruder);
        m_first_printing_extruder = m_all_printing_extruders.front();
    }
}

void ToolOrdering::reorder_extruders_for_minimum_flush_volume()
{
    const PrintConfig *print_config = m_print_config_ptr;
    if (!print_config && m_print_object_ptr) {
        print_config = &(m_print_object_ptr->print()->config());
    }

    if (!print_config || m_layer_tools.empty())
        return;

    std::vector<int> valid_layers;
    for (int i = 0; i < (int) m_layer_tools.size(); ++i) {
        if (!m_layer_tools[i].extruders.empty())
            valid_layers.push_back(i);
    }
    if (valid_layers.empty())
        return;

    int minFlush               = 2;
    const ConfigOptionInts* first_layer_seq_op = print_config->option<ConfigOptionInts>("first_layer_print_sequence");
    if (first_layer_seq_op && !first_layer_seq_op->values.empty() &&
        first_layer_seq_op->values.size() >= m_layer_tools[valid_layers[0]].extruders.size())
            minFlush -= 1;

    std::vector<LayerPrintSequence> other_layers_seqs;
    const ConfigOptionInts*         other_layers_print_sequence_op = print_config->option<ConfigOptionInts>("other_layers_print_sequence");
    const ConfigOptionInt* other_layers_print_sequence_nums_op = print_config->option<ConfigOptionInt>("other_layers_print_sequence_nums");
    if (other_layers_print_sequence_op && other_layers_print_sequence_nums_op) {
        const std::vector<int>& print_sequence = other_layers_print_sequence_op->values;
        int                     sequence_nums  = other_layers_print_sequence_nums_op->value;
        other_layers_seqs                      = get_other_layers_print_sequence(sequence_nums, print_sequence);
        if (sequence_nums > 0)
            minFlush -= 1;
    }

    // Get wiping matrix to get number of extruders.
    const auto& flush_matrix = print_config->flush_volumes_matrix.values;
    const unsigned int number_of_extruders = (unsigned int) (sqrt(flush_matrix.size()) + EPSILON);
    if (number_of_extruders == 0)
        return;

    bool has_multi_tool_layer = false;
    for (LayerTools &lt : m_layer_tools) {
        if (m_has_mixed_filaments) {
            for (unsigned int &extruder_id : lt.extruders) {
                const unsigned int resolved = resolve_matrix_extruder_1based(m_mixed_mgr,
                                                                             m_num_physical,
                                                                             extruder_id + 1,
                                                                             lt.layer_index,
                                                                             float(lt.print_z),
                                                                             float(lt.layer_height),
                                                                             m_mixed_layer_height_a,
                                                                             m_mixed_layer_height_b,
                                                                             m_mixed_base_layer_height,
                                                                             number_of_extruders);
                extruder_id = resolved > 0 ? resolved - 1 : 0;
            }
        }
        // Remove duplicates while preserving the existing order. We must NOT sort here,
        // because the first layer's extruders have already been arranged according to
        // first_layer_print_sequence by reorder_extruders(); sorting would discard that
        // custom order (the first layer is skipped by the reordering loop below, so it
        // would otherwise never get the custom sequence re-applied).
        {
            std::unordered_set<unsigned int> seen;
            seen.reserve(lt.extruders.size());
            auto new_end = std::remove_if(lt.extruders.begin(), lt.extruders.end(),
                                          [&seen](unsigned int id) { return !seen.insert(id).second; });
            lt.extruders.erase(new_end, lt.extruders.end());
        }
        has_multi_tool_layer = has_multi_tool_layer || lt.extruders.size() > 1;
    }
    if (!has_multi_tool_layer)
        return;

    // Extract purging volumes for each extruder pair:
    std::vector<std::vector<float>> wipe_volumes;
    wipe_volumes.reserve(number_of_extruders);
    if (print_config->purge_in_prime_tower || m_is_BBL_printer || minFlush > 1) {
        for (unsigned int i = 0; i < number_of_extruders; ++i)
            wipe_volumes.push_back( std::vector<float>(flush_matrix.begin() + i * number_of_extruders,
                                                       flush_matrix.begin() + (i + 1) * number_of_extruders));
    } else {
        // populate wipe_volumes with prime_volume
        for (unsigned int i = 0; i < number_of_extruders; ++i)
            wipe_volumes.push_back(std::vector<float>(number_of_extruders, print_config->prime_volume));
    }
  
    // other_layers_seq: the layer_idx and extruder_idx are base on 1
    auto get_custom_seq = [&other_layers_seqs](int layer_idx, std::vector<int>& out_seq) -> bool {
        for (size_t idx = other_layers_seqs.size() - 1; idx != size_t(-1); --idx) {
            const auto& other_layers_seq = other_layers_seqs[idx];
            if (layer_idx + 1 >= other_layers_seq.first.first && layer_idx + 1 <= other_layers_seq.first.second) {
                out_seq = other_layers_seq.second;
                return true;
            }
        }
        return false;
    };


    if (print_config->purge_in_prime_tower || m_is_BBL_printer || minFlush > 1)
    {
        if (valid_layers.empty())
            return;

        // CompactPath: stores cost and the ordered extruder path for one TSP result.
        // Used only inside tsp_cache; dp_global no longer stores paths.
        struct CompactPath
        {
            float                cost = std::numeric_limits<float>::max();
            std::vector<uint8_t> path;

            bool is_better_than(float new_cost, const uint8_t* new_p, size_t new_l) const
            {
                if (new_cost < cost - 1e-4f)
                    return true;
                if (new_cost > cost + 1e-4f)
                    return false;
                size_t curr_l = path.size();
                for (size_t i = 0; i < std::min(curr_l, new_l); ++i) {
                    if (new_p[i] < path[i])
                        return true;
                    if (new_p[i] > path[i])
                        return false;
                }
                return new_l < curr_l;
            }

            void set(float c, const uint8_t* p, size_t l)
            {
                cost = c;
                path.assign(p, p + l);
            }
        };

        const unsigned int num_ex = (unsigned int) (sqrt(flush_matrix.size()) + EPSILON);
        const float        INF    = std::numeric_limits<float>::max();

        // Opt-4: threshold n>16 (instead of n>20) to limit TSP DP memory to at most 2^16*16*4=4MB.
        auto compute_layer_tsp = [&](const std::vector<unsigned int>& extruders, unsigned int start_ex) -> std::vector<CompactPath> {
            int n = (int) extruders.size();
            // Fall back to O(n^2) greedy nearest-neighbor when n is too large to avoid std::bad_alloc.
            if (n > k_tsp_exact_max_extruders) {
                std::vector<CompactPath> results(num_ex);
                std::vector<bool>        visited(n, false);
                int                      cur = -1;
                for (int i = 0; i < n; ++i) {
                    if (extruders[i] == start_ex) { cur = i; break; }
                }
                if (cur == -1) cur = 0;
                std::vector<uint8_t> path;
                path.reserve(n);
                float total_cost = 0.0f;
                visited[cur]     = true;
                path.push_back((uint8_t) extruders[cur]);
                for (int step = 1; step < n; ++step) {
                    int   best_next = -1;
                    float best_w    = INF;
                    for (int j = 0; j < n; ++j) {
                        if (!visited[j]) {
                            float w = wipe_volumes[extruders[cur]][extruders[j]];
                            if (w < best_w) { best_w = w; best_next = j; }
                        }
                    }
                    if (best_next == -1) break;
                    total_cost += best_w;
                    visited[best_next] = true;
                    path.push_back((uint8_t) extruders[best_next]);
                    cur = best_next;
                }
                if (!path.empty()) {
                    unsigned int end_ex = (unsigned int) path.back();
                    if (end_ex < (unsigned int) results.size())
                        results[end_ex].set(total_cost, path.data(), path.size());
                }
                return results;
            }

            // Opt-3: flat 1-D arrays (single allocation, cache-friendly) instead of vector-of-vector.
            const int states = 1 << n;
            std::vector<float>  dp_cost(states * n, INF);
            std::vector<int8_t> tsp_parent(states * n, -1);

            int start_idx = -1;
            for (int i = 0; i < n; ++i) {
                if (extruders[i] == start_ex)
                    start_idx = i;
            }
            if (start_idx != -1)
                dp_cost[(1 << start_idx) * n + start_idx] = 0.0f;

            for (int mask = 1; mask < states; ++mask) {
                for (int curr = 0; curr < n; ++curr) {
                    float c = dp_cost[mask * n + curr];
                    if (c == INF)
                        continue;
                    for (int next = 0; next < n; ++next) {
                        if (!(mask & (1 << next))) {
                            float        new_cost  = c + wipe_volumes[extruders[curr]][extruders[next]];
                            unsigned int next_mask = (unsigned int) mask | (1 << next);
                            float&       slot      = dp_cost[next_mask * n + next];
                            if (new_cost < slot) {
                                slot                              = new_cost;
                                tsp_parent[next_mask * n + next] = (int8_t) curr;
                            }
                        }
                    }
                }
            }

            std::vector<CompactPath> results(num_ex);
            int                      final_mask = states - 1;
            for (int i = 0; i < n; ++i) {
                float end_cost = dp_cost[final_mask * n + i];
                if (end_cost == INF)
                    continue;
                std::vector<uint8_t> path;
                int                  curr_p = i;
                int                  curr_m = final_mask;
                while (curr_p != -1) {
                    path.push_back((uint8_t) extruders[curr_p]);
                    int8_t prev_p = tsp_parent[curr_m * n + curr_p];
                    curr_m &= ~(1 << curr_p);
                    curr_p = (int) prev_p;
                }
                std::reverse(path.begin(), path.end());
                results[extruders[i]].set(end_cost, path.data(), path.size());
            }
            return results;
        };

        struct TspQuery
        {
            uint32_t                  mask_val;
            unsigned int              start_ex;
            std::vector<unsigned int> extruders;

            bool operator<(const TspQuery& other) const
            {
                if (mask_val != other.mask_val)
                    return mask_val < other.mask_val;
                return start_ex < other.start_ex;
            }
        };

        std::set<TspQuery> unique_queries;

        for (size_t v = 0; v < valid_layers.size(); ++v) {
            LayerTools& lt           = m_layer_tools[valid_layers[v]];
            uint32_t    current_mask = 0;
            for (auto ex : lt.extruders)
                current_mask |= (1 << ex);

            if (v == 0) {
                unique_queries.insert({current_mask, lt.extruders.front(), lt.extruders});
            } else {
                for (auto start_ex : lt.extruders) {
                    unique_queries.insert({current_mask, start_ex, lt.extruders});
                }
            }
        }

        std::vector<TspQuery>                 query_vec(unique_queries.begin(), unique_queries.end());
        std::vector<std::vector<CompactPath>> query_results(query_vec.size());

        tbb::parallel_for(size_t(0), query_vec.size(),
                          [&](size_t idx) { query_results[idx] = compute_layer_tsp(query_vec[idx].extruders, query_vec[idx].start_ex); });

        std::map<std::pair<uint32_t, unsigned int>, std::vector<CompactPath>> tsp_cache;
        for (size_t i = 0; i < query_vec.size(); ++i) {
            tsp_cache[{query_vec[i].mask_val, query_vec[i].start_ex}] = std::move(query_results[i]);
        }
        query_results.clear(); // release temporary memory early

        auto get_layer_tsp = [&](const std::vector<unsigned int>& extruders, unsigned int start_ex) -> const std::vector<CompactPath>& {
            uint32_t mask_val = 0;
            for (auto ex : extruders)
                mask_val |= (1 << ex);
            return tsp_cache.at({mask_val, start_ex});
        };

        // Opt-1+2: replace dp_global (L x num_ex CompactPath, each storing a full path vector)
        // with two rolling float rows + compact int8_t parent/which_start tables.
        // Saves ~L * num_ex * avg_path_len bytes; paths are reconstructed during traceback.
        const size_t L = valid_layers.size();
        std::vector<float>   dp_prev(num_ex, INF);
        std::vector<float>   dp_curr(num_ex, INF);
        std::vector<std::vector<int8_t>>  parent_ex(L, std::vector<int8_t>(num_ex, -1));
        std::vector<std::vector<uint8_t>> which_start(L, std::vector<uint8_t>(num_ex, 0xFF));

        {
            LayerTools& lt  = m_layer_tools[valid_layers[0]];
            const auto& res = get_layer_tsp(lt.extruders, lt.extruders.front());
            for (unsigned int ex : lt.extruders) {
                if (res[ex].cost < INF) {
                    dp_prev[ex]         = res[ex].cost;
                    which_start[0][ex]  = (uint8_t) lt.extruders.front();
                }
            }
        }

        for (size_t v = 1; v < L; ++v) {
            LayerTools& lt           = m_layer_tools[valid_layers[v]];
            uint32_t    current_mask = 0;
            for (auto ex : lt.extruders)
                current_mask |= (1 << ex);

            std::fill(dp_curr.begin(), dp_curr.end(), INF);

            for (unsigned int prev_e = 0; prev_e < num_ex; ++prev_e) {
                if (dp_prev[prev_e] == INF)
                    continue;

                bool                      can_link = (current_mask & (1 << prev_e)) != 0;
                std::vector<unsigned int> starts   = can_link ? std::vector<unsigned int>{prev_e} : lt.extruders;

                for (unsigned int start : starts) {
                    float       transition = (start == prev_e) ? 0.0f : wipe_volumes[prev_e][start];
                    const auto& tsp_res    = get_layer_tsp(lt.extruders, start);

                    for (unsigned int curr_e : lt.extruders) {
                        if (tsp_res[curr_e].cost == INF)
                            continue;
                        float total = dp_prev[prev_e] + transition + tsp_res[curr_e].cost;
                        // Use the same tie-breaking as before: lower cost wins; equal cost uses
                        // lexicographic path comparison via tsp_res paths.
                        bool better = false;
                        if (total < dp_curr[curr_e] - 1e-4f) {
                            better = true;
                        } else if (total <= dp_curr[curr_e] + 1e-4f) {
                            // Tie-break: compare paths lexicographically
                            if (which_start[v][curr_e] != 0xFF) {
                                const auto& old_path = get_layer_tsp(lt.extruders, (unsigned int) which_start[v][curr_e])[curr_e].path;
                                const auto& new_path = tsp_res[curr_e].path;
                                for (size_t pi = 0; pi < std::min(old_path.size(), new_path.size()); ++pi) {
                                    if (new_path[pi] < old_path[pi]) { better = true; break; }
                                    if (new_path[pi] > old_path[pi]) break;
                                }
                                if (!better && new_path.size() < old_path.size())
                                    better = true;
                            } else {
                                better = true;
                            }
                        }
                        if (better) {
                            dp_curr[curr_e]         = total;
                            parent_ex[v][curr_e]    = (int8_t) prev_e;
                            which_start[v][curr_e]  = (uint8_t) start;
                        }
                    }
                }
            }
            std::swap(dp_prev, dp_curr);
        }

        float min_total = INF;
        int   best_last = -1;
        for (unsigned int i = 0; i < num_ex; ++i) {
            if (dp_prev[i] < min_total) {
                min_total = dp_prev[i];
                best_last = (int) i;
            }
        }

        // Traceback: reconstruct paths by re-querying tsp_cache with which_start.
        if (best_last != -1) {
            int curr = best_last;
            for (int v = (int) L - 1; v >= 0; --v) {
                LayerTools&  lt    = m_layer_tools[valid_layers[v]];
                uint8_t      ws    = which_start[v][(unsigned int) curr];
                unsigned int s_ex  = (ws != 0xFF) ? (unsigned int) ws : lt.extruders.front();
                const auto&  tpath = get_layer_tsp(lt.extruders, s_ex)[(unsigned int) curr].path;
                lt.extruders.assign(tpath.begin(), tpath.end());
                // parent_ex stores int8_t; -1 (0xFF) means no parent (first layer).
                int8_t p = parent_ex[v][(unsigned int) curr];
                curr = (p == -1) ? -1 : (int)(unsigned int)(uint8_t) p;
            }
        }


    } else {
        using ToolOrderCacheKey = std::pair<std::vector<unsigned int>, std::optional<unsigned int>>;
        auto extruders_to_hash_key = [](const std::vector<unsigned int>& extruders,
                                        std::optional<unsigned int>      initial_extruder_id) -> ToolOrderCacheKey {
            std::vector<unsigned int> curr_set = extruders;
            std::sort(curr_set.begin(), curr_set.end());
            curr_set.erase(std::unique(curr_set.begin(), curr_set.end()), curr_set.end());
            return { std::move(curr_set), initial_extruder_id };
        };

        std::optional<unsigned int> current_extruder_id;
        for (int i = 0; i < m_layer_tools.size(); ++i) {
            LayerTools& lt = m_layer_tools[i];
            if (lt.extruders.empty())
                continue;

            std::vector<int> custom_extruder_seq;
            if (get_custom_seq(i, custom_extruder_seq) && !custom_extruder_seq.empty()) {
                std::vector<unsigned int> unsign_custom_extruder_seq;
                for (int extruder : custom_extruder_seq) {
                    unsigned int unsign_extruder = static_cast<unsigned int>(extruder) - 1;
                    auto         it              = std::find(lt.extruders.begin(), lt.extruders.end(), unsign_extruder);
                    if (it != lt.extruders.end()) {
                        unsign_custom_extruder_seq.emplace_back(unsign_extruder);
                    }
                }
                assert(lt.extruders.size() == unsign_custom_extruder_seq.size());
                lt.extruders        = unsign_custom_extruder_seq;
                current_extruder_id = lt.extruders.back();
                continue;
            }

            // The algorithm complexity is O(n2*2^n)
            if (i != 0) {
                auto hash_key = extruders_to_hash_key(lt.extruders, current_extruder_id);
                auto iter     = m_tool_order_cache.find(hash_key);
                if (iter == m_tool_order_cache.end()) {
                    lt.extruders = get_extruders_order(wipe_volumes, lt.extruders, current_extruder_id);
                    std::vector<uint8_t> hash_val;
                    hash_val.reserve(lt.extruders.size());
                    for (auto item : lt.extruders)
                        hash_val.emplace_back(static_cast<uint8_t>(item));
                    m_tool_order_cache[hash_key] = hash_val;
                } else {
                    std::vector<unsigned int> extruder_order;
                    extruder_order.reserve(iter->second.size());
                    for (auto item : iter->second)
                        extruder_order.emplace_back(static_cast<unsigned int>(item));
                    lt.extruders = std::move(extruder_order);
                }
            }
            current_extruder_id = lt.extruders.back();
        }
    }
}

// Layers are marked for infinite skirt aka draft shield. Not all the layers have to be printed.
void ToolOrdering::mark_skirt_layers(const PrintConfig &config, coordf_t max_layer_height)
{
    if (m_layer_tools.empty())
        return;

    if (m_layer_tools.front().extruders.empty()) {
        // Empty first layer, no skirt will be printed.
        //FIXME throw an exception?
        return;
    }

    size_t i = 0;
    for (;;) {
        m_layer_tools[i].has_skirt = true;
        size_t j = i + 1;
        for (; j < m_layer_tools.size() && ! m_layer_tools[j].has_object; ++ j);
        // i and j are two successive layers printing an object.
        if (j == m_layer_tools.size())
            // Don't print skirt above the last object layer.
            break;
        // Mark some printing intermediate layers as having skirt.
        double last_z = m_layer_tools[i].print_z;
        for (size_t k = i + 1; k < j; ++ k) {
            if (m_layer_tools[k + 1].print_z - last_z > max_layer_height + EPSILON) {
                // Layer k is the last one not violating the maximum layer height.
                // Don't extrude skirt on empty layers.
                while (m_layer_tools[k].extruders.empty())
                    -- k;
                if (m_layer_tools[k].has_skirt) {
                    // Skirt cannot be generated due to empty layers, there would be a missing layer in the skirt.
                    //FIXME throw an exception?
                    break;
                }
                m_layer_tools[k].has_skirt = true;
                last_z = m_layer_tools[k].print_z;
            }
        }
        i = j;
    }
}

// Assign a pointer to a custom G-code to the respective ToolOrdering::LayerTools.
// Ignore color changes, which are performed on a layer and for such an extruder, that the extruder will not be printing above that layer.
// If multiple events are planned over a span of a single layer, use the last one.

// BBS: replace model custom gcode with current plate custom gcode
static CustomGCode::Info custom_gcode_per_print_z;
void ToolOrdering::assign_custom_gcodes(const Print &print)
{
	// Only valid for non-sequential print.
	assert(print.config().print_sequence == PrintSequence::ByLayer);

    custom_gcode_per_print_z = print.model().get_curr_plate_custom_gcodes();
	if (custom_gcode_per_print_z.gcodes.empty())
		return;

    // BBS
	auto 						num_filaments = unsigned(print.config().filament_diameter.size());
	CustomGCode::Mode 			mode          =
		(num_filaments == 1) ? CustomGCode::SingleExtruder :
		print.object_extruders().size() == 1 ? CustomGCode::MultiAsSingle : CustomGCode::MultiExtruder;
    CustomGCode::Mode           model_mode    = print.model().get_curr_plate_custom_gcodes().mode;
	std::vector<unsigned char> 	extruder_printing_above(num_filaments, false);
	auto 						custom_gcode_it = custom_gcode_per_print_z.gcodes.rbegin();
	// Tool changes and color changes will be ignored, if the model's tool/color changes were entered in mm mode and the print is in non mm mode
	// or vice versa.
	bool 						ignore_tool_and_color_changes = (mode == CustomGCode::MultiExtruder) != (model_mode == CustomGCode::MultiExtruder);
	// If printing on a single extruder machine, make the tool changes trigger color change (M600) events.
	bool 						tool_changes_as_color_changes = mode == CustomGCode::SingleExtruder && model_mode == CustomGCode::MultiAsSingle;

	// From the last layer to the first one:
	for (auto it_lt = m_layer_tools.rbegin(); it_lt != m_layer_tools.rend(); ++ it_lt) {
		LayerTools &lt = *it_lt;
		// Add the extruders of the current layer to the set of extruders printing at and above this print_z.
		for (unsigned int i : lt.extruders)
			extruder_printing_above[i] = true;
		// Skip all custom G-codes above this layer and skip all extruder switches.
		for (; custom_gcode_it != custom_gcode_per_print_z.gcodes.rend() && (custom_gcode_it->print_z > lt.print_z + EPSILON || custom_gcode_it->type == CustomGCode::ToolChange); ++ custom_gcode_it);
		if (custom_gcode_it == custom_gcode_per_print_z.gcodes.rend())
			// Custom G-codes were processed.
			break;
		// Some custom G-code is configured for this layer or a layer below.
		const CustomGCode::Item &custom_gcode = *custom_gcode_it;
		// print_z of the layer below the current layer.
		coordf_t print_z_below = 0.;
		if (auto it_lt_below = it_lt; ++ it_lt_below != m_layer_tools.rend())
			print_z_below = it_lt_below->print_z;
		if (custom_gcode.print_z > print_z_below + 0.5 * EPSILON) {
			// The custom G-code applies to the current layer.
			bool color_change = custom_gcode.type == CustomGCode::ColorChange;
			bool tool_change  = custom_gcode.type == CustomGCode::ToolChange;
			bool pause_or_custom_gcode = ! color_change && ! tool_change;
			bool apply_color_change = ! ignore_tool_and_color_changes &&
				// If it is color change, it will actually be useful as the exturder above will print.
                // BBS
				(color_change ? 
					mode == CustomGCode::SingleExtruder || 
						(custom_gcode.extruder <= int(num_filaments) && extruder_printing_above[unsigned(custom_gcode.extruder - 1)]) :
				 	tool_change && tool_changes_as_color_changes);
			if (pause_or_custom_gcode || apply_color_change)
        		lt.custom_gcode = &custom_gcode;
			// Consume that custom G-code event.
			++ custom_gcode_it;
		}
	}
}

const LayerTools& ToolOrdering::tools_for_layer(coordf_t print_z) const
{
    auto it_layer_tools = std::lower_bound(m_layer_tools.begin(), m_layer_tools.end(), LayerTools(print_z - EPSILON));
    assert(it_layer_tools != m_layer_tools.end());
    coordf_t dist_min = std::abs(it_layer_tools->print_z - print_z);
    for (++ it_layer_tools; it_layer_tools != m_layer_tools.end(); ++ it_layer_tools) {
        coordf_t d = std::abs(it_layer_tools->print_z - print_z);
        if (d >= dist_min)
            break;
        dist_min = d;
    }
    -- it_layer_tools;
    assert(dist_min < EPSILON);
    return *it_layer_tools;
}

// This function is called from Print::mark_wiping_extrusions and sets extruder this entity should be printed with (-1 .. as usual)
void WipingExtrusions::set_extruder_override(const ExtrusionEntity* entity, const PrintObject* object, size_t copy_id, int extruder, size_t num_of_copies)
{
    something_overridden = true;

    auto entity_map_it = (entity_map.emplace(std::make_tuple(entity, object), ExtruderPerCopy())).first; // (add and) return iterator
    ExtruderPerCopy& copies_vector = entity_map_it->second;
    copies_vector.resize(num_of_copies, -1);

    if (copies_vector[copy_id] != -1)
        std::cout << "ERROR: Entity extruder overriden multiple times!!!\n";    // A debugging message - this must never happen.

    copies_vector[copy_id] = extruder;
}

// BBS


bool WipingExtrusions::is_skeleton_flush_target(const PrintObject* object, unsigned int copy, unsigned int old_extruder, unsigned int new_extruder) const
{
    return skeleton_flush_targets.find(std::make_tuple(object, copy, old_extruder, new_extruder)) != skeleton_flush_targets.end();
}

void WipingExtrusions::set_support_extruder_override(const PrintObject* object, size_t copy_id, int extruder, size_t num_of_copies)
{
    something_overridden = true;
    support_map.emplace(object, extruder);
}

void WipingExtrusions::set_support_interface_extruder_override(const PrintObject* object, size_t copy_id, int extruder, size_t num_of_copies)
{
    something_overridden = true;
    support_intf_map.emplace(object, extruder);
}

// Finds first non-soluble extruder on the layer
int WipingExtrusions::first_nonsoluble_extruder_on_layer(const PrintConfig& print_config) const
{
    const LayerTools& lt = *m_layer_tools;
    for (auto extruders_it = lt.extruders.begin(); extruders_it != lt.extruders.end(); ++extruders_it)
        if (!print_config.filament_soluble.get_at(*extruders_it) && !print_config.filament_is_support.get_at(*extruders_it))
            return (*extruders_it);

    return (-1);
}

// Finds last non-soluble extruder on the layer
int WipingExtrusions::last_nonsoluble_extruder_on_layer(const PrintConfig& print_config) const
{
    const LayerTools& lt = *m_layer_tools;
    for (auto extruders_it = lt.extruders.rbegin(); extruders_it != lt.extruders.rend(); ++extruders_it)
        if (!print_config.filament_soluble.get_at(*extruders_it) && !print_config.filament_is_support.get_at(*extruders_it))
            return (*extruders_it);

    return (-1);
}

// Decides whether this entity could be overridden
bool WipingExtrusions::is_overriddable(const ExtrusionEntityCollection& eec, const PrintConfig& print_config, const PrintObject& object, const PrintRegion& region) const
{
    if (print_config.filament_soluble.get_at(m_layer_tools->extruder(eec, region)))
        return false;

    if (object.config().flush_into_objects)
        return true;

    if (can_flush_into_skeleton(eec, print_config, object))
        return true;

    if (!object.config().flush_into_infill || eec.role() != erInternalInfill)
        return false;

    return true;
}
bool WipingExtrusions::is_obj_overriddable(const ExtrusionEntityCollection &eec, const PrintObject &object) const
{
    if (object.config().flush_into_objects)
        return true;

    if (can_flush_into_skeleton(eec, object.print()->config(), object))
        return true;

    if (object.config().flush_into_infill && eec.role() == erInternalInfill)
        return true;

    return false;
}

// BBS
bool WipingExtrusions::is_support_overriddable(const ExtrusionRole role, const PrintObject& object) const
{
    if (!object.config().flush_into_support)
        return false;

    if (role == erMixed) {
        return object.config().support_filament == 0 || object.config().support_interface_filament == 0;
    }
    else if (role == erSupportMaterial || role == erSupportTransition) {
        return object.config().support_filament == 0;
    }
    else if (role == erSupportMaterialInterface) {
        return object.config().support_interface_filament == 0;
    }

    return false;
}

// Following function iterates through all extrusions on the layer, remembers those that could be used for wiping after toolchange
// and returns volume that is left to be wiped on the wipe tower.
float WipingExtrusions::mark_wiping_extrusions(const Print& print, unsigned int old_extruder, unsigned int new_extruder, float volume_to_wipe, float skeleton_volume_to_wipe, bool skeleton_only)
{
    const LayerTools& lt = *m_layer_tools;
    const float min_infill_volume = 0.f; // ignore infill with smaller volume than this
    float skeleton_volume_left = skeleton_volume_to_wipe < 0.f ? volume_to_wipe : std::min(volume_to_wipe, std::max(0.f, skeleton_volume_to_wipe));

    if (! this->something_overridable || volume_to_wipe <= 0. || print.config().filament_soluble.get_at(old_extruder) || print.config().filament_soluble.get_at(new_extruder))
        return std::max(0.f, volume_to_wipe); // Soluble filament cannot be wiped in a random infill, neither the filament after it

    // BBS
    if (print.config().filament_is_support.get_at(old_extruder) || print.config().filament_is_support.get_at(new_extruder))
        return std::max(0.f, volume_to_wipe); // Support filament cannot be used to print support, infill, wipe_tower, etc.

    // we will sort objects so that dedicated for wiping are at the beginning:
    ConstPrintObjectPtrs object_list = print.objects().vector();
    // BBS: fix the exception caused by not fixed order between different objects
    std::sort(object_list.begin(), object_list.end(), [object_list](const PrintObject* a, const PrintObject* b) {
        if (a->config().flush_into_objects != b->config().flush_into_objects) {
            return a->config().flush_into_objects.getBool();
        }
        else {
            return a->id() < b->id();
        }
    });

    auto skeleton_instances_in_print_order = [&]() {
        std::vector<const PrintInstance*> instances;
        if (print.config().print_sequence == PrintSequence::ByLayer && print.config().print_order == PrintOrder::Default) {
            if (print.config().enable_prime_tower.value && print.extruders().size() > 1) {
                ConstPrintObjectPtrs skeleton_objects = print.objects().vector();
                const int plate_idx = print.get_plate_index();
                Point wt_pos(print.config().wipe_tower_x.get_at(plate_idx), print.config().wipe_tower_y.get_at(plate_idx));
                instances = chain_print_object_instances(skeleton_objects, &wt_pos);
                std::reverse(instances.begin(), instances.end());
            } else {
                instances = chain_print_object_instances(print);
            }
        } else {
            for (const PrintObject* object : print.objects())
                for (const PrintInstance& instance : object->instances())
                    instances.emplace_back(&instance);
        }
        return instances;
    };

    const std::vector<const PrintInstance*> skeleton_instance_order = skeleton_instances_in_print_order();

    auto try_mark_skeleton_flush = [&](ExtrusionEntityCollection& fills, size_t fill_idx, const PrintObject* object,
                                        unsigned int copy, const PrintRegion& region, size_t num_of_copies) -> bool {
        if (fill_idx >= fills.entities.size())
            return false;

        auto* fill = dynamic_cast<const ExtrusionEntityCollection*>(fills.entities[fill_idx]);
        if (fill == nullptr || !can_flush_into_skeleton(*fill, print.config(), *object))
            return false;

        if (skeleton_volume_left <= min_infill_volume)
            return false;
        if (lt.extruder(*fill, region) != old_extruder)
            return false;
        float consumed_volume = float(fill->total_volume());
        const bool already_overridden_in_some_copy = entity_map.find(std::make_tuple(fill, object)) != entity_map.end();
        if (already_overridden_in_some_copy && consumed_volume > skeleton_volume_left + EPSILON)
            return false;

        ExtrusionEntityCollection* wipe_fill = nullptr;
        if (!already_overridden_in_some_copy &&
            !split_skeleton_collection_for_wipe(fills, fill_idx, skeleton_volume_left, wipe_fill, consumed_volume))
            return false;
        if (wipe_fill != nullptr) {
            consumed_volume = float(wipe_fill->total_volume());
            fill = wipe_fill;
        }

        skeleton_volume_left -= consumed_volume;
        set_extruder_override(fill, object, copy, old_extruder, num_of_copies);
        skeleton_flush_targets.emplace(object, copy, old_extruder, new_extruder);
        skeleton_flush_volume_map[std::make_pair(old_extruder, new_extruder)] += consumed_volume;
        return true;
    };

    // Use as much of the available skeleton trajectory as the purge volume requires.
    // Mark skeletons in print order so GCode can defer them until all clean exterior
    // features of the current color are finished.
    auto mark_color_batch_skeleton_flush = [&]() {
        if (skeleton_volume_left <= min_infill_volume)
            return;

        for (const PrintInstance* instance : skeleton_instance_order) {
            const PrintObject* object = instance->print_object;
            if (!object->config().flush_into_skeleton.value)
                continue;

            const Layer* this_layer = object->get_layer_at_printz(lt.print_z, EPSILON);
            if (this_layer == nullptr)
                continue;

            const size_t num_of_copies = object->instances().size();
            const unsigned int copy = static_cast<unsigned int>(instance - object->instances().data());
            if (copy >= num_of_copies)
                continue;

            for (const LayerRegion* layerm : this_layer->regions()) {
                const auto& region = layerm->region();
                ExtrusionEntityCollection& fills = const_cast<ExtrusionEntityCollection&>(layerm->fills);
                for (size_t fill_idx = 0; fill_idx < fills.entities.size(); ++fill_idx) {
                    try_mark_skeleton_flush(fills, fill_idx, object, copy, region, num_of_copies);
                    if (skeleton_volume_left <= min_infill_volume)
                        break;
                }
                if (skeleton_volume_left <= min_infill_volume)
                    break;
            }
        }
    };

    mark_color_batch_skeleton_flush();
    // We will now iterate through
    //  - first the dedicated objects to mark perimeters or infills (depending on infill_first)
    //  - second through the dedicated ones again to mark infills or perimeters (depending on infill_first)
    //  - then all the others to mark infills (in case that !infill_first, we must also check that the perimeter is finished already
    // this is controlled by the following variable:
    bool perimeters_done = false;

    for (int i=0 ; i<(int)object_list.size() + (perimeters_done ? 0 : 1); ++i) {
        if (!perimeters_done && (i==(int)object_list.size() || !object_list[i]->config().flush_into_objects)) { // we passed the last dedicated object in list
            perimeters_done = true;
            i=-1;   // let's go from the start again
            continue;
        }

        const PrintObject* object = object_list[i];

        // Finds this layer:
        const Layer* this_layer = object->get_layer_at_printz(lt.print_z, EPSILON);
        if (this_layer == nullptr)
        	continue;

        size_t num_of_copies = object->instances().size();

        // iterate through copies (aka PrintObject instances) first, so that we mark neighbouring infills to minimize travel moves
        for (unsigned int copy = 0; copy < num_of_copies; ++copy) {
            for (const LayerRegion *layerm : this_layer->regions()) {
                const auto &region = layerm->region();

                if (!object->config().flush_into_infill && !object->config().flush_into_objects && !object->config().flush_into_support
                    && !object->config().flush_into_skeleton && !print.config().multicolor_method.value)
                    continue;
                bool wipe_into_infill_only = !object->config().flush_into_objects && object->config().flush_into_infill;
                bool is_infill_first = region.config().is_infill_first;
                if (is_infill_first != perimeters_done || wipe_into_infill_only) {
                    ExtrusionEntityCollection& fills = const_cast<ExtrusionEntityCollection&>(layerm->fills);
                    for (size_t fill_idx = 0; fill_idx < fills.entities.size(); ++fill_idx) { // iterate through all infill Collections
                        auto* fill = dynamic_cast<const ExtrusionEntityCollection*>(fills.entities[fill_idx]);
                        if (fill == nullptr)
                            continue;

                        if (!is_overriddable(*fill, print.config(), *object, region))
                            continue;

                        if (wipe_into_infill_only && ! is_infill_first)
                            // In this case we must check that the original extruder is used on this layer before the one we are overridding
                            // (and the perimeters will be finished before the infill is printed):
                            if (!lt.is_extruder_order(lt.wall_filament(region), new_extruder))
                                continue;

                        if ((!is_entity_overridden(fill, object, copy) && fill->total_volume() > min_infill_volume))
                        {     // this infill will be used to wipe this extruder
                            const bool flush_into_skeleton = can_flush_into_skeleton(*fill, print.config(), *object);
                            if (skeleton_only && !flush_into_skeleton)
                                continue;

                            if (flush_into_skeleton) {
                                if (!try_mark_skeleton_flush(fills, fill_idx, object, copy, region, num_of_copies))
                                    continue;
                            } else {
                                float consumed_volume = float(fill->total_volume());
                                set_extruder_override(fill, object, copy, new_extruder, num_of_copies);
                                if ((volume_to_wipe -= consumed_volume) <= 0.f) {
                                    // More material was purged already than asked for.
                                    return 0.f;
                                }
                            }
                        }
                    }
                }

                // Now the same for perimeters - see comments above for explanation:
                if (!skeleton_only && object->config().flush_into_objects && is_infill_first == perimeters_done)
                {
                    for (const ExtrusionEntity* ee : layerm->perimeters.entities) {
                        auto* fill = dynamic_cast<const ExtrusionEntityCollection*>(ee);
                        if (is_overriddable(*fill, print.config(), *object, region) && !is_entity_overridden(fill, object, copy) && fill->total_volume() > min_infill_volume) {
                            set_extruder_override(fill, object, copy, new_extruder, num_of_copies);
                            if ((volume_to_wipe -= float(fill->total_volume())) <= 0.f)
                            	// More material was purged already than asked for.
	                            return 0.f;
                        }
                    }
                }
            }

            // BBS
            if (!skeleton_only && object->config().flush_into_support) {
                auto& object_config = object->config();
                const SupportLayer* this_support_layer = object->get_support_layer_at_printz(lt.print_z, EPSILON);

                do {
                    if (this_support_layer == nullptr)
                        break;

                    bool support_overriddable = object_config.support_filament == 0;
                    bool support_intf_overriddable = object_config.support_interface_filament == 0;
                    if (!support_overriddable && !support_intf_overriddable)
                        break;

                    auto &entities = this_support_layer->support_fills.entities;
                    if (support_overriddable && !is_support_overridden(object) && !(object_config.support_interface_not_for_body.value && !support_intf_overriddable &&(new_extruder==object_config.support_interface_filament-1||old_extruder==object_config.support_interface_filament-1))) {
                        set_support_extruder_override(object, copy, new_extruder, num_of_copies);
                        for (const ExtrusionEntity* ee : entities) {
                            if (ee->role() == erSupportMaterial || ee->role() == erSupportTransition)
                                volume_to_wipe -= ee->total_volume();

                            if (volume_to_wipe <= 0.f)
                                return 0.f;
                        }
                    }

                    if (support_intf_overriddable && !is_support_interface_overridden(object)) {
                        set_support_interface_extruder_override(object, copy, new_extruder, num_of_copies);
                        for (const ExtrusionEntity* ee : entities) {
                            if (ee->role() == erSupportMaterialInterface)
                                volume_to_wipe -= ee->total_volume();

                            if (volume_to_wipe <= 0.f)
                                return 0.f;
                        }
                    }
                } while (0);
            }
        }
    }
	// Some purge remains to be done on the Wipe Tower.
    assert(volume_to_wipe > 0.);
    return volume_to_wipe;
}



// Called after all toolchanges on a layer were mark_infill_overridden. There might still be overridable entities,
// that were not actually overridden. If they are part of a dedicated object, printing them with the extruder
// they were initially assigned to might mean violating the perimeter-infill order. We will therefore go through
// them again and make sure we override it.
void WipingExtrusions::ensure_perimeters_infills_order(const Print& print)
{
	if (! this->something_overridable)
		return;

    const LayerTools& lt = *m_layer_tools;
    unsigned int first_nonsoluble_extruder = first_nonsoluble_extruder_on_layer(print.config());
    unsigned int last_nonsoluble_extruder = last_nonsoluble_extruder_on_layer(print.config());

    for (const PrintObject* object : print.objects()) {
        // Finds this layer:
        const Layer* this_layer = object->get_layer_at_printz(lt.print_z, EPSILON);
        if (this_layer == nullptr)
        	continue;
        size_t num_of_copies = object->instances().size();

        for (size_t copy = 0; copy < num_of_copies; ++copy) {    // iterate through copies first, so that we mark neighbouring infills to minimize travel moves
            for (const LayerRegion *layerm : this_layer->regions()) {
                const auto &region = layerm->region();
                //BBS
                if (!object->config().flush_into_infill && !object->config().flush_into_objects
                    && !object->config().flush_into_skeleton && !print.config().multicolor_method.value)
                    continue;

                bool is_infill_first = region.config().is_infill_first;
                for (const ExtrusionEntity* ee : layerm->fills.entities) {                      // iterate through all infill Collections
                    auto* fill = dynamic_cast<const ExtrusionEntityCollection*>(ee);

                    if (can_flush_into_skeleton(*fill, print.config(), *object))
                        continue;

                    if (!is_overriddable(*fill, print.config(), *object, region)
                     || is_entity_overridden(fill, object, copy) )
                        continue;

                    // This infill could have been overridden but was not - unless we do something, it could be
                    // printed before its perimeter, or not be printed at all (in case its original extruder has
                    // not been added to LayerTools
                    // Either way, we will now force-override it with something suitable:
                    //BBS
                    if (is_infill_first
                    //BBS
                    //|| object->config().flush_into_objects  // in this case the perimeter is overridden, so we can override by the last one safely
                    || lt.is_extruder_order(lt.wall_filament(region), last_nonsoluble_extruder    // !infill_first, but perimeter is already printed when last extruder prints
                    || ! lt.has_extruder(lt.sparse_infill_filament(region)))) // we have to force override - this could violate infill_first (FIXME)
                        set_extruder_override(fill, object, copy, (is_infill_first ? first_nonsoluble_extruder : last_nonsoluble_extruder), num_of_copies);
                    else {
                        // In this case we can (and should) leave it to be printed normally.
                        // Force overriding would mean it gets printed before its perimeter.
                    }
                }

                // Now the same for perimeters - see comments above for explanation:
                for (const ExtrusionEntity* ee : layerm->perimeters.entities) {                      // iterate through all perimeter Collections
                    auto* fill = dynamic_cast<const ExtrusionEntityCollection*>(ee);
                    if (is_overriddable(*fill, print.config(), *object, region) && ! is_entity_overridden(fill, object, copy))
                        set_extruder_override(fill, object, copy, (is_infill_first ? last_nonsoluble_extruder : first_nonsoluble_extruder), num_of_copies);
                }
            }
        }
    }
}

// Following function is called from GCode::process_layer and returns pointer to vector with information about which extruders should be used for given copy of this entity.
// If this extrusion does not have any override, nullptr is returned.
// Otherwise it modifies the vector in place and changes all -1 to correct_extruder_id (at the time the overrides were created, correct extruders were not known,
// so -1 was used as "print as usual").
// The resulting vector therefore keeps track of which extrusions are the ones that were overridden and which were not. If the extruder used is overridden,
// its number is saved as is (zero-based index). Regular extrusions are saved as -number-1 (unfortunately there is no negative zero).
const WipingExtrusions::ExtruderPerCopy* WipingExtrusions::get_extruder_overrides(const ExtrusionEntity* entity, const PrintObject* object, int correct_extruder_id, size_t num_of_copies)
{
	ExtruderPerCopy *overrides = nullptr;
    auto entity_map_it = entity_map.find(std::make_tuple(entity, object));
    if (entity_map_it != entity_map.end()) {
        overrides = &entity_map_it->second;
    	overrides->resize(num_of_copies, -1);
	    // Each -1 now means "print as usual" - we will replace it with actual extruder id (shifted it so we don't lose that information):
	    std::replace(overrides->begin(), overrides->end(), -1, -correct_extruder_id-1);
	}
    return overrides;
}

WipingExtrusions::ExtruderPerCopy WipingExtrusions::resolved_extruder_overrides(const ExtrusionEntity* entity, const PrintObject& object, int correct_extruder_id, size_t num_of_copies) const
{
    ExtruderPerCopy resolved;
    resolved.resize(num_of_copies, correct_extruder_id);

    auto entity_map_it = entity_map.find(std::make_tuple(entity, &object));
    if (entity_map_it == entity_map.end())
        return resolved;

    const ExtruderPerCopy& overrides = entity_map_it->second;
    for (size_t copy_id = 0; copy_id < num_of_copies; ++copy_id) {
        const int extruder = copy_id < overrides.size() ? overrides[copy_id] : -1;
        if (extruder >= 0)
            resolved[copy_id] = extruder;
        else if (extruder < -1)
            resolved[copy_id] = -extruder - 1;
        else
            resolved[copy_id] = correct_extruder_id;
    }

    return resolved;
}

// BBS
int WipingExtrusions::get_support_extruder_overrides(const PrintObject* object)
{
    auto iter = support_map.find(object);
    if (iter != support_map.end())
        return iter->second;

    return -1;
}

int WipingExtrusions::get_support_interface_extruder_overrides(const PrintObject* object)
{
    auto iter = support_intf_map.find(object);
    if (iter != support_intf_map.end())
        return iter->second;

    return -1;
}

float WipingExtrusions::skeleton_flush_volume(unsigned int old_extruder, unsigned int new_extruder) const
{
    auto it = skeleton_flush_volume_map.find(std::make_pair(old_extruder, new_extruder));
    return it == skeleton_flush_volume_map.end() ? 0.f : it->second;
}
// Resolve a 1-based filament ID through the mixed-filament manager for this layer.
unsigned int LayerTools::resolve_mixed_1based(unsigned int filament_id) const
{
    if (!has_mixed_filaments || filament_id <= num_physical)
        return filament_id;
    return resolve_mixed_with_layer_heights(mixed_mgr,
                                            num_physical,
                                            filament_id,
                                            this->layer_index,
                                            float(this->print_z),
                                            float(this->layer_height),
                                            mixed_layer_height_a,
                                            mixed_layer_height_b,
                                            mixed_base_layer_height);
}

// Return a zero based extruder from the region, or extruder_override if overriden.
unsigned int LayerTools::wall_filament(const PrintRegion &region) const
{
    assert(region.config().wall_filament.value > 0);
    unsigned int id = (this->extruder_override == 0) ? region.config().wall_filament.value : this->extruder_override;
    return resolve_mixed_1based(id) - 1;
}

unsigned int LayerTools::sparse_infill_filament(const PrintRegion &region) const
{
    assert(region.config().sparse_infill_filament.value > 0);
    unsigned int id = (this->extruder_override == 0) ? region.config().sparse_infill_filament.value : this->extruder_override;
    return resolve_mixed_1based(id) - 1;
}

unsigned int LayerTools::solid_infill_filament(const PrintRegion &region) const
{
    assert(region.config().solid_infill_filament.value > 0);
    unsigned int id = (this->extruder_override == 0) ? region.config().solid_infill_filament.value : this->extruder_override;
    return resolve_mixed_1based(id) - 1;
}

// Resolve a 1-based filament ID through the mixed-filament manager.
unsigned int ToolOrdering::resolve_mixed(unsigned int filament_id_1based,
                                         int          layer_index,
                                         float        layer_print_z,
                                         float        layer_height) const
{
    if (!m_has_mixed_filaments || filament_id_1based <= m_num_physical)
        return filament_id_1based;
    return resolve_mixed_with_layer_heights(m_mixed_mgr,
                                            m_num_physical,
                                            filament_id_1based,
                                            layer_index,
                                            layer_print_z,
                                            layer_height,
                                            m_mixed_layer_height_a,
                                            m_mixed_layer_height_b,
                                            m_mixed_base_layer_height);
}

void ToolOrdering::update_mixed_layer_height_settings()
{
    const PrintConfig *cfg = m_print_config_ptr;
    if (cfg == nullptr && m_print_object_ptr != nullptr)
        cfg = &m_print_object_ptr->print()->config();

    m_mixed_layer_height_a = 0.f;
    m_mixed_layer_height_b = 0.f;
    if (m_print_full_config != nullptr &&
        m_print_full_config->has("mixed_color_layer_height_a") &&
        m_print_full_config->has("mixed_color_layer_height_b")) {
        m_mixed_layer_height_a = float(m_print_full_config->opt_float("mixed_color_layer_height_a"));
        m_mixed_layer_height_b = float(m_print_full_config->opt_float("mixed_color_layer_height_b"));
    } else if (cfg != nullptr) {
        m_mixed_layer_height_a = cfg->mixed_color_layer_height_a.value;
        m_mixed_layer_height_b = cfg->mixed_color_layer_height_b.value;
    }

    float base_height = 0.2f;
    if (m_print_object_ptr != nullptr)
        base_height = float(m_print_object_ptr->config().layer_height.value);
    m_mixed_base_layer_height = base_height;
}

} // namespace Slic3r
