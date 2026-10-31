#include <catch2/catch.hpp>
#include <boost/filesystem.hpp>
#include "libslic3r/PresetBundle.hpp"

using namespace Slic3r;

TEST_CASE("Legacy project nozzle indices do not discard single-row overrides", "[Config][18082]")
{
    const bool filament = GENERATE(false, true);
    const auto type = filament ? Preset::TYPE_FILAMENT : Preset::TYPE_PRINT;
    const char* key = filament ? "filament_max_volumetric_speed" : "initial_layer_speed";
    const char* selector = filament ? "filament_nozzle_variant" : "print_nozzle_variant";
    DynamicPrintConfig source = DynamicPrintConfig::full_print_config();
    source.option<ConfigOptionInts>(selector, true)->values = {0};
    source.option<ConfigOptionFloats>(key)->values = {40.};
    DynamicPrintConfig project(source);
    project.option<ConfigOptionInts>(selector)->values = {2};
    project.option<ConfigOptionFloats>(key)->values = {41.};
    project.option<ConfigOptionFloats>("nozzle_diameter")->values = {0.6};
    project.option<ConfigOptionStrings>("nozzle_variant_ids", true)->values.clear();

    bool preserve = true;
    SECTION("Legacy single nozzle") {}
    SECTION("Structured nozzle identities must still match") {
        project.option<ConfigOptionStrings>("nozzle_variant_ids")->values = {"N06"};
        preserve = false;
    }
    SECTION("Unknown printer topology must not enable legacy fallback") {
        project.erase("nozzle_diameter");
        preserve = false;
    }
    SECTION("Multiple physical nozzles must not enable legacy fallback") {
        project.option<ConfigOptionFloats>("nozzle_diameter")->values = {0.4, 0.6};
        preserve = false;
    }
    SECTION("Flow type mismatch must still fall back") {
        project.option<ConfigOptionStrings>(filament ? "filament_extruder_variant" : "print_extruder_variant")->values = {"Direct Drive High Flow"};
        preserve = false;
    }
    DynamicPrintConfig target(project);
    restore_project_variant_overrides(type, target, project, source, {key, selector});
    REQUIRE(target.option<ConfigOptionFloats>(key)->values == std::vector<double>{preserve ? 41. : 40.});
    REQUIRE(target.option<ConfigOptionInts>(selector)->values == std::vector<int>{0});
}

TEST_CASE("External process import retains legacy project speed", "[Config][18082]")
{
    PresetBundle bundle;
    DynamicPrintConfig base(bundle.prints.default_preset().config);
    base.option<ConfigOptionFloats>("initial_layer_speed")->values = {40.};
    base.option<ConfigOptionInts>("print_nozzle_variant", true)->values = {0};
    auto& preset = bundle.prints.load_preset("", "K2 process", base, false);
    preset.is_system = true;
    DynamicPrintConfig project(base);
    project.option<ConfigOptionFloats>("nozzle_diameter", true)->values = {0.6};
    project.option<ConfigOptionInts>("print_nozzle_variant")->values = {2};
    project.option<ConfigOptionFloats>("initial_layer_speed")->values = {41.};
    const bool inherited = GENERATE(false, true);
    project.option<ConfigOptionString>("inherits", true)->value = inherited ? "K2 process" : "";
    bundle.prints.load_external_preset("c2.3mf", "c2.3mf", inherited ? "custom" : "K2 process",
        project, {"initial_layer_speed"}, PresetCollection::LoadAndSelect::Always);
    REQUIRE(bundle.prints.get_edited_preset().config.option<ConfigOptionFloats>("initial_layer_speed")->values == std::vector<double>{41.});
    REQUIRE_FALSE(bundle.prints.get_edited_preset().config.has("nozzle_diameter"));
}

