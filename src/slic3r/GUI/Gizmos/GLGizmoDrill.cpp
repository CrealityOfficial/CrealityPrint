#include "GLGizmoDrill.hpp"
#include "slic3r/GUI/GLCanvas3D.hpp"
#include "slic3r/GUI/ImGuiWrapper.hpp"
#include "slic3r/GUI/GUI.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/GUI_ObjectList.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/Gizmos/GizmoObjectManipulation.hpp"
#include "slic3r/GUI/Selection.hpp"
#include "slic3r/GUI/NotificationManager.hpp"

#include "libslic3r/MeshBoolean.hpp"
#include "libslic3r/PresetBundle.hpp"

#include "imgui/imgui.h"
#include <imgui/imgui_internal.h>

#include <numeric>

#include <GL/glew.h>
#define MAX_SIZE std::string_view { "9999.99" }

namespace Slic3r {
namespace GUI {
static const Slic3r::ColorRGBA DRILL_TOOL_COLOR = {0.25f, 0.75f, 0.75f, 1.0f};
static const std::string warning_text = _u8L("Unable to perform drill operation on selected parts");

GLGizmoDrill::GLGizmoDrill(GLCanvas3D& parent, const std::string& icon_filename, unsigned int sprite_id)
    : GLGizmoBase(parent, icon_filename, sprite_id)
{
    // init mesh
    indexed_triangle_set cylinder = its_make_cylinder(1.0f, 1.0f, 2.0 * PI / 16.0);
    m_cylinder.model.init_from(cylinder);
    mesh_cylinder = TriangleMesh(cylinder);

    indexed_triangle_set cube = its_make_cylinder(1.0f, 1.0f, 2.0 * PI / 4.0);
    m_cube.model.init_from(cube);
    mesh_cube = TriangleMesh(cube);

    indexed_triangle_set triPrism = its_make_cylinder(1.0f, 1.0f, 2.0 * PI / 3.0);
    m_triPrism.model.init_from(triPrism);
    mesh_triPrism = TriangleMesh(triPrism);

    m_labels_map = {
        {"Shape", _u8L("Shape:")},
        {"Radius", _u8L("Radius:")},
        {"Depth", _u8L("Depth:")},
        {"Direction", _u8L("Direction:")},
        {"One layer only", _u8L("One layer only")},
        {"Reset radius", _u8L("Reset radius")},
    };

    m_new_unit_string = wxGetApp().app_config->get("use_inches") == "1" ? L("in") : L("mm");
}

bool GLGizmoDrill::on_init()
{
    m_shortcut_key = WXK_CONTROL_D;
    return true;
}

bool GLGizmoDrill::on_is_activable() const
{
    const Selection& selection = m_parent.get_selection();
    bool res = (wxGetApp().preset_bundle->printers.get_edited_preset().printer_technology() == ptSLA) ?
                selection.is_single_full_instance() :
                selection.is_single_full_instance() || selection.is_single_volume() || selection.is_single_modifier();
    if (res)
        res &= !selection.contains_sinking_volumes();

    return res;
}

std::string GLGizmoDrill::on_get_name() const { return _u8L("Drill"); }


void GLGizmoDrill::getDirection(Vec3d& normal)
{
    switch ((Direction) m_direction) {
    case Direction::NORMAL_DIRECTION: break;
    case Direction::PARALLEL_TO_PLATFORM:
        // set z to 0
        normal.z() = 0.0;
        break;
    case Direction::PERPENDICULAR_TO_SCREEN:
        const Camera& camera = wxGetApp().plater()->get_camera();
        normal               = -camera.get_dir_forward();
        break;
    }
}

bool GLGizmoDrill::on_mouse(const wxMouseEvent& mouse_event)
{
    m_mouse_pos = {double(mouse_event.GetX()), double(mouse_event.GetY())};

    if (mouse_event.LeftDown()) {
        // let the event pass through to allow panning/rotating the 3D scene
        if (mouse_event.CmdDown())
            return false;

        return gizmo_event(SLAGizmoEventType::LeftDown, m_mouse_pos, mouse_event.ShiftDown(), mouse_event.AltDown(), false);
    }
    return false;
}

bool GLGizmoDrill::gizmo_event(SLAGizmoEventType action, const Vec2d& mouse_position, bool shift_down, bool alt_down, bool control_down)
{
    if (action != SLAGizmoEventType::LeftDown)
        return false;

    const ModelObject* mo = m_c->selection_info()->model_object();
    if (mo == nullptr)
        return false;

    double depth = m_depth;
    {
    const ModelInstance*     mi = mo->instances[m_parent.get_selection().get_instance_idx()];
    std::vector<Transform3d> trafo_matrices;
    for (const ModelVolume* mv : mo->volumes) {
        trafo_matrices.emplace_back(mi->get_transformation().get_matrix() * mv->get_matrix());
    }

    const Camera& camera                       = wxGetApp().plater()->get_camera();
    Vec3f         normal                       = Vec3f::Zero();
    Vec3f         hit                          = Vec3f::Zero();
    size_t        facet                        = 0;
    Vec3f         closest_hit                  = Vec3f::Zero();
    Vec3f         closest_normal               = Vec3f::Zero();
    double        closest_hit_squared_distance = std::numeric_limits<double>::max();
    int           closest_hit_mesh_id          = -1;
    double        drillDist                    = 0;

    // Cast a ray on all meshes, pick the closest hit and save it for the respective mesh
    for (int mesh_id = 0; mesh_id < int(trafo_matrices.size()); ++mesh_id) {
        MeshRaycaster mesh_raycaster = MeshRaycaster(mo->volumes[mesh_id]->mesh_ptr());
        if (mesh_raycaster.unproject_on_mesh(m_mouse_pos, trafo_matrices[mesh_id], camera, hit, normal,
                                                m_c->object_clipper()->get_clipping_plane(), &facet)) {
            // Is this hit the closest to the camera so far?
            double hit_squared_distance = (camera.get_position() - trafo_matrices[mesh_id] * hit.cast<double>()).squaredNorm();
            if (hit_squared_distance < closest_hit_squared_distance) {
                closest_hit_squared_distance = hit_squared_distance;
                closest_hit_mesh_id          = mesh_id;
                closest_hit                  = hit;
                closest_normal               = normal;

                {
                Vec3f normalSec = Vec3f::Zero();
                Vec3f hitSec    = Vec3f::Zero();
                // get second hit by MeshRaycaster::second_closest_hit
                if (m_one_layer_only && mesh_raycaster.second_closest_hit(m_mouse_pos, trafo_matrices[mesh_id], camera, hitSec, normalSec,
                                                      m_c->object_clipper()->get_clipping_plane())) {
                    depth = (hit - hitSec).norm();
                }
                }
            }
        }
    }

    if (closest_hit == Vec3f::Zero() && closest_normal == Vec3f::Zero())
        return false;
    m_src.trafo      = mo->volumes[closest_hit_mesh_id]->get_matrix();
    m_src.volume_idx = closest_hit_mesh_id;
    m_src.mv         = mo->volumes[closest_hit_mesh_id];

    const Selection& selection = m_parent.get_selection();
    if (selection.is_empty())
        return false;

    if (m_state != On || m_volumes_cache.empty())
        return false;
    }

    const Camera& camera = wxGetApp().plater()->get_camera();
    Vec3f      position_on_model;
    Vec3f      normal_on_model;
    size_t     model_facet_idx;
    const bool mouse_on_object = m_raycaster->unproject_on_mesh(m_mouse_pos, Transform3d::Identity(), camera, position_on_model,
                                                                normal_on_model, nullptr, &model_facet_idx);
    if (!mouse_on_object)
        return false;

    Vec3d pos = {position_on_model.x(), position_on_model.y(), position_on_model.z()};
    Vec3d normal = {normal_on_model.x(), normal_on_model.y(), normal_on_model.z()};


    getDirection(normal);
    if (normal.norm() <= epsilon())
        return true;
    float             depth_scale_factor    = 100.f;
    const Transform3d feature_matrix = Geometry::translation_transform(pos + normal * (depth_scale_factor)) *
                                    Eigen::Quaternion<double>::FromTwoVectors(Vec3d::UnitZ(), -normal) *
                                    Geometry::scale_transform({(double) m_radius, (double) m_radius, (normal_on_model).norm() * (depth + depth_scale_factor)});
    // boolean mesh
    // get selection volume
    const ModelVolume* selected_volumes = m_src.mv;

    Transform3d selected_volumes_matrix = Transform3d::Identity();
    for (const auto& vol : m_volumes_cache) {
        if (vol.volume == selected_volumes) {
            selected_volumes_matrix = vol.world_trafo;
            break;
        }
    }

    TriangleMesh        temp_src_mesh{selected_volumes->mesh().its};

    TriangleMesh              temp_tool_mesh(get_drill_mesh());
    std::vector<TriangleMesh> temp_mesh_resuls;
    const Transform3d         src_matrix = selected_volumes->get_transformation().get_matrix();
    temp_src_mesh.transform(selected_volumes_matrix);
    temp_tool_mesh.transform(feature_matrix);
    Slic3r::MeshBoolean::mcut::make_boolean(temp_src_mesh, temp_tool_mesh, temp_mesh_resuls, "A_NOT_B");
    if (temp_mesh_resuls.size() != 0) {
        int active_inst = m_c->selection_info()->get_active_instance();
        const Transform3d instance_matrix = mo->instances[active_inst]->get_transformation().get_matrix_no_offset();
        temp_mesh_resuls.front().transform(instance_matrix.inverse());
        generate_new_volume(true, *temp_mesh_resuls.begin());
        wxGetApp().notification_manager()->close_plater_warning_notification(warning_text);
    }
    return mouse_on_object;
}

void GLGizmoDrill::update_if_needed()
{
    auto do_update = [this](const std::vector<VolumeCacheItem>& volumes_cache, const Selection& selection) {
        TriangleMesh composite_mesh;
        for (const auto& vol : volumes_cache) {

            TriangleMesh volume_mesh = vol.volume->mesh();
            volume_mesh.transform(vol.world_trafo);

            if (vol.world_trafo.matrix().determinant() < 0.0)
                volume_mesh.flip_triangles();

            composite_mesh.merge(volume_mesh);
        }

        m_raycaster.reset(new MeshRaycaster(std::make_shared<const TriangleMesh>(composite_mesh)));
        m_volumes_cache = volumes_cache;
    };

    const Selection& selection = m_parent.get_selection();
    if (selection.is_empty())
        return;

    const Selection::IndicesList& idxs = selection.get_volume_idxs();
    std::vector<VolumeCacheItem>  volumes_cache;
    volumes_cache.reserve(idxs.size());
    for (unsigned int idx : idxs) {
        const GLVolume* v          = selection.get_volume(idx);
        const int       volume_idx = v->volume_idx();
        if (volume_idx < 0)
            continue;
        const ModelObject*    obj  = selection.get_model()->objects[v->object_idx()];
        const ModelInstance*  inst = obj->instances[v->instance_idx()];
        const ModelVolume*    vol  = obj->volumes[volume_idx];
        if (vol->is_negative_volume() || vol->is_modifier())
            continue;

        const VolumeCacheItem item = {obj, inst, vol,
                                      Geometry::translation_transform(selection.get_first_volume()->get_sla_shift_z() * Vec3d::UnitZ()) *
                                          inst->get_matrix() * vol->get_matrix()};
        volumes_cache.emplace_back(item);
    }

    if (m_state != On || volumes_cache.empty())
        return;

    if (m_volumes_cache != volumes_cache)
        do_update(volumes_cache, selection);
}

void GLGizmoDrill::on_render()
{
    update_if_needed();

    const Camera& camera = wxGetApp().plater()->get_camera();
    const float inv_zoom = (float) camera.get_inv_zoom();

    if (!m_raycaster)
    {
        return;
    }

    Vec3f position_on_model;
    Vec3f normal_on_model;
    size_t model_facet_idx;
    const bool mouse_on_object = m_raycaster->unproject_on_mesh(m_mouse_pos, Transform3d::Identity(), camera, position_on_model,
                                                                normal_on_model, nullptr, &model_facet_idx);
    if (!mouse_on_object)
        return;

    GLShaderProgram* shader = wxGetApp().get_shader("gouraud_light");
    if (shader == nullptr)
        return;

    shader->start_using();
    shader->set_uniform("projection_matrix", camera.get_projection_matrix());

    glsafe(::glEnable(GL_DEPTH_TEST));
    const bool old_cullface = ::glIsEnabled(GL_CULL_FACE);
    glsafe(::glDisable(GL_CULL_FACE));

    const Transform3d& view_matrix = camera.get_view_matrix();

    auto set_matrix_uniforms = [shader, &view_matrix](const Transform3d& model_matrix) {
        const Transform3d view_model_matrix = view_matrix * model_matrix;
        shader->set_uniform("view_model_matrix", view_model_matrix);
        const Matrix3d view_normal_matrix = view_matrix.matrix().block(0, 0, 3, 3) *
                                            model_matrix.matrix().block(0, 0, 3, 3).inverse().transpose();
        shader->set_uniform("view_normal_matrix", view_normal_matrix);
    };

    Vec3d pos = {position_on_model.x(), position_on_model.y(), position_on_model.z()};
    Vec3d normal = {normal_on_model.x(), normal_on_model.y(), normal_on_model.z()};
    getDirection(normal);
    if (normal.norm() <= epsilon()) {
        return;
    }

    const Transform3d drill_matrix = Geometry::translation_transform(pos + normal * 10) *
                                    Eigen::Quaternion<double>::FromTwoVectors(Vec3d::UnitZ(), -normal) *
                                     Geometry::scale_transform({(double) m_radius, (double) m_radius, (normal_on_model).norm() * (m_depth + 10)});
    set_matrix_uniforms(drill_matrix);
    get_picking_model().model.set_color(DRILL_TOOL_COLOR);
    get_picking_model().model.render();
}

CommonGizmosDataID GLGizmoDrill::on_get_requirements() const
{
    return CommonGizmosDataID(int(CommonGizmosDataID::SelectionInfo) | int(CommonGizmosDataID::InstancesHider) |
                              int(CommonGizmosDataID::Raycaster) | int(CommonGizmosDataID::ObjectClipper));
}

void GLGizmoDrill::on_render_input_window(float x, float y, float bottom_limit)
{
    bool is_dark = wxGetApp().dark_mode();
    float label_width = 0.0f;
    for (const auto& item : m_labels_map) {
        const float width = m_imgui->calc_text_size(item.second).x;
        if (label_width < width)
            label_width = width;
    }
    label_width += m_imgui->scaled(1.f);
    label_width += ImGui::GetStyle().WindowPadding.x;

    m_label_width = label_width;
    
    ImGuiWrapper::push_toolbar_style(m_parent.get_scale());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0, 4.0));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);

    float  item_height        = ImGui::GetFrameHeight();
    ImVec2 updown_button_size = ImVec2(15.0f * m_parent.get_scale(), item_height / 2.0);
    float  space_size         = m_imgui->get_style_scaling() * 14.0f;
    float  unit_size          = MAX(162,ImGuiWrapper::calc_text_size(MAX_SIZE).x +
                                ImGuiWrapper::calc_text_size(m_new_unit_string).x + space_size +
                                updown_button_size.x);
    float end_text_size = ImGuiWrapper::calc_text_size(this->m_new_unit_string).x;

    GizmoImguiSetNextWIndowPos(x, y, ImGuiCond_Always, 0.0f, 0.0f);
    
    GizmoImguiBegin(get_name(), ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);

    const bool is_changed = render_combo(m_labels_map["Shape"], m_shapes, m_shape);

    ImGui::AlignTextToFramePadding();
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
    ImGui::PushItemWidth(m_label_width);
    m_imgui->text(m_labels_map["Radius"]);
    ImGui::SameLine(m_label_width);
    ImGui::PushItemWidth(m_control_width);

    draw_cp_input_double("##Radius:", &m_radius, 0.05, Vec2d(unit_size, item_height));
    m_radius == RADIUS_DEFAULT ? ImGui::Dummy(ImVec2(0, 0)): ImGui::SameLine(0, 6);
    if (render_reset_button("reset_radius")) {
        m_radius = RADIUS_DEFAULT;
    }
    ImGui::AlignTextToFramePadding();
    ImGui::PushItemWidth(m_label_width);
    if (m_one_layer_only) {
        ImGui::PushStyleColor(ImGuiCol_Text, is_dark ? ImVec4(1.0f, 1.0f, 1.0f, 0.3f) : ImVec4{ 214 / 255.0f, 214 / 255.0f, 220 / 255.0f, 0.5f });
    }
    m_imgui->text(m_labels_map["Depth"]);
    if (m_one_layer_only) {
        ImGui::PopStyleColor();
    }
    ImGui::SameLine(m_label_width);
    ImGui::PushItemWidth(m_control_width);
    draw_cp_input_double("##Depth:", &m_depth, 0.05, Vec2d(unit_size, item_height), m_one_layer_only ? ImGuiInputTextFlags_ReadOnly : 0);
    ImGui::PopStyleVar();

    ImGui::Dummy(ImVec2(0, 0));
    const bool is_changed1 = render_combo(m_labels_map["Direction"], m_directions, m_direction);

    ImGui::Dummy(ImVec2(0, 0));
    ImGui::SameLine(m_label_width);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, is_dark ? ImVec4(1.0f, 1.0f, 1.0f, 0.3f) : ImVec4{ 214 / 255.0f, 214 / 255.0f, 220 / 255.0f, 1.0f });
    m_imgui->bbl_checkbox(_L("One layer only"), m_one_layer_only);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    GizmoImguiEnd();
    ImGui::PopStyleVar(3);

    ImGuiWrapper::pop_toolbar_style();
}


