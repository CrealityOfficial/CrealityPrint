#ifndef slic3r_Model_hpp_
#define slic3r_Model_hpp_

#include "calib.hpp"

//BBS: add bbs 3mf
#include "Format/bbs_3mf.hpp"
//BBS: add step
//#include "Format/STEP.hpp"
#include "Format/step_func_def.hpp"
//BBS: add stl
#include "Format/STL.hpp"
#include "Format/OBJ.hpp"
#include "Format/AssimpModel.hpp"

#include "ModelCommon.hpp"
#include "ModelWipeTower.hpp"

namespace Slic3r {
enum class ConversionType;

class BuildVolume;
class Model;
class ModelInstance;
class ModelMaterial;
class ModelObject;
class ModelVolume;
class ModelWipeTower;
class Print;
class SLAPrint;
class TriangleSelector;
//BBS: add Preset
class Preset;
class BBLProject;

class KeyStore;
class Step;

// The print bed content.
// Description of a triangular model with multiple materials, multiple instances with various affine transformations
// and with multiple modifier meshes.
// A model groups multiple objects, each object having possibly multiple instances,
// all objects may share mutliple materials.
class Model final : public ObjectBase
{
public:
    // Materials are owned by a model and referenced by objects through t_model_material_id.
    // Single material may be shared by multiple models.
    ModelMaterialMap    materials;
    // Objects are owned by a model. Each model may have multiple instances, each instance having its own transformation (shift, scale, rotation).
    ModelObjectPtrs     objects;
    // Wipe tower object.
    ModelWipeTower	wipe_tower;
    // BBS static members store extruder parameters and speed map of all models
    static std::map<size_t, ExtruderParams> extruderParamsMap;
    static GlobalSpeedMap printSpeedMap;

    // DesignInfo of Model
    std::string stl_design_id;
    std::string stl_design_country;
    std::shared_ptr<ModelDesignInfo> design_info = nullptr;
    std::shared_ptr<ModelInfo> model_info = nullptr;
    std::shared_ptr<ModelProfileInfo> profile_info = nullptr;

    //makerlab information
    std::string mk_name;
    std::string mk_version;
    std::vector<std::string> md_name;
    std::vector<std::string> md_value;

    void SetDesigner(std::string designer, std::string designer_user_id) {
        if (design_info == nullptr) {
            design_info = std::make_shared<ModelDesignInfo>();
        }
        design_info->Designer = designer;
        //BBS tips: clean design user id when set designer
        design_info->DesignerUserId = designer_user_id;
    }

    // Extensions for color print
    // CustomGCode::Info custom_gcode_per_print_z;
    //BBS: replace model custom gcode with current plate custom gcode
    int curr_plate_index{ 0 };
    std::map<int, CustomGCode::Info> plates_custom_gcodes; //map<plate_index, CustomGCode::Info>

    const CustomGCode::Info get_curr_plate_custom_gcodes() const {
        if (plates_custom_gcodes.find(curr_plate_index) != plates_custom_gcodes.end()) {
            return plates_custom_gcodes.at(curr_plate_index);
        }
        return CustomGCode::Info();
    }

    // Default constructor assigns a new ID to the model.
    Model() { assert(this->id().valid()); }
    ~Model();

    /* To be able to return an object from own copy / clone methods. Hopefully the compiler will do the "Copy elision" */
    /* (Omits copy and move(since C++11) constructors, resulting in zero - copy pass - by - value semantics). */
    Model(const Model &rhs) : ObjectBase(-1) { assert(this->id().invalid()); this->assign_copy(rhs); assert(this->id().valid()); assert(this->id() == rhs.id()); }
    // BBS: remove explicit, prefer use move constructor in function return model
    Model(Model &&rhs) : ObjectBase(-1) { assert(this->id().invalid()); this->assign_copy(std::move(rhs)); assert(this->id().valid()); assert(this->id() == rhs.id()); }
    Model& operator=(const Model &rhs) { this->assign_copy(rhs); assert(this->id().valid()); assert(this->id() == rhs.id()); return *this; }
    Model& operator=(Model &&rhs) { this->assign_copy(std::move(rhs)); assert(this->id().valid()); assert(this->id() == rhs.id()); return *this; }

