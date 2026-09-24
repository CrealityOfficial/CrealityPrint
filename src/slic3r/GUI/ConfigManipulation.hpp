#ifndef slic3r_ConfigManipulation_hpp_
#define slic3r_ConfigManipulation_hpp_

/*	 Class for validation config options
 *	 and update (enable/disable) IU components
 *	 
 *	 Used for config validation for global config (Print Settings Tab)
 *	 and local config (overrides options on sidebar)
 * */

// The standalone planner regression test defines this macro to compile only
// the value-based helpers below, without pulling the GUI/config dependency
// graph into that small executable. Normal production includes see the full
// ConfigManipulation interface.
#ifndef SLIC3R_CONFIG_MANIPULATION_PLANNER_ONLY
#include "libslic3r/PrintConfig.hpp"
#include "Field.hpp"
#endif

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace Slic3r {

class ModelConfig;
class ObjectBase;

using ModelConfigEntries = std::vector<std::pair<ObjectBase*, ModelConfig*>>;

namespace GUI {

// Pure planning data shared by the process-parameter editors and Plater. These
// helpers intentionally inspect only value snapshots: they never open dialogs
// or mutate live configuration. Keeping them inline here avoids a standalone
// implementation file while preserving direct unit coverage of the decision
// matrix.
enum class ZaaUiChangeScope {
    Global,
    Object,
    Plate
};

// A modal confirmation runs a nested wxWidgets event loop. Option fields write
// their backing config before notifying the tab, so callbacks queued during a
// normalization transaction need three distinct outcomes.
enum class ZaaUiReentryAction {
    ProcessNormally,
    SuppressEcho,
    RejectCompetingEnable
};

inline ZaaUiReentryAction classify_zaa_ui_tab_reentry(bool normalization_in_progress,
                                                       const std::string& active_key,
                                                       ZaaUiChangeScope active_scope,
                                                       const std::string& changed_key,
                                                       ZaaUiChangeScope changed_scope,
                                                       bool changed_feature_enabled)
{
    if (!normalization_in_progress)
        return ZaaUiReentryAction::ProcessNormally;
    if (!changed_feature_enabled || (changed_key == active_key && changed_scope == active_scope))
        return ZaaUiReentryAction::SuppressEcho;
    return ZaaUiReentryAction::RejectCompetingEnable;
}

struct ZaaUiObjectState {
    size_t object_id { 0 };
    bool   in_scope { false };
    bool   zaa_enabled { false };
    bool   scarf_enabled { false };
    bool   spiral_enabled { false };
    bool   object_zaa_enabled { false };
    bool   object_scarf_enabled { false };
    bool   unsupported_scarf_enabled { false };
    // True only when an explicit enabled Plate override contributes Spiral.
    // A global winner needs to close those overrides, but must not create
    // redundant Plate overrides for Spiral inherited from the global config.
    bool   explicit_plate_spiral_enabled { false };
};

struct ZaaUiGlobalState {
    bool zaa_enabled { false };
    bool scarf_enabled { false };
    bool spiral_enabled { false };
    bool mixed_sublayer_enabled { false };
};

struct ZaaUiNormalizationRequest {
    std::string      changed_key;
    ZaaUiChangeScope scope { ZaaUiChangeScope::Global };
    bool             changed_feature_enabled { false };
};

struct ZaaUiNormalizationPlan {
    bool                reject_changed_feature { false };
    bool                disable_global_zaa { false };
    bool                disable_global_scarf { false };
    bool                disable_global_spiral { false };
    bool                disable_global_mixed { false };
    std::vector<size_t> zaa_false_object_ids;
    std::vector<size_t> scarf_none_object_ids;
    std::vector<size_t> spiral_false_object_ids;
    std::vector<size_t> affected_object_ids;
    std::vector<size_t> unsupported_object_ids;

    bool empty() const
    {
        return !reject_changed_feature && !disable_global_zaa && !disable_global_scarf &&
               !disable_global_spiral && !disable_global_mixed && zaa_false_object_ids.empty() &&
               scarf_none_object_ids.empty() && spiral_false_object_ids.empty();
    }
};

inline ZaaUiNormalizationPlan plan_zaa_ui_normalization(const ZaaUiNormalizationRequest& request,
                                                         const std::vector<ZaaUiObjectState>& states,
                                                         const ZaaUiGlobalState& global_state)
{
    ZaaUiNormalizationPlan plan;
    if (!request.changed_feature_enabled)
        return plan;

    const bool is_global = request.scope == ZaaUiChangeScope::Global;
    const bool is_zaa = request.changed_key == "zaa_enabled";
    const bool is_scarf = request.changed_key == "seam_slope_type";
    const bool is_spiral = request.changed_key == "spiral_mode";
    const bool is_mixed = request.changed_key == "enable_mixed_color_sublayer";
    if (!is_zaa && !is_scarf && !is_spiral && !is_mixed)
        return plan;

    if (is_spiral) {
        if (is_global) {
            plan.disable_global_zaa = global_state.zaa_enabled;
            plan.disable_global_scarf = global_state.scarf_enabled;
            plan.disable_global_mixed = global_state.mixed_sublayer_enabled;
        } else {
            plan.disable_global_mixed = global_state.mixed_sublayer_enabled;
        }
    } else if (is_zaa) {
        if (is_global) {
            plan.disable_global_spiral = global_state.spiral_enabled;
            plan.disable_global_scarf = global_state.scarf_enabled;
            plan.disable_global_mixed = global_state.mixed_sublayer_enabled;
        } else {
            plan.disable_global_mixed = global_state.mixed_sublayer_enabled;
        }
    } else if (is_scarf) {
        if (is_global) {
            plan.disable_global_spiral = global_state.spiral_enabled;
            plan.disable_global_zaa = global_state.zaa_enabled;
            plan.disable_global_mixed = global_state.mixed_sublayer_enabled;
        } else {
            plan.disable_global_mixed = global_state.mixed_sublayer_enabled;
        }
    } else {
        plan.disable_global_spiral = global_state.spiral_enabled;
        plan.disable_global_zaa = global_state.zaa_enabled;
        plan.disable_global_scarf = global_state.scarf_enabled;
    }

    const auto append_unique = [](std::vector<size_t>& ids, size_t id) {
        if (std::find(ids.begin(), ids.end(), id) == ids.end())
            ids.emplace_back(id);
    };

    for (const ZaaUiObjectState& state : states) {
        if (!state.in_scope)
            continue;

        bool winner_effective = true;
        if (is_global) {
            if (is_spiral)
                winner_effective = state.spiral_enabled;
            else if (is_zaa)
                winner_effective = state.zaa_enabled;
            else if (is_scarf)
                winner_effective = state.scarf_enabled;
        }
        if (!winner_effective)
            continue;

        bool has_conflict = false;
        const auto disable_zaa = [&] {
            if (!state.zaa_enabled)
                return;
            has_conflict = true;
            if (!is_global || state.object_zaa_enabled)
                append_unique(plan.zaa_false_object_ids, state.object_id);
        };
        const auto disable_scarf = [&] {
            if (!state.scarf_enabled)
                return;
            has_conflict = true;
            if (state.unsupported_scarf_enabled) {
                plan.reject_changed_feature = true;
                append_unique(plan.unsupported_object_ids, state.object_id);
            } else if (!is_global || state.object_scarf_enabled) {
                append_unique(plan.scarf_none_object_ids, state.object_id);
            }
        };
        const auto disable_spiral = [&] {
            if (!state.spiral_enabled)
                return;
            has_conflict = true;
            if (!is_global || state.explicit_plate_spiral_enabled)
                append_unique(plan.spiral_false_object_ids, state.object_id);
        };

        if (is_spiral) {
            disable_zaa();
            disable_scarf();
        } else if (is_zaa) {
            disable_spiral();
            disable_scarf();
        } else if (is_scarf) {
            disable_spiral();
            disable_zaa();
        } else {
            disable_spiral();
            disable_zaa();
            disable_scarf();
        }

        if (has_conflict)
            append_unique(plan.affected_object_ids, state.object_id);
    }

    return plan;
}

// Removing a Plate override may either keep the effective value unchanged or
// enable Spiral by inheriting a true global value. Model the effective values
// explicitly so all Plate entry points preflight only real false-to-true moves.
struct PlateSpiralModeTransition {
    bool effective_before { false };
    bool effective_after { false };

    bool enables_spiral() const { return !effective_before && effective_after; }
};

inline PlateSpiralModeTransition plan_plate_spiral_mode_transition(bool has_before_override,
                                                                    bool before_override,
                                                                    bool has_after_override,
                                                                    bool after_override,
                                                                    bool global_value)
{
    return {
        has_before_override ? before_override : global_value,
        has_after_override ? after_override : global_value
    };
}

#ifndef SLIC3R_CONFIG_MANIPULATION_PLANNER_ONLY
class ConfigManipulation
{
    bool                is_msg_dlg_already_exist{ false };
    bool                m_is_initialized_support_material_overhangs_queried{ false };
    bool                m_support_material_overhangs_queried{ false };
    bool                is_BBL_Printer{false};
    bool                is_CX_Printer{false};

    // function to loading of changed configuration 
    std::function<void()>                                       load_config = nullptr;
    std::function<void (const std::string&, bool toggle, int opt_index)>   cb_toggle_field = nullptr;
    std::function<void (const std::string&, bool toggle)>   cb_toggle_line = nullptr;
    // callback to propagation of changed value, if needed 
    std::function<void(const std::string&, const boost::any&)>  cb_value_change = nullptr;
    //BBS: change local config to const DynamicPrintConfig
    const DynamicPrintConfig* local_config = nullptr;
    //ModelConfig* local_config = nullptr;
    wxWindow*    m_msg_dlg_parent {nullptr};

    t_config_option_keys m_applying_keys;

public:
    ConfigManipulation(std::function<void()> load_config,
        std::function<void(const std::string&, bool toggle, int opt_index)> cb_toggle_field,
        std::function<void(const std::string&, bool toggle)> cb_toggle_line,
        std::function<void(const std::string&, const boost::any&)>  cb_value_change,
        //BBS: change local config to DynamicPrintConfig
        const DynamicPrintConfig* local_config = nullptr,
        wxWindow* msg_dlg_parent  = nullptr) :
        load_config(load_config),
        cb_toggle_field(cb_toggle_field),
        cb_toggle_line(cb_toggle_line),
        cb_value_change(cb_value_change),
        m_msg_dlg_parent(msg_dlg_parent),
        local_config(local_config) {}
    ConfigManipulation() {}

    ~ConfigManipulation() {
        load_config = nullptr;
        cb_toggle_field = nullptr;
        cb_toggle_line = nullptr;
        cb_value_change = nullptr;
    }

    bool    is_applying() const;

    void    apply(DynamicPrintConfig* config, DynamicPrintConfig* new_config);
    t_config_option_keys const &applying_keys() const;
    void    toggle_field(const std::string& field_key, const bool toggle, int opt_index = -1);
    void    toggle_line(const std::string& field_key, const bool toggle);

    // FFF print
    void    update_print_fff_config(DynamicPrintConfig* config, const bool is_global_config = false, const bool is_plate_config = false);
    void    toggle_print_fff_options(DynamicPrintConfig* config, const bool is_global_config = false);
    void    apply_null_fff_config(DynamicPrintConfig *config, std::vector<std::string> const &keys, ModelConfigEntries const &configs);

    //BBS: FFF filament nozzle temperature range
    void    check_nozzle_recommended_temperature_range(DynamicPrintConfig *config);
    void    check_nozzle_temperature_range(DynamicPrintConfig* config, int index = 0);
    void    check_nozzle_temperature_initial_layer_range(DynamicPrintConfig* config, int index = 0);
    void    check_filament_max_volumetric_speed(DynamicPrintConfig *config);
    void    check_chamber_temperature(DynamicPrintConfig* config);
    void    set_is_BBL_Printer(bool is_bbl_printer) { is_BBL_Printer = is_bbl_printer; };
    void    set_is_CX_Printer(bool is_cx_printer) { is_CX_Printer = is_cx_printer; };

    // Show a warning dialog when enabling variable layer height together with
    // mixed color sublayer. Used at three trigger points (toolbar button to
    // toggle variable layer editing, object list to add a layer range, and the
    // print config validator that runs after the user flips the sublayer
    // checkbox on). Coalesces repeated warnings within one session via static
    // state; honours the per-user "Don't show again" opt-out persisted under
    // app_config key "no_warn_mixed_sublayer_variable_layer". Mirrors the
    // behaviour of BambuStudio's three call sites.
    static void warn_mixed_sublayer_variable_layer(wxWindow* parent);
    // SLA print
    void    update_print_sla_config(DynamicPrintConfig* config, const bool is_global_config = false);
    void    toggle_print_sla_options(DynamicPrintConfig* config);

    bool    is_initialized_support_material_overhangs_queried() { return m_is_initialized_support_material_overhangs_queried; }
    void    initialize_support_material_overhangs_queried(bool queried)
    {
        m_is_initialized_support_material_overhangs_queried = true;
        m_support_material_overhangs_queried = queried;
    }
    int    show_spiral_mode_settings_dialog(bool is_object_config = false,
                                            const wxString& conflict_message = wxEmptyString,
                                            ZaaUiChangeScope normalization_scope = ZaaUiChangeScope::Global,
                                            bool spiral_already_enabled = false);

private:
    bool get_temperature_range(DynamicPrintConfig *config, int &range_low, int &range_high);
};
#endif // SLIC3R_CONFIG_MANIPULATION_PLANNER_ONLY

} // GUI
} // Slic3r

#endif /* slic3r_ConfigManipulation_hpp_ */
