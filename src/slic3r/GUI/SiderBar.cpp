#include "SiderBar.h"
#include "Plater.hpp"

#include <string>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/bmpcbox.h>
#include <wx/statbox.h>
#include <wx/statbmp.h>
#include <wx/filedlg.h>
#include <wx/dnd.h>
#include <wx/progdlg.h>
#include <wx/string.h>
#include <wx/wupdlock.h>
#include <wx/numdlg.h>
#include <wx/debug.h>
#include <wx/busyinfo.h>
#include <wx/event.h>
#include <wx/wrapsizer.h>

#include <algorithm>
#include <cmath>
#include <locale>
#include <sstream>
#include <set>
#include <nlohmann/json.hpp>

#include "GUI.hpp"
#include "MsgDialog.hpp"
#include "PresetComboBoxes.hpp"
#include "PhysicalPrinterDialog.hpp"
#include "Tab.hpp"

namespace Slic3r {
namespace GUI {

struct SidebarPrinter::priv
{
    Plater*               plater                  = nullptr;
    PlaterPresetComboBox* combo_printer           = nullptr;
    ScalableButton*       edit_btn                = nullptr;
    ScalableButton*       connection_btn          = nullptr;
    ComboBox*             m_bed_type_list         = nullptr;
    StaticBox*            m_panel_printer_title   = nullptr;
    ScalableButton*       m_printer_icon          = nullptr;
    Label*                m_text_printer_settings = nullptr;
    ScalableButton*       m_printer_setting       = nullptr;
    wxBoxSizer* hsizer_printer = nullptr;
};

bool SidebarPrinter::bShow = false;
SidebarPrinter::SidebarPrinter(Plater* parent): wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(400, 150)), p(new priv())
{
#ifdef __WXGTK__
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetDoubleBuffered(true);
    Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) {});
