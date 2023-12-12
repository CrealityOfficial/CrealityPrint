//Copyright (c) 2020 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef BOOST_INTERFACE_HPP
#define BOOST_INTERFACE_HPP

#include <boost/polygon/voronoi.hpp>
#include <boost/polygon/polygon.hpp>

#include "utils/IntPoint.h"
#include "utils/PolygonsSegmentIndex.h"
#include "utils/polygon.h"


using CSegment = cura52::PolygonsSegmentIndex;
using CPolygon = boost::polygon::polygon_data<cura52::coord_t>;
using CPolygonSet = std::vector<CPolygon>;

namespace boost {
    namespace polygon {

        template <>
        struct geometry_concept<cura52::Point>
        {
            typedef point_concept type;
        };

        template <>
        struct point_traits<cura52::Point>
        {
            typedef cura52::coord_t coordinate_type;

            static inline coordinate_type get(
                const cura52::Point& point, orientation_2d orient)
            {
                return (orient == HORIZONTAL_) ? point.X : point.Y;
            }
        };

        template <>
        struct geometry_concept<CSegment>
        {
            typedef segment_concept type;
        };

        template <>
        struct segment_traits<CSegment>
        {
            typedef cura52::coord_t coordinate_type;
            typedef cura52::Point point_type;
            static inline point_type get(const CSegment& CSegment, direction_1d dir) {
                return dir.to_int() ? CSegment.p() : CSegment.next().p();
            }
        };
    }   // polygon
}    // boost

#endif // BOOST_INTERFACE_HPP
