#include <catch2/catch.hpp>
#include "slic3r/GUI/NoTowerOptions.hpp"
#include "slic3r/GUI/PrimeTowerOptions.hpp"
#include "slic3r/GUI/PartPlate.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/ModelInstance.hpp"
#include "libslic3r/ModelVolume.hpp"
#include "libslic3r/Format/bbs_3mf.hpp"
#include "libslic3r/Format/STL.hpp"
#include <cstdlib>
#include <iostream>
#include "libslic3r/Fill/Tower/FillLayerFilamentWipePacking.hpp"

using namespace Slic3r;
using namespace Slic3r::GUI;

static DynamicPrintConfig no_tower_config()
{
    auto config = DynamicPrintConfig::full_print_config();
    config.set_key_value("enable_prime_tower", new ConfigOptionBool(false));
    config.set_key_value("flush_into_solid_skeleton", new ConfigOptionBool(false));
    config.set_key_value("time_lapse_gcode", new ConfigOptionString(""));
    config.set_key_value("timelapse_type", new ConfigOptionEnum<TimelapseType>(tlTraditional));
    config.set_key_value("filament_diameter", new ConfigOptionFloats{1.75, 1.75});
    config.set_key_value("filament_soluble", new ConfigOptionBools{false, false});
    config.set_key_value("filament_is_support", new ConfigOptionBools{false, false});
    return config;
}

TEST_CASE("Machine-linked no-tower options are visible only for capable machines with the tower disabled", "[PrimeTower][18489]")
{
    REQUIRE(show_machine_linked_no_tower_options(false, true));
    REQUIRE_FALSE(show_machine_linked_no_tower_options(false, false));
    REQUIRE_FALSE(show_machine_linked_no_tower_options(true, true));
    REQUIRE_FALSE(show_machine_linked_no_tower_options(true, false));
}

TEST_CASE("Explicitly disabling the tower applies defaults while editing presets", "[PrimeTower]")
{
    auto config = no_tower_config();
    config.set_key_value("enable_prime_tower", new ConfigOptionBool(true));
    // These are editing contexts, not prerequisites for entering a slicing mode.
    SECTION("default preset before adding models") {}
    SECTION("one material") {
        config.set_key_value("filament_diameter", new ConfigOptionFloats{1.75});
    }
    SECTION("printer contains timelapse gcode") {
        config.set_key_value("time_lapse_gcode", new ConfigOptionString("M400"));
    }
    SECTION("switching away from solid skeleton flushing") {
        config.set_key_value("flush_into_solid_skeleton", new ConfigOptionBool(true));
    }
    REQUIRE(apply_prime_tower_user_change(config, false, true));
    REQUIRE_FALSE(config.opt_bool("enable_prime_tower"));
    REQUIRE(config.opt_float("sparse_infill_density") == 10);
    REQUIRE(config.opt_enum<InfillPattern>("sparse_infill_pattern") == ipLockedZag);
    REQUIRE_FALSE(config.opt_bool("flush_into_support"));
    REQUIRE_FALSE(config.opt_bool("flush_into_solid_skeleton"));
}

TEST_CASE("Disabling the tower preserves process settings when the machine capability is off", "[PrimeTower][18254]")
{
    auto config = no_tower_config();
    config.set_key_value("enable_prime_tower", new ConfigOptionBool(true));
    config.set_key_value("sparse_infill_density", new ConfigOptionPercent(35));
    config.set_key_value("sparse_infill_pattern", new ConfigOptionEnum<InfillPattern>(ipGyroid));
    config.set_key_value("flush_into_support", new ConfigOptionBool(true));
    const auto before = config;

    REQUIRE(apply_prime_tower_user_change(config, false, false));
    REQUIRE_FALSE(config.opt_bool("enable_prime_tower"));
    REQUIRE(config.opt_float("sparse_infill_density") == 35);
    REQUIRE(config.opt_enum<InfillPattern>("sparse_infill_pattern") == ipGyroid);
    REQUIRE(config.opt_bool("flush_into_support"));

    auto expected = before;
    expected.set_key_value("enable_prime_tower", new ConfigOptionBool(false));
    REQUIRE(config.diff(expected).empty());
}

