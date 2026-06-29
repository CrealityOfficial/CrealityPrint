// ImGuiFilamentPanel remains in the tree as a legacy compatibility UI module.
// Historically it mixed rendering, device-slot matching, scene config writes,
// and send-time export logic in one place.
//
// That design is now being phased out in favor of smaller service-oriented
// modules with explicit responsibilities:
// - SceneFilamentSourceSnapshotManager owns original scene/source filament data.
// - FilamentMappingService owns UI-independent mapping rules and send export.
// - AISendWorkflowService owns AI send session state and workflow coordination.
//
// This file should therefore be treated as deprecated for new business logic.
// New mapping behavior should be implemented in the service layer first, with
// this panel only calling into those services when legacy UI compatibility is
// still required.

#include "ImGuiFilamentPanel.hpp"
#include "ImGuiThumbnailPreview.hpp"
#include "ThumbnailDataRecolor.hpp"
#include "ResidentFilamentMappingLogic.hpp"
#include "ResidentFilamentMappingAdapter.hpp"
#include "ResidentFilamentMappingPopupPolicy.hpp"
#include "ResidentFilamentMappingView.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <iomanip>
#include <map>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

// Pull required Slic3r headers to talk to project_config and plater.
#include "../../GUI_App.hpp"
#include "../../Plater.hpp"
#include "../../GLCanvas3D.hpp"
#include "libslic3r/Config.hpp"
#include "libslic3r/Preset.hpp"
#include "../../Tab.hpp"
#include "../../print_manage/data/DataCenter.hpp"
#include "../../PartPlate.hpp"
#include "match_color.hpp"
#include "libslic3r/GCode/ThumbnailData.hpp"
#include "slic3r/Utils/ColorSpaceConvert.hpp"
#include "imgui/imgui_internal.h"

#include <wx/colordlg.h>

namespace Slic3r {
namespace GUI {

static std::string clamp_hex(const std::string &in)
{
    if (in.empty()) return "#000000";
    if (in[0] != '#') return "#" + in;
    return in;
}

struct ResidentFilamentPanelTransientUiState {
    ImVec2 add_palette_anchor = ImVec2(0.f, 0.f);
    bool   open_add_palette_requested = false;
    int    pending_delete_row_index = -1;
    bool   open_delete_confirm_requested = false;
    int    pending_scene_color_edit_index = -1;
    bool   open_scene_color_dialog_requested = false;
    int    pending_scene_material_popup_index = -1;
    ImVec2 scene_material_anchor = ImVec2(0.f, 0.f);
    bool   open_scene_material_popup_requested = false;
};

static ResidentFilamentPanelTransientUiState& transient_ui_state()
{
    static ResidentFilamentPanelTransientUiState state;
    return state;
}

static std::string trim_copy(std::string value)
{
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

static std::string to_lower_copy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

static std::string extract_material_family(std::string value)
{
    const std::string normalized = to_lower_copy(trim_copy(std::move(value)));
    static const char* kFamilies[] = {
        "pla", "petg", "abs", "asa", "tpu", "pa", "nylon", "pc", "hips", "pva", "support"
    };

    for (const char* family : kFamilies) {
        if (normalized.find(family) != std::string::npos)
            return family;
    }

    return normalized;
}

static bool loose_material_match(const std::string& lhs, const std::string& rhs)
{
    const std::string left = extract_material_family(lhs);
    const std::string right = extract_material_family(rhs);
    if (left.empty() || right.empty())
        return true;
    if (left == right)
        return true;
    return left.find(right) != std::string::npos || right.find(left) != std::string::npos;
}
static float color_distance_sq(const ImVec4& lhs, const ImVec4& rhs)
{
    const float dr = lhs.x - rhs.x;
    const float dg = lhs.y - rhs.y;
    const float db = lhs.z - rhs.z;
    return dr * dr + dg * dg + db * db;
}
static bool auto_mapping_box_matches_mode(ImGuiFilamentPanel::Mode mode, int box_type)
{
    return ResidentFilamentMappingAdapter::does_box_type_match_mode(mode, box_type);
}
static std::string auto_mapping_slot_label_for_box(int box_type, int box_id, int material_id)
{
    if (box_type == 1 || box_type == 2)
        return "EXT";
    const char letter = static_cast<char>('A' + (material_id % 26));
    return std::to_string(box_id) + letter;
}

static bool try_apply_popup_fallback_mapping(
    ImGuiFilamentItemState& item,
    const ResidentFilamentMappingAdapter::PopupOptionCatalog& popup_catalog)
{
    if (!item.device_match_slot.empty())
        return false;

    const ResidentFilamentMappingAdapter::PopupOptionSeed* best_option = nullptr;
    float best_distance = std::numeric_limits<float>::max();
    const std::string scene_type = trim_copy(item.type_label.empty() ? item.preset_display : item.type_label);

    for (const auto& group : popup_catalog.groups) {
        for (const auto& option : group.options) {
            if (!option.available)
                continue;

            const std::string option_type = trim_copy(option.material_match_key);
            if (!scene_type.empty() && !option_type.empty() && to_lower_copy(scene_type) != to_lower_copy(option_type) &&
                !loose_material_match(scene_type, option_type))
                continue;

            const float distance = color_distance_sq(item.color, option.material_color);
            if (best_option == nullptr || distance < best_distance) {
                best_option = &option;
                best_distance = distance;
            }
        }
    }

    if (best_option == nullptr || best_option->selection_token.empty())
        return false;

    if (!ResidentFilamentMappingAdapter::apply_popup_selection(popup_catalog, best_option->selection_token, item))
        return false;

    item.mapping_token = best_option->selection_token;
    return true;
}

static std::string material_choice_label_from_preset(const Preset& preset)
{
    if (!preset.alias.empty())
        return preset.alias;

    std::string fallback = preset.name;
    const size_t at_pos = fallback.find(" @");
    if (at_pos != std::string::npos)
        fallback = fallback.substr(0, at_pos);
    fallback = trim_copy(fallback);
    return fallback.empty() ? std::string("Filament") : fallback;
}

static bool choose_filament_color_with_dialog(const wxColour& initial_color, const wxString& title, std::string& out_hex)
{
    wxColourData color_data;
    color_data.SetColour(initial_color);
    color_data.SetChooseFull(true);
    color_data.SetChooseAlpha(false);

    std::vector<std::string> custom_colors = wxGetApp().app_config->get_custom_color_from_config();
    for (size_t i = 0; i < custom_colors.size() && i < static_cast<size_t>(CUSTOM_COLOR_COUNT); ++i)
        color_data.SetCustomColour(static_cast<int>(i), string_to_wxColor(custom_colors[i]));

    wxColourDialog dialog(wxGetApp().plater(), &color_data);
    dialog.CenterOnParent();
    dialog.SetTitle(title);
    if (dialog.ShowModal() != wxID_OK)
        return false;

    color_data = dialog.GetColourData();
    if (custom_colors.size() != CUSTOM_COLOR_COUNT)
        custom_colors.resize(CUSTOM_COLOR_COUNT);
    for (int i = 0; i < CUSTOM_COLOR_COUNT; ++i)
        custom_colors[i] = color_to_string(color_data.GetCustomColour(i));
    wxGetApp().app_config->save_custom_color_to_config(custom_colors);

    out_hex = color_data.GetColour().GetAsString(wxC2S_HTML_SYNTAX).ToStdString();
    return true;
}

static std::vector<std::string> build_scene_material_type_options(int item_index, const std::string& current_label)
{
    std::vector<std::string> options;
    auto* bundle = wxGetApp().preset_bundle;
    if (bundle == nullptr)
        return options;

    const auto& presets = bundle->filaments.get_presets();
    const std::string selected_preset_name =
        (item_index >= 0 && item_index < static_cast<int>(bundle->filament_presets.size())) ? bundle->filament_presets[item_index] : std::string();

    for (const Preset& preset : presets) {
        if (preset.name.empty())
            continue;

        const bool is_selected = !selected_preset_name.empty() && preset.name == selected_preset_name;
        if (!preset.is_visible)
            continue;
        if (!preset.is_compatible && !is_selected)
            continue;

        const std::string label = material_choice_label_from_preset(preset);
        if (label.empty())
            continue;
        if (std::find(options.begin(), options.end(), label) == options.end())
            options.push_back(label);
    }

    if (!current_label.empty() && std::find(options.begin(), options.end(), current_label) == options.end())
        options.push_back(current_label);

    return options;
}


ImGuiFilamentPanel::ImGuiFilamentPanel()
{
    ImTextureID new_tex = nullptr;

    if (IMTexture::load_from_png_file(Slic3r::resources_dir() + "/images/filament_disable_background_simple.png", 56, 40, new_tex)) {
        m_filament_disable_icon_tex  = new_tex;
    }

    m_thumbnail_preview = std::make_unique<ImGuiThumbnailPreview>();
}

ImGuiFilamentPanel::~ImGuiFilamentPanel()
{
    if (m_filament_disable_icon_tex) {
        IMTexture::release_texture(m_filament_disable_icon_tex);
        m_filament_disable_icon_tex = nullptr;
    }
}

ImGuiFilamentPanel::ModeAvailability ImGuiFilamentPanel::mode_availability_from_device(const DM::Device& device) const
{
    auto has_box_type = [&](int box_type) -> bool {
        if (!device.valid)
            return false;
        return std::any_of(device.materialBoxes.begin(), device.materialBoxes.end(),
            [box_type](const DM::MaterialBox& box) { return box.box_type == box_type; });
    };

    auto has_any_colored_material = [&](int box_type) -> bool {
        if (!device.valid)
            return false;
        for (const auto& box : device.materialBoxes) {
            if (box.box_type != box_type)
                continue;
            for (const auto& m : box.materials) {
                if (!m.color.empty())
                    return true;
            }
        }
        return false;
    };

    const bool cfs_box_present = has_box_type(0) || has_box_type(2);
    const bool ext_box_present = has_box_type(1);

    ModeAvailability avail;
    // CFS must have real consumable colors to be selectable.
    avail.show_cfs = cfs_box_present && (has_any_colored_material(0) || has_any_colored_material(2));
    // External rack:
    // - If device reports an external box, require at least one colored material to show it.
    // - If device reports no box info at all, still allow external rack as fallback.
    avail.show_external = !device.valid ? true : (ext_box_present ? has_any_colored_material(1) : true);
    return avail;
}

ImGuiFilamentPanel::Mode ImGuiFilamentPanel::resolve_mode_for_device(const DM::Device& device, Mode desired) const
{
    const ModeAvailability avail = mode_availability_from_device(device);
    if (desired == Mode::CFS) {
        if (avail.show_cfs)
            return Mode::CFS;
        if (avail.show_external)
            return Mode::External;
        return Mode::External;
    }

    if (avail.show_external)
        return Mode::External;
    if (avail.show_cfs)
        return Mode::CFS;
    return Mode::External;
}

void ImGuiFilamentPanel::reset()
{
    auto& ui_state = transient_ui_state();
    ui_state.add_palette_anchor = ImVec2(0.f, 0.f);
    ui_state.open_add_palette_requested = false;
    ui_state.pending_delete_row_index = -1;
    ui_state.open_delete_confirm_requested = false;

    m_items.clear();
    m_material_options.clear();
    m_popup_target = -1;
    m_initialized = false;
    m_mode = Mode::CFS;
    m_preview_view_state = PreviewViewState{};
    m_last_sig = 0;
    m_last_popup_frame_drawn = -1;
    m_hover_preview_override = HoverPreviewOverride{};
    if (m_thumbnail_preview)
        m_thumbnail_preview->reset();
    m_thumb_base_plate = nullptr;
    m_thumb_base_lit.reset();
    m_thumb_base_no_light.reset();
    m_thumb_base_rgb.clear();
    m_thumb_recolor_dirty = false;
}

void ImGuiFilamentPanel::invalidate_preview_thumbnail_cache(bool invalidate_plate_thumbnail, bool invalidate_local_preview_cache)
{
    PartPlateList& plate_list = wxGetApp().plater()->get_partplate_list();
    PartPlate* plate = plate_list.get_curr_plate();
    if (invalidate_plate_thumbnail && plate != nullptr) {
        plate->thumbnail_data.reset();
        plate->no_light_thumbnail_data.reset();
    }

    if (invalidate_local_preview_cache) {
        if (m_thumbnail_preview)
            m_thumbnail_preview->reset();
    }

    m_thumb_base_plate = nullptr;
    m_thumb_base_lit.reset();
    m_thumb_base_no_light.reset();
    m_thumb_base_rgb.clear();
    m_thumb_recolor_dirty = true;
}


bool ImGuiFilamentPanel::preview_supports_rotation(PreviewViewType type)
{
    return type != PreviewViewType::Top;
}

int ImGuiFilamentPanel::preview_direction_count(PreviewViewType type)
{
    switch (type) {
    case PreviewViewType::Iso:
        return 4;
    case PreviewViewType::Front:
        return 4;
    case PreviewViewType::Top:
    default:
        return 1;
    }
}

void ImGuiFilamentPanel::rotate_preview_direction_index(int& index, int step, int count)
{
    if (count <= 1) {
        index = 0;
        return;
    }

    int normalized = index % count;
    if (normalized < 0)
        normalized += count;
    normalized = (normalized + step) % count;
    if (normalized < 0)
        normalized += count;
    index = normalized;
}

void ImGuiFilamentPanel::rotate_preview_left()
{
    if (!preview_supports_rotation(m_preview_view_state.type))
        return;

    switch (m_preview_view_state.type) {
    case PreviewViewType::Iso:
        rotate_preview_direction_index(m_preview_view_state.iso_direction_index, -1, preview_direction_count(PreviewViewType::Iso));
        break;
    case PreviewViewType::Front:
        rotate_preview_direction_index(m_preview_view_state.front_direction_index, -1, preview_direction_count(PreviewViewType::Front));
        break;
    case PreviewViewType::Top:
    default:
        return;
    }

    invalidate_preview_thumbnail_cache(true, false);
}

void ImGuiFilamentPanel::rotate_preview_right()
{
    if (!preview_supports_rotation(m_preview_view_state.type))
        return;

    switch (m_preview_view_state.type) {
    case PreviewViewType::Iso:
        rotate_preview_direction_index(m_preview_view_state.iso_direction_index, 1, preview_direction_count(PreviewViewType::Iso));
        break;
    case PreviewViewType::Front:
        rotate_preview_direction_index(m_preview_view_state.front_direction_index, 1, preview_direction_count(PreviewViewType::Front));
        break;
    case PreviewViewType::Top:
    default:
        return;
    }

    invalidate_preview_thumbnail_cache(true, false);
}

ImGuiFilamentPanel::PreviewThumbnailRenderParams ImGuiFilamentPanel::build_preview_thumbnail_render_params() const
{
    PreviewThumbnailRenderParams params;

    switch (m_preview_view_state.type) {
    case PreviewViewType::Top:
        params.use_top_view = true;
        params.view_type.clear();
        break;
    case PreviewViewType::Front: {
        static const char* kFrontViews[] = { "front", "right", "rear", "left" };
        const int count = preview_direction_count(PreviewViewType::Front);
        int index = m_preview_view_state.front_direction_index % count;
        if (index < 0)
            index += count;
        params.view_type = kFrontViews[index];
        break;
    }
    case PreviewViewType::Iso: {
        static const char* kIsoViews[] = { "iso", "iso_front_right", "iso_rear_right", "iso_rear_left" };
        const int count = preview_direction_count(PreviewViewType::Iso);
        int index = m_preview_view_state.iso_direction_index % count;
        if (index < 0)
            index += count;
        params.view_type = kIsoViews[index];
        break;
    }
    default:
        params.view_type = "iso";
        break;
    }

    return params;
}

ImVec4 ImGuiFilamentPanel::hex_to_imvec4(const std::string &hex_in)
{
    std::string hex = clamp_hex(hex_in);
    unsigned int r = 255, g = 255, b = 255, a = 255;
    if (hex.size() == 7 || hex.size() == 9) {
        // #RRGGBB[AA]
        unsigned int v = 0;
        std::stringstream ss;
        ss << std::hex << hex.substr(1);
        ss >> v;
        if (hex.size() == 7) {
            r = (v >> 16) & 0xFF; g = (v >> 8) & 0xFF; b = (v) & 0xFF; a = 255;
        } else {
            r = (v >> 24) & 0xFF; g = (v >> 16) & 0xFF; b = (v >> 8) & 0xFF; a = (v) & 0xFF;
        }
    }
    return ImVec4(r/255.f, g/255.f, b/255.f, a/255.f);
}

std::string ImGuiFilamentPanel::imvec4_to_hex(const ImVec4 &c)
{
    auto to_u8 = [](float x)->unsigned { float v = x; if (v < 0) v = 0; if (v > 1) v = 1; return (unsigned)std::round(v * 255.f); };
    unsigned r = to_u8(c.x), g = to_u8(c.y), b = to_u8(c.z), a = to_u8(c.w);
    std::ostringstream os;
    if (a == 255) {
        os << "#" << std::uppercase << std::setfill('0') << std::hex
           << std::setw(2) << r << std::setw(2) << g << std::setw(2) << b;
    } else {
        os << "#" << std::uppercase << std::setfill('0') << std::hex
           << std::setw(2) << r << std::setw(2) << g << std::setw(2) << b << std::setw(2) << a;
    }
    return os.str();
}

bool ImGuiFilamentPanel::is_dark_text_on(const ImVec4 &bg)
{
    const float brightness = (bg.x * 299.f + bg.y * 587.f + bg.z * 114.f) / 1000.f;
    return brightness > 0.5f; // true => use dark text
}

bool ImGuiFilamentPanel::can_add_scene_color() const
{
    return m_items.size() < 64;
}

void ImGuiFilamentPanel::draw_capsule(const ImVec2& pos, const ImVec2& size, const ImVec4& bg, const char* text, float rounding)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 bg_col = ImGui::GetColorU32(bg);
    const ImU32 border_col = ImGui::GetColorU32(ImGuiCol_Border);
    dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), bg_col, rounding);
    dl->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), border_col, rounding, 0, 1.0f);

    ImU32 txt_col = ImGui::GetColorU32(is_dark_text_on(bg) ? ImVec4(0,0,0,1) : ImVec4(1,1,1,1));
    ImVec2 text_sz = ImGui::CalcTextSize(text);
    ImVec2 text_pos(pos.x + (size.x - text_sz.x) * 0.5f, pos.y + (size.y - text_sz.y) * 0.5f);
    dl->AddText(text_pos, txt_col, text);
}

