#include "GLGizmoMeshBoolean.hpp"
#include "slic3r/GUI/GLCanvas3D.hpp"
#include "slic3r/GUI/ImGuiWrapper.hpp"
#include "slic3r/GUI/GUI.hpp"
#include "libslic3r/MeshBoolean.hpp"
#include "slic3r/GUI/GUI_ObjectList.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/Camera.hpp"
#include "slic3r/GUI/NotificationManager.hpp"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui/imgui_internal.h>
#include "libslic3r/Model.hpp"
//#include "libslic3r/ModelWipeTower.hpp"
#include "libslic3r/ModelInstance.hpp"
#include "libslic3r/ModelObject.hpp"
#include "libslic3r/ModelVolume.hpp"
namespace Slic3r {
namespace GUI {

//static const std::string warning_text = _u8L("Unable to perform boolean operation on selected parts");
static const std::string warning_text = "Unable to perform boolean operation on selected parts";

GLGizmoMeshBoolean::GLGizmoMeshBoolean(GLCanvas3D& parent, const std::string& icon_filename, unsigned int sprite_id)
    : GLGizmoBase(parent, icon_filename, sprite_id)
{
}

GLGizmoMeshBoolean::~GLGizmoMeshBoolean() 
{
}

void GLGizmoMeshBoolean::set_src_volume(ModelVolume* mv)
{
    if (!mv)
    {
        m_src.reset();
        return;
    }

    if (mv == m_tool.mv) {
        //set_tool_volume(m_src.mv);
        return;
    }

    m_src.mv = mv;
    m_src.trafo = mv->get_matrix();
    m_src.volume_idx = -1;
    const auto& volumes = mv->get_object()->volumes;
    auto it = std::find(volumes.begin(), volumes.end(), mv);
    assert(it != volumes.end());
    if (it != volumes.end())
        m_src.volume_idx = std::distance(volumes.begin(), it);

    if (m_src.mv == m_tool.mv)
        m_tool.reset();

    m_selecting_state = MeshBooleanSelectingState::SelectTool;
}

void GLGizmoMeshBoolean::set_tool_volume(ModelVolume* mv)
{
    if (!mv)
    {
        m_tool.reset();
        return;
    }

    if (mv == m_src.mv)
    {
        //set_src_volume(m_tool.mv);
        return;
    }

    m_tool.mv = mv;
    m_tool.trafo = mv->get_matrix();
    m_tool.volume_idx = -1;
    const auto& volumes = mv->get_object()->volumes;
    auto it = std::find(volumes.begin(), volumes.end(), mv);
    assert(it != volumes.end());
    if (it != volumes.end())
        m_tool.volume_idx = std::distance(volumes.begin(), it);
}

bool GLGizmoMeshBoolean::is_selection_valid(const Selection& selection)
{
    int object_idx = selection.get_object_idx();
    int size = selection.get_volume_idxs().size();
    //selection.is_single_full_object()
    return size >= 1 && (m_current_obj_idx != -1 ? m_current_obj_idx == object_idx : true);
}

bool GLGizmoMeshBoolean::gizmo_event(SLAGizmoEventType action, const Vec2d& mouse_position, bool shift_down, bool alt_down, bool control_down) 
{
    if (action == SLAGizmoEventType::LeftDown) {
        const ModelObject* mo = m_c->selection_info()->model_object();
        if (mo == nullptr)
            return true;
        const ModelInstance* mi = mo->instances[m_parent.get_selection().get_instance_idx()];
        std::vector<Transform3d> trafo_matrices;
        for (const ModelVolume* mv : mo->volumes) {
            //if (mv->is_model_part()) { 
                trafo_matrices.emplace_back(mi->get_transformation().get_matrix() * mv->get_matrix()); 
            //}
        }

        const Camera& camera = wxGetApp().plater()->get_camera();
        Vec3f  normal = Vec3f::Zero();
        Vec3f  hit = Vec3f::Zero();
        size_t facet = 0;
        Vec3f  closest_hit = Vec3f::Zero();
        Vec3f  closest_normal = Vec3f::Zero();
        double closest_hit_squared_distance = std::numeric_limits<double>::max();
        int    closest_hit_mesh_id = -1;

        // Cast a ray on all meshes, pick the closest hit and save it for the respective mesh
        for (int mesh_id = 0; mesh_id < int(trafo_matrices.size()); ++mesh_id) {
            MeshRaycaster mesh_raycaster = MeshRaycaster(mo->volumes[mesh_id]->mesh_ptr());
            if (mesh_raycaster.unproject_on_mesh(mouse_position, trafo_matrices[mesh_id], camera, hit, normal,
                m_c->object_clipper()->get_clipping_plane(), &facet)) {
                // Is this hit the closest to the camera so far?
                double hit_squared_distance = (camera.get_position() - trafo_matrices[mesh_id] * hit.cast<double>()).squaredNorm();
                if (hit_squared_distance < closest_hit_squared_distance) {
                    closest_hit_squared_distance = hit_squared_distance;
                    closest_hit_mesh_id = mesh_id;
                    closest_hit = hit;
                    closest_normal = normal;
                }
            }
        }

        if (closest_hit == Vec3f::Zero() && closest_normal == Vec3f::Zero())
            return true;


        if (get_selecting_state() == MeshBooleanSelectingState::SelectTool) {
            set_tool_volume(mo->volumes[closest_hit_mesh_id]);
            return true;
        }

        if (get_selecting_state() == MeshBooleanSelectingState::SelectSource) {
            set_src_volume(mo->volumes[closest_hit_mesh_id]);
            return true;
        }
    }
    return true;
}

bool GLGizmoMeshBoolean::on_mouse(const wxMouseEvent &mouse_event)
{
    // wxCoord == int --> wx/types.h
    Vec2i32 mouse_coord(mouse_event.GetX(), mouse_event.GetY());
    Vec2d mouse_pos = mouse_coord.cast<double>();

    // when control is down we allow scene pan and rotation even when clicking
    // over some object
    bool control_down           = mouse_event.CmdDown();
    bool grabber_contains_mouse = (get_hover_id() != -1);
    if (mouse_event.LeftDown()) {
        if ((!control_down || grabber_contains_mouse) &&            
            gizmo_event(SLAGizmoEventType::LeftDown, mouse_pos, mouse_event.ShiftDown(), mouse_event.AltDown(), false))
            // the gizmo got the event and took some action, there is no need
            // to do anything more
            return true;
    }

    return false;
}

bool GLGizmoMeshBoolean::on_init()
{
    m_shortcut_key = WXK_CONTROL_B;
    return true;
}

std::string GLGizmoMeshBoolean::on_get_name() const
{
    return _u8L("Mesh Boolean");
}

void GLGizmoMeshBoolean::handle_object_list_changed(Selection& selection, bool& needUpdate)
{
    auto set_state = [=](EState state) {
        m_state = state;
        on_set_state();
    };

    needUpdate = false;
    if (selection.is_single_volume_or_modifier()) {
        const GLVolume* v = selection.get_first_volume();
        if (v == NULL) {
            set_state(EState::Off);
            return;
        }

        int id     = v->volume_idx();
        int obj_id = v->object_idx();
        if (obj_id != m_current_obj_idx) {
            needUpdate = true;
            set_state(EState::On);
        }

        // int curr = current_obj_idx;
        Model& model = wxGetApp().model();
        if (obj_id >= 0 && obj_id < model.objects.size()) {
            const ModelObject* mo = model.objects[obj_id];
            // m_c->selection_info()->model_object();
            if (get_selecting_state() == MeshBooleanSelectingState::SelectTool) {
                set_tool_volume(mo->volumes[id]);
            }

            if (get_selecting_state() == MeshBooleanSelectingState::SelectSource) {
                set_src_volume(mo->volumes[id]);
            }
        }

    } else if (selection.is_single_full_object()) {
        set_state(EState::On);
        needUpdate = true;
    } else {
        set_state(EState::Off);
        needUpdate = true;
    }
}

bool GLGizmoMeshBoolean::on_is_activable() const
{
    const Selection& selection = m_parent.get_selection();
    bool is_single_full_instance = selection.is_single_full_instance();
    int size = selection.get_volume_idxs().size();
    return (selection.is_single_full_instance() && selection.get_volume_idxs().size() > 1) || m_current_obj_idx >= 0;
}

void GLGizmoMeshBoolean::on_render()
{
    const Selection& selection = m_parent.get_selection();
    if (selection.get_object_idx() < 0)
        return;
    static ModelObject* last_mo = nullptr;
    ModelObject* curr_mo = selection.get_model()->objects[selection.get_object_idx()];
    if (last_mo != curr_mo) {
        last_mo = curr_mo;
        ModelObject* sobj        = m_src.mv ? m_src.mv->get_object() : NULL;
        bool         src_changed = sobj != curr_mo;
        if (m_src.mv && (m_src.mv->get_object() != curr_mo))
            m_src.reset();
        ModelObject* tobj         = m_tool.mv ? m_tool.mv->get_object() : NULL;
        bool         tool_changed = tobj != curr_mo;
        if (m_tool.mv && (m_tool.mv->get_object() != curr_mo))
            m_tool.reset();
        m_operation_mode = MeshBooleanOperation::Union;
        m_selecting_state = MeshBooleanSelectingState::SelectSource;
        return;
    }

    BoundingBoxf3 src_bb;
    BoundingBoxf3 tool_bb;
    const ModelObject* mo = m_c->selection_info()->model_object();
    const ModelInstance* mi = mo->instances[selection.get_instance_idx()];
    const Selection::IndicesList& idxs = selection.get_volume_idxs();

    Model& model = wxGetApp().model();
    int m_id = -1;
    for (int i = 0; i < model.objects.size(); ++i)
    {
        if (model.objects[i] == mo)
        {
            m_id = i;
            break;
        }
    }
    int sv_id = -1;
    int tv_id = -1;
    for (int i = 0; i < mo->volumes.size(); ++i) {
        if (mo->volumes[i] == m_tool.mv) {
            tv_id = i;
        }
        if (mo->volumes[i] == m_src.mv) {
            sv_id = i;
        }
    }
    int i_id = -1;
    for (int i = 0; i < mo->instances.size(); ++i) {
        if (mo->instances[i] == mi) {
            i_id = i;
            break;
        }
    }

    auto get_volume_box = [](const Selection& selection, int m_id, int v_id, int iid, BoundingBoxf3& box) {
        const GLVolume* volume = selection.get_volume(m_id, v_id, iid);
        if (volume)
            box = volume->transformed_convex_hull_bounding_box();
    };
    get_volume_box(selection, m_id, sv_id, i_id, src_bb);
    get_volume_box(selection, m_id, tv_id, i_id, tool_bb);

    ColorRGB src_color = { 1.0f, 1.0f, 1.0f };
    ColorRGB tool_color = {0.0f, 150.0f / 255.0f, 136.0f / 255.0f};
    m_parent.get_selection().render_bounding_box(src_bb, src_color, m_parent.get_scale());
    m_parent.get_selection().render_bounding_box(tool_bb, tool_color, m_parent.get_scale());
}

void GLGizmoMeshBoolean::on_set_state()
{
     if (m_state == EState::On) {
         m_src.reset();
         m_tool.reset();
         bool m_diff_delete_input = false;
         bool m_inter_delete_input = false;
         m_operation_mode = MeshBooleanOperation::Union;
         m_selecting_state = MeshBooleanSelectingState::SelectSource;
         m_current_obj_idx = m_parent.get_selection().get_object_idx();
     }
     else if (m_state == EState::Off) {
         m_src.reset();
         m_tool.reset();
         bool m_diff_delete_input = false;
         bool m_inter_delete_input = false;
         m_operation_mode = MeshBooleanOperation::Undef;
         m_selecting_state = MeshBooleanSelectingState::Undef;
         //wxGetApp().notification_manager()->close_plater_warning_notification(warning_text);
         m_show_warning = false;
         m_current_obj_idx = -1;
     }
}

CommonGizmosDataID GLGizmoMeshBoolean::on_get_requirements() const
{
    return CommonGizmosDataID(
        int(CommonGizmosDataID::SelectionInfo)
        | int(CommonGizmosDataID::InstancesHider)
        | int(CommonGizmosDataID::Raycaster)
        | int(CommonGizmosDataID::ObjectClipper));
}

void GLGizmoMeshBoolean::on_render_input_window(float x, float y, float bottom_limit)
{
    y = std::min(y, bottom_limit - ImGui::GetWindowHeight());

    static float last_y = 0.0f;
    static float last_w = 0.0f;

    const float currt_scale = m_parent.get_scale();
    ImGuiWrapper::push_toolbar_style(currt_scale);
    GizmoImguiSetNextWIndowPos(x, y, ImGuiCond_Always, 0.0f, 0.0f);
    GizmoImguiBegin(_u8L("Mesh Boolean"),ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    const int max_tab_length = 2 * ImGui::GetStyle().FramePadding.x + std::max(ImGui::CalcTextSize(_u8L("Union").c_str()).x,
        std::max(ImGui::CalcTextSize(_u8L("Difference").c_str()).x, ImGui::CalcTextSize(_u8L("Intersection").c_str()).x));
    const int max_cap_length = ImGui::GetStyle().WindowPadding.x + ImGui::GetStyle().ItemSpacing.x +
                               std::max({ImGui::CalcTextSize(_u8L("Source Volume").c_str()).x,
                                         ImGui::CalcTextSize(_u8L("Tool Volume").c_str()).x,
                                         ImGui::CalcTextSize(_u8L("Subtract from").c_str()).x,
                                         ImGui::CalcTextSize(_u8L("Subtract with").c_str()).x});

    const int select_btn_length = 2 * ImGui::GetStyle().FramePadding.x + std::max(ImGui::CalcTextSize(("1 " + _u8L("selected")).c_str()).x, ImGui::CalcTextSize(_u8L("Select").c_str()).x);

    auto selectable = [this](const std::string& label, wchar_t icon_id, bool selected, const ImVec2& size_arg) {
        
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext&     g          = *GImGui;
        const ImGuiStyle& style      = g.Style;
        const ImGuiID     id         = window->GetID(label.c_str());

        ImVec2 pos = window->DC.CursorPos;
        ImVec2 button_size = size_arg;

        bool is_dark = wxGetApp().dark_mode();
        
        const ImRect bb(pos, pos + button_size);
        ImGui::ItemSize(bb, style.FramePadding.y);
        if (!ImGui::ItemAdd(bb, id))
            return false;

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, 0);

        const float rounding = 5.0f;

        ImU32 fg, bg;
        if (selected || hovered) {
            fg = IM_COL32_WHITE;
            bg = ImGui::GetColorU32(ImGuiWrapper::COL_CREALITY);
        } else {
            if (is_dark) {
                fg = IM_COL32_WHITE;
                bg = ImGui::GetColorU32(ImGuiCol_WindowBg, 1.0f);
            } else {
                fg = IM_COL32_BLACK;
                bg = ImGui::GetColorU32(ImGuiCol_WindowBg, 1.0f);
            }
        }

        ImDrawFlags corners = ImDrawFlags_RoundCornersNone;
        if (label == _u8L("Union")) {
            corners = ImDrawFlags_RoundCornersLeft;
        } else if (label == _u8L("Intersection")) {
            corners = ImDrawFlags_RoundCornersRight;
        }
        window->DrawList->AddRectFilled(pos, pos + button_size, bg, rounding, corners);

        if (ImGui::IsItemVisible() && icon_id != 0) {
            ImVec2      button_pos = ImGui::GetItemRectMin();
            ImDrawList* draw_list  = ImGui::GetWindowDrawList();

            float y_offset = ImGui::GetStyle().FramePadding.y;
            {
                std::wstring icon     = icon_id + boost::nowide::widen("  ");
                ImVec2       text_size = ImGui::CalcTextSize(into_u8(icon).c_str());
                y_offset = button_size.y / 2.0 - text_size.y - 2;
                ImVec2 text_pos(button_pos.x + (button_size.x - text_size.x) * 0.5f, button_pos.y + y_offset);
                draw_list->AddText(text_pos, fg, into_u8(icon).c_str());
            }
            {
                const char* line      = label.data();
                ImVec2      text_size = ImGui::CalcTextSize(line);
                y_offset = button_size.y / 2.0 + 2;
                ImVec2 text_pos(button_pos.x + (button_size.x - text_size.x) * 0.5f, button_pos.y + y_offset);
                draw_list->AddText(text_pos, fg, line);
            }

        }

        return pressed;
    };

    auto sub_selectable = [this](const std::string& label, bool selected, const ImVec2& size_arg) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;
        const float frame_padding = 2.0f;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(frame_padding, frame_padding));