TEST_CASE("Smooth timelapse tower toggles preserve model purge and custom infill", "[PrimeTower][timelapse]")
{
    auto config = no_tower_config();
    config.set_key_value("enable_prime_tower", new ConfigOptionBool(true));
    config.set_key_value("timelapse_type", new ConfigOptionEnum<TimelapseType>(tlSmooth));
    config.set_key_value("sparse_infill_density", new ConfigOptionPercent(35));
    config.set_key_value("sparse_infill_pattern", new ConfigOptionEnum<InfillPattern>(ipGyroid));
    config.set_key_value("flush_into_infill", new ConfigOptionBool(true));
    config.set_key_value("flush_into_objects", new ConfigOptionBool(true));
    config.set_key_value("flush_into_support", new ConfigOptionBool(true));
    config.set_key_value("purge_in_prime_tower", new ConfigOptionBool(true));
    const auto before = config;
    auto expected = before;
    expected.set_key_value("enable_prime_tower", new ConfigOptionBool(false));
    REQUIRE(apply_prime_tower_user_change(config, false, true));
    REQUIRE(config.diff(expected).empty());
    LinkedOptionBackup backup;
    backup.capture(before, config);
    config.set_key_value("flush_into_support", new ConfigOptionBool(false));
    REQUIRE(apply_prime_tower_user_change(config, true, true));
    backup.restore(config);
    expected = before;
    expected.set_key_value("flush_into_support", new ConfigOptionBool(false));
    REQUIRE(config.diff(expected).empty());
}

TEST_CASE("Smooth timelapse linked defaults retain explicit model purge choices", "[PrimeTower][timelapse]")
{
    auto config = no_tower_config();
    config.set_key_value("timelapse_type", new ConfigOptionEnum<TimelapseType>(tlSmooth));
    const bool flush = GENERATE(false, true);
    config.set_key_value("flush_into_infill", new ConfigOptionBool(flush));
    config.set_key_value("flush_into_objects", new ConfigOptionBool(flush));
    config.set_key_value("flush_into_support", new ConfigOptionBool(flush));
    config.set_key_value("purge_in_prime_tower", new ConfigOptionBool(true));
    apply_no_wipe_tower_no_purge_linked_options(config);
    REQUIRE(config.opt_bool("flush_into_infill") == flush);
    REQUIRE(config.opt_bool("flush_into_objects") == flush);
    REQUIRE(config.opt_bool("flush_into_support") == flush);
    REQUIRE(config.opt_bool("purge_in_prime_tower"));
}

TEST_CASE("An unchanged tower value does not apply edit defaults", "[PrimeTower]")
{
    auto config = no_tower_config();
    const bool enabled = GENERATE(false, true);
    config.set_key_value("enable_prime_tower", new ConfigOptionBool(enabled));
    const auto original = config;
    REQUIRE_FALSE(apply_prime_tower_user_change(config, enabled, true));
    REQUIRE(config.diff(original).empty());
}

TEST_CASE("Enabling the tower preserves settings when there is no session backup", "[PrimeTower]")
{
    auto config = no_tower_config();
    auto expected = config;
    expected.set_key_value("enable_prime_tower", new ConfigOptionBool(true));
    REQUIRE(apply_prime_tower_user_change(config, true, true));
    REQUIRE(config.diff(expected).empty());
}

TEST_CASE("Re-enabling linked no-tower editing restores the saved process", "[PrimeTower]")
{
    auto config = no_tower_config();
    config.set_key_value("layer_height", new ConfigOptionFloat(0.12));
    auto saved = config;
    saved.set_key_value("layer_height", new ConfigOptionFloat(0.2));
    bool linked = true;
    bool preserves_edits = false;
    SECTION("ordinary linked mode restores the saved preset") {}
    SECTION("machine capability off preserves edits") {
        linked = false;
        preserves_edits = true;
    }
    SECTION("smooth timelapse preserves edits") {
        config.set_key_value("timelapse_type", new ConfigOptionEnum<TimelapseType>(tlSmooth));
        preserves_edits = true;
    }
    auto expected = preserves_edits ? config : saved;
    expected.set_key_value("enable_prime_tower", new ConfigOptionBool(true));
    REQUIRE(apply_prime_tower_user_change(config, true, linked, &saved));
    REQUIRE(config.diff(expected).empty());
}

