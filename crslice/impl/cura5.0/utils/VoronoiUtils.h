//Copyright (c) 2020 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.


#ifndef UTILS_VORONOI_UTILS_H
#define UTILS_VORONOI_UTILS_H

#include <vector>
#include <boost/polygon/voronoi.hpp>
#include "PolygonsSegmentIndex.h"

namespace cura52
{

    /*!
     */
    class VoronoiUtils
    {
    public:
        using Segment = PolygonsSegmentIndex;
        using voronoi_data_t = double;
        using vd_t = boost::polygon::voronoi_diagram<voronoi_data_t>;

        static Point getSourcePoint(const vd_t::cell_type& cell, const std::vector<Point>& points, const std::vector<Segment>& segments);
        static const Segment& getSourceSegment(const vd_t::cell_type& cell, const std::vector<Point>& points, const std::vector<Segment>& segments);
        static PolygonsPointIndex getSourcePointIndex(const vd_t::cell_type& cell, const std::vector<Point>& points, const std::vector<Segment>& segments);

        static Point p(const vd_t::vertex_type* node);

        static bool isSourcePoint(Point p, const vd_t::cell_type& cell, const std::vector<Point>& points, const std::vector<Segment>& segments, coord_t snap_dist = 10);

        static coord_t getDistance(Point p, const vd_t::cell_type& cell, const std::vector<Point>& points, const std::vector<Segment>& segments);

        /*!
         * Discretize a parabola based on (approximate) step size.
         * The \p approximate_step_size is measured parallel to the \p source_segment, not along the parabola.
         */
        static std::vector<Point> discretizeParabola(const Point& source_point, const Segment& source_segment, Point start, Point end, coord_t approximate_step_size, float transitioning_angle);
        static inline bool is_finite(const VoronoiUtils::vd_t::vertex_type& vertex)
        {
            return std::isfinite(vertex.x()) && std::isfinite(vertex.y());
        }
    protected:
        /*!
         * Discretize parabola based on max absolute deviation from the parabola.
         *
         * adapted from boost::polygon::voronoi_visual_utils.cpp
         *
         * Discretize parabolic Voronoi edge.
         * Parabolic Voronoi edges are always formed by one point and one segment
         * from the initial input set.
         *
         * Args:
         * point: input point.
         * segment: input segment.
         * max_dist: maximum discretization distance.
         * discretization: point discretization of the given Voronoi edge.
         *
         * Template arguments:
         * InCT: coordinate type of the input geometries (usually integer).
         * Point: point type, should model point concept.
         * Segment: segment type, should model segment concept.
         *
         * Important:
         * discretization should contain both edge endpoints initially.
         */
        static void discretize(
            const Point& point,
            const Segment& segment,
            const coord_t max_dist,
            std::vector<Point>* discretization);

        /*!
         * adapted from boost::polygon::voronoi_visual_utils.cpp
         * Compute y(x) = ((x - a) * (x - a) + b * b) / (2 * b).
         */
        static coord_t parabolaY(coord_t x, coord_t a, coord_t b);

        /*!
         * adapted from boost::polygon::voronoi_visual_utils.cpp
         * Get normalized length of the distance between:
         *     1) point projection onto the segment
         *     2) start point of the segment
         * Return this length divided by the segment length. This is made to avoid
         * sqrt computation during transformation from the initial space to the
         * transformed one and vice versa. The assumption is made that projection of
         * the point lies between the start-point and endpoint of the segment.
         */
        static double getPointProjection(const Point& point, const Segment& segment);

    public:
        /*!
        * Compute the range of line segments that surround a cell of the skeletal
        * graph that belongs to a line segment of the medial axis.
        *
        * This should only be used on cells that belong to a central line segment
        * of the skeletal graph, e.g. trapezoid cells, not triangular cells.
        *
        * The resulting line segments is just the first and the last segment. They
        * are linked to the neighboring segments, so you can iterate over the
        * segments until you reach the last segment.
        * \param cell The cell to compute the range of line segments for.
        * \param[out] start_source_point The start point of the source segment of
        * this cell.
        * \param[out] end_source_point The end point of the source segment of this
        * cell.
        * \param[out] starting_vd_edge The edge of the Voronoi diagram where the
        * loop around the cell starts.
        * \param[out] ending_vd_edge The edge of the Voronoi diagram where the loop
        * around the cell ends.
        * \param points All vertices of the input Polygons.
        * \param segments All edges of the input Polygons.
        * /return Whether the cell is inside of the polygon. If it's outside of the
        * polygon we should skip processing it altogether.
        */
        static void computeSegmentCellRange(vd_t::cell_type& cell, Point& start_source_point, Point& end_source_point, vd_t::edge_type*& starting_vd_edge, vd_t::edge_type*& ending_vd_edge, const std::vector<Point>& points, const std::vector<Segment>& segments);

        /*!
         * Compute the range of line segments that surround a cell of the skeletal
         * graph that belongs to a point on the medial axis.
         *
         * This should only be used on cells that belong to a corner in the skeletal
         * graph, e.g. triangular cells, not trapezoid cells.
         *
         * The resulting line segments is just the first and the last segment. They
         * are linked to the neighboring segments, so you can iterate over the
         * segments until you reach the last segment.
         * \param cell The cell to compute the range of line segments for.
         * \param[out] start_source_point The start point of the source segment of
         * this cell.
         * \param[out] end_source_point The end point of the source segment of this
         * cell.
         * \param[out] starting_vd_edge The edge of the Voronoi diagram where the
         * loop around the cell starts.
         * \param[out] ending_vd_edge The edge of the Voronoi diagram where the loop
         * around the cell ends.
         * \param points All vertices of the input Polygons.
         * \param segments All edges of the input Polygons.
         * /return Whether the cell is inside of the polygon. If it's outside of the
         * polygon we should skip processing it altogether.
         */
        static bool computePointCellRange(vd_t::cell_type& cell, Point& start_source_point, Point& end_source_point, vd_t::edge_type*& starting_vd_edge, vd_t::edge_type*& ending_vd_edge, const std::vector<Point>& points, const std::vector<Segment>& segments);

        static bool has_finite_edge_with_non_finite_vertex(const vd_t& voronoi_diagram);
        static bool detect_missing_voronoi_vertex(const vd_t& voronoi_diagram, const std::vector<VoronoiUtils::Segment>& segments);
    };

} // namespace cura52

#endif // UTILS_VORONOI_UTILS_H