    /* Copy a model, copy the IDs. The Print::apply() will call the Model::copy() method */
    /* to make a private copy for background processing. */
    static Model* new_copy(const Model &rhs)  { auto *ret = new Model(rhs); assert(ret->id() == rhs.id()); return ret; }
    static Model* new_copy(Model &&rhs)       { auto *ret = new Model(std::move(rhs)); assert(ret->id() == rhs.id()); return ret; }
    static Model  make_copy(const Model &rhs) { Model ret(rhs); assert(ret.id() == rhs.id()); return ret; }
    static Model  make_copy(Model &&rhs)      { Model ret(std::move(rhs)); assert(ret.id() == rhs.id()); return ret; }
    Model&        assign_copy(const Model &rhs);
    Model&        assign_copy(Model &&rhs);
    /* Copy a Model, generate new IDs. The front end will use this call. */
    static Model* new_clone(const Model &rhs) {
        /* Default constructor assigning an invalid ID. */
        auto obj = new Model(-1);
        obj->assign_clone(rhs);
        assert(obj->id().valid() && obj->id() != rhs.id());
        return obj;
	}
    Model         make_clone(const Model &rhs) {
        /* Default constructor assigning an invalid ID. */
        Model obj(-1);
        obj.assign_clone(rhs);
        assert(obj.id().valid() && obj.id() != rhs.id());
        return obj;
    }
    Model&        assign_clone(const Model &rhs) {
        this->assign_copy(rhs);
        assert(this->id().valid() && this->id() == rhs.id());
        this->assign_new_unique_ids_recursive();
        assert(this->id().valid() && this->id() != rhs.id());
		return *this;
    }

    static Model read_from_step(const std::string&                                      input_file,
                        LoadStrategy                                            options,
                        ImportStepProgressFn                                    stepFn,
                        StepIsUtf8Fn                                            stepIsUtf8Fn,
                        std::function<int(Slic3r::Step&, double&, double&, bool&)>     step_mesh_fn,
                        double                                                  linear_defletion,
                        double                                                  angle_defletion,
                        bool                                                    is_split_compound);

    //BBS: add part plate related logic
    // BBS: backup
    //BBS: is_xxx is used for is_bbs_3mf when loading 3mf, is used for is_inches when loading amf
    static Model read_from_file(
        const std::string& input_file,
        DynamicPrintConfig* config = nullptr, ConfigSubstitutionContext* config_substitutions = nullptr,
        LoadStrategy options = LoadStrategy::AddDefaultInstances, PlateDataPtrs* plate_data = nullptr,
        std::vector<Preset*>* project_presets = nullptr, bool* is_xxx = nullptr, Semver* file_version = nullptr, Import3mfProgressFn proFn = nullptr,
                                ImportstlProgressFn        stlFn                = nullptr,
                                ImportStepProgressFn       stepFn               = nullptr,
                                StepIsUtf8Fn               stepIsUtf8Fn         = nullptr,
                                BBLProject *               project              = nullptr,
                                int                        plate_id             = 0,
                                ObjImportColorFn           objFn                = nullptr
                                );
    // BBS
    static bool    obj_import_vertex_color_deal(const std::vector<unsigned char> &vertex_filament_ids, const unsigned char &first_extruder_id, Model *model);
    static bool    obj_import_face_color_deal(const std::vector<unsigned char> &face_filament_ids, const unsigned char &first_extruder_id, Model *model);
    static double findMaxSpeed(const ModelObject* object);
    static double getThermalLength(const ModelVolume* modelVolumePtr);
    static double getThermalLength(const std::vector<ModelVolume*> modelVolumePtrs);
    static Polygon getBedPolygon() { return Model::printSpeedMap.bed_poly; }
    //BBS static functions that update extruder params and speed table
    static void setPrintSpeedTable(const DynamicPrintConfig& config, const PrintConfig& print_config);
    static void setExtruderParams(const DynamicPrintConfig& config, int extruders_count);

    // BBS: backup
    static Model read_from_archive(
        const std::string& input_file,
        DynamicPrintConfig* config, ConfigSubstitutionContext* config_substitutions, En3mfType& out_file_type,
        LoadStrategy options = LoadStrategy::AddDefaultInstances, PlateDataPtrs* plate_data = nullptr, std::vector<Preset*>* project_presets = nullptr, Semver* file_version = nullptr, Import3mfProgressFn proFn = nullptr, BBLProject* project = nullptr);

