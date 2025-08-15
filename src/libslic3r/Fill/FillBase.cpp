#include <stdio.h>
#include <numeric>

#include "../ClipperUtils.hpp"
#include "../EdgeGrid.hpp"
#include "../Geometry.hpp"
#include "../Geometry/Circle.hpp"
#include "../Point.hpp"
#include "../PrintConfig.hpp"
#include "../Surface.hpp"
#include "../VariableWidth.hpp"
#include "../ShortestPath.hpp"

#include "FillBase.hpp"
#include "FillConcentric.hpp"
#include "FillHoneycomb.hpp"
#include "Fill3DHoneycomb.hpp"
#include "FillGyroid.hpp"
#include "FillPlanePath.hpp"
#include "FillLine.hpp"
#include "FillRectilinear.hpp"
#include "FillAdaptive.hpp"
#include "FillLightning.hpp"
#include "FillCross.hpp"
#include "FillQuarter.hpp"
// BBS: new infill pattern header
#include "FillConcentricInternal.hpp"
#include "FillCrossHatch.hpp"
#include "FillTpmsD.hpp"

// #define INFILL_DEBUG_OUTPUT
#define FRIST_LINE_LAYER 85732
#define STANDARD_LAYER 100000
#define LINE_WIDTH 520
#define EPS 1e-2

