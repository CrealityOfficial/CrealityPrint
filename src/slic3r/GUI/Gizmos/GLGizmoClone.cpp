#include "GLGizmoClone.hpp"
#include "slic3r/GUI/GLCanvas3D.hpp"
#include "slic3r/GUI/GUI_App.hpp"
//BBS: GUI refactor
#include "slic3r/GUI/Plater.hpp"
#include "libslic3r/AppConfig.hpp"


#include <GL/glew.h>

#include <wx/utils.h>

namespace Slic3r {
namespace GUI {

//BBS: GUI refactor: add obj manipulation
GLGizmoClone::GLGizmoClone(GLCanvas3D& parent, const std::string& icon_filename, unsigned int sprite_id, GizmoObjectManipulation* obj_manipulation)
    : GLGizmoBase(parent, icon_filename, sprite_id)
    //BBS: GUI refactor: add obj manipulation
    , m_object_manipulation(obj_manipulation)
{}

std::string GLGizmoClone::get_tooltip() const
{
    const Selection& selection = m_parent.get_selection();
    bool show_position = selection.is_single_full_instance();
    const Vec3d& position = selection.get_bounding_box().center();

    return "";
}

bool GLGizmoClone::on_mouse(const wxMouseEvent &mouse_event) 
{
    //return use_grabbers(mouse_event);
    return false;
}

void GLGizmoClone::data_changed(bool is_serializing) 
{
    if (m_parent.get_selection().volumes_count() >= 1) {
        m_parent.get_selection().copy_to_clipboard();
        m_clone_num = 1;
        m_parent.get_selection().calculate_clone_preview_offsets(m_clone_num);
    }
}

bool GLGizmoClone::on_init()
{
    return true;
}

std::string GLGizmoClone::on_get_name() const
{
    return _u8L("Clone");
}

bool GLGizmoClone::on_is_activable() const
{
    return !m_parent.get_selection().is_empty();
}

void GLGizmoClone::on_render()
{
    m_parent.get_selection().render_clone_shells();
}

void GLGizmoClone::on_register_raycasters_for_picking()
{
    // the gizmo grabbers are rendered on top of the scene, so the raytraced picker should take it into account
    m_parent.set_raycaster_gizmos_on_top(true);
}

void GLGizmoClone::on_unregister_raycasters_for_picking()
{
    m_parent.set_raycaster_gizmos_on_top(false);
}


bool GLGizmoClone::_render_object_clone_panel()
{
    Selection& selection = m_parent.get_selection();

    ImGuiWrapper* imgui = wxGetApp().imgui();

    // BBS
    ImGuiWrapper::push_toolbar_style(m_parent.get_scale());

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, (30.0f * m_parent.get_scale() - ImGui::GetFontSize()) / 2.0)); // use for titlebar
    //imgui->begin(_L("Object Clone Options"), ImGuiWrapper::TOOLBAR_WINDOW_FLAGS);
    //ImGui::PopStyleVar(1);
    ImGuiWindowFlags clone = ImGuiWrapper::TOOLBAR_WINDOW_FLAGS;
    clone &= ~ImGuiWindowFlags_NoMove;
    imgui->begin_with_drag(_L("Object Clone Options"), clone);
    GizmoImguiRenderSimpleCloseButton();
    ImGui::PopStyleVar(1);

    const float slider_icon_width    = imgui->get_slider_icon_size().x;
    const float cursor_slider_left   = imgui->calc_text_size(_L("Number of copies:")).x + imgui->scaled(1.5f);
    const float minimal_slider_width = imgui->scaled(4.f);
    float       window_width         = minimal_slider_width + 2 * slider_icon_width;

    ImGui::AlignTextToFramePadding();
    imgui->text(_L("Number of copies:"));
    ImGui::SameLine(1.2 * cursor_slider_left);
    ImGui::PushItemWidth(window_width - slider_icon_width);
    // float before_dist = settings.distance;

    int v_min = 1, v_max = 99;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(238 / 255.0f, 238 / 255.0f, 238 / 255.0f, 0.00f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(238 / 255.0f, 238 / 255.0f, 238 / 255.0f, 0.00f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.81f, 0.81f, 0.81f, 1.0f));
    const ImVec4 COL_CREALITY = ImVec4{0.090f, 0.80f, 0.373, 1.0f};
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, COL_CREALITY);

    // bool slider_number_change = imgui->bbl_slider_int_style("##Number of copies:", &m_clone_settings.clone_num, 1, 100, "%d", true,
    // _L("Copies of the selected object"));
    // fix bug:[#8878], "##Number of copies" is the same with "draw_input_int_v2" would cause ui cover problem.
    bool slider_number_change = ImGui::BBLSliderScalar("##Number of copies1:", ImGuiDataType_S32, &m_clone_num, &v_min,
                                                       &v_max);

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(1);

    ImGui::SameLine(window_width - slider_icon_width + 1.3 * cursor_slider_left);
    ImGui::PushItemWidth(1.5 * slider_icon_width);
    // bool b_spacing_input = ImGui::BBLDragFloat("##spacing_input", &settings.distance, 0.05f, 0.0f, 0.0f, "%.2f");

    ImGui::SameLine(0.f);
    bool input_number_change = m_parent.draw_input_int_v2("##Number of copies2:", &m_clone_num, 1, &v_min, &v_max,
                                                 Vec2d(70.f * m_parent.get_scale(), 30.f));

    if (slider_number_change || input_number_change) {
        selection.calculate_clone_preview_offsets(m_clone_num);
    }

    ImGui::Separator();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(15.0f, 10.0f));

    if (imgui->button(_L("Confirm"))) {
        wxGetApp().plater()->clone_selection();
        selection.release_clone_preview_info();
        m_parent.exit_gizmo();
    }

    ImGui::SameLine();

    if (imgui->button(_L("Cancel"))) {
        selection.release_clone_preview_info();
        m_parent.exit_gizmo();
    }

    ImGui::PopStyleVar(1);
    m_last_input_window_height = ImGui::GetWindowHeight();
    imgui->end();

    // BBS
    ImGuiWrapper::pop_toolbar_style();

    return true;
}



//BBS: add input window for move
void GLGizmoClone::on_render_input_window(float x, float y, float bottom_limit, bool force_update_pos)
{
    //if (m_object_manipulation)
    //    m_object_manipulation->do_render_move_window(m_imgui, _u8L("Move"), x, y, bottom_limit, force_update_pos);

    //    case ERenderEvent::ObjectCloneOptions: {
    //    if (m_selection.volumes_count() >= 1) {
    //        m_extra_render_callbacks[index] = [this]() { this->_render_object_clone_options(0, 0, 100, 100); };
    //        m_selection.copy_to_clipboard();
    //        m_clone_settings.clone_num = 1;
    //        m_selection.calculate_clone_preview_offsets(m_clone_settings.clone_num);
    //        m_main_toolbar.on_set_virtual_item("virtual-clone-item");
    //    }
    //    break;
    //}

    if (ImGuiWrapper* imgui = wxGetApp().imgui())
        imgui->set_draggable_window_pos(x, y, ImGuiCond_Always, 0.f, 0.f, force_update_pos);

    _render_object_clone_panel();
}

} // namespace GUI
} // namespace Slic3r
