// Copyright (c) 2022 Ultimaker B.V.
// CuraEngine is released under the terms of the AGPLv3 or higher.

#include <algorithm> //For std::partition_copy and std::min_element.
#include <unordered_set>

#include "WallToolPaths.h"
#include "BeadingStrategyFactory.h"

#include "SkeletalTrapezoidation.h"
#include "VariableLineUtils.h"
#include "utils/polygonUtils.h"

#include "utils/SettingsWrapper.h"
#include "types/EnumSettings.h"

#include "communication/slicecontext.h"
#include "crslice/../src/conv.h"

namespace cura52
{

    WallToolPaths::WallToolPaths(const Polygons& outline, const coord_t nominal_bead_width, const size_t inset_count, const coord_t wall_0_inset,
        const Settings& settings)
        : outline(outline)
        , bead_width_0(nominal_bead_width)
        , bead_width_x(nominal_bead_width)
        , inset_count(inset_count)
        , wall_0_inset(wall_0_inset)
        , print_thin_walls(settings.get<bool>("fill_outline_gaps"))
        , min_feature_size(settings.get<coord_t>("min_feature_size"))
        , min_bead_width(settings.get<coord_t>("min_bead_width"))
        , small_area_length(INT2MM(static_cast<double>(nominal_bead_width) / 2))
        , toolpaths_generated(false)
        , settings(settings)
    {
    }

    WallToolPaths::WallToolPaths(const Polygons& outline, const coord_t bead_width_0, const coord_t bead_width_x,
        const size_t inset_count, const coord_t wall_0_inset, const Settings& settings)
        : outline(outline)
        , bead_width_0(bead_width_0)
        , bead_width_x(bead_width_x)
        , inset_count(inset_count)
        , wall_0_inset(wall_0_inset)
        , print_thin_walls(settings.get<bool>("fill_outline_gaps"))
        , min_feature_size(settings.get<coord_t>("min_feature_size"))
        , min_bead_width(settings.get<coord_t>("min_bead_width"))
        , small_area_length(INT2MM(static_cast<double>(bead_width_0) / 2))
        , toolpaths_generated(false)
        , settings(settings)
    {
    }

