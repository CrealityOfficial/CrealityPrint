#include <catch2/catch.hpp>

#include <cmath>
#include <cstdlib>

#include "libslic3r/BoundingBox.hpp"
#include "libslic3r/ExtrusionEntityCollection.hpp"
#include "libslic3r/ExtrusionEntity.hpp"
#include "libslic3r/Fill/FillFloatingConcentric.hpp"
#include "libslic3r/Point.hpp"
#include "libslic3r/libslic3r.h"

#include "test_data.hpp"

using namespace Slic3r;

static inline Slic3r::Point random_point(float LO=-50, float HI=50) 
{
    Vec2f pt = Vec2f(LO, LO) + (Vec2d(rand(), rand()) * (HI-LO) / RAND_MAX).cast<float>();
	return pt.cast<coord_t>();
}

// Use exact grid coordinates for the trace and the strict 2 mm boundary cases.
static Point shell_point_mm(double x, double y)
{
    return Point(coord_t(std::lround(scale_(x))), coord_t(std::lround(scale_(y))));
}

TEST_CASE("The floating stretches of Overhang layer 159 survive every closed seam", "[FloatingShell]")
{
    // Toolpath offsets in mm, before the slicer moves the part to the bed.
    for (const bool outer : {true, false}) {
        const double right = outer ? 23.21949 : 22.84718;
        const double left  = outer ? 21.58599 : 21.95830;
        const double y     = outer ? 11.51540 : 11.14309;
        const double cut = 22.79524;
        const Points vertices{
            shell_point_mm(right, y), shell_point_mm(cut, y), shell_point_mm(cut, y),
            shell_point_mm(left, y), shell_point_mm(left, -y), shell_point_mm(cut, -y),
            shell_point_mm(cut, -y), shell_point_mm(right, -y)};
        const std::vector<bool> marks{true, true, false, false, false, false, true, true};
        const size_t n = vertices.size();
        for (size_t scan = 0; scan < n; ++scan) {
            FloatingThickPolyline line;
            std::vector<bool> input;
            for (size_t j = 0; j <= n; ++j) {
                const size_t i = (scan + j) % n;
                line.points.emplace_back(vertices[i]);
                input.push_back(marks[i]);
                if (j < n) {
                    line.width.push_back(scale_(0.4) + 2 * i);
                    line.width.push_back(scale_(0.4) + 2 * i + 1);
                }
            }
            line.set_smoothed_floating_edges(input, true);
            for (size_t start = 0; start < n; ++start) {
                const auto rotated = line.rebase_at(start);
                REQUIRE(rotated.floating_edges.size() == n);
                for (size_t j = 0; j < n; ++j) {
                    const size_t i = (scan + start + j) % n;
                    CAPTURE(outer, scan, start, i);
                    CHECK(rotated.points[j] == vertices[i]);
                    CHECK(rotated.width[2 * j] == scale_(0.4) + 2 * i);
                    CHECK(rotated.width[2 * j + 1] == scale_(0.4) + 2 * i + 1);
                    if (rotated.points[j] != rotated.points[j + 1])
                        CHECK(rotated.floating_edges[j] == (i == 0 || i == 6 || i == 7));
                }
            }
        }
    }
}

