#ifndef slic3r_MixedFilament_hpp_
#define slic3r_MixedFilament_hpp_

#include <limits>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <utility>

namespace Slic3r {

// ---------------------------------------------------------------------------
// Gradient curve (Photoshop-style). Shared between the GUI curve editor and
// the slicing backend so what the user sees in the editor matches the
// per-layer color blend the slicer actually emits.
//
// The curve parameterises "Z progress (x) -> first-component ratio (y)" over
// [0,1]^2. y is the ratio of component 0 (component_a) in the two-color mix;
// (1 - y) is the ratio of component 1 (component_b). x is the normalized
// height progress of the current run.
//
// Empty GradientCurve means "no custom curve" (callers fall back to the
// linear start..end range).
// ---------------------------------------------------------------------------
struct GradientAnchor {
    double x     = 0.0;
    double y     = 0.0;
    // Cubic Hermite tangent overrides. NaN means "use the PCHIP-computed
    // default", which is the case for plain anchors loaded from old 2-field
    // 3MF projects or freshly added via a quick click. A press-and-drag on a
    // curve segment populates m_out of the left anchor and m_in of the right
    // anchor so the segment bends without inserting a new anchor.
    double m_in  = std::numeric_limits<double>::quiet_NaN();
    double m_out = std::numeric_limits<double>::quiet_NaN();
};

struct GradientCurve {
    std::vector<GradientAnchor> points;
    bool empty() const { return points.empty(); }
    // Explicit copy/move so callers in GUI code (Plater.cpp, MixedFilamentDialog)
    // can assign a curve to / from a const ref without depending on the
    // implicit special members (the user-defined operator== below is a free
    // function, but some MSVC configurations still suppress the implicit
    // copy assignment when the struct defines any non-data member function).
    GradientCurve() = default;
    GradientCurve(const GradientCurve &) = default;
    GradientCurve(GradientCurve &&) noexcept = default;
    GradientCurve &operator=(const GradientCurve &) = default;
    GradientCurve &operator=(GradientCurve &&) noexcept = default;
};

// Reserved blend ratio range. Anchor y values (= component 0's ratio) are
// constrained to this band so the mixed filament never reaches pure 0% / 100%
// of either physical component, which keeps both extruders flowing and avoids
// degenerate transitions. Both the editor and the sampler enforce this clamp.
constexpr double kGradientMinRatio = 0.1;
constexpr double kGradientMaxRatio = 0.9;

// Parse "x0,y0[,m_in0,m_out0];x1,y1[,m_in1,m_out1];..." into a GradientCurve.
// Accepts both the legacy 2-field form (tangents -> NaN) and the new 4-field
// form (empty token or "nan" preserved as NaN). Returns an empty curve when
// the input is empty or unparsable. Points are clamped to [0,1] for (x, y)
// and re-sorted by x.
GradientCurve parse_gradient_curve(const std::string &s);

// Serialize a GradientCurve back to a string. Emits 4 fields per anchor when
// any tangent override is finite; emits 2 fields when both tangents are NaN
// so unchanged projects stay byte-identical with the legacy format. Returns
// "" when empty. Separator between anchors is ';'.
std::string serialize_gradient_curve(const GradientCurve &c);

// Sample the curve at t in [0,1] using cubic Hermite with Fritsch-Carlson
// PCHIP default tangents, optionally overridden per anchor via m_in / m_out.
// Returns the clamped end values when t is outside the control point range.
// Returns 0.5 when the curve has fewer than 2 points (a safety fallback;
// callers should check empty()).
double sample_gradient_curve(const GradientCurve &c, double t);

// Compute Fritsch-Carlson PCHIP default tangents for a sorted-by-x anchor
// list. Result size == pts.size(). Useful for callers that need to know
// what tangent the sampler would synthesize when m_in / m_out are NaN.
std::vector<double> compute_pchip_default_tangents(const std::vector<GradientAnchor> &pts);

// Equality / inequality on the curve so it can participate in STL containers
// and struct field comparisons without extra boilerplate.
inline bool operator==(const GradientAnchor &a, const GradientAnchor &b)
{
    return a.x == b.x && a.y == b.y && a.m_in == b.m_in && a.m_out == b.m_out;
}
inline bool operator!=(const GradientAnchor &a, const GradientAnchor &b) { return !(a == b); }

inline bool operator==(const GradientCurve &a, const GradientCurve &b)
{
    return a.points == b.points;
}
inline bool operator!=(const GradientCurve &a, const GradientCurve &b) { return !(a == b); }

// Represents a virtual "mixed" filament created from physical filaments
// (layer cadence and/or same-layer interleaved stripe distribution). Display
// colour blending uses FilamentMixer  so pair previews better
//  match expected print mixing 
// (for example Blue+Yellow -> Green, Red+Yellow -> Orange, Red+Blue -> Purple). 
// Legacy RYB code is retained in source for reference only.
struct MixedFilament
{
    enum DistributionMode : uint8_t {
        LayerCycle = 0,
        SameLayerPointillisme = 1,
        Simple = 2
    };

