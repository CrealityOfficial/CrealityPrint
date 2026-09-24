#ifndef slic3r_ModelObject_hpp_
#define slic3r_ModelObject_hpp_

#include "ModelCommon.hpp"
#include "Slicing.hpp"
#include "SLA/SupportPoint.hpp"
#include "SLA/Hollowing.hpp"
#include "BrimEarsPoint.hpp"
#include "Format/bbs_3mf.hpp"

namespace Slic3r {

class Model;
class ModelObject final : public ObjectBase
{
public:
    std::string             name;
    //BBS: add module name for assemble
    std::string             module_name;
    std::string             input_file;    // XXX: consider fs::path
    // Instances of this ModelObject. Each instance defines a shift on the print bed, rotation around the Z axis and a uniform scaling.
    // Instances are owned by this ModelObject.
    ModelInstancePtrs       instances;
    // Printable and modifier volumes, each with its material ID and a set of override parameters.
    // ModelVolumes are owned by this ModelObject.
    ModelVolumePtrs         volumes;
    // Configuration parameters specific to a single ModelObject, overriding the global Slic3r settings.
    ModelConfigObject 		config;
    // Variation of a layer thickness for spans of Z coordinates + optional parameter overrides.
    t_layer_config_ranges   layer_config_ranges;
    // Profile of increasing z to a layer height, to be linearly interpolated when calculating the layers.
    // The pairs of <z, layer_height> are packed into a 1D array.
    LayerHeightProfile      layer_height_profile;
    // Whether or not this object is printable
    bool                    printable { true };

    // This vector holds position of selected support points for SLA. The data are
    // saved in mesh coordinates to allow using them for several instances.
    // The format is (x, y, z, point_size, supports_island)
    sla::SupportPoints      sla_support_points;
    // To keep track of where the points came from (used for synchronization between
    // the SLA gizmo and the backend).
    sla::PointsStatus       sla_points_status = sla::PointsStatus::NoPoints;

    // Holes to be drilled into the object so resin can flow out
    sla::DrainHoles         sla_drain_holes;

    BrimPoints              brim_points;

    // Connectors to be added into the object before cut and are used to create a solid/negative volumes during a cut perform
    CutConnectors           cut_connectors;
    CutObjectBase           cut_id;
    std::vector<const ModelVolume*> const_volumes() const {return std::vector<const ModelVolume*>(volumes.begin(), volumes.end());}
    /* This vector accumulates the total translation applied to the object by the
        center_around_origin() method. Callers might want to apply the same translation
        to new volumes before adding them to this object in order to preserve alignment
        when user expects that. */
    Vec3d                   origin_translation;

    // BBS: save for compare with new load volumes
    std::vector<ObjectID>   volume_ids;

    // if this ModelObject is loaded from 3mf file, the "from_loaded_id" is related to the Object id in the "3dmodel.model" file
    int from_loaded_id = -1;
    
    // BBS: UUID from 3MF file (p:UUID attribute)
    std::string             uuid;

    Model*                  get_model() { return m_model; }
    const Model*            get_model() const { return m_model; }
    // BBS: production extension
    int                     get_backup_id() const;
    template<typename T> const T* get_config_value(const DynamicPrintConfig& global_config, const std::string& config_option) {
        if (config.has(config_option))
            return static_cast<const T*>(config.option(config_option));
        else
            return global_config.option<T>(config_option);
    }

    ModelVolume*            add_volume(const TriangleMesh &mesh);
    ModelVolume*            add_volume(TriangleMesh &&mesh, ModelVolumeType type = ModelVolumeType::MODEL_PART,
                                       bool modify_to_center_geometry = true, bool backup = true);
    ModelVolume*            add_volume(const ModelVolume &volume, ModelVolumeType type = ModelVolumeType::INVALID);
    ModelVolume*            add_volume(const ModelVolume &volume, TriangleMesh &&mesh);
    ModelVolume*            add_volume_with_shared_mesh(const ModelVolume &other, ModelVolumeType type = ModelVolumeType::MODEL_PART, bool backup = true);
    void                    delete_volume(size_t idx);
    void                    clear_volumes();
    void                    sort_volumes(bool full_sort);
    bool                    is_multiparts() const { return volumes.size() > 1; }
    // Checks if any of object volume is painted using the fdm support painting gizmo.
    bool                    is_fdm_support_painted() const;
    // Checks if any of object volume is painted using the seam painting gizmo.
    bool                    is_seam_painted() const;
    // Checks if any of object volume is painted using the multi-material painting gizmo.
    bool                    is_mm_painted() const;
    // Checks if any of object volume is painted using the fuzzy skin painting gizmo.
    bool                    is_fuzzy_skin_painted() const;
    // This object may have a varying layer height by painting or by a table.
    // Even if true is returned, the layer height profile may be "flat" with no difference to default layering.
    bool                    has_custom_layering() const
        { return ! this->layer_config_ranges.empty() || ! this->layer_height_profile.empty(); }