TEST_CASE("Floating shell smoothing respects support boundaries and path topology", "[FloatingShell]")
{
    FloatingThickPolyline line;
    const Point a = shell_point_mm(0, 0), b = shell_point_mm(4, 0);
    const Point c = shell_point_mm(4, 4), d = shell_point_mm(0, 4);
    SECTION("A real boundary at the seam keeps both incident edge states after clipping") {
        line.points = {a, b, c, c, d, a};
        line.width.assign(10, scale_(0.4));
        line.set_smoothed_floating_edges({false, false, false, true, true, true}, true);
        auto loop = line.rebase_at(0);
        CHECK_FALSE(loop.floating_edges.front());
        CHECK(loop.floating_edges.back());
        loop.clip_end(scale_(1.));
        REQUIRE(loop.floating_edges.size() + 1 == loop.points.size());
        CHECK(loop.floating_edges.back());
        for (const bool variable : {false, true}) {
            if (variable)
                loop.width[7] = scale_(0.56);
            double length = 0.;
            const auto paths = loop.to_extrusion_paths(erFloatingVerticalShell, Flow(0.42f, 0.2f, 0.4f), 0.012);
            for (const auto& path : paths)
                if (path.get_customize_flag() == CustomizeFlag::cfFloatingVerticalShell)
                    length += unscale<double>(path.length());
            CHECK(length == Approx(7.));
        }
        loop.clip_end(scale_(20.));
        CHECK(loop.floating_edges.empty());
    }
    SECTION("A supported gap across the seam is measured at its full length") {
        for (const double gap : {1. - SCALING_FACTOR, 1., 1. + SCALING_FACTOR}) {
            const Point p = shell_point_mm(1., 0.), q = shell_point_mm(0., gap);
            line.points = {a, p, p, b, c, d, q, q, a};
            line.set_smoothed_floating_edges({false, false, true, true, true, true, true, false, false}, true);
            CHECK(line.floating_edges.front() == (gap < 1.));
            CHECK(line.floating_edges.back() == (gap < 1.));
            CHECK(line.floating_edges[3]);
        }
    }
    SECTION("Only a topologically closed path joins its first and last floating stretches") {
        const Point p = shell_point_mm(0.6, 0.), q = shell_point_mm(0., 1.6);
        line.points = {a, p, p, c, q, q, a};
        for (const bool closed : {false, true}) {
            line.set_smoothed_floating_edges({true, true, false, false, false, true, true}, closed);
            CHECK(line.floating_edges.front() == closed);
            CHECK(line.floating_edges.back() == closed);
            CHECK_FALSE(line.floating_edges[2]);
        }
    }
    SECTION("A fully floating loop uses its circumference and the strict 2 mm threshold") {
        for (const double side : {0.5 - SCALING_FACTOR, 0.5, 0.5 + SCALING_FACTOR}) {
            line.points = {a, shell_point_mm(side, 0.), shell_point_mm(side, side), shell_point_mm(0., side), a};
            line.set_smoothed_floating_edges(std::vector<bool>(5, true), true);
            CHECK(line.floating_edges == std::vector<bool>(4, side >= 0.5));
        }
    }
}

// build a sample extrusion entity collection with random start and end points.
static Slic3r::ExtrusionPath random_path(size_t length = 20, float LO = -50, float HI = 50)
{
    ExtrusionPath t {erPerimeter, 1.0, 1.0, 1.0};
    for (size_t j = 0; j < length; ++ j)
        t.polyline.append(random_point(LO, HI));
    return t;
}

static Slic3r::ExtrusionPaths random_paths(size_t count = 10, size_t length = 20, float LO = -50, float HI = 50)
{
    Slic3r::ExtrusionPaths p;
    for (size_t i = 0; i < count; ++ i)
        p.push_back(random_path(length, LO, HI));
    return p;
}

SCENARIO("ExtrusionEntityCollection: Polygon flattening", "[ExtrusionEntity]") {
    srand(0xDEADBEEF); // consistent seed for test reproducibility.

    // Generate one specific random path set and save it for later comparison
    Slic3r::ExtrusionPaths nosort_path_set = random_paths();

    Slic3r::ExtrusionEntityCollection sub_nosort;
    sub_nosort.append(nosort_path_set);
    sub_nosort.no_sort = true;

    Slic3r::ExtrusionEntityCollection sub_sort;
    sub_sort.no_sort = false;
    sub_sort.append(random_paths());

    GIVEN("A Extrusion Entity Collection with a child that has one child that is marked as no-sort") {
        Slic3r::ExtrusionEntityCollection sample;
        Slic3r::ExtrusionEntityCollection output;

        sample.append(sub_sort);
        sample.append(sub_nosort);
        sample.append(sub_sort);

        WHEN("The EEC is flattened with default options (preserve_order=false)") {
			output = sample.flatten();
            THEN("The output EEC contains no Extrusion Entity Collections") {
                CHECK(std::count_if(output.entities.cbegin(), output.entities.cend(), [=](const ExtrusionEntity* e) {return e->is_collection();}) == 0);
            }
        }
        WHEN("The EEC is flattened with preservation (preserve_order=true)") {
			output = sample.flatten(true);
            THEN("The output EECs contains one EEC.") {
                CHECK(std::count_if(output.entities.cbegin(), output.entities.cend(), [=](const ExtrusionEntity* e) {return e->is_collection();}) == 1);
            }
            AND_THEN("The ordered EEC contains the same order of elements than the original") {
                // find the entity in the collection
                for (auto e : output.entities)
                    if (e->is_collection()) {
                        ExtrusionEntityCollection *temp = dynamic_cast<ExtrusionEntityCollection*>(e);
                        // check each Extrusion path against nosort_path_set to see if the first and last match the same
                        CHECK(nosort_path_set.size() == temp->entities.size());
                        for (size_t i = 0; i < nosort_path_set.size(); ++ i) {
                            CHECK(temp->entities[i]->first_point() == nosort_path_set[i].first_point());
                            CHECK(temp->entities[i]->last_point() == nosort_path_set[i].last_point());
                        }
                    }
            }
        }
    }
}
