#include <catch2/catch.hpp>

#include "libslic3r/BoundingBox.hpp"
#include "libslic3r/ExtrusionEntity.hpp"
#include "libslic3r/ZaaPathGeometry.hpp"
#include "libslic3r/GCodeWriter.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/Layer.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/Preset.hpp"
#include "libslic3r/Format/bbs_3mf.hpp"
#include "libslic3r/Utils.hpp"

#include <boost/filesystem.hpp>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <chrono>
#include <iostream>
#include <limits>

using namespace Slic3r;

namespace {
constexpr double width = 0.42;
constexpr double height = 0.2;
constexpr double nominal_z = 1.2;
const double bead_area = height * (width - height * (1.0 - PI / 4.0));
const double e_per_mm3 = 0.95 / (PI * 1.75 * 1.75 / 4.0);

ZaaMaterialMotionPlanResult motion_plan(const std::vector<Vec3d> &input, bool no_extrusion = false)
{
    Polyline3 polyline;
    std::vector<Vec3d> emitted;
    for (const auto &p : input) {
        // Only the point count is consumed by the planner; use finite fixture geometry
        // so the nonfinite-input tests never convert NaN to an integer.
        polyline.points.emplace_back(scale_(double(polyline.points.size())), 0, scale_(nominal_z));
        emitted.emplace_back(GCodeFormatter::quantize_xyzf(p.x()),
            GCodeFormatter::quantize_xyzf(p.y()), GCodeFormatter::quantize_xyzf(p.z()));
    }
    ZaaLayerGeometry layer;
    layer.query_lower_z_mm = 1.0;
    layer.query_upper_z_mm = nominal_z;
    return make_zaa_material_motion_plan(ExtrusionPath3(std::move(polyline)), layer, nominal_z,
        input, emitted, {width, height, false}, bead_area, e_per_mm3, no_extrusion);
}
}

TEST_CASE("ZAA accepts the reported sub-resolution top-fill path", "[ZAA][zaa-material]")
{
    const auto result = motion_plan({{143.154870, 245.431360, 1.098030},
                                    {143.154880, 245.430960, 1.098030}});
    INFO((result.failure ? zaa_invariant_code(*result.failure) : std::string()));
    // This must become an explicit skip, not a fatal invariant failure.
    REQUIRE_FALSE(result.failure.has_value());
    REQUIRE_FALSE(result.plan.has_value());
}

TEST_CASE("ZAA coalesces collapsed segments without losing material", "[ZAA][zaa-material]")
{
    std::vector<double> x;
    SECTION("leading") { x = {0.0001, 0.0002, 1.0}; }
    SECTION("interior") { x = {0.0, 0.9998, 1.0001, 2.0}; }
    SECTION("trailing") { x = {0.0, 0.9998, 1.0001}; }
    SECTION("closed path retains intermediate motion") { x = {0.0, 1.0, 0.0}; }
    SECTION("short segment across rounding boundary remains printable") { x = {0.00049, 0.00051}; }
    std::vector<Vec3d> input;
    double length = 0;
    for (size_t i = 0; i < x.size(); ++i) {
        input.emplace_back(x[i], 0.0, nominal_z);
        if (i) length += std::abs(x[i] - x[i - 1]);
    }
    const auto result = motion_plan(input);
    REQUIRE_FALSE(result.failure.has_value());
    REQUIRE(result.plan.has_value());
    double volume = 0, extrusion = 0;
    for (const auto &segment : result.plan->segments) {
        CHECK(segment.length_3d_emitted_mm > 0);
        CHECK(std::isfinite(segment.q3d_mm2));
        volume += segment.volume_mm3;
        extrusion += segment.dE;
    }
    CHECK(volume == Approx(length * bead_area).epsilon(1e-8));
    CHECK(extrusion == Approx(length * bead_area * e_per_mm3).epsilon(1e-8));
}

TEST_CASE("ZAA does not hide excessive collapsed travel or invalid data", "[ZAA][zaa-material]")
{
    std::vector<Vec3d> input;
    SECTION("many backtracks in one output cell") {
        for (int i = 0; i < 20; ++i)
            input.emplace_back(i % 2 ? 0.0004 : -0.0004, 0.0, nominal_z);
    }
    SECTION("nonfinite coordinate") {
        input = {{0.0, 0.0, nominal_z}, {std::numeric_limits<double>::quiet_NaN(), 0.0, nominal_z}};
    }
    SECTION("finite short path with excessive material thickness") {
        input = {{-0.00049, -0.00049, 1.4}, {0.00049, 0.00049, 1.4}};
    }
    const auto result = motion_plan(input);
    REQUIRE(result.failure.has_value());
    REQUIRE_FALSE(result.plan.has_value());
}

