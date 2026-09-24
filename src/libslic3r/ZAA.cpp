#include "I18N.hpp"
#include "ZAA.hpp"

#include "AABBMesh.hpp"
#include "AABBTreeIndirect.hpp"
#include "ZaaWallSlope.hpp"
#include "Exception.hpp"
#include "ExtrusionEntity.hpp"
#include "GCodeWriter.hpp"

#include <boost/log/trivial.hpp>

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <iterator>
#include <limits>
#include <map>
#include <sstream>
#include <unordered_map>
#include <utility>

namespace Slic3r {

namespace {

constexpr double kNumericalEpsilon          = 0.0001;
constexpr double kPi                        = 3.1415926535897932384626433832795;
constexpr double kRoundedRectangleCorrection = 1.0 - 0.25 * kPi;
constexpr bool kEnableZaaWriterReadyCheck   = false;
constexpr size_t kRaycastGrainSize          = 256;
constexpr size_t kCancellationCheckStride   = 64;

static_assert(static_cast<size_t>(ZaaIncompatibilityReason::SlicePlaneOffsetUnrepresentable) + 1 ==
              ZAA_INCOMPATIBILITY_REASON_COUNT);

bool finite(double value)
{
    return std::isfinite(value);
}

double rounded_rectangle_area(double width, double height)
{
    return height * (width - kRoundedRectangleCorrection * height);
}

double canonical_zero(double value)
{
    return value == 0.0 ? 0.0 : value;
}

bool valid_profile_reason(ZaaInvariantReason reason)
{
    return reason >= ZaaInvariantReason::ProfileStateViolation &&
           reason <= ZaaInvariantReason::EmittedSegmentCollapsed;
}

bool valid_writer_reason(ZaaInvariantReason reason)
{
    return reason >= ZaaInvariantReason::MissingExtruder &&
           reason <= ZaaInvariantReason::PositionMismatch;
}

std::string stable_token(std::string value)
{
    for (char &ch : value)
        if (ch <= ' ' || ch == '=')
            ch = '_';
    return value;
}

bool close_xyz(const Vec3d &lhs, const Vec3d &rhs)
{
    return (lhs - rhs).cwiseAbs().maxCoeff() <= EPSILON;
}

struct ZaaSampleOrigin {
    size_t segment_index{0};
    double segment_ratio{0.0};
    double path_distance_mm{0.0};
};

struct ZaaSample {
    Point xy;
    ZaaSampleOrigin origin;
};

Vec2d point_mm(const Point &point)
{
    return {unscale<double>(point.x()), unscale<double>(point.y())};
}

double point_distance_mm(const Point &lhs, const Point &rhs)
{
    return (point_mm(rhs) - point_mm(lhs)).norm();
}

Point interpolate_scaled(const Point &start, const Point &end, double ratio)
{
    const double x = double(start.x()) + ratio * (double(end.x()) - double(start.x()));
    const double y = double(start.y()) + ratio * (double(end.y()) - double(start.y()));
    return Point(static_cast<coord_t>(std::llround(x)), static_cast<coord_t>(std::llround(y)));
}

size_t checked_size_add(size_t lhs, size_t rhs, const char *message)
{
    if (rhs > std::numeric_limits<size_t>::max() - lhs)
        throw OutOfRange(_u8L(message));
    return lhs + rhs;
}

size_t checked_size_multiply(size_t lhs, size_t rhs, const char *message)
{
    if (lhs != 0 && rhs > std::numeric_limits<size_t>::max() / lhs)
        throw OutOfRange(_u8L(message));
    return lhs * rhs;
}

size_t segment_division_count(double length_mm, double max_spacing_mm)
{
    if (!finite(length_mm) || length_mm < 0.0)
        throw InvalidArgument(_u8L("ZAA path contains a non-finite XY segment"));
    if (length_mm == 0.0)
        return 0;

    const double rounded = std::ceil(length_mm / max_spacing_mm);
    // The conservative >= guard avoids a floating-to-integer conversion at
    // SIZE_MAX, whose double representation may round up by one.
    if (!finite(rounded) || rounded < 1.0 ||
        rounded >= static_cast<double>(std::numeric_limits<size_t>::max())) {
        throw OutOfRange(_u8L("ZAA segment sample count exceeds size_t"));
    }
    return static_cast<size_t>(rounded);
}

size_t path_sample_count_for_points(const Points &points, double max_spacing_mm)
{
    if (points.size() < 2)
        throw InvalidArgument(_u8L("ZAA path planning requires at least two XY points"));
    if (!finite(max_spacing_mm) || max_spacing_mm <= 0.0)
        throw InvalidArgument(_u8L("ZAA path planning requires positive finite sample spacing"));

    size_t division_count = 0;
    double path_distance_mm = 0.0;
    for (size_t i = 0; i + 1 < points.size(); ++i) {
        const double length_mm = point_distance_mm(points[i], points[i + 1]);
        const size_t divisions = segment_division_count(length_mm, max_spacing_mm);
        division_count = checked_size_add(
            division_count, divisions, L("ZAA path sample count exceeds size_t"));
        path_distance_mm += length_mm;
        if (!finite(path_distance_mm))
            throw OutOfRange(_u8L("ZAA path distance exceeds finite double range"));
    }

    if (division_count == 0)
        throw InvalidArgument(_u8L("ZAA path planning requires a non-zero XY segment"));
    return checked_size_add(division_count, 1, L("ZAA path sample count exceeds size_t"));
}

void fill_path_samples(
    const Points &points,
    double max_spacing_mm,
    ZaaSample *output,
    size_t expected_count)
{
    if (output == nullptr || expected_count < 2)
        throw LogicError(_u8L("ZAA sample storage invariant failed"));

    size_t written = 0;
    size_t last_non_zero_segment = size_t(-1);
    double path_distance_mm = 0.0;
    double last_non_zero_distance_mm = 0.0;
    for (size_t i = 0; i + 1 < points.size(); ++i) {
        const double length_mm = point_distance_mm(points[i], points[i + 1]);
        const size_t divisions = segment_division_count(length_mm, max_spacing_mm);
        if (divisions != 0) {
            if (written >= expected_count || divisions > expected_count - written - 1)
                throw LogicError(_u8L("ZAA sample count changed while filling a path"));
            for (size_t step = 0; step < divisions; ++step) {
                const double ratio = double(step) / double(divisions);
                output[written++] = {
                    interpolate_scaled(points[i], points[i + 1], ratio),
                    {i, ratio, path_distance_mm + ratio * length_mm}
                };
            }
            last_non_zero_segment = i;
        }

        path_distance_mm += length_mm;
        if (!finite(path_distance_mm))
            throw OutOfRange(_u8L("ZAA path distance exceeds finite double range"));
        if (divisions != 0)
            last_non_zero_distance_mm = path_distance_mm;
    }

    if (last_non_zero_segment == size_t(-1) || written + 1 != expected_count)
        throw LogicError(_u8L("ZAA sample count changed while filling a path"));
    output[written] = {
        points[last_non_zero_segment + 1],
        {last_non_zero_segment, 1.0, last_non_zero_distance_mm}
    };

    if (output[0].xy != points.front() || output[expected_count - 1].xy != points.back())
        throw LogicError(_u8L("ZAA sampling changed a path endpoint"));
}

std::vector<ZaaSample> sample_path(const Points &points, double max_spacing_mm)
{
    const size_t sample_count = path_sample_count_for_points(points, max_spacing_mm);
    std::vector<ZaaSample> samples;
    if (sample_count > samples.max_size())
        throw OutOfRange(_u8L("ZAA path samples exceed vector max_size"));
    samples.resize(sample_count);
    fill_path_samples(points, max_spacing_mm, samples.data(), samples.size());
    return samples;
}

// Shared by the conservative prefilter and the full-mesh ray resolver.
std::pair<double, double> guarded_ray_interval(const ZaaLayerGeometry &layer)
{
    const double nominal_z = layer.nominal_z_mm();
    const double min_delta = layer.min_delta_mm();
    const double max_delta = layer.max_delta_mm();
    const double guarded_lower_z = std::nextafter(
        std::min(layer.query_lower_z_mm - ZAA_MAX_BOUNDARY_PROJECTION_MM,
                 nominal_z + min_delta - ZAA_MAX_BOUNDARY_PROJECTION_MM),
        -std::numeric_limits<double>::infinity());
    const double guarded_upper_z = std::nextafter(
        std::max(layer.query_upper_z_mm + ZAA_MAX_BOUNDARY_PROJECTION_MM,
                 nominal_z + max_delta + ZAA_MAX_BOUNDARY_PROJECTION_MM),
        std::numeric_limits<double>::infinity());
    if (!finite(guarded_lower_z) || !finite(guarded_upper_z) ||
        guarded_upper_z <= guarded_lower_z)
        throw LogicError(_u8L("ZAA guarded ray interval invariant failed"));
    return {guarded_lower_z, guarded_upper_z};
}

// Existence queries only: never raycast this filtered index. Removing a steep
// or down-facing occluder from the actual ray mesh would expose a farther hit.
// The fourth bounding-box coordinate is cos(theta), allowing a different
// slope limit for each layer/path without rebuilding or scanning every face.
class WallSlopeIndex {
    using Tree = AABBTreeIndirect::Tree<4, double>;
    using Box = Tree::BoundingBox;
    using Vector = Tree::VectorType;
    struct FaceBox {
        size_t face;
        Box bounds;
        size_t idx() const { return face; }
        Vector centroid() const { return bounds.center(); }
        const Box &bbox() const { return bounds; }
    };
    Tree m_tree;
    bool m_trusted{true};

    bool intersects(const Box &box) const
    {
        bool found = false;
        AABBTreeIndirect::traverse(m_tree, AABBTreeIndirect::intersecting(box),
            [&](const Tree::Node &) { found = true; return false; });
        return found;
    }

public:
    explicit WallSlopeIndex(const AABBMesh &mesh, const ZaaPathPlanBatchCallbacks *callbacks = nullptr)
    {
        std::vector<FaceBox> boxes;
        for (size_t i = 0; i < mesh.indices().size(); ++i) {
            if (i % kCancellationCheckStride == 0 && callbacks && callbacks->throw_if_canceled)
                callbacks->throw_if_canceled();
            const Vec3d normal = mesh.normal_by_face_id(int(i));
            if (!normal.allFinite()) {
                m_trusted = false;
                continue;
            }
            if (normal.z() <= ZAA_FACING_COS_EPSILON)
                continue;
            Box box;
            for (int vertex : mesh.indices(i)) {
                const Vec3d p = mesh.vertices(size_t(vertex)).cast<double>();
                if (!p.allFinite()) {
                    m_trusted = false;
                    continue;
                }
                box.extend(Vector(p.x(), p.y(), p.z(), normal.z()));
            }
            // Account for float mesh/ray arithmetic as well as XY quantization.
            const double magnitude = std::max(box.min().head<3>().cwiseAbs().maxCoeff(),
                                              box.max().head<3>().cwiseAbs().maxCoeff());
            const double padding = std::max(kNumericalEpsilon,
                8.0 * double(std::numeric_limits<float>::epsilon()) * magnitude);
            box.min().head<3>().array() -= padding;
            box.max().head<3>().array() += padding;
            box.min()[3] -= 1e-12;
            box.max()[3] += 1e-12;
            boxes.push_back({i, box});
        }
        m_tree.build(std::move(boxes));
    }

