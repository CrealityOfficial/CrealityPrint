#ifndef slic3r_ZaaPathGeometry_hpp_
#define slic3r_ZaaPathGeometry_hpp_

#include "I18N.hpp"
#include "ExtrusionEntityCollection.hpp"
#include "ZAA.hpp"

#include <algorithm>

namespace Slic3r {

// Geometry is independent of the routing policy. In particular, a planar ZAA
// result still has motion, and equal endpoints do not make a loop empty.
enum class ZaaPathGeometry { NoMotion, HasMotion, Invalid };

inline ZaaPathGeometry zaa_path_geometry(const ExtrusionPath &path)
{
    if (!path.polyline.fitting_result.empty() || dynamic_cast<const ExtrusionPathSloped *>(&path) != nullptr)
        return ZaaPathGeometry::Invalid;

    const Points &xy = path.polyline.points;
    if (const ExtrusionPath3 *path3 = path.path3()) {
        const Points3 &xyz = path3->polyline3().points;
        if (xyz.size() != xy.size())
            return ZaaPathGeometry::Invalid;
        bool has_motion = false;
        for (size_t i = 0; i < xyz.size(); ++i) {
            if (Point(xyz[i].x(), xyz[i].y()) != xy[i])
                return ZaaPathGeometry::Invalid;
            if (i > 0 && (xyz[i].array() != xyz[i - 1].array()).any())
                has_motion = true;
        }
        return has_motion ? ZaaPathGeometry::HasMotion : ZaaPathGeometry::NoMotion;
    }
    for (size_t i = 1; i < xy.size(); ++i)
        if (xy[i] != xy[i - 1])
            return ZaaPathGeometry::HasMotion;
    return ZaaPathGeometry::NoMotion;
}

// Caller supplies the offset-plane layer scope. Conventional roles are left
// untouched, even when they are in the same collection as a ZAA candidate.
inline bool zaa_keep_path(const ExtrusionPath &path)
{
    if (!zaa_role_is_eligible(path.role()))
        return true;
    const ZaaPathGeometry geometry = zaa_path_geometry(path);
    if (geometry == ZaaPathGeometry::Invalid)
        throw LogicError(_u8L("ZAA path geometry is inconsistent or is not linear"));
    return geometry != ZaaPathGeometry::NoMotion;
}

inline bool zaa_prune_no_motion_paths(ExtrusionPaths &paths)
{
    paths.erase(std::remove_if(paths.begin(), paths.end(), [](const ExtrusionPath &path) {
        return !zaa_keep_path(path);
    }), paths.end());
    return !paths.empty();
}

// Remove through the owning container. Never leave an empty loop/multipath for
// ordering or seam placement, which require usable first/last points.
inline bool zaa_prune_no_motion_paths(ExtrusionEntity &entity)
{
    if (auto *collection = dynamic_cast<ExtrusionEntityCollection *>(&entity)) {
        for (size_t i = 0; i < collection->entities.size();) {
            if (zaa_prune_no_motion_paths(*collection->entities[i]))
                ++i;
            else
                collection->remove(i);
        }
        return !collection->empty();
    }
    if (auto *multipath = dynamic_cast<ExtrusionMultiPath *>(&entity))
        return zaa_prune_no_motion_paths(multipath->paths);
    if (auto *loop = dynamic_cast<ExtrusionLoop *>(&entity))
        return zaa_prune_no_motion_paths(loop->paths);
    if (const auto *path = dynamic_cast<const ExtrusionPath *>(&entity))
        return zaa_keep_path(*path);
    throw LogicError(_u8L("ZAA geometry cleanup encountered an unsupported extrusion entity"));
}

// Simplification must not turn existing motion into a no-op. Retain the source
// geometry in that case, including all backtracking, so material/output limits
// are decided later with the original travel still available.
inline void zaa_simplify_path(ExtrusionPath &path, double tolerance)
{
    // Like Polyline::simplify, replace any fitting metadata from an earlier
    // conventional pass with a linear representation of the source points.
    path.polyline.fitting_result.clear();
    const ZaaPathGeometry before = zaa_path_geometry(path);
    if (before == ZaaPathGeometry::Invalid || path.has_path3())
        throw LogicError(_u8L("ZAA simplification requires a linear 2D path"));
    Polyline original = std::move(path.polyline);
    path.polyline.clear();
    path.polyline.points = MultiPoint::_douglas_peucker(original.points, tolerance);
    if (before == ZaaPathGeometry::HasMotion && zaa_path_geometry(path) == ZaaPathGeometry::NoMotion)
        path.polyline = std::move(original);
    path.set_zaa_path_policy(ZaaPathPolicy::ZaaLinearCandidate);
}

} // namespace Slic3r

#endif