#endif
    p->plater = parent;

    wxBoxSizer* vsizer_printer = new wxBoxSizer(wxVERTICAL);
    wxColour    title_bg       = wxColour(248, 248, 248);
    int         em             = wxGetApp().em_unit();
    /***************** 1. create printer title bar    **************/
    // 1.1 create title bar resources
    p->m_panel_printer_title = new StaticBox(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_NONE);
    p->m_panel_printer_title->SetBackgroundColor(title_bg);
    p->m_panel_printer_title->SetBackgroundColor2(0xF1F1F1);

    p->m_printer_icon          = new ScalableButton(p->m_panel_printer_title, wxID_ANY, "printer");
    p->m_text_printer_settings = new Label(p->m_panel_printer_title, _L("Printer"), LB_PROPAGATE_MOUSE_EVENT);

    p->m_printer_icon->Bind(wxEVT_BUTTON, [this](wxCommandEvent& e) {
        // auto wizard_t = new ConfigWizard(wxGetApp().mainframe);
        // wizard_t->run(ConfigWizard::RR_USER, ConfigWizard::SP_CUSTOM);
    });

    p->m_printer_setting = new ScalableButton(p->m_panel_printer_title, wxID_ANY, "settings");
    p->m_printer_setting->Bind(wxEVT_BUTTON, [this](wxCommandEvent& e) {
        // p->editing_filament = -1;
        // wxGetApp().params_dialog()->Popup();
        // wxGetApp().get_tab(Preset::TYPE_FILAMENT)->restore_last_select_item();
        wxGetApp().run_wizard(ConfigWizard::RR_USER, ConfigWizard::SP_PRINTERS);
    });

    wxBoxSizer* h_sizer_title = new wxBoxSizer(wxHORIZONTAL);
    h_sizer_title->Add(p->m_printer_icon, 0, wxALIGN_CENTRE | wxLEFT | wxRIGHT, em);
    h_sizer_title->Add(p->m_text_printer_settings, 0, wxALIGN_CENTER);
    h_sizer_title->AddStretchSpacer();
    h_sizer_title->Add(p->m_printer_setting, 0, wxALIGN_CENTER);
    h_sizer_title->Add(15 * em / 10, 0, 0, 0, 0);
    h_sizer_title->SetMinSize(-1, 3 * em);

    p->m_panel_printer_title->SetSizer(h_sizer_title);
    p->m_panel_printer_title->Layout();

    // 1.2 Add spliters around title bar
    // add spliter 1
    // auto spliter_1 = new ::StaticLine(p->scrolled);
    // spliter_1->SetBackgroundColour("#A6A9AA");
    // scrolled_sizer->Add(spliter_1, 0, wxEXPAND);

    // add printer title
    vsizer_printer->Add(p->m_panel_printer_title, 0, wxEXPAND | wxALL, 0);

    p->m_panel_printer_title->Bind(wxEVT_LEFT_UP, [this](auto& e) { /*this->Hide();*/ });

    //         // add spliter 2
    //         auto spliter_2 = new ::StaticLine(p->scrolled);
    //         spliter_2->SetLineColour("#CECECE");
    //         scrolled_sizer->Add(spliter_2, 0, wxEXPAND);

    /***************** 2. create printer context  **************/
    this->SetBackgroundColour(wxColour(255, 255, 255));
    p->combo_printer = new PlaterPresetComboBox(this, Preset::TYPE_PRINTER);
    this->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent& e) {
        Slic3r::GUI::wxGetApp().sidebar().GetEventHandler()->ProcessEvent(e);
        PresetBundle& preset_bundle = *wxGetApp().preset_bundle;
        bool is_cx_vendor = preset_bundle.is_cx_vendor();
        p->connection_btn->Show(!is_cx_vendor);

        p->hsizer_printer->Layout();
        
        e.Skip();
    });

    p->edit_btn      = new ScalableButton(this, wxID_ANY, "edit");
    p->edit_btn->SetToolTip(_L("Click to edit preset1"));
    p->edit_btn->Bind(wxEVT_BUTTON, [&](wxCommandEvent) {
        p->plater->sidebar().set_edit_filament(-1);
        if (p->combo_printer->switch_to_tab()) {
            p->plater->sidebar().set_edit_filament(0);
        }
    });

    p->connection_btn = new ScalableButton(this, wxID_ANY, "monitor_signal_strong");
    p->connection_btn->SetBackgroundColour(wxColour(255, 255, 255));
    p->connection_btn->SetToolTip(_L("Connection"));
    p->connection_btn->Bind(wxEVT_BUTTON, [&](wxCommandEvent) {
        PhysicalPrinterDialog dlg(this->GetParent());
        dlg.ShowModal();
    });

    p->hsizer_printer = new wxBoxSizer(wxHORIZONTAL);

    vsizer_printer->AddSpacer(FromDIP(5));
    p->hsizer_printer->Add(p->combo_printer, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(3));
    p->hsizer_printer->Add(p->edit_btn, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(3));
    p->hsizer_printer->Add(FromDIP(8), 0, 0, 0, 0);
    p->hsizer_printer->Add(p->connection_btn, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(3));
    p->hsizer_printer->Add(FromDIP(8), 0, 0, 0, 0);
    vsizer_printer->Add(p->hsizer_printer, 0, wxEXPAND, 0);

    // Bed type selection
    wxBoxSizer*   bed_type_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* bed_type_title = new wxStaticText(this, wxID_ANY, _L("Bed type"));

    bed_type_title->Wrap(-1);
    bed_type_title->SetFont(Label::Body_14);

    p->m_bed_type_list = new ComboBox(this, wxID_ANY, wxString(""), wxDefaultPosition, {-1, FromDIP(30)}, 0, nullptr, wxCB_READONLY);
    p->m_bed_type_list->Bind(wxEVT_COMBOBOX,
                           [this](wxCommandEvent& e) { Slic3r::GUI::wxGetApp().sidebar().GetEventHandler()->ProcessEvent(e); });

    update_bed_type();

    bed_type_title->Bind(wxEVT_ENTER_WINDOW, [bed_type_title, this](wxMouseEvent& e) {
        e.Skip();
        auto font = bed_type_title->GetFont();
        font.SetUnderlined(true);
        bed_type_title->SetFont(font);
        SetCursor(wxCURSOR_HAND);
    });

    bed_type_title->Bind(wxEVT_LEAVE_WINDOW, [bed_type_title, this](wxMouseEvent& e) {
        e.Skip();
        auto font = bed_type_title->GetFont();
        font.SetUnderlined(false);
        bed_type_title->SetFont(font);
        SetCursor(wxCURSOR_ARROW);
    });
    bed_type_title->Bind(wxEVT_LEFT_UP, [bed_type_title, this](wxMouseEvent& e) {
      //  wxLaunchDefaultBrowser("https://github.com/SoftFever/OrcaSlicer/wiki/bed-types");
    });

    AppConfig*  app_config     = wxGetApp().app_config;
    std::string str_bed_type   = app_config->get("curr_bed_type");
    int         bed_type_value = atoi(str_bed_type.c_str());
    // hotfix: btDefault is added as the first one in BedType, and app_config should not be btDefault
    if (bed_type_value == 0) {
        app_config->set("curr_bed_type", "1");
        bed_type_value = 1;
    }

    int bed_type_idx = bed_type_value - 1;
    if(bed_type_idx >= p->m_bed_type_list->GetCount())
    {
        bed_type_idx = p->m_bed_type_list->GetCount();
        app_config->set("curr_bed_type", std::to_string(bed_type_idx));
    }
    
    p->m_bed_type_list->Select(bed_type_idx);
    bed_type_sizer->Add(bed_type_title, 0, wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, FromDIP(10));
    bed_type_sizer->Add(p->m_bed_type_list, 1, wxLEFT | wxRIGHT | wxEXPAND, FromDIP(10));
    vsizer_printer->Add(bed_type_sizer, 0, wxEXPAND | wxTOP, FromDIP(5));
    vsizer_printer->AddSpacer(FromDIP(16));

    BedType bed_type = (BedType) bed_type_value;
    auto& project_config = wxGetApp().preset_bundle->project_config;    
    project_config.set_key_value("curr_bed_type", new ConfigOptionEnum<BedType>(bed_type));

    this->SetSizer(vsizer_printer);
    this->Layout();
}