    bool may_affect(const ZaaPathPlanRequest &request, const ZaaPathPlanBatchCallbacks *callbacks = nullptr) const
    {
        if (!m_trusted)
            return true;
        const auto &path = *request.source_path;
        const auto interval = guarded_ray_interval(*request.layer_geometry);
        const ZaaWallSlopePolicy policy(*request.layer_geometry, double(path.width));
        const double cos_min = policy.min_normal_z() - 1e-12;
        const auto &points = path.polyline.points;
        // Test segment boxes, not just a closed wall's enclosing box, which
        // would spuriously include the entire interior of the object.
        for (size_t i = 1; i < points.size(); ++i) {
            if (i % kCancellationCheckStride == 0 && callbacks && callbacks->throw_if_canceled)
                callbacks->throw_if_canceled();
            const Vec2d a = point_mm(points[i - 1]);
            const Vec2d b = point_mm(points[i]);
            const Box box(
                Vector(std::min(a.x(), b.x()) - kNumericalEpsilon,
                       std::min(a.y(), b.y()) - kNumericalEpsilon, interval.first, cos_min),
                Vector(std::max(a.x(), b.x()) + kNumericalEpsilon,
                       std::max(a.y(), b.y()) + kNumericalEpsilon, interval.second, 1.0 + 1e-12));
            if (intersects(box))
                return true;
        }
        return false;
    }
};

struct ResolvedPoint {
    ZaaPointOutcome outcome;
    std::optional<double> slope_degrees;
    // Keep the steepest equivalent hit for wall gating: averaged normals can
    // make a sharp ridge appear shallow when both sides are actually steep.
    std::optional<double> steepest_surface_tangent;
};

struct HitCandidate {
    double delta_mm{0.0};
    Vec3d normal{Vec3d::Zero()};
    int face_id{-1};
};

ResolvedPoint resolve_point(
    const AABBMesh &mesh,
    const Vec2d &xy,
    const ZaaLayerGeometry &layer)
{
    const double nominal_z = layer.nominal_z_mm();
    const double min_delta = layer.min_delta_mm();
    const double max_delta = layer.max_delta_mm();
    const Vec3d nominal_point(xy.x(), xy.y(), nominal_z);

    // Query exactly through the boundary-projection band. A valid hit in this
    // band is later projected onto the closed printable interval; a farther hit
    // is intentionally not a candidate for this layer.
    const auto interval = guarded_ray_interval(layer);
    const double guarded_lower_z = interval.first;
    const double guarded_upper_z = interval.second;
    // Each of the two ray directions contributes at most one closest hit.
    // Fixed storage avoids a heap allocation for every sampled point.
    std::array<HitCandidate, 2> candidates;
    size_t candidate_count = 0;
    auto collect = [&](const Vec3d &direction) {
        Vec3d ray_source = nominal_point;
        ray_source.z() = std::nextafter(
            nominal_z,
            direction.z() > 0.0 ? -std::numeric_limits<double>::infinity()
                                : std::numeric_limits<double>::infinity());
        const double max_t = direction.z() > 0.0
            ? guarded_upper_z - ray_source.z()
            : ray_source.z() - guarded_lower_z;
        if (!finite(max_t) || max_t < 0.0)
            throw LogicError(_u8L("ZAA guarded ray distance invariant failed"));

        const AABBMesh::hit_result hit =
            mesh.query_ray_hit(ray_source, direction, 0.0, max_t);
        if (!hit.is_hit())
            return;
        const Vec3d position = hit.position();
        if (!position.allFinite() || !hit.normal().allFinite())
            return;

        double candidate_z = position.z();
        if (std::abs(candidate_z - layer.query_lower_z_mm) <= kNumericalEpsilon)
            candidate_z = layer.query_lower_z_mm;
        else if (std::abs(candidate_z - layer.query_upper_z_mm) <= kNumericalEpsilon)
            candidate_z = layer.query_upper_z_mm;

        // Shared planes belong to the lower interval: (lower, upper].
        // Only the first interval owns the global lower boundary.
        if (layer.layer_index > 0 && candidate_z == layer.query_lower_z_mm)
            return;

        const double delta = candidate_z - nominal_z;
        if (delta < min_delta - ZAA_MAX_BOUNDARY_PROJECTION_MM || delta > max_delta + ZAA_MAX_BOUNDARY_PROJECTION_MM)
            return;
        if (candidate_count >= candidates.size())
            throw LogicError(_u8L("ZAA ray candidate storage invariant failed"));
        candidates[candidate_count++] = {delta, hit.normal(), hit.face()};
    };

    collect(Vec3d::UnitZ());
    collect(-Vec3d::UnitZ());
    if (candidate_count == 0)
        return {ZaaPointOutcome::planar(ZaaPlanarReason::NoHit), std::nullopt};

    std::sort(candidates.begin(), candidates.begin() + candidate_count, [](const HitCandidate &lhs, const HitCandidate &rhs) {
        const double lhs_distance = std::abs(lhs.delta_mm);
        const double rhs_distance = std::abs(rhs.delta_mm);
        if (lhs_distance != rhs_distance)
            return lhs_distance < rhs_distance;
        if (lhs.delta_mm != rhs.delta_mm)
            return lhs.delta_mm < rhs.delta_mm;
        return lhs.face_id < rhs.face_id;
    });

    const double selected_delta = candidates.front().delta_mm;
    size_t equivalent_end = 1;
    while (equivalent_end < candidate_count &&
           std::abs(candidates[equivalent_end].delta_mm - selected_delta) <= ZAA_HIT_EQUIVALENCE_MM)
        ++equivalent_end;

    if (equivalent_end < candidate_count &&
        std::abs(std::abs(candidates[equivalent_end].delta_mm) - std::abs(selected_delta)) <= ZAA_DISTANCE_TIE_MM)
        return {ZaaPointOutcome::planar(ZaaPlanarReason::AmbiguousHit), std::nullopt};

    bool has_up = false;
    bool has_down = false;
    bool has_grazing = false;
    Vec3d representative = Vec3d::Zero();
    double steepest_tangent = 0.0;
    for (size_t i = 0; i < equivalent_end; ++i) {
        const Vec3d normal = candidates[i].normal.normalized();
        if (!normal.allFinite())
            return {ZaaPointOutcome::planar(ZaaPlanarReason::AmbiguousHit), std::nullopt};
        const double normal_z = std::abs(normal.z());
        const double tangent = normal_z > 0.0
            ? std::hypot(normal.x(), normal.y()) / normal_z : std::numeric_limits<double>::infinity();
        steepest_tangent = std::max(steepest_tangent, tangent);
        representative += normal;
        if (normal.z() > ZAA_FACING_COS_EPSILON)
            has_up = true;
        else if (normal.z() < -ZAA_FACING_COS_EPSILON)
            has_down = true;
        else
            has_grazing = true;
    }

    const unsigned int classifications = unsigned(has_up) + unsigned(has_down) + unsigned(has_grazing);
    if (classifications != 1 || representative.norm() == 0.0)
        return {ZaaPointOutcome::planar(ZaaPlanarReason::AmbiguousHit), std::nullopt};

    representative.normalize();
    const double slope = std::acos(std::clamp(std::abs(representative.z()), 0.0, 1.0)) * 180.0 / kPi;

    if (has_down)
        return {ZaaPointOutcome::planar(ZaaPlanarReason::WrongFacing), slope};
    if (has_grazing)
        return {ZaaPointOutcome::planar(ZaaPlanarReason::GrazingSurface), slope};

    if (selected_delta < min_delta || selected_delta > max_delta) {
        // Candidate collection already bounded the distance to the interval by
        // ZAA_MAX_BOUNDARY_PROJECTION_MM. Projecting here keeps a valid surface
        // continuous at the legal print boundary instead of reverting this point
        // alone to nominal planar Z.
        return {ZaaPointOutcome::boundary_projected_to(
            std::clamp(selected_delta, min_delta, max_delta)), slope, steepest_tangent};
    }
    return {ZaaPointOutcome::displaced_by(selected_delta), slope, steepest_tangent};
}

size_t incompatibility_index(ZaaIncompatibilityReason reason)
{
    return static_cast<size_t>(reason);
}

struct PointDebugRecord {
    size_t point_index{0};
    ZaaSampleOrigin origin;
    ZaaPointOutcome outcome{ZaaPointOutcome::planar(ZaaPlanarReason::NoHit)};
    // Reuse the old diagnostic-angle slot; no additional per-sample storage.
    // Finite d is retained before gating, including below-threshold samples.
    std::optional<double> wall_step_width_mm;
    Point xy;
    Vec3d lifted_mm{Vec3d::Zero()};
};

bool same_profile_class(const ZaaPointOutcome &lhs, const ZaaPointOutcome &rhs)
{
    const ZaaDisplaced *lhs_displaced = lhs.displaced();
    const ZaaDisplaced *rhs_displaced = rhs.displaced();
    if ((lhs_displaced != nullptr) != (rhs_displaced != nullptr))
        return false;
    if (lhs_displaced != nullptr)
        return true;

    const ZaaPlanar *lhs_planar = lhs.planar_result();
    const ZaaPlanar *rhs_planar = rhs.planar_result();
    return lhs_planar != nullptr && rhs_planar != nullptr &&
           lhs_planar->reason == rhs_planar->reason;
}

bool same_profile_class(const PointDebugRecord &lhs, const PointDebugRecord &rhs)
{
    return same_profile_class(lhs.outcome, rhs.outcome);
}

bool same_emitted_xyz(const Vec3d &lhs, const Vec3d &rhs)
{
    return lhs.x() == rhs.x() && lhs.y() == rhs.y() && lhs.z() == rhs.z();
}

bool is_strictly_collinear_3d(
    const Vec3d &a,
    const Vec3d &b,
    const Vec3d &c)
{
    const Vec3d ab = b - a;
    const Vec3d ac = c - a;
    const double ac_length_sq = ac.squaredNorm();
    if (!finite(ac_length_sq) || ac_length_sq <= kNumericalEpsilon * kNumericalEpsilon)
        return false;

    const Vec3d cross = ab.cross(ac);
    const double distance_sq = cross.squaredNorm() / ac_length_sq;
    if (!finite(distance_sq) || distance_sq > kNumericalEpsilon * kNumericalEpsilon)
        return false;

    const double projection = ab.dot(ac) / ac_length_sq;
    return projection > 0.0 && projection < 1.0;
}

bool can_compress_middle_point(
    const PointDebugRecord &lhs,
    const PointDebugRecord &middle,
    const PointDebugRecord &rhs)
{
    if (!same_profile_class(lhs, middle) || !same_profile_class(middle, rhs))
        return false;
    return is_strictly_collinear_3d(lhs.lifted_mm, middle.lifted_mm, rhs.lifted_mm);
}

std::vector<size_t> compress_profile_points(
    const std::vector<PointDebugRecord> &records,
    size_t begin,
    size_t end)
{
    if (begin > end || end > records.size())
        throw LogicError(_u8L("ZAA profile compression range invariant failed"));

    std::vector<size_t> retained;
    retained.reserve(end - begin);
    for (size_t i = begin; i < end; ++i) {
        retained.push_back(i);
        while (retained.size() >= 3) {
            const size_t lhs_index = retained[retained.size() - 3];
            const size_t middle_index = retained[retained.size() - 2];
            const size_t rhs_index = retained[retained.size() - 1];
            if (!can_compress_middle_point(
                    records[lhs_index], records[middle_index], records[rhs_index]))
                break;
            retained[retained.size() - 2] = retained.back();
            retained.pop_back();
        }
    }
    return retained;
}

const ExtrusionPath &validate_path_plan_request(const ZaaPathPlanRequest &request)
{
    if (request.source_path == nullptr || request.layer_geometry == nullptr || request.query_mesh == nullptr)
        throw InvalidArgument(_u8L("ZAA path planning requires a path, layer geometry, and query mesh"));

    const ExtrusionPath &path = *request.source_path;
    const ZaaLayerGeometry &layer = *request.layer_geometry;
    if (!layer.uses_zaa_offset_plane())
        throw LogicError(_u8L("ZAA path planning requires an offset-plane layer"));
    if (!zaa_role_is_eligible(path.role()) || path.zaa_path_policy() != ZaaPathPolicy::ZaaLinearCandidate ||
        !path.polyline.fitting_result.empty() || dynamic_cast<const ExtrusionPathSloped *>(&path) != nullptr) {
        throw LogicError(_u8L("ZAA path planning requires an eligible linear candidate path without arc fitting"));
    }
    if (!finite(request.layer_print_z_mm) || !finite(request.max_sample_spacing_mm) ||
        request.max_sample_spacing_mm <= 0.0) {
        throw InvalidArgument(_u8L("ZAA path planning requires finite layer Z and positive sample spacing"));
    }

    if (!finite(request.minimize_perimeter_height_angle_degrees) ||
        request.minimize_perimeter_height_angle_degrees < 0.0 ||
        request.minimize_perimeter_height_angle_degrees > 90.0) {
        throw InvalidArgument(_u8L("ZAA perimeter height angle must be finite and within [0, 90] degrees"));
    }
    if (request.minimize_perimeter_height_angle_degrees > 0.0 && is_perimeter(path.role()) &&
        (!finite(double(path.width)) || path.width <= 0.0f)) {
        throw InvalidArgument(_u8L("ZAA perimeter height minimization requires a finite positive path width"));
    }
    if (is_perimeter(path.role()))
        (void)ZaaWallSlopePolicy(layer, double(path.width));
    if (path.polyline.points.size() < 2)
        throw InvalidArgument(_u8L("ZAA path planning requires at least two XY points"));
    return path;
}

void apply_perimeter_height_minimization(
    const ZaaPathPlanRequest &request,
    const ExtrusionPath &path,
    const ZaaLayerGeometry &layer,
    ResolvedPoint &resolved)
{
    const double threshold_degrees = request.minimize_perimeter_height_angle_degrees;
    if (threshold_degrees <= 0.0 || !is_perimeter(path.role()))
        return;

    const ZaaDisplaced *displaced = resolved.outcome.displaced();
    if (displaced == nullptr || !resolved.slope_degrees ||
        *resolved.slope_degrees <= threshold_degrees)
        return;

    const double min_delta = layer.min_delta_mm();
    if (displaced->delta_mm <= min_delta)
        return;

    const double slope_radians = *resolved.slope_degrees * kPi / 180.0;
    const double adjustment = -0.5 * double(path.width) * std::sin(slope_radians);
    const double requested_delta = displaced->delta_mm + adjustment;
    if (!finite(adjustment) || adjustment >= 0.0 || !finite(requested_delta))
        throw LogicError(_u8L("ZAA perimeter height minimization produced an invalid adjustment"));

    if (requested_delta <= min_delta)
        resolved.outcome = ZaaPointOutcome::boundary_projected_to(min_delta);
    else
        resolved.outcome = ZaaPointOutcome::displaced_by(requested_delta);
}

std::optional<double> apply_wall_slope_policy(const ZaaPathPlanRequest &request, ResolvedPoint &resolved)
{
    if (!is_perimeter(request.source_path->role()))
        return std::nullopt;
    const ZaaDisplaced *displaced = resolved.outcome.displaced();
    if (displaced == nullptr)
        return std::nullopt;
    const ZaaWallSlopePolicy policy(*request.layer_geometry, double(request.source_path->width));
    const auto step = resolved.steepest_surface_tangent
        ? policy.step_width_mm(*resolved.steepest_surface_tangent) : std::nullopt;
    const bool flat_cap = resolved.steepest_surface_tangent && *resolved.steepest_surface_tangent == 0.0;
    const double weight = flat_cap ? 1.0 : (step ? policy.weight_for_step_width(*step) : 0.0);
    if (weight == 0.0) {
        resolved.outcome = ZaaPointOutcome::planar(ZaaPlanarReason::OutOfRange);
        return step;
    }
    // The step score controls usefulness, not collision safety or displacement
    // direction. Retain the original ray resolver's bounds and both signs.
    const double delta = displaced->delta_mm * weight;
    if (delta != displaced->delta_mm)
        resolved.outcome = ZaaPointOutcome::displaced_by(delta);
    return step;
}

PointDebugRecord resolve_sample_record(
    const ZaaPathPlanRequest &request,
    const ZaaSample &sample,
    size_t path_point_index)
{
    const ZaaLayerGeometry &layer = *request.layer_geometry;
    const Vec2d lifted_xy = point_mm(sample.xy);
    ResolvedPoint resolved = resolve_point(*request.query_mesh, lifted_xy, layer);
    apply_perimeter_height_minimization(request, *request.source_path, layer, resolved);
    const auto wall_step = apply_wall_slope_policy(request, resolved);
    if (const ZaaDisplaced *displaced = resolved.outcome.displaced()) {
        if (!finite(displaced->delta_mm) || displaced->delta_mm < layer.min_delta_mm() - kNumericalEpsilon ||
            displaced->delta_mm > layer.max_delta_mm() + kNumericalEpsilon) {
            throw LogicError(_u8L("ZAA resolver produced a displacement outside the planned layer interval"));
        }
    }

    return {
        path_point_index,
        sample.origin,
        resolved.outcome,
        wall_step,
        sample.xy,
        resolved.outcome.displaced() != nullptr
            ? Vec3d(lifted_xy.x(), lifted_xy.y(), resolved.outcome.displaced()->delta_mm)
            : Vec3d(lifted_xy.x(), lifted_xy.y(), 0.0)
    };
}

ZaaPathPlanResult finalize_path_plan(
    const ZaaPathPlanRequest &request,
    const std::vector<PointDebugRecord> &records,
    size_t begin,
    size_t end)
{
    if (begin > end || end > records.size() || end - begin < 2)
        throw LogicError(_u8L("ZAA path finalization requires at least two resolved points"));

    const std::vector<size_t> retained_indices = compress_profile_points(records, begin, end);
    if (retained_indices.size() < 2)
        throw LogicError(_u8L("ZAA profile compression produced fewer than two points"));

    Polyline3 planned;
    planned.points.reserve(retained_indices.size());
    const coord_t nominal_z = scale_(request.layer_print_z_mm);
    bool has_effective_z_displacement = false;
    for (size_t retained_index : retained_indices) {
        const PointDebugRecord &record = records[retained_index];
        const ZaaDisplaced *displaced = record.outcome.displaced();
        const double delta_mm = displaced == nullptr ? 0.0 : displaced->delta_mm;
        const coord_t z = scale_(request.layer_print_z_mm + delta_mm);
        planned.points.emplace_back(record.xy.x(), record.xy.y(), z);
        has_effective_z_displacement = has_effective_z_displacement || z != nominal_z;
    }

    ZaaPathPlanResult result;
    if (has_effective_z_displacement) {
        result.path3 = std::make_unique<ExtrusionPath3>(std::move(planned));
        result.prepared_polyline = std::make_unique<Polyline>(result.path3->polyline3().to_polyline());
    }
    return result;
}

void throw_if_batch_canceled(const ZaaPathPlanBatchCallbacks *callbacks)
{
    if (callbacks != nullptr && callbacks->throw_if_canceled)
        callbacks->throw_if_canceled();
}

void report_resolved_samples(const ZaaPathPlanBatchCallbacks *callbacks, size_t sample_count)
{
    if (callbacks != nullptr && callbacks->samples_resolved)
        callbacks->samples_resolved(sample_count);
}

} // namespace

double ZaaLayerGeometry::nominal_z_mm(double object_print_z_min) const
{
    return query_upper_z_mm - (query_lower_z_mm - object_z_lower_mm) + object_print_z_min;
}

double ZaaLayerGeometry::min_delta_mm() const
{
    const double s = query_lower_z_mm - object_z_lower_mm;
    return s - (query_upper_z_mm - query_lower_z_mm);
}

double ZaaLayerGeometry::max_delta_mm() const
{
    return query_lower_z_mm - object_z_lower_mm;
}

ZaaPathPlanResult::ZaaPathPlanResult() = default;
ZaaPathPlanResult::~ZaaPathPlanResult() = default;
ZaaPathPlanResult::ZaaPathPlanResult(ZaaPathPlanResult &&) noexcept = default;
ZaaPathPlanResult &ZaaPathPlanResult::operator=(ZaaPathPlanResult &&) noexcept = default;

void ZaaPathPlanResult::exchange_with(ExtrusionPath &target) noexcept
{
    if (prepared_polyline != nullptr) {
        target.polyline.points.swap(prepared_polyline->points);
        target.polyline.fitting_result.swap(prepared_polyline->fitting_result);
    }
    target.m_path3.swap(path3);
}

size_t zaa_path_sample_count(const ExtrusionPath &path, double max_sample_spacing_mm)
{
    return path_sample_count_for_points(path.polyline.points, max_sample_spacing_mm);
}

ZaaPathPlanResult build_zaa_path_plan(const ZaaPathPlanRequest &request)
{
    const ExtrusionPath &path = validate_path_plan_request(request);
    if (is_perimeter(path.role())) {
        // Validate even a rejected path before bypassing sample allocation.
        (void)path_sample_count_for_points(path.polyline.points, request.max_sample_spacing_mm);
        if (!WallSlopeIndex(*request.query_mesh).may_affect(request))
            return {};
    }
    const std::vector<ZaaSample> samples = sample_path(
        path.polyline.points, request.max_sample_spacing_mm);

    std::vector<PointDebugRecord> records;
    records.reserve(samples.size());
    for (size_t sample_index = 0; sample_index < samples.size(); ++sample_index)
        records.push_back(resolve_sample_record(request, samples[sample_index], sample_index));
    return finalize_path_plan(request, records, 0, records.size());
}

std::vector<ZaaPathPlanResult> build_zaa_path_plans(
    const std::vector<ZaaPathPlanRequest> &requests,
    const ZaaPathPlanBatchCallbacks *callbacks)
{
    throw_if_batch_canceled(callbacks);

    std::vector<size_t> path_offsets;
    const size_t offset_count = checked_size_add(
        requests.size(), 1, L("ZAA path offset count exceeds size_t"));
    if (offset_count > path_offsets.max_size())
        throw OutOfRange(_u8L("ZAA path offsets exceed vector max_size"));
    path_offsets.resize(offset_count, 0);
    std::unordered_map<const AABBMesh *, std::unique_ptr<WallSlopeIndex>> slope_indices;
    size_t prefiltered_paths = 0;
    size_t prefiltered_samples = 0;

    for (size_t path_index = 0; path_index < requests.size(); ++path_index) {
        throw_if_batch_canceled(callbacks);
        const ExtrusionPath &path = validate_path_plan_request(requests[path_index]);
        size_t count = path_sample_count_for_points(
            path.polyline.points, requests[path_index].max_sample_spacing_mm);
        if (is_perimeter(path.role())) {
            const auto &request = requests[path_index];
            auto &index = slope_indices[request.query_mesh];
            if (!index)
                index = std::make_unique<WallSlopeIndex>(*request.query_mesh, callbacks);
            if (!index->may_affect(request, callbacks)) {
                ++prefiltered_paths;
                prefiltered_samples = checked_size_add(prefiltered_samples, count,
                    L("ZAA prefiltered sample count exceeds size_t"));
                count = 0;
            }
        }
        path_offsets[path_index + 1] = checked_size_add(
            path_offsets[path_index], count, L("ZAA batch sample count exceeds size_t"));
    }
    throw_if_batch_canceled(callbacks);

    // Prefiltered paths are completed work, so progress still reaches its
    // original sample total even when no samples/rays need to be materialized.
    report_resolved_samples(callbacks, prefiltered_samples);
    const size_t total_samples = path_offsets.back();
    std::vector<ZaaSample> samples;
    std::vector<PointDebugRecord> records;
    std::vector<ZaaPathPlanResult> results;
    if (total_samples > samples.max_size() || total_samples > records.max_size())
        throw OutOfRange(_u8L("ZAA batch samples exceed vector max_size"));
    if (requests.size() > results.max_size())
        throw OutOfRange(_u8L("ZAA batch results exceed vector max_size"));

    size_t base_scratch_bytes = checked_size_multiply(
        total_samples, sizeof(ZaaSample), L("ZAA sample scratch byte count exceeds size_t"));
    base_scratch_bytes = checked_size_add(
        base_scratch_bytes,
        checked_size_multiply(
            total_samples, sizeof(PointDebugRecord), L("ZAA record scratch byte count exceeds size_t")),
        "ZAA base scratch byte count exceeds size_t");
    base_scratch_bytes = checked_size_add(
        base_scratch_bytes,
        checked_size_multiply(
            offset_count, sizeof(size_t), L("ZAA offset scratch byte count exceeds size_t")),
        "ZAA base scratch byte count exceeds size_t");

    BOOST_LOG_TRIVIAL(info)
        << "ZAA flattened batch paths=" << requests.size()
        << " samples=" << total_samples
        << " wall_step_min_mm=" << ZaaWallSlopePolicy::min_step_width_mm
        << " wall_step_full_mm=" << ZaaWallSlopePolicy::full_step_width_mm
        << " wall_slope_prefiltered_paths=" << prefiltered_paths
        << " wall_slope_prefiltered_samples=" << prefiltered_samples
        << " base_scratch_bytes=" << base_scratch_bytes
        << " base_scratch_mib=" << std::fixed << std::setprecision(2)
        << (double(base_scratch_bytes) / (1024.0 * 1024.0));

    samples.resize(total_samples);
    records.resize(total_samples);

    if (!requests.empty()) {
        tbb::parallel_for(
            tbb::blocked_range<size_t>(0, requests.size()),
            [&](const tbb::blocked_range<size_t> &range) {
                throw_if_batch_canceled(callbacks);
                for (size_t path_index = range.begin(); path_index != range.end(); ++path_index) {
                    throw_if_batch_canceled(callbacks);
                    const ExtrusionPath &path = *requests[path_index].source_path;
                    const size_t begin = path_offsets[path_index];
                    const size_t end = path_offsets[path_index + 1];
                    if (begin == end)
                        continue;
                    fill_path_samples(
                        path.polyline.points,
                        requests[path_index].max_sample_spacing_mm,
                        samples.data() + begin,
                        end - begin);
                }
            });
    }
    throw_if_batch_canceled(callbacks);

    if (total_samples != 0) {
        tbb::parallel_for(
            tbb::blocked_range<size_t>(0, total_samples, kRaycastGrainSize),
            [&](const tbb::blocked_range<size_t> &range) {
                throw_if_batch_canceled(callbacks);
                const auto offset_it = std::upper_bound(
                    path_offsets.begin(), path_offsets.end(), range.begin());
                if (offset_it == path_offsets.begin() || offset_it == path_offsets.end())
                    throw LogicError(_u8L("ZAA flattened path lookup invariant failed"));
                size_t path_index = size_t(std::distance(path_offsets.begin(), offset_it) - 1);

                for (size_t global_index = range.begin(); global_index != range.end(); ++global_index) {
                    if (global_index != range.begin() &&
                        (global_index - range.begin()) % kCancellationCheckStride == 0) {
                        throw_if_batch_canceled(callbacks);
                    }
                    while (global_index >= path_offsets[path_index + 1]) {
                        ++path_index;
                        if (path_index >= requests.size())
                            throw LogicError(_u8L("ZAA flattened path lookup crossed the final path"));
                    }

                    const size_t path_point_index = global_index - path_offsets[path_index];
                    records[global_index] = resolve_sample_record(
                        requests[path_index], samples[global_index], path_point_index);
                }
                report_resolved_samples(callbacks, range.size());
            });
    }
    throw_if_batch_canceled(callbacks);

    // Only finite estimates from valid, ray-resolved wall hits are summarized.
    // Prefiltered paths and flat caps have no measured d in these statistics.
    // d/w is diagnostic only and uses the outer-wall width when available.
    size_t step_samples = 0, step_rejected = 0, step_faded = 0;
    double step_min = std::numeric_limits<double>::infinity(), step_max = 0.0;
    double ratio_min = std::numeric_limits<double>::infinity(), ratio_max = 0.0;
    for (size_t i = 0; i < requests.size(); ++i) {
        throw_if_batch_canceled(callbacks);
        const auto &request = requests[i];
        if (path_offsets[i] == path_offsets[i + 1] || !is_perimeter(request.source_path->role()))
            continue;
        const ZaaWallSlopePolicy policy(*request.layer_geometry, double(request.source_path->width));
        for (size_t j = path_offsets[i]; j < path_offsets[i + 1]; ++j) {
            if ((j - path_offsets[i]) % kCancellationCheckStride == 0)
                throw_if_batch_canceled(callbacks);
            if (const auto d = records[j].wall_step_width_mm) {
                ++step_samples;
                step_rejected += *d <= ZaaWallSlopePolicy::min_step_width_mm;
                step_faded += *d > ZaaWallSlopePolicy::min_step_width_mm && *d < ZaaWallSlopePolicy::full_step_width_mm;
                step_min = std::min(step_min, *d);
                step_max = std::max(step_max, *d);
                const double ratio = *d / policy.reference_width_mm();
                ratio_min = std::min(ratio_min, ratio);
                ratio_max = std::max(ratio_max, ratio);
            }
        }
    }

    // ------ Estimatation of d/w: ------
    // if (step_samples > 0) {
    //     BOOST_LOG_TRIVIAL(info) << "ZAA wall step diagnostics samples=" << step_samples
    //         << " rejected_samples=" << step_rejected << " faded_samples=" << step_faded
    //         << " d_min_mm=" << step_min << " d_max_mm=" << step_max
    //         << " d_over_w_min=" << ratio_min << " d_over_w_max=" << ratio_max;
    // }

    results.resize(requests.size());
    if (!requests.empty()) {
        tbb::parallel_for(
            tbb::blocked_range<size_t>(0, requests.size()),
            [&](const tbb::blocked_range<size_t> &range) {
                throw_if_batch_canceled(callbacks);
                for (size_t path_index = range.begin(); path_index != range.end(); ++path_index) {
                    throw_if_batch_canceled(callbacks);
                    if (path_offsets[path_index] == path_offsets[path_index + 1])
                        continue;
                    results[path_index] = finalize_path_plan(
                        requests[path_index],
                        records,
                        path_offsets[path_index],
                        path_offsets[path_index + 1]);
                }
            });
    }
    throw_if_batch_canceled(callbacks);
    return results;
}

const ZaaLayerGeometry *ZaaObjectGeometryPlan::layer(size_t layer_index) const
{
    if (layer_index < layers.size() && layers[layer_index].layer_index == layer_index)
        return &layers[layer_index];
    const auto it = std::find_if(layers.begin(), layers.end(), [layer_index](const ZaaLayerGeometry &layer) {
        return layer.layer_index == layer_index;
    });
    return it == layers.end() ? nullptr : &*it;
}

bool ZaaObjectGeometryPlan::uses_zaa_offset_plane(size_t layer_index) const
{
    const ZaaLayerGeometry *geometry = this->layer(layer_index);
    return geometry != nullptr && geometry->uses_zaa_offset_plane();
}

size_t ZaaObjectGeometryPlan::zaa_offset_layer_count() const
{
    size_t count = 0;
    for (const ZaaLayerGeometry &geometry : layers)
        count += geometry.uses_zaa_offset_plane() ? 1 : 0;
    return count;
}

const char *to_string(ZaaIncompatibilityReason reason)
{
    switch (reason) {
    case ZaaIncompatibilityReason::RaftEnabled: return "RaftEnabled";
    case ZaaIncompatibilityReason::ScarfEnabled: return "ScarfEnabled";
    case ZaaIncompatibilityReason::SpiralEnabled: return "SpiralEnabled";
    case ZaaIncompatibilityReason::IroningEnabled: return "IroningEnabled";
    case ZaaIncompatibilityReason::NegativeVolume: return "NegativeVolume";
    case ZaaIncompatibilityReason::GeometricModifier: return "GeometricModifier";
    case ZaaIncompatibilityReason::UnknownVolumeType: return "UnknownVolumeType";
    case ZaaIncompatibilityReason::MissingModelPart: return "MissingModelPart";
    case ZaaIncompatibilityReason::EmptyQueryMesh: return "EmptyQueryMesh";
    case ZaaIncompatibilityReason::NonFiniteQueryMesh: return "NonFiniteQueryMesh";
    case ZaaIncompatibilityReason::NonFiniteTransform: return "NonFiniteTransform";
    case ZaaIncompatibilityReason::SingularTransform: return "SingularTransform";
    case ZaaIncompatibilityReason::IllConditionedTransform: return "IllConditionedTransform";
    case ZaaIncompatibilityReason::SlicePlaneOffsetUnrepresentable: return "SlicePlaneOffsetUnrepresentable";
    }
    return "Unknown";
}

ZaaObjectSliceDecision ZaaObjectSliceDecision::disabled()
{
    return {};
}

ZaaObjectSliceDecision ZaaObjectSliceDecision::incompatible(ZaaIncompatibilityReason reason)
{
    ZaaObjectSliceDecision out;
    out.m_kind = ZaaObjectDecisionKind::Incompatible;
    out.m_reason = reason;
    return out;
}

ZaaObjectSliceDecision ZaaObjectSliceDecision::slice_plane_offset_limit_exceeded(
    ZaaSlicePlaneOffsetViolation violation)
{
    ZaaObjectSliceDecision out;
    out.m_kind = ZaaObjectDecisionKind::SlicePlaneOffsetLimitExceeded;
    out.m_slice_plane_offset_violation = violation;
    return out;
}

ZaaObjectSliceDecision ZaaObjectSliceDecision::supported(ZaaObjectGeometryPlan plan)
{
    ZaaObjectSliceDecision out;
    out.m_kind = ZaaObjectDecisionKind::Supported;
    out.m_plan = std::move(plan);
    return out;
}

ZaaSlicePlaneOffsetLimitError::ZaaSlicePlaneOffsetLimitError(
    std::string message,
    std::vector<ZaaSlicePlaneOffsetObjectViolation> violations)
    : SlicingError(message)
    , m_violations(std::move(violations))
{
}

ZaaSlicePlaneMode ZaaObjectSliceDecision::slice_plane_mode() const
{
    const ZaaObjectGeometryPlan *plan = this->geometry_plan();
    return plan == nullptr ? ZaaSlicePlaneMode::ConventionalMidplane : plan->mode;
}

const ZaaSlicePlaneOffsetViolation *ZaaObjectSliceDecision::slice_plane_offset_violation() const
{
    return m_slice_plane_offset_violation ? &*m_slice_plane_offset_violation : nullptr;
}

const ZaaObjectGeometryPlan *ZaaObjectSliceDecision::geometry_plan() const
{
    return m_plan ? &*m_plan : nullptr;
}

ZaaObjectSliceDecision make_zaa_object_slice_decision(
    const ZaaCapabilityFacts &facts,
    const std::vector<ZaaPreparedLayerInterval> &schedule)
{
    if (!facts.enabled)
        return ZaaObjectSliceDecision::disabled();

    const std::pair<bool, ZaaIncompatibilityReason> incompatibilities[] = {
        {facts.has_raft, ZaaIncompatibilityReason::RaftEnabled},
        {facts.has_scarf, ZaaIncompatibilityReason::ScarfEnabled},
        {facts.spiral_mode, ZaaIncompatibilityReason::SpiralEnabled},
        {facts.ironing_enabled, ZaaIncompatibilityReason::IroningEnabled},
        {facts.has_negative_volume, ZaaIncompatibilityReason::NegativeVolume},
        {facts.has_geometric_modifier, ZaaIncompatibilityReason::GeometricModifier},
        {facts.has_unknown_volume_type, ZaaIncompatibilityReason::UnknownVolumeType},
        {!facts.has_model_part, ZaaIncompatibilityReason::MissingModelPart},
        {facts.query_mesh_empty, ZaaIncompatibilityReason::EmptyQueryMesh},
        {!facts.query_mesh_finite, ZaaIncompatibilityReason::NonFiniteQueryMesh},
        {!facts.transforms_finite, ZaaIncompatibilityReason::NonFiniteTransform},
        {!facts.transforms_invertible, ZaaIncompatibilityReason::SingularTransform},
        {!facts.transforms_well_conditioned, ZaaIncompatibilityReason::IllConditionedTransform}
    };
    for (const auto &entry : incompatibilities)
        if (entry.first)
            return ZaaObjectSliceDecision::incompatible(entry.second);

    if (schedule.empty())
        throw LogicError(_u8L("ZAA prepared layer schedule is empty"));

    // The first prepared object layer remains conventional to protect the
    // build-plate interface. It has a midplane slice and never enters ray
    // planning, so only subsequent offset-plane layers constrain zaa_slice_plane_offset.
    double h_min = std::numeric_limits<double>::infinity();
    for (size_t i = 0; i < schedule.size(); ++i) {
        const ZaaPreparedLayerInterval &layer = schedule[i];
        if (layer.layer_index != i || !finite(layer.object_z_lower_mm) ||
            !finite(layer.object_z_upper_mm) || !finite(layer.outer_wall_width_mm) ||
            layer.object_z_upper_mm <= layer.object_z_lower_mm || layer.outer_wall_width_mm < 0.0 ||
            (i > 0 && layer.object_z_lower_mm != schedule[i - 1].object_z_upper_mm))
            throw LogicError(_u8L("ZAA prepared layer schedule invariant failed"));
        if (i != 0)
            h_min = std::min(h_min, layer.object_z_upper_mm - layer.object_z_lower_mm);
    }

    if (!finite(facts.requested_slice_plane_offset_mm) || facts.requested_slice_plane_offset_mm < 0.0)
        throw InvalidArgument(_u8L("ZAA slice-plane offset must be finite and non-negative"));

    ZaaObjectGeometryPlan plan;
    plan.layers.reserve(schedule.size());
    if (schedule.size() == 1) {
        // A one-layer object has no layer that may use ZAA. Keep it fully
        // conventional and deliberately ignore an explicit zaa_slice_plane_offset value.
        const ZaaPreparedLayerInterval &source = schedule.front();
        plan.mode = ZaaSlicePlaneMode::ConventionalMidplane;
        plan.layers.push_back({
            source.layer_index,
            canonical_zero(source.object_z_lower_mm),
            canonical_zero(source.object_z_upper_mm),
            canonical_zero(0.5 * (source.object_z_lower_mm + source.object_z_upper_mm)),
            canonical_zero(source.object_z_lower_mm),
            canonical_zero(source.object_z_upper_mm),
            canonical_zero(source.outer_wall_width_mm),
            ZaaSlicePlaneMode::ConventionalMidplane
        });
        return ZaaObjectSliceDecision::supported(std::move(plan));
    }

    // The common object-wide offset must leave the plane strictly below every
    // ZAA offset layer's physical top boundary. This preserves the
    // layer-ownership interval construction without assuming a permanently
    // low slice plane.
    const double maximum_offset_mm = canonical_zero(h_min - ZAA_SLICE_PLANE_TOP_CLEARANCE_MM);
    if (!finite(maximum_offset_mm) || maximum_offset_mm <= kNumericalEpsilon)
        return ZaaObjectSliceDecision::incompatible(ZaaIncompatibilityReason::SlicePlaneOffsetUnrepresentable);

    const bool automatic_request = facts.requested_slice_plane_offset_mm == 0.0;
    const double requested_offset_mm = automatic_request
        ? ZAA_DEFAULT_SLICE_PLANE_OFFSET_MM
        : facts.requested_slice_plane_offset_mm;
    if (!automatic_request && requested_offset_mm > maximum_offset_mm + kNumericalEpsilon) {
        return ZaaObjectSliceDecision::slice_plane_offset_limit_exceeded({
            requested_offset_mm,
            maximum_offset_mm,
            h_min,
            ZAA_SLICE_PLANE_TOP_CLEARANCE_MM
        });
    }

    // Automatic mode adapts to the object. The min also absorbs insignificant
    // floating-point overshoot accepted by the explicit-value comparison.
    const double slice_plane_offset_mm = canonical_zero(std::min(requested_offset_mm, maximum_offset_mm));
    if (!finite(slice_plane_offset_mm) || slice_plane_offset_mm <= kNumericalEpsilon)
        return ZaaObjectSliceDecision::incompatible(ZaaIncompatibilityReason::SlicePlaneOffsetUnrepresentable);

    plan.mode = ZaaSlicePlaneMode::ZaaOffsetPlane;
    plan.slice_plane_offset_mm = slice_plane_offset_mm;
    for (size_t i = 0; i < schedule.size(); ++i) {
        const ZaaPreparedLayerInterval &source = schedule[i];
        const double height = source.object_z_upper_mm - source.object_z_lower_mm;
        if (i == 0) {
            plan.layers.push_back({
                source.layer_index,
                canonical_zero(source.object_z_lower_mm),
                canonical_zero(source.object_z_upper_mm),
                canonical_zero(0.5 * (source.object_z_lower_mm + source.object_z_upper_mm)),
                canonical_zero(source.object_z_lower_mm),
                canonical_zero(source.object_z_upper_mm),
                canonical_zero(source.outer_wall_width_mm),
                ZaaSlicePlaneMode::ConventionalMidplane
            });
            continue;
        }

        if (height - slice_plane_offset_mm < ZAA_SLICE_PLANE_TOP_CLEARANCE_MM - kNumericalEpsilon)
            return ZaaObjectSliceDecision::incompatible(ZaaIncompatibilityReason::SlicePlaneOffsetUnrepresentable);
        plan.layers.push_back({
            source.layer_index,
            canonical_zero(source.object_z_lower_mm),
            canonical_zero(source.object_z_upper_mm),
            canonical_zero(source.object_z_lower_mm + slice_plane_offset_mm),
            canonical_zero(source.object_z_lower_mm + slice_plane_offset_mm),
            canonical_zero(source.object_z_upper_mm + slice_plane_offset_mm),
            canonical_zero(source.outer_wall_width_mm),
            ZaaSlicePlaneMode::ZaaOffsetPlane
        });
    }
    return ZaaObjectSliceDecision::supported(std::move(plan));
}

const char *to_string(ZaaPlanarReason reason)
{
    switch (reason) {
    case ZaaPlanarReason::NoHit: return "NoHit";
    case ZaaPlanarReason::AmbiguousHit: return "AmbiguousHit";
    case ZaaPlanarReason::WrongFacing: return "WrongFacing";
    case ZaaPlanarReason::GrazingSurface: return "GrazingSurface";
    case ZaaPlanarReason::OutOfRange: return "OutOfRange";
    }
    return "Unknown";
}

ZaaPointOutcome ZaaPointOutcome::displaced_by(double delta_mm)
{
    return ZaaPointOutcome(ZaaDisplaced{delta_mm});
}

ZaaPointOutcome ZaaPointOutcome::boundary_projected_to(double delta_mm)
{
    return ZaaPointOutcome(ZaaDisplaced{delta_mm, ZaaDisplacementSource::BoundaryProjected});
}

ZaaPointOutcome ZaaPointOutcome::planar(ZaaPlanarReason reason)
{
    return ZaaPointOutcome(ZaaPlanar{reason});
}

bool ZaaPointOutcome::is_displaced() const
{
    return std::holds_alternative<ZaaDisplaced>(m_value);
}

const ZaaDisplaced *ZaaPointOutcome::displaced() const
{
    return std::get_if<ZaaDisplaced>(&m_value);
}

const ZaaPlanar *ZaaPointOutcome::planar_result() const
{
    return std::get_if<ZaaPlanar>(&m_value);
}

ZaaRoleCapability zaa_role_capability(ExtrusionRole role)
{
    switch (role) {
    case erTopSolidInfill:
    case erPerimeter:
    case erExternalPerimeter:
    case erOverhangPerimeter:
        return ZaaRoleCapability::EligibleScalarMotion;
    default:
        return ZaaRoleCapability::ExcludedKeepPlanar;
    }
}

bool zaa_role_is_eligible(ExtrusionRole role)
{
    return zaa_role_capability(role) != ZaaRoleCapability::ExcludedKeepPlanar;
}

const char *to_string(ZaaPathRouting routing)
{
    switch (routing) {
    case ZaaPathRouting::Eligible: return "Eligible";
    case ZaaPathRouting::ExcludedKeepPlanar: return "ExcludedKeepPlanar";
    }
    return "Unknown";
}

const char *to_string(ZaaDiagnosticFallbackScope scope)
{
    switch (scope) {
    case ZaaDiagnosticFallbackScope::None: return "None";
    case ZaaDiagnosticFallbackScope::ObjectConventionalLayers: return "ObjectConventionalLayers";
    }
    return "Unknown";
}

const char *to_string(ZaaInvariantPhase phase)
{
    switch (phase) {
    case ZaaInvariantPhase::ProfileBuild: return "ProfileBuild";
    case ZaaInvariantPhase::WriterReady: return "WriterReady";
    }
    return "Unknown";
}

const char *to_string(ZaaInvariantReason reason)
{
    switch (reason) {
    case ZaaInvariantReason::ProfileStateViolation: return "ProfileStateViolation";
    case ZaaInvariantReason::PathPolicyViolation: return "PathPolicyViolation";
    case ZaaInvariantReason::NonFiniteValue: return "NonFiniteValue";
    case ZaaInvariantReason::EmittedSegmentCollapsed: return "EmittedSegmentCollapsed";
    case ZaaInvariantReason::MissingExtruder: return "MissingExtruder";
    case ZaaInvariantReason::ExtruderChanged: return "ExtruderChanged";
    case ZaaInvariantReason::PositionUnknown: return "PositionUnknown";
    case ZaaInvariantReason::NonFiniteWriterPosition: return "NonFiniteWriterPosition";
    case ZaaInvariantReason::ActiveLift: return "ActiveLift";
    case ZaaInvariantReason::PendingLift: return "PendingLift";
    case ZaaInvariantReason::ExtruderRetracted: return "ExtruderRetracted";
    case ZaaInvariantReason::RestartExtraPending: return "RestartExtraPending";
    case ZaaInvariantReason::PositionMismatch: return "PositionMismatch";
    }
    return "Unknown";
}

namespace {

struct ZaaInvariantCodeEntry {
    ZaaInvariantPhase phase;
    ZaaInvariantReason reason;
    const char *code;
};

constexpr std::array<ZaaInvariantCodeEntry, 13> kZaaInvariantCodes{{
    {ZaaInvariantPhase::ProfileBuild, ZaaInvariantReason::ProfileStateViolation, "ZAA.Invariant.ProfileBuild.ProfileStateViolation"},
    {ZaaInvariantPhase::ProfileBuild, ZaaInvariantReason::PathPolicyViolation, "ZAA.Invariant.ProfileBuild.PathPolicyViolation"},
    {ZaaInvariantPhase::ProfileBuild, ZaaInvariantReason::NonFiniteValue, "ZAA.Invariant.ProfileBuild.NonFiniteValue"},
    {ZaaInvariantPhase::ProfileBuild, ZaaInvariantReason::EmittedSegmentCollapsed, "ZAA.Invariant.ProfileBuild.EmittedSegmentCollapsed"},
    {ZaaInvariantPhase::WriterReady, ZaaInvariantReason::MissingExtruder, "ZAA.Invariant.WriterReady.MissingExtruder"},
    {ZaaInvariantPhase::WriterReady, ZaaInvariantReason::ExtruderChanged, "ZAA.Invariant.WriterReady.ExtruderChanged"},
    {ZaaInvariantPhase::WriterReady, ZaaInvariantReason::PositionUnknown, "ZAA.Invariant.WriterReady.PositionUnknown"},
    {ZaaInvariantPhase::WriterReady, ZaaInvariantReason::NonFiniteWriterPosition, "ZAA.Invariant.WriterReady.NonFiniteWriterPosition"},
    {ZaaInvariantPhase::WriterReady, ZaaInvariantReason::ActiveLift, "ZAA.Invariant.WriterReady.ActiveLift"},
    {ZaaInvariantPhase::WriterReady, ZaaInvariantReason::PendingLift, "ZAA.Invariant.WriterReady.PendingLift"},
    {ZaaInvariantPhase::WriterReady, ZaaInvariantReason::ExtruderRetracted, "ZAA.Invariant.WriterReady.ExtruderRetracted"},
    {ZaaInvariantPhase::WriterReady, ZaaInvariantReason::RestartExtraPending, "ZAA.Invariant.WriterReady.RestartExtraPending"},
    {ZaaInvariantPhase::WriterReady, ZaaInvariantReason::PositionMismatch, "ZAA.Invariant.WriterReady.PositionMismatch"}
}};

std::string diagnostic_number(double value)
{
    if (std::isnan(value))
        return "nan";
    if (std::isinf(value))
        return value > 0.0 ? "+inf" : "-inf";
    std::ostringstream out;
    out << std::fixed << std::setprecision(9) << canonical_zero(value);
    return out.str();
}

const char *diagnostic_bool(bool value)
{
    return value ? "true" : "false";
}

const char *path_policy_token(ZaaPathPolicy policy)
{
    switch (policy) {
    case ZaaPathPolicy::Conventional: return "Conventional";
    case ZaaPathPolicy::ZaaLinearCandidate: return "ZaaLinearCandidate";
    }
    return "Unknown";
}

template<class Details>
const Details &required_details(const ZaaInvariantFailure &failure)
{
    const Details *details = std::get_if<Details>(&failure.details);
    if (details == nullptr)
        throw LogicError(_u8L("ZAA invariant reason/details mismatch"));
    return *details;
}

void append_xyz(std::ostringstream &out, const char *prefix, const Vec3d &xyz)
{
    out << ' ' << prefix << "_x_mm=" << diagnostic_number(xyz.x())
        << ' ' << prefix << "_y_mm=" << diagnostic_number(xyz.y())
        << ' ' << prefix << "_z_mm=" << diagnostic_number(xyz.z());
}

} // namespace

std::string zaa_invariant_code(const ZaaInvariantFailure &failure)
{
    for (const ZaaInvariantCodeEntry &entry : kZaaInvariantCodes)
        if (entry.phase == failure.phase && entry.reason == failure.reason)
            return entry.code;
    throw LogicError(_u8L("Invalid ZAA invariant phase/reason pair"));
}

std::string serialize_zaa_invariant(const ZaaInvariantFailure &failure)
{
    if (failure.model_object_id.empty() || failure.model_instance_id.empty() ||
        !failure.layer_id || !failure.layer_print_z_mm || failure.role.empty() || !failure.path_ordinal)
        throw LogicError(_u8L("ZAA invariant diagnostic is missing required common context"));

    std::ostringstream out;
    out << _u8L("ZAA internal consistency check failed. G-code generation was stopped.") << "\n"
        << "ZAA_DIAGNOSTIC code=" << zaa_invariant_code(failure)
        << " phase=" << to_string(failure.phase)
        << " reason=" << to_string(failure.reason)
        << " model_object_id=" << stable_token(failure.model_object_id)
        << " model_instance_id=" << stable_token(failure.model_instance_id)
        << " layer_id=" << *failure.layer_id
        << " layer_print_z_mm=" << diagnostic_number(*failure.layer_print_z_mm)
        << " role=" << stable_token(failure.role)
        << " path_ordinal=" << *failure.path_ordinal;

    switch (failure.reason) {
    case ZaaInvariantReason::ProfileStateViolation: {
        const auto &details = required_details<ZaaSpatialPathStateFailure>(failure);
        out << " expected_path3_present=" << diagnostic_bool(details.expected_path3_present)
            << " actual_path3_present=" << diagnostic_bool(details.actual_path3_present);
        if (details.actual_path3_present) {
            if (!details.path3_point_count)
                throw LogicError(_u8L("ZAA spatial path state is missing path3_point_count"));
            out << " path3_point_count=" << *details.path3_point_count;
        }
        if (!details.failing_check.empty())
            out << " check=" << stable_token(details.failing_check);
        break;
    }
    case ZaaInvariantReason::PathPolicyViolation: {
        const auto &details = required_details<ZaaPathPolicyFailure>(failure);
        out << " expected_path_policy=" << path_policy_token(details.expected_path_policy)
            << " actual_path_policy=" << path_policy_token(details.actual_path_policy)
            << " arc_fit_present=" << diagnostic_bool(details.arc_fit_present);
        break;
    }
    case ZaaInvariantReason::NonFiniteValue: {
        const auto &details = required_details<ZaaNonFiniteFailure>(failure);
        if (details.value_field.empty())
            throw LogicError(_u8L("ZAA NonFiniteValue is missing value_field"));
        out << " value_field=" << stable_token(details.value_field)
            << " actual_value=" << diagnostic_number(details.actual_value);
        if (details.point_index)
            out << " point_index=" << *details.point_index;
        if (details.segment_index)
            out << " segment_index=" << *details.segment_index;
        break;
    }
    case ZaaInvariantReason::EmittedSegmentCollapsed: {
        const auto &details = required_details<ZaaEmittedSegmentCollapsedFailure>(failure);
        out << " segment_index=" << details.segment_index;
        append_xyz(out, "input_start", details.input_start_xyz);
        append_xyz(out, "input_end", details.input_end_xyz);
        append_xyz(out, "emitted_start", details.emitted_start_xyz);
        append_xyz(out, "emitted_end", details.emitted_end_xyz);
        break;
    }
    case ZaaInvariantReason::MissingExtruder:
    case ZaaInvariantReason::ExtruderChanged:
    case ZaaInvariantReason::PositionUnknown:
    case ZaaInvariantReason::NonFiniteWriterPosition:
    case ZaaInvariantReason::ActiveLift:
    case ZaaInvariantReason::PendingLift:
    case ZaaInvariantReason::ExtruderRetracted:
    case ZaaInvariantReason::RestartExtraPending:
    case ZaaInvariantReason::PositionMismatch: {
        const auto &state = required_details<ZaaWriterReadyState>(failure);
        out << " expected_extruder_id=" << state.expected_extruder_id
            << " writer_extruder_present=" << diagnostic_bool(state.writer_extruder_present);
        if (state.writer_extruder_present && state.actual_extruder_id)
            out << " actual_extruder_id=" << *state.actual_extruder_id;
        out << " writer_position_known=" << diagnostic_bool(state.position_known);
        append_xyz(out, "writer_stored", state.writer_position);
        append_xyz(out, "expected_start", state.expected_position);
        out << " writer_nominal_z_mm=" << diagnostic_number(state.writer_nominal_z_mm)
            << " active_lift_mm=" << diagnostic_number(state.active_lift_mm)
            << " pending_lift_mm=" << diagnostic_number(state.pending_lift_mm);
        if (state.writer_extruder_present) {
            if (!state.retracted_mm || !state.restart_extra_mm)
                throw LogicError("ZAA WriterReady snapshot is missing extruder state");
            out << " retracted_mm=" << diagnostic_number(*state.retracted_mm)
                << " restart_extra_mm=" << diagnostic_number(*state.restart_extra_mm);
        }
        if (failure.reason == ZaaInvariantReason::NonFiniteWriterPosition) {
            std::string axes;
            if (!finite(state.writer_position.x())) axes += "X";
            if (!finite(state.writer_position.y())) axes += axes.empty() ? "Y" : ",Y";
            if (!finite(state.writer_position.z())) axes += axes.empty() ? "Z" : ",Z";
            out << " nonfinite_axes=" << (axes.empty() ? "none" : axes);
        } else if (failure.reason == ZaaInvariantReason::PositionMismatch) {
            out << " raw_position_match=" << diagnostic_bool(close_xyz(state.writer_position, state.expected_position))
                << " emitted_position_match=" << diagnostic_bool(state.writer_emitted_position == state.expected_emitted_position)
                << " position_epsilon_mm=" << diagnostic_number(EPSILON);
            append_xyz(out, "writer_emitted", state.writer_emitted_position);
            append_xyz(out, "expected_emitted", state.expected_emitted_position);
        }
        break;
    }
    }
    return out.str();
}

ZaaMaterialMotionPlanResult make_zaa_material_motion_plan(
    const ExtrusionPath3 &path3,
    const ZaaLayerGeometry &layer_geometry,
    double nominal_print_z_mm,
    const std::vector<Vec3d> &input_xyz,
    const std::vector<Vec3d> &emitted_xyz,
    const ZaaFlowReference &flow,
    double q_nominal_mm2,
    double e_per_mm3,
    bool force_no_extrusion)
{
    auto failed = [](ZaaInvariantReason reason, ZaaInvariantDetails details) {
        ZaaMaterialMotionPlanResult result;
        ZaaInvariantFailure failure;
        failure.phase = ZaaInvariantPhase::ProfileBuild;
        failure.reason = reason;
        failure.details = std::move(details);
        result.failure = std::move(failure);
        return result;
    };
    auto nonfinite = [&](std::string field, double value,
                         std::optional<size_t> point_index = std::nullopt,
                         std::optional<size_t> segment_index = std::nullopt) {
        return failed(ZaaInvariantReason::NonFiniteValue,
            ZaaNonFiniteFailure{std::move(field), value, point_index, segment_index});
    };
    auto emitted_collapsed = [&](size_t start_index, size_t end_index) {
        return failed(ZaaInvariantReason::EmittedSegmentCollapsed,
            ZaaEmittedSegmentCollapsedFailure{
                start_index,
                input_xyz[start_index],
                input_xyz[end_index],
                emitted_xyz[start_index],
                emitted_xyz[end_index]
            });
    };

    const Polyline3 &spatial = path3.polyline3();
    if (!finite(nominal_print_z_mm) || spatial.points.size() < 2 ||
        input_xyz.size() != spatial.points.size() || emitted_xyz.size() != spatial.points.size()) {
        return failed(ZaaInvariantReason::ProfileStateViolation, ZaaSpatialPathStateFailure{
            true,
            true,
            spatial.points.size()
        });
    }

    const double reference_height = layer_geometry.query_upper_z_mm - layer_geometry.query_lower_z_mm;
    if (!finite(reference_height) || reference_height <= 0.0)
        return nonfinite("reference_height_mm", reference_height);
    assert(flow.bridge || finite(flow.width_mm));
    assert(flow.bridge || finite(flow.height_mm));
    assert(flow.bridge || std::abs(flow.height_mm - reference_height) <= kNumericalEpsilon);
    assert(flow.bridge || (flow.height_mm > 0.0 && flow.height_mm <= flow.width_mm));
    const double reference_area = flow.bridge ? 0.0 : rounded_rectangle_area(flow.width_mm, reference_height);
    assert(flow.bridge || reference_area > 0.0);
    if (!finite(q_nominal_mm2) || q_nominal_mm2 < 0.0)
        return nonfinite("q_nominal_mm2", q_nominal_mm2);
    if (!force_no_extrusion && (!finite(e_per_mm3) || e_per_mm3 < 0.0))
        return nonfinite("e_per_mm3", e_per_mm3);

    for (size_t index = 0; index < spatial.points.size(); ++index) {
        for (size_t axis = 0; axis < 3; ++axis) {
            if (!finite(input_xyz[index][axis]))
                return nonfinite(axis == 0 ? "input_x_mm" : axis == 1 ? "input_y_mm" : "input_z_mm",
                                 input_xyz[index][axis], index);
            if (!finite(emitted_xyz[index][axis]))
                return nonfinite(axis == 0 ? "emitted_x_mm" : axis == 1 ? "emitted_y_mm" : "emitted_z_mm",
                                 emitted_xyz[index][axis], index);
        }
    }

    ZaaMaterialMotionPlan plan;
    plan.reference_height_mm = reference_height;
    plan.q_nominal_mm2 = q_nominal_mm2;
    plan.q_effective_mm2 = force_no_extrusion ? 0.0 : q_nominal_mm2;
    plan.segments.reserve(spatial.points.size() - 1);

    struct PendingSpan {
        double length_xy_mm{0.0};
        double length_3d_mm{0.0};
        double volume_mm3{0.0};
        double dE{0.0};
        bool has_raw_segment{false};
    };

    auto add_raw_segment = [&](size_t end_index, PendingSpan &span)
        -> std::optional<ZaaMaterialMotionPlanResult> {
        const size_t segment_index = end_index - 1;
        const Vec3d raw_delta = input_xyz[end_index] - input_xyz[segment_index];
        const double length_xy = std::hypot(raw_delta.x(), raw_delta.y());
        const double delta_0 = input_xyz[segment_index].z() - nominal_print_z_mm;
        const double delta_1 = input_xyz[end_index].z() - nominal_print_z_mm;
        const double thickness_0 = reference_height + delta_0;
        const double thickness_1 = reference_height + delta_1;
        const double ratio_0 = thickness_0 / reference_height;
        const double ratio_1 = thickness_1 / reference_height;
        double material_ratio = (ratio_0 + ratio_1) * 0.5;
        if (!flow.bridge) {
            assert(thickness_0 > 0.0 && thickness_0 <= flow.width_mm);
            assert(thickness_1 > 0.0 && thickness_1 <= flow.width_mm);
            const double thickness_avg = (thickness_0 + thickness_1) * 0.5;
            const double thickness_delta = thickness_1 - thickness_0;
            // Exact mean of A(t) = w*t - (1 - pi/4)*t^2 for linearly interpolated thickness.
            const double average_area =
                flow.width_mm * thickness_avg -
                kRoundedRectangleCorrection *
                    (thickness_avg * thickness_avg + thickness_delta * thickness_delta / 12.0);
            material_ratio = average_area / reference_area;
        }
        const double volume = plan.q_effective_mm2 * length_xy * material_ratio;
        const double dE = force_no_extrusion ? 0.0 : volume * e_per_mm3;

        if (!finite(length_xy) || length_xy <= 0.0)
            return nonfinite("length_xy_mm", length_xy, std::nullopt, segment_index);
        if (!finite(ratio_0))
            return nonfinite("start_thickness_ratio", ratio_0, std::nullopt, segment_index);
        if (!finite(ratio_1))
            return nonfinite("end_thickness_ratio", ratio_1, std::nullopt, segment_index);
        if (!finite(material_ratio))
            return nonfinite("material_ratio", material_ratio, std::nullopt, segment_index);
        if (!finite(volume) || volume < 0.0)
            return nonfinite("volume_mm3", volume, std::nullopt, segment_index);
        if (!finite(dE) || dE < 0.0)
            return nonfinite("dE", dE, std::nullopt, segment_index);

        span.length_xy_mm += length_xy;
        span.length_3d_mm += raw_delta.norm();
        span.volume_mm3 += volume;
        span.dE += dE;
        span.has_raw_segment = true;
        return std::nullopt;
    };

    auto append_emitted_span = [&](size_t start_index, size_t end_index, const PendingSpan &span)
        -> std::optional<ZaaMaterialMotionPlanResult> {
        if (!span.has_raw_segment)
            return std::nullopt;

        const Vec3d emitted_delta = emitted_xyz[end_index] - emitted_xyz[start_index];
        const double length_3d_emitted = emitted_delta.norm();
        if (length_3d_emitted == 0.0) {
            if (span.volume_mm3 > 0.0 || span.dE > 0.0)
                return emitted_collapsed(start_index, end_index);
            return std::nullopt;
        }

        const double delta_z_emitted = emitted_delta.z();
        const double q3d = force_no_extrusion ? 0.0 : span.volume_mm3 / length_3d_emitted;
        const double z_ratio = std::abs(delta_z_emitted) / length_3d_emitted;
        const size_t segment_index = start_index;
        if (!finite(length_3d_emitted))
            return nonfinite("length_3d_emitted_mm", length_3d_emitted, std::nullopt, segment_index);
        if (!finite(delta_z_emitted))
            return nonfinite("delta_z_emitted_mm", delta_z_emitted, std::nullopt, segment_index);
        if (!finite(q3d))
            return nonfinite("q3d_mm2", q3d, std::nullopt, segment_index);
        if (!finite(z_ratio))
            return nonfinite("z_ratio", z_ratio, std::nullopt, segment_index);

        plan.q3d_max_mm2 = std::max(plan.q3d_max_mm2, q3d);
        plan.z_ratio_max = std::max(plan.z_ratio_max, z_ratio);
        plan.segments.push_back({
            start_index,
            end_index,
            end_index,
            span.length_xy_mm,
            length_3d_emitted,
            delta_z_emitted,
            span.volume_mm3,
            span.dE,
            q3d,
            z_ratio
        });
        return std::nullopt;
    };

    const size_t last_index = spatial.points.size() - 1;
    size_t span_start = 0;
    while (span_start < last_index) {
        PendingSpan pending;
        size_t target = span_start;
        do {
            ++target;
            if (auto failure = add_raw_segment(target, pending))
                return *failure;
        } while (target < last_index && same_emitted_xyz(emitted_xyz[target], emitted_xyz[span_start]));

        if (same_emitted_xyz(emitted_xyz[target], emitted_xyz[span_start])) {
            // The scan above visited every point of this span. Only a whole path
            // contained in one output cell may be discarded; equal endpoints of
            // a closed path alone do not qualify. Bound total travel as well as
            // material, so repeated backtracking within the cell is not hidden.
            const double max_length_xy = std::sqrt(2.0) * GCodeFormatter::XYZ_EPSILON;
            const double max_length_3d = std::sqrt(3.0) * GCodeFormatter::XYZ_EPSILON;
            const double max_volume = plan.q_effective_mm2 * max_length_xy;
            const double max_dE = force_no_extrusion ? 0.0 : max_volume * e_per_mm3;
            constexpr double roundoff = 1.0 + 1e-9;
            if (span_start == 0 && target == last_index && plan.segments.empty() &&
                finite(pending.length_xy_mm) && finite(pending.volume_mm3) && finite(pending.dE) &&
                finite(pending.length_3d_mm) && pending.length_3d_mm <= max_length_3d * roundoff &&
                finite(max_volume) && finite(max_dE) &&
                pending.length_xy_mm <= max_length_xy * roundoff &&
                pending.volume_mm3 <= max_volume * roundoff && pending.dE <= max_dE * roundoff) {
                ZaaMaterialMotionPlanResult result;
                result.skipped_micro_path = ZaaSkippedMicroPath{
                    pending.length_xy_mm, pending.length_3d_mm, pending.volume_mm3, pending.dE};
                return result;
            }
            return emitted_collapsed(span_start, target);
        }

        while (target < last_index && same_emitted_xyz(emitted_xyz[target + 1], emitted_xyz[target])) {
            ++target;
            if (auto failure = add_raw_segment(target, pending))
                return *failure;
        }
        if (auto failure = append_emitted_span(span_start, target, pending))
            return *failure;
        span_start = target;
    }

    if (plan.segments.empty())
        return emitted_collapsed(0, last_index);

    ZaaMaterialMotionPlanResult result;
    result.plan = std::move(plan);
    return result;
}

double zaa_max_volumetric_speed_limit(
    double max_volumetric_speed_mm3_s,
    double q3d_max_mm2,
    double filament_flow_ratio)
{
    assert(finite(max_volumetric_speed_mm3_s) && max_volumetric_speed_mm3_s > 0.0);
    assert(finite(q3d_max_mm2) && q3d_max_mm2 > 0.0);
    assert(finite(filament_flow_ratio) && filament_flow_ratio > 0.0);
    return max_volumetric_speed_mm3_s / (q3d_max_mm2 * filament_flow_ratio);
}

[[noreturn]] void throw_zaa_invariant(const ZaaInvariantFailure &failure)
{
    const std::string message = serialize_zaa_invariant(failure);
    BOOST_LOG_TRIVIAL(error) << message;
    throw LogicError(message);
}

std::optional<ZaaInvariantFailure> zaa_writer_ready_failure(const ZaaWriterReadyState &state)
{
    if (!kEnableZaaWriterReadyCheck)
        return std::nullopt;

    auto failure = [&](ZaaInvariantReason reason) {
        ZaaInvariantFailure out;
        out.phase = ZaaInvariantPhase::WriterReady;
        out.reason = reason;
        out.details = state;
        return std::optional<ZaaInvariantFailure>(std::move(out));
    };
    if (!state.writer_extruder_present || !state.actual_extruder_id)
        return failure(ZaaInvariantReason::MissingExtruder);
    if (*state.actual_extruder_id != state.expected_extruder_id)
        return failure(ZaaInvariantReason::ExtruderChanged);
    if (!state.position_known)
        return failure(ZaaInvariantReason::PositionUnknown);
    if (!state.writer_position.allFinite() || !state.expected_position.allFinite() ||
        !state.writer_emitted_position.allFinite() || !state.expected_emitted_position.allFinite())
        return failure(ZaaInvariantReason::NonFiniteWriterPosition);
    if (!finite(state.active_lift_mm) || std::abs(state.active_lift_mm) > EPSILON)
        return failure(ZaaInvariantReason::ActiveLift);
    if (!finite(state.pending_lift_mm) || std::abs(state.pending_lift_mm) > EPSILON)
        return failure(ZaaInvariantReason::PendingLift);
    if (!state.retracted_mm || !finite(*state.retracted_mm) ||
        std::abs(*state.retracted_mm) > EPSILON)
        return failure(ZaaInvariantReason::ExtruderRetracted);
    if (!state.restart_extra_mm || !finite(*state.restart_extra_mm) ||
        std::abs(*state.restart_extra_mm) > EPSILON)
        return failure(ZaaInvariantReason::RestartExtraPending);
    // m_pos tracks coordinates at the formatter's output precision, whereas a
    // planned ZAA path keeps the original resampled coordinate. The emitted
    // positions are therefore the printer-visible boundary contract.
    if (state.writer_emitted_position != state.expected_emitted_position)
        return failure(ZaaInvariantReason::PositionMismatch);
    return std::nullopt;
}

class ZaaTaskContext::Impl {
public:
    struct KeyHash {
        size_t operator()(const ZaaInstanceKey &key) const noexcept
        {
            const size_t pointer_hash = std::hash<const void *>{}(key.print_object);
            return pointer_hash ^ (key.instance_index + 0x9e3779b9u + (pointer_hash << 6) + (pointer_hash >> 2));
        }
    };

