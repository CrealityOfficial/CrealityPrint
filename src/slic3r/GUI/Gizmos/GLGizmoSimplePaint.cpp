#include "GLGizmoSimplePaint.hpp"

#include "slic3r/GUI/GLCanvas3D.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/ImGuiWrapper.hpp"
#include "slic3r/GUI/Camera.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/BitmapCache.hpp"
#include "slic3r/GUI/format.hpp"
#include "slic3r/GUI/GUI_ObjectList.hpp"
#include "slic3r/GUI/NotificationManager.hpp"
#include "slic3r/GUI/GUI.hpp"
#include "slic3r/GUI/PartPlate.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/ModelObject.hpp"
#include "libslic3r/ModelVolume.hpp"
#include "libslic3r/ModelInstance.hpp"
#include "slic3r/Utils/UndoRedo.hpp"
#include <wx/window.h>
#include <GL/glew.h>

namespace Slic3r::GUI {

static inline void show_notification_extruders_limit_exceeded()
{
    wxGetApp()
        .plater()
        ->get_notification_manager()
        ->push_notification(NotificationType::MmSegmentationExceededExtrudersLimit, NotificationManager::NotificationLevel::PrintInfoNotificationLevel,
                            GUI::format(_L("Filament count exceeds the maximum number that painting tool supports. only the "
                                           "first %1% filaments will be available in painting tool."), GLGizmoSimplePaint::EXTRUDERS_LIMIT));
}

void GLGizmoSimplePaint::on_opening()
{
    if (wxGetApp().filaments_cnt() > int(GLGizmoSimplePaint::EXTRUDERS_LIMIT))
        show_notification_extruders_limit_exceeded();
}

void GLGizmoSimplePaint::on_shutdown()
{
    m_parent.use_slope(false);
    m_parent.toggle_model_objects_visibility(true);
}

std::string GLGizmoSimplePaint::on_get_name() const
{
    return _u8L("Color Painting");
}

bool GLGizmoSimplePaint::on_is_selectable() const
{
    //return (wxGetApp().preset_bundle->printers.get_edited_preset().printer_technology() == ptFFF
    //        && /*wxGetApp().get_mode() != comSimple && */wxGetApp().filaments_cnt() > 1);
    return true;
}

bool GLGizmoSimplePaint::on_is_activable() const
{
    const Selection& selection = m_parent.get_selection();
    return !selection.is_empty() && (selection.is_single_full_instance() || selection.is_any_volume()) /*&& wxGetApp().filaments_cnt() > 1*/;
}

//BBS: use the global one in 3DScene.cpp
/*static std::vector<ColorRGBA> get_extruders_colors()
{
    unsigned char                     rgb_color[3] = {};
    std::vector<std::string>          colors       = Slic3r::GUI::wxGetApp().plater()->get_extruder_colors_from_plater_config();
    std::vector<ColorRGBA> colors_out(colors.size());
    for (const std::string &color : colors) {
        Slic3r::GUI::BitmapCache::parse_color(color, rgb_color);
        size_t color_idx      = &color - &colors.front();
        colors_out[color_idx] = {float(rgb_color[0]) / 255.f, float(rgb_color[1]) / 255.f, float(rgb_color[2]) / 255.f, 1.f};
    }

    return colors_out;
}*/

static std::vector<int> get_extruder_id_for_volumes(const ModelObject &model_object)
{
    std::vector<int> extruders_idx;
    extruders_idx.reserve(model_object.volumes.size());
    for (const ModelVolume *model_volume : model_object.volumes) {
        if (!model_volume->is_model_part())
            continue;

        extruders_idx.emplace_back(model_volume->extruder_id());
    }

    return extruders_idx;
}

void GLGizmoSimplePaint::init_extruders_data()
{
    m_extruders_colors = get_extruders_colors();
    if(m_extruders_colors.size()>1 && m_selected_extruder_idx==m_extruders_colors.size())
    {
        m_selected_extruder_idx--;
    }
}

bool GLGizmoSimplePaint::on_init()
{
    // BBS
    m_shortcut_key = WXK_CONTROL_N;

    m_desc["clipping_of_view_caption"] = _L("Alt + Mouse wheel");
    m_desc["clipping_of_view"]     = _L("Section view");
    m_desc["reset_direction"]     = _L("Reset direction");
    m_desc["cursor_size_caption"]  = _L("Ctrl + Mouse wheel");
    m_desc["cursor_size"]          = _L("Pen size");
    m_desc["cursor_type"]          = _L("Pen shape");

    m_desc["paint_caption"]        = _L("Left mouse button");
    m_desc["paint"]                = _L("Paint");
    m_desc["erase_caption"]        = _L("Shift + Left mouse button");
    m_desc["erase"]                = _L("Erase");
    m_desc["shortcut_key_caption"] = _L("Key 1~9");
    m_desc["shortcut_key"]         = _L("Choose filament");
    m_desc["edge_detection"]       = _L("Edge detection");
    m_desc["gap_area_caption"]     = _L("Ctrl + Mouse wheel");
    m_desc["gap_area"]             = _L("Gap area");
    m_desc["perform"]              = _L("Perform");

    m_desc["remove_all"]           = _L("Erase all painting");
    m_desc["circle"]               = _L("Circle");
    m_desc["sphere"]               = _L("Sphere");
    m_desc["pointer"]              = _L("Triangles");

    m_desc["filaments"]            = _L("Filaments");
    m_desc["add_filament"]         = _L("Add");
    m_desc["tool_type"]            = _L("Tool type");
    m_desc["tool_brush"]           = _L("Brush");
    m_desc["tool_object_fill"]     = _L("Object fill");
    m_desc["tool_smart_fill"]      = _L("Smart fill");
    m_desc["tool_bucket_fill"]     = _L("Bucket fill");

    m_desc["smart_fill_angle_caption"] = _L("Ctrl + Mouse wheel");
    m_desc["smart_fill_angle"]     = _L("Smart fill angle");

    m_desc["height_range_caption"] = _L("Ctrl + Mouse wheel");
    m_desc["height_range"]         = _L("Height range");

    //add toggle wire frame hint
    m_desc["toggle_wireframe_caption"]        = _L("Alt + Shift + Enter");
    m_desc["toggle_wireframe"]                = _L("Toggle Wireframe");

    m_detect_geometry_edge = true;
    m_smart_fill_angle     = 30.f;

    init_extruders_data();

    return true;
}

GLGizmoSimplePaint::GLGizmoSimplePaint(GLCanvas3D& parent, const std::string& icon_filename, unsigned int sprite_id)
    : GLGizmoPainterBase(parent, icon_filename, sprite_id), m_current_tool(ImGui::FillButtonIcon)
{
}

void GLGizmoSimplePaint::render_painter_gizmo()
{
    const Selection& selection = m_parent.get_selection();

    glsafe(::glEnable(GL_BLEND));
    glsafe(::glEnable(GL_DEPTH_TEST));

    render_triangles(selection);

    m_c->object_clipper()->render_cut();
    m_c->instances_hider()->render_cut();
    render_cursor();

    glsafe(::glDisable(GL_BLEND));
}

void GLGizmoSimplePaint::data_changed(bool is_serializing)
{
    GLGizmoPainterBase::data_changed(is_serializing);
    if (m_state != On || wxGetApp().preset_bundle->printers.get_edited_preset().printer_technology() != ptFFF )
        return;

//     int nnn = wxGetApp().extruders_edited_cnt();
//     if(m_extruders_colors.size()!=wxGetApp().filaments_cnt() )
//     {
//     }

    ModelObject* model_object = m_c->selection_info()->model_object();
    int prev_extruders_count = int(m_extruders_colors.size());
    if (prev_extruders_count != wxGetApp().filaments_cnt()) {
        if (wxGetApp().filaments_cnt() > int(GLGizmoSimplePaint::EXTRUDERS_LIMIT))
            show_notification_extruders_limit_exceeded();

        this->init_extruders_data();
        // Reinitialize triangle selectors because of change of extruder count need also change the size of GLIndexedVertexArray
        if (prev_extruders_count != wxGetApp().filaments_cnt())
            this->init_model_triangle_selectors();
    }
    else if (get_extruders_colors() != m_extruders_colors) {
        this->init_extruders_data();
        this->update_triangle_selectors_colors();
    }
    else if (model_object != nullptr && get_extruder_id_for_volumes(*model_object) != m_volumes_extruder_idxs) {
        this->init_model_triangle_selectors();
    }
}

void GLGizmoSimplePaint::render_triangles(const Selection &selection) const
{
    ClippingPlaneDataWrapper clp_data = this->get_clipping_plane_data();
    auto* shader = wxGetApp().get_shader("mm_gouraud");
    if (!shader)
        return;
    shader->start_using();
    shader->set_uniform("clipping_plane", clp_data.clp_dataf);
    shader->set_uniform("z_range", clp_data.z_range);
    shader->set_uniform("slope.actived", m_parent.is_using_slope());
    ScopeGuard guard([shader]() { if (shader) shader->stop_using(); });

    //BBS: to improve the random white pixel issue
    glsafe(::glDisable(GL_CULL_FACE));

    const ModelObject *mo      = m_c->selection_info()->model_object();

    if (mo == nullptr)
    {
        return;
    }

    int                mesh_id = -1;
    for (const ModelVolume *mv : mo->volumes) {
        if (!mv->is_model_part())
            continue;

        ++mesh_id;

        Transform3d trafo_matrix;
        if (m_parent.get_canvas_type() == GLCanvas3D::CanvasAssembleView) {
            trafo_matrix = mo->instances[selection.get_instance_idx()]->get_assemble_transformation().get_matrix() * mv->get_matrix();
            trafo_matrix.translate(mv->get_transformation().get_offset() * (GLVolume::explosion_ratio - 1.0) + mo->instances[selection.get_instance_idx()]->get_offset_to_assembly() * (GLVolume::explosion_ratio - 1.0));
        }
        else {
            trafo_matrix = mo->instances[selection.get_instance_idx()]->get_transformation().get_matrix()* mv->get_matrix();
        }

        const bool is_left_handed = trafo_matrix.matrix().determinant() < 0.0;
        if (is_left_handed)
            glsafe(::glFrontFace(GL_CW));

        const Camera& camera = wxGetApp().plater()->get_camera();
        const Transform3d& view_matrix = camera.get_view_matrix();
        shader->set_uniform("view_model_matrix", view_matrix * trafo_matrix);
        shader->set_uniform("projection_matrix", camera.get_projection_matrix());
        const Matrix3d view_normal_matrix = view_matrix.matrix().block(0, 0, 3, 3) * trafo_matrix.matrix().block(0, 0, 3, 3).inverse().transpose();
        shader->set_uniform("view_normal_matrix", view_normal_matrix);

        shader->set_uniform("volume_world_matrix", trafo_matrix);
        shader->set_uniform("volume_mirrored", is_left_handed);
        m_triangle_selectors[mesh_id]->render(m_imgui, trafo_matrix);

        if (is_left_handed)
            glsafe(::glFrontFace(GL_CCW));
    }
}

// BBS
bool GLGizmoSimplePaint::on_number_key_down(int number)
{
    int extruder_idx = number - 1;
    if (extruder_idx < m_extruders_colors.size() && extruder_idx >= 0)
        m_selected_extruder_idx = extruder_idx;

    return true;
}

bool GLGizmoSimplePaint::on_key_down_select_tool_type(int keyCode) {
    switch (keyCode)
    {
    case 'O':
        m_current_tool = ImGui::SphereButtonIcon;
        break;
    case 'F':
        m_current_tool = ImGui::FillButtonIcon;
        break;
    case 'H':
        m_current_tool = ImGui::HeightRangeIcon;
        break;
    default:
        return false;
        break;
    }
    return true;
}

static void render_extruders_combo(const std::string& label,
                                   const std::vector<std::string>& extruders,
                                   const std::vector<ColorRGBA>& extruders_colors,
                                   size_t& selection_idx)
{
    assert(!extruders_colors.empty());
    assert(extruders_colors.size() == extruders_colors.size());

    size_t selection_out = selection_idx;
    // It is necessary to use BeginGroup(). Otherwise, when using SameLine() is called, then other items will be drawn inside the combobox.
    ImGui::BeginGroup();
    ImVec2 combo_pos = ImGui::GetCursorScreenPos();
    if (ImGui::BeginCombo(label.c_str(), "")) {
        for (size_t extruder_idx = 0; extruder_idx < std::min(extruders.size(), GLGizmoSimplePaint::EXTRUDERS_LIMIT); ++extruder_idx) {
            ImGui::PushID(int(extruder_idx));
            ImVec2 start_position = ImGui::GetCursorScreenPos();

            if (ImGui::Selectable("", extruder_idx == selection_idx))
                selection_out = extruder_idx;

            ImGui::SameLine();
            ImGuiStyle &style  = ImGui::GetStyle();
            float       height = ImGui::GetTextLineHeight();
            ImGui::GetWindowDrawList()->AddRectFilled(start_position, ImVec2(start_position.x + height + height / 2, start_position.y + height), ImGuiWrapper::to_ImU32(extruders_colors[extruder_idx]));
            ImGui::GetWindowDrawList()->AddRect(start_position, ImVec2(start_position.x + height + height / 2, start_position.y + height), IM_COL32_BLACK);

            ImGui::SetCursorScreenPos(ImVec2(start_position.x + height + height / 2 + style.FramePadding.x, start_position.y));
            ImGui::Text("%s", extruders[extruder_idx].c_str());
            ImGui::PopID();
        }

        ImGui::EndCombo();
    }

    ImVec2      backup_pos = ImGui::GetCursorScreenPos();
    ImGuiStyle &style      = ImGui::GetStyle();

    ImGui::SetCursorScreenPos(ImVec2(combo_pos.x + style.FramePadding.x, combo_pos.y + style.FramePadding.y));
    ImVec2 p      = ImGui::GetCursorScreenPos();
    float  height = ImGui::GetTextLineHeight();

    ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + height + height / 2, p.y + height), ImGuiWrapper::to_ImU32(extruders_colors[selection_idx]));
    ImGui::GetWindowDrawList()->AddRect(p, ImVec2(p.x + height + height / 2, p.y + height), IM_COL32_BLACK);

    ImGui::SetCursorScreenPos(ImVec2(p.x + height + height / 2 + style.FramePadding.x, p.y));
    ImGui::Text("%s", extruders[selection_out].c_str());
    ImGui::SetCursorScreenPos(backup_pos);
    ImGui::EndGroup();

    selection_idx = selection_out;
}