void SidebarPrinter::update()
{
    //update printer
    p->combo_printer->update();

    //update bed type
    PresetBundle& preset_bundle = *wxGetApp().preset_bundle;
    bool is_bbl_vendor = preset_bundle.is_bbl_vendor();
    bool is_creality_vendor = preset_bundle.is_cx_vendor();
    auto cfg = preset_bundle.printers.get_edited_preset().config;

    auto get_default_bed_type = [&preset_bundle, &is_creality_vendor](Plater* plater, ComboBox* bed_type_list) {
        BedType bed_type = preset_bundle.printers.get_edited_preset().get_default_bed_type(&preset_bundle);
        if (is_creality_vendor) {
            bed_type = (BedType) plater->get_bed_type_list_second(bed_type);
        }
        bed_type_list->SelectAndNotify((int) bed_type - 1);
    };

    update_bed_type();

    if (/*is_creality_vendor || */is_bbl_vendor || cfg.opt_bool("support_multi_bed_types")) {
        p->m_bed_type_list->Enable();
        auto str_bed_type = wxGetApp().app_config->get_printer_setting(wxGetApp().preset_bundle->printers.get_selected_preset_name(),
                                                                       "curr_bed_type");
        if (!str_bed_type.empty()) {
            int bed_type_value = atoi(str_bed_type.c_str());

            if (is_creality_vendor) {
                bed_type_value = p->plater->get_bed_type_list_second(bed_type_value);
            }
                
            if (bed_type_value == 0 )
                bed_type_value = 1;

            if (bed_type_value - 1 >= p->m_bed_type_list->GetCount())
            {
                get_default_bed_type(p->plater, p->m_bed_type_list);
            }

            if(bed_type_value - 1 < p->m_bed_type_list->GetCount())
                p->m_bed_type_list->SelectAndNotify(bed_type_value - 1);
        } else {
            get_default_bed_type(p->plater, p->m_bed_type_list);
        }
    } else {
            if (is_creality_vendor) {
                p->m_bed_type_list->SelectAndNotify(p->plater->get_bed_type_list_second((int)btPEI) - 1);
            }
            else{
                p->m_bed_type_list->SelectAndNotify(btPEI - 1);
            }
            p->m_bed_type_list->Disable();
    }

}
BedType SidebarPrinter::get_selected_bed_type()
{
    const ConfigOptionDef* bed_type_def       = print_config_def.get("curr_bed_type");
    int bed_type_idx = p->m_bed_type_list->GetSelection();
    wxString t = p->m_bed_type_list->GetString(bed_type_idx);
    for (int i =0; i< bed_type_def->enum_labels.size(); i++) {
        if (t.ToStdString() == _L(bed_type_def->enum_labels[i])) {
            std::string key = bed_type_def->enum_values[i];
            return (BedType) bed_type_def->enum_keys_map->at(key);
        }
    }
    return (BedType) 0;
}
void SidebarPrinter::on_bed_type_change(BedType bed_type)
{
    // btDefault option is not included in global bed type setting
     const ConfigOptionDef* bed_type_def       = print_config_def.get("curr_bed_type");
     std::string bed_type_str = "";
     for (const auto& key_val : *bed_type_def->enum_keys_map) {
        if(key_val.second == (int)bed_type)
        {
            bed_type_str = key_val.first;
            break;
        }
        
    }
     for (int i =0; i< bed_type_def->enum_values.size(); i++) {
        if (bed_type_str == bed_type_def->enum_values[i]) {
            unsigned int count = p->m_bed_type_list->GetCount();
            for (size_t j = 0; j < count; j++) 
            {
                wxString t = p->m_bed_type_list->GetString(j);
                if(t.ToStdString() == _L(bed_type_def->enum_labels[i]))
                {
                    p->m_bed_type_list->SetSelection(j);
                    return;
                }
            }
            //p->m_bed_type_list->SetSelection(i);
            //break;
        }
     }
     p->m_bed_type_list->SetSelection(0);
    //int sel_idx = (int) bed_type - 1;
    //PresetBundle& preset_bundle      = *wxGetApp().preset_bundle;
    //bool          is_creality_vendor = preset_bundle.is_cx_vendor();
    //if (is_creality_vendor) {
    //    p->plater->get_bed_type_list(sel_idx);
    //}

    //if (p->m_bed_type_list != nullptr)
    //    p->m_bed_type_list->SetSelection(sel_idx);
}

void SidebarPrinter::update_bed_type() 
{
    PresetBundle& preset_bundle      = *wxGetApp().preset_bundle;
    bool is_creality_vendor = preset_bundle.is_cx_vendor();
    const ConfigOptionDef* bed_type_def       = print_config_def.get("curr_bed_type");

   if (is_creality_vendor) {
        p->m_bed_type_list->Clear();
        if (bed_type_def && bed_type_def->enum_keys_map) {
            int i = 1;
            int count = 0;
            std::map<int, int> _bed_type_list;
            for (auto item : bed_type_def->enum_labels) {
                count++;
                if ("Cool Plate" == item || "Engineering Plate" == item)
                    continue;
                _bed_type_list.insert(std::pair<int, int>(i, count));
                i++;    
                p->m_bed_type_list->AppendString(_L(item));
            }
            p->plater->set_bed_type_list(_bed_type_list);
        }
    } 
   else {
        p->m_bed_type_list->Clear();
        if (bed_type_def && bed_type_def->enum_keys_map) {
            for (auto item : bed_type_def->enum_labels) {
                if ("Epoxy Resin Plate" == item)
                    continue;
                p->m_bed_type_list->AppendString(_L(item));
            }
        }
    }
}

void SidebarPrinter::rescale()
{
    SetMinSize(wxSize(42 * wxGetApp().em_unit(), -1));
    if(nullptr != p->m_panel_printer_title)
    {
        p->m_panel_printer_title->GetSizer()->SetMinSize(-1, 3 * wxGetApp().em_unit());
    }
    if(p->m_printer_icon) 
        p->m_printer_icon->msw_rescale();
    if(p->m_printer_setting)
        p->m_printer_setting->msw_rescale();
    if(p->m_bed_type_list)
    {
        p->m_bed_type_list->Rescale();
        p->m_bed_type_list->SetMinSize({-1, 3 * wxGetApp().em_unit()});
    }

}

void SidebarPrinter::sys_color_changed() { 
    p->combo_printer->sys_color_changed(); 
}

