#include "ModelInstance.hpp"
#include "Model.hpp"
#include "ModelObject.hpp"
#include "Arrange.hpp"
#include "ModelVolume.hpp"

#include <algorithm>
#include <initializer_list>

namespace Slic3r {

static const ConfigOption* find_config_option(std::initializer_list<const ConfigBase*> configs, const char* key)
{
    for (auto it = configs.end(); it != configs.begin();) {
        --it;
        if (*it == nullptr)
            continue;
        if (const ConfigOption* opt = (*it)->option(key))
            return opt;
    }
    return nullptr;
}

static int config_int(std::initializer_list<const ConfigBase*> configs, const char* key, int default_value = 0)
{
    if (const ConfigOption* opt = find_config_option(configs, key))
        return opt->getInt();
    return default_value;
}

static double config_float(std::initializer_list<const ConfigBase*> configs, const char* key, double default_value = 0.0)
{
    if (const ConfigOption* opt = find_config_option(configs, key))
        return opt->getFloat();
    return default_value;
}

static void append_role_extruders(std::vector<int>& extruders, std::initializer_list<const ConfigBase*> configs)
{
    const bool has_brim = config_int(configs, "brim_type", int(btNoBrim)) != int(btNoBrim);
    if (config_int(configs, "wall_loops") > 0 || has_brim) {
        const int extruder = config_int(configs, "wall_filament");
        if (extruder > 0)
            extruders.push_back(extruder);
    }
    if (config_float(configs, "sparse_infill_density") > 0.0) {
        const int extruder = config_int(configs, "sparse_infill_filament");
        if (extruder > 0)
            extruders.push_back(extruder);
    }
    if (config_int(configs, "top_shell_layers") > 0 || config_int(configs, "bottom_shell_layers") > 0) {
        const int extruder = config_int(configs, "solid_infill_filament");
        if (extruder > 0)
            extruders.push_back(extruder);
    }
}

//BBS adhesion coefficients from model object class
double getadhesionCoeff(const ModelVolumePtrs objectVolumes)
{
    double adhesionCoeff = 1;
    for (const ModelVolume* modelVolume : objectVolumes) {
        if (Model::extruderParamsMap.find(modelVolume->extruder_id()) != Model::extruderParamsMap.end())
            if (Model::extruderParamsMap.at(modelVolume->extruder_id()).materialName == "PETG" ||
                Model::extruderParamsMap.at(modelVolume->extruder_id()).materialName == "PCTG") {
                adhesionCoeff = 2;
            }
            else if (Model::extruderParamsMap.at(modelVolume->extruder_id()).materialName == "TPU") {
                adhesionCoeff = 0.5;
            }
    }
    return adhesionCoeff;
}

//BBS maximum temperature difference from model object class
double getTemperatureFromExtruder(const ModelVolumePtrs objectVolumes) {
    // BBS: FIXME
#if 1
    std::vector<size_t> extruders;
    for (const ModelVolume* modelVolume : objectVolumes) {
        if (modelVolume->extruder_id() >= 0)
            extruders.push_back(modelVolume->extruder_id());
    }

    double maxDeltaTemp = 0;
    for (auto extruderID : extruders) {
        if (Model::extruderParamsMap.find(extruderID) != Model::extruderParamsMap.end())
            if (Model::extruderParamsMap.at(extruderID).bedTemp != 0){
                maxDeltaTemp = std::max(maxDeltaTemp, (double)Model::extruderParamsMap.at(extruderID).bedTemp);
                break;
            }
    }
    return maxDeltaTemp;
#else
    return 0.f;
#endif
}

void ModelInstance::transform_mesh(TriangleMesh* mesh, bool dont_translate) const
{
    mesh->transform(dont_translate ? get_matrix_no_offset() : get_matrix());
}
    
BoundingBoxf3 ModelInstance::transform_bounding_box(const BoundingBoxf3 &bbox, bool dont_translate) const
{
    return bbox.transformed(dont_translate ? get_matrix_no_offset() : get_matrix());
}
    
Vec3d ModelInstance::transform_vector(const Vec3d& v, bool dont_translate) const
{
    return dont_translate ? get_matrix_no_offset() * v : get_matrix() * v;
}
    
void ModelInstance::transform_polygon(Polygon* polygon) const
{
        // CHECK_ME -> Is the following correct or it should take in account all three rotations ?
    polygon->rotate(get_rotation(Z)); // rotate around polygon origin
    // CHECK_ME -> Is the following correct ?
    polygon->scale(get_scaling_factor(X), get_scaling_factor(Y)); // scale around polygon origin
}

bool ModelInstance::is_printable() const 
{
    return object->printable && printable && (print_volume_state == ModelInstancePVS_Inside); 
}

// max printing speed, difference in bed temperature and envirument temperature and bed adhension coefficients are considered
double ModelInstance::get_auto_brim_width(double deltaT, double adhension) const
{
    BoundingBoxf3 raw_bbox = object->raw_mesh_bounding_box();
    double maxSpeed = Model::findMaxSpeed(object);

    auto bbox_size = transform_bounding_box(raw_bbox).size();
    double height_to_area = std::max(bbox_size(2) / (bbox_size(0) * bbox_size(0) * bbox_size(1)),
        bbox_size(2) / (bbox_size(1) * bbox_size(1) * bbox_size(0)));
    double thermalLength = sqrt(bbox_size(0)* bbox_size(0) + bbox_size(1)* bbox_size(1));
    double thermalLengthRef = Model::getThermalLength(object->volumes);

    double brim_width = adhension * std::min(std::min(std::max(height_to_area * 200 * maxSpeed/200, thermalLength * 8. / thermalLengthRef * std::min(bbox_size(2), 30.) / 30.), 20.), 1.5 * thermalLength);
    // small brims are omitted
    if (brim_width < 5 && brim_width < 1.5 * thermalLength)
        brim_width = 0;
    return brim_width;
}

//BBS: instance's convex_hull_2d
Polygon ModelInstance::convex_hull_2d()
{
    //BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": name %1%, is_valid %2%")% this->object->name.c_str()% convex_hull.is_valid();
    //if (!convex_hull.is_valid())
    { // this logic is not working right now, as moving instance doesn't update convex_hull
        const Transform3d& trafo_instance = get_matrix();
        convex_hull = get_object()->convex_hull_2d(trafo_instance);
    }
    //int size = convex_hull.size();
    //BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": convex_hull, point size %1%")% size;
    //for (int i = 0; i < size; i++)
    //    BOOST_LOG_TRIVIAL(info) << boost::format(": point %1%, position {%2%, %3%}")% i% convex_hull[i].x()% convex_hull[i].y();

    return convex_hull;
}

//BBS: invalidate instance's convex_hull_2d
void ModelInstance::invalidate_convex_hull_2d()
{
    convex_hull.clear();
}
 
double ModelInstance::get_auto_brim_width() const
{
    return 0.;
    double adhcoeff = getadhesionCoeff(object->volumes);
    double DeltaT = getTemperatureFromExtruder(object->volumes);
    // get auto brim width (Note even if the global brim_type=btOuterBrim, we can still go into this branch)
    return get_auto_brim_width(DeltaT, adhcoeff);
}

void ModelInstance::get_arrange_polygon(void *ap, const Slic3r::DynamicPrintConfig &config_global) const
{
//    static const double SIMPLIFY_TOLERANCE_MM = 0.1;

    //Vec3d rotation = get_rotation();
    //rotation.z()   = 0.;
    //Transform3d trafo_instance =
    //    Geometry::assemble_transform(get_offset().z() * Vec3d::UnitZ(), rotation, get_scaling_factor(), get_mirror());

    //Polygon p = get_object()->convex_hull_2d(trafo_instance);

    Vec3d rotation = get_rotation();
    rotation.z()   = 0.;
    Geometry::Transformation t(m_transformation);
    t.set_offset(get_offset().z() * Vec3d::UnitZ());
    t.set_rotation(rotation);
    Polygon p = get_object()->convex_hull_2d(t.get_matrix());

    //if (!p.points.empty()) {
    //    Polygons pp{p};
    //    pp = p.simplify(scaled<double>(SIMPLIFY_TOLERANCE_MM));
    //    if (!pp.empty()) p = pp.front();
    //}

    arrangement::ArrangePolygon& ret = *(arrangement::ArrangePolygon*)ap;
    ret.poly.contour = std::move(p);
    ret.translation  = Vec2crd{scaled(get_offset(X)), scaled(get_offset(Y))};
    ret.rotation     = get_rotation(Z);

    //BBS: add materials related information
    ModelVolume *volume = NULL;
    for (size_t i = 0; i < object->volumes.size(); ++i) {
        if (object->volumes[i]->is_model_part()) {
            volume = object->volumes[i];
            if (!volume) {
                BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << "invalid object, should not happen";
                return;
            }
            auto ve = object->volumes[i]->get_extruders();
            ret.extrude_ids.insert(ret.extrude_ids.end(), ve.begin(), ve.end());
            append_role_extruders(ret.extrude_ids, {&config_global, &object->config.get(), &volume->config.get()});
        } else if (object->volumes[i]->is_modifier()) {
            append_role_extruders(ret.extrude_ids, {&config_global, &object->config.get(), &object->volumes[i]->config.get()});
        }
    }

    append_role_extruders(ret.extrude_ids, {&config_global, &object->config.get()});
    for (const auto& layer_range : object->layer_config_ranges) {
        if (layer_range.second.has("extruder")) {
            if (const int extruder = layer_range.second.option("extruder")->getInt(); extruder > 0)
                ret.extrude_ids.push_back(extruder);
        }
        append_role_extruders(ret.extrude_ids, {&config_global, &object->config.get(), &layer_range.second.get()});
    }

    // get per-object support extruders
    auto op = object->get_config_value<ConfigOptionBool>(config_global, "enable_support");
    bool is_support_enabled = op && op->getBool();
    if (is_support_enabled) {
        auto op1 = object->get_config_value<ConfigOptionInt>(config_global, "support_filament");
        auto op2 = object->get_config_value<ConfigOptionInt>(config_global, "support_interface_filament");
        int extruder_id;
        // id==0 means follow previous material, so need not be recorded
        if (op1 && (extruder_id = op1->getInt()) > 0) ret.extrude_ids.push_back(extruder_id);
        if (op2 && (extruder_id = op2->getInt()) > 0) ret.extrude_ids.push_back(extruder_id);
    }
    ret.bed_idx = 0;
    if (ret.extrude_ids.empty()) //the default extruder
        ret.extrude_ids.push_back(1);
    std::vector<int> unique_extruders;
    unique_extruders.reserve(ret.extrude_ids.size());
    for (int extruder : ret.extrude_ids)
        if (std::find(unique_extruders.begin(), unique_extruders.end(), extruder) == unique_extruders.end())
            unique_extruders.push_back(extruder);
    ret.extrude_ids = std::move(unique_extruders);
}


void ModelInstance::apply_arrange_result(const Vec2d& offs, double rotation)
{
    // write the transformation data into the model instance
    set_rotation(Z, rotation);
    set_offset(X, unscale<double>(offs(X)));
    set_offset(Y, unscale<double>(offs(Y)));
    this->object->invalidate_bounding_box();
}

};
