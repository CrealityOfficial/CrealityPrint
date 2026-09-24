#include "ModelObject.hpp"
#include "BuildVolume.hpp"
#include "Model.hpp"
#include "TriangleMeshSlicer.hpp"

// BBS
#include "FaceDetector.hpp"

#include <boost/log/trivial.hpp>

#include "libslic3r/Geometry/ConvexHull.hpp"

// BBS: for segment
#include "MeshBoolean.hpp"

#include "ModelVolume.hpp"
#include "ModelInstance.hpp"

namespace Slic3r {

ModelObject::~ModelObject()
{
    BOOST_LOG_TRIVIAL(warning) << "ModelObject destructor called: name=" << name 
                               << ", volumes_count=" << volumes.size() 
                               << ", instances_count=" << instances.size()
                               << ", layer_config_ranges_count=" << layer_config_ranges.size()
                               << ", object_id=" << id().id;
    boost::log::core::get()->flush();
    this->clear_volumes();
    this->clear_instances();
}

// maintains the m_model pointer
ModelObject& ModelObject::assign_copy(const ModelObject &rhs)
{
    assert(this->id().invalid() || this->id() == rhs.id());
    assert(this->config.id().invalid() || this->config.id() == rhs.config.id());
    this->copy_id(rhs);

    this->name                        = rhs.name;
    //BBS: add module name
    this->module_name                 = rhs.module_name;
    this->input_file                  = rhs.input_file;
    this->from_loaded_id              = rhs.from_loaded_id;
    // BBS: copy UUID from 3MF file
    this->uuid                        = rhs.uuid;
    // Copies the config's ID
    this->config                      = rhs.config;
    assert(this->config.id() == rhs.config.id());
    this->sla_support_points          = rhs.sla_support_points;
    this->sla_points_status           = rhs.sla_points_status;
    this->sla_drain_holes             = rhs.sla_drain_holes;
    this->brim_points                 = rhs.brim_points;
    this->layer_config_ranges         = rhs.layer_config_ranges;
    this->layer_height_profile        = rhs.layer_height_profile;
    this->printable                   = rhs.printable;
    this->origin_translation          = rhs.origin_translation;
    this->cut_id.copy(rhs.cut_id);
    this->copy_transformation_caches(rhs);

    this->clear_volumes();
    this->volumes.reserve(rhs.volumes.size());
    for (ModelVolume *model_volume : rhs.volumes) {
        this->volumes.emplace_back(new ModelVolume(*model_volume));
        this->volumes.back()->set_model_object(this);
    }
    this->clear_instances();
    this->instances.reserve(rhs.instances.size());
    for (const ModelInstance *model_instance : rhs.instances) {
        this->instances.emplace_back(new ModelInstance(*model_instance));
        this->instances.back()->set_model_object(this);
    }

    return *this;
}

// maintains the m_model pointer
ModelObject& ModelObject::assign_copy(ModelObject &&rhs)
{
    assert(this->id().invalid());
    this->copy_id(rhs);

    this->name                        = std::move(rhs.name);
    //BBS: add module name
    this->module_name                 = std::move(rhs.module_name);
    this->input_file                  = std::move(rhs.input_file);
    this->from_loaded_id              = std::move(rhs.from_loaded_id);
    // BBS: move UUID from 3MF file
    this->uuid                        = std::move(rhs.uuid);
    // Moves the config's ID
    this->config                      = std::move(rhs.config);
    assert(this->config.id() == rhs.config.id());
    this->sla_support_points          = std::move(rhs.sla_support_points);
    this->sla_points_status           = std::move(rhs.sla_points_status);
    this->sla_drain_holes             = std::move(rhs.sla_drain_holes);
    this->brim_points                 = std::move(brim_points);
    this->layer_config_ranges         = std::move(rhs.layer_config_ranges);
    this->layer_height_profile        = std::move(rhs.layer_height_profile);
    this->printable                   = std::move(rhs.printable);
    this->origin_translation          = std::move(rhs.origin_translation);
    this->copy_transformation_caches(rhs);

    this->clear_volumes();
    this->volumes = std::move(rhs.volumes);
    rhs.volumes.clear();
    for (ModelVolume *model_volume : this->volumes)
        model_volume->set_model_object(this);
    this->clear_instances();
    this->instances = std::move(rhs.instances);
    rhs.instances.clear();
    for (ModelInstance *model_instance : this->instances)
        model_instance->set_model_object(this);

    return *this;
}

void ModelObject::assign_new_unique_ids_recursive()
{
    this->set_new_unique_id();
    for (ModelVolume *model_volume : this->volumes)
        model_volume->assign_new_unique_ids_recursive();
    for (ModelInstance *model_instance : this->instances)
        model_instance->assign_new_unique_ids_recursive();
    this->layer_height_profile.set_new_unique_id();
}

// Clone this ModelObject including its volumes and instances, keep the IDs of the copies equal to the original.
// Called by Print::apply() to clone the Model / ModelObject hierarchy to the back end for background processing.
//ModelObject* ModelObject::clone(Model *parent)
//{
//    return new ModelObject(parent, *this, true);
//}


// BBS: production extension
int ModelObject::get_backup_id() const { return m_model ? get_model()->get_object_backup_id(*this) : -1; }

// BBS: Boolean Operations impl. - MusangKing
bool ModelObject::make_boolean(ModelObject *cut_object, const std::string &boolean_opts)
{
    // merge meshes into single volume instead of multi-parts object
    if (this->volumes.size() != 1) {
        // we can't merge meshes if there's not just one volume
        return false;
    }
    std::vector<TriangleMesh> new_meshes;

    const TriangleMesh &cut_mesh = cut_object->mesh();

#ifndef CLOUD_SKIP_MESHBOOLEAN
    MeshBoolean::mcut::make_boolean(this->mesh(), cut_mesh, new_meshes, boolean_opts);
#endif

    this->clear_volumes();
    int i = 1;
    for (TriangleMesh &mesh : new_meshes) {
        ModelVolume *vol = this->add_volume(mesh);
        vol->name        = this->name + "_" + std::to_string(i++);
    }
    return true;
}

ModelVolume* ModelObject::add_volume(const TriangleMesh &mesh)
{
    ModelVolume* v = new ModelVolume(this, mesh);
    this->volumes.push_back(v);
    v->center_geometry_after_creation();
    this->invalidate_bounding_box();
    // BBS: backup
    Slic3r::save_object_mesh(*this);
    return v;
}

ModelVolume* ModelObject::add_volume(TriangleMesh &&mesh, ModelVolumeType type /*= ModelVolumeType::MODEL_PART*/,
                                     bool modify_to_center_geometry /*= true*/, bool backup /*= true*/)
{
    // The former call bound the rvalue to ModelVolume's const-reference constructor,
    // which copied the complete mesh and calculated a convex hull before centering.
    ModelVolume* v = new ModelVolume(this, std::shared_ptr<const TriangleMesh>(), type);
    v->m_mesh = std::make_shared<TriangleMesh>(std::move(mesh));
    if (v->m_mesh->facets_count() > 1)
        v->calculate_convex_hull();
    this->volumes.push_back(v);
    if (modify_to_center_geometry) {
        v->center_geometry_after_creation();
        this->invalidate_bounding_box();
    }
    // BBS: backup
    if (backup)
        Slic3r::save_object_mesh(*this);
    return v;
}

ModelVolume* ModelObject::add_volume(const ModelVolume &other, ModelVolumeType type /*= ModelVolumeType::INVALID*/)
{
    ModelVolume* v = new ModelVolume(this, other);
    if (type != ModelVolumeType::INVALID && v->type() != type)
        v->set_type(type);

    v->cut_info = other.cut_info;

    this->volumes.push_back(v);
    // The volume should already be centered at this point of time when copying shared pointers of the triangle mesh and convex hull.
//	v->center_geometry_after_creation();
//    this->invalidate_bounding_box();
    // BBS: backup
    Slic3r::save_object_mesh(*this);
    return v;
}

ModelVolume* ModelObject::add_volume(const ModelVolume &other, TriangleMesh &&mesh)
{
    ModelVolume* v = new ModelVolume(this, other, std::move(mesh));
    this->volumes.push_back(v);
    v->center_geometry_after_creation();
    this->invalidate_bounding_box();
    // BBS: backup
    Slic3r::save_object_mesh(*this);
    return v;
}

ModelVolume* ModelObject::add_volume_with_shared_mesh(const ModelVolume &other, ModelVolumeType type /*= ModelVolumeType::INVALID*/, bool backup /*= true*/)
{
    // Construct with an empty mesh to avoid recalculating the convex hull before sharing it.
    ModelVolume* v = new ModelVolume(this, std::shared_ptr<const TriangleMesh>());
    v->m_mesh = other.m_mesh;
    v->m_convex_hull = other.m_convex_hull;
    if (type != ModelVolumeType::INVALID && v->type() != type)
        v->set_type(type);
    this->volumes.push_back(v);
    // The volume should already be centered at this point of time when copying shared pointers of the triangle mesh and convex hull.
//	v->center_geometry_after_creation();
//    this->invalidate_bounding_box();
    // BBS: backup
    if (backup)
        Slic3r::save_object_mesh(*this);
    return v;
}

void ModelObject::delete_volume(size_t idx)
{
    ModelVolumePtrs::iterator i = this->volumes.begin() + idx;
    delete *i;
    this->volumes.erase(i);

    if (this->volumes.size() == 1)
    {
        // only one volume left
        // we need to collapse the volume transform into the instances transforms because now when selecting this volume
        // it will be seen as a single full instance ans so its volume transform may be ignored
        ModelVolume* v = this->volumes.front();
        Transform3d v_t = v->get_transformation().get_matrix();
        for (ModelInstance* inst : this->instances)
        {
            inst->set_transformation(Geometry::Transformation(inst->get_transformation().get_matrix() * v_t));
        }
        Geometry::Transformation t;
        v->set_transformation(t);
        v->set_new_unique_id();
    }

    this->invalidate_bounding_box();
    // BBS: backup
    Slic3r::save_object_mesh(*this);
}

void ModelObject::clear_volumes()
{
    BOOST_LOG_TRIVIAL(warning) << "ModelObject::clear_volumes called: object_name=" << name
                               << ", volumes_to_delete=" << volumes.size()
                               << ", object_id=" << id().id;
    for (ModelVolume *v : this->volumes)
        delete v;
    this->volumes.clear();
    this->invalidate_bounding_box();
    // BBS: backup: do not save
    // Slic3r::save_object_mesh(*this);
}

bool ModelObject::is_fdm_support_painted() const
{
    try {
        // 关键安全检查：验证当前ModelObject的ObjectID是否仍然有效
        // 这可以检测到对象是否已被释放或无效化
        if (!this->id().valid()) {
            BOOST_LOG_TRIVIAL(error) << "is_fdm_support_painted: ModelObject has invalid ObjectID, object may have been deleted";
            return false;
        }
        
        // 安全检查：确保volumes容器有效
        if (this->volumes.empty()) {
            return false;
        }
        
        return std::any_of(this->volumes.cbegin(), this->volumes.cend(), [this](const ModelVolume *mv) { 
            // 安全检查：确保ModelVolume指针有效
            if (!mv) {
                BOOST_LOG_TRIVIAL(error) << "is_fdm_support_painted: ModelVolume pointer is null";
                return false;
            }
            
            // 验证ModelVolume的ObjectID是否有效
            if (!mv->id().valid()) {
                BOOST_LOG_TRIVIAL(error) << "is_fdm_support_painted: ModelVolume has invalid ObjectID, volume may have been deleted";
                return false;
            }
            
            // 再次验证父对象在访问子对象前仍然有效
            if (!this->id().valid()) {
                BOOST_LOG_TRIVIAL(error) << "is_fdm_support_painted: ModelObject ObjectID became invalid during volume iteration";
                return false;
            }
            
            return mv->is_fdm_support_painted(); 
        });
    }
    catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "is_fdm_support_painted: Exception caught: " << e.what();
        return false;
    }
    catch (...) {
        BOOST_LOG_TRIVIAL(error) << "is_fdm_support_painted: Unknown exception caught";
        return false;
    }
}

