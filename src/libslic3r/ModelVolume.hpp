#ifndef slic3r_ModelVolume_hpp_
#define slic3r_ModelVolume_hpp_

#include "ModelCommon.hpp"
#include "TextConfiguration.hpp"
#include "EmbossShape.hpp"
#include "Geometry.hpp"
#include "Format/bbs_3mf.hpp"

namespace Slic3r {

class ModelObject;
class FacetsAnnotation final : public ObjectWithTimestamp {
public:
    // Assign the content if the timestamp differs, don't assign an ObjectID.
    void assign(const FacetsAnnotation& rhs) { if (! this->timestamp_matches(rhs)) { m_data = rhs.m_data; this->copy_timestamp(rhs); } }
    void assign(FacetsAnnotation&& rhs) { if (! this->timestamp_matches(rhs)) { m_data = std::move(rhs.m_data); this->copy_timestamp(rhs); } }
    //const std::pair<std::vector<std::pair<int, int>>, std::vector<bool>>& get_data() const throw() { return m_data; }
    const TriangleSelector::TriangleSplittingData& get_data() const noexcept { return m_data; }
    bool set(const TriangleSelector& selector);
    indexed_triangle_set get_facets(const ModelVolume& mv, EnforcerBlockerType type) const;
    // BBS
    void get_facets(const ModelVolume& mv, std::vector<indexed_triangle_set>& facets_per_type) const;
        void                 set_enforcer_block_type_limit(const ModelVolume& mv,
            EnforcerBlockerType max_type,
            EnforcerBlockerType to_delete_filament = EnforcerBlockerType::NONE,
            EnforcerBlockerType replace_filament = EnforcerBlockerType::NONE);
    indexed_triangle_set get_facets_strict(const ModelVolume& mv, EnforcerBlockerType type) const;
    bool has_facets(const ModelVolume& mv, EnforcerBlockerType type) const;
    //bool empty() const { return m_data.first.empty(); }
    bool empty() const { return m_data.triangles_to_split.empty(); }

    // Following method clears the config and increases its timestamp, so the deleted
    // state is considered changed from perspective of the undo/redo stack.
    void reset();

    // Serialize triangle into string, for serialization into 3MF/AMF.
    std::string get_triangle_as_string(int i) const;

    // Before deserialization, reserve space for n_triangles.
    //void reserve(int n_triangles) { m_data.first.reserve(n_triangles); }
    void reserve(int n_triangles) { m_data.triangles_to_split.reserve(n_triangles); }

    // Deserialize triangles one by one, with strictly increasing triangle_id.
    void set_triangle_from_string(int triangle_id, const std::string& str);
    // After deserializing the last triangle, shrink data to fit.
    //void shrink_to_fit() { m_data.first.shrink_to_fit(); m_data.second.shrink_to_fit(); }
    void shrink_to_fit()
    {
        m_data.triangles_to_split.shrink_to_fit();
        m_data.bitstream.shrink_to_fit();
    }

    bool equals(const FacetsAnnotation &other) const;

private:
    // Constructors to be only called by derived classes.
    // Default constructor to assign a unique ID.
    explicit FacetsAnnotation() = default;
    // Constructor with ignored int parameter to assign an invalid ID, to be replaced
    // by an existing ID copied from elsewhere.
    explicit FacetsAnnotation(int) : ObjectWithTimestamp(-1) {}
    // Copy constructor copies the ID.
    explicit FacetsAnnotation(const FacetsAnnotation &rhs) = default;
    // Move constructor copies the ID.
    explicit FacetsAnnotation(FacetsAnnotation &&rhs) = default;

    // called by ModelVolume::assign_copy()
    FacetsAnnotation& operator=(const FacetsAnnotation &rhs) = default;
    FacetsAnnotation& operator=(FacetsAnnotation &&rhs) = default;

    friend class cereal::access;
    friend class UndoRedo::StackImpl;

    template<class Archive> void serialize(Archive &ar)
    {
        ar(cereal::base_class<ObjectWithTimestamp>(this), m_data);
    }

    //std::pair<std::vector<std::pair<int, int>>, std::vector<bool>> m_data;
    TriangleSelector::TriangleSplittingData m_data;