    ModelInstance*          add_instance();
    ModelInstance*          add_instance(const ModelInstance &instance);
    ModelInstance*          add_instance(const Vec3d &offset, const Vec3d &scaling_factor, const Vec3d &rotation, const Vec3d &mirror);
    void                    delete_instance(size_t idx);
    void                    delete_last_instance();
    void                    clear_instances();

    // Returns the bounding box of the transformed instances. This bounding box is approximate and not snug, it is being cached.
    const BoundingBoxf3&    bounding_box_approx() const;
    // Returns an exact bounding box of the transformed instances. The result it is being cached.
    const BoundingBoxf3&    bounding_box_exact() const;

    const BoundingBoxf3& belt_bounding_box_exact() const;


    Vec3d get_instances_min_offset() const;
    // Return minimum / maximum of a printable object transformed into the world coordinate system.
    // All instances share the same min / max Z.
    double                  min_z() const;
    double                  max_z() const;
    double                  min_y() const;
    double                  current_min_y() const;
    double                  max_y() const;
    double                  current_max_y() const;
    double                  depth() const;
    BoundingBoxf3           belt_box();

    void invalidate_bounding_box() {
        m_bounding_box_approx_valid     = false;
        m_bounding_box_exact_valid      = false;
        m_min_max_z_valid               = false;
        m_raw_bounding_box_valid        = false;
        m_raw_mesh_bounding_box_valid   = false;
    }

    // A mesh containing all transformed instances of this object.
    TriangleMesh mesh() const;
    // Non-transformed (non-rotated, non-scaled, non-translated) sum of non-modifier object volumes.
    // Currently used by ModelObject::mesh() and to calculate the 2D envelope for 2D plater.
    TriangleMesh raw_mesh() const;
    // The same as above, but producing a lightweight indexed_triangle_set.
    indexed_triangle_set raw_indexed_triangle_set() const;
    // A transformed snug bounding box around the non-modifier object volumes, without the translation applied.
    // This bounding box is only used for the actual slicing.
    const BoundingBoxf3& raw_bounding_box() const;
    // A snug bounding box around the transformed non-modifier object volumes.
    BoundingBoxf3 instance_bounding_box(size_t instance_idx, bool dont_translate = false) const;
    BoundingBoxf3 instance_bounding_box(const ModelInstance& instance, bool dont_translate = false) const;
    BoundingBoxf3 instance_belt_bounding_box(size_t instance_idx, bool dont_translate = false) const;
    BoundingBoxf3 instance_belt_bounding_box(const ModelInstance& instance, bool dont_translate = false) const;

	// A snug bounding box of non-transformed (non-rotated, non-scaled, non-translated) sum of non-modifier object volumes.
	const BoundingBoxf3& raw_mesh_bounding_box() const;
	// A snug bounding box of non-transformed (non-rotated, non-scaled, non-translated) sum of all object volumes.
    BoundingBoxf3 full_raw_mesh_bounding_box() const;

    //BBS: add instance convex hull bounding box
    BoundingBoxf3 instance_convex_hull_bounding_box(size_t instance_idx, bool dont_translate = false) const;
    BoundingBoxf3 instance_convex_hull_bounding_box(const ModelInstance* instance, bool dont_translate = false) const;

    // Calculate 2D convex hull of of a projection of the transformed printable volumes into the XY plane.
    // This method is cheap in that it does not make any unnecessary copy of the volume meshes.
    // This method is used by the auto arrange function.
    Polygon       convex_hull_2d(const Transform3d &trafo_instance) const;

    void center_around_origin(bool include_modifiers = true);
    void ensure_on_bed(bool allow_negative_z = false);

