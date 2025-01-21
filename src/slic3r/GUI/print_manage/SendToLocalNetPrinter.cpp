#include "SendToLocalNetPrinter.hpp"
#include "slic3r/GUI/I18N.hpp"

#include "libslic3r/Utils.hpp"
#include "libslic3r/Thread.hpp"
#include "slic3r/GUI/GUI.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/GUI_Preview.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "slic3r/GUI/PartPlate.hpp"
#include "slic3r/GUI/Widgets/Button.hpp"
#include "slic3r/GUI/Widgets/Label.hpp"
#include "slic3r/GUI/Widgets/StateColor.hpp"
#include "slic3r/GUI/format.hpp"
#include "slic3r/GUI/Widgets/ProgressDialog.hpp"
#include "slic3r/GUI/Widgets/RoundedRectangle.hpp"
#include "slic3r/GUI/Widgets/StaticBox.hpp"
#include "slic3r/GUI/ConnectPrinter.hpp"
#include "slic3r/GUI/Widgets/HoverBorderIcon.hpp"
#include "slic3r/GUI/wxExtensions.hpp"
#include "slic3r/GUI/Jobs/BoostThreadWorker.hpp"
#include "slic3r/GUI/Jobs/PlaterWorker.hpp"

#include <string>
#include <vector>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/progdlg.h>
#include <wx/clipbrd.h>
#include <wx/dcgraph.h>
#include <miniz.h>
#include <algorithm>
#include <wx/string.h>
#include "slic3r/GUI/BitmapCache.hpp"
#include "slic3r/GUI/print_manage/Utils.hpp"
#include "slic3r/GUI/print_manage/RemotePrinterManager.hpp"
#include "slic3r/GUI/print_manage/MaterialMapPanel.hpp"
#include "slic3r/GUI/Notebook.hpp"

namespace Slic3r {
namespace GUI {

#define INITIAL_NUMBER_OF_MACHINES 0
#define LIST_REFRESH_INTERVAL 200
#define MACHINE_LIST_REFRESH_INTERVAL 2000

wxDEFINE_EVENT(EVT_UPDATE_USER_MACHINE_LIST, wxCommandEvent);
wxDEFINE_EVENT(EVT_PRINT_JOB_CANCEL, wxCommandEvent);
wxDEFINE_EVENT(EVT_SEND_JOB_SUCCESS, wxCommandEvent);
// wxDEFINE_EVENT(EVT_CLEAR_IPADDRESS, wxCommandEvent);


static wxString extract_ip_address(const wxString& combo_selection)
{
    std::regex ip_regex(R"(\(([^)]+)\))");
    std::smatch match;
    std::string combo_str = combo_selection.ToStdString();
    if (std::regex_search(combo_str, match, ip_regex)) {
        return wxString(match[1].str());
    }
    return "";
}


void SendToLocalNetPrinterDialog::init_device_list_from_db()
{

    m_comboBox_printer->Clear();
    int defaultSelectionIndex = wxNOT_FOUND;
    int firstOnlineIndex      = wxNOT_FOUND;

    for (size_t i = 0; i < RemotePrint::DeviceDB::getInstance().devices.size(); ++i) {
        const auto& device              = RemotePrint::DeviceDB::getInstance().devices[i];
        wxString    online_status       = device.online ? _L("Online") : _L("Offline");
        wxString    printer_busy_status = device.deviceState ? _L("Busy") : _L("Ready");
        m_comboBox_printer->Append(device.name + " (" + device.address + ")" + "                    " + online_status);

        if (device.online) {
            if (device.model == "F008" && defaultSelectionIndex == wxNOT_FOUND) {
                defaultSelectionIndex = i;
            }
            if (firstOnlineIndex == wxNOT_FOUND) {
                firstOnlineIndex = i;
            }
        }
    }

    if (defaultSelectionIndex != wxNOT_FOUND) {
        m_comboBox_printer->SetSelection(defaultSelectionIndex);
    } else if (firstOnlineIndex != wxNOT_FOUND) {
        m_comboBox_printer->SetSelection(firstOnlineIndex);
    }

    // Manually trigger the selection changed event
    wxCommandEvent event(wxEVT_COMBOBOX, m_comboBox_printer->GetId());
    event.SetEventObject(m_comboBox_printer);
    on_selection_changed(event);
}

void SendToLocalNetPrinterDialog::on_device_list_updated()
{
    set_plate_info(m_print_plate_idx);
}

void SendToLocalNetPrinterDialog::stripWhiteSpace(std::string& str)
{
    if (str == "") { return; }

    string::iterator cur_it;
    cur_it = str.begin();

    while (cur_it != str.end()) {
        if ((*cur_it) == '\n' || (*cur_it) == ' ') {
            cur_it = str.erase(cur_it);
        }
        else {
            cur_it++;
        }
    }
}

wxString SendToLocalNetPrinterDialog::format_text(wxString &m_msg)
{

	if (wxGetApp().app_config->get("language") != "zh_CN") { return m_msg; }

	wxString out_txt = m_msg;
	wxString count_txt = "";
	int      new_line_pos = 0;

	for (int i = 0; i < m_msg.length(); i++) {
		auto text_size = m_statictext_printer_msg->GetTextExtent(count_txt);
		if (text_size.x < (FromDIP(400))) {
			count_txt += m_msg[i];
		}
		else {
			out_txt.insert(i - 1, '\n');
			count_txt = "";
		}
	}
	return out_txt;
}

void SendToLocalNetPrinterDialog::check_focus(wxWindow* window)
{
    if (window == m_rename_input || window == m_rename_input->GetTextCtrl()) {
        on_rename_enter();
    }
}

void SendToLocalNetPrinterDialog::check_fcous_state(wxWindow* window)
{
    check_focus(window);
    auto children = window->GetChildren();
    for (auto child : children) {
        check_fcous_state(child);
    }
}

void SendToLocalNetPrinterDialog::on_rename_click(wxCommandEvent& event)
{
    m_is_rename_mode = true;
    m_rename_input->GetTextCtrl()->SetValue(m_current_project_name);
    m_rename_switch_panel->SetSelection(1);
    m_rename_input->GetTextCtrl()->SetFocus();
    m_rename_input->GetTextCtrl()->SetInsertionPointEnd();
}

void SendToLocalNetPrinterDialog::on_rename_enter()
{
    if (m_is_rename_mode == false) {
        return;
    }
    else {
        m_is_rename_mode = false;
    }

    auto     new_file_name = m_rename_input->GetTextCtrl()->GetValue();

    wxString temp;
    int      num = 0;
    for (auto t : new_file_name) {
        if (t == wxString::FromUTF8("\x20")) {
            num++;
            if (num == 1) temp += t;
        } else {
            num = 0;
            temp += t;
        }
    }
    new_file_name = temp;

    auto     m_valid_type = Valid;
    wxString info_line;

    const char* unusable_symbols = "<>[]:/\\|?*\"";

    const std::string unusable_suffix = PresetCollection::get_suffix_modified(); //"(modified)";
    for (size_t i = 0; i < std::strlen(unusable_symbols); i++) {
        if (new_file_name.find_first_of(unusable_symbols[i]) != std::string::npos) {
            info_line = _L("Name is invalid;") + "\n" + _L("illegal characters:") + " " + unusable_symbols;
            m_valid_type = NoValid;
            break;
        }
    }

    if (m_valid_type == Valid && new_file_name.find(unusable_suffix) != std::string::npos) {
        info_line = _L("Name is invalid;") + "\n" + _L("illegal suffix:") + "\n\t" + from_u8(PresetCollection::get_suffix_modified());
        m_valid_type = NoValid;
    }

    if (m_valid_type == Valid && new_file_name.empty()) {
        info_line = _L("The name is not allowed to be empty.");
        m_valid_type = NoValid;
    }

    if (m_valid_type == Valid && new_file_name.find_first_of(' ') == 0) {
        info_line = _L("The name is not allowed to start with space character.");
        m_valid_type = NoValid;
    }

    if (m_valid_type == Valid && new_file_name.find_last_of(' ') == new_file_name.length() - 1) {
        info_line = _L("The name is not allowed to end with space character.");
        m_valid_type = NoValid;
    }

    if (m_valid_type != Valid) {
        MessageDialog msg_wingow(nullptr, info_line, "", wxICON_WARNING | wxOK);
        if (msg_wingow.ShowModal() == wxID_OK) {
            m_rename_switch_panel->SetSelection(0);
            m_rename_text->SetLabel(m_current_project_name);
            m_rename_normal_panel->Layout();
            return;
        }
    }

    m_current_project_name = new_file_name;
    m_rename_switch_panel->SetSelection(0);
    m_rename_text->SetLabel(m_current_project_name);
    m_rename_normal_panel->Layout();
}

SendToLocalNetPrinterDialog::SendToLocalNetPrinterDialog(Plater *plater)
    : DPIDialog(static_cast<wxWindow *>(wxGetApp().mainframe), wxID_ANY, _L("Send to LocalNet Printer"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX)
    , m_plater(plater), m_export_3mf_cancel(false)
{
#ifdef __WINDOWS__
    SetDoubleBuffered(true);
#endif //__WINDOWS__

    wxSize minSize = wxSize(600, 550); // 设置最小尺寸
    // wxSize initialSize = wxSize(1024, 768); // 设置初始尺寸
    wxSize initialSize = wxSize(600, 550);
    SetMinSize(minSize);
    SetSize(initialSize);

    // bind
    Bind(wxEVT_CLOSE_WINDOW, &SendToLocalNetPrinterDialog::on_cancel, this);
    Bind(wxEVT_THREAD, &SendToLocalNetPrinterDialog::on_progress_update, this);

    // font
    SetFont(wxGetApp().normal_font());

    // icon
    std::string icon_path = (boost::format("%1%/images/Creative3DTitle.ico") % resources_dir()).str();
    SetIcon(wxIcon(encode_path(icon_path.c_str()), wxBITMAP_TYPE_ICO));

    Freeze();
    SetBackgroundColour(m_colour_def_color);

    m_sizer_main = new wxBoxSizer(wxVERTICAL);

    m_sizer_main->SetMinSize(wxSize(0, -1));

    m_line_top = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 1), wxTAB_TRAVERSAL);
    m_line_top->SetBackgroundColour(wxColour(166, 169, 170));