TEST_CASE("Switching back restores unsaved values without overwriting later edits", "[PrimeTower]")
{
    auto before = no_tower_config();
    before.set_key_value("enable_prime_tower", new ConfigOptionBool(true));
    before.set_key_value("sparse_infill_density", new ConfigOptionPercent(35));
    before.set_key_value("sparse_infill_pattern", new ConfigOptionEnum<InfillPattern>(ipGyroid));
    before.set_key_value("infill_wall_overlap", new ConfigOptionPercent(23));
    auto active = before;
    REQUIRE(apply_prime_tower_user_change(active, false, true));
    LinkedOptionBackup backup;
    backup.capture(before, active);

    SECTION("unmodified linked values return to their previous values") {
        REQUIRE(active.opt_float("sparse_infill_density") == 10);
        REQUIRE(active.opt_enum<InfillPattern>("sparse_infill_pattern") == ipLockedZag);
        REQUIRE_FALSE(active.opt_bool("flush_into_support"));
        REQUIRE(apply_prime_tower_user_change(active, true, true));
        backup.restore(active);
        REQUIRE(active.diff(before).empty());
    }
    SECTION("later edits survive") {
        active.set_key_value("infill_wall_overlap", new ConfigOptionPercent(31));
        active.set_key_value("layer_height", new ConfigOptionFloat(0.12));
        backup.restore(active);
        REQUIRE(active.opt_float("sparse_infill_density") == 35);
        REQUIRE(active.opt_enum<InfillPattern>("sparse_infill_pattern") == ipGyroid);
        REQUIRE(active.opt_float("infill_wall_overlap") == 31);
        REQUIRE(active.opt_float("layer_height") == 0.12);
    }
    SECTION("loading another preset invalidates the session backup") {
        backup.clear();
        const auto loaded = active;
        backup.restore(active);
        REQUIRE(active.diff(loaded).empty());
    }
    SECTION("restoring consumes the backup") {
        backup.restore(active);
        apply_no_wipe_tower_no_purge_linked_options(active);
        const auto changed = active;
        backup.restore(active);
        REQUIRE(active.diff(changed).empty());
    }
}

TEST_CASE("Toggling the tower preserves the purge-in-tower preference", "[PrimeTower][18311]")
{
    auto config = DynamicPrintConfig::full_print_config();
    const bool purge_in_tower = GENERATE(false, true);
    config.set_key_value("enable_prime_tower", new ConfigOptionBool(true));
    config.set_key_value("purge_in_prime_tower", new ConfigOptionBool(purge_in_tower));
    const auto before = config;
    REQUIRE(apply_prime_tower_user_change(config, false, true));
    REQUIRE(config.opt_bool("purge_in_prime_tower") == purge_in_tower);
    LinkedOptionBackup backup;
    backup.capture(before, config);
    SECTION("re-enabling retains the original preference") {
        REQUIRE(apply_prime_tower_user_change(config, true, true));
        backup.restore(config);
        REQUIRE(config.opt_bool("purge_in_prime_tower") == purge_in_tower);
    }
    SECTION("an edit while the tower is disabled survives re-enabling") {
        config.set_key_value("purge_in_prime_tower", new ConfigOptionBool(!purge_in_tower));
        REQUIRE(apply_prime_tower_user_change(config, true, true));
        backup.restore(config);
        REQUIRE(config.opt_bool("purge_in_prime_tower") == !purge_in_tower);
    }
}

TEST_CASE("A disabled tower cannot consume pending skeleton flush transitions", "[PrimeTower][18311]")
{
    const bool tower_enabled = GENERATE(false, true);
    auto config = DynamicPrintConfig::full_print_config();
    config.set_key_value("enable_prime_tower", new ConfigOptionBool(tower_enabled));
    config.set_key_value("purge_in_prime_tower", new ConfigOptionBool(false));
    config.set_key_value("filament_diameter", new ConfigOptionFloats{1.75, 1.75});
    config.set_key_value("flush_volumes_matrix", new ConfigOptionFloats{0., 100., 120., 0.});
    config.set_key_value("flush_multiplier", new ConfigOptionFloat(1.));
    config.set_key_value("prime_volume", new ConfigOptionFloat(10.));
    config.set_key_value("filament_minimal_purge_on_wipe_tower", new ConfigOptionFloats{0., 0.});
    Print print;
    Model model;
    print.apply(model, config);
    ToolOrdering ordering;
    // No available skeleton capacity: both transitions must be retained for external flushing.
    ordering.layer_tools().emplace_back(0.4);
    ordering.layer_tools().back().extruders = {0, 1};
    ordering.layer_tools().emplace_back(0.6);
    ordering.layer_tools().back().extruders = {0};
    const auto baseline = FillTower::plan_layer_filament_wipe_packing(print, ordering, {}, true, false);
    REQUIRE(baseline.external_flush_transitions.size() == 2);

    config.set_key_value("purge_in_prime_tower", new ConfigOptionBool(true));
    print.apply(model, config);
    const auto enabled = FillTower::plan_layer_filament_wipe_packing(print, ordering, {}, true, false);
    REQUIRE(print.config().purge_in_prime_tower.value);
    if (tower_enabled)
        REQUIRE(enabled.external_flush_transitions.empty());
    else
        REQUIRE(enabled.external_flush_transitions == baseline.external_flush_transitions);
}

