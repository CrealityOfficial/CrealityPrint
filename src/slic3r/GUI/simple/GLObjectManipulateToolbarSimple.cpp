#include "libslic3r/libslic3r.h"
#include "SupportSimple.hpp"
#include "DeviceListSimple.hpp"
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
#include "simple/gpu/GpuOrient.hpp"
#include "libslic3r/Orient.hpp"
#include "slic3r/GUI/Gizmos/GLGizmoPainterBase.hpp"
#include "slic3r/GUI/Gizmos/GLGizmoEmboss.hpp"
#include "slic3r/GUI/Gizmos/GLGizmoFdmSupports.hpp"
#include "slic3r/GUI/Gizmos/GLGizmoClone.hpp"
#include "slic3r/GUI/Gizmos/GLGizmoCut.hpp"
#include "slic3r/GUI/Gizmos/GizmoObjectManipulation.hpp"
#include "slic3r/Utils/UndoRedo.hpp"
#include "slic3r/Utils/MacDarkMode.hpp"
#include "slic3r/Config/DispConfig.h"
#include <slic3r/GUI/GUI_Utils.hpp>
#include "slic3r/GUI/Widgets/SideButton.hpp"

#if ENABLE_RETINA_GL
#include "slic3r/Utils/RetinaHelper.hpp"
#endif

#include <glad/gl.h>

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
#include <array>
#include <limits>
#include <unordered_map>
#include <Eigen/Dense>

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui/imgui_internal.h>

#include <imguizmo/ImGuizmo.h>
#include "libslic3r/common_header/common_header.h"
#include "PrintSettingsPanel.hpp"

#include "GLSimpleUtils.hpp"
#include "print_manage/data/DataCenter.hpp"
#include "filamentMapping/ImGuiFilamentPanel.hpp"
#include <cctype>
#include <cstring>

#define TOP_GAP_DISTANCE 16.0f
#define PREVIEW_BTN_WIDTH 68.0f
#define SEND_PRINT_BTN_WIDTH 112.0f

namespace Slic3r
{
namespace GUI
{

bool consume_simple_arrange_close_requested();

 enum class ObjManipToolbarAnchor { Default, Above, Below, Left, Right };
 std::unordered_map<const GLCanvas3D*, ObjManipToolbarAnchor> s_obj_manip_toolbar_anchor_by_canvas;
 std::unordered_map<const GLCanvas3D*, bool> s_simple_arrange_menu_open_by_canvas;
 std::unordered_map<const GLCanvas3D*, bool> s_simple_arrange_menu_pos_initialized_by_canvas;
 std::unordered_map<const GLCanvas3D*, GLGizmosManager::EType> s_simple_gizmo_popup_type_by_canvas;
 std::unordered_map<const GLCanvas3D*, ImGuiID> s_simple_gizmo_popup_window_id_by_canvas;
 std::unordered_map<const GLCanvas3D*, bool> s_simple_toolbar_waiting_left_up_by_canvas;
 std::unordered_map<const GLCanvas3D*, bool> s_simple_toolbar_left_up_seen_by_canvas;
 std::unordered_map<const GLCanvas3D*, bool> s_simple_orient_menu_open_by_canvas;
 std::unordered_map<const GLCanvas3D*, bool> s_simple_orient_menu_pos_initialized_by_canvas;

namespace {

constexpr float  kObjManipPreferAboveCenterRatio         = 0.60f;
constexpr double kObjManipSideAnchorZoomThresholdFactor  = 5.0;
constexpr float  kObjManipSideAnchorBboxMaxRatioThreshold = 0.35f;

struct ObjManipToolbarScreenRectPx
{
    float min_x = 0.0f;
    float min_y = 0.0f;
    float max_x = 0.0f;
    float max_y = 0.0f;

    float center_x() const { return 0.5f * (min_x + max_x); }
    float center_y() const { return 0.5f * (min_y + max_y); }
};

struct ObjManipToolbarMetrics
{
    float scale          = 1.0f;
    float canvas_w       = 0.0f;
    float canvas_h       = 0.0f;
    float toolbar_h      = 0.0f;
    float icons_space_px = 0.0f;
    float anchor_gap_px  = 0.0f;

    float icon_px    = 0.0f;
    float border_px  = 0.0f;
    float row_gap_px = 0.0f;

    float max_left_span = 0.0f;
    float max_top_span  = 0.0f;
};

struct ObjManipToolbarPlacement
{
    ObjManipToolbarAnchor anchor            = ObjManipToolbarAnchor::Default;
    bool                  expand_up         = false;
    bool                  popup_anchor_above = false;
    float                 toolbar_left_px   = 0.0f;
    float                 toolbar_top_px    = 0.0f;
};

static bool render_simple_tool_window_close_button(ImGuiWindow* window)
{
    if (window == nullptr || !window->WasActive)
        return false;

    const float scale       = std::max(1.0f, ImGui::GetFontSize() / 18.0f);
    const float button_size = 24.0f * scale;
    const float titlebar_h  = std::max(30.0f * scale, ImGui::GetFrameHeight());
    const ImVec2 pos(window->Pos.x + window->Size.x - button_size - 4.0f * scale,
                     window->Pos.y + 0.5f * (titlebar_h - button_size));
    const ImRect bb(pos, ImVec2(pos.x + button_size, pos.y + button_size));

    if (window == ImGui::GetCurrentWindow()) {
        // Do not use InvisibleButton here: positioned layout items can affect
        // AlwaysAutoResize tool windows and make them grow every frame.
        ImGuiID id = window->GetID("##simple_tool_window_close_button");
        bool hovered = false;
        bool held    = false;
        ImGui::PushClipRect(ImVec2(0.0f, 0.0f), ImGui::GetIO().DisplaySize, false);
        ImGui::ItemAdd(bb, id);
        const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_PressedOnClickRelease);
        ImGui::PopClipRect();

        // ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImDrawList* draw_list = ImGui::GetForegroundDrawList();
        if (hovered || held) {
            const ImU32 hover_col = ImGui::GetColorU32(ImVec4(0.082f, 0.749f, 0.349f, 1.0f));
            draw_list->AddRect(bb.Min, bb.Max, hover_col, 2.0f * scale, 0, 1.0f * scale);
        }

        const ImU32 line_col = ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        const float pad       = 7.0f * scale;
        const float thickness = 2.0f * scale;
        draw_list->AddLine(ImVec2(bb.Min.x + pad, bb.Min.y + pad), ImVec2(bb.Max.x - pad, bb.Max.y - pad), line_col, thickness);
        draw_list->AddLine(ImVec2(bb.Min.x + pad, bb.Max.y - pad), ImVec2(bb.Max.x - pad, bb.Min.y + pad), line_col, thickness);
        return pressed;
    }


    ImGuiID id = window->GetID("##simple_tool_window_close_button");
    bool hovered = false;
    bool held    = false;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_PressedOnClickRelease);

    ImDrawList* draw_list = window->DrawList;
    if (draw_list == nullptr)
        draw_list = ImGui::GetForegroundDrawList();
    if (hovered || held) {
        const ImU32 hover_col = ImGui::GetColorU32(ImVec4(0.082f, 0.749f, 0.349f, 1.0f));
        draw_list->AddRect(bb.Min, bb.Max, hover_col, 2.0f * scale, 0, 1.0f * scale);
    }

