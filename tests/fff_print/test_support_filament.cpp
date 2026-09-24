#include <catch2/catch.hpp>

#include "libslic3r/GCode/ToolOrdering.hpp"

using namespace Slic3r;

TEST_CASE("Support filament configuration uses zero-based tools", "[support][tool_ordering]")
{
    LayerTools layer_tools(0.2);
    layer_tools.extruders = {0, 1};

    SECTION("Automatic selection keeps a placeholder without unsigned underflow") {
        // The G-code caller retains the config == 0 flag to select the actual tool.
        REQUIRE(layer_tools.support_filament(0) == 0u);
    }

    SECTION("First and last physical filaments map to emitted tools") {
        REQUIRE(layer_tools.support_filament(1) == 0u);
        REQUIRE(layer_tools.support_filament(2) == 1u);
        REQUIRE(layer_tools.has_extruder(layer_tools.support_filament(1)));
        REQUIRE(layer_tools.has_extruder(layer_tools.support_filament(2)));
    }

    SECTION("Shared support and interface filament uses the same group") {
        const unsigned int support = layer_tools.support_filament(2);
        const unsigned int interface_extruder = layer_tools.support_filament(2);
        REQUIRE(support == interface_extruder);
        REQUIRE(layer_tools.has_extruder(interface_extruder));
    }

    SECTION("Single-filament support stays on the only emitted tool") {
        layer_tools.extruders = {0};
        REQUIRE(layer_tools.has_extruder(layer_tools.support_filament(1)));
    }
}

TEST_CASE("Mixed support filament resolves to a physical tool", "[support][tool_ordering][mixed_filament]")
{
    MixedFilamentManager manager;
    manager.mixed_filaments().emplace_back(); // Virtual filament 3 combines physical filaments 1 and 2.

    LayerTools layer_tools(0.2);
    layer_tools.mixed_mgr = &manager;
    layer_tools.num_physical = 2;
    layer_tools.has_mixed_filaments = true;
    layer_tools.layer_height = 0.2;
    layer_tools.total_layer_count = 100;
    // Object geometry may retain its virtual slot; support must not use this map.
    layer_tools.mixed_filament_resolution[2] = 2;

    SECTION("Physical configuration is not shifted into the next tool") {
        REQUIRE(layer_tools.support_filament(0) == 0u);
        REQUIRE(layer_tools.support_filament(1) == 0u);
        REQUIRE(layer_tools.support_filament(2) == 1u);
    }

    SECTION("Without sublayers support alternates physical tools per layer") {
        layer_tools.enable_mixed_color_sublayer = false;
        for (int layer = 0; layer < 4; ++layer) {
            layer_tools.layer_index = layer;
            const unsigned int expected = static_cast<unsigned int>(layer % 2);
            layer_tools.extruders = {expected};
            REQUIRE(layer_tools.support_filament(3) == expected);
            REQUIRE(layer_tools.has_extruder(layer_tools.support_filament(3)));
        }
    }

    SECTION("With sublayers support follows the physical-tool collection cadence") {
        layer_tools.enable_mixed_color_sublayer = true;
        for (int layer : {0, 49, 50, 99}) {
            layer_tools.layer_index = layer;
            const unsigned int expected = layer < 50 ? 0u : 1u;
            layer_tools.extruders = {expected};
            REQUIRE(layer_tools.support_filament(3) == expected);
            REQUIRE(layer_tools.has_extruder(layer_tools.support_filament(3)));
        }
    }
}
