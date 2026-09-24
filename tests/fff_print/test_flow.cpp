#include <catch2/catch.hpp>

#include <numeric>
#include <sstream>

#include "test_data.hpp" // get access to init_print, etc

#include "libslic3r/Config.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/Config.hpp"
#include "libslic3r/GCodeReader.hpp"
#include "libslic3r/Flow.hpp"
#include "libslic3r/libslic3r.h"

using namespace Slic3r::Test;
using namespace Slic3r;

SCENARIO("Extrusion width specifics", "[Flow]") {
    GIVEN("A config with a skirt, brim, some fill density, 3 perimeters, and 1 bottom solid layer and a 20mm cube mesh") {
        // this is a sharedptr
        DynamicPrintConfig config = Slic3r::DynamicPrintConfig::full_print_config();
		config.set_deserialize_strict({
			{ "brim_width",			2 },
			{ "skirts",				1 },
			{ "perimeters",			3 },
			{ "fill_density",		"40%" },
			{ "first_layer_height", 0.3 }
			});

        WHEN("first layer width set to 2mm") {
            Slic3r::Model model;
            config.set("first_layer_extrusion_width", 2);
            Slic3r::Print print;
            Slic3r::Test::init_print({TestMesh::cube_20x20x20}, print, model, config);

            std::vector<double> E_per_mm_bottom;
            std::string gcode = Test::gcode(print);
            Slic3r::GCodeReader parser;
            const double layer_height = config.opt_float("layer_height");
            parser.parse_buffer(gcode, [&E_per_mm_bottom, layer_height] (Slic3r::GCodeReader& self, const Slic3r::GCodeReader::GCodeLine& line)
            { 
                if (self.z() == Approx(layer_height).margin(0.01)) { // only consider first layer
                    if (line.extruding(self) && line.dist_XY(self) > 0) {
                        E_per_mm_bottom.emplace_back(line.dist_E(self) / line.dist_XY(self));
                    }
                }
            });
            THEN(" First layer width applies to everything on first layer.") {
                bool pass = false;
                double avg_E = std::accumulate(E_per_mm_bottom.cbegin(), E_per_mm_bottom.cend(), 0.0) / static_cast<double>(E_per_mm_bottom.size());

                pass = (std::count_if(E_per_mm_bottom.cbegin(), E_per_mm_bottom.cend(), [avg_E] (const double& v) { return v == Approx(avg_E); }) == 0);
                REQUIRE(pass == true);
                REQUIRE(E_per_mm_bottom.size() > 0); // make sure it actually passed because of extrusion
            }
            THEN(" First layer width does not apply to upper layer.") {
            }
        }
    }
}
// needs gcode export
SCENARIO(" Bridge flow specifics.", "[Flow]") {
    GIVEN("A default config with no cooling and a fixed bridge speed, flow ratio and an overhang mesh.") {
        WHEN("bridge_flow_ratio is set to 1.0") {
            THEN("Output flow is as expected.") {
            }
        }
        WHEN("bridge_flow_ratio is set to 0.5") {
            THEN("Output flow is as expected.") {
            }
        }
        WHEN("bridge_flow_ratio is set to 2.0") {
            THEN("Output flow is as expected.") {
            }
        }
    }
    GIVEN("A default config with no cooling and a fixed bridge speed, flow ratio, fixed extrusion width of 0.4mm and an overhang mesh.") {
        WHEN("bridge_flow_ratio is set to 1.0") {
            THEN("Output flow is as expected.") {
            }
        }
        WHEN("bridge_flow_ratio is set to 0.5") {
            THEN("Output flow is as expected.") {
            }
        }
        WHEN("bridge_flow_ratio is set to 2.0") {
            THEN("Output flow is as expected.") {
            }
        }
    }
}