void SidebarPrinter::edit_filament()
{
    p->plater->sidebar().set_edit_filament(-1);
    if (p->combo_printer->switch_to_tab()) {
        p->plater->sidebar().set_edit_filament(0);
    }
}

int SidebarPrinter::get_selection_combo_printer()
{ 
    return p->combo_printer->GetSelection();
}

std::vector<std::string> SidebarPrinter::texts_of_combo_printer()
{ 
    std::vector<string> result;
    ComboBox*    combo = p->combo_printer;
    unsigned int count = combo->GetCount();

    for (size_t i = 0; i < count; i++) 
    {
        wxString t = combo->GetString(i);
        result.emplace_back(t.ToUTF8());
    }
    
    return result;
}

void SidebarPrinter::select_printer_preset(const wxString& preset, int idx)
{
    if (p->combo_printer == nullptr || idx < 0 || idx >= int(p->combo_printer->GetCount())) {
        return;
    }

    // A native ComboBox selection updates the control before sending wxEVT_COMBOBOX.
    // Keep the synthetic event path equivalent so downstream code never observes
    // the requested item in the event and the previous item in GetSelection().
    p->combo_printer->SetSelection(idx);

    wxCommandEvent event(wxEVT_COMBOBOX, p->combo_printer->GetId());
    event.SetInt(idx);
    event.SetString(preset);
    event.SetEventObject(p->combo_printer);
    wxPostEvent(p->combo_printer, event);
}

static std::string printer_selector_separator(const wxString& label)
{
#ifdef __linux__
    return into_u8(wxString::FromUTF8("------- ") + label + wxString::FromUTF8(" -------"));
#else
    return into_u8(wxString::FromUTF8("------ ") + label + wxString::FromUTF8(" ------"));
#endif
}

static PrinterPresetOrigin printer_origin_for_preset(const Preset& preset)
{
    if (preset.is_project_embedded)
        return PrinterPresetOrigin::Project;
    if (preset.is_system || preset.is_default)
        return PrinterPresetOrigin::System;
    return PrinterPresetOrigin::User;
}

static PrinterMachineKey machine_key_for_preset(const Preset& preset)
{
    PrinterMachineKey key;
    key.origin = printer_origin_for_preset(preset);

    const std::string model = preset.config.opt_string("printer_model");
    if (key.origin == PrinterPresetOrigin::System && !model.empty()) {
        const std::string vendor = preset.vendor == nullptr ? std::string() : preset.vendor->id;
        key.machine_id = "model:" + vendor + ":" + model;
    } else if (key.origin == PrinterPresetOrigin::Project) {
        key.machine_id = "project:" + preset.name;
    } else {
        // User presets are independent logical printers. In particular, two
        // Save As presets must never be merged through base_id or setting_id.
        key.machine_id = "preset:" + preset.name;
    }
    return key;
}

static PrinterSelectorSection make_printer_section(PrinterPresetOrigin origin, const wxString& label,
                                                   const std::vector<PrinterMachineItem>& machines)
{
    PrinterSelectorSection section;
    section.origin       = origin;
    section.display_name = printer_selector_separator(label);
    for (const PrinterMachineItem& machine : machines) {
        if (machine.key.origin == origin)
            section.machines.emplace_back(machine);
    }
    return section;
}

static bool has_valid_nozzle_variant_metadata(const Preset &preset)
{
    const auto *ids = preset.config.option<ConfigOptionStrings>("nozzle_variant_ids");
    const auto *diameters = preset.config.option<ConfigOptionFloats>("nozzle_variant_diameters");
    const auto *extruder_ids = preset.config.option<ConfigOptionInts>("nozzle_variant_extruder_ids");
    return ids != nullptr && diameters != nullptr && extruder_ids != nullptr && !ids->empty() &&
           ids->size() == diameters->size() && ids->size() == extruder_ids->size();
}

// Resolve identity from saved presets only. An edited nozzle_diameter must
// not redefine a printer specification, including after Save As.
static std::string legacy_nozzle_specification(const PresetCollection &presets, const Preset &preset)
{
    std::set<const Preset *> visited;
    const Preset *current = &preset;
    while (current != nullptr && visited.insert(current).second) {
        const auto *variant = current->config.option<ConfigOptionString>("printer_variant");
        if (variant != nullptr && !variant->value.empty())
            return variant->value;
        const Preset *parent = presets.get_preset_parent(*current);
        if (parent == nullptr)
            break;
        // Parent lookup may return the edit copy when that parent is selected.
        const auto &saved = presets.get_presets();
        const auto it = std::find_if(saved.begin(), saved.end(), [parent](const Preset &candidate) {
            return candidate.name == parent->name;
        });
        current = it == saved.end() ? nullptr : &*it;
    }
    // Standalone legacy profiles may have no specification metadata at all.
    const auto *diameters = preset.config.option<ConfigOptionFloats>("nozzle_diameter");
    return diameters == nullptr || diameters->empty() ? std::string() : diameters->vserialize().front();
}

static void add_nozzle_preset(PrinterMachineItem &item, const std::string &specification,
                              const std::string &preset_name)
{
    const auto inserted = item.nozzle_presets.emplace(specification, preset_name);
    if (inserted.second || inserted.first->second == preset_name)
        return;

    const std::string conflict_key = item.key.machine_id + ":" +
        specification + ":" +
        inserted.first->second + ":" + preset_name;
    static std::set<std::string> logged_conflicts;
    if (logged_conflicts.insert(conflict_key).second) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": conflicting system nozzle presets for machine "
                                 << item.key.machine_id << ", specification=" << specification
                                 << ", keep=" << inserted.first->second << ", ignore=" << preset_name;
    }
}