bool ModelObject::is_seam_painted() const
{
    return std::any_of(this->volumes.cbegin(), this->volumes.cend(), [](const ModelVolume *mv) { return mv->is_seam_painted(); });
}

bool ModelObject::is_mm_painted() const
{
    return std::any_of(this->volumes.cbegin(), this->volumes.cend(), [](const ModelVolume *mv) { return mv->is_mm_painted(); });
}

bool ModelObject::is_fuzzy_skin_painted() const
{
    return std::any_of(this->volumes.cbegin(), this->volumes.cend(), [](const ModelVolume *mv) { return mv->is_fuzzy_skin_painted(); });
}

void ModelObject::sort_volumes(bool full_sort)
{
    // sort volumes inside the object to order "Model Part, Negative Volume, Modifier, Support Blocker and Support Enforcer. "
    if (full_sort)
        std::stable_sort(volumes.begin(), volumes.end(), [](ModelVolume* vl, ModelVolume* vr) {
            return vl->type() < vr->type();
        });
    // sort have to controll "place" of the support blockers/enforcers. But one of the model parts have to be on the first place.
    else
        std::stable_sort(volumes.begin(), volumes.end(), [](ModelVolume* vl, ModelVolume* vr) {
            ModelVolumeType vl_type = vl->type() > ModelVolumeType::PARAMETER_MODIFIER ? vl->type() : ModelVolumeType::PARAMETER_MODIFIER;
            ModelVolumeType vr_type = vr->type() > ModelVolumeType::PARAMETER_MODIFIER ? vr->type() : ModelVolumeType::PARAMETER_MODIFIER;
            return vl_type < vr_type;
        });
}

ModelInstance* ModelObject::add_instance()
{
    ModelInstance* i = new ModelInstance(this);
    this->instances.push_back(i);
    this->invalidate_bounding_box();
    // BBS: backup: do not save
    if (this->instances.size() == 1)
        Slic3r::save_object_mesh(*this);
    return i;
}

ModelInstance* ModelObject::add_instance(const ModelInstance &other)
{
    ModelInstance* i = new ModelInstance(this, other);
    this->instances.push_back(i);
    this->invalidate_bounding_box();
    return i;
}

ModelInstance* ModelObject::add_instance(const Vec3d &offset, const Vec3d &scaling_factor, const Vec3d &rotation, const Vec3d &mirror)
{
    auto *instance = add_instance();
    instance->set_offset(offset);
    instance->set_scaling_factor(scaling_factor);
    instance->set_rotation(rotation);
    instance->set_mirror(mirror);
    return instance;
}

void ModelObject::delete_instance(size_t idx)
{
    ModelInstancePtrs::iterator i = this->instances.begin() + idx;
    delete *i;
    this->instances.erase(i);
    this->invalidate_bounding_box();
}