void GLGizmoSimplePaint::show_tooltip_information(float caption_max, float x, float y)
{
    ImTextureID normal_id = m_parent.get_gizmos_manager().get_icon_texture_id(GLGizmosManager::MENU_ICON_NAME::IC_TOOLBAR_TOOLTIP);
    ImTextureID hover_id  = m_parent.get_gizmos_manager().get_icon_texture_id(GLGizmosManager::MENU_ICON_NAME::IC_TOOLBAR_TOOLTIP_HOVER);

    caption_max += m_imgui->calc_text_size(std::string_view{": "}).x + 15.f;

    float  scale       = m_parent.get_scale();
    ImVec2 button_size = ImVec2(25 * scale, 25 * scale); // ORCA: Use exact resolution will prevent blur on icon
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0, 0}); // ORCA: Dont add padding
    ImGui::ImageButton3(normal_id, hover_id, button_size);

    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip2(ImVec2(x, y));
        auto draw_text_with_caption = [this, &caption_max](const wxString &caption, const wxString &text) {
            m_imgui->text_colored(ImGuiWrapper::COL_ACTIVE, caption);
            ImGui::SameLine(caption_max);
            m_imgui->text_colored(ImGuiWrapper::COL_WINDOW_BG, text);
        };

        std::vector<std::string> tip_items;
        switch (m_tool_type) {
            case ToolType::BRUSH: 
                tip_items = {"paint", "erase", "height_range", "toggle_wireframe"};
                break;
            case ToolType::OBJECT_FILL:
                tip_items = {"paint", "erase"};
                break;
            case ToolType::BUCKET_FILL: 
                tip_items = {"paint", "erase", "smart_fill_angle", "toggle_wireframe"};
                break;
            default:
                break;
        }
        for (const auto &t : tip_items) draw_text_with_caption(m_desc.at(t + "_caption") + ": ", m_desc.at(t));
        ImGui::EndTooltip();
    }
    ImGui::PopStyleVar(2);
}

