#ifndef _TRANSMITTANCE_DIALOG_H_
#define _TRANSMITTANCE_DIALOG_H_

#include "GUI_Utils.hpp"

#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/msgdlg.h>

#include "Plater.hpp"
#include "RammingChart.hpp"
class Button;
class CheckBox;
class Label;

class TransmittancePanel : public wxPanel
{
public:
    // BBS
    TransmittancePanel(wxWindow*                       parent,
                const std::vector<float>&       matrix,
                const std::vector<float>&       extruders,
                const std::vector<std::string>& extruder_colours,
                Button*                         calc_button,
                const std::vector<int>&         extra_flush_volume,
                float                           nozzle_diameter = 0.8f);

    std::vector<float> read_matrix_values();
    std::vector<float> read_extruders_values();
    void               toggle_advanced(bool user_action = false);
    void               create_panels(wxWindow* parent, const int num);
    void               msw_rescale();

private:
    void                                  fill_in_matrix();
    bool                                  advanced_matches_simple();
    void                                  update_warning_texts();

    std::vector<wxSpinCtrl*>              m_old;
    std::vector<wxSpinCtrl*>              m_new;
    std::vector<std::vector<wxTextCtrl*>> edit_boxes;
    std::vector<wxColour>                 m_colours;
    unsigned int                          m_number_of_extruders = 0;
    bool                                  m_advanced            = false;
    wxPanel*                              m_page_simple         = nullptr;
    wxPanel*                              m_page_advanced       = nullptr;
    wxPanel*                              header_line_panel     = nullptr;
    wxBoxSizer*                           m_sizer               = nullptr;
    wxBoxSizer*                           m_sizer_simple        = nullptr;
    wxBoxSizer*                           m_sizer_advanced      = nullptr;
    wxGridSizer*                          m_gridsizer_advanced  = nullptr;
    wxButton*                             m_widget_button       = nullptr;
    Label*                                m_tip_message_label   = nullptr;
    ScalableButton*                       m_btn_reset           = nullptr;
    std::vector<wxButton*>                icon_list1;
    std::vector<wxButton*>                icon_list2;

    float                  m_min_transmittance_skin_depth = 0.8f; // minimum allowed non-zero skeleton flush skin depth
    const std::vector<int> m_min_skin_depth;
    const int              m_max_skin_depth;
    int                    m_type_skin_depth;

    wxStaticText* m_min_skin_depth_label  = nullptr;
    wxStaticText* m_type_skin_depth_label = nullptr;

    std::vector<float> m_matrix;
};

class TransmittanceDialog : public Slic3r::GUI::DPIDialog
{
public:
    TransmittanceDialog(wxWindow*                       parent,
                 const std::vector<float>&       matrix,
                 const std::vector<float>&       extruders,
                 const std::vector<std::string>& extruder_colours,
                 const std::vector<int>&         extra_skin_depth,
                 float                           nozzle_diameter = 0.8f,
                 bool                            flush_into_skeleton = false,
                 bool                            show_flush_into_skeleton = false);
    std::vector<float> get_matrix() const { return m_output_matrix; }
    std::vector<float> get_extruders() const { return m_output_extruders; }
    bool               get_flush_into_skeleton() const { return m_flush_into_skeleton; }
    wxBoxSizer*        create_btn_sizer(long flags);

    void on_dpi_changed(const wxRect& suggested_rect) override;

private:
    TransmittancePanel*              m_panel_wiping = nullptr;
    ::CheckBox*                      m_flush_into_skeleton_checkbox = nullptr;
    std::vector<float>               m_output_matrix;
    std::vector<float>               m_output_extruders;
    bool                             m_flush_into_skeleton = false;
    std::unordered_map<int, Button*> m_button_list;
};

#endif // _TRANSMITTANCE_DIALOG_H_
