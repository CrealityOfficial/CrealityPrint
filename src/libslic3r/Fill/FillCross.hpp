#ifndef slic3r_FillCross_hpp_
#define slic3r_FillCross_hpp_

#include <map>

#include "../libslic3r.h"

#include "FillBase.hpp"
#include "Cross/SierpinskiFillProvider.h"

namespace Slic3r {

class FillCross : public Fill
{
public:
    ~FillCross() override {}

	void set_cross_fill_provider(BoundingBox& abox, const Point& offset, InfillPattern _pattern, const float infill_line_distance, const float sparse_infill_line_width);
protected:
    Fill* clone() const override { return new FillCross(*this); };
	void _fill_surface_single(
	    const FillParams                &params, 
	    unsigned int                     thickness_layers,
	    const std::pair<float, Point>   &direction, 
	    ExPolygon                 		 expolygon,
	    Polylines                       &polylines_out) override;

private:
	std::shared_ptr<Cross::SierpinskiFillProvider> m_cross_fill_provider;
	InfillPattern m_pattern{ipCross};
	Point m_offset{0,0};
};

} // namespace Slic3r::Cross

#endif // slic3r_FillCross_hpp_
