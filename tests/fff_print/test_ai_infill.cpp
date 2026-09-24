#include <catch2/catch.hpp>

#include <array>
#include <memory>

#include "libslic3r/ClipperUtils.hpp"
#include "libslic3r/Fill/Fill.hpp"
#include "libslic3r/Fill/FillAIInfill.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/Surface.hpp"

using namespace Slic3r;

namespace {

class ScopedScalingFactor
{
public:
    explicit ScopedScalingFactor(double value) : previous(SCALING_FACTOR) { SCALING_FACTOR = value; }
    ~ScopedScalingFactor() { SCALING_FACTOR = previous; }
    ScopedScalingFactor(const ScopedScalingFactor&) = delete;
    ScopedScalingFactor& operator=(const ScopedScalingFactor&) = delete;

private:
    double previous;
};

ExPolygon ai_square(double side)
{
    return ExPolygon(Points {
        Point::new_scale(100., 100.), Point::new_scale(100. + side, 100.),
        Point::new_scale(100. + side, 100. + side), Point::new_scale(100., 100. + side)
    });
}

Polylines ai_paths(const ExPolygon &polygon, InfillPattern pattern, bool enabled,
                  ExtrusionRole role = erInternalInfill)
{
    PrintRegionConfig config;
    config.intelligent_infill.value = enabled;
    std::unique_ptr<Fill> fill(Fill::new_from_type(pattern));
    fill->bounding_box = get_extents(polygon.contour);
    fill->angle = float(PI / 4.);
    fill->layer_id = 3;
    fill->z = 0.8;
    fill->spacing = 0.42;
    fill->overlap = 0.0;

    FillParams params;
    params.config = &config;
    params.pattern = pattern;
    params.density = 0.25f;
    params.extrusion_role = role;
    Surface surface(stInternal, polygon);
    return fill->fill_surface(&surface, params);
}

double length_mm(const Polylines &paths)
{
    double length = 0.;
    for (const Polyline &path : paths)
        length += path.length();
    return unscale<double>(length);
}

} // namespace

TEST_CASE("AI infill distances retain millimetre units across precision changes", "[Fill][AI]")
{
    // Returning to the first precision also catches cached scaled constants.
    for (double precision : { 1e-6, 1e-5, 1e-6 }) {
        ScopedScalingFactor scaling(precision);
        const auto thresholds = AIInfill::depth_thresholds();
        const std::array<double, 4> expected {{ -1., -1.5, -3., -5. }};
        REQUIRE(thresholds.size() == expected.size());
        for (size_t i = 0; i < expected.size(); ++i)
            CHECK(unscale<double>(thresholds[i]) == Approx(expected[i]).margin(1e-9));
        CHECK(unscale<double>(AIInfill::max_grid_edge()) == Approx(10.).margin(1e-9));
    }
}

TEST_CASE("AI infill removes interior material at both coordinate precisions", "[Fill][AI]")
{
    for (double precision : { 1e-6, 1e-5 }) {
        ScopedScalingFactor scaling(precision);
        // Rectilinear exercises the line implementation; grid exercises the
        // two-sweep implementation containing the second set of thresholds.
        for (InfillPattern pattern : { ipRectilinear, ipGrid }) {
            CAPTURE(precision, pattern);
            const ExPolygon polygon = ai_square(20.);
            const Polylines off = ai_paths(polygon, pattern, false);
            const Polylines on = ai_paths(polygon, pattern, true);
            REQUIRE_FALSE(off.empty());
            REQUIRE_FALSE(on.empty());
            CHECK(length_mm(on) < length_mm(off) - 0.01);
            CHECK(diff_pl(on, offset(polygon, float(scale_(0.002)))).empty());

            // AI may remove deep interior segments, not the perimeter-side
            // portions of the original fill lines.
            const ExPolygons interior = offset_ex(polygon, -float(scale_(1.0)));
            CHECK(length_mm(diff_pl(on, interior)) ==
                  Approx(length_mm(diff_pl(off, interior))).margin(0.02));
        }
    }
}

TEST_CASE("AI infill leaves shallow regions and non-infill roles unchanged", "[Fill][AI]")
{
    ScopedScalingFactor scaling(1e-5);
    for (InfillPattern pattern : { ipRectilinear, ipGrid }) {
        CAPTURE(pattern);
        const ExPolygon small = ai_square(2.);
        const Polylines off = ai_paths(small, pattern, false);
        const Polylines on = ai_paths(small, pattern, true);
        REQUIRE_FALSE(off.empty());
        CHECK(length_mm(on) == Approx(length_mm(off)).margin(0.002));

        const ExPolygon normal = ai_square(20.);
        const Polylines solid_off = ai_paths(normal, pattern, false, erSolidInfill);
        const Polylines solid_on = ai_paths(normal, pattern, true, erSolidInfill);
        REQUIRE_FALSE(solid_off.empty());
        CHECK(length_mm(solid_on) == Approx(length_mm(solid_off)).margin(0.002));
    }
}
