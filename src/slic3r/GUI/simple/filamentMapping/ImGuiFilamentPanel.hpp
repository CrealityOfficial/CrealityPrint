// ImGuiFilamentPanel is a legacy compatibility module that mirrors a subset of
// the older wx-based filament panel behavior inside the simple-mode ImGui UI.
//
// Important status:
// - This module is being deprecated as the primary owner of filament-mapping
//   business logic.
// - New AI-mode mapping flows should prefer the service-oriented path instead
//   of depending on this panel's internal UI state.
//
// Logic that has already been moved out of this module:
// - Source scene filament snapshot capture:
//   SceneFilamentSourceSnapshotManager
// - UI-independent mapping construction / auto-match / apply / send export:
//   FilamentMappingService
// - AI send workflow session orchestration:
//   AISendWorkflowService
//
// Current role of this module:
// - Keep legacy ImGui rendering and compatibility behavior working for older
//   simple-mode entry points.
// - Provide a temporary bridge for flows that have not yet been fully migrated.
//
// When adding or changing filament-mapping behavior, prefer extending the
// service-layer modules above unless the change is strictly visual or strictly
// scoped to this legacy panel UI.

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "imgui/imgui.h"
#include "libslic3r/GCode/ThumbnailData.hpp"
#include "print_manage/data/DataType.hpp"

namespace Slic3r {
namespace GUI {

class PartPlate;
class ImGuiThumbnailPreview;

// Lightweight state used by the ImGui panel. Data is sourced from Slic3r presets at render time.
struct ImGuiFilamentItemState {
    int         index = 0;
    ImVec4      color = ImVec4(1, 1, 1, 1);
    ImVec4      match_color = ImVec4(1, 1, 1, 1);
    std::string preset_display;   // Human readable preset label
    std::string type_label;       // Filament type, e.g., PLA / PETG-CF
    std::string sync_label;       // CFS label like 1A/1B or "EXT"
    std::string device_match_slot;  // 1A/1B/... from device
    std::string mapping_token;      // device material token, e.g. "0:1:3"
};

class ImGuiFilamentPanel {
public:
    // Mode switches from outer UI (CFS or external rack)
    enum class PreviewViewType
    {
        Iso = 0,
        Front,
        Top
    };

    struct PreviewViewState
    {
        PreviewViewType type = PreviewViewType::Iso;
        int             iso_direction_index = 0;
        int             front_direction_index = 0;
    };

    struct PreviewThumbnailRenderParams
    {
        bool        use_top_view = false;
        std::string view_type = "iso";
    };

    enum class Mode
    {
        CFS = 0,
        External = 1
    };

    struct ModeAvailability {
        bool show_cfs = false;
        bool show_external = true;
    };

public:
    ImGuiFilamentPanel();
    ~ImGuiFilamentPanel();

    void reset();

    // Render the whole panel. Pulls latest project_config each frame.
    void Render();

    void render_add_button_and_palette();

    // Auto-map from device materials (CFS/EXT) to panel/presets
    void on_auto_mapping_filament(const DM::Device& deviceData);
    void on_auto_mapping_filament_ex(const DM::Device& deviceData);

    // Optional external hooks to integrate with app-specific flows.
    std::function<void(int /*idx*/)>              on_delete_filament;
    std::function<void(int /*src*/, int /*dst*/)> on_merge_with; // src merges into dst

    void set_mode(Mode m) { m_mode = m; }
    Mode mode() const { return m_mode; }

    // Compute which mode options should be shown for a given device.
    // Intended to encapsulate the mode-selection rules currently implemented in filamentSimplePage.cpp.
    ModeAvailability mode_availability_from_device(const DM::Device& device) const;

    // Resolve the desired mode to a device-supported mode based on ModeAvailability.
    // If desired mode is not available, falls back to the other mode when available.
    Mode resolve_mode_for_device(const DM::Device& device, Mode desired) const;

    void check_and_resolve_mode_by_current_device();
    void check_device_filament_auto_mapping();
    bool is_current_device_valid();
    void refresh_items_from_config();