TEST_CASE("A saved prime tower must fit on its own plate before slicing", "[PrimeTower][18773]")
{
    auto config = no_tower_config();
    config.set_deserialize_strict({{"enable_prime_tower", "1"}, {"wipe_tower_rotation_angle", "42"},
                                   {"printable_area", "0x0,260x0,260x260,0x260"},
                                   {"wipe_tower_x", "100,672.01"}, {"wipe_tower_y", "100,-88.5369"}});
    Model model;
    ModelObject* object = model.add_object();
    for (int tool : {1, 2}) {
        TriangleMesh mesh = make_cube(5, 5, 5);
        mesh.translate(float(tool * 5), 0.f, 0.f);
        object->add_volume(std::move(mesh))->config.set("extruder", tool);
    }
    object->add_instance()->set_offset(Vec3d(350., 40., 0.));

    Print print;
    print.set_plate_index(1);
    print.set_plate_origin(Vec3d(300., 0., 0.));
    SECTION("Entire tower outside") {}
    SECTION("Anchor inside but the rotated tower crosses the edge") {
        config.set_deserialize_strict({{"wipe_tower_x", "100,259"}, {"wipe_tower_y", "100,100"}});
    }
    SECTION("Single object sequential printing") {
        config.set_deserialize_strict({{"print_sequence", "by object"}});
    }
    print.apply(model, config);
    REQUIRE(print.validate().type == STRING_EXCEPT_PRIME_TOWER_OUTSIDE_PLATE);
    std::shared_ptr<const WipeTowerPreview> ready_preview;
    auto lookup = [&](WipeTowerPreviewInput input) {
        auto current = ready_preview;
        if (!current)
            ready_preview = std::make_shared<WipeTowerPreview>(input.estimate());
        return current;
    };
    auto validate = [&] { return print.try_validate(nullptr, nullptr, nullptr, false, lookup); };
    REQUIRE_FALSE(validate().has_value());
    REQUIRE(validate().value().type == STRING_EXCEPT_PRIME_TOWER_OUTSIDE_PLATE);

    config.set_deserialize_strict({{"wipe_tower_x", "100,130"}, {"wipe_tower_y", "100,100"}});
    print.apply(model, config);
    REQUIRE(print.validate().type != STRING_EXCEPT_PRIME_TOWER_OUTSIDE_PLATE);
    REQUIRE(validate().value().type != STRING_EXCEPT_PRIME_TOWER_OUTSIDE_PLATE);

    config.set_deserialize_strict({{"enable_prime_tower", "0"}, {"wipe_tower_x", "100,672.01"}});
    print.apply(model, config);
    REQUIRE(print.validate().type != STRING_EXCEPT_PRIME_TOWER_OUTSIDE_PLATE);
}

static PrintConfig tower_preview_config()
{
    PrintConfig config;
    config.prime_volume.value = 200.;
    config.filament_minimal_purge_on_wipe_tower.values = {20., 30.};
    config.purge_in_prime_tower.value = false;
    config.prime_tower_width.value = 50.;
    config.prime_tower_rib_wall.value = false;
    config.prime_tower_enhance_type.value = PrimeTowerEnhanceType::pteChamfer;
    config.prime_tower_brim_width.value = 3.;
    config.wipe_tower_extra_spacing.value = 100.;
    config.nozzle_diameter.values = {0.4};
    config.filament_diameter.values = {1.75, 1.75};
    config.timelapse_type.value = tlTraditional;
    return config;
}

