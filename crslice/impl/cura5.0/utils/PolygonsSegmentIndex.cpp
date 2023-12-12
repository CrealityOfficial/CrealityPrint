//Copyright (c) 2022 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#include "utils/PolygonsSegmentIndex.h"

namespace cura52
{

    PolygonsSegmentIndex::PolygonsSegmentIndex() : PolygonsPointIndex() {}

    PolygonsSegmentIndex::PolygonsSegmentIndex(const Polygons* polygons, unsigned int poly_idx, unsigned int point_idx) : PolygonsPointIndex(polygons, poly_idx, point_idx) {}

    Point PolygonsSegmentIndex::from() const
    {
        return PolygonsPointIndex::p() * 1000;  //scale
    }

    Point PolygonsSegmentIndex::to() const
    {
        return PolygonsSegmentIndex::next().p() * 1000;  //scale 
    }

    Point polygonsIndexPoint(const PolygonsPointIndex& index)
    {
        return index.p() * 1000;
    }

}
