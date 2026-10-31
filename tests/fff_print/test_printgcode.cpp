#include <catch2/catch.hpp>

#include "libslic3r/libslic3r.h"
#include "libslic3r/GCodeReader.hpp"
#include "libslic3r/ModelObject.hpp"
#include "libslic3r/ModelVolume.hpp"
#include "libslic3r/BoundingBox.hpp"

#include "test_data.hpp"

#include <algorithm>
#include <boost/regex.hpp>

using namespace Slic3r;
using namespace Slic3r::Test;

boost::regex perimeters_regex("G1 X[-0-9.]* Y[-0-9.]* E[-0-9.]* ; perimeter");
boost::regex infill_regex("G1 X[-0-9.]* Y[-0-9.]* E[-0-9.]* ; infill");
boost::regex skirt_regex("G1 X[-0-9.]* Y[-0-9.]* E[-0-9.]* ; skirt");

SCENARIO( "PrintGCode basic functionality", "[PrintGCode]") {
    GIVEN("A default configuration and a print test object") {
        WHEN("the output is executed with no support material") {
            Slic3r::Print print;
            Slic3r::Model model;
            Slic3r::Test::init_print({TestMesh::cube_20x20x20}, print, model, {
                { "layer_height",					0.2 },
                { "first_layer_height",				0.2 },
                { "first_layer_extrusion_width",	0 },
                { "gcode_comments",					true },
                { "start_gcode",					"" }
                });
            std::string gcode = Slic3r::Test::gcode(print);
            THEN("Some text output is generated.") {
                REQUIRE(gcode.size() > 0);
            }
            THEN("Exported text contains slic3r version") {
                REQUIRE(gcode.find(SLIC3R_VERSION) != std::string::npos);
            }
            //THEN("Exported text contains git commit id") {
            //    REQUIRE(gcode.find("; Git Commit") != std::string::npos);
            //    REQUIRE(gcode.find(SLIC3R_BUILD_ID) != std::string::npos);
            //}
            THEN("Exported text contains extrusion statistics.") {
                REQUIRE(gcode.find("; external perimeters extrusion width") != std::string::npos);
                REQUIRE(gcode.find("; perimeters extrusion width") != std::string::npos);
                REQUIRE(gcode.find("; infill extrusion width") != std::string::npos);
                REQUIRE(gcode.find("; solid infill extrusion width") != std::string::npos);
                REQUIRE(gcode.find("; top infill extrusion width") != std::string::npos);
                REQUIRE(gcode.find("; support material extrusion width") == std::string::npos);
                REQUIRE(gcode.find("; first layer extrusion width") == std::string::npos);
            }
            THEN("Exported text does not contain cooling markers (they were consumed)") {
                REQUIRE(gcode.find(";_EXTRUDE_SET_SPEED") == std::string::npos);
            }

            THEN("GCode preamble is emitted.") {
                REQUIRE(gcode.find("G21 ; set units to millimeters") != std::string::npos);
            }

            THEN("Config options emitted for print config, default region config, default object config") {
                REQUIRE(gcode.find("; first_layer_temperature") != std::string::npos);
                REQUIRE(gcode.find("; layer_height") != std::string::npos);
                REQUIRE(gcode.find("; fill_density") != std::string::npos);
            }
            THEN("Infill is emitted.") {
                boost::smatch has_match;
                REQUIRE(boost::regex_search(gcode, has_match, infill_regex));
            }
            THEN("Perimeters are emitted.") {
				boost::smatch has_match;
                REQUIRE(boost::regex_search(gcode, has_match, perimeters_regex));
            }
            THEN("Skirt is emitted.") {
                boost::smatch has_match;
                REQUIRE(boost::regex_search(gcode, has_match, skirt_regex));
            }
            THEN("final Z height is 20mm") {
                double final_z = 0.0;
                GCodeReader reader;
                reader.apply_config(print.config());
                reader.parse_buffer(gcode, [&final_z] (GCodeReader& self, const GCodeReader::GCodeLine& line) {
                    final_z = std::max<double>(final_z, static_cast<double>(self.z())); // record the highest Z point we reach
                });
                REQUIRE(final_z == Approx(20.));
            }
        }
        WHEN("output is executed with complete objects and two differently-sized meshes") {
            Slic3r::Print print;
            Slic3r::Model model;
            Slic3r::Test::init_print({TestMesh::cube_20x20x20,TestMesh::cube_20x20x20}, print, model, {
                { "first_layer_extrusion_width",    0 },
                { "first_layer_height",             0.3 },
                { "layer_height",                   0.2 },
                { "support_material",               false },
                { "raft_layers",                    0 },
                { "complete_objects",               true },
                { "gcode_comments",                 true },
                { "between_objects_gcode",          "; between-object-gcode" }
                });
            std::string gcode = Slic3r::Test::gcode(print);
            THEN("Some text output is generated.") {
                REQUIRE(gcode.size() > 0);
            }
            THEN("Infill is emitted.") {
                boost::smatch has_match;
                REQUIRE(boost::regex_search(gcode, has_match, infill_regex));
            }
            THEN("Perimeters are emitted.") {
                boost::smatch has_match;
                REQUIRE(boost::regex_search(gcode, has_match, perimeters_regex));
            }
            THEN("Skirt is emitted.") {
                boost::smatch has_match;
                REQUIRE(boost::regex_search(gcode, has_match, skirt_regex));
            }
            THEN("Between-object-gcode is emitted.") {
                REQUIRE(gcode.find("; between-object-gcode") != std::string::npos);
            }
            THEN("final Z height is 20.1mm") {
                double final_z = 0.0;
                GCodeReader reader;
                reader.apply_config(print.config());
                reader.parse_buffer(gcode, [&final_z] (GCodeReader& self, const GCodeReader::GCodeLine& line) {
                    final_z = std::max(final_z, static_cast<double>(self.z())); // record the highest Z point we reach
                });
                REQUIRE(final_z == Approx(20.1));
            }
            THEN("Z height resets on object change") {
                double final_z = 0.0;
                bool reset = false;
                GCodeReader reader;
                reader.apply_config(print.config());
                reader.parse_buffer(gcode, [&final_z, &reset] (GCodeReader& self, const GCodeReader::GCodeLine& line) {
                    if (final_z > 0 && std::abs(self.z() - 0.3) < 0.01 ) { // saw higher Z before this, now it's lower
                        reset = true;
                    } else {
                        final_z = std::max(final_z, static_cast<double>(self.z())); // record the highest Z point we reach
                    }
                });
                REQUIRE(reset == true);
            }
            THEN("Shorter object is printed before taller object.") {
                double final_z = 0.0;
                bool reset = false;
                GCodeReader reader;
                reader.apply_config(print.config());
                reader.parse_buffer(gcode, [&final_z, &reset] (GCodeReader& self, const GCodeReader::GCodeLine& line) {
                    if (final_z > 0 && std::abs(self.z() - 0.3) < 0.01 ) { 
                        reset = (final_z > 20.0);
                    } else {
                        final_z = std::max(final_z, static_cast<double>(self.z())); // record the highest Z point we reach
                    }
                });
                REQUIRE(reset == true);
            }
        }
        WHEN("the output is executed with support material") {
            std::string gcode = ::Test::slice({TestMesh::cube_20x20x20}, {
                { "first_layer_extrusion_width",    0 },
                { "support_material",               true },
                { "raft_layers",                    3 },
                { "gcode_comments",                 true }
                });
            THEN("Some text output is generated.") {
                REQUIRE(gcode.size() > 0);
            }
            THEN("Exported text contains extrusion statistics.") {
                REQUIRE(gcode.find("; external perimeters extrusion width") != std::string::npos);
                REQUIRE(gcode.find("; perimeters extrusion width") != std::string::npos);
                REQUIRE(gcode.find("; infill extrusion width") != std::string::npos);
                REQUIRE(gcode.find("; solid infill extrusion width") != std::string::npos);
                REQUIRE(gcode.find("; top infill extrusion width") != std::string::npos);
                REQUIRE(gcode.find("; support material extrusion width") != std::string::npos);
                REQUIRE(gcode.find("; first layer extrusion width") == std::string::npos);
            }
            THEN("Raft is emitted.") {
                REQUIRE(gcode.find("; raft") != std::string::npos);
            }
        }
        WHEN("the output is executed with a separate first layer extrusion width") {
			std::string gcode = ::Test::slice({ TestMesh::cube_20x20x20 }, {
                { "first_layer_extrusion_width", "0.5" }
                });
            THEN("Some text output is generated.") {
                REQUIRE(gcode.size() > 0);
            }
            THEN("Exported text contains extrusion statistics.") {
                REQUIRE(gcode.find("; external perimeters extrusion width") != std::string::npos);
                REQUIRE(gcode.find("; perimeters extrusion width") != std::string::npos);
                REQUIRE(gcode.find("; infill extrusion width") != std::string::npos);
                REQUIRE(gcode.find("; solid infill extrusion width") != std::string::npos);
                REQUIRE(gcode.find("; top infill extrusion width") != std::string::npos);
                REQUIRE(gcode.find("; support material extrusion width") == std::string::npos);
                REQUIRE(gcode.find("; first layer extrusion width") != std::string::npos);
            }
        }
        WHEN("Cooling is enabled and the fan is disabled.") {
			std::string gcode = ::Test::slice({ TestMesh::cube_20x20x20 }, {
				{ "cooling",                    true },
                { "disable_fan_first_layers",   5 }
                });
            THEN("GCode to disable fan is emitted."){
                REQUIRE(gcode.find("M107") != std::string::npos);
            }
        }
        WHEN("end_gcode exists with layer_num and layer_z") {
			std::string gcode = ::Test::slice({ TestMesh::cube_20x20x20 }, {
				{ "end_gcode",              "; Layer_num [layer_num]\n; Layer_z [layer_z]" },
                { "layer_height",           0.1 },
                { "first_layer_height",     0.1 }
                });
            THEN("layer_num and layer_z are processed in the end gcode") {
                REQUIRE(gcode.find("; Layer_num 199") != std::string::npos);
                REQUIRE(gcode.find("; Layer_z 20") != std::string::npos);
            }
        }
        WHEN("current_extruder exists in start_gcode") {
            {
				std::string gcode = ::Test::slice({ TestMesh::cube_20x20x20 }, {
					{ "start_gcode", "; Extruder [current_extruder]" }
                });
                THEN("current_extruder is processed in the start gcode and set for first extruder") {
                    REQUIRE(gcode.find("; Extruder 0") != std::string::npos);
                }
            }
			{
                DynamicPrintConfig config = DynamicPrintConfig::full_print_config();
                config.set_num_extruders(4);
                config.set_deserialize_strict({
                    { "start_gcode",                    "; Extruder [current_extruder]" },
                    { "infill_extruder",                2 },
                    { "solid_infill_extruder",          2 },
                    { "perimeter_extruder",             2 },
                    { "support_material_extruder",      2 },
                    { "support_material_interface_extruder", 2 }
                });
                std::string gcode = Slic3r::Test::slice({TestMesh::cube_20x20x20}, config);
                THEN("current_extruder is processed in the start gcode and set for second extruder") {
                    REQUIRE(gcode.find("; Extruder 1") != std::string::npos);
                }
            }
        }

        WHEN("layer_num represents the layer's index from z=0") {
			std::string gcode = ::Test::slice({ TestMesh::cube_20x20x20, TestMesh::cube_20x20x20 }, {
				{ "complete_objects",               true },
                { "gcode_comments",                 true },
                { "layer_gcode",                    ";Layer:[layer_num] ([layer_z] mm)" },
                { "layer_height",                   0.1 },
                { "first_layer_height",             0.1 }
                });
			// End of the 1st object.
            std::string token = ";Layer:199 ";
			size_t pos = gcode.find(token);
			THEN("First and second object last layer is emitted") {
				// First object
				REQUIRE(pos != std::string::npos);
				pos += token.size();
				REQUIRE(pos < gcode.size());
				double z = 0;
				REQUIRE((sscanf(gcode.data() + pos, "(%lf mm)", &z) == 1));
				REQUIRE(z == Approx(20.));
				// Second object
				pos = gcode.find(";Layer:399 ", pos);
				REQUIRE(pos != std::string::npos);
				pos += token.size();
				REQUIRE(pos < gcode.size());
				REQUIRE((sscanf(gcode.data() + pos, "(%lf mm)", &z) == 1));
				REQUIRE(z == Approx(20.));
			}
        }
    }
}


