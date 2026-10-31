#include "FillPlanePath.hpp"

#include "../ClipperUtils.hpp"
#include "../AABBTreeLines.hpp"
#include "../ShortestPath.hpp"
#include "../Surface.hpp"

#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/parallel_for.h>

#include <boost/log/trivial.hpp>
#include <numeric>
#include <unordered_map>

namespace Slic3r {

void Hilbert::filter_corner_gaps(ExPolygons &areas, const ExPolygons &boundary, double bead_width,
                                  const std::function<void()> &throw_if_canceled)
{
    const auto check_cancel = [&] { if (throw_if_canceled) throw_if_canceled(); };
    check_cancel();
    if (areas.empty() || boundary.empty() || bead_width <= 0.)
        return;
    // Only discard small, compact, isolated interior triangles from
    // Hilbert turns. These are conservative size heuristics, not a
    // minimum printable width: all retained gaps use the original
    // opening and medial-axis widths, including long thin gaps.
    const double extent_limit = scaled<double>(0.5 * bead_width);
    const double area_limit = 0.5 * extent_limit * extent_limit;
    // Squared distances in scaled coordinates exceed the integer range.
    const Linesf boundary_lines = to_linesf(boundary);
    const auto boundary_tree = AABBTreeLines::build_aabb_tree_over_indexed_lines(boundary_lines);
    if (boundary_tree.empty())
        return;
    const size_t original_count = areas.size();
    size_t triangles = 0, compact = 0, protected_count = 0;
    areas.erase(std::remove_if(areas.begin(), areas.end(),
        [&](const ExPolygon &area) {
            check_cancel();
            if (!area.holes.empty() || area.contour.size() != 3)
                return false;
            ++triangles;
            const BoundingBox box = get_extents(area);
            const Vec2d size = box.size().cast<double>();
            // Use the triangle diameter, not axis-aligned side lengths:
            // the same turn residual must qualify at 0, 45 and 90 degrees.
            const Points &points = area.contour.points;
            for (size_t i = 0; i < 3; ++i)
                if ((points[i].cast<double>() - points[(i + 1) % 3].cast<double>()).squaredNorm() >=
                    2. * extent_limit * extent_limit)
                    return false;
            const double area_size = area.area();
            if (area_size <= 0. || area_size >= area_limit)
                return false;
            // Reject slivers even when rotated into a nearly square
            // bounding box. A square has perimeter^2 / area = 16.
            const double perimeter = area.contour.length();
            if (perimeter * perimeter > 24. * area_size)
                return false;
            ++compact;
            // Keep the entire residual at least half a bead away from
            // every infill boundary, including hole edges. The box's
            // circumscribed circle bounds every point of the residual.
            const double protected_radius = 0.5 * size.norm() + extent_limit + SCALED_EPSILON;
            size_t hit_index = size_t(-1);
            Vec2d hit_point = Vec2d::Zero();
            const Vec2d center = box.center().cast<double>();
            AABBTreeLines::squared_distance_to_indexed_lines(
                boundary_lines, boundary_tree, center, hit_index, hit_point,
                protected_radius * protected_radius);
            protected_count += hit_index != size_t(-1);
            return hit_index == size_t(-1);
        }), areas.end());
    BOOST_LOG_TRIVIAL(debug) << "Hilbert corner gap filtering: before=" << original_count
        << " triangles=" << triangles << " compact=" << compact
        << " protected=" << protected_count << " after=" << areas.size()
        << " bead_width=" << bead_width;
}

Polylines Hilbert::clip(const Polyline &polyline, const ExPolygon &boundary, size_t block_size,
                       const std::function<void()> &throw_if_canceled)
{
    const auto check_cancel = [&] { if (throw_if_canceled) throw_if_canceled(); };
    const auto serial_clip = [&] {
        check_cancel();
        Polylines result = intersection_pl(polyline, boundary);
        check_cancel();
        return result;
    };
    check_cancel();
    const size_t segment_count = polyline.size() > 1 ? polyline.size() - 1 : 0;
    // Bound duplicated boundary storage even when many layers run concurrently.
    // Use the same partition with one worker and many workers.
    const size_t block_count = std::min(size_t(16), segment_count / std::max(size_t(1), block_size));
    if (block_count < 2)
        return serial_clip();

    std::vector<size_t> cuts{0};
    std::vector<bool> inside;
    for (size_t block = 1; block < block_count; ++block) {
        const size_t target = segment_count * block / block_count;
        // Do not split a collinear run (Clipper may remove that vertex), or a
        // boundary contact whose inclusion depends on the adjacent segments.
        size_t cut = target;
        for (; cut < std::min(target + 64, segment_count); ++cut) {
            const Point &a = polyline.points[cut - 1];
            const Point &p = polyline.points[cut];
            const Point &b = polyline.points[cut + 1];
            if ((a.x() == p.x() && p.x() == b.x()) || (a.y() == p.y() && p.y() == b.y()))
                continue;
            if (!boundary.on_boundary(p, SCALED_EPSILON))
                break;
        }
        if (cut >= std::min(target + 64, segment_count) || cut <= cuts.back())
            return serial_clip();
        cuts.push_back(cut);
        inside.push_back(boundary.contains(polyline.points[cut], false));
    }
    cuts.push_back(segment_count);

    std::vector<Polylines> blocks(block_count);
    tbb::parallel_for(tbb::blocked_range<size_t>(0, block_count, 1),
        [&](const tbb::blocked_range<size_t> &range) {
            for (size_t block = range.begin(); block < range.end(); ++block) {
                check_cancel();
                Polyline part;
                // Share the endpoint only; every input edge belongs to one block.
                part.points.assign(polyline.points.begin() + cuts[block],
                                   polyline.points.begin() + cuts[block + 1] + 1);
                blocks[block] = intersection_pl(part, boundary);
                check_cancel();
            }
        });

    // Record only artificial cuts. Natural boundary contacts must never be
    // joined merely because their rounded coordinates coincide.
    std::unordered_map<Point, size_t, PointHash> cut_indices;
    for (size_t i = 0; i < inside.size(); ++i)
        if (inside[i])
            cut_indices.emplace(polyline.points[cuts[i + 1]], i);
    const size_t invalid = size_t(-1);
    std::vector<std::pair<size_t, size_t>> joins(inside.size(), {invalid, invalid});
    size_t path_count = 0;
    for (const Polylines &block : blocks)
        path_count += block.size();
    Polylines paths;
    paths.reserve(path_count);
    for (size_t block = 0; block < block_count; ++block) {
        check_cancel();
        for (Polyline &path : blocks[block]) {
            if (path.size() < 2)
                continue;
            for (const Point *end : {&path.first_point(), &path.last_point()}) {
                const auto it = cut_indices.find(*end);
                if (it != cut_indices.end()) {
                    const size_t cut = it->second;
                    if (block != cut && block != cut + 1)
                        return serial_clip();
                    size_t &index = block == cut ? joins[cut].first : joins[cut].second;
                    if (index != invalid)
                        return serial_clip();
                    index = paths.size();
                }
            }
            paths.emplace_back();
            paths.back().points = std::move(path.points);
        }
    }

    std::vector<size_t> parent(paths.size());
    std::iota(parent.begin(), parent.end(), size_t(0));
    auto root = [&parent](size_t index) {
        while (parent[index] != index)
            index = parent[index];
        return index;
    };
    for (size_t cut = 0; cut < inside.size(); ++cut) {
        if (!inside[cut])
            continue;
        if (joins[cut].first == invalid || joins[cut].second == invalid)
            return serial_clip();
        const size_t left = root(joins[cut].first);
        const size_t right = root(joins[cut].second);
        if (left == right)
            return serial_clip();
        Points &a = paths[left].points;
        Points &b = paths[right].points;
        const Point &p = polyline.points[cuts[cut + 1]];
        if (a.front() == p)
            std::reverse(a.begin(), a.end());
        if (b.back() == p)
            std::reverse(b.begin(), b.end());
        if (a.back() != p || b.front() != p)
            return serial_clip();
        a.insert(a.end(), b.begin() + 1, b.end());
        b.clear();
        parent[right] = left;
    }

    // Clipper does not preserve path orientation or input order. Canonicalize
    // before greedy chaining so task completion order cannot affect toolpaths.
    auto point_less = [](const Point &a, const Point &b) {
        return a.x() < b.x() || (a.x() == b.x() && a.y() < b.y());
    };
    paths.erase(std::remove_if(paths.begin(), paths.end(),
        [](const Polyline &path) { return path.empty(); }), paths.end());
    for (Polyline &path : paths)
        if (point_less(path.last_point(), path.first_point()))
            std::reverse(path.points.begin(), path.points.end());
    std::sort(paths.begin(), paths.end(), [&](const Polyline &a, const Polyline &b) {
        return std::lexicographical_compare(a.points.begin(), a.points.end(),
                                           b.points.begin(), b.points.end(), point_less);
    });
    check_cancel();
    return paths;
}

class InfillPolylineClipper : public InfillPolylineOutput {
public:
    InfillPolylineClipper(const BoundingBox bbox, const double scale_out) : InfillPolylineOutput(scale_out), m_bbox(bbox) {}

