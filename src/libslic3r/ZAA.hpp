#ifndef slic3r_ZAA_hpp_
#define slic3r_ZAA_hpp_

#include "Exception.hpp"
#include "Point.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace Slic3r {

enum ExtrusionRole : uint8_t;
class AABBMesh;
class ExtrusionPath;
class ExtrusionPath3;
class Polyline;
class PrintObject;
enum class ZaaPathPolicy : unsigned char;

inline constexpr double ZAA_MAX_SAMPLE_SPACING_MM = 0.1;
// Physical reference from the ZAA paper. It is the automatic request, not a
// hard lower validity bound for the still-coupled slice-plane parameter.
inline constexpr double ZAA_MIN_DEPOSITABLE_LAYER_THICKNESS_MM = 0.05;
// `zaa_slice_plane_offset = 0` requests this offset automatically. A positive configured
// value is instead validated against the prepared object layer schedule.
inline constexpr double ZAA_DEFAULT_SLICE_PLANE_OFFSET_MM = ZAA_MIN_DEPOSITABLE_LAYER_THICKNESS_MM;
// Keep the common slice plane strictly inside the thinnest physical layer.
// This is an ownership/topology guard, independent of deposition thickness.
inline constexpr double ZAA_SLICE_PLANE_TOP_CLEARANCE_MM = 0.01;
// A valid, top-facing surface hit may be projected to the closest legal layer
// boundary by at most 30 micrometres. This is a geometry policy, not a floating
// point epsilon: it keeps near-boundary mesh/sampling jitter spatial while still
// rejecting hits that are materially outside the current layer's print envelope.
inline constexpr double ZAA_MAX_BOUNDARY_PROJECTION_MM = 0.03;
inline constexpr double ZAA_HIT_EQUIVALENCE_MM          = 0.0001;
inline constexpr double ZAA_DISTANCE_TIE_MM         = 0.0001;
inline constexpr double ZAA_FACING_COS_EPSILON      = 0.000001;
inline constexpr double ZAA_TRANSFORM_KAPPA_MAX     = 1'000'000.0;

enum class ZaaSlicePlaneMode : uint8_t {
    ConventionalMidplane,
    ZaaOffsetPlane
};

struct ZaaPreparedLayerInterval {
    size_t layer_index{0};
    double object_z_lower_mm{0.0};
    double object_z_upper_mm{0.0};
    double outer_wall_width_mm{0.0};
};

struct ZaaLayerGeometry {
    size_t layer_index{0};
    double object_z_lower_mm{0.0};
    double object_z_upper_mm{0.0};
    double slice_z_mm{0.0};
    double query_lower_z_mm{0.0};
    double query_upper_z_mm{0.0};
    double outer_wall_width_mm{0.0};
    // The first prepared object layer is always conventional so it cannot
    // collide with the build plate. Only offset-plane layers enter ZAA planning.
    ZaaSlicePlaneMode slice_plane_mode{ZaaSlicePlaneMode::ZaaOffsetPlane};

    bool uses_zaa_offset_plane() const { return slice_plane_mode == ZaaSlicePlaneMode::ZaaOffsetPlane; }
    double nominal_z_mm(double object_print_z_min = 0.0) const;
    double min_delta_mm() const;
    double max_delta_mm() const;
};

struct ZaaObjectGeometryPlan {
    // Object-level summary: Conventional only when no layer uses ZAA. Per-layer
    // mode is authoritative for geometry, path planning, and G-code emission.
    ZaaSlicePlaneMode mode{ZaaSlicePlaneMode::ZaaOffsetPlane};
    double slice_plane_offset_mm{0.0};
    std::vector<ZaaLayerGeometry> layers;

    const ZaaLayerGeometry *layer(size_t layer_index) const;
    bool uses_zaa_offset_plane(size_t layer_index) const;
    size_t zaa_offset_layer_count() const;
};

enum class ZaaIncompatibilityReason : uint8_t {
    RaftEnabled,
    ScarfEnabled,
    SpiralEnabled,
    IroningEnabled,
    NegativeVolume,
    GeometricModifier,
    UnknownVolumeType,
    MissingModelPart,
    EmptyQueryMesh,
    NonFiniteQueryMesh,
    NonFiniteTransform,
    SingularTransform,
    IllConditionedTransform,
    SlicePlaneOffsetUnrepresentable
};

