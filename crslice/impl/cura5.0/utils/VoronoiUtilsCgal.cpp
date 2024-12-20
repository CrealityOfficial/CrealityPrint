#include "utils/VoronoiUtilsCgal.h"
#include "utils/polygonUtils.h"
#include "utils/VoronoiUtils.h"

namespace cura52 {
    enum cType
    {
        COLLINEAR,
        LEFT_TURN,
        RIGHT_TURN
    };

    inline static Point to_Point(const vd_t::vertex_type& pt) 
    {
        return VoronoiUtils::p(&pt);
    }

    inline static Polygon to_Polygon(const Point& pt_0, const Point& pt_1, const Point& pt_2)
    {
        Polygon pol;
        pol.add(pt_0);
        pol.add(pt_1);
        pol.add(pt_2);
        return pol;
    }

    cType check_ccw(const Point& pt_0, const Point& pt_1, const Point& pt_2)
    {
        Polygon pol = to_Polygon(pt_0, pt_1, pt_2);
        double area = pol.area();
        if (area == 0)
        {
            return COLLINEAR;
        }
        else if (area < 0) {
            return RIGHT_TURN;
        }
        else {
            return LEFT_TURN;
        }
    }

    bool check_polyLines_self_intersection(Polygons segments)
    {
        int size = segments.size();
        for (int i = 0; i < size; i++)
        {
            int next_idx = i == size - 1 ? 0 : i + 1;
            int last_idx = i == 0 ? size - 1 : i - 1;
            for (int j = 0; j < size; j++)
            {
                if (j == next_idx || j == last_idx || j == i) continue;
                if (PolygonUtils::polygonCollidesWithLineSegment(segments[i], segments[j][0], segments[j][1]))
                {
                    return true;
                }
            }
        }
        return false;
    }

    // FIXME Lukas H.: Also includes parabolic segments.
    bool VoronoiUtilsCgal::is_voronoi_diagram_planar_intersection(const vd_t& voronoi_diagram)
    {
        assert(std::all_of(voronoi_diagram.edges().cbegin(), voronoi_diagram.edges().cend(),
            [](const vd_t::edge_type& edge) { return edge.color() == 0; }));

        Polygons segments;
        for (const vd_t::edge_type& edge : voronoi_diagram.edges()) {
            if (edge.color() != 0)
                continue;

            if (edge.is_finite() && edge.is_linear() && edge.vertex0() != nullptr && edge.vertex1() != nullptr &&
                VoronoiUtils::is_finite(*edge.vertex0()) && VoronoiUtils::is_finite(*edge.vertex1())) {
                segments.addLine(to_Point(*edge.vertex0()), to_Point(*edge.vertex1()));
                edge.color(1);
                assert(edge.twin() != nullptr);
                edge.twin()->color(1);
            }
        }

        for (const vd_t::edge_type& edge : voronoi_diagram.edges())
            edge.color(0);

        return check_polyLines_self_intersection(segments);
    }

    bool VoronoiUtilsCgal::check_if_three_vectors_are_ccw(const Point& common_pt, const Point& pt_1, const Point& pt_2, const Point& test_pt) {
        cType orientation = check_ccw(common_pt, pt_1, pt_2);
        if (orientation == COLLINEAR) {
            // The first two edges are collinear, so the third edge must be on the right side on the first of them.
            return check_ccw(common_pt, pt_1, test_pt) == RIGHT_TURN;
        }
        else if (orientation == LEFT_TURN) {
            // CCW oriented angle between vectors (common_pt, pt1) and (common_pt, pt2) is bellow PI.
            // So we need to check if test_pt isn't between them.
            cType orientation1 = check_ccw(common_pt, pt_1, test_pt);
            cType orientation2 = check_ccw(common_pt, pt_2, test_pt);
            return (orientation1 != LEFT_TURN || orientation2 != RIGHT_TURN);
        }
        else {
            assert(orientation == RIGHT_TURN);
            // CCW oriented angle between vectors (common_pt, pt1) and (common_pt, pt2) is upper PI.
            // So we need to check if test_pt is between them.
            cType orientation1 = check_ccw(common_pt, pt_1, test_pt);
            cType orientation2 = check_ccw(common_pt, pt_2, test_pt);
            return (orientation1 == RIGHT_TURN || orientation2 == LEFT_TURN);
        }
    }

    bool VoronoiUtilsCgal::is_voronoi_diagram_planar_angle(const vd_t& voronoi_diagram)
    {
        for (const vd_t::vertex_type& vertex : voronoi_diagram.vertices()) {
            std::vector<const vd_t::edge_type*> edges;
            const vd_t::edge_type* edge = vertex.incident_edge();

            do {
                // FIXME Lukas H.: Also process parabolic segments.
                if (edge->is_finite() && edge->is_linear() && edge->vertex0() != nullptr && edge->vertex1() != nullptr &&
                    VoronoiUtils::is_finite(*edge->vertex0()) && VoronoiUtils::is_finite(*edge->vertex1()))
                    edges.emplace_back(edge);

                edge = edge->rot_next();
            } while (edge != vertex.incident_edge());

            // Checking for CCW make sense for three and more edges.
            if (edges.size() > 2) {
                for (auto edge_it = edges.begin(); edge_it != edges.end(); ++edge_it) {
                    const vd_t::edge_type* prev_edge = edge_it == edges.begin() ? edges.back() : *std::prev(edge_it);
                    const vd_t::edge_type* curr_edge = *edge_it;
                    const vd_t::edge_type* next_edge = std::next(edge_it) == edges.end() ? edges.front() : *std::next(edge_it);

                    if (!check_if_three_vectors_are_ccw(to_Point(*prev_edge->vertex0()), to_Point(*prev_edge->vertex1()),
                        to_Point(*curr_edge->vertex1()), to_Point(*next_edge->vertex1())))
                        return false;
                }
            }
        }

        return true;
    }
}// namespace cura52