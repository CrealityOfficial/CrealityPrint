#ifndef siderBar_hpp_
#define siderBar_hpp_

#include <wx/panel.h>
#include "GUI_App.hpp"
#include <map>
#include <string>
#include <vector>

class wxButton;
class ScalableButton;
class wxScrolledWindow;
class wxString;
class ComboBox;
class Button;

namespace Slic3r {


namespace GUI {

enum class PrinterPresetOrigin
{
    Project,
    User,
    System
};

struct PrinterMachineKey
{
    PrinterPresetOrigin origin = PrinterPresetOrigin::User;
    std::string         machine_id;

    bool operator==(const PrinterMachineKey& rhs) const
    {
        return origin == rhs.origin && machine_id == rhs.machine_id;
    }

    bool operator<(const PrinterMachineKey& rhs) const
    {
        return origin < rhs.origin || (origin == rhs.origin && machine_id < rhs.machine_id);
    }
};

struct PrinterMachineItem
{
    PrinterMachineKey key;
    std::string display_name;
    std::string default_preset;
    bool        multi_extruder = false;
    int         extruder_count = 1;
    // Legacy nozzle specification -> saved printer preset name.
    std::map<std::string, std::string> nozzle_presets;
};

struct PrinterSelectorSection
{
    PrinterPresetOrigin             origin = PrinterPresetOrigin::User;
    std::string                     display_name;
    std::vector<PrinterMachineItem> machines;
};

enum class PrinterManagementAction
{
    ManagePrinters,
    CreateNozzle,
    CreatePrinter
};

struct PrinterManagementItem
{
    PrinterManagementAction action = PrinterManagementAction::ManagePrinters;
    std::string             display_name;
};

struct PrinterSelectorModel
{
    std::vector<PrinterSelectorSection> sections;
    std::vector<PrinterManagementItem>  management_items;
    bool                                has_selected_machine = false;
    PrinterMachineKey                   selected_machine;

    const PrinterMachineItem* find_machine(const PrinterMachineKey& key) const
    {
        for (const PrinterSelectorSection& section : sections) {
            for (const PrinterMachineItem& machine : section.machines) {
                if (machine.key == key)
                    return &machine;
            }
        }
        return nullptr;
    }

    const PrinterMachineItem* selected_machine_item() const
    {
        return has_selected_machine ? find_machine(selected_machine) : nullptr;
    }
};

// Shared project-level nozzle variant transaction used by both the sidebar
// and the printer settings tab.
bool apply_nozzle_variant_change(wxWindow* parent, int physical_extruder_id, int variant_index);
bool apply_nozzle_variant_changes(const std::vector<int>& variant_indices);

class SidebarPrinter : public wxPanel
{
public:
    SidebarPrinter(Plater* parent);
    void update();
    void on_bed_type_change(BedType bed_type);
    void rescale();
    void sys_color_changed();
    void edit_filament();

    int get_selection_combo_printer();
    std::vector<std::string> texts_of_combo_printer();
    void select_printer_preset(const wxString& preset, int idx);

    PrinterSelectorModel printer_selector_model() const;
    bool                 select_machine(const PrinterMachineKey& key);
    void                 execute_printer_management(PrinterManagementAction action);

    std::vector<NozzleVariantInfo> nozzle_items(int physical_extruder_id) const;
    int get_selection_nozzle_variant(int physical_extruder_id) const;
    bool select_nozzle_variant(int physical_extruder_id, int variant_index);
    bool select_nozzle_variants(const std::vector<int>& variant_indices);
    bool is_multi_extruder() const;

    bool get_bed_type_enable_status();
    int get_selection_bed_type();
    std::vector<std::string> texts_of_bed_type_list();
    void select_bed_type(int idx);
    BedType get_selected_bed_type(); 

protected:
    void update_bed_type();

public:
    static bool bShow;

private:
    struct priv;
    std::unique_ptr<priv> p;
};

} // namespace GUI
} // namespace Slic3r

#endif
