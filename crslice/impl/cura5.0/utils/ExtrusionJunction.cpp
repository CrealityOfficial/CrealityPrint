//Copyright (c) 2022 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#include "utils/ExtrusionJunction.h"

namespace cura52
{

    bool ExtrusionJunction::operator ==(const ExtrusionJunction& other) const
    {
        return p == other.p
            && w == other.w
            && perimeter_index == other.perimeter_index
            && overhang_distance == other.overhang_distance;
    }

    ExtrusionJunction::ExtrusionJunction(const Point p, const coord_t w, const coord_t perimeter_index, const coord_t overhang_distance)
        : p(p),
        w(w),
        perimeter_index(perimeter_index),
        overhang_distance(overhang_distance)
    {}

}
