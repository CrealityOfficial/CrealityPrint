#include <assert.h>
#include <stdio.h>
#include <memory>
#include <algorithm>

#include "../ClipperUtils.hpp"
#include "../Geometry.hpp"
#include "../Layer.hpp"
#include "../Print.hpp"
#include "../PrintConfig.hpp"
#include "../Surface.hpp"
#include "../AABBTreeLines.hpp"
#include "../FDM/NoWipeTowerMaterialChange.hpp"

#include "ExtrusionEntity.hpp"
#include "Fill.hpp"
#include "FillBase.hpp"
#include "FillRectilinear.hpp"
#include "FillLightning.hpp"
#include "FillCross.hpp"
#include "FillQuarter.hpp"
#include "FillConcentricInternal.hpp"
#include "FillTpmsD.hpp"
#include "FillTpmsFK.hpp"
#include "FillConcentric.hpp"
#include "libslic3r.h"

namespace Slic3r {

static float lockedzag_skin_depth_for_layer_region(const LayerRegion& layerm, const Surface* surface = nullptr)
{
    const PrintRegionConfig& region_config = layerm.region().config();
    if (surface != nullptr) {
        if (const float* resolved_skin_mm = layerm.layer()->object()->print()->lockedzag_skin_infill_depth(layerm, *surface))
            return *resolved_skin_mm;
    } else if (const float* resolved_skin_mm = layerm.layer()->object()->print()->lockedzag_skin_infill_depth(layerm)) {
        return *resolved_skin_mm;
    }


    return static_cast<float>(region_config.skin_infill_depth);
}

static float sparse_infill_density_for_surface(const LayerRegion& layerm, const Surface& surface)
{
    if (const float* resolved_density = layerm.layer()->object()->print()->layer_filament_wipe_packing_sparse_infill_density(layerm, surface))
        return *resolved_density;

    return static_cast<float>(layerm.region().config().sparse_infill_density);
}

static InfillPattern no_wipe_tower_plain_sparse_infill_pattern(const PrintRegionConfig& region_config)
{
    if (region_config.sparse_infill_pattern.value != ipLockedZag)
        return region_config.sparse_infill_pattern.value;

    const InfillPattern fallback = region_config.locked_skeleton_infill_pattern.value;
    return fallback == ipLockedZag ? ipGrid : fallback;
}

struct SurfaceFillParams
{
	// Zero based extruder ID.
    unsigned int 	extruder = 0;
	// Infill pattern, adjusted for the density etc.
    InfillPattern  	pattern = InfillPattern(0);
    // for locked zag
    InfillPattern skin_pattern     = InfillPattern(0);
    InfillPattern skeleton_pattern = InfillPattern(0);
    // FillBase
    // in unscaled coordinates
    coordf_t    	spacing = 0.;
    // infill / perimeter overlap, in unscaled coordinates
    coordf_t    	overlap = 0.;
    bool     rotate_angle = true;
    // Angle as provided by the region config, in radians.
    float       	angle = 0.f;
    // Is bridging used for this fill? Bridging parameters may be used even if this->flow.bridge() is not set.
    bool 			bridge;
    bool            enable_gap_fill = true;
    // Non-negative for a bridge.
    float 			bridge_angle = 0.f;

    // FillParams
    float       	density = 0.f;
    // Infill line multiplier count.
    int   multiline = 1;
    // Don't adjust spacing to fill the space evenly.
//    bool        	dont_adjust = false;
    // Length of the infill anchor along the perimeter line.
    // 1000mm is roughly the maximum length line that fits into a 32bit coord_t.
    float 			anchor_length     = 1000.f;
    float 			anchor_length_max = 1000.f;

    // width, height of extrusion, nozzle diameter, is bridge
    // For the output, for fill generator.
    Flow 			flow;

	// For the output
    ExtrusionRole	extrusion_role = ExtrusionRole(0);

	// Various print settings?

	// Index of this entry in a linear vector.
    size_t 			idx = 0;
	// infill speed settings
	float			sparse_infill_speed = 0;
	float			top_surface_speed = 0;
	float			solid_infill_speed = 0;

    float           infill_shift_step  = 0; // param for cross zag
    float           infill_rotate_step = 0; // param for zig zag to get cross texture


    // Params for lattice infill angles
    float lateral_lattice_angle_1 = 0.f;
    float lateral_lattice_angle_2 = 0.f;
    float infill_lock_depth          = 0;
    float skin_infill_depth          = 0;
    bool symmetric_infill_y_axis = false;
    bool solid_skeleton_wipe_path = false;
    size_t solid_skeleton_start_corner = 0;

    // Params for Lateral honeycomb
    float infill_overhang_angle = 45.f;

	bool operator<(const SurfaceFillParams &rhs) const {
#define RETURN_COMPARE_NON_EQUAL(KEY) if (this->KEY < rhs.KEY) return true; if (this->KEY > rhs.KEY) return false;
#define RETURN_COMPARE_NON_EQUAL_TYPED(TYPE, KEY) if (TYPE(this->KEY) < TYPE(rhs.KEY)) return true; if (TYPE(this->KEY) > TYPE(rhs.KEY)) return false;

		// Sort first by decreasing bridging angle, so that the bridges are processed with priority when trimming one layer by the other.
		if (this->bridge_angle > rhs.bridge_angle) return true;
		if (this->bridge_angle < rhs.bridge_angle) return false;

		RETURN_COMPARE_NON_EQUAL(extruder);
		RETURN_COMPARE_NON_EQUAL_TYPED(unsigned, pattern);
		RETURN_COMPARE_NON_EQUAL(spacing);
		RETURN_COMPARE_NON_EQUAL(overlap);
		RETURN_COMPARE_NON_EQUAL(angle);
        RETURN_COMPARE_NON_EQUAL(rotate_angle);
		RETURN_COMPARE_NON_EQUAL(density);
		RETURN_COMPARE_NON_EQUAL(multiline);
//		RETURN_COMPARE_NON_EQUAL_TYPED(unsigned, dont_adjust);
		RETURN_COMPARE_NON_EQUAL(anchor_length);
		RETURN_COMPARE_NON_EQUAL(anchor_length_max);
		RETURN_COMPARE_NON_EQUAL(flow.width());
		RETURN_COMPARE_NON_EQUAL(flow.height());
		RETURN_COMPARE_NON_EQUAL(flow.nozzle_diameter());
		RETURN_COMPARE_NON_EQUAL_TYPED(unsigned, bridge);
        RETURN_COMPARE_NON_EQUAL_TYPED(unsigned, enable_gap_fill);
		RETURN_COMPARE_NON_EQUAL_TYPED(unsigned, extrusion_role);
		RETURN_COMPARE_NON_EQUAL(sparse_infill_speed);
		RETURN_COMPARE_NON_EQUAL(top_surface_speed);
		RETURN_COMPARE_NON_EQUAL(solid_infill_speed);
        RETURN_COMPARE_NON_EQUAL(infill_shift_step);
        RETURN_COMPARE_NON_EQUAL(infill_rotate_step);
        RETURN_COMPARE_NON_EQUAL(lateral_lattice_angle_1);
		RETURN_COMPARE_NON_EQUAL(lateral_lattice_angle_2);
		RETURN_COMPARE_NON_EQUAL(symmetric_infill_y_axis);
		RETURN_COMPARE_NON_EQUAL(infill_lock_depth);
        RETURN_COMPARE_NON_EQUAL_TYPED(unsigned, solid_skeleton_wipe_path);
        RETURN_COMPARE_NON_EQUAL(solid_skeleton_start_corner);
        RETURN_COMPARE_NON_EQUAL(skin_infill_depth);
        RETURN_COMPARE_NON_EQUAL(infill_overhang_angle);
        RETURN_COMPARE_NON_EQUAL_TYPED(unsigned, skin_pattern);
        RETURN_COMPARE_NON_EQUAL_TYPED(unsigned, skeleton_pattern);
		return false;
	}

	bool operator==(const SurfaceFillParams &rhs) const {
		return  this->extruder 			== rhs.extruder 		&&
				this->pattern 			== rhs.pattern 			&&
				this->spacing 			== rhs.spacing 			&&
				this->overlap 			== rhs.overlap 			&&
                this->angle             == rhs.angle            && 
                this->rotate_angle      == rhs.rotate_angle     &&
				this->bridge   			== rhs.bridge   		&&
                this->enable_gap_fill == rhs.enable_gap_fill &&
				this->bridge_angle 		== rhs.bridge_angle		&&
				this->density   		== rhs.density   		&&
				this->multiline             == rhs.multiline    &&
//				this->dont_adjust   	== rhs.dont_adjust 		&&
				this->anchor_length  	== rhs.anchor_length    &&
				this->anchor_length_max == rhs.anchor_length_max &&
				this->flow 				== rhs.flow 			&&
				this->extrusion_role	== rhs.extrusion_role	&&
				this->sparse_infill_speed	== rhs.sparse_infill_speed &&
				this->top_surface_speed		== rhs.top_surface_speed &&
				this->solid_infill_speed	== rhs.solid_infill_speed &&
                this->infill_shift_step == rhs.infill_shift_step &&
                this->infill_rotate_step == rhs.infill_rotate_step &&
                this->lateral_lattice_angle_1		== rhs.lateral_lattice_angle_1 &&
				this->lateral_lattice_angle_2	    == rhs.lateral_lattice_angle_2 &&
				this->infill_lock_depth      ==  rhs.infill_lock_depth &&
                this->solid_skeleton_wipe_path == rhs.solid_skeleton_wipe_path &&
                this->solid_skeleton_start_corner == rhs.solid_skeleton_start_corner &&
                this->skin_infill_depth == rhs.skin_infill_depth &&
                this->infill_overhang_angle == rhs.infill_overhang_angle &&
                this->skin_pattern == rhs.skin_pattern &&
                this->skeleton_pattern == rhs.skeleton_pattern;
	}
};

struct SurfaceFill {
	SurfaceFill(const SurfaceFillParams& params) : region_id(size_t(-1)), surface(stCount, ExPolygon()), params(params) {}

	size_t 				region_id;
	Surface 			surface;
	ExPolygons       	expolygons;
	SurfaceFillParams	params;
    // BBS
    std::vector<size_t> region_id_group;
    ExPolygons          no_overlap_expolygons;
};


// Detect narrow infill regions
// Based on the anti-vibration algorithm from PrusaSlicer:
// https://github.com/prusa3d/PrusaSlicer/blob/5dc04b4e8f14f65bbcc5377d62cad3e86c2aea36/src/libslic3r/Fill/FillEnsuring.cpp#L37-L273

static coord_t _MAX_LINE_LENGTH_TO_FILTER() // 4 mm.
{
    return scaled<coord_t>(4.);
}
const constexpr size_t  MAX_SKIPS_ALLOWED = 2; // Skip means propagation through long line.
const constexpr size_t  MIN_DEPTH_FOR_LINE_REMOVING = 5;

struct LineNode
{
    struct State
    {
        // The total number of long lines visited before this node was reached.
        // We just need the minimum number of all possible paths to decide whether we can remove the line or not.
        int min_skips_taken = 0;
        // The total number of short lines visited before this node was reached.
        int total_short_lines = 0;
        // Some initial line is touching some long line. This information is propagated to neighbors.
        bool initial_touches_long_lines = false;
        bool initialized = false;

        void reset() {
            this->min_skips_taken = 0;
            this->total_short_lines = 0;
            this->initial_touches_long_lines = false;
            this->initialized = false;
        }
    };

    explicit LineNode(const Line& line) : line(line) {}

    Line                   line;
    // Pointers to line nodes in the previous and the next section that overlap with this line.
    std::vector<LineNode*> next_section_overlapping_lines;
    std::vector<LineNode*> prev_section_overlapping_lines;

    bool                   is_removed = false;

    State                  state;

    // Return true if some initial line is touching some long line and this information was propagated into the current line.
    bool is_initial_line_touching_long_lines() const {
        if (prev_section_overlapping_lines.empty())
            return false;

        for (LineNode* line_node : prev_section_overlapping_lines) {
            if (line_node->state.initial_touches_long_lines)
                return true;
        }

        return false;
    }

    // Return true if the current line overlaps with some long line in the previous section.
    bool is_touching_long_lines_in_previous_layer() const {
        if (prev_section_overlapping_lines.empty())
            return false;

        const auto MAX_LINE_LENGTH_TO_FILTER = _MAX_LINE_LENGTH_TO_FILTER();
        for (LineNode* line_node : prev_section_overlapping_lines) {
            if (!line_node->is_removed && line_node->line.length() >= MAX_LINE_LENGTH_TO_FILTER)
                return true;
        }

        return false;
    }

