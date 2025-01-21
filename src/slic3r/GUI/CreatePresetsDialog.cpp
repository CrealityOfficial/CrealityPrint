#include "CreatePresetsDialog.hpp"
#include <vector>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <wx/dcgraph.h>
#include <wx/tooltip.h>
#include <wx/generic/statbmpg.h>
#include <boost/nowide/cstdio.hpp>
#include "libslic3r/PresetBundle.hpp"
#include "I18N.hpp"
#include "GUI_App.hpp"
#include "MsgDialog.hpp"
#include "FileHelp.hpp"
#include "Tab.hpp"
#include "MainFrame.hpp"
#include "libslic3r_version.h"
#include <filesystem>

#define NAME_OPTION_COMBOBOX_SIZE wxSize(FromDIP(200), FromDIP(24))
#define FILAMENT_PRESET_COMBOBOX_SIZE wxSize(FromDIP(300), FromDIP(24))
#define OPTION_SIZE wxSize(FromDIP(100), FromDIP(24))
#define PRINTER_LIST_SIZE wxSize(-1, FromDIP(100))
#define FILAMENT_LIST_SIZE wxSize(FromDIP(560), FromDIP(100))
#define FILAMENT_OPTION_SIZE wxSize(FromDIP(-1), FromDIP(30))
#define PRESET_TEMPLATE_SIZE wxSize(FromDIP(-1), FromDIP(100))
#define PRINTER_SPACE_SIZE wxSize(FromDIP(100), FromDIP(24)) // ORCA Match size with other components
#define ORIGIN_TEXT_SIZE wxSize(FromDIP(10), FromDIP(24))
#define PRINTER_PRESET_VENDOR_SIZE wxSize(FromDIP(150), FromDIP(24))
#define PRINTER_PRESET_MODEL_SIZE wxSize(FromDIP(280), FromDIP(24))
#define STATIC_TEXT_COLOUR wxColour("#363636")
#define PRINTER_LIST_COLOUR wxColour("#EEEEEE")
#define FILAMENT_OPTION_COLOUR wxColour("#D9D9D9")
#define SELECT_ALL_OPTION_COLOUR wxColour("#009688")
#define DEFAULT_PROMPT_TEXT_COLOUR wxColour("#ACACAC")

namespace Slic3r { 
namespace GUI {

static const std::vector<std::string> filament_vendors = 
    {"3Dgenius",               "3DJake",                 "3DXTECH",                "3D BEST-Q",              "3D Hero",
     "3D-Fuel",                "Aceaddity",              "AddNorth",               "Amazon Basics",          "AMOLEN",
     "Ankermake",              "Anycubic",               "Atomic",                 "AzureFilm",              "BASF",
     "Bblife",                 "BCN3D",                  "Beyond Plastic",         "California Filament",    "Capricorn",
     "CC3D",                   "colorFabb",              "Comgrow",                "Cookiecad",              "Creality",
     "CERPRiSE",               "Das Filament",           "DO3D",                   "DOW",                    "DSM",
     "Duramic",                "ELEGOO",                 "Eryone",                 "Essentium",              "eSUN",
     "Extrudr",                "Fiberforce",             "Fiberlogy",              "FilaCube",               "Filamentive",
     "Fillamentum",            "FLASHFORGE",             "Formfutura",             "Francofil",              "FilamentOne",
     "GEEETECH",               "Giantarm",               "Gizmo Dorks",            "GreenGate3D",            "HATCHBOX",
     "Hello3D",                "IC3D",                   "IEMAI",                  "IIID Max",               "INLAND",
     "iProspect",              "iSANMATE",               "Justmaker",              "Keene Village Plastics", "Kexcelled",
     "MakerBot",               "MatterHackers",          "MIKA3D",                 "NinjaTek",               "Nobufil",
     "Novamaker",              "OVERTURE",               "OVVNYXE",                "Polymaker",              "Priline",
     "Printed Solid",          "Protopasta",             "Prusament",              "Push Plastic",           "R3D",
     "Re-pet3D",               "Recreus",                "Regen",                  "Sain SMART",             "SliceWorx",
     "Snapmaker",              "SnoLabs",                "Spectrum",               "SUNLU",                  "TTYT3D",
     "Tianse",                 "UltiMaker",              "Valment",                "Verbatim",               "VO3D",
     "Voxelab",                "VOXELPLA",               "YOOPAI",                 "Yousu",                  "Ziro",
     "Zyltech"};
     
static const std::vector<std::string> filament_types = {"PLA",    "rPLA",  "PLA+",      "PLA Tough", "PETG",  "ABS",    "ASA",    "FLEX",   "HIPS",   "PA",     "PACF",
                                                        "NYLON",  "PVA",   "PVB",       "PC",        "PCABS", "PCTG",   "PCCF",   "PHA",    "PP",     "PEI",    "PET",    "PETG",
                                                        "PETGCF", "PTBA",  "PTBA90A",   "PEEK",  "TPU93A", "TPU75D", "TPU",       "TPU92A", "TPU98A", "Misc",
                                                        "TPE",    "GLAZE", "Nylon",     "CPE",   "METAL",  "ABST",   "Carbon Fiber"};

static const std::vector<std::string> printer_vendors = 
    {"Anker",              "Anycubic",           "Artillery",          "Bambulab",           "BIQU",
     "Comgrow",            "Creality ENDER",      "Creality CR",    "Creality SERMOON",      "Custom Printer",     "Elegoo",             "Flashforge",
     "FLSun",              "FlyingBear",         "Folgertech",         "InfiMech",           "Kingroon",
     "CrealityPrint Arena Printer", "Peopoly",            "Prusa",              "Qidi",               "Raise3D",
     "RatRig",             "SecKit",             "Snapmaker",          "Sovol",              "Tronxy",
     "TwoTrees",           "UltiMaker",          "Vivedino",           "Voron",              "Voxelab",
     "Vzbot",              "Wanhao"};

static const std::unordered_map<std::string, std::vector<std::string>> printer_model_map =
    {{"Anker",          {"Anker M5", "Anker M5 All-Metal Hot End", "Anker M5C"}},
     {"Anycubic",       {"Kossel Linear Plus", "Kossel Pulley(Linear)", "Mega Zero", "i3 Mega", "Predator"}},
     {"Artillery",      {"sidewinder X1",   "Genius", "Hornet"}},
     {"BIBO",           {"BIBO2 Touch"}},
     {"BIQU",           {"BX"}},
     {"Creality ENDER", {"Ender-3",         "Ender-3 BLTouch",  "Ender-3 Pro",      "Ender-3 Neo",      
                        "Ender-3 V2 Neo",   "Ender-3 S1 Plus",  "Ender-3 Max", "Ender-3 Max Neo",
                        "Ender-4",          "Ender-5 Pro",      "Ender-5 Pro",      
                        "Ender-7",          "Ender-2",          "Ender-2 Pro"}},
     {"Creality CR",    {"CR-5 Pro",        "CR-5 Pro H",       "CR-10 SMART", "CR-10 SMART Pro",   "CR-10 Mini",
                        "CR-10",            "CR-10 v3",         "CR-10 S",     "CR-10 v2",          "CR-10 v2",
                        "CR-10 S Pro",      "CR-10 S Pro v2",   "CR-10 S4",         "CR-10 S5",         "CR-20",       "CR-20 Pro",         "CR-200B",
                        "CR-8"}},
     {"Creality SERMOON",{"Sermoon-D1",     "Sermoon-V1",       "Sermoon-V1 Pro"}},
     {"FLSun",          {"FLSun QQs Pro",   "FLSun Q5"}},
     {"gCreate",        {"gMax 1.5XT Plus", "gMax 2",           "gMax 2 Pro",       "gMax 2 Dual 2in1", "gMax 2 Dual Chimera"}},
     {"Geeetech",       {"Thunder",         "Thunder Pro",      "Mizar s",          "Mizar Pro",        "Mizar",        "Mizar Max",
                        "Mizar M",          "A10 Pro",          "A10 M",            "A10 T",            "A20",          "A20 M",
                        "A20T",             "A30 Pro",          "A30 M",            "A30 T",            "E180",         "Me Ducer",
                        "Me creator",       "Me Creator2",      "GiantArmD200",     "l3 ProB",          "l3 Prow",      "l3 ProC"}},
     {"INAT",           {"Proton X Rail",   "Proton x Rod",     "Proton XE-750"}},
     {"Infinity3D",     {"DEV-200",         "DEV-350"}},
     {"Jubilee",        {"Jubilee"}},
     {"LNL3D",          {"D3 v2",           "D3 Vulcan",        "D5",               "D6"}},
     {"LulzBot",        {"Mini Aero",       "Taz6 Aero"}},
     {"MakerGear",      {"Micro",           "M2(V4 Hotend)",    "M2 Dual",          "M3-single Extruder", "M3-Independent Dual Rev.0", "M3-Independent Dual Rev.0(Duplication Mode)",
                        "M3-Independent Dual Rev.1",            "M3-Independent Dual Rev.1(Duplication Mode)", "ultra One", "Ultra One (DuplicationMode)"}},
     {"Original Prusa", {"MK4", "SL1S SPEED", "MMU3"}},
     {"Papapiu",        {"N1s"}},
     {"Print4Taste",    {"mycusini 2.0"}},
     {"RatRig",         {"V-core-3 300mm",  "V-Core-3 400mm",   "V-Core-3 500mm", "V-Minion"}},
     {"Rigid3D",        {"Zero2",           "Zero3"}},
     {"Snapmaker",      {"A250",            "A350"}},
     {"Sovol",          {"SV06",            "SV06 PLUS",        "SV05",             "SV04",             "SV03 / SV03 BLTOUCH",      "SVO2 / SV02 BLTOUCH",      "SVO1 / SV01 BLToUCH", "SV01 PRO"}},
     {"TriLAB",         {"AzteQ Industrial","AzteQ Dynamic",    "DeltiQ 2",         "DeltiQ 2 Plus",    "DeltiQ 2 + FlexPrint 2",   "DeltiQ 2 Plus + FlexPrint 2", "DeltiQ 2 +FlexPrint",
                        "DeltiQ 2 Plus + FlexPrint",            "DeltiQ M",         "DeltiQ L",         "DeltiQ XL"}},
     {"Trimaker",       {"Nebula cloud",    "Nebula",           "Cosmos ll"}},
     {"Ultimaker",      {"Ultimaker 2"}},
     {"Voron",          {"v2 250mm3",        "v2 300mm3",        "v2 350mm3",        "v1 250mm3",        "v1 300mm3", "v1 350mm3",
                        "Zero 120mm3",      "Switchwire"}},
     {"Zonestar",       {"Z5",              "Z6",               "Z5x",              "Z8",               "Z9"}}};

static std::vector<std::string>               nozzle_diameter_vec = {"0.4", "0.15", "0.2", "0.25", "0.3", "0.35", "0.5", "0.6", "0.75", "0.8", "1.0", "1.2"};
static std::unordered_map<std::string, float> nozzle_diameter_map = {{"0.15", 0.15}, {"0.2", 0.2},   {"0.25", 0.25}, {"0.3", 0.3},
                                                                     {"0.35", 0.35}, {"0.4", 0.4},   {"0.5", 0.5},   {"0.6", 0.6},
                                                                     {"0.75", 0.75}, {"0.8", 0.8},   {"1.0", 1.0},   {"1.2", 1.2}};

static std::set<int> cannot_input_key = {9, 10, 13, 33, 35, 36, 37, 38, 40, 41, 42, 44, 46, 47, 59, 60, 62, 63, 64, 92, 94, 95, 124, 126};

static std::set<char> special_key = {'\n', '\t', '\r', '\v', '@', ';'};

static std::string remove_special_key(const std::string &str)
{
    std::string res_str;
    for (char c : str) {
        if (special_key.find(c) == special_key.end()) {
            res_str.push_back(c);
        }
    }
    return res_str;
}

static bool str_is_all_digit(const std::string &str) {
    for (const char &c : str) {
        if (!std::isdigit(c)) return false;
    }
    return true; 
}

static bool delete_filament_preset_by_name(std::string delete_preset_name, std::string &selected_preset_name)
{
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format("select preset, name %1%") % delete_preset_name;
    if (delete_preset_name.empty()) return false;

    // Find an alternate preset to be selected after the current preset is deleted.
    PresetCollection &m_presets = wxGetApp().preset_bundle->filaments;
    if (delete_preset_name == selected_preset_name) {
        const std::deque<Preset> &presets     = m_presets.get_presets();
        size_t                    idx_current = m_presets.get_idx_selected();

        // Find the visible preset.
        size_t idx_new = idx_current;
        if (idx_current > presets.size()) idx_current = presets.size();
        if (idx_current < 0) idx_current = 0;
        if (idx_new < presets.size())
            for (; idx_new < presets.size() && (presets[idx_new].name == delete_preset_name || !presets[idx_new].is_visible); ++idx_new)
                ;
        if (idx_new == presets.size())
            for (idx_new = idx_current - 1; idx_new > 0 && (presets[idx_new].name == delete_preset_name || !presets[idx_new].is_visible); --idx_new)
                ;
        selected_preset_name = presets[idx_new].name;
        BOOST_LOG_TRIVIAL(info) << boost::format("cause by delete current ,choose the next visible, idx %1%, name %2%") % idx_new % selected_preset_name;
    }

    try {
        // BBS delete preset
        Preset *need_delete_preset = m_presets.find_preset(delete_preset_name);
        if (!need_delete_preset) BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(" can't find delete preset and name: %1%") % delete_preset_name;
        if (!need_delete_preset->setting_id.empty()) {
            BOOST_LOG_TRIVIAL(info) << "delete preset = " << need_delete_preset->name << ", setting_id = " << need_delete_preset->setting_id;
            m_presets.set_sync_info_and_save(need_delete_preset->name, need_delete_preset->setting_id, "delete", 0);
            wxGetApp().delete_preset_from_cloud(need_delete_preset->setting_id);
        } else {
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(" can't preset setting id is empty and name: %1%") % delete_preset_name;
        }
        if (m_presets.get_edited_preset().name == delete_preset_name) {
            m_presets.discard_current_changes();
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format("delete preset dirty and cancelled");
        }
        m_presets.delete_preset(need_delete_preset->name);
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " preset has been delete from filaments, and preset name is: " << delete_preset_name;
    } catch (const std::exception &ex) {
        // FIXME add some error reporting!
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format("found exception when delete: %1% and preset name: %%") % ex.what() % delete_preset_name;
        return false;
    }

    return true;
}

static std::string get_curr_time(const char* format = "%Y_%m_%d_%H_%M_%S")
{
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

    std::time_t time = std::chrono::system_clock::to_time_t(now);

    std::tm            local_time = *std::localtime(&time);
    std::ostringstream time_stream;
    time_stream << std::put_time(&local_time, format);

    std::string current_time = time_stream.str();
    return current_time;
}

static std::string get_curr_timestmp()
{
    return get_curr_time("%Y%m%d%H%M%S");
    // std::time_t currentTime = std::time(nullptr);
    // std::ostringstream oss;
    // oss << currentTime;
    // std::string timestampString = oss.str();
    // return timestampString;
}

static void get_filament_compatible_printer(Preset* preset, vector<std::string>& printers)
{ 
    auto compatible_printers = dynamic_cast<ConfigOptionStrings *>(preset->config.option("compatible_printers"));
    if (compatible_printers == nullptr) return;
    for (const std::string &printer_name : compatible_printers->values) {
        printers.push_back(printer_name);
    }
}

static wxBoxSizer* create_checkbox(wxWindow* parent, Preset* preset, wxString& preset_name, std::vector<std::pair<::CheckBox*, Preset*>>& preset_checkbox)
{
    wxBoxSizer *sizer    = new wxBoxSizer(wxHORIZONTAL);
    ::CheckBox *  checkbox = new ::CheckBox(parent);
    sizer->Add(checkbox, 0, 0, 0);
    preset_checkbox.push_back(std::make_pair(checkbox, preset));
    wxStaticText *preset_name_str = new wxStaticText(parent, wxID_ANY, preset_name);
    wxToolTip *   toolTip         = new wxToolTip(preset_name);
    preset_name_str->SetToolTip(toolTip);
    sizer->Add(preset_name_str, 0, wxLEFT, 5);
    return sizer;
}

static wxBoxSizer *create_checkbox(wxWindow *parent, std::string &compatible_printer, Preset* preset, std::unordered_map<::CheckBox *, std::pair<std::string, Preset *>> &ptinter_compatible_filament_preset)
{
    wxBoxSizer *sizer    = new wxBoxSizer(wxHORIZONTAL);
    ::CheckBox *checkbox = new ::CheckBox(parent);
    sizer->Add(checkbox, 0, 0, 0);
    ptinter_compatible_filament_preset[checkbox] = std::make_pair(compatible_printer, preset);
    wxStaticText *preset_name_str = new wxStaticText(parent, wxID_ANY, wxString::FromUTF8(compatible_printer));
    sizer->Add(preset_name_str, 0, wxLEFT, 5);
    return sizer;
}

static wxBoxSizer *create_checkbox(wxWindow *parent, wxString &preset_name, std::vector<std::pair<::CheckBox *, std::string>> &preset_checkbox)
{
    wxBoxSizer *sizer    = new wxBoxSizer(wxHORIZONTAL);
    ::CheckBox *checkbox = new ::CheckBox(parent);
    sizer->Add(checkbox, 0, 0, 0);
    preset_checkbox.push_back(std::make_pair(checkbox, into_u8(preset_name)));
    wxStaticText *preset_name_str = new wxStaticText(parent, wxID_ANY, preset_name);
    sizer->Add(preset_name_str, 0, wxLEFT, 5);
    return sizer;
}

static wxArrayString get_exist_vendor_choices(VendorMap& vendors)
{
    wxArrayString choices;
    PresetBundle  temp_preset_bundle;
    temp_preset_bundle.load_system_models_from_json(ForwardCompatibilitySubstitutionRule::EnableSystemSilent);
    PresetBundle *preset_bundle = wxGetApp().preset_bundle;

    VendorProfile users_models  = preset_bundle->get_custom_vendor_models();

    vendors = temp_preset_bundle.vendors;

    if (!users_models.models.empty()) {
        vendors[users_models.name] = users_models;
    }

    for (const pair<std::string, VendorProfile> &vendor : vendors) {
        if (vendor.second.models.empty() || vendor.second.id.empty()) continue;
        choices.Add(vendor.first);
    }
    return choices;
}

static std::string get_machine_name(const std::string &preset_name)
{
    size_t index_at = preset_name.find_last_of("@");
    if (std::string::npos == index_at) {
        return "";
    } else {
        return preset_name.substr(index_at + 1);
    }
}

static std::string get_filament_name(std::string &preset_name)
{
    size_t index_at = preset_name.find_last_of("@");
    if (std::string::npos == index_at) {
        return preset_name;
    } else {
        return preset_name.substr(0, index_at - 1);
    }
}

static wxBoxSizer *create_preset_tree(wxWindow *parent, std::pair<std::string, std::vector<std::shared_ptr<Preset>>> printer_and_preset)
{
    wxTreeCtrl *treeCtrl = new wxTreeCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE | wxNO_BORDER);
    wxColour    backgroundColor = parent->GetBackgroundColour();
    treeCtrl->SetBackgroundColour(backgroundColor);

    wxString     printer_name = wxString::FromUTF8(printer_and_preset.first);
    wxTreeItemId rootId       = treeCtrl->AddRoot(printer_name);
    int          row          = 1;
    for (std::shared_ptr<Preset> preset : printer_and_preset.second) {
        wxString     preset_name = wxString::FromUTF8(preset->name);
        wxTreeItemId childId1    = treeCtrl->AppendItem(rootId, preset_name);
        row++;
    }

    treeCtrl->Expand(rootId);
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    treeCtrl->SetMinSize(wxSize(-1, row * 22));
    treeCtrl->SetMaxSize(wxSize(-1, row * 22));
    sizer->Add(treeCtrl, 0, wxEXPAND | wxALL, 0);

    return sizer;
}

static std::string get_vendor_name(std::string& preset_name)
{
    if (preset_name.empty()) return "";
    std::string vendor_name = preset_name.substr(preset_name.find_first_not_of(' ')); //remove the name prefix space
    size_t index_at = vendor_name.find(" ");
    if (std::string::npos == index_at) {
        return vendor_name;
    } else {
        vendor_name = vendor_name.substr(0, index_at);
        return vendor_name;
    }
}

static std::string get_machine_model(const std::string &preset_model)
{
    std::string modelName = preset_model;
    if (modelName.empty())
        return modelName;

    size_t index_at = modelName.find(" nozzle");
    if (index_at != std::string::npos) {
        modelName = modelName.substr(0, index_at); 
    }

    size_t index_space = modelName.find_last_of(" ");
    if (index_space != std::string::npos) {
        modelName.replace(index_space, 1, "-"); 
    }

    return modelName; 
}


 static wxBoxSizer *create_select_filament_preset_checkbox(wxWindow *                                    parent,
                                                           std::string &                                 compatible_printer,
                                                           std::vector<Preset *>                         presets,
                                                           std::unordered_map<::CheckBox *, std::pair<std::string, Preset *>> &machine_filament_preset)
 {

     //获取继承此打印机的所有机型
     std::vector<std::string> vecPrinterInherits;
     vecPrinterInherits.push_back(compatible_printer);
     PresetCollection* presetColl = &wxGetApp().preset_bundle->printers;
     if (presetColl)
     {
         const std::deque<Preset> presetsDeque = presetColl->get_presets();
         for (int i = 0; i < presetsDeque.size(); i++)
         {
            Preset presetTmp = presetsDeque[i];
            string strTmp = presetTmp.inherits();
            if(compatible_printer == strTmp)
            {
                vecPrinterInherits.push_back(presetTmp.name);
            }
         }
     }

     wxBoxSizer*   vertical_sizer = new wxBoxSizer(wxVERTICAL);
     vertical_sizer->SetMinSize(wxSize(912, -1));

     wxBoxSizer*   horizontal_sizer = new wxBoxSizer(wxHORIZONTAL);
     wxBoxSizer *checkbox_sizer   = new wxBoxSizer(wxHORIZONTAL);
     //wxStaticText *machine_name_str = new wxStaticText(parent, wxID_ANY, wxString::FromUTF8(compatible_printer));
     std::string sModel = get_machine_model(compatible_printer);
     wxStaticText *machine_name_str = new wxStaticText(parent, wxID_ANY, wxString::FromUTF8(sModel));
     ComboBox *    combobox        = new ComboBox(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(200, 20), 0, nullptr, wxCB_READONLY);
     ::CheckBox* checkbox = new ::CheckBox(parent);
     checkbox->Bind(wxEVT_TOGGLEBUTTON, [checkbox, combobox, presets, &machine_filament_preset, compatible_printer](wxCommandEvent& event){
         bool value = checkbox->GetValue();
         if (value)
         {
             checkbox->SetValue(true);
             wxString preset_name = combobox->GetStringSelection();
             for (Preset* preset : presets)
             {
                 if (preset_name == wxString::FromUTF8(preset->name))
                 {
                     machine_filament_preset[checkbox] = std::make_pair(compatible_printer, preset);
                 }
             }
         }
         else
         {
             checkbox->SetValue(false);
         }
         });
     checkbox_sizer->Add(checkbox, 0, wxEXPAND | wxRIGHT, 5);
     checkbox_sizer->Add(machine_name_str, 0, wxEXPAND | wxRIGHT, 0);

     wxBoxSizer *combobox_sizer = new wxBoxSizer(wxHORIZONTAL);
     wxStaticText *filament_name_str = new wxStaticText(parent, wxID_ANY, _L("Reference material template"));
     combobox->SetBackgroundColor(*wxWHITE);
     combobox->SetBorderColor(PRINTER_LIST_COLOUR);
     combobox->SetLabel(_L("Select filament preset"));
     combobox->Bind(wxEVT_COMBOBOX, [combobox, checkbox, presets, &machine_filament_preset, compatible_printer](wxCommandEvent& e) {
         combobox->SetLabelColor(*wxBLACK);
         wxString preset_name = combobox->GetStringSelection();
         checkbox->SetValue(true);
         for (Preset *preset : presets) {
             if (preset_name == wxString::FromUTF8(preset->name)) {
                 machine_filament_preset[checkbox] = std::make_pair(compatible_printer, preset);
             }
         }
         e.Skip();
     });
     combobox_sizer->Add(filament_name_str, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxLEFT, 20);
     combobox_sizer->Add(combobox, 0, wxEXPAND | wxLEFT, 5);

     wxArrayString choices;

     //将用户预设置前
     for (Preset* preset : presets)
     {
         if (preset->is_user())
         {
             choices.Add(wxString::FromUTF8(preset->name));
         }
     }
     for (Preset* preset : presets)
     {
         if (!preset->is_user())
         {
             choices.Add(wxString::FromUTF8(preset->name));
         }
     }
     combobox->Set(choices);
     combobox->SetSelection(0);

     //wxPanel* panel = new wxPanel(parent, wxID_ANY,wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL|wxBORDER_SIMPLE);
     wxPanel* panel = new wxPanel(parent, wxID_ANY,wxDefaultPosition, wxDefaultSize);
     //panel->SetMinSize(wxSize(912, -1));
     panel->SetBackgroundColour(wxColour("#474749"));
     int iNum = vecPrinterInherits.size();
     if (iNum)
     {
         wxBoxSizer* textBox = new wxBoxSizer(wxVERTICAL);
         textBox->SetMinSize(wxSize(912, 30));
         for (int i = 0; i < vecPrinterInherits.size(); ++i)
         {
             wxStaticText* itemText = new wxStaticText(panel, wxID_ANY, wxString::FromUTF8(vecPrinterInherits[i]));
             textBox->Add(itemText, 0, wxEXPAND | wxALL, 5);
         }
         panel->SetSizer(textBox);
     }

     horizontal_sizer->Add(checkbox_sizer);
     horizontal_sizer->AddStretchSpacer();
     horizontal_sizer->Add(combobox_sizer);

     vertical_sizer->Add(horizontal_sizer, 0, wxEXPAND | wxTop, 20);
     vertical_sizer->Add(panel,0,wxEXPAND,0);

     return vertical_sizer;
 }

static wxString get_curr_radio_type(std::vector<std::pair<RadioBox *, wxString>> &radio_btns)
{
    for (std::pair<RadioBox *, wxString> radio_string : radio_btns) {
        if (radio_string.first->GetValue()) {
            return radio_string.second;
        }
    }
    return "";
}

static std::string calculate_md5(const std::string &input)
{
    unsigned char digest[MD5_DIGEST_LENGTH];
    std::string   md5;

    EVP_MD_CTX *mdContext = EVP_MD_CTX_new();
    EVP_DigestInit(mdContext, EVP_md5());
    EVP_DigestUpdate(mdContext, input.c_str(), input.length());
    EVP_DigestFinal(mdContext, digest, nullptr);
    EVP_MD_CTX_free(mdContext);

    char hexDigest[MD5_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) { sprintf(hexDigest + (i * 2), "%02x", digest[i]); }
    hexDigest[MD5_DIGEST_LENGTH * 2] = '\0';

    md5 = std::string(hexDigest);
    return md5;
}

static std::string get_filament_id(std::string vendor_typr_serial)
{
    std::unordered_map<std::string, std::set<std::string>> filament_id_to_filament_name;

    // temp filament presets

    static bool is_load_system_filaments = false;
    static PresetBundle temp_system_preset_bundle;
    if (!is_load_system_filaments)
    {
        temp_system_preset_bundle.load_system_filaments_json(Slic3r::ForwardCompatibilitySubstitutionRule::EnableSilent);
        is_load_system_filaments = true;
    }
    const std::deque<Preset> &system_filament_presets = temp_system_preset_bundle.filaments.get_presets();

    for (const Preset &preset : system_filament_presets) {
        std::string preset_name = preset.name;
        size_t      index_at    = preset_name.find_first_of('@');
        if (index_at == std::string::npos) {
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " filament preset name has no @ and name is: " << preset_name;
            continue;
        }
        std::string filament_name = preset_name.substr(0, index_at - 1);
        if (filament_name == vendor_typr_serial && preset.filament_id != "null")
            return preset.filament_id;
        filament_id_to_filament_name[preset.filament_id].insert(filament_name);
    }


    PresetBundle temp_preset_bundle;
    //temp_preset_bundle.load_system_filaments_json(Slic3r::ForwardCompatibilitySubstitutionRule::EnableSilent);

    std::string dir_user_presets = wxGetApp().app_config->get("preset_folder");
    if (dir_user_presets.empty()) {
        temp_preset_bundle.load_user_presets(DEFAULT_USER_FOLDER_NAME, ForwardCompatibilitySubstitutionRule::EnableSilent);
    } else {
        temp_preset_bundle.load_user_presets(dir_user_presets, ForwardCompatibilitySubstitutionRule::EnableSilent);
    }
    const std::deque<Preset> &filament_presets = temp_preset_bundle.filaments.get_presets();

    for (const Preset &preset : filament_presets) {
        std::string preset_name = preset.name;
        size_t      index_at    = preset_name.find_first_of('@');
        if (index_at == std::string::npos) {
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " filament preset name has no @ and name is: " << preset_name;
            continue;
        }
        std::string filament_name = preset_name.substr(0, index_at - 1);
        if (filament_name == vendor_typr_serial && preset.filament_id != "null")
            return preset.filament_id;
        filament_id_to_filament_name[preset.filament_id].insert(filament_name);
    }
    // global filament presets
    PresetBundle *                                     preset_bundle               = wxGetApp().preset_bundle;
    std::map<std::string, std::vector<Preset const *>> temp_filament_id_to_presets = preset_bundle->filaments.get_filament_presets();
    for (std::pair<std::string, std::vector<Preset const *>> filament_id_to_presets : temp_filament_id_to_presets) {
        if (filament_id_to_presets.first.empty()) continue;
        for (const Preset *preset : filament_id_to_presets.second) {
            std::string preset_name = preset->name;
            size_t      index_at    = preset_name.find_first_of('@');
            if (index_at == std::string::npos) {
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " filament preset name has no @ and name is: " << preset_name;
                continue;
            }
            std::string filament_name = preset_name.substr(0, index_at - 1);
            if (filament_name == vendor_typr_serial && preset->filament_id != "null")
                return preset->filament_id;
            filament_id_to_filament_name[preset->filament_id].insert(filament_name);
        }
    }

    std::string user_filament_id = "P" + calculate_md5(vendor_typr_serial).substr(0, 7);

    while (filament_id_to_filament_name.find(user_filament_id) != filament_id_to_filament_name.end()) {//find same filament id
        bool have_same_filament_name = false;
        for (const std::string &name : filament_id_to_filament_name.find(user_filament_id)->second) {
            if (name == vendor_typr_serial) {
                have_same_filament_name = true;
                break;
            }
        }
        if (have_same_filament_name) {
            break;
        }
        else { //Different names correspond to the same filament id
            user_filament_id = "P" + calculate_md5(vendor_typr_serial + get_curr_time()).substr(0, 7);
        }
    }
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " filament name is: " << vendor_typr_serial << "and create filament_id is: " << user_filament_id;
    return user_filament_id;
}

static json get_config_json(const Preset* preset) {
    json j;
    // record the headers
    j[BBL_JSON_KEY_VERSION] = preset->version.to_string();
    j[BBL_JSON_KEY_NAME]    = preset->name;
    j[BBL_JSON_KEY_FROM]    = "";

   DynamicPrintConfig config = preset->config;

    // record all the key-values
    for (const std::string &opt_key : config.keys()) {
        const ConfigOption *opt = config.option(opt_key);
        if (opt->is_scalar()) {
            if (opt->type() == coString)
                // keep \n, \r, \t
                j[opt_key] = (dynamic_cast<const ConfigOptionString *>(opt))->value;
            else
                j[opt_key] = opt->serialize();
        } else {
            const ConfigOptionVectorBase *vec = static_cast<const ConfigOptionVectorBase *>(opt);
            std::vector<std::string> string_values = vec->vserialize();

            json j_array(string_values);
            j[opt_key] = j_array;
        }
    }

    return j;
}

static char* read_json_file(const std::string &preset_path)
{
    FILE *json_file = boost::nowide::fopen(boost::filesystem::path(preset_path).make_preferred().string().c_str(), "rb");
    if (json_file == NULL) {
        BOOST_LOG_TRIVIAL(info) << "Failed to open JSON file: " << preset_path;
        return NULL;
    }
    fseek(json_file, 0, SEEK_END);     // seek to end
    long file_size = ftell(json_file); // get file size
    fseek(json_file, 0, SEEK_SET);     // seek to start

    char * json_contents = (char *) malloc(file_size);
    if (json_contents == NULL) {
        BOOST_LOG_TRIVIAL(info) << "Failed to allocate memory for JSON file ";
        fclose(json_file);
        return NULL;
    }

    fread(json_contents, 1, file_size, json_file);
    fclose(json_file);

    return json_contents;
}

static std::string get_printer_nozzle_diameter(std::string printer_name) {

    size_t index = printer_name.find(" nozzle");
    if (std::string::npos == index) {
        return "";
    }
    std::string nozzle           = printer_name.substr(0, index);
    size_t      last_space_index = nozzle.find_last_of(" ");
    if (std::string::npos == index) {
        return "";
    }
    return nozzle.substr(last_space_index + 1);
}

static void adjust_dialog_in_screen(DPIDialog* dialog) {
    wxSize screen_size = wxGetDisplaySize();
    int    pos_x, pos_y, size_x, size_y, screen_width, screen_height, dialog_x, dialog_y;
    pos_x         = dialog->GetPosition().x;
    pos_y         = dialog->GetPosition().y;
    size_x        = dialog->GetSize().x;
    size_y        = dialog->GetSize().y;
    screen_width  = screen_size.GetWidth();
    screen_height = screen_size.GetHeight();
    dialog_x      = pos_x;
    dialog_y      = pos_y;
    if (pos_x + size_x > screen_width) {
        int exceed_x = pos_x + size_x - screen_width;
        dialog_x -= exceed_x;
    }
    if (pos_y + size_y > screen_height - 50) {
        int exceed_y = pos_y + size_y - screen_height + 50;
        dialog_y -= exceed_y;
    }
    if (pos_x != dialog_x || pos_y != dialog_y) { dialog->SetPosition(wxPoint(dialog_x, dialog_y)); }
}

CreateFilamentPresetDialog::CreateFilamentPresetDialog(wxWindow *parent) 
	: DPIDialog(parent ? parent : nullptr, wxID_ANY, _L("Create Filament"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX | wxCENTRE)
{
    m_create_type.base_filament = _L("Create Based on Current Filament");
    m_create_type.base_filament_preset = _L("Copy Current Filament Preset ");
    get_all_filament_presets();

	this->SetBackgroundColour(*wxWHITE);
    this->SetSize(wxSize(FromDIP(600), FromDIP(480)));

    std::string icon_path = (boost::format("%1%/images/Creative3DTitle.ico") % resources_dir()).str();
    SetIcon(wxIcon(encode_path(icon_path.c_str()), wxBITMAP_TYPE_ICO));

	wxBoxSizer *m_main_sizer = new wxBoxSizer(wxVERTICAL);
    // top line
    auto m_line_top = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 1), wxTAB_TRAVERSAL);
    m_line_top->SetBackgroundColour(wxColour(0xA6, 0xa9, 0xAA));
    m_main_sizer->Add(m_line_top, 0, wxEXPAND, 0);
    m_main_sizer->Add(0, 0, 0, wxTOP, FromDIP(5));

    wxStaticText *basic_infomation = new wxStaticText(this, wxID_ANY, _L("Basic Information"));
    basic_infomation->SetFont(Label::Head_16);
    m_main_sizer->Add(basic_infomation, 0, wxLEFT, FromDIP(10));

    m_main_sizer->Add(create_item(FilamentOptionType::VENDOR), 0, wxEXPAND | wxALL, FromDIP(5));
    m_main_sizer->Add(create_item(FilamentOptionType::TYPE), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));
    m_main_sizer->Add(create_item(FilamentOptionType::SERIAL), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));

    // divider line
    auto line_divider = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 1), wxTAB_TRAVERSAL);
    line_divider->SetBackgroundColour(wxColour(0xA6, 0xa9, 0xAA));
    m_main_sizer->Add(line_divider, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(10));
    m_main_sizer->Add(0, 0, 0, wxTOP, FromDIP(5));

    wxStaticText *presets_infomation = new wxStaticText(this, wxID_ANY, _L("You can create the material for these added models simultaneously"));
    presets_infomation->SetFont(Label::Head_16);
    m_main_sizer->Add(presets_infomation, 0, wxLEFT | wxRIGHT, FromDIP(15));

    {
        wxPanel* panel = new wxPanel(this);
        wxBoxSizer* boxSizer = new wxBoxSizer(wxVERTICAL);
        boxSizer->Add(create_item(FilamentOptionType::FILAMENT_PRESET,panel), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));
        panel->SetSizer(boxSizer);
        panel->Hide();

        //m_main_sizer->Add(create_item(FilamentOptionType::FILAMENT_PRESET), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));
        //m_filament_preset_text = new wxStaticText(this, wxID_ANY, _L("We could create the filament presets for your following printer:"), wxDefaultPosition, wxDefaultSize);
        //m_main_sizer->Add(m_filament_preset_text, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(15));
    }

    m_scrolled_preset_panel = new wxScrolledWindow(this, wxID_ANY);
    m_scrolled_preset_panel->SetMaxSize(wxSize(-1, FromDIP(350)));
    m_scrolled_preset_panel->SetBackgroundColour(*wxWHITE);
    m_scrolled_preset_panel->SetScrollRate(5, 5);
    
    m_scrolled_sizer = new wxBoxSizer(wxVERTICAL);
    m_scrolled_sizer->Add(create_item(FilamentOptionType::PRESET_FOR_PRINTER), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));
    m_scrolled_sizer->Add(0, 0, 0, wxTOP, FromDIP(5));
    m_scrolled_preset_panel->SetSizerAndFit(m_scrolled_sizer);
    m_main_sizer->Add(m_scrolled_preset_panel, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(10));
    m_main_sizer->Add(create_button_item(), 0, wxEXPAND | wxALL, FromDIP(10));

    get_all_visible_printer_name();
    select_curr_radiobox(m_create_type_btns, 1);

    this->SetSizer(m_main_sizer);

    Layout();
    Fit();

	wxGetApp().UpdateDlgDarkUI(this);
}

CreateFilamentPresetDialog::~CreateFilamentPresetDialog()
{
    for (std::pair<std::string, Preset *> preset : m_all_presets_map) {
        Preset *p = preset.second;
        if (p) {
            delete p;
            p = nullptr;
        }
    }
}

void CreateFilamentPresetDialog::on_dpi_changed(const wxRect &suggested_rect) {
    
    m_button_create->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_create->SetMaxSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_create->SetCornerRadius(FromDIP(12));
    m_button_cancel->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_cancel->SetMaxSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_cancel->SetCornerRadius(FromDIP(12));

    Layout();
}

bool CreateFilamentPresetDialog::is_check_box_selected()
{
    for (const std::pair<::CheckBox *, std::pair<std::string, Preset *>> &checkbox_preset : m_filament_preset) {
        if (checkbox_preset.first->GetValue()) { return true; }
    }

    for (const std::pair<::CheckBox *, std::pair<std::string, Preset *>> &checkbox_preset : m_machint_filament_preset) {
        if (checkbox_preset.first->GetValue()) { return true; }
    }

    return false;
}

wxBoxSizer *CreateFilamentPresetDialog::create_item(FilamentOptionType option_type,wxWindow *parent)
{

    wxSizer *item = nullptr;
    switch (option_type) {
        case VENDOR:             return create_vendor_item();
        case TYPE:               return create_type_item();
        case SERIAL:             return create_serial_item();
        case FILAMENT_PRESET:    return create_filament_preset_item(parent);
        case PRESET_FOR_PRINTER: return create_filament_preset_for_printer_item();
        default:                 return nullptr;
    }
}
wxBoxSizer *CreateFilamentPresetDialog::create_vendor_item()
{ 
    wxBoxSizer *horizontal_sizer = new wxBoxSizer(wxHORIZONTAL); 

    wxBoxSizer *  optionSizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText *static_vendor_text = new wxStaticText(this, wxID_ANY, _L("Vendor"), wxDefaultPosition, wxDefaultSize);
    optionSizer->Add(static_vendor_text, 0, wxEXPAND | wxALL, 0);
    optionSizer->SetMinSize(OPTION_SIZE);
    horizontal_sizer->Add(optionSizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(5)); 

    wxArrayString choices;
    for (const wxString &vendor : filament_vendors) {
        choices.push_back(vendor);
    }
    choices.Sort();

    wxBoxSizer *vendor_sizer   = new wxBoxSizer(wxHORIZONTAL);
    m_filament_vendor_combobox = new ComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, NAME_OPTION_COMBOBOX_SIZE, 0, nullptr, wxCB_READONLY);
    m_filament_vendor_combobox->SetLabel(_L("Select Vendor"));
    m_filament_vendor_combobox->SetLabelColor(DEFAULT_PROMPT_TEXT_COLOUR);
    m_filament_vendor_combobox->Set(choices);
    m_filament_vendor_combobox->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent &e) {
        m_filament_vendor_combobox->SetLabelColor(*wxBLACK);
        e.Skip();
    });
    vendor_sizer->Add(m_filament_vendor_combobox, 0, wxEXPAND | wxALL, 0);
    wxBoxSizer *textInputSizer = new wxBoxSizer(wxVERTICAL);
    m_filament_custom_vendor_input = new TextInput(this, wxEmptyString, wxEmptyString, wxEmptyString, wxDefaultPosition, NAME_OPTION_COMBOBOX_SIZE, wxTE_PROCESS_ENTER);
    m_filament_custom_vendor_input->GetTextCtrl()->SetMaxLength(50);
    m_filament_custom_vendor_input->SetSize(NAME_OPTION_COMBOBOX_SIZE);
    textInputSizer->Add(m_filament_custom_vendor_input, 0, wxEXPAND | wxALL, 0);
    m_filament_custom_vendor_input->GetTextCtrl()->SetHint(_L("Input Custom Vendor"));
    m_filament_custom_vendor_input->GetTextCtrl()->Bind(wxEVT_CHAR, [this](wxKeyEvent &event) {
        int key = event.GetKeyCode();
        if (cannot_input_key.find(key) != cannot_input_key.end()) {
            event.Skip(false);
            return;
        }
        event.Skip();
    });
    m_filament_custom_vendor_input->Hide();
    vendor_sizer->Add(textInputSizer, 0, wxEXPAND | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    //wxBoxSizer *comboBoxSizer      = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *comboBoxSizer      = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *checkbox_sizer     = new wxBoxSizer(wxHORIZONTAL);
    m_can_not_find_vendor_checkbox = new ::CheckBox(this);

    checkbox_sizer->Add(m_can_not_find_vendor_checkbox, 0, wxALIGN_CENTER, 0);
    checkbox_sizer->Add(0, 0, 0, wxEXPAND |wxRIGHT, FromDIP(5));

    wxStaticText *m_can_not_find_vendor_text = new wxStaticText(this, wxID_ANY, _L("Can't find vendor I want"), wxDefaultPosition, wxDefaultSize, 0);
    m_can_not_find_vendor_text->SetFont(::Label::Body_13);

    wxSize size = m_can_not_find_vendor_text->GetTextExtent(_L("Can't find vendor I want"));
    m_can_not_find_vendor_text->SetMinSize(wxSize(size.x + FromDIP(4), -1));
    m_can_not_find_vendor_text->Wrap(-1);
    checkbox_sizer->Add(m_can_not_find_vendor_text, 0, wxALIGN_CENTER, 0);

    m_can_not_find_vendor_checkbox->Bind(wxEVT_TOGGLEBUTTON, [this](wxCommandEvent &e) {
        bool value = m_can_not_find_vendor_checkbox->GetValue();
        if (value) {
            m_can_not_find_vendor_checkbox->SetValue(true);
            m_filament_vendor_combobox->Hide();
            m_filament_custom_vendor_input->Show();
        } else {
            m_can_not_find_vendor_checkbox->SetValue(false);
            m_filament_vendor_combobox->Show();
            m_filament_custom_vendor_input->Hide();
        }
        Refresh();
        Layout();
        Fit();
    });

    comboBoxSizer->Add(vendor_sizer, 0, wxEXPAND | wxTOP, FromDIP(5));
    comboBoxSizer->AddSpacer(FromDIP(20));
    comboBoxSizer->Add(checkbox_sizer, 0, wxEXPAND | wxTOP, FromDIP(5));
    horizontal_sizer->Add(comboBoxSizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(5));

    return horizontal_sizer;

}

wxBoxSizer *CreateFilamentPresetDialog::create_type_item()
{
    wxBoxSizer *horizontal_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxBoxSizer *  optionSizer        = new wxBoxSizer(wxVERTICAL);
    wxStaticText *static_type_text = new wxStaticText(this, wxID_ANY, _L("Type"), wxDefaultPosition, wxDefaultSize);
    optionSizer->Add(static_type_text, 0, wxEXPAND | wxALL, 0);
    optionSizer->SetMinSize(OPTION_SIZE);
    horizontal_sizer->Add(optionSizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(5));

    wxArrayString filament_type;
    for (const wxString &filament : m_system_filament_types_set) {
        filament_type.Add(filament);
    }
    filament_type.Sort();

    wxBoxSizer *comboBoxSizer = new wxBoxSizer(wxVERTICAL);
    m_filament_type_combobox  = new ComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, NAME_OPTION_COMBOBOX_SIZE, 0, nullptr, wxCB_READONLY);
    m_filament_type_combobox->SetLabel(_L("Select Type"));
    m_filament_type_combobox->SetLabelColor(DEFAULT_PROMPT_TEXT_COLOUR);
    m_filament_type_combobox->Set(filament_type);
    comboBoxSizer->Add(m_filament_type_combobox, 0, wxEXPAND | wxALL, 0);
    horizontal_sizer->Add(comboBoxSizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(5));

    m_filament_type_combobox->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent &e) { 
        m_filament_type_combobox->SetLabelColor(*wxBLACK);
        const wxString &curr_create_type = curr_create_filament_type();
        clear_filament_preset_map();
        if (curr_create_type == m_create_type.base_filament) {
            wxArrayString filament_preset_choice = get_filament_preset_choices();
            m_filament_preset_combobox->Set(filament_preset_choice);
            m_filament_preset_combobox->SetLabel(_L("Select Filament Preset"));
            m_filament_preset_combobox->SetLabelColor(DEFAULT_PROMPT_TEXT_COLOUR);
            
        } else if (curr_create_type == m_create_type.base_filament_preset) {
            get_filament_presets_by_machine();
        }
        m_scrolled_preset_panel->SetSizerAndFit(m_scrolled_sizer);

        update_dialog_size();
        e.Skip();
    });

    return horizontal_sizer;
}

wxBoxSizer *CreateFilamentPresetDialog::create_serial_item()
{
    wxBoxSizer *horizontal_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxBoxSizer *  optionSizer        = new wxBoxSizer(wxVERTICAL);
    wxStaticText *static_serial_text = new wxStaticText(this, wxID_ANY, _L("Serial"), wxDefaultPosition, wxDefaultSize);
    optionSizer->Add(static_serial_text, 0, wxEXPAND | wxALL, 0);
    optionSizer->SetMinSize(OPTION_SIZE);
    horizontal_sizer->Add(optionSizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(5));

    wxBoxSizer *comboBoxSizer = new wxBoxSizer(wxVERTICAL);
    m_filament_serial_input   = new TextInput(this, "", "", "", wxDefaultPosition, NAME_OPTION_COMBOBOX_SIZE, wxTE_PROCESS_ENTER);
    m_filament_serial_input->GetTextCtrl()->SetMaxLength(50);
    comboBoxSizer->Add(m_filament_serial_input, 0, wxEXPAND | wxALL, 0);
    m_filament_serial_input->GetTextCtrl()->Bind(wxEVT_CHAR, [this](wxKeyEvent &event) {
        int key = event.GetKeyCode();
        if (cannot_input_key.find(key) != cannot_input_key.end()) {
            event.Skip(false);
            return;
        }
        event.Skip();
        });

    wxStaticText *static_eg_text = new wxStaticText(this, wxID_ANY, _L("e.g. Basic, Matte, Silk, Marble"), wxDefaultPosition, wxDefaultSize);
    static_eg_text->SetForegroundColour(wxColour("#6B6B6B"));
    static_eg_text->SetFont(::Label::Body_12);
    comboBoxSizer->Add(static_eg_text, 0, wxEXPAND | wxTOP, FromDIP(5));
    horizontal_sizer->Add(comboBoxSizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(5));

    return horizontal_sizer;
}

wxBoxSizer *CreateFilamentPresetDialog::create_filament_preset_item(wxWindow* parent)
{
    wxBoxSizer *horizontal_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxBoxSizer *  optionSizer        = new wxBoxSizer(wxVERTICAL);
    wxStaticText *static_filament_preset_text = new wxStaticText(parent, wxID_ANY, _L("Filament Preset"), wxDefaultPosition, wxDefaultSize);
    optionSizer->Add(static_filament_preset_text, 0, wxEXPAND | wxALL, 0);
    optionSizer->SetMinSize(OPTION_SIZE);
    horizontal_sizer->Add(optionSizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    wxBoxSizer *  comboBoxSizer  = new wxBoxSizer(wxVERTICAL);
    comboBoxSizer->Add(create_radio_item(m_create_type.base_filament, parent, wxEmptyString, m_create_type_btns), 0, wxEXPAND | wxALL, 0);

    
    m_filament_preset_combobox = new ComboBox(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, FILAMENT_PRESET_COMBOBOX_SIZE, 0, nullptr, wxCB_READONLY);
    m_filament_preset_combobox->SetLabel(_L("Select Filament Preset"));
    m_filament_preset_combobox->SetLabelColor(DEFAULT_PROMPT_TEXT_COLOUR);

    
    m_filament_preset_combobox->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent &e) {
        m_filament_preset_combobox->SetLabelColor(*wxBLACK);
        wxString filament_type = m_filament_preset_combobox->GetStringSelection();
        std::unordered_map<std::string, std::vector<Preset *>>::iterator iter = m_filament_choice_map.find(m_public_name_to_filament_id_map[filament_type]);
        
        m_scrolled_preset_panel->Freeze();
        m_filament_presets_sizer->Clear(true);
        m_filament_preset.clear();

        std::vector<std::pair<std::string, Preset *>> printer_name_to_filament_preset;
        if (iter != m_filament_choice_map.end()) {
            std::unordered_map<std::string, float> nozzle_diameter = nozzle_diameter_map;
            for (Preset* preset : iter->second) {
                auto compatible_printers = preset->config.option<ConfigOptionStrings>("compatible_printers", true);
                if (!compatible_printers || compatible_printers->values.empty()) {
                    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "there is a preset has no compatible printers and the preset name is: " << preset->name;
                    continue;
                }
                for (std::string &compatible_printer_name : compatible_printers->values) {
                    if (m_visible_printers.find(compatible_printer_name) == m_visible_printers.end()) {
                        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "there is a comppatible printer no exist: " << compatible_printer_name
                                                << "and the preset name is: " << preset->name;
                        continue;
                    }
                    std::string nozzle = get_printer_nozzle_diameter(compatible_printer_name);
                    if (nozzle_diameter[nozzle] == 0) {
                        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " compatible printer nozzle encounter exception and name is: " << compatible_printer_name;
                        continue;
                    }
                    printer_name_to_filament_preset.push_back(std::make_pair(compatible_printer_name,preset));
                    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "show compatible printer name: " << compatible_printer_name << "and preset name is: " << preset;
                }
            }
        } else {
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " not find filament_id corresponding to the type: and the type is" << filament_type;
        }
        sort_printer_by_nozzle(printer_name_to_filament_preset);
        for (std::pair<std::string, Preset *> printer_to_preset : printer_name_to_filament_preset)
            m_filament_presets_sizer->Add(create_checkbox(m_filament_preset_panel, printer_to_preset.first, printer_to_preset.second, m_filament_preset), 0,
                                          wxEXPAND | wxTOP | wxLEFT, FromDIP(5));
        m_scrolled_preset_panel->SetSizerAndFit(m_scrolled_sizer);
        m_scrolled_preset_panel->Thaw();

        update_dialog_size();
        e.Skip();
    });

    comboBoxSizer->Add(m_filament_preset_combobox, 0, wxEXPAND | wxTOP, FromDIP(5));

    comboBoxSizer->Add(create_radio_item(m_create_type.base_filament_preset, parent, wxEmptyString, m_create_type_btns), 0, wxEXPAND | wxTOP, FromDIP(10));

    
    horizontal_sizer->Add(comboBoxSizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    horizontal_sizer->Add(0, 0, 0, wxLEFT, FromDIP(30));

    return horizontal_sizer;

}

wxBoxSizer *CreateFilamentPresetDialog::create_filament_preset_for_printer_item()
{
    wxBoxSizer *vertical_sizer = new wxBoxSizer(wxVERTICAL);
    m_filament_preset_panel = new wxPanel(m_scrolled_preset_panel, wxID_ANY);
    // m_filament_preset_panel->SetBackgroundColour(PRINTER_LIST_COLOUR);
    m_filament_preset_panel->SetBackgroundColour(*wxWHITE);
    m_filament_preset_panel->SetSize(PRINTER_LIST_SIZE);
    m_filament_presets_sizer = new wxFlexGridSizer(1, FromDIP(5), FromDIP(5));
    // m_filament_presets_sizer->SetMinSize(FromDIP(912), FromDIP(-1));
    m_filament_preset_panel->SetSizer(m_filament_presets_sizer);
    vertical_sizer->Add(m_filament_preset_panel, 0, wxEXPAND | wxTOP | wxALIGN_CENTER_HORIZONTAL, FromDIP(5));

    return vertical_sizer;
}

wxBoxSizer *CreateFilamentPresetDialog::create_button_item()
{
    wxBoxSizer *bSizer_button = new wxBoxSizer(wxHORIZONTAL);
    bSizer_button->Add(0, 0, 1, wxEXPAND, 0);

    StateColor btn_bg_green(std::pair<wxColour, int>(wxColour(0, 137, 123), StateColor::Pressed), std::pair<wxColour, int>(wxColour(38, 166, 154), StateColor::Hovered),
                            std::pair<wxColour, int>(wxColour(0, 150, 136), StateColor::Normal));

    m_button_create = new Button(this, _L("Create"));
    m_button_create->SetBackgroundColor(btn_bg_green);
    m_button_create->SetBorderColor(*wxWHITE);
    m_button_create->SetTextColor(wxColour(0xFFFFFE));
    m_button_create->SetFont(Label::Body_12);
    m_button_create->SetSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_create->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_create->SetCornerRadius(FromDIP(12));
    bSizer_button->Add(m_button_create, 0, wxRIGHT, FromDIP(10));

    m_button_create->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent &e) { 
        //get vendor name
        wxString vendor_str = m_filament_vendor_combobox->GetLabel();
        std::string vendor_name;

        if (!m_can_not_find_vendor_checkbox->GetValue()) {
            if (_L("Select Vendor") == vendor_str) {
                MessageDialog dlg(this, _L("Vendor is not selected, please reselect vendor."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"),
                                  wxYES | wxYES_DEFAULT | wxCENTRE);
                dlg.ShowModal();
                return;
            } else {
                vendor_name = into_u8(vendor_str);
            }
        } else {
            if (m_filament_custom_vendor_input->GetTextCtrl()->GetValue().empty()) {
                MessageDialog dlg(this, _L("Custom vendor is not input, please input custom vendor."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"),
                                  wxYES | wxYES_DEFAULT | wxCENTRE);
                dlg.ShowModal();
                return;
            } else {
                vendor_name = into_u8(m_filament_custom_vendor_input->GetTextCtrl()->GetValue());
                if (vendor_name == "Bambu" || vendor_name == "Generic") {
                    MessageDialog dlg(this, _L("\"Bambu\" or \"Generic\" can not be used as a Vendor for custom filaments."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"),
                                      wxYES | wxYES_DEFAULT | wxCENTRE);
                    dlg.ShowModal();
                    return;
                }
            }
        }

        //get fialment type name
        wxString type_str = m_filament_type_combobox->GetLabel();
        std::string type_name;
        if (_L("Select Type") == type_str) {
            MessageDialog dlg(this, _L("Filament type is not selected, please reselect type."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES | wxYES_DEFAULT | wxCENTRE);
            dlg.ShowModal();
            return;
        } else {
            type_name = into_u8(type_str);
        }
        //get filament serial
        wxString    serial_str = m_filament_serial_input->GetTextCtrl()->GetValue();
        std::string serial_name;
        if (serial_str.empty()) {
            MessageDialog dlg(this, _L("Filament serial is not inputed, please input serial."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"),
                              wxYES | wxYES_DEFAULT | wxCENTRE);
            dlg.ShowModal();
            return;
        } else {
            serial_name = into_u8(serial_str);
        }
        vendor_name = remove_special_key(vendor_name);
        serial_name = remove_special_key(serial_name);
        
        if (vendor_name.empty() || serial_name.empty()) {
            MessageDialog dlg(this, _L("There may be escape characters in the vendor or serial input of filament. Please delete and re-enter."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"),
                              wxYES | wxYES_DEFAULT | wxCENTRE);
            dlg.ShowModal();
            return;
        }
        boost::algorithm::trim(vendor_name);
        boost::algorithm::trim(serial_name);
        if (vendor_name.empty() || serial_name.empty()) {
            MessageDialog dlg(this, _L("All inputs in the custom vendor or serial are spaces. Please re-enter."),
                              wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES | wxYES_DEFAULT | wxCENTRE);
            dlg.ShowModal();
            return;
        }
        if (m_can_not_find_vendor_checkbox->GetValue() && str_is_all_digit(vendor_name)) {
            MessageDialog dlg(this, _L("The vendor can not be a number. Please re-enter."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"),
                              wxYES | wxYES_DEFAULT | wxCENTRE);
            dlg.ShowModal();
            return;
        }

        if (!is_check_box_selected()) {
            MessageDialog dlg(this, _L("You have not selected a printer or preset yet. Please select at least one."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"),
                              wxYES | wxYES_DEFAULT | wxCENTRE);
            dlg.ShowModal();
            return;
        }

        std::string filament_preset_name = vendor_name + " " + (type_name == "PLA-AERO" ? "PLA Aero" : type_name) + " " + serial_name;
        PresetBundle *preset_bundle        = wxGetApp().preset_bundle;
        if (preset_bundle->filaments.is_alias_exist(filament_preset_name)) {
            MessageDialog dlg(this,
                              wxString::Format(_L("The Filament name %s you created already exists. \nIf you continue creating, the preset created will be displayed with its "
                                                  "full name. Do you want to continue?"),
                                               from_u8(filament_preset_name)),
                              wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES_NO | wxYES_DEFAULT | wxCENTRE);
            if (wxID_YES != dlg.ShowModal()) { return; }
        }

        std::string user_filament_id     = get_filament_id(filament_preset_name);

        const wxString &curr_create_type = curr_create_filament_type();
        
        if (curr_create_type == m_create_type.base_filament) {
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ":clone filament  create type  filament ";
            for (const std::pair<::CheckBox *, std::pair<std::string, Preset *>> &checkbox_preset : m_filament_preset) {
                if (checkbox_preset.first->GetValue()) { 
                    std::string compatible_printer_name = checkbox_preset.second.first;
                    std::vector<std::string> failures;
                    Preset const *const      checked_preset = checkbox_preset.second.second;
                    DynamicConfig            dynamic_config;
                    dynamic_config.set_key_value("filament_vendor", new ConfigOptionStrings({vendor_name}));
                    dynamic_config.set_key_value("compatible_printers", new ConfigOptionStrings({compatible_printer_name}));
                    dynamic_config.set_key_value("filament_type", new ConfigOptionStrings({type_name}));
                    bool res = preset_bundle->filaments.clone_presets_for_filament(checked_preset, failures, filament_preset_name, user_filament_id, dynamic_config,
                                                                                   compatible_printer_name);
                    if (!res) {
                        std::string failure_names;
                        for (std::string &failure : failures) { failure_names += failure + "\n"; }
                        MessageDialog dlg(this, _L("Some existing presets have failed to be created, as follows:\n") + from_u8(failure_names) + _L("\nDo you want to rewrite it?"),
                                          wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES_NO | wxYES_DEFAULT | wxCENTRE);
                        if (dlg.ShowModal() == wxID_YES) {
                            res = preset_bundle->filaments.clone_presets_for_filament(checked_preset, failures, filament_preset_name, user_filament_id, dynamic_config,
                                                                                      compatible_printer_name, true);
                            BOOST_LOG_TRIVIAL(info) << "clone filament  have failures  rewritten  is successful? " << res;
                        }
                    }
                    BOOST_LOG_TRIVIAL(info) << "clone filament  no failures  is successful? " << res;
                }
            }
        } else if (curr_create_type == m_create_type.base_filament_preset) {
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ":clone filament presets  create type  filament preset";
            for (const std::pair<::CheckBox *, std::pair<std::string, Preset *>> &checkbox_preset : m_machint_filament_preset) {
                if (checkbox_preset.first->GetValue()) {
                    std::string compatible_printer_name = checkbox_preset.second.first;
                    std::vector<std::string> failures;
                    Preset const *const      checked_preset = checkbox_preset.second.second;
                    DynamicConfig            dynamic_config;
                    dynamic_config.set_key_value("filament_vendor", new ConfigOptionStrings({vendor_name}));
                    dynamic_config.set_key_value("compatible_printers", new ConfigOptionStrings({compatible_printer_name}));
                    dynamic_config.set_key_value("filament_type", new ConfigOptionStrings({type_name}));
                    bool res = preset_bundle->filaments.clone_presets_for_filament(checked_preset, failures, filament_preset_name, user_filament_id, dynamic_config,
                                                                                   compatible_printer_name);
                    if (!res) {
                        std::string failure_names;
                        for (std::string &failure : failures) { failure_names += failure + "\n"; }
                        MessageDialog dlg(this, _L("Some existing presets have failed to be created, as follows:\n") + from_u8(failure_names) + _L("\nDo you want to rewrite it?"),
                                          wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES_NO | wxYES_DEFAULT | wxCENTRE);
                        if (wxID_YES == dlg.ShowModal()) {
                            res = preset_bundle->filaments.clone_presets_for_filament(checked_preset, failures, filament_preset_name, user_filament_id, dynamic_config,
                                                                                      compatible_printer_name, true);
                            BOOST_LOG_TRIVIAL(info) << "clone filament presets  have failures  rewritten  is successful? " << res;
                        }
                    }
                    BOOST_LOG_TRIVIAL(info) << "clone filament presets  no failures  is successful? " << res << " old preset is: " << checked_preset->name
                                            << " compatible_printer_name is: " << compatible_printer_name;
                }
            }
        }
        preset_bundle->update_compatible(PresetSelectCompatibleType::Always);
        EndModal(wxID_OK); 
        });

    StateColor btn_bg_white(std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Pressed), std::pair<wxColour, int>(wxColour(238, 238, 238), StateColor::Hovered),
                            std::pair<wxColour, int>(*wxWHITE, StateColor::Normal));

    m_button_cancel = new Button(this, _L("Cancel"));
    m_button_cancel->SetBackgroundColor(btn_bg_white);
    m_button_cancel->SetBorderColor(wxColour(38, 46, 48));
    m_button_cancel->SetFont(Label::Body_12);
    m_button_cancel->SetSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_cancel->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_cancel->SetCornerRadius(FromDIP(12));
    bSizer_button->Add(m_button_cancel, 0, wxRIGHT, FromDIP(10));

    m_button_cancel->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent &e) { 
        EndModal(wxID_CANCEL); 
        });

    return bSizer_button;
}

wxArrayString CreateFilamentPresetDialog::get_filament_preset_choices()
{ 
    wxArrayString choices;
    // get fialment type name
    wxString    type_str = m_filament_type_combobox->GetLabel();
    std::string type_name;
    if (_L("Select Type") == type_str) {
        /*MessageDialog dlg(this, _L("Filament type is not selected, please reselect type."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES | wxYES_DEFAULT | wxCENTRE);
        dlg.ShowModal();*/
        return choices;
    } else {
        type_name = into_u8(type_str);
    }

    for (std::pair<std::string, Preset*> filament_presets : m_all_presets_map) {
        Preset *preset = filament_presets.second;
        auto    inherit = preset->config.option<ConfigOptionString>("inherits");
        if (inherit && !inherit->value.empty()) {
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " inherit user preset is:" << preset->name << " and inherits is: " << inherit->value;
            continue;
        }
        auto fila_type = preset->config.option<ConfigOptionStrings>("filament_type");
        if (!fila_type || fila_type->values.empty() || type_name != fila_type->values[0]) continue;
        m_filament_choice_map[preset->filament_id].push_back(preset);
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " base user preset is:" << preset->name;
    }
    
    int suffix = 0;
    for (const pair<std::string, std::vector<Preset *>> &preset : m_filament_choice_map) { 
        if (preset.second.empty()) continue;
        std::set<wxString> preset_name_set;
        for (Preset* filament_preset : preset.second) { 
            std::string preset_name = filament_preset->name;
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " filament_id: " << filament_preset->filament_id << " preset name: " << filament_preset->name;
            size_t      index_at    = preset_name.find(" @");
            if (std::string::npos != index_at) {
                std::string cur_preset_name = preset_name.substr(0, index_at);
                preset_name_set.insert(from_u8(cur_preset_name));
            }
        }
        assert(1 == preset_name_set.size());
        if (preset_name_set.size() > 1) {
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " the same filament has different filament(vendor type serial)"; 
        }
        for (const wxString& public_name : preset_name_set) {
            if (m_public_name_to_filament_id_map.find(public_name) != m_public_name_to_filament_id_map.end()) {
                suffix++;
                m_public_name_to_filament_id_map[public_name + "_" + std::to_string(suffix)] = preset.first;
                choices.Add(public_name + "_" + std::to_string(suffix));
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " add filament choice: " << choices.back();
            } else {
                m_public_name_to_filament_id_map[public_name] = preset.first;
                choices.Add(public_name);
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " add filament choice: " << choices.back();
            }
        }
    }

    return choices;
}

wxBoxSizer *CreateFilamentPresetDialog::create_radio_item(wxString title, wxWindow *parent, wxString tooltip, std::vector<std::pair<RadioBox *, wxString>> &radiobox_list)
{
    wxBoxSizer *horizontal_sizer = new wxBoxSizer(wxHORIZONTAL);
    RadioBox *  radiobox         = new RadioBox(parent);
    horizontal_sizer->Add(radiobox, 0, wxEXPAND | wxALL, 0);
    horizontal_sizer->Add(0, 0, 0, wxEXPAND | wxLEFT, FromDIP(5));
    radiobox_list.push_back(std::make_pair(radiobox, title));
    int btn_idx = radiobox_list.size() - 1;
    radiobox->Bind(wxEVT_LEFT_DOWN, [this, &radiobox_list, btn_idx](wxMouseEvent &e) { select_curr_radiobox(radiobox_list, btn_idx); });

    wxStaticText *text = new wxStaticText(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize);
    text->Bind(wxEVT_LEFT_DOWN, [this, &radiobox_list, btn_idx](wxMouseEvent &e) { select_curr_radiobox(radiobox_list, btn_idx); });
    horizontal_sizer->Add(text, 0, wxEXPAND | wxLEFT, 0);

    radiobox->SetToolTip(tooltip);
    text->SetToolTip(tooltip);
    return horizontal_sizer;
}

void CreateFilamentPresetDialog::select_curr_radiobox(std::vector<std::pair<RadioBox *, wxString>> &radiobox_list, int btn_idx)
{
    int len = radiobox_list.size();
    for (int i = 0; i < len; ++i) {
        if (i == btn_idx) {
            radiobox_list[i].first->SetValue(true);
            const wxString &curr_selected_type = radiobox_list[i].second;
            this->Freeze();
            if (curr_selected_type == m_create_type.base_filament) {
                //m_filament_preset_text->SetLabel(_L("We could create the filament presets for your following printer:"));
                m_filament_preset_combobox->Show();
                if (_L("Select Type") != m_filament_type_combobox->GetLabel()) {
                    clear_filament_preset_map();
                    wxArrayString filament_preset_choice = get_filament_preset_choices();
                    m_filament_preset_combobox->Set(filament_preset_choice);
                    m_filament_preset_combobox->SetLabel(_L("Select Filament Preset"));
                    m_filament_preset_combobox->SetLabelColor(DEFAULT_PROMPT_TEXT_COLOUR);
                }
            } else if (curr_selected_type == m_create_type.base_filament_preset) {
                //m_filament_preset_text->SetLabel(_L("We would rename the presets as \"Vendor Type Serial @printer you selected\". \nTo add preset for more printers, Please go to printer selection"));
                m_filament_preset_combobox->Hide();
                if (_L("Select Type") != m_filament_type_combobox->GetLabel()) {
                    
                    clear_filament_preset_map();
                    get_filament_presets_by_machine();
                    
                }
            }
            m_scrolled_preset_panel->SetSizerAndFit(m_scrolled_sizer);
            this->Thaw();
        } else {
            radiobox_list[i].first->SetValue(false);
        }
    }
    update_dialog_size();
}

wxString CreateFilamentPresetDialog::curr_create_filament_type()
{
    wxString curr_filament_type;
    for (const std::pair<RadioBox *, wxString> &printer_radio : m_create_type_btns) {
        if (printer_radio.first->GetValue()) {
            curr_filament_type = printer_radio.second;
        }
    }
    return curr_filament_type;
}

void CreateFilamentPresetDialog::get_filament_presets_by_machine()
{
    wxArrayString choices;
    // get fialment type name
    wxString    type_str = m_filament_type_combobox->GetLabel();
    std::string type_name;
    if (_L("Select Type") == type_str) {
        /*MessageDialog dlg(this, _L("Filament type is not selected, please reselect type."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES | wxYES_DEFAULT |
        wxCENTRE); dlg.ShowModal();*/
        return;
    } else {
        type_name = into_u8(type_str);
    }
    
    std::unordered_map<std::string, float>                 nozzle_diameter = nozzle_diameter_map;
    std::unordered_map<std::string, std::vector<Preset *>> machine_name_to_presets;
    PresetBundle *                                         preset_bundle = wxGetApp().preset_bundle;
    for (std::pair<std::string, Preset*> filament_preset : m_all_presets_map) {
        Preset *    preset      = filament_preset.second;
        auto    compatible_printers = preset->config.option<ConfigOptionStrings>("compatible_printers", true);
        if (!compatible_printers || compatible_printers->values.empty()) {
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "there is a preset has no compatible printers and the preset name is: " << preset->name;
            continue;
        }
        for (std::string &compatible_printer_name : compatible_printers->values) {
            if (m_visible_printers.find(compatible_printer_name) == m_visible_printers.end()) {
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " compatable printer is not visible and preset name is: " << preset->name;
                continue;
            }
            Preset *             inherit_preset = nullptr;
            auto inherit = dynamic_cast<ConfigOptionString*>(preset->config.option(BBL_JSON_KEY_INHERITS,false));
            if (inherit && !inherit->value.empty()) {
                std::string inherits_value = inherit->value;
                inherit_preset             = preset_bundle->filaments.find_preset(inherits_value, false, true);
            }
            ConfigOptionStrings *filament_types;
            if (!inherit_preset) {
                filament_types = dynamic_cast<ConfigOptionStrings *>(preset->config.option("filament_type"));
            } else {
                filament_types = dynamic_cast<ConfigOptionStrings *>(inherit_preset->config.option("filament_type"));
            }

            if (filament_types && filament_types->values.empty()) continue;
            const std::string filament_type = filament_types->values[0];
            if (filament_type != type_name) {
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " preset type is not selected type and preset name is: " << preset->name;
                continue;
            }
            std::string nozzle = get_printer_nozzle_diameter(compatible_printer_name);
            if (nozzle_diameter[nozzle] == 0) {
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " compatible printer nozzle encounter exception and name is: " << compatible_printer_name;
                continue;
            }
            machine_name_to_presets[compatible_printer_name].push_back(preset);
        }
    }
    std::vector<std::pair<std::string, std::vector<Preset *>>> printer_name_to_filament_presets;
    for (std::pair<std::string, std::vector<Preset *>> machine_filament_presets : machine_name_to_presets) {
        printer_name_to_filament_presets.push_back(machine_filament_presets);
    }
    sort_printer_by_nozzle(printer_name_to_filament_presets);
    m_filament_preset_panel->Freeze();
    for (std::pair<std::string, std::vector<Preset *>> machine_filament_presets : printer_name_to_filament_presets) {
        std::string            compatible_printer = machine_filament_presets.first;
        std::vector<Preset *> &presets      = machine_filament_presets.second;
        m_filament_presets_sizer->Add(create_select_filament_preset_checkbox(m_filament_preset_panel, compatible_printer, presets, m_machint_filament_preset), 0, wxEXPAND | wxALL, FromDIP(5));
    }
    m_filament_preset_panel->Thaw();
}

void CreateFilamentPresetDialog::get_all_filament_presets()
{
    // temp filament presets
    PresetBundle temp_preset_bundle;
    std::string dir_user_presets = wxGetApp().app_config->get("preset_folder");
    if (dir_user_presets.empty()) {
        temp_preset_bundle.load_user_presets(DEFAULT_USER_FOLDER_NAME, ForwardCompatibilitySubstitutionRule::EnableSilent);
    } else {
        temp_preset_bundle.load_user_presets(dir_user_presets, ForwardCompatibilitySubstitutionRule::EnableSilent);
    }
    const std::deque<Preset> &filament_presets = temp_preset_bundle.filaments.get_presets();

    for (const Preset &preset : filament_presets) {
        if (preset.filament_id.empty() || "null" == preset.filament_id) continue;
        std::string filament_preset_name = preset.name;
        Preset *filament_preset = new Preset(preset);
        m_all_presets_map[filament_preset_name] = filament_preset;
    }
    // global filament presets
    PresetBundle * preset_bundle = wxGetApp().preset_bundle;
    const std::deque<Preset> &temp_filament_presets = preset_bundle->filaments.get_presets();
    for (const Preset& preset : temp_filament_presets) {
        if (preset.filament_id.empty() || "null" == preset.filament_id) continue;
        auto filament_type = preset.config.option<ConfigOptionStrings>("filament_type");
        if (filament_type && filament_type->values.size())
            m_system_filament_types_set.insert(filament_type->values[0]);
        if (!preset.is_visible) continue;
        std::string filament_preset_name        = preset.name;
        Preset *filament_preset                 = new Preset(preset);
        m_all_presets_map[filament_preset_name] = filament_preset;
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " loaded preset name is: " << filament_preset->name;
    }
}

void CreateFilamentPresetDialog::get_all_visible_printer_name()
{
    PresetBundle *preset_bundle = wxGetApp().preset_bundle;
    for (const Preset &printer_preset : preset_bundle->printers.get_presets()) {
        if (!printer_preset.is_visible) continue;
        assert(m_visible_printers.find(printer_preset.name) == m_visible_printers.end());
        m_visible_printers.insert(printer_preset.name);
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " entry, and visible printer is: " << printer_preset.name;
    }

}

void CreateFilamentPresetDialog::update_dialog_size()
{
    this->Freeze();
    m_filament_preset_panel->SetSizerAndFit(m_filament_presets_sizer);
    int width      = m_filament_preset_panel->GetSize().GetWidth();
    int height     = m_filament_preset_panel->GetSize().GetHeight();
    m_scrolled_preset_panel->SetMinSize(wxSize(std::min(1400, width + FromDIP(26)), std::min(600, height + FromDIP(18))));
    m_scrolled_preset_panel->SetMaxSize(wxSize(std::min(1400, width + FromDIP(26)), std::min(600, height + FromDIP(18))));
    m_scrolled_preset_panel->SetSize(wxSize(std::min(1500, width + FromDIP(26)), std::min(600, height + FromDIP(18))));
    Layout();
    Fit();
    Refresh();
    adjust_dialog_in_screen(this);
    this->Thaw();
}

template<typename T>
void CreateFilamentPresetDialog::sort_printer_by_nozzle(std::vector<std::pair<std::string, T>> &printer_name_to_filament_preset)
{
    std::unordered_map<std::string, float> nozzle_diameter = nozzle_diameter_map;
    std::sort(printer_name_to_filament_preset.begin(), printer_name_to_filament_preset.end(),
              [&nozzle_diameter](const std::pair<string, T> &a, const std::pair<string, T> &b) {
                  size_t nozzle_index_a = a.first.find(" nozzle");
                  size_t nozzle_index_b = b.first.find(" nozzle");
                  if (nozzle_index_a == std::string::npos || nozzle_index_b == std::string::npos) return a.first < b.first;
                  std::string nozzle_str_a;
                  std::string nozzle_str_b;
                  try {
                      nozzle_str_a = a.first.substr(0, nozzle_index_a);
                      nozzle_str_b = b.first.substr(0, nozzle_index_b);
                      size_t last_space_index = nozzle_str_a.find_last_of(" ");
                      nozzle_str_a            = nozzle_str_a.substr(last_space_index + 1);
                      last_space_index        = nozzle_str_b.find_last_of(" ");
                      nozzle_str_b            = nozzle_str_b.substr(last_space_index + 1);
                  } catch (...) {
                      BOOST_LOG_TRIVIAL(info) << "substr filed, and printer name is: " << a.first << " and " << b.first;
                      return a.first < b.first;
                  }
                  float nozzle_a, nozzle_b;
                  try {
                      nozzle_a = nozzle_diameter[nozzle_str_a];
                      nozzle_b = nozzle_diameter[nozzle_str_b];
                      assert(nozzle_a != 0 && nozzle_b != 0);
                  } catch (...) {
                      BOOST_LOG_TRIVIAL(info) << "find nozzle filed, and nozzle is: " << nozzle_str_a << "mm and " << nozzle_str_b << "mm";
                      return a.first < b.first;
                  }
                  float diff_nozzle_a = std::abs(nozzle_a - 0.4);
                  float diff_nozzle_b = std::abs(nozzle_b - 0.4);
                  if (nozzle_a == nozzle_b) return a.first < b.first;
                  if (diff_nozzle_a == diff_nozzle_b) return nozzle_a < nozzle_b;

                  return diff_nozzle_a < diff_nozzle_b;
              });
}

void CreateFilamentPresetDialog::clear_filament_preset_map()
{
    m_filament_choice_map.clear();
    m_filament_preset.clear();
    m_machint_filament_preset.clear();
    m_public_name_to_filament_id_map.clear();
    m_filament_preset_panel->Freeze();
    m_filament_presets_sizer->Clear(true);
    m_filament_preset_panel->Thaw();
}

CreatePrinterPresetDialog::CreatePrinterPresetDialog(wxWindow *parent,int iType) 
: DPIDialog(parent ? parent : nullptr, wxID_ANY, iType ? _L("Create Nozzle") : _L("Create Printer"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX | wxCENTER)
{
    m_create_type.create_printer    = _L("Create Printer");
    m_create_type.create_nozzle     = _L("Create Nozzle for Existing Printer");
    m_create_type.base_template     = _L("Create from Template");
    m_create_type.base_curr_printer = _L("Create Based on Current Printer");
    m_type = iType;
    m_nozzle = 0.4;

    this->SetBackgroundColour(*wxWHITE);
    SetSizeHints(wxDefaultSize, wxDefaultSize);

    std::string icon_path = (boost::format("%1%/images/Creative3DTitle.ico") % resources_dir()).str();
    SetIcon(wxIcon(encode_path(icon_path.c_str()), wxBITMAP_TYPE_ICO));

    wxBoxSizer *m_main_sizer = new wxBoxSizer(wxVERTICAL);
    // top line
    auto m_line_top = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 2), wxTAB_TRAVERSAL);
    m_line_top->SetBackgroundColour(wxColour(0xA6, 0xa9, 0xAA));
    m_main_sizer->Add(m_line_top, 0, wxEXPAND, 0);
    m_main_sizer->Add(0, 0, 0, wxTOP, FromDIP(5));
    m_main_sizer->Add(create_step_switch_item(), 0, wxEXPAND | wxALL, FromDIP(5));

    wxBoxSizer *page_sizer = new wxBoxSizer(wxHORIZONTAL);

    m_page1 = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    m_page1->SetBackgroundColour(*wxWHITE);
    m_page1->SetScrollRate(5, 5);
    m_page2 = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);\
    m_page2->SetBackgroundColour(*wxWHITE);

    create_printer_page1(m_page1);
    create_printer_page2(m_page2);
    m_page2->Hide();

    page_sizer->Add(m_page1, 1, wxEXPAND, 0);
    page_sizer->Add(m_page2, 1, wxEXPAND, 0);
    m_main_sizer->Add(page_sizer, 0, wxEXPAND | wxRIGHT, FromDIP(10));
    select_curr_radiobox(m_create_type_btns, iType);
    select_curr_radiobox(m_create_presets_btns, 0);
    

    m_main_sizer->Add(0, 0, 0, wxTOP, FromDIP(10));

    this->SetSizer(m_main_sizer);

    Layout();
    Fit();

    wxSize screen_size = wxGetDisplaySize();
    int    dialogX     = (screen_size.GetWidth() - GetSize().GetWidth()) / 2;
    int    dialogY     = (screen_size.GetHeight() - GetSize().GetHeight()) / 2;
    SetPosition(wxPoint(dialogX, dialogY));

    wxGetApp().UpdateDlgDarkUI(this);
}

CreatePrinterPresetDialog::~CreatePrinterPresetDialog()
{
    clear_preset_combobox();
    if (m_printer_preset) {
        delete m_printer_preset;
        m_printer_preset = nullptr;
    }
}

void CreatePrinterPresetDialog::on_dpi_changed(const wxRect &suggested_rect) {
    m_button_OK->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_OK->SetMaxSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_OK->SetCornerRadius(FromDIP(12));
    m_button_create->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_create->SetMaxSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_create->SetCornerRadius(FromDIP(12));
    m_button_page1_cancel->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_page1_cancel->SetMaxSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_page1_cancel->SetCornerRadius(FromDIP(12));
    m_button_page2_cancel->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_page2_cancel->SetMaxSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_page2_cancel->SetCornerRadius(FromDIP(12));
    m_button_page2_back->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_page2_back->SetMaxSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_page2_back->SetCornerRadius(FromDIP(12));
    Layout();
}

wxBoxSizer *CreatePrinterPresetDialog::create_step_switch_item()
{ 
    wxBoxSizer *step_switch_sizer = new wxBoxSizer(wxVERTICAL); 

    // std::string      wiki_url             = "https://wiki.bambulab.com/en/software/bambu-studio/3rd-party-printer-profile";
    // wxHyperlinkCtrl *m_download_hyperlink = new wxHyperlinkCtrl(this, wxID_ANY, _L("wiki"), wiki_url, wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE);
    // step_switch_sizer->Add(m_download_hyperlink, 0,  wxRIGHT | wxALIGN_RIGHT, FromDIP(5));

    wxBoxSizer *horizontal_sizer  = new wxBoxSizer(wxHORIZONTAL);
    wxPanel *   step_switch_panel = new wxPanel(this);
    step_switch_panel->SetBackgroundColour(*wxWHITE);
    horizontal_sizer->Add(0, 0, 1, wxEXPAND,0);
    m_step_1 = new wxStaticBitmap(step_switch_panel, wxID_ANY, create_scaled_bitmap("step_1", nullptr, FromDIP(20)), wxDefaultPosition, wxDefaultSize);
    horizontal_sizer->Add(m_step_1, 0, wxEXPAND | wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, FromDIP(3));
    wxStaticText *static_create_printer_text = new wxStaticText(step_switch_panel, wxID_ANY, m_type ? _L("Create Nozzle") : _L("Create Printer"), wxDefaultPosition, wxDefaultSize);
    horizontal_sizer->Add(static_create_printer_text, 0, wxEXPAND | wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, FromDIP(3));
    auto divider_line = new wxPanel(step_switch_panel, wxID_ANY, wxDefaultPosition, wxSize(FromDIP(50), 1));
    divider_line->SetBackgroundColour(PRINTER_LIST_COLOUR);
    horizontal_sizer->Add(divider_line, 0, wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, FromDIP(3));
    m_step_2 = new wxStaticBitmap(step_switch_panel, wxID_ANY, create_scaled_bitmap("step_2_ready", nullptr, FromDIP(20)), wxDefaultPosition, wxDefaultSize);
    horizontal_sizer->Add(m_step_2, 0, wxEXPAND | wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, FromDIP(3));
    wxStaticText *static_import_presets_text = new wxStaticText(step_switch_panel, wxID_ANY, _L("Import Preset"), wxDefaultPosition, wxDefaultSize);
    horizontal_sizer->Add(static_import_presets_text, 0, wxEXPAND | wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, FromDIP(3));
    horizontal_sizer->Add(0, 0, 1, wxEXPAND, 0);

    step_switch_panel->SetSizer(horizontal_sizer);

    step_switch_sizer->Add(step_switch_panel, 0, wxBOTTOM | wxALIGN_CENTER_HORIZONTAL, FromDIP(10));

    auto line_top = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 1), wxTAB_TRAVERSAL);
    line_top->SetBackgroundColour(PRINTER_LIST_COLOUR);

    step_switch_sizer->Add(line_top, 0, wxEXPAND | wxALL, FromDIP(10));
    
    return step_switch_sizer;
}

void CreatePrinterPresetDialog::create_printer_page1(wxWindow *parent)
{ 
    this->SetBackgroundColour(*wxWHITE);

    m_page1_sizer = new wxBoxSizer(wxVERTICAL); 

    {   
        //隐藏勾选打印机和喷嘴的checkbox
        wxPanel* panelTmp = new wxPanel(parent);
        wxBoxSizer* boxSizerTmp = new wxBoxSizer(wxVERTICAL);
        boxSizerTmp->Add(create_type_item(panelTmp), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));
        panelTmp->SetSizer(boxSizerTmp);
        panelTmp->Hide();
    }

    //m_page1_sizer->Add(create_type_item(parent), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));
    m_page1_sizer->Add(create_printer_item(parent), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));

    if (m_type)
    {
        m_page1_sizer->Add(create_nozzle_diameter_item(parent), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));
        m_page1_sizer->Add(create_printer_preset_item(parent), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));
    }
    else
    {
        m_page1_sizer->Add(create_printer_preset_item(parent), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));
        m_page1_sizer->Add(create_nozzle_diameter_item(parent), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));
    }

    m_printer_info_pane_nozzle = new wxPanel(parent);
    m_printer_info_pane_nozzle->SetBackgroundColour(*wxWHITE);
    m_printer_info_sizer_nozzle = new wxBoxSizer(wxVERTICAL);
    m_printer_info_sizer_nozzle->Add(create_height_limit(m_printer_info_pane_nozzle), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));
    m_printer_info_sizer_nozzle->Add(create_displacement_distance(m_printer_info_pane_nozzle), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));
    m_printer_info_pane_nozzle->SetSizer(m_printer_info_sizer_nozzle);
    m_page1_sizer->Add(m_printer_info_pane_nozzle, 0, wxEXPAND, 0);

    m_printer_info_panel = new wxPanel(parent);
    m_printer_info_panel->SetBackgroundColour(*wxWHITE);
    m_printer_info_sizer = new wxBoxSizer(wxVERTICAL);
    m_printer_info_sizer->Add(create_bed_shape_item(m_printer_info_panel), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));
    m_printer_info_sizer->Add(create_bed_size_item(m_printer_info_panel), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));
    m_printer_info_sizer->Add(create_origin_item(m_printer_info_panel), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));
    m_printer_info_sizer->Add(create_hot_bed_stl_item(m_printer_info_panel), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));
    m_printer_info_sizer->Add(create_hot_bed_svg_item(m_printer_info_panel), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));
    m_printer_info_sizer->Add(create_max_print_height_item(m_printer_info_panel), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));
    m_printer_info_panel->SetSizer(m_printer_info_sizer);
    m_page1_sizer->Add(m_printer_info_panel, 0, wxEXPAND, 0);
    m_page1_sizer->Add(create_page1_btns_item(parent), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));

    data_init();

    parent->SetSizerAndFit(m_page1_sizer);
    Layout();

    wxGetApp().UpdateDlgDarkUI(this);
}

wxBoxSizer *CreatePrinterPresetDialog::create_type_item(wxWindow *parent)
{
    wxBoxSizer *horizontal_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxBoxSizer *  optionSizer        = new wxBoxSizer(wxVERTICAL);
    wxStaticText *static_serial_text = new wxStaticText(parent, wxID_ANY, _L("Create Type"), wxDefaultPosition, wxDefaultSize);
    optionSizer->Add(static_serial_text, 0, wxEXPAND | wxALL, 0);
    optionSizer->SetMinSize(OPTION_SIZE);
    horizontal_sizer->Add(optionSizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    wxBoxSizer *radioBoxSizer = new wxBoxSizer(wxVERTICAL);
    
    radioBoxSizer->Add(create_radio_item(m_create_type.create_printer, parent, wxEmptyString, m_create_type_btns), 0, wxEXPAND | wxALL, 0);
    radioBoxSizer->Add(create_radio_item(m_create_type.create_nozzle, parent, wxEmptyString, m_create_type_btns), 0, wxEXPAND | wxTOP, FromDIP(10));
    horizontal_sizer->Add(radioBoxSizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    return horizontal_sizer;
}

void CreatePrinterPresetDialog::update_nozzle_other_data_new()
{
    if(!m_printer_preset)  
        return ;

    float min_height = m_printer_preset->config.opt_float("min_layer_height", 0);
    float max_height = m_printer_preset->config.opt_float("max_layer_height", 0);
    float length = m_printer_preset->config.opt_float("retraction_length", 0);

    string str_min_height = wxString::Format("%.2f", min_height).ToStdString();
    string str_max_height = wxString::Format("%.2f", max_height).ToStdString();
    string str_length = wxString::Format("%.2f", length).ToStdString();

    m_print_min_height_input->GetTextCtrl()->SetValue(str_min_height);
    m_print_max_height_input->GetTextCtrl()->SetValue(str_max_height);
    m_print_length_input->GetTextCtrl()->SetValue(str_length);
}

void CreatePrinterPresetDialog::update_printer_other_data_new()
{
    if (!m_printer_preset)
        return;

    double dNozzle =m_printer_preset->config.option<ConfigOptionFloats>("nozzle_diameter")->get_at(0);
    int count = m_nozzle_diameter->GetCount(); // 获取选项的数量
    for (int i = 0; i < count; i++) 
    {
        double diameter = std::stod(m_nozzle_diameter->GetString(i).ToStdString());
        if (std::abs(dNozzle - diameter) < 0.01) 
        {
            m_nozzle_diameter->SetSelection(i);
            break; 
        }
    }

    int iPrinterHeight = (int)(m_printer_preset->config.opt_float("printable_height"));
    m_print_height_input->GetTextCtrl()->SetValue(std::to_string(iPrinterHeight));

    Pointfs bedfs = m_printer_preset->config.opt<ConfigOptionPoints>("printable_area")->values;
    double  current_width = bedfs[2].x() - bedfs[0].x();
    double  current_depth = bedfs[2].y() - bedfs[0].y();
    double  origin_x = 0.0 - bedfs[0].x();
    double  origin_y = 0.0 - bedfs[0].y();

    string size_x_input = wxString::Format("%.1f", current_width).ToStdString();
    string size_y_input = wxString::Format("%.1f", current_depth).ToStdString();
    string origin_x_input = wxString::Format("%.1f", origin_x).ToStdString();
    string origin_y_input = wxString::Format("%.1f", origin_y).ToStdString();

    m_bed_size_x_input->GetTextCtrl()->SetValue(size_x_input);
    m_bed_size_y_input->GetTextCtrl()->SetValue(size_y_input);
    m_bed_origin_x_input->GetTextCtrl()->SetValue(origin_x_input);
    m_bed_origin_y_input->GetTextCtrl()->SetValue(origin_y_input);

    string strSTL = m_printer_preset->config.option<ConfigOptionString>("bed_custom_model")->value;
    string strSVG = m_printer_preset->config.option<ConfigOptionString>("bed_custom_texture")->value;

    m_custom_model = strSTL;
    wxGCDC dcSTL;
    auto textSTL = wxControl::Ellipsize(_L(boost::filesystem::path(strSTL).filename().string()), dcSTL, wxELLIPSIZE_END, FromDIP(200));
    strSTL.empty() ? m_upload_stl_tip_text->SetLabelText(_L("Empty")) : m_upload_stl_tip_text->SetLabelText(textSTL);

    m_custom_texture = strSVG;
    wxGCDC dcSVG;
    auto textSVG = wxControl::Ellipsize(_L(boost::filesystem::path(strSVG).filename().string()), dcSVG, wxELLIPSIZE_END, FromDIP(200));
    strSVG.empty() ? m_upload_svg_tip_text->SetLabelText(_L("Empty")) : m_upload_svg_tip_text->SetLabelText(textSVG);
}


void CreatePrinterPresetDialog::update_nozzle_data_new()
{
    std::string selected_model_name = into_u8(m_select_printer->GetStringSelection());

    std::vector<std::string> nozzleInfo = nozzle_diameter_vec;
    for (int i = 0; i < m_printer_nozzle_info.size(); i++)
    {
        string strModel = m_printer_nozzle_info[i].model_name;
        if (selected_model_name == strModel)
        {
            for (int j = 0; j < m_printer_nozzle_info[i].vec_nozzle.size(); j++)
            {
                std::string strTmp = m_printer_nozzle_info[i].vec_nozzle[j];
                auto it = std::find(nozzleInfo.begin(), nozzleInfo.end(), strTmp);
                if (it != nozzleInfo.end())
                {
                    nozzleInfo.erase(it);
                }
            }
            break;
        }
    }

    std::vector<double> vecNozzel;
    for (const auto& pair : m_printer_name_to_preset)
    {
        const std::string& printerName = pair.first;
        const auto& preset = pair.second;
        string strNametmp = preset->config.option<ConfigOptionString>("printer_model", false)->value;
        if (selected_model_name == strNametmp)
        {
            double dNozzleTmp = preset->config.option<ConfigOptionFloats>("nozzle_diameter")->get_at(0);
            vecNozzel.push_back(dNozzleTmp);
        }
    }

    for (int n = 0; n < vecNozzel.size(); n++)
    {
        for (int m = 0; m < nozzleInfo.size(); m++)
        {
            if (std::fabs(std::stof(nozzleInfo[m]) - vecNozzel[n]) < 0.01)
            {
                nozzleInfo.erase(nozzleInfo.begin() + m);
                break;
            }
        }
    }

    wxArrayString nozzle_diameters;
    for (const std::string nozzle : nozzleInfo)
    {
        nozzle_diameters.Add(nozzle + " mm");
    }
    m_nozzle_diameter->Set(nozzle_diameters);
    m_nozzle_diameter->SetSelection(0);

}

wxBoxSizer *CreatePrinterPresetDialog::create_printer_item(wxWindow *parent)
{
    wxBoxSizer *horizontal_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxBoxSizer *  optionSizer        = new wxBoxSizer(wxVERTICAL);
    wxStaticText *static_vendor_text = new wxStaticText(parent, wxID_ANY, _L("Printer"), wxDefaultPosition, wxDefaultSize);
    optionSizer->Add(static_vendor_text, 0, wxEXPAND | wxALL, 0);
    optionSizer->SetMinSize(OPTION_SIZE);
    horizontal_sizer->Add(optionSizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    wxBoxSizer *vertical_sizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *comboBoxSizer = new wxBoxSizer(wxHORIZONTAL);
    m_select_vendor            = new ComboBox(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, NAME_OPTION_COMBOBOX_SIZE, 0, nullptr, wxCB_READONLY);
    m_select_vendor->SetValue(_L("Select Vendor"));
    m_select_vendor->SetLabelColor(DEFAULT_PROMPT_TEXT_COLOUR);
    wxArrayString printer_vendor;
    for (const std::string &vendor : printer_vendors) { 
        printer_vendor.Add(vendor); 
    }
    m_select_vendor->Set(printer_vendor);
    m_select_vendor->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent e) { 
        m_select_vendor->SetLabelColor(*wxBLACK);
        std::string curr_selected_vendor = into_u8(m_select_vendor->GetStringSelection());
        std::unordered_map<std::string,std::vector<std::string>>::const_iterator iter  = printer_model_map.find(curr_selected_vendor);
        if (iter != printer_model_map.end())
        {
            std::vector<std::string> vendor_model = iter->second;
            wxArrayString            model_choice;
            for (const std::string &model : vendor_model) { 
                model_choice.Add(model);
            }
            m_select_model->Set(model_choice);
            if (!model_choice.empty()) { 
                m_select_model->SetSelection(0);
                m_select_model->SetLabelColor(*wxBLACK);
            }
        } else {
            MessageDialog dlg(this, _L("The model is not fond, place reselect vendor."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES | wxYES_DEFAULT | wxCENTRE);
            dlg.ShowModal();
        }
        e.Skip();
    });
    
    comboBoxSizer->Add(m_select_vendor, 0, wxEXPAND | wxALL, 0);

    m_select_model = new ComboBox(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, NAME_OPTION_COMBOBOX_SIZE, 0, nullptr, wxCB_READONLY);
    comboBoxSizer->Add(m_select_model, 0, wxEXPAND | wxLEFT, FromDIP(5));
    m_select_model->SetValue(_L("Select Model"));
    m_select_model->SetLabelColor(DEFAULT_PROMPT_TEXT_COLOUR);
    m_select_model->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent e) {
        m_select_model->SetLabelColor(*wxBLACK);
        e.Skip();
    });

    m_select_printer = new ComboBox(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, PRINTER_PRESET_MODEL_SIZE, 0, nullptr, wxCB_READONLY);
    comboBoxSizer->Add(m_select_printer, 0, wxEXPAND | wxALL, 0);
    m_select_printer->SetValue(_L("Select Printer"));
    m_select_printer->SetLabelColor(DEFAULT_PROMPT_TEXT_COLOUR);
    m_select_printer->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent e) {
        m_select_printer->SetLabelColor(*wxBLACK);
        
        //鏇存柊鏁版嵁 娣诲姞鐨勬暟鎹?
        update_nozzle_data_new();

        e.Skip();
    });
    m_select_printer->Hide();

    m_custom_vendor_text_ctrl                      = new wxTextCtrl(parent, wxID_ANY, "", wxDefaultPosition, NAME_OPTION_COMBOBOX_SIZE);
    m_custom_vendor_text_ctrl->SetHint(_L("Input Custom Vendor"));
    m_custom_vendor_text_ctrl->Bind(wxEVT_CHAR, [this](wxKeyEvent &event) {
        int key = event.GetKeyCode();
        if (cannot_input_key.find(key) != cannot_input_key.end()) { // "@" can not be inputed
            event.Skip(false);
            return;
        }
        event.Skip();
    });
    comboBoxSizer->Add(m_custom_vendor_text_ctrl, 0, wxEXPAND | wxALL, 0);
    m_custom_vendor_text_ctrl->Hide();
    m_custom_model_text_ctrl = new wxTextCtrl(parent, wxID_ANY, "", wxDefaultPosition, NAME_OPTION_COMBOBOX_SIZE);
    m_custom_model_text_ctrl->SetHint(_L("Input Custom Model"));
    m_custom_model_text_ctrl->Bind(wxEVT_CHAR, [this](wxKeyEvent &event) {
        int key = event.GetKeyCode();
        if (cannot_input_key.find(key) != cannot_input_key.end()) { // "@" can not be inputed
            event.Skip(false);
            return;
        }
        event.Skip();
    });
    comboBoxSizer->Add(m_custom_model_text_ctrl, 0, wxEXPAND | wxLEFT, FromDIP(5));
    m_custom_model_text_ctrl->Hide();

    vertical_sizer->Add(comboBoxSizer, 0, wxEXPAND, 0);

    wxBoxSizer *checkbox_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_can_not_find_vendor_combox = new ::CheckBox(parent);

    checkbox_sizer->Add(m_can_not_find_vendor_combox, 0, wxALIGN_CENTER, 0);
    checkbox_sizer->Add(0, 0, 0, wxEXPAND | wxRIGHT, FromDIP(5));

    m_can_not_find_vendor_text = new wxStaticText(parent, wxID_ANY, _L("Can't find my printer model"), wxDefaultPosition, wxDefaultSize, 0);
    m_can_not_find_vendor_text->SetFont(::Label::Body_13);

    wxSize size = m_can_not_find_vendor_text->GetTextExtent(_L("Can't find my printer model"));
    m_can_not_find_vendor_text->SetMinSize(wxSize(size.x + FromDIP(4), -1));
    m_can_not_find_vendor_text->Wrap(-1);
    checkbox_sizer->Add(m_can_not_find_vendor_text, 0, wxALIGN_CENTER, 0);

    m_can_not_find_vendor_combox->Bind(wxEVT_TOGGLEBUTTON, [this](wxCommandEvent &e) {
        bool value = m_can_not_find_vendor_combox->GetValue();
        if (value) {
            m_can_not_find_vendor_combox->SetValue(true);
            m_custom_vendor_text_ctrl->Show();
            m_custom_model_text_ctrl->Show();
            m_select_vendor->Hide();
            m_select_model->Hide();
        } else {
            m_can_not_find_vendor_combox->SetValue(false);
            m_custom_vendor_text_ctrl->Hide();
            m_custom_model_text_ctrl->Hide();
            m_select_vendor->Show();
            m_select_model->Show();
        }
        Refresh();
        Layout();
        m_page1->SetSizerAndFit(m_page1_sizer);
        Fit();
    });

    vertical_sizer->Add(checkbox_sizer, 0, wxEXPAND | wxTOP, FromDIP(5));

    horizontal_sizer->Add(vertical_sizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    return horizontal_sizer;

}

wxBoxSizer *CreatePrinterPresetDialog::create_nozzle_diameter_item(wxWindow *parent)
{
    wxBoxSizer *horizontal_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxBoxSizer *  optionSizer      = new wxBoxSizer(wxVERTICAL);
    wxStaticText *static_type_text = new wxStaticText(parent, wxID_ANY, _L("Nozzle Diameter"), wxDefaultPosition, wxDefaultSize);
    optionSizer->Add(static_type_text, 0, wxEXPAND | wxALL, 0);
    optionSizer->SetMinSize(OPTION_SIZE);
    horizontal_sizer->Add(optionSizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    wxBoxSizer *comboBoxSizer = new wxBoxSizer(wxVERTICAL);
    m_nozzle_diameter         = new ComboBox(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, OPTION_SIZE, 0, nullptr, wxCB_READONLY);
    wxArrayString nozzle_diameters;
    for (const std::string nozzle : nozzle_diameter_vec) { 
        nozzle_diameters.Add(nozzle + " mm");
    }
    m_nozzle_diameter->Set(nozzle_diameters);
    m_nozzle_diameter->SetSelection(0);
    comboBoxSizer->Add(m_nozzle_diameter, 0, wxEXPAND | wxALL, 0);
    horizontal_sizer->Add(comboBoxSizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(10));
    horizontal_sizer->Add(0, 0, 0, wxEXPAND | wxLEFT, FromDIP(200));

    return horizontal_sizer;
}

wxBoxSizer *CreatePrinterPresetDialog::create_bed_shape_item(wxWindow *parent)
{
    wxBoxSizer *horizontal_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxBoxSizer *  optionSizer      = new wxBoxSizer(wxVERTICAL);
    wxStaticText *static_type_text = new wxStaticText(parent, wxID_ANY, _L("Bed Shape"), wxDefaultPosition, wxDefaultSize);
    optionSizer->Add(static_type_text, 0, wxEXPAND | wxALL, 0);
    optionSizer->SetMinSize(OPTION_SIZE);
    horizontal_sizer->Add(optionSizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    wxBoxSizer *  bed_shape_sizer       = new wxBoxSizer(wxVERTICAL);
    wxStaticText *static_bed_shape_text = new wxStaticText(parent, wxID_ANY, _L("Rectangle"), wxDefaultPosition, wxDefaultSize);
    bed_shape_sizer->Add(static_bed_shape_text, 0, wxEXPAND | wxALL, 0);
    horizontal_sizer->Add(bed_shape_sizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    return horizontal_sizer;
}

 wxBoxSizer *CreatePrinterPresetDialog::create_bed_size_item(wxWindow *parent)
 {
     wxBoxSizer *horizontal_sizer = new wxBoxSizer(wxHORIZONTAL);

     wxBoxSizer *  optionSizer      = new wxBoxSizer(wxVERTICAL);
     wxStaticText *static_type_text = new wxStaticText(parent, wxID_ANY, _L("Printable Space"), wxDefaultPosition, wxDefaultSize);
     optionSizer->Add(static_type_text, 0, wxEXPAND | wxALL, 0);
     optionSizer->SetMinSize(OPTION_SIZE);
     horizontal_sizer->Add(optionSizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(10));

     wxBoxSizer *  length_sizer          = new wxBoxSizer(wxVERTICAL);
      // ORCA use icon on input box to match style with other Point fields
     horizontal_sizer->Add(length_sizer, 0, wxEXPAND | wxLEFT | wxTOP | wxALIGN_CENTER_VERTICAL, FromDIP(10));
     wxBoxSizer *length_input_sizer      = new wxBoxSizer(wxVERTICAL);
     m_bed_size_x_input = new TextInput(parent, "200", "mm", "inputbox_x", wxDefaultPosition, PRINTER_SPACE_SIZE, wxTE_PROCESS_ENTER);
     wxTextValidator validator(wxFILTER_DIGITS);
     m_bed_size_x_input->GetTextCtrl()->SetValidator(validator);
     length_input_sizer->Add(m_bed_size_x_input, 0, wxEXPAND | wxLEFT, FromDIP(5));
     horizontal_sizer->Add(length_input_sizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(5));

     wxBoxSizer *  width_sizer      = new wxBoxSizer(wxVERTICAL);
     // ORCA use icon on input box to match style with other Point fields
     horizontal_sizer->Add(width_sizer, 0, wxEXPAND | wxLEFT | wxTOP | wxALIGN_CENTER_VERTICAL, FromDIP(10));
     wxBoxSizer *width_input_sizer      = new wxBoxSizer(wxVERTICAL);
     m_bed_size_y_input            = new TextInput(parent, "200", "mm", "inputbox_y", wxDefaultPosition, PRINTER_SPACE_SIZE, wxTE_PROCESS_ENTER);
     m_bed_size_y_input->GetTextCtrl()->SetValidator(validator);
     width_input_sizer->Add(m_bed_size_y_input, 0, wxEXPAND | wxALL, 0);
     horizontal_sizer->Add(width_input_sizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(5));

     return horizontal_sizer;

 }

wxBoxSizer *CreatePrinterPresetDialog::create_origin_item(wxWindow *parent)
{
    wxBoxSizer *horizontal_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxBoxSizer *  optionSizer      = new wxBoxSizer(wxVERTICAL);
    wxStaticText *static_type_text = new wxStaticText(parent, wxID_ANY, _L("Origin"), wxDefaultPosition, wxDefaultSize);
    optionSizer->Add(static_type_text, 0, wxEXPAND | wxALL, 0);
    optionSizer->SetMinSize(OPTION_SIZE);
    horizontal_sizer->Add(optionSizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    wxBoxSizer *  length_sizer       = new wxBoxSizer(wxVERTICAL);
    // ORCA use icon on input box to match style with other Point fields
    horizontal_sizer->Add(length_sizer, 0, wxEXPAND | wxLEFT | wxTOP | wxALIGN_CENTER_VERTICAL, FromDIP(10));
    wxBoxSizer *length_input_sizer = new wxBoxSizer(wxVERTICAL);
    m_bed_origin_x_input           = new TextInput(parent, "0", "mm", "inputbox_x", wxDefaultPosition, PRINTER_SPACE_SIZE, wxTE_PROCESS_ENTER);
    wxTextValidator validator(wxFILTER_DIGITS);
    m_bed_origin_x_input->GetTextCtrl()->SetValidator(validator);
    length_input_sizer->Add(m_bed_origin_x_input, 0, wxEXPAND | wxLEFT, FromDIP(5)); // Align with other
    horizontal_sizer->Add(length_input_sizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(5));

    wxBoxSizer *  width_sizer       = new wxBoxSizer(wxVERTICAL);
    // ORCA use icon on input box to match style with other Point fields
    horizontal_sizer->Add(width_sizer, 0, wxEXPAND | wxLEFT | wxTOP | wxALIGN_CENTER_VERTICAL, FromDIP(10));
    wxBoxSizer *width_input_sizer = new wxBoxSizer(wxVERTICAL);
    m_bed_origin_y_input          = new TextInput(parent, "0", "mm", "inputbox_y", wxDefaultPosition, PRINTER_SPACE_SIZE, wxTE_PROCESS_ENTER);
    m_bed_origin_y_input->GetTextCtrl()->SetValidator(validator);
    width_input_sizer->Add(m_bed_origin_y_input, 0, wxEXPAND | wxALL, 0);
    horizontal_sizer->Add(width_input_sizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(5));

    return horizontal_sizer;
}

wxBoxSizer *CreatePrinterPresetDialog::create_hot_bed_stl_item(wxWindow *parent)
{
    wxBoxSizer *horizontal_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxBoxSizer *  optionSizer      = new wxBoxSizer(wxVERTICAL);
    wxStaticText *static_type_text = new wxStaticText(parent, wxID_ANY, _L("Hot Bed STL"), wxDefaultPosition, wxDefaultSize);
    optionSizer->Add(static_type_text, 0, wxEXPAND | wxALL, 0);
    optionSizer->SetMinSize(OPTION_SIZE);
    horizontal_sizer->Add(optionSizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    wxBoxSizer *hot_bed_stl_sizer = new wxBoxSizer(wxVERTICAL);

    StateColor flush_bg_col(std::pair<wxColour, int>(wxColour(219, 253, 231), StateColor::Pressed), std::pair<wxColour, int>(wxColour(238, 238, 238), StateColor::Hovered),
                            std::pair<wxColour, int>(wxColour(238, 238, 238), StateColor::Normal));

    StateColor flush_bd_col(std::pair<wxColour, int>(wxColour(0, 150, 136), StateColor::Pressed), std::pair<wxColour, int>(wxColour(0, 150, 136), StateColor::Hovered),
                            std::pair<wxColour, int>(wxColour(172, 172, 172), StateColor::Normal));

    m_button_bed_stl = new Button(parent, _L("Load stl"));
    m_button_bed_stl->Bind(wxEVT_BUTTON, ([this](wxCommandEvent &e) { load_model_stl(); }));
    m_button_bed_stl->SetFont(Label::Body_10);

    m_button_bed_stl->SetPaddingSize(wxSize(FromDIP(30), FromDIP(8)));
    m_button_bed_stl->SetFont(Label::Body_13);
    m_button_bed_stl->SetCornerRadius(FromDIP(8));
    m_button_bed_stl->SetBackgroundColor(flush_bg_col);
    m_button_bed_stl->SetBorderColor(flush_bd_col);
    hot_bed_stl_sizer->Add(m_button_bed_stl, 0, wxEXPAND | wxALL, 0);

    horizontal_sizer->Add(hot_bed_stl_sizer, 0, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    m_upload_stl_tip_text = new wxStaticText(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize);
    m_upload_stl_tip_text->SetLabelText(_L("Empty"));
    horizontal_sizer->Add(m_upload_stl_tip_text, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(10));
    return horizontal_sizer;
}

wxBoxSizer *CreatePrinterPresetDialog::create_hot_bed_svg_item(wxWindow *parent)
{ 
    wxBoxSizer *horizontal_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxBoxSizer *  optionSizer      = new wxBoxSizer(wxVERTICAL);
    wxStaticText *static_type_text = new wxStaticText(parent, wxID_ANY, _L("Hot Bed SVG"), wxDefaultPosition, wxDefaultSize);
    optionSizer->Add(static_type_text, 0, wxEXPAND | wxALL, 0);
    optionSizer->SetMinSize(OPTION_SIZE);
    horizontal_sizer->Add(optionSizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    wxBoxSizer *hot_bed_stl_sizer = new wxBoxSizer(wxVERTICAL);

    StateColor flush_bg_col(std::pair<wxColour, int>(wxColour(219, 253, 231), StateColor::Pressed), std::pair<wxColour, int>(wxColour(238, 238, 238), StateColor::Hovered),
                            std::pair<wxColour, int>(wxColour(238, 238, 238), StateColor::Normal));

    StateColor flush_bd_col(std::pair<wxColour, int>(wxColour(0, 150, 136), StateColor::Pressed), std::pair<wxColour, int>(wxColour(0, 150, 136), StateColor::Hovered),
                            std::pair<wxColour, int>(wxColour(172, 172, 172), StateColor::Normal));

    m_button_bed_svg = new Button(parent, _L("Load svg"));
    m_button_bed_svg->Bind(wxEVT_BUTTON, ([this](wxCommandEvent &e) { load_texture(); }));
    m_button_bed_svg->SetFont(Label::Body_10);

    m_button_bed_svg->SetPaddingSize(wxSize(FromDIP(30), FromDIP(8)));
    m_button_bed_svg->SetFont(Label::Body_13);
    m_button_bed_svg->SetCornerRadius(FromDIP(8));
    m_button_bed_svg->SetBackgroundColor(flush_bg_col);
    m_button_bed_svg->SetBorderColor(flush_bd_col);
    hot_bed_stl_sizer->Add(m_button_bed_svg, 0, wxEXPAND | wxALL, 0);

    horizontal_sizer->Add(hot_bed_stl_sizer, 0, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    m_upload_svg_tip_text = new wxStaticText(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize);
    m_upload_svg_tip_text->SetLabelText(_L("Empty"));
    horizontal_sizer->Add(m_upload_svg_tip_text, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(10));
    return horizontal_sizer;
}

wxBoxSizer *CreatePrinterPresetDialog::create_max_print_height_item(wxWindow *parent)
{
    wxBoxSizer *  horizontal_sizer  = new wxBoxSizer(wxHORIZONTAL);

    wxBoxSizer *  optionSizer      = new wxBoxSizer(wxVERTICAL);
    wxStaticText *static_type_text = new wxStaticText(parent, wxID_ANY, _L("Max Print Height"), wxDefaultPosition, wxDefaultSize);
    optionSizer->Add(static_type_text, 0, wxEXPAND | wxALL, 0);
    optionSizer->SetMinSize(OPTION_SIZE);
    horizontal_sizer->Add(optionSizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    wxBoxSizer *hight_input_sizer = new wxBoxSizer(wxVERTICAL);
    m_print_height_input          = new TextInput(parent, "200", "mm", wxEmptyString, wxDefaultPosition, PRINTER_SPACE_SIZE, wxTE_PROCESS_ENTER); // Use same alignment with all other input boxes
    wxTextValidator validator(wxFILTER_DIGITS);
    m_print_height_input->GetTextCtrl()->SetValidator(validator);
    hight_input_sizer->Add(m_print_height_input, 0, wxEXPAND | wxLEFT, FromDIP(5));
    horizontal_sizer->Add(hight_input_sizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(5));

    return horizontal_sizer;
}

wxBoxSizer *CreatePrinterPresetDialog::create_height_limit(wxWindow *parent)
{
    wxBoxSizer *horizontal_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxBoxSizer *optionSizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText *static_type_text = new wxStaticText(parent, wxID_ANY, _L("Layer height limits"), wxDefaultPosition, wxDefaultSize);
    optionSizer->SetMinSize(OPTION_SIZE);
    optionSizer->Add(static_type_text, 0, wxEXPAND | wxALL, 0);
    horizontal_sizer->Add(optionSizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    wxBoxSizer *input_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxBoxSizer *min_size_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText *minLabel = new wxStaticText(parent, wxID_ANY, _L("Min"), wxDefaultPosition, wxDefaultSize);
    min_size_sizer->Add(minLabel, 0,  wxALIGN_CENTER_VERTICAL, 0);
    m_print_min_height_input = new TextInput(parent, "", "mm", wxEmptyString, wxDefaultPosition, PRINTER_SPACE_SIZE, wxTE_PROCESS_ENTER);
    wxTextValidator validator(wxFILTER_INCLUDE_CHAR_LIST);
    validator.SetCharIncludes("0123456789."); 
    m_print_min_height_input->GetTextCtrl()->SetValidator(validator);
    min_size_sizer->AddSpacer(5); 
    min_size_sizer->Add(m_print_min_height_input, 0, wxEXPAND | wxRIGHT, 0);
    min_size_sizer->AddSpacer(10); 
    input_sizer->Add(min_size_sizer, 0, wxEXPAND | wxALL , 0);

    wxBoxSizer *max_size_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText *maxLabel = new wxStaticText(parent, wxID_ANY, _L("Max"), wxDefaultPosition, wxDefaultSize);
    max_size_sizer->Add(maxLabel, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_print_max_height_input = new TextInput(parent, "", "mm", wxEmptyString, wxDefaultPosition, PRINTER_SPACE_SIZE, wxTE_PROCESS_ENTER);
    m_print_max_height_input->GetTextCtrl()->SetValidator(validator);
    max_size_sizer->AddSpacer(5); 
    max_size_sizer->Add(m_print_max_height_input, 0, wxEXPAND | wxLIGHT, 0);
    input_sizer->Add(max_size_sizer, 0, wxEXPAND | wxALL , 0);

    horizontal_sizer->Add(input_sizer, 0, wxEXPAND | wxALL , FromDIP(10));

    return horizontal_sizer;
}

wxBoxSizer* CreatePrinterPresetDialog::create_displacement_distance(wxWindow *parent)
{
    wxBoxSizer* horizontal_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxStaticText* static_type_text = new wxStaticText(parent, wxID_ANY, _L("Retract"), wxDefaultPosition, wxDefaultSize);
    horizontal_sizer->Add(static_type_text, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, FromDIP(10));

    horizontal_sizer->AddSpacer(85);

    wxBoxSizer* input_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxStaticText* minLabel = new wxStaticText(parent, wxID_ANY, _L("Length"), wxDefaultPosition, wxDefaultSize);
    input_sizer->Add(minLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(5));

    m_print_length_input = new TextInput(parent, "", "mm", wxEmptyString, wxDefaultPosition, PRINTER_SPACE_SIZE, wxTE_PROCESS_ENTER);
    wxTextValidator validator(wxFILTER_DIGITS);
    m_print_length_input->GetTextCtrl()->SetValidator(validator);
    input_sizer->Add(m_print_length_input, 0, wxEXPAND | wxRIGHT, FromDIP(5));

    horizontal_sizer->Add(input_sizer, 1, wxEXPAND | wxALL, 0);

    return horizontal_sizer;
}

wxBoxSizer *CreatePrinterPresetDialog::create_page1_btns_item(wxWindow *parent)
{
    wxBoxSizer *bSizer_button = new wxBoxSizer(wxHORIZONTAL);
    bSizer_button->Add(0, 0, 1, wxEXPAND, 0);

    StateColor btn_bg_green(std::pair<wxColour, int>(wxColour(0, 137, 123), StateColor::Pressed), std::pair<wxColour, int>(wxColour(38, 166, 154), StateColor::Hovered),
                            std::pair<wxColour, int>(wxColour(0, 150, 136), StateColor::Normal));

    m_button_OK = new Button(parent, _L("OK"));
    m_button_OK->SetBackgroundColor(btn_bg_green);
    m_button_OK->SetBorderColor(*wxWHITE);
    m_button_OK->SetTextColor(wxColour(0xFFFFFE));
    m_button_OK->SetFont(Label::Body_12);
    m_button_OK->SetSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_OK->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_OK->SetCornerRadius(FromDIP(12));
    bSizer_button->Add(m_button_OK, 0, wxRIGHT, FromDIP(10));

    m_button_OK->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent &e) {
        if (!validate_input_valid()) return;
        //data_init();
        show_page2();
        });

    StateColor btn_bg_white(std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Pressed), std::pair<wxColour, int>(wxColour(238, 238, 238), StateColor::Hovered),
                            std::pair<wxColour, int>(*wxWHITE, StateColor::Normal));

    m_button_page1_cancel = new Button(parent, _L("Cancel"));
    m_button_page1_cancel->SetBackgroundColor(btn_bg_white);
    m_button_page1_cancel->SetBorderColor(wxColour(38, 46, 48));
    m_button_page1_cancel->SetFont(Label::Body_12);
    m_button_page1_cancel->SetSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_page1_cancel->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_page1_cancel->SetCornerRadius(FromDIP(12));
    bSizer_button->Add(m_button_page1_cancel, 0, wxRIGHT, FromDIP(10));

    m_button_page1_cancel->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent &e) { EndModal(wxID_CANCEL); });

    return bSizer_button;
}
static std::string last_directory = "";
void CreatePrinterPresetDialog::load_texture() {
    wxFileDialog       dialog(this, _L("Choose a file to import bed texture from (PNG/SVG):"), last_directory, "", file_wildcards(FT_TEX), wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (dialog.ShowModal() != wxID_OK)
        return;

    m_custom_texture = "";
    m_upload_svg_tip_text->SetLabelText(_L("Empty"));
    last_directory        = dialog.GetDirectory().ToUTF8().data();
    std::string file_name = dialog.GetPath().ToUTF8().data();
    if (!boost::algorithm::iends_with(file_name, ".png") && !boost::algorithm::iends_with(file_name, ".svg")) {
        show_error(this, _L("Invalid file format."));
        return;
    }
    bool try_ok;
    if (Utils::is_file_too_large(file_name, try_ok)) {
        if (try_ok) {
            m_upload_svg_tip_text->SetLabelText(wxString::Format(_L("The file exceeds %d MB, please import again."), STL_SVG_MAX_FILE_SIZE_MB));
        } else {
            m_upload_svg_tip_text->SetLabelText(_L("Exception in obtaining file size, please import again."));
        }
        return;
    }
    m_custom_texture = file_name;
    wxGCDC dc;
    auto text = wxControl::Ellipsize(_L(boost::filesystem::path(file_name).filename().string()), dc, wxELLIPSIZE_END, FromDIP(200));
    m_upload_svg_tip_text->SetLabelText(text);
}

void CreatePrinterPresetDialog::load_model_stl()
{
    wxFileDialog dialog(this, _L("Choose an STL file to import bed model from:"), last_directory, "", file_wildcards(FT_STL), wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (dialog.ShowModal() != wxID_OK)
        return;

    m_custom_model = "";
    m_upload_stl_tip_text->SetLabelText(_L("Empty"));
    last_directory        = dialog.GetDirectory().ToUTF8().data();
    std::string file_name = dialog.GetPath().ToUTF8().data();
    if (!boost::algorithm::iends_with(file_name, ".stl")) {
        show_error(this, _L("Invalid file format."));
        return;
    }
    bool try_ok;
    if (Utils::is_file_too_large(file_name, try_ok)) {
        if (try_ok) {
            m_upload_stl_tip_text->SetLabelText(wxString::Format(_L("The file exceeds %d MB, please import again."), STL_SVG_MAX_FILE_SIZE_MB));
        }
        else {
            m_upload_stl_tip_text->SetLabelText(_L("Exception in obtaining file size, please import again."));
        }
        return;
    }
    m_custom_model = file_name;
    wxGCDC dc;
    auto text      = wxControl::Ellipsize(_L(boost::filesystem::path(file_name).filename().string()), dc, wxELLIPSIZE_END, FromDIP(200));
    m_upload_stl_tip_text->SetLabelText(text);
}

bool CreatePrinterPresetDialog::load_system_and_user_presets_with_curr_model(PresetBundle &temp_preset_bundle, bool just_template, bool isUseDefault)
{
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " is load template: "<< just_template;
    std::string selected_vendor_id;
    std::string preset_path;
    if (m_printer_preset) {
        delete m_printer_preset;
        m_printer_preset = nullptr;
    }

    //std::string curr_selected_model = into_u8(m_printer_model->GetStringSelection());

    std::string curr_selected_model = isUseDefault ? (into_u8(m_printer_model->GetStringSelection())) : (into_u8(m_printer_model_page2->GetStringSelection()));
    int         nozzle_index        = curr_selected_model.find_first_of("@");
    std::string select_model        = curr_selected_model.substr(0, nozzle_index - 1);
    for (const Slic3r::VendorProfile::PrinterModel &model : m_printer_preset_vendor_selected.models) {
        if (model.name == select_model) {
            m_printer_preset_model_selected = model;
            break;
        }
    }
    if (m_printer_preset_vendor_selected.id.empty() || m_printer_preset_model_selected.id.empty()) {
        BOOST_LOG_TRIVIAL(info) << "selected id is not find";
        MessageDialog dlg(this, _L("Preset path is not find, please reselect vendor."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES_NO | wxYES_DEFAULT | wxCENTRE);
        dlg.ShowModal();
        return false;
    }

    bool is_custom_vendor = false;
    if (PRESET_CUSTOM_VENDOR == m_printer_preset_vendor_selected.name || PRESET_CUSTOM_VENDOR == m_printer_preset_vendor_selected.id) {
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " select custom vendor ";
        is_custom_vendor   = true;
        temp_preset_bundle = *(wxGetApp().preset_bundle);
    } else {
        selected_vendor_id = m_printer_preset_vendor_selected.id;

        if (boost::filesystem::exists(boost::filesystem::path(Slic3r::data_dir()) / PRESET_SYSTEM_DIR / selected_vendor_id)) {
            preset_path = (boost::filesystem::path(Slic3r::data_dir()) / PRESET_SYSTEM_DIR).string();
        } else if (boost::filesystem::exists(boost::filesystem::path(Slic3r::resources_dir()) / "profiles" / selected_vendor_id)) {
            preset_path = (boost::filesystem::path(Slic3r::resources_dir()) / "profiles").string();
        }

        if (preset_path.empty()) {
            BOOST_LOG_TRIVIAL(info) << "Preset path is not find";
            MessageDialog dlg(this, _L("Preset path is not find, please reselect vendor."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"),
                              wxYES_NO | wxYES_DEFAULT | wxCENTRE);
            dlg.ShowModal();
            return false;
        }

        try {
            temp_preset_bundle.load_vendor_configs_from_json(preset_path, selected_vendor_id, PresetBundle::LoadConfigBundleAttribute::LoadSystem,
                                                             ForwardCompatibilitySubstitutionRule::EnableSilent);
        } catch (...) {
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "load vendor fonfigs form json failed";
            MessageDialog dlg(this, _L("The printer model was not found, please reselect."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"),
                              wxYES_NO | wxYES_DEFAULT | wxCENTRE);
            dlg.ShowModal();
            return false;
        }

        if (!just_template) {
            std::string dir_user_presets = wxGetApp().app_config->get("preset_folder");
            if (dir_user_presets.empty()) {
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "default user presets path";
                temp_preset_bundle.load_user_presets(DEFAULT_USER_FOLDER_NAME, ForwardCompatibilitySubstitutionRule::EnableSilent);
            } else {
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "user presets path";
                temp_preset_bundle.load_user_presets(dir_user_presets, ForwardCompatibilitySubstitutionRule::EnableSilent);
            }
        }
    }
    //get model varient
    //std::string model_varient = into_u8(m_printer_model->GetStringSelection());
    std::string model_varient = isUseDefault ? (into_u8(m_printer_model->GetStringSelection())) : (into_u8(m_printer_model_page2->GetStringSelection()));
    size_t      index_at      = model_varient.find(" @ ");
    size_t      index_nozzle  = model_varient.find("nozzle");
    std::string varient;
    if (index_at != std::string::npos && index_nozzle != std::string::npos) {
        varient = model_varient.substr(index_at + 3, index_nozzle - index_at - 4);
    } else {
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "get nozzle failed";
        MessageDialog dlg(this, _L("The nozzle diameter is not fond, place reselect."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES_NO | wxYES_DEFAULT | wxCENTRE);
        dlg.ShowModal();
        return false;
    }

    const Preset *temp_printer_preset = is_custom_vendor ? temp_preset_bundle.printers.find_custom_preset_by_model_and_variant(m_printer_preset_model_selected.id, varient) :
                                                           temp_preset_bundle.printers.find_system_preset_by_model_and_variant(m_printer_preset_model_selected.id, varient);

    if (temp_printer_preset) {
        m_printer_preset = new Preset(*temp_printer_preset);
    } else {
        MessageDialog dlg(this, _L("The printer preset is not fond, place reselect."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES_NO | wxYES_DEFAULT | wxCENTRE);
        dlg.ShowModal();
        return false;
    }

    if (!just_template) {
        temp_preset_bundle.printers.select_preset_by_name(m_printer_preset->name, true);
        temp_preset_bundle.update_compatible(PresetSelectCompatibleType::Always);
    } else {
        selected_vendor_id = PRESET_TEMPLATE_DIR;
        preset_path.clear();
        if (boost::filesystem::exists(boost::filesystem::path(Slic3r::resources_dir()) / PRESET_PROFILES_TEMOLATE_DIR / selected_vendor_id)) {
            preset_path = (boost::filesystem::path(Slic3r::resources_dir()) / PRESET_PROFILES_TEMOLATE_DIR).string();
        }
        if (preset_path.empty()) {
            BOOST_LOG_TRIVIAL(info) << "Preset path is not find";
            MessageDialog dlg(this, _L("Preset path is not find, please reselect vendor."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"),
                              wxYES_NO | wxYES_DEFAULT | wxCENTRE);
            dlg.ShowModal();
            return false;
        }
        try {
            temp_preset_bundle.load_vendor_configs_from_json(preset_path, selected_vendor_id, PresetBundle::LoadConfigBundleAttribute::LoadSystem,
                                                             ForwardCompatibilitySubstitutionRule::EnableSilent);
        } catch (...) {
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "load template vendor configs form json failed";
            MessageDialog dlg(this, _L("The printer model was not found, please reselect."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"),
                              wxYES_NO | wxYES_DEFAULT | wxCENTRE);
            dlg.ShowModal();
            return false;
        }
    }

    return true;
}

void CreatePrinterPresetDialog::generate_process_presets_data(std::vector<Preset const *> presets, std::string nozzle)
{
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " entry, and nozzle is: " << nozzle;
    std::unordered_map<std::string, float> nozzle_diameter_map_ = nozzle_diameter_map;
    for (const Preset *preset : presets) {
        float nozzle_dia = nozzle_diameter_map_[nozzle];
        assert(nozzle_dia != 0);

        auto layer_height = dynamic_cast<ConfigOptionFloat *>(const_cast<Preset *>(preset)->config.option("layer_height", true));
        if (layer_height)
            layer_height->value = nozzle_dia / 2;
        else
            BOOST_LOG_TRIVIAL(info) << "process template has no layer_height";

        auto initial_layer_print_height = dynamic_cast<ConfigOptionFloat *>(const_cast<Preset *>(preset)->config.option("initial_layer_print_height", true));
        if (initial_layer_print_height)
            initial_layer_print_height->value = nozzle_dia / 2;
        else
            BOOST_LOG_TRIVIAL(info) << "process template has no initial_layer_print_height";
       
        auto line_width = dynamic_cast<ConfigOptionFloat *>(const_cast<Preset *>(preset)->config.option("line_width", true));
        if (line_width)
            line_width->value = nozzle_dia;
        else
            BOOST_LOG_TRIVIAL(info) << "process template has no line_width";

        auto initial_layer_line_width = dynamic_cast<ConfigOptionFloat *>(const_cast<Preset *>(preset)->config.option("initial_layer_line_width", true));
        if (initial_layer_line_width)
            initial_layer_line_width->value = nozzle_dia;
        else
            BOOST_LOG_TRIVIAL(info) << "process template has no initial_layer_line_width";

        auto outer_wall_line_width = dynamic_cast<ConfigOptionFloat *>(const_cast<Preset *>(preset)->config.option("outer_wall_line_width", true));
        if (outer_wall_line_width)
            outer_wall_line_width->value = nozzle_dia;
        else
            BOOST_LOG_TRIVIAL(info) << "process template has no outer_wall_line_width";

        auto inner_wall_line_width = dynamic_cast<ConfigOptionFloat *>(const_cast<Preset *>(preset)->config.option("inner_wall_line_width", true));
        if (inner_wall_line_width)
            inner_wall_line_width->value = nozzle_dia;
        else
            BOOST_LOG_TRIVIAL(info) << "process template has no inner_wall_line_width";

        auto top_surface_line_width = dynamic_cast<ConfigOptionFloat *>(const_cast<Preset *>(preset)->config.option("top_surface_line_width", true));
        if (top_surface_line_width)
            top_surface_line_width->value = nozzle_dia;
        else
            BOOST_LOG_TRIVIAL(info) << "process template has no top_surface_line_width";

        auto sparse_infill_line_width = dynamic_cast<ConfigOptionFloat *>(const_cast<Preset *>(preset)->config.option("sparse_infill_line_width", true));
        if (sparse_infill_line_width)
            sparse_infill_line_width->value = nozzle_dia;
        else
            BOOST_LOG_TRIVIAL(info) << "process template has no sparse_infill_line_width";

        auto internal_solid_infill_line_width = dynamic_cast<ConfigOptionFloat *>(const_cast<Preset *>(preset)->config.option("internal_solid_infill_line_width", true));
        if (internal_solid_infill_line_width)
            internal_solid_infill_line_width->value = nozzle_dia;
        else
            BOOST_LOG_TRIVIAL(info) << "process template has no internal_solid_infill_line_width";

        auto support_line_width = dynamic_cast<ConfigOptionFloat *>(const_cast<Preset *>(preset)->config.option("support_line_width", true));
        if (support_line_width)
            support_line_width->value = nozzle_dia;
        else
            BOOST_LOG_TRIVIAL(info) << "process template has no support_line_width";

        auto wall_loops = dynamic_cast<ConfigOptionInt *>(const_cast<Preset *>(preset)->config.option("wall_loops", true));
        if (wall_loops)
            wall_loops->value = std::max(2, (int) std::ceil(2 * 0.4 / nozzle_dia));
        else
            BOOST_LOG_TRIVIAL(info) << "process template has no wall_loops";

        auto top_shell_layers = dynamic_cast<ConfigOptionInt *>(const_cast<Preset *>(preset)->config.option("top_shell_layers", true));
        if (top_shell_layers)
            top_shell_layers->value = std::max(5, (int) std::ceil(5 * 0.4 / nozzle_dia));
        else
            BOOST_LOG_TRIVIAL(info) << "process template has no top_shell_layers";

        auto bottom_shell_layers = dynamic_cast<ConfigOptionInt *>(const_cast<Preset *>(preset)->config.option("bottom_shell_layers", true));
        if (bottom_shell_layers)
            bottom_shell_layers->value = std::max(3, (int) std::ceil(3 * 0.4 / nozzle_dia));
        else
            BOOST_LOG_TRIVIAL(info) << "process template has no bottom_shell_layers";
    }
}

void CreatePrinterPresetDialog::update_preset_list_size()
{
    m_scrolled_preset_window->Freeze();
    m_preset_template_panel->SetSizerAndFit(m_filament_sizer);
    m_preset_template_panel->SetMinSize(wxSize(FromDIP(660), -1));
    m_preset_template_panel->SetSize(wxSize(FromDIP(660), -1));
    int width = m_preset_template_panel->GetSize().GetWidth();
    int height = m_preset_template_panel->GetSize().GetHeight();
    m_scrolled_preset_window->SetMinSize(wxSize(std::min(1500, width + FromDIP(26)), std::min(600, height)));
    m_scrolled_preset_window->SetMaxSize(wxSize(std::min(1500, width + FromDIP(26)), std::min(600, height)));
    m_scrolled_preset_window->SetSize(wxSize(std::min(1500, width + FromDIP(26)), std::min(600, height)));
    m_page2->SetSizerAndFit(m_page2_sizer);
    Layout();
    Fit();
    Refresh();
    adjust_dialog_in_screen(this);
    m_scrolled_preset_window->Thaw();
}

wxBoxSizer *CreatePrinterPresetDialog::create_radio_item(wxString title, wxWindow *parent, wxString tooltip, std::vector<std::pair<RadioBox *, wxString>> &radiobox_list)
{
    wxBoxSizer *horizontal_sizer = new wxBoxSizer(wxHORIZONTAL);
    RadioBox *  radiobox         = new RadioBox(parent);
    horizontal_sizer->Add(radiobox, 0, wxEXPAND | wxALL, 0);
    horizontal_sizer->Add(0, 0, 0, wxEXPAND | wxLEFT, FromDIP(5));
    radiobox_list.push_back(std::make_pair(radiobox,title));
    int btn_idx = radiobox_list.size() - 1;
    radiobox->Bind(wxEVT_LEFT_DOWN, [this, &radiobox_list, btn_idx](wxMouseEvent &e) {
        select_curr_radiobox(radiobox_list, btn_idx);
    });

    if(title == m_create_type.base_curr_printer)
    {
        m_current_printer_text = new wxStaticText(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize);
        m_current_printer_text->Bind(wxEVT_LEFT_DOWN, [this, &radiobox_list, btn_idx](wxMouseEvent& e) {
            select_curr_radiobox(radiobox_list, btn_idx); });
        horizontal_sizer->Add(m_current_printer_text, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
        m_current_printer_text->SetToolTip(tooltip);
    }
    else
    {
        wxStaticText *text = new wxStaticText(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize);
        text->Bind(wxEVT_LEFT_DOWN, [this, &radiobox_list, btn_idx](wxMouseEvent& e) {
            select_curr_radiobox(radiobox_list, btn_idx);});
        horizontal_sizer->Add(text, 0, wxEXPAND | wxLEFT, 0);
        text->SetToolTip(tooltip);
    }

    radiobox->SetToolTip(tooltip);

    return horizontal_sizer;
}

wxBoxSizer* CreatePrinterPresetDialog::create_previous_page_combox(wxWindow *parent)
{
    wxBoxSizer* comboBox_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_printer_vendor_page2 = new ComboBox(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, PRINTER_PRESET_VENDOR_SIZE, 0, nullptr, wxCB_READONLY);
    m_printer_vendor_page2->SetValue(_L("Select Vendor"));
    m_printer_vendor_page2->SetLabelColor(DEFAULT_PROMPT_TEXT_COLOUR);

    m_printer_model_page2 = new ComboBox(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, PRINTER_PRESET_MODEL_SIZE, 0, nullptr, wxCB_READONLY);
    m_printer_model_page2->SetLabelColor(DEFAULT_PROMPT_TEXT_COLOUR);
    m_printer_model_page2->SetValue(_L("Select Model"));

    comboBox_sizer->Add(m_printer_vendor_page2, 0, wxEXPAND, 0);
    comboBox_sizer->Add(m_printer_model_page2, 0, wxEXPAND | wxLEFT, FromDIP(10));
    return comboBox_sizer;
}

void CreatePrinterPresetDialog::select_curr_radiobox(std::vector<std::pair<RadioBox *, wxString>> &radiobox_list, int btn_idx)
{
    int len = radiobox_list.size();
    for (int i = 0; i < len; ++i) {
        if (i == btn_idx) {
            radiobox_list[i].first->SetValue(true);
            wxString curr_selected_type = radiobox_list[i].second;
            this->Freeze();
            if (curr_selected_type == m_create_type.base_template) {
                if (m_printer_model->GetValue() == _L("Select Model")) {
                    m_filament_preset_template_sizer->Clear(true);
                    m_filament_preset.clear();
                    m_process_preset_template_sizer->Clear(true);
                    m_process_preset.clear();
                } else {
                    (m_type == 1) ? (update_presets_list(true,false)) : (update_presets_list(true,true));
                }
                if (m_type == 1)
                {
                    m_printer_vendor_page2->Enable(false);
                    m_printer_model_page2->Enable(false);
                }
                m_page2->SetSizerAndFit(m_page2_sizer);
            } else if (curr_selected_type == m_create_type.base_curr_printer) {
                if (m_printer_model->GetValue() == _L("Select Model")) {
                    m_filament_preset_template_sizer->Clear(true);
                    m_filament_preset.clear();
                    m_process_preset_template_sizer->Clear(true);
                    m_process_preset.clear();
                } else {
                    (m_type == 1) ? (update_presets_list(false,false)) : (update_presets_list(false,true));
                }
                if (m_type == 1)
                {
                    m_printer_vendor_page2->Enable(true);
                    m_printer_model_page2->Enable(true);
                }
                m_page2->SetSizerAndFit(m_page2_sizer);
            } else if (curr_selected_type == m_create_type.create_printer) {
                m_select_printer->Hide();
                m_printer_info_pane_nozzle->Hide();
                m_can_not_find_vendor_combox->Show();
                m_can_not_find_vendor_text->Show();
                m_printer_info_panel->Show();
                if (m_can_not_find_vendor_combox->GetValue()) { 
                    m_custom_vendor_text_ctrl->Show();
                    m_custom_model_text_ctrl->Show();
                    m_select_vendor->Hide();
                    m_select_model->Hide();
                } else {
                    m_select_vendor->Show();
                    m_select_model->Show();
                }
                m_page1->SetSizerAndFit(m_page1_sizer);
            } else if (curr_selected_type == m_create_type.create_nozzle) {
                //set_current_visible_printer();
                set_current_visible_printer_new();
                m_select_vendor->Hide();
                m_select_model->Hide();
                m_can_not_find_vendor_combox->Hide();
                m_can_not_find_vendor_text->Hide();
                m_custom_vendor_text_ctrl->Hide();
                m_custom_model_text_ctrl->Hide();
                m_printer_info_panel->Hide();
                m_printer_info_pane_nozzle->Show();
                m_select_printer->Show();
                m_page1->SetSizerAndFit(m_page1_sizer);
            }
            this->Thaw();
        } else {
            radiobox_list[i].first->SetValue(false);
        }
    }
    
    update_preset_list_size();
}

void CreatePrinterPresetDialog::create_printer_page2(wxWindow *parent)
{
    this->SetBackgroundColour(*wxWHITE);

    m_page2_sizer = new wxBoxSizer(wxVERTICAL);

    //m_page2_sizer->Add(create_printer_preset_item(parent), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));
    m_page2_sizer->Add(create_presets_item(parent), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));
    m_page2_sizer->Add(create_presets_template_item(parent), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));
    m_page2_sizer->Add(create_page2_btns_item(parent), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));

    parent->SetSizerAndFit(m_page2_sizer);
    Layout();
    Fit();

    wxGetApp().UpdateDlgDarkUI(this);
}

wxBoxSizer *CreatePrinterPresetDialog::create_printer_preset_item(wxWindow *parent)
{
    wxBoxSizer *horizontal_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxBoxSizer *  optionSizer        = new wxBoxSizer(wxVERTICAL);
    wxStaticText *static_vendor_text = new wxStaticText(parent, wxID_ANY, _L("Reference Template"), wxDefaultPosition, wxDefaultSize);
    optionSizer->Add(static_vendor_text, 0, wxEXPAND | wxALL, 0);
    optionSizer->SetMinSize(OPTION_SIZE);
    horizontal_sizer->Add(optionSizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    wxBoxSizer *  vertical_sizer = new wxBoxSizer(wxVERTICAL);
    // wxStaticText *combobox_title = new wxStaticText(parent, wxID_ANY, m_create_type.base_curr_printer, wxDefaultPosition, wxDefaultSize, 0);
    // combobox_title->SetFont(::Label::Body_13);
    // auto size = combobox_title->GetTextExtent(m_create_type.base_curr_printer);
    // combobox_title->SetMinSize(wxSize(size.x + FromDIP(4), -1));
    // combobox_title->Wrap(-1);
    // vertical_sizer->Add(combobox_title, 0, wxEXPAND | wxALL, 0);

    wxBoxSizer *comboBox_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_printer_vendor           = new ComboBox(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, PRINTER_PRESET_VENDOR_SIZE, 0, nullptr, wxCB_READONLY);
    m_printer_vendor->SetValue(_L("Select Vendor"));
    m_printer_vendor->SetLabelColor(DEFAULT_PROMPT_TEXT_COLOUR);
    comboBox_sizer->Add(m_printer_vendor, 0, wxEXPAND, 0);
    m_printer_model = new ComboBox(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, PRINTER_PRESET_MODEL_SIZE, 0, nullptr, wxCB_READONLY);
    m_printer_model->SetLabelColor(DEFAULT_PROMPT_TEXT_COLOUR);
    m_printer_model->SetValue(_L("Select Model"));

    comboBox_sizer->Add(m_printer_model, 0, wxEXPAND | wxLEFT, FromDIP(10));
    vertical_sizer->Add(comboBox_sizer, 0, wxEXPAND | wxTOP, 0);

    horizontal_sizer->Add(vertical_sizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    return horizontal_sizer;

}

wxBoxSizer *CreatePrinterPresetDialog::create_presets_item(wxWindow *parent)
{
    wxBoxSizer *horizontal_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxBoxSizer *  optionSizer        = new wxBoxSizer(wxVERTICAL);
    wxStaticText *static_serial_text = new wxStaticText(parent, wxID_ANY, _L("Presets"), wxDefaultPosition, wxDefaultSize);
    optionSizer->Add(static_serial_text, 0, wxEXPAND | wxALL, 0);
    optionSizer->SetMinSize(OPTION_SIZE);
    horizontal_sizer->Add(optionSizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    wxBoxSizer *radioBoxSizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer *radioBoxAndCombox = new wxBoxSizer(wxHORIZONTAL);
    radioBoxAndCombox->Add(create_radio_item(m_create_type.base_curr_printer, parent, wxEmptyString, m_create_presets_btns), 0, wxEXPAND | wxALL, 0);
    if (m_type == 1)
    {
        radioBoxAndCombox->Add(create_previous_page_combox(parent), 0, wxEXPAND | wxALL, 0);
    }
    radioBoxSizer->Add(radioBoxAndCombox, 0, wxEXPAND | wxALL, 0);
    radioBoxSizer->Add(create_radio_item(m_create_type.base_template, parent, wxEmptyString, m_create_presets_btns), 0, wxEXPAND | wxTOP, FromDIP(10));

    horizontal_sizer->Add(radioBoxSizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    return horizontal_sizer;
}

wxBoxSizer *CreatePrinterPresetDialog::create_presets_template_item(wxWindow *parent)
{
    wxBoxSizer *vertical_sizer = new wxBoxSizer(wxVERTICAL);

    m_scrolled_preset_window = new wxScrolledWindow(parent);
    m_scrolled_preset_window->SetScrollRate(5, 5);
    m_scrolled_preset_window->SetBackgroundColour(*wxWHITE);
    //m_scrolled_preset_window->SetMinSize(wxSize(FromDIP(1500), FromDIP(-1)));
    m_scrolled_preset_window->SetMaxSize(wxSize(FromDIP(1500), FromDIP(-1)));
    m_scrolled_preset_window->SetSize(wxSize(FromDIP(1500), FromDIP(-1)));
    m_scrooled_preset_sizer = new wxBoxSizer(wxHORIZONTAL);

    m_preset_template_panel = new wxPanel(m_scrolled_preset_window);
    m_preset_template_panel->SetSize(wxSize(-1, -1));
    m_preset_template_panel->SetBackgroundColour(PRINTER_LIST_COLOUR);
    m_preset_template_panel->SetMinSize(wxSize(FromDIP(660), -1));
    m_filament_sizer              = new wxBoxSizer(wxVERTICAL);
    wxStaticText *static_filament_preset_text = new wxStaticText(m_preset_template_panel, wxID_ANY, _L("Filament Preset Template"), wxDefaultPosition, wxDefaultSize);
    m_filament_sizer->Add(static_filament_preset_text, 0, wxEXPAND | wxALL, FromDIP(5));
    m_filament_preset_panel          = new wxPanel(m_preset_template_panel);
    m_filament_preset_template_sizer = new wxGridSizer(3, FromDIP(5), FromDIP(5));
    m_filament_preset_panel->SetSize(PRESET_TEMPLATE_SIZE);
    m_filament_preset_panel->SetSizer(m_filament_preset_template_sizer);
    m_filament_sizer->Add(m_filament_preset_panel, 0, wxEXPAND | wxALL, FromDIP(5));
    
    wxBoxSizer *hori_filament_btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxPanel *   filament_btn_panel      = new wxPanel(m_preset_template_panel);
    filament_btn_panel->SetBackgroundColour(FILAMENT_OPTION_COLOUR);
    wxStaticText *filament_sel_all_text = new wxStaticText(filament_btn_panel, wxID_ANY, _L("Select All"), wxDefaultPosition, wxDefaultSize);
    filament_sel_all_text->SetForegroundColour(SELECT_ALL_OPTION_COLOUR);
    filament_sel_all_text->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent &e) { 
        select_all_preset_template(m_filament_preset); 
        e.Skip();
        });
    wxStaticText *filament_desel_all_text = new wxStaticText(filament_btn_panel, wxID_ANY, _L("Deselect All"), wxDefaultPosition, wxDefaultSize);
    filament_desel_all_text->SetForegroundColour(SELECT_ALL_OPTION_COLOUR);
    filament_desel_all_text->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent &e) {
        deselect_all_preset_template(m_filament_preset);
        e.Skip();
    });
    hori_filament_btn_sizer->Add(filament_sel_all_text, 0, wxEXPAND | wxALL, FromDIP(5));
    hori_filament_btn_sizer->Add(filament_desel_all_text, 0, wxEXPAND | wxALL, FromDIP(5));
    filament_btn_panel->SetSizer(hori_filament_btn_sizer);
    m_filament_sizer->Add(filament_btn_panel, 0, wxEXPAND, 0);
    
    wxPanel *split_panel = new wxPanel(m_preset_template_panel, wxID_ANY, wxDefaultPosition, wxSize(-1, FromDIP(10)));
    split_panel->SetBackgroundColour(wxColour(*wxWHITE));
    m_filament_sizer->Add(split_panel, 0, wxEXPAND, 0);

    wxStaticText *static_process_preset_text = new wxStaticText(m_preset_template_panel, wxID_ANY, _L("Process Preset Template"), wxDefaultPosition, wxDefaultSize);
    m_filament_sizer->Add(static_process_preset_text, 0, wxEXPAND | wxALL, FromDIP(5));
    m_process_preset_panel = new wxPanel(m_preset_template_panel);
    m_process_preset_panel->SetSize(PRESET_TEMPLATE_SIZE);
    m_process_preset_template_sizer = new wxGridSizer(3, FromDIP(5), FromDIP(5));
    m_process_preset_panel->SetSizer(m_process_preset_template_sizer);
    m_filament_sizer->Add(m_process_preset_panel, 0, wxEXPAND | wxALL, FromDIP(5));
    

    wxBoxSizer *hori_process_btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxPanel *   process_btn_panel      = new wxPanel(m_preset_template_panel);
    process_btn_panel->SetBackgroundColour(FILAMENT_OPTION_COLOUR);
    wxStaticText *process_sel_all_text = new wxStaticText(process_btn_panel, wxID_ANY, _L("Select All"), wxDefaultPosition, wxDefaultSize);
    process_sel_all_text->SetForegroundColour(SELECT_ALL_OPTION_COLOUR);
    process_sel_all_text->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent &e) {
        select_all_preset_template(m_process_preset);
        e.Skip();
    });
    wxStaticText *process_desel_all_text = new wxStaticText(process_btn_panel, wxID_ANY, _L("Deselect All"), wxDefaultPosition, wxDefaultSize);
    process_desel_all_text->SetForegroundColour(SELECT_ALL_OPTION_COLOUR);
    process_desel_all_text->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent &e) {
        deselect_all_preset_template(m_process_preset);
        e.Skip();
    });
    hori_process_btn_sizer->Add(process_sel_all_text, 0, wxEXPAND | wxALL, FromDIP(5));
    hori_process_btn_sizer->Add(process_desel_all_text, 0, wxEXPAND | wxALL, FromDIP(5));
    process_btn_panel->SetSizer(hori_process_btn_sizer);
    m_filament_sizer->Add(process_btn_panel, 0, wxEXPAND, 0);

    m_preset_template_panel->SetSizer(m_filament_sizer);
    m_scrooled_preset_sizer->Add(m_preset_template_panel, 0, wxEXPAND | wxALL, 0);
    m_scrolled_preset_window->SetSizerAndFit(m_scrooled_preset_sizer);
    vertical_sizer->Add(m_scrolled_preset_window, 0, wxEXPAND | wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    return vertical_sizer;
}

wxBoxSizer *CreatePrinterPresetDialog::create_page2_btns_item(wxWindow *parent)
{
    wxBoxSizer *bSizer_button = new wxBoxSizer(wxHORIZONTAL);
    bSizer_button->Add(0, 0, 1, wxEXPAND, 0);

    StateColor btn_bg_green(std::pair<wxColour, int>(wxColour(0, 137, 123), StateColor::Pressed), std::pair<wxColour, int>(wxColour(38, 166, 154), StateColor::Hovered),
                            std::pair<wxColour, int>(wxColour(0, 150, 136), StateColor::Normal));

    StateColor btn_bg_white(std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Pressed), std::pair<wxColour, int>(wxColour(238, 238, 238), StateColor::Hovered),
                            std::pair<wxColour, int>(*wxWHITE, StateColor::Normal));

    m_button_page2_back = new Button(parent, _L("Back Page 1"));
    m_button_page2_back->SetBackgroundColor(btn_bg_white);
    m_button_page2_back->SetBorderColor(wxColour(38, 46, 48));
    m_button_page2_back->SetFont(Label::Body_12);
    m_button_page2_back->SetSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_page2_back->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_page2_back->SetCornerRadius(FromDIP(12));
    bSizer_button->Add(m_button_page2_back, 0, wxRIGHT, FromDIP(10));

    m_button_page2_back->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent &e) { show_page1(); });

    m_button_create = new Button(parent, _L("Create"));
    m_button_create->SetBackgroundColor(btn_bg_green);
    m_button_create->SetBorderColor(*wxWHITE);
    m_button_create->SetTextColor(wxColour(0xFFFFFE));
    m_button_create->SetFont(Label::Body_12);
    m_button_create->SetSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_create->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_create->SetCornerRadius(FromDIP(12));
    bSizer_button->Add(m_button_create, 0, wxRIGHT, FromDIP(10));

    m_button_create->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent &e) {

        PresetBundle *preset_bundle = wxGetApp().preset_bundle;
        const wxString curr_selected_printer_type = curr_create_printer_type();
        const wxString curr_selected_preset_type  = curr_create_preset_type();

        // Confirm if the printer preset exists
        if (!m_printer_preset) {
            MessageDialog dlg(this, _L("You have not yet chosen which printer preset to create based on. Please choose the vendor and model of the printer"),
                              wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES | wxYES_DEFAULT | wxCENTRE);
            dlg.ShowModal();
            return;
        }

        if (!save_printable_area_config(m_printer_preset)) {
            MessageDialog dlg(this, _L("You have entered an illegal input in the printable area section on the first page. Please check before creating it."),
                              wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES | wxYES_DEFAULT | wxCENTRE);
            dlg.ShowModal();
            show_page1();
            return;
        }

        // create preset name
        std::string printer_preset_name;
        std::string printer_model_name;
        std::string printer_nozzle_name;
        std::string nozzle_diameter = into_u8(m_nozzle_diameter->GetStringSelection());
        size_t      index_mm        = nozzle_diameter.find("mm");
        if (std::string::npos != index_mm) {
            nozzle_diameter.replace(index_mm, 2, "nozzle");
        }
        if (curr_selected_printer_type == m_create_type.create_printer) {
            if (m_can_not_find_vendor_combox->GetValue()) {
                std::string custom_vendor = into_u8(m_custom_vendor_text_ctrl->GetValue());
                std::string custom_model  = into_u8(m_custom_model_text_ctrl->GetValue());
                if (custom_vendor.empty() || custom_model.empty()) {
                    MessageDialog dlg(this, _L("The custom printer or model is not inputed, place input."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"),
                                      wxYES | wxYES_DEFAULT | wxCENTRE);
                    dlg.ShowModal();
                    show_page1();
                    return;
                }
                custom_vendor = remove_special_key(custom_vendor);
                custom_model  = remove_special_key(custom_model);
                boost::algorithm::trim(custom_vendor);
                boost::algorithm::trim(custom_model);

                printer_preset_name = custom_vendor + " " + custom_model + " " + nozzle_diameter;
                printer_model_name  = custom_vendor + " " + custom_model;
            } else {
                std::string vender_name = into_u8(m_select_vendor->GetStringSelection());
                std::string model_name  = into_u8(m_select_model->GetStringSelection());
                printer_preset_name     = vender_name + " " + model_name + " " + nozzle_diameter;
                printer_model_name      = vender_name + " " + model_name;
                
            }
        } else if (curr_selected_printer_type == m_create_type.create_nozzle) 
        {
            printer_model_name = into_u8(m_select_printer->GetStringSelection());
            printer_preset_name = printer_model_name + " " + nozzle_diameter;

            //std::string selected_printer_preset_name = into_u8(m_select_printer->GetStringSelection());
            //std::unordered_map<std::string, std::shared_ptr<Preset>>::iterator itor = m_printer_name_to_preset.find(selected_printer_preset_name);
            //assert(m_printer_name_to_preset.end() != itor);
            //if (m_printer_name_to_preset.end() != itor)
            //{
            //    std::shared_ptr<Preset> printer_preset = itor->second;
            //    try
            //    {
            //        printer_model_name = printer_preset->config.opt_string("printer_model", true);
            //        printer_preset_name = printer_model_name + " " + nozzle_diameter;
            //    }
            //    catch (...)
            //    {
            //        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " get config printer_model or , and the name is: " << selected_printer_preset_name;
            //    }
            //}
            //else
            //{
            //    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " don't get printer preset, and the name is: " << selected_printer_preset_name;
            //}
        }
        printer_nozzle_name = nozzle_diameter.substr(0, nozzle_diameter.find(" nozzle"));

        // Confirm if the printer preset has a duplicate name
        if (!rewritten && preset_bundle->printers.find_preset(printer_preset_name)) {
            MessageDialog dlg(this,
                              _L("The printer preset you created already has a preset with the same name. Do you want to overwrite it?\n\tYes: Overwrite the printer preset with the "
                                 "same name, and filament and process presets with the same preset name will be recreated \nand filament and process presets without the same preset name will be reserve.\n\tCancel: Do not create a preset, return to the "
                                 "creation interface."),
                              wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES | wxCANCEL | wxYES_DEFAULT | wxCENTRE);
            int           res = dlg.ShowModal();
            if (res == wxID_YES) {
                rewritten = true;
            } else {
                return;
            }
        }

        // Confirm if the filament preset is exist
        bool                        filament_preset_is_exist = false;
        std::vector<Preset const *> selected_filament_presets;
        for (std::pair<::CheckBox *, Preset const *> filament_preset : m_filament_preset) {
            if (filament_preset.first->GetValue()) { selected_filament_presets.push_back(filament_preset.second); }
            if (!filament_preset_is_exist && preset_bundle->filaments.find_preset(filament_preset.second->alias + " @ " + printer_preset_name) != nullptr) {
                filament_preset_is_exist = true;
            }
        }
        if (selected_filament_presets.empty() && !filament_preset_is_exist) {
            MessageDialog dlg(this, _L("You need to select at least one filament preset."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES | wxYES_DEFAULT | wxCENTRE);
            dlg.ShowModal();
            return;
        }

        // Confirm if the process preset is exist
        bool                        process_preset_is_exist = false;
        std::vector<Preset const *> selected_process_presets;
        for (std::pair<::CheckBox *, Preset const *> process_preset : m_process_preset) {
            if (process_preset.first->GetValue()) { selected_process_presets.push_back(process_preset.second); }
            if (!process_preset_is_exist && preset_bundle->prints.find_preset(process_preset.second->alias + " @" + printer_preset_name) != nullptr) {
                process_preset_is_exist = true;
            }
        }
        if (selected_process_presets.empty() && !process_preset_is_exist) {
            MessageDialog dlg(this, _L("You need to select at least one process preset."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES | wxYES_DEFAULT | wxCENTRE);
            dlg.ShowModal();
            return;
        }

        std::vector<std::string> successful_preset_names;
        if (curr_selected_preset_type == m_create_type.base_template) {
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " base template";
            /******************************   clone filament preset    ********************************/
            std::vector<std::string> failures;
            if (!selected_filament_presets.empty()) {
                bool create_preset_result = preset_bundle->filaments.clone_presets_for_printer(selected_filament_presets, failures, printer_preset_name, get_filament_id, rewritten);
                if (!create_preset_result) {
                    std::string message;
                    for (const std::string &failure : failures) { message += "\t" + failure + "\n"; }
                    MessageDialog dlg(this, _L("Create filament presets failed. As follows:\n") + from_u8(message) + _L("\nDo you want to rewrite it?"),
                                      wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"),
                                      wxYES | wxYES_DEFAULT | wxCENTRE);
                    int res = dlg.ShowModal();
                    if (wxID_YES == res) {
                        create_preset_result = preset_bundle->filaments.clone_presets_for_printer(selected_filament_presets, failures, printer_preset_name,
                                                                                                              get_filament_id, true);
                    } else {
                        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " printer preset no same preset but filament has same preset, user cancel create the printer preset";
                        return;
                    }
                }
                // save created successfully preset name
                for (Preset const *sucessful_preset : selected_filament_presets)
                    successful_preset_names.push_back(sucessful_preset->name.substr(0, sucessful_preset->name.find(" @")) + " @" + printer_preset_name);
            }

            /******************************   clone process preset    ********************************/
            failures.clear();
            if (!selected_process_presets.empty()) {
                generate_process_presets_data(selected_process_presets, printer_nozzle_name);
                bool create_preset_result = preset_bundle->prints.clone_presets_for_printer(selected_process_presets, failures, printer_preset_name,
                                                                                                           get_filament_id, rewritten);
                if (!create_preset_result) {
                    std::string message;
                    for (const std::string &failure : failures) { message += "\t" + failure + "\n"; }
                    MessageDialog dlg(this, _L("Create process presets failed. As follows:\n") + from_u8(message) + _L("\nDo you want to rewrite it?"),
                                      wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"),
                                      wxYES | wxYES_DEFAULT | wxCENTRE);
                    int res = dlg.ShowModal();
                    if (wxID_YES == res) {
                        create_preset_result = preset_bundle->prints.clone_presets_for_printer(selected_process_presets, failures, printer_preset_name, get_filament_id, true);
                    } else {
                        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " printer preset no same preset but process has same preset, user cancel create the printer preset";
                        return;
                    }
                }
            }
        } else if (curr_selected_preset_type == m_create_type.base_curr_printer) { // create printer and based on printer
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " base curr printer";
            /******************************   clone filament preset    ********************************/
            std::vector<std::string> failures;
            if (!selected_filament_presets.empty()) {
                bool create_preset_result = preset_bundle->filaments.clone_presets_for_printer(selected_filament_presets, failures, printer_preset_name, get_filament_id, rewritten);
                if (!create_preset_result) {
                    std::string message;
                    for (const std::string& failure : failures) {
                        message += "\t" + failure + "\n";
                    }
                    MessageDialog dlg(this, _L("Create filament presets failed. As follows:\n") + from_u8(message) + _L("\nDo you want to rewrite it?"),
                                      wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"),
                                      wxYES | wxYES_DEFAULT | wxCENTRE);
                    int           res = dlg.ShowModal();
                    if (wxID_YES == res) {
                        create_preset_result = preset_bundle->filaments.clone_presets_for_printer(selected_filament_presets, failures, printer_preset_name, get_filament_id, true);
                    } else {
                        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " printer preset no same preset but filament has same preset, user cancel create the printer preset";
                        return;
                    }
                }
            }

            /******************************   clone process preset    ********************************/
            failures.clear();
            if (!selected_process_presets.empty()) {
                bool create_preset_result = preset_bundle->prints.clone_presets_for_printer(selected_process_presets, failures, printer_preset_name, get_filament_id, rewritten);
                if (!create_preset_result) {
                    std::string message;
                    for (const std::string& failure : failures) {
                        message += "\t" + failure + "\n";
                    }
                    MessageDialog dlg(this, _L("Create process presets failed. As follows:\n") + from_u8(message) + _L("\nDo you want to rewrite it?"), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES | wxYES_DEFAULT | wxCENTRE);
                    int           res = dlg.ShowModal();
                    if (wxID_YES == res) {
                        create_preset_result = preset_bundle->prints.clone_presets_for_printer(selected_process_presets, failures, printer_preset_name, get_filament_id, true);
                    } else {
                        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " printer preset no same preset but filament has same preset, user cancel create the printer preset";
                        return;
                    }
                }
                // save created successfully preset name
                for (Preset const *sucessful_preset : selected_filament_presets)
                    successful_preset_names.push_back(sucessful_preset->name.substr(0, sucessful_preset->name.find(" @")) + " @" + printer_preset_name);
            }
        }
         
        /******************************   clone printer preset     ********************************/
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ":creater printer ";
        try {
            auto printer_model = dynamic_cast<ConfigOptionString *>(m_printer_preset->config.option("printer_model", true));
            if (printer_model)
                printer_model->value = printer_model_name;

            auto printer_variant = dynamic_cast<ConfigOptionString *>(m_printer_preset->config.option("printer_variant", true));
            if (printer_variant)
                printer_variant->value = printer_nozzle_name;

            auto nozzle_diameter = dynamic_cast<ConfigOptionFloats *>(m_printer_preset->config.option("nozzle_diameter", true));
            if (nozzle_diameter) {
                std::unordered_map<std::string, float>::const_iterator iter = nozzle_diameter_map.find(printer_nozzle_name);
                if (nozzle_diameter_map.end() != iter) 
                {
                    //nozzle_diameter->values = {iter->second};
                    for(int i = 0; i < nozzle_diameter->values.size(); i++)
                    {
                        nozzle_diameter->values[i] = iter->second;
                    }
                }
            }
        }
        catch (...) {
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " bisic info is not rewritten, may be printer_model, printer_variant, or nozzle_diameter";
        }
        preset_bundle->printers.save_current_preset(printer_preset_name, true, false, m_printer_preset);
        preset_bundle->update_compatible(PresetSelectCompatibleType::Always);
        EndModal(wxID_OK);
        
        });

    m_button_page2_cancel = new Button(parent, _L("Cancel"));
    m_button_page2_cancel->SetBackgroundColor(btn_bg_white);
    m_button_page2_cancel->SetBorderColor(wxColour(38, 46, 48));
    m_button_page2_cancel->SetFont(Label::Body_12);
    m_button_page2_cancel->SetSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_page2_cancel->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_page2_cancel->SetCornerRadius(FromDIP(12));
    bSizer_button->Add(m_button_page2_cancel, 0, wxRIGHT, FromDIP(10));

    m_button_page2_cancel->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent &e) { EndModal(wxID_CANCEL); });

    return bSizer_button;
}

void CreatePrinterPresetDialog::show_page1()
{
    m_step_1->SetBitmap(create_scaled_bitmap("step_1", nullptr, FromDIP(20)));
    m_step_2->SetBitmap(create_scaled_bitmap("step_2_ready", nullptr, FromDIP(20)));
    m_page1->Show();
    m_page2->Hide();
    Refresh();
    Layout();
    Fit();
}

void CreatePrinterPresetDialog::update_current_printer_text()
{
    if (!m_current_printer_text)
    {
        return;
    }

    string strCurrentText;
    if ((1 == m_type) && (m_printer_model_page2))
    {
        strCurrentText = (into_u8(m_printer_model_page2->GetStringSelection()));
    }
    else
    {
        strCurrentText = (into_u8(m_printer_model->GetStringSelection()));
    }
    size_t spacePos = strCurrentText.find(' ');
    if (spacePos != string::npos)
    {
        strCurrentText = strCurrentText.substr(spacePos);
    }
    std::replace(strCurrentText.begin(), strCurrentText.end(), '@', '-');

    m_current_printer_text->SetLabel(_L("Based on") + strCurrentText);
}


void CreatePrinterPresetDialog::show_page2()
{
    if (m_type == 1)
    {
        data_init_vendor_and_model();
    }
    else
    {
        update_current_printer_text();
    }

    m_step_1->SetBitmap(create_scaled_bitmap("step_is_ok", nullptr, FromDIP(20)));
    m_step_2->SetBitmap(create_scaled_bitmap("step_2", nullptr, FromDIP(20)));
    m_page2->Show();
    m_page1->Hide();
    Refresh();
    Layout();
    Fit();
}

bool CreatePrinterPresetDialog::data_init()
{ 
    //VendorMap vendors;
    wxArrayString exist_vendor_choice = get_exist_vendor_choices(m_vendors);

    std::string nozzle_type = into_u8(m_nozzle_diameter->GetStringSelection());
    size_t      index_mm = nozzle_type.find(" mm");
    if (std::string::npos != index_mm)
    {
        nozzle_type = nozzle_type.substr(0, index_mm);
    }
    m_nozzle = nozzle_diameter_map[nozzle_type];
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " entry and nozzle type is: " << nozzle_type << " and nozzle is: " << m_nozzle;

    m_select_printer->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent e) {
        m_select_printer->SetLabelColor(*wxBLACK);

        update_nozzle_data_new();

        std::string selected_model_name = into_u8(m_select_printer->GetStringSelection());
        string strModel = selected_model_name + " @ " + "0.4" + " nozzle";
        string vendorName;
        for (const auto& pair : m_printer_name_to_preset)
        {
            const auto& preset = pair.second;
            if (selected_model_name == preset->config.option<ConfigOptionString>("printer_model", false)->value)
            {
                if(preset->vendor)
                {
                    vendorName = preset->vendor->name;
                    break;
                }
            }
        }

        int idex = 0;
        for (const auto& vendor : m_vendors) 
        {
            const std::string& vendor_name  = vendor.first; 
            if (vendor_name == vendorName) 
            {
                m_printer_preset_vendor_selected = vendor.second;

                m_printer_vendor->SetLabelColor(*wxBLACK);
                m_printer_vendor->SetSelection(idex);
               
                break;
            }
            idex++;
        }

        bool isFlag = false;
        int j = 0;
        wxArrayString printer_preset_model = printer_preset_sort_with_nozzle_diameter(m_printer_preset_vendor_selected, m_nozzle);
        if (printer_preset_model.size() != 0)
        {
            for (; j < printer_preset_model.size(); j++)
            {
                wxString strTmp = printer_preset_model[j];
                if (strTmp == strModel)
                {
                    isFlag = true;
                    break;
                }
            }
        }
        if(false == isFlag)
        {
            e.Skip();
            return ;
        }

        m_printer_model->Set(printer_preset_model);
        if (!printer_preset_model.empty())
        {
            m_printer_model->SetSelection(j);
            wxCommandEvent e;
            on_preset_model_value_change(e);
            update_preset_list_size();
        }
        rewritten = false;

        e.Skip();
        });

    m_printer_vendor->Set(exist_vendor_choice);

    m_printer_model->Bind(wxEVT_COMBOBOX, &CreatePrinterPresetDialog::on_preset_model_value_change, this);
    
    m_printer_vendor->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent e) {
        m_printer_vendor->SetLabelColor(*wxBLACK);

        std::string   curr_selected_vendor = into_u8(m_printer_vendor->GetStringSelection());
        auto          iterator             = m_vendors.find(curr_selected_vendor);
        if (iterator != m_vendors.end()) {
            m_printer_preset_vendor_selected = iterator->second;
        } else {
            MessageDialog dlg(this, _L("Vendor is not find, please reselect."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES_NO | wxYES_DEFAULT | wxCENTRE);
            dlg.ShowModal();
            return;
        }
        
        wxArrayString printer_preset_model = printer_preset_sort_with_nozzle_diameter(m_printer_preset_vendor_selected, m_nozzle);
        if (printer_preset_model.size() == 0) {
            MessageDialog dlg(this, _L("Current vendor has no models, please reselect."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES | wxYES_DEFAULT | wxCENTRE);
            dlg.ShowModal();
            return;
        }
        m_printer_model->Set(printer_preset_model);
        if (!printer_preset_model.empty()) { 
            m_printer_model->SetSelection(0);
            wxCommandEvent e;
            on_preset_model_value_change(e);
            update_preset_list_size();
        }
        rewritten = false;
        e.Skip();
        
    });
    return true;

}

bool CreatePrinterPresetDialog::data_init_vendor_and_model()
{ 
    if (into_u8(m_printer_vendor_page2->GetStringSelection()).empty())
    {
        wxArrayString choices;
        for (const pair<std::string, VendorProfile>& vendor : m_vendors)
        {
            if (vendor.second.models.empty() || vendor.second.id.empty())
            {
                continue;
            }
            choices.Add(vendor.first);
        }

        m_printer_vendor_page2->Set(choices);
        std::string   curr_selected_vendor = into_u8(m_printer_vendor->GetStringSelection());
        auto iterator = m_vendors.find(curr_selected_vendor);
        if (iterator != m_vendors.end())
        {
            m_printer_preset_vendor_selected = iterator->second;
            m_printer_vendor_page2->SetLabelColor(*wxBLACK);
            m_printer_vendor_page2->SetStringSelection(curr_selected_vendor);
        }

        std::string   curr_selected_model = into_u8(m_printer_model->GetStringSelection());
        wxArrayString printer_preset_model = printer_preset_sort_with_nozzle_diameter(m_printer_preset_vendor_selected, m_nozzle);
        if (!printer_preset_model.empty())
        {
            m_printer_model_page2->Set(printer_preset_model);
            m_printer_model_page2->SetStringSelection(curr_selected_model);

            //刷新列表
            wxCommandEvent e;
            on_preset_model_page2_value_change(e);
        }
    }

    std::string   curr_selected_vendor = into_u8(m_printer_vendor_page2->GetStringSelection());
    auto iterator = m_vendors.find(curr_selected_vendor);
    if (iterator != m_vendors.end())
    {
        m_printer_preset_vendor_selected = iterator->second;
    }

    m_printer_model_page2->Bind(wxEVT_COMBOBOX, &CreatePrinterPresetDialog::on_preset_model_page2_value_change, this);
    m_printer_vendor_page2->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent e) 
    {
        m_printer_vendor_page2->SetLabelColor(*wxBLACK);

        VendorProfile  printer_preset_vendor_selected;
        std::string   curr_selected_vendor = into_u8(m_printer_vendor_page2->GetStringSelection());
        auto          iterator = m_vendors.find(curr_selected_vendor);
        if (iterator != m_vendors.end())
        {
            m_printer_preset_vendor_selected = iterator->second;
        }
        else
        {
            MessageDialog dlg(this, _L("Vendor is not find, please reselect."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES_NO | wxYES_DEFAULT | wxCENTRE);
            dlg.ShowModal();
            return;
        }

        wxArrayString printer_preset_model = printer_preset_sort_with_nozzle_diameter(m_printer_preset_vendor_selected, m_nozzle);
        if (printer_preset_model.size() == 0)
        {
            MessageDialog dlg(this, _L("Current vendor has no models, please reselect."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES | wxYES_DEFAULT | wxCENTRE);
            dlg.ShowModal();
            return;
        }
        m_printer_model_page2->Set(printer_preset_model);
        if (!printer_preset_model.empty())
        {
            m_printer_model_page2->SetSelection(0);
            wxCommandEvent e;
            on_preset_model_page2_value_change(e);
            //update_preset_list_size();
        }
        rewritten = false;
        e.Skip();

        });
    return true;
}


void CreatePrinterPresetDialog::set_current_visible_printer()
{
    //The entire process of creating a custom printer only needs to be done once
    if (m_printer_name_to_preset.size() > 0) return;
    PresetBundle *preset_bundle = wxGetApp().preset_bundle; 
    const std::deque<Preset> &printer_presets =  preset_bundle->printers.get_presets();
    wxArrayString             printer_choice;
    m_printer_name_to_preset.clear();
    for (const Preset &printer_preset : printer_presets) {
        if (printer_preset.is_system || !printer_preset.is_visible) continue;
        if (preset_bundle->printers.get_preset_base(printer_preset)->name != printer_preset.name) continue;
        printer_choice.push_back(from_u8(printer_preset.name));
        m_printer_name_to_preset[printer_preset.name] = std::make_shared<Preset>(printer_preset);
    }
    m_select_printer->Set(printer_choice);
}

void CreatePrinterPresetDialog::set_current_visible_printer_new()
{
    //The entire process of creating a custom printer only needs to be done once
    if (m_printer_name_to_preset.size() > 0) return;
    PresetBundle *preset_bundle = wxGetApp().preset_bundle; 
    const std::deque<Preset> &printer_presets =  preset_bundle->printers.get_presets();
    wxArrayString             printer_choice;
    m_printer_name_to_preset.clear();
    for (const Preset &printer_preset : printer_presets) {
        if (!printer_preset.is_visible) continue;
        //if (preset_bundle->printers.get_preset_base(printer_preset)->name != printer_preset.name) continue;

        m_printer_name_to_preset[printer_preset.name] = std::make_shared<Preset>(printer_preset);
    }

    for (const auto& pair : m_printer_name_to_preset) 
    {
        const std::shared_ptr<Preset>& preset = pair.second;
        string strModel = preset->config.option<ConfigOptionString>("printer_model", false)->value;
        if (std::find(printer_choice.begin(), printer_choice.end(), from_u8(strModel)) == printer_choice.end()) 
        {
            printer_choice.push_back(from_u8(strModel)); 
        }
    }

    m_select_printer->Set(printer_choice);

    //鑾峰彇鎵撳嵃鏈哄搴旂殑鍠峰槾淇℃伅  鏍规嵁json鏁版嵁
    load_nozzle_info_form_json();
}

void CreatePrinterPresetDialog::load_nozzle_info_form_json()
{
    json m_profile_json;
    try
    {
        m_profile_json = json::parse("{}");
        m_profile_json["model"] = json::array();

         boost::filesystem::path vendor_dir = (boost::filesystem::path(Slic3r::data_dir()) / PRESET_SYSTEM_DIR).make_preferred();
         boost::filesystem::path rsrc_vendor_dir = (boost::filesystem::path(resources_dir()) / "profiles").make_preferred();

        auto bbl_bundle_path = vendor_dir;
        bool bbl_bundle_rsrc = false;
        if (!boost::filesystem::exists((vendor_dir / PresetBundle::BBL_BUNDLE).replace_extension(".json")))
        {
            bbl_bundle_path = rsrc_vendor_dir;
            bbl_bundle_rsrc = true;
        }

        //string                                targetPath = bbl_bundle_path.make_preferred().string();
        //boost::filesystem::path               myPath(targetPath);
        //boost::filesystem::directory_iterator endIter;
        //for (boost::filesystem::directory_iterator iter(myPath); iter != endIter; iter++)
        //{
        //    if (boost::filesystem::is_directory(*iter))
        //    {
        //        //cout << "is dir" << endl;
        //        //cout << iter->path().string() << endl;
        //    }
        //    else
        //    {
        //        //cout << "is a file" << endl;
        //        //cout << iter->path().string() << endl;
        //        wxString strVendor = from_u8(iter->path().string()).BeforeLast('.');
        //        strVendor = strVendor.AfterLast('\\');
        //        strVendor = strVendor.AfterLast('\/');
        //        wxString strExtension = from_u8(iter->path().string()).AfterLast('.').Lower();
        //        if (w2s(strVendor) == PresetBundle::BBL_BUNDLE && strExtension.CmpNoCase("json") == 0)
        //            LoadProfileFamily(w2s(strVendor), iter->path().string());
        //    }
        //}

        boost::filesystem::directory_iterator others_endIter;
        for (boost::filesystem::directory_iterator iter(rsrc_vendor_dir); iter != others_endIter; iter++)
        {
            if (boost::filesystem::is_directory(*iter))
            {
                //cout << "is dir" << endl;
                //cout << iter->path().string() << endl;
            }
            else
            {
                wxString strVendor = from_u8(iter->path().string()).BeforeLast('.');
                strVendor = strVendor.AfterLast('\\');
                strVendor = strVendor.AfterLast('\/');
                wxString strExtension = from_u8(iter->path().string()).AfterLast('.').Lower();

                if (strVendor.ToStdString() != PresetBundle::BBL_BUNDLE && strExtension.CmpNoCase("json") == 0)
                {
                    load_profile_family(strVendor.ToStdString(), iter->path().string());
                }
            }
        }
    }
    catch (std::exception& e)
    {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ", error: " << e.what() << std::endl;
    }

    std::string strAll = m_profile_json.dump(-1, ' ', false, json::error_handler_t::ignore);

    return;
}

void CreatePrinterPresetDialog::StringReplace(string &strBase, string strSrc, string strDes)
{
    string::size_type pos    = 0;
    string::size_type srcLen = strSrc.size();
    string::size_type desLen = strDes.size();
    pos                      = strBase.find(strSrc, pos);
    while ((pos != string::npos)) {
        strBase.replace(pos, srcLen, strDes);
        pos = strBase.find(strSrc, (pos + desLen));
    }
}

int CreatePrinterPresetDialog::load_profile_family(std::string strVendor, std::string strFilePath)
{
    boost::filesystem::path file_path(strFilePath);
    boost::filesystem::path vendor_dir = boost::filesystem::absolute(file_path.parent_path() / strVendor).make_preferred();

    std::string contents;
    load_file(strFilePath, contents);
    json jLocal = json::parse(contents);
    json pmodels = jLocal["machine_model_list"];
    int  nsize = pmodels.size();

    for (int n = 0; n < nsize; n++)
    {
        json OneModel = pmodels.at(n);

        OneModel["model"] = OneModel["name"];
        OneModel.erase("name");

        std::string s1 = OneModel["model"];
        std::string s2 = OneModel["sub_path"];

        boost::filesystem::path sub_path = boost::filesystem::absolute(vendor_dir / s2).make_preferred();
        std::string             sub_file = sub_path.string();

        load_file(sub_file, contents);
        json pm = json::parse(contents);

        OneModel["vendor"] = strVendor;
        std::string NozzleOpt = pm["nozzle_diameter"];
        StringReplace(NozzleOpt, " ", "");
        OneModel["nozzle_diameter"] = NozzleOpt;
        OneModel["materials"] = pm["default_materials"];

        //std::string             cover_file = s1 + "_cover.png";
        //boost::filesystem::path cover_path = boost::filesystem::absolute(boost::filesystem::path(resources_dir()) / "/profiles/" / strVendor / cover_file).make_preferred();
        //if (!boost::filesystem::exists(cover_path))
        //{
        //    cover_path =
        //        (boost::filesystem::absolute(boost::filesystem::path(resources_dir()) / "/web/image/printer/") /
        //            cover_file)
        //        .make_preferred();
        //}
        //OneModel["cover"] = cover_path.string();

        PrinterAndNozzleInfo tempInfo;
        tempInfo.printer_vendor = strVendor;
        tempInfo.model_name = s1;
        std::stringstream ss(NozzleOpt);
        std::string item;
        while (std::getline(ss, item, ';')) 
        {
            tempInfo.vec_nozzle.push_back(item);
        }

        m_printer_nozzle_info.push_back(tempInfo);

    }
    return 0;
}

bool CreatePrinterPresetDialog::load_file(std::string jPath, std::string &sContent)
{
    try {
        boost::nowide::ifstream t(jPath);
        std::stringstream buffer;
        buffer << t.rdbuf();
        sContent=buffer.str();
        BOOST_LOG_TRIVIAL(trace) << __FUNCTION__ << boost::format(", load %1% into buffer")% jPath;
    }
    catch (std::exception &e)
    {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ",  got exception: "<<e.what();
        return false;
    }

    return true;
}

wxArrayString CreatePrinterPresetDialog::printer_preset_sort_with_nozzle_diameter(const VendorProfile &vendor_profile, float nozzle_diameter)
{
    std::vector<pair<float, std::string>> preset_sort;

    for (const Slic3r::VendorProfile::PrinterModel &model : vendor_profile.models) {
        std::string model_name = model.name;
        for (const Slic3r::VendorProfile::PrinterVariant &variant : model.variants) {
            try {
                float variant_diameter = std::stof(variant.name);
                preset_sort.push_back(std::make_pair(variant_diameter, model_name + " @ " + variant.name + " nozzle"));
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "nozzle: " << variant_diameter << "model: " << preset_sort.back().second;
            }
            catch (...) {
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " prase varient fialed and the model_name is: " << model_name;
                continue;
            }
        }
    }

    std::sort(preset_sort.begin(), preset_sort.end(), [](const std::pair<float, std::string> &a, const std::pair<float, std::string> &b) { return a.first < b.first; });

    int index_nearest_nozzle = -1;
    float nozzle_diameter_diff = 1;
    for (int i = 0; i < preset_sort.size(); ++i) { 
        float curr_nozzle_diameter_diff = std::abs(nozzle_diameter - preset_sort[i].first);
        if (curr_nozzle_diameter_diff < nozzle_diameter_diff) { 
            index_nearest_nozzle = i;
            nozzle_diameter_diff = curr_nozzle_diameter_diff;
            if (curr_nozzle_diameter_diff == 0) break;
        }
    }
    wxArrayString printer_preset_model_selection;
    int right_index = index_nearest_nozzle + 1;
    while (index_nearest_nozzle >= 0 || right_index < preset_sort.size()) { 
        if (index_nearest_nozzle >= 0 && right_index < preset_sort.size()) {
            float left_nozzle_diff  = std::abs(nozzle_diameter - preset_sort[index_nearest_nozzle].first);
            float right_nozzle_diff = std::abs(nozzle_diameter - preset_sort[right_index].first);
            bool  left_is_little    = left_nozzle_diff < right_nozzle_diff;
            if (left_is_little) {
                printer_preset_model_selection.Add(from_u8(preset_sort[index_nearest_nozzle].second));
                index_nearest_nozzle--;
            } else {
                printer_preset_model_selection.Add(from_u8(preset_sort[right_index].second));
                right_index++;
            }
        } else if (index_nearest_nozzle >= 0) {
            printer_preset_model_selection.Add(from_u8(preset_sort[index_nearest_nozzle].second));
            index_nearest_nozzle--;
        } else if (right_index < preset_sort.size()) {
            printer_preset_model_selection.Add(from_u8(preset_sort[right_index].second));
            right_index++;
        }    
    }
    return printer_preset_model_selection;
}

void CreatePrinterPresetDialog::select_all_preset_template(std::vector<std::pair<::CheckBox *, Preset *>> &preset_templates)
{
    for (std::pair<::CheckBox *, Preset const *> filament_preset : preset_templates) { 
        filament_preset.first->SetValue(true);
    }
}

void CreatePrinterPresetDialog::deselect_all_preset_template(std::vector<std::pair<::CheckBox *, Preset *>> &preset_templates)
{
    for (std::pair<::CheckBox *, Preset const *> filament_preset : preset_templates) { 
        filament_preset.first->SetValue(false); 
    }
}

void CreatePrinterPresetDialog::update_presets_list(bool just_template, bool isUseDefault)
{
    PresetBundle temp_preset_bundle;
    if (!load_system_and_user_presets_with_curr_model(temp_preset_bundle, just_template, isUseDefault)) return;

    const std::deque<Preset> &filament_presets = temp_preset_bundle.filaments.get_presets();
    const std::deque<Preset> &process_presets  = temp_preset_bundle.prints.get_presets();

    // clear filament preset window sizer
    m_preset_template_panel->Freeze();
    clear_preset_combobox();

    // update filament preset window sizer
    for (const Preset &filament_preset : filament_presets) {
        if (filament_preset.is_compatible) {
            if (filament_preset.is_default) continue;
            Preset *temp_filament = new Preset(filament_preset);
            wxString filament_name = wxString::FromUTF8(temp_filament->name);
            m_filament_preset_template_sizer->Add(create_checkbox(m_filament_preset_panel, temp_filament, filament_name, m_filament_preset), 0,
                                                  wxEXPAND, FromDIP(5));
        }
    }

    select_all_preset_template(m_filament_preset);

    for (const Preset &process_preset : process_presets) {
        if (process_preset.is_compatible) { 
            if (process_preset.is_default) continue;

            Preset *temp_process = new Preset(process_preset);
            wxString process_name = wxString::FromUTF8(temp_process->name);
            m_process_preset_template_sizer->Add(create_checkbox(m_process_preset_panel, temp_process, process_name, m_process_preset), 0, wxEXPAND,
                                                 FromDIP(5));
        }
    }

    select_all_preset_template(m_process_preset);

    m_preset_template_panel->Thaw();
}

void CreatePrinterPresetDialog::clear_preset_combobox()
{ 
    for (std::pair<::CheckBox *, Preset *> preset : m_filament_preset) {
        if (preset.second) { 
            delete preset.second;
            preset.second = nullptr;
        }
    }
    m_filament_preset.clear();
    m_filament_preset_template_sizer->Clear(true);

    for (std::pair<::CheckBox *, Preset *> preset : m_process_preset) {
        if (preset.second) {
            delete preset.second;
            preset.second = nullptr;
        }
    }
    m_process_preset.clear();
    m_process_preset_template_sizer->Clear(true);
}

bool CreatePrinterPresetDialog::save_printable_area_config(Preset *preset)
{
    const wxString      curr_selected_printer_type = curr_create_printer_type();
    DynamicPrintConfig &config                     = preset->config;

    if (curr_selected_printer_type == m_create_type.create_printer) {
        double x = 0;
        m_bed_size_x_input->GetTextCtrl()->GetValue().ToDouble(&x);
        double y = 0;
        m_bed_size_y_input->GetTextCtrl()->GetValue().ToDouble(&y);
        double dx = 0;
        m_bed_origin_x_input->GetTextCtrl()->GetValue().ToDouble(&dx);
        double dy = 0;
        m_bed_origin_y_input->GetTextCtrl()->GetValue().ToDouble(&dy);
        // range check begin
        if (x == 0 || y == 0) { return false; }
        double x0 = 0.0;
        double y0 = 0.0;
        double x1 = x;
        double y1 = y;
        if (dx >= x || dy >= y) { return false; }
        x0 -= dx;
        x1 -= dx;
        y0 -= dy;
        y1 -= dy;
        // range check end
        std::vector<Vec2d> points = {Vec2d(x0, y0), Vec2d(x1, y0), Vec2d(x1, y1), Vec2d(x0, y1)};
        config.set_key_value("printable_area", new ConfigOptionPoints(points));

        double max_print_height = 0;
        m_print_height_input->GetTextCtrl()->GetValue().ToDouble(&max_print_height);
        config.set("printable_height", max_print_height);

        Utils::slash_to_back_slash(m_custom_texture);
        Utils::slash_to_back_slash(m_custom_model);
        config.set("bed_custom_model", m_custom_model);
        config.set("bed_custom_texture", m_custom_texture);

    } 
    else if(m_create_type.create_nozzle)
    {
        //应产品要求
        //std::string selected_printer_preset_name = into_u8(m_select_printer->GetStringSelection());
        //std::unordered_map<std::string, std::shared_ptr<Preset>>::iterator itor = m_printer_name_to_preset.find(selected_printer_preset_name);
        //assert(m_printer_name_to_preset.end() != itor);
        //if (m_printer_name_to_preset.end() != itor) {
        //    std::shared_ptr<Preset> printer_preset = itor->second;
        //    std::vector<std::string> keys = { "printable_area", "printable_height", "bed_custom_model", "bed_custom_texture" };
        //    config.apply_only(printer_preset->config, keys, true);
        //}

        double min_height_input = 0;
        double max_height_input = 0;
        double length_input = 0;
        m_print_min_height_input->GetTextCtrl()->GetValue().ToDouble(&min_height_input);
        m_print_max_height_input->GetTextCtrl()->GetValue().ToDouble(&max_height_input);
        m_print_length_input->GetTextCtrl()->GetValue().ToDouble(&length_input);

        config.set_key_value("retraction_length", new ConfigOptionFloats{ length_input });
        config.set_key_value("min_layer_height", new ConfigOptionFloats{ min_height_input });
        config.set_key_value("max_layer_height", new ConfigOptionFloats{ max_height_input });
    }
    return true;
}

bool CreatePrinterPresetDialog::check_printable_area() {
    double x = 0;
    m_bed_size_x_input->GetTextCtrl()->GetValue().ToDouble(&x);
    double y = 0;
    m_bed_size_y_input->GetTextCtrl()->GetValue().ToDouble(&y);
    double dx = 0;
    m_bed_origin_x_input->GetTextCtrl()->GetValue().ToDouble(&dx);
    double dy = 0;
    m_bed_origin_y_input->GetTextCtrl()->GetValue().ToDouble(&dy);
    // range check begin
    if (x == 0 || y == 0) {
        return false;
    }
    double x0 = 0.0;
    double y0 = 0.0;
    double x1 = x;
    double y1 = y;
    if (dx >= x || dy >= y) {
        return false;
    }
    return true;
}

bool CreatePrinterPresetDialog::validate_input_valid()
{
    const wxString curr_selected_printer_type = curr_create_printer_type();
    if (curr_selected_printer_type == m_create_type.create_printer) {
        std::string vendor_name, model_name;
        if (m_can_not_find_vendor_combox->GetValue()) {
            vendor_name = into_u8(m_custom_vendor_text_ctrl->GetValue());
            model_name  = into_u8(m_custom_model_text_ctrl->GetValue());

        } else {
            vendor_name = into_u8(m_select_vendor->GetStringSelection());
            model_name  = into_u8(m_select_model->GetStringSelection());
        }
        if ((vendor_name.empty() || model_name.empty())) {
            MessageDialog dlg(this, _L("You have not selected the vendor and model or inputed the custom vendor and model."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"),
                              wxYES | wxYES_DEFAULT | wxCENTRE);
            dlg.ShowModal();
            return false;
        }

        vendor_name = remove_special_key(vendor_name);
        model_name  = remove_special_key(model_name);
        if (vendor_name.empty() || model_name.empty()) {
            MessageDialog dlg(this, _L("There may be escape characters in the custom printer vendor or model. Please delete and re-enter."),
                              wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES | wxYES_DEFAULT | wxCENTRE);
            dlg.ShowModal();
            return false;
        }
        boost::algorithm::trim(vendor_name);
        boost::algorithm::trim(model_name);
        if (vendor_name.empty() || model_name.empty()) {
            MessageDialog dlg(this, _L("All inputs in the custom printer vendor or model are spaces. Please re-enter."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"),
                              wxYES | wxYES_DEFAULT | wxCENTRE);
            dlg.ShowModal();
            return false;
        }

        if (check_printable_area() == false) {
            MessageDialog dlg(this, _L("Please check bed printable shape and origin input."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES | wxYES_DEFAULT | wxCENTRE);
            dlg.ShowModal();
            return false;
        }
    } else if (curr_selected_printer_type == m_create_type.create_nozzle) {
        wxString printer_name = m_select_printer->GetStringSelection();
        if (printer_name.empty()) {
            MessageDialog dlg(this, _L("You have not yet selected the printer to replace the nozzle, please choose."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"),
                              wxYES | wxYES_DEFAULT | wxCENTRE);
            dlg.ShowModal();
            return false;
        }
    }
    
    return true;
}

void CreatePrinterPresetDialog::on_preset_model_value_change(wxCommandEvent &e)
{
    m_printer_model->SetLabelColor(*wxBLACK);
    if (m_printer_preset_vendor_selected.models.empty()) {
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " selected vendor has no models, and the vendor is: " << m_printer_preset_vendor_selected.id;
        return;
    }

    if (m_type == 1)
    {
        PresetBundle temp_preset_bundle;
        bool just_template = false;
        if (!load_system_and_user_presets_with_curr_model(temp_preset_bundle, just_template, true)) 
            return;
    }
    else
    {
        wxString curr_selected_preset_type = curr_create_preset_type();
        if (curr_selected_preset_type == m_create_type.base_curr_printer)
        {
            update_presets_list();
        }
        else if (curr_selected_preset_type == m_create_type.base_template)
        {
            update_presets_list(true);
        }
        rewritten = false;
    }

    update_preset_list_size();

    update_nozzle_other_data_new();

    update_printer_other_data_new();

    e.Skip();
}

void CreatePrinterPresetDialog::on_preset_model_page2_value_change(wxCommandEvent &e)
{

    m_printer_model_page2->SetLabelColor(*wxBLACK);
    if (m_printer_preset_vendor_selected.models.empty()) {
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " selected vendor has no models, and the vendor is: " << m_printer_preset_vendor_selected.id;
        return;
    }

    wxString curr_selected_preset_type = curr_create_preset_type();
    if (curr_selected_preset_type == m_create_type.base_curr_printer) 
    {
        //update_presets_list(false);
        (m_type == 1) ? update_presets_list(false,false) : update_presets_list(false,true);
    } 
    else if (curr_selected_preset_type == m_create_type.base_template) 
    {
        update_presets_list(true);
    }
    rewritten = false;

    update_current_printer_text();
    update_preset_list_size();

    e.Skip();
}


wxString CreatePrinterPresetDialog::curr_create_preset_type()
{
    wxString curr_selected_preset_type;
    for (const std::pair<RadioBox *, wxString> &presets_radio : m_create_presets_btns) {
        if (presets_radio.first->GetValue()) { 
            curr_selected_preset_type = presets_radio.second; 
        }
    }
    return curr_selected_preset_type;
}

wxString CreatePrinterPresetDialog::curr_create_printer_type()
{
    wxString curr_selected_printer_type;
    for (const std::pair<RadioBox *, wxString> &printer_radio : m_create_type_btns) {
        if (printer_radio.first->GetValue()) { curr_selected_printer_type = printer_radio.second; }
    }
    return curr_selected_printer_type;
}

CreatePresetSuccessfulDialog::CreatePresetSuccessfulDialog(wxWindow *parent, const SuccessType &create_success_type)
    : DPIDialog(parent ? parent : nullptr, wxID_ANY, PRINTER == create_success_type ? _L("Create Printer Successful") : _L("Create Filament Successful"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX)
{
    this->SetBackgroundColour(*wxWHITE);
    this->SetSize(wxSize(FromDIP(450), FromDIP(200)));
    std::string icon_path = (boost::format("%1%/images/Creative3DTitle.ico") % resources_dir()).str();
    SetIcon(wxIcon(encode_path(icon_path.c_str()), wxBITMAP_TYPE_ICO));

    wxBoxSizer *m_main_sizer = new wxBoxSizer(wxVERTICAL);
    // top line
    auto m_line_top = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 1), wxTAB_TRAVERSAL);
    m_line_top->SetBackgroundColour(wxColour(0xA6, 0xa9, 0xAA));
    m_main_sizer->Add(m_line_top, 0, wxEXPAND, 0);
    m_main_sizer->Add(0, 0, 0, wxTOP, FromDIP(5));

    wxBoxSizer *horizontal_sizer = new wxBoxSizer(wxHORIZONTAL);
    horizontal_sizer->Add(0, 0, 0, wxLEFT, FromDIP(30));

    wxBoxSizer *success_bitmap_sizer = new wxBoxSizer(wxVERTICAL);
    wxStaticBitmap *success_bitmap       = new wxStaticBitmap(this,wxID_ANY, create_scaled_bitmap("create_success", nullptr, FromDIP(24)));
    success_bitmap_sizer->Add(success_bitmap, 0, wxEXPAND, 0);
    horizontal_sizer->Add(success_bitmap_sizer, 0, wxEXPAND | wxALL, FromDIP(5));

    wxBoxSizer *success_text_sizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText *success_text;
    wxStaticText *next_step_text;
    bool          sync_user_preset_need_enabled = wxGetApp().getAgent() && wxGetApp().app_config->get("sync_user_preset") == "false";
    switch (create_success_type) {
    case PRINTER: 
        success_text = new wxStaticText(this, wxID_ANY, _L("Printer Created")); 
        next_step_text = new wxStaticText(this, wxID_ANY, _L("Please go to printer settings to edit your presets")); 
        break;
    case FILAMENT: 
        success_text = new wxStaticText(this, wxID_ANY, _L("Filament Created")); 
        wxString prompt_text = _L("Please go to filament setting to edit your presets if you need.\nPlease note that nozzle temperature, hot bed temperature, and maximum "
                                  "volumetric speed has a significant impact on printing quality. Please set them carefully.");
        wxString sync_text = sync_user_preset_need_enabled ? _L("\n\nCrealiyPrint has detected that your user presets synchronization function is not enabled, which may result in unsuccessful Filament settings on "
                   "the Device page. \nClick \"Sync user presets\" to enable the synchronization function.") : "";
        next_step_text = new wxStaticText(this, wxID_ANY, prompt_text + sync_text); 
        break;
    }
    success_text->SetFont(Label::Head_18);
    success_text_sizer->Add(success_text, 0, wxEXPAND, 0);
    success_text_sizer->Add(next_step_text, 0, wxEXPAND | wxTOP, FromDIP(5));
    horizontal_sizer->Add(success_text_sizer, 0, wxEXPAND | wxALL, FromDIP(5));
    horizontal_sizer->Add(0, 0, 0, wxLEFT, FromDIP(60));

    m_main_sizer->Add(horizontal_sizer, 0, wxALL, FromDIP(5));

    wxBoxSizer *btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    btn_sizer->Add(0, 0, 1, wxEXPAND, 0);
    switch (create_success_type) {
    case PRINTER: 
        m_button_ok = new Button(this, _L("Printer Setting"));
        break;
    case FILAMENT: m_button_ok = sync_user_preset_need_enabled ? new Button(this, _L("Sync user presets")) : new Button(this, _L("OK"));
        break;
    }
    StateColor btn_bg_green(std::pair<wxColour, int>(wxColour(0, 137, 123), StateColor::Pressed), std::pair<wxColour, int>(wxColour(38, 166, 154), StateColor::Hovered),
                            std::pair<wxColour, int>(wxColour(0, 150, 136), StateColor::Normal));

    StateColor btn_bg_white(std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Pressed), std::pair<wxColour, int>(wxColour(238, 238, 238), StateColor::Hovered),
                            std::pair<wxColour, int>(*wxWHITE, StateColor::Normal));
    m_button_ok->SetBackgroundColor(btn_bg_green);
    m_button_ok->SetBorderColor(wxColour(*wxWHITE));
    m_button_ok->SetTextColor(wxColour(*wxWHITE));
    m_button_ok->SetFont(Label::Body_12);
    m_button_ok->SetSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_ok->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_ok->SetCornerRadius(FromDIP(12));
    btn_sizer->Add(m_button_ok, 0, wxRIGHT, FromDIP(10));

    m_button_ok->Bind(wxEVT_LEFT_DOWN, [this, sync_user_preset_need_enabled](wxMouseEvent &e) {
        if (sync_user_preset_need_enabled) {
            wxGetApp().app_config->set("sync_user_preset", "true");
            wxGetApp().start_sync_user_preset();
        }
        EndModal(wxID_OK);
        });
    
    if (PRINTER == create_success_type || sync_user_preset_need_enabled) {
        m_button_cancel = new Button(this, _L("Cancel"));
        m_button_cancel->SetBackgroundColor(btn_bg_white);
        m_button_cancel->SetBorderColor(wxColour(38, 46, 48));
        m_button_cancel->SetTextColor(wxColour(38, 46, 48));
        m_button_cancel->SetFont(Label::Body_12);
        m_button_cancel->SetSize(wxSize(FromDIP(58), FromDIP(24)));
        m_button_cancel->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
        m_button_cancel->SetCornerRadius(FromDIP(12));
        btn_sizer->Add(m_button_cancel, 0, wxRIGHT, FromDIP(10));
        m_button_cancel->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent &e) { EndModal(wxID_CANCEL); });
    }

    m_main_sizer->Add(btn_sizer, 0, wxEXPAND | wxALL, FromDIP(15));
    m_main_sizer->Add(0, 0, 0, wxTOP, FromDIP(10));

    SetSizer(m_main_sizer);
    Layout();
    Fit();
    wxGetApp().UpdateDlgDarkUI(this);
}

CreatePresetSuccessfulDialog::~CreatePresetSuccessfulDialog() {}

void CreatePresetSuccessfulDialog::on_dpi_changed(const wxRect &suggested_rect) {
    m_button_ok->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_ok->SetMaxSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_ok->SetCornerRadius(FromDIP(12));
    m_button_cancel->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_cancel->SetMaxSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_cancel->SetCornerRadius(FromDIP(12));
    Layout();
}

int ExportCheckbox::s_proportion = 1;
ExportCheckbox::ExportCheckbox(wxWindow* parent, Preset* preset) : wxPanel(parent, wxID_ANY)
{
    this->SetMinSize(wxSize(FromDIP(350), FromDIP(28)));
    this->SetBackgroundColour(wxColour("#3E3E40"));
    wxBoxSizer* mainBoxSize = new wxBoxSizer(wxHORIZONTAL);
    this->SetSizer(mainBoxSize);
    ::CheckBox* checkbox = new ::CheckBox(this);
    this->checkbox       = checkbox;
    mainBoxSize->Add(checkbox, 0, wxALIGN_CENTER_VERTICAL | wxALL, FromDIP(2));
    wxStaticText* preset_name_str = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(preset->name), wxDefaultPosition, wxDefaultSize,
                                                     wxST_NO_AUTORESIZE | wxST_ELLIPSIZE_END);
    preset_name_str->SetForegroundColour(wxColour("#FFFFFF"));
    wxToolTip* toolTip = new wxToolTip(wxString::FromUTF8(preset->name));
    preset_name_str->SetToolTip(toolTip);
    mainBoxSize->Add(preset_name_str, 0, wxALIGN_CENTER_VERTICAL|wxALL, FromDIP(5));
    this->preset = preset;
    proportion   = s_proportion++;
}

int ParamCheckbox::s_proportion = 1;
ParamCheckbox::ParamCheckbox(wxWindow* parent, Preset* preset) : wxPanel(parent, wxID_ANY)
{
    this->SetMinSize(wxSize(FromDIP(124), FromDIP(48)));
    this->SetBackgroundColour(wxColour("#18CC5C"));
    wxBoxSizer* mainBoxSize = new wxBoxSizer(wxHORIZONTAL);
    this->SetSizer(mainBoxSize);
    ::CheckBox* checkbox = new ::CheckBox(this);

    mainBoxSize->Add(checkbox, 0, 0, 0);
    wxStaticText* preset_name_str = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(preset->name));
    preset_name_str->SetForegroundColour(wxColour("#FFFFFF"));
    wxToolTip* toolTip = new wxToolTip(wxString::FromUTF8(preset->name));
    preset_name_str->SetToolTip(toolTip);
    mainBoxSize->Add(preset_name_str, 0, wxLEFT, FromDIP(5));
    this->preset = preset;
    proportion   = s_proportion++;
}

class HoveableCheckbox : public wxPanel
{
    bool                     m_bHoved     = false;
    bool                     m_bSelected  = false;
    int                      m_state      = 0; // 0-未选中，1-选中，2-半选中
    bool                     m_bClicked   = false;
    wxPoint                  m_clickedPos = wxPoint();
    wxRect                   m_rtCheckbox = wxRect(FromDIP(5), FromDIP(5), FromDIP(16), FromDIP(16));
    bool                     m_ellipsize  = true;
    int                      m_round      = 2;
    wxString                 m_label;
    std::function<void(int)> m_funcItemCheckedCb = nullptr;
    wxColour                 m_bkColour          = wxColour("#4B4B4D");
    std::function<void()>    m_funcItemClickedCb = nullptr;

public:
    HoveableCheckbox(wxWindow* parent, wxString label) : wxPanel(parent, wxID_ANY)
    {
        m_label = label;
        this->SetBackgroundColour(m_bkColour);

        Bind(wxEVT_PAINT, &HoveableCheckbox::OnPaint, this);
        this->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& e) {
            m_bHoved = true;
            Refresh();
        });
        this->Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& e) {
            m_bHoved = false;
            Refresh();
        });
        this->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& e) {
            m_bClicked   = true;
            m_clickedPos = wxGetMousePosition();
            m_clickedPos = ScreenToClient(m_clickedPos);

            if (m_clickedPos.x >= m_rtCheckbox.x && m_clickedPos.x <= m_rtCheckbox.x + m_rtCheckbox.width &&
                m_clickedPos.y >= m_rtCheckbox.y && m_clickedPos.y <= m_rtCheckbox.y + m_rtCheckbox.height) {
                if (m_state == 0) {
                    m_state = 1;
                    if (m_funcItemCheckedCb) {
                        m_funcItemCheckedCb(m_state);
                    }
                } else if (m_state == 1) {
                    m_state = 0;
                    if (m_funcItemCheckedCb) {
                        m_funcItemCheckedCb(m_state);
                    }
                }
            }
            if (m_funcItemClickedCb) {
                m_funcItemClickedCb();
            }
            Refresh();
        });
        this->SetToolTip(m_label);
    }
    bool GetValue()
    {
        bool value = (m_state == 0) ? false : true;
        return value;
    }
    void SetValue(bool value)
    {
        if (value)
            m_state = 1;
        else
            m_state = 0;
        Refresh();
    }
    void SetSelected(bool bSelected)
    {
        m_bSelected = bSelected;
        Refresh();
    }
    void OnPaint(wxPaintEvent& e)
    {
        int        width  = GetClientSize().GetWidth();
        int        height = GetClientSize().GetHeight();
        wxMemoryDC memDC;
        {
            wxBitmap bitmap(width, height, -1);
            memDC.SelectObject(bitmap);
            m_bkColour = this->GetBackgroundColour();
            memDC.SetBrush(wxBrush(m_bkColour, wxBRUSHSTYLE_SOLID));
            if (m_bHoved || m_bSelected)
                memDC.SetPen(wxPen(wxColour("#17CC5F"), 1, wxPENSTYLE_SOLID));
            else
                memDC.SetPen(wxPen(wxColour("#17CC5F"), 1, wxTRANSPARENT));
            memDC.DrawRectangle(wxRect(0, 0, width, height));

            // 画checkbox
            if (m_round > 0) {
                wxGraphicsContext* gc = wxGraphicsContext::Create(memDC);
                if (gc) {
                    if (m_state == 0) {
                        gc->SetPen(wxPen(wxColour("#6E6E72"), 2, wxPENSTYLE_SOLID));
                        gc->DrawRoundedRectangle(m_rtCheckbox.x, m_rtCheckbox.y, m_rtCheckbox.width, m_rtCheckbox.height, m_round);
                    } else if (m_state == 1) {
                        gc->SetBrush(wxBrush(wxColour("#17CC5F"), wxBRUSHSTYLE_SOLID));
                        gc->DrawRoundedRectangle(m_rtCheckbox.x, m_rtCheckbox.y, m_rtCheckbox.width, m_rtCheckbox.height, m_round);
                        gc->SetPen(wxPen(wxColour("#FFFFFF"), 2, wxPENSTYLE_SOLID));
                        wxPoint2DDouble points[3] = {{m_rtCheckbox.x + 3.007, m_rtCheckbox.y + 7.147},
                                                     {m_rtCheckbox.x + 6.441, m_rtCheckbox.y + 10.998},
                                                     {m_rtCheckbox.x + 12.996, m_rtCheckbox.y + 4.113}};
                        gc->DrawLines(3, points);
                    }
                    delete gc;
                }
            } else {
                if (m_state == 0) {
                    memDC.SetPen(wxPen(wxColour("#6E6E72"), 2, wxPENSTYLE_SOLID));
                } else if (m_state == 1) {
                    memDC.SetBrush(wxBrush(wxColour("#17CC5F"), wxBRUSHSTYLE_SOLID));
                }
                memDC.DrawRectangle(m_rtCheckbox);
            }

            //wxFont font(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
            //memDC.SetFont(font);
            if (m_state == 1) {
                memDC.SetTextForeground(wxColour("#17CC5F"));
            } else {
                memDC.SetTextForeground(wxColour("#FFFFFF"));
            }
            wxString text = m_label;
            wxPoint  textPos(m_rtCheckbox.GetRight() + FromDIP(5), FromDIP(5));
            int      maxWidth = width - textPos.x - 25;
            if (m_ellipsize) {
                wxSize textSize = memDC.GetTextExtent(text);
                if (textSize.GetWidth() > maxWidth) {
                    int len = text.Length();
                    while (memDC.GetTextExtent(text.Left(len)).GetWidth() > maxWidth) {
                        len--;
                    }
                    wxString ellipsisText = text.Left(len) + "...";

                    memDC.DrawText(ellipsisText, textPos);
                } else {
                    memDC.DrawText(text, textPos);
                }
            } else {
                memDC.DrawText(text, textPos);
            }
        }
        wxPaintDC dc(this);
        dc.Blit(0, 0, width, height, &memDC, 0, 0);
        memDC.SelectObject(wxNullBitmap);
    }
    void setItemCheckedCb(std::function<void(int)> funcItemCheckedCb) { m_funcItemCheckedCb = funcItemCheckedCb; }
    void setItemClickedCb(std::function<void()> funcItemClickedCb) { m_funcItemClickedCb = funcItemClickedCb; }
};

ExportMidPanel::ExportMidPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_SIMPLE)
{
    this->SetMinSize(wxSize(FromDIP(218), FromDIP(585)));
    this->SetBackgroundColour(wxColour("#4B4B4D"));
    //  创建头
    wxBoxSizer* bkBoxSizer = new wxBoxSizer(wxVERTICAL);
    m_bkBoxSizer           = bkBoxSizer;
    this->SetSizer(bkBoxSizer);

    wxBoxSizer* pSttSizer = new wxBoxSizer(wxHORIZONTAL);
    pSttSizer->SetMinSize(wxSize(FromDIP(124), FromDIP(48)));

    wxStaticText* pSttText = new wxStaticText(this, wxID_ANY, _L("Preset Param"), wxDefaultPosition, wxDefaultSize,
                                              wxALIGN_CENTER_HORIZONTAL);
    m_pSttText             = pSttText;
    pSttText->SetForegroundColour(wxColour("#FFFFFF"));
    pSttText->SetFont(Label::Body_14);
    pSttText->SetSize(wxSize(FromDIP(110), FromDIP(18)));
    pSttText->SetMinSize(wxSize(FromDIP(110), FromDIP(18)));
    pSttSizer->Add(pSttText, 0, wxLeft | wxALIGN_CENTER_VERTICAL, FromDIP(0));
    bkBoxSizer->Add(pSttSizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(0));

    wxScrolledWindow* scrolledWindow = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
    m_scrolledWindow                 = scrolledWindow;
    scrolledWindow->SetMinSize(wxSize(FromDIP(218), FromDIP(590)));
    scrolledWindow->SetBackgroundColour(wxColour("#4B4B4D"));
    scrolledWindow->SetScrollRate(5, 5);

    wxBoxSizer* sizer_body = new wxBoxSizer(wxVERTICAL);
    m_mainBoxSizer         = sizer_body;

    scrolledWindow->SetSizer(sizer_body);
    bkBoxSizer->Add(scrolledWindow);

}
ExportMidPanel::~ExportMidPanel() {}

bool ExportMidPanel::hasPrinterInit() { return m_stPrinterPresets.hasInit; }
bool ExportMidPanel::hasFilamentInit() { return m_stFilamentPresets.hasInit; }
bool ExportMidPanel::hasProcessInit() { return m_stProcessPresets.hasInit; }

void ExportMidPanel::updatePrinterPresets(const std::unordered_map<std::string, std::list<STLineDataNode*>>& mapPrinterPresets)
{
    m_index         = 0;
    size_t oldCount = m_mainBoxSizer->GetItemCount();
    for (size_t i = 0; i < oldCount; ++i) {
        m_mainBoxSizer->Hide(size_t(0));
        m_mainBoxSizer->Detach(0);
    }

    m_pSttText->SetLabelText(_L("Preset Param"));

    bool bFirstTreeLineDataNode = true;
    STTreeLineDataNode* pFirstTreeLineDataNode      = nullptr;
    if (!m_stPrinterPresets.hasInit) {
        m_stPrinterPresets.hasInit = true;
        for (const auto& item : mapPrinterPresets) {
            if (item.second.size() == 0)
                continue;
            wxString        name          = (item.first == "User Preset Param") ? _L("User Preset Param") : _L("System Preset Param");
            STTreeDataNode* pTreeDataNode = createTreeNode(name);

            for (auto item2 : item.second) {
                STTreeLineDataNode* pTreeLineDataNode = createCheckbox(item2);
                pTreeDataNode->sizer->Add(pTreeLineDataNode->sizer, 0, wxEXPAND | wxALL, FromDIP(0));
                pTreeDataNode->lstTreeLineDataNode.push_back(pTreeLineDataNode);
                m_rightPanel->updatePresets(pTreeLineDataNode);
                if (bFirstTreeLineDataNode) {
                    bFirstTreeLineDataNode = false;
                    pFirstTreeLineDataNode = pTreeLineDataNode;
                }
            }
            m_stPrinterPresets.lstPresets.push_back(pTreeDataNode);
            m_mainBoxSizer->Add(pTreeDataNode->sizer);
        }
    } else {
        for (const auto& item : m_stPrinterPresets.lstPresets) {
            size_t itemCount = item->sizer->GetItemCount();
            for (size_t i = 0; i < itemCount; ++i) {
                item->sizer->Show(size_t(i));
                if (bFirstTreeLineDataNode) {
                    bFirstTreeLineDataNode = false;
                    pFirstTreeLineDataNode = *(item->lstTreeLineDataNode.begin());
                }
            }
            m_mainBoxSizer->Add(item->sizer);
        }
    }
    if (pFirstTreeLineDataNode != nullptr) {
        if (m_stPrinterPresets.selectedLineDataNode != nullptr) {
            m_rightPanel->updatePresets(m_stPrinterPresets.selectedLineDataNode);
            m_stPrinterPresets.selectedLineDataNode->checkbox->SetSelected(true);
        } else {
            m_rightPanel->updatePresets(pFirstTreeLineDataNode);
            m_stPrinterPresets.selectedLineDataNode = pFirstTreeLineDataNode;
            m_stPrinterPresets.selectedLineDataNode->checkbox->SetSelected(true);
        }
    } else {
        STTreeLineDataNode node;
        m_rightPanel->updatePresets(&node);
    }
    m_mainBoxSizer->Layout();
    m_scrolledWindow->Layout();
    m_bkBoxSizer->Layout();
    updateCtrlSize();
    m_mainBoxSizer->Layout();
    m_scrolledWindow->Layout();
    m_bkBoxSizer->Layout();
}
void ExportMidPanel::updateFilamentPresets(const std::unordered_map<std::string, std::list<STLineDataNode*>>& mapFilamentPresets)
{
    m_index         = 1;
    size_t oldCount = m_mainBoxSizer->GetItemCount();
    for (size_t i = 0; i < oldCount; ++i) {
        m_mainBoxSizer->Hide(size_t(0));
        m_mainBoxSizer->Detach(0);
    }

    m_pSttText->SetLabelText(_L("Filament Name"));

    bool                bFirstTreeLineDataNode = true;
    STTreeLineDataNode* pFirstTreeLineDataNode = nullptr;
    if (!m_stFilamentPresets.hasInit) {
        m_stFilamentPresets.hasInit                = true;
        for (const auto& item : mapFilamentPresets) {
            if (item.second.size() == 0)
                continue;

            STTreeDataNode* pTreeDataNode = createTreeNode(item.first);

            for (auto item2 : item.second) {
                STTreeLineDataNode* pTreeLineDataNode = createCheckbox(item2);
                pTreeDataNode->sizer->Add(pTreeLineDataNode->sizer, 0, wxEXPAND | wxALL, FromDIP(0));
                pTreeDataNode->lstTreeLineDataNode.push_back(pTreeLineDataNode);
                m_rightPanel->updatePresets(pTreeLineDataNode);
                if (bFirstTreeLineDataNode) {
                    bFirstTreeLineDataNode = false;
                    pFirstTreeLineDataNode = pTreeLineDataNode;
                }
            }
            m_stFilamentPresets.lstPresets.push_back(pTreeDataNode);
            m_mainBoxSizer->Add(pTreeDataNode->sizer);
        }
    } else {
        for (const auto& item : m_stFilamentPresets.lstPresets) {
            size_t itemCount = item->sizer->GetItemCount();
            for (size_t i = 0; i < itemCount; ++i) {
                item->sizer->Show(size_t(i));
                if (bFirstTreeLineDataNode) {
                    bFirstTreeLineDataNode = false;
                    pFirstTreeLineDataNode = *(item->lstTreeLineDataNode.begin());
                }
            }
            m_mainBoxSizer->Add(item->sizer);
        }
    }
    if (pFirstTreeLineDataNode != nullptr) {
        if (m_stFilamentPresets.selectedLineDataNode != nullptr) {
            m_rightPanel->updatePresets(m_stFilamentPresets.selectedLineDataNode);
            m_stFilamentPresets.selectedLineDataNode->checkbox->SetSelected(true);
        } else {
            m_rightPanel->updatePresets(pFirstTreeLineDataNode);
            m_stFilamentPresets.selectedLineDataNode = pFirstTreeLineDataNode;
            m_stFilamentPresets.selectedLineDataNode->checkbox->SetSelected(true);
        }
    } else {
        STTreeLineDataNode node;
        m_rightPanel->updatePresets(&node);
    }
    m_mainBoxSizer->Layout();
    m_scrolledWindow->Layout();
    m_bkBoxSizer->Layout();
    updateCtrlSize();
    m_mainBoxSizer->Layout();
    m_scrolledWindow->Layout();
    m_bkBoxSizer->Layout();
}
void ExportMidPanel::updateProcessPresets(const std::unordered_map<std::string, std::list<STLineDataNode*>>& mapProcessPresets)
{
    m_index         = 2;
    size_t oldCount = m_mainBoxSizer->GetItemCount();
    for (size_t i = 0; i < oldCount; ++i) {
        m_mainBoxSizer->Hide(size_t(0));
        m_mainBoxSizer->Detach(0);
    }

    m_pSttText->SetLabelText(_L("Printer Type"));

    bool                bFirstTreeLineDataNode = true;
    STTreeLineDataNode* pFirstTreeLineDataNode = nullptr;
    if (!m_stProcessPresets.hasInit) {
        m_stProcessPresets.hasInit = true;
        for (const auto& item : mapProcessPresets) {
            if (item.second.size() == 0)
                continue;

            STTreeDataNode* pTreeDataNode = createTreeNode(item.first);
            pTreeDataNode->sizer->Hide(size_t(0));

            for (auto item2 : item.second) {
                STTreeLineDataNode* pTreeLineDataNode = createCheckbox(item2);
                pTreeDataNode->sizer->Add(pTreeLineDataNode->sizer, 0, wxEXPAND | wxALL, FromDIP(0));
                pTreeDataNode->lstTreeLineDataNode.push_back(pTreeLineDataNode);
                m_rightPanel->updatePresets(pTreeLineDataNode);
                if (bFirstTreeLineDataNode) {
                    bFirstTreeLineDataNode = false;
                    pFirstTreeLineDataNode = pTreeLineDataNode;
                }
            }
            m_stProcessPresets.lstPresets.push_back(pTreeDataNode);
            m_mainBoxSizer->Add(pTreeDataNode->sizer);
        }
    } else {
        for (const auto& item : m_stProcessPresets.lstPresets) {
            size_t itemCount = item->sizer->GetItemCount();
            for (size_t i = 0; i < itemCount; ++i) {
                if (i == 0) {
                    item->sizer->Hide(size_t(0));
                } else {
                    item->sizer->Show(size_t(i));
                }
                if (bFirstTreeLineDataNode) {
                    bFirstTreeLineDataNode = false;
                    pFirstTreeLineDataNode = *(item->lstTreeLineDataNode.begin());
                }
            }
            m_mainBoxSizer->Add(item->sizer);
        }
    }
    if (pFirstTreeLineDataNode != nullptr) {
        if (m_stProcessPresets.selectedLineDataNode != nullptr) {
            m_rightPanel->updatePresets(m_stProcessPresets.selectedLineDataNode);
            m_stProcessPresets.selectedLineDataNode->checkbox->SetSelected(true);
        } else {
            m_rightPanel->updatePresets(pFirstTreeLineDataNode);
            m_stProcessPresets.selectedLineDataNode = pFirstTreeLineDataNode;
            m_stProcessPresets.selectedLineDataNode->checkbox->SetSelected(true);
        }
    } else {
        STTreeLineDataNode node;
        m_rightPanel->updatePresets(&node);
    }
    m_mainBoxSizer->Layout();
    m_scrolledWindow->Layout();
    m_bkBoxSizer->Layout();
    updateCtrlSize();
    m_mainBoxSizer->Layout();
    m_scrolledWindow->Layout();
    m_bkBoxSizer->Layout();

}

void ExportMidPanel::getCheckedPrinterPresets(std::list<std::shared_ptr<ExportMidPanel::STLineDataNode>>& lstCheckedPrinterPresets) {
    for (auto item : m_stPrinterPresets.lstPresets) {
        for (auto item2 : item->lstTreeLineDataNode) {
            std::shared_ptr<STLineDataNode> spSTLineDataNode = std::make_shared<STLineDataNode>();
            spSTLineDataNode->name                           = item2->name.ToStdString();
            bool hasChecked                                  = false;
            for (auto item3 : item2->lstFilamentPresetParam) {
                if (item3->checkbox->GetValue()) {
                    spSTLineDataNode->lstFilamentPresetParam.push_back(item3->preset);
                    hasChecked = true;
                }
            }
            for (auto item3 : item2->lstProcessPresetParam) {
                if (item3->checkbox->GetValue()) {
                    spSTLineDataNode->lstProcessPresetParam.push_back(item3->preset);
                    hasChecked = true;
                }
            }
            for (auto item3 : item2->lstPrinterPresetParam) {
                if (item3->checkbox->GetValue()) {
                    spSTLineDataNode->lstPrinterPresetParam.push_back(item3->preset);
                    hasChecked = true;
                } else if (hasChecked && item3->preset->is_system) { // 如果是系统预设，则不显示，但进行导出
                    spSTLineDataNode->lstPrinterPresetParam.push_back(item3->preset);
                    hasChecked = true;
                }
            }
            if (hasChecked)
                lstCheckedPrinterPresets.push_back(spSTLineDataNode);
        }
    }
}
void ExportMidPanel::getCheckedFilamentPresets(std::list<std::shared_ptr<ExportMidPanel::STLineDataNode>>& lstCheckedFilamentPresets) {
    for (auto item : m_stFilamentPresets.lstPresets) {
        for (auto item2 : item->lstTreeLineDataNode) {
            std::shared_ptr<STLineDataNode> spSTLineDataNode = std::make_shared<STLineDataNode>();
            spSTLineDataNode->name                           = item2->name.ToStdString();
            bool hasChecked                                  = false;
            for (auto item3 : item2->lstPrinterPresetParam) {
                if (item3->checkbox->GetValue()) {
                    spSTLineDataNode->lstPrinterPresetParam.push_back(item3->preset);
                    hasChecked = true;
                }
            }
            for (auto item3 : item2->lstFilamentPresetParam) {
                if (item3->checkbox->GetValue()) {
                    spSTLineDataNode->lstFilamentPresetParam.push_back(item3->preset);
                    hasChecked = true;
                }
            }
            for (auto item3 : item2->lstProcessPresetParam) {
                if (item3->checkbox->GetValue()) {
                    spSTLineDataNode->lstProcessPresetParam.push_back(item3->preset);
                    hasChecked = true;
                }
            }
            if (hasChecked)
                lstCheckedFilamentPresets.push_back(spSTLineDataNode);
        }
    }
}
void ExportMidPanel::getCheckedProcessPresets(std::list<std::shared_ptr<ExportMidPanel::STLineDataNode>>& lstCheckedProcessPresets) {
    for (auto item : m_stProcessPresets.lstPresets) {
        for (auto item2 : item->lstTreeLineDataNode) {
            std::shared_ptr<STLineDataNode> spSTLineDataNode = std::make_shared<STLineDataNode>();
            spSTLineDataNode->name                           = item2->name.ToStdString();
            bool hasChecked                                  = false;
            for (auto item3 : item2->lstPrinterPresetParam) {
                if (item3->checkbox->GetValue()) {
                    spSTLineDataNode->lstPrinterPresetParam.push_back(item3->preset);
                    hasChecked = true;
                }
            }
            for (auto item3 : item2->lstFilamentPresetParam) {
                if (item3->checkbox->GetValue()) {
                    spSTLineDataNode->lstFilamentPresetParam.push_back(item3->preset);
                    hasChecked = true;
                }
            }
            for (auto item3 : item2->lstProcessPresetParam) {
                if (item3->checkbox->GetValue()) {
                    spSTLineDataNode->lstProcessPresetParam.push_back(item3->preset);
                    hasChecked = true;
                }
            }
            if (hasChecked)
                lstCheckedProcessPresets.push_back(spSTLineDataNode);
        }
    }
}

void ExportMidPanel::funcRightPanelItemCheckedCb(bool value)
{
    if (m_index == 0) {
        if (m_stPrinterPresets.hasInit) {
            bool bHasSelected = false;
            for (const auto& item : m_stPrinterPresets.lstPresets) {
                for (const auto& item2 : item->lstTreeLineDataNode) {
                    for (const auto& item3 : item2->lstPrinterPresetParam) {
                        if (value) {
                            //item3->checkbox->GetValue();
                            m_rightPanel->setExportBtnState(true);
                            return;
                        } else {
                            if (item3->checkbox->GetValue()) {
                                bHasSelected = true;
                                break;
                            }
                        }
                    }
                    if (bHasSelected) {
                        break;
                    }
                    for (const auto& item3 : item2->lstFilamentPresetParam) {
                        if (value) {
                            // item3->checkbox->GetValue();
                            m_rightPanel->setExportBtnState(true);
                            return;
                        } else {
                            if (item3->checkbox->GetValue()) {
                                bHasSelected = true;
                                break;
                            }
                        }
                    }
                    if (bHasSelected) {
                        break;
                    }
                    for (const auto& item3 : item2->lstProcessPresetParam) {
                        if (value) {
                            // item3->checkbox->GetValue();
                            m_rightPanel->setExportBtnState(true);
                            return;
                        } else {
                            if (item3->checkbox->GetValue()) {
                                bHasSelected = true;
                                break;
                            }
                        }
                    }
                    if (bHasSelected) {
                        break;
                    }
                }
                if (bHasSelected) {
                    break;
                }
            }
            if (bHasSelected) {
                m_rightPanel->setExportBtnState(true);
            } else {
                m_rightPanel->setExportBtnState(false);
            }
        }
    } else if (m_index == 1) {
        if (m_stFilamentPresets.hasInit) {
            bool bHasSelected = false;
            for (const auto& item : m_stFilamentPresets.lstPresets) {
                for (const auto& item2 : item->lstTreeLineDataNode) {
                    for (const auto& item3 : item2->lstPrinterPresetParam) {
                        if (value) {
                            // item3->checkbox->GetValue();
                            m_rightPanel->setExportBtnState(true);
                            return;
                        } else {
                            if (item3->checkbox->GetValue()) {
                                bHasSelected = true;
                                break;
                            }
                        }
                    }
                    if (bHasSelected) {
                        break;
                    }
                    for (const auto& item3 : item2->lstFilamentPresetParam) {
                        if (value) {
                            // item3->checkbox->GetValue();
                            m_rightPanel->setExportBtnState(true);
                            return;
                        } else {
                            if (item3->checkbox->GetValue()) {
                                bHasSelected = true;
                                break;
                            }
                        }
                    }
                    if (bHasSelected) {
                        break;
                    }
                    for (const auto& item3 : item2->lstProcessPresetParam) {
                        if (value) {
                            // item3->checkbox->GetValue();
                            m_rightPanel->setExportBtnState(true);
                            return;
                        } else {
                            if (item3->checkbox->GetValue()) {
                                bHasSelected = true;
                                break;
                            }
                        }
                    }
                    if (bHasSelected) {
                        break;
                    }
                }
                if (bHasSelected) {
                    break;
                }
            }
            if (bHasSelected) {
                m_rightPanel->setExportBtnState(true);
            } else {
                m_rightPanel->setExportBtnState(false);
            }
        }
    } else if (m_index == 2) {
        if (m_stProcessPresets.hasInit) {
            bool bHasSelected = false;
            for (const auto& item : m_stProcessPresets.lstPresets) {
                for (const auto& item2 : item->lstTreeLineDataNode) {
                    for (const auto& item3 : item2->lstPrinterPresetParam) {
                        if (value) {
                            // item3->checkbox->GetValue();
                            m_rightPanel->setExportBtnState(true);
                            return;
                        } else {
                            if (item3->checkbox->GetValue()) {
                                bHasSelected = true;
                                break;
                            }
                        }
                    }
                    if (bHasSelected) {
                        break;
                    }
                    for (const auto& item3 : item2->lstFilamentPresetParam) {
                        if (value) {
                            // item3->checkbox->GetValue();
                            m_rightPanel->setExportBtnState(true);
                            return;
                        } else {
                            if (item3->checkbox->GetValue()) {
                                bHasSelected = true;
                                break;
                            }
                        }
                    }
                    if (bHasSelected) {
                        break;
                    }
                    for (const auto& item3 : item2->lstProcessPresetParam) {
                        if (value) {
                            // item3->checkbox->GetValue();
                            m_rightPanel->setExportBtnState(true);
                            return;
                        } else {
                            if (item3->checkbox->GetValue()) {
                                bHasSelected = true;
                                break;
                            }
                        }
                    }
                    if (bHasSelected) {
                        break;
                    }
                }
                if (bHasSelected) {
                    break;
                }
            }
            if (bHasSelected) {
                m_rightPanel->setExportBtnState(true);
            } else {
                m_rightPanel->setExportBtnState(false);
            }
        }
    }
}

ExportMidPanel::STTreeDataNode* ExportMidPanel::createTreeNode(wxString nodeName)
{
    STTreeDataNode* stTreeDataNode = new STTreeDataNode;
    stTreeDataNode->name           = nodeName;
    wxBoxSizer* bk = new wxBoxSizer(wxVERTICAL);
    stTreeDataNode->sizer          = bk;
    wxPanel*    panel = new wxPanel(m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                 wxTAB_TRAVERSAL | wxBG_STYLE_COLOUR);
    stTreeDataNode->panel = panel;
    panel->SetMinSize(wxSize(FromDIP(216), FromDIP(28)));
    panel->SetBackgroundColour(wxColour("#3E3E40"));
    bk->Add(panel, 1, wxEXPAND | wxALL, FromDIP(0));
    //  创建头
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    panel->SetSizer(sizer);
    panel->Bind(wxEVT_LEFT_DOWN, [panel](wxMouseEvent&) {
        panel->SetBackgroundStyle(wxBG_STYLE_COLOUR);
        panel->SetBackgroundColour(wxColour("#18CC5C"));
    });
    ::CheckBox* checkbox = new ::CheckBox(panel);
    //checkbox->SetHalfChecked(true);
    checkbox->setItemClickedCb([checkbox, stTreeDataNode, this]() -> void {
        for (auto itemTreeLineDataNode : stTreeDataNode->lstTreeLineDataNode) {
            ExportMidPanel::STTreeLineDataNode* stTreeLineDataNode = itemTreeLineDataNode;
            //m_rightPanel->updatePresets(stTreeLineDataNode);
            stTreeLineDataNode->checkbox->SetValue(checkbox->GetValue());

            for (auto item : stTreeLineDataNode->lstPrinterPresetParam) {
                item->checkbox->SetValue(checkbox->GetValue());
            }
            for (auto item : stTreeLineDataNode->lstFilamentPresetParam) {
                item->checkbox->SetValue(checkbox->GetValue());
            }
            for (auto item : stTreeLineDataNode->lstProcessPresetParam) {
                item->checkbox->SetValue(checkbox->GetValue());
            }
        }
        funcRightPanelItemCheckedCb(checkbox->GetValue());
    });
    sizer->Add(checkbox, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 5);

    wxStaticText* preset_name_str = new wxStaticText(panel, wxID_ANY, nodeName);
    preset_name_str->Wrap(-1);
    preset_name_str->SetMinSize(wxSize(FromDIP(138), FromDIP(28)));
    preset_name_str->SetForegroundColour(wxColour("#FFFFFF"));
    wxToolTip* toolTip = new wxToolTip(nodeName);
    preset_name_str->SetToolTip(toolTip);
    sizer->Add(preset_name_str, 0, wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(4));
    wxButton* btnRight = new wxButton(panel, wxID_ANY, "^", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    btnRight->SetSize(wxSize(FromDIP(24), FromDIP(24)));
    btnRight->SetMinSize(wxSize(FromDIP(24), FromDIP(24)));
    btnRight->SetForegroundColour(wxColour("#FFFFFF"));
    btnRight->SetBackgroundColour(wxColour("#3E3E40"));
    btnRight->Bind(wxEVT_LEFT_DOWN, [stTreeDataNode, this](wxMouseEvent& e) {
        if (stTreeDataNode->isExpanded) {
            stTreeDataNode->isExpanded = false;
            size_t count              = stTreeDataNode->sizer->GetItemCount();
            for (size_t i = 1; i < count; ++i)
                stTreeDataNode->sizer->Hide(i);
        } else {
            stTreeDataNode->isExpanded = true;
            size_t count              = stTreeDataNode->sizer->GetItemCount();
            for (size_t i = 1; i < count; ++i)
                stTreeDataNode->sizer->Show(i);
        }
        m_mainBoxSizer->Layout();
        m_scrolledWindow->Layout();
        m_bkBoxSizer->Layout();
        updateCtrlSize();
        m_mainBoxSizer->Layout();
        m_scrolledWindow->Layout();
        m_bkBoxSizer->Layout();
    });
    sizer->Add(0, 0, 1, wxEXPAND, 5);
    sizer->Add(btnRight, 0, wxRight | wxALIGN_CENTER_VERTICAL, 0);

    return stTreeDataNode;
}

ExportMidPanel::STTreeLineDataNode* ExportMidPanel::createCheckbox(const STLineDataNode* pSTLineDataNode)
{
    STTreeLineDataNode* stTreeLineDataNode = new STTreeLineDataNode;
    stTreeLineDataNode->name               = pSTLineDataNode->name;
    wxBoxSizer*         sizer              = new wxBoxSizer(wxHORIZONTAL);
    stTreeLineDataNode->sizer              = sizer;
    sizer->SetMinSize(wxSize(FromDIP(124), FromDIP(28)));
    HoveableCheckbox* checkbox = new HoveableCheckbox(m_scrolledWindow, wxString::FromUTF8(pSTLineDataNode->name));
    checkbox->SetMinSize(wxSize(FromDIP(216), FromDIP(28)));
    stTreeLineDataNode->checkbox = checkbox;
    { /*
        for (auto item : pSTLineDataNode->lstPrinterPresetParam) {
            ExportCheckbox* pExportCheckbox = new ExportCheckbox(m_rightPanel->m_scrolledWindow, item);
            pExportCheckbox->Hide();
            stTreeLineDataNode->lstPrinterPresetParam.push_back(pExportCheckbox);
        }
        for (auto item : pSTLineDataNode->lstFilamentPresetParam) {
            ExportCheckbox* pExportCheckbox = new ExportCheckbox(m_rightPanel->m_scrolledWindow, item);
            stTreeLineDataNode->lstFilamentPresetParam.push_back(pExportCheckbox);
        }
        for (auto item : pSTLineDataNode->lstProcessPresetParam) {
            ExportCheckbox* pExportCheckbox = new ExportCheckbox(m_rightPanel->m_scrolledWindow, item);
            stTreeLineDataNode->lstProcessPresetParam.push_back(pExportCheckbox);
        }*/
        stTreeLineDataNode->data = *pSTLineDataNode;
     }

    checkbox->setItemCheckedCb([checkbox, stTreeLineDataNode, this](int state) -> void {
        m_rightPanel->updatePresets(stTreeLineDataNode);
        for (auto item : stTreeLineDataNode->lstPrinterPresetParam) {
            item->checkbox->SetValue(checkbox->GetValue());
        }
        for (auto item : stTreeLineDataNode->lstFilamentPresetParam) {
            item->checkbox->SetValue(checkbox->GetValue());
        }
        for (auto item : stTreeLineDataNode->lstProcessPresetParam) {
            item->checkbox->SetValue(checkbox->GetValue());
        }
         funcRightPanelItemCheckedCb(checkbox->GetValue());
    });
     checkbox->setItemClickedCb([stTreeLineDataNode, this]() -> void {
         stTreeLineDataNode->checkbox->SetSelected(true);
         if (m_index == 0) {
             if (m_stPrinterPresets.selectedLineDataNode == nullptr) {
                 m_stPrinterPresets.selectedLineDataNode = stTreeLineDataNode;
             } else {
                 m_stPrinterPresets.selectedLineDataNode->checkbox->SetSelected(false);
                 m_stPrinterPresets.selectedLineDataNode = stTreeLineDataNode;
             }
         } else if (m_index == 1) {
             if (m_stFilamentPresets.selectedLineDataNode == nullptr) {
                 m_stFilamentPresets.selectedLineDataNode = stTreeLineDataNode;
             } else {
                 m_stFilamentPresets.selectedLineDataNode->checkbox->SetSelected(false);
                 m_stFilamentPresets.selectedLineDataNode = stTreeLineDataNode;
             }
         } else if (m_index == 2) {
             if (m_stProcessPresets.selectedLineDataNode == nullptr) {
                 m_stProcessPresets.selectedLineDataNode = stTreeLineDataNode;
             } else {
                 m_stProcessPresets.selectedLineDataNode->checkbox->SetSelected(false);
                 m_stProcessPresets.selectedLineDataNode = stTreeLineDataNode;
             }
         }
     });
     checkbox->Bind(wxEVT_LEFT_DOWN, [stTreeLineDataNode, this](wxMouseEvent& e) {
         // if (m_funcItemClicked != nullptr)
         //   m_funcItemClicked(preset->name);
         m_rightPanel->updatePresets(stTreeLineDataNode);
        e.Skip();
     });
    sizer->Add(checkbox, 0, wxLEFT, FromDIP(0));
    return stTreeLineDataNode;
}

void ExportMidPanel::updateCtrlSize()
{
    if (m_index == 0) {
        if (m_scrolledWindow->HasScrollbar(wxVERTICAL)) {
            for (const auto& item : m_stPrinterPresets.lstPresets) {
                item->panel->SetMinSize(wxSize(FromDIP(198), FromDIP(28)));
                item->panel->SetMaxSize(wxSize(FromDIP(198), FromDIP(28)));
                for (auto item2 : item->lstTreeLineDataNode) {
                    item2->checkbox->SetMinSize(wxSize(FromDIP(198), FromDIP(28)));
                }
                item->sizer->Layout();
            }
        } else {
            for (const auto& item : m_stPrinterPresets.lstPresets) {
                item->panel->SetMinSize(wxSize(FromDIP(216), FromDIP(28)));
                item->panel->SetMaxSize(wxSize(FromDIP(216), FromDIP(28)));
                for (auto item2 : item->lstTreeLineDataNode) {
                    item2->checkbox->SetMinSize(wxSize(FromDIP(216), FromDIP(28)));
                }
                item->sizer->Layout();
            }
        }

    } else if (m_index == 1) {
        if (m_scrolledWindow->HasScrollbar(wxVERTICAL)) {
            for (const auto& item : m_stFilamentPresets.lstPresets) {
                item->panel->SetMinSize(wxSize(FromDIP(198), FromDIP(28)));
                item->panel->SetMaxSize(wxSize(FromDIP(198), FromDIP(28)));
                for (auto item2 : item->lstTreeLineDataNode) {
                    item2->checkbox->SetMinSize(wxSize(FromDIP(198), FromDIP(28)));
                }
                item->sizer->Layout();
            }
        } else {
            for (const auto& item : m_stFilamentPresets.lstPresets) {
                item->panel->SetMinSize(wxSize(FromDIP(216), FromDIP(28)));
                item->panel->SetMaxSize(wxSize(FromDIP(216), FromDIP(28)));
                for (auto item2 : item->lstTreeLineDataNode) {
                    item2->checkbox->SetMinSize(wxSize(FromDIP(216), FromDIP(28)));
                }
                item->sizer->Layout();
            }
        }
    } else if (m_index == 2) {
        if (m_scrolledWindow->HasScrollbar(wxVERTICAL)) {
            for (const auto& item : m_stProcessPresets.lstPresets) {
                item->panel->SetMinSize(wxSize(FromDIP(198), FromDIP(28)));
                item->panel->SetMaxSize(wxSize(FromDIP(198), FromDIP(28)));
                for (auto item2 : item->lstTreeLineDataNode) {
                    item2->checkbox->SetMinSize(wxSize(FromDIP(198), FromDIP(28)));
                }
                item->sizer->Layout();
            }
        } else {
            for (const auto& item : m_stProcessPresets.lstPresets) {
                item->panel->SetMinSize(wxSize(FromDIP(216), FromDIP(28)));
                item->panel->SetMaxSize(wxSize(FromDIP(216), FromDIP(28)));
                for (auto item2 : item->lstTreeLineDataNode) {
                    item2->checkbox->SetMinSize(wxSize(FromDIP(216), FromDIP(28)));
                }
                item->sizer->Layout();
            }
        }
    }
}


ExportRightPanel::ExportRightPanel(wxWindow* parent, wxString titleName)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_SIMPLE)
{
    this->SetMinSize(wxSize(FromDIP(534), FromDIP(585)));
    this->SetMaxSize(wxSize(FromDIP(534), FromDIP(585)));
    this->SetBackgroundColour(wxColour("#474749"));
    //  创建头
    wxBoxSizer* bkBoxSizer = new wxBoxSizer(wxVERTICAL);
    m_mainBoxSizer         = bkBoxSizer;
    this->SetSizer(bkBoxSizer);
    //  添加标题
    wxBoxSizer* pSttSizer = new wxBoxSizer(wxHORIZONTAL);
    pSttSizer->SetMinSize(wxSize(FromDIP(124), FromDIP(48)));

    wxStaticText* pSttText = new wxStaticText(this, wxID_ANY, titleName, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
    pSttText->SetForegroundColour(wxColour("#FFFFFF"));
    pSttText->SetFont(Label::Body_14);
    pSttText->SetSize(wxSize(FromDIP(110), FromDIP(18)));
    pSttText->SetMinSize(wxSize(FromDIP(110), FromDIP(18)));
    pSttSizer->Add(pSttText, 0, wxLeft | wxALIGN_CENTER_VERTICAL, FromDIP(0));
    bkBoxSizer->Add(pSttSizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(0));

    wxScrolledWindow* scrolledWindow = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
    m_scrolledWindow                 = scrolledWindow;
    scrolledWindow->SetMinSize(wxSize(FromDIP(534), FromDIP(480)));
    scrolledWindow->SetBackgroundColour(wxColour("#4B4B4D"));
    scrolledWindow->SetScrollRate(5, 5);

    wxBoxSizer* sizer_body = new wxBoxSizer(wxVERTICAL);
    m_midDataBoxSizer      = sizer_body;

    //  创建打印机预设参数
    {
        wxPanel*    panel = new wxPanel(m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_SIMPLE);
        m_stPrinterPresetParam.panel       = panel;
        panel->SetSize(wxSize(FromDIP(140), FromDIP(28)));
        panel->SetMinSize(wxSize(FromDIP(140), FromDIP(28)));
        panel->SetBackgroundColour(wxColour("#474749"));
        wxBoxSizer* panelSizer = new wxBoxSizer(wxHORIZONTAL); 
        m_stPrinterPresetParam.mainBoxSize = panelSizer;
        panel->SetSizer(panelSizer);
        
        wxStaticText* stt = new wxStaticText(panel, wxID_ANY, _L("Printer Preset Param"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
        stt->SetFont(Label::Body_12);
        stt->SetSize(wxSize(FromDIP(140), FromDIP(28)));
        stt->SetMinSize(wxSize(FromDIP(140), FromDIP(28)));
        stt->SetForegroundColour(wxColour("#FFFFFF"));
        stt->SetBackgroundColour(wxColour("#474749"));
        m_stPrinterPresetParam.mainBoxSize->Add(stt, 0, wxALIGN_CENTER | wxALL, FromDIP(5));

        m_stPrinterPresetParam.contentBoxSize = new wxBoxSizer(wxVERTICAL);
        m_stPrinterPresetParam.mainBoxSize->Add(m_stPrinterPresetParam.contentBoxSize);
        m_midDataBoxSizer->Add(panel, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(5));
    }
    //  创建耗材预设参数
    {
        wxPanel* panel = new wxPanel(m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_SIMPLE);
        m_stFilamentPresetParam.panel = panel;
        panel->SetSize(wxSize(FromDIP(140), FromDIP(28)));
        panel->SetMinSize(wxSize(FromDIP(140), FromDIP(28)));
        panel->SetBackgroundColour(wxColour("#474749"));
        wxBoxSizer* panelSizer             = new wxBoxSizer(wxHORIZONTAL);
        m_stFilamentPresetParam.mainBoxSize = panelSizer;
        panel->SetSizer(panelSizer);

        wxStaticText* stt = new wxStaticText(panel, wxID_ANY, _L("Filament Preset Param"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
        stt->SetFont(Label::Body_12);
        stt->SetSize(wxSize(FromDIP(140), FromDIP(28)));
        stt->SetMinSize(wxSize(FromDIP(140), FromDIP(28)));
        stt->SetForegroundColour(wxColour("#FFFFFF"));
        stt->SetBackgroundColour(wxColour("#474749"));
        m_stFilamentPresetParam.mainBoxSize->Add(stt, 0, wxALIGN_CENTER|wxALL, FromDIP(5));

        m_stFilamentPresetParam.contentBoxSize = new wxBoxSizer(wxVERTICAL);
        m_stFilamentPresetParam.mainBoxSize->Add(m_stFilamentPresetParam.contentBoxSize);
        m_midDataBoxSizer->Add(panel, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(5));
    }
    //  创建工艺预设参数
    {
        wxPanel* panel = new wxPanel(m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_SIMPLE);
        m_stProcessPresetParam.panel = panel;
        panel->SetSize(wxSize(FromDIP(140), FromDIP(28)));
        panel->SetMinSize(wxSize(FromDIP(140), FromDIP(28)));
        panel->SetBackgroundColour(wxColour("#474749"));
        wxBoxSizer* panelSizer              = new wxBoxSizer(wxHORIZONTAL);
        m_stProcessPresetParam.mainBoxSize = panelSizer;
        panel->SetSizer(panelSizer);

        wxStaticText* stt = new wxStaticText(panel, wxID_ANY, _L("Process Preset Param"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
        stt->SetFont(Label::Body_12);
        stt->SetSize(wxSize(FromDIP(140), FromDIP(28)));
        stt->SetMinSize(wxSize(FromDIP(140), FromDIP(28)));
        stt->SetForegroundColour(wxColour("#FFFFFF"));
        stt->SetBackgroundColour(wxColour("#474749"));
        m_stProcessPresetParam.mainBoxSize->Add(stt, 0, wxALIGN_CENTER | wxALL, FromDIP(5));

        m_stProcessPresetParam.contentBoxSize = new wxBoxSizer(wxVERTICAL);
        m_stProcessPresetParam.mainBoxSize->Add(m_stProcessPresetParam.contentBoxSize);
        m_midDataBoxSizer->Add(panel, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(5));
    }

    scrolledWindow->SetSizer(sizer_body);
    bkBoxSizer->Add(scrolledWindow);

    //  添加底部按钮
    wxBoxSizer* pBottomBox = new wxBoxSizer(wxHORIZONTAL);
    pBottomBox->Add(0, 0, 1, wxEXPAND, 5);
    wxButton*   pBtnExport = new wxButton(this, wxID_ANY, _L("Export"), wxDefaultPosition, wxDefaultSize, wxLeft | wxBORDER_NONE);
    m_pBtnExport         = pBtnExport;
    pBtnExport->SetFont(Label::Body_12);
    pBtnExport->SetSize(wxSize(FromDIP(128), FromDIP(32)));
    pBtnExport->SetMinSize(wxSize(FromDIP(128), FromDIP(32)));
    pBtnExport->SetForegroundColour(wxColour("#FFFFFF"));
    //pBtnExport->SetBackgroundColour(wxColour("#18CC5C"));
    pBtnExport->SetBackgroundColour(wxColour("#6E6E72"));
    pBtnExport->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& e) {
        if (!m_bCanExport)
            return;
        if (m_funcExportBtnClicked)
            m_funcExportBtnClicked();
    });
    pBottomBox->Add(pBtnExport, 0, wxEXPAND | wxALL, FromDIP(9));
    wxButton* pBtnCalcel = new wxButton(this, wxID_ANY, _L("Cancel"), wxDefaultPosition, wxDefaultSize, wxLeft | wxBORDER_NONE);
    m_pBtnCalcel         = pBtnCalcel;
    pBtnCalcel->SetFont(Label::Body_12);
    pBtnCalcel->SetSize(wxSize(FromDIP(128), FromDIP(32)));
    pBtnCalcel->SetMinSize(wxSize(FromDIP(128), FromDIP(32)));
    pBtnCalcel->SetForegroundColour(wxColour("#FFFFFF"));
    pBtnCalcel->SetBackgroundColour(wxColour("#6E6E72"));
    pBtnCalcel->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& e) {
        if (m_funcCancleBtnClicked != nullptr)
            m_funcCancleBtnClicked();
    });
    pBottomBox->Add(pBtnCalcel, 0, wxEXPAND | wxALL, FromDIP(9));
    pBottomBox->Add(0, 0, 1, wxEXPAND, 5);

    bkBoxSizer->Add(pBottomBox, 0, wxALIGN_CENTER_HORIZONTAL, FromDIP(0));
}

void ExportRightPanel::updatePresets(ExportMidPanel::STTreeLineDataNode* pLineDataNode)
{
    if (!pLineDataNode->hasCreateCtl) {
        pLineDataNode->hasCreateCtl = true;
        for (auto item : pLineDataNode->data.lstPrinterPresetParam)
        {
            ExportCheckbox* pExportCheckbox = new ExportCheckbox(m_stPrinterPresetParam.panel, item);
            pLineDataNode->lstPrinterPresetParam.push_back(pExportCheckbox);
            pExportCheckbox->checkbox->setItemClickedCb([this, pExportCheckbox]() {
                if (m_funcCheckboxClickedCb)
                    m_funcCheckboxClickedCb(pExportCheckbox->checkbox->GetValue());
                });
        }
        for (auto item : pLineDataNode->data.lstFilamentPresetParam) {
            ExportCheckbox* pExportCheckbox = new ExportCheckbox(m_stFilamentPresetParam.panel, item);
            pLineDataNode->lstFilamentPresetParam.push_back(pExportCheckbox);
            pExportCheckbox->checkbox->setItemClickedCb([this, pExportCheckbox]() {
                if (m_funcCheckboxClickedCb)
                    m_funcCheckboxClickedCb(pExportCheckbox->checkbox->GetValue());
            });
        }
        for (auto item : pLineDataNode->data.lstProcessPresetParam) {
            ExportCheckbox* pExportCheckbox = new ExportCheckbox(m_stProcessPresetParam.panel, item);
            pLineDataNode->lstProcessPresetParam.push_back(pExportCheckbox);
            pExportCheckbox->checkbox->setItemClickedCb([this, pExportCheckbox]() {
                if (m_funcCheckboxClickedCb)
                    m_funcCheckboxClickedCb(pExportCheckbox->checkbox->GetValue());
            });
        }
    }

    // printer
    size_t oldCount = m_stPrinterPresetParam.contentBoxSize->GetItemCount();
    for (size_t i = 0; i < oldCount; ++i) {
        m_stPrinterPresetParam.contentBoxSize->Hide(size_t(0));
        m_stPrinterPresetParam.contentBoxSize->Detach(0);
    }
    int index = 0;
    for (auto item : pLineDataNode->lstPrinterPresetParam) {
        m_stPrinterPresetParam.contentBoxSize->Add(item);
        if (item->preset->is_system) {// 如果是系统预设，则不显示，但进行导出
            m_stPrinterPresetParam.contentBoxSize->Hide(size_t(index));
            m_stPrinterPresetParam.contentBoxSize->Detach(index);
            continue;
        }
        index++;
    }
    m_stPrinterPresetParam.contentBoxSize->ShowItems(true);
    size_t count = m_stPrinterPresetParam.contentBoxSize->GetItemCount();
    if (count > 0) {
        m_stPrinterPresetParam.panel->SetMinSize(wxSize(FromDIP(140), FromDIP(28 * count)));
        m_midDataBoxSizer->Show(size_t(0));
    } else
        m_midDataBoxSizer->Hide(size_t(0));

    //  filament
    oldCount = m_stFilamentPresetParam.contentBoxSize->GetItemCount();
    for (size_t i = 0; i < oldCount; ++i) {
        m_stFilamentPresetParam.contentBoxSize->Hide(size_t(0));
        m_stFilamentPresetParam.contentBoxSize->Detach(0);
    }
    for (const auto& item : pLineDataNode->lstFilamentPresetParam) {
        m_stFilamentPresetParam.contentBoxSize->Add(item);
    }
    m_stFilamentPresetParam.contentBoxSize->ShowItems(true);
    count = m_stFilamentPresetParam.contentBoxSize->GetItemCount();
    if (count > 0) {
        m_stFilamentPresetParam.panel->SetMinSize(wxSize(FromDIP(140), FromDIP(28 * count)));
        m_midDataBoxSizer->Show(size_t(1));
    } else
        m_midDataBoxSizer->Hide(size_t(1));

    // process
    oldCount = m_stProcessPresetParam.contentBoxSize->GetItemCount();
    for (size_t i = 0; i < oldCount; ++i) {
        m_stProcessPresetParam.contentBoxSize->Hide(size_t(0));
        m_stProcessPresetParam.contentBoxSize->Detach(0);
    }
    for (const auto& item : pLineDataNode->lstProcessPresetParam) {
        m_stProcessPresetParam.contentBoxSize->Add(item);
    }
    m_stProcessPresetParam.contentBoxSize->ShowItems(true);
    count = m_stProcessPresetParam.contentBoxSize->GetItemCount();
    if (count > 0) {
        m_stProcessPresetParam.panel->SetMinSize(wxSize(FromDIP(140), FromDIP(28 * count)));
        m_midDataBoxSizer->Show(size_t(2));
    } else
        m_midDataBoxSizer->Hide(size_t(2));

    m_midDataBoxSizer->Layout();
    m_mainBoxSizer->Layout();
}
void ExportRightPanel::setExportBtnClicked(std::function<void()> funcExportBtnClicked) { m_funcExportBtnClicked = funcExportBtnClicked; }
void ExportRightPanel::setCancelBtnClicked(std::function<void()> funcCancelBtnClicked) { m_funcCancleBtnClicked = funcCancelBtnClicked; }

void ExportRightPanel::setCheckboxClickedCb(std::function<void(bool)> funcCheckboxClickedCb) {
    m_funcCheckboxClickedCb = funcCheckboxClickedCb;
}

void ExportRightPanel::setExportBtnState(bool bCanExport)
{
    if (bCanExport) {
        m_bCanExport = true;
        m_pBtnExport->SetBackgroundColour(wxColour("#18CC5C"));

    } else {
        m_bCanExport = false;
        m_pBtnExport->SetBackgroundColour(wxColour("#6E6E72"));
    }
}

ExportConfigsDialog::ExportConfigsDialog(wxWindow* parent, int type)
    : DPIDialog(parent ? parent : nullptr, wxID_ANY, _L("Export Preset Bundle"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX)
{
    data_init();
}

ExportConfigsDialog::ExportConfigsDialog(wxWindow* parent, const wxString& export_type)
    : DPIDialog(parent ? parent : nullptr, wxID_ANY, _L("Export Preset Bundle"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX)
    , m_cur_export_type(export_type)
{
    m_exprot_type.preset_bundle   = _L("Printer config bundle(.creality_printer)");
    m_exprot_type.filament_bundle = _L("Filament bundle(.creality_filament)");
    m_exprot_type.printer_preset  = _L("Printer presets(.zip)");
    m_exprot_type.filament_preset = _L("Filament presets(.zip)");
    m_exprot_type.process_preset  = _L("Process presets(.zip)");

    this->SetBackgroundColour(*wxWHITE);
    this->SetSize(wxSize(FromDIP(600), FromDIP(600)));

    std::string icon_path = (boost::format("%1%/images/Creative3DTitle.ico") % resources_dir()).str();
    SetIcon(wxIcon(encode_path(icon_path.c_str()), wxBITMAP_TYPE_ICO));

    m_main_sizer = new wxBoxSizer(wxVERTICAL);
    // top line
    auto m_line_top = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 1), wxTAB_TRAVERSAL);
    m_line_top->SetBackgroundColour(wxColour(0xA6, 0xa9, 0xAA));
    m_main_sizer->Add(m_line_top, 0, wxEXPAND, 0);
    m_main_sizer->Add(0, 0, 0, wxTOP, FromDIP(5));
    if (export_type.empty()) {
        m_main_sizer->Add(create_export_config_item(this), 0, wxEXPAND | wxALL, FromDIP(5));
    }
    m_main_sizer->Add(create_select_printer(this), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));
    m_main_sizer->Add(create_button_item(this), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));

    data_init();

    this->SetSizer(m_main_sizer);

    this->Layout();
    this->Fit();

    wxGetApp().UpdateDlgDarkUI(this);
    if (!export_type.empty()) {
        set_export_type(export_type);
    }
}

ExportConfigsDialog::~ExportConfigsDialog()
{
    for (std::pair<std::string, Preset *> printer_preset : m_printer_presets) {
        Preset *preset = printer_preset.second;
        if (preset) {
            delete preset;
            preset = nullptr;
        }
    }
    for (std::pair<std::string, std::vector<Preset *>> filament_presets : m_filament_presets) {
        for (Preset* preset : filament_presets.second) {
            if (preset) {
                delete preset;
                preset = nullptr;
            }
        }
    }
    for (std::pair<std::string, std::vector<Preset *>> filament_presets : m_process_presets) {
        for (Preset *preset : filament_presets.second) {
            if (preset) {
                delete preset;
                preset = nullptr;
            }
        }
    }
    for (std::pair<std::string, std::vector<std::pair<std::string, Preset *>>> filament_preset : m_filament_name_to_presets) {
        for (std::pair<std::string, Preset*> printer_name_preset : filament_preset.second) {
            Preset *preset = printer_name_preset.second;
            if (preset) {
                delete preset;
                preset = nullptr;
            }
        }
    }

    // Delete the Temp folder
    boost::filesystem::path temp_folder(data_dir() + "/" + PRESET_USER_DIR + "/" + "Temp");
    if (boost::filesystem::exists(temp_folder)) boost::filesystem::remove_all(temp_folder);
}

void ExportConfigsDialog::exportPresets()
{
    this->SetBackgroundColour(*wxWHITE);
    wxGetApp().UpdateDlgDarkUI(this);
    this->SetMinSize(wxSize(FromDIP(992), FromDIP(660)));
    this->SetMaxSize(wxSize(FromDIP(992), FromDIP(660)));

    std::string icon_path = (boost::format("%1%/images/Creative3DTitle.ico") % resources_dir()).str();
    SetIcon(wxIcon(encode_path(icon_path.c_str()), wxBITMAP_TYPE_ICO));

    m_main_sizer = new wxBoxSizer(wxHORIZONTAL);

    m_main_sizer->Add(create_left_navigation(this));
    m_main_sizer->Add(createRightContent(this));
    //m_main_sizer->Add(createMidPresets(this));
    //m_main_sizer->Add(create_right_export_context(this));


    this->SetSizer(m_main_sizer);

    this->Layout();
    this->Fit();


    onBtnFilamentClicked();
    onBtnProcessClicked();
    onBtnPrinterClicked();
    m_stPrinterPresets.bClicked = true;
    m_stPrinterPresets.pBtnPresets->SetForegroundColour(wxColour("#FFFFFF"));
    m_stPrinterPresets.pBtnPresets->SetBackgroundColour(wxColour("#18CC5C"));

    this->Center();
    this->ShowModal();
}

void ExportConfigsDialog::on_dpi_changed(const wxRect &suggested_rect) {
    m_button_ok->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_ok->SetMaxSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_ok->SetCornerRadius(FromDIP(12));
    m_button_cancel->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_cancel->SetMaxSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_cancel->SetCornerRadius(FromDIP(12));
    Layout();
}

void ExportConfigsDialog::show_export_result(const ExportCase &export_case)
{
    MessageDialog *msg_dlg = nullptr;
    switch (export_case) {
    case ExportCase::INITIALIZE_FAIL:
        msg_dlg = new MessageDialog(this, _L("initialize fail"), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES | wxYES_DEFAULT | wxCENTRE);
        break;
    case ExportCase::ADD_FILE_FAIL:
        msg_dlg = new MessageDialog(this, _L("add file fail"), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES | wxYES_DEFAULT | wxCENTRE);
        break;
    case ExportCase::ADD_BUNDLE_STRUCTURE_FAIL:
        msg_dlg = new MessageDialog(this, _L("add bundle structure file fail"), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES | wxYES_DEFAULT | wxCENTRE);
        break;
    case ExportCase::FINALIZE_FAIL:
        msg_dlg = new MessageDialog(this, _L("finalize fail"), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES | wxYES_DEFAULT | wxCENTRE);
        break;
    case ExportCase::OPEN_ZIP_WRITTEN_FILE:
        msg_dlg = new MessageDialog(this, _L("open zip written fail"), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES | wxYES_DEFAULT | wxCENTRE);
        break;
    case ExportCase::EXPORT_SUCCESS:
        msg_dlg = new MessageDialog(this, _L("Export successful"), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES | wxYES_DEFAULT | wxCENTRE);
        break;
    }
    
    if (msg_dlg) {
        msg_dlg->ShowModal();
        delete msg_dlg;
        msg_dlg = nullptr;
    }
}

bool ExportConfigsDialog::has_check_box_selected()
{
    for (std::pair<::CheckBox *, Preset *> checkbox_preset : m_preset) {
        if (checkbox_preset.first->GetValue()) return true;
    }
    for (std::pair<::CheckBox *, std::string> checkbox_filament_name : m_printer_name) {
        if (checkbox_filament_name.first->GetValue()) return true;
    }

    return false;
}

bool ExportConfigsDialog::earse_preset_fields_for_safe(Preset *preset)
{ 
    if (preset->type != Preset::Type::TYPE_PRINTER) return true;
    
    boost::filesystem::path file_path(data_dir() + "/" + PRESET_USER_DIR + "/" + "Temp" + "/" + (preset->name + ".json"));
    preset->file = file_path.make_preferred().string();

    DynamicPrintConfig &config = preset->config;
    config.erase("print_host");
    config.erase("print_host_webui");
    config.erase("printhost_apikey");
    config.erase("printhost_cafile");
    config.erase("printhost_user");
    config.erase("printhost_password");
    config.erase("printhost_port");

    preset->save(nullptr);
    return true; 
}

std::string ExportConfigsDialog::initial_file_path(const wxString &path, const std::string &sub_file_path)
{
    std::string             export_path         = into_u8(path);
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "initial file path and path is:" << export_path << " and sub path is: " << sub_file_path;
    boost::filesystem::path printer_export_path = (boost::filesystem::path(export_path) / sub_file_path).make_preferred();
    if (!boost::filesystem::exists(printer_export_path)) {
        boost::filesystem::create_directories(printer_export_path);
        export_path = printer_export_path.string();
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "Same path exists, delete and rebuild, and path is: " << export_path;
    }
    return export_path;
}

std::string ExportConfigsDialog::initial_file_name(const wxString &path, const std::string file_name)
{
    std::string             export_path         = into_u8(path);
    boost::filesystem::path printer_export_path = (boost::filesystem::path(export_path) / file_name).make_preferred();
    if (boost::filesystem::exists(printer_export_path)) {
        MessageDialog dlg(this, wxString::Format(_L("The '%s' folder already exists in the current directory. Do you want to clear it and rebuild it.\nIf not, a time suffix will be "
                             "added, and you can modify the name after creation."), file_name), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES_NO | wxYES_DEFAULT | wxCENTRE);
        int           res = dlg.ShowModal();
        if (wxID_YES == res) {
            try {
                boost::filesystem::remove_all(printer_export_path);
            }
            catch(...) {
                MessageDialog dlg(this,
                                  _L(wxString::Format("The file: %s \nmay have been opened by another program. \nPlease close it and try again.",
                                                      encode_path(printer_export_path.string().c_str()))),
                                  wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES | wxYES_DEFAULT | wxCENTRE);
                dlg.ShowModal();
                return "initial_failed";
            }
            export_path = printer_export_path.string();
        } else if (wxID_NO == res) {
            export_path = printer_export_path.string();
            export_path = export_path.substr(0, export_path.find(".zip"));
            std::string              export_path_with_time;
            boost::filesystem::path *printer_export_path_with_time = nullptr;
            do {
                if (printer_export_path_with_time) {
                    delete printer_export_path_with_time;
                    printer_export_path_with_time = nullptr;
                }
                export_path_with_time         = export_path + " " + get_curr_time() + ".zip";
                printer_export_path_with_time = new boost::filesystem::path(export_path_with_time);
            } while (boost::filesystem::exists(*printer_export_path_with_time));
            export_path = export_path_with_time;
            if (printer_export_path_with_time) {
                delete printer_export_path_with_time;
                printer_export_path_with_time = nullptr;
            }
        } else {
            return "";
        }
    } else {
        export_path = printer_export_path.string();
    }
    return export_path;
}

wxBoxSizer *ExportConfigsDialog::create_export_config_item(wxWindow *parent)
{
    wxBoxSizer *horizontal_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxBoxSizer *  optionSizer        = new wxBoxSizer(wxVERTICAL);
    wxStaticText *static_serial_text = new wxStaticText(parent, wxID_ANY, _L("Presets"), wxDefaultPosition, wxDefaultSize);
    optionSizer->Add(static_serial_text, 0, wxEXPAND | wxALL, 0);
    optionSizer->SetMinSize(OPTION_SIZE);
    horizontal_sizer->Add(optionSizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    wxBoxSizer *radioBoxSizer = new wxBoxSizer(wxVERTICAL);

    radioBoxSizer->Add(create_radio_item(m_exprot_type.preset_bundle, parent, wxEmptyString, m_export_type_btns), 0, wxEXPAND | wxALL, 0);
    radioBoxSizer->Add(0, 0, 0, wxTOP, FromDIP(6));
    wxStaticText *static_export_printer_preset_bundle_text = new wxStaticText(parent, wxID_ANY, _L("Printer and all the filament&&process presets that belongs to the printer. \nCan be shared with others."), wxDefaultPosition, wxDefaultSize);
    static_export_printer_preset_bundle_text->SetFont(Label::Body_12);
    static_export_printer_preset_bundle_text->SetForegroundColour(wxColour("#6B6B6B"));
    radioBoxSizer->Add(static_export_printer_preset_bundle_text, 0, wxEXPAND | wxLEFT, FromDIP(22));
    radioBoxSizer->Add(create_radio_item(m_exprot_type.filament_bundle, parent, wxEmptyString, m_export_type_btns), 0, wxEXPAND | wxTOP, FromDIP(10));
    wxStaticText *static_export_filament_preset_bundle_text = new wxStaticText(parent, wxID_ANY, _L("User's fillment preset set. \nCan be shared with others."), wxDefaultPosition, wxDefaultSize);
    static_export_filament_preset_bundle_text->SetFont(Label::Body_12);
    static_export_filament_preset_bundle_text->SetForegroundColour(wxColour("#6B6B6B"));
    radioBoxSizer->Add(static_export_filament_preset_bundle_text, 0, wxEXPAND | wxLEFT, FromDIP(22));
    radioBoxSizer->Add(create_radio_item(m_exprot_type.printer_preset, parent, wxEmptyString, m_export_type_btns), 0, wxEXPAND | wxTOP, FromDIP(10));
    radioBoxSizer->Add(create_radio_item(m_exprot_type.filament_preset, parent, wxEmptyString, m_export_type_btns), 0, wxEXPAND | wxTOP, FromDIP(10));
    radioBoxSizer->Add(create_radio_item(m_exprot_type.process_preset, parent, wxEmptyString, m_export_type_btns), 0, wxEXPAND | wxTOP, FromDIP(10));
    horizontal_sizer->Add(radioBoxSizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    return horizontal_sizer;
}

wxBoxSizer *ExportConfigsDialog::create_radio_item(wxString title, wxWindow *parent, wxString tooltip, std::vector<std::pair<RadioBox *, wxString>> &radiobox_list)
{
    wxBoxSizer *horizontal_sizer = new wxBoxSizer(wxHORIZONTAL);
    RadioBox *  radiobox         = new RadioBox(parent);
    horizontal_sizer->Add(radiobox, 0, wxEXPAND | wxALL, 0);
    horizontal_sizer->Add(0, 0, 0, wxEXPAND | wxLEFT, FromDIP(5));
    radiobox_list.push_back(std::make_pair(radiobox, title));
    int btn_idx = radiobox_list.size() - 1;
    radiobox->Bind(wxEVT_LEFT_DOWN, [this, &radiobox_list, btn_idx](wxMouseEvent &e) {
        select_curr_radiobox(radiobox_list, btn_idx);
        });

    wxStaticText *text = new wxStaticText(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize);
    text->Bind(wxEVT_LEFT_DOWN, [this, &radiobox_list, btn_idx](wxMouseEvent &e) {
        select_curr_radiobox(radiobox_list, btn_idx);
        });
    horizontal_sizer->Add(text, 0, wxEXPAND | wxLEFT, 0);

    radiobox->SetToolTip(tooltip);
    text->SetToolTip(tooltip);
    return horizontal_sizer;
}

mz_bool ExportConfigsDialog::initial_zip_archive(mz_zip_archive &zip_archive, const std::string &file_path)
{
    mz_zip_zero_struct(&zip_archive);
    mz_bool status;

    // Initialize the ZIP file to write to the structure, using memory storage

    std::string export_dir = encode_path(file_path.c_str());
    status                 = mz_zip_writer_init_file(&zip_archive, export_dir.c_str(), 0);
    return status;
}

ExportConfigsDialog::ExportCase ExportConfigsDialog::save_zip_archive_to_file(mz_zip_archive &zip_archive)
{
    // Complete writing of ZIP file
    mz_bool status = mz_zip_writer_finalize_archive(&zip_archive);
    if (MZ_FALSE == status) {
        BOOST_LOG_TRIVIAL(info) << "Failed to finalize ZIP archive";
        mz_zip_writer_end(&zip_archive);
        return ExportCase::FINALIZE_FAIL;
    }

    // Release ZIP file to write structure and related resources
    mz_zip_writer_end(&zip_archive);

    return ExportCase::CASE_COUNT;
}

ExportConfigsDialog::ExportCase ExportConfigsDialog::save_presets_to_zip(const std::string &export_file, const std::vector<std::pair<std::string, std::string>> &config_paths)
{
    mz_zip_archive zip_archive;
    mz_bool        status = initial_zip_archive(zip_archive, export_file);

    if (MZ_FALSE == status) {
        BOOST_LOG_TRIVIAL(info) << "Failed to initialize ZIP archive";
        return ExportCase::INITIALIZE_FAIL;
    }

    for (std::pair<std::string, std::string> config_path : config_paths) {
        std::string preset_name = config_path.first;

        // Add a file to the ZIP file
        status = mz_zip_writer_add_file(&zip_archive, (preset_name).c_str(), encode_path(config_path.second.c_str()).c_str(), NULL, 0, MZ_DEFAULT_COMPRESSION);
        // status = mz_zip_writer_add_mem(&zip_archive, ("printer/" + printer_preset->name + ".json").c_str(), json_contents, strlen(json_contents), MZ_DEFAULT_COMPRESSION);
        if (MZ_FALSE == status) {
            BOOST_LOG_TRIVIAL(info) << preset_name << " Filament preset failed to add file to ZIP archive";
            mz_zip_writer_end(&zip_archive);
            return ExportCase::ADD_FILE_FAIL;
        }
        BOOST_LOG_TRIVIAL(info) << "Printer preset json add successful: " << preset_name;
    }
    return save_zip_archive_to_file(zip_archive);
}

void ExportConfigsDialog::select_curr_radiobox(std::vector<std::pair<RadioBox *, wxString>> &radiobox_list, int btn_idx)
{
    int len = radiobox_list.size();
    for (int i = 0; i < len; ++i) {
        if (i == btn_idx) {
            radiobox_list[i].first->SetValue(true);
            const wxString &export_type = radiobox_list[i].second;
            m_preset_sizer->Clear(true);
            m_printer_name.clear();
            m_preset.clear();
            PresetBundle *preset_bundle = wxGetApp().preset_bundle;
            this->Freeze();
            if (export_type == m_exprot_type.preset_bundle) {
                for (std::pair<std::string, Preset *> preset : m_printer_presets) {
                    std::string preset_name = preset.first;
                    //printer preset mast have user's filament or process preset or printer preset is user preset
                    if (m_filament_presets.find(preset_name) == m_filament_presets.end() && m_process_presets.find(preset_name) == m_process_presets.end() && preset.second->is_system) continue;
                    wxString printer_name = wxString::FromUTF8(preset_name);
                    m_preset_sizer->Add(create_checkbox(m_presets_window, preset.second, printer_name, m_preset), 0, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, FromDIP(5));
                }
                m_serial_text->SetLabel(_L("Only display printer names with changes to printer, filament, and process presets."));
            }else if (export_type == m_exprot_type.filament_bundle) {
                for (std::pair<std::string, std::vector<std::pair<std::string, Preset*>>> filament_name_to_preset : m_filament_name_to_presets) {
                    if (filament_name_to_preset.second.empty()) continue;
                    wxString filament_name = wxString::FromUTF8(filament_name_to_preset.first);
                    m_preset_sizer->Add(create_checkbox(m_presets_window, filament_name, m_printer_name), 0, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, FromDIP(5));
                }
                m_serial_text->SetLabel(_L("Only display the filament names with changes to filament presets."));
            } else if (export_type == m_exprot_type.printer_preset) {
                for (std::pair<std::string, Preset *> preset : m_printer_presets) {
                    if (preset.second->is_system) continue;
                    wxString printer_name = wxString::FromUTF8(preset.first);
                    m_preset_sizer->Add(create_checkbox(m_presets_window, preset.second, printer_name, m_preset), 0, wxEXPAND | wxTOP | wxLEFT | wxRIGHT,
                                        FromDIP(5));
                }
                m_serial_text->SetLabel(_L("Only printer names with user printer presets will be displayed, and each preset you choose will be exported as a zip."));
            } else if (export_type == m_exprot_type.filament_preset) {
                for (std::pair<std::string, std::vector<std::pair<std::string, Preset *>>> filament_name_to_preset : m_filament_name_to_presets) {
                    if (filament_name_to_preset.second.empty()) continue;
                    wxString filament_name = wxString::FromUTF8(filament_name_to_preset.first);
                    m_preset_sizer->Add(create_checkbox(m_presets_window, filament_name, m_printer_name), 0, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, FromDIP(5));
                }
                m_serial_text->SetLabel(_L("Only the filament names with user filament presets will be displayed, \nand all user filament presets in each filament name you select will be exported as a zip."));
            } else if (export_type == m_exprot_type.process_preset) {
                for (std::pair<std::string, std::vector<Preset *>> presets : m_process_presets) {
                    Preset *      printer_preset = preset_bundle->printers.find_preset(presets.first, false);
                    if (!printer_preset) continue;
                    if (!printer_preset->is_system) continue;
                    if (preset_bundle->printers.get_preset_base(*printer_preset) != printer_preset) continue;
                    for (Preset *preset : presets.second) {
                        if (!preset->is_system) {
                            wxString printer_name = wxString::FromUTF8(presets.first);
                            m_preset_sizer->Add(create_checkbox(m_presets_window, printer_name, m_printer_name), 0, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, FromDIP(5));
                            break;
                        }
                    }
                    
                }
                m_serial_text->SetLabel(_L("Only printer names with changed process presets will be displayed, \nand all user process presets in each printer name you select will be exported as a zip."));
            }
            //m_presets_window->SetSizerAndFit(m_preset_sizer);
            m_presets_window->Layout();
            m_presets_window->Fit();
            int width  = m_presets_window->GetSize().GetWidth();
            int height = m_presets_window->GetSize().GetHeight();
            m_scrolled_preset_window->SetMinSize(wxSize(std::min(1200, width), std::min(600, height)));
            m_scrolled_preset_window->SetMaxSize(wxSize(std::min(1200, width), std::min(600, height)));
            m_scrolled_preset_window->SetSize(wxSize(std::min(1200, width), std::min(600, height)));
            this->SetSizerAndFit(m_main_sizer);
            Layout();
            Fit();
            Refresh();
            adjust_dialog_in_screen(this);
            this->Thaw();
        } else {
            radiobox_list[i].first->SetValue(false);
        }
    }
}

ExportConfigsDialog::ExportCase ExportConfigsDialog::archive_preset_bundle_to_file(const wxString &path)
{
    std::string export_path = initial_file_path(path, "");
    if (export_path.empty() || "initial_failed" == export_path) return ExportCase::EXPORT_CANCEL;
    BOOST_LOG_TRIVIAL(info) << "Export printer preset bundle";
    
    for (std::pair<::CheckBox *, Preset *> checkbox_preset : m_preset) {
        if (checkbox_preset.first->GetValue()) {
            Preset *printer_preset = checkbox_preset.second;
            std::string printer_preset_name_ = printer_preset->name;

            json          bundle_structure;
            NetworkAgent *agent = wxGetApp().getAgent();
            std::string   clock = get_curr_timestmp();
            if (agent) {
                bundle_structure["version"]   = agent->get_version();
                bundle_structure["bundle_id"] = agent->get_user_id() + "_" + printer_preset_name_ + "_" + clock;
            } else {
                bundle_structure["version"]   = "";
                bundle_structure["bundle_id"] = "offline_" + printer_preset_name_ + "_" + clock;
            }
            bundle_structure["bundle_type"] = "printer config bundle";
            bundle_structure["printer_preset_name"] = printer_preset_name_;
            json printer_config   = json::array();
            json filament_configs = json::array();
            json process_configs  = json::array();

            mz_zip_archive zip_archive;
            mz_bool        status = initial_zip_archive(zip_archive, export_path + "/" + printer_preset->name + ".creality_printer");
            if (MZ_FALSE == status) {
                BOOST_LOG_TRIVIAL(info) << "Failed to initialize ZIP archive";
                return ExportCase::INITIALIZE_FAIL;
            }
            
            boost::filesystem::path printer_file_path      = boost::filesystem::path(printer_preset->file);
            std::string             preset_path       = printer_file_path.make_preferred().string();
            if (preset_path.empty()) {
                BOOST_LOG_TRIVIAL(info) << "Export printer preset: " << printer_preset->name << " skip because of the preset file path is empty.";
                continue;
            }

            // Add a file to the ZIP file
            std::string printer_config_file_name = "printer/" + printer_file_path.filename().string();
            status = mz_zip_writer_add_file(&zip_archive, printer_config_file_name.c_str(), encode_path(preset_path.c_str()).c_str(), NULL, 0, MZ_DEFAULT_COMPRESSION);
            //status = mz_zip_writer_add_mem(&zip_archive, ("printer/" + printer_preset->name + ".json").c_str(), json_contents, strlen(json_contents), MZ_DEFAULT_COMPRESSION);
            if (MZ_FALSE == status) {
                BOOST_LOG_TRIVIAL(info) << printer_preset->name << " Failed to add file to ZIP archive";
                mz_zip_writer_end(&zip_archive);
                return ExportCase::ADD_FILE_FAIL;
            }
            printer_config.push_back(printer_config_file_name);
            BOOST_LOG_TRIVIAL(info) << "Printer preset json add successful: " << printer_preset->name;

            const std::string printer_preset_name = printer_preset->name;
            std::unordered_map<std::string, std::vector<Preset *>>::iterator iter = m_filament_presets.find(printer_preset_name);
            if (m_filament_presets.end() != iter) {
                for (Preset *preset : iter->second) {
                    boost::filesystem::path filament_file_path   = boost::filesystem::path(preset->file);
                    std::string             filament_preset_path = filament_file_path.make_preferred().string();
                    if (filament_preset_path.empty()) {
                        BOOST_LOG_TRIVIAL(info) << "Export filament preset: " << preset->name << " skip because of the preset file path is empty.";
                        continue;
                    }

                    std::string filament_config_file_name = "filament/" + filament_file_path.filename().string();
                    status = mz_zip_writer_add_file(&zip_archive, filament_config_file_name.c_str(), encode_path(filament_preset_path.c_str()).c_str(), NULL, 0, MZ_DEFAULT_COMPRESSION);
                    if (MZ_FALSE == status) {
                        BOOST_LOG_TRIVIAL(info) << preset->name << " Failed to add file to ZIP archive";
                        mz_zip_writer_end(&zip_archive);
                        return ExportCase::ADD_FILE_FAIL;
                    }
                    filament_configs.push_back(filament_config_file_name);
                    BOOST_LOG_TRIVIAL(info) << "Filament preset json add successful.";
                }
            }

            iter = m_process_presets.find(printer_preset_name);
            if (m_process_presets.end() != iter) {
                for (Preset *preset : iter->second) {
                    boost::filesystem::path process_file_path   = boost::filesystem::path(preset->file);
                    std::string             process_preset_path = process_file_path.make_preferred().string();
                    if (process_preset_path.empty()) {
                        BOOST_LOG_TRIVIAL(info) << "Export process preset: " << preset->name << " skip because of the preset file path is empty.";
                        continue;
                    }

                    std::string process_config_file_name = "process/" + process_file_path.filename().string();
                    status = mz_zip_writer_add_file(&zip_archive, process_config_file_name.c_str(), encode_path(process_preset_path.c_str()).c_str(), NULL, 0, MZ_DEFAULT_COMPRESSION);
                    if (MZ_FALSE == status) {
                        BOOST_LOG_TRIVIAL(info) << preset->name << " Failed to add file to ZIP archive";
                        mz_zip_writer_end(&zip_archive);
                        return ExportCase::ADD_FILE_FAIL;
                    }
                    process_configs.push_back(process_config_file_name);
                    BOOST_LOG_TRIVIAL(info) << "Process preset json add successful: ";
                }
            }

            bundle_structure["printer_config"]  = printer_config;
            bundle_structure["filament_config"] = filament_configs;
            bundle_structure["process_config"]  = process_configs;

            std::string bundle_structure_str = bundle_structure.dump();
            status = mz_zip_writer_add_mem(&zip_archive, BUNDLE_STRUCTURE_JSON_NAME, bundle_structure_str.data(), bundle_structure_str.size(), MZ_DEFAULT_COMPRESSION);
            if (MZ_FALSE == status) {
                BOOST_LOG_TRIVIAL(info) << " Failed to add file: " << BUNDLE_STRUCTURE_JSON_NAME;
                mz_zip_writer_end(&zip_archive);
                return ExportCase::ADD_BUNDLE_STRUCTURE_FAIL;
            }
            BOOST_LOG_TRIVIAL(info) << " Success to add file: " << BUNDLE_STRUCTURE_JSON_NAME;

            ExportCase save_result = save_zip_archive_to_file(zip_archive);
            if (ExportCase::CASE_COUNT != save_result) return save_result;
        }
    }
    BOOST_LOG_TRIVIAL(info) << "ZIP archive created successfully";

    return ExportCase::EXPORT_SUCCESS;
}

ExportConfigsDialog::ExportCase ExportConfigsDialog::archive_filament_bundle_to_file(const wxString &path)
{
    std::string export_path = initial_file_path(path, "");
    if (export_path.empty() || "initial_failed" == export_path) return ExportCase::EXPORT_CANCEL;
    BOOST_LOG_TRIVIAL(info) << "Export filament preset bundle";

    for (std::pair<::CheckBox *, std::string> checkbox_filament_name : m_printer_name) {
        if (checkbox_filament_name.first->GetValue()) {
            std::string filament_name = checkbox_filament_name.second;

            json          bundle_structure;
            NetworkAgent *agent = wxGetApp().getAgent();
            std::string   clock = get_curr_timestmp();
            if (agent) {
                bundle_structure["version"]   = agent->get_version();
                bundle_structure["bundle_id"] = agent->get_user_id() + "_" + filament_name + "_" + clock;
            } else {
                bundle_structure["version"]   = "";
                bundle_structure["bundle_id"] = "offline_" + filament_name + "_" + clock;
            }
            bundle_structure["bundle_type"] = "filament config bundle";
            bundle_structure["filament_name"] = filament_name;
            std::unordered_map<std::string, json> vendor_structure;

            mz_zip_archive zip_archive;
            mz_bool        status = initial_zip_archive(zip_archive, export_path + "/" + filament_name + ".creality_filament");
            if (MZ_FALSE == status) {
                BOOST_LOG_TRIVIAL(info) << "Failed to initialize ZIP archive";
                return ExportCase::INITIALIZE_FAIL;
            }

            std::unordered_map<std::string, std::vector<std::pair<std::string, Preset *>>>::iterator iter = m_filament_name_to_presets.find(filament_name);
            if (m_filament_name_to_presets.end() == iter) {
                BOOST_LOG_TRIVIAL(info) << "Filament name do not find, filament name:" << filament_name;
                continue;
            }
            std::set<std::pair<std::string, std::string>> vendor_to_filament_name;
            for (std::pair<std::string, Preset *> printer_name_to_preset : iter->second) {
                std::string printer_vendor = printer_name_to_preset.first;
                if (printer_vendor.empty()) continue;
                Preset *    filament_preset = printer_name_to_preset.second;
                if (vendor_to_filament_name.find(std::make_pair(printer_vendor, filament_preset->name)) != vendor_to_filament_name.end()) continue;
                vendor_to_filament_name.insert(std::make_pair(printer_vendor, filament_preset->name));
                std::string preset_path     = boost::filesystem::path(filament_preset->file).make_preferred().string();
                if (preset_path.empty()) {
                    BOOST_LOG_TRIVIAL(info) << "Export printer preset: " << filament_preset->name << " skip because of the preset file path is empty.";
                    continue;
                }
                // Add a file to the ZIP file
                std::string file_name = printer_vendor + "/" + filament_preset->name + ".json";
                status                = mz_zip_writer_add_file(&zip_archive, file_name.c_str(), encode_path(preset_path.c_str()).c_str(), NULL, 0, MZ_DEFAULT_COMPRESSION);
                // status = mz_zip_writer_add_mem(&zip_archive, ("printer/" + printer_preset->name + ".json").c_str(), json_contents, strlen(json_contents), MZ_DEFAULT_COMPRESSION);
                if (MZ_FALSE == status) {
                    BOOST_LOG_TRIVIAL(info) << filament_preset->name << " Failed to add file to ZIP archive";
                    mz_zip_writer_end(&zip_archive);
                    return ExportCase::ADD_FILE_FAIL;
                }
                std::unordered_map<std::string, json>::iterator iter = vendor_structure.find(printer_vendor);
                if (vendor_structure.end() == iter) {
                    json j = json::array();
                    j.push_back(file_name);
                    vendor_structure[printer_vendor] = j;
                } else {
                    iter->second.push_back(file_name);
                }
                BOOST_LOG_TRIVIAL(info) << "Filament preset json add successful: " << filament_preset->name;
            }
            
            for (const std::pair<std::string, json>& vendor_name_to_json : vendor_structure) {
                json j;
                std::string printer_vendor = vendor_name_to_json.first;
                j["vendor"]                = printer_vendor;
                j["filament_path"]         = vendor_name_to_json.second;
                bundle_structure["printer_vendor"].push_back(j);
            }

            std::string bundle_structure_str = bundle_structure.dump();
            status = mz_zip_writer_add_mem(&zip_archive, BUNDLE_STRUCTURE_JSON_NAME, bundle_structure_str.data(), bundle_structure_str.size(), MZ_DEFAULT_COMPRESSION);
            if (MZ_FALSE == status) {
                BOOST_LOG_TRIVIAL(info) << " Failed to add file: " << BUNDLE_STRUCTURE_JSON_NAME;
                mz_zip_writer_end(&zip_archive);
                return ExportCase::ADD_BUNDLE_STRUCTURE_FAIL;
            }
            BOOST_LOG_TRIVIAL(info) << " Success to add file: " << BUNDLE_STRUCTURE_JSON_NAME;

            // Complete writing of ZIP file
            ExportCase save_result = save_zip_archive_to_file(zip_archive);
            if (ExportCase::CASE_COUNT != save_result) return save_result;
        }
    }
    BOOST_LOG_TRIVIAL(info) << "ZIP archive created successfully";

    return ExportCase::EXPORT_SUCCESS;
}

ExportConfigsDialog::ExportCase ExportConfigsDialog::archive_printer_preset_to_file(const wxString &path)
{
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "start exprot printer presets";
    std::string export_file = "Printer presets.zip";
    export_file             = initial_file_name(path, export_file);
    if (export_file.empty() || "initial_failed" == export_file) return ExportCase::EXPORT_CANCEL;

    std::vector<std::pair<std::string, std::string>> config_paths;

    for (std::pair<::CheckBox *, Preset *> checkbox_preset : m_preset) {
        if (checkbox_preset.first->GetValue()) {
            Preset *    printer_preset = checkbox_preset.second;
            std::string preset_path    = boost::filesystem::path(printer_preset->file).make_preferred().string();
            if (preset_path.empty()) {
                BOOST_LOG_TRIVIAL(info) << "Export printer preset: " << printer_preset->name << " skip because of the preset file path is empty.";
                continue;
            }
            std::string preset_name = printer_preset->name + ".json";
            config_paths.push_back(std::make_pair(preset_name, preset_path));
        }
    }

    ExportCase save_result = save_presets_to_zip(export_file, config_paths);
    if (ExportCase::CASE_COUNT != save_result) return save_result;

    BOOST_LOG_TRIVIAL(info) << "ZIP archive created successfully";

    return ExportCase::EXPORT_SUCCESS;

}

ExportConfigsDialog::ExportCase ExportConfigsDialog::archive_filament_preset_to_file(const wxString &path)
{
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "start exprot filament presets";
    std::string export_file = "Filament presets.zip";
    export_file             = initial_file_name(path, export_file);
    if (export_file.empty() || "initial_failed" == export_file) return ExportCase::EXPORT_CANCEL;

    std::vector<std::pair<std::string, std::string>> config_paths;

    std::set<std::string> filament_presets;
    for (std::pair<::CheckBox *, std::string> checkbox_preset : m_printer_name) {
        if (checkbox_preset.first->GetValue()) {
            std::string filament_name = checkbox_preset.second;

            std::unordered_map<std::string, std::vector<std::pair<std::string, Preset *>>>::iterator iter = m_filament_name_to_presets.find(filament_name);
            if (m_filament_name_to_presets.end() == iter) {
                BOOST_LOG_TRIVIAL(info) << "Filament name do not find, filament name:" << filament_name;
                continue;
            }
            for (std::pair<std::string, Preset*> printer_name_preset : iter->second) {
                Preset *    filament_preset = printer_name_preset.second;
                if (filament_presets.find(filament_preset->name) != filament_presets.end()) continue;
                filament_presets.insert(filament_preset->name);
                std::string preset_path     = boost::filesystem::path(filament_preset->file).make_preferred().string();
                if (preset_path.empty()) {
                    BOOST_LOG_TRIVIAL(info) << "Export filament preset: " << filament_preset->name << " skip because of the filament file path is empty.";
                    continue;
                }

                std::string preset_name = filament_preset->name + ".json";
                config_paths.push_back(std::make_pair(preset_name, preset_path));
            }
        }
    }

    ExportCase save_result = save_presets_to_zip(export_file, config_paths);
    if (ExportCase::CASE_COUNT != save_result) return save_result;

    BOOST_LOG_TRIVIAL(info) << "ZIP archive created successfully";

    return ExportCase::EXPORT_SUCCESS;
}

ExportConfigsDialog::ExportCase ExportConfigsDialog::archive_process_preset_to_file(const wxString &path)
{
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "start exprot process presets";
    std::string export_file = "Process presets.zip";
    export_file             = initial_file_name(path, export_file);
    if (export_file.empty() || "initial_failed" == export_file) return ExportCase::EXPORT_CANCEL;

    std::vector<std::pair<std::string, std::string>> config_paths;

    std::set<std::string> process_presets;
    for (std::pair<::CheckBox *, std::string> checkbox_preset : m_printer_name) {
        if (checkbox_preset.first->GetValue()) {
            std::string printer_name = checkbox_preset.second;
            std::unordered_map<std::string, std::vector<Preset *>>::iterator iter = m_process_presets.find(printer_name);
            if (m_process_presets.end() != iter) {
                for (Preset *process_preset : iter->second) {
                    if (process_presets.find(process_preset->name) != process_presets.end()) continue;
                    process_presets.insert(process_preset->name);
                    std::string preset_path = boost::filesystem::path(process_preset->file).make_preferred().string();
                    if (preset_path.empty()) {
                        BOOST_LOG_TRIVIAL(info) << "Export process preset: " << process_preset->name << " skip because of the preset file path is empty.";
                        continue;
                    }
                    
                    std::string preset_name = process_preset->name + ".json";
                    config_paths.push_back(std::make_pair(preset_name, preset_path));
                }
            }
        }
    }

    ExportCase save_result = save_presets_to_zip(export_file, config_paths);
    if (ExportCase::CASE_COUNT != save_result) return save_result;

    BOOST_LOG_TRIVIAL(info) << "ZIP archive created successfully";

    return ExportCase::EXPORT_SUCCESS;
}


wxBoxSizer* ExportConfigsDialog::create_left_navigation(wxWindow* parent)
{
    // wxBoxSizer*     vertical_sizer2 = new wxBoxSizer(wxVERTICAL);
    // m_leftNavigationPanel           = new ExportLeftNavigationPanel(this);
    // vertical_sizer2->Add(m_leftNavigationPanel, 1, wxEXPAND | wxALL, FromDIP(16));

    // return vertical_sizer2;
    wxBoxSizer* bk    = new wxBoxSizer(wxHORIZONTAL);
    wxPanel*    panel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    panel->SetMinSize(wxSize(FromDIP(218), FromDIP(585)));
    panel->SetMaxSize(wxSize(FromDIP(218), FromDIP(585)));
    panel->SetBackgroundColour(wxColour("#4B4B4D"));
    bk->Add(panel, 1, wxEXPAND | wxALL, 8);

    wxBoxSizer* vertical_sizer = new wxBoxSizer(wxVERTICAL);
    panel->SetSizer(vertical_sizer);

    wxBoxSizer* pSttSizer = new wxBoxSizer(wxHORIZONTAL);
    pSttSizer->SetMinSize(wxSize(FromDIP(124), FromDIP(48)));

    wxStaticText* pSttText = new wxStaticText(panel, wxID_ANY, _L("Export Type:"), wxDefaultPosition, wxDefaultSize,
                                              wxALIGN_CENTER_HORIZONTAL);
    pSttText->SetForegroundColour(wxColour("#FFFFFF"));
    pSttText->SetFont(Label::Body_14);
    pSttText->SetSize(wxSize(FromDIP(110), FromDIP(18)));
    pSttText->SetMinSize(wxSize(FromDIP(110), FromDIP(18)));
    pSttSizer->Add(pSttText, 0, wxLeft | wxALIGN_CENTER_VERTICAL, FromDIP(0));
    vertical_sizer->Add(pSttSizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(0));

    //  打印机预设集
    wxButton* pBtnPrinterPresets          = new wxButton(panel, wxID_ANY, _L("Printer Presets"), wxDefaultPosition, wxDefaultSize,
                                                wxLEFT | wxBORDER_NONE);
    m_stPrinterPresets.pBtnPresets = pBtnPrinterPresets;
    pBtnPrinterPresets->SetFont(Label::Body_12);
    pBtnPrinterPresets->SetSize(wxSize(FromDIP(124), FromDIP(28)));
    pBtnPrinterPresets->SetMinSize(wxSize(FromDIP(124), FromDIP(28)));
    pBtnPrinterPresets->SetForegroundColour(wxColour("#FFFFFF"));
    pBtnPrinterPresets->SetBackgroundColour(wxColour("#4B4B4D"));
    pBtnPrinterPresets->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& e) {
        m_stPrinterPresets.pBtnPresets->SetForegroundColour(wxColour("#FFFFFF"));
        m_stPrinterPresets.pBtnPresets->SetBackgroundColour(wxColour("#18CC5C"));
    });
    pBtnPrinterPresets->Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& e) {
        if (!m_stPrinterPresets.bClicked)
            m_stPrinterPresets.pBtnPresets->SetBackgroundColour(wxColour("#4B4B4D"));
    });
    pBtnPrinterPresets->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& e) {
        m_stPrinterPresets.bClicked = true;
        m_stPrinterPresets.pBtnPresets->SetBackgroundColour(wxColour("#18CC5C"));
        m_stFilamentPresets.bClicked = false;
        m_stFilamentPresets.pBtnPresets->SetBackgroundColour(wxColour("#4B4B4D"));
        m_stProcessPresets.bClicked = false;
        m_stProcessPresets.pBtnPresets->SetBackgroundColour(wxColour("#4B4B4D"));
        onBtnPrinterClicked();
    });
    vertical_sizer->Add(pBtnPrinterPresets, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(1));

    //  耗材预设集
    wxButton* pBtnFilamentPresets           = new wxButton(panel, wxID_ANY, _L("Filament Presets"), wxDefaultPosition, wxDefaultSize,
                                                 wxLEFT | wxBORDER_NONE);
    m_stFilamentPresets.pBtnPresets = pBtnFilamentPresets;
    pBtnFilamentPresets->SetFont(Label::Body_12);
    pBtnFilamentPresets->SetSize(wxSize(FromDIP(124), FromDIP(28)));
    pBtnFilamentPresets->SetMinSize(wxSize(FromDIP(124), FromDIP(28)));
    pBtnFilamentPresets->SetForegroundColour(wxColour("#FFFFFF"));
    pBtnFilamentPresets->SetBackgroundColour(wxColour("#4B4B4D"));
    pBtnFilamentPresets->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& e) {
        m_stFilamentPresets.pBtnPresets->SetForegroundColour(wxColour("#FFFFFF"));
        m_stFilamentPresets.pBtnPresets->SetBackgroundColour(wxColour("#18CC5C"));
    });
    pBtnFilamentPresets->Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& e) {
        if (!m_stFilamentPresets.bClicked)
            m_stFilamentPresets.pBtnPresets->SetBackgroundColour(wxColour("#4B4B4D"));
    });
    pBtnFilamentPresets->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& e) {
        m_stFilamentPresets.bClicked = true;
        m_stFilamentPresets.pBtnPresets->SetBackgroundColour(wxColour("#18CC5C"));
        m_stPrinterPresets.bClicked = false;
        m_stPrinterPresets.pBtnPresets->SetBackgroundColour(wxColour("#4B4B4D"));
        m_stProcessPresets.bClicked = false;
        m_stProcessPresets.pBtnPresets->SetBackgroundColour(wxColour("#4B4B4D"));
        onBtnFilamentClicked();
    });
    vertical_sizer->Add(pBtnFilamentPresets, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(1));

    //  工艺预设集
    wxButton* pBtnProcessPresets          = new wxButton(panel, wxID_ANY, _L("Process Presets"), wxDefaultPosition, wxDefaultSize,
                                                wxLEFT | wxBORDER_NONE);
    m_stProcessPresets.pBtnPresets = pBtnProcessPresets;
    pBtnProcessPresets->SetFont(Label::Body_12);
    pBtnProcessPresets->SetSize(wxSize(FromDIP(124), FromDIP(28)));
    pBtnProcessPresets->SetMinSize(wxSize(FromDIP(124), FromDIP(28)));
    pBtnProcessPresets->SetForegroundColour(wxColour("#FFFFFF"));
    pBtnProcessPresets->SetBackgroundColour(wxColour("#4B4B4D"));
    pBtnProcessPresets->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& e) {
        m_stProcessPresets.pBtnPresets->SetForegroundColour(wxColour("#FFFFFF"));
        m_stProcessPresets.pBtnPresets->SetBackgroundColour(wxColour("#18CC5C"));
    });
    pBtnProcessPresets->Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& e) {
        if (!m_stProcessPresets.bClicked)
            m_stProcessPresets.pBtnPresets->SetBackgroundColour(wxColour("#4B4B4D"));
    });
    pBtnProcessPresets->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& e) {
        m_stProcessPresets.bClicked = true;
        m_stProcessPresets.pBtnPresets->SetBackgroundColour(wxColour("#18CC5C"));
        m_stPrinterPresets.bClicked = false;
        m_stPrinterPresets.pBtnPresets->SetBackgroundColour(wxColour("#4B4B4D"));
        m_stFilamentPresets.bClicked = false;
        m_stFilamentPresets.pBtnPresets->SetBackgroundColour(wxColour("#4B4B4D"));
        onBtnProcessClicked();
    });
    vertical_sizer->Add(pBtnProcessPresets, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(1));

    return bk;
}

wxBoxSizer* ExportConfigsDialog::createRightContent(wxWindow* parent)
{
    wxBoxSizer* bk             = new wxBoxSizer(wxHORIZONTAL);
    wxPanel*    panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    panel->SetMinSize(wxSize(FromDIP(760), FromDIP(585)));
    panel->SetMaxSize(wxSize(FromDIP(760), FromDIP(585)));
    panel->SetBackgroundColour(wxColour("#4B4B4D"));

    wxBoxSizer* vertical_sizer = new wxBoxSizer(wxHORIZONTAL);
    panel->SetSizer(vertical_sizer);

    vertical_sizer->Add(createMidPresets(panel));

    vertical_sizer->Add(create_right_export_context(panel), 0, wxLEFT, FromDIP(8));

    bk->Add(panel, 1, wxTOP|wxRIGHT, FromDIP(11));
    return bk;
}

wxBoxSizer* ExportConfigsDialog::createMidPresets(wxWindow* parent)
{
    wxBoxSizer* vertical_sizer = new wxBoxSizer(wxVERTICAL);
    m_exportMidPanel           = new ExportMidPanel(parent);
    vertical_sizer->Add(m_exportMidPanel, 1, wxEXPAND | wxALL, FromDIP(0));

    return vertical_sizer;
}
wxBoxSizer* ExportConfigsDialog::create_right_export_context(wxWindow* parent)
{
    wxBoxSizer* vertical_sizer = new wxBoxSizer(wxVERTICAL);
    m_exportRightPanel         = new ExportRightPanel(parent, _L("Export Content"));
    m_exportMidPanel->m_rightPanel = m_exportRightPanel;
    m_exportRightPanel->setExportBtnClicked([this]() { archivePresetToFile(); });
    m_exportRightPanel->setCancelBtnClicked([this]() { cancelExport(); });
    m_exportRightPanel->setCheckboxClickedCb(std::bind(&ExportMidPanel::funcRightPanelItemCheckedCb, m_exportMidPanel, std::placeholders::_1));

    vertical_sizer->Add(m_exportRightPanel, 2, wxEXPAND | wxALL, FromDIP(0));

    return vertical_sizer;
}
void ExportConfigsDialog::onBtnPrinterClicked()
{
    if (m_index == 0)
        return;
    std::unordered_map<std::string, std::list<ExportMidPanel::STLineDataNode*>> mapPrinterPresets;
    if (!m_exportMidPanel->hasPrinterInit()) {
        std::list<ExportMidPanel::STLineDataNode*> lstUserPresets;
        std::list<ExportMidPanel::STLineDataNode*> lstSystemPresets;
        for (std::pair<std::string, Preset*> preset : m_printer_presets) {
            // wxString printer_name = wxString::FromUTF8(preset.first);
            ExportMidPanel::STLineDataNode* pLineDataNode = new ExportMidPanel::STLineDataNode;
            pLineDataNode->lstPrinterPresetParam.push_back(preset.second);
            pLineDataNode->name = preset.second->name;
            bool hasFoundFilamentOrProcess = false;
            auto iter2 = m_filament_presets.find(preset.second->name);
            if (iter2 != m_filament_presets.end()) {
                for (auto item3 : iter2->second) {
                    pLineDataNode->lstFilamentPresetParam.push_back(item3);
                    hasFoundFilamentOrProcess = true;
                }
            }
            iter2 = m_process_presets.find(preset.second->name);
            if (iter2 != m_process_presets.end()) {
                for (auto item3 : iter2->second) {
                    pLineDataNode->lstProcessPresetParam.push_back(item3);
                    hasFoundFilamentOrProcess = true;
                }
            }
            if (preset.second->is_system) {
                if (hasFoundFilamentOrProcess) {
                    lstSystemPresets.push_back(pLineDataNode);
                } else {//如果系统打印机没有自定义耗材和工艺，则不显示导出
                    delete pLineDataNode;
                    pLineDataNode = nullptr;
                }
            } else {
                lstUserPresets.push_back(pLineDataNode);
            }
        }
        mapPrinterPresets["User Preset Param"]   = lstUserPresets;
        mapPrinterPresets["System Preset Param"] = lstSystemPresets;
    }
    m_exportMidPanel->updatePrinterPresets(mapPrinterPresets);
    m_index = 0;
}
void ExportConfigsDialog::onBtnFilamentClicked()
{
    if (m_index == 1)
        return;
    std::unordered_map<std::string, std::list<ExportMidPanel::STLineDataNode*>> mapPrinterPresets;
    if (!m_exportMidPanel->hasFilamentInit()) {
        for (auto item : m_filament_name_to_presets) {
            std::list<ExportMidPanel::STLineDataNode*>* lstLineDataNode = nullptr;
            bool                                        bFound          = false;

            for (const auto& item2 : item.second) {
                std::string ssFilamentType = "";
                item2.second->get_filament_type(ssFilamentType);
                auto iter = mapPrinterPresets.find(ssFilamentType);
                if (iter == mapPrinterPresets.end()) {
                    mapPrinterPresets[ssFilamentType] = std::list<ExportMidPanel::STLineDataNode*>();
                    iter                              = mapPrinterPresets.find(ssFilamentType);
                }
                lstLineDataNode = &iter->second;

                ExportMidPanel::STLineDataNode* pLineDataNode = nullptr;
                {// 判断是否已经有同名的节点
                    for (auto item3 : *lstLineDataNode) {
                        if (item3->name == item.first) {
                            pLineDataNode = item3;
                            break;
                        }
                    }
                }
                if (pLineDataNode == nullptr) {
                    pLineDataNode = new ExportMidPanel::STLineDataNode;
                    pLineDataNode->name = item.first; // item2.second->name;
                    lstLineDataNode->push_back(pLineDataNode);
                }
                pLineDataNode->lstFilamentPresetParam.push_back(item2.second);
            }
        }
    }
    m_exportMidPanel->updateFilamentPresets(mapPrinterPresets);
    m_index = 1;
    // m_midpanel->updateFilamentPresets(m_mapFilamentPresets);
}
void ExportConfigsDialog::onBtnProcessClicked()
{
    if (m_index == 2)
        return;
    std::unordered_map<std::string, std::list<ExportMidPanel::STLineDataNode*>> mapPrinterPresets;

    if (!m_exportMidPanel->hasProcessInit()) {
        PresetBundle preset_bundle(*wxGetApp().preset_bundle);
        mapPrinterPresets["Process Presets"]                        = std::list<ExportMidPanel::STLineDataNode*>();
        std::list<ExportMidPanel::STLineDataNode*>* lstLineDataNode = &mapPrinterPresets["Process Presets"];
        for (std::pair<std::string, std::vector<Preset*>> presets : m_process_presets) {
            Preset* printer_preset = preset_bundle.printers.find_preset(presets.first, false);
            if (!printer_preset)
                continue;
            if (!printer_preset->is_system)
                continue;
            if (preset_bundle.printers.get_preset_base(*printer_preset) != printer_preset)
                continue;
            ExportMidPanel::STLineDataNode* pLineDataNode = new ExportMidPanel::STLineDataNode;
            pLineDataNode->name                           = presets.first;
            for (Preset* preset : presets.second) {
                if (!preset->is_system) {
                    wxString printer_name = wxString::FromUTF8(presets.first);
                    pLineDataNode->lstProcessPresetParam.push_back(preset);
                }
            }
            lstLineDataNode->push_back(pLineDataNode);
        }
    }
    m_exportMidPanel->updateProcessPresets(mapPrinterPresets);
    m_index = 2;
}
void                            ExportConfigsDialog::cancelExport() { EndModal(wxID_CANCEL); }
ExportConfigsDialog::ExportCase ExportConfigsDialog::archivePresetToFile() {
    wxDirDialog dlg(this, _L("Choose a directory"), "", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
    wxString    path;
    if (dlg.ShowModal() == wxID_OK)
        path = dlg.GetPath();
    ExportCase export_case = ExportCase::EXPORT_CANCEL;
    if (!path.IsEmpty()) {
        wxGetApp().app_config->update_config_dir(into_u8(path));
        wxGetApp().app_config->save();

        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "start exprot printer presets";
        std::string export_file = "Presets.zip";
        // printer

        // path += "\\Printer presets";
        // export_file             = initial_file_name(path, export_file);
        // if (export_file.empty() || "initial_failed" == export_file)
        //    return ExportCase::EXPORT_CANCEL;


        if (m_index == 0) {
            std::list<std::shared_ptr<ExportMidPanel::STLineDataNode>> lstCheckedPrinterPresets;
            m_exportMidPanel->getCheckedPrinterPresets(lstCheckedPrinterPresets);

            for (const auto& item : lstCheckedPrinterPresets) {
                export_file = initial_file_name(path, item->name) + ".creality_printer";

                json        bundle_structure;
                std::string clock                       = get_curr_timestmp();
                bundle_structure["version"]             = wxGetApp().getAgent() ? wxGetApp().getAgent()->get_version() : "";
                bundle_structure["bundle_id"]           = item->name + "_" + clock;
                bundle_structure["bundle_type"]         = "printer config bundle";
                bundle_structure["printer_preset_name"] = item->name;
                json                                             printer_config   = json::array();
                json                                             filament_configs = json::array();
                json                                             process_configs  = json::array();

                mz_zip_archive                                   zip_archive;
                mz_bool                                          status = initial_zip_archive(zip_archive, export_file);
                if (MZ_FALSE == status) {
                    BOOST_LOG_TRIVIAL(info) << "Failed to initialize ZIP archive";
                    export_case = ExportCase::INITIALIZE_FAIL;
                    break;
                }

                std::vector<std::pair<std::string, std::string>> config_paths;
                export_case = ExportCase::EXPORT_SUCCESS;
                for (const auto& itemPrinter : item->lstPrinterPresetParam) {
                    Preset*     printer_preset = itemPrinter;
                    std::string preset_path    = boost::filesystem::path(printer_preset->file).make_preferred().string();
                    if (preset_path.empty()) {
                        BOOST_LOG_TRIVIAL(info)
                            << "Export printer preset: " << printer_preset->name << " skip because of the preset file path is empty.";
                        continue;
                    }
                    std::string preset_name = "printer/" + printer_preset->name + ".json";
                    config_paths.push_back(std::make_pair(preset_name, preset_path));

                    status = mz_zip_writer_add_file(&zip_archive, preset_name.c_str(),
                                                    encode_path(preset_path.c_str()).c_str(), NULL, 0, MZ_DEFAULT_COMPRESSION);
                    if (MZ_FALSE == status) {
                        BOOST_LOG_TRIVIAL(info) << printer_preset->name << " Failed to add file to ZIP archive";
                        mz_zip_writer_end(&zip_archive);
                        export_case = ExportCase::ADD_FILE_FAIL;
                        break;
                    }
                    printer_config.push_back(preset_name);
                }
                if (export_case != ExportCase::EXPORT_SUCCESS)
                    break;

                for (const auto& itemFilament : item->lstFilamentPresetParam) {
                    Preset*     filament_preset = itemFilament;
                    std::string preset_path     = boost::filesystem::path(filament_preset->file).make_preferred().string();
                    if (preset_path.empty()) {
                        BOOST_LOG_TRIVIAL(info)
                            << "Export filament preset: " << filament_preset->name << " skip because of the preset file path is empty.";
                        continue;
                    }
                    std::string preset_name = "filament/" + filament_preset->name + ".json";
                    config_paths.push_back(std::make_pair(preset_name, preset_path));

                    status = mz_zip_writer_add_file(&zip_archive, preset_name.c_str(), encode_path(preset_path.c_str()).c_str(), NULL, 0,
                                                    MZ_DEFAULT_COMPRESSION);
                    if (MZ_FALSE == status) {
                        BOOST_LOG_TRIVIAL(info) << filament_preset->name << " Failed to add file to ZIP archive";
                        mz_zip_writer_end(&zip_archive);
                        export_case = ExportCase::ADD_FILE_FAIL;
                        break;
                    }
                    filament_configs.push_back(preset_name);
                }
                if (export_case != ExportCase::EXPORT_SUCCESS)
                    break;

                for (const auto& itemProcess : item->lstProcessPresetParam) {
                    Preset*     process_preset = itemProcess;
                    std::string preset_path    = boost::filesystem::path(process_preset->file).make_preferred().string();
                    if (preset_path.empty()) {
                        BOOST_LOG_TRIVIAL(info)
                            << "Export process preset: " << process_preset->name << " skip because of the preset file path is empty.";
                        continue;
                    }
                    std::string preset_name = "process/" + process_preset->name + ".json";
                    config_paths.push_back(std::make_pair(preset_name, preset_path));

                    status = mz_zip_writer_add_file(&zip_archive, preset_name.c_str(), encode_path(preset_path.c_str()).c_str(), NULL, 0,
                                                    MZ_DEFAULT_COMPRESSION);
                    if (MZ_FALSE == status) {
                        BOOST_LOG_TRIVIAL(info) << process_preset->name << " Failed to add file to ZIP archive";
                        mz_zip_writer_end(&zip_archive);
                        export_case = ExportCase::ADD_FILE_FAIL;
                        break;
                    }
                    process_configs.push_back(preset_name);
                }
                if (export_case != ExportCase::EXPORT_SUCCESS)
                    break;
                //ExportCase save_result = save_presets_to_zip(export_file, config_paths);
                //if (ExportCase::CASE_COUNT != save_result)
                //    export_case = save_result;
                //else
                //    export_case = EXPORT_SUCCESS;

                bundle_structure["printer_config"]  = printer_config;
                bundle_structure["filament_config"] = filament_configs;
                bundle_structure["process_config"]  = process_configs;

                std::string bundle_structure_str = bundle_structure.dump();
                status = mz_zip_writer_add_mem(&zip_archive, BUNDLE_STRUCTURE_JSON_NAME, bundle_structure_str.data(),
                                               bundle_structure_str.size(), MZ_DEFAULT_COMPRESSION);
                if (MZ_FALSE == status) {
                    BOOST_LOG_TRIVIAL(info) << " Failed to add file: " << BUNDLE_STRUCTURE_JSON_NAME;
                    mz_zip_writer_end(&zip_archive);
                    export_case = ExportCase::ADD_BUNDLE_STRUCTURE_FAIL;
                    break;
                }
                BOOST_LOG_TRIVIAL(info) << " Success to add file: " << BUNDLE_STRUCTURE_JSON_NAME;

                ExportCase save_result = save_zip_archive_to_file(zip_archive);
                //if (ExportCase::CASE_COUNT != save_result)
                    //return save_result;
                if (ExportCase::CASE_COUNT != save_result)
                   export_case = save_result;
                else
                   export_case = EXPORT_SUCCESS;
            }

        } else if (m_index == 1) {
            std::list<std::shared_ptr<ExportMidPanel::STLineDataNode>> lstCheckedFilamentPresets;
            m_exportMidPanel->getCheckedFilamentPresets(lstCheckedFilamentPresets);
            for (const auto& item : lstCheckedFilamentPresets) {
                export_file = initial_file_name(path, item->name) + ".creality_filament";
                std::vector<std::pair<std::string, std::string>> config_paths;

                for (const auto& itemFilament : item->lstFilamentPresetParam) {
                    Preset*     filament_preset = itemFilament;
                    std::string preset_path     = boost::filesystem::path(filament_preset->file).make_preferred().string();
                    if (preset_path.empty()) {
                        BOOST_LOG_TRIVIAL(info)
                            << "Export filament preset: " << filament_preset->name << " skip because of the preset file path is empty.";
                        continue;
                    }
                    std::string preset_name = filament_preset->name + ".json";
                    config_paths.push_back(std::make_pair(preset_name, preset_path));
                }

                ExportCase save_result = save_presets_to_zip(export_file, config_paths);
                if (ExportCase::CASE_COUNT != save_result)
                    export_case = save_result;
                else
                    export_case = EXPORT_SUCCESS;
            }

        } else if (m_index == 2) {
            std::list<std::shared_ptr<ExportMidPanel::STLineDataNode>> lstCheckedProcessPresets;
            m_exportMidPanel->getCheckedProcessPresets(lstCheckedProcessPresets);
            for (const auto& item : lstCheckedProcessPresets) {
                export_file = initial_file_name(path, item->name) + ".zip";
                std::vector<std::pair<std::string, std::string>> config_paths;

                for (const auto& itemProcess : item->lstProcessPresetParam) {
                    Preset*     process_preset = itemProcess;
                    std::string preset_path    = boost::filesystem::path(process_preset->file).make_preferred().string();
                    if (preset_path.empty()) {
                        BOOST_LOG_TRIVIAL(info)
                            << "Export process preset: " << process_preset->name << " skip because of the preset file path is empty.";
                        continue;
                    }
                    std::string preset_name = process_preset->name + ".json";
                    config_paths.push_back(std::make_pair(preset_name, preset_path));
                }
                ExportCase save_result = save_presets_to_zip(export_file, config_paths);
                if (ExportCase::CASE_COUNT != save_result)
                    export_case = save_result;
                else
                    export_case = EXPORT_SUCCESS;
            }
        
        }


        BOOST_LOG_TRIVIAL(info) << "ZIP archive created successfully";
    } else {
        return ExportCase::EXPORT_SUCCESS;
    }

    show_export_result(export_case);
    if (ExportCase::EXPORT_SUCCESS != export_case)
        return ExportCase::EXPORT_SUCCESS;

    EndModal(wxID_OK);
    return ExportCase::EXPORT_SUCCESS;
}

wxBoxSizer *ExportConfigsDialog::create_button_item(wxWindow* parent)
{
    wxBoxSizer *bSizer_button = new wxBoxSizer(wxHORIZONTAL);
    bSizer_button->Add(0, 0, 1, wxEXPAND, 0);

    StateColor btn_bg_green(std::pair<wxColour, int>(wxColour(0, 137, 123), StateColor::Pressed), std::pair<wxColour, int>(wxColour(38, 166, 154), StateColor::Hovered),
                            std::pair<wxColour, int>(wxColour(0, 150, 136), StateColor::Normal));

    m_button_ok = new Button(this, _L("OK"));
    m_button_ok->SetBackgroundColor(btn_bg_green);
    m_button_ok->SetBorderColor(*wxWHITE);
    m_button_ok->SetTextColor(wxColour(0xFFFFFE));
    m_button_ok->SetFont(Label::Body_12);
    m_button_ok->SetSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_ok->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_ok->SetCornerRadius(FromDIP(12));
    bSizer_button->Add(m_button_ok, 0, wxRIGHT, FromDIP(10));

    m_button_ok->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent &e) {
        if (!has_check_box_selected()) {
            MessageDialog dlg(this, _L("Please select at least one printer or filament."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"),
                              wxYES | wxYES_DEFAULT | wxCENTRE);
            dlg.ShowModal();
            return;
        }

        wxDirDialog dlg(this, _L("Choose a directory"), "", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
        wxString    path;
        if (dlg.ShowModal() == wxID_OK) path = dlg.GetPath();
        ExportCase export_case = ExportCase::EXPORT_CANCEL;
        if (!path.IsEmpty()) {
            wxGetApp().app_config->update_config_dir(into_u8(path));
            wxGetApp().app_config->save();

            if (m_cur_export_type == m_exprot_type.preset_bundle) {
                export_case = archive_preset_bundle_to_file(path);
            } else if (m_cur_export_type == m_exprot_type.filament_bundle) {
                export_case = archive_filament_bundle_to_file(path);
            } else if (m_cur_export_type == m_exprot_type.printer_preset) {
                export_case = archive_printer_preset_to_file(path);
            } else if (m_cur_export_type == m_exprot_type.filament_preset) {
                export_case = archive_filament_preset_to_file(path);
            } else if (m_cur_export_type == m_exprot_type.process_preset) {
                export_case = archive_process_preset_to_file(path);
            }
        } else {
            return;
        }
        show_export_result(export_case);
        if (ExportCase::EXPORT_SUCCESS != export_case) return;
        
        EndModal(wxID_OK);
        });

    StateColor btn_bg_white(std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Pressed), std::pair<wxColour, int>(wxColour(238, 238, 238), StateColor::Hovered),
                            std::pair<wxColour, int>(*wxWHITE, StateColor::Normal));

    m_button_cancel = new Button(this, _L("Cancel"));
    m_button_cancel->SetBackgroundColor(btn_bg_white);
    m_button_cancel->SetBorderColor(wxColour(38, 46, 48));
    m_button_cancel->SetFont(Label::Body_12);
    m_button_cancel->SetSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_cancel->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_cancel->SetCornerRadius(FromDIP(12));
    bSizer_button->Add(m_button_cancel, 0, wxRIGHT, FromDIP(10));

    m_button_cancel->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent &e) { EndModal(wxID_CANCEL); });

    return bSizer_button;
}

wxBoxSizer *ExportConfigsDialog::create_select_printer(wxWindow *parent)
{
    wxBoxSizer *horizontal_sizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer *  optionSizer        = new wxBoxSizer(wxVERTICAL);
    m_serial_text           = new wxStaticText(parent, wxID_ANY, _L("Please select a type you want to export"), wxDefaultPosition, wxDefaultSize);
    optionSizer->Add(m_serial_text, 0, wxEXPAND | wxALL, 0);
    optionSizer->SetMinSize(OPTION_SIZE);
    horizontal_sizer->Add(optionSizer, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(10));
    m_scrolled_preset_window = new wxScrolledWindow(parent);
    m_scrolled_preset_window->SetScrollRate(5, 5);
    m_scrolled_preset_window->SetBackgroundColour(PRINTER_LIST_COLOUR);
    m_scrolled_preset_window->SetMaxSize(wxSize(FromDIP(660), FromDIP(400)));
    m_scrolled_preset_window->SetSize(wxSize(FromDIP(660), FromDIP(400)));
    wxBoxSizer *scrolled_window = new wxBoxSizer(wxHORIZONTAL);

    m_presets_window = new wxPanel(m_scrolled_preset_window, wxID_ANY);
    m_presets_window->SetBackgroundColour(PRINTER_LIST_COLOUR);
    wxBoxSizer *select_printer_sizer  = new wxBoxSizer(wxVERTICAL);

    m_preset_sizer = new wxGridSizer(3, FromDIP(5), FromDIP(5));
    select_printer_sizer->Add(m_preset_sizer, 0, wxEXPAND, FromDIP(5));
    m_presets_window->SetSizer(select_printer_sizer);
    scrolled_window->Add(m_presets_window, 0, wxEXPAND, 0);
    m_scrolled_preset_window->SetSizerAndFit(scrolled_window);

    horizontal_sizer->Add(m_scrolled_preset_window, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(10));

    return horizontal_sizer;
}

void ExportConfigsDialog::data_init()
{
    // Delete the Temp folder
    boost::filesystem::path folder(data_dir() + "/" + PRESET_USER_DIR + "/" + "Temp");
    if (boost::filesystem::exists(folder)) boost::filesystem::remove_all(folder);

    boost::system::error_code ec;
    boost::filesystem::path user_folder(data_dir() + "/" + PRESET_USER_DIR);
    bool                      temp_folder_exist = true;
    if (!boost::filesystem::exists(user_folder)) {
        if (!boost::filesystem::create_directories(user_folder, ec)) {
            BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << " create directory failed: " << user_folder << " "<<ec.message();
            temp_folder_exist = false;
        }
    }
    boost::filesystem::path temp_folder(user_folder / "Temp");
    if (!boost::filesystem::exists(temp_folder)) {
        if (!boost::filesystem::create_directories(temp_folder, ec)) {
            BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << " create directory failed: " << temp_folder << " " << ec.message();
            temp_folder_exist = false;
        }
    }
    if (!temp_folder_exist) {
        MessageDialog dlg(this, _L("Failed to create temporary folder, please try Export Configs again."), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES_NO | wxYES_DEFAULT | wxCENTRE);
        dlg.ShowModal();
        EndModal(wxCANCEL);
    }

    PresetBundle preset_bundle(*wxGetApp().preset_bundle);

    const std::deque<Preset> & printer_presets = preset_bundle.printers.get_presets();
    for (const Preset &printer_preset : printer_presets) {
        
        std::string preset_name        = printer_preset.name;
        if (!printer_preset.is_visible || printer_preset.is_default || printer_preset.is_project_embedded) continue;
        if (preset_bundle.printers.select_preset_by_name(preset_name, true)) {
            preset_bundle.update_compatible(PresetSelectCompatibleType::Always);

            const std::deque<Preset> &filament_presets = preset_bundle.filaments.get_presets();
            for (const Preset &filament_preset : filament_presets) {
                if (filament_preset.is_system || filament_preset.is_default || filament_preset.is_project_embedded) continue;
                if (filament_preset.is_compatible) {
                    Preset *new_filament_preset = new Preset(filament_preset);
                    m_filament_presets[preset_name].push_back(new_filament_preset);
                }
            }

            const std::deque<Preset> &process_presets = preset_bundle.prints.get_presets();
            for (const Preset &process_preset : process_presets) {
                if (process_preset.is_system || process_preset.is_default || process_preset.is_project_embedded) continue;
                if (process_preset.is_compatible) {
                    Preset *new_prpcess_preset = new Preset(process_preset);
                    m_process_presets[preset_name].push_back(new_prpcess_preset);
                }
            }
            
            Preset *new_printer_preset     = new Preset(printer_preset);
            earse_preset_fields_for_safe(new_printer_preset);
            m_printer_presets[preset_name] = new_printer_preset;
        }
    }
    const std::deque<Preset> &filament_presets = preset_bundle.filaments.get_presets();
    for (const Preset &filament_preset : filament_presets) {
        if (filament_preset.is_system || filament_preset.is_default) continue;
        Preset *new_filament_preset = new Preset(filament_preset);
        const Preset *base_filament_preset = preset_bundle.filaments.get_preset_base(*new_filament_preset);
        if (base_filament_preset == nullptr)
            continue;

        std::string filament_preset_name = base_filament_preset->name;
        std::string machine_name         = get_machine_name(filament_preset_name);
        m_filament_name_to_presets[get_filament_name(filament_preset_name)].push_back(std::make_pair(get_vendor_name(machine_name), new_filament_preset));
    }
}

void ExportConfigsDialog::set_export_type(const wxString& export_type)
{
    m_preset_sizer->Clear(true);
    m_printer_name.clear();
    m_preset.clear();
    PresetBundle* preset_bundle = wxGetApp().preset_bundle;
    this->Freeze();
    bool empty = true;
    if (export_type == m_exprot_type.printer_preset) {
        for (std::pair<std::string, Preset*> preset : m_printer_presets) {
            if (preset.second->is_system)
                continue;
            empty                 = false;
            wxString printer_name = wxString::FromUTF8(preset.first);
            m_preset_sizer->Add(create_checkbox(m_presets_window, preset.second, printer_name, m_preset), 0,
                                wxEXPAND | wxTOP | wxLEFT | wxRIGHT, FromDIP(5));
        }
        m_serial_text->SetLabel(
            _L("Only printer names with user printer presets will be displayed, and each preset you choose will be exported as a zip."));
    } else if (export_type == m_exprot_type.filament_preset) {
        for (std::pair<std::string, std::vector<std::pair<std::string, Preset*>>> filament_name_to_preset : m_filament_name_to_presets) {
            if (filament_name_to_preset.second.empty())
                continue;
            empty                  = false;
            wxString filament_name = wxString::FromUTF8(filament_name_to_preset.first);
            m_preset_sizer->Add(create_checkbox(m_presets_window, filament_name, m_printer_name), 0, wxEXPAND | wxTOP | wxLEFT | wxRIGHT,
                                FromDIP(5));
        }
        m_serial_text->SetLabel(_L("Only the filament names with user filament presets will be displayed, \nand all user filament presets "
                                   "in each filament name you select will be exported as a zip."));
    } else if (export_type == m_exprot_type.process_preset) {
        for (std::pair<std::string, std::vector<Preset*>> presets : m_process_presets) {
            Preset* printer_preset = preset_bundle->printers.find_preset(presets.first, false);
            if (!printer_preset)
                continue;
            if (!printer_preset->is_system)
                continue;
            if (preset_bundle->printers.get_preset_base(*printer_preset) != printer_preset)
                continue;
            for (Preset* preset : presets.second) {
                if (!preset->is_system) {
                    empty                 = false;
                    wxString printer_name = wxString::FromUTF8(presets.first);
                    m_preset_sizer->Add(create_checkbox(m_presets_window, printer_name, m_printer_name), 0,
                                        wxEXPAND | wxTOP | wxLEFT | wxRIGHT, FromDIP(5));
                    break;
                }
            }
        }
        m_serial_text->SetLabel(_L("Only printer names with changed process presets will be displayed, \nand all user process presets in "
                                   "each printer name you select will be exported as a zip."));
    }

    if (empty) {
        m_serial_text->SetLabel(_L("There are currently no presets available for export"));
        m_button_ok->Show(false);
        m_button_cancel->Show(false);
    }
    m_presets_window->Layout();
    m_presets_window->Fit();
    int width  = m_presets_window->GetSize().GetWidth();
    int height = m_presets_window->GetSize().GetHeight();
    m_scrolled_preset_window->SetMinSize(wxSize(std::min(1200, width), std::min(600, height)));
    m_scrolled_preset_window->SetMaxSize(wxSize(std::min(1200, width), std::min(600, height)));
    m_scrolled_preset_window->SetSize(wxSize(std::min(1200, width), std::min(600, height)));
    this->SetSizerAndFit(m_main_sizer);
    Layout();
    Fit();
    Refresh();
    adjust_dialog_in_screen(this);
    this->Thaw();
}

EditFilamentPresetDialog::EditFilamentPresetDialog(wxWindow *parent, FilamentInfomation *filament_info)
    : DPIDialog(parent ? parent : nullptr, wxID_ANY, _L("Edit Filament"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX)
    , m_filament_id("")
    , m_filament_name("")
    , m_vendor_name("")
    , m_filament_type("")
    , m_filament_serial("")
{
    m_preset_tree_creater = new PresetTree(this);

    this->SetBackgroundColour(*wxWHITE);
    this->SetMinSize(wxSize(FromDIP(600), -1));

    std::string icon_path = (boost::format("%1%/images/Creative3DTitle.ico") % resources_dir()).str();
    SetIcon(wxIcon(encode_path(icon_path.c_str()), wxBITMAP_TYPE_ICO));

    m_main_sizer = new wxBoxSizer(wxVERTICAL);
    // top line
    auto m_line_top = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 1), wxTAB_TRAVERSAL);
    m_line_top->SetBackgroundColour(wxColour(0xA6, 0xa9, 0xAA));
    m_main_sizer->Add(m_line_top, 0, wxEXPAND, 0);
    m_main_sizer->Add(0, 0, 0, wxTOP, FromDIP(5));

    wxStaticText* basic_infomation = new wxStaticText(this, wxID_ANY, _L("Basic Information")); 
    basic_infomation->SetFont(Label::Head_16);
    
    m_main_sizer->Add(basic_infomation, 0, wxALL, FromDIP(10));
    m_filament_id = filament_info->filament_id;
    //std::string filament_name = filament_info->filament_name;
    bool get_filament_presets = get_same_filament_id_presets(m_filament_id);
    // get filament vendor, type, serial, and name
    if (get_filament_presets && !m_printer_compatible_presets.empty()) {
        std::shared_ptr<Preset> preset;
        for (std::pair<std::string, std::vector<std::shared_ptr<Preset>>> pair : m_printer_compatible_presets) {
            for (std::shared_ptr<Preset> fialment_preset : pair.second) {
                if (fialment_preset->inherits().empty()) {
                    preset = fialment_preset;
                    break;
                }
            }
        }
        if (!preset.get()) preset = m_printer_compatible_presets.begin()->second[0];
        m_filament_name                = get_filament_name(preset->name);
        auto vendor_names              = dynamic_cast<ConfigOptionStrings *>(preset->config.option("filament_vendor"));
        if (vendor_names  && !vendor_names->values.empty()) m_vendor_name = vendor_names->values[0];
        auto filament_types = dynamic_cast<ConfigOptionStrings *>(preset->config.option("filament_type"));
        if (filament_types && !filament_types->values.empty()) m_filament_type = filament_types->values[0];
        std::string filament_type = m_filament_type == "PLA-AERO" ? "PLA Aero" : m_filament_type;
        size_t      index         = m_filament_name.find(filament_type);
        if (std::string::npos != index && index + filament_type.size() < m_filament_name.size()) {
            m_filament_serial = m_filament_name.substr(index + filament_type.size());
            if (m_filament_serial.size() > 2 && m_filament_serial[0] == ' ') {
                m_filament_serial = m_filament_serial.substr(1);
            }
        }
    }

    m_main_sizer->Add(create_filament_basic_info(), 0, wxEXPAND | wxALL, 0);

    // divider line
    auto line_divider = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 1), wxTAB_TRAVERSAL);
    line_divider->SetBackgroundColour(wxColour(0xA6, 0xa9, 0xAA));
    m_main_sizer->Add(line_divider, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(10));
    m_main_sizer->Add(0, 0, 0, wxTOP, FromDIP(5));

    wxStaticText *presets_infomation = new wxStaticText(this, wxID_ANY, _L("Filament presets under this filament"));
    presets_infomation->SetFont(Label::Head_16);
    m_main_sizer->Add(presets_infomation, 0, wxLEFT | wxRIGHT, FromDIP(10));

    m_main_sizer->Add(create_add_filament_btn(), 0, wxEXPAND | wxALL, 0);
    m_main_sizer->Add(create_preset_tree_sizer(), 0, wxEXPAND | wxALL, 0);
    m_note_text = new wxStaticText(this, wxID_ANY, _L("Note: If the only preset under this filament is deleted, the filament will be deleted after exiting the dialog."));
    m_main_sizer->Add(m_note_text, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM | wxALIGN_CENTER_VERTICAL, FromDIP(10));
    m_note_text->Hide();
    m_main_sizer->Add(create_button_sizer(), 0, wxEXPAND | wxALL, 0);

    update_preset_tree();

    this->SetSizer(m_main_sizer);
    this->Layout();
    this->Fit();
    wxGetApp().UpdateDlgDarkUI(this);
}
EditFilamentPresetDialog::~EditFilamentPresetDialog() {}

void EditFilamentPresetDialog::on_dpi_changed(const wxRect &suggested_rect) {
    /*m_add_filament_btn->Rescale();
    m_del_filament_btn->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
    m_del_filament_btn->SetMaxSize(wxSize(FromDIP(58), FromDIP(24)));
    m_del_filament_btn->SetCornerRadius(FromDIP(12));
    m_ok_btn->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
    m_ok_btn->SetMaxSize(wxSize(FromDIP(58), FromDIP(24)));
    m_ok_btn->SetCornerRadius(FromDIP(12));*/ 
    Layout();
}

bool EditFilamentPresetDialog::get_same_filament_id_presets(std::string filament_id)
{
    PresetBundle *preset_bundle = wxGetApp().preset_bundle;
    const std::deque<Preset> &filament_presets = preset_bundle->filaments.get_presets();
    
    m_printer_compatible_presets.clear();
    for (Preset const &preset : filament_presets) {
        if (preset.is_system || preset.filament_id != filament_id) continue;
        std::shared_ptr<Preset> new_preset = std::make_shared<Preset>(preset);
        std::vector<std::string> printers;
        get_filament_compatible_printer(new_preset.get(), printers);
        for (const std::string &printer_name : printers) {
            m_printer_compatible_presets[printer_name].push_back(new_preset);
        }
    }
    if (m_printer_compatible_presets.empty()) {
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " no filament presets ";
        return false;
    }

    return true;
}

void EditFilamentPresetDialog::update_preset_tree()
{
    this->Freeze();
    m_preset_tree_sizer->Clear(true);
    for (std::pair<std::string, std::vector<std::shared_ptr<Preset>>> printer_and_presets : m_printer_compatible_presets) {
        m_preset_tree_sizer->Add(m_preset_tree_creater->get_preset_tree(printer_and_presets), 0, wxEXPAND | wxLEFT | wxTOP | wxRIGHT, 5);
    }
    if (m_printer_compatible_presets.size() == 1 && m_printer_compatible_presets.begin()->second.size() == 1) {
        m_note_text->Show();
    } else {
        m_note_text->Hide();
    }

    m_preset_tree_panel->SetSizerAndFit(m_preset_tree_sizer);
    int width  = m_preset_tree_panel->GetSize().GetWidth();
    int height = m_preset_tree_panel->GetSize().GetHeight();
    if (width < m_note_text->GetSize().GetWidth()) {
        width = m_note_text->GetSize().GetWidth();
        m_preset_tree_panel->SetMinSize(wxSize(width, -1));
    }
    int width_extend = 0;
    int height_extend = 0;
    if (width > 1000) height_extend = 22;
    if (height > 400) width_extend = 22;
    m_preset_tree_window->SetMinSize(wxSize(std::min(1000, width + width_extend), std::min(400, height + height_extend)));
    m_preset_tree_window->SetMaxSize(wxSize(std::min(1000, width + width_extend), std::min(400, height + height_extend)));
    m_preset_tree_window->SetSize(wxSize(std::min(1000, width + width_extend), std::min(400, height + height_extend)));
    this->SetSizerAndFit(m_main_sizer);
    
    this->Layout();
    this->Fit();
    this->Refresh();
    wxGetApp().UpdateDlgDarkUI(this);
    adjust_dialog_in_screen(this);
    this->Thaw();
}

void EditFilamentPresetDialog::delete_preset()
{
    if (m_selected_printer.empty()) return;
    if (m_need_delete_preset_index < 0) return;
    std::unordered_map<std::string, std::vector<std::shared_ptr<Preset>>>::iterator iter = m_printer_compatible_presets.find(m_selected_printer);
    if (m_printer_compatible_presets.end() == iter) {
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " can not find printer and printer name is: " << m_selected_printer;
        return;
    }
    std::vector<std::shared_ptr<Preset>>& filament_presets = iter->second;
    if (m_need_delete_preset_index >= filament_presets.size()) {
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " index error and selected printer is: " << m_selected_printer << " and index: " << m_need_delete_preset_index;
        return;
    }
    std::shared_ptr<Preset> need_delete_preset = filament_presets[m_need_delete_preset_index];
    // is selecetd filament preset
    if (need_delete_preset->name == wxGetApp().preset_bundle->filaments.get_selected_preset_name()) {
        wxGetApp().get_tab(need_delete_preset->type)->delete_preset();
        // is preset exist? exist: not delete
        Preset *delete_preset = wxGetApp().preset_bundle->filaments.find_preset(need_delete_preset->name, false);
        if (delete_preset) {
            m_selected_printer.clear();
            m_need_delete_preset_index = -1;
            return;
        }
    } else {
        Preset *filament_preset = wxGetApp().preset_bundle->filaments.find_preset(need_delete_preset->name);

        // is root preset ?
        bool is_base_preset = false;
        if (filament_preset && wxGetApp().preset_bundle->filaments.get_preset_base(*filament_preset) == filament_preset) {
            is_base_preset = true;
            int      count = 0;
            wxString presets;
            for (auto &preset2 : wxGetApp().preset_bundle->filaments)
                if (preset2.inherits() == filament_preset->name) {
                    ++count;
                    presets += "\n - " + from_u8(preset2.name);
                }
            wxString msg;
            if (count > 0) {
                msg = _L("Presets inherited by other presets can not be deleted");
                msg += "\n";
                msg += _L_PLURAL("The following presets inherits this preset.", "The following preset inherits this preset.", count);
                wxString title = _L("Delete Preset");
                MessageDialog(this, msg + presets, title, wxOK | wxICON_ERROR).ShowModal();
                m_selected_printer.clear();
                m_need_delete_preset_index = -1;
                return;
            }
        }
        wxString msg;
        if (is_base_preset) {
            msg = _L("Are you sure to delete the selected preset? \nIf the preset corresponds to a filament currently in use on your printer, please reset the filament information for that slot.");
        } else {
            msg = _L("Are you sure to delete the selected preset?");
        }
        if (wxID_YES != MessageDialog(this, msg, _L("Delete preset"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION).ShowModal()) {
            m_selected_printer.clear();
            m_need_delete_preset_index = -1;
            return;
        }

        // delete preset
        std::string next_selected_preset_name = wxGetApp().preset_bundle->filaments.get_selected_preset().name;
        bool        delete_result             = delete_filament_preset_by_name(need_delete_preset->name, next_selected_preset_name);
        BOOST_LOG_TRIVIAL(info) << __LINE__ << " filament preset name: " << need_delete_preset->name << (delete_result ? " delete successful" : " delete failed");

        wxGetApp().preset_bundle->filaments.select_preset_by_name(next_selected_preset_name, true);
        for (size_t i = 0; i < wxGetApp().preset_bundle->filament_presets.size(); ++i) {
            auto preset = wxGetApp().preset_bundle->filaments.find_preset(wxGetApp().preset_bundle->filament_presets[i]);
            if (preset == nullptr) wxGetApp().preset_bundle->filament_presets[i] = wxGetApp().preset_bundle->filaments.get_selected_preset_name();
        }
    }

    // remove preset shared_ptr from m_printer_compatible_presets
    int                     last_index         = filament_presets.size() - 1;
    if (m_need_delete_preset_index != last_index) {
        std::swap(filament_presets[m_need_delete_preset_index], filament_presets[last_index]);
    }
    filament_presets.pop_back();
    if (filament_presets.empty()) m_printer_compatible_presets.erase(iter);

    update_preset_tree();

    m_selected_printer.clear();
    m_need_delete_preset_index = -1;
}

void EditFilamentPresetDialog::edit_preset()
{
    if (m_selected_printer.empty()) return;
    if (m_need_edit_preset_index < 0) return;
    std::unordered_map<std::string, std::vector<std::shared_ptr<Preset>>>::iterator iter = m_printer_compatible_presets.find(m_selected_printer);
    if (m_printer_compatible_presets.end() == iter) {
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " can not find printer and printer name is: " << m_selected_printer;
        return;
    }
    std::vector<std::shared_ptr<Preset>> &filament_presets = iter->second;
    if (m_need_edit_preset_index >= filament_presets.size()) {
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " index error and selected printer is: " << m_selected_printer << " and index: " << m_need_edit_preset_index;
        return;
    }

    // edit preset
    //m_need_edit_preset = filament_presets[m_need_edit_preset_index];
    //wxGetApp().params_dialog()->set_editing_filament_id(m_filament_id);

    m_need_edit_preset = filament_presets[m_need_edit_preset_index];
    if(!m_need_edit_preset) 
    {
        return;
    }

    const std::string strName = m_need_edit_preset->name;
    Tab* tab = wxGetApp().get_tab(Preset::Type::TYPE_FILAMENT);
    if(!tab)  
    {
        return;
    }

    if(tab->GetParent() == wxGetApp().params_panel())
    {
        wxGetApp().mainframe->select_tab(MainFrame::tp3DEditor);
    }
    else
    {
        wxGetApp().params_dialog()->Popup();
    }
    tab->restore_last_select_item();


    if(wxGetApp().get_tab(Preset::Type::TYPE_FILAMENT)->select_preset(strName))
    {
        wxGetApp().get_tab(Preset::Type::TYPE_FILAMENT)->get_combo_box()->set_filament_idx(0);
    }
    else
    {
        wxGetApp().params_dialog()->Hide();
    }
    
    wxGetApp().params_dialog()->set_editing_filament_id(m_filament_id);
    wxGetApp().params_dialog()->panel()->OnPanelShowInit();


    EndModal(wxID_EDIT);
}

wxBoxSizer *EditFilamentPresetDialog::create_filament_basic_info()
{
    wxBoxSizer *basic_info_sizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer *vendor_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *type_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *serial_sizer = new wxBoxSizer(wxHORIZONTAL);

    //vendor
    wxBoxSizer *  vendor_key_sizer        = new wxBoxSizer(wxVERTICAL);
    wxStaticText *static_vendor_text = new wxStaticText(this, wxID_ANY, _L("Vendor"), wxDefaultPosition, wxDefaultSize);
    vendor_key_sizer->Add(static_vendor_text, 0, wxEXPAND | wxALL, 0);
    vendor_key_sizer->SetMinSize(OPTION_SIZE);
    vendor_sizer->Add(vendor_key_sizer, 0, wxEXPAND | wxLEFT | wxBOTTOM | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    wxBoxSizer *vendor_value_sizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText *vendor_text = new wxStaticText(this, wxID_ANY, from_u8(m_vendor_name), wxDefaultPosition, wxDefaultSize);
    vendor_value_sizer->Add(vendor_text, 0, wxEXPAND | wxALL, 0);
    vendor_sizer->Add(vendor_value_sizer, 0, wxEXPAND | wxLEFT | wxBOTTOM | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    //type
    wxBoxSizer *  type_key_sizer   = new wxBoxSizer(wxVERTICAL);
    wxStaticText *static_type_text = new wxStaticText(this, wxID_ANY, _L("Type"), wxDefaultPosition, wxDefaultSize);
    type_key_sizer->Add(static_type_text, 0, wxEXPAND | wxALL, 0);
    type_key_sizer->SetMinSize(OPTION_SIZE);
    type_sizer->Add(type_key_sizer, 0, wxEXPAND | wxLEFT | wxBOTTOM | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    wxBoxSizer *  type_value_sizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText *type_text        = new wxStaticText(this, wxID_ANY, from_u8(m_filament_type), wxDefaultPosition, wxDefaultSize);
    type_value_sizer->Add(type_text, 0, wxEXPAND | wxALL, 0);
    type_sizer->Add(type_value_sizer, 0, wxEXPAND | wxLEFT | wxBOTTOM | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    //serial
    wxBoxSizer *  serial_key_sizer   = new wxBoxSizer(wxVERTICAL);
    wxStaticText *static_serial_text = new wxStaticText(this, wxID_ANY, _L("Serial"), wxDefaultPosition, wxDefaultSize);
    serial_key_sizer->Add(static_serial_text, 0, wxEXPAND | wxALL, 0);
    serial_key_sizer->SetMinSize(OPTION_SIZE);
    serial_sizer->Add(serial_key_sizer, 0, wxEXPAND | wxLEFT | wxBOTTOM | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    wxBoxSizer *  serial_value_sizer = new wxBoxSizer(wxVERTICAL);
    wxString      full_filamnet_serial = from_u8(m_filament_serial);
    wxString      show_filament_serial = full_filamnet_serial;
    if (m_filament_serial.size() > 40) {
        show_filament_serial = from_u8(m_filament_serial.substr(0, 20)) + "...";
    }
    wxStaticText *serial_text = new wxStaticText(this, wxID_ANY, show_filament_serial, wxDefaultPosition, wxDefaultSize);
    wxToolTip *   toolTip     = new wxToolTip(full_filamnet_serial);
    serial_text->SetToolTip(toolTip);
    serial_value_sizer->Add(serial_text, 0, wxEXPAND | wxALL, 0);
    serial_sizer->Add(serial_value_sizer, 0, wxEXPAND | wxLEFT | wxBOTTOM | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    basic_info_sizer->Add(vendor_sizer, 0, wxEXPAND | wxALL, 0);
    basic_info_sizer->Add(type_sizer, 0, wxEXPAND | wxALL, 0);
    basic_info_sizer->Add(serial_sizer, 0, wxEXPAND | wxALL, 0);

    return basic_info_sizer;
}

wxBoxSizer *EditFilamentPresetDialog::create_add_filament_btn()
{
    wxBoxSizer *add_filament_btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_add_filament_btn                 = new Button(this, _L("+ Add Preset"));
    m_add_filament_btn->SetFont(Label::Body_10);
    m_add_filament_btn->SetPaddingSize(wxSize(FromDIP(8), FromDIP(3)));
    m_add_filament_btn->SetCornerRadius(FromDIP(8));

    StateColor flush_bg_col(std::pair<wxColour, int>(wxColour(219, 253, 231), StateColor::Pressed), std::pair<wxColour, int>(wxColour(238, 238, 238), StateColor::Hovered),
                            std::pair<wxColour, int>(wxColour(238, 238, 238), StateColor::Normal));

    StateColor flush_fg_col(std::pair<wxColour, int>(wxColour(107, 107, 106), StateColor::Pressed), std::pair<wxColour, int>(wxColour(107, 107, 106), StateColor::Hovered),
                            std::pair<wxColour, int>(wxColour(107, 107, 106), StateColor::Normal));

    StateColor flush_bd_col(std::pair<wxColour, int>(wxColour(0, 150, 136), StateColor::Pressed), std::pair<wxColour, int>(wxColour(0, 150, 136), StateColor::Hovered),
                            std::pair<wxColour, int>(wxColour(172, 172, 172), StateColor::Normal));

    m_add_filament_btn->SetBackgroundColor(flush_bg_col);
    m_add_filament_btn->SetBorderColor(flush_bd_col);
    m_add_filament_btn->SetTextColor(flush_fg_col);
    add_filament_btn_sizer->Add(m_add_filament_btn, 0, wxEXPAND | wxALL, FromDIP(10));

    m_add_filament_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent &e) {
        CreatePresetForPrinterDialog dlg(nullptr, m_filament_type, m_filament_id, m_vendor_name, m_filament_name);
        int res = dlg.ShowModal();
        if (res == wxID_OK) {
            if (get_same_filament_id_presets(m_filament_id)) {
                update_preset_tree();
            }
        }
    });

    return add_filament_btn_sizer;
}

wxBoxSizer *EditFilamentPresetDialog::create_preset_tree_sizer()
{
    wxBoxSizer *filament_preset_tree_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_preset_tree_window = new wxScrolledWindow(this);
    m_preset_tree_window->SetScrollRate(5, 5);
    m_preset_tree_window->SetBackgroundColour(PRINTER_LIST_COLOUR);
    m_preset_tree_window->SetMinSize(wxSize(-1, FromDIP(400)));
    m_preset_tree_window->SetMaxSize(wxSize(-1, FromDIP(300)));
    m_preset_tree_window->SetSize(wxSize(-1, FromDIP(300)));
    m_preset_tree_panel = new wxPanel(m_preset_tree_window);
    m_preset_tree_sizer = new wxBoxSizer(wxVERTICAL);
    m_preset_tree_panel->SetSizer(m_preset_tree_sizer);
    m_preset_tree_panel->SetMinSize(wxSize(580, -1));
    m_preset_tree_panel->SetBackgroundColour(PRINTER_LIST_COLOUR);
    wxBoxSizer* m_preset_tree_window_sizer = new wxBoxSizer(wxVERTICAL);
    m_preset_tree_window_sizer->Add(m_preset_tree_panel, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(10));
    m_preset_tree_window->SetSizerAndFit(m_preset_tree_window_sizer);
    filament_preset_tree_sizer->Add(m_preset_tree_window, 0, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(10));

    return filament_preset_tree_sizer;
}

wxBoxSizer *EditFilamentPresetDialog::create_button_sizer()
{
    wxBoxSizer *bSizer_button = new wxBoxSizer(wxHORIZONTAL);

    m_del_filament_btn = new Button(this, _L("Delete Filament"));
    m_del_filament_btn->SetBackgroundColor(*wxRED);
    m_del_filament_btn->SetBorderColor(*wxWHITE);
    m_del_filament_btn->SetTextColor(wxColour(0xFFFFFE));
    m_del_filament_btn->SetFont(Label::Body_12);
    m_del_filament_btn->SetSize(wxSize(FromDIP(58), FromDIP(24)));
    m_del_filament_btn->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
    m_del_filament_btn->SetCornerRadius(FromDIP(12));
    bSizer_button->Add(m_del_filament_btn, 0, wxLEFT | wxBOTTOM, FromDIP(10));

    bSizer_button->Add(0, 0, 1, wxEXPAND, 0);

    StateColor btn_bg_green(std::pair<wxColour, int>(wxColour(0, 137, 123), StateColor::Pressed), std::pair<wxColour, int>(wxColour(38, 166, 154), StateColor::Hovered),
                            std::pair<wxColour, int>(wxColour(0, 150, 136), StateColor::Normal));

    m_ok_btn = new Button(this, _L("OK"));
    m_ok_btn->SetBackgroundColor(btn_bg_green);
    m_ok_btn->SetBorderColor(*wxWHITE);
    m_ok_btn->SetTextColor(wxColour(0xFFFFFE));
    m_ok_btn->SetFont(Label::Body_12);
    m_ok_btn->SetSize(wxSize(FromDIP(58), FromDIP(24)));
    m_ok_btn->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
    m_ok_btn->SetCornerRadius(FromDIP(12));
    bSizer_button->Add(m_ok_btn, 0, wxRIGHT | wxBOTTOM, FromDIP(10));

    StateColor btn_bg_white(std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Pressed), std::pair<wxColour, int>(wxColour(238, 238, 238), StateColor::Hovered),
                            std::pair<wxColour, int>(*wxWHITE, StateColor::Normal));

    m_del_filament_btn->Bind(wxEVT_BUTTON, ([this](wxCommandEvent &e) {
        WarningDialog dlg(this, _L("All the filament presets belong to this filament would be deleted. \nIf you are using this filament on your printer, please reset the filament information for that slot."), _L("Delete filament"), wxYES | wxCANCEL | wxCANCEL_DEFAULT | wxCENTRE);
        int res = dlg.ShowModal();
        if (wxID_YES == res) {
            PresetBundle *preset_bundle = wxGetApp().preset_bundle;
            std::set<std::shared_ptr<Preset>> inherit_preset_names;
            std::set<std::shared_ptr<Preset>> root_preset_names;
            for (std::pair<std::string, std::vector<std::shared_ptr<Preset>>> printer_and_preset : m_printer_compatible_presets) {
                for (std::shared_ptr<Preset> preset : printer_and_preset.second) {
                    if (preset->inherits().empty()) {
                        root_preset_names.insert(preset);
                    } else {
                        inherit_preset_names.insert(preset);
                    }
                }
            }
            // delete inherit preset first
            std::string next_selected_preset_name = wxGetApp().preset_bundle->filaments.get_selected_preset().name;
            for (std::shared_ptr<Preset> preset : inherit_preset_names) {
                bool delete_result = delete_filament_preset_by_name(preset->name, next_selected_preset_name);
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " inherit filament name: " << preset->name << (delete_result ? " delete successful" : " delete failed");
            }
            for (std::shared_ptr<Preset> preset : root_preset_names) {
                bool delete_result = delete_filament_preset_by_name(preset->name, next_selected_preset_name);
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " root filament name: " << preset->name << (delete_result ? " delete successful" : " delete failed");
            }
            m_printer_compatible_presets.clear();
            wxGetApp().preset_bundle->filaments.select_preset_by_name(next_selected_preset_name,true);
            
            for (size_t i = 0; i < wxGetApp().preset_bundle->filament_presets.size(); ++i) {
                auto preset = wxGetApp().preset_bundle->filaments.find_preset(wxGetApp().preset_bundle->filament_presets[i]);
                if (preset == nullptr) wxGetApp().preset_bundle->filament_presets[i] = wxGetApp().preset_bundle->filaments.get_selected_preset_name();
            }
            EndModal(wxID_OK);
        }
        e.Skip(); 
        }));

    m_ok_btn->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent &e) { EndModal(wxID_OK); });

    return bSizer_button;

}

CreatePresetForPrinterDialog::CreatePresetForPrinterDialog(wxWindow *parent, std::string filament_type, std::string filament_id, std::string filament_vendor, std::string filament_name)
    : DPIDialog(parent ? parent : nullptr, wxID_ANY, _L("Add Preset"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX)
    , m_filament_id(filament_id)
    , m_filament_name(filament_name)
    , m_filament_vendor(filament_vendor)
    , m_filament_type(filament_type)
{
    m_preset_bundle = std::make_shared<PresetBundle>(*(wxGetApp().preset_bundle));
    get_visible_printer_and_compatible_filament_presets();

    this->SetBackgroundColour(*wxWHITE);

    std::string icon_path = (boost::format("%1%/images/Creative3DTitle.ico") % resources_dir()).str();
    SetIcon(wxIcon(encode_path(icon_path.c_str()), wxBITMAP_TYPE_ICO));

    wxBoxSizer *main_sizer = new wxBoxSizer(wxVERTICAL);
    // top line
    auto m_line_top = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 1), wxTAB_TRAVERSAL);
    m_line_top->SetBackgroundColour(wxColour(0xA6, 0xa9, 0xAA));
    main_sizer->Add(m_line_top, 0, wxEXPAND, 0);
    main_sizer->Add(0, 0, 0, wxTOP, FromDIP(5));

    wxStaticText *basic_infomation = new wxStaticText(this, wxID_ANY, _L("Add preset for new printer"));
    basic_infomation->SetFont(Label::Head_16);
    main_sizer->Add(basic_infomation, 0, wxALL, FromDIP(10));

    main_sizer->Add(create_selected_printer_preset_sizer(), 0, wxALL, FromDIP(10));
    main_sizer->Add(create_selected_filament_preset_sizer(), 0, wxALL, FromDIP(10));
    main_sizer->Add(create_button_sizer(), 0, wxEXPAND | wxALL, FromDIP(10));

    this->SetSizer(main_sizer);
    this->Layout();
    this->Fit();
    wxGetApp().UpdateDlgDarkUI(this);
}

CreatePresetForPrinterDialog::~CreatePresetForPrinterDialog() {}

void CreatePresetForPrinterDialog::on_dpi_changed(const wxRect &suggested_rect) {
    m_ok_btn->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
    m_ok_btn->SetMaxSize(wxSize(FromDIP(58), FromDIP(24)));
    m_ok_btn->SetCornerRadius(FromDIP(12));
    m_cancel_btn->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
    m_cancel_btn->SetMaxSize(wxSize(FromDIP(58), FromDIP(24)));
    m_cancel_btn->SetCornerRadius(FromDIP(12));
    Layout();
}

void CreatePresetForPrinterDialog::get_visible_printer_and_compatible_filament_presets()
{
    const std::deque<Preset> &printer_presets = m_preset_bundle->printers.get_presets();
    m_printer_compatible_filament_presets.clear();
    for (const Preset &printer_preset : printer_presets) {
        if (printer_preset.is_visible) {
            if (m_preset_bundle->printers.get_preset_base(printer_preset) != &printer_preset) continue;
            if (m_preset_bundle->printers.select_preset_by_name(printer_preset.name, true)) {
                m_preset_bundle->update_compatible(PresetSelectCompatibleType::Always);
                const std::deque<Preset> &filament_presets = m_preset_bundle->filaments.get_presets();
                for (const Preset &filament_preset : filament_presets) {
                    if (filament_preset.is_default || !filament_preset.is_compatible || filament_preset.is_project_embedded) continue;
                    ConfigOptionStrings *filament_types;
                    const Preset *       filament_preset_base = m_preset_bundle->filaments.get_preset_base(filament_preset);
                    if (filament_preset_base == &filament_preset) {
                        filament_types = dynamic_cast<ConfigOptionStrings *>(const_cast<Preset *>(&filament_preset)->config.option("filament_type"));
                    } else {
                        filament_types = dynamic_cast<ConfigOptionStrings *>(const_cast<Preset *>(filament_preset_base)->config.option("filament_type"));
                    }
                    
                    if (filament_types && filament_types->values.empty()) continue;
                    const std::string filament_type = filament_types->values[0];
                    std::string filament_type_ = m_filament_type == "PLA Aero" ? "PLA-AERO" : m_filament_type;
                    if (filament_type == filament_type_) {
                        m_printer_compatible_filament_presets[printer_preset.name].push_back(std::make_shared<Preset>(filament_preset));
                    }
                }
            }
        }
    }
}

wxBoxSizer *CreatePresetForPrinterDialog::create_selected_printer_preset_sizer()
{
    wxBoxSizer *select_preseter_preset_sizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText *printer_text = new wxStaticText(this, wxID_ANY, _L("Printer"), wxDefaultPosition, wxDefaultSize);
    select_preseter_preset_sizer->Add(printer_text, 0, wxEXPAND | wxALL, 0);
    m_selected_printer = new ComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, PRINTER_PRESET_MODEL_SIZE, 0, nullptr, wxCB_READONLY);
    select_preseter_preset_sizer->Add(m_selected_printer, 0, wxEXPAND | wxTOP, FromDIP(5));
    
    wxArrayString printer_choices;
    for (std::pair<std::string, std::vector<std::shared_ptr<Preset>>> printer_to_filament_presets : m_printer_compatible_filament_presets) {
        auto compatible_printer_name = printer_to_filament_presets.first;
        if (compatible_printer_name.empty()) {
            BOOST_LOG_TRIVIAL(info)<<__FUNCTION__ << " a printer has no name";
            continue;
        }
        wxString printer_name              = from_u8(compatible_printer_name);
        printer_choices.push_back(printer_name);
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " entry, and visible printer is: " << compatible_printer_name;
    }
    m_selected_printer->Set(printer_choices);

    return select_preseter_preset_sizer;
}

wxBoxSizer *CreatePresetForPrinterDialog::create_selected_filament_preset_sizer()
{
    wxBoxSizer *  select_filament_preset_sizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText *printer_text                 = new wxStaticText(this, wxID_ANY, _L("Copy preset from filament"), wxDefaultPosition, wxDefaultSize);
    select_filament_preset_sizer->Add(printer_text, 0, wxEXPAND | wxALL, 0);
    m_selected_filament = new ComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, PRINTER_PRESET_MODEL_SIZE, 0, nullptr, wxCB_READONLY);
    select_filament_preset_sizer->Add(m_selected_filament, 0, wxEXPAND | wxTOP, FromDIP(5));

    m_selected_printer->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent &e) {
        wxString printer_name = m_selected_printer->GetStringSelection();
        std::unordered_map<string, std::vector<std::shared_ptr<Preset>>>::iterator filament_iter = m_printer_compatible_filament_presets.find(into_u8(printer_name));
        if (m_printer_compatible_filament_presets.end() != filament_iter) {
            filament_choice_to_filament_preset.clear();
            wxArrayString filament_choices;
            for (std::shared_ptr<Preset> filament_preset : filament_iter->second) {
                wxString filament_name                            = wxString::FromUTF8(filament_preset->name);
                filament_choice_to_filament_preset[filament_name] = filament_preset;
                filament_choices.push_back(filament_name);
            }
            m_selected_filament->Set(filament_choices);
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " count of compatible filament presets :" << filament_choices.size();
            if (filament_choices.size()) { m_selected_filament->SetSelection(0); }
        } else {
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "printer preset not find compatible filament presets";
        }
    });

    return select_filament_preset_sizer;
}

wxBoxSizer *CreatePresetForPrinterDialog::create_button_sizer()
{
    wxBoxSizer *bSizer_button = new wxBoxSizer(wxHORIZONTAL);

    bSizer_button->Add(0, 0, 1, wxEXPAND, 0);

    StateColor btn_bg_green(std::pair<wxColour, int>(wxColour(0, 137, 123), StateColor::Pressed), std::pair<wxColour, int>(wxColour(38, 166, 154), StateColor::Hovered),
                            std::pair<wxColour, int>(wxColour(0, 150, 136), StateColor::Normal));

    m_ok_btn = new Button(this, _L("OK"));
    m_ok_btn->SetBackgroundColor(btn_bg_green);
    m_ok_btn->SetBorderColor(*wxWHITE);
    m_ok_btn->SetTextColor(wxColour(0xFFFFFE));
    m_ok_btn->SetFont(Label::Body_12);
    m_ok_btn->SetSize(wxSize(FromDIP(58), FromDIP(24)));
    m_ok_btn->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
    m_ok_btn->SetCornerRadius(FromDIP(12));
    bSizer_button->Add(m_ok_btn, 0, wxRIGHT | wxBOTTOM, FromDIP(10));

    StateColor btn_bg_white(std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Pressed), std::pair<wxColour, int>(wxColour(238, 238, 238), StateColor::Hovered),
                            std::pair<wxColour, int>(*wxWHITE, StateColor::Normal));

    m_cancel_btn = new Button(this, _L("Cancel"));
    m_cancel_btn->SetBackgroundColor(btn_bg_white);
    m_cancel_btn->SetBorderColor(wxColour(38, 46, 48));
    m_cancel_btn->SetFont(Label::Body_12);
    m_cancel_btn->SetSize(wxSize(FromDIP(58), FromDIP(24)));
    m_cancel_btn->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
    m_cancel_btn->SetCornerRadius(FromDIP(12));
    bSizer_button->Add(m_cancel_btn, 0, wxRIGHT | wxBOTTOM, FromDIP(10));

    m_ok_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent &e) {
        wxString selected_printer_name  = m_selected_printer->GetStringSelection();
        std::string printer_name = into_u8(selected_printer_name);
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " add preset: get compatible printer name:";

        wxString filament_preset_name = m_selected_filament->GetStringSelection();
        std::unordered_map<wxString, std::shared_ptr<Preset>>::iterator iter = filament_choice_to_filament_preset.find(filament_preset_name);
        if (filament_choice_to_filament_preset.end() != iter) {
            std::shared_ptr<Preset>  filament_preset = iter->second;
            PresetBundle *           preset_bundle   = wxGetApp().preset_bundle;
            std::vector<std::string> failures;
            DynamicConfig            dynamic_config;
            dynamic_config.set_key_value("filament_vendor", new ConfigOptionStrings({m_filament_vendor}));
            dynamic_config.set_key_value("compatible_printers", new ConfigOptionStrings({printer_name}));
            dynamic_config.set_key_value("filament_type", new ConfigOptionStrings({m_filament_type}));
            bool res = preset_bundle->filaments.clone_presets_for_filament(filament_preset.get(), failures, m_filament_name, m_filament_id, dynamic_config, printer_name);
            if (!res) {
                std::string failure_names;
                for (std::string &failure : failures) { failure_names += failure + "\n"; }
                MessageDialog dlg(this, _L("Some existing presets have failed to be created, as follows:\n") + from_u8(failure_names) + _L("\nDo you want to rewrite it?"),
                                  wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"), wxYES_NO | wxYES_DEFAULT | wxCENTRE);
                if (dlg.ShowModal() == wxID_YES) {
                    res = preset_bundle->filaments.clone_presets_for_filament(filament_preset.get(), failures, m_filament_name, m_filament_id, dynamic_config, printer_name, true);
                    BOOST_LOG_TRIVIAL(info) << "clone filament  have failures  rewritten  is successful? " << res;
                } else {
                    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "have same name preset and not rewritten";
                    return;
                }
            } else {
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "create filament preset successful and name is:" << m_filament_name + " @" + printer_name;
            }

        } else {
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "filament choice not find filament preset and choice is:" << filament_preset_name;
            MessageDialog dlg(this, _L("The filament choice not find filament preset, please reselect it"), wxString(SLIC3R_APP_FULL_NAME) + " - " + _L("Info"),
                              wxYES | wxYES_DEFAULT | wxCENTRE);
            dlg.ShowModal();
            return;
        }

        EndModal(wxID_OK);
        });
    m_cancel_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent &e) { EndModal(wxID_CANCEL); });
    
    return bSizer_button;
}

PresetTree::PresetTree(EditFilamentPresetDialog * dialog)
: m_parent_dialog(dialog)
{}

wxPanel *PresetTree::get_preset_tree(std::pair<std::string, std::vector<std::shared_ptr<Preset>>> printer_and_presets)
{
    wxBoxSizer *sizer           = new wxBoxSizer(wxVERTICAL);
    wxPanel *   parent          = m_parent_dialog->get_preset_tree_panel();
    wxPanel *   panel           = new wxPanel(parent);
    wxColour    backgroundColor = parent->GetBackgroundColour();
    panel->SetBackgroundColour(backgroundColor);
    const std::string &printer_name = printer_and_presets.first;
    sizer->Add(get_root_item(panel, printer_name), 0, wxEXPAND, 0);
    int child_count = printer_and_presets.second.size();
    for (int i = 0; i < child_count; i++) {
        if (i == child_count - 1) {
            sizer->Add(get_child_item(panel, printer_and_presets.second[i], printer_name, i, true), 0, wxEXPAND, 0);
        } else {
            sizer->Add(get_child_item(panel, printer_and_presets.second[i], printer_name, i, false), 0, wxEXPAND, 0);
        }
    }
    panel->SetSizerAndFit(sizer);
    return panel;
}

wxPanel *PresetTree::get_root_item(wxPanel *parent, const std::string &printer_name)
{
    wxBoxSizer *sizer           = new wxBoxSizer(wxHORIZONTAL);
    wxPanel *   panel           = new wxPanel(parent);
    wxColour    backgroundColor = parent->GetBackgroundColour();
    panel->SetBackgroundColour(backgroundColor);
    wxStaticText *preset_name = new wxStaticText(panel, wxID_ANY, from_u8(printer_name));
    preset_name->SetFont(Label::Body_11);
    preset_name->SetForegroundColour(*wxBLACK);
    sizer->Add(preset_name, 0, wxEXPAND | wxALL, 5);
    panel->SetSizer(sizer);

    return panel;
}

wxPanel *PresetTree::get_child_item(wxPanel *parent, std::shared_ptr<Preset> preset, std::string printer_name, int preset_index, bool is_last)
{
    wxBoxSizer *sizer           = new wxBoxSizer(wxHORIZONTAL);
    wxPanel *   panel           = new wxPanel(parent);
    wxColour    backgroundColor = parent->GetBackgroundColour();
    panel->SetBackgroundColour(backgroundColor);
    sizer->Add(0, 0, 0, wxLEFT, 10);
    wxPanel *line_left = new wxPanel(panel, wxID_ANY, wxDefaultPosition, is_last ? wxSize(1, 12) : wxSize(1, -1));
    line_left->SetBackgroundColour(*wxBLACK);
    sizer->Add(line_left, 0, is_last ? wxALL : wxEXPAND | wxALL, 0);
    wxPanel *line_right = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxSize(10, 1));
    line_right->SetBackgroundColour(*wxBLACK);
    sizer->Add(line_right, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);
    sizer->Add(0, 0, 0, wxLEFT, 5);
    wxStaticText *preset_name = new wxStaticText(panel, wxID_ANY, from_u8(preset->name));
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " create child item: " << preset->name;
    preset_name->SetFont(Label::Body_10);
    preset_name->SetForegroundColour(*wxBLACK);
    sizer->Add(preset_name, 0, wxEXPAND | wxALL, 5);
    bool base_id_error = false;
    if (preset->inherits() == "" && preset->base_id != "") base_id_error = true;
    if (base_id_error) {
        std::string      wiki_url             = "https://wiki.bambulab.com/en/software/bambu-studio/custom-filament-issue";
        wxHyperlinkCtrl *m_download_hyperlink = new wxHyperlinkCtrl(panel, wxID_ANY, _L("[Delete Required]"), wiki_url, wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE);
        m_download_hyperlink->SetFont(Label::Body_10);
        sizer->Add(m_download_hyperlink, 0, wxEXPAND | wxALL, 5);
    }
    sizer->Add(0, 0, 1, wxEXPAND, 0);

    StateColor flush_bg_col(std::pair<wxColour, int>(wxColour(219, 253, 231), StateColor::Pressed), std::pair<wxColour, int>(wxColour(238, 238, 238), StateColor::Hovered),
                            std::pair<wxColour, int>(wxColour(238, 238, 238), StateColor::Normal));

    StateColor flush_fg_col(std::pair<wxColour, int>(wxColour(107, 107, 106), StateColor::Pressed), std::pair<wxColour, int>(wxColour(107, 107, 106), StateColor::Hovered),
                            std::pair<wxColour, int>(wxColour(107, 107, 106), StateColor::Normal));

    StateColor flush_bd_col(std::pair<wxColour, int>(wxColour(0, 150, 136), StateColor::Pressed), std::pair<wxColour, int>(wxColour(0, 150, 136), StateColor::Hovered),
                            std::pair<wxColour, int>(wxColour(172, 172, 172), StateColor::Normal));

    StateColor btn_bg_green(std::pair<wxColour, int>(wxColour(0, 137, 123), StateColor::Pressed), std::pair<wxColour, int>(wxColour(38, 166, 154), StateColor::Hovered),
                            std::pair<wxColour, int>(wxColour(0, 150, 136), StateColor::Normal));

    Button *edit_preset_btn = new Button(panel, _L("Edit Preset"));
    edit_preset_btn->SetFont(Label::Body_10);
    edit_preset_btn->SetPaddingSize(wxSize(8, 3));
    edit_preset_btn->SetCornerRadius(8);
    edit_preset_btn->SetBackgroundColor(flush_bg_col);
    edit_preset_btn->SetBorderColor(flush_bd_col);
    edit_preset_btn->SetTextColor(flush_fg_col);
    //edit_preset_btn->Hide();
    sizer->Add(edit_preset_btn, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);
    sizer->Add(0, 0, 0, wxLEFT, 5);

    Button *del_preset_btn = new Button(panel, _L("Delete Preset"));
    del_preset_btn->SetFont(Label::Body_10);
    del_preset_btn->SetPaddingSize(wxSize(8, 3));
    del_preset_btn->SetCornerRadius(8);
    if (base_id_error) {
        del_preset_btn->SetBackgroundColor(btn_bg_green);
        del_preset_btn->SetBorderColor(btn_bg_green);
        del_preset_btn->SetTextColor(wxColour(0xFFFFFE));
    } else {
        del_preset_btn->SetBackgroundColor(flush_bg_col);
        del_preset_btn->SetBorderColor(flush_bd_col);
        del_preset_btn->SetTextColor(flush_fg_col);
    }
    
    //del_preset_btn->Hide();
    sizer->Add(del_preset_btn, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);

    edit_preset_btn->Bind(wxEVT_BUTTON, [this, printer_name, preset_index](wxCommandEvent &e) {
        wxGetApp().CallAfter([this, printer_name, preset_index]() { edit_preset(printer_name, preset_index); });
    });
    del_preset_btn->Bind(wxEVT_BUTTON, [this, printer_name, preset_index](wxCommandEvent &e) {
        wxGetApp().CallAfter([this, printer_name, preset_index]() { delete_preset(printer_name, preset_index); });
    });

    panel->SetSizer(sizer);

    return panel;
}

void PresetTree::delete_preset(std::string printer_name, int need_delete_preset_index)
{
    m_parent_dialog->set_printer_name(printer_name);
    m_parent_dialog->set_need_delete_preset_index(need_delete_preset_index);
    m_parent_dialog->delete_preset();
}

void PresetTree::edit_preset(std::string printer_name, int need_edit_preset_index)
{
    m_parent_dialog->set_printer_name(printer_name);
    m_parent_dialog->set_need_edit_preset_index(need_edit_preset_index);
    m_parent_dialog->edit_preset();
}

// BBS: fix scroll to tip view
class DetailScrolledWindow : public wxScrolledWindow
{
public:
    DetailScrolledWindow(wxWindow* parent)
        : wxScrolledWindow(parent,
            wxID_ANY,
            wxDefaultPosition,
            wxDefaultSize,
            wxVSCROLL) // hide hori-bar will cause hidden field mis-position
    {
        SetBackgroundColour(*wxWHITE);

        // ShowScrollBar(GetHandle(), SB_BOTH, FALSE);
        Bind(wxEVT_SCROLL_CHANGED, [this](auto& e) {
            wxWindow* child = dynamic_cast<wxWindow*>(e.GetEventObject());
            if (child != this)
                EnsureVisible(child);
            });
    }
    virtual bool ShouldScrollToChildOnFocus(wxWindow* child)
    {
        EnsureVisible(child);
        return false;
    }
    void EnsureVisible(wxWindow* win)
    {
        const wxRect viewRect(m_targetWindow->GetClientRect());
        const wxRect winRect(m_targetWindow->ScreenToClient(win->GetScreenPosition()), win->GetSize());
        if (viewRect.Contains(winRect)) {
            return;
        }
        if (winRect.GetWidth() > viewRect.GetWidth() || winRect.GetHeight() > viewRect.GetHeight()) {
            return;
        }
        int stepx, stepy;
        GetScrollPixelsPerUnit(&stepx, &stepy);

        int startx, starty;
        GetViewStart(&startx, &starty);
        // first in vertical direction:
        if (stepy > 0) {
            int diff = 0;

            if (winRect.GetTop() < 0) {
                diff = winRect.GetTop();
            }
            else if (winRect.GetBottom() > viewRect.GetHeight()) {
                diff = winRect.GetBottom() - viewRect.GetHeight() + 1;
                // round up to next scroll step if we can't get exact position,
                // so that the window is fully visible:
                diff += stepy - 1;
            }
            starty = (starty * stepy + diff) / stepy;
        }
        // then horizontal:
        if (stepx > 0) {
            int diff = 0;
            if (winRect.GetLeft() < 0) {
                diff = winRect.GetLeft();
            }
            else if (winRect.GetRight() > viewRect.GetWidth()) {
                diff = winRect.GetRight() - viewRect.GetWidth() + 1;
                // round up to next scroll step if we can't get exact position,
                // so that the window is fully visible:
                diff += stepx - 1;
            }
            startx = (startx * stepx + diff) / stepx;
        }
        Scroll(startx, starty);
    }
};

struct NozzleSelected
{
    int index;
    bool modify;
    std::string model;
    int selected_size;
    std::vector<std::string> diameters;
    std::vector<int> selected;
};

class ClientDataVender : public wxClientData
{
public:
    ClientDataVender()
    {}

    ClientDataVender(const wxString& vendor)
        : m_text(vendor)
    {}

    void SetText(const wxString& text)
    {
        m_text = text;
    }

    wxString GetText() const
    {
        return m_text;
    }

private:
    wxString m_text;
};

class SelectPrinterPresetPanel : public wxPanel
{
public:
    SelectPrinterPresetPanel(nlohmann::json& profile_json, wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(1800, 1080), long style = wxTAB_TRAVERSAL, const wxString& type = wxEmptyString);
    ~SelectPrinterPresetPanel();

private:
    void UpdateProfile(nlohmann::json& profile_json, const std::vector<NozzleSelected>& nozzle_selected);

    wxDataViewTreeCtrl* _OnInitVendorTree(nlohmann::json& profile_json);
    wxBoxSizer* _OnInitVendorView(nlohmann::json& profile_json);
    void _OnInitAddModel(wxBoxSizer* detail_sizer, std::unordered_map<wxString, wxBoxSizer*>& vendor_view, std::unordered_map<std::string, std::pair<wxBoxSizer*, int>>& vendor_view_last_item, NozzleSelected& item, nlohmann::json& profile_model);
    void _OnInitAddModel(wxBoxSizer* detail_sizer, std::unordered_map<wxString, wxBoxSizer*>& vendor_view, std::unordered_map<std::string, std::pair<wxBoxSizer*, int>>& vendor_view_last_item, NozzleSelected& item, nlohmann::json& profile_model, const std::string& vendor);
    void _OnUpdateVendorView(const wxString& name);

private:
    wxBoxSizer* m_detail_sizer{ nullptr };
    wxScrolledWindow* m_detail_view{ nullptr };
    wxBoxSizer* m_last_vendor_view{ nullptr };
    std::unordered_map<wxString, wxBoxSizer*> m_vendor_view;

    std::vector<NozzleSelected> m_nozzle_selected;
    std::vector<std::pair<int, int>> m_nozzle_selected_index;
};

struct ProfileFilament
{
    std::string name;
    std::string vendor;
    std::string type;
    bool vendor_checked;
    bool type_checked;
    bool is_system;
    bool is_selected;
    bool is_selected_init;
};

static std::vector<std::string> Loc_GetMaterials(const std::string& materials)
{
    std::vector<std::string> results;
    int idx = 0;
    while (idx != std::string::npos && idx < materials.size())
    {
        int index = materials.find(';', idx);
        if (idx < index)
        {
            std::string nozzle = materials.substr(idx, index - idx);
            results.emplace_back(nozzle);
            idx = index + 1;
        }
        else if (index == std::string::npos)
        {
            std::string nozzle = materials.substr(idx);
            results.emplace_back(nozzle);
            idx = index;
        }
        else
        {
            idx = std::string::npos;
        }
    }
    return results;
}

SelectPrinterPresetPanel::SelectPrinterPresetPanel(nlohmann::json& profile_json, wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& type)
    : wxPanel(parent, id, pos, size, style, type)
{
    wxGetApp().UpdateDarkUIWin(this);
    this->SetBackgroundColour(*wxWHITE);

    std::set<std::string> other_vendors;
    std::unordered_map<std::string, wxBoxSizer*> vendor_view;
    std::unordered_map<std::string, std::pair<wxBoxSizer*, int>> vendor_view_last_item;

    Label* label_system = new Label(this, _L("System"), LB_PROPAGATE_MOUSE_EVENT);
    //Label* label_user = new Label(this, _L("User"), LB_PROPAGATE_MOUSE_EVENT);
    wxBoxSizer* type_sizer = new wxBoxSizer(wxHORIZONTAL);
    type_sizer->AddSpacer(10 * em_unit(this) / 10);
    type_sizer->Add(label_system, 0, wxALIGN_CENTER);
    type_sizer->AddSpacer(10 * em_unit(this) / 10);
    //type_sizer->Add(label_user, 0, wxALIGN_LEFT);

    m_detail_view = new DetailScrolledWindow(this);
    m_detail_view->SetBackgroundColour(*wxWHITE);

    m_detail_sizer = _OnInitVendorView(profile_json);
    wxDataViewTreeCtrl* vendor_veiw = _OnInitVendorTree(profile_json);

    m_detail_view->SetSizer(m_detail_sizer);
    m_detail_view->SetScrollbars(1, 20, 1, 2);

    wxBoxSizer * right_sizer = new wxBoxSizer(wxVERTICAL);
    right_sizer->Add(type_sizer, 0, wxALIGN_CENTER);
    right_sizer->AddSpacer(10 * em_unit(this) / 10);
    right_sizer->Add(m_detail_view, 4, wxEXPAND);

    wxBoxSizer* sizer_data = new wxBoxSizer(wxHORIZONTAL);
    sizer_data->AddSpacer(10 * em_unit(this) / 10);
    sizer_data->Add(vendor_veiw, 1, wxEXPAND);
    sizer_data->AddSpacer(5 * em_unit(this) / 10);
    sizer_data->Add(right_sizer, 4, wxEXPAND);
    sizer_data->AddSpacer(10 * em_unit(this) / 10);

    StateColor btn_bg_white(std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(238, 238, 238), StateColor::Hovered),
        std::pair<wxColour, int>(*wxWHITE, StateColor::Normal));

    wxBoxSizer* sizer_button = new wxBoxSizer(wxHORIZONTAL);
    Button* button_confirm = new Button(this, _L("Confirm"));
    {
        button_confirm->SetBackgroundColor(btn_bg_white);
        button_confirm->SetBorderColor(wxColour(38, 46, 48));
        //button_confirm->SetTextColor(wxColour(0xFFFFFE));
        button_confirm->SetFont(Label::Body_12);
        button_confirm->SetSize(wxSize(FromDIP(58), FromDIP(24)));
        button_confirm->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
        button_confirm->SetCornerRadius(FromDIP(12));
    }
    Button* button_cancel = new Button(this, _L("Cancel"));
    {
        button_cancel->SetBackgroundColor(btn_bg_white);
        button_cancel->SetBorderColor(wxColour(38, 46, 48));
        //button_cancel->SetTextColor(wxColour(0xFFFFFE));
        button_cancel->SetFont(Label::Body_12);
        button_cancel->SetSize(wxSize(FromDIP(58), FromDIP(24)));
        button_cancel->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
        button_cancel->SetCornerRadius(FromDIP(12));
    }
    sizer_button->Add(button_confirm, 0, wxRIGHT | wxBOTTOM, FromDIP(10));
    sizer_button->Add(button_cancel, 0, wxRIGHT, FromDIP(10));

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddSpacer(10 * em_unit(this) / 10);
    sizer->Add(sizer_data, 1, wxEXPAND);
    sizer->AddSpacer(10 * em_unit(this) / 10);
    sizer->Add(sizer_button, 0, wxALIGN_RIGHT|wxRIGHT|wxBOTTOM);
    sizer->AddSpacer(10 * em_unit(this) / 10);
    
    SetSizer(sizer);

    for (auto iter : m_vendor_view)
    {
        m_detail_sizer->Hide(iter.second, true);
    }

    _OnUpdateVendorView(_L("Flagship"));


    button_confirm->Bind(wxEVT_BUTTON, [this, &profile_json](wxCommandEvent&)
        {
            UpdateProfile(profile_json, m_nozzle_selected);
            
            dynamic_cast<SelectPrinterPresetDialog*>(this->GetParent())->EndModal(wxID_OK);
        });

    button_cancel->Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
        {
            dynamic_cast<SelectPrinterPresetDialog*>(this->GetParent())->EndModal(wxID_CANCEL);
        });

    this->Layout();
}

SelectPrinterPresetPanel::~SelectPrinterPresetPanel()
{

}

void SelectPrinterPresetPanel::UpdateProfile(nlohmann::json& profile_json, const std::vector<NozzleSelected>& nozzle_selected)
{
    PresetCollection &filaments_presets = wxGetApp().preset_bundle->filaments;
    std::set<std::string> selected_filaments;
    std::set<std::string> un_selected_filaments;

    for (int i=0; i< nozzle_selected.size(); ++i)
    {
        const NozzleSelected& nozzle = nozzle_selected[i];

        if (profile_json["model"][i]["materials"].is_string())
        {
            std::vector<std::string> materials = Loc_GetMaterials(profile_json["model"][i]["materials"]);
            if (nozzle.selected_size > 0)
            {
                for (auto& material : materials)
                {
                    Preset* preset = filaments_presets.find_preset(material,false);
                    if(preset)
                        selected_filaments.emplace(material);
                    else
                     {
                        size_t pos = material.find("@");
                        if(pos == std::string::npos) {
                            for(int i = 0; i < nozzle.diameters.size(); ++i)
                            {
                                if(nozzle.selected[i] > 0)
                                {
                                    std::string new_material = material + " @"+ nozzle.model + " " + nozzle.diameters[i] + " nozzle";
                                    Preset* preset = filaments_presets.find_preset(new_material,false);
                                    if(preset)
                                        selected_filaments.emplace(new_material);
                                }
                            }
                        }
                     };
                }
            }
            else
            {
                for (auto& material : materials)
                {
                    Preset* preset = filaments_presets.find_preset(material,false);
                    if(preset)
                        un_selected_filaments.emplace(material);
                    else{
                        size_t pos = material.find("@");
                        if(pos == std::string::npos) {
                            for(int i = 0; i < nozzle.diameters.size(); ++i)
                            {
                                if(nozzle.selected[i] > 0)
                                {
                                    std::string new_material = material + " @"+ nozzle.model + " " + nozzle.diameters[i] + " nozzle";
                                    Preset* preset = filaments_presets.find_preset(new_material,false);
                                    if(preset)
                                        un_selected_filaments.emplace(new_material);
                                }
                            }
                        }
                    }
                }
            }
        }

        if(!nozzle.modify)
            continue;

        std::string selected;
        if (nozzle.selected_size > 0)
        {
            for (int j = 0; j < nozzle.diameters.size(); ++j)
            {
                if (nozzle.selected[j] <= 0)
                    continue;

                if (selected.empty())
                {
                    selected = nozzle.diameters[j];
                }
                else
                {
                    selected += ";" + nozzle.diameters[j];
                }
            }
        }

        profile_json["model"][i]["nozzle_selected"] = selected;
    }

    std::vector<std::string> diff_material;
    std::set_difference(
        un_selected_filaments.begin(), un_selected_filaments.end(), 
        selected_filaments.begin(), selected_filaments.end(),
        std::inserter(diff_material, diff_material.begin()));

    // set default material
    for (auto& material : selected_filaments)
    {
        if (profile_json["filament"].contains(material))
        {
            profile_json["filament"][material]["selected"] = 1;
        }
    }
    // remove un-used material
    for (auto& material : diff_material)
    {
        if (profile_json["filament"].contains(material))
        {
            profile_json["filament"][material]["selected"] = 0;
        }
    }

}

wxDataViewTreeCtrl* SelectPrinterPresetPanel::_OnInitVendorTree(nlohmann::json& profile_json)
{
    nlohmann::json& profile_model = profile_json["model"];
    std::set<std::string> other_vendors;
    for (json::iterator it = profile_model.begin(); it != profile_model.end(); ++it)
    {
        std::string vendor = (*it)["vendor"];
        if (vendor != "Creality")
        {
            other_vendors.emplace(vendor);
        }
    }

    wxDataViewTreeCtrl* vendor_view = new wxDataViewTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_NO_HEADER | wxBORDER_SIMPLE);

    wxDataViewItem item_creality = vendor_view->AppendContainer(wxDataViewItem(nullptr), _L("Creality"));
    {
        vendor_view->AppendItem(item_creality, _L("Flagship"), wxWithImages::NO_IMAGE, new ClientDataVender("Flagship"));
        vendor_view->AppendItem(item_creality, _L("CR"), wxWithImages::NO_IMAGE, new ClientDataVender("CR"));
        vendor_view->AppendItem(item_creality, _L("Ender"), wxWithImages::NO_IMAGE, new ClientDataVender("Ender"));
        vendor_view->AppendItem(item_creality, _L("Others"), wxWithImages::NO_IMAGE, new ClientDataVender("Others"));
    }

    wxDataViewItem item_others = vendor_view->AppendContainer(wxDataViewItem(nullptr), _L("Others"));
    for (auto& vendor : other_vendors)
    {
        vendor_view->AppendItem(item_others, _L(vendor), wxWithImages::NO_IMAGE, new ClientDataVender(vendor));
    }

    vendor_view->Expand(item_creality);
    vendor_view->Expand(item_others);

    vendor_view->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, [this](wxCommandEvent& evt)
        {
            wxDataViewTreeCtrl* vendor_view = dynamic_cast<wxDataViewTreeCtrl*>(evt.GetEventObject());
            wxDataViewItem item = vendor_view->GetSelection();
            wxClientData* item_data = vendor_view->GetItemData(item);
            if (item_data)
            {
                ClientDataVender* data = dynamic_cast<ClientDataVender*>(item_data);
                wxString name = data->GetText();
                _OnUpdateVendorView(name);
            }
        });

    vendor_view->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, [this](wxDataViewEvent& evt)
        {
            evt.Veto();
        });

    vendor_view->Bind(wxEVT_DATAVIEW_ITEM_START_EDITING, [this](wxDataViewEvent& evt)
        {
            evt.Veto();
        });

    return vendor_view;
}

wxBoxSizer* SelectPrinterPresetPanel::_OnInitVendorView(nlohmann::json& profile_json)
{
    wxBoxSizer* detail_sizer = new wxBoxSizer(wxVERTICAL);

    std::unordered_map<std::string, std::pair<wxBoxSizer*, int>> vendor_view_last_item;
    nlohmann::json& profile_model = profile_json["model"];
    m_nozzle_selected.resize(profile_model.size());
    int index = 0;
    for (json::iterator it = profile_model.begin(); it != profile_model.end(); ++it)
    {
        m_nozzle_selected[index].index = index;
        _OnInitAddModel(detail_sizer, m_vendor_view, vendor_view_last_item, m_nozzle_selected[index], *it);
        index++;
    }

    return detail_sizer;
}

void SelectPrinterPresetPanel::_OnInitAddModel(wxBoxSizer* detail_sizer, std::unordered_map<wxString, wxBoxSizer*>& vendor_view, std::unordered_map<std::string, std::pair<wxBoxSizer*, int>>& vendor_view_last_item, NozzleSelected& item, nlohmann::json& profile_model)
{
    std::string vendor = profile_model["vendor"];
    if (vendor == "Creality")
    {
        std::string model = profile_model["model"];
        if (model.find("K1") != std::string::npos || model.find("K2") != std::string::npos || model.find("K3") != std::string::npos)
        {
            _OnInitAddModel(detail_sizer, vendor_view, vendor_view_last_item, item, profile_model, "Flagship");
        }
        else if (model.find("CR") != std::string::npos)
        {
            _OnInitAddModel(detail_sizer, vendor_view, vendor_view_last_item, item, profile_model, "CR");
        }
        else if (model.find("Ender") != std::string::npos)
        {
            _OnInitAddModel(detail_sizer, vendor_view, vendor_view_last_item, item, profile_model, "Ender");
        }
        else
        {
            _OnInitAddModel(detail_sizer, vendor_view, vendor_view_last_item, item, profile_model, "Others");
        }
    }
    else
    {
        _OnInitAddModel(detail_sizer, vendor_view, vendor_view_last_item, item, profile_model, vendor);
    }
}

static std::vector<std::string> Loc_GetNozzleDiameter(const std::string& diameters)
{
    std::vector<std::string> nozzles;
    int idx = 0;
    while (idx != std::string::npos && idx < diameters.size())
    {
        int index = diameters.find(';', idx);
        if (idx < index)
        {
            std::string nozzle = diameters.substr(idx, index - idx);
            nozzles.emplace_back(nozzle);
            idx = index + 1;
        }
        else if (index == std::string::npos)
        {
            std::string nozzle = diameters.substr(idx);
            nozzles.emplace_back(nozzle);
            idx = index;
        }
        else
        {
            idx = std::string::npos;
        }
    }
    return nozzles;
}

static std::vector<std::string> Loc_GetModel(const std::string& models)
{
    std::vector<std::string> result;
    int idx = 0;
    while (idx != std::string::npos && idx < models.size())
    {
        int index1 = models.find('[', idx);
        if (index1 == std::string::npos)
        {
            idx = index1 + 1;
            continue;
        }

        int index2 = models.find(']', index1);
        if (index2 == std::string::npos)
        {
            idx = index2 + 1;
            continue;
        }

        if (index1 + 1 >= index2)
        {
            idx = index2 + 1;
            continue;
        }

        std::string model = models.substr(index1+1, index2-index1-1);
        result.emplace_back(model);
        idx = index2 + 1;
    }

    return result;
}

void SelectPrinterPresetPanel::_OnInitAddModel(wxBoxSizer* detail_sizer, std::unordered_map<wxString, wxBoxSizer*>& vendor_view, std::unordered_map<std::string, std::pair<wxBoxSizer*, int>>& vendor_view_last_item, NozzleSelected& item, nlohmann::json& profile_model, const std::string& vendor)
{
    static wxPNGHandler* s_handler = nullptr;
    if (!s_handler)
    {
        s_handler = new wxPNGHandler;
        wxImage::AddHandler(s_handler);
    }

    if (vendor_view.count(vendor) <= 0)
    {
        wxBoxSizer* sizer_vendor = new wxBoxSizer(wxVERTICAL);
        Label* labe_vendor = new Label(m_detail_view, _L(vendor), LB_PROPAGATE_MOUSE_EVENT);
        sizer_vendor->Add(labe_vendor, 0, wxALIGN_LEFT);
        sizer_vendor->AddSpacer(10 * em_unit(m_detail_view) / 10);
        vendor_view[vendor] = sizer_vendor;
        vendor_view_last_item[vendor] = std::pair<wxBoxSizer*, int>(nullptr, 0);
        detail_sizer->Add(sizer_vendor, 0, wxALIGN_LEFT);
    }

    wxBoxSizer* sizer_row = vendor_view_last_item[vendor].first;
    if (!vendor_view_last_item[vendor].first || vendor_view_last_item[vendor].second==3)
    {
        wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
        vendor_view_last_item[vendor] = std::pair<wxBoxSizer*, int>(sizer, 0);
        sizer_row = sizer;
        sizer_row->AddSpacer(10 * em_unit(m_detail_view) / 10);
    }

    // image
    const std::string cover_path = profile_model["cover"];
    wxStaticBitmap* bitmap = new wxStaticBitmap(m_detail_view, wxID_ANY, create_scaled_bitmap(cover_path, this, 200));
    bitmap->SetBackgroundColour(wxColour(0, 0, 0));
    // model
    const std::string model = profile_model["model"];
    Label* label_model = new Label(m_detail_view, _L(model), LB_PROPAGATE_MOUSE_EVENT);
    // nozzle
    std::vector<std::string> nozzle_diameter = Loc_GetNozzleDiameter(profile_model["nozzle_diameter"]);
    std::vector<std::string> nozzle_selected = Loc_GetNozzleDiameter(profile_model["nozzle_selected"]);
    item.modify = false;
    item.model = model;
    item.diameters = nozzle_diameter;
    item.selected_size = 0;
    item.selected.resize(nozzle_diameter.size(), 0);
    wxBoxSizer* sizer_nozzle = new wxBoxSizer(wxVERTICAL);
    for (int i = 0; i < nozzle_diameter.size(); ++i)
    {
        wxCheckBox* checkbox_nozzle = new wxCheckBox(m_detail_view, wxID_ANY, _L(nozzle_diameter[i] + " mm"));
        sizer_nozzle->Add(checkbox_nozzle, 0, wxALIGN_LEFT);
        for (int j = 0; j < nozzle_selected.size(); ++j)
        {
            if (nozzle_diameter[i] != nozzle_selected[j])
                continue;
            checkbox_nozzle->SetValue(true);
            item.selected[i] = 1;
            item.selected_size++;
        }

        m_nozzle_selected_index.emplace_back(std::pair<int, int>(item.index, i));
        void* ptr = reinterpret_cast<void*>(m_nozzle_selected_index.size() - 1);
        checkbox_nozzle->SetClientData(ptr);
        checkbox_nozzle->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& evt)
            {
                wxCheckBox* checkbox_nozzle = dynamic_cast<wxCheckBox*>(evt.GetEventObject());
                uint64_t index = reinterpret_cast<std::uintptr_t>(checkbox_nozzle->GetClientData());
                bool checked = checkbox_nozzle->GetValue();
                NozzleSelected& nozzle_selected = m_nozzle_selected[m_nozzle_selected_index[index].first];
                nozzle_selected.modify = true;
                nozzle_selected.selected_size += checked ? 1 : -1;
                nozzle_selected.selected[m_nozzle_selected_index[index].second] = checked ? 1 : 0;
            });
    }

    wxBoxSizer* sizer_item = new wxBoxSizer(wxVERTICAL);
    sizer_item->Add(bitmap, 0, wxALIGN_LEFT, 0);
    sizer_item->Add(label_model, 0, wxALIGN_LEFT);
    sizer_item->Add(sizer_nozzle, 0, wxALIGN_LEFT);

    sizer_row->Add(sizer_item, 0, wxALIGN_LEFT);
    sizer_row->AddSpacer(20 * em_unit(m_detail_view) / 10);
    vendor_view_last_item[vendor].second += 1;

    if (vendor_view_last_item[vendor].second == 1)
    {
        wxBoxSizer* sizer_vendor = vendor_view[vendor];
        sizer_vendor->Add(sizer_row, 0, wxALIGN_LEFT);
        sizer_vendor->AddSpacer(20 * em_unit(m_detail_view) / 10);
    }
}

void SelectPrinterPresetPanel::_OnUpdateVendorView(const wxString& name)
{
    auto iter = m_vendor_view.find(name);
    if (iter == m_vendor_view.end())
    {
        m_detail_view->Layout();
        return;
    }

    if (m_last_vendor_view)
    {
        m_detail_sizer->Hide(m_last_vendor_view, true);
        m_last_vendor_view = nullptr;
    }

    m_last_vendor_view = iter->second;

    if (!m_last_vendor_view)
    {
        m_detail_view->Layout();
        return;
    }

    m_detail_sizer->Show(m_last_vendor_view, true);

    m_detail_view->SetVirtualSize(m_detail_view->GetBestVirtualSize());
}

SelectPrinterPresetDialog::SelectPrinterPresetDialog(nlohmann::json& profile_json, wxWindow* parent)
    : DPIDialog(parent ? parent : nullptr, wxID_ANY, _L("Select/Remove Printer"), wxDefaultPosition, wxDefaultSize, wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX | wxCENTRE)
{
    this->SetBackgroundColour(*wxWHITE);
    this->SetSize(wxSize(FromDIP(900), FromDIP(600)));

    m_panel = new SelectPrinterPresetPanel(profile_json, this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBK_LEFT | wxTAB_TRAVERSAL);
    
    this->Center();

    wxGetApp().UpdateDlgDarkUI(this);
}

SelectPrinterPresetDialog::~SelectPrinterPresetDialog()
{

}

void SelectPrinterPresetDialog::on_dpi_changed(const wxRect& suggested_rect)
{
    Fit();
    SetSize({ 75 * em_unit(), 60 * em_unit() });
    //m_panel->msw_rescale();
    Refresh();
}

} // namespace GUI

} // namespace Slic3r
