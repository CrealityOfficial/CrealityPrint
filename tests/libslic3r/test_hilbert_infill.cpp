#include <catch2/catch.hpp>

#include "libslic3r/Fill/FillPlanePath.hpp"
#include "libslic3r/ShortestPath.hpp"
#include "libslic3r/ClipperUtils.hpp"

#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/task_arena.h>

using namespace Slic3r;

namespace {

class TestHilbert : public FillHilbertCurve {
public:
    using FillHilbertCurve::generate;
    using FillPlanePath::_fill_surface_single;
};

// Exercise the same generator through serial clipping and legacy ordering,
// without inheriting the Hilbert-specific fast paths.
class LegacyHilbert : public FillPlanePath {
public:
    Fill *clone() const override { return new LegacyHilbert(*this); }
    using FillPlanePath::_fill_surface_single;

protected:
    bool centered() const override { return false; }
    void generate(coord_t min_x, coord_t min_y, coord_t max_x, coord_t max_y,
                  double resolution, InfillPolylineOutput &output) override
    {
        TestHilbert generator;
        generator.generate(min_x, min_y, max_x, max_y, resolution, output);
    }
};

ExPolygon perforated_surface()
{
    ExPolygon surface(Polygon{
        Point::new_scale(0, 0), Point::new_scale(24, 0),
        Point::new_scale(24, 24), Point::new_scale(0, 24)});
    for (int y = 4; y <= 16; y += 6)
        for (int x = 4; x <= 16; x += 6)
            surface.holes.emplace_back(Polygon{
                Point::new_scale(x, y), Point::new_scale(x, y + 2),
                Point::new_scale(x + 2, y + 2), Point::new_scale(x + 2, y)});
    return surface;
}

void check_same_paths(const Polylines &actual, const Polylines &expected)
{
    REQUIRE(actual.size() == expected.size());
    for (size_t i = 0; i < actual.size(); ++i)
        CHECK(actual[i].points == expected[i].points);
}

Polylines canonical_paths(Polylines paths)
{
    auto less = [](const Point &a, const Point &b) {
        return a.x() < b.x() || (a.x() == b.x() && a.y() < b.y());
    };
    for (Polyline &path : paths)
        if (less(path.last_point(), path.first_point()))
            std::reverse(path.points.begin(), path.points.end());
    std::sort(paths.begin(), paths.end(), [&](const Polyline &a, const Polyline &b) {
        return std::lexicographical_compare(a.points.begin(), a.points.end(),
                                           b.points.begin(), b.points.end(), less);
    });
    return paths;
}

Polyline hilbert_grid()
{
    TestHilbert generator;
    InfillPolylineOutput output(1000.);
    generator.generate(0, 0, 63, 63, 0., output);
    Polyline polyline;
    polyline.points = std::move(output.result());
    return polyline;
}

} // namespace

TEST_CASE("Greedy polyline chaining preserves empty and single paths", "[Geometry][Hilbert]")
{
    CHECK(chain_polylines_greedy(Polylines{}).empty());
    const Polyline path{{0, 0}, {10, 0}, {10, 10}};
    check_same_paths(chain_polylines_greedy(Polylines{path}), Polylines{path});
}

TEST_CASE("Greedy chaining preserves complete paths and honors the start point", "[Geometry][Hilbert]")
{
    const Polylines paths{
        {{30, 10}, {30, 0}, {20, 0}},
        {{0, 0}, {0, 10}, {10, 10}},
        {{20, 0}, {20, 10}, {10, 10}}};
    const Point start(0, 0);
    Polylines input(paths);
    const Polylines chained = chain_polylines_greedy(std::move(input), &start);
    REQUIRE(chained.size() == paths.size());
    CHECK(chained.front().first_point() == start);
    for (size_t i = 1; i < chained.size(); ++i)
        CHECK(chained[i - 1].last_point() == chained[i].first_point());

    // Every original polyline must occur exactly once, possibly reversed,
    // including all interior vertices.
    std::vector<bool> used(paths.size(), false);
    for (const Polyline &path : chained) {
        bool found = false;
        for (size_t i = 0; i < paths.size(); ++i) {
            Points reversed(paths[i].points.rbegin(), paths[i].points.rend());
            if (!used[i] && (path.points == paths[i].points || path.points == reversed)) {
                used[i] = true;
                found = true;
                break;
            }
        }
        CHECK(found);
    }
}