const char *to_string(ZaaIncompatibilityReason reason);

struct ZaaCapabilityFacts {
    bool enabled{false};
    // Object-level `zaa_slice_plane_offset`, in mm. Zero requests automatic selection.
    double requested_slice_plane_offset_mm{ZAA_DEFAULT_SLICE_PLANE_OFFSET_MM};
    bool has_raft{false};
    bool has_scarf{false};
    bool spiral_mode{false};
    bool ironing_enabled{false};
    bool has_negative_volume{false};
    bool has_geometric_modifier{false};
    bool has_unknown_volume_type{false};
    bool has_model_part{true};
    bool query_mesh_empty{false};
    bool query_mesh_finite{true};
    bool transforms_finite{true};
    bool transforms_invertible{true};
    bool transforms_well_conditioned{true};
};

// Produced only for an explicit positive `zaa_slice_plane_offset` that exceeds the limit
// derived from this object's prepared layer schedule. It must not be confused
// with an ordinary ZAA incompatibility, which can safely fall back to 2D.
struct ZaaSlicePlaneOffsetViolation {
    double requested_offset_mm{0.0};
    double maximum_offset_mm{0.0};
    double min_layer_height_mm{0.0};
    double top_clearance_mm{ZAA_SLICE_PLANE_TOP_CLEARANCE_MM};
};

enum class ZaaObjectDecisionKind : uint8_t {
    Disabled,
    Incompatible,
    SlicePlaneOffsetLimitExceeded,
    Supported
};

class ZaaObjectSliceDecision {
public:
    static ZaaObjectSliceDecision disabled();
    static ZaaObjectSliceDecision incompatible(ZaaIncompatibilityReason reason);
    static ZaaObjectSliceDecision slice_plane_offset_limit_exceeded(ZaaSlicePlaneOffsetViolation violation);
    static ZaaObjectSliceDecision supported(ZaaObjectGeometryPlan plan);

    ZaaObjectDecisionKind kind() const { return m_kind; }
    bool is_disabled() const { return m_kind == ZaaObjectDecisionKind::Disabled; }
    bool is_incompatible() const { return m_kind == ZaaObjectDecisionKind::Incompatible; }
    bool is_slice_plane_offset_limit_exceeded() const
        { return m_kind == ZaaObjectDecisionKind::SlicePlaneOffsetLimitExceeded; }
    bool is_supported() const { return m_kind == ZaaObjectDecisionKind::Supported; }
    ZaaSlicePlaneMode slice_plane_mode() const;
    std::optional<ZaaIncompatibilityReason> incompatibility_reason() const { return m_reason; }
    const ZaaSlicePlaneOffsetViolation *slice_plane_offset_violation() const;
    const ZaaObjectGeometryPlan *geometry_plan() const;

private:
    ZaaObjectDecisionKind m_kind{ZaaObjectDecisionKind::Disabled};
    std::optional<ZaaIncompatibilityReason> m_reason;
    std::optional<ZaaSlicePlaneOffsetViolation> m_slice_plane_offset_violation;
    std::optional<ZaaObjectGeometryPlan> m_plan;
};

struct ZaaSlicePlaneOffsetObjectViolation {
    std::string object_name;
    ZaaSlicePlaneOffsetViolation limit;
};

// A non-critical, user-correctable slicing error. The GUI presents it as a
// yellow blocking warning instead of treating it as a planar-fallback case.
class ZaaSlicePlaneOffsetLimitError : public SlicingError {
public:
    ZaaSlicePlaneOffsetLimitError(
        std::string message,
        std::vector<ZaaSlicePlaneOffsetObjectViolation> violations);

    const std::vector<ZaaSlicePlaneOffsetObjectViolation> &violations() const
        { return m_violations; }

private:
    std::vector<ZaaSlicePlaneOffsetObjectViolation> m_violations;
};

ZaaObjectSliceDecision make_zaa_object_slice_decision(
    const ZaaCapabilityFacts &facts,
    const std::vector<ZaaPreparedLayerInterval> &schedule);

enum class ZaaPlanarReason : uint8_t {
    NoHit,
    AmbiguousHit,
    WrongFacing,
    GrazingSurface,
    OutOfRange
};

const char *to_string(ZaaPlanarReason reason);

enum class ZaaDisplacementSource : uint8_t {
    SurfaceHit,
    BoundaryProjected
};

