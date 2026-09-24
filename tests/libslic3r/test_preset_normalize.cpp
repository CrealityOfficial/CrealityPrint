#include <catch2/catch.hpp>
#include "libslic3r/Preset.hpp"

using namespace Slic3r;

TEST_CASE("Preset normalization keeps filament slots independent of nozzle count", "[Config][PresetNormalize]")
{
    DynamicPrintConfig config = DynamicPrintConfig::full_print_config();
    const size_t nozzle_count = GENERATE(size_t(1), size_t(2));
    config.option<ConfigOptionFloats>("nozzle_diameter")->values.assign(nozzle_count, 0.4);
    config.option<ConfigOptionStrings>("printer_extruder_variant", true)->values.assign(nozzle_count, "Direct Drive Standard");
    config.option<ConfigOptionInts>("printer_nozzle_variant", true)->values.assign(nozzle_count, 0);
    config.option<ConfigOptionInts>("printer_extruder_id", true)->values = nozzle_count == 1 ? std::vector<int>{1} : std::vector<int>{1, 2};
    config.option<ConfigOptionFloats>("filament_diameter")->values = {1.75, 1.75, 1.75};
    const std::vector<std::string> names = {"PLA preset", "PETG preset", "ABS preset"};
    config.option<ConfigOptionStrings>("filament_settings_id", true)->values = names;
    config.option<ConfigOptionStrings>("filament_type")->values = {"PLA", "PETG", "ABS"};
    config.option<ConfigOptionStrings>("filament_colour")->values = {"#FFFFFF", "#00CE00", "#FF9F40"};
    config.option<ConfigOptionInts>("nozzle_temperature")->values = {210, 240, 260};

    Preset::normalize(config);

    REQUIRE(config.option<ConfigOptionStrings>("filament_settings_id")->values == names);
    REQUIRE(config.option<ConfigOptionStrings>("filament_type")->values == std::vector<std::string>{"PLA", "PETG", "ABS"});
    REQUIRE(config.option<ConfigOptionFloats>("filament_diameter")->size() == 3);
    REQUIRE(config.option<ConfigOptionStrings>("filament_colour")->values == std::vector<std::string>{"#FFFFFF", "#00CE00", "#FF9F40"});
    REQUIRE(config.option<ConfigOptionInts>("nozzle_temperature")->values == std::vector<int>{210, 240, 260});
    REQUIRE(config.option<ConfigOptionFloats>("nozzle_diameter")->size() == nozzle_count);
    REQUIRE(config.option<ConfigOptionFloats>("retraction_length")->size() == nozzle_count);
}

TEST_CASE("Normalizing a filament preset preserves its variant rows", "[Config][PresetNormalize]")
{
    DynamicPrintConfig config;
    config.set_key_value("filament_diameter", new ConfigOptionFloats{1.75});
    config.set_key_value("filament_settings_id", new ConfigOptionStrings{"PLA preset"});
    config.set_key_value("filament_extruder_variant", new ConfigOptionStrings{"Direct Drive Standard", "Direct Drive High Flow"});
    config.set_key_value("filament_nozzle_variant", new ConfigOptionInts{0, 1});
    config.set_key_value("nozzle_temperature", new ConfigOptionInts{210, 220});

    Preset::normalize(config);

    REQUIRE(config.option<ConfigOptionStrings>("filament_settings_id")->values == std::vector<std::string>{"PLA preset"});
    REQUIRE(config.option<ConfigOptionInts>("nozzle_temperature")->values == std::vector<int>{210, 220});
    REQUIRE(config.option<ConfigOptionStrings>("filament_extruder_variant")->size() == 2);
    REQUIRE(config.option<ConfigOptionInts>("filament_nozzle_variant")->size() == 2);
    REQUIRE_FALSE(config.has("nozzle_diameter"));
}
