#include "../ClipperUtils.hpp"
#include "../ShortestPath.hpp"
#include "../Surface.hpp"

#include "FillCross.hpp"

#include <cmath>

namespace Slic3r {

namespace {

// The imported Cross implementation works in integer micrometres, while the
// rest of libslic3r uses coord_t units. Keep the physical mm <-> um conversion
// separate from the configurable coord_t <-> mm scale.
constexpr double  MICRONS_PER_MM               = 1000.0;
constexpr coord_t CROSS_3D_POCKET_SIZE_MICRONS = 200;

double internal_to_microns_factor()
{
    return SCALING_FACTOR * MICRONS_PER_MM;
}

double microns_to_internal_factor()
{
    return 1.0 / internal_to_microns_factor();
}

coord_t millimeters_to_microns(double millimeters)
{
    return static_cast<coord_t>(std::llround(millimeters * MICRONS_PER_MM));
}

} // namespace

void FillCross::_fill_surface_single(
    const FillParams                &params, 
    unsigned int                     thickness_layers,
    const std::pair<float, Point>   &direction, 
    ExPolygon                        expolygon,
    Polylines                       &polylines_out)
{
    
   Polygon cross_pattern_polygon = m_cross_fill_provider->generate(
       m_pattern,
       millimeters_to_microns(z),
       millimeters_to_microns(params.flow.width()),
       CROSS_3D_POCKET_SIZE_MICRONS);
   cross_pattern_polygon.scale(microns_to_internal_factor());

   for (Point& apoint: cross_pattern_polygon.points)
   {
       apoint -= m_offset;
   }

   Slic3r::Polygons apolygons = Slic3r::intersection(expolygon, ExPolygon(cross_pattern_polygon));
   polylines_out = Slic3r::to_polylines(apolygons);
}

void FillCross::set_cross_fill_provider(BoundingBox& abox,const Point& offset, InfillPattern _pattern, const float infill_line_distance, const float sparse_infill_line_width)
{
    m_pattern = _pattern;
    m_offset = offset;
    BoundingBox _abox = abox;
    _abox.max += offset;
    _abox.min += offset;
    _abox.scale(internal_to_microns_factor());
    m_cross_fill_provider.reset(new Cross::SierpinskiFillProvider(_abox, infill_line_distance, sparse_infill_line_width));
}
} // namespace Slic3r
