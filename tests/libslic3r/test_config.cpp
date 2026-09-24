#include <catch2/catch.hpp>

#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/LocalesUtils.hpp"

#include <cereal/types/polymorphic.hpp>
#include <cereal/types/string.hpp> 
#include <cereal/types/vector.hpp> 
#include <cereal/archives/binary.hpp>

using namespace Slic3r;


TEST_CASE("Shared process percentages use the mapped physical nozzle", "[Config][NozzleVariant][17885]")
{
    DynamicPrintConfig config = DynamicPrintConfig::full_print_config();
    config.option<ConfigOptionInts>("filament_map")->values = {1, 2, 1, 2};

    struct ParameterCase {
        const char* key;
        const char* base;
        double first_base;
        double second_base;
        double percent;
        double first_expected;
        double second_expected;
    };
    const ParameterCase cases[] = {
        {"internal_solid_infill_acceleration", "default_acceleration", 5004., 7003., 100., 5004., 7003.},
        {"bridge_acceleration", "outer_wall_acceleration", 3687., 7458., 50., 1843.5, 3729.},
        {"wipe_speed", "travel_speed", 400., 500., 80., 320., 400.},
    };
    for (const auto& item : cases) {
        INFO(item.key);
        config.option<ConfigOptionFloats>(item.base)->values = {item.first_base, item.second_base};
        auto* option = config.option<ConfigOptionFloatOrPercent>(item.key);
        option->value = item.percent;
        option->percent = true;
        const size_t first_nozzle = get_physical_nozzle_index(config, 0);
        const size_t second_nozzle = get_physical_nozzle_index(config, 3);
        REQUIRE(first_nozzle == 0);
        REQUIRE(second_nozzle == 1);
        REQUIRE(config.get_abs_value_at(item.key, first_nozzle) == Approx(item.first_expected));
        REQUIRE(config.get_abs_value_at(item.key, second_nozzle) == Approx(item.second_expected));

        // Absolute overrides and zero keep their meaning for either nozzle.
        option->percent = false;
        option->value = 123.;
        REQUIRE(config.get_abs_value_at(item.key, first_nozzle) == Approx(123.));
        REQUIRE(config.get_abs_value_at(item.key, second_nozzle) == Approx(123.));
        option->value = 0.;
        REQUIRE(config.get_abs_value_at(item.key, second_nozzle) == Approx(0.));

        // Legacy single-value bases still fall back to their only value.
        config.option<ConfigOptionFloats>(item.base)->values = {item.first_base};
        option->value = item.percent;
        option->percent = true;
        REQUIRE(config.get_abs_value_at(item.key, second_nozzle) == Approx(item.first_expected));
        config.option<ConfigOptionFloats>(item.base)->values = {item.first_base, 0.};
        REQUIRE(config.get_abs_value_at(item.key, second_nozzle) == Approx(0.));
    }
}

TEST_CASE("Internal bridge speed is independent for each physical nozzle", "[Config][NozzleVariant]")
{
    DynamicPrintConfig config = DynamicPrintConfig::full_print_config();
    config.option<ConfigOptionInts>("filament_map")->values = {1, 2, 1, 2};
    config.option<ConfigOptionFloats>("bridge_speed")->values = {30., 60.};
    auto* speed = config.option<ConfigOptionFloatsOrPercentsNullable>("internal_bridge_speed");
    REQUIRE(speed != nullptr);
    speed->values = {FloatOrPercent(150., true), FloatOrPercent(120., true)};
    REQUIRE(config.get_abs_value_at("internal_bridge_speed", get_physical_nozzle_index(config, 0)) == Approx(45.));
    REQUIRE(config.get_abs_value_at("internal_bridge_speed", get_physical_nozzle_index(config, 3)) == Approx(72.));
    speed->values[1] = FloatOrPercent(80., false);
    REQUIRE(config.get_abs_value_at("internal_bridge_speed", 0) == Approx(45.));
    REQUIRE(config.get_abs_value_at("internal_bridge_speed", 1) == Approx(80.));

    // Legacy presets have a single serialized value and retain nozzle-specific percentage bases.
    config.set_deserialize_strict("internal_bridge_speed", "150%");
    REQUIRE(config.get_abs_value_at("internal_bridge_speed", 0) == Approx(45.));
    REQUIRE(config.get_abs_value_at("internal_bridge_speed", 1) == Approx(90.));
    config.set_deserialize_strict("internal_bridge_speed", "150%,80");
    REQUIRE(config.opt_serialize("internal_bridge_speed") == "150%,80");
    REQUIRE(config.get_abs_value_at("internal_bridge_speed", 1) == Approx(80.));
}

