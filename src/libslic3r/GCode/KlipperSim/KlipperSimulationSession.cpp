#include "KlipperSimulationSession.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Slic3r {
namespace KlipperSim {

namespace {

double junction_deviation(double square_corner_velocity, double max_accel)
{
    return max_accel > 0.0
        ? square_corner_velocity * square_corner_velocity
          * (std::sqrt(2.0) - 1.0) / max_accel
        : 0.0;
}

} // namespace

struct KlipperSimulationSession::Impl
{
    SimConfig cfg;
    KlipperTrapQ xy_tq;
    KlipperTrapQ e_tq;
    Coord xy_pos{0, 0, 0};
    Coord e_pos{0, 0, 0};
    std::array<double, 4> current{0, 0, 0, 0};
    bool seeded = false;
    bool finished = false;
    size_t moves = 0;
    size_t barriers = 0;
    size_t process_batches = 0;
    size_t max_batch_moves = 0;
    size_t max_batch_short_moves = 0;
    double max_batch_span = 0.0;
    double max_batch_start = 0.0;
    int max_batch_first_line = -1;
    int max_batch_last_line = -1;
    std::vector<double> line_starts;
    std::vector<double> line_ends;
    std::vector<PlanMove> planned;
    bool capture_planned = false;
    std::unique_ptr<KlipperHostToolhead> host;

