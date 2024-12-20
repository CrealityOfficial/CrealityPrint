// Copyright (c) 2022 Ultimaker B.V.
// CuraEngine is released under the terms of the AGPLv3 or higher

#include <algorithm> //For std::sort.
#include <functional>
#include <unordered_set>

#include "limits.h"

#include "ccglobal/log.h"

#include "skeletal/WallToolPaths.h"
#include "infill.h"
#include "infill/GyroidInfill.h"
#include "infill/ImageBasedDensityProvider.h"
#include "infill/LightningGenerator.h"
#include "infill/NoZigZagConnectorProcessor.h"
#include "infill/SierpinskiFill.h"
#include "infill/SierpinskiFillProvider.h"
#include "infill/SubDivCube.h"
#include "infill/UniformDensityProvider.h"
#include "communication/sliceDataStorage.h"
#include "utils/PolygonConnector.h"
#include "utils/PolylineStitcher.h"
#include "utils/Simplify.h"
#include "utils/UnionFind.h"
#include "utils/polygonUtils.h"

/*!
 * Function which returns the scanline_idx for a given x coordinate
 *
 * For negative \p x this is different from simple division.
 *
 * \warning \p line_width is assumed to be positive
 *
 * \param x the point to get the scansegment index for
 * \param line_width the width of the scan segments
 */
static inline int computeScanSegmentIdx(int x, int line_width)
{
    if (x < 0)
    {
        return (x + 1) / line_width - 1;
        // - 1 because -1 belongs to scansegment -1
        // + 1 because -line_width belongs to scansegment -1
    }
    return x / line_width;
}