void ModelObject::delete_last_instance()
{
    this->delete_instance(this->instances.size() - 1);
}

void ModelObject::clear_instances()
{
    BOOST_LOG_TRIVIAL(warning) << "ModelObject::clear_instances called: object_name=" << name
                               << ", instances_to_delete=" << instances.size()
                               << ", object_id=" << id().id;
    for (ModelInstance *i : this->instances)
        delete i;
    this->instances.clear();
    this->invalidate_bounding_box();
}

// Returns the bounding box of the transformed instances.
// This bounding box is approximate and not snug.
const BoundingBoxf3& ModelObject::bounding_box_approx() const
{
    if (! m_bounding_box_approx_valid) {
        m_bounding_box_approx_valid = true;
        BoundingBoxf3 raw_bbox = this->raw_mesh_bounding_box();
        m_bounding_box_approx.reset();
        for (const ModelInstance *i : this->instances)
            m_bounding_box_approx.merge(i->transform_bounding_box(raw_bbox));
    }
    return m_bounding_box_approx;
}

// Returns the bounding box of the transformed instances.
// This bounding box is approximate and not snug.
const BoundingBoxf3& ModelObject::bounding_box_exact() const
{
    if (! m_bounding_box_exact_valid) {
        m_bounding_box_exact_valid = true;
        m_min_max_z_valid = true;
        m_bounding_box_exact.reset();
        for (size_t i = 0; i < this->instances.size(); ++ i)
            m_bounding_box_exact.merge(this->instance_bounding_box(i));
    }
    return m_bounding_box_exact;
}

const BoundingBoxf3& ModelObject::belt_bounding_box_exact() const
{
   m_belt_bounding_box_exact.reset();
   for (size_t i = 0; i < this->instances.size(); ++i)
        m_belt_bounding_box_exact.merge(this->instance_belt_bounding_box(i));
   return m_belt_bounding_box_exact;
}

Vec3d ModelObject::get_instances_min_offset() const { return Vec3d(); }


double ModelObject::min_z() const
{
    const_cast<ModelObject*>(this)->update_min_max_z();
    return m_bounding_box_exact.min.z();
}

double ModelObject::max_z() const
{
    const_cast<ModelObject*>(this)->update_min_max_z();
    return m_bounding_box_exact.max.z();
}

double ModelObject::min_y() const
{
    const_cast<ModelObject*>(this)->update_min_max_y();
    return m_bounding_box_exact.min.y();
}

double ModelObject::current_min_y() const
{
    m_min_max_y_valid = false;
    const_cast<ModelObject*>(this)->update_min_max_y();
    return m_bounding_box_exact.min.y();
}

double ModelObject::max_y() const
{
    const_cast<ModelObject*>(this)->update_min_max_y();
    return m_bounding_box_exact.max.y();
}

double ModelObject::current_max_y() const
{
    m_min_max_y_valid = false;
    const_cast<ModelObject*>(this)->update_min_max_y();
    return m_bounding_box_exact.max.y();
}

double ModelObject::depth() const
{
    const_cast<ModelObject*>(this)->update_min_max_y();
    return (m_bounding_box_exact.max.y() - m_bounding_box_exact.min.y());
}

  BoundingBoxf3 ModelObject::belt_box() {
    return m_belt_bounding_box_exact;
 }


void ModelObject::update_min_max_y()
{
    assert(!this->instances.empty());
    if (!m_min_max_y_valid && !this->instances.empty()) {
        m_min_max_y_valid              = true;
        const Transform3d mat_instance = this->instances.front()->get_transformation().get_matrix();
        double            global_min_y = std::numeric_limits<double>::max();
        double            global_max_y = -std::numeric_limits<double>::max();
        for (ModelVolume* v : this->volumes)
            if (v->is_model_part())
            {
                const Transform3d m          = mat_instance * v->get_matrix();
                const Vec3d       row_y      = m.linear().row(1).cast<double>();
                const double      shift_y    = m.translation().y();
                double            this_min_y = std::numeric_limits<double>::max();
                double            this_max_y = -std::numeric_limits<double>::max();
                for (const Vec3f& p : v->mesh().its.vertices) {
                    double y   = row_y.dot(p.cast<double>());
                    this_min_y = std::min(this_min_y, y);
                    this_max_y = std::max(this_max_y, y);
                }
                this_min_y += shift_y;
                this_max_y += shift_y;
                v->min_y     = this_min_y;
                global_min_y = std::min(global_min_y, this_min_y);
                global_max_y = std::max(global_max_y, this_max_y);
            }
        m_bounding_box_exact.min.y() = global_min_y;
        m_bounding_box_exact.max.y() = global_max_y;
    }
}

void ModelObject::update_min_max_z()
{
    assert(! this->instances.empty());
    if (! m_min_max_z_valid && ! this->instances.empty()) {
        m_min_max_z_valid = true;
        const Transform3d mat_instance = this->instances.front()->get_transformation().get_matrix();
        double global_min_z = std::numeric_limits<double>::max();
        double global_max_z = - std::numeric_limits<double>::max();
        for (const ModelVolume *v : this->volumes)
            if (v->is_model_part()) {
                const Transform3d m = mat_instance * v->get_matrix();
                const Vec3d  row_z   = m.linear().row(2).cast<double>();
                const double shift_z = m.translation().z();
                double this_min_z = std::numeric_limits<double>::max();
                double this_max_z = - std::numeric_limits<double>::max();
                for (const Vec3f &p : v->mesh().its.vertices) {
                    double z = row_z.dot(p.cast<double>());
                    this_min_z = std::min(this_min_z, z);
                    this_max_z = std::max(this_max_z, z);
                }
                this_min_z += shift_z;
                this_max_z += shift_z;
                global_min_z = std::min(global_min_z, this_min_z);
                global_max_z = std::max(global_max_z, this_max_z);
            }
        m_bounding_box_exact.min.z() = global_min_z;
        m_bounding_box_exact.max.z() = global_max_z;
    }
}

// A mesh containing all transformed instances of this object.
TriangleMesh ModelObject::mesh() const
{
    TriangleMesh mesh;
    TriangleMesh raw_mesh = this->raw_mesh();
    for (const ModelInstance *i : this->instances) {
        TriangleMesh m = raw_mesh;
        i->transform_mesh(&m);
        mesh.merge(m);
    }
    return mesh;
}

// Non-transformed (non-rotated, non-scaled, non-translated) sum of non-modifier object volumes.
// Currently used by ModelObject::mesh(), to calculate the 2D envelope for 2D plater
// and to display the object statistics at ModelObject::print_info().
TriangleMesh ModelObject::raw_mesh() const
{
    TriangleMesh mesh;
    for (const ModelVolume *v : this->volumes)
        if (v->is_model_part())
        {
            TriangleMesh vol_mesh(v->mesh());
            vol_mesh.transform(v->get_matrix());
            mesh.merge(vol_mesh);
        }
    return mesh;
}

