#include <catch2/catch.hpp>

#include "libslic3r/Arrange.hpp"
#include "libslic3r/BoundingBox.hpp"
#include "libslic3r/Geometry.hpp"

using namespace Slic3r;

TEST_CASE("CR-30 arrangement centers objects across the belt", "[Arrange][CR-30]")
{
    const Points bed{
        Point::new_scale(0.0, 0.0),
        Point::new_scale(170.0, 0.0),
        Point::new_scale(170.0, 1000.0),
        Point::new_scale(0.0, 1000.0)
    };

    arrangement::ArrangePolygon item;
    item.poly.contour.points = {
        Point::new_scale(10.0, 20.0),
        Point::new_scale(50.0, 20.0),
        Point::new_scale(50.0, 60.0),
        Point::new_scale(10.0, 60.0)
    };

    arrangement::ArrangePolygons items{item};
    arrangement::cr30_arrange(items, {}, bed);

    const BoundingBox bed_box(bed);
    const BoundingBox arranged_box(items.front().transformed_poly().contour.points);
    REQUIRE(arranged_box.center().x() == bed_box.center().x());
}


#include "libslic3r/ArrangeGrid.hpp"
#include "libslic3r/ClipperUtils.hpp"

namespace {
arrangement::ArrangePolygon grid_rectangle(double x, double y, double w, double h)
{
    arrangement::ArrangePolygon item;
    item.poly.contour = Slic3r::Polygon({Point::new_scale(x, y), Point::new_scale(x + w, y),
                                Point::new_scale(x + w, y + h), Point::new_scale(x, y + h)});
    item.bed_idx = 0;
    return item;
}
}

TEST_CASE("Dense bed filling avoids models and rotated tower envelopes", "[Arrange][FillBed]")
{
    auto item = grid_rectangle(7, -3, 8, 6); // An off-center, rotated source.
    item.rotation = 0.31;
    item.inflation = scaled(1.);
    arrangement::ArrangePolygons copies(200, item);
    auto model = grid_rectangle(25, 25, 30, 20);
    model.inflation = scaled(2.);
    auto tower = grid_rectangle(-8, -12, 16, 24); // Includes its brim and ribs.
    tower.is_wipe_tower = true;
    tower.rotation = 0.7;
    tower.translation = scaled(Vec2d(75, 65));
    tower.inflation = scaled(1.);
    auto edge_model = grid_rectangle(-5, 65, 15, 20); // Partly outside the bed.
    const arrangement::ArrangePolygons fixed{model, tower, edge_model};
    const auto bed = grid_rectangle(0, 0, 100, 100).poly.contour;
    arrangement::ArrangeParams params;
    arrangement::arrange_grid(copies, fixed, bed.points, params);
    ExPolygons placed;
    size_t count = 0;
    for (const auto& copy : copies) {
        if (copy.bed_idx == arrangement::UNARRANGED)
            continue;
        ++count;
        REQUIRE(copy.bed_idx == 0);
        REQUIRE(copy.rotation == item.rotation);
        const auto shape = offset_ex(copy.transformed_poly(), double(copy.inflation));
        REQUIRE(diff_ex(shape, Polygons{bed}).empty());
        for (const auto& obstacle : fixed)
            REQUIRE(intersection_ex(shape, offset_ex(obstacle.transformed_poly(), double(obstacle.inflation))).empty());
        REQUIRE(intersection_ex(shape, placed).empty());
        placed.insert(placed.end(), shape.begin(), shape.end());
    }
    REQUIRE(count > 10);
    REQUIRE(count < copies.size());
    REQUIRE(fixed[1].translation == tower.translation);
}

TEST_CASE("Grid filling respects concave beds and occupied plates", "[Arrange][FillBed]")
{
    const Slic3r::Polygon bed({Point::new_scale(0, 0), Point::new_scale(60, 0), Point::new_scale(60, 30),
                       Point::new_scale(30, 30), Point::new_scale(30, 60), Point::new_scale(0, 60)});
    arrangement::ArrangePolygons copies(200, grid_rectangle(0, 0, 10, 10));
    arrangement::ArrangeParams params;
    SECTION("Concave boundary") {
        arrangement::arrange_grid(copies, {}, bed.points, params);
        size_t count = 0;
        for (const auto& copy : copies)
            if (copy.is_arranged()) {
                ++count;
                REQUIRE(diff_ex(ExPolygons{copy.transformed_poly()}, Polygons{bed}).empty());
            }
        REQUIRE(count == 27);
    }
    SECTION("No free cells") {
        arrangement::arrange_grid(copies, {grid_rectangle(0, 0, 60, 60)}, bed.points, params);
        REQUIRE(std::none_of(copies.begin(), copies.end(), [](const auto& p) { return p.is_arranged(); }));
    }
    SECTION("Ignore another plate") {
        auto other = grid_rectangle(0, 0, 60, 60);
        other.bed_idx = 1;
        arrangement::arrange_grid(copies, {other}, bed.points, params);
        REQUIRE(copies.front().is_arranged());
    }
    SECTION("Canceled fill leaves candidates unplaced") {
        params.stopcondition = [] { return true; };
        arrangement::arrange_grid(copies, {}, bed.points, params);
        REQUIRE(std::none_of(copies.begin(), copies.end(), [](const auto& p) { return p.is_arranged(); }));
    }
}


