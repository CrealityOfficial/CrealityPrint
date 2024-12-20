#include "g2g3.h"
#include "Slice3rBase/ArcFitter.hpp"

namespace cura52
{
    bool do_arc_fitting(const std::vector<Point>& points, std::vector<PathFittingData>& result, double tolerance)
    {
        std::vector< Slic3r::Point> _points;
        std::vector<Slic3r::PathFittingData> _fitting_result;

        _points.resize(points.size());
        for (int i =0;i< points.size();i++)
        {
            _points[i] = Slic3r::Point((int64_t)points[i].X, (int64_t)points[i].Y);
        }

        Slic3r::ArcFitter::do_arc_fitting(_points, _fitting_result, tolerance);

        bool Large_arc_exist = false;
        for (size_t fitting_index = 0; fitting_index < _fitting_result.size(); fitting_index++)
        {
            if (_fitting_result[fitting_index].path_type == Slic3r::EMovePathType::Arc_move_cw
                || _fitting_result[fitting_index].path_type == Slic3r::EMovePathType::Arc_move_ccw)
            {
                const double arc_length = _fitting_result[fitting_index].arc_data.length;
                if (arc_length > 5000)
                {
                    Large_arc_exist = true;
                    break;
                }
            }
        }
        //if (Large_arc_exist)
        //{//大圆弧拟合需缩小公差
        //    _fitting_result.clear();
        //    tolerance = 1;
        //    Slic3r::ArcFitter::do_arc_fitting(_points, _fitting_result, tolerance);
        //}

        result.resize(_fitting_result.size());
        for (size_t i = 0; i < _fitting_result.size(); i++)
        {
            result[i].arc_data.center = Point(_fitting_result[i].arc_data.center.x(), _fitting_result[i].arc_data.center.y()); 
            result[i].arc_data.end_point = Point(_fitting_result[i].arc_data.end_point.x(), _fitting_result[i].arc_data.end_point.y());
            result[i].arc_data.length = _fitting_result[i].arc_data.length;
            result[i].arc_data.start_point = Point(_fitting_result[i].arc_data.start_point.x(), _fitting_result[i].arc_data.start_point.y());
            result[i].arc_data.direction = ArcDirection(_fitting_result[i].arc_data.direction);
            
            result[i].end_point_index = _fitting_result[i].end_point_index;
            result[i].path_type = EMovePathType(_fitting_result[i].path_type);
            result[i].start_point_index = _fitting_result[i].start_point_index;
        }

        return true;
    }
}