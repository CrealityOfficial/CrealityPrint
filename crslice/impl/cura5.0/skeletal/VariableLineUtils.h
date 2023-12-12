// Copyright (c) 2020 Ultimaker B.V.
// CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef CURAENGINE_WALLTOOLPATHS_UTILS_H
#define CURAENGINE_WALLTOOLPATHS_UTILS_H

#include <memory>
#include "utils/ExtrusionLine.h"

namespace cura52
{
    /*!
     * Removes empty paths from the toolpaths
     * \param toolpaths the toolpaths binned by inset_idx generated with \p generate()
     * \return true if there are still paths left. If all toolpaths were removed it returns false
     */
    bool removeEmptyToolPaths(std::vector<VariableWidthLines>& toolpaths);

    /*!
     * Stitch the polylines together and form closed polygons.
     *
     * Works on both toolpaths and inner contours simultaneously.
     *
     * \param stitch_distance
     */
    void stitchToolPaths(std::vector<VariableWidthLines>& toolpaths, coord_t stitch_distance);

    /*!
     * Remove polylines shorter than half the smallest line width along that polyline.
     */
    void removeSmallLines(std::vector<VariableWidthLines>& toolpaths);

    /*!
     * Simplifies the variable-width toolpaths by calling the simplify on every line in the toolpath using the provided
     * settings.
     * \param coord_t max_resolution
     * \param coord_t max_deviation
     * \param coord_t max_area_deviation
     */
    void simplifyToolPaths(std::vector<VariableWidthLines>& toolpaths, coord_t max_resolution, coord_t max_deviation, coord_t max_area_deviation);

    /*!
     * Compute the inner contour of the walls. This contour indicates where the walled area ends and its infill begins.
     * The inside can then be filled, e.g. with skin/infill for the walls of a part, or with a pattern in the case of
     * infill with extra infill walls.
     */
    Polygons separateOutInnerContour(std::vector<VariableWidthLines>& toolpaths, size_t inset_count);
} // namespace cura52

#endif // CURAENGINE_WALLTOOLPATHS_UTILS_H