    // Return true if the current line overlaps with some line in the next section.
    bool has_next_layer_neighbours() const {
        if (next_section_overlapping_lines.empty())
            return false;

        for (LineNode* line_node : next_section_overlapping_lines) {
            if (!line_node->is_removed)
                return true;
        }

        return false;
    }
};

using LineNodes = std::vector<LineNode>;

inline bool are_lines_overlapping_in_y_axes(const Line& first_line, const Line& second_line) {
    return (second_line.a.y() <= first_line.a.y() && first_line.a.y() <= second_line.b.y())
        || (second_line.a.y() <= first_line.b.y() && first_line.b.y() <= second_line.b.y())
        || (first_line.a.y() <= second_line.a.y() && second_line.a.y() <= first_line.b.y())
        || (first_line.a.y() <= second_line.b.y() && second_line.b.y() <= first_line.b.y());
}

bool can_line_note_be_removed(const LineNode& line_node) {
    const auto MAX_LINE_LENGTH_TO_FILTER = _MAX_LINE_LENGTH_TO_FILTER();
    return (line_node.line.length() < MAX_LINE_LENGTH_TO_FILTER)
        && (line_node.state.total_short_lines > int(MIN_DEPTH_FOR_LINE_REMOVING)
            || (!line_node.is_initial_line_touching_long_lines() && !line_node.has_next_layer_neighbours()));
}

// Remove the node and propagate its removal to the previous sections.
void propagate_line_node_remove(const LineNode& line_node) {
    std::queue<LineNode*> line_node_queue;
    for (LineNode* prev_line : line_node.prev_section_overlapping_lines) {
        if (prev_line->is_removed)
            continue;

        line_node_queue.emplace(prev_line);
    }

    for (; !line_node_queue.empty(); line_node_queue.pop()) {
        LineNode& line_to_check = *line_node_queue.front();

        if (can_line_note_be_removed(line_to_check)) {
            line_to_check.is_removed = true;

            for (LineNode* prev_line : line_to_check.prev_section_overlapping_lines) {
                if (prev_line->is_removed)
                    continue;

                line_node_queue.emplace(prev_line);
            }
        }
    }
}

// Filter out short extrusions that could create vibrations.
static std::vector<Lines> filter_vibrating_extrusions(const std::vector<Lines>& lines_sections) {
    // Initialize all line nodes.
    std::vector<LineNodes> line_nodes_sections(lines_sections.size());
    for (const Lines& lines_section : lines_sections) {
        const size_t section_idx = &lines_section - lines_sections.data();

        line_nodes_sections[section_idx].reserve(lines_section.size());
        for (const Line& line : lines_section) {
            line_nodes_sections[section_idx].emplace_back(line);
        }
    }

    // Precalculate for each line node which line nodes in the previous and next section this line node overlaps.
    for (auto curr_lines_section_it = line_nodes_sections.begin(); curr_lines_section_it != line_nodes_sections.end(); ++curr_lines_section_it) {
        if (curr_lines_section_it != line_nodes_sections.begin()) {
            const auto prev_lines_section_it = std::prev(curr_lines_section_it);
            for (LineNode& curr_line : *curr_lines_section_it) {
                for (LineNode& prev_line : *prev_lines_section_it) {
                    if (are_lines_overlapping_in_y_axes(curr_line.line, prev_line.line)) {
                        curr_line.prev_section_overlapping_lines.emplace_back(&prev_line);
                    }
                }
            }
        }

        if (std::next(curr_lines_section_it) != line_nodes_sections.end()) {
            const auto next_lines_section_it = std::next(curr_lines_section_it);
            for (LineNode& curr_line : *curr_lines_section_it) {
                for (LineNode& next_line : *next_lines_section_it) {
                    if (are_lines_overlapping_in_y_axes(curr_line.line, next_line.line)) {
                        curr_line.next_section_overlapping_lines.emplace_back(&next_line);
                    }
                }
            }
        }
    }

    const auto MAX_LINE_LENGTH_TO_FILTER = _MAX_LINE_LENGTH_TO_FILTER();
    // Select each section as the initial lines section and propagate line node states from this initial lines section to the last lines section.
    // During this propagation, we remove those lines that meet the conditions for its removal.
    // When some line is removed, we propagate this removal to previous layers.
    for (size_t initial_line_section_idx = 0; initial_line_section_idx < line_nodes_sections.size(); ++initial_line_section_idx) {
        // Stars from non-removed short lines.
        for (LineNode& initial_line : line_nodes_sections[initial_line_section_idx]) {
            if (initial_line.is_removed || initial_line.line.length() >= MAX_LINE_LENGTH_TO_FILTER)
                continue;

            initial_line.state.reset();
            initial_line.state.total_short_lines = 1;
            initial_line.state.initial_touches_long_lines = initial_line.is_touching_long_lines_in_previous_layer();
            initial_line.state.initialized = true;
        }

        // Iterate from the initial lines section until the last lines section.
        for (size_t propagation_line_section_idx = initial_line_section_idx; propagation_line_section_idx < line_nodes_sections.size(); ++propagation_line_section_idx) {
            // Before we propagate node states into next lines sections, we reset the state of all line nodes in the next line section.
            if (propagation_line_section_idx + 1 < line_nodes_sections.size()) {
                for (LineNode& propagation_line : line_nodes_sections[propagation_line_section_idx + 1]) {
                    propagation_line.state.reset();
                }
            }

            for (LineNode& propagation_line : line_nodes_sections[propagation_line_section_idx]) {
                if (propagation_line.is_removed || !propagation_line.state.initialized)
                    continue;

                for (LineNode* neighbour_line : propagation_line.next_section_overlapping_lines) {
                    if (neighbour_line->is_removed)
                        continue;

                    const bool is_short_line = neighbour_line->line.length() < MAX_LINE_LENGTH_TO_FILTER;
                    const bool is_skip_allowed = propagation_line.state.min_skips_taken < int(MAX_SKIPS_ALLOWED);

                    if (!is_short_line && !is_skip_allowed)
                        continue;

                    const int neighbour_total_short_lines = propagation_line.state.total_short_lines + int(is_short_line);
                    const int neighbour_min_skips_taken = propagation_line.state.min_skips_taken + int(!is_short_line);

                    if (neighbour_line->state.initialized) {
                        // When the state of the node was previously filled, then we need to update data in such a way
                        // that will maximize the possibility of removing this node.
                        neighbour_line->state.min_skips_taken = std::max(neighbour_line->state.min_skips_taken, neighbour_total_short_lines);
                        neighbour_line->state.min_skips_taken = std::min(neighbour_line->state.min_skips_taken, neighbour_min_skips_taken);

                        // We will keep updating neighbor initial_touches_long_lines until it is equal to false.
                        if (neighbour_line->state.initial_touches_long_lines) {
                            neighbour_line->state.initial_touches_long_lines = propagation_line.state.initial_touches_long_lines;
                        }
                    }
                    else {
                        neighbour_line->state.total_short_lines = neighbour_total_short_lines;
                        neighbour_line->state.min_skips_taken = neighbour_min_skips_taken;
                        neighbour_line->state.initial_touches_long_lines = propagation_line.state.initial_touches_long_lines;
                        neighbour_line->state.initialized = true;
                    }
                }

                if (can_line_note_be_removed(propagation_line)) {
                    // Remove the current node and propagate its removal to the previous sections.
                    propagation_line.is_removed = true;
                    propagate_line_node_remove(propagation_line);
                }
            }
        }
    }

    // Create lines sections without filtered-out lines.
    std::vector<Lines> lines_sections_out(line_nodes_sections.size());
    for (const std::vector<LineNode>& line_nodes_section : line_nodes_sections) {
        const size_t section_idx = &line_nodes_section - line_nodes_sections.data();

        for (const LineNode& line_node : line_nodes_section) {
            if (!line_node.is_removed) {
                lines_sections_out[section_idx].emplace_back(line_node.line);
            }
        }
    }

    return lines_sections_out;
}

void split_solid_surface(size_t layer_id, const SurfaceFill &fill, ExPolygons &normal_infill, ExPolygons &narrow_infill)
{
    assert(fill.surface.surface_type == stInternalSolid);

    switch (fill.params.pattern) {
    case ipRectilinear:
    case ipMonotonic:
    case ipMonotonicLine:
    case ipAlignedRectilinear:
        // Only support straight line based infill
        break;

    default:
        // For all other types, don't split
        return;
    }

    Polygons normal_fill_areas;  // Areas that filled with normal infill

    constexpr double connect_extrusions = true;

    const coord_t scaled_spacing = scaled<coord_t>(fill.params.spacing);
    double        distance_limit_reconnection = 2.0 * double(scaled_spacing);
    double        squared_distance_limit_reconnection = distance_limit_reconnection * distance_limit_reconnection;
    // Calculate infill direction, see Fill::_infill_direction
    double        base_angle = fill.params.angle + float(M_PI / 2.);
    // For pattern other than ipAlignedRectilinear, the angle are alternated
    if (fill.params.pattern != ipAlignedRectilinear) {
        size_t idx = layer_id / fill.surface.thickness_layers;
        base_angle += (idx & 1) ? float(M_PI / 2.) : 0;
    }
    const double aligning_angle = -base_angle + PI;

    for (const ExPolygon& expolygon : fill.expolygons) {
        Polygons filled_area = to_polygons(expolygon);
        polygons_rotate(filled_area, aligning_angle);
        BoundingBox bb = get_extents(filled_area);

        Polygons inner_area = intersection(filled_area, opening(filled_area, 2 * scaled_spacing, 3 * scaled_spacing));

        inner_area = shrink(inner_area, scaled_spacing * 0.5 - scaled<double>(fill.params.overlap));

        AABBTreeLines::LinesDistancer<Line> area_walls{ to_lines(inner_area) };

        const size_t  n_vlines = (bb.max.x() - bb.min.x() + scaled_spacing - 1) / scaled_spacing;
        const coord_t y_min = bb.min.y();
        const coord_t y_max = bb.max.y();
        Lines         vertical_lines(n_vlines);
        for (size_t i = 0; i < n_vlines; i++) {
            coord_t x = bb.min.x() + i * double(scaled_spacing);
            vertical_lines[i].a = Point{ x, y_min };
            vertical_lines[i].b = Point{ x, y_max };
        }

        if (!vertical_lines.empty()) {
            vertical_lines.push_back(vertical_lines.back());
            vertical_lines.back().a = Point{ coord_t(bb.min.x() + n_vlines * double(scaled_spacing) + scaled_spacing * 0.5), y_min };
            vertical_lines.back().b = Point{ vertical_lines.back().a.x(), y_max };
        }

        std::vector<Lines> polygon_sections(n_vlines);

        for (size_t i = 0; i < n_vlines; i++) {
            const auto intersections = area_walls.intersections_with_line<true>(vertical_lines[i]);

            for (int intersection_idx = 0; intersection_idx < int(intersections.size()) - 1; intersection_idx++) {
                const auto& a = intersections[intersection_idx];
                const auto& b = intersections[intersection_idx + 1];
                if (area_walls.outside((a.first + b.first) / 2) < 0) {
                    if (std::abs(a.first.y() - b.first.y()) > scaled_spacing) {
                        polygon_sections[i].emplace_back(a.first, b.first);
                    }
                }
            }
        }

        polygon_sections = filter_vibrating_extrusions(polygon_sections);

        Polygons reconstructed_area{};
        // reconstruct polygon from polygon sections
        {
            struct TracedPoly
            {
                Points lows;
                Points highs;
            };

            std::vector<std::vector<Line>> polygon_sections_w_width = polygon_sections;
            for (auto& slice : polygon_sections_w_width) {
                for (Line& l : slice) {
                    l.a -= Point{ 0.0, 0.5 * scaled_spacing };
                    l.b += Point{ 0.0, 0.5 * scaled_spacing };
                }
            }

            std::vector<TracedPoly> current_traced_polys;
            for (const auto& polygon_slice : polygon_sections_w_width) {
                std::unordered_set<const Line*> used_segments;
                for (TracedPoly& traced_poly : current_traced_polys) {
                    auto candidates_begin = std::upper_bound(polygon_slice.begin(), polygon_slice.end(), traced_poly.lows.back(),
                        [](const Point& low, const Line& seg) { return seg.b.y() > low.y(); });
                    auto candidates_end = std::upper_bound(polygon_slice.begin(), polygon_slice.end(), traced_poly.highs.back(),
                        [](const Point& high, const Line& seg) { return seg.a.y() > high.y(); });

                    bool segment_added = false;
                    for (auto candidate = candidates_begin; candidate != candidates_end && !segment_added; candidate++) {
                        if (used_segments.find(&(*candidate)) != used_segments.end()) {
                            continue;
                        }
                        if (connect_extrusions && (traced_poly.lows.back() - candidates_begin->a).cast<double>().squaredNorm() <
                            squared_distance_limit_reconnection) {
                            traced_poly.lows.push_back(candidates_begin->a);
                        }
                        else {
                            traced_poly.lows.push_back(traced_poly.lows.back() + Point{ scaled_spacing / 2, coord_t(0) });
                            traced_poly.lows.push_back(candidates_begin->a - Point{ scaled_spacing / 2, 0 });
                            traced_poly.lows.push_back(candidates_begin->a);
                        }

                        if (connect_extrusions && (traced_poly.highs.back() - candidates_begin->b).cast<double>().squaredNorm() <
                            squared_distance_limit_reconnection) {
                            traced_poly.highs.push_back(candidates_begin->b);
                        }
                        else {
                            traced_poly.highs.push_back(traced_poly.highs.back() + Point{ scaled_spacing / 2, 0 });
                            traced_poly.highs.push_back(candidates_begin->b - Point{ scaled_spacing / 2, 0 });
                            traced_poly.highs.push_back(candidates_begin->b);
                        }
                        segment_added = true;
                        used_segments.insert(&(*candidates_begin));
                    }

                    if (!segment_added) {
                        // Zero or multiple overlapping segments. Resolving this is nontrivial,
                        // so we just close this polygon and maybe open several new. This will hopefully happen much less often
                        traced_poly.lows.push_back(traced_poly.lows.back() + Point{ scaled_spacing / 2, 0 });
                        traced_poly.highs.push_back(traced_poly.highs.back() + Point{ scaled_spacing / 2, 0 });
                        Polygon& new_poly = reconstructed_area.emplace_back(std::move(traced_poly.lows));
                        new_poly.points.insert(new_poly.points.end(), traced_poly.highs.rbegin(), traced_poly.highs.rend());
                        traced_poly.lows.clear();
                        traced_poly.highs.clear();
                    }
                }

                current_traced_polys.erase(std::remove_if(current_traced_polys.begin(), current_traced_polys.end(),
                    [](const TracedPoly& tp) { return tp.lows.empty(); }),
                    current_traced_polys.end());

                for (const auto& segment : polygon_slice) {
                    if (used_segments.find(&segment) == used_segments.end()) {
                        TracedPoly& new_tp = current_traced_polys.emplace_back();
                        new_tp.lows.push_back(segment.a - Point{ scaled_spacing / 2, 0 });
                        new_tp.lows.push_back(segment.a);
                        new_tp.highs.push_back(segment.b - Point{ scaled_spacing / 2, 0 });
                        new_tp.highs.push_back(segment.b);
                    }
                }
            }

            // add not closed polys
            for (TracedPoly& traced_poly : current_traced_polys) {
                Polygon& new_poly = reconstructed_area.emplace_back(std::move(traced_poly.lows));
                new_poly.points.insert(new_poly.points.end(), traced_poly.highs.rbegin(), traced_poly.highs.rend());
            }
        }

        polygons_append(normal_fill_areas, reconstructed_area);
    }

    polygons_rotate(normal_fill_areas, -aligning_angle);

    // Do the split
    ExPolygons normal_fill_areas_ex = union_safety_offset_ex(normal_fill_areas);
    ExPolygons narrow_fill_areas = diff_ex(fill.expolygons, normal_fill_areas_ex);

    // Merge very small areas that is smaller than a single line width to the normal infill if they touches
    for (auto iter = narrow_fill_areas.begin(); iter != narrow_fill_areas.end();) {
        auto shrinked_expoly = offset_ex(*iter, -scaled_spacing * 0.5);
        if (shrinked_expoly.empty()) {
            // Too small! Check if it touches any normal infills
            auto     expanede_exploy = offset_ex(*iter, scaled_spacing * 0.3);
            Polygons normal_fill_area_clipped = ClipperUtils::clip_clipper_polygons_with_subject_bbox(normal_fill_areas_ex, get_extents(expanede_exploy));
            auto     touch_check = intersection_ex(normal_fill_area_clipped, expanede_exploy);
            if (!touch_check.empty()) {
                normal_fill_areas_ex.emplace_back(*iter);
                iter = narrow_fill_areas.erase(iter);
                continue;
            }
        }
        iter++;
    }

    if (narrow_fill_areas.empty()) {
        // No split needed
        return;
    }

    // Expand the normal infills a little bit to avoid gaps between normal and narrow infills
    normal_infill = intersection_ex(offset_ex(normal_fill_areas_ex, scaled_spacing * 0.1), fill.expolygons);
    narrow_infill = narrow_fill_areas;

#ifdef DEBUG_SURFACE_SPLIT
    {
        BoundingBox bbox = get_extents(fill.expolygons);
        bbox.offset(scale_(1.));
        ::Slic3r::SVG svg(debug_out_path("surface_split_%d.svg", layer_id), bbox);
        svg.draw(to_lines(fill.expolygons), "red", scale_(0.1));
        svg.draw(normal_infill, "blue", 0.5);
        svg.draw(narrow_infill, "green", 0.5);
        svg.Close();
    }
#endif
}

struct LockedZagSimulationOverride
{
    float skeleton_density_percent = 0.f;
    bool  include_solid_as_sparse   = false;
};

static float layer_filament_wipe_skin_depth_mm(const LayerRegion& layer_region)
{
    const PrintRegionConfig& region_config = layer_region.region().config();
    const float configured_depth = static_cast<float>(region_config.skin_infill_depth);
    if (configured_depth > EPSILON)
        return configured_depth;

    const Layer* layer = layer_region.layer();
    if (layer == nullptr || layer->object() == nullptr || layer->object()->print() == nullptr)
        return 0.f;

    const PrintConfig& print_config = layer->object()->print()->config();
    if (print_config.nozzle_diameter.values.empty())
        return 0.f;

    const unsigned int extruder = layer_region.region().extruder(frInfill);
    const size_t nozzle_idx = std::min<size_t>(get_physical_nozzle_index(print_config, extruder > 0 ? extruder - 1 : 0),
                                               print_config.nozzle_diameter.values.size() - 1);
    return float(2. * print_config.nozzle_diameter.get_at(nozzle_idx));
}

std::vector<SurfaceFill> group_fills(const Layer &layer, LockRegionParam &lock_param,
                                     const LockedZagSimulationOverride* simulation = nullptr)
{
	std::vector<SurfaceFill> surface_fills;

	// Fill in a map of a region & surface to SurfaceFillParams.
	std::set<SurfaceFillParams> 						set_surface_params;
	std::vector<std::vector<const SurfaceFillParams*>> 	region_to_surface_params(layer.regions().size(), std::vector<const SurfaceFillParams*>());
    SurfaceFillParams									params;
    bool 												has_internal_voids = false;
	const PrintObjectConfig&							object_config = layer.object()->config();
    const bool zaa_top_fill_direction_lock_active =
        object_config.zaa_lock_top_surface_fill_direction && layer.object()->zaa_layer_uses_offset_plane(layer);
    const size_t solid_skeleton_protected_layers = 2;
    const bool solid_skeleton_protected_layer = layer.id() < solid_skeleton_protected_layers;
    const bool solid_skeleton_mode = layer.object()->print()->has_prime_volume_solid_skeleton() && !solid_skeleton_protected_layer;
    const bool skeleton_packing_mode = flush_into_skeleton_packing_mode_enabled(*layer.object()->print());
    const bool force_locked_zag_mode = solid_skeleton_mode || skeleton_packing_mode;
    const size_t solid_skeleton_start_corner = solid_skeleton_mode ? ((layer.id() >= solid_skeleton_protected_layers ? layer.id() - solid_skeleton_protected_layers : 0) % 4) : 0;

	auto append_flow_param = [](std::map<Flow, ExPolygons> &flow_params, Flow flow, const ExPolygon &exp) {
        auto it = flow_params.find(flow);
        if (it == flow_params.end())
            flow_params.insert({flow, {exp}});
        else
            it->second.push_back(exp);
        it++;
    };

	auto append_density_param = [](std::map<float, ExPolygons> &density_params, float density, const ExPolygon &exp) {
        auto it = density_params.find(density);
        if (it == density_params.end())
            density_params.insert({density, {exp}});
        else
            it->second.push_back(exp);
        it++;
    };

	for (size_t region_id = 0; region_id < layer.regions().size(); ++ region_id) {
		const LayerRegion  &layerm = *layer.regions()[region_id];
		region_to_surface_params[region_id].assign(layerm.fill_surfaces.size(), nullptr);
	    for (const Surface &surface : layerm.fill_surfaces.surfaces)
	        if (surface.surface_type == stInternalVoid)
	        	has_internal_voids = true;
	        else {
		        const PrintRegionConfig &region_config = layerm.region().config();
                const bool simulate_solid_as_sparse = simulation != nullptr &&
                    simulation->include_solid_as_sparse && surface.is_solid();
                const bool simulate_wipe_skeleton = simulation != nullptr &&
                    (surface.surface_type == stInternal || simulate_solid_as_sparse);
		        FlowRole extrusion_role = simulate_solid_as_sparse ? frInfill :
                    (surface.is_top() ? frTopSolidInfill : (surface.is_solid() ? frSolidInfill : frInfill));
		        bool     is_bridge 	    = !simulate_solid_as_sparse && layer.id() > 0 && surface.is_bridge();
		        params.extruder 	 = layerm.region().extruder(extrusion_role);
                const bool no_wipe_tower_plain_infill = !simulate_wipe_skeleton && skeleton_packing_mode &&
                    surface.surface_type == stInternal &&
                    layer.object()->print()->layer_filament_wipe_packing_force_plain_infill(layerm, surface);
                const InfillPattern sparse_pattern = no_wipe_tower_plain_infill ?
                    no_wipe_tower_plain_sparse_infill_pattern(region_config) : region_config.sparse_infill_pattern.value;
                params.pattern    = simulate_wipe_skeleton ? ipLockedZag :
                                    ((force_locked_zag_mode && !no_wipe_tower_plain_infill) ? ipLockedZag : sparse_pattern);
		        params.density       = simulate_wipe_skeleton ?
                    std::max(float(region_config.sparse_infill_density.value), simulation->skeleton_density_percent) :
                    sparse_infill_density_for_surface(layerm, surface);
                params.multiline     = int(region_config.fill_multiline);
                params.lateral_lattice_angle_1 = region_config.lateral_lattice_angle_1;
                params.lateral_lattice_angle_2 = region_config.lateral_lattice_angle_2;
                params.infill_overhang_angle = region_config.infill_overhang_angle;
                params.angle        = 0.;
                params.infill_lock_depth = 0;
                params.skin_infill_depth = 0;
                params.skin_pattern      = InfillPattern(0);
                params.skeleton_pattern  = InfillPattern(0);
                if (params.pattern == ipLockedZag) {
                    params.infill_lock_depth = force_locked_zag_mode ? 0 : scale_(region_config.infill_lock_depth);
                    params.skin_infill_depth = scale_(simulate_wipe_skeleton ?
                        layer_filament_wipe_skin_depth_mm(layerm) :
                        lockedzag_skin_depth_for_layer_region(layerm, &surface));
                    // Keep the configured skin pattern when flushing into the skeleton.
                    // Only the hidden skeleton pattern is specialized for this mode.
                    const bool flush_into_skeleton = object_config.flush_into_skeleton.value;
                    params.skin_pattern = region_config.locked_skin_infill_pattern.value;
                    params.skeleton_pattern = flush_into_skeleton
                        ? ipGrid
                        : (solid_skeleton_mode ? ipAlignedRectilinear :
                           region_config.locked_skeleton_infill_pattern.value);
                }
                if (params.pattern == ipCrossZag || params.pattern == ipLockedZag) {
                    params.infill_shift_step       = scale_(region_config.infill_shift_step);
                    params.symmetric_infill_y_axis = region_config.symmetric_infill_y_axis;
                } else if (params.pattern == ipZigZag) {
                    params.infill_rotate_step = region_config.infill_rotate_step * M_PI / 360;
                    params.symmetric_infill_y_axis = region_config.symmetric_infill_y_axis;
                }

		        if (surface.is_solid() && !simulate_solid_as_sparse) {
		            params.density = 100.f;
					//FIXME for non-thick bridges, shall we allow a bottom surface pattern?
					if (surface.is_solid_infill())
                        params.pattern = region_config.internal_solid_infill_pattern.value;
                    else if (surface.is_external() && ! is_bridge) {
                        if(surface.is_top())
                            params.pattern = region_config.top_surface_pattern.value;
                        else
                            params.pattern = region_config.bottom_surface_pattern.value;
                    }
                    else {
                        if(region_config.top_surface_pattern == ipMonotonic || region_config.top_surface_pattern == ipMonotonicLine)
                            params.pattern = ipMonotonic;
                        else
                            params.pattern = ipRectilinear;
                    }
		        } else if (params.density <= 0)
		            continue;


                if (params.pattern != ipLockedZag) {
                    params.infill_lock_depth = 0;
                    params.skin_infill_depth = 0;
                    params.skin_pattern      = InfillPattern(0);
                    params.skeleton_pattern  = InfillPattern(0);
                }
				params.extrusion_role = erInternalInfill;
                if (is_bridge) {
                    if (surface.is_internal_bridge())
                        params.extrusion_role = erInternalBridgeInfill;
                    else
                        params.extrusion_role = erBridgeInfill;
                } else if (surface.is_solid() && !simulate_solid_as_sparse) {
                    if (surface.is_top()) {
                        params.extrusion_role = erTopSolidInfill;
                    } else if (surface.is_bottom()) {
                        params.extrusion_role = erBottomSurface;
                    } else {
                        params.extrusion_role = erSolidInfill;
                    }
                }
                params.bridge_angle = float(surface.bridge_angle);
                params.solid_skeleton_wipe_path =
                    solid_skeleton_mode && params.pattern == ipLockedZag && params.extrusion_role == erInternalInfill;
                params.solid_skeleton_start_corner =
                    params.solid_skeleton_wipe_path ? solid_skeleton_start_corner : 0;
                
                // if (region_config.align_infill_direction_to_model) {
                //    auto m = layer.object()->trafo().matrix();
                //    params.angle += atan2((float) m(1, 0), (float) m(0, 0));
                //}

                if (params.extrusion_role == erInternalInfill) {
                    params.angle        = float(Geometry::deg2rad(region_config.infill_direction.value));
                    params.rotate_angle = (params.pattern == ipRectilinear || params.pattern == ipLine || params.pattern == ipCrossZag ||
                                           params.pattern == ipZigZag);
                } else {
                    params.angle        = float(Geometry::deg2rad(region_config.solid_infill_direction.value));
                    params.rotate_angle = region_config.rotate_solid_infill_direction;
                }

                if (zaa_locks_top_fill_rotation(zaa_top_fill_direction_lock_active, surface.is_top(), is_bridge))
                    params.rotate_angle = false;

                // Calculate the actual flow we'll be using for this infill.
		        params.bridge = is_bridge || Fill::use_bridge_flow(params.pattern);
                const bool is_thick_bridge = surface.is_bridge() && (surface.is_internal_bridge() ? object_config.thick_internal_bridges : object_config.thick_bridges);
				params.flow   = params.bridge ?
					//Orca: enable thick bridge based on config
					layerm.bridging_flow(extrusion_role, is_thick_bridge) :
					layerm.flow(extrusion_role, (surface.thickness == -1) ? layer.height : surface.thickness);
				// record speed params
                if (!params.bridge) {
                    const size_t nozzle_idx = get_physical_nozzle_index(layer.object()->print()->config(), params.extruder);
                    if (params.extrusion_role == erInternalInfill)
                        params.sparse_infill_speed = region_config.sparse_infill_speed.get_at(nozzle_idx);
                    else if (params.extrusion_role == erTopSolidInfill)
                        params.top_surface_speed = region_config.top_surface_speed.get_at(nozzle_idx);
                    else if (params.extrusion_role == erSolidInfill)
                        params.solid_infill_speed = region_config.internal_solid_infill_speed.get_at(nozzle_idx);
                }
				// Calculate flow spacing for infill pattern generation.
		        if ((surface.is_solid() && !simulate_solid_as_sparse) || is_bridge) {
		            params.spacing = params.flow.spacing();
		            // Don't limit anchor length for solid or bridging infill.
		            params.anchor_length = 1000.f;
					params.anchor_length_max = 1000.f;
		        } else {
					// Internal infill. Calculating infill line spacing independent of the current layer height and 1st layer status,
					// so that internall infill will be aligned over all layers of the current region.
		            params.spacing = layerm.region().flow(*layer.object(), frInfill, layer.object()->config().layer_height, false).spacing();
		            // Anchor a sparse infill to inner perimeters with the following anchor length:
			        params.anchor_length = float(region_config.infill_anchor);
					if (region_config.infill_anchor.percent)
						params.anchor_length = float(params.anchor_length * 0.01 * params.spacing);
					params.anchor_length_max = float(region_config.infill_anchor_max);
					if (region_config.infill_anchor_max.percent)
						params.anchor_length_max = float(params.anchor_length_max * 0.01 * params.spacing);
					params.anchor_length = std::min(params.anchor_length, params.anchor_length_max);
				}

				//get locked region param
				if (params.pattern == ipLockedZag){
					const PrintObject *object = layerm.layer()->object();
                    Flow skin_flow = params.bridge ? params.flow : resolve_infill_detail_flow_width(*object, layerm.region(), extrusion_role, true).flow(float((surface.thickness == -1) ? layer.height : surface.thickness));
					//add skin flow
					append_flow_param(lock_param.skin_flow_params, skin_flow, surface.expolygon);

					Flow skeleton_flow = params.bridge ? params.flow : resolve_infill_detail_flow_width(*object, layerm.region(), extrusion_role, false).flow(float((surface.thickness == -1) ? layer.height : surface.thickness));
					// add skeleton flow
					append_flow_param(lock_param.skeleton_flow_params, skeleton_flow, surface.expolygon);

                    // A purge skeleton is dimensioned from purge volume and must be
					// solid. Normal LockedZag keeps its configured skeleton density.
					append_density_param(lock_param.skeleton_density_params,
                        solid_skeleton_mode ? 1.f : float(0.01 * region_config.skeleton_infill_density),
                        surface.expolygon);

                    append_density_param(lock_param.skin_density_params,
                                         float(0.01 * region_config.skin_infill_density),
                                         surface.expolygon);

                }

                auto it_params = set_surface_params.find(params);

		        if (it_params == set_surface_params.end())
		        	it_params = set_surface_params.insert(it_params, params);
		        region_to_surface_params[region_id][&surface - &layerm.fill_surfaces.surfaces.front()] = &(*it_params);
		    }
	}

	surface_fills.reserve(set_surface_params.size());
	for (const SurfaceFillParams &params : set_surface_params) {
		const_cast<SurfaceFillParams&>(params).idx = surface_fills.size();
		surface_fills.emplace_back(params);
	}

	for (size_t region_id = 0; region_id < layer.regions().size(); ++ region_id) {
		const LayerRegion &layerm = *layer.regions()[region_id];
	    for (const Surface &surface : layerm.fill_surfaces.surfaces)
	        if (surface.surface_type != stInternalVoid) {
	        	const SurfaceFillParams *params = region_to_surface_params[region_id][&surface - &layerm.fill_surfaces.surfaces.front()];
				if (params != nullptr) {
	        		SurfaceFill &fill = surface_fills[params->idx];
                    if (fill.region_id == size_t(-1)) {
	        			fill.region_id = region_id;
	        			fill.surface = surface;
	        			fill.expolygons.emplace_back(std::move(fill.surface.expolygon));
						//BBS
						fill.region_id_group.push_back(region_id);
						fill.no_overlap_expolygons = layerm.fill_no_overlap_expolygons;
					} else {
						fill.expolygons.emplace_back(surface.expolygon);
						//BBS
						auto t = find(fill.region_id_group.begin(), fill.region_id_group.end(), region_id);
						if (t == fill.region_id_group.end()) {
							fill.region_id_group.push_back(region_id);
							fill.no_overlap_expolygons = union_ex(fill.no_overlap_expolygons, layerm.fill_no_overlap_expolygons);
						}
					}
				}
	        }
	}

    {
		Polygons all_polygons;
		for (SurfaceFill &fill : surface_fills)
			if (! fill.expolygons.empty()) {
				if (fill.expolygons.size() > 1 || ! all_polygons.empty()) {
					Polygons polys = to_polygons(std::move(fill.expolygons));
		            // Make a union of polygons, use a safety offset, subtract the preceding polygons.
				    // Bridges are processed first (see SurfaceFill::operator<())
		            fill.expolygons = all_polygons.empty() ? union_safety_offset_ex(polys) : diff_ex(polys, all_polygons, ApplySafetyOffset::Yes);
					append(all_polygons, std::move(polys));
				} else if (&fill != &surface_fills.back())
					append(all_polygons, to_polygons(fill.expolygons));
	        }
	}

    // we need to detect any narrow surfaces that might collapse
    // when adding spacing below
    // such narrow surfaces are often generated in sloping walls
    // by bridge_over_infill() and combine_infill() as a result of the
    // subtraction of the combinable area from the layer infill area,
    // which leaves small areas near the perimeters
    // we are going to grow such regions by overlapping them with the void (if any)
    // TODO: detect and investigate whether there could be narrow regions without
    // any void neighbors
    if (has_internal_voids) {
    	// Internal voids are generated only if "infill_only_where_needed" or "infill_every_layers" are active.
        coord_t  distance_between_surfaces = 0;
        Polygons surfaces_polygons;
        Polygons voids;
		int      region_internal_infill = -1;
		int		 region_solid_infill = -1;
		int		 region_some_infill = -1;
    	for (SurfaceFill &surface_fill : surface_fills)
			if (! surface_fill.expolygons.empty()) {
    			distance_between_surfaces = std::max(distance_between_surfaces, surface_fill.params.flow.scaled_spacing());
				append((surface_fill.surface.surface_type == stInternalVoid) ? voids : surfaces_polygons, to_polygons(surface_fill.expolygons));
				if (surface_fill.surface.surface_type == stInternalSolid)
					region_internal_infill = (int)surface_fill.region_id;
				if (surface_fill.surface.is_solid())
					region_solid_infill = (int)surface_fill.region_id;
				if (surface_fill.surface.surface_type != stInternalVoid)
					region_some_infill = (int)surface_fill.region_id;
			}
    	if (! voids.empty() && ! surfaces_polygons.empty()) {
    		// First clip voids by the printing polygons, as the voids were ignored by the loop above during mutual clipping.
    		voids = diff(voids, surfaces_polygons);
	        // Corners of infill regions, which would not be filled with an extrusion path with a radius of distance_between_surfaces/2
	        Polygons collapsed = diff(
	            surfaces_polygons,
				opening(surfaces_polygons, float(distance_between_surfaces /2), float(distance_between_surfaces / 2 + ClipperSafetyOffset)));
	        //FIXME why the voids are added to collapsed here? First it is expensive, second the result may lead to some unwanted regions being
	        // added if two offsetted void regions merge.
	        // polygons_append(voids, collapsed);
	        ExPolygons extensions = intersection_ex(expand(collapsed, float(distance_between_surfaces)), voids, ApplySafetyOffset::Yes);
	        // Now find an internal infill SurfaceFill to add these extrusions to.
	        SurfaceFill *internal_solid_fill = nullptr;
			unsigned int region_id = 0;
			if (region_internal_infill != -1)
				region_id = region_internal_infill;
			else if (region_solid_infill != -1)
				region_id = region_solid_infill;
			else if (region_some_infill != -1)
				region_id = region_some_infill;
			const LayerRegion& layerm = *layer.regions()[region_id];
	        for (SurfaceFill &surface_fill : surface_fills)
	        	if (surface_fill.surface.surface_type == stInternalSolid && std::abs(layer.height - surface_fill.params.flow.height()) < EPSILON) {
	        		internal_solid_fill = &surface_fill;
	        		break;
	        	}
	        if (internal_solid_fill == nullptr) {
	        	// Produce another solid fill.
		        params.extruder 	 = layerm.region().extruder(frSolidInfill);
                const auto top_pattern = layerm.region().config().top_surface_pattern;
                if(top_pattern == ipMonotonic || top_pattern == ipMonotonicLine)
                    params.pattern = top_pattern;
                else
                    params.pattern 		 = ipRectilinear;
	            params.density 		 = 100.f;
		        params.extrusion_role = erSolidInfill;
		        params.angle 		= float(Geometry::deg2rad(layerm.region().config().solid_infill_direction.value));
                params.rotate_angle   = layerm.region().config().rotate_solid_infill_direction;
		        // calculate the actual flow we'll be using for this infill
				params.flow = layerm.flow(frSolidInfill);
		        params.spacing = params.flow.spacing();
				surface_fills.emplace_back(params);
				surface_fills.back().surface.surface_type = stInternalSolid;
				surface_fills.back().surface.thickness = layer.height;
				surface_fills.back().expolygons = std::move(extensions);
	        } else {
	        	append(extensions, std::move(internal_solid_fill->expolygons));
	        	internal_solid_fill->expolygons = union_ex(extensions);
	        }
		}
    }

	// BBS: detect narrow internal solid infill area and use ipConcentricInternal pattern instead
	if (layer.object()->config().detect_narrow_internal_solid_infill) {
		size_t surface_fills_size = surface_fills.size();
		for (size_t i = 0; i < surface_fills_size; i++) {
			if (surface_fills[i].surface.surface_type != stInternalSolid)
				continue;

			ExPolygons normal_infill;
            ExPolygons narrow_infill;
            split_solid_surface(layer.id(), surface_fills[i], normal_infill, narrow_infill);

			if (narrow_infill.empty()) {
				// BBS: has no narrow expolygon
				continue;
			} else if (normal_infill.empty()) {
				// BBS: all expolygons are narrow, directly change the fill pattern
				surface_fills[i].params.pattern = ipConcentricInternal;
			}
			else {
				// BBS: some expolygons are narrow, spilit surface_fills[i] and rearrange the expolygons
				params = surface_fills[i].params;
				params.pattern = ipConcentricInternal;
				surface_fills.emplace_back(params);
				surface_fills.back().region_id = surface_fills[i].region_id;
				surface_fills.back().surface.surface_type = stInternalSolid;
				surface_fills.back().surface.thickness = surface_fills[i].surface.thickness;
                surface_fills.back().region_id_group       = surface_fills[i].region_id_group;
                surface_fills.back().no_overlap_expolygons = surface_fills[i].no_overlap_expolygons;
			    // BBS: move the narrow expolygons to new surface_fills.back();
			    surface_fills.back().expolygons = std::move(narrow_infill);
			    // BBS: delete the narrow expolygons from old surface_fills
                surface_fills[i].expolygons = std::move(normal_infill);
			}
		}
	}

	return surface_fills;
}

namespace {

struct LockedZagCachedFlowRegion
{
    Flow       flow;
    ExPolygons expolygons;
};

struct LockedZagCachedSkeletonPart
{
    Surface                              surface;
    ExPolygons                           skeleton_expolygons;
    ExPolygons                           no_overlap_expolygons;
    std::vector<LockedZagCachedFlowRegion> flow_regions;
};

struct LockedZagCachedSurface
{
    const LayerRegion*                    layer_region = nullptr;
    SurfaceFillParams                     params;
    std::vector<LockedZagCachedSkeletonPart> parts;
};

struct LockedZagCachedDensityRegion
{
    float      density = 0.f;
    ExPolygons expolygons;
};

void add_lockedzag_skeleton_metrics(const Polylines& polylines, const Flow& flow,
                                    LockedZagSkeletonMetrics& metrics)
{
    const double flow_mm3_per_mm = flow.mm3_per_mm();
    for (const Polyline& polyline : polylines) {
        const double length_mm = unscale<double>(polyline.length());
        metrics.trajectory_length_mm += length_mm;
        metrics.extrusion_volume_mm3 += length_mm * flow_mm3_per_mm;
    }
}

} // namespace

struct LockedZagSkeletonSimulation::Impl
{
    const Layer* layer = nullptr;
    BoundingBox bbox;
    coord_t     bbox_height = 0;
    double      resolution = 0.;
    std::vector<LockedZagCachedDensityRegion> density_regions;
    std::vector<LockedZagCachedSurface>        surfaces;
};

LockedZagSkeletonSimulation::LockedZagSkeletonSimulation(const Layer& layer, bool include_solid_as_sparse)
    : m_impl(new Impl)
{
    m_impl->layer       = &layer;
    m_impl->bbox        = layer.object()->bounding_box();
    m_impl->bbox_height = layer.object()->height();
    m_impl->resolution  = layer.object()->print()->config().resolution.value;

    // Keep the configured density while building the geometry cache. Candidate
    // densities change line spacing only; the clipped layer regions do not.
    LockedZagSimulationOverride simulation;
    simulation.skeleton_density_percent = 0.f;
    simulation.include_solid_as_sparse   = include_solid_as_sparse;
    LockRegionParam lock_param;
    std::vector<SurfaceFill> surface_fills = group_fills(layer, lock_param, &simulation);

    m_impl->density_regions.reserve(lock_param.skeleton_density_params.size());
    for (const auto& density_entry : lock_param.skeleton_density_params) {
        LockedZagCachedDensityRegion cached_region;
        cached_region.density    = density_entry.first;
        cached_region.expolygons = union_safety_offset_ex(density_entry.second);
        if (!cached_region.expolygons.empty())
            m_impl->density_regions.emplace_back(std::move(cached_region));
    }

    std::vector<LockedZagCachedFlowRegion> flow_regions;
    flow_regions.reserve(lock_param.skeleton_flow_params.size());
    for (const auto& flow_entry : lock_param.skeleton_flow_params) {
        LockedZagCachedFlowRegion cached_region;
        cached_region.flow       = flow_entry.first;
        cached_region.expolygons = union_safety_offset_ex(flow_entry.second);
        if (!cached_region.expolygons.empty())
            flow_regions.emplace_back(std::move(cached_region));
    }

    m_impl->surfaces.reserve(surface_fills.size());
    for (const SurfaceFill& surface_fill : surface_fills) {
        if (surface_fill.params.pattern != ipLockedZag ||
            surface_fill.params.extrusion_role != erInternalInfill ||
            surface_fill.region_id >= layer.regions().size())
            continue;

        LockedZagCachedSurface cached_surface;
        cached_surface.layer_region = layer.regions()[surface_fill.region_id];
        cached_surface.params       = surface_fill.params;
        cached_surface.parts.reserve(surface_fill.expolygons.size());

        for (const ExPolygon& expoly : surface_fill.expolygons) {
            ExPolygons skeleton_expolygons = offset_ex(
                ExPolygons{expoly}, -double(surface_fill.params.skin_infill_depth));

            // Retain the one-time safety-expanded density clipping used by
            // FillLockedZag. It is independent of the candidate line spacing.
            const float density_key = float(0.01 * surface_fill.params.density);
            for (const LockedZagCachedDensityRegion& density_region : m_impl->density_regions) {
                if (std::abs(density_region.density - density_key) <= EPSILON) {
                    skeleton_expolygons = intersection_ex(
                        density_region.expolygons, skeleton_expolygons);
                    break;
                }
            }

            skeleton_expolygons = intersection_ex(
                offset_ex(skeleton_expolygons, double(surface_fill.params.infill_lock_depth)),
                ExPolygons{expoly});
            if (skeleton_expolygons.empty())
                continue;

            LockedZagCachedSkeletonPart cached_part;
            cached_part.surface = surface_fill.surface;
            cached_part.surface.surface_type = stInternal;
            cached_part.surface.expolygon = expoly;
            cached_part.skeleton_expolygons = std::move(skeleton_expolygons);
            cached_part.no_overlap_expolygons = intersection_ex(
                surface_fill.no_overlap_expolygons,
                ExPolygons{expoly}, ApplySafetyOffset::Yes);

            for (const LockedZagCachedFlowRegion& flow_region : flow_regions) {
                ExPolygons clipped_region = intersection_ex(
                    flow_region.expolygons, cached_part.skeleton_expolygons);
                if (clipped_region.empty())
                    continue;
                cached_part.flow_regions.push_back(
                    LockedZagCachedFlowRegion{flow_region.flow, std::move(clipped_region)});
            }

            cached_surface.parts.emplace_back(std::move(cached_part));
        }

        if (!cached_surface.parts.empty())
            m_impl->surfaces.emplace_back(std::move(cached_surface));
    }
}

LockedZagSkeletonSimulation::~LockedZagSkeletonSimulation() = default;

LockedZagSkeletonMetricsByRegion LockedZagSkeletonSimulation::simulate(
    float skeleton_density_percent) const
{
    LockedZagSkeletonMetricsByRegion result;
    if (!m_impl || m_impl->layer == nullptr)
        return result;

    const Layer& layer = *m_impl->layer;
    const float requested_density = std::clamp(skeleton_density_percent, 0.f, 100.f);
    const PrintConfig& print_config = layer.object()->print()->config();
    const PrintObjectConfig& print_object_config = layer.object()->config();

    for (const LockedZagCachedSurface& cached_surface : m_impl->surfaces) {
        const LayerRegion* layer_region = cached_surface.layer_region;
        if (layer_region == nullptr)
            continue;

        std::unique_ptr<Fill> skeleton_f(
            Fill::new_from_type(cached_surface.params.skeleton_pattern));
        if (!skeleton_f)
            continue;
        skeleton_f->set_bounding_box(m_impl->bbox);
        skeleton_f->layer_id        = layer.id();
        skeleton_f->z               = layer.print_z;
        skeleton_f->angle           = cached_surface.params.angle;
        skeleton_f->spacing         = cached_surface.params.spacing;
        skeleton_f->overlap         = 0.;
        const float effective_density = std::max(cached_surface.params.density, requested_density);
        // Match FillLockedZag's inner skeleton filler. copy_fill_data() keeps the
        // default rotation mode and copies the outer link limit, which is zero
        // when Layer::make_fills() creates the outer LockedZag filler.
        skeleton_f->link_max_length = 0;
        skeleton_f->loop_clipping   = coord_t(scale_(layer_region->region().config().seam_gap.get_abs_value(
            cached_surface.params.flow.nozzle_diameter())));
        if (cached_surface.params.solid_skeleton_wipe_path)
            // LockedZag adds another 90 degrees to the skeleton pattern.
            skeleton_f->angle = float(M_PI / 2.);

        FillParams params;
        params.density                  = float(0.01 * effective_density);
        params.multiline                = cached_surface.params.multiline;
        params.dont_adjust              = false;
        params.anchor_length            = cached_surface.params.anchor_length;
        params.anchor_length_max        = cached_surface.params.anchor_length_max;
        params.resolution               = m_impl->resolution;
        params.use_arachne              = false;
        params.layer_height             = layer_region->layer()->height;
        params.lateral_lattice_angle_1  = cached_surface.params.lateral_lattice_angle_1;
        params.lateral_lattice_angle_2  = cached_surface.params.lateral_lattice_angle_2;
        params.infill_overhang_angle    = cached_surface.params.infill_overhang_angle;
        params.flow                     = cached_surface.params.flow;
        params.extrusion_role           = erInternalInfill;
        params.using_internal_flow      = true;
        params.enable_gap_fill          = cached_surface.params.enable_gap_fill;
        params.solid_skeleton_wipe_path = false;
        params.solid_skeleton_start_corner = 0;
        params.no_extrusion_overlap     = cached_surface.params.overlap;
        params.config                   = &layer_region->region().config();
        params.pattern                  = ipLockedZag;
        params.locked_zag               = true;
        params.infill_lock_depth        = cached_surface.params.infill_lock_depth;
        params.skin_infill_depth        = cached_surface.params.skin_infill_depth;

        if (layer.id() % 2 == 0)
            params.horiz_move -= cached_surface.params.infill_shift_step * (layer.id() / 2);
        else
            params.horiz_move += cached_surface.params.infill_shift_step * (layer.id() / 2);
        if (cached_surface.params.skeleton_pattern != ipCrossZag)
            params.horiz_move = 0;
        params.symmetric_infill_y_axis = cached_surface.params.symmetric_infill_y_axis;
        const coord_t symmetric_axis = skeleton_f->extended_object_bounding_box().center().x();

        LockedZagSkeletonMetrics& metrics = result[layer_region];
        for (const LockedZagCachedSkeletonPart& cached_part : cached_surface.parts) {
            skeleton_f->no_overlap_expolygons = cached_part.no_overlap_expolygons;
            for (const ExPolygon& skeleton_expoly : cached_part.skeleton_expolygons) {
                Surface skeleton_surface = cached_part.surface;
                skeleton_surface.expolygon = skeleton_expoly;
                if (params.symmetric_infill_y_axis) {
                    params.symmetric_y_axis = symmetric_axis;
                    skeleton_surface.expolygon.symmetric_y(params.symmetric_y_axis);
                }

                // Fill may adjust spacing for a full-density request.
                skeleton_f->spacing = cached_surface.params.spacing;
                Polylines skeleton_lines;
                try {
                    skeleton_lines = skeleton_f->fill_surface(&skeleton_surface, params);
                } catch (InfillFailedException&) {
                    continue;
                }

                for (const LockedZagCachedFlowRegion& flow_region : cached_part.flow_regions) {
                    const Polylines clipped_lines = intersection_pl(
                        skeleton_lines, flow_region.expolygons);
                    add_lockedzag_skeleton_metrics(clipped_lines, flow_region.flow, metrics);
                }
            }
        }
    }

    return result;
}
LockedZagSkeletonMetricsByRegion simulate_layer_filament_wipe_locked_zag(
    const Layer& layer, float skeleton_density_percent)
{
    LockedZagSimulationOverride simulation;
    simulation.skeleton_density_percent = std::clamp(skeleton_density_percent, 0.f, 100.f);

    LockRegionParam lock_param;
    std::vector<SurfaceFill> surface_fills = group_fills(layer, lock_param, &simulation);
    const BoundingBox bbox = layer.object()->bounding_box();
    const double resolution = layer.object()->print()->config().resolution.value;
    LockedZagSkeletonMetricsByRegion result;

    for (SurfaceFill& surface_fill : surface_fills) {
        if (surface_fill.surface.surface_type != stInternal ||
            surface_fill.params.pattern != ipLockedZag ||
            surface_fill.params.extrusion_role != erInternalInfill)
            continue;

        std::unique_ptr<Fill> filler(Fill::new_from_type(ipLockedZag));
        filler->set_bounding_box(bbox);
        filler->set_bounding_box_height(layer.object()->height());
        filler->layer_id = layer.id();
        filler->z = layer.print_z;
        filler->angle = surface_fill.params.angle;
        filler->rotate_angle = surface_fill.params.rotate_angle;
        filler->print_config = &layer.object()->print()->config();
        filler->print_object_config = &layer.object()->config();

        const LayerRegion* layer_region = layer.regions()[surface_fill.region_id];
        double link_max_length = 0.;
        if (!surface_fill.params.bridge && surface_fill.params.density > 80.)
            link_max_length = 3. * filler->spacing;
        filler->link_max_length = coord_t(scale_(link_max_length));
        filler->loop_clipping = coord_t(scale_(layer_region->region().config().seam_gap.get_abs_value(
            surface_fill.params.flow.nozzle_diameter())));

        FillParams params;
        params.density = float(0.01 * surface_fill.params.density);
        params.multiline = surface_fill.params.multiline;
        params.dont_adjust = false;
        params.anchor_length = surface_fill.params.anchor_length;
        params.anchor_length_max = surface_fill.params.anchor_length_max;
        params.resolution = resolution;
        params.use_arachne = false;
        params.layer_height = layer_region->layer()->height;
        params.lateral_lattice_angle_1 = surface_fill.params.lateral_lattice_angle_1;
        params.lateral_lattice_angle_2 = surface_fill.params.lateral_lattice_angle_2;
        params.infill_overhang_angle = surface_fill.params.infill_overhang_angle;
        params.flow = surface_fill.params.flow;
        params.extrusion_role = surface_fill.params.extrusion_role;
        params.using_internal_flow = true;
        params.enable_gap_fill = surface_fill.params.enable_gap_fill;
        params.solid_skeleton_wipe_path = false;
        params.solid_skeleton_start_corner = 0;
        params.no_extrusion_overlap = surface_fill.params.overlap;
        params.config = &layer_region->region().config();
        params.pattern = ipLockedZag;
        params.locked_zag = true;
        params.infill_lock_depth = surface_fill.params.infill_lock_depth;
        params.skin_infill_depth = surface_fill.params.skin_infill_depth;
        filler->set_lock_region_param(lock_param);
        filler->set_skin_and_skeleton_pattern(surface_fill.params.skin_pattern,
                                              surface_fill.params.skeleton_pattern);

        if (layer.id() % 2 == 0)
            params.horiz_move -= surface_fill.params.infill_shift_step * (layer.id() / 2);
        else
            params.horiz_move += surface_fill.params.infill_shift_step * (layer.id() / 2);
        params.symmetric_infill_y_axis = surface_fill.params.symmetric_infill_y_axis;

        ExtrusionEntityCollection simulated_entities;
        for (ExPolygon& expoly : surface_fill.expolygons) {
            filler->no_overlap_expolygons = intersection_ex(
                surface_fill.no_overlap_expolygons, ExPolygons{expoly}, ApplySafetyOffset::Yes);
            if (params.symmetric_infill_y_axis) {
                params.symmetric_y_axis = filler->extended_object_bounding_box().center().x();
                expoly.symmetric_y(params.symmetric_y_axis);
            }

            filler->spacing = surface_fill.params.spacing;
            surface_fill.surface.expolygon = std::move(expoly);
            filler->fill_surface_extrusion(&surface_fill.surface, params, simulated_entities.entities);
        }

        LockedZagSkeletonMetrics& metrics = result[layer_region];
        for (const ExtrusionEntity* entity : simulated_entities.entities) {
            if (entity == nullptr || entity->role() != erInternalInfill)
                continue;
            Polylines skeleton_polylines;
            entity->collect_polylines(skeleton_polylines);
            for (const Polyline& polyline : skeleton_polylines)
                metrics.trajectory_length_mm += unscale<double>(polyline.length());
            metrics.extrusion_volume_mm3 += entity->total_volume();
        }
    }

    return result;
}

#ifdef SLIC3R_DEBUG_SLICE_PROCESSING
void export_group_fills_to_svg(const char *path, const std::vector<SurfaceFill> &fills)
{
    BoundingBox bbox;
    for (const auto &fill : fills)
        for (const auto &expoly : fill.expolygons)
            bbox.merge(get_extents(expoly));
    Point legend_size = export_surface_type_legend_to_svg_box_size();
    Point legend_pos(bbox.min(0), bbox.max(1));
    bbox.merge(Point(std::max(bbox.min(0) + legend_size(0), bbox.max(0)), bbox.max(1) + legend_size(1)));

    SVG svg(path, bbox);
    const float transparency = 0.5f;
    for (const auto &fill : fills)
        for (const auto &expoly : fill.expolygons)
            svg.draw(expoly, surface_type_to_color_name(fill.surface.surface_type), transparency);
    export_surface_type_legend_to_svg(svg, legend_pos);
    svg.Close();
}
#endif

// friend to Layer
void Layer::make_fills(FillAdaptive::Octree* adaptive_fill_octree, FillAdaptive::Octree* support_fill_octree, const Point offset, FillLightning::Generator* lightning_generator)
{
	for (LayerRegion *layerm : m_regions)
	{
		layerm->fills.clear();
		layerm->skeleton_flush_density_replaced = false;
	}


#ifdef SLIC3R_DEBUG_SLICE_PROCESSING
//	this->export_region_fill_surfaces_to_svg_debug("10_fill-initial");
#endif /* SLIC3R_DEBUG_SLICE_PROCESSING */
    LockRegionParam lock_param;
    std::vector<SurfaceFill>     surface_fills = group_fills(*this, lock_param);
	const Slic3r::BoundingBox bbox 			= this->object()->bounding_box();
	const auto                resolution 	= this->object()->print()->config().resolution.value;

#ifdef SLIC3R_DEBUG_SLICE_PROCESSING
	{
		static int iRun = 0;
		export_group_fills_to_svg(debug_out_path("Layer-fill_surfaces-10_fill-final-%d.svg", iRun ++).c_str(), surface_fills);
	}
#endif /* SLIC3R_DEBUG_SLICE_PROCESSING */

    for (SurfaceFill &surface_fill : surface_fills) {
        // Create the filler object.
        std::unique_ptr<Fill> f = std::unique_ptr<Fill>(Fill::new_from_type(surface_fill.params.pattern));
        f->set_bounding_box(bbox);
        f->set_bounding_box_height(this->object()->height());
        f->layer_id = this->id();
        f->z 		= this->print_z;
        f->angle 	= surface_fill.params.angle;
        f->rotate_angle      = surface_fill.params.rotate_angle;
        f->adapt_fill_octree   = (surface_fill.params.pattern == ipSupportCubic) ? support_fill_octree : adaptive_fill_octree;
        if (surface_fill.params.pattern == ipField)
            f->field_sdf_grid = this->object()->field_sdf_grid_ptr();
        if (surface_fill.params.pattern == ipZigZag) {
            if (f->layer_id % 2 == 0)
                f->angle -= surface_fill.params.infill_rotate_step * (f->layer_id / 2);
            else
                f->angle += surface_fill.params.infill_rotate_step * (f->layer_id / 2);
        }
        f->print_config        = &this->object()->print()->config();
        f->print_object_config = &this->object()->config();
        //this->object()->con

        LayerRegion* layerm = this->m_regions[surface_fill.region_id];
        //creality for zero linewidth
        const PrintConfig &print_config = this->object()->print()->config();
        const size_t nozzle_index = get_physical_nozzle_index(print_config, layerm->region().extruder(frInfill) - 1);
        const double nozzle_diameter = print_config.nozzle_diameter.get_at(nozzle_index);
        double sparse_infill_line_width = nozzle_variant_abs_value(layerm->region().config().sparse_infill_line_width, nozzle_index, nozzle_diameter);
        if(sparse_infill_line_width == 0.0)
            sparse_infill_line_width = nozzle_diameter;
        double infill_line_distance = layerm->region().config().sparse_infill_density <= 0 ? 4.6 : sparse_infill_line_width * 100.0 / layerm->region().config().sparse_infill_density;

		if (surface_fill.params.pattern == ipLightning)
            dynamic_cast<FillLightning::Filler*>(f.get())->generator = lightning_generator;
        else if (surface_fill.params.pattern == ipCross || surface_fill.params.pattern == ipCross3d)
        {
            auto boundingbox =this->object()->bounding_box();
            dynamic_cast<FillCross*>(f.get())->set_cross_fill_provider(boundingbox, offset, surface_fill.params.pattern, infill_line_distance * 1000.0f, sparse_infill_line_width * 1000.0f);
        }
        else if (surface_fill.params.pattern == ipquarter_cubic || surface_fill.params.pattern == iptetrahedral)
        {
            Vec3crd center = this->object()->size();
            Point _offset = offset;

            dynamic_cast<FillQuarter*>(f.get())->setOrigin(surface_fill.params.pattern,Point(center.x()/2.0, center.y()/2.0), _offset, slice_z *1000.0f, infill_line_distance * 1000.0f, sparse_infill_line_width * 1000.0f);
        }

        // calculate flow spacing for infill pattern generation
        bool using_internal_flow = ! surface_fill.surface.is_solid() && ! surface_fill.params.bridge;
        double link_max_length = 0.;
        if (! surface_fill.params.bridge) {
#if 0
            link_max_length = layerm.region()->config().get_abs_value(surface.is_external() ? "external_fill_link_max_length" : "fill_link_max_length", flow.spacing());
//            printf("flow spacing: %f,  is_external: %d, link_max_length: %lf\n", flow.spacing(), int(surface.is_external()), link_max_length);
#else
            if (surface_fill.params.density > 80.) // 80%
                link_max_length = 3. * f->spacing;
#endif
        }

        // Maximum length of the perimeter segment linking two infill lines.
        f->link_max_length = (coord_t)scale_(link_max_length);
        // Used by the concentric infill pattern to clip the loops to create extrusion paths.
        f->loop_clipping = coord_t(scale_(layerm->region().config().seam_gap.get_abs_value(surface_fill.params.flow.nozzle_diameter())));

        // apply half spacing using this flow's own spacing and generate infill
        FillParams params;
        params.density 		     = float(0.01 * surface_fill.params.density);
        params.multiline         = surface_fill.params.multiline;
		params.dont_adjust		 = false; //  surface_fill.params.dont_adjust;
        params.anchor_length     = surface_fill.params.anchor_length;
		params.anchor_length_max = surface_fill.params.anchor_length_max;
		params.resolution        = resolution;
        params.use_arachne       = surface_fill.params.pattern == ipConcentric || surface_fill.params.pattern == ipConcentricInternal;
        params.layer_height      = layerm->layer()->height;
        params.lateral_lattice_angle_1   = surface_fill.params.lateral_lattice_angle_1;
        params.lateral_lattice_angle_2   = surface_fill.params.lateral_lattice_angle_2;
        params.infill_overhang_angle   = surface_fill.params.infill_overhang_angle;

		// BBS
		params.flow = surface_fill.params.flow;
		params.extrusion_role = surface_fill.params.extrusion_role;
		params.using_internal_flow = using_internal_flow;
        params.enable_gap_fill = surface_fill.params.enable_gap_fill;
        params.solid_skeleton_wipe_path = surface_fill.params.solid_skeleton_wipe_path;
        params.solid_skeleton_start_corner = surface_fill.params.solid_skeleton_start_corner;
		params.no_extrusion_overlap = surface_fill.params.overlap;

        params.config       = &layerm->region().config();
        auto& region_config = layerm->region().config();
        params.pattern      = surface_fill.params.pattern;

        //ConfigOptionFloats rotate_angles;
        //const std::string  search_string = "/NnZz$LlUuQq~^|#";
        //std::string        v(params.extrusion_role == erInternalInfill ? "0,0" : region_config.solid_infill_rotate_template.value);
        //if (regex_search(v, std::regex("[+\\-%*@\'\"cmSODMR" + search_string + "]"))) { // template metalanguage of rotating infill
        //    std::regex                 del("[\\s,]+");
        //    std::sregex_token_iterator it(v.begin(), v.end(), del, -1);
        //    std::vector<std::string>   tk;
        //    std::sregex_token_iterator end;
        //    while (it != end) {
        //        tk.push_back(*it++);
        //    }
        //    int               t            = 0;
        //    int               repeats      = 0;
        //    double            angle        = 0;
        //    double            angle_add    = 0;
        //    double            angle_steps  = 1;
        //    double            angle_start  = 0;
        //    double            limit_fill_z = this->object()->get_layer(0)->bottom_z();
        //    double            start_fill_z = limit_fill_z;
        //    bool              _noop        = false;
        //    auto              solid        = std::string::npos; // -1 - sparse,  0 - native (D), 1 - internal solid (S), 2 - concentric (O), 3 - monotonic (M), 4 - rectilinear (R)           
        //    auto              fill_form    = std::string::npos;
        //    bool              _absolute    = false;
        //    bool              _negative    = false;
        //    std::vector<bool> stop(tk.size(), false);

        //    for (int i = 0; i <= this->id(); i++) {
        //        double fill_z = this->object()->get_layer(i)->bottom_z();

        //        if (limit_fill_z < this->object()->get_layer(i)->slice_z) {
        //            if (repeats) { // if repeats >0 then restore parameters for new iteration
        //                limit_fill_z += limit_fill_z - start_fill_z;
        //                start_fill_z = fill_z;
        //                repeats--;
        //            } else {
        //                start_fill_z = fill_z;
        //                limit_fill_z = this->object()->get_layer(i)->print_z;
        //                solid        = std::string::npos;
        //                fill_form    = std::string::npos;
        //                do {
        //                    if (!stop[t]) {
        //                        _noop       = false;
        //                        _absolute   = false;
        //                        _negative   = false;
        //                        angle_start += angle_add;
        //                        angle_add   = 0;
        //                        angle_steps = 1;
        //                        repeats     = 1;
        //                        if (tk[t].find('!') != std::string::npos) // this is an one-time instruction
        //                            stop[t] = true;

        //                        char* cs = &tk[t][0];

        //                        if ((cs[0] >= '0' && cs[0] <= '9') && !(cs[0] == '+' || cs[0] == '-')) // absolute/relative
        //                            _absolute = true;

        //                        angle_add = strtod(cs, &cs);        // read angle parameter

        //                        if (cs[0] == '%') {                 // percentage of angles
        //                            angle_add *= 3.6;
        //                            cs = &cs[1];
        //                        }

        //                        int tit = tk[t].find('*');
        //                        if (tit != std::string::npos)                   // overall angle_cycles
        //                            repeats = strtol(&tk[t][tit + 1], &cs, 0);

        //                        if (repeats) {                                  // run if overall cycles greater than 0
        //                            solid = std::string("DSOMR").find(cs[0]);   // solid infill
        //                            if (solid != std::string::npos) 
        //                                cs = &cs[1];

        //                            if (cs[0] == 'B') {
        //                                angle_steps = this->object()->print()->default_region_config().bottom_shell_layers.value;
        //                            } else if (cs[0] == 'T') {
        //                                angle_steps = this->object()->print()->default_region_config().top_shell_layers.value;
        //                            } else {
        //                                fill_form = search_string.find(cs[0]);
        //                                if (fill_form != std::string::npos)
        //                                    cs = &cs[1];

        //                                _negative = (cs[0] == '-'); // negative parameter
        //                                angle_steps = abs(strtod(cs, &cs));

        //                                if (angle_steps && cs[0] != '\0' && cs[0] != '!') {
        //                                    if (cs[0] == '%')       // value in the percents of fill_z
        //                                        limit_fill_z = angle_steps * this->object()->height() * 1e-8;
        //                                    else if (cs[0] == '#')  // value in the feet
        //                                        limit_fill_z = angle_steps * this->object()->config().layer_height;
        //                                    else if (cs[0] == '\'') // value in the feet
        //                                        limit_fill_z = angle_steps * 12 * 25.4;
        //                                    else if (cs[0] == '\"') // value in the inches
        //                                        limit_fill_z = angle_steps * 25.4;
        //                                    else if (cs[0] == 'c')  // value in centimeters
        //                                        limit_fill_z = angle_steps * 10.;
        //                                    else if (cs[0] == 'm')
        //                                        if (cs[1] == 'm') { // value in the millimeters
        //                                            limit_fill_z = angle_steps * 1.;
        //                                        } else              // value in the meters
        //                                            limit_fill_z = angle_steps * 1000.;
        //                                    limit_fill_z += fill_z;
        //                                    angle_steps = 0; // limit_fill_z has already count
        //                                } 
        //                            }
        //                            if (angle_steps) {       // if limit_fill_z does not setting by lenght method. Get count the layer id above model height
        //                                if (fill_form == std::string::npos && !_absolute) 
        //                                    angle_add     *= (int) angle_steps;
        //                                int idx       = i + std::max(angle_steps - 1, 0.);
        //                                int sdx       = std::max(0, idx - (int) this->object()->layers().size());
        //                                idx           = std::min(idx, (int) this->object()->layers().size() - 1);
        //                                limit_fill_z  = this->object()->get_layer(idx)->print_z + sdx * this->object()->config().layer_height;
        //                            } 
        //                            repeats = std::max(--repeats, 0);
        //                        } else 
        //                            _noop = true;   // set the dumb cycle
        //                        if (_absolute) {    // is absolute
        //                            angle_start = angle_add;
        //                            angle_add   = 0;
        //                        }
        //                    }
        //                    if (++t >= tk.size())
        //                        t = 0;
        //                } while (std::all_of(stop.begin(), stop.end(), [](bool v) { return v; }) ? false :
        //                             (t ? _noop : false) || stop[t]); // if this is a dumb instruction which never reaprated twice
        //            }
        //        }
        //        double top_z    = this->object()->get_layer(i)->print_z;
        //        double negvalue = (_negative ? limit_fill_z - top_z : top_z - start_fill_z) / (limit_fill_z - start_fill_z);

        //        switch (fill_form) {
        //        case 0: break;                                                  // /-joint, linear
        //        case 1: negvalue -= sin(negvalue * PI * 2.) / (PI * 2.); break; // N-joint, sinus, vertical start
        //        case 2: negvalue -= sin(negvalue * PI * 2.) / (PI * 4.); break; // n-joint, sinus, vertical start, lazy
        //        case 3: negvalue += sin(negvalue * PI * 2.) / (PI * 2.); break; // Z-joint, sinus, horizontal start
        //        case 4: negvalue += sin(negvalue * PI * 2.) / (PI * 4.); break; // z-joint, sinus, horizontal start, lazy
        //        case 5: negvalue = asin(negvalue * 2. - 1.) / PI + 0.5; break;  // $-joint, arcsin
        //        case 6: negvalue = sin(negvalue * PI / 2.); break;              // L-joint, quarter of circle, horizontal start
        //        case 7: negvalue = 1. - cos(negvalue * PI / 2.); break;         // l-joint, quarter of circle, vertical start
        //        case 8: negvalue = 1. - pow(1. - negvalue, 2); break;           // U-joint, squared, x2
        //        case 9: negvalue = pow(1 - negvalue, 2); break;                 // u-joint, squared, x2 inverse
        //        case 10: negvalue = 1. - pow(1. - negvalue, 3); break;          // Q-joint, cubic, x3
        //        case 11: negvalue = pow(1. - negvalue, 3); break;               // q-joint, cubic, x3 inverse
        //        case 12: negvalue = (double) rand() / RAND_MAX; break;          // ~-joint, random, fill the whole angle
        //        case 13: negvalue += (double) rand() / RAND_MAX - 0.5; break;   // ^-joint, pseudorandom, disperse at middle line
        //        case 14: negvalue = 0.5; break;                                 // |-joint, like #-joint but placed at middle angle
        //        case 15: negvalue = _negative ? 0. : 1.; break;                 // #-joint, vertical at the end angle
        //        }
        //        angle = angle_start + angle_add * negvalue;
        //    }
        //    if (solid != std::string::npos) {
        //        switch (solid) {
        //        case 1: params.pattern = region_config.internal_solid_infill_pattern.value; break; // selected solid pattern
        //        case 2: params.pattern = ipConcentric; break;                                      // concentric pattern 
        //        case 3: params.pattern = ipMonotonic; break;                                       // monotonic pattern 
        //        case 4: params.pattern = ipRectilinear;                                            // rectilinear pattern  
        //        }                                                                                  // or else use native pattern
        //        params.extrusion_role = erSolidInfill;
        //        params.density        = 1.;
        //        surface_fill.params.pattern = params.pattern;

        //        f = std::unique_ptr<Fill>(Fill::new_from_type(params.pattern)); // reinitialize surface
        //        f->set_bounding_box(bbox);
        //        f->layer_id            = this->id();
        //        f->z                   = this->print_z;
        //        f->angle               = surface_fill.params.angle;
        //        f->print_config        = &this->object()->print()->config();
        //        f->print_object_config = &this->object()->config();
        //        params.use_arachne = surface_fill.params.pattern == ipConcentric || surface_fill.params.pattern == ipConcentricInternal;
        //    }
        //    f->rotate_angle = Geometry::deg2rad(angle);
        //} else {
        //    rotate_angles.deserialize(v);
        //    auto rotate_angle_idx = f->layer_id % rotate_angles.size();
        //    f->rotate_angle = Geometry::deg2rad(rotate_angles.values[rotate_angle_idx]);
        //}

		if( surface_fill.params.pattern == ipLockedZag ) {
			params.locked_zag = true;
            params.infill_lock_depth = surface_fill.params.infill_lock_depth;
            params.skin_infill_depth = surface_fill.params.skin_infill_depth;
            f->set_lock_region_param(lock_param);
            f->set_skin_and_skeleton_pattern(surface_fill.params.skin_pattern, surface_fill.params.skeleton_pattern);
        }
        if (surface_fill.params.pattern == ipCrossZag || surface_fill.params.pattern == ipLockedZag) {
            if (f->layer_id % 2 == 0) {
                params.horiz_move -= surface_fill.params.infill_shift_step * (f->layer_id / 2);
            } else {
                params.horiz_move += surface_fill.params.infill_shift_step * (f->layer_id / 2);
            }

            params.symmetric_infill_y_axis = surface_fill.params.symmetric_infill_y_axis;

        } else if (surface_fill.params.pattern == ipZigZag) {
            params.symmetric_infill_y_axis = surface_fill.params.symmetric_infill_y_axis;
        }
		if (surface_fill.params.pattern == ipGrid)
			params.can_reverse = false;
		for (ExPolygon& expoly : surface_fill.expolygons) {

      f->no_overlap_expolygons = intersection_ex(surface_fill.no_overlap_expolygons, ExPolygons() = {expoly}, ApplySafetyOffset::Yes);
            if (params.symmetric_infill_y_axis) {
                params.symmetric_y_axis = f->extended_object_bounding_box().center().x();
                expoly.symmetric_y(params.symmetric_y_axis);
            }

			// Spacing is modified by the filler to indicate adjustments. Reset it for each expolygon.
			f->spacing = surface_fill.params.spacing;
			surface_fill.surface.expolygon = std::move(expoly);

			if(surface_fill.params.bridge && surface_fill.surface.is_external() && surface_fill.params.density > 99.0){
				params.density = layerm->region().config().bridge_density.get_abs_value(1.0);
			}
			// BBS: make fill
			f->fill_surface_extrusion(&surface_fill.surface,
				params,
				m_regions[surface_fill.region_id]->fills.entities);
		}
    }

    // add thin fill regions
    // Unpacks the collection, creates multiple collections per path.
    // The path type could be ExtrusionPath, ExtrusionLoop or ExtrusionEntityCollection.
    // Why the paths are unpacked?
	for (LayerRegion *layerm : m_regions)
	    for (const ExtrusionEntity *thin_fill : layerm->thin_fills.entities) {
	        ExtrusionEntityCollection &collection = *(new ExtrusionEntityCollection());
	        layerm->fills.entities.push_back(&collection);
	        collection.entities.push_back(thin_fill->clone());
	    }

#ifndef NDEBUG
	for (LayerRegion *layerm : m_regions)
	    for (size_t i = 0; i < layerm->fills.entities.size(); ++ i)
    	    assert(dynamic_cast<ExtrusionEntityCollection*>(layerm->fills.entities[i]) != nullptr);
#endif
}

Polylines Layer::generate_sparse_infill_polylines_for_anchoring(FillAdaptive::Octree* adaptive_fill_octree, FillAdaptive::Octree* support_fill_octree, FillLightning::Generator* lightning_generator, const Point offset) const
//Polylines Layer::generate_sparse_infill_polylines_for_anchoring(FillAdaptivinfill_polylines_for_anchoring(FillAdaptive::Octree* adaptive_fill_octree, FillAdaptive::Octree* support_fill_octree,  FillLightning::Generator* lightning_generator) const
//Polylines Layer::generate_sparse_infill_polylines_for_anchoring(FillAdaptive::Octree* adaptive_fill_octree, FillAdaptive::Octree* support_fill_octree, FillLightning::Generator* lightning_generator, Cross::SierpinskiFillProvider* cross_fill_provider, const Point offset) const
{
    LockRegionParam skin_inner_param;
    std::vector<SurfaceFill> surface_fills = group_fills(*this, skin_inner_param);
	const Slic3r::BoundingBox bbox = this->object()->bounding_box();
	const auto                resolution = this->object()->print()->config().resolution.value;

    Polylines sparse_infill_polylines{};

    for (SurfaceFill &surface_fill : surface_fills) {
		if (surface_fill.surface.surface_type != stInternal) {
			continue;
		}

        InfillPattern anchoring_pattern = surface_fill.params.pattern;
        switch (anchoring_pattern) {
        case ipField:
            // Use the selected regular TPMS cell to estimate bridge directions.
            // FillField's non-extrusion fallback generates straight lines instead.
            switch (m_regions[surface_fill.region_id]->region().config().cell_type.value) {
            case FieldCell_Gyroid:   anchoring_pattern = ipGyroid; break;
            case FieldCell_SchwarzD: anchoring_pattern = ipTpmsD; break;
            case FieldCell_FK:       anchoring_pattern = ipTpmsFK; break;
            }
            break;
        case ipCount: continue; break;
        case ipSupportBase: continue; break;
        case ipConcentricInternal: continue; break;
        case ipLightning:
        case ipCross:
        case ipCross3d:
        case ipquarter_cubic:
        case iptetrahedral:
		case ipAdaptiveCubic:
        case ipSupportCubic:
        case ipRectilinear:
        case ipMonotonic:
        case ipMonotonicLine:
        case ipAlignedRectilinear:
        case ipGrid:
        case ipLateralLattice:
        case ipTriangles:
        case ipStars:
        case ipCubic:
        case ipLine:
        case ipConcentric:
        case ipHoneycomb:
        case ipLateralHoneycomb:
        case ip3DHoneycomb:
        case ipGyroid:
        case ipTpmsD:
        case ipGradualTpmsG:
        case ipGradualTpmsD:  
        case ipGradualTpmsFK:
        case ipTpmsFK:
        case ipHilbertCurve:
        case ipArchimedeanChords:
        case ipOctagramSpiral:
        case ipZigZag:
        case ipCrossZag:
		case ipLockedZag:
            break;
        }

        // Create the filler object.
        std::unique_ptr<Fill> f = std::unique_ptr<Fill>(Fill::new_from_type(anchoring_pattern));
        f->set_bounding_box(bbox);
        f->layer_id = this->id() - this->object()->get_layer(0)->id(); // We need to subtract raft layers.
        f->z        = this->print_z;
        f->angle    = surface_fill.params.angle;
        f->adapt_fill_octree   = (surface_fill.params.pattern == ipSupportCubic) ? support_fill_octree : adaptive_fill_octree;
        f->print_config        = &this->object()->print()->config();
        f->print_object_config = &this->object()->config();

        LayerRegion &layerm = *m_regions[surface_fill.region_id];

        //creality for zero linewidth
        const PrintConfig &print_config = this->object()->print()->config();
        const size_t nozzle_index = get_physical_nozzle_index(print_config, layerm.region().extruder(frInfill) - 1);
        const double nozzle_diameter = print_config.nozzle_diameter.get_at(nozzle_index);
        double sparse_infill_line_width = nozzle_variant_abs_value(layerm.region().config().sparse_infill_line_width, nozzle_index, nozzle_diameter);
        if(sparse_infill_line_width == 0.0)
            sparse_infill_line_width = nozzle_diameter;
        double infill_line_distance = layerm.region().config().sparse_infill_density <= 0 ? 4.6 : sparse_infill_line_width * 100.0 / layerm.region().config().sparse_infill_density;

        if (surface_fill.params.pattern == ipLightning)
            dynamic_cast<FillLightning::Filler *>(f.get())->generator = lightning_generator;
        else if (surface_fill.params.pattern == ipCross || surface_fill.params.pattern == ipCross3d)
        {
            auto boundingbox = this->object()->bounding_box();
            dynamic_cast<FillCross*>(f.get())->set_cross_fill_provider(boundingbox, offset, surface_fill.params.pattern, infill_line_distance * 1000.0f, sparse_infill_line_width * 1000.0f);
        }
        else if (surface_fill.params.pattern == ipquarter_cubic || surface_fill.params.pattern == iptetrahedral)
        {
            Vec3crd center = this->object()->size();
            Point _offset = offset;

            dynamic_cast<FillQuarter*>(f.get())->setOrigin(surface_fill.params.pattern,Point(center.x() / 2.0, center.y() / 2.0), _offset, slice_z* 1000.0f, infill_line_distance * 1000.0f, sparse_infill_line_width * 1000.0f);
        }

        // calculate flow spacing for infill pattern generation
        double link_max_length = 0.;
        if (!surface_fill.params.bridge) {
#if 0
            link_max_length = layerm.region()->config().get_abs_value(surface.is_external() ? "external_fill_link_max_length" : "fill_link_max_length", flow.spacing());
//            printf("flow spacing: %f,  is_external: %d, link_max_length: %lf\n", flow.spacing(), int(surface.is_external()), link_max_length);
#else
            if (surface_fill.params.density > 80.) // 80%
                link_max_length = 3. * f->spacing;
#endif
        }

        // Maximum length of the perimeter segment linking two infill lines.
        f->link_max_length = (coord_t) scale_(link_max_length);
        // Used by the concentric infill pattern to clip the loops to create extrusion paths.
        f->loop_clipping = coord_t(scale_(layerm.region().config().seam_gap.get_abs_value(surface_fill.params.flow.nozzle_diameter())));

        // apply half spacing using this flow's own spacing and generate infill
        FillParams params;
        params.density           = float(0.01 * surface_fill.params.density);
        params.dont_adjust       = false; //  surface_fill.params.dont_adjust;
        params.anchor_length     = surface_fill.params.anchor_length;
        params.anchor_length_max = surface_fill.params.anchor_length_max;
        params.resolution        = resolution;
        params.use_arachne       = false;
        params.layer_height      = layerm.layer()->height;
        params.lateral_lattice_angle_1   = surface_fill.params.lateral_lattice_angle_1;
        params.lateral_lattice_angle_2   = surface_fill.params.lateral_lattice_angle_2;
        params.infill_overhang_angle   = surface_fill.params.infill_overhang_angle;
        params.multiline         = surface_fill.params.multiline;
        params.solid_skeleton_wipe_path = surface_fill.params.solid_skeleton_wipe_path;
        params.solid_skeleton_start_corner = surface_fill.params.solid_skeleton_start_corner;

        for (ExPolygon &expoly : surface_fill.expolygons) {
            // Spacing is modified by the filler to indicate adjustments. Reset it for each expolygon.
            f->spacing                     = surface_fill.params.spacing;
            surface_fill.surface.expolygon = std::move(expoly);
            try {
                Polylines polylines = f->fill_surface(&surface_fill.surface, params);
                sparse_infill_polylines.insert(sparse_infill_polylines.end(), polylines.begin(), polylines.end());
            } catch (InfillFailedException &) {}
        }
    }

    return sparse_infill_polylines;
}

// Create ironing extrusions over top surfaces.
void Layer::make_ironing()
{
	// LayerRegion::slices contains surfaces marked with SurfaceType.
	// Here we want to collect top surfaces extruded with the same extruder.
	// A surface will be ironed with the same extruder to not contaminate the print with another material leaking from the nozzle.

	// First classify regions based on the extruder used.
	struct IroningParams {
		InfillPattern pattern;
		int 		extruder 	= -1;
		bool 		just_infill = false;
		// Spacing of the ironing lines, also to calculate the extrusion flow from.
		double 		line_spacing;
		// Height of the extrusion, to calculate the extrusion flow from.
		double 		height;
		double 		speed;
		double 		angle;

		bool operator<(const IroningParams &rhs) const {
			if (this->extruder < rhs.extruder)
				return true;
			if (this->extruder > rhs.extruder)
				return false;
			if (int(this->just_infill) < int(rhs.just_infill))
				return true;
			if (int(this->just_infill) > int(rhs.just_infill))
				return false;
			if (this->line_spacing < rhs.line_spacing)
				return true;
			if (this->line_spacing > rhs.line_spacing)
				return false;
			if (this->height < rhs.height)
				return true;
			if (this->height > rhs.height)
				return false;
			if (this->speed < rhs.speed)
				return true;
			if (this->speed > rhs.speed)
				return false;
			if (this->angle < rhs.angle)
				return true;
			if (this->angle > rhs.angle)
				return false;
			return false;
		}

		bool operator==(const IroningParams &rhs) const {
			return this->extruder == rhs.extruder && this->just_infill == rhs.just_infill &&
				   this->line_spacing == rhs.line_spacing && this->height == rhs.height && this->speed == rhs.speed && this->angle == rhs.angle && this->pattern == rhs.pattern;
		}

		LayerRegion *layerm		= nullptr;

		// IdeaMaker: ironing
		// ironing flowrate (5% percent)
		// ironing speed (10 mm/sec)

		// Kisslicer:
		// iron off, Sweep, Group
		// ironing speed: 15 mm/sec

		// Cura:
		// Pattern (zig-zag / concentric)
		// line spacing (0.1mm)
		// flow: from normal layer height. 10%
		// speed: 20 mm/sec
	};

	std::vector<IroningParams> by_extruder;
    double default_layer_height = this->object()->config().layer_height;

	for (LayerRegion *layerm : m_regions)
		if (! layerm->slices.empty()) {
			IroningParams ironing_params;
			const PrintRegionConfig &config = layerm->region().config();
			if (config.ironing_type != IroningType::NoIroning &&
				(config.ironing_type == IroningType::AllSolid ||
				 	(config.top_shell_layers > 0 &&
						(config.ironing_type == IroningType::TopSurfaces ||
					 	(config.ironing_type == IroningType::TopmostOnly && layerm->layer()->upper_layer == nullptr))))) {
				if (config.wall_filament == config.solid_infill_filament || config.wall_loops == 0) {
					// Iron the whole face.
					ironing_params.extruder = config.solid_infill_filament;
				} else {
					// Iron just the infill.
					ironing_params.extruder = config.solid_infill_filament;
				}
			}
			if (ironing_params.extruder != -1) {
				//TODO just_infill is currently not used.
				ironing_params.just_infill 	= false;
				ironing_params.line_spacing = config.ironing_spacing;
				ironing_params.height 		= default_layer_height * 0.01 * config.ironing_flow;
				ironing_params.speed 		= config.ironing_speed;
                ironing_params.angle        = (config.ironing_angle >= 0 ? config.ironing_angle : config.infill_direction) * M_PI / 180.;
				ironing_params.pattern      = config.ironing_pattern;
				ironing_params.layerm 		= layerm;
				by_extruder.emplace_back(ironing_params);
			}
		}
	std::sort(by_extruder.begin(), by_extruder.end());

    FillParams 			fill_params;
    fill_params.density 	 = 1.;
    fill_params.monotonic    = true;
    InfillPattern         f_pattern = ipRectilinear;
    std::unique_ptr<Fill> f         = std::unique_ptr<Fill>(Fill::new_from_type(f_pattern));
    f->set_bounding_box(this->object()->bounding_box());
    f->layer_id = this->id();
    f->z        = this->print_z;
    f->overlap  = 0;
	for (size_t i = 0; i < by_extruder.size();) {
		// Find span of regions equivalent to the ironing operation.
		IroningParams &ironing_params = by_extruder[i];
		// Create the filler object.
		if( f_pattern != ironing_params.pattern )
		{
            f_pattern               = ironing_params.pattern;
            f = std::unique_ptr<Fill>(Fill::new_from_type(f_pattern));
            f->set_bounding_box(this->object()->bounding_box());
            f->layer_id = this->id();
            f->z        = this->print_z;
            f->overlap  = 0;
		}

		size_t j = i;
		for (++ j; j < by_extruder.size() && ironing_params == by_extruder[j]; ++ j) ;

		// Create the ironing extrusions for regions <i, j)
		ExPolygons ironing_areas;
		double nozzle_dmr = get_physical_nozzle_diameter(this->object()->print()->config(), ironing_params.extruder - 1);
		if (ironing_params.just_infill) {
			//TODO just_infill is currently not used.
			// Just infill.
		} else {
			// Infill and perimeter.
			// Merge top surfaces with the same ironing parameters.
			Polygons polys;
			Polygons infills;
			for (size_t k = i; k < j; ++ k) {
				const IroningParams		 &ironing_params  = by_extruder[k];
				const PrintRegionConfig  &region_config   = ironing_params.layerm->region().config();
				bool					  iron_everything = region_config.ironing_type == IroningType::AllSolid;
				bool					  iron_completely = iron_everything;
				if (iron_everything) {
					// Check whether there is any non-solid hole in the regions.
					bool internal_infill_solid = region_config.sparse_infill_density.value > 95.;
					for (const Surface &surface : ironing_params.layerm->fill_surfaces.surfaces)
						if ((!internal_infill_solid && surface.surface_type == stInternal) || surface.surface_type == stInternalBridge || surface.surface_type == stInternalVoid) {
							// Some fill region is not quite solid. Don't iron over the whole surface.
							iron_completely = false;
							break;
						}
				}
				if (iron_completely) {
					// Iron everything. This is likely only good for solid transparent objects.
					for (const Surface &surface : ironing_params.layerm->slices.surfaces)
						polygons_append(polys, surface.expolygon);
				} else {
					for (const Surface &surface : ironing_params.layerm->slices.surfaces)
						if ((surface.surface_type == stTop && region_config.top_shell_layers > 0) || (iron_everything && surface.surface_type == stBottom && region_config.bottom_shell_layers > 0))
							// stBottomBridge is not being ironed on purpose, as it would likely destroy the bridges.
							polygons_append(polys, surface.expolygon);
				}
				if (iron_everything && ! iron_completely) {
					// Add solid fill surfaces. This may not be ideal, as one will not iron perimeters touching these
					// solid fill surfaces, but it is likely better than nothing.
					for (const Surface &surface : ironing_params.layerm->fill_surfaces.surfaces)
						if (surface.surface_type == stInternalSolid)
							polygons_append(infills, surface.expolygon);
				}
			}

			if (! infills.empty() || j > i + 1) {
				// Ironing over more than a single region or over solid internal infill.
				if (! infills.empty())
					// For IroningType::AllSolid only:
					// Add solid infill areas for layers, that contain some non-ironable infil (sparse infill, bridge infill).
					append(polys, std::move(infills));
				polys = union_safety_offset(polys);
			}
			// Trim the top surfaces with half the nozzle diameter.
			ironing_areas = intersection_ex(polys, offset(this->lslices, - float(scale_(0.5 * nozzle_dmr))));
		}

        // Create the filler object.
        f->spacing = ironing_params.line_spacing;
        f->angle = float(ironing_params.angle);
        f->link_max_length = (coord_t) scale_(3. * f->spacing);
		double  extrusion_height = ironing_params.height * f->spacing / nozzle_dmr;
		float  extrusion_width  = Flow::rounded_rectangle_extrusion_width_from_spacing(float(nozzle_dmr), float(extrusion_height));
		double flow_mm3_per_mm = nozzle_dmr * extrusion_height;
        Surface surface_fill(stTop, ExPolygon());
        for (ExPolygon &expoly : ironing_areas) {
			surface_fill.expolygon = std::move(expoly);
			Polylines polylines;
			try {
				polylines = f->fill_surface(&surface_fill, fill_params);
			} catch (InfillFailedException &) {
			}
	        if (! polylines.empty()) {
		        // Save into layer.
				ExtrusionEntityCollection *eec = nullptr;
		        ironing_params.layerm->fills.entities.push_back(eec = new ExtrusionEntityCollection());
		        // Don't sort the ironing infill lines as they are monotonicly ordered.
				eec->no_sort = true;
		        extrusion_entities_append_paths(
		            eec->entities, std::move(polylines),
		            erIroning,
		            flow_mm3_per_mm, extrusion_width, float(extrusion_height));
		    }
		}

		// Regions up to j were processed.
		i = j;
	}
}

} // namespace Slic3r