// Non-transformed (non-rotated, non-scaled, non-translated) sum of non-modifier object volumes.
// Currently used by ModelObject::mesh(), to calculate the 2D envelope for 2D plater
// and to display the object statistics at ModelObject::print_info().
indexed_triangle_set ModelObject::raw_indexed_triangle_set() const
{
    size_t num_vertices = 0;
    size_t num_faces    = 0;
    for (const ModelVolume *v : this->volumes)
        if (v->is_model_part()) {
            num_vertices += v->mesh().its.vertices.size();
            num_faces    += v->mesh().its.indices.size();
        }
    indexed_triangle_set out;
    out.vertices.reserve(num_vertices);
    out.indices.reserve(num_faces);
    for (const ModelVolume *v : this->volumes)
        if (v->is_model_part()) {
            size_t i = out.vertices.size();
            size_t j = out.indices.size();
            append(out.vertices, v->mesh().its.vertices);
            append(out.indices,  v->mesh().its.indices);
            const Transform3d& m = v->get_matrix();
            for (; i < out.vertices.size(); ++ i)
                out.vertices[i] = (m * out.vertices[i].cast<double>()).cast<float>().eval();
            if (v->is_left_handed()) {
                for (; j < out.indices.size(); ++ j)
                    std::swap(out.indices[j][0], out.indices[j][1]);
            }
        }
    return out;
}


const BoundingBoxf3& ModelObject::raw_mesh_bounding_box() const
{
    if (! m_raw_mesh_bounding_box_valid) {
        m_raw_mesh_bounding_box_valid = true;
        m_raw_mesh_bounding_box.reset();
        for (const ModelVolume *v : this->volumes)
            if (v->is_model_part())
                m_raw_mesh_bounding_box.merge(v->mesh().transformed_bounding_box(v->get_matrix()));
    }
    return m_raw_mesh_bounding_box;
}

BoundingBoxf3 ModelObject::full_raw_mesh_bounding_box() const
{
    BoundingBoxf3 bb;
    for (const ModelVolume *v : this->volumes)
        bb.merge(v->mesh().transformed_bounding_box(v->get_matrix()));
    return bb;
}

// A transformed snug bounding box around the non-modifier object volumes, without the translation applied.
// This bounding box is only used for the actual slicing and for layer editing UI to calculate the layers.
const BoundingBoxf3& ModelObject::raw_bounding_box() const
{
    if (! m_raw_bounding_box_valid) {
        m_raw_bounding_box_valid = true;
        m_raw_bounding_box.reset();
        if (this->instances.empty())
            throw Slic3r::InvalidArgument("Can't call raw_bounding_box() with no instances");

        const Transform3d inst_matrix = this->instances.front()->get_transformation().get_matrix_no_offset();
        for (const ModelVolume *v : this->volumes)
            if (v->is_model_part())
                m_raw_bounding_box.merge(v->mesh().transformed_bounding_box(inst_matrix * v->get_matrix()));
    }
    return m_raw_bounding_box;
}

// This returns an accurate snug bounding box of the transformed object instance, without the translation applied.
BoundingBoxf3 ModelObject::instance_bounding_box(size_t instance_idx, bool dont_translate) const {
    return instance_bounding_box(*this->instances[instance_idx], dont_translate);
}

BoundingBoxf3 ModelObject::instance_bounding_box(const ModelInstance &instance, bool dont_translate) const
{
    BoundingBoxf3 bb;
    const Transform3d inst_matrix = dont_translate ?
        instance.get_transformation().get_matrix_no_offset() :
        instance.get_transformation().get_matrix();

    for (ModelVolume *v : this->volumes) {
        if (v->is_model_part())
            bb.merge(v->mesh().transformed_bounding_box(inst_matrix * v->get_matrix()));
    }
    return bb;
}


BoundingBoxf3 ModelObject::instance_belt_bounding_box(size_t instance_idx, bool dont_translate) const
{
    return instance_belt_bounding_box(*this->instances[instance_idx], dont_translate);
}

Transform3d beltXForm(const Transform3d& offset, float angle)
{
    float theta = angle * PI / 180.0f;

    Transform3d xf0 = offset;

    Transform3d xf1 = Transform3d::Identity();
    xf1(2, 2)       = 1.0f / sinf(theta);
    xf1(1, 2)       = -1.0f / tanf(theta);

    Transform3d xf2 = Transform3d::Identity();
    xf2(2, 2)       = 0.0f;
    xf2(1, 1)       = 0.0f;
    xf2(2, 1)       = -1.0f;
    xf2(1, 2)       = 1.0f;

    Vec3d                    xf3Data(0.0f, 0.0f, 0.0f);
    Geometry::Transformation _trans;
    _trans.set_offset(xf3Data);
    auto xf3 = _trans.get_offset_matrix();

    Transform3d xf = xf3 * xf2 * xf1 * xf0;
    return xf;
}

BoundingBoxf3 ModelObject::instance_belt_bounding_box(const ModelInstance& instance, bool dont_translate) const
{
    BoundingBoxf3     bb;
    
    Transform3d inst_matrix = instance.get_transformation().get_matrix_no_offset();
    for (ModelVolume* v : this->volumes) {
        if (v->is_model_part())
            bb.merge(v->mesh().transformed_bounding_box(inst_matrix * v->get_matrix(), beltXForm(Transform3d::Identity(),45.0f)));
    }
    return bb;
}


//BBS: add convex bounding box
BoundingBoxf3 ModelObject::instance_convex_hull_bounding_box(size_t instance_idx, bool dont_translate) const
{
    return instance_convex_hull_bounding_box(this->instances[instance_idx], dont_translate);
}

BoundingBoxf3 ModelObject::instance_convex_hull_bounding_box(const ModelInstance* instance, bool dont_translate) const
{
    BoundingBoxf3 bb;
    const Transform3d inst_matrix = dont_translate ? instance->get_transformation().get_matrix_no_offset() :
                                                        instance->get_transformation().get_matrix();
    for (ModelVolume* v : this->volumes) {
        if (v->is_model_part())
            bb.merge(v->get_convex_hull().transformed_bounding_box(inst_matrix * v->get_matrix()));
    }
    return bb;
}


// Calculate 2D convex hull of of a projection of the transformed printable volumes into the XY plane.
// This method is cheap in that it does not make any unnecessary copy of the volume meshes.
// This method is used by the auto arrange function.
Polygon ModelObject::convex_hull_2d(const Transform3d& trafo_instance) const
{
#if 0
    Points pts;

    for (const ModelVolume* v : volumes) {
        if (v->is_model_part())
            //BBS: use convex hull vertex instead of all
            append(pts, its_convex_hull_2d_above(v->get_convex_hull().its, (trafo_instance * v->get_matrix()).cast<float>(), 0.0f).points);
            //append(pts, its_convex_hull_2d_above(v->mesh().its, (trafo_instance * v->get_matrix()).cast<float>(), 0.0f).points);
    }
    return Geometry::convex_hull(std::move(pts));
#else
    Points pts;
    for (const ModelVolume *v : this->volumes)
        if (v->is_model_part()) {
            const Polygon& volume_hull = v->get_convex_hull_2d(trafo_instance);

            pts.insert(pts.end(), volume_hull.points.begin(), volume_hull.points.end());
        }

    //std::sort(pts.begin(), pts.end(), [](const Point& a, const Point& b) { return a(0) < b(0) || (a(0) == b(0) && a(1) < b(1)); });
    //pts.erase(std::unique(pts.begin(), pts.end(), [](const Point& a, const Point& b) { return a(0) == b(0) && a(1) == b(1); }), pts.end());
    /*std::vector<Points> points;
    //points.push_back(pts);
    Polygon hull = Geometry::convex_hull(std::move(pts));
    static int irun = 0;
    BoundingBox bbox_svg;

    bbox_svg.merge(get_extents(pts));
    bbox_svg.merge(get_extents(hull));
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": bbox_svg.min{%1%,%2%} max{%3%,%4%}, points count %5%")% bbox_svg.min.x()% bbox_svg.min.y()% bbox_svg.max.x()% bbox_svg.max.y()%points[0].size();
    {
        std::stringstream stri;
        stri << "convex_2d_hull_" << irun << ".svg";
        SVG svg(stri.str(), bbox_svg);

        std::vector<Polygon> hulls;
        hulls.push_back(hull);
        svg.draw(to_polylines(points), "blue");
        svg.draw(to_polylines(hulls), "red");
        svg.Close();
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": stri %1%, Polygon.size %2%, point[0] {%3%, %4%}, point[1] {%5%, %6%}")% stri.str()% hull.size()% hull[0].x()% hull[0].y()% hull[1].x()% hull[1].y();
    }
    ++ irun;
    return hull;*/
    return Geometry::convex_hull(std::move(pts));
#endif
}