    // 1-based physical filament IDs that are combined. A value of 0 means
    // that the referenced physical filament was removed and the component
    // must be selected again before this mixed filament can be enabled.
    unsigned int component_a = 1;
    unsigned int component_b = 2;

    // Persistent row identity used to keep painted virtual-tool assignments
    // stable even when the visible mixed-filament list is rebuilt.
    uint64_t stable_id = 0;

    // Layer-alternation ratio.  With ratio_a = 2, ratio_b = 1 the cycle is
    // A, A, B, A, A, B, ...
    int ratio_a = 1;
    int ratio_b = 1;

    // Blend percentage of component B in [0..100].
    int mix_b_percent = 50;

    // Optional manual pattern for this mixed filament. Tokens:
    // '1' => component_a, '2' => component_b, '3'..'9' => direct physical
    // filament IDs (1-based). Example: "11112222" => AAAABBBB repeating.
    std::string manual_pattern;

    // Optional explicit gradient multi-color component list, encoded as
    // compact physical filament IDs (for example "123" -> filaments 1,2,3).
    // A '0' preserves the position and weight of a removed component.
    // Interleaved stripe mode is active for gradient rows only when this list has 3+ IDs.
    std::string gradient_component_ids;
    // Optional explicit multi-color weights aligned with gradient_component_ids.
    // Compact integer list joined by '/': for example "50/25/25".
    std::string gradient_component_weights;

    // Legacy compatibility flag from earlier prototype serialization.
    bool pointillism_all_filaments = false;

    // How this mixed row is distributed:
    // - LayerCycle: one filament per layer based on cadence.
    // - SameLayerPointillisme: split painted masks in XY on each layer.
    int distribution_mode = int(Simple);

    // Whether this mixed filament is enabled (available for assignment).
    bool enabled = true;

    // True when this mixed filament row was deleted from UI and should stay hidden.
    bool deleted = false;

    // True when this row was user-created (custom) instead of auto-generated.
    bool custom = false;

    // True when this row originated from an auto-generated pair. This remains
    // true even after editing so delete logic can keep the base auto pair
    // tombstoned instead of letting regeneration resurrect it.
    bool origin_auto = false;

    // Optional Photoshop-style custom curve overriding the linear gradient
    // blend across model height. Empty -> use the linear start..end range.
    // Non-empty -> cubic Hermite over [0,1]^2 with optional per-anchor
    // tangent overrides (see GradientAnchor).
    GradientCurve gradient_curve;

    // When true, the gradient is evaluated independently for every distinct
    // part of the model (per-part), so a tall object and a short object get
    // their own full A->B ramp instead of sharing one global ramp.
    // Only meaningful when the mixed row is in two-color mode and the slicer
    // is per-part-capable.
    bool per_part_gradient = false;