static void delete_scene_color_at(ImGuiFilamentPanel& panel, int idx)
{
    if (idx < 0)
        return;

    auto* bundle = GUI::wxGetApp().preset_bundle;
    if (bundle == nullptr)
        return;

    const int filament_count = static_cast<int>(bundle->filament_presets.size());
    if (filament_count <= 1 || idx >= filament_count)
        return;

    if (GImGui != nullptr && GImGui->OpenPopupStack.Size > 0)
        ImGui::ClosePopupToLevel(0, true);
    GUI::wxGetApp().plater()->sidebar().delete_filament(static_cast<size_t>(idx), -1);
    panel.on_scene_reloaded();
    panel.refresh_items_from_config();
    if (panel.on_delete_filament)
        panel.on_delete_filament(idx);
}

static ImVec4 popup_rgba(float r, float g, float b, float a)
{
    return ImVec4(r, g, b, a);
}

static ImU32 popup_col(const ImVec4& c)
{
    return ImGui::GetColorU32(c);
}

static void draw_add_popup_section_label(const char* title, const char* subtitle, float scale)
{
    ImGui::TextUnformatted(title);
    if (subtitle != nullptr && subtitle[0] != '\0') {
        ImGui::Dummy(ImVec2(0.f, 2.f * scale));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f, 0.66f, 0.73f, 1.0f));
        ImGui::TextUnformatted(subtitle);
        ImGui::PopStyleColor();
    }
}

static bool draw_add_popup_color_tile(const char* id, const ImVec4& color, const char* label, float scale)
{
    const ImVec2 tile_size(56.f * scale, 62.f * scale);
    ImGui::InvisibleButton(id, tile_size);
    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(min, max, popup_col(hovered ? popup_rgba(0.15f, 0.18f, 0.22f, 0.98f) : popup_rgba(0.11f, 0.14f, 0.17f, 0.96f)), 13.f * scale);
    draw_list->AddRect(min, max, popup_col(hovered ? popup_rgba(1.f, 1.f, 1.f, 0.17f) : popup_rgba(1.f, 1.f, 1.f, 0.08f)), 13.f * scale, 0, hovered ? 1.6f : 1.0f);

    const ImVec2 swatch_min(min.x + 8.f * scale, min.y + 8.f * scale);
    const ImVec2 swatch_max(max.x - 8.f * scale, min.y + 34.f * scale);
    const ImVec4 top_tint(std::min(color.x + 0.08f, 1.f), std::min(color.y + 0.08f, 1.f), std::min(color.z + 0.08f, 1.f), 1.f);
    draw_list->AddRectFilledMultiColor(swatch_min, swatch_max, popup_col(top_tint), popup_col(top_tint), popup_col(color), popup_col(color));
    draw_list->AddRect(swatch_min, swatch_max, IM_COL32(255, 255, 255, hovered ? 90 : 48), 10.f * scale, 0, hovered ? 1.6f : 1.0f);

    const ImVec2 text_size = ImGui::CalcTextSize(label);
    draw_list->AddText(ImVec2(min.x + (tile_size.x - text_size.x) * 0.5f, max.y - 19.f * scale), IM_COL32(188, 196, 204, 255), label);

    if (hovered) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        ImGui::SetTooltip("%s", label);
    }

    return clicked;
}

static bool draw_add_popup_custom_button(const char* label, const char* subtitle, float width, float scale)
{
    const ImVec2 button_size(width, 40.f * scale);
    ImGui::InvisibleButton("##add_popup_custom", button_size);
    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(min, max, popup_col(hovered ? popup_rgba(0.23f, 0.18f, 0.10f, 0.96f) : popup_rgba(0.18f, 0.14f, 0.09f, 0.94f)), 12.f * scale);
    draw_list->AddRect(min, max, popup_col(hovered ? popup_rgba(0.97f, 0.74f, 0.33f, 0.74f) : popup_rgba(0.88f, 0.66f, 0.28f, 0.36f)), 12.f * scale, 0, hovered ? 1.6f : 1.0f);

    const ImVec2 plus_center(min.x + 16.f * scale, min.y + button_size.y * 0.5f);
    draw_list->AddLine(ImVec2(plus_center.x - 4.f * scale, plus_center.y), ImVec2(plus_center.x + 4.f * scale, plus_center.y), IM_COL32(247, 201, 111, 255), 1.8f);
    draw_list->AddLine(ImVec2(plus_center.x, plus_center.y - 4.f * scale), ImVec2(plus_center.x, plus_center.y + 4.f * scale), IM_COL32(247, 201, 111, 255), 1.8f);
    draw_list->AddText(ImVec2(min.x + 28.f * scale, min.y + 6.f * scale), IM_COL32(250, 228, 184, 255), label);
    draw_list->AddText(ImVec2(min.x + 28.f * scale, min.y + 20.f * scale), IM_COL32(184, 160, 122, 255), subtitle);

    if (hovered)
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

    return clicked;
}

static void draw_add_popup_info_pill(const char* label, float scale)
{
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const ImVec2 text_size = ImGui::CalcTextSize(label);
    const ImVec2 size(std::max(108.f * scale, text_size.x + 22.f * scale), 26.f * scale);
    ImGui::Dummy(size);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), popup_col(popup_rgba(0.14f, 0.22f, 0.18f, 0.98f)), 999.f);
    draw_list->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), popup_col(popup_rgba(0.30f, 0.74f, 0.47f, 0.34f)), 999.f);
    draw_list->AddCircleFilled(ImVec2(pos.x + 10.f * scale, pos.y + size.y * 0.5f), 3.6f * scale, IM_COL32(78, 210, 124, 255));
    draw_list->AddText(ImVec2(pos.x + 18.f * scale, pos.y + (size.y - text_size.y) * 0.5f), IM_COL32(216, 241, 224, 255), label);
}

