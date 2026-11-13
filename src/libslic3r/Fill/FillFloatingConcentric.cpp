#include "../BoundingBox.hpp"
#include "../ClipperUtils.hpp"
#include <boost/log/trivial.hpp>
#include "../ClipperZUtils.hpp"
#include "../ExPolygon.hpp"
#include "../Surface.hpp"
#include "../VariableWidth.hpp"
#include "../format.hpp"
#include "../EdgeGrid.hpp"
#include "FillFloatingConcentric.hpp"
#include <libslic3r/ShortestPath.hpp>
#include <numeric>
#include <unordered_map>
#include <unordered_set>

namespace Slic3r {

using ZPath  = ClipperLib_Z::Path;
using ZPaths = ClipperLib_Z::Paths;

// Keep these geometry adapters local: CP's ThickPolyline stores two widths per
// edge and does not expose Bambu's vertex-width or rebasing helpers.
static coordf_t get_width_at(const ThickPolyline& line, size_t point_idx)
{
    assert(line.points.size() >= 2 && line.width.size() == 2 * (line.points.size() - 1));
    assert(point_idx < line.points.size());
    return line.width[point_idx < 2 ? point_idx : 2 * point_idx - 1];
}

FloatingThickPolyline FloatingThickPolyline::rebase_at(size_t idx) const
{
    if (points.size() < 2 || !is_closed())
        return {};
    assert(width.size() == 2 * (points.size() - 1) && is_floating.size() == points.size());

    FloatingThickPolyline ret = *this;
    const size_t edge_count = points.size() - 1;
    idx %= edge_count;
    for (size_t j = 0; j < edge_count; ++j) {
        const size_t source = (idx + j) % edge_count;
        ret.points[j] = points[source];
        ret.is_floating[j] = is_floating[source];
        ret.width[2 * j] = width[2 * source];
        ret.width[2 * j + 1] = width[2 * source + 1];
    }
    ret.points.back() = ret.points.front();
    ret.is_floating.back() = ret.is_floating.front();
    return ret;
}

void FloatingThickPolyline::clip_end(double distance)
{
    if (points.empty())
        return;
    Polyline::clip_end(distance);
    // Preserve Bambu's surviving edge widths while keeping the parallel arrays
    // in sync when the loop gap removes one or more vertices.
    width.resize(points.empty() ? 0 : 2 * (points.size() - 1));
    is_floating.resize(points.size());
}

FloatingThicklines FloatingThickPolyline::floating_thicklines() const
{
    FloatingThicklines lines;
    assert(is_floating.size() == points.size());
    assert(width.size() == (points.empty() ? 0 : 2 * (points.size() - 1)));
    if (this->points.size() >= 2) {
        lines.reserve(this->points.size() - 1);
        for (size_t i = 0; i + 1 < this->points.size(); ++i)
            lines.emplace_back(this->points[i], this->points[i + 1], this->width[2 * i], this->width[2 * i + 1], this->is_floating[i], this->is_floating[i + 1]);
    }
    return lines;
}


//BBS: new function to filter width to avoid too fragmented segments
static ExtrusionPaths floating_thick_polyline_to_extrusion_paths(const FloatingThickPolyline& floating_polyline, ExtrusionRole role, const Flow& flow, const float tolerance)
{
    ExtrusionPaths paths;
    ExtrusionPath path(role);
    FloatingThicklines lines = floating_polyline.floating_thicklines();

    size_t start_index = 0;
    double max_width = 0.0, min_width = 0.0;

    auto set_flow_for_path = [&flow](ExtrusionPath& path, double width) {
        Flow new_flow = flow.with_width(unscale<float>(width) + flow.height() * float(1. - 0.25 * PI));
        path.mm3_per_mm = new_flow.mm3_per_mm();
        path.width = new_flow.width();
        path.height = new_flow.height();
        };

    auto append_path_and_reset = [role, &paths](double& length, double& sum, ExtrusionPath& path){
            length = sum = 0;
            paths.emplace_back(std::move(path));
            path = ExtrusionPath(role);
        };

    for (int i = 0; i < (int)lines.size(); ++i) {
        const FloatingThickline line = lines[i]; // Insertion below may reallocate lines.

        if (i == 0) {
            max_width = min_width = line.a_width;
        }

        const coordf_t line_len = line.length();
        if (line_len < SCALED_EPSILON) continue;

        double thickness_delta = std::max(fabs(max_width - line.b_width), fabs(min_width - line.b_width));
        //BBS: has large difference in width
        if (thickness_delta > tolerance) {
            //BBS: 1 generate path from start_index to i(not included)
            if (start_index != i) {
                path = ExtrusionPath(role);
                double length = 0, sum = 0;
                bool is_floating = false;
                for (int idx = start_index; idx < i; idx++) {
                    bool curr_floating = lines[idx].is_a_floating && lines[idx].is_b_floating;
                    if (curr_floating != is_floating && length != 0) {
                        path.polyline.append(lines[idx].a);
                        if (is_floating)
                            path.set_customize_flag(CustomizeFlag::cfFloatingVerticalShell);
                        set_flow_for_path(path, sum / length);
                        append_path_and_reset(length, sum, path);
                    }
                    is_floating = curr_floating;

                    double line_length = lines[idx].length();
                    length += line_length;
                    sum += line_length * (lines[idx].a_width + lines[idx].b_width) * 0.5;
                    path.polyline.append(lines[idx].a);
                }
                path.polyline.append(lines[i].a);
                if (length > SCALED_EPSILON) {
                    if (is_floating)
                        path.set_customize_flag(CustomizeFlag::cfFloatingVerticalShell);
                    set_flow_for_path(path, sum / length);
                    paths.emplace_back(std::move(path));
                }
            }

            start_index = i;
            max_width = line.a_width;
            min_width = line.a_width;

            //BBS: 2 handle the i-th segment
            thickness_delta = fabs(line.a_width - line.b_width);
            if (thickness_delta > tolerance) {
                const unsigned int segments = (unsigned int)ceil(thickness_delta / tolerance);
                const coordf_t seg_len = line_len / segments;
                Points pp;
                std::vector<coordf_t> width;
                {
                    pp.push_back(line.a);
                    width.push_back(line.a_width);
                    for (size_t j = 1; j < segments; ++j) {
                        pp.push_back((line.a.cast<double>() + (line.b - line.a).cast<double>().normalized() * (j * seg_len)).cast<coord_t>());

                        coordf_t w = line.a_width + (j * seg_len) * (line.b_width - line.a_width) / line_len;
                        width.push_back(w);
                        width.push_back(w);
                    }
                    pp.push_back(line.b);
                    width.push_back(line.b_width);

                    assert(pp.size() == segments + 1u);
                    assert(width.size() == segments * 2);
                }

                // delete this line and insert new ones
                lines.erase(lines.begin() + i);
                for (size_t j = 0; j < segments; ++j) {
                    FloatingThickline new_line(pp[j], pp[j + 1],width[2 * j],width[2 * j+1], line.is_a_floating,line.is_b_floating);
                    lines.insert(lines.begin() + i + j, new_line);
                }
                --i;
                continue;
            }
        }
        //BBS: just update the max and min width and continue
        else {
            max_width = std::max(max_width, std::max(line.a_width, line.b_width));
            min_width = std::min(min_width, std::min(line.a_width, line.b_width));
        }
    }
    //BBS: handle the remaining segment
    size_t final_size = lines.size();
    if (start_index < final_size) {
        path = ExtrusionPath(role);
        double length = 0, sum = 0;
        bool is_floating = false;
        for (int idx = start_index; idx < final_size; idx++) {
            bool curr_floating = lines[idx].is_a_floating && lines[idx].is_b_floating;
            if (curr_floating!= is_floating && length != 0) {
                path.polyline.append(lines[idx].a);
                if(is_floating)
                    path.set_customize_flag(CustomizeFlag::cfFloatingVerticalShell);
                set_flow_for_path(path, sum / length);
                append_path_and_reset(length, sum, path);
            }
            is_floating = curr_floating;
            double line_length = lines[idx].length();
            length += line_length;
            sum += line_length * (lines[idx].a_width + lines[idx].b_width) * 0.5;
            path.polyline.append(lines[idx].a);

        }
        path.polyline.append(lines[final_size - 1].b);
        if (length > SCALED_EPSILON) {
            if (lines[final_size - 1].is_a_floating && lines[final_size - 1].is_b_floating)
                path.set_customize_flag(CustomizeFlag::cfFloatingVerticalShell);
            set_flow_for_path(path, sum / length);
            paths.emplace_back(std::move(path));
        }
    }

    return paths;
}

namespace {
// Douglas-Peucker-style simplification for narrow floating infill.
// This non-bridge converter treats width as spacing s: W = s + h*(1-PI/4), q = h*s.
// Bound centerline, width and cumulative volume errors against the reconstructed input.
struct NarrowInfillSpan {
    Point a, b;                 // Scaled integer coordinates.
    double a_spacing, b_spacing; // Endpoint spacing in mm, linear along the span.
    double length;              // Length in mm before coordinate rounding.
    bool floating;              // True only if both source vertices float.
};

struct NarrowInfillLimits {
    double height;
    double nozzle;
    double geometry_error;