PrinterSelectorModel SidebarPrinter::printer_selector_model() const
{
    PrinterSelectorModel            model;
    std::vector<PrinterMachineItem> machines;
    std::map<PrinterMachineKey, size_t> positions;
    std::set<PrinterMachineKey>     structured_machines;
    const PresetBundle&             bundle = *wxGetApp().preset_bundle;
    for (const Preset &preset : bundle.printers.get_presets()) {
        if (preset.is_default || !preset.is_visible || preset.printer_technology() != ptFFF)
            continue;
        const auto *diameters = dynamic_cast<const ConfigOptionVector<double> *>(preset.config.option("nozzle_diameter"));
        if (diameters == nullptr || diameters->empty())
            continue;

        const PrinterMachineKey key = machine_key_for_preset(preset);
        auto position = positions.find(key);
        if (position == positions.end()) {
            PrinterMachineItem item;
            item.key = key;
            item.display_name = preset.is_system && !preset.config.opt_string("printer_model").empty() ?
                preset.config.opt_string("printer_model") : preset.name;
            item.multi_extruder = diameters->size() > 1 && !preset.config.opt_bool("single_extruder_multi_material");
            item.extruder_count = int(diameters->size());
            item.default_preset = preset.name;
            position = positions.emplace(key, machines.size()).first;
            machines.emplace_back(std::move(item));
        }

        PrinterMachineItem &item = machines[position->second];
        const bool legacy_single_nozzle = !item.multi_extruder && !has_valid_nozzle_variant_metadata(preset);
        const std::string specification = legacy_single_nozzle ?
            legacy_nozzle_specification(bundle.printers, preset) : diameters->vserialize().front();
        const bool is_default_nozzle_set = legacy_single_nozzle ? specification == "0.4" :
            std::all_of(diameters->values.begin(), diameters->values.end(), [](double value) {
            return std::abs(value - 0.4) < EPSILON;
        });
        const bool has_structured_variants = has_valid_nozzle_variant_metadata(preset);
        if (has_structured_variants) {
            const bool first_structured_preset = structured_machines.insert(key).second;
            // A structured representative owns the machine's complete nozzle
            // table and must win over a sibling preset regardless of traversal order.
            item.nozzle_presets.clear();
            if (first_structured_preset || is_default_nozzle_set)
                item.default_preset = preset.name;
        }

        if (structured_machines.find(key) == structured_machines.end()) {
            add_nozzle_preset(item, specification, preset.name);
            if (is_default_nozzle_set)
                item.default_preset = preset.name;
        }
    }

    // A sibling nozzle preset may be hidden from the printer selector while it
    // still needs to be selectable from the nozzle diameter combo. Only add it
    // to a machine group that was created by a visible preset above.
    for (const Preset &preset : bundle.printers.get_presets()) {
        if (preset.is_default || preset.printer_technology() != ptFFF)
            continue;
        const auto position = positions.find(machine_key_for_preset(preset));
        if (position == positions.end())
            continue;
        PrinterMachineItem &item = machines[position->second];

        // Structured machines get nozzle choices from their representative preset.
        if (structured_machines.find(position->first) != structured_machines.end())
            continue;
        const auto *diameters = dynamic_cast<const ConfigOptionVector<double> *>(preset.config.option("nozzle_diameter"));
        if (diameters == nullptr || diameters->empty() || diameters->size() != size_t(item.extruder_count))
            continue;

        const bool legacy_single_nozzle = !item.multi_extruder && !has_valid_nozzle_variant_metadata(preset);
        const std::string specification = legacy_single_nozzle ?
            legacy_nozzle_specification(bundle.printers, preset) : diameters->vserialize().front();
        const bool is_default_nozzle_set = legacy_single_nozzle ? specification == "0.4" :
            std::all_of(diameters->values.begin(), diameters->values.end(), [](double value) {
            return std::abs(value - 0.4) < EPSILON;
        });
        add_nozzle_preset(item, specification, preset.name);
        if (is_default_nozzle_set)
            item.default_preset = preset.name;
    }
    if (bundle.printers.get_selected_idx() != size_t(-1)) {
        const Preset &selected_preset = bundle.printers.get_selected_preset();
        model.selected_machine = machine_key_for_preset(selected_preset);
        const auto selected_position = positions.find(model.selected_machine);
        model.has_selected_machine = selected_position != positions.end();
        // Machine labels omit the nozzle preset name, but must retain its modified marker.
        if (model.has_selected_machine && selected_preset.is_dirty)
            machines[selected_position->second].display_name.insert(0, Preset::suffix_modified());
    }

    PrinterSelectorSection project = make_printer_section(PrinterPresetOrigin::Project, _L("Project-inside presets"), machines);
    PrinterSelectorSection user    = make_printer_section(PrinterPresetOrigin::User, _L("User presets"), machines);
    PrinterSelectorSection system  = make_printer_section(PrinterPresetOrigin::System, _L("System presets"), machines);
    if (!project.machines.empty())
        model.sections.emplace_back(std::move(project));
    if (!user.machines.empty())
        model.sections.emplace_back(std::move(user));
    if (!system.machines.empty())
        model.sections.emplace_back(std::move(system));

    model.management_items.push_back(
        {PrinterManagementAction::ManagePrinters, printer_selector_separator(_L("Select/Remove printers(system presets)"))});
    if (wxGetApp().app_config->get("role_type") != "0") {
        model.management_items.push_back(
            {PrinterManagementAction::CreateNozzle, printer_selector_separator(_L("Create Nozzle"))});
        model.management_items.push_back(
            {PrinterManagementAction::CreatePrinter, printer_selector_separator(_L("Create printer"))});
    }
    return model;
}

