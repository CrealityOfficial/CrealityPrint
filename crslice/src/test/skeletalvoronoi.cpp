#include "skeletalimpl.h"

namespace crslice
{
    void SkeletalCheckImpl::checkNoPlanarVertex()
    {
        for (const vd_t::vertex_type& vertex : graph.vertices()) {
            std::vector<const vd_t::edge_type*> edges;
            const vd_t::edge_type* edge = vertex.incident_edge();

            bool no_plannar = false;
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

                    if (!VoronoiUtilsCgal::check_if_three_vectors_are_ccw(VoronoiUtils::p(prev_edge->vertex0()),
                        VoronoiUtils::p(prev_edge->vertex1()),
                        VoronoiUtils::p(curr_edge->vertex1()),
                        VoronoiUtils::p(next_edge->vertex1())))
                    {
                        no_plannar = true;
                    }
                }
            }

            if(no_plannar)
                noplanar_vertexes.push_back(&vertex);
            else {
                //if (vertex.x() < -10000000000.0 || vertex.y() < -10000000000.0)
                //    noplanar_vertexes.push_back(&vertex);
            }
        }

        //check
        for (const vertex_type* vertex : noplanar_vertexes)
        {
            const vd_t::edge_type* edge = vertex->incident_edge();
            std::vector<const vd_t::edge_type*> edges;

            do {
                // FIXME Lukas H.: Also process parabolic segments.
                if (edge->is_finite() && edge->is_linear() && edge->vertex0() != nullptr && edge->vertex1() != nullptr &&
                    VoronoiUtils::is_finite(*edge->vertex0()) && VoronoiUtils::is_finite(*edge->vertex1()))
                    edges.emplace_back(edge);

                edge = edge->rot_next();
            } while (edge != vertex->incident_edge());

            // Checking for CCW make sense for three and more edges.
            if (edges.size() > 2) {
                for (auto edge_it = edges.begin(); edge_it != edges.end(); ++edge_it) {
                    const vd_t::edge_type* prev_edge = edge_it == edges.begin() ? edges.back() : *std::prev(edge_it);
                    const vd_t::edge_type* curr_edge = *edge_it;
                    const vd_t::edge_type* next_edge = std::next(edge_it) == edges.end() ? edges.front() : *std::next(edge_it);

                    if (!VoronoiUtilsCgal::check_if_three_vectors_are_ccw(VoronoiUtils::p(prev_edge->vertex0()),
                        VoronoiUtils::p(prev_edge->vertex1()),
                        VoronoiUtils::p(curr_edge->vertex1()),
                        VoronoiUtils::p(next_edge->vertex1())))
                    {
                        LOGD("check_if_three_vectors_are_ccw");
                    }
                }
            }
        }
    }

    void SkeletalCheckImpl::detectMissingVoronoiVertex(CrMissingVertex& missingVertex)
    {
        for (VoronoiUtils::vd_t::cell_type cell : graph.cells()) {
            if (!cell.incident_edge())
                continue; // There is no spoon

            if (cell.contains_segment()) {
                const Segment& source_segment = VoronoiUtils::getSourceSegment(cell, std::vector<Point>(), segments);
                const Point                            from = source_segment.from();
                const Point                            to = source_segment.to();

                // Find starting edge
                // Find end edge
                bool                           seen_possible_start = false;
                bool                           after_start = false;
                bool                           ending_edge_is_set_before_start = false;
                VoronoiUtils::vd_t::edge_type* starting_vd_edge = nullptr;
                VoronoiUtils::vd_t::edge_type* ending_vd_edge = nullptr;
                VoronoiUtils::vd_t::edge_type* edge = cell.incident_edge();
                do {
                    if (edge->is_infinite() || edge->vertex0() == nullptr || edge->vertex1() == nullptr || !VoronoiUtils::is_finite(*edge->vertex0()) || !VoronoiUtils::is_finite(*edge->vertex1()))
                        continue;

                    Point v0 = VoronoiUtils::p(edge->vertex0());
                    Point v1 = VoronoiUtils::p(edge->vertex1());

                    //assert(!(v0 == to && v1 == from));
                    if (v0 == to && !after_start) { // Use the last edge which starts in source_segment.to
                        starting_vd_edge = edge;
                        seen_possible_start = true;
                    }
                    else if (seen_possible_start) {
                        after_start = true;
                    }

                    if (v1 == from && (!ending_vd_edge || ending_edge_is_set_before_start)) {
                        ending_edge_is_set_before_start = !after_start;
                        ending_vd_edge = edge;
                    }
                } while (edge = edge->next(), edge != cell.incident_edge());

                if (!starting_vd_edge || !ending_vd_edge || starting_vd_edge == ending_vd_edge)
                {
                    if (starting_vd_edge && ending_vd_edge)
                    {
                        edge = cell.incident_edge();
                        do {
                            if (edge->is_infinite() || edge->vertex0() == nullptr || edge->vertex1() == nullptr || !VoronoiUtils::is_finite(*edge->vertex0()) || !VoronoiUtils::is_finite(*edge->vertex1()))
                                continue;
                            
                            insertEdge(edge, missingVertex.edges1);
                        } while (edge = edge->next(), edge != cell.incident_edge());
                    }
                    else {
                        LOGE("null edge.");
                    }
                }
            }
        }

        for (const vd_t::vertex_type& vertex : graph.vertices()) {
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

                    if (!VoronoiUtilsCgal::check_if_three_vectors_are_ccw(VoronoiUtils::p(prev_edge->vertex0()),
                        VoronoiUtils::p(prev_edge->vertex1()),
                        VoronoiUtils::p(curr_edge->vertex1()),
                        VoronoiUtils::p(next_edge->vertex1())))
                    {
                        insertEdge(prev_edge, missingVertex.edges2);
                        insertEdge(curr_edge, missingVertex.edges2);
                        insertEdge(next_edge, missingVertex.edges2);
                    }
                }
            }
        }
    }

    void SkeletalCheckImpl::classifyEdge()
    {
        discretize_edges.clear();
        discretize_cells.clear();

        for (const cell_type& cell : graph.cells())
        {
            if (!cell.incident_edge())
            { // There is no spoon
                continue;
            }

            DiscretizeCell DC;

            Point start_source_point;
            Point end_source_point;
            edge_type* starting_vonoroi_edge = nullptr;
            edge_type* ending_vonoroi_edge = nullptr;
            // Compute and store result in above variables

            if (cell.contains_point())
            {
                const bool keep_going = VoronoiUtils::computePointCellRange((cell_type&)cell, start_source_point, end_source_point, starting_vonoroi_edge, ending_vonoroi_edge, points, segments);
                if (!keep_going)
                {
                    continue;
                }
            }
            else
            {
                VoronoiUtils::computeSegmentCellRange((cell_type&)cell, start_source_point, end_source_point, starting_vonoroi_edge, ending_vonoroi_edge, points, segments);
            }

            if (!starting_vonoroi_edge || !ending_vonoroi_edge)
            {
                invalid_cells.push_back(&cell);
                continue;
            }

            if (discretize_cells.size() == 1568)
                int i = 0;

            DC.ending_vonoroi_edge = ending_vonoroi_edge;
            DC.end_source_point = end_source_point;
            DC.starting_vonoroi_edge = starting_vonoroi_edge;
            DC.start_source_point = start_source_point;
            discretize_cells.push_back(DC);

            auto f = [this](edge_type* edge) {
                assert(edge->is_finite());

                DiscretizeEdge DE;
                DE.type = EdgeDiscretizeType::edt_edge;
                DE.edge = edge;

                const cell_type* left_cell = edge->cell();
                const cell_type* right_cell = edge->twin()->cell();

                bool point_left = left_cell->contains_point();
                bool point_right = right_cell->contains_point();
                if ((!point_left && !point_right) || edge->is_secondary())
                {
                    DE.type = EdgeDiscretizeType::edt_edge;
                }
                else if (point_left != point_right)
                {
                    DE.type = EdgeDiscretizeType::edt_parabola;
                }
                else
                {
                    DE.type = EdgeDiscretizeType::edt_straightedge;
                }

                discretize_edges.push_back(DE);
            };

            for (edge_type* edge = starting_vonoroi_edge; edge != ending_vonoroi_edge; edge = edge->next())
                f(edge);
            f(ending_vonoroi_edge);
        }
    }

    void SkeletalCheckImpl::transferEdges(CrDiscretizeEdges& discretizeEdges)
    {
        const std::vector<edge_type>& edges = graph.edges();
        for (const DiscretizeEdge& DE : discretize_edges)
        {
            edge_type* edge = DE.edge;

            if (DE.type == EdgeDiscretizeType::edt_edge)
            {
                insertEdge(edge, discretizeEdges.edges);
            }
            else if (DE.type == EdgeDiscretizeType::edt_parabola)
            {
                insertEdge(edge, discretizeEdges.parabola);
            }
            else if (DE.type == EdgeDiscretizeType::edt_straightedge)
            {
                insertEdge(edge, discretizeEdges.straightedges);
            }
        }
    }

    bool SkeletalCheckImpl::transferCell(int index, CrDiscretizeCell& discretizeCell)
    {
        if (index >= 0 && index < discretize_cells.size())
        {
            const DiscretizeCell& cell = discretize_cells.at(index);

            discretizeCell.start = convertScale(cell.start_source_point);
            discretizeCell.end = convertScale(cell.end_source_point);

            auto f = [this, &discretizeCell](edge_type* edge) {
                insertEdge(edge, discretizeCell.edges);
            };

            for (edge_type* edge = cell.starting_vonoroi_edge; edge != cell.ending_vonoroi_edge; edge = edge->next()){
                f(edge);
            }
            f(cell.ending_vonoroi_edge);

            return true;
        }

        return false;
    }

    bool SkeletalCheckImpl::transferInvalidCell(int index, CrDiscretizeCell& discretizeCell)
    {
        if (index >= 0 && index < invalid_cells.size())
        {
            const cell_type* cell = invalid_cells.at(index);
            const edge_type* start = cell->incident_edge();

            if (!start)
                return false;

            int count = 0;
            do {
                insertEdge(start, discretizeCell.edges);

                start = start->next();
            } while (start != cell->incident_edge());

            return true;
        }

        return false;
    }
}
