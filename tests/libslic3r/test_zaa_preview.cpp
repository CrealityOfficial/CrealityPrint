#include <catch2/catch.hpp>

#include "libslic3r/BoundingBox.hpp"
#include "libslic3r/GCode.hpp"
#include "libslic3r/GCode/CoolingBuffer.hpp"
#include "libslic3r/GCode/GCodeProcessor.hpp"

using namespace Slic3r;

namespace {
std::string cool_preview_gcode(const std::string& input, bool parse = true)
{
    GCode generator;
    generator.apply_print_config(PrintConfig());
    generator.writer().set_extruders({ 0 });
    generator.writer().set_extruder(0);
    GCodeEditor editor(generator);
    editor.set_current_extruder(0);
    editor.reset(Vec3d(0.0, 0.0, 0.2));
    std::vector<PerExtruderAdjustments> adjustments;
    std::string parsed = parse ? editor.process_layer(std::string(input), 0, adjustments, {}, true, false) : input;
    return editor.write_layer_gcode(parsed, 0, 60.0f, adjustments);
}

std::string preview_gcode(const std::string& first_z, const std::string& second_z)
{
    return "G90\nM83\n;LAYER_CHANGE\n" + first_z +
        "\n;HEIGHT:0.2\nG1 Z0.2 F600\nG1 X1 Y0 E0.1\n"
        ";LAYER_CHANGE\n" + second_z +
        "\n;HEIGHT:0.2\nG1 Z0.4\n; ZAA_BEGIN layer=1 role=ExternalPerimeter\n"
        "G1 X2 Y0 Z0.35 E0.1\nG1 X3 Y0 Z0.45 E0.1\n; ZAA_END\n";
}

void check_preview_layers(const std::string& gcode, size_t count)
{
    GCodeProcessor processor;
    processor.apply_config(PrintConfig());
    processor.initialize("zaa-preview-test.gcode");
    processor.process_buffer(gcode);
    processor.finalize(false);
    const auto& layers = processor.get_result().zaa_layers;
    REQUIRE(layers.size() == count);
    if (count == 2) {
        CHECK(layers[0].first == Approx(0.2));
        CHECK(layers[1].first == Approx(0.4));
        CHECK(layers[0].second.first <= layers[0].second.second);
        CHECK(layers[0].second.second <= layers[1].second.first);
        CHECK(layers[1].second.first < layers[1].second.second);
    }
}
}

TEST_CASE("Cooling preserves nominal layer heights for ZAA preview", "[ZaaPreview][CoolingBuffer]")
{
    const std::string input = preview_gcode(";Z:0.2", ";Z:0.4");
    const std::string cooled = cool_preview_gcode(input);
    REQUIRE(cooled.find(";Z:0.2\n") != std::string::npos);
    REQUIRE(cooled.find(";Z:0.4\n") != std::string::npos);
    REQUIRE(cooled.find("G1 X2 Y0 Z0.35 E0.1\nG1 X3 Y0 Z0.45 E0.1\n") != std::string::npos);
    check_preview_layers(cooled, 2);
}

TEST_CASE("Cooling Z cleanup preserves metadata and non-motion commands", "[ZaaPreview][CoolingBuffer]")
{
    const std::string metadata =
        ";Z:0.2\n; Z_HEIGHT: 0.2\n; comment Z0\n"
        "SET_GCODE_OFFSET Z=0\nG92 Z0\n";
    const std::string cooled = cool_preview_gcode(metadata + "G1 X1 Z0 F600 ; keep Z0\n");
    CHECK(cooled.find(metadata) != std::string::npos);
    CHECK(cooled.find("G1 X1  F600 ; keep Z0\n") != std::string::npos);
}

TEST_CASE("ZAA protected zero Z motion survives cooling", "[ZaaPreview][CoolingBuffer]")
{
    const std::string protected_moves =
        "; ZAA_BEGIN layer=0 role=ExternalPerimeter\n"
        "G1 X1 Y0 Z0 E0.1 F600\nG1 X2 Y0 Z0.2 E0.1\n; ZAA_END\n";
    CHECK(cool_preview_gcode(protected_moves).find(protected_moves) != std::string::npos);
}