namespace Slic3r {

//BBS: 0% of sparse_infill_line_width, no anchor at the start of sparse infill
float Fill::infill_anchor = 400;
//BBS: 20mm
float Fill::infill_anchor_max = 20;

Fill* Fill::new_from_type(const InfillPattern type)
{
    switch (type) {
    case ipConcentric:          return new FillConcentric();
    case ipHoneycomb:           return new FillHoneycomb();
    case ip3DHoneycomb:         return new Fill3DHoneycomb();
    case ipGyroid:              return new FillGyroid();
    case ipTpmsD:               return new FillTpmsD();
    case ipRectilinear:         return new FillRectilinear();
    case ipAlignedRectilinear:  return new FillAlignedRectilinear();
    case ipCrossHatch:          return new FillCrossHatch();
    case ipMonotonic:           return new FillMonotonic();
    case ipLine:                return new FillLine();
    case ipGrid:                return new FillGrid();
    case ipTriangles:           return new FillTriangles();
    case ipStars:               return new FillStars();
    case ipCubic:               return new FillCubic();
    case ipArchimedeanChords:   return new FillArchimedeanChords();
    case ipHilbertCurve:        return new FillHilbertCurve();
    case ipOctagramSpiral:      return new FillOctagramSpiral();
    case ipAdaptiveCubic:       return new FillAdaptive::Filler();
    case ipSupportCubic:        return new FillAdaptive::Filler();
    case ipSupportBase:         return new FillSupportBase();
    case ipLightning:           return new FillLightning::Filler();
    case ipCross:          
    case ipCross3d:             return new FillCross();
    // BBS: for internal solid infill only
    case ipConcentricInternal:  return new FillConcentricInternal();
    // BBS: for bottom and top surface only
    // Orca: Replace BBS implementation with Prusa implementation
    case ipMonotonicLine:       return new FillMonotonicLines();
    case ipquarter_cubic:       
    case iptetrahedral:       return new FillQuarter();
    default: throw Slic3r::InvalidArgument("unknown type");
    }
}

Fill* Fill::new_from_type(const std::string &type)
{
    const t_config_enum_values &enum_keys_map = ConfigOptionEnum<InfillPattern>::get_enum_values();
    t_config_enum_values::const_iterator it = enum_keys_map.find(type);
    return (it == enum_keys_map.end()) ? nullptr : new_from_type(InfillPattern(it->second));
}

// Force initialization of the Fill::use_bridge_flow() internal static map in a thread safe fashion even on compilers
// not supporting thread safe non-static data member initializers.
static bool use_bridge_flow_initializer = Fill::use_bridge_flow(ipGrid);

bool Fill::use_bridge_flow(const InfillPattern type)
{
	static std::vector<unsigned char> cached;
	if (cached.empty()) {
		cached.assign(size_t(ipCount), 0);
		for (size_t i = 0; i < cached.size(); ++ i) {
			auto *fill = Fill::new_from_type((InfillPattern)i);
			cached[i] = fill->use_bridge_flow();
			delete fill;
		}
	}
	return cached[type] != 0;
}

Polylines Fill::fill_surface(const Surface *surface, const FillParams &params)
{
    // Perform offset.
    Slic3r::ExPolygons expp = offset_ex(surface->expolygon, float(scale_(this->overlap - 0.5 * this->spacing)));
    // Create the infills for each of the regions.
    Polylines polylines_out;
    for (size_t i = 0; i < expp.size(); ++ i)
        _fill_surface_single(
            params,
            surface->thickness_layers,
            _infill_direction(surface),
            std::move(expp[i]),
            polylines_out);
    return polylines_out;
}

ThickPolylines Fill::fill_surface_arachne(const Surface* surface, const FillParams& params)
{
    // Perform offset.
    Slic3r::ExPolygons expp = offset_ex(surface->expolygon, float(scale_(this->overlap - 0.5 * this->spacing)));
    // Create the infills for each of the regions.
    ThickPolylines thick_polylines_out;
    for (ExPolygon& expoly : expp)
        _fill_surface_single(params, surface->thickness_layers, _infill_direction(surface), std::move(expoly), thick_polylines_out);
    return thick_polylines_out;
}

// BBS: this method is used to fill the ExtrusionEntityCollection. It call fill_surface by default
void Fill::fill_surface_extrusion(const Surface* surface, const FillParams& params, ExtrusionEntitiesPtr& out)
{
    //-----------try  angle---------------------
    // int max_angle = 0;
    // int output_angle = 0;
    // std::pair<int, float> best_angle(0,std::numeric_limits<float>::max());
    // for (int a = 0; a < 4; a++)
    //{
    //    double auoi = M_PI / 4;
    //    Points point_container;
    //    for (int i = 0; i < surface->expolygon.contour.points.size(); i++)
    //    {
    //        Eigen::Vector2d point(surface->expolygon.contour.points[i][0], surface->expolygon.contour.points[i][1]);
    //        double angle = auoi*a;
    //        Eigen::Rotation2D rotation(angle);
    //        Eigen::Vector2d rotated_point = rotation * point;
    //        point_container.push_back(Point(rotated_point[0], rotated_point[1]));
    //    }
    //    if (!surface->expolygon.holes.empty())
    //        point_container.push_back(Point(std::numeric_limits<int>::max(), std::numeric_limits<int>::max()));
    //    for (int i = 0; i < surface->expolygon.holes.size(); i++)
    //    {
    //        for (int j = 0; j < surface->expolygon.holes[i].points.size(); j++)
    //        {
    //            point_container.push_back(Point(surface->expolygon.holes[i].points[j][0], surface->expolygon.holes[i].points[j][1]));
    //        }
    //        if (i != surface->expolygon.holes.size() - 1)
    //            point_container.push_back(Point(std::numeric_limits<int>::max(), std::numeric_limits<int>::max()));
    //    }

    //    std::vector<int> point_y_container;
    //    std::vector<int> point_x_container;
    //    for (int i = 0; i < point_container.size(); i++)
    //    {
    //        point_y_container.push_back(point_container[i][1]);
    //        point_x_container.push_back(point_container[i][0]);
    //    }
    //    /* for (int i = 0; i < surface->expolygon.contour.points.size(); i++)
    //         point_y_container.push_back(surface->expolygon.contour.points[i][1]);
    //     if(!surface->expolygon.holes.empty())
    //         point_y_container.push_back(std::numeric_limits<int>::max());
    //     for (int i = 0; i < surface->expolygon.holes.size(); i++)
    //     {
    //         for (int j = 0; j < surface->expolygon.holes[i].points.size(); j++)
    //         {
    //             point_y_container.push_back(surface->expolygon.holes[i].points[j][1]);
    //         }
    //         if(i!=surface->expolygon.holes.size()-1)
    //             point_y_container.push_back(std::numeric_limits<int>::max());
    //     }*/

    //    std::vector<std::pair<int, int>>  path_loop;
    //    std::vector<std::pair<int, int>> path_loop_x;
    //    int min_z = std::numeric_limits<int>::max();
    //    int max_z = std::numeric_limits<int>::min();
    //    int mark_ptr = 0;
    //    for (int i = 0; i < point_y_container.size(); i++)
    //    {
    //        int index = i + 1;
    //        if (i == point_y_container.size() - 1)
    //            index = 0;

    //        if (point_y_container[index] != std::numeric_limits<int>::max())
    //        {
    //            path_loop.push_back(std::make_pair(point_y_container[i], point_y_container[index]));
    //            path_loop_x.push_back(std::make_pair(point_x_container[i], point_x_container[index]));
    //        }
    //        else
    //        {
    //            index = mark_ptr;
    //            path_loop.push_back(std::make_pair(point_y_container[i], point_y_container[index]));
    //            path_loop_x.push_back(std::make_pair(point_x_container[i], point_x_container[index]));
    //            i++;
    //            mark_ptr = i + 1;
    //        }

    //        if (point_y_container[i] != std::numeric_limits<int>::max())
    //        {
    //            if (min_z > point_y_container[i])
    //            {
    //                min_z = point_y_container[i];
    //            }
    //            if (max_z < point_y_container[i])
    //            {
    //                max_z = point_y_container[i];
    //            }
    //        }
    //    }

    //    std::vector<int> mark_lines;
    //    std::vector<std::vector<int>> mark_indexs;
    //    int begin_ptr = min_z + STANDARD_LAYER;

    //    while (begin_ptr < max_z)
    //    {
    //        bool first_into = false;
    //        for (int i = 0; i < path_loop.size(); i++)
    //        {
    //            if ((begin_ptr > path_loop[i].first && begin_ptr < path_loop[i].second) || (begin_ptr<path_loop[i].first &&
    //            begin_ptr>path_loop[i].second))
    //            {
    //                if (!first_into)
    //                {
    //                    mark_lines.push_back(1);
    //                    mark_indexs.push_back(std::vector<int>(1,i));
    //                    first_into = true;
    //                }
    //                else
    //                {
    //                    mark_lines.back() = (mark_lines.back() + 1);
    //                    mark_indexs.back().push_back(i);
    //                }
    //            }
    //        }

    //        int next_ptr = begin_ptr + STANDARD_LAYER;
    //        if (begin_ptr < 0 && next_ptr>0)
    //        {
    //            begin_ptr += FRIST_LINE_LAYER;
    //        }
    //        else
    //        {
    //            begin_ptr += STANDARD_LAYER;
    //        }
    //    }

    //    for (int i = 0; i < mark_indexs.size(); i++)
    //    {
    //        std::sort(mark_indexs[i].begin(), mark_indexs[i].end(), [&](int a,int b) {
    //            if (path_loop_x[a] < path_loop_x[b])
    //                return a;
    //            else
    //                return b;
    //            });
    //    }

    //    int ares_num = 1;
    //    int ptr = 1;
    //    int last_index = 0;
    //    int distance = 0;
    //    for (int i = 0; i < mark_lines.size(); i++)
    //    {
    //        int ares_n = mark_lines[i] / 2;
    //        if (ptr != ares_n)
    //        {
    //            if (ares_n > ptr)
    //            {
    //                ares_num++;
    //                /*if(i != 0)
    //                    for (int j = 0; j < mark_indexs[i].size() - 1; j += 2)
    //                    {
    //                        distance +=
    //                        std::abs((path_loop[mark_indexs[last_index][j]].first+path_loop[mark_indexs[last_index][j]].second)/2 -
    //                            (path_loop[mark_indexs[last_index][j+1]].first+path_loop[mark_indexs[last_index][j+1]].second)/2);
    //                    }*/
    //            }

    //            last_index = i;
    //            ptr = ares_n;
    //        }

    //        if (i != mark_lines.size() - 1)
    //        {
    //            if (mark_indexs[i].size() != mark_indexs[i + 1].size())
    //            {
    //                if (mark_indexs[i].size() > mark_indexs[i + 1].size())
    //                {
    //                    std::vector<int> distance_line;
    //                    for (int j = 0; j < mark_indexs[i].size()-1; j+=2)
    //                    {
    //                        distance_line.push_back(std::abs((path_loop[mark_indexs[i][j]].first+path_loop[mark_indexs[i][j]].second)/2 -
    //                            (path_loop[mark_indexs[i][j+1]].first+path_loop[mark_indexs[i][j+1]].second)/2));
    //                    }
    //                    std::sort(distance_line.begin(), distance_line.end(), [](int a,int b) {
    //                        if (a > b)
    //                            return a;
    //                        });
    //                    distance_line.erase(distance_line.begin());
    //                    distance+=std::accumulate(distance_line.begin(), distance_line.end(), 0);
    //                }
    //                else if (mark_indexs[i].size() < mark_indexs[i + 1].size())
    //                {
    //                     std::vector<int> distance_line;
    //                    for (int j = 0; j < mark_indexs[i+1].size()-1; j+=2)
    //                    {
    //                          distance_line.push_back(std::abs((path_loop[mark_indexs[i+1][j]].first+path_loop[mark_indexs[i+1][j]].second)/2
    //                          -
    //                            (path_loop[mark_indexs[i+1][j+1]].first+path_loop[mark_indexs[i+1][j+1]].second)/2));
    //                    }
    //                     std::sort(distance_line.begin(), distance_line.end(), [](int a,int b) {
    //                        if (a > b)
    //                            return a;
    //                        });
    //                     distance_line.erase(distance_line.begin());
    //                     distance+=std::accumulate(distance_line.begin(), distance_line.end(), 0);
    //                }
    //            }
    //        }

    //    }
    //    if (best_angle.second > distance)
    //    {
    //        best_angle.first = a;
    //        best_angle.second = distance;
    //    }
    //    if (ares_num > max_angle)
    //    {
    //        max_angle = ares_num;
    //        output_angle = a;
    //    }
    //}

    Polylines      polylines;
    ThickPolylines thick_polylines;
    try {
        if (params.use_arachne)
            thick_polylines = this->fill_surface_arachne(surface, params);
        else
            polylines = this->fill_surface(surface, params);
        // polylines = this->fill_surface_angle(surface,params,best_angle.first*M_PI/4.f);
    } catch (InfillFailedException&) {}

    //----------------test------------------
    /* std::ofstream outfile_input("input.txt");
     for (int i = 0; i < surface->expolygon.contour.points.size(); i++)
     {
         outfile_input << "(" << surface->expolygon.contour.points[i][0]/1000.f << "," << surface->expolygon.contour.points[i][1]/1000.f <<
     ")" << std::endl;
     }
     outfile_input.close();*/

    //-----output------stTop
    long long aaarree = surface->expolygon.contour.area();
    float     sclae_width = params.flow.width() / 0.45f;
    //while(0){
    while (1 /*&&aaarree>2e15*/ && surface->is_top() && !surface->is_internal() && params.config->top_surface_pattern.value!=16) {
        if (polylines.empty())
            break;

        std::vector<std::vector<std::pair<double, double>>> rot_points;
        // std::ofstream outfile("output.txt");

        Eigen::Vector2d   point1(polylines[0].points[0][0], polylines[0].points[0][1]);
        Eigen::Vector2d   point2(polylines[0].points[1][0], polylines[0].points[1][1]);
        double            fir_angle = -(double) (this->angle);
        Eigen::Rotation2D rotation(fir_angle);
        Eigen::Vector2d   rotated_point1 = rotation * point1;
        Eigen::Vector2d   rotated_point2 = rotation * point2;
        double            rot_angle      = std::abs(rotated_point1[1] - rotated_point2[1]) < 10 ? this->angle :
                                           this->angle < M_PI / 2.0                             ? this->angle + M_PI / 2.0 :
                                                                                                  this->angle - M_PI / 2.0;

        Polylines polyline_baseline;
        for (int z = 0; z < polylines.size(); z++) {
            rot_points.push_back(std::vector<std::pair<double, double>>());
            polyline_baseline.push_back(Polyline());
            for (int i = 0; i < polylines[z].points.size(); i++) {
                Eigen::Vector2d   point(polylines[z].points[i][0], polylines[z].points[i][1]);
                double            angle = -(double) rot_angle;
                Eigen::Rotation2D rotation(angle);
                Eigen::Vector2d   rotated_point = rotation * point;
                rot_points.back().push_back(std::make_pair(rotated_point[0] / 1000.f, rotated_point[1] / 1000.f));
                polyline_baseline.back().points.push_back(Point(rotated_point[0], rotated_point[1]));
                // outfile << "(" << std::fixed <<rotated_point[0]/1000.f<< "," << std::fixed <<rotated_point[1]/1000.f<< ")" << std::endl;
            }
            // outfile << " " << std::endl;
        }
        // outfile.close();

        //--------------------------------------------------------------------
        bool plan_a = false;
#if plan_a
        std::vector<std::vector<float>>                   distance_block;
        std::vector<std::vector<std::pair<Vec2d, Vec2d>>> pair_points_block;
        std::vector<std::pair<double, int>>               areas_container;
#else

        //-------------big   CODE--------------------------

        //            std::vector<std::vector<std::pair<Point, Point>>> endpoints_block;
        //            std::vector<std::vector<bool>> endpoints_mark;
        //            Polylines polyline_temp;
        //            Polylines polyline_copy;
        //            //std::vector<float> connect_for_one;
        // #endif
        //            for (int i = 0; i < rot_points.size(); i++)
        //            {
        //                polyline_temp.push_back(Polyline());
        //                std::vector<std::pair<double, double>> new_points;
        //                for (int j = 0; j < rot_points[i].size() - 1; j += 2)
        //                {
        //                    if (std::abs(rot_points[i][j].second - rot_points[i][j + 1].second) < EPS)
        //                    {
        //                        new_points.push_back(rot_points[i][j]);
        //                        new_points.push_back(rot_points[i][j + 1]);
        //                       // outfile << "(" << rot_points[i][j].first << "," << rot_points[i][j].second << ")" << std::endl;
        //                       // outfile << "(" << rot_points[i][j + 1].first << "," << rot_points[i][j + 1].second << ")" << std::endl;
        //                        polyline_temp.back().points.push_back(polylines[i].points[j]);
        //                        polyline_temp.back().points.push_back(polylines[i].points[j+1]);
        //                    }
        //                    else {
        //                         polyline_temp.back().points.push_back(polylines[i].points[j]);
        //                        j--;
        //                    }
        //                }
        //                //outfile << " " << std::endl;
        //
        //                //-----------cal areas---------------
        //               /* double areas = 0.f;
        //                for (int j = 0; j < new_points.size() - 3; j += 2)
        //                {
        //                    double bot_x_distance = std::abs(new_points[j].first - new_points[j + 1].first);
        //                    double top_x_distance = std::abs(new_points[j + 2].first - new_points[j + 3].first);
        //                    double height = std::abs(new_points[j + 2].second - new_points[j].second);
        //                    double area = (bot_x_distance + top_x_distance) * height / 2.0f;
        //                    areas += area;
        //                }
        //                areas_container.push_back(std::make_pair(areas, i));*/
        //
        // #if plan_a
        //                std::vector<float> distance_span;
        //                std::vector<std::pair<Vec2d, Vec2d>> pair_points;
        //                std::vector<int> index_span;
        //                for (int j = 0; j < new_points.size(); j += 2)
        //                {
        //                    if (j == 0 || j == new_points.size() - 2)
        //                    {
        //                        if (std::abs(new_points[j].first - new_points[j + 1].first) > 1800)
        //                        {
        //                            distance_span.push_back(std::abs(new_points[j].first - new_points[j + 1].first));
        //                            pair_points.push_back(std::make_pair(Vec2d(new_points[j].first, new_points[j].second),
        //                            Vec2d(new_points[j + 1].first, new_points[j + 1].second))); index_span.push_back(j);
        //                        }
        //                    }
        //                    else
        //                    {
        //                        // judge orient and distanca
        //                        float d1 = new_points[j].first - new_points[j + 1].first;
        //                        float d2 = new_points[j - 2].first - new_points[j - 1].first;
        //                        float distance1 = std::abs(d1);
        //                        float distance2 = std::abs(d2);
        //                        if (d1 * d2 > 0)
        //                        {
        //                            /* distance_span.push_back(std::abs(new_points[j].first - new_points[j + 1].first));
        //                             pair_points.push_back(std::make_pair(Vec2d(new_points[j].first,new_points[j].second),Vec2d(new_points[j+1].first,new_points[j+1].second)));
        //                             index_span.push_back(j);*/
        //                        }
        //                        else
        //                        {
        //                            if (std::abs(distance1 - distance2) > 1800)
        //                            {
        //                                distance_span.push_back(std::abs(distance1 - distance2));
        //                                pair_points.push_back(std::make_pair(Vec2d(new_points[j].first, new_points[j].second),
        //                                Vec2d(new_points[j + 1].first, new_points[j + 1].second))); index_span.push_back(j);
        //                            }
        //                        }
        //
        //                    }
        //                }
        //                distance_block.push_back(distance_span);
        //                pair_points_block.push_back(pair_points);
        // #else
        //                for (int j = 0; j < new_points.size(); j += 2)
        //                {
        //                    if (new_points[j].first > new_points[j + 1].first)
        //                    {
        //                        std::swap(new_points[j], new_points[j + 1]);
        //                    }
        //                }
        //
        //                //endpoints_block.push_back(std::vector<std::pair<Vec2d, Vec2d>>());
        //                //endpoints_mark.push_back(std::vector<bool>());
        //                /*for (int j = 0; j<new_points.size(); j+=2)
        //                {
        //                    if (j == 0||j == new_points.size()-2)
        //                    {
        //                        if (std::abs(new_points[j].first - new_points[j + 1].first) > 1600)
        //                        {
        //                            endpoints_block.back().push_back(std::make_pair(Vec2d(new_points[j].first,new_points[j].second),
        //                                Vec2d(new_points[j + 1].first,new_points[j + 1].second)));
        //                        }
        //                    }
        //                    else
        //                    {
        //                        Vec2d p1(new_points[j].first,new_points[j].second);
        //                        Vec2d p2(new_points[j+1].first,new_points[j+1].second);
        //                        Vec2d p3(new_points[j+2].first,new_points[j+2].second);
        //                        Vec2d p4(new_points[j+3].first,new_points[j+3].second);
        //
        //                        if (std::abs(p1.x() - p3.x()) > 1600 || std::abs(p2.x() - p4.x()) > 1600)
        //                        {
        //                            endpoints_block.back().push_back(std::make_pair(Vec2d(new_points[j].first,new_points[j].second),
        //                                Vec2d(new_points[j + 1].first,new_points[j + 1].second)));
        //                        }
        //                    }
        //                }*/
        //
        //                //----find potential targets-----method 1 ------
        //                //for (int j = 0; j < new_points.size(); j+=2)
        //                //{
        //                //    float dist_l = std::abs(new_points[j].first - new_points[j + 1].first)/5.0f;
        //                //    int connect_num_bot = 0;
        //                //    int connect_num_up = 0;
        //                //    //----------div 4--------------
        //                //    if (j == 0||j == new_points.size() - 2)
        //                //    {
        //                //        if (dist_l * 5.0f > 1800.f)
        //                //            connect_num_bot++;
        //                //    }
        //                //    else
        //                //    {
        //                //        for (int k = 1; k <= 1; k++)
        //                //        {
        //                //            for (int z = 1; z <= 4; z++)
        //                //            {
        //                //                float c = new_points[j].first + dist_l * z;
        //                //                if (c > new_points[j - 2 * k].first && c < new_points[j - 2 * k + 1].first)
        //                //                {
        //                //                    if (new_points[j - 2 * k].second > new_points[j].second)
        //                //                        connect_num_up++;
        //                //                    if (new_points[j - 2 * k].second < new_points[j].second)
        //                //                        connect_num_bot++;
        //                //
        //                //                }
        //                //                if (c > new_points[j + 2 * k].first && c < new_points[j + 2 * k + 1].first)
        //                //                {
        //                //                    if (new_points[j + 2 * k].second > new_points[j].second)
        //                //                        connect_num_up++;
        //                //                    if (new_points[j + 2 * k].second < new_points[j].second)
        //                //                        connect_num_bot++;
        //                //
        //                //                }
        //                //            }
        //                //            //if (connect_num_bot > 0 && connect_num_up > 0)
        //                //            //{
        //                //            //    /*float dist_2 = std::abs(new_points[j + 2].first - new_points[j + 3].first);
        //                //            //    if (std::abs(dist_l * 5.0f - dist_2) > 2000)
        //                //            //    {
        //                //            //        connect_num_bot = 1;
        //                //            //        connect_num_up = 0;
        //                //            //    }*/
        //                //            //    break;
        //                //            //}
        //                //        }
        //                //
        //                //    }
        //                //    //-------potential targets---
        //                //    //if ((connect_num_bot > 0 && connect_num_up == 0)||(connect_num_bot == 0 && connect_num_up > 0))
        //                //    if(connect_num_bot!=4||connect_num_up!=4)
        //                //    {
        //                //        if(std::abs(new_points[j].first-new_points[j + 1].first)>1600)
        //                //            endpoints_block.back().push_back(std::make_pair(Vec2d(new_points[j].first, new_points[j].second),
        //                //                Vec2d(new_points[j + 1].first, new_points[j + 1].second)));
        //                //    }
        //                //}
        //
        //
        //                //-------------------------------method 2 -----------------------
        //
        //                /*for (int j = 0; j < new_points.size(); j += 2)
        //                {
        //                    if (j == 0 || j == new_points.size() - 2)
        //                    {
        //                        if (std::abs(new_points[j].first - new_points[j + 1].first) > 1800.f)
        //                        {
        //                            endpoints_block.back().push_back(std::make_pair(Vec2d(new_points[j].first, new_points[j].second),
        //                                Vec2d(new_points[j + 1].first, new_points[j + 1].second)));
        //                            endpoints_mark.back().push_back(false);
        //                        }
        //                    }
        //                    else {
        //                        if (std::abs(new_points[j].first - new_points[j - 2].first) > 1800.f)
        //                        {
        //                            if (std::abs(new_points[j].second - new_points[j - 2].second) < 100.f)
        //                            {
        //                                endpoints_block.back().push_back(std::make_pair(Vec2d(new_points[j - 2].first, new_points[j -
        //                                2].second),
        //                                    Vec2d(new_points[j - 1].first, new_points[j - 1].second)));
        //                                endpoints_mark.back().push_back(true);
        //                            }
        //                            else {
        //                                if (new_points[j].first - new_points[j - 2].first > 0.f)
        //                                {
        //                                    endpoints_block.back().push_back(std::make_pair(Vec2d(new_points[j - 2].first, new_points[j -
        //                                    2].second),
        //                                        Vec2d(new_points[j].first, new_points[j].second)));
        //                                    endpoints_mark.back().push_back(true);
        //                                }
        //                                else {
        //                                    endpoints_block.back().push_back(std::make_pair(Vec2d(new_points[j].first,
        //                                    new_points[j].second),
        //                                        Vec2d(new_points[j - 2].first, new_points[j - 2].second)));
        //                                    endpoints_mark.back().push_back(true);
        //                                }
        //                            }
        //                        }
        //                        if (std::abs(new_points[j + 1].first - new_points[j - 1].first) > 1800.f)
        //                        {
        //                            if (std::abs(new_points[j + 1].second - new_points[j - 1].second) < 100.f)
        //                            {
        //                                endpoints_block.back().push_back(std::make_pair(Vec2d(new_points[j - 2].first, new_points[j -
        //                                2].second),
        //                                    Vec2d(new_points[j - 1].first, new_points[j - 1].second)));
        //                                endpoints_mark.back().push_back(true);
        //                            }
        //                            else {
        //                                if (new_points[j + 1].first - new_points[j - 1].first > 0.f)
        //                                {
        //                                    endpoints_block.back().push_back(std::make_pair(Vec2d(new_points[j - 1].first, new_points[j +
        //                                    1].second),
        //                                        Vec2d(new_points[j + 1].first, new_points[j + 1].second)));
        //                                    endpoints_mark.back().push_back(true);
        //                                }
        //                                else {
        //                                    endpoints_block.back().push_back(std::make_pair(Vec2d(new_points[j + 1].first, new_points[j -
        //                                    1].second),
        //                                        Vec2d(new_points[j - 1].first, new_points[j - 1].second)));
        //                                    endpoints_mark.back().push_back(true);
        //                                }
        //                            }
        //                        }
        //                    }
        //                }
        //
        //                for (int j = 1; j < endpoints_block.back().size(); j++)
        //                {
        //                    if (std::abs(endpoints_block.back().at(j).first.x() - endpoints_block.back().at(j - 1).first.x()) < 1e-2 &&
        //                        std::abs(endpoints_block.back().at(j).first.y() - endpoints_block.back().at(j - 1).first.y()) < 1e-2 &&
        //                        std::abs(endpoints_block.back().at(j).second.x() - endpoints_block.back().at(j - 1).second.x()) < 1e-2 &&
        //                        std::abs(endpoints_block.back().at(j).second.y() - endpoints_block.back().at(j - 1).second.y()) < 1e-2)
        //                    {
        //                        endpoints_block.back().erase(endpoints_block.back().begin() + j);
        //                        endpoints_mark.back().erase(endpoints_mark.back().begin() + j);
        //                        j--;
        //                    }
        //                }*/
        //
        //                //---------------------method3-------------------------
        //                std::pair<Point, Point> last_point;
        //                int begin_input = 0; // 1.beign 2.end 3.add now 4.add next 5.stop
        //                for (int j = 0; j < new_points.size(); j += 2)
        //                {
        //                   // outfile << "(" << new_points[j].first<<","<<new_points[j].second<<")"<<std::endl;
        //                    //outfile << "(" << new_points[j+1].first<<","<<new_points[j+1].second<<")"<<std::endl;
        //                    if (begin_input == 3 || begin_input == 4 || begin_input == 5)
        //                    {
        //                        endpoints_block.back().push_back(last_point);
        //                    }
        //                    begin_input = 0;
        //                    if (j == 0)
        //                    {
        //                        begin_input = 1;
        //                    }
        //                    else if (j == new_points.size() - 2)
        //                    {
        //                        begin_input = 2;
        //                    }
        //                    else
        //                    {
        //                        if (std::abs(new_points[j+1].second - new_points[j+3 ].second) < 100.f)
        //                        {
        //                            begin_input = 4;
        //                        }
        //                        else {
        //                            if (std::abs(new_points[j].first - new_points[j + 2].first) > 1800.f)
        //                                begin_input = 5;
        //                            if (std::abs(new_points[j+1].first - new_points[j + 3].first) > 1800.f)
        //                                begin_input = 3;
        //                            if (std::abs(new_points[j + 2].second - new_points[j + 4].second) < 100.f)
        //                                begin_input = 0;
        //                        }
        //                    }
        //
        //                    switch (begin_input)
        //                    {
        //                    case 1:
        //                        polyline_copy.push_back(Polyline());
        //                        polyline_copy.back().points.push_back(polyline_temp[i].points[j]);
        //                        polyline_copy.back().points.push_back(polyline_temp[i].points[j+1]);
        //                        endpoints_block.push_back(std::vector<std::pair<Point, Point>>());
        //                        if (std::abs(new_points[j].first - new_points[j + 1].first) > 1800)
        //                        {
        //                            endpoints_block.back().push_back(std::make_pair(Point(new_points[j].first, new_points[j].second),
        //                            Point(new_points[j + 1].first, new_points[j + 1].second)));
        //                        }
        //                        break;
        //                    case 2:
        //                        polyline_copy.back().points.push_back(polyline_temp[i].points[j]);
        //                        polyline_copy.back().points.push_back(polyline_temp[i].points[j+1]);
        //                        if(std::abs(new_points[j].first - new_points[j + 1].first)>1800)
        //                            endpoints_block.back().push_back(std::make_pair(Point(new_points[j].first,new_points[j].second),Point(new_points[j+1].first,new_points[j+1].second)));
        //                        break;
        //                    case 3:
        //                    case 5:
        //                        polyline_copy.back().points.push_back(polyline_temp[i].points[j]);
        //                        polyline_copy.back().points.push_back(polyline_temp[i].points[j+1]);
        //                        polyline_copy.push_back(Polyline());
        //                        if (begin_input == 5)
        //                        {
        //                            if (new_points[j].first > new_points[j + 2].first)
        //                            {
        //                                 endpoints_block.back().push_back(std::make_pair(Point(new_points[j].first, new_points[j].second),
        //                                 Point(new_points[j + 1].first, new_points[j + 1].second)));
        //                                 endpoints_block.push_back(std::vector<std::pair<Point, Point>>());
        //                                 endpoints_block.back().push_back(std::make_pair(Point(new_points[j].first, new_points[j].second),
        //                                 Point(new_points[j + 1].first, new_points[j + 1].second))); last_point =
        //                                 std::make_pair(Point(new_points[j+2].first, new_points[j].second), Point(new_points[j ].first,
        //                                 new_points[j].second));
        //                            }
        //                            else
        //                            {
        //                                 endpoints_block.back().push_back(std::make_pair(Point(new_points[j+2].first,
        //                                 new_points[j+2].second), Point(new_points[j + 3].first, new_points[j + 3].second)));
        //                                 endpoints_block.back().push_back(std::make_pair(Point(new_points[j].first,
        //                                 new_points[j+2].second), Point(new_points[j + 2].first, new_points[j + 2].second)));
        //                                 endpoints_block.push_back(std::vector<std::pair<Point, Point>>());
        //                                 last_point = std::make_pair(Point(new_points[j+2].first, new_points[j+2].second),
        //                                 Point(new_points[j+3].first, new_points[j+3].second));
        //                            }
        //                            //endpoints_block.back().push_back(std::make_pair(Point(new_points[j].first, new_points[j +
        //                            2].second), Point(new_points[j + 2].first, new_points[j + 2].second)));
        //                        }
        //                        else if (begin_input == 3)
        //                        {
        //                            if (new_points[j + 1].first > new_points[j + 3].first)
        //                            {
        //                                endpoints_block.back().push_back(std::make_pair(Point(new_points[j+2].first,
        //                                new_points[j+2].second), Point(new_points[j + 3].first, new_points[j + 3].second)));
        //                                endpoints_block.back().push_back(std::make_pair(Point(new_points[j+3].first,
        //                                new_points[j+3].second), Point(new_points[j + 1].first, new_points[j + 3].second)));
        //                                endpoints_block.push_back(std::vector<std::pair<Point, Point>>());
        //                                //endpoints_block.back().push_back(std::make_pair(Point(new_points[j + 2].first, new_points[j +
        //                                2].second), Point(new_points[j + 3].first, new_points[j + 3].second))); last_point =
        //                                std::make_pair(Point(new_points[j+2].first, new_points[j+2].second), Point(new_points[j+3].first,
        //                                new_points[j+3].second));
        //                            }
        //                            else {
        //                                endpoints_block.back().push_back(std::make_pair(Point(new_points[j].first, new_points[j].second),
        //                                Point(new_points[j + 1].first, new_points[j + 1].second)));
        //                                endpoints_block.push_back(std::vector<std::pair<Point, Point>>());
        //                                endpoints_block.back().push_back(std::make_pair(Point(new_points[j+1].first,
        //                                new_points[j].second), Point(new_points[j + 3].first, new_points[j ].second))); last_point =
        //                                std::make_pair(Point(new_points[j].first, new_points[j].second), Point(new_points[j + 1].first,
        //                                new_points[j + 1].second));
        //                            }
        //                            //endpoints_block.back().push_back(std::make_pair(Point(new_points[j].first, new_points[j].second),
        //                            Point(new_points[j + 1].first, new_points[j + 1].second)));
        //                            //endpoints_block.back().push_back(std::make_pair(Point(new_points[j + 1].first, new_points[j +
        //                            1].second), Point(new_points[j + 3].first, new_points[j + 1].second)));
        //                        }
        //                       // endpoints_block.push_back(std::vector<std::pair<Point, Point>>());
        //                       // last_point = std::make_pair(Point(new_points[j].first, new_points[j].second), Point(new_points[j +
        //                       1].first, new_points[j + 1].second));
        //
        //                       /* connect_for_one.push_back(polyline_copy.size() - 1);
        //                        connect_for_one.push_back(polyline_copy.size());
        //                        if (begin_input == 5)
        //                            connect_for_one.push_back(std::abs(new_points[j].first - new_points[j+2].first));
        //                        else if(begin_input == 3)
        //                            connect_for_one.push_back(std::abs(new_points[j+1].first - new_points[j+3].first));*/
        //                        break;
        //                    case 4:
        //                        polyline_copy.back().points.push_back(polyline_temp[i].points[j]);
        //                        polyline_copy.back().points.push_back(polyline_temp[i].points[j+1]);
        //                        polyline_copy.back().points.push_back(polyline_temp[i].points[j+2]);
        //                        polyline_copy.back().points.push_back(polyline_temp[i].points[j+3]);
        //                        polyline_copy.push_back(Polyline());
        //                        endpoints_block.back().push_back(std::make_pair(Point(new_points[j].first,new_points[j].second),Point(new_points[j+1].first,new_points[j+1].second)));
        //                        endpoints_block.back().push_back(std::make_pair(Point(new_points[j+2].first,new_points[j+2].second),Point(new_points[j+3].first,new_points[j+3].second)));
        //                        endpoints_block.push_back(std::vector<std::pair<Point, Point>>());
        //                        //endpoints_block.back().push_back(std::make_pair(Point(new_points[j+2].first,new_points[j+2].second),Point(new_points[j+3].first,new_points[j+3].second)));
        //                       /* connect_for_one.push_back(polyline_copy.size() - 1);
        //                        connect_for_one.push_back(polyline_copy.size());
        //                        connect_for_one.push_back(std::abs(new_points[j+2].first-new_points[j+3].first));*/
        //
        //                        last_point =
        //                        std::make_pair(Point(new_points[j+2].first,new_points[j+2].second),Point(new_points[j+3].first,new_points[j+3].second));
        //
        //                        j += 2;
        //                        break;
        //                    default:
        //                        polyline_copy.back().points.push_back(polyline_temp[i].points[j]);
        //                        polyline_copy.back().points.push_back(polyline_temp[i].points[j+1]);
        //                        break;
        //                    }
        //                }
        //                //outfile << " " << std::endl;
        // #endif
        //            }
        //
        //            //outfile.close();
        //
        //
        //            /*float distance_max = std::numeric_limits<float>::min();
        //            int index_dis = 0;
        //            for (int i = 0; i < distance_block.size(); i++)
        //            {
        //                for (int j = 0; j < distance_block[i].size(); j++)
        //                {
        //                    if (distance_block[i][j] > distance_max)
        //                    {
        //                        distance_max = distance_block[i][j];
        //                        index_dis = i;
        //                    }
        //                }
        //            }*/
        //
        //            Polylines polyline_rot;
        //            for (int i = 0; i < polyline_copy.size(); i++)
        //            {
        //                if(!polyline_copy[i].points.empty())
        //                    polyline_rot.push_back(Polyline());
        //                for (int j = 0; j < polyline_copy[i].points.size(); j++)
        //                {
        //                    Eigen::Vector2d point(polyline_copy[i].points[j][0] / 1000.f, polyline_copy[i].points[j][1] / 1000.f);
        //                    double angle = -M_PI / 4.0f;
        //                    Eigen::Rotation2D rotation(angle);
        //                    Eigen::Vector2d rotated_point = rotation * point;
        //                    polyline_rot.back().points.push_back(Point(rotated_point[0],rotated_point[1]));
        //                    //outfile << "(" << rotated_point[0]<< "," << rotated_point[1] << ")" << std::endl;
        //                }
        //                //outfile << " " << std::endl;
        //            }
        //            //outfile.close();
        //
        //
        //
        // #if plan_a
        //            auto dist = [&](int in) {
        //                int re = std::numeric_limits<float>::min();
        //                for (int i = 0; i < distance_block[in].size(); i++)
        //                {
        //                    if (distance_block[in][i] > re)
        //                        re = distance_block[in][i];
        //                }
        //                return re;
        //            };
        //
        //
        //            std::vector<int> sort_distance(distance_block.size());
        //            std::iota(sort_distance.begin(), sort_distance.end(), 0);
        //            std::sort(sort_distance.begin(), sort_distance.end(), [&](int a, int b) {
        //                float d1 = dist(a);
        //                float d2 = dist(b);
        //                if (d1 > d2)
        //                    return a;
        //                else
        //                    return b;
        //                });
        //
        //            // begin path planning
        //            // merge block
        //            // find connect block
        //            for (int i = 0; i < sort_distance.size(); i++)
        //            {
        //                pair_points_block[sort_distance[i]];
        //            }
        // #else
        //            //--resize blocks-----
        //            //----method 2----
        //            //std::vector<std::vector<float>> map_distance(endpoints_block.size(), std::vector<float>());
        //            //for (int i = 0; i < endpoints_block.size(); i++)
        //            //{
        //            //    map_distance[i].resize(endpoints_block.size(), -1);
        //            //}
        //
        //            std::vector<int> mark_block(polyline_rot.size(),0);//0. normal 1.top 2.bottom
        //            for (int i = 0; i < polyline_rot.size(); i++)
        //            {
        //                if (polyline_copy[i].points.empty())
        //                    continue;
        //                if (std::abs(polyline_rot[i].points.back().y() - polyline_rot[i].points[polyline_rot[i].size() - 3].y()) < 100)
        //                {
        //                    mark_block[i] = 1;
        //                }
        //                if (std::abs(polyline_rot[i].points[0].y() - polyline_rot[i].points[2].y()) < 100)
        //                {
        //                    mark_block[i] = 2;
        //                    if(mark_block[i])
        //                        mark_block[i] = 3;
        //                }
        //            }
        //
        //            std::vector<std::vector<float>> map_distance(polyline_rot.size(), std::vector<float>());
        //            for (int i = 0; i < polyline_rot.size(); i++)
        //            {
        //                map_distance[i].resize(polyline_rot.size(), -1);
        //            }
        //
        //            auto point_distance=[&](Point&a,Point &b)->float{
        //                float a_x = a.x();
        //                float a_y = a.y();
        //                float b_x = b.x();
        //                float b_y = b.y();
        //                float ab_x = std::abs(a_x - b_x);
        //                float ab_y = std::abs(a_y - b_y);
        //                float d = std::sqrtf(ab_x*ab_x+ab_y*ab_y);
        //                return d;
        //            };
        //
        //            auto point_jug = [&](Point&a,Point &b)->bool{
        //                bool h_jug = std::abs(a.y() - b.y()) < LINE_WIDTH ? 1 : 0;
        //                bool w_jug = std::abs(a.x() - b.x()) < 800 ? 1 : 0;
        //                return h_jug & w_jug;
        //            };
        //
        //            std::vector<std::vector<Point>> point_container;
        //            for (int i = 0; i < polyline_rot.size(); i++)
        //            {
        //                point_container.push_back(std::vector<Point>());
        //                if (mark_block[i] == 0)
        //                {
        //                    point_container.back()={polyline_rot[i][0],polyline_rot[i][1],polyline_rot[i].back(),polyline_rot[i][polyline_rot[i].size()-2]};
        //                }
        //                else if (mark_block[i] == 1)
        //                {
        //                    point_container.back()={polyline_rot[i][0],polyline_rot[i][1],polyline_rot[i].back(),polyline_rot[i][polyline_rot[i].size()-2],
        //                                polyline_rot[i][polyline_rot[i].size()-3], polyline_rot[i][polyline_rot[i].size()-4]};
        //                }
        //                else if (mark_block[i] == 2)
        //                {
        //                    point_container.back()={polyline_rot[i][0],polyline_rot[i][1],polyline_rot[i].back(),polyline_rot[i][polyline_rot[i].size()-2],
        //                                polyline_rot[i][2],polyline_rot[i][3]};
        //                }
        //                else if (mark_block[i] == 3)
        //                {
        //                    point_container.back()={polyline_rot[i][0],polyline_rot[i][1],polyline_rot[i].back(),polyline_rot[i][polyline_rot[i].size()-2],
        //                                polyline_rot[i][2],polyline_rot[i][3],
        //                                polyline_rot[i][polyline_rot[i].size()-3], polyline_rot[i][polyline_rot[i].size()-4]};
        //                }
        //            }
        //
        //
        //            for (int i = 0; i < polyline_rot.size(); i++)
        //            {
        //                for (int j = 0; j < polyline_rot.size(); j++)
        //                {
        //                    if (i != j)
        //                    {
        //                        float dist = std::numeric_limits<float>::min();
        //                        for (int n = 0; n < point_container[i].size(); n++)
        //                        {
        //                            for (int m = 0; m < point_container[j].size(); m++)
        //                            {
        //                                if (std::abs(point_container[i][n].y() - point_container[j][m].y()) < 100)
        //                                    continue;
        //                                if (point_distance(point_container[i][n], point_container[j][m])<1200.f)
        //                                {
        //                                    float x1; float x2;
        //                                    if(n%2==1)
        //                                        x1 = std::abs(point_container[i][n].x() - point_container[i][n-1].x());
        //                                    else
        //                                        x1 = std::abs(point_container[i][n].x() - point_container[i][n+1].x());
        //                                    if (m % 2 == 1)
        //                                        x2 = std::abs(point_container[j][m].x() - point_container[j][m - 1].x());
        //                                    else
        //                                        x2 = std::abs(point_container[j][m].x() - point_container[j][m + 1].x());
        //                                    float d = x1 < x2 ? x1 : x2;
        //                                    dist = dist > d ? dist : d;
        //                                }
        //                            }
        //                        }
        //                        if (dist != std::numeric_limits<float>::min())
        //                        {
        //                            map_distance[i][j] = dist;
        //                            map_distance[j][i] = dist;
        //                        }
        //                    }
        //                }
        //
        //            }
        //
        //
        //            /*for (int i = 0; i < connect_for_one.size(); i+=3)
        //            {
        //                int x = connect_for_one[i];
        //                int y = connect_for_one[i + 1];
        //                int d = connect_for_one[i + 2];
        //                map_distance[x-1][y-1] = d;
        //            }*/
        //
        //            ////----- merge line -----------
        //           /* std::vector<std::vector<int>> merge_block(endpoints_block.size(), std::vector<int>());
        //            std::vector<std::pair<Vec2d, Vec2d>> merge_point;
        //            for (int i = 0; i < endpoints_block.size(); i++)
        //            {
        //                merge_block[i].resize(endpoints_block[i].size(), -1);
        //            }
        //            for (int i = 0; i < endpoints_block.size(); i++)
        //            {
        //                for (int j = 0; j < endpoints_block.size(); j++)
        //                {
        //                    if (i != j)
        //                    {
        //                        for (int n = 0; n < endpoints_block[i].size(); n++)
        //                            for (int m = 0; m < endpoints_block[j].size(); m++)
        //                            {
        //                                if (std::abs(endpoints_block[i][n].first.y() - endpoints_block[j][m].first.y()) < 100 &&
        //                                    std::abs(endpoints_block[i][n].second.y() - endpoints_block[j][m].second.y()) < 100)
        //                                {
        //                                    Vec2d left_p = endpoints_block[i][n].first.x() < endpoints_block[j][m].first.x() ?
        //                                        Vec2d(endpoints_block[i][n].first.x(), endpoints_block[i][n].first.y()) :
        //                                        Vec2d(endpoints_block[j][m].first.x(), endpoints_block[j][m].first.y());
        //                                    Vec2d right_p = endpoints_block[i][n].second.x() > endpoints_block[j][m].second.x() ?
        //                                        Vec2d(endpoints_block[i][n].second.x(), endpoints_block[i][n].second.y()) :
        //                                        Vec2d(endpoints_block[j][m].second.x(), endpoints_block[j][m].second.y());
        //
        //                                    merge_point.push_back(std::make_pair(left_p, right_p));
        //                                    merge_block[i][n] = merge_point.size() - 1;
        //                                    merge_block[j][m] = merge_point.size() - 1;
        //                                }
        //                            }
        //                    }
        //                }
        //            }*/
        //
        //            ////----- filter line -------------
        //            //for (int i = 0; i < endpoints_block.size(); i++)
        //            //{
        //            //    for (int j = i; j < endpoints_block.size(); j++)
        //            //    {
        //            //        if (i != j)
        //            //        {
        //            //            for (int n = 0; n < endpoints_block[i].size(); n++)
        //            //                for (int m = 0; m < endpoints_block[j].size(); m++)
        //            //                {
        //            //                    //--judge merge--- and big change
        //            //                    bool is_merge = false;
        //            //                    if (merge_block[i][n] != -1 && merge_block[j][m] != -1)
        //            //                        continue;
        //            //                    float dist1 = std::abs(endpoints_block[i][n].first.x() - endpoints_block[i][n].second.x());
        //            //                    float dist2 = std::abs(endpoints_block[j][m].first.x() - endpoints_block[j][m].second.x());
        //            //                    float x_span1 = std::abs(endpoints_block[i][n].first.x() - endpoints_block[j][m].first.x());
        //            //                    float x_span2 = std::abs(endpoints_block[i][n].second.x() - endpoints_block[j][m].second.x());
        //            //                    float y_span1 = std::abs(endpoints_block[i][n].first.y() - endpoints_block[j][m].first.y());
        //            //                    float y_span2 = std::abs(endpoints_block[i][n].second.y() - endpoints_block[j][m].second.y());
        //            //                    if (merge_block[i][n] != -1 && merge_block[j][m] == -1)
        //            //                    {
        //            //                        dist1 = std::abs(merge_point[merge_block[i][n]].first.x() -
        //            merge_point[merge_block[i][n]].second.x());
        //            //                        x_span1 = std::abs(merge_point[merge_block[i][n]].first.x() -
        //            endpoints_block[j][m].first.x());
        //            //                        y_span1 = std::abs(merge_point[merge_block[i][n]].first.y() -
        //            endpoints_block[j][m].first.y());
        //            //                        x_span2 = std::abs(merge_point[merge_block[i][n]].second.x() -
        //            endpoints_block[j][m].second.x());
        //            //                        y_span2 = std::abs(merge_point[merge_block[i][n]].second.y() -
        //            endpoints_block[j][m].second.y());
        //            //                        is_merge = true;
        //            //                    }
        //            //                    if (merge_block[j][m] != -1 && merge_block[i][n] == -1)
        //            //                    {
        //            //                        dist2 = std::abs(merge_point[merge_block[j][m]].first.x() -
        //            merge_point[merge_block[j][m]].second.x());
        //            //                        x_span1 = std::abs(merge_point[merge_block[j][m]].first.x() -
        //            endpoints_block[i][n].first.x());
        //            //                        y_span1 = std::abs(merge_point[merge_block[j][m]].first.y() -
        //            endpoints_block[i][n].first.y());
        //            //                        x_span2 = std::abs(merge_point[merge_block[j][m]].second.x() -
        //            endpoints_block[i][n].second.x());
        //            //                        y_span2 = std::abs(merge_point[merge_block[j][m]].second.y() -
        //            endpoints_block[i][n].second.y());
        //            //                        is_merge = true;
        //            //                    }
        //            //                    if (x_span1 < 1600 && y_span1 < LINE_WIDTH && x_span2 < 1600 && y_span2 < LINE_WIDTH)
        //            //                    {
        //            //                        /* float dist1 = std::abs(endpoints_block[i][n].first.x()-endpoints_block[i][n].second.x());
        //            //                        float dist2 = std::abs(endpoints_block[j][m].first.x()-endpoints_block[j][m].second.x());*/
        //            //                        if (std::abs(dist1 - dist2) < 1980)
        //            //                        {
        //            //                            float dist_final = (dist1 + dist2) / 2.0f;
        //            //                            if (is_merge)
        //            //                            {
        //            //                                dist1 = std::abs(endpoints_block[i][n].first.x() -
        //            endpoints_block[i][n].second.x());
        //            //                                dist2 = std::abs(endpoints_block[j][m].first.x() -
        //            endpoints_block[j][m].second.x());
        //            //                                dist_final = dist1 < dist2 ? dist1 : dist2;
        //            //                            }
        //            //                            if (!endpoints_mark[i][n] && endpoints_mark[j][m])
        //            //                            {
        //            //                            }
        //            //                            else
        //            //                            {
        //            //                                map_distance[i][j] = dist_final;
        //            //                                map_distance[j][i] = dist_final;
        //            //                            }
        //            //                        }
        //            //                    }
        //            //                }
        //            //        }
        //            //        else
        //            //        {
        //            //            map_distance[i][j] = 0;
        //            //        }
        //            //    }
        //            //}
        //
        //            //----------other method-------------------
        //
        //            /*for (int i = 0; i < endpoints_block.size(); i++)
        //            {
        //                for (int j = i; j < endpoints_block.size(); j++)
        //                {
        //                    if (i != j)
        //                    {
        //                         for (int n = 0; n < endpoints_block[i].size(); n++)
        //                             for (int m = 0; m < endpoints_block[j].size(); m++)
        //                             {
        //                                 bool is_merge = false;
        //                                 if (merge_block[i][n] != -1 && merge_block[j][m] != -1)
        //                                     continue;
        //                                 float dist1 = std::abs(endpoints_block[i][n].first.x() - endpoints_block[i][n].second.x());
        //                                 float dist2 = std::abs(endpoints_block[j][m].first.x() - endpoints_block[j][m].second.x());
        //                                 float x_span1 = std::abs(endpoints_block[i][n].first.x() - endpoints_block[j][m].first.x());
        //                                 float x_span2 = std::abs(endpoints_block[i][n].second.x() - endpoints_block[j][m].second.x());
        //                                 float y_span1 = std::abs(endpoints_block[i][n].first.y() - endpoints_block[j][m].first.y());
        //                                 float y_span2 = std::abs(endpoints_block[i][n].second.y() - endpoints_block[j][m].second.y());
        //                                 if (merge_block[i][n] != -1 && merge_block[j][m] == -1)
        //                                 {
        //                                     dist1 = std::abs(merge_point[merge_block[i][n]].first.x() -
        //                                     merge_point[merge_block[i][n]].second.x()); x_span1 =
        //                                     std::abs(merge_point[merge_block[i][n]].first.x() - endpoints_block[j][m].first.x()); y_span1
        //                                     = std::abs(merge_point[merge_block[i][n]].first.y() - endpoints_block[j][m].first.y());
        //                                     x_span2 = std::abs(merge_point[merge_block[i][n]].second.x() -
        //                                     endpoints_block[j][m].second.x()); y_span2 =
        //                                     std::abs(merge_point[merge_block[i][n]].second.y() - endpoints_block[j][m].second.y());
        //                                     is_merge = true;
        //                                 }
        //                                 if (merge_block[j][m] != -1 && merge_block[i][n] == -1)
        //                                 {
        //                                     dist2 = std::abs(merge_point[merge_block[j][m]].first.x() -
        //                                     merge_point[merge_block[j][m]].second.x()); x_span1 =
        //                                     std::abs(merge_point[merge_block[j][m]].first.x() - endpoints_block[i][n].first.x()); y_span1
        //                                     = std::abs(merge_point[merge_block[j][m]].first.y() - endpoints_block[i][n].first.y());
        //                                     x_span2 = std::abs(merge_point[merge_block[j][m]].second.x() -
        //                                     endpoints_block[i][n].second.x()); y_span2 =
        //                                     std::abs(merge_point[merge_block[j][m]].second.y() - endpoints_block[i][n].second.y());
        //                                     is_merge = true;
        //                                 }
        //
        //                                  if (x_span1 < 1600 && y_span1 < LINE_WIDTH && x_span2 < 1600 && y_span2 < LINE_WIDTH)
        //                                  {
        //
        //                                      if (std::abs(dist1 - dist2) < 1980)
        //                                      {
        //                                          float dist_final = (dist1 + dist2) / 2.0f;
        //                                          if (is_merge)
        //                                          {
        //                                              dist1 = std::abs(endpoints_block[i][n].first.x() -
        //                                              endpoints_block[i][n].second.x()); dist2 = std::abs(endpoints_block[j][m].first.x()
        //                                              - endpoints_block[j][m].second.x()); dist_final = dist1 < dist2 ? dist1 : dist2;
        //                                          }
        //                                            map_distance[i][j] = dist_final;
        //                                            map_distance[j][i] = dist_final;
        //                                      }
        //                                  }
        //                             }
        //                    }
        //                }
        //            }*/
        //
        //
        //
        //
        //            //-----find path-----------
        //            //std::vector<int> path = { 0 };
        //            //std::vector<bool> path_mark(map_distance.size(),false);
        //            //path_mark[0] = true;
        //            //int index_ptr = 0;
        //
        //            //while (1)
        //            //{
        //            //    bool brk = true;
        //            //    for (int i = 0; i < path_mark.size(); i++)
        //            //    {
        //            //        if (!path_mark[i])
        //            //            brk = false;
        //            //    }
        //            //    if (brk)
        //            //        break;
        //            //    path_mark[index_ptr] = true;
        //            //    float max_dist = std::numeric_limits<float>::min();
        //            //    int index_temp = index_ptr;
        //            //    bool is_next = true;
        //            //    for (int i = 0; i < map_distance[index_ptr].size(); i++)
        //            //    {
        //            //        if (map_distance[index_ptr][i] != -1 && map_distance[index_ptr][i] != 0 && !path_mark[i] &&
        //            map_distance[index_ptr][i] > max_dist)
        //            //        {
        //            //            max_dist = map_distance[index_ptr][i];
        //            //            index_temp = i;
        //            //            is_next = false;
        //            //        }
        //            //    }
        //
        //            //    if (is_next)
        //            //    {
        //            //        for (int i = 0; i < path_mark.size(); i++)
        //            //        {
        //            //            if (!path_mark[i])
        //            //            {
        //            //                index_ptr = i;
        //            //                path.push_back(index_ptr);
        //            //                break;
        //            //            }
        //            //        }
        //            //    }
        //            //    else
        //            //    {
        //            //        path.push_back(index_temp);
        //            //         index_ptr = index_temp;
        //            //    }
        //            //    //path_mark[index_temp] = true;
        //            //}
        //
        //            //------------------merge--------------
        //
        //            std::vector<std::vector<int>> path_container;
        //            std::vector<std::vector<bool>> mark_map(map_distance.size(), std::vector<bool>(map_distance.size(), false));
        //            while (true)
        //            {
        //                float v = std::numeric_limits<float>::min();
        //                int begin_idx = -1;
        //                int end_idx = -1;
        //                for (int i = 0; i < map_distance.size(); i++)
        //                {
        //                    for (int j = i + 1; j < map_distance[i].size(); j++)
        //                    {
        //                        if (!mark_map[i][j] && map_distance[i][j] > v)
        //                        {
        //                            v = map_distance[i][j];
        //                            begin_idx = i;
        //                            end_idx = j;
        //                        }
        //                    }
        //                }
        //                if (begin_idx == -1 || end_idx == -1)
        //                {
        //                    break;
        //                }
        //                mark_map[begin_idx][end_idx] = true;
        //                mark_map[end_idx][begin_idx] = true;
        //                path_container.push_back(std::vector<int>());
        //                path_container.back().push_back(begin_idx);
        //                path_container.back().push_back(end_idx);
        //            }
        //
        //            if (path_container.empty())
        //                goto outdoor;
        //
        //            std::vector<bool> path_mark(path_container.size(),false);
        //            std::vector<bool> block_mark(polyline_rot.size(),false);
        //
        //            auto find_next_block = [&](int in)->std::vector<std::pair<int,int>>{
        //                std::vector<std::pair<int,int>> result;
        //                for (int i = 0; i < path_container.size(); i++)
        //                {
        //                    if (!path_mark[i])
        //                    {
        //                        if (path_container[i][0] == in)
        //                        {
        //                            if(!block_mark[path_container[i][1]])
        //                                result.push_back(std::make_pair(path_container[i][1],i));
        //                        }
        //                        else if (path_container[i][1] == in)
        //                        {
        //                            if(!block_mark[path_container[i][0]])
        //                                result.push_back(std::make_pair(path_container[i][0],i));
        //                        }
        //                    }
        //                }
        //                return result;
        //            };
        //
        //
        //            //0:down 1:up
        //            auto find_path = [&](int next_step,bool orient)->std::vector<int> {
        //                int nx = next_step;
        //                std::vector<int> result;
        //                if(!block_mark[nx])
        //                    result = { nx };
        //                block_mark[nx] = true;
        //                while (1)
        //                {
        //                    float max_dist = std::numeric_limits<float>::min();
        //                    std::vector<std::pair<int, int>> sl = find_next_block(nx);
        //                    if (sl.empty())
        //                        return result;
        //
        //                    int index = -1;
        //                    for (int i = 0; i < sl.size(); i++)
        //                    {
        //                        if (orient)
        //                        {
        //                            if (polyline_rot[sl[i].first][0].y() < polyline_rot[nx].points[0].y())
        //                                continue;
        //                        }
        //                        else {
        //                            if (polyline_rot[sl[i].first][0].y() > polyline_rot[nx].points[0].y())
        //                                continue;
        //                        }
        //                        int x = path_container[sl[i].second][0];
        //                        int y = path_container[sl[i].second][1];
        //                        if (max_dist < map_distance[x][y])
        //                        {
        //                            max_dist = map_distance[x][y];
        //                            index = i;
        //                        }
        //                    }
        //                    if (index != -1)
        //                    {
        //                        path_mark[sl[index].second] = true;
        //                        if(!block_mark[sl[index].first])
        //                            result.push_back(sl[index].first);
        //                        block_mark[sl[index].first] = true;
        //                        nx = sl[index].first;
        //                    }
        //                    else
        //                        return result;
        //                }
        //            };
        //
        //            auto find_lines = [&](int bn)->std::vector<int> {
        //                int in1 = path_container[bn][0];
        //                int in2 = path_container[bn][1];
        //                path_mark[bn] = true;
        //                std::vector<int> tr;
        //                std::vector<int> br;
        //                if (polyline_rot[in1][0].y() > polyline_rot[in2][0].y())
        //                {
        //                    tr=find_path(in1,1);
        //                    std::sort(tr.begin(), tr.end());
        //                    br=find_path(in2,0);
        //                    std::sort(br.begin(),br.end());
        //                }
        //                else {
        //                    tr=find_path(in1,0);
        //                    std::sort(tr.begin(), tr.end());
        //                    br=find_path(in2,1);
        //                    std::sort(br.begin(),br.end());
        //                }
        //                tr.reserve(tr.size()+br.size());
        //                tr.insert(tr.end(),br.begin(),br.end());
        //                return tr;
        //            };
        //
        //            std::vector<int> path;
        //            std::vector<std::vector<int>> path_array;
        //            for (int i = 0; i < path_container.size(); i++)
        //            {
        //                if (!path_mark[i])
        //                {
        //                    path_array.push_back(std::vector<int>());
        //                    path_array.back() = find_lines(i);
        //                }
        //            }
        //            for (int i = 0; i < path_array.size(); i++)
        //            {
        //                path.insert(path.end(), path_array[i].begin(), path_array[i].end());
        //            }

        //---other----

        Polylines poly_vec;
        Polylines return_polylines;
        for (size_t i = 0; i < polyline_baseline.size(); i++) {
            poly_vec.push_back(Polyline());
            return_polylines.push_back(Polyline());
            for (size_t j = 0; j < polyline_baseline[i].size()-1; j++) {
                if (j == 0) {
                    poly_vec.back().points.push_back(polyline_baseline[i][j]);
                    poly_vec.back().points.push_back(polyline_baseline[i][j + 1]);
                    return_polylines.back().points.push_back(polylines[i][j]);
                    return_polylines.back().points.push_back(polylines[i][j + 1]);
                    j++;
                    continue;
                } else if (!poly_vec.back().empty() && std::abs(polyline_baseline[i][j].y() - polyline_baseline[i][j + 1].y()) < 10 * sclae_width) {
                    long long              d1   = std::abs(polyline_baseline[i][j].x() - polyline_baseline[i][j + 1].x());
                    long long              d2   = std::abs(poly_vec.back().points.back().x() -
                                            poly_vec.back().points[poly_vec.back().points.size() - 2].x());
                    std::vector<long long> dist = {polyline_baseline[i][j].x(), polyline_baseline[i][j + 1].x(),
                                                   poly_vec.back().points.back().x(),
                                                   poly_vec.back().points[poly_vec.back().points.size() - 2].x()};
                    std::sort(dist.begin(), dist.end());
                    long long d = std::abs(dist.back() - dist.front());

                    poly_vec.back().points.push_back(polyline_baseline[i][j]);
                    poly_vec.back().points.push_back(polyline_baseline[i][j + 1]);
                    return_polylines.back().points.push_back(polylines[i][j]);
                    return_polylines.back().points.push_back(polylines[i][j + 1]);
                    if (d >= d1 + d2 + 1800000 /** sclae_width*/ || std::abs(d1 - d2) > 2000000 /** sclae_width*/) {
                        poly_vec.push_back(Polyline());
                    }
                    j++;
                } else if (poly_vec.back().empty() && std::abs(polyline_baseline[i][j].y() - polyline_baseline[i][j + 1].y()) < 10 * sclae_width) {
                    poly_vec.back().points.push_back(polyline_baseline[i][j]);
                    poly_vec.back().points.push_back(polyline_baseline[i][j + 1]);
                    return_polylines.back().points.push_back(polylines[i][j]);
                    return_polylines.back().points.push_back(polylines[i][j + 1]);
                    j++;
                } else if (std::abs(polyline_baseline[i][j].y() - polyline_baseline[i][j + 1].y()) > 5 * sclae_width) {
                    poly_vec.back().points.push_back(polyline_baseline[i][j]);
                    return_polylines.back().points.push_back(polylines[i][j]);
                } else {
                    poly_vec.back().points.push_back(polyline_baseline[i][j]);
                }
            }
        }

        for (int i = 0; i < poly_vec.size(); i++)
            if (poly_vec[i].points.empty()) {
                poly_vec.erase(poly_vec.begin() + i);
                i--;
            }

        std::sort(poly_vec.begin(), poly_vec.end(), [&](Polyline& a, Polyline& b) { return a.points[0].y() < b.points[0].y(); });

        std::vector<bool> poly_vec_mark(poly_vec.size(), false);
        int               index_ptr                  = 0;
        poly_vec_mark[0]                             = true;
        std::vector<int>              path_polylines = {0};
        std::vector<std::vector<int>> path_demo;
        path_demo.push_back(std::vector<int>() = {0});
        while (1) {
            std::vector<int> save_poly;
            std::vector<int> pi_c;
            for (int i = 0; i < poly_vec.size(); i++) {
                if (!poly_vec_mark[i]) {
                    int pi        = 0;
                    int back_size = poly_vec[index_ptr].size();
                    for (int j = 0; j < poly_vec[i].size()-1; j++) {
                        if (std::abs(poly_vec[i][j].y() - poly_vec[i][j + 1].y()) < 20 * sclae_width) {
                            pi = j;
                            break;
                        }
                    }

                    if (std::abs(poly_vec[i][pi].y() - poly_vec[index_ptr].back().y()) < LINE_WIDTH * sclae_width * 1000.) {
                        pi_c.push_back(pi);
                        save_poly.push_back(i);
                    }
                }
            }

            //------------maybe big change--------------
            int p_size = poly_vec[index_ptr].size();

            long long left  = poly_vec[index_ptr].back().x();
            long long right = poly_vec[index_ptr][poly_vec[index_ptr].size() - 2].x();
            if (p_size >= 4) {
                if (std::abs(poly_vec[index_ptr][p_size - 3].y() - poly_vec[index_ptr][p_size - 4].y()) < 20 * sclae_width) {
                    std::vector<long long> x1 = {poly_vec[index_ptr][p_size - 1].x(), poly_vec[index_ptr][p_size - 2].x()};
                    std::sort(x1.begin(), x1.end());
                    std::vector<long long> x2 = {poly_vec[index_ptr][p_size - 3].x(), poly_vec[index_ptr][p_size - 4].x()};
                    std::sort(x2.begin(), x2.end());
                    if (std::abs(x1.front() - x2.front()) > 2800 * 1000 /** sclae_width*/) {
                        if (std::abs(x1.front() - x2.front()) >
                            std::abs(poly_vec[index_ptr].back().x() - poly_vec[index_ptr][poly_vec[index_ptr].size() - 2].x())) {
                            left  = x1.front() > x2.front() ? x2.front() : x1.front();
                            right = x1.front() > x2.front() ? x1.front() : x2.front();
                        }
                    }
                    if (std::abs(x1.back() - x2.back()) > 2800 * 1000 /** sclae_width*/) {
                        if (std::abs(x1.back() - x2.back()) >
                            std::abs(poly_vec[index_ptr].back().x() - poly_vec[index_ptr][poly_vec[index_ptr].size() - 2].x())) {
                            left  = x1.back() > x2.back() ? x2.back() : x1.back();
                            right = x1.back() > x2.back() ? x1.back() : x2.back();
                        }
                    }
                }
            }
        

            std::vector<int> index_container;
            for (int i = 0; i < save_poly.size(); i++) {
                long long              bs     = std::abs(poly_vec[save_poly[i]][pi_c[i]].x() - poly_vec[save_poly[i]][pi_c[i] + 1].x());
                long long              ts     = std::abs(left - right);
                std::vector<long long> x_sort = {poly_vec[save_poly[i]][pi_c[i]].x(), poly_vec[save_poly[i]][pi_c[i] + 1].x(), left, right};

                std::sort(x_sort.begin(), x_sort.end());
                long long dd = x_sort.back() - x_sort.front();

                if (dd < bs + ts && (bs - ts) < ts) {
                    index_container.push_back(save_poly[i]);
                }
            }

            int       index      = -1;
            long long max_x_span = std::numeric_limits<long long>::min();
            for (int i = 0; i < index_container.size(); i++) {
                long long x_span = std::abs(poly_vec[index_container[i]][0].x() - poly_vec[index_container[i]][1].x());
                if (x_span > max_x_span) {
                    index      = index_container[i];
                    max_x_span = x_span;
                }
            }

            if (index != -1) {
                path_polylines.push_back(index);
                path_demo.back().push_back(index);
                poly_vec_mark[index] = true;
                index_ptr            = index;
            }

            bool is_brk = true;
            for (int i = 0; i < poly_vec_mark.size(); i++) {
                if (!poly_vec_mark[i]) {
                    if (index == -1) {
                        index_ptr = i;
                        path_polylines.push_back(i);
                        path_demo.push_back(std::vector<int>());
                        path_demo.back().push_back(i);
                        poly_vec_mark[i] = true;
                    }
                    is_brk = false;
                    break;
                }
            }
            if (is_brk)
                break;
        }

        //---------------------------------------------------------

        /*std::sort(path_demo.begin(), path_demo.end(), [&](std::vector<int>& a,std::vector<int>& b) {
            return poly_vec[a[0]].points[0].y() > poly_vec[b[0]].points[0].y();
            });*/

        // std::vector<bool> reverse_bool(poly_vec.size(),false);
        // std::vector<int> ys;
        // for (int i = 0; i < poly_vec.size(); i++)
        //{
        //     for (int j = 0; j < poly_vec.size(); j++)
        //     {
        //         if (i != j&&!reverse_bool[i]&&!reverse_bool[j])
        //         {
        //             if (std::abs(poly_vec[i].back().y() - poly_vec[j][0].y()) < LINE_WIDTH * 1000)
        //             {
        //                 long long bs = std::abs(poly_vec[i].back().x() - poly_vec[i][poly_vec[i].size()-2].x());
        //                 long long ts = std::abs(poly_vec[j][0].x() - poly_vec[j][1].y());
        //                 std::vector<long long> x_sort = {poly_vec[i].back().x(),poly_vec[i][poly_vec[i].size()-2].x(),
        //                    poly_vec[j][0].x(),poly_vec[j][1].y()};

        //                std::sort(x_sort.begin(), x_sort.end());
        //                long long dd = x_sort.back() - x_sort.front();
        //
        //                if (dd < bs + ts)
        //                {
        //                    //poly_vec[i][poly_vec[i].size() - 1] =
        //                    Point(poly_vec[i][poly_vec[i].size()-1].x(),poly_vec[i][poly_vec[i].size() - 1].y()-5800);
        //                    //poly_vec[i][poly_vec[i].size() - 2] =
        //                    Point(poly_vec[i][poly_vec[i].size()-2].x(),poly_vec[i][poly_vec[i].size() - 2].y()-5800); int index =
        //                    poly_vec[i][0].y() > poly_vec[j][0].y() ? j : i; ys.push_back(index); reverse_bool[index] = true;
        //                }
        //                break;
        //            }
        //        }
        //    }
        //}

        /* std::ofstream outfile("output.txt");
         for (int i = 0; i < poly_vec.size(); i++)
         {
             outfile << "------" << i << "---------" << std::endl;
             for (int j = 0; j < poly_vec[i].size(); j++)
             {
                 outfile << "(" << poly_vec[i][j].x() << "," << poly_vec[i][j].y() << ")" << std::endl;
             }
             outfile << " " << std::endl;
         }
         outfile.close();*/

        std::vector<std::vector<int>> path_demo1;
        std::vector<bool>             mark_path_demo(path_demo.size(), false);
        for (int i = 0; i < path_demo.size(); i++) {
            for (int n = 0; n < path_demo[i].size(); n++) {
                for (int j = 0; j < path_demo.size(); j++) {
                    if (i != j && !mark_path_demo[j]) {
                        // for (int k = 0; k < poly_vec[path_demo[i][n]].points.size(); k++)
                        {
                            float by = poly_vec[path_demo[i][n]].points[0].y(); //...
                            float ty = poly_vec[path_demo[j].back()].points.back().y();
                            if ((by - ty) < LINE_WIDTH * sclae_width * 1000 && (by - ty) > 0) {
                                //----is cover----------
                                long long bs = std::abs(poly_vec[path_demo[i][n]].points[0].x() - poly_vec[path_demo[i][n]].points[1].x());
                                long long ts = std::abs(
                                    poly_vec[path_demo[j].back()].points.back().x() -
                                    poly_vec[path_demo[j].back()].points[poly_vec[path_demo[j].back()].points.size() - 2].x());
                                std::vector<long long> x_sort =
                                    {poly_vec[path_demo[i][n]].points[0].x(), poly_vec[path_demo[i][n]].points[1].x(),
                                     poly_vec[path_demo[j].back()].points.back().x(),
                                     poly_vec[path_demo[j].back()].points[poly_vec[path_demo[j].back()].points.size() - 2].x()};

                                std::sort(x_sort.begin(), x_sort.end());
                                long long dd = x_sort.back() - x_sort.front();
                                if (dd < bs + ts) {
                                    mark_path_demo[j] = true;
                                    path_demo1.push_back(std::vector<int>());
                                    for (int m = 0; m < n; m++) {
                                        path_demo1.back().push_back(path_demo[i][m]);
                                    }
                                    path_demo1.push_back(std::vector<int>());
                                    for (int m = 0; m < path_demo[j].size(); m++) {
                                        path_demo1.back().push_back(path_demo[j][m]);
                                    }
                                    path_demo1.push_back(std::vector<int>());
                                    for (int m = n; m < path_demo[i].size(); m++) {
                                        path_demo1.back().push_back(path_demo[i][m]);
                                    }
                                    goto outside;
                                }
                            }
                        }
                    }
                }
            }
            if (!mark_path_demo[i]) {
                path_demo1.push_back(std::vector<int>());
                for (int n = 0; n < path_demo[i].size(); n++) {
                    path_demo1.back().push_back(path_demo[i][n]);
                }
            }
        outside:
            continue;
        }

        for (int i = 0; i < poly_vec.size(); i++) {
            for (int j = 0; j < poly_vec[i].points.size(); j++) {
                Eigen::Vector2d   point(poly_vec[i].points[j][0], poly_vec[i].points[j][1]);
                double            angle = (double) rot_angle;
                Eigen::Rotation2D rotation(angle);
                Eigen::Vector2d   rotated_point = rotation * point;
                poly_vec[i].points[j]           = (Point((double) rotated_point[0], (double) rotated_point[1]));
            }
        }

        // for (int i = 0; i < ys.size(); i++)
        //{
        //     //poly_vec[ys[i]].points.clear();
        //     for (int j = 0,k=poly_vec[ys[i]].points.size()-1; j < k; j++,k--)
        //     {
        //         Point tmp = poly_vec[ys[i]].points[j];
        //         poly_vec[ys[i]].points[j] = poly_vec[ys[i]].points[k];
        //         poly_vec[ys[i]].points[k] = tmp;
        //     }
        // }

        std::vector<int> path_demo_total;
        for (int i = 0; i < path_demo1.size(); i++)
            for (int j = 0; j < path_demo1[i].size(); j++) {
                bool insert = true;
                for (int n = 0; n < path_demo_total.size(); n++) {
                    if (path_demo1[i][j] == path_demo_total[n]) {
                        insert = false;
                    }
                }
                if (insert)
                    path_demo_total.push_back(path_demo1[i][j]);
            }

        polylines.clear();
        for (int i = 0; i < path_demo_total.size(); i++) {
            polylines.push_back(poly_vec[path_demo_total[i]]);
        }
        /*for (int i = 0; i < path_demo1.size(); i++)
        {
            for (int j = 0; j < path_demo1[i].size(); j++)
            {
                polylines.push_back(poly_vec[path_demo1[i][j]]);
            }
        }*/
        poly_vec.clear();

        // polylines.clear();
        // std::vector<bool> pcp(polyline_copy.size(), false);
        // for (int i = 0; i < path.size(); i++)
        //{
        //    polylines.push_back(polyline_copy[path[i]]);
        //   // pcp[path[i]] = true;
        //   /* for (int j = 0; j < polylines[i].size(); j++)
        //    {
        //        outfile << "(" << polylines[i].points[j][0]<< "," << polylines[i].points[j][1] << ")" << std::endl;
        //    }
        //    outfile << " "<< std::endl;*/
        //}
        /* outfile.close();
         for (int i = 0; i < pcp.size(); i++)
         {
             if (!pcp[i])
                 polylines.push_back(polyline_copy[i]);
         }*/

        /*std::vector<bool> mark_path(path_container.size(), false);
        int path_index = 0;
        mark_path[0] = true;
        std::vector<int> path = path_container[0];
        while (true)
        {
            int before_index = path_index;
            for (int i = 0; i < path_container.size(); i++)
            {
                if (!mark_path[i])
                {
                    if (path_container[path_index][1] == path_container[i][0])
                    {
                        path.push_back(path_container[i][1]);
                        mark_path[i] = true;
                        path_index = i;
                        break;
                    }
                }
            }

            bool is_brk = true;
            for (int i = 0; i < mark_path.size(); i++)
            {
                if (!mark_path[i])
                    is_brk = false;
            }
            if (is_brk)
                break;

            if (before_index == path_index)
            {
                for (int i = 0; i < mark_path.size(); i++)
                    if (!mark_path[i])
                    {
                        path_index = i;
                        mark_path[i] = true;
                        path.push_back(path_container[i][0]);
                        path.push_back(path_container[i][1]);
                        break;
                    }
            }
        }


        for (int i = 0; i < map_distance.size(); i++)
        {
            bool is_lost = true;
            for (int j = 0; j < path.size(); j++)
            {
                if (i == path[j])
                {
                    is_lost = false;
                }
            }

            if (is_lost)
                path.push_back(i);
        }*/

#endif

        /*std::sort(areas_container.begin(), areas_container.end(), [](std::pair<double, int>& a, std::pair<double, int>& b) {
            if (a.first > b.first)
                return true;
            else
                return false;
            });*/
        /* Polylines polylines_copy;
         for (int i = 0; i < sort_distance.size(); i++)
             polylines_copy.push_back(polylines[sort_distance[i]]);

         polylines.clear();
         polylines = polylines_copy;*/

        //---------swap----------
        // mark_polylines
        /*Polylines polyline_temp=polylines;
        for (int i = 0; i < polylines.size(); i++)
        {
            polyline_temp[i].points.clear();
            for (int j = 0; j < polylines[i].points.size(); j++)
            {
                if (mark_polylines[i][j])
                    continue;
                else
                {
                    polyline_temp[i].points.push_back(polylines[i].points[j]);
                }
            }
        }

        polylines.clear();
        polylines = polyline_temp;*/

        // merge-->separate

        // std::sort(poly_tatal.points.begin(), poly_tatal.points.end(), [](Point& a, Point& b) {
        //     return a.y() < b.y();
        //     });

        //
        // Polylines poly_container;
        // std::vector<bool> poly_mark(poly_tatal.points.size(),false);
        // while (1)
        //{
        //    bool is_break=true;
        //    Polyline poly_path;
        //    int current_index = 0;
        //    for (size_t i = 0; i < poly_tatal.size(); i++)//maybe lose but it is impossible
        //    {
        //        if (!poly_mark[i] && std::abs(poly_tatal[i].y()-poly_tatal[i+1].y())<60)
        //        {
        //            poly_path.points.push_back(poly_tatal[i]);
        //            poly_path.points.push_back(poly_tatal[i+1]);
        //            poly_mark[i] = true;
        //            poly_mark[i + 1] = true;
        //            current_index = i;
        //            is_break = false;
        //            break;
        //        }
        //    }
        //    if (is_break)
        //        break;
        //
        //    int index_ptr = current_index;
        //    for (size_t i = current_index; i < poly_tatal.size(); i++)
        //    {
        //        if (poly_mark[i])
        //            continue;
        //        if (std::abs(poly_tatal[i].y() - poly_tatal[i + 1].y()) > 1000 && std::abs(poly_tatal[i].y() - poly_tatal[i + 1].y()) <
        //        LINE_WIDTH*1000&&
        //            std::abs(poly_tatal[i].y() - poly_tatal[index_ptr].y())<LINE_WIDTH*1000 &&
        //            (std::abs(poly_tatal[i].x() - poly_tatal[index_ptr].x())<1200000.f||std::abs(poly_tatal[i].x() -
        //            poly_tatal[index_ptr+1].x())<1200000.f))
        //        {
        //            poly_path.points.push_back(poly_tatal[i]);
        //            poly_mark[i] = true;
        //            continue;
        //        }
        //        // need to find overcover are,if cover then add it
        //        if (std::abs(poly_tatal[i].y() - poly_tatal[i + 1].y()) < 1000 &&std::abs(poly_tatal[i].y() - poly_tatal[index_ptr].y()) <
        //        LINE_WIDTH*1000 &&
        //            (std::abs(poly_tatal[i].x() - poly_tatal[index_ptr].x())<1200000.f||std::abs(poly_tatal[i+1].x() -
        //            poly_tatal[index_ptr].x())<1200000.f||
        //                std::abs(poly_tatal[i].x() - poly_tatal[index_ptr+1].x())<1200000.f||std::abs(poly_tatal[i+1].x() -
        //                poly_tatal[index_ptr+1].x())<1200000.f))
        //        {
        //            // is cover
        //            long long distance_two_line = std::abs(poly_tatal[i + 1].x() - poly_tatal[i].x()) + std::abs(poly_tatal[index_ptr].x()
        //            - poly_tatal[index_ptr+1].x()); std::vector<long long> point_d =
        //            {poly_tatal[i].x(),poly_tatal[i+1].x(),poly_tatal[index_ptr].x() , poly_tatal[index_ptr+1].x()};
        //            std::sort(point_d.begin(),point_d.end());
        //            long long distance_tatal = point_d.back() - point_d[0];
        //            if (distance_tatal<distance_two_line)
        //            {
        //                poly_path.points.push_back(poly_tatal[i]);
        //                poly_path.points.push_back(poly_tatal[i + 1]);
        //                index_ptr = i;
        //                poly_mark[i] = true;
        //                poly_mark[i + 1] = true;
        //            }
        //        }
        //    }
        //

        //    if (!poly_path.empty())
        //    {
        //        poly_container.push_back(poly_path);
        //    }
        //}

        // for (int i = 0; i < poly_container.size(); i++)
        //{
        //     for (int j = 0; j < poly_container[i].points.size(); j++)
        //     {
        //         Eigen::Vector2d point(poly_container[i].points[j][0],poly_container[i].points[j][1]);
        //         double angle = M_PI / 4.0f;
        //         Eigen::Rotation2D rotation(angle);
        //         Eigen::Vector2d rotated_point = rotation * point;
        //         outfile << "(" << poly_container[i].points[j][0]<< "," << poly_container[i].points[j][1] << ")" << std::endl;
        //         poly_container[i].points[j]=(Point((double)rotated_point[0],(double)rotated_point[1]));
        //     }
        //      outfile << " " << std::endl;
        // }
        // outfile.close();

        // polylines.clear();
        // polylines = poly_container;

        // std::sort(polyline_baseline.begin(), polyline_baseline.end(), [&](Polyline &a,Polyline &b){
        //     return a.points[0].y() < b.points[0].y();
        //     });

        // std::vector<bool> polyline_mark(polyline_baseline.size(),false);
        // std::vector<int> path_a;
        // int polyline_index=-1;
        // while (1)
        //{
        //     if (polyline_index == -1)
        //     {
        //         for (int i = 0; i < polyline_mark.size(); i++)
        //         {
        //             if (!polyline_mark[i])
        //             {
        //                 polyline_index = i;
        //                 break;
        //             }
        //         }
        //     }
        //     if (polyline_index == -1)
        //         break;
        //     path_a.push_back(polyline_index);
        //     polyline_mark[polyline_index] = true;
        //

        //    float standard_y = polyline_baseline[polyline_index].points.back().y();
        //    float standard_x = polyline_baseline[polyline_index].points.back().x();
        //    std::vector<int> path_next;
        //    for (int i = 0; i < polyline_baseline.size(); i++)
        //    {
        //        if (!polyline_mark[i] && polyline_baseline[i].points[0].y() - standard_y < LINE_WIDTH &&
        //            (std::abs(polyline_baseline[i].points[0].x()-standard_x)<1200.f ||
        //            std::abs(polyline_baseline[i].points[1].x()-standard_x)<1200.f))//this maybe lose
        //        {
        //            path_next.push_back(i);
        //        }
        //    }

        //    int max_i = -1;
        //    float max_dx = std::numeric_limits<float>::min();
        //    for (int i = 0; i < path_next.size(); i++)
        //    {
        //        float dx = std::abs(polyline_baseline[path_next[i]].points[0].x() - polyline_baseline[path_next[i]].points[1].x());
        //        if (dx > max_dx)
        //        {
        //            max_i = path_next[i];
        //            max_dx = dx;
        //        }
        //    }
        //    polyline_index = max_i;
        //}

        // for (int i = 0; i < polyline_baseline.size(); i++)
        //{
        //     for (int j = 0; j < polyline_baseline[i].points.size(); j++)
        //     {
        //         Eigen::Vector2d point(polyline_baseline[i].points[j][0] * 1000.f, polyline_baseline[i].points[j][1] * 1000.f);
        //         double angle = M_PI / 4.0f;
        //         Eigen::Rotation2D rotation(angle);
        //         Eigen::Vector2d rotated_point = rotation * point;
        //         polyline_baseline[i].points[j]=(Point(rotated_point[0],rotated_point[1]));
        //     }
        // }

        // polylines.clear();
        // for (int i = 0; i < path_a.size(); i++)
        //{
        //     polylines.push_back(polyline_baseline[path_a[i]]);
        // }

        break;
    }

outdoor:
    if (!polylines.empty() || !thick_polylines.empty()) {
        // calculate actual flow from spacing (which might have been adjusted by the infill
        // pattern generator)
        double flow_mm3_per_mm = params.flow.mm3_per_mm();
        double flow_width      = params.flow.width();
        if (params.using_internal_flow) {
            // if we used the internal flow we're not doing a solid infill
            // so we can safely ignore the slight variation that might have
            // been applied to f->spacing
        } else {
            Flow new_flow   = params.flow.with_spacing(this->spacing);
            flow_mm3_per_mm = new_flow.mm3_per_mm();
            flow_width      = new_flow.width();
        }
        // Save into layer.
        ExtrusionEntityCollection* eec = nullptr;
        out.push_back(eec = new ExtrusionEntityCollection());
        // Only concentric fills are not sorted.
        eec->no_sort = this->no_sort();
        size_t idx   = eec->entities.size();
        if (params.use_arachne) {
            Flow new_flow = params.flow.with_spacing(float(this->spacing));
            variable_width(thick_polylines, params.extrusion_role, new_flow, eec->entities);
            thick_polylines.clear();
        } else {
            extrusion_entities_append_paths(eec->entities, std::move(polylines), params.extrusion_role, flow_mm3_per_mm, float(flow_width),
                                            params.flow.height());
        }
        if (!params.can_reverse) {
            for (size_t i = idx; i < eec->entities.size(); i++)
                eec->entities[i]->set_reverse();
        }

        // Orca: run gap fill
        this->_create_gap_fill(surface, params, eec);
    }
}

// Orca: Dedicated function to calculate gap fill lines for the provided surface, according to the print object parameters
// and append them to the out ExtrusionEntityCollection.
void Fill::_create_gap_fill(const Surface* surface, const FillParams& params, ExtrusionEntityCollection* out){
    
    //Orca: just to be safe, check against null pointer for the print object config and if NULL return.
    if (this->print_object_config == nullptr) return;
    
    // Orca: Enable gap fill as per the user preference. Return early if gap fill is to not be applied.
    if ((this->print_object_config->gap_fill_target.value == gftNowhere) ||
        (surface->surface_type == stInternalSolid && this->print_object_config->gap_fill_target.value != gftEverywhere))
        return;
    
    Flow new_flow = params.flow;
    ExPolygons unextruded_areas;
    unextruded_areas = diff_ex(this->no_overlap_expolygons, union_ex(out->polygons_covered_by_spacing(10)));
    ExPolygons gapfill_areas = union_ex(unextruded_areas);
    if (!this->no_overlap_expolygons.empty())
        gapfill_areas = intersection_ex(gapfill_areas, this->no_overlap_expolygons);
    
    if (gapfill_areas.size() > 0 && params.density >= 1) {
        double min = 0.2 * new_flow.scaled_spacing() * (1 - INSET_OVERLAP_TOLERANCE);
        double max = 2. * new_flow.scaled_spacing();
        ExPolygons gaps_ex = diff_ex(
                                     opening_ex(gapfill_areas, float(min / 2.)),
                                     offset2_ex(gapfill_areas, -float(max / 2.), float(max / 2. + ClipperSafetyOffset)));
        //BBS: sort the gap_ex to avoid mess travel
        Points ordering_points;
        ordering_points.reserve(gaps_ex.size());
        ExPolygons gaps_ex_sorted;
        gaps_ex_sorted.reserve(gaps_ex.size());
        for (const ExPolygon &ex : gaps_ex)
            ordering_points.push_back(ex.contour.first_point());
        std::vector<Points::size_type> order2 = chain_points(ordering_points);
        for (size_t i : order2)
            gaps_ex_sorted.emplace_back(std::move(gaps_ex[i]));
        
        ThickPolylines polylines;
        for (ExPolygon& ex : gaps_ex_sorted) {
            //BBS: Use DP simplify to avoid duplicated points and accelerate medial-axis calculation as well.
            ex.douglas_peucker(SCALED_RESOLUTION * 0.1);

            coord_t epsilon_off = 500;
            Slic3r::ExPolygons result = Slic3r::offset_ex(Slic3r::offset_ex(ex, epsilon_off), -epsilon_off);
            for(Slic3r::ExPolygon& eex : result)
            {
                eex.medial_axis(min, max, &polylines);
            } 

            // ex.medial_axis(min, max, &polylines);
        }
        
        if (!polylines.empty() && !is_bridge(params.extrusion_role)) {
            polylines.erase(std::remove_if(polylines.begin(), polylines.end(),
                                           [&](const ThickPolyline& p) {
                return p.length() < scale_(params.config->filter_out_gap_fill.value);
            }), polylines.end());
            
            ExtrusionEntityCollection gap_fill;
            variable_width(polylines, erGapFill, params.flow, gap_fill.entities);
            auto gap = std::move(gap_fill.entities);
            out->append(gap);
        }
    }
}

// Calculate a new spacing to fill width with possibly integer number of lines,
// the first and last line being centered at the interval ends.
// This function possibly increases the spacing, never decreases, 
// and for a narrow width the increase in spacing may become severe,
// therefore the adjustment is limited to 20% increase.
coord_t Fill::_adjust_solid_spacing(const coord_t width, const coord_t distance)
{
    assert(width >= 0);
    assert(distance > 0);
    // floor(width / distance)
    const auto  number_of_intervals = coord_t((width - EPSILON) / distance);
    coord_t     distance_new        = (number_of_intervals == 0) ? 
        distance : 
        coord_t((width - EPSILON) / number_of_intervals);
    const coordf_t factor = coordf_t(distance_new) / coordf_t(distance);
    assert(factor > 1. - 1e-5);
    // How much could the extrusion width be increased? By 20%.
    const coordf_t factor_max = 1.2;
    if (factor > factor_max)
        distance_new = coord_t(floor((coordf_t(distance) * factor_max + 0.5)));
    return distance_new;
}

// Returns orientation of the infill and the reference point of the infill pattern.
// For a normal print, the reference point is the center of a bounding box of the STL.
std::pair<float, Point> Fill::_infill_direction(const Surface *surface) const
{
    // set infill angle
    float out_angle = this->angle;

	if (out_angle == FLT_MAX) {
		//FIXME Vojtech: Add a warning?
        printf("Using undefined infill angle\n");
        out_angle = 0.f;
    }

    // Bounding box is the bounding box of a perl object Slic3r::Print::Object (c++ object Slic3r::PrintObject)
    // The bounding box is only undefined in unit tests.
    Point out_shift = empty(this->bounding_box) ? 
    	surface->expolygon.contour.bounding_box().center() : 
        this->bounding_box.center();

#if 0
    if (empty(this->bounding_box)) {
        printf("Fill::_infill_direction: empty bounding box!");
    } else {
        printf("Fill::_infill_direction: reference point %d, %d\n", out_shift.x, out_shift.y);
    }
#endif

    if (surface->bridge_angle >= 0) {
	    // use bridge angle
		//FIXME Vojtech: Add a debugf?
        // Slic3r::debugf "Filling bridge with angle %d\n", rad2deg($surface->bridge_angle);
#ifdef SLIC3R_DEBUG
        printf("Filling bridge with angle %f\n", surface->bridge_angle);
#endif /* SLIC3R_DEBUG */
        out_angle = float(surface->bridge_angle);
    } else if (this->layer_id != size_t(-1)) {
        // alternate fill direction
        out_angle += this->_layer_angle(this->layer_id / surface->thickness_layers);
    } else {
//    	printf("Layer_ID undefined!\n");
    }

    out_angle += float(M_PI/2.);
    return std::pair<float, Point>(out_angle, out_shift);
}

// A single T joint of an infill line to a closed contour or one of its holes.
struct ContourIntersectionPoint {
    // Contour and point on a contour where an infill line is connected to.
    size_t                      contour_idx;
    size_t                      point_idx;
    // Eucleidean parameter of point_idx along its contour.
    double                      param;
    // Other intersection points along the same contour. If there is only a single T-joint on a contour
    // with an intersection line, then the prev_on_contour and next_on_contour remain nulls.
    ContourIntersectionPoint*   prev_on_contour { nullptr };
    ContourIntersectionPoint*   next_on_contour { nullptr };
    // Length of the contour not yet allocated to some extrusion path going back (clockwise), or masked out by some overlapping infill line.
    double                      contour_not_taken_length_prev { std::numeric_limits<double>::max() };
    // Length of the contour not yet allocated to some extrusion path going forward (counter-clockwise), or masked out by some overlapping infill line.
    double                      contour_not_taken_length_next { std::numeric_limits<double>::max() };
    // End point is consumed if an infill line connected to this T-joint was already connected left or right along the contour,
    // or if the infill line was processed, but it was not possible to connect it left or right along the contour.
    bool                        consumed { false };
    // Whether the contour was trimmed by an overlapping infill line, or whether part of this contour was connected to some infill line.
    bool                        prev_trimmed { false };
    bool                        next_trimmed { false };

