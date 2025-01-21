#ifndef slic3r_Config_Relate_Dialog_hpp_
#define slic3r_Config_Relate_Dialog_hpp_

#include "GUI.hpp"
#include "GUI_Utils.hpp"

#include <wx/window.h>
#include <wx/simplebook.h>
#include <wx/dialog.h>
#include <wx/timer.h>
#include <vector>
#include <list>
#include <map>
#include "Widgets/ComboBox.hpp"
#include "Widgets/CheckBox.hpp"
#include "Widgets/TextInput.hpp"
#include "Preferences.hpp"

namespace Slic3r { namespace GUI {

 class RelateBundle;

class ConfigRelateDialog : public DPIDialog
{
private:
    AppConfig *app_config;

protected:
    wxBoxSizer *  m_sizer_body;
    wxScrolledWindow* m_scrolledWindow;

    // bool								m_settings_layout_changed {false};
    bool m_seq_top_layer_only_changed{false};
    bool m_recreate_GUI{false};

public:
    bool seq_top_layer_only_changed() const { return m_seq_top_layer_only_changed; }
    bool recreate_GUI() const { return m_recreate_GUI; }
    void on_dpi_changed(const wxRect &suggested_rect) override;

    void resetLayout();

public:
    ConfigRelateDialog(wxWindow *      parent,
                      wxWindowID      id    = wxID_ANY,
                      const wxString &title = wxT(""),
                      const wxPoint & pos   = wxDefaultPosition,
                      const wxSize &  size  = wxDefaultSize,
                       long            style = wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER);

    ~ConfigRelateDialog();

    RelateBundle* m_relateBundle;
    wxString m_backup_interval_time;

    void      create();
    wxWindow *create_tab_button(int id, wxString text);

    // debug mode
    ::CheckBox * m_developer_mode_ckeckbox   = {nullptr};
    ::CheckBox * m_internal_developer_mode_ckeckbox = {nullptr};
    ::CheckBox * m_dark_mode_ckeckbox        = {nullptr};
    ::TextInput *m_backup_interval_textinput = {nullptr};

    wxString m_developer_mode_def;
    wxString m_internal_developer_mode_def;
    wxString m_backup_interval_def;
    wxString m_iot_environment_def;

    SelectorHash      m_hash_selector;
    RadioSelectorList m_radio_group;
    // ComboBoxSelectorList    m_comxbo_group;
    void set_dark_mode();

    void Split(const std::string &src, const std::string &separator, std::vector<wxString> &dest);
    int m_current_language_selected = {0};

protected:
    //void OnSelectTabel(wxCommandEvent &event);
    //void OnSelectRadio(wxMouseEvent &event);
    void notify_preferences_changed();
};

wxDECLARE_EVENT(EVT_PREFERENCES_SELECT_TAB, wxCommandEvent);

}} // namespace Slic3r::GUI

#endif /* slic3r_Config_Relate_Dialog_hpp_ */