    // To access set_new_unique_id() when copy / pasting a ModelVolume.
    friend class ModelVolume;
};

// An object STL, or a modifier volume, over which a different set of parameters shall be applied.
// ModelVolume instances are owned by a ModelObject.
class ModelVolume final : public ObjectBase
{
public:
    std::string         name;
    // struct used by reload from disk command to recover data from disk
    struct Source
    {
        std::string input_file;
        int object_idx{ -1 };
        int volume_idx{ -1 };
        Vec3d mesh_offset{ Vec3d::Zero() };
        Geometry::Transformation transform; 
        bool is_converted_from_inches{ false };
        bool is_converted_from_meters{ false };
        bool is_from_builtin_objects{ false };

        template<class Archive> void serialize(Archive& ar) { 
            //FIXME Vojtech: Serialize / deserialize only if the Source is set.
            // likely testing input_file or object_idx would be sufficient.
            ar(input_file, object_idx, volume_idx, mesh_offset, transform, is_converted_from_inches, is_converted_from_meters, is_from_builtin_objects);
        }
    };
    Source              source;

    // struct used by cut command 
    // It contains information about connetors
    struct CutInfo
    {
        bool                is_from_upper{ true };
        bool                is_connector{ false };
        bool                is_processed{ true };
        CutConnectorType    connector_type{ CutConnectorType::Plug };
        float               radius_tolerance{ 0.f };// [0.f : 1.f]
        float               height_tolerance{ 0.f };// [0.f : 1.f]

        CutInfo() = default;
        CutInfo(CutConnectorType type, float rad_tolerance, float h_tolerance, bool processed = false) :
        is_connector(true),
        is_processed(processed),
        connector_type(type),
        radius_tolerance(rad_tolerance),
        height_tolerance(h_tolerance)
        {}

        void set_processed() { is_processed = true; }
        void invalidate()    { is_connector = false; }
        void reset_from_upper() { is_from_upper = true; }

        template<class Archive> inline void serialize(Archive& ar) {
            ar(is_connector, is_processed, connector_type, radius_tolerance, height_tolerance);
        }
    };
    CutInfo             cut_info;

    bool                is_from_upper() const    { return cut_info.is_from_upper; }
    void                reset_from_upper()       { cut_info.reset_from_upper(); }

    bool                is_cut_connector() const { return cut_info.is_processed && cut_info.is_connector; }
    void                invalidate_cut_info()    { cut_info.invalidate(); }

    // The triangular model.
    const TriangleMesh& mesh() const { return *m_mesh.get(); }
    std::shared_ptr<const TriangleMesh> mesh_ptr() const { return m_mesh; }
    void                set_mesh(const TriangleMesh &mesh) { m_mesh = std::make_shared<const TriangleMesh>(mesh); }
    void                set_mesh(TriangleMesh &&mesh) { m_mesh = std::make_shared<const TriangleMesh>(std::move(mesh)); }
    void                set_mesh(const indexed_triangle_set &mesh) { m_mesh = std::make_shared<const TriangleMesh>(mesh); }
    void                set_mesh(indexed_triangle_set &&mesh) { m_mesh = std::make_shared<const TriangleMesh>(std::move(mesh)); }
    void                set_mesh(std::shared_ptr<const TriangleMesh> &mesh) { m_mesh = mesh; }
    void                set_mesh(std::unique_ptr<const TriangleMesh> &&mesh) { m_mesh = std::move(mesh); }
	void				reset_mesh() { m_mesh = std::make_shared<const TriangleMesh>(); }
    const std::shared_ptr<const TriangleMesh>& get_mesh_shared_ptr() const { return m_mesh; }
    // Configuration parameters specific to an object model geometry or a modifier volume, 
    // overriding the global Slic3r settings and the ModelObject settings.
    ModelConfigObject	config;

    // List of mesh facets to be supported/unsupported.
    FacetsAnnotation    supported_facets;

    // List of seam enforcers/blockers.
    FacetsAnnotation    seam_facets;

    // List of mesh facets painted for MMU segmentation.
    FacetsAnnotation    mmu_segmentation_facets;

    // BBS: quick access for volume extruders, 1 based
    mutable std::vector<int> mmuseg_extruders;
    mutable Timestamp        mmuseg_ts;

    // List of exterior faces
    FacetsAnnotation    exterior_facets;

    // Is set only when volume is Embossed Text type
    // Contain information how to re-create volume
    std::optional<TextConfiguration> text_configuration;

    // Is set only when volume is Embossed Shape
    // Contain 2d information about embossed shape to be editabled
    std::optional<EmbossShape> emboss_shape; 