TEST_CASE("Equivalent project filament slots reuse one imported preset", "[Config][18462]")
{
    PresetBundle bundle;
    const std::string filament_name = "Hyper PLA @Creality K2 Plus 0.4 nozzle";
    DynamicPrintConfig system_config(bundle.filaments.default_preset().config);
    system_config.option<ConfigOptionFloats>("filament_diameter")->values = {1.75};
    auto &system = bundle.filaments.load_preset("", filament_name, system_config, false);
    system.is_system = true;

    DynamicPrintConfig project = DynamicPrintConfig::full_print_config();
    project.option<ConfigOptionStrings>("filament_colour")->values = {"#FFFFFF", "#000000"};
    project.option<ConfigOptionStrings>("filament_settings_id")->values = {filament_name, filament_name};
    project.option<ConfigOptionStrings>("filament_ids")->values = {"01001", "01001"};
    project.option<ConfigOptionFloats>("filament_diameter")->values = {1.8, 1.8};
    project.option<ConfigOptionFloats>("pellet_flow_coefficient")->values = {0.392975, 0.392975};
    project.option<ConfigOptionStrings>("different_settings_to_system")->values = {
        "", "filament_diameter;pellet_flow_coefficient",
        "filament_diameter;pellet_flow_coefficient", ""};

    bundle.load_config_model("same-filaments.3mf", project);

    REQUIRE(bundle.filament_presets.size() == 2);
    REQUIRE(bundle.filament_presets[0] == filament_name);
    REQUIRE(bundle.filament_presets[1] == filament_name);
    REQUIRE(bundle.filaments.get_edited_preset().is_dirty);
    REQUIRE(bundle.filaments.get_project_embedded_presets().empty());
    REQUIRE(bundle.project_config.option<ConfigOptionStrings>("filament_colour")->values ==
            std::vector<std::string>{"#FFFFFF", "#000000"});
}

TEST_CASE("Different project filament overrides remain separate presets", "[Config][18462]")
{
    PresetBundle bundle;
    const std::string filament_name = "Hyper PLA @Creality K2 Plus 0.4 nozzle";
    DynamicPrintConfig system_config(bundle.filaments.default_preset().config);
    system_config.option<ConfigOptionFloats>("filament_diameter")->values = {1.75};
    auto &system = bundle.filaments.load_preset("", filament_name, system_config, false);
    system.is_system = true;

    DynamicPrintConfig project = DynamicPrintConfig::full_print_config();
    project.option<ConfigOptionStrings>("filament_colour")->values = {"#FFFFFF", "#000000"};
    project.option<ConfigOptionStrings>("filament_settings_id")->values = {filament_name, filament_name};
    project.option<ConfigOptionStrings>("filament_ids")->values = {"01001", "01001"};
    project.option<ConfigOptionFloats>("filament_diameter")->values = {1.8, 1.9};
    project.option<ConfigOptionStrings>("different_settings_to_system")->values = {
        "", "filament_diameter", "filament_diameter", ""};

    bundle.load_config_model("different-filaments.3mf", project);

    REQUIRE(bundle.filament_presets.size() == 2);
    REQUIRE(bundle.filament_presets[0] == filament_name);
    REQUIRE(bundle.filament_presets[1] != bundle.filament_presets[0]);
    REQUIRE(bundle.filaments.get_project_embedded_presets().size() == 1);
}
TEST_CASE("Structured process table restores only the matching nozzle row", "[Config][18082]")
{
    DynamicPrintConfig source = DynamicPrintConfig::full_print_config();
    source.option<ConfigOptionInts>("print_extruder_id")->values = {1, 1};
    source.option<ConfigOptionStrings>("print_extruder_variant")->values = {"Direct Drive Standard", "Direct Drive Standard"};
    source.option<ConfigOptionInts>("print_nozzle_variant", true)->values = {0, 2};
    source.option<ConfigOptionFloats>("initial_layer_speed")->values = {40., 60.};
    DynamicPrintConfig project(source);
    project.option<ConfigOptionInts>("print_extruder_id")->values = {1};
    project.option<ConfigOptionStrings>("print_extruder_variant")->values = {"Direct Drive Standard"};
    project.option<ConfigOptionInts>("print_nozzle_variant")->values = {2};
    project.option<ConfigOptionFloats>("initial_layer_speed")->values = {41.};
    DynamicPrintConfig target(project);
    restore_project_variant_overrides(Preset::TYPE_PRINT, target, project, source, {"initial_layer_speed"});
    REQUIRE(target.option<ConfigOptionFloats>("initial_layer_speed")->values == std::vector<double>{40., 41.});
}

