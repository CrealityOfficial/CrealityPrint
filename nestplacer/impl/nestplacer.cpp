#include"nestplacer/nestplacer.h"
#include "data.h"

#include <libnest2d/libnest2d.hpp>

const double Pi = 3.141592653589793238463;
#define NEST_FACTOR  100.0

namespace nestplacer
{
    typedef std::function<void(int, const TransMatrix&)> PlaceFunc;
    typedef std::function<void(const TransMatrix&)> PlaceOneFunc;
    class NestItemer
    {
    public:
        virtual ~NestItemer() {}
        virtual const Clipper3r::Path& path() const = 0;
        virtual Clipper3r::IntPoint translate() = 0;
        virtual float rotation() = 0;
    };

    void InitCfg(libnest2d::NestConfig<libnest2d::NfpPlacer, libnest2d::FirstFitSelection>& cfg, NestParaCInt para)
    {
        if (para.packType != PlaceType::CONCAVE && para.packType != PlaceType::CONCAVE_ALL)
        {
            int step = std::max(1, para.rotationStep);
            float angle_per_step = 2 * Pi / step;
            cfg.placer_config.rotations.clear();
            for(int i = 0;i < step;i++)
                cfg.placer_config.rotations.push_back(libnest2d::Radians(i * angle_per_step));//多边形可用旋转角
        }
        cfg.placer_config.parallel = para.parallel;  //启用单线程

        switch (para.sp)
        {
        case StartPoint::CENTER: {
            cfg.placer_config.starting_point = libnest2d::NfpPlacer::Config::Alignment::CENTER;
            cfg.placer_config.alignment = libnest2d::NfpPlacer::Config::Alignment::CENTER;
        }break;
        case StartPoint::TOP_LEFT: {
            cfg.placer_config.starting_point = libnest2d::NfpPlacer::Config::Alignment::TOP_LEFT;
            cfg.placer_config.alignment = libnest2d::NfpPlacer::Config::Alignment::DONT_ALIGN;
        }break;
        case StartPoint::BOTTOM_LEFT: {
            cfg.placer_config.starting_point = libnest2d::NfpPlacer::Config::Alignment::BOTTOM_LEFT;
            cfg.placer_config.alignment = libnest2d::NfpPlacer::Config::Alignment::DONT_ALIGN;
        }break;
        case StartPoint::TOP_RIGHT: {
            cfg.placer_config.starting_point = libnest2d::NfpPlacer::Config::Alignment::TOP_RIGHT;
            cfg.placer_config.alignment = libnest2d::NfpPlacer::Config::Alignment::DONT_ALIGN;
        }break;
        case StartPoint::BOTTOM_RIGHT: {
            cfg.placer_config.starting_point = libnest2d::NfpPlacer::Config::Alignment::BOTTOM_RIGHT;
            cfg.placer_config.alignment = libnest2d::NfpPlacer::Config::Alignment::DONT_ALIGN;
        }break;
        case StartPoint::NULLTYPE:
            switch (para.packType)
            {
            case PlaceType::CENTER_TO_SIDE:
            case PlaceType::ALIGNMENT:
            case PlaceType::ONELINE:
            case PlaceType::CONCAVE: {
                cfg.placer_config.starting_point = libnest2d::NfpPlacer::Config::Alignment::CENTER;
                cfg.placer_config.alignment = libnest2d::NfpPlacer::Config::Alignment::CENTER;
            }break;
            case PlaceType::TOP_TO_BOTTOM: {
                cfg.placer_config.starting_point = libnest2d::NfpPlacer::Config::Alignment::TOP_LEFT;
                cfg.placer_config.alignment = libnest2d::NfpPlacer::Config::Alignment::DONT_ALIGN;
            }break;
            case PlaceType::BOTTOM_TO_TOP: {
                cfg.placer_config.starting_point = libnest2d::NfpPlacer::Config::Alignment::BOTTOM_LEFT;
                cfg.placer_config.alignment = libnest2d::NfpPlacer::Config::Alignment::DONT_ALIGN;
            }break;
            case PlaceType::LEFT_TO_RIGHT: {
                cfg.placer_config.starting_point = libnest2d::NfpPlacer::Config::Alignment::TOP_LEFT;
                cfg.placer_config.alignment = libnest2d::NfpPlacer::Config::Alignment::DONT_ALIGN;
            }break;
            case PlaceType::RIGHT_TO_LEFT: {
                cfg.placer_config.starting_point = libnest2d::NfpPlacer::Config::Alignment::TOP_RIGHT;
                cfg.placer_config.alignment = libnest2d::NfpPlacer::Config::Alignment::DONT_ALIGN;
            }break;
            }
            break;
        }
        
        //cfg.placer_config.accuracy; //优化率 
        //cfg.placer_config.explore_holes;  //孔内是否放置图元,目前源码中还未实现
        //cfg.placer_config.before_packing; //摆放下一个Item之前，先对前面已经摆放好的Item进行调整的函数，若为空，则Item拍好位置后将不再变动。传入函数接口
        //cfg.placer_config.object_function;  //添加优化方向，向函数输出值最小化优化，以此改变排放方式，传入函数接口

        //cfg.selector_config;

        if (para.packType != PlaceType::NULLTYPE)
        {
            cfg.placer_config.object_function = [para](const libnest2d::Item&)  //优化方向
            {
                switch (para.packType)
                {
                case PlaceType::CENTER_TO_SIDE: return 3; break;
                case PlaceType::ALIGNMENT: return 4; break;
                case PlaceType::ONELINE: return 5; break;
                case PlaceType::CONCAVE: return 6; break;
                case PlaceType::TOP_TO_BOTTOM: return 7; break;
                case PlaceType::BOTTOM_TO_TOP: return 8; break;
                case PlaceType::LEFT_TO_RIGHT: return 9; break;
                case PlaceType::RIGHT_TO_LEFT: return 10; break;
                case PlaceType::CONCAVE_ALL: return 11; break;
                }
                return 3;
            };
        }
    }

    Clipper3r::IntRect getRect(Clipper3r::Path path)
    {
        Clipper3r::ClipperBase clip;
        clip.AddPath(path, Clipper3r::ptSubject, true);
        return clip.GetBounds();
    }

