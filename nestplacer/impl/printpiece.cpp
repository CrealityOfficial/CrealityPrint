#include "nestplacer/printpiece.h"
#include "libnest2d/libnest2d.hpp"
#include "msbase/utils/trimeshserial.h"
#include "../libnest2d/tools/svgtools.hpp"

#define INT2UM(x) (static_cast<double>(x) / 1000000.0)
#define UM2INT(x) (static_cast<Clipper3r::cInt>((x) * 1000000.0 + 0.5 * (double((x) > 0) - ((x) < 0))))

namespace nestplacer {
    using Vertex = libnest2d::TPoint<Clipper3r::Polygon>;

    Clipper3r::IntPoint convertIntPoint(const trimesh::vec3& pt)
    {
        return Clipper3r::IntPoint(UM2INT(pt.x), UM2INT(pt.y));
    }

    trimesh::vec3 convertDoublePoint(const Clipper3r::IntPoint& pt)
    {
        return trimesh::vec3(INT2UM(pt.X), INT2UM(pt.Y), 0);
    }

    class PRInput : public ccglobal::Serializeable {
    public:
        std::vector<nestplacer::PR_Polygon> polys;
        trimesh::vec3 point;
        bool calDist = false;
        
        PRInput() {}
        ~PRInput() {}

        int version() override { return 0; }

        bool save(std::fstream& out, ccglobal::Tracer* tracer) override
        {
            msbase::CXNDGeometrys geometrys;
            geometrys.reserve(polys.size());
            for (auto& poly : polys) {
                msbase::CXNDGeometry geo;
                geo.contour.swap(poly);
                geometrys.emplace_back(geo);
            }
            msbase::saveGeometrys(out, geometrys);
            ccglobal::cxndSaveT(out, point);
            ccglobal::cxndSaveT(out, calDist);
            return true;
        }

        bool load(std::fstream& in, int ver, ccglobal::Tracer* tracer) override
        {
            if (ver == 0) {
                msbase::CXNDGeometrys geometrys;
                msbase::loadGeometrys(in, geometrys);
                polys.reserve(geometrys.size());
                for (auto& geometry : geometrys) {
                    nestplacer::PR_Polygon poly;
                    poly.swap(geometry.contour);
                    polys.emplace_back(poly);
                }
                ccglobal::cxndLoadT(in, point);
                ccglobal::cxndLoadT(in, calDist);
                return true;
            }
            return false;
        }
    };

    void loadFromPrintFile(const std::string& fileName, std::vector<PR_Polygon>& polys, trimesh::vec3& mp, bool calDist)
    {
        PRInput input;
        if (!ccglobal::cxndLoad(input, fileName, nullptr)) {
            LOGE("printFromFile load error [%s]", fileName.c_str());
            return;
        }
        polys = input.polys;
        mp = input.point;
        calDist = input.calDist;
    }
    
    void Oritentation(const PR_Polygon& poly, PR_Polygon& result, bool counterClockWise = false)
    {
        float area = 0.0f;
        size_t len = poly.size();
        result.reserve(len);
        //逆时针轮廓面积为正
        for (size_t i = 0, j = len - 1; i < len; j = i++) {
            const auto& a = poly[i];
            const auto& b = poly[j];
            area += 0.5 * (b.x * a.y - b.y * a.x);
            result.emplace_back(a);
        }
        //确保输出轮廓为顺时针方向
        if ((area > 0.0f) && (!counterClockWise)) {
            std::reverse(result.begin(), result.end());
        } else if ((area < 0.0f) && counterClockWise) {
            std::reverse(result.begin(), result.end());
        }
    }
    
