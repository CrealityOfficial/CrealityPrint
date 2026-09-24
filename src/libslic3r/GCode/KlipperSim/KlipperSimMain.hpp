#ifndef slic3r_GCode_KlipperSim_KlipperSimMain_hpp_
#define slic3r_GCode_KlipperSim_KlipperSimMain_hpp_

// ---------------------------------------------------------------------------
// KlipperSimMain �?full-pipeline driver
// ---------------------------------------------------------------------------
// gcode moves -> KlipperMoveQueue (lookahead) -> trapq -> itersolve (CoreXY A/B,
// Extruder) -> stepcompress -> per-MCU shared pool occupancy (steppersync).
// Detects "Move queue overflow" per MCU, mirroring firmware.
// ---------------------------------------------------------------------------

#include <string>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>
#include "KlipperToolhead.hpp"
#include "KlipperHostDispatch.hpp"

namespace Slic3r {
namespace KlipperSim {

inline constexpr int kPathologicalProbeSchemaVersion = 1;

// ===========================================================================
// SimConfig �?machine parameters for the ported Klipper motion pipeline.
//
// 默认�?= 当前机型（i7 CoreXY，Nozzle_mcu=GD32F303），来自�?printer.cfg�?
// 换机型时按下表填写。每个参数标注了【重要度】和【来源】：
//
//   【必须准确】错了会直接导致占用/命中误判�?
//     - mcu_total / nozzle_total   MCU move 池槽位数（占用率的分母，最关键�?
//     - xy_step_dist / e_step_dist 步距（决定每 mm 产生多少 step→命令数�?
//     - kinematics 构型（CoreXY �?x±y；Cartesian 需�?itersolve 回调�?
//   【尽量准确】影响趋势，容差较大�?
//     - pressure_advance           E 在加减速区的额外挤出步；PA 有无差别明显�?
//                                  0.03 vs 0.035 影响�?
//   【无需按机型改】Klipper 全局默认或数学上会约掉：
//     - mcu_freq                   仅用于秒→tick 量化；压缩判据里�?max_error
//                                  同乘同除会约掉，取足够大的值即可，不影响结�?
//     - max_stepper_error          Klipper 全局默认 0.000025s，所有机器一�?
//     - buffer_time_high           Klipper 固件默认 3.0s，几乎所有机器一�?
//     - pa_smooth_time             PA 平滑窗口默认 0.040s
// ===========================================================================
struct SimConfig
{
    // Capability metadata is deliberately opt-in.  The numerical defaults
    // below remain useful for low-level simulator tests, but production probe
    // creation must pass validate_pathological_probe_config().// ---- 【必须准确】换机型必改：错了直接导致占�?命中误判 ----------------

    // 步距 = rotation_distance / (microsteps × full_steps_per_rotation)�?
    // CoreXY：电机走 (x+y)/(x−y) 合成坐标，用同一电机步距�?
    // 当前 i7：X/Y rotation_distance=39, microsteps=16, full_steps=200�?
    double xy_step_dist = 39.0 / (16.0 * 200.0);   // = 0.0121875 mm/step
    // 挤出机步距。当�?i7：rotation_distance=4.21, microsteps=16, full_steps=200�?
    double e_step_dist  = 4.21 / (16.0 * 200.0);   // = 0.001315625 mm/step

    // MCU move 队列共享池槽位总数�? 固件 get_config �?move_count）�?
    // 是占用率的分母，最关键。当�?i7：mcu=2960（X+Y 总线），nozzle=1710（E 总线）�?
    // Values advertised by get_config; Klippy subtracts reserved slots from
    // these counts to obtain its host-side steppersync motion limit.
    int    mcu_total    = 2950;
    int    nozzle_total = 1700;
    // Actual firmware move_free_list capacities shared by step/PWM/digital
    // commands. These are the occupancy denominators and overflow limits.
    int    mcu_pool_total    = 2960;
    int    nozzle_pool_total = 1710;
    // request_move_queue_slot() reserved slots derived from printer.cfg and
    // Klipper pin object setup on this machine:
    // - mcu: shared stepper enable + heater_bed pwm + board_fan digital = 3
    // - nozzle_mcu: extruder enable + heater pwm + fanp0 + fan0 + e_fan
    //   + hotend_fan pwm + hotend_fan enable = 7
    int    mcu_reserved_slots = 3;
    int    nozzle_reserved_slots = 7;