void GLGizmoSimplePaint::on_render_input_window(float x, float y, float bottom_limit, bool force_update_pos)
{
    ModelObject* model_obj = m_c->selection_info()->model_object();

    const float approx_height = m_imgui->scaled(22.0f);
    y = std::min(y, bottom_limit - approx_height);
    GizmoImguiSetNextWIndowPos(x, y, ImGuiCond_Always, 0.0f, 0.0f, force_update_pos);
    m_imgui_start_pos[0] = x;
    m_imgui_start_pos[1] = y;

    // 空态提示：没有模型时只显示简短文案
    if (model_obj == nullptr) {
        ImGuiWrapper::push_toolbar_style(m_parent.get_scale());
        GizmoImguiBegin(get_name(false), ImGuiWrapper::TOOLBAR_WINDOW_FLAGS);
        ImGui::TextUnformatted(into_u8(_L("请选择模型以开始涂色")).c_str());
        m_imgui_end_pos[0] = m_imgui_start_pos[0] + ImGui::GetWindowWidth();
        m_imgui_end_pos[1] = m_imgui_start_pos[1] + ImGui::GetWindowHeight();
        GizmoImguiEnd();
        ImGuiWrapper::pop_toolbar_style();
        return;
    }

    wchar_t old_tool = m_current_tool;
    if (m_current_tool != ImGui::FillButtonIcon && m_current_tool != ImGui::HeightRangeIcon && m_current_tool != ImGui::SphereButtonIcon)
        m_current_tool = ImGui::FillButtonIcon;

    // BBS
    ImGuiWrapper::push_toolbar_style(m_parent.get_scale());
    GizmoImguiBegin(get_name(false), ImGuiWrapper::TOOLBAR_WINDOW_FLAGS);

    // First calculate width of all the texts that are could possibly be shown. We will decide set the dialog width based on that:
    const float space_size = m_imgui->get_style_scaling() * 8;
    const float height_range_slider_left = m_imgui->calc_text_size(m_desc.at("height_range")).x + m_imgui->scaled(2.f);

    const float remove_btn_width = m_imgui->calc_text_size(m_desc.at("remove_all")).x + m_imgui->scaled(1.f);
    const float minimal_slider_width = m_imgui->scaled(4.f);

    float caption_max = 0.f;
    float total_text_max = 0.f;
    for (const auto &t : std::array<std::string, 3>{"paint", "erase", "height_range"}) {
        caption_max = std::max(caption_max, m_imgui->calc_text_size(m_desc[t + "_caption"]).x);
        total_text_max = std::max(total_text_max, m_imgui->calc_text_size(m_desc[t]).x);
    }
    total_text_max += caption_max + m_imgui->scaled(1.f);
    caption_max += m_imgui->scaled(1.f);

    const float sliders_left_width = height_range_slider_left + space_size;
    const float slider_icon_width = m_imgui->get_slider_icon_size().x;
    float window_width = minimal_slider_width + sliders_left_width + slider_icon_width;
    const int max_filament_items_per_line = 8;
    const float empty_button_width = m_imgui->calc_button_size("").x;
    const float filament_item_width = empty_button_width + m_imgui->scaled(1.5f);

    window_width = std::max(window_width, total_text_max);
    window_width = std::max(window_width, remove_btn_width);
    window_width = std::max(window_width, max_filament_items_per_line * filament_item_width + m_imgui->scaled(0.5f));

    const float sliders_width = m_imgui->scaled(7.0f);
    const float drag_left_width = ImGui::GetStyle().WindowPadding.x + sliders_width - space_size;

    const float max_tooltip_width = ImGui::GetFontSize() * 20.0f;
    ImDrawList * draw_list = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    static float color_button_high  = 25.0;
    draw_list->AddRectFilled({pos.x - 10.0f, pos.y - 7.0f}, {pos.x + window_width + ImGui::GetFrameHeight(), pos.y + color_button_high}, ImGui::GetColorU32(ImGuiCol_FrameBgActive, 1.0f), 5.0f);

    float color_button = ImGui::GetCursorPos().y;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 0.75f));
    m_imgui->text(m_desc.at("filaments"));
    ImGui::PopStyleColor(1);
    ImGui::SameLine();
    const float add_btn_w = m_imgui->calc_text_size(m_desc.at("add_filament")).x + m_imgui->scaled(2.0f);
    if (ImGui::Button(into_u8(m_desc.at("add_filament")).c_str(), ImVec2(add_btn_w, 0.f))) {
        if (m_add_filament_handler)
            m_add_filament_handler();
    }
    ImGui::NewLine();

    float start_pos_x = ImGui::GetCursorPos().x;
    const ImVec2 max_label_size = ImGui::CalcTextSize("99", NULL, true);
    const float item_spacing = m_imgui->scaled(0.8f);
    size_t n_extruder_colors = std::min((size_t)EnforcerBlockerType::ExtruderMax, m_extruders_colors.size());
    for (int extruder_idx = 0; extruder_idx < n_extruder_colors; extruder_idx++) {
        const ColorRGBA &extruder_color = m_extruders_colors[extruder_idx];
        ImVec4           color_vec      = ImGuiWrapper::to_ImVec4(extruder_color);
        std::string color_label = std::string("##extruder color ") + std::to_string(extruder_idx);
        std::string item_text = std::to_string(extruder_idx + 1);
        const ImVec2 label_size = ImGui::CalcTextSize(item_text.c_str(), NULL, true);

        const ImVec2 button_size(max_label_size.x + m_imgui->scaled(0.5f),0.f);

        float button_offset = start_pos_x;
        if (extruder_idx % max_filament_items_per_line != 0) {
            button_offset += filament_item_width * (extruder_idx % max_filament_items_per_line);
            ImGui::SameLine(button_offset);
        }

        // draw filament background
        ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoTooltip;
        if (m_selected_extruder_idx != extruder_idx) flags |= ImGuiColorEditFlags_NoBorder;
        #ifdef __APPLE__
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGuiWrapper::COL_ORCA); // ORCA use orca color for selected filament border
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0);
            bool color_picked = ImGui::ColorButton(color_label.c_str(), color_vec, flags, button_size);
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(1);
        #else
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGuiWrapper::COL_CREALITY); // ORCA use orca color for selected filament border
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0);
            bool color_picked = ImGui::ColorButton(color_label.c_str(), color_vec, flags, button_size);
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(1);
        #endif
        color_button_high = ImGui::GetCursorPos().y - color_button - 2.0;
        if (color_picked) { m_selected_extruder_idx = extruder_idx; }

        if (extruder_idx < 16 && ImGui::IsItemHovered()) m_imgui->tooltip(_L("Shortcut Key ") + std::to_string(extruder_idx + 1), max_tooltip_width);

        // draw filament id
        float gray = 0.299 * extruder_color.r() + 0.587 * extruder_color.g() + 0.114 * extruder_color.b();
        ImGui::SameLine(button_offset + (button_size.x - label_size.x) / 2.f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {10.0,15.0});
        if (gray * 255.f < 80.f)
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", item_text.c_str());
        else
            ImGui::TextColored(ImVec4(0.0f, 0.0f, 0.0f, 1.0f), "%s", item_text.c_str());

        ImGui::PopStyleVar();
    }
    //ImGui::NewLine();
    ImGui::Dummy(ImVec2(0.0f, ImGui::GetFontSize() * 0.1));

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 0.75f));
    m_imgui->text(m_desc.at("tool_type"));
    ImGui::PopStyleColor(1);

    std::array<wchar_t, 3> tool_ids;
    tool_ids = { ImGui::SphereButtonIcon, ImGui::FillButtonIcon, ImGui::HeightRangeIcon };
    std::array<wchar_t, 3> icons;
    if (m_is_dark_mode)
        icons = { ImGui::SphereButtonDarkIcon, ImGui::FillButtonDarkIcon, ImGui::HeightRangeDarkIcon };
    else
        icons = { ImGui::SphereButtonIcon, ImGui::FillButtonIcon, ImGui::HeightRangeIcon };
    std::array<wxString, 3> tool_tips = { _L("Object fill"), _L("Fill"), _L("Height Range") };
    std::array<wxString, 3> tool_labels = { m_desc.at("tool_object_fill"), m_desc.at("tool_smart_fill"), m_desc.at("height_range") };
    for (int i = 0; i < tool_ids.size(); i++) {
        std::string  str_label = std::string("");
        std::wstring btn_name  = icons[i] + boost::nowide::widen(str_label);

        if (i != 0) ImGui::SameLine();

        ImGui::BeginGroup();
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        bool is_selected = (m_current_tool == tool_ids[i]);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_Button, m_is_dark_mode ? ImVec4(0.3f, 0.3f, 0.32f, 1.f) : ImVec4(0.95f, 0.95f, 0.95f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGuiWrapper::COL_CREALITY);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGuiWrapper::COL_CREALITY);
        if (is_selected)
            ImGui::PushStyleColor(ImGuiCol_Button, ImGuiWrapper::COL_CREALITY);

        float view_scale = wxGetApp().plater()->get_current_canvas3D()->get_scale();
        ImVec2 tool_btn_size(34.f * view_scale, 34.f * view_scale);
        bool btn_clicked = ImGui::Button(into_u8(btn_name).c_str(), tool_btn_size);

        ImGui::PopStyleColor(is_selected ? 5 : 4);
        ImGui::PopStyleVar(2);

        // Label under icon, centered
        ImVec2 text_size = ImGui::CalcTextSize(into_u8(tool_labels[i]).c_str());
        float label_x = ImGui::GetCursorPosX() + std::max(0.f, (tool_btn_size.x - text_size.x) * 0.5f);
        ImGui::SetCursorPosX(label_x);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 0.8f));
        ImGui::TextUnformatted(into_u8(tool_labels[i]).c_str());
        ImGui::PopStyleColor(1);
        ImGui::EndGroup();

        if (btn_clicked && m_current_tool != tool_ids[i]) {
            m_current_tool = tool_ids[i];
            for (auto &triangle_selector : m_triangle_selectors) {
                triangle_selector->seed_fill_unselect_all_triangles();
                triangle_selector->request_update_render_data();
            }
        }

        if (ImGui::IsItemHovered()) {
            m_imgui->tooltip(tool_tips[i], max_tooltip_width);
        }
    }

    ImGui::Dummy(ImVec2(0.0f, ImGui::GetFontSize() * 0.1));

    if (m_current_tool != old_tool)
        this->tool_changed(old_tool, m_current_tool);

    if (m_current_tool == ImGui::SphereButtonIcon) {
        m_tool_type   = ToolType::OBJECT_FILL;
        m_cursor_type = TriangleSelector::CursorType::POINTER;
        m_detect_geometry_edge = false;
    } else if (m_current_tool == ImGui::FillButtonIcon) {
        m_cursor_type = TriangleSelector::CursorType::POINTER;
        m_detect_geometry_edge = true;
        m_smart_fill_angle = 30.f;
        m_tool_type = ToolType::BUCKET_FILL;

    } else if (m_current_tool == ImGui::HeightRangeIcon) {
        m_tool_type   = ToolType::BRUSH;
        m_cursor_type = TriangleSelector::CursorType::HEIGHT_RANGE;
        ImGui::AlignTextToFramePadding();
        m_imgui->text(m_desc["height_range"] + ":");
        ImGui::SameLine(height_range_slider_left);
        ImGui::PushItemWidth(sliders_width);
        std::string format_str = std::string("%.2f") + I18N::translate_utf8("mm", "Heigh range," "Facet in [cursor z, cursor z + height] will be selected.");
        m_imgui->bbl_slider_float_style("##cursor_height", &m_cursor_height, CursorHeightMin, CursorHeightMax, format_str.data(), 1.0f, true);
        ImGui::SameLine(drag_left_width + height_range_slider_left);
        ImGui::PushItemWidth(1.5 * slider_icon_width);
        ImVec4 ghost_bg = ImGui::GetStyle().Colors[ImGuiCol_FrameBg];
        ghost_bg.w = 0.25f * ghost_bg.w + 0.05f;
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ghost_bg);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ghost_bg);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ghost_bg);
        ImGui::BBLDragFloat("##cursor_height_input", &m_cursor_height, 0.05f, 0.0f, 0.0f, "%.2f");
        ImGui::PopStyleColor(3);
    }

    ImGui::Separator();
    float f_scale =m_parent.get_gizmos_manager().get_layout_scale();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 4.0f * f_scale));

    if (m_imgui->button(m_desc.at("remove_all"))) {
        Plater::TakeSnapshot snapshot(wxGetApp().plater(), "Reset selection", UndoRedo::SnapshotType::GizmoAction);
        ModelObject *        mo  = m_c->selection_info()->model_object();
        int                  idx = -1;
        for (ModelVolume *mv : mo->volumes)
            if (mv->is_model_part()) {
                ++idx;
                m_triangle_selectors[idx]->reset();
                m_triangle_selectors[idx]->request_update_render_data(true);
            }

        update_model_object();
        m_parent.set_as_dirty();
    }
    ImGui::PopStyleVar(1);
    m_imgui_end_pos[0] = m_imgui_start_pos[0] + ImGui::GetWindowWidth();
    m_imgui_end_pos[1] = m_imgui_start_pos[1] + ImGui::GetWindowHeight();
    GizmoImguiEnd();

    // BBS
    ImGuiWrapper::pop_toolbar_style();
}


