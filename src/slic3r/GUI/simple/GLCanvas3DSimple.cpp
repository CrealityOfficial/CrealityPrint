#include "libslic3r/libslic3r.h"
#include "SupportSimple.hpp"
#include "DeviceListSimple.hpp"
#include "MCPChatPanel.hpp"
#include "GLCanvas3D.hpp"

#include <igl/unproject.h>

#include "libslic3r/BuildVolume.hpp"
#include "libslic3r/ClipperUtils.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/GCode/ThumbnailData.hpp"
#include "libslic3r/Geometry/ConvexHull.hpp"
#include "libslic3r/ExtrusionEntity.hpp"
#include "libslic3r/Layer.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/Technologies.hpp"
#include "libslic3r/Tesselate.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/ModelObject.hpp"
#include "libslic3r/ModelInstance.hpp"
#include "libslic3r/ModelVolume.hpp"
#include "3DBed.hpp"
#include "3DScene.hpp"
#include "BackgroundSlicingProcess.hpp"
#include "GLShader.hpp"
#include "GLTexture.hpp"
#include "ImGuiWrapper.hpp"
#include "GUI.hpp"
#include "UITour.hpp"
#include "Tab.hpp"
#include "GUI_Preview.hpp"
#include "OpenGLManager.hpp"
#include "Plater.hpp"
#include "SiderBar.h"
#include "MainFrame.hpp"
#include "GUI_App.hpp"
#include "GUI_ObjectList.hpp"
#include "GUI_Colors.hpp"
#include "Mouse3DController.hpp"
#include "I18N.hpp"
#include "NotificationManager.hpp"
#include "format.hpp"
#include "DailyTips.hpp"
#include "Widgets/NumberEntryDialog.hpp"

#include "slic3r/GUI/Gizmos/GLGizmoMeshBoolean.hpp"
#include "slic3r/GUI/Gizmos/GLGizmoPainterBase.hpp"
#include "slic3r/GUI/Gizmos/GLGizmoEmboss.hpp"
#include "slic3r/GUI/Gizmos/GLGizmoFdmSupports.hpp"
#include "slic3r/Utils/UndoRedo.hpp"
#include "slic3r/Utils/MacDarkMode.hpp"
#include "slic3r/Config/DispConfig.h"
#include <slic3r/GUI/GUI_Utils.hpp>
#include "slic3r/GUI/Widgets/SideButton.hpp"
#include "slic3r/GUI/print_manage/Utils.hpp"

#if ENABLE_RETINA_GL
#include "slic3r/Utils/RetinaHelper.hpp"
#endif

#include <GL/glew.h>

#include <wx/glcanvas.h>
#include <wx/bitmap.h>
#include <wx/dcmemory.h>
#include <wx/image.h>
#include <wx/settings.h>
#include <wx/tooltip.h>
#include <wx/debug.h>
#include <wx/fontutil.h>
// Print now includes tbb, and tbb includes Windows. This breaks compilation of wxWidgets if included before wx.
#include "libslic3r/Print.hpp"
#include "libslic3r/SLAPrint.hpp"

#include "wxExtensions.hpp"

#include <tbb/parallel_for.h>
#include <tbb/spin_mutex.h>

#include <boost/log/trivial.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <iostream>
#include <float.h>
#include <algorithm>
#include <cmath>

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui/imgui_internal.h>

#include <imguizmo/ImGuizmo.h>
#include "libslic3r/common_header/common_header.h"

#include "GLSimpleUtils.hpp"
#include "print_manage/data/DataCenter.hpp"
#include "filamentMapping/ImGuiFilamentPanel.hpp"
#include "PrintSettingsPanel.hpp"
#include <cctype>
#include <cstring>

#include "sendWorkflow/EasyPrintSender.hpp"

#define TOP_GAP_DISTANCE 16.0f
#define PREVIEW_BTN_WIDTH 68.0f
#define SEND_PRINT_BTN_WIDTH 112.0f
namespace Slic3r
{
namespace GUI
{
namespace {

void refresh_ai_send_mapping_for_current_device_simple(bool auto_match)
{
    if (!wxGetApp().easy_mode())
        return;

    if (MCPChatWindow* chat_window = MCPChatWindow::Get()) {
        if (MCPChatPanel* chat_panel = chat_window->GetChatPanel())
            chat_panel->RefreshAISendMappingForCurrentDevice(auto_match);
    }
}

} // namespace

struct ImGuiDisableScope {
    bool active;
    explicit ImGuiDisableScope(bool disabled)
        : active(disabled)
    {
        if (active) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
    }
    ~ImGuiDisableScope() {
        if (active) {
            ImGui::PopStyleVar();
            ImGui::PopItemFlag();
        }
    }
};


// ??????? NDC ?????绻??????ê??
static inline float ndc_center_to_px(float left_ndc, float right_ndc) 
{
    const float canvas_w = float(wxGetApp().plater()->get_current_canvas3D()->get_canvas_size().get_width());
    const float center_ndc_x = 0.5f * (left_ndc + right_ndc);
    return 0.5f * canvas_w + center_ndc_x;
}
static constexpr const char* kEasyModeLayoutSection                         = "easy_mode";
static constexpr const char* kResidentFilamentPanelOverrideKey             = "resident_filament_panel_user_override";
static constexpr const char* kResidentFilamentPanelXKey                    = "resident_filament_panel_x";
static constexpr const char* kResidentFilamentPanelYKey                    = "resident_filament_panel_y";
static constexpr const char* kResidentFilamentPanelWidthKey                = "resident_filament_panel_w";
static constexpr const char* kResidentFilamentPanelHeightKey               = "resident_filament_panel_h";
static constexpr const char* kProModeLayoutSection                          = "pro_mode";
static constexpr const char* kAIChatFloatPosOverrideKey                    = "ai_chat_float_pos_user_override";
static constexpr const char* kAIChatFloatPosXKey                           = "ai_chat_float_pos_x";
static constexpr const char* kAIChatFloatPosYKey                           = "ai_chat_float_pos_y";
static bool parse_layout_float(const std::string& value, float& out)
{
    if (value.empty())
        return false;
    try {
        out = std::stof(value);
        return true;
    } catch (...) {
        return false;
    }
}

static constexpr float kEasyModeLeftWorkspaceSectionGap = 16.0f;
static constexpr float kEasyModeLeftWorkspaceTopInset = 14.0f;
static constexpr float kEasyModeLeftDeviceCardGap = 12.0f;
static constexpr float kEasyModeLeftBedTypeHeight = 34.0f;
static constexpr float kEasyModeLeftBedTypeGap = 12.0f;

static bool should_embed_easy_mode_left_workspace()
{
    return GetEmbeddedAIChatPanel() != nullptr;
}

static void open_easy_mode_send_workflow_from_scene()
{
    if (OpenActiveAISendWorkflowCardFromScene())
        return;

    BOOST_LOG_TRIVIAL(warning)
        << "[GLCanvas3DSimple] Failed to open AI send workflow from scene button, fallback to legacy send flow";
    if (wxGetApp().mainframe != nullptr)
        wxGetApp().mainframe->print_plate(MainFrame::eSendToLocalNetPrinter);
}
static ImVec4 make_rect_union(const ImVec4& a, const ImVec4& b)
{
    const float left = std::min(a.x, b.x);
    const float top = std::min(a.y, b.y);
    const float right = std::max(a.x + a.z, b.x + b.z);
    const float bottom = std::max(a.y + a.w, b.y + b.w);
    return ImVec4(left, top, right - left, bottom - top);
}

static void draw_easy_mode_left_workspace_shell(const ImVec4& outer_rect, const ImVec4& object_rect, float scale)
{
    if (outer_rect.z <= 1.0f || outer_rect.w <= 1.0f)
        return;

    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    const ImVec2 min(outer_rect.x, outer_rect.y);
    const ImVec2 max(outer_rect.x + outer_rect.z, outer_rect.y + outer_rect.w);
    const float rounding = 22.0f * scale;
    const ImVec4 shadow_rgb(118.0f / 255.0f, 142.0f / 255.0f, 171.0f / 255.0f, 1.0f);
    const float shadow_alpha[8] = {0.050f, 0.040f, 0.032f, 0.026f, 0.020f, 0.014f, 0.010f, 0.006f};
    const float shadow_steps[8] = {2.00f, 1.60f, 1.20f, 0.90f, 0.70f, 0.50f, 0.35f, 0.20f};
    for (int i = 0; i < 8; ++i) {
        const float spread = shadow_steps[i] * scale;
        const float round = rounding + spread;
        const float off_x = spread * 0.10f;
        const float off_y = spread * 0.80f;
        const ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(shadow_rgb.x, shadow_rgb.y, shadow_rgb.z, shadow_alpha[i]));
        draw_list->AddRectFilled(ImVec2(min.x - off_x, min.y + off_y), ImVec2(max.x + off_x, max.y + off_y), col, round);
    }

    draw_list->AddRectFilled(min, max, IM_COL32(20, 25, 31, 240), rounding);
    draw_list->AddRect(min, max, IM_COL32(255, 255, 255, 18), rounding, 0, 1.0f);

    if (object_rect.w > 1.0f) {
        const float divider_y = std::max(min.y + 24.0f * scale, object_rect.y - 0.5f * kEasyModeLeftWorkspaceSectionGap * scale);
        draw_list->AddLine(ImVec2(min.x + 14.0f * scale, divider_y),
                           ImVec2(max.x - 14.0f * scale, divider_y),
                           IM_COL32(255, 255, 255, 16),
                           1.0f);
    }
}

static ImVec4 make_easy_mode_left_bed_type_rect(const ImVec4& device_card_rect, float scale)
{
    return ImVec4(device_card_rect.x,
                  device_card_rect.y + device_card_rect.w + kEasyModeLeftDeviceCardGap * scale,
                  device_card_rect.z,
                  kEasyModeLeftBedTypeHeight * scale);
}

static void render_easy_mode_left_bed_type_selector(const ImVec4& rect, float scale)
{
    auto* plater = wxGetApp().plater();
    if (plater == nullptr || rect.z <= 1.0f || rect.w <= 1.0f)
        return;

    SidebarPrinter& bar = plater->sidebar_printer();
    std::vector<std::string> bed_types = bar.texts_of_bed_type_list();
    if (bed_types.empty())
        return;

    const bool dark = wxGetApp().dark_mode();
    const bool enabled = bar.get_bed_type_enable_status();
    const int selected_idx = std::max(0, std::min(bar.get_selection_bed_type(), int(bed_types.size()) - 1));
    const float pad_x = 8.0f * scale;
    const float label_gap = 10.0f * scale;
    const float combo_w = std::max(120.0f * scale, rect.z - 108.0f * scale);
    const float combo_h = 28.0f * scale;
    const ImVec4 frame_bg = dark ? ImVec4(0.18f, 0.20f, 0.24f, 0.96f) : ImVec4(1.0f, 1.0f, 1.0f, 0.96f);
    const ImVec4 border_col = dark ? ImVec4(0.38f, 0.40f, 0.45f, 0.78f) : ImVec4(0.75f, 0.75f, 0.78f, 0.78f);
    const ImVec4 text_col = dark ? ImVec4(0.90f, 0.92f, 0.95f, 1.0f) : ImVec4(0.10f, 0.10f, 0.12f, 1.0f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                             ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings;

    ImGui::SetNextWindowPos(ImVec2(rect.x, rect.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(rect.z, rect.w), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    if (ImGui::Begin("##easy_mode_bed_type_selector", nullptr, flags)) {
        ImGui::SetCursorPos(ImVec2(pad_x, 0.5f * (rect.w - ImGui::GetTextLineHeight())));
        ImGui::PushStyleColor(ImGuiCol_Text, text_col);
        ImGui::TextUnformatted(_u8L("Bed type").c_str());
        ImGui::PopStyleColor();

        ImGui::SameLine(0.0f, label_gap);
        ImGui::SetCursorPosX(rect.z - pad_x - combo_w);
        ImGui::SetCursorPosY(0.5f * (rect.w - combo_h));
        ImGui::PushItemWidth(combo_w);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, frame_bg);
        ImGui::PushStyleColor(ImGuiCol_Border, border_col);
        ImGui::PushStyleColor(ImGuiCol_Header, ImGuiWrapper::COL_CREALITY);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGuiWrapper::COL_CREALITY);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImGuiWrapper::COL_CREALITY);
        if (!enabled) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(text_col.x, text_col.y, text_col.z, 0.45f));
        }

