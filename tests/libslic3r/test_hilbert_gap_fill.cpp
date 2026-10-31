#include <catch2/catch.hpp>

#include "libslic3r/Fill/FillPlanePath.hpp"
#include "libslic3r/ClipperUtils.hpp"
#include "libslic3r/PrintBase.hpp"
#include "libslic3r/Surface.hpp"

#include <atomic>

using namespace Slic3r;

namespace {
class HilbertWithGapFill : public FillHilbertCurve {
public:
    using FillHilbertCurve::generate;
    using Fill::_create_gap_fill;
};

// The shared gap algorithm without Hilbert-specific filtering is our reference.
class ReferenceGapFill : public FillArchimedeanChords {
public:
    using Fill::_create_gap_fill;
};

// Supply exact coverage so the tests can exercise a known residual shape
// through the real opening, medial-axis and variable-width extrusion stages.
class KnownGapCoverage : public ExtrusionEntityCollection {
public:
    explicit KnownGapCoverage(Polygons coverage) : m_coverage(std::move(coverage)) {}
    void polygons_covered_by_spacing(Polygons &out, float) const override
    {
        polygons_append(out, m_coverage);
    }
private:
    Polygons m_coverage;
};

ExPolygon gap_rectangle(double width, double height)
{
    return ExPolygon(Polygon{Point::new_scale(0, 0), Point::new_scale(width, 0),
        Point::new_scale(width, height), Point::new_scale(0, height)});
}

template<class Filler>
ExtrusionEntityCollection fill_test_gaps(const ExPolygon &domain, const ExPolygons &gaps,
                                        const FillParams &params, const PrintObjectConfig &object)
{
    Filler fill;
    fill.print_object_config = &object;
    fill.no_overlap_expolygons = {domain};
    Surface surface(stTop, domain);
    ExtrusionEntityCollection result;
    result.entities.push_back(new KnownGapCoverage(to_polygons(diff_ex(ExPolygons{domain}, gaps))));
    fill._create_gap_fill(&surface, params, &result);
    return result;
}

void check_same_gap_extrusion(const ExtrusionEntityCollection &actual, const ExtrusionEntityCollection &expected,
                              bool reference_has_paths = true)
{
    const Polylines a = actual.as_polylines();
    const Polylines b = expected.as_polylines();
    REQUIRE(b.empty() == !reference_has_paths);
    REQUIRE(a.size() == b.size());
    for (size_t i = 0; i < a.size(); ++i)
        CHECK(a[i].points == b[i].points);
    CHECK(actual.total_volume() == Approx(expected.total_volume()));
}
}

TEST_CASE("Hilbert cancellation interrupts generation and parallel clipping", "[Infill][Hilbert]")
{
    HilbertWithGapFill fill;
    SECTION("Curve generation")
    {
        size_t checks = 0;
        fill.set_cancel_callback([&] { if (++checks == 2) throw CanceledException(); });
        InfillPolylineOutput output(1000.);
        REQUIRE_THROWS_AS(fill.generate(0, 0, 1023, 1023, 0., output), CanceledException);
        CHECK(output.result().size() < 1024 * 1024);
    }
    SECTION("Parallel clipping and subsequent recovery")
    {
        InfillPolylineOutput output(1000.);
        fill.generate(0, 0, 63, 63, 0., output);
        Polyline curve;
        curve.points = std::move(output.result());
        ExPolygon boundary(Polygon{{-1000, -1000}, {64000, -1000}, {64000, 64000}, {-1000, 64000}});
        std::atomic<size_t> checks{0};
        const std::function<void()> cancel = [&] {
            if (checks.fetch_add(1) >= 3)
                throw CanceledException();
        };
        REQUIRE_THROWS_AS(Hilbert::clip(curve, boundary, 128, cancel), CanceledException);
        const Polylines recovered = Hilbert::clip(curve, boundary, 128);
        REQUIRE(recovered.size() == 1);
        const Points reversed(curve.points.rbegin(), curve.points.rend());
        CHECK((recovered.front().points == curve.points || recovered.front().points == reversed));
    }
    SECTION("Gap filtering cancellation and recovery")
    {
        ExPolygons gaps{gap_rectangle(.08, 4.)};
        const ExPolygons domain{gap_rectangle(10., 10.)};
        size_t checks = 0;
        REQUIRE_THROWS_AS(Hilbert::filter_corner_gaps(gaps, domain, .4,
            [&] { if (++checks == 2) throw CanceledException(); }), CanceledException);
        Hilbert::filter_corner_gaps(gaps, domain, .4);
        REQUIRE(gaps.size() == 1);
    }
}