/// Test the expected behavior for auto-width, 
/// spacing, etc
SCENARIO("Flow: Flow math for non-bridges", "[Flow]") {
    GIVEN("Nozzle Diameter of 0.4, a desired width of 1mm and layer height of 0.5") {
        ConfigOptionFloatOrPercent	width(1.0, false);
        float nozzle_diameter	= 0.4f;
        float layer_height		= 0.4f;

        // Spacing for non-bridges is has some overlap
        THEN("External perimeter flow has spacing fixed to 1.125 * nozzle_diameter") {
            auto flow = Flow::new_from_config_width(frExternalPerimeter, ConfigOptionFloatOrPercent(0, false), nozzle_diameter, layer_height);
            REQUIRE(flow.spacing() == Approx(1.125 * nozzle_diameter - layer_height * (1.0 - PI / 4.0)));
        }

        THEN("Internal perimeter flow has spacing fixed to 1.125 * nozzle_diameter") {
            auto flow = Flow::new_from_config_width(frPerimeter, ConfigOptionFloatOrPercent(0, false), nozzle_diameter, layer_height);
            REQUIRE(flow.spacing() == Approx(1.125 *nozzle_diameter - layer_height * (1.0 - PI / 4.0)));
        }
        THEN("Spacing for supplied width is 0.8927f") {
            auto flow = Flow::new_from_config_width(frExternalPerimeter, width, nozzle_diameter, layer_height);
            REQUIRE(flow.spacing() == Approx(width.value - layer_height * (1.0 - PI / 4.0)));
            flow = Flow::new_from_config_width(frPerimeter, width, nozzle_diameter, layer_height);
            REQUIRE(flow.spacing() == Approx(width.value - layer_height * (1.0 - PI / 4.0)));
        }
    }
    /// Check the min/max
    GIVEN("Nozzle Diameter of 0.25") {
        float nozzle_diameter	= 0.25f;
        float layer_height		= 0.5f;
        WHEN("layer height is set to 0.2") {
            layer_height = 0.15f;
            THEN("Max width is set.") {
                auto flow = Flow::new_from_config_width(frPerimeter, ConfigOptionFloatOrPercent(0, false), nozzle_diameter, layer_height);
                REQUIRE(flow.width() == Approx(1.125 * nozzle_diameter));
            }
        }
        WHEN("Layer height is set to 0.25") {
            layer_height = 0.25f;
            THEN("Min width is set.") {
                auto flow = Flow::new_from_config_width(frPerimeter, ConfigOptionFloatOrPercent(0, false), nozzle_diameter, layer_height);
                REQUIRE(flow.width() == Approx(1.125 * nozzle_diameter));
            }
        }
    }

#if 0
    /// Check for an edge case in the maths where the spacing could be 0; original
    /// math is 0.99. Slic3r issue #4654
    GIVEN("Input spacing of 0.414159 and a total width of 2") {
        double in_spacing = 0.414159;
        double total_width = 2.0;
        auto flow = Flow::new_from_spacing(1.0, 0.4, 0.3);
        WHEN("solid_spacing() is called") {
            double result = flow.solid_spacing(total_width, in_spacing);
            THEN("Yielded spacing is greater than 0") {
                REQUIRE(result > 0);
            }
        }
    }
#endif    

}

/// Spacing, width calculation for bridge extrusions
SCENARIO("Flow: Flow math for bridges", "[Flow]") {
    GIVEN("Nozzle Diameter of 0.4, a desired width of 1mm and layer height of 0.5") {
		float nozzle_diameter	= 0.4f;
		float bridge_flow		= 1.0f;
        WHEN("Flow role is frExternalPerimeter") {
            auto flow = Flow::bridging_flow(nozzle_diameter * sqrt(bridge_flow), nozzle_diameter);
            THEN("Bridge width is same as nozzle diameter") {
                REQUIRE(flow.width() == Approx(nozzle_diameter));
            }
            THEN("Bridge spacing is same as nozzle diameter + BRIDGE_EXTRA_SPACING") {
                REQUIRE(flow.spacing() == Approx(nozzle_diameter + BRIDGE_EXTRA_SPACING));
            }
        }
    }
}