namespace {
class RenameTestCollection : public PresetCollection
{
public:
    using PresetCollection::PresetCollection;
    using PresetCollection::update_map_system_profile_renamed;
    static void rebuild_renames(PresetCollection &collection)
    {
        const auto rebuild = &RenameTestCollection::update_map_system_profile_renamed;
        (collection.*rebuild)();
    }
};
}

TEST_CASE("K3 renamed project presets preserve legacy nozzle overrides", "[Config][18126]")
{
    const auto type = GENERATE(Preset::TYPE_PRINT, Preset::TYPE_PRINTER, Preset::TYPE_FILAMENT);
    const bool inherited = GENERATE(false, true);
    PresetBundle bundle;
    const PresetCollection &defaults = type == Preset::TYPE_PRINT ? bundle.prints :
        type == Preset::TYPE_PRINTER ? static_cast<const PresetCollection &>(bundle.printers) : bundle.filaments;
    RenameTestCollection collection(type, defaults.default_preset().config.keys(), static_cast<const PrintRegionConfig &>(FullPrintConfig::defaults()));
    const bool filament = type == Preset::TYPE_FILAMENT;
    const std::string scope = filament ? "filament" : type == Preset::TYPE_PRINT ? "print" : "printer";
    const std::string prefix = filament ? "Hyper PLA @" : type == Preset::TYPE_PRINT ? "0.20mm Standard @" : "";
    const std::string key = filament ? "filament_max_volumetric_speed" :
        type == Preset::TYPE_PRINT ? "initial_layer_speed" : "retraction_length";
    const std::string old_name = prefix + "Creality F039 0.4 nozzle";
    const std::string new_name = prefix + "Creality K3";
    const size_t rows = filament ? 4 : 16;
    DynamicPrintConfig source(defaults.default_preset().config);
    source.option<ConfigOptionStrings>(scope + "_extruder_variant", true)->values.assign(rows, "Direct Drive Standard");
    auto &nozzles = source.option<ConfigOptionInts>(scope + "_nozzle_variant", true)->values;
    nozzles.resize(rows);
    for (size_t i = 0; i < rows; ++i)
        nozzles[i] = int(i % 4);
    if (!filament) {
        auto &extruders = source.option<ConfigOptionInts>(scope + "_extruder_id", true)->values;
        extruders.resize(rows);
        for (size_t i = 0; i < rows; ++i)
            extruders[i] = int(i / 4 + 1);
    }
    if (type == Preset::TYPE_PRINTER)
        source.option<ConfigOptionFloats>("nozzle_diameter")->values.assign(4, 0.4);
    source.option<ConfigOptionFloats>(key)->values.assign(rows, 40.);
    auto &system = collection.load_preset("", new_name, source, false);
    system.is_system = true;
    VendorProfile vendor;
    vendor.id = "Creality";
    system.vendor = &vendor;
    collection.update_map_system_profile_renamed();
    REQUIRE(collection.canonical_preset_name(old_name) == new_name);

    DynamicPrintConfig project(defaults.default_preset().config);
    project.option<ConfigOptionFloats>(key)->values = {41.};
    if (type == Preset::TYPE_PRINTER)
        project.option<ConfigOptionFloats>(key)->values = {41., 42., 43., 44.};
    project.opt_string("inherits", true) = inherited ? old_name : "";
    collection.load_external_preset("boat.3mf", "boat.3mf", inherited ? "Custom profile" : old_name,
        project, {key, "inherits"}, PresetCollection::LoadAndSelect::Always);
    const auto &loaded = collection.get_edited_preset().config;
    const auto &values = loaded.option<ConfigOptionFloats>(key)->values;
    REQUIRE(collection.get_selected_preset_name() ==
            (inherited && type == Preset::TYPE_PRINTER ? "Custom profile" : new_name));
    REQUIRE(values.size() == rows);
    for (size_t i = 0; i < rows; ++i) {
        const double expected = i % 4 != 0 ? 40. : type == Preset::TYPE_PRINTER ? 41. + i / 4 : 41.;
        REQUIRE(values[i] == Approx(expected));
    }
    if (inherited)
        REQUIRE(loaded.opt_string("inherits") == new_name);

    // An actual user preset owning the old name has precedence over an alias.
    collection.load_preset("", old_name, source, false);
    REQUIRE(collection.canonical_preset_name(old_name) == old_name);
    REQUIRE(collection.canonical_preset_name("Unrelated printer") == "Unrelated printer");
}