void SidebarPrinter::execute_printer_management(PrinterManagementAction action)
{
    switch (action) {
    case PrinterManagementAction::ManagePrinters:
        wxTheApp->CallAfter([]() { wxGetApp().run_wizard(ConfigWizard::RR_USER, ConfigWizard::SP_PRINTERS); });
        break;
    case PrinterManagementAction::CreateNozzle:
        wxTheApp->CallAfter([]() {
            if (wxGetApp().plater() != nullptr)
                wxGetApp().plater()->sidebar().create_printer_preset(1);
        });
        break;
    case PrinterManagementAction::CreatePrinter:
        wxTheApp->CallAfter([]() {
            if (wxGetApp().plater() != nullptr)
                wxGetApp().plater()->sidebar().create_printer_preset(0);
        });
        break;
    }
}

static void reset_project_nozzle_variants(PresetBundle &bundle)
{
    const int extruder_count = std::max(1, bundle.get_printer_extruder_count());
    std::vector<int> indices(size_t(extruder_count), 0);
    std::vector<std::string> ids(static_cast<size_t>(extruder_count));
    for (size_t i = 0; i < indices.size(); ++i) {
        const std::vector<NozzleVariantInfo> variants = bundle.get_nozzle_variants(i);
        const auto default_it = std::find_if(variants.begin(), variants.end(), [](const NozzleVariantInfo &item) {
            return item.is_default;
        });
        if (default_it != variants.end()) {
            indices[i] = default_it->variant_index;
            ids[i] = default_it->variant_id;
        }
    }
    bundle.project_config.option<ConfigOptionInts>("variant_index", true)->values = std::move(indices);
    bundle.project_config.option<ConfigOptionStrings>("variant_id", true)->values = std::move(ids);
}

static void save_selected_nozzle_variants(const PresetBundle& bundle)
{
    AppConfig* config = wxGetApp().app_config;
    if (config == nullptr)
        return;

    const int nozzle_count = bundle.get_printer_extruder_count();
    if (nozzle_count <= 1)
        return;
    nlohmann::json ids = nlohmann::json::array();
    for (int nozzle = 0; nozzle < nozzle_count; ++nozzle)
        ids.push_back(bundle.get_selected_nozzle_variant(size_t(nozzle)).variant_id);
    config->set_printer_setting(bundle.printers.get_selected_preset_name(),
                                "selected_nozzle_variant_ids", ids.dump());
    config->save();
}

bool SidebarPrinter::select_machine(const PrinterMachineKey& key)
{
    const PrinterSelectorModel model = this->printer_selector_model();
    const PrinterMachineItem*  selected_machine = model.find_machine(key);
    if (selected_machine == nullptr)
        return false;
    if (selected_machine->default_preset.empty())
        return false;

    // Keep F039's grouped machine list, but dispatch the selection through the
    // original wx combobox event path. Selecting a preset directly from the
    // ImGui render call synchronously rebuilds all dependent preset pages.
    if (p->combo_printer != nullptr) {
        const unsigned int count = p->combo_printer->GetCount();
        for (unsigned int i = 0; i < count; ++i) {
            const size_t marker = reinterpret_cast<size_t>(p->combo_printer->GetClientData(i));
            if (marker >= PresetComboBox::LABEL_ITEM_MARKER && marker < PresetComboBox::LABEL_ITEM_MAX)
                continue;
            const std::string preset_name = Preset::remove_suffix_modified(
                p->combo_printer->GetString(i).ToUTF8().data());
            if (preset_name == selected_machine->default_preset) {
                this->select_printer_preset(p->combo_printer->GetString(i), int(i));
                return true;
            }
        }

    }

    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << ": printer combo item not found: " << selected_machine->default_preset;
    return false;
}

std::vector<NozzleVariantInfo> SidebarPrinter::nozzle_items(int physical_extruder_id) const
{
    if (physical_extruder_id < 0)
        return {};
    const PresetBundle &bundle = *wxGetApp().preset_bundle;
    if (this->is_multi_extruder() || bundle.has_structured_nozzle_variants())
        return bundle.get_nozzle_variants(size_t(physical_extruder_id));
    if (physical_extruder_id != 0)
        return {};

    const PrinterSelectorModel model = this->printer_selector_model();
    const PrinterMachineItem*  machine = model.selected_machine_item();
    if (machine == nullptr)
        return {};

    std::vector<NozzleVariantInfo> result;
    result.reserve(machine->nozzle_presets.size());
    // Keep the machine's default preset first, matching the existing 0.4-first
    // presentation, while deriving every item from a saved preset identity.
    for (int default_pass = 1; default_pass >= 0; --default_pass) {
        for (const auto& nozzle_preset : machine->nozzle_presets) {
            const bool is_default = nozzle_preset.second == machine->default_preset;
            if (is_default != (default_pass != 0))
                continue;
            // The string is the selection identity and display label. Keep a
            // numeric projection only for consumers that compare hardware diameters.
            double diameter = 0.0;
            std::istringstream stream(nozzle_preset.first);
            stream.imbue(std::locale::classic());
            if (!(stream >> diameter) || !stream.eof() || !std::isfinite(diameter) || diameter <= 0.0)
                diameter = 0.0;
            result.push_back({int(result.size()), nozzle_preset.first, diameter,
                              nvtStandard, is_default});
        }
    }
    return result;
}

