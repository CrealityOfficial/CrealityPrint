#ifndef slic3r_ModelInstance_hpp_
#define slic3r_ModelInstance_hpp_

#include "ModelCommon.hpp"
#include "Geometry.hpp"

namespace Slic3r {


class ModelObject;

 // A single instance of a ModelObject.
// Knows the affine transformation of an object.
class ModelInstance final : public ObjectBase
{
private:
    Geometry::Transformation m_transformation;
    Geometry::Transformation m_belt_transformation;
    Geometry::Transformation m_old_transformation;
    Geometry::Transformation m_assemble_transformation;
    Vec3d m_offset_to_assembly{ 0.0, 0.0, 0.0 };
    bool m_old_transformation_valid { false };
    bool m_assemble_initialized;

public:
    // flag showing the position of this instance with respect to the print volume (set by Print::validate() using ModelObject::check_instances_print_volume_state())
    ModelInstanceEPrintVolumeState print_volume_state;
    // Whether or not this instance is printable
    bool printable;
    bool auto_drop {true};
    bool use_loaded_id_for_label {false};
    int arrange_order = 0; // BBS
    size_t loaded_id = 0; // BBS

    size_t get_labeled_id() const
    {
        if (use_loaded_id_for_label && (loaded_id > 0))
            return loaded_id;
        else
            return id().id;
    }

    ModelObject* get_object() const { return this->object; }

    const Geometry::Transformation& get_transformation() const { 
            return m_transformation;
    }

    void restore_belt_transformation() {
         m_transformation = m_belt_transformation;
    }
    void restore_transformation() {
         m_transformation = m_old_transformation;
    }
    void set_transformation(const Geometry::Transformation& transformation) { m_transformation = transformation; }
    void set_belt_transformation(const Geometry::Transformation& transformation) { 
        m_belt_transformation = transformation; 
    }
    void set_old_transformation(const Geometry::Transformation& transformation) { 
        m_old_transformation = transformation; 
        m_old_transformation_valid = true;
    }
    bool has_old_transformation() const { return m_old_transformation_valid; }
    const Geometry::Transformation& get_old_transformation() const { return m_old_transformation; }

    const Geometry::Transformation& get_assemble_transformation() const { return m_assemble_transformation; }
    void set_assemble_transformation(const Geometry::Transformation& transformation) {
        m_assemble_initialized = true;
        m_assemble_transformation = transformation;
    }
    void set_assemble_from_transform(Transform3d& transform) {
        m_assemble_initialized = true;
        m_assemble_transformation.set_matrix(transform);
    }
    void set_assemble_offset(const Vec3d& offset) { m_assemble_transformation.set_offset(offset); }
    void rotate_assemble(double angle, const Vec3d& axis) {
        m_assemble_transformation.set_rotation(m_assemble_transformation.get_rotation() + Geometry::extract_euler_angles(Eigen::Quaterniond(Eigen::AngleAxisd(angle, axis)).toRotationMatrix()));
    }

    // BBS
    void set_offset_to_assembly(const Vec3d& offset) { m_offset_to_assembly = offset; }
    Vec3d get_offset_to_assembly() const { return m_offset_to_assembly; }

    Vec3d get_offset() const { return m_transformation.get_offset(); }
    double get_offset(Axis axis) const { return m_transformation.get_offset(axis); }

    void set_offset(const Vec3d& offset) { m_transformation.set_offset(offset); }
    void set_offset(Axis axis, double offset) { m_transformation.set_offset(axis, offset); }

    Vec3d get_rotation() const { return m_transformation.get_rotation(); }
    double get_rotation(Axis axis) const { return m_transformation.get_rotation(axis); }

    void set_rotation(const Vec3d& rotation) { m_transformation.set_rotation(rotation); }
    void set_rotation(Axis axis, double rotation) { m_transformation.set_rotation(axis, rotation); }

    // BBS
    void rotate(Matrix3d rotation_matrix) {
        auto rotation = m_transformation.get_rotation_matrix();
        rotation      = rotation_matrix * rotation;
        set_rotation(Geometry::Transformation(rotation).get_rotation());
    }

    Vec3d get_scaling_factor() const { return m_transformation.get_scaling_factor(); }
    double get_scaling_factor(Axis axis) const { return m_transformation.get_scaling_factor(axis); }

