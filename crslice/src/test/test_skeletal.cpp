#include "src/test/skeletalimpl.h"

namespace crslice
{
    void testDiscretizeParabola(CrPolygon& points, ParabolaDetal& detail)
    {
        if (points.size() != 5)
            return;

        auto add_detail = [&detail](const Point& point) {
            detail.points.push_back(convert(point));
        };

        Polygons polys;
        polys.emplace_back(Polygon());
        convertRaw(points, polys.paths.at(0));
        const ClipperLib::Path& path = polys.paths.at(0);

        Point a = path.at(0);
        Point b = path.at(1);
        Point s = path.at(2);
        Point e = path.at(3);
        Point p = path.at(4);

        AABB box;
        box.include(a);
        box.include(b);
        box.include(s);
        box.include(e);
        box.include(p);
        box.expand(200);

        SVG svg("tmp.SVG", box);
        svg.writePoint(a);
        svg.writeText(a, "a");
        svg.writePoint(b);
        svg.writeText(b, "b");
        svg.writePoint(s);
        svg.writeText(s, "s");
        svg.writePoint(e);
        svg.writeText(e, "e");
        svg.writePoint(p);
        svg.writeText(p, "p");

        SVG::ColorObject bColor(SVG::Color::BLUE);
        svg.writeLine(a, b, bColor);

        SVG::ColorObject rColor(SVG::Color::RED);
        svg.writeLine(s, e, rColor);

        PolygonsSegmentIndex segment(&polys, 0, 0);

        coord_t approximate_step_size = 800;
        float transitioning_angle = 0.17453292519943275;

        do{
            std::vector<Point> discretized;
            // x is distance of point projected on the segment ab
            // xx is point projected on the segment ab
            const Point a = segment.from();
            const Point b = segment.to();
            const Point ab = b - a;

            const Point as = s - a;

            const Point ae = e - a;

            const coord_t ab_size = vSize(ab);
            if (ab_size == 0)
            {
                discretized.emplace_back(s);
                discretized.emplace_back(e);
                break;
            }
            const coord_t sx = dot(as, ab) / ab_size;
            const coord_t ex = dot(ae, ab) / ab_size;
            const coord_t sxex = ex - sx;

            const Point ap = p - a;

            const coord_t px = dot(ap, ab) / ab_size;

            const Point pxx = LinearAlg2D::getClosestOnLine(p, a, b);
            add_detail(pxx);
            svg.writePoint(pxx);
            svg.writeText(pxx, "pxx");

            const Point ppxx = pxx - p;
            const coord_t d = vSize(ppxx);
            const PointMatrix rot = PointMatrix(turn90CCW(ppxx));

            if (d == 0)
            {
                discretized.emplace_back(s);
                discretized.emplace_back(e);
                break;
            }

            const float marking_bound = atan(transitioning_angle * 0.5);
            coord_t msx = -marking_bound * d; // projected marking_start
            coord_t mex = marking_bound * d; // projected marking_end
            const coord_t marking_start_end_h = msx * msx / (2 * d) + d / 2;
            Point marking_start = rot.unapply(Point(msx, marking_start_end_h)) + pxx;
            Point marking_end = rot.unapply(Point(mex, marking_start_end_h)) + pxx;

            const int dir = (sx > ex) ? -1 : 1;
            if (dir < 0)
            {
                std::swap(marking_start, marking_end);
                std::swap(msx, mex);
            }

            add_detail(marking_start);
            svg.writePoint(marking_start);
            svg.writeText(marking_start, "ms");
            add_detail(marking_end);
            svg.writePoint(marking_end);
            svg.writeText(marking_end, "me");

            bool add_marking_start = msx * dir > (sx - px) * dir && msx * dir < (ex - px)* dir;
            bool add_marking_end = mex * dir > (sx - px) * dir && mex * dir < (ex - px)* dir;

            const Point apex = rot.unapply(Point(0, d / 2)) + pxx;
            // Only at the apex point if the projected start and end points
            // are more than 10 microns away from the projected apex
            bool add_apex = (sx - px) * dir < -10 && (ex - px) * dir > 10;

            add_detail(apex);
            svg.writePoint(apex);
            svg.writeText(apex, "apex");

            //assert(!(add_marking_start && add_marking_end) || add_apex);
            if (add_marking_start && add_marking_end && !add_apex)
            {
                LOGW("Failing to discretize parabola! Must add an apex or one of the endpoints.");
            }

            const coord_t step_count = static_cast<coord_t>(static_cast<float>(std::abs(ex - sx)) / approximate_step_size + 0.5);

            discretized.emplace_back(s);
            for (coord_t step = 1; step < step_count; step++)
            {
                const coord_t x = sx + sxex * step / step_count - px;
                const coord_t y = x * x / (2 * d) + d / 2;

                if (add_marking_start && msx * dir < x * dir)
                {
                    discretized.emplace_back(marking_start);
                    add_marking_start = false;
                }
                if (add_apex && x * dir > 0)
                {
                    discretized.emplace_back(apex);
                    add_apex = false; // only add the apex just before the
                }
                if (add_marking_end && mex * dir < x * dir)
                {
                    discretized.emplace_back(marking_end);
                    add_marking_end = false;
                }
                const Point result = rot.unapply(Point(x, y)) + pxx;
                discretized.emplace_back(result);

                svg.writePoint(result);
                svg.writeText(result, "r");

            }
            if (add_apex)
            {
                discretized.emplace_back(apex);
            }
            if (add_marking_end)
            {
                discretized.emplace_back(marking_end);
            }
            discretized.emplace_back(e);

            for (const Point& p : discretized)
                add_detail(p);
        } while (0);
    }

