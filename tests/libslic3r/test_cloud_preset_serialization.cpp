#include <catch2/catch.hpp>
#include "libslic3r/Preset.hpp"
#include "libslic3r/PresetSyncUtils.hpp"
#include <nlohmann/json.hpp>

using namespace Slic3r;

TEST_CASE("Cloud string arrays preserve G-code bytes and element boundaries", "[Config][CloudPreset]")
{
    const std::vector<std::vector<std::string>> cases = {
        {}, {""}, {"", ""}, {"", "G1 X1", ""},
        {";comment"}, {"G1;comment"}, {";first;second"},
        {";filament start gcode\nM104 S210\r\nM117 \"ready\"\n"},
        {R"(;filament start gcode\n{if (position[2] > first_layer_height) }\nM104 S[nozzle_temperature]\n{endif}\n)"},
        {";first\nG1 X1,Y2", ";second\nG92 E0"},
        {" ", "\t", "末尾中文注释;", "C:\\presets\\test", "trailing\\"}
    };
    for (const auto& values : cases) {
        for (const char* key : {"filament_start_gcode", "filament_end_gcode", "compatible_printers"}) {
            INFO(key);
            INFO(nlohmann::json(values).dump());
            const auto json_values = nlohmann::json::parse(nlohmann::json(values).dump());
            DynamicPrintConfig config;
            std::map<std::string, std::string> serialized{{key, print_config_def.serialize_array(key, json_values.get<std::vector<std::string>>())}};
            config.load_string_map(serialized, ForwardCompatibilitySubstitutionRule::Enable);
            const auto* strings = config.option<ConfigOptionStrings>(key);
            REQUIRE(strings != nullptr);
            REQUIRE(strings->values == values);
            // save_to_json uses vserialize() for arrays. Reload those raw elements.
            REQUIRE(nlohmann::json(strings->vserialize()) == json_values);
            ConfigOptionStrings again;
            REQUIRE(again.deserialize(strings->serialize()));
            REQUIRE(again.values == values);
        }
    }
}

TEST_CASE("Cloud numeric arrays retain comma serialization", "[Config][CloudPreset]")
{
    REQUIRE(print_config_def.serialize_array("filament_diameter", {"1.75", "2.85"}) == "1.75,2.85");
    DynamicPrintConfig config;
    std::map<std::string, std::string> serialized{{"filament_diameter", print_config_def.serialize_array("filament_diameter", {"1.75", "2.85"})}};
    config.load_string_map(serialized, ForwardCompatibilitySubstitutionRule::Enable);
    REQUIRE(config.option<ConfigOptionFloats>("filament_diameter")->values == std::vector<double>{1.75, 2.85});
}

TEST_CASE("Normalized cloud G-code survives comments and multiple filaments", "[Config][CloudPreset]")
{
    const std::vector<std::string> gcode{";first\nG1 X1", ";second\nG1 X2"};
    DynamicPrintConfig config;
    std::map<std::string, std::string> serialized{{"filament_start_gcode", print_config_def.serialize_array("filament_start_gcode", gcode)},
                                                {"filament_diameter", "1.75,1.75"}};
    config.load_string_map(serialized, ForwardCompatibilitySubstitutionRule::Enable);
    Preset::normalize(config);
    REQUIRE(config.option<ConfigOptionStrings>("filament_start_gcode")->values == gcode);
}

TEST_CASE("Equal cloud revision repairs lost G-code without replacing other settings", "[Config][CloudPreset]")
{
    PresetCollection collection(Preset::TYPE_FILAMENT, Preset::filament_options(),
                                static_cast<const PrintRegionConfig&>(FullPrintConfig::defaults()));
    DynamicPrintConfig original = collection.default_preset().config;
    original.option<ConfigOptionStrings>("filament_start_gcode")->values = {""};
    original.option<ConfigOptionStrings>("filament_end_gcode")->values = {";local end"};
    Preset& preset = collection.load_preset("", "Cloud G-code repair", original, false);
    preset.updated_time = 100;
    preset.setting_id = "cloud-id";
    preset.sync_info.clear();
    preset.is_dirty = false;
    const std::vector<std::string> cloud_start{";cloud start\nG92 E0"};
    std::map<std::string, std::string> values{
        {"version", "1.0.0"}, {"setting_id", "cloud-id"}, {"updated_time", "100"},
        {"user_id", "test-user"}, {"base_id", ""}, {"inherits", ""},
        {"filament_start_gcode", print_config_def.serialize_array("filament_start_gcode", cloud_start)},
        {"filament_end_gcode", print_config_def.serialize_array("filament_end_gcode", {";cloud end"})}
    };
    bool should_repair = true;
    SECTION("Repair a clean preset") {}
    SECTION("Preserve pending upload") { preset.sync_info = "update"; should_repair = false; }
    SECTION("Preserve pending save") { preset.sync_info = "save"; should_repair = false; }
    SECTION("Preserve dirty preset") { preset.is_dirty = true; should_repair = false; }
    const std::string previous_state = preset.sync_info;
    PresetsConfigSubstitutions substitutions;
    collection.load_user_preset(preset.name, values, substitutions, ForwardCompatibilitySubstitutionRule::Enable);
    if (!should_repair) {
        REQUIRE(preset.config.diff(original).empty());
        REQUIRE(preset.sync_info == previous_state);
        return;
    }
    REQUIRE(preset.config.option<ConfigOptionStrings>("filament_start_gcode")->values == cloud_start);
    REQUIRE(preset.config.diff(original) == std::vector<std::string>{"filament_start_gcode"});
    REQUIRE(preset.updated_time == 100);
    REQUIRE(preset.sync_info == "save");
    preset.sync_info.clear(); // Simulate the existing save stage completing.
    collection.load_user_preset(preset.name, values, substitutions, ForwardCompatibilitySubstitutionRule::Enable);
    REQUIRE(preset.sync_info.empty()); // No repeated repair after synchronization.
}

TEST_CASE("G-code recovery protects local edits and requires the same cloud revision", "[Config][CloudPreset]")
{
    using namespace PresetSyncUtils;
    REQUIRE(can_repair_gcode(100, 100, "id", "id", "", false));
    REQUIRE_FALSE(can_repair_gcode(0, 0, "id", "id", "", false));
    REQUIRE_FALSE(can_repair_gcode(99, 100, "id", "id", "", false));
    REQUIRE_FALSE(can_repair_gcode(101, 100, "id", "id", "", false));
    REQUIRE_FALSE(can_repair_gcode(100, 100, "other", "id", "", false));
    REQUIRE_FALSE(can_repair_gcode(100, 100, "id", "id", "", true));
    for (const char* state : {"update", "create", "hold", "save"})
        REQUIRE_FALSE(can_repair_gcode(100, 100, "id", "id", state, false));
    REQUIRE(has_lost_gcode({""}, {";cloud"}));
    REQUIRE_FALSE(has_lost_gcode({";local"}, {";cloud"}));
    REQUIRE_FALSE(has_lost_gcode({""}, {""}));
    REQUIRE_FALSE(has_lost_gcode({}, {";cloud"}));
    REQUIRE_FALSE(has_lost_gcode({""}, {";cloud", ";second"}));
}