    // Whether the "Gradient Effect" toggle was enabled in the dialog when
    // this row was last edited. Persisted to 3MF so reopening the dialog
    // keeps the checkbox state across sessions. When true, the slicer runs
    // in gradient mode (linear range by default, or the user-defined curve
    // if gradient_curve is non-empty). When false, the slicer uses the
    // fixed mix_b_percent ratio regardless of curve content.
    bool gradient_enabled = false;

    // Gradient direction for two-color gradient. 0 = A→B (bottom to top goes
    // A-dominant to B-dominant), 1 = B→A. Persisted to 3MF so reopening the
    // dialog matches the curve shape that the user previously saved. The
    // slicer backend does not consume this field directly; it is the GUI's
    // responsibility to keep gradient_curve / linear-fallback consistent
    // with the selected direction.
    int gradient_direction = 0;

    // Computed display colour as "#RRGGBB".
    std::string display_color;

    bool has_missing_component() const
    {
        if (component_a == 0 || component_b == 0)
            return true;
        if (gradient_component_ids.empty())
            return false;
        // Support both formats:
        // - New '|'-separated: check for "0" token
        // - Old compact: check for '0' character
        if (gradient_component_ids.find('|') != std::string::npos) {
            std::string tok;
            for (char c : gradient_component_ids) {
                if (c >= '0' && c <= '9') {
                    tok.push_back(c);
                } else if (c == '|') {
                    if (tok == "0") return true;
                    tok.clear();
                }
            }
            return tok == "0";
        }
        return gradient_component_ids.find('0') != std::string::npos;
    }

    // Collect the 1-based physical filament IDs this row references.
    // Uses gradient_component_ids when present; otherwise component_a/b.
    // IDs outside [1, num_physical] and missing (0) placeholders are skipped.
    std::vector<unsigned int> referenced_component_ids(size_t num_physical) const;

    // True when the referenced physical components have more than one distinct
    // filament type (for example PLA + PETG). Missing/incomplete references are
    // not treated as a type mismatch; use has_missing_component() for those.
    bool has_type_mismatch(const std::vector<std::string> &physical_types) const;

    // An allocated row may participate in slicing or merging only while every
    // referenced physical filament still exists and the row is enabled.
    bool is_available(size_t num_physical) const
    {
        if (!enabled || deleted || has_missing_component() ||
            component_a > num_physical || component_b > num_physical ||
            component_a == component_b)
            return false;

        if (gradient_component_ids.empty())
            return true;

        // Support both formats:
        // - New '|'-separated: "1|2|11|12"
        // - Old compact: "123" (single-char IDs 1-9)
        if (gradient_component_ids.find('|') != std::string::npos) {
            // New format: parse tokens and validate each ID
            std::string tok;
            for (char c : gradient_component_ids) {
                if (c >= '0' && c <= '9') {
                    tok.push_back(c);
                } else if (c == '|') {
                    if (!tok.empty()) {
                        try {
                            unsigned int id = std::stoi(tok);
                            if (id == 0 || id > num_physical)
                                return false;
                        } catch (...) {
                            return false;
                        }
                        tok.clear();
                    }
                }
            }
            if (!tok.empty()) {
                try {
                    unsigned int id = std::stoi(tok);
                    if (id == 0 || id > num_physical)
                        return false;
                } catch (...) {
                    return false;
                }
            }
            return true;
        }

        // Old format: each character is a single-digit ID
        return std::all_of(gradient_component_ids.begin(), gradient_component_ids.end(),
            [num_physical](char component) {
                return component >= '1' && component <= '9' &&
                       size_t(component - '0') <= num_physical;
            });
    }

    // Missing rows remain allocated so their virtual IDs stay stable while the
    // user repairs them. Other disabled rows retain the historical no-slot behavior.
    bool occupies_virtual_slot() const
    {
        return !deleted && (enabled || has_missing_component());
    }