TEST_CASE("Indexed percentages retain vector and scalar base semantics", "[Config][NozzleVariant][17885]")
{
    DynamicPrintConfig config = DynamicPrintConfig::full_print_config();
    config.option<ConfigOptionFloats>("default_acceleration")->values = {5004., 7003.};
    config.option<ConfigOptionFloatsOrPercents>("sparse_infill_acceleration")->values = {
        FloatOrPercent(50., true), FloatOrPercent(80., true)
    };
    REQUIRE(config.get_abs_value_at("sparse_infill_acceleration", 0) == Approx(2502.));
    REQUIRE(config.get_abs_value_at("sparse_infill_acceleration", 1) == Approx(5602.4));

    config.option<ConfigOptionFloat>("layer_height")->value = 0.2;
    auto* seam_height = config.option<ConfigOptionFloatOrPercent>("seam_slope_start_height");
    seam_height->value = 50.;
    seam_height->percent = true;
    REQUIRE(config.get_abs_value_at("seam_slope_start_height", 1) == Approx(0.1));

    // Absolute vector values do not require their percentage base to be present.
    DynamicPrintConfig absolute_config;
    absolute_config.set_key_value("sparse_infill_acceleration", new ConfigOptionFloatsOrPercentsNullable{
        FloatOrPercent(8865., false), FloatOrPercent(9927., false)
    });
    REQUIRE(absolute_config.get_abs_value_at("sparse_infill_acceleration", 1) == Approx(9927.));
}

TEST_CASE("Nozzle diameter variant selects per-extruder parameter rows", "[Config][NozzleVariant]")
{
    DynamicPrintConfig printer = DynamicPrintConfig::full_print_config();
    printer.option<ConfigOptionEnumsGeneric>("extruder_type")->values = {etDirectDrive, etDirectDrive};

    DynamicPrintConfig source = DynamicPrintConfig::full_print_config();
    source.option<ConfigOptionInts>("print_extruder_id")->values = {1, 1, 2, 2};
    source.option<ConfigOptionStrings>("print_extruder_variant")->values = {
        "Direct Drive Standard", "Direct Drive Standard",
        "Direct Drive Standard", "Direct Drive Standard"
    };
    source.option<ConfigOptionInts>("print_nozzle_variant", true)->values = {0, 1, 0, 1};
    source.option<ConfigOptionFloats>("outer_wall_speed")->values = {10.0, 20.0, 30.0, 40.0};
    source.option<ConfigOptionFloatsOrPercentsNullable>("internal_bridge_speed")->values = {
        FloatOrPercent(110., true), FloatOrPercent(120., true),
        FloatOrPercent(70., false), FloatOrPercent(80., false)};

    const std::vector<int> selected = source.select_extruder_variant_values(
        printer, {nvtStandard, nvtStandard}, {"outer_wall_speed", "internal_bridge_speed"},
        "print_extruder_id", "print_extruder_variant", 1, 0, {1, 0}, "print_nozzle_variant");

    REQUIRE(selected == std::vector<int>{1, 2});
    REQUIRE(source.option<ConfigOptionFloats>("outer_wall_speed")->values == std::vector<double>{20.0, 30.0});
    const auto* internal = source.option<ConfigOptionFloatsOrPercentsNullable>("internal_bridge_speed");
    REQUIRE(internal->size() == 2);
    REQUIRE(internal->get_at(0).value == Approx(120.));
    REQUIRE(internal->get_at(0).percent);
    REQUIRE(internal->get_at(1).value == Approx(70.));
    REQUIRE_FALSE(internal->get_at(1).percent);
}