    const ImU32 line_col = ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    const float pad       = 7.0f * scale;
    const float thickness = 2.0f * scale;
    draw_list->AddLine(ImVec2(bb.Min.x + pad, bb.Min.y + pad), ImVec2(bb.Max.x - pad, bb.Max.y - pad), line_col, thickness);
    draw_list->AddLine(ImVec2(bb.Min.x + pad, bb.Max.y - pad), ImVec2(bb.Max.x - pad, bb.Min.y + pad), line_col, thickness);

    return pressed;
}

static bool render_simple_tool_window_close_button(const std::string& window_name)
{
    return render_simple_tool_window_close_button(ImGui::FindWindowByName(window_name.c_str()));
}


static ImGuiWindow* find_simple_tool_window_by_id(ImGuiID window_id)
{
    if (window_id == 0)
        return nullptr;

    ImGuiContext* context = ImGui::GetCurrentContext();
    if (context == nullptr)
        return nullptr;

    for (int i = context->Windows.Size - 1; i >= 0; --i) {
        ImGuiWindow* window = context->Windows[i];
        if (window == nullptr || window->ID != window_id || !window->WasActive || window->Hidden)
            continue;
        if ((window->Flags & (ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_Popup | ImGuiWindowFlags_Modal)) != 0)
            continue;
        if (window->Size.x < 120.0f || window->Size.y < 60.0f)
            continue;
        return window;
    }

    return nullptr;
}

static bool is_simple_tool_window(ImGuiWindow* window)
{
    if (window == nullptr || !window->WasActive || window->Hidden)
        return false;
    if ((window->Flags & (ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_Popup | ImGuiWindowFlags_Modal)) != 0)
        return false;
    if (window->Size.x < 120.0f || window->Size.y < 60.0f)
        return false;
    return true;
}

static ImGuiWindow* find_topmost_simple_tool_window()
{
    ImGuiContext* context = ImGui::GetCurrentContext();
    if (context == nullptr)
        return nullptr;

    for (int i = context->Windows.Size - 1; i >= 0; --i) {
        ImGuiWindow* window = context->Windows[i];
        if (is_simple_tool_window(window))
            return window;
    }

    return nullptr;
}
static bool render_simple_tool_window_close_button_near(float x, float y, ImGuiID* tracked_window_id = nullptr)
{
    ImGuiContext* context = ImGui::GetCurrentContext();
    if (context == nullptr)
        return false;

    if (tracked_window_id != nullptr) {
        if (ImGuiWindow* tracked_window = find_simple_tool_window_by_id(*tracked_window_id))
            return render_simple_tool_window_close_button(tracked_window);
        *tracked_window_id = 0;
    }

    for (int i = context->Windows.Size - 1; i >= 0; --i) {
        ImGuiWindow* window = context->Windows[i];
        // only filtered by position. Tooltip windows can be near the popup anchor and are too small for tool panels.
        if (!is_simple_tool_window(window))
            continue;
        if (std::abs(window->Pos.y - y) > 80.0f)
            continue;
        if (x < window->Pos.x - 80.0f || x > window->Pos.x + window->Size.x + 80.0f)
            continue;
        if (tracked_window_id != nullptr)
            *tracked_window_id = window->ID;
        return render_simple_tool_window_close_button(window);
    }

    if (ImGuiWindow* topmost_window = find_topmost_simple_tool_window()) {
        if (tracked_window_id != nullptr)
            *tracked_window_id = topmost_window->ID;
        return render_simple_tool_window_close_button(topmost_window);
    }

    return false;
}

static bool render_simple_tool_window_close_button_by_name_or_near(const std::string& window_name,
                                                                   float x,
                                                                   float y,
                                                                   ImGuiID* tracked_window_id)
{
    if (!window_name.empty()) {
        ImGuiWindow* named_window = ImGui::FindWindowByName(window_name.c_str());
        if (is_simple_tool_window(named_window)) {
            if (tracked_window_id != nullptr)
                *tracked_window_id = named_window->ID;
            return render_simple_tool_window_close_button(named_window);
        }
    }

    return render_simple_tool_window_close_button_near(x, y, tracked_window_id);
}

static void begin_simple_toolbar_close_after_click(const GLCanvas3D* canvas)
{
    s_simple_toolbar_waiting_left_up_by_canvas[canvas] = true;
    s_simple_toolbar_left_up_seen_by_canvas[canvas]    = false;
}

static bool keep_simple_toolbar_enabled_for_click_release(const GLCanvas3D* canvas, bool close_toolbar)
{
    bool& waiting_left_up = s_simple_toolbar_waiting_left_up_by_canvas[canvas];
    bool& left_up_seen    = s_simple_toolbar_left_up_seen_by_canvas[canvas];

    if (!close_toolbar) {
        waiting_left_up = false;
        left_up_seen    = false;
        return false;
    }

    if (!waiting_left_up)
        return false;

    if (wxGetMouseState().LeftIsDown()) {
        left_up_seen = false;
        return true;
    }

    if (!left_up_seen) {
        left_up_seen = true;
        return true;
    }

    waiting_left_up = false;
    return false;
}

struct GizmoPopupEstimate
{
    GLGizmoBase* gizmo     = nullptr;
    bool         has_popup = false;
    float        height_px = 0.0f;
};

static float calc_toolbar_row_gap_px(const GLToolbar::Layout& toolbar_layout)
{
    const float icon_px = toolbar_layout.icons_size * toolbar_layout.scale;
    float       gap_px  = toolbar_layout.gap_size * (toolbar_layout.gap_scale > 0.f ? toolbar_layout.gap_scale : 1.f) *
                   toolbar_layout.scale;
    return std::min(gap_px, icon_px * 0.20f);
}

static ObjManipToolbarMetrics calc_obj_manip_toolbar_metrics(GLToolbar& toolbar, float scale, float canvas_w, float canvas_h)
{
    ObjManipToolbarMetrics metrics;
    metrics.scale    = scale;
    metrics.canvas_w = canvas_w;
    metrics.canvas_h = canvas_h;

    metrics.toolbar_h      = toolbar.get_height_horizontal_simple();
    metrics.icons_space_px = std::min(toolbar.get_width(), canvas_w);
    metrics.anchor_gap_px  = 8.0f * scale;

    const GLToolbar::Layout toolbar_layout = toolbar.get_layout();
    metrics.icon_px                      = toolbar_layout.icons_size * toolbar_layout.scale;
    metrics.border_px                    = toolbar_layout.border * toolbar_layout.scale;
    metrics.row_gap_px                   = calc_toolbar_row_gap_px(toolbar_layout);

    metrics.max_left_span = std::max(0.0f, canvas_w - metrics.icons_space_px);
    metrics.max_top_span  = std::max(0.0f, canvas_h - metrics.toolbar_h);
    return metrics;
}

static GizmoPopupEstimate estimate_gizmo_popup(GLGizmosManager& gizmos, GLGizmosManager::EType current_gizmo, float scale)
{
    GizmoPopupEstimate estimate;

    if (current_gizmo == GLGizmosManager::EType::Move || current_gizmo == GLGizmosManager::EType::Rotate ||
        current_gizmo == GLGizmosManager::EType::Scale || current_gizmo == GLGizmosManager::EType::MmuSegmentation ||
        current_gizmo == GLGizmosManager::EType::Clone || current_gizmo == GLGizmosManager::EType::Cut ||
        current_gizmo == GLGizmosManager::EType::Simplify) {
        estimate.gizmo = gizmos.get_gizmo(current_gizmo);
        if (estimate.gizmo) {
            estimate.has_popup = true;
            estimate.height_px = 240.0f * scale;

            if (current_gizmo == GLGizmosManager::EType::Simplify) {
                // The Simplify gizmo manages its own (centered) window position; we only
                // need to make sure its input window gets rendered in easy/AI mode.
                estimate.height_px = 320.0f * scale;
            } else if (current_gizmo == GLGizmosManager::EType::Clone) {
                if (auto* clone_gizmo = dynamic_cast<GLGizmoClone*>(estimate.gizmo)) {
                    const float last_h = clone_gizmo->get_last_input_window_height();
                    estimate.height_px = (last_h > 1.0f) ? last_h : (120.0f * scale);
                } else {
                    estimate.height_px = 120.0f * scale;
                }
            } else if (current_gizmo == GLGizmosManager::EType::Cut) {
                if (auto* cut_gizmo = dynamic_cast<GLGizmoCut3D*>(estimate.gizmo)) {
                    const float last_h = cut_gizmo->get_last_input_window_height();
                    estimate.height_px = (last_h > 1.0f) ? last_h : (320.0f * scale);
                } else {
                    estimate.height_px = 320.0f * scale;
                }
            } else if (auto* manipul = wxGetApp().obj_manipul()) {
                const float last_h = manipul->get_last_input_window_height();
                if (last_h > 1.0f)
                    estimate.height_px = last_h;
            }
        }
    }

    return estimate;
}

static bool project_bbox_to_screen_rect_px(const Eigen::Matrix4d& pv_matrix,
                                          const BoundingBoxf3&    bbox,
                                          float                   canvas_w,
                                          float                   canvas_h,
                                          ObjManipToolbarScreenRectPx& out_rect)
{
    float min_x = std::numeric_limits<float>::max();
    float min_y = std::numeric_limits<float>::max();
    float max_x = -std::numeric_limits<float>::max();
    float max_y = -std::numeric_limits<float>::max();
    bool  projected = false;

    auto project_corner = [&](const Vec3d& corner) {
        Eigen::Vector4d clip = pv_matrix * Eigen::Vector4d(corner.x(), corner.y(), corner.z(), 1.0);
        if (clip.w() == 0.0)
            return;
        Eigen::Vector3d ndc = clip.head<3>() / clip.w();
        const float screen_x = float((ndc.x() + 1.0) * 0.5 * canvas_w);
        const float screen_y = float((1.0 - (ndc.y() + 1.0) * 0.5) * canvas_h);
        min_x = std::min(min_x, screen_x);
        max_x = std::max(max_x, screen_x);
        min_y = std::min(min_y, screen_y);
        max_y = std::max(max_y, screen_y);
        projected = true;
    };

    const Vec3d min_pt(bbox.min.x(), bbox.min.y(), bbox.min.z());
    const Vec3d max_pt(bbox.max.x(), bbox.max.y(), bbox.max.z());
    const std::array<Vec3d, 8> corners = {
        Vec3d(min_pt.x(), min_pt.y(), min_pt.z()),
        Vec3d(max_pt.x(), min_pt.y(), min_pt.z()),
        Vec3d(min_pt.x(), max_pt.y(), min_pt.z()),
        Vec3d(max_pt.x(), max_pt.y(), min_pt.z()),
        Vec3d(min_pt.x(), min_pt.y(), max_pt.z()),
        Vec3d(max_pt.x(), min_pt.y(), max_pt.z()),
        Vec3d(min_pt.x(), max_pt.y(), max_pt.z()),
        Vec3d(max_pt.x(), max_pt.y(), max_pt.z()),
    };

    for (const auto& corner : corners)
        project_corner(corner);

    if (!projected)
        return false;

    out_rect.min_x = min_x;
    out_rect.min_y = min_y;
    out_rect.max_x = max_x;
    out_rect.max_y = max_y;
    return true;
}

static ObjManipToolbarPlacement calc_obj_manip_toolbar_placement(const ObjManipToolbarMetrics&        metrics,
                                                                 const ObjManipToolbarScreenRectPx&  rect,
                                                                 bool                                more_tools_expanded,
                                                                 bool                                has_gizmo_popup,
                                                                 float                               popup_h_px,
                                                                 double                              camera_zoom)
{
    ObjManipToolbarPlacement placement;

    const float center_x = rect.center_x();
    const float center_y = rect.center_y();

    const bool prefer_above = (center_y >= kObjManipPreferAboveCenterRatio * metrics.canvas_h);

    const float extra_row_above_px = more_tools_expanded ? (metrics.row_gap_px + metrics.icon_px + metrics.border_px) : 0.0f;
    const float extra_row_below_px = more_tools_expanded ? (metrics.row_gap_px + metrics.toolbar_h) : 0.0f;

    float need_above_px = metrics.anchor_gap_px + metrics.toolbar_h + extra_row_above_px;
    float need_below_px = metrics.anchor_gap_px + metrics.toolbar_h + extra_row_below_px;
    if (has_gizmo_popup) {
        need_above_px += metrics.anchor_gap_px + popup_h_px;
        need_below_px += metrics.anchor_gap_px + popup_h_px;
    }

    const float space_above_px = std::max(0.0f, rect.min_y);
    const float space_below_px = std::max(0.0f, metrics.canvas_h - rect.max_y);
    const bool  can_place_above = (space_above_px >= need_above_px);
    const bool  can_place_below = (space_below_px >= need_below_px);

    const float need_side_px = metrics.icons_space_px + metrics.anchor_gap_px;
    const float space_left_px = std::max(0.0f, rect.min_x);
    const float space_right_px = std::max(0.0f, metrics.canvas_w - rect.max_x);
    const bool  can_place_left = (space_left_px >= need_side_px);
    const bool  can_place_right = (space_right_px >= need_side_px);

    const double side_anchor_zoom_threshold = kObjManipSideAnchorZoomThresholdFactor * metrics.scale;
    const bool   allow_side_anchor_by_zoom  = (camera_zoom >= side_anchor_zoom_threshold);

    const float bbox_vis_min_x = std::clamp(rect.min_x, 0.0f, metrics.canvas_w);
    const float bbox_vis_max_x = std::clamp(rect.max_x, 0.0f, metrics.canvas_w);
    const float bbox_vis_min_y = std::clamp(rect.min_y, 0.0f, metrics.canvas_h);
    const float bbox_vis_max_y = std::clamp(rect.max_y, 0.0f, metrics.canvas_h);
    const float bbox_w_px      = std::max(0.0f, bbox_vis_max_x - bbox_vis_min_x);
    const float bbox_h_px      = std::max(0.0f, bbox_vis_max_y - bbox_vis_min_y);
    const float bbox_max_ratio = std::max(bbox_w_px / metrics.canvas_w, bbox_h_px / metrics.canvas_h);
    const bool  allow_side_anchor_by_bbox = (bbox_max_ratio >= kObjManipSideAnchorBboxMaxRatioThreshold);
    const bool  allow_side_anchor = (allow_side_anchor_by_zoom || allow_side_anchor_by_bbox);

    placement.anchor = ObjManipToolbarAnchor::Default;
    if (allow_side_anchor && (can_place_left || can_place_right)) {
        if (can_place_right && (!can_place_left || (space_right_px >= space_left_px)))
            placement.anchor = ObjManipToolbarAnchor::Right;
        else
            placement.anchor = ObjManipToolbarAnchor::Left;
    } else {
        if (prefer_above) {
            if (can_place_above) placement.anchor = ObjManipToolbarAnchor::Above;
            else if (can_place_below) placement.anchor = ObjManipToolbarAnchor::Below;
        } else {
            if (can_place_below) placement.anchor = ObjManipToolbarAnchor::Below;
            else if (can_place_above) placement.anchor = ObjManipToolbarAnchor::Above;
        }
    }

    if (placement.anchor == ObjManipToolbarAnchor::Default)
        placement.anchor = prefer_above ? ObjManipToolbarAnchor::Above : ObjManipToolbarAnchor::Below;

    placement.expand_up = (placement.anchor == ObjManipToolbarAnchor::Above) ||
                          ((placement.anchor == ObjManipToolbarAnchor::Left || placement.anchor == ObjManipToolbarAnchor::Right) && prefer_above);

    if (placement.anchor == ObjManipToolbarAnchor::Above) {
        placement.toolbar_left_px = std::clamp(center_x - 0.5f * metrics.icons_space_px, 0.0f, metrics.max_left_span);
        placement.toolbar_top_px  = std::clamp(rect.min_y - metrics.anchor_gap_px - metrics.toolbar_h, 0.0f, metrics.max_top_span);
    } else if (placement.anchor == ObjManipToolbarAnchor::Below) {
        placement.toolbar_left_px = std::clamp(center_x - 0.5f * metrics.icons_space_px, 0.0f, metrics.max_left_span);
        placement.toolbar_top_px  = std::clamp(rect.max_y + metrics.anchor_gap_px, 0.0f, metrics.max_top_span);
    } else if (placement.anchor == ObjManipToolbarAnchor::Left) {
        placement.toolbar_left_px = std::clamp(rect.min_x - metrics.anchor_gap_px - metrics.icons_space_px, 0.0f, metrics.max_left_span);
        placement.toolbar_top_px  = std::clamp(center_y - 0.5f * metrics.toolbar_h, 0.0f, metrics.max_top_span);
    } else if (placement.anchor == ObjManipToolbarAnchor::Right) {
        placement.toolbar_left_px = std::clamp(rect.max_x + metrics.anchor_gap_px, 0.0f, metrics.max_left_span);
        placement.toolbar_top_px  = std::clamp(center_y - 0.5f * metrics.toolbar_h, 0.0f, metrics.max_top_span);
    }

    placement.popup_anchor_above =
        (placement.anchor == ObjManipToolbarAnchor::Above) ||
        ((placement.anchor == ObjManipToolbarAnchor::Left || placement.anchor == ObjManipToolbarAnchor::Right) && placement.expand_up);

    return placement;
}

static float calc_gizmo_popup_panel_y_px(bool                               popup_anchor_above,
                                        bool                               more_tools_expanded,
                                        decltype(GLToolbar::Layout::VO_Top) toolbar_orientation,
                                        float                              toolbar_top_px,
                                        float                              toolbar_h,
                                        float                              row_gap_px,
                                        float                              icon_px,
                                        float                              border_px,
                                        float                              anchor_gap_px,
                                        float                              popup_h_px,
                                        float                              canvas_h)
{
    const float max_top = std::max(0.0f, canvas_h - popup_h_px);

    if (popup_anchor_above) {
        float toolbar_topmost_y = toolbar_top_px;
        if (more_tools_expanded && toolbar_orientation == GLToolbar::Layout::VO_Top)
            toolbar_topmost_y = toolbar_top_px - row_gap_px - icon_px - border_px;

        const float desired_top = toolbar_topmost_y - anchor_gap_px - popup_h_px;
        return std::clamp(desired_top, 0.0f, max_top);
    }

    float toolbar_total_h = toolbar_h;
    if (more_tools_expanded && toolbar_orientation == GLToolbar::Layout::VO_Bottom)
        toolbar_total_h = toolbar_h + row_gap_px + toolbar_h;

    return std::clamp((toolbar_top_px + toolbar_total_h) + anchor_gap_px, 0.0f, max_top);
}

} // namespace

