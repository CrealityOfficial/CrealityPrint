#include "ClipperUtils.hpp"
#include "Geometry.hpp"
#include "ShortestPath.hpp"

// #define CLIPPER_UTILS_DEBUG

#ifdef CLIPPER_UTILS_DEBUG
#include "SVG.hpp"
#endif /* CLIPPER_UTILS_DEBUG */

// Profiling support using the Shiny intrusive profiler
//#define CLIPPER_UTILS_PROFILE
#if defined(SLIC3R_PROFILE) && defined(CLIPPER_UTILS_PROFILE)
	#include <Shiny/Shiny.h>
	#define CLIPPERUTILS_PROFILE_FUNC() PROFILE_FUNC()
	#define CLIPPERUTILS_PROFILE_BLOCK(name) PROFILE_BLOCK(name)
#else
	#define CLIPPERUTILS_PROFILE_FUNC()
	#define CLIPPERUTILS_PROFILE_BLOCK(name)
#endif

#define CLIPPER_OFFSET_SHORTEST_EDGE_FACTOR (0.005f)

namespace Slic3r {

#ifdef CLIPPER_UTILS_DEBUG
// For debugging the Clipper library, for providing bug reports to the Clipper author.
bool export_clipper_input_polygons_bin(const char *path, const Clipper3r::Paths &input_subject, const Clipper3r::Paths &input_clip)
{
    FILE *pfile = fopen(path, "wb");
    if (pfile == NULL)
        return false;

    uint32_t sz = uint32_t(input_subject.size());
    fwrite(&sz, 1, sizeof(sz), pfile);
    for (size_t i = 0; i < input_subject.size(); ++i) {
        const Clipper3r::Path &path = input_subject[i];
        sz = uint32_t(path.size());
        ::fwrite(&sz, 1, sizeof(sz), pfile);
        ::fwrite(path.data(), sizeof(Clipper3r::IntPoint), sz, pfile);
    }
    sz = uint32_t(input_clip.size());
    ::fwrite(&sz, 1, sizeof(sz), pfile);
    for (size_t i = 0; i < input_clip.size(); ++i) {
        const Clipper3r::Path &path = input_clip[i];
        sz = uint32_t(path.size());
        ::fwrite(&sz, 1, sizeof(sz), pfile);
        ::fwrite(path.data(), sizeof(Clipper3r::IntPoint), sz, pfile);
    }
    ::fclose(pfile);
    return true;

err:
    ::fclose(pfile);
    return false;
}
#endif /* CLIPPER_UTILS_DEBUG */

namespace ClipperUtils {
Points EmptyPathsProvider::s_empty_points;
Points SinglePathProvider::s_end;

// Clip source polygon to be used as a clipping polygon with a bouding box around the source (to be clipped) polygon.
// Useful as an optimization for expensive ClipperLib operations, for example when clipping source polygons one by one
// with a set of polygons covering the whole layer below.
template<typename PointType> inline void clip_clipper_polygon_with_subject_bbox_templ(const std::vector<PointType> &src, const BoundingBox &bbox, std::vector<PointType> &out, const bool get_entire_polygons=false)
{
    out.clear();
    const size_t cnt = src.size();
    if (cnt < 3) return;

    enum class Side { Left = 1, Right = 2, Top = 4, Bottom = 8 };

    auto sides = [bbox](const PointType &p) {
        return int(p.x() < bbox.min.x()) * int(Side::Left) + int(p.x() > bbox.max.x()) * int(Side::Right) + int(p.y() < bbox.min.y()) * int(Side::Bottom) +
               int(p.y() > bbox.max.y()) * int(Side::Top);
    };

    int          sides_prev = sides(src.back());
    int          sides_this = sides(src.front());
    const size_t last       = cnt - 1;
    for (size_t i = 0; i < last; ++i) {
        int sides_next = sides(src[i + 1]);
        if ( // This point is inside. Take it.
            sides_this == 0 ||
            // Either this point is outside and previous or next is inside, or
            // the edge possibly cuts corner of the bounding box.
            (sides_prev & sides_this & sides_next) == 0) {
            out.emplace_back(src[i]);
            sides_prev = sides_this;
        } else {
            // All the three points (this, prev, next) are outside at the same side.
            // Ignore this point.
        }
        sides_this = sides_next;
    }

    // Never produce just a single point output polygon.
    if (!out.empty())
        if(get_entire_polygons){
            out=src;
        }else{
            if (int sides_next = sides(out.front());
            // The last point is inside. Take it.
            sides_this == 0 ||
            // Either this point is outside and previous or next is inside, or
            // the edge possibly cuts corner of the bounding box.
            (sides_prev & sides_this & sides_next) == 0)
            out.emplace_back(src.back());
        }

}

void clip_clipper_polygon_with_subject_bbox(const Points &src, const BoundingBox &bbox, Points &out, const bool get_entire_polygons) { clip_clipper_polygon_with_subject_bbox_templ(src, bbox, out, get_entire_polygons); }
void clip_clipper_polygon_with_subject_bbox(const ZPoints &src, const BoundingBox &bbox, ZPoints &out) { clip_clipper_polygon_with_subject_bbox_templ(src, bbox, out); }

template<typename PointType> [[nodiscard]] std::vector<PointType> clip_clipper_polygon_with_subject_bbox_templ(const std::vector<PointType> &src, const BoundingBox &bbox)
{
    std::vector<PointType> out;
    clip_clipper_polygon_with_subject_bbox(src, bbox, out);
    return out;
}

[[nodiscard]] Points  clip_clipper_polygon_with_subject_bbox(const Points &src, const BoundingBox &bbox) { return clip_clipper_polygon_with_subject_bbox_templ(src, bbox); }
[[nodiscard]] ZPoints clip_clipper_polygon_with_subject_bbox(const ZPoints &src, const BoundingBox &bbox) { return clip_clipper_polygon_with_subject_bbox_templ(src, bbox); }

void clip_clipper_polygon_with_subject_bbox(const Polygon &src, const BoundingBox &bbox, Polygon &out) { 
    clip_clipper_polygon_with_subject_bbox(src.points, bbox, out.points);
}

[[nodiscard]] Polygon clip_clipper_polygon_with_subject_bbox(const Polygon &src, const BoundingBox &bbox, const bool get_entire_polygons)
{
    Polygon out;
    clip_clipper_polygon_with_subject_bbox(src.points, bbox, out.points, get_entire_polygons);
    return out;
}

[[nodiscard]] Polygons clip_clipper_polygons_with_subject_bbox(const Polygons &src, const BoundingBox &bbox)
{
    Polygons out;
    out.reserve(src.size());
    for (const Polygon &p : src) out.emplace_back(clip_clipper_polygon_with_subject_bbox(p, bbox));
    out.erase(std::remove_if(out.begin(), out.end(), [](const Polygon &polygon) { return polygon.empty(); }), out.end());
    return out;
}
[[nodiscard]] Polygons clip_clipper_polygons_with_subject_bbox(const ExPolygon &src, const BoundingBox &bbox, const bool get_entire_polygons)
{
    Polygons out;
    out.reserve(src.num_contours());
    out.emplace_back(clip_clipper_polygon_with_subject_bbox(src.contour, bbox, get_entire_polygons));
    for (const Polygon &p : src.holes) out.emplace_back(clip_clipper_polygon_with_subject_bbox(p, bbox, get_entire_polygons));
    out.erase(std::remove_if(out.begin(), out.end(), [](const Polygon &polygon) { return polygon.empty(); }), out.end());
    return out;
}
[[nodiscard]] Polygons clip_clipper_polygons_with_subject_bbox(const ExPolygons &src, const BoundingBox &bbox, const bool get_entire_polygons)
{
    Polygons out;
    out.reserve(number_polygons(src));
    for (const ExPolygon &p : src) {
        Polygons temp = clip_clipper_polygons_with_subject_bbox(p, bbox, get_entire_polygons);
        out.insert(out.end(), temp.begin(), temp.end());
    }

    out.erase(std::remove_if(out.begin(), out.end(), [](const Polygon &polygon) {return polygon.empty(); }), out.end());
    return out;
}
}

static ExPolygons PolyTreeToExPolygons(Clipper3r::PolyTree &&polytree)
{
    struct Inner {
        static void PolyTreeToExPolygonsRecursive(Clipper3r::PolyNode &&polynode, ExPolygons *expolygons)
        {  
            size_t cnt = expolygons->size();
            expolygons->resize(cnt + 1);
            (*expolygons)[cnt].contour.points = std::move(polynode.Contour);
            (*expolygons)[cnt].holes.resize(polynode.ChildCount());
            for (int i = 0; i < polynode.ChildCount(); ++ i) {
                (*expolygons)[cnt].holes[i].points = std::move(polynode.Childs[i]->Contour);
                // Add outer polygons contained by (nested within) holes.
                for (int j = 0; j < polynode.Childs[i]->ChildCount(); ++ j)
                    PolyTreeToExPolygonsRecursive(std::move(*polynode.Childs[i]->Childs[j]), expolygons);
            }
        }

        static size_t PolyTreeCountExPolygons(const Clipper3r::PolyNode &polynode)
        {
            size_t cnt = 1;
            for (int i = 0; i < polynode.ChildCount(); ++ i) {
                for (int j = 0; j < polynode.Childs[i]->ChildCount(); ++ j)
                cnt += PolyTreeCountExPolygons(*polynode.Childs[i]->Childs[j]);
            }
            return cnt;
        }
    };

    ExPolygons retval;
    size_t cnt = 0;
    for (int i = 0; i < polytree.ChildCount(); ++ i)
        cnt += Inner::PolyTreeCountExPolygons(*polytree.Childs[i]);
    retval.reserve(cnt);
    for (int i = 0; i < polytree.ChildCount(); ++ i)
        Inner::PolyTreeToExPolygonsRecursive(std::move(*polytree.Childs[i]), &retval);
    return retval;
}

Polylines PolyTreeToPolylines(Clipper3r::PolyTree &&polytree)
{
    struct Inner {
        static void AddPolyNodeToPaths(Clipper3r::PolyNode &polynode, Polylines &out)
        {
            if (! polynode.Contour.empty())
                out.emplace_back(std::move(polynode.Contour));
            for (Clipper3r::PolyNode *child : polynode.Childs)
                AddPolyNodeToPaths(*child, out);
        }
    };

    Polylines out;
    out.reserve(polytree.Total());
    Inner::AddPolyNodeToPaths(polytree, out);
    return out;
}

#if 0
// Global test.
bool has_duplicate_points(const Clipper3r::PolyTree &polytree)
{
    struct Helper {
        static void collect_points_recursive(const Clipper3r::PolyNode &polynode, Clipper3r::Path &out) {
            // For each hole of the current expolygon:
            out.insert(out.end(), polynode.Contour.begin(), polynode.Contour.end());
            for (int i = 0; i < polynode.ChildCount(); ++ i)
                collect_points_recursive(*polynode.Childs[i], out);
        }
    };
    Clipper3r::Path pts;
    for (int i = 0; i < polytree.ChildCount(); ++ i)
        Helper::collect_points_recursive(*polytree.Childs[i], pts);
    return has_duplicate_points(std::move(pts));
}
#else
// Local test inside each of the contours.
bool has_duplicate_points(const Clipper3r::PolyTree &polytree)
{
    struct Helper {
        static bool has_duplicate_points_recursive(const Clipper3r::PolyNode &polynode) {
            if (has_duplicate_points(polynode.Contour))
                return true;
            for (int i = 0; i < polynode.ChildCount(); ++ i)
                if (has_duplicate_points_recursive(*polynode.Childs[i]))
                    return true;
            return false;
        }
    };
    Clipper3r::Path pts;
    for (int i = 0; i < polytree.ChildCount(); ++ i)
        if (Helper::has_duplicate_points_recursive(*polytree.Childs[i]))
            return true;
    return false;
}
#endif

// Offset CCW contours outside, CW contours (holes) inside.
// Don't calculate union of the output paths.
template<typename PathsProvider, Clipper3r::EndType endType = Clipper3r::etClosedPolygon>
static Clipper3r::Paths raw_offset(PathsProvider &&paths, float offset, Clipper3r::JoinType joinType, double miterLimit)
{
    Clipper3r::ClipperOffset co;
    Clipper3r::Paths out;
    out.reserve(paths.size());
    Clipper3r::Paths out_this;
    if (joinType == jtRound)
        co.ArcTolerance = miterLimit;
    else
        co.MiterLimit = miterLimit;
    co.ShortestEdgeLength = double(std::abs(offset * CLIPPER_OFFSET_SHORTEST_EDGE_FACTOR));
    for (const Clipper3r::Path &path : paths) {
        co.Clear();
        // Execute reorients the contours so that the outer most contour has a positive area. Thus the output
        // contours will be CCW oriented even though the input paths are CW oriented.
        // Offset is applied after contour reorientation, thus the signum of the offset value is reversed.
        co.AddPath(path, joinType, endType);
        bool ccw = endType == Clipper3r::etClosedPolygon ? Clipper3r::Orientation(path) : true;
        co.Execute(out_this, ccw ? offset : - offset);
        if (! ccw) {
            // Reverse the resulting contours.
            for (Clipper3r::Path &path : out_this)
                std::reverse(path.begin(), path.end());
        }
        append(out, std::move(out_this));
    }
    return out;
}

// Offset outside by 10um, one by one.
template<typename PathsProvider>
static Clipper3r::Paths safety_offset(PathsProvider &&paths)
{
    return raw_offset(std::forward<PathsProvider>(paths), ClipperSafetyOffset, DefaultJoinType, DefaultMiterLimit);
}

template<class TResult, class TSubj, class TClip>
TResult clipper_do(
    const Clipper3r::ClipType     clipType,
    TSubj &&                       subject,
    TClip &&                       clip,
    const Clipper3r::PolyFillType fillType)
{
    Clipper3r::Clipper clipper;
    clipper.AddPaths(std::forward<TSubj>(subject), Clipper3r::ptSubject, true);
    clipper.AddPaths(std::forward<TClip>(clip),    Clipper3r::ptClip,    true);
    TResult retval;
    clipper.Execute(clipType, retval, fillType, fillType);
    return retval;
}

template<class TResult, class TSubj, class TClip>
TResult clipper_do(
    const Clipper3r::ClipType     clipType,
    TSubj &&                       subject,
    TClip &&                       clip,
    const Clipper3r::PolyFillType fillType,
    const ApplySafetyOffset        do_safety_offset)
{
    // Safety offset only allowed on intersection and difference.
    assert(do_safety_offset == ApplySafetyOffset::No || clipType != Clipper3r::ctUnion);
    return do_safety_offset == ApplySafetyOffset::Yes ? 
        clipper_do<TResult>(clipType, std::forward<TSubj>(subject), safety_offset(std::forward<TClip>(clip)), fillType) :
        clipper_do<TResult>(clipType, std::forward<TSubj>(subject), std::forward<TClip>(clip), fillType);
}

template<class TResult, class TSubj>
TResult clipper_union(
    TSubj &&                       subject,
    // fillType pftNonZero and pftPositive "should" produce the same result for "normalized with implicit union" set of polygons
    const Clipper3r::PolyFillType fillType = Clipper3r::pftNonZero)
{
    Clipper3r::Clipper clipper;
    clipper.AddPaths(std::forward<TSubj>(subject), Clipper3r::ptSubject, true);
    TResult retval;
    clipper.Execute(Clipper3r::ctUnion, retval, fillType, fillType);
    return retval;
}

// Perform union of input polygons using the positive rule, convert to ExPolygons.
//FIXME is there any benefit of not doing the boolean / using pftEvenOdd?
ExPolygons ClipperPaths_to_Slic3rExPolygons(const Clipper3r::Paths &input, bool do_union)
{
    return PolyTreeToExPolygons(clipper_union<Clipper3r::PolyTree>(input, do_union ? Clipper3r::pftNonZero : Clipper3r::pftEvenOdd));
}

template<typename PathsProvider, Clipper3r::EndType endType = Clipper3r::etClosedPolygon>
static Clipper3r::Paths raw_offset_polyline(PathsProvider &&paths, float offset, Clipper3r::JoinType joinType, double miterLimit)
{
    assert(offset > 0);
    return raw_offset<PathsProvider, Clipper3r::etOpenButt>(std::forward<PathsProvider>(paths), offset, joinType, miterLimit);
}

template<class TResult, typename PathsProvider>
static TResult expand_paths(PathsProvider &&paths, float offset, Clipper3r::JoinType joinType, double miterLimit)
{
    // BBS
    //assert(offset > 0);
    return clipper_union<TResult>(raw_offset(std::forward<PathsProvider>(paths), offset, joinType, miterLimit));
}

// used by shrink_paths()
template<class Container> static void remove_outermost_polygon(Container & solution);
template<> void remove_outermost_polygon<Clipper3r::Paths>(Clipper3r::Paths &solution)
    { if (! solution.empty()) solution.erase(solution.begin()); }
template<> void remove_outermost_polygon<Clipper3r::PolyTree>(Clipper3r::PolyTree &solution)
    { solution.RemoveOutermostPolygon(); }

template<class TResult, typename PathsProvider>
static TResult shrink_paths(PathsProvider &&paths, float offset, Clipper3r::JoinType joinType, double miterLimit)
{
    // BBS
    //assert(offset > 0);
    TResult out;
    if (auto raw = raw_offset(std::forward<PathsProvider>(paths), - offset, joinType, miterLimit); ! raw.empty()) {
        Clipper3r::Clipper clipper;
        clipper.AddPaths(raw, Clipper3r::ptSubject, true);
        Clipper3r::IntRect r = clipper.GetBounds();
        clipper.AddPath({ { r.left - 10, r.bottom + 10 }, { r.right + 10, r.bottom + 10 }, { r.right + 10, r.top - 10 }, { r.left - 10, r.top - 10 } }, Clipper3r::ptSubject, true);
        clipper.ReverseSolution(true);
        clipper.Execute(Clipper3r::ctUnion, out, Clipper3r::pftNegative, Clipper3r::pftNegative);
        remove_outermost_polygon(out);
    }
    return out;
}

template<class TResult, typename PathsProvider>
static TResult offset_paths(PathsProvider &&paths, float offset, Clipper3r::JoinType joinType, double miterLimit)
{
    // BBS
    //assert(offset != 0);

    return offset > 0 ?
        expand_paths<TResult>(std::forward<PathsProvider>(paths),   offset, joinType, miterLimit) :
        shrink_paths<TResult>(std::forward<PathsProvider>(paths), - offset, joinType, miterLimit);
}

Slic3r::Polygons offset(const Slic3r::Polygon &polygon, const float delta, Clipper3r::JoinType joinType, double miterLimit)
    { return to_polygons(raw_offset(ClipperUtils::SinglePathProvider(polygon.points), delta, joinType, miterLimit)); }

Slic3r::Polygons offset(const Slic3r::Polygons &polygons, const float delta, Clipper3r::JoinType joinType, double miterLimit)
    { return to_polygons(offset_paths<Clipper3r::Paths>(ClipperUtils::PolygonsProvider(polygons), delta, joinType, miterLimit)); }
Slic3r::ExPolygons offset_ex(const Slic3r::Polygons &polygons, const float delta, Clipper3r::JoinType joinType, double miterLimit)
    { return PolyTreeToExPolygons(offset_paths<Clipper3r::PolyTree>(ClipperUtils::PolygonsProvider(polygons), delta, joinType, miterLimit)); }

Slic3r::Polygons offset(const Slic3r::Polyline &polyline, const float delta, Clipper3r::JoinType joinType, double miterLimit)
    { assert(delta > 0); return to_polygons(clipper_union<Clipper3r::Paths>(raw_offset_polyline(ClipperUtils::SinglePathProvider(polyline.points), delta, joinType, miterLimit))); }
Slic3r::Polygons offset(const Slic3r::Polylines &polylines, const float delta, Clipper3r::JoinType joinType, double miterLimit)
    { assert(delta > 0); return to_polygons(clipper_union<Clipper3r::Paths>(raw_offset_polyline(ClipperUtils::PolylinesProvider(polylines), delta, joinType, miterLimit))); }

// returns number of expolygons collected (0 or 1).
static int offset_expolygon_inner(const Slic3r::ExPolygon &expoly, const float delta, Clipper3r::JoinType joinType, double miterLimit, Clipper3r::Paths &out)
{
    // 1) Offset the outer contour.
    Clipper3r::Paths contours;
    {
        Clipper3r::ClipperOffset co;
        if (joinType == jtRound)
            co.ArcTolerance = miterLimit;
        else
            co.MiterLimit = miterLimit;
        co.ShortestEdgeLength = double(std::abs(delta * CLIPPER_OFFSET_SHORTEST_EDGE_FACTOR));
        co.AddPath(expoly.contour.points, joinType, Clipper3r::etClosedPolygon);
        co.Execute(contours, delta);
    }
    if (contours.empty())
        // No need to try to offset the holes.
        return 0;

    if (expoly.holes.empty()) {
        // No need to subtract holes from the offsetted expolygon, we are done.
        append(out, std::move(contours));
    } else {
        // 2) Offset the holes one by one, collect the offsetted holes.
        Clipper3r::Paths holes;
        {
            for (const Polygon &hole : expoly.holes) {
                Clipper3r::ClipperOffset co;
                if (joinType == jtRound)
                    co.ArcTolerance = miterLimit;
                else
                    co.MiterLimit = miterLimit;
                co.ShortestEdgeLength = double(std::abs(delta * CLIPPER_OFFSET_SHORTEST_EDGE_FACTOR));
                co.AddPath(hole.points, joinType, Clipper3r::etClosedPolygon);
                Clipper3r::Paths out2;
                // Execute reorients the contours so that the outer most contour has a positive area. Thus the output
                // contours will be CCW oriented even though the input paths are CW oriented.
                // Offset is applied after contour reorientation, thus the signum of the offset value is reversed.
                co.Execute(out2, - delta);
                append(holes, std::move(out2));
            }
        }

        // 3) Subtract holes from the contours.
        if (holes.empty()) {
            // No hole remaining after an offset. Just copy the outer contour.
            append(out, std::move(contours));
        } else if (delta < 0) {
            // Negative offset. There is a chance, that the offsetted hole intersects the outer contour. 
            // Subtract the offsetted holes from the offsetted contours.            
            if (auto output = clipper_do<Clipper3r::Paths>(Clipper3r::ctDifference, contours, holes, Clipper3r::pftNonZero); ! output.empty()) {
                append(out, std::move(output));
            } else {
                // The offsetted holes have eaten up the offsetted outer contour.
                return 0;
            }
        } else {
            // Positive offset. As long as the Clipper offset does what one expects it to do, the offsetted hole will have a smaller
            // area than the original hole or even disappear, therefore there will be no new intersections.
            // Just collect the reversed holes.
            out.reserve(contours.size() + holes.size());
            append(out, std::move(contours));
            // Reverse the holes in place.
            for (size_t i = 0; i < holes.size(); ++ i)
                std::reverse(holes[i].begin(), holes[i].end());
            append(out, std::move(holes));
        }
    }

    return 1;
}

static int offset_expolygon_inner(const Slic3r::Surface &surface, const float delta, Clipper3r::JoinType joinType, double miterLimit, Clipper3r::Paths &out)
    { return offset_expolygon_inner(surface.expolygon, delta, joinType, miterLimit, out); }
static int offset_expolygon_inner(const Slic3r::Surface *surface, const float delta, Clipper3r::JoinType joinType, double miterLimit, Clipper3r::Paths &out)
    { return offset_expolygon_inner(surface->expolygon, delta, joinType, miterLimit, out); }

Clipper3r::Paths expolygon_offset(const Slic3r::ExPolygon &expolygon, const float delta, Clipper3r::JoinType joinType, double miterLimit)
{
    Clipper3r::Paths out;
    offset_expolygon_inner(expolygon, delta, joinType, miterLimit, out);
    return out;
}

// This is a safe variant of the polygons offset, tailored for multiple ExPolygons.
// It is required, that the input expolygons do not overlap and that the holes of each ExPolygon don't intersect with their respective outer contours.
// Each ExPolygon is offsetted separately. For outer offset, the the offsetted ExPolygons shall be united outside of this function.
template<typename ExPolygonVector>
static std::pair<Clipper3r::Paths, size_t> expolygons_offset_raw(const ExPolygonVector &expolygons, const float delta, Clipper3r::JoinType joinType, double miterLimit)
{
    // Offsetted ExPolygons before they are united.
    Clipper3r::Paths output;
    output.reserve(expolygons.size());
    // How many non-empty offsetted expolygons were actually collected into output?
    // If only one, then there is no need to do a final union.
    size_t expolygons_collected = 0;
    for (const auto &expoly : expolygons)
        expolygons_collected += offset_expolygon_inner(expoly, delta, joinType, miterLimit, output);
    return std::make_pair(std::move(output), expolygons_collected);
}

// See comment on expolygon_offsets_raw. In addition, for positive offset the contours are united.
template<typename ExPolygonVector>
static Clipper3r::Paths expolygons_offset(const ExPolygonVector &expolygons, const float delta, Clipper3r::JoinType joinType, double miterLimit)
{
    auto [output, expolygons_collected] = expolygons_offset_raw(expolygons, delta, joinType, miterLimit);
    // Unite the offsetted expolygons.
    return expolygons_collected > 1 && delta > 0 ?
        // There is a chance that the outwards offsetted expolygons may intersect. Perform a union.
        clipper_union<Clipper3r::Paths>(output) :
        // Negative offset. The shrunk expolygons shall not mutually intersect. Just copy the output.
        output;
}

// See comment on expolygons_offset_raw. In addition, the polygons are always united to conver to polytree.
template<typename ExPolygonVector>
static Clipper3r::PolyTree expolygons_offset_pt(const ExPolygonVector &expolygons, const float delta, Clipper3r::JoinType joinType, double miterLimit)
{
    auto [output, expolygons_collected] = expolygons_offset_raw(expolygons, delta, joinType, miterLimit);
    // Unite the offsetted expolygons for both the 
    return clipper_union<Clipper3r::PolyTree>(output);
}

Slic3r::Polygons offset(const Slic3r::ExPolygon &expolygon, const float delta, Clipper3r::JoinType joinType, double miterLimit)
    { return to_polygons(expolygon_offset(expolygon, delta, joinType, miterLimit)); }
Slic3r::Polygons offset(const Slic3r::ExPolygons &expolygons, const float delta, Clipper3r::JoinType joinType, double miterLimit)
    { return to_polygons(expolygons_offset(expolygons, delta, joinType, miterLimit)); }
Slic3r::Polygons offset(const Slic3r::Surfaces &surfaces, const float delta, Clipper3r::JoinType joinType, double miterLimit)
    { return to_polygons(expolygons_offset(surfaces, delta, joinType, miterLimit)); }
Slic3r::Polygons offset(const Slic3r::SurfacesPtr &surfaces, const float delta, Clipper3r::JoinType joinType, double miterLimit)
    { return to_polygons(expolygons_offset(surfaces, delta, joinType, miterLimit)); }
Slic3r::ExPolygons offset_ex(const Slic3r::ExPolygon &expolygon, const float delta, Clipper3r::JoinType joinType, double miterLimit)
    //FIXME one may spare one Clipper Union call.
    { return ClipperPaths_to_Slic3rExPolygons(expolygon_offset(expolygon, delta, joinType, miterLimit)); }
Slic3r::ExPolygons offset_ex(const Slic3r::ExPolygons &expolygons, const float delta, Clipper3r::JoinType joinType, double miterLimit)
    { return PolyTreeToExPolygons(expolygons_offset_pt(expolygons, delta, joinType, miterLimit)); }
Slic3r::ExPolygons offset_ex(const Slic3r::Surfaces &surfaces, const float delta, Clipper3r::JoinType joinType, double miterLimit)
    { return PolyTreeToExPolygons(expolygons_offset_pt(surfaces, delta, joinType, miterLimit)); }

Polygons offset2(const ExPolygons &expolygons, const float delta1, const float delta2, Clipper3r::JoinType joinType, double miterLimit)
{
    return to_polygons(offset_paths<Clipper3r::Paths>(expolygons_offset(expolygons, delta1, joinType, miterLimit), delta2, joinType, miterLimit));
}
ExPolygons offset2_ex(const ExPolygons &expolygons, const float delta1, const float delta2, Clipper3r::JoinType joinType, double miterLimit)
{
    return PolyTreeToExPolygons(offset_paths<Clipper3r::PolyTree>(expolygons_offset(expolygons, delta1, joinType, miterLimit), delta2, joinType, miterLimit));
}
ExPolygons offset2_ex(const Surfaces &surfaces, const float delta1, const float delta2, Clipper3r::JoinType joinType, double miterLimit)
{
    //FIXME it may be more efficient to offset to_expolygons(surfaces) instead of to_polygons(surfaces).
    return PolyTreeToExPolygons(offset_paths<Clipper3r::PolyTree>(expolygons_offset(surfaces, delta1, joinType, miterLimit), delta2, joinType, miterLimit));
}

// Offset outside, then inside produces morphological closing. All deltas should be positive.
Slic3r::Polygons closing(const Slic3r::Polygons &polygons, const float delta1, const float delta2, Clipper3r::JoinType joinType, double miterLimit)
{
    assert(delta1 > 0);
    assert(delta2 > 0);
    return to_polygons(shrink_paths<Clipper3r::Paths>(expand_paths<Clipper3r::Paths>(ClipperUtils::PolygonsProvider(polygons), delta1, joinType, miterLimit), delta2, joinType, miterLimit));
}
Slic3r::ExPolygons closing_ex(const Slic3r::Polygons &polygons, const float delta1, const float delta2, Clipper3r::JoinType joinType, double miterLimit)
{
    assert(delta1 > 0);
    assert(delta2 > 0);
    return PolyTreeToExPolygons(shrink_paths<Clipper3r::PolyTree>(expand_paths<Clipper3r::Paths>(ClipperUtils::PolygonsProvider(polygons), delta1, joinType, miterLimit), delta2, joinType, miterLimit));
}
Slic3r::ExPolygons closing_ex(const Slic3r::Surfaces &surfaces, const float delta1, const float delta2, Clipper3r::JoinType joinType, double miterLimit)
{
    assert(delta1 > 0);
    assert(delta2 > 0);
    //FIXME it may be more efficient to offset to_expolygons(surfaces) instead of to_polygons(surfaces).
    return PolyTreeToExPolygons(shrink_paths<Clipper3r::PolyTree>(expand_paths<Clipper3r::Paths>(ClipperUtils::SurfacesProvider(surfaces), delta1, joinType, miterLimit), delta2, joinType, miterLimit));
}

// Offset inside, then outside produces morphological opening. All deltas should be positive.
Slic3r::Polygons opening(const Slic3r::Polygons &polygons, const float delta1, const float delta2, Clipper3r::JoinType joinType, double miterLimit)
{
    assert(delta1 > 0);
    assert(delta2 > 0);
    return to_polygons(expand_paths<Clipper3r::Paths>(shrink_paths<Clipper3r::Paths>(ClipperUtils::PolygonsProvider(polygons), delta1, joinType, miterLimit), delta2, joinType, miterLimit));
}
Slic3r::Polygons opening(const Slic3r::ExPolygons &expolygons, const float delta1, const float delta2, Clipper3r::JoinType joinType, double miterLimit)
{
    assert(delta1 > 0);
    assert(delta2 > 0);
    return to_polygons(expand_paths<Clipper3r::Paths>(shrink_paths<Clipper3r::Paths>(ClipperUtils::ExPolygonsProvider(expolygons), delta1, joinType, miterLimit), delta2, joinType, miterLimit));
}
Slic3r::Polygons opening(const Slic3r::Surfaces &surfaces, const float delta1, const float delta2, Clipper3r::JoinType joinType, double miterLimit)
{
    assert(delta1 > 0);
    assert(delta2 > 0);
    //FIXME it may be more efficient to offset to_expolygons(surfaces) instead of to_polygons(surfaces).
    return to_polygons(expand_paths<Clipper3r::Paths>(shrink_paths<Clipper3r::Paths>(ClipperUtils::SurfacesProvider(surfaces), delta1, joinType, miterLimit), delta2, joinType, miterLimit));
}

// Fix of #117: A large fractal pyramid takes ages to slice
// The Clipper library has difficulties processing overlapping polygons.
// Namely, the function Clipper3r::JoinCommonEdges() has potentially a terrible time complexity if the output
// of the operation is of the PolyTree type.
// This function implemenets a following workaround:
// 1) Peform the Clipper operation with the output to Paths. This method handles overlaps in a reasonable time.
// 2) Run Clipper Union once again to extract the PolyTree from the result of 1).
template<typename PathProvider1, typename PathProvider2>
inline Clipper3r::PolyTree clipper_do_polytree(
    const Clipper3r::ClipType       clipType,
    PathProvider1                  &&subject,
    PathProvider2                  &&clip,
    const Clipper3r::PolyFillType   fillType)
{
    // Perform the operation with the output to input_subject.
    // This pass does not generate a PolyTree, which is a very expensive operation with the current Clipper library
    // if there are overapping edges.
    if (auto output = clipper_do<Clipper3r::Paths>(clipType, subject, clip, fillType); ! output.empty())
        // Perform an additional Union operation to generate the PolyTree ordering.
        return clipper_union<Clipper3r::PolyTree>(output, fillType);
    return Clipper3r::PolyTree();
}
template<typename PathProvider1, typename PathProvider2>
inline Clipper3r::PolyTree clipper_do_polytree(
    const Clipper3r::ClipType       clipType,
    PathProvider1                  &&subject,
    PathProvider2                  &&clip,
    const Clipper3r::PolyFillType   fillType,
    const ApplySafetyOffset          do_safety_offset)
{
    assert(do_safety_offset == ApplySafetyOffset::No || clipType != Clipper3r::ctUnion);
    return do_safety_offset == ApplySafetyOffset::Yes ? 
        clipper_do_polytree(clipType, std::forward<PathProvider1>(subject), safety_offset(std::forward<PathProvider2>(clip)), fillType) :
        clipper_do_polytree(clipType, std::forward<PathProvider1>(subject), std::forward<PathProvider2>(clip), fillType);
}

template<class TSubj, class TClip>
static inline Polygons _clipper(Clipper3r::ClipType clipType, TSubj &&subject, TClip &&clip, ApplySafetyOffset do_safety_offset)
{
    return to_polygons(clipper_do<Clipper3r::Paths>(clipType, std::forward<TSubj>(subject), std::forward<TClip>(clip), Clipper3r::pftNonZero, do_safety_offset));
}

Slic3r::Polygons diff(const Slic3r::Polygon &subject, const Slic3r::Polygon &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper(Clipper3r::ctDifference, ClipperUtils::SinglePathProvider(subject.points), ClipperUtils::SinglePathProvider(clip.points), do_safety_offset); }
Slic3r::Polygons diff(const Slic3r::Polygons &subject, const Slic3r::Polygons &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper(Clipper3r::ctDifference, ClipperUtils::PolygonsProvider(subject), ClipperUtils::PolygonsProvider(clip), do_safety_offset); }
Slic3r::Polygons diff(const Slic3r::Polygons &subject, const Slic3r::ExPolygons &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper(Clipper3r::ctDifference, ClipperUtils::PolygonsProvider(subject), ClipperUtils::ExPolygonsProvider(clip), do_safety_offset); }
Slic3r::Polygons diff(const Slic3r::ExPolygons &subject, const Slic3r::Polygons &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper(Clipper3r::ctDifference, ClipperUtils::ExPolygonsProvider(subject), ClipperUtils::PolygonsProvider(clip), do_safety_offset); }
Slic3r::Polygons diff(const Slic3r::ExPolygons &subject, const Slic3r::ExPolygons &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper(Clipper3r::ctDifference, ClipperUtils::ExPolygonsProvider(subject), ClipperUtils::ExPolygonsProvider(clip), do_safety_offset); }
Slic3r::Polygons diff(const Slic3r::Surfaces &subject, const Slic3r::Polygons &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper(Clipper3r::ctDifference, ClipperUtils::SurfacesProvider(subject), ClipperUtils::PolygonsProvider(clip), do_safety_offset); }
Slic3r::Polygons intersection(const Slic3r::Polygon &subject, const Slic3r::Polygon &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper(Clipper3r::ctIntersection, ClipperUtils::SinglePathProvider(subject.points), ClipperUtils::SinglePathProvider(clip.points), do_safety_offset); }
Slic3r::Polygons intersection(const Slic3r::Polygons &subject, const Slic3r::ExPolygon &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper(Clipper3r::ctIntersection, ClipperUtils::PolygonsProvider(subject), ClipperUtils::ExPolygonProvider(clip), do_safety_offset); }
Slic3r::Polygons intersection(const Slic3r::Polygons &subject, const Slic3r::Polygons &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper(Clipper3r::ctIntersection, ClipperUtils::PolygonsProvider(subject), ClipperUtils::PolygonsProvider(clip), do_safety_offset); }
Slic3r::Polygons intersection(const Slic3r::ExPolygon &subject, const Slic3r::ExPolygon &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper(Clipper3r::ctIntersection, ClipperUtils::ExPolygonProvider(subject), ClipperUtils::ExPolygonProvider(clip), do_safety_offset); }
Slic3r::Polygons intersection(const Slic3r::ExPolygons &subject, const Slic3r::Polygons &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper(Clipper3r::ctIntersection, ClipperUtils::ExPolygonsProvider(subject), ClipperUtils::PolygonsProvider(clip), do_safety_offset); }
Slic3r::Polygons intersection(const Slic3r::ExPolygons &subject, const Slic3r::ExPolygons &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper(Clipper3r::ctIntersection, ClipperUtils::ExPolygonsProvider(subject), ClipperUtils::ExPolygonsProvider(clip), do_safety_offset); }
Slic3r::Polygons intersection(const Slic3r::Surfaces &subject, const Slic3r::Polygons &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper(Clipper3r::ctIntersection, ClipperUtils::SurfacesProvider(subject), ClipperUtils::PolygonsProvider(clip), do_safety_offset); }
Slic3r::Polygons intersection(const Slic3r::Surfaces &subject, const Slic3r::ExPolygons &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper(Clipper3r::ctIntersection, ClipperUtils::SurfacesProvider(subject), ClipperUtils::ExPolygonsProvider(clip), do_safety_offset); }
// BBS
Slic3r::Polygons intersection(const Slic3r::Polygons& subject, const Slic3r::Polygon& clip, ApplySafetyOffset do_safety_offset)
{
    Slic3r::Polygons clip_temp;
    clip_temp.push_back(clip);
    return intersection(subject, clip_temp, do_safety_offset);
}

Slic3r::Polygons union_(const Slic3r::Polygons &subject)
    { return _clipper(Clipper3r::ctUnion, ClipperUtils::PolygonsProvider(subject), ClipperUtils::EmptyPathsProvider(), ApplySafetyOffset::No); }
Slic3r::Polygons union_(const Slic3r::ExPolygons &subject)
    { return _clipper(Clipper3r::ctUnion, ClipperUtils::ExPolygonsProvider(subject), ClipperUtils::EmptyPathsProvider(), ApplySafetyOffset::No); }
Slic3r::Polygons union_(const Slic3r::Polygons &subject, const Clipper3r::PolyFillType fillType)
    { return to_polygons(clipper_do<Clipper3r::Paths>(Clipper3r::ctUnion, ClipperUtils::PolygonsProvider(subject), ClipperUtils::EmptyPathsProvider(), fillType, ApplySafetyOffset::No)); }
Slic3r::Polygons union_(const Slic3r::Polygons &subject, const Slic3r::Polygons &subject2)
    {
        // BBS
        Polygons polys = subject;
        for (const Polygon& poly : subject2)
            polys.push_back(poly);
        return union_(polys);
    }

template <typename TSubject, typename TClip>
static ExPolygons _clipper_ex(Clipper3r::ClipType clipType, TSubject &&subject,  TClip &&clip, ApplySafetyOffset do_safety_offset, Clipper3r::PolyFillType fill_type = Clipper3r::pftNonZero)
    { return PolyTreeToExPolygons(clipper_do_polytree(clipType, std::forward<TSubject>(subject), std::forward<TClip>(clip), fill_type, do_safety_offset)); }

Slic3r::ExPolygons diff_ex(const Slic3r::Polygons &subject, const Slic3r::Polygons &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper_ex(Clipper3r::ctDifference, ClipperUtils::PolygonsProvider(subject), ClipperUtils::PolygonsProvider(clip), do_safety_offset); }
Slic3r::ExPolygons diff_ex(const Slic3r::Polygons &subject, const Slic3r::Surfaces &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper_ex(Clipper3r::ctDifference, ClipperUtils::PolygonsProvider(subject), ClipperUtils::SurfacesProvider(clip), do_safety_offset); }
Slic3r::ExPolygons diff_ex(const Slic3r::Polygons &subject, const Slic3r::ExPolygons &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper_ex(Clipper3r::ctDifference, ClipperUtils::PolygonsProvider(subject), ClipperUtils::ExPolygonsProvider(clip), do_safety_offset); }
Slic3r::ExPolygons diff_ex(const Slic3r::ExPolygon &subject, const Slic3r::Polygon &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper_ex(Clipper3r::ctDifference, ClipperUtils::ExPolygonProvider(subject), ClipperUtils::SinglePathProvider(clip.points), do_safety_offset); }
Slic3r::ExPolygons diff_ex(const Slic3r::ExPolygon &subject, const Slic3r::Polygons &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper_ex(Clipper3r::ctDifference, ClipperUtils::ExPolygonProvider(subject), ClipperUtils::PolygonsProvider(clip), do_safety_offset); }
Slic3r::ExPolygons diff_ex(const Slic3r::ExPolygons &subject, const Slic3r::Polygons &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper_ex(Clipper3r::ctDifference, ClipperUtils::ExPolygonsProvider(subject), ClipperUtils::PolygonsProvider(clip), do_safety_offset); }
Slic3r::ExPolygons diff_ex(const Slic3r::ExPolygons &subject, const Slic3r::ExPolygons &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper_ex(Clipper3r::ctDifference, ClipperUtils::ExPolygonsProvider(subject), ClipperUtils::ExPolygonsProvider(clip), do_safety_offset); }
Slic3r::ExPolygons diff_ex(const Slic3r::Surfaces &subject, const Slic3r::Polygons &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper_ex(Clipper3r::ctDifference, ClipperUtils::SurfacesProvider(subject), ClipperUtils::PolygonsProvider(clip), do_safety_offset); }
Slic3r::ExPolygons diff_ex(const Slic3r::Surfaces &subject, const Slic3r::ExPolygons &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper_ex(Clipper3r::ctDifference, ClipperUtils::SurfacesProvider(subject), ClipperUtils::ExPolygonsProvider(clip), do_safety_offset); }
Slic3r::ExPolygons diff_ex(const Slic3r::ExPolygons &subject, const Slic3r::Surfaces &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper_ex(Clipper3r::ctDifference, ClipperUtils::ExPolygonsProvider(subject), ClipperUtils::SurfacesProvider(clip), do_safety_offset); }
Slic3r::ExPolygons diff_ex(const Slic3r::Surfaces &subject, const Slic3r::Surfaces &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper_ex(Clipper3r::ctDifference, ClipperUtils::SurfacesProvider(subject), ClipperUtils::SurfacesProvider(clip), do_safety_offset); }
Slic3r::ExPolygons diff_ex(const Slic3r::SurfacesPtr &subject, const Slic3r::Polygons &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper_ex(Clipper3r::ctDifference, ClipperUtils::SurfacesPtrProvider(subject), ClipperUtils::PolygonsProvider(clip), do_safety_offset); }
// BBS
inline Slic3r::ExPolygons diff_ex(const Slic3r::Polygon& subject, const Slic3r::Polygons& clip, ApplySafetyOffset do_safety_offset)
{
    Slic3r::Polygons subject_temp;
    subject_temp.push_back(subject);

    return diff_ex(subject_temp, clip, do_safety_offset);
}

inline Slic3r::ExPolygons diff_ex(const Slic3r::Polygon& subject, const Slic3r::Polygon& clip, ApplySafetyOffset do_safety_offset)
{
    Slic3r::Polygons subject_temp;
    Slic3r::Polygons clip_temp;

    subject_temp.push_back(subject);
    clip_temp.push_back(clip);
    return diff_ex(subject_temp, clip_temp, do_safety_offset);
}

Slic3r::ExPolygons intersection_ex(const Slic3r::Polygons &subject, const Slic3r::Polygons &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper_ex(Clipper3r::ctIntersection, ClipperUtils::PolygonsProvider(subject), ClipperUtils::PolygonsProvider(clip), do_safety_offset); }
Slic3r::ExPolygons intersection_ex(const Slic3r::ExPolygon &subject, const Slic3r::Polygons &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper_ex(Clipper3r::ctIntersection, ClipperUtils::ExPolygonProvider(subject), ClipperUtils::PolygonsProvider(clip), do_safety_offset); }
Slic3r::ExPolygons intersection_ex(const Slic3r::ExPolygon& subject, const Slic3r::ExPolygon& clip, ApplySafetyOffset do_safety_offset)
    { return _clipper_ex(Clipper3r::ctIntersection, ClipperUtils::ExPolygonProvider(subject), ClipperUtils::ExPolygonProvider(clip), do_safety_offset); }
Slic3r::ExPolygons intersection_ex(const Slic3r::Polygons &subject, const Slic3r::ExPolygons &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper_ex(Clipper3r::ctIntersection, ClipperUtils::PolygonsProvider(subject), ClipperUtils::ExPolygonsProvider(clip), do_safety_offset); }
Slic3r::ExPolygons intersection_ex(const Slic3r::ExPolygons &subject, const Slic3r::Polygons &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper_ex(Clipper3r::ctIntersection, ClipperUtils::ExPolygonsProvider(subject), ClipperUtils::PolygonsProvider(clip), do_safety_offset); }
Slic3r::ExPolygons intersection_ex(const Slic3r::ExPolygons& subject, const Slic3r::ExPolygon& clip, ApplySafetyOffset do_safety_offset)
    { return _clipper_ex(Clipper3r::ctIntersection, ClipperUtils::ExPolygonsProvider(subject), ClipperUtils::ExPolygonProvider(clip), do_safety_offset);}
Slic3r::ExPolygons intersection_ex(const Slic3r::ExPolygon& subject, const Slic3r::ExPolygons& clip, ApplySafetyOffset do_safety_offset)
    { return _clipper_ex(Clipper3r::ctIntersection, ClipperUtils::ExPolygonProvider(subject), ClipperUtils::ExPolygonsProvider(clip), do_safety_offset);}
Slic3r::ExPolygons intersection_ex(const Slic3r::ExPolygons &subject, const Slic3r::ExPolygons &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper_ex(Clipper3r::ctIntersection, ClipperUtils::ExPolygonsProvider(subject), ClipperUtils::ExPolygonsProvider(clip), do_safety_offset); }
Slic3r::ExPolygons intersection_ex(const Slic3r::Surfaces &subject, const Slic3r::Polygons &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper_ex(Clipper3r::ctIntersection, ClipperUtils::SurfacesProvider(subject), ClipperUtils::PolygonsProvider(clip), do_safety_offset); }
Slic3r::ExPolygons intersection_ex(const Slic3r::Surfaces &subject, const Slic3r::ExPolygons &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper_ex(Clipper3r::ctIntersection, ClipperUtils::SurfacesProvider(subject), ClipperUtils::ExPolygonsProvider(clip), do_safety_offset); }
Slic3r::ExPolygons intersection_ex(const Slic3r::Surfaces &subject, const Slic3r::Surfaces &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper_ex(Clipper3r::ctIntersection, ClipperUtils::SurfacesProvider(subject), ClipperUtils::SurfacesProvider(clip), do_safety_offset); }
Slic3r::ExPolygons intersection_ex(const Slic3r::SurfacesPtr &subject, const Slic3r::ExPolygons &clip, ApplySafetyOffset do_safety_offset)
    { return _clipper_ex(Clipper3r::ctIntersection, ClipperUtils::SurfacesPtrProvider(subject), ClipperUtils::ExPolygonsProvider(clip), do_safety_offset); }
// May be used to "heal" unusual models (3DLabPrints etc.) by providing fill_type (pftEvenOdd, pftNonZero, pftPositive, pftNegative).
Slic3r::ExPolygons union_ex(const Slic3r::Polygons &subject, Clipper3r::PolyFillType fill_type)
    { return _clipper_ex(Clipper3r::ctUnion, ClipperUtils::PolygonsProvider(subject), ClipperUtils::EmptyPathsProvider(), ApplySafetyOffset::No, fill_type); }
Slic3r::ExPolygons union_ex(const Slic3r::ExPolygons &subject)
    { return PolyTreeToExPolygons(clipper_do_polytree(Clipper3r::ctUnion, ClipperUtils::ExPolygonsProvider(subject), ClipperUtils::EmptyPathsProvider(), Clipper3r::pftNonZero)); }
Slic3r::ExPolygons union_ex(const Slic3r::Surfaces &subject)
    { return PolyTreeToExPolygons(clipper_do_polytree(Clipper3r::ctUnion, ClipperUtils::SurfacesProvider(subject), ClipperUtils::EmptyPathsProvider(), Clipper3r::pftNonZero)); }
// BBS
Slic3r::ExPolygons union_ex(const Slic3r::ExPolygons& poly1, const Slic3r::ExPolygons& poly2, bool safety_offset_)
    {
    ExPolygons expolys = poly1;
    for (const ExPolygon& expoly : poly2)
        expolys.push_back(expoly);
    return union_ex(expolys);
    }

template<typename PathsProvider1, typename PathsProvider2>
Polylines _clipper_pl_open(Clipper3r::ClipType clipType, PathsProvider1 &&subject, PathsProvider2 &&clip)
{
    Clipper3r::Clipper clipper;
    clipper.AddPaths(std::forward<PathsProvider1>(subject), Clipper3r::ptSubject, false);
    clipper.AddPaths(std::forward<PathsProvider2>(clip), Clipper3r::ptClip, true);
    Clipper3r::PolyTree retval;
    clipper.Execute(clipType, retval, Clipper3r::pftNonZero, Clipper3r::pftNonZero);
    return PolyTreeToPolylines(std::move(retval));
}

// If the split_at_first_point() call above happens to split the polygon inside the clipping area
// we would get two consecutive polylines instead of a single one, so we go through them in order
// to recombine continuous polylines.
static void _clipper_pl_recombine(Polylines &polylines)
{
    for (size_t i = 0; i < polylines.size(); ++i) {
        for (size_t j = i+1; j < polylines.size(); ++j) {
            if (polylines[i].points.back() == polylines[j].points.front()) {
                /* If last point of i coincides with first point of j,
                   append points of j to i and delete j */
                polylines[i].points.insert(polylines[i].points.end(), polylines[j].points.begin()+1, polylines[j].points.end());
                polylines.erase(polylines.begin() + j);
                --j;
            } else if (polylines[i].points.front() == polylines[j].points.back()) {
                /* If first point of i coincides with last point of j,
                   prepend points of j to i and delete j */
                polylines[i].points.insert(polylines[i].points.begin(), polylines[j].points.begin(), polylines[j].points.end()-1);
                polylines.erase(polylines.begin() + j);
                --j;
            } else if (polylines[i].points.front() == polylines[j].points.front()) {
                /* Since Clipper does not preserve orientation of polylines, 
                   also check the case when first point of i coincides with first point of j. */
                polylines[j].reverse();
                polylines[i].points.insert(polylines[i].points.begin(), polylines[j].points.begin(), polylines[j].points.end()-1);
                polylines.erase(polylines.begin() + j);
                --j;
            } else if (polylines[i].points.back() == polylines[j].points.back()) {
                /* Since Clipper does not preserve orientation of polylines, 
                   also check the case when last point of i coincides with last point of j. */
                polylines[j].reverse();
                polylines[i].points.insert(polylines[i].points.end(), polylines[j].points.begin()+1, polylines[j].points.end());
                polylines.erase(polylines.begin() + j);
                --j;
            }
        }
    }
}

template<typename PathProvider1, typename PathProvider2>
Polylines _clipper_pl_closed(Clipper3r::ClipType clipType, PathProvider1 &&subject, PathProvider2 &&clip)
{
    // Transform input polygons into open paths.
    Clipper3r::Paths paths;
    paths.reserve(subject.size());
    for (const Points &poly : subject) {
        // Emplace polygon, duplicate the 1st point.
        paths.push_back({});
        Clipper3r::Path &path = paths.back();
        path.reserve(poly.size() + 1);
        path = poly;
        path.emplace_back(poly.front());
    }
    // perform clipping
    Polylines retval = _clipper_pl_open(clipType, paths, std::forward<PathProvider2>(clip));
    _clipper_pl_recombine(retval);
    return retval;
}

Slic3r::Polylines diff_pl(const Slic3r::Polyline& subject, const Slic3r::Polygons& clip)
    { return _clipper_pl_open(Clipper3r::ctDifference, ClipperUtils::SinglePathProvider(subject.points), ClipperUtils::PolygonsProvider(clip)); }
Slic3r::Polylines diff_pl(const Slic3r::Polylines &subject, const Slic3r::Polygons &clip)
    { return _clipper_pl_open(Clipper3r::ctDifference, ClipperUtils::PolylinesProvider(subject), ClipperUtils::PolygonsProvider(clip)); }
Slic3r::Polylines diff_pl(const Slic3r::Polyline &subject, const Slic3r::ExPolygon &clip)
    { return _clipper_pl_open(Clipper3r::ctDifference, ClipperUtils::SinglePathProvider(subject.points), ClipperUtils::ExPolygonProvider(clip)); }
Slic3r::Polylines diff_pl(const Slic3r::Polylines &subject, const Slic3r::ExPolygon &clip)
    { return _clipper_pl_open(Clipper3r::ctDifference, ClipperUtils::PolylinesProvider(subject), ClipperUtils::ExPolygonProvider(clip)); }
Slic3r::Polylines diff_pl(const Slic3r::Polylines &subject, const Slic3r::ExPolygons &clip)
    { return _clipper_pl_open(Clipper3r::ctDifference, ClipperUtils::PolylinesProvider(subject), ClipperUtils::ExPolygonsProvider(clip)); }
Slic3r::Polylines diff_pl(const Slic3r::Polygons &subject, const Slic3r::Polygons &clip)
    { return _clipper_pl_closed(Clipper3r::ctDifference, ClipperUtils::PolygonsProvider(subject), ClipperUtils::PolygonsProvider(clip)); }
Slic3r::Polylines intersection_pl(const Slic3r::Polylines &subject, const Slic3r::Polygon &clip)
    { return _clipper_pl_open(Clipper3r::ctIntersection, ClipperUtils::PolylinesProvider(subject), ClipperUtils::SinglePathProvider(clip.points)); }
Slic3r::Polylines intersection_pl(const Slic3r::Polyline &subject, const Slic3r::ExPolygon &clip)
    { return _clipper_pl_open(Clipper3r::ctIntersection, ClipperUtils::SinglePathProvider(subject.points), ClipperUtils::ExPolygonProvider(clip)); }
Slic3r::Polylines intersection_pl(const Slic3r::Polylines &subject, const Slic3r::ExPolygon &clip)
    { return _clipper_pl_open(Clipper3r::ctIntersection, ClipperUtils::PolylinesProvider(subject), ClipperUtils::ExPolygonProvider(clip)); }
Slic3r::Polylines intersection_pl(const Slic3r::Polyline &subject, const Slic3r::Polygons &clip)
    { return _clipper_pl_open(Clipper3r::ctIntersection, ClipperUtils::SinglePathProvider(subject.points), ClipperUtils::PolygonsProvider(clip)); }
Slic3r::Polylines intersection_pl(const Slic3r::Polylines &subject, const Slic3r::Polygons &clip)
    { return _clipper_pl_open(Clipper3r::ctIntersection, ClipperUtils::PolylinesProvider(subject), ClipperUtils::PolygonsProvider(clip)); }
Slic3r::Polylines intersection_pl(const Slic3r::Polylines &subject, const Slic3r::ExPolygons &clip)
    { return _clipper_pl_open(Clipper3r::ctIntersection, ClipperUtils::PolylinesProvider(subject), ClipperUtils::ExPolygonsProvider(clip)); }
Slic3r::Polylines intersection_pl(const Slic3r::Polygons &subject, const Slic3r::Polygons &clip)
    { return _clipper_pl_closed(Clipper3r::ctIntersection, ClipperUtils::PolygonsProvider(subject), ClipperUtils::PolygonsProvider(clip)); }

Lines _clipper_ln(Clipper3r::ClipType clipType, const Lines &subject, const Polygons &clip)
{
    // convert Lines to Polylines
    Polylines polylines;
    polylines.reserve(subject.size());
    for (const Line &line : subject)
        polylines.emplace_back(Polyline(line.a, line.b));
    
    // perform operation
    polylines = _clipper_pl_open(clipType, ClipperUtils::PolylinesProvider(polylines), ClipperUtils::PolygonsProvider(clip));
    
    // convert Polylines to Lines
    Lines retval;
    for (Polylines::const_iterator polyline = polylines.begin(); polyline != polylines.end(); ++polyline)
        if (polyline->size() >= 2)
            //FIXME It may happen, that Clipper produced a polyline with more than 2 collinear points by clipping a single line with polygons. It is a very rare issue, but it happens, see GH #6933.
            retval.push_back({ polyline->front(), polyline->back() });
    return retval;
}

// Convert polygons / expolygons into Clipper3r::PolyTree using Clipper3r::pftEvenOdd, thus union will NOT be performed.
// If the contours are not intersecting, their orientation shall not be modified by union_pt().
Clipper3r::PolyTree union_pt(const Polygons &subject)
{
    return clipper_do<Clipper3r::PolyTree>(Clipper3r::ctUnion, ClipperUtils::PolygonsProvider(subject), ClipperUtils::EmptyPathsProvider(), Clipper3r::pftEvenOdd);
}
Clipper3r::PolyTree union_pt(const ExPolygons &subject)
{
    return clipper_do<Clipper3r::PolyTree>(Clipper3r::ctUnion, ClipperUtils::ExPolygonsProvider(subject), ClipperUtils::EmptyPathsProvider(), Clipper3r::pftEvenOdd);
}

// Simple spatial ordering of Polynodes
Clipper3r::PolyNodes order_nodes(const Clipper3r::PolyNodes &nodes)
{
    // collect ordering points
    Points ordering_points;
    ordering_points.reserve(nodes.size());
    
    for (const Clipper3r::PolyNode *node : nodes)
        ordering_points.emplace_back(
            Point(node->Contour.front().x(), node->Contour.front().y()));

    // perform the ordering
    Clipper3r::PolyNodes ordered_nodes; /*=
        chain_clipper_polynodes(ordering_points, nodes);*/

    return ordered_nodes;
}

static void traverse_pt_noholes(const Clipper3r::PolyNodes &nodes, Polygons *out)
{
    foreach_node<e_ordering::ON>(nodes, [&out](const Clipper3r::PolyNode *node) 
    {
        traverse_pt_noholes(node->Childs, out);
        out->emplace_back(node->Contour);
        if (node->IsHole()) out->back().reverse(); // ccw
    });
}

static void traverse_pt_outside_in(Clipper3r::PolyNodes &&nodes, Polygons *retval)
{
    // collect ordering points
    Points ordering_points;
    ordering_points.reserve(nodes.size());
    for (const Clipper3r::PolyNode *node : nodes)
        ordering_points.emplace_back(node->Contour.front().x(), node->Contour.front().y());

    // Perform the ordering, push results recursively.
    //FIXME pass the last point to chain_clipper_polynodes?
    //for (Clipper3r::PolyNode *node : chain_clipper_polynodes(ordering_points, nodes)) {
    //    retval->emplace_back(std::move(node->Contour));
    //    if (node->IsHole()) 
    //        // Orient a hole, which is clockwise oriented, to CCW.
    //        retval->back().reverse();
    //    // traverse the next depth
    //    traverse_pt_outside_in(std::move(node->Childs), retval);
    //}
}

Polygons union_pt_chained_outside_in(const Polygons &subject)
{
    Polygons retval;
    traverse_pt_outside_in(union_pt(subject).Childs, &retval);
    return retval;
}

Polygons simplify_polygons(const Polygons &subject, bool preserve_collinear)
{
    Clipper3r::Paths output;
    if (preserve_collinear) {
        Clipper3r::Clipper c;
        c.PreserveCollinear(true);
        c.StrictlySimple(true);
        c.AddPaths(ClipperUtils::PolygonsProvider(subject), Clipper3r::ptSubject, true);
        c.Execute(Clipper3r::ctUnion, output, Clipper3r::pftNonZero, Clipper3r::pftNonZero);
    } else {
        output = Clipper3r::SimplifyPolygons(ClipperUtils::PolygonsProvider(subject), Clipper3r::pftNonZero);
    }
    
    // convert into Slic3r polygons
    return to_polygons(std::move(output));
}

ExPolygons simplify_polygons_ex(const Polygons &subject, bool preserve_collinear)
{
    if (! preserve_collinear)
        return union_ex(simplify_polygons(subject, false));

    Clipper3r::PolyTree polytree;    
    Clipper3r::Clipper c;
    c.PreserveCollinear(true);
    c.StrictlySimple(true);
    c.AddPaths(ClipperUtils::PolygonsProvider(subject), Clipper3r::ptSubject, true);
    c.Execute(Clipper3r::ctUnion, polytree, Clipper3r::pftNonZero, Clipper3r::pftNonZero);
    
    // convert into ExPolygons
    return PolyTreeToExPolygons(std::move(polytree));
}

Polygons top_level_islands(const Slic3r::Polygons &polygons)
{
    // init Clipper
    Clipper3r::Clipper clipper;
    clipper.Clear();
    // perform union
    clipper.AddPaths(ClipperUtils::PolygonsProvider(polygons), Clipper3r::ptSubject, true);
    Clipper3r::PolyTree polytree;
    clipper.Execute(Clipper3r::ctUnion, polytree, Clipper3r::pftEvenOdd, Clipper3r::pftEvenOdd); 
    // Convert only the top level islands to the output.
    Polygons out;
    out.reserve(polytree.ChildCount());
    for (int i = 0; i < polytree.ChildCount(); ++i)
        out.emplace_back(std::move(polytree.Childs[i]->Contour));
    return out;
}

// Outer offset shall not split the input contour into multiples. It is expected, that the solution will be non empty and it will contain just a single polygon.
Clipper3r::Paths fix_after_outer_offset(
	const Clipper3r::Path 		&input, 
													// combination of default prameters to correspond to void ClipperOffset::Execute(Paths& solution, double delta)
													// to produce a CCW output contour from CCW input contour for a positive offset.
	Clipper3r::PolyFillType 	 filltype, 			// = Clipper3r::pftPositive
	bool 						 reverse_result)	// = false
{
  	Clipper3r::Paths solution;
  	if (! input.empty()) {
		Clipper3r::Clipper clipper;
	  	clipper.AddPath(input, Clipper3r::ptSubject, true);
		clipper.ReverseSolution(reverse_result);
		clipper.Execute(Clipper3r::ctUnion, solution, filltype, filltype);
	}
    return solution;
}

// Inner offset may split the source contour into multiple contours, but one resulting contour shall not lie inside the other.
Clipper3r::Paths fix_after_inner_offset(
	const Clipper3r::Path 		&input, 
													// combination of default prameters to correspond to void ClipperOffset::Execute(Paths& solution, double delta)
													// to produce a CCW output contour from CCW input contour for a negative offset.
	Clipper3r::PolyFillType 	 filltype, 			// = Clipper3r::pftNegative
	bool 						 reverse_result) 	// = true
{
  	Clipper3r::Paths solution;
  	if (! input.empty()) {
		Clipper3r::Clipper clipper;
		clipper.AddPath(input, Clipper3r::ptSubject, true);
		Clipper3r::IntRect r = clipper.GetBounds();
		r.left -= 10; r.top -= 10; r.right += 10; r.bottom += 10;
		if (filltype == Clipper3r::pftPositive)
			clipper.AddPath({ Clipper3r::IntPoint(r.left, r.bottom), Clipper3r::IntPoint(r.left, r.top), Clipper3r::IntPoint(r.right, r.top), Clipper3r::IntPoint(r.right, r.bottom) }, Clipper3r::ptSubject, true);
		else
			clipper.AddPath({ Clipper3r::IntPoint(r.left, r.bottom), Clipper3r::IntPoint(r.right, r.bottom), Clipper3r::IntPoint(r.right, r.top), Clipper3r::IntPoint(r.left, r.top) }, Clipper3r::ptSubject, true);
		clipper.ReverseSolution(reverse_result);
		clipper.Execute(Clipper3r::ctUnion, solution, filltype, filltype);
		if (! solution.empty())
			solution.erase(solution.begin());
	}
	return solution;
}

Clipper3r::Path mittered_offset_path_scaled(const Points &contour, const std::vector<float> &deltas, double miter_limit)
{
	assert(contour.size() == deltas.size());

#ifndef NDEBUG
	// Verify that the deltas are either all positive, or all negative.
	bool positive = false;
	bool negative = false;
	for (float delta : deltas)
		if (delta < 0.f)
			negative = true;
		else if (delta > 0.f)
			positive = true;
	assert(! (negative && positive));
#endif /* NDEBUG */

	Clipper3r::Path out;

	if (deltas.size() > 2)
	{
		out.reserve(contour.size() * 2);

		// Clamp miter limit to 2.
		miter_limit = (miter_limit > 2.) ? 2. / (miter_limit * miter_limit) : 0.5;
		
		// perpenduclar vector
		auto   perp = [](const Vec2d &v) -> Vec2d { return Vec2d(v.y(), - v.x()); };

		// Add a new point to the output, scale by CLIPPER_OFFSET_SCALE and round to Clipper3r::cInt.
		auto   add_offset_point = [&out](Vec2d pt) {
            pt += Vec2d(0.5 - (pt.x() < 0), 0.5 - (pt.y() < 0));
			out.emplace_back(Clipper3r::cInt(pt.x()), Clipper3r::cInt(pt.y()));
		};

		// Minimum edge length, squared.
		double lmin  = *std::max_element(deltas.begin(), deltas.end()) * CLIPPER_OFFSET_SHORTEST_EDGE_FACTOR;
		double l2min = lmin * lmin;
		// Minimum angle to consider two edges to be parallel.
		// Vojtech's estimate.
//		const double sin_min_parallel = EPSILON + 1. / double(CLIPPER_OFFSET_SCALE);
		// Implementation equal to Clipper.
		const double sin_min_parallel = 1.;

		// Find the last point further from pt by l2min.
		Vec2d  pt     = contour.front().cast<double>();
		size_t iprev  = contour.size() - 1;
		Vec2d  ptprev;
		for (; iprev > 0; -- iprev) {
			ptprev = contour[iprev].cast<double>();
			if ((ptprev - pt).squaredNorm() > l2min)
				break;
		}

		if (iprev != 0) {
			size_t ilast = iprev;
			// Normal to the (pt - ptprev) segment.
			Vec2d nprev = perp(pt - ptprev).normalized();
			for (size_t i = 0; ; ) {
				// Find the next point further from pt by l2min.
				size_t j = i + 1;
				Vec2d ptnext;
				for (; j <= ilast; ++ j) {
					ptnext = contour[j].cast<double>();
					double l2 = (ptnext - pt).squaredNorm();
					if (l2 > l2min)
						break;
				}
				if (j > ilast) {
					assert(i <= ilast);
					// If the last edge is too short, merge it with the previous edge.
					i = ilast;
					ptnext = contour.front().cast<double>();
				}

				// Normal to the (ptnext - pt) segment.
				Vec2d nnext  = perp(ptnext - pt).normalized();

				double delta  = deltas[i];
				double sin_a  = std::clamp(cross2(nprev, nnext), -1., 1.);
				double convex = sin_a * delta;
				if (convex <= - sin_min_parallel) {
					// Concave corner.
					add_offset_point(pt + nprev * delta);
					add_offset_point(pt);
					add_offset_point(pt + nnext * delta);
				} else {
					double dot = nprev.dot(nnext);
					if (convex < sin_min_parallel && dot > 0.) {
						// Nearly parallel.
						add_offset_point((nprev.dot(nnext) > 0.) ? (pt + nprev * delta) : pt);
					} else {
						// Convex corner, possibly extremely sharp if convex < sin_min_parallel.
						double r = 1. + dot;
					  	if (r >= miter_limit)
							add_offset_point(pt + (nprev + nnext) * (delta / r));
					  	else {
							double dx = std::tan(std::atan2(sin_a, dot) / 4.);
							Vec2d  newpt1 = pt + (nprev - perp(nprev) * dx) * delta;
							Vec2d  newpt2 = pt + (nnext + perp(nnext) * dx) * delta;
#ifndef NDEBUG
							Vec2d vedge = 0.5 * (newpt1 + newpt2) - pt;
							double dist_norm = vedge.norm();
							assert(std::abs(dist_norm - std::abs(delta)) < SCALED_EPSILON);
#endif /* NDEBUG */
							add_offset_point(newpt1);
							add_offset_point(newpt2);
					  	}
					}
				}

				if (i == ilast)
					break;

				ptprev = pt;
				nprev  = nnext;
				pt     = ptnext;
				i = j;
			}
		}
	}

#if 0
	{
		Clipper3r::Path polytmp(out);
		unscaleClipperPolygon(polytmp);
		Slic3r::Polygon offsetted(std::move(polytmp));
		BoundingBox bbox = get_extents(contour);
		bbox.merge(get_extents(offsetted));
		static int iRun = 0;
		SVG svg(debug_out_path("mittered_offset_path_scaled-%d.svg", iRun ++).c_str(), bbox);
		svg.draw_outline(Polygon(contour), "blue", scale_(0.01));
		svg.draw_outline(offsetted, "red", scale_(0.01));
		svg.draw(contour, "blue", scale_(0.03));
		svg.draw((Points)offsetted, "blue", scale_(0.03));
	}
#endif

	return out;
}

Polygons variable_offset_inner(const ExPolygon &expoly, const std::vector<std::vector<float>> &deltas, double miter_limit)
{
#ifndef NDEBUG
	// Verify that the deltas are all non positive.
	for (const std::vector<float> &ds : deltas)
		for (float delta : ds)
			assert(delta <= 0.);
	assert(expoly.holes.size() + 1 == deltas.size());
#endif /* NDEBUG */

	// 1) Offset the outer contour.
	Clipper3r::Paths contours = fix_after_inner_offset(mittered_offset_path_scaled(expoly.contour.points, deltas.front(), miter_limit), Clipper3r::pftNegative, true);
#ifndef NDEBUG	
	for (auto &c : contours)
		assert(Clipper3r::Area(c) > 0.);
#endif /* NDEBUG */

	// 2) Offset the holes one by one, collect the results.
	Clipper3r::Paths holes;
	holes.reserve(expoly.holes.size());
	for (const Polygon& hole : expoly.holes)
		append(holes, fix_after_outer_offset(mittered_offset_path_scaled(hole.points, deltas[1 + &hole - expoly.holes.data()], miter_limit), Clipper3r::pftNegative, false));
#ifndef NDEBUG	
	for (auto &c : holes)
		assert(Clipper3r::Area(c) > 0.);
#endif /* NDEBUG */

	// 3) Subtract holes from the contours.
	Clipper3r::Paths output;
	if (holes.empty())
		output = std::move(contours);
	else {
		Clipper3r::Clipper clipper;
		clipper.Clear();
		clipper.AddPaths(contours, Clipper3r::ptSubject, true);
		clipper.AddPaths(holes, Clipper3r::ptClip, true);
		clipper.Execute(Clipper3r::ctDifference, output, Clipper3r::pftNonZero, Clipper3r::pftNonZero);
	}

	return to_polygons(std::move(output));
}

Polygons variable_offset_outer(const ExPolygon &expoly, const std::vector<std::vector<float>> &deltas, double miter_limit)
{
#ifndef NDEBUG
	// Verify that the deltas are all non positive.
for (const std::vector<float>& ds : deltas)
		for (float delta : ds)
			assert(delta >= 0.);
	assert(expoly.holes.size() + 1 == deltas.size());
#endif /* NDEBUG */

	// 1) Offset the outer contour.
	Clipper3r::Paths contours = fix_after_outer_offset(mittered_offset_path_scaled(expoly.contour.points, deltas.front(), miter_limit), Clipper3r::pftPositive, false);
#ifndef NDEBUG
	for (auto &c : contours)
		assert(Clipper3r::Area(c) > 0.);
#endif /* NDEBUG */

	// 2) Offset the holes one by one, collect the results.
	Clipper3r::Paths holes;
	holes.reserve(expoly.holes.size());
	for (const Polygon& hole : expoly.holes)
		append(holes, fix_after_inner_offset(mittered_offset_path_scaled(hole.points, deltas[1 + &hole - expoly.holes.data()], miter_limit), Clipper3r::pftPositive, true));
#ifndef NDEBUG
	for (auto &c : holes)
		assert(Clipper3r::Area(c) > 0.);
#endif /* NDEBUG */

	// 3) Subtract holes from the contours.
	Clipper3r::Paths output;
	if (holes.empty())
		output = std::move(contours);
	else {
		Clipper3r::Clipper clipper;
		clipper.Clear();
		clipper.AddPaths(contours, Clipper3r::ptSubject, true);
		clipper.AddPaths(holes, Clipper3r::ptClip, true);
		clipper.Execute(Clipper3r::ctDifference, output, Clipper3r::pftNonZero, Clipper3r::pftNonZero);
	}

	return to_polygons(std::move(output));
}

ExPolygons variable_offset_outer_ex(const ExPolygon &expoly, const std::vector<std::vector<float>> &deltas, double miter_limit)
{
#ifndef NDEBUG
	// Verify that the deltas are all non positive.
for (const std::vector<float>& ds : deltas)
		for (float delta : ds)
			assert(delta >= 0.);
	assert(expoly.holes.size() + 1 == deltas.size());
#endif /* NDEBUG */

	// 1) Offset the outer contour.
	Clipper3r::Paths contours = fix_after_outer_offset(mittered_offset_path_scaled(expoly.contour.points, deltas.front(), miter_limit), Clipper3r::pftPositive, false);
#ifndef NDEBUG
	for (auto &c : contours)
		assert(Clipper3r::Area(c) > 0.);
#endif /* NDEBUG */

	// 2) Offset the holes one by one, collect the results.
	Clipper3r::Paths holes;
	holes.reserve(expoly.holes.size());
	for (const Polygon& hole : expoly.holes)
		append(holes, fix_after_inner_offset(mittered_offset_path_scaled(hole.points, deltas[1 + &hole - expoly.holes.data()], miter_limit), Clipper3r::pftPositive, true));
#ifndef NDEBUG
	for (auto &c : holes)
		assert(Clipper3r::Area(c) > 0.);
#endif /* NDEBUG */

	// 3) Subtract holes from the contours.
	ExPolygons output;
	if (holes.empty()) {
		output.reserve(contours.size());
		for (Clipper3r::Path &path : contours) 
			output.emplace_back(std::move(path));
	} else {
		Clipper3r::Clipper clipper;
		clipper.AddPaths(contours, Clipper3r::ptSubject, true);
		clipper.AddPaths(holes, Clipper3r::ptClip, true);
	    Clipper3r::PolyTree polytree;
		clipper.Execute(Clipper3r::ctDifference, polytree, Clipper3r::pftNonZero, Clipper3r::pftNonZero);
	    output = PolyTreeToExPolygons(std::move(polytree));
	}

	return output;
}


ExPolygons variable_offset_inner_ex(const ExPolygon &expoly, const std::vector<std::vector<float>> &deltas, double miter_limit)
{
#ifndef NDEBUG
	// Verify that the deltas are all non positive.
	for (const std::vector<float>& ds : deltas)
		for (float delta : ds)
			assert(delta <= 0.);
	assert(expoly.holes.size() + 1 == deltas.size());
#endif /* NDEBUG */

	// 1) Offset the outer contour.
	Clipper3r::Paths contours = fix_after_inner_offset(mittered_offset_path_scaled(expoly.contour.points, deltas.front(), miter_limit), Clipper3r::pftNegative, true);
#ifndef NDEBUG
	for (auto &c : contours)
		assert(Clipper3r::Area(c) > 0.);
#endif /* NDEBUG */

	// 2) Offset the holes one by one, collect the results.
	Clipper3r::Paths holes;
	holes.reserve(expoly.holes.size());
	for (const Polygon& hole : expoly.holes)
		append(holes, fix_after_outer_offset(mittered_offset_path_scaled(hole.points, deltas[1 + &hole - expoly.holes.data()], miter_limit), Clipper3r::pftNegative, false));
#ifndef NDEBUG
	for (auto &c : holes)
		assert(Clipper3r::Area(c) > 0.);
#endif /* NDEBUG */

	// 3) Subtract holes from the contours.
	ExPolygons output;
	if (holes.empty()) {
		output.reserve(contours.size());
		for (Clipper3r::Path &path : contours) 
			output.emplace_back(std::move(path));
	} else {
		Clipper3r::Clipper clipper;
		clipper.AddPaths(contours, Clipper3r::ptSubject, true);
		clipper.AddPaths(holes, Clipper3r::ptClip, true);
	    Clipper3r::PolyTree polytree;
		clipper.Execute(Clipper3r::ctDifference, polytree, Clipper3r::pftNonZero, Clipper3r::pftNonZero);
	    output = PolyTreeToExPolygons(std::move(polytree));
	}

	return output;
}

}