void GLGizmoDrill::generate_new_volume(bool delete_input, const TriangleMesh& mesh_result)
{
    wxGetApp().plater()->take_snapshot("Mesh Drill");
    const Selection& selection         = m_parent.get_selection();
    //const ModelVolume* selected_volumes  = get_selected_volume(selection);
    const ModelVolume* selected_volumes  = m_src.mv;
    ModelObject*       curr_model_object = selected_volumes->get_object();

    const ModelVolume* old_volume = selected_volumes;

    // generate new volume
    ModelVolume* new_volume = curr_model_object->add_volume(std::move(mesh_result));

    // assign to new_volume from old_volume
    new_volume->name = old_volume->name;
    new_volume->set_new_unique_id();
    new_volume->config.apply(old_volume->config);
    new_volume->set_type(old_volume->type());
    new_volume->set_material_id(old_volume->material_id());
    new_volume->set_offset(old_volume->get_transformation().get_offset());
    // Vec3d translate_z = { 0,0, (new_volume->source.mesh_offset - old_volume->source.mesh_offset).z() };
    // new_volume->translate(new_volume->get_transformation().get_matrix_no_offset() * translate_z);
    // new_volume->supported_facets.assign(old_volume->supported_facets);
    // new_volume->seam_facets.assign(old_volume->seam_facets);
    // new_volume->mmu_segmentation_facets.assign(old_volume->mmu_segmentation_facets);

    // delete old_volume
    std::swap(curr_model_object->volumes[m_src.volume_idx], curr_model_object->volumes.back());
    curr_model_object->delete_volume(curr_model_object->volumes.size() - 1);

    wxGetApp().plater()->update();
    wxGetApp().obj_list()->select_item([this, new_volume]() {
        wxDataViewItem sel_item;

        wxDataViewItemArray items = wxGetApp().obj_list()->reorder_volumes_and_get_selection(m_parent.get_selection().get_object_idx(),
                                                                                             [new_volume](const ModelVolume* volume) {
                                                                                                 return volume == new_volume;
                                                                                             });
        if (!items.IsEmpty())
            sel_item = items.front();

        return sel_item;
    });
}