    PR_Polygon sweepAreaProfile(const PR_Polygon& subject, const PR_Polygon& object, const trimesh::vec3& mp, std::string* fileName)
    {
        if (fileName) {
            PRInput input;
            std::vector<PR_Polygon> polys;
            polys.emplace_back(subject);
            polys.emplace_back(object);
            input.polys = polys;
            input.point = mp;
            input.calDist = false;
            ccglobal::cxndSave(input, *fileName, nullptr);
        }
        PR_Polygon result;
        if (subject.empty() || object.empty()) return result;
        PR_Polygon station, orbit;
        Oritentation(subject, station);
        Oritentation(object, orbit);
        std::vector<trimesh::vec3> tranBs;
        tranBs.reserve(orbit.size());
        for (const auto& p : orbit) {
            tranBs.emplace_back(p - mp);
        }
        libnest2d::TMultiShape<Clipper3r::Polygon> transApolys;
        transApolys.reserve(tranBs.size());
        const size_t snums = station.size();
        for (const auto& tr : tranBs) {
            Clipper3r::Path path;
            path.reserve(snums);
            for (const auto& p : station) {
                path.emplace_back(convertIntPoint(p + tr));
            }
            Clipper3r::Polygon poly(path);
            transApolys.emplace_back(poly);
        }
        //计算
        std::vector<trimesh::vec3> tranAs;
        tranAs.reserve(station.size());
        for (const auto& p : station) {
            tranAs.emplace_back(p - mp);
        }
        libnest2d::TMultiShape<Clipper3r::Polygon> transBpolys;
        transBpolys.reserve(tranAs.size());
        const size_t onums = orbit.size();
        for (const auto& tr : tranAs) {
            Clipper3r::Path path;
            path.reserve(onums);
            for (const auto& p : orbit) {
                path.emplace_back(convertIntPoint(p + tr));
            }
            Clipper3r::Polygon poly(path);
            transBpolys.emplace_back(poly);
        }
        
#ifdef _WIN32
#ifdef _DEBUG
        if (false) {
            libnest2d::writer::ItemWriter<Clipper3r::Polygon> itemWriter;
            itemWriter.saveShapes(transApolys, "D://test/trackPolys", 3);
            itemWriter.saveShapes(transBpolys, "D://test/statePolys", 5);
        }
#endif // _DEBUG
#endif // _WIN32
        
        // merge polys
        libnest2d::TMultiShape<Clipper3r::Polygon> polys;
        libnest2d::TMultiShape<Clipper3r::Polygon> polysA = libnest2d::nfp::merge(transApolys);
        libnest2d::TMultiShape<Clipper3r::Polygon> polysB = libnest2d::nfp::merge(transBpolys);
        polys.insert(polys.end(), polysA.begin(), polysA.end());
        polys.insert(polys.end(), polysB.begin(), polysB.end());
        libnest2d::TMultiShape<Clipper3r::Polygon> mpolys = libnest2d::nfp::merge(polys);
#ifdef _WIN32
        #ifdef _DEBUG
        if (false) {
            libnest2d::writer::ItemWriter<Clipper3r::Polygon> itemWriter;
            Clipper3r::Polygon polyA;
            polyA.Contour.reserve(snums);
            for (const auto& p : station) {
                polyA.Contour.emplace_back(convertIntPoint(p));
            }
            itemWriter.saveShapes(polyA, polysA, polysB, mpolys, "D://test/mergePolys");
        }
        #endif // _DEBUG
#endif // _WIN32

        if (mpolys.empty()) return result;
        Clipper3r::Path contour = mpolys.front().Contour;
        double maxArea = std::fabs(Clipper3r::Area(contour));
        for (const auto& poly : mpolys) {
            const auto& path = poly.Contour;
            const double area = std::fabs(Clipper3r::Area(path));
            if (area > maxArea) {
                maxArea = area;
                contour = path;
            }
        }
        if (contour.front() == contour.back())
            contour.pop_back();
        PR_Polygon prList;
        prList.reserve(contour.size());
        for (const auto& p : contour) {
            prList.emplace_back(convertDoublePoint(p));
        }
        Oritentation(prList, result);
        std::reverse(result.begin(), result.end());
        return result;
    }

    void convertPolygon(const PR_Polygon& geometry, Clipper3r::Polygon& poly)
    {
        Clipper3r::Path contour;
        contour.reserve(geometry.size());
        for (const auto& p : geometry) {
            contour.emplace_back(convertIntPoint(p));
        }
        Clipper3r::Paths holes;
        /*holes.reserve(geometry.holes.size());
        for (const auto& hole : geometry.holes) {
            holes.emplace_back();
            holes.back().reserve(hole.size());
            for (const auto& p : hole) {
                holes.back().emplace_back(convertIntPoint(p));
            }
        }*/
        poly.Contour.swap(contour);
        poly.Holes.swap(holes);
    }
    
    bool pointInPolygon(const Clipper3r::IntPoint& pt, const Clipper3r::Path& input)
    {
        bool inside = false;
        const size_t len = input.size();
        for (const auto& v : input) {
            if (pt == v) return false;
        }
        for (size_t i = 0, j = len - 1; i < len; j = i++) {
            const auto& a = input[i];
            const auto& b = input[j];
            if (((a.Y > pt.Y) != (b.Y > pt.Y)) &&
                ((pt.X < (b.X - a.X) * (pt.Y - a.Y) / (b.Y - a.Y) + a.X))) inside = !inside;
        }
        return inside;
    }

