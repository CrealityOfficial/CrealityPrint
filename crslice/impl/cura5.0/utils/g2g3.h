#ifndef G2G3_INFILL274874
#define G2G3_INFILL274874

#include "utils/polygon.h"

namespace cura52
{
    enum class ArcDirection 
    {
        Arc_Dir_unknow,
        Arc_Dir_CCW,
        Arc_Dir_CW,
        Count
    };

    enum class EMovePathType
    {
        Noop_move,
        Linear_move,
        Arc_move_cw,
        Arc_move_ccw,
        Count
    };

    class ArcSegment
    {
    public:
        double length = 0;
        Point start_point{ Point(0,0) };
        Point end_point{ Point(0,0) };
        Point center;
        ArcDirection direction = ArcDirection::Arc_Dir_unknow;
    };

    //BBS
    struct PathFittingData 
    {
        size_t start_point_index;
        size_t end_point_index;
        EMovePathType path_type;
        ArcSegment arc_data;
    };

    bool do_arc_fitting(const std::vector<Point>& points, std::vector<PathFittingData>& result, double tolerance);
}

#endif  