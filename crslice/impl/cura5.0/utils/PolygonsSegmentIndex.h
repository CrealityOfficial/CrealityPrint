//Copyright (c) 2020 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef UTILS_POLYGONS_SEGMENT_INDEX_H
#define UTILS_POLYGONS_SEGMENT_INDEX_H

#include <vector>

#include "PolygonsPointIndex.h"

namespace cura52
{

    /*!
     * A class for iterating over the points in one of the polygons in a \ref Polygons object
     */
    class PolygonsSegmentIndex : public PolygonsPointIndex
    {
    public:
        PolygonsSegmentIndex();
        PolygonsSegmentIndex(const Polygons* polygons, unsigned int poly_idx, unsigned int point_idx);

        Point from() const;

        Point to() const;
    };

    Point polygonsIndexPoint(const PolygonsPointIndex& index);

} // namespace cura52


#endif//UTILS_POLYGONS_SEGMENT_INDEX_H
