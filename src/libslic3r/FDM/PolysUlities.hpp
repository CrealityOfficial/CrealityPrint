#ifndef POLYSUTILITIES_HPP
#define POLYSUTILITIES_HPP
#include "libslic3r/ExPolygon.hpp"

namespace creality
{
    void contains(const std::vector<Slic3r::ExPolygons*>& sources, std::vector<int>& all_indices);
}
#endif // POLYSUTILITIES_HPP