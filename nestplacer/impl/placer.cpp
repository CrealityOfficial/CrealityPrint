#include <clipper3r/clipper.hpp>
#include "../libnest2d/tools/svgtools.hpp"
#include "msbase/utils/trimeshserial.h"
#include "nestplacer/placer.h"
#include "ccglobal/log.h"
#include "debug.h"
#include <random>

#define INT2UM(x) (static_cast<double>(x) / 10000.0)
#define UM2INT(x) (static_cast<Clipper3r::cInt>((x) * 10000.0 + 0.5 * (double((x) > 0) - ((x) < 0))))

namespace nestplacer
{
    class NestInput : public ccglobal::Serializeable {
    public:
        std::vector<nestplacer::PlacerItemGeometry> fixed;
        std::vector<nestplacer::PlacerItemGeometry> actives;
        PlacerParameter param;

        NestInput()
        {

        }
        ~NestInput()
        {

        }

        int version() override
        {
            return 0;
        }
        bool save(std::fstream& out, ccglobal::Tracer* tracer) override
        {
            msbase::CXNDGeometrys fgeometrys;
            fgeometrys.reserve(fixed.size());
            std::vector<int> fixIndexs;
            fixIndexs.reserve(fixed.size());
            for (auto& fitem : fixed) {
                msbase::CXNDGeometry geo;
                geo.contour.swap(fitem.outline);
                geo.holes.swap(fitem.holes);
                fgeometrys.emplace_back(geo);
                fixIndexs.emplace_back(fitem.binIndex);
            }
            msbase::saveGeometrys(out, fgeometrys);
            ccglobal::cxndSaveVectorT(out, fixIndexs);

            msbase::CXNDGeometrys ageometrys;
            ageometrys.reserve(actives.size());
            for (auto& aitem : actives) {
                msbase::CXNDGeometry geo;
                geo.contour.swap(aitem.outline);
                geo.holes.swap(aitem.holes);
                ageometrys.emplace_back(geo);
            }
            msbase::saveGeometrys(out, ageometrys);
            ccglobal::cxndSaveT(out, param.box.min);
            ccglobal::cxndSaveT(out, param.box.max);
            ccglobal::cxndSaveT(out, param.box.valid);

            ccglobal::cxndSaveT(out, param.concaveCal);
            ccglobal::cxndSaveT(out, param.itemGap);
            ccglobal::cxndSaveT(out, param.binItemGap);
            ccglobal::cxndSaveT(out, param.rotate);
            ccglobal::cxndSaveT(out, param.rotateAngle);
            ccglobal::cxndSaveT(out, param.needAlign);
            ccglobal::cxndSaveT(out, param.alignMode);
            ccglobal::cxndSaveT(out, param.startPoint);
            
            ccglobal::cxndSaveT(out, param.curBinId);
            ccglobal::cxndSaveT(out, param.binDist);
            msbase::CXNDPolygons polys;
            polys.reserve(param.multiBins.size());
            for (const auto& box : param.multiBins) {
                msbase::CXNDPolygon poly;
                poly.reserve(2);
                poly.emplace_back(box.min);
                poly.emplace_back(box.max);
                polys.emplace_back(poly);
            }
            msbase::savePolys(out, polys);
            return true;
        }
        bool load(std::fstream& in, int ver, ccglobal::Tracer* tracer) override
        {
            if (ver == 0) {
                msbase::CXNDGeometrys fgeometrys;
                msbase::loadGeometrys(in, fgeometrys);
                fixed.reserve(fgeometrys.size());
                std::vector<int> fixIndexs;
                ccglobal::cxndLoadVectorT(in, fixIndexs);
                for (int i = 0; i < fgeometrys.size(); ++i) {
                    auto geometry = fgeometrys[i];
                    PlacerItemGeometry pitem;
                    pitem.outline.swap(geometry.contour);
                    pitem.holes.swap(geometry.holes);
                    pitem.binIndex = fixIndexs[i];
                    fixed.emplace_back(pitem);
                }
                
                msbase::CXNDGeometrys ageometrys;
                msbase::loadGeometrys(in, ageometrys);
                actives.reserve(ageometrys.size());
                for (auto& geometry : ageometrys) {
                    PlacerItemGeometry pitem;
                    pitem.outline.swap(geometry.contour);
                    pitem.holes.swap(geometry.holes);
                    actives.emplace_back(pitem);
                }
                ccglobal::cxndLoadT(in, param.box.min);
                ccglobal::cxndLoadT(in, param.box.max);
                ccglobal::cxndLoadT(in, param.box.valid);

                ccglobal::cxndLoadT(in, param.concaveCal);
                ccglobal::cxndLoadT(in, param.itemGap);
                ccglobal::cxndLoadT(in, param.binItemGap);
                ccglobal::cxndLoadT(in, param.rotate);
                ccglobal::cxndLoadT(in, param.rotateAngle);
                ccglobal::cxndLoadT(in, param.needAlign);
                ccglobal::cxndLoadT(in, param.alignMode);
                ccglobal::cxndLoadT(in, param.startPoint);
                
                ccglobal::cxndLoadT(in, param.curBinId);
                ccglobal::cxndLoadT(in, param.binDist);
                msbase::CXNDPolygons polys;
                msbase::loadPolys(in, polys);
                param.multiBins.reserve(polys.size());
                for (const auto& poly : polys) {
                    trimesh::box3 box;
                    if (poly.size() == 2) {
                        for (const auto& p : poly) {
                            box += p;
                        }
                    }
                    param.multiBins.emplace_back(box);
                }
                return true;
            }
            return false;
        }
    };