TEST_CASE("Tower preview order follows flush matrix and sequence edits", "[PrimeTower][TowerPreview][18799]")
{
    auto config = tower_preview_config();
    config.enable_prime_tower.value = true;
    config.purge_in_prime_tower.value = true;
    config.filament_diameter.values = {1.75, 1.75, 1.75};
    config.flush_volumes_matrix.values = {0., 1., 9., 9., 0., 1., 1., 9., 0.};
    const std::vector<unsigned int> tools{0, 1, 2};
    auto plan = [&] { return ToolOrdering::preview_extruder_order(config, tools, 1).front(); };
    REQUIRE(plan() == tools);
    config.wipe_tower_x.values = {150.};
    REQUIRE(plan() == tools);

    const std::vector<unsigned int> reversed_tail{0, 2, 1};
    config.flush_volumes_matrix.values = {0., 9., 1., 1., 0., 9., 9., 1., 0.};
    REQUIRE(plan() == reversed_tail);
    config.first_layer_print_sequence.values = {2, 1, 3};
    const std::vector<unsigned int> custom{1, 0, 2};
    REQUIRE(plan() == custom);
}

TEST_CASE("Tower preview responds to actual layer height and purge line spacing", "[PrimeTower][TowerPreview]")
{
    auto config = tower_preview_config();
    const auto baseline = Print::estimate_wipe_tower_preview(config, 2, 0.2, 20.);
    const auto thin = Print::estimate_wipe_tower_preview(config, 2, 0.1, 20.);
    REQUIRE(thin.size.x() == Approx(baseline.size.x()));
    REQUIRE(thin.size.y() > baseline.size.y());
    config.wipe_tower_extra_spacing.value = 200.;
    REQUIRE(Print::estimate_wipe_tower_preview(config, 2, 0.2, 20.).size.y() > baseline.size.y());
    config.prime_tower_rib_wall.value = true;
    const auto square = Print::estimate_wipe_tower_preview(config, 2, 0.2, 20.);
    // The real planner rounds purge rows; the nominal square mode need not
    // produce an exactly square outer envelope. Its width must be replanned.
    REQUIRE(square.size.x() != Approx(config.prime_tower_width.value));
    config.wipe_tower_extra_spacing.value = 100.;
    const auto compact = Print::estimate_wipe_tower_preview(config, 2, 0.2, 20.);
    REQUIRE(square.size.x() > compact.size.x());
    REQUIRE(square.size.y() > compact.size.y());
}

TEST_CASE("Square tower preview counts pre-flush row spacing once", "[PrimeTower][TowerPreview]")
{
    auto config = tower_preview_config();
    config.prime_volume.value = 300.;
    config.prime_tower_enhance_type.value = PrimeTowerEnhanceType::pteCornerRib;
    config.filament_pre_flush_volume.values = {20., 20.};
    config.wipe_tower_extra_spacing.value = 200.;
    const auto preview = Print::estimate_wipe_tower_preview(config, 2, 0.2, 20.);
    REQUIRE_FALSE(preview.tower_mesh.empty());
    // Row rounding can move the wall by a few extrusion widths.
    REQUIRE(preview.size.x() == Approx(preview.size.y()).margin(2.));
}

TEST_CASE("Tower preview includes mandatory purge and every nozzle matrix", "[PrimeTower][TowerPreview]")
{
    auto config = tower_preview_config();
    const auto baseline = Print::estimate_wipe_tower_preview(config, 2, 0.2, 20.);
    config.filament_minimal_purge_on_wipe_tower.values = {20., 600.};
    REQUIRE(Print::estimate_wipe_tower_preview(config, 2, 0.2, 20.).size.y() > baseline.size.y());
    config.filament_minimal_purge_on_wipe_tower.values = {20., 30.};
    config.nozzle_diameter.values = {0.4, 0.6};
    config.filament_map.values = {1, 2};
    const auto dual = Print::estimate_wipe_tower_preview(config, 2, 0.2, 20.);
    REQUIRE((dual.size - baseline.size).norm() > EPSILON);
    config.purge_in_prime_tower.value = true;
    config.flush_multiplier.value = 1.;
    config.flush_volumes_matrix.values = {0., 100., 100., 0., 0., 800., 600., 0.};
    const auto flushing = Print::estimate_wipe_tower_preview(config, 2, 0.2, 20.);
    REQUIRE(flushing.size.y() > dual.size.y());
    config.flush_multiplier.value = 2.;
    REQUIRE(Print::estimate_wipe_tower_preview(config, 2, 0.2, 20.).size.y() > flushing.size.y());
}