    SkeletalCheck::SkeletalCheck()
        :impl(new SkeletalCheckImpl())
    {

    }

    SkeletalCheck::~SkeletalCheck()
    {
        delete impl;
    }

    void SkeletalCheck::setInput(const SerailCrSkeletal& skeletal)
    {
        outPoly = skeletal.polygons;
        impl->setInput(skeletal);
    }

    bool SkeletalCheck::isValid()
    {
        return impl->isValid();
    }

    void SkeletalCheck::detectMissingVoronoiVertex(CrMissingVertex& vertex)
    {
        impl->detectMissingVoronoiVertex(vertex);
    }

    const CrPolygons& SkeletalCheck::outline()
    {
        return outPoly;
    }

    void SkeletalCheck::skeletalTrapezoidation(CrPolygons& innerPoly, std::vector<CrVariableLines>& out,
        SkeletalDetail* detail)
    {
        impl->skeletalTrapezoidation(innerPoly, out, detail);
    }

    void SkeletalCheck::transferEdges(CrDiscretizeEdges& discretizeEdges)
    {
        impl->transferEdges(discretizeEdges);
    }

    bool SkeletalCheck::transferCell(int index, CrDiscretizeCell& discretizeCell)
    {
        return impl->transferCell(index, discretizeCell);
    }

    bool SkeletalCheck::transferInvalidCell(int index, CrDiscretizeCell& discretizeCell)
    {
        return impl->transferInvalidCell(index, discretizeCell);
    }

    void SkeletalCheck::transferGraph()
    {
        impl->transferGraph();
    }

    void SkeletalCheck::generateBoostVoronoiTxt(const std::string& fileName)
    {
        impl->generateBoostVoronoiTxt(fileName);
    }

    void SkeletalCheck::generateNoPlanarVertexSVG(const std::string& fileName)
    {
        impl->generateNoPlanarVertexSVG(fileName);
    }

    void SkeletalCheck::generateTransferEdgeSVG(const std::string& fileName)
    {
        impl->generateTransferEdgeSVG(fileName);
    }
}