    Clipper3r::IntPoint convertPoint(const trimesh::vec3& point)
    {
        return Clipper3r::IntPoint(UM2INT(point.x), UM2INT(point.y));
    }

    libnest2d::_Box<libnest2d::PointImpl> convertBox(const trimesh::box3& b)
    {
        Clipper3r::IntPoint minPoint(convertPoint(b.min));
        Clipper3r::IntPoint maxPoint(convertPoint(b.max));
        Clipper3r::IntPoint rect = maxPoint - minPoint;

        libnest2d::_Box<libnest2d::PointImpl> binBox
            = libnest2d::_Box<libnest2d::PointImpl>(rect.X, rect.Y, convertPoint(b.center()));
        return binBox;
    }

    void convertPolygon(const PlacerItemGeometry& geometry, Clipper3r::Polygon& poly)
    {
        Clipper3r::Path contour;
        contour.reserve(geometry.outline.size());
        for (const auto& p : geometry.outline) {
            contour.emplace_back(convertPoint(p));
        }
        Clipper3r::Paths holes;
        holes.reserve(geometry.holes.size());
        for (const auto& hole : geometry.holes) {
            holes.emplace_back();
            holes.back().reserve(hole.size());
            for (const auto& p : hole) {
                holes.back().emplace_back(convertPoint(p));
            }
        }
        poly.Contour.swap(contour);
        poly.Holes.swap(holes);
    }

    double PointLineDistance(const Clipper3r::IntPoint& pt, const Clipper3r::IntPoint& lineStart, const Clipper3r::IntPoint& lineEnd)
    {
        double dx = lineEnd.X - lineStart.X;
        double dy = lineEnd.Y - lineStart.Y;
        //Normalize
        double mag = std::pow(std::pow(dx, 2.0) + std::pow(dy, 2.0), 0.5);
        if (mag > 0.0) {
            dx /= mag; dy /= mag;
        }

        double pvx = pt.X - lineStart.X;
        double pvy = pt.Y - lineStart.Y;
        //Get dot product (project pv onto normalized direction)
        double pvdot = dx * pvx + dy * pvy;

        //Scale line direction vector
        double dsx = pvdot * dx;
        double dsy = pvdot * dy;

        //Subtract this from pv
        double ax = pvx - dsx;
        double ay = pvy - dsy;

        return std::pow(std::pow(ax, 2.0) + std::pow(ay, 2.0), 0.5);
    }