void ImGuiFilamentPanel::refresh_items_from_config()
{
    auto *bundle = GUI::wxGetApp().preset_bundle;
    if (!bundle) return;

    DynamicPrintConfig *cfg = &bundle->project_config;
    auto *colors_opt = static_cast<ConfigOptionStrings*>(cfg->option("filament_colour"));
    if (!colors_opt) return;

    size_t count = colors_opt->values.size();
    if (m_items.size() != count) 
        m_items.assign(count, {});

    for (size_t i = 0; i < count; ++i) {
        auto &it = m_items[i];
        it.index = static_cast<int>(i);
        it.color = hex_to_imvec4(colors_opt->values[i]);

        // Best-effort to fetch a readable preset label for this extruder slot.
        std::string display;
        std::string preset_id;
        if (i < bundle->filament_presets.size()) {
            preset_id = bundle->filament_presets[i];
            if (!preset_id.empty()) {
                if (auto *preset = bundle->filaments.find_preset(preset_id)) {
                    display = preset->name.empty() ? preset_id : preset->name;
                    std::string ftype; 
                    preset->get_filament_type(ftype);
                    it.type_label = preset->alias.empty() ? ftype : preset->alias;
                } else {
                    display = preset_id;
                }
            }
        }

        it.preset_display = display;

        if (m_mode == Mode::External) 
            it.sync_label = "EXT";
        else {
            int box = int(i / 4) + 1; 
            char letter = char('A' + (i % 4));
            it.sync_label = std::to_string(box) + letter;
        }
    }
    m_initialized = true;
    m_thumb_recolor_dirty = true;
}

void ImGuiFilamentPanel::commit_color_change(int idx, const ImVec4 &c)
{
    auto *bundle = GUI::wxGetApp().preset_bundle;
    if (!bundle) return;
    DynamicPrintConfig *cfg = &bundle->project_config;
    auto  colors = static_cast<ConfigOptionStrings*>(cfg->option("filament_colour")->clone());
    if (!colors) return;
    if (idx < 0 || idx >= (int)colors->values.size()) return;

    colors->values[idx] = imvec4_to_hex(c);
    DynamicPrintConfig cfg_new = *cfg;
    cfg_new.set_key_value("filament_colour", colors);

    cfg->apply(cfg_new);
    GUI::wxGetApp().plater()->update_project_dirty_from_presets();
    GUI::wxGetApp().preset_bundle->export_selections(*GUI::wxGetApp().app_config);
    GUI::wxGetApp().plater()->on_config_change(cfg_new);

    m_thumb_recolor_dirty = true;
}

bool ImGuiFilamentPanel::is_item_matched(const ImGuiFilamentItemState &item)
{
    if(item.device_match_slot.empty()) {
        return false;
    }

    return true;
}

void ImGuiFilamentPanel::render_item(ImGuiFilamentItemState &item)
{
    ImGui::PushID(item.index);
    const float scale = wxGetApp().plater()->get_current_canvas3D()->get_scale();

    // Top capsule
    const ImVec2 cap_size(104.f * scale, 32.f * scale);
    const ImVec2 cap_pos  = ImGui::GetCursorScreenPos();
    draw_capsule(cap_pos, cap_size, item.color,
                 item.type_label.empty() ? (item.preset_display.empty() ? "<type>" : item.preset_display.c_str()) : item.type_label.c_str());
    ImGui::Dummy(cap_size);

    // Conditionally show mapping UI only when device has materials and signature changed
    if(!is_current_device_valid() || !is_item_matched(item) ){
        ImGui::PopID();
        return;
    }

    // Downward grey triangle linking top capsule to mapping card
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float tri_w = 12.f * scale;
        const float tri_h = 6.f * scale;
        const float gap   = 6.f * scale;
        ImVec2 p_tip(cap_pos.x + cap_size.x * 0.5f, cap_pos.y + cap_size.y + gap + tri_h);
        ImVec2 p_l(p_tip.x - tri_w * 0.5f, p_tip.y - tri_h);
        ImVec2 p_r(p_tip.x + tri_w * 0.5f, p_tip.y - tri_h);
        const ImU32 tri_col = ImGui::GetColorU32(ImVec4(0.7f, 0.72f, 0.75f, 1.0f));
        dl->AddTriangleFilled(p_l, p_r, p_tip, tri_col);
    }
    ImGui::Dummy(ImVec2(1, 12 * scale));

    // Bottom mapping widget
    const ImVec2 map_pos  = ImGui::GetCursorScreenPos();
    const ImVec2 map_size(104.f * scale, 44.f * scale);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 map_bg = IM_COL32(255, 255, 255, 255); // white
    const ImVec2 map_br(map_pos.x + map_size.x, map_pos.y + map_size.y);
    const bool map_hovered = ImGui::IsMouseHoveringRect(map_pos, map_br, false);
    const bool map_clicked = map_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    const bool allow_popup = (m_mode == Mode::CFS);
    {
        char popup_id_click[64];
        std::snprintf(popup_id_click, sizeof(popup_id_click), "MapPopup##%d", item.index);
        const bool already_open = ImGui::IsPopupOpen(popup_id_click);
        if (allow_popup && map_clicked && !already_open) {
            m_popup_target = item.index;
            ImGui::OpenPopup(popup_id_click);
        }

    }

    const ImU32 map_border_default = IM_COL32(208, 212, 222, 255); // light grey border
    const ImU32 map_border_hover   = ImGui::GetColorU32(ImVec4(0.08f, 0.65f, 0.25f, 1.0f)); // green highlight
    const ImU32 map_border = map_hovered ? map_border_hover : map_border_default;
    dl->AddRectFilled(map_pos, map_br, map_bg, 6.f);
    dl->AddRect(map_pos, map_br, map_border, 6.f, 0, map_hovered ? 2.0f : 1.0f);

    // dot with white inner circle (concentric), centered vertically
    const ImU32  dot_col    = ImGui::GetColorU32(item.match_color);
    const float  dot_r_px   = 10.0f * scale;
    const float  dot_pad_l  = 4.0f * scale; // left padding from card edge to circle edge
    const ImVec2 dot_center(map_pos.x + dot_pad_l + dot_r_px, map_pos.y + map_size.y * 0.5f);
    dl->AddCircleFilled(dot_center, dot_r_px, dot_col);
    // When match color is near-white, add a darker outline so the dot remains visible on the white card.
    const float min_chan = std::min(std::min(item.match_color.x, item.match_color.y), item.match_color.z);
    if (min_chan > 0.85f) {
        const float border_thickness = 1.5f * scale;
        dl->AddCircle(dot_center, dot_r_px, IM_COL32(140, 144, 152, 255), 0, border_thickness);
    }
    // inner white circle to create a ring effect
    const float  ring_thickness = 8.0f * scale;
    const float  inner_r = dot_r_px - ring_thickness;
    if (inner_r > 0.0f)
        dl->AddCircleFilled(dot_center, inner_r, IM_COL32(255, 255, 255, 255));

    // compute horizontal text bounds in card
    const float x_l = dot_center.x + dot_r_px + 8.f * scale; // text starts to the right of large dot
    const float x_r = map_pos.x + map_size.x - 24.f * scale;
    const float x_mid = (x_l + x_r) * 0.5f;

    const ImU32 map_text_color = IM_COL32(38, 44, 52, 255);  // dark text for white card

    // top text centered between x_l and x_r
    {
        const char* sync_txt = item.device_match_slot.c_str();
        ImVec2 tsize = ImGui::CalcTextSize(sync_txt);
        float x = x_mid - tsize.x * 0.5f;
        if (x < x_l) x = x_l; if (x + tsize.x > x_r) x = x_r - tsize.x;
        ImGui::SetCursorScreenPos(ImVec2(x, map_pos.y + 8.f * scale));
        ImGui::PushStyleColor(ImGuiCol_Text, map_text_color);
        ImGui::TextUnformatted(sync_txt);
        ImGui::PopStyleColor();
    }

    // divider line between two text rows
    {
        const float y_div = map_pos.y + 24.f * scale;
        dl->AddLine(ImVec2(x_l, y_div), ImVec2(x_r, y_div), IM_COL32(208,212,222,255), 1.0f);
    }

     //bottom text centered between x_l and x_r (slightly smaller font)
    {
        const char* mat_txt = item.type_label.empty() ? (item.preset_display.empty() ? "<material>" : item.preset_display.c_str()) : item.type_label.c_str();
        ImFont* font = ImGui::GetFont();
        const float base_fs = ImGui::GetFontSize();
        const float small_fs = base_fs * 0.90f; // 10% smaller
        ImVec2 tsize = font->CalcTextSizeA(small_fs, FLT_MAX, 0.0f, mat_txt);
        float x = x_mid - tsize.x * 0.5f;
        if (x < x_l) x = x_l; if (x + tsize.x > x_r) x = x_r - tsize.x;
        ImVec2 pos = ImVec2(x, map_pos.y + 28.f * scale);
        ImGui::GetWindowDrawList()->AddText(font, small_fs, pos, map_text_color, mat_txt);
    }
    
     // right chevron '>' as grey
    {
        const ImU32 chev = IM_COL32(160, 164, 172, 255);
        ImVec2 c = ImVec2(map_pos.x + map_size.x - 14.f * scale, map_pos.y + map_size.y * 0.5f);
        const float len = 6.f * scale;
        dl->AddLine(ImVec2(c.x - len, c.y - len), ImVec2(c.x, c.y), chev, 2.0f);
        dl->AddLine(ImVec2(c.x - len, c.y + len), ImVec2(c.x, c.y), chev, 2.0f);
    }

    // context menu on map
    ImGui::SetCursorScreenPos(map_pos);
    ImGui::InvisibleButton("##map_area", map_size);

    // Left-click popup: show available device materials mapped by CFS box
    if (allow_popup) {
        char popup_id[64]; 
        std::snprintf(popup_id, sizeof(popup_id), "MapPopup##%d", item.index);
        bool pushed_bg = false;
        if (ImGui::IsPopupOpen(popup_id))
        { 
            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(1,1,1,1));
            pushed_bg = true; 
        }

        if (ImGui::BeginPopup(popup_id)) {
            // Allow building interactive items on duplicate calls within the same frame,
            // but suppress duplicate draw primitives to avoid visual duplication.
            const int  cur_frame    = ImGui::GetFrameCount();
            const bool is_owner     = (m_popup_target == item.index);
            bool       suppress_draw = (m_last_popup_frame_drawn == cur_frame);
            if (!suppress_draw && is_owner) m_last_popup_frame_drawn = cur_frame;
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f * scale);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f * scale, 8.f * scale));

            ImDrawList* dlp = ImGui::GetWindowDrawList();
            // Build view options directly from current device_data
            std::vector<MaterialOption> view_options;
            {
                const DM::Device& dev = DM::DataCenter::Ins().get_current_device_data();
                if (dev.valid) {
                    for (const auto& box : dev.materialBoxes) {
                        if (box.box_type != 0 && box.box_type != 2) continue; // only CFS and CFS-mini/EXT
                        for (size_t si = 0; si < box.materials.size(); ++si) {
                            const auto &m = box.materials[si];
                            // For CFS, only material_id 0..3 are valid display slots (A/B/C/D)
                            if (box.box_type == 0 && (m.material_id < 0 || m.material_id > 3))
                                continue;
                            // De-duplicate by (box_id, material_id)
                            bool exists = false;
                            for (const auto &ex : view_options) {
                                if (ex.box_id == box.box_id && ex.material.material_id == m.material_id) { exists = true; break; }
                            }
                            if (exists) continue;
                            MaterialOption opt; 
                            opt.box_id = box.box_id; 
                            opt.slot_index = (int)si; 
                            opt.box_type = box.box_type; 
                            opt.material = m; 
                            view_options.push_back(std::move(opt));
                        }
                    }
                }
            }
            if (view_options.empty()) {
                ImGui::TextUnformatted(u8"\u6682\u65e0\u53ef\u7528\u7684\u8bbe\u5907\u8017\u6750");
            } else {
                // Group options by box id
                struct Group { int box_id; std::vector<int> idxs; };
                std::map<int, Group> groups;
                for (int i = 0; i < (int)view_options.size(); ++i) {
                    const auto &opt = view_options[i];
                    auto it = groups.find(opt.box_id);
                    if (it == groups.end()) it = groups.emplace(opt.box_id, Group{opt.box_id,{}}).first;
                    it->second.idxs.push_back(i);
                }
                const ImVec2 cell_size(84.f * scale, 45.f * scale);
                const float  cell_gap = 8.f * scale;
                for (auto &kv : groups) {
                    std::string header = "CFS" + std::to_string(kv.first);
                    if (!suppress_draw) ImGui::TextUnformatted(header.c_str());
                    else {
                        ImVec2 sz = ImGui::CalcTextSize(header.c_str());
                        ImGui::Dummy(ImVec2(0.f, sz.y));
                    }
                    for (size_t n = 0; n < kv.second.idxs.size(); ++n) {
                        const int opt_index = kv.second.idxs[n];
                        const auto &opt = view_options[opt_index];
                        if (n > 0) ImGui::SameLine(0.f, cell_gap);
                        ImVec2 p = ImGui::GetCursorScreenPos();
                        // Stable ID based on (box_id, material_id) to avoid index reuse issues
                        std::string btn_id = std::string("##mopt") + std::to_string(opt.box_id) + ":" + std::to_string(opt.material.material_id);
                        ImGui::InvisibleButton(btn_id.c_str(), cell_size);
                        const bool hov_raw = ImGui::IsItemHovered();
                        const bool clk = ImGui::IsItemClicked();
                        ImVec2 rmin = ImGui::GetItemRectMin();
                        ImVec2 rmax = ImGui::GetItemRectMax();
                        const bool no_color = opt.material.color.empty();
                        ImVec4 bg = hex_to_imvec4(opt.material.color);
                        ImU32 bg_col = ImGui::GetColorU32(bg);

                        // Determine disabled state before hover effect
                        // Disabled when type mismatch or no color
                        bool type_mismatch = false;
                        if (!item.type_label.empty() && !opt.material.name.empty()) {
                            std::string a = item.type_label; 
                            std::string b = opt.material.name;
                            for (auto &ch : a) ch = (char)std::tolower((unsigned char)ch);
                            for (auto &ch : b) ch = (char)std::tolower((unsigned char)ch);
                            type_mismatch = (a != b);
                        }
                        const bool disable_hover = (type_mismatch || no_color);
                        const bool hov = !disable_hover && hov_raw;
                        ImU32 border_col = hov ? ImGui::GetColorU32(ImVec4(0.08f, 0.65f, 0.25f, 1.0f))
                                               : IM_COL32(208, 212, 222, 255);
                        
                        // When the device material cannot match current filament, draw disabled texture background
                        // Also treat no-color as disabled rendering
                        if (!suppress_draw) {
                            if ((type_mismatch || no_color) && m_filament_disable_icon_tex != nullptr) {
                                dlp->AddImage(m_filament_disable_icon_tex, rmin, rmax);
                            } else {
                                dlp->AddRectFilled(rmin, rmax, bg_col, 6.f);
                            }
                            dlp->AddRect(rmin, rmax, border_col, 6.f, 0, hov ? 2.0f : 1.0f);
                        }

                        // Prefer dark (black) text when the disable mask is shown to ensure contrast
                        const bool prefer_dark_due_to_mask = ((type_mismatch || no_color) && m_filament_disable_icon_tex != nullptr);
                        const bool dark = prefer_dark_due_to_mask ? true : is_dark_text_on(bg);
                        ImU32 txt_col = ImGui::GetColorU32(dark ? ImVec4(0,0,0,1) : ImVec4(1,1,1,1));
                        char top[16];
                        if (opt.box_type == 2) std::snprintf(top, sizeof(top), "EXT");
                        else std::snprintf(top, sizeof(top), "%d%c", opt.box_id, char('A' + (opt.material.material_id % 26)));
                        //const char* bottom = opt.material.name.c_str();
                        // Bottom text logic similar to ColorMatch.vue::cfsType
                        // '/' => slot has no consumables and cannot be mapped
                        // '?' => consumables exist but not edited (no color)
                        std::string bottom_str;
                        if (opt.material.state == -1 || (opt.material.state == 0 && no_color))
                            bottom_str = "/";
                        else if (no_color)
                            bottom_str = "?";
                        else
                            bottom_str = !opt.material.type.empty() ? opt.material.type : opt.material.name;
                        const char* bottom = bottom_str.c_str();
                        ImVec2 t1 = ImGui::CalcTextSize(top);
                        ImVec2 t2 = ImGui::CalcTextSize(bottom);
                        float top_y = rmin.y + 6.f * scale;
                        float x1 = rmin.x + (cell_size.x - t1.x) * 0.5f;
                        if (!suppress_draw)
                            dlp->AddText(ImVec2(x1, p.y + 6.f * scale), txt_col, top);

                        // divider line between two text rows inside option cell
                        float y_div = top_y + t1.y + 2.f * scale;
                        float x_l = p.x + 6.f * scale;
                        float x_r = p.x + cell_size.x - 6.f * scale;
                        ImU32 divider_line_color = ImGui::GetColorU32(prefer_dark_due_to_mask ? ImVec4(0,0,0,1) : ImVec4(208/255.0f,212/255.0f,222/255.0f,1.0f));
                        if (!suppress_draw)
                            dlp->AddLine(ImVec2(rmin.x + 6.f * scale, y_div), ImVec2(rmin.x + cell_size.x - 6.f * scale, y_div), divider_line_color, 1.0f);
                        float bottom_y = y_div + 2.f * scale;

                        float x2 = rmin.x + (cell_size.x - t2.x) * 0.5f;
                        if (!suppress_draw)
                            dlp->AddText(ImVec2(x2, bottom_y), txt_col, bottom);

                        if (clk && is_owner && !no_color) {
                            ImVec4 newc = hex_to_imvec4(opt.material.color);
                            const int target_idx = m_popup_target;
                            if (target_idx >= 0 && target_idx < (int)m_items.size()) {
                                auto &dst = m_items[target_idx];
                                //dst.color = newc;
                                //dst.type_label = opt.material.name;
                                dst.sync_label = (opt.box_type == 2) ? std::string("EXT") : (std::to_string(opt.box_id) + char('A' + (opt.material.material_id % 26)));
                                dst.device_match_slot = dst.sync_label;
                                dst.match_color = newc;
                                //commit_color_change(target_idx, newc);
                                m_thumb_recolor_dirty = true;
                            }
                        }
                    }
                    ImGui::Dummy(ImVec2(1.f, 6.f * scale));
                }
            }
            ImGui::PopStyleVar(2);
            if (pushed_bg) 
                ImGui::PopStyleColor();
            ImGui::EndPopup();
        }
        else if(pushed_bg) {
            ImGui::PopStyleColor();
        }
    }

    ImGui::SetCursorScreenPos(map_pos);
    ImGui::Dummy(map_size);

    ImGui::PopID();
}