    void set_scaling_factor(const Vec3d& scaling_factor) { m_transformation.set_scaling_factor(scaling_factor); }
    void set_scaling_factor(Axis axis, double scaling_factor) { m_transformation.set_scaling_factor(axis, scaling_factor); }

    Vec3d get_mirror() const { return m_transformation.get_mirror(); }
    double get_mirror(Axis axis) const { return m_transformation.get_mirror(axis); }
    bool is_left_handed() const { return m_transformation.is_left_handed(); }

    void set_mirror(const Vec3d& mirror) { m_transformation.set_mirror(mirror); }
    void set_mirror(Axis axis, double mirror) { m_transformation.set_mirror(axis, mirror); }

    // To be called on an external mesh
    void transform_mesh(TriangleMesh* mesh, bool dont_translate = false) const;
    // Transform an external bounding box, thus the resulting bounding box is no more snug.
    BoundingBoxf3 transform_bounding_box(const BoundingBoxf3 &bbox, bool dont_translate = false) const;
    // Transform an external vector.
    Vec3d transform_vector(const Vec3d& v, bool dont_translate = false) const;
    // To be called on an external polygon. It does not translate the polygon, only rotates and scales.
    void transform_polygon(Polygon* polygon) const;

    const Transform3d& get_old_matrix() const { return m_old_transformation.get_matrix(); }

    const Transform3d& get_matrix() const { return m_transformation.get_matrix(); }
    Transform3d get_matrix_no_offset() const { return m_transformation.get_matrix_no_offset(); }

    bool is_printable() const;
    bool is_assemble_initialized() { return m_assemble_initialized; }

    //BBS
    double get_auto_brim_width(double deltaT, double adhension) const;
    double get_auto_brim_width() const;
    // BBS
    Polygon convex_hull_2d();
    void invalidate_convex_hull_2d();

    // Getting the input polygon for arrange
    // We use void* as input type to avoid including Arrange.hpp in Model.hpp.
    void get_arrange_polygon(void *arrange_polygon, const Slic3r::DynamicPrintConfig &config = Slic3r::DynamicPrintConfig()) const;

    // Apply the arrange result on the ModelInstance
    void apply_arrange_result(const Vec2d& offs, double rotation);

protected:
    friend class Print;
    friend class SLAPrint;
    friend class Model;
    friend class ModelObject;

    explicit ModelInstance(const ModelInstance &rhs) = default;
    void     set_model_object(ModelObject *model_object) { object = model_object; }

private:
    // Parent object, owning this instance.
    ModelObject* object;
    Polygon convex_hull; // BBS

    // Constructor, which assigns a new unique ID.
    explicit ModelInstance(ModelObject* object) : print_volume_state(ModelInstancePVS_Inside), printable(true), object(object), m_assemble_initialized(false) { assert(this->id().valid()); }
    // Constructor, which assigns a new unique ID.
    explicit ModelInstance(ModelObject *object, const ModelInstance &other) :
        m_transformation(other.m_transformation),
        m_belt_transformation(other.m_belt_transformation)
        , m_assemble_transformation(other.m_assemble_transformation)
        , m_offset_to_assembly(other.m_offset_to_assembly)
        , print_volume_state(ModelInstancePVS_Inside)
        , printable(other.printable)
        , auto_drop(other.auto_drop)
        , object(object)
        , m_assemble_initialized(false) { assert(this->id().valid() && this->id() != other.id()); }

    explicit ModelInstance(ModelInstance &&rhs) = delete;
    ModelInstance& operator=(const ModelInstance &rhs) = delete;
    ModelInstance& operator=(ModelInstance &&rhs) = delete;

	friend class cereal::access;
	friend class UndoRedo::StackImpl;
	// Used for deserialization, therefore no IDs are allocated.
	ModelInstance() : ObjectBase(-1), object(nullptr) { assert(this->id().invalid()); }
    // BBS. Add added members to archive.
    template<class Archive> void serialize(Archive& ar) {
        ar(m_transformation, print_volume_state, printable, m_assemble_transformation, m_offset_to_assembly, m_assemble_initialized, auto_drop);
    }
};

};

#endif /* slic3r_ModelInstance_hpp_ */