    // A parent object owning this modifier volume.
    ModelObject*        get_object() const { return this->object; }
    ModelVolumeType     type() const { return m_type; }
    void                set_type(const ModelVolumeType t) { m_type = t; }
	bool                is_model_part()         const { return m_type == ModelVolumeType::MODEL_PART; }
    bool                is_negative_volume()    const { return m_type == ModelVolumeType::NEGATIVE_VOLUME; }
	bool                is_modifier()           const { return m_type == ModelVolumeType::PARAMETER_MODIFIER; }
	bool                is_support_enforcer()   const { return m_type == ModelVolumeType::SUPPORT_ENFORCER; }
	bool                is_support_blocker()    const { return m_type == ModelVolumeType::SUPPORT_BLOCKER; }
	bool                is_support_modifier()   const { return m_type == ModelVolumeType::SUPPORT_BLOCKER || m_type == ModelVolumeType::SUPPORT_ENFORCER; }
    bool                is_text()               const { return text_configuration.has_value(); }
    bool                is_svg() const { return emboss_shape.has_value()  && !text_configuration.has_value(); }
    bool                is_the_only_one_part() const; // behave like an object
    t_model_material_id material_id() const { return m_material_id; }
    void                set_material_id(t_model_material_id material_id);
    void                reset_extra_facets();
    ModelMaterial*      material() const;
    void                set_material(t_model_material_id material_id, const ModelMaterial &material);
    // Extract the current extruder ID based on this ModelVolume's config and the parent ModelObject's config.
    // Extruder ID is only valid for FFF. Returns -1 for SLA or if the extruder ID is not applicable (support volumes).
    int                 extruder_id() const;

    bool                is_splittable() const;

    // BBS
    std::vector<int>    get_extruders() const;
    void                update_extruder_count(size_t extruder_count);
    void                update_extruder_count_when_delete_filament(size_t extruder_count, size_t filament_id, int replace_filament_id = -1);
    // Split this volume, append the result to the object owning this volume.
    // Return the number of volumes created from this one.
    // This is useful to assign different materials to different volumes of an object.
    size_t              split(unsigned int max_extruders);
    void                translate(double x, double y, double z) { translate(Vec3d(x, y, z)); }
    void                translate(const Vec3d& displacement);
    void                scale(const Vec3d& scaling_factors);
    void                scale(double x, double y, double z) { scale(Vec3d(x, y, z)); }
    void                scale(double s) { scale(Vec3d(s, s, s)); }
    void                rotate(double angle, Axis axis);
    void                rotate(double angle, const Vec3d& axis);
    void                mirror(Axis axis);

    // This method could only be called before the meshes of this ModelVolumes are not shared!
    void                scale_geometry_after_creation(const Vec3f &versor);
    void                scale_geometry_after_creation(const float scale) { this->scale_geometry_after_creation(Vec3f(scale, scale, scale)); }

    // Translates the mesh and the convex hull so that the origin of their vertices is in the center of this volume's bounding box.
    // Attention! This method may only be called just after ModelVolume creation! It must not be called once the TriangleMesh of this ModelVolume is shared!
    void                center_geometry_after_creation(bool update_source_offset = true);

    void                calculate_convex_hull();
    const TriangleMesh& get_convex_hull() const;
    const std::shared_ptr<const TriangleMesh>& get_convex_hull_shared_ptr() const { return m_convex_hull; }
    //BBS: add convex_hell_2d related logic
    const Polygon& get_convex_hull_2d(const Transform3d &trafo_instance) const;
    void invalidate_convex_hull_2d()
    {
        m_convex_hull_2d.clear();
    }

    // Get count of errors in the mesh
    int                 get_repaired_errors_count() const;

    // Helpers for loading / storing into AMF / 3MF files.
    static ModelVolumeType type_from_string(const std::string &s);
    static std::string  type_to_string(const ModelVolumeType t);

    const Geometry::Transformation& get_transformation() const { return m_transformation; }
    void set_transformation(const Geometry::Transformation& transformation) { m_transformation = transformation; }
    void set_transformation(const Transform3d& trafo) { m_transformation.set_matrix(trafo); }

    Vec3d get_offset() const { return m_transformation.get_offset(); }

    double get_offset(Axis axis) const { return m_transformation.get_offset(axis); }

    void set_offset(const Vec3d& offset) { m_transformation.set_offset(offset); }
    void set_offset(Axis axis, double offset) { m_transformation.set_offset(axis, offset); }