        ImGuiContext&     g          = *GImGui;
        const ImGuiStyle& style      = g.Style;
        const ImGuiID     id         = window->GetID(label.data());
        const ImVec2      label_size = ImGui::CalcTextSize(label.data(), NULL, true);

        const float max_label = ImMax(label_size.x, label_size.y);

        ImVec2 pos  = window->DC.CursorPos;
        ImVec2 win_size = ImGui::GetContentRegionAvail();
        float view_scale = wxGetApp().plater()->get_current_canvas3D()->get_scale();
        ImVec2 size(std::max(60.0f * view_scale, size_arg.x), 24 * view_scale);
        //ImVec2 size = ImGui::CalcItemSize(size_arg, max_label + style.FramePadding.x * 2.0f, max_label + style.FramePadding.y * 2.0f);
        
        //pos.x = pos.x + win_size.x * 0.5 - size.x * 0.5; //in window top center
        const ImRect bb(pos, pos + size);
        ImGui::ItemSize(size, style.FramePadding.y);
        if (!ImGui::ItemAdd(bb, id)) {
            ImGui::PopStyleVar();
            return false;
        }

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

        float radius = size.x * 0.5f;
        // Render
        bool isDark = wxGetApp().dark_mode();
        ImU32 normal = ImGui::GetColorU32(ImGuiCol_WindowBg, 1.0f);
        ImU32 hover = ImGui::GetColorU32(ImGuiWrapper::COL_CREALITY);
        bool  highlight = held || hovered || selected;
        const ImU32 col = highlight ? hover : normal;

        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, highlight ? IM_COL32_WHITE : ImGui::GetColorU32(ImGuiCol_Text));
        ImGui::RenderNavHighlight(bb, id);
        ImGui::RenderFrame(bb.Min, bb.Max, col, selected ? false : true, radius);