TEST_CASE("Embedded legacy K3 filament preserves its custom name and overrides", "[Config][18126]")
{
    PresetBundle bundle;
    RenameTestCollection collection(Preset::TYPE_FILAMENT, bundle.filaments.default_preset().config.keys(), static_cast<const PrintRegionConfig &>(FullPrintConfig::defaults()));
    auto source = bundle.filaments.default_preset().config;
    source.option<ConfigOptionStrings>("filament_extruder_variant", true)->values.assign(4, "Direct Drive Standard");
    source.option<ConfigOptionInts>("filament_nozzle_variant", true)->values = {0, 1, 2, 3};
    source.option<ConfigOptionFloats>("filament_max_volumetric_speed")->values = {40., 20., 60., 80.};
    auto &system = collection.load_preset("", "Hyper PLA @Creality K3", source, false);
    system.is_system = true;
    VendorProfile vendor;
    vendor.id = "Creality";
    system.vendor = &vendor;
    collection.update_map_system_profile_renamed();
    Preset custom(Preset::TYPE_FILAMENT, "My tuned PLA");
    custom.is_project_embedded = true;
    custom.config.opt_string("inherits", true) = system.renamed_from.front();
    custom.config.option<ConfigOptionFloats>("filament_max_volumetric_speed", true)->values = {41.};
    std::vector<Preset *> embedded = {&custom};
    PresetsConfigSubstitutions substitutions;
    collection.load_project_embedded_presets(embedded, PRESET_FILAMENT_NAME, substitutions,
                                            ForwardCompatibilitySubstitutionRule::Enable);
    const Preset *loaded = collection.find_preset("My tuned PLA", false, true);
    REQUIRE(loaded != nullptr);
    REQUIRE(loaded->inherits() == "Hyper PLA @Creality K3");
    REQUIRE(loaded->config.option<ConfigOptionFloats>("filament_max_volumetric_speed")->values ==
            std::vector<double>{41., 20., 60., 80.});
}