void GLGizmoSimplePaint::update_model_object()
{
    bool updated = false;
    ModelObject* mo = m_c->selection_info()->model_object();
    int idx = -1;
    for (ModelVolume* mv : mo->volumes) {
        if (! mv->is_model_part())
            continue;
        ++idx;
        updated |= mv->mmu_segmentation_facets.set(*m_triangle_selectors[idx].get());
    }

    if (updated) {
        const ModelObjectPtrs &mos = wxGetApp().model().objects;
        size_t obj_idx = std::find(mos.begin(), mos.end(), mo) - mos.begin();
        wxGetApp().obj_list()->update_info_items(obj_idx);
        wxGetApp().plater()->get_partplate_list().notify_instance_update(obj_idx, 0);
        m_parent.post_event(SimpleEvent(EVT_GLCANVAS_SCHEDULE_BACKGROUND_PROCESS));
    }
}

void GLGizmoSimplePaint::init_model_triangle_selectors()
{
    const ModelObject *mo = m_c->selection_info()->model_object();
    m_triangle_selectors.clear();
    m_volumes_extruder_idxs.clear();

    // Don't continue when extruders colors are not initialized
    if(m_extruders_colors.empty())
        return;

    // BBS: Don't continue when model object is null
    if (mo == nullptr)
        return;

    for (const ModelVolume *mv : mo->volumes) {
        if (!mv->is_model_part())
            continue;

        int extruder_idx = (mv->extruder_id() > 0) ? mv->extruder_id() - 1 : 0;
        std::vector<ColorRGBA> ebt_colors;
        ebt_colors.push_back(m_extruders_colors[size_t(extruder_idx)]);
        ebt_colors.insert(ebt_colors.end(), m_extruders_colors.begin(), m_extruders_colors.end());

        // This mesh does not account for the possible Z up SLA offset.
        const TriangleMesh* mesh = &mv->mesh();
        m_triangle_selectors.emplace_back(std::make_unique<TriangleSelectorPatch>(*mesh, ebt_colors, 0.2));
        // Reset of TriangleSelector is done inside TriangleSelectorMmGUI's constructor, so we don't need it to perform it again in deserialize().
        EnforcerBlockerType max_ebt = (EnforcerBlockerType)std::min(m_extruders_colors.size(), (size_t)EnforcerBlockerType::ExtruderMax);
        m_triangle_selectors.back()->deserialize(mv->mmu_segmentation_facets.get_data(), false, max_ebt);
        m_triangle_selectors.back()->request_update_render_data();
        m_triangle_selectors.back()->set_wireframe_needed(true);
        m_volumes_extruder_idxs.push_back(mv->extruder_id());
    }
}