bool GLCanvas3D::_init_object_manipulate_toolbar_simple()
{
    m_object_manipulate_toolbar.set_enabled(true);
    
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

    BackgroundTexture::Metadata md;
    md.filename = m_is_dark ? "toolbar_background_dark.png" : "toolbar_background.png";
    md.left = md.top = md.right = md.bottom = 8;
    md.icon_borders.push_back(!m_is_dark ? "toolbar_border_disable_dark.svg" : "toolbar_border_disable.svg");
    md.icon_borders.push_back("toolbar_border_hover.svg");
    md.icon_borders.push_back("toolbar_border_press.svg");
    md.icon_borders.push_back(!m_is_dark ? "toolbar_border_normal_dark.svg" : "toolbar_border_normal.svg");

    if (!m_object_manipulate_toolbar.init(background_data)) {
        // unable to init the toolbar texture, disable it
        m_object_manipulate_toolbar.set_enabled(false);
        return true;
    }
    // init arrow
    if (!m_object_manipulate_toolbar.init_arrow("toolbar_arrow.svg"))
        BOOST_LOG_TRIVIAL(error) << "Main toolbar failed to load arrow texture.";

    // m_gizmos is created at constructor, thus we can init arrow here.
    if (!m_gizmos.init_arrow("toolbar_arrow.svg"))
        BOOST_LOG_TRIVIAL(error) << "Gizmos manager failed to load arrow texture.";


    m_object_manipulate_toolbar.set_horizontal_expand(true);
    m_object_manipulate_toolbar.set_layout_type(GLToolbar::Layout::Horizontal);
    // BBS: main toolbar is at the top and left, we don't need the rounded-corner effect at the right side and the top side
    m_object_manipulate_toolbar.set_horizontal_orientation(GLToolbar::Layout::HO_Center);
    m_object_manipulate_toolbar.set_vertical_orientation(GLToolbar::Layout::VO_Top);
    m_object_manipulate_toolbar.set_border(6.0f);
    m_object_manipulate_toolbar.set_separator_size(1.0f);
    m_object_manipulate_toolbar.set_gap_size(3.0f);
    m_object_manipulate_toolbar.del_all_item();
    m_object_manipulate_toolbar.set_icons_size(70.0f);
    m_object_manipulate_toolbar.set_layout_gap_scale(0.35f);
    m_object_manipulate_toolbar.set_layout_step_scale(0.35f);

    // 你可以统一一个 margin 和预估尺寸（也可在各弹层内动态测量）
    const float ui_scale      = get_scale();
    const float popup_gap_px  = 8.0f * ui_scale;      // 工具栏底部与弹窗的垂直间距
    const float popup_h_px    = 320.0f * ui_scale;    // 预估/固定高度

    GLToolbarItem::Data item;
    auto close_object_toolbar_for_popup = [this]() {
        m_more_tools_expanded = false;
        m_object_manipulate_toolbar.reset_item_state_simple();
        begin_simple_toolbar_close_after_click(this);
    };

    auto open_gizmo_and_close_toolbar = [this, close_object_toolbar_for_popup](GLGizmosManager::EType type) {
        close_object_toolbar_for_popup();
        s_simple_arrange_menu_open_by_canvas[this] = false;
        s_simple_arrange_menu_pos_initialized_by_canvas[this] = false;
        s_simple_orient_menu_open_by_canvas[this] = false;
        s_simple_orient_menu_pos_initialized_by_canvas[this] = false;
        m_gizmos.open_gizmo(type);
        set_as_dirty();
    };

    // move gizmo
    {
        GLGizmoBase* obj = m_gizmos.get_gizmo(GLGizmosManager::EType::Move);
        if (obj) {
            item.name          = obj->get_name(false);
            item.label_below_icon = true;
            item.icon_label_split = 0.62f;
            item.icon_filename = obj->get_icon_filename();
            item.tooltip       = obj->get_name(true);
            item.sprite_id++;
            // item.left.action_callback = [this]() { m_gizmos.open_gizmo(GLGizmosManager::EType::Move); };
            item.left.action_callback = [open_gizmo_and_close_toolbar]() { open_gizmo_and_close_toolbar(GLGizmosManager::EType::Move); };
            item.enabling_callback    = [obj]() -> bool { return obj->is_activable(); };
            if (!m_object_manipulate_toolbar.add_item(item, GLToolbarItem::EType::ActionWithText))
                return false;
        }
    }

    // rotate gizmo
    {
        GLGizmoBase* obj = m_gizmos.get_gizmo(GLGizmosManager::EType::Rotate);
        if (obj) {
            item.name          = obj->get_name(false);
            item.label_below_icon = true;
            item.icon_label_split = 0.62f;
            item.icon_filename = obj->get_icon_filename();
            item.tooltip       = obj->get_name(true);
            item.sprite_id++;
            // item.left.action_callback = [this]() { m_gizmos.open_gizmo(GLGizmosManager::EType::Rotate); };
            item.left.action_callback = [open_gizmo_and_close_toolbar]() { open_gizmo_and_close_toolbar(GLGizmosManager::EType::Rotate); };
            item.enabling_callback    = [obj]() -> bool { return obj->is_activable(); };
            if (!m_object_manipulate_toolbar.add_item(item, GLToolbarItem::EType::ActionWithText))
                return false;
        }
    }

    // Scale
    {
        GLGizmoBase* obj = m_gizmos.get_gizmo(GLGizmosManager::EType::Scale);
        if (obj) {
            item.name          = obj->get_name(false);
            item.label_below_icon = true;
            item.icon_label_split = 0.62f;
            item.icon_filename = obj->get_icon_filename();
            item.tooltip       = obj->get_name(true);
            item.sprite_id++;
            // item.left.action_callback = [this]() { m_gizmos.open_gizmo(GLGizmosManager::EType::Scale); };
            item.left.action_callback = [open_gizmo_and_close_toolbar]() { open_gizmo_and_close_toolbar(GLGizmosManager::EType::Scale); };
            item.enabling_callback    = [obj]() -> bool { return obj->is_activable(); };
            if (!m_object_manipulate_toolbar.add_item(item, GLToolbarItem::EType::ActionWithText))
                return false;
        }
    }

    /*
    // Color Paint
    {
        GLGizmoBase* obj = m_gizmos.get_gizmo(GLGizmosManager::EType::MmuSegmentation);
        if (obj) {
            item.name          = obj->get_name(false);
            item.label_below_icon = true;
            item.icon_label_split = 0.62f;
            item.icon_filename = obj->get_icon_filename();
            item.tooltip       = obj->get_name(true);
            item.sprite_id++;

            item.enabling_callback    = [obj]() -> bool { return obj->is_activable(); };
            // item.left.action_callback = [this]() { m_gizmos.open_gizmo(GLGizmosManager::EType::MmuSegmentation); };
            item.left.action_callback = [open_gizmo_and_close_toolbar]() { open_gizmo_and_close_toolbar(GLGizmosManager::EType::MmuSegmentation); };
            item.visibility_callback = [obj]() -> bool { return true; };
            if (!m_object_manipulate_toolbar.add_item(item, GLToolbarItem::EType::ActionWithText))
                return false;
        }
    }
    */

    // More 按钮
    {
        item.name             = "more";
        item.label_below_icon = true;
        item.icon_label_split = 0.62f;
        item.icon_filename    = m_is_dark ? "toolbar_more_dark.svg" : "toolbar_more.svg";
        item.tooltip          = _utf8(L("More Tool"));
        item.sprite_id++;
        // 5; 
        item.enabling_callback    = []() -> bool { return true; };
        //more_item.left.toggable        = true;
        item.left.action_callback = [this]() {
            m_more_tools_expanded = !m_more_tools_expanded;
            set_as_dirty();
        };

        if (!m_object_manipulate_toolbar.add_item(item, GLToolbarItem::EType::ActionWithText))
            return false;
    }

    // Arrange all objects
    {
        item.name             = "arrange";
        item.label_below_icon = true;
        item.icon_label_split = 0.62f;
        item.icon_filename    = m_is_dark ? "toolbar_arrange_dark.svg" : "toolbar_arrange.svg";
        item.tooltip          = _utf8(L("Arrange all objects"));
        item.sprite_id++;
        item.left.action_callback = [this]() {
            m_gizmos.reset_all_states();
            m_object_manipulate_toolbar.reset_item_state_simple();
            // toolbar stayed active while arrange popup opened.
            // m_object_manipulate_toolbar.set_enabled(false);
            begin_simple_toolbar_close_after_click(this);
            const bool open = !s_simple_arrange_menu_open_by_canvas[this];
            s_simple_arrange_menu_open_by_canvas[this] = open;
            s_simple_orient_menu_open_by_canvas[this] = false;
            m_more_tools_expanded = false;
            if (open)
                s_simple_arrange_menu_pos_initialized_by_canvas[this] = false;
            set_as_dirty();
        };
        item.left.toggable        = false;
        item.left.render_callback = GLToolbarItem::Default_Render_Callback;
        item.enabling_callback   = []() -> bool { return wxGetApp().plater()->can_arrange(); };
        item.visibility_callback = [this]() -> bool { return m_more_tools_expanded; };
        if (!m_object_manipulate_toolbar.add_item(item, GLToolbarItem::EType::ActionWithText))
            return false;

    }

    // Cut
    {
        GLGizmoBase* obj = m_gizmos.get_gizmo(GLGizmosManager::EType::Cut);
        if (obj) {
            item.name          = obj->get_name(false);
            item.label_below_icon = true;
            item.icon_label_split = 0.62f;
            item.icon_filename = obj->get_icon_filename();
            item.tooltip       = obj->get_name(true);
            item.sprite_id++;
            // item.left.action_callback = [this]() { m_gizmos.open_gizmo(GLGizmosManager::EType::Cut); };
            item.left.action_callback = [open_gizmo_and_close_toolbar]() { open_gizmo_and_close_toolbar(GLGizmosManager::EType::Cut); };
            item.enabling_callback    = [obj]() -> bool { return obj->is_activable(); };
            item.visibility_callback  = [this]() -> bool { return m_more_tools_expanded; };
            if (!m_object_manipulate_toolbar.add_item(item, GLToolbarItem::EType::ActionWithText))
                return false;
        }
    }
    

    // Orient Button — toggable: click to open mode-selection popup (GpuOrient)
    {
        item.name          = "orient";
        item.label_below_icon = true;
        item.icon_label_split = 0.62f;
        item.icon_filename = m_is_dark ? "toolbar_orient_dark.svg" : "toolbar_orient.svg";
        item.tooltip       = _utf8(L("Auto orient"));
        item.sprite_id++;
        // item.left.action_callback = []() {};
        // item.left.toggable        = true; // allow popup on click
        // item.left.render_callback = [this](float, float, float, float) {
        //     _render_orient_menu_simple();
        // };
        item.left.action_callback = [this]() {
            m_gizmos.reset_all_states();
            m_object_manipulate_toolbar.reset_item_state_simple();
            begin_simple_toolbar_close_after_click(this);
            s_simple_arrange_menu_open_by_canvas[this] = false;
            s_simple_arrange_menu_pos_initialized_by_canvas[this] = false;
            s_simple_orient_menu_open_by_canvas[this] = !s_simple_orient_menu_open_by_canvas[this];
            if (s_simple_orient_menu_open_by_canvas[this])
                s_simple_orient_menu_pos_initialized_by_canvas[this] = false;
            m_more_tools_expanded = false;
            set_as_dirty();
        };
        item.enabling_callback    = []() -> bool { return true; };
        item.visibility_callback  = [this]() -> bool { return m_more_tools_expanded; };
        item.left.toggable        = false;
        item.left.render_callback = GLToolbarItem::Default_Render_Callback;
        if (!m_object_manipulate_toolbar.add_item(item, GLToolbarItem::EType::ActionWithText))
            return false;
    }

    // Clone Button
    // {
    //     GLGizmoBase* obj = m_gizmos.get_gizmo(GLGizmosManager::EType::Clone);
    //     if(obj) {
    //         item.name             = "clone";
    //         item.label_below_icon = true;
    //         item.icon_label_split = 0.62f;
    //         item.icon_filename    = m_is_dark ? "toolbar_clone_dark.svg" : "toolbar_clone.svg";
    //         item.tooltip          = _utf8(L("Clone"));
    //         item.sprite_id++;
    //         //clone_item.left.toggable        = true;
    //         item.left.action_callback = [this]() {
    //             m_gizmos.open_gizmo(GLGizmosManager::EType::Clone); 
    //         };

    //         item.enabling_callback    = [obj]() -> bool { return obj->is_activable();};
    //         item.visibility_callback  = [this]() -> bool { return m_more_tools_expanded; };
    //         m_object_manipulate_toolbar.add_item(item, GLToolbarItem::EType::ActionWithText);
    //     }
    // }

    // Remove Button
    // {
    //     item.name             = "remove";
    //     item.label_below_icon = true;
    //     item.icon_label_split = 0.62f;
    //     item.icon_filename    = m_is_dark ? "toolbar_remove_dark.svg" : "toolbar_remove.svg";
    //     item.tooltip          = _utf8(L("Remove"));
    //     item.sprite_id++;
    //     // 7;
    //     item.left.action_callback = [this]() {
    //         if (wxGetApp().plater()) {
    //             wxGetApp().plater()->remove_selected();
    //         }
    //     };
    //     item.enabling_callback = []() -> bool { return true; };
    //     item.visibility_callback = [this]() -> bool { return m_more_tools_expanded; };
    //     m_object_manipulate_toolbar.add_item(item, GLToolbarItem::EType::ActionWithText);
    // }



    return true;
}