    // ---- Steady-state baseline pool occupancy (fraction 0-1) ---------------
    // The MCU move pool is shared by queue_step, queue_pwm_out, and
    // queue_digital_out commands.  During normal printing the host keeps the
    // queues filled ~80% ahead of execution.  The per-layer analysis models
    // only queue_step commands from the current layer, so it misses:
    //  (a) PWM/digital commands (heaters, fans, sensors), and
    //  (b) queue_step commands from previous layers still in flight.
    // These fields estimate the constant baseline pool consumption from
    // unmodeled commands (PWM/digital for fans, heaters, sensors) that we
    // don't simulate explicitly.  The value is added on top of the step-
    // command occupancy derived from the accumulated pipeline events.
    // Cross-layer step-command carryover is already captured by the event
    // history in LineAnalysisState �?these fields do NOT need to cover it.
    // Calibrated from real MQDIAG samples:
    // - MCU needs a small baseline to match the observed ~31% peak on
    //   successful.gcode while raw step-only simulation sits ~19%.
    // - nozzle has a large always-on share from heater/fan/digital commands
    //   on this machine; the raw step-only model is far too low, so we lift
    //   the constant baseline to match the observed nozzle saturation.
    // ---- 【尽量准确】换机型建议填对，容差较�?----------------------------

    // 稳态基线占用率�?-1）。加到事件流峰值之上的恒定背景占用�?
    // 补偿模拟器未显式建模�?PWM/digital 稳态命令（heater/fan/sensor 轮询）�?
    // nozzle_mcu 上尤其显著�?
    double mcu_baseline_frac    = 0.0;
    double nozzle_baseline_frac = 0.0;

    // 挤出机压力提前量（printer.cfg �?pressure_advance）�?
    // 影响加减速区 E 的额外步进；PA 有无差别明显�?.03 vs 0.035 影响小。当�?i7�?.031�?
    double pressure_advance = 0.031;
    // Creality's ENABLE_PRESSURE_ADVANCE controls whether subsequent
    // SET_PRESSURE_ADVANCE commands are accepted. It does not clear the PA
    // value already installed in the extruder kinematics.
    // The slicer-side file simulation starts after Creality's print-preparation
    // macro, which locks out in-file PA changes before SDCARD_PRINT_FILE.
    bool pressure_advance_commands_enabled = false;

    // ---- 【无需按机型改】Klipper 全局默认 / 数学上会约掉，换机型保持不变 --

    // MCU 主频，仅用于秒→tick 量化；在 stepcompress 压缩判据中与 max_error
    // 同乘同除会约掉，不影响命令数/占用结果。取足够大的值即可�?
    double mcu_freq          = 120e6;
    // Current i7 main/nozzle links are UART at 230400 baud. These values feed
    // the deterministic serialqueue model (8N1 => 10 wire bits per byte).
    double mcu_baud          = 230400.0;
    double nozzle_baud       = 230400.0;
    int    mcu_receive_window = 192;
    int    nozzle_receive_window = 192;
    // Klipper 全局默认步进时间误差容差（秒），所有机器一致�?
    double max_stepper_error = 0.000025;
    // host lookahead 提前灌入时间窗（Klipper toolhead 默认 3.0s），决定占用统计窗口宽度�?
    double buffer_time_high  = 3.0;
    // PA 平滑时间窗，Klipper 默认 0.040s�?
    double pa_smooth_time    = 0.040;
    // kinematics/extruder.py check_move() 限制，属于机器配置而非拟合参数�?
    // Creality Klippy derives max_e_accel from the default cross section
    // (4 * nozzle_diameter^2), not the configured max_extrude_cross_section.
    double max_extrude_only_velocity = 30.0;
    double max_extrude_only_accel = 2660.8108036914427;
    double max_z_velocity = 20.0;
    double max_z_accel = 100.0;

    // Enable concurrent A/B/E generation in the production TBB runtime.
    bool parallel_step_generation = true;
    // L2: amortize the parser-to-simulator callback across consecutive moves.
    // This does not merge moves or alter Toolhead/MCU flush boundaries.
    bool microsegment_batching = true;
    size_t microsegment_batch_size = 32;