    m_scrollable_region       = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    m_sizer_scrollable_region = new wxBoxSizer(wxVERTICAL); 

    m_panel_image = new wxPanel(m_scrollable_region, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    m_panel_image->SetBackgroundColour(m_colour_def_color);

    m_button_left = new ScalableButton(m_panel_image, wxID_ANY, "left_arrow_default");
    m_button_left->Bind(wxEVT_LEFT_DOWN, &SendToLocalNetPrinterDialog::on_left, this);

    m_button_right = new ScalableButton(m_panel_image, wxID_ANY, "right_arrow_default");
    m_button_right->Bind(wxEVT_LEFT_DOWN, &SendToLocalNetPrinterDialog::on_right, this);

    sizer_thumbnail = new wxBoxSizer(wxHORIZONTAL);
    m_thumbnailPanel = new ThumbnailPanel(m_panel_image);
    m_thumbnailPanel->set_force_draw_background(true);
    m_thumbnailPanel->set_background_size(wxSize(FromDIP(550), FromDIP(300)));
    m_thumbnailPanel->SetSize(wxSize(FromDIP(550), FromDIP(300)));
    m_thumbnailPanel->SetMinSize(wxSize(FromDIP(550), FromDIP(300)));
    m_thumbnailPanel->SetMaxSize(wxSize(FromDIP(550), FromDIP(300)));

    sizer_thumbnail->Add(m_button_left, 0, wxEXPAND, 0);
    sizer_thumbnail->AddSpacer(FromDIP(25));
    sizer_thumbnail->Add(m_thumbnailPanel, 0, wxEXPAND, 0);
    sizer_thumbnail->AddSpacer(FromDIP(25));
    sizer_thumbnail->Add(m_button_right, 0, wxEXPAND, 0);
    m_panel_image->SetSizer(sizer_thumbnail);
    m_panel_image->Layout();

    wxBoxSizer *m_sizer_basic        = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *m_sizer_basic_weight = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *m_sizer_basic_time   = new wxBoxSizer(wxHORIZONTAL);

    auto timeimg = new wxStaticBitmap(m_scrollable_region, wxID_ANY, create_scaled_bitmap("print-time", this, 18), wxDefaultPosition, wxSize(FromDIP(18), FromDIP(18)), 0);
    m_sizer_basic_weight->Add(timeimg, 1, wxEXPAND | wxALL, FromDIP(5));
    m_stext_time = new wxStaticText(m_scrollable_region, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
    m_sizer_basic_weight->Add(m_stext_time, 0, wxALL, FromDIP(5));
    m_sizer_basic->Add(m_sizer_basic_weight, 0, wxALIGN_CENTER, 0);
    m_sizer_basic->Add(0, 0, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(30));

    auto weightimg = new wxStaticBitmap(m_scrollable_region, wxID_ANY, create_scaled_bitmap("print-weight", this, 18), wxDefaultPosition, wxSize(FromDIP(18), FromDIP(18)), 0);
    m_sizer_basic_time->Add(weightimg, 1, wxEXPAND | wxALL, FromDIP(5));
    m_stext_weight = new wxStaticText(m_scrollable_region, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
    m_sizer_basic_time->Add(m_stext_weight, 0, wxALL, FromDIP(5));
    m_sizer_basic->Add(m_sizer_basic_time, 0, wxALIGN_CENTER, 0);

    m_line_materia = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 1), wxTAB_TRAVERSAL);
    m_line_materia->SetForegroundColour(wxColour(238, 238, 238));
    m_line_materia->SetBackgroundColour(wxColour(238, 238, 238));

    wxBoxSizer *m_sizer_printer = new wxBoxSizer(wxHORIZONTAL);

    m_stext_printer_title = new wxStaticText(this, wxID_ANY, _L("Printer"), wxDefaultPosition, wxSize(-1, -1), 0);
    m_stext_printer_title->SetFont(::Label::Head_14);
    m_stext_printer_title->Wrap(-1);
    m_stext_printer_title->SetForegroundColour(m_colour_bold_color);
    m_stext_printer_title->SetBackgroundColour(m_colour_def_color);

    m_sizer_printer->Add(m_stext_printer_title, 0, wxALL | wxLEFT, FromDIP(10));
    m_sizer_printer->Add(0, 0, 0, wxEXPAND | wxLEFT, FromDIP(5));

    m_materialMapPanel = new RemotePrint::MaterialMapPanel(this);

    m_comboBox_printer = new ::ComboBox(this, wxID_ANY, "", wxDefaultPosition, wxSize(FromDIP(330), 15), 0, nullptr, wxCB_READONLY);
    m_comboBox_printer->Bind(wxEVT_COMBOBOX, &SendToLocalNetPrinterDialog::on_selection_changed, this);

    m_sizer_printer->Add(m_comboBox_printer, 0, wxEXPAND | wxRIGHT, FromDIP(5));

    wxBoxSizer *sizer_error_info = new wxBoxSizer(wxHORIZONTAL);
    m_label_error_info = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    m_label_error_info->SetForegroundColour(*wxRED);
    m_label_error_info->SetFont(::Label::Body_13);
    m_label_error_info->SetMinSize(wxSize(250, -1));
    sizer_error_info->Add(m_label_error_info, 0, wxEXPAND, FromDIP(50));

    // Add checkboxes
    wxBoxSizer *m_sizer_checkboxes = new wxBoxSizer(wxHORIZONTAL);
    m_checkbox_calibration = new wxCheckBox(this, wxID_ANY, _L("Enable Calibration"));
    m_checkbox_open_cfs = new wxCheckBox(this, wxID_ANY, _L("Open CFS"));
    m_checkbox_open_cfs->Bind(wxEVT_CHECKBOX, &SendToLocalNetPrinterDialog::on_checkbox_open_cfs, this);

    m_sizer_checkboxes->AddSpacer((FromDIP(70)));
    m_sizer_checkboxes->Add(m_checkbox_calibration, 0, wxEXPAND, FromDIP(25));
    m_sizer_checkboxes->Add(m_checkbox_open_cfs, 0, wxEXPAND, FromDIP(25));

    btn_bg_enable = StateColor(        std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(150, 150, 155), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Normal),
        std::pair<wxColour, int>(wxColour(200, 200, 200), StateColor::Disabled)); 

