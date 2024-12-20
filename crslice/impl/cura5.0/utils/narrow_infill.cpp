#include "Slice3rBase/ClipperUtils.hpp"
#include "Slice3rBase/overhangquality/overhanghead.hpp"
#include "Slice3rBase/overhangquality/extrusionerocessor.hpp"

#include "narrow_infill.h"

#define CLIPPER_OFFSET_SHORTEST_EDGE_FACTOR (0.005f)

namespace cura52
{

    Slic3r::ExPolygon convert(const cura52::Polygons& polygons)
    {
        Slic3r::Polygons Sli3crpolygons;
        Slic3r::Points points_in;
        Slic3r::Point point_in;
        for (int i = 0; i < polygons.size(); i++)
        {
            ClipperLib::Path path = *(polygons.begin() + i);
            for (ClipperLib::IntPoint p : path)
            {
                point_in.x() = p.X * 1000;
                point_in.y() = p.Y * 1000;
                points_in.emplace_back(point_in);
            }
        }

        Slic3r::ExPolygon expolygon;
        expolygon.contour.append(points_in);
        return expolygon;
    }

    Slic3r::Clipper3r::Paths  Polygons2clipperpaths(Slic3r::Polygons& out)
    {
        Slic3r::Clipper3r::Path out2path;
        Slic3r::Clipper3r::Paths out2paths;
        for (int i = 0; i < out.size(); i++)
        {
            Slic3r::Polygon path = out.at(i);
            Slic3r::Clipper3r::IntPoint pointcli;

            for (int j = 0; j < path.size(); j++)
            {
                pointcli.x() = path[j].x();
                pointcli.y() = path[j].y();
                out2path.emplace_back(pointcli);
            }
            out2paths.emplace_back(out2path);
            out2path.clear();
        }
        return out2paths;
    }
    bool result_is_top_area(const cura52::Polygons& area, cura52::Polygons& polygons)
    {
        return  is_top_area(area, polygons);
    }
    static bool is_top_area(const cura52::Polygons& area, const cura52::Polygons& polygons)
    {
        Slic3r::ExPolygon expolygon;
        expolygon = convert(polygons);
        Slic3r::ExPolygon areaex;
        areaex = convert(area);
        Slic3r::ExPolygons result = Slic3r::diff_ex(areaex, expolygon, Slic3r::ApplySafetyOffset::Yes);
        if (result.empty())
            return false;

        return true;
    }

    bool result_is_narrow_infill_area(const cura52::Polygons& polygons)
    {
        return  is_narrow_infill_area(polygons);
    }

    static bool is_narrow_infill_area(const cura52::Polygons& polygons)
    {

        Slic3r::ExPolygon expolygon;
        expolygon = convert(polygons);

        const float delta = -3000000.0;
        double miterLimit = 3.000;

        Slic3r::Clipper3r::JoinType joinType = Slic3r::Clipper3r::JoinType::jtMiter;;
        Slic3r::Polygons  out;
        out = Slic3r::offset(expolygon, delta, joinType, miterLimit);

        bool fillType = false;

        Slic3r::Clipper3r::Paths out2paths;
        out2paths = Polygons2clipperpaths(out);

        Slic3r::ExPolygons result;
        result = Slic3r::ClipperPaths_to_Slic3rExPolygons(out2paths, fillType);

        if (result.empty())
            return true;

        return false;

    }

    /*!
    * ??????
    *
    *
    * \param prev_paths [in] ?????
    * \param cur_paths [in] ?????
    * \param layer_width  ??
    * \param extendedPoints ???????????
    */
    bool processEstimatePoints(const Polygons& prev_paths, const Polygons& cur_paths, const coord_t layer_width, std::vector<std::vector<Slic3r::ExtendedPoint>>& extendedPoints)
    {
        Slic3r::Clipper3r::Paths  paths3r;
        for (auto path : prev_paths.paths)
        {
            paths3r.push_back(Slic3r::Clipper3r::Path());
            for (auto p : path)
            {
                paths3r.back().push_back(Slic3r::Clipper3r::IntPoint((int)p.X, (int)p.Y));
            }
        }

        //Slic3r::Clipper3r::Paths  paths3rTest;
        //paths3rTest.push_back(Slic3r::Clipper3r::Path());
        //paths3rTest.back().push_back(Slic3r::Clipper3r::IntPoint(-11370.994, -17539.082));
        //paths3rTest.back().push_back(Slic3r::Clipper3r::IntPoint(-11251.467, -17515.446));
        //paths3rTest.back().push_back(Slic3r::Clipper3r::IntPoint(-17071.256, 11914.638));
        //paths3rTest.back().push_back(Slic3r::Clipper3r::IntPoint(-23867.872, 10570.606));
        //paths3rTest.back().push_back(Slic3r::Clipper3r::IntPoint(-18048.084, -18859.478));

        Slic3r::ExPolygons _prev_layer = Slic3r::ClipperPaths_to_Slic3rExPolygons(paths3r);
        Slic3r::ExtrusionQualityEstimator extrusion_quality_estimator;
        extrusion_quality_estimator.prepare_for_new_layer(0, _prev_layer);

        Slic3r::Points points;
        for (auto path : cur_paths.paths)
        {
            points.clear();
            for (auto p : path)
            {
                points.push_back(Slic3r::Point((int)p.X, (int)p.Y));

                //Slic3r::Points pointsTest;
                //pointsTest.push_back(Slic3r::Point(-17156.329, -17674.819));
                //pointsTest.push_back(Slic3r::Point(-12300.448, -16714.569));
                //pointsTest.push_back(Slic3r::Point(-12780.572, -14286.629));
                //pointsTest.push_back(Slic3r::Point(-17736.457, 10774.780));
                //pointsTest.push_back(Slic3r::Point(-22592.338, 9814.529));
                //pointsTest.push_back(Slic3r::Point(-17167.968, -17615.958));
                //float layer_widthTest = 0.449999392f;
            }
            std::vector<Slic3r::ExtendedPoint> extended_point =
                Slic3r::estimate_points_properties<true, true, true, true>(points, extrusion_quality_estimator.prev_layer_boundaries[0], layer_width / 1000.0, -2.0f);
            for (Slic3r::ExtendedPoint& pt : extended_point)
            {
                pt.position *= 1000;
                pt.distance *= 1000;
            }
            extendedPoints.push_back(extended_point);

        }
        return true;
    }

