#include <catch2/catch.hpp>

#include "libslic3r/BoundingBox.hpp"
#include "libslic3r/GCode/GCodeProcessor.hpp"
#include "libslic3r/GCode/PathologicalSegmentAccelFilter.hpp"

#include <boost/filesystem.hpp>
#include <fstream>
#include <iterator>

using namespace Slic3r;

TEST_CASE("G-code written motion validation follows parsed command order", "[GCodeProcessor][PostProcess]")
{
    GCodeProcessor processor;
    const std::string first_line = "G1 X1 F600\n";
    const std::string second_line = "G1 X2\n";

    processor.process_buffer(first_line + second_line);
    CHECK_NOTHROW(processor.record_written_line(first_line));
    CHECK_NOTHROW(processor.record_written_line(second_line));
}

TEST_CASE("G-code written motion validation rejects unparsed commands", "[GCodeProcessor][PostProcess]")
{
    GCodeProcessor processor;
    REQUIRE_THROWS_WITH(
        processor.record_written_line("G1 X1\n"),
        "G-code motion mapping has more written commands than parsed commands.\n");
}

namespace {
struct ExcludeObjectGCodeFile {
    boost::filesystem::path path = boost::filesystem::temp_directory_path() /
        boost::filesystem::unique_path("exclude-object-%%%%-%%%%.gcode");

    ~ExcludeObjectGCodeFile()
    {
        boost::system::error_code ec;
        boost::filesystem::remove(path, ec);
        boost::filesystem::remove(path.string() + ".postprocess", ec);
    }

    void write(const std::string& gcode) const
    {
        std::ofstream out(path.string(), std::ios::binary);
        out.write(gcode.data(), static_cast<std::streamsize>(gcode.size()));
        REQUIRE(out.good());
    }

    std::string read() const
    {
        std::ifstream in(path.string(), std::ios::binary);
        REQUIRE(in.good());
        return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }
};

ExcludeObjectEndInfo exclude_object_info(int id, const std::string& name)
{
    ExcludeObjectEndInfo info;
    info.id = id;
    info.name = name;
    info.end_pos = Vec3d(1.25, 2.5, 3.75);
    info.has_end_pos = true;
    return info;
}
}

TEST_CASE("Exclude-object directory uses final optimized output offsets", "[GCodeProcessor][PostProcess][ExcludeObject]")
{
    const std::string end_line = "EXCLUDE_OBJECT_END NAME=part\n";
    const std::string missing_pos_line = "EXCLUDE_OBJECT_END NAME=missing_pos\n";
    std::string gcode = "; total filament used [g] = 100000.00\n" + end_line;
    bool marker = true;
    SECTION("last END crosses a copy buffer boundary") {
        const size_t padding_size = 256 * 1024 - 12 - end_line.size() - missing_pos_line.size();
        gcode += std::string(padding_size - 1, ';') + "\n";
    }
    SECTION("missing executable marker appends at EOF") { marker = false; }
    gcode += missing_pos_line;
    gcode += end_line;
    if (marker)
        gcode += "; EXECUTABLE_BLOCK_END\n; CONFIG_BLOCK_START\n; retained tail\n";

    ExcludeObjectGCodeFile file;
    file.write(gcode);
    GCodeProcessor processor;
    PrintConfig config;
    config.gcode_flavor.value = gcfKlipper;
    processor.apply_config(config);
    processor.initialize(file.path.string());
    auto missing_pos = exclude_object_info(8, "missing_pos");
    missing_pos.has_end_pos = false;
    processor.set_exclude_object_end_infos({exclude_object_info(7, "part"), missing_pos,
                                           exclude_object_info(9, "missing_end")});
    processor.process_buffer(gcode);
    processor.finalize(true, 0.f, 0.f, false);

    const std::string output = file.read();
    REQUIRE(output.find("100000.00") == std::string::npos);
    const size_t end_pos = output.rfind(end_line);
    REQUIRE(end_pos != std::string::npos);
    const size_t expected_offset = end_pos + end_line.size() - 1;
    const std::string block = "\n; EXCLUDE_OBJECT_ID_BLOCK_START\n"
        "; EXCLUDE_OBJECT_ID ID=7 NAME=part END_OFFSET=" + std::to_string(expected_offset) +
        " END_POS=(1.250,2.500,3.750)\n; EXCLUDE_OBJECT_ID_BLOCK_END\n";
    REQUIRE(output.find(block) != std::string::npos);
    CHECK(output.find("; EXCLUDE_OBJECT_ID ID=8 ") == std::string::npos);
    CHECK(output.find("; EXCLUDE_OBJECT_ID ID=9 ") == std::string::npos);
    CHECK(output.find("; EXCLUDE_OBJECT_ID_BLOCK_START", output.find(block) + block.size()) == std::string::npos);
    if (marker)
        CHECK(output.find("; EXECUTABLE_BLOCK_END\n" + block + "; CONFIG_BLOCK_START\n") != std::string::npos);
    else
        CHECK(output.substr(output.size() - block.size()) == block);
    std::vector<size_t> line_ends;
    for (size_t i = 0; i < output.size(); ++i)
        if (output[i] == '\n')
            line_ends.push_back(i + 1);
    CHECK(processor.result().lines_ends == line_ends);
}

