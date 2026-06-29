#include "DeviceListSimple.hpp"

#include "MCPChatPanel.hpp"

#include "slic3r/GUI/GLCanvas3D.hpp"
#include "slic3r/GUI/ImGuiWrapper.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/I18N.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "slic3r/GUI/GUI_ObjectList.hpp"
#include "slic3r/GUI/PartPlate.hpp"
#include "slic3r/GUI/GUI_Preview.hpp"
#include "slic3r/GUI/Tab.hpp"
#include "slic3r/GUI/ConfigWizard.hpp"
#include "print_manage/data/DataCenter.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/ModelInstance.hpp"
#include <nlohmann/json.hpp>

#include <imgui/imgui.h>
#include <boost/log/trivial.hpp>
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <set>
#include <limits>
#include <GL/glew.h>
#include "slic3r/GUI/MsgDialog.hpp"
#include "GLSimpleUtils.hpp"
#include "slic3r/GUI/simple/SimpleModelMgr.hpp"
#include "filamentMapping/ImGuiFilamentPanel.hpp"
#include "sendWorkflow/EasyPrintSender.hpp"

namespace Slic3r { namespace GUI {

namespace {

const std::set<std::string> simple_mode_preset_meta_keys = {
    "inherits",
    "print_settings_id",
    "filament_settings_id",
    "printer_settings_id"
};

const std::vector<std::string> simple_mode_project_options = {
    "flush_volumes_vector",
    "flush_volumes_matrix",
    "flush_volumes_changed",
    "filament_colour",
    "wipe_tower_x",
    "wipe_tower_y",
    "curr_bed_type",
    "flush_multiplier",
    "belt_Z_offset"
};

std::vector<std::string> get_string_values(const DynamicConfig& config, const std::string& key)
{
    const ConfigOption* opt = config.option(key);
    const auto*         strings = dynamic_cast<const ConfigOptionStrings*>(opt);
    return strings ? strings->values : std::vector<std::string>();
}

size_t get_loaded_filament_count(const DynamicConfig& config)
{
    std::vector<std::string> filament_colours = get_string_values(config, "filament_colour");
    if (!filament_colours.empty())
        return filament_colours.size();

    std::vector<std::string> filament_ids = get_string_values(config, "filament_settings_id");
    if (!filament_ids.empty())
        return filament_ids.size();

    return 1;
}

std::set<std::string> parse_diff_keys(const std::vector<std::string>& different_values, size_t idx)
{
    std::set<std::string> result;
    std::vector<std::string> keys;
    if (idx < different_values.size())
        Slic3r::unescape_strings_cstyle(different_values[idx], keys);
    result.insert(keys.begin(), keys.end());
    return result;
}

std::set<std::string> preset_diff_keys_for_load(const std::set<std::string>& diff_keys)
{
    std::set<std::string> result = diff_keys;
    result.insert(simple_mode_preset_meta_keys.begin(), simple_mode_preset_meta_keys.end());
    return result;
}


std::string trim_copy(std::string value)
{
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

std::string strip_filament_printer_suffix(std::string value)
{
    value = trim_copy(std::move(value));
    const size_t at_pos = value.find('@');
    if (at_pos != std::string::npos)
        value = value.substr(0, at_pos);
    return trim_copy(std::move(value));
}

std::string normalize_filament_type_key(std::string value)
{
    value = trim_copy(std::move(value));
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool is_valid_filament_preset(const Preset* preset)
{
    return preset != nullptr && !preset->file.empty();
}

bool is_current_printer_candidate(
    const Preset& preset,
    const std::string& current_printer_name,
    const std::string& current_printer_settings_id)
{
    if (!is_valid_filament_preset(&preset) || preset.is_default || preset.is_external)
        return false;

    if (!current_printer_settings_id.empty() && boost::algorithm::iends_with(preset.name, " @" + current_printer_settings_id))
        return true;

    if (!preset.is_compatible)
        return false;

    const auto* compatible_printers = preset.config.option<ConfigOptionStrings>("compatible_printers");
    if (compatible_printers == nullptr || compatible_printers->values.empty())
        return true;

    return std::find(
               compatible_printers->values.begin(),
               compatible_printers->values.end(),
               current_printer_name) != compatible_printers->values.end();
}

const Preset* resolve_filament_preset_by_type_for_current_printer(
    PresetBundle& preset_bundle,
    const std::string& imported_filament_type,
    const std::string& current_printer_name,
    const std::string& current_printer_settings_id)
{
    const std::string normalized_type = normalize_filament_type_key(imported_filament_type);
    if (normalized_type.empty())
        return nullptr;

    const Preset* best_match = nullptr;
    int           best_score = std::numeric_limits<int>::min();
    for (const Preset& preset : preset_bundle.filaments.get_presets()) {
        if (!is_current_printer_candidate(preset, current_printer_name, current_printer_settings_id))
            continue;

        const std::string preset_filament_type = normalize_filament_type_key(preset.config.opt_string("filament_type", 0));
        if (preset_filament_type != normalized_type)
            continue;

        int score = 0;
        if (!current_printer_settings_id.empty() && boost::algorithm::iends_with(preset.name, " @" + current_printer_settings_id))
            score += 100;
        if (preset.is_system)
            score += 20;
        else if (preset.is_user())
            score += 10;
        if (preset.is_visible)
            score += 5;

        if (score > best_score) {
            best_score = score;
            best_match = &preset;
        }
    }

    return best_match;
}

std::string resolve_current_printer_settings_id(const PresetBundle& preset_bundle)
{
    const DynamicPrintConfig& printer_config = preset_bundle.printers.get_edited_preset().config;
    std::string printer_settings_id = printer_config.opt_string("printer_settings_id");
    if (printer_settings_id.empty())
        printer_settings_id = preset_bundle.printers.get_selected_preset_name();
    return printer_settings_id;
}

const Preset* resolve_filament_preset_for_current_printer(
    PresetBundle& preset_bundle,
    const std::string& imported_filament_name,
    const std::string& imported_filament_type,
    const std::string& current_printer_name,
    const std::string& current_printer_settings_id)
{
    const std::string base_name = strip_filament_printer_suffix(imported_filament_name);
    if (!base_name.empty()) {
        Preset* bare_preset = preset_bundle.filaments.find_preset(base_name, false, true);
        if (is_valid_filament_preset(bare_preset) && bare_preset->is_user())
            return bare_preset;

        if (!current_printer_settings_id.empty()) {
            const std::string preset_with_printer = base_name + " @" + current_printer_settings_id;
            if (Preset* preset = preset_bundle.filaments.find_preset(preset_with_printer, false, true);
                is_valid_filament_preset(preset))
                return preset;
        }

        if (is_valid_filament_preset(bare_preset))
            return bare_preset;
    }

    if (const Preset* type_match = resolve_filament_preset_by_type_for_current_printer(
            preset_bundle,
            imported_filament_type,
            current_printer_name,
            current_printer_settings_id);
        type_match != nullptr)
        return type_match;

    return nullptr;
}
void set_current_printer_compatibility(DynamicPrintConfig& config, const std::string& current_printer_name)
{
    if (current_printer_name.empty())
        return;

    config.option<ConfigOptionStrings>("compatible_printers", true)->values = { current_printer_name };
}

void apply_imported_key(DynamicPrintConfig& target_config, const DynamicPrintConfig& import_config, const std::string& key, size_t import_filament_idx = std::numeric_limits<size_t>::max())
{
    if (simple_mode_preset_meta_keys.find(key) != simple_mode_preset_meta_keys.end())
        return;

    const ConfigOption* import_opt = import_config.option(key);
    ConfigOption*       target_opt = target_config.option(key, false);
    if (import_opt == nullptr || target_opt == nullptr)
        return;

    if (import_filament_idx == std::numeric_limits<size_t>::max() || import_opt->is_scalar()) {
        target_opt->set(import_opt);
        return;
    }

    auto* target_vec = dynamic_cast<ConfigOptionVectorBase*>(target_opt);
    if (target_vec != nullptr)
        target_vec->set_at(import_opt, 0, import_filament_idx);
}

void apply_imported_diff_keys(DynamicPrintConfig& target_config, const DynamicPrintConfig& import_config, const std::set<std::string>& diff_keys, size_t import_filament_idx = std::numeric_limits<size_t>::max())
{
    for (const std::string& key : diff_keys)
        apply_imported_key(target_config, import_config, key, import_filament_idx);
}

void normalize_3mf_default_profile_names(DynamicPrintConfig& config)
{
    ConfigOptionStrings* filament_settings_id = config.opt<ConfigOptionStrings>("filament_settings_id");
    ConfigOptionStrings* default_filament_profile = config.opt<ConfigOptionStrings>("default_filament_profile");
    if (filament_settings_id != nullptr && default_filament_profile != nullptr && !default_filament_profile->values.empty()) {
        for (std::string& item : filament_settings_id->values)
            if (item == "Default Filament")
                item = default_filament_profile->values.front();
    }

    ConfigOptionString* print_settings_id = config.opt<ConfigOptionString>("print_settings_id");
    ConfigOptionString* default_print_profile = config.opt<ConfigOptionString>("default_print_profile");
    if (print_settings_id != nullptr && print_settings_id->value == "Default Setting" && default_print_profile != nullptr)
        print_settings_id->set(default_print_profile);
}

const Preset* find_simple_mode_filament_base(PresetBundle& preset_bundle, size_t filament_idx)
{
    if (filament_idx < preset_bundle.filament_presets.size()) {
        const Preset* preset = preset_bundle.filaments.find_preset(preset_bundle.filament_presets[filament_idx], true);
        if (preset != nullptr)
            return preset;
    }

    const Preset* preset = preset_bundle.filaments.find_preset(preset_bundle.filaments.get_selected_preset_name(), true);
    return preset != nullptr ? preset : &preset_bundle.filaments.first_visible();
}


const Preset* find_simple_mode_filament_base_from_import(
    PresetBundle& preset_bundle,
    const DynamicPrintConfig& import_config,
    size_t filament_idx,
    const std::string& current_printer_name,
    const std::string& current_printer_settings_id)
{
    const std::vector<std::string> imported_filament_names = get_string_values(import_config, "filament_settings_id");
    const std::vector<std::string> imported_filament_types = get_string_values(import_config, "filament_type");
    if (filament_idx < imported_filament_names.size() || filament_idx < imported_filament_types.size()) {
        const std::string imported_filament_name = filament_idx < imported_filament_names.size() ? imported_filament_names[filament_idx] : std::string();
        const std::string imported_filament_type = filament_idx < imported_filament_types.size() ? imported_filament_types[filament_idx] : std::string();
        if (const Preset* resolved = resolve_filament_preset_for_current_printer(
                preset_bundle,
                imported_filament_name,
                imported_filament_type,
                current_printer_name,
                current_printer_settings_id);
            resolved != nullptr) {
            BOOST_LOG_TRIVIAL(info)
                << __FUNCTION__
                << ": slot=" << filament_idx
                << ", imported_filament=" << imported_filament_name
                << ", imported_type=" << imported_filament_type
                << ", resolved_base=" << resolved->name
                << ", printer_settings_id=" << current_printer_settings_id;
            return resolved;
        }

        if (!imported_filament_name.empty() || !imported_filament_type.empty()) {
            BOOST_LOG_TRIVIAL(info)
                << __FUNCTION__
                << ": slot=" << filament_idx
                << ", imported_filament=" << imported_filament_name
                << ", imported_type=" << imported_filament_type
                << ", resolved_base=<fallback>"
                << ", printer_settings_id=" << current_printer_settings_id;
        }
    }

    return find_simple_mode_filament_base(preset_bundle, filament_idx);
}
void refresh_simple_mode_after_3mf_migration(PresetBundle& preset_bundle, size_t filament_count)
{
    preset_bundle.prints.update_dirty();
    preset_bundle.filaments.update_dirty();
    preset_bundle.printers.update_dirty();

    preset_bundle.update_compatible(PresetSelectCompatibleType::Never);
    preset_bundle.update_multi_material_filament_presets();

    Plater* plater = wxGetApp().plater();
    if (plater != nullptr) {
        plater->on_filaments_change(filament_count);

        if (ConfigOption* bed_type_opt = preset_bundle.project_config.option("curr_bed_type")) {
            BedType bed_type = static_cast<BedType>(bed_type_opt->getInt());
            if (wxGetApp().app_config != nullptr && preset_bundle.is_bbl_vendor())
                wxGetApp().app_config->set("curr_bed_type", std::to_string(int(bed_type)));
            plater->on_bed_type_change(bed_type);
        }

        plater->update_project_dirty_from_presets();
        plater->on_config_change(preset_bundle.full_config());
    }

    if (wxGetApp().app_config != nullptr)
        preset_bundle.export_selections(*wxGetApp().app_config);

    if (auto* tab = wxGetApp().get_tab(Preset::TYPE_PRINT)) {
        tab->update_dirty();
        tab->reload_config();
    }
    if (auto* tab = wxGetApp().get_tab(Preset::TYPE_FILAMENT)) {
        tab->update_dirty();
        tab->reload_config();
    }
    if (auto* tab = wxGetApp().get_tab(Preset::TYPE_PRINTER)) {
        tab->update_dirty();
        tab->reload_config();
    }
}

} // namespace

// Parse preset string such as: "Creality K2 Plus 0.4 nozzle"
// Returns: vendor_name, model_name, nozzle_diameter
// Note: if vendor_name contains multiple word like "Bambu Lab", this function might not suitable
static std::tuple<std::string, std::string, std::string> parse_device_preset(const std::string& device_preset)
{
    size_t last_space_pos = device_preset.find_last_of(' ');
    
    if (last_space_pos == std::string::npos) {
        // No spaces found, return entire string as vendor, empty model and nozzle
        return {device_preset, "", ""};
    }
    
    // Remove "nozzle" from the end
    std::string before_nozzle = device_preset.substr(0, last_space_pos);
    
    // Find the space before nozzle diameter
    size_t diameter_start_pos = before_nozzle.find_last_of(' ');
    
    if (diameter_start_pos == std::string::npos) {
        // No space found before diameter, return entire string as vendor
        return {device_preset, "", ""};
    }
    
    // Extract nozzle diameter (between last two spaces)
    std::string nozzle_diameter = before_nozzle.substr(diameter_start_pos + 1);
    
    // Extract vendor and model (everything before nozzle diameter)
    std::string vendor_and_model = device_preset.substr(0, diameter_start_pos);
    
    // Find first space to separate vendor and model
    size_t first_space_pos = vendor_and_model.find(' ');
    
    if (first_space_pos == std::string::npos) {
        // No space found, entire string is vendor name
        return {vendor_and_model, "", nozzle_diameter};
    }
    
    // Extract vendor name (before first space)
    std::string vendor_name = vendor_and_model.substr(0, first_space_pos);
    
    // Extract model name (after first space)
    std::string model_name = vendor_and_model.substr(first_space_pos + 1);
    
    return {vendor_name, model_name, nozzle_diameter};
}

void Simple_Device_List_Data::push(std::string name, Simple_Device_List_Item_Data item)
{ 
    // Cover textures are loaded lazily on the first frame each card is
    // rendered (see render_device_list_popup). Failed loads are not cached,
    // so a transient miss won't permanently turn the card into a white box.
    //if (cover2textureId.find(item.cover_path) == cover2textureId.end())
    //{
    //    auto& texture = wxGetApp().obj_list()->get_png_textures_simple().get(ObjectList::ObjList_Png_Texture_Wrapper::pngTexDeviceListIItem);
    //    //auto& texture = objPtr->m_png_textures->get(ObjList_Png_Texture_Wrapper::pngTexDeviceListIItem);
    //    texture->load_from_png_file(item.cover_path, true, GLTexture::None, false);
    //    cover2textureId.insert({item.cover_path, texture->get_id()});
    //    texture->reset(true); // delay until ~device_list_data() releases the id
    //}

    datas.insert({name, item}); 
    if (item.online)
        online_device_list.insert(name);
    else
        offline_device_list.insert(name);

    if (mac_2_key_map.find(item.mac) == mac_2_key_map.end())
    {
        mac_2_key_map[item.mac] = item.device_type == 1 ? std::pair{name, std::vector<std::string>{}} :
                                                          std::pair{"", std::vector<std::string>{name}};
    }
    else
    {
        if (item.device_type == 1)
            mac_2_key_map[item.mac].first = name;
        else
            mac_2_key_map[item.mac].second.push_back(name);
        manager_duplicate_deivce(mac_2_key_map[item.mac]);
    }
}

void Simple_Device_List_Data::manager_duplicate_deivce(std::pair<std::string, std::vector<std::string>>& cloud_local_pair)
{
    // duplicate removal
    std::string online_local_device_key;
    std::string cloud_key = cloud_local_pair.first;
    for (auto local_key : cloud_local_pair.second) {
        if (datas[local_key].online) {
            online_local_device_key = local_key;
        } else {
            datas[local_key].visible = false;
        }
    }
    if (online_local_device_key.empty())
    {
        auto local_key = cloud_local_pair.second[0];
        datas[local_key].visible = true;
        if (!cloud_key.empty())
        {
            datas[cloud_key].visible = true;
            datas[local_key].visible = false;
        }
    }
    else
    {
        datas[online_local_device_key].visible = true;
        if (!cloud_key.empty())
            datas[cloud_key].visible = false;
    }
}

//Simple_Device_List_Data::~Simple_Device_List_Data()
//{ 
//    for (auto it : cover2textureId)
//    {
//        if (it.second != GLTexture::INVAILD_ID)
//            glsafe(::glDeleteTextures(1, &it.second));
//    }
//}

SimpleDeviceMgr& SimpleDeviceMgr::instance()
{
    static SimpleDeviceMgr inst;
    return inst;
}

// check original logic from "Plater::priv::on_select_preset(wxCommandEvent& evt)"
bool SimpleDeviceMgr::auto_select_printer_preset(const std::string& printer_preset_name)
{
    Plater* plater = wxGetApp().plater();
    Sidebar&     sidebar     = plater->sidebar();
    Preset::Type preset_type = Preset::TYPE_PRINTER;

    std::string preset_name = printer_preset_name;
    PresetCollection& printers = wxGetApp().preset_bundle->printers;
    if (!printers.find_preset(preset_name, false, true)) {
        BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << ": printer preset not found: " << preset_name;
        return false;
    }

    if (1) {
        if (preset_type == Preset::TYPE_PRINTER) {
            PhysicalPrinterCollection& physical_printers = wxGetApp().preset_bundle->physical_printers;
            physical_printers.unselect_printer();
        }
        // BBS
        // wxWindowUpdateLocker noUpdates1(sidebar->print_panel());
        wxWindowUpdateLocker noUpdates2(sidebar.filament_panel());
        if (!wxGetApp().get_tab(preset_type)->select_preset(preset_name))
            return false;
    }

    // update plater with new config
    plater->on_config_change(wxGetApp().preset_bundle->full_config());

    if (preset_type == Preset::TYPE_PRINTER) {
        /* Settings list can be changed after printer preset changing, so
         * update all settings items for all item had it.
         * Furthermore, Layers editing is implemented only for FFF printers
         * and for SLA presets they should be deleted
         */
        wxGetApp().obj_list()->update_object_list_by_printer_technology();
    }
    return true;
}

std::string SimpleDeviceMgr::get_preset_name_by_current_device(bool& already_loaded)
{
    const DM::Device& cur_dev = DM::DataCenter::Ins().get_current_device_data();
    if (!cur_dev.valid)
        return "";

    PresetBundle& preset_bundle = *wxGetApp().preset_bundle;
    std::vector<std::string> items = wxGetApp().plater()->sidebar_printer().texts_of_combo_printer();

    // Exact model match avoids the substring trap where device model "K2"
    // matches "K2 Plus" / "K2 Pro". Falls back to legacy substring matching
    // only when no exact match exists.
    auto matches = [&](const std::string& preset_name, bool exact) {
        if (exact) {
            const std::string model = std::get<1>(parse_device_preset(preset_name));
            return boost::iequals(model, cur_dev.model) || boost::iequals(model, cur_dev.modelName);
        }
        return caseInsensitiveFind(preset_name, cur_dev.model) || caseInsensitiveFind(preset_name, cur_dev.modelName);
    };

    auto has_04_nozzle = [](const std::string& preset_name) {
        return caseInsensitiveFind(preset_name, " 0.4 ") ||
               preset_name.find(" 0.4 nozzle") != std::string::npos ||
               preset_name.find(" 0.4mm") != std::string::npos;
    };

    // Two passes: prefer exact model match, then fall back to substring match.
    for (bool exact : {true, false}) {
        // Prefer an already loaded (visible) preset.
        for (const auto& item : items) {
            if (matches(item, exact)) {
                already_loaded = true;
                return item;
            }
        }

        // Otherwise scan the whole collection, preferring a 0.4 nozzle preset.
        const Preset* found = nullptr;
        const Preset* found_04 = nullptr;
        for (const auto& preset : preset_bundle.printers.get_presets()) {
            if (!matches(preset.name, exact))
                continue;
            if (has_04_nozzle(preset.name)) {
                if (!found_04) found_04 = &preset;
            } else if (!found) {
                found = &preset;
            }
        }
        if (const Preset* chosen = found_04 ? found_04 : found) {
            already_loaded = chosen->is_visible;
            return chosen->name;
        }
    }

    return "";
}

bool SimpleDeviceMgr::is_current_device_match_current_preset()
{
    const DM::Device& cur_dev = DM::DataCenter::Ins().get_current_device_data();
    if (cur_dev.valid) {
        auto cur_preset_name = wxGetApp().preset_bundle->printers.get_selected_preset_name();

        bool already_loaded = false;
        auto device_related_preset = get_preset_name_by_current_device(already_loaded);
        if(device_related_preset.empty()) {
            return false;
        }

        if(device_related_preset != cur_preset_name) {
            return false;
        }

        return already_loaded ? true : false;

    }

    return false;
}

void SimpleDeviceMgr::clear()
{

}

void SimpleDeviceMgr::on_device_switch_and_check_preset_change()
{
    auto cur_preset_name = wxGetApp().preset_bundle->printers.get_selected_preset_name();

    bool already_loaded = false;
    auto device_related_preset = get_preset_name_by_current_device(already_loaded);

    wxBusyCursor busy;

    if(!device_related_preset.empty() && device_related_preset != cur_preset_name) {

        // Parse device preset string to get vendor, model, and nozzle diameter
        auto [vendor_name, device_related_model, device_nozzle] = parse_device_preset(device_related_preset);
        auto [current_vendor, current_model, current_nozzle] = parse_device_preset(cur_preset_name);
        
        // compare model type
        if (device_related_model != current_model) {
            if(!already_loaded) {
                GUI_App &app = wxGetApp();
                PresetBundle& preset_bundle = *wxGetApp().preset_bundle;

                // Resolve the actual system preset so we can register the variant using
                // the preset's real metadata instead of values parsed from its display
                // name. The vendor id, printer_model and printer_variant come straight
                // from the preset/vendor profile, which avoids mismatches when a model's
                // brand name differs from the vendor profile id (e.g. "SPARKX i7" presets
                // live under the "Creality" vendor profile). set_visible_from_appconfig()
                // looks up visibility by vendor->id, so the key written here must match.
                const Preset* device_preset = preset_bundle.printers.find_preset(device_related_preset, false, true);

                std::string config_vendor   = vendor_name;
                std::string config_model     = vendor_name + " " + device_related_model;
                std::string config_variant   = device_nozzle;

                if (device_preset != nullptr) {
                    if (device_preset->vendor != nullptr && !device_preset->vendor->id.empty())
                        config_vendor = device_preset->vendor->id;

                    const std::string preset_model   = device_preset->config.opt_string("printer_model");
                    const std::string preset_variant = device_preset->config.opt_string("printer_variant");
                    if (!preset_model.empty())
                        config_model = preset_model;
                    if (!preset_variant.empty())
                        config_variant = preset_variant;
                }

                app.app_config->set_variant(config_vendor, config_model, config_variant, "true");

                std::string nozzle_diameter = config_variant.empty() ? "0.4" : config_variant;
                // PresetPreferences matches by printer_model id, which is the full model
                // name (e.g. "SPARKX i7"), not the brand-stripped token.
                std::string preferred_model = config_model;

                preset_bundle.load_presets(*app.app_config, 
                    ForwardCompatibilitySubstitutionRule::EnableSilentDisableSystem,
                    { preferred_model, nozzle_diameter, "", std::string() });

                // Update the selections from the compatibility.
                preset_bundle.export_selections(*app.app_config);
            }

            auto_change_printer_preset(device_related_preset);
        }
    }
}

void SimpleDeviceMgr::auto_change_printer_preset(const std::string& preset_name)
{
    Plater* plater = wxGetApp().plater();
    Model& model = plater->model();
    Sidebar& sidebar = plater->sidebar();
    View3D*  view3D  = plater->get_vew3D();

    Preset::Type preset_type = Preset::TYPE_PRINTER;

    PartPlateList&   old_plate_list = plater->get_partplate_list();
    PartPlate*       old_plate      = old_plate_list.get_selected_plate();
    Vec3d            old_plate_pos  = old_plate->get_center_origin();

    std::vector<int> out_plate_obj_idxs;
    out_plate_obj_idxs.clear();
    std::vector<int> tmp_in_plate_obj_idxs;

    std::vector<std::vector<int>> all_plate_objects;

    for (size_t i = 0; i < old_plate_list.get_plate_count(); ++i) {
        PartPlate*       plate = old_plate_list.get_plate(i);
        std::vector<int> obj_idxs;
        for (int obj_idx = 0; obj_idx < model.objects.size(); obj_idx++) {
            if (plate && plate->contain_instance(obj_idx, 0)) {
                obj_idxs.emplace_back(obj_idx);
                tmp_in_plate_obj_idxs.emplace_back(obj_idx);
            }
        }

        if(!obj_idxs.empty()) {
            all_plate_objects.emplace_back(obj_idxs);
        }
        
    }

    for (int k = 0; k < model.objects.size(); k++) {
        if (std::find(tmp_in_plate_obj_idxs.begin(), tmp_in_plate_obj_idxs.end(), k) == tmp_in_plate_obj_idxs.end()) {
            out_plate_obj_idxs.emplace_back(k);
        }
    }

    if (!auto_select_printer_preset(preset_name))
        return;

    if (1) {
        // BBS:Model reset by plate center
        PartPlateList& cur_plate_list = plater->get_partplate_list();
        PartPlate*     cur_plate      = cur_plate_list.get_curr_plate();
        Vec3d          cur_plate_pos  = cur_plate->get_center_origin();
        if (1) {

            for (int i = 0; i < all_plate_objects.size(); ++i) {
                view3D->select_object_from_idx(all_plate_objects[i]);
                sidebar.obj_list()->update_selections();
                view3D->center_selected_plate(i);
            }
            // Centering relies on selecting each plate's objects; clear the
            // selection afterwards so switching printer model doesn't leave the
            // model (and the object toolbar/gizmos) selected.
            view3D->get_canvas3d()->deselect_all();
            sidebar.obj_list()->update_selections();
            // if in the original 3mf, there are some ModelObjects  out of any plate;  need to put them on top of first plate
            PartPlate* first_plate = cur_plate_list.get_plate(0);
            if (!first_plate)
                return;
            BoundingBoxf3 plate_bbox = first_plate->get_build_volume();

            for (int k = 0; k < out_plate_obj_idxs.size(); k++) {
                int object_idx = out_plate_obj_idxs[k];
                if (object_idx < 0 || object_idx >= model.objects.size())
                    continue;
                ModelObject* mo = model.objects[object_idx];
                if (!mo)
                    continue;
                for (ModelInstance* mi : mo->instances) {
                    Vec3d instance_bbox_size = mi->get_object()->instance_bounding_box(0).size();
                    auto  offset             = mi->get_offset();
                    Vec3d top_left = {plate_bbox.min.x() + instance_bbox_size.x(), plate_bbox.max.y() + instance_bbox_size.y(), offset(2)};
                    mi->set_offset(top_left);
                }
            }

        }
    }

}

bool SimpleDeviceMgr::check_device_change_and_cache()
{
    const DM::Device& device = DM::DataCenter::Ins().get_current_device_data();

    if (!device.valid) {
        m_current_device_cache.valid = false;
        return false;
    }
    // Compare with cached info
    if (m_current_device_cache.valid && m_current_device_cache.mac == device.mac && m_current_device_cache.name == device.name &&
        m_current_device_cache.addr == device.address && m_current_device_cache.model == device.modelName &&
        m_current_device_cache.online == static_cast<int>(device.online) && m_current_device_cache.state == device.deviceState )
    {
        return false; // no change
    }
    // Update cache
    m_current_device_cache.valid  = true;
    m_current_device_cache.mac    = device.mac;
    m_current_device_cache.name   = device.name;
    m_current_device_cache.addr   = device.address;
    m_current_device_cache.model  = device.modelName;
    m_current_device_cache.online = device.online;
    m_current_device_cache.state  = device.deviceState;

    check_current_preset_match_printer();

    return true;
}

bool SimpleDeviceMgr::is_printer_preset_changed()
{
    PresetBundle& preset_bundle       = *wxGetApp().preset_bundle;
    const int  cur_preset_idx = preset_bundle.printers.get_selected_idx();
    if (m_last_selected_preset_idx != cur_preset_idx) {
        m_last_selected_preset_idx = cur_preset_idx;
        return true;
    }

    return false;
}

bool SimpleDeviceMgr::switch_current_device_simple(const std::string& mac, bool sync_printer_preset)
{
    if (mac.empty() || !wxGetApp().mainframe || !wxGetApp().plater())
        return false;

    auto* printer_mgr_view = wxGetApp().mainframe->get_printer_mgr_view();
    if (!printer_mgr_view)
        return false;

    const DM::Device& cur_dev = DM::DataCenter::Ins().get_current_device_data();
    const bool is_new_device = (!cur_dev.valid || cur_dev.mac != mac);
    bool device_switch_applied = false;

    if (is_new_device) {
        auto* canvas = wxGetApp().plater()->get_view3D_canvas3D();
        if (canvas) {
            auto* panel = canvas->get_filament_panel();
            if (panel)
                panel->reset();
        }

        printer_mgr_view->requeset_set_current_device(mac);

        device_switch_applied = DM::DataCenter::Ins().set_current_device(mac);
        if (device_switch_applied) {
            m_device_list_dirty_mark = true;
            wxPostEvent(wxGetApp().plater(), wxCommandEvent(EVT_CURRENT_DEVICE_CHANGED));
        }
    }

    if (sync_printer_preset) {
        wxGetApp().CallAfter([]() {
            auto& mgr = SimpleDeviceMgr::instance();
            if (!mgr.check_device_change_and_cache())
                mgr.on_device_switch_and_check_preset_change();
        });
    }

    if (device_switch_applied) {
        const std::string switched_mac = mac;
        const bool switched_sync_printer_preset = sync_printer_preset;
        wxGetApp().CallAfter([switched_mac, switched_sync_printer_preset]() {
            const DM::Device& current_device = DM::DataCenter::Ins().get_current_device_data();
            nlohmann::json payload = {
                {"source", "current_device_switch"},
                {"reason", "current_device_changed"},
                {"mac", switched_mac},
                {"sync_printer_preset", switched_sync_printer_preset}
            };

            if (current_device.valid) {
                payload["device"] = {
                    {"name", current_device.name},
                    {"address", current_device.address},
                    {"mac", current_device.mac},
                    {"model_name", current_device.modelName},
                    {"online", current_device.online},
                    {"state", current_device.deviceState}
                };
            }

            NotifyAIChatProjectWorkflowResetRequested(payload);
        });
    }

    return true;
}

void SimpleDeviceMgr::ensure_current_device_initialized()
{
    // When there is no current device yet, request the frontend to select a default device.
    const DM::Device& cur_dev = DM::DataCenter::Ins().get_current_device_data();
    if (cur_dev.valid)
        return;

    auto devicesData = DM::DataCenter::Ins().get_data();
    auto printerData = devicesData["data"];
    for (const auto& group : printerData["printerList"]) 
    {
        if (!group.contains("list")) 
        {
            continue;
        }
        for (const auto& printer : group["list"]) 
        {
            DM::Device device = DM::Device::deserialize(const_cast<nlohmann::json&>(printer));

            if (!device.mac.empty()) {
                wxGetApp().mainframe->get_printer_mgr_view()->requeset_set_current_device(device.mac);
                return;
            }
        }
    }
}

bool SimpleDeviceMgr::get_cur_device_info(std::string& out_title,
                                          std::string& out_subtitle,
                                          std::string& out_icon_path)
{
    static const std::string default_title    = _u8L("Current No Device");
    static const std::string default_subtitle = _u8L("Please add printer");
    static const std::string       default_icon     = Slic3r::resources_dir() + "/images/current_no_device_simple.svg";

    const DM::Device& cur_dev = DM::DataCenter::Ins().get_current_device_data();
    bool refreshed = false;

    if (!cur_dev.valid) {
        ensure_current_device_initialized();
        if (m_current_device_cache.valid ||
            m_current_device_cache.title != default_title ||
            m_current_device_cache.subtitle != default_subtitle) 
        {
            m_current_device_cache.valid     = false;
            m_current_device_cache.title     = default_title;
            m_current_device_cache.subtitle  = default_subtitle;
            m_current_device_cache.icon_path = default_icon;
            refreshed = true;
        }
    } else {
        const bool device_changed = check_device_change_and_cache();

        std::string desired_title;
        if (!cur_dev.name.empty())
            desired_title = cur_dev.name;
        else if (!cur_dev.address.empty())
            desired_title = cur_dev.address;
        else
            desired_title = cur_dev.mac;

        std::string status_txt = cur_dev.online
            ? ((cur_dev.deviceState == 0) ? _u8L("Idle") : _u8L("Printing"))
            : _u8L("Offline");

        std::string desired_subtitle;
        if (!cur_dev.modelName.empty())
            desired_subtitle = cur_dev.modelName + " " + status_txt;
        else
            desired_subtitle = status_txt;

        std::string desired_icon_path;
        const auto& dl = get_device_list_data_simple(false);
        for (const auto& kv : dl.datas) {
            const auto& it = kv.second;
            if (it.mac == cur_dev.mac) {
                desired_icon_path = it.cover_path;
                break;
            }
        }
        if (desired_icon_path.empty())
            desired_icon_path = default_icon;

        if (device_changed ||
            m_current_device_cache.title != desired_title ||
            m_current_device_cache.subtitle != desired_subtitle ||
            m_current_device_cache.icon_path != desired_icon_path)
        {
            m_current_device_cache.title = desired_title;
            m_current_device_cache.subtitle = desired_subtitle;
            m_current_device_cache.icon_path = desired_icon_path;
            refreshed = true;
        }

    }

    out_title    = m_current_device_cache.title.empty()    ? default_title    : m_current_device_cache.title;
    out_subtitle = m_current_device_cache.subtitle.empty() ? default_subtitle : m_current_device_cache.subtitle;
    out_icon_path= m_current_device_cache.icon_path.empty()? default_icon     : m_current_device_cache.icon_path;

    return refreshed;
}

void SimpleDeviceMgr::update_printer_device_list_data_simple(std::string vendor, bool bForce)
{
    if (!bForce && !m_device_list_dirty_mark)
        return;
    rebuild_device_list(m_simple_device_list_data, vendor, [](const DM::Device&) { return true; });
}

void SimpleDeviceMgr::update_same_model_printer_device_list_data(std::string vendor, bool bForce)
{
    if (!bForce && !m_device_list_dirty_mark)
        return;
    PresetBundle& preset_bundle       = *wxGetApp().preset_bundle;
    auto          preset              = preset_bundle.printers.get_edited_preset();
    auto          current_printer_model = preset.config.opt_string("printer_model");

    rebuild_device_list(m_same_model_device_list_data, vendor,
        [=](const DM::Device& dev){
            auto printer_model = vendor + " " + dev.modelName;
            if (printer_model != current_printer_model && dev.modelName != current_printer_model) {
                if (dev.oldPrinter && dev.name == "Morandi" && current_printer_model == "Creality Sermoon D3 Pro") {
                    return true;
                }
                else
                {
                    return false;
                }
            }

            return true;

        });
}

const Simple_Device_List_Data& SimpleDeviceMgr::get_device_list_data_simple(bool force_refresh)
{
    // Vendor currently fixed as Creality for this UI flow
    update_printer_device_list_data_simple("Creality", force_refresh);
    return m_simple_device_list_data;
}

bool SimpleDeviceMgr::set_cur_device_by_cur_preset_simple()
{
    std::string selected_mac;
    auto        cur_preset        = wxGetApp().preset_bundle->printers.get_selected_preset();
    auto        cur_preset_config = cur_preset.config;
    if (wxGetApp().preset_bundle->printers.get_selected_preset().is_system) {
        auto cache         = EasyCache::get_instance().data();
        auto printer_model = cur_preset_config.opt_string("printer_model");
        if (cache.contains("system_preset_bundle_deivce") && cache["system_preset_bundle_deivce"].contains(printer_model)) {
            std::string json_key   = "unique";
            auto        nozzle_dia = cur_preset_config.opt_serialize("nozzle_diameter");
            if (!nozzle_dia.empty())
                json_key = nozzle_dia;

            if (cache["system_preset_bundle_deivce"][printer_model].contains(json_key))
                selected_mac = cache["system_preset_bundle_deivce"][printer_model][json_key];
        }
    } else {
        if (cur_preset_config.has("printer_select_mac"))
            selected_mac = cur_preset_config.opt_string("printer_select_mac");
    }

    if (selected_mac.empty()) {
        update_same_model_printer_device_list_data("Creality", true);
        // Prefer online devices; fall back to any device if needed.
        for (const auto& it : m_same_model_device_list_data.datas) {
            if (it.second.online) {
                selected_mac = it.second.mac;
                break;
            }
        }
        if (selected_mac.empty() && !m_same_model_device_list_data.datas.empty()) {
            selected_mac = m_same_model_device_list_data.datas.begin()->second.mac;
        }

    }

    if(!selected_mac.empty()) {
        wxGetApp().mainframe->get_printer_mgr_view()->requeset_set_current_device(selected_mac);
    }
    else
    {
        check_current_preset_match_printer();
    }
    

    return !selected_mac.empty();
}


template<typename Predicate>
void SimpleDeviceMgr::rebuild_device_list(Simple_Device_List_Data& out, const std::string& vendor, Predicate pred)
{
    std::srand(std::time(nullptr));
    m_device_list_dirty_mark = false;
    auto devicesData = DM::DataCenter::Ins().GetData();
    auto printerData = devicesData["data"];
    //const auto& model2CoverMap = wxGetApp().app_config->get_model2cover_path();
    const auto& printer_cover_map = wxGetApp().app_config->get_model2cover_path();
    PresetBundle& preset_bundle       = *wxGetApp().preset_bundle;
    auto          preset              = preset_bundle.printers.get_edited_preset();
    auto          current_printer_model = preset.config.opt_string("printer_model");

    out.clear();
    for (const auto& group : printerData["printerList"])
    {
        if (!group.contains("list"))
            continue;
        for (const auto& printer : group["list"])
        {
            DM::Device device = DM::Device::deserialize(const_cast<nlohmann::json&>(printer));
            if (!pred(device))
                continue;

            // model img
            auto printer_model = vendor + " " + device.modelName;

            auto coverPath = Slic3r::resources_dir() + "/images/printer_default.png";
            {
                auto iter = printer_cover_map.find(printer_model);
                if (iter == printer_cover_map.end()) {
                    iter = printer_cover_map.find(device.modelName);
                }
                if (iter != printer_cover_map.end()) {
                    coverPath = iter->second;
                }
            }

            bool              is_current     = false;
            const DM::Device& current_device = DM::DataCenter::Ins().get_current_device_data();
            if (current_device.valid)
                is_current = current_device.mac == device.mac;

            auto key_name = device.name.empty() ? (device.modelName + device.mac + device.address) : device.name;
            if (device.deviceType == 1)
                key_name += "_##CXYDevice##_" + std::to_string(std::rand());

            out.push(key_name,
                {device.name, coverPath, device.modelName,
                device.address, device.mac, device.deviceState,
                device.online, is_current, device.deviceType});
        }
    }
}

void SimpleDeviceMgr::check_current_preset_match_printer()
{
    auto        cur_preset        = wxGetApp().preset_bundle->printers.get_selected_preset();
    auto        cur_preset_config = cur_preset.config;
    const DM::Device& cur_dev = DM::DataCenter::Ins().get_current_device_data();
    if (cur_dev.valid) {
        std::string cur_printer_model = cur_preset_config.opt_string("printer_model");

        if(!caseInsensitiveFind(cur_printer_model, cur_dev.model) && !caseInsensitiveFind(cur_printer_model, cur_dev.modelName)) {

            on_device_switch_and_check_preset_change();

        }
    }
}

std::string SimpleDeviceMgr::get_same_model_device_mac_with_current_preset()
{
    update_same_model_printer_device_list_data("Creality", true);

    for (const auto& it : m_same_model_device_list_data.datas) {
        if (it.second.online) {
            return it.second.mac;
        }
    }

    return "";
}

bool SimpleDeviceMgr::check_need_to_change_current_preset(const Slic3r::DynamicConfig& loaded_config, const Slic3r::PlateDataPtrs& plate_datas, Slic3r::Model& model)
{
    try
    {
        std::string printer_model_name = loaded_config.opt_string("printer_model");
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__
                                << ": migrate 3mf to current simple-mode printer, source printer_model="
                                << printer_model_name;

        check_diff_settings_to_system(loaded_config);

        Slic3r::GUI::SimpleModelMgr::instance().mark_3mf_plate_objects(plate_datas, model);

        return false;

    }
    catch (const std::exception& e)
    {
        BOOST_LOG_TRIVIAL(warning) << __FUNCTION__
                                   << ": failed to migrate 3mf settings in simple mode, keep current printer. "
                                   << e.what();
        return false;
    }

    return false;
}

void SimpleDeviceMgr::check_diff_settings_to_system(const Slic3r::DynamicConfig& loaded_config)
{
    PresetBundle& preset_bundle = *wxGetApp().preset_bundle;

    DynamicPrintConfig import_config(loaded_config);
    normalize_3mf_default_profile_names(import_config);

    const size_t loaded_filament_count = std::max<size_t>(1, get_loaded_filament_count(import_config));
    std::vector<std::string> different_values = get_string_values(import_config, "different_settings_to_system");
    different_values.resize(loaded_filament_count + 2, std::string());

    const std::string current_printer_name = preset_bundle.printers.get_selected_preset_name();
    const std::string current_printer_settings_id = resolve_current_printer_settings_id(preset_bundle);
    const std::string import_name = "simple-mode-3mf-import.3mf";

    const Preset&      current_print_preset = preset_bundle.prints.get_edited_preset();
    DynamicPrintConfig print_config(current_print_preset.config);
    const std::set<std::string> print_diff_keys = parse_diff_keys(different_values, 0);
    apply_imported_diff_keys(print_config, import_config, print_diff_keys);
    print_config.option<ConfigOptionString>("inherits", true)->value = current_print_preset.name;
    set_current_printer_compatibility(print_config, current_printer_name);

    preset_bundle.prints.load_external_preset(
        import_name,
        import_name,
        std::string(),
        print_config,
        preset_diff_keys_for_load(print_diff_keys),
        PresetCollection::LoadAndSelect::Always);

    preset_bundle.project_config.apply_only(import_config, simple_mode_project_options, true);

    if (preset_bundle.filament_presets.empty())
        preset_bundle.filament_presets.emplace_back(preset_bundle.filaments.get_selected_preset_name());
    if (preset_bundle.filament_presets.empty())
        preset_bundle.filament_presets.emplace_back(preset_bundle.filaments.first_visible().name);
    preset_bundle.filament_presets.resize(loaded_filament_count, preset_bundle.filament_presets.back());

    std::vector<std::string> migrated_filament_presets(loaded_filament_count);
    std::vector<std::string> filament_ids = get_string_values(import_config, "filament_ids");
    filament_ids.resize(loaded_filament_count, std::string());

    for (int i = int(loaded_filament_count) - 1; i >= 0; --i) {
        const Preset* base_filament = find_simple_mode_filament_base_from_import(
            preset_bundle,
            import_config,
            size_t(i),
            current_printer_name,
            current_printer_settings_id);
        const std::string base_filament_name = base_filament->name;
        DynamicPrintConfig filament_config(base_filament->config);
        const std::set<std::string> filament_diff_keys = parse_diff_keys(different_values, size_t(i) + 1);
        apply_imported_diff_keys(filament_config, import_config, filament_diff_keys, size_t(i));

        if (filament_diff_keys.empty()) {
            migrated_filament_presets[size_t(i)] = base_filament_name;
            continue;
        }

        filament_config.option<ConfigOptionString>("inherits", true)->value = base_filament_name;
        set_current_printer_compatibility(filament_config, current_printer_name);

        const std::string external_filament_name = strip_filament_printer_suffix(base_filament_name);

        auto loaded_result = preset_bundle.filaments.load_external_preset(
            import_name,
            external_filament_name.empty() ? base_filament_name : external_filament_name,
            base_filament_name,
            filament_config,
            preset_diff_keys_for_load(filament_diff_keys),
            i == 0 ? PresetCollection::LoadAndSelect::Always : PresetCollection::LoadAndSelect::Never,
            Semver(),
            filament_ids[size_t(i)]);

        Preset* loaded = loaded_result.first;
        migrated_filament_presets[size_t(i)] = loaded != nullptr ? loaded->name : base_filament_name;
    }
    preset_bundle.filament_presets = migrated_filament_presets;

    DynamicPrintConfig& current_printer_config = preset_bundle.printers.get_edited_preset().config;
    const std::set<std::string> printer_diff_keys = parse_diff_keys(different_values, loaded_filament_count + 1);
    apply_imported_diff_keys(current_printer_config, import_config, printer_diff_keys);

    BOOST_LOG_TRIVIAL(info) << __FUNCTION__
                            << ": migrated simple-mode 3mf settings to current printer="
                            << current_printer_name
                            << ", filaments=" << loaded_filament_count
                            << ", print_diff_keys=" << print_diff_keys.size()
                            << ", printer_diff_keys=" << printer_diff_keys.size();

    refresh_simple_mode_after_3mf_migration(preset_bundle, loaded_filament_count);
}


void render_device_list_popup(GLCanvas3D& canvas, float /*x*/, float /*y*/, float /*bottom_limit*/)
{
    const float scale = canvas.get_scale();
    const bool  dark  = wxGetApp().dark_mode();

    // Title
    ImGui::PushStyleColor(ImGuiCol_Text, dark ? ImVec4(1,1,1,1) : ImVec4(0.1686f,0.1686f,0.1765f,1.0f));
    const float base_font_px = ImGui::GetFontSize();
    const float title_px     = 16.0f * scale;
    const float prev_scale   = 1.0f;
    ImGui::SetWindowFontScale(title_px / base_font_px);
    ImGui::TextUnformatted(_u8L("Device Lists").c_str());
    ImGui::SetWindowFontScale(prev_scale);
    ImGui::PopStyleColor();

    {
        //const std::string btn_label = _u8L("Select/Remove printers(system presets)");
        const std::string btn_label   = _u8L(" add printer");
        ImVec2            text_sz   = ImGui::CalcTextSize(btn_label.c_str());

        const float win_w   = ImGui::GetWindowSize().x;
        const float marginX = 12.0f * scale; // move link a bit left
        float       start_x = win_w - text_sz.x - marginX;
        if (start_x < 0.0f)
            start_x = 0.0f;

        ImGui::SameLine();
        ImGui::SetCursorPosX(start_x);

        ImVec2 link_pos_screen = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##select_remove_printers_simple", text_sz);
        bool link_clicked = ImGui::IsItemClicked();

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImU32 link_col = IM_COL32(21,192,89,255);
        dl->AddText(link_pos_screen, link_col, btn_label.c_str());

        if (ImGui::IsItemHovered()) {
            ImVec2 minp = link_pos_screen;
            ImVec2 maxp = ImVec2(link_pos_screen.x + text_sz.x, link_pos_screen.y + text_sz.y);
            dl->AddLine(ImVec2(minp.x, maxp.y), ImVec2(maxp.x, maxp.y), link_col, 1.0f);
        }

        if (link_clicked) {
            //wxTheApp->CallAfter([]() {
            //    wxGetApp().run_wizard(ConfigWizard::RR_USER, ConfigWizard::SP_PRINTERS);
            //    SimpleDeviceMgr::instance().ensure_current_device_initialized();
            //});

            if (wxGetApp().mainframe)
                wxGetApp().mainframe->select_tab((int) Slic3r::GUI::MainFrame::tpDeviceMgr);

            //EasyPrintSender sender;
            //sender.openLoginPage();
        }
    }

    ImGui::Dummy(ImVec2(0, 6.0f * scale));

    const auto& data = SimpleDeviceMgr::instance().get_device_list_data_simple(false);

    // If there is no device, shrink the popup size for a compact guide view.
    if (data.online_device_list.empty() && data.offline_device_list.empty()) {
        ImGui::SetWindowSize(ImVec2(420.0f * scale, 180.0f * scale), ImGuiCond_Always);
    }

    // If no devices to show, guide user to add a device.
    if (data.online_device_list.empty() && data.offline_device_list.empty()) {
        const float panel_w = ImGui::GetWindowSize().x;
        const std::string line1  = _u8L("Current No Device");
        const std::string line11 = _u8L("Please");
        const std::string link   = _u8L(" add printer");

        // Adaptive text color per theme (compute locally)
        ImGui::PushStyleColor(ImGuiCol_Text, dark ? ImVec4(1,1,1,1) : ImVec4(0.1686f,0.1686f,0.1765f,1.0f));

        // Single-line: "<line1>  <line11><link>" at 16px font size
        const float base_px   = ImGui::GetFontSize();
        const float guide_px  = 16.0f * scale;
        ImGui::SetWindowFontScale(guide_px / base_px);
        ImVec2 sz1  = ImGui::CalcTextSize(line1.c_str());
        ImVec2 sz11 = ImGui::CalcTextSize(line11.c_str());
        ImVec2 szL  = ImGui::CalcTextSize(link.c_str());
        const float gap_a = 8.0f * scale;
        const float total_w = sz1.x + gap_a + sz11.x + szL.x;
        ImGui::Dummy(ImVec2(0, 14.0f * scale));
        float start_x = std::max(0.0f, 0.5f * (panel_w - total_w));
        ImGui::SetCursorPosX(start_x);
        ImGui::TextUnformatted(line1.c_str());
        ImGui::SameLine(0.0f, gap_a);
        ImGui::TextUnformatted(line11.c_str());
        ImGui::SameLine(0.0f, 0.0f);

        // Draw a green, clickable text for "add printer"
        ImVec2 link_pos_screen = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##add_printer_link_simple", szL);
        bool link_clicked = ImGui::IsItemClicked();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImU32 link_col = IM_COL32(21,192,89,255);
        dl->AddText(link_pos_screen, link_col, link.c_str());
        if (ImGui::IsItemHovered()) {
            // Optional underline on hover
            ImVec2 minp = link_pos_screen;
            ImVec2 maxp = ImVec2(link_pos_screen.x + szL.x, link_pos_screen.y + szL.y);
            dl->AddLine(ImVec2(minp.x, maxp.y), ImVec2(maxp.x, maxp.y), link_col, 1.0f);
        }
        if (link_clicked) {
            if (wxGetApp().mainframe)
                wxGetApp().mainframe->select_tab((int) Slic3r::GUI::MainFrame::tpDeviceMgr);
        }
        // restore window font scale
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();
        return;
    }

    // Layout: keep up to 3 cards per row, but shrink/flow inside the current popup width.
    const float pad      = 12.0f * scale;
    const float radius   = 8.0f  * scale;
    const float min_card_w = 150.0f * scale;
    const float card_gap_x = 20.0f * scale;
    const float cell_gap_x = 0.5f * card_gap_x;
    const float cell_gap_y = 8.0f * scale;
    const float avail_w    = std::max(1.0f, ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ScrollbarSize - 4.0f * scale);
    const int columns = std::min(3, std::max(1, static_cast<int>((avail_w + card_gap_x) / (min_card_w + card_gap_x))));
    const float card_w = std::max(min_card_w, (avail_w - card_gap_x * static_cast<float>(columns - 1)) / static_cast<float>(columns));
    const float img_w  = std::max(1.0f, card_w - pad);
    const float img_h  = img_w;
    const float card_h = img_h + 56.0f * scale;

    const ImU32 br_grey  = dark ? IM_COL32(110,110,114,200) : IM_COL32(192,192,200,200);
    const ImU32 br_green = IM_COL32(41,190,85,255);
    const ImU32 text_col = ImGui::GetColorU32(dark ? ImVec4(1,1,1,1) : ImVec4(0.1686f,0.1686f,0.1765f,1));
    const ImU32 status_free = IM_COL32(21,192,89,255);
    const ImU32 status_busy = IM_COL32(56,125,255,255);
    const ImU32 status_off  = IM_COL32(255,125,0,255);

    // Ordered keys: online first
    std::vector<std::string> keys;
    keys.reserve(data.online_device_list.size() + data.offline_device_list.size());
    for (const auto& k : data.online_device_list)  keys.push_back(k);
    for (const auto& k : data.offline_device_list) keys.push_back(k);

    static std::unordered_map<std::string, ImTextureID> s_tex;

    // Increase spacing between cards via table cell padding
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(cell_gap_x, cell_gap_y));
    if (ImGui::BeginTable("##device_grid", columns, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_NoPadOuterX)) {
        for (int i = 0; i < columns; ++i)
            ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, card_w);
        int col = 0;
        for (const auto& key : keys) {
            auto it = data.datas.find(key);
            if (it == data.datas.end()) continue;
            const auto& item = it->second;
            if (!item.visible) continue;

            if (col == 0) ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(col);
            col = (col + 1) % columns;

            ImVec2 tl = ImGui::GetCursorScreenPos();
            ImVec2 br = ImVec2(tl.x + card_w, tl.y + card_h);
            ImDrawList* dl = ImGui::GetWindowDrawList();
            dl->AddRectFilled(tl, br, IM_COL32(255,255,255,255), radius);
            dl->AddRect(tl, br, item.isCurrent ? br_green : br_grey, radius, 0, 1.0f);

            ImGui::InvisibleButton(("##dev_card_" + key).c_str(), ImVec2(card_w, card_h));
            bool clicked = ImGui::IsItemClicked();

            // Round status button at top-right
            const float r_outer = 8.0f * scale;
            const float margin  = 8.0f  * scale;      // move closer to the card corner
            ImVec2 rc = ImVec2(br.x - margin - r_outer, tl.y + margin + r_outer);
            bool is_current_draw = item.isCurrent;
            {
                const DM::Device& cur = DM::DataCenter::Ins().get_current_device_data();
                if (cur.valid) {
                    is_current_draw = (cur.mac == item.mac) && (cur.deviceType == item.device_type);
                }
            }
            dl->AddCircle(rc, r_outer, is_current_draw ? br_green : br_grey, 0, 2.0f);
            if (is_current_draw) dl->AddCircleFilled(rc, r_outer - 4.0f * scale, br_green);

            // Image (no button background)
            ImVec2 img_tl = ImVec2(tl.x + 0.5f * (card_w - img_w), tl.y + pad + 4.0f * scale);
            ImTextureID tex = nullptr;
            auto itex = s_tex.find(item.cover_path);
            if (itex != s_tex.end()) {
                tex = itex->second;
            } else {
                ImTextureID id = nullptr;
                // Don't poison the cache on failure: only remember successful
                // loads. A failed load (missing file, transient GL hiccup,
                // path encoding issue) will simply re-attempt next frame
                // instead of leaving a permanent white card.
                if (IMTexture::load_from_png_file(item.cover_path, (unsigned)img_w, (unsigned)img_h, id) && id != nullptr) {
                    s_tex[item.cover_path] = id;
                    tex = id;
                }
            }
            if (tex) { ImGui::SetCursorScreenPos(img_tl); ImGui::Image(tex, ImVec2(img_w, img_h)); }

            // Model label
            const std::string model_label = item.model_name;
            const float base_px  = ImGui::GetFontSize();
            const float text_px  = 16.0f * scale;
            const float scale_px = (base_px > 0.0f) ? (text_px / base_px) : 1.0f;
            ImVec2 raw_sz  = ImGui::CalcTextSize(model_label.c_str());
            ImVec2 label_sz = ImVec2(raw_sz.x * scale_px, raw_sz.y * scale_px);
            ImVec2 label_pos = ImVec2(tl.x + 0.5f * (card_w - label_sz.x), img_tl.y + img_h + 6.0f * scale);
            dl->AddText(ImGui::GetFont(), text_px, label_pos, IM_COL32(150,150,155,255), model_label.c_str());

            // Device name
            ImGui::SetCursorScreenPos(ImVec2(tl.x, br.y + 6.0f * scale));
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(text_col));
            ImGui::TextUnformatted(item.name.c_str());
            ImGui::PopStyleColor();

            // Status text (fix lifetime issues by using std::string)
            ImU32 st_col = status_off;
            std::string st_txt = _u8L("Offline");
            if (item.online) {
                if (item.state == 0) { st_col = status_free; st_txt = _u8L("Idle"); } else { st_col = status_busy; st_txt = _u8L("Printing"); }
            }
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(st_col));
            ImGui::TextUnformatted(st_txt.c_str());
            ImGui::PopStyleColor();

            if (clicked) {
                // Guard: ignore if clicking the already-current device to avoid toggling to "no device"
                const DM::Device& cur_dev = DM::DataCenter::Ins().get_current_device_data();
                const bool        is_new_device = (!cur_dev.valid || cur_dev.mac != item.mac);

                if (is_new_device) {
                    if (SimpleDeviceMgr::instance().switch_current_device_simple(item.mac))
                        canvas.close_device_list_popup();
                }
            }

            ImGui::SetCursorScreenPos(ImVec2(tl.x, br.y + 40.0f * scale));
        }
        ImGui::EndTable();
    }
    ImGui::PopStyleVar();
}

}} // namespace Slic3r::GUI