    bool boundIntersect(const libnest2d::Item& rsh, const libnest2d::Item& other)
    {
        const libnest2d::Box& bbin = rsh.boundingBox();
        const libnest2d::Box& box = other.boundingBox();
        const auto& bmin = bbin.minCorner();
        const auto& bmax = bbin.maxCorner();
        const auto& min = box.minCorner();
        const auto& max = box.maxCorner();
        if (bmin.X > max.X || bmin.Y > max.Y || bmax.X < min.X || bmax.Y < min.Y) {
            return false;
        } else if ((bmin == max) || (bmax == min)) {
            return false;
        }
        return true;
    }
    
    double getPointPolygonDist(const Vertex& point, const Clipper3r::Polygon& poly) {
        
        auto dotProduct = [&](const Vertex & v1, const Vertex & v2) {
            return double(v1.X * v2.X + v1.Y * v2.Y);
        };

        auto magnitude2 = [&](const Vertex & vec) {
            return double(vec.X * vec.X + vec.Y * vec.Y);
        };

        auto getSegmentDistance = [&](const Vertex & p, const Vertex & a, const Vertex & b) {
            Vertex ab = b - a, ap = p - a;
            double k = dotProduct(ap, ab) / magnitude2(ab);
            Vertex nearPt;
            if (k < 0) {
                nearPt.X = a.X;
                nearPt.Y = a.Y;
            } else if (k > 1) {
                nearPt.X = b.X;
                nearPt.Y = b.Y;
            } else {
                nearPt.X = a.X * (1 - k) + b.X * k;
                nearPt.Y = a.Y * (1 - k) + b.Y * k;
            }
            return std::sqrt(magnitude2(p - nearPt));
        };

        auto calculateDist = [&](const Vertex & p, const Clipper3r::Path & input) {
            Clipper3r::Path path = input;
            if (path.front() == path.back()) {
                path.pop_back();
            }
            double d = std::numeric_limits<double>::max();
            const size_t nums = path.size();
            for (size_t i = 0; i < nums; ++i) {
                const auto& a = path[i];
                const auto& b = path[(i + 1 == nums) ? 0 : (i + 1)];
                double dist = getSegmentDistance(p, a, b);
                if (dist < d) {
                    d = dist;
                }
            }
            return d;
        };
        const auto& contour = poly.Contour;
        double minDist = calculateDist(point, contour);

        const auto& holes = poly.Holes;
        for (const auto& hole : holes) {
            double dist = calculateDist(point, hole);
            if (dist < minDist) {
                minDist = dist;
            }
        }
        return minDist;
    }

    bool pointOnPolygon(const Vertex& point, const Clipper3r::Polygon& poly)
    {
        auto magnitude2 = [&](const Vertex & vec) {
            return double(vec.X * vec.X + vec.Y * vec.Y);
        };

        auto vecUnit = [&](const Vertex & vec) {
            double d = std::sqrt(magnitude2(vec));
            return trimesh::dvec2((double)vec.X / d, (double)vec.Y / d);
        };

        auto dotProduct = [&](const Vertex & v1, const Vertex & v2) {
            return double(v1.X * v2.X + v1.Y * v2.Y);
        };

        auto crossProduct = [&](const trimesh::dvec2 & v1, const trimesh::dvec2 & v2) {
            return v1.x * v2.y - v1.y * v2.x;
        };

        auto pointOnSegment = [&](const Vertex & p, const Vertex & a, const Vertex & b) {
            if (p == a || p == b) return true;
            Vertex ab = b - a, ap = p - a;
            trimesh::dvec2 dir1 = vecUnit(ap);
            trimesh::dvec2 dir2 = vecUnit(ab);
            double k = dotProduct(ap, ab) / dotProduct(ab, ab);
            double area = std::fabs(crossProduct(dir1, dir2));
            return (k > 0 && k < 1) && (area < 1E-8);
        };

        auto pointOnPath = [&](const Vertex & p, const Clipper3r::Path & input) {
            Clipper3r::Path path = input;
            if (path.front() == path.back()) {
                path.pop_back();
            }
            const size_t nums = path.size();
            for (size_t i = 0; i < nums; ++i) {
                const auto& a = path[i];
                const auto& b = path[(i + 1 == nums) ? 0 : (i + 1)];
                if (pointOnSegment(p, a, b)) return true;
            }
            return false;
        };

        const auto& contour = poly.Contour;
        if (pointOnPath(point, contour)) return true;

        const auto& holes = poly.Holes;
        for (const auto& hole : holes) {
            if (pointOnPath(point, hole)) return true;
        }
        return false;
    }