    bool operator==(const MixedFilament &rhs) const
    {
        return component_a == rhs.component_a &&
               component_b == rhs.component_b &&
               stable_id   == rhs.stable_id   &&
               ratio_a     == rhs.ratio_a     &&
               ratio_b     == rhs.ratio_b     &&
               mix_b_percent == rhs.mix_b_percent &&
               manual_pattern == rhs.manual_pattern &&
               gradient_component_ids == rhs.gradient_component_ids &&
               gradient_component_weights == rhs.gradient_component_weights &&
               pointillism_all_filaments == rhs.pointillism_all_filaments &&
               distribution_mode == rhs.distribution_mode &&
               enabled      == rhs.enabled &&
               deleted      == rhs.deleted &&
               custom       == rhs.custom &&
               origin_auto  == rhs.origin_auto &&
               gradient_curve == rhs.gradient_curve &&
               per_part_gradient == rhs.per_part_gradient &&
               gradient_enabled == rhs.gradient_enabled &&
               gradient_direction == rhs.gradient_direction;
    }
    bool operator!=(const MixedFilament &rhs) const { return !(*this == rhs); }
};

struct ExpandedFilamentUsage
{
    bool                      has_mixed_filament = false;
    std::vector<unsigned int> physical_filament_ids;
    std::vector<unsigned int> invalid_filament_ids;

    bool valid() const { return invalid_filament_ids.empty(); }
};

// ---------------------------------------------------------------------------
// MixedFilamentManager
//
// Owns the list of mixed filaments and provides helpers used by the slicing
// pipeline to resolve virtual IDs back to physical extruders.
//
// Virtual filament IDs are numbered starting at (num_physical + 1).  For a
// 4-extruder printer the first mixed filament has ID 5, the second 6, etc.
// ---------------------------------------------------------------------------
class MixedFilamentManager
{
public:
    MixedFilamentManager() = default;

    // ---- Auto-generation ------------------------------------------------

    // Rebuild the mixed-filament list from the current set of physical
    // filament colours.  Generates all C(N,2) pairwise combinations.
    // Previous ratio/enabled state is preserved when a combination still
    // exists.
    void auto_generate(const std::vector<std::string> &filament_colours);

    // Remove a physical filament (1-based ID) from the mixed list.
    // Referenced components become 0 (missing), while later physical IDs are
    // shifted down. Incomplete rows are retained for editing and keep their
    // allocated virtual filament slot.
    void remove_physical_filament(unsigned int deleted_filament_id, unsigned int num_physicals);

    // Add a custom mixed filament.
    void add_custom_filament(unsigned int component_a, unsigned int component_b, int mix_b_percent, const std::vector<std::string> &filament_colours);

    // Remove all custom rows, keep auto-generated ones.
    void clear_custom_entries();

    // Recompute cadence ratios from gradient settings.
    // gradient_mode: 0 = Layer cycle weighted, 1 = Height weighted.
    void apply_gradient_settings(int   gradient_mode,
                                 float lower_bound,
                                 float upper_bound,
                                 bool  advanced_dithering = false);

    // Persist mixed rows, including auto/deleted state, into the compact
    // project-settings string.
    std::string serialize_custom_entries();
    void load_custom_entries(const std::string &serialized, const std::vector<std::string> &filament_colours);

    // Compare two serialized mixed-filament definition strings and report
    // whether the change from `old_serialized` to `new_serialized` only appends
    // new mixed-filament rows at the end, leaving every previously enabled
    // (non-deleted) row unchanged in its geometry-affecting fields and virtual-ID
    // order. When true, a previously computed slice result / G-code stays valid,
    // because no existing painted virtual filament changed and any appended rows
    // are not yet painted on the model. Returns false for any edit, delete,
    // disable or reorder of existing rows, or if either string cannot be parsed.
    static bool is_definitions_change_append_only(const std::string &old_serialized,
                                                  const std::string &new_serialized);

    // Normalize a manual mixed-pattern string into compact token form.
    // Accepts separators and A/B aliases. Returns empty string if invalid.
    static std::string normalize_manual_pattern(const std::string &pattern);
    static int         mix_percent_from_manual_pattern(const std::string &pattern);

    // ---- Queries --------------------------------------------------------

