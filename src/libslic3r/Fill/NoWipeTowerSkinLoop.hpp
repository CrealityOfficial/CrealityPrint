#ifndef slic3r_NoWipeTowerSkinLoop_hpp_
#define slic3r_NoWipeTowerSkinLoop_hpp_

#include "../Polygon.hpp"
#include "../ExtrusionEntity.hpp"

namespace Slic3r {

// Preserve the full ring so moving its entrance also moves the clipped gap.
// Arbitrary open skin fragments must never be converted to this type.
class NoWipeTowerSkinLoop final : public ExtrusionPath
{
public:
    NoWipeTowerSkinLoop(const Polyline& ring, double clip_length,
                        double mm3_per_mm, float width, float height)
        : ExtrusionPath(erSkinInfill, mm3_per_mm, width, height), m_clip_length(clip_length)
    {
        m_ring.points.assign(ring.points.begin(), ring.points.end() - 1);
        polyline = ring;
        polyline.clip_end(m_clip_length);
    }

    ExtrusionEntity* clone() const override { return new NoWipeTowerSkinLoop(*this); }
    ExtrusionEntity* clone_move() override { return new NoWipeTowerSkinLoop(std::move(*this)); }
    bool can_reverse() const override { return false; }

    ExtrusionPath path_from(const Point& reference) const
    {
        ExtrusionPath path(*this);
        path.polyline = m_ring.split_at_index(reference.nearest_point_index(m_ring.points));
        path.polyline.clip_end(m_clip_length);
        return path;
    }

private:
    Polygon m_ring;
    double m_clip_length;
};

} // namespace Slic3r

#endif
