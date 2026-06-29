#include "Model.hpp"
#include "BuildVolume.hpp"
#include "Format/AMF.hpp"
#include "Format/svg.hpp"
#include "GCodeWriter.hpp"
#include "Format/3mf.hpp"
#include "Format/STEP.hpp"
#include <boost/log/trivial.hpp>

#include <future>

// Transtltion
#include "I18N.hpp"

 #include "ModelObject.hpp"
 #include "ModelVolume.hpp"
 #include "ModelInstance.hpp"

// ModelIO support
#ifdef __APPLE__
#include "Format/ModelIO.hpp"
#endif

#define _L(s) Slic3r::I18N::translate(s)

namespace Slic3r {
const std::vector<std::string> CONST_FILAMENTS = {
    "", "4", "8", "0C", "1C", "2C", "3C", "4C", "5C", "6C", "7C", "8C", "9C", "AC", "BC", "CC", "DC",
    // IDs 17-32: 3 nibbles (hex((N-3)%15) + "F" + "C")
    "EC", "0FC", "1FC", "2FC", "3FC", "4FC", "5FC", "6FC", "7FC", "8FC", "9FC", "AFC", "BFC", "CFC", "DFC", "EFC",
    // IDs 33-47: 4 nibbles (hex((N-3)%15) + "FF" + "C")
    "0FFC", "1FFC", "2FFC", "3FFC", "4FFC", "5FFC", "6FFC", "7FFC", "8FFC", "9FFC", "AFFC", "BFFC", "CFFC", "DFFC", "EFFC",
    // IDs 48-62: 5 nibbles (hex((N-3)%15) + "FFF" + "C")
    "0FFFC", "1FFFC", "2FFFC", "3FFFC", "4FFFC", "5FFFC", "6FFFC", "7FFFC", "8FFFC", "9FFFC", "AFFFC", "BFFFC", "CFFFC", "DFFFC", "EFFFC",
    // IDs 63-64: 6 nibbles (hex((N-3)%15) + "FFFF" + "C")
    "0FFFFC", "1FFFFC",
}; // 65 entries: ID 0 (empty), 1-2 (1 nibble), 3-16 (2 nibbles), 17-32 (3 nibbles), 33-47, 48-62, 63-64 (6 nibbles)
    // BBS initialization of static variables
    std::map<size_t, ExtruderParams> Model::extruderParamsMap = { {0,{"",0,0}}};
    GlobalSpeedMap Model::printSpeedMap{};
Model& Model::assign_copy(const Model &rhs)
{
    this->copy_id(rhs);
    // copy materials
    this->clear_materials();
    this->materials = rhs.materials;
    for (std::pair<const t_model_material_id, ModelMaterial*> &m : this->materials) {
        // Copy including the ID and m_model.
        m.second = new ModelMaterial(*m.second);
        m.second->set_model(this);
    }
    // copy objects
    this->clear_objects();
    this->objects.reserve(rhs.objects.size());
	for (const ModelObject *model_object : rhs.objects) {
        // Copy including the ID, leave ID set to invalid (zero).
        auto mo = ModelObject::new_copy(*model_object);
        mo->set_model(this);
		this->objects.emplace_back(mo);
    }

    // copy custom code per height
    // BBS
    this->plates_custom_gcodes = rhs.plates_custom_gcodes;
    this->curr_plate_index = rhs.curr_plate_index;
    this->calib_pa_pattern.reset();

    if (rhs.calib_pa_pattern) {
        this->calib_pa_pattern = std::make_unique<CalibPressureAdvancePattern>(
            CalibPressureAdvancePattern(*rhs.calib_pa_pattern)
        );
    }

    if (rhs.calib_pa_pattern) {
        this->calib_pa_pattern = std::make_unique<CalibPressureAdvancePattern>(CalibPressureAdvancePattern(*rhs.calib_pa_pattern));
    }

    // BBS: for design info
    this->design_info = rhs.design_info;
    this->model_info = rhs.model_info;
    this->stl_design_id = rhs.stl_design_id;
    this->stl_design_country = rhs.stl_design_country;
    this->profile_info = rhs.profile_info;

    this->mk_name = rhs.mk_name;
    this->mk_version = rhs.mk_version;
    this->md_name = rhs.md_name;
    this->md_value = rhs.md_value;

    return *this;
}

Model& Model::assign_copy(Model &&rhs)
{
    this->copy_id(rhs);
	// Move materials, adjust the parent pointer.
    this->clear_materials();
    this->materials = std::move(rhs.materials);
    for (std::pair<const t_model_material_id, ModelMaterial*> &m : this->materials)
        m.second->set_model(this);
    rhs.materials.clear();
    // Move objects, adjust the parent pointer.
    this->clear_objects();
	this->objects = std::move(rhs.objects);
    for (ModelObject *model_object : this->objects)
        model_object->set_model(this);
    rhs.objects.clear();

    // copy custom code per height
    // BBS
    this->plates_custom_gcodes = std::move(rhs.plates_custom_gcodes);
    this->curr_plate_index = rhs.curr_plate_index;
    this->calib_pa_pattern.reset();
    this->calib_pa_pattern.swap(rhs.calib_pa_pattern);

    //BBS: add auxiliary path logic
    // BBS: backup, all in one temp dir
    this->stl_design_id = rhs.stl_design_id;
    this->stl_design_country = rhs.stl_design_country;
    this->mk_name = rhs.mk_name;
    this->mk_version = rhs.mk_version;
    this->md_name = rhs.md_name;
    this->md_value = rhs.md_value;
    this->backup_path = std::move(rhs.backup_path);
    this->object_backup_id_map = std::move(rhs.object_backup_id_map);
    this->next_object_backup_id = rhs.next_object_backup_id;
    this->design_info = rhs.design_info;
    rhs.design_info.reset();
    this->model_info = rhs.model_info;
    rhs.model_info.reset();
    this->profile_info = rhs.profile_info;
    rhs.profile_info.reset();
    return *this;
}

void Model::assign_new_unique_ids_recursive()
{
    this->set_new_unique_id();
    for (std::pair<const t_model_material_id, ModelMaterial*> &m : this->materials)
        m.second->assign_new_unique_ids_recursive();
    for (ModelObject *model_object : this->objects)
        model_object->assign_new_unique_ids_recursive();
}

void Model::update_links_bottom_up_recursive()
{
	for (std::pair<const t_model_material_id, ModelMaterial*> &kvp : this->materials)
		kvp.second->set_model(this);
	for (ModelObject *model_object : this->objects) {
		model_object->set_model(this);
		for (ModelInstance *model_instance : model_object->instances)
			model_instance->set_model_object(model_object);
		for (ModelVolume *model_volume : model_object->volumes)
			model_volume->set_model_object(model_object);
	}
}

Model::~Model()
{
    this->clear_objects();
    this->clear_materials();
    // BBS: clear backup dir of temparary model
    if (!backup_path.empty())
        Slic3r::remove_backup(*this, true);
}

// Load a STEP file, return a Model object.
Model Model::read_from_step(const std::string&                                      input_file,
                            LoadStrategy                                            options,
                            ImportStepProgressFn                                    stepFn,
                            StepIsUtf8Fn                                            stepIsUtf8Fn,
                            std::function<int(Slic3r::Step&, double&, double&, bool&)>     step_mesh_fn,
                            double                                                  linear_defletion,
                            double                                                  angle_defletion,
                            bool                                                   is_split_compound)
{
    Model model;
    bool result = false;
    bool is_cb_cancel = false;
    Step::Step_Status status;
    Step step_file(input_file, stepFn);
    status = step_file.load();
    if(status != Step::Step_Status::LOAD_SUCCESS) {
        goto _finished;
    }
    if (step_mesh_fn) {
        if (step_mesh_fn(step_file, linear_defletion, angle_defletion, is_split_compound) == -1) {
            status = Step::Step_Status::CANCEL;
            goto _finished;
        }
    }
    
    status = step_file.mesh(&model, is_cb_cancel, is_split_compound, linear_defletion, angle_defletion);

_finished:

    switch (status){
        case Step::Step_Status::CANCEL: {
            Model empty_model;
            return empty_model;
        }
        case Step::Step_Status::LOAD_ERROR:
            throw Slic3r::RuntimeError(_L("Loading of a model file failed."));
        case Step::Step_Status::MESH_ERROR:
            throw Slic3r::RuntimeError(_L("Meshing of a model file failed or no valid shape."));
        default:
            break;
    }

    if (model.objects.empty())
        throw Slic3r::RuntimeError(_L("The supplied file couldn't be read because it's empty"));

    for (ModelObject *o : model.objects)
        o->input_file = input_file;

    if (options & LoadStrategy::AddDefaultInstances)
        model.add_default_instances();

    return model;
}

// BBS: add part plate related logic
// BBS: backup & restore
// Loading model from a file, it may be a simple geometry file as STL or OBJ, however it may be a project file as well.
Model Model::read_from_file(const std::string& input_file, DynamicPrintConfig* config, ConfigSubstitutionContext* config_substitutions,
                            LoadStrategy options, PlateDataPtrs* plate_data, std::vector<Preset*>* project_presets, bool *is_xxx, Semver* file_version, Import3mfProgressFn proFn,
                            ImportstlProgressFn        stlFn,
                            ImportStepProgressFn       stepFn,
                            StepIsUtf8Fn               stepIsUtf8Fn,
                            BBLProject *               project,
                            int                        plate_id,
                            ObjImportColorFn           objFn)
{
    Model model;


    DynamicPrintConfig temp_config;
    ConfigSubstitutionContext temp_config_substitutions_context(ForwardCompatibilitySubstitutionRule::EnableSilent);
    if (config == nullptr)
        config = &temp_config;
    if (config_substitutions == nullptr)
        config_substitutions = &temp_config_substitutions_context;
    //BBS: plate_data
    PlateDataPtrs temp_plate_data;
    bool temp_is_xxx;
    Semver temp_version;
    if (plate_data == nullptr)
        plate_data = &temp_plate_data;
    if (is_xxx == nullptr)
        is_xxx = &temp_is_xxx;
    if (file_version == nullptr)
        file_version = &temp_version;

    bool result = false;
    bool is_cb_cancel = false;
    std::string message;
    //if (boost::algorithm::iends_with(input_file, ".stp") ||
    //    boost::algorithm::iends_with(input_file, ".step"))
    //    result = load_step(input_file.c_str(), &model, is_cb_cancel, stepFn, stepIsUtf8Fn);
    if (boost::algorithm::iends_with(input_file, ".stl"))
        result = load_stl(input_file.c_str(), &model, nullptr, stlFn);
    else if (boost::algorithm::iends_with(input_file, ".oltp"))
        result = load_stl(input_file.c_str(), &model, nullptr, stlFn,256);
    else if (boost::algorithm::iends_with(input_file, ".obj")) {
        ObjInfo                 obj_info;
        result = load_obj(input_file.c_str(), &model, obj_info, message);
        if (result){
            unsigned char first_extruder_id;
            if (obj_info.vertex_colors.size() > 0) {
                std::vector<unsigned char> vertex_filament_ids;
                if (objFn) { // 1.result is ok and pop up a dialog
                    objFn(obj_info.vertex_colors, false, vertex_filament_ids, first_extruder_id);
                    if (vertex_filament_ids.size() > 0) {
                        result = obj_import_vertex_color_deal(vertex_filament_ids, first_extruder_id, & model);
                    }
                }
            } else if (obj_info.face_colors.size() > 0 && obj_info.has_uv_png == false) { // mtl file
                std::vector<unsigned char> face_filament_ids;
                if (objFn) { // 1.result is ok and pop up a dialog
                    objFn(obj_info.face_colors, obj_info.is_single_mtl, face_filament_ids, first_extruder_id);
                    if (face_filament_ids.size() > 0) {
                        result = obj_import_face_color_deal(face_filament_ids, first_extruder_id, &model);
                    }
                }
            } /*else if (obj_info.has_uv_png && obj_info.uvs.size() > 0) {
                boost::filesystem::path full_path(input_file);
                std::string             obj_directory = full_path.parent_path().string();
                obj_info.obj_dircetory = obj_directory;
                result = false;
                message = _L("Importing obj with png function is developing.");
            }*/
        }
    }
    else if (boost::algorithm::iends_with(input_file, ".svg"))
        result = load_svg(input_file.c_str(), &model, message);
    //BBS: remove the old .amf.xml files
    //else if (boost::algorithm::iends_with(input_file, ".amf") || boost::algorithm::iends_with(input_file, ".amf.xml"))
    else if (boost::algorithm::iends_with(input_file, ".amf"))
        //BBS: is_xxx is used for is_inches when load amf
        result = load_amf(input_file.c_str(), config, config_substitutions, &model, is_xxx);
    else if (boost::algorithm::iends_with(input_file, ".3mf") || boost::algorithm::iends_with(input_file, ".cxprj"))
        //BBS: add part plate related logic
        // BBS: backup & restore
        //FIXME options & LoadStrategy::CheckVersion ?
        //BBS: is_xxx is used for is_bbs_3mf when load 3mf
        result = load_bbs_3mf(input_file.c_str(), config, config_substitutions, &model, plate_data, project_presets, is_xxx, file_version, proFn, options, project, plate_id);
    else if (boost::algorithm::iends_with(input_file, ".dae") || boost::algorithm::iends_with(input_file, ".3ds") ||
        boost::algorithm::iends_with(input_file, ".ply") || boost::algorithm::iends_with(input_file, ".off"))
    {
        result = load_assimp_model(input_file.c_str(), &model, is_cb_cancel, stepFn);
    }
#ifdef __APPLE__
    else if (boost::algorithm::iends_with(input_file, ".usd") || boost::algorithm::iends_with(input_file, ".usda") ||
             boost::algorithm::iends_with(input_file, ".usdc") || boost::algorithm::iends_with(input_file, ".usdz") ||
             boost::algorithm::iends_with(input_file, ".abc") || boost::algorithm::iends_with(input_file, ".ply")) {
        std::string temp_stl = make_temp_stl_with_modelio(input_file);
        if (temp_stl.empty()) {
            throw Slic3r::RuntimeError("Failed to convert asset to STL via ModelIO.");
        }
        result = load_stl(temp_stl.c_str(), &model);
        delete_temp_file(temp_stl);
    }
#endif
    else
        throw Slic3r::RuntimeError(_L("Unknown file format. Input file must have .stl, .obj, .amf(.xml) extension."));

    if (is_cb_cancel) {
        Model empty_model;
        return empty_model;
    }

    if (!result) {
        if (message.empty())
            throw Slic3r::RuntimeError(_L("Loading of a model file failed."));
        else
            throw Slic3r::RuntimeError(message);
    }

    if (model.objects.empty())
        throw Slic3r::RuntimeError(_L("The supplied file couldn't be read because it's empty"));

    for (ModelObject *o : model.objects)
        o->input_file = input_file;

    if (options & LoadStrategy::AddDefaultInstances)
        model.add_default_instances();

    //BBS
    //CustomGCode::update_custom_gcode_per_print_z_from_config(model.custom_gcode_per_print_z, config);
    //BBS
    for (auto& plate_gcodes : model.plates_custom_gcodes)
        CustomGCode::check_mode_for_custom_gcode_per_print_z(plate_gcodes.second);

    sort_remove_duplicates(config_substitutions->substitutions);
    return model;
}

//BBS: add part plate related logic
// BBS: backup & restore
// Loading model from a file (3MF or AMF), not from a simple geometry file (STL or OBJ).
Model Model::read_from_archive(const std::string& input_file, DynamicPrintConfig* config, ConfigSubstitutionContext* config_substitutions, En3mfType& out_file_type, LoadStrategy options,
        PlateDataPtrs* plate_data, std::vector<Preset*>* project_presets, Semver* file_version, Import3mfProgressFn proFn, BBLProject *project)
{
    assert(config != nullptr);
    assert(config_substitutions != nullptr);

    Model model;

    struct ArchiveLoadResult
    {
        bool      result { false };
        bool      is_bbl_3mf { false };
        En3mfType file_type { En3mfType::From_Other };
    };

    bool result = false;
    bool cb_cancel = false;

    std::future<ArchiveLoadResult> fut = std::async(std::launch::async, [&]() {
        ArchiveLoadResult load_result;
        if (boost::algorithm::iends_with(input_file, ".3mf") || boost::algorithm::iends_with(input_file, ".cxprj")) {
            PrusaFileParser prusa_file_parser;
            if (prusa_file_parser.check_3mf_from_prusa(input_file)) {
                // for Prusa 3mf
                load_result.result = load_3mf(input_file.c_str(), *config, *config_substitutions, &model, true);
                load_result.file_type = En3mfType::From_Prusa;
            } else {
                // BBS: add part plate related logic
                // BBS: backup & restore
                load_result.result = load_bbs_3mf(input_file.c_str(), config, config_substitutions, &model, plate_data, project_presets, &load_result.is_bbl_3mf, file_version, nullptr, options, project);
            }
        }
        else if (boost::algorithm::iends_with(input_file, ".zip.amf")) {
            load_result.result = load_amf(input_file.c_str(), config, config_substitutions, &model, &load_result.is_bbl_3mf);
        }
        else
            throw Slic3r::RuntimeError(_L("Unknown file format. Input file must have .3mf or .zip.amf extension."));

        return load_result;
    });

    while (fut.wait_for(std::chrono::milliseconds(50)) != std::future_status::ready) {
        if (proFn) {
            proFn(IMPORT_STAGE_READ_FILES, 0, 3, cb_cancel);
        }
    }

    ArchiveLoadResult load_result = fut.get();
    result = load_result.result;
    out_file_type = load_result.file_type == En3mfType::From_Prusa ?
        En3mfType::From_Prusa :
        (load_result.is_bbl_3mf ? En3mfType::From_BBS : En3mfType::From_Other);
    if (!result)
        throw Slic3r::RuntimeError(_L("Loading of a model file failed."));

    for (ModelObject *o : model.objects) {
//        if (boost::algorithm::iends_with(input_file, ".zip.amf"))
//        {
//            // we remove the .zip part of the extension to avoid it be added to filenames when exporting
//            o->input_file = boost::ireplace_last_copy(input_file, ".zip.", ".");
//        }
//        else
            o->input_file = input_file;
    }

    if (options & LoadStrategy::AddDefaultInstances) {
        model.add_default_instances();
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ":" <<__LINE__ << boost::format("import 3mf IMPORT_STAGE_ADD_INSTANCE\n");
        if (proFn) {
            proFn(IMPORT_STAGE_ADD_INSTANCE, 0, 1, cb_cancel);
            if (cb_cancel)
                throw Slic3r::RuntimeError(_L("Canceled"));
        }
    }

    //BBS
    //CustomGCode::update_custom_gcode_per_print_z_from_config(model.custom_gcode_per_print_z, config);

    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ":" << __LINE__ << boost::format("import 3mf IMPORT_STAGE_UPDATE_GCODE\n");
    if (proFn) {
        proFn(IMPORT_STAGE_UPDATE_GCODE, 0, 1, cb_cancel);
        if (cb_cancel)
            throw Slic3r::RuntimeError(_L("Canceled"));
    }

    //BBS
    for (auto& plate_gcodes : model.plates_custom_gcodes)
        CustomGCode::check_mode_for_custom_gcode_per_print_z(plate_gcodes.second);

    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ":" << __LINE__ << boost::format("import 3mf IMPORT_STAGE_CHECK_MODE_GCODE\n");
    if (proFn) {
        proFn(IMPORT_STAGE_CHECK_MODE_GCODE, 0, 1, cb_cancel);
        if (cb_cancel)
            throw Slic3r::RuntimeError(_L("Canceled"));
    }

    handle_legacy_sla(*config);

    return model;
}

ModelObject* Model::add_object()
{
    this->objects.emplace_back(new ModelObject(this));
    return this->objects.back();
}

ModelObject* Model::add_object(const char *name, const char *path, const TriangleMesh &mesh)
{
    ModelObject* new_object = new ModelObject(this);
    this->objects.push_back(new_object);
    new_object->name = name;
    new_object->input_file = path;
    ModelVolume *new_volume = new_object->add_volume(mesh);
    new_volume->name = name;
    new_volume->source.input_file = path;
    new_volume->source.object_idx = (int)this->objects.size() - 1;
    new_volume->source.volume_idx = (int)new_object->volumes.size() - 1;
    // BBS: set extruder id to 1
    if (!new_object->config.has("extruder") || new_object->config.extruder() == 0)
        new_object->config.set_key_value("extruder", new ConfigOptionInt(1));
    new_object->invalidate_bounding_box();
    return new_object;
}

ModelObject* Model::add_object(const char *name, const char *path, TriangleMesh &&mesh)
{
    ModelObject* new_object = new ModelObject(this);
    this->objects.push_back(new_object);
    new_object->name = name;
    new_object->input_file = path;
    ModelVolume *new_volume = new_object->add_volume(std::move(mesh));
    new_volume->name = name;
    new_volume->source.input_file = path;
    new_volume->source.object_idx = (int)this->objects.size() - 1;
    new_volume->source.volume_idx = (int)new_object->volumes.size() - 1;
    // BBS: set default extruder id to 1
    if (!new_object->config.has("extruder") || new_object->config.extruder() == 0)
        new_object->config.set_key_value("extruder", new ConfigOptionInt(1));
    new_object->invalidate_bounding_box();
    return new_object;
}

ModelObject* Model::add_object(const ModelObject &other)
{
	ModelObject* new_object = ModelObject::new_clone(other);
    new_object->set_model(this);
    // BBS: set default extruder id to 1
    if (!new_object->config.has("extruder") || new_object->config.extruder() == 0)
        new_object->config.set_key_value("extruder", new ConfigOptionInt(1));
    this->objects.push_back(new_object);
    // BBS: backup
    if (need_backup) {
        if (auto model = other.get_model()) {
            auto iter = object_backup_id_map.find(other.id().id);
            if (iter != object_backup_id_map.end()) {
                object_backup_id_map.emplace(new_object->id().id, iter->second);
                object_backup_id_map.erase(iter);
                return new_object;
            }
        }
        Slic3r::save_object_mesh(*new_object);
    }
    return new_object;
}

void Model::delete_object(size_t idx)
{
    ModelObjectPtrs::iterator i = this->objects.begin() + idx;
    BOOST_LOG_TRIVIAL(warning) << "Model::delete_object(idx=" << idx << ") - Object name: '" << (*i)->name << "', ID: " << (*i)->id().id << ", volumes: " << (*i)->volumes.size() << ", instances: " << (*i)->instances.size() << ", layer_config_ranges: " << (*i)->layer_config_ranges.size();
    // BBS: backup
    Slic3r::delete_object_mesh(**i);
    delete *i;
    this->objects.erase(i);
}

bool Model::delete_object(ModelObject* object)
{
    if (object != nullptr) {
        size_t idx = 0;
        for (ModelObject *model_object : objects) {
            if (model_object == object) {
                BOOST_LOG_TRIVIAL(warning) << "Model::delete_object(ptr) - Object name: '" << model_object->name << "', ID: " << model_object->id().id << ", volumes: " << model_object->volumes.size() << ", instances: " << model_object->instances.size() << ", layer_config_ranges: " << model_object->layer_config_ranges.size() << ", idx: " << idx;
                // BBS: backup
                Slic3r::delete_object_mesh(*model_object);
                delete model_object;
                objects.erase(objects.begin() + idx);
                return true;
            }
            ++ idx;
        }
    }
    return false;
}

bool Model::delete_object(ObjectID id)
{
    if (id.id != 0) {
        size_t idx = 0;
        for (ModelObject *model_object : objects) {
            if (model_object->id() == id) {
                BOOST_LOG_TRIVIAL(warning) << "Model::delete_object(ID=" << id.id << ") - Object name: '" << model_object->name << "', volumes: " << model_object->volumes.size() << ", instances: " << model_object->instances.size() << ", layer_config_ranges: " << model_object->layer_config_ranges.size() << ", idx: " << idx;
                // BBS: backup
                Slic3r::delete_object_mesh(*model_object);
                delete model_object;
                objects.erase(objects.begin() + idx);
                return true;
            }
            ++ idx;
        }
    }
    return false;
}

void Model::clear_objects()
{
    BOOST_LOG_TRIVIAL(warning) << "Model::clear_objects() - Clearing " << this->objects.size() << " objects";
    for (ModelObject* o : this->objects) {
        BOOST_LOG_TRIVIAL(warning) << "Model::clear_objects() - Deleting object name: '" << o->name << "', ID: " << o->id().id << ", volumes: " << o->volumes.size() << ", instances: " << o->instances.size() << ", layer_config_ranges: " << o->layer_config_ranges.size();
        // BBS: backup
        Slic3r::delete_object_mesh(*o);
        delete o;
    }
    this->objects.clear();
    object_backup_id_map.clear();
    next_object_backup_id = 1;
}

// BBS: backup, reuse objects
void Model::collect_reusable_objects(std::vector<ObjectBase*>& objects)
{
    for (ModelObject* model_object : this->objects) {
        objects.push_back(model_object);
        for (ModelVolume* model_volume : model_object->volumes)
            objects.push_back(model_volume);
        std::transform(model_object->volumes.begin(),
                       model_object->volumes.end(),
                       std::back_inserter(model_object->volume_ids),
                       std::mem_fn(&ObjectBase::id));
        model_object->volumes.clear();
    }
    // we never own these objects
    this->objects.clear();
}

void Model::set_object_backup_id(ModelObject const& object, int uuid)
{
    object_backup_id_map[object.id().id] = uuid;
    if (uuid >= next_object_backup_id) next_object_backup_id = uuid + 1;
}

int Model::get_object_backup_id(ModelObject const& object)
{
    auto i = object_backup_id_map.find(object.id().id);
    if (i == object_backup_id_map.end()) {
        i = object_backup_id_map.insert(std::make_pair(object.id().id, next_object_backup_id++)).first;
    }
    return i->second;
}

int Model::get_object_backup_id(ModelObject const& object) const
{
    return object_backup_id_map.find(object.id().id)->second;
}

void Model::delete_material(t_model_material_id material_id)
{
    ModelMaterialMap::iterator i = this->materials.find(material_id);
    if (i != this->materials.end()) {
        delete i->second;
        this->materials.erase(i);
    }
}

void Model::clear_materials()
{
    for (auto &m : this->materials)
        delete m.second;
    this->materials.clear();
}

ModelMaterial* Model::add_material(t_model_material_id material_id)
{
    assert(! material_id.empty());
    ModelMaterial* material = this->get_material(material_id);
    if (material == nullptr)
        material = this->materials[material_id] = new ModelMaterial(this);
    return material;
}

ModelMaterial* Model::add_material(t_model_material_id material_id, const ModelMaterial &other)
{
    assert(! material_id.empty());
    // delete existing material if any
    ModelMaterial* material = this->get_material(material_id);
    delete material;
    // set new material
	material = new ModelMaterial(other);
	material->set_model(this);
    this->materials[material_id] = material;
    return material;
}

// makes sure all objects have at least one instance
bool Model::add_default_instances()
{
    // apply a default position to all objects not having one
    for (ModelObject *o : this->objects)
        if (o->instances.empty())
            o->add_instance();
    return true;
}

// this returns the bounding box of the *transformed* instances
BoundingBoxf3 Model::bounding_box_approx() const
{
    BoundingBoxf3 bb;
    for (ModelObject *o : this->objects)
        bb.merge(o->bounding_box_approx());
    return bb;
}

BoundingBoxf3 Model::bounding_box_exact() const
{
    BoundingBoxf3 bb;
    for (ModelObject *o : this->objects)
        bb.merge(o->bounding_box_exact());
    return bb;
}

double Model::max_z() const
{
    double z = 0;
    for (ModelObject *o : this->objects)
        z = std::max(z, o->max_z());
    return z;
}

unsigned int Model::update_print_volume_state(const BuildVolume &build_volume)
{
    unsigned int num_printable = 0;
    for (ModelObject* model_object : this->objects)
        num_printable += model_object->update_instances_print_volume_state(build_volume);
    //BBS: add logs for build_volume
    const BoundingBoxf3& print_volume = build_volume.bounding_volume();
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(", print_volume {%1%, %2%, %3%} to {%4%, %5%, %6%}, got %7% printable istances")\
        %print_volume.min.x() %print_volume.min.y() %print_volume.min.z()%print_volume.max.x() %print_volume.max.y() %print_volume.max.z() %num_printable;
    return num_printable;
}

bool Model::center_instances_around_point(const Vec2d &point)
{
    BoundingBoxf3 bb;
    for (ModelObject *o : this->objects)
        for (size_t i = 0; i < o->instances.size(); ++ i)
            bb.merge(o->instance_bounding_box(i, false));

    Vec2d shift2 = point - to_2d(bb.center());
	if (std::abs(shift2(0)) < EPSILON && std::abs(shift2(1)) < EPSILON)
		// No significant shift, don't do anything.
		return false;

	Vec3d shift3 = Vec3d(shift2(0), shift2(1), 0.0);
	for (ModelObject *o : this->objects) {
		for (ModelInstance *i : o->instances)
			i->set_offset(i->get_offset() + shift3);
		o->invalidate_bounding_box();
	}
	return true;
}

void Model::translate(coordf_t x, coordf_t y, coordf_t z)
{
    for (ModelObject* o : this->objects)
        o->translate(x, y, z);
}

// flattens everything to a single mesh
TriangleMesh Model::mesh() const
{
    TriangleMesh mesh;
    for (const ModelObject *o : this->objects)
        mesh.merge(o->mesh());
    return mesh;
}

void Model::duplicate_objects_grid(size_t x, size_t y, coordf_t dist)
{
    if (this->objects.size() > 1) throw "Grid duplication is not supported with multiple objects";
    if (this->objects.empty()) throw "No objects!";

    ModelObject* object = this->objects.front();
    object->clear_instances();

    Vec3d ext_size = object->bounding_box_exact().size() + dist * Vec3d::Ones();

    for (size_t x_copy = 1; x_copy <= x; ++x_copy) {
        for (size_t y_copy = 1; y_copy <= y; ++y_copy) {
            ModelInstance* instance = object->add_instance();
            instance->set_offset(Vec3d(ext_size(0) * (double)(x_copy - 1), ext_size(1) * (double)(y_copy - 1), 0.0));
        }
    }
}

bool Model::looks_like_multipart_object() const
{
    if (this->objects.size() <= 1)
        return false;
    double zmin = std::numeric_limits<double>::max();
    for (const ModelObject *obj : this->objects) {
        if (obj->volumes.size() > 1 || obj->config.keys().size() > 1)
            return false;

        double zmin_this = obj->min_z();
        if (zmin == std::numeric_limits<double>::max())
            zmin = zmin_this;
        else if (std::abs(zmin - zmin_this) > EPSILON)
            // The Object don't share zmin.
            return true;
    }
    return false;
}

// Generate next extruder ID string, in the range of (1, max_extruders).
static inline int auto_extruder_id(unsigned int max_extruders, unsigned int &cntr)
{
    int out = ++ cntr;
    if (cntr == max_extruders)
    	cntr = 0;
    return out;
}

void Model::convert_multipart_object(unsigned int max_extruders)
{
    assert(this->objects.size() >= 2);
    if (this->objects.size() < 2)
        return;

    ModelObject* object = new ModelObject(this);
    object->input_file = this->objects.front()->input_file;
    object->name = boost::filesystem::path(this->objects.front()->input_file).stem().string();
    //FIXME copy the config etc?

    unsigned int extruder_counter = 0;

	for (const ModelObject* o : this->objects)
    	for (const ModelVolume* v : o->volumes) {
            // If there are more than one object, put all volumes together
            // Each object may contain any number of volumes and instances
            // The volumes transformations are relative to the object containing them...
            Geometry::Transformation trafo_volume = v->get_transformation();
            // Revert the centering operation.
            trafo_volume.set_offset(trafo_volume.get_offset() - o->origin_translation);
            int counter = 1;
            auto copy_volume = [o, v, max_extruders, &counter, &extruder_counter](ModelVolume *new_v) {
                assert(new_v != nullptr);
                new_v->name = (counter > 1) ? o->name + "_" + std::to_string(counter++) : o->name;
                //BBS: Use extruder priority: volumn > object > default
                if (v->config.option("extruder"))
                    new_v->config.set("extruder", v->config.extruder());
                else if (o->config.option("extruder"))
                    new_v->config.set("extruder", o->config.extruder());

                return new_v;
            };
            if (o->instances.empty()) {
                copy_volume(object->add_volume(*v))->set_transformation(trafo_volume);
            } else {
                for (const ModelInstance* i : o->instances)
                    // ...so, transform everything to a common reference system (world)
                    copy_volume(object->add_volume(*v))->set_transformation(i->get_transformation() * trafo_volume);
            }
        }

    // commented-out to fix #2868
//    object->add_instance();
//    object->instances[0]->set_offset(object->raw_mesh_bounding_box().center());

    this->clear_objects();
    this->objects.push_back(object);
}

static constexpr const double volume_threshold_inches = 8.0; // 9 = 2*2*2;

bool Model::looks_like_imperial_units() const
{
    if (this->objects.size() == 0)
        return false;

    for (ModelObject* obj : this->objects)
        if (obj->get_object_stl_stats().volume < volume_threshold_inches) {
            if (!obj->is_cut())
                return true;
            bool all_cut_parts_look_like_imperial_units = true;
            for (ModelObject* obj_other : this->objects) {
                if (obj_other == obj)
                    continue;
                if (obj_other->cut_id.is_equal(obj->cut_id) && obj_other->get_object_stl_stats().volume >= volume_threshold_inches) {
                    all_cut_parts_look_like_imperial_units = false;
                    break;
                }
            }
            if (all_cut_parts_look_like_imperial_units)
                return true;
        }

    return false;
}
void Model::convert_from_creality5(Vec2d bed_size,Vec2d new_bed_size,int plate_size)
{
    if (objects.empty())
        return;
    Vec2d offset = new_bed_size - bed_size - Vec2d(50,50);
   
    for (ModelObject* obj : objects)
    {
            if (obj != nullptr)
            {
                //判断对象属于哪一行
                float posx = obj->bounding_box_exact().center().x();
                float posy = obj->bounding_box_exact().center().y();
                int lrow = posx / (bed_size.x() + 50);
                int lcol = (bed_size.y() - posy) / (bed_size.y() + 50);
                BOOST_LOG_TRIVIAL(info) << boost::format("posx:%1%,posy:%2%")%lrow%lcol;
                obj->translate_instances(Vec3d(lrow*offset.x(), -lcol*offset.y(), 0.0));
            }
    }

}
void Model::convert_from_imperial_units(bool only_small_volumes)
{
    static constexpr const float in_to_mm = 25.4f;
    for (ModelObject* obj : this->objects)
        if (! only_small_volumes || obj->get_object_stl_stats().volume < volume_threshold_inches) {
            obj->scale_mesh_after_creation(in_to_mm);
            for (ModelVolume* v : obj->volumes) {
                assert(! v->source.is_converted_from_meters);
                v->source.is_converted_from_inches = true;
            }
        }
}

static constexpr const double volume_threshold_meters = 0.008; // 0.008 = 0.2*0.2*0.2

bool Model::looks_like_saved_in_meters() const
{
    if (this->objects.size() == 0)
        return false;

    for (ModelObject* obj : this->objects)
        if (obj->get_object_stl_stats().volume < volume_threshold_meters)
            return true;

    return false;
}

void Model::convert_from_meters(bool only_small_volumes)
{
    static constexpr const double m_to_mm = 1000;
    for (ModelObject* obj : this->objects)
        if (! only_small_volumes || obj->get_object_stl_stats().volume < volume_threshold_meters) {
            obj->scale_mesh_after_creation(m_to_mm);
            for (ModelVolume* v : obj->volumes) {
                assert(! v->source.is_converted_from_inches);
                v->source.is_converted_from_meters = true;
            }
        }
}

static constexpr const double zero_volume = 0.0000000001;

int Model::removed_objects_with_zero_volume()
{
    if (objects.size() == 0)
        return 0;

    int removed = 0;
    for (int i = int(objects.size()) - 1; i >= 0; i--)
        if (objects[i]->get_object_stl_stats().volume < zero_volume) {
            delete_object(size_t(i));
            removed++;
        }
    return removed;
}

void Model::adjust_min_z()
{
    if (objects.empty())
        return;

    if (this->bounding_box_exact().min.z() < 0.0)
    {
        for (ModelObject* obj : objects)
        {
            if (obj != nullptr)
            {
                coordf_t obj_min_z = obj->min_z();
                if (obj_min_z < 0.0)
                    obj->translate_instances(Vec3d(0.0, 0.0, -obj_min_z));
            }
        }
    }
}

void Model::print_info() const
{
    for (const ModelObject* o : this->objects)
        o->print_info();
}

// Propose a filename including path derived from the ModelObject's input path.
// If object's name is filled in, use the object name, otherwise use the input name.
std::string Model::propose_export_file_name_and_path() const
{
    std::string input_file;
    for (const ModelObject *model_object : this->objects)
        for (ModelInstance *model_instance : model_object->instances)
            if (model_instance->is_printable()) {
                input_file = model_object->get_export_filename();

                if (!input_file.empty())
                    goto end;
                // Other instances will produce the same name, skip them.
                break;
            }
end:
    return input_file;
}

//BBS: add auxiliary files temp path
// BBS: backup all in one dir
std::string Model::get_auxiliary_file_temp_path()
{
    return get_backup_path("Auxiliaries");
}

// BBS: backup dir
std::string Model::get_backup_path()
{
    if (backup_path.empty())
    {
        auto pid = get_current_pid();
        boost::filesystem::path parent_path(temporary_dir());
        std::time_t t = std::time(0);
        std::tm* now_time = std::localtime(&t);
        std::stringstream buf;
        buf << "/crealityprint_model/";
        buf << std::put_time(now_time, "%a_%b_%d/%H_%M_%S#");
        buf << pid << "#";
        buf << this->id().id;

        backup_path = parent_path.string() + buf.str();
        BOOST_LOG_TRIVIAL(info) << boost::format("model %1%, id %2%, backup_path empty, set to %3%")%this%this->id().id%backup_path;
        boost::filesystem::path temp_path(backup_path);
        if (boost::filesystem::exists(temp_path))
        {
            BOOST_LOG_TRIVIAL(info) << boost::format("model %1%, id %2%, remove previous %3%")%this%this->id().id%backup_path;
            boost::filesystem::remove_all(temp_path);
        }
    }
    boost::filesystem::path temp_path(backup_path);
    try {
        if (!boost::filesystem::exists(temp_path))
        {
            BOOST_LOG_TRIVIAL(info) << "create /3D/Objects in " << temp_path;
            boost::filesystem::create_directories(backup_path + "/3D/Objects");
            BOOST_LOG_TRIVIAL(info) << "create /Metadata in " << temp_path;
            boost::filesystem::create_directories(backup_path + "/Metadata");
            BOOST_LOG_TRIVIAL(info) << "create /lock.txt in " << temp_path;
            save_string_file(backup_path + "/lock.txt",
                boost::lexical_cast<std::string>(get_current_pid()));
        }
    } catch (std::exception &ex) {
        BOOST_LOG_TRIVIAL(error) << "Failed to create backup path" << temp_path << ": " << ex.what();
    }

    return backup_path;
}

void Model::remove_backup_path_if_exist()
{
    if (!backup_path.empty()) {
        boost::filesystem::path temp_path(backup_path);
        if (boost::filesystem::exists(temp_path))
        {
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format("model %1%, id %2% remove backup_path %3%")%this%this->id().id%backup_path;
            boost::filesystem::remove_all(temp_path);
        }
	backup_path.clear();
    }
}

std::string Model::get_backup_path(const std::string &sub_path)
{
    auto path = get_backup_path() + "/" + sub_path;
    try {
        if (!boost::filesystem::exists(path)) {
            BOOST_LOG_TRIVIAL(info) << "create missing sub_path" << path;
            boost::filesystem::create_directories(path);
        }
    } catch (std::exception &ex) {
        BOOST_LOG_TRIVIAL(error) << "Failed to create missing sub_path" << path << ": " << ex.what();
    }
    return path;
}

void Model::set_backup_path(std::string const& path)
{
    if (backup_path == path)
        return;
    if ("detach" == path) {
        backup_path.clear();
        return;
    }
    if (!backup_path.empty()) {
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__<<boost::format(", model %1%, id %2%, remove previous backup %3%")%this%this->id().id%backup_path;
        Slic3r::remove_backup(*this, true);
    }
    backup_path = path;
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__<<boost::format(", model %1%, id %2%, set backup to %3%")%this%this->id().id%backup_path;
}

void Model::load_from(Model& model)
{
    set_backup_path(model.get_backup_path());
    model.backup_path.clear();
    object_backup_id_map = model.object_backup_id_map;
    next_object_backup_id = model.next_object_backup_id;
    design_info = model.design_info;
    stl_design_id = model.stl_design_id;
    stl_design_country = model.stl_design_country;
    model_info  = model.model_info;
    profile_info  = model.profile_info;
    mk_name = model.mk_name;
    mk_version = model.mk_version;
    md_name = model.md_name;
    md_value = model.md_value;
    model.design_info.reset();
    model.model_info.reset();
    model.profile_info.reset();
    model.calib_pa_pattern.reset();
}

// BBS: backup
void Model::set_need_backup()
{
    need_backup = true;
}

std::string Model::propose_export_file_name_and_path(const std::string &new_extension) const
{
    return boost::filesystem::path(this->propose_export_file_name_and_path()).replace_extension(new_extension).string();
}

bool Model::is_fdm_support_painted() const
{
    return std::any_of(this->objects.cbegin(), this->objects.cend(), [](const ModelObject *mo) { return mo->is_fdm_support_painted(); });
}

bool Model::is_seam_painted() const
{
    return std::any_of(this->objects.cbegin(), this->objects.cend(), [](const ModelObject *mo) { return mo->is_seam_painted(); });
}

bool Model::is_mm_painted() const
{
    return std::any_of(this->objects.cbegin(), this->objects.cend(), [](const ModelObject *mo) { return mo->is_mm_painted(); });
}

bool Model::is_fuzzy_skin_painted() const
{
    return std::any_of(this->objects.cbegin(), this->objects.cend(), [](const ModelObject *mo) { return mo->is_fuzzy_skin_painted(); });
}

static void add_cut_volume(TriangleMesh& mesh, ModelObject* object, const ModelVolume* src_volume, const Transform3d& cut_matrix, const std::string& suffix = {}, ModelVolumeType type = ModelVolumeType::MODEL_PART)
{
    if (mesh.empty())
        return;

    mesh.transform(cut_matrix);
    ModelVolume* vol = object->add_volume(mesh);
    vol->set_type(type);

    vol->name = src_volume->name + suffix;
    // Don't copy the config's ID.
    vol->config.assign_config(src_volume->config);
    assert(vol->config.id().valid());
    assert(vol->config.id() != src_volume->config.id());
    vol->set_material(src_volume->material_id(), *src_volume->material());
    vol->cut_info = src_volume->cut_info;
}

//BBS
// BBS set print speed table and find maximum speed
void Model::setPrintSpeedTable(const DynamicPrintConfig& config, const PrintConfig& print_config) {
    //Slic3r::DynamicPrintConfig config = wxGetApp().preset_bundle->full_config();
    printSpeedMap.maxSpeed = 0;
    if (config.has("inner_wall_speed")) {
        printSpeedMap.perimeterSpeed = config.opt_float("inner_wall_speed");
        if (printSpeedMap.perimeterSpeed > printSpeedMap.maxSpeed)
            printSpeedMap.maxSpeed = printSpeedMap.perimeterSpeed;
    }
    if (config.has("outer_wall_speed")) {
        printSpeedMap.externalPerimeterSpeed = config.opt_float("outer_wall_speed");
        printSpeedMap.maxSpeed = std::max(printSpeedMap.maxSpeed, printSpeedMap.externalPerimeterSpeed);
    }
    if (config.has("sparse_infill_speed")) {
        printSpeedMap.infillSpeed = config.opt_float("sparse_infill_speed");
        if (printSpeedMap.infillSpeed > printSpeedMap.maxSpeed)
            printSpeedMap.maxSpeed = printSpeedMap.infillSpeed;
    }
    if (config.has("internal_solid_infill_speed")) {
        printSpeedMap.solidInfillSpeed = config.opt_float("internal_solid_infill_speed");
        if (printSpeedMap.solidInfillSpeed > printSpeedMap.maxSpeed)
            printSpeedMap.maxSpeed = printSpeedMap.solidInfillSpeed;
    }
    if (config.has("top_surface_speed")) {
        printSpeedMap.topSolidInfillSpeed = config.opt_float("top_surface_speed");
        if (printSpeedMap.topSolidInfillSpeed > printSpeedMap.maxSpeed)
            printSpeedMap.maxSpeed = printSpeedMap.topSolidInfillSpeed;
    }
    if (config.has("support_speed")) {
        printSpeedMap.supportSpeed = config.opt_float("support_speed");

        if (printSpeedMap.supportSpeed > printSpeedMap.maxSpeed)
            printSpeedMap.maxSpeed = printSpeedMap.supportSpeed;
    }


    //auto& print = wxGetApp().plater()->get_partplate_list().get_current_fff_print();
    //auto print_config = print.config();
    //printSpeedMap.bed_poly.points = get_bed_shape(*(wxGetApp().plater()->config()));
    printSpeedMap.bed_poly.points = get_bed_shape(config);
    Pointfs excluse_area_points = print_config.bed_exclude_area.values;
    Polygons exclude_polys;
    Polygon exclude_poly;
    for (int i = 0; i < excluse_area_points.size(); i++) {
        auto pt = excluse_area_points[i];
        exclude_poly.points.emplace_back(scale_(pt.x()), scale_(pt.y()));
        if (i % 4 == 3) {  // exclude areas are always rectangle
            exclude_polys.push_back(exclude_poly);
            exclude_poly.points.clear();
        }
    }
    printSpeedMap.bed_poly = diff({ printSpeedMap.bed_poly }, exclude_polys)[0];
}

// find temperature of heatend and bed and matierial of an given extruder
void Model::setExtruderParams(const DynamicPrintConfig& config, int extruders_count) {
    extruderParamsMap.clear();
    //Slic3r::DynamicPrintConfig config = wxGetApp().preset_bundle->full_config();
    // BBS
    //int numExtruders = wxGetApp().preset_bundle->filament_presets.size();
    for (unsigned int i = 0; i != extruders_count; ++i) {
        std::string matName = "";
        // BBS
        int bedTemp = 35;
        double endTemp = 0.f;
        if (config.has("filament_type")) {
            matName = config.opt_string("filament_type", i);
        }
        if (config.has("nozzle_temperature")) {
            endTemp = config.opt_int("nozzle_temperature", i);
        }

        // FIXME: curr_bed_type is now a plate config rather than a global config.
        // Currently bed temp is not used for brim generation, so just comment it for now.
#if 0
        if (config.has("curr_bed_type")) {
            BedType curr_bed_type = config.opt_enum<BedType>("curr_bed_type");
            bedTemp = config.opt_int(get_bed_temp_key(curr_bed_type), i);
        }
#endif
        if (i == 0) extruderParamsMap.insert({ i,{matName, bedTemp, endTemp} });
        extruderParamsMap.insert({ i + 1,{matName, bedTemp, endTemp} });
    }
}

static void get_real_filament_id(const unsigned char &id, std::string &result) {
    if (id < CONST_FILAMENTS.size()) {
        result = CONST_FILAMENTS[id];
    } else {
        result = "";//error
    }
};

bool Model::obj_import_vertex_color_deal(const std::vector<unsigned char> &vertex_filament_ids, const unsigned char &first_extruder_id, Model *model)
{
    if (vertex_filament_ids.size() == 0) {
        return false;
    }
    // 2.generate mmu_segmentation_facets
    if (model->objects.size() == 1 ) {
        auto obj = model->objects[0];
        obj->config.set("extruder", first_extruder_id);
        if (obj->volumes.size() == 1) {
            enum VertexColorCase {
                _3_SAME_COLOR,
                _3_DIFF_COLOR,
                _2_SAME_1_DIFF_COLOR,
            };
            auto calc_vertex_color_case = [](const unsigned char &c0, const unsigned char &c1, const unsigned char &c2, VertexColorCase &vertex_color_case,
                                             unsigned char &iso_index) {
                if (c0 == c1 && c1 == c2) {
                    vertex_color_case = VertexColorCase::_3_SAME_COLOR;
                } else if (c0 != c1 && c1 != c2 && c0 != c2) {
                    vertex_color_case = VertexColorCase::_3_DIFF_COLOR;
                } else if (c0 == c1) {
                    vertex_color_case = _2_SAME_1_DIFF_COLOR;
                    iso_index         = 2;
                } else if (c1 == c2) {
                    vertex_color_case = _2_SAME_1_DIFF_COLOR;
                    iso_index         = 0;
                } else if (c0 == c2) {
                    vertex_color_case = _2_SAME_1_DIFF_COLOR;
                    iso_index         = 1;
                } else {
                    std::cout << "error";
                }
            };
            auto calc_tri_area = [](const Vec3f &v0, const Vec3f &v1, const Vec3f &v2) {
                return std::abs((v0 - v1).cross(v0 - v2).norm()) / 2;
            };
            auto volume = obj->volumes[0];
            volume->config.set("extruder", first_extruder_id);
            auto face_count = volume->mesh().its.indices.size();
            volume->mmu_segmentation_facets.reserve(face_count);
            if (volume->mesh().its.vertices.size() != vertex_filament_ids.size()) {
                return false;
            }
            for (size_t i = 0; i < volume->mesh().its.indices.size(); i++) {
                auto face   = volume->mesh().its.indices[i];
                auto filament_id0 = vertex_filament_ids[face[0]];
                auto filament_id1 = vertex_filament_ids[face[1]];
                auto filament_id2 = vertex_filament_ids[face[2]];
                if (filament_id0 <= 1 && filament_id1 <= 1 && filament_id2 <= 2) {
                    continue;
                }
                if (i == 0) {
                    std::cout << "";
                }
                VertexColorCase vertex_color_case;
                unsigned char iso_index;
                calc_vertex_color_case(filament_id0, filament_id1, filament_id2, vertex_color_case, iso_index);
                switch (vertex_color_case) {
                case _3_SAME_COLOR: {
                    std::string result;
                    get_real_filament_id(filament_id0, result);
                    volume->mmu_segmentation_facets.set_triangle_from_string(i, result);
                    break;
                }
                case _3_DIFF_COLOR: {
                    std::string result0, result1, result2;
                    get_real_filament_id(filament_id0, result0);
                    get_real_filament_id(filament_id1, result1);
                    get_real_filament_id(filament_id2, result2);

                    auto v0 = volume->mesh().its.vertices[face[0]];
                    auto v1 = volume->mesh().its.vertices[face[1]];
                    auto v2 = volume->mesh().its.vertices[face[2]];
                    auto                 dir_0_1  = (v1 - v0).normalized();
                    auto                 dir_0_2  = (v2 - v0).normalized();
                    float                sita0    = acos(dir_0_1.dot(dir_0_2));
                    auto                 dir_1_0  = -dir_0_1;
                    auto                 dir_1_2  = (v2 - v1).normalized();
                    float                sita1    = acos(dir_1_0.dot(dir_1_2));
                    float                sita2    = PI - sita0 - sita1;
                    std::array<float, 3> sitas    = {sita0, sita1, sita2};
                    float                max_sita = sitas[0];
                    int                  max_sita_vertex_index = 0;
                    for (size_t j = 1; j < sitas.size(); j++) {
                        if (sitas[j] > max_sita) {
                            max_sita_vertex_index = j;
                            max_sita = sitas[j];
                        }
                    }
                    if (max_sita_vertex_index == 0) {
                        volume->mmu_segmentation_facets.set_triangle_from_string(i, result0 + result1 + result2 + (result1 + result2 + "5" )+ "3"); //"1C0C2C0C1C13"
                    } else if (max_sita_vertex_index == 1) {
                        volume->mmu_segmentation_facets.set_triangle_from_string(i, result0 + result1 + result2 + (result0 + result2 + "9") + "3");
                    } else{// if (max_sita_vertex_index == 2)
                        volume->mmu_segmentation_facets.set_triangle_from_string(i, result0 + result1 + result2 + (result1 + result0 + "1") + "3");
                    }
                    break;
                }
                case _2_SAME_1_DIFF_COLOR: {
                    std::string result0, result1, result2;
                    get_real_filament_id(filament_id0, result0);
                    get_real_filament_id(filament_id1, result1);
                    get_real_filament_id(filament_id2, result2);
                    if (iso_index == 0) {
                        volume->mmu_segmentation_facets.set_triangle_from_string(i, result0 + result1 + result1 + "2");
                    } else if (iso_index == 1) {
                        volume->mmu_segmentation_facets.set_triangle_from_string(i, result1 + result0 + result0 + "6");
                    } else if (iso_index == 2) {
                        volume->mmu_segmentation_facets.set_triangle_from_string(i, result2 + result0 + result0 + "A");
                    }
                    break;
                }
                default: break;
                }
            }
            return true;
        }
    }
    return false;
}

bool Model::obj_import_face_color_deal(const std::vector<unsigned char> &face_filament_ids, const unsigned char &first_extruder_id, Model *model)
{
    if (face_filament_ids.size() == 0) { return false; }
    // 2.generate mmu_segmentation_facets
    if (model->objects.size() == 1) {
        auto obj = model->objects[0];
        obj->config.set("extruder", first_extruder_id);
        if (obj->volumes.size() == 1) {
            auto volume        = obj->volumes[0];
            volume->config.set("extruder", first_extruder_id);
            auto face_count    = volume->mesh().its.indices.size();
            volume->mmu_segmentation_facets.reserve(face_count);
            if (volume->mesh().its.indices.size() != face_filament_ids.size()) { return false; }
            for (size_t i = 0; i < volume->mesh().its.indices.size(); i++) {
                auto face         = volume->mesh().its.indices[i];
                auto filament_id = face_filament_ids[i];
                // BUG FIX: filament_id=1 should NOT be skipped!
                // filament_id=1 corresponds to EnforcerBlockerType::Extruder1 (yellow)
                // The original code skipped when filament_id <= 1, which incorrectly skipped valid filament_id=1
                if (filament_id < 1) { continue; }
                std::string result;
                get_real_filament_id(filament_id, result);
                volume->mmu_segmentation_facets.set_triangle_from_string(i, result);
            }
            return true;
        }
    }
    return false;
}

// update the maxSpeed of an object if it is different from the global configuration
double Model::findMaxSpeed(const ModelObject* object) {
    auto objectKeys = object->config.keys();
    double objMaxSpeed = -1.;
    if (objectKeys.empty())
        return Model::printSpeedMap.maxSpeed;
    double perimeterSpeedObj = Model::printSpeedMap.perimeterSpeed;
    double externalPerimeterSpeedObj = Model::printSpeedMap.externalPerimeterSpeed;
    double infillSpeedObj = Model::printSpeedMap.infillSpeed;
    double solidInfillSpeedObj = Model::printSpeedMap.solidInfillSpeed;
    double topSolidInfillSpeedObj = Model::printSpeedMap.topSolidInfillSpeed;
    double supportSpeedObj = Model::printSpeedMap.supportSpeed;
    double smallPerimeterSpeedObj = Model::printSpeedMap.smallPerimeterSpeed;
    for (std::string objectKey : objectKeys) {
        if (objectKey == "inner_wall_speed"){
            perimeterSpeedObj = object->config.opt_float(objectKey);
            externalPerimeterSpeedObj = Model::printSpeedMap.externalPerimeterSpeed / Model::printSpeedMap.perimeterSpeed * perimeterSpeedObj;
        }
        if (objectKey == "sparse_infill_speed")
            infillSpeedObj = object->config.opt_float(objectKey);
        if (objectKey == "internal_solid_infill_speed")
            solidInfillSpeedObj = object->config.opt_float(objectKey);
        if (objectKey == "top_surface_speed")
            topSolidInfillSpeedObj = object->config.opt_float(objectKey);
        if (objectKey == "support_speed")
            supportSpeedObj = object->config.opt_float(objectKey);
        if (objectKey == "outer_wall_speed")
            externalPerimeterSpeedObj = object->config.opt_float(objectKey);
        if (objectKey == "small_perimeter_speed")
            smallPerimeterSpeedObj = object->config.opt_float(objectKey);
    }
    objMaxSpeed = std::max(perimeterSpeedObj, std::max(externalPerimeterSpeedObj, std::max(infillSpeedObj, std::max(solidInfillSpeedObj, std::max(topSolidInfillSpeedObj, std::max(supportSpeedObj, std::max(smallPerimeterSpeedObj, objMaxSpeed)))))));
    if (objMaxSpeed <= 0) objMaxSpeed = 250.;
    return objMaxSpeed;
}

// BBS: thermal length is calculated according to the material of a volume
double Model::getThermalLength(const ModelVolume* modelVolumePtr) {
    double thermalLength = 200.;
    auto aa = modelVolumePtr->extruder_id();
    if (Model::extruderParamsMap.find(aa) != Model::extruderParamsMap.end()) {
        if (Model::extruderParamsMap.at(aa).materialName == "ABS" ||
            Model::extruderParamsMap.at(aa).materialName == "PA-CF" ||
            Model::extruderParamsMap.at(aa).materialName == "PET-CF") {
            thermalLength = 100;
        }
        if (Model::extruderParamsMap.at(aa).materialName == "PC") {
            thermalLength = 40;
        }
        if (Model::extruderParamsMap.at(aa).materialName == "TPU") {
            thermalLength = 1000;
        }

    }
    return thermalLength;
}

// BBS: thermal length calculation for a group of volumes
double Model::getThermalLength(const std::vector<ModelVolume*> modelVolumePtrs)
{
    double thermalLength = 1250.;

    for (const auto& modelVolumePtr : modelVolumePtrs) {
        if (modelVolumePtr != nullptr) {
            // the thermal length of a group is decided by the volume with shortest thermal length
            thermalLength = std::min(thermalLength, getThermalLength(modelVolumePtr));
        }
    }
    return thermalLength;
}

// Test whether the two models contain the same number of ModelObjects with the same set of IDs
// ordered in the same order. In that case it is not necessary to kill the background processing.
bool model_object_list_equal(const Model &model_old, const Model &model_new)
{
    if (model_old.objects.size() != model_new.objects.size())
        return false;
    for (size_t i = 0; i < model_old.objects.size(); ++ i)
        if (model_old.objects[i]->id() != model_new.objects[i]->id())
            return false;
    return true;
}

// Test whether the new model is just an extension of the old model (new objects were added
// to the end of the original list. In that case it is not necessary to kill the background processing.
bool model_object_list_extended(const Model &model_old, const Model &model_new)
{
    if (model_old.objects.size() >= model_new.objects.size())
        return false;
    for (size_t i = 0; i < model_old.objects.size(); ++ i)
        if (model_old.objects[i]->id() != model_new.objects[i]->id())
            return false;
    return true;
}

template<typename TypeFilterFn>
bool model_volume_list_changed(const ModelObject &model_object_old, const ModelObject &model_object_new, TypeFilterFn type_filter)
{
    size_t i_old, i_new;
    for (i_old = 0, i_new = 0; i_old < model_object_old.volumes.size() && i_new < model_object_new.volumes.size();) {
        const ModelVolume &mv_old = *model_object_old.volumes[i_old];
        const ModelVolume &mv_new = *model_object_new.volumes[i_new];
        if (! type_filter(mv_old.type())) {
            ++ i_old;
            continue;
        }
        if (! type_filter(mv_new.type())) {
            ++ i_new;
            continue;
        }
        if (mv_old.type() != mv_new.type() || mv_old.id() != mv_new.id())
            return true;
        //FIXME test for the content of the mesh!
        if (! mv_old.get_matrix().isApprox(mv_new.get_matrix()))
            return true;
        ++ i_old;
        ++ i_new;
    }
    for (; i_old < model_object_old.volumes.size(); ++ i_old) {
        const ModelVolume &mv_old = *model_object_old.volumes[i_old];
        if (type_filter(mv_old.type()))
            // ModelVolume was deleted.
            return true;
    }
    for (; i_new < model_object_new.volumes.size(); ++ i_new) {
        const ModelVolume &mv_new = *model_object_new.volumes[i_new];
        if (type_filter(mv_new.type()))
            // ModelVolume was added.
            return true;
    }
    return false;
}

bool model_volume_list_changed(const ModelObject &model_object_old, const ModelObject &model_object_new, const ModelVolumeType type)
{
    return model_volume_list_changed(model_object_old, model_object_new, [type](const ModelVolumeType t) { return t == type; });
}

bool model_volume_list_changed(const ModelObject &model_object_old, const ModelObject &model_object_new, const std::initializer_list<ModelVolumeType> &types)
{
    return model_volume_list_changed(model_object_old, model_object_new, [&types](const ModelVolumeType t) {
        return std::find(types.begin(), types.end(), t) != types.end();
    });
}

template< typename TypeFilterFn, typename CompareFn>
bool model_property_changed(const ModelObject &model_object_old, const ModelObject &model_object_new, TypeFilterFn type_filter, CompareFn compare)
{
    assert(! model_volume_list_changed(model_object_old, model_object_new, type_filter));
    size_t i_old, i_new;
    for (i_old = 0, i_new = 0; i_old < model_object_old.volumes.size() && i_new < model_object_new.volumes.size();) {
        const ModelVolume &mv_old = *model_object_old.volumes[i_old];
        const ModelVolume &mv_new = *model_object_new.volumes[i_new];
        if (! type_filter(mv_old.type())) {
            ++ i_old;
            continue;
        }
        if (! type_filter(mv_new.type())) {
            ++ i_new;
            continue;
        }
        assert(mv_old.type() == mv_new.type() && mv_old.id() == mv_new.id());
        if (! compare(mv_old, mv_new))
            return true;
        ++ i_old;
        ++ i_new;
    }
    return false;
}

bool model_custom_supports_data_changed(const ModelObject& mo, const ModelObject& mo_new)
{
    return model_property_changed(mo, mo_new,
        [](const ModelVolumeType t) { return t == ModelVolumeType::MODEL_PART; },
        [](const ModelVolume &mv_old, const ModelVolume &mv_new){ return mv_old.supported_facets.timestamp_matches(mv_new.supported_facets); });
}

bool model_custom_seam_data_changed(const ModelObject& mo, const ModelObject& mo_new)
{
    return model_property_changed(mo, mo_new,
        [](const ModelVolumeType t) { return t == ModelVolumeType::MODEL_PART; },
        [](const ModelVolume &mv_old, const ModelVolume &mv_new){ return mv_old.seam_facets.timestamp_matches(mv_new.seam_facets); });
}

bool model_mmu_segmentation_data_changed(const ModelObject& mo, const ModelObject& mo_new)
{
    return model_property_changed(mo, mo_new,
        [](const ModelVolumeType t) { return t == ModelVolumeType::MODEL_PART; },
        [](const ModelVolume &mv_old, const ModelVolume &mv_new){ return mv_old.mmu_segmentation_facets.timestamp_matches(mv_new.mmu_segmentation_facets); });
}

bool model_fuzzy_skin_data_changed(const ModelObject &mo, const ModelObject &mo_new)
{
    return model_property_changed(mo, mo_new,
        [](const ModelVolumeType t) { return t == ModelVolumeType::MODEL_PART; },
        [](const ModelVolume &mv_old, const ModelVolume &mv_new){ return mv_old.fuzzy_skin_facets.timestamp_matches(mv_new.fuzzy_skin_facets); });
}

bool model_brim_points_data_changed(const ModelObject& mo, const ModelObject& mo_new)
{
    if (mo.brim_points.size() != mo_new.brim_points.size())
        return true;
    for (size_t i = 0; i < mo.brim_points.size(); ++i) {
        if (mo.brim_points[i] != mo_new.brim_points[i])
            return true;
    }
    return false;
}

bool model_has_multi_part_objects(const Model &model)
{
    for (const ModelObject *model_object : model.objects)
    	if (model_object->volumes.size() != 1 || ! model_object->volumes.front()->is_model_part())
    		return true;
    return false;
}

bool model_has_advanced_features(const Model &model)
{
	auto config_is_advanced = [](const ModelConfig &config) {
        return ! (config.empty() || (config.size() == 1 && config.cbegin()->first == "extruder"));
	};
    for (const ModelObject *model_object : model.objects) {
        // Is there more than one instance or advanced config data?
        if (model_object->instances.size() > 1 || config_is_advanced(model_object->config))
        	return true;
        // Is there any modifier or advanced config data?
        for (const ModelVolume* model_volume : model_object->volumes)
            if (! model_volume->is_model_part() || config_is_advanced(model_volume->config))
            	return true;
    }
    return false;
}

#ifndef NDEBUG
// Verify whether the IDs of Model / ModelObject / ModelVolume / ModelInstance / ModelMaterial are valid and unique.
void check_model_ids_validity(const Model &model)
{
    std::set<ObjectID> ids;
    auto check = [&ids](ObjectID id) {
        assert(id.valid());
        assert(ids.find(id) == ids.end());
        ids.insert(id);
    };
    for (const ModelObject *model_object : model.objects) {
        check(model_object->id());
        check(model_object->config.id());
        for (const ModelVolume *model_volume : model_object->volumes) {
            check(model_volume->id());
	        check(model_volume->config.id());
        }
        for (const ModelInstance *model_instance : model_object->instances)
            check(model_instance->id());
    }
    for (const auto mm : model.materials) {
        check(mm.second->id());
        check(mm.second->config.id());
    }
}

void check_model_ids_equal(const Model &model1, const Model &model2)
{
    // Verify whether the IDs of model1 and model match.
    assert(model1.objects.size() == model2.objects.size());
    for (size_t idx_model = 0; idx_model < model2.objects.size(); ++ idx_model) {
        const ModelObject &model_object1 = *model1.objects[idx_model];
        const ModelObject &model_object2 = *  model2.objects[idx_model];
        assert(model_object1.id() == model_object2.id());
        assert(model_object1.config.id() == model_object2.config.id());
        assert(model_object1.volumes.size() == model_object2.volumes.size());
        assert(model_object1.instances.size() == model_object2.instances.size());
        for (size_t i = 0; i < model_object1.volumes.size(); ++ i) {
            assert(model_object1.volumes[i]->id() == model_object2.volumes[i]->id());
        	assert(model_object1.volumes[i]->config.id() == model_object2.volumes[i]->config.id());
        }
        for (size_t i = 0; i < model_object1.instances.size(); ++ i)
            assert(model_object1.instances[i]->id() == model_object2.instances[i]->id());
    }
    assert(model1.materials.size() == model2.materials.size());
    {
        auto it1 = model1.materials.begin();
        auto it2 = model2.materials.begin();
        for (; it1 != model1.materials.end(); ++ it1, ++ it2) {
            assert(it1->first == it2->first); // compare keys
            assert(it1->second->id() == it2->second->id());
        	assert(it1->second->config.id() == it2->second->config.id());
        }
    }
}

#endif /* NDEBUG */

}

#if 0
CEREAL_REGISTER_TYPE(Slic3r::ModelObject)
CEREAL_REGISTER_TYPE(Slic3r::ModelVolume)
CEREAL_REGISTER_TYPE(Slic3r::ModelInstance)
CEREAL_REGISTER_TYPE(Slic3r::Model)

CEREAL_REGISTER_POLYMORPHIC_RELATION(Slic3r::ObjectBase, Slic3r::ModelObject)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Slic3r::ObjectBase, Slic3r::ModelVolume)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Slic3r::ObjectBase, Slic3r::ModelInstance)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Slic3r::ObjectBase, Slic3r::Model)
#endif
