#include "../ClipperUtils.hpp"
#include "../ShortestPath.hpp"
#include "../Surface.hpp"

#include "FillCross.hpp"

namespace Slic3r {

void FillCross::_fill_surface_single(
    const FillParams                &params, 
    unsigned int                     thickness_layers,
    const std::pair<float, Point>   &direction, 
    ExPolygon                        expolygon,
    Polylines                       &polylines_out)
{
    
   Polygon cross_pattern_polygon = m_cross_fill_provider->generate(m_pattern, z*1000, params.flow.width()*1000, 200);
   cross_pattern_polygon.scale(1000);

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
    _abox.scale(0.001);
    m_cross_fill_provider.reset(new Cross::SierpinskiFillProvider(_abox, infill_line_distance, sparse_infill_line_width));
}
} // namespace Slic3r