    bool apply_mapping_selection(int item_index, const std::string& selection_token);
    bool apply_mapping_colors_to_scene();

    nlohmann::json export_color_match_info() const;
    nlohmann::json export_ai_mapping_items() const;
    bool print_calibration_enabled() const { return m_print_calibration; }
    void set_print_calibration(bool enabled) { m_print_calibration = enabled; }
    void set_embed_in_unified_left_panel(bool enabled) { m_embed_in_unified_left_panel = enabled; }
    bool embed_in_unified_left_panel() const { return m_embed_in_unified_left_panel; }

    void on_scene_reloaded();

private:
    // Internal helpers
    void render_item(ImGuiFilamentItemState& item);
    void render_external_item(float scale);
    void render_thumbnail_preview(float preview_w, float child_h, float scale);
    void invalidate_preview_thumbnail_cache(bool invalidate_plate_thumbnail, bool invalidate_local_preview_cache = true);
    static bool preview_supports_rotation(PreviewViewType type);
    static int preview_direction_count(PreviewViewType type);
    static void rotate_preview_direction_index(int& index, int step, int count);
    void rotate_preview_left();
    void rotate_preview_right();
    PreviewThumbnailRenderParams build_preview_thumbnail_render_params() const;
    static ImVec4 hex_to_imvec4(const std::string& hex);
    static std::string imvec4_to_hex(const ImVec4& c);
    void commit_color_change(int idx, const ImVec4& c);
    void on_update_filament_type(int idx, const std::string& filament_type);
    bool is_item_matched(const ImGuiFilamentItemState& item);
    bool can_add_scene_color() const;

    size_t device_fingerprint(const DM::Device& d);

    void on_pick_and_add_filament_colour(const std::string& filament_colour);
    void remove_last_filament();
    void remap_item_with_match_color(int idx); // idx == -1 => remap all m_items
    void remap_external_item(const DM::Device& deviceData);

    // Drawing helpers
    static bool is_dark_text_on(const ImVec4& bg);
    static void draw_capsule(const ImVec2& pos, const ImVec2& size, const ImVec4& bg, const char* text, float rounding = 6.f);

private:
    struct HoverPreviewOverride {
        int    item_index = -1;
        ImVec4 match_color = ImVec4(1.f, 1.f, 1.f, 1.f);
        bool   active = false;
    };

    struct MaterialOption {
        int          box_id = 0;     // CFS box id
        int          slot_index = 0; // index in the box materials list
        int          box_type = 0;   // 0: CFS, 2: CFS-mini/external
        DM::Material material;       // full material info
    };

    std::vector<ImGuiFilamentItemState>   m_items;
    std::vector<MaterialOption>           m_material_options; // cached from last device scan
    int                                   m_popup_target = -1; // which item opened the popup
    bool                                  m_initialized = false;
    Mode                                  m_mode = Mode::CFS;
    std::string                           m_selected_material_type = "Hyper PLA"; // 0: Hyper PLA, 1: Hyper PETG, 2: Generic PLA, 3: Generic ABS
    bool                                  m_print_calibration = true;
    bool                                  m_embed_in_unified_left_panel = false;
    PreviewViewState                      m_preview_view_state;

    size_t                                m_last_sig = 0;
    ImTextureID                           m_filament_disable_icon_tex = nullptr;
    // Guard to ensure popup content renders once per ImGui frame
    int                                   m_last_popup_frame_drawn = -1;
    std::unique_ptr<ImGuiThumbnailPreview> m_thumbnail_preview;
    const PartPlate*                      m_thumb_base_plate = nullptr;
    ThumbnailData                         m_thumb_base_lit;
    ThumbnailData                         m_thumb_base_no_light;
    std::vector<unsigned>                 m_thumb_base_rgb;
    bool                                  m_thumb_recolor_dirty = false;
    HoverPreviewOverride                  m_hover_preview_override;
};

} // namespace GUI
} // namespace Slic3r
