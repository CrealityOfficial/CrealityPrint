#include "skeletalimpl.h"
#include "skeletal/SkeletalTrapezoidation.h"

namespace crslice
{
    void convertSkeletalGraph(const SkeletalTrapezoidationGraph& graph, SkeletalGraph& skeletalGraph)
    {
        for (const STHalfEdge& edge : graph.edges)
        {
            SkeletalEdge e;
            e.from = convert(edge.from->p);
            e.to = convert(edge.to->p);
            skeletalGraph.edges.push_back(e);
        }

        for (const STHalfEdgeNode& node : graph.nodes)
        {
            SkeletalNode n;
            n.p = convert(node.p);
            skeletalGraph.nodes.push_back(n);
        }
    }

    void SkeletalCheckImpl::skeletalTrapezoidation(CrPolygons& innerPoly, std::vector<CrVariableLines>& out,
        SkeletalDetail* detail)
    {
        const BeadingStrategyPtr beading_strat = BeadingStrategyFactory::makeStrategy
        (
            scaled_spacing_wall_0,
            scaled_spacing_wall_X,
            wall_transition_length,
            transitioning_angle,
            print_thin_walls,
            min_bead_width,
            min_feature_size,
            wall_split_middle_threshold,
            wall_add_middle_threshold,
            max_bead_count,
            wall_0_inset,
            wall_distribution_count
        );

        cura52::SkeletalTrapezoidation wall_maker
        (
            input,
            *beading_strat,
            beading_strat->getTransitioningAngle(),
            discretization_step_size,
            transition_filter_dist,
            allowed_filter_deviation,
            wall_transition_length
        );

        if (detail)
            convertSkeletalGraph(wall_maker.graph, detail->graph);

        std::vector<VariableWidthLines> variableLines;
        wall_maker.generateToolpaths(variableLines);

        stitchToolPaths(variableLines, stitch_distance);
        removeSmallLines(variableLines);

        Polygons inner = separateOutInnerContour(variableLines, wall_inset_count);
        simplifyToolPaths(variableLines, max_resolution, max_deviation, max_area_deviation);
        removeEmptyToolPaths(variableLines);

        convertPolygonRaw(inner, innerPoly);
        convertVectorVariableLines(variableLines, out);
    }

    void SkeletalCheckImpl::transferGraph()
    {
        if (!degenerated_voronoi_diagram && has_missing_twin_edge(HE)) {
            LOGE("has_missing_twin_edge ..");
        }

        if (degenerated_voronoi_diagram)
            rotate_back_skeletal_trapezoidation_graph_after_fix(HE, fix_angle, vertex_mapping);

        separatePointyQuadEndNodes(HE);

        HE.collapseSmallEdges();

        // Set [incident_edge] the the first possible edge that way we can iterate over all reachable edges from node.incident_edge,
        // without needing to iterate backward
        for (edge_t& edge : HE.edges)
        {
            if (!edge.prev)
            {
                edge.from->incident_edge = &edge;
            }
        }
    }

    void SkeletalCheckImpl::transfer()
    {
        for (const DiscretizeCell& cell : discretize_cells)
        {
            int cellIndex = &cell - &discretize_cells.at(0);
            (void)cellIndex;

            Point start_source_point = cell.start_source_point;
            Point end_source_point = cell.end_source_point;
            vd_t::edge_type* starting_vonoroi_edge = cell.starting_vonoroi_edge;
            vd_t::edge_type* ending_vonoroi_edge = cell.ending_vonoroi_edge;
            // Compute and store result in above variables

            if (!starting_vonoroi_edge || !ending_vonoroi_edge)
            {
                LOGE("Each cell should start / end in a polygon vertex");
                continue;
            }

            SkeletalEdgeMap vd_edge_to_he_edge;
            SkeletalNodeMap vd_node_to_he_node;
            // Copy start to end edge to graph
            edge_t* prev_edge = nullptr;
            transferEdge(start_source_point, VoronoiUtils::p(starting_vonoroi_edge->vertex1()), *starting_vonoroi_edge, prev_edge, start_source_point, end_source_point, points, segments,
                HE, vd_edge_to_he_edge, vd_node_to_he_node, transitioning_angle, discretization_step_size);
            node_t* starting_node = vd_node_to_he_node[starting_vonoroi_edge->vertex0()];
            starting_node->data.distance_to_boundary = 0;

            constexpr bool is_next_to_start_or_end = true;
            HE.makeRib(prev_edge, start_source_point, end_source_point, is_next_to_start_or_end);
            for (vd_t::edge_type* vd_edge = starting_vonoroi_edge->next(); vd_edge != ending_vonoroi_edge; vd_edge = vd_edge->next())
            {
                assert(vd_edge->is_finite());
                Point v1 = VoronoiUtils::p(vd_edge->vertex0());
                Point v2 = VoronoiUtils::p(vd_edge->vertex1());
                transferEdge(v1, v2, *vd_edge, prev_edge, start_source_point, end_source_point, points, segments,
                    HE, vd_edge_to_he_edge, vd_node_to_he_node, transitioning_angle, discretization_step_size);

                HE.makeRib(prev_edge, start_source_point, end_source_point, vd_edge->next() == ending_vonoroi_edge);
            }

            transferEdge(VoronoiUtils::p(ending_vonoroi_edge->vertex0()), end_source_point, *ending_vonoroi_edge, prev_edge, start_source_point, end_source_point, points, segments,
                HE, vd_edge_to_he_edge, vd_node_to_he_node, transitioning_angle, discretization_step_size);
            prev_edge->to->data.distance_to_boundary = 0;
        }
    }
}