bool GLGizmoDrill::render_combo(const std::string& label, const std::vector<std::string>& lines, int& selection_idx)
{
    ImGui::AlignTextToFramePadding();
    bool is_dark = wxGetApp().dark_mode();
    ImVec4 borderColor_dark = { 110 / 255.f, 110 / 255.f, 114 / 255.f ,1.0f };
    ImVec4 borderColor_light = { 214 / 255.0f,214 / 255.0f, 220 / 255.0f, 1.0f };
    ImGui::PushStyleColor(ImGuiCol_Border, is_dark ? borderColor_dark : borderColor_light);
    ImGuiWrapper::push_combo_style(m_parent.get_scale());
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
    const bool is_changed    = m_imgui->combo(label, lines, selection_idx, 0, m_label_width, m_control_width);
    ImGui::PopStyleVar();
    ImGuiWrapper::pop_combo_style();
    ImGui::PopStyleColor();
    return is_changed;
}

bool GLGizmoDrill::render_reset_button(const std::string& label_id) const
{
    if (m_radius == RADIUS_DEFAULT)
        return false;
    const ImGuiStyle& style = ImGui::GetStyle();
    //ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {1, style.ItemSpacing.y});

    ImGui::PushStyleColor(ImGuiCol_Button, {0.25f, 0.25f, 0.25f, 0.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.4f, 0.4f, 0.4f, 0.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0.4f, 0.4f, 0.4f, 0.0f});

    const bool revert = m_imgui->button(wxString(ImGui::RevertBtn) + "##" + label_id);

    ImGui::PopStyleColor(3);

    if (ImGui::IsItemHovered())
        m_imgui->tooltip(_u8L("Reset radius").c_str(), ImGui::GetFontSize() * 20.0f);

    ImGui::PopStyleVar();

    return revert;
}