void ModelObject::center_around_origin(bool include_modifiers)
{
    // calculate the displacements needed to
    // center this object around the origin
    const BoundingBoxf3 bb = include_modifiers ? full_raw_mesh_bounding_box() : raw_mesh_bounding_box();

    // Shift is the vector from the center of the bounding box to the origin
    const Vec3d shift = -bb.center();

    this->translate(shift);
    this->origin_translation += shift;
}

void ModelObject::ensure_on_bed(bool allow_negative_z)
{
    double z_offset = 0.0;

    if (allow_negative_z) {
        if (parts_count() == 1) {
            const double min_z = this->min_z();
            const double max_z = this->max_z();
            if (min_z >= SINKING_Z_THRESHOLD || max_z < 0.0)
                z_offset = -min_z;
        }
        else {
            const double max_z = this->max_z();
            if (max_z < SINKING_MIN_Z_THRESHOLD)
                z_offset = SINKING_MIN_Z_THRESHOLD - max_z;
        }
    }
    else
        z_offset = -this->min_z();

    if (z_offset != 0.0)
        translate_instances(z_offset * Vec3d::UnitZ());
}

void ModelObject::translate_instances(const Vec3d& vector)
{
    for (size_t i = 0; i < instances.size(); ++i) {
        translate_instance(i, vector);
    }
}

void ModelObject::translate_instance(size_t instance_idx, const Vec3d& vector)
{
    assert(instance_idx < instances.size());
    ModelInstance* i = instances[instance_idx];
    i->set_offset(i->get_offset() + vector);
    invalidate_bounding_box();
}

void ModelObject::translate(double x, double y, double z)
{
    for (ModelVolume *v : this->volumes) {
        v->translate(x, y, z);
    }

    if (m_bounding_box_approx_valid)
        m_bounding_box_approx.translate(x, y, z);
    if (m_bounding_box_exact_valid)
        m_bounding_box_exact.translate(x, y, z);
}

void ModelObject::scale(const Vec3d &versor)
{
    for (ModelVolume *v : this->volumes) {
        v->scale(versor);
    }
    this->invalidate_bounding_box();
}

void ModelObject::rotate(double angle, Axis axis)
{
    for (ModelVolume *v : this->volumes) {
        v->rotate(angle, axis);
    }
    center_around_origin();
    this->invalidate_bounding_box();
}

void ModelObject::rotate(double angle, const Vec3d& axis)
{
    for (ModelVolume *v : this->volumes) {
        v->rotate(angle, axis);
    }

    //BBS update assemble transformation when modify volume rotation
    for (int i = 0; i < instances.size(); i++) {
        instances[i]->rotate_assemble(-angle, axis);
    }

    center_around_origin();
    this->invalidate_bounding_box();
}

void ModelObject::mirror(Axis axis)
{
    for (ModelVolume *v : this->volumes) {
        v->mirror(axis);
    }
    this->invalidate_bounding_box();
}

// This method could only be called before the meshes of this ModelVolumes are not shared!
void ModelObject::scale_mesh_after_creation(const float scale)
{
    for (ModelVolume *v : this->volumes) {
        v->scale_geometry_after_creation(scale);
        v->set_offset(Vec3d(scale, scale, scale).cwiseProduct(v->get_offset()));
    }
    this->invalidate_bounding_box();
}

void ModelObject::convert_units(ModelObjectPtrs& new_objects, ConversionType conv_type, std::vector<int> volume_idxs)
{
    BOOST_LOG_TRIVIAL(trace) << "ModelObject::convert_units - start";

    ModelObject* new_object = new_clone(*this);

    float koef = conv_type == ConversionType::CONV_FROM_INCH   ? 25.4f  : conv_type == ConversionType::CONV_TO_INCH  ? 0.0393700787f  :
                    conv_type == ConversionType::CONV_FROM_METER  ? 1000.f : conv_type == ConversionType::CONV_TO_METER ? 0.001f         : 1.f;

    new_object->set_model(nullptr);
    new_object->sla_support_points.clear();
    new_object->sla_drain_holes.clear();
    new_object->sla_points_status = sla::PointsStatus::NoPoints;
    new_object->brim_points.clear();
    new_object->clear_volumes();
    new_object->input_file.clear();

    int vol_idx = 0;
    for (ModelVolume* volume : volumes) {
        if (!volume->mesh().empty()) {
            TriangleMesh mesh(volume->mesh());

            ModelVolume* vol = new_object->add_volume(mesh);
            vol->name = volume->name;
            vol->set_type(volume->type());
            // Don't copy the config's ID.
            vol->config.assign_config(volume->config);
            assert(vol->config.id().valid());
            assert(vol->config.id() != volume->config.id());
            vol->set_material(volume->material_id(), *volume->material());
            vol->source.input_file = volume->source.input_file;
            vol->source.object_idx = (int)new_objects.size();
            vol->source.volume_idx = vol_idx;
            vol->source.is_converted_from_inches = volume->source.is_converted_from_inches;
            vol->source.is_converted_from_meters = volume->source.is_converted_from_meters;
            vol->source.is_from_builtin_objects = volume->source.is_from_builtin_objects;

            vol->supported_facets.assign(volume->supported_facets);
            vol->seam_facets.assign(volume->seam_facets);
            vol->mmu_segmentation_facets.assign(volume->mmu_segmentation_facets);
            vol->fuzzy_skin_facets.assign(volume->fuzzy_skin_facets);

            // Perform conversion only if the target "imperial" state is different from the current one.
            // This check supports conversion of "mixed" set of volumes, each with different "imperial" state.
            if (//vol->source.is_converted_from_inches != from_imperial &&
                (volume_idxs.empty() ||
                    std::find(volume_idxs.begin(), volume_idxs.end(), vol_idx) != volume_idxs.end())) {
                vol->scale_geometry_after_creation(koef);
                vol->set_offset(Vec3d(koef, koef, koef).cwiseProduct(volume->get_offset()));
                if (conv_type == ConversionType::CONV_FROM_INCH || conv_type == ConversionType::CONV_TO_INCH)
                    vol->source.is_converted_from_inches = conv_type == ConversionType::CONV_FROM_INCH;
                if (conv_type == ConversionType::CONV_FROM_METER || conv_type == ConversionType::CONV_TO_METER)
                    vol->source.is_converted_from_meters = conv_type == ConversionType::CONV_FROM_METER;
                assert(! vol->source.is_converted_from_inches || ! vol->source.is_converted_from_meters);
            }
            else
                vol->set_offset(volume->get_offset());
        }
        vol_idx ++;
    }
    new_object->invalidate_bounding_box();

    new_objects.push_back(new_object);

    BOOST_LOG_TRIVIAL(trace) << "ModelObject::convert_units - end";
}

size_t ModelObject::materials_count() const
{
    std::set<t_model_material_id> material_ids;
    for (const ModelVolume *v : this->volumes)
        material_ids.insert(v->material_id());
    return material_ids.size();
}