    // True when `filament_id` (1-based) refers to a mixed filament.
    bool is_mixed(unsigned int filament_id, size_t num_physical) const
    {
        return mixed_index_from_filament_id(filament_id, num_physical) >= 0;
    }

    // Resolve a mixed filament ID to a physical extruder (1-based) for the
    // given layer context. Returns `filament_id` unchanged when it is not a
    // mixed filament.
    unsigned int resolve(unsigned int filament_id,
                         size_t       num_physical,
                         int          layer_index,
                         float        layer_print_z = 0.f,
                         float        layer_height  = 0.f,
                         bool         force_height_weighted = false) const;
    unsigned int resolve_perimeter(unsigned int filament_id,
                                   size_t       num_physical,
                                   int          layer_index,
                                   int          perimeter_index,
                                   float        layer_print_z = 0.f,
                                   float        layer_height  = 0.f,
                                   bool         force_height_weighted = false) const;
    std::vector<unsigned int> ordered_perimeter_extruders(unsigned int filament_id,
                                                          size_t       num_physical,
                                                          int          layer_index,
                                                          float        layer_print_z = 0.f,
                                                          float        layer_height  = 0.f,
                                                          bool         force_height_weighted = false) const;

    // Map virtual filament ID (1-based, after physical IDs) to index into
    // m_mixed. Virtual IDs enumerate allocated slots; an incomplete row keeps
    // its slot even though it is temporarily unavailable for slicing.
    int mixed_index_from_filament_id(unsigned int filament_id, size_t num_physical) const;

    // Blend N colours using weighted FilamentMixer blending.
    // color_percents: vector of (hex_color, percent) where percents sum to 100.
    static std::string blend_color_multi(
        const std::vector<std::pair<std::string, int>> &color_percents);

    const MixedFilament *mixed_filament_from_id(unsigned int filament_id, size_t num_physical) const;

    // Expand 1-based logical filament IDs into the physical filament IDs that
    // may actually be selected by the current mixed-filament definitions.
    // Invalid virtual rows or invalid component references are reported
    // explicitly and are never silently mapped to filament/nozzle 1.
    ExpandedFilamentUsage expand_filament_usage(const std::vector<unsigned int> &filament_ids,
                                                size_t                           num_physical) const;

    // Compute a display colour by blending two colours with FilamentMixer.
    static std::string blend_color(const std::string &color_a,
                                   const std::string &color_b,
                                   int ratio_a, int ratio_b);

    // ---- Accessors ------------------------------------------------------

    const std::vector<MixedFilament> &mixed_filaments() const { return m_mixed; }
    std::vector<MixedFilament>       &mixed_filaments()       { return m_mixed; }

    // Number of complete, enabled mixed rows available to slicing.
    size_t enabled_count() const;

    // Number of allocated virtual slots. Incomplete rows still own a slot until deleted.
    size_t virtual_count() const;

    // Total filament count = physical filaments + all allocated virtual slots.
    size_t total_filaments(size_t num_physical) const { return num_physical + virtual_count(); }

    // Return display colours for all allocated virtual slots, in ID order.
    std::vector<std::string> display_colors() const;

    // Refresh display colors when physical filament colors change
    void refresh_display_colors(const std::vector<std::string> &filament_colours);

private:
    // Convert a 1-based virtual ID to a 0-based index into m_mixed.
    size_t index_of(unsigned int filament_id, size_t num_physical) const
    {
        return static_cast<size_t>(filament_id - num_physical - 1);
    }
    uint64_t allocate_stable_id();
    uint64_t normalize_stable_id(uint64_t stable_id);

    std::vector<MixedFilament> m_mixed;
    int                        m_gradient_mode       = 0;
    float                      m_height_lower_bound  = 0.04f;
    float                      m_height_upper_bound  = 0.16f;
    bool                       m_advanced_dithering  = false;
    uint64_t                   m_next_stable_id      = 1;
};

} // namespace Slic3r

#endif /* slic3r_MixedFilament_hpp_ */