        const char* combo_preview = bed_types[selected_idx].c_str();
        if (ImGui::BBLBeginCombo("##easy_mode_bed_type_combo", combo_preview,
                                 ImGuiComboFlags_HeightLargest, combo_w, combo_h)) {
            for (int idx = 0; idx < int(bed_types.size()); ++idx) {
                const bool is_selected = (selected_idx == idx);
                if (ImGui::Selectable(bed_types[idx].c_str(), is_selected))
                    bar.select_bed_type(idx);
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (!enabled) {
            ImGui::PopStyleColor();
            ImGui::PopItemFlag();
        }
        ImGui::PopStyleColor(5);
        ImGui::PopItemWidth();
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void GLCanvas3D::check_filament_mapping_simple()
{
    // Keep the 3MF import hook, but route AI mapping through the no-UI workflow service.
    refresh_ai_send_mapping_for_current_device_simple(true);
}

bool GLCanvas3D::_init_ui_simple()
{
    if (!_init_main_toolbar_simple())
        return false;

    if(!_init_object_manipulate_toolbar_simple())
        return false;

    return true;
}

bool GLCanvas3D::_init_main_toolbar_simple()
{
    if (!m_easymode_main_toolbar.is_enabled())
        return true;

    m_cur_device_icon_path = Slic3r::resources_dir() + "/images/current_no_device_simple.svg";
    _ensure_preset_icon_tex(48,48);
    
    BackgroundTexture::Metadata background_data;
    background_data.filename = !m_is_dark ? "toolbar_background_dark.png" : "toolbar_background.png";
    background_data.left     = 16;
    background_data.top      = 16;
    background_data.right    = 16;
    background_data.bottom   = 16;

    background_data.icon_borders.push_back(!m_is_dark ? "toolbar_border_disable_dark.svg" : "toolbar_border_disable.svg");
    background_data.icon_borders.push_back("toolbar_border_hover.svg");
    background_data.icon_borders.push_back("toolbar_border_press.svg");
    background_data.icon_borders.push_back(!m_is_dark ? "toolbar_border_normal_dark.svg" : "toolbar_border_normal.svg");

    if (!m_easymode_main_toolbar.init(background_data)) {
        // unable to init the toolbar texture, disable it
        m_easymode_main_toolbar.set_enabled(false);
        return true;
    }
    // init arrow
    if (!m_easymode_main_toolbar.init_arrow("toolbar_arrow.svg"))
        BOOST_LOG_TRIVIAL(error) << "Main toolbar failed to load arrow texture.";

    // m_gizmos is created at constructor, thus we can init arrow here.
    if (!m_gizmos.init_arrow("toolbar_arrow.svg"))
        BOOST_LOG_TRIVIAL(error) << "Gizmos manager failed to load arrow texture.";


    m_easymode_main_toolbar.set_horizontal_expand(true);
    m_easymode_main_toolbar.set_layout_type(GLToolbar::Layout::Horizontal);
    // BBS: main toolbar is at the top and left, we don't need the rounded-corner effect at the right side and the top side
    m_easymode_main_toolbar.set_horizontal_orientation(GLToolbar::Layout::HO_Center);
    m_easymode_main_toolbar.set_vertical_orientation(GLToolbar::Layout::VO_Top);
    // m_easymode_main_toolbar.set_border(10.0f);
    m_easymode_main_toolbar.set_border(8.0f);
    m_easymode_main_toolbar.set_separator_size(1.0f);
    m_easymode_main_toolbar.set_gap_size(15.0f);
    // m_easymode_main_toolbar.set_icons_size(40.0f);
    m_easymode_main_toolbar.del_all_item();
    // m_easymode_main_toolbar.set_icons_size(80.0f);
    m_easymode_main_toolbar.set_icons_size(72.0f);
    m_easymode_main_toolbar.set_layout_gap_scale(0.35f);
    m_easymode_main_toolbar.set_layout_step_scale(0.35f);

    // ?????????? margin ???????磨????????????????????
    const float ui_scale      = get_scale();
    const float popup_gap_px  = 8.0f * ui_scale;      // ????????????????????
    const float popup_h_px    = 320.0f * ui_scale;    // ???/??????

    // ?????????? item.left.render_callback?????????????
    auto make_popup_renderer =
        [this, popup_gap_px, popup_h_px]
        (bool& open_flag, PopupHAlign halign,
         const char* window_id,
         std::function<void()> content_fn)
    {
        return [this, &open_flag, halign, popup_gap_px, popup_h_px, content_fn, window_id]
               (float left_ndc, float right_ndc, float, float)
        {
            render_card_popup_if_open(
                open_flag,
                left_ndc, right_ndc,
                popup_h_px,
                halign,
                popup_gap_px,
                window_id,
                content_fn
            );
        };
    };

    GLToolbarItem::Data item;

    item.name          = "add";
    item.always_enable = true;
    item.label_below_icon = true;
    item.icon_label_split = 0.62f;
    item.icon_filename = m_is_dark ? "main_toolbar_import_models_simple.svg" : "main_toolbar_import_models_simple.svg";
    item.tooltip       = _utf8(L("Import Models"));
    item.sprite_id++;
    item.left.action_callback = [this]() {
        if (m_canvas != nullptr)
            wxPostEvent(m_canvas, SimpleEvent(EVT_GLTOOLBAR_ADD));
    };
    item.enabling_callback = []() -> bool { return wxGetApp().plater()->can_add_model(); };
    if (!m_easymode_main_toolbar.add_item(item,GLToolbarItem::EType::ActionWithText))
        return false;

    item.name          = "addplate";
    item.always_enable = true;
    item.label_below_icon = true;
    item.icon_label_split = 0.62f;
    item.icon_filename = m_is_dark ? "main_toolbar_add_plate_simple.svg" : "main_toolbar_add_plate_simple.svg";
    item.tooltip       = _utf8(L("Add plate"));
    item.sprite_id++;
    item.left.action_callback = [this]() {
        if (m_canvas != nullptr)
            wxPostEvent(m_canvas, SimpleEvent(EVT_GLTOOLBAR_ADD_PLATE));
    };
    item.enabling_callback = []() -> bool { return wxGetApp().plater()->can_add_plate(); };
    if (!m_easymode_main_toolbar.add_item(item, GLToolbarItem::EType::ActionWithText))
        return false;

    item.name          = "Support Settings";
    item.always_enable = true;
    item.label_below_icon = true;
    item.icon_label_split = 0.62f;
    item.icon_filename = m_is_dark ? "main_toolbar_support_simple.svg" : "main_toolbar_support_simple.svg";
    item.tooltip       = _utf8(L("Support"));
    item.sprite_id++;
    item.left.action_callback = [this]() {
        // Toggle panel
        m_supports_popup_open = !m_supports_popup_open;
    };
    item.enabling_callback    = []() -> bool { return true; };
    item.left.toggable        = true; // allow right mouse click

    item.left.render_callback =
        make_popup_renderer(
            m_supports_popup_open,
            PopupHAlign::GroupCenter,
            "##supports_popover",
            [this]() { _render_supports_popup(); }
        );

    if (!m_easymode_main_toolbar.add_item(item, GLToolbarItem::EType::ActionWithText))
        return false;

    //item.name          = "preset_settings";
    //item.always_enable = true;
    //item.label_below_icon = true;
    //item.icon_label_split = 0.62f;
    //item.icon_filename = m_is_dark ? "main_toolbar_setting_simple.svg" : "main_toolbar_setting_simple.svg";
    //item.tooltip       = _utf8(L("Settings"));
    //item.sprite_id++;
    //item.left.action_callback = [this]() {
    //    // Toggle panel
    //    m_preset_settings_open = !m_preset_settings_open;
    //    if(m_preset_settings_open) {
    //        m_print_settings_panel->on_popup();
    //    }
    //};

    //item.enabling_callback    = []() -> bool { return true; };

    //item.left.toggable = true;
    //item.left.render_callback =
    //    make_popup_renderer(
    //        m_preset_settings_open,
    //        PopupHAlign::GroupCenter,
    //        "##preset_settings",
    //        [this]() { _render_preset_settings(); }
    //    );
    //if (!m_easymode_main_toolbar.add_item(item, GLToolbarItem::EType::ActionWithText))
    //    return false;

    return true;
}

void GLCanvas3D::_render_main_toolbar_simple()
{
    if (!m_easymode_main_toolbar.is_enabled()) return;

    const float scale          = get_scale();
    m_easymode_main_toolbar.set_scale(scale);

    const Size  cnv_size = get_canvas_size();
    const float canvas_w = float(cnv_size.get_width());
    const float canvas_h = float(cnv_size.get_height());
    const float top_ndc  = 0.5f * canvas_h - TOP_GAP_DISTANCE * scale;
    const float safe_left_px = get_easy_mode_overlay_safe_left_px();
    const float safe_right_px = std::max(safe_left_px, canvas_w - get_easy_mode_overlay_safe_right_px());
    const float safe_width_px = std::max(0.0f, safe_right_px - safe_left_px);

    const float obj_list_right = get_main_toolbar_offset();

    const float pad_px         = 6.0f  * scale;
    const float gap_px         = 8.0f  * scale;
    const float preview_btn_w_px       = PREVIEW_BTN_WIDTH * scale;
    const float send_btn_w_px = SEND_PRINT_BTN_WIDTH * scale;
    // const float right_block_w  = pad_px + preview_btn_w_px + gap_px + send_btn_w_px + pad_px;
    const float right_block_w  = 0.0f;

    _render_cur_device_simple();
    const float prefix_w = m_top_prefix_width_px;

    const float preset_toolbar_gap_px = 10.0f * scale;

    const float full_space_w    = std::max(0.0f, safe_width_px - obj_list_right);
    const float toolbar_space_w = std::max(0.f, full_space_w - prefix_w - preset_toolbar_gap_px - right_block_w);
    const float toolbar_width   = m_easymode_main_toolbar.get_width();
    const float icons_space_px  = std::min(toolbar_space_w, toolbar_width);

    const float prefix_w_with_gap = prefix_w + preset_toolbar_gap_px;

    m_right_buttons_block_px    = right_block_w;
    m_top_toolbar_visible_px    = icons_space_px;
    m_top_group_width_px        = m_top_prefix_width_px + m_top_toolbar_visible_px + m_right_buttons_block_px + preset_toolbar_gap_px;

    const float group_w   = prefix_w + preset_toolbar_gap_px + icons_space_px + right_block_w;
    const float group_l   = safe_left_px + 0.5f * std::max(0.0f, safe_width_px - group_w);
    const float group_r   = group_l + group_w;
    m_top_group_left_px   = group_l;
    m_top_group_right_px  = group_r;

    m_easymode_main_toolbar.set_scroll(0.0f);
    m_easymode_main_toolbar.set_limit_width(icons_space_px);

    const float toolbar_left_px = m_top_group_left_px + prefix_w_with_gap;
    const float left_ndc        = -0.5f * canvas_w + toolbar_left_px;

    m_easymode_main_toolbar.set_position(top_ndc, left_ndc);
    m_easymode_main_toolbar.render(*this, m_easymode_main_toolbar.get_scroll());

    if (m_toolbar_highlighter.m_render_arrow)
        m_easymode_main_toolbar.render_arrow(*this, m_toolbar_highlighter.m_toolbar_item);

    // _render_slice_control_simple();

    _render_device_list_dropdown_panel_simple();
}

// A white panel placed right under the whole header row (centered group).
void GLCanvas3D::_render_device_list_dropdown_panel_simple() const
{
    if (!m_preset_panel_open) return;

    const float scale        = get_scale();
    const ImVec2 display_sz  = ImGui::GetIO().DisplaySize;
    const float safe_left_px = get_easy_mode_overlay_safe_left_px();
    const float safe_right_px = get_easy_mode_overlay_safe_right_px();
    const float safe_width_px = std::max(0.0f, display_sz.x - safe_left_px - safe_right_px);
    const float safe_bottom_px = get_easy_mode_overlay_safe_bottom_px();
    float panel_w   = 750.0f * scale;
    float panel_h   = 380.0f * scale; // default when there are devices (<=3 handled below)
    const float toolbar_h    = m_easymode_main_toolbar.get_height_horizontal_simple();
    const float popup_gap_y  = (30.0f - TOP_GAP_DISTANCE) * scale;

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoResize   |
                             ImGuiWindowFlags_NoScrollbar| ImGuiWindowFlags_NoScrollWithMouse;

    // Adjust size by number of devices; enable scrolling when > 6
    if (1) {
        const auto& dl = SimpleDeviceMgr::instance().get_device_list_data_simple(false);
        // Count visible devices (online + offline)
        int dev_count = 0;
        for (const auto& k : dl.online_device_list)  { auto it = dl.datas.find(k); if (it != dl.datas.end() && it->second.visible) ++dev_count; }
        for (const auto& k : dl.offline_device_list) { auto it = dl.datas.find(k); if (it != dl.datas.end() && it->second.visible) ++dev_count; }

        if (dev_count <= 0) {
            panel_h = 100.0f * scale; // no devices
        } else if (dev_count <= 3) {
            panel_h = 380.0f * scale; // up to 3 devices
        } else if (dev_count <= 6) {
            panel_h = 700.0f * scale; // 4~6 devices
        } else {
            panel_h = 700.0f * scale; // more than 6 devices: keep height, enable scrollbar
            flags &= ~(ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        }
        panel_w = 752.0f * scale; // fixed width
    }

    if (safe_width_px > 1.0f) {
        const float min_panel_w = std::min(240.0f * scale, safe_width_px);
        panel_w = std::clamp(panel_w, min_panel_w, safe_width_px);
    }

    ImVec4 anchor_rect = m_device_card_anchor_valid
        ? m_device_card_anchor_rect
        : ImVec4(m_top_group_left_px, TOP_GAP_DISTANCE * scale, m_top_prefix_width_px, toolbar_h);

    // float panel_x = anchor_rect.x;
    float panel_x = safe_left_px + 0.5f * std::max(0.0f, safe_width_px - panel_w);
    float panel_y = anchor_rect.y + anchor_rect.w + popup_gap_y;

    const float max_panel_h = std::max(80.0f * scale, display_sz.y - safe_bottom_px - panel_y);
    if (panel_h > max_panel_h) {
        panel_h = max_panel_h;
        flags &= ~(ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    }

    const float max_panel_x = std::max(safe_left_px, display_sz.x - safe_right_px - panel_w);
    panel_x = std::max(safe_left_px, std::min(panel_x, max_panel_x));

    ImGui::SetNextWindowPos(ImVec2(panel_x, panel_y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panel_w, panel_h));

    // Slightly larger inner padding for the popup
    const float panel_pad = 15.0f * scale;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(panel_pad, panel_pad));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    const bool dark = wxGetApp().dark_mode();
    ImGui::PushStyleColor(ImGuiCol_WindowBg, dark ? ImVec4(0.157f,0.157f,0.172f,1.0f) : ImVec4(1.0f,1.0f,1.0f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border,   dark ? ImVec4(0.431f,0.431f,0.447f,0.78f) : ImVec4(0.753f,0.753f,0.784f,0.78f));
    // Thin, subtle scrollbar like the mockup: narrow, pill-shaped grab, light track
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 10.0f * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 4.0f * scale);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,          ImVec4(0.0f,0.0f,0.0f,0.0f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab,        dark ? ImVec4(0.42f,0.42f,0.46f,1.0f) : ImVec4(0.71f,0.73f,0.78f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, dark ? ImVec4(0.50f,0.50f,0.54f,1.0f) : ImVec4(0.62f,0.64f,0.68f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive,  dark ? ImVec4(0.58f,0.58f,0.62f,1.0f) : ImVec4(0.54f,0.56f,0.60f,1.0f));
    if (ImGui::Begin("##preset_dropdown_panel_simple", nullptr, flags)) {
        const ImVec2 win_pos  = ImGui::GetWindowPos();
        const ImVec2 win_size = ImGui::GetWindowSize();
        const float  bottom   = win_pos.y + win_size.y;
        render_device_list_popup(*const_cast<GLCanvas3D*>(this), win_pos.x, win_pos.y, bottom);

        // Close when clicking outside this panel (or pressing ESC), like other popups
        if (_should_close_current_imgui_popup())
            m_preset_panel_open = false;
    }
    ImGui::End();
    ImGui::PopStyleColor(6);
    ImGui::PopStyleVar(5);

    // No style pops needed since we did not push
}



void GLCanvas3D::_render_slice_control_simple() const
{
    auto push_green_button_style = [](){
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.16f, 0.74f, 0.33f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.14f, 0.66f, 0.30f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.12f, 0.58f, 0.26f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(1,1,1,1));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    };
    auto pop_green_button_style = [](){
        ImGui::PopStyleVar(1);
        ImGui::PopStyleColor(4);
    };

    const float scale   = get_scale();
    
    const float pad_px  = 6.0f * scale;
    const float gap_px  = 8.0f * scale;

    // These two are already computed this frame by main_toolbar_simple()
    const float prefix_w       = m_top_prefix_width_px;
    const float toolbar_height = m_easymode_main_toolbar.get_height_horizontal_simple();

    const ImVec2 preview_btn_sz(PREVIEW_BTN_WIDTH * scale, toolbar_height);
    const ImVec2 send_btn_sz   (SEND_PRINT_BTN_WIDTH * scale, toolbar_height);

    const float after_toolbar_gap = std::max(gap_px, 6.0f * scale);

    // Where to start the right-button block: just after the toolbar??s rendered right edge.
    const float block_left_px = m_easymode_main_toolbar.get_rendered_right_edge_px(*this) + after_toolbar_gap;

    // Y align: vertically center buttons inside the toolbar strip
    const float y_offset = 0.0f;

    // Total width of the two-button block
    //const float right_block_w = pad_px + big_btn.x + gap_px + big_btn.x + pad_px;
    const float right_block_w = pad_px + preview_btn_sz.x + gap_px + send_btn_sz.x + pad_px;

    ImGuiWindowFlags wflags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                              ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoResize   |
                              ImGuiWindowFlags_NoScrollbar| ImGuiWindowFlags_NoScrollWithMouse |
                              ImGuiWindowFlags_NoBackground;

    ImGui::SetNextWindowPos(ImVec2(block_left_px, y_offset + TOP_GAP_DISTANCE * scale), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(right_block_w, toolbar_height), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (ImGui::Begin("##mtb_right_buttons_simple", nullptr, wflags)) {

        ImGui::SetCursorPosX(pad_px);

        // Preview
        const WorkflowToolbarState workflow_toolbar_state = GetAIWorkflowToolbarState();
        const bool can_slice = (wxGetApp().mainframe && wxGetApp().mainframe->get_enable_slice_status()) && (!workflow_toolbar_state.available || workflow_toolbar_state.can_slice);
        {
            ImGuiDisableScope _disable(!can_slice);
            push_green_button_style();
            if (ImGui::Button(_u8L("Slice plate").c_str(), preview_btn_sz) && can_slice)
                OpenActiveAISliceWorkflowFromScene();
            pop_green_button_style();
        }

        ImGui::SameLine(0.0f, gap_px);

        // Send
        const DM::Device& cur_dev          = DM::DataCenter::Ins().get_current_device_data();
        const bool        has_bound_device = (cur_dev.valid && !cur_dev.address.empty());
        const bool        device_online    = (cur_dev.valid && cur_dev.online);

        const bool printer_idle = (cur_dev.valid && cur_dev.deviceState == 0);

        /*const bool        can_send         = (wxGetApp().mainframe &&
                                              wxGetApp().mainframe->get_enable_print_status(false) &&
                                              has_bound_device && device_online && printer_idle);*/
        const bool basic_available   = (wxGetApp().mainframe && wxGetApp().mainframe->get_enable_print_status(false));
        const bool printer_available = (has_bound_device && device_online && printer_idle);
        const bool can_send = basic_available && printer_available && (!workflow_toolbar_state.available || workflow_toolbar_state.can_send_print);
        
        std::string printer_tooltip;
        if (!has_bound_device) {
            printer_tooltip = _u8L("No bound printer device found, please bind a printer first.").c_str();
        } else if (!device_online) {
            printer_tooltip = _u8L("The bound printer is offline, please check the printer connection.").c_str();
        } else if (!printer_idle) {
            printer_tooltip = _u8L("The printer is not idle and cannot send print jobs for the moment.").c_str();
        }

        {
            ImGuiDisableScope _disable(!can_send);
            push_green_button_style();
            if (ImGui::Button(_u8L("Send print").c_str(), send_btn_sz)) {
                if (can_send)
                    open_easy_mode_send_workflow_from_scene();
            }
            pop_green_button_style();

            if (!can_send && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                if (basic_available && !printer_available) {
                    ImGui::SetTooltip("%s", printer_tooltip.c_str());
                }
            }

        }
    }
    ImGui::End();
    ImGui::PopStyleVar(); // WindowPadding
}

void GLCanvas3D::render_preview_top_buttons_simple()
{
    const float scale    = this->get_scale();
    const Size  cnv_size = this->get_canvas_size();
    const float canvas_w = float(cnv_size.get_width());
    const float safe_left_px = get_easy_mode_overlay_safe_left_px();
    const float safe_right_px = std::max(safe_left_px, canvas_w - get_easy_mode_overlay_safe_right_px());
    const float safe_width_px = std::max(0.0f, safe_right_px - safe_left_px);

    const float pad_px   = 6.0f * scale;
    const float gap_px   = 8.0f * scale;

    const float btn_h         = 40.0f * scale;
    const float exit_btn_w    = 136.0f * scale;
    const float slice_btn_w   = 176.0f * scale;
    const float send_btn_w    = 176.0f * scale;

    const ImVec2 exit_btn_sz (exit_btn_w,  btn_h);
    const ImVec2 slice_btn_sz(slice_btn_w, btn_h);
    const ImVec2 send_btn_sz (send_btn_w,  btn_h);

   // const float block_w = pad_px + exit_btn_sz.x + gap_px + slice_btn_sz.x + gap_px + send_btn_sz.x + pad_px;
    const float block_w = pad_px + slice_btn_sz.x + gap_px + send_btn_sz.x + pad_px;

    const float x       = safe_left_px + 0.5f * std::max(0.0f, safe_width_px - block_w);
    const float y       = TOP_GAP_DISTANCE * scale;

    ImGuiWindowFlags wflags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                              ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoResize   |
                              ImGuiWindowFlags_NoScrollbar| ImGuiWindowFlags_NoScrollWithMouse |
                              ImGuiWindowFlags_NoBackground;

    auto push_green_button_style = [](){
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.16f, 0.74f, 0.33f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.14f, 0.66f, 0.30f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.12f, 0.58f, 0.26f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(1,1,1,1));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    };
    auto pop_green_button_style = [](){
        ImGui::PopStyleVar(1);
        ImGui::PopStyleColor(4);
    };
    auto push_exit_button_style = [](){
        const ImVec4 bg       = ImVec4(1.0f, 1.0f, 1.0f, 0.6f); // normal 60%
        const ImVec4 bg_hover = ImVec4(1.0f, 1.0f, 1.0f, 0.8f); // hover / active 80%
        const ImVec4 txt      = ImVec4(48.0f / 255.0f, 55.0f / 255.0f, 61.0f / 255.0f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button,        bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, bg_hover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  bg_hover);
        ImGui::PushStyleColor(ImGuiCol_Text,          txt);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    };
    auto pop_exit_button_style = [](){
        ImGui::PopStyleVar(1);
        ImGui::PopStyleColor(4);
    };

    ImGui::SetNextWindowPos (ImVec2(x, y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(block_w, btn_h), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (ImGui::Begin("##preview_top_buttons_simple", nullptr, wflags)) {
        // Enlarge font for these three buttons
        ImGuiIO& io = ImGui::GetIO();
        if (io.FontDefault != nullptr) {
            float base_size = io.FontDefault->FontSize;
            if (base_size > 0.0f) {
                float font_scale = 16.0f / base_size;
                ImGui::SetWindowFontScale(font_scale);
            }
        }

        ImGui::SetCursorPosX(pad_px);

        /*
        // Exit preview
        {
            push_exit_button_style();
            std::string label = "X " + _u8L("Exit preview");
            if (ImGui::Button(label.c_str(), exit_btn_sz)) {
                if (wxGetApp().mainframe)
                    wxGetApp().mainframe->request_select_tab(MainFrame::TabPosition::tp3DEditor);
            }
            pop_exit_button_style();
        }

        ImGui::SameLine(0.0f, gap_px);
        */

        // Slice current plate
        const WorkflowToolbarState workflow_toolbar_state = GetAIWorkflowToolbarState();
        const bool can_slice = (wxGetApp().mainframe && wxGetApp().mainframe->get_enable_slice_status()) && (!workflow_toolbar_state.available || workflow_toolbar_state.can_slice);
        {
            if (can_slice) {
                push_green_button_style();
                if (ImGui::Button(_u8L("Slice plate").c_str(), slice_btn_sz))
                    OpenActiveAISliceWorkflowFromScene();
                pop_green_button_style();
            } else {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                push_exit_button_style();
                ImGui::Button(_u8L("Slice plate").c_str(), slice_btn_sz);
                pop_exit_button_style();
                ImGui::PopItemFlag();
            }
        }

        ImGui::SameLine(0.0f, gap_px);

        // Send print
        /*const DM::Device& cur_dev          = DM::DataCenter::Ins().get_current_device_data();
        const bool        has_bound_device = (cur_dev.valid && !cur_dev.address.empty());
        const bool        device_online    = (cur_dev.valid && cur_dev.online);
        const bool        can_send         = (wxGetApp().mainframe &&
                                              wxGetApp().mainframe->get_enable_print_status(false) &&
                                              has_bound_device && device_online);
        {
            if (can_send) {
                push_green_button_style();
                if (ImGui::Button(_u8L("Send print").c_str(), send_btn_sz))
                    open_easy_mode_send_workflow_from_scene();
                pop_green_button_style();
            } else {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                push_exit_button_style();
                ImGui::Button(_u8L("Send print").c_str(), send_btn_sz);
                pop_exit_button_style();
                ImGui::PopItemFlag();
            }
        }*/

        // Send
        const DM::Device& cur_dev          = DM::DataCenter::Ins().get_current_device_data();
        const bool        has_bound_device = (cur_dev.valid && !cur_dev.address.empty());
        const bool        device_online    = (cur_dev.valid && cur_dev.online);

        const bool printer_idle = (cur_dev.valid && cur_dev.deviceState == 0);

        const bool basic_available   = (wxGetApp().mainframe && wxGetApp().mainframe->get_enable_print_status(false));
        const bool printer_available = (has_bound_device && device_online && printer_idle);
        const bool can_send          = basic_available && printer_available && (!workflow_toolbar_state.available || workflow_toolbar_state.can_send_print);

        std::string printer_tooltip;
        if (!has_bound_device) {
            printer_tooltip = _u8L("No bound printer device found, please bind a printer first.").c_str();
        } else if (!device_online) {
            printer_tooltip = _u8L("The bound printer is offline, please check the printer connection.").c_str();
        } else if (!printer_idle) {
            printer_tooltip = _u8L("The printer is not idle and cannot send print jobs for the moment.").c_str();
        }

        {
            ImGuiDisableScope _disable(!can_send);
            push_green_button_style();
            if (ImGui::Button(_u8L("Send print").c_str(), send_btn_sz)) {
                if (can_send)
                    open_easy_mode_send_workflow_from_scene();
            }
            pop_green_button_style();

            if (!can_send && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                if (basic_available && !printer_available) {
                    ImGui::SetTooltip("%s", printer_tooltip.c_str());
                }
            }
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(); // WindowPadding
}


// Draw an inline preset pill: [icon] [Title on top][Subtitle below].
// White rounded background; green border on hover.
// Updates m_top_prefix_width_px. Aligns vertically with toolbar height.
void GLCanvas3D::_render_cur_device_card_simple(const ImVec4& rect, const char* window_id, bool update_top_prefix_width)
{
    const float scale   = get_scale();
    const float block_h = rect.w;

    const float pad_x  = 8.0f * scale;
    const float gap_x  = 8.0f * scale;
    const float icon_w = 48.0f * scale;
    const float icon_h = 48.0f * scale;
    const float radius = 6.0f * scale;

    std::string title;
    std::string subtitle;

    std::string desired_icon_path = m_cur_device_icon_path;
    SimpleDeviceMgr::instance().get_cur_device_info(title, subtitle, desired_icon_path);
    if (desired_icon_path.empty())
        desired_icon_path = Slic3r::resources_dir() + "/images/current_no_device_simple.svg";
    if (desired_icon_path != m_cur_device_icon_path) {
        m_cur_device_icon_path = desired_icon_path;
        _ensure_preset_icon_tex((unsigned)icon_w, (unsigned)icon_h);
    }

    const ImVec2 title_sz = ImGui::CalcTextSize(title.c_str());
    const ImVec2 subtitle_sz = ImGui::CalcTextSize(subtitle.c_str());
    const float content_w = rect.z;

    if (update_top_prefix_width)
        m_top_prefix_width_px = content_w;
    m_device_card_anchor_valid = false;

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                             ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings;

    ImGui::SetNextWindowPos(ImVec2(rect.x, rect.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(content_w, block_h));
    if (ImGui::Begin(window_id, nullptr, flags)) {
        ImDrawList* dl       = ImGui::GetWindowDrawList();
        const ImVec2 win_pos = ImGui::GetWindowPos();
        const ImVec2 win_sz  = ImGui::GetWindowSize();
        const ImVec2 win_max = win_pos + win_sz;

        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::InvisibleButton("##preset_hit", win_sz);
        const bool hovered = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked()) {
            // m_preset_panel_open = !m_preset_panel_open;
            const bool open_preset_panel = !m_preset_panel_open;
            m_easymode_main_toolbar.reset_item_state_simple();
            m_supports_popup_open = false;
            m_preset_panel_open = open_preset_panel;
        }

        {
            const bool  dark      = wxGetApp().dark_mode();
            const float radius_px = radius;
            const float border_px = 1.0f;
            const bool  drop_shad = false;
            const float shadow_px = 6.0f * scale;

            ImU32 fill_col = ImGui::GetColorU32(dark
                ? ImVec4(0.12f, 0.12f, 0.14f, 0.85f)
                : ImVec4(1.00f, 1.00f, 1.00f, 0.92f));

            ImU32 base_border_col = dark
                ? IM_COL32(110,110,114,200)
                : IM_COL32(192,192,200,200);

            ImU32 shadow_col = ImGui::GetColorU32(ImVec4(0,0,0, 40.0f/255.0f));

            draw_toolbar_bg_round_rect_px(
                win_pos.x, win_pos.y, win_max.x, win_max.y,
                radius_px, border_px, fill_col, base_border_col,
                drop_shad, shadow_px, shadow_col
            );
        }

        if (hovered) {
            ImDrawList* fdl = ImGui::GetForegroundDrawList();
            const ImU32 hover_col = IM_COL32(41,190,85,255);
            const float inset = 0.75f;
            ImVec2 min2 = ImVec2(win_pos.x + inset, win_pos.y + inset);
            ImVec2 max2 = ImVec2(win_max.x - inset, win_max.y - inset);
            fdl->AddRect(min2, max2, hover_col, radius, 0, 2.0f);
        }

        const ImVec2 icon_pos = win_pos + ImVec2(pad_x, 0.5f * (block_h - icon_h));
        if (m_preset_icon_tex != nullptr) {
            dl->AddImage(m_preset_icon_tex, icon_pos, icon_pos + ImVec2(icon_w, icon_h));
        } else {
            const ImU32 ph_col = IM_COL32(200, 200, 200, 255);
            dl->AddRectFilled(icon_pos, icon_pos + ImVec2(icon_w, icon_h), ph_col, 4.0f * scale);
        }

        const float text_x = pad_x + icon_w + gap_x;
        const float font_px = ImGui::GetFontSize();
        const float line_gap = 0.15f * font_px;
        const float total_text_height = title_sz.y + line_gap + subtitle_sz.y;
        const float text_start_y = 0.5f * (block_h - total_text_height);
        const ImVec2 title_pos = win_pos + ImVec2(text_x, text_start_y);
        const ImVec2 subtitle_pos = title_pos + ImVec2(0.0f, title_sz.y + line_gap);

        const bool dark_mode = wxGetApp().dark_mode();
        const ImU32 title_col = IM_COL32(41,190,85,255);
        dl->AddText(title_pos, title_col, title.c_str());

        std::string model_label_local, status_txt_local;
        ImU32 status_col_u32 = IM_COL32(255,125,0,255);
        {
            const DM::Device& d = DM::DataCenter::Ins().get_current_device_data();
            if (d.valid) {
                model_label_local = d.modelName;
                if (d.online) {
                    if (d.deviceState == 0) {
                        status_txt_local = _u8L("Idle");
                        status_col_u32 = IM_COL32(41,190,85,255);
                    } else {
                        status_txt_local = _u8L("Printing");
                        status_col_u32 = IM_COL32(44,130,230,255);
                    }
                } else {
                    status_txt_local = _u8L("Offline");
                    status_col_u32 = IM_COL32(255,125,0,255);
                }
            }
        }
        const ImU32 sub_col = dark_mode ? IM_COL32(235,235,235,255) : IM_COL32(96, 96, 96, 255);
        if (model_label_local.empty() && status_txt_local.empty()) {
            const ImU32 empty_sub_col = dark_mode ? IM_COL32(235,235,235,255) : IM_COL32(35,35,35,255);
            dl->AddText(subtitle_pos, empty_sub_col, subtitle.c_str());
        } else {
            const float base_px_sub = ImGui::GetFontSize();
            const float model_px = 16.0f * scale;
            const float model_ratio = (base_px_sub > 0.0f) ? (model_px / base_px_sub) : 1.0f;
            ImVec2 model_sz0 = ImGui::CalcTextSize(model_label_local.c_str());
            ImVec2 model_sz = ImVec2(model_sz0.x * model_ratio, model_sz0.y * model_ratio);
            if (!model_label_local.empty())
                dl->AddText(ImGui::GetFont(), model_px, subtitle_pos, sub_col, model_label_local.c_str());
            if (!status_txt_local.empty()) {
                float gap = model_label_local.empty() ? 0.0f : (6.0f * scale);
                ImVec2 stat_pos = ImVec2(subtitle_pos.x + (model_label_local.empty() ? 0.0f : model_sz.x + gap), subtitle_pos.y);
                dl->AddText(stat_pos, status_col_u32, status_txt_local.c_str());
            }
        }

        const float caret_w = 7.0f * scale;
        const float caret_h = 4.0f * scale;
        const float caret_pad = 8.0f * scale;
        const ImU32 caret_col = IM_COL32(41,190,85,255);
        const float thick = 2.0f;
        if (m_preset_panel_open) {
            ImVec2 left  = ImVec2(win_max.x - caret_pad - 2*caret_w, win_pos.y + 0.5f * block_h + caret_h);
            ImVec2 apex  = ImVec2(win_max.x - caret_pad - caret_w,   win_pos.y + 0.5f * block_h - caret_h);
            ImVec2 right = ImVec2(win_max.x - caret_pad,             win_pos.y + 0.5f * block_h + caret_h);
            dl->AddLine(left, apex, caret_col, thick);
            dl->AddLine(apex, right, caret_col, thick);
        } else {
            ImVec2 left  = ImVec2(win_max.x - caret_pad - 2*caret_w, win_pos.y + 0.5f * block_h - caret_h);
            ImVec2 apex  = ImVec2(win_max.x - caret_pad - caret_w,   win_pos.y + 0.5f * block_h + caret_h);
            ImVec2 right = ImVec2(win_max.x - caret_pad,             win_pos.y + 0.5f * block_h - caret_h);
            dl->AddLine(left, apex, caret_col, thick);
            dl->AddLine(apex, right, caret_col, thick);
        }

        m_device_card_anchor_rect = ImVec4(win_pos.x, win_pos.y, win_sz.x, win_sz.y);
        m_device_card_anchor_valid = true;
    }
    ImGui::End();
}

void GLCanvas3D::_render_cur_device_simple()
{
    const float scale = get_scale();
    const float toolbar_h = m_easymode_main_toolbar.get_height_horizontal_simple();
    const float block_h = toolbar_h;
    const float y_offset = TOP_GAP_DISTANCE * scale;
    const float content_w = 240.0f * scale;

    _render_cur_device_card_simple(ImVec4(m_top_group_left_px, y_offset, content_w, block_h),
                                   "##preset_inline_simple",
                                   true);
}

void GLCanvas3D::_render_supports_popup()
{
    const float scale = get_scale();
    ImGui::SetCursorPos(ImVec2(22.0f * scale, 12.0f * scale));
    SupportSimple::render_simple_input_window();
}

void GLCanvas3D::_render_preset_settings()
{
    if(m_print_settings_panel) {
        bool changed = m_print_settings_panel->render(true);

        if (changed) {
            // TODO
        }
    }

}

// Ensures m_preset_icon_tex is valid and up-to-date with m_cur_device_icon_path.
// Safe to call from const render paths (members are mutable).
void GLCanvas3D::_ensure_preset_icon_tex(unsigned icon_w_px, unsigned icon_h_px) const
{
    // 1) Resolve desired icon path for current preset/printer.
    //    If you already set m_cur_device_icon_path elsewhere, you can keep this as-is.
    std::string desired_path = m_cur_device_icon_path;

    if (desired_path.empty()) {
        // Try obtain a default icon path from your business logic.
        // (Replace this with your own retrieval code if needed.)
        // Example: desired_path = resources_dir() + "/icons/preset_default.svg";
        return; // Nothing to load.
    }

    // 2) If we already have a texture and the path didn't change, nothing to do.
    //    We rely on m_cur_device_icon_path storing the *currently loaded* path.
    //    If you want to support runtime size changes, also compare size here.
    //    (For now, icon_w/h changes will trigger a reload below if you prefer.)
    // NOTE: If you track the "loaded path" in a separate member, compare it here.
    // For simplicity we treat m_cur_device_icon_path as the canonical "to load" path.
    // If you need strict comparison, add: if (m_loaded_icon_path == desired_path) return;

    // 3) (Re)load texture from path (SVG/PNG supported).
    ImTextureID new_tex = nullptr;

    // Free the previous texture before creating a new one to avoid leaks.
    destroy_imgui_texture(m_preset_icon_tex);
    m_preset_icon_tex = nullptr;

    // Clamp to a minimal size for safety.
    icon_w_px = std::max(1u, icon_w_px);
    icon_h_px = std::max(1u, icon_h_px);

    auto ends_with_ci = [](const std::string& s, const char* suf) {
        const size_t n = s.size(), m = strlen(suf);
        if (m > n) return false;
        for (size_t i = 0; i < m; ++i) {
            char a = (char)tolower((unsigned char)s[n - m + i]);
            char b = (char)tolower((unsigned char)suf[i]);
            if (a != b) return false;
        }
        return true;
    };

    bool ok = false;
    if (ends_with_ci(desired_path, ".svg")) {
        ok = IMTexture::load_from_svg_file(desired_path, icon_w_px, icon_h_px, new_tex);
    } else if (ends_with_ci(desired_path, ".png")) {
        ok = IMTexture::load_from_png_file(desired_path, icon_w_px, icon_h_px, new_tex);
    } else {
        // Try SVG then PNG if extension missing
        ok = IMTexture::load_from_svg_file(desired_path, icon_w_px, icon_h_px, new_tex) ||
             IMTexture::load_from_png_file(desired_path, icon_w_px, icon_h_px, new_tex);
    }

    if (ok) {
        m_preset_icon_tex  = new_tex;
        m_cur_device_icon_path = desired_path;
        return;
    }

    // If all failed, leave texture null; UI code should draw a placeholder.
    m_preset_icon_tex = nullptr;
}


void GLCanvas3D::_ensure_ai_chat_logo_tex(unsigned icon_w_px, unsigned icon_h_px) const
{
    if (m_ai_chat_logo_tex != nullptr)
        return;

    icon_w_px = std::max(1u, icon_w_px);
    icon_h_px = std::max(1u, icon_h_px);

    ImTextureID new_tex = nullptr;
    const std::string logo_path = Slic3r::resources_dir() + "/web/image/ai_chat_float_logo.svg";
    if (IMTexture::load_from_svg_file(logo_path, icon_w_px, icon_h_px, new_tex))
        m_ai_chat_logo_tex = new_tex;
}

ImVec2 GLCanvas3D::compute_toolbar_popup_pos_px(
                                           float group_left_px, float group_right_px,
                                           float pref_w, float pref_h,
                                           PopupHAlign halign,
                                           float anchor_x_px,
                                           float margin_top_px)
{
    float bg_l = 0.f, bg_t = 0.f, bg_r = 0.f, bg_b = 0.f;
    // Fallback: if not available (extremely rare if toolbar was not rendered), use group bounds
    if (!m_easymode_main_toolbar.get_last_bg_rect_px(bg_l, bg_t, bg_r, bg_b)) {
        bg_l = group_left_px;
        bg_r = group_right_px;
        bg_t = 0.f;
        bg_b = 0.f;
    }

    // Vertical: directly below the toolbar background with a small margin
    const float win_y = bg_b + margin_top_px;

    // Horizontal by policy
    float win_x = group_left_px;
    switch (halign) {
    case PopupHAlign::GroupCenter: {
        const float center = 0.5f * (group_left_px + group_right_px);
        win_x = center - 0.5f * pref_w;
        break;
    }
    case PopupHAlign::AnchorCenter: {
        const float anchor = (anchor_x_px >= 0.f) ? anchor_x_px : 0.5f * (group_left_px + group_right_px);
        win_x = anchor - 0.5f * pref_w;
        break;
    }
    case PopupHAlign::AlignLeftEdge:
        win_x = group_left_px;
        break;
    case PopupHAlign::AlignRightEdge:
        win_x = group_right_px - pref_w;
        break;
    default:
        break;
    }

    // Clamp inside the whole group horizontally
    if (win_x < group_left_px) win_x = group_left_px;
    if (win_x + pref_w > group_right_px) win_x = std::max(group_left_px, group_right_px - pref_w);

    return ImVec2(win_x, win_y);
}

void GLCanvas3D::_reset_simple_toolbar_item_state()
{
    m_supports_popup_open = false;
    m_preset_settings_open = false;
    m_filament_settings_open = false;
    m_easymode_main_toolbar.reset_item_state_simple();
}

void GLCanvas3D::open_filament_toolbar_popup()
{
    const int filament_id = m_easymode_main_toolbar.get_item_id("filament");
    if (filament_id >= 0) {
        // Simulate a left-click on the filament toolbar item (no hover check).
        m_easymode_main_toolbar.do_action(GLToolbarItem::Left, filament_id, *this, false);
    }
}

bool GLCanvas3D::_should_close_current_imgui_popup() const
{
    // ??????? Begin/End ?????У????? current window ??????
    const ImGuiHoveredFlags kHoverFlags =
        ImGuiHoveredFlags_RootAndChildWindows |
        ImGuiHoveredFlags_AllowWhenBlockedByActiveItem |
        ImGuiHoveredFlags_AllowWhenBlockedByPopup;

    // ImGui 1.89+ ????? Shortcut?????? IsKeyPressed ???????
    //const bool esc = ImGui::Shortcut(ImGuiKey_Escape) || ImGui::IsKeyPressed(ImGuiKey_Escape, false);
    const bool esc = ImGui::IsKeyPressed(ImGuiKey_Escape, false);
    const ImGuiIO& io = ImGui::GetIO();
    const bool clicked_outside = io.MouseClicked[0] && !ImGui::IsWindowHovered(kHoverFlags) && !io.WantCaptureMouse;
    return esc || clicked_outside;
}

bool GLCanvas3D::_close_popup_if(bool* open_flag)
{
    if (!_should_close_current_imgui_popup()) return false;
    if (open_flag) {
        _reset_simple_toolbar_item_state();
    }
    return true;
}

// ??????????????????????λ?????????????????????????
void GLCanvas3D::render_card_popup_if_open(
    bool&                    open_flag,           // ???????????
    float                    left_ndc, float right_ndc,
    float                    popup_h_px,
    PopupHAlign              halign,
    float                    popup_gap_px,
    const char*              window_id,           // ???? "##supports_popover"
    const std::function<void()>& content_fn       // ??????????????? Begin/End??
)
{
    if (!m_canvas || !open_flag) return;

    const float anchor_x_px = ndc_center_to_px(left_ndc, right_ndc);
    const bool is_supports_popup = window_id != nullptr && std::strcmp(window_id, "##supports_popover") == 0;
    const ImVec2 display_sz = ImGui::GetIO().DisplaySize;
    const float safe_left_px = get_easy_mode_overlay_safe_left_px();
    const float safe_right_edge_px = display_sz.x - get_easy_mode_overlay_safe_right_px();
    const float safe_width_px = std::max(0.0f, safe_right_edge_px - safe_left_px);
    const float scale = get_scale();
    const float support_popup_side_inset_px = 20.0f * scale;
    const float support_popup_tile_gap_px   = 18.0f * scale;
    const float support_popup_img_px        = 138.0f * scale;
    const float support_popup_design_w_px   = support_popup_img_px * 5.0f
                                            + support_popup_tile_gap_px * 4.0f
                                            + support_popup_side_inset_px * 2.0f
                                            + ImGui::GetStyle().WindowPadding.x * 2.0f;
    float popup_w_px = is_supports_popup ? support_popup_design_w_px : std::max(0.f, m_top_group_width_px);
    if (is_supports_popup && safe_width_px > 1.0f) {
        const float min_popup_w_px = std::min(360.0f * scale, safe_width_px);
        popup_w_px = std::clamp(popup_w_px, min_popup_w_px, safe_width_px);
    }

    float popup_h_px_for_window = popup_h_px;
    if (is_supports_popup) {
        const float side_inset = 20.0f * scale;
        const float tile_gap   = support_popup_tile_gap_px;
        const float max_img    = 138.0f * scale;
        const float min_img    = 48.0f * scale;
        const float content_w  = std::max(1.0f, popup_w_px - ImGui::GetStyle().WindowPadding.x * 2.0f - side_inset * 2.0f);
        const float fit_img    = (content_w - tile_gap * 4.0f) / 5.0f;
        const float img_edge   = std::max(min_img, std::min(max_img, fit_img));
        const float title_h    = 28.0f * scale;
        const float label_h    = 2.0f * ImGui::GetTextLineHeightWithSpacing() + 6.0f * scale;
        popup_h_px_for_window  = std::max(190.0f * scale, title_h + img_edge + label_h + 18.0f * scale);
    }

    ImVec2 win_pos = compute_toolbar_popup_pos_px(
        m_top_group_left_px, m_top_group_right_px,
        popup_w_px, popup_h_px_for_window, halign, anchor_x_px, popup_gap_px
    );

    if (is_supports_popup) {
        win_pos.x = safe_left_px + 0.5f * std::max(0.0f, safe_width_px - popup_w_px);
    }

    ImGui::SetNextWindowPos(win_pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(popup_w_px, popup_h_px_for_window), ImGuiCond_Always);

    if (is_supports_popup) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f * get_scale());
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    }

    ImGuiWindowFlags wflags = ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoResize   | ImGuiWindowFlags_NoScrollbar
        | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoBackground;

    if (!ImGui::Begin(window_id, nullptr, wflags)) {
        ImGui::End();
        if (is_supports_popup)
            ImGui::PopStyleVar(2);
        return;
    }

    // ?????????
    {
        const float scale = get_scale();
        ImDrawList* dl    = ImGui::GetWindowDrawList();
        const ImVec2 p    = ImGui::GetWindowPos();
        const ImVec2 sz   = ImGui::GetWindowSize();
        const ImVec2 q    = p + sz;

        const bool  dark  = wxGetApp().dark_mode();
        // const ImU32 bg    = dark ? IM_COL32(50, 50, 50, 255) : IM_COL32(255, 255, 255, 255);
        // const ImU32 br    = dark ? IM_COL32(110,110,114,200) : IM_COL32(192,192,200,200);
        // const float r     = 10.0f * scale;
        const ImU32 bg = is_supports_popup
            // ? (dark ? IM_COL32(34, 34, 34, 100) : IM_COL32(255, 255, 255, 100))
            // ? (dark ? IM_COL32(34, 34, 34, 80) : IM_COL32(255, 255, 255, 80))
            // ? (dark ? IM_COL32(34, 34, 34, 64) : IM_COL32(255, 255, 255, 64))
            // ? (dark ? IM_COL32(34, 34, 34, 112) : IM_COL32(255, 255, 255, 112))
            // ? (dark ? IM_COL32(34, 34, 34, 160) : IM_COL32(255, 255, 255, 160))
            ? (dark ? IM_COL32(34, 34, 34, 200) : IM_COL32(255, 255, 255, 200))
            : (dark ? IM_COL32(50, 50, 50, 255) : IM_COL32(255, 255, 255, 255));
        const ImU32 br = is_supports_popup
            ? (dark ? IM_COL32(255, 255, 255, 34) : IM_COL32(225, 228, 233, 255))
            : (dark ? IM_COL32(110,110,114,200) : IM_COL32(192,192,200,200));
        const float r = is_supports_popup ? 12.0f * scale : 10.0f * scale;

        dl->AddRectFilled(p, q, bg, r, ImDrawFlags_RoundCornersAll);
        dl->AddRect(p, q, br, r, ImDrawFlags_RoundCornersAll, 1.0f);
    }

    // ???????????????????
    content_fn();

    // ????????
    if (_close_popup_if(&open_flag)) {
        ImGui::End();
        if (is_supports_popup)
            ImGui::PopStyleVar(2);
        return;
    }

    ImGui::End();
    if (is_supports_popup)
        ImGui::PopStyleVar(2);
}

ImVec4 GLCanvas3D::_compute_object_drawer_rect_simple() const
{
    const float scale = get_scale();
    const ImVec2 canvas_sz = ImGui::GetIO().DisplaySize;
    const ImVec4 resident_rect = _compute_resident_filament_rect_simple();
    const ImVec2 object_info_window = DispConfig().getWindowSize(DispConfig::e_wt_msg, scale);
    const float object_info_reserve = !m_selection.is_empty() ? (object_info_window.y + 20.0f * scale) : 0.0f;
    const float bottom_bound = std::max(resident_rect.y + resident_rect.w, canvas_sz.y - 8.0f * scale - object_info_reserve);
    const float panel_gap = kEasyModeLeftWorkspaceSectionGap * scale;
    const float panel_y = resident_rect.y + resident_rect.w + panel_gap;
    const float panel_h = std::max(1.0f, bottom_bound - panel_y);
    return ImVec4(resident_rect.x, panel_y, resident_rect.z, panel_h);
}

ImVec4 GLCanvas3D::_compute_resident_filament_rect_default_simple() const
{
    const float scale = get_scale();
    const ImVec2 canvas_sz = ImGui::GetIO().DisplaySize;
    const float panel_x = 18.0f * scale;
    const float panel_y = get_main_toolbar_height() + 14.0f * scale;
    const float panel_w = std::min(430.0f * scale, canvas_sz.x * 0.24f);
    const float panel_h = std::min(720.0f * scale, canvas_sz.y * 0.82f);
    return ImVec4(panel_x, panel_y, panel_w, panel_h);
}
ImVec4 GLCanvas3D::_clamp_resident_filament_rect_simple(const ImVec4& rect, GLCanvas3D::ResidentFilamentResizeEdge active_edge) const
{
    const auto edge_bits = [](ResidentFilamentResizeEdge edge) -> unsigned int {
        return static_cast<unsigned int>(edge);
    };
    const auto has_edge = [&](ResidentFilamentResizeEdge mask, ResidentFilamentResizeEdge edge) {
        return (edge_bits(mask) & edge_bits(edge)) != 0;
    };
    const float scale = get_scale();
    const ImVec2 canvas_sz = ImGui::GetIO().DisplaySize;
    const float left_bound = 8.0f * scale;
    const float top_bound = get_main_toolbar_height() + 14.0f * scale;
    const float right_bound = std::max(left_bound, canvas_sz.x - 8.0f * scale);
    const ImVec2 object_info_window = DispConfig().getWindowSize(DispConfig::e_wt_msg, scale);
    const float object_info_reserve = !m_selection.is_empty() ? (object_info_window.y + 20.0f * scale) : 0.0f;
    const float bottom_bound = std::max(top_bound, canvas_sz.y - 8.0f * scale - object_info_reserve);
    const float max_available_w = std::max(1.0f, right_bound - left_bound);
    const float max_available_h = std::max(1.0f, bottom_bound - top_bound);
    const float min_w = std::min(410.0f * scale, max_available_w);
    const float min_h = std::min(520.0f * scale, max_available_h);
    const float max_w = std::min(std::min(680.0f * scale, canvas_sz.x * 0.45f), max_available_w);
    const float max_h = max_available_h;

    float width = std::max(min_w, std::min(rect.z, std::max(min_w, max_w)));
    float height = std::max(min_h, std::min(rect.w, std::max(min_h, max_h)));
    float x = rect.x;
    float y = rect.y;
    const float desired_right = rect.x + rect.z;
    const float desired_bottom = rect.y + rect.w;

    if (has_edge(active_edge, ResidentFilamentResizeEdge::Left) && !has_edge(active_edge, ResidentFilamentResizeEdge::Right)) {
        const float anchored_right = std::max(left_bound + width, std::min(desired_right, right_bound));
        x = anchored_right - width;
    } else {
        x = std::max(left_bound, std::min(x, right_bound - width));
    }

    if (has_edge(active_edge, ResidentFilamentResizeEdge::Top) && !has_edge(active_edge, ResidentFilamentResizeEdge::Bottom)) {
        const float anchored_bottom = std::max(top_bound + height, std::min(desired_bottom, bottom_bound));
        y = anchored_bottom - height;
    } else {
        y = std::max(top_bound, std::min(y, bottom_bound - height));
    }

    if (has_edge(active_edge, ResidentFilamentResizeEdge::Right) && !has_edge(active_edge, ResidentFilamentResizeEdge::Left)) {
        const float clamped_right = std::min(right_bound, std::max(left_bound + width, x + width));
        width = clamped_right - x;
    }
    if (has_edge(active_edge, ResidentFilamentResizeEdge::Bottom) && !has_edge(active_edge, ResidentFilamentResizeEdge::Top)) {
        const float clamped_bottom = std::min(bottom_bound, std::max(top_bound + height, y + height));
        height = clamped_bottom - y;
    }

    width = std::max(min_w, std::min(width, std::max(min_w, max_w)));
    height = std::max(min_h, std::min(height, std::max(min_h, max_h)));
    x = std::max(left_bound, std::min(x, right_bound - width));
    y = std::max(top_bound, std::min(y, bottom_bound - height));
    return ImVec4(x, y, width, height);
}
void GLCanvas3D::_load_resident_filament_layout_simple()
{
    auto& state = m_resident_filament_layout_state;
    state.loaded_from_config = true;
    state.user_override = false;
    state.x = 0.0f;
    state.y = 0.0f;
    state.w = 0.0f;
    state.h = 0.0f;
    AppConfig* app_config = wxGetApp().app_config;
    if (app_config == nullptr)
        return;
    std::string override_value;
    if (!app_config->get(kEasyModeLayoutSection, kResidentFilamentPanelOverrideKey, override_value))
        return;
    if (!(override_value == "true" || override_value == "1"))
        return;
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    if (!parse_layout_float(app_config->get(kEasyModeLayoutSection, kResidentFilamentPanelXKey), x) ||
        !parse_layout_float(app_config->get(kEasyModeLayoutSection, kResidentFilamentPanelYKey), y) ||
        !parse_layout_float(app_config->get(kEasyModeLayoutSection, kResidentFilamentPanelWidthKey), w) ||
        !parse_layout_float(app_config->get(kEasyModeLayoutSection, kResidentFilamentPanelHeightKey), h))
        return;
    state.user_override = true;
    state.x = x;
    state.y = y;
    state.w = w;
    state.h = h;
}
void GLCanvas3D::_save_resident_filament_layout_simple() const
{
    AppConfig* app_config = wxGetApp().app_config;
    if (app_config == nullptr)
        return;
    const auto& state = m_resident_filament_layout_state;
    app_config->set(kEasyModeLayoutSection, kResidentFilamentPanelOverrideKey, state.user_override ? "true" : "false");
    app_config->set(kEasyModeLayoutSection, kResidentFilamentPanelXKey, std::to_string(state.x));
    app_config->set(kEasyModeLayoutSection, kResidentFilamentPanelYKey, std::to_string(state.y));
    app_config->set(kEasyModeLayoutSection, kResidentFilamentPanelWidthKey, std::to_string(state.w));
    app_config->set(kEasyModeLayoutSection, kResidentFilamentPanelHeightKey, std::to_string(state.h));
    app_config->save();
}
void GLCanvas3D::_reset_resident_filament_layout_simple()
{
    auto& state = m_resident_filament_layout_state;
    state.loaded_from_config = true;
    state.user_override = false;
    state.x = 0.0f;
    state.y = 0.0f;
    state.w = 0.0f;
    state.h = 0.0f;
    _save_resident_filament_layout_simple();
}
ImVec4 GLCanvas3D::_compute_resident_filament_rect_simple() const
{
    if (!m_resident_filament_layout_state.loaded_from_config)
        const_cast<GLCanvas3D*>(this)->_load_resident_filament_layout_simple();
    const float scale = get_scale();
    ImVec4 rect = _compute_resident_filament_rect_default_simple();
    if (m_resident_filament_layout_state.user_override) {
        rect.x = m_resident_filament_layout_state.x * scale;
        rect.y = m_resident_filament_layout_state.y * scale;
        rect.z = m_resident_filament_layout_state.w * scale;
        rect.w = m_resident_filament_layout_state.h * scale;
    }
    return _clamp_resident_filament_rect_simple(rect, ResidentFilamentResizeEdge::None);
}

void GLCanvas3D::_render_resident_filament_panel_simple(const ImVec4& rect)
{
    if (!m_imgui_filament_panel)
        return;
    if (m_canvas_type != ECanvasType::CanvasView3D)
        return;

    const auto edge_bits = [](ResidentFilamentResizeEdge edge) -> unsigned int {
        return static_cast<unsigned int>(edge);
    };
    const auto has_edge = [&](ResidentFilamentResizeEdge mask, ResidentFilamentResizeEdge edge) {
        return (edge_bits(mask) & edge_bits(edge)) != 0;
    };
    const auto combine_edges = [&](ResidentFilamentResizeEdge a, ResidentFilamentResizeEdge b) {
        return static_cast<ResidentFilamentResizeEdge>(edge_bits(a) | edge_bits(b));
    };

    ImGuiWrapper& imgui = *wxGetApp().imgui();
    const ImVec2 display_size = ImGui::GetIO().DisplaySize;
    const float view_scale = wxGetApp().plater()->get_current_canvas3D()->get_scale();
    auto& resize_state = m_resident_filament_resize_state;
    const ImVec2 mouse_pos = ImGui::GetIO().MousePos;
    const float edge_hit_thickness = 6.0f * view_scale;

    auto hit_test_edge = [&](const ImVec4& target_rect) {
        const float left = target_rect.x;
        const float top = target_rect.y;
        const float right = target_rect.x + target_rect.z;
        const float bottom = target_rect.y + target_rect.w;
        const float dist_left = std::fabs(mouse_pos.x - left);
        const float dist_right = std::fabs(mouse_pos.x - right);
        const float dist_top = std::fabs(mouse_pos.y - top);
        const float dist_bottom = std::fabs(mouse_pos.y - bottom);
        const bool near_left = mouse_pos.y >= top - edge_hit_thickness && mouse_pos.y <= bottom + edge_hit_thickness && dist_left <= edge_hit_thickness;
        const bool near_right = mouse_pos.y >= top - edge_hit_thickness && mouse_pos.y <= bottom + edge_hit_thickness && dist_right <= edge_hit_thickness;
        const bool near_top = mouse_pos.x >= left - edge_hit_thickness && mouse_pos.x <= right + edge_hit_thickness && dist_top <= edge_hit_thickness;
        const bool near_bottom = mouse_pos.x >= left - edge_hit_thickness && mouse_pos.x <= right + edge_hit_thickness && dist_bottom <= edge_hit_thickness;

        ResidentFilamentResizeEdge horizontal = ResidentFilamentResizeEdge::None;
        ResidentFilamentResizeEdge vertical = ResidentFilamentResizeEdge::None;
        if (near_left || near_right)
            horizontal = (near_left && (!near_right || dist_left <= dist_right)) ? ResidentFilamentResizeEdge::Left : ResidentFilamentResizeEdge::Right;
        if (near_top || near_bottom)
            vertical = (near_top && (!near_bottom || dist_top <= dist_bottom)) ? ResidentFilamentResizeEdge::Top : ResidentFilamentResizeEdge::Bottom;
        if (horizontal != ResidentFilamentResizeEdge::None && vertical != ResidentFilamentResizeEdge::None)
            return combine_edges(horizontal, vertical);
        if (horizontal != ResidentFilamentResizeEdge::None)
            return horizontal;
        if (vertical != ResidentFilamentResizeEdge::None)
            return vertical;
        return ResidentFilamentResizeEdge::None;
    };

    auto set_resize_cursor = [&](ResidentFilamentResizeEdge edge) {
        const bool left = has_edge(edge, ResidentFilamentResizeEdge::Left);
        const bool right = has_edge(edge, ResidentFilamentResizeEdge::Right);
        const bool top = has_edge(edge, ResidentFilamentResizeEdge::Top);
        const bool bottom = has_edge(edge, ResidentFilamentResizeEdge::Bottom);
        if ((left && top) || (right && bottom)) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
            return;
        }
        if ((right && top) || (left && bottom)) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);
            return;
        }
        if (left || right) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            return;
        }
        if (top || bottom)
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    };

    ImVec4 panel_rect = resize_state.dragging ? resize_state.live_rect : rect;
    if (resize_state.dragging) {
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (resize_state.dirty_since_mouse_down)
                _save_resident_filament_layout_simple();
            resize_state.dragging = false;
            resize_state.active_edge = ResidentFilamentResizeEdge::None;
            resize_state.dirty_since_mouse_down = false;
            resize_state.hot_edge = hit_test_edge(panel_rect);
        } else {
            ImVec2 delta(mouse_pos.x - resize_state.drag_start_mouse.x, mouse_pos.y - resize_state.drag_start_mouse.y);
            ImVec4 candidate = resize_state.drag_start_rect;
            if (has_edge(resize_state.active_edge, ResidentFilamentResizeEdge::Left)) {
                candidate.x = resize_state.drag_start_rect.x + delta.x;
                candidate.z = resize_state.drag_start_rect.z - delta.x;
            } else if (has_edge(resize_state.active_edge, ResidentFilamentResizeEdge::Right)) {
                candidate.z = resize_state.drag_start_rect.z + delta.x;
            }
            if (has_edge(resize_state.active_edge, ResidentFilamentResizeEdge::Top)) {
                candidate.y = resize_state.drag_start_rect.y + delta.y;
                candidate.w = resize_state.drag_start_rect.w - delta.y;
            } else if (has_edge(resize_state.active_edge, ResidentFilamentResizeEdge::Bottom)) {
                candidate.w = resize_state.drag_start_rect.w + delta.y;
            }
            panel_rect = _clamp_resident_filament_rect_simple(candidate, resize_state.active_edge);
            resize_state.live_rect = panel_rect;
            resize_state.dirty_since_mouse_down = true;
            resize_state.hot_edge = resize_state.active_edge;
            m_resident_filament_layout_state.user_override = true;
            m_resident_filament_layout_state.x = panel_rect.x / view_scale;
            m_resident_filament_layout_state.y = panel_rect.y / view_scale;
            m_resident_filament_layout_state.w = panel_rect.z / view_scale;
            m_resident_filament_layout_state.h = panel_rect.w / view_scale;
        }
    }

    if (!resize_state.dragging) {
        resize_state.hot_edge = hit_test_edge(panel_rect);
        const bool reset_to_default = resize_state.hot_edge != ResidentFilamentResizeEdge::None &&
                                      ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
        if (reset_to_default) {
            _reset_resident_filament_layout_simple();
            resize_state = ResidentFilamentPanelResizeState{};
            panel_rect = _compute_resident_filament_rect_simple();
            resize_state.hot_edge = hit_test_edge(panel_rect);
        } else if (resize_state.hot_edge != ResidentFilamentResizeEdge::None && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            resize_state.dragging = true;
            resize_state.active_edge = resize_state.hot_edge;
            resize_state.drag_start_mouse = mouse_pos;
            resize_state.drag_start_rect = panel_rect;
            resize_state.live_rect = panel_rect;
            resize_state.dirty_since_mouse_down = false;
        }
    }

    const ResidentFilamentResizeEdge draw_edge = resize_state.dragging ? resize_state.active_edge : resize_state.hot_edge;
    if (draw_edge != ResidentFilamentResizeEdge::None)
        set_resize_cursor(draw_edge);

    m_imgui_filament_panel->set_embed_in_unified_left_panel(should_embed_easy_mode_left_workspace());

    imgui.set_next_window_pos(panel_rect.x, panel_rect.y, ImGuiCond_Always, 0.0f, 0.0f);
    imgui.set_next_window_size(panel_rect.z, panel_rect.w, ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f * view_scale);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    const int window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;
    if (ImGui::Begin("##resident_filament_panel_simple", nullptr, (ImGuiWindowFlags)window_flags)) {
        m_imgui_filament_panel->Render();
        ImGui::End();
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);

    bool reset_clicked = false;
    if (m_resident_filament_layout_state.user_override && !resize_state.dragging) {
        const char* reset_label = "Default";
        const ImVec2 text_size = ImGui::CalcTextSize(reset_label);
        const ImVec2 button_size(text_size.x + 20.0f * view_scale, 26.0f * view_scale);
        const ImVec2 button_pos(panel_rect.x + panel_rect.z - button_size.x - 14.0f * view_scale,
                                panel_rect.y + panel_rect.w - button_size.y - 14.0f * view_scale);
        ImGui::SetNextWindowPos(button_pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(button_size, ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 999.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        const int overlay_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
                                  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                  ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoNav;
        if (ImGui::Begin("##resident_filament_panel_default_button", nullptr, (ImGuiWindowFlags)overlay_flags)) {
            ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
            reset_clicked = ImGui::InvisibleButton("##resident_filament_panel_default_button_hit", button_size);
            const bool hovered = ImGui::IsItemHovered();
            ImDrawList* button_draw_list = ImGui::GetWindowDrawList();
            const ImVec2 min = ImGui::GetWindowPos();
            const ImVec2 max(min.x + button_size.x, min.y + button_size.y);
            button_draw_list->AddRectFilled(min,
                                            max,
                                            hovered ? IM_COL32(33, 39, 46, 230) : IM_COL32(20, 26, 32, 210),
                                            999.0f);
            button_draw_list->AddRect(min,
                                      max,
                                      hovered ? IM_COL32(255, 255, 255, 60) : IM_COL32(255, 255, 255, 30),
                                      999.0f,
                                      0,
                                      1.0f);
            button_draw_list->AddText(ImVec2(min.x + (button_size.x - text_size.x) * 0.5f,
                                             min.y + (button_size.y - text_size.y) * 0.5f),
                                      hovered ? IM_COL32(246, 249, 252, 255) : IM_COL32(214, 222, 230, 240),
                                      reset_label);
            ImGui::End();
        }
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);
    }
    if (reset_clicked) {
        _reset_resident_filament_layout_simple();
        resize_state = ResidentFilamentPanelResizeState{};
    }

    if (draw_edge != ResidentFilamentResizeEdge::None) {
        ImDrawList* draw_list = ImGui::GetForegroundDrawList();
        const float left = panel_rect.x;
        const float top = panel_rect.y;
        const float right = panel_rect.x + panel_rect.z;
        const float bottom = panel_rect.y + panel_rect.w;
        const float highlight_inset = 14.0f * view_scale;
        const float highlight_thickness = resize_state.dragging ? (2.0f * view_scale) : (1.35f * view_scale);
        const ImU32 highlight_col = resize_state.dragging ? IM_COL32(0, 220, 120, 210) : IM_COL32(126, 214, 166, 136);
        if (has_edge(draw_edge, ResidentFilamentResizeEdge::Left))
            draw_list->AddLine(ImVec2(left, top + highlight_inset), ImVec2(left, bottom - highlight_inset), highlight_col, highlight_thickness);
        if (has_edge(draw_edge, ResidentFilamentResizeEdge::Right))
            draw_list->AddLine(ImVec2(right, top + highlight_inset), ImVec2(right, bottom - highlight_inset), highlight_col, highlight_thickness);
        if (has_edge(draw_edge, ResidentFilamentResizeEdge::Top))
            draw_list->AddLine(ImVec2(left + highlight_inset, top), ImVec2(right - highlight_inset, top), highlight_col, highlight_thickness);
        if (has_edge(draw_edge, ResidentFilamentResizeEdge::Bottom))
            draw_list->AddLine(ImVec2(left + highlight_inset, bottom), ImVec2(right - highlight_inset, bottom), highlight_col, highlight_thickness);
    }

    if (resize_state.dragging) {
        const int logical_w = std::max(1, (int)std::lround(panel_rect.z / view_scale));
        const int logical_h = std::max(1, (int)std::lround(panel_rect.w / view_scale));
        const std::string size_label = std::to_string(logical_w) + " x " + std::to_string(logical_h);
        const ImVec2 text_size = ImGui::CalcTextSize(size_label.c_str());
        const ImVec2 box_size(text_size.x + 18.0f * view_scale, text_size.y + 12.0f * view_scale);
        ImVec2 box_pos(mouse_pos.x + 18.0f * view_scale, mouse_pos.y + 18.0f * view_scale);
        box_pos.x = std::min(box_pos.x, display_size.x - box_size.x - 8.0f * view_scale);
        box_pos.y = std::min(box_pos.y, display_size.y - box_size.y - 8.0f * view_scale);
        box_pos.x = std::max(8.0f * view_scale, box_pos.x);
        box_pos.y = std::max(8.0f * view_scale, box_pos.y);
        ImDrawList* draw_list = ImGui::GetForegroundDrawList();
        draw_list->AddRectFilled(box_pos,
                                 ImVec2(box_pos.x + box_size.x, box_pos.y + box_size.y),
                                 IM_COL32(16, 20, 26, 228),
                                 10.0f * view_scale);
        draw_list->AddRect(box_pos,
                           ImVec2(box_pos.x + box_size.x, box_pos.y + box_size.y),
                           IM_COL32(255, 255, 255, 34),
                           10.0f * view_scale,
                           0,
                           1.0f);
        draw_list->AddText(ImVec2(box_pos.x + (box_size.x - text_size.x) * 0.5f,
                                  box_pos.y + (box_size.y - text_size.y) * 0.5f),
                           IM_COL32(241, 245, 249, 255),
                           size_label.c_str());
    }
}

void GLCanvas3D::_render_object_drawer_panel_simple(const ImVec4& rect)
{
    ImGuiWrapper& imgui = *wxGetApp().imgui();
    const float view_scale = wxGetApp().plater()->get_current_canvas3D()->get_scale();
    const bool is_dark = wxGetApp().dark_mode();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f * view_scale, 10.0f * view_scale));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    const ImVec4 transparent(0.0f, 0.0f, 0.0f, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, transparent);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, transparent);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, is_dark ? ImVec4(0.0, 0.0, 0.0, 0.9) : ImVec4(1.0, 1.0, 1.0, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGuiWrapper::COL_CREALITY);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImGuiWrapper::COL_CREALITY);
    ImGui::PushStyleColor(ImGuiCol_Border, transparent);
    ImVec4 tmp_color = ImVec4(158.0f / 255.0, 158.0f / 255.0, 158.0f / 255.0, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImGui::ColorConvertU32ToFloat4(IM_COL32_BLACK_TRANS));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, tmp_color);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, tmp_color);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, tmp_color);

    ImGui::PushStyleColor(ImGuiCol_Button, transparent);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, transparent);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, transparent);

    imgui.set_next_window_pos(rect.x, rect.y, ImGuiCond_Always, 0.0f, 0.0f);
    imgui.set_next_window_size(rect.z, rect.w, ImGuiCond_Always);

    const int window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoBackground;

    ObjectList* obj_list = wxGetApp().obj_list();
    if (obj_list == nullptr) {
        ImGui::PopStyleColor(13);
        ImGui::PopStyleVar(3);
        return;
    }

    if (obj_list->get_left_panel_fold())
        obj_list->set_left_panel_fold(false);

    if (ImGui::Begin("##obj_drawer", nullptr, (ImGuiWindowFlags) window_flags)) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const ImVec2 win_pos = ImGui::GetWindowPos();
        const ImVec2 win_sz = ImGui::GetWindowSize();
        draw_list->AddLine(ImVec2(win_pos.x + 2.0f * view_scale, win_pos.y),
                           ImVec2(win_pos.x + win_sz.x - 2.0f * view_scale, win_pos.y),
                           IM_COL32(255, 255, 255, 16),
                           1.0f);

        ImGui::Dummy(ImVec2(0.0f, 6.0f * view_scale));
        const bool show_objects = _render_global_objects_switch_button();
        if (show_objects) {
            ImGui::Dummy(ImVec2(0.0f, 8.0f * view_scale));
            obj_list->render_plate_tree_by_ImGui();
        }

        m_printer_objects_panel_size = ImGui::GetWindowSize();
        m_printer_objects_panel_pos = ImGui::GetWindowPos();
        ImGui::End();
    }

    ImGui::PopStyleColor(13);
    ImGui::PopStyleVar(3);
}

