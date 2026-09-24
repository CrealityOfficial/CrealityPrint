#include <catch2/catch.hpp>

#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/AppConfig.hpp"

using namespace Slic3r;

TEST_CASE("Project reset refreshes process dirty state without losing kept changes", "[PresetProjectReset]")
{
    PresetBundle bundle;
    DynamicPrintConfig config = bundle.prints.default_preset().config;
    config.set_key_value("layer_height", new ConfigOptionFloat(0.2));
    config.set_key_value("mixed_filament_definitions", new ConfigOptionString(""));
    bundle.prints.load_preset("", "System process", config);
    bundle.prints.get_selected_preset().is_system = true;
    bundle.prints.get_edited_preset().is_system = true;

    // The third-party 3MF changes project-only mixed filament settings. The old
    // import path forced the dirty flags even though current_is_dirty() is false.
    auto& edited = bundle.prints.get_edited_preset();
    edited.config.set_key_value("mixed_filament_definitions", new ConfigOptionString("1,2,1,1,50,0,g,w,m2"));
    edited.config.set_key_value("mixed_filament_height_lower_bound", new ConfigOptionFloat(0.05));
    bundle.prints.get_selected_preset().set_dirty();
    edited.set_dirty();

    bool expect_dirty = false;
    double expected_layer_height = 0.2;
    SECTION("Project-only differences clear a stale modified label")
    {
        REQUIRE_FALSE(bundle.prints.current_is_dirty());
    }
    SECTION("Keeping a real process change preserves its value and modified label")
    {
        edited.config.set_key_value("layer_height", new ConfigOptionFloat(0.16));
        expect_dirty = true;
        expected_layer_height = 0.16;
        REQUIRE(bundle.prints.current_is_dirty());
    }
    SECTION("Discarding a real process change restores the system value")
    {
        edited.config.set_key_value("layer_height", new ConfigOptionFloat(0.16));
        bundle.prints.discard_current_changes();
    }

    bundle.reset_project_embedded_presets();

    REQUIRE(bundle.prints.get_selected_preset_name() == "System process");
    REQUIRE(bundle.prints.get_selected_preset().is_system);
    REQUIRE(bundle.prints.get_selected_preset().config.opt_float("layer_height") == Approx(0.2));
    REQUIRE(bundle.prints.get_edited_preset().config.opt_float("layer_height") == Approx(expected_layer_height));
    REQUIRE(bundle.prints.current_is_dirty() == expect_dirty);
    REQUIRE(bundle.prints.get_selected_preset().is_dirty == expect_dirty);
    REQUIRE(bundle.prints.get_edited_preset().is_dirty == expect_dirty);
}

TEST_CASE("Flush multiplier preferences are isolated by printer preset", "[PresetFlushMultiplier]")
{
    PresetBundle bundle;
    AppConfig app;
    bundle.filament_presets = {bundle.filaments.get_selected_preset_name()};
    auto config = bundle.printers.default_preset().config;
    config.set_key_value("default_flush_multiplier", new ConfigOptionFloat(1.3));
    bundle.printers.load_preset("", "Printer A", config);
    config.set_key_value("default_flush_multiplier", new ConfigOptionFloat(0.8));
    bundle.printers.load_preset("", "Printer B", config);
    auto select = [&](const std::string &name) {
        bundle.printers.select_preset_by_name(name, true);
        bundle.load_flush_multiplier(app);
        return bundle.project_config.opt_float("flush_multiplier");
    };

    // The legacy global setting must not leak into either printer.
    app.set("flush_multiplier", "2.5");
    REQUIRE(select("Printer A") == Approx(1.3));
    bundle.project_config.option<ConfigOptionFloat>("flush_multiplier")->value = 1.7;
    bundle.save_flush_multiplier(app);
    bundle.export_selections(app);
    REQUIRE(select("Printer B") == Approx(0.8));
    bundle.project_config.option<ConfigOptionFloat>("flush_multiplier")->value = 0.6;
    bundle.save_flush_multiplier(app);
    bundle.export_selections(app);
    REQUIRE(select("Printer A") == Approx(1.7));
    REQUIRE(select("Printer B") == Approx(0.6));

    // Project values are transient until the user explicitly edits the multiplier.
    bundle.project_config.option<ConfigOptionFloat>("flush_multiplier")->value = 2.0;
    bundle.export_selections(app);
    REQUIRE(select("Printer A") == Approx(1.7));
    REQUIRE(select("Printer B") == Approx(0.6));

    bundle.project_config.option<ConfigOptionFloat>("flush_multiplier")->value = bundle.default_flush_multiplier();
    bundle.save_flush_multiplier(app);
    bundle.export_selections(app);
    REQUIRE(select("Printer A") == Approx(1.7));
    REQUIRE(select("Printer B") == Approx(0.8));

    // Saved preferences also survive reconstruction of the preset bundle.
    PresetBundle restarted;
    restarted.printers.load_preset("", "Printer B", config);
    restarted.printers.select_preset_by_name("Printer B", true);
    restarted.load_flush_multiplier(app);
    REQUIRE(restarted.project_config.opt_float("flush_multiplier") == Approx(0.8));
}

