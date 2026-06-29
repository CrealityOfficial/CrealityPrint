#include "libslic3r/libslic3r.h"
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
#include "slic3r/GUI/Gizmos/GLGizmoPainterBase.hpp"
#include "slic3r/GUI/Gizmos/GLGizmoEmboss.hpp"
#include "slic3r/Utils/UndoRedo.hpp"
#include "slic3r/Utils/MacDarkMode.hpp"
#include "slic3r/Config/DispConfig.h"
#include <slic3r/GUI/GUI_Utils.hpp>
#include "slic3r/GUI/Widgets/SideButton.hpp"

#include "filamentMapping/ImGuiFilamentPanel.hpp"

#if ENABLE_RETINA_GL
#include "slic3r/Utils/RetinaHelper.hpp"
#endif

#include <GL/glew.h>

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

namespace Slic3r
{
namespace GUI
{

void GLCanvas3D::_draw_filament_menu_contents()
{
    if (!m_imgui_filament_panel)
        return;

    const float scale = get_scale();
    ImGui::SetCursorPos(ImVec2(12.0f * scale, 12.0f * scale));

    const DM::Device& cur_dev = DM::DataCenter::Ins().get_current_device_data();

    // Keep mode rules in ImGuiFilamentPanel as the single source of truth.
    const auto avail = m_imgui_filament_panel->mode_availability_from_device(cur_dev);
    const bool show_cfs = avail.show_cfs;
    const bool show_ext = avail.show_external;

    // Resolve current mode against the device (auto-fallback when a mode is not available).
    {
        const auto resolved = m_imgui_filament_panel->resolve_mode_for_device(cur_dev, m_imgui_filament_panel->mode());
        if (resolved != m_imgui_filament_panel->mode())
            m_imgui_filament_panel->set_mode(resolved);
    }

    const ImGuiFilamentPanel::Mode prev_mode = m_imgui_filament_panel->mode();

    // Adjust text/check colors per theme so dark mode stays readable.
    const bool dark = wxGetApp().dark_mode();
    const ImVec4 text_col  = dark ? ImVec4(0.90f, 0.93f, 0.97f, 1.0f) : ImVec4(0.18f, 0.21f, 0.24f, 1.0f);
    const ImVec4 check_col = dark ? ImVec4(0.18f, 0.80f, 0.38f, 1.0f) : ImVec4(0.08f, 0.65f, 0.25f, 1.0f);
    const ImU32 col_on  = ImGui::GetColorU32(check_col);
    const ImU32 col_off = ImGui::GetColorU32(dark ? ImVec4(0.82f, 0.84f, 0.88f, 1.0f) : ImVec4(0.38f, 0.41f, 0.45f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, text_col);
    ImGui::PushStyleColor(ImGuiCol_CheckMark, check_col);

    //// The panel background is white, so enforce dark text for readability in this scope.
    //ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.18f, 0.21f, 0.24f, 1.0f));   // near #30373D
    //ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.08f, 0.65f, 0.25f, 1.0f)); // green check for radios

    // 顶部单选：启用CFS / 使用外置料架 + 打印校准(复选框)
    int  source_mode = (m_imgui_filament_panel->mode() == ImGuiFilamentPanel::Mode::CFS) ? 0 : 1; // 0=CFS, 1=EXT
    bool print_calibration = m_imgui_filament_panel->print_calibration_enabled();
    // Keep source_mode aligned with availability (for cases where both options are hidden/shown).
    if (source_mode == 0 && !show_cfs)
        source_mode = 1;
    if (source_mode == 1 && !show_ext)
        source_mode = 0;
    if (!show_cfs && !show_ext)
        source_mode = 1;
    // 自定义圆形单选样式：未选为圆环，选中为绿色圆环 + 实心小圆
    auto radio_circle = [&](const char* label, int value) -> bool {
        ImGui::PushID(label);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 start = ImGui::GetCursorScreenPos();
        const float r_outer = 7.0f * scale;
        const float ring_thickness = 1.5f;
        const ImU32 col_off = ImGui::GetColorU32(ImVec4(0.75f, 0.77f, 0.80f, 1.0f));
        const ImU32 col_on  = ImGui::GetColorU32(ImVec4(0.08f, 0.65f, 0.25f, 1.0f));
        ImVec2 center(start.x + r_outer, start.y + r_outer);
        ImVec2 text_sz = ImGui::CalcTextSize(label);
        const float text_gap = 8.0f * scale;
        ImRect bb(start, ImVec2(start.x + r_outer * 2 + text_gap + text_sz.x, start.y + r_outer * 2));
        ImGui::ItemSize(bb);
        bool hovered = false, held = false;
        bool pressed = ImGui::ButtonBehavior(bb, ImGui::GetID("##radio"), &hovered, &held);
        if (pressed) source_mode = value;
        const bool selected = (source_mode == value);
        dl->AddCircle(center, r_outer, selected ? col_on : col_off, 0, ring_thickness);
        if (selected) dl->AddCircleFilled(center, r_outer - 3.0f * scale, col_on);
        ImVec2 text_pos(start.x + r_outer * 2 + text_gap, start.y + r_outer - text_sz.y * 0.5f);
        dl->AddText(text_pos, ImGui::GetColorU32(ImGuiCol_Text), label);
        ImGui::PopID();
        return pressed;
    };

    bool mode_clicked = false;
    bool any_mode_radio = false;
    if (show_cfs) {
        mode_clicked |= radio_circle(_u8L("Enable CFS").c_str(), 0);
        any_mode_radio = true;
        if (show_ext)
            ImGui::SameLine(0.0f, 14.0f * scale); // 两个单选之间间距
    }
    if (show_ext) {
        mode_clicked |= radio_circle(_u8L("Use External Spool").c_str(), 1);
        any_mode_radio = true;
    }
    if (any_mode_radio)
        ImGui::SameLine(0.0f, 20.0f * scale); // 与“打印校准”之间的间距
    if (ImGui::Checkbox(_u8L("Print Calibration").c_str(), &print_calibration))
        m_imgui_filament_panel->set_print_calibration(print_calibration);

    if (source_mode == 0 && show_cfs) {
        ImGui::Dummy(ImVec2(0, 6 * scale));
        ImGui::TextUnformatted(_u8L("Select the CFS slot for each filament").c_str());
        ImGui::Dummy(ImVec2(0, 6 * scale));
    }

    if (source_mode == 1 && show_ext) {
        ImGui::Dummy(ImVec2(0, 6 * scale));
        ImGui::TextUnformatted(_u8L("It will be printed using the External Spool Holder").c_str());
        ImGui::Dummy(ImVec2(0, 6 * scale));
    }

    const ImGuiFilamentPanel::Mode desired_mode = (source_mode == 0) ? ImGuiFilamentPanel::Mode::CFS
                                                                     : ImGuiFilamentPanel::Mode::External;
    const ImGuiFilamentPanel::Mode new_mode = m_imgui_filament_panel->resolve_mode_for_device(cur_dev, desired_mode);
    if (new_mode != prev_mode)
        m_imgui_filament_panel->set_mode(new_mode);

    // When user switches CFS/External, trigger one auto-mapping immediately.
    if (mode_clicked && new_mode != prev_mode && cur_dev.valid)
        m_imgui_filament_panel->on_auto_mapping_filament_ex(cur_dev);


    m_imgui_filament_panel->check_device_filament_auto_mapping();

    m_imgui_filament_panel->Render();

    ImGui::PopStyleColor(2);

}


}
}// namespace Slic3r::GUI