size_t ModelObject::facets_count() const
{
    size_t num = 0;
    for (const ModelVolume *v : this->volumes)
        if (v->is_model_part())
            num += v->mesh().facets_count();
    return num;
}

size_t ModelObject::parts_count() const
{
    size_t num = 0;
    for (const ModelVolume* v : this->volumes)
        if (v->is_model_part())
            ++num;
    return num;
}

bool ModelObject::has_connectors() const
{
    assert(is_cut());
    for (const ModelVolume *v : this->volumes)
        if (v->cut_info.is_connector) return true;

    return false;
}

void ModelObject::invalidate_cut()
{
    this->cut_id.invalidate();
    for (ModelVolume *volume : this->volumes)
        volume->invalidate_cut_info();
}

void ModelObject::delete_connectors()
{
    for (int id = int(this->volumes.size()) - 1; id >= 0; id--) {
        if (volumes[id]->is_cut_connector())
            this->delete_volume(size_t(id));
    }
}

void ModelObject::clone_for_cut(ModelObject **obj)
{
    (*obj) = ModelObject::new_clone(*this);
    (*obj)->set_model(this->get_model());
    (*obj)->sla_support_points.clear();
    (*obj)->sla_drain_holes.clear();
    (*obj)->sla_points_status = sla::PointsStatus::NoPoints;
    (*obj)->clear_volumes();
    (*obj)->input_file.clear();
}

static void invalidate_translations(ModelObject* object, const ModelInstance* src_instance)
{
    if (!object->origin_translation.isApprox(Vec3d::Zero()) && src_instance->get_offset().isApprox(Vec3d::Zero())) {
        object->center_around_origin();
        object->translate_instances(-object->origin_translation);
        object->origin_translation = Vec3d::Zero();
    }
    else {
        object->invalidate_bounding_box();
        object->center_around_origin();
    }
}

void ModelObject::split(ModelObjectPtrs* new_objects)
{
    std::vector<TriangleMesh> all_meshes;
    std::vector<Transform3d> all_transfos;
    std::vector<std::pair<int, int>> volume_mesh_counts;
    all_meshes.reserve(this->volumes.size() * 5);
    bool is_multi_volume_object = (this->volumes.size() > 1);

    for (int volume_idx = 0; volume_idx < this->volumes.size(); volume_idx++) {
        ModelVolume* volume = this->volumes[volume_idx];
        if (volume->type() != ModelVolumeType::MODEL_PART)
            continue;

        // splited volume should not be text object 
        if (volume->text_configuration.has_value())
            volume->text_configuration.reset();

        if (!is_multi_volume_object) {
            //BBS: not multi volume object, then split mesh.
            std::vector<TriangleMesh> volume_meshes = volume->mesh().split();
            int mesh_count = 0;
            for (TriangleMesh& mesh : volume_meshes) {
                if (mesh.facets_count() < 3)
                    continue;

                all_meshes.emplace_back(std::move(mesh));
                all_transfos.emplace_back(volume->get_matrix());
                mesh_count++;
            }
            volume_mesh_counts.push_back({ volume_idx, mesh_count });
        } else {
            //BBS: multi volume object, then only split to volume
            if (volume->mesh().facets_count() >= 3) {
                all_meshes.emplace_back(std::move(volume->mesh()));
                all_transfos.emplace_back(volume->get_matrix());
                volume_mesh_counts.push_back({ volume_idx, 1 });
            }
        }
    }

    FaceDetector face_detector(all_meshes, all_transfos, 1.0);
    face_detector.detect_exterior_face();

    int volume_mesh_begin = 0;
    for (int i = 0; i < volume_mesh_counts.size(); i++) {
        std::pair<int, int> mesh_info = volume_mesh_counts[i];
        ModelVolume* volume = this->volumes[mesh_info.first];

        std::vector<TriangleMesh> meshes;
        for (int mesh_idx = volume_mesh_begin; mesh_idx < volume_mesh_begin + mesh_info.second; mesh_idx++) {
            meshes.emplace_back(std::move(all_meshes[mesh_idx]));
        }
        volume_mesh_begin += mesh_info.second;

        size_t counter = 1;
        for (TriangleMesh& mesh : meshes) {
            // FIXME: crashes if not satisfied
            if (mesh.facets_count() < 3)
                continue;

            // XXX: this seems to be the only real usage of m_model, maybe refactor this so that it's not needed?
            ModelObject* new_object = m_model->add_object();
            //BBS: refine the config logic
            //use object as basic, and add volume's config
            if (meshes.size() == 1) {
                new_object->name = volume->name;
                // Don't copy the config's ID.
                //new_object->config.assign_config(this->config.size() > 0 ? this->config : volume->config);
            }
            else {
                new_object->name = this->name + (meshes.size() > 1 ? "_" + std::to_string(counter++) : "");
                // Don't copy the config's ID.
                //new_object->config.assign_config(this->config);
            }
            new_object->config.assign_config(this->config);
            new_object->config.apply(volume->config, true);

            assert(new_object->config.id().valid());
            assert(new_object->config.id() != this->config.id());
            new_object->instances.reserve(this->instances.size());
            for (const ModelInstance* model_instance : this->instances)
                new_object->add_instance(*model_instance);
            ModelVolume* new_vol = new_object->add_volume(*volume, std::move(mesh));

            if (is_multi_volume_object) {
                // BBS: volume geometry not changed, so we can keep the color paint facets
                if (new_vol->mmu_segmentation_facets.timestamp() == volume->mmu_segmentation_facets.timestamp())
                    new_vol->mmu_segmentation_facets.reset(); // BBS: let next assign take effect
                new_vol->mmu_segmentation_facets.assign(volume->mmu_segmentation_facets);
            }

            // BBS: clear volume's config, as we already set them into object
            new_vol->config.reset();

            for (ModelInstance* model_instance : new_object->instances)
            {
                const Vec3d shift = model_instance->get_transformation().get_matrix_no_offset() * new_vol->get_offset();
                model_instance->set_offset(model_instance->get_offset() + shift);

                //BBS: add assemble_view related logic
                Geometry::Transformation instance_transformation_copy = model_instance->get_transformation();
                instance_transformation_copy.set_offset(-new_vol->get_offset());
                const Transform3d &assemble_matrix = model_instance->get_assemble_transformation().get_matrix();
                const Transform3d &instance_inverse_matrix = instance_transformation_copy.get_matrix().inverse();
                Transform3d new_instance_inverse_matrix = instance_inverse_matrix * model_instance->get_transformation().get_matrix_no_offset().inverse();
                Transform3d new_assemble_transform      = assemble_matrix * new_instance_inverse_matrix;
                model_instance->set_assemble_from_transform(new_assemble_transform);
                model_instance->set_offset_to_assembly(new_vol->get_offset());
            }

            new_vol->set_offset(Vec3d::Zero());
            // reset the source to disable reload from disk
            new_vol->source = ModelVolume::Source();
            new_objects->emplace_back(new_object);
        }
    }
}


void ModelObject::merge()
{
    if (this->volumes.size() == 1) {
        // We can't merge meshes if there's just one volume
        return;
    }

    TriangleMesh mesh;

    for (ModelVolume* volume : volumes)
        if (!volume->mesh().empty())
            mesh.merge(volume->mesh());

    this->clear_volumes();
    ModelVolume* vol = this->add_volume(mesh);

    if (!vol)
        return;
}

