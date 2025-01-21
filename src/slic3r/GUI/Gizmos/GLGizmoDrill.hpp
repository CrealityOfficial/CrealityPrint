#ifndef slic3r_GLGizmoDrill_hpp_
#define slic3r_GLGizmoDrill_hpp_

#include "GLGizmoBase.hpp"
#include "slic3r/GUI/GLModel.hpp"
#include "slic3r/GUI/GUI_Utils.hpp"
#include "slic3r/GUI/MeshUtils.hpp"
#include "slic3r/GUI/I18N.hpp"
#include "libslic3r/Model.hpp"

namespace Slic3r {

enum class ModelVolumeType : int;

namespace GUI {

enum class SLAGizmoEventType : unsigned char;

const double RADIUS_DEFAULT = 5.0;

class GLGizmoDrill : public GLGizmoBase
{
    enum class Shape : int { CIRCLE = 0, TRIANGLE = 1, SQUARE = 2 };

    enum class Direction : int {
        NORMAL_DIRECTION        = 0,
        PARALLEL_TO_PLATFORM    = 1,
        PERPENDICULAR_TO_SCREEN = 2,
    };
    int m_shape = {0};
    std::vector<std::string> m_shapes = {_u8L("Circle"), _u8L("Triangle"), _u8L("Square")};

    int m_direction = {0};
    std::vector<std::string> m_directions = {_u8L("Normal Direction"), _u8L("Parallel to Platform"), _u8L("Perpendicular to Screen")};
    
    std::map<std::string, std::string> m_labels_map;

    float m_label_width{0.f};
    float m_control_width{162.0};

    double m_radius = {RADIUS_DEFAULT};
    double m_depth  = {50};
    bool   m_one_layer_only = {false};
    struct VolumeInfo
    {
        ModelVolume* mv{nullptr};
        int          volume_idx{-1};
        Transform3d  trafo;
        void         reset()
        {
            mv         = nullptr;
            volume_idx = -1;
            trafo      = Transform3d::Identity();
        };
        template<class Archive> void serialize(Archive& ar) { ar(volume_idx, trafo); }
    };
    struct VolumeCacheItem
    {
        const ModelObject* object{ nullptr };
        const ModelInstance* instance{ nullptr };
        const ModelVolume* volume{ nullptr };
        Transform3d world_trafo;

        bool operator == (const VolumeCacheItem& other) const {
            return this->object == other.object && this->instance == other.instance && this->volume == other.volume &&
                this->world_trafo.isApprox(other.world_trafo);
        }
    };
    std::vector<VolumeCacheItem> m_volumes_cache;

    PickingModel m_cube;
    PickingModel m_triPrism;
    PickingModel m_cylinder;

    PickingModel& get_picking_model()
    {
        switch (m_shape) {
        case 0: return m_cylinder;
        case 1: return m_triPrism;
        case 2: return m_cube;
        default: break;
        }
        return m_cube;
    }

    TriangleMesh mesh_cube;
    TriangleMesh mesh_cylinder;
    TriangleMesh mesh_triPrism;

    TriangleMesh get_drill_mesh()
    {
        switch (m_shape) {
        case 0: return mesh_cylinder;
        case 1: return mesh_triPrism;
        case 2: return mesh_cube;
        default: break;
        }
        return mesh_cube;
    }

    std::string m_new_unit_string;

    void getDirection(Vec3d& normal);

    // Uses a standalone raycaster and not the shared one because of the
    // difference in how the mesh is updated
    std::unique_ptr<MeshRaycaster> m_raycaster;

    struct SceneRaycasterState
    {
        std::shared_ptr<SceneRaycasterItem> raycaster{ nullptr };
        bool state{true};

    };
    std::vector<SceneRaycasterState> m_scene_raycasters;

    Vec2d m_mouse_pos{ Vec2d::Zero() };

    void update_if_needed();

public:
    GLGizmoDrill(GLCanvas3D& parent, const std::string& icon_filename, unsigned int sprite_id);

    /// <summary>
    /// Apply rotation on select plane
    /// </summary>
    /// <param name="mouse_event">Keep information about mouse click</param>
    /// <returns>Return True when use the information otherwise False.</returns>
    bool on_mouse(const wxMouseEvent &mouse_event) override;

    bool gizmo_event(SLAGizmoEventType action, const Vec2d& mouse_position, bool shift_down, bool alt_down, bool control_down);

    bool wants_enter_leave_snapshots() const override { return true; }
    std::string get_gizmo_entering_text() const override { return _u8L("Entering Drill gizmo"); }
    std::string get_gizmo_leaving_text() const override { return _u8L("Leaving Drill gizmo"); }
    std::string get_action_snapshot_name() const override { return _u8L("Drill gizmo editing"); }

protected:
    bool on_init() override;
    std::string on_get_name() const override;
    bool on_is_activable() const override;
    void on_render() override;
    //void on_set_state() override;
    virtual CommonGizmosDataID on_get_requirements() const override;

    virtual void on_render_input_window(float x, float y, float bottom_limit) override;

private:
    // This map holds all translated description texts, so they can be easily referenced during layout calculations
    // etc. When language changes, GUI is recreated and this class constructed again, so the change takes effect.
    std::map<std::string, wxString> m_desc;

    void generate_new_volume(bool delete_input, const TriangleMesh& mesh_result);
    VolumeInfo m_src;

    bool render_combo(const std::string& label, const std::vector<std::string>& lines, int& selection_idx);
    bool render_reset_button(const std::string& label_id) const;
    void draw_cp_input_double(const std::string& label, double* v, double stride, const Vec2d& size, int flags = 0);
};

} // namespace GUI
} // namespace Slic3r

#endif // slic3r_GLGizmoDrill_hpp_