TEST_CASE("ZAA supplied Hilbert project completes export", "[.][zaa-collapse-project]")
{
    const char *input = std::getenv("ZAA_COLLAPSE_3MF");
    const char *output = std::getenv("ZAA_COLLAPSE_GCODE");
    REQUIRE(input != nullptr);
    REQUIRE(output != nullptr);
    if (const char *resources = std::getenv("ZAA_COLLAPSE_RESOURCES"))
        set_resources_dir(resources);
    set_logging_level(3);
    auto config = DynamicPrintConfig::full_print_config();
    ConfigSubstitutionContext substitutions(ForwardCompatibilitySubstitutionRule::Enable);
    En3mfType type;
    PlateDataPtrs plates;
    std::vector<Preset *> presets;
    Semver version;
    const auto start = std::chrono::steady_clock::now();
    Model model = Model::read_from_archive(input, &config, &substitutions, type,
        LoadStrategy::LoadModel | LoadStrategy::LoadConfig | LoadStrategy::AddDefaultInstances,
        &plates, &presets, &version);
    release_PlateData_list(plates);
    for (auto *preset : presets) delete preset;
    REQUIRE(config.opt_bool("zaa_enabled"));
    REQUIRE(config.option("top_surface_pattern")->serialize() == "hilbertcurve");
    Print print;
    print.set_status_callback([](const PrintBase::SlicingStatus &status) {
        std::cout << "ZAA_COLLAPSE_PROGRESS " << status.percent << " " << status.text << std::endl;
    });
    print.apply(model, config);
    print.process();
    const auto sliced = std::chrono::steady_clock::now();
    REQUIRE_FALSE(print.objects().empty());
    size_t layers = 0;
    for (const PrintObject *object : print.objects()) {
        REQUIRE(object->zaa_slice_decision().is_supported());
        REQUIRE(object->is_step_done(posZaaPathPlan));
        layers += object->layers().size();
    }
    GCodeProcessorResult result;
    print.export_gcode(output, &result);
    const auto done = std::chrono::steady_clock::now();
    std::cout << "ZAA_COLLAPSE_RESULT layers=" << layers
              << " slice_s=" << std::chrono::duration<double>(sliced - start).count()
              << " export_s=" << std::chrono::duration<double>(done - sliced).count() << std::endl;
}

TEST_CASE("ZAA records skipped material and handles zero extrusion", "[ZAA][zaa-material]")
{
    const bool no_extrusion = GENERATE(false, true);
    const std::vector<Vec3d> input{{0.0001, 0.0, nominal_z},
                                 {0.0002, 0.0, nominal_z},
                                 {0.0003, 0.0, nominal_z}};
    const auto result = motion_plan(input, no_extrusion);
    REQUIRE_FALSE(result.failure.has_value());
    REQUIRE_FALSE(result.plan.has_value());
    REQUIRE(result.skipped_micro_path.has_value());
    CHECK(result.skipped_micro_path->length_xy_mm == Approx(0.0002));
    CHECK(result.skipped_micro_path->length_3d_mm == Approx(0.0002));
    CHECK(result.skipped_micro_path->volume_mm3 == Approx(no_extrusion ? 0.0 : 0.0002 * bead_area));
    CHECK(result.skipped_micro_path->dE == Approx(no_extrusion ? 0.0 : 0.0002 * bead_area * e_per_mm3));
}

TEST_CASE("ZAA checks intermediate XYZ and accumulated vertical travel", "[ZAA][zaa-material]")
{
    SECTION("equal endpoints with an intermediate different Z are not skipped") {
        const auto result = motion_plan({{0.0001, 0.0, 1.1}, {0.0002, 0.0, 1.101}, {0.0001, 0.0, 1.1}});
        REQUIRE_FALSE(result.failure.has_value());
        REQUIRE_FALSE(result.skipped_micro_path.has_value());
        REQUIRE(result.plan.has_value());
        REQUIRE(result.plan->segments.size() == 2);
    }
    SECTION("vertical backtracking within a cell exceeds the micro-path budget") {
        std::vector<Vec3d> input;
        for (int i = 0; i < 20; ++i)
            input.emplace_back(i * 0.000001, 0.0, i % 2 ? 1.1004 : 1.0996);
        const auto result = motion_plan(input);
        REQUIRE(result.failure.has_value());
        CHECK(result.failure->reason == ZaaInvariantReason::EmittedSegmentCollapsed);
        CHECK_FALSE(result.skipped_micro_path.has_value());
    }
}