int SidebarPrinter::get_selection_nozzle_variant(int physical_extruder_id) const
{
    if (physical_extruder_id < 0)
        return -1;
    const PresetBundle &bundle = *wxGetApp().preset_bundle;
    const std::vector<NozzleVariantInfo> variants = this->nozzle_items(physical_extruder_id);
    if (variants.empty())
        return -1;

    if (!this->is_multi_extruder() && !bundle.has_structured_nozzle_variants()) {
        const std::string active = legacy_nozzle_specification(bundle.printers, bundle.printers.get_selected_preset());
        const auto it = std::find_if(variants.begin(), variants.end(), [&active](const NozzleVariantInfo &item) {
            return item.variant_id == active;
        });
        return it == variants.end() ? -1 : it->variant_index;
    }
    return bundle.get_selected_nozzle_variant(size_t(physical_extruder_id)).variant_index;
}

bool apply_nozzle_variant_change(wxWindow*, int physical_extruder_id, int variant_index)
{
    if (physical_extruder_id < 0) {
        BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << ": invalid extruder " << physical_extruder_id;
        return false;
    }
    Plater* plater = wxGetApp().plater();
    if (plater == nullptr) {
        BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << ": plater is not available";
        return false;
    }
    SidebarPrinter& sidebar_printer = plater->sidebar_printer();
    PresetBundle &bundle = *wxGetApp().preset_bundle;
    const std::vector<NozzleVariantInfo> variants = sidebar_printer.nozzle_items(physical_extruder_id);
    const auto target = std::find_if(variants.begin(), variants.end(), [variant_index](const NozzleVariantInfo &item) {
        return item.variant_index == variant_index;
    });
    if (target == variants.end()) {
        BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << ": variant not found, extruder=" << physical_extruder_id + 1
                                   << ", variant_index=" << variant_index;
        return false;
    }

    if (!sidebar_printer.is_multi_extruder() && !bundle.has_structured_nozzle_variants()) {
        const PrinterSelectorModel model = sidebar_printer.printer_selector_model();
        const PrinterMachineItem*  machine = model.selected_machine_item();
        if (machine == nullptr) {
            BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << ": selected machine not found";
            return false;
        }
        const auto preset = machine->nozzle_presets.find(target->variant_id);
        if (preset == machine->nozzle_presets.end()) {
            BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << ": nozzle preset not found, machine=" << machine->key.machine_id
                                       << ", diameter=" << target->nozzle_diameter;
            return false;
        }
        Tab *printer_tab = wxGetApp().get_tab(Preset::TYPE_PRINTER);
        if (printer_tab == nullptr) {
            BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << ": printer tab is not available";
            return false;
        }
        const std::string target_name = preset->second;
        Preset* target_preset = bundle.printers.find_preset(target_name, false, true);
        if (target_preset == nullptr) {
            BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << ": nozzle preset no longer exists: " << target_name;
            return false;
        }
        // The nozzle menu includes hidden siblings of an imported printer.
        // Name-based selection requires visibility or falls back to another printer.
        const bool was_visible = target_preset->is_visible;
        target_preset->is_visible = true;
        const bool accepted = printer_tab->select_preset(target_name);
        const bool selected = accepted && bundle.printers.get_selected_preset_name() == target_name;
        if (!selected && !was_visible) {
            // A save/discard dialog can modify the collection; look up the preset again.
            if (Preset* restored = bundle.printers.find_preset(target_name, false, true))
                restored->is_visible = false;
        }
        if (selected)
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ": preset=" << preset->second
                                    << ", diameter=" << target->nozzle_diameter;
        else
            BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << ": failed to select preset=" << preset->second;
        return selected;
    }

    const int extruder_count = std::max(1, bundle.get_printer_extruder_count());
    if (physical_extruder_id >= extruder_count)
        return false;

    const int current_variant_index = bundle.get_selected_nozzle_variant(size_t(physical_extruder_id)).variant_index;
    if (current_variant_index == target->variant_index)
        return true;

    auto *indices = bundle.project_config.option<ConfigOptionInts>("variant_index", true);
    auto *ids = bundle.project_config.option<ConfigOptionStrings>("variant_id", true);
    if (indices->values.size() != size_t(extruder_count) || ids->values.size() != size_t(extruder_count))
        reset_project_nozzle_variants(bundle);
    indices = bundle.project_config.option<ConfigOptionInts>("variant_index", true);
    ids = bundle.project_config.option<ConfigOptionStrings>("variant_id", true);
    indices->values[size_t(physical_extruder_id)] = target->variant_index;
    ids->values[size_t(physical_extruder_id)] = target->variant_id;

    Tab *print_tab = wxGetApp().get_tab(Preset::TYPE_PRINT);
    if (print_tab != nullptr) {
        print_tab->update_process_extruder_switch(true);
        print_tab->update_preset_choice();
    }
    if (auto* filament_tab = wxGetApp().get_tab(Preset::TYPE_FILAMENT))
        filament_tab->update_filament_nozzle_variant_switch(true);
    if (auto* printer_tab = dynamic_cast<TabPrinter*>(wxGetApp().get_tab(Preset::TYPE_PRINTER)))
        printer_tab->refresh_nozzle_variant_ui(true);
    plater->invalid_slice_result_need_reslice();
    plater->set_plater_dirty(true);
    plater->schedule_background_process();
    save_selected_nozzle_variants(bundle);
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ": extruder=" << physical_extruder_id + 1
                            << ", variant_index=" << target->variant_index
                            << ", variant_id=" << target->variant_id
                            << ", diameter=" << target->nozzle_diameter
                            << ", nozzle_volume_type=" << int(target->nozzle_volume_type);
    return true;
}