void GLGizmoDrill::draw_cp_input_double(const std::string& label, double* v, double stride, const Vec2d& size, int disable)
{
    bool is_dark = wxGetApp().dark_mode();
    ImVec4 borderColor_dark = { 110 / 255.f, 110 / 255.f, 114 / 255.f ,1.0f };
    ImVec4 borderColor_light = { 214 / 255.0f,214 / 255.0f, 220 / 255.0f, 1.0f };
    ImGui::PushStyleColor(ImGuiCol_Button, {0.25f, 0.25f, 0.25f, 0.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.4f, 0.4f, 0.4f, 0.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0.4f, 0.4f, 0.4f, 0.0f});
    ImVec2 bd = ImVec2(0, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, bd);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, bd);

    if (disable) {
        ImGui::PushStyleColor(ImGuiCol_Text, is_dark ? ImVec4(1.0f, 1.0f, 1.0f, 0.3f) : ImVec4{ 214 / 255.0f, 214 / 255.0f, 220 / 255.0f, 0.5f });
    }

    {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2      pos       = ImGui::GetCursorScreenPos();
        ImVec2 pos_max = ImVec2(pos.x + size.x(), pos.y + size.y());
        draw_list->AddRect(pos, pos_max, ImGui::GetColorU32(is_dark ? borderColor_dark : borderColor_light), 5.0f, ImDrawFlags_RoundCornersAll);
    }
    const ImVec2 updown_button_size = ImVec2(15.0f * m_parent.get_scale(), size.y() * 0.5);
    const char*  text               = m_new_unit_string.c_str();
    float        text_size          = ImGui::CalcTextSize(text).x;
    float        space              = 5.0f;
    ImGui::PushItemWidth(size.x() - updown_button_size.x - text_size - space * 3.0);
    ImGui::BBLInputDouble(label.c_str(), v, 0.0f, 0.0f, "%.2f", disable);
    ImGui::SameLine(0, space);
    ImGui::Text(text);
    ImGui::SameLine(0, space);
    {
        ImGui::BeginGroup();

        ImVec2 pos   = ImGui::GetCursorScreenPos();
        bool   hover = ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + updown_button_size.x, pos.y + updown_button_size.y));

        if (hover)
            ImGui::PushStyleColor(ImGuiCol_Text, ImGuiWrapper::COL_CREALITY);

        if (ImGui::ArrowButtonEX_CP((label + "##up").c_str(), ImGuiDir_Up, updown_button_size, disable)) {
            *v += stride;
        }
        if (hover)
            ImGui::PopStyleColor();

        pos   = ImGui::GetCursorScreenPos();
        hover = ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + updown_button_size.x, pos.y + updown_button_size.y));

        if (hover)
            ImGui::PushStyleColor(ImGuiCol_Text, ImGuiWrapper::COL_CREALITY);

        if (ImGui::ArrowButtonEX_CP((label + "##down").c_str(), ImGuiDir_Down, updown_button_size, disable)) {
            *v -= stride;
        }

        if (hover)
            ImGui::PopStyleColor();

        
        ImGui::EndGroup();
    }
    if (disable) {
        ImGui::PopStyleColor();
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);
}


}
}