    void DouglasPeucker(const Clipper3r::Path& pointList, double epsilon, Clipper3r::Path& out)
    {
        if (pointList.size() < 2)
            return;

        // Find the point with the maximum distance from line between start and end
        double dmax = 0.0;
        size_t index = 0;
        size_t end = pointList.size() - 1;
        out.reserve(end + 1);
        for (size_t i = 1; i < end; i++) {
            double d = PointLineDistance(pointList[i], pointList[0], pointList[end]);
            if (d > dmax) {
                index = i;
                dmax = d;
            }
        }

        // If max distance is greater than epsilon, recursively simplify
        if (dmax > epsilon) {
            // Recursive call
            Clipper3r::Path recResults1;
            Clipper3r::Path recResults2;
            Clipper3r::Path firstLine(pointList.begin(), pointList.begin() + index + 1);
            Clipper3r::Path lastLine(pointList.begin() + index, pointList.end());
            DouglasPeucker(firstLine, epsilon, recResults1);
            DouglasPeucker(lastLine, epsilon, recResults2);

            // Build the result list
            out.assign(recResults1.begin(), recResults1.end() - 1);
            out.insert(out.end(), recResults2.begin(), recResults2.end());
            if (out.size() < 2)
                return;
        } else {
            //Just return start and end path
            out.clear();
            out.push_back(pointList[0]);
            out.push_back(pointList[end]);
        }
    }
    
    Clipper3r::Path pathOffset(const Clipper3r::Path& input, double distance)
    {
#define DISABLE_BOOST_OFFSET

        using Clipper3r::ClipperOffset;
        using Clipper3r::jtMiter;
        using Clipper3r::etClosedPolygon;
        using Clipper3r::Polygon;
        using Clipper3r::Paths;
        using Clipper3r::Path;
        Paths result;
        Path contour = input;
        if (contour.front() != contour.back())
            contour.emplace_back(contour.front());

        ClipperOffset offs;
        if (Clipper3r::Orientation(contour))
            offs.AddPath(contour, jtMiter, etClosedPolygon);
        else {
            Paths holes;
            holes.emplace_back(contour);
            offs.AddPaths(holes, jtMiter, etClosedPolygon);
        }
        offs.Execute(result, static_cast<double>(distance));

        // Offsetting reverts the orientation and also removes the last vertex
        // so boost will not have a closed polygon.
        Polygon sh;
        bool found_the_contour = false;
        for (auto& r : result) {
            if (Clipper3r::Orientation(r)) {
                // We don't like if the offsetting generates more than one contour
                // but throwing would be an overkill. Instead, we should warn the
                // caller about the inability to create correct geometries
                if (!found_the_contour) {
                    sh.Contour = std::move(r);
                    Clipper3r::ReversePath(sh.Contour);
                    auto front_p = sh.Contour.front();
                    sh.Contour.emplace_back(std::move(front_p));
                    found_the_contour = true;
                }
            } else {
                // TODO If there are multiple contours we can't be sure which hole
                // belongs to the first contour. (But in this case the situation is
                // bad enough to let it go...)
                sh.Holes.emplace_back(std::move(r));
                Clipper3r::ReversePath(sh.Holes.back());
                auto front_p = sh.Holes.back().front();
                sh.Holes.back().emplace_back(std::move(front_p));
            }
        }
        return sh.Contour;
    }

    Clipper3r::Paths pathUnit(const Clipper3r::Path& subject, const Clipper3r::Path& clip)
    {
        Clipper3r::Clipper clipper(Clipper3r::ioReverseSolution);
        using Clipper3r::ctUnion;
        using Clipper3r::pftEvenOdd;
        using Clipper3r::ptSubject;
        using Clipper3r::ptClip;
        Clipper3r::Paths paths;
        bool valid = false;
        valid &= clipper.AddPath(subject, ptSubject, true);
        valid &= clipper.AddPath(clip, ptClip, true);
        valid &= clipper.Execute(ctUnion, paths, pftEvenOdd);
        if (valid) {
            for (auto& path : paths) {
                if (path.front() != path.back())
                    path.emplace_back(path.front());
            }
        }
        return paths;
    }