TEST_CASE("Unterminated END retains legacy eligibility and EOF offset", "[GCodeProcessor][PostProcess][ExcludeObject]")
{
    GCodeProcessor processor;
    processor.set_exclude_object_end_infos({exclude_object_info(7, "part")});
    REQUIRE(processor.build_exclude_object_id_block().empty());
    const std::string line = "EXCLUDE_OBJECT_END NAME=part";
    CHECK_FALSE(processor.process_exclude_object_output_line(line, line.size()));
    CHECK(processor.build_exclude_object_id_block().find(
        "NAME=part END_OFFSET=" + std::to_string(line.size()) + " END_POS=") != std::string::npos);
    processor.reset();
    CHECK(processor.build_exclude_object_id_block().empty());
    CHECK_FALSE(processor.process_exclude_object_output_line("; EXECUTABLE_BLOCK_END", 100));
}

TEST_CASE("Protection output callback observes rewritten byte positions", "[GCodeProcessor][PostProcess][ExcludeObject]")
{
    GCodeProcessor processor;
    processor.set_exclude_object_end_infos({exclude_object_info(7, "part")});
    KlipperSim::SimConfig sim_config;
    sim_config.mcu_total = 8;
    sim_config.nozzle_total = 8;
    sim_config.mcu_pool_total = 8;
    sim_config.nozzle_pool_total = 8;
    sim_config.mcu_reserved_slots = 0;
    sim_config.nozzle_reserved_slots = 0;
    PathologicalSegmentAccelFilter filter(true, gcfKlipper, {}, {}, sim_config);
    bool finished = false;
    filter.set_output_callback([&](std::string_view line, std::string& out, bool final) {
        if (final) {
            finished = true;
        } else {
            REQUIRE(!line.empty());
            REQUIRE(line.back() == '\n');
            line.remove_suffix(1);
            if (processor.process_exclude_object_output_line(line, out.size() - 1))
                out += processor.build_exclude_object_id_block();
        }
    });
    std::string gcode = "G90\nM83\nSET_VELOCITY_LIMIT ACCEL=10000 ACCEL_TO_DECEL=5000\n";
    for (int i = 0; i < 100; ++i)
        gcode += i % 2 ? "G1 X0 Y0 E0.01 F12000\n" : "G1 X0.2 Y0.2 E0.01 F12000\n";
    const std::string end_line = "EXCLUDE_OBJECT_END NAME=part\n";
    gcode += end_line + "; EXECUTABLE_BLOCK_END\n; CONFIG_BLOCK_START\n";
    const size_t original_offset = gcode.find(end_line) + end_line.size() - 1;
    const std::string output = filter.process_gcode(std::move(gcode), true);
    REQUIRE(finished);
    const size_t expected_offset = output.find(end_line) + end_line.size() - 1;
    REQUIRE(expected_offset > original_offset);
    CHECK(output.find("NAME=part END_OFFSET=" + std::to_string(expected_offset) + " END_POS=") != std::string::npos);
    CHECK(output.find("; EXECUTABLE_BLOCK_END\n\n; EXCLUDE_OBJECT_ID_BLOCK_START\n") != std::string::npos);
}