void ImGuiFilamentPanel::render_external_item(float scale)
{
    ImGui::PushID("external_item");

    // Bottom mapping widget (EXT-only)
    const ImVec2 map_pos = ImGui::GetCursorScreenPos();
    const ImVec2 map_size(104.f * scale, 44.f * scale);
    ImDrawList*  dl = ImGui::GetWindowDrawList();
    const ImVec2 map_br(map_pos.x + map_size.x, map_pos.y + map_size.y);

    // External item is currently non-interactive here.
    const bool allow_popup = false;
    const bool map_hovered = allow_popup && ImGui::IsMouseHoveringRect(map_pos, map_br, false);

    const ImU32 map_bg            = IM_COL32(255, 255, 255, 255);
    const ImU32 map_border_default = IM_COL32(208, 212, 222, 255);
    const ImU32 map_border_hover   = ImGui::GetColorU32(ImVec4(0.08f, 0.65f, 0.25f, 1.0f));
    const ImU32 map_border         = map_hovered ? map_border_hover : map_border_default;
    const ImU32 map_text_color     = IM_COL32(38, 44, 52, 255);

    // Resolve external rack material (box_type == 1) from current device.
    ImVec4      rack_color = ImVec4(1, 1, 1, 1);
    std::string mat_txt_str;
    {
        const DM::Device& dev = DM::DataCenter::Ins().get_current_device_data();
        const DM::Material* chosen = nullptr;
        if (dev.valid) {
            for (const auto& box : dev.materialBoxes) {
                if (box.box_type != 1)
                    continue;

                for (const auto& m : box.materials) {
                    if (m.color.empty())
                        continue;
                    if (m.selected) {
                        chosen = &m;
                        break; // selected wins for this box
                    }
                    if (!chosen)
                        chosen = &m;
                }
                if (chosen && chosen->selected)
                    break; // selected found, stop searching other boxes
            }
        }

        if (chosen != nullptr) {
            rack_color = hex_to_imvec4(chosen->color);
            if (!chosen->name.empty())
                mat_txt_str = chosen->name;
            else if (!chosen->type.empty())
                mat_txt_str = chosen->type;
        }
    }
    if (mat_txt_str.empty())
        mat_txt_str = "<material>";

    const char* sync_txt = "EXT";
    const char* mat_txt  = mat_txt_str.c_str();

    dl->AddRectFilled(map_pos, map_br, map_bg, 6.f);
    dl->AddRect(map_pos, map_br, map_border, 6.f, 0, map_hovered ? 2.0f : 1.0f);

    // dot geometry (centered vertically)
    const float  dot_r_px  = 10.0f * scale;
    const float  dot_pad_l = 4.0f * scale;
    const ImVec2 dot_center(map_pos.x + dot_pad_l + dot_r_px, map_pos.y + map_size.y * 0.5f);

    // dot with white inner circle (concentric)
    const ImU32 dot_col = ImGui::GetColorU32(rack_color);
    dl->AddCircleFilled(dot_center, dot_r_px, dot_col);
    // When rack color is near-white, add a darker outline so the dot remains visible on the white card.
    const float min_chan = std::min(std::min(rack_color.x, rack_color.y), rack_color.z);
    if (min_chan > 0.85f) {
        const float border_thickness = 1.5f * scale;
        dl->AddCircle(dot_center, dot_r_px, IM_COL32(140, 144, 152, 255), 0, border_thickness);
    }
    const float ring_thickness = 8.0f * scale;
    const float inner_r        = dot_r_px - ring_thickness;
    if (inner_r > 0.0f)
        dl->AddCircleFilled(dot_center, inner_r, IM_COL32(255, 255, 255, 255));

    const float x_l   = dot_center.x + dot_r_px + 8.f * scale;
    const float x_r   = map_pos.x + map_size.x - (allow_popup ? 24.f * scale : 8.f * scale);
    const float x_mid = (x_l + x_r) * 0.5f;

    // top text centered between x_l and x_r
    {
        ImVec2 tsize = ImGui::CalcTextSize(sync_txt);
        float  x     = x_mid - tsize.x * 0.5f;
        if (x < x_l) x = x_l;
        if (x + tsize.x > x_r) x = x_r - tsize.x;
        ImGui::SetCursorScreenPos(ImVec2(x, map_pos.y + 8.f * scale));
        ImGui::PushStyleColor(ImGuiCol_Text, map_text_color);
        ImGui::TextUnformatted(sync_txt);
        ImGui::PopStyleColor();
    }

    // divider line between two text rows
    {
        const float y_div = map_pos.y + 24.f * scale;
        dl->AddLine(ImVec2(x_l, y_div), ImVec2(x_r, y_div), map_border_default, 1.0f);
    }

    // bottom text centered between x_l and x_r (slightly smaller font)
    {
        ImFont*     font     = ImGui::GetFont();
        const float base_fs  = ImGui::GetFontSize();
        const float small_fs = base_fs * 0.90f;
        ImVec2      tsize    = font->CalcTextSizeA(small_fs, FLT_MAX, 0.0f, mat_txt);
        float       x        = x_mid - tsize.x * 0.5f;
        if (x < x_l) x = x_l;
        if (x + tsize.x > x_r) x = x_r - tsize.x;
        dl->AddText(font, small_fs, ImVec2(x, map_pos.y + 28.f * scale), map_text_color, mat_txt);
    }

    ImGui::SetCursorScreenPos(map_pos);
    ImGui::InvisibleButton("##map_area", map_size);
    ImGui::SetCursorScreenPos(map_pos);
    ImGui::Dummy(map_size);

    ImGui::PopID();
}