TEST_CASE("Custom printer identity survives equal parameters and project reopening", "[Config][18126]")
{
    PresetBundle bundle;
    auto source = bundle.printers.default_preset().config;
    auto &system = bundle.printers.load_preset("", "Creality K3", source, false);
    system.is_system = true;
    auto project = source;
    project.opt_string("inherits", true) = "Creality K3";
    project.opt_string("printer_settings_id", true) = "My F039 printer";
    const bool local_exists = GENERATE(false, true);
    if (local_exists)
        bundle.printers.load_preset("", "My F039 printer", project, false);
    bundle.printers.load_external_preset("222.3mf", "222.3mf", "My F039 printer", project,
        {"inherits", "printer_settings_id"}, PresetCollection::LoadAndSelect::Always);
    REQUIRE(bundle.printers.get_selected_preset_name() == "My F039 printer");
    REQUIRE(bundle.printers.get_edited_preset().inherits() == "Creality K3");
    auto embedded = bundle.printers.get_project_embedded_presets();
    REQUIRE(embedded.size() == 1);
    REQUIRE(embedded.front()->name == "My F039 printer");
    REQUIRE(embedded.front()->config.opt_string("printer_settings_id") == "My F039 printer");
    REQUIRE(embedded.front()->inherits() == "Creality K3");

    PresetBundle reopened;
    auto &parent = reopened.printers.load_preset("", "Creality K3", source, false);
    parent.is_system = true;
    PresetsConfigSubstitutions substitutions;
    reopened.printers.load_project_embedded_presets(embedded, PRESET_PRINTER_NAME, substitutions,
                                                   ForwardCompatibilitySubstitutionRule::Enable);
    reopened.printers.load_external_preset("saved.3mf", "saved.3mf", "My F039 printer", project,
        {"inherits", "printer_settings_id"}, PresetCollection::LoadAndSelect::Always);
    REQUIRE(reopened.printers.get_selected_preset_name() == "My F039 printer");
    REQUIRE(reopened.printers.get_edited_preset().inherits() == "Creality K3");
    for (Preset *preset : embedded)
        delete preset;
}

TEST_CASE("Same-name local printer is not overwritten by imported project parameters", "[Config][18126]")
{
    PresetBundle bundle;
    auto source = bundle.printers.default_preset().config;
    auto &system = bundle.printers.load_preset("", "Creality K3", source, false);
    system.is_system = true;
    source.opt_string("inherits", true) = "Creality K3";
    source.opt_string("printer_settings_id", true) = "My printer";
    source.option<ConfigOptionFloat>("extruder_clearance_radius")->value = 30.;
    bundle.printers.load_preset("", "My printer", source, false);
    auto project = source;
    project.option<ConfigOptionFloat>("extruder_clearance_radius")->value = 35.;
    bundle.printers.load_external_preset("222.3mf", "222.3mf", "My printer", project,
        {"inherits", "printer_settings_id", "extruder_clearance_radius"}, PresetCollection::LoadAndSelect::Always);
    REQUIRE(bundle.printers.get_selected_preset_name() == "My printer (222.3mf)");
    REQUIRE(bundle.printers.get_edited_preset().config.opt_float("extruder_clearance_radius") == Approx(35.));
    REQUIRE(bundle.printers.find_preset("My printer", false, true)->config.opt_float("extruder_clearance_radius") == Approx(30.));
}

TEST_CASE("K3 runtime aliases are scoped and idempotent without profile metadata", "[Config][18126]")
{
    PresetBundle bundle;
    RenameTestCollection collection(Preset::TYPE_PRINTER, bundle.printers.default_preset().config.keys(), static_cast<const PrintRegionConfig &>(FullPrintConfig::defaults()));
    auto config = bundle.printers.default_preset().config;
    auto &preset = collection.load_preset("", "Creality K3", config, false);
    VendorProfile vendor;
    vendor.id = "Creality";
    preset.vendor = &vendor;
    preset.is_system = true;
    bool should_alias = true;
    SECTION("Creality system preset") {}
    SECTION("User preset must not acquire system aliases") {
        preset.is_system = false;
        should_alias = false;
    }
    SECTION("Other vendors must not acquire Creality aliases") {
        vendor.id = "Other vendor";
        should_alias = false;
    }
    SECTION("Unknown vendor must not acquire Creality aliases") {
        preset.vendor = nullptr;
        should_alias = false;
    }
    collection.update_map_system_profile_renamed();
    collection.update_map_system_profile_renamed();
    REQUIRE(preset.renamed_from.size() == (should_alias ? 3 : 0));
    for (const std::string name : {"Creality F039 0.4 nozzle", "Creality F039", "Creality K3 0.4 nozzle"})
        REQUIRE(collection.canonical_preset_name(name) == (should_alias ? "Creality K3" : name));
}