TEST_CASE("Nozzle combination variant selects layer-height limits", "[Config][NozzleVariant]")
{
    DynamicPrintConfig printer = DynamicPrintConfig::full_print_config();
    printer.option<ConfigOptionEnumsGeneric>("extruder_type")->values = {etDirectDrive, etDirectDrive};

    DynamicPrintConfig source = DynamicPrintConfig::full_print_config();
    source.option<ConfigOptionInts>("printer_extruder_id")->values = {1, 1, 2, 2};
    source.option<ConfigOptionStrings>("printer_extruder_variant")->values = {
        "Direct Drive Standard", "Direct Drive Standard",
        "Direct Drive Standard", "Direct Drive Standard"
    };
    source.option<ConfigOptionInts>("printer_nozzle_variant", true)->values = {0, 1, 0, 1};
    source.option<ConfigOptionFloats>("min_layer_height")->values = {0.08, 0.04, 0.08, 0.04};
    source.option<ConfigOptionFloats>("max_layer_height")->values = {0.32, 0.14, 0.32, 0.14};

    REQUIRE(source.get_parameter_size("min_layer_height", 2) == 4);
    const std::vector<int> selected = source.select_extruder_variant_values(
        printer, {nvtStandard, nvtStandard}, {"min_layer_height", "max_layer_height"},
        "printer_extruder_id", "printer_extruder_variant", 1, 0, {1, 0}, "printer_nozzle_variant");

    REQUIRE(selected == std::vector<int>{1, 2});
    REQUIRE(source.option<ConfigOptionFloats>("min_layer_height")->values == std::vector<double>{0.04, 0.08});
    REQUIRE(source.option<ConfigOptionFloats>("max_layer_height")->values == std::vector<double>{0.14, 0.32});
    REQUIRE(source.option<ConfigOptionEnumsGeneric>("nozzle_variant_volume_types") != nullptr);
}

TEST_CASE("Printer motion variants omit Silent stride when the preset has no Silent mode",
          "[Config][NozzleVariant][MachineLimits]")
{
    DynamicPrintConfig printer = DynamicPrintConfig::full_print_config();
    printer.option<ConfigOptionEnumsGeneric>("extruder_type")->values = {etDirectDrive, etDirectDrive};
    printer.option<ConfigOptionStrings>("nozzle_variant_ids")->values = {
        "E1-0.4-standard", "E1-0.2-standard", "E2-0.4-standard", "E2-0.2-standard"
    };
    printer.option<ConfigOptionInts>("printer_extruder_id")->values = {1, 1, 2, 2};
    printer.option<ConfigOptionStrings>("printer_extruder_variant")->values = {
        "Direct Drive Standard", "Direct Drive Standard",
        "Direct Drive Standard", "Direct Drive Standard"
    };
    printer.option<ConfigOptionInts>("printer_nozzle_variant", true)->values = {0, 1, 0, 1};
    printer.option<ConfigOptionBool>("silent_mode")->value = false;
    printer.option<ConfigOptionFloats>("machine_max_speed_e")->values = {10.0, 20.0, 30.0, 40.0};

    REQUIRE(printer.printer_motion_options_per_nozzle());
    REQUIRE(printer.printer_motion_option_stride() == 1);
    REQUIRE(printer.get_parameter_size("machine_max_speed_e", 2) == 4);

    const std::vector<int> selected = printer.select_extruder_variant_values(
        printer, {nvtStandard, nvtStandard}, {"machine_max_speed_e"},
        "printer_extruder_id", "printer_extruder_variant",
        unsigned(printer.printer_motion_option_stride()), 0, {1, 0}, "printer_nozzle_variant");

    REQUIRE(selected == std::vector<int>{1, 2});
    REQUIRE(printer.option<ConfigOptionFloats>("machine_max_speed_e")->values ==
            std::vector<double>{20.0, 30.0});

    DynamicPrintConfig silent_printer = DynamicPrintConfig::full_print_config();
    silent_printer.option<ConfigOptionStrings>("nozzle_variant_ids")->values = {"E1-0.4-standard"};
    silent_printer.option<ConfigOptionInts>("printer_nozzle_variant", true)->values = {0};
    silent_printer.option<ConfigOptionStrings>("printer_extruder_variant")->values = {"Direct Drive Standard"};
    silent_printer.option<ConfigOptionBool>("silent_mode")->value = true;
    REQUIRE(silent_printer.printer_motion_options_per_nozzle());
    REQUIRE(silent_printer.printer_motion_option_stride() == 2);

    DynamicPrintConfig legacy_printer = DynamicPrintConfig::full_print_config();
    legacy_printer.option<ConfigOptionBool>("silent_mode")->value = false;
    REQUIRE_FALSE(legacy_printer.printer_motion_options_per_nozzle());
    REQUIRE(legacy_printer.printer_motion_option_stride() == 2);
}