    void translate_instances(const Vec3d& vector);
    void translate_instance(size_t instance_idx, const Vec3d& vector);
    void translate(const Vec3d &vector) { this->translate(vector(0), vector(1), vector(2)); }
    void translate(double x, double y, double z);
    void scale(const Vec3d &versor);
    void scale(const double s) { this->scale(Vec3d(s, s, s)); }
    void scale(double x, double y, double z) { this->scale(Vec3d(x, y, z)); }
    /// Scale the current ModelObject to fit by altering the scaling factor of ModelInstances.
    /// It operates on the total size by duplicating the object according to all the instances.
    ///param size Sizef3 the size vector
    void scale_to_fit(const Vec3d &size);
    void rotate(double angle, Axis axis);
    void rotate(double angle, const Vec3d& axis);
    void mirror(Axis axis);

    // This method could only be called before the meshes of this ModelVolumes are not shared!
    void scale_mesh_after_creation(const float scale);
    void convert_units(ModelObjectPtrs&new_objects, ConversionType conv_type, std::vector<int> volume_idxs);

    size_t materials_count() const;
    size_t facets_count() const;
    size_t parts_count() const;
    // invalidate cut state for this object and its connectors/volumes
    void invalidate_cut();
    // delete volumes which are marked as connector for this object
    void delete_connectors();
    void clone_for_cut(ModelObject **obj);

    void split(ModelObjectPtrs*new_objects);
    void merge();

    // BBS: Boolean opts - Musang King
    bool make_boolean(ModelObject *cut_object, const std::string &boolean_opts);

    ModelObjectPtrs merge_volumes(std::vector<int>& vol_indeces);//BBS
    // Support for non-uniform scaling of instances. If an instance is rotated by angles, which are not multiples of ninety degrees,
    // then the scaling in world coordinate system is not representable by the Geometry::Transformation structure.
    // This situation is solved by baking in the instance transformation into the mesh vertices.
    // Rotation and mirroring is being baked in. In case the instance scaling was non-uniform, it is baked in as well.
    void bake_xy_rotation_into_meshes(size_t instance_idx);

    double get_instance_min_z(size_t instance_idx) const;
    double get_instance_max_z(size_t instance_idx) const;

    // Print object statistics to console.
    void print_info() const;

    std::string get_export_filename() const;

    // Get full stl statistics for all object's meshes
    TriangleMeshStats get_object_stl_stats() const;
    // Get count of errors in the mesh( or all object's meshes, if volume index isn't defined)
    int         get_repaired_errors_count(const int vol_idx = -1) const;

    // Detect if object has at least one solid mash
    bool has_solid_mesh() const;
    bool is_cut() const { return cut_id.id().valid(); }
    bool has_connectors() const;
private:
    friend class Model;
    // This constructor assigns new ID to this ModelObject and its config.
    explicit ModelObject(Model* model) : m_model(model), origin_translation(Vec3d::Zero())
    {
        assert(this->id().valid());
        assert(this->config.id().valid());
        assert(this->layer_height_profile.id().valid());
    }
    explicit ModelObject(int) : ObjectBase(-1), config(-1), layer_height_profile(-1), origin_translation(Vec3d::Zero())
    {
        assert(this->id().invalid());
        assert(this->config.id().invalid());
        assert(this->layer_height_profile.id().invalid());
    }
	~ModelObject();
	void assign_new_unique_ids_recursive() override;