    PR_RESULT checkTwoPolygon(const libnest2d::Item& rsh, const libnest2d::Item& other, bool orbconvex = true, bool calConvex = true, bool calDist = false)
    {
        PR_RESULT res;
        if (!boundIntersect(rsh, other)) {
            if (!calDist) {
                res.state = ContactState::SEPARATE;
                return res;
            }
        }
        const auto& sh = rsh.transformedShape();
        const auto& orb = other.transformedShape();
        bool shconvex = rsh.isContourConvex();
        libnest2d::nfp::NfpResult<Clipper3r::Polygon> subnfp;
        if (calConvex || (shconvex && orbconvex)) {
            subnfp = libnest2d::nfp::noFitPolygon<libnest2d::nfp::NfpLevel::CONVEX_ONLY, Clipper3r::Polygon>(sh, orb);
        } else {
            subnfp = libnest2d::nfp::noFitPolygon<libnest2d::nfp::NfpLevel::BOTH_CONCAVE, Clipper3r::Polygon>(sh, orb);
        }
        libnest2d::placers::correctNfpPosition<Clipper3r::Polygon>(subnfp, rsh, other);
        const libnest2d::TPoint<Clipper3r::Polygon>& rv = other.referenceVertex();

#ifdef _WIN32
#if _DEBUG
        if (false) {
            libnest2d::writer::ItemWriter<Clipper3r::Polygon> itemWriter;
            libnest2d::writer::ItemWriter<Clipper3r::Polygon>::SVGData datas;
            datas.items.emplace_back(std::ref(const_cast<libnest2d::Item&>(rsh)));
            datas.orsh = other;
            datas.nfps.emplace_back(std::ref(subnfp.first));
            datas.p = other.referenceVertex();
            itemWriter.saveItems(datas, "D://test/nfps");
        }
#endif
#endif // _WIN32

        const Clipper3r::Polygon& poly = subnfp.first;
        const Clipper3r::Path contour = poly.Contour;
        if (pointOnPolygon(rv, poly)) {
            res.state = ContactState::TANGENT;
            if (calDist) res.dist = getPointPolygonDist(rv, poly);
        } else if (pointInPolygon(rv, contour)) {
            res.state = ContactState::INTERSECT;
            if (calDist) res.dist = -getPointPolygonDist(rv, poly);
        } else {
            res.state = ContactState::SEPARATE;
            if (calDist) res.dist = getPointPolygonDist(rv, poly);
        }
        return res;
    }

    void collisionCheck(const std::vector<PR_Polygon>& polys, std::vector<PR_RESULT>& results, bool calConvex, bool calDist, std::string* fileName)
    {
        if (polys.empty()) return;
        if (fileName) {
            PRInput input;
            input.polys = polys;
            input.calDist = calDist;
            //input.point = trimesh::vec3(0, 0, 0);
            ccglobal::cxndSave(input, *fileName, nullptr);
        }
        const size_t nums = polys.size();
        std::vector<libnest2d::Item> inputs;
        inputs.reserve(nums);
        for (const auto& tmp : polys) {
            if (tmp.empty()) continue;
            PR_Polygon poly;
            Oritentation(tmp, poly);
            Clipper3r::Polygon geometry;
            convertPolygon(poly, geometry);
            auto con = geometry.Contour;
            if (con.front() != con.back()) {
                con.emplace_back(con.front());
                geometry.Contour.swap(con);
            }
            libnest2d::Item item(geometry);
            item.inflate(0);
            item.convexCal(calConvex);
            inputs.emplace_back(item);
        }
        if (inputs.empty()) return;
        std::launch policy = std::launch::deferred;
        policy |= std::launch::async;
        for (auto st = inputs.begin(); st != inputs.end(); ++st) {
            const libnest2d::Item& state = *st;
            bool orbconvex = state.isContourConvex();
            int start = std::distance(inputs.begin(), st);
            std::vector<PR_RESULT> remains(nums - start - 1);
            libnest2d::__parallel::enumerate(std::next(st), inputs.end(),
                [&remains, &state, &orbconvex, start, &calConvex, &calDist](const libnest2d::Item & orbit, size_t n) {
                remains[n].first = start;
                remains[n].second = n + start + 1;
                Clipper3r::Polygon sPoly = state.transformedShape();
                //A为逆时针轮廓,凸包处理时边为(first,next)
                if (Clipper3r::Area(sPoly.Contour) > 0) {
                    Clipper3r::ReversePath(sPoly.Contour);
                }
                //B为逆时针轮廓，凸包处理时边为(next,first)
                Clipper3r::Polygon oPoly = orbit.transformedShape();
                if (Clipper3r::Area(oPoly.Contour) > 0) {
                    Clipper3r::ReversePath(oPoly.Contour);
                }
                libnest2d::Item sItem(sPoly);
                libnest2d::Item oItem(oPoly);
                PR_RESULT res = checkTwoPolygon(sItem, oItem, orbconvex, calConvex, calDist);
                remains[n].state = res.state;
                remains[n].dist = INT2UM(res.dist);
            }, policy);
            results.insert(results.end(), remains.begin(), remains.end());
        }
    }
}