TEST_CASE("Object process overrides are materialized by selected nozzle source rows", "[Config][NozzleVariant][Object]")
{
    PrintObjectConfig defaults;
    defaults.default_acceleration.values = {60.0, 70.0};

    Model model;
    ModelObject* object = model.add_object();
    object->config.set_key_value("default_acceleration", new ConfigOptionFloatsNullable({
        ConfigOptionFloatsNullable::nil_value(), 80.0,
        ConfigOptionFloatsNullable::nil_value(), 40.0
    }));

    SECTION("non-nil source rows override their physical nozzle")
    {
        const PrintObjectConfig materialized = PrintObject::object_config_from_model_object(
            defaults, *object, 2, {1, 3});
        REQUIRE(materialized.default_acceleration.values == std::vector<double>{80.0, 40.0});
    }

    SECTION("nil source rows keep the inherited process value")
    {
        const PrintObjectConfig materialized = PrintObject::object_config_from_model_object(
            defaults, *object, 2, {0, 3});
        REQUIRE(materialized.default_acceleration.values == std::vector<double>{60.0, 40.0});
    }

    SECTION("a legacy single common value still applies to every runtime nozzle")
    {
        object->config.set_key_value("default_acceleration", new ConfigOptionFloatsNullable({90.0}));
        const PrintObjectConfig materialized = PrintObject::object_config_from_model_object(
            defaults, *object, 2, {1, 3});
        REQUIRE(materialized.default_acceleration.values == std::vector<double>{90.0, 90.0});
    }
}

TEST_CASE("Filament nozzle mapping configuration and normalization", "[Config][FilamentMap]")
{
    FullPrintConfig config;
    REQUIRE(config.filament_map_mode.value == fmmAutoForSaving);
    REQUIRE(config.filament_map.values == std::vector<int>{1});
    REQUIRE(config.filament_volume_map.values == std::vector<int>{0});
    REQUIRE(config.filament_map_2.values == std::vector<int>{0});
    REQUIRE_FALSE(config.support_filament_nozzle_mapping.value);

    DynamicPrintConfig dynamic = DynamicPrintConfig::full_print_config();
    REQUIRE(dynamic.opt_serialize("filament_map_mode") == "AutoForSaving");
    REQUIRE_NOTHROW(dynamic.set_deserialize_strict("filament_map_mode", "Manual"));
    REQUIRE(dynamic.option<ConfigOptionEnum<FilamentMapMode>>("filament_map_mode")->value == fmmManual);

    REQUIRE(normalize_filament_map({2, 0, 5}, 5, 4) == std::vector<int>{2, 1, 1, 1, 1});
    REQUIRE(normalize_filament_map({4, 3, 2}, 2, 4) == std::vector<int>{4, 3});
    REQUIRE(normalize_filament_map({1}, 1, 0).empty());
    REQUIRE(validate_complete_filament_map({1, 1, 1}, 3, 1).empty());
    REQUIRE_FALSE(validate_complete_filament_map({1}, 3, 1).empty());
    REQUIRE_FALSE(validate_complete_filament_map({1, 3}, 2, 2).empty());
    REQUIRE(build_legacy_filament_map(6, 1) == std::vector<int>{1, 1, 1, 1, 1, 1});
    REQUIRE(build_legacy_filament_map(6, 4) == std::vector<int>{1, 2, 3, 4, 1, 2});
    REQUIRE(build_legacy_filament_map(1, 0).empty());
    REQUIRE(build_filament_map_2({1, 2, 4}) == std::vector<int>{0, 1, 3});
}