TEST_CASE("Invalid saved flush multipliers fall back to the printer default", "[PresetFlushMultiplier]")
{
    PresetBundle bundle;
    AppConfig app;
    bundle.printers.get_edited_preset().config.set_key_value("default_flush_multiplier", new ConfigOptionFloat(1.2));
    const auto saved = GENERATE("", "garbage", "nan", "inf", "-0.1", "3.1", "1.5garbage", "1e999");
    app.set_printer_setting(bundle.printers.get_selected_preset_name(), "flush_multiplier", saved);
    bundle.load_flush_multiplier(app);
    REQUIRE(bundle.project_config.opt_float("flush_multiplier") == Approx(1.2));
}

TEST_CASE("Printer model identity follows user preset inheritance", "[PresetFlushMultiplier]")
{
    PresetBundle bundle;
    auto config = bundle.printers.default_preset().config;
    config.set_key_value("printer_model", new ConfigOptionString("K2"));
    bundle.printers.load_preset("", "K2 system", config);
    const std::string system_id = bundle.printer_model_identity(*bundle.printers.find_preset("K2 system", false));

    config.set_key_value("inherits", new ConfigOptionString("K2 system"));
    config.set_key_value("printer_model", new ConfigOptionString(""));
    bundle.printers.load_preset("", "My K2", config);
    REQUIRE(bundle.printer_model_identity(*bundle.printers.find_preset("My K2", false)) == system_id);
    config.set_key_value("inherits", new ConfigOptionString("My K2"));
    bundle.printers.load_preset("", "My second K2", config);
    REQUIRE(bundle.printer_model_identity(*bundle.printers.find_preset("My second K2", false)) == system_id);

    config.set_key_value("inherits", new ConfigOptionString(""));
    config.set_key_value("printer_model", new ConfigOptionString("K3"));
    bundle.printers.load_preset("", "K3 system", config);
    REQUIRE(bundle.printer_model_identity(*bundle.printers.find_preset("K3 system", false)) != system_id);

    config.set_key_value("printer_model", new ConfigOptionString(""));
    bundle.printers.load_preset("", "Third party", config);
    const std::string third_party_id = bundle.printer_model_identity(*bundle.printers.find_preset("Third party", false));
    config.set_key_value("inherits", new ConfigOptionString("Third party"));
    bundle.printers.load_preset("", "Third party user", config);
    REQUIRE(bundle.printer_model_identity(*bundle.printers.find_preset("Third party user", false)) == third_party_id);
    REQUIRE(third_party_id != system_id);
}

TEST_CASE("Same-model preset restoration keeps the current flush matrix", "[PresetFlushMultiplier]")
{
    PresetBundle bundle;
    AppConfig app;
    const std::string filament = bundle.filaments.get_selected_preset_name();
    bundle.filament_presets = {filament, filament};
    auto &matrix = bundle.project_config.option<ConfigOptionFloats>("flush_volumes_matrix")->values;
    const std::vector<double> manual_matrix = {0.0, 123.0, 456.0, 0.0};
    matrix = manual_matrix;
    bundle.project_config.option<ConfigOptionStrings>("filament_colour")->values = {"#FFFFFF", "#000000"};
    bundle.project_config.option<ConfigOptionFloats>("flush_volumes_vector")->values = {140, 140, 140, 140};
    bundle.export_selections(app);
    app.set_printer_setting(bundle.printers.get_selected_preset_name(), "flush_volumes_matrix", "0|600|700|0");
    bundle.update_selections(app, false);
    REQUIRE(matrix == manual_matrix);
}