namespace cura52
{

Polygons Infill::generateWallToolPaths(std::vector<VariableWidthLines>& toolpaths, Polygons& outer_contour, const size_t wall_line_count, const coord_t line_width, const coord_t infill_overlap, const Settings& settings)
{
    outer_contour = outer_contour.offset(infill_overlap);

    Polygons inner_contour;
    if (wall_line_count > 0)
    {
        constexpr coord_t wall_0_inset = 0; // Don't apply any outer wall inset for these. That's just for the outer wall.
        WallToolPaths wall_toolpaths(outer_contour, line_width, wall_line_count, wall_0_inset, settings);
        wall_toolpaths.pushToolPaths(toolpaths);
        inner_contour = wall_toolpaths.getInnerContour();
    }
    else
    {
        inner_contour = outer_contour;
    }
    return inner_contour;
}

void Infill::generate(std::vector<VariableWidthLines>& toolpaths,
                      Polygons& result_polygons,
                      Polygons& result_lines,
                      const Settings& settings,
                      const SierpinskiFillProvider* cross_fill_provider,
                      const LightningLayer* lightning_trees,
                      const SliceMeshStorage* mesh)
{
    if (outer_contour.empty())
    {
        return;
    }

    inner_contour = generateWallToolPaths(toolpaths, outer_contour, wall_line_count, infill_line_width, infill_overlap, settings);

    // Apply a half-line-width offset if the pattern prints partly alongside the walls, to get an area that we can simply print the centreline alongside the edge.
    // The lines along the edge must lie next to the border, not on it.
    // This makes those algorithms a lot simpler.
    if (pattern == EFillMethod::ZIG_ZAG // Zig-zag prints the zags along the walls.
		|| pattern == EFillMethod::ALIGNLINES
        || (zig_zaggify
            && (pattern == EFillMethod::LINES // Zig-zaggified infill patterns print their zags along the walls.
                || pattern == EFillMethod::TRIANGLES || pattern == EFillMethod::GRID || pattern == EFillMethod::CUBIC || pattern == EFillMethod::TETRAHEDRAL || pattern == EFillMethod::QUARTER_CUBIC || pattern == EFillMethod::TRIHEXAGON
                || pattern == EFillMethod::GYROID || pattern == EFillMethod::CROSS || pattern == EFillMethod::CROSS_3D))
        || infill_multiplier % 2 == 0) // Multiplied infill prints loops of infill, partly along the walls, if even. For odd multipliers >1 it gets offset by the multiply algorithm itself.
    {
        // Get gaps beforehand (that are caused when the 1/2 line width inset is done after this):
        // (Note that we give it a _full_ line width here, because unlike the old situation this can produce walls that are actually smaller than that.)
        constexpr coord_t gap_wall_count = 1; // Only need one wall here, less even, in a sense.
        constexpr coord_t wall_0_inset = 0; // Don't apply any outer wall inset for these. That's just for the outer wall.
        WallToolPaths wall_toolpaths(inner_contour, infill_line_width, gap_wall_count, wall_0_inset, settings);
        std::vector<VariableWidthLines> gap_fill_paths = wall_toolpaths.getToolPaths();

        // Add the gap filling to the toolpaths and make the new inner contour 'aware' of the gap infill:
        // (Can't use getContours here, because only _some_ of the lines Arachne has generated are needed.)
        Polygons gap_filled_areas;
        for (const auto& var_width_line : gap_fill_paths)
        {
            VariableWidthLines thin_walls_only;
            for (const auto& extrusion : var_width_line)
            {
                if (extrusion.is_odd && extrusion.inset_idx == 0)
                {
                    Polygon path;
                    for (const auto& junction : extrusion.junctions)
                    {
                        path.add(junction.p);
                    }
                    if (path.polygonLength() >= infill_line_width * 4) // Don't fill gaps that are very small (with paths less than 2 line widths long, 4 back and forth).
                    {
                        gap_filled_areas.add(path);
                        if (fill_gaps)
                        {
                            thin_walls_only.push_back(extrusion);
                        }
                    }
                }
            }
            if (! thin_walls_only.empty())
            {
                toolpaths.push_back(thin_walls_only);
            }
        }
        gap_filled_areas = gap_filled_areas.offsetPolyLine(infill_line_width / 2).unionPolygons();

        // Now do the actual inset, to make place for the extra 'zig-zagify' lines:
        inner_contour = inner_contour.difference(gap_filled_areas).offset(-infill_line_width / 2);
    }
    inner_contour = Simplify(max_resolution, max_deviation, 0).polygon(inner_contour);

    if (infill_multiplier > 1)
    {
        bool zig_zaggify_real = zig_zaggify;
        if (infill_multiplier % 2 == 0)
        {
            zig_zaggify = false;
        }
        Polygons generated_result_polygons;
        Polygons generated_result_lines;

        _generate(toolpaths, generated_result_polygons, generated_result_lines, settings, cross_fill_provider, lightning_trees, mesh);
        zig_zaggify = zig_zaggify_real;
        multiplyInfill(generated_result_polygons, generated_result_lines);
        result_polygons.add(generated_result_polygons);
        result_lines.add(generated_result_lines);
    }
    else
    {
        //_generate may clear() the generated_result_lines, but this is an output variable that may contain data before we start.
        // So make sure we provide it with a Polygons that is safe to clear and only add stuff to result_lines.
        Polygons generated_result_polygons;
        Polygons generated_result_lines;
        _generate(toolpaths, generated_result_polygons, generated_result_lines, settings, cross_fill_provider, lightning_trees, mesh);
        result_polygons.add(generated_result_polygons);
        result_lines.add(generated_result_lines);
    }
    if (connect_polygons)
    {
        // remove too small polygons
        coord_t snap_distance = infill_line_width * 2; // polygons with a span of max 1 * nozzle_size are too small
        auto it = std::remove_if(result_polygons.begin(), result_polygons.end(), [snap_distance](PolygonRef poly) { return poly.shorterThan(snap_distance); });
        result_polygons.erase(it, result_polygons.end());

        PolygonConnector connector(infill_line_width);
        connector.add(result_polygons);
        connector.add(toolpaths);
        Polygons connected_polygons;
        std::vector<VariableWidthLines> connected_paths;
        connector.connect(connected_polygons, connected_paths);
        result_polygons = connected_polygons;
        toolpaths = connected_paths;
    }
}


Polygons Infill::generateWallToolPathsT(std::vector<VariableWidthLines>& toolpaths, Polygons& outer_contour, const size_t wall_line_count, const coord_t line_width, const coord_t infill_overlap, const Settings& settings, int layer_idx, cura54::SectionType section_type)
{
    outer_contour = outer_contour.offset(infill_overlap);
    //scripta::log("infill_outer_contour", outer_contour, section_type, layer_idx, scripta::CellVDI{ "infill_overlap", infill_overlap });

    Polygons inner_contour;
    if (wall_line_count > 0)
    {
        constexpr coord_t wall_0_inset = 0; // Don't apply any outer wall inset for these. That's just for the outer wall.
        WallToolPaths wall_toolpaths(outer_contour, line_width, wall_line_count, wall_0_inset, settings);

        wall_toolpaths.pushToolPaths(toolpaths);
        inner_contour = wall_toolpaths.getInnerContour();
    }
    else
    {
        inner_contour = outer_contour;
    }
    return inner_contour;
}

void Infill::generateThomas(std::vector<VariableWidthLines>& toolpaths,
    Polygons& result_polygons,
    Polygons& result_lines,
    const Settings& settings,
    int layer_idx,
    cura54::SectionType section_type,
    const SierpinskiFillProvider* cross_fill_provider,
    const LightningLayer* lightning_trees,
    const SliceMeshStorage* mesh)
{
    if (outer_contour.empty())
    {
        return;
    }

    inner_contour = generateWallToolPathsT(toolpaths, outer_contour, wall_line_count, infill_line_width, infill_overlap, settings, layer_idx, section_type);
    //scripta::log("infill_inner_contour_0", inner_contour, section_type, layer_idx);

    // It does not make sense to print a pattern in a small region. So the infill region
    // is split into a small region that will be filled with walls and the normal region
    // that will be filled with the pattern. This split of regions is not needed if the
    // infill pattern is concentric or if the small_area_width is zero.
    if (pattern != EFillMethod::CONCENTRIC && small_area_width > 0)
    {
        // Split the infill region in a narrow region and the normal region.
        Polygons small_infill = inner_contour;
        inner_contour = inner_contour.offset(-small_area_width / 2).offset(small_area_width / 2);
        small_infill = small_infill.difference(inner_contour);
        small_infill = Simplify(max_resolution, max_deviation, 0).polygon(small_infill);

        // Small corners of a bigger area should not be considered narrow and are therefore added to the bigger area again.
        auto small_infill_parts = small_infill.splitIntoParts();
        small_infill.clear();
        for (const auto& small_infill_part : small_infill_parts)
        {
            if (
                small_infill_part.offset(-infill_line_width / 2).offset(infill_line_width / 2).area() < infill_line_width * infill_line_width * 10
                && !inner_contour.intersection(small_infill_part.offset(infill_line_width / 4)).empty()
                )
            {
                inner_contour.add(small_infill_part);
            }
            else
            {
                // the part must still be printed, so re-add it
                small_infill.add(small_infill_part);
            }
        }
        inner_contour.unionPolygons();

        // Fill narrow area with walls.
        const size_t narrow_wall_count = small_area_width / infill_line_width + 1;
        WallToolPaths wall_toolpaths(small_infill, infill_line_width, narrow_wall_count, 0, settings);
        std::vector<VariableWidthLines> small_infill_paths = wall_toolpaths.getToolPaths();
        //scripta::log("infill_small_infill_paths_0", small_infill_paths, section_type, layer_idx,
        //    scripta::CellVDI{ "is_closed", &ExtrusionLine::is_closed },
        //    scripta::CellVDI{ "is_odd", &ExtrusionLine::is_odd },
        //    scripta::CellVDI{ "inset_idx", &ExtrusionLine::inset_idx },
        //    scripta::PointVDI{ "width", &ExtrusionJunction::w },
        //    scripta::PointVDI{ "perimeter_index", &ExtrusionJunction::perimeter_index });
        for (const auto& small_infill_path : small_infill_paths)
        {
            toolpaths.emplace_back(small_infill_path);
        }
    }
    //scripta::log("infill_inner_contour_1", inner_contour, section_type, layer_idx);

    // apply an extra offset in case the pattern prints along the sides of the area.
    if (pattern == EFillMethod::ZIG_ZAG // Zig-zag prints the zags along the walls.
        || (zig_zaggify
            && (pattern == EFillMethod::LINES // Zig-zaggified infill patterns print their zags along the walls.
                || pattern == EFillMethod::TRIANGLES || pattern == EFillMethod::GRID || pattern == EFillMethod::CUBIC || pattern == EFillMethod::TETRAHEDRAL || pattern == EFillMethod::QUARTER_CUBIC || pattern == EFillMethod::TRIHEXAGON
                || pattern == EFillMethod::GYROID || pattern == EFillMethod::CROSS || pattern == EFillMethod::CROSS_3D))
        || infill_multiplier % 2 == 0) // Multiplied infill prints loops of infill, partly along the walls, if even. For odd multipliers >1 it gets offset by the multiply algorithm itself.
    {
        inner_contour = inner_contour.offset(-infill_line_width / 2);
        inner_contour = Simplify(max_resolution, max_deviation, 0).polygon(inner_contour);
    }
    //scripta::log("infill_inner_contour_2", inner_contour, section_type, layer_idx);

    if (infill_multiplier > 1)
    {
        bool zig_zaggify_real = zig_zaggify;
        if (infill_multiplier % 2 == 0)
        {
            zig_zaggify = false;
        }
        Polygons generated_result_polygons;
        Polygons generated_result_lines;

        _generate(toolpaths, generated_result_polygons, generated_result_lines, settings, cross_fill_provider, lightning_trees, mesh);
        zig_zaggify = zig_zaggify_real;
        multiplyInfill(generated_result_polygons, generated_result_lines);
        result_polygons.add(generated_result_polygons);
        result_lines.add(generated_result_lines);
    }
    else
    {
        //_generate may clear() the generated_result_lines, but this is an output variable that may contain data before we start.
        // So make sure we provide it with a Polygons that is safe to clear and only add stuff to result_lines.
        Polygons generated_result_polygons;
        Polygons generated_result_lines;
        _generate(toolpaths, generated_result_polygons, generated_result_lines, settings, cross_fill_provider, lightning_trees, mesh);
        result_polygons.add(generated_result_polygons);
        result_lines.add(generated_result_lines);
    }
    //scripta::log("infill_result_polygons_0", result_polygons, section_type, layer_idx);
    //scripta::log("infill_result_lines_0", result_lines, section_type, layer_idx);
    //scripta::log("infill_toolpaths_0", toolpaths, section_type, layer_idx,
    //    scripta::CellVDI{ "is_closed", &ExtrusionLine::is_closed },
    //    scripta::CellVDI{ "is_odd", &ExtrusionLine::is_odd },
    //    scripta::CellVDI{ "inset_idx", &ExtrusionLine::inset_idx },
    //    scripta::PointVDI{ "width", &ExtrusionJunction::w },
    //    scripta::PointVDI{ "perimeter_index", &ExtrusionJunction::perimeter_index });
    if (connect_polygons)
    {
        // remove too small polygons
        coord_t snap_distance = infill_line_width * 2; // polygons with a span of max 1 * nozzle_size are too small
        auto it = std::remove_if(result_polygons.begin(), result_polygons.end(), [snap_distance](PolygonRef poly) { return poly.shorterThan(snap_distance); });
        result_polygons.erase(it, result_polygons.end());

        PolygonConnector connector(infill_line_width);
        connector.add(result_polygons);
        connector.add(toolpaths);
        Polygons connected_polygons;
        std::vector<VariableWidthLines> connected_paths;
        connector.connect(connected_polygons, connected_paths);
        result_polygons = connected_polygons;
        toolpaths = connected_paths;
        //scripta::log("infill_result_polygons_1", result_polygons, section_type, layer_idx);
        //scripta::log("infill_result_lines_1", result_lines, section_type, layer_idx);
        //scripta::log("infill_toolpaths_1", toolpaths, section_type, layer_idx,
        //    scripta::CellVDI{ "is_closed", &ExtrusionLine::is_closed },
        //    scripta::CellVDI{ "is_odd", &ExtrusionLine::is_odd },
        //    scripta::CellVDI{ "inset_idx", &ExtrusionLine::inset_idx },
        //    scripta::PointVDI{ "width", &ExtrusionJunction::w },
        //    scripta::PointVDI{ "perimeter_index", &ExtrusionJunction::perimeter_index });
    }
}

void Infill::_generate(std::vector<VariableWidthLines>& toolpaths,
                       Polygons& result_polygons,
                       Polygons& result_lines,
                       const Settings& settings,
                       const SierpinskiFillProvider* cross_fill_provider,
                       const LightningLayer* lightning_trees,
                       const SliceMeshStorage* mesh)
{
    if (inner_contour.empty())
        return;
    if (line_distance == 0)
        return;

    switch (pattern)
    {
    case EFillMethod::GRID:
        generateGridInfill(result_lines);
        break;
    case EFillMethod::LINES:
        generateLineInfill(result_lines, line_distance, fill_angle, 0);
        break;
    case EFillMethod::CUBIC:
        generateCubicInfill(result_lines);
        break;
    case EFillMethod::TETRAHEDRAL:
        generateTetrahedralInfill(result_lines);
        break;
    case EFillMethod::QUARTER_CUBIC:
        generateQuarterCubicInfill(result_lines);
        break;
    case EFillMethod::TRIANGLES:
        generateTriangleInfill(result_lines);
        break;
    case EFillMethod::TRIHEXAGON:
        generateTrihexagonInfill(result_lines);
        break;
    case EFillMethod::CONCENTRIC:
        generateConcentricInfill(toolpaths, settings);
        break;
	case EFillMethod::HONEYCOMB:
		generateHoneycombInfill(result_polygons, line_distance);
		break;
    case EFillMethod::ZIG_ZAG:
        generateZigZagInfill(result_lines, line_distance, fill_angle);
        break;
	case EFillMethod::ALIGNLINES:
		generateZigZagInfill(result_lines, line_distance, 45.0);
		break;
    case EFillMethod::CUBICSUBDIV:
        if (! mesh)
        {
            LOGE("Cannot generate Cubic Subdivision infill without a mesh!");
            break;
        }
        generateCubicSubDivInfill(result_lines, *mesh);
        break;
    case EFillMethod::CROSS:
    case EFillMethod::CROSS_3D:
        if (! cross_fill_provider)
        {
            LOGE("Cannot generate Cross infill without a cross fill provider!\n");
            break;
        }
        generateCrossInfill(*cross_fill_provider, result_polygons, result_lines);
        break;
    case EFillMethod::GYROID:
        generateGyroidInfill(result_lines, result_polygons);
        break;
    case EFillMethod::LIGHTNING:
        assert(lightning_trees); // "Cannot generate Lightning infill without a generator!\n"
        generateLightningInfill(lightning_trees, result_lines);
        break;
    default:
        LOGE("Fill pattern has unknown value.\n");
        break;
    }

    if (connect_lines)
    {
        // The list should be empty because it will be again filled completely. Otherwise might have double lines.
        assert(result_lines.empty());
        result_lines.clear();
        connectLines(result_lines);
    }

    Simplify simplifier(max_resolution, max_deviation, 0);
    result_polygons = simplifier.polygon(result_polygons);

    if (! skip_line_stitching && (zig_zaggify || pattern == EFillMethod::CROSS || pattern == EFillMethod::CROSS_3D || pattern == EFillMethod::CUBICSUBDIV || pattern == EFillMethod::GYROID || pattern == EFillMethod::ZIG_ZAG))
    { // don't stich for non-zig-zagged line infill types
        Polygons stitched_lines;
        PolylineStitcher<Polygons, Polygon, Point>::stitch(result_lines, stitched_lines, result_polygons, infill_line_width);
        result_lines = stitched_lines;
    }
    result_lines = simplifier.polyline(result_lines);
}

void Infill::multiplyInfill(Polygons& result_polygons, Polygons& result_lines)
{
    if (pattern == EFillMethod::CONCENTRIC)
    {
        result_polygons = result_polygons.processEvenOdd(); // make into areas
    }

    bool odd_multiplier = infill_multiplier % 2 == 1;
    coord_t offset = (odd_multiplier) ? infill_line_width : infill_line_width / 2;

    // Get the first offset these are mirrored from the original center line
    Polygons result;
    Polygons first_offset;
    {
        const Polygons first_offset_lines = result_lines.offsetPolyLine(offset); // make lines on both sides of the input lines
        const Polygons first_offset_polygons_inward = result_polygons.offset(-offset); // make lines on the inside of the input polygons
        const Polygons first_offset_polygons_outward = result_polygons.offset(offset); // make lines on the other side of the input polygons
        const Polygons first_offset_polygons = first_offset_polygons_outward.difference(first_offset_polygons_inward);
        first_offset = first_offset_lines.unionPolygons(first_offset_polygons); // usually we only have either lines or polygons, but this code also handles an infill pattern which generates both
        if (zig_zaggify)
        {
            first_offset = inner_contour.difference(first_offset);
        }
    }
    result.add(first_offset);

    // Create the additional offsets from the first offsets, generated earlier, the direction of these offsets is
    // depended on whether these lines should be connected or not.
    if (infill_multiplier > 3)
    {
        Polygons reference_polygons = first_offset;
        const size_t multiplier = static_cast<size_t>(infill_multiplier / 2);

        const int extra_offset = mirror_offset ? -infill_line_width : infill_line_width;
        for (size_t infill_line = 1; infill_line < multiplier; ++infill_line)
        {
            Polygons extra_polys = reference_polygons.offset(extra_offset);
            result.add(extra_polys);
            reference_polygons = std::move(extra_polys);
        }
    }
    if (zig_zaggify)
    {
        result = result.intersection(inner_contour);
    }

    // Remove the original center lines when there are an even number of lines required.
    if (! odd_multiplier)
    {
        result_polygons.clear();
        result_lines.clear();
    }
    result_polygons.add(result);
    if (! zig_zaggify)
    {
        for (PolygonRef poly : result_polygons)
        { // make polygons into polylines
            if (poly.empty())
            {
                continue;
            }
            poly.add(poly[0]);
        }
        Polygons polylines = inner_contour.intersectionPolyLines(result_polygons);
        result_polygons.clear();
        PolylineStitcher<Polygons, Polygon, Point>::stitch(polylines, result_lines, result_polygons, infill_line_width);
    }
}

void Infill::generateGyroidInfill(Polygons& result_lines, Polygons& result_polygons)
{
    Polygons line_segments;
    GyroidInfill::generateTotalGyroidInfill(line_segments, zig_zaggify, line_distance, inner_contour, z);
    PolylineStitcher<Polygons, Polygon, Point>::stitch(line_segments, result_lines, result_polygons, infill_line_width);
}

void Infill::generateLightningInfill(const LightningLayer* trees, Polygons& result_lines)
{
    // Don't need to support areas smaller than line width, as they are always within radius:
    if (std::abs(inner_contour.area()) < infill_line_width || ! trees)
    {
        return;
    }
    result_lines.add(trees->convertToLines(inner_contour, infill_line_width));
}

void Infill::generateConcentricInfill(std::vector<VariableWidthLines>& toolpaths, const Settings& settings)
{
    const coord_t min_area = infill_line_width * infill_line_width;

    Polygons current_inset = inner_contour;

    coord_t max_resolution = settings.get<coord_t>("meshfix_maximum_resolution");
    coord_t max_deviation = settings.get<coord_t>("meshfix_maximum_deviation");
    coord_t max_area_deviation = settings.get<coord_t>("meshfix_maximum_extrusion_area_deviation");

    Simplify simplifier(max_resolution, max_deviation, max_area_deviation);
    while (true)
    {
        // If line_distance is 0, start from the same contour as the previous line, except where the previous line closed up the shape.
        // So we add the whole nominal line width first (to allow lines to be closer together than 1 line width if the line distance is smaller) and then subtract line_distance.
        current_inset = current_inset.offset(infill_line_width - line_distance);
        current_inset = simplifier.polygon(current_inset); // Many insets lead to increasingly detailed shapes. Simplify to speed up processing.
        if (current_inset.area() < min_area) // So small that it's inconsequential. Stop here.
        {
            break;
        }

        constexpr size_t inset_wall_count = 1; // 1 wall at a time.
        constexpr coord_t wall_0_inset = 0; // Don't apply any outer wall inset for these. That's just for the outer wall.
        WallToolPaths wall_toolpaths(current_inset, infill_line_width, inset_wall_count, wall_0_inset, settings);
        const std::vector<VariableWidthLines> inset_paths = wall_toolpaths.getToolPaths();
        toolpaths.insert(toolpaths.end(), inset_paths.begin(), inset_paths.end());

        current_inset = wall_toolpaths.getInnerContour();
    }
}


struct CacheID
{
	CacheID(float adensity, double aspacing) :
		density(adensity), spacing(aspacing) {}
	float		density;
	double	spacing;
	bool operator<(const CacheID& other) const
	{
		return (density < other.density) || (density == other.density && spacing < other.spacing);
	}
	bool operator==(const CacheID& other) const
	{
		return density == other.density && spacing == other.spacing;
	}
};
struct CacheData
{
	coord_t	distance;
	coord_t hex_side;
	coord_t hex_width;
	coord_t	pattern_height;
	coord_t y_short;
	coord_t x_offset;
	coord_t	y_offset;
	Point	hex_center;
};

typedef std::map<CacheID, CacheData> Cache;
Cache cache;

// Align a coordinate to a grid. The coordinate may be negative,
// the aligned value will never be bigger than the original one.
static coord_t _align_to_grid(const coord_t coord, const coord_t spacing) {
	// Current C++ standard defines the result of integer division to be rounded to zero,
	// for both positive and negative numbers. Here we want to round down for negative
	// numbers as well.
	coord_t aligned = (coord < 0) ?
		((coord - spacing + 1) / spacing) * spacing :
		(coord / spacing) * spacing;
	assert(aligned <= coord);
	return aligned;
}

static Point   _align_to_grid(Point   coord, Point   spacing)
{
	return Point(_align_to_grid(coord.X, spacing.X), _align_to_grid(coord.Y, spacing.Y));
}
static coord_t _align_to_grid(coord_t coord, coord_t spacing, coord_t base)
{
	return base + _align_to_grid(coord - base, spacing);
}
static Point   _align_to_grid(Point   coord, Point   spacing, Point   base)
{
	return Point(_align_to_grid(coord.X, spacing.X, base.X), _align_to_grid(coord.Y, spacing.Y, base.Y));
}
void Infill::generateHoneycombInfill(Polygons& result, int _line_distance)
{
	// cache hexagons math
	CacheID cache_id(0.1, _line_distance);
	Cache::iterator it_m = cache.find(cache_id);
	if (it_m == cache.end())
	{
		it_m = cache.insert(it_m, std::pair<CacheID, CacheData>(cache_id, CacheData()));
		CacheData& m = it_m->second;
		coord_t min_spacing = _line_distance * 0.5;//coord_t(scale_(_line_distance));
		m.distance = _line_distance * 0.5;//coord_t(min_spacing / params.density);
		m.hex_side = coord_t(m.distance / (sqrt(3) / 2));
		m.hex_width = m.distance * 2; // $m->{hex_width} == $m->{hex_side} * sqrt(3);
		coord_t hex_height = m.hex_side * 2;
		m.pattern_height = hex_height + m.hex_side;
		m.y_short = coord_t(m.distance * sqrt(3) / 3);
		m.x_offset = infill_line_width / 2;//min_spacing / 2;
		m.y_offset = coord_t(m.x_offset * sqrt(3) / 3);
		m.hex_center = Point(m.hex_width / 2, m.hex_side);
	}
	CacheData& m = it_m->second;

	infill_overlap;
	Polygons outline = outer_contour.offset(-infill_line_width / 2 + infill_overlap);
	Polygons all_polylines;
    if (outline.size()>0)
	{
		AABB bounding_box;
		bounding_box.calculate(outline);
		//
		Point aPoint = _align_to_grid(bounding_box.min, Point(m.hex_width, m.pattern_height));
		bounding_box.include(aPoint);

		coord_t x = bounding_box.min.X;
		while (x <= bounding_box.max.X)
		{
			Polygon p;
			coord_t ax[2] = { x + infill_line_width / 2, x + m.distance }; //{ x + infill_line_width, x + m.distance};
			for (size_t i = 0; i < 2; ++i)
			{
				std::reverse(p.begin(), p.end()); // turn first half upside down
				for (coord_t y = bounding_box.min.Y; y <= bounding_box.max.Y; y += m.y_short + m.hex_side + m.y_short + m.hex_side)
				{
					p.add(Point(ax[1], y + m.y_offset));
					p.add(Point(ax[0], y + m.y_short - m.y_offset));
					p.add(Point(ax[0], y + m.y_short + m.hex_side + m.y_offset));
					p.add(Point(ax[1], y + m.y_short + m.hex_side + m.y_short - m.y_offset));
					p.add(Point(ax[1], y + m.y_short + m.hex_side + m.y_short + m.hex_side + m.y_offset));
				}
				ax[0] = ax[0] + m.distance;
				ax[1] = ax[1] + m.distance;
				std::swap(ax[0], ax[1]); // draw symmetrical pattern
				x += m.distance;
			}
			//p.rotate(-direction.first, m.hex_center);
			all_polylines.add(p);
		}
	}

	//TEST
	//ClipperLib::save(all_polylines.getPaths(), "infill.path");
	//ClipperLib::save(outline.getPaths(), "outline.path");

	// init Clipper
	ClipperLib::Clipper clipper;
	clipper.Clear();
	// add polygons
    ClipperLib::Paths all_paths;
    for (int n=0;n<all_polylines.size();n++)
    {
        all_paths.push_back(*all_polylines[n]);
    }
    ClipperLib::Paths outline_paths;
	for (int n = 0; n < outline.size(); n++)
	{
        outline_paths.push_back(*outline[n]);
	}

	clipper.AddPaths(all_paths, ClipperLib::ptSubject, true);
	clipper.AddPaths(outline_paths, ClipperLib::ptClip, true);
	// perform operation
	ClipperLib::PolyTree retval;
	ClipperLib::Paths resultPaths;
	clipper.Execute(ClipperLib::ClipType::ctIntersection, retval, ClipperLib::pftEvenOdd, ClipperLib::pftEvenOdd);
	ClipperLib::PolyTreeToPaths(retval, resultPaths);

	ClipperLib::Paths resultPaths2;
	for (ClipperLib::Path aPath : resultPaths)
	{
		if (aPath.size() > 2)
		{
			resultPaths2.push_back(aPath);
		}
	}

    for (ClipperLib::Path& apath: resultPaths2)
    {
        result.add(apath);
    }
}

void Infill::generateGridInfill(Polygons& result)
{
    generateLineInfill(result, line_distance, fill_angle, 0);
    generateLineInfill(result, line_distance, fill_angle + 90, 0);
}

void Infill::generateCubicInfill(Polygons& result)
{
    const coord_t shift = one_over_sqrt_2 * z;
    generateLineInfill(result, line_distance, fill_angle, shift);
    generateLineInfill(result, line_distance, fill_angle + 120, shift);
    generateLineInfill(result, line_distance, fill_angle + 240, shift);
}

void Infill::generateTetrahedralInfill(Polygons& result)
{
    generateHalfTetrahedralInfill(0.0, 0, result);
    generateHalfTetrahedralInfill(0.0, 90, result);
}

void Infill::generateQuarterCubicInfill(Polygons& result)
{
    generateHalfTetrahedralInfill(0.0, 0, result);
    generateHalfTetrahedralInfill(0.5, 90, result);
}

void Infill::generateHalfTetrahedralInfill(float pattern_z_shift, int angle_shift, Polygons& result)
{
    const coord_t period = line_distance * 2;
    coord_t shift = coord_t(one_over_sqrt_2 * (z + pattern_z_shift * period * 2)) % period;
    shift = std::min(shift, period - shift); // symmetry due to the fact that we are applying the shift in both directions
    shift = std::min(shift, period / 2 - infill_line_width / 2); // don't put lines too close to each other
    shift = std::max(shift, infill_line_width / 2); // don't put lines too close to each other
    generateLineInfill(result, period, fill_angle + angle_shift, shift);
    generateLineInfill(result, period, fill_angle + angle_shift, -shift);
}

void Infill::generateTriangleInfill(Polygons& result)
{
    generateLineInfill(result, line_distance, fill_angle, 0);
    generateLineInfill(result, line_distance, fill_angle + 60, 0);
    generateLineInfill(result, line_distance, fill_angle + 120, 0);
}

void Infill::generateTrihexagonInfill(Polygons& result)
{
    generateLineInfill(result, line_distance, fill_angle, 0);
    generateLineInfill(result, line_distance, fill_angle + 60, 0);
    generateLineInfill(result, line_distance, fill_angle + 120, line_distance / 2);
}

void Infill::generateCubicSubDivInfill(Polygons& result, const SliceMeshStorage& mesh)
{
    Polygons uncropped;
    mesh.base_subdiv_cube->generateSubdivisionLines(z, uncropped);
    constexpr bool restitch = false; // cubic subdivision lines are always single line segments - not polylines consisting of multiple segments.
    result = outer_contour.offset(infill_overlap).intersectionPolyLines(uncropped, restitch);
}

void Infill::generateCrossInfill(const SierpinskiFillProvider& cross_fill_provider, Polygons& result_polygons, Polygons& result_lines)
{
    Polygon cross_pattern_polygon = cross_fill_provider.generate(pattern, z, infill_line_width, pocket_size);

    if (cross_pattern_polygon.empty())
    {
        return;
    }

    if (zig_zaggify)
    {
        Polygons cross_pattern_polygons;
        cross_pattern_polygons.add(cross_pattern_polygon);
        result_polygons.add(inner_contour.intersection(cross_pattern_polygons));
    }
    else
    {
        // make the polyline closed in order to handle cross_pattern_polygon as a polyline, rather than a closed polygon
        cross_pattern_polygon.add(cross_pattern_polygon[0]);

        Polygons cross_pattern_polylines;
        cross_pattern_polylines.add(cross_pattern_polygon);
        Polygons poly_lines = inner_contour.intersectionPolyLines(cross_pattern_polylines);
        PolylineStitcher<Polygons, Polygon, Point>::stitch(poly_lines, result_lines, result_polygons, infill_line_width);
    }
}

void Infill::addLineInfill(Polygons& result, const PointMatrix& rotation_matrix, const int scanline_min_idx, const int line_distance, const AABB boundary, std::vector<std::vector<coord_t>>& cut_list, coord_t shift)
{
    assert(! connect_lines && "connectLines() should add the infill lines, not addLineInfill");

    unsigned int scanline_idx = 0;
    for (coord_t x = scanline_min_idx * line_distance + shift; x < boundary.max.X; x += line_distance)
    {
        if (scanline_idx >= cut_list.size())
        {
            break;
        }
        std::vector<coord_t>& crossings = cut_list[scanline_idx];
        std::sort(crossings.begin(), crossings.end()); // sort by increasing Y coordinates
        for (unsigned int crossing_idx = 0; crossing_idx + 1 < crossings.size(); crossing_idx += 2)
        {
            if (crossings[crossing_idx + 1] - crossings[crossing_idx] < infill_line_width / 5)
            { // segment is too short to create infill
                continue;
            }
            result.addLine(rotation_matrix.unapply(Point(x, crossings[crossing_idx])), rotation_matrix.unapply(Point(x, crossings[crossing_idx + 1])));
        }
        scanline_idx += 1;
    }
}

coord_t Infill::getShiftOffsetFromInfillOriginAndRotation(const double& infill_rotation)
{
    if (infill_origin.X != 0 || infill_origin.Y != 0)
    {
        const double rotation_rads = infill_rotation * M_PI / 180;
        return infill_origin.X * std::cos(rotation_rads) - infill_origin.Y * std::sin(rotation_rads);
    }
    return 0;
}

void Infill::generateLineInfill(Polygons& result, int line_distance, const double& infill_rotation, coord_t shift)
{
    shift += getShiftOffsetFromInfillOriginAndRotation(infill_rotation);
    PointMatrix rotation_matrix(infill_rotation);
    NoZigZagConnectorProcessor lines_processor(rotation_matrix, result);
    bool connected_zigzags = false;
    generateLinearBasedInfill(result, line_distance, rotation_matrix, lines_processor, connected_zigzags, shift);
}


void Infill::generateZigZagInfill(Polygons& result, const coord_t line_distance, const double& infill_rotation)
{
    const coord_t shift = getShiftOffsetFromInfillOriginAndRotation(infill_rotation);

    PointMatrix rotation_matrix(infill_rotation);
    ZigzagConnectorProcessor zigzag_processor(rotation_matrix, result, use_endpieces, connected_zigzags, skip_some_zags, zag_skip_count);
    generateLinearBasedInfill(result, line_distance, rotation_matrix, zigzag_processor, connected_zigzags, shift);
}

/*
 * algorithm:
 * 1. for each line segment of each polygon:
 *      store the intersections of that line segment with all scanlines in a mapping (vector of vectors) from scanline to intersections
 *      (zigzag): add boundary segments to result
 * 2. for each scanline:
 *      sort the associated intersections
 *      and connect them using the even-odd rule
 *
 * rough explanation of the zigzag algorithm:
 * while walking around (each) polygon (1.)
 *  if polygon intersects with even scanline
 *      start boundary segment (add each following segment to the [result])
 *  when polygon intersects with a scanline again
 *      stop boundary segment (stop adding segments to the [result])
 *  (see infill/ZigzagConnectorProcessor.h for actual implementation details)
 *
 *
 * we call the areas between two consecutive scanlines a 'scansegment'.
 * Scansegment x is the area between scanline x and scanline x+1
 * Edit: the term scansegment is wrong, since I call a boundary segment leaving from an even scanline to the left as belonging to an even scansegment,
 *  while I also call a boundary segment leaving from an even scanline toward the right as belonging to an even scansegment.
 */
void Infill::generateLinearBasedInfill(Polygons& result, const int line_distance, const PointMatrix& rotation_matrix, ZigzagConnectorProcessor& zigzag_connector_processor, const bool connected_zigzags, coord_t extra_shift)
{
    if (line_distance == 0 || inner_contour.empty()) // No infill to generate (0% density) or no area to generate it in.
    {
        return;
    }

    Polygons outline = inner_contour; // Make a copy. We'll be rotating this outline to make intersections always horizontal, for better performance.
    outline.applyMatrix(rotation_matrix);

    coord_t shift = extra_shift + this->shift;
    if (shift < 0)
    {
        shift = line_distance - (-shift) % line_distance;
    }
    else
    {
        shift = shift % line_distance;
    }

    AABB boundary(outline);

    int scanline_min_idx = computeScanSegmentIdx(boundary.min.X - shift, line_distance);
    int line_count = computeScanSegmentIdx(boundary.max.X - shift, line_distance) + 1 - scanline_min_idx;

    std::vector<std::vector<coord_t>> cut_list(line_count); // mapping from scanline to all intersections with polygon segments

    // When we find crossings, keep track of which crossing belongs to which scanline and to which polygon line segment.
    // Then we can later join two crossings together to form lines and still know what polygon line segments that infill line connected to.
    struct Crossing
    {
        Crossing(Point coordinate, size_t polygon_index, size_t vertex_index) : coordinate(coordinate), polygon_index(polygon_index), vertex_index(vertex_index){};
        Point coordinate;
        size_t polygon_index;
        size_t vertex_index;
        bool operator<(const Crossing& other) const // Crossings will be ordered by their Y coordinate so that they get ordered along the scanline.
        {
            return coordinate.Y < other.coordinate.Y;
        }
    };
    std::vector<std::vector<Crossing>> crossings_per_scanline; // For each scanline, a list of crossings.
    const int min_scanline_index = computeScanSegmentIdx(boundary.min.X - shift, line_distance) + 1;
    const int max_scanline_index = computeScanSegmentIdx(boundary.max.X - shift, line_distance) + 1;
    crossings_per_scanline.resize(max_scanline_index - min_scanline_index);
    if (connect_lines)
    {
        crossings_on_line.resize(outline.size()); // One for each polygon.
    }

    auto getStartPointAndIdx = [](const PolygonRef& poly, const Point prePoint, Point& p0)
    {
        size_t startIdx = 0;
        if (prePoint != Point())
        {
            p0 = poly.closestPointTo(prePoint);
            for (size_t point_idx = 0; point_idx < poly.size(); point_idx++)
            {
                if (poly[point_idx] == p0)
                    startIdx = (point_idx + 1) % poly.size();
            }
        }
        return startIdx;
    };

    for (size_t poly_idx = 0; poly_idx < outline.size(); poly_idx++)
    {
        PolygonRef poly = outline[poly_idx];
        if (connect_lines)
        {
            crossings_on_line[poly_idx].resize(poly.size()); // One for each line in this polygon.
        }
        Point p0 = poly.back();
        size_t startIdx = getStartPointAndIdx(poly, rotation_matrix.apply(current_position), p0);
        zigzag_connector_processor.registerVertex(p0); // always adds the first point to ZigzagConnectorProcessorEndPieces::first_zigzag_connector when using a zigzag infill type

        for (size_t i = startIdx; i < poly.size() + startIdx; i++)
        {
            size_t point_idx = i % poly.size();
            Point p1 = poly[point_idx];
            if (p1.X == p0.X)
            {
                zigzag_connector_processor.registerVertex(p1);
                // TODO: how to make sure it always adds the shortest line? (in order to prevent overlap with the zigzag connectors)
                // note: this is already a problem for normal infill, but hasn't really bothered anyone so far.
                p0 = p1;
                continue;
            }

            int scanline_idx0;
            int scanline_idx1;
            // this way of handling the indices takes care of the case where a boundary line segment ends exactly on a scanline:
            // in case the next segment moves back from that scanline either 2 or 0 scanline-boundary intersections are created
            // otherwise only 1 will be created, counting as an actual intersection
            int direction = 1;
            if (p0.X < p1.X)
            {
                scanline_idx0 = computeScanSegmentIdx(p0.X - shift, line_distance) + 1; // + 1 cause we don't cross the scanline of the first scan segment
                scanline_idx1 = computeScanSegmentIdx(p1.X - shift, line_distance); // -1 cause the vertex point is handled in the next segment (or not in the case which looks like >)
            }
            else
            {
                direction = -1;
                scanline_idx0 = computeScanSegmentIdx(p0.X - shift, line_distance); // -1 cause the vertex point is handled in the previous segment (or not in the case which looks like >)
                scanline_idx1 = computeScanSegmentIdx(p1.X - shift, line_distance) + 1; // + 1 cause we don't cross the scanline of the first scan segment
            }

            for (int scanline_idx = scanline_idx0; scanline_idx != scanline_idx1 + direction; scanline_idx += direction)
            {
                int x = scanline_idx * line_distance + shift;
                int y = p1.Y + (p0.Y - p1.Y) * (x - p1.X) / (p0.X - p1.X);
                assert(scanline_idx - scanline_min_idx >= 0 && scanline_idx - scanline_min_idx < int(cut_list.size()) && "reading infill cutlist index out of bounds!");
                cut_list[scanline_idx - scanline_min_idx].push_back(y);
                Point scanline_linesegment_intersection(x, y);
                zigzag_connector_processor.registerScanlineSegmentIntersection(scanline_linesegment_intersection, scanline_idx);
                crossings_per_scanline[scanline_idx - min_scanline_index].emplace_back(scanline_linesegment_intersection, poly_idx, point_idx);
            }
            zigzag_connector_processor.registerVertex(p1);
            p0 = p1;
        }
        zigzag_connector_processor.registerPolyFinished();
    }

    if (connect_lines)
    {
        // Gather all crossings per scanline and find out which crossings belong together, then store them in crossings_on_line.
        for (int scanline_index = min_scanline_index; scanline_index < max_scanline_index; scanline_index++)
        {
            auto& crossings = crossings_per_scanline[scanline_index - min_scanline_index];
            // Sorts them by Y coordinate.
            std::sort(crossings.begin(), crossings.end());
            // Combine each 2 subsequent crossings together.
            for (long crossing_index = 0; crossing_index < static_cast<long>(crossings.size()) - 1; crossing_index += 2)
            {
                const Crossing& first = crossings[crossing_index];
                const Crossing& second = crossings[crossing_index + 1];
                // Avoid creating zero length crossing lines
                const Point unrotated_first = rotation_matrix.unapply(first.coordinate);
                const Point unrotated_second = rotation_matrix.unapply(second.coordinate);
                if (unrotated_first == unrotated_second)
                {
                    continue;
                }
                InfillLineSegment* new_segment = new InfillLineSegment(unrotated_first, first.vertex_index, first.polygon_index, unrotated_second, second.vertex_index, second.polygon_index);
                // Put the same line segment in the data structure twice: Once for each of the polygon line segment that it crosses.
                crossings_on_line[first.polygon_index][first.vertex_index].push_back(new_segment);
                crossings_on_line[second.polygon_index][second.vertex_index].push_back(new_segment);
            }
        }
    }
    else
    {
        if (cut_list.size() == 0)
        {
            return;
        }
        if (connected_zigzags && cut_list.size() == 1 && cut_list[0].size() <= 2)
        {
            return; // don't add connection if boundary already contains whole outline!
        }

        // We have to create our own lines when they are not created by the method connectLines.
        addLineInfill(result, rotation_matrix, scanline_min_idx, line_distance, boundary, cut_list, shift);
    }
}

void Infill::connectLines(Polygons& result_lines)
{
    UnionFind<InfillLineSegment*> connected_lines; // Keeps track of which lines are connected to which.
    for (const std::vector<std::vector<InfillLineSegment*>>& crossings_on_polygon : crossings_on_line)
    {
        for (const std::vector<InfillLineSegment*>& crossings_on_polygon_segment : crossings_on_polygon)
        {
            for (InfillLineSegment* infill_line : crossings_on_polygon_segment)
            {
                if (connected_lines.find(infill_line) == (size_t)-1)
                {
                    connected_lines.add(infill_line); // Put every line in there as a separate set.
                }
            }
        }
    }

    auto getStartPointAndIdx = [](const ConstPolygonRef& poly, const Point prePoint, Point& p0)
    {
        size_t startIdx = 0;
        if (prePoint != Point())
        {
            p0 = poly.closestPointTo(prePoint);
            for (size_t point_idx = 0; point_idx < poly.size(); point_idx++)
            {
                if (poly[point_idx] == p0)
                    startIdx = (point_idx + 1) % poly.size();
            }
        }
        return startIdx;
    };

    for (size_t polygon_index = 0; polygon_index < inner_contour.size(); polygon_index++)
    {
        ConstPolygonRef inner_contour_polygon = inner_contour[polygon_index];
        if (inner_contour_polygon.empty())
        {
            continue;
        }
        assert(crossings_on_line.size() > polygon_index && "crossings dimension should be bigger then polygon index");
        std::vector<std::vector<InfillLineSegment*>>& crossings_on_polygon = crossings_on_line[polygon_index];
        InfillLineSegment* previous_crossing = nullptr; // The crossing that we should connect to. If nullptr, we have been skipping until we find the next crossing.
        InfillLineSegment* previous_segment = nullptr; // The last segment we were connecting while drawing a line along the border.
        Point vertex_before = inner_contour_polygon.back();
        size_t startIdx = getStartPointAndIdx(inner_contour_polygon, current_position, vertex_before);
        for (size_t i = startIdx; i < inner_contour_polygon.size() + startIdx; i++)
        {
            size_t vertex_index = i % inner_contour_polygon.size();
            assert(crossings_on_polygon.size() > vertex_index && "crossings on line for the current polygon should be bigger then vertex index");
            std::vector<InfillLineSegment*>& crossings_on_polygon_segment = crossings_on_polygon[vertex_index];
            Point vertex_after = inner_contour_polygon[vertex_index];

            // Sort crossings on every line by how far they are from their initial point.
            std::sort(crossings_on_polygon_segment.begin(),
                      crossings_on_polygon_segment.end(),
                      [&vertex_before, polygon_index, vertex_index](InfillLineSegment* left_hand_side, InfillLineSegment* right_hand_side)
                      {
                          // Find the two endpoints that are relevant.
                          const Point left_hand_point = (left_hand_side->start_segment == vertex_index && left_hand_side->start_polygon == polygon_index) ? left_hand_side->start : left_hand_side->end;
                          const Point right_hand_point = (right_hand_side->start_segment == vertex_index && right_hand_side->start_polygon == polygon_index) ? right_hand_side->start : right_hand_side->end;
                          return vSize(left_hand_point - vertex_before) < vSize(right_hand_point - vertex_before);
                      });

            for (InfillLineSegment* crossing : crossings_on_polygon_segment)
            {
                if (! previous_crossing) // If we're not yet drawing, then we have been trying to find the next vertex. We found it! Let's start drawing.
                {
                    previous_crossing = crossing;
                    previous_segment = crossing;
                }
                else
                {
                    const size_t crossing_handle = connected_lines.find(crossing);
                    assert(crossing_handle != (size_t)-1);
                    const size_t previous_crossing_handle = connected_lines.find(previous_crossing);
                    assert(previous_crossing_handle != (size_t)-1);
                    if (crossing_handle == previous_crossing_handle) // These two infill lines are already connected. Don't create a loop now. Continue connecting with the next crossing.
                    {
                        continue;
                    }

                    // Join two infill lines together with a connecting line.
                    // Here the InfillLineSegments function as a linked list, so that they can easily be joined.
                    const Point previous_point = (previous_segment->start_segment == vertex_index && previous_segment->start_polygon == polygon_index) ? previous_segment->start : previous_segment->end;
                    const Point next_point = (crossing->start_segment == vertex_index && crossing->start_polygon == polygon_index) ? crossing->start : crossing->end;
                    InfillLineSegment* new_segment;
                    // If the segment is zero length, we avoid creating it but still want to connect the crossing with the previous segment
                    if (previous_point == next_point)
                    {
                        if (previous_segment->start_segment == vertex_index && previous_segment->start_polygon == polygon_index)
                        {
                            previous_segment->previous = crossing;
                        }
                        else
                        {
                            previous_segment->next = crossing;
                        }
                        new_segment = previous_segment;
                    }
                    else
                    {
                        new_segment = new InfillLineSegment(previous_point, vertex_index, polygon_index, next_point, vertex_index, polygon_index); // A connecting line between them.
                        new_segment->previous = previous_segment;
                        if (previous_segment->start_segment == vertex_index && previous_segment->start_polygon == polygon_index)
                        {
                            previous_segment->previous = new_segment;
                        }
                        else
                        {
                            previous_segment->next = new_segment;
                        }
                        new_segment->next = crossing;
                    }

                    if (crossing->start_segment == vertex_index && crossing->start_polygon == polygon_index)
                    {
                        crossing->previous = new_segment;
                    }
                    else
                    {
                        crossing->next = new_segment;
                    }
                    connected_lines.unite(crossing_handle, previous_crossing_handle);
                    previous_crossing = nullptr;
                    previous_segment = nullptr;
                }
            }

            // Upon going to the next vertex, if we're drawing, put an extra vertex in our infill lines.
            if (previous_crossing)
            {
                InfillLineSegment* new_segment;
                if (vertex_index == previous_segment->start_segment && polygon_index == previous_segment->start_polygon)
                {
                    if (previous_segment->start == vertex_after)
                    {
                        // Edge case when an infill line ends directly on top of vertex_after: We skip the extra connecting line segment, as that would be 0-length.
                        previous_segment = nullptr;
                        previous_crossing = nullptr;
                    }
                    else
                    {
                        new_segment = new InfillLineSegment(previous_segment->start, vertex_index, polygon_index, vertex_after, (vertex_index + 1) % inner_contour[polygon_index].size(), polygon_index);
                        previous_segment->previous = new_segment;
                        new_segment->previous = previous_segment;
                        previous_segment = new_segment;
                    }
                }
                else
                {
                    if (previous_segment->end == vertex_after)
                    {
                        // Edge case when an infill line ends directly on top of vertex_after: We skip the extra connecting line segment, as that would be 0-length.
                        previous_segment = nullptr;
                        previous_crossing = nullptr;
                    }
                    else
                    {
                        new_segment = new InfillLineSegment(previous_segment->end, vertex_index, polygon_index, vertex_after, (vertex_index + 1) % inner_contour[polygon_index].size(), polygon_index);
                        previous_segment->next = new_segment;
                        new_segment->previous = previous_segment;
                        previous_segment = new_segment;
                    }
                }
            }

            vertex_before = vertex_after;
            crossings_on_polygon_segment.clear();
        }
    }

    // Save all lines, now connected, to the output.
    std::unordered_set<size_t> completed_groups;
    for (InfillLineSegment* infill_line : connected_lines)
    {
        const size_t group = connected_lines.find(infill_line);
        if (completed_groups.find(group) != completed_groups.end()) // We already completed this group.
        {
            continue;
        }

        // Find where the polyline ends by searching through previous and next lines.
        // Note that the "previous" and "next" lines don't necessarily match up though, because the direction while connecting infill lines was not yet known.
        Point previous_vertex = infill_line->start; // Take one side arbitrarily to start from. This variable indicates the vertex that connects to the previous line.
        InfillLineSegment* current_infill_line = infill_line;
        while (current_infill_line->next && current_infill_line->previous) // Until we reached an endpoint.
        {
            const Point next_vertex = (previous_vertex == current_infill_line->start) ? current_infill_line->end : current_infill_line->start;
            current_infill_line = (previous_vertex == current_infill_line->start) ? current_infill_line->next : current_infill_line->previous;
            previous_vertex = next_vertex;
        }

        // Now go along the linked list of infill lines and output the infill lines to the actual result.
        InfillLineSegment* old_line = current_infill_line;
        const Point first_vertex = (! current_infill_line->previous) ? current_infill_line->start : current_infill_line->end;
        previous_vertex = (! current_infill_line->previous) ? current_infill_line->end : current_infill_line->start;
        current_infill_line = (first_vertex == current_infill_line->start) ? current_infill_line->next : current_infill_line->previous;
        PolygonRef result_line = result_lines.newPoly();
        result_line.add(first_vertex);
        result_line.add(previous_vertex);
        delete old_line;
        while (current_infill_line)
        {
            old_line = current_infill_line; // We'll delete this after we've traversed to the next line.
            const Point next_vertex = (previous_vertex == current_infill_line->start) ? current_infill_line->end : current_infill_line->start; // Opposite side of the line.
            current_infill_line = (previous_vertex == current_infill_line->start) ? current_infill_line->next : current_infill_line->previous;
            result_line.add(next_vertex);
            previous_vertex = next_vertex;
            delete old_line;
        }

        completed_groups.insert(group);
    }
}

bool Infill::InfillLineSegment::operator==(const InfillLineSegment& other) const
{
    return start == other.start && end == other.end;
}

} // namespace cura52