TEST_CASE("Smooth timelapse keeps automatic no-tower infill material changes", "[PrintGCode][timelapse][no_wipe_tower]")
{
    DynamicPrintConfig config = DynamicPrintConfig::full_print_config();
    config.set_num_filaments(2);
    config.set_deserialize_strict({
        {"printer_model", "SPARKX i7"},
        {"enable_prime_tower", "0"},
        {"timelapse_type", "1"},
        {"multicolor_method", "1"},
        {"single_extruder_multi_material", "1"},
        {"flush_into_infill", "0"},
        {"flush_into_objects", "0"},
        {"flush_into_support", "0"},
        {"flush_into_skeleton", "0"},
        {"flush_into_solid_skeleton", "0"},
        {"sparse_infill_pattern", "locked-zag"},
        {"sparse_infill_density", "10%"},
        {"layer_height", "0.2"},
        {"initial_layer_print_height", "0.2"},
        {"bottom_shell_layers", "1"},
        {"top_shell_layers", "1"},
        {"top_shell_thickness", "0"},
        {"bottom_shell_thickness", "0"},
        {"bottom_fill_flush_layers", "0"},
        {"top_fill_flush_layers", "0"},
        {"wall_loops", "2"},
        {"skirt_loops", "0"},
        {"brim_type", "no_brim"},
        {"flush_volumes_matrix", "0,30,30,0"},
        {"filament_minimal_purge_on_wipe_tower", "0,0"},
        {"prime_tower_width", "20"},
        {"prime_tower_brim_width", "0"},
        {"wipe_tower_x", "10"},
        {"wipe_tower_y", "10"},
        // These prime-tower preferences must not change the camera tower's tool or height.
        {"wipe_tower_filament", "1"},
        {"wipe_tower_no_sparse_layers", "1"},
        {"machine_start_gcode", "G90\nM83\n"},
        {"machine_end_gcode", ""},
        {"change_filament_gcode", "; OUTSIDE_PURGE\nT[next_extruder]\n"},
        {"time_lapse_gcode", "; CAMERA_SHOT\nG1 X267 Y110 F24000\n"},
        {"filament_start_gcode", ";,;"},
        {"filament_end_gcode", ";,;"}
    });

    Print print;
    Model model;
    for (int tool = 1; tool <= 2; ++tool) {
        ModelObject* object = model.add_object();
        object->name = "timelapse_" + std::to_string(tool);
        TriangleMesh mesh = make_cube(30., 30., 3.);
        mesh.translate(Vec3f(60.f + 45.f * (tool - 1), 60.f, 0.f));
        object->add_volume(std::move(mesh));
        object->add_instance();
        object->config.set_key_value("extruder", new ConfigOptionInt(tool));
        object->volumes.front()->config.set_key_value("extruder", new ConfigOptionInt(tool));
    }
    print.apply(model, config);
    print.set_status_silent();

    REQUIRE_FALSE(print.has_wipe_tower());
    REQUIRE(print.needs_timelapse_tower());
    const std::string output = Test::gcode(print);
    CHECK_FALSE(print.config().enable_prime_tower.value);
    REQUIRE(output.find("; NO_WIPE_TOWER_NO_PURGE_") != std::string::npos);
    REQUIRE(output.find("; CAMERA_SHOT\n") != std::string::npos);
    REQUIRE(output.find("; CP EMPTY GRID START") != std::string::npos);
    CHECK(output.find("[change_filament_gcode]") == std::string::npos);
    CHECK(output.find("[deretraction") == std::string::npos);
    REQUIRE_FALSE(print.first_layer_wipe_tower_corners().empty());
    REQUIRE_FALSE(print.wipe_tower_data().tool_changes.empty());
    CHECK(print.wipe_tower_data().priming->empty());
    CHECK(print.wipe_tower_data().final_purge->gcode.empty());
    CHECK(print.wipe_tower_data().number_of_toolchanges == 0);
    size_t tower_layer = 0;
    for (const LayerTools& layer : print.tool_ordering()) {
        if (!layer.has_wipe_tower)
            continue;
        REQUIRE(tower_layer < print.wipe_tower_data().tool_changes.size());
        const auto& paths = print.wipe_tower_data().tool_changes[tower_layer++];
        REQUIRE(paths.size() == 1);
        CHECK(paths.front().initial_tool == paths.front().new_tool);
        CHECK(paths.front().new_tool == int(layer.extruders.back()));
        CHECK(paths.front().purge_volume == 0.f);
        CHECK_FALSE(paths.front().is_tool_change);
        CHECK(paths.front().print_z == Approx(layer.print_z));
    }
    CHECK(tower_layer == print.wipe_tower_data().tool_changes.size());

    // Check the emitted order on every complete model layer: both model tools,
    // then one photo and one camera-tower path, without a tower toolchange.
    size_t layer_begin = output.find(";LAYER_CHANGE\n");
    size_t camera_count = 0;
    while (layer_begin != std::string::npos) {
        const size_t next = output.find(";LAYER_CHANGE\n", layer_begin + 1);
        const size_t block_end = next == std::string::npos ? output.find("; EXECUTABLE_BLOCK_END", layer_begin) : next;
        const std::string block = output.substr(layer_begin, block_end - layer_begin);
        const size_t shot = block.find("; CAMERA_SHOT\n");
        REQUIRE(shot != std::string::npos);
        CHECK(block.find("; CAMERA_SHOT\n", shot + 1) == std::string::npos);
        const size_t tower = block.find("; CP EMPTY GRID START");
        REQUIRE(tower != std::string::npos);
        CHECK(shot < tower);
        CHECK(block.find("\nT", shot) == std::string::npos);
        ++camera_count;
        layer_begin = next;
    }
    CHECK(camera_count == tower_layer);
}