TEST_CASE("Hilbert gap filtering preserves thin gaps and boundary compensation", "[Infill][Hilbert]")
{
    const float nozzle = GENERATE(0.2f, 0.4f, 0.6f);
    const float height_ratio = GENERATE(0.25f, 0.75f);
    const float width_ratio = GENERATE(0.8f, 1.05f);
    CAPTURE(nozzle, height_ratio, width_ratio);
    PrintObjectConfig object;
    object.gap_fill_target.value = gftEverywhere;
    PrintRegionConfig region;
    FillParams params;
    params.density = 1.;
    params.flow = Flow(nozzle * width_ratio, nozzle * height_ratio, nozzle);
    params.extrusion_role = erTopSolidInfill;
    params.config = &region;
    ExPolygon domain = gap_rectangle(20. * nozzle, 20. * nozzle);
    ExPolygon gap;
    bool filtered = false;
    bool reference_has_paths = true;
    SECTION("Small non-triangular gaps retain compensation")
    {
        gap = gap_rectangle(0.18 * nozzle, 0.45 * nozzle);
        gap.translate(scale_(10. * nozzle), scale_(10. * nozzle));
    }
    SECTION("Small triangular turn residual")
    {
        const double angle = GENERATE(0., PI / 4., PI / 2.);
        CAPTURE(angle);
        gap = ExPolygon(Polygon{Point::new_scale(0, 0), Point::new_scale(0.39 * nozzle, 0),
            Point::new_scale(0, 0.39 * nozzle)});
        gap.rotate(angle);
        gap.translate(scale_(10. * nozzle), scale_(10. * nozzle));
        filtered = true;
        reference_has_paths = false; // The legacy medial axis also drops these short branches.
    }
    SECTION("Long thin gaps retain the original width limit at every angle")
    {
        const double angle = GENERATE(0., PI / 4., PI / 2.);
        CAPTURE(angle);
        gap = gap_rectangle(0.2 * nozzle, 10. * nozzle);
        gap.translate(scale_(-0.1 * nozzle), scale_(-5. * nozzle));
        gap.rotate(angle);
        gap.translate(scale_(10. * nozzle), scale_(10. * nozzle));
    }
    SECTION("A rotated sliver is not mistaken for a compact speck")
    {
        gap = gap_rectangle(0.13 * nozzle, 0.52 * nozzle);
        gap.rotate(PI / 4.);
        gap.translate(scale_(10. * nozzle), scale_(10. * nozzle));
    }
    SECTION("Long triangular gaps retain compensation")
    {
        gap = ExPolygon(Polygon{Point::new_scale(0, 0), Point::new_scale(5. * nozzle, 0),
            Point::new_scale(0, 0.4 * nozzle)});
        gap.translate(scale_(8. * nozzle), scale_(8. * nozzle));
    }
    SECTION("A larger compact gap retains compensation")
    {
        gap = gap_rectangle(0.3 * nozzle, 0.8 * nozzle);
        gap.translate(scale_(10. * nozzle), scale_(10. * nozzle));
    }
    SECTION("Small gaps at the outside boundary retain compensation")
    {
        gap = gap_rectangle(0.18 * nozzle, 0.45 * nozzle);
        gap.translate(0, scale_(10. * nozzle));
    }
    SECTION("Small gaps beside a hole retain compensation")
    {
        ExPolygon hole = gap_rectangle(4. * nozzle, 4. * nozzle);
        hole.translate(scale_(8. * nozzle), scale_(8. * nozzle));
        hole.contour.make_clockwise();
        domain.holes.push_back(hole.contour);
        gap = gap_rectangle(0.18 * nozzle, 0.45 * nozzle);
        gap.translate(scale_(7.82 * nozzle), scale_(10. * nozzle));
    }
    SECTION("A triangular residual near the outside edge is not filtered")
    {
        gap = ExPolygon(Polygon{Point::new_scale(0, 0), Point::new_scale(0.39 * nozzle, 0),
            Point::new_scale(0, 0.39 * nozzle)});
        gap.translate(0, scale_(10. * nozzle));
        reference_has_paths = false;
    }
    SECTION("A triangular residual near a hole is not filtered")
    {
        ExPolygon hole = gap_rectangle(4. * nozzle, 4. * nozzle);
        hole.translate(scale_(8. * nozzle), scale_(8. * nozzle));
        hole.contour.make_clockwise();
        domain.holes.push_back(hole.contour);
        gap = ExPolygon(Polygon{Point::new_scale(0, 0), Point::new_scale(0.39 * nozzle, 0),
            Point::new_scale(0, 0.39 * nozzle)});
        gap.translate(scale_(7.61 * nozzle), scale_(10. * nozzle));
        reference_has_paths = false;
    }
    SECTION("Gaps surrounding an island retain compensation")
    {
        gap = gap_rectangle(0.48 * nozzle, 0.48 * nozzle);
        ExPolygon hole = gap_rectangle(0.12 * nozzle, 0.12 * nozzle);
        hole.translate(scale_(0.18 * nozzle), scale_(0.18 * nozzle));
        hole.contour.make_clockwise();
        gap.holes.push_back(hole.contour);
        gap.translate(scale_(10. * nozzle), scale_(10. * nozzle));
    }
    ExPolygons regions{gap};
    Hilbert::filter_corner_gaps(regions, {domain}, std::min(params.flow.width(), nozzle));
    REQUIRE(regions.size() == (filtered ? 0 : 1));
    if (!filtered) {
        CHECK(regions.front().contour.points == gap.contour.points);
        REQUIRE(regions.front().holes.size() == gap.holes.size());
        for (size_t i = 0; i < gap.holes.size(); ++i)
            CHECK(regions.front().holes[i].points == gap.holes[i].points);
    }
    const auto expected = fill_test_gaps<ReferenceGapFill>(domain, {gap}, params, object);
    const auto actual = fill_test_gaps<HilbertWithGapFill>(domain, {gap}, params, object);
    if (filtered) {
        CHECK(expected.as_polylines().empty() == !reference_has_paths);
        CHECK(actual.as_polylines().empty());
    } else {
        check_same_gap_extrusion(actual, expected, reference_has_paths);
    }
}