    Impl(const SimConfig& in_cfg,
         const std::array<double, 4>& start_position,
         bool have_position,
         size_t line_count,
         EstimatedPrintTimeCb estimated_cb,
         ToolheadFlushCb flush_cb,
         SessionStepGenerationCb step_generation_cb,
         bool capture_planned_moves,
         std::vector<double>* positive_extrusion_cruise_by_line,
         std::vector<double>* positive_extrusion_distance_by_line,
         double* last_positive_extrusion_cruise,
         SessionPlannedBatchCb planned_batch_cb)
        : cfg(in_cfg)
        , current(start_position)
        , seeded(have_position)
        , line_starts(line_count, -1.0)
        , line_ends(line_count, -1.0)
        , capture_planned(capture_planned_moves)
    {
        ToolheadTiming timing;
        timing.buffer_time_high = cfg.buffer_time_high;
        // ExtruderStepper connect-time setup registers half of the active PA
        // smoothing window as a step-generation scan delay.
        timing.kin_flush_delay = std::max(
            timing.kin_flush_delay,
            cfg.pressure_advance > 0.0 ? cfg.pa_smooth_time * 0.5 : 0.0);
        auto process = [this, positive_extrusion_cruise_by_line,
                        positive_extrusion_distance_by_line,
                        last_positive_extrusion_cruise,
                        planned_batch_cb = std::move(planned_batch_cb)](const std::vector<PlanMove>& batch, double batch_start) {
            double move_time = batch_start;
            size_t short_moves = 0;
            for (const PlanMove& move : batch) {
                const bool positive_spatial_extrusion = move.axes_d[3] > 1e-12
                    && (std::fabs(move.axes_d[0]) > 1e-12
                        || std::fabs(move.axes_d[1]) > 1e-12
                        || std::fabs(move.axes_d[2]) > 1e-12);
                if (positive_spatial_extrusion) {
                    if (last_positive_extrusion_cruise != nullptr)
                        *last_positive_extrusion_cruise = move.cruise_v;
                    if (move.line_idx >= 0 && positive_extrusion_cruise_by_line != nullptr
                        && static_cast<size_t>(move.line_idx) < positive_extrusion_cruise_by_line->size()) {
                        double& line_cruise = (*positive_extrusion_cruise_by_line)[static_cast<size_t>(move.line_idx)];
                        line_cruise = std::max(line_cruise, move.cruise_v);
                    }
                    if (move.line_idx >= 0 && positive_extrusion_distance_by_line != nullptr
                        && static_cast<size_t>(move.line_idx) < positive_extrusion_distance_by_line->size())
                        (*positive_extrusion_distance_by_line)[static_cast<size_t>(move.line_idx)] += move.move_d;
                }
                if (capture_planned)
                    planned.push_back(move);
                const double move_end = move_time + move.accel_t
                                      + move.cruise_t + move.decel_t;
                if (move_end - move_time < 0.002)
                    ++short_moves;
                if (move.line_idx >= 0
                    && static_cast<size_t>(move.line_idx) < line_starts.size()) {
                    const size_t li = static_cast<size_t>(move.line_idx);
                    if (line_starts[li] < 0.0)
                        line_starts[li] = move_time;
                    line_ends[li] = std::max(line_ends[li], move_end);
                }

                Coord axes_r{move.axes_r[0], move.axes_r[1], move.axes_r[2]};
                xy_tq.append(move_time, batch_start,
                             std::max(0.0, move.accel_t),
                             std::max(0.0, move.cruise_t),
                             std::max(0.0, move.decel_t),
                             xy_pos, axes_r, move.start_v, move.cruise_v,
                             move.accel, move.line_idx);
                xy_pos.x += move.axes_d[0];
                xy_pos.y += move.axes_d[1];
                xy_pos.z += move.axes_d[2];

                const double de = move.axes_d[3];
                const double escale = move.move_d > 1e-12 ? de / move.move_d : 0.0;
                // PrinterExtruder.move(): PA is valid only for positive
                // extrusion accompanied by X or Y motion.
                const double can_pressure_advance =
                    escale > 0.0
                    && (std::fabs(move.axes_d[0]) > 1e-12
                        || std::fabs(move.axes_d[1]) > 1e-12)
                        ? 1.0 : 0.0;
                e_tq.append(move_time, batch_start,
                            std::max(0.0, move.accel_t),
                            std::max(0.0, move.cruise_t),
                            std::max(0.0, move.decel_t),
                            e_pos, Coord{1, can_pressure_advance, 0},
                            move.start_v * escale, move.cruise_v * escale,
                            move.accel * escale, move.line_idx,
                            move.pressure_advance, move.pa_smooth_time);
                e_pos.x += de;
                move_time = move_end;
            }
            if (planned_batch_cb)
                planned_batch_cb(batch, batch_start);
            ++process_batches;
            if (batch.size() > max_batch_moves) {
                max_batch_moves = batch.size();
                max_batch_short_moves = short_moves;
                max_batch_span = move_time - batch_start;
                max_batch_start = batch_start;
                max_batch_first_line = batch.empty() ? -1 : batch.front().line_idx;
                max_batch_last_line = batch.empty() ? -1 : batch.back().line_idx;
            }
        };
        host = std::make_unique<KlipperHostToolhead>(
            cfg.limits, timing, std::move(process),
            std::move(estimated_cb),
            [this, flush_cb = std::move(flush_cb),
             step_generation_cb = std::move(step_generation_cb)](
                const ToolheadFlushWindow& window) {
                if (step_generation_cb)
                    step_generation_cb(window, xy_tq, e_tq);
                if (flush_cb)
                    flush_cb(window);
            });
    }
};

KlipperSimulationSession::KlipperSimulationSession(
    const SimConfig& cfg,
    const std::array<double, 4>& start_position,
    bool have_position,
    size_t line_count,
    EstimatedPrintTimeCb estimated_print_time_cb,
    ToolheadFlushCb flush_cb,
    SessionStepGenerationCb step_generation_cb,
    bool capture_planned_moves,
    std::vector<double>* positive_extrusion_cruise_by_line,
    std::vector<double>* positive_extrusion_distance_by_line,
    double* last_positive_extrusion_cruise,
    SessionPlannedBatchCb planned_batch_cb)
    : m_impl(std::make_unique<Impl>(cfg, start_position, have_position,
                                    line_count,
                                    std::move(estimated_print_time_cb),
                                    std::move(flush_cb),
                                    std::move(step_generation_cb),
                                    capture_planned_moves,
                                    positive_extrusion_cruise_by_line,
                                    positive_extrusion_distance_by_line,
                                    last_positive_extrusion_cruise,
                                    std::move(planned_batch_cb)))
{
}

KlipperSimulationSession::~KlipperSimulationSession() = default;

void KlipperSimulationSession::consume_move(const GMove& move)
{
    Impl& s = *m_impl;
    s.finished = false;
    if (!s.seeded) {
        s.current = {move.x, move.y, move.z, 0.0};
        s.seeded = true;
        return;
    }

    const double end[4] = {move.x, move.y, move.z, s.current[3] + move.e};
    ToolheadLimits limits = s.cfg.limits;
    limits.max_accel = move.accel;
    limits.max_accel_to_decel = move.accel_to_decel;
    limits.square_corner_velocity = move.square_corner_velocity;
    limits.junction_deviation = junction_deviation(
        limits.square_corner_velocity, limits.max_accel);

    PlanMove planned;
    planned.init(limits, s.current.data(), end, move.v);
    // cartesian.py check_move(): any move containing Z is constrained by the
    // configured Z-axis velocity and acceleration before lookahead.
    if (std::fabs(planned.axes_d[2]) > 1e-12) {
        const double z_ratio = planned.move_d / std::fabs(planned.axes_d[2]);
        const double speed_limit = s.cfg.max_z_velocity * z_ratio;
        const double accel_limit = s.cfg.max_z_accel * z_ratio;
        if (speed_limit > 0.0
            && speed_limit * speed_limit < planned.max_cruise_v2) {
            planned.max_cruise_v2 = speed_limit * speed_limit;
            planned.min_move_t = planned.move_d / speed_limit;
        }
        if (accel_limit > 0.0) {
            planned.accel = std::min(planned.accel, accel_limit);
            planned.delta_v2 = 2.0 * planned.move_d * planned.accel;
            planned.smooth_delta_v2 = std::min(planned.smooth_delta_v2,
                                               planned.delta_v2);
        }
    }
    // PrinterExtruder.check_move(): pure E moves and retractions are limited
    // in E-axis units before entering lookahead.  This changes min_move_t and
    // therefore the deterministic lazy-flush boundary as well as move timing.
    const bool has_xy = std::fabs(planned.axes_d[0]) > 1e-12
                     || std::fabs(planned.axes_d[1]) > 1e-12;
    const double e_ratio = planned.axes_r[3];
    if ((!has_xy || e_ratio < 0.0) && std::fabs(e_ratio) > 1e-12) {
        const double inv_e_ratio = 1.0 / std::fabs(e_ratio);
        const double speed_limit = s.cfg.max_extrude_only_velocity
                                 * inv_e_ratio;
        const double accel_limit = s.cfg.max_extrude_only_accel
                                 * inv_e_ratio;
        if (speed_limit > 0.0
            && speed_limit * speed_limit < planned.max_cruise_v2) {
            planned.max_cruise_v2 = speed_limit * speed_limit;
            planned.min_move_t = planned.move_d / speed_limit;
        }
        if (accel_limit > 0.0) {
            planned.accel = std::min(planned.accel, accel_limit);
            planned.delta_v2 = 2.0 * planned.move_d * planned.accel;
            planned.smooth_delta_v2 = std::min(planned.smooth_delta_v2,
                                               planned.delta_v2);
        }
    }
    planned.line_idx = move.line_idx;
    planned.pressure_advance = move.pressure_advance;
    planned.pa_smooth_time = move.pa_smooth_time;
    s.host->update_limits(limits);
    s.host->add_move(planned);
    s.current = {end[0], end[1], end[2], end[3]};
    ++s.moves;
}

void KlipperSimulationSession::set_position(
    const std::array<double, 4>& position, bool have_position)
{
    m_impl->current = position;
    m_impl->seeded = have_position;
}

void KlipperSimulationSession::consume_barrier(const SimulationBarrier& barrier)
{
    Impl& s = *m_impl;
    if (barrier.kind == SimulationBarrierKind::StepGenerationFlush)
        s.host->note_step_generation_scan_time(
            std::max(0.0, barrier.step_generation_scan_time));
    else
        s.host->synchronize(std::max(0.0, barrier.dwell_seconds));
    if (barrier.invalidates_position)
        s.seeded = false;
    ++s.barriers;
    s.finished = true;
}

void KlipperSimulationSession::finish()
{
    if (m_impl->finished)
        return;
    m_impl->host->finish();
    m_impl->finished = true;
}

KlipperTrapQ& KlipperSimulationSession::xy_trapq() { return m_impl->xy_tq; }
KlipperTrapQ& KlipperSimulationSession::e_trapq() { return m_impl->e_tq; }
const KlipperTrapQ& KlipperSimulationSession::xy_trapq() const { return m_impl->xy_tq; }
const KlipperTrapQ& KlipperSimulationSession::e_trapq() const { return m_impl->e_tq; }
KlipperHostToolhead& KlipperSimulationSession::toolhead() { return *m_impl->host; }
const KlipperHostToolhead& KlipperSimulationSession::toolhead() const { return *m_impl->host; }
const std::vector<double>& KlipperSimulationSession::line_start_times() const { return m_impl->line_starts; }
const std::vector<double>& KlipperSimulationSession::line_end_times() const { return m_impl->line_ends; }
const std::vector<PlanMove>& KlipperSimulationSession::planned_moves() const { return m_impl->planned; }
size_t KlipperSimulationSession::move_count() const { return m_impl->moves; }
size_t KlipperSimulationSession::barrier_count() const { return m_impl->barriers; }
size_t KlipperSimulationSession::process_batch_count() const { return m_impl->process_batches; }
size_t KlipperSimulationSession::max_process_batch_moves() const { return m_impl->max_batch_moves; }
size_t KlipperSimulationSession::max_process_batch_short_moves() const { return m_impl->max_batch_short_moves; }
double KlipperSimulationSession::max_process_batch_span() const { return m_impl->max_batch_span; }
double KlipperSimulationSession::max_process_batch_start() const { return m_impl->max_batch_start; }
int KlipperSimulationSession::max_process_batch_first_line() const { return m_impl->max_batch_first_line; }
int KlipperSimulationSession::max_process_batch_last_line() const { return m_impl->max_batch_last_line; }

}} // namespace Slic3r::KlipperSim
