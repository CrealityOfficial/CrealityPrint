#include <catch2/catch.hpp>

#include "libslic3r/Arrange.hpp"
#include "libslic3r/BoundingBox.hpp"

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
