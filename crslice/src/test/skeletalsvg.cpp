#include "skeletalimpl.h"

namespace crslice
{
    void SkeletalCheckImpl::generateBoostVoronoiTxt(const std::string& fileName)
    {
        std::fstream out;
        out.open(fileName, std::ios::out);
        if (out.is_open())
        {
            out << 0 << "\n";
            out << (int)segments.size() << "\n";
            for (std::size_t i = 0; i < segments.size(); ++i) {
                const Segment& seg = segments.at(i);
                cura52::Point f = seg.from() / 1000; //scale
                cura52::Point t = seg.to() / 1000; //scale

                out << f.X << " "
                    << f.Y << " "
                    << t.X << " "
                    << t.Y << "\n";
            }
        }
        out.close();
    }

    void SkeletalCheckImpl::generateNoPlanarVertexSVG(const std::string& fileName)
    {
#if 0
        AABB b;

        for (const vertex_type* vertex : noplanar_vertexes)
        {
            b.include(VoronoiUtils::p(vertex));
        }

        b.expand(400);

        SVG out(fileName, b);
        out.setFlipY(true);

        SVG::ColorObject bColor(SVG::Color::BLUE);
        SVG::ColorObject rColor(SVG::Color::RED);
        SVG::ColorObject gColor(SVG::Color::GREEN);

        for (const vertex_type* vertex : noplanar_vertexes)
        {
            const edge_type* edge = vertex->incident_edge();
            do {
                // FIXME Lukas H.: Also process parabolic segments.
                if (edge->is_finite() && edge->is_linear() && edge->vertex0() != nullptr && edge->vertex1() != nullptr &&
                    VoronoiUtils::is_finite(*edge->vertex0()) && VoronoiUtils::is_finite(*edge->vertex1()))
                {
                    auto f = [this, &out, &bColor, &rColor, &gColor](const edge_type* edge) {
                        SVG::ColorObject color(SVG::Color::BLACK);
                        assert(edge->is_finite());
                        Point v1 = VoronoiUtils::p(edge->vertex0());
                        Point v2 = VoronoiUtils::p(edge->vertex1());

                        Point s = v1;
                        Point e = v2;

                        out.writeLine(s, e, color, 0.3f);
                        out.writePoint(VoronoiUtils::p(edge->vertex0()), false, 0.4f);
                    };

                    const edge_type* start_edge = edge;
                    do {
                        f(start_edge);
                        start_edge = start_edge->next();
                    } while (start_edge != edge);
                }

                edge = edge->rot_next();
            } while (edge != vertex->incident_edge());
        }
#endif
    }

    void SkeletalCheckImpl::generateTransferEdgeSVG(const std::string& fileName)
    {
#if 0
        SVG out(fileName, box);
        out.setFlipY(true);

        SVG::ColorObject bColor(SVG::Color::BLUE);
        SVG::ColorObject rColor(SVG::Color::RED);
        SVG::ColorObject gColor(SVG::Color::GREEN);

        for (cell_type cell : graph.cells())
        {
            if (!cell.incident_edge())
            { // There is no spoon
                continue;
            }

            Point start_source_point;
            Point end_source_point;
            edge_type* starting_vonoroi_edge = nullptr;
            edge_type* ending_vonoroi_edge = nullptr;
            // Compute and store result in above variables

            if (cell.contains_point())
            {
                const bool keep_going = VoronoiUtils::computePointCellRange(cell, start_source_point, end_source_point, starting_vonoroi_edge, ending_vonoroi_edge, points, segments);
                if (!keep_going)
                {
                    continue;
                }
            }
            else
            {
                VoronoiUtils::computeSegmentCellRange(cell, start_source_point, end_source_point, starting_vonoroi_edge, ending_vonoroi_edge, points, segments);
            }

            auto f = [this, &out, &bColor, &rColor, &gColor](edge_type* edge) {
                SVG::ColorObject color(SVG::Color::BLACK);
                assert(edge->is_finite());
                Point v1 = VoronoiUtils::p(edge->vertex0());
                Point v2 = VoronoiUtils::p(edge->vertex1());

                const cell_type* left_cell = edge->cell();
                const cell_type* right_cell = edge->twin()->cell();
                Point start = VoronoiUtils::p(edge->vertex0());
                Point end = VoronoiUtils::p(edge->vertex1());

                bool point_left = left_cell->contains_point();
                bool point_right = right_cell->contains_point();
                if ((!point_left && !point_right) || edge->is_secondary())
                {
                    color = gColor;
                }
                else if (point_left != point_right)
                {
                    color = rColor;
                }
                else
                {
                    color = bColor;
                }

                Point s = start;
                Point e = end;

                shrinkLine(s, e, 50);
                out.writeArrow(s, e, color, 0.3f, 0.8f);
                out.writePoint(VoronoiUtils::p(edge->vertex0()), false, 0.4f);
            };

            for (edge_type* edge = starting_vonoroi_edge; edge != ending_vonoroi_edge; edge = edge->next())
                f(edge);
            f(ending_vonoroi_edge);
        }
#endif
    }
}
