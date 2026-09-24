#include <catch2/catch.hpp>

#include "libslic3r/PresetBundle.hpp"

using namespace Slic3r;

TEST_CASE("Filament appearance follows physical slots and preserves nozzle mapping", "[FilamentColourState]")
{
    PresetBundle bundle;
    bundle.set_num_filaments(3, "#FF0000FF");
    auto& config = bundle.project_config;
    REQUIRE(config.option<ConfigOptionStrings>("filament_multi_colour") != nullptr);
    REQUIRE(config.option<ConfigOptionStrings>("filament_colour_type") != nullptr);
    const std::vector<std::string> appearances{
        "#FF0000FF #00FF00FF", "#FF0000FF", "#FF0000FF #0000FF80"};
    config.option<ConfigOptionStrings>("filament_multi_colour")->values = appearances;
    config.option<ConfigOptionStrings>("filament_colour_type")->values = {"0", "1", "1"};
    config.option<ConfigOptionInts>("filament_map")->values = {2, 1, 2};
    config.option<ConfigOptionInts>("filament_volume_map")->values = {0, 1, 0};

    SECTION("Deleting the middle slot retains the two distinct appearances with the same first colour") {
        bundle.update_num_filaments(1);
        REQUIRE(bundle.filament_presets.size() == 2);
        REQUIRE(config.option<ConfigOptionStrings>("filament_multi_colour")->values ==
                std::vector<std::string>{appearances[0], appearances[2]});
        REQUIRE(config.option<ConfigOptionStrings>("filament_colour_type")->values ==
                std::vector<std::string>{"0", "1"});
        REQUIRE(config.option<ConfigOptionInts>("filament_map")->values == std::vector<int>{2, 2});
        REQUIRE(config.option<ConfigOptionInts>("filament_volume_map")->values == std::vector<int>{0, 0});
    }

    SECTION("Appending a solid slot leaves existing gradient and nozzle assignments intact") {
        bundle.set_num_filaments(4, "#ABCDEF80");
        REQUIRE(config.option<ConfigOptionStrings>("filament_multi_colour")->values ==
                std::vector<std::string>{appearances[0], appearances[1], appearances[2], "#ABCDEF80"});
        REQUIRE(config.option<ConfigOptionStrings>("filament_colour_type")->values ==
                std::vector<std::string>{"0", "1", "1", "1"});
        REQUIRE(config.option<ConfigOptionInts>("filament_map")->values == std::vector<int>{2, 1, 2, 1});
        REQUIRE(config.option<ConfigOptionInts>("filament_volume_map")->values == std::vector<int>{0, 1, 0, 0});
    }

    SECTION("Configuration serialization preserves every RGBA stop and the gradient flag") {
        DynamicPrintConfig restored;
        for (const char* key : {"filament_colour", "filament_multi_colour", "filament_colour_type"}) {
            restored.set_deserialize_strict(key, config.option(key)->serialize());
            REQUIRE(*restored.option(key) == *config.option(key));
        }
    }
}