void ImGuiFilamentPanel::Render()
{
    const DM::Device& device = DM::DataCenter::Ins().get_current_device_data();
    bool prefer_embedded_cfs_mode = false;
    if (m_embed_in_unified_left_panel && device.valid) {
        const Mode preferred_mode = resolve_mode_for_device(device, Mode::CFS);
        if (preferred_mode != m_mode) {
            m_mode = preferred_mode;
            m_last_sig = 0;
            prefer_embedded_cfs_mode = true;
        }
    }
    if (!m_initialized) {
        refresh_items_from_config();
        if (!prefer_embedded_cfs_mode)
            check_and_resolve_mode_by_current_device();
        m_last_sig = 0;
        check_device_filament_auto_mapping();
    } else if (prefer_embedded_cfs_mode) {
        refresh_items_from_config();
        check_device_filament_auto_mapping();
    }
    const float scale = GUI::wxGetApp().plater()->get_current_canvas3D()->get_scale();
    auto& ui_state = transient_ui_state();
    m_hover_preview_override.active = false;
    m_hover_preview_override.item_index = -1;

    ResidentFilamentMapping::RuntimeSignals signals;
    signals.device_is_online = device.valid && device.online;
    signals.device_supports_multi_color = (m_mode == Mode::CFS) && device.valid && device.isMultiColorDevice;
    signals.device_materials_available = is_current_device_valid();
    signals.scene_color_count = static_cast<int>(m_items.size());
    signals.has_cached_mapping_result = ResidentFilamentMappingAdapter::has_cached_mapping_result(m_items);

    const ResidentFilamentMapping::UnifiedOutputInput unified_output =
        ResidentFilamentMappingAdapter::collect_unified_output_input(device);
    const ResidentFilamentMapping::UiModel ui_model = ResidentFilamentMapping::build_ui_model(
        signals,
        ResidentFilamentMappingAdapter::collect_row_inputs_from_items(m_items, device),
        unified_output);
    const ResidentFilamentMappingAdapter::PopupOptionCatalog popup_catalog =
        ResidentFilamentMappingAdapter::build_popup_option_catalog(device);
    const ResidentFilamentMappingPopupPolicy::PopupMatchPolicyConfig popup_policy =
        ResidentFilamentMappingPopupPolicy::popup_policy_config_for_runtime(device, m_mode, signals);

    const float reserve_for_button = 0.f;
    const float max_list_height = 520.f * scale;
    float avail_h = ImGui::GetContentRegionAvail().y;
    float child_h = std::min(max_list_height, std::max(220.f * scale, avail_h - reserve_for_button));

    const float preview_w = 0.f;
    const float split_gap = 0.f;

    ResidentFilamentMappingView::PanelViewData view_data =
        ResidentFilamentMappingAdapter::build_panel_view_data(
            ui_model,
            popup_catalog,
            m_items,
            scale,
            child_h,
            preview_w,
            split_gap,
            popup_policy);
    view_data.add_enabled = can_add_scene_color();
    view_data.embed_in_unified_panel = m_embed_in_unified_left_panel;
    view_data.preview_view = static_cast<ResidentFilamentMappingView::PreviewViewType>(m_preview_view_state.type);
    view_data.show_preview_rotate_buttons = preview_supports_rotation(m_preview_view_state.type);
    view_data.preview_rotate_left_enabled = view_data.show_preview_rotate_buttons && preview_direction_count(m_preview_view_state.type) > 1;
    view_data.preview_rotate_right_enabled = view_data.preview_rotate_left_enabled;

    switch (m_preview_view_state.type) {
    case PreviewViewType::Front:
        view_data.preview_direction_hint = u8"\u524d/\u53f3/\u540e/\u5de6";
        break;
    case PreviewViewType::Iso:
        view_data.preview_direction_hint = u8"\u5de6\u524d/\u53f3\u524d/\u53f3\u540e/\u5de6\u540e";
        break;
    case PreviewViewType::Top:
    default:
        view_data.preview_direction_hint.clear();
        break;
    }

    ResidentFilamentMappingView::render_panel(view_data, {
        [this](int item_index, const std::string& selection_token) {
            apply_mapping_selection(item_index, selection_token);
        },
        [this, &popup_catalog](int item_index, const std::string& selection_token) {
            if (item_index < 0 || item_index >= (int)m_items.size())
                return;
            if (selection_token.empty())
                return;

            const auto* payload = ResidentFilamentMappingAdapter::find_popup_selection_payload(popup_catalog, selection_token);
            if (payload == nullptr)
                return;

            m_hover_preview_override.item_index = item_index;
            m_hover_preview_override.match_color = payload->material_color;
            m_hover_preview_override.active = true;
        },
        [&ui_state](int item_index) {
            ui_state.pending_scene_color_edit_index = item_index;
            ui_state.open_scene_color_dialog_requested = true;
        },
        [&ui_state](int item_index, const ImVec2& anchor_min, const ImVec2& anchor_max) {
            ui_state.pending_scene_material_popup_index = item_index;
            ui_state.scene_material_anchor = ImVec2(anchor_min.x, anchor_max.y + 6.f);
            ui_state.open_scene_material_popup_requested = true;
        },
        [&ui_state](const ImVec2& anchor_min, const ImVec2& anchor_max) {
            ui_state.add_palette_anchor = ImVec2((anchor_min.x + anchor_max.x) * 0.5f, anchor_max.y + 6.f);
            ui_state.open_add_palette_requested = true;
        },
        [&ui_state](int item_index) {
            ui_state.pending_delete_row_index = item_index;
            ui_state.open_delete_confirm_requested = true;
        },
        [this](ResidentFilamentMappingView::PreviewViewType view_type) {
            const PreviewViewType next_view = static_cast<PreviewViewType>(view_type);
            if (m_preview_view_state.type == next_view)
                return;
            m_preview_view_state.type = next_view;
            invalidate_preview_thumbnail_cache(true, false);
        },
        [this]() {
            rotate_preview_left();
        },
        [this]() {
            rotate_preview_right();
        },
        [this](float preview_width, float preview_height, float preview_scale) {
            render_thumbnail_preview(preview_width, preview_height, preview_scale);
        }
    });

    render_add_button_and_palette();

    if (ui_state.open_scene_color_dialog_requested) {
        ui_state.open_scene_color_dialog_requested = false;
        const int item_index = ui_state.pending_scene_color_edit_index;
        ui_state.pending_scene_color_edit_index = -1;

        if (item_index >= 0 && item_index < static_cast<int>(m_items.size())) {
            const ImVec4 current_color = m_items[item_index].color;
            const wxColour initial_color(
                static_cast<unsigned char>(std::round(std::max(0.f, std::min(1.f, current_color.x)) * 255.f)),
                static_cast<unsigned char>(std::round(std::max(0.f, std::min(1.f, current_color.y)) * 255.f)),
                static_cast<unsigned char>(std::round(std::max(0.f, std::min(1.f, current_color.z)) * 255.f)));

            std::string picked_hex;
            if (choose_filament_color_with_dialog(initial_color, _L("Please choose the filament colour"), picked_hex)) {
                commit_color_change(item_index, hex_to_imvec4(picked_hex));
                remap_item_with_match_color(item_index);
                on_scene_reloaded();
                refresh_items_from_config();
            }
        }
    }

    if (ui_state.open_scene_material_popup_requested) {
        if (GImGui != nullptr && GImGui->OpenPopupStack.Size > 0)
            ImGui::ClosePopupToLevel(0, true);
        ImGui::OpenPopup("resident_scene_material_popup");
        ui_state.open_scene_material_popup_requested = false;
    }

    ImGui::SetNextWindowPos(ui_state.scene_material_anchor, ImGuiCond_Appearing, ImVec2(0.f, 0.f));
    ImGui::SetNextWindowSize(ImVec2(236.f * scale, 0.f), ImGuiCond_Appearing);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16.f * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.f * scale, 10.f * scale));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.f * scale, 8.f * scale));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.10f, 0.12f, 0.15f, 0.99f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.f, 1.f, 1.f, 0.08f));
    if (ImGui::BeginPopup("resident_scene_material_popup")) {
        const int item_index = ui_state.pending_scene_material_popup_index;
        std::string current_label;
        if (item_index >= 0 && item_index < static_cast<int>(m_items.size()))
            current_label = m_items[item_index].type_label.empty() ? m_items[item_index].preset_display : m_items[item_index].type_label;

        const std::vector<std::string> options = build_scene_material_type_options(item_index, current_label);
        ImGui::TextUnformatted(u8"\u9009\u62e9\u573a\u666f\u8017\u6750\u7c7b\u578b");
        ImGui::Dummy(ImVec2(0.f, 2.f * scale));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.68f, 0.73f, 0.79f, 1.f));
        ImGui::TextUnformatted(u8"\u4fee\u6539\u540e\u5c06\u7528\u4e8e\u5f53\u524d\u573a\u666f\u989c\u8272\u7684\u5339\u914d\u4e0e\u6620\u5c04");
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0.f, 6.f * scale));

        ImGui::BeginChild("resident_scene_material_options", ImVec2(0.f, 220.f * scale), false, ImGuiWindowFlags_NoScrollbar);
        for (size_t i = 0; i < options.size(); ++i) {
            const bool selected = options[i] == current_label;
            const ImVec2 option_size(ImGui::GetContentRegionAvail().x, 30.f * scale);
            ImGui::InvisibleButton((std::string("##scene_material_option_") + std::to_string(i)).c_str(), option_size);
            const bool hovered = ImGui::IsItemHovered();
            const bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
            const ImVec2 min = ImGui::GetItemRectMin();
            const ImVec2 max = ImGui::GetItemRectMax();
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(min, max, selected ? IM_COL32(255, 191, 71, 28) : IM_COL32(255, 255, 255, hovered ? 16 : 8), 10.f * scale);
            draw_list->AddRect(min, max, selected ? IM_COL32(255, 191, 71, 180) : IM_COL32(255, 255, 255, hovered ? 34 : 16), 10.f * scale, 0, selected ? 1.8f : 1.f);
            draw_list->AddText(ImVec2(min.x + 10.f * scale, min.y + 7.f * scale),
                               selected ? IM_COL32(247, 238, 214, 255) : IM_COL32(228, 234, 239, 255),
                               options[i].c_str());
            if (selected)
                draw_list->AddText(ImVec2(max.x - 14.f * scale, min.y + 7.f * scale), IM_COL32(255, 191, 71, 255), "v");

            if (clicked && item_index >= 0 && item_index < static_cast<int>(m_items.size())) {
                on_update_filament_type(item_index, options[i]);
                remap_item_with_match_color(item_index);
                on_scene_reloaded();
                refresh_items_from_config();
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndChild();
        ImGui::EndPopup();
    } else {
        ui_state.pending_scene_material_popup_index = -1;
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);

    if (ui_state.open_delete_confirm_requested) {
        ImGui::OpenPopup("resident_delete_confirm");
        ui_state.open_delete_confirm_requested = false;
    }

    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(360.f * scale, 0.f), ImGuiCond_Appearing);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16.f * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18.f * scale, 16.f * scale));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.10f, 0.12f, 0.15f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.f, 1.f, 1.f, 0.10f));
    if (ImGui::BeginPopupModal("resident_delete_confirm", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(u8"\u5220\u9664\u8be5\u989c\u8272\uff1f");
        ImGui::Dummy(ImVec2(0.f, 8.f * scale));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.84f, 0.88f, 0.92f, 1.f));
        ImGui::TextWrapped("%s", u8"\u5220\u9664\u540e\uff0c\u8fd9\u4e2a\u573a\u666f\u989c\u8272\u5c06\u4ece\u5f53\u524d\u6253\u5370\u4efb\u52a1\u4e2d\u79fb\u9664\uff0c\u6620\u5c04\u5173\u7cfb\u548c\u9884\u89c8\u4f1a\u540c\u6b65\u66f4\u65b0\u3002");
        ImGui::Dummy(ImVec2(0.f, 6.f * scale));
        ImGui::TextWrapped("%s", u8"\u6b64\u64cd\u4f5c\u4e0d\u4f1a\u5220\u9664\u8bbe\u5907\u4e2d\u7684\u8017\u6750\uff0c\u53ea\u4f1a\u5220\u9664\u5f53\u524d\u573a\u666f\u989c\u8272\u3002");
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0.f, 14.f * scale));

        const float button_w = 112.f * scale;
        const float button_h = 34.f * scale;
        const float gap = 10.f * scale;
        const float total_w = button_w * 2.f + gap;
        const float start_x = ImGui::GetCursorPosX() + std::max(0.f, (ImGui::GetContentRegionAvail().x - total_w) * 0.5f);
        ImGui::SetCursorPosX(start_x);
        if (ImGui::Button(u8"\u53d6\u6d88", ImVec2(button_w, button_h))) {
            ui_state.pending_delete_row_index = -1;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine(0.f, gap);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.52f, 0.18f, 0.18f, 0.96f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.66f, 0.22f, 0.22f, 0.98f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.44f, 0.14f, 0.14f, 1.f));
        if (ImGui::Button(u8"\u5220\u9664", ImVec2(button_w, button_h))) {
            delete_scene_color_at(*this, ui_state.pending_delete_row_index);
            ui_state.pending_delete_row_index = -1;
        }
        ImGui::PopStyleColor(3);

        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            ui_state.pending_delete_row_index = -1;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