ModelObjectPtrs ModelObject::merge_volumes(std::vector<int>& vol_indeces)
{
    ModelObjectPtrs res;
    if (this->volumes.size() == 1) {
        // We can't merge meshes if there's just one volume
        return res;
    }

    ModelObject* upper = ModelObject::new_clone(*this);
    upper->set_model(nullptr);
    upper->sla_support_points.clear();
    upper->sla_drain_holes.clear();
    upper->sla_points_status = sla::PointsStatus::NoPoints;
    upper->brim_points.clear();
    upper->clear_volumes();
    upper->input_file.clear();

#if 1
    TriangleMesh mesh;
    for (int i : vol_indeces) {
        auto volume = volumes[i];
        if (!volume->mesh().empty()) {
            const auto volume_matrix = volume->get_matrix();
            TriangleMesh mesh_(volume->mesh());
            mesh_.transform(volume_matrix, true);
            volume->reset_mesh();

            mesh.merge(mesh_);
        }
    }
#else
    std::vector<TriangleMesh> meshes;
    for (int i : vol_indeces) {
        auto volume = volumes[i];
        if (!volume->mesh().empty())
            meshes.emplace_back(volume->mesh());
    }
    TriangleMesh mesh = MeshBoolean::cgal::merge(meshes);
#endif

    ModelVolume* vol = upper->add_volume(mesh);
    for (int i = 0; i < volumes.size();i++) {
        if (std::find(vol_indeces.begin(), vol_indeces.end(), i) != vol_indeces.end()) {
            vol->name = volumes[i]->name + "_merged";
            vol->config.assign_config(volumes[i]->config);
        }
        else
            upper->add_volume(*volumes[i]);
    }
    upper->invalidate_bounding_box();
    res.push_back(upper);
    return res;
}

// Support for non-uniform scaling of instances. If an instance is rotated by angles, which are not multiples of ninety degrees,
// then the scaling in world coordinate system is not representable by the Geometry::Transformation structure.
// This situation is solved by baking in the instance transformation into the mesh vertices.
// Rotation and mirroring is being baked in. In case the instance scaling was non-uniform, it is baked in as well.
void ModelObject::bake_xy_rotation_into_meshes(size_t instance_idx)
{
    assert(instance_idx < this->instances.size());

    const Geometry::Transformation reference_trafo = this->instances[instance_idx]->get_transformation();
    if (Geometry::is_rotation_ninety_degrees(reference_trafo.get_rotation()))
        // nothing to do, scaling in the world coordinate space is possible in the representation of Geometry::Transformation.
        return;

    bool   left_handed        = reference_trafo.is_left_handed();
    bool   has_mirrorring     = ! reference_trafo.get_mirror().isApprox(Vec3d(1., 1., 1.));
    bool   uniform_scaling    = std::abs(reference_trafo.get_scaling_factor().x() - reference_trafo.get_scaling_factor().y()) < EPSILON &&
                                std::abs(reference_trafo.get_scaling_factor().x() - reference_trafo.get_scaling_factor().z()) < EPSILON;
    double new_scaling_factor = uniform_scaling ? reference_trafo.get_scaling_factor().x() : 1.;

    // Adjust the instances.
    for (size_t i = 0; i < this->instances.size(); ++ i) {
        ModelInstance &model_instance = *this->instances[i];
        model_instance.set_rotation(Vec3d(0., 0., Geometry::rotation_diff_z(reference_trafo.get_rotation(), model_instance.get_rotation())));
        model_instance.set_scaling_factor(Vec3d(new_scaling_factor, new_scaling_factor, new_scaling_factor));
        model_instance.set_mirror(Vec3d(1., 1., 1.));
    }

    // Adjust the meshes.
    // Transformation to be applied to the meshes.
    Geometry::Transformation reference_trafo_mod = reference_trafo;
    reference_trafo_mod.reset_offset();
    if (uniform_scaling)
        reference_trafo_mod.reset_scaling_factor();
    if (!has_mirrorring)
        reference_trafo_mod.reset_mirror();
    Eigen::Matrix3d mesh_trafo_3x3 = reference_trafo_mod.get_matrix().matrix().block<3, 3>(0, 0);
    Transform3d     volume_offset_correction = this->instances[instance_idx]->get_transformation().get_matrix().inverse() * reference_trafo.get_matrix();
    for (ModelVolume *model_volume : this->volumes) {
        const Geometry::Transformation volume_trafo = model_volume->get_transformation();
        bool   volume_left_handed        = volume_trafo.is_left_handed();
        bool   volume_has_mirrorring     = ! volume_trafo.get_mirror().isApprox(Vec3d(1., 1., 1.));
        bool   volume_uniform_scaling    = std::abs(volume_trafo.get_scaling_factor().x() - volume_trafo.get_scaling_factor().y()) < EPSILON &&
                                            std::abs(volume_trafo.get_scaling_factor().x() - volume_trafo.get_scaling_factor().z()) < EPSILON;
        double volume_new_scaling_factor = volume_uniform_scaling ? volume_trafo.get_scaling_factor().x() : 1.;
        // Transform the mesh.
        Geometry::Transformation volume_trafo_mod = volume_trafo;
        volume_trafo_mod.reset_offset();
        if (volume_uniform_scaling)
            volume_trafo_mod.reset_scaling_factor();
        if (!volume_has_mirrorring)
            volume_trafo_mod.reset_mirror();
        Eigen::Matrix3d volume_trafo_3x3 = volume_trafo_mod.get_matrix().matrix().block<3, 3>(0, 0);
        // Following method creates a new shared_ptr<TriangleMesh>
        model_volume->transform_this_mesh(mesh_trafo_3x3 * volume_trafo_3x3, left_handed != volume_left_handed);
        // Reset the rotation, scaling and mirroring.
        model_volume->set_rotation(Vec3d(0., 0., 0.));
        model_volume->set_scaling_factor(Vec3d(volume_new_scaling_factor, volume_new_scaling_factor, volume_new_scaling_factor));
        model_volume->set_mirror(Vec3d(1., 1., 1.));
        // Move the reference point of the volume to compensate for the change of the instance trafo.
        model_volume->set_offset(volume_offset_correction * volume_trafo.get_offset());
        // reset the source to disable reload from disk
        model_volume->source = ModelVolume::Source();
    }

    this->invalidate_bounding_box();
}

double ModelObject::get_instance_min_z(size_t instance_idx) const
{
    double min_z = DBL_MAX;

    const ModelInstance* inst = instances[instance_idx];
    const Transform3d mi = inst->get_matrix_no_offset();

    for (const ModelVolume* v : volumes) {
        if (!v->is_model_part())
            continue;

        const Transform3d mv = mi * v->get_matrix();
        const TriangleMesh& hull = v->get_convex_hull();
        //BBS: in some case the convex hull is empty due to the qhull algo
        //use the original mesh instead
        //TODO: when the vertex's x/y/z are all the same, the run_qhull can not get correct result
        //we need to find another algo then
        if (hull.its.indices.size() == 0) {
            const TriangleMesh& mesh = v->mesh();
            for (const stl_triangle_vertex_indices& facet : mesh.its.indices)
                for (int i = 0; i < 3; ++i)
                    min_z = std::min(min_z, (mv * mesh.its.vertices[facet[i]].cast<double>()).z());
        }
        else {
            for (const stl_triangle_vertex_indices& facet : hull.its.indices)
                for (int i = 0; i < 3; ++i)
                    min_z = std::min(min_z, (mv * hull.its.vertices[facet[i]].cast<double>()).z());
        }
    }

    //BBS: add some logic to avoid wrong compute for min_z
    if (min_z == DBL_MAX)
        min_z = 0;
    return min_z + inst->get_offset(Z);
}