    struct StoredInstance {
        struct PathDiagnostic {
            ZaaPathRouting routing{ZaaPathRouting::ExcludedKeepPlanar};
            double layer_print_z_mm{0.0};
            std::string role;
        };

        ZaaInstanceContext public_context;
        std::map<size_t, size_t> next_path_ordinal_by_layer;
        std::map<std::pair<size_t, size_t>, PathDiagnostic> paths;

        StoredInstance(
            ZaaInstanceKey key,
            std::string model_object_id,
            std::string model_instance_id)
            : public_context{key, std::move(model_object_id), std::move(model_instance_id)}
        {}
    };

    std::unordered_map<ZaaInstanceKey, std::unique_ptr<StoredInstance>, KeyHash> instances;
    std::vector<ZaaTaskContext::ObjectDiagnostics> object_diagnostics;
    std::unordered_map<const PrintObject *, size_t> object_index_by_print_object;
    StoredInstance *current{nullptr};
    mutable bool summary_emitted{false};
    uint64_t paths_skipped_micro{0};
    ZaaSkippedMicroPath skipped_micro_totals;

    ZaaTaskContext::ObjectDiagnostics &current_object_diagnostics()
    {
        if (current == nullptr)
            throw LogicError(_u8L("ZAA diagnostics require an active instance binding"));
        const auto it = object_index_by_print_object.find(current->public_context.key.print_object);
        if (it == object_index_by_print_object.end())
            throw LogicError(_u8L("ZAA diagnostics object decision is missing"));
        ZaaTaskContext::ObjectDiagnostics &object = object_diagnostics[it->second];
        if (object.decision != ZaaObjectDecisionKind::Supported)
            throw LogicError(_u8L("ZAA path diagnostics require a Supported object"));
        return object;
    }

