// Copyright (c) 2022 Ultimaker B.V.
// CuraEngine is released under the terms of the AGPLv3 or higher.

#include <algorithm> //For std::partition_copy and std::min_element.
#include <unordered_set>

#include "VariableLineUtils.h"
#include "utils/polygonUtils.h"
#include "utils/PolylineStitcher.h"
#include "utils/Simplify.h"

namespace cura52
{
    void stitchToolPaths(std::vector<VariableWidthLines>& toolpaths, coord_t stitch_distance)
    {
        for (unsigned int wall_idx = 0; wall_idx < toolpaths.size(); wall_idx++)
        {
            VariableWidthLines& wall_lines = toolpaths[wall_idx];

            VariableWidthLines stitched_polylines;
            VariableWidthLines closed_polygons;
            PolylineStitcher<VariableWidthLines, ExtrusionLine, ExtrusionJunction>::stitch(wall_lines, stitched_polylines, closed_polygons, stitch_distance);
            wall_lines = stitched_polylines; // replace input toolpaths with stitched polylines

            for (ExtrusionLine& wall_polygon : closed_polygons)
            {
                if (wall_polygon.junctions.empty())
                {
                    continue;
                }
                // PolylineStitcher, in some cases, produced closed extrusion (polygons),
                // but the endpoints differ by a small distance. So we reconnect them.
                if (wall_polygon.junctions.front().p != wall_polygon.junctions.back().p &&
                    vSize(wall_polygon.junctions.back().p - wall_polygon.junctions.front().p) < stitch_distance) {
                    wall_polygon.junctions.emplace_back(wall_polygon.junctions.front());
                }
                wall_polygon.is_closed = true;
                wall_lines.emplace_back(std::move(wall_polygon)); // add stitched polygons to result
            }
#ifdef DEBUG
            for (ExtrusionLine& line : wall_lines)
            {
                assert(line.inset_idx == wall_idx);
            }
#endif // DEBUG
        }
    }

    void removeSmallLines(std::vector<VariableWidthLines>& toolpaths)
    {
        for (VariableWidthLines& inset : toolpaths)
        {
            for (size_t line_idx = 0; line_idx < inset.size(); line_idx++)
            {
                ExtrusionLine& line = inset[line_idx];
                coord_t min_width = std::numeric_limits<coord_t>::max();
                for (const ExtrusionJunction& j : line)
                {
                    min_width = std::min(min_width, j.w);
                }
                if (line.is_odd && !line.is_closed && shorterThan(line, min_width / 2))
                { // remove line
                    line = std::move(inset.back());
                    inset.erase(--inset.end());
                    line_idx--; // reconsider the current position
                }
            }
        }
    }

    void simplifyToolPaths(std::vector<VariableWidthLines>& toolpaths, coord_t max_resolution, coord_t max_deviation, coord_t max_area_deviation)
    {
        Simplify simplifier(max_resolution, max_deviation, max_area_deviation);
        for (size_t toolpaths_idx = 0; toolpaths_idx < toolpaths.size(); ++toolpaths_idx)
        {
            for (ExtrusionLine& line : toolpaths[toolpaths_idx])
            {
                if (line.is_closed)
                    line = simplifier.polygon(line);
                else
                    line = simplifier.polyline(line);
            }
        }
    }

    Polygons separateOutInnerContour(std::vector<VariableWidthLines>& toolpaths, size_t inset_count)
    {
        assert(inset_count > 0);

        //We'll remove all 0-width paths from the original toolpaths and store them separately as polygons.
        std::vector<VariableWidthLines> actual_toolpaths;
        actual_toolpaths.reserve(toolpaths.size()); //A bit too much, but the correct order of magnitude.
        std::vector<VariableWidthLines> contour_paths;
        contour_paths.reserve(toolpaths.size() / inset_count);
        
        Polygons inner_contour;
        for (const VariableWidthLines& inset : toolpaths)
        {
            if (inset.empty())
            {
                continue;
            }
            bool is_contour = false;
            for (const ExtrusionLine& line : inset)
            {
                for (const ExtrusionJunction& j : line)
                {
                    if (j.w == 0)
                    {
                        is_contour = true;
                    }
                    else
                    {
                        is_contour = false;
                    }
                    break;
                }
            }


            if (is_contour)
            {
#ifdef DEBUG
                for (const ExtrusionLine& line : inset)
                {
                    for (const ExtrusionJunction& j : line)
                    {
                        assert(j.w == 0);
                    }
                }
#endif // DEBUG
                for (const ExtrusionLine& line : inset)
                {
                    if (line.is_odd)
                    {
                        continue; // odd lines don't contribute to the contour
                    }
                    else if (line.is_closed) // sometimes an very small even polygonal wall is not stitched into a polygon
                    {
                        inner_contour.emplace_back(line.toPolygon());
                    }
                }
            }
            else
            {
                actual_toolpaths.emplace_back(inset);
            }
        }
        if (!actual_toolpaths.empty())
        {
            toolpaths = std::move(actual_toolpaths); //Filtered out the 0-width paths.
        }
        else
        {
            toolpaths.clear();
        }

        //The output walls from the skeletal trapezoidation have no known winding order, especially if they are joined together from polylines.
        //They can be in any direction, clockwise or counter-clockwise, regardless of whether the shapes are positive or negative.
        //To get a correct shape, we need to make the outside contour positive and any holes inside negative.
        //This can be done by applying the even-odd rule to the shape. This rule is not sensitive to the winding order of the polygon.
        //The even-odd rule would be incorrect if the polygon self-intersects, but that should never be generated by the skeletal trapezoidation.
        inner_contour = inner_contour.processEvenOdd();

        return inner_contour;
    }

    bool removeEmptyToolPaths(std::vector<VariableWidthLines>& toolpaths)
    {
        toolpaths.erase(std::remove_if(toolpaths.begin(), toolpaths.end(), [](const VariableWidthLines& lines)
            {
                return lines.empty();
            }), toolpaths.end());
        return toolpaths.empty();
    }

} // namespace cura52