void GLCanvas3D::_render_object_manipulate_toolbar_simple()
{
    ObjManipToolbarAnchor& obj_toolbar_anchor = s_obj_manip_toolbar_anchor_by_canvas[this];

    // if (!m_object_manipulate_toolbar.is_enabled())
    if (m_object_manipulate_toolbar.get_items_count() == 0)
        return;

    if (m_canvas_type != ECanvasType::CanvasView3D)
        return;

    // Only show object-manipulation toolbar when something is selected.
    if (wxGetApp().plater()->is_selection_empty()) {
        m_more_tools_expanded = false;
        s_simple_arrange_menu_open_by_canvas[this] = false;
        s_simple_arrange_menu_pos_initialized_by_canvas[this] = false;
        s_simple_gizmo_popup_type_by_canvas.erase(this);
        s_simple_gizmo_popup_window_id_by_canvas.erase(this);
        s_simple_toolbar_waiting_left_up_by_canvas.erase(this);
        s_simple_toolbar_left_up_seen_by_canvas.erase(this);
        s_simple_orient_menu_open_by_canvas.erase(this);
        s_simple_orient_menu_pos_initialized_by_canvas.erase(this);
        obj_toolbar_anchor    = ObjManipToolbarAnchor::Default;
        return;
    }

    m_object_manipulate_toolbar.set_vertical_orientation(
        (obj_toolbar_anchor == ObjManipToolbarAnchor::Above) ? GLToolbar::Layout::VO_Top : GLToolbar::Layout::VO_Bottom);

    bool popup_anchor_above = (obj_toolbar_anchor == ObjManipToolbarAnchor::Above);

    if (m_mouse.drag.move_volume_idx != -1 && m_mouse.dragging) {
        m_more_tools_expanded = false;
        s_simple_arrange_menu_open_by_canvas[this] = false;
        s_simple_arrange_menu_pos_initialized_by_canvas[this] = false;
        s_simple_gizmo_popup_type_by_canvas.erase(this);
        s_simple_gizmo_popup_window_id_by_canvas.erase(this);
        s_simple_toolbar_waiting_left_up_by_canvas.erase(this);
        s_simple_toolbar_left_up_seen_by_canvas.erase(this);
        s_simple_orient_menu_open_by_canvas[this] = false;
        s_simple_orient_menu_pos_initialized_by_canvas[this] = false;
        return;
    }

    if (m_camera_movement) {
        m_more_tools_expanded = false;
        return;
    }

    // Refresh per-item visibility
    m_object_manipulate_toolbar.update_items_state_simple();

    const float scale    = get_scale();
    m_object_manipulate_toolbar.set_scale(scale);

    const Size  cnv_size = get_canvas_size();
    const float canvas_w = float(cnv_size.get_width());
    const float canvas_h = float(cnv_size.get_height());

    const ObjManipToolbarMetrics metrics = calc_obj_manip_toolbar_metrics(m_object_manipulate_toolbar, scale, canvas_w, canvas_h);

    const auto               current_gizmo = m_gizmos.get_current_type();
    if (current_gizmo != GLGizmosManager::EType::Undefined) {
        m_more_tools_expanded = false;
        s_simple_arrange_menu_open_by_canvas[this] = false;
        s_simple_arrange_menu_pos_initialized_by_canvas[this] = false;
        s_simple_orient_menu_open_by_canvas[this] = false;
        s_simple_orient_menu_pos_initialized_by_canvas[this] = false;
    } else {
        s_simple_gizmo_popup_type_by_canvas.erase(this);
        s_simple_gizmo_popup_window_id_by_canvas.erase(this);
    }

    const GizmoPopupEstimate popup         = estimate_gizmo_popup(m_gizmos, current_gizmo, scale);
    const bool               popup_near_toolbar = popup.has_popup;
    const bool               close_object_toolbar = m_preset_panel_open || popup.has_popup || s_simple_arrange_menu_open_by_canvas[this] ||
                                                    s_simple_orient_menu_open_by_canvas[this];
    const bool               keep_toolbar_for_click_release = keep_simple_toolbar_enabled_for_click_release(this, close_object_toolbar);

    if (close_object_toolbar)
        m_object_manipulate_toolbar.reset_item_state_simple();
    // m_object_manipulate_toolbar.set_enabled(!close_object_toolbar);
    m_object_manipulate_toolbar.set_enabled(!close_object_toolbar || keep_toolbar_for_click_release);

    float toolbar_left_px = std::clamp(0.5f * (canvas_w - metrics.icons_space_px), 0.0f, metrics.max_left_span);
    float toolbar_top_px  = std::clamp(0.5f * (canvas_h - metrics.toolbar_h), 0.0f, metrics.max_top_span);

    const auto& camera           = wxGetApp().plater()->get_camera();
    const BoundingBoxf3& bbox    = wxGetApp().plater()->get_selection().get_bounding_box();
    const Eigen::Matrix4d pv_matrix = camera.get_projection_matrix().matrix() * camera.get_view_matrix().matrix();

    ObjManipToolbarScreenRectPx rect;
    if (project_bbox_to_screen_rect_px(pv_matrix, bbox, canvas_w, canvas_h, rect)) {
        const ObjManipToolbarPlacement placement =
            calc_obj_manip_toolbar_placement(metrics, rect, m_more_tools_expanded, popup_near_toolbar, popup.height_px, camera.get_zoom());

        obj_toolbar_anchor = placement.anchor;
        m_object_manipulate_toolbar.set_vertical_orientation(placement.expand_up ? GLToolbar::Layout::VO_Top : GLToolbar::Layout::VO_Bottom);

        toolbar_left_px      = placement.toolbar_left_px;
        toolbar_top_px       = placement.toolbar_top_px;
        popup_anchor_above   = placement.popup_anchor_above;
    }

    m_object_manipulate_toolbar.set_scroll(0.0f);
    m_object_manipulate_toolbar.set_limit_width(metrics.icons_space_px);

    const float left_ndc = -0.5f * canvas_w + toolbar_left_px;
    const float top_ndc  = 0.5f * canvas_h - toolbar_top_px;

    m_object_manipulate_toolbar.set_position(top_ndc, left_ndc);
    // m_object_manipulate_toolbar.render(*this, m_object_manipulate_toolbar.get_scroll());
    if (!close_object_toolbar)
        m_object_manipulate_toolbar.render(*this, m_object_manipulate_toolbar.get_scroll());

    if (s_simple_arrange_menu_open_by_canvas[this]) {
        ImGuiWrapper* imgui = wxGetApp().imgui();
        // Popup placement previously used the toolbar item and expanded-row geometry.
        const float popup_h_px = 120.0f * scale;
        const float x = std::clamp(toolbar_left_px, get_easy_mode_overlay_safe_left_px(), std::max(get_easy_mode_overlay_safe_left_px(), canvas_w - 360.0f * scale));
        const float y = std::clamp(toolbar_top_px, 0.0f, std::max(0.0f, canvas_h - popup_h_px));
        const bool force_pos = !s_simple_arrange_menu_pos_initialized_by_canvas[this];
        imgui->set_draggable_window_pos(x, y, ImGuiCond_Always, 0.0f, 0.0f, force_pos);
        s_simple_arrange_menu_pos_initialized_by_canvas[this] = true;
        _render_arrange_menu(0.0f, 0.0f, 0.0f, 0.0f);
        // if (render_simple_tool_window_close_button(into_u8(_L("Arrange options")))) {
        if (consume_simple_arrange_close_requested()) {
            s_simple_arrange_menu_open_by_canvas[this] = false;
            s_simple_arrange_menu_pos_initialized_by_canvas[this] = false;
        }
    }

    if (s_simple_orient_menu_open_by_canvas[this])
        _render_orient_menu_simple();

    // Render Move/Rotate/Scale (etc) popup near the toolbar
    if (popup.has_popup && popup.gizmo) {
        const float panel_x_param = toolbar_left_px;
        // panel_y previously used calc_gizmo_popup_panel_y_px(...), placing the popup below/above the toolbar.
        const float panel_y = std::clamp(toolbar_top_px, 0.0f, std::max(0.0f, canvas_h - popup.height_px));

        auto popup_type_it = s_simple_gizmo_popup_type_by_canvas.find(this);
        const bool force_popup_pos = popup_type_it == s_simple_gizmo_popup_type_by_canvas.end() ||
                                     popup_type_it->second != current_gizmo;
        if (force_popup_pos)
            s_simple_gizmo_popup_window_id_by_canvas[this] = 0;
        s_simple_gizmo_popup_type_by_canvas[this] = current_gizmo;

        popup.gizmo->render_input_window(panel_x_param, panel_y, canvas_h, force_popup_pos);
        // if (render_simple_tool_window_close_button_by_name_or_near(popup.gizmo->get_name(false),
        //                                                            panel_x_param,
        //                                                            panel_y,
        //                                                            &s_simple_gizmo_popup_window_id_by_canvas[this]))
        //     m_gizmos.reset_all_states();
    }

    // if (m_toolbar_highlighter.m_render_arrow)
    if (!close_object_toolbar && m_toolbar_highlighter.m_render_arrow)
        m_object_manipulate_toolbar.render_arrow(*this, m_toolbar_highlighter.m_toolbar_item);
}