namespace {
DynamicPrintConfig nozzle_width_test_config()
{
    DynamicPrintConfig config = DynamicPrintConfig::full_print_config();
    config.set_num_filaments(2);
    config.set_deserialize_strict({
        {"nozzle_diameter", "0.6,0.4,0.2,0.2"},
        {"filament_map", "1,2"},
        {"filament_map_2", "0,1"},
        {"filament_map_mode", "Manual"},
        {"support_filament_nozzle_mapping", "1"},
        {"layer_height", "0.2"},
        {"initial_layer_print_height", "0.2"},
        {"enable_prime_tower", "0"},
        {"enable_support", "0"},
        {"brim_type", "no_brim"},
        {"skirt_loops", "0"},
        {"before_layer_change_gcode", "G92 E0"},
        {"layer_change_gcode", "G92 E0"},
        {"initial_layer_line_width", "0.65,0.5,0.25,0.25"},
        {"line_width", "0.66,0.42,0.22,0.22"},
        {"outer_wall_line_width", "0.6,0.42,0.22,0.22"},
        {"inner_wall_line_width", "0.61,0.45,0.25,0.25"},
        {"sparse_infill_line_width", "0.64,0.45,0.25,0.25"},
        {"internal_solid_infill_line_width", "0.63,0.42,0.22,0.22"},
        {"top_surface_line_width", "0.68,0.42,0.22,0.22"},
        {"support_line_width", "0.62,0.4,0.2,0.2"}
    });
    return config;
}
}

TEST_CASE("Unused nozzle support widths do not block a 0.6 mm cube", "[Flow][NozzleVariant][17866]")
{
    auto config = nozzle_width_test_config();
    config.set_deserialize_strict({{"enable_support", "1"}});
    Print print;
    Model model;
    init_print({TestMesh::cube_20x20x20}, print, model, config);
    REQUIRE(print.validate().string.empty());
    REQUIRE(support_material_flow(print.objects().front()).width() == Approx(0.62));

    SECTION("An invalid width on the actual support nozzle is still rejected") {
        config.set_deserialize_strict({{"support_line_width", "0.2,0.4,0.2,0.2"}});
        print.apply(model, config);
        REQUIRE(print.validate().opt_key == "support_line_width");
    }
    SECTION("A default-width fallback reports the actual source parameter") {
        config.set_deserialize_strict({{"support_line_width", "0"}, {"line_width", "0.19,0.42,0.22,0.22"}});
        print.apply(model, config);
        REQUIRE(print.validate().opt_key == "line_width");
    }
}

TEST_CASE("Wall and infill validation use their own mapped nozzle rows", "[Flow][NozzleVariant][17866]")
{
    auto config = nozzle_width_test_config();
    config.set_deserialize_strict({
        {"wall_filament", "1"}, {"sparse_infill_filament", "2"}, {"solid_infill_filament", "2"},
        {"outer_wall_line_width", "0.6,0.1,0.1,0.1"},
        {"inner_wall_line_width", "0.61,0.1,0.1,0.1"},
        {"sparse_infill_line_width", "0.1,0.45,0.1,0.1"},
        {"internal_solid_infill_line_width", "0.1,0.42,0.1,0.1"},
        {"top_surface_line_width", "0.1,0.42,0.1,0.1"}
    });
    Print print;
    Model model;
    init_print({TestMesh::cube_20x20x20}, print, model, config);
    REQUIRE(print.validate().string.empty());
    const PrintObject &object = *print.objects().front();
    const PrintRegion &region = object.all_regions().front().get();
    REQUIRE(region.flow(object, frExternalPerimeter, 0.2).width() == Approx(0.6));
    REQUIRE(region.flow(object, frInfill, 0.2).width() == Approx(0.45));

    config.set_deserialize_strict({{"sparse_infill_line_width", "0.64,0.19,0.1,0.1"}});
    print.apply(model, config);
    REQUIRE(print.validate().opt_key == "sparse_infill_line_width");
}

TEST_CASE("Automatic support geometry follows a participating nozzle instead of nozzle zero", "[Flow][NozzleVariant][17866]")
{
    auto config = nozzle_width_test_config();
    config.set_deserialize_strict({{"enable_support", "1"}, {"filament_map", "2,2"}, {"filament_map_2", "1,1"},
                                   {"support_line_width", "0.1,0.4,0.2,0.2"}});
    Print print;
    Model model;
    init_print({TestMesh::cube_20x20x20}, print, model, config);
    REQUIRE(print.validate().string.empty());
    REQUIRE(support_material_flow(print.objects().front()).width() == Approx(0.4));
    REQUIRE(support_material_flow(print.objects().front()).nozzle_diameter() == Approx(0.4));
    REQUIRE(support_material_interface_flow(print.objects().front()).width() == Approx(0.4));
}