TEST_CASE("Small bed fills retain fixed models and wipe towers", "[Arrange][FillBed]")
{
    auto item = grid_rectangle(0, 0, 10, 10);
    item.inflation = scaled(1.);
    item.height = 1.;
    item.extrude_ids = {1};
    arrangement::ArrangePolygons copies(6, item);
    auto model = grid_rectangle(15, 15, 25, 25);
    auto tower = grid_rectangle(-5, -10, 10, 20);
    tower.translation = scaled(Vec2d(55, 45));
    tower.rotation = 0.5;
    tower.is_virt_object = tower.is_wipe_tower = true;
    const arrangement::ArrangePolygons fixed{model, tower};
    const auto bed = grid_rectangle(0, 0, 80, 80).poly.contour;
    arrangement::ArrangeParams params;
    params.do_final_align = false;
    params.parallel = false;
    params.progressind = [](unsigned, std::string) {};
    arrangement::arrange(copies, fixed, bed.points, params);
    size_t count = 0;
    for (const auto& copy : copies)
        if (copy.bed_idx == 0) {
            ++count;
            for (const auto& obstacle : fixed)
                REQUIRE(intersection_ex(ExPolygons{copy.transformed_poly()}, ExPolygons{obstacle.transformed_poly()}).empty());
        }
    REQUIRE(count > 0);
}

#include "libslic3r/WipeTowerPlacement.hpp"

static Slic3r::Polygon tower_rectangle(double x0, double y0, double x1, double y1)
{
    return Slic3r::Polygon({Point::new_scale(x0, y0), Point::new_scale(x1, y0),
                           Point::new_scale(x1, y1), Point::new_scale(x0, y1)});
}

TEST_CASE("Growing towers preserve valid anchors and move minimally inside the bed", "[Arrange][TowerPlacement]")
{
    const auto bed = tower_rectangle(0, 0, 100, 100);
    const auto tower = tower_rectangle(-5, -3, 35, 27); // Includes brim and an offset anchor.
    const Point valid = Point::new_scale(40, 40);
    REQUIRE(nearest_wipe_tower_position(tower, bed, {}, valid) == std::optional<Point>(valid));
    const auto moved = nearest_wipe_tower_position(tower, bed, {}, Point::new_scale(80, 40));
    REQUIRE(moved.has_value());
    REQUIRE(unscale<double>(moved->x()) == Approx(65.).margin(0.001));
    REQUIRE(unscale<double>(moved->y()) == Approx(40.).margin(0.001));
    auto placed = tower;
    placed.translate(*moved);
    REQUIRE(diff_ex(Polygons{placed}, Polygons{bed}).empty());
}

TEST_CASE("Dragging a rotated tower keeps its brim clear of all four plate edges", "[Arrange][TowerPlacement][18773]")
{
    auto bed = tower_rectangle(0, 0, 260, 260);
    const bool clockwise = GENERATE(false, true);
    if (clockwise)
        bed.reverse();
    auto tower = tower_rectangle(-5, -3, 35, 27);
    tower.rotate(Geometry::deg2rad(42.));
    const bool sliced = GENERATE(false, true);
    const double margin = sliced ? 0.2 : 15.;
    const Point center = Point::new_scale(130, 130);
    REQUIRE(dragged_wipe_tower_position(tower, bed, center, sliced) == std::optional<Point>(center));

    for (int axis : {0, 1}) {
        for (int direction : {-1, 1}) {
            Point target = center;
            target[axis] = scaled(direction < 0 ? -50. : 310.);
            const auto anchor = dragged_wipe_tower_position(tower, bed, target, sliced);
            REQUIRE(anchor.has_value());
            auto placed = tower;
            placed.translate(*anchor);
            const auto bounds = get_extents(placed);
            const double gap = direction < 0 ? unscale<double>(bounds.min[axis]) : 260. - unscale<double>(bounds.max[axis]);
            REQUIRE(gap == Approx(margin).margin(0.001));
            REQUIRE((*anchor)[1 - axis] == center[1 - axis]);
        }
    }
}

TEST_CASE("Tower relocation avoids models and excluded regions with rotation", "[Arrange][TowerPlacement]")
{
    const auto bed = tower_rectangle(0, 0, 180, 180);
    auto tower = tower_rectangle(-4, -6, 30, 42);
    tower.rotate(0.6);
    const Polygons obstacles{tower_rectangle(60, 60, 115, 125), tower_rectangle(0, 0, 40, 45)};
    const Point original = Point::new_scale(80, 80);
    const auto position = nearest_wipe_tower_position(tower, bed, obstacles, original);
    REQUIRE(position.has_value());
    REQUIRE(*position != original);
    tower.translate(*position);
    REQUIRE(diff_ex(Polygons{tower}, Polygons{bed}).empty());
    REQUIRE(intersection_ex(Polygons{tower}, obstacles).empty());
}