    // Add a new ModelObject to this Model, generate a new ID for this ModelObject.
    ModelObject* add_object();
    ModelObject* add_object(const char *name, const char *path, const TriangleMesh &mesh);
    ModelObject* add_object(const char *name, const char *path, TriangleMesh &&mesh);
    ModelObject* add_object(const ModelObject &other);
    void         delete_object(size_t idx);
    bool         delete_object(ObjectID id);
    bool         delete_object(ModelObject* object);
    void         clear_objects();
    // BBS: backup, reuse objects
    void         collect_reusable_objects(std::vector<ObjectBase *> & objects);
    void         set_object_backup_id(ModelObject const & object, int uuid);
    int          get_object_backup_id(ModelObject const & object); // generate new if needed
    int          get_object_backup_id(ModelObject const & object) const; // generate new if needed

    ModelMaterial* add_material(t_model_material_id material_id);
    ModelMaterial* add_material(t_model_material_id material_id, const ModelMaterial &other);
    ModelMaterial* get_material(t_model_material_id material_id) {
        ModelMaterialMap::iterator i = this->materials.find(material_id);
        return (i == this->materials.end()) ? nullptr : i->second;
    }

    void          delete_material(t_model_material_id material_id);
    void          clear_materials();
    bool          add_default_instances();
    // Returns approximate axis aligned bounding box of this model.
    BoundingBoxf3 bounding_box_approx() const;
    // Returns exact axis aligned bounding box of this model.
    BoundingBoxf3 bounding_box_exact() const;
    // Return maximum height of all printable objects.
    double        max_z() const;
    // Set the print_volume_state of PrintObject::instances,
    // return total number of printable objects.
    unsigned int  update_print_volume_state(const BuildVolume &build_volume);
    // Returns true if any ModelObject was modified.
    bool 		  center_instances_around_point(const Vec2d &point);
    void         translate(coordf_t x, coordf_t y, coordf_t z);
    TriangleMesh  mesh() const;

    // Croaks if the duplicated objects do not fit the print bed.
    void duplicate_objects_grid(size_t x, size_t y, coordf_t dist);

    bool 		  looks_like_multipart_object() const;
    void 		  convert_multipart_object(unsigned int max_extruders);
    bool          looks_like_imperial_units() const;
    void          convert_from_imperial_units(bool only_small_volumes);
    bool          looks_like_saved_in_meters() const;
    void          convert_from_meters(bool only_small_volumes);
    int           removed_objects_with_zero_volume();
    void          convert_from_creality5(Vec2d bed_size,Vec2d new_bed_size,int plate_size);
    // Ensures that the min z of the model is not negative
    void 		  adjust_min_z();

    void print_info() const;

    // Propose an output file name & path based on the first printable object's name and source input file's path.
    std::string   propose_export_file_name_and_path() const;
    // Propose an output path, replace extension. The new_extension shall contain the initial dot.
    std::string   propose_export_file_name_and_path(const std::string &new_extension) const;
    //BBS: add auxiliary files temp path
    std::string   get_auxiliary_file_temp_path();

    // BBS: backup
    std::string   get_backup_path();
    std::string   get_backup_path(const std::string &sub_path);
    void          set_backup_path(const std::string &path);
    void          load_from(Model & model);
    bool          is_need_backup() { return need_backup;  }
    void          set_need_backup();
    void          remove_backup_path_if_exist();

    // Checks if any of objects is painted using the fdm support painting gizmo.
    bool          is_fdm_support_painted() const;
    // Checks if any of objects is painted using the seam painting gizmo.
    bool          is_seam_painted() const;
    // Checks if any of objects is painted using the multi-material painting gizmo.
    bool          is_mm_painted() const;
    // Checks if any of objects is painted using the fuzzy skin painting gizmo.
    bool          is_fuzzy_skin_painted() const;

    std::unique_ptr<CalibPressureAdvancePattern> calib_pa_pattern;

private:
    explicit Model(int) : ObjectBase(-1)
        {
        assert(this->id().invalid());
    }
	void assign_new_unique_ids_recursive();
	void update_links_bottom_up_recursive();

