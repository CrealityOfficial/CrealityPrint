#pragma once

#include "GLGizmoBase.hpp"
#include "GizmoObjectManipulation.hpp"


namespace Slic3r {
namespace GUI {

class GizmoObjectManipulation;

class GLGizmoClone : public GLGizmoBase
{
public:
    //GLGizmoMove3D(GLCanvas3D& parent, const std::string& icon_filename, unsigned int sprite_id);
    GLGizmoClone(GLCanvas3D& parent, const std::string& icon_filename, unsigned int sprite_id, GizmoObjectManipulation* obj_manipulation);
    virtual ~GLGizmoClone() = default;

    std::string get_tooltip() const override;
    float get_last_input_window_height() const { return m_last_input_window_height; }

    bool on_mouse(const wxMouseEvent &mouse_event) override;

    /// <summary>
    /// Detect reduction of move for wipetover on selection change
    /// </summary>
    void data_changed(bool is_serializing) override;
protected:
    bool on_init() override;
    std::string on_get_name() const override;
    bool on_is_activable() const override;
    void on_render() override;
    void on_register_raycasters_for_picking() override;
    void on_unregister_raycasters_for_picking() override;
    //BBS: GUI refactor: add object manipulation
    virtual void on_render_input_window(float x, float y, float bottom_limit, bool force_update_pos = false);

private:
    bool _render_object_clone_panel();

private:
    GizmoObjectManipulation* m_object_manipulation;
    int m_clone_num = 1;
    float m_last_input_window_height { 0.0f };
};



} // namespace GUI
} // namespace Slic3r