    void            add_point(const Vec2d &pt);
    Points&&        result() { return std::move(m_out); }
    bool            clips() const override { return true; }

private:
    enum class Side {
        Left   = 1,
        Right  = 2,
        Top    = 4,
        Bottom = 8
    };

    int sides(const Point &p) const {
        return int(p.x() < m_bbox.min.x()) * int(Side::Left) +
               int(p.x() > m_bbox.max.x()) * int(Side::Right) +
               int(p.y() < m_bbox.min.y()) * int(Side::Bottom) +
               int(p.y() > m_bbox.max.y()) * int(Side::Top);
    };

    // Bounding box to clip the polyline with.
    BoundingBox m_bbox;

    // Classification of the two last points processed.
    int         m_sides_prev;
    int         m_sides_this;
};

void InfillPolylineClipper::add_point(const Vec2d &fpt)
{
    const Point pt{ this->scaled(fpt) };

    if (m_out.size() < 2) {
        // Collect the two first points and their status.
        (m_out.empty() ? m_sides_prev : m_sides_this) = sides(pt);
        m_out.emplace_back(pt);
    } else {
        // Classify the last inserted point, possibly remove it.
        int sides_next = sides(pt);
        if (// This point is inside. Take it.
            m_sides_this == 0 ||
            // Either this point is outside and previous or next is inside, or
            // the edge possibly cuts corner of the bounding box.
            (m_sides_prev & m_sides_this & sides_next) == 0) {
            // Keep the last point.
            m_sides_prev = m_sides_this;
        } else {
            // All the three points (this, prev, next) are outside at the same side.
            // Ignore the last point.
            m_out.pop_back();
        }
        // And save the current point.
        m_out.emplace_back(pt);
        m_sides_this = sides_next;
    }
}

void FillPlanePath::_fill_surface_single(
    const FillParams                &params, 
    unsigned int                     thickness_layers,
    const std::pair<float, Point>   &direction, 
    ExPolygon                        expolygon,
    Polylines                       &polylines_out)
{
    expolygon.rotate(-direction.first);

    //FIXME Vojtech: We are not sure whether the user expects the fill patterns on visible surfaces to be aligned across all the islands of a single layer.
    // One may align for this->centered() to align the patterns for Archimedean Chords and Octagram Spiral patterns.
    const bool align = params.density < 0.995;

    BoundingBox snug_bounding_box = get_extents(expolygon).inflated(SCALED_EPSILON);

    // Rotated bounding box of the area to fill in with the pattern.
    BoundingBox bounding_box = align ?
        // Sparse infill needs to be aligned across layers. Align infill across layers using the object's bounding box.
        this->bounding_box.rotated(-direction.first) :
        // Solid infill does not need to be aligned across layers, generate the infill pattern
        // around the clipping expolygon only.
        snug_bounding_box;

    Point shift = this->centered() ? 
        bounding_box.center() :
        bounding_box.min;
    expolygon.translate(-shift.x(), -shift.y());
    bounding_box.translate(-shift.x(), -shift.y());

    Polyline polyline;
    {
        auto distance_between_lines = scaled<double>(this->spacing) / params.density;
        auto min_x = coord_t(ceil(coordf_t(bounding_box.min.x()) / distance_between_lines));
        auto min_y = coord_t(ceil(coordf_t(bounding_box.min.y()) / distance_between_lines));
        auto max_x = coord_t(ceil(coordf_t(bounding_box.max.x()) / distance_between_lines));
        auto max_y = coord_t(ceil(coordf_t(bounding_box.max.y()) / distance_between_lines));
        auto resolution = scaled<double>(params.resolution) / distance_between_lines;
        if (align) {
            // Filling in a bounding box over the whole object, clip generated polyline against the snug bounding box.
            snug_bounding_box.translate(-shift.x(), -shift.y());
            InfillPolylineClipper output(snug_bounding_box, distance_between_lines);
            this->generate(min_x, min_y, max_x, max_y, resolution, output);
            polyline.points = std::move(output.result());
        } else {
            // Filling in a snug bounding box, no need to clip.
            InfillPolylineOutput output(distance_between_lines);
            this->generate(min_x, min_y, max_x, max_y, resolution, output);
            polyline.points = std::move(output.result());
        }
    }

    if (polyline.size() >= 2) {
        const auto *hilbert = dynamic_cast<const FillHilbertCurve *>(this);
        // Sparse patterns use the stateful bounding-box preclipper. Limit block
        // clipping to dense Hilbert paths, whose grid vertices remain unique.
        Polylines polylines = hilbert && !align ?
            Hilbert::clip(polyline, expolygon, 65536, hilbert->cancel_callback()) : intersection_pl(polyline, expolygon);
        if (hilbert)
            hilbert->throw_if_canceled();
        Polylines chained;
        if (params.dont_connect() || params.density > 0.5 || polylines.size() <= 1) {
            // Holes can split Hilbert infill into thousands of pieces. Keep the
            // greedy order without the quadratic two-exchange refinement.
            // The legacy refinement builds a local result without writing it
            // back. Keep only the effective greedy ordering.
            chained = hilbert ?
                chain_polylines_greedy(std::move(polylines)) :
                chain_polylines(std::move(polylines));
        } else
            connect_infill(std::move(polylines), expolygon, chained, this->spacing, params);
        if (hilbert)
            hilbert->throw_if_canceled();
        // paths must be repositioned and rotated back
        for (Polyline &pl : chained) {
            pl.translate(shift.x(), shift.y());
            pl.rotate(direction.first);
        }
        append(polylines_out, std::move(chained));
    }
}

// Follow an Archimedean spiral, in polar coordinates: r=a+b\theta
template<typename Output>
static void generate_archimedean_chords(coord_t min_x, coord_t min_y, coord_t max_x, coord_t max_y, const double resolution, Output &output)
{
    // Radius to achieve.
    coordf_t rmax = std::sqrt(coordf_t(max_x)*coordf_t(max_x)+coordf_t(max_y)*coordf_t(max_y)) * std::sqrt(2.) + 1.5;
    // Now unwind the spiral.
    coordf_t a = 1.;
    coordf_t b = 1./(2.*M_PI);
    coordf_t theta = 0.;
    coordf_t r = 1;
    Pointfs out;
    //FIXME Vojtech: If used as a solid infill, there is a gap left at the center.
    output.add_point({ 0, 0 });
    output.add_point({ 1, 0 });
    while (r < rmax) {
        // Discretization angle to achieve a discretization error lower than resolution.
        theta += 2. * acos(1. - resolution / r);
        r = a + b * theta;
        output.add_point({ r * cos(theta), r * sin(theta) });
    }
}

void FillArchimedeanChords::generate(coord_t min_x, coord_t min_y, coord_t max_x, coord_t max_y, const double resolution, InfillPolylineOutput &output)
{
    if (output.clips())
        generate_archimedean_chords(min_x, min_y, max_x, max_y, resolution, static_cast<InfillPolylineClipper&>(output));
    else
        generate_archimedean_chords(min_x, min_y, max_x, max_y, resolution, output);
}

// Adapted from 
// http://cpansearch.perl.org/src/KRYDE/Math-PlanePath-122/lib/Math/PlanePath/HilbertCurve.pm
//
// state=0    3--2   plain
//               |
//            0--1
//
// state=4    1--2  transpose
//            |  |
//            0  3
//
// state=8
//
// state=12   3  0  rot180 + transpose
//            |  |
//            2--1
//
static inline Point hilbert_n_to_xy(const size_t n)
{
    static constexpr const int next_state[16] { 4,0,0,12, 0,4,4,8, 12,8,8,4, 8,12,12,0 };
    static constexpr const int digit_to_x[16] { 0,1,1,0, 0,0,1,1, 1,0,0,1, 1,1,0,0 };
    static constexpr const int digit_to_y[16] { 0,0,1,1, 0,1,1,0, 1,1,0,0, 1,0,0,1 };

    // Number of 2 bit digits.
    size_t ndigits = 0;
    {
        size_t nc = n;
        while(nc > 0) {
            nc >>= 2;
            ++ ndigits;
        }
    }
    int state = (ndigits & 1) ? 4 : 0;
    coord_t x = 0;
    coord_t y = 0;
    for (int i = (int)ndigits - 1; i >= 0; -- i) {
        int digit = (n >> (i * 2)) & 3;
        state += digit;
        x |= digit_to_x[state] << i;
        y |= digit_to_y[state] << i;
        state = next_state[state];
    }
    return Point(x, y);
}

template<typename Output>
static void generate_hilbert_curve(coord_t min_x, coord_t min_y, coord_t max_x, coord_t max_y, Output &output,
                                   const std::function<void()> &throw_if_canceled)
{
    // Minimum power of two square to fit the domain.
    size_t sz = 2;
    size_t pw = 1;
    {
        size_t sz0 = std::max(max_x + 1 - min_x, max_y + 1 - min_y);
        while (sz < sz0) {
            sz = sz << 1;
            ++ pw;
        }
    }

    size_t sz2 = sz * sz;
    output.reserve(sz2);
    for (size_t i = 0; i < sz2; ++ i) {
        if ((i & 4095) == 0 && throw_if_canceled)
            throw_if_canceled();
        Point p = hilbert_n_to_xy(i);
        output.add_point({ p.x() + min_x, p.y() + min_y });
    }
}

void FillHilbertCurve::generate(coord_t min_x, coord_t min_y, coord_t max_x, coord_t max_y, const double /* resolution */, InfillPolylineOutput &output)
{
    if (output.clips())
        generate_hilbert_curve(min_x, min_y, max_x, max_y, static_cast<InfillPolylineClipper&>(output), m_cancel_callback);
    else
        generate_hilbert_curve(min_x, min_y, max_x, max_y, output, m_cancel_callback);
}

template<typename Output>
static void generate_octagram_spiral(coord_t min_x, coord_t min_y, coord_t max_x, coord_t max_y, Output &output)
{
    // Radius to achieve.
    coordf_t rmax = std::sqrt(coordf_t(max_x)*coordf_t(max_x)+coordf_t(max_y)*coordf_t(max_y)) * std::sqrt(2.) + 1.5;
    // Now unwind the spiral.
    coordf_t r = 0;
    coordf_t r_inc = sqrt(2.);
    output.add_point({ 0., 0. });
    while (r < rmax) {
        r += r_inc;
        coordf_t rx = r / sqrt(2.);
        coordf_t r2 = r + rx;
        output.add_point({ r,   0. });
        output.add_point({ r2,  rx });
        output.add_point({ rx,  rx });
        output.add_point({ rx,  r2 });
        output.add_point({ 0.,  r  });
        output.add_point({-rx,  r2 });
        output.add_point({-rx,  rx });
        output.add_point({-r2,  rx });
        output.add_point({- r,  0. });
        output.add_point({-r2, -rx });
        output.add_point({-rx, -rx });
        output.add_point({-rx, -r2 });
        output.add_point({ 0., -r  });
        output.add_point({ rx, -r2 });
        output.add_point({ rx, -rx });
        output.add_point({ r2+r_inc, -rx });
    }
}

void FillOctagramSpiral::generate(coord_t min_x, coord_t min_y, coord_t max_x, coord_t max_y, const double /* resolution */, InfillPolylineOutput &output)
{
    if (output.clips())
        generate_octagram_spiral(min_x, min_y, max_x, max_y, static_cast<InfillPolylineClipper&>(output));
    else
        generate_octagram_spiral(min_x, min_y, max_x, max_y, output);
}

} // namespace Slic3r