    btn_bd_enable = StateColor(
        std::pair<wxColour, int>(wxColour(38, 46, 48), StateColor::Normal)
    );                       

    m_button_refresh = new Button(this, _L("Printer Details"));
    m_button_refresh->SetBackgroundColor(btn_bg_enable);
    m_button_refresh->SetBorderColor(btn_bd_enable);
    m_button_refresh->SetTextColor(StateColor::darkModeColorFor("#FFFFFE"));
    m_button_refresh->SetSize(SELECT_MACHINE_DIALOG_BUTTON_SIZE);
    m_button_refresh->SetMinSize(SELECT_MACHINE_DIALOG_BUTTON_SIZE);
    m_button_refresh->SetCornerRadius(FromDIP(10));
    m_button_refresh->Bind(wxEVT_BUTTON, &SendToLocalNetPrinterDialog::on_details, this);
    m_sizer_printer->Add(m_button_refresh, 0, wxALL | wxLEFT, FromDIP(5));

    m_statictext_printer_msg = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
    m_statictext_printer_msg->SetFont(::Label::Body_13);
    m_statictext_printer_msg->SetForegroundColour(*wxBLACK);
    m_statictext_printer_msg->Hide();

    // line schedule
    m_line_schedule = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    m_line_schedule->SetBackgroundColour(wxColour(238, 238, 238));

    wxBoxSizer *m_sizer_send_btns   = new wxBoxSizer(wxHORIZONTAL);

    m_sizer_send_btns->Add(0, 0, 0, wxEXPAND, FromDIP(22));
    m_button_ensure = new Button(this, _L("Send"));
    m_button_ensure->SetBackgroundColor(btn_bg_enable);
    m_button_ensure->SetBorderColor(btn_bd_enable);
    m_button_ensure->SetTextColor(StateColor::darkModeColorFor("#FFFFFE"));
    // m_button_ensure->SetSize(SELECT_MACHINE_DIALOG_BUTTON_SIZE);
    // m_button_ensure->SetMinSize(SELECT_MACHINE_DIALOG_BUTTON_SIZE);
    m_button_ensure->SetCornerRadius(FromDIP(12));
    m_button_ensure->Bind(wxEVT_BUTTON, &SendToLocalNetPrinterDialog::on_send_gcode, this);

    m_button_send_print_cmd = new Button(this, _L("Send gcode and print"));
    m_button_send_print_cmd->SetBackgroundColor(btn_bg_enable);
    m_button_send_print_cmd->SetBorderColor(btn_bd_enable);
    m_button_send_print_cmd->SetTextColor(StateColor::darkModeColorFor("#FFFFFE"));
    // m_button_send_print_cmd->SetSize(SELECT_MACHINE_DIALOG_BUTTON_SIZE);
    // m_button_send_print_cmd->SetMinSize(SELECT_MACHINE_DIALOG_BUTTON_SIZE);
    m_button_send_print_cmd->SetCornerRadius(FromDIP(12));
    m_button_send_print_cmd->Bind(wxEVT_BUTTON, &SendToLocalNetPrinterDialog::send_and_print, this);

    wxSize max_size     = m_button_ensure->GetBestSize();
    wxSize tmp_size = m_button_send_print_cmd->GetBestSize();
    if (tmp_size.GetWidth() > max_size.GetWidth()) {
        max_size.SetWidth(tmp_size.GetWidth());
    }
    if (tmp_size.GetHeight() > max_size.GetHeight()) {
        max_size.SetHeight(tmp_size.GetHeight());
    }

    m_button_ensure->SetMinSize(max_size);
    m_button_send_print_cmd->SetMinSize(max_size);

    m_progress_bar = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxSize(550, wxDefaultSize.GetHeight()), wxGA_HORIZONTAL);
    m_progress_bar->SetValue(0);
    m_progress_bar->SetBackgroundColour(wxColour(30, 144, 255));
    m_progress_bar->Hide();

