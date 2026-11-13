#include <catch2/catch.hpp>
#include "libslic3r/PresetBundle.hpp"

using namespace Slic3r;

namespace {
void add_profile(PresetCollection &collection, const std::string &name)
{
    auto config = collection.default_preset().config;
    collection.load_preset("", name, config, true);
    collection.get_selected_preset().is_system = true;
    collection.get_edited_preset().is_system = true;
}
void initialize(PresetBundle &bundle)
{
    add_profile(bundle.printers, "Printer");
    add_profile(bundle.prints, "Process");
    add_profile(bundle.filaments, "Material");
    bundle.filament_presets = {"Material"};
}
}

TEST_CASE("Live profile update merges defaults and unsaved changes", "[LiveProfileUpdate]")
{
    PresetBundle live;
    initialize(live);
    live.prints.get_selected_preset().config.set_key_value("layer_height", new ConfigOptionFloat(0.2));
    live.prints.discard_current_changes();
    PresetBundle updated(live);
    live.prints.get_edited_preset().config.set_key_value("layer_height", new ConfigOptionFloat(0.16));
    live.prints.update_dirty();
    live.printers.get_edited_preset().config.set_key_value("extruder_clearance_radius", new ConfigOptionFloat(42));
    live.filaments.get_edited_preset().config.set_key_value("nozzle_temperature", new ConfigOptionInts({237}));
    live.printers.update_dirty();
    live.filaments.update_dirty();
    updated.prints.get_selected_preset().config.set_key_value("layer_height", new ConfigOptionFloat(0.24));
    updated.prints.get_selected_preset().config.set_key_value("initial_layer_speed", new ConfigOptionFloat(37));
    updated.prints.discard_current_changes();
    live.project_config.set_key_value("filament_colour", new ConfigOptionStrings({"#112233", "#445566"}));
    const auto project = live.project_config;
    std::string reason;
    REQUIRE(live.apply_profile_update(updated, reason));
    CHECK(live.prints.get_selected_preset().config.opt_float("layer_height") == Approx(0.24));
    CHECK(live.prints.get_edited_preset().config.opt_float("layer_height") == Approx(0.16));
    CHECK(live.prints.get_edited_preset().config.opt_float("initial_layer_speed") == Approx(37));
    CHECK(live.prints.get_edited_preset().is_dirty);
    CHECK(live.printers.get_edited_preset().config.opt_float("extruder_clearance_radius") == Approx(42));
    CHECK(live.filaments.get_edited_preset().config.option<ConfigOptionInts>("nozzle_temperature")->values.front() == 237);
    CHECK(live.printers.get_edited_preset().is_dirty);
    CHECK(live.filaments.get_edited_preset().is_dirty);
    CHECK(live.prints.get_selected_preset_name() == "Process");
    CHECK(live.filament_presets == std::vector<std::string>{"Material"});
    CHECK(live.project_config.diff(project).empty());

    // A later update matching the user edit should clear the modified state.
    updated.prints.get_selected_preset().config.set_key_value("layer_height", new ConfigOptionFloat(0.16));
    updated.prints.discard_current_changes();
    REQUIRE(live.apply_profile_update(updated, reason));
    CHECK_FALSE(live.prints.get_edited_preset().is_dirty);
}

TEST_CASE("Live profile update merges nozzle vector elements independently", "[LiveProfileUpdate]")
{
    PresetBundle live;
    initialize(live);
    live.filaments.get_selected_preset().config.set_key_value("nozzle_temperature", new ConfigOptionInts({200, 210}));
    live.filaments.discard_current_changes();
    PresetBundle updated(live);
    live.filaments.get_edited_preset().config.set_key_value("nozzle_temperature", new ConfigOptionInts({205, 210}));
    updated.filaments.get_selected_preset().config.set_key_value("nozzle_temperature", new ConfigOptionInts({220, 230}));
    updated.filaments.discard_current_changes();
    std::string reason;
    REQUIRE(live.apply_profile_update(updated, reason));
    CHECK(live.filaments.get_edited_preset().config.option<ConfigOptionInts>("nozzle_temperature")->values == std::vector<int>{205, 230});
    CHECK(live.filaments.get_edited_preset().is_dirty);
}

TEST_CASE("Incompatible live profile update leaves the project untouched", "[LiveProfileUpdate]")
{
    PresetBundle live;
    initialize(live);
    PresetBundle updated(live);
    live.prints.get_edited_preset().config.set_key_value("layer_height", new ConfigOptionFloat(0.17));
    const auto edited = live.prints.get_edited_preset().config;
    SECTION("Active preset removed") { updated.prints.reset(false); }
    SECTION("Physical nozzle layout changed") {
        updated.printers.get_selected_preset().config.set_key_value("nozzle_diameter", new ConfigOptionFloats({0.4, 0.4}));
        updated.printers.discard_current_changes();
    }
    SECTION("Edited vector layout changed") {
        live.filaments.get_selected_preset().config.set_key_value("nozzle_temperature", new ConfigOptionInts({200, 210}));
        live.filaments.get_edited_preset().config.set_key_value("nozzle_temperature", new ConfigOptionInts({205, 210}));
        updated.filaments.get_selected_preset().config.set_key_value("nozzle_temperature", new ConfigOptionInts({220, 230, 240}));
        updated.filaments.discard_current_changes();
    }
    std::string reason;
    REQUIRE_FALSE(live.apply_profile_update(updated, reason));
    CHECK_FALSE(reason.empty());
    CHECK(live.prints.get_edited_preset().config.diff(edited).empty());
    CHECK(live.prints.get_selected_preset_name() == "Process");
}

TEST_CASE("Live profile update preserves project presets and all material slots", "[LiveProfileUpdate]")
{
    PresetBundle live;
    initialize(live);
    PresetBundle updated(live);
    auto config = live.filaments.default_preset().config;
    config.set_key_value("nozzle_temperature", new ConfigOptionInts({245}));
    auto &project = live.filaments.load_preset("", "Project material", config, false);
    project.is_project_embedded = true;
    live.filament_presets = {"Material", "Project material", "Material"};
    live.filaments.select_preset_by_name("Project material", true);
    live.filaments.get_edited_preset().config.set_key_value("nozzle_temperature", new ConfigOptionInts({250}));
    std::string reason;
    REQUIRE(live.apply_profile_update(updated, reason));
    CHECK(live.filament_presets == std::vector<std::string>{"Material", "Project material", "Material"});
    CHECK(live.filaments.get_selected_preset_name() == "Project material");
    CHECK(live.filaments.get_edited_preset().is_project_embedded);
    CHECK(live.filaments.get_selected_preset().config.option<ConfigOptionInts>("nozzle_temperature")->values.front() == 245);
    CHECK(live.filaments.get_edited_preset().config.option<ConfigOptionInts>("nozzle_temperature")->values.front() == 250);
}

TEST_CASE("Live profile update rebinds edited vendor pointers", "[LiveProfileUpdate]")
{
    PresetBundle live;
    initialize(live);
    live.vendors.emplace("Test vendor", VendorProfile("Test vendor"));
    live.prints.get_selected_preset().vendor = &live.vendors.at("Test vendor");
    live.prints.select_preset_by_name("Process", true);
    {
        PresetBundle updated(live);
        std::string reason;
        REQUIRE(live.apply_profile_update(updated, reason));
    }
    CHECK(live.prints.get_selected_preset().vendor == &live.vendors.at("Test vendor"));
    CHECK(live.prints.get_edited_preset().vendor == &live.vendors.at("Test vendor"));
}