TEST_CASE("Nozzle layer-height compatibility uses one physical nozzle range", "[Config][NozzleVariant]")
{
    REQUIRE(is_nozzle_compatible_with_layer_heights(0.4, 0.08, 0.32, 0.20, 0.20));
    REQUIRE_FALSE(is_nozzle_compatible_with_layer_heights(0.2, 0.04, 0.14, 0.20, 0.20));
    REQUIRE_FALSE(is_nozzle_compatible_with_layer_heights(0.4, 0.08, 0.32, 0.20, 0.34));

    // Missing limits use the same fallback as nozzle variant materialization.
    REQUIRE(is_nozzle_compatible_with_layer_heights(0.4, 0.0, 0.0, 0.08, 0.30));
    REQUIRE_FALSE(is_nozzle_compatible_with_layer_heights(0.2, 0.0, 0.0, 0.16, 0.10));
}

TEST_CASE("Filament nozzle mapping mode resolution", "[Config][FilamentMap]")
{
    FilamentMapAutoInput input;
    input.filament_count = 6;
    input.nozzle_count = 4;
    input.used_filaments = {0, 1, 2, 3, 4, 5};
    input.filament_presets = {"PLA", "PLA", "PETG", "ABS", "TPU", "TPU"};
    input.filament_types = {"PLA", "PLA", "PETG", "ABS", "TPU", "TPU"};
    input.filament_colours = {"#00FF00", "#00FF00", "#000000", "#FFFFFF", "#FF0000", "#FF0000"};

    std::string error;
    REQUIRE(resolve_effective_filament_map(fmmAutoForSaving, {}, input, &error) ==
            std::vector<int>{1, 2, 3, 4, 1, 2});
    REQUIRE(error.empty());

    REQUIRE(resolve_effective_filament_map(fmmManual, {2, 2, 8}, input, &error).empty());
    REQUIRE_FALSE(error.empty());

    REQUIRE(resolve_effective_filament_map(fmmManual, {2, 2, 1, 1, 1, 1}, input, &error) ==
            std::vector<int>{2, 2, 1, 1, 1, 1});
    REQUIRE(error.empty());

    REQUIRE(resolve_effective_filament_map(fmmAutoForSaving, {1}, input, &error) ==
            std::vector<int>{1, 2, 3, 4, 1, 2});
    REQUIRE(error.empty());

    input.nozzle_compatibility = {false, true, false, true};
    REQUIRE(resolve_effective_filament_map(fmmAutoForSaving, {}, input, &error) ==
            std::vector<int>{2, 4, 2, 4, 2, 4});
    REQUIRE(error.empty());

    REQUIRE(resolve_effective_filament_map(fmmManual, {2, 4, 2, 4, 2, 4}, input, &error) ==
            std::vector<int>{2, 4, 2, 4, 2, 4});
    REQUIRE(error.empty());

    REQUIRE(resolve_effective_filament_map(fmmManual, {1, 4, 2, 4, 2, 4}, input, &error).empty());
    REQUIRE_FALSE(error.empty());

    input.filament_count = 4;
    input.used_filaments = {1, 3};
    REQUIRE(resolve_effective_filament_map(fmmAutoForSaving, {1, 1, 3, 3}, input, &error) ==
            std::vector<int>{1, 2, 3, 4});
    REQUIRE(error.empty());

    input.nozzle_compatibility = {false, false, false, false};
    REQUIRE(resolve_effective_filament_map(fmmAutoForSaving, {}, input, &error).empty());
    REQUIRE_FALSE(error.empty());

    input.nozzle_compatibility = {true, false};
    REQUIRE(resolve_effective_filament_map(fmmAutoForSaving, {}, input, &error).empty());
    REQUIRE_FALSE(error.empty());

    input.nozzle_compatibility.clear();
    REQUIRE(resolve_effective_filament_map(fmmAutoForMatch, {}, input, &error).empty());
    REQUIRE_FALSE(error.empty());
}