void GLGizmoSimplePaint::update_triangle_selectors_colors()
{
    for (int i = 0; i < m_triangle_selectors.size(); i++) {
        TriangleSelectorPatch* selector = dynamic_cast<TriangleSelectorPatch*>(m_triangle_selectors[i].get());
        int extruder_idx = m_volumes_extruder_idxs[i];
        int extruder_color_idx = std::max(0, extruder_idx - 1);
        std::vector<ColorRGBA> ebt_colors;
        ebt_colors.push_back(m_extruders_colors[extruder_color_idx]);
        ebt_colors.insert(ebt_colors.end(), m_extruders_colors.begin(), m_extruders_colors.end());
        selector->set_ebt_colors(ebt_colors);
    }
}

void GLGizmoSimplePaint::update_from_model_object(bool first_update)
{
    wxBusyCursor wait;

    // Extruder colors need to be reloaded before calling init_model_triangle_selectors to render painted triangles
    // using colors from loaded 3MF and not from printer profile in Slicer.
    if (int prev_extruders_count = int(m_extruders_colors.size());
        prev_extruders_count != wxGetApp().filaments_cnt() || get_extruders_colors() != m_extruders_colors)
        this->init_extruders_data();

    this->init_model_triangle_selectors();
}

void GLGizmoSimplePaint::tool_changed(wchar_t old_tool, wchar_t new_tool)
{
    if ((old_tool == ImGui::GapFillIcon && new_tool == ImGui::GapFillIcon) ||
        (old_tool != ImGui::GapFillIcon && new_tool != ImGui::GapFillIcon))
        return;

    for (auto& selector_ptr : m_triangle_selectors) {
        TriangleSelectorPatch* tsp = dynamic_cast<TriangleSelectorPatch*>(selector_ptr.get());
        tsp->set_filter_state(new_tool == ImGui::GapFillIcon);
    }
}

PainterGizmoType GLGizmoSimplePaint::get_painter_type() const
{
    return PainterGizmoType::MMU_SEGMENTATION;
}

// BBS
ColorRGBA GLGizmoSimplePaint::get_cursor_hover_color() const
{
    if (m_selected_extruder_idx < m_extruders_colors.size())
        return m_extruders_colors[m_selected_extruder_idx];
    else
        return m_extruders_colors[0];
}

void GLGizmoSimplePaint::on_set_state()
{
    GLGizmoPainterBase::on_set_state();

    if (get_state() == Off) {
        ModelObject* mo = m_c->selection_info()->model_object();
        if (mo) Slic3r::save_object_mesh(*mo);
        m_parent.post_event(SimpleEvent(EVT_GLCANVAS_FORCE_UPDATE));
    }
}

wxString GLGizmoSimplePaint::handle_snapshot_action_name(bool shift_down, GLGizmoPainterBase::Button button_down) const
{
    wxString action_name;
    if (shift_down)
        action_name = _L("Remove painted color");
    else {
        action_name        = GUI::format(_L("Painted using: Filament %1%"), m_selected_extruder_idx);
    }
    return action_name;
}

} // namespace Slic3r