    ToolheadLimits limits;   // 运动上限�?gcode / SET_VELOCITY_LIMIT 动态获�?
};

// Validate the machine capability contract before constructing a probe.  A
// missing/unknown field is treated as unsupported; callers must not fall back
// to the simulator's test defaults.
inline bool validate_pathological_probe_config(const SimConfig& cfg,
                                               std::string* reason = nullptr)
{
    auto fail = [reason](const char* why) {
        if (reason)
            *reason = why;
        return false;
    };
    const auto finite_positive = [](double value) {
        return std::isfinite(value) && value > 0.0;
    };
    if (!finite_positive(cfg.xy_step_dist) || !finite_positive(cfg.e_step_dist))
        return fail("invalid_step_distance");
    if (cfg.mcu_pool_total <= 0 || cfg.nozzle_pool_total <= 0)
        return fail("invalid_pool_capacity");
    return true;
}

struct SimReport
{
    // per-MCU
    int    mcu_peak = 0,    nozzle_peak = 0;
    double mcu_peak_t = 0,  nozzle_peak_t = 0;
    bool   mcu_overflow = false, nozzle_overflow = false;
    size_t total_moves = 0;
    size_t xy_cmds = 0, e_cmds = 0;
    // the print-time (s) where the first overflow occurs, -1 if none
    double first_overflow_time = -1.0;
    std::string first_overflow_mcu;
    double last_positive_extrusion_cruise = 0.0;
    double nozzle_avg_dur=0, nozzle_max_dur=0, mcu_avg_dur=0, mcu_max_dur=0;
    double print_time_total=0;
    std::vector<std::pair<double,int>> nozzle_samples, mcu_samples;
    // Physical shared-pool totals used as occupancy denominators.
    int nozzle_total=0, mcu_total=0;
    // Host steppersync motion limits after subtracting reserved slots.
    int nozzle_host_slots=0, mcu_host_slots=0;
    // Separate legal host-timing envelope; never replaces nominal fields.
    int nozzle_late_batch_peak = 0;
    bool nozzle_late_batch_overflow = false;
    double nozzle_late_batch_peak_t = 0.0;
    double nozzle_late_batch_source_t = 0.0;
    int nozzle_late_batch_first_line = -1;
    int nozzle_late_batch_last_line = -1;
    size_t nozzle_late_batch_commands = 0;
    int nozzle_physical_upper_peak = 0;
    bool nozzle_physical_upper_saturated = false;
    size_t toolhead_batch_count = 0;
    size_t toolhead_max_batch_moves = 0;
    size_t toolhead_max_batch_short_moves = 0;
    double toolhead_max_batch_span = 0.0;
    double toolhead_max_batch_start = 0.0;
    int toolhead_max_batch_first_line = -1;
    int toolhead_max_batch_last_line = -1;
};

// Run the full simulation on a sequence of parsed gcode moves.
// Each GMove is one G1: absolute target xyze + requested speed (mm/s).
struct GMove {
    double x,y,z,e;
    double v;
    double accel=10000.0;
    double accel_to_decel=5000.0;
    double square_corner_velocity=8.0;
    double pressure_advance=0.031;
    double pa_smooth_time=0.040;
    int    line_idx = -1;
    // Legacy/offline parser adapter: a non-negative value requests the same
    // non-waiting scan-time flush immediately before this move.
    double step_generation_scan_time_before = -1.0;
};

// Klipper expands G2/G3 into G1 moves before they enter the toolhead.
enum class ArcPlane { XY, XZ, YZ };

inline std::vector<std::array<double, 3>> expand_arc_moves(
    const std::array<double, 3>& current,
    const std::array<double, 3>& target,
    double offset_first,
    double offset_second,
    bool clockwise,
    ArcPlane plane,
    double mm_per_arc_segment = 1.0)
{
    const std::array<int, 3> axes = plane == ArcPlane::XY ? std::array<int, 3>{0, 1, 2}
                                  : plane == ArcPlane::XZ ? std::array<int, 3>{0, 2, 1}
                                                           : std::array<int, 3>{1, 2, 0};
    const int alpha = axes[0], beta = axes[1], helical = axes[2];
    const double r_p = -offset_first, r_q = -offset_second;
    const double center_p = current[alpha] - r_p, center_q = current[beta] - r_q;
    const double rt_alpha = target[alpha] - center_p, rt_beta = target[beta] - center_q;
    double angular_travel = std::atan2(r_p * rt_beta - r_q * rt_alpha,
                                       r_p * rt_alpha + r_q * rt_beta);
    constexpr double pi = 3.14159265358979323846;
    if (angular_travel < 0.0)
        angular_travel += 2.0 * pi;
    if (clockwise)
        angular_travel -= 2.0 * pi;
    if (angular_travel == 0.0 && current[alpha] == target[alpha]
        && current[beta] == target[beta])
        angular_travel = 2.0 * pi;

    const double linear_travel = target[helical] - current[helical];
    const double flat_mm = std::hypot(r_p, r_q) * angular_travel;
    const double mm_of_travel = linear_travel != 0.0
        ? std::hypot(flat_mm, linear_travel) : std::fabs(flat_mm);
    const size_t segments = std::max<size_t>(1, static_cast<size_t>(std::floor(
        mm_of_travel / mm_per_arc_segment)));
    std::vector<std::array<double, 3>> coords;
    coords.reserve(segments);
    const double theta_per_segment = angular_travel / segments;
    const double linear_per_segment = linear_travel / segments;
    for (size_t i = 1; i < segments; ++i) {
        const double theta = i * theta_per_segment;
        std::array<double, 3> coord{};
        coord[alpha] = center_p - offset_first * std::cos(theta)
            + offset_second * std::sin(theta);
        coord[beta] = center_q - offset_first * std::sin(theta)
            - offset_second * std::cos(theta);
        coord[helical] = current[helical] + i * linear_per_segment;
        coords.push_back(coord);
    }
    coords.push_back(target);
    return coords;
}

enum class GCodeAuxKind {
    NozzleFan,
    NozzleHeater,
    BedHeater
};

struct GCodeAuxEvent
{
    GCodeAuxKind kind = GCodeAuxKind::NozzleFan;
    int          line_idx = -1;
    bool         active = false;
    double       value = 0.0;
};

struct AuxDispatchState
{
    bool   nozzle_heater_active = false;
    bool   bed_heater_active    = false;
    double next_nozzle_pwm_t    = 0.0;
    double next_bed_pwm_t       = 0.0;
};

// Append auxiliary fan/heater commands with the same enqueue and MCU-slot
// semantics used by whole-file and generation-time streaming analysis.
AuxDispatchState append_aux_dispatch_cmds(
    const std::vector<GCodeAuxEvent>& aux_events,
    double segment_end_t,
    double buffer_time_high,
    double serialqueue_advance_window,
    double mcu_freq,
    const std::function<double(int)>& line_exec_time,
    std::vector<HostDispatchCmd>& mcu_cmds,
    std::vector<HostDispatchCmd>& nozzle_cmds,
    AuxDispatchState state = {});

struct ParsedGCode
{
    std::array<double, 4> initial_position{0.0, 0.0, 0.0, 0.0};
    bool have_initial_position = false;
    std::vector<GMove> moves;
    std::vector<GCodeAuxEvent> aux_events;
};

SimReport simulate_full(const std::vector<GMove>& gmoves, const SimConfig& cfg);
SimReport simulate_full(const ParsedGCode& gcode, const SimConfig& cfg);

// Scan the accumulated events from a cross-layer LineAnalysisState and
// return the peak pool occupancy (0..1) for the given MCU.  Used for
// end-to-end audit �?feeds layers through analyze_gcode_lines (same code
// path as the engine) then reads the final occupancy curve.
double compute_peak_occupancy(const std::vector<std::pair<double,int>>& events,
                              int total);

// Parse a gcode file into GMove list (handles G1/G0, F, G92, SET_VELOCITY_LIMIT).
std::vector<GMove> parse_gcode(const std::string& path, size_t line_lo=0, size_t line_hi=~size_t(0));
ParsedGCode parse_gcode_detailed(const std::string& path, size_t line_lo=0, size_t line_hi=~size_t(0));

// ---------------------------------------------------------------------------
// Line-level analysis for filter integration.
// Given the lines of a gcode fragment (one layer), run the full firmware
// pipeline and return, per input line, whether that line's motion contributes
// to an MCU move-queue occupancy at/above `threshold` (fraction of pool).
// e_relative / current state are carried in/out so multi-layer calls chain.
// ---------------------------------------------------------------------------
struct LineAnalysisState
{
    double x=0, y=0, z=0, e_abs=0;
    // Logical-to-machine offsets maintained by G92.  Motion planning consumes
    // machine coordinates while the parser continues to resolve absolute and
    // relative words in the active G-code coordinate system.
    double x_offset=0, y_offset=0, z_offset=0;
    double feed=100.0;         // mm/s
    bool   e_relative=false;
    bool   xyz_relative=false;
    ArcPlane arc_plane=ArcPlane::XY;
    bool   have_pos=false;
    double requested_accel_to_decel = 5000.0;
    double current_accel = 10000.0;
    double current_accel_to_decel = 5000.0;
    double current_square_corner_velocity = 8.0;
    double current_pressure_advance = 0.031;
    double current_pa_smooth_time = 0.040;
    bool   pressure_advance_enabled = true;
    bool   simulation_config_initialized = false;
    bool   extruder_heater_active = false;
    bool   bed_heater_active = false;
    double next_extruder_pwm_time = 0.0;
    double next_bed_pwm_time = 0.0;