TEST_CASE("Skipping a micro-path preserves subsequent G-code and wipe state", "[ZAA][zaa-material-state]")
{
    const bool at_end = GENERATE(false, true);
    const bool multipath = GENERATE(false, true);
    auto config = DynamicPrintConfig::full_print_config();
    config.set("zaa_enabled", true);
    config.set("layer_height", 0.2);
    config.set("initial_layer_print_height", 0.2);
    config.set("wall_filament", 1);
    config.set("sparse_infill_filament", 1);
    config.set("solid_infill_filament", 1);
    config.set_key_value("wipe", new ConfigOptionBools{true});
    Model model;
    auto *object = model.add_object();
    object->name = "micro-path-state.stl";
    object->add_volume(make_cube(20, 20, 1));
    object->add_instance();
    object->ensure_on_bed();
    Print print;
    print.auto_assign_extruders(object);
    print.apply(model, config);
    print.set_status_silent();
    print.process();
    auto *layer = print.objects().front()->layers().at(1);
    auto &fills = layer->regions().front()->fills;
    fills.clear();
    auto *ordered = new ExtrusionEntityCollection;
    ordered->no_sort = true;
    fills.entities.push_back(ordered);
    const auto make_path = [&](double x0, double x1, double y) {
        auto path = std::make_unique<ExtrusionPath>(erTopSolidInfill, bead_area, float(width), float(height));
        Polyline3 spatial;
        spatial.points.emplace_back(scale_(x0), scale_(y), scale_(layer->print_z - 0.05));
        spatial.points.emplace_back(scale_(x1), scale_(y), scale_(layer->print_z - 0.05));
        path->polyline = spatial.to_polyline();
        path->set_path3(std::make_unique<ExtrusionPath3>(std::move(spatial)));
        path->set_zaa_path_policy(ZaaPathPolicy::ZaaLinearCandidate);
        return path;
    };
    ordered->entities.push_back(make_path(2, 3, 2).release());
    ordered->entities.push_back(make_path(4, 5, 2).release());
    const auto export_commands = [&]() {
        const auto output = boost::filesystem::temp_directory_path() /
            boost::filesystem::unique_path("zaa-micro-state-%%%%-%%%%.gcode");
        GCodeProcessorResult result;
        print.export_gcode(output.string(), &result);
        std::ifstream file(output.string());
        REQUIRE(file.good());
        std::string line, commands;
        size_t spatial_top_paths = 0;
        while (std::getline(file, line)) {
            if (line.find("; ZAA_BEGIN") == 0 && line.find("role=TopSolidInfill") != std::string::npos)
                ++spatial_top_paths;
            line = line.substr(0, line.find(';'));
            const auto last = line.find_last_not_of(" \t\r");
            if (last != std::string::npos)
                commands += line.substr(0, last + 1) + '\n';
        }
        file.close();
        boost::filesystem::remove(output);
        REQUIRE(spatial_top_paths >= 2);
        return commands;
    };
    const auto baseline = export_commands();
    auto micro = make_path(15.0001, 15.0002, 15);
    ExtrusionEntity *entity = multipath ? static_cast<ExtrusionEntity *>(new ExtrusionMultiPath(*micro)) : micro.release();
    ordered->entities.insert(ordered->entities.begin() + (at_end ? 2 : 1), entity);
    // A discarded path must cause neither travel to (15,15), unretraction,
    // extrusion, nor a later wipe over geometry that was never printed.
    CHECK(export_commands() == baseline);
}