TEST_CASE("Opening a project preserves the user's multiplier for the same model", "[PresetFlushMultiplier]")
{
    PresetBundle bundle;
    AppConfig app;
    auto config = bundle.printers.default_preset().config;
    config.set_key_value("printer_model", new ConfigOptionString("K2"));
    config.set_key_value("default_flush_multiplier", new ConfigOptionFloat(1.3));
    bundle.printers.load_preset("", "K2 system", config);
    bundle.printers.select_preset_by_name("K2 system", true);
    const std::string previous_model = bundle.printer_model_identity(bundle.printers.get_edited_preset());
    const double previous_multiplier = 1.8;
    bundle.project_config.option<ConfigOptionFloat>("flush_multiplier")->value = previous_multiplier;
    bundle.save_flush_multiplier(app);

    double expected = previous_multiplier;
    SECTION("Same preset") {}
    SECTION("Project uses a derived preset of the same model") {
        config.set_key_value("inherits", new ConfigOptionString("K2 system"));
        config.set_key_value("printer_model", new ConfigOptionString(""));
        bundle.printers.load_preset("", "Project K2", config);
        bundle.printers.select_preset_by_name("Project K2", true);
    }
    SECTION("Different model uses its saved preference") {
        config.set_key_value("printer_model", new ConfigOptionString("K3"));
        bundle.printers.load_preset("", "K3", config);
        bundle.printers.select_preset_by_name("K3", true);
        app.set_printer_setting("K3", "flush_multiplier", "0.7");
        expected = 0.7;
    }
    SECTION("Different model without a saved preference uses its default") {
        config.set_key_value("printer_model", new ConfigOptionString("K3"));
        bundle.printers.load_preset("", "K3", config);
        bundle.printers.select_preset_by_name("K3", true);
        expected = 1.3;
    }

    // Simulate the project-config import overwriting the multiplier, then reconcile the final printer.
    DynamicPrintConfig imported;
    imported.set_key_value("flush_multiplier", new ConfigOptionFloat(1.0));
    bundle.project_config.apply(imported);
    bundle.restore_flush_multiplier_after_project_load(app, previous_model, previous_multiplier);
    REQUIRE(bundle.project_config.opt_float("flush_multiplier") == Approx(expected));
    REQUIRE(app.get_printer_setting("K2 system", "flush_multiplier") == "1.8");
}

TEST_CASE("Saved project coefficient wins over the current printer default", "[PresetFlushMultiplier]")
{
    PresetBundle bundle;
    AppConfig app;
    bundle.printers.get_edited_preset().config.set_key_value("printer_model", new ConfigOptionString("SPARKX i7"));
    bundle.printers.get_edited_preset().config.set_key_value("default_flush_multiplier", new ConfigOptionFloat(0.8));
    const std::string model = bundle.printer_model_identity(bundle.printers.get_edited_preset());
    bundle.project_config.option<ConfigOptionFloat>("flush_multiplier")->value = 0.8;
    const double saved = GENERATE(0.0, 0.8, 1.1, 3.0);
    bundle.restore_flush_multiplier_after_project_load(app, model, 0.8, saved);
    REQUIRE(bundle.project_config.opt_float("flush_multiplier") == Approx(saved));
    REQUIRE_FALSE(app.has_printer_setting(bundle.printers.get_selected_preset_name(), "flush_multiplier"));
}

TEST_CASE("Third-party imports discard flushing settings after selecting the printer", "[PresetFlushMultiplier]")
{
    PresetBundle bundle;
    AppConfig app;
    const std::string filament = bundle.filaments.get_selected_preset_name();
    bundle.filament_presets = {filament, filament};
    bundle.project_config.option<ConfigOptionStrings>("filament_colour")->values = {"#FFFFFF", "#000000"};
    const std::string previous_model = bundle.printer_model_identity(bundle.printers.get_edited_preset());
    auto &matrix = bundle.project_config.option<ConfigOptionFloats>("flush_volumes_matrix")->values;
    const std::vector<double> foreign_matrix = {0.0, 9999.0, 8888.0, 0.0};
    matrix = foreign_matrix;
    bundle.project_config.option<ConfigOptionBool>("flush_volumes_changed")->value = true;
    bundle.project_config.option<ConfigOptionFloat>("flush_multiplier")->value = 2.7;
    bool is_native = false;
    double expected_multiplier = 1.1;
    SECTION("Same model keeps the current coefficient but recalculates the matrix") {}
    SECTION("Different model uses its saved coefficient") {
        bundle.printers.get_edited_preset().config.set_key_value("printer_model", new ConfigOptionString("Another model"));
        app.set_printer_setting(bundle.printers.get_selected_preset_name(), "flush_multiplier", "0.9");
        expected_multiplier = 0.9;
    }
    SECTION("Different model without a preference uses its default") {
        bundle.printers.get_edited_preset().config.set_key_value("printer_model", new ConfigOptionString("Another model"));
        bundle.printers.get_edited_preset().config.set_key_value("default_flush_multiplier", new ConfigOptionFloat(0.8));
        expected_multiplier = 0.8;
    }
    SECTION("Native projects retain the saved matrix and coefficient") {
        is_native = true;
        expected_multiplier = 2.7;
    }
    bundle.restore_flush_multiplier_after_project_load(app, previous_model, 1.1, 2.7, is_native);
    REQUIRE(bundle.project_config.opt_float("flush_multiplier") == Approx(expected_multiplier));
    REQUIRE(matrix.size() == 4);
    REQUIRE(matrix[0] == 0.0);
    REQUIRE(matrix[3] == 0.0);
    if (is_native) {
        REQUIRE(matrix == foreign_matrix);
        REQUIRE(bundle.project_config.option<ConfigOptionBool>("flush_volumes_changed")->value);
    } else {
        REQUIRE(matrix[1] != foreign_matrix[1]);
        REQUIRE(matrix[2] != foreign_matrix[2]);
        REQUIRE_FALSE(bundle.project_config.option<ConfigOptionBool>("flush_volumes_changed")->value);
    }
}