SCENARIO("Generic config validation performs as expected.", "[Config]") {
    GIVEN("A config generated from default options") {
        Slic3r::DynamicPrintConfig config = Slic3r::DynamicPrintConfig::full_print_config();
        WHEN( "perimeter_extrusion_width is set to 250%, a valid value") {
            config.set_deserialize_strict("perimeter_extrusion_width", "250%");
            THEN( "The config is read as valid.") {
                REQUIRE(config.validate().empty());
            }
        }
        WHEN( "perimeter_extrusion_width is set to -10, an invalid value") {
            config.set("perimeter_extrusion_width", -10);
            THEN( "Validate returns error") {
                REQUIRE(! config.validate().empty());
            }
        }

        WHEN( "perimeters is set to -10, an invalid value") {
            config.set("perimeters", -10);
            THEN( "Validate returns error") {
                REQUIRE(! config.validate().empty());
            }
        }
    }
}

SCENARIO("Config accessor functions perform as expected.", "[Config]") {
    GIVEN("A config generated from default options") {
        Slic3r::DynamicPrintConfig config = Slic3r::DynamicPrintConfig::full_print_config();
        WHEN("A boolean option is set to a boolean value") {
            REQUIRE_NOTHROW(config.set("gcode_comments", true));
            THEN("The underlying value is set correctly.") {
                REQUIRE(config.opt<ConfigOptionBool>("gcode_comments")->getBool() == true);
            }
        }
        WHEN("A boolean option is set to a string value representing a 0 or 1") {
            CHECK_NOTHROW(config.set_deserialize_strict("gcode_comments", "1"));
            THEN("The underlying value is set correctly.") {
                REQUIRE(config.opt<ConfigOptionBool>("gcode_comments")->getBool() == true);
            }
        }
        WHEN("A boolean option is set to a string value representing something other than 0 or 1") {
            THEN("A BadOptionTypeException exception is thrown.") {
                REQUIRE_THROWS_AS(config.set("gcode_comments", "Z"), BadOptionTypeException);
            }
            AND_THEN("Value is unchanged.") {
                REQUIRE(config.opt<ConfigOptionBool>("gcode_comments")->getBool() == false);
            }
        }
        WHEN("A boolean option is set to an int value") {
            THEN("A BadOptionTypeException exception is thrown.") {
                REQUIRE_THROWS_AS(config.set("gcode_comments", 1), BadOptionTypeException);
            }
        }
        WHEN("A numeric option is set from serialized string") {
            config.set_deserialize_strict("bed_temperature", "100");
            THEN("The underlying value is set correctly.") {
                REQUIRE(config.opt<ConfigOptionInts>("bed_temperature")->get_at(0) == 100);
            }
        }
#if 0
		//FIXME better design accessors for vector elements.
		WHEN("An integer-based option is set through the integer interface") {
            config.set("bed_temperature", 100);
            THEN("The underlying value is set correctly.") {
                REQUIRE(config.opt<ConfigOptionInts>("bed_temperature")->get_at(0) == 100);
            }
        }
#endif
        WHEN("An floating-point option is set through the integer interface") {
            config.set("perimeter_speed", 10);
            THEN("The underlying value is set correctly.") {
                REQUIRE(config.opt<ConfigOptionFloat>("perimeter_speed")->getFloat() == 10.0);
            }
        }
        WHEN("A floating-point option is set through the double interface") {
            config.set("perimeter_speed", 5.5);
            THEN("The underlying value is set correctly.") {
                REQUIRE(config.opt<ConfigOptionFloat>("perimeter_speed")->getFloat() == 5.5);
            }
        }
        WHEN("An integer-based option is set through the double interface") {
            THEN("A BadOptionTypeException exception is thrown.") {
                REQUIRE_THROWS_AS(config.set("bed_temperature", 5.5), BadOptionTypeException);
            }
        }
        WHEN("A numeric option is set to a non-numeric value.") {
            THEN("A BadOptionTypeException exception is thown.") {
                REQUIRE_THROWS_AS(config.set_deserialize_strict("perimeter_speed", "zzzz"), BadOptionValueException);
            }
            THEN("The value does not change.") {
                REQUIRE(config.opt<ConfigOptionFloat>("perimeter_speed")->getFloat() == 60.0);
            }
        }
        WHEN("A string option is set through the string interface") {
            config.set("end_gcode", "100");
            THEN("The underlying value is set correctly.") {
                REQUIRE(config.opt<ConfigOptionString>("end_gcode")->value == "100");
            }
        }
        WHEN("A string option is set through the integer interface") {
            config.set("end_gcode", 100);
            THEN("The underlying value is set correctly.") {
                REQUIRE(config.opt<ConfigOptionString>("end_gcode")->value == "100");
            }
        }
        WHEN("A string option is set through the double interface") {
            config.set("end_gcode", 100.5);
            THEN("The underlying value is set correctly.") {
                REQUIRE(config.opt<ConfigOptionString>("end_gcode")->value == float_to_string_decimal_point(100.5));
            }
        }
        WHEN("A float or percent is set as a percent through the string interface.") {
            config.set_deserialize_strict("first_layer_extrusion_width", "100%");
            THEN("Value and percent flag are 100/true") {
                auto tmp = config.opt<ConfigOptionFloatOrPercent>("first_layer_extrusion_width");
                REQUIRE(tmp->percent == true);
                REQUIRE(tmp->value == 100);
            }
        }
        WHEN("A float or percent is set as a float through the string interface.") {
            config.set_deserialize_strict("first_layer_extrusion_width", "100");
            THEN("Value and percent flag are 100/false") {
                auto tmp = config.opt<ConfigOptionFloatOrPercent>("first_layer_extrusion_width");
                REQUIRE(tmp->percent == false);
                REQUIRE(tmp->value == 100);
            }
        }
        WHEN("A float or percent is set as a float through the int interface.") {
            config.set("first_layer_extrusion_width", 100);
            THEN("Value and percent flag are 100/false") {
                auto tmp = config.opt<ConfigOptionFloatOrPercent>("first_layer_extrusion_width");
                REQUIRE(tmp->percent == false);
                REQUIRE(tmp->value == 100);
            }
        }
        WHEN("A float or percent is set as a float through the double interface.") {
            config.set("first_layer_extrusion_width", 100.5);
            THEN("Value and percent flag are 100.5/false") {
                auto tmp = config.opt<ConfigOptionFloatOrPercent>("first_layer_extrusion_width");
                REQUIRE(tmp->percent == false);
                REQUIRE(tmp->value == 100.5);
            }
        }
        WHEN("An invalid option is requested during set.") {
            THEN("A BadOptionTypeException exception is thrown.") {
                REQUIRE_THROWS_AS(config.set("deadbeef_invalid_option", 1), UnknownOptionException);
                REQUIRE_THROWS_AS(config.set("deadbeef_invalid_option", 1.0), UnknownOptionException);
                REQUIRE_THROWS_AS(config.set("deadbeef_invalid_option", "1"), UnknownOptionException);
                REQUIRE_THROWS_AS(config.set("deadbeef_invalid_option", true), UnknownOptionException);
            }
        }

        WHEN("An invalid option is requested during get.") {
            THEN("A UnknownOptionException exception is thrown.") {
                REQUIRE_THROWS_AS(config.option_throw<ConfigOptionString>("deadbeef_invalid_option", false), UnknownOptionException);
                REQUIRE_THROWS_AS(config.option_throw<ConfigOptionFloat>("deadbeef_invalid_option", false), UnknownOptionException);
                REQUIRE_THROWS_AS(config.option_throw<ConfigOptionInt>("deadbeef_invalid_option", false), UnknownOptionException);
                REQUIRE_THROWS_AS(config.option_throw<ConfigOptionBool>("deadbeef_invalid_option", false), UnknownOptionException);
            }
        }
        WHEN("An invalid option is requested during opt.") {
            THEN("A UnknownOptionException exception is thrown.") {
                REQUIRE_THROWS_AS(config.option_throw<ConfigOptionString>("deadbeef_invalid_option", false), UnknownOptionException);
                REQUIRE_THROWS_AS(config.option_throw<ConfigOptionFloat>("deadbeef_invalid_option", false), UnknownOptionException);
                REQUIRE_THROWS_AS(config.option_throw<ConfigOptionInt>("deadbeef_invalid_option", false), UnknownOptionException);
                REQUIRE_THROWS_AS(config.option_throw<ConfigOptionBool>("deadbeef_invalid_option", false), UnknownOptionException);
            }
        }

        WHEN("getX called on an unset option.") {
            THEN("The default is returned.") {
                REQUIRE(config.opt_float("layer_height") == 0.3);
                REQUIRE(config.opt_int("raft_layers") == 0);
                REQUIRE(config.opt_bool("support_material") == false);
            }
        }

        WHEN("getFloat called on an option that has been set.") {
            config.set("layer_height", 0.5);
            THEN("The set value is returned.") {
                REQUIRE(config.opt_float("layer_height") == 0.5);
            }
        }
    }
}