    void                        consume_prev() { this->contour_not_taken_length_prev = 0.; this->prev_trimmed = true; this->consumed = true; }
    void                        consume_next() { this->contour_not_taken_length_next = 0.; this->next_trimmed = true; this->consumed = true; }

    void                        trim_prev(const double new_len) {
        if (new_len < this->contour_not_taken_length_prev) {
            this->contour_not_taken_length_prev = new_len;
            this->prev_trimmed = true;
        }
    }
    void                        trim_next(const double new_len) {
        if (new_len < this->contour_not_taken_length_next) {
            this->contour_not_taken_length_next = new_len;
            this->next_trimmed = true;
        }
    }

    // The end point of an infill line connected to this T-joint was not processed yet and a piece of the contour could be extruded going backwards.
    bool                        could_take_prev() const throw() { return ! this->consumed && this->contour_not_taken_length_prev > SCALED_EPSILON; }
    // The end point of an infill line connected to this T-joint was not processed yet and a piece of the contour could be extruded going forward.
    bool                        could_take_next() const throw() { return ! this->consumed && this->contour_not_taken_length_next > SCALED_EPSILON; }

    // Could extrude a complete segment from this to this->prev_on_contour.
    bool                        could_connect_prev() const throw() 
        { return ! this->consumed && this->prev_on_contour != this && ! this->prev_on_contour->consumed && ! this->prev_trimmed && ! this->prev_on_contour->next_trimmed; }
    // Could extrude a complete segment from this to this->next_on_contour.
    bool                        could_connect_next() const throw() 
        { return ! this->consumed && this->next_on_contour != this && ! this->next_on_contour->consumed && ! this->next_trimmed && ! this->next_on_contour->prev_trimmed; }
};

// Distance from param1 to param2 when going counter-clockwise.
static inline double closed_contour_distance_ccw(double param1, double param2, double contour_length)
{
    assert(param1 >= 0. && param1 <= contour_length);
    assert(param2 >= 0. && param2 <= contour_length);
    double d = param2 - param1;
    if (d < 0.)
        d += contour_length;
    return d;
}

// Distance from param1 to param2 when going clockwise.
static inline double closed_contour_distance_cw(double param1, double param2, double contour_length)
{
    return closed_contour_distance_ccw(param2, param1, contour_length);
}

// Length along the contour from cp1 to cp2 going counter-clockwise.
double path_length_along_contour_ccw(const ContourIntersectionPoint *cp1, const ContourIntersectionPoint *cp2, double contour_length)
{
    assert(cp1 != nullptr);
    assert(cp2 != nullptr);
    assert(cp1->contour_idx == cp2->contour_idx);
    assert(cp1 != cp2);
    return closed_contour_distance_ccw(cp1->param, cp2->param, contour_length);
}

// Lengths along the contour from cp1 to cp2 going CCW and going CW.
std::pair<double, double> path_lengths_along_contour(const ContourIntersectionPoint *cp1, const ContourIntersectionPoint *cp2, double contour_length)
{
    // Zero'th param is the length of the contour.
    double param_lo  = cp1->param;
    double param_hi  = cp2->param;
    assert(param_lo >= 0. && param_lo <= contour_length);
    assert(param_hi >= 0. && param_hi <= contour_length);
    bool  reversed  = false;
    if (param_lo > param_hi) {
        std::swap(param_lo, param_hi);
        reversed = true;
    }
    auto out = std::make_pair(param_hi - param_lo, param_lo + contour_length - param_hi);
    if (reversed)
        std::swap(out.first, out.second);
    return out;
}

// Add contour points from interval (idx_start, idx_end> to polyline.
static inline void take_cw_full(Polyline &pl, const Points &contour, size_t idx_start, size_t idx_end)
{
    assert(! pl.empty() && pl.points.back() == contour[idx_start]);
    size_t i = (idx_start == 0) ? contour.size() - 1 : idx_start - 1;
    while (i != idx_end) {
        pl.points.emplace_back(contour[i]);
        if (i == 0)
            i = contour.size();
        -- i;
    }
    pl.points.emplace_back(contour[i]);
}

// Add contour points from interval (idx_start, idx_end> to polyline, limited by the Eucleidean length taken.
static inline double take_cw_limited(Polyline &pl, const Points &contour, const std::vector<double> &params, size_t idx_start, size_t idx_end, double length_to_take)
{
    // If appending to an infill line, then the start point of a perimeter line shall match the end point of an infill line.
    assert(pl.empty() || pl.points.back() == contour[idx_start]);
    assert(contour.size() + 1 == params.size());
    assert(length_to_take > SCALED_EPSILON);
    // Length of the contour.
    double length = params.back();
    // Parameter (length from contour.front()) for the first point.
    double p0     = params[idx_start];
    // Current (2nd) point of the contour.
    size_t i      = (idx_start == 0) ? contour.size() - 1 : idx_start - 1;
    // Previous point of the contour.
    size_t iprev  = idx_start;
    // Length of the contour curve taken for iprev.
    double lprev  = 0.;

    for (;;) {
        double l = closed_contour_distance_cw(p0, params[i], length);
        if (l >= length_to_take) {
            // Trim the last segment.
            double t = double(length_to_take - lprev) / (l - lprev);
            pl.points.emplace_back(lerp(contour[iprev], contour[i], t));
            return length_to_take;
        }
        // Continue with the other segments.
        pl.points.emplace_back(contour[i]);
        if (i == idx_end)
            return l;
        iprev = i;
        lprev = l;
        if (i == 0)
            i = contour.size();
        -- i;
    }
    assert(false);
    return 0;
}

// Add contour points from interval (idx_start, idx_end> to polyline.
static inline void take_ccw_full(Polyline &pl, const Points &contour, size_t idx_start, size_t idx_end)
{
    assert(! pl.empty() && pl.points.back() == contour[idx_start]);
    size_t i = idx_start;
    if (++ i == contour.size())
        i = 0;
    while (i != idx_end) {
        pl.points.emplace_back(contour[i]);
        if (++ i == contour.size())
            i = 0;
    }
    pl.points.emplace_back(contour[i]);
}

// Add contour points from interval (idx_start, idx_end> to polyline, limited by the Eucleidean length taken.
// Returns length of the contour taken.
static inline double take_ccw_limited(Polyline &pl, const Points &contour, const std::vector<double> &params, size_t idx_start, size_t idx_end, double length_to_take)
{
    // If appending to an infill line, then the start point of a perimeter line shall match the end point of an infill line.
    assert(pl.empty() || pl.points.back() == contour[idx_start]);
    assert(contour.size() + 1 == params.size());
    assert(length_to_take > SCALED_EPSILON);
    // Length of the contour.
    double length = params.back();
    // Parameter (length from contour.front()) for the first point.
    double p0     = params[idx_start];
    // Current (2nd) point of the contour.
    size_t i      = idx_start;
    if (++ i == contour.size())
        i = 0;
    // Previous point of the contour.
    size_t iprev  = idx_start;
    // Length of the contour curve taken at iprev.
    double lprev  = 0;
    for (;;) {
        double l = closed_contour_distance_ccw(p0, params[i], length);
        if (l >= length_to_take) {
            // Trim the last segment.
            double t = double(length_to_take - lprev) / (l - lprev);
            pl.points.emplace_back(lerp(contour[iprev], contour[i], t));
            return length_to_take;
        }
        // Continue with the other segments.
        pl.points.emplace_back(contour[i]);
        if (i == idx_end)
            return l;
        iprev = i;
        lprev = l;
        if (++ i == contour.size())
            i = 0;
    }
    assert(false);
    return 0;
}

// Connect end of pl1 to the start of pl2 using the perimeter contour.
// If clockwise, then a clockwise segment from idx_start to idx_end is taken, otherwise a counter-clockwise segment is being taken.
static void take(Polyline &pl1, const Polyline &pl2, const Points &contour, size_t idx_start, size_t idx_end, bool clockwise)
{
#ifndef NDEBUG
	assert(idx_start != idx_end);
    assert(pl1.size() >= 2);
    assert(pl2.size() >= 2);
#endif /* NDEBUG */

	{
		// Reserve memory at pl1 for the connecting contour and pl2.
		int new_points = int(idx_end) - int(idx_start) - 1;
		if (new_points < 0)
			new_points += int(contour.size());
		pl1.points.reserve(pl1.points.size() + size_t(new_points) + pl2.points.size());
	}

	if (clockwise)
        take_cw_full(pl1, contour, idx_start, idx_end);
	else
        take_ccw_full(pl1, contour, idx_start, idx_end);

    pl1.points.insert(pl1.points.end(), pl2.points.begin() + 1, pl2.points.end());
}

static void take(Polyline &pl1, const Polyline &pl2, const Points &contour, ContourIntersectionPoint *cp_start, ContourIntersectionPoint *cp_end, bool clockwise)
{
    assert(cp_start->prev_on_contour != nullptr);
    assert(cp_start->next_on_contour != nullptr);
    assert(cp_end  ->prev_on_contour != nullptr);
    assert(cp_end  ->next_on_contour != nullptr);
    assert(cp_start != cp_end);

    take(pl1, pl2, contour, cp_start->point_idx, cp_end->point_idx, clockwise);

    // Mark the contour segments in between cp_start and cp_end as consumed.
    if (clockwise)
        std::swap(cp_start, cp_end);
    if (cp_start->next_on_contour != cp_end)
        for (auto *cp = cp_start->next_on_contour; cp->next_on_contour != cp_end; cp = cp->next_on_contour) {
            cp->consume_prev();
            cp->consume_next();
        }
    cp_start->consume_next();
    cp_end->consume_prev();
}

static void take_limited(
    Polyline &pl1, const Points &contour, const std::vector<double> &params, 
    ContourIntersectionPoint *cp_start, ContourIntersectionPoint *cp_end, bool clockwise, double take_max_length, double line_half_width)
{
#ifndef NDEBUG
    // This is a valid case, where a single infill line connect to two different contours (outer contour + hole or two holes).
//    assert(cp_start != cp_end);
    assert(cp_start->prev_on_contour != nullptr);
    assert(cp_start->next_on_contour != nullptr);
    assert(cp_end  ->prev_on_contour != nullptr);
    assert(cp_end  ->next_on_contour != nullptr);
    assert(pl1.size() >= 2);
    assert(contour.size() + 1 == params.size());
#endif /* NDEBUG */

    if (! (clockwise ? cp_start->could_take_prev() : cp_start->could_take_next()))
        return;

    assert(pl1.points.front() == contour[cp_start->point_idx] || pl1.points.back() == contour[cp_start->point_idx]);
    bool        add_at_start = pl1.points.front() == contour[cp_start->point_idx];
    Points      pl_tmp;
    if (add_at_start) {
        pl_tmp = std::move(pl1.points);
        pl1.points.clear();
    }

    {
        // Reserve memory at pl1 for the perimeter segment.
        // Pessimizing - take the complete segment.
        int new_points = int(cp_end->point_idx) - int(cp_start->point_idx) - 1;
        if (new_points < 0)
            new_points += int(contour.size());
        pl1.points.reserve(pl1.points.size() + pl_tmp.size() + size_t(new_points));
    }

    double length = params.back();
    double length_to_go = take_max_length;
    cp_start->consumed = true;
    if (cp_start == cp_end) {
        length_to_go = std::max(0., std::min(length_to_go, length - line_half_width));
        length_to_go = std::min(length_to_go, clockwise ? cp_start->contour_not_taken_length_prev : cp_start->contour_not_taken_length_next);
        cp_start->consume_prev();
        cp_start->consume_next();
        if (length_to_go > SCALED_EPSILON)
            clockwise ?
                take_cw_limited (pl1, contour, params, cp_start->point_idx, cp_start->point_idx, length_to_go) :
                take_ccw_limited(pl1, contour, params, cp_start->point_idx, cp_start->point_idx, length_to_go);
    } else if (clockwise) {
        // Going clockwise from cp_start to cp_end.
        assert(cp_start != cp_end);
        for (ContourIntersectionPoint *cp = cp_start; cp != cp_end; cp = cp->prev_on_contour) {
            // Length of the segment from cp to cp->prev_on_contour.
            double l = closed_contour_distance_cw(cp->param, cp->prev_on_contour->param, length);
            length_to_go = std::min(length_to_go, cp->contour_not_taken_length_prev);
            //if (cp->prev_on_contour->consumed)
                // Don't overlap with an already extruded infill line.
                length_to_go = std::max(0., std::min(length_to_go, l - line_half_width));
            cp->consume_prev();
            if (l >= length_to_go) {
                if (length_to_go > SCALED_EPSILON) {
                    cp->prev_on_contour->trim_next(l - length_to_go);
                    take_cw_limited(pl1, contour, params, cp->point_idx, cp->prev_on_contour->point_idx, length_to_go);
                }
                break;
            } else {
                cp->prev_on_contour->trim_next(0.);
                take_cw_full(pl1, contour, cp->point_idx, cp->prev_on_contour->point_idx);
                length_to_go -= l;
            }
        }
    } else {
        assert(cp_start != cp_end);
        for (ContourIntersectionPoint *cp = cp_start; cp != cp_end; cp = cp->next_on_contour) {
            double l = closed_contour_distance_ccw(cp->param, cp->next_on_contour->param, length);
            length_to_go = std::min(length_to_go, cp->contour_not_taken_length_next);
            //if (cp->next_on_contour->consumed)
                // Don't overlap with an already extruded infill line.
                length_to_go = std::max(0., std::min(length_to_go, l - line_half_width));
            cp->consume_next();
            if (l >= length_to_go) {
                if (length_to_go > SCALED_EPSILON) {
                    cp->next_on_contour->trim_prev(l - length_to_go);
                    take_ccw_limited(pl1, contour, params, cp->point_idx, cp->next_on_contour->point_idx, length_to_go);
                }
                break;
            } else {
                cp->next_on_contour->trim_prev(0.);
                take_ccw_full(pl1, contour, cp->point_idx, cp->next_on_contour->point_idx);
                length_to_go -= l;
            }
        }
    }

    if (add_at_start) {
        pl1.reverse();
        append(pl1.points, pl_tmp);
    }
}

// Return an index of start of a segment and a point of the clipping point at distance from the end of polyline.
struct SegmentPoint {
	// Segment index, defining a line <idx_segment, idx_segment + 1).
	size_t idx_segment = std::numeric_limits<size_t>::max();
	// Parameter of point in <0, 1) along the line <idx_segment, idx_segment + 1)
	double t;
	Vec2d  point;

