#include "ExtrusionEntity.hpp"
#include "ExtrusionEntityCollection.hpp"
#include "ExPolygon.hpp"
#include "ClipperUtils.hpp"
#include "Extruder.hpp"
#include "Flow.hpp"
#include "Exception.hpp"
#include <cmath>
#include <limits>
#include <sstream>
#include <utility>
#include "Utils.hpp"

#define L(s) (s)

namespace Slic3r {
    
static const double slope_inner_outer_wall_gap = 0.4;

namespace {

using PolylineFittingResult = decltype(std::declval<Polyline &>().fitting_result);

static_assert(noexcept(std::declval<Points &>().swap(std::declval<Points &>())),
              "ZAA transaction requires no-throw point storage exchange");
static_assert(noexcept(std::declval<PolylineFittingResult &>().swap(std::declval<PolylineFittingResult &>())),
              "ZAA transaction requires no-throw fitting-result exchange");
static_assert(noexcept(std::declval<std::unique_ptr<ExtrusionPath3> &>().swap(
                  std::declval<std::unique_ptr<ExtrusionPath3> &>())),
              "ZAA transaction requires no-throw 3D-path exchange");

void reject_unmodeled_path3_topology_change(const ExtrusionPath &path, const char *operation)
{
    if (path.has_path3())
        throw LogicError(operation);
}

} // namespace

ExtrusionPath3::ExtrusionPath3(Polyline3 polyline)
    : m_polyline(std::move(polyline))
{
}

std::unique_ptr<ExtrusionPath3> ExtrusionPath3::clone() const
{
    return std::make_unique<ExtrusionPath3>(*this);
}

void ExtrusionPath3::reverse()
{
    m_polyline.reverse();
}

void ExtrusionPath3::clip_end(double distance)
{
    m_polyline.clip_end(distance);
}

bool ExtrusionPath3::split_at_xy(const Point &query,
                                  Point &actual_xy,
                                  std::unique_ptr<ExtrusionPath3> &before,
                                  std::unique_ptr<ExtrusionPath3> &after) const
{
    if (&before == &after)
        return false;

    Polyline3 before_polyline;
    Polyline3 after_polyline;
    if (!m_polyline.split_at_xy(query, actual_xy, &before_polyline, &after_polyline))
        return false;

    before = std::make_unique<ExtrusionPath3>(std::move(before_polyline));
    after = std::make_unique<ExtrusionPath3>(std::move(after_polyline));
    return true;
}

bool ExtrusionPath3::append(const ExtrusionPath3 &suffix)
{
    if (suffix.m_polyline.points.empty())
        return true;
    if (m_polyline.points.empty()) {
        m_polyline = suffix.m_polyline;
        return true;
    }
    if (m_polyline.points.back() != suffix.m_polyline.points.front())
        return false;

    m_polyline.points.insert(m_polyline.points.end(), suffix.m_polyline.points.begin() + 1, suffix.m_polyline.points.end());
    return true;
}

const Polyline3 &ExtrusionPath3::polyline3() const
{
    return m_polyline;
}

double ExtrusionPath3::length_xy() const
{
    return m_polyline.length_xy();
}

double ExtrusionPath3::length_3d() const
{
    return m_polyline.length_3d();
}

void ExtrusionPath::set_path3(std::unique_ptr<ExtrusionPath3> path3)
{
    if (!path3)
        throw InvalidArgument("ExtrusionPath::set_path3 requires a non-null 3D path");

    polyline = path3->polyline3().to_polyline();
    m_path3 = std::move(path3);
}

bool ExtrusionPath::split_at_xy(const Point &query, Point &actual_xy, ExtrusionPath *before, ExtrusionPath *after) const
{
    if (before == nullptr || after == nullptr || before == after || polyline.points.size() < 2)
        return false;

    if (m_path3) {
        std::unique_ptr<ExtrusionPath3> before_path3;
        std::unique_ptr<ExtrusionPath3> after_path3;
        if (!m_path3->split_at_xy(query, actual_xy, before_path3, after_path3))
            return false;

        ExtrusionPath before_result(polyline, *this);
        ExtrusionPath after_result(polyline, *this);
        before_result.set_path3(std::move(before_path3));
        after_result.set_path3(std::move(after_path3));
        *before = std::move(before_result);
        *after = std::move(after_result);
        return true;
    }

    Point split_xy = query;
    Polyline before_polyline;
    Polyline after_polyline;
    polyline.split_at(split_xy, &before_polyline, &after_polyline);
    ExtrusionPath before_result(std::move(before_polyline), *this);
    ExtrusionPath after_result(std::move(after_polyline), *this);
    *before = std::move(before_result);
    *after = std::move(after_result);
    actual_xy = split_xy;
    return true;
}

void ExtrusionPath::append_path3(const ExtrusionPath &suffix)
{
    if (!m_path3 || !suffix.m_path3)
        throw LogicError("ExtrusionPath::append_path3 requires two 3D paths");
    if (!m_path3->append(*suffix.m_path3))
        throw LogicError("ExtrusionPath::append_path3 requires matching 3D endpoints");

    polyline = m_path3->polyline3().to_polyline();
}

void ExtrusionPath::reverse()
{
    if (m_path3) {
        m_path3->reverse();
        polyline = m_path3->polyline3().to_polyline();
    } else {
        polyline.reverse();
    }
}

void ExtrusionPath::intersect_expolygons(const ExPolygons &collection, ExtrusionEntityCollection* retval) const
{
    reject_unmodeled_path3_topology_change(*this, "ExtrusionPath::intersect_expolygons does not support a 3D path");
    this->_inflate_collection(intersection_pl(Polylines{ polyline }, collection), retval);
}

void ExtrusionPath::subtract_expolygons(const ExPolygons &collection, ExtrusionEntityCollection* retval) const
{
    reject_unmodeled_path3_topology_change(*this, "ExtrusionPath::subtract_expolygons does not support a 3D path");
    this->_inflate_collection(diff_pl(Polylines{ this->polyline }, collection), retval);
}

void ExtrusionPath::clip_end(double distance)
{
    if (m_path3) {
        m_path3->clip_end(distance);
        polyline = m_path3->polyline3().to_polyline();
    } else {
        polyline.clip_end(distance);
    }
}

void ExtrusionPath::simplify(double tolerance)
{
    reject_unmodeled_path3_topology_change(*this, "ExtrusionPath::simplify does not support a 3D path");
    this->polyline.simplify(tolerance);
}

void ExtrusionPath::simplify_by_fitting_arc(double tolerance)
{
    reject_unmodeled_path3_topology_change(*this, "ExtrusionPath::simplify_by_fitting_arc does not support a 3D path");
    this->polyline.simplify_by_fitting_arc(tolerance);
}

double ExtrusionPath::length() const
{
    return this->polyline.length();
}

void ExtrusionPath::_inflate_collection(const Polylines &polylines, ExtrusionEntityCollection* collection) const
{
    for (const Polyline &polyline : polylines)
        collection->entities.emplace_back(new ExtrusionPath(polyline, *this));
}

void ExtrusionPath::polygons_covered_by_width(Polygons &out, const float scaled_epsilon) const
{
    polygons_append(out, offset(this->polyline, float(scale_(this->width/2)) + scaled_epsilon));
}

void ExtrusionPath::polygons_covered_by_spacing(Polygons &out, const float scaled_epsilon) const
{
    // Instantiating the Flow class to get the line spacing.
    // Don't know the nozzle diameter, setting to zero. It shall not matter it shall be optimized out by the compiler.
    bool bridge = is_bridge(this->role());
    // SoftFever: TODO Mac trigger assersion errors
//    assert(! bridge || this->width == this->height);
    auto flow = bridge ? Flow::bridging_flow(this->width, 0.f) : Flow(this->width, this->height, 0.f);
    polygons_append(out, offset(this->polyline, 0.5f * float(flow.scaled_spacing()) + scaled_epsilon));
}

bool ExtrusionPath::can_merge(const ExtrusionPath& other)
{
    return !this->has_path3() && !other.has_path3() &&
           overhang_degree == other.overhang_degree && curve_degree == other.curve_degree && mm3_per_mm == other.mm3_per_mm &&
           width == other.width && height == other.height && m_can_reverse == other.m_can_reverse && m_role == other.m_role &&
           m_no_extrusion == other.m_no_extrusion && smooth_speed == other.smooth_speed &&
           m_zaa_path_policy == other.m_zaa_path_policy;
}

void ExtrusionMultiPath::reverse()
{
    for (ExtrusionPath &path : this->paths)
        path.reverse();
    std::reverse(this->paths.begin(), this->paths.end());
}

double ExtrusionMultiPath::length() const
{
    double len = 0;
    for (const ExtrusionPath &path : this->paths)
        len += path.polyline.length();
    return len;
}

void ExtrusionMultiPath::polygons_covered_by_width(Polygons &out, const float scaled_epsilon) const
{
    for (const ExtrusionPath &path : this->paths)
        path.polygons_covered_by_width(out, scaled_epsilon);
}

void ExtrusionMultiPath::polygons_covered_by_spacing(Polygons &out, const float scaled_epsilon) const
{
    for (const ExtrusionPath &path : this->paths)
        path.polygons_covered_by_spacing(out, scaled_epsilon);
}

double ExtrusionMultiPath::min_mm3_per_mm() const
{
    double min_mm3_per_mm = std::numeric_limits<double>::max();
    for (const ExtrusionPath &path : this->paths)
        min_mm3_per_mm = std::min(min_mm3_per_mm, path.mm3_per_mm);
    return min_mm3_per_mm;
}

Polyline ExtrusionMultiPath::as_polyline() const
{
    Polyline out;
    if (! paths.empty()) {
        size_t len = 0;
        for (size_t i_path = 0; i_path < paths.size(); ++ i_path) {
            assert(! paths[i_path].polyline.points.empty());
            assert(i_path == 0 || paths[i_path - 1].polyline.points.back() == paths[i_path].polyline.points.front());
            len += paths[i_path].polyline.points.size();
        }
        // The connecting points between the segments are equal.
        len -= paths.size() - 1;
        assert(len > 0);
        out.points.reserve(len);
        out.points.push_back(paths.front().polyline.points.front());
        for (size_t i_path = 0; i_path < paths.size(); ++ i_path)
            out.points.insert(out.points.end(), paths[i_path].polyline.points.begin() + 1, paths[i_path].polyline.points.end());
    }
    return out;
}

bool ExtrusionLoop::make_clockwise()
{
    bool was_ccw = this->polygon().is_counter_clockwise();
    if (was_ccw) this->reverse();
    return was_ccw;
}

bool ExtrusionLoop::make_counter_clockwise()
{
    bool was_cw = this->polygon().is_clockwise();
    if (was_cw) this->reverse();
    return was_cw;
}

void ExtrusionLoop::reverse()
{
    for (ExtrusionPath &path : this->paths)
        path.reverse();
    std::reverse(this->paths.begin(), this->paths.end());
}

Polygon ExtrusionLoop::polygon() const
{
    Polygon polygon;
    for (const ExtrusionPath &path : this->paths) {
        // for each polyline, append all points except the last one (because it coincides with the first one of the next polyline)
        polygon.points.insert(polygon.points.end(), path.polyline.points.begin(), path.polyline.points.end()-1);
    }
    return polygon;
}

double ExtrusionLoop::length() const
{
    double len = 0;
    for (const ExtrusionPath &path : this->paths)
        len += path.polyline.length();
    return len;
}

bool ExtrusionLoop::split_and_rotate_at_xy(size_t path_idx, const Point &seam_xy)
{
    if (path_idx >= paths.size())
        return false;

    const ExtrusionPath &source = paths[path_idx];
    ExtrusionPath before;
    ExtrusionPath after;
    Point actual_xy;
    if (!source.split_at_xy(seam_xy, actual_xy, &before, &after))
        return false;
    // A legacy 2D path may have an arc-fitting record. Splitting one of its
    // sampled vertices canonicalizes that endpoint onto the fitted arc, so its
    // emitted XY can intentionally differ from the requested sample. A 3D
    // sidecar has no arc representation and must retain its exact XY mirror.
    if (source.has_path3() && actual_xy != seam_xy) {
        throw LogicError("ExtrusionLoop::split_and_rotate_at_xy changed the requested seam point");
    }

    if (paths.size() == 1) {
        if (before.polyline.is_valid() && after.polyline.is_valid()) {
            if (after.has_path3()) {
                if (!before.has_path3())
                    throw LogicError("ExtrusionLoop::split_and_rotate_at_xy produced mixed path dimensions");
                after.append_path3(before);
            } else {
                if (before.has_path3())
                    throw LogicError("ExtrusionLoop::split_and_rotate_at_xy produced mixed path dimensions");
                after.polyline.append(std::move(before.polyline));
            }
            paths.front() = std::move(after);
        } else if (after.polyline.is_valid()) {
            paths.front() = std::move(after);
        } else if (before.polyline.is_valid()) {
            paths.front() = std::move(before);
        } else {
            return false;
        }
        return true;
    }

    ExtrusionPaths reordered;
    reordered.reserve(paths.size() + 1);
    if (after.polyline.is_valid())
        reordered.emplace_back(std::move(after));
    reordered.insert(reordered.end(), paths.begin() + path_idx + 1, paths.end());
    reordered.insert(reordered.end(), paths.begin(), paths.begin() + path_idx);
    if (before.polyline.is_valid())
        reordered.emplace_back(std::move(before));
    if (reordered.empty())
        return false;

    paths.swap(reordered);
    return true;
}

bool ExtrusionLoop::split_at_vertex(const Point &point, const double scaled_epsilon)
{
    for (size_t path_idx = 0; path_idx < paths.size(); ++path_idx) {
        const ExtrusionPath &path = paths[path_idx];
        const int vertex_idx = path.polyline.find_point(point, scaled_epsilon);
        if (vertex_idx != -1)
            return split_and_rotate_at_xy(path_idx, path.polyline.points[size_t(vertex_idx)]);
    }
    return false;
}

ExtrusionLoop::ClosestPathPoint ExtrusionLoop::get_closest_path_and_point(const Point &point, bool prefer_non_overhang) const
{
    // Find the closest path and closest point belonging to that path. Avoid overhangs, if asked for.
    ClosestPathPoint out{0, 0};
    double           min2 = std::numeric_limits<double>::max();
    ClosestPathPoint best_non_overhang{0, 0};
    double           min2_non_overhang = std::numeric_limits<double>::max();
    for (const ExtrusionPath &path : this->paths) {
        std::pair<int, Point> foot_pt_ = foot_pt(path.polyline.points, point);
        double                d2       = (foot_pt_.second - point).cast<double>().squaredNorm();
        if (d2 < min2) {
            out.foot_pt     = foot_pt_.second;
            out.path_idx    = &path - &this->paths.front();
            out.segment_idx = foot_pt_.first;
            min2            = d2;
        }
        if (prefer_non_overhang && !is_bridge(path.role()) && d2 < min2_non_overhang) {
            best_non_overhang.foot_pt     = foot_pt_.second;
            best_non_overhang.path_idx    = &path - &this->paths.front();
            best_non_overhang.segment_idx = foot_pt_.first;
            min2_non_overhang             = d2;
        }
    }
    if (prefer_non_overhang && min2_non_overhang != std::numeric_limits<double>::max())
        // Only apply the non-overhang point if there is one.
        out = best_non_overhang;
    return out;
}

// Splitting an extrusion loop, possibly made of multiple segments, some of the segments may be bridging.
void ExtrusionLoop::split_at(const Point &point, bool prefer_non_overhang, const double scaled_epsilon)
{
    if (paths.empty())
        return;

    auto [path_idx, segment_idx, seam_xy] = get_closest_path_and_point(point, prefer_non_overhang);

    // Snap seam_xy to the closest endpoint of the selected segment when requested.
    {
        const Point *segment_begin = paths[path_idx].polyline.points.data() + segment_idx;
        const Point *segment_end = segment_begin + 1;
        const double distance_to_begin = (point - *segment_begin).cast<double>().squaredNorm();
        const double distance_to_end = (point - *segment_end).cast<double>().squaredNorm();
        const double threshold_squared = scaled_epsilon * scaled_epsilon;
        if (distance_to_begin < distance_to_end) {
            if (distance_to_begin < threshold_squared)
                seam_xy = *segment_begin;
        } else if (distance_to_end < threshold_squared) {
            seam_xy = *segment_end;
        }
    }

    if (!split_and_rotate_at_xy(path_idx, seam_xy))
        throw LogicError("ExtrusionLoop::split_at could not split the selected path");
}

void ExtrusionLoop::clip_end(double distance, ExtrusionPaths* paths) const
{
    *paths = this->paths;
    
    while (distance > 0 && !paths->empty()) {
        ExtrusionPath &last = paths->back();
        double len = last.length();
        if (len <= distance) {
            paths->pop_back();
            distance -= len;
        } else {
            last.clip_end(distance);
            break;
        }
    }
}

bool ExtrusionLoop::has_overhang_point(const Point &point) const
{
    for (const ExtrusionPath &path : this->paths) {
        int pos = path.polyline.find_point(point);
        if (pos != -1) {
            // point belongs to this path
            // we consider it overhang only if it's not an endpoint
            return (is_bridge(path.role()) && pos > 0 && pos != (int)(path.polyline.points.size())-1);
        }
    }
    return false;
}

void ExtrusionLoop::polygons_covered_by_width(Polygons &out, const float scaled_epsilon) const
{
    for (const ExtrusionPath &path : this->paths)
        path.polygons_covered_by_width(out, scaled_epsilon);
}

void ExtrusionLoop::polygons_covered_by_spacing(Polygons &out, const float scaled_epsilon) const
{
    for (const ExtrusionPath &path : this->paths)
        path.polygons_covered_by_spacing(out, scaled_epsilon);
}

double ExtrusionLoop::min_mm3_per_mm() const
{
    double min_mm3_per_mm = std::numeric_limits<double>::max();
    for (const ExtrusionPath &path : this->paths)
        min_mm3_per_mm = std::min(min_mm3_per_mm, path.mm3_per_mm);
    return min_mm3_per_mm;
}

// Orca: This function is used to check if the loop is smooth(continuous) or not. 
// TODO: the main logic is largly copied from the calculate_polygon_angles_at_vertices function in SeamPlacer file. Need to refactor the code in the future.
bool ExtrusionLoop::is_smooth(double angle_threshold, double min_arm_length) const
{
    // go through all the points in the loop and check if the angle between two segments(AB and BC) is less than the threshold
    size_t idx_prev = 0;
    size_t idx_curr = 0;
    size_t idx_next = 0;

    float distance_to_prev = 0;
    float distance_to_next = 0;

    const auto _polygon = polygon();
    const Points& points = _polygon.points;

    std::vector<float> lengths{};
    for (size_t point_idx = 0; point_idx < points.size() - 1; ++point_idx) {
        lengths.push_back((unscale(points[point_idx]) - unscale(points[point_idx + 1])).norm());
    }
    lengths.push_back(std::max((unscale(points[0]) - unscale(points[points.size() - 1])).norm(), 0.1));

    // push idx_prev far enough back as initialization
    while (distance_to_prev < min_arm_length) {
        idx_prev = Slic3r::prev_idx_modulo(idx_prev, points.size());
        distance_to_prev += lengths[idx_prev];
    }

    for (size_t _i = 0; _i < points.size(); ++_i) {
        // pull idx_prev to current as much as possible, while respecting the min_arm_length
        while (distance_to_prev - lengths[idx_prev] > min_arm_length) {
            distance_to_prev -= lengths[idx_prev];
            idx_prev = Slic3r::next_idx_modulo(idx_prev, points.size());
        }

        // push idx_next forward as far as needed
        while (distance_to_next < min_arm_length) {
            distance_to_next += lengths[idx_next];
            idx_next = Slic3r::next_idx_modulo(idx_next, points.size());
        }

        // Calculate angle between idx_prev, idx_curr, idx_next.
        const Point& p0 = points[idx_prev];
        const Point& p1 = points[idx_curr];
        const Point& p2 = points[idx_next];
        const auto a = angle(p0 - p1, p2 - p1);
        if (a > 0 ? a < angle_threshold : a > -angle_threshold) {
            return false;
        }

        // increase idx_curr by one
        float curr_distance = lengths[idx_curr];
        idx_curr++;
        distance_to_prev += curr_distance;
        distance_to_next -= curr_distance;
    }

    return true;
}

ExtrusionLoopSloped::ExtrusionLoopSloped(ExtrusionPaths&   original_paths,
                                         double            seam_gap,
                                         double            slope_min_length,
                                         double            slope_max_segment_length,
                                         double            start_slope_ratio,
                                         ExtrusionLoopRole role)
    : ExtrusionLoop(role)
{
    // create slopes
    const auto add_slop = [this, slope_max_segment_length, seam_gap](const ExtrusionPath &path, const Polyline &poly, double ratio_begin, double ratio_end) {
        if (poly.empty()) { return; }

        // Ensure `slope_max_segment_length`
        Polyline detailed_poly;
        {
            detailed_poly.append(poly.first_point());

            // Recursively split the line into half until no longer than `slope_max_segment_length`
            const std::function<void(const Line &)> handle_line = [slope_max_segment_length, &detailed_poly, &handle_line](const Line &line) {
                if (line.length() <= slope_max_segment_length) {
                    detailed_poly.append(line.b);
                } else {
                    // Then process left half
                    handle_line({line.a, line.midpoint()});
                    // Then process right half
                    handle_line({line.midpoint(), line.b});
                }
            };

            for (const auto &l : poly.lines()) { handle_line(l); }
        }

        starts.emplace_back(detailed_poly, path, ExtrusionPathSloped::Slope{ratio_begin, ratio_begin}, ExtrusionPathSloped::Slope{ratio_end, ratio_end});

        if (is_approx(ratio_end, 1.) && seam_gap > 0) {
            // Remove the segments that has no extrusion
            const auto seg_length = detailed_poly.length();
            if (seg_length > seam_gap) {
                // Split the segment and remove the last `seam_gap` bit
                const Polyline orig = detailed_poly;
                Polyline       tmp;
                orig.split_at_length(seg_length - seam_gap, &detailed_poly, &tmp);

                ratio_end = lerp(ratio_begin, ratio_end, (seg_length - seam_gap) / seg_length);
                assert(1. - ratio_end > EPSILON);
            } else {
                // Remove the entire segment
                detailed_poly.clear();
            }
        }
        if (!detailed_poly.empty()) { ends.emplace_back(detailed_poly, path, ExtrusionPathSloped::Slope{1., 1. - ratio_begin}, ExtrusionPathSloped::Slope{1., 1. - ratio_end}); }

    };

    double remaining_length = slope_min_length;

    ExtrusionPaths::iterator path        = original_paths.begin();
    double                   start_ratio = start_slope_ratio;
    for (; path != original_paths.end() && remaining_length > 0; ++path) {
        const double path_len = unscale_(path->length());
        if (path_len > remaining_length) {
            // Split current path into slope and non-slope part
            Polyline slope_path;
            Polyline flat_path;
            path->polyline.split_at_length(scale_(remaining_length), &slope_path, &flat_path);

            add_slop(*path, slope_path, start_ratio, 1);
            start_ratio = 1;

            paths.emplace_back(std::move(flat_path), *path);
            remaining_length = 0;
        } else {
            remaining_length -= path_len;
            const double end_ratio = lerp(1.0, start_slope_ratio, remaining_length / slope_min_length);
            add_slop(*path, path->polyline, start_ratio, end_ratio);
            start_ratio = end_ratio;
        }
    }
    assert(remaining_length <= 0);
    assert(start_ratio == 1.);

    // Put remaining flat paths
    paths.insert(paths.end(), path, original_paths.end());
}

std::vector<const ExtrusionPath*> ExtrusionLoopSloped::get_all_paths() const {
    std::vector<const ExtrusionPath*> r;
    r.reserve(starts.size() + paths.size() + ends.size());
    for (const auto& p : starts) {
        r.push_back(&p);
    }
    for (const auto& p : paths) {
        r.push_back(&p);
    }
    for (const auto& p : ends) {
        r.push_back(&p);
    }

    return r;
}

void ExtrusionLoopSloped::clip_slope(double distance, bool inter_perimeter)
{

    this->clip_end(distance);
    this->clip_front(distance*2);
}

void ExtrusionLoopSloped::clip_end(const double distance)
{
    double clip_dist = distance;
    std::vector<ExtrusionPathSloped> &ends_slope = this->ends;
    while (clip_dist > 0 && !ends_slope.empty()) {
        ExtrusionPathSloped &last_path = ends_slope.back();
        double len = last_path.length();
        if (len <= clip_dist) {
            ends_slope.pop_back();
            clip_dist -= len;
        } else {
            last_path.polyline.clip_end(clip_dist);
            break;
        }
    }
}

void ExtrusionLoopSloped::clip_front(const double distance)
{
    double clip_dist = distance;
    if (this->role() == erPerimeter)
        clip_dist = scale_(this->slope_path_length()) * slope_inner_outer_wall_gap;

    std::vector<ExtrusionPathSloped> &start_slope = this->starts;

    Polyline front_inward;
    while (distance > 0 && !start_slope.empty()) {
        ExtrusionPathSloped &first_path = start_slope.front();
        double len = first_path.length();
        if (len <= clip_dist) {
            start_slope.erase(start_slope.begin());
            clip_dist -= len;
        } else {
            first_path.polyline.reverse();
            first_path.polyline.clip_end(clip_dist);
            first_path.polyline.reverse();
            break;
        }
    }
}

double ExtrusionLoopSloped::slope_path_length() {
    double total_length = 0.0;
    for (ExtrusionPathSloped start_ep : this->starts) {
        total_length += unscale_(start_ep.length());
    }
    return total_length;
}

std::string ExtrusionEntity::role_to_string(ExtrusionRole role)
{
    switch (role) {
        case erNone                         : return L("Undefined");
        case erPerimeter                    : return L("Inner wall");
        case erExternalPerimeter            : return L("Outer wall");
        case erOverhangPerimeter            : return L("Overhang wall");
        case erInternalInfill               : return L("Sparse infill");
        case erSolidInfill                  : return L("Internal solid infill");
        case erTopSolidInfill               : return L("Top surface");
        case erBottomSurface                : return L("Bottom surface");
        case erIroning                      : return L("Ironing");
        case erBridgeInfill                 : return L("Bridge");
        case erInternalBridgeInfill         : return L("Internal Bridge");
        case erGapFill                      : return L("Gap infill");
        case erSkirt                        : return L("Skirt");
        case erBrim                         : return L("Brim");
        case erSupportMaterial              : return L("Support");
        case erSupportMaterialInterface     : return L("Support interface");
        case erSupportTransition            : return L("Support transition");
        case erWipeTower                    : return L("Prime tower");
        case erSkinInfill                   : return L("Skin infill");
        case erCustom                       : return L("Custom");
        case erMixed                        : return L("Multiple");
        default                             : assert(false);
    }
    return "";
}

ExtrusionRole ExtrusionEntity::string_to_role(const std::string_view role)
{
    if (role == L("Inner wall"))
        return erPerimeter;
    else if (role == L("Outer wall"))
        return erExternalPerimeter;
    else if (role == L("Overhang wall"))
        return erOverhangPerimeter;
    else if (role == L("Sparse infill"))
        return erInternalInfill;
    else if (role == L("Internal solid infill"))
        return erSolidInfill;
    else if (role == L("Top surface"))
        return erTopSolidInfill;
    else if (role == L("Bottom surface"))
        return erBottomSurface;
    else if (role == L("Ironing"))
        return erIroning;
    else if (role == L("Bridge"))
        return erBridgeInfill;
    else if (role == L("Internal Bridge"))
        return erInternalBridgeInfill;
    else if (role == L("Gap infill"))
        return erGapFill;
    else if (role == ("Skirt"))
        return erSkirt;
    else if (role == ("Brim"))
        return erBrim;
    else if (role == L("Support"))
        return erSupportMaterial;
    else if (role == L("Support interface"))
        return erSupportMaterialInterface;
    else if (role == L("Support transition"))
        return erSupportTransition;
    else if (role == L("Prime tower"))
        return erWipeTower;
    else if (role == L("Skin infill"))
        return erSkinInfill;
    else if (role == L("Custom"))
        return erCustom;
    else if (role == L("Multiple"))
        return erMixed;
    else
        return erNone;
}

}