    void processOverhang(const SliceLayerPart& part, const Polygons outlines_below, const int half_outer_wall_width, 
        std::vector<VariableWidthLines>& new_wall_toolpaths)
    {
        std::vector<std::vector<Slic3r::ExtendedPoint>> extendedPoints;
        Polygons toolpaths;
        for (VariableWidthLines path : part.wall_toolpaths)
        {
            for (ExtrusionLine line : path)
            {
                toolpaths.add(line.toPolygon());
            }
        }
        processEstimatePoints(outlines_below, toolpaths, 2 * half_outer_wall_width, extendedPoints);


        auto bSamePoint = [](Point p0, Point p1)
        {
            return std::abs(p0.X - p1.X) < 2 && std::abs(p0.Y - p1.Y) < 2;
        };
        int extendedPoints_idx = 0;
        for (VariableWidthLines path : part.wall_toolpaths)
        {
            for (ExtrusionLine& line : path)
            {
                std::vector<ExtrusionJunction> new_junctions;
                std::vector<ExtrusionJunction>& junctions = line.junctions;

                if (line.start_idx >= line.junctions.size())
                {
                    line.start_idx = line.junctions.size() - 1;
                }

                //??z ?flag
                std::vector<Point> vctIntercept;
                for (ExtrusionJunction& J:line.junctions)
                {
                    if (J.flag == ExtrusionJunction::INTERCEPT)
                    {
                        vctIntercept.push_back(J.p);
                    }
                }
				//??z ?flag
				std::vector<Point> vctPaint;
				for (ExtrusionJunction& J : line.junctions)
				{
					if (J.flag == ExtrusionJunction::PAINT)
					{
                        vctPaint.push_back(J.p);
					}
				}

                Point pt_z_seam = (line.start_idx > -1 && line.start_idx < junctions.size()) ? junctions[line.start_idx].p : Point();
                int junctions_idx = 0;
                for (int i = 0; i < extendedPoints[extendedPoints_idx].size(); i++)
                {
                    Point pt = Point(extendedPoints[extendedPoints_idx][i].position.x(), extendedPoints[extendedPoints_idx][i].position.y());
                    if (junctions_idx < junctions.size())
                    {
                        ExtrusionJunction new_pt = ExtrusionJunction(pt, junctions[junctions_idx].w, junctions[junctions_idx].perimeter_index, extendedPoints[extendedPoints_idx][i].distance);
                        for (Point& apoint: vctIntercept)
                        {
                            if (bSamePoint(new_pt.p,apoint))
                            {
                                new_pt.flag = ExtrusionJunction::INTERCEPT;
                            }
                        }
						for (Point& apoint : vctPaint)
						{
							if (bSamePoint(new_pt.p, apoint))
							{
								new_pt.flag = ExtrusionJunction::PAINT;
							}
						}
                        new_junctions.push_back(new_pt);
                        if (bSamePoint(pt, pt_z_seam) && line.start_idx > -1)
                            line.start_idx = i;
                        if (bSamePoint(pt, junctions[junctions_idx].p))
                            junctions_idx++;

                    }
                }
                line.junctions.swap(new_junctions);
                extendedPoints_idx++;
            }
            new_wall_toolpaths.push_back(path);
        }
    }
    void excludePoints(Polygons& polyin, Polygons& polyout)
    {
        Slic3r::Points sps;
        for (ClipperLib::Path path : polyin)
        {
            for (int j = 0; j < path.size(); j++)
            {
                ClipperLib::IntPoint p = path.at(j);
                sps.push_back(Slic3r::Point((int64_t)(p.X), (int64_t)(p.Y)));
            }
            // sps.push_back(Slic3r::Point(INT2MM(path.at(0).X), INT2MM(path.at(0).Y)));
        }
        Slic3r::Polygon spg(sps);
        if (spg.size() > 49) //cube data strange  [[-25,-25],[25,-25],[25,-25],[25,25],[25,25],[-25,-25]]  exclude this 
            spg.douglas_peucker(30);
        Polygon p;
        for (int i = 0; i < spg.points.size() ; i++)
        {
            p.emplace_back(spg.points.at(i).x(), spg.points.at(i).y());
        }
        polyout.add(p);
    }
}