TEST_CASE("ZAA geometry cleanup distinguishes motion from routing and invalid data", "[ZAA][zaa-geometry]")
{
    const auto policy = GENERATE(ZaaPathPolicy::Conventional, ZaaPathPolicy::ZaaLinearCandidate);
    ExtrusionPath path(erTopSolidInfill, bead_area, float(width), float(height));
    path.set_zaa_path_policy(policy);
    const Point a(scale_(2.), scale_(2.));
    const Point b = a + Point(1, 0);
    SECTION("empty, singleton and repeated points have no motion") {
        for (const Points &points : {Points{}, Points{a}, Points{a, a, a}}) {
            path.polyline.points = points;
            CHECK(zaa_path_geometry(path) == ZaaPathGeometry::NoMotion);
            CHECK_FALSE(zaa_keep_path(path));
            zaa_simplify_path(path, scale_(0.01));
            CHECK_FALSE(zaa_keep_path(path));
        }
    }
    SECTION("even one coordinate unit of motion survives") {
        for (const Points &points : {Points{a, b}, Points{a, b, a, b, a}}) {
            path.polyline.points = points;
            CHECK(zaa_keep_path(path));
            zaa_simplify_path(path, scale_(0.01));
            CHECK(path.polyline.points == points);
            CHECK(zaa_keep_path(path));
        }
    }
    SECTION("spatial motion checks all XYZ points") {
        Polyline3 spatial;
        spatial.points = {Vec3crd(a.x(), a.y(), 10), Vec3crd(a.x(), a.y(), 11), Vec3crd(a.x(), a.y(), 10)};
        path.set_path3(std::make_unique<ExtrusionPath3>(std::move(spatial)));
        CHECK(zaa_path_geometry(path) == ZaaPathGeometry::HasMotion);
        CHECK(zaa_keep_path(path));
    }
    SECTION("spatial no-motion is removable only when its XY mirror agrees") {
        Polyline3 spatial;
        spatial.points = {Vec3crd(a.x(), a.y(), 10), Vec3crd(a.x(), a.y(), 10)};
        path.set_path3(std::make_unique<ExtrusionPath3>(std::move(spatial)));
        CHECK_FALSE(zaa_keep_path(path));
        path.polyline.points.back() = b;
        CHECK(zaa_path_geometry(path) == ZaaPathGeometry::Invalid);
        CHECK_THROWS_AS(zaa_keep_path(path), LogicError);
        path.polyline.points.pop_back();
        CHECK(zaa_path_geometry(path) == ZaaPathGeometry::Invalid);
        CHECK_THROWS_AS(zaa_keep_path(path), LogicError);
    }
}

TEST_CASE("ZAA simplification removes no-motion entities and preserves collapsed motion", "[ZAA][zaa-geometry]")
{
    const bool enabled = GENERATE(false, true);
    const size_t layer_index = GENERATE(size_t(0), size_t(1));
    const bool cleanup = enabled && layer_index != 0;
    auto config = DynamicPrintConfig::full_print_config();
    config.set("zaa_enabled", enabled);
    config.set("layer_height", height);
    config.set("initial_layer_print_height", height);
    config.set("resolution", 0.01);
    config.set("enable_arc_fitting", false);
    config.set("wall_filament", 1);
    config.set("sparse_infill_filament", 1);
    config.set("solid_infill_filament", 1);
    Model model;
    auto *object = model.add_object();
    object->add_volume(make_cube(20, 20, 1));
    object->add_instance();
    object->ensure_on_bed();
    Print print;
    print.auto_assign_extruders(object);
    print.apply(model, config);
    print.set_status_silent();
    print.process();
    auto *region = print.objects().front()->layers().at(layer_index)->regions().front();
    auto &fills = region->fills;
    fills.clear();
    const Point a(scale_(2.), scale_(2.));
    const Point b = a + Point(20, 0);
    auto make_path = [&](Points points, ExtrusionRole role = erTopSolidInfill) {
        auto *path = new ExtrusionPath(role, bead_area, float(width), float(height));
        path->polyline.points = std::move(points);
        return path;
    };
    auto *ordered = new ExtrusionEntityCollection;
    ordered->no_sort = true;
    fills.entities.push_back(ordered);
    ordered->entities.push_back(make_path({a, a}));
    ordered->entities.push_back(make_path({a, b, a}));
    ordered->entities.push_back(make_path({a, a}, erInternalInfill));
    auto *nested = new ExtrusionEntityCollection;
    auto *multi = new ExtrusionMultiPath;
    multi->paths.emplace_back(*static_cast<ExtrusionPath *>(ordered->entities.front()));
    nested->entities.push_back(multi);
    auto *loop = new ExtrusionLoop;
    loop->paths.emplace_back(multi->paths.front());
    nested->entities.push_back(loop);
    ordered->entities.push_back(nested);
    region->simplify_infill_extrusion_entity();
    REQUIRE(fills.entities.size() == 1);
    REQUIRE(ordered->no_sort);
    REQUIRE(ordered->entities.size() == (cleanup ? 2 : 4));
    const auto *roundtrip = dynamic_cast<const ExtrusionPath *>(ordered->entities[cleanup ? 0 : 1]);
    REQUIRE(roundtrip != nullptr);
    if (cleanup) {
        CHECK(roundtrip->polyline.points == Points{a, b, a});
        CHECK(roundtrip->zaa_path_policy() == ZaaPathPolicy::ZaaLinearCandidate);
        CHECK(ordered->entities.back()->role() == erInternalInfill);
    } else {
        CHECK(roundtrip->polyline.points == Points{a, a});
        CHECK(roundtrip->zaa_path_policy() == ZaaPathPolicy::Conventional);
    }
}