TEST_CASE("Full Hilbert extrusion preserves primary paths when gap filling is enabled", "[Infill][Hilbert]")
{
    PrintObjectConfig object;
    object.gap_fill_target.value = gftEverywhere;
    PrintRegionConfig region;
    region.top_surface_pattern.value = ipHilbertCurve;
    HilbertWithGapFill fill;
    fill.print_object_config = &object;
    fill.spacing = 0.37707963267948963;
    fill.angle = float(PI / 4.);
    fill.layer_id = 4;
    Surface surface(stTop, gap_rectangle(24., 24.));
    surface.expolygon.holes.emplace_back(Polygon{
        Point::new_scale(8., 8.), Point::new_scale(8., 16.),
        Point::new_scale(16., 16.), Point::new_scale(16., 8.)});
    fill.bounding_box = get_extents(surface.expolygon);
    fill.no_overlap_expolygons = {surface.expolygon};
    FillParams params;
    params.density = 1.;
    params.pattern = ipHilbertCurve;
    params.flow = Flow(0.42f, 0.2f, 0.4f);
    params.extrusion_role = erTopSolidInfill;
    params.config = &region;

    ExtrusionEntityCollection without_gaps, with_gaps;
    params.enable_gap_fill = false;
    fill.fill_surface_extrusion(&surface, params, without_gaps.entities);
    params.enable_gap_fill = true;
    fill.fill_surface_extrusion(&surface, params, with_gaps.entities);
    Polylines primary;
    const auto flattened = with_gaps.flatten();
    for (const ExtrusionEntity *entity : flattened.entities)
        if (entity->role() != erGapFill)
            entity->collect_polylines(primary);
    const Polylines expected = without_gaps.as_polylines();
    REQUIRE(!primary.empty());
    REQUIRE(primary.size() == expected.size());
    for (size_t i = 0; i < primary.size(); ++i)
        CHECK(primary[i].points == expected[i].points);
}
