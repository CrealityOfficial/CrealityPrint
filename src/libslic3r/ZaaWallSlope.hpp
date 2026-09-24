#ifndef slic3r_ZaaWallSlope_hpp_
#define slic3r_ZaaWallSlope_hpp_

#include "I18N.hpp"
#include "ZAA.hpp"

#include <cmath>
#include <optional>

namespace Slic3r {

// The surface inclination is measured from XY, not along the extrusion tangent.
// A local plane predicts horizontal contour retreat d = slice_spacing / tan(theta).
// This first calibration follows the user's 0.20 mm-layer prints: 30 degrees
// has a small benefit (d=0.3464 mm), while 35 degrees does not (d=0.2856 mm).
// Width and the displacement sign do not determine eligibility.
class ZaaWallSlopePolicy {
public:
    static constexpr double min_step_width_mm = 0.34;
    // Preserve the existing smooth transition over the last 10% of the
    // normalized tangent range. Express its endpoints in step-width units.
    static constexpr double full_step_width_mm = min_step_width_mm / 0.9;

    ZaaWallSlopePolicy(const ZaaLayerGeometry &layer, double path_width_mm)
    {
        m_slice_spacing_mm = layer.query_upper_z_mm - layer.query_lower_z_mm;
        const double s = layer.max_delta_mm();
        if (!std::isfinite(m_slice_spacing_mm) || !std::isfinite(s) || m_slice_spacing_mm <= 0.0 ||
            s < 0.0 || s >= m_slice_spacing_mm ||
            !std::isfinite(path_width_mm) || path_width_mm <= 0.0 ||
            !std::isfinite(layer.outer_wall_width_mm) || layer.outer_wall_width_mm < 0.0)
            throw InvalidArgument(_u8L("ZAA wall step filtering requires valid layer geometry and positive width"));
        // Offset-plane query bounds are the current and next contour planes,
        // even with adaptive heights. At the final layer the upper bound is
        // the existing virtual continuation of that layer's slice interval.
        m_reference_width_mm = layer.outer_wall_width_mm > 0.0 ? layer.outer_wall_width_mm : path_width_mm;
    }

    double max_tangent() const { return m_slice_spacing_mm / min_step_width_mm; }
    double min_normal_z() const { return 1.0 / std::hypot(1.0, max_tangent()); }
    double reference_width_mm() const { return m_reference_width_mm; }

    // Flat caps have no finite sloping-step estimate. Handle them separately
    // without reporting an infinite score; preserve their existing ZAA behavior.
    std::optional<double> step_width_mm(double surface_tangent) const
    {
        if (!std::isfinite(surface_tangent) || surface_tangent <= 0.0)
            return std::nullopt;
        const double d = m_slice_spacing_mm / surface_tangent;
        return std::isfinite(d) ? std::optional<double>(d) : std::nullopt;
    }

    static double weight_for_step_width(double d)
    {
        if (!std::isfinite(d) || d <= min_step_width_mm)
            return 0.0;
        if (d >= full_step_width_mm)
            return 1.0;
        const double t = (1.0 - min_step_width_mm / d) / 0.1;
        return t * t * (3.0 - 2.0 * t);
    }

    double weight(double slope_degrees) const
    {
        if (!std::isfinite(slope_degrees) || slope_degrees < 0.0 || slope_degrees >= 90.0)
            return 0.0;
        if (slope_degrees == 0.0)
            return 1.0;
        constexpr double radians_per_degree = 3.14159265358979323846 / 180.0;
        const auto d = step_width_mm(std::tan(slope_degrees * radians_per_degree));
        return d ? weight_for_step_width(*d) : 0.0;
    }

private:
    double m_slice_spacing_mm{0.0};
    double m_reference_width_mm{0.0};
};

} // namespace Slic3r

#endif // slic3r_ZaaWallSlope_hpp_
