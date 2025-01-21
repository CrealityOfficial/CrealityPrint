#ifndef slic3r_GUI_SendToLocalNetPrinter_hpp_
#define slic3r_GUI_SendToLocalNetPrinter_hpp_

#include <wx/wx.h>
#include <wx/intl.h>
#include <wx/collpane.h>
#include <wx/dataview.h>
#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/dataview.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/hyperlink.h>
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/popupwin.h>
#include <wx/spinctrl.h>
#include <wx/artprov.h>
#include <wx/wrapsizer.h>
#include <wx/srchctrl.h>

#include "slic3r/GUI/SelectMachine.hpp"
#include "slic3r/GUI/GUI_Utils.hpp"
#include "slic3r/GUI/Widgets/StateColor.hpp"
#include "slic3r/GUI/wxExtensions.hpp"
#include "slic3r/GUI/DeviceManager.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/BBLStatusBar.hpp"
#include "slic3r/GUI/BBLStatusBarSend.hpp"
#include "slic3r/GUI/Widgets/Label.hpp"
#include "slic3r/GUI/Widgets/Button.hpp"
#include "slic3r/GUI/Widgets/CheckBox.hpp"
#include "slic3r/GUI/Widgets/ComboBox.hpp"
#include "slic3r/GUI/Widgets/ScrolledWindow.hpp"
#include "slic3r/GUI/Widgets/StaticBox.hpp"
#include <wx/simplebook.h>
#include <wx/hashmap.h>
#include "slic3r/GUI/print_manage/DeviceDB.hpp"
#include "slic3r/GUI/wxExtensions.hpp"

namespace RemotePrint {
    class MaterialMapPanel;
}

namespace Slic3r {
namespace GUI {

class SendToLocalNetPrinterDialog : public DPIDialog
{
private:
	void init_bind();
	void init_timer();

	int									m_print_plate_idx;
    int									m_current_filament_id;
    int                                 m_print_error_code = 0;
    int									timeout_count = 0;
    bool								m_is_in_sending_mode{ false };
    bool								m_is_rename_mode{ false };
    bool								enable_prepare_mode{ true };
    bool								m_need_adaptation_screen{ false };
    bool								m_export_3mf_cancel{ false };
    bool								m_is_canceled{ false };
    std::string                         m_print_error_msg;
    std::string                         m_print_error_extra;
    std::string							m_print_info;
	std::string							m_printer_last_select;
    wxString							m_current_project_name;

    TextInput*							m_rename_input{ nullptr };
    wxSimplebook*						m_rename_switch_panel{ nullptr };
	Plater*								m_plater{ nullptr };
	wxStaticBitmap*						m_staticbitmap{ nullptr };
	ThumbnailPanel*						m_thumbnailPanel{ nullptr };
    RemotePrint::MaterialMapPanel*      m_materialMapPanel { nullptr };
    wxCheckBox*                         m_checkbox_calibration { nullptr };
    wxCheckBox*                         m_checkbox_open_cfs { nullptr };
	ComboBox*							m_comboBox_printer{ nullptr };
	ComboBox*							m_comboBox_bed{ nullptr };
	Button*								m_rename_button{ nullptr };
    Button*								m_button_refresh{ nullptr };
    Button*								m_button_ensure{ nullptr };
    Button*                             m_button_send_print_cmd { nullptr };
    StaticBox*                          m_staticbox_left { nullptr };
    StaticBox*                          m_staticbox_right { nullptr };
    ScalableButton*                     m_button_left { nullptr };
    ScalableButton*                     m_button_right { nullptr };
    wxStaticText*                       m_label_error_info { nullptr };      
    wxGauge*                            m_progress_bar { nullptr };
    wxTimer*                            m_show_progress_timer { nullptr };
    wxStaticText*                       m_label_progress { nullptr };
    wxBoxSizer*                         m_sizer_progress { nullptr };
	wxPanel*							m_scrollable_region;
	wxPanel*							m_line_schedule{ nullptr };
	wxPanel*							m_panel_sending{ nullptr };
	wxPanel*							m_panel_prepare{ nullptr };
	wxPanel*							m_panel_finish{ nullptr };
    wxPanel*							m_line_top{ nullptr };
    wxPanel*							m_panel_image{ nullptr };
    wxPanel*							m_rename_normal_panel{ nullptr };
    wxPanel*							m_line_materia{ nullptr };
	wxStaticText*						m_statictext_finish{ nullptr };
    wxStaticText*						m_stext_sending{ nullptr };
    wxStaticText*						m_staticText_bed_title{ nullptr };
    wxStaticText*						m_statictext_printer_msg{ nullptr };
    wxStaticText*						m_stext_printer_title{ nullptr };
    wxStaticText*						m_rename_text{ nullptr };
    wxStaticText*						m_stext_time{ nullptr };
    wxStaticText*						m_stext_weight{ nullptr };
    Label*                              m_st_txt_error_code{ nullptr };
    Label*                              m_st_txt_error_desc{ nullptr };
    Label*                              m_st_txt_extra_info{ nullptr };
    wxHyperlinkCtrl*                    m_link_network_state{ nullptr };
	StateColor							btn_bg_enable;
    StateColor                          btn_bd_enable;
    wxBoxSizer*							rename_sizer_v{ nullptr };
    wxBoxSizer*							rename_sizer_h{ nullptr };
	wxBoxSizer*							sizer_thumbnail;
	wxBoxSizer*							m_sizer_scrollable_region;
	wxBoxSizer*							m_sizer_main;
	wxStaticText*						m_file_name;
    PrintDialogStatus					m_print_status{ PrintStatusInit };