void GLCanvas3D::_render__obj_list_simple()
{
    m_printer_objects_panel_size = m_printer_objects_panel_pos = ImVec2(0.0f, 0.0f);
    return;
}

void GLCanvas3D::_render_overlays_easymode()
{
    glsafe(::glDisable(GL_DEPTH_TEST));

    if (!should_embed_easy_mode_left_workspace())
        _render_resident_filament_panel_simple(_compute_resident_filament_rect_simple());
    _render__obj_list_simple();

    _check_and_update_toolbar_icon_scale();

    // if (m_canvas_type == ECanvasType::CanvasPreview)
    //     render_preview_top_buttons_simple();
    
    _render_assemble_control();
    _render_assemble_info();

    _render_main_toolbar_simple();
    _render_object_manipulate_toolbar_simple();
    _render_extra_popup();

    _render_imgui_select_plate_toolbar();
    _render_return_toolbar();

    _render_paint_toolbar();

    _render_assemble_view_toolbar();

    _render_3d_navigator();

    // _render_ai_chat_toggle_easymode();

}

void GLCanvas3D::_render_ai_chat_toggle()
{
    if (wxGetApp().easy_mode())
        return;

    const float scale  = get_scale();
    const float btn_w  = 48.0f * scale;
    const float btn_h  = 48.0f * scale;
    const float window_pad = 8.0f * scale;
    const float window_w = btn_w + window_pad * 2.0f;
    const float window_h = btn_h + window_pad * 2.0f;
    const float logo_w = 48.0f * scale;
    const float logo_h = 48.0f * scale;
    const ImVec2 canvas_sz = ImGui::GetIO().DisplaySize;

    auto clamp_pos = [&](ImVec2 pos) {
        pos.x = std::max(0.0f, std::min(pos.x, std::max(0.0f, canvas_sz.x - window_w)));
        pos.y = std::max(0.0f, std::min(pos.y, std::max(0.0f, canvas_sz.y - window_h)));
        return pos;
    };

    auto default_pos = [&]() {
        const float slice_h = 82.0f * scale;
        const float slice_right = canvas_sz.x - _right_leaning_widget_margin() - 6.0f * scale;
        const float slice_top = canvas_sz.y - slice_h - 6.0f * scale;
        return clamp_pos(ImVec2(slice_right - window_w, slice_top - window_h - 8.0f * scale));
    };

    const std::string override_key = kAIChatFloatPosOverrideKey;
    const std::string pos_x_key = kAIChatFloatPosXKey;
    const std::string pos_y_key = kAIChatFloatPosYKey;

    if (!m_ai_chat_float_dragging) {
        m_ai_chat_float_pos_loaded = true;
        m_ai_chat_float_pos_user_override = false;
        m_ai_chat_float_pos = default_pos();

        AppConfig* app_config = wxGetApp().app_config;
        if (app_config != nullptr) {
            std::string override_value;
            if (app_config->get(kProModeLayoutSection, override_key, override_value) &&
                (override_value == "true" || override_value == "1")) {
                float saved_x = 0.0f;
                float saved_y = 0.0f;
                if (parse_layout_float(app_config->get(kProModeLayoutSection, pos_x_key), saved_x) &&
                    parse_layout_float(app_config->get(kProModeLayoutSection, pos_y_key), saved_y)) {
                    m_ai_chat_float_pos_user_override = true;
                    m_ai_chat_float_pos = clamp_pos(ImVec2(saved_x, saved_y));
                }
            }
        }
    }

    if (!m_ai_chat_float_pos_user_override)
        m_ai_chat_float_pos = default_pos();
    else
        m_ai_chat_float_pos = clamp_pos(m_ai_chat_float_pos);

    _ensure_ai_chat_logo_tex((unsigned)std::max(1.0f, logo_w), (unsigned)std::max(1.0f, logo_h));

    ImGui::SetNextWindowPos(m_ai_chat_float_pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(window_w, window_h), ImGuiCond_Always);

    ImGuiWindowFlags ai_flags = ImGuiWindowFlags_NoTitleBar  | ImGuiWindowFlags_NoResize |
                                ImGuiWindowFlags_NoMove      | ImGuiWindowFlags_NoScrollbar |
                                ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        // Auto-open AI assistant on first render in simple mode
        // static bool s_ai_auto_opened = false;
        // if (!s_ai_auto_opened) {
        //     s_ai_auto_opened = true;
        //     Slic3r::GUI::MCPChatWindow::Show();
        // }

    if (ImGui::Begin("##ai_chat_toggle", nullptr, ai_flags)) {
        // ImGui::PushStyleColor(ImGuiCol_Button,         ImVec4(0.00f, 0.73f, 0.42f, 1.0f));
        // ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.00f, 0.63f, 0.36f, 1.0f));
        // ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.00f, 0.53f, 0.30f, 1.0f));
        // ImGui::PushStyleColor(ImGuiCol_Text,           ImVec4(1, 1, 1, 1));
        // ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, btn_w / 2);
        //
        // if (ImGui::Button("AI", ImVec2(btn_w, btn_h)))

        ImGui::SetCursorPos(ImVec2(window_pad, window_pad));
        const bool pressed = ImGui::InvisibleButton("##ai_chat_logo_button", ImVec2(btn_w, btn_h));
        const bool hovered = ImGui::IsItemHovered();
        const bool active  = ImGui::IsItemActive();

        if (ImGui::IsItemActivated()) {
            m_ai_chat_float_dragging = false;
            m_ai_chat_float_dragged_this_action = false;
        }

        if (active && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 4.0f * scale)) {
            ImGuiIO& io = ImGui::GetIO();
            m_ai_chat_float_pos_user_override = true;
            m_ai_chat_float_dragging = true;
            m_ai_chat_float_dragged_this_action = true;
            m_ai_chat_float_pos = clamp_pos(ImVec2(m_ai_chat_float_pos.x + io.MouseDelta.x,
                                                   m_ai_chat_float_pos.y + io.MouseDelta.y));
        }

        if (m_ai_chat_float_pos_user_override && m_ai_chat_float_dragging && ImGui::IsItemDeactivated()) {
            AppConfig* app_config = wxGetApp().app_config;
            if (app_config != nullptr) {
                app_config->set(kProModeLayoutSection, override_key, "true");
                app_config->set(kProModeLayoutSection, pos_x_key, std::to_string(m_ai_chat_float_pos.x));
                app_config->set(kProModeLayoutSection, pos_y_key, std::to_string(m_ai_chat_float_pos.y));
                app_config->save();
                // m_ai_chat_float_dragging = false;
            }
        }

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const ImVec2 pos      = ImGui::GetItemRectMin();
        const ImVec2 center(pos.x + btn_w * 0.5f, pos.y + btn_h * 0.5f);

        if (m_ai_chat_logo_tex != nullptr) {
            const float anim_t = static_cast<float>(ImGui::GetTime());
            const float bob_y = std::sin(anim_t * 1.5f) * 2.0f * scale;
            const float pulse = hovered ? 1.08f : (1.0f + std::sin(anim_t * 2.0f) * 0.035f);
            const float draw_w = logo_w * pulse;
            const float draw_h = logo_h * pulse;
            const ImVec2 logo_min(center.x - draw_w * 0.5f, center.y - draw_h * 0.5f + bob_y);
            const ImVec2 logo_max(center.x + draw_w * 0.5f, center.y + draw_h * 0.5f + bob_y);
            draw_list->AddImage(m_ai_chat_logo_tex, logo_min, logo_max);
        }

        if (pressed && !m_ai_chat_float_dragged_this_action)
            Slic3r::GUI::MCPChatWindow::Toggle();

        if (ImGui::IsItemDeactivated())
            m_ai_chat_float_dragging = false;
    }

    ImGui::End();
    ImGui::PopStyleVar();

    //can not call this every frame; fix:[#17078] Keeping the GPU continuously occupied during the preparation phase, even without performing any additional operations, will cause the software to crash.
    // request_extra_frame();
}

void GLCanvas3D::close_device_list_popup()
{
    m_preset_panel_open = false;
}

void GLCanvas3D::on_easy_mode_switch()
{
    m_resident_filament_resize_state = ResidentFilamentPanelResizeState{};
    if(wxGetApp().easy_mode()) {
        if(m_imgui_filament_panel) {
            m_imgui_filament_panel->reset();
        }
    }
    else
    {
        MCPChatWindow::Hide();

        ObjectList* obj_list = wxGetApp().obj_list();
        if(obj_list) {
            // sync current device when switch from easy_mode to normal_mode
            obj_list->set_last_preset_name(wxGetApp().preset_bundle->printers.get_selected_preset_name());
        }
        //wxGetApp().sidebar().sync_current_device_filament();    
    }
}

void GLCanvas3D::easy_mode_on_scene_reloaded()
{
}

}
}// namespace Slic3r::GUI