    Vec3d get_rotation() const { return m_transformation.get_rotation(); }
    double get_rotation(Axis axis) const { return m_transformation.get_rotation(axis); }

    void set_rotation(const Vec3d& rotation) { m_transformation.set_rotation(rotation); }
    void set_rotation(Axis axis, double rotation) { m_transformation.set_rotation(axis, rotation); }

    Vec3d get_scaling_factor() const { return m_transformation.get_scaling_factor(); }
    double get_scaling_factor(Axis axis) const { return m_transformation.get_scaling_factor(axis); }

    void set_scaling_factor(const Vec3d& scaling_factor) { m_transformation.set_scaling_factor(scaling_factor); }
    void set_scaling_factor(Axis axis, double scaling_factor) { m_transformation.set_scaling_factor(axis, scaling_factor); }

    Vec3d get_mirror() const { return m_transformation.get_mirror(); }
    double get_mirror(Axis axis) const { return m_transformation.get_mirror(axis); }
    bool is_left_handed() const { return m_transformation.is_left_handed(); }

    void set_mirror(const Vec3d& mirror) { m_transformation.set_mirror(mirror); }
    void set_mirror(Axis axis, double mirror) { m_transformation.set_mirror(axis, mirror); }
    void convert_from_imperial_units();
    void convert_from_meters();

    const Transform3d& get_matrix() const { return m_transformation.get_matrix(); }
    Transform3d get_matrix_no_offset() const { return m_transformation.get_matrix_no_offset(); }

	void set_new_unique_id() {
        ObjectBase::set_new_unique_id();
        this->config.set_new_unique_id();
        this->supported_facets.set_new_unique_id();
        this->seam_facets.set_new_unique_id();
        this->mmu_segmentation_facets.set_new_unique_id();
    }

    bool is_fdm_support_painted() const { return !this->supported_facets.empty(); }
    bool is_seam_painted() const { return !this->seam_facets.empty(); }
    bool is_mm_painted() const { return !this->mmu_segmentation_facets.empty(); }

protected:
	friend class Print;
    friend class SLAPrint;
    friend class Model;
	friend class ModelObject;
    friend void model_volume_list_update_supports(ModelObject& model_object_dst, const ModelObject& model_object_new);

	// Copies IDs of both the ModelVolume and its config.
	explicit ModelVolume(const ModelVolume &rhs) = default;
    void     set_model_object(ModelObject *model_object) { object = model_object; }
	void 	 assign_new_unique_ids_recursive() override;
    void     transform_this_mesh(const Transform3d& t, bool fix_left_handed);
    void     transform_this_mesh(const Matrix3d& m, bool fix_left_handed);

private:
    // Parent object owning this ModelVolume.
    ModelObject*                    	object;
    // The triangular model.
    std::shared_ptr<const TriangleMesh> m_mesh;
    // Is it an object to be printed, or a modifier volume?
    ModelVolumeType                 	m_type;
    t_model_material_id             	m_material_id;
    // The convex hull of this model's mesh.
    std::shared_ptr<const TriangleMesh> m_convex_hull;
    //BBS: add convex hull 2d related logic
    mutable Polygon                     m_convex_hull_2d; //BBS, used for convex_hell_2d acceleration
    mutable Transform3d                 m_cached_trans_matrix; //BBS, used for convex_hell_2d acceleration
    mutable Polygon                     m_cached_2d_polygon;   //BBS, used for convex_hell_2d acceleration
    Geometry::Transformation        	m_transformation;

    //BBS: add convex_hell_2d related logic
    void  calculate_convex_hull_2d(const Geometry::Transformation &transformation) const;

    // flag to optimize the checking if the volume is splittable
    //     -1   ->   is unknown value (before first cheking)
    //      0   ->   is not splittable
    //      1   ->   is splittable
    mutable int               		m_is_splittable{ -1 };

