#include <catch2/catch.hpp>

#include "libslic3r/BoundingBox.hpp"
#include "libslic3r/GCode/GCodeProcessor.hpp"

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