    // ---- Cross-layer occupancy accumulation ----
    // Cumulative print time of all previous layers (seconds). Each new layer's
    // command times are offset by this value so the occupancy curve spans the
    // entire print without restarting from zero each layer.
    double global_time = 0.0;
    double toolhead_print_time = 0.0;
    double toolhead_host_eventtime = 0.0;
    double toolhead_estimated_print_time = 0.0;
    double toolhead_last_kin_flush_time = 0.0;
    double toolhead_last_kin_move_time = 0.0;

    // Pre-sorted occupancy events accumulated from all processed layers.
    // Each event is (print_time, delta): +1 when a pool node is allocated
    // (command sent to MCU), -1 when it is freed (command done executing).
    // These grow across calls; old events before (global_time - 3*buffer_time)
    // are safe to prune with an offset, but we defer that optimization.
    std::vector<std::pair<double,int>> mcu_events;
    std::vector<std::pair<double,int>> noz_events;
};

struct LineAnalysisDebug
{
    double layer_window_start = 0.0;
    double layer_window_end = 0.0;
    double mcu_peak_frac = 0.0;
    double nozzle_peak_frac = 0.0;
    double mcu_peak_time = -1.0;
    double nozzle_peak_time = -1.0;
    double nozzle_late_batch_peak_frac = 0.0;
    double nozzle_late_batch_peak_time = -1.0;
    int nozzle_late_batch_peak_slots = 0;
    int nozzle_late_batch_first_line = -1;
    int nozzle_late_batch_last_line = -1;
    double nozzle_physical_upper_peak_frac = 0.0;
    int nozzle_physical_upper_peak_slots = 0;
    size_t toolhead_batch_count = 0;
    size_t toolhead_max_batch_moves = 0;
    size_t toolhead_max_batch_short_moves = 0;
    double toolhead_max_batch_span = 0.0;
    double toolhead_max_batch_start = 0.0;
    int toolhead_max_batch_first_line = -1;
    int toolhead_max_batch_last_line = -1;
    size_t parsed_moves = 0;
    size_t aux_events = 0;
    size_t xy_trap_moves = 0;
    size_t e_trap_moves = 0;
    size_t mcu_cmds = 0;
    size_t nozzle_cmds = 0;
    size_t new_mcu_events = 0;
    size_t new_nozzle_events = 0;
    std::vector<std::pair<double, double>> mcu_hi_segments;
    std::vector<std::pair<double, double>> nozzle_hi_segments;
};

enum class EarlyRiskSource {
    None,
    MainMcu,
    NozzleMcu
};

struct EarlyRiskResult {
    bool detected = false;
    bool streaming_fast_path = false;
    bool used_reference_fallback = false;
    EarlyRiskSource source = EarlyRiskSource::None;
    double threshold = 0.80;
    double occupancy = 0.0;
    int occupied_slots = 0;
    int total_slots = 0;
    int line_idx = -1;
    double simulated_time = -1.0;
    size_t lines_consumed = 0;
    size_t main_commands_consumed = 0;
    size_t nozzle_commands_consumed = 0;
    size_t main_commands_total = 0;
    size_t nozzle_commands_total = 0;
    double stream_consume_time_ms = 0.0;
    double gcode_parse_dispatch_time_ms = 0.0;
    double move_callback_time_ms = 0.0;
    double toolhead_time_ms = 0.0;
    double barrier_time_ms = 0.0;
    double position_time_ms = 0.0;
    double aux_event_time_ms = 0.0;
    double aux_resolve_time_ms = 0.0;
    double step_prepare_time_ms = 0.0;
    double step_generate_a_time_ms = 0.0;
    double step_generate_b_time_ms = 0.0;
    double step_generate_e_time_ms = 0.0;
    double step_generation_wall_time_ms = 0.0;
    size_t step_generation_invocations = 0;
    size_t parallel_generation_invocations = 0;
    size_t serial_generation_invocations = 0;
    size_t parallel_phase_total = 0;
    size_t serial_phase_total = 0;
    size_t max_generation_phases = 0;
    double step_mcu_check_time_ms = 0.0;
    double step_main_flush_time_ms = 0.0;
    double step_nozzle_flush_time_ms = 0.0;
    double step_publish_time_ms = 0.0;
    double serial_dispatch_time_ms = 0.0;
    double trapq_release_time_ms = 0.0;
    double finalize_time_ms = 0.0;
    bool parallel_step_generation = false;
    bool microsegment_batching = false;
    size_t move_batch_count = 0;
    size_t batched_move_count = 0;
    size_t forced_batch_flush_count = 0;
    size_t max_moves_per_batch = 0;
};

// Returns a vector<char> sized to `lines.size()`: 1 if that line is in a
// pathological (>=threshold occupancy) region, else 0.
std::vector<char> analyze_gcode_lines(const std::vector<std::string>& lines,
                                      const SimConfig& cfg,
                                      double threshold,
                                      LineAnalysisState& state,
                                      LineAnalysisDebug* debug = nullptr,
                                      const std::function<void()>& cancel = {},
                                      bool compact_streaming = false,
                                      std::vector<double>* positive_extrusion_cruise_by_line = nullptr,
                                      std::vector<double>* positive_extrusion_distance_by_line = nullptr,
                                      EarlyRiskResult* early_risk = nullptr,
                                      bool streaming_early_probe = true);

// ---------------------------------------------------------------------------
// Region-level occupancy probe (used to SOLVE the accel-reduction amount).
// Runs the full firmware pipeline on `lines` (a flagged pathological region) in
// isolation, using cfg.limits (whose max_accel/max_accel_to_decel encode the
// CANDIDATE accel being tested), and reports the peak per-MCU move-pool
// occupancy as a fraction of the pool (0..1+).
//
// Because the firmware buffer window (buffer_time_high ~3s) is large relative to
// a dense region's own duration, the region's own commands dominate the window
// overlap; simulating in isolation closely matches the in-context peak for the
// dense regions we care about.
//
// The filter adaptively lowers acceleration until the mcu/nozzle peak fraction
// stays below the target or the acceleration floor is reached. This is why
// full porting was needed:
// changing accel here re-plans lookahead -> re-generates steps -> re-compresses
// -> re-derives occupancy (including the low-accel command-inflation effect),
// none of which the old simplified model could predict.
// ---------------------------------------------------------------------------
struct OccResult
{
    double mcu_frac = 0.0;      // peak mcu-bus occupancy / mcu_total
    double nozzle_frac = 0.0;   // peak nozzle-bus occupancy / nozzle_total
    double print_time = 0.0;    // simulated duration of the probe window
    double last_positive_extrusion_cruise = 0.0;
    bool   mcu_overflow = false;
    bool   nozzle_overflow = false;
};

OccResult simulate_lines_occupancy(const std::vector<std::string>& lines,
                                   const SimConfig& cfg,
                                   bool e_relative);
// Borrow immutable source lines to avoid deep-copying each candidate region.
OccResult simulate_line_refs_occupancy(const std::vector<const std::string*>& lines,
                                       const SimConfig& cfg,
                                       bool e_relative);

}} // namespace Slic3r::KlipperSim

#endif