        ImVec2 center = ImVec2((bb.Min.x + bb.Max.x) * 0.5f, (bb.Min.y + bb.Max.y) * 0.5f);
        /*window->DrawList->AddCircleFilled(center, radius, col, 32);*/

        // Draw text
        ImVec2 text_pos = ImVec2(center.x - label_size.x * 0.5f, center.y - label_size.y * 0.5f);
        ImGui::RenderText(text_pos, label.data());
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);

        return held;
    };

    auto operate_button = [this](const wxString &label, const wxString &suffix, bool enable) {

        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

     if (m_show_warning) {
            ImGui::PushTextWrapPos(200);
            ImGui::TextWrapped( "%s", _u8L(warning_text.c_str()).c_str());
            ImGui::PopTextWrapPos();
            ImGui::SameLine(0, 0);
        }

        const float frame_padding = 2.0f;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(frame_padding, frame_padding));

        ImGuiContext&     g          = *GImGui;
        const ImGuiStyle& style      = g.Style;
        wxString combindLabel = label + suffix;
        const ImGuiID     id         = window->GetID(combindLabel.ToStdString().data());

        //const float max_label = ImMax(label_size.x, label_size.y);

        float  view_scale = wxGetApp().plater()->get_current_canvas3D()->get_scale();
        ImVec2 size(100 * view_scale, 24 * view_scale);
        float  windowWidth = ImGui::GetWindowWidth() - 2.0 * ImGui::GetStyle().WindowPadding.x;
        ImVec2 pos  = ImVec2(window->DC.CursorStartPos.x, window->DC.CursorPos.y); 
        pos.x += (windowWidth - size.x);
       
         //pos.x = 0;
        ImVec2 win_size = ImGui::GetContentRegionAvail();
        const ImRect bb(pos, pos + size);
        ImGui::ItemSize(size, style.FramePadding.y);
        if (!ImGui::ItemAdd(bb, id)) {
            ImGui::PopStyleVar();
            return false;
        }

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

        float radius = size.x * 0.5f;
        // Render
        bool isDark = wxGetApp().dark_mode();
        ImU32 normal;

        if (!enable)
        {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            normal = ImGui::GetColorU32(IM_COL32(230.0f, 230.0f, 230.0f, 125));
            if (isDark)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(108.0f / 255.0f, 108.0f / 255.0f, 108.0f / 255.0f, 1.0f));
            else 
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(163.0f / 255.0f, 163.0f / 255.0f, 163.0f / 255.0f, 1.0f));
        }
        else 
        {
            normal = isDark ? ImGui::GetColorU32(IM_COL32(110, 110, 115, 255)) : ImGui::GetColorU32(IM_COL32(75, 75, 77, 125));
            ImGui::PushStyleColor(ImGuiCol_Text, hovered ? ImGui::GetColorU32(IM_COL32_WHITE) : ImGui::GetColorU32(ImGuiCol_Text, 1.0f)); 
        }
        
        ImU32 hover = ImGui::GetColorU32(ImGuiWrapper::COL_CREALITY);
        const ImU32 col = (!held && !hovered) || !enable ? normal : hover;
        ImGui::RenderNavHighlight(bb, id);
        ImGui::RenderFrame(bb.Min, bb.Max, col, false, radius);

        const ImVec2 label_size = ImGui::CalcTextSize(into_u8(label).data());
        ImVec2 center = ImVec2((bb.Min.x + bb.Max.x) * 0.5f, (bb.Min.y + bb.Max.y) * 0.5f);
        /*window->DrawList->AddCircleFilled(center, radius, col, 32);*/
        //ImVec2 windowSize(bb.Max.x, bb.Max.y);

        // Draw text

        ImVec2 text_pos = ImVec2(center.x - label_size.x * 0.5f, center.y - label_size.y * 0.5f);
        ImGui::RenderText(text_pos, into_u8(combindLabel).data());

        if (!enable)
        {
            ImGui::PopItemFlag();
        }
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

        return enable && pressed;
    };

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0);


    float view_scale = wxGetApp().plater()->get_current_canvas3D()->get_scale();
    bool  is_dark    = wxGetApp().dark_mode();
    
    {
        ImGuiContext& g      = *GImGui;
        ImGuiWindow*  window = g.CurrentWindow;
        ImVec2 p_min = window->DC.CursorPos;

        ImVec2 selectable_size = ImVec2(std::max((float) max_tab_length, 96.0f * view_scale), 43 * view_scale);

        if (selectable(_u8L("Union"), ImGui::UnionBooleanButton, m_operation_mode == MeshBooleanOperation::Union, selectable_size)) {
            if (m_operation_mode != MeshBooleanOperation::Union)
                m_show_warning = false;
            
            m_operation_mode = MeshBooleanOperation::Union;
        }
        ImGui::SameLine(0, 0);
        if (selectable(_u8L("Difference"), ImGui::DifferenceBooleanButton, m_operation_mode == MeshBooleanOperation::Difference,
                       selectable_size)) {
            if (m_operation_mode != MeshBooleanOperation::Difference)
                m_show_warning = false;

            m_operation_mode = MeshBooleanOperation::Difference;
        }
        ImGui::SameLine(0, 0);
        if (selectable(_u8L("Intersection"), ImGui::IntersectionBooleanButton, m_operation_mode == MeshBooleanOperation::Intersection,
                       selectable_size)) {
            if (m_operation_mode != MeshBooleanOperation::Intersection)
                m_show_warning = false;
            
            m_operation_mode = MeshBooleanOperation::Intersection;
        }

        ImVec4 line_color = is_dark ? ImVec4(110.0 / 255.0, 110.0 / 255.0, 114.0 / 255.0, 1) : ImVec4(214.0 / 255.0, 214.0 / 255.0, 220.0 / 255.0, 1.0);

        ImGui::PushStyleColor(ImGuiCol_Border, line_color);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0);

        ImGui::RenderFrameBorder(p_min, p_min + ImVec2(selectable_size.x * 3.0, selectable_size.y), 5);

        window->DrawList->AddLine(ImVec2(p_min.x + selectable_size.x, p_min.y), 
                                    ImVec2(p_min.x + selectable_size.x, p_min.y + selectable_size.y), ImGui::GetColorU32(line_color));
        window->DrawList->AddLine(ImVec2(p_min.x + selectable_size.x * 2, p_min.y),
                                  ImVec2(p_min.x + selectable_size.x * 2, p_min.y + selectable_size.y), ImGui::GetColorU32(line_color));

        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }

    ImGui::PopStyleVar();

    ImGui::AlignTextToFramePadding();
    std::string cap_str1 = m_operation_mode != MeshBooleanOperation::Difference ? _u8L("Part 1") : _u8L("Subtract from");
    m_imgui->text(cap_str1);
    ImGui::SameLine(max_cap_length);
    std::string select_src_str = m_src.mv ? "1 " + _u8L("selected") : _u8L("Select");
    select_src_str += "##select_source_volume";
    ImGui::PushItemWidth(select_btn_length);
    if (sub_selectable(select_src_str, m_selecting_state == MeshBooleanSelectingState::SelectSource, ImVec2(select_btn_length, 0)))
        m_selecting_state = MeshBooleanSelectingState::SelectSource;
    ImGui::PopItemWidth();
    if (m_src.mv) {
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        m_imgui->text(m_src.mv->name);

        ImGui::SameLine();
        //ImGui::PushStyleColor(ImGuiCol_Button, { 0, 0, 0, 0 });
        //ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.43f, 0.43f, 0.447f, 1.f));
        //ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGuiWrapper::COL_CREALITY);
        //ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGuiWrapper::COL_CREALITY);
        ImGui::PushStyleColor(ImGuiCol_Border, { 0, 0, 0, 0 });
        std::string text = into_u8(ImGui::TextSearchCloseIcon);
        ImVec2 size = ImGui::CalcTextSize(text.data()) + ImVec2(8, 5);
        if (ImGui::Button((text + "##src").c_str(), size))
        {
            m_src.reset();
        }
        //ImGui::PopStyleColor(4);
        ImGui::PopStyleColor(1);
    }

    ImGui::AlignTextToFramePadding();
    std::string cap_str2 = m_operation_mode != MeshBooleanOperation::Difference ? _u8L("Part 2") : _u8L("Subtract with");
    m_imgui->text(cap_str2);
    ImGui::SameLine(max_cap_length);
    std::string select_tool_str = m_tool.mv ? "1 " + _u8L("selected") : _u8L("Select");
    select_tool_str += "##select_tool_volume";
    ImGui::PushItemWidth(select_btn_length);
    if (sub_selectable(select_tool_str, m_selecting_state == MeshBooleanSelectingState::SelectTool, ImVec2(select_btn_length, 0)))
        m_selecting_state = MeshBooleanSelectingState::SelectTool;
    ImGui::PopItemWidth();
    if (m_tool.mv) {
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        m_imgui->text(m_tool.mv->name);

        ImGui::SameLine();

        ImVec2 textPos  = ImGui::GetCursorPos();
        ImVec2 nameSize = ImGui::CalcTextSize(m_tool.mv->name.data()) + ImVec2(8, 5);
        textPos.x += nameSize.x;
        ImGui::SetNextWindowPos(textPos);
        //ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.43f, 0.43f, 0.447f, 1.f));
        //ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGuiWrapper::COL_CREALITY);
        //ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGuiWrapper::COL_CREALITY);
        //ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_Button));
        ImGui::PushStyleColor(ImGuiCol_Border, {0, 0, 0, 0});
        std::string text = into_u8(ImGui::TextSearchCloseIcon) + "tool";
        ImVec2 size = ImGui::CalcTextSize(text.data()) + ImVec2(8, 5);
        if (ImGui::Button(text.c_str(), size))
        {
            m_tool.reset();
        }
        //ImGui::PopStyleColor(4);
        ImGui::PopStyleColor(1);
    }

    bool enable_button = m_src.mv && m_tool.mv;
    if (m_operation_mode == MeshBooleanOperation::Union)
    {
        if (operate_button(_L("Perform"), "##btn", enable_button)) {
            TriangleMesh temp_src_mesh = m_src.mv->mesh();
            temp_src_mesh.transform(m_src.trafo);
            TriangleMesh temp_tool_mesh = m_tool.mv->mesh();
            temp_tool_mesh.transform(m_tool.trafo);
            std::vector<TriangleMesh> temp_mesh_resuls;
            Slic3r::MeshBoolean::mcut::make_boolean(temp_src_mesh, temp_tool_mesh, temp_mesh_resuls, "UNION");
            if (temp_mesh_resuls.size() != 0) {
                generate_new_volume(true, *temp_mesh_resuls.begin());
                //wxGetApp().notification_manager()->close_plater_warning_notification(warning_text);
                m_show_warning = false;
            }
            else {
                //wxGetApp().notification_manager()->push_plater_warning_notification(warning_text);
                m_show_warning = true;
            }
        }
    }
    else if (m_operation_mode == MeshBooleanOperation::Difference) {
        m_imgui->bbl_checkbox(_L("Delete input"), m_diff_delete_input);
        if (operate_button(_L("Perform"), "##btn", enable_button)) {
            TriangleMesh temp_src_mesh = m_src.mv->mesh();
            temp_src_mesh.transform(m_src.trafo);
            TriangleMesh temp_tool_mesh = m_tool.mv->mesh();
            temp_tool_mesh.transform(m_tool.trafo);
            std::vector<TriangleMesh> temp_mesh_resuls;
            Slic3r::MeshBoolean::mcut::make_boolean(temp_src_mesh, temp_tool_mesh, temp_mesh_resuls, "A_NOT_B");
            if (temp_mesh_resuls.size() != 0) {
                generate_new_volume(m_diff_delete_input, *temp_mesh_resuls.begin());
                //wxGetApp().notification_manager()->close_plater_warning_notification(warning_text);
                m_show_warning = false;
            }
            else {
                //wxGetApp().notification_manager()->push_plater_warning_notification(warning_text);
                m_show_warning = true;
            }
        }
    }
    else if (m_operation_mode == MeshBooleanOperation::Intersection){
        m_imgui->bbl_checkbox(_L("Delete input"), m_inter_delete_input);
        if (operate_button(_L("Perform"), "##btn", enable_button)) {
            TriangleMesh temp_src_mesh = m_src.mv->mesh();
            temp_src_mesh.transform(m_src.trafo);
            TriangleMesh temp_tool_mesh = m_tool.mv->mesh();
            temp_tool_mesh.transform(m_tool.trafo);
            std::vector<TriangleMesh> temp_mesh_resuls;
            Slic3r::MeshBoolean::mcut::make_boolean(temp_src_mesh, temp_tool_mesh, temp_mesh_resuls, "INTERSECTION");
            if (temp_mesh_resuls.size() != 0) {
                generate_new_volume(m_inter_delete_input, *temp_mesh_resuls.begin());
                //wxGetApp().notification_manager()->close_plater_warning_notification(warning_text);
                m_show_warning = false;
            }
            else {
                //wxGetApp().notification_manager()->push_plater_warning_notification(warning_text);
                m_show_warning = true;
            }
        }
    }

    float win_w = ImGui::GetWindowWidth();
    if (last_w != win_w || last_y != y) {
        // ask canvas for another frame to render the window in the correct position
        m_imgui->set_requires_extra_frame();
        m_parent.set_as_dirty();
        m_parent.request_extra_frame();
        if (last_w != win_w)
            last_w = win_w;
        if (last_y != y)
            last_y = y;
    }

    GizmoImguiEnd();
    ImGuiWrapper::pop_toolbar_style();
}