double ModelObject::get_instance_max_z(size_t instance_idx) const
{
    double max_z = -DBL_MAX;

    const ModelInstance* inst = instances[instance_idx];
    const Transform3d mi = inst->get_matrix_no_offset();

    for (const ModelVolume* v : volumes) {
        if (!v->is_model_part())
            continue;

        const Transform3d mv = mi * v->get_matrix();
        const TriangleMesh& hull = v->get_convex_hull();
        for (const stl_triangle_vertex_indices& facet : hull.its.indices)
            for (int i = 0; i < 3; ++i)
                max_z = std::max(max_z, (mv * hull.its.vertices[facet[i]].cast<double>()).z());
    }

    return max_z + inst->get_offset(Z);
}

unsigned int ModelObject::update_instances_print_volume_state(const BuildVolume &build_volume)
{
    unsigned int num_printable = 0;
    enum {
        INSIDE = 1,
        OUTSIDE = 2
    };

    //BBS: add logs for build_volume
    //const BoundingBoxf3& print_volume = build_volume.bounding_volume();
    //BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(", print_volume {%1%, %2%, %3%} to {%4%, %5%, %6%}")\
    //    %print_volume.min.x() %print_volume.min.y() %print_volume.min.z()%print_volume.max.x() %print_volume.max.y() %print_volume.max.z();
    for (ModelInstance* model_instance : this->instances) {
        unsigned int inside_outside = 0;
        for (const ModelVolume *vol : this->volumes) {
            if (vol->is_model_part()) {
                //BBS: add bounding box empty check logic, for some volume is empty before split(it will be removed after split to object)
                BoundingBoxf3 bb = vol->get_convex_hull().bounding_box();
                Vec3d size = bb.size();
                if ((size.x() == 0.f) || (size.y() == 0.f) || (size.z() == 0.f)) {
                    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(", object %1%'s vol %2% is empty, skip it, box: {%3%, %4%, %5%} to {%6%, %7%, %8%}")%this->name %vol->name\
                        %bb.min.x() %bb.min.y() %bb.min.z()%bb.max.x() %bb.max.y() %bb.max.z();
                    continue;
                }

                /*const*/ Transform3d matrix = model_instance->get_matrix() * vol->get_matrix();
                if (build_volume.type() == BuildVolume_Type::Belt) {
                    matrix = model_instance->get_old_matrix() * vol->get_matrix();
                }

                BuildVolume::ObjectState state = build_volume.object_state(vol->mesh().its, matrix.cast<float>(), true /* may be below print bed */);
                if (state == BuildVolume::ObjectState::Inside)
                    // Volume is completely inside.
                    inside_outside |= INSIDE;
                else if (state == BuildVolume::ObjectState::Outside)
                    // Volume is completely outside.
                    inside_outside |= OUTSIDE;
                else if (state == BuildVolume::ObjectState::Below) {
                    // Volume below the print bed, thus it is completely outside, however this does not prevent the object to be printable
                    // if some of its volumes are still inside the build volume.
                } else
                    // Volume colliding with the build volume.
                    inside_outside |= INSIDE | OUTSIDE;
            }
        }
        model_instance->print_volume_state =
            inside_outside == (INSIDE | OUTSIDE) ? ModelInstancePVS_Partly_Outside :
            inside_outside == INSIDE ? ModelInstancePVS_Inside : ModelInstancePVS_Fully_Outside;
        if (inside_outside == INSIDE) {
            //BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(", object %1%'s instance inside print volum")%this->name;
            ++num_printable;
        }
    }
    //BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(", found %1% printable instances")%num_printable;
    return num_printable;
}

void ModelObject::print_info() const
{
    using namespace std;
    cout << fixed;
    boost::nowide::cout << "[" << boost::filesystem::path(this->input_file).filename().string() << "]" << endl;

    TriangleMesh mesh = this->raw_mesh();
    BoundingBoxf3 bb = mesh.bounding_box();
    Vec3d size = bb.size();
    cout << "size_x = " << size(0) << endl;
    cout << "size_y = " << size(1) << endl;
    cout << "size_z = " << size(2) << endl;
    cout << "min_x = " << bb.min(0) << endl;
    cout << "min_y = " << bb.min(1) << endl;
    cout << "min_z = " << bb.min(2) << endl;
    cout << "max_x = " << bb.max(0) << endl;
    cout << "max_y = " << bb.max(1) << endl;
    cout << "max_z = " << bb.max(2) << endl;
    cout << "number_of_facets = " << mesh.facets_count() << endl;

    cout << "manifold = "   << (mesh.stats().manifold() ? "yes" : "no") << endl;
    if (! mesh.stats().manifold())
        cout << "open_edges = " << mesh.stats().open_edges << endl;

    if (mesh.stats().repaired()) {
        const RepairedMeshErrors& stats = mesh.stats().repaired_errors;
        if (stats.degenerate_facets > 0)
            cout << "degenerate_facets = "  << stats.degenerate_facets << endl;
        if (stats.edges_fixed > 0)
            cout << "edges_fixed = "        << stats.edges_fixed       << endl;
        if (stats.facets_removed > 0)
            cout << "facets_removed = "     << stats.facets_removed    << endl;
        if (stats.facets_reversed > 0)
            cout << "facets_reversed = "    << stats.facets_reversed   << endl;
        if (stats.backwards_edges > 0)
            cout << "backwards_edges = "    << stats.backwards_edges   << endl;
    }
    cout << "number_of_parts =  " << mesh.stats().number_of_parts << endl;
    cout << "volume = "           << mesh.volume()                << endl;
}

std::string ModelObject::get_export_filename() const
{
    std::string ret = input_file;

    if (!name.empty())
    {
        if (ret.empty())
            // input_file was empty, just use name
            ret = name;
        else
        {
            // Replace file name in input_file with name, but keep the path and file extension.
            ret = (boost::filesystem::path(name).parent_path().empty()) ?
                (boost::filesystem::path(ret).parent_path() / name).make_preferred().string() : name;
        }
    }

    return ret;
}

TriangleMeshStats ModelObject::get_object_stl_stats() const
{
    TriangleMeshStats full_stats;
    full_stats.volume = 0.f;

    // fill full_stats from all objet's meshes
    for (ModelVolume* volume : this->volumes)
    {
        const TriangleMeshStats& stats = volume->mesh().stats();

        // initialize full_stats (for repaired errors)
        full_stats.open_edges           += stats.open_edges;
        full_stats.repaired_errors.merge(stats.repaired_errors);

        // another used satistics value
        if (volume->is_model_part()) {
            Transform3d trans = instances.empty() ? volume->get_matrix() : (volume->get_matrix() * instances[0]->get_matrix());
            full_stats.volume           += stats.volume * std::fabs(trans.matrix().block(0, 0, 3, 3).determinant());
            full_stats.number_of_parts  += stats.number_of_parts;
        }
    }

    return full_stats;
}

int ModelObject::get_repaired_errors_count(const int vol_idx /*= -1*/) const
{
    if (vol_idx >= 0)
        return this->volumes[vol_idx]->get_repaired_errors_count();

    const RepairedMeshErrors& stats = get_object_stl_stats().repaired_errors;

    return  stats.degenerate_facets + stats.edges_fixed     + stats.facets_removed +
            stats.facets_reversed + stats.backwards_edges;
}

bool ModelObject::has_solid_mesh() const
{
    for (const ModelVolume* volume : volumes)
        if (volume->is_model_part())
            return true;
    return false;
}


};