TEST_CASE("ZAA no-motion export has no positioning or wipe side effects", "[ZAA][zaa-geometry]")
{
    const int entity_kind = GENERATE(0, 1, 2);
    const bool spatial = GENERATE(false, true);
    auto config = DynamicPrintConfig::full_print_config();
    config.set("zaa_enabled", true);
    config.set("layer_height", height);
    config.set("initial_layer_print_height", height);
    config.set("wall_filament", 1);
    config.set("sparse_infill_filament", 1);
    config.set("solid_infill_filament", 1);
    config.set_key_value("wipe", new ConfigOptionBools{true});
    Model model;
    auto *object = model.add_object();
    object->add_volume(make_cube(20, 20, 1));
    object->add_instance();
    object->ensure_on_bed();
    Print print;
    print.auto_assign_extruders(object);
    print.apply(model, config);
    print.set_status_silent();
    print.process();
    auto &fills = print.objects().front()->layers().at(1)->regions().front()->fills;
    fills.clear();
    auto *ordered = new ExtrusionEntityCollection;
    ordered->no_sort = true;
    fills.entities.push_back(ordered);
    auto make_path = [](double x0, double x1, double y) {
        auto path = std::make_unique<ExtrusionPath>(erTopSolidInfill, bead_area, float(width), float(height));
        path->polyline.points = {Point(scale_(x0), scale_(y)), Point(scale_(x1), scale_(y))};
        path->set_zaa_path_policy(ZaaPathPolicy::ZaaLinearCandidate);
        return path;
    };
    ordered->entities.push_back(make_path(2, 3, 2).release());
    ordered->entities.push_back(make_path(4, 5, 2).release());
    const auto export_commands = [&]() {
        const auto output = boost::filesystem::temp_directory_path() /
            boost::filesystem::unique_path("zaa-no-motion-%%%%-%%%%.gcode");
        GCodeProcessorResult result;
        print.export_gcode(output.string(), &result);
        std::ifstream file(output.string());
        REQUIRE(file.good());
        std::string commands, line;
        while (std::getline(file, line)) {
            line = line.substr(0, line.find(';'));
            const auto last = line.find_last_not_of(" \t\r");
            if (last != std::string::npos)
                commands += line.substr(0, last + 1) + '\n';
        }
        file.close();
        boost::filesystem::remove(output);
        return commands;
    };
    const auto baseline = export_commands();
    auto no_motion = make_path(15, 15, 15);
    no_motion->set_zaa_path_policy(ZaaPathPolicy::Conventional);
    if (spatial) {
        Polyline3 xyz;
        xyz.points = {Vec3crd(scale_(15.), scale_(15.), scale_(0.35)),
                      Vec3crd(scale_(15.), scale_(15.), scale_(0.35))};
        no_motion->set_path3(std::make_unique<ExtrusionPath3>(std::move(xyz)));
    }
    ExtrusionEntity *entity = nullptr;
    if (entity_kind == 0)
        entity = no_motion.release();
    else if (entity_kind == 1)
        entity = new ExtrusionMultiPath(*no_motion);
    else {
        auto *loop = new ExtrusionLoop;
        loop->paths.push_back(*no_motion);
        entity = loop;
    }
    ordered->entities.insert(ordered->entities.begin() + 1, entity);
    CHECK(export_commands() == baseline);
}