void ImGuiFilamentPanel::render_thumbnail_preview(float preview_w, float child_h, float scale)
{
    (void)preview_w;
    (void)child_h;

    PartPlateList& plate_list = wxGetApp().plater()->get_partplate_list();
    PartPlate* plate = plate_list.get_curr_plate();
    const int plate_idx = plate_list.get_curr_plate_index();
    bool rendered_lit = false;
    bool rendered_no_light = false;
    if (plate != nullptr) {
        ThumbnailsParams thumbnail_params = {{}, false, true, true, true, plate_idx};
        const PreviewThumbnailRenderParams preview_render_params = build_preview_thumbnail_render_params();
        const bool use_top_view = preview_render_params.use_top_view;
        const std::string& view_type = preview_render_params.view_type;

        if (!plate->thumbnail_data.is_valid()) {
            wxGetApp().plater()->get_view3D_canvas3D()->render_thumbnail(
                plate->thumbnail_data,
                (unsigned int)plate->plate_thumbnail_width,
                (unsigned int)plate->plate_thumbnail_height,
                thumbnail_params,
                Camera::EType::Ortho,
                use_top_view,
                false,
                false,
                view_type);
            rendered_lit = true;
        }

        if (!plate->no_light_thumbnail_data.is_valid()) {
            wxGetApp().plater()->get_view3D_canvas3D()->render_thumbnail(
                plate->no_light_thumbnail_data,
                (unsigned int)plate->plate_thumbnail_width,
                (unsigned int)plate->plate_thumbnail_height,
                thumbnail_params,
                Camera::EType::Ortho,
                use_top_view,
                false,
                true,
                view_type);
            rendered_no_light = true;
        }
    }

    // Cache base thumbnails (lit + no-light) for in-place recolor without re-rendering.
    if (plate != m_thumb_base_plate) {
        m_thumb_base_plate = plate;
        m_thumb_base_lit.reset();
        m_thumb_base_no_light.reset();
        m_thumb_base_rgb.clear();
        m_thumb_recolor_dirty = true;
    }

    auto to_u8 = [](float x) -> unsigned {
        float v = x;
        if (v < 0.0f) v = 0.0f;
        if (v > 1.0f) v = 1.0f;
        return (unsigned)std::lround(v * 255.0f);
    };
    auto to_rgb_u32 = [&](const ImVec4& c) -> unsigned {
        const unsigned r = to_u8(c.x);
        const unsigned g = to_u8(c.y);
        const unsigned b = to_u8(c.z);
        return (r << 16) | (g << 8) | b;
    };

    std::vector<unsigned> curr_rgb;
    curr_rgb.reserve(m_items.size());
    for (const auto& it : m_items)
        curr_rgb.push_back(to_rgb_u32(it.color));

    if (m_thumb_base_rgb.size() != curr_rgb.size() && plate == m_thumb_base_plate) {
        m_thumb_base_lit.reset();
        m_thumb_base_no_light.reset();
        m_thumb_base_rgb.clear();
        m_thumb_recolor_dirty = true;
    }

    if (plate != nullptr && plate->thumbnail_data.is_valid() && plate->no_light_thumbnail_data.is_valid()) {
        if (rendered_lit || rendered_no_light || !m_thumb_base_lit.is_valid() || !m_thumb_base_no_light.is_valid() ||
            m_thumb_base_rgb.empty()) {
            m_thumb_base_lit.set(plate->thumbnail_data.width, plate->thumbnail_data.height);
            m_thumb_base_lit.pixels = plate->thumbnail_data.pixels;
            m_thumb_base_no_light.set(plate->no_light_thumbnail_data.width, plate->no_light_thumbnail_data.height);
            m_thumb_base_no_light.pixels = plate->no_light_thumbnail_data.pixels;
            m_thumb_base_rgb = curr_rgb;
            m_thumb_recolor_dirty = false;
        }
    }

    if (m_thumbnail_preview != nullptr) {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        avail.x = std::max(1.f, avail.x);
        avail.y = std::max(1.f, avail.y);

        static thread_local std::vector<RGB8> match_colors;
        match_colors.clear();
        match_colors.reserve(m_items.size());
        for (const auto& it : m_items) {
            RGB8 c;
            if (m_hover_preview_override.active && m_hover_preview_override.item_index == it.index) {
                c.r = (std::uint8_t)to_u8(m_hover_preview_override.match_color.x);
                c.g = (std::uint8_t)to_u8(m_hover_preview_override.match_color.y);
                c.b = (std::uint8_t)to_u8(m_hover_preview_override.match_color.z);
            } else if(is_item_matched(it)) {
                c.r = (std::uint8_t)to_u8(it.match_color.x);
                c.g = (std::uint8_t)to_u8(it.match_color.y);
                c.b = (std::uint8_t)to_u8(it.match_color.z);
            }
            else
            {
                c.r = (std::uint8_t)to_u8(it.color.x);
                c.g = (std::uint8_t)to_u8(it.color.y);
                c.b = (std::uint8_t)to_u8(it.color.z);
            }

            match_colors.push_back(c);
        }

        const ThumbnailData* lit_td = (m_thumb_base_lit.is_valid() ? &m_thumb_base_lit :
                                       (plate != nullptr ? &plate->thumbnail_data : nullptr));
        const ThumbnailData* nl_td  = (m_thumb_base_no_light.is_valid() ? &m_thumb_base_no_light :
                                       (plate != nullptr ? &plate->no_light_thumbnail_data : nullptr));
        m_thumbnail_preview->draw_recolored(lit_td, nl_td, match_colors, ThumbnailRecolorParams{}, avail, scale);
    }
}

void ImGuiFilamentPanel::render_add_button_and_palette()
{
    auto& ui_state = transient_ui_state();
    m_selected_material_type = "Hyper PLA";
    if (!ui_state.open_add_palette_requested)
        return;

    ui_state.open_add_palette_requested = false;

    const wxColour initial_color = Slic3r::GUI::Plater::get_next_color_for_filament();
    std::string picked_hex;
    if (choose_filament_color_with_dialog(initial_color, _L("Please choose the filament colour"), picked_hex))
        on_pick_and_add_filament_colour(picked_hex);
}

void ImGuiFilamentPanel::on_pick_and_add_filament_colour(const std::string& filament_colour)
{
    int         filament_count = m_items.size() + 1;
    //wxColour    new_col        = Plater::get_next_color_for_filament();
    //std::string new_color      = new_col.GetAsString(wxC2S_HTML_SYNTAX).ToStdString();
    wxGetApp().preset_bundle->set_num_filaments(filament_count);
    wxGetApp().plater()->on_filaments_change(filament_count);
    wxGetApp().get_tab(Preset::TYPE_PRINT)->update();
    wxGetApp().preset_bundle->export_selections(*wxGetApp().app_config);
    GUI::wxGetApp().plater()->sidebar().auto_calc_flushing_volumes(filament_count - 1);

    m_items.emplace_back();

    int last_item_idx = filament_count-1;
    // 1) Color
    commit_color_change(last_item_idx, hex_to_imvec4(filament_colour));

    // 2) Sync label
    //std::string lbl;
    //if (isCfsMini && box_id == 5) 
    //    lbl = "CFS"; // keep compatibility with FilamentPanel
    //else {
    //    char index_char = (char)('A' + (material.material_id % 4));
    //    lbl = std::to_string(box_id) + index_char;
    //}

    m_items[last_item_idx].index = last_item_idx;
    m_items[last_item_idx].color = hex_to_imvec4(filament_colour);
    m_items[last_item_idx].sync_label = "Hyper PLA";
    m_items[last_item_idx].type_label = m_selected_material_type;
    m_items[last_item_idx].device_match_slot = "";
            
    // 3) update filament type
    on_update_filament_type(last_item_idx, m_selected_material_type);

    remap_item_with_match_color(last_item_idx);
    on_scene_reloaded();
    refresh_items_from_config();
}

void ImGuiFilamentPanel::remap_item_with_match_color(int idx)
{
    if (m_items.empty())
        return;
    const bool remap_all = (idx == -1);
    if (idx < -1)
        return;
    if (!remap_all && (idx < 0 || idx >= (int)m_items.size()))
        return;

    const DM::Device& device = DM::DataCenter::Ins().get_current_device_data();
    if (!device.valid)
        return;

    ColorMatch::Device match_device;
    for (const auto& box : device.materialBoxes) {
        if (!auto_mapping_box_matches_mode(m_mode, box.box_type))
            continue;
        for (const auto& m : box.materials) {
            if (m.color.empty())
                continue;
            ColorMatch::DeviceBoxColorInfo info;
            info.boxType = box.box_type;
            info.color = m.color;
            info.filamentType = !m.type.empty() ? m.type : m.name;
            info.materialId = m.material_id;
            info.boxId = box.box_id;
            match_device.boxsInfo.boxColorInfo.push_back(std::move(info));
        }
    }

    if (match_device.boxsInfo.boxColorInfo.empty())
        return;

    std::vector<ColorMatch::ModelColor> model_colors;
    if (remap_all) {
        model_colors.reserve(m_items.size());
        for (int i = 0; i < (int)m_items.size(); ++i) {
            ColorMatch::ModelColor model_color;
            model_color.extruderId     = i;
            model_color.extruderColor  = imvec4_to_hex(m_items[i].color);
            model_color.filamentType   = m_items[i].type_label;
            if (model_color.filamentType.empty())
                model_color.filamentType = m_items[i].preset_display;
            model_color.filamentLength = 0.0;
            model_colors.push_back(std::move(model_color));
        }
    } else {
        ColorMatch::ModelColor model_color;
        model_color.extruderId     = idx;
        model_color.extruderColor  = imvec4_to_hex(m_items[idx].color);
        model_color.filamentType   = m_items[idx].type_label;
        if (model_color.filamentType.empty())
            model_color.filamentType = m_items[idx].preset_display;
        model_color.filamentLength = 0.0;
        model_colors.push_back(std::move(model_color));
    }

    auto matches = ColorMatch::getColorMatchInfo(match_device, model_colors);
    if (matches.empty())
        return;

    std::vector<ColorMatch::MatchResult> matches_by_id(m_items.size());
    std::vector<bool> has_match(m_items.size(), false);
    for (const auto& match : matches) {
        if (match.extruderId < 0 || match.extruderId >= (int)m_items.size())
            continue;
        matches_by_id[match.extruderId] = match;
        has_match[match.extruderId] = true;
    }

    const int start = remap_all ? 0 : idx;
    const int end   = remap_all ? (int)m_items.size() : (idx + 1);
    for (int i = start; i < end; ++i) {
        if (!has_match[i])
            continue;

        const auto& match = matches_by_id[i];
        if (match.matchStatusCode != 0)
            continue;

        int matched_box_type = -1;
        std::string mapping_token = m_items[i].mapping_token;
        for (const auto& opt : m_material_options) {
            if (opt.box_id == match.boxId && opt.material.material_id == match.materialId) {
                matched_box_type = opt.box_type;
                mapping_token = std::to_string(opt.box_type) + ":" + std::to_string(opt.box_id) + ":" + std::to_string(opt.material.material_id);
                break;
            }
        }
        if (mapping_token.empty() && match.boxId > 0)
            mapping_token = "0:" + std::to_string(match.boxId) + ":" + std::to_string(match.materialId);
        std::string slot_label;
        if (matched_box_type >= 0) {
            slot_label = auto_mapping_slot_label_for_box(matched_box_type, match.boxId, match.materialId);
        } else if (match.boxId > 0) {
            slot_label = auto_mapping_slot_label_for_box(0, match.boxId, match.materialId);
        } else {
            slot_label = m_items[i].device_match_slot.empty() ? std::string("EXT") : m_items[i].device_match_slot;
        }

        m_items[i].match_color = hex_to_imvec4(match.matchColor);
        m_items[i].device_match_slot = slot_label;
        m_items[i].sync_label = slot_label;
        m_items[i].mapping_token = mapping_token;
    }
}