    double PerpendicularDistance(const Clipper3r::IntPoint& pt, const Clipper3r::IntPoint& lineStart, const Clipper3r::IntPoint& lineEnd)
    {
        double dx = lineEnd.X - lineStart.X;
        double dy = lineEnd.Y - lineStart.Y;

        //Normalize
        double mag = pow(pow(dx, 2.0) + pow(dy, 2.0), 0.5);
        if (mag > 0.0)
        {
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

        return pow(pow(ax, 2.0) + pow(ay, 2.0), 0.5);
    }

    //polugon simplify
    void RamerDouglasPeucker(const Clipper3r::Path& pointList, double epsilon, Clipper3r::Path& out)
    {
        if (pointList.size() < 2)
            return ;

        // Find the point with the maximum distance from line between start and end
        double dmax = 0.0;
        size_t index = 0;
        size_t end = pointList.size() - 1;
        for (size_t i = 1; i < end; i++)
        {
            double d = PerpendicularDistance(pointList[i], pointList[0], pointList[end]);
            if (d > dmax)
            {
                index = i;
                dmax = d;
            }
        }

        // If max distance is greater than epsilon, recursively simplify
        if (dmax > epsilon)
        {
            // Recursive call
            Clipper3r::Path recResults1;
            Clipper3r::Path recResults2;
            Clipper3r::Path firstLine(pointList.begin(), pointList.begin() + index + 1);
            Clipper3r::Path lastLine(pointList.begin() + index, pointList.end());
            RamerDouglasPeucker(firstLine, epsilon, recResults1);
            RamerDouglasPeucker(lastLine, epsilon, recResults2);

            // Build the result list
            out.assign(recResults1.begin(), recResults1.end() - 1);
            out.insert(out.end(), recResults2.begin(), recResults2.end());
            if (out.size() < 2)
                return;
        }
        else
        {
            //Just return start and end points
            out.clear();
            out.push_back(pointList[0]);
            out.push_back(pointList[end]);
        }
    }

    TransMatrix getTransMatrixFromItem(libnest2d::Item iitem, Clipper3r::cInt offsetX, Clipper3r::cInt offsetY)
    {
        TransMatrix itemTrans;
        itemTrans.rotation = iitem.rotation().toDegrees();
        itemTrans.x = iitem.translation().X + offsetX;
        itemTrans.y = iitem.translation().Y + offsetY;
        return itemTrans;
    }
    
    Clipper3r::Path getItemPath(NestItemer* itemer, PlaceType type)
    {
        Clipper3r::Path newItemPath;
        if (type != PlaceType::CONCAVE) 
            newItemPath = libnest2d::shapelike::convexHull(itemer->path());
        else
            newItemPath = itemer->path();
        if (Clipper3r::Orientation(newItemPath))
        {
            Clipper3r::ReversePath(newItemPath);
        }
        return newItemPath;
    }

    Clipper3r::Path ItemPathDataTrans(std::vector<trimesh::vec3> model, bool pretreatment)
    {
        Clipper3r::Path result;
        for (trimesh::vec3 pt : model)
        {
            Clipper3r::IntPoint pt_t;
            pt_t.X = pt.x * NEST_FACTOR;
            pt_t.Y = pt.y * NEST_FACTOR;
            result.push_back(pt_t);
        }
        if (pretreatment)
        {
            if (Clipper3r::Orientation(result))
            {
                Clipper3r::ReversePath(result);
            }
            result = libnest2d::shapelike::convexHull(result);
        }
        return result;
    }

    int bOnTheEdge(Clipper3r::Path item_path, int width, int height)
    {
        Clipper3r::Path rect = {
            {0, 0},
            {width, 0},
            {width, height},
            {0, height}
        };
        Clipper3r::Paths result;
        Clipper3r::Clipper a;
        a.AddPath(rect, Clipper3r::ptSubject, true);
        a.AddPath(item_path, Clipper3r::ptClip, true);
        a.Execute(Clipper3r::ctIntersection, result, Clipper3r::pftNonZero, Clipper3r::pftNonZero);
        if (result.empty())
            return 0;
        double inter_area = fabs(Clipper3r::Area(result[0]));
        double diff_area = fabs(inter_area - fabs(Clipper3r::Area(item_path)));
        if (inter_area > 0 && diff_area > 1000) return 1;
        if (diff_area < 1000) return 2;
        return 0;
    }

    bool nest2d_base(const Clipper3r::Paths& ItemsPaths, NestParaCInt para, std::vector<TransMatrix>& transData)
    {
        PlaceType type = para.packType;
        size_t size = ItemsPaths.size();
        transData.resize(size);

        libnest2d::NestControl ctl;
        ctl.progressfn = [&size, &para](int remain) {
            if (para.tracer)
            {
                para.tracer->progress((float)((int)size - remain) / (float)size);
            }
        };
        ctl.stopcond = [&para]()->bool {
            if (para.tracer)
                return para.tracer->interrupt();
            return false;
        };

        libnest2d::NestConfig<libnest2d::NfpPlacer, libnest2d::FirstFitSelection> cfg;
        InitCfg(cfg, para);

        auto convert = [&ItemsPaths, type](Clipper3r::Path& oItem, int index) {
            Clipper3r::Path lines = ItemsPaths.at(index);
            Clipper3r::Clipper a;
            a.AddPath(lines, Clipper3r::ptSubject, true);
            Clipper3r::IntRect r = a.GetBounds();
            oItem.resize(4);
            oItem[0] = Clipper3r::IntPoint(r.left, r.bottom);
            oItem[1] = Clipper3r::IntPoint(r.right, r.bottom);
            oItem[2] = Clipper3r::IntPoint(r.right, r.top);
            oItem[3] = Clipper3r::IntPoint(r.left, r.top);
        };

        std::vector<libnest2d::Item> input;
        for (int i = 0; i < size; i++)
        {
            Clipper3r::Path ItemPath;
            if (type == PlaceType::ALIGNMENT) convert(ItemPath, i);
            else
            {
                ItemPath = ItemsPaths[i];
            }
                
            if (Clipper3r::Orientation(ItemPath))
            {
                Clipper3r::ReversePath(ItemPath);//正轮廓倒序
            }
            if (type != PlaceType::CONCAVE && type != PlaceType::CONCAVE_ALL) ItemPath = libnest2d::shapelike::convexHull(ItemPath);
            libnest2d::Item item = libnest2d::Item(ItemPath);
            if (type == PlaceType::CONCAVE || type == PlaceType::CONCAVE_ALL) item.convexCal(false);
            input.push_back(item);
        }

        Clipper3r::cInt imgW_dst = para.workspaceW, imgH_dst = para.workspaceH;
        double offsetX = 0., offsetY = 0.;
        int egde_dist = 10;//排样到边缘最近距离为5单位
        if (input.size() == 1 && (input.back().boundingBox().height() >= imgH_dst || input.back().boundingBox().width() >= imgW_dst)) egde_dist = 0;
        if (egde_dist > para.modelsDist) egde_dist = para.modelsDist;
        imgW_dst += para.modelsDist - egde_dist;
        imgH_dst += para.modelsDist - egde_dist;
        offsetX += (para.modelsDist - egde_dist) / 2;
        offsetY += (para.modelsDist - egde_dist) / 2;
        if (para.modelsDist == 0) para.modelsDist = 1;
        libnest2d::Box maxBox = libnest2d::Box(imgW_dst, imgH_dst, { imgW_dst / 2, imgH_dst / 2 });

        std::size_t result = libnest2d::nest(input, maxBox, para.modelsDist, cfg, ctl);


        Clipper3r::Clipper a;
        for (int i = 0; i < size; i++)
        {
            double angle = input.at(i).rotation().toDegrees();
            if (angle != -90.)//区域内模型
            {
                auto trans_item = input.at(i).transformedShape_s();
                a.AddPath(trans_item.Contour, Clipper3r::ptSubject, true);
            }
        }
        Clipper3r::IntRect ibb_dst = a.GetBounds();
        int center_offset_x = 0.5 * para.workspaceW - (0.5 * (ibb_dst.right + ibb_dst.left) + offsetX);
        int center_offset_y = 0.5 * para.workspaceH - (0.5 * (ibb_dst.bottom + ibb_dst.top) + offsetY);
        if (type == PlaceType::ONELINE) center_offset_y = -ibb_dst.top - offsetY;
        if (type == PlaceType::TOP_TO_BOTTOM || type == PlaceType::BOTTOM_TO_TOP || type == PlaceType::LEFT_TO_RIGHT || type == PlaceType::RIGHT_TO_LEFT)
        {
            center_offset_x = -2 * offsetX;
            center_offset_y = -2 * offsetY;
        }

        std::vector<libnest2d::Item> input_outer_items(1,
            {
                {0, para.workspaceH},
                {para.workspaceW, para.workspaceH},
                {para.workspaceW, 0},
                {0, 0},
                {0, para.workspaceH}
            });//定义第一个轮廓遮挡中心排样区域

        input_outer_items[0].translation({ 1, 0 }); //packed mark
        if (type == PlaceType::CONCAVE || type == PlaceType::CONCAVE_ALL) input_outer_items[0].convexCal(false);
        for (int i = 0; i < size; i++)
        {
            libnest2d::Item& iitem = input.at(i);

            int model_offset_x = iitem.translation().X;
            int model_offset_y = iitem.translation().Y;
            double angle = iitem.rotation().toDegrees();

            if (angle == -90.)
            {
                if (type == PlaceType::CONCAVE || type == PlaceType::CONCAVE_ALL)
                {
                    auto convex_path = libnest2d::shapelike::convexHull(iitem.rawShape().Contour);
                    input_outer_items.push_back(libnest2d::Item(convex_path));
                }
                else
                {
                    input_outer_items.push_back(libnest2d::Item(iitem.rawShape()));//收集排样区域外模型
                }
            }
            else
            {
                model_offset_x += center_offset_x;
                model_offset_y += center_offset_y;
                TransMatrix itemTrans;
                itemTrans.rotation = input[i].rotation().toDegrees();
                itemTrans.x = model_offset_x + offsetX;
                itemTrans.y = model_offset_y + offsetY;
                transData[i] = itemTrans;
            }
        }

          //////区域外模型排样
        if (input_outer_items.size() == 1) return true;
        cfg.placer_config.object_function = NULL;
        cfg.placer_config.starting_point = libnest2d::NfpPlacer::Config::Alignment::CENTER;
        cfg.placer_config.alignment = libnest2d::NfpPlacer::Config::Alignment::DONT_ALIGN;
        imgW_dst = para.workspaceW * 1001; imgH_dst = para.workspaceH * 1001;
        maxBox = libnest2d::Box(imgW_dst, imgH_dst, { para.workspaceW / 2, para.workspaceH / 2 });
        std::size_t result_dst = libnest2d::nest(input_outer_items, maxBox, para.modelsDist, cfg, ctl);
        int idx = 1;
        for (int i = 0; i < size; i++)
        {
            libnest2d::Item& iitem = input.at(i);
            double angle = iitem.rotation().toDegrees();
            if (angle == -90.)
            {
                libnest2d::Item& iitem_dst = input_outer_items.at(idx);
                TransMatrix itemTrans;
                itemTrans.rotation = iitem_dst.rotation().toDegrees();
                itemTrans.x = iitem_dst.translation().X;
                itemTrans.y = iitem_dst.translation().Y;
                transData[i] = itemTrans;
                idx++;
            }
        }

        return result;
    }

    Clipper3r::IntPoint RotateByVector(Clipper3r::IntPoint old_pt, Clipper3r::IntPoint offset, double c, double s)
    {
        double new_x = c * old_pt.X - s * old_pt.Y;
        double new_y = s * old_pt.X + c * old_pt.Y;
        Clipper3r::IntPoint output = Clipper3r::IntPoint(new_x, new_y);
        output += offset;
        return output;
    }

    Clipper3r::Path RotToMinRect(Clipper3r::Path result, std::vector<TransMatrix>& transData)
    {
        Clipper3r::Path resultPath;
        Clipper3r::cInt minArea = LONG_MAX;
        double minAreaAngle = 0;
        double anglePerStep = 5;
        int stepNum = 180 / anglePerStep;
        for (int i = 0; i < stepNum; i++)
        {
            Clipper3r::Path rotPath;
            rotPath.reserve(result.size());
            double r = anglePerStep * i * M_PIf / 180;
            double c = cos(r);
            double s = sin(r);
            for (Clipper3r::IntPoint pt : result)
            {
                rotPath .push_back(RotateByVector(pt, Clipper3r::IntPoint(), c, s));
            }
            Clipper3r::IntRect pathRect = getRect(rotPath);
            Clipper3r::cInt rectArea = (pathRect.right - pathRect.left) * (pathRect.bottom - pathRect.top);
            if (rectArea < minArea && ((pathRect.right - pathRect.left) > (pathRect.bottom - pathRect.top)))
            {
                minArea = rectArea;
                resultPath = rotPath;
                minAreaAngle = anglePerStep * i;
            }
        }
        for (int j = 0; j < transData.size(); j++)
        {
            transData[j].merge(TransMatrix(0, 0, minAreaAngle));
        }
        return resultPath;
    }

    Clipper3r::Path getPairConcave(Clipper3r::Paths pairPaths, std::vector<TransMatrix>& transData, bool changMat = true)
    {
        Clipper3r::Path allPts, ConcaveHull;
        for (int i = 0; i < pairPaths.size(); i++)
        {
            double r = transData[i].rotation * M_PIf / 180;
            double c = cos(r);
            double s = sin(r);
            Clipper3r::IntPoint offset = Clipper3r::IntPoint(transData[i].x, transData[i].y);
            for (Clipper3r::IntPoint pt : pairPaths[i])
            {
                allPts.push_back(RotateByVector(pt, offset, c, s));
            }
        }
        ConcaveHull = polygonLib::PolygonPro::polygonConcaveHull(allPts, 0.2);

        Clipper3r::IntRect pathRect = getRect(ConcaveHull);
        Clipper3r::IntPoint offsetC = Clipper3r::IntPoint((pathRect.left + pathRect.right) / 2, (pathRect.top + pathRect.bottom) / 2);
        for (Clipper3r::IntPoint& pt : ConcaveHull)
        {
            pt -= offsetC;
        }
        if (changMat)
        {
            for (int i = 0; i < transData.size(); i++)
            {
                transData[i].x -= offsetC.X;
                transData[i].y -= offsetC.Y;
            }
        }
     
        return ConcaveHull;
    }

    void pairPackPro(Clipper3r::Paths itemPath, std::vector<TransMatrix>& transData, NestParaCInt para, int totalNum)
    {
        int inputSize = itemPath.size();
        transData.resize(inputSize);
        int pair_num = inputSize / 2;
        if (pair_num == 0) return;
        if (2 * inputSize == totalNum) para.packType = PlaceType::BOTTOM_TO_TOP;
        else para.packType = PlaceType::CONCAVE;
        bool changMat = pair_num != 1;;
        std::vector < std::vector<TransMatrix>> transData_pair(pair_num);
        Clipper3r::Paths pair_convex(pair_num);
#if defined(_OPENMP)
#pragma omp parallel for
#endif
        for (int i = 0; i < pair_num; i++)
        {
            Clipper3r::Paths allItem_pair(2);
            allItem_pair[0] = itemPath[2 * i];
            allItem_pair[1] = itemPath[2 * i + 1];
            nest2d_base(allItem_pair, para, transData_pair[i]);
            pair_convex[i] = getPairConcave(allItem_pair, transData_pair[i], changMat);
            if (totalNum == inputSize)
                pair_convex[i] = RotToMinRect(pair_convex[i], transData_pair[i]);
        }

        std::vector<TransMatrix> transData_low;
        pairPackPro(pair_convex, transData_low, para, totalNum);

        for (int i = 0; i < inputSize; i++)
        {
            int pairIdx = i / 2;
            transData[i].merge(transData_pair[pairIdx][(i - 2 * pairIdx)]);
            transData[i].merge(transData_low[i/2]);
        }

    }

    bool nest2d(const std::vector<NestItemer*>& items, NestParaCInt para, PlaceFunc func)
    {
        Clipper3r::Paths ItemsPaths;
        for (NestItemer* itemer : items)
            ItemsPaths.emplace_back(getItemPath(itemer, para.packType));
        std::vector<TransMatrix> transData;
        bool nestResult = nest2d_base(ItemsPaths, para, transData);

        if (func && nestResult)
        {
            size_t size = transData.size();
            for (size_t i = 0; i < size; i++)
                func((int)i, transData.at(i));
        }
        return nestResult;
    }

    bool nest2d_base(const Clipper3r::Paths& ItemsPaths, const Clipper3r::Path& transData, const Clipper3r::Path& newItemPath, 
        NestParaCInt para, TransMatrix& NewItemTransData)
    {
        Clipper3r::cInt offsetX = 0, offsetY = 0;
        int egde_dist = 100;
        if (egde_dist > para.modelsDist) egde_dist = para.modelsDist;
        offsetX += (para.modelsDist - egde_dist) / 2;
        offsetY += (para.modelsDist - egde_dist) / 2;

        libnest2d::NestConfig<libnest2d::NfpPlacer, libnest2d::FirstFitSelection> cfg;
        cfg.placer_config.alignment = libnest2d::NfpPlacer::Config::Alignment::DONT_ALIGN;
        InitCfg(cfg, para);
        std::vector<libnest2d::Item> input;
        std::vector<libnest2d::Item> input_out_pack(1,
            {
                {0, para.workspaceH},
                {para.workspaceW, para.workspaceH},
                {para.workspaceW, 0},
                {0, 0},
                {0, para.workspaceH}
            });//定义第一个轮廓遮挡中心排样区域
        input_out_pack[0].translation({ 1, 0 }); //packed mark
        for (int i = 0; i < ItemsPaths.size(); i++)
        {
            Clipper3r::Path ItemPath = ItemsPaths[i];
            libnest2d::Item item = libnest2d::Item(ItemPath);
            Clipper3r::IntPoint trans_data = transData[i];
            if (para.packType == PlaceType::CONCAVE) item.convexCal(false);
            item.translation({ trans_data.X, trans_data.Y });
            auto trans_item = item.transformedShape_s();
            if (bOnTheEdge(trans_item.Contour, para.workspaceW, para.workspaceH) == 2)
            {
                input.push_back(item);
            }
            else
            {
                input_out_pack.push_back(item);
            }
        }

        auto can_pack_mid = [&input, para](libnest2d::PolygonImpl nItem)
        {
            libnest2d::sl::offset(nItem, para.modelsDist);
            Clipper3r::Paths result;
            Clipper3r::Clipper a;
            for (libnest2d::Item dItem : input)
            {
                auto trans_item = dItem.transformedShape_s();
                a.AddPath(trans_item.Contour, Clipper3r::ptSubject, true);
            }
            a.AddPath(nItem.Contour, Clipper3r::ptClip, true);
            a.Execute(Clipper3r::ctIntersection, result, Clipper3r::pftNonZero, Clipper3r::pftNonZero);
            return result.empty();
        };

        libnest2d::Item newItem = libnest2d::Item(newItemPath);
        bool can_pack = false;
        Clipper3r::cInt imgW_dst = para.workspaceW, imgH_dst = para.workspaceH;
        libnest2d::Box maxBox = libnest2d::Box(imgW_dst + 2 * offsetX, imgH_dst + 2 * offsetY, { para.workspaceW / 2, para.workspaceH / 2 });
        newItem.translation({ para.workspaceW / 2, para.workspaceH / 2 });
        for (auto rot : cfg.placer_config.rotations) {
            newItem.rotation(rot);
            auto trans_item = newItem.transformedShape_s();
            if (bOnTheEdge(trans_item.Contour, para.workspaceW, para.workspaceH) == 2)
            {
                if (can_pack_mid(trans_item))
                {
                    NewItemTransData.x = newItem.translation().X;
                    NewItemTransData.y = newItem.translation().Y;
                    NewItemTransData.rotation = newItem.rotation().toDegrees();
                    return true;
                }
                can_pack = true;
                break;
            }
        }
        if (!can_pack)
        {
            newItem.translation({ 0, 0 });
            newItem.rotation(0);
            input_out_pack.push_back(newItem);
        }
        else
        {
            newItem.translation({ 0, 0 });
            if (para.packType == PlaceType::CONCAVE) newItem.convexCal(false);
            input.push_back(newItem);
            if (para.modelsDist == 0) para.modelsDist = 1;
            std::size_t result = libnest2d::nest(input, maxBox, para.modelsDist, cfg, libnest2d::NestControl());

            int total_size = input.size();
            libnest2d::Item iitem = input.at(total_size - 1);
            if (iitem.rotation().toDegrees() == -90.)
            {
                iitem.rotation(0);
                iitem.translation({ 0, 0 });
                input_out_pack.push_back(iitem);
            }
            else
            {
                NewItemTransData.x = iitem.translation().X;
                NewItemTransData.y = iitem.translation().Y;
                NewItemTransData.rotation = iitem.rotation().toDegrees();
                return true;
            }
        }

        cfg.placer_config.object_function = NULL;
        cfg.placer_config.starting_point = libnest2d::NfpPlacer::Config::Alignment::CENTER;
        imgW_dst = para.workspaceW * 1001; imgH_dst = para.workspaceH * 1001;
        maxBox = libnest2d::Box(imgW_dst, imgH_dst, { para.workspaceW / 2, para.workspaceH / 2 });
        libnest2d::nest(input_out_pack, maxBox, para.modelsDist, cfg, libnest2d::NestControl());
        libnest2d::Item oitem = input_out_pack.at(input_out_pack.size() - 1);
        NewItemTransData.x = oitem.translation().X;
        NewItemTransData.y = oitem.translation().Y;
        NewItemTransData.rotation = oitem.rotation().toDegrees();
        return false;
    }

    bool nest2d(const std::vector<NestItemer*>& items, NestItemer* item, NestParaCInt para, PlaceOneFunc func)
    {
        Clipper3r::Paths ItemsPaths;
        Clipper3r::Path transData_cInt;
        for (NestItemer* itemer : items)
        {
            ItemsPaths.emplace_back(getItemPath(itemer, para.packType));
            transData_cInt.emplace_back(itemer->translate());
        }

        Clipper3r::Path newItemPath = getItemPath(item, para.packType);
        TransMatrix NewItemTransData;
        bool can_pack = nest2d_base(ItemsPaths, transData_cInt, newItemPath, para, NewItemTransData);

        func(NewItemTransData);
        return can_pack;
    }

    void nest2d_base(const Clipper3r::Paths& ItemsPaths, const Clipper3r::Path& transData, const Clipper3r::Paths& newItemPaths,
        NestParaCInt para, std::vector<TransMatrix>& NewItemTransDatas)
    {
        NewItemTransDatas.resize(newItemPaths.size());
        Clipper3r::cInt offsetX = 0, offsetY = 0;
        int egde_dist = 100;
        if (egde_dist > para.modelsDist) egde_dist = para.modelsDist;
        offsetX += (para.modelsDist - egde_dist) / 2;
        offsetY += (para.modelsDist - egde_dist) / 2;

        libnest2d::NestConfig<libnest2d::NfpPlacer, libnest2d::FirstFitSelection> cfg;
        cfg.placer_config.alignment = libnest2d::NfpPlacer::Config::Alignment::DONT_ALIGN;
        InitCfg(cfg, para);
        std::vector<libnest2d::Item> input;
        std::vector<libnest2d::Item> input_out_pack(1,
            {
                {0, para.workspaceH},
                {para.workspaceW, para.workspaceH},
                {para.workspaceW, 0},
                {0, 0},
                {0, para.workspaceH}
            });//定义第一个轮廓遮挡中心排样区域
        input_out_pack[0].translation({ 1, 0 }); //packed mark
        for (int i = 0; i < ItemsPaths.size(); i++)
        {
            Clipper3r::Path ItemPath = ItemsPaths[i];
            libnest2d::Item item = libnest2d::Item(ItemPath);
            Clipper3r::IntPoint trans_data = transData[i];
            if (para.packType == PlaceType::CONCAVE) item.convexCal(false);
            item.translation({ trans_data.X, trans_data.Y });
            auto trans_item = item.transformedShape_s();
            if (bOnTheEdge(trans_item.Contour, para.workspaceW, para.workspaceH) == 2)
            {
                input.push_back(item);
            }
            else
            {
                input_out_pack.push_back(item);
            }
        }

        int packedInputNum = input.size();
        int packedOutNum = input_out_pack.size();
        std::vector<int> NonPackedInputId;
        std::vector<int> NonPackedOutId;
        Clipper3r::cInt imgW_dst = para.workspaceW, imgH_dst = para.workspaceH;
        libnest2d::Box maxBox = libnest2d::Box(imgW_dst + 2 * offsetX, imgH_dst + 2 * offsetY, { para.workspaceW / 2, para.workspaceH / 2 });
        for (int i = 0; i < newItemPaths.size(); i++)
        {
            libnest2d::Item newItem = libnest2d::Item(newItemPaths[i]);
            bool can_pack = false;
            newItem.translation({ para.workspaceW / 2, para.workspaceH / 2 });
            for (auto rot : cfg.placer_config.rotations) {
                newItem.rotation(rot);
                auto trans_item = newItem.transformedShape_s();
                if (bOnTheEdge(trans_item.Contour, para.workspaceW, para.workspaceH) == 2)
                {
                    can_pack = true;
                    break;
                }
            }
            if (!can_pack)
            {
                newItem.translation({ 0, 0 });
                newItem.rotation(0);
                input_out_pack.push_back(newItem);
                NonPackedOutId.push_back(i);
            }
            else
            {
                newItem.translation({ 0, 0 });
                if (para.packType == PlaceType::CONCAVE) newItem.convexCal(false);
                input.push_back(newItem);
                NonPackedInputId.push_back(i);
            }
        }

        if (para.modelsDist == 0) para.modelsDist = 1;
        std::size_t result = libnest2d::nest(input, maxBox, para.modelsDist, cfg, libnest2d::NestControl());

        int total_size = input.size();
        for (int i = packedInputNum; i < total_size; i++)
        {
            libnest2d::Item iitem = input.at(i);
            int idx = NonPackedInputId[i - packedInputNum];
            if (iitem.rotation().toDegrees() == -90.)
            {
                iitem.rotation(0);
                iitem.translation({ 0, 0 });
                input_out_pack.push_back(iitem);
                NonPackedOutId.push_back(idx);
            }
            else
            {
                TransMatrix NewItemTransData;
                NewItemTransData.x = iitem.translation().X;
                NewItemTransData.y = iitem.translation().Y;
                NewItemTransData.rotation = iitem.rotation().toDegrees();
                NewItemTransDatas[idx] = NewItemTransData;
            }
        }

        if (packedOutNum == input_out_pack.size()) return;
        cfg.placer_config.object_function = NULL;
        cfg.placer_config.starting_point = libnest2d::NfpPlacer::Config::Alignment::CENTER;
        imgW_dst = para.workspaceW * 1001; imgH_dst = para.workspaceH * 1001;
        maxBox = libnest2d::Box(imgW_dst, imgH_dst, { para.workspaceW / 2, para.workspaceH / 2 });
        libnest2d::nest(input_out_pack, maxBox, para.modelsDist, cfg, libnest2d::NestControl());

        total_size = input_out_pack.size();
        for (int i = packedOutNum; i < total_size; i++)
        {
            int idx = NonPackedOutId[i - packedOutNum];
            libnest2d::Item oitem = input_out_pack.at(i);
            TransMatrix NewItemTransData;
            NewItemTransData.x = oitem.translation().X;
            NewItemTransData.y = oitem.translation().Y;
            NewItemTransData.rotation = oitem.rotation().toDegrees();
            NewItemTransDatas[idx] = NewItemTransData;
        }  
    }

    void layout_new_items(const std::vector < std::vector<trimesh::vec3>>& models, const std::vector<trimesh::vec3>& transData,
        const std::vector < std::vector<trimesh::vec3>>& NewItems, NestParaFloat para, std::function<void(int, trimesh::vec3)> func)
    {
        trimesh::box3 basebox = para.workspaceBox;
        Clipper3r::cInt w = (basebox.max.x - basebox.min.x) * NEST_FACTOR;
        Clipper3r::cInt h = (basebox.max.y - basebox.min.y) * NEST_FACTOR;
        Clipper3r::cInt d = para.modelsDist * NEST_FACTOR;
        NestParaCInt para_cInt = NestParaCInt(w, h, d, PlaceType::NULLTYPE, para.parallel, StartPoint::NULLTYPE, para.rotationStep);
        para_cInt.tracer = para.tracer;

        Clipper3r::Paths ItemsPaths;
        for (int i = 0; i < models.size(); i++)
        {
            std::vector<trimesh::vec3> model = models[i];
            ItemsPaths.push_back(ItemPathDataTrans(model, true));
        }
        Clipper3r::Path transData_cInt = ItemPathDataTrans(transData, false);
        Clipper3r::Paths newItemPaths;
        for (int i = 0; i < NewItems.size(); i++)
        {
            std::vector<trimesh::vec3> NewModel = NewItems[i];
            newItemPaths.push_back(ItemPathDataTrans(NewModel, true));
        }
        std::vector<TransMatrix> NewItemTransDatas;
        nest2d_base(ItemsPaths, transData_cInt, newItemPaths, para_cInt, NewItemTransDatas);

        for (int i = 0; i < NewItems.size(); i++)
        {
            trimesh::vec3 tmp;
            tmp.x = NewItemTransDatas[i].x / NEST_FACTOR;
            tmp.y = NewItemTransDatas[i].y / NEST_FACTOR;
            tmp.z = NewItemTransDatas[i].rotation;
            func(i, tmp);
        }
    }

    void layout_all_nest(const std::vector < std::vector<trimesh::vec3>>& models, std::vector<int> modelIndices,
        NestParaFloat para, std::function<void(int, trimesh::vec3)> modelPositionUpdateFunc)
    {
        trimesh::box3 basebox = para.workspaceBox;
        Clipper3r::Paths allItem;
        allItem.reserve(models.size());
        for (std::vector<trimesh::vec3> m : models)
        {
            Clipper3r::Path oItem;
            int m_size = m.size();
            oItem.resize(m_size);
            for (int i = 0; i < m_size; i++)
            {
                oItem.at(i).X = (Clipper3r::cInt)(m.at(i).x * NEST_FACTOR);
                oItem.at(i).Y = (Clipper3r::cInt)(m.at(i).y * NEST_FACTOR);
            }
            allItem.push_back(oItem);
        }

        Clipper3r::cInt _imageW = (basebox.max.x - basebox.min.x) * NEST_FACTOR;
        Clipper3r::cInt _imageH = (basebox.max.y - basebox.min.y) * NEST_FACTOR;
        Clipper3r::cInt _dist = para.modelsDist * NEST_FACTOR;
        std::vector<TransMatrix> transData;
        NestParaCInt para_cInt = NestParaCInt(_imageW, _imageH, _dist, para.packType, para.parallel, StartPoint::NULLTYPE, para.rotationStep);
        para_cInt.tracer = para.tracer;

        if (para.packType != PlaceType::CONCAVE)
            nest2d_base(allItem, para_cInt, transData);
        else
        {
            transData.resize(allItem.size());
#if 0
            pairPackPro(allItem, transData, para_cInt, allItem.size());

#else
            auto rotateDirUpDown = [&transData](Clipper3r::Paths input, int itemIdx)
            {
                Clipper3r::Path output = input[itemIdx];
                auto vSize2 = [](const Clipper3r::IntPoint& p0)
                {
                    return p0.X * p0.X + p0.Y * p0.Y;
                };
                Clipper3r::Path convex = libnest2d::shapelike::convexHull(polygonLib::PolygonPro::polygonSimplyfy(output, 100));
                int maxLineIdx = -1;
                Clipper3r::cInt maxLen = 0;
                for (int i = 1; i < convex.size(); i++)
                {
                    if (vSize2(convex[i] - convex[i - 1]) > maxLen * maxLen)
                    {
                        maxLineIdx = i;
                        maxLen = sqrt(vSize2(convex[i] - convex[i - 1]));
                    }
                }
                float angle = -atan2f(convex[maxLineIdx].Y - convex[maxLineIdx - 1].Y, convex[maxLineIdx].X - convex[maxLineIdx - 1].X);
                double c = cos(angle);
                double s = sin(angle);

                if (RotateByVector(convex[maxLineIdx], Clipper3r::IntPoint(), c, s).Y > RotateByVector(convex[(maxLineIdx + convex.size() / 2) % convex.size()], Clipper3r::IntPoint(), c, s).Y)
                {
                    angle += angle > M_PIf ? -M_PIf : M_PIf;
                    c = cos(angle);
                    s = sin(angle);
                }
                if (angle != 0)
                {
                    for (Clipper3r::IntPoint& pt : output)
                    {
                        pt = RotateByVector(pt, Clipper3r::IntPoint(), c, s);
                    }
                    transData[itemIdx] = TransMatrix(0, 0, angle * 180 / M_PIf);
                }

                return output;
            };

            int pair_num = allItem.size() / 2;
            std::vector < std::vector<TransMatrix>> transData_pair(pair_num);
            Clipper3r::Paths pair_convex(pair_num);
#if defined(_OPENMP)
#pragma omp parallel for
#endif
            for (int i = 0; i < pair_num; i++)
            {
                Clipper3r::Paths allItem_pair(2);
                allItem_pair[0] = rotateDirUpDown(allItem, 2 * i);
                allItem_pair[1] = rotateDirUpDown(allItem, 2 * i + 1);
                nest2d_base(allItem_pair, para_cInt, transData_pair[i]);
                pair_convex[i] = getPairConcave(allItem_pair, transData_pair[i]);
                pair_convex[i] = RotToMinRect(pair_convex[i], transData_pair[i]);
            }

            para_cInt.sp = StartPoint::BOTTOM_LEFT;
            std::vector<TransMatrix> transData_blk;
            nest2d_base(pair_convex, para_cInt, transData_blk);

            std::vector<int> noPackedPathsId;
            Clipper3r::Paths non_packedPaths;
            std::vector<std::vector<int>> itemId;
            for (int i = 0; i < pair_num; i++)
            {
                if (transData_blk[i].x > 0 && transData_blk[i].x < _imageW && transData_blk[i].y > 0 && transData_blk[i].y < _imageH)
                {
                    non_packedPaths.push_back(pair_convex[i]);
                    std::vector<int> PairItemId;
                    PairItemId.push_back(2 * i);
                    PairItemId.push_back(2 * i + 1);
                    itemId.push_back(PairItemId);
                }
                else
                {
                    noPackedPathsId.push_back(2 * i);
                    noPackedPathsId.push_back(2 * i + 1);
                }
            }
            for (int i = 0; i < noPackedPathsId.size(); i++)
            {
                int idx = noPackedPathsId[i];
                non_packedPaths.push_back(allItem[idx]);
                std::vector<int> singleItemId;
                singleItemId.push_back(idx);
                itemId.push_back(singleItemId);
            }
            if (allItem.size() % 2)
            {
                non_packedPaths.push_back(allItem.back());
                std::vector<int> singleItemId;
                singleItemId.push_back(allItem.size() - 1);
                itemId.push_back(singleItemId);
            }

            if (noPackedPathsId.empty() && allItem.size() % 2 == 0)
            {
                for (int i = 0; i < transData_blk.size(); i++)
                {
                    transData_pair[i][0].merge(transData_blk[i]);
                    transData[2 * i].merge(transData_pair[i][0]);
                    transData_pair[i][1].merge(transData_blk[i]);
                    transData[2 * i + 1].merge(transData_pair[i][1]);
                }
            }
            else
            {
                std::vector<TransMatrix> transData_blk_dst;
                nest2d_base(non_packedPaths, para_cInt, transData_blk_dst);
                for (int i = 0; i < transData_blk_dst.size(); i++)
                {
                    std::vector<int> curItemId = itemId[i];
                    if (curItemId.size() > 1)
                    {
                        int pairIdx = curItemId[0] / 2;
                        transData_pair[pairIdx][0].merge(transData_blk_dst[i]);
                        transData[curItemId[0]].merge(transData_pair[pairIdx][0]);
                        transData_pair[pairIdx][1].merge(transData_blk_dst[i]);
                        transData[curItemId[1]].merge(transData_pair[pairIdx][1]);
                    }
                    else
                    {
                        transData[curItemId[0]].merge(transData_blk_dst[i]);
                    }
                }
            }
#endif
            int minX = INT_MAX, minY = INT_MAX, maxX = 0, maxY = 0;
            for (int i = 0; i < allItem.size(); i++)
            {
                if (transData[i].x > 0 && transData[i].x < _imageW && transData[i].y > 0 && transData[i].y < _imageH)
                {
                    double r = transData[i].rotation * M_PIf / 180;
                    double c = cos(r);
                    double s = sin(r);
                    Clipper3r::IntPoint offset = Clipper3r::IntPoint(transData[i].x, transData[i].y);
                    for (Clipper3r::IntPoint itemPt : allItem[i])
                    {
                        itemPt = RotateByVector(itemPt, offset, c, s);
                        if (itemPt.X < minX) minX = itemPt.X;
                        if (itemPt.Y < minY) minY = itemPt.Y;
                        if (itemPt.X > maxX) maxX = itemPt.X;
                        if (itemPt.Y > maxY) maxY = itemPt.Y;
                    }
                }
            }
            Clipper3r::IntPoint offset2mid = Clipper3r::IntPoint((_imageW - (minX + maxX)) / 2 + 0.5, (_imageH - (minY + maxY)) / 2 + 0.5);
            for (TransMatrix& mat : transData)
            {
                if (mat.x > 0 && mat.x < _imageW && mat.y > 0 && mat.y < _imageH)
                {
                    mat.merge(TransMatrix(offset2mid.X, offset2mid.Y, 0));
                }
            }
        }

        /////settle models that can be settled inside
        trimesh::vec3 total_offset;
        for (size_t i = 0; i < modelIndices.size(); i++)
        {
            trimesh::vec3 newBoxCenter;
            newBoxCenter.x = transData.at(i).x / NEST_FACTOR;
            newBoxCenter.y = transData.at(i).y / NEST_FACTOR;
            newBoxCenter.z = transData.at(i).rotation;
            int modelIndexI = modelIndices[i];
            modelPositionUpdateFunc(modelIndexI, newBoxCenter);
        }
    }

    bool layout_new_item(const std::vector < std::vector<trimesh::vec3>>& models, const std::vector<trimesh::vec3>& transData,
        const std::vector<trimesh::vec3>& NewItem, NestParaFloat para, std::function<void(trimesh::vec3)> func)
    {
        trimesh::box3 basebox = para.workspaceBox;
        Clipper3r::cInt w = (basebox.max.x - basebox.min.x) * NEST_FACTOR;
        Clipper3r::cInt h = (basebox.max.y - basebox.min.y) * NEST_FACTOR;
        Clipper3r::cInt d = para.modelsDist * NEST_FACTOR;
        NestParaCInt para_cInt = NestParaCInt(w, h, d, PlaceType::NULLTYPE, para.parallel, StartPoint::NULLTYPE, para.rotationStep);
        para_cInt.tracer = para.tracer;

        Clipper3r::Paths ItemsPaths;
        for (int i = 0; i < models.size(); i++)
        {
            std::vector<trimesh::vec3> model = models[i];
            ItemsPaths.push_back(ItemPathDataTrans(model, true));
        }
        Clipper3r::Path transData_cInt = ItemPathDataTrans(transData, false);
        Clipper3r::Path newItemPath = ItemPathDataTrans(NewItem, true);
        TransMatrix NewItemTransData;
        bool can_pack = nest2d_base(ItemsPaths, transData_cInt, newItemPath, para_cInt, NewItemTransData);

        trimesh::vec3 tmp;
        tmp.x = NewItemTransData.x / NEST_FACTOR;
        tmp.y = NewItemTransData.y / NEST_FACTOR;
        tmp.z = NewItemTransData.rotation;
        func(tmp);
        return can_pack;
    }


    void InitCfg2(libnest2d::NestConfig<libnest2d::NfpPlacer, libnest2d::FirstFitSelection>& cfg, NestParaToCInt para)
    {
        if (para.packType != PlaceType::CONCAVE && para.packType != PlaceType::CONCAVE_ALL) {
            double angle_per_step = para.rotationAngle * Pi / 180.0;
            int step = std::floor(2.0 * Pi / angle_per_step);
            cfg.placer_config.rotations.clear();
            for (int i = 0; i < step; i++)
                cfg.placer_config.rotations.push_back(libnest2d::Radians((double)i * angle_per_step));//多边形可用旋转角
        }
        cfg.placer_config.parallel = para.parallel;  //启用单线程

        switch (para.sp) {
        case StartPoint::CENTER:
        {
            cfg.placer_config.starting_point = libnest2d::NfpPlacer::Config::Alignment::CENTER;
            cfg.placer_config.alignment = libnest2d::NfpPlacer::Config::Alignment::CENTER;
        }break;
        case StartPoint::TOP_LEFT:
        {
            cfg.placer_config.starting_point = libnest2d::NfpPlacer::Config::Alignment::TOP_LEFT;
            cfg.placer_config.alignment = libnest2d::NfpPlacer::Config::Alignment::DONT_ALIGN;
        }break;
        case StartPoint::BOTTOM_LEFT:
        {
            cfg.placer_config.starting_point = libnest2d::NfpPlacer::Config::Alignment::BOTTOM_LEFT;
            cfg.placer_config.alignment = libnest2d::NfpPlacer::Config::Alignment::DONT_ALIGN;
        }break;
        case StartPoint::TOP_RIGHT:
        {
            cfg.placer_config.starting_point = libnest2d::NfpPlacer::Config::Alignment::TOP_RIGHT;
            cfg.placer_config.alignment = libnest2d::NfpPlacer::Config::Alignment::DONT_ALIGN;
        }break;
        case StartPoint::BOTTOM_RIGHT:
        {
            cfg.placer_config.starting_point = libnest2d::NfpPlacer::Config::Alignment::BOTTOM_RIGHT;
            cfg.placer_config.alignment = libnest2d::NfpPlacer::Config::Alignment::DONT_ALIGN;
        }break;
        case StartPoint::NULLTYPE:
            switch (para.packType) {
            case PlaceType::CENTER_TO_SIDE:
            case PlaceType::ALIGNMENT:
            case PlaceType::ONELINE:
            case PlaceType::CONCAVE:
            {
                cfg.placer_config.starting_point = libnest2d::NfpPlacer::Config::Alignment::CENTER;
                cfg.placer_config.alignment = libnest2d::NfpPlacer::Config::Alignment::CENTER;
            }break;
            case PlaceType::TOP_TO_BOTTOM:
            {
                cfg.placer_config.starting_point = libnest2d::NfpPlacer::Config::Alignment::TOP_LEFT;
                cfg.placer_config.alignment = libnest2d::NfpPlacer::Config::Alignment::DONT_ALIGN;
            }break;
            case PlaceType::BOTTOM_TO_TOP:
            {
                cfg.placer_config.starting_point = libnest2d::NfpPlacer::Config::Alignment::BOTTOM_LEFT;
                cfg.placer_config.alignment = libnest2d::NfpPlacer::Config::Alignment::DONT_ALIGN;
            }break;
            case PlaceType::LEFT_TO_RIGHT:
            {
                cfg.placer_config.starting_point = libnest2d::NfpPlacer::Config::Alignment::TOP_LEFT;
                cfg.placer_config.alignment = libnest2d::NfpPlacer::Config::Alignment::DONT_ALIGN;
            }break;
            case PlaceType::RIGHT_TO_LEFT:
            {
                cfg.placer_config.starting_point = libnest2d::NfpPlacer::Config::Alignment::TOP_RIGHT;
                cfg.placer_config.alignment = libnest2d::NfpPlacer::Config::Alignment::DONT_ALIGN;
            }break;
            }
            break;
        }

        //cfg.placer_config.accuracy; //优化率 
        //cfg.placer_config.explore_holes;  //孔内是否放置图元,目前源码中还未实现
        //cfg.placer_config.before_packing; //摆放下一个Item之前，先对前面已经摆放好的Item进行调整的函数，若为空，则Item摆好位置后将不再变动。传入函数接口
        //cfg.placer_config.object_function;  //添加优化方向，向函数输出值最小化优化，以此改变排放方式，传入函数接口

        //cfg.selector_config;

        if (para.packType != PlaceType::NULLTYPE) {
            cfg.placer_config.object_function = [para](const libnest2d::Item&)  //优化方向
            {
                switch (para.packType) {
                case PlaceType::CENTER_TO_SIDE: return 3; break;
                case PlaceType::ALIGNMENT: return 4; break;
                case PlaceType::ONELINE: return 5; break;
                case PlaceType::CONCAVE: return 6; break;
                case PlaceType::TOP_TO_BOTTOM: return 7; break;
                case PlaceType::BOTTOM_TO_TOP: return 8; break;
                case PlaceType::LEFT_TO_RIGHT: return 9; break;
                case PlaceType::RIGHT_TO_LEFT: return 10; break;
                case PlaceType::CONCAVE_ALL: return 11; break;
                }
                return 3;
            };
        }
    }

    bool nest2d_base2(const Clipper3r::Paths& ItemsPaths, NestParaToCInt para, std::vector<TransMatrix>& transData)
    {
        PlaceType type = para.packType;
        size_t size = ItemsPaths.size();
        transData.resize(size);

        libnest2d::NestControl ctl;
        libnest2d::NestConfig<libnest2d::NfpPlacer, libnest2d::FirstFitSelection> cfg;
        InitCfg2(cfg, para);

        auto convert = [&ItemsPaths, type](Clipper3r::Path & oItem, int index) {
            Clipper3r::Path lines = ItemsPaths.at(index);
            Clipper3r::Clipper a;
            a.AddPath(lines, Clipper3r::ptSubject, true);
            Clipper3r::IntRect r = a.GetBounds();
            oItem.resize(4);
            oItem[0] = Clipper3r::IntPoint(r.left, r.bottom);
            oItem[1] = Clipper3r::IntPoint(r.right, r.bottom);
            oItem[2] = Clipper3r::IntPoint(r.right, r.top);
            oItem[3] = Clipper3r::IntPoint(r.left, r.top);
        };

        std::vector<libnest2d::Item> input;
        for (int i = 0; i < size; i++) {
            Clipper3r::Path ItemPath;
            if (type == PlaceType::ALIGNMENT) convert(ItemPath, i);
            else {
                ItemPath = ItemsPaths[i];
            }

            if (Clipper3r::Orientation(ItemPath)) {
                Clipper3r::ReversePath(ItemPath);//正轮廓倒序
            }
            if (type != PlaceType::CONCAVE && type != PlaceType::CONCAVE_ALL) ItemPath = libnest2d::shapelike::convexHull(ItemPath);
            libnest2d::Item item = libnest2d::Item(ItemPath);
            if (type == PlaceType::CONCAVE || type == PlaceType::CONCAVE_ALL) item.convexCal(false);
            input.push_back(item);
        }

        Clipper3r::cInt imgW_dst = para.workspaceW, imgH_dst = para.workspaceH;
        double offsetX = 0., offsetY = 0.;
        int egde_dist = para.edgeDist;//排样到边缘最近距离为5单位
        if (input.size() == 1 && (input.back().boundingBox().height() >= imgH_dst || input.back().boundingBox().width() >= imgW_dst)) egde_dist = 0;
        if (egde_dist > para.modelsDist) egde_dist = para.modelsDist;
        imgW_dst += para.modelsDist - egde_dist;
        imgH_dst += para.modelsDist - egde_dist;
        offsetX += (para.modelsDist - egde_dist) / 2;
        offsetY += (para.modelsDist - egde_dist) / 2;
        if (para.modelsDist == 0) para.modelsDist = 1;
        libnest2d::Box maxBox = libnest2d::Box(imgW_dst, imgH_dst, { imgW_dst / 2, imgH_dst / 2 });


        std::size_t result = libnest2d::nest(input, maxBox, para.modelsDist, cfg, ctl);


        Clipper3r::Clipper a;
        for (int i = 0; i < size; i++) {
            double angle = input.at(i).rotation().toDegrees();
            if (angle != -90.)//区域内模型
            {
                auto trans_item = input.at(i).transformedShape_s();
                a.AddPath(trans_item.Contour, Clipper3r::ptSubject, true);
            }
        }
        Clipper3r::IntRect ibb_dst = a.GetBounds();
        int center_offset_x = 0.5 * para.workspaceW - (0.5 * (ibb_dst.right + ibb_dst.left) + offsetX);
        int center_offset_y = 0.5 * para.workspaceH - (0.5 * (ibb_dst.bottom + ibb_dst.top) + offsetY);
        if (type == PlaceType::ONELINE) center_offset_y = -ibb_dst.top - offsetY;
        if (type == PlaceType::TOP_TO_BOTTOM || type == PlaceType::BOTTOM_TO_TOP || type == PlaceType::LEFT_TO_RIGHT || type == PlaceType::RIGHT_TO_LEFT) {
            center_offset_x = -2 * offsetX;
            center_offset_y = -2 * offsetY;
        }

        std::vector<libnest2d::Item> input_outer_items(1,
            {
                {0, para.workspaceH},
                {para.workspaceW, para.workspaceH},
                {para.workspaceW, 0},
                {0, 0},
                {0, para.workspaceH}
            });//定义第一个轮廓遮挡中心排样区域

        input_outer_items[0].translation({ 1, 0 }); //packed mark
        if (type == PlaceType::CONCAVE || type == PlaceType::CONCAVE_ALL) input_outer_items[0].convexCal(false);
        for (int i = 0; i < size; i++) {
            libnest2d::Item& iitem = input.at(i);

            int model_offset_x = iitem.translation().X;
            int model_offset_y = iitem.translation().Y;
            double angle = iitem.rotation().toDegrees();

            if (angle == -90.) {
                if (type == PlaceType::CONCAVE || type == PlaceType::CONCAVE_ALL) {
                    auto convex_path = libnest2d::shapelike::convexHull(iitem.rawShape().Contour);
                    input_outer_items.push_back(libnest2d::Item(convex_path));
                } else {
                    input_outer_items.push_back(libnest2d::Item(iitem.rawShape()));//收集排样区域外模型
                }
            } else {
                model_offset_x += center_offset_x;
                model_offset_y += center_offset_y;
                TransMatrix itemTrans;
                itemTrans.rotation = input[i].rotation().toDegrees();
                itemTrans.x = model_offset_x + offsetX;
                itemTrans.y = model_offset_y + offsetY;
                transData[i] = itemTrans;
            }
        }

        //////区域外模型排样
        if (input_outer_items.size() == 1) return true;
        cfg.placer_config.object_function = NULL;
        cfg.placer_config.starting_point = libnest2d::NfpPlacer::Config::Alignment::CENTER;
        cfg.placer_config.alignment = libnest2d::NfpPlacer::Config::Alignment::DONT_ALIGN;
        imgW_dst = para.workspaceW * 1001; imgH_dst = para.workspaceH * 1001;
        maxBox = libnest2d::Box(imgW_dst, imgH_dst, { para.workspaceW / 2, para.workspaceH / 2 });
        std::size_t result_dst = libnest2d::nest(input_outer_items, maxBox, para.modelsDist, cfg, ctl);
        int idx = 1;
        for (int i = 0; i < size; i++) {
            libnest2d::Item& iitem = input.at(i);
            double angle = iitem.rotation().toDegrees();
            if (angle == -90.) {
                libnest2d::Item& iitem_dst = input_outer_items.at(idx);
                TransMatrix itemTrans;
                itemTrans.rotation = iitem_dst.rotation().toDegrees();
                itemTrans.x = iitem_dst.translation().X;
                itemTrans.y = iitem_dst.translation().Y;
                transData[i] = itemTrans;
                idx++;
            }
        }

        return result;
    }

    void layout_all_nest2(const std::vector < std::vector<trimesh::vec3>>& models, std::vector<int> modelIndices,
        NestPara para, std::function<void(int, trimesh::vec3)> modelPositionUpdateFunc, NestPlacerDebugger* debugger)
    {
        trimesh::box3 basebox = para.workspaceBox;
        Clipper3r::Paths allItem;
        allItem.reserve(models.size());
        for (const std::vector<trimesh::vec3>& m : models) {
            Clipper3r::Path oItem;
            int m_size = m.size();
            oItem.resize(m_size);
            for (int i = 0; i < m_size; i++) {
                oItem.at(i).X = (Clipper3r::cInt)(m.at(i).x * NEST_FACTOR);
                oItem.at(i).Y = (Clipper3r::cInt)(m.at(i).y * NEST_FACTOR);
            }
            allItem.push_back(oItem);
        }

        Clipper3r::cInt _imageW = (basebox.max.x - basebox.min.x) * NEST_FACTOR;
        Clipper3r::cInt _imageH = (basebox.max.y - basebox.min.y) * NEST_FACTOR;
        Clipper3r::cInt _dist = para.modelsDist * NEST_FACTOR;
        Clipper3r::cInt _offset = para.modelsDist * NEST_FACTOR;
        std::vector<TransMatrix> transData;
        NestParaToCInt para_cInt = NestParaToCInt(_imageW, _imageH, _dist, _offset, para.packType, para.parallel, StartPoint::NULLTYPE, para.rotationAngle);

        if (para.packType != PlaceType::CONCAVE)
        {
#if _DEBUG
            if (debugger)
            {
                int count = (int)models.size();
                for (int i = 0; i < 100; ++i)
                {
                    for (int j = 0; j < count; ++j)
                    {
                        trimesh::vec3 rt((float)i * (float)j, (float)i * (float)j, (float)i * 5.0f);
                        debugger->onUpdate(j, rt);
                    }
                }
            }
#endif
            nest2d_base2(allItem, para_cInt, transData);
        }
        else {
            transData.resize(allItem.size());
#if 0
            pairPackPro(allItem, transData, para_cInt, allItem.size());

#else
            auto rotateDirUpDown = [&transData](Clipper3r::Paths input, int itemIdx) {
                Clipper3r::Path output = input[itemIdx];
                auto vSize2 = [](const Clipper3r::IntPoint & p0) {
                    return p0.X* p0.X + p0.Y * p0.Y;
                };
                Clipper3r::Path convex = libnest2d::shapelike::convexHull(polygonLib::PolygonPro::polygonSimplyfy(output, 100));
                int maxLineIdx = -1;
                Clipper3r::cInt maxLen = 0;
                for (int i = 1; i < convex.size(); i++) {
                    if (vSize2(convex[i] - convex[i - 1]) > maxLen * maxLen) {
                        maxLineIdx = i;
                        maxLen = sqrt(vSize2(convex[i] - convex[i - 1]));
                    }
                }
                float angle = -atan2f(convex[maxLineIdx].Y - convex[maxLineIdx - 1].Y, convex[maxLineIdx].X - convex[maxLineIdx - 1].X);
                double c = cos(angle);
                double s = sin(angle);

                if (RotateByVector(convex[maxLineIdx], Clipper3r::IntPoint(), c, s).Y > RotateByVector(convex[(maxLineIdx + convex.size() / 2) % convex.size()], Clipper3r::IntPoint(), c, s).Y) {
                    angle += angle > M_PIf ? -M_PIf : M_PIf;
                    c = cos(angle);
                    s = sin(angle);
                }
                if (angle != 0) {
                    for (Clipper3r::IntPoint& pt : output) {
                        pt = RotateByVector(pt, Clipper3r::IntPoint(), c, s);
                    }
                    transData[itemIdx] = TransMatrix(0, 0, angle * 180 / M_PIf);
                }

                return output;
            };

            int pair_num = allItem.size() / 2;
            std::vector < std::vector<TransMatrix>> transData_pair(pair_num);
            Clipper3r::Paths pair_convex(pair_num);
#if defined(_OPENMP)
#pragma omp parallel for
#endif
            for (int i = 0; i < pair_num; i++) {
                Clipper3r::Paths allItem_pair(2);
                allItem_pair[0] = rotateDirUpDown(allItem, 2 * i);
                allItem_pair[1] = rotateDirUpDown(allItem, 2 * i + 1);
                nest2d_base2(allItem_pair, para_cInt, transData_pair[i]);
                pair_convex[i] = getPairConcave(allItem_pair, transData_pair[i]);
                pair_convex[i] = RotToMinRect(pair_convex[i], transData_pair[i]);
            }

            para_cInt.sp = StartPoint::BOTTOM_LEFT;
            std::vector<TransMatrix> transData_blk;

            nest2d_base2(pair_convex, para_cInt, transData_blk);

            std::vector<int> noPackedPathsId;
            Clipper3r::Paths non_packedPaths;
            std::vector<std::vector<int>> itemId;
            for (int i = 0; i < pair_num; i++) {
                if (transData_blk[i].x > 0 && transData_blk[i].x < _imageW && transData_blk[i].y > 0 && transData_blk[i].y < _imageH) {
                    non_packedPaths.push_back(pair_convex[i]);
                    std::vector<int> PairItemId;
                    PairItemId.push_back(2 * i);
                    PairItemId.push_back(2 * i + 1);
                    itemId.push_back(PairItemId);
                } else {
                    noPackedPathsId.push_back(2 * i);
                    noPackedPathsId.push_back(2 * i + 1);
                }
            }
            for (int i = 0; i < noPackedPathsId.size(); i++) {
                int idx = noPackedPathsId[i];
                non_packedPaths.push_back(allItem[idx]);
                std::vector<int> singleItemId;
                singleItemId.push_back(idx);
                itemId.push_back(singleItemId);
            }
            if (allItem.size() % 2) {
                non_packedPaths.push_back(allItem.back());
                std::vector<int> singleItemId;
                singleItemId.push_back(allItem.size() - 1);
                itemId.push_back(singleItemId);
            }

            if (noPackedPathsId.empty() && allItem.size() % 2 == 0) {
                for (int i = 0; i < transData_blk.size(); i++) {
                    transData_pair[i][0].merge(transData_blk[i]);
                    transData[2 * i].merge(transData_pair[i][0]);
                    transData_pair[i][1].merge(transData_blk[i]);
                    transData[2 * i + 1].merge(transData_pair[i][1]);
                }
            } else {
                std::vector<TransMatrix> transData_blk_dst;
                nest2d_base2(non_packedPaths, para_cInt, transData_blk_dst);
                for (int i = 0; i < transData_blk_dst.size(); i++) {
                    std::vector<int> curItemId = itemId[i];
                    if (curItemId.size() > 1) {
                        int pairIdx = curItemId[0] / 2;
                        transData_pair[pairIdx][0].merge(transData_blk_dst[i]);
                        transData[curItemId[0]].merge(transData_pair[pairIdx][0]);
                        transData_pair[pairIdx][1].merge(transData_blk_dst[i]);
                        transData[curItemId[1]].merge(transData_pair[pairIdx][1]);
                    } else {
                        transData[curItemId[0]].merge(transData_blk_dst[i]);
                    }
                }
            }
#endif
            int minX = INT_MAX, minY = INT_MAX, maxX = 0, maxY = 0;
            for (int i = 0; i < allItem.size(); i++) {
                if (transData[i].x > 0 && transData[i].x < _imageW && transData[i].y > 0 && transData[i].y < _imageH) {
                    double r = transData[i].rotation * M_PIf / 180;
                    double c = cos(r);
                    double s = sin(r);
                    Clipper3r::IntPoint offset = Clipper3r::IntPoint(transData[i].x, transData[i].y);
                    for (Clipper3r::IntPoint itemPt : allItem[i]) {
                        itemPt = RotateByVector(itemPt, offset, c, s);
                        if (itemPt.X < minX) minX = itemPt.X;
                        if (itemPt.Y < minY) minY = itemPt.Y;
                        if (itemPt.X > maxX) maxX = itemPt.X;
                        if (itemPt.Y > maxY) maxY = itemPt.Y;
                    }
                }
            }
            Clipper3r::IntPoint offset2mid = Clipper3r::IntPoint((_imageW - (minX + maxX)) / 2 + 0.5, (_imageH - (minY + maxY)) / 2 + 0.5);
            for (TransMatrix& mat : transData) {
                if (mat.x > 0 && mat.x < _imageW && mat.y > 0 && mat.y < _imageH) {
                    mat.merge(TransMatrix(offset2mid.X, offset2mid.Y, 0));
                }
            }
        }

        /////settle models that can be settled inside
        trimesh::vec3 total_offset;
        for (size_t i = 0; i < modelIndices.size(); i++) {
            trimesh::vec3 newBoxCenter;
            newBoxCenter.x = transData.at(i).x / NEST_FACTOR;
            newBoxCenter.y = transData.at(i).y / NEST_FACTOR;
            newBoxCenter.z = transData.at(i).rotation;
            int modelIndexI = modelIndices[i];
            modelPositionUpdateFunc(modelIndexI, newBoxCenter);
        }
    }
}