TEST_CASE("Legacy K3 import replaces stale nozzle choices without hiding real overrides", "[Config][18126]")
{
    PresetBundle bundle;
    auto source = bundle.printers.default_preset().config;
    source.opt_string("printer_model", true) = "Creality K3";
    source.option<ConfigOptionFloats>("nozzle_diameter")->values.assign(4, 0.4);
    source.option<ConfigOptionStrings>("nozzle_variant_ids", true)->values =
        {"E1-N04", "E1-N02", "E2-N04", "E2-N02", "E3-N04", "E3-N06", "E4-N04", "E4-N08"};
    source.option<ConfigOptionFloats>("nozzle_variant_diameters", true)->values =
        {0.4, 0.2, 0.4, 0.2, 0.4, 0.6, 0.4, 0.8};
    source.option<ConfigOptionInts>("nozzle_variant_extruder_ids", true)->values = {1, 1, 2, 2, 3, 3, 4, 4};
    source.option<ConfigOptionInts>("nozzle_variant_indices", true)->values = {0, 1, 0, 1, 0, 2, 0, 3};
    source.option<ConfigOptionEnumsGeneric>("nozzle_variant_volume_types", true)->values.assign(8, int(nvtStandard));
    auto &system = bundle.printers.load_preset("", "Creality K3", source, true);
    VendorProfile vendor;
    vendor.id = "Creality";
    system.vendor = &vendor;
    system.is_system = true;
    bundle.project_config.option<ConfigOptionStrings>("variant_id", true)->values = {"E1-N02", "E2-N04", "E3-N04", "E4-N04"};
    bundle.project_config.option<ConfigOptionInts>("variant_index", true)->values = {1, 0, 0, 0};

    auto project = DynamicPrintConfig::full_print_config();
    project.apply(source);
    project.opt_string("printer_settings_id", true) = "Creality K3";
    project.option<ConfigOptionStrings>("filament_colour", true)->values = {"#FFFFFF"};
    project.option<ConfigOptionStrings>("different_settings_to_system", true)->values = {"", "", "nozzle_diameter;extruder_clearance_radius"};
    project.option<ConfigOptionFloats>("nozzle_diameter")->values = {0.4, 0.2, 0.6, 0.8};
    project.option<ConfigOptionFloat>("extruder_clearance_radius")->value = 37.;
    project.erase("variant_id");
    project.erase("variant_index");
    bool supported = true;
    bool modern = false;
    SECTION("All legacy physical nozzles map to system variants") {}
    SECTION("Unknown diameter keeps the imported override") {
        project.option<ConfigOptionFloats>("nozzle_diameter")->values[2] = 0.5;
        supported = false;
    }
    SECTION("Modern project selectors have precedence") {
        project.option<ConfigOptionStrings>("variant_id", true)->values = {"E1-N04", "E2-N04", "E3-N04", "E4-N04"};
        modern = true;
    }
    bundle.load_config_model("mixed.3mf", project);
    const auto &loaded = bundle.printers.get_edited_preset().config;
    REQUIRE(loaded.opt_float("extruder_clearance_radius") == Approx(37.));
    REQUIRE(loaded.option<ConfigOptionFloats>("nozzle_diameter")->values ==
        (supported && !modern ? std::vector<double>{0.4, 0.4, 0.4, 0.4} :
                                project.option<ConfigOptionFloats>("nozzle_diameter")->values));
    if (supported && !modern) {
        REQUIRE(bundle.project_config.option<ConfigOptionInts>("variant_index")->values == std::vector<int>{0, 1, 2, 3});
        for (size_t i = 0; i < 4; ++i)
            REQUIRE(bundle.get_selected_nozzle_variant(i).nozzle_diameter ==
                    Approx(project.option<ConfigOptionFloats>("nozzle_diameter")->values[i]));
    } else {
        REQUIRE(bundle.project_config.option<ConfigOptionInts>("variant_index")->values.empty());
        REQUIRE(bundle.project_config.option<ConfigOptionStrings>("variant_id")->values ==
            (modern ? std::vector<std::string>{"E1-N04", "E2-N04", "E3-N04", "E4-N04"} : std::vector<std::string>{}));
    }
}