TEST_CASE("Directional flushing uses optimized transitions instead of repeating the worst pair", "[PrimeTower][TowerPreview]")
{
    auto config = tower_preview_config();
    config.enable_prime_tower.value = true;
    config.prime_volume.value = 1.;
    config.prime_tower_width.value = 40.;
    config.prime_tower_rib_wall.value = true;
    config.prime_tower_enhance_type.value = PrimeTowerEnhanceType::pteCornerRib;
    config.prime_tower_corner_rib_length.value = 10.;
    config.prime_tower_skip_points.value = false;
    config.wipe_tower_extra_spacing.value = 200.;
    config.purge_in_prime_tower.value = true;
    config.flush_multiplier.value = 0.13;
    config.filament_diameter.values = {1.75, 1.75, 1.75, 1.75, 1.75};
    config.filament_adhesiveness_category.values = {100, 100, 100, 100, 100};
    config.filament_minimal_purge_on_wipe_tower.values = {15., 15., 15., 15., 15.};
    config.flush_volumes_matrix.values = {
        0., 242., 270., 88., 270.,
        315., 0., 264., 135., 264.,
        594., 548., 0., 98., 68.,
        568., 669., 416., 0., 416.,
        594., 548., 68., 98., 0.};
    const auto preview = Print::estimate_wipe_tower_preview(config, 4, 0.2, 26., {1, 3, 4, 5});
    REQUIRE_FALSE(preview.tower_mesh.empty());
    auto bounds = preview.tower_mesh.bounding_box();
    bounds.merge(preview.brim_mesh.bounding_box());
    // Measured from a complete slice of four 26 mm cubes, using these colors
    // and purge settings. The previous worst-pair estimate was 78.4 x 58.9 mm.
    REQUIRE(bounds.size().x() == Approx(61.899).margin(0.1));
    REQUIRE(bounds.size().y() == Approx(48.899).margin(0.1));
}

TEST_CASE("Tower preview reserves reinforcement without moving its anchor", "[PrimeTower][TowerPreview]")
{
    auto config = tower_preview_config();
    config.prime_tower_rib_wall.value = true;
    const auto plain = Print::estimate_wipe_tower_preview(config, 2, 0.2, 200.);
    SECTION("corner ribs") {
        config.prime_tower_enhance_type.value = PrimeTowerEnhanceType::pteCornerRib;
        config.prime_tower_corner_rib_length.value = 12.;
    }
    SECTION("stabilizing cone") {
        config.prime_tower_enhance_type.value = PrimeTowerEnhanceType::pteCone;
        config.wipe_tower_cone_angle.value = 60.;
    }
    const auto reinforced = Print::estimate_wipe_tower_preview(config, 2, 0.2, 200.);
    REQUIRE(reinforced.size.y() > plain.size.y());
    REQUIRE(reinforced.offset.y() < 0.);
    REQUIRE(reinforced.offset.x() + reinforced.size.x() / 2. == Approx(plain.size.x() / 2.));
    REQUIRE(reinforced.offset.y() + reinforced.size.y() / 2. == Approx(plain.size.y() / 2.));
    config.prime_tower_brim_width.value = -1.;
    const auto automatic = Print::estimate_wipe_tower_preview(config, 2, 0.2, 200.);
    REQUIRE(automatic.brim_width > 0.);
    REQUIRE(std::abs(automatic.brim_width - WipeTower::get_auto_brim_by_height(200.f)) < 0.5);
}

TEST_CASE("Tower placement rotates around the anchor and includes the brim once", "[PrimeTower][TowerPreview]")
{
    WipeTowerPreview preview;
    preview.size = Vec3d(20., 30., 10.);
    preview.offset = Vec2d(-5., -7.);
    preview.brim_width = 3.;
    const auto bounds = get_extents(preview.footprint(90., Vec2d(100., 200.)));
    REQUIRE(unscaled<double>(bounds.min.x()) == Approx(74.));
    REQUIRE(unscaled<double>(bounds.min.y()) == Approx(192.));
    REQUIRE(unscaled<double>(bounds.max.x()) == Approx(110.));
    REQUIRE(unscaled<double>(bounds.max.y()) == Approx(218.));
    const auto negative = get_extents(preview.footprint(-90., Vec2d(100., 200.)));
    REQUIRE(unscaled<double>(negative.min.x()) == Approx(90.));
    REQUIRE(unscaled<double>(negative.min.y()) == Approx(182.));
    REQUIRE(unscaled<double>(negative.max.x()) == Approx(126.));
    REQUIRE(unscaled<double>(negative.max.y()) == Approx(208.));
}