    double width_error(bool floating) const { return nozzle / (floating ? 16. : 8.); }
    double material_error(bool floating) const { return height * nozzle * nozzle / (floating ? 128. : 64.); }
};

// Half-open span range and candidate evaluation.
struct NarrowInfillCandidate {
    size_t begin, end, split;
    double worst = 0.;
    double q = 0.;

    NarrowInfillCandidate(size_t first, size_t last)
        : begin(first), end(last), split(first + (last - first) / 2) {}

    // Error 1 reaches the limit; strict > preserves tie-breaking.
    // Split at an existing span boundary nearest the error location.
    void record(double error, size_t edge, double t)
    {
        if (error > worst) {
            worst = error;
            if (end - begin > 1)
                split = std::clamp(edge + size_t(t >= 0.5), begin + 1, end - 1);
        }
    }
};

static bool preserve_narrow_infill_polyline(const FloatingThickPolyline& line, const NarrowInfillLimits& budget)
{
    const double height = budget.height;
    // Preserve nearly constant-width polylines for downstream arc fitting.
    const bool uniform_state = std::all_of(line.is_floating.begin(), line.is_floating.end(),
        [&](bool floating) { return floating == line.is_floating.front(); });
    const auto limits = std::minmax_element(line.width.begin(), line.width.end());
    if (*limits.first == *limits.second)
        return true; // Legacy conversion still splits mixed floating states.
    if (uniform_state && unscale<double>(*limits.second - *limits.first) <=
        std::min(0.05, budget.width_error(line.is_floating.front())) - EPSILON) {
        double total_length = 0., total_volume = 0.;
        for (size_t i = 0; i + 1 < line.points.size(); ++i) {
            const double length = unscale<double>((line.points[i + 1] - line.points[i]).cast<double>().norm());
            total_length += length;
            total_volume += height * length * unscale<double>(line.width[2 * i] + line.width[2 * i + 1]) * 0.5;
        }
        // q is nominal volume per unit length (mm^3/mm), using the original polyline length.
        const double q = total_length > 0. ? total_volume / total_length : 0.;
        double residual = 0., largest_residual = 0.;
        for (size_t i = 0; i + 1 < line.points.size(); ++i) {
            const double length = unscale<double>((line.points[i + 1] - line.points[i]).cast<double>().norm());
            const double wa = unscale<double>(line.width[2 * i]);
            const double wb = unscale<double>(line.width[2 * i + 1]);
            // The cumulative volume residual is quadratic; check its interior extremum.
            if (wa != wb) {
                const double t = (q / height - wa) / (wb - wa);
                if (t > 0. && t < 1.)
                    largest_residual = std::max(largest_residual, std::abs(residual + length *
                        ((height * wa - q) * t + 0.5 * height * (wb - wa) * t * t)));
            }
            residual += length * (height * (wa + wb) * 0.5 - q);
            largest_residual = std::max(largest_residual, std::abs(residual));
        }
        if (q > 0. && largest_residual <= budget.material_error(line.is_floating.front()))
            return true;
    }
    return false;
}

static bool build_narrow_infill_spans(const FloatingThickPolyline& line, const NarrowInfillLimits& budget,
    std::vector<NarrowInfillSpan>& spans)
{
    const double height = budget.height;
    // Seed split points for width ramps; DP may merge these spans again.
    spans.reserve(line.points.size());
    for (size_t edge = 0; edge + 1 < line.points.size(); ++edge) {
        const Point& a = line.points[edge];
        const Point& b = line.points[edge + 1];
        const double length = unscale<double>((b - a).cast<double>().norm());
        if (a == b)
            continue;
        // Each edge owns two widths; shared vertices may have different adjacent widths.
        const double wa = unscale<double>(line.width[2 * edge]);
        const double wb = unscale<double>(line.width[2 * edge + 1]);
        if (!std::isfinite(wa) || !std::isfinite(wb) || wa <= 0 || wb <= 0)
            return false;
        const bool floating = line.is_floating[edge] && line.is_floating[edge + 1];
        // For N equal pieces: width error = delta/(2*N), cumulative volume error = h*delta*L/(8*N*N).
        // The denominators include a 1% margin; rounded geometry is checked again below.
        const double delta = std::abs(wb - wa);
        const double pieces = std::max(delta / (1.98 * budget.width_error(floating)),
            std::sqrt(height * delta * length / (7.92 * budget.material_error(floating))));
        // Bound subdivision for pathological input.
        if (!std::isfinite(pieces) || pieces > 4096.)
            return false;
        const size_t count = std::max<size_t>(1, size_t(std::ceil(pieces)));
        Point previous = a;
        for (size_t part = 0; part < count; ++part) {
            const double t0 = double(part) / count;
            const double t1 = double(part + 1) / count;
            const Point next = part + 1 == count ? b :
                Point((a.cast<double>() + (b - a).cast<double>() * t1).cast<coord_t>());
            // Reject collapsed spans rather than lose their material.
            if (next == previous)
                return false;
            // Integrate with the original length to avoid rounding-induced volume changes.
            spans.push_back({previous, next, wa + (wb - wa) * t0,
                wa + (wb - wa) * t1, length / count, floating});
            previous = next;
        }
    }
    return !spans.empty();
}

static void fill_narrow_infill_material_prefix(const std::vector<NarrowInfillSpan>& spans, double height,
    std::vector<double>& material)
{
    // Prefix volumes in mm^3: V_i = h*L_i*(s_a+s_b)/2.
    // Range [begin,end) uses material[end]-material[begin].
    for (size_t i = 0; i < spans.size(); ++i)
        material[i + 1] = material[i] + height * spans[i].length *
            (spans[i].a_spacing + spans[i].b_spacing) * 0.5;
}

static inline void check_narrow_infill_material(const NarrowInfillSpan& span, double height, double volume,
    double start_volume, double ta, double tb, double material_limit, size_t i, NarrowInfillCandidate& candidate)
{
    const auto check_material = [&](double t) {
        // Compare cumulative volumes from the candidate start at matching projected positions.
        const double expected = start_volume + height * span.length *
            (span.a_spacing * t + 0.5 * (span.b_spacing - span.a_spacing) * t * t);
        const double proposed = volume * (ta + (tb - ta) * t);
        candidate.record(std::abs(expected - proposed) / material_limit, i, t);
    };
    check_material(0.);
    check_material(1.);
    // The residual is quadratic; also test any interior stationary point.
    const double slope = height * span.length * (span.b_spacing - span.a_spacing);
    if (std::abs(slope) > 1e-15) {
        const double t = (volume * (tb - ta) - height * span.length * span.a_spacing) / slope;
        if (t > 0. && t < 1.)
            check_material(t);
    }
}

static void evaluate_narrow_infill_candidate(const std::vector<NarrowInfillSpan>& spans,
    const std::vector<double>& material, const NarrowInfillLimits& budget, NarrowInfillCandidate& candidate)
{
    const double height = budget.height;
    const size_t begin = candidate.begin, end = candidate.end;
    // Replace the span range with its endpoint chord, measured in mm.
    const Point& a = spans[begin].a;
    const Point& b = spans[end - 1].b;
    const Vec2d chord = (b - a).cast<double>() * SCALING_FACTOR;
    const double length = chord.norm();
    // Reject zero-length chords, including unsplit closed loops.
    if (length <= EPSILON) {
        candidate.worst = std::numeric_limits<double>::infinity();
        return;
    }
    // Preserve input volume: q = volume / chord length; width and distribution still need checks.
    const double volume = material[end] - material[begin];
    candidate.q = volume / length;
    const double spacing = candidate.q / height;
    const double width_limit = budget.width_error(spans[begin].floating);
    const double material_limit = budget.material_error(spans[begin].floating);
    double previous_projection = 0.;
    for (size_t i = begin; i < end; ++i) {
        const NarrowInfillSpan& span = spans[i];
        const Vec2d pa = (span.a - a).cast<double>() * SCALING_FACTOR;
        const Vec2d pb = (span.b - a).cast<double>() * SCALING_FACTOR;
        // ta/tb are chord projections, unlike the span-local parameter t used below.
        const double ta = pa.dot(chord) / chord.squaredNorm();
        const double tb = pb.dot(chord) / chord.squaredNorm();
        // Require forward projections within the chord to reject hairpin shortcuts.
        if (ta < previous_projection - 1e-9 || ta < -1e-9 || tb > 1. + 1e-9 || tb <= ta) {
            candidate.record(std::numeric_limits<double>::infinity(), i, 0.5);
            break;
        }
        previous_projection = tb;
        // Centerline distance is maximal at a span endpoint.
        candidate.record((pa - ta * chord).norm() / budget.geometry_error, i, 0.);
        candidate.record((pb - tb * chord).norm() / budget.geometry_error, i, 1.);
        // Linear spacing error is maximal at an endpoint; fixed h gives the same width error.
        candidate.record(std::abs(spacing - span.a_spacing) / width_limit, i, 0.);
        candidate.record(std::abs(spacing - span.b_spacing) / width_limit, i, 1.);
        // Check cumulative volume distribution, not just the total.
        const double start_volume = material[i] - material[begin];
        check_narrow_infill_material(span, height, volume, start_volume,
            ta, tb, material_limit, i, candidate);
    }
}

static void append_narrow_infill_candidate(const std::vector<NarrowInfillSpan>& spans,
    const NarrowInfillCandidate& candidate, ExtrusionRole role, const Flow& flow, ExtrusionPaths& result)
{
    const Point& a = spans[candidate.begin].a;
    const Point& b = spans[candidate.end - 1].b;
    const double q = candidate.q;
    const double height = flow.height();
    // Emit a fixed-q two-point path directly, avoiding another width-conversion pass.
    // Output width W = q/h + h*(1-PI/4); mm3_per_mm stores q.
    ExtrusionPath path(role);
    path.polyline.points = {a, b};
    path.height = flow.height();
    path.width = Flow::rounded_rectangle_extrusion_width_from_spacing(float(q / height), flow.height());
    path.mm3_per_mm = q;
    if (spans[candidate.begin].floating)
        path.set_customize_flag(CustomizeFlag::cfFloatingVerticalShell);
    result.emplace_back(std::move(path));
}

static bool narrow_infill_loop_collapsed(const FloatingThickPolyline& line, const ExtrusionPaths& result)
{
    // Reject collinear closed-loop output; this is not a full topology check.
    if (line.is_closed() && !result.empty()) {
        const Point& origin = result.front().first_point();
        const Vec2d direction = (result.front().last_point() - origin).cast<double>();
        const bool has_area = std::any_of(result.begin(), result.end(), [&](const ExtrusionPath& path) {
            const Vec2d offset = (path.last_point() - origin).cast<double>();
            return direction.x() * offset.y() != direction.y() * offset.x();
        });
        if (!has_area)
            return true;
    }
    return false;
}

// Own all buffers and fall back for the entire input if any stage fails.
static ExtrusionPaths narrow_floating_infill_paths(const FloatingThickPolyline& line,
    ExtrusionRole role, const Flow& flow, double resolution)
{
    // Discard partial results on fallback; retain the legacy 0.05 mm tolerance.
    const auto legacy = [&] {
        return floating_thick_polyline_to_extrusion_paths(line, role, flow, float(scale_(0.05)));
    };
    if (line.points.size() < 2)
        return {};
    // Supported spans within narrow floating infill are eligible; bridge flows use legacy conversion.
    if (role != erFloatingVerticalShell || flow.bridge() || flow.height() <= 0 ||
        flow.nozzle_diameter() <= 0 || !std::isfinite(resolution) || resolution <= 0)
        return legacy();

    // Geometry/width budgets are in mm; volume budgets are in mm^3.
    const NarrowInfillLimits budget{flow.height(), flow.nozzle_diameter(), std::min(resolution, 0.012)};
    if (preserve_narrow_infill_polyline(line, budget))
        return legacy();

    std::vector<NarrowInfillSpan> spans;
    if (!build_narrow_infill_spans(line, budget, spans))
        return legacy();
    std::vector<double> material(spans.size() + 1, 0.);
    fill_narrow_infill_material_prefix(spans, budget.height, material);

    ExtrusionPaths result;
    std::vector<std::pair<size_t, size_t>> pending;
    // Merge only contiguous spans with the same floating state; pending ranges are half-open.
    for (size_t first = 0; first < spans.size();) {
        size_t last = first + 1;
        while (last < spans.size() && spans[last].floating == spans[first].floating &&
               spans[last - 1].b == spans[last].a)
            ++last;
        pending.emplace_back(first, last);
        while (!pending.empty()) {
            const auto [begin, end] = pending.back();
            pending.pop_back();
            NarrowInfillCandidate candidate(begin, end);
            // Cap candidate checks at 256 spans to bound worst-case DP work.
            if (end - begin > 256) {
                pending.emplace_back(candidate.split, end);
                pending.emplace_back(begin, candidate.split);
                continue;
            }
            evaluate_narrow_infill_candidate(spans, material, budget, candidate);
            // Allow normalized rounding slack; a failing single span triggers whole-line fallback.
            if (candidate.worst > 1. + 1e-7) {
                if (end - begin == 1)
                    return legacy();
                // Push right first so the left half is emitted first.
                pending.emplace_back(candidate.split, end);
                pending.emplace_back(begin, candidate.split);
                continue;
            }
            append_narrow_infill_candidate(spans, candidate, role, flow, result);
        }
        first = last;
    }
    if (narrow_infill_loop_collapsed(line, result))
        return legacy();
    return result;
}
} // namespace

// Two widths per edge and one floating marker per vertex.
ExtrusionPaths FloatingThickPolyline::to_extrusion_paths(ExtrusionRole role, const Flow& flow, double resolution) const
{
    assert(width.size() == (points.empty() ? 0 : 2 * (points.size() - 1)));
    assert(is_floating.size() == points.size());
    return narrow_floating_infill_paths(*this, role, flow, resolution);
}

static double interpolate_width(const ZPath& path,
    const ThickPolyline& line,
    const int subject_idx_range,
    const int default_width,
    size_t idx)
{
    int prev_idx = idx;
    while (prev_idx >= 0 && (path[prev_idx].z() < 0 || path[prev_idx].z() >= subject_idx_range))
        --prev_idx;

    int next_idx = idx;
    while (next_idx < path.size() && (path[next_idx].z() < 0 || path[next_idx].z() >= subject_idx_range))
        ++next_idx;

    double width_prev;
    double width_next;
    if (prev_idx < 0) {
        width_prev = default_width;
    }
    else {
        size_t prev_z_idx = path[prev_idx].z();
        width_prev = get_width_at(line, prev_z_idx);
    }

    if (next_idx >= path.size()) {
        width_next = default_width;
    }
    else {
        size_t next_z_idx = path[next_idx].z();
        width_next = get_width_at(line, next_z_idx);
    }
    // A clipped end may have no original vertex on one side. Interpolate on the
    // nearest original segment instead of indexing past the clipped path.
    if (prev_idx < 0 || next_idx >= int(path.size())) {
        const Vec2d point = Point(path[idx].x(), path[idx].y()).cast<double>();
        double best_distance = std::numeric_limits<double>::max();
        double result = default_width;
        for (size_t edge = 0; edge + 1 < line.points.size(); ++edge) {
            const Vec2d a = line.points[edge].cast<double>();
            const Vec2d ab = line.points[edge + 1].cast<double>() - a;
            const double squared_length = ab.squaredNorm();
            const double t = squared_length > 0 ? std::clamp((point - a).dot(ab) / squared_length, 0.0, 1.0) : 0.0;
            const double distance = (point - a - t * ab).squaredNorm();
            if (distance < best_distance) {
                best_distance = distance;
                result = (1.0 - t) * line.width[2 * edge] + t * line.width[2 * edge + 1];
            }
        }
        return result;
    }
    Point prev(path[prev_idx].x(), path[prev_idx].y());
    Point next(path[next_idx].x(), path[next_idx].y());
    Point curr(path[idx].x(), path[idx].y());
    double d_total = (next - prev).cast<double>().norm();
    double d_curr = (curr - prev).cast<double>().norm();
    double t = (d_total > 0) ? (d_curr / d_total) : 0.0;
    return (1 - t) * width_prev + t * width_next;
}

static FloatingThickPolyline merge_lines(ZPaths lines, const std::vector<bool>& mark_flags, const ThickPolyline& line, const int subject_idx_range ,const int default_width)
{
    using PathFlag = std::vector<bool>;
    using PathFlags = std::vector<PathFlag>;

    std::vector<bool>used(lines.size(), false);
    ZPaths merged_paths;
    PathFlags merged_marks;

    auto update_path_flag = [](PathFlag& mark_flags, const ZPath& path, bool mark) {
        for (auto p : path)
            mark_flags.emplace_back(mark);
        };

    std::unordered_map<int64_t, std::unordered_set<size_t>> start_z_map;
    std::unordered_map<int64_t, std::unordered_set<size_t>> end_z_map;

    for (size_t idx = 0; idx < lines.size(); ++idx) {
        if (lines[idx].empty()) {
            used[idx] = true;
            continue;
        }
        start_z_map[lines[idx].front().z()].insert(idx);
        end_z_map[lines[idx].back().z()].insert(idx);
    }

    auto remove_from_map = [&start_z_map, &end_z_map, &lines](size_t idx) {
        if (lines[idx].empty())
            return;
        int64_t start_z = lines[idx].front().z();
        int64_t end_z = lines[idx].back().z();
        start_z_map[start_z].erase(idx);
        if (start_z_map[start_z].empty())
            start_z_map.erase(start_z);
        end_z_map[end_z].erase(idx);
        if (end_z_map[end_z].empty())
            end_z_map.erase(end_z);
        };

    for (size_t idx = 0; idx < lines.size(); ++idx) {
        if (used[idx])
            continue;
        ZPath curr_path = lines[idx];
        PathFlag curr_mark;
        update_path_flag(curr_mark, curr_path, mark_flags[idx]);
        used[idx] = true;
        remove_from_map(idx);

        bool merged;
        do {
            merged = false;
            int64_t curr_end = curr_path.back().z();
            int64_t curr_start = curr_path.front().z();

            // search after
            {
                if (auto start_iter = start_z_map.find(curr_end);start_iter != start_z_map.end()) {
                    size_t j = *start_iter->second.begin();
                    remove_from_map(j);
                    curr_path.insert(curr_path.end(), lines[j].begin(), lines[j].end());
                    update_path_flag(curr_mark, lines[j], mark_flags[j]);
                    used[j] = true;
                    merged = true;
                }
                else if (auto end_iter = end_z_map.find(curr_end); end_iter != end_z_map.end()) {
                    size_t j = *end_iter->second.begin();
                    remove_from_map(j);
                    std::reverse(lines[j].begin(), lines[j].end());
                    curr_path.insert(curr_path.end(), lines[j].begin(), lines[j].end());
                    update_path_flag(curr_mark, lines[j], mark_flags[j]);
                    used[j] = true;
                    merged = true;
                }
            }

            if (merged)
                continue;

            //search before
            {
                if (auto end_iter = end_z_map.find(curr_start);end_iter != end_z_map.end()) {
                    size_t j = *end_iter->second.begin();
                    remove_from_map(j);
                    ZPath new_path = lines[j];
                    PathFlag new_mark;
                    update_path_flag(new_mark, new_path, mark_flags[j]);

                    new_path.insert(new_path.end(), curr_path.begin(), curr_path.end());
                    new_mark.insert(new_mark.end(), curr_mark.begin(), curr_mark.end());
                    curr_path = std::move(new_path);
                    curr_mark = std::move(new_mark);
                    used[j] = true;
                    merged = true;
                }
                else if (auto start_iter = start_z_map.find(curr_start); start_iter != start_z_map.end()) {
                    size_t j = *start_iter->second.begin();
                    remove_from_map(j);
                    ZPath new_path = lines[j];
                    std::reverse(new_path.begin(), new_path.end());
                    PathFlag new_mark;
                    update_path_flag(new_mark, new_path, mark_flags[j]);

                    new_path.insert(new_path.end(), curr_path.begin(), curr_path.end());
                    new_mark.insert(new_mark.end(), curr_mark.begin(), curr_mark.end());
                    curr_path = std::move(new_path);
                    curr_mark = std::move(new_mark);
                    used[j] = true;
                    merged = true;
                }
            }

        } while (merged);

        merged_paths.emplace_back(curr_path);
        merged_marks.emplace_back(curr_mark);
    }

    // If clipping cannot be stitched into one path, retain the complete original
    // extrusion instead of dropping components or accessing an empty result.
    if (merged_paths.size() != 1 || merged_paths.front().size() < 2) {
        FloatingThickPolyline original;
        static_cast<ThickPolyline&>(original) = line;
        original.is_floating.assign(line.points.size(), false);
        return original;
    }

    FloatingThickPolyline res;

    auto& valid_path = merged_paths.front();
    auto& valid_mark = merged_marks.front();

    for (size_t idx = 0; idx < valid_path.size(); ++idx) {
        int zvalue = valid_path[idx].z();
        res.points.emplace_back(valid_path[idx].x(), valid_path[idx].y());
        res.is_floating.emplace_back(valid_mark[idx]);
        if (0 <= zvalue && zvalue < subject_idx_range) {
            // Match BambuStudio 56a57d00's width reconstruction for compatibility.
            // Preserve this reconstruction as the input to narrow-infill DP.
            res.width.emplace_back(get_width_at(line, prev_idx_modulo(zvalue, line.points)));
            res.width.emplace_back(get_width_at(line, zvalue));
        }
        else {
            double width = interpolate_width(valid_path, line, subject_idx_range, default_width, idx);
            res.width.emplace_back(width);
            res.width.emplace_back(width);
        }
    }
    res.width = std::vector<coordf_t>(res.width.begin() + 1, res.width.end()-1);
    assert(res.width.size() == 2 * res.points.size() - 2);

    return res;
}

FloatingThickPolyline FillFloatingConcentric::detect_floating_line(const ThickPolyline& line, const ExPolygons& floating_areas, const double default_width ,bool force_no_detect)
{
    {
        Polyline polyline = line;
        auto bbox_line = get_extents(polyline);
        auto bbox_area = get_extents(floating_areas);
        if (force_no_detect || floating_areas.empty() || !bbox_area.overlap(bbox_line) || intersection_pl(Polylines{polyline}, floating_areas).empty()) {
            FloatingThickPolyline res;
            res.width = line.width;
            res.points = line.points;
            res.is_floating.resize(res.points.size(), false);
            return res;
        }
    }

    using ZPoint = ClipperLib_Z::IntPoint;
    auto hash_function = [](const int a1, const int b1, const int a2, const int b2)->int32_t {
        uint32_t hash_val = 1000u * (uint32_t(a1) * 13u + uint32_t(b1)) + uint32_t(a2) * 17u + uint32_t(b2) + 1u;
        hash_val &= 0x7fffffff;
        return hash_val;
        };

    int idx = 0;
    int subject_idx_range;
    ZPaths subject_paths;
    {
        subject_paths.emplace_back();
        for (auto p : line)
            subject_paths.back().emplace_back(p.x(), p.y(), idx++);
    }

    subject_idx_range = idx;
    ZPaths clip_paths;
    {
        Polygons floating_polygons = to_polygons(floating_areas);
        for (auto& poly : floating_polygons) {
            clip_paths.emplace_back();
            for (const auto& p : poly)
                clip_paths.back().emplace_back(p.x(), p.y(), idx++);
        }
    }

    ClipperLib_Z::ZFillCallback z_filler = [hash_function, subject_idx_range](const ZPoint& e1_a, const ZPoint& e1_b, const ZPoint& e2_a, const ZPoint& e2_b, ZPoint& d) {
        if (e1_a.z() < subject_idx_range && e1_b.z() < subject_idx_range && e2_a.z() < subject_idx_range && e2_b.z() < subject_idx_range) {
            BOOST_LOG_TRIVIAL(error) << Slic3r::format("ZFiller: both point in subject : %d, %d, %d, %d ", e1_a.z(), e1_b.z(), e2_a.z(), e2_b.z());
        }
        if (e1_a.z() >= subject_idx_range && e1_b.z() >= subject_idx_range && e2_a.z() >= subject_idx_range && e2_b.z() >= subject_idx_range) {
            BOOST_LOG_TRIVIAL(error) << Slic3r::format("ZFiller: both point in clip : %d, %d, %d, %d ", e1_a.z(), e1_b.z(), e2_a.z(), e2_b.z());
        }
        if (e1_a.z() < 0 || e1_b.z() < 0 || e2_a.z() < 0 || e2_b.z() < 0)
            BOOST_LOG_TRIVIAL(error) << Slic3r::format("ZFiller: Encounter negative z : %d, %d, %d, %d ", e1_a.z(), e1_b.z(), e2_a.z(), e2_b.z());
        if (e1_a.z() == e1_b.z() || e2_a.z() == e2_b.z()) {
            BOOST_LOG_TRIVIAL(error) << Slic3r::format("ZFiller: Encounter same z in one line : %d, %d, %d, %d ", e1_a.z(), e1_b.z(), e2_a.z(), e2_b.z());
        }
        if (e1_a.z() == e1_b.z() && e1_b.z() == e2_a.z() && e2_a.z() == e2_b.z()) {
            BOOST_LOG_TRIVIAL(error) << Slic3r::format("ZFiller: Encounter same z in both line : %d, %d, %d, %d ", e1_a.z(), e1_b.z(), e2_a.z(), e2_b.z());
            // the intersect is generated by two lines in subject
            d.z() = e1_a.z();
            return;
        }   // the intersect is generate by two line from subject and clip
        d.z() = -hash_function(e1_a.z(), e1_b.z(), e2_a.z(), e2_b.z());
        if (d.z() >= 0)
            BOOST_LOG_TRIVIAL(error) << Slic3r::format("ZFiller: hash function generate postive value : %d", d.z());
    };


    ZPaths intersect_out;
    {
        ClipperLib_Z::Clipper c;
        ClipperLib_Z::PolyTree polytree;
        c.ZFillFunction(z_filler);
        c.AddPaths(subject_paths, ClipperLib_Z::ptSubject, false);
        c.AddPaths(clip_paths, ClipperLib_Z::ptClip, true);
        c.Execute(ClipperLib_Z::ctIntersection, polytree, ClipperLib_Z::pftNonZero);
        ClipperLib_Z::PolyTreeToPaths(std::move(polytree), intersect_out);
    }

    ZPaths diff_out;
    {
        ClipperLib_Z::Clipper c;
        ClipperLib_Z::PolyTree polytree;
        c.ZFillFunction(z_filler);
        c.AddPaths(subject_paths, ClipperLib_Z::ptSubject, false);
        c.AddPaths(clip_paths, ClipperLib_Z::ptClip, true);
        c.Execute(ClipperLib_Z::ctDifference, polytree, ClipperLib_Z::pftNonZero);
        ClipperLib_Z::PolyTreeToPaths(std::move(polytree), diff_out);
    }


    ZPaths  to_merge = diff_out;
    to_merge.insert(to_merge.end(), intersect_out.begin(), intersect_out.end());
    std::vector<bool>floating_flags(to_merge.size(), false);
    for (size_t idx = diff_out.size(); idx < diff_out.size() + intersect_out.size(); ++idx)
        floating_flags[idx] = true;

    for (size_t idx = 0; idx < to_merge.size(); ++idx) {
        for (auto iter = to_merge[idx].begin(); iter != to_merge[idx].end();++iter) {
            if (iter->z() >= subject_idx_range) {
                BOOST_LOG_TRIVIAL(error) << Slic3r::format("ZFiller: encounter idx from clip: %d",iter->z());
            }
        }
    }

    return merge_lines(to_merge,floating_flags,line,subject_idx_range,default_width);
}

static int start_none_floating_idx(int idx, const std::vector<int>& none_floating_count)
{
    int backtrace_idx = idx - none_floating_count[idx] + 1;
    if (backtrace_idx >= 0)
        return backtrace_idx;
    else
        return none_floating_count.size() + backtrace_idx;
}

template<typename PointContainer>
void get_none_floating_prefix(const PointContainer& container, const ExPolygons& floating_areas, const Polygons& sparse_polys, std::vector<double>& none_floating_length, std::vector<int>& none_floating_count)
{
    std::vector<double>(container.points.size(), 0).swap(none_floating_length);
    std::vector<int>(container.points.size(), 0).swap(none_floating_count);

    std::vector<BoundingBox> floating_bboxs;
    for (size_t idx = 0; idx < floating_areas.size(); ++idx)
        floating_bboxs.emplace_back(get_extents(floating_areas[idx]));
    std::vector<BoundingBox> sparse_bboxs;
    for (size_t idx = 0; idx < sparse_polys.size(); ++idx)
        sparse_bboxs.emplace_back(get_extents(sparse_polys[idx]));

    auto point_in_floating_area = [&floating_bboxs, &sparse_bboxs, &floating_areas, &sparse_polys](const Point& p)->bool {
        for (size_t idx = 0; idx < sparse_polys.size(); ++idx) {
            if (!sparse_bboxs[idx].contains(p))
                continue;
            if (sparse_polys[idx].contains(p))
                return false;
        }
        for (size_t idx = 0; idx < floating_areas.size(); ++idx) {
            if (!floating_bboxs[idx].contains(p))
                continue;
            if (floating_areas[idx].contains(p))
                return true;
        }

        return false;
    };

    for (size_t idx = 0; idx < container.points.size(); ++idx) {
        const Point& p = container.points[idx];
        if (!point_in_floating_area(p)) {
            if (idx == 0)
                none_floating_count[idx] = 1;
            else
                none_floating_count[idx] = none_floating_count[idx - 1] + 1;
            if (none_floating_count[idx] > 1)
                none_floating_length[idx] = none_floating_length[idx - 1] + ((Point)(container.points[prev_idx_modulo(idx, container.points)] - p)).cast<double>().norm();
            else
                none_floating_length[idx] = 0;
        }
        else {
            none_floating_length[idx] = 0;
            none_floating_count[idx] = 0;
        }
    }

    if (none_floating_count.back() > 0) {
        for (size_t idx = 0; idx < container.points.size(); ++idx) {
            if (none_floating_count[idx] == 0)
                break;
            none_floating_count[idx] = none_floating_count[prev_idx_modulo(idx, container.points)] + 1;
            none_floating_length[idx] = none_floating_length[prev_idx_modulo(idx, container.points)] + ((Point)(container.points[prev_idx_modulo(idx, container.points)] - container.points[idx])).cast<double>().norm();
        }
    }
}

template<typename PointContainer>
int get_best_loop_start(const PointContainer& container, const ExPolygons& floating_areas, const Polygons& sparse_polys) {
    std::vector<double> none_floating_length;
    std::vector<int> none_floating_count;

    BoundingBox floating_bbox = get_extents(floating_areas);
    BoundingBox poly_bbox(container.points);

    if (!poly_bbox.overlap(floating_bbox))
        return 0;

    Polygons clipped_sparse_polys = ClipperUtils::clip_clipper_polygons_with_subject_bbox(sparse_polys, poly_bbox);
    get_none_floating_prefix(container, floating_areas, clipped_sparse_polys, none_floating_length, none_floating_count);
    int best_idx = std::distance(none_floating_length.begin(), std::max_element(none_floating_length.begin(), none_floating_length.end()));
    return start_none_floating_idx(best_idx, none_floating_count);
}

template<typename PointContainer>
std::vector<int> get_loop_start_candidates(const PointContainer& container, const ExPolygons& floating_areas, const Polygons& sparse_polys)
{
    std::vector<double> none_floating_length;
    std::vector<int> none_floating_count;

    BoundingBox floating_bbox = get_extents(floating_areas);
    BoundingBox poly_bbox(container.points);
    std::vector<int> candidate_list;

    if (!poly_bbox.overlap(floating_bbox)) {
        candidate_list.resize(container.points.size());
        std::iota(candidate_list.begin(), candidate_list.end(), 0);
        return candidate_list;
    }
    Polygons clipped_sparse_polys = ClipperUtils::clip_clipper_polygons_with_subject_bbox(sparse_polys, poly_bbox);
    get_none_floating_prefix(container, floating_areas, clipped_sparse_polys, none_floating_length, none_floating_count);
    for (size_t idx = 0; idx < none_floating_length.size(); ++idx) {
        if (none_floating_length[idx] > 0)
            candidate_list.emplace_back(start_none_floating_idx(idx, none_floating_count));
    }
    return candidate_list;
}


static void smooth_floating_line(FloatingThickPolyline& line,coord_t max_gap_threshold, coord_t min_floating_threshold)
{
    if (line.empty())
        return;
    struct LineParts {
        int start;
        int end;
        bool is_floating;
    };

    auto build_line_parts = [&](const FloatingThickPolyline& line)->std::vector<LineParts> {
        std::vector<LineParts> line_parts;
        bool current_val = line.is_floating.front();
        int start = 0;
        for (size_t idx = 1; idx < line.is_floating.size(); ++idx) {
            if (line.is_floating[idx] != current_val) {
                line_parts.push_back({ start,(int)(idx - 1),current_val });
                current_val = line.is_floating[idx];
                start = idx;
            }
        }
        line_parts.push_back({ start,(int)(line.is_floating.size() - 1),current_val });
        return line_parts;
    };

    std::vector<double> distance_prefix(line.points.size(),0);
    for (size_t idx = 0; idx < line.points.size();++idx) {
        if (idx == 0)
            distance_prefix[idx] = 0;
        else {
            distance_prefix[idx] = distance_prefix[idx - 1] + (line.points[idx] - line.points[idx - 1]).cast<double>().norm();
        }
    }
    {
        // remove too small gaps
        std::vector<LineParts> line_parts = build_line_parts(line);
        std::vector<std::pair<int, int>> gaps_to_merge;

        for (size_t i = 1; i + 1 < line_parts.size(); ++i) {
            const auto& curr = line_parts[i];
            if (!curr.is_floating) {
                const auto& prev = line_parts[i - 1];
                const auto& next = line_parts[i + 1];
                if (prev.is_floating && next.is_floating) {
                    double total_length = distance_prefix[next.start] - distance_prefix[prev.end];
                    if (total_length < max_gap_threshold) {
                        gaps_to_merge.emplace_back(curr.start, curr.end);
                    }
                }
            }
        }

        for (const auto& gap : gaps_to_merge) {
            for (int i = gap.first; i <= gap.second; ++i) {
                line.is_floating[i] = true;
            }
        }
    }

    {
        std::vector<LineParts> line_parts = build_line_parts(line);
        std::vector<std::pair<int, int>> segments_to_remove;

        for (auto& part : line_parts) {
            if (part.is_floating && distance_prefix[part.end] - distance_prefix[part.start] < min_floating_threshold) {
                segments_to_remove.emplace_back(part.start, part.end);
            }
        }

        for (const auto& seg : segments_to_remove) {
            for (int i = seg.first; i <= seg.second; ++i) {
                line.is_floating[i] = false;
            }
        }
    }
}

// nearest neibour排序，但是取点时，只能取get_loop_start_candidates得到的点
FloatingThickPolylines FillFloatingConcentric::resplit_order_loops(Point curr_point, const std::vector<const Arachne::ExtrusionLine*>& all_extrusions, const ExPolygons& floating_areas, const Polygons& sparse_polys, const coord_t default_width)
{
    FloatingThickPolylines result;

    for (size_t idx = 0; idx < all_extrusions.size(); ++idx) {
        if (all_extrusions[idx]->junctions.size() < 2)
            continue;
        ThickPolyline thick_polyline = Arachne::to_thick_polyline(*all_extrusions[idx]);
        bool is_self_intersect = false;
        if(print_object_config->detect_floating_vertical_shell.value)
        {
            Polyline polyline = thick_polyline;
            auto bbox_line = get_extents(polyline);

            // EdgeGrid borrows the contour points, which must outlive the grid.
            const std::vector<Points> grid_contours{ polyline.points };
            EdgeGrid::Grid grid;
            grid.set_bbox(bbox_line);
            grid.create(grid_contours, scaled<coord_t>(10.), !all_extrusions[idx]->is_closed);
            if (grid.has_intersecting_edges())
                is_self_intersect = true;
        }
        FloatingThickPolyline thick_line_with_floating = detect_floating_line(thick_polyline, floating_areas, default_width, !print_object_config->detect_floating_vertical_shell.value || is_self_intersect);
        smooth_floating_line(thick_line_with_floating, scale_(2), scale_(2));
        int split_idx = 0;
        if (!floating_areas.empty() && all_extrusions[idx]->is_closed && thick_line_with_floating.points.front() == thick_line_with_floating.points.back()) {
            if (idx == 0)
                split_idx = get_best_loop_start(thick_line_with_floating, floating_areas,sparse_polys);
            else {
                auto candidates = get_loop_start_candidates(thick_line_with_floating, floating_areas,sparse_polys);
                double min_dist = std::numeric_limits<double>::max();
                for (auto candidate : candidates) {
                    double dist = (curr_point - thick_line_with_floating.points[candidate]).cast<double>().norm();
                    if (min_dist > dist) {
                        min_dist = dist;
                        split_idx = candidate;
                    }
                }
            }
            FloatingThickPolyline new_line = thick_line_with_floating.rebase_at(split_idx);
            assert(new_line.width.size() == 2 * new_line.points.size() - 2);
            result.emplace_back(std::move(new_line));
        }
        else {
            assert(thick_line_with_floating.width.size() == 2 * thick_line_with_floating.points.size() - 2);
            result.emplace_back(thick_line_with_floating);
        }
        curr_point = result.back().last_point();
    }
    return result;
}

// Generate floating paths with Arachne below.

static std::vector<const Arachne::ExtrusionLine*>  toplogic_sort_extruisons(const std::vector<Arachne::ExtrusionLine*>& all_extrusions)
{
    std::vector<const Arachne::ExtrusionLine*> ordered_extrusions;
    // Find topological order with constraints from extrusions_constrains.
    std::vector<size_t>              blocked(all_extrusions.size(), 0); // Value indicating how many extrusions it is blocking (preceding extrusions) an extrusion.
    std::vector<std::vector<size_t>> blocking(all_extrusions.size());   // Each extrusion contains a vector of extrusions that are blocked by this extrusion.
    std::unordered_map<const Arachne::ExtrusionLine*, size_t> map_extrusion_to_idx;
    for (size_t idx = 0; idx < all_extrusions.size(); idx++)
        map_extrusion_to_idx.emplace(all_extrusions[idx], idx);

    auto extrusions_constrains = Arachne::WallToolPaths::getRegionOrder(all_extrusions, true);
    for (auto [before, after] : extrusions_constrains) {
        auto after_it = map_extrusion_to_idx.find(after);
        ++blocked[after_it->second];
        blocking[map_extrusion_to_idx.find(before)->second].emplace_back(after_it->second);
    }

    std::vector<bool> processed(all_extrusions.size(), false);          // Indicate that the extrusion was already processed.
    Point             current_position = all_extrusions.empty() || all_extrusions.front()->junctions.empty() ? Point::Zero() : all_extrusions.front()->junctions.front().p; // Some starting position.
    while (ordered_extrusions.size() < all_extrusions.size()) {
        size_t best_candidate = 0;
        double best_distance_sqr = std::numeric_limits<double>::max();
        bool   is_best_closed = false;

        std::vector<size_t> available_candidates;
        for (size_t candidate = 0; candidate < all_extrusions.size(); ++candidate) {
            if (processed[candidate] || blocked[candidate])
                continue; // Not a valid candidate.
            available_candidates.push_back(candidate);
        }

        // A malformed/cyclic constraint set must not duplicate the first path.
        if (available_candidates.empty()) {
            const auto remaining = std::find(processed.begin(), processed.end(), false);
            available_candidates.emplace_back(size_t(std::distance(processed.begin(), remaining)));
        }

        std::sort(available_candidates.begin(), available_candidates.end(), [&all_extrusions](const size_t a_idx, const size_t b_idx) -> bool {
            return all_extrusions[a_idx]->is_closed < all_extrusions[b_idx]->is_closed;
            });

        for (const size_t candidate_path_idx : available_candidates) {
            auto& path = all_extrusions[candidate_path_idx];

            if (path->junctions.empty()) { // No vertices in the path. Can't find the start position then or really plan it in. Put that at the end.
                if (best_distance_sqr == std::numeric_limits<double>::max()) {
                    best_candidate = candidate_path_idx;
                    is_best_closed = path->is_closed;
                }
                continue;
            }

            const Point candidate_position = path->junctions.front().p;
            double      distance_sqr = (current_position - candidate_position).cast<double>().norm();
            if (distance_sqr < best_distance_sqr) { // Closer than the best candidate so far.
                if (path->is_closed || (!path->is_closed && best_distance_sqr != std::numeric_limits<double>::max()) || (!path->is_closed && !is_best_closed)) {
                    best_candidate = candidate_path_idx;
                    best_distance_sqr = distance_sqr;
                    is_best_closed = path->is_closed;
                }
            }
        }

        auto& best_path = all_extrusions[best_candidate];
        ordered_extrusions.push_back(best_path);
        processed[best_candidate] = true;
        for (size_t unlocked_idx : blocking[best_candidate])
            if (blocked[unlocked_idx] > 0)
                --blocked[unlocked_idx];

        if (!best_path->junctions.empty()) { //If all paths were empty, the best path is still empty. We don't upate the current position then.
            if (best_path->is_closed)
                current_position = best_path->junctions[0].p; //We end where we started.
            else
                current_position = best_path->junctions.back().p; //Pick the other end from where we started.
        }
    }
    return ordered_extrusions;
}

void FillFloatingConcentric::_fill_surface_single(const FillParams& params,
    unsigned int                   thickness_layers,
    const std::pair<float, Point>& direction,
    ExPolygon                      expolygon,
    FloatingThickPolylines& thick_polylines_out)
{
    coord_t min_spacing = params.flow.scaled_spacing();
    if (min_spacing <= 0 || expolygon.contour.empty())
        return;
    Point bbox_size = expolygon.contour.bounding_box().size();

    coord_t                loops_count = std::max(bbox_size.x(), bbox_size.y()) / min_spacing + 1;
    Polygons               polygons = to_polygons(expolygon);

    // Multi-diameter: Arachne thresholds must follow the nozzle that prints this fill, not the smallest one,
    // otherwise the bead split threshold collapses and narrow strips get two overlapping full-width beads.
    double nozzle_diameter = params.flow.nozzle_diameter();
    if (nozzle_diameter <= EPSILON)
        nozzle_diameter = *std::min_element(print_config->nozzle_diameter.values.begin(), print_config->nozzle_diameter.values.end());
    Arachne::WallToolPathsParams input_params{};
    input_params.min_length_factor = 0.5f;
    input_params.is_top_or_bottom_layer = false;
    input_params.min_bead_width = 0.85 * nozzle_diameter;
    input_params.min_feature_size = 0.25 * nozzle_diameter;
    input_params.wall_transition_length = 1.0 * nozzle_diameter;
    input_params.wall_transition_angle = 10;
    input_params.wall_transition_filter_deviation = 0.25 * nozzle_diameter;
    input_params.wall_distribution_count = 1;

    Arachne::WallToolPaths wallToolPaths(polygons, min_spacing, min_spacing, loops_count, 0, params.layer_height, input_params);

    std::vector<Arachne::VariableWidthLines>    loops = wallToolPaths.getToolPaths();
    std::vector<const Arachne::ExtrusionLine*> ordered_extrusions;
    {
        std::vector<Arachne::ExtrusionLine*> all_extrusions;
        for (Arachne::VariableWidthLines& loop : loops) {
            if (loop.empty())
                continue;
            for (Arachne::ExtrusionLine& wall : loop)
                all_extrusions.emplace_back(&wall);
        }
        ordered_extrusions = toplogic_sort_extruisons(all_extrusions);
    }

    // Split paths using a nearest neighbor search.
    size_t firts_poly_idx = thick_polylines_out.size();
    auto thick_polylines = resplit_order_loops({ 0,0 }, ordered_extrusions, this->lower_layer_unsupport_areas, this->lower_sparse_polys,min_spacing);
    append(thick_polylines_out, thick_polylines);


    // clip the paths to prevent the extruder from getting exactly on the first point of the loop
    // Keep valid paths only.
    size_t j = firts_poly_idx;
    for (size_t i = firts_poly_idx; i < thick_polylines_out.size(); ++i) {
        thick_polylines_out[i].clip_end(this->loop_clipping);
        if (thick_polylines_out[i].is_valid()) {
            if (j < i)
                thick_polylines_out[j] = std::move(thick_polylines_out[i]);
            ++j;
        }
    }
    if (j < thick_polylines_out.size())
        thick_polylines_out.erase(thick_polylines_out.begin() + int(j), thick_polylines_out.end());
}


FloatingThickPolylines FillFloatingConcentric::fill_surface_arachne_floating(const Surface* surface, const FillParams& params)
{
    // Create the infills for each of the regions.
    FloatingThickPolylines floating_thick_polylines_out;
    for (ExPolygon& expoly : no_overlap_expolygons)
        _fill_surface_single(params, surface->thickness_layers, _infill_direction(surface), std::move(expoly), floating_thick_polylines_out);
    return floating_thick_polylines_out;
}

void FillFloatingConcentric::fill_surface_extrusion(const Surface* surface, const FillParams& params, ExtrusionEntitiesPtr& out)
{
    assert(print_config != nullptr && print_object_config != nullptr);
    FloatingThickPolylines floating_lines = this->fill_surface_arachne_floating(surface, params);
    // Preserve the topology and supported starts selected by this filler.
    if (floating_lines.empty())
        return;
    Flow new_flow = params.flow.with_spacing(float(this->spacing));

    auto ecc = std::make_unique<ExtrusionEntityCollection>();
    ecc->no_sort = true;

    const double resolution = print_config->resolution.value;
    for (const auto& line : floating_lines) {
        ExtrusionPaths paths = line.to_extrusion_paths(params.extrusion_role, new_flow, resolution);
        // Append paths to collection.
        if (!paths.empty()) {
            if (paths.front().first_point() == paths.back().last_point())
                ecc->entities.emplace_back(new ExtrusionLoop(std::move(paths)));
            else {
                for (ExtrusionPath& path : paths) {
                    assert(!path.empty());
                    ecc->entities.emplace_back(new ExtrusionPath(std::move(path)));
                }
            }
        }
    }
    if (!ecc->entities.empty())
        out.push_back(ecc.release());
}


} // namespace Slic3r
