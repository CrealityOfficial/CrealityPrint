#ifndef VORONOI_UTILS_CGAL_H
#define VORONOI_UTILS_CGAL_H
#include <boost/polygon/voronoi.hpp>
#include "utils/IntPoint.h"

namespace cura52 {
    using pos_t = double;
    using vd_t = boost::polygon::voronoi_diagram<pos_t>;

    class VoronoiUtilsCgal
    {
    public:
        // Check if the Voronoi diagram is planar using CGAL sweeping edge algorithm for enumerating all intersections between lines.
        static bool is_voronoi_diagram_planar_intersection(const vd_t& voronoi_diagram);

        // Check if the Voronoi diagram is planar using verification that all neighboring edges are ordered CCW for each vertex.
        static bool is_voronoi_diagram_planar_angle(const vd_t& voronoi_diagram);

        static bool check_if_three_vectors_are_ccw(const Point& common_pt, const Point& pt_1, const Point& pt_2, const Point& test_pt);
    };
} // cura52

#endif // VORONOI_UTILS_CGAL_H
