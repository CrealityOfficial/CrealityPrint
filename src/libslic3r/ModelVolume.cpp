#include "ModelVolume.hpp"
#include "libslic3r/Geometry/ConvexHull.hpp"
#include "ModelObject.hpp"
#include "ModelInstance.hpp"
#include "Model.hpp"
#include "Print.hpp"

#include <map>
#include <boost/log/trivial.hpp>

namespace Slic3r {

bool ModelVolume::is_the_only_one_part() const 
{
    if (m_type != ModelVolumeType::MODEL_PART)
        return false;
    if (object == nullptr)
        return false;
    for (const ModelVolume *v : object->volumes) {
        if (v == nullptr)
            continue;
        // is this volume?
        if (v->id() == this->id())
            continue;
        // exist another model part in object?
        if (v->type() == ModelVolumeType::MODEL_PART)
            return false;
    }
    return true;
}

void ModelVolume::reset_extra_facets()
{
    this->supported_facets.reset();
    this->seam_facets.reset();
    this->mmu_segmentation_facets.reset();
    this->fuzzy_skin_facets.reset();
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

void ModelVolume::set_material_id(t_model_material_id material_id)
{
    m_material_id = material_id;
    // ensure m_material_id references an existing material
    if (! material_id.empty())
        this->object->get_model()->add_material(material_id);
}

ModelMaterial* ModelVolume::material() const
{
    return this->object->get_model()->get_material(m_material_id);
}

void ModelVolume::set_material(t_model_material_id material_id, const ModelMaterial &material)
{
    m_material_id = material_id;
    if (! material_id.empty())
        this->object->get_model()->add_material(material_id, material);
}

// Extract the current extruder ID based on this ModelVolume's config and the parent ModelObject's config.
int ModelVolume::extruder_id() const
{
    int extruder_id = -1;
    //if (this->is_model_part())
    {
        const ConfigOption *opt = this->config.option("extruder");
        if ((opt == nullptr) || (opt->getInt() == 0))
            opt = this->object->config.option("extruder");
        extruder_id = (opt == nullptr) ? 1 : opt->getInt();
    }
    return extruder_id;
}

bool ModelVolume::is_splittable() const
{
    // the call mesh.is_splittable() is expensive, so cache the value to calculate it only once
    if (m_is_splittable == -1)
        m_is_splittable = its_is_splittable(this->mesh().its);

    return m_is_splittable == 1;
}

// BBS
std::vector<int> ModelVolume::get_extruders() const
{
    if (m_type == ModelVolumeType::INVALID
        || m_type == ModelVolumeType::NEGATIVE_VOLUME
        || m_type == ModelVolumeType::SUPPORT_BLOCKER
        || m_type == ModelVolumeType::SUPPORT_ENFORCER)
        return std::vector<int>();

    if (mmu_segmentation_facets.timestamp() != mmuseg_ts) {
        std::vector<indexed_triangle_set> its_per_type;
        mmuseg_extruders.clear();
        mmuseg_ts = mmu_segmentation_facets.timestamp();
        mmu_segmentation_facets.get_facets(*this, its_per_type);
        for (int idx = 1; idx < its_per_type.size(); idx++) {
            indexed_triangle_set& its = its_per_type[idx];
            if (its.indices.empty())
                continue;

            mmuseg_extruders.push_back(idx);
        }
    }

    std::vector<int> volume_extruders = mmuseg_extruders;

    int volume_extruder_id = this->extruder_id();
    if (volume_extruder_id > 0)
        volume_extruders.push_back(volume_extruder_id);

    return volume_extruders;
}

void ModelVolume::update_extruder_count(size_t extruder_count)
{
    std::vector<int> used_extruders = get_extruders();
    for (int extruder_id : used_extruders) {
        if (extruder_id > extruder_count) {
            mmu_segmentation_facets.set_enforcer_block_type_limit(*this, (EnforcerBlockerType)extruder_count);
            break;
        }
    }
}
void ModelVolume::update_extruder_count_when_delete_filament(size_t extruder_count, size_t filament_id, int replace_filament_id)
{
    std::vector<int> used_extruders = get_extruders();
    for (int extruder_id : used_extruders) {
        if (extruder_id >= filament_id) {
            mmu_segmentation_facets.set_enforcer_block_type_limit(*this, (EnforcerBlockerType)(extruder_count),
                (EnforcerBlockerType)(filament_id),
                (EnforcerBlockerType)(replace_filament_id));
            break;
        }
    }
}

void ModelVolume::remap_mmu_painting_states(const std::vector<unsigned int>& remap)
{
    if (remap.empty() || mmu_segmentation_facets.empty())
        return;
    mmu_segmentation_facets.remap_states(*this, remap);
}

void ModelVolume::center_geometry_after_creation(bool update_source_offset)
{
    Vec3d shift = this->mesh().bounding_box().center();
    if (!shift.isApprox(Vec3d::Zero()))
    {
        if (m_mesh) {
            const_cast<TriangleMesh*>(m_mesh.get())->translate(-(float)shift(0), -(float)shift(1), -(float)shift(2));
            const_cast<TriangleMesh*>(m_mesh.get())->set_init_shift(shift);
        }
        if (m_convex_hull)
			const_cast<TriangleMesh*>(m_convex_hull.get())->translate(-(float)shift(0), -(float)shift(1), -(float)shift(2));
        translate(shift);
    }

    if (update_source_offset)
        source.mesh_offset = shift;
}

void ModelVolume::calculate_convex_hull()
{
    m_convex_hull = std::make_shared<TriangleMesh>(this->mesh().convex_hull_3d());
    assert(m_convex_hull.get());
}

//BBS: convex_hull_2d using convex_hull_3d
void  ModelVolume::calculate_convex_hull_2d(const Geometry::Transformation &transformation) const
{
    const indexed_triangle_set &its = m_convex_hull->its;
	if (its.vertices.empty())
        return;

    Points pts;
    Vec3d rotation = transformation.get_rotation();
    Vec3d mirror = transformation.get_mirror();
    Vec3d scale = transformation.get_scaling_factor();
    //rotation(2) = 0.f;
    Transform3d new_matrix = Geometry::assemble_transform(Vec3d::Zero(), rotation, scale, mirror);
    new_matrix = transformation.get_matrix_no_offset();
    pts.reserve(its.vertices.size());
    // Using the shared vertices should be a bit quicker than using the STL faces.
    for (size_t i = 0; i < its.vertices.size(); ++ i) {
        Vec3d p = new_matrix * its.vertices[i].cast<double>();
        pts.emplace_back(coord_t(scale_(p.x())), coord_t(scale_(p.y())));
    }
    //TODO, do we need to remove the duplicate points before convex_hull?
    m_cached_2d_polygon = Slic3r::Geometry::convex_hull(pts);

    m_convex_hull_2d = m_cached_2d_polygon;
    m_convex_hull_2d.translate(scale_(transformation.get_offset(X)), scale_(transformation.get_offset(Y)));
    //int size = m_cached_2d_polygon.size();
    //BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": size %1%, offset {%2%, %3%}")% size% transformation.get_offset(X)% transformation.get_offset(Y);
    //for (int i = 0; i < size; i++)
    //    BOOST_LOG_TRIVIAL(info) << boost::format(": point %1%, position {%2%, %3%}")% i% m_cached_2d_polygon[i].x()% m_cached_2d_polygon[i].y();
	//m_convex_hull_2d.rotate(transformation.get_rotation(Z));
    //m_convex_hull_2d.scale(transformation.get_scaling_factor(X), transformation.get_scaling_factor(Y));
}

const Polygon& ModelVolume::get_convex_hull_2d(const Transform3d &trafo_instance) const
{
    Transform3d  new_matrix;

    new_matrix = trafo_instance * m_transformation.get_matrix();

    auto need_recompute = [](Geometry::Transformation& old_transform, Geometry::Transformation& new_transform)->bool {
            //double old_rot_x = old_transform.get_rotation(X);
            //double old_rot_y = old_transform.get_rotation(Y);
            //double new_rot_x = new_transform.get_rotation(X);
            //double new_rot_y = new_transform.get_rotation(Y);
            const Vec3d &old_rotation = old_transform.get_rotation();
            const Vec3d &new_rotation = new_transform.get_rotation();
            const Vec3d &old_mirror = old_transform.get_mirror();
            const Vec3d &new_mirror = new_transform.get_mirror();
            const Vec3d &old_scaling = old_transform.get_scaling_factor();
            const Vec3d &new_scaling = new_transform.get_scaling_factor();

            if ((old_scaling != new_scaling) || (old_rotation != new_rotation) || (old_mirror != new_mirror))
                return true;
            else
                return false;
        };

    if ((new_matrix.matrix() != m_cached_trans_matrix.matrix()) || !m_convex_hull_2d.is_valid())
    {
        Geometry::Transformation new_trans(new_matrix), old_trans(m_cached_trans_matrix);

        if (need_recompute(old_trans, new_trans) || !m_convex_hull_2d.is_valid())
        {
            //need to update
            calculate_convex_hull_2d(new_trans);
        }
        else
        {
            m_convex_hull_2d = m_cached_2d_polygon;
            m_convex_hull_2d.translate(scale_(new_trans.get_offset(X)), scale_(new_trans.get_offset(Y)));
            //m_convex_hull_2d.rotate(new_trans.get_rotation(Z));
            //m_convex_hull_2d.scale(new_trans.get_scaling_factor(X), new_trans.get_scaling_factor(Y));
            //int size = m_cached_2d_polygon.size();
            //BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": use previous cached, size %1%, offset {%2%, %3%}")% size% new_trans.get_offset(X)% new_trans.get_offset(Y);
            //for (int i = 0; i < size; i++)
            //    BOOST_LOG_TRIVIAL(info) << boost::format(": point %1%, position {%2%, %3%}")% i% m_cached_2d_polygon[i].x()% m_cached_2d_polygon[i].y();
        }
        m_cached_trans_matrix = new_matrix;
    }

    return m_convex_hull_2d;
}

int ModelVolume::get_repaired_errors_count() const
{
    const RepairedMeshErrors &stats = this->mesh().stats().repaired_errors;

    return  stats.degenerate_facets + stats.edges_fixed     + stats.facets_removed +
            stats.facets_reversed + stats.backwards_edges;
}

const TriangleMesh& ModelVolume::get_convex_hull() const
{
    return *m_convex_hull.get();
}

//BBS: refine the model part names
ModelVolumeType ModelVolume::type_from_string(const std::string &s)
{
    // New type (supporting the support enforcers & blockers)
    if (s == "normal_part")
		return ModelVolumeType::MODEL_PART;
    if (s == "negative_part")
        return ModelVolumeType::NEGATIVE_VOLUME;
    if (s == "modifier_part")
		return ModelVolumeType::PARAMETER_MODIFIER;
    if (s == "support_enforcer")
		return ModelVolumeType::SUPPORT_ENFORCER;
    if (s == "support_blocker")
		return ModelVolumeType::SUPPORT_BLOCKER;
    //assert(s == "0");
    // Default value if invalud type string received.
	return ModelVolumeType::MODEL_PART;
}

//BBS: refine the model part names
std::string ModelVolume::type_to_string(const ModelVolumeType t)
{
    switch (t) {
	case ModelVolumeType::MODEL_PART:         return "normal_part";
    case ModelVolumeType::NEGATIVE_VOLUME:    return "negative_part";
	case ModelVolumeType::PARAMETER_MODIFIER: return "modifier_part";
	case ModelVolumeType::SUPPORT_ENFORCER:   return "support_enforcer";
	case ModelVolumeType::SUPPORT_BLOCKER:    return "support_blocker";
    default:
        assert(false);
        return "normal_part";
    }
}

// Split this volume, append the result to the object owning this volume.
// Return the number of volumes created from this one.
// This is useful to assign different materials to different volumes of an object.
size_t ModelVolume::split(unsigned int max_extruders)
{
    std::vector<TriangleMesh> meshes = this->mesh().split();
    if (meshes.size() <= 1)
        return 1;

    // splited volume should not be text object
    if (text_configuration.has_value())
        text_configuration.reset();

    size_t idx = 0;
    size_t ivolume = std::find(this->object->volumes.begin(), this->object->volumes.end(), this) - this->object->volumes.begin();
    const std::string name = this->name;

    unsigned int extruder_counter = 0;
    const Vec3d offset = this->get_offset();

    for (TriangleMesh &mesh : meshes) {
        if (mesh.empty())
            // Repair may have removed unconnected triangles, thus emptying the mesh.
            continue;

        if (idx == 0) {
            this->set_mesh(std::move(mesh));
            this->calculate_convex_hull();
            this->invalidate_convex_hull_2d();
            // Assign a new unique ID, so that a new GLVolume will be generated.
            this->set_new_unique_id();
            // reset the source to disable reload from disk
            this->source = ModelVolume::Source();

            // BBS: reset facet annotations
            this->mmu_segmentation_facets.reset();
            this->exterior_facets.reset();
            this->supported_facets.reset();
            this->seam_facets.reset();
            this->fuzzy_skin_facets.reset();
        }
        else
            this->object->volumes.insert(this->object->volumes.begin() + (++ivolume), new ModelVolume(object, *this, std::move(mesh)));

        this->object->volumes[ivolume]->set_offset(Vec3d::Zero());
        this->object->volumes[ivolume]->center_geometry_after_creation();
        this->object->volumes[ivolume]->translate(offset);
        this->object->volumes[ivolume]->name = name + "_" + std::to_string(idx + 1);
        //BBS: always set the extruder id the same as original
        this->object->volumes[ivolume]->config.set("extruder", this->extruder_id());
        //this->object->volumes[ivolume]->config.set("extruder", auto_extruder_id(max_extruders, extruder_counter));
        this->object->volumes[ivolume]->m_is_splittable = 0;
        ++ idx;
    }

    // discard volumes for which the convex hull was not generated or is degenerate
    size_t i = 0;
    while (i < this->object->volumes.size()) {
        const std::shared_ptr<const TriangleMesh> &hull = this->object->volumes[i]->get_convex_hull_shared_ptr();
        if (hull == nullptr || hull->its.vertices.empty() || hull->its.indices.empty()) {
            this->object->delete_volume(i);
            --idx;
            --i;
        }
        ++i;
    }

    return idx;
}

void ModelVolume::translate(const Vec3d& displacement)
{
    set_offset(get_offset() + displacement);
}

void ModelVolume::scale(const Vec3d& scaling_factors)
{
    set_scaling_factor(get_scaling_factor().cwiseProduct(scaling_factors));
}

void ModelObject::scale_to_fit(const Vec3d &size)
{
    Vec3d orig_size = this->bounding_box_exact().size();
    double factor = std::min(
        size.x() / orig_size.x(),
        std::min(
            size.y() / orig_size.y(),
            size.z() / orig_size.z()
        )
    );
    this->scale(factor);
}

void ModelVolume::assign_new_unique_ids_recursive()
{
    ObjectBase::set_new_unique_id();
    config.set_new_unique_id();
    supported_facets.set_new_unique_id();
    seam_facets.set_new_unique_id();
    mmu_segmentation_facets.set_new_unique_id();
    fuzzy_skin_facets.set_new_unique_id();
}

void ModelVolume::rotate(double angle, Axis axis)
{
    switch (axis)
    {
    case X: { rotate(angle, Vec3d::UnitX()); break; }
    case Y: { rotate(angle, Vec3d::UnitY()); break; }
    case Z: { rotate(angle, Vec3d::UnitZ()); break; }
    default: break;
    }
}

void ModelVolume::rotate(double angle, const Vec3d& axis)
{
    set_rotation(get_rotation() + Geometry::extract_euler_angles(Eigen::Quaterniond(Eigen::AngleAxisd(angle, axis)).toRotationMatrix()));
}

void ModelVolume::mirror(Axis axis)
{
    Vec3d mirror = get_mirror();
    switch (axis)
    {
    case X: { mirror(0) *= -1.0; break; }
    case Y: { mirror(1) *= -1.0; break; }
    case Z: { mirror(2) *= -1.0; break; }
    default: break;
    }
    set_mirror(mirror);
}

// This method could only be called before the meshes of this ModelVolumes are not shared!
void ModelVolume::scale_geometry_after_creation(const Vec3f& versor)
{
	const_cast<TriangleMesh*>(m_mesh.get())->scale(versor);
    if (m_convex_hull->empty())
        //BBS: recompute the convex hull if it is null for previous too small
        this->calculate_convex_hull();
    else
        const_cast<TriangleMesh*>(m_convex_hull.get())->scale(versor);
}

void ModelVolume::transform_this_mesh(const Transform3d &mesh_trafo, bool fix_left_handed)
{
	TriangleMesh mesh = this->mesh();
	mesh.transform(mesh_trafo, fix_left_handed);
	this->set_mesh(std::move(mesh));
    TriangleMesh convex_hull = this->get_convex_hull();
    convex_hull.transform(mesh_trafo, fix_left_handed);
    m_convex_hull = std::make_shared<TriangleMesh>(std::move(convex_hull));
    // Let the rest of the application know that the geometry changed, so the meshes have to be reloaded.
    this->set_new_unique_id();
}

void ModelVolume::transform_this_mesh(const Matrix3d &matrix, bool fix_left_handed)
{
	TriangleMesh mesh = this->mesh();
	mesh.transform(matrix, fix_left_handed);
	this->set_mesh(std::move(mesh));
    TriangleMesh convex_hull = this->get_convex_hull();
    convex_hull.transform(matrix, fix_left_handed);
    m_convex_hull = std::make_shared<TriangleMesh>(std::move(convex_hull));
    // Let the rest of the application know that the geometry changed, so the meshes have to be reloaded.
    this->set_new_unique_id();
}

void ModelVolume::convert_from_imperial_units()
{
    assert(! this->source.is_converted_from_meters);
    this->scale_geometry_after_creation(25.4f);
    this->set_offset(Vec3d(0, 0, 0));
    this->source.is_converted_from_inches = true;
}

void ModelVolume::convert_from_meters()
{
    assert(! this->source.is_converted_from_inches);
    this->scale_geometry_after_creation(1000.f);
    this->set_offset(Vec3d(0, 0, 0));
    this->source.is_converted_from_meters = true;
}

indexed_triangle_set FacetsAnnotation::get_facets(const ModelVolume& mv, EnforcerBlockerType type) const
{
    TriangleSelector selector(mv.mesh());
    // Reset of TriangleSelector is done inside TriangleSelector's constructor, so we don't need it to perform it again in deserialize().
    selector.deserialize(m_data, false);
    return selector.get_facets(type);
}

// BBS
void FacetsAnnotation::get_facets(const ModelVolume& mv, std::vector<indexed_triangle_set>& facets_per_type) const
{
    TriangleSelector selector(mv.mesh());
    selector.deserialize(m_data, false);
    selector.get_facets(facets_per_type);
}

void FacetsAnnotation::set_enforcer_block_type_limit(const ModelVolume& mv,
    EnforcerBlockerType max_type,
    EnforcerBlockerType to_delete_filament,
    EnforcerBlockerType replace_filament)
{
    TriangleSelector selector(mv.mesh());
    selector.deserialize(m_data, false, max_type, to_delete_filament, replace_filament);
    this->set(selector);
}

void FacetsAnnotation::remap_states(const ModelVolume& mv, const std::vector<unsigned int>& remap)
{
    if (remap.empty())
        return;
    TriangleSelector selector(mv.mesh());
    selector.deserialize(m_data, false);
    selector.remap_states(remap);
    this->set(selector);
}

indexed_triangle_set FacetsAnnotation::get_facets_strict(const ModelVolume& mv, EnforcerBlockerType type) const
{
    TriangleSelector selector(mv.mesh());
    // Reset of TriangleSelector is done inside TriangleSelector's constructor, so we don't need it to perform it again in deserialize().
    selector.deserialize(m_data, false);
    return selector.get_facets_strict(type);
}

bool FacetsAnnotation::has_facets(const ModelVolume& mv, EnforcerBlockerType type) const
{
    return TriangleSelector::has_facets(m_data, type);
}

bool FacetsAnnotation::set(const TriangleSelector& selector)
{
    TriangleSelector::TriangleSplittingData sel_map = selector.serialize();
    if (sel_map != m_data) {
        m_data = std::move(sel_map);
        this->touch();
        return true;
    }
    return false;
}

void FacetsAnnotation::reset()
{
    m_data.triangles_to_split.clear();
    m_data.bitstream.clear();
    this->touch();
}

// Following function takes data from a triangle and encodes it as string
// of hexadecimal numbers (one digit per triangle). Used for 3MF export,
// changing it may break backwards compatibility !!!!!
std::string FacetsAnnotation::get_triangle_as_string(int triangle_idx) const
{
    std::string out;

    auto triangle_it = std::lower_bound(m_data.triangles_to_split.begin(), m_data.triangles_to_split.end(), triangle_idx,
                                        [](const TriangleSelector::TriangleBitStreamMapping& l, const int r) { return l.triangle_idx < r; });
    if (triangle_it != m_data.triangles_to_split.end() && triangle_it->triangle_idx == triangle_idx) {
        int offset = triangle_it->bitstream_start_idx;
        int end    = ++triangle_it == m_data.triangles_to_split.end() ? int(m_data.bitstream.size()) : triangle_it->bitstream_start_idx;
        while (offset < end) {
            int next_code = 0;
            for (int i = 3; i >= 0; --i) {
                next_code = next_code << 1;
                next_code |= int(m_data.bitstream[offset + i]);
            }
            offset += 4;

            assert(next_code >= 0 && next_code <= 15);
            char digit = next_code < 10 ? next_code + '0' : (next_code - 10) + 'A';
            out.insert(out.begin(), digit);
        }
    }
    return out;
}

// Recover triangle splitting & state from string of hexadecimal values previously
// generated by get_triangle_as_string. Used to load from 3MF.
void FacetsAnnotation::set_triangle_from_string(int triangle_id, const std::string& str)
{
    assert(!str.empty());
    assert(m_data.triangles_to_split.empty() || m_data.triangles_to_split.back().triangle_idx < triangle_id);
    m_data.triangles_to_split.emplace_back(triangle_id, int(m_data.bitstream.size()));

    const size_t bitstream_start_idx = m_data.bitstream.size();
    for (auto it = str.crbegin(); it != str.crend(); ++it) {
        const char ch  = *it;
        int        dec = 0;
        if (ch >= '0' && ch <= '9')
            dec = int(ch - '0');
        else if (ch >= 'A' && ch <= 'F')
            dec = 10 + int(ch - 'A');
        else
            assert(false);

        // Convert to binary and append into code.
        for (int i = 0; i < 4; ++i)
            m_data.bitstream.insert(m_data.bitstream.end(), bool(dec & (1 << i)));
    }

    m_data.update_used_states(bitstream_start_idx);
}

bool FacetsAnnotation::equals(const FacetsAnnotation &other) const
{
    const auto& data = other.get_data();
    return (m_data == data);
}

};