TEST_CASE("User K3 presets migrate cloud and local references without losing overrides", "[Config][UserPresetMigration]")
{
    const auto type = GENERATE(Preset::TYPE_PRINT, Preset::TYPE_PRINTER, Preset::TYPE_FILAMENT);
    const std::string legacy = GENERATE("Creality F039 0.4 nozzle", "Creality K3 0.4 nozzle");
    const bool cloud = GENERATE(false, true);
    PresetBundle bundle;
    const PresetCollection &defaults = type == Preset::TYPE_PRINT ? bundle.prints :
        type == Preset::TYPE_PRINTER ? static_cast<const PresetCollection &>(bundle.printers) : bundle.filaments;
    RenameTestCollection collection(type, defaults.default_preset().config.keys(), static_cast<const PrintRegionConfig &>(FullPrintConfig::defaults()));
    const bool filament = type == Preset::TYPE_FILAMENT;
    const std::string scope = filament ? "filament" : type == Preset::TYPE_PRINT ? "print" : "printer";
    const std::string prefix = filament ? "HP-TPU @" : type == Preset::TYPE_PRINT ? "0.20mm Standard @" : "";
    const std::string key = filament ? "filament_max_volumetric_speed" :
        type == Preset::TYPE_PRINT ? "initial_layer_speed" : "retraction_length";
    const size_t rows = filament ? 4 : 16;
    DynamicPrintConfig base(defaults.default_preset().config);
    base.option<ConfigOptionStrings>(scope + "_extruder_variant", true)->values.assign(rows, "Direct Drive Standard");
    auto &nozzles = base.option<ConfigOptionInts>(scope + "_nozzle_variant", true)->values;
    for (size_t i = 0; i < rows; ++i) {
        if (i == 0) nozzles.clear();
        nozzles.push_back(int(i % 4));
    }
    if (!filament) {
        auto &extruders = base.option<ConfigOptionInts>(scope + "_extruder_id", true)->values;
        extruders.clear();
        for (size_t i = 0; i < rows; ++i) extruders.push_back(int(i / 4 + 1));
    }
    if (type == Preset::TYPE_PRINTER)
        base.option<ConfigOptionFloats>("nozzle_diameter")->values.assign(4, 0.4);
    base.option<ConfigOptionFloats>(key)->values.assign(rows, 40.);
    VendorProfile vendor;
    vendor.id = "Creality";
    auto &system = collection.load_preset("", prefix + "Creality K3", base, false);
    system.is_system = true;
    system.vendor = &vendor;
    collection.update_map_system_profile_renamed();
    DynamicPrintConfig user;
    user.opt_string("inherits", true) = prefix + legacy;
    user.option<ConfigOptionFloats>(key, true)->values = type == Preset::TYPE_PRINTER ?
        std::vector<double>{41., 42., 43., 44.} : std::vector<double>{41.};
    if (cloud) {
        std::map<std::string, std::string> values{{"version", "2.0.0"}, {"setting_id", "cloud-id"},
            {"user_id", "test-user"}, {"base_id", "base-id"}, {"updated_time", "123"}};
        for (const auto &k : user.keys()) values[k] = user.option(k)->serialize();
        PresetsConfigSubstitutions substitutions;
        REQUIRE(collection.load_user_preset("My tuned preset", values, substitutions, ForwardCompatibilitySubstitutionRule::Enable));
        const auto *loaded = collection.find_preset("My tuned preset", false);
        REQUIRE(loaded != nullptr);
        REQUIRE(loaded->setting_id == "cloud-id");
        REQUIRE(loaded->updated_time == 123);
        user = loaded->config;
    } else {
        struct TempPresets {
            boost::filesystem::path path = boost::filesystem::temp_directory_path() /
                boost::filesystem::unique_path("k3-user-presets-%%%%-%%%%");
            TempPresets() { boost::filesystem::create_directories(path / "presets"); }
            ~TempPresets() { boost::system::error_code ec; boost::filesystem::remove_all(path, ec); }
        } temp;
        user.save_to_json((temp.path / "presets" / "My tuned preset.json").string(),
                          "My tuned preset", "user", "2.0.0");
        PresetsConfigSubstitutions substitutions;
        collection.load_presets(temp.path.string(), "presets", substitutions, ForwardCompatibilitySubstitutionRule::Enable);
        const auto *loaded = collection.find_preset("My tuned preset", false);
        REQUIRE(loaded != nullptr);
        user = loaded->config;
    }
    REQUIRE(user.opt_string("inherits") == prefix + "Creality K3");
    const auto &values = user.option<ConfigOptionFloats>(key)->values;
    REQUIRE(values.size() == rows);
    for (size_t i = 0; i < rows; ++i)
        REQUIRE(values[i] == Approx(i % 4 != 0 ? 40. : type == Preset::TYPE_PRINTER ? 41. + i / 4 : 41.));
    const DynamicPrintConfig migrated(user);
    collection.migrate_user_preset_config(user);
    REQUIRE(user.diff(migrated).empty());
    DynamicPrintConfig unknown;
    unknown.opt_string("inherits", true) = "Unknown @Creality F039 0.4 nozzle";
    collection.migrate_user_preset_config(unknown);
    REQUIRE(unknown.opt_string("inherits") == "Unknown @Creality F039 0.4 nozzle");
}