    ZaaTaskContext::DiagnosticsSummary make_summary() const
    {
        ZaaTaskContext::DiagnosticsSummary result;
        result.paths_skipped_micro = paths_skipped_micro;
        result.skipped_micro_totals = skipped_micro_totals;
        result.objects.reserve(object_diagnostics.size());
        std::vector<const ZaaTaskContext::ObjectDiagnostics *> ordered_objects;
        ordered_objects.reserve(object_diagnostics.size());
        for (const ZaaTaskContext::ObjectDiagnostics &object : object_diagnostics)
            ordered_objects.push_back(&object);
        std::sort(ordered_objects.begin(), ordered_objects.end(),
            [](const ZaaTaskContext::ObjectDiagnostics *lhs,
               const ZaaTaskContext::ObjectDiagnostics *rhs) {
                if (lhs->model_object_id != rhs->model_object_id)
                    return lhs->model_object_id < rhs->model_object_id;
                return lhs->task_object_ordinal < rhs->task_object_ordinal;
            });

        for (const ZaaTaskContext::ObjectDiagnostics *object_ptr : ordered_objects) {
            const ZaaTaskContext::ObjectDiagnostics &object = *object_ptr;
            result.objects.push_back(object);
            switch (object.decision) {
            case ZaaObjectDecisionKind::Disabled:
                ++result.objects_disabled;
                break;
            case ZaaObjectDecisionKind::Incompatible:
                ++result.objects_incompatible;
                result.enabled = true;
                if (object.incompatibility_reason) {
                    const size_t reason = incompatibility_index(*object.incompatibility_reason);
                    if (reason < result.incompatibility_reasons.size())
                        ++result.incompatibility_reasons[reason];
                }
                break;
            case ZaaObjectDecisionKind::SlicePlaneOffsetLimitExceeded:
                // Print::process rejects this user-correctable configuration
                // before task diagnostics are normally created.
                result.enabled = true;
                break;
            case ZaaObjectDecisionKind::Supported:
                ++result.objects_supported;
                result.enabled = true;
                break;
            }
            result.paths_eligible += object.paths_eligible;
            result.paths_excluded += object.paths_excluded;
        }
        return result;
    }
};

ZaaTaskContext::ZaaTaskContext() : m_impl(std::make_unique<Impl>()) {}
ZaaTaskContext::~ZaaTaskContext() = default;
ZaaTaskContext::ZaaTaskContext(ZaaTaskContext &&) noexcept = default;
ZaaTaskContext &ZaaTaskContext::operator=(ZaaTaskContext &&) noexcept = default;

void ZaaTaskContext::reset()
{
    m_impl = std::make_unique<Impl>();
}

void ZaaTaskContext::prepare_instance(
    ZaaInstanceKey key,
    std::string model_object_id,
    std::string model_instance_id)
{
    if (key.print_object == nullptr)
        throw LogicError(_u8L("Cannot prepare an empty ZAA instance context"));
    const auto object_it = m_impl->object_index_by_print_object.find(key.print_object);
    if (object_it == m_impl->object_index_by_print_object.end())
        throw LogicError(_u8L("ZAA instance preparation requires a recorded object decision"));
    const ObjectDiagnostics &object = m_impl->object_diagnostics[object_it->second];
    if (object.decision != ZaaObjectDecisionKind::Supported)
        throw LogicError(_u8L("Only a Supported object may prepare a ZAA instance context"));
    if (object.model_object_id != model_object_id)
        throw LogicError(_u8L("ZAA instance identity does not match its object diagnostics partition"));
    if (m_impl->instances.find(key) != m_impl->instances.end())
        throw LogicError(_u8L("ZAA instance context was prepared twice"));
    m_impl->instances.emplace(key, std::make_unique<Impl::StoredInstance>(
        key, std::move(model_object_id), std::move(model_instance_id)));
}

ZaaScopedInstanceBinding ZaaTaskContext::bind_instance(ZaaInstanceKey key)
{
    return ZaaScopedInstanceBinding(*this, key);
}

void ZaaTaskContext::activate_instance(ZaaInstanceKey key)
{
    if (m_impl->current != nullptr)
        throw LogicError(_u8L("Nested ZAA instance bindings are not allowed"));
    const auto it = m_impl->instances.find(key);
    if (it == m_impl->instances.end())
        throw LogicError(_u8L("ZAA instance context is missing"));
    m_impl->current = it->second.get();
}

void ZaaTaskContext::deactivate_instance()
{
    m_impl->current = nullptr;
}

const ZaaInstanceContext *ZaaTaskContext::current_instance() const
{
    return m_impl->current == nullptr ? nullptr : &m_impl->current->public_context;
}

void ZaaTaskContext::record_object_decision(
    const PrintObject *print_object,
    std::string model_object_id,
    const ZaaObjectSliceDecision &decision)
{
    if (print_object == nullptr || model_object_id.empty())
        throw InvalidArgument(_u8L("ZAA object diagnostics require a PrintObject key and model object ID"));
    if (m_impl->object_index_by_print_object.find(print_object) !=
        m_impl->object_index_by_print_object.end())
        throw LogicError(_u8L("ZAA object decision was recorded twice"));

    ObjectDiagnostics object;
    object.task_object_ordinal = m_impl->object_diagnostics.size();
    object.model_object_id = std::move(model_object_id);
    object.decision = decision.kind();
    object.incompatibility_reason = decision.incompatibility_reason();
    object.fallback_scope = decision.is_incompatible() ?
        ZaaDiagnosticFallbackScope::ObjectConventionalLayers :
        ZaaDiagnosticFallbackScope::None;

    const size_t object_index = object.task_object_ordinal;
    m_impl->object_diagnostics.push_back(std::move(object));
    m_impl->object_index_by_print_object.emplace(print_object, object_index);
}

size_t ZaaTaskContext::next_path_ordinal(size_t layer_id) const
{
    if (m_impl->current == nullptr)
        throw LogicError(_u8L("ZAA path ordinal requires an active instance binding"));
    const auto it = m_impl->current->next_path_ordinal_by_layer.find(layer_id);
    return it == m_impl->current->next_path_ordinal_by_layer.end() ? 0 : it->second;
}

size_t ZaaTaskContext::begin_path(
    ZaaPathRouting routing,
    size_t layer_id,
    double layer_print_z_mm,
    std::string role)
{
    if (!finite(layer_print_z_mm) || role.empty())
        throw InvalidArgument(_u8L("ZAA path diagnostics require a finite layer Z and stable role"));

    ObjectDiagnostics &object = m_impl->current_object_diagnostics();
    size_t &next_ordinal = m_impl->current->next_path_ordinal_by_layer[layer_id];
    const size_t path_ordinal = next_ordinal++;
    m_impl->current->paths.emplace(
        std::make_pair(layer_id, path_ordinal),
        Impl::StoredInstance::PathDiagnostic{routing, layer_print_z_mm, role});

    if (routing == ZaaPathRouting::Eligible)
        ++object.paths_eligible;
    else
        ++object.paths_excluded;

    // std::ostringstream detail;
    // detail << std::setprecision(std::numeric_limits<double>::max_digits10)
    //        << "ZAA path model_object_id="
    //        << stable_token(m_impl->current->public_context.model_object_id)
    //        << " model_instance_id="
    //        << stable_token(m_impl->current->public_context.model_instance_id)
    //        << " layer_id=" << layer_id
    //        << " layer_print_z_mm=" << canonical_zero(layer_print_z_mm)
    //        << " role=" << stable_token(role)
    //        << " path_ordinal=" << path_ordinal
    //        << " routing=" << to_string(routing);
    // BOOST_LOG_TRIVIAL(debug) << detail.str();
    return path_ordinal;
}

void ZaaTaskContext::record_skipped_micro_path(
    size_t layer_id, size_t path_ordinal, const ZaaSkippedMicroPath &skipped)
{
    if (m_impl->current == nullptr)
        throw LogicError(_u8L("ZAA skipped path requires an active instance binding"));
    const auto it = m_impl->current->paths.find({layer_id, path_ordinal});
    if (it == m_impl->current->paths.end() || it->second.routing != ZaaPathRouting::Eligible)
        throw LogicError(_u8L("ZAA skipped path requires a registered eligible path"));
    ++m_impl->paths_skipped_micro;
    m_impl->skipped_micro_totals.length_xy_mm += skipped.length_xy_mm;
    m_impl->skipped_micro_totals.length_3d_mm += skipped.length_3d_mm;
    m_impl->skipped_micro_totals.volume_mm3 += skipped.volume_mm3;
    m_impl->skipped_micro_totals.dE += skipped.dE;

    const auto &instance = m_impl->current->public_context;
    BOOST_LOG_TRIVIAL(info) << "ZAA skipped_micro_path model_object_id=" << stable_token(instance.model_object_id)
        << " model_instance_id=" << stable_token(instance.model_instance_id)
        << " layer_id=" << layer_id << " layer_print_z_mm=" << diagnostic_number(it->second.layer_print_z_mm)
        << " role=" << stable_token(it->second.role) << " path_ordinal=" << path_ordinal
        << " length_xy_mm=" << diagnostic_number(skipped.length_xy_mm)
        << " length_3d_mm=" << diagnostic_number(skipped.length_3d_mm)
        << " discarded_volume_mm3=" << diagnostic_number(skipped.volume_mm3)
        << " discarded_filament_mm=" << diagnostic_number(skipped.dE);
}

void ZaaTaskContext::emit_summary() const
{
    if (m_impl->summary_emitted)
        return;
    m_impl->summary_emitted = true;
    // const DiagnosticsSummary s = m_impl->make_summary();

    // std::ostringstream out;
    // out << "ZAA summary enabled=" << (s.enabled ? "true" : "false")
    //     << " objects={disabled:" << s.objects_disabled
    //     << ",incompatible:" << s.objects_incompatible
    //     << ",supported:" << s.objects_supported
    //     << "} incompatibility_reasons={";
    // for (size_t reason = 0; reason < s.incompatibility_reasons.size(); ++reason) {
    //     if (reason != 0)
    //         out << ',';
    //     out << to_string(static_cast<ZaaIncompatibilityReason>(reason))
    //         << ':' << s.incompatibility_reasons[reason];
    // }
    // out << "} paths={eligible:" << s.paths_eligible
    //     << ",excluded:" << s.paths_excluded
    //     << ",skipped_micro:" << s.paths_skipped_micro
    //     << "} skipped_micro_totals={length_xy_mm:" << diagnostic_number(s.skipped_micro_totals.length_xy_mm)
    //     << ",length_3d_mm:" << diagnostic_number(s.skipped_micro_totals.length_3d_mm)
    //     << ",discarded_volume_mm3:" << diagnostic_number(s.skipped_micro_totals.volume_mm3)
    //     << ",discarded_filament_mm:" << diagnostic_number(s.skipped_micro_totals.dE)
    //     << "} object_partitions=[";
    // for (size_t i = 0; i < s.objects.size(); ++i) {
    //     if (i != 0)
    //         out << ';';
    //     const ObjectDiagnostics &object = s.objects[i];
    //     out << "{task_object_ordinal=" << object.task_object_ordinal
    //         << ",model_object_id=" << stable_token(object.model_object_id)
    //         << ",decision=";
    //     switch (object.decision) {
    //     case ZaaObjectDecisionKind::Disabled: out << "Disabled"; break;
    //     case ZaaObjectDecisionKind::Incompatible: out << "Incompatible"; break;
    //     case ZaaObjectDecisionKind::SlicePlaneOffsetLimitExceeded:
    //         out << "SlicePlaneOffsetLimitExceeded";
    //         break;
    //     case ZaaObjectDecisionKind::Supported: out << "Supported"; break;
    //     }
    //     if (object.incompatibility_reason)
    //         out << ",reason=" << to_string(*object.incompatibility_reason);
    //     out << ",fallback=" << to_string(object.fallback_scope)
    //         << ",paths=" << object.paths_eligible << '/' << object.paths_excluded;
    //     out << '}';
    // }
    // out << ']';
    // BOOST_LOG_TRIVIAL(info) << out.str();
}

ZaaScopedInstanceBinding::ZaaScopedInstanceBinding(ZaaTaskContext &context, ZaaInstanceKey key)
    : m_context(&context)
{
    m_context->activate_instance(key);
}

ZaaScopedInstanceBinding::~ZaaScopedInstanceBinding()
{
    if (m_context != nullptr)
        m_context->deactivate_instance();
}

ZaaScopedInstanceBinding::ZaaScopedInstanceBinding(ZaaScopedInstanceBinding &&other) noexcept
    : m_context(std::exchange(other.m_context, nullptr))
{}

ZaaScopedInstanceBinding &ZaaScopedInstanceBinding::operator=(ZaaScopedInstanceBinding &&other) noexcept
{
    if (this != &other) {
        if (m_context != nullptr)
            m_context->deactivate_instance();
        m_context = std::exchange(other.m_context, nullptr);
    }
    return *this;
}

} // namespace Slic3r
