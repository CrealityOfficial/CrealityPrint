//Copyright (c) 2022 Ultimaker B.V.

#include "SparseGridDebug.h"
#include "SVG.h"

namespace cura52
{
	void debug_sparse_grid()
	{
        //AABB aabb;
        //for (std::pair<GridPoint, ElemT> cell : SparseGrid<ElemT>::m_grid)
        //{
        //    aabb.include(SparseGrid<ElemT>::toLowerCorner(cell.first));
        //    aabb.include(SparseGrid<ElemT>::toLowerCorner(cell.first + GridPoint(SparseGrid<ElemT>::nonzero_sign(cell.first.X), SparseGrid<ElemT>::nonzero_sign(cell.first.Y))));
        //}
        //SVG svg(filename.c_str(), aabb);
        //for (std::pair<GridPoint, ElemT> cell : SparseGrid<ElemT>::m_grid)
        //{
        //    // doesn't draw cells at x = 0 or y = 0 correctly (should be double size)
        //    Point lb = SparseGrid<ElemT>::toLowerCorner(cell.first);
        //    Point lt = SparseGrid<ElemT>::toLowerCorner(cell.first + GridPoint(0, SparseGrid<ElemT>::nonzero_sign(cell.first.Y)));
        //    Point rt = SparseGrid<ElemT>::toLowerCorner(cell.first + GridPoint(SparseGrid<ElemT>::nonzero_sign(cell.first.X), SparseGrid<ElemT>::nonzero_sign(cell.first.Y)));
        //    Point rb = SparseGrid<ElemT>::toLowerCorner(cell.first + GridPoint(SparseGrid<ElemT>::nonzero_sign(cell.first.X), 0));
        //    if (lb.X == 0)
        //    {
        //        lb.X = -SparseGrid<ElemT>::cell_size;
        //        lt.X = -SparseGrid<ElemT>::cell_size;
        //    }
        //    if (lb.Y == 0)
        //    {
        //        lb.Y = -SparseGrid<ElemT>::cell_size;
        //        rb.Y = -SparseGrid<ElemT>::cell_size;
        //    }
        //    //         svg.writePoint(lb, true, 1);
        //    svg.writeLine(lb, lt, SVG::Color::GRAY);
        //    svg.writeLine(lt, rt, SVG::Color::GRAY);
        //    svg.writeLine(rt, rb, SVG::Color::GRAY);
        //    svg.writeLine(rb, lb, SVG::Color::GRAY);
        //
        //    std::pair<Point, Point> line = m_locator(cell.second);
        //    svg.writePoint(line.first, true);
        //    svg.writePoint(line.second, true);
        //    svg.writeLine(line.first, line.second, SVG::Color::BLACK);
        //}
	}

	void test_sparse_grid()
	{
        struct PairLocator
        {
            std::pair<Point, Point> operator()(const std::pair<Point, Point>& val) const
            {
                return val;
            }
        };
        SparseLineGrid<std::pair<Point, Point>, PairLocator> line_grid(10);

        // straight lines
        line_grid.insert(std::make_pair<Point, Point>(Point(50, 0), Point(50, 70)));
        line_grid.insert(std::make_pair<Point, Point>(Point(0, 90), Point(50, 90)));
        line_grid.insert(std::make_pair<Point, Point>(Point(253, 103), Point(253, 173)));
        line_grid.insert(std::make_pair<Point, Point>(Point(203, 193), Point(253, 193)));
        line_grid.insert(std::make_pair<Point, Point>(Point(-50, 0), Point(-50, -70)));
        line_grid.insert(std::make_pair<Point, Point>(Point(0, -90), Point(-50, -90)));
        line_grid.insert(std::make_pair<Point, Point>(Point(-253, -103), Point(-253, -173)));
        line_grid.insert(std::make_pair<Point, Point>(Point(-203, -193), Point(-253, -193)));

        // diagonal lines
        line_grid.insert(std::make_pair<Point, Point>(Point(113, 133), Point(166, 125)));
        line_grid.insert(std::make_pair<Point, Point>(Point(13, 73), Point(26, 25)));
        line_grid.insert(std::make_pair<Point, Point>(Point(166, 33), Point(113, 25)));
        line_grid.insert(std::make_pair<Point, Point>(Point(26, 173), Point(13, 125)));
        line_grid.insert(std::make_pair<Point, Point>(Point(-24, -18), Point(-19, -64)));
        line_grid.insert(std::make_pair<Point, Point>(Point(-113, -133), Point(-166, -125)));
        line_grid.insert(std::make_pair<Point, Point>(Point(-166, -33), Point(-113, -25)));
        line_grid.insert(std::make_pair<Point, Point>(Point(-26, -173), Point(-13, -125)));

        // diagonal lines exactly crossing cell corners
        line_grid.insert(std::make_pair<Point, Point>(Point(160, 190), Point(220, 170)));
        line_grid.insert(std::make_pair<Point, Point>(Point(60, 130), Point(80, 70)));
        line_grid.insert(std::make_pair<Point, Point>(Point(220, 90), Point(160, 70)));
        line_grid.insert(std::make_pair<Point, Point>(Point(80, 220), Point(60, 160)));
        line_grid.insert(std::make_pair<Point, Point>(Point(-160, -190), Point(-220, -170)));
        line_grid.insert(std::make_pair<Point, Point>(Point(-60, -130), Point(-80, -70)));
        line_grid.insert(std::make_pair<Point, Point>(Point(-220, -90), Point(-160, -70)));
        line_grid.insert(std::make_pair<Point, Point>(Point(-80, -220), Point(-60, -160)));

        // single cell
        line_grid.insert(std::make_pair<Point, Point>(Point(203, 213), Point(203, 213)));
        line_grid.insert(std::make_pair<Point, Point>(Point(223, 213), Point(223, 215)));
        line_grid.insert(std::make_pair<Point, Point>(Point(243, 213), Point(245, 213)));
        line_grid.insert(std::make_pair<Point, Point>(Point(263, 213), Point(265, 215)));
        line_grid.insert(std::make_pair<Point, Point>(Point(283, 215), Point(285, 213)));
        line_grid.insert(std::make_pair<Point, Point>(Point(-203, -213), Point(-203, -213)));

        // around origin
        line_grid.insert(std::make_pair<Point, Point>(Point(20, -20), Point(-20, 20)));
	}

    void writePolylines(SVG& svg, Polygons polylines, SVG::Color color) // todo remove as only for debugging relevant
    {
        for (auto path : polylines)
        {
            if (path.size() == 0)
            {
                continue;
            }
            if (path.size() == 1)
            {
                svg.writePoint(path[0], false, 2, color);
            }
            Point before = path[0];
            for (size_t i = 1; i < path.size(); i++)
            {
                svg.writeLine(before, path[i], color, 2);
                before = path[i];
            }
        }
    }

    void writePoints(SVG& svg, Polygons polylines, SVG::Color color) // todo remove as only for debugging relevant
    {
        for (auto path : polylines)
        {
            for (Point p : path)
            {
                svg.writePoint(p, false, 2, color);
            }
        }
    }
} // namespace cura52
