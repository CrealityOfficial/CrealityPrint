#ifndef slic3r_GCode_AppearanceUnderExtrusionAccelRecoveryFilter_hpp_
#define slic3r_GCode_AppearanceUnderExtrusionAccelRecoveryFilter_hpp_

#include "../libslic3r.h"
#include "../ExtrusionEntity.hpp"
#include "../PrintConfig.hpp"
#include "ZaaIntervalProtocol.hpp"

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace Slic3r {

struct LayerResult;

struct AppearanceUnderExtrusionAccelRecoveryConfig
{
    bool enabled{ true };

    // ROI (Trigger/Defect) detection parameters are intentionally not duplicated here.
    // They follow InterestRegion::AppearanceUnderExtrusionDefinition defaults so that:
    // - GUI preview ROI highlight (cached at export time) and
    // - AUE post-processing ROI consumption (this filter)
    // stay consistent.

    // Mitigation:
    // At trigger start, set accel to accel_safe.
    // At defect start, keep the existing (low) speed for L_safe_transition_mm.
    // When transition ends, set cruise speed to velocity_safe_mm_s.
    // Keep accel_safe for L_safe = L_safe_transition_mm + L_safe_accel + L_safe_cruise_mm along the defect path, then recover.
    //
    // Notes on defaults:
    // - accel_safe / velocity_safe_mm_s are set to -1 here intentionally as a "must be provided" sentinel.
    //   They must be filled from PrintObjectConfig (UI / preset / CLI), so that there is no hidden second
    //   source of truth in this struct.
    float        accel_safe{ -1.0f };
    float        velocity_safe_mm_s{ -1.0f };
    float        L_safe_transition_mm{ 1.0f };
    float        L_safe_cruise_mm{ 1.0f };

    // If true, append debug comments starting with "AUE" to the injected commands / split moves.
    bool emit_aue_comments{ true };
};

// A per-layer G-code filter to mitigate appearance under-extrusion when transitioning from
// prolonged low speed to higher speed, by temporarily limiting acceleration and capping speed.
//
// Intended to be placed after PressureEqualizer and before parsing/cooling rewrite.
class AppearanceUnderExtrusionAccelRecoveryFilter
{
public:
    AppearanceUnderExtrusionAccelRecoveryFilter() = delete;
    struct ObjectParams
    {
        bool  enabled{ false };
        float accel_safe{ 0.0f };
        float velocity_safe_mm_s{ 0.0f };
    };

    AppearanceUnderExtrusionAccelRecoveryFilter(const GCodeConfig& config,
                                               const PrintObjectConfig& default_object_config,
                                               std::unordered_map<int, ObjectParams> object_params_by_id,
                                               GCodeFlavor flavor);
    ~AppearanceUnderExtrusionAccelRecoveryFilter();

    AppearanceUnderExtrusionAccelRecoveryConfig&       params() { return m_params; }
    const AppearanceUnderExtrusionAccelRecoveryConfig& params() const { return m_params; }

    void reset();
    LayerResult process_layer(LayerResult&& input);
    LayerResult process_layer(LayerResult&& input, bool layer_end);

private:
    struct State;
    static std::string apply_to_gcode_layer(std::string&& gcode,
                                            const AppearanceUnderExtrusionAccelRecoveryConfig& params,
                                            const GCodeConfig& config,
                                            GCodeFlavor flavor,
                                            const std::unordered_map<int, ObjectParams>& object_params_by_id,
                                            State& state,
                                            ZaaIntervalTracker zaa_interval);

    AppearanceUnderExtrusionAccelRecoveryConfig m_params;
    std::unordered_map<int, ObjectParams> m_object_params_by_id;
    GCodeConfig                                 m_config;
    GCodeFlavor                                 m_flavor;
    std::unique_ptr<State>                      m_state;
};

} // namespace Slic3r

#endif /* slic3r_GCode_AppearanceUnderExtrusionAccelRecoveryFilter_hpp_ */
