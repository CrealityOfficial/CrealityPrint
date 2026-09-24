#ifndef slic3r_FillTpmsGradual_hpp_
#define slic3r_FillTpmsGradual_hpp_

#include "../libslic3r.h"
#include "FillBase.hpp"
#include"FillTpmsD.hpp"

namespace Slic3r {

namespace MarchingSquares {
struct Point
{
    double x, y;
};

// Extract contour lines from a scalar field using marching squares algorithm.
void drawContour(double                                            contourValue,
                 int                                               gridSize_w,
                 int                                               gridSize_h,
                 std::vector<std::vector<double>>&                 data,
                 std::vector<std::vector<MarchingSquares::Point>>& posxy,
                 Polylines&                                        repls);
}

class TpmsGradual : public Fill
{
public:
    TpmsGradual();
    Fill* clone() const override { return new TpmsGradual(*this); }

    // require bridge flow since most of this pattern hangs in air
    bool use_bridge_flow() const override { return false; }

    // Correction applied to regular infill angle to maximize printing
    // speed in default configuration (degrees)
    static constexpr float CorrectionAngle = -45.;

    // Density adjustment to have a good %of weight.
    static constexpr double DensityAdjust = 2.44;

    // Gyroid upper resolution tolerance (mm^-2)
    static constexpr double PatternTolerance = 0.2;

    bool is_self_crossing() override { return false; }

    virtual void cal_scalar_field(const FillParams&                                 params,
                                  std::vector<std::vector<MarchingSquares::Point>>& posxy,
                                  std::vector<std::vector<double>>&                 data);

protected:
    void _fill_surface_single_brige(
        const FillParams                &params, 
        unsigned int                     thickness_layers,
        const std::pair<float, Point>   &direction, 
        ExPolygon                        expolygon,
        Polylines                       &polylines_out);
    void _fill_surface_single(const FillParams&              params,
                                    unsigned int                   thickness_layers,
                                    const std::pair<float, Point>& direction,
                                    ExPolygon                      expolygon,
                                    Polylines&                     polylines_out) override;
};

class FillTpmsGradual : public TpmsGradual
{
public:
    FillTpmsGradual(size_t lattice_Id) { Lattice_Id = lattice_Id; };
    Fill* clone() const override { return new FillTpmsGradual(*this); }
    // type of Lattice   0-g; 1-d; 2-p; 3-Fischer - Koch S; 
    size_t Lattice_Id;

    virtual void cal_scalar_field(const FillParams&                                 params,
                                  std::vector<std::vector<MarchingSquares::Point>>& posxy,
                                  std::vector<std::vector<double>>&                 data);

};

} // namespace Slic3r


#endif // slic3r_FillTpmsGradual_hpp_