    std::vector<wxString>               m_bedtype_list;
    std::map<std::string, ::CheckBox*>	m_checkbox_list;
    std::vector<MachineObject*>			m_list;
    wxColour							m_colour_def_color{ wxColour(255, 255, 255) };
    wxColour							m_colour_bold_color{ wxColour(38, 46, 48) };
    std::shared_ptr<BBLStatusBarSend>   m_status_bar;
	wxScrolledWindow*                   m_sw_print_failed_info{nullptr};
    std::shared_ptr<int>                m_token = std::make_shared<int>(0);
    bool                                m_show_status{ false };
   
public:
	SendToLocalNetPrinterDialog(Plater* plater = nullptr);
    ~SendToLocalNetPrinterDialog();

    void init_device_list_from_db();
	bool Show(bool show);
	bool is_timeout();
    void on_rename_click(wxCommandEvent& event);
    void on_rename_enter();
    void stripWhiteSpace(std::string& str);
    void reset_timeout();
    void update_show_status();
    bool is_blocking_printing(MachineObject* obj_);
    void prepare(int print_plate_idx);
    void check_focus(wxWindow* window);
    void check_fcous_state(wxWindow* window);
    void update_priner_status_msg(wxString msg, bool is_warning = false);
    void update_print_status_msg(wxString msg, bool is_warning = false, bool is_printer = true);
	void on_cancel(wxCloseEvent& event);
    void on_send_gcode(wxCommandEvent& event);
    void send_and_print(wxCommandEvent& event);
	void clear_ip_address_config(wxCommandEvent& e);
	void on_details(wxCommandEvent& event);
    void on_left(wxMouseEvent &event);
    void on_right(wxMouseEvent &event);
	void on_print_job_cancel(wxCommandEvent& evt);
	void set_plate_info(int plate_idx);
	void on_timer(wxTimerEvent& event);
	void on_selection_changed(wxCommandEvent& event);
	void Enable_Refresh_Button(bool en);
	void show_status(PrintDialogStatus status, std::vector<wxString> params = std::vector<wxString>());
	void Enable_Send_Button(bool en);
	void on_dpi_changed(const wxRect& suggested_rect) override;
    void update_user_machine_list();
    // void show_print_failed_info(bool show, int code = 0, wxString description = wxEmptyString, wxString extra = wxEmptyString);
    void update_print_error_info(int code, std::string msg, std::string extra);
    void on_change_color_mode() { wxGetApp().UpdateDlgDarkUI(this); }
    wxString format_text(wxString& m_msg);
	std::vector<std::string> sort_string(std::vector<std::string> strArray);
    void on_device_list_updated();
    bool get_show_status() { return m_show_status; }

private:
    void send_gcode(bool start_print = false);
    std::string build_print_cmd_info();
    std::string build_match_color_cmd_info();
    std::string build_switch_printer_details_page_cmd_info();
    void on_progress_update(wxThreadEvent& event);
    std::vector<std::string> get_all_filament_types();
    std::string get_selected_printer_ip();

    void handle_receive_color_match_info(const nlohmann::json& json_data);
    // void handle_receive_device_list(const nlohmann::json& json_data);

    void on_checkbox_open_cfs(wxCommandEvent& event);
    void on_timer_hide_progressbar(wxTimerEvent& event);

    bool should_disable_send_print_cmd() const;
    void update_send_print_cmd_button(const RemotePrint::DeviceDB::Data& deviceData);
    std::vector<int> get_gcode_extruders();
    double get_gcode_total_weight();

};

wxDECLARE_EVENT(EVT_CLEAR_IPADDRESS, wxCommandEvent);
}
}

#endif