	bool valid() const { return idx_segment != std::numeric_limits<size_t>::max(); }
};

static inline SegmentPoint clip_start_segment_and_point(const Points &polyline, double distance)
{
	assert(polyline.size() >= 2);
	assert(distance > 0.);
	// Initialized to "invalid".
	SegmentPoint out;
	if (polyline.size() >= 2) {
	    Vec2d pt_prev = polyline.front().cast<double>();
        for (size_t i = 1; i < polyline.size(); ++ i) {
			Vec2d pt = polyline[i].cast<double>();
			Vec2d v = pt - pt_prev;
	        double l = v.norm();
	        if (l > distance) {
	        	out.idx_segment = i - 1;
	        	out.t 			= distance / l;
	        	out.point 		= pt_prev + out.t * v;
	            break;
	        }
	        distance -= l;
	        pt_prev = pt;
	    }
	}
	return out;
}

static inline SegmentPoint clip_end_segment_and_point(const Points &polyline, double distance)
{
	assert(polyline.size() >= 2);
	assert(distance > 0.);
	// Initialized to "invalid".
	SegmentPoint out;
	if (polyline.size() >= 2) {
	    Vec2d pt_next = polyline.back().cast<double>();
		for (int i = int(polyline.size()) - 2; i >= 0; -- i) {
			Vec2d pt = polyline[i].cast<double>();
			Vec2d v = pt - pt_next;
	        double l = v.norm();
	        if (l > distance) {
	        	out.idx_segment = i;
	        	out.t 			= distance / l;
	        	out.point 		= pt_next + out.t * v;
				// Store the parameter referenced to the starting point of a segment.
				out.t			= 1. - out.t;
	            break;
	        }
	        distance -= l;
	        pt_next = pt;
	    }
	}
	return out;
}

// Calculate intersection of a line with a thick segment.
// Returns Eucledian parameters of the line / thick segment overlap.
static inline bool line_rounded_thick_segment_collision(
    const Vec2d &line_a,    const Vec2d &line_b, 
    const Vec2d &segment_a, const Vec2d &segment_b, const double offset, 
    std::pair<double, double> &out_interval)
{
    const Vec2d  line_v0   = line_b - line_a;
    double       lv        = line_v0.squaredNorm();

    const Vec2d  segment_v = segment_b - segment_a;
    const double segment_l = segment_v.norm();
    const double offset2   = offset * offset;

    bool intersects = false;
    if (lv < SCALED_EPSILON * SCALED_EPSILON)
    {
        // Very short line vector. Just test whether the center point is inside the offset line.
        Vec2d lpt = 0.5 * (line_a + line_b);
        if (segment_l > SCALED_EPSILON) {
            intersects = line_alg::distance_to_squared(Linef{ segment_a, segment_b }, lpt) < offset2;
        } else
            intersects = (0.5 * (segment_a + segment_b) - lpt).squaredNorm() < offset2;
        if (intersects) {
            out_interval.first = 0.;
            out_interval.second = sqrt(lv);
        }
    }
    else
    {
        // Output interval.
        double tmin = std::numeric_limits<double>::max();
        double tmax = -tmin;
        auto extend_interval = [&tmin, &tmax](double atmin, double atmax) {
            tmin = std::min(tmin, atmin);
            tmax = std::max(tmax, atmax);
        };

        // Intersections with the inflated segment end points.
        auto ray_circle_intersection_interval_extend = [&extend_interval](const Vec2d &segment_pt, const double offset2, const Vec2d &line_pt, const Vec2d &line_vec) {
            std::pair<Vec2d, Vec2d> pts;
            Vec2d  p0 = line_pt - segment_pt;
            double lv2 = line_vec.squaredNorm();
            if (Geometry::ray_circle_intersections_r2_lv2_c(offset2, line_vec.y(), - line_vec.x(), lv2, - line_vec.y() * p0.x() + line_vec.x() * p0.y(), pts)) {
                double tmin = (pts.first  - p0).dot(line_vec) / lv2;
                double tmax = (pts.second - p0).dot(line_vec) / lv2;
                if (tmin > tmax)
                    std::swap(tmin, tmax);
                tmin = std::max(tmin, 0.);
                tmax = std::min(tmax, 1.);
                if (tmin <= tmax)
                    extend_interval(tmin, tmax);
            }
        };

        // Intersections with the inflated segment.
        if (segment_l > SCALED_EPSILON) {
            ray_circle_intersection_interval_extend(segment_a, offset2, line_a, line_v0);
            ray_circle_intersection_interval_extend(segment_b, offset2, line_a, line_v0);
            // Clip the line segment transformed into a coordinate space of the segment,
            // where the segment spans (0, 0) to (segment_l, 0).
            const Vec2d dir_x = segment_v / segment_l;
            const Vec2d dir_y(- dir_x.y(), dir_x.x());
            const Vec2d line_p0(line_a - segment_a);
            std::pair<double, double> interval;
            if (Geometry::liang_barsky_line_clipping_interval(
                    Vec2d(line_p0.dot(dir_x), line_p0.dot(dir_y)),
                    Vec2d(line_v0.dot(dir_x), line_v0.dot(dir_y)), 
                    BoundingBoxf(Vec2d(0., - offset), Vec2d(segment_l, offset)), 
                    interval))
                extend_interval(interval.first, interval.second);
        } else
            ray_circle_intersection_interval_extend(0.5 * (segment_a + segment_b), offset, line_a, line_v0);

        intersects = tmin <= tmax;
        if (intersects) {
            lv = sqrt(lv);
            out_interval.first  = tmin * lv;
            out_interval.second = tmax * lv;
        }
    }

#if 0
    {
        BoundingBox bbox;
        bbox.merge(line_a.cast<coord_t>());
        bbox.merge(line_a.cast<coord_t>());
        bbox.merge(segment_a.cast<coord_t>());
        bbox.merge(segment_b.cast<coord_t>());
        static int iRun = 0;
        ::Slic3r::SVG svg(debug_out_path("%s-%03d.svg", "line-thick-segment-intersect", iRun ++), bbox);
        svg.draw(Line(line_a.cast<coord_t>(), line_b.cast<coord_t>()), "black");
        svg.draw(Line(segment_a.cast<coord_t>(), segment_b.cast<coord_t>()), "blue", offset * 2.);
        svg.draw(segment_a.cast<coord_t>(), "blue", offset);
        svg.draw(segment_b.cast<coord_t>(), "blue", offset);
        svg.draw(Line(segment_a.cast<coord_t>(), segment_b.cast<coord_t>()), "black");
        if (intersects)
            svg.draw(Line((line_a + (line_b - line_a).normalized() * out_interval.first).cast<coord_t>(),
                          (line_a + (line_b - line_a).normalized() * out_interval.second).cast<coord_t>()), "red");
    }
#endif

    return intersects;
}

#ifndef NDEBUG
static inline bool inside_interval(double low, double high, double p)
{
    return p >= low && p <= high;
}

static inline bool interval_inside_interval(double outer_low, double outer_high, double inner_low, double inner_high, double epsilon)
{
    outer_low -= epsilon;
    outer_high += epsilon;
    return inside_interval(outer_low, outer_high, inner_low) && inside_interval(outer_low, outer_high, inner_high);
}

static inline bool cyclic_interval_inside_interval(double outer_low, double outer_high, double inner_low, double inner_high, double length)
{
    if (outer_low > outer_high)
        outer_high += length;
    if (inner_low > inner_high)
        inner_high += length;
    else if (inner_high < outer_low) {
        inner_low += length;
        inner_high += length;
    }
    return interval_inside_interval(outer_low, outer_high, inner_low, inner_high, double(SCALED_EPSILON));
}
#endif // NDEBUG

#ifdef INFILL_DEBUG_OUTPUT
static void export_infill_to_svg(
    // Boundary contour, along which the perimeter extrusions will be drawn.
    const std::vector<Points>                              &boundary,
    // Parametrization of boundary with Euclidian length.
    const std::vector<std::vector<double>>                 &boundary_parameters,
    // Intersections (T-joints) of the infill lines with the boundary.
    std::vector<std::vector<ContourIntersectionPoint*>>    &boundary_intersections,
    // Infill lines, either completely inside the boundary, or touching the boundary.
    const Polylines                                        &infill,
    const coord_t                                           scaled_spacing,
    const std::string                                      &path,
    const Polylines                                        &overlap_lines = Polylines(),
    const Polylines                                        &polylines = Polylines(),
    const Points                                           &pts = Points())
{
    Polygons    polygons;
    std::transform(boundary.begin(), boundary.end(), std::back_inserter(polygons), [](auto &pts) { return Polygon(pts); });
    ExPolygons  expolygons = union_ex(polygons);
    BoundingBox bbox = get_extents(polygons);
    bbox.offset(scale_(3.));

    ::Slic3r::SVG svg(path, bbox);
    // Draw the filled infill polygons.
    svg.draw(expolygons);

    // Draw the pieces of boundary allowed to be used as anchors of infill lines, not yet consumed.
    const std::string color_boundary_trimmed     = "blue";
    const std::string color_boundary_not_trimmed = "yellow";
    const coordf_t    boundary_line_width        = scaled_spacing;
    svg.draw_outline(polygons, "red", boundary_line_width);
    for (const std::vector<ContourIntersectionPoint*> &intersections : boundary_intersections) {
        const size_t                 boundary_idx  = &intersections - boundary_intersections.data();
        const Points                &contour       = boundary[boundary_idx];
        const std::vector<double>   &contour_param = boundary_parameters[boundary_idx];
        for (const ContourIntersectionPoint *ip : intersections) {
            assert(ip->next_trimmed == ip->next_on_contour->prev_trimmed);
            assert(ip->prev_trimmed == ip->prev_on_contour->next_trimmed);
            {
                Polyline pl { contour[ip->point_idx] };
                if (ip->next_trimmed) {
                    if (ip->contour_not_taken_length_next > SCALED_EPSILON) {
                        take_ccw_limited(pl, contour, contour_param, ip->point_idx, ip->next_on_contour->point_idx, ip->contour_not_taken_length_next);
                        svg.draw(pl, color_boundary_trimmed, boundary_line_width);
                    }
                } else {
                    take_ccw_full(pl, contour, ip->point_idx, ip->next_on_contour->point_idx);
                    svg.draw(pl, color_boundary_not_trimmed, boundary_line_width);
                }
            }
            {
                Polyline pl { contour[ip->point_idx] };
                if (ip->prev_trimmed) {
                    if (ip->contour_not_taken_length_prev > SCALED_EPSILON) {
                        take_cw_limited(pl, contour, contour_param, ip->point_idx, ip->prev_on_contour->point_idx, ip->contour_not_taken_length_prev);
                        svg.draw(pl, color_boundary_trimmed, boundary_line_width);
                    }
                } else {
                    take_cw_full(pl, contour, ip->point_idx, ip->prev_on_contour->point_idx);
                    svg.draw(pl, color_boundary_not_trimmed, boundary_line_width);
                }
            }
        }
    }

    // Draw the full infill polygon boundary.
    svg.draw_outline(polygons, "green");

    // Draw the infill lines, first the full length with red color, then a slightly shortened length with black color.
    svg.draw(infill, "brown");
    static constexpr double trim_length = scale_(0.15);
    for (Polyline polyline : infill)
        if (! polyline.empty()) {
            Vec2d a = polyline.points.front().cast<double>();
            Vec2d d = polyline.points.back().cast<double>();
            if (polyline.size() == 2) {
                Vec2d v = d - a;
                double l = v.norm();
                if (l > 2. * trim_length) {
                    a += v * trim_length / l;
                    d -= v * trim_length / l;
                    polyline.points.front() = a.cast<coord_t>();
                    polyline.points.back() = d.cast<coord_t>();
                } else
                    polyline.points.clear();
            } else if (polyline.size() > 2) {
                Vec2d b = polyline.points[1].cast<double>();
                Vec2d c = polyline.points[polyline.points.size() - 2].cast<double>();
                Vec2d v = b - a;
                double l = v.norm();
                if (l > trim_length) {
                    a += v * trim_length / l;
                    polyline.points.front() = a.cast<coord_t>();
                } else
                    polyline.points.erase(polyline.points.begin());
                v = d - c;
                l = v.norm();
                if (l > trim_length)
                    polyline.points.back() = (d - v * trim_length / l).cast<coord_t>();
                else
                    polyline.points.pop_back();
            }
            svg.draw(polyline, "black");
        }

    svg.draw(overlap_lines, "red", scale_(0.05));
    svg.draw(polylines, "magenta", scale_(0.05));
    svg.draw(pts, "magenta");
}
#endif // INFILL_DEBUG_OUTPUT

#ifndef NDEBUG
bool validate_boundary_intersections(const std::vector<std::vector<ContourIntersectionPoint*>> &boundary_intersections)
{
    for (const std::vector<ContourIntersectionPoint*>& contour : boundary_intersections) {
        for (ContourIntersectionPoint* ip : contour) {
            assert(ip->next_trimmed == ip->next_on_contour->prev_trimmed);
            assert(ip->prev_trimmed == ip->prev_on_contour->next_trimmed);
        }
    }
    return true;
}
#endif // NDEBUG

// Mark the segments of split boundary as consumed if they are very close to some of the infill line.
void mark_boundary_segments_touching_infill(
    // Boundary contour, along which the perimeter extrusions will be drawn.
	const std::vector<Points>                              &boundary,
    // Parametrization of boundary with Euclidian length.
	const std::vector<std::vector<double>>                 &boundary_parameters,
    // Intersections (T-joints) of the infill lines with the boundary.
    std::vector<std::vector<ContourIntersectionPoint*>>    &boundary_intersections,
    // Bounding box around the boundary.
	const BoundingBox 		                               &boundary_bbox,
    // Infill lines, either completely inside the boundary, or touching the boundary.
	const Polylines 		                               &infill,
    // How much of the infill ends should be ignored when marking the boundary segments?
	const double			                                clip_distance,
    // Roughly width of the infill line.
	const double 				                            distance_colliding)
{
    assert(boundary.size() == boundary_parameters.size());
#ifndef NDEBUG
    for (size_t i = 0; i < boundary.size(); ++ i)
        assert(boundary[i].size() + 1 == boundary_parameters[i].size());
    assert(validate_boundary_intersections(boundary_intersections));
#endif

#ifdef INFILL_DEBUG_OUTPUT
    static int iRun = 0;
    ++ iRun;
    int iStep = 0;
    export_infill_to_svg(boundary, boundary_parameters, boundary_intersections, infill, distance_colliding * 2, debug_out_path("%s-%03d.svg", "FillBase-mark_boundary_segments_touching_infill-start", iRun));
    Polylines perimeter_overlaps;
#endif // INFILL_DEBUG_OUTPUT

	EdgeGrid::Grid grid;
    // Make sure that the the grid is big enough for queries against the thick segment.
	grid.set_bbox(boundary_bbox.inflated(distance_colliding * 1.43));
	// Inflate the bounding box by a thick line width.
	grid.create(boundary, coord_t(std::max(clip_distance, distance_colliding) + scale_(10.)));

    // Visitor for the EdgeGrid to trim boundary_intersections with existing infill lines.
	struct Visitor {
		Visitor(const EdgeGrid::Grid &grid,
                const std::vector<Points> &boundary, const std::vector<std::vector<double>> &boundary_parameters, std::vector<std::vector<ContourIntersectionPoint*>> &boundary_intersections,
                const double radius) :
			grid(grid), boundary(boundary), boundary_parameters(boundary_parameters), boundary_intersections(boundary_intersections), radius(radius), trim_l_threshold(0.5 * radius) {}

        // Init with a segment of an infill line.
		void init(const Vec2d &infill_pt1, const Vec2d &infill_pt2) {
			this->infill_pt1 = &infill_pt1;
			this->infill_pt2 = &infill_pt2;
            this->infill_bbox.reset();
            this->infill_bbox.merge(infill_pt1);
            this->infill_bbox.merge(infill_pt2);
            this->infill_bbox.offset(this->radius + SCALED_EPSILON);
        }

		bool operator()(coord_t iy, coord_t ix) {
			// Called with a row and colum of the grid cell, which is intersected by a line.
			auto cell_data_range = this->grid.cell_data_range(iy, ix);
			for (auto it_contour_and_segment = cell_data_range.first; it_contour_and_segment != cell_data_range.second; ++ it_contour_and_segment) {
				// End points of the line segment and their vector.
				auto segment = this->grid.segment(*it_contour_and_segment);
                std::vector<ContourIntersectionPoint*> &intersections = boundary_intersections[it_contour_and_segment->first];
                if (intersections.empty())
                    // There is no infil line touching this contour, thus effort will be saved to calculate overlap with other infill lines.
                    continue;
				const Vec2d seg_pt1 = segment.first.cast<double>();
				const Vec2d seg_pt2 = segment.second.cast<double>();
                std::pair<double, double> interval;
                BoundingBoxf bbox_seg;
                bbox_seg.merge(seg_pt1);
                bbox_seg.merge(seg_pt2);
#ifdef INFILL_DEBUG_OUTPUT
                //if (this->infill_bbox.overlap(bbox_seg)) this->perimeter_overlaps.push_back({ segment.first, segment.second });
#endif // INFILL_DEBUG_OUTPUT
                if (this->infill_bbox.overlap(bbox_seg) && line_rounded_thick_segment_collision(seg_pt1, seg_pt2, *this->infill_pt1, *this->infill_pt2, this->radius, interval)) {
                    // The boundary segment intersects with the infill segment thickened by radius.
                    // Interval is specified in Euclidian length from seg_pt1 to seg_pt2.
                    // 1) Find the Euclidian parameters of seg_pt1 and seg_pt2 on its boundary contour.
                    const std::vector<double> &contour_parameters = boundary_parameters[it_contour_and_segment->first];
                    const double contour_length = contour_parameters.back();
					const double param_seg_pt1  = contour_parameters[it_contour_and_segment->second];
                    const double param_seg_pt2  = contour_parameters[it_contour_and_segment->second + 1];
#ifdef INFILL_DEBUG_OUTPUT
                    this->perimeter_overlaps.push_back({ Point((seg_pt1 + (seg_pt2 - seg_pt1).normalized() * interval.first).cast<coord_t>()),
                                                         Point((seg_pt1 + (seg_pt2 - seg_pt1).normalized() * interval.second).cast<coord_t>()) });
#endif // INFILL_DEBUG_OUTPUT
                    assert(interval.first >= 0.);
                    assert(interval.second >= 0.);
                    assert(interval.first <= interval.second);
                    const auto param_overlap1 = std::min(param_seg_pt2, param_seg_pt1 + interval.first);
                    const auto param_overlap2 = std::min(param_seg_pt2, param_seg_pt1 + interval.second);
                    // 2) Find the ContourIntersectionPoints before param_overlap1 and after param_overlap2.
                    // Find the span of ContourIntersectionPoints, that is trimmed by the interval (param_overlap1, param_overlap2).
                    ContourIntersectionPoint *ip_low, *ip_high;
                    if (intersections.size() == 1) {
                        // Only a single infill line touches this contour.
                        ip_low = ip_high = intersections.front();
                    } else {
                        assert(intersections.size() > 1);
                        auto it_low  = Slic3r::lower_bound_by_predicate(intersections.begin(), intersections.end(), [param_overlap1](const ContourIntersectionPoint *l) { return l->param < param_overlap1; });
                        auto it_high = Slic3r::lower_bound_by_predicate(intersections.begin(), intersections.end(), [param_overlap2](const ContourIntersectionPoint *l) { return l->param < param_overlap2; });
                        ip_low  = it_low  == intersections.end() ? intersections.front() : *it_low;
                        ip_high = it_high == intersections.end() ? intersections.front() : *it_high;
                        if (ip_low->param != param_overlap1)
                            ip_low = ip_low->prev_on_contour;
                        assert(ip_low != ip_high);
                        // Verify that the interval (param_overlap1, param_overlap2) is inside the interval (ip_low->param, ip_high->param).
                        assert(cyclic_interval_inside_interval(ip_low->param, ip_high->param, param_overlap1, param_overlap2, contour_length));
                    }
                    assert(validate_boundary_intersections(boundary_intersections));
                    // Mark all ContourIntersectionPoints between ip_low and ip_high as consumed.
                    if (ip_low->next_on_contour != ip_high)
                        for (ContourIntersectionPoint *ip = ip_low->next_on_contour; ip != ip_high; ip = ip->next_on_contour) {
                            ip->consume_prev();
                            ip->consume_next();
                        }
                    // Subtract the interval from the first and last segments.
                    double trim_l = closed_contour_distance_ccw(ip_low->param, param_overlap1, contour_length);
                    //if (trim_l > trim_l_threshold)
                        ip_low->trim_next(trim_l);
                    trim_l = closed_contour_distance_ccw(param_overlap2, ip_high->param, contour_length);
                    //if (trim_l > trim_l_threshold)
                        ip_high->trim_prev(trim_l);
                    assert(ip_low->next_trimmed == ip_high->prev_trimmed);
                    assert(validate_boundary_intersections(boundary_intersections));
                    //FIXME mark point as consumed?
                    //FIXME verify the sequence between prev and next?
#ifdef INFILL_DEBUG_OUTPUT
					{
#if 0
                        static size_t iRun = 0;
						ExPolygon expoly(Polygon(*grid.contours().front()));
						for (size_t i = 1; i < grid.contours().size(); ++i)
							expoly.holes.emplace_back(Polygon(*grid.contours()[i]));
						SVG svg(debug_out_path("%s-%d.svg", "FillBase-mark_boundary_segments_touching_infill", iRun ++).c_str(), get_extents(expoly));
						svg.draw(expoly, "green");
						svg.draw(Line(segment.first, segment.second), "red");
						svg.draw(Line(this->infill_pt1->cast<coord_t>(), this->infill_pt2->cast<coord_t>()), "magenta");
#endif
                    }
#endif // INFILL_DEBUG_OUTPUT
				}
			}
			// Continue traversing the grid along the edge.
			return true;
		}

		const EdgeGrid::Grid 			   			        &grid;
		const std::vector<Points> 					        &boundary;
        const std::vector<std::vector<double>>              &boundary_parameters;
        std::vector<std::vector<ContourIntersectionPoint*>> &boundary_intersections;
		// Maximum distance between the boundary and the infill line allowed to consider the boundary not touching the infill line.
		const double								         radius;
        // Region around the contour / infill line intersection point, where the intersections are ignored.
        const double                                         trim_l_threshold;

		const Vec2d 								        *infill_pt1;
		const Vec2d 								        *infill_pt2;
        BoundingBoxf                                         infill_bbox;

#ifdef INFILL_DEBUG_OUTPUT
        Polylines                                            perimeter_overlaps;
#endif // INFILL_DEBUG_OUTPUT
	} visitor(grid, boundary, boundary_parameters, boundary_intersections, distance_colliding);

	for (const Polyline &polyline : infill) {
#ifdef INFILL_DEBUG_OUTPUT
        ++ iStep;
#endif // INFILL_DEBUG_OUTPUT
		// Clip the infill polyline by the Eucledian distance along the polyline.
		SegmentPoint start_point = clip_start_segment_and_point(polyline.points, clip_distance);
		SegmentPoint end_point   = clip_end_segment_and_point(polyline.points, clip_distance);
		if (start_point.valid() && end_point.valid() && 
			(start_point.idx_segment < end_point.idx_segment || (start_point.idx_segment == end_point.idx_segment && start_point.t < end_point.t))) {
			// The clipped polyline is non-empty.
#ifdef INFILL_DEBUG_OUTPUT
            visitor.perimeter_overlaps.clear();
#endif // INFILL_DEBUG_OUTPUT
			for (size_t point_idx = start_point.idx_segment; point_idx <= end_point.idx_segment; ++ point_idx) {
//FIXME extend the EdgeGrid to suport tracing a thick line.
#if 0
				Point pt1, pt2;
				Vec2d pt1d, pt2d;
				if (point_idx == start_point.idx_segment) {
					pt1d = start_point.point;
					pt1  = pt1d.cast<coord_t>();
				} else {
					pt1  = polyline.points[point_idx];
					pt1d = pt1.cast<double>();
				}
				if (point_idx == start_point.idx_segment) {
					pt2d = end_point.point;
					pt2  = pt1d.cast<coord_t>();
				} else {
					pt2  = polyline.points[point_idx];
					pt2d = pt2.cast<double>();
				}
				visitor.init(pt1d, pt2d);
				grid.visit_cells_intersecting_thick_line(pt1, pt2, distance_colliding, visitor);
#else
				Vec2d pt1 = (point_idx == start_point.idx_segment) ? start_point.point : polyline.points[point_idx    ].cast<double>();
				Vec2d pt2 = (point_idx == end_point  .idx_segment) ? end_point  .point : polyline.points[point_idx + 1].cast<double>();
#if 0
					{
						static size_t iRun = 0;
						ExPolygon expoly(Polygon(*grid.contours().front()));
						for (size_t i = 1; i < grid.contours().size(); ++i)
							expoly.holes.emplace_back(Polygon(*grid.contours()[i]));
						SVG svg(debug_out_path("%s-%d.svg", "FillBase-mark_boundary_segments_touching_infill0", iRun ++).c_str(), get_extents(expoly));
						svg.draw(expoly, "green");
						svg.draw(polyline, "blue");
						svg.draw(Line(pt1.cast<coord_t>(), pt2.cast<coord_t>()), "magenta", scale_(0.1));
					}
#endif
				visitor.init(pt1, pt2);
				// Simulate tracing of a thick line. This only works reliably if distance_colliding <= grid cell size.
				Vec2d v = (pt2 - pt1).normalized() * distance_colliding;
				Vec2d vperp = perp(v);
				Vec2d a = pt1 - v - vperp;
				Vec2d b = pt2 + v - vperp;
                assert(grid.bbox().contains(a.cast<coord_t>()));
                assert(grid.bbox().contains(b.cast<coord_t>()));
				grid.visit_cells_intersecting_line(a.cast<coord_t>(), b.cast<coord_t>(), visitor);
				a = pt1 - v + vperp;
				b = pt2 + v + vperp;
                assert(grid.bbox().contains(a.cast<coord_t>()));
                assert(grid.bbox().contains(b.cast<coord_t>()));
                grid.visit_cells_intersecting_line(a.cast<coord_t>(), b.cast<coord_t>(), visitor);
#endif
#ifdef INFILL_DEBUG_OUTPUT
//                export_infill_to_svg(boundary, boundary_parameters, boundary_intersections, infill, distance_colliding * 2, debug_out_path("%s-%03d-%03d-%03d.svg", "FillBase-mark_boundary_segments_touching_infill-step", iRun, iStep, int(point_idx)), { polyline });
#endif // INFILL_DEBUG_OUTPUT
			}
#ifdef INFILL_DEBUG_OUTPUT
            Polylines perimeter_overlaps;
            export_infill_to_svg(boundary, boundary_parameters, boundary_intersections, infill, distance_colliding * 2, debug_out_path("%s-%03d-%03d.svg", "FillBase-mark_boundary_segments_touching_infill-step", iRun, iStep), visitor.perimeter_overlaps, { polyline });
            append(perimeter_overlaps, std::move(visitor.perimeter_overlaps));
            perimeter_overlaps.clear();
#endif // INFILL_DEBUG_OUTPUT
        }
	}

#ifdef INFILL_DEBUG_OUTPUT
    export_infill_to_svg(boundary, boundary_parameters, boundary_intersections, infill, distance_colliding * 2, debug_out_path("%s-%03d.svg", "FillBase-mark_boundary_segments_touching_infill-end", iRun), perimeter_overlaps);
#endif // INFILL_DEBUG_OUTPUT
    assert(validate_boundary_intersections(boundary_intersections));
}

void Fill::connect_infill(Polylines &&infill_ordered, const ExPolygon &boundary_src, Polylines &polylines_out, const double spacing, const FillParams &params)
{
	assert(! boundary_src.contour.points.empty());
    auto polygons_src = reserve_vector<const Polygon*>(boundary_src.holes.size() + 1);
    polygons_src.emplace_back(&boundary_src.contour);
    for (const Polygon &polygon : boundary_src.holes)
        polygons_src.emplace_back(&polygon);

    connect_infill(std::move(infill_ordered), polygons_src, get_extents(boundary_src.contour), polylines_out, spacing, params);
}

void Fill::connect_infill(Polylines &&infill_ordered, const Polygons &boundary_src, const BoundingBox &bbox, Polylines &polylines_out, const double spacing, const FillParams &params)
{
    auto polygons_src = reserve_vector<const Polygon*>(boundary_src.size());
    for (const Polygon &polygon : boundary_src)
        polygons_src.emplace_back(&polygon);

    connect_infill(std::move(infill_ordered), polygons_src, bbox, polylines_out, spacing, params);
}

static constexpr auto boundary_idx_unconnected = std::numeric_limits<size_t>::max();

struct BoundaryInfillGraph
{
    std::vector<Points>                     boundary;
    std::vector<std::vector<double>>        boundary_params;
    std::vector<ContourIntersectionPoint>   map_infill_end_point_to_boundary;

