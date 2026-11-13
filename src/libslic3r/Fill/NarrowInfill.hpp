#ifndef slic3r_NarrowInfill_hpp_
#define slic3r_NarrowInfill_hpp_

#include "../BoundingBox.hpp"
#include "../ClipperUtils.hpp"

namespace Slic3r {

enum class NarrowInfillType { Regular, Concentric, Floating };

// Bambu's test applies to the entire island, including its holes. A surviving
// wide core keeps connected thin branches in the original infill pattern.
inline NarrowInfillType classify_narrow_infill(const ExPolygon& area, const ExPolygons& lower_internal_areas)
{
    if (!offset_ex(area, -scaled<float>(3.)).empty())
        return NarrowInfillType::Regular;

    if (!lower_internal_areas.empty()) {
        const BoundingBox bbox = get_extents(area);
        const Polygons lower = ClipperUtils::clip_clipper_polygons_with_subject_bbox(
            lower_internal_areas, bbox.inflated(scaled<coord_t>(2.)));
        if (!lower.empty() && !intersection_ex(offset_ex(area, SCALED_EPSILON), lower).empty())
            return NarrowInfillType::Floating;
    }
    return NarrowInfillType::Concentric;
}

} // namespace Slic3r

#endif