    const std::vector<VariableWidthLines>& WallToolPaths::generate()
    {
        const coord_t allowed_distance = settings.get<coord_t>("meshfix_maximum_deviation");
        const coord_t epsilon_offset = (allowed_distance / 2) - 1;
        const AngleRadians transitioning_angle = settings.get<AngleRadians>("wall_transition_angle");
        constexpr coord_t discretization_step_size = MM2INT(0.8);

        double scaled_spacing_wall_0 = bead_width_0;
        double scaled_spacing_wall_X = bead_width_x;
        const coord_t layer_thickness = settings.get<coord_t>("layer_height");
        const bool exact_flow_enable = settings.get<bool>("special_exact_flow_enable");
        auto getScaledSpacing = [layer_thickness](size_t line_width)
        {
            size_t out = line_width - layer_thickness * float(1. - 0.25 * M_PI);
            if (out <= 0.f)
                return line_width;
            return out;
        };
        if (exact_flow_enable)
        {
            scaled_spacing_wall_0 = getScaledSpacing(bead_width_0);
            scaled_spacing_wall_X = getScaledSpacing(bead_width_x);
        }

        // Simplify outline for boost::voronoi consumption. Absolutely no self intersections or near-self intersections allowed:
        // TODO: Open question: Does this indeed fix all (or all-but-one-in-a-million) cases for manifold but otherwise possibly complex polygons?
        Polygons prepared_outline = outline.offset(-epsilon_offset).offset(epsilon_offset * 2).offset(-epsilon_offset);
        prepared_outline = simplifyPolygon(prepared_outline, settings);
        PolygonUtils::fixSelfIntersections(epsilon_offset, prepared_outline);
        prepared_outline.removeDegenerateVerts();
        prepared_outline.removeColinearEdges(AngleRadians(0.005));
        // Removing collinear edges may introduce self intersections, so we need to fix them again
        PolygonUtils::fixSelfIntersections(epsilon_offset, prepared_outline);
        prepared_outline.removeDegenerateVerts();
        prepared_outline.removeSmallAreas(small_area_length * small_area_length, false);
        prepared_outline = prepared_outline.unionPolygons();

        if (prepared_outline.area() <= 0)
        {
            assert(toolpaths.empty());
            return toolpaths;
        }

        const coord_t wall_transition_length = settings.get<coord_t>("wall_transition_length");

        // When to split the middle wall into two:
        const double min_even_wall_line_width = settings.get<double>("min_even_wall_line_width");
        const double wall_line_width_0 = settings.get<double>("wall_line_width_0");
        const Ratio wall_split_middle_threshold = std::max(1.0, std::min(99.0, 100.0 * (2.0 * min_even_wall_line_width - wall_line_width_0) / wall_line_width_0)) / 100.0;

        // When to add a new middle in between the innermost two walls:
        const double min_odd_wall_line_width = settings.get<double>("min_odd_wall_line_width");
        const double wall_line_width_x = settings.get<double>("wall_line_width_x");
        const Ratio wall_add_middle_threshold = std::max(1.0, std::min(99.0, 100.0 * min_odd_wall_line_width / wall_line_width_x)) / 100.0;

        const int wall_distribution_count = settings.get<int>("wall_distribution_count");
        const size_t max_bead_count = (inset_count < (size_t)std::numeric_limits<coord_t>::max() / 2) ? 2 * inset_count : std::numeric_limits<coord_t>::max();
        const auto beading_strat = BeadingStrategyFactory::makeStrategy
        (
            scaled_spacing_wall_0,
            scaled_spacing_wall_X,
            wall_transition_length,
            transitioning_angle,
            print_thin_walls,
            min_bead_width,
            min_feature_size,
            wall_split_middle_threshold,
            wall_add_middle_threshold,
            max_bead_count,
            wall_0_inset,
            wall_distribution_count
        );
        const coord_t transition_filter_dist = settings.get<coord_t>("wall_transition_filter_distance");
        const coord_t allowed_filter_deviation = settings.get<coord_t>("wall_transition_filter_deviation");

        const coord_t stitch_distance = settings.get<coord_t>("wall_line_width_x") - 1; //In 0-width contours, junctions can cause up to 1-line-width gaps. Don't stitch more than 1 line width.
        coord_t max_resolution = settings.get<coord_t>("meshfix_maximum_resolution");
        coord_t max_deviation = settings.get<coord_t>("meshfix_maximum_deviation");
        coord_t max_area_deviation = settings.get<coord_t>("meshfix_maximum_extrusion_area_deviation");

#ifdef USE_CACHE
        crslice::SerailCrSkeletal serialSkeletal;
        
        serialSkeletal.param.allowed_distance = INT2MM(allowed_distance);
        serialSkeletal.param.transitioning_angle = transitioning_angle;
        serialSkeletal.param.discretization_step_size = INT2MM(discretization_step_size);
        serialSkeletal.param.scaled_spacing_wall_0 = scaled_spacing_wall_0;
        serialSkeletal.param.scaled_spacing_wall_X = scaled_spacing_wall_X;
        serialSkeletal.param.wall_transition_length = INT2MM(wall_transition_length);
        serialSkeletal.param.min_even_wall_line_width = min_even_wall_line_width;
        serialSkeletal.param.wall_line_width_0 = wall_line_width_0;
        serialSkeletal.param.wall_split_middle_threshold = wall_split_middle_threshold;
        serialSkeletal.param.min_odd_wall_line_width = min_odd_wall_line_width;
        serialSkeletal.param.wall_line_width_x = wall_line_width_x;
        serialSkeletal.param.wall_add_middle_threshold = wall_add_middle_threshold;
        serialSkeletal.param.wall_distribution_count = wall_distribution_count;
        serialSkeletal.param.max_bead_count = max_bead_count;
        serialSkeletal.param.transition_filter_dist = INT2MM(transition_filter_dist);
        serialSkeletal.param.allowed_filter_deviation = INT2MM(allowed_filter_deviation);
        serialSkeletal.param.print_thin_walls = print_thin_walls ? 1 : 0;
        serialSkeletal.param.min_feature_size = INT2MM(min_feature_size);
        serialSkeletal.param.min_bead_width = INT2MM(min_bead_width);
        serialSkeletal.param.wall_0_inset = INT2MM(wall_0_inset);
        serialSkeletal.param.wall_inset_count = inset_count;
        serialSkeletal.param.stitch_distance = INT2MM(stitch_distance);
        serialSkeletal.param.max_resolution = INT2MM(max_resolution);
        serialSkeletal.param.max_deviation = INT2MM(max_deviation);
        serialSkeletal.param.max_area_deviation = INT2MM(max_area_deviation);
        
        crslice::convertPolygonRaw(prepared_outline, serialSkeletal.polygons);
        cacheSkeletal(serialSkeletal);
#endif

        SkeletalTrapezoidation wall_maker
        (
            prepared_outline,
            *beading_strat,
            beading_strat->getTransitioningAngle(),
            discretization_step_size * 1000,  //scale
            transition_filter_dist,
            allowed_filter_deviation,
            wall_transition_length
        );

        wall_maker.generateToolpaths(toolpaths);

        stitchToolPaths(toolpaths, stitch_distance);
        removeSmallLines(toolpaths);

        inner_contour = separateOutInnerContour(toolpaths, inset_count);
        simplifyToolPaths(toolpaths, max_resolution, max_deviation, max_area_deviation);
        removeEmptyToolPaths(toolpaths);

        for (VariableWidthLines& path : toolpaths)
        {
            for (ExtrusionLine& line : path)
            {
                line.start_idx = -1;
            }
        }
        assert(std::is_sorted(toolpaths.cbegin(), toolpaths.cend(),
            [](const VariableWidthLines& l, const VariableWidthLines& r)
            {
                return l.front().inset_idx < r.front().inset_idx;
            }) && "WallToolPaths should be sorted from the outer 0th to inner_walls");
        toolpaths_generated = true;
        return toolpaths;
    }

    const std::vector<VariableWidthLines>& WallToolPaths::getToolPaths()
    {
        if (!toolpaths_generated)
        {
            return generate();
        }
        return toolpaths;
    }

    void WallToolPaths::pushToolPaths(std::vector<VariableWidthLines>& paths)
    {
        if (!toolpaths_generated)
        {
            generate();
        }
        paths.insert(paths.end(), toolpaths.begin(), toolpaths.end());
    }

    const Polygons& WallToolPaths::getInnerContour()
    {
        if (!toolpaths_generated && inset_count > 0)
        {
            generate();
        }
        else if (inset_count == 0)
        {
            return outline;
        }
        return inner_contour;
    }

} // namespace cura52