SCENARIO("Config ini load/save interface", "[Config]") {
    WHEN("new_from_ini is called") {
		Slic3r::DynamicPrintConfig config;
		std::string path = std::string(TEST_DATA_DIR) + "/test_config/new_from_ini.ini";
		config.load_from_ini(path, ForwardCompatibilitySubstitutionRule::Disable);
        THEN("Config object contains ini file options.") {
			REQUIRE(config.option_throw<ConfigOptionStrings>("filament_colour", false)->values.size() == 1);
			REQUIRE(config.option_throw<ConfigOptionStrings>("filament_colour", false)->values.front() == "#ABCD");
        }
    }
}

SCENARIO("DynamicPrintConfig serialization", "[Config]") {
    WHEN("DynamicPrintConfig is serialized and deserialized") {
        FullPrintConfig full_print_config;
        DynamicPrintConfig cfg;
        cfg.apply(full_print_config, false);

        std::string serialized;
        try {
            std::ostringstream ss;
            cereal::BinaryOutputArchive oarchive(ss);
            oarchive(cfg);
            serialized = ss.str();
        } catch (const std::runtime_error & /* e */) {
            // e.what();
        }

        THEN("Config object contains ini file options.") {
            DynamicPrintConfig cfg2;
            try {
                std::stringstream ss(serialized);
                cereal::BinaryInputArchive iarchive(ss);
                iarchive(cfg2);
            } catch (const std::runtime_error & /* e */) {
                // e.what();
            }
            REQUIRE(cfg == cfg2);
        }
    }
}