void GLGizmoMeshBoolean::on_load(cereal::BinaryInputArchive &ar)
{
    ar(m_enable, m_operation_mode, m_selecting_state, m_diff_delete_input, m_inter_delete_input, m_src, m_tool);
    ModelObject *curr_model_object = m_c->selection_info() ? m_c->selection_info()->model_object() : nullptr;

    //reset if don't have any model object being selected
    if (!curr_model_object)
    {
        m_src.reset();
        m_tool.reset();
    }

    if (m_src.volume_idx < 0)
    {
        m_src.reset();
    }

	if (m_tool.volume_idx < 0)
	{
		m_tool.reset();
	}

    m_src.mv = curr_model_object == nullptr ? nullptr : m_src.volume_idx < 0 ? nullptr : curr_model_object->volumes[m_src.volume_idx];
    m_tool.mv = curr_model_object == nullptr ? nullptr : m_tool.volume_idx < 0 ? nullptr : curr_model_object->volumes[m_tool.volume_idx];
}

void GLGizmoMeshBoolean::on_save(cereal::BinaryOutputArchive &ar) const
{
    ar(m_enable, m_operation_mode, m_selecting_state, m_diff_delete_input, m_inter_delete_input, m_src, m_tool);
}

void GLGizmoMeshBoolean::generate_new_volume(bool delete_input, const TriangleMesh& mesh_result) {

    wxGetApp().plater()->take_snapshot("Mesh Boolean");

    ModelObject* curr_model_object = m_c->selection_info()->model_object();

    // generate new volume
    ModelVolume* new_volume = curr_model_object->add_volume(std::move(mesh_result));

    // assign to new_volume from old_volume
    ModelVolume* old_volume = m_src.mv;
    std::string suffix;
    switch (m_operation_mode)
    {
    case MeshBooleanOperation::Union:
        suffix = "union";
        break;
    case MeshBooleanOperation::Difference:
        suffix = "difference";
        break;
    case MeshBooleanOperation::Intersection:
        suffix = "intersection";
        break;
    }
    new_volume->name = old_volume->name + " - " + suffix;
    new_volume->set_new_unique_id();
    new_volume->config.apply(old_volume->config);
    new_volume->set_type(old_volume->type());
    new_volume->set_material_id(old_volume->material_id());
    //new_volume->set_offset(old_volume->get_transformation().get_offset());
    //Vec3d translate_z = { 0,0, (new_volume->source.mesh_offset - old_volume->source.mesh_offset).z() };
    //new_volume->translate(new_volume->get_transformation().get_matrix_no_offset() * translate_z);
    //new_volume->supported_facets.assign(old_volume->supported_facets);
    //new_volume->seam_facets.assign(old_volume->seam_facets);
    //new_volume->mmu_segmentation_facets.assign(old_volume->mmu_segmentation_facets);

    // delete old_volume
    std::swap(curr_model_object->volumes[m_src.volume_idx], curr_model_object->volumes.back());
    curr_model_object->delete_volume(curr_model_object->volumes.size() - 1);

    //auto& selection = m_parent.get_selection();
    int current_obj_idx = m_current_obj_idx;
    if (delete_input) {
        std::vector<ItemForDelete> items;
        auto obj_idx = m_parent.get_selection().get_object_idx();
        m_parent.get_selection().add_object(current_obj_idx);
        items.emplace_back(ItemType::itVolume, obj_idx, m_tool.volume_idx);
        wxGetApp().obj_list()->delete_from_model_and_list(items);
    }

    //bool sinking = curr_model_object->bounding_box().min.z() < SINKING_Z_THRESHOLD;
    //if (!sinking)
    //    curr_model_object->ensure_on_bed();
    //curr_model_object->sort_volumes(true);

    wxGetApp().plater()->update();
    wxGetApp().obj_list()->select_item([this, new_volume, current_obj_idx]() {
        wxDataViewItem sel_item;

        wxDataViewItemArray items =
            wxGetApp().obj_list()->reorder_volumes_and_get_selection(/* m_parent.get_selection().get_object_idx() */ current_obj_idx,
                                                                                             [new_volume](const ModelVolume* volume) {
                                                                                                 return volume == new_volume;
                                                                                             });
        if (!items.IsEmpty())
            sel_item = items.front();

        return sel_item;
        });

    m_src.reset();
    m_tool.reset();
    m_selecting_state = MeshBooleanSelectingState::SelectSource;
}


}}