TEST_CASE("Resolved flow preserves percentages, first-layer overrides and automatic values", "[Flow][NozzleVariant][17866]")
{
    auto config = nozzle_width_test_config();
    config.set_deserialize_strict({{"outer_wall_line_width", "110%"}, {"initial_layer_line_width", "0.7"}});
    Print print;
    Model model;
    init_print({TestMesh::cube_20x20x20}, print, model, config);
    const PrintObject &object = *print.objects().front();
    const PrintRegion &region = object.all_regions().front().get();
    REQUIRE(region.flow(object, frExternalPerimeter, 0.2).width() == Approx(0.66));
    REQUIRE(region.flow(object, frExternalPerimeter, 0.2, true).width() == Approx(0.7));

    config.set_deserialize_strict({{"outer_wall_line_width", "0"}, {"line_width", "0"}});
    print.apply(model, config);
    const PrintObject &updated_object = *print.objects().front();
    const PrintRegion &updated_region = updated_object.all_regions().front().get();
    REQUIRE(print.validate().string.empty());
    REQUIRE(updated_region.flow(updated_object, frExternalPerimeter, 0.2).width() == Approx(0.675));
}

TEST_CASE("Preset width checks defer nozzle-dependent limits to effective print roles", "[Flow][Config][17866]")
{
    auto config = nozzle_width_test_config();
    // 1.6 is above the former import-only 2.5x limit, but below the retained 5x print limit.
    config.set_deserialize_strict({{"outer_wall_line_width", "1.6,10,10,10"}});
    REQUIRE(config.validate().count("outer_wall_line_width") == 0);
    Print print;
    Model model;
    init_print({TestMesh::cube_20x20x20}, print, model, config);
    REQUIRE(print.validate().string.empty());

    config.set_deserialize_strict({{"outer_wall_line_width", "3.1,10,10,10"}});
    print.apply(model, config);
    REQUIRE(print.validate().opt_key == "outer_wall_line_width");
    config.set_deserialize_strict({{"outer_wall_line_width", "-1"}});
    REQUIRE(config.validate().count("outer_wall_line_width") == 1);
}


TEST_CASE("Disabled print roles do not validate their stored widths", "[Flow][NozzleVariant][17866]")
{
    auto config = nozzle_width_test_config();
    config.set_deserialize_strict({
        {"sparse_infill_density", "0%"}, {"top_shell_layers", "0"}, {"bottom_shell_layers", "0"},
        {"sparse_infill_line_width", "0.1"}, {"internal_solid_infill_line_width", "0.1"},
        {"top_surface_line_width", "0.1"}, {"support_line_width", "0.1"}
    });
    Print print;
    Model model;
    init_print({TestMesh::cube_20x20x20}, print, model, config);
    REQUIRE(print.validate().string.empty());

    config.set_deserialize_strict({{"sparse_infill_density", "15%"}});
    print.apply(model, config);
    REQUIRE(print.validate().opt_key == "sparse_infill_line_width");
}

TEST_CASE("Mixed flow resolves its geometry representative before the runtime map is installed", "[Flow][NozzleVariant][17866]")
{
    auto config = nozzle_width_test_config();
    config.set_num_filaments(4);
    config.set_deserialize_strict({
        {"nozzle_diameter", "0.6,0.6,0.2,0.2"}, {"filament_map", "2,1,2,2"}, {"filament_map_2", "1,0,1,1"},
        {"mixed_filament_definitions", "4,2,1,1,50,0,g,w,m2,d0,o0,u1"}
    });
    Print print;
    Model model;
    init_print({TestMesh::cube_20x20x20}, print, model, config);
    REQUIRE(print.config().filament_map.size() == 4);
    // Members are sorted by physical filament ID, matching resolve_filament_mapping():
    // filament 2 -> nozzle 1 supplies the geometry, not the first recipe entry (4).
    REQUIRE(resolve_flow_nozzle_index(print, 5) == 0);
    PrintRegionConfig region_config = print.get_print_region(0).config();
    region_config.wall_filament.value = 5;
    PrintRegion region(region_config);
    const PrintObject &object = *print.objects().front();
    REQUIRE(region.flow(object, frExternalPerimeter, 0.2).width() == Approx(0.6));
    REQUIRE_THROWS_AS(resolve_flow_nozzle_index(print, 99), SlicingError);
}