void ImGuiFilamentPanel::on_auto_mapping_filament(const DM::Device& deviceData)
{
    // Build list of valid materials from device based on current mode:
    // - Mode::CFS      => only box_type == 0
    // - Mode::External => only box_type == 1
    std::vector<std::pair<int, DM::Material>> validMaterials;
    m_material_options.clear();
    const bool external_mode = (m_mode == Mode::External);

    for (const auto& box : deviceData.materialBoxes) {
        if (!auto_mapping_box_matches_mode(m_mode, box.box_type))
            continue;

        for (size_t si = 0; si < box.materials.size(); ++si) {
            const auto& m = box.materials[si];
            if (!m.color.empty()) {
                validMaterials.emplace_back(box.box_id, m);
                MaterialOption opt; 
                opt.box_id = box.box_id; 
                opt.slot_index = (int)si; 
                opt.box_type = box.box_type; 
                opt.material = m;
                m_material_options.push_back(std::move(opt));
            }
        }
    }

    if (validMaterials.empty()) return;

    auto *bundle = GUI::wxGetApp().preset_bundle;
    if (!bundle) return;
    // Ensure preset bundle number of filaments matches device
    const size_t filament_count = validMaterials.size();
    const bool need_more = (bundle->filament_presets.size() < filament_count);
    if (bundle->filament_presets.size() != filament_count) {
        if (need_more) {
            wxColour    new_col   = Slic3r::GUI::Plater::get_next_color_for_filament();
            std::string new_color = new_col.GetAsString(wxC2S_HTML_SYNTAX).ToStdString();
            bundle->set_num_filaments(filament_count, new_color);
        } else {
            bundle->set_num_filaments(filament_count);
        }
        
        wxGetApp().plater()->sidebar().clear_all_filament();
        wxGetApp().plater()->on_filaments_change(filament_count);
        GUI::wxGetApp().get_tab(Slic3r::Preset::TYPE_PRINT)->update();
        GUI::wxGetApp().preset_bundle->export_selections(*GUI::wxGetApp().app_config);
        if (need_more)
            GUI::wxGetApp().plater()->sidebar().auto_calc_flushing_volumes((int)filament_count - 1);

    }

    m_items.assign(filament_count, {});

    for (size_t i = 0; i < validMaterials.size(); ++i) {
        const int box_id = validMaterials[i].first;
        const DM::Material& material = validMaterials[i].second;
        // 1) Color
        commit_color_change((int)i, hex_to_imvec4(material.color));

        // 2) Sync label
        std::string lbl;
        if (external_mode)
            lbl = "EXT";
        else {
            char index_char = (char)('A' + (material.material_id % 4));
            lbl = std::to_string(box_id) + index_char;
        }
        if (i < m_items.size()) {
            m_items[i].index = i;
            m_items[i].color = hex_to_imvec4(material.color);
            m_items[i].match_color = hex_to_imvec4(material.color);
            m_items[i].sync_label = lbl;
            m_items[i].type_label = material.name;
            m_items[i].device_match_slot = lbl;
            m_items[i].mapping_token = std::to_string(m_material_options[i].box_type) + ":" + std::to_string(box_id) + ":" + std::to_string(material.material_id);
        } 
            
        // 3) update filament type
        on_update_filament_type((int) i, material.name);

    }

    wxGetApp().preset_bundle->update_filament_presets = false;
    wxGetApp().plater()->sidebar().update_filament_panel();
    wxGetApp().preset_bundle->update_filament_presets = true;

    // Apply mapping per material
    //refresh_items_from_config();

    // Mark as synced for the current device+mode to avoid duplicate auto-mapping in the same frame.
    m_last_sig = device_fingerprint(deviceData);

}


void ImGuiFilamentPanel::on_auto_mapping_filament_ex(const DM::Device& deviceData)
{
    // Build list of valid materials from device based on current mode:
    // - Mode::CFS      => only box_type == 0
    // - Mode::External => only box_type == 1
    std::vector<std::pair<int, DM::Material>> validMaterials;
    m_material_options.clear();
    const bool external_mode = (m_mode == Mode::External);

    for (const auto& box : deviceData.materialBoxes) {
        if (!auto_mapping_box_matches_mode(m_mode, box.box_type))
            continue;

        for (size_t si = 0; si < box.materials.size(); ++si) {
            const auto& m = box.materials[si];
            if (!m.color.empty()) {
                validMaterials.emplace_back(box.box_id, m);
                MaterialOption opt; 
                opt.box_id = box.box_id; 
                opt.slot_index = (int)si; 
                opt.box_type = box.box_type; 
                opt.material = m;
                m_material_options.push_back(std::move(opt));
            }
        }
    }

    if (validMaterials.empty()) return;

    auto *preset_bundle = GUI::wxGetApp().preset_bundle;
    if (!preset_bundle) return;
    // Ensure preset bundle number of filaments matches device
    const size_t filament_count = validMaterials.size();

    const size_t scene_filament_count = preset_bundle->filament_presets.size();

    auto extractBeforeAt = [](const std::string& str) -> std::string {
        size_t atPos = str.find('@');
        if (atPos != std::string::npos) {
            return str.substr(0, atPos);
        }
        return str;
    };


    m_items.assign(scene_filament_count, {});

    const size_t preset_count = preset_bundle->filament_presets.size();
    std::string filament_type;
    // Defensive: determine a safe preset name before lookup.
    std::string filament_preset_name;

    for(size_t i = 0; i < scene_filament_count; i++) {
    
        if (i < preset_count) {
            filament_preset_name = preset_bundle->filament_presets[i];
        } else {
            filament_preset_name = preset_bundle->filaments.get_selected_preset_name();
        }

        Slic3r::Preset* preset = preset_bundle->filaments.find_preset(filament_preset_name);
        if (!preset) {
            preset = preset_bundle->filaments.find_preset("Default Filament");
        }
        if (preset) {
            //preset->get_filament_type(filament_type);
            filament_type = extractBeforeAt(filament_preset_name);
            if (!filament_type.empty()) {
                size_t endPos = filament_type.find_last_not_of(" \t\n\r");
                if (endPos != std::string::npos) {
                    filament_type = filament_type.substr(0, endPos + 1);
                }
            }
        } else {
            filament_type = "Filament"; // final fallback label
        }

        std::string filament_color = preset_bundle->project_config.opt_string("filament_colour", (unsigned int)i);

        // need to sync bundle filament info into m_items first
        m_items[i].index = i;
        m_items[i].color = hex_to_imvec4(filament_color);
        m_items[i].type_label = filament_type;

        
    }

    if(external_mode) {
        remap_external_item(deviceData);
    }
    else
    {
        remap_item_with_match_color(-1);
        const auto popup_catalog = ResidentFilamentMappingAdapter::build_popup_option_catalog(deviceData);
        for (auto& item : m_items)
            try_apply_popup_fallback_mapping(item, popup_catalog);
    }

    // Mark as synced for the current device+mode to avoid duplicate auto-mapping in the same frame.
    m_last_sig = device_fingerprint(deviceData);
}


bool ImGuiFilamentPanel::is_current_device_valid()
{
    const DM::Device& current_device = DM::DataCenter::Ins().get_current_device_data();
    return ResidentFilamentMappingAdapter::device_has_available_materials(current_device, m_mode);
}

void ImGuiFilamentPanel::on_update_filament_type(int idx, const std::string& filament_type)
{
    bool flag = is_support_filament(idx);

    std::string preset_name = wxGetApp().preset_bundle->get_preset_name_by_alias(Preset::TYPE_FILAMENT, filament_type);

    // if can NOT find preset_name (for some filament config resons), just return
    if(preset_name == filament_type) {
        return;
    }

    wxGetApp().preset_bundle->set_filament_preset(idx, preset_name);
    wxGetApp().plater()->update_project_dirty_from_presets();
    wxGetApp().preset_bundle->export_selections(*wxGetApp().app_config);
    wxGetApp().plater()->sidebar().update_dynamic_filament_list();
    bool flag_is_change = is_support_filament(idx);
    if (flag != flag_is_change) {
        wxGetApp().plater()->sidebar().auto_calc_flushing_volumes(idx);
    }

    // update plater with new config
    wxGetApp().plater()->on_config_change(wxGetApp().preset_bundle->full_config());

    if (wxGetApp().app_config->get("auto_calculate_when_filament_change") == "true") {
        wxGetApp().plater()->sidebar().auto_calc_flushing_volumes(idx);
    }

    // BBS: log modify of filament selection
    Slic3r::put_other_changes();

    // update slice state and set bedtype default for 3rd-party printer
    PartPlateList& ppl = wxGetApp().plater()->get_partplate_list();
    for (auto plate : ppl.get_plate_list()) {
        plate->update_slice_result_valid_state(false);
    }
}

size_t ImGuiFilamentPanel::device_fingerprint(const DM::Device& d)
{
    std::string acc; 
    acc.reserve(256);
    acc.append("mode:").append(std::to_string(static_cast<int>(m_mode))).append(";");
    for (const auto& box : d.materialBoxes) {
        if (!auto_mapping_box_matches_mode(m_mode, box.box_type))
            continue;
        acc.append("b").append(std::to_string(box.box_type)).append(":").append(std::to_string(box.box_id)).append(";");
        for (const auto& m : box.materials) {
            if (m.color.empty()) 
                continue;

            acc.append("m").append(std::to_string(m.material_id)).append("@").append(m.color).append("@").append(m.name).append(";");
        }
    }
    return std::hash<std::string>{}(acc);
}

void ImGuiFilamentPanel::check_device_filament_auto_mapping()
{
    const DM::Device& dev = DM::DataCenter::Ins().get_current_device_data();

    if(is_current_device_valid()) {
        size_t sig = device_fingerprint(dev);
        if(sig != m_last_sig) {
            on_auto_mapping_filament_ex(dev);
            m_last_sig = sig;
        }
    }

}

