#ifndef siderBar_hpp_
#define siderBar_hpp_

#include <wx/panel.h>
#include "GUI_App.hpp"

class wxButton;
class ScalableButton;
class wxScrolledWindow;
class wxString;
class ComboBox;
class Button;

namespace Slic3r {


namespace GUI {

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