TEST_CASE("Tower preview handles empty plates single filament and invalid dimensions", "[PrimeTower][TowerPreview]")
{
    auto config = tower_preview_config();
    REQUIRE(Print::estimate_wipe_tower_preview(config, 0, 0.2, 20.).footprint().empty());
    REQUIRE(Print::estimate_wipe_tower_preview(config, 1, 0.2, 20.).footprint().empty());
    REQUIRE(Print::estimate_wipe_tower_preview(config, 2, 0., 20.).footprint().empty());
    REQUIRE(Print::estimate_wipe_tower_preview(config, 2, 0.2, 0.).footprint().empty());
    REQUIRE(Print::estimate_wipe_tower_preview(config, 2, std::numeric_limits<double>::quiet_NaN(), 20.).footprint().empty());
    config.timelapse_type.value = tlSmooth;
    REQUIRE_FALSE(Print::estimate_wipe_tower_preview(config, 1, 0.2, 20.).footprint().empty());
    config.prime_tower_width.value = 0.;
    REQUIRE(Print::estimate_wipe_tower_preview(config, 2, 0.2, 20.).footprint().empty());
}

TEST_CASE("Corner tower preview is a tapered mesh with one skirt and a stable anchor", "[PrimeTower][TowerPreview]")
{
    auto config = tower_preview_config();
    config.prime_volume.value = 45.;
    config.nozzle_diameter.values = {0.4, 0.4, 0.4, 0.4};
    config.prime_tower_enhance_type.value = PrimeTowerEnhanceType::pteCornerRib;
    config.prime_tower_corner_rib_length.value = 10.;
    config.wipe_tower_x.values = {110.};
    config.wipe_tower_y.values = {214.787};
    const auto before = config;
    const auto preview = Print::estimate_wipe_tower_preview(config, 2, 0.2, 26.);
    REQUIRE_FALSE(preview.tower_mesh.empty());
    REQUIRE_FALSE(preview.brim_mesh.empty());
    REQUIRE(preview.tower_mesh.its.vertices.size() > 8);
    BoundingBoxf3 bottom, top;
    for (const Vec3f& vertex : preview.tower_mesh.its.vertices) {
        if (vertex.z() < 0.21f) bottom.merge(vertex.cast<double>());
        if (vertex.z() > 25.9f) top.merge(vertex.cast<double>());
    }
    REQUIRE(bottom.size().x() > top.size().x());
    REQUIRE(bottom.size().y() > top.size().y());
    BoundingBoxf3 combined = preview.tower_mesh.bounding_box();
    combined.merge(preview.brim_mesh.bounding_box());
    const auto placed = get_extents(preview.footprint(0., Vec2d(110., 214.787)));
    REQUIRE(unscaled<double>(placed.min.x()) == Approx(110. + combined.min.x()).margin(1e-5));
    REQUIRE(unscaled<double>(placed.min.y()) == Approx(214.787 + combined.min.y()).margin(1e-5));
    REQUIRE(unscaled<double>(placed.size().x()) == Approx(combined.size().x()).margin(1e-5));
    REQUIRE(unscaled<double>(placed.size().y()) == Approx(combined.size().y()).margin(1e-5));
    REQUIRE(config.diff(before).empty());
}