    // To be able to return an object from own copy / clone methods. Hopefully the compiler will do the "Copy elision"
    // (Omits copy and move(since C++11) constructors, resulting in zero - copy pass - by - value semantics).
    ModelObject(const ModelObject &rhs) : ObjectBase(-1), config(-1), layer_height_profile(-1), m_model(rhs.m_model) {
    	assert(this->id().invalid());
        assert(this->config.id().invalid());
        assert(this->layer_height_profile.id().invalid());
        assert(rhs.id() != rhs.config.id());
        assert(rhs.id() != rhs.layer_height_profile.id());
    	this->assign_copy(rhs);
    	assert(this->id().valid());
        assert(this->config.id().valid());
        assert(this->layer_height_profile.id().valid());
        assert(this->id() != this->config.id());
        assert(this->id() != this->layer_height_profile.id());
    	assert(this->id() == rhs.id());
        assert(this->config.id() == rhs.config.id());
        assert(this->layer_height_profile.id() == rhs.layer_height_profile.id());
    }
    explicit ModelObject(ModelObject &&rhs) : ObjectBase(-1), config(-1), layer_height_profile(-1) {
    	assert(this->id().invalid());
        assert(this->config.id().invalid());
        assert(this->layer_height_profile.id().invalid());
        assert(rhs.id() != rhs.config.id());
        assert(rhs.id() != rhs.layer_height_profile.id());
    	this->assign_copy(std::move(rhs));
    	assert(this->id().valid());
        assert(this->config.id().valid());
        assert(this->layer_height_profile.id().valid());
        assert(this->id() != this->config.id());
        assert(this->id() != this->layer_height_profile.id());
    	assert(this->id() == rhs.id());
        assert(this->config.id() == rhs.config.id());
        assert(this->layer_height_profile.id() == rhs.layer_height_profile.id());
    }
    ModelObject& operator=(const ModelObject &rhs) {
    	this->assign_copy(rhs);
    	m_model = rhs.m_model;
    	assert(this->id().valid());
        assert(this->config.id().valid());
        assert(this->layer_height_profile.id().valid());
        assert(this->id() != this->config.id());
        assert(this->id() != this->layer_height_profile.id());
    	assert(this->id() == rhs.id());
        assert(this->config.id() == rhs.config.id());
        assert(this->layer_height_profile.id() == rhs.layer_height_profile.id());
    	return *this;
    }
    ModelObject& operator=(ModelObject &&rhs) {
    	this->assign_copy(std::move(rhs));
    	m_model = rhs.m_model;
    	assert(this->id().valid());
        assert(this->config.id().valid());
        assert(this->layer_height_profile.id().valid());
        assert(this->id() != this->config.id());
        assert(this->id() != this->layer_height_profile.id());
    	assert(this->id() == rhs.id());
        assert(this->config.id() == rhs.config.id());
        assert(this->layer_height_profile.id() == rhs.layer_height_profile.id());
    	return *this;
    }
	void set_new_unique_id() {
        ObjectBase::set_new_unique_id();
        this->config.set_new_unique_id();
        this->layer_height_profile.set_new_unique_id();
    }

    /* Copy a model, copy the IDs. The Print::apply() will call the ModelObject::copy() method */
    /* to make a private copy for background processing. */
    static ModelObject* new_copy(const ModelObject& rhs)
    {
        auto* ret = new ModelObject(rhs);
        assert(ret->id() == rhs.id());
        return ret;
    }
    static ModelObject* new_copy(ModelObject&& rhs)
    {
        auto* ret = new ModelObject(std::move(rhs));
        assert(ret->id() == rhs.id());
        return ret;
    }
    static ModelObject make_copy(const ModelObject& rhs)
    {
        ModelObject ret(rhs);
        assert(ret.id() == rhs.id());
        return ret;
    }
    static ModelObject make_copy(ModelObject&& rhs)
    {
        ModelObject ret(std::move(rhs));
        assert(ret.id() == rhs.id());
        return ret;
    }
    ModelObject& assign_copy(const ModelObject& rhs);
    ModelObject& assign_copy(ModelObject&& rhs);
    /* Copy a ModelObject, generate new IDs. The front end will use this call. */
    static ModelObject* new_clone(const ModelObject& rhs)
    {
        /* Default constructor assigning an invalid ID. */
        auto obj = new ModelObject(-1);
        obj->assign_clone(rhs);
        assert(obj->id().valid() && obj->id() != rhs.id());
        return obj;
    }
    ModelObject make_clone(const ModelObject& rhs)
    {
        /* Default constructor assigning an invalid ID. */
        ModelObject obj(-1);
        obj.assign_clone(rhs);
        assert(obj.id().valid() && obj.id() != rhs.id());
        return obj;
    }
    ModelObject& assign_clone(const ModelObject& rhs)
    {
        this->assign_copy(rhs);
        assert(this->id().valid() && this->id() == rhs.id());
        this->assign_new_unique_ids_recursive();
        assert(this->id().valid() && this->id() != rhs.id());
        return *this;
    }

    // Parent object, owning this ModelObject. Set to nullptr here, so the macros above will have it initialized.
    Model                *m_model { nullptr };

    // Bounding box, cached.
    mutable BoundingBoxf3 m_bounding_box_approx;
    mutable bool          m_bounding_box_approx_valid { false };
    mutable BoundingBoxf3 m_bounding_box_exact;
    mutable BoundingBoxf3 m_belt_bounding_box_exact;
    mutable bool          m_bounding_box_exact_valid { false };
    mutable bool          m_min_max_z_valid { false };
    mutable bool          m_min_max_y_valid{false};
    mutable BoundingBoxf3 m_raw_bounding_box;
    mutable bool          m_raw_bounding_box_valid { false };
    mutable BoundingBoxf3 m_raw_mesh_bounding_box;
    mutable bool          m_raw_mesh_bounding_box_valid { false };

