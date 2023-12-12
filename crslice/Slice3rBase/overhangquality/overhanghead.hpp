#ifndef slic3r_ExtrusionProcessor_QUALITYHEAD_hpp_
#define slic3r_ExtrusionProcessor_QUALITYHEAD_hpp_

#include "Slice3rBase/Point.hpp"

namespace Slic3r {

struct ExtendedPoint
{
    ExtendedPoint(Vec2d position, float distance = 0.0, size_t nearest_prev_layer_line = size_t(-1), float curvature = 0.0)
        : position(position), distance(distance), nearest_prev_layer_line(nearest_prev_layer_line), curvature(curvature)
    {}

    Vec2d  position;
    float  distance;
    size_t nearest_prev_layer_line;
    float  curvature;
};


} // namespace Slic3r

#endif // slic3r_ExtrusionProcessor_QUALITYHEAD_hpp_