TEST_CASE("An off-plate project tower can be recovered with clearance around the bed and models", "[Arrange][TowerPlacement]")
{
    const auto bed = tower_rectangle(0, 0, 260, 260);
    auto tower = tower_rectangle(-6, -4, 60, 80);
    const WipeTowerPose saved{Point::new_scale(672.01, -88.5369), 42.};
    const Polygons obstacles{tower_rectangle(140, 0, 260, 130)};
    const auto pose = find_loaded_wipe_tower_pose(tower, bed, obstacles, saved, false);
    REQUIRE(pose);
    tower.rotate(Geometry::deg2rad(pose->rotation_degrees));
    tower.translate(pose->anchor);
    REQUIRE(diff(tower, tower_rectangle(15, 15, 245, 245)).empty());
    REQUIRE(intersection_ex(Polygons{tower}, obstacles).empty());
}

TEST_CASE("Opening a project tries every translation at its saved angle before rotating a single plate", "[Arrange][TowerPlacement]")
{
    const auto bed = tower_rectangle(0, 0, 100, 100);
    const auto tower = tower_rectangle(0, 0, 72, 28);
    const WipeTowerPose saved{Point::new_scale(180, -20), 0.};
    // Only 36 mm remain on the right. An upright tower fits, a horizontal one cannot.
    const Polygons narrow{tower_rectangle(0, 0, 64, 100)};
    REQUIRE_FALSE(find_loaded_wipe_tower_pose(tower, bed, narrow, saved, false));
    const auto turned = find_loaded_wipe_tower_pose(tower, bed, narrow, saved, true);
    REQUIRE(turned);
    REQUIRE(turned->rotation_degrees == 90.);
    auto placed = tower;
    placed.rotate(Geometry::deg2rad(turned->rotation_degrees));
    placed.translate(turned->anchor);
    REQUIRE(diff(placed, bed).empty());
    REQUIRE(intersection_ex(Polygons{placed}, narrow).empty());

    // The saved angle fits within the hard bed bounds but outside the preferred margin.
    const auto tight = find_loaded_wipe_tower_pose(tower, bed, {tower_rectangle(0, 0, 26, 100)}, saved, true);
    REQUIRE(tight);
    REQUIRE(tight->rotation_degrees == saved.rotation_degrees);
    REQUIRE_FALSE(find_loaded_wipe_tower_pose(tower, bed, {bed}, saved, true));
}

TEST_CASE("Tower relocation respects concave beds and reports no space without a position", "[Arrange][TowerPlacement]")
{
    const Slic3r::Polygon bed({Point::new_scale(0, 0), Point::new_scale(100, 0), Point::new_scale(100, 40),
                              Point::new_scale(40, 40), Point::new_scale(40, 100), Point::new_scale(0, 100)});
    auto tower = tower_rectangle(0, 0, 20, 20);
    const Point original = Point::new_scale(60, 60);
    const auto position = nearest_wipe_tower_position(tower, bed, {}, original);
    REQUIRE(position.has_value());
    tower.translate(*position);
    REQUIRE(diff_ex(Polygons{tower}, Polygons{bed}).empty());
    REQUIRE(((*position - original).cast<double>().norm() * SCALING_FACTOR) == Approx(40.).margin(0.001));
    REQUIRE_FALSE(nearest_wipe_tower_position(tower_rectangle(0, 0, 20, 20), bed,
                                             {tower_rectangle(0, 0, 100, 100)}, original).has_value());
    REQUIRE_FALSE(nearest_wipe_tower_position(tower_rectangle(0, 0, 120, 120), bed, {}, original).has_value());
}

TEST_CASE("Tower placement responds only to footprint changes and respects manual moves", "[Arrange][TowerPlacement]")
{
    WipeTowerPlacementObservation state;
    const auto smaller_tower = tower_rectangle(-3, -3, 23, 23);
    const auto large = tower_rectangle(-3, -3, 43, 33);
    const Point anchor = Point::new_scale(80, 80);
    REQUIRE_FALSE(state.observe(smaller_tower, anchor)); // Opening an existing project.
    REQUIRE_FALSE(state.observe(smaller_tower, anchor)); // Redraw or an unused filament slot.
    REQUIRE(state.observe(large, anchor));
    REQUIRE_FALSE(state.observe(large, anchor)); // No room: do not retry on every redraw.
    REQUIRE_FALSE(state.observe(large, Point::new_scale(50, 50))); // Manual movement.
    REQUIRE_FALSE(state.observe(smaller_tower, anchor)); // Explicit restored position takes precedence.
    state = WipeTowerPlacementObservation{}; // Undo/load starts a fresh baseline.
    REQUIRE_FALSE(state.observe(large, anchor));
    REQUIRE_FALSE(state.observe({}, anchor));
    REQUIRE(state.observe(smaller_tower, anchor)); // A newly needed tower.
}