TEST_CASE("Perforated Hilbert fill retains the legacy path geometry and order", "[Infill][Hilbert]")
{
    const float density = GENERATE(1.f, 0.65f, 0.15f);
    const bool connect = GENERATE(false, true);
    const float angle = GENERATE(0.f, float(PI / 4.));
    CAPTURE(density, connect, angle);

    TestHilbert current;
    LegacyHilbert legacy;
    current.spacing = legacy.spacing = 0.37707963267948963;
    // Sparse infill uses the object's box; deliberately make it larger than
    // the local surface to exercise InfillPolylineClipper as well.
    current.bounding_box = legacy.bounding_box =
        BoundingBox(Point::new_scale(-6, -6), Point::new_scale(30, 30));
    FillParams params;
    params.density = density;
    params.resolution = 0.012;
    params.anchor_length_max = connect ? 1000.f : 0.f;

    ExPolygon surface = perforated_surface();
    surface.translate(scale_(1.25), scale_(-2.5));
    Polylines actual, expected;
    current._fill_surface_single(params, 1, {angle, Point(0, 0)}, surface, actual);
    legacy._fill_surface_single(params, 1, {angle, Point(0, 0)}, surface, expected);
    REQUIRE(actual.size() > 1);
    check_same_paths(actual, expected);
}

TEST_CASE("Hilbert block clipping preserves continuous paths and hole contacts", "[Infill][Hilbert]")
{
    const size_t block_size = GENERATE(size_t(128), size_t(512), size_t(2048), size_t(8192));
    const int shape = GENERATE(0, 1, 2, 3, 4);
    CAPTURE(block_size, shape);
    const Polyline polyline = hilbert_grid();
    ExPolygon boundary(Polygon{{-1000, -1000}, {64000, -1000}, {64000, 64000}, {-1000, 64000}});
    if (shape == 1) {
        // Off-grid cuts, including diagonal edges.
        boundary.holes.emplace_back(Polygon{{15321, 15891}, {15321, 44456}, {44789, 44456}, {44789, 15891}});
        boundary.holes.emplace_back(Polygon{{5000, 5000}, {10000, 15000}, {15000, 5000}});
    } else if (shape == 2) {
        // Edges and corners coincide with the Hilbert grid.
        boundary.holes.emplace_back(Polygon{{16000, 16000}, {16000, 48000}, {48000, 48000}, {48000, 16000}});
        boundary.contour = Polygon{{0, 0}, {63000, 0}, {63000, 63000}, {0, 63000}};
    } else if (shape == 3) {
        // Most blocks are outside; retain a narrow, disconnected set of runs.
        boundary.contour = Polygon{{14001, -2000}, {14999, -2000}, {14999, 65000}, {14001, 65000}};
    } else if (shape == 4) {
        boundary.translate(100000, 100000);
    }
    const Polylines expected = canonical_paths(intersection_pl(polyline, boundary));
    const Polylines actual = canonical_paths(Hilbert::clip(polyline, boundary, block_size));
    check_same_paths(actual, expected);
}

TEST_CASE("Hilbert block clipping is deterministic across worker counts and nested tasks", "[Infill][Hilbert]")
{
    const Polyline polyline = hilbert_grid();
    ExPolygon boundary(Polygon{{500, 500}, {62500, 500}, {62500, 62500}, {500, 62500}});
    boundary.holes.emplace_back(Polygon{{10123, 10123}, {10123, 39987}, {39987, 39987}, {39987, 10123}});
    tbb::task_arena serial(1);
    const Polylines expected = serial.execute([&] { return Hilbert::clip(polyline, boundary, 256); });
    tbb::task_arena parallel(4);
    parallel.execute([&] {
        std::vector<Polylines> layers(4);
        tbb::parallel_for(size_t(0), layers.size(), [&](size_t i) {
            layers[i] = Hilbert::clip(polyline, boundary, 256);
        });
        for (const Polylines &paths : layers)
            check_same_paths(paths, expected);
    });
}

TEST_CASE("Large rotated Hilbert fill preserves geometry through the parallel dispatch", "[Infill][Hilbert]")
{
    TestHilbert current;
    LegacyHilbert legacy;
    current.spacing = legacy.spacing = 0.37707963267948963;
    ExPolygon surface(Polygon{
        Point::new_scale(-75, -75), Point::new_scale(75, -75),
        Point::new_scale(75, 75), Point::new_scale(-75, 75)});
    surface.holes.emplace_back(Polygon{
        Point::new_scale(-20, -20), Point::new_scale(0, 30), Point::new_scale(20, -20)});
    current.bounding_box = legacy.bounding_box = get_extents(surface);
    FillParams params;
    params.density = 1.f;
    params.resolution = 0.012;
    const std::pair<float, Point> direction{float(PI / 4.), Point(0, 0)};
    Polylines actual, expected;
    current._fill_surface_single(params, 1, direction, surface, actual);
    legacy._fill_surface_single(params, 1, direction, surface, expected);
    check_same_paths(canonical_paths(std::move(actual)), canonical_paths(std::move(expected)));
}