	friend class cereal::access;
	friend class UndoRedo::StackImpl;
    template<class Archive> void load(Archive& ar) {
        Internal::StaticSerializationWrapper<ModelWipeTower> wipe_tower_wrapper(wipe_tower);
        ar(materials, objects, wipe_tower_wrapper);
    }
    template<class Archive> void save(Archive& ar) const {
        Internal::StaticSerializationWrapper<ModelWipeTower const> wipe_tower_wrapper(wipe_tower);
        ar(materials, objects, wipe_tower_wrapper);
    }

    //BBS: add aux temp directory
    // BBS: backup
    std::string backup_path;
    bool need_backup = false;
    std::map<int, int> object_backup_id_map; // ObjectId -> backup id;
    int next_object_backup_id = 1;
};

// Test whether the two models contain the same number of ModelObjects with the same set of IDs
// ordered in the same order. In that case it is not necessary to kill the background processing.
bool model_object_list_equal(const Model &model_old, const Model &model_new);

// Test whether the new model is just an extension of the old model (new objects were added
// to the end of the original list. In that case it is not necessary to kill the background processing.
bool model_object_list_extended(const Model &model_old, const Model &model_new);

// Test whether the new ModelObject contains a different set of volumes (or sorted in a different order)
// than the old ModelObject.
bool model_volume_list_changed(const ModelObject &model_object_old, const ModelObject &model_object_new, const ModelVolumeType type);
bool model_volume_list_changed(const ModelObject &model_object_old, const ModelObject &model_object_new, const std::initializer_list<ModelVolumeType> &types);

// Test whether the now ModelObject has newer custom supports data than the old one.
// The function assumes that volumes list is synchronized.
bool model_custom_supports_data_changed(const ModelObject& mo, const ModelObject& mo_new);

// Test whether the now ModelObject has newer custom seam data than the old one.
// The function assumes that volumes list is synchronized.
bool model_custom_seam_data_changed(const ModelObject& mo, const ModelObject& mo_new);

// Test whether the now ModelObject has newer MMU segmentation data than the old one.
// The function assumes that volumes list is synchronized.
extern bool model_mmu_segmentation_data_changed(const ModelObject& mo, const ModelObject& mo_new);

// Test whether the now ModelObject has newer fuzzy skin data than the old one.
// The function assumes that volumes list is synchronized.
extern bool model_fuzzy_skin_data_changed(const ModelObject &mo, const ModelObject &mo_new);

// Filament ID to TriangleSelector hex string lookup table (src/libslic3r/Model.cpp).
// The table is used by get_real_filament_id() to encode filament_id (1..size()-1) as
// a hex nibble sequence compatible with TriangleSelector::set_triangle_from_string.
// Length is tied to the OBJ color dialog UI limit (g_max_color) - extend both together.
extern const std::vector<std::string> CONST_FILAMENTS;
bool model_brim_points_data_changed(const ModelObject& mo, const ModelObject& mo_new);

// If the model has multi-part objects, then it is currently not supported by the SLA mode.
// Either the model cannot be loaded, or a SLA printer has to be activated.
bool model_has_multi_part_objects(const Model &model);
// If the model has advanced features, then it cannot be processed in simple mode.
bool model_has_advanced_features(const Model &model);

#ifndef NDEBUG
// Verify whether the IDs of Model / ModelObject / ModelVolume / ModelInstance / ModelMaterial are valid and unique.
void check_model_ids_validity(const Model &model);
void check_model_ids_equal(const Model &model1, const Model &model2);
#endif /* NDEBUG */

static const float SINKING_Z_THRESHOLD = -0.001f;
static const double SINKING_MIN_Z_THRESHOLD = 0.05;

} // namespace Slic3r

namespace cereal
{
    template <class Archive> struct specialize<Archive, Slic3r::ModelVolume, cereal::specialization::member_load_save> {};
    // BBS: backup
    template <class Archive> struct specialize<Archive, Slic3r::Model, cereal::specialization::member_load_save> {};
    template <class Archive> struct specialize<Archive, Slic3r::ModelObject, cereal::specialization::member_load_save> {};
    template <class Archive> struct specialize<Archive, Slic3r::ModelConfigObject, cereal::specialization::member_serialize> {};
}

#endif /* slic3r_Model_hpp_ */