	ModelVolume(ModelObject *object, const TriangleMesh &mesh, ModelVolumeType type = ModelVolumeType::MODEL_PART) : m_mesh(new TriangleMesh(mesh)), m_type(type), object(object)
    {
		assert(this->id().valid());
        assert(this->config.id().valid());
        assert(this->supported_facets.id().valid());
        assert(this->seam_facets.id().valid());
        assert(this->mmu_segmentation_facets.id().valid());
        assert(this->id() != this->config.id());
        assert(this->id() != this->supported_facets.id());
        assert(this->id() != this->seam_facets.id());
        assert(this->id() != this->mmu_segmentation_facets.id());
        if (mesh.facets_count() > 1) {
            calculate_convex_hull();
        }
        else {
            // fix crash problem when mesh.facets_count() <= 1
            m_convex_hull = std::make_shared<TriangleMesh>();
        }
    }
    ModelVolume(ModelObject *object, const std::shared_ptr<const TriangleMesh> &mesh, ModelVolumeType type = ModelVolumeType::MODEL_PART) : m_mesh(mesh), m_type(type), object(object)
    {
		assert(this->id().valid());
        assert(this->config.id().valid());
        assert(this->supported_facets.id().valid());
        assert(this->seam_facets.id().valid());
        assert(this->mmu_segmentation_facets.id().valid());
        assert(this->id() != this->config.id());
        assert(this->id() != this->supported_facets.id());
        assert(this->id() != this->seam_facets.id());
        assert(this->id() != this->mmu_segmentation_facets.id());
        if (mesh && mesh->facets_count() > 1) {
            calculate_convex_hull();
        }
        else {
            // fix crash problem when mesh.facets_count() <= 1
            m_convex_hull = std::make_shared<TriangleMesh>();
        }
    }
    ModelVolume(ModelObject *object, TriangleMesh &&mesh, TriangleMesh &&convex_hull, ModelVolumeType type = ModelVolumeType::MODEL_PART) :
		m_mesh(new TriangleMesh(std::move(mesh))), m_convex_hull(new TriangleMesh(std::move(convex_hull))), m_type(type), object(object) {
		assert(this->id().valid());
        assert(this->config.id().valid());
        assert(this->supported_facets.id().valid());
        assert(this->seam_facets.id().valid());
        assert(this->mmu_segmentation_facets.id().valid());
        assert(this->id() != this->config.id());
        assert(this->id() != this->supported_facets.id());
        assert(this->id() != this->seam_facets.id());
        assert(this->id() != this->mmu_segmentation_facets.id());
	}

    // Copying an existing volume, therefore this volume will get a copy of the ID assigned.
    ModelVolume(ModelObject *object, const ModelVolume &other) :
        ObjectBase(other),
        name(other.name), source(other.source), m_mesh(other.m_mesh), m_convex_hull(other.m_convex_hull),
        config(other.config), m_type(other.m_type), object(object), m_transformation(other.m_transformation),
        supported_facets(other.supported_facets), seam_facets(other.seam_facets), mmu_segmentation_facets(other.mmu_segmentation_facets),
        cut_info(other.cut_info), text_configuration(other.text_configuration), emboss_shape(other.emboss_shape)
    {
		assert(this->id().valid()); 
        assert(this->config.id().valid()); 
        assert(this->supported_facets.id().valid());
        assert(this->seam_facets.id().valid());
        assert(this->mmu_segmentation_facets.id().valid());
        assert(this->id() != this->config.id());
        assert(this->id() != this->supported_facets.id());
        assert(this->id() != this->seam_facets.id());
        assert(this->id() != this->mmu_segmentation_facets.id());
		assert(this->id() == other.id());
        assert(this->config.id() == other.config.id());
        assert(this->supported_facets.id() == other.supported_facets.id());
        assert(this->seam_facets.id() == other.seam_facets.id());
        assert(this->mmu_segmentation_facets.id() == other.mmu_segmentation_facets.id());
        this->set_material_id(other.material_id());
    }
    // Providing a new mesh, therefore this volume will get a new unique ID assigned.
    ModelVolume(ModelObject *object, const ModelVolume &other, TriangleMesh &&mesh) :
        name(other.name), source(other.source), config(other.config), object(object), m_mesh(new TriangleMesh(std::move(mesh))), m_type(other.m_type), m_transformation(other.m_transformation),
        cut_info(other.cut_info), text_configuration(other.text_configuration), emboss_shape(other.emboss_shape)
    {
		assert(this->id().valid()); 
        assert(this->config.id().valid()); 
        assert(this->supported_facets.id().valid());
        assert(this->seam_facets.id().valid());
        assert(this->mmu_segmentation_facets.id().valid());
        assert(this->id() != this->config.id());
        assert(this->id() != this->supported_facets.id());
        assert(this->id() != this->seam_facets.id());
        assert(this->id() != this->mmu_segmentation_facets.id());
		assert(this->id() != other.id());
        assert(this->config.id() == other.config.id());
        this->set_material_id(other.material_id());
        this->config.set_new_unique_id();
        if (m_mesh->facets_count() > 1)
            calculate_convex_hull();
        else {
            // fix crash problem when mesh.facets_count() <= 1
            m_convex_hull = std::make_shared<TriangleMesh>();
        }
		assert(this->config.id().valid()); 
        assert(this->config.id() != other.config.id()); 
        assert(this->supported_facets.id() != other.supported_facets.id());
        assert(this->seam_facets.id() != other.seam_facets.id());
        assert(this->mmu_segmentation_facets.id() != other.mmu_segmentation_facets.id());
        assert(this->id() != this->config.id());
        assert(this->supported_facets.empty());
        assert(this->seam_facets.empty());
        assert(this->mmu_segmentation_facets.empty());
    }

