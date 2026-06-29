#include "SlicerBridgeDiagnostics.hpp"

#include "slic3r/GUI/PartPlate.hpp"
#include "slic3r/GUI/Plater.hpp"

#include "libslic3r/Model.hpp"
#include "libslic3r/ModelInstance.hpp"
#include "libslic3r/ModelObject.hpp"

#include <string>
#include <vector>

using json = nlohmann::json;

namespace Slic3r {
namespace GUI {
namespace Bridge {
namespace {

struct CurrentPlateInstanceSnapshot
{
    int obj_id = -1;
    int instance_id = -1;
    std::string name;
    BoundingBoxf3 bbox;
};

int JsonIntValue(const json& item, const char* key, int fallback = -1)
{
    if (!item.is_object() || !item.contains(key))
        return fallback;
    const json& value = item[key];
    if (value.is_number_integer())
        return value.get<int>();
    if (value.is_string()) {
        try { return std::stoi(value.get<std::string>()); } catch (...) { return fallback; }
    }
    return fallback;
}

json JsonArrayValue(const json& item, const char* key)
{
    if (!item.is_object() || !item.contains(key) || !item[key].is_array())
        return json::array();
    return item[key];
}

json BuildObjectRef(const json& item)
{
    const int object_id = JsonIntValue(item, "object_id");
    const int object_index = JsonIntValue(item, "object_index", object_id);
    const int instance_id = JsonIntValue(item, "instance_id");
    return {
        {"object_id", object_id},
        {"object_index", object_index},
        {"object_name", item.value("object_name", item.value("name", std::string()))},
        {"instance_id", instance_id},
        {"instance_index", JsonIntValue(item, "instance_index", instance_id)}
    };
}

std::string OutsideIssueType(const json& item)
{
    if (item.value("fully_outside_plate", false))
        return "model_fully_outside_plate";
    if (item.value("exceeds_height", false) && !item.value("outside_x", false) && !item.value("outside_y", false))
        return "model_exceeds_height";
    return "model_out_of_bed";
}

std::string OutsideIssueTitle(const std::string& issue_type)
{
    if (issue_type == "model_fully_outside_plate")
        return "Object is fully outside the current plate build volume.";
    if (issue_type == "model_exceeds_height")
        return "Object exceeds build height limit.";
    return "Object is outside the current plate build volume.";
}

json OutOfBoundsAxes(const json& item)
{
    json axes = json::array();
    if (item.value("outside_x", false))
        axes.push_back("x");
    if (item.value("outside_y", false))
        axes.push_back("y");
    if (item.value("exceeds_height", false))
        axes.push_back("z");
    return axes;
}

bool CanMoveObjectFixBoundsIssue(const json& item)
{
    return !item.value("exceeds_height", false) && !item.value("larger_than_plate", false);
}

json OutsideCandidateTools(const json& item)
{
    if (CanMoveObjectFixBoundsIssue(item))
        return json::array({"move_object", "auto_arrange"});
    if (item.value("exceeds_height", false))
        return json::array({"scale_object", "auto_orient_model"});
    return json::array({"scale_object"});
}

json OutsideRepairHints(const json& item, const json& hint_args)
{
    if (!CanMoveObjectFixBoundsIssue(item))
        return json::array();

    return json::array({{
        {"tool", "move_object"},
        {"confidence", 0.9},
        {"args_hint", hint_args}
    }});
}

json BuildUnlocatedModelFitIssue(const json& validation)
{
    const int plate_index = validation.value("plate_index", -1);
    const std::string message = "Current plate validation failed, but no object-level diagnostics were produced.";
    return {
        {"issue_id", "current_plate_model_not_fit_unlocated"},
        {"issue_type", "scene_diagnostics_incomplete"},
        {"severity", "error"},
        {"source", "part_plate"},
        {"blocking", true},
        {"plate_index", plate_index},
        {"objects", json::array()},
        {"related_objects", json::array()},
        {"geometry", json::object()},
        {"params", json::object()},
        {"message_key", "current_plate_model_not_fit_unlocated"},
        {"message_display", message},
        {"title", message},
        {"description", message},
        {"candidate_tools", json::array()},
        {"repair_hints", json::array()},
        {"evidence", {
            {"source", "part_plate"},
            {"current_plate_validation", validation}
        }}
    };
}
} // namespace

json BuildCurrentPlateValidationSnapshot(Plater* plater, bool model_fits, bool validate_error)
{
    json snapshot = {
        {"available", false},
        {"model_fits", model_fits},
        {"validate_error", validate_error},
        {"validate_error_message", std::string()},
        {"has_model_overlap", false},
        {"has_bbox_overlap", false},
        {"has_model_outside", false},
        {"plate_index", -1},
        {"checked_instance_count", 0},
        {"overlap_method", "trusted_validation_only"},
        {"outside_instances", json::array()},
        {"overlap_pairs", json::array()},
        {"bbox_overlap_pairs", json::array()}
    };

    if (!plater) {
        snapshot["reason"] = "plater_unavailable";
        return snapshot;
    }

    PartPlateList& plate_list = plater->get_partplate_list();
    const int plate_index = plate_list.get_curr_plate_index();
    const int plate_count = plate_list.get_plate_count();
    snapshot["plate_index"] = plate_index;
    if (plate_count <= 0 || plate_index < 0 || plate_index >= plate_count) {
        snapshot["reason"] = "plate_unavailable";
        return snapshot;
    }

    PartPlate* plate = plate_list.get_curr_plate();
    if (!plate) {
        snapshot["reason"] = "plate_unavailable";
        return snapshot;
    }

    snapshot["available"] = true;
    snapshot["validate_error_message"] = plate->get_validate_error_message();

    Model& model = plater->model();
    std::vector<CurrentPlateInstanceSnapshot> instances;

    for (int obj_id = 0; obj_id < static_cast<int>(model.objects.size()); ++obj_id) {
        ModelObject* object = model.objects[obj_id];
        if (!object)
            continue;

        for (int instance_id = 0; instance_id < static_cast<int>(object->instances.size()); ++instance_id) {
            ModelInstance* instance = object->instances[instance_id];
            if (!instance)
                continue;

            const bool instance_printable = instance->is_printable();
            BoundingBoxf3 bbox = object->instance_convex_hull_bounding_box(static_cast<size_t>(instance_id));
            const bool intersects_current_plate = plate->intersect_instance(obj_id, instance_id, &bbox);
            const int registered_plate_index = plate_list.find_instance(obj_id, instance_id);
            const bool registered_on_plate = registered_plate_index == plate_index;
            if (registered_plate_index >= 0 && registered_plate_index != plate_index)
                continue;
            if (!intersects_current_plate && !registered_on_plate && !(plate_count == 1 && registered_plate_index < 0))
                continue;

            const BoundingBoxf3 build_volume = plate->get_build_volume();
            const double eps = 1e-6;
            const bool outside_x = bbox.min.x() < build_volume.min.x() - eps || bbox.max.x() > build_volume.max.x() + eps;
            const bool outside_y = bbox.min.y() < build_volume.min.y() - eps || bbox.max.y() > build_volume.max.y() + eps;
            const bool exceeds_height = bbox.max.z() > build_volume.max.z() + eps;
            const bool fully_outside_plate = !build_volume.intersects(bbox);
            const bool larger_than_plate =
                (bbox.max.x() - bbox.min.x()) > (build_volume.max.x() - build_volume.min.x()) + eps ||
                (bbox.max.y() - bbox.min.y()) > (build_volume.max.y() - build_volume.min.y()) + eps;
            const bool outside = outside_x || outside_y || exceeds_height || fully_outside_plate ||
                plate->check_outside(obj_id, instance_id, &bbox) ||
                (registered_on_plate && !plate->contain_instance_totally(obj_id, instance_id));
            if (outside) {
                snapshot["has_model_outside"] = true;
                snapshot["outside_instances"].push_back({
                    {"object_id", obj_id},
                    {"object_index", obj_id},
                    {"instance_id", instance_id},
                    {"instance_index", instance_id},
                    {"name", object->name},
                    {"object_name", object->name},
                    {"plate_index", plate_index},
                    {"registered_plate_index", registered_plate_index},
                    {"registered_on_current_plate", registered_on_plate},
                    {"intersects_current_plate", intersects_current_plate},
                    {"is_printable", instance_printable},
                    {"bbox_min", {bbox.min.x(), bbox.min.y(), bbox.min.z()}},
                    {"bbox_max", {bbox.max.x(), bbox.max.y(), bbox.max.z()}},
                    {"build_volume_min", {build_volume.min.x(), build_volume.min.y(), build_volume.min.z()}},
                    {"build_volume_max", {build_volume.max.x(), build_volume.max.y(), build_volume.max.z()}},
                    {"outside_x", outside_x},
                    {"outside_y", outside_y},
                    {"exceeds_height", exceeds_height},
                    {"fully_outside_plate", fully_outside_plate},
                    {"larger_than_plate", larger_than_plate}
                });
            }

            instances.push_back({obj_id, instance_id, object->name, bbox});
        }
    }

    snapshot["checked_instance_count"] = instances.size();
    for (size_t i = 0; i < instances.size(); ++i) {
        for (size_t j = i + 1; j < instances.size(); ++j) {
            if (!instances[i].bbox.intersects(instances[j].bbox))
                continue;

            // Convex-hull bounding boxes are useful evidence, but they are too coarse
            // to be treated as real model collisions for repair verification.
            snapshot["has_bbox_overlap"] = true;
            snapshot["bbox_overlap_pairs"].push_back({
                {"a", {
                    {"object_id", instances[i].obj_id},
                    {"instance_id", instances[i].instance_id},
                    {"name", instances[i].name}
                }},
                {"b", {
                    {"object_id", instances[j].obj_id},
                    {"instance_id", instances[j].instance_id},
                    {"name", instances[j].name}
                }}
            });
        }
    }

    return snapshot;
}

json BuildStructuredSceneIssuesFromPlateValidation(const json& validation)
{
    json issues = json::array();
    if (!validation.is_object() || !validation.value("available", false))
        return issues;

    const int plate_index = validation.value("plate_index", -1);
    const json outside_instances = validation.value("outside_instances", json::array());
    if (outside_instances.is_array()) {
        for (const auto& item : outside_instances) {
            if (!item.is_object())
                continue;

            const int object_id = JsonIntValue(item, "object_id");
            const int instance_id = JsonIntValue(item, "instance_id");
            if (object_id < 0)
                continue;

            const std::string issue_type = OutsideIssueType(item);
            const std::string title = OutsideIssueTitle(issue_type);
            const json object_ref = BuildObjectRef(item);
            json hint_args = {
                {"object_id", object_id},
                {"object_index", JsonIntValue(item, "object_index", object_id)},
                {"instance_id", instance_id},
                {"auto_fix_bounds", true}
            };
            if (item.value("fully_outside_plate", false))
                hint_args["fully_outside_plate"] = true;

            issues.push_back({
                {"issue_id", issue_type + "_obj_" + std::to_string(object_id) + "_inst_" + std::to_string(instance_id)},
                {"issue_type", issue_type},
                {"severity", "error"},
                {"source", "part_plate"},
                {"blocking", true},
                {"plate_index", plate_index},
                {"objects", json::array({object_ref})},
                {"related_objects", json::array()},
                {"geometry", {
                    {"bbox_world", {
                        {"min", JsonArrayValue(item, "bbox_min")},
                        {"max", JsonArrayValue(item, "bbox_max")}
                    }},
                    {"plate_bbox", {
                        {"min", JsonArrayValue(item, "build_volume_min")},
                        {"max", JsonArrayValue(item, "build_volume_max")}
                    }},
                    {"out_of_bounds_axes", OutOfBoundsAxes(item)},
                    {"outside_x", item.value("outside_x", false)},
                    {"outside_y", item.value("outside_y", false)},
                    {"exceeds_height", item.value("exceeds_height", false)},
                    {"fully_outside_plate", item.value("fully_outside_plate", false)},
                    {"larger_than_plate", item.value("larger_than_plate", false)}
                }},
                {"params", json::object()},
                {"message_key", issue_type},
                {"message_display", title},
                {"title", title},
                {"description", title},
                {"candidate_tools", OutsideCandidateTools(item)},
                {"repair_hints", OutsideRepairHints(item, hint_args)},
                {"evidence", {
                    {"source", "part_plate"},
                    {"object_id", object_id},
                    {"object_index", JsonIntValue(item, "object_index", object_id)},
                    {"object_name", item.value("object_name", item.value("name", std::string()))},
                    {"instance_id", instance_id},
                    {"plate_index", plate_index},
                    {"geometry", {
                        {"outside_x", item.value("outside_x", false)},
                        {"outside_y", item.value("outside_y", false)},
                        {"exceeds_height", item.value("exceeds_height", false)},
                        {"fully_outside_plate", item.value("fully_outside_plate", false)},
                        {"larger_than_plate", item.value("larger_than_plate", false)}
                    }}
                }}
            });
        }
    }

    const json overlap_pairs = validation.value("overlap_pairs", json::array());
    if (overlap_pairs.is_array()) {
        for (size_t i = 0; i < overlap_pairs.size(); ++i) {
            const auto& pair = overlap_pairs[i];
            if (!pair.is_object())
                continue;

            const json a = pair.value("a", json::object());
            const json b = pair.value("b", json::object());
            const int a_object_id = JsonIntValue(a, "object_id");
            const int b_object_id = JsonIntValue(b, "object_id");
            if (a_object_id < 0 || b_object_id < 0)
                continue;

            const std::string title = "Models overlap on the current plate.";
            issues.push_back({
                {"issue_id", "model_collision_pair_" + std::to_string(a_object_id) + "_" + std::to_string(b_object_id) + "_" + std::to_string(i)},
                {"issue_type", "model_collision"},
                {"severity", "error"},
                {"source", "part_plate"},
                {"blocking", true},
                {"plate_index", plate_index},
                {"objects", json::array({BuildObjectRef(b)})},
                {"related_objects", json::array({BuildObjectRef(a)})},
                {"geometry", { {"overlap_pair", pair} }},
                {"params", json::object()},
                {"message_key", "model_collision"},
                {"message_display", title},
                {"title", title},
                {"description", title},
                {"candidate_tools", json::array({"move_object", "auto_arrange"})},
                {"repair_hints", json::array({{
                    {"tool", "move_object"},
                    {"confidence", 0.75},
                    {"args_hint", {
                        {"object_id", b_object_id},
                        {"object_index", JsonIntValue(b, "object_index", b_object_id)},
                        {"instance_id", JsonIntValue(b, "instance_id")},
                        {"auto_fix_clearance", true}
                    }}
                }})},
                {"evidence", {
                    {"source", "part_plate"},
                    {"plate_index", plate_index},
                    {"object_id", b_object_id},
                    {"object_index", JsonIntValue(b, "object_index", b_object_id)},
                    {"object_name", b.value("object_name", b.value("name", std::string()))},
                    {"reference_object_id", a_object_id},
                    {"reference_object_index", JsonIntValue(a, "object_index", a_object_id)},
                    {"reference_object_name", a.value("object_name", a.value("name", std::string()))},
                    {"current_plate_validation", {
                        {"available", true},
                        {"has_model_overlap", true},
                        {"overlap_pairs", json::array({pair})},
                        {"plate_index", plate_index}
                    }}
                }}
            });
        }
    }

    if (issues.empty() && validation.value("model_fits", true) == false)
        issues.push_back(BuildUnlocatedModelFitIssue(validation));

    return issues;
}
void AttachCurrentPlateValidationResult(json& result, Plater* plater, bool model_fits, bool validate_error, const json& params)
{
    json validation = BuildCurrentPlateValidationSnapshot(plater, model_fits, validate_error);
    result["current_plate_validation"] = validation;

    const std::string effect_scope = params.is_object() ? params.value("_effect_scope", std::string()) : std::string();
    if (effect_scope != "precheck_repair")
        return;

    const bool available = validation.value("available", false);
    const bool has_overlap = validation.value("has_model_overlap", false);
    const bool has_outside = validation.value("has_model_outside", false);
    const bool resolved = available && !has_overlap && !has_outside;

    std::string status = "unknown";
    if (!available)
        status = "unknown";
    else if (has_overlap && has_outside)
        status = "unresolved_overlap_and_outside";
    else if (has_overlap)
        status = "unresolved_overlap";
    else if (has_outside)
        status = "unresolved_outside";
    else
        status = "resolved";

    result["repair_verification"] = {
        {"schema_version", "1.0"},
        {"source", "part_plate"},
        {"available", available},
        {"resolved", resolved},
        {"status", status},
        {"issue_id", params.is_object() ? params.value("issue_id", std::string()) : std::string()},
        {"issue_type", params.is_object() ? params.value("issue_type", std::string()) : std::string()},
        {"evidence", validation}
    };
}

} // namespace Bridge
} // namespace GUI
} // namespace Slic3r