    Clipper3r::Path pathSimplyfy(const Clipper3r::Path& pointList, double epsilon = 1.0)
    {
        if (pointList.empty()) return pointList;
        if (pointList.size() <= 20) return pointList;
        Clipper3r::Path input = pointList;
        if (input.front() != input.back()) {
            input.emplace_back(input.front());
        }
        Clipper3r::Path path;
        path.reserve(input.size());
        for (size_t i = 0; i < input.size() - 1; ++i) {
            const auto& start = input[i];
            const auto& end = input[i + 1];
            path.emplace_back();
            path.back().X = (start.X + end.X) / 2.0;
            path.back().Y = (start.Y + end.Y) / 2.0;
        }
        auto start = path.back();
        auto end = path.front();
        double dmax = PointLineDistance(input[0], start, end);
        for (size_t i = 1; i < input.size() - 1; ++i) {
            const auto& v = input[i];
            const auto& start = path[i - 1];
            const auto& end = path[i];
            double d = PointLineDistance(v, start, end);
            if (d > dmax) {
                dmax = d;
            }
        }
        Clipper3r::Path result;
        DouglasPeucker(pointList, std::min(dmax, epsilon), result);
        //result = pathOffset(result, std::min(dmax, epsilon));
        if (result.front() != result.back()) {
            result.emplace_back(result.front());
        }
#ifdef _WIN32
#if _DEBUG
        if (false) {
            libnest2d::writer::ItemWriter<Clipper3r::Polygon> itemWriter;
            itemWriter.savePaths(pointList, result, "D://test/paths");
        }
#endif
#endif // _WIN32

        return result;
    }

    Clipper3r::Polygon concaveSimplyfy(Clipper3r::Polygon& poly, double epsilon = 1.0)
    {
        Clipper3r::Polygon sh;
        sh.Contour = pathSimplyfy(poly.Contour, epsilon);
        sh.Holes.reserve(poly.Holes.size());
        for (const auto& hole : poly.Holes) {
            sh.Holes.emplace_back();
            sh.Holes.back() = pathSimplyfy(hole, epsilon);
        }
        return sh;
    }

    bool IntersectToBox(const libnest2d::Item& item, const libnest2d::Box& bbox)
    {
        const libnest2d::Box& box = item.boundingBox();
        const auto& bmin = box.minCorner();
        const auto& bmax = box.maxCorner();
        const auto& min = bbox.minCorner();
        const auto& max = bbox.maxCorner();
        if (bmin.X >= max.X || bmin.Y >= max.Y || bmax.X <= min.X || bmax.Y <= min.Y) {
            return false;
        }
        return true;
    }

    float randomValue() {
        std::random_device rd;  // 随机设备用于生成种子
        std::mt19937 gen(rd()); // Mersenne Twister 随机数引擎
        std::uniform_int_distribution<> dis(1, 4); // 均匀分布的整数范围
        int number = dis(gen); // 生成1到4之间的随机整数
        return (float)number * 0.001;
    }

    class PItem : public nestplacer::PlacerItem {
    public:
        PItem(const PlacerItemGeometry& geometry);
        virtual ~PItem();
        void polygon(nestplacer::PlacerItemGeometry& geometry) override;
        int binIndex = -1;
        std::vector<trimesh::vec3> contour;
        std::vector<std::vector<trimesh::vec3>> holes;
    };

    PItem::PItem(const nestplacer::PlacerItemGeometry& geometry)
    {
        contour = geometry.outline;
        holes = geometry.holes;
        binIndex = geometry.binIndex;
    }

    PItem::~PItem()
    {

    }

    void PItem::polygon(nestplacer::PlacerItemGeometry& geometry)
    {
        geometry.outline = contour;
        geometry.holes = holes;
        geometry.binIndex = binIndex;
    }

    void loadDataFromFile(const std::string& fileName, std::vector<PlacerItem*>& fixed, std::vector<PlacerItem*>& actives, PlacerParameter& parameter)
    {
        NestInput input;
        if (!ccglobal::cxndLoad(input, fileName, nullptr)) {
            LOGE("load data from file error [%s]", fileName.c_str());
            return;
        }
        fixed.clear(), actives.clear();
        fixed.reserve(input.fixed.size());
        actives.reserve(input.actives.size());
        for (const auto& fix : input.fixed) {
            PItem* pitem = new PItem(fix);
            fixed.emplace_back(pitem);
        }
        for (const auto& active : input.actives) {
            PItem* pitem = new PItem(active);
            actives.emplace_back(pitem);
        }
        parameter = input.param;
    }