TEST_CASE("Supplied project tower preview matches the sliced placement", "[.][TowerPreviewProject]")
{
    const char* input = std::getenv("TOWER_PREVIEW_3MF");
    REQUIRE(input != nullptr);
    if (const char* resources = std::getenv("TOWER_PREVIEW_RESOURCES"))
        set_resources_dir(resources);
    auto config = DynamicPrintConfig::full_print_config();
    ConfigSubstitutionContext substitutions(ForwardCompatibilitySubstitutionRule::Enable);
    En3mfType type;
    PlateDataPtrs plates;
    std::vector<Preset*> presets;
    Semver version;
    Model model = Model::read_from_archive(input, &config, &substitutions, type,
        LoadStrategy::LoadModel | LoadStrategy::LoadConfig | LoadStrategy::AddDefaultInstances,
        &plates, &presets, &version);
    release_PlateData_list(plates);
    for (auto* preset : presets) delete preset;
    REQUIRE_FALSE(model.objects.empty());
    Print print;
    print.apply(model, config);
    const auto used = print.used_physical_extruders();
    std::vector<unsigned int> physical_ids;
    for (unsigned int id : used) physical_ids.push_back(id + 1);
    PartPlate plate(nullptr, Vec3d::Zero(), 400, 400, 400, nullptr, &model);
    plate.set_index(0);
    plate.set_shape({Vec2d(0., 0.), Vec2d(400., 0.), Vec2d(400., 400.), Vec2d(0., 400.)},
                    {}, Vec2d::Zero(), 400.f, 400.f);
    for (size_t obj = 0; obj < model.objects.size(); ++obj)
        for (size_t inst = 0; inst < model.objects[obj]->instances.size(); ++inst)
            REQUIRE(plate.add_instance(int(obj), int(inst), false) == 0);
    const auto preview = plate.estimate_wipe_tower_preview(config, int(physical_ids.size()));
    REQUIRE_FALSE(preview.tower_mesh.empty());
    print.process();
    REQUIRE(print.is_step_done(psWipeTower));
    REQUIRE(print.wipe_tower_data().wipe_tower_mesh_data.has_value());
    const auto& real = *print.wipe_tower_data().wipe_tower_mesh_data;
    auto actual = real.real_wipe_tower_mesh;
    actual.merge(real.real_brim_mesh);
    auto predicted = preview.tower_mesh;
    predicted.merge(preview.brim_mesh);
    const auto a = actual.bounding_box();
    const auto p = predicted.bounding_box();
    std::cout << "Tower predicted local XY: " << p.min.head<2>().transpose() << " -> " << p.max.head<2>().transpose() << '\n'
              << "Tower actual local XY: " << a.min.head<2>().transpose() << " -> " << a.max.head<2>().transpose() << '\n'
              << "Tower saved XY: " << print.config().wipe_tower_x.get_at(0) << ", " << print.config().wipe_tower_y.get_at(0) << '\n';
    if (const char* output = std::getenv("TOWER_PREVIEW_OUTPUT")) {
        store_stl((std::string(output) + "-predicted.stl").c_str(), &predicted, true);
        store_stl((std::string(output) + "-actual.stl").c_str(), &actual, true);
    }
    REQUIRE(print.config().wipe_tower_x.get_at(0) == Approx(config.option<ConfigOptionFloats>("wipe_tower_x")->get_at(0)));
    REQUIRE(print.config().wipe_tower_y.get_at(0) == Approx(config.option<ConfigOptionFloats>("wipe_tower_y")->get_at(0)));
    std::cout << "Tower predicted/actual height: " << p.max.z() << " / " << a.max.z() << '\n';
    REQUIRE(std::abs(a.max.z() - p.max.z()) < 0.25);
    REQUIRE((a.min.head<2>() - p.min.head<2>()).norm() < 0.1);
    REQUIRE((a.max.head<2>() - p.max.head<2>()).norm() < 0.1);
}

TEST_CASE("Stabilizing cone preview narrows at intermediate heights", "[PrimeTower][TowerPreview]")
{
    auto config = tower_preview_config();
    config.prime_tower_enhance_type.value = PrimeTowerEnhanceType::pteCone;
    config.prime_volume.value = 45.;
    config.prime_tower_enable_framework.value = false;
    config.wipe_tower_cone_angle.value = 30.;
    config.wipe_tower_no_sparse_layers.value = false;
    const auto preview = Print::estimate_wipe_tower_preview(config, 2, 0.2, 80.);
    REQUIRE_FALSE(preview.tower_mesh.empty());
    BoundingBoxf3 bottom, middle;
    for (const Vec3f& vertex : preview.tower_mesh.its.vertices) {
        if (vertex.z() < 0.21f) bottom.merge(vertex.cast<double>());
        if (vertex.z() > 35.f && vertex.z() < 45.f) middle.merge(vertex.cast<double>());
    }
    REQUIRE(middle.defined);
    REQUIRE(bottom.size().y() > middle.size().y() + 5.);
}

TEST_CASE("Tower preview survives repeated width edits", "[PrimeTower][TowerPreview]")
{
    auto config = tower_preview_config();
    config.prime_volume.value = 45.;
    config.prime_tower_enhance_type.value = PrimeTowerEnhanceType::pteCone;
    config.prime_tower_enable_framework.value = false;
    config.wipe_tower_cone_angle.value = 30.;
    for (double width : {40., 20., 10., 60., 20., 40.}) {
        CAPTURE(width);
        config.prime_tower_width.value = width;
        const auto preview = Print::estimate_wipe_tower_preview(config, 2, 0.2, 27.);
        REQUIRE_FALSE(preview.tower_mesh.empty());
        REQUIRE_FALSE(preview.footprint().empty());
        REQUIRE(preview.size.x() == Approx(width).margin(0.5));
    }
}