    // Only use this method if now the source and dest ModelObjects are equal, for example they were synchronized by Print::apply().
    void copy_transformation_caches(const ModelObject &src) {
        m_bounding_box_approx             = src.m_bounding_box_approx;
        m_bounding_box_approx_valid       = src.m_bounding_box_approx_valid;
        m_bounding_box_exact              = src.m_bounding_box_exact;
        m_bounding_box_exact_valid        = src.m_bounding_box_exact_valid;
        m_min_max_z_valid                 = src.m_min_max_z_valid;
        m_min_max_y_valid                 = src.m_min_max_y_valid;
        m_raw_bounding_box                = src.m_raw_bounding_box;
        m_raw_bounding_box_valid          = src.m_raw_bounding_box_valid;
        m_raw_mesh_bounding_box           = src.m_raw_mesh_bounding_box;
        m_raw_mesh_bounding_box_valid     = src.m_raw_mesh_bounding_box_valid;
    }

    // Called by Print::apply() to set the model pointer after making a copy.
    friend class Print;
    friend class SLAPrint;
    void        set_model(Model *model) { m_model = model; }

    // Undo / Redo through the cereal serialization library
	friend class cereal::access;
	friend class UndoRedo::StackImpl;
	// Used for deserialization -> Don't allocate any IDs for the ModelObject or its config.
	ModelObject() :
        ObjectBase(-1), config(-1), layer_height_profile(-1) {
		assert(this->id().invalid());
        assert(this->config.id().invalid());
        assert(this->layer_height_profile.id().invalid());
	}
    template<class Archive> void save(Archive& ar) const {
        ar(cereal::base_class<ObjectBase>(this));
        Internal::StaticSerializationWrapper<ModelConfigObject const> config_wrapper(config);
        Internal::StaticSerializationWrapper<LayerHeightProfile const> layer_heigth_profile_wrapper(layer_height_profile);
        ar(name, module_name, input_file, instances, volumes, config_wrapper, layer_config_ranges, layer_heigth_profile_wrapper,
            sla_support_points, sla_points_status, sla_drain_holes, printable, origin_translation, brim_points,
            m_bounding_box_approx, m_bounding_box_approx_valid, 
            m_bounding_box_exact, m_bounding_box_exact_valid, m_min_max_z_valid,
            m_raw_bounding_box, m_raw_bounding_box_valid, m_raw_mesh_bounding_box, m_raw_mesh_bounding_box_valid,
            cut_connectors, cut_id);
    }
    template<class Archive> void load(Archive& ar) {
        ar(cereal::base_class<ObjectBase>(this));
        Internal::StaticSerializationWrapper<ModelConfigObject> config_wrapper(config);
        Internal::StaticSerializationWrapper<LayerHeightProfile> layer_heigth_profile_wrapper(layer_height_profile);
        // BBS: add backup, check modify
        SaveObjectGaurd gaurd(*this);
        ar(name, module_name, input_file, instances, volumes, config_wrapper, layer_config_ranges, layer_heigth_profile_wrapper,
            sla_support_points, sla_points_status, sla_drain_holes, printable, origin_translation, brim_points,
            m_bounding_box_approx, m_bounding_box_approx_valid, 
            m_bounding_box_exact, m_bounding_box_exact_valid, m_min_max_z_valid,
            m_raw_bounding_box, m_raw_bounding_box_valid, m_raw_mesh_bounding_box, m_raw_mesh_bounding_box_valid,
            cut_connectors, cut_id);
        std::vector<ObjectID> volume_ids2;
        std::transform(volumes.begin(), volumes.end(), std::back_inserter(volume_ids2), std::mem_fn(&ObjectBase::id));
        if (volume_ids != volume_ids2)
            Slic3r::save_object_mesh(*this);
        volume_ids.clear();
    }

    // Called by Print::validate() from the UI thread.
    unsigned int update_instances_print_volume_state(const BuildVolume &build_volume);

    // Called by min_z(), max_z()
    void update_min_max_z();
    void update_min_max_y();
};

extern Transform3d beltXForm(const Transform3d& offset, float angle);
};

#endif /* slic3r_ModelObject_hpp_ */