bool apply_nozzle_variant_changes(const std::vector<int>& variant_indices)
{
    Plater* plater = wxGetApp().plater();
    if (plater == nullptr) {
        BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << ": plater is not available";
        return false;
    }

    SidebarPrinter& sidebar_printer = plater->sidebar_printer();
    PresetBundle&   bundle          = *wxGetApp().preset_bundle;
    const int       extruder_count  = std::max(1, bundle.get_printer_extruder_count());
    if (!sidebar_printer.is_multi_extruder() || variant_indices.size() != size_t(extruder_count)) {
        BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << ": invalid synchronized nozzle count="
                                   << variant_indices.size() << ", expected=" << extruder_count;
        return false;
    }

    std::vector<NozzleVariantInfo> targets;
    targets.reserve(size_t(extruder_count));
    for (int extruder_id = 0; extruder_id < extruder_count; ++extruder_id) {
        const std::vector<NozzleVariantInfo> variants = bundle.get_nozzle_variants(size_t(extruder_id));
        const int target_index = variant_indices[size_t(extruder_id)];
        const auto target = std::find_if(variants.begin(), variants.end(), [target_index](const NozzleVariantInfo& item) {
            return item.variant_index == target_index;
        });
        if (target == variants.end()) {
            BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << ": variant not found, extruder=" << extruder_id + 1
                                       << ", variant_index=" << target_index;
            return false;
        }
        targets.emplace_back(*target);
    }

    auto* indices = bundle.project_config.option<ConfigOptionInts>("variant_index", true);
    auto* ids     = bundle.project_config.option<ConfigOptionStrings>("variant_id", true);
    if (indices->values.size() != size_t(extruder_count) || ids->values.size() != size_t(extruder_count))
        reset_project_nozzle_variants(bundle);
    indices = bundle.project_config.option<ConfigOptionInts>("variant_index", true);
    ids     = bundle.project_config.option<ConfigOptionStrings>("variant_id", true);

    bool changed = false;
    for (int extruder_id = 0; extruder_id < extruder_count; ++extruder_id) {
        const NozzleVariantInfo& target = targets[size_t(extruder_id)];
        changed |= indices->values[size_t(extruder_id)] != target.variant_index ||
                   ids->values[size_t(extruder_id)] != target.variant_id;
        indices->values[size_t(extruder_id)] = target.variant_index;
        ids->values[size_t(extruder_id)]      = target.variant_id;
    }
    if (!changed)
        return true;

    Tab* print_tab = wxGetApp().get_tab(Preset::TYPE_PRINT);
    if (print_tab != nullptr) {
        print_tab->update_process_extruder_switch(true);
        print_tab->update_preset_choice();
    }
    if (auto* filament_tab = wxGetApp().get_tab(Preset::TYPE_FILAMENT))
        filament_tab->update_filament_nozzle_variant_switch(true);
    if (auto* printer_tab = dynamic_cast<TabPrinter*>(wxGetApp().get_tab(Preset::TYPE_PRINTER)))
        printer_tab->refresh_nozzle_variant_ui(true);
    plater->invalid_slice_result_need_reslice();
    plater->set_plater_dirty(true);
    plater->schedule_background_process();
    save_selected_nozzle_variants(bundle);

    for (int extruder_id = 0; extruder_id < extruder_count; ++extruder_id) {
        const NozzleVariantInfo& target = targets[size_t(extruder_id)];
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ": extruder=" << extruder_id + 1
                                << ", variant_index=" << target.variant_index
                                << ", variant_id=" << target.variant_id
                                << ", diameter=" << target.nozzle_diameter
                                << ", nozzle_volume_type=" << int(target.nozzle_volume_type);
    }
    return true;
}

bool SidebarPrinter::select_nozzle_variant(int physical_extruder_id, int variant_index)
{
    return apply_nozzle_variant_change(this, physical_extruder_id, variant_index);
}

bool SidebarPrinter::select_nozzle_variants(const std::vector<int>& variant_indices)
{
    return apply_nozzle_variant_changes(variant_indices);
}

bool SidebarPrinter::is_multi_extruder() const
{
    const Preset &printer = wxGetApp().preset_bundle->printers.get_edited_preset();
    const auto *diameters = dynamic_cast<const ConfigOptionVector<double> *>(printer.config.option("nozzle_diameter"));
    return diameters != nullptr && diameters->size() > 1 && !printer.config.opt_bool("single_extruder_multi_material");
}

bool SidebarPrinter::get_bed_type_enable_status() { return p->m_bed_type_list->IsEnabled(); }

int SidebarPrinter::get_selection_bed_type() 
{
    return p->m_bed_type_list->GetSelection();
}

std::vector<std::string> SidebarPrinter::texts_of_bed_type_list() 
{
    std::vector<string> result;
    ComboBox*           combo = p->m_bed_type_list;
    unsigned int        count = combo->GetCount();

    for (size_t i = 0; i < count; i++) {
        wxString t = combo->GetString(i);
        result.emplace_back(t.ToUTF8());
    }

    return result;
}

void SidebarPrinter::select_bed_type(int idx) 
{ 
    p->m_bed_type_list->SelectAndNotify(idx);
}


}}    // namespace Slic3r::GUI