struct ZaaDisplaced {
    double delta_mm{0.0};
    ZaaDisplacementSource source{ZaaDisplacementSource::SurfaceHit};
};

struct ZaaPlanar {
    ZaaPlanarReason reason{ZaaPlanarReason::NoHit};
};

class ZaaPointOutcome {
public:
    static ZaaPointOutcome displaced_by(double delta_mm);
    // `delta_mm` must already be one of the current layer's legal boundaries.
    // The separate source preserves why a spatial point was clamped without
    // turning the public outcome into another planar/3D sum-type branch.
    static ZaaPointOutcome boundary_projected_to(double delta_mm);
    static ZaaPointOutcome planar(ZaaPlanarReason reason);

    bool is_displaced() const;
    const ZaaDisplaced *displaced() const;
    const ZaaPlanar *planar_result() const;

private:
    explicit ZaaPointOutcome(std::variant<ZaaDisplaced, ZaaPlanar> value) : m_value(std::move(value)) {}
    std::variant<ZaaDisplaced, ZaaPlanar> m_value{ZaaPlanar{}};
};

struct ZaaEmissionSegmentPlan {
    size_t source_start_point_index{0};
    size_t source_end_point_index{0};
    size_t target_point_index{0};
    double length_xy_mm{0.0};
    double length_3d_emitted_mm{0.0};
    double delta_z_emitted_mm{0.0};
    double volume_mm3{0.0};
    double dE{0.0};
    double q3d_mm2{0.0};
    double z_ratio{0.0};
};

struct ZaaFlowReference {
    double width_mm{0.0};
    double height_mm{0.0};
    bool bridge{false};
};

struct ZaaMaterialMotionPlan {
    double reference_height_mm{0.0};
    double q_nominal_mm2{0.0};
    double q_effective_mm2{0.0};
    double q3d_max_mm2{0.0};
    double z_ratio_max{0.0};
    std::vector<ZaaEmissionSegmentPlan> segments;
};

struct ZaaInstanceKey {
    const PrintObject *print_object{nullptr};
    size_t instance_index{0};

    bool operator==(const ZaaInstanceKey &rhs) const
    {
        return print_object == rhs.print_object && instance_index == rhs.instance_index;
    }
};

struct ZaaInstanceContext {
    ZaaInstanceKey key;
    std::string model_object_id;
    std::string model_instance_id;
};

enum class ZaaRoleCapability : uint8_t {
    ExcludedKeepPlanar,
    EligibleScalarMotion
};

ZaaRoleCapability zaa_role_capability(ExtrusionRole role);
bool zaa_role_is_eligible(ExtrusionRole role);

enum class ZaaPathRouting : uint8_t {
    Eligible,
    ExcludedKeepPlanar
};

inline constexpr size_t ZAA_INCOMPATIBILITY_REASON_COUNT = 14;

// Immutable input and thread-private result for one frozen 2D candidate.
// The result owns a sidecar only when quantized Z differs from the nominal layer Z.
struct ZaaPathPlanRequest {
    const ExtrusionPath *source_path{nullptr};
    const ZaaLayerGeometry *layer_geometry{nullptr};
    const AABBMesh *query_mesh{nullptr};
    double layer_print_z_mm{0.0};
    double minimize_perimeter_height_angle_degrees{0.0};
    double max_sample_spacing_mm{ZAA_MAX_SAMPLE_SPACING_MM};
};

struct ZaaPathPlanBatchCallbacks {
    // Both callbacks may be invoked concurrently. They must remain valid only
    // until the synchronous batch call returns.
    std::function<void(size_t)> samples_resolved;
    std::function<void()> throw_if_canceled;
};

struct ZaaPathPlanResult {
    ZaaPathPlanResult();
    ~ZaaPathPlanResult();
    ZaaPathPlanResult(const ZaaPathPlanResult &) = delete;
    ZaaPathPlanResult &operator=(const ZaaPathPlanResult &) = delete;
    ZaaPathPlanResult(ZaaPathPlanResult &&) noexcept;
    ZaaPathPlanResult &operator=(ZaaPathPlanResult &&) noexcept;

    // A spatial result owns the already-prepared 2D mirror and its 3D sidecar.
    // A planar result owns neither until exchange_with() captures prior state
    // for a possible transaction rollback.
    std::unique_ptr<Polyline> prepared_polyline;
    std::unique_ptr<ExtrusionPath3> path3;

