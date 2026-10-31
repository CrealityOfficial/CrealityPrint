#include <catch2/catch.hpp>
#include "slic3r/GUI/NoTowerOptions.hpp"
#include "slic3r/GUI/PrimeTowerOptions.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/Model.hpp"
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
