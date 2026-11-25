#ifndef SLIC3R_ARRANGE_GRID_HPP
#define SLIC3R_ARRANGE_GRID_HPP

#include "Arrange.hpp"

namespace Slic3r { namespace arrangement {
// Fast single-bed placement with fixed obstacles in bed-local coordinates.
// Rotations are preserved; inflated bounding boxes define the grid cells.
void arrange_grid(ArrangePolygons& items, const ArrangePolygons& excludes,
                  const Points& bed, const ArrangeParams& params);
}}

#endif