    bool is_spatial() const noexcept { return prepared_polyline != nullptr; }
    void exchange_with(ExtrusionPath &target) noexcept;
};

size_t zaa_path_sample_count(const ExtrusionPath &path, double max_sample_spacing_mm);
ZaaPathPlanResult build_zaa_path_plan(const ZaaPathPlanRequest &request);
// Results preserve request order. The call is synchronous and retains none of
// the non-owning request pointers or callbacks after returning.
std::vector<ZaaPathPlanResult> build_zaa_path_plans(
    const std::vector<ZaaPathPlanRequest> &requests,
    const ZaaPathPlanBatchCallbacks *callbacks = nullptr);

enum class ZaaDiagnosticFallbackScope : uint8_t {
    None,
    ObjectConventionalLayers
};

const char *to_string(ZaaPathRouting routing);
const char *to_string(ZaaDiagnosticFallbackScope scope);

enum class ZaaInvariantPhase : uint8_t {
    ProfileBuild,
    WriterReady
};

enum class ZaaInvariantReason : uint8_t {
    ProfileStateViolation,
    PathPolicyViolation,
    NonFiniteValue,
    EmittedSegmentCollapsed,
    MissingExtruder,
    ExtruderChanged,
    PositionUnknown,
    NonFiniteWriterPosition,
    ActiveLift,
    PendingLift,
    ExtruderRetracted,
    RestartExtraPending,
    PositionMismatch
};

struct ZaaSpatialPathStateFailure {
    bool expected_path3_present{false};
    bool actual_path3_present{false};
    std::optional<size_t> path3_point_count;
    std::string failing_check;
};

struct ZaaPathPolicyFailure {
    ZaaPathPolicy expected_path_policy{};
    ZaaPathPolicy actual_path_policy{};
    bool arc_fit_present{false};
};

struct ZaaNonFiniteFailure {
    std::string value_field;
    double actual_value{0.0};
    std::optional<size_t> point_index;
    std::optional<size_t> segment_index;
};

struct ZaaEmittedSegmentCollapsedFailure {
    size_t segment_index{0};
    Vec3d input_start_xyz{Vec3d::Zero()};
    Vec3d input_end_xyz{Vec3d::Zero()};
    Vec3d emitted_start_xyz{Vec3d::Zero()};
    Vec3d emitted_end_xyz{Vec3d::Zero()};
};

struct ZaaWriterReadyState {
    bool writer_extruder_present{false};
    std::optional<unsigned int> actual_extruder_id;
    unsigned int expected_extruder_id{0};
    bool position_known{false};
    Vec3d writer_position{Vec3d::Zero()};
    Vec3d expected_position{Vec3d::Zero()};
    Vec3d writer_emitted_position{Vec3d::Zero()};
    Vec3d expected_emitted_position{Vec3d::Zero()};
    double writer_nominal_z_mm{0.0};
    double active_lift_mm{0.0};
    double pending_lift_mm{0.0};
    std::optional<double> retracted_mm;
    std::optional<double> restart_extra_mm;
};

using ZaaInvariantDetails = std::variant<
    std::monostate,
    ZaaSpatialPathStateFailure,
    ZaaPathPolicyFailure,
    ZaaNonFiniteFailure,
    ZaaEmittedSegmentCollapsedFailure,
    ZaaWriterReadyState>;

struct ZaaInvariantFailure {
    ZaaInvariantPhase phase{ZaaInvariantPhase::ProfileBuild};
    ZaaInvariantReason reason{ZaaInvariantReason::ProfileStateViolation};
    std::string model_object_id;
    std::string model_instance_id;
    std::optional<size_t> layer_id;
    std::optional<double> layer_print_z_mm;
    std::string role;
    std::optional<size_t> path_ordinal;
    ZaaInvariantDetails details;
};

// Material deliberately discarded when an entire sub-resolution path is skipped.
struct ZaaSkippedMicroPath {
    double length_xy_mm{0.0};
    double length_3d_mm{0.0};
    double volume_mm3{0.0};
    double dE{0.0};
};

struct ZaaMaterialMotionPlanResult {
    // Exactly one outcome is populated. A skipped path must not reach the writer.
    std::optional<ZaaMaterialMotionPlan> plan;
    std::optional<ZaaInvariantFailure> failure;
    std::optional<ZaaSkippedMicroPath> skipped_micro_path;
};