bool GLCanvas3D::_render_orient_menu_simple()
{
    ImGuiWrapper* imgui = wxGetApp().imgui();

    // Position logic previously used toolbar_bottom_y + anchor_gap_px, placing the popup below the toolbar.
    const float scale     = get_scale();
    const Size  cnv_size  = get_canvas_size();
    const float canvas_w  = float(cnv_size.get_width());
    const float canvas_h  = float(cnv_size.get_height());
    const GLToolbar::Layout obj_toolbar_layout = m_object_manipulate_toolbar.get_layout();
    const float toolbar_left_px = obj_toolbar_layout.left + 0.5f * canvas_w;
    const float toolbar_top_px  = 0.5f * canvas_h - obj_toolbar_layout.top;
    const float panel_x_param   = std::clamp(toolbar_left_px, get_easy_mode_overlay_safe_left_px(), std::max(get_easy_mode_overlay_safe_left_px(), canvas_w - 360.0f * scale));
    const float panel_y         = std::clamp(toolbar_top_px, 0.0f, std::max(0.0f, canvas_h - 320.0f * scale));


    ImGuiWrapper::push_toolbar_style(get_scale());
    // imgui->set_draggable_window_pos(panel_x_param, panel_y, ImGuiCond_Always, 0.0f, 0.0f, true);
    const bool force_pos = !s_simple_orient_menu_pos_initialized_by_canvas[this];
    imgui->set_draggable_window_pos(panel_x_param, panel_y, ImGuiCond_Always, 0.0f, 0.0f, force_pos);
    s_simple_orient_menu_pos_initialized_by_canvas[this] = true;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, (30.0f * get_scale() - ImGui::GetFontSize()) / 2.0)); // use for titlebar
    imgui->begin_with_drag(_L("Auto Orientation options"),
                           /*ImGuiWindowFlags_NoMove |*/ ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
    if (render_simple_tool_window_close_button(ImGui::GetCurrentWindow())) {
        s_simple_orient_menu_open_by_canvas[this] = false;
        s_simple_orient_menu_pos_initialized_by_canvas[this] = false;
    }
    ImGui::PopStyleVar(1);

    OrientSettings  settings     = get_orient_settings();
    OrientSettings& settings_out = get_orient_settings();

    auto&             appcfg = wxGetApp().app_config;
    PrinterTechnology ptech  = current_printer_technology();

    bool        settings_changed = false;
    float       angle_min        = 45.f;
    std::string angle_key = "overhang_angle", rot_key = "enable_rotation";
    std::string key_min_area   = "min_area";
    std::string key_min_volume = "min_volume";
    std::string key_min_time   = "min_time";
    std::string postfix        = "_fff";

    if (ptech == ptSLA) {
        angle_min = 45.f;
        postfix   = "_sla";
    }

    angle_key += postfix;
    rot_key += postfix;

    ImGuiWrapper::push_radio_style();
    if (imgui->radio_button(_L("Default Orient"), settings.min_area)) {
        settings.min_area     = true;
        settings_out.min_area = settings.min_area;
        settings_out.min_time = settings_out.min_volume = false;
        appcfg->set("orient", key_min_area, settings_out.min_area ? "1" : "0");
        settings_changed = true;
    }

    if (imgui->radio_button(_L("Minimize support volume"), settings.min_volume)) {
        settings.min_volume     = true;
        settings_out.min_volume = settings.min_volume;
        settings_out.min_time = settings_out.min_area = false;
        appcfg->set("orient", key_min_volume, settings.min_volume ? "1" : "0");
        settings_changed = true;
    }

    if (imgui->radio_button(_L("Minimize print time"), settings.min_time)) {
        settings.min_time       = true;
        settings_out.min_time   = settings.min_time;
        settings_out.min_volume = settings_out.min_area = false;
        appcfg->set("orient", key_min_time, settings_out.min_time ? "1" : "0");
        settings_changed = true;
    }
    ImGuiWrapper::pop_radio_style();
    ImGui::Separator();

    if (imgui->button(_L("Orient"))) {
        // GPU-accelerated orient with CPU fallback
        auto* plater = wxGetApp().plater();
        if (plater) {
            auto* canvas = plater->canvas3D();
            if (canvas) {
                const OrientSettings& s = canvas->get_orient_settings();
                orientation::EOrientType otype = orientation::MinArea;
                if (s.min_volume) otype = orientation::MinVolume;
                else if (s.min_time) otype = orientation::MinTime;

                orientation::OrientParams op_params;
                if (otype == orientation::MinArea) {
                    orientation::OrientParamsArea params_area;
                    std::memcpy(&op_params, &params_area, sizeof(op_params));
                }
                op_params.orient_type = otype;

                static orientation::GpuOrient s_gpu_orienter;
                if (s_gpu_orienter.available()) {
                    Model& model = plater->model();
                    orientation::OrientMeshs items, excludes;
                    size_t pcnt = 0;
                    for (auto* obj : model.objects)
                        for (auto* mi : obj->instances)
                            if (mi && mi->printable) ++pcnt;
                    items.reserve(pcnt);
                    for (auto* obj : model.objects) {
                        if (!obj) continue;
                        for (auto* mi : obj->instances) {
                            if (!mi || !mi->printable) continue;
                            orientation::OrientMesh om;
                            om.name = obj->name;
                            om.mesh = obj->mesh();
                            if (obj->config.has("support_threshold_angle"))
                                om.overhang_angle = obj->config.opt_int("support_threshold_angle");
                            else {
                                const DynamicPrintConfig& fc = wxGetApp().preset_bundle->full_config();
                                om.overhang_angle = fc.opt_int("support_threshold_angle");
                            }
                            om.setter = [mi](const orientation::OrientMesh& p) {
                                mi->rotate(p.rotation_matrix);
                                mi->get_object()->invalidate_bounding_box();
                                mi->get_object()->ensure_on_bed();
                            };
                            items.emplace_back(std::move(om));
                        }
                    }
                    if (!items.empty()) {
                        plater->take_snapshot(_u8L("Orient"));
                        std::string error;
                        if (s_gpu_orienter.orient(items, excludes, op_params, true, &error)) {
                            for (auto& mesh : items) mesh.apply();
                            plater->update();
                            BOOST_LOG_TRIVIAL(info)
                                << "SimpleOrient: mode="
                                << (otype == orientation::MinVolume ? "min_volume" :
                                    otype == orientation::MinTime   ? "min_time"   : "min_area")
                                << " items=" << items.size()
                                << (error.empty() ? "" : (" warn=" + error));
                        }
                    }
                } else {
                    plater->set_prepare_state(Job::PREPARE_STATE_DEFAULT);
                    plater->orient();
                }
            } else {
                plater->set_prepare_state(Job::PREPARE_STATE_DEFAULT);
                plater->orient();
            }
            // Close the popup by deactivating the toolbar toggle button.
            _deactivate_orient_menu();
        }
    }

    ImGui::SameLine();

    if (imgui->button(_L("Reset"))) {
        settings_out                = OrientSettings{};
        settings_out.overhang_angle = 60.f;
        appcfg->set("orient", angle_key, std::to_string(settings_out.overhang_angle));
        appcfg->set("orient", rot_key, settings_out.enable_rotation ? "1" : "0");
        appcfg->set("orient", key_min_area, "1");
        appcfg->set("orient", key_min_volume, "0");
        appcfg->set("orient", key_min_time, "0");
        settings_changed = true;
    }

    imgui->end();
    ImGuiWrapper::pop_toolbar_style();
    return settings_changed;
}

}
}// namespace Slic3r::GUI
