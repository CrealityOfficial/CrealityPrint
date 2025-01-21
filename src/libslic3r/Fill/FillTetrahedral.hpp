#ifndef slic3r_FillTetrahedral_hpp_
#define slic3r_FillTetrahedral_hpp_

#include "FillBase.hpp"

namespace Slic3r {

class FillTetrahedral : public Fill
{
public:
    ~FillTetrahedral() override = default;

protected:
    Fill* clone() const override { return new FillTetrahedral(*this); };
	void _fill_surface_single(
	    const FillParams                &params, 
	    unsigned int                     thickness_layers,
	    const std::pair<float, Point>   &direction, 
	    ExPolygon     		             expolygon,
	    Polylines                       &polylines_out) override;

	void _fill_surface_single(const FillParams& params,
		unsigned int                   thickness_layers,
		const std::pair<float, Point>& direction,
		ExPolygon                      expolygon,
		ThickPolylines& thick_polylines_out) override;

    bool no_sort() const override { return true; }
};

} // namespace Slic3r

#endif // slic3r_FillTetrahedral_hpp_
