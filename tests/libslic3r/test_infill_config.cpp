#include <catch2/catch.hpp>
#include "libslic3r/PrintConfig.hpp"

using namespace Slic3r;

TEST_CASE("Legacy infill keys retain their values when loading presets", "[Config][Infill][Legacy]")
{
    for (const char* enabled : {"0", "1"}) {
        DynamicPrintConfig config = DynamicPrintConfig::full_print_config();
        ConfigSubstitutionContext substitutions(ForwardCompatibilitySubstitutionRule::Disable);
        config.set_deserialize("ai_infill", enabled, substitutions);
        CHECK(config.opt_bool("intelligent_infill") == (std::string(enabled) == "1"));
        CHECK(substitutions.unrecogized_keys.empty());
        PrintRegionConfig region;
        region.apply(config, true);
        CHECK(region.intelligent_infill.value == config.opt_bool("intelligent_infill"));
    }
    DynamicPrintConfig config = DynamicPrintConfig::full_print_config();
    ConfigSubstitutionContext substitutions(ForwardCompatibilitySubstitutionRule::Disable);
    config.set_deserialize("field_min_scale", "4%", substitutions);
    config.set_deserialize("field_max_scale_multiplier", "10%", substitutions);
    config.set_deserialize("field_cell_type", "tpmsd", substitutions);
    CHECK(config.option<ConfigOptionPercent>("interior_coefficient")->value == Approx(4.));
    CHECK(config.option<ConfigOptionPercent>("surface_coefficient")->value == Approx(10.));
    CHECK(config.opt_enum<FieldCellType>("cell_type") == FieldCell_SchwarzD);
    CHECK(substitutions.unrecogized_keys.empty());
    config.set_deserialize("different_settings_to_system", "ai_infill;field_min_scale", substitutions);
    const std::string changed_keys = config.option("different_settings_to_system")->serialize();
    CHECK(changed_keys.find("intelligent_infill") != std::string::npos);
    CHECK(changed_keys.find("interior_coefficient") != std::string::npos);
    CHECK(changed_keys.find("ai_infill") == std::string::npos);
}