    void placeFromFile(const std::string& fileName, std::vector<PlacerResultRT>& results, const BinExtendStrategy& binExtendStrategy, ccglobal::Tracer* tracer)
    {
        NestInput input;
        if (!ccglobal::cxndLoad(input, fileName, tracer)) {
            LOGE("placeFromFile load error [%s]", fileName.c_str());
            return;
        }
        std::vector<PlacerItem*> fixed, actives;
        fixed.reserve(input.fixed.size());
        actives.reserve(input.actives.size());
        for (const auto& fix : input.fixed) {
            PItem* pitem = new PItem(fix);
            fixed.emplace_back(pitem);
        }
        for (const auto& active : input.actives) {
            PItem* pitem = new PItem(active);
            actives.emplace_back(pitem);
        }
        PlacerParameter param = input.param;
        return place(fixed, actives, param, results, binExtendStrategy);
    }

    void extendFillFromFile(const std::string& fileName, std::vector<PlacerResultRT>& results, const BinExtendStrategy& binExtendStrategy,  ccglobal::Tracer* tracer)
    {
        NestInput input;
        if (!ccglobal::cxndLoad(input, fileName, tracer)) {
            LOGE("extendFillFromFile error [%s]", fileName.c_str());
            return;
        }
        std::vector<PlacerItem*> fixed, actives;
        fixed.reserve(input.fixed.size());
        actives.reserve(input.actives.size());
        for (const auto& fix : input.fixed) {
            PItem* pitem = new PItem(fix);
            fixed.emplace_back(pitem);
        }
        for (const auto& active : input.actives) {
            PItem* pitem = new PItem(active);
            actives.emplace_back(pitem);
        }
        PlacerItem* active = actives.front();
        PlacerParameter param = input.param;
        return extendFill(fixed, active, param, results);
    }