TEST_CASE("ZAA preview accepts legacy damaged heights at the layer boundary", "[ZaaPreview]")
{
    SECTION("Creality dialect") {
        check_preview_layers(preview_gcode(";:0.2", ";:0.4"), 2);
    }
    SECTION("Alternate dialect") {
        check_preview_layers(preview_gcode("; _HEIGHT: 0.2", "; _HEIGHT: 0.4"), 2);
    }
    SECTION("Normal labels remain supported") {
        check_preview_layers(preview_gcode("; Z_HEIGHT: 0.2", "; Z_HEIGHT: 0.4"), 2);
    }
}

TEST_CASE("Legacy ZAA height compatibility rejects unrelated comments", "[ZaaPreview]")
{
    SECTION("Comment later in the layer") {
        check_preview_layers(preview_gcode(";unrelated\n;:0.2", ";:0.4"), 0);
    }
    SECTION("Comment after a move") {
        check_preview_layers(preview_gcode("G1 Z0.2\n;:0.2", ";:0.4"), 0);
    }
    SECTION("Malformed height") {
        check_preview_layers(preview_gcode(";:oops", ";:0.4"), 0);
    }
    SECTION("Nonfinite height") {
        check_preview_layers(preview_gcode(";:nan", ";:0.4"), 0);
    }
    SECTION("Z hop alone must not enable ZAA grouping") {
        check_preview_layers("G90\nM83\n;LAYER_CHANGE\n;:0.2\n;HEIGHT:0.2\n"
            "G1 Z0.2 F600\nG1 X1 E0.1\nG1 Z0.6\nG1 X2\nG1 Z0.2\nG1 X3 E0.1\n", 0);
    }
}

TEST_CASE("Cooling Z cleanup requires a complete finite numeric parameter", "[ZaaPreview][CoolingBuffer]")
{
    for (const std::string line : { "G1 X1 Z F600", "G1 X1 Z:0.2 F600", "G1 X1 Z0oops F600",
                                   "G1 X1 Znan F600", "G1 X1 Z+-0 F600", "G1 X1 Z0.2 F600" }) {
        INFO(line);
        CHECK(cool_preview_gcode(line + "\n", false).find(line + "\n") != std::string::npos);
    }
    CHECK(cool_preview_gcode("G1 X1 Z+0 F600\n", false).find("G1 X1  F600\n") != std::string::npos);
    CHECK(cool_preview_gcode("G1 X1 Z-0 F600\n", false).find("G1 X1  F600\n") != std::string::npos);
}

TEST_CASE("Cooling tracks motion Z without reading comment or macro payloads", "[ZaaPreview][CoolingBuffer]")
{
    GCode generator;
    generator.apply_print_config(PrintConfig());
    generator.writer().set_extruders({ 0 });
    generator.writer().set_extruder(0);
    GCodeEditor editor(generator);
    editor.set_current_extruder(0);
    // Start below zero so comment-only Z values cannot enable zero-Z cleanup.
    editor.reset(Vec3d(0.0, 0.0, -0.2));
    std::vector<PerExtruderAdjustments> adjustments;
    editor.write_layer_gcode("; calibration Z9\nSET_GCODE_OFFSET Z9\nG1 X1 ; Z9\n", 0, 60.0f, adjustments);
    CHECK(editor.write_layer_gcode("G1 X2 Z0\n", 0, 60.0f, adjustments).find("G1 X2 Z0\n") != std::string::npos);
    editor.write_layer_gcode("G1 Z0.2\n", 0, 60.0f, adjustments);
    CHECK(editor.write_layer_gcode("G1 X3 Z0\n", 0, 60.0f, adjustments).find("G1 X3 \n") != std::string::npos);
}
