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

#include "GUI.hpp"
#include "PresetComboBoxes.hpp"
#include "PhysicalPrinterDialog.hpp"

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
    wxCommandEvent event(wxEVT_COMBOBOX);
    event.SetInt(idx);
    event.SetString(preset);
    event.SetEventObject(p->combo_printer);
    wxPostEvent(p->combo_printer, event);
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
