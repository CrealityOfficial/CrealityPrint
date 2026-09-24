#include "../ClipperUtils.hpp"
#include "../ShortestPath.hpp"
#include "../Surface.hpp"
#include "FillField.hpp"
#include "FillTpmsGradual.hpp"

#include <cmath>
#include <algorithm>
#include <vector>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

// FillTensor precompiled library interface.
#include "cr_FillTensor_library.h"

namespace Slic3r {

FillField::~FillField() {}

void FillField::_fill_surface_single(const FillParams&              params,
                                     unsigned int                   thickness_layers,
                                     const std::pair<float, Point>& direction,
                                     ExPolygon                      expolygon,
                                     Polylines&                     polylines_out)
{
    if (params.extrusion_role != erInternalInfill) {
        _fill_surface_single_brige(params, thickness_layers, direction, expolygon, polylines_out);
        return;
    }

    // field_sdf_grid stores the FillTensorHandle precomputed in PrintObject.
    FillTensorHandle tensor_handle = static_cast<FillTensorHandle>(this->field_sdf_grid);
    if (!tensor_handle)
        return;

    BoundingBox bb      = expolygon.contour.bounding_box();
    auto        cenpos  = unscale(bb.center());
    auto        boxsize = unscale(bb.size());
    float       xlen    = boxsize.x();
    float       ylen    = boxsize.y();
    float       delta   = 0.5f;

    // Build the 2D sampling grid for this layer section.
    std::vector<std::vector<MarchingSquares::Point>> posxy;
    int                                              i = 0, j = 0;
    for (float y = -(ylen) / 2.0f - 2; y < (ylen) / 2.0f + 2; y += delta, i++) {
        j = 0;
        std::vector<MarchingSquares::Point> colposxy;
        for (float x = -(xlen) / 2.0f - 2; x < (xlen) / 2.0f + 2; x += delta, j++) {
            MarchingSquares::Point pt;
            pt.x = cenpos.x() + x;
            pt.y = cenpos.y() + y;
            colposxy.push_back(pt);
        }
        posxy.push_back(colposxy);
    }

    int width  = j;
    int height = i;

    // Batch-query the scalar field from the DLL.
    int                total = height * width;
    std::vector<float> points(total * 3);
    std::vector<float> results(total);
    float              curz = this->z;

    for (int ii = 0; ii < height; ++ii)
        for (int jj = 0; jj < width; ++jj) {
            int idx             = ii * width + jj;
            points[idx * 3 + 0] = (float) posxy[ii][jj].x;
            points[idx * 3 + 1] = (float) posxy[ii][jj].y;
            points[idx * 3 + 2] = curz;
        }

    FillTensor_QueryBatch(tensor_handle, points.data(), results.data(), total);

    // Convert the flat query result to a 2D array for MarchingSquares.
    std::vector<std::vector<double>> data(height, std::vector<double>(width, 0.0));
    for (int ii = 0; ii < height; ++ii)
        for (int jj = 0; jj < width; ++jj)
            data[ii][jj] = results[ii * width + jj];

    // Extract the zero level set as infill polylines.
    Polylines polylines;
    MarchingSquares::drawContour(0, width, height, data, posxy, polylines);

    polylines = intersection_pl(polylines, expolygon);

    if (!polylines.empty()) {
        const double minlength = scale_(0.8 * this->spacing);
        polylines.erase(std::remove_if(polylines.begin(), polylines.end(),
                                       [minlength](const Polyline& pl) { return pl.length() < minlength; }),
                        polylines.end());
    }

    if (!polylines.empty()) {
        if (params.dont_connect())
            append(polylines_out, chain_polylines(polylines));
        else
            this->connect_infill(std::move(polylines), expolygon, polylines_out, this->spacing, params);
    }
}

void FillField::_fill_surface_single_brige(const FillParams&              params,
                                           unsigned int                   thickness_layers,
                                           const std::pair<float, Point>& direction,
                                           ExPolygon                      expolygon,
                                           Polylines&                     polylines_out)
{
    float infill_angle = float(this->angle + (-45.f * 2 * M_PI) / 360.f);
    if (std::abs(infill_angle) >= EPSILON)
        expolygon.rotate(-infill_angle);

    BoundingBox bb               = expolygon.contour.bounding_box();
    double      density_adjusted = std::max(0., params.density * 2.44);
    coord_t     distance         = coord_t(scale_(this->spacing) / density_adjusted);

    bb.merge(align_to_grid(bb.min, Point(2 * M_PI * distance, 2 * M_PI * distance)));

    double scaleFactor = scale_(this->spacing) / density_adjusted;
    double z           = scale_(this->z) / scaleFactor;
    double z_sin = std::sin(z), z_cos = std::cos(z);

    double width  = std::ceil(bb.size()(0) / distance) + 1.;
    double height = std::ceil(bb.size()(1) / distance) + 1.;

    Polylines polylines;
    bool      vertical = (std::abs(z_sin) <= std::abs(z_cos));
    double    step     = M_PI;
    double    upper    = vertical ? width : height;

    for (double offset = 0; offset < upper; offset += step) {
        Polyline pl;
        if (vertical) {
            pl.points.push_back(Point(coord_t(offset * scaleFactor), 0));
            pl.points.push_back(Point(coord_t(offset * scaleFactor), coord_t(height * scaleFactor)));
        } else {
            pl.points.push_back(Point(0, coord_t(offset * scaleFactor)));
            pl.points.push_back(Point(coord_t(width * scaleFactor), coord_t(offset * scaleFactor)));
        }
        polylines.push_back(pl);
    }

    for (Polyline& pl : polylines)
        pl.translate(bb.min);

    polylines = intersection_pl(polylines, expolygon);

    if (!polylines.empty()) {
        const double minlength = scale_(0.8 * this->spacing);
        polylines.erase(std::remove_if(polylines.begin(), polylines.end(),
                                       [minlength](const Polyline& pl) { return pl.length() < minlength; }),
                        polylines.end());
    }

    if (!polylines.empty()) {
        size_t polylines_out_first_idx = polylines_out.size();
        if (params.dont_connect())
            append(polylines_out, chain_polylines(polylines));
        else
            this->connect_infill(std::move(polylines), expolygon, polylines_out, this->spacing, params);

        if (std::abs(infill_angle) >= EPSILON) {
            for (auto it = polylines_out.begin() + polylines_out_first_idx; it != polylines_out.end(); ++it)
                it->rotate(infill_angle);
        }
    }
}

} // namespace Slic3r
