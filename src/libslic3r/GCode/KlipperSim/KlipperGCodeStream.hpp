#ifndef slic3r_GCode_KlipperSim_KlipperGCodeStream_hpp_
#define slic3r_GCode_KlipperSim_KlipperGCodeStream_hpp_

#include "KlipperSimulationSession.hpp"

#include <array>
#include <functional>
#include <string>
#include <vector>

namespace Slic3r {
namespace KlipperSim {

struct GCodeStreamCallbacks
{
    std::function<void(const GMove&)> on_move;
    // Optional streaming fast-path callback. Moves remain distinct and ordered;
    // batching only amortizes the callback boundary.
    std::function<bool(const std::vector<GMove>&)> on_move_batch;
    std::function<void(const GCodeAuxEvent&)> on_aux;
    std::function<void(const SimulationBarrier&)> on_barrier;
    std::function<void(const std::array<double, 4>&, bool)> on_position;
};

struct GCodeStreamStats
{
    size_t lines = 0;
    size_t moves = 0;
    size_t aux_events = 0;
    size_t barriers = 0;
    size_t ignored = 0;
    size_t move_batches = 0;
    size_t batched_moves = 0;
    size_t forced_batch_flushes = 0;
    size_t max_moves_per_batch = 0;
};

class KlipperGCodeStream
{
public:
    KlipperGCodeStream(const SimConfig& cfg,
                       LineAnalysisState& state,
                       GCodeStreamCallbacks callbacks);

    void consume_line(const std::string& line, int line_idx);
    bool flush_pending_moves();
    bool stopped() const { return m_stopped; }
    const GCodeStreamStats& stats() const { return m_stats; }

private:
    bool emit_move(const GMove& move);
    bool flush_move_batch(bool forced);

    SimConfig m_cfg;
    LineAnalysisState& m_state;
    GCodeStreamCallbacks m_callbacks;
    GCodeStreamStats m_stats;
    std::vector<GMove> m_move_batch;
    size_t m_move_batch_size = 0;
    bool m_stopped = false;
};

}} // namespace Slic3r::KlipperSim

#endif