TEST_CASE("User preset references follow installed K3 aliases and preserve custom names", "[Config][UserPresetMigration]")
{
    PresetBundle bundle;
    VendorProfile vendor;
    vendor.id = "Creality";
    auto install = [&](PresetCollection &collection, const std::string &name) {
        auto &system = collection.load_preset("", name, collection.default_preset().config, false);
        system.is_system = true;
        system.vendor = &vendor;
        RenameTestCollection::rebuild_renames(collection);
    };
    install(bundle.printers, "Creality K3");
    install(bundle.prints, "0.20mm Standard @Creality K3");
    install(bundle.filaments, "HP-TPU @Creality K3");
    const std::string old_name = GENERATE("Creality F039 0.4 nozzle", "Creality K3 0.4 nozzle");
    DynamicPrintConfig config;
    config.opt_string("inherits", true) = "HP-TPU @" + old_name;
    config.opt_string("printer_model", true) = "Creality F039";
    config.opt_string("default_print_profile", true) = "0.20mm Standard @" + old_name;
    config.option<ConfigOptionStrings>("default_filament_profile", true)->values = {"HP-TPU @" + old_name};
    config.option<ConfigOptionStrings>("compatible_printers", true)->values = {old_name, "My custom F039", "Creality F038 0.4 nozzle"};
    config.option<ConfigOptionStrings>("print_compatible_printers", true)->values = {old_name};
    bundle.filaments.migrate_user_preset_config(config);
    REQUIRE(config.opt_string("inherits") == "HP-TPU @Creality K3");
    REQUIRE(config.opt_string("printer_model") == "Creality K3");
    REQUIRE(config.opt_string("default_print_profile") == "0.20mm Standard @Creality K3");
    REQUIRE(config.option<ConfigOptionStrings>("default_filament_profile")->values == std::vector<std::string>{"HP-TPU @Creality K3"});
    REQUIRE(config.option<ConfigOptionStrings>("compatible_printers")->values ==
            std::vector<std::string>{"Creality K3", "My custom F039", "Creality F038 0.4 nozzle"});
    REQUIRE(config.option<ConfigOptionStrings>("print_compatible_printers")->values == std::vector<std::string>{"Creality K3"});
    bundle.printers.load_preset("", old_name, bundle.printers.default_preset().config, false);
    config.option<ConfigOptionStrings>("compatible_printers")->values = {old_name};
    bundle.migrate_preset_references(config);
    REQUIRE(config.option<ConfigOptionStrings>("compatible_printers")->values == std::vector<std::string>{old_name});
}