    void place(const std::vector<PlacerItem*>& fixed, const std::vector<PlacerItem*>& actives,
		const PlacerParameter& parameter, std::vector<PlacerResultRT>& results, const BinExtendStrategy& binExtendStrategy)
	{
        //debug模式下保存算法输入的原数据及配置参数
        if (!parameter.fileName.empty() && (!fixed.empty() || !actives.empty())) {
            NestInput input;
            std::vector<nestplacer::PlacerItemGeometry> pfixed;
            std::vector<nestplacer::PlacerItemGeometry> pactives;
            pfixed.reserve(fixed.size());
            for (PlacerItem* pitem : fixed) {
                nestplacer::PlacerItemGeometry geometry;
                pitem->polygon(geometry);
                pfixed.emplace_back(geometry);
            }
            pactives.reserve(actives.size());
            for (PlacerItem* pitem : actives) {
                nestplacer::PlacerItemGeometry geometry;
                pitem->polygon(geometry);
                pactives.emplace_back(geometry);
            }
            input.fixed.swap(pfixed);
            input.actives.swap(pactives);
            input.param = parameter;
            ccglobal::cxndSave(input, parameter.fileName, parameter.tracer);
        }
        //自定义的获取箱子/盘的函数对象
        auto box_func = [&binExtendStrategy](const int& index) {
            trimesh::box3 binBox = binExtendStrategy.bounding(index);
            libnest2d::Box box = convertBox(binBox);
            return box;
        };
        //libnest2d的配置器设置
        libnest2d::NestConfig<libnest2d::NfpPlacer, libnest2d::FirstFitSelection> config;
        config.placer_config.box_function = box_func;
        libnest2d::Box bbin = box_func(0);
        bool concaveCal = parameter.concaveCal;
        config.placer_config.calConcave = concaveCal;
        const float ramValue = randomValue();
        libnest2d::Coord itemGap = UM2INT(parameter.itemGap - ramValue);
        libnest2d::Coord edgeGap = UM2INT(parameter.binItemGap - ramValue);
        const double threshold = 0.5 * itemGap;

        config.placer_config.setStartPoint(parameter.startPoint);
        config.placer_config.setNewStartPoint(parameter.startPoint);
        config.placer_config.binItemGap = edgeGap;
        config.placer_config.minItemGap = itemGap;
        config.placer_config.needNewBin = true;
        if (parameter.needAlign) {
            config.placer_config.setAlignment(parameter.alignMode);
            config.placer_config.setNewAlignment(parameter.alignMode);
        } else {
            config.placer_config.alignment= libnest2d::NfpPlacer::Config::Alignment::DONT_ALIGN;
            config.placer_config.new_alignment = libnest2d::NfpPlacer::Config::Alignment::DONT_ALIGN;
        }

        if (parameter.rotate) {
            int angle = parameter.rotateAngle;
            config.placer_config.rotations.clear();
            if (angle >= 5) {
                int step = (int)(360.0f / angle);
                for (int i = 0; i < step; ++i) {
                    double theta = (double)i * angle;
                    libnest2d::Radians rotate = libnest2d::Degrees(theta);
                    config.placer_config.rotations.emplace_back(rotate);
                }
            }
            if(config.placer_config.rotations.empty())
                config.placer_config.rotations.emplace_back(0);
        } else {
            config.placer_config.rotations.clear();
            config.placer_config.rotations.emplace_back(0);
        }

        auto fix_box_check_func = [&binExtendStrategy](const int& index) {
            trimesh::box3 binBox = binExtendStrategy.fixBounding(index);
            libnest2d::Box box = convertBox(binBox);
            return box;
        };
        
        //输入的二维轮廓处理
        std::vector<libnest2d::Item> inputs;
        inputs.reserve(fixed.size() + actives.size());
        for (PlacerItem* pitem : fixed) {
            nestplacer::PlacerItemGeometry geometry;
            pitem->polygon(geometry);
            Clipper3r::Polygon sh, poly;
            convertPolygon(geometry, sh);
            int binid = geometry.binIndex;
            const auto& bbox = fix_box_check_func(binid);
            if (concaveCal) {
                poly = concaveSimplyfy(sh, threshold);
                libnest2d::Item item(poly);
                item.markAsFixedInBin(binid);
                if (!IntersectToBox(item, bbox)) {
                    results.emplace_back();
                    continue;
                }
                item.convexCal(false);
                inputs.emplace_back(item);
            } else {
                libnest2d::Item item(sh);
                item.markAsFixedInBin(binid);
                if (!IntersectToBox(item, bbox)) {
                    results.emplace_back();
                    continue;
                }
                inputs.emplace_back(item);
            }
        }
        for (PlacerItem* pitem : actives) {
            nestplacer::PlacerItemGeometry geometry;
            pitem->polygon(geometry);
            Clipper3r::Polygon sh, poly;
            convertPolygon(geometry, sh);
            if (concaveCal) {
                poly = concaveSimplyfy(sh, threshold);
                libnest2d::Item item(poly);
                item.convexCal(false);
                inputs.emplace_back(item);
            } else {
                libnest2d::Item item(sh);
                inputs.emplace_back(item);
            }
        }
        
        const size_t size = inputs.size();
        //libnest2d的控制器（配置回调函数）
        libnest2d::NestControl ctl;
        ctl.progressfn = [&size, &parameter](int remain) {
            if (parameter.tracer) {
                parameter.tracer->progress((float)((int)size - remain) / (float)size);
            }
        };
        ctl.stopcond = [&parameter]()->bool {
            if (parameter.tracer)
                return parameter.tracer->interrupt();
            return false;
        };
        
        //libnest2d库的入口，调用libnest2d排料算法。
        size_t bins = nest(inputs, bbin, itemGap, config, ctl);

        //返回值处理
        std::vector<PlacerResultRT> activeRts;
        activeRts.reserve(actives.size());
        for (size_t i = 0; i < inputs.size(); ++i) {
            const libnest2d::Item& item = inputs.at(i);
            PlacerResultRT pr;
            pr.rt.x = INT2UM(item.translation().X);
            pr.rt.y = INT2UM(item.translation().Y);
            pr.rt.z = item.rotation().toDegrees();
            pr.binIndex = item.binId();
            if (!item.isFixed())  // fixed item info not needed by outer call
                activeRts.emplace_back(pr);
        }
        results.insert(results.end(), activeRts.begin(), activeRts.end());
	}