    m_label_progress = new wxStaticText(this, wxID_ANY, "0%", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
    m_label_progress->SetForegroundColour(*wxGREEN);
    m_label_progress->Hide();

    wxBoxSizer* m_sizer_progress_with_text = new wxBoxSizer(wxVERTICAL);
    m_sizer_progress_with_text->Add(m_label_progress, 0, wxALIGN_CENTER, 0);
    m_sizer_progress_with_text->Add(m_progress_bar, 0, wxEXPAND, 0);

    m_sizer_send_btns->Add(m_button_ensure, 0, wxEXPAND | wxBOTTOM, FromDIP(10));
    m_sizer_send_btns->AddSpacer(FromDIP(30)); // Add some space between the buttons
    m_sizer_send_btns->Add(m_button_send_print_cmd, 0, wxEXPAND | wxBOTTOM , FromDIP(10));

    // Create a new horizontal sizer for the progress bar
    m_sizer_progress = new wxBoxSizer(wxHORIZONTAL);
    m_sizer_progress->Add(m_sizer_progress_with_text, 1, wxEXPAND | wxALL, 0);
    // m_sizer_progress->Add(0, 0, 1, wxEXPAND, 0);  // add a cancel button here

    m_sizer_scrollable_region->Add(m_panel_image, 0, wxALIGN_CENTER_HORIZONTAL, 0);
    m_sizer_scrollable_region->Add(0, 0, 0, wxTOP, FromDIP(10));
    m_sizer_scrollable_region->Add(m_sizer_basic, 0, wxALIGN_CENTER_HORIZONTAL, 0);
	m_scrollable_region->SetSizer(m_sizer_scrollable_region);
	m_scrollable_region->Layout();

    //file name
    //rename normal
    m_rename_switch_panel = new wxSimplebook(this);
    m_rename_switch_panel->SetSize(wxSize(FromDIP(420), FromDIP(25)));
    m_rename_switch_panel->SetMinSize(wxSize(FromDIP(420), FromDIP(25)));
    m_rename_switch_panel->SetMaxSize(wxSize(FromDIP(420), FromDIP(25)));

    m_rename_normal_panel = new wxPanel(m_rename_switch_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    m_rename_normal_panel->SetBackgroundColour(*wxWHITE);
    rename_sizer_v = new wxBoxSizer(wxVERTICAL);
    rename_sizer_h = new wxBoxSizer(wxHORIZONTAL);

    m_rename_text = new wxStaticText(m_rename_normal_panel, wxID_ANY, wxT("MyLabel"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
    m_rename_text->SetForegroundColour(*wxBLACK);
    m_rename_text->SetFont(::Label::Body_13);
    m_rename_text->SetMaxSize(wxSize(FromDIP(390), -1));
    m_rename_button = new Button(m_rename_normal_panel, "", "ams_editable", wxBORDER_NONE, FromDIP(10));
    m_rename_button->SetBackgroundColor(*wxWHITE);
    m_rename_button->SetBackgroundColour(*wxWHITE);

    rename_sizer_h->Add(m_rename_text, 0, wxALIGN_CENTER, 0);
    rename_sizer_h->Add(m_rename_button, 0, wxALIGN_CENTER, 0);
    rename_sizer_v->Add(rename_sizer_h, 1, wxALIGN_CENTER, 0);
    m_rename_normal_panel->SetSizer(rename_sizer_v);
    m_rename_normal_panel->Layout();
    rename_sizer_v->Fit(m_rename_normal_panel);

    //rename edit
    auto m_rename_edit_panel = new wxPanel(m_rename_switch_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    m_rename_edit_panel->SetBackgroundColour(*wxWHITE);
    auto rename_edit_sizer_v = new wxBoxSizer(wxVERTICAL);

    m_rename_input = new ::TextInput(m_rename_edit_panel, wxEmptyString, wxEmptyString, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    m_rename_input->GetTextCtrl()->SetFont(::Label::Body_13);
    m_rename_input->SetSize(wxSize(FromDIP(380), FromDIP(24)));
    m_rename_input->SetMinSize(wxSize(FromDIP(380), FromDIP(24)));
    m_rename_input->SetMaxSize(wxSize(FromDIP(380), FromDIP(24)));
    m_rename_input->Bind(wxEVT_TEXT_ENTER, [this](auto& e) {on_rename_enter();});
    m_rename_input->Bind(wxEVT_KILL_FOCUS, [this](auto& e) {
        if (!m_rename_input->HasFocus() && !m_rename_text->HasFocus())
            on_rename_enter();
        else
            e.Skip(); });
    rename_edit_sizer_v->Add(m_rename_input, 1, wxALIGN_CENTER, 0);


    m_rename_edit_panel->SetSizer(rename_edit_sizer_v);
    m_rename_edit_panel->Layout();
    rename_edit_sizer_v->Fit(m_rename_edit_panel);

    m_rename_button->Bind(wxEVT_BUTTON, &SendToLocalNetPrinterDialog::on_rename_click, this);
    m_rename_switch_panel->AddPage(m_rename_normal_panel, wxEmptyString, true);
    m_rename_switch_panel->AddPage(m_rename_edit_panel, wxEmptyString, false);

    Bind(wxEVT_CHAR_HOOK, [this](wxKeyEvent& e) {
        if (e.GetKeyCode() == WXK_ESCAPE) {
            if (m_rename_switch_panel->GetSelection() == 0) {
                e.Skip();
            }
            else {
                m_rename_switch_panel->SetSelection(0);
                m_rename_text->SetLabel(m_current_project_name);
                m_rename_normal_panel->Layout();
            }
        }
        else {
            e.Skip();
        }
        });

    m_scrollable_region->Bind(wxEVT_LEFT_DOWN, [this](auto& e) {
        check_fcous_state(this);
        e.Skip();
        });

    Bind(wxEVT_LEFT_DOWN, [this](auto& e) {
        check_fcous_state(this);
        e.Skip();
        });

    m_sizer_main->Add(m_sizer_progress, 0, wxEXPAND, 0);
    // m_sizer_main->Add(m_sizer_object_name, 0, wxEXPAND, FromDIP(10));
    m_sizer_main->Add(m_line_top, 0, wxEXPAND, 0);
    m_sizer_main->Add(0, 0, 0, wxEXPAND | wxTOP, FromDIP(6));
    m_sizer_main->Add(m_rename_switch_panel, 0, wxALIGN_CENTER_HORIZONTAL, 0);
    
    // m_sizer_main->Add(0, 0, 0, wxTOP, FromDIP(10));
    m_sizer_main->Add(m_scrollable_region, 0, wxALIGN_CENTER_HORIZONTAL, 0);
    
    // m_sizer_main->Add(m_rename_switch_panel, 0, wxALIGN_CENTER_HORIZONTAL, 0);
    // m_sizer_main->Add(0, 0, 0, wxEXPAND | wxTOP, FromDIP(6));
    m_sizer_main->Add(m_line_materia, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(30));
    m_sizer_main->Add(0, 0, 0, wxEXPAND | wxTOP, FromDIP(2));
    m_sizer_main->Add(m_materialMapPanel, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(80));
    m_sizer_main->Add(sizer_error_info, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(80));
    m_sizer_main->Add(m_sizer_printer, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(10));
    m_sizer_main->AddSpacer(FromDIP(5));
    m_sizer_main->Add(m_sizer_checkboxes, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(10));
    m_sizer_main->AddSpacer(FromDIP(5));
    m_sizer_main->Add(m_line_schedule, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(30));
    m_sizer_main->AddSpacer(FromDIP(10));
    m_sizer_main->Add(m_sizer_send_btns, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(280));

    // show_print_failed_info(false);
    SetSizer(m_sizer_main);
    Layout();
    Fit();
    Thaw();

    init_bind();
    init_timer();
    // CenterOnParent();
    Centre(wxBOTH);
    wxGetApp().UpdateDlgDarkUI(this);

    if (wxGetApp().mainframe->get_printer_mgr_view()) {
        wxGetApp().mainframe->get_printer_mgr_view()->RegisterHandler("receive_color_match_info",
            [this](const nlohmann::json& json_data) {
                this->handle_receive_color_match_info(json_data);
            });
    }
}

void SendToLocalNetPrinterDialog::update_print_error_info(int code, std::string msg, std::string extra)
{
    m_print_error_code = code;
    m_print_error_msg = msg;
    m_print_error_extra = extra;
}

void SendToLocalNetPrinterDialog::on_progress_update(wxThreadEvent& event)
{
    int progressValue = event.GetInt();

    if(!m_progress_bar->IsShown()) {
        m_progress_bar->Show(true);
        m_label_progress->Show(true);
        m_sizer_main->Layout();
    }

    m_progress_bar->SetValue(progressValue);
    wxString progressText = wxString::Format("%d%%", progressValue);
    m_label_progress->SetLabel(progressText);

    // Center the label on the progress bar
    wxSize progressBarSize = m_progress_bar->GetSize();
    wxSize labelSize = m_label_progress->GetSize();
    int labelX = (progressBarSize.GetWidth() - labelSize.GetWidth()) / 2;
    int labelY = (progressBarSize.GetHeight() - labelSize.GetHeight()) / 2;
    m_label_progress->SetPosition(wxPoint(labelX, labelY));

    if (progressValue >= 100) {
        m_show_progress_timer->StartOnce(2000); // 2 seconds
    }

    Layout();
    Fit();
    Thaw();
}

void SendToLocalNetPrinterDialog::prepare(int print_plate_idx)
{
    m_show_status = true;
    m_print_plate_idx = print_plate_idx;

    wxGetApp().mainframe->get_printer_mgr_view()->RequestDeviceListFromDB();
}

void SendToLocalNetPrinterDialog::update_priner_status_msg(wxString msg, bool is_warning) 
{
    auto colour = is_warning ? wxColour(0xFF, 0x6F, 0x00) : wxColour(0x6B, 0x6B, 0x6B);
    m_statictext_printer_msg->SetForegroundColour(colour);

    if (msg.empty()) {
        if (!m_statictext_printer_msg->GetLabel().empty()) {
            m_statictext_printer_msg->SetLabel(wxEmptyString);
            m_statictext_printer_msg->Hide();
            Layout();
            Fit();
        }
    } else {
        msg          = format_text(msg);

        auto str_new = msg.ToStdString();
        stripWhiteSpace(str_new);

        auto str_old = m_statictext_printer_msg->GetLabel().ToStdString();
        stripWhiteSpace(str_old);

        if (str_new != str_old) {
            if (m_statictext_printer_msg->GetLabel() != msg) {
                m_statictext_printer_msg->SetLabel(msg);
                m_statictext_printer_msg->SetMinSize(wxSize(FromDIP(400), -1));
                m_statictext_printer_msg->SetMaxSize(wxSize(FromDIP(400), -1));
                m_statictext_printer_msg->Wrap(FromDIP(400));
                m_statictext_printer_msg->Show();
                Layout();
                Fit();
            }
        }
    }
}

void SendToLocalNetPrinterDialog::update_print_status_msg(wxString msg, bool is_warning, bool is_printer_msg)
{
    if (is_printer_msg) {
        update_priner_status_msg(msg, is_warning);
    } else {
        update_priner_status_msg(wxEmptyString, false);
    }
}


void SendToLocalNetPrinterDialog::init_bind()
{
    Bind(wxEVT_TIMER, &SendToLocalNetPrinterDialog::on_timer_hide_progressbar, this);
}

void SendToLocalNetPrinterDialog::init_timer()
{
    m_show_progress_timer = new wxTimer();
    m_show_progress_timer->SetOwner(this);
}

void SendToLocalNetPrinterDialog::on_cancel(wxCloseEvent &event)
{
    this->EndModal(wxID_CANCEL);
}

std::string SendToLocalNetPrinterDialog::get_selected_printer_ip()
{
    wxString cur_combo_selection = m_comboBox_printer->GetValue();
    if (cur_combo_selection.IsEmpty()) {
        return "";
    }

    wxString ipAddress = extract_ip_address(cur_combo_selection);
    if (ipAddress.IsEmpty()) {
        return "";
    }

    return ipAddress.ToStdString();
}

std::string SendToLocalNetPrinterDialog::build_match_color_cmd_info()
{
    std::string ipAddress = get_selected_printer_ip();
    if (ipAddress.empty()) {
        return "";
    }

    PartPlate*     plate = wxGetApp().plater()->get_partplate_list().get_plate(m_print_plate_idx);
    if(!plate)
        return "";

    // get filament types
    std::vector<std::string> filament_presets = wxGetApp().preset_bundle->filament_presets;
    std::vector<std::string> filament_types;

    for (const auto& preset_name : filament_presets) {
        std::string     filament_type;
        Slic3r::Preset* preset = wxGetApp().preset_bundle->filaments.find_preset(preset_name);
        if (preset) {
            preset->get_filament_type(filament_type);
            filament_types.emplace_back(filament_type);
        }
    }

    std::vector<std::string> all_extruder_colors = Slic3r::GUI::wxGetApp().plater()->get_extruder_colors_from_plater_config();

    nlohmann::json           plate_extruder_colors_json     = nlohmann::json::array();
    std::vector<int> plate_extruders = plate->get_extruders(true);
    if(m_plater->only_gcode_mode()) {
        plate_extruders = get_gcode_extruders();
    }
    if (plate_extruders.size() > 0) {
        for (const auto& extruder : plate_extruders) {
            if(all_extruder_colors.size() > (extruder-1)) {
                nlohmann::json extruder_info = {
                    {"extruder_id", extruder}, 
                    {"extruder_color", all_extruder_colors[extruder - 1]},
                    {"filament_type", filament_types[extruder - 1]}
                };
                plate_extruder_colors_json.push_back(extruder_info);
            }
        }
    }

    // Create top-level JSON object
    nlohmann::json top_level_json = {
        {"printer_ip", ipAddress},
        {"plate_extruder_colors", plate_extruder_colors_json}
    };

    // Create command JSON object
    nlohmann::json commandJson = {
        {"command", "req_match_color_info"},
        {"data", top_level_json.dump()}
    };

    // Encode the command string
    return RemotePrint::Utils::url_encode(commandJson.dump());
}

std::string SendToLocalNetPrinterDialog::build_print_cmd_info()
{
    std::string ipAddress = get_selected_printer_ip();
    if (ipAddress.empty()) {
        return "";
    }

    PartPlate* plate = m_plater->get_partplate_list().get_plate(m_print_plate_idx);
    if(!plate)
        return "";

    std::vector<int> plate_extruders = plate->get_extruders(true);

    std::vector<RemotePrint::MaterialMapPanel::ColorMatchInfo> colorMatchInfo;
    if(m_materialMapPanel) {
        colorMatchInfo = m_materialMapPanel->GetColorMatchInfo();
    }


    nlohmann::json top_level_json;
    top_level_json["printer_ip"] = ipAddress;
    top_level_json["print_calibration"]  = m_checkbox_calibration->IsChecked() ? 1 : 0; // 0: not calibrated, 1: calibrated
    top_level_json["open_cfs"] = m_checkbox_open_cfs->IsChecked() ? 1 : 0; // 0: not open, 1: open

    if (m_checkbox_open_cfs->IsChecked()) {
        nlohmann::json color_match_info_json = nlohmann::json::array();
        for (const auto& info : colorMatchInfo) {
            nlohmann::json info_json;
            info_json["extruderId"]           = info.extruderId;
            info_json["extruderFilamentType"] = info.extruderFilamentType;
            info_json["matchColor"]           = info.matchColor.GetAsString(wxC2S_HTML_SYNTAX).ToStdString();
            color_match_info_json.push_back(info_json);
        }
        top_level_json["color_match_info"] = color_match_info_json;
    }
    else {
        top_level_json["color_match_info"] = "";
    }

    wxString renameText = m_rename_text->GetLabelText();
    wxString uploadName = renameText + ".gcode";
    top_level_json["upload_gcode_name"] = uploadName.ToUTF8().data();

    std::string json_str = top_level_json.dump();

    // create command to send to the webview
    nlohmann::json commandJson;
    commandJson["command"] = "send_print_cmd";
    commandJson["data"]    = top_level_json.dump();

    return RemotePrint::Utils::url_encode(commandJson.dump());

}

std::string SendToLocalNetPrinterDialog::build_switch_printer_details_page_cmd_info()
{
    std::string ipAddress = get_selected_printer_ip();
    if (ipAddress.empty()) {
        return "";
    }

    auto printerData = RemotePrint::DeviceDB::getInstance().get_printer_data(ipAddress);

    // Create top-level JSON object
    nlohmann::json top_level_json = {
        {"printer_ip", ipAddress},
        {"printer_name", printerData.name},
    };

    // Create command JSON object
    nlohmann::json commandJson = {
        {"command", "switch_printer_details_page"},
        {"data", top_level_json.dump()}
    };

    // Encode the command string
    return RemotePrint::Utils::url_encode(commandJson.dump());
}

void SendToLocalNetPrinterDialog::send_gcode(bool start_print)
{
    wxString cur_combo_selection = m_comboBox_printer->GetValue();
    if(cur_combo_selection.IsEmpty()) {
        wxMessageBox("Please select a printer.", "Error", wxOK | wxICON_ERROR);
        return;
    }

    wxString ipAddress = extract_ip_address(cur_combo_selection);
    if (ipAddress.IsEmpty()) {
        wxMessageBox("Invalid printer selection.", "Error", wxOK | wxICON_ERROR);
        return;
    }

    PartPlate* plate = wxGetApp().plater()->get_partplate_list().get_plate(m_print_plate_idx);
    if(!plate)
        return;

    std::string gcodeFilePath = plate->get_tmp_gcode_path();
    if(m_plater->only_gcode_mode())
    {
        gcodeFilePath = m_plater->get_last_loaded_gcode().ToStdString();
    }

    wxString renameText = m_rename_text->GetLabelText();
    wxString uploadName = renameText + ".gcode";

    std::string uploadNameUtf8 = uploadName.ToUTF8().data();

    RemotePrint::RemotePrinterManager::getInstance().pushUploadTasks(
        ipAddress.ToStdString(), uploadNameUtf8, gcodeFilePath,
        [this](std::string ip, float progress) {
            // Update the progress bar
            int           progressValue = static_cast<int>(progress);
            wxThreadEvent evt(wxEVT_THREAD);
            evt.SetInt(progressValue);
            wxQueueEvent(this, evt.Clone());
        },
        [this, start_print](std::string ip, int statusCode) {

            // statusCode == 0 means upload success
            if(0 == statusCode && start_print) {
                wxGetApp().mainframe->get_printer_mgr_view()->ExecuteScriptCommand(build_print_cmd_info(), true);
            }

        });
}

void SendToLocalNetPrinterDialog::on_send_gcode(wxCommandEvent& event)
{
    send_gcode(false);
}

void SendToLocalNetPrinterDialog::send_and_print(wxCommandEvent& event)
{
    send_gcode(true);
}

void SendToLocalNetPrinterDialog::clear_ip_address_config(wxCommandEvent& e)
{
    enable_prepare_mode = true;
}

void SendToLocalNetPrinterDialog::update_user_machine_list()
{
    NetworkAgent* m_agent = wxGetApp().getAgent();
    if (m_agent && m_agent->is_user_login()) {
        boost::thread get_print_info_thread = Slic3r::create_thread([this, token = std::weak_ptr<int>(m_token)] {
            NetworkAgent* agent = wxGetApp().getAgent();
            unsigned int http_code;
            std::string body;
            int result = agent->get_user_print_info(&http_code, &body);
            CallAfter([token, this, result, body] {
                if (token.expired()) {return;}
                if (result == 0) {
                    m_print_info = body;
                }
                else {
                    m_print_info = "";
                }
                wxCommandEvent event(EVT_UPDATE_USER_MACHINE_LIST);
                event.SetEventObject(this);
                wxPostEvent(this, event);
            });
        });
    } else {
        wxCommandEvent event(EVT_UPDATE_USER_MACHINE_LIST);
        event.SetEventObject(this);
        wxPostEvent(this, event);
    }
}


void SendToLocalNetPrinterDialog::on_details(wxCommandEvent &event)
{
    std::string ipAddress = get_selected_printer_ip();
    if (ipAddress.empty()) {
        return;
    }

    wxCommandEvent e = wxCommandEvent(wxCUSTOMEVT_NOTEBOOK_SEL_CHANGED);
    e.SetId(9);  // printer details page
    wxPostEvent(wxGetApp().mainframe->topbar(), e);

    wxGetApp().mainframe->get_printer_mgr_view()->ExecuteScriptCommand(build_switch_printer_details_page_cmd_info());

    this->Show(false);
}

void SendToLocalNetPrinterDialog::on_left(wxMouseEvent &event)
{
    int plate_count = m_plater->get_partplate_list().get_plate_count();
    m_print_plate_idx --;
    if (m_print_plate_idx < 0)
    {
        m_print_plate_idx = plate_count - 1;
    }

    PartPlate* selected_plate = m_plater->get_partplate_list().get_plate(m_print_plate_idx);
    if(!selected_plate)
        return;

    ThumbnailData &data   = selected_plate->thumbnail_data;
    if (!data.is_valid())
        return;

    set_plate_info(m_print_plate_idx);

}

void SendToLocalNetPrinterDialog::on_right(wxMouseEvent &event)
{
    int plate_count = m_plater->get_partplate_list().get_plate_count();
    m_print_plate_idx ++;
    if (m_print_plate_idx >= plate_count)
    {
        m_print_plate_idx = 0;
    }

    PartPlate* selected_plate = m_plater->get_partplate_list().get_plate(m_print_plate_idx);
    if(!selected_plate)
        return;

    ThumbnailData &data   = selected_plate->thumbnail_data;
    if (!data.is_valid())
        return;

    set_plate_info(m_print_plate_idx);
}

void SendToLocalNetPrinterDialog::on_print_job_cancel(wxCommandEvent &evt)
{
    BOOST_LOG_TRIVIAL(info) << "print_job: canceled";
    show_status(PrintDialogStatus::PrintStatusSendingCanceled);
}

std::vector<std::string> SendToLocalNetPrinterDialog::sort_string(std::vector<std::string> strArray)
{
    std::vector<std::string> outputArray;
    std::sort(strArray.begin(), strArray.end());
    std::vector<std::string>::iterator st;
    for (st = strArray.begin(); st != strArray.end(); st++) { outputArray.push_back(*st); }

    return outputArray;
}

bool  SendToLocalNetPrinterDialog::is_timeout()
{
    if (timeout_count > 15 * 1000 / LIST_REFRESH_INTERVAL) {
        return true;
    }
    return false;
}

void  SendToLocalNetPrinterDialog::reset_timeout()
{
    timeout_count = 0;
}

void SendToLocalNetPrinterDialog::on_timer(wxTimerEvent &event)
{
    wxGetApp().reset_to_active();
    update_show_status();
}

void SendToLocalNetPrinterDialog::on_selection_changed(wxCommandEvent &event)
{
    std::string ipAddress = get_selected_printer_ip();
    if (ipAddress.empty()) {
        return;
    }

    int plate_count = m_plater->get_partplate_list().get_plate_count();
    if(plate_count > 1) {
        m_button_left->Show();
        m_button_right->Show();
    }
    else {
        m_button_left->Hide();
        m_button_right->Hide();
    }

    if(m_progress_bar) {
        m_progress_bar->SetValue(0);
        m_progress_bar->Hide();
    }

    if(m_materialMapPanel) {
        m_materialMapPanel->related_printer_ip = ipAddress;
    }

    if(RemotePrint::DeviceDB::getInstance().DeviceHasBoxColor(ipAddress)) {
        wxGetApp().mainframe->get_printer_mgr_view()->ExecuteScriptCommand(build_match_color_cmd_info());
    }
    else {
        if(m_materialMapPanel) {
            m_materialMapPanel->clear();
        }
    }

    // Get device data based on ipAddress
    {
        auto device = RemotePrint::DeviceDB::getInstance().get_printer_data(ipAddress);
        if (device.boxColorInfos.empty()) {
            m_checkbox_open_cfs->SetValue(false);
            m_checkbox_open_cfs->Hide();
            m_materialMapPanel->Hide();
        } else {
            m_checkbox_open_cfs->SetValue(true);
            m_checkbox_open_cfs->Show();
            m_materialMapPanel->Show();
        }

        if (!device.online) {
            m_button_ensure->Disable();
            m_label_error_info->SetLabelText(_L("Printer is offline"));
        } else {
            m_button_ensure->Enable();
            update_send_print_cmd_button(device);

        }
    }

    Layout();
    Fit();

}

bool SendToLocalNetPrinterDialog::should_disable_send_print_cmd() const
{
    if (m_materialMapPanel && m_materialMapPanel->IsShown()) {
        auto colorMatchInfos = m_materialMapPanel->GetColorMatchInfo();
        for (const auto& info : colorMatchInfos) {
            if (info.matchFilamentType.empty()) {
                return true;
            }
        }
    }
    return false;
}

void SendToLocalNetPrinterDialog::update_send_print_cmd_button(const RemotePrint::DeviceDB::Data& deviceData)
{
    if (0 == deviceData.deviceState) {
        m_button_send_print_cmd->Enable();

        if (should_disable_send_print_cmd()) {
            m_button_send_print_cmd->Disable();
            m_label_error_info->SetLabelText(_L("Filament can not match"));
        }
        else {
            m_label_error_info->SetLabelText(_L(""));
        }
    }
    else {
        m_button_send_print_cmd->Disable();
        m_label_error_info->SetLabelText(_L("Printer is busy, can send gcode only"));
    }

}

std::vector<int> SendToLocalNetPrinterDialog::get_gcode_extruders()
{
    std::vector<int> out_extruders;
    GCodeProcessorResult* current_result = m_plater->get_partplate_list().get_current_slice_result();
    if (current_result) {
        auto&  ps         = current_result->print_statistics;
        double total_cost = 0.0;
        for (auto volume : ps.total_volumes_per_extruder) {
            size_t extruder_id = volume.first;
            double density     = current_result->filament_densities.at(extruder_id);
            double cost        = current_result->filament_costs.at(extruder_id);
            double weight      = volume.second * density * 0.001;
            total_cost += weight * cost * 0.001;
            out_extruders.push_back(extruder_id+1);
        }
    }

    return out_extruders;

}

double SendToLocalNetPrinterDialog::get_gcode_total_weight()
{
    double total_weight = 0.0;
    GCodeProcessorResult* current_result = m_plater->get_partplate_list().get_current_slice_result();
    if (current_result) {
        auto&  ps         = current_result->print_statistics;
        for (auto volume : ps.total_volumes_per_extruder) {
            size_t extruder_id = volume.first;
            double density     = current_result->filament_densities.at(extruder_id);
            // double cost        = current_result->filament_costs.at(extruder_id);
            double weight      = volume.second * density * 0.001;
            total_weight += weight;
        }
    }

    return total_weight;
}

void SendToLocalNetPrinterDialog::update_show_status()
{
    NetworkAgent* agent = Slic3r::GUI::wxGetApp().getAgent();
    DeviceManager* dev = Slic3r::GUI::wxGetApp().getDeviceManager();
    if (!agent) return;
    if (!dev) return;
    MachineObject* obj_ = dev->get_my_machine(m_printer_last_select);
    if (!obj_) {
        if (agent) {
            if (agent->is_user_login()) {
                show_status(PrintDialogStatus::PrintStatusInvalidPrinter);
            }
            else {
                show_status(PrintDialogStatus::PrintStatusNoUserLogin);
            }
        }
        return;
    }

    /* check cloud machine connections */
    if (!obj_->is_lan_mode_printer()) {
        if (!agent->is_server_connected()) {
            agent->refresh_connection();
            show_status(PrintDialogStatus::PrintStatusConnectingServer);
            reset_timeout();
            return;
        }
    }

    if (!obj_->is_info_ready()) {
        if (is_timeout()) {
            show_status(PrintDialogStatus::PrintStatusReadingTimeout);
            return;
        }
        else {
            timeout_count++;
            show_status(PrintDialogStatus::PrintStatusReading);
            return;
        }
        return;
    }

    reset_timeout();

    // reading done
    if (is_blocking_printing(obj_)) {
        show_status(PrintDialogStatus::PrintStatusUnsupportedPrinter);
        return;
    }
    else if (obj_->is_in_upgrading()) {
        show_status(PrintDialogStatus::PrintStatusInUpgrading);
        return;
    }
    else if (obj_->is_system_printing()) {
        show_status(PrintDialogStatus::PrintStatusInSystemPrinting);
        return;
    }

    // check sdcard when if lan mode printer
   /* if (obj_->is_lan_mode_printer()) {
    }*/
	if (obj_->get_sdcard_state() == MachineObject::SdcardState::NO_SDCARD) {
		show_status(PrintDialogStatus::PrintStatusNoSdcard);
		return;
	}


    if (!obj_->is_support_send_to_sdcard) {
        show_status(PrintDialogStatus::PrintStatusNotSupportedSendToSDCard);
        return;
    }

    if (!m_is_in_sending_mode) {
        show_status(PrintDialogStatus::PrintStatusReadingFinished);
        return;
    }
}

bool SendToLocalNetPrinterDialog::is_blocking_printing(MachineObject* obj_)
{
    DeviceManager* dev = Slic3r::GUI::wxGetApp().getDeviceManager();
    if (!dev) return true;

    PresetBundle* preset_bundle = wxGetApp().preset_bundle;
    auto source_model = preset_bundle->printers.get_edited_preset().get_printer_type(preset_bundle);
    auto target_model = obj_->printer_type;

    if (source_model != target_model) {
        std::vector<std::string> compatible_machine = dev->get_compatible_machine(target_model);
        vector<std::string>::iterator it = find(compatible_machine.begin(), compatible_machine.end(), source_model);
        if (it == compatible_machine.end()) {
            return true;
        }
    }

    return false;
}

void SendToLocalNetPrinterDialog::Enable_Refresh_Button(bool en)
{
    if (!en) {
        if (m_button_refresh->IsEnabled()) {
            m_button_refresh->Disable();
            m_button_refresh->SetBackgroundColor(wxColour(0x90, 0x90, 0x90));
            m_button_refresh->SetBorderColor(wxColour(0x90, 0x90, 0x90));
        }
    } else {
        if (!m_button_refresh->IsEnabled()) {
            m_button_refresh->Enable();
            m_button_refresh->SetBackgroundColor(btn_bg_enable);
            m_button_refresh->SetBorderColor(btn_bg_enable);
        }
    }
}

void SendToLocalNetPrinterDialog::show_status(PrintDialogStatus status, std::vector<wxString> params)
{
	if (m_print_status != status)
		BOOST_LOG_TRIVIAL(info) << "select_machine_dialog: show_status = " << status;
	m_print_status = status;

	// m_comboBox_printer
	if (status == PrintDialogStatus::PrintStatusRefreshingMachineList)
		m_comboBox_printer->Disable();
	else
		m_comboBox_printer->Enable();

	// other
	if (status == PrintDialogStatus::PrintStatusInit) {
		update_print_status_msg(wxEmptyString, false, false);
		Enable_Send_Button(false);
		Enable_Refresh_Button(true);
	}
	else if (status == PrintDialogStatus::PrintStatusNoUserLogin) {
		wxString msg_text = _L("No login account, only printers in LAN mode are displayed");
		update_print_status_msg(msg_text, false, true);
		Enable_Send_Button(false);
		Enable_Refresh_Button(true);
	}
	else if (status == PrintDialogStatus::PrintStatusInvalidPrinter) {
		update_print_status_msg(wxEmptyString, true, true);
		Enable_Send_Button(false);
		Enable_Refresh_Button(true);
	}
	else if (status == PrintDialogStatus::PrintStatusConnectingServer) {
		wxString msg_text = _L("Connecting to server");
		update_print_status_msg(msg_text, true, true);
		Enable_Send_Button(true);
		Enable_Refresh_Button(true);
	}
	else if (status == PrintDialogStatus::PrintStatusReading) {
		wxString msg_text = _L("Synchronizing device information");
		update_print_status_msg(msg_text, false, true);
		Enable_Send_Button(false);
		Enable_Refresh_Button(true);
	}
	else if (status == PrintDialogStatus::PrintStatusReadingFinished) {
		update_print_status_msg(wxEmptyString, false, true);
		Enable_Send_Button(true);
		Enable_Refresh_Button(true);
	}
	else if (status == PrintDialogStatus::PrintStatusReadingTimeout) {
		wxString msg_text = _L("Synchronizing device information time out");
		update_print_status_msg(msg_text, true, true);
		Enable_Send_Button(true);
		Enable_Refresh_Button(true);
	}
	else if (status == PrintDialogStatus::PrintStatusInUpgrading) {
		wxString msg_text = _L("Cannot send the print task when the upgrade is in progress");
		update_print_status_msg(msg_text, true, true);
		Enable_Send_Button(false);
		Enable_Refresh_Button(true);
	}
    else if (status == PrintDialogStatus::PrintStatusUnsupportedPrinter) {
        wxString msg_text = _L("The selected printer is incompatible with the chosen printer presets.");
        update_print_status_msg(msg_text, true, true);
        Enable_Send_Button(false);
        Enable_Refresh_Button(true);
    }
	else if (status == PrintDialogStatus::PrintStatusRefreshingMachineList) {
		update_print_status_msg(wxEmptyString, false, true);
		Enable_Send_Button(false);
		Enable_Refresh_Button(false);
	}
	else if (status == PrintDialogStatus::PrintStatusSending) {
		Enable_Send_Button(false);
		Enable_Refresh_Button(false);
	}
	else if (status == PrintDialogStatus::PrintStatusSendingCanceled) {
		Enable_Send_Button(true);
		Enable_Refresh_Button(true);
	}
	else if (status == PrintDialogStatus::PrintStatusNoSdcard) {
		wxString msg_text = _L("An SD card needs to be inserted before send to printer SD card.");
		update_print_status_msg(msg_text, true, true);
		Enable_Send_Button(false);
		Enable_Refresh_Button(true);
    }
    else if (status == PrintDialogStatus::PrintStatusNotOnTheSameLAN) {
        wxString msg_text = _L("The printer is required to be in the same LAN as Creality Print");
        update_print_status_msg(msg_text, true, true);
        Enable_Send_Button(false);
        Enable_Refresh_Button(true);
    }
    else if (status == PrintDialogStatus::PrintStatusNotSupportedSendToSDCard) {
        wxString msg_text = _L("The printer does not support sending to printer SD card.");
        update_print_status_msg(msg_text, true, true);
        Enable_Send_Button(false);
        Enable_Refresh_Button(true);
    }
    else {
		Enable_Send_Button(true);
		Enable_Refresh_Button(true);
    }
}


void SendToLocalNetPrinterDialog::Enable_Send_Button(bool en)
{
    // if (!en) {
    //     if (m_button_ensure->IsEnabled()) {
    //         m_button_ensure->Disable();
    //         m_button_ensure->SetBackgroundColor(wxColour(0x90, 0x90, 0x90));
    //         m_button_ensure->SetBorderColor(wxColour(0x90, 0x90, 0x90));
    //     }
    // } else {
    //     if (!m_button_ensure->IsEnabled()) {
    //         m_button_ensure->Enable();
    //         m_button_ensure->SetBackgroundColor(btn_bg_enable);
    //         m_button_ensure->SetBorderColor(btn_bg_enable);
    //     }
    // }
}

void SendToLocalNetPrinterDialog::on_dpi_changed(const wxRect &suggested_rect)
{
    m_button_refresh->SetMinSize(SELECT_MACHINE_DIALOG_BUTTON_SIZE);
    m_button_refresh->SetCornerRadius(FromDIP(12));
    m_button_ensure->SetMinSize(SELECT_MACHINE_DIALOG_BUTTON_SIZE);
    m_button_ensure->SetCornerRadius(FromDIP(12));
    m_status_bar->msw_rescale();
    Fit();
    Refresh();
}

std::vector<std::string> SendToLocalNetPrinterDialog::get_all_filament_types()
{
    // get filament types
    std::vector<std::string> filament_presets = wxGetApp().preset_bundle->filament_presets;
    std::vector<std::string> all_filament_types;
    for (const auto& preset_name : filament_presets) {
        std::string     filament_type;
        Slic3r::Preset* preset = wxGetApp().preset_bundle->filaments.find_preset(preset_name);
        if (preset) {
            preset->get_filament_type(filament_type);
            all_filament_types.emplace_back(filament_type);
        }
    }

    return all_filament_types;
}

void SendToLocalNetPrinterDialog::set_plate_info(int plate_idx)
{
    //project name
    m_rename_switch_panel->SetSelection(0);

    PartPlate* plate = m_plater->get_partplate_list().get_plate(plate_idx);
    if(!plate) return;
    wxString default_gcode_name = "";

    if(m_plater->only_gcode_mode())
    {
        wxString   last_loaded_gcode = m_plater->get_last_loaded_gcode();
        wxFileName fileName(last_loaded_gcode);
        default_gcode_name = fileName.GetName();
    }
    else {
        ModelObjectPtrs plate_objects = plate->get_objects_on_this_plate();
        wxString        obj0_name     = ""; // the first object's name
        if (plate_objects.size() > 0 && nullptr != plate_objects[0]) {
            obj0_name = wxString::FromUTF8(plate_objects[0]->name);
        }

        auto                                  plate_print_statistics = plate->get_slice_result()->print_statistics;
        const PrintEstimatedStatistics::Mode& plate_time_mode =
            plate_print_statistics.modes[static_cast<size_t>(PrintEstimatedStatistics::ETimeMode::Normal)];

        std::vector<int>         plate_extruders = plate->get_extruders(true);
        std::vector<std::string> filament_types  = get_all_filament_types();
        if (plate_extruders.size() > 0) {
            default_gcode_name = obj0_name + "_" + filament_types[plate_extruders[0] - 1] + "_" + get_bbl_time_dhms(plate_time_mode.time);
        }
    }

    m_rename_text->SetLabelText(default_gcode_name);
    m_current_project_name = default_gcode_name;

    m_rename_normal_panel->Layout();

    init_device_list_from_db();

    //clear combobox
    m_list.clear();
    // m_comboBox_printer->Clear();
    m_printer_last_select = "";
    m_print_info = "";
    // m_comboBox_printer->SetValue(wxEmptyString);
    m_comboBox_printer->Enable();

    // thumbmail
    //wxBitmap bitmap;
    ThumbnailData &data   = plate->thumbnail_data;
    if (data.is_valid()) {
        wxImage image(data.width, data.height);
        image.InitAlpha();
        for (unsigned int r = 0; r < data.height; ++r) {
            unsigned int rr = (data.height - 1 - r) * data.width;
            for (unsigned int c = 0; c < data.width; ++c) {
                unsigned char *px = (unsigned char *) data.pixels.data() + 4 * (rr + c);
                image.SetRGB((int) c, (int) r, px[0], px[1], px[2]);
                image.SetAlpha((int) c, (int) r, px[3]);
            }
        }
        image  = image.Rescale(FromDIP(300), FromDIP(300));
        m_thumbnailPanel->set_thumbnail(image);
    }

    m_scrollable_region->Layout();
    m_scrollable_region->Fit();
    Layout();
    Fit();

    // basic info
    auto       aprint_stats = m_plater->get_partplate_list().get_current_fff_print().print_statistics();
    wxString   time;

    if(m_plater->only_gcode_mode())
    {
        GCodeProcessorResult* gcode_process_result = m_plater->get_partplate_list().get_current_slice_result();
        if (gcode_process_result) {
            time = wxString::Format("%s", short_time(get_time_dhms(gcode_process_result->print_statistics.modes[0].time)));
        }
    }
    else {
        if (plate) {
            if (plate->get_slice_result()) { time = wxString::Format("%s", short_time(get_time_dhms(plate->get_slice_result()->print_statistics.modes[0].time))); }
        }
    }


    double total_weight = aprint_stats.total_weight;
    if(m_plater->only_gcode_mode())
    {
        total_weight = get_gcode_total_weight();
    }

    char weight[64];
    if (wxGetApp().app_config->get("use_inches") == "1") {
        ::sprintf(weight, "  %.2f oz", total_weight*0.035274);
    }else{
        ::sprintf(weight, "  %.2f g", total_weight);
    }

    m_stext_time->SetLabel(time);
    m_stext_weight->SetLabel(weight);
}

bool SendToLocalNetPrinterDialog::Show(bool show)
{
    show_status(PrintDialogStatus::PrintStatusInit);

    // set default value when show this dialog
    if (show) {
        wxGetApp().reset_to_active();
        if(m_plater)
        {
            set_plate_info(m_plater->get_partplate_list().get_curr_plate_index());
        }
        update_user_machine_list();
    }
    else {
        m_show_status = false;
    }

    Layout();
    Fit();
    if (show) { CenterOnParent(); }
    return DPIDialog::Show(show);
}

void SendToLocalNetPrinterDialog::handle_receive_color_match_info(const nlohmann::json& json_data)
{
    const auto& data = json_data["data"];
    std::vector<RemotePrint::MaterialMapPanel::ColorMatchInfo> materialMapInfo;

    std::vector<std::string> all_filament_types = get_all_filament_types();

    for (const auto& matchInfo : data) {
        RemotePrint::MaterialMapPanel::ColorMatchInfo tmpMatchInfo;
        tmpMatchInfo.extruderId = matchInfo["extruderId"];
        tmpMatchInfo.extruderFilamentType = all_filament_types[tmpMatchInfo.extruderId-1];
        tmpMatchInfo.extruderColor = RemotePrint::Utils::hex_string_to_wxcolour(matchInfo["extruderColor"].get<std::string>());
        tmpMatchInfo.matchColor = RemotePrint::Utils::hex_string_to_wxcolour(matchInfo["matchColor"].get<std::string>());

        // if the return matchFilamentType is empty, means NO filamentType is matched (for example, "ABS" CAN NOT be matched to "PLA")
        tmpMatchInfo.matchFilamentType = matchInfo["matchFilamentType"];

        tmpMatchInfo.materialId = matchInfo["materialId"];
        materialMapInfo.emplace_back(tmpMatchInfo);
    }

    if (m_materialMapPanel) {
        m_materialMapPanel->SetMaterialMapInfo(materialMapInfo);
    }

    std::string ipAddress = get_selected_printer_ip();
    if (ipAddress.empty()) {
        return;
    }
    auto deviceData = RemotePrint::DeviceDB::getInstance().get_printer_data(ipAddress);
    update_send_print_cmd_button(deviceData);

    Layout();
    Fit();
    Thaw();
}

void SendToLocalNetPrinterDialog::on_checkbox_open_cfs(wxCommandEvent& event) 
{
    if (event.IsChecked()) {
        if (m_materialMapPanel) {
            m_materialMapPanel->Show();
        }
    } else {
        if(m_materialMapPanel) {
            m_materialMapPanel->Hide();
        }
    }

    Layout();
    Fit();
    Thaw();
}

void SendToLocalNetPrinterDialog::on_timer_hide_progressbar(wxTimerEvent& event)
{
    m_progress_bar->Show(false);
    m_label_progress->Show(false);

    Layout();
    Fit();
}

SendToLocalNetPrinterDialog::~SendToLocalNetPrinterDialog()
{
    if(m_show_progress_timer) {
        m_show_progress_timer->Stop();
        delete m_show_progress_timer;
        m_show_progress_timer = nullptr;
    }
}

}
}