    const Point&    point(const ContourIntersectionPoint &cp) const {
        assert(cp.contour_idx != size_t(-1));
        assert(cp.point_idx != size_t(-1));
        return this->boundary[cp.contour_idx][cp.point_idx];
    }

    const Point&    infill_end_point(size_t infill_end_point_idx) const {
        return this->point(this->map_infill_end_point_to_boundary[infill_end_point_idx]);
    }

    const Point     interpolate_contour_point(const ContourIntersectionPoint &cp, double param) {
        const Points                &contour        = this->boundary[cp.contour_idx];
        const std::vector<double>   &contour_params = this->boundary_params[cp.contour_idx];
        // Find the start of a contour segment with param.
        auto it = std::lower_bound(contour_params.begin(), contour_params.end(), param);
        if (*it != param) {
            assert(it != contour_params.begin());
            -- it;
        }
        size_t i = it - contour_params.begin();
        if (i == contour.size())
            i = 0;
        double t1 = contour_params[i];
        double t2 = next_value_modulo(i, contour_params);
        return lerp(contour[i], next_value_modulo(i, contour), (param - t1) / (t2 - t1));
    }

    enum Direction {
        Left,
        Right,
        Up,
        Down,
        Taken,
    };

    static Direction dir(const Point &p1, const Point &p2) {
        return p1.x() == p2.x() ? 
            (p1.y() < p2.y() ? Up : Down) :
            (p1.x() < p2.x() ? Right : Left);
    }