	void extendFill(const std::vector<PlacerItem*>& fixed, PlacerItem* active, const PlacerParameter& parameter, 
        std::vector<PlacerResultRT>& results)
	{
        //debug模式下保存算法输入的原数据及配置参数
        if (!parameter.fileName.empty() && (!fixed.empty() || !active)) {
            NestInput input;
            std::vector<nestplacer::PlacerItemGeometry> pfixed;
            std::vector<nestplacer::PlacerItemGeometry> pactives;
            pfixed.reserve(fixed.size());
            for (PlacerItem* pitem : fixed) {
                nestplacer::PlacerItemGeometry geometry;
                pitem->polygon(geometry);
                pfixed.emplace_back(geometry);
            }

            {
                nestplacer::PlacerItemGeometry geometry;
                active->polygon(geometry);
                pactives.emplace_back(geometry);
            }
            input.fixed.swap(pfixed);
            input.actives.swap(pactives);
            input.param = parameter;
            ccglobal::cxndSave(input, parameter.fileName, parameter.tracer);
        }

        std::vector<libnest2d::Item> inputs;
        trimesh::box3 binBox = parameter.box;
        libnest2d::Box bbin = convertBox(binBox);
        const float ramValue = randomValue();
        libnest2d::Coord itemGap = UM2INT(parameter.itemGap - ramValue);
        libnest2d::Coord edgeGap = UM2INT(parameter.binItemGap - ramValue);
        const double threshold = 0.5 * itemGap;
        bool concaveCal = parameter.concaveCal;
        
        //libnest2d的配置器设置
        libnest2d::NestConfig<libnest2d::NfpPlacer, libnest2d::FirstFitSelection> config;
        config.placer_config.setStartPoint(parameter.startPoint);
        config.placer_config.setNewStartPoint(parameter.startPoint);
        config.placer_config.binItemGap = edgeGap;
        config.placer_config.minItemGap = itemGap;
        config.placer_config.calConcave = concaveCal;
        if (parameter.needAlign) {
            config.placer_config.setAlignment(parameter.alignMode);
            config.placer_config.setNewAlignment(parameter.alignMode);
        } else {
            config.placer_config.alignment = libnest2d::NfpPlacer::Config::Alignment::DONT_ALIGN;
            config.placer_config.new_alignment = libnest2d::NfpPlacer::Config::Alignment::DONT_ALIGN;
        }
        //自定义的获取箱子/盘的函数对象
        auto box_func = [&binBox, &parameter](const int& index) {
            libnest2d::Box box;
            if (index == parameter.curBinId) box = convertBox(binBox);
            return box;
        };
        config.placer_config.box_function = box_func;

        if (parameter.rotate) {
            int angle = parameter.rotateAngle;
            config.placer_config.rotations.clear();
            if (angle >= 5) {
                int step = (int)(360.0f / angle);
                for (int i = 0; i < step; ++i) {
                    double theta = (double)i * angle;
                    libnest2d::Radians rotate = libnest2d::Degrees(theta);
                    config.placer_config.rotations.emplace_back(rotate);
                }
            }
            if (config.placer_config.rotations.empty())
                config.placer_config.rotations.emplace_back(0);
        } else {
            config.placer_config.rotations.clear();
            config.placer_config.rotations.emplace_back(0);
        }
        
        //输入的二维轮廓处理
        double fixArea = 0;
        inputs.reserve(fixed.size());
        for (PlacerItem* pitem : fixed) {
            nestplacer::PlacerItemGeometry geometry;
            pitem->polygon(geometry);
            Clipper3r::Polygon sh, poly;
            convertPolygon(geometry, sh);
            int binid = geometry.binIndex;
            const auto& bbox = box_func(binid);
            if (concaveCal) {
                poly = concaveSimplyfy(sh, threshold);
                libnest2d::Item item(poly);
                if (!IntersectToBox(item, bbox)) {
                    results.emplace_back();
                    continue;
                }
                item.markAsFixedInBin(binid);
                item.convexCal(false);
                inputs.emplace_back(item);
                fixArea += std::fabs(item.area());
            } else {
                libnest2d::Item item(sh);
                if (!IntersectToBox(item, bbox)) {
                    results.emplace_back();
                    continue;
                }
                item.markAsFixedInBin(binid);
                inputs.emplace_back(item);
                fixArea += std::fabs(item.area());
            }
        }
        {
            if (!active) return;
            double binArea = bbin.area();
            nestplacer::PlacerItemGeometry geometry;
            active->polygon(geometry);
            Clipper3r::Polygon sh, poly;
            convertPolygon(geometry, sh);
            if (concaveCal) {
                poly = concaveSimplyfy(sh, threshold);
                poly.Contour.swap(sh.Contour);
                poly.Holes.swap(sh.Holes);
            }
            libnest2d::Item item(sh);
            if (concaveCal) {
                item.convexCal(false);
            }
            libnest2d::Item cpy(sh);
            cpy.inflate(threshold);
            double itemArea = std::fabs(cpy.area());
            int nums = std::floor((binArea - fixArea) / itemArea);
            inputs.reserve(fixed.size() + nums);
            for (int i = 0; i < nums; ++i) {
                inputs.emplace_back(item);
            }
        }
        
        const size_t size = inputs.size();
        //libnest2d库的控制器（回调函数设置）
        libnest2d::NestControl ctl;
        ctl.progressfn = [&size, &parameter](int remain) {
            if (parameter.tracer) {
                parameter.tracer->progress((float)((int)size - remain) / (float)size);
            }
        };
        ctl.stopcond = [&parameter]()->bool {
            if (parameter.tracer)
                return parameter.tracer->interrupt();
            return false;
        };

        //libnest2d库入口，调用libnest2d库的排料算法
        size_t bins = nest(inputs, bbin, itemGap, config, ctl);

        //返回值处理
        std::vector<PlacerResultRT> activeRts;
        activeRts.reserve(inputs.size());
        for (size_t i = 0; i < inputs.size(); ++i) {
            const libnest2d::Item& item = inputs.at(i);
            if (item.binId() < 0) continue;
            PlacerResultRT pr;
            pr.rt.x = INT2UM(item.translation().X);
            pr.rt.y = INT2UM(item.translation().Y);
            pr.rt.z = item.rotation().toDegrees();
            pr.binIndex = item.binId();
            if (!item.isFixed())  // fixed item info not needed
                activeRts.emplace_back(pr);
        }
        results.insert(results.end(), activeRts.begin(), activeRts.end());
	}

	YDefaultBinExtendStrategy::YDefaultBinExtendStrategy(const trimesh::box3& box, float dy)
		: BinExtendStrategy()
		, m_box(box)
		, m_dy(dy)
	{

	}

	YDefaultBinExtendStrategy::~YDefaultBinExtendStrategy()
	{

	}

	trimesh::box3 YDefaultBinExtendStrategy::bounding(int index) const
	{
		trimesh::box3 b = m_box;
		float y = b.size().y;
        b.min.y += (float)index * (y + m_dy);
        b.max.y += (float)index * (y + m_dy);
		return b;
	}

    DiagonalBinExtendStrategy::DiagonalBinExtendStrategy(const trimesh::box3& box, float rate)
        : BinExtendStrategy()
        , m_box(box)
        , m_ratio(rate)
    {
    }

    DiagonalBinExtendStrategy::~DiagonalBinExtendStrategy()
    {
    }

    trimesh::box3 DiagonalBinExtendStrategy::bounding(int index) const
    {
        trimesh::box3 b = m_box;
        trimesh::vec3 dir = b.size();
        b.min += (float)index * dir * (1.0 + m_ratio);
        b.max += (float)index * dir * (1.0 + m_ratio);
        return b;
    }    
    
    
}