const char *to_string(ZaaInvariantPhase phase);
const char *to_string(ZaaInvariantReason reason);
std::string zaa_invariant_code(const ZaaInvariantFailure &failure);
std::string serialize_zaa_invariant(const ZaaInvariantFailure &failure);
[[noreturn]] void throw_zaa_invariant(const ZaaInvariantFailure &failure);

ZaaMaterialMotionPlanResult make_zaa_material_motion_plan(
    const ExtrusionPath3 &path3,
    const ZaaLayerGeometry &layer_geometry,
    double nominal_print_z_mm,
    const std::vector<Vec3d> &input_xyz,
    const std::vector<Vec3d> &emitted_xyz,
    const ZaaFlowReference &flow,
    double q_nominal_mm2,
    double e_per_mm3,
    bool force_no_extrusion);

double zaa_max_volumetric_speed_limit(
    double max_volumetric_speed_mm3_s,
    double q3d_max_mm2,
    double filament_flow_ratio);

std::optional<ZaaInvariantFailure> zaa_writer_ready_failure(const ZaaWriterReadyState &state);

class ZaaScopedInstanceBinding;

class ZaaTaskContext {
public:
    struct ObjectDiagnostics {
        size_t task_object_ordinal{0};
        std::string model_object_id;
        ZaaObjectDecisionKind decision{ZaaObjectDecisionKind::Disabled};
        std::optional<ZaaIncompatibilityReason> incompatibility_reason;
        ZaaDiagnosticFallbackScope fallback_scope{ZaaDiagnosticFallbackScope::None};
        uint64_t paths_eligible{0};
        uint64_t paths_excluded{0};
    };

    struct DiagnosticsSummary {
        bool enabled{false};
        uint64_t objects_disabled{0};
        uint64_t objects_incompatible{0};
        uint64_t objects_supported{0};
        std::array<uint64_t, ZAA_INCOMPATIBILITY_REASON_COUNT> incompatibility_reasons{};
        uint64_t paths_eligible{0};
        uint64_t paths_excluded{0};
        std::vector<ObjectDiagnostics> objects;
        uint64_t paths_skipped_micro{0};
        ZaaSkippedMicroPath skipped_micro_totals;
    };

    ZaaTaskContext();
    ~ZaaTaskContext();
    ZaaTaskContext(const ZaaTaskContext &) = delete;
    ZaaTaskContext &operator=(const ZaaTaskContext &) = delete;
    ZaaTaskContext(ZaaTaskContext &&) noexcept;
    ZaaTaskContext &operator=(ZaaTaskContext &&) noexcept;

    void reset();
    void prepare_instance(
        ZaaInstanceKey key,
        std::string model_object_id = {},
        std::string model_instance_id = {});
    ZaaScopedInstanceBinding bind_instance(ZaaInstanceKey key);
    const ZaaInstanceContext *current_instance() const;

    void record_object_decision(
        const PrintObject *print_object,
        std::string model_object_id,
        const ZaaObjectSliceDecision &decision);
    size_t next_path_ordinal(size_t layer_id) const;
    size_t begin_path(
        ZaaPathRouting routing,
        size_t layer_id,
        double layer_print_z_mm,
        std::string role);
    void record_skipped_micro_path(size_t layer_id, size_t path_ordinal, const ZaaSkippedMicroPath &skipped);
    void emit_summary() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;

    void activate_instance(ZaaInstanceKey key);
    void deactivate_instance();
    friend class ZaaScopedInstanceBinding;
};

class ZaaScopedInstanceBinding {
public:
    ~ZaaScopedInstanceBinding();
    ZaaScopedInstanceBinding(const ZaaScopedInstanceBinding &) = delete;
    ZaaScopedInstanceBinding &operator=(const ZaaScopedInstanceBinding &) = delete;
    ZaaScopedInstanceBinding(ZaaScopedInstanceBinding &&other) noexcept;
    ZaaScopedInstanceBinding &operator=(ZaaScopedInstanceBinding &&other) noexcept;

private:
    explicit ZaaScopedInstanceBinding(ZaaTaskContext &context, ZaaInstanceKey key);
    ZaaTaskContext *m_context{nullptr};
    friend class ZaaTaskContext;
};

} // namespace Slic3r

#endif // slic3r_ZAA_hpp_
