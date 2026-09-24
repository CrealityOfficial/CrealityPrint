#ifndef slic3r_FillField_hpp_
#define slic3r_FillField_hpp_

#include <utility>
#include <vector>

#include "libslic3r/libslic3r.h"
#include "FillBase.hpp"
#include "libslic3r/ExPolygon.hpp"
#include "libslic3r/Polyline.hpp"

namespace Slic3r {

namespace MarchingSquares {
struct Point;
}

/// FillField: tensor-field-driven infill based on an OpenVDB distance field.
///
/// All expensive preprocessing is done in PrintObject:
///   1. Convert the mesh to an OpenVDB SDF.
///   2. Sample the SDF and pass it to FillTensorDll to compute the field.
///   3. Store the computed FillTensorHandle in m_field_sdf_grid.
///
/// During infill generation, querying is thread-safe and read-only:
///   field_sdf_grid is the FillTensorHandle and is queried with QueryBatch.
class FillField : public Fill
{
public:
    FillField() {}
    ~FillField() override;
    Fill* clone() const override { return new FillField(*this); }

    bool use_bridge_flow() const override { return false; }
    bool is_self_crossing() override { return false; }

    void _fill_surface_single(const FillParams&              params,
                              unsigned int                   thickness_layers,
                              const std::pair<float, Point>& direction,
                              ExPolygon                      expolygon,
                              Polylines&                     polylines_out) override;

private:
    void _fill_surface_single_brige(const FillParams&              params,
                                    unsigned int                   thickness_layers,
                                    const std::pair<float, Point>& direction,
                                    ExPolygon                      expolygon,
                                    Polylines&                     polylines_out);
};

} // namespace Slic3r

#endif // slic3r_FillField_hpp_