namespace {

struct ResolvedMappingMaterial {
    bool        found = false;
    int         box_id = -1;
    int         box_type = -1;
    int         material_id = -1;
    std::string slot_label;
    std::string material_type;
    std::string material_name;
    std::string color_hex;
    std::string mapping_token;
};

static bool parse_mapping_token(const std::string& token, int& box_type, int& box_id, int& material_id)
{
    if (token.empty())
        return false;

    std::stringstream ss(token);
    std::string part;
    if (!std::getline(ss, part, ':'))
        return false;
    try {
        box_type = std::stoi(part);
    } catch (...) {
        return false;
    }
    if (!std::getline(ss, part, ':'))
        return false;
    try {
        box_id = std::stoi(part);
    } catch (...) {
        return false;
    }
    if (!std::getline(ss, part, ':'))
        return false;
    try {
        material_id = std::stoi(part);
    } catch (...) {
        return false;
    }
    return true;
}

static std::string build_mapping_token(int box_type, int box_id, int material_id)
{
    return std::to_string(box_type) + ":" + std::to_string(box_id) + ":" + std::to_string(material_id);
}

static std::string build_mapping_slot_label(int box_type, int box_id, int material_id)
{
    if (box_type == 1 || box_type == 2)
        return "EXT";

    const char letter = static_cast<char>('A' + (material_id % 26));
    return std::to_string(box_id) + letter;
}

static std::string build_mapping_material_display_label(const DM::Material& material)
{
    const bool no_color = material.color.empty();
    if (material.state == -1 || (material.state == 0 && no_color))
        return "/";
    if (no_color)
        return "?";
    return material.type.empty() ? material.name : material.type;
}

static ResolvedMappingMaterial build_resolved_mapping_material(int box_type, int box_id, const DM::Material& material)
{
    ResolvedMappingMaterial resolved;
    resolved.found = true;
    resolved.box_id = box_id;
    resolved.box_type = box_type;
    resolved.material_id = material.material_id;
    resolved.slot_label = build_mapping_slot_label(box_type, box_id, material.material_id);
    resolved.material_type = build_mapping_material_display_label(material);
    resolved.material_name = material.name;
    resolved.color_hex = material.color;
    resolved.mapping_token = build_mapping_token(box_type, box_id, material.material_id);
    return resolved;
}

static ResolvedMappingMaterial resolve_external_mapping_material(const DM::Device& device)
{
    const DM::Material* chosen = nullptr;
    int chosen_box_id = -1;
    for (const auto& box : device.materialBoxes) {
        if (box.box_type != 1)
            continue;

        for (const auto& material : box.materials) {
            if (material.color.empty())
                continue;
            if (material.selected) {
                chosen = &material;
                chosen_box_id = box.box_id;
                break;
            }
            if (chosen == nullptr) {
                chosen = &material;
                chosen_box_id = box.box_id;
            }
        }
        if (chosen != nullptr && chosen->selected)
            break;
    }

    if (chosen == nullptr)
        return {};
    return build_resolved_mapping_material(1, chosen_box_id, *chosen);
}

static ResolvedMappingMaterial resolve_mapping_material_for_item(const DM::Device& device, const ImGuiFilamentItemState& item)
{
    int token_box_type = -1;
    int token_box_id = -1;
    int token_material_id = -1;
    if (parse_mapping_token(item.mapping_token, token_box_type, token_box_id, token_material_id)) {
        for (const auto& box : device.materialBoxes) {
            if (box.box_type != token_box_type || box.box_id != token_box_id)
                continue;
            for (const auto& material : box.materials) {
                if (material.material_id == token_material_id)
                    return build_resolved_mapping_material(box.box_type, box.box_id, material);
            }
        }
    }

    if (item.device_match_slot == "EXT")
        return resolve_external_mapping_material(device);

    for (const auto& box : device.materialBoxes) {
        for (const auto& material : box.materials) {
            if (build_mapping_slot_label(box.box_type, box.box_id, material.material_id) == item.device_match_slot)
                return build_resolved_mapping_material(box.box_type, box.box_id, material);
        }
    }

    return {};
}

} // namespace

bool ImGuiFilamentPanel::apply_mapping_selection(int item_index, const std::string& selection_token)
{
    if (item_index < 0 || item_index >= static_cast<int>(m_items.size()) || selection_token.empty())
        return false;

    const DM::Device& current_device = DM::DataCenter::Ins().get_current_device_data();
    const auto popup_catalog = ResidentFilamentMappingAdapter::build_popup_option_catalog(current_device);
    if (!ResidentFilamentMappingAdapter::apply_popup_selection(popup_catalog, selection_token, m_items[item_index]))
        return false;

    m_items[item_index].mapping_token = selection_token;
    m_thumb_recolor_dirty = true;
    return true;
}

bool ImGuiFilamentPanel::apply_mapping_colors_to_scene()
{
    auto* bundle = GUI::wxGetApp().preset_bundle;
    if (!bundle)
        return false;

    DynamicPrintConfig* cfg = &bundle->project_config;
    auto* colors = static_cast<ConfigOptionStrings*>(cfg->option("filament_colour")->clone());
    if (!colors)
        return false;

    const DM::Device& current_device = DM::DataCenter::Ins().get_current_device_data();
    bool changed = false;
    for (const auto& item : m_items) {
        if (item.index < 0 || item.index >= static_cast<int>(colors->values.size()))
            continue;

        const ResolvedMappingMaterial resolved = resolve_mapping_material_for_item(current_device, item);
        if (!resolved.found || resolved.color_hex.empty())
            continue;

        if (colors->values[item.index] == resolved.color_hex)
            continue;

        colors->values[item.index] = resolved.color_hex;
        changed = true;
    }

    if (!changed) {
        refresh_items_from_config();
        m_thumb_recolor_dirty = true;
        return true;
    }

    DynamicPrintConfig cfg_new = *cfg;
    cfg_new.set_key_value("filament_colour", colors);

    cfg->apply(cfg_new);
    GUI::wxGetApp().plater()->update_project_dirty_from_presets();
    GUI::wxGetApp().preset_bundle->export_selections(*GUI::wxGetApp().app_config);
    GUI::wxGetApp().plater()->on_config_change(cfg_new);

    refresh_items_from_config();
    m_thumb_recolor_dirty = true;
    return true;
}

nlohmann::json ImGuiFilamentPanel::export_ai_mapping_items() const
{
    nlohmann::json arr = nlohmann::json::array();
    const DM::Device& current_device = DM::DataCenter::Ins().get_current_device_data();
    const auto popup_catalog = ResidentFilamentMappingAdapter::build_popup_option_catalog(current_device);

    for (const auto& item : m_items) {
        const ResolvedMappingMaterial resolved = resolve_mapping_material_for_item(current_device, item);
        const auto* popup_payload =
            item.mapping_token.empty()
                ? nullptr
                : ResidentFilamentMappingAdapter::find_popup_selection_payload(popup_catalog, item.mapping_token);

        std::string exported_slot_label = resolved.found ? resolved.slot_label : item.device_match_slot;
        std::string exported_match_color = resolved.found ? resolved.color_hex : imvec4_to_hex(item.match_color);
        if (popup_payload != nullptr) {
            if (exported_slot_label.empty())
                exported_slot_label = popup_payload->slot_label;
            if (!resolved.found)
                exported_match_color = imvec4_to_hex(popup_payload->material_color);
        }

        nlohmann::json info;
        info["item_index"] = item.index;
        info["extruderId"] = item.index + 1;
        info["extruderFilamentType"] = item.type_label;
        info["presetDisplay"] = item.preset_display;
        info["sourceColor"] = imvec4_to_hex(item.color);
        info["mapped"] = resolved.found;
        info["slotLabel"] = exported_slot_label;
        info["matchColor"] = exported_match_color;
        info["boxId"] = resolved.found ? resolved.box_id : -1;
        info["materialId"] = resolved.found ? resolved.material_id : -1;
        info["materialType"] = resolved.found ? resolved.material_type : std::string();
        info["materialName"] = resolved.found ? resolved.material_name : std::string();
        info["selection_token"] = resolved.found ? resolved.mapping_token : item.mapping_token;
        arr.push_back(std::move(info));
    }

    return arr;
}

nlohmann::json ImGuiFilamentPanel::export_color_match_info() const
{
    nlohmann::json arr = nlohmann::json::array();
    const DM::Device& current_device = DM::DataCenter::Ins().get_current_device_data();

    const auto resolve_cid = [&current_device](int box_id, int material_id) -> std::string {
        for (const auto& box_color_info : current_device.boxColorInfos) {
            if (box_color_info.boxId == box_id &&
                box_color_info.materialId == material_id &&
                !box_color_info.cId.empty()) {
                return box_color_info.cId;
            }
        }
        return {};
    };

    for (const auto& item : m_items) {
        const ResolvedMappingMaterial resolved = resolve_mapping_material_for_item(current_device, item);
        if (!resolved.found)
            continue;

        const std::string filament_type = item.type_label.empty() ? item.preset_display : item.type_label;

        nlohmann::json info;
        info["boxId"] = resolved.box_id;
        info["boxType"] = resolved.box_type;
        info["extruderId"] = item.index + 1;
        info["extruderFilamentType"] = filament_type;
        info["extruderColor"] = imvec4_to_hex(item.color);
        info["filamentType"] = filament_type;
        info["matchColor"] = resolved.color_hex;
        info["materialId"] = resolved.material_id;
        info["materialName"] = resolved.material_name;
        info["slotLabel"] = resolved.slot_label;
        info["selection_token"] = resolved.mapping_token;
        info["cId"] = resolve_cid(resolved.box_id, resolved.material_id);
        arr.push_back(std::move(info));
    }
    return arr;
}

void ImGuiFilamentPanel::remove_last_filament()
{
    if (m_items.empty())
        return;

    auto *bundle = GUI::wxGetApp().preset_bundle;
    if (!bundle)
        return;

    const int remove_idx = static_cast<int>(m_items.size()) - 1;
    const int filament_count = static_cast<int>(m_items.size()) - 1;
    if (filament_count < 0)
        return;

    bundle->set_num_filaments(filament_count);
    GUI::wxGetApp().plater()->on_filaments_change(filament_count);
    GUI::wxGetApp().get_tab(Preset::TYPE_PRINT)->update();
    GUI::wxGetApp().preset_bundle->export_selections(*GUI::wxGetApp().app_config);
    if (filament_count > 0)
        GUI::wxGetApp().plater()->sidebar().auto_calc_flushing_volumes(filament_count - 1);

    m_items.pop_back();
    if (on_delete_filament)
        on_delete_filament(remove_idx);

    refresh_items_from_config();
}

void ImGuiFilamentPanel::check_and_resolve_mode_by_current_device()
{
    const DM::Device& current_device= DM::DataCenter::Ins().get_current_device_data();
    if (!current_device.valid) {
        return;
    }

    // Ensure current mode is supported by the device before validating.
    const Mode resolved_mode = resolve_mode_for_device(current_device, m_mode);
    if (resolved_mode != m_mode)
        m_mode = resolved_mode;
}

void ImGuiFilamentPanel::on_scene_reloaded()
{
    invalidate_preview_thumbnail_cache(true);
}

void ImGuiFilamentPanel::remap_external_item(const DM::Device& deviceData)
{
    // External rack: use the rack material color as match color for all extruders.
    // Prefer selected material; otherwise use the first colored material from box_type==1.
    const DM::Material* chosen = nullptr;
    int chosen_box_id = -1;
    for (const auto& box : deviceData.materialBoxes) {
        if (box.box_type != 1)
            continue;
        for (const auto& m : box.materials) {
            if (m.color.empty())
                continue;
            if (m.selected) {
                chosen = &m;
                chosen_box_id = box.box_id;
                break;
            }
            if (!chosen) {
                chosen = &m;
                chosen_box_id = box.box_id;
            }
        }
        if (chosen && chosen->selected)
            break;
    }

    const ImVec4 ext_color = (chosen != nullptr) ? hex_to_imvec4(chosen->color) : ImVec4(1, 1, 1, 1);
    const std::string ext_mapping_token = (chosen != nullptr)
        ? ("1:" + std::to_string(chosen_box_id) + ":" + std::to_string(chosen->material_id))
        : std::string();
    for (auto& it : m_items) {
        it.match_color       = ext_color;
        it.device_match_slot = "EXT";
        it.sync_label        = "EXT";
        it.mapping_token     = ext_mapping_token;
    }
}


} // namespace GUI
} // namespace Slic3r


