#ifndef slic3r_GCode_KlipperSim_KlipperDispatchAnalysis_hpp_
#define slic3r_GCode_KlipperSim_KlipperDispatchAnalysis_hpp_

#include <functional>
#include <utility>
#include <vector>

#include "KlipperHostDispatch.hpp"
#include "KlipperStepCompress.hpp"
#include "KlipperSteppersync.hpp"
#include "KlipperTrapQ.hpp"

namespace Slic3r {
namespace KlipperSim {

void append_stepcompress_dispatch_cmds(const KlipperStepCompress& sc,
                                       const std::vector<TrapMove>& phases,
                                       const std::function<bool(const TrapMove&)>& phase_active,
                                       double mcu_freq,
                                       int source_kind,
                                       std::vector<HostDispatchCmd>& out);
void append_stepcompress_dispatch_cmds(const std::vector<StepMoveCmd>& commands,
                                       const std::vector<StepAuxCmd>& aux_commands,
                                       double mcu_freq,
                                       int source_kind,
                                       std::vector<HostDispatchCmd>& out);
void annotate_stepcompress_commands(KlipperStepCompress& sc,
                                    const std::vector<TrapMove>& phases,
                                    const std::function<bool(const TrapMove&)>& phase_active,
                                    double mcu_freq);

void build_dispatch_events(const std::vector<HostDispatchCmd>& cmds,
                           double time_offset,
                           std::vector<std::pair<double, int>>& dst);

OverflowReport analyze_dispatch_cmds(const std::vector<HostDispatchCmd>& cmds,
                                     int total_slots);

}} // namespace Slic3r::KlipperSim

#endif