    ModelVolume& operator=(ModelVolume &rhs) = delete;

	friend class cereal::access;
	friend class UndoRedo::StackImpl;
	// Used for deserialization, therefore no IDs are allocated.
	ModelVolume() : ObjectBase(-1), config(-1), supported_facets(-1), seam_facets(-1), mmu_segmentation_facets(-1), object(nullptr) {
		assert(this->id().invalid());
        assert(this->config.id().invalid());
        assert(this->supported_facets.id().invalid());
        assert(this->seam_facets.id().invalid());
        assert(this->mmu_segmentation_facets.id().invalid());

        // if m_convex_hull is null,  would cause crash problem when calling "ModelObject::instance_convex_hull_bounding_box"
        m_convex_hull = std::make_shared<TriangleMesh>();
	}
	template<class Archive> void load(Archive &ar) {
		bool has_convex_hull;
        // BBS: add backup, check modify
        bool mesh_changed = false;
        auto tr = m_transformation;
        ar(name, source, m_mesh, m_type, m_material_id, m_transformation, m_is_splittable, has_convex_hull, cut_info);
        mesh_changed |= !(tr == m_transformation);
        auto t = supported_facets.timestamp();
        cereal::load_by_value(ar, supported_facets);
        mesh_changed |= t != supported_facets.timestamp();
        t = seam_facets.timestamp();
        cereal::load_by_value(ar, seam_facets);
        mesh_changed |= t != seam_facets.timestamp();
        t = mmu_segmentation_facets.timestamp();
        cereal::load_by_value(ar, mmu_segmentation_facets);
        mesh_changed |= t != mmu_segmentation_facets.timestamp();
        cereal::load_by_value(ar, config);
        cereal::load(ar, text_configuration);
        cereal::load(ar, emboss_shape);
		assert(m_mesh);
		if (has_convex_hull) {
			cereal::load_optional(ar, m_convex_hull);
			if (! m_convex_hull && ! m_mesh->empty())
				// The convex hull was released from the Undo / Redo stack to conserve memory. Recalculate it.
				this->calculate_convex_hull();
		} else
			m_convex_hull.reset();
        if (mesh_changed && object)
            Slic3r::save_object_mesh(*object);
	}
	template<class Archive> void save(Archive &ar) const {
		bool has_convex_hull = m_convex_hull.get() != nullptr;
        ar(name, source, m_mesh, m_type, m_material_id, m_transformation, m_is_splittable, has_convex_hull, cut_info);
        cereal::save_by_value(ar, supported_facets);
        cereal::save_by_value(ar, seam_facets);
        cereal::save_by_value(ar, mmu_segmentation_facets);
        cereal::save_by_value(ar, config);
        cereal::save(ar, text_configuration);
        cereal::save(ar, emboss_shape);
		if (has_convex_hull)
			cereal::save_optional(ar, m_convex_hull);
	}
};

inline void model_volumes_sort_by_id(ModelVolumePtrs &model_volumes)
{
    std::sort(model_volumes.begin(), model_volumes.end(), [](const ModelVolume *l, const ModelVolume *r) { return l->id() < r->id(); });
}

inline const ModelVolume* model_volume_find_by_id(const ModelVolumePtrs &model_volumes, const ObjectID id)
{
    auto it = lower_bound_by_predicate(model_volumes.begin(), model_volumes.end(), [id](const ModelVolume *mv) { return mv->id() < id; });
    return it != model_volumes.end() && (*it)->id() == id ? *it : nullptr;
}
};

#endif /* slic3r_ModelVolume_hpp_ */