    const Direction dir_prev(const ContourIntersectionPoint &cp) const {
        assert(cp.prev_on_contour);
        return cp.could_take_prev() ? 
            dir(this->point(cp), this->point(*cp.prev_on_contour)) :
            Taken;
    }

    const Direction dir_next(const ContourIntersectionPoint &cp) const {
        assert(cp.next_on_contour);
        return cp.could_take_next() ? 
            dir(this->point(cp), this->point(*cp.next_on_contour)) :
            Taken;
    }

    bool            first(const ContourIntersectionPoint &cp) const {
        return ((&cp - this->map_infill_end_point_to_boundary.data()) & 1) == 0;
    }

    const ContourIntersectionPoint& other(const ContourIntersectionPoint &cp) const {
        return this->map_infill_end_point_to_boundary[((&cp - this->map_infill_end_point_to_boundary.data()) ^ 1)];
    }

    ContourIntersectionPoint& other(const ContourIntersectionPoint &cp) {
        return this->map_infill_end_point_to_boundary[((&cp - this->map_infill_end_point_to_boundary.data()) ^ 1)];
    }

    bool            prev_vertical(const ContourIntersectionPoint &cp) const {
        return this->point(cp).x() == this->point(*cp.prev_on_contour).x();
    }

    bool            next_vertical(const ContourIntersectionPoint &cp) const {
        return this->point(cp).x() == this->point(*cp.next_on_contour).x();
    }

};


// After mark_boundary_segments_touching_infill() marks boundary segments overlapping trimmed infill lines,
// there are possibly some very short boundary segments unmarked, but overlapping the untrimmed infill lines fully
// Mark those short boundary segments.
static inline void mark_boundary_segments_overlapping_infill(
    BoundaryInfillGraph                                    &graph,
    // Infill lines, either completely inside the boundary, or touching the boundary.
    const Polylines                                        &infill,
    // Spacing (width) of the infill lines.
    const double                                            spacing)
{
    for (ContourIntersectionPoint &cp : graph.map_infill_end_point_to_boundary) {
        const Points                &contour         = graph.boundary[cp.contour_idx];
        const std::vector<double>   &contour_params  = graph.boundary_params[cp.contour_idx];
        const Polyline              &infill_polyline = infill[(&cp - graph.map_infill_end_point_to_boundary.data()) / 2];
        const double                 radius          = 0.5 * (spacing + SCALED_EPSILON);
        assert(infill_polyline.size() == 2);
        const Linef                  infill_line { infill_polyline.points.front().cast<double>(), infill_polyline.points.back().cast<double>() };
        if (cp.could_take_next()) {
            bool inside = true;
            for (size_t i = cp.point_idx; i != cp.next_on_contour->point_idx; ) {
                size_t j = next_idx_modulo(i, contour);
                const Vec2d seg_pt2 = contour[j].cast<double>();
                if (line_alg::distance_to_squared(infill_line, seg_pt2) < radius * radius) {
                    // The segment is completely inside.
                } else {
                    std::pair<double, double> interval;
                    line_rounded_thick_segment_collision(contour[i].cast<double>(), seg_pt2, infill_line.a, infill_line.b, radius, interval);
                    assert(interval.first == 0.);
                    double len_out = closed_contour_distance_ccw(contour_params[cp.point_idx], contour_params[i], contour_params.back()) + interval.second;
                    if (len_out < cp.contour_not_taken_length_next) {
                        // Leaving the infill line region before exiting cp.contour_not_taken_length_next, 
                        // thus at least some of the contour is outside and we will extrude this segment.
                        inside = false;
                        break;
                    }
                }
                if (closed_contour_distance_ccw(contour_params[cp.point_idx], contour_params[j], contour_params.back()) >= cp.contour_not_taken_length_next)
                    break;
                i = j;
            }
            if (inside) {
                if (! cp.next_trimmed)
                    // The arc from cp to cp.next_on_contour was not trimmed yet, however it is completely overlapping the infill line.
                    cp.next_on_contour->trim_prev(0);
                cp.trim_next(0);
            }
        } else
            cp.trim_next(0);
        if (cp.could_take_prev()) {
            bool inside = true;
            for (size_t i = cp.point_idx; i != cp.prev_on_contour->point_idx; ) {
                size_t j = prev_idx_modulo(i, contour);
                const Vec2d seg_pt2 = contour[j].cast<double>();
                // Distance of the second segment line from the infill line.
                if (line_alg::distance_to_squared(infill_line, seg_pt2) < radius * radius) {
                    // The segment is completely inside.
                } else {
                    std::pair<double, double> interval;
                    line_rounded_thick_segment_collision(contour[i].cast<double>(), seg_pt2, infill_line.a, infill_line.b, radius, interval);
                    assert(interval.first == 0.);
                    double len_out = closed_contour_distance_cw(contour_params[cp.point_idx], contour_params[i], contour_params.back()) + interval.second;
                    if (len_out < cp.contour_not_taken_length_prev) {
                        // Leaving the infill line region before exiting cp.contour_not_taken_length_next, 
                        // thus at least some of the contour is outside and we will extrude this segment.
                        inside = false;
                        break;
                    }
                }
                if (closed_contour_distance_cw(contour_params[cp.point_idx], contour_params[j], contour_params.back()) >= cp.contour_not_taken_length_prev)
                    break;
                i = j;
            }
            if (inside) {
                if (! cp.prev_trimmed)
                    // The arc from cp to cp.prev_on_contour was not trimmed yet, however it is completely overlapping the infill line.
                    cp.prev_on_contour->trim_next(0);
                cp.trim_prev(0);
            }
        } else
            cp.trim_prev(0);
    }
}

BoundaryInfillGraph create_boundary_infill_graph(const Polylines &infill_ordered, const std::vector<const Polygon*> &boundary_src, const BoundingBox &bbox, const double spacing)
{
    BoundaryInfillGraph out;
    out.boundary.assign(boundary_src.size(), Points());
    out.boundary_params.assign(boundary_src.size(), std::vector<double>());
    out.map_infill_end_point_to_boundary.assign(infill_ordered.size() * 2, ContourIntersectionPoint{ boundary_idx_unconnected, boundary_idx_unconnected });
    {
        // Project the infill_ordered end points onto boundary_src.
        std::vector<std::pair<EdgeGrid::Grid::ClosestPointResult, size_t>> intersection_points;
        {
            EdgeGrid::Grid grid;
            grid.set_bbox(bbox.inflated(SCALED_EPSILON));
            grid.create(boundary_src, coord_t(scale_(10.)));
            intersection_points.reserve(infill_ordered.size() * 2);
            for (const Polyline &pl : infill_ordered)
                for (const Point *pt : { &pl.points.front(), &pl.points.back() }) {
                    EdgeGrid::Grid::ClosestPointResult cp = grid.closest_point_signed_distance(*pt, coord_t(SCALED_EPSILON));
                    if (cp.valid()) {
                        // The infill end point shall lie on the contour.
                        assert(cp.distance <= 3.);
                        intersection_points.emplace_back(cp, (&pl - infill_ordered.data()) * 2 + (pt == &pl.points.front() ? 0 : 1));
                    }
                }
            std::sort(intersection_points.begin(), intersection_points.end(), [](const std::pair<EdgeGrid::Grid::ClosestPointResult, size_t> &cp1, const std::pair<EdgeGrid::Grid::ClosestPointResult, size_t> &cp2) {
                return   cp1.first.contour_idx < cp2.first.contour_idx ||
                        (cp1.first.contour_idx == cp2.first.contour_idx &&
                            (cp1.first.start_point_idx < cp2.first.start_point_idx ||
                                (cp1.first.start_point_idx == cp2.first.start_point_idx && cp1.first.t < cp2.first.t)));
            });
        }
        auto it = intersection_points.begin();
        auto it_end = intersection_points.end();
        std::vector<std::vector<ContourIntersectionPoint*>> boundary_intersection_points(out.boundary.size(), std::vector<ContourIntersectionPoint*>());
        for (size_t idx_contour = 0; idx_contour < boundary_src.size(); ++ idx_contour) {
            // Copy contour_src to contour_dst while adding intersection points.
            // Map infill end points map_infill_end_point_to_boundary to the newly inserted boundary points of contour_dst.
            // chain the points of map_infill_end_point_to_boundary along their respective contours.
            const Polygon &contour_src = *boundary_src[idx_contour];
            Points        &contour_dst = out.boundary[idx_contour];
            std::vector<ContourIntersectionPoint*> &contour_intersection_points = boundary_intersection_points[idx_contour];
            ContourIntersectionPoint *pfirst = nullptr;
            ContourIntersectionPoint *pprev  = nullptr;
            {
                // Reserve intersection points.
                size_t n_intersection_points = 0;
                for (auto itx = it; itx != it_end && itx->first.contour_idx == idx_contour; ++ itx)
                    ++ n_intersection_points;
                contour_intersection_points.reserve(n_intersection_points);
            }
            for (size_t idx_point = 0; idx_point < contour_src.points.size(); ++ idx_point) {
                const Point &ipt = contour_src.points[idx_point];
                if (contour_dst.empty() || contour_dst.back() != ipt)
                    contour_dst.emplace_back(ipt);
                for (; it != it_end && it->first.contour_idx == idx_contour && it->first.start_point_idx == idx_point; ++ it) {
                    // Add these points to the destination contour.
                    const Polyline  &infill_line = infill_ordered[it->second / 2];
                    const Point     &pt          = (it->second & 1) ? infill_line.points.back() : infill_line.points.front();
//#ifndef NDEBUG
//                    {
//                      const Vec2d pt1 = ipt.cast<double>();
//                      const Vec2d pt2 = (idx_point + 1 == contour_src.size() ? contour_src.points.front() : contour_src.points[idx_point + 1]).cast<double>();
//                      const Vec2d ptx = lerp(pt1, pt2, it->first.t);
//                      assert(std::abs(ptx.x() - pt.x()) < SCALED_EPSILON);
//                      assert(std::abs(ptx.y() - pt.y()) < SCALED_EPSILON);
//                    }
//#endif // NDEBUG
                    size_t idx_tjoint_pt = 0;
                    if (idx_point + 1 < contour_src.size() || pt != contour_dst.front()) {
                        if (pt != contour_dst.back())
                            contour_dst.emplace_back(pt);
                        idx_tjoint_pt = contour_dst.size() - 1;
                    }
                    out.map_infill_end_point_to_boundary[it->second] = ContourIntersectionPoint{ /* it->second, */ idx_contour, idx_tjoint_pt };
                    ContourIntersectionPoint *pthis = &out.map_infill_end_point_to_boundary[it->second];
                    if (pprev) {
                        pprev->next_on_contour = pthis;
                        pthis->prev_on_contour = pprev;                        
                    } else
                        pfirst = pthis;
                    contour_intersection_points.emplace_back(pthis);
                    pprev = pthis;
                }
                if (pfirst) {
                    pprev->next_on_contour = pfirst;
                    pfirst->prev_on_contour = pprev;
                }
            }
            // Parametrize the new boundary with the intersection points inserted.
            std::vector<double> &contour_params = out.boundary_params[idx_contour];
            contour_params.assign(contour_dst.size() + 1, 0.);
            for (size_t i = 1; i < contour_dst.size(); ++i) {
                contour_params[i] = contour_params[i - 1] + (contour_dst[i].cast<double>() - contour_dst[i - 1].cast<double>()).norm();
                assert(contour_params[i] > contour_params[i - 1]);
            }
            contour_params.back() = contour_params[contour_params.size() - 2] + (contour_dst.back().cast<double>() - contour_dst.front().cast<double>()).norm();
            assert(contour_params.back() > contour_params[contour_params.size() - 2]);
            // Map parameters from contour_params to boundary_intersection_points.
            for (ContourIntersectionPoint *ip : contour_intersection_points)
                ip->param = contour_params[ip->point_idx];
            // and measure distance to the previous and next intersection point.
            const double contour_length = contour_params.back();
            for (ContourIntersectionPoint *ip : contour_intersection_points) 
                if (ip->next_on_contour == ip) {
                    assert(ip->prev_on_contour == ip);
                    ip->contour_not_taken_length_prev = ip->contour_not_taken_length_next = contour_length;
                } else {
                    assert(ip->prev_on_contour != ip);
                    ip->contour_not_taken_length_prev = closed_contour_distance_ccw(ip->prev_on_contour->param, ip->param, contour_length);
                    ip->contour_not_taken_length_next = closed_contour_distance_ccw(ip->param, ip->next_on_contour->param, contour_length);
                }
        }

        assert(out.boundary.size() == boundary_src.size());
#if 0
        // Adaptive Cubic Infill produces infill lines, which not always end at the outer boundary.
        assert(std::all_of(out.map_infill_end_point_to_boundary.begin(), out.map_infill_end_point_to_boundary.end(),
            [&out.boundary](const ContourIntersectionPoint &contour_point) {
                return contour_point.contour_idx < out.boundary.size() && contour_point.point_idx < out.boundary[contour_point.contour_idx].size();
            }));
#endif

        // Mark the points and segments of split out.boundary as consumed if they are very close to some of the infill line.
        {
            // @supermerill used 2. * scale_(spacing)
            const double clip_distance      = 1.7 * scale_(spacing);
            // Allow a bit of overlap. This value must be slightly higher than the overlap of FillAdaptive, otherwise
            // the anchors of the adaptive infill will mask the other side of the perimeter line.
            // (see connect_lines_using_hooks() in FillAdaptive.cpp)
            const double distance_colliding = 0.8 * scale_(spacing);
            mark_boundary_segments_touching_infill(out.boundary, out.boundary_params, boundary_intersection_points, bbox, infill_ordered, clip_distance, distance_colliding);
        }
    }

    return out;
}

void Fill::connect_infill(Polylines &&infill_ordered, const std::vector<const Polygon*> &boundary_src, const BoundingBox &bbox, Polylines &polylines_out, const double spacing, const FillParams &params)
{
	assert(! infill_ordered.empty());
    assert(params.anchor_length     >= 0.);
    assert(params.anchor_length_max >= 0.01f);
    assert(params.anchor_length_max >= params.anchor_length);
    const double anchor_length     = scale_(params.anchor_length);
    const double anchor_length_max = scale_(params.anchor_length_max);

#if 0
    append(polylines_out, infill_ordered);
    return;
#endif

    BoundaryInfillGraph graph = create_boundary_infill_graph(infill_ordered, boundary_src, bbox, spacing);

    std::vector<size_t> merged_with(infill_ordered.size());
    std::iota(merged_with.begin(), merged_with.end(), 0);

    auto get_and_update_merged_with = [&merged_with](size_t polyline_idx) -> size_t {
        for (size_t last = polyline_idx;;) {
            size_t lower = merged_with[last];
            assert(lower <= last);
            if (lower == last) {
                merged_with[polyline_idx] = last;
                return last;
            }
            last = lower;
        }
        assert(false);
        return std::numeric_limits<size_t>::max();
    };

    const double line_half_width = 0.5 * scale_(spacing);

#if 0
    // Connection from end of one infill line to the start of another infill line.
    //const double length_max = scale_(spacing);
//  const auto length_max = double(scale_((2. / params.density) * spacing));
    const auto length_max = double(scale_((1000. / params.density) * spacing));
    struct ConnectionCost {
        ConnectionCost(size_t idx_first, double cost, bool reversed) : idx_first(idx_first), cost(cost), reversed(reversed) {}
        size_t  idx_first;
        double  cost;
        bool    reversed;
    };
    std::vector<ConnectionCost> connections_sorted;
    connections_sorted.reserve(infill_ordered.size() * 2 - 2);
    for (size_t idx_chain = 1; idx_chain < infill_ordered.size(); ++ idx_chain) {
        const ContourIntersectionPoint      *cp1            = &graph.map_infill_end_point_to_boundary[(idx_chain - 1) * 2 + 1];
        const ContourIntersectionPoint      *cp2            = &graph.map_infill_end_point_to_boundary[idx_chain * 2];
        if (cp1->contour_idx != boundary_idx_unconnected && cp1->contour_idx == cp2->contour_idx) {
            // End points on the same contour. Try to connect them.
            std::pair<double, double> len = path_lengths_along_contour(cp1, cp2, graph.boundary_params[cp1->contour_idx].back());
            if (len.first < length_max)
                connections_sorted.emplace_back(idx_chain - 1, len.first, false);
            if (len.second < length_max)
                connections_sorted.emplace_back(idx_chain - 1, len.second, true);
        }
    }
    std::sort(connections_sorted.begin(), connections_sorted.end(), [](const ConnectionCost& l, const ConnectionCost& r) { return l.cost < r.cost; });

    for (ConnectionCost &connection_cost : connections_sorted) {
		ContourIntersectionPoint *cp1    = &graph.map_infill_end_point_to_boundary[connection_cost.idx_first * 2 + 1];
		ContourIntersectionPoint *cp2    = &graph.map_infill_end_point_to_boundary[(connection_cost.idx_first + 1) * 2];
        assert(cp1 != cp2);
        assert(cp1->contour_idx == cp2->contour_idx && cp1->contour_idx != boundary_idx_unconnected);
        if (cp1->consumed || cp2->consumed)
            continue;
        const double              length = connection_cost.cost;
        bool                      could_connect;
        {
            // cp1, cp2 sorted CCW.
            ContourIntersectionPoint *cp_low  = connection_cost.reversed ? cp2 : cp1;
            ContourIntersectionPoint *cp_high = connection_cost.reversed ? cp1 : cp2;
            assert(std::abs(length - closed_contour_distance_ccw(cp_low->param, cp_high->param, graph.boundary_params[cp1->contour_idx].back())) < SCALED_EPSILON);
            could_connect = ! cp_low->next_trimmed && ! cp_high->prev_trimmed;
            if (could_connect && cp_low->next_on_contour != cp_high) {
                // Other end of cp1, may or may not be on the same contour as cp1.
                const ContourIntersectionPoint *cp1prev = cp1 - 1;
                // Other end of cp2, may or may not be on the same contour as cp2.
                const ContourIntersectionPoint *cp2next = cp2 + 1;
                for (auto *cp = cp_low->next_on_contour; cp != cp_high; cp = cp->next_on_contour)
                    if (cp->consumed || cp == cp1prev || cp == cp2next || cp->prev_trimmed || cp->next_trimmed) {
                        could_connect = false;
                        break;
                    }
            }
        }
        // Indices of the polylines to be connected by a perimeter segment.
        size_t idx_first  = connection_cost.idx_first;
        size_t idx_second = idx_first + 1;
        idx_first = get_and_update_merged_with(idx_first);
        assert(idx_first < idx_second);
        assert(idx_second == merged_with[idx_second]);
        if (could_connect && length < anchor_length_max) {
            // Take the complete contour.
            // Connect the two polygons using the boundary contour.
            take(infill_ordered[idx_first], infill_ordered[idx_second], graph.boundary[cp1->contour_idx], cp1, cp2, connection_cost.reversed);
            // Mark the second polygon as merged with the first one.
            merged_with[idx_second] = merged_with[idx_first];
            infill_ordered[idx_second].points.clear();
        } else {
            // Try to connect cp1 resp. cp2 with a piece of perimeter line.
            take_limited(infill_ordered[idx_first],  graph.boundary[cp1->contour_idx], graph.boundary_params[cp1->contour_idx], cp1, cp2, connection_cost.reversed, anchor_length, line_half_width);
            take_limited(infill_ordered[idx_second], graph.boundary[cp1->contour_idx], graph.boundary_params[cp1->contour_idx], cp2, cp1, ! connection_cost.reversed, anchor_length, line_half_width);
        }
	}
#endif

    struct Arc {
        ContourIntersectionPoint    *intersection;
        double                       arc_length;
    };
    std::vector<Arc> arches;
    if (!params.dont_sort) {
        arches.reserve(graph.map_infill_end_point_to_boundary.size());
        for (ContourIntersectionPoint& cp : graph.map_infill_end_point_to_boundary)
            if (cp.contour_idx != boundary_idx_unconnected && cp.next_on_contour != &cp && cp.could_connect_next())
                arches.push_back({ &cp, path_length_along_contour_ccw(&cp, cp.next_on_contour, graph.boundary_params[cp.contour_idx].back()) });
        std::sort(arches.begin(), arches.end(), [](const auto& l, const auto& r) { return l.arc_length < r.arc_length; });
    }

    //FIXME improve the Traveling Salesman problem with 2-opt and 3-opt local optimization.
    for (Arc &arc : arches)
        if (! arc.intersection->consumed && ! arc.intersection->next_on_contour->consumed) {
            // Indices of the polylines to be connected by a perimeter segment.
            ContourIntersectionPoint *cp1            = arc.intersection;
            ContourIntersectionPoint *cp2            = arc.intersection->next_on_contour;
            size_t                    polyline_idx1  = get_and_update_merged_with(((cp1 - graph.map_infill_end_point_to_boundary.data()) / 2));
            size_t                    polyline_idx2  = get_and_update_merged_with(((cp2 - graph.map_infill_end_point_to_boundary.data()) / 2));
            const Points             &contour        = graph.boundary[cp1->contour_idx];
            const std::vector<double> &contour_params = graph.boundary_params[cp1->contour_idx];
            if (polyline_idx1 != polyline_idx2) {
                Polyline &polyline1 = infill_ordered[polyline_idx1];
                Polyline &polyline2 = infill_ordered[polyline_idx2];
                if (arc.arc_length < anchor_length_max) {
                    // Not closing a loop, connecting the lines.
                    assert(contour[cp1->point_idx] == polyline1.points.front() || contour[cp1->point_idx] == polyline1.points.back());
                    if (contour[cp1->point_idx] == polyline1.points.front())
                        polyline1.reverse();
                    assert(contour[cp2->point_idx] == polyline2.points.front() || contour[cp2->point_idx] == polyline2.points.back());
                    if (contour[cp2->point_idx] == polyline2.points.back())
                        polyline2.reverse();
                    take(polyline1, polyline2, contour, cp1, cp2, false);
                    // Mark the second polygon as merged with the first one.
                    if (polyline_idx2 < polyline_idx1) {
                        polyline2 = std::move(polyline1);
                        polyline1.points.clear();
                        merged_with[polyline_idx1] = merged_with[polyline_idx2];
                    } else {
                        polyline2.points.clear();
                        merged_with[polyline_idx2] = merged_with[polyline_idx1];
                    }
                } else if (anchor_length > SCALED_EPSILON) {
                    // Move along the perimeter, but don't take the whole arc.
                    take_limited(polyline1, contour, contour_params, cp1, cp2, false, anchor_length, line_half_width);
                    take_limited(polyline2, contour, contour_params, cp2, cp1, true,  anchor_length, line_half_width);
                }
            }
        }

    // Connect the remaining open infill lines to the perimeter lines if possible.
    for (ContourIntersectionPoint &contour_point : graph.map_infill_end_point_to_boundary)
        if (! contour_point.consumed && contour_point.contour_idx != boundary_idx_unconnected) {
            const Points              &contour        = graph.boundary[contour_point.contour_idx];
            const std::vector<double> &contour_params = graph.boundary_params[contour_point.contour_idx];

            double    lprev         = contour_point.could_connect_prev() ?
                path_length_along_contour_ccw(contour_point.prev_on_contour, &contour_point, contour_params.back()) :
                std::numeric_limits<double>::max();
            double    lnext         = contour_point.could_connect_next() ?
                path_length_along_contour_ccw(&contour_point, contour_point.next_on_contour, contour_params.back()) :
                std::numeric_limits<double>::max();
            size_t    polyline_idx  = get_and_update_merged_with(((&contour_point - graph.map_infill_end_point_to_boundary.data()) / 2));
            Polyline &polyline      = infill_ordered[polyline_idx];
            assert(! polyline.empty());
            assert(contour[contour_point.point_idx] == polyline.points.front() || contour[contour_point.point_idx] == polyline.points.back());
            bool connected = false;
            for (double l : { std::min(lprev, lnext), std::max(lprev, lnext) }) {
                if (l == std::numeric_limits<double>::max() || l > anchor_length_max)
                    break;
                // Take the complete contour.
                bool      reversed      = l == lprev;
                ContourIntersectionPoint *cp2 = reversed ? contour_point.prev_on_contour : contour_point.next_on_contour;
                // Identify which end of the polyline touches the boundary.
                size_t    polyline_idx2 = get_and_update_merged_with(((cp2 - graph.map_infill_end_point_to_boundary.data()) / 2));
                if (polyline_idx == polyline_idx2)
                    // Try the other side.
                    continue;
                // Not closing a loop.
                if (contour[contour_point.point_idx] == polyline.points.front())
                    polyline.reverse();
                Polyline &polyline2 = infill_ordered[polyline_idx2];
                assert(! polyline.empty());
                assert(contour[cp2->point_idx] == polyline2.points.front() || contour[cp2->point_idx] == polyline2.points.back());
                if (contour[cp2->point_idx] == polyline2.points.back())
                    polyline2.reverse();
                take(polyline, polyline2, contour, &contour_point, cp2, reversed);
                if (polyline_idx < polyline_idx2) {
                    // Mark the second polyline as merged with the first one.
                    merged_with[polyline_idx2] = polyline_idx;
                    polyline2.points.clear();
                } else {
                    // Mark the first polyline as merged with the second one.
                    merged_with[polyline_idx] = polyline_idx2;
                    polyline2 = std::move(polyline);
                    polyline.points.clear();
                }
                connected = true;
                break;
            }
            if (! connected && anchor_length > SCALED_EPSILON) {
                // Which to take? One could optimize for:
                // 1) Shortest path
                // 2) Hook length
                // ...
                // Let's take the longer now, as this improves the chance of another hook to be placed on the other side of this contour point.
                double l = std::max(contour_point.contour_not_taken_length_prev, contour_point.contour_not_taken_length_next);
                if (l > SCALED_EPSILON) {
                    if (contour_point.contour_not_taken_length_prev > contour_point.contour_not_taken_length_next)
                        take_limited(polyline, contour, contour_params, &contour_point, contour_point.prev_on_contour, true, anchor_length, line_half_width);
                    else
                        take_limited(polyline, contour, contour_params, &contour_point, contour_point.next_on_contour, false, anchor_length, line_half_width);
                }
            }
        }

    polylines_out.reserve(polylines_out.size() + std::count_if(infill_ordered.begin(), infill_ordered.end(), [](const Polyline &pl) { return ! pl.empty(); }));
	for (Polyline &pl : infill_ordered)
		if (! pl.empty())
			polylines_out.emplace_back(std::move(pl));
}

// Extend the infill lines along the perimeters, this is mainly useful for grid aligned support, where a perimeter line may be nearly
// aligned with the infill lines.
static inline void base_support_extend_infill_lines(Polylines &infill, BoundaryInfillGraph &graph, const double spacing, const FillParams &params)
{
/*
    // Backup the source lines.
    Lines lines;
    lines.reserve(linfill.size());
    std::transform(infill.begin(), infill.end(), std::back_inserter(lines), [](const Polyline &pl) { assert(pl.size() == 2); return Line(pl.points.begin(), pl.points.end()); });
*/

    const double    line_spacing    = scale_(spacing) / params.density;
    // Maximum deviation perpendicular to the infill line to allow merging as a continuation of the same infill line.
    const auto      dist_max_x      = coord_t(line_spacing * 0.33);
    // Minimum length of the arc away from the infill end point to allow merging as a continuation of the same infill line.
    const auto      dist_min_y      = coord_t(line_spacing * 0.5);

    for (ContourIntersectionPoint &cp : graph.map_infill_end_point_to_boundary) {
        const Points                &contour         = graph.boundary[cp.contour_idx];
        const std::vector<double>   &contour_param   = graph.boundary_params[cp.contour_idx];
        const Point                 &pt              = contour[cp.point_idx];
        const bool                   first           = graph.first(cp);
        int                          extend_next_idx = -1;
        int                          extend_prev_idx = -1;
        coord_t                      dist_y_prev;
        coord_t                      dist_y_next;
        double                       arc_len_prev;
        double                       arc_len_next;

        if (! graph.next_vertical(cp)){
            size_t i = cp.point_idx;
            size_t j = next_idx_modulo(i, contour);
            while (j != cp.next_on_contour->point_idx) {
                //const Point &p1 = contour[i];
                const Point &p2 = contour[j];
                if (std::abs(p2.x() - pt.x()) > dist_max_x)
                    break;
                i = j;
                j = next_idx_modulo(j, contour);
            }
            if (i != cp.point_idx) {
                const Point &p2 = contour[i];
                coord_t      dist_y = p2.y() - pt.y();
                if (first)
                    dist_y = - dist_y;
                if (dist_y > dist_min_y) {
                    arc_len_next    = closed_contour_distance_ccw(contour_param[cp.point_idx], contour_param[i], contour_param.back());
                    if (arc_len_next < cp.contour_not_taken_length_next) {
                        extend_next_idx = i;
                        dist_y_next     = dist_y;
                    }
                }
            }
        }

        if (! graph.prev_vertical(cp)) {
            size_t i = cp.point_idx;
            size_t j = prev_idx_modulo(i, contour);
            while (j != cp.prev_on_contour->point_idx) {
                //const Point &p1 = contour[i];
                const Point &p2 = contour[j];
                if (std::abs(p2.x() - pt.x()) > dist_max_x)
                    break;
                i = j;
                j = prev_idx_modulo(j, contour);
            }
            if (i != cp.point_idx) {
                const Point &p2 = contour[i];
                coord_t      dist_y = p2.y() - pt.y();
                if (first)
                    dist_y = - dist_y;
                if (dist_y > dist_min_y) {
                    arc_len_prev = closed_contour_distance_ccw(contour_param[i], contour_param[cp.point_idx], contour_param.back());
                    if (arc_len_prev < cp.contour_not_taken_length_prev) {
                        extend_prev_idx = i;
                        dist_y_prev     = dist_y;
                    }
                }
            }
        }

        if (extend_prev_idx >= 0 && extend_next_idx >= 0)
            // Which side to move the point?
            dist_y_prev < dist_y_next ? extend_prev_idx : extend_next_idx = -1;

        assert(cp.prev_trimmed == cp.prev_on_contour->next_trimmed);
        assert(cp.next_trimmed == cp.next_on_contour->prev_trimmed);
        Polyline &infill_line = infill[(&cp - graph.map_infill_end_point_to_boundary.data()) / 2];
        if (extend_prev_idx >= 0) {
            if (first)
                infill_line.reverse();
            take_cw_full(infill_line, contour, cp.point_idx, extend_prev_idx);
            if (first)
                infill_line.reverse();
            cp.point_idx = extend_prev_idx;
            if (cp.prev_trimmed)
                cp.contour_not_taken_length_prev -= arc_len_prev;
            else
                cp.contour_not_taken_length_prev = cp.prev_on_contour->contour_not_taken_length_next =
                    closed_contour_distance_ccw(contour_param[cp.prev_on_contour->point_idx], contour_param[cp.point_idx], contour_param.back());
            cp.trim_next(0);
            cp.next_on_contour->prev_trimmed = true;
        } else if (extend_next_idx >= 0) {
            if (first)
                infill_line.reverse();
            take_ccw_full(infill_line, contour, cp.point_idx, extend_next_idx);
            if (first)
                infill_line.reverse();
            cp.point_idx = extend_next_idx;
            cp.trim_prev(0);
            cp.prev_on_contour->next_trimmed = true;
            if (cp.next_trimmed)
                cp.contour_not_taken_length_next -= arc_len_next;
            else
                cp.contour_not_taken_length_next = cp.next_on_contour->contour_not_taken_length_prev =
                    closed_contour_distance_ccw(contour_param[cp.point_idx], contour_param[cp.next_on_contour->point_idx], contour_param.back());
        }
    }
}

// Called by Fill::connect_base_support() as part of the sparse support infill generator.
// Emit contour loops tracing the contour from tbegin to tend inside a band of (left, right).
// The contour is supposed to enter the "forbidden" zone outside of the (left, right) band at tbegin and also at tend.
static inline void emit_loops_in_band(
    // Vertical band, which will trim the contour between tbegin and tend.
    coord_t                      left, 
    coord_t                      right,
    // Contour and its parametrization.
    const Points                &contour,
    const std::vector<double>   &contour_params,
    // Span of the parameters of an arch to trim with the vertical band.
    double                       tbegin,
    double                       tend, 
    // Minimum arch length to put into polylines_out. Shorter arches are not necessary to support a dense support infill.
    double                       min_length,
    Polylines                   &polylines_out)
{
    assert(left < right);
    assert(contour.size() + 1 == contour_params.size());
    assert(contour.size() >= 3);
#ifndef NDEBUG
    double contour_length = contour_params.back();
    assert(tbegin >= 0 && tbegin < contour_length);
    assert(tend   >= 0 && tend   < contour_length);
    assert(min_length > 0);
#endif // NDEBUG

    // Find iterators of the range of segments, where the first and last segment contains tbegin and tend.
    size_t ibegin, iend;
    {
        auto it_begin = std::lower_bound(contour_params.begin(), contour_params.end(), tbegin);
        auto it_end   = std::lower_bound(contour_params.begin(), contour_params.end(), tend);
        assert(it_begin != contour_params.end());
        assert(it_end   != contour_params.end());
        if (*it_begin != tbegin) {
            assert(it_begin != contour_params.begin());
            -- it_begin;
        }
        ibegin = it_begin - contour_params.begin();
        iend   = it_end   - contour_params.begin();
    }

    if (ibegin == contour.size())
        ibegin = 0;
    if (iend == contour.size())
        iend = 0;
    assert(ibegin != iend);

    // Trim the start and end segment to calculate start and end points.
    Point pbegin, pend;
    {
        double t1 = contour_params[ibegin];
        double t2 = next_value_modulo(ibegin, contour_params);
        pbegin = lerp(contour[ibegin], next_value_modulo(ibegin, contour), (tbegin - t1) / (t2 - t1));
        t1 = contour_params[iend];
        t2 = prev_value_modulo(iend, contour_params);
        pend = lerp(contour[iend], prev_value_modulo(iend, contour), (tend - t1) / (t2 - t1));
    }

    // Trace the contour from ibegin to iend.
    enum Side {
        Left,
        Right,
        Mid,
        Unknown
    };

    enum InOutBand {
        Entering, 
        Leaving,
    };

    class State {
    public:
        State(coord_t left, coord_t right, double min_length, Polylines &polylines_out) : 
            m_left(left), m_right(right), m_min_length(min_length), m_polylines_out(polylines_out) {}

        void add_inner_point(const Point* p)
        {
            m_polyline.points.emplace_back(*p);
        }

        void add_outer_point(const Point* p)
        {
            if (m_polyline_end > 0)
                m_polyline.points.emplace_back(*p);
        }

        void add_interpolated_point(const Point* p1, const Point* p2, Side side, InOutBand inout)
        {
            assert(side == Left || side == Right);

            coord_t x = side == Left ? m_left : m_right;
            coord_t y = p1->y() + coord_t(double(x - p1->x()) * double(p2->y() - p1->y()) / double(p2->x() - p1->x()));

            if (inout == Leaving) {
                assert(m_polyline_end == 0);
                m_polyline_end = m_polyline.size();
                m_polyline.points.emplace_back(x, y);
            } else {
                assert(inout == Entering);
                if (m_polyline_end > 0) {
                    if ((this->side1 == Left) == (y - m_polyline.points[m_polyline_end].y() < 0)) {
                        // Emit the vertical segment. Remove the point, where the source contour was split the last time at m_left / m_right.
                        m_polyline.points.erase(m_polyline.points.begin() + m_polyline_end);
                    } else {
                        // Don't emit the vertical segment, split the contour.
                        this->finalize();
                        m_polyline.points.emplace_back(x, y);
                    }
                    m_polyline_end = 0;
                } else
                    m_polyline.points.emplace_back(x, y);
            }
        };

        void finalize()
        {
            m_polyline.points.erase(m_polyline.points.begin() + m_polyline_end, m_polyline.points.end());
            if (! m_polyline.empty()) {
                if (! m_polylines_out.empty() && (m_polylines_out.back().points.back() - m_polyline.points.front()).cast<int64_t>().squaredNorm() < SCALED_EPSILON)
                    m_polylines_out.back().points.insert(m_polylines_out.back().points.end(), m_polyline.points.begin() + 1, m_polyline.points.end());
                else if (m_polyline.length() > m_min_length)
                    m_polylines_out.emplace_back(std::move(m_polyline));
                m_polyline.clear();
            }
        };

    private:
        coord_t      m_left;
        coord_t      m_right;
        double       m_min_length;
        Polylines   &m_polylines_out;

        Polyline     m_polyline;
        size_t       m_polyline_end { 0 };
        Polyline     m_overlapping;

    public:
        Side         side1 { Unknown };
        Side         side2 { Unknown };
    };

    State state { left, right, min_length, polylines_out };

    const Point *p1 = &pbegin;
    auto side = [left, right](const Point* p) {
        coord_t x = p->x();
        return x < left ? Left : x > right ? Right : Mid;
    };
    state.side1 = side(p1);
    if (state.side1 == Mid)
        state.add_inner_point(p1);

    for (size_t i = ibegin; i != iend; ) {
        size_t inext = i + 1;
        if (inext == contour.size())
            inext = 0;
        const Point *p2 = inext == iend ? &pend : &contour[inext];
        state.side2 = side(p2);
        if (state.side1 == Mid) {
            if (state.side2 == Mid) {
                // Inside the band.
                state.add_inner_point(p2);
            } else {
                // From intisde the band to the outside of the band.
                state.add_interpolated_point(p1, p2, state.side2, Leaving);
                state.add_outer_point(p2);
            }
        } else if (state.side2 == Mid) {
            // From outside the band into the band.
            state.add_interpolated_point(p1, p2, state.side1, Entering);
            state.add_inner_point(p2);
        } else if (state.side1 != state.side2) {
            // Both points outside the band.
            state.add_interpolated_point(p1, p2, state.side1, Entering);
            state.add_interpolated_point(p1, p2, state.side2, Leaving);
        } else {
            // Complete segment is outside.
            assert((state.side1 == Left && state.side2 == Left) || (state.side1 == Right && state.side2 == Right));
            state.add_outer_point(p2);
        }
        state.side1 = state.side2;
        p1 = p2;
        i  = inext;
    }
    state.finalize();
}

#ifdef INFILL_DEBUG_OUTPUT
static void export_partial_infill_to_svg(const std::string &path, const BoundaryInfillGraph &graph, const Polylines &infill, const Polylines &emitted)
{
    Polygons polygons;
    for (const Points &pts : graph.boundary)
        polygons.emplace_back(pts);
    BoundingBox bbox = get_extents(polygons);
    bbox.merge(get_extents(infill));
    ::Slic3r::SVG svg(path, bbox);
    svg.draw(union_ex(polygons));
    svg.draw(infill, "blue");
    svg.draw(emitted, "darkblue");
    for (const ContourIntersectionPoint &cp : graph.map_infill_end_point_to_boundary)
        svg.draw(graph.point(cp), cp.consumed ? "red" : "green", scaled(0.2));
    for (const ContourIntersectionPoint &cp : graph.map_infill_end_point_to_boundary) {
        assert(cp.next_trimmed == cp.next_on_contour->prev_trimmed);
        assert(cp.prev_trimmed == cp.prev_on_contour->next_trimmed);
        if (cp.contour_not_taken_length_next > SCALED_EPSILON) {
            Polyline pl { graph.point(cp) };
            take_ccw_limited(pl, graph.boundary[cp.contour_idx], graph.boundary_params[cp.contour_idx], cp.point_idx, cp.next_on_contour->point_idx, cp.contour_not_taken_length_next);
            svg.draw(pl, cp.could_take_next() ? "lime" : "magenta", scaled(0.1));
        }
        if (cp.contour_not_taken_length_prev > SCALED_EPSILON) {
            Polyline pl { graph.point(cp) };
            take_cw_limited(pl, graph.boundary[cp.contour_idx], graph.boundary_params[cp.contour_idx], cp.point_idx, cp.prev_on_contour->point_idx, cp.contour_not_taken_length_prev);
            svg.draw(pl, cp.could_take_prev() ? "lime" : "magenta", scaled(0.1));
        }
    }
}
#endif // INFILL_DEBUG_OUTPUT

// To classify perimeter segments connecting infill lines, whether they are required for structural stability of the supports.
struct SupportArcCost
{
    // Connecting one end of an infill line to the other end of the same infill line.
    bool    self_loop { false };
    // Some of the arc touches some infill line.
    bool    open { false };
    // How needed is this arch for support structural stability.
    // Zero means don't take. The higher number, the more likely it is to take the arc.
    double  cost { 0 };
};

static double evaluate_support_arch_cost(const Polyline &pl)
{
    Point front = pl.points.front();
    Point back  = pl.points.back();

    coord_t ymin = front.y();
    coord_t ymax = back.y();
    if (ymin > ymax)
        std::swap(ymin, ymax);

    double dmax = 0;
    // Maximum distance in Y axis out of the (ymin, ymax) band and from the (front, back) line.
    Linef line { front.cast<double>(), back.cast<double>() };
    for (const Point &pt : pl.points)
        dmax = std::max<double>(std::max(dmax, line_alg::distance_to(line, Vec2d(pt.cast<double>()))), std::max(pt.y() - ymax, ymin - pt.y()));
    return dmax;
}

// Costs for prev / next arch of each infill line end point.
static inline std::vector<SupportArcCost> evaluate_support_arches(Polylines &infill, BoundaryInfillGraph &graph, const double spacing, const FillParams &params)
{
    std::vector<SupportArcCost> arches(graph.map_infill_end_point_to_boundary.size() * 2);

    Polyline pl;
    for (ContourIntersectionPoint &cp : graph.map_infill_end_point_to_boundary) {
        // Not a losed loop, such loops should already be consumed.
        assert(cp.next_on_contour != &cp);
        const size_t                    infill_line_idx = &cp - graph.map_infill_end_point_to_boundary.data();
        const bool                      first           = (infill_line_idx & 1) == 0;
        const ContourIntersectionPoint *other_end       = first ? &cp + 1 : &cp - 1;

        SupportArcCost &out_prev = arches[infill_line_idx * 2];
        SupportArcCost &out_next = *(&out_prev + 1);
        out_prev.self_loop = cp.prev_on_contour == other_end;
        out_prev.open      = cp.prev_trimmed;
        out_next.self_loop = cp.next_on_contour == other_end;
        out_next.open      = cp.next_trimmed;

        if (cp.contour_not_taken_length_next > SCALED_EPSILON) {
            pl.clear();
            pl.points.emplace_back(graph.point(cp));
            if (cp.next_trimmed)
                take_ccw_limited(pl, graph.boundary[cp.contour_idx], graph.boundary_params[cp.contour_idx], cp.point_idx, cp.next_on_contour->point_idx, cp.contour_not_taken_length_next);
            else
                take_ccw_full(pl, graph.boundary[cp.contour_idx], cp.point_idx, cp.next_on_contour->point_idx);
            out_next.cost = evaluate_support_arch_cost(pl);
        }

        if (cp.contour_not_taken_length_prev > SCALED_EPSILON) {
            pl.clear();
            pl.points.emplace_back(graph.point(cp));
            if (cp.prev_trimmed)
                take_cw_limited(pl, graph.boundary[cp.contour_idx], graph.boundary_params[cp.contour_idx], cp.point_idx, cp.prev_on_contour->point_idx, cp.contour_not_taken_length_prev);
            else
                take_cw_full(pl, graph.boundary[cp.contour_idx], cp.point_idx, cp.prev_on_contour->point_idx);
            out_prev.cost = evaluate_support_arch_cost(pl);
        }
    }

    return arches;
}

// Both the poly_with_offset and polylines_out are rotated, so the infill lines are strictly vertical.
void Fill::connect_base_support(Polylines &&infill_ordered, const std::vector<const Polygon*> &boundary_src, const BoundingBox &bbox, Polylines &polylines_out, const double spacing, const FillParams &params)
{
//    assert(! infill_ordered.empty());
    assert(params.anchor_length     >= 0.);
    assert(params.anchor_length_max >= 0.01f);
    assert(params.anchor_length_max >= params.anchor_length);

    BoundaryInfillGraph graph = create_boundary_infill_graph(infill_ordered, boundary_src, bbox, spacing);

#ifdef INFILL_DEBUG_OUTPUT
    static int iRun = 0;
    ++ iRun;
    export_partial_infill_to_svg(debug_out_path("connect_base_support-initial-%03d.svg", iRun), graph, infill_ordered, polylines_out);
#endif // INFILL_DEBUG_OUTPUT

    const double        line_half_width = 0.5 * scale_(spacing);
    const double        line_spacing    = scale_(spacing) / params.density;
    const double        min_arch_length = 1.3 * line_spacing;
    const double        trim_length     = line_half_width * 0.3;

// After mark_boundary_segments_touching_infill() marks boundary segments overlapping trimmed infill lines,
// there are possibly some very short boundary segments unmarked, but overlapping the untrimmed infill lines fully.
// Mark those short boundary segments.
    mark_boundary_segments_overlapping_infill(graph, infill_ordered, scale_(spacing));

#ifdef INFILL_DEBUG_OUTPUT
    export_partial_infill_to_svg(debug_out_path("connect_base_support-marked-%03d.svg", iRun), graph, infill_ordered, polylines_out);
#endif // INFILL_DEBUG_OUTPUT

    // Detect loops with zero infill end points connected.
    // Extrude these loops as perimeters.
    {
        std::vector<size_t> num_boundary_contour_infill_points(graph.boundary.size(), 0);
        for (ContourIntersectionPoint &cp : graph.map_infill_end_point_to_boundary)
            ++ num_boundary_contour_infill_points[cp.contour_idx];
        for (size_t i = 0; i < num_boundary_contour_infill_points.size(); ++ i)
            if (num_boundary_contour_infill_points[i] == 0 && graph.boundary_params[i].back() > trim_length + 0.5 * line_spacing) {
                // Emit a perimeter.
                Polyline pl(graph.boundary[i]);
                pl.points.emplace_back(pl.points.front());
                pl.clip_end(trim_length);
                if (pl.size() > 1)
                    polylines_out.emplace_back(std::move(pl));
            }
    }

    // Before processing the boundary arches, emit those arches, which were trimmed by the infill lines at both sides, but which
    // depart from the infill line at least once after touching the infill line.
    for (ContourIntersectionPoint &cp : graph.map_infill_end_point_to_boundary) {
        if (cp.next_on_contour && cp.next_trimmed && cp.next_on_contour->prev_trimmed) {
            // The arch is leaving one infill line to end up at the same infill line or at the neighbouring one.
            // The arch is touching one of those infill lines at least once.
            // Trace those arches and emit their parts, which are not attached to the end points and they are not overlapping with the two infill lines mentioned.
            bool    first    = graph.first(cp);
            coord_t left     = graph.point(cp).x();
            coord_t right    = left;
            if (first) {
                left  += line_half_width;
                right += line_spacing - line_half_width;
            } else {
                left  -= line_spacing - line_half_width;
                right -= line_half_width;
            }
            double param_start    = cp.param + cp.contour_not_taken_length_next;
            double param_end      = cp.next_on_contour->param - cp.next_on_contour->contour_not_taken_length_prev;
            double contour_length = graph.boundary_params[cp.contour_idx].back();
            if (param_start >= contour_length)
                param_start -= contour_length;
            if (param_end < 0)
                param_end += contour_length;
            // Verify that the interval (param_overlap1, param_overlap2) is inside the interval (ip_low->param, ip_high->param).
            assert(cyclic_interval_inside_interval(cp.param, cp.next_on_contour->param, param_start, param_end, contour_length));
            emit_loops_in_band(left, right, graph.boundary[cp.contour_idx], graph.boundary_params[cp.contour_idx], param_start, param_end, 0.5 * line_spacing, polylines_out);
        }
    }
#ifdef INFILL_DEBUG_OUTPUT
    export_partial_infill_to_svg(debug_out_path("connect_base_support-excess-%03d.svg", iRun), graph, infill_ordered, polylines_out);
#endif // INFILL_DEBUG_OUTPUT

    base_support_extend_infill_lines(infill_ordered, graph, spacing, params);
    
#ifdef INFILL_DEBUG_OUTPUT
    export_partial_infill_to_svg(debug_out_path("connect_base_support-extended-%03d.svg", iRun), graph, infill_ordered, polylines_out);
#endif // INFILL_DEBUG_OUTPUT

    std::vector<size_t> merged_with(infill_ordered.size());
    std::iota(merged_with.begin(), merged_with.end(), 0);
    auto get_and_update_merged_with = [&graph, &merged_with](const ContourIntersectionPoint *cp) -> size_t {
        size_t polyline_idx = (cp - graph.map_infill_end_point_to_boundary.data()) / 2;
        for (size_t last = polyline_idx;;) {
            size_t lower = merged_with[last];
            assert(lower <= last);
            if (lower == last) {
                merged_with[polyline_idx] = last;
                return last;
            }
            last = lower;
        }
        assert(false);
        return std::numeric_limits<size_t>::max();
    };

    auto vertical = [](BoundaryInfillGraph::Direction dir) {
        return dir == BoundaryInfillGraph::Up || dir == BoundaryInfillGraph::Down;
    };
    // When both left / right arch connected to cp is vertical (ends up at the same vertical infill line), which one to take?
    auto take_vertical_prev = [](const ContourIntersectionPoint &cp) {
        return cp.prev_trimmed == cp.next_trimmed ?
            // Both are either trimmed or not trimmed. Take the longer contour.
            cp.contour_not_taken_length_prev > cp.contour_not_taken_length_next :
            // One is trimmed, the other is not trimmed. Take the not trimmed.
            ! cp.prev_trimmed && cp.next_trimmed;
    };

    // Connect infill lines at cp and cpo_next_on_contour.
    // If the complete arch cannot be taken, then 
    // if (take_first)
    //    take the infill line at cp and an arc from cp towards cp.next_on_contour.
    // else
    //    take the infill line at cp_next_on_contour and an arc from cp.next_on_contour towards cp.
    // If cp1 == next_on_contour (a single infill line is connected to a contour, this is a valid case for contours with holes),
    // then extrude the full circle.
    // Nothing is done if the arch could no more be taken (one of it end points were consumed already).
    auto take_next = [&graph, &infill_ordered, &merged_with, get_and_update_merged_with, line_half_width, trim_length](ContourIntersectionPoint &cp, bool take_first) {
        // Indices of the polylines to be connected by a perimeter segment.
        ContourIntersectionPoint  *cp1            = &cp;
        ContourIntersectionPoint  *cp2            = cp.next_on_contour;
        assert(cp1->next_trimmed == cp2->prev_trimmed);
        //assert(cp1->next_trimmed || cp1->consumed == cp2->consumed);
        if (take_first ? cp1->consumed : cp2->consumed)
            return;
        size_t                     polyline_idx1  = get_and_update_merged_with(cp1);
        size_t                     polyline_idx2  = get_and_update_merged_with(cp2);
        Polyline                  &polyline1      = infill_ordered[polyline_idx1];
        Polyline                  &polyline2      = infill_ordered[polyline_idx2];
        const Points              &contour        = graph.boundary[cp1->contour_idx];
        const std::vector<double> &contour_params = graph.boundary_params[cp1->contour_idx];
        assert(cp1->consumed || contour[cp1->point_idx] == polyline1.points.front() || contour[cp1->point_idx] == polyline1.points.back());
        assert(cp2->consumed || contour[cp2->point_idx] == polyline2.points.front() || contour[cp2->point_idx] == polyline2.points.back());
        bool trimmed = take_first ? cp1->next_trimmed : cp2->prev_trimmed;
        if (! trimmed) {
            // Trim the end if closing a loop or making a T-joint.
            trimmed = cp1 == cp2 || polyline_idx1 == polyline_idx2 || (take_first ? cp2->consumed : cp1->consumed);
            if (! trimmed) {
                const bool                      cp1_first = ((cp1 - graph.map_infill_end_point_to_boundary.data()) & 1) == 0;
                const ContourIntersectionPoint* cp1_other = cp1_first ? cp1 + 1 : cp1 - 1;
                // Self loop, connecting the end points of the same infill line.
                trimmed = cp2 == cp1_other;
            }
            if (trimmed) /* [[unlikely]] */ {
                // Single end point on a contour. This may happen on contours with holes. Extrude a loop.
                // Or a self loop, connecting the end points of the same infill line.
                // Or closing a chain of infill lines. This may happen if infilling a contour with a hole.
                double len = cp1 == cp2 ? contour_params.back() : path_length_along_contour_ccw(cp1, cp2, contour_params.back());
                if (take_first) {
                    cp1->trim_next(std::max(0., len - trim_length - SCALED_EPSILON));
                    cp2->trim_prev(0);
                } else {
                    cp1->trim_next(0);
                    cp2->trim_prev(std::max(0., len - trim_length - SCALED_EPSILON));
                }
            }
        }
        if (trimmed) {
            if (take_first)
                take_limited(polyline1, contour, contour_params, cp1, cp2, false, 1e10, line_half_width);
            else
                take_limited(polyline2, contour, contour_params, cp2, cp1, true, 1e10, line_half_width);
        } else if (! cp1->consumed && ! cp2->consumed) {
            if (contour[cp1->point_idx] == polyline1.points.front())
                polyline1.reverse();
            if (contour[cp2->point_idx] == polyline2.points.back())
                polyline2.reverse();
            take(polyline1, polyline2, contour, cp1, cp2, false);
            // Mark the second polygon as merged with the first one.
            if (polyline_idx2 < polyline_idx1) {
                polyline2 = std::move(polyline1);
                polyline1.points.clear();
                merged_with[polyline_idx1] = merged_with[polyline_idx2];
            } else {
                polyline2.points.clear();
                merged_with[polyline_idx2] = merged_with[polyline_idx1];
            }
        }
    };

    // Consume all vertical arches. If a vertical arch is touching a neighboring vertical infill line, thus the vertical arch is trimmed,
    // only consume the trimmed part if it is longer than min_arch_length.
    for (ContourIntersectionPoint &cp : graph.map_infill_end_point_to_boundary) {
        assert(cp.contour_idx != boundary_idx_unconnected);
        if (cp.consumed)
            continue;
        const ContourIntersectionPoint &cp_other = graph.other(cp);
        assert((cp.next_on_contour == &cp_other) == (cp_other.prev_on_contour == &cp));
        assert((cp.prev_on_contour == &cp_other) == (cp_other.next_on_contour == &cp));
        BoundaryInfillGraph::Direction dir_prev = graph.dir_prev(cp);
        BoundaryInfillGraph::Direction dir_next = graph.dir_next(cp);
        // Following code will also consume contours with just a single infill line attached. (cp1->next_on_contour == cp1).
        assert((cp.next_on_contour == &cp) == (cp.prev_on_contour == &cp));
        bool can_take_prev = vertical(dir_prev) && ! cp.prev_on_contour->consumed && cp.prev_on_contour != &cp_other;
        bool can_take_next = vertical(dir_next) && ! cp.next_on_contour->consumed && cp.next_on_contour != &cp_other;
        if (can_take_prev && (! can_take_next || take_vertical_prev(cp))) {
            if (! cp.prev_trimmed || cp.contour_not_taken_length_prev > min_arch_length)
                // take previous
                take_next(*cp.prev_on_contour, false);
        } else if (can_take_next) {
            if (! cp.next_trimmed || cp.contour_not_taken_length_next > min_arch_length)
                // take next
                take_next(cp, true);
        }
    }

#ifdef INFILL_DEBUG_OUTPUT
    export_partial_infill_to_svg(debug_out_path("connect_base_support-vertical-%03d.svg", iRun), graph, infill_ordered, polylines_out);
#endif // INFILL_DEBUG_OUTPUT

    const std::vector<SupportArcCost> arches = evaluate_support_arches(infill_ordered, graph, spacing, params);
    static const double cost_low      = line_spacing * 1.3;
    static const double cost_high     = line_spacing * 2.;
    static const double cost_veryhigh = line_spacing * 3.;

    {
        std::vector<const SupportArcCost*> selected;
        selected.reserve(graph.map_infill_end_point_to_boundary.size());
        for (ContourIntersectionPoint &cp : graph.map_infill_end_point_to_boundary) {
            if (cp.consumed)
                continue;
            const SupportArcCost &cost_prev = arches[(&cp - graph.map_infill_end_point_to_boundary.data()) * 2];
            const SupportArcCost &cost_next = *(&cost_prev + 1);
            double                cost_min = cost_prev.cost;
            double                cost_max = cost_next.cost;
            if (cost_min > cost_max)
                std::swap(cost_min, cost_max);
            if (cost_max < cost_low || cost_min > cost_high)
                // Don't take any of the prev / next arches now, take zig-zag instead. It does not matter which one will be taken.
                continue;
            const double           cost_diff_relative = (cost_max - cost_min) / cost_max;
            if (cost_diff_relative < 0.25)
                // Don't take any of the prev / next arches now, take zig-zag instead. It does not matter which one will be taken.
                continue;
            if (cost_prev.cost > cost_low)
                selected.emplace_back(&cost_prev);
            if (cost_next.cost > cost_low)
                selected.emplace_back(&cost_next);
        }
        // Take the longest arch first.
        std::sort(selected.begin(), selected.end(), [](const auto *l, const auto *r) { return l->cost > r->cost; });
        // And connect along the arches.
        for (const SupportArcCost *arc : selected) {
            ContourIntersectionPoint &cp = graph.map_infill_end_point_to_boundary[(arc - arches.data()) / 2];
            if (! cp.consumed) {
                bool prev = ((arc - arches.data()) & 1) == 0;
                if (prev)
                    take_next(*cp.prev_on_contour, false);
                else
                    take_next(cp, true);
            }
        }
    }

#if 0
    {
        // Connect infill lines with long horizontal arches. Only take a horizontal arch, if it will not block
        // the end caps (vertical arches) at the other side of the infill line.
        struct Arc {
            ContourIntersectionPoint    *intersection;
            double                       arc_length;
            bool                         take_next;
        };
        std::vector<Arc> arches;
        arches.reserve(graph.map_infill_end_point_to_boundary.size());
        for (ContourIntersectionPoint &cp : graph.map_infill_end_point_to_boundary) {
            if (cp.consumed)
                continue;
            // Not a losed loop, such loops should already be consumed.
            assert(cp.next_on_contour != &cp);
            const bool                      first     = ((&cp - graph.map_infill_end_point_to_boundary.data()) & 1) == 0;
            const ContourIntersectionPoint *other_end = first ? &cp + 1 : &cp - 1;
            const bool                      loop_next = cp.next_on_contour == other_end;
            if (! loop_next && cp.could_connect_next()) {
                if (cp.contour_not_taken_length_next > min_arch_length) {
                    // Try both directions. This is useful to be able to close a loop back to the same line to take a long arch.
                    arches.push_back({ &cp, cp.contour_not_taken_length_next, true });
                    arches.push_back({ cp.next_on_contour, cp.contour_not_taken_length_next, false });
                }
            } else {
                //bool    first     = ((&cp - graph.map_infill_end_point_to_boundary) & 1) == 0;
                if (cp.prev_trimmed && cp.could_take_prev()) {
                    //FIXME trace the trimmed line to decide what priority to assign to it.
                    // Is the end point close to the current vertical line or to the other vertical line?
                    const Point &pt   = graph.point(cp);
                    const Point &prev = graph.point(*cp.prev_on_contour);
                    if (std::abs(pt.x() - prev.x()) < coord_t(0.5 * line_spacing)) {
                        // End point on the same line.
                        // Measure maximum distance from the current vertical line.
                        if (cp.contour_not_taken_length_prev > 0.5 * line_spacing)
                            arches.push_back({ &cp, cp.contour_not_taken_length_prev, false });
                    } else {
                        // End point on the other line.
                        if (cp.contour_not_taken_length_prev > min_arch_length)
                            arches.push_back({ &cp, cp.contour_not_taken_length_prev, false });
                    }
                }
                if (cp.next_trimmed && cp.could_take_next()) {
                    //FIXME trace the trimmed line to decide what priority to assign to it.
                    const Point &pt   = graph.point(cp);
                    const Point &next = graph.point(*cp.next_on_contour);
                    if (std::abs(pt.x() - next.x()) < coord_t(0.5 * line_spacing)) {
                        // End point on the same line.
                        // Measure maximum distance from the current vertical line.
                        if (cp.contour_not_taken_length_next > 0.5 * line_spacing)
                            arches.push_back({ &cp, cp.contour_not_taken_length_next, true });
                    } else {
                        // End point on the other line.
                        if (cp.contour_not_taken_length_next > min_arch_length)
                            arches.push_back({ &cp, cp.contour_not_taken_length_next, true });
                    }
                }
            }
        }
        // Take the longest arch first.
        std::sort(arches.begin(), arches.end(), [](const auto &l, const auto &r) { return l.arc_length > r.arc_length; });
        // And connect along the arches.
        for (Arc &arc : arches)
            if (arc.take_next)
                take_next(*arc.intersection, true);
            else
                take_next(*arc.intersection->prev_on_contour, false);
    }
#endif

#ifdef INFILL_DEBUG_OUTPUT
    export_partial_infill_to_svg(debug_out_path("connect_base_support-arches-%03d.svg", iRun), graph, infill_ordered, polylines_out);
#endif // INFILL_DEBUG_OUTPUT

    // Traverse the unconnected lines in a zig-zag fashion, left to right only.
    for (ContourIntersectionPoint &cp : graph.map_infill_end_point_to_boundary) {
        assert(cp.contour_idx != boundary_idx_unconnected);
        if (cp.consumed)
            continue;
        bool first = ((&cp - graph.map_infill_end_point_to_boundary.data()) & 1) == 0;
        if (first) {
            // Only connect if the two lines are not connected by the same line already.
            if (get_and_update_merged_with(&cp) != get_and_update_merged_with(cp.next_on_contour))
                take_next(cp, true);
        } else {
            if (get_and_update_merged_with(&cp) != get_and_update_merged_with(cp.prev_on_contour))
                take_next(*cp.prev_on_contour, false);
        }
    }

#ifdef INFILL_DEBUG_OUTPUT
    export_partial_infill_to_svg(debug_out_path("connect_base_support-zigzag-%03d.svg", iRun), graph, infill_ordered, polylines_out);
#endif // INFILL_DEBUG_OUTPUT

    // Add the left caps.
    for (ContourIntersectionPoint &cp : graph.map_infill_end_point_to_boundary) {
        const bool                      first = ((&cp - graph.map_infill_end_point_to_boundary.data()) & 1) == 0;
        const ContourIntersectionPoint *other_end = first ? &cp + 1 : &cp - 1;
        const bool                      loop_next = cp.next_on_contour == other_end;
        const bool                      loop_prev = other_end->next_on_contour == &cp;
#ifndef NDEBUG
        const SupportArcCost           &cost_prev = arches[(&cp - graph.map_infill_end_point_to_boundary.data()) * 2];
        const SupportArcCost           &cost_next = *(&cost_prev + 1);
        assert(cost_prev.self_loop == loop_prev);
        assert(cost_next.self_loop == loop_next);
#endif // NDEBUG
        if (loop_prev && cp.could_take_prev())
            take_next(*cp.prev_on_contour, false);
        if (loop_next && cp.could_take_next())
            take_next(cp, true);
    }

#ifdef INFILL_DEBUG_OUTPUT
    export_partial_infill_to_svg(debug_out_path("connect_base_support-caps-%03d.svg", iRun), graph, infill_ordered, polylines_out);
#endif // INFILL_DEBUG_OUTPUT

    // Connect with T joints using long arches. Loops could be created only if a very long arc has to be added.
    {
        std::vector<const SupportArcCost*> candidates;
        for (ContourIntersectionPoint &cp : graph.map_infill_end_point_to_boundary) {
            if (cp.could_take_prev())
                candidates.emplace_back(&arches[(&cp - graph.map_infill_end_point_to_boundary.data()) * 2]);
            if (cp.could_take_next())
                candidates.emplace_back(&arches[(&cp - graph.map_infill_end_point_to_boundary.data()) * 2 + 1]);
        }
        std::sort(candidates.begin(), candidates.end(), [](auto *c1, auto *c2) { return c1->cost > c2->cost; });
        for (const SupportArcCost *candidate : candidates) {
            ContourIntersectionPoint &cp   = graph.map_infill_end_point_to_boundary[(candidate - arches.data()) / 2];
            bool                      prev = ((candidate - arches.data()) & 1) == 0;
            if (prev) {
                if (cp.could_take_prev() && (get_and_update_merged_with(&cp) != get_and_update_merged_with(cp.prev_on_contour) || candidate->cost > cost_high))
                    take_next(*cp.prev_on_contour, false);
            } else {
                if (cp.could_take_next() && (get_and_update_merged_with(&cp) != get_and_update_merged_with(cp.next_on_contour) || candidate->cost > cost_high))
                    take_next(cp, true);
            }
        }
    }

#ifdef INFILL_DEBUG_OUTPUT
    export_partial_infill_to_svg(debug_out_path("connect_base_support-Tjoints-%03d.svg", iRun), graph, infill_ordered, polylines_out);
#endif // INFILL_DEBUG_OUTPUT

    // Add very long arches and reasonably long caps even if both of its end points were already consumed.
    const double cap_cost = 0.5 * line_spacing;
    for (ContourIntersectionPoint &cp : graph.map_infill_end_point_to_boundary) {
        const SupportArcCost &cost_prev = arches[(&cp - graph.map_infill_end_point_to_boundary.data()) * 2];
        const SupportArcCost &cost_next = *(&cost_prev + 1);
        if (cp.contour_not_taken_length_prev > SCALED_EPSILON && 
            (cost_prev.self_loop ?
                cost_prev.cost > cap_cost :
                cost_prev.cost > cost_veryhigh)) {
            assert(cp.consumed && (cp.prev_on_contour->consumed || cp.prev_trimmed));
            Polyline pl { graph.point(cp) };
            if (! cp.prev_trimmed) {
                cp.trim_prev(cp.contour_not_taken_length_prev - line_half_width);
                cp.prev_on_contour->trim_next(0);
            }
            if (cp.contour_not_taken_length_prev > SCALED_EPSILON) {
                take_cw_limited(pl, graph.boundary[cp.contour_idx], graph.boundary_params[cp.contour_idx], cp.point_idx, cp.prev_on_contour->point_idx, cp.contour_not_taken_length_prev);
                cp.trim_prev(0);
                pl.clip_start(line_half_width);
                polylines_out.emplace_back(std::move(pl));
            }
        }
        if (cp.contour_not_taken_length_next > SCALED_EPSILON && 
            (cost_next.self_loop ?
                cost_next.cost > cap_cost :
                cost_next.cost > cost_veryhigh)) {
            assert(cp.consumed && (cp.next_on_contour->consumed || cp.next_trimmed));
            Polyline pl { graph.point(cp) };
            if (! cp.next_trimmed) {
                cp.trim_next(cp.contour_not_taken_length_next - line_half_width);
                cp.next_on_contour->trim_prev(0);
            }
            if (cp.contour_not_taken_length_next > SCALED_EPSILON) {
                take_ccw_limited(pl, graph.boundary[cp.contour_idx], graph.boundary_params[cp.contour_idx], cp.point_idx, cp.next_on_contour->point_idx, cp.contour_not_taken_length_next); // line_half_width);
                cp.trim_next(0);
                pl.clip_start(line_half_width);
                polylines_out.emplace_back(std::move(pl));
            }
        }
    }

#ifdef INFILL_DEBUG_OUTPUT
    export_partial_infill_to_svg(debug_out_path("connect_base_support-final-%03d.svg", iRun), graph, infill_ordered, polylines_out);
#endif // INFILL_DEBUG_OUTPUT

    polylines_out.reserve(polylines_out.size() + std::count_if(infill_ordered.begin(), infill_ordered.end(), [](const Polyline &pl) { return ! pl.empty(); }));
    for (Polyline &pl : infill_ordered)
        if (! pl.empty())
            polylines_out.emplace_back(std::move(pl));
}

void Fill::connect_base_support(Polylines &&infill_ordered, const Polygons &boundary_src, const BoundingBox &bbox, Polylines &polylines_out, const double spacing, const FillParams &params)
{
    auto polygons_src = reserve_vector<const Polygon*>(boundary_src.size());
    for (const Polygon &polygon : boundary_src)
        polygons_src.emplace_back(&polygon);

    connect_base_support(std::move(infill_ordered), polygons_src, bbox, polylines_out, spacing, params);
}

} // namespace Slic3r
