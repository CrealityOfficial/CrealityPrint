#include "Upload3mfToCloud.hpp"
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

namespace Slic3r { namespace GUI {

#define INITIAL_NUMBER_OF_MACHINES 0
#define LIST_REFRESH_INTERVAL 200
#define MACHINE_LIST_REFRESH_INTERVAL 2000

wxDEFINE_EVENT(EVT_UPDATE_USER_MACHINE_LIST, wxCommandEvent);
wxDEFINE_EVENT(EVT_PRINT_JOB_CANCEL, wxCommandEvent);
wxDEFINE_EVENT(EVT_SEND_JOB_SUCCESS, wxCommandEvent);
// wxDEFINE_EVENT(EVT_CLEAR_IPADDRESS, wxCommandEvent);

static wxString extract_ip_address(const wxString& combo_selection)
{
    std::regex  ip_regex(R"(\(([^)]+)\))");
    std::smatch match;
    std::string combo_str = combo_selection.ToStdString();
    if (std::regex_search(combo_str, match, ip_regex)) {
        return wxString(match[1].str());
    }
    return "";
}

void Upload3mfToCloudDialog::init_device_list_from_db()
{
    m_comboBox_printer->Clear();
    int defaultSelectionIndex = wxNOT_FOUND;
    int firstOnlineIndex      = wxNOT_FOUND;

    for (size_t i = 0; i < RemotePrint::DeviceDB::getInstance().devices.size(); ++i) {
        const auto& device              = RemotePrint::DeviceDB::getInstance().devices[i];
        wxString    online_status       = device.online ? _L("Online") : _L("Offline");
        wxString    printer_busy_status = device.deviceState ? _L("Busy") : _L("Ready");
        m_comboBox_printer->Append(device.model + " (" + device.address + ")" + "                    " + online_status);

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

void Upload3mfToCloudDialog::stripWhiteSpace(std::string& str)
{
    if (str == "") {
        return;
    }

    string::iterator cur_it;
    cur_it = str.begin();

    while (cur_it != str.end()) {
        if ((*cur_it) == '\n' || (*cur_it) == ' ') {
            cur_it = str.erase(cur_it);
        } else {
            cur_it++;
        }
    }
}

wxString Upload3mfToCloudDialog::format_text(wxString& m_msg)
{
    if (wxGetApp().app_config->get("language") != "zh_CN") {
        return m_msg;
    }

    wxString out_txt      = m_msg;
    wxString count_txt    = "";
    int      new_line_pos = 0;

    for (int i = 0; i < m_msg.length(); i++) {
        auto text_size = m_statictext_printer_msg->GetTextExtent(count_txt);
        if (text_size.x < (FromDIP(400))) {
            count_txt += m_msg[i];
        } else {
            out_txt.insert(i - 1, '\n');
            count_txt = "";
        }
    }
    return out_txt;
}

void Upload3mfToCloudDialog::check_focus(wxWindow* window)
{
    if (window == m_rename_input || window == m_rename_input->GetTextCtrl()) {
        on_rename_enter();
    }
}

void Upload3mfToCloudDialog::check_fcous_state(wxWindow* window)
{
    check_focus(window);
    auto children = window->GetChildren();
    for (auto child : children) {
        check_fcous_state(child);
    }
}

void Upload3mfToCloudDialog::on_rename_click(wxCommandEvent& event)
{
    m_is_rename_mode = true;
    m_rename_input->GetTextCtrl()->SetValue(m_current_project_name);
    m_rename_switch_panel->SetSelection(1);
    m_rename_input->GetTextCtrl()->SetFocus();
    m_rename_input->GetTextCtrl()->SetInsertionPointEnd();
}

void Upload3mfToCloudDialog::on_rename_enter()
{
    if (m_is_rename_mode == false) {
        return;
    } else {
        m_is_rename_mode = false;
    }

    auto new_file_name = m_rename_input->GetTextCtrl()->GetValue();

    wxString temp;
    int      num = 0;
    for (auto t : new_file_name) {
        if (t == wxString::FromUTF8("\x20")) {
            num++;
            if (num == 1)
                temp += t;
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
            info_line    = _L("Name is invalid;") + "\n" + _L("illegal characters:") + " " + unusable_symbols;
            m_valid_type = NoValid;
            break;
        }
    }

    if (m_valid_type == Valid && new_file_name.find(unusable_suffix) != std::string::npos) {
        info_line    = _L("Name is invalid;") + "\n" + _L("illegal suffix:") + "\n\t" + from_u8(PresetCollection::get_suffix_modified());
        m_valid_type = NoValid;
    }

    if (m_valid_type == Valid && new_file_name.empty()) {
        info_line    = _L("The name is not allowed to be empty.");
        m_valid_type = NoValid;
    }

    if (m_valid_type == Valid && new_file_name.find_first_of(' ') == 0) {
        info_line    = _L("The name is not allowed to start with space character.");
        m_valid_type = NoValid;
    }

    if (m_valid_type == Valid && new_file_name.find_last_of(' ') == new_file_name.length() - 1) {
        info_line    = _L("The name is not allowed to end with space character.");
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
    Plater* plater         = wxGetApp().mainframe->plater();
    m_current_project_name = new_file_name;
    m_rename_switch_panel->SetSelection(0);
    m_rename_text->SetLabel(m_current_project_name);
    wxString formatName = m_current_project_name.append(".3mf");
    plater->set_project_filename(formatName);
    m_rename_normal_panel->Layout();
}

Upload3mfToCloudDialog::Upload3mfToCloudDialog(Plater* plater)
    : DPIDialog(static_cast<wxWindow*>(wxGetApp().mainframe),
                wxID_ANY,
                _L("upload 3mf to crealitycloud"),
                wxDefaultPosition,
                wxDefaultSize,
                wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER)
    , m_plater(plater)
    , m_export_3mf_cancel(false)
{
#ifdef __WINDOWS__
    SetDoubleBuffered(true);
#endif //__WINDOWS__
    
    wxSize minSize = wxSize(FromDIP(500), FromDIP(300)); // 设置最小尺寸
    // wxSize initialSize = wxSize(1024, 768); // 设置初始尺寸
    wxSize initialSize = wxSize(FromDIP(770), FromDIP(721));
    SetMinSize(minSize);
    SetSize(initialSize);
    m_scrolledWindow = new wxScrolledWindow(this, wxID_ANY);
    if (wxGetApp().dark_mode()) {
        m_scrolledWindow->SetBackgroundColour(wxColour(75, 75, 73));
    } else {
        m_scrolledWindow->SetBackgroundColour(wxColour(255, 255, 255));
    }
    m_scrolledWindow->SetScrollRate(10, 10);
    wxBoxSizer* wsizer = new wxBoxSizer(wxVERTICAL);
    wsizer->Add(m_scrolledWindow, 1, wxEXPAND, 0);
    wsizer->SetMinSize(initialSize);
    SetSizer(wsizer);
    Refresh();
    wsizer->Fit(this);
    //m_scrolledWindow->SetMinSize(wxSize(FromDIP(752), FromDIP(755)));
    //m_scrolledWindow->SetMaxSize(wxSize(FromDIP(700), FromDIP(900)));
    //m_scrolledWindow->SetSize(initialSize);
    Bind(wxEVT_SHOW, [this](wxShowEvent& event) {
        if (event.IsShown()) {
            wxPoint position = GetPosition();
            if (position.y < 0)
            {
                wxSize newSize = wxSize(FromDIP(770), FromDIP(721) + position.y * 2);
                SetSize(newSize);
                SetPosition(wxPoint(position.x, 0));
            }
                
        }
        event.Skip();
    });
    // bind
    Bind(wxEVT_CLOSE_WINDOW, &Upload3mfToCloudDialog::on_cancel, this);
    Bind(wxEVT_THREAD, &Upload3mfToCloudDialog::on_progress_update, this);

    // font
    SetFont(wxGetApp().normal_font());

    // icon
    std::string icon_path = (boost::format("%1%/images/Creative3DTitle.ico") % resources_dir()).str();
    SetIcon(wxIcon(encode_path(icon_path.c_str()), wxBITMAP_TYPE_ICO));

    Freeze();
    SetBackgroundColour(m_colour_def_color);

    m_sizer_main = new wxBoxSizer(wxVERTICAL);

    m_sizer_main->SetMinSize(wxSize(0, -1));

    m_line_top = new wxPanel(m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxSize(-1, 1), wxTAB_TRAVERSAL);
    m_line_top->SetBackgroundColour(wxColour(166, 169, 170));

    // m_scrollable_region       = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    m_sizer_scrollable_region = new wxBoxSizer(wxVERTICAL);

    m_line_materia = new wxPanel(m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxSize(-1, 1), wxTAB_TRAVERSAL);
    m_line_materia->SetForegroundColour(wxColour(238, 238, 238));
    m_line_materia->SetBackgroundColour(wxColour(238, 238, 238));

    // wxBoxSizer* m_sizer_plate = new wxBoxSizer(wxVERTICAL);

    wxSizerFlags flags;
    flags.Border(wxTOP | wxBOTTOM, FromDIP(5));

    wxBoxSizer* m_sizer_printer = new wxBoxSizer(wxHORIZONTAL);

    auto printerimg = new wxStaticBitmap(m_scrolledWindow, wxID_ANY, create_scaled_bitmap("printer_3mf", this, 18), wxDefaultPosition,
                                         wxSize(FromDIP(18), FromDIP(18)), 0);
    m_sizer_printer->Add(printerimg, 0, wxALIGN_LEFT | wxALL, FromDIP(2));
    m_sizer_printer->Add(new wxStaticText(m_scrolledWindow, wxID_ANY, _L("Printer") + ":", wxDefaultPosition, wxSize(-1, -1), 0), 0,
                         wxALIGN_LEFT | wxALL,
                         FromDIP(5));
    m_stext_printer = new wxStaticText(m_scrolledWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(-1, -1), 0);

    m_sizer_printer->Add(m_stext_printer, 0, wxALL, FromDIP(5));
    m_sizer_material = new wxBoxSizer(wxHORIZONTAL);

    wxBoxSizer* m_sizer_process = new wxBoxSizer(wxHORIZONTAL);
    auto        processimg      = new wxStaticBitmap(m_scrolledWindow, wxID_ANY, create_scaled_bitmap("process_3mf", this, 18), wxDefaultPosition,
                                                     wxSize(FromDIP(18), FromDIP(18)), 0);
    m_sizer_process->Add(processimg, 0, wxALIGN_LEFT | wxALL, FromDIP(2));
    m_sizer_process->Add(new wxStaticText(m_scrolledWindow, wxID_ANY, _L("Process") + ":", wxDefaultPosition, wxSize(-1, -1), 0), 0,
                         wxALIGN_LEFT | wxALL,
                         FromDIP(5));
    m_stext_process = new wxStaticText(m_scrolledWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(-1, -1), 0);

    m_sizer_process->Add(m_stext_process, 0, wxALL, FromDIP(5));

    wxBoxSizer* m_sizer_upload_type = new wxBoxSizer(wxHORIZONTAL);
    auto        uploadimg           = new wxStaticBitmap(m_scrolledWindow, wxID_ANY, create_scaled_bitmap("upload_3mf", this, 18), wxDefaultPosition,
                                                         wxSize(FromDIP(18), FromDIP(18)), 0);
    m_sizer_upload_type->Add(uploadimg, 0, wxALIGN_LEFT | wxALL, FromDIP(5));
    m_sizer_upload_type->Add(new wxStaticText(m_scrolledWindow, wxID_ANY, _L("Upload type") + ":", wxDefaultPosition, wxSize(-1, -1), 0), 0,
                             wxALIGN_LEFT | wxALL, FromDIP(5));

    wxPanel* panel = new wxPanel(m_scrolledWindow, wxID_ANY);

    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);

    m_radio_upload_new  = new wxRadioButton(panel, wxID_ANY, _L("New model upload"));
    m_radio_upload_exit = new wxRadioButton(panel, wxID_ANY, _L("Upload an existing model"));

    sizer->Add(m_radio_upload_new, 0, wxALL, 10);
    sizer->Add(m_radio_upload_exit, 0, wxALL, 10);

    panel->SetSizerAndFit(sizer);

    m_sizer_upload_type->Add(panel);
    wxFont font(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);

    wxBoxSizer* m_sizer_plate_count = new wxBoxSizer(wxHORIZONTAL);
    auto        platecountimg       = new wxStaticBitmap(m_scrolledWindow, wxID_ANY, create_scaled_bitmap("plate_3mf", this, 18), wxDefaultPosition,
                                                         wxSize(FromDIP(18), FromDIP(18)), 0);
    m_sizer_plate_count->Add(platecountimg, 0, wxALIGN_LEFT | wxALL, FromDIP(2));
    m_sizer_plate_count->Add(new wxStaticText(m_scrolledWindow, wxID_ANY, _L("Divide plate") + ":", wxDefaultPosition, wxSize(-1, -1), 0), 0,
                             wxALIGN_LEFT | wxALL, FromDIP(5));
    m_stext_plate_count = new wxStaticText(m_scrolledWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(-1, -1), 0);
    m_stext_plate_count->SetFont(font);

    m_sizer_plate_count->Add(m_stext_plate_count, 0, wxALL, FromDIP(5));
    m_stext_plate_count->SetFont(font);

    wxBoxSizer* m_sizer_plate_sliced_count = new wxBoxSizer(wxHORIZONTAL);
    auto        sliceimg = new wxStaticBitmap(m_scrolledWindow, wxID_ANY, create_scaled_bitmap("slice_3mf", this, 18), wxDefaultPosition,
                                              wxSize(FromDIP(18), FromDIP(18)), 0);
    m_sizer_plate_sliced_count->Add(sliceimg, 0, wxALIGN_LEFT | wxALL, FromDIP(2));
    m_sizer_plate_sliced_count->Add(new wxStaticText(m_scrolledWindow, wxID_ANY, _L("Sliced plate") + ":", wxDefaultPosition, wxSize(-1, -1), 0), 0,
                                    wxALIGN_LEFT | wxALL, FromDIP(5));
    m_stext_plate_sliced_count = new wxStaticText(m_scrolledWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(-1, -1), 0);

    m_sizer_plate_sliced_count->Add(m_stext_plate_sliced_count, 0, wxALL, FromDIP(5));

    btn_bg_enable = StateColor(std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Pressed),
                               std::pair<wxColour, int>(wxColour(150, 150, 155), StateColor::Hovered),
                               std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Normal),
                               std::pair<wxColour, int>(wxColour(200, 200, 200), StateColor::Disabled));

    btn_bd_enable = StateColor(std::pair<wxColour, int>(wxColour(38, 46, 48), StateColor::Normal));

    // line schedule
    m_line_schedule = new wxPanel(m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    m_line_schedule->SetBackgroundColour(wxColour(238, 238, 238));

    wxBoxSizer* m_sizer_send_btns = new wxBoxSizer(wxHORIZONTAL);

    m_sizer_send_btns->Add(0, 0, 0, wxEXPAND, FromDIP(22));
    m_button_ensure = new Button(m_scrolledWindow, _L("Export and upload"));
    m_button_ensure->SetBackgroundColor(btn_bg_enable);
    m_button_ensure->SetBorderColor(btn_bd_enable);
    m_button_ensure->SetTextColor(StateColor::lightModeColorFor("#000000"));

    m_button_ensure->SetSize(SELECT_MACHINE_DIALOG_BUTTON_SIZE);
    m_button_ensure->SetMinSize(SELECT_MACHINE_DIALOG_BUTTON_SIZE);
    m_button_ensure->SetCornerRadius(FromDIP(12));
    m_button_ensure->Bind(wxEVT_BUTTON, &Upload3mfToCloudDialog::on_upload_3mf, this);

    m_button_send_print_cmd = new Button(m_scrolledWindow, _L("Cancel"));
    m_button_send_print_cmd->SetBackgroundColor(btn_bg_enable);
    m_button_send_print_cmd->SetBorderColor(btn_bd_enable);
    m_button_send_print_cmd->SetTextColor(StateColor::darkModeColorFor("#000000"));
    if (wxGetApp().dark_mode()) {
        m_button_ensure->SetTextColor(StateColor::darkModeColorFor("#FFFFFE"));
        m_button_send_print_cmd->SetTextColor(StateColor::darkModeColorFor("#FFFFFE"));
    }
    m_button_send_print_cmd->SetSize(SELECT_MACHINE_DIALOG_BUTTON_SIZE);
    m_button_send_print_cmd->SetMinSize(SELECT_MACHINE_DIALOG_BUTTON_SIZE);
    m_button_send_print_cmd->SetCornerRadius(FromDIP(12));
    m_button_send_print_cmd->Bind(wxEVT_BUTTON, &Upload3mfToCloudDialog::send_and_print, this);

    m_progress_bar = new wxGauge(m_scrolledWindow, wxID_ANY, 100, wxDefaultPosition, wxSize(FromDIP(550), FromDIP(wxDefaultSize.GetHeight())),
                                 wxGA_HORIZONTAL);
    m_progress_bar->SetValue(0);
    m_progress_bar->SetBackgroundColour(wxColour(30, 144, 255));
    m_progress_bar->Hide();

    m_label_progress = new wxStaticText(m_scrolledWindow, wxID_ANY, "0%", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
    m_label_progress->SetForegroundColour(*wxWHITE);
    m_label_progress->Hide();

    wxBoxSizer* m_sizer_progress_with_text = new wxBoxSizer(wxVERTICAL);
    m_sizer_progress_with_text->Add(m_label_progress, 0, wxALIGN_CENTER | wxBOTTOM, 0);
    m_sizer_progress_with_text->Add(m_progress_bar, 0, wxEXPAND, 0);

    m_sizer_send_btns->Add(m_button_ensure, 0, wxEXPAND | wxBOTTOM, FromDIP(10));
    m_sizer_send_btns->AddSpacer(FromDIP(30)); // Add some space between the buttons
    m_sizer_send_btns->Add(m_button_send_print_cmd, 0, wxEXPAND | wxBOTTOM, FromDIP(10));

    // Create a new horizontal sizer for the progress bar
    m_sizer_progress = new wxBoxSizer(wxHORIZONTAL);
    m_sizer_progress->Add(m_sizer_progress_with_text, 1, wxEXPAND | wxALL, 0);

    // file name
    // rename normal
    m_rename_switch_panel = new wxSimplebook(m_scrolledWindow);
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

    // rename edit
    auto m_rename_edit_panel = new wxPanel(m_rename_switch_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    m_rename_edit_panel->SetBackgroundColour(*wxWHITE);
    auto rename_edit_sizer_v = new wxBoxSizer(wxVERTICAL);

    m_rename_input = new ::TextInput(m_rename_edit_panel, wxEmptyString, wxEmptyString, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                     wxTE_PROCESS_ENTER);
    m_rename_input->GetTextCtrl()->SetFont(::Label::Body_13);
    m_rename_input->SetSize(wxSize(FromDIP(380), FromDIP(24)));
    m_rename_input->SetMinSize(wxSize(FromDIP(380), FromDIP(24)));
    m_rename_input->SetMaxSize(wxSize(FromDIP(380), FromDIP(24)));
    m_rename_input->Bind(wxEVT_TEXT_ENTER, [this](auto& e) { on_rename_enter(); });
    m_rename_input->Bind(wxEVT_KILL_FOCUS, [this](auto& e) {
        if (!m_rename_input->HasFocus() && !m_rename_text->HasFocus())
            on_rename_enter();
        else
            e.Skip();
    });
    rename_edit_sizer_v->Add(m_rename_input, 1, wxALIGN_CENTER, 0);

    m_rename_edit_panel->SetSizer(rename_edit_sizer_v);
    m_rename_edit_panel->Layout();
    rename_edit_sizer_v->Fit(m_rename_edit_panel);

    m_rename_button->Bind(wxEVT_BUTTON, &Upload3mfToCloudDialog::on_rename_click, this);
    m_rename_switch_panel->AddPage(m_rename_normal_panel, wxEmptyString, true);
    m_rename_switch_panel->AddPage(m_rename_edit_panel, wxEmptyString, false);

    Bind(wxEVT_CHAR_HOOK, [this](wxKeyEvent& e) {
        if (e.GetKeyCode() == WXK_ESCAPE) {
            if (m_rename_switch_panel->GetSelection() == 0) {
                e.Skip();
            } else {
                m_rename_switch_panel->SetSelection(0);
                m_rename_text->SetLabel(m_current_project_name);
                m_rename_normal_panel->Layout();
            }
        } else {
            e.Skip();
        }
    });

    /*m_scrollable_region->Bind(wxEVT_LEFT_DOWN, [this](auto& e) {
        check_fcous_state(this);
        e.Skip();
        });*/

    /*Bind(wxEVT_LEFT_DOWN, [this](auto& e) {
        check_fcous_state(this);
        e.Skip();
        })*/
    ;

    m_sizer_main->Add(m_sizer_progress, 0, wxEXPAND, 0);
    // m_sizer_main->Add(m_sizer_object_name, 0, wxEXPAND, FromDIP(10));
    m_sizer_main->Add(m_line_top, 0, wxEXPAND, 0);
    m_sizer_main->Add(0, 0, 0, wxEXPAND | wxTOP, FromDIP(6));
    m_sizer_main->Add(m_rename_switch_panel, 0, wxALIGN_CENTER_HORIZONTAL, 0);

    // m_sizer_main->Add(0, 0, 0, wxTOP, FromDIP(10));
    // m_sizer_main->Add(m_scrollable_region, 0, wxALIGN_CENTER_HORIZONTAL, 0);

    // m_sizer_main->Add(m_rename_switch_panel, 0, wxALIGN_CENTER_HORIZONTAL, 0);
    // m_sizer_main->Add(0, 0, 0, wxEXPAND | wxTOP, FromDIP(6));
    m_sizer_main->Add(m_line_materia, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(30));
    m_sizer_main->Add(0, 0, 0, wxEXPAND | wxTOP, FromDIP(2));
    m_sizer_main->Add(m_materialMapPanel, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(80));
    // m_sizer_main->Add(m_sizer_thumbnail, 0, wxALIGN_CENTER_HORIZONTAL, 0);
    m_sizer_main->Add(m_sizer_scrollable_region, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(20));

    // m_sizer_main->Add(sizer_error_info, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(80));
    m_sizer_main->Add(m_sizer_printer, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(10));
    m_sizer_main->AddSpacer(FromDIP(5));
    m_sizer_main->Add(m_sizer_material, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(10));
    m_sizer_main->AddSpacer(FromDIP(5));
    m_sizer_main->Add(m_sizer_process, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(10));
    m_sizer_main->AddSpacer(FromDIP(5));
    m_sizer_main->Add(m_sizer_plate_count, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(10));
    m_sizer_main->AddSpacer(FromDIP(5));
    m_sizer_main->Add(m_sizer_plate_sliced_count, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(10));
    m_sizer_main->AddSpacer(FromDIP(5));
    m_sizer_main->Add(m_sizer_upload_type, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(10));

    // m_sizer_main->Add(m_sizer_checkboxes, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(10));
    m_sizer_main->AddSpacer(FromDIP(5));
    m_sizer_main->Add(m_line_schedule, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(30));
    m_sizer_main->AddSpacer(FromDIP(10));
    m_sizer_main->Add(m_sizer_send_btns, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(280));

    // show_print_failed_info(false);
    //SetSizer(m_sizer_main);
    Layout();
    //Fit();
    Thaw();

    init_bind();
    init_timer();
    // CenterOnParent();
    Centre(wxBOTH);
    wxGetApp().UpdateDlgDarkUI(this);
    if (wxGetApp().mainframe->get_printer_mgr_view()) {
        // wxGetApp().mainframe->get_printer_mgr_view()->RegisterHandler("receive_color_match_info", [this](const nlohmann::json& json_data) {
        //     this->handle_receive_color_match_info(json_data);
        // });

        // wxGetApp().mainframe->get_printer_mgr_view()->RegisterHandler("receive_device_list", [this](const nlohmann::json& json_data) {
        //     this->handle_receive_device_list(json_data);
        // });
    }
    m_scrolledWindow->SetSizer(m_sizer_main);
    m_scrolledWindow->FitInside();
    m_scrolledWindow->SetScrollRate(10, 10);
    m_scrolledWindow->Layout();
    //m_scrolledWindow->SetVirtualSize(-1, 50);
    Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { 
        this->m_scrolledWindow->Layout();
        e.Skip();
    });
  
    //this->Bind(wxEVT_SIZE, &Upload3mfToCloudDialog::OnResize, this);
}
 void Upload3mfToCloudDialog::OnResize(wxSizeEvent& event)
 {
    m_scrolledWindow->Layout();
    event.Skip(); // Allow the event to be handled further
 }
void Upload3mfToCloudDialog::update_print_error_info(int code, std::string msg, std::string extra)
{
    m_print_error_code  = code;
    m_print_error_msg   = msg;
    m_print_error_extra = extra;
}

void Upload3mfToCloudDialog::on_progress_update(wxThreadEvent& event)
{
    int progressValue = event.GetInt();

    if (!m_progress_bar->IsShown()) {
        m_progress_bar->Show(true);
        m_label_progress->Show(true);
        m_sizer_main->Layout();
    }

    m_progress_bar->SetValue(progressValue);
    wxString progressText = wxString::Format("%d%%", progressValue);
    m_label_progress->SetLabel(progressText);

    // if (progressValue >= 100) {
    //     m_sizer_progress->Show(false);
    //     m_sizer_main->Layout();
    // }

    Layout();
    Fit();
    Thaw();
}

void Upload3mfToCloudDialog::prepare(int print_plate_idx)
{
    m_print_plate_idx = print_plate_idx;

    wxGetApp().mainframe->get_printer_mgr_view()->RequestDeviceListFromDB();
}

void Upload3mfToCloudDialog::update_priner_status_msg(wxString msg, bool is_warning)
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
        msg = format_text(msg);

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

void Upload3mfToCloudDialog::update_print_status_msg(wxString msg, bool is_warning, bool is_printer_msg)
{
    if (is_printer_msg) {
        update_priner_status_msg(msg, is_warning);
    } else {
        update_priner_status_msg(wxEmptyString, false);
    }
}

void Upload3mfToCloudDialog::init_bind()
{
    Bind(wxEVT_TIMER, &Upload3mfToCloudDialog::on_timer, this);
    // Bind(EVT_CLEAR_IPADDRESS, &Upload3mfToCloudDialog::clear_ip_address_config, this);
}

void Upload3mfToCloudDialog::init_timer()
{
    m_refresh_timer = new wxTimer();
    m_refresh_timer->SetOwner(this);
}

void Upload3mfToCloudDialog::on_cancel(wxCloseEvent& event) { this->EndModal(wxID_CANCEL); }

std::string Upload3mfToCloudDialog::get_selected_printer_ip()
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

int findNumberForMultipleOfThree(int n)
{
    int sum  = 0;
    int temp = n;
    while (temp != 0) {
        sum += temp % 10;
        temp = std::floor(temp / 10);
    }
    int m = 3 - (sum % 3);
    if (m == 3) {
        m = 0;
    }
    return m;
}

void Upload3mfToCloudDialog::get_current_plate_color()
{
    m_sizer_material->Clear(true);
    m_sizer_scrollable_region->Clear(true);

    auto                     plate_list                 = wxGetApp().plater()->get_partplate_list().get_plate_list();
    nlohmann::json           plate_extruder_colors_json = nlohmann::json::array();
    std::vector<std::string> all_extruder_colors        = Slic3r::GUI::wxGetApp().plater()->get_extruder_colors_from_plater_config();
    std::vector<std::string> all_filament_types         = get_all_filament_types();

    wxScrolledWindow* scrolledWindow1 = new wxScrolledWindow(m_scrolledWindow, wxID_ANY);
    scrolledWindow1->SetMinSize(wxSize(FromDIP(710), FromDIP(400)));
    scrolledWindow1->SetMaxSize(wxSize(FromDIP(710), FromDIP(400)));
    scrolledWindow1->SetScrollRate(0, 5);

    wxWrapSizer* wrapSizer = new wxWrapSizer(wxHORIZONTAL);

    wxColor cardColor  = wxGetApp().dark_mode() ? wxColor(45, 45, 49) : wxColor(240, 243, 249);
    int     plateCount = 0;
    for (auto plate : plate_list) {
        plateCount++;
        ThumbnailData& data = plate->thumbnail_data;
        if (data.is_valid()) {
            wxImage image(data.width, data.height);
            image.InitAlpha();
            for (unsigned int r = 0; r < data.height; ++r) {
                unsigned int rr = (data.height - 1 - r) * data.width;
                for (unsigned int c = 0; c < data.width; ++c) {
                    unsigned char* px = (unsigned char*) data.pixels.data() + 4 * (rr + c);
                    image.SetRGB((int) c, (int) r, px[0], px[1], px[2]);
                    image.SetAlpha((int) c, (int) r, px[3]);
                }
            }
            image = image.Rescale(FromDIP(100), FromDIP(100));

            image = image.Rescale(FromDIP(100), FromDIP(100));
            wxPanel*    panel = new wxPanel(scrolledWindow1, wxID_ANY, wxDefaultPosition, wxSize(FromDIP(220), FromDIP(182)));
            panel->SetBackgroundColour(cardColor);
            wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
            panel->SetSizer(sizer);

            

            wxStaticText* cardIndex = new wxStaticText(panel, wxID_ANY, wxString::FromUTF8(std::to_string(plateCount).c_str()), wxPoint(5,5),
                                                       wxDefaultSize, 0);
            cardIndex->SetForegroundColour(wxGetApp().dark_mode() ?*wxWHITE : *wxBLACK);
            cardIndex->SetFont(::Label::Body_12);
            cardIndex->SetBackgroundColour(cardColor);
            //sizer->Add(cardIndex, 0, wxALIGN_LEFT | wxALIGN_TOP | wxALL, 5);

            wxPanel* panel2 = new wxPanel(panel, wxID_ANY);
            panel2->SetBackgroundColour(cardColor);
            panel2->SetMinSize(wxSize(FromDIP(100), FromDIP(100)));
            panel2->SetMaxSize(wxSize(FromDIP(100), FromDIP(100)));
            wxBoxSizer* panel2sizer = new wxBoxSizer(wxVERTICAL);
            panel2->SetSizer(panel2sizer);
            sizer->AddStretchSpacer();
            sizer->Add(panel2, 0, wxALIGN_CENTER | wxALL, 0);
            

            auto m_thumbnailPanel = new ThumbnailPanel(panel2);
            m_thumbnailPanel->SetBackgroundColour(cardColor);
            //m_thumbnailPanel->set_force_draw_background(true);
            m_thumbnailPanel->set_background_size(wxSize(FromDIP(100), FromDIP(100)));
            m_thumbnailPanel->SetSize(wxSize(FromDIP(100), FromDIP(100)));
            m_thumbnailPanel->SetMinSize(wxSize(FromDIP(100), FromDIP(100)));
            m_thumbnailPanel->SetMaxSize(wxSize(FromDIP(100), FromDIP(100)));
            m_thumbnailPanel->set_thumbnail(image);
            sizer->Layout();
           
            wxBitmap        emptyBitmap;
            wxBitmapButton* iconButton = new wxBitmapButton(panel, wxID_ANY, emptyBitmap, wxDefaultPosition, wxDefaultSize,
                                                            wxBORDER_NONE | wxBU_AUTODRAW);
            wxString        tipStr     = "";
            if (plate->is_slice_result_valid()) {
                wxBitmap bmp = create_scaled_bitmap("plate_sliced", this, FromDIP(18));
                iconButton   = new wxBitmapButton(panel, wxID_ANY, bmp, wxDefaultPosition, wxSize(FromDIP(20), FromDIP(20)),
                                                  wxBORDER_NONE | wxBU_AUTODRAW);
                tipStr       = _L("Sliced");
            }
            if (plate->empty()) {
                wxBitmap bmp = create_scaled_bitmap("plate_empty", this, FromDIP(18));
                iconButton   = new wxBitmapButton(panel, wxID_ANY, bmp, wxDefaultPosition, wxSize(FromDIP(20), FromDIP(20)),
                                                  wxBORDER_NONE | wxBU_AUTODRAW);
                tipStr       = _L("Empty");
            }
            if (tipStr != "")
                iconButton->SetToolTip(tipStr);
            iconButton->SetBackgroundColour(cardColor);
            sizer->Add(iconButton, 0, wxALIGN_RIGHT | wxALIGN_BOTTOM | wxALL, 5);
            sizer->Layout();
            wrapSizer->Add(panel, 0, wxALL, 5);
        }

        std::vector<int> plate_extruders = plate->get_extruders(true);
        if (plate_extruders.size() > 0) {
            for (const auto& extruder : plate_extruders) {
                if (all_extruder_colors.size() > (extruder - 1)) {
                    bool findId = false;
                    for (int i = 0; i < plate_extruder_colors_json.size(); i++) {
                        auto extruderId = plate_extruder_colors_json.at(i)["extruder_id"];
                        if (extruderId == extruder) {
                            findId = true;
                            break;
                        }
                    }
                    if (findId)
                        continue;
                    nlohmann::json extruder_info = {{"extruder_id", extruder},
                                                    {"extruder_color", all_extruder_colors[extruder - 1]},
                                                    {"filament_type", all_filament_types[extruder - 1]}};
                    plate_extruder_colors_json.push_back(extruder_info);
                }
            }
        }
    }

    int plateSize = plate_list.size();
    int placehold = plateSize < 6 && plateSize >= 1 ? 6 - plateSize : findNumberForMultipleOfThree(plateSize);
    for (int i = 1; i <= placehold; i++) {
        wxPanel* panel1 = new wxPanel(scrolledWindow1, wxID_ANY, wxDefaultPosition, wxSize(FromDIP(220), FromDIP(182)));
        panel1->SetBackgroundColour(cardColor);
        wxBitmap    bmp        = create_scaled_bitmap("placehold_3mf", this, 80);
        auto        holderimg  = new wxBitmapButton(panel1, wxID_ANY, bmp, wxDefaultPosition, wxSize(FromDIP(90), FromDIP(80)),
                                                    wxBORDER_NONE | wxBU_AUTODRAW);
        holderimg->SetBackgroundColour(cardColor);
        wxBoxSizer* panelSizer = new wxBoxSizer(wxVERTICAL);
        panelSizer->SetMinSize(wxSize(FromDIP(220), FromDIP(182)));
        panelSizer->Add(holderimg, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP, FromDIP(40));
        panel1->SetSizer(panelSizer);

        wrapSizer->Add(panel1, 1, wxALL, 5);
    }
    scrolledWindow1->SetSizer(wrapSizer);
    scrolledWindow1->SetScrollRate(0, 10); // 设置垂直滚动速率
    scrolledWindow1->Layout();
    m_sizer_scrollable_region->Add(scrolledWindow1);

    wxBoxSizer* m_sizer_material_list = new wxBoxSizer(wxHORIZONTAL);
    auto        materialimg = new wxStaticBitmap(this, wxID_ANY, create_scaled_bitmap("material_3mf", this, 18), wxDefaultPosition,
                                                 wxSize(FromDIP(18), FromDIP(18)), 0);
    m_sizer_material->Add(materialimg, 0, wxALIGN_LEFT | wxALL, FromDIP(2));
    m_sizer_material->Add(new wxStaticText(m_scrolledWindow, wxID_ANY, _L("Filament") + ":", wxDefaultPosition, wxSize(-1, -1), 0), 0,
                          wxALIGN_LEFT | wxALL,
                          FromDIP(2));
    auto extruderSize = plate_extruder_colors_json.size();
    for (int i = 0; i < plate_extruder_colors_json.size(); i++) {
        std::string       color      = plate_extruder_colors_json.at(i)["extruder_color"];
        std::string       type       = plate_extruder_colors_json.at(i)["filament_type"];
        auto              extruderId = plate_extruder_colors_json.at(i)["extruder_id"];
        std::stringstream ss;
        ss << extruderId;
        std::string extruder_id_str = ss.str();
        wxString    wxType          = wxString::FromUTF8(type.c_str());
        wxString    wxColorStr      = wxString::FromUTF8(color.c_str());
        wxString    wxExtruderIdStr = wxString::FromUTF8(extruder_id_str.c_str());
        wxColour    wxColor;
        wxColor.Set(wxColorStr);

        wxPanel* panel = new wxPanel(m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxSize(FromDIP(40), FromDIP(40)));
        panel->SetBackgroundColour(wxColor);

        wxFont font(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);

        wxStaticText* textTop = new wxStaticText(panel, wxID_ANY, wxExtruderIdStr, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
        textTop->SetFont(font);
        textTop->SetForegroundColour(*wxBLACK);
        wxStaticText* textBottom = new wxStaticText(panel, wxID_ANY, wxType, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
        textBottom->SetForegroundColour(*wxBLACK);
        textBottom->SetFont(font);
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(textTop, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);
        sizer->Add(textBottom, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);
        panel->SetSizer(sizer);
        m_sizer_material_list->Add(panel, 0, wxEXPAND | wxALL, 10);
    }

    m_sizer_material->Add(m_sizer_material_list);

    m_sizer_material->Layout();
};

void Upload3mfToCloudDialog::update_3mf_info()
{
    // auto print_config = wxGetApp().preset_bundle->prints.get_selected_preset();
    auto print_config = wxGetApp().preset_bundle->prints.get_edited_preset();
    wxGetApp().preset_bundle->prints.update_after_user_presets_loaded();
    PresetCollection* presetCollect = &wxGetApp().preset_bundle->printers;
    std::string selectPreset  = presetCollect->get_edited_preset().name;
    wxString wxStrPreset = wxString(selectPreset.c_str(), wxConvUTF8);
    if (wxStrPreset.length() > 35) {
        wxStrPreset = wxStrPreset.Mid(0, 35);
        wxStrPreset.Append("...");
    }

    m_stext_printer->SetLabel(wxStrPreset);
    double              layer_height          = print_config.config.opt_float("layer_height");
    auto                wall_loops            = print_config.config.opt_int("wall_loops");
    DynamicPrintConfig* config                = &wxGetApp().preset_bundle->prints.get_edited_preset().config;
    auto                sparse_infill_density = static_cast<int>(config->option<ConfigOptionPercent>("sparse_infill_density")->value);

    std::stringstream ss;
    std::string       formatted_layer_height;
    ss << std::fixed << std::setprecision(2) << layer_height;
    formatted_layer_height = ss.str();
    std::string name       = print_config.name;
    wxString wxstrName  = wxString(name.c_str(), wxConvUTF8);
    if (wxstrName.length() > 55) {
        wxstrName = wxstrName.Mid(0,32);
        wxstrName.Append("...");
    }
    std::string label = "(" + formatted_layer_height + "mm layer, " + std::to_string(wall_loops) + " walls," +
                        std::to_string(sparse_infill_density) + "% infill)";
    wxstrName.Append(from_u8(label));
    m_stext_process->SetLabel(wxstrName);

    auto modelDownload = wxGetApp().get_cloud_model_download();
    auto isOneImport   = modelDownload.size() == 1;
    m_radio_upload_exit->SetValue(isOneImport);
    m_radio_upload_exit->Enable(isOneImport);
    m_radio_upload_new->SetValue(!isOneImport);
    if (wxGetApp().plater()) {
    
        int plate_count = wxGetApp().plater()->get_partplate_list().get_plate_count();
        m_stext_plate_count->SetLabel(std::to_string(plate_count));
        auto plate_slice_result = wxGetApp().plater()->get_partplate_list().get_nonempty_plate_list();
        int  slicedCount        = 0;
        for (auto item : plate_slice_result) {
            if (item->is_slice_result_valid())
                slicedCount++;
        }
        m_stext_plate_sliced_count->SetLabel(std::to_string(slicedCount));
    }
    // Layout();
    // Fit();
    // Thaw();
}
std::string Upload3mfToCloudDialog::build_match_color_cmd_info()
{
    std::string ipAddress = get_selected_printer_ip();
    if (ipAddress.empty()) {
        return "";
    }

    PartPlate* plate = wxGetApp().plater()->get_partplate_list().get_plate(m_print_plate_idx);
    if (!plate)
        return "";

    std::vector<std::string> all_extruder_colors = Slic3r::GUI::wxGetApp().plater()->get_extruder_colors_from_plater_config();

    nlohmann::json   plate_extruder_colors_json = nlohmann::json::array();
    std::vector<int> plate_extruders            = plate->get_extruders(true);
    if (plate_extruders.size() > 0) {
        for (const auto& extruder : plate_extruders) {
            if (all_extruder_colors.size() > (extruder - 1)) {
                nlohmann::json extruder_info = {{"extruder_id", extruder}, {"extruder_color", all_extruder_colors[extruder - 1]}};
                plate_extruder_colors_json.push_back(extruder_info);
            }
        }
    }

    // Create top-level JSON object
    nlohmann::json top_level_json = {{"printer_ip", ipAddress}, {"plate_extruder_colors", plate_extruder_colors_json}};

    // Create command JSON object
    nlohmann::json commandJson = {{"command", "req_match_color_info"}, {"data", top_level_json.dump()}};

    // Encode the command string
    return RemotePrint::Utils::url_encode(commandJson.dump());
}

std::string Upload3mfToCloudDialog::build_print_cmd_info()
{
    std::string ipAddress = get_selected_printer_ip();
    if (ipAddress.empty()) {
        return "";
    }

    PartPlate* plate = wxGetApp().plater()->get_partplate_list().get_plate(m_print_plate_idx);
    if (!plate)
        return "";

    std::vector<int> plate_extruders = plate->get_extruders(true);

    std::vector<RemotePrint::MaterialMapPanel::ColorMatchInfo> colorMatchInfo;
    if (m_materialMapPanel) {
        colorMatchInfo = m_materialMapPanel->GetColorMatchInfo();
    }

    nlohmann::json top_level_json;
    top_level_json["printer_ip"]        = ipAddress;
    top_level_json["print_calibration"] = m_checkbox_calibration->IsChecked() ? 1 : 0; // 0: not calibrated, 1: calibrated
    top_level_json["open_cfs"]          = m_checkbox_open_cfs->IsChecked() ? 1 : 0;    // 0: not open, 1: open

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
    } else {
        top_level_json["color_match_info"] = "";
    }

    wxString renameText                 = m_rename_text->GetLabelText();
    wxString uploadName                 = renameText + ".gcode";
    top_level_json["upload_gcode_name"] = uploadName.ToUTF8().data();

    std::string json_str = top_level_json.dump();

    // create command to send to the webview
    nlohmann::json commandJson;
    commandJson["command"] = "send_print_cmd";
    commandJson["data"]    = top_level_json.dump();

    return RemotePrint::Utils::url_encode(commandJson.dump());
}

std::string Upload3mfToCloudDialog::build_switch_printer_details_page_cmd_info()
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
    nlohmann::json commandJson = {{"command", "switch_printer_details_page"}, {"data", top_level_json.dump()}};

    // Encode the command string
    return RemotePrint::Utils::url_encode(commandJson.dump());
}

void Upload3mfToCloudDialog::send_gcode(bool start_print)
{
    wxString cur_combo_selection = m_comboBox_printer->GetValue();
    if (cur_combo_selection.IsEmpty()) {
        wxMessageBox("Please select a printer.", "Error", wxOK | wxICON_ERROR);
        return;
    }

    wxString ipAddress = extract_ip_address(cur_combo_selection);
    if (ipAddress.IsEmpty()) {
        wxMessageBox("Invalid printer selection.", "Error", wxOK | wxICON_ERROR);
        return;
    }

    PartPlate* plate = wxGetApp().plater()->get_partplate_list().get_plate(m_print_plate_idx);
    if (!plate)
        return;

    std::string gcodeFilePath = plate->get_tmp_gcode_path();
    // wxString uploadName = m_rename_text->GetLabelText() + ".gcode";

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
            if (0 == statusCode && start_print) {
                wxGetApp().mainframe->get_printer_mgr_view()->ExecuteScriptCommand(build_print_cmd_info(), true);
            }
        });
}

void Upload3mfToCloudDialog::on_upload_3mf(wxCommandEvent& event)
{
    auto    modelDownload = wxGetApp().get_cloud_model_download();
    Plater* plater        = wxGetApp().mainframe->plater();
    int returnStatus =   plater->save_project(true);
    if (returnStatus == wxID_CANCEL)
        return;
    std::string version  = std::string(PROJECT_VERSION_EXTRA);
    bool        is_alpha = boost::algorithm::icontains(version, "alpha");
    bool        is_beta  = boost::algorithm::icontains(version, "beta");
    std::string domain   = "https://www.crealitycloud.com";
     //if (is_beta) domain = "https://dev.crealitycloud.com";
     if (is_alpha) domain = "https://pre.crealitycloud.com";
    if (const std::string country_code = wxGetApp().app_config->get_country_code(); country_code == "CN") {
        size_t pos = domain.find("com");
        if (pos != std::string::npos)
            domain.replace(pos, 3, "cn");
    }
    if (m_radio_upload_exit->GetValue()) {
        auto modelGroupId = modelDownload.front();
        wxLaunchDefaultBrowser(domain + "/create-model?type=addFile3mf&modelId=" + modelGroupId + "&source=1");
    } else if (m_radio_upload_new->GetValue()) {
        wxLaunchDefaultBrowser(domain + "/create-model?source=12");
    }

    // send_gcode(false);
}

void Upload3mfToCloudDialog::send_and_print(wxCommandEvent& event) { this->EndModal(wxID_CANCEL);}

void Upload3mfToCloudDialog::clear_ip_address_config(wxCommandEvent& e) { enable_prepare_mode = true; }

void Upload3mfToCloudDialog::update_user_machine_list()
{
    NetworkAgent* m_agent = wxGetApp().getAgent();
    if (m_agent && m_agent->is_user_login()) {
        boost::thread get_print_info_thread = Slic3r::create_thread([this, token = std::weak_ptr<int>(m_token)] {
            NetworkAgent* agent = wxGetApp().getAgent();
            unsigned int  http_code;
            std::string   body;
            int           result = agent->get_user_print_info(&http_code, &body);
            CallAfter([token, this, result, body] {
                if (token.expired()) {
                    return;
                }
                if (result == 0) {
                    m_print_info = body;
                } else {
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

void Upload3mfToCloudDialog::on_details(wxCommandEvent& event)
{
    std::string ipAddress = get_selected_printer_ip();
    if (ipAddress.empty()) {
        return;
    }

    wxCommandEvent e = wxCommandEvent(wxCUSTOMEVT_NOTEBOOK_SEL_CHANGED);
    e.SetId(9); // printer details page
    wxPostEvent(wxGetApp().mainframe->topbar(), e);

    wxGetApp().mainframe->get_printer_mgr_view()->ExecuteScriptCommand(build_switch_printer_details_page_cmd_info());
}

void Upload3mfToCloudDialog::on_left(wxMouseEvent& event)
{
    int plate_count = wxGetApp().plater()->get_partplate_list().get_plate_count();
    m_print_plate_idx--;
    if (m_print_plate_idx < 0) {
        m_print_plate_idx = plate_count - 1;
    }

    PartPlate* selected_plate = wxGetApp().plater()->get_partplate_list().get_plate(m_print_plate_idx);
    if (!selected_plate)
        return;

    ThumbnailData& data = selected_plate->thumbnail_data;
    if (!data.is_valid())
        return;

    set_plate_info(m_print_plate_idx);
}

void Upload3mfToCloudDialog::on_right(wxMouseEvent& event)
{
    int plate_count = wxGetApp().plater()->get_partplate_list().get_plate_count();
    m_print_plate_idx++;
    if (m_print_plate_idx >= plate_count) {
        m_print_plate_idx = 0;
    }

    PartPlate* selected_plate = wxGetApp().plater()->get_partplate_list().get_plate(m_print_plate_idx);
    if (!selected_plate)
        return;

    ThumbnailData& data = selected_plate->thumbnail_data;
    if (!data.is_valid())
        return;

    set_plate_info(m_print_plate_idx);
}

void Upload3mfToCloudDialog::on_print_job_cancel(wxCommandEvent& evt)
{
    BOOST_LOG_TRIVIAL(info) << "print_job: canceled";
    show_status(PrintDialogStatus::PrintStatusSendingCanceled);
}

std::vector<std::string> Upload3mfToCloudDialog::sort_string(std::vector<std::string> strArray)
{
    std::vector<std::string> outputArray;
    std::sort(strArray.begin(), strArray.end());
    std::vector<std::string>::iterator st;
    for (st = strArray.begin(); st != strArray.end(); st++) {
        outputArray.push_back(*st);
    }

    return outputArray;
}

bool Upload3mfToCloudDialog::is_timeout()
{
    if (timeout_count > 15 * 1000 / LIST_REFRESH_INTERVAL) {
        return true;
    }
    return false;
}

void Upload3mfToCloudDialog::reset_timeout() { timeout_count = 0; }

void Upload3mfToCloudDialog::on_timer(wxTimerEvent& event)
{
    wxGetApp().reset_to_active();
    update_show_status();
}

void Upload3mfToCloudDialog::on_selection_changed(wxCommandEvent& event)
{
    std::string ipAddress = get_selected_printer_ip();
    if (ipAddress.empty()) {
        return;
    }

    int plate_count = wxGetApp().plater()->get_partplate_list().get_plate_count();
    if (plate_count > 1) {
        m_button_left->Show();
        m_button_right->Show();
    } else {
        m_button_left->Hide();
        m_button_right->Hide();
    }

    if (m_progress_bar) {
        m_progress_bar->SetValue(0);
        m_progress_bar->Hide();
    }

    if (m_materialMapPanel) {
        m_materialMapPanel->related_printer_ip = ipAddress;
    }

    if (RemotePrint::DeviceDB::getInstance().DeviceHasBoxColor(ipAddress)) {
        wxGetApp().mainframe->get_printer_mgr_view()->ExecuteScriptCommand(build_match_color_cmd_info());
    } else {
        if (m_materialMapPanel) {
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
            // m_button_send_print_cmd->Disable();
            m_label_error_info->SetLabelText(_L("Printer is offline"));
        } else {
            m_button_ensure->Enable();
            // m_button_send_print_cmd->Enable(0 == device.deviceState);
            if (device.deviceState != 0) {
                m_label_error_info->SetLabelText(_L("Printer is busy, can send gcode only"));
            } else {
                m_label_error_info->SetLabelText(_L(""));
            }
        }
    }

    Layout();
    Fit();
}

void Upload3mfToCloudDialog::update_show_status()
{
    NetworkAgent*  agent = Slic3r::GUI::wxGetApp().getAgent();
    DeviceManager* dev   = Slic3r::GUI::wxGetApp().getDeviceManager();
    if (!agent)
        return;
    if (!dev)
        return;
    MachineObject* obj_ = dev->get_my_machine(m_printer_last_select);
    if (!obj_) {
        if (agent) {
            if (agent->is_user_login()) {
                show_status(PrintDialogStatus::PrintStatusInvalidPrinter);
            } else {
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
        } else {
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
    } else if (obj_->is_in_upgrading()) {
        show_status(PrintDialogStatus::PrintStatusInUpgrading);
        return;
    } else if (obj_->is_system_printing()) {
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

bool Upload3mfToCloudDialog::is_blocking_printing(MachineObject* obj_)
{
    DeviceManager* dev = Slic3r::GUI::wxGetApp().getDeviceManager();
    if (!dev)
        return true;

    PresetBundle* preset_bundle = wxGetApp().preset_bundle;
    auto          source_model  = preset_bundle->printers.get_edited_preset().get_printer_type(preset_bundle);
    auto          target_model  = obj_->printer_type;

    if (source_model != target_model) {
        std::vector<std::string>      compatible_machine = dev->get_compatible_machine(target_model);
        vector<std::string>::iterator it                 = find(compatible_machine.begin(), compatible_machine.end(), source_model);
        if (it == compatible_machine.end()) {
            return true;
        }
    }

    return false;
}

void Upload3mfToCloudDialog::Enable_Refresh_Button(bool en)
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

void Upload3mfToCloudDialog::show_status(PrintDialogStatus status, std::vector<wxString> params)
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
    } else if (status == PrintDialogStatus::PrintStatusNoUserLogin) {
        wxString msg_text = _L("No login account, only printers in LAN mode are displayed");
        update_print_status_msg(msg_text, false, true);
        Enable_Send_Button(false);
        Enable_Refresh_Button(true);
    } else if (status == PrintDialogStatus::PrintStatusInvalidPrinter) {
        update_print_status_msg(wxEmptyString, true, true);
        Enable_Send_Button(false);
        Enable_Refresh_Button(true);
    } else if (status == PrintDialogStatus::PrintStatusConnectingServer) {
        wxString msg_text = _L("Connecting to server");
        update_print_status_msg(msg_text, true, true);
        Enable_Send_Button(true);
        Enable_Refresh_Button(true);
    } else if (status == PrintDialogStatus::PrintStatusReading) {
        wxString msg_text = _L("Synchronizing device information");
        update_print_status_msg(msg_text, false, true);
        Enable_Send_Button(false);
        Enable_Refresh_Button(true);
    } else if (status == PrintDialogStatus::PrintStatusReadingFinished) {
        update_print_status_msg(wxEmptyString, false, true);
        Enable_Send_Button(true);
        Enable_Refresh_Button(true);
    } else if (status == PrintDialogStatus::PrintStatusReadingTimeout) {
        wxString msg_text = _L("Synchronizing device information time out");
        update_print_status_msg(msg_text, true, true);
        Enable_Send_Button(true);
        Enable_Refresh_Button(true);
    } else if (status == PrintDialogStatus::PrintStatusInUpgrading) {
        wxString msg_text = _L("Cannot send the print task when the upgrade is in progress");
        update_print_status_msg(msg_text, true, true);
        Enable_Send_Button(false);
        Enable_Refresh_Button(true);
    } else if (status == PrintDialogStatus::PrintStatusUnsupportedPrinter) {
        wxString msg_text = _L("The selected printer is incompatible with the chosen printer presets.");
        update_print_status_msg(msg_text, true, true);
        Enable_Send_Button(false);
        Enable_Refresh_Button(true);
    } else if (status == PrintDialogStatus::PrintStatusRefreshingMachineList) {
        update_print_status_msg(wxEmptyString, false, true);
        Enable_Send_Button(false);
        Enable_Refresh_Button(false);
    } else if (status == PrintDialogStatus::PrintStatusSending) {
        Enable_Send_Button(false);
        Enable_Refresh_Button(false);
    } else if (status == PrintDialogStatus::PrintStatusSendingCanceled) {
        Enable_Send_Button(true);
        Enable_Refresh_Button(true);
    } else if (status == PrintDialogStatus::PrintStatusNoSdcard) {
        wxString msg_text = _L("An SD card needs to be inserted before send to printer SD card.");
        update_print_status_msg(msg_text, true, true);
        Enable_Send_Button(false);
        Enable_Refresh_Button(true);
    } else if (status == PrintDialogStatus::PrintStatusNotOnTheSameLAN) {
        wxString msg_text = _L("The printer is required to be in the same LAN as Creality Print");
        update_print_status_msg(msg_text, true, true);
        Enable_Send_Button(false);
        Enable_Refresh_Button(true);
    } else if (status == PrintDialogStatus::PrintStatusNotSupportedSendToSDCard) {
        wxString msg_text = _L("The printer does not support sending to printer SD card.");
        update_print_status_msg(msg_text, true, true);
        Enable_Send_Button(false);
        Enable_Refresh_Button(true);
    } else {
        Enable_Send_Button(true);
        Enable_Refresh_Button(true);
    }
}

void Upload3mfToCloudDialog::Enable_Send_Button(bool en)
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

void Upload3mfToCloudDialog::on_dpi_changed(const wxRect& suggested_rect)
{
    if (m_button_refresh) {
    m_button_refresh->SetMinSize(SELECT_MACHINE_DIALOG_BUTTON_SIZE);
    m_button_refresh->SetCornerRadius(FromDIP(12));
    }
    m_button_ensure->SetMinSize(SELECT_MACHINE_DIALOG_BUTTON_SIZE);
    m_button_ensure->SetCornerRadius(FromDIP(12));
    if (m_status_bar)
        m_status_bar->msw_rescale();
    Fit();
    Refresh();
}

std::vector<std::string> Upload3mfToCloudDialog::get_all_filament_types()
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

void Upload3mfToCloudDialog::set_plate_info(int plate_idx)
{
    // project name
    m_rename_switch_panel->SetSelection(0);
    Slic3r::GUI::wxGetApp().plater()->update_all_plate_thumbnails(true);
    PartPlate* plate = wxGetApp().plater()->get_partplate_list().get_plate(plate_idx);
    if (!plate)
        return;
    wxString default_gcode_name = "";

    ModelObjectPtrs plate_objects = plate->get_objects_on_this_plate();
    wxString        obj0_name     = ""; // the first object's name
    if (plate_objects.size() > 0 && nullptr != plate_objects[0]) {
        obj0_name = wxString::FromUTF8(plate_objects[0]->name);
    }

    auto                                  plate_print_statistics = plate->get_slice_result()->print_statistics;
    const PrintEstimatedStatistics::Mode& plate_time_mode        = plate_print_statistics
                                                                .modes[static_cast<size_t>(PrintEstimatedStatistics::ETimeMode::Normal)];

    std::vector<int>         plate_extruders = plate->get_extruders(true);
    std::vector<std::string> filament_types  = get_all_filament_types();
    /*   if (plate_extruders.size() > 0) {
           default_gcode_name = obj0_name + "_" + filament_types[plate_extruders[0] - 1] + "_" + get_bbl_time_dhms(plate_time_mode.time);
       }*/
    wxString project_name = GUI::wxGetApp().mainframe->plater()->get_project_name();
    if (plate_extruders.size() > 0) {
        default_gcode_name = project_name;
     //   +"_" + filament_types[plate_extruders[0] - 1] + "_" + get_bbl_time_dhms(plate_time_mode.time);
    }
    m_rename_text->SetLabelText(default_gcode_name);
    m_current_project_name = default_gcode_name;

    m_rename_normal_panel->Layout();

    // init_device_list_from_db();

    // clear combobox
    m_list.clear();
    // m_comboBox_printer->Clear();
    m_printer_last_select = "";
    m_print_info          = "";
    // m_comboBox_printer->SetValue(wxEmptyString);
    // m_comboBox_printer->Enable();

    // thumbmail
    // wxBitmap bitmap;
    ThumbnailData& data = plate->thumbnail_data;
    if (data.is_valid()) {
        wxImage image(data.width, data.height);
        image.InitAlpha();
        for (unsigned int r = 0; r < data.height; ++r) {
            unsigned int rr = (data.height - 1 - r) * data.width;
            for (unsigned int c = 0; c < data.width; ++c) {
                unsigned char* px = (unsigned char*) data.pixels.data() + 4 * (rr + c);
                image.SetRGB((int) c, (int) r, px[0], px[1], px[2]);
                image.SetAlpha((int) c, (int) r, px[3]);
            }
        }
        image = image.Rescale(FromDIP(300), FromDIP(300));
        // m_thumbnailPanel->set_thumbnail(image);
    }

    /*  m_scrollable_region->Layout();
      m_scrollable_region->Fit();*/
    Layout();
    Fit();

    // basic info
    auto     aprint_stats = wxGetApp().plater()->get_partplate_list().get_current_fff_print().print_statistics();
    wxString time;

    if (plate) {
        if (plate->get_slice_result()) {
            time = wxString::Format("%s", short_time(get_time_dhms(plate->get_slice_result()->print_statistics.modes[0].time)));
        }
    }

    char weight[64];
    if (wxGetApp().app_config->get("use_inches") == "1") {
        ::sprintf(weight, "  %.2f oz", aprint_stats.total_weight * 0.035274);
    } else {
        ::sprintf(weight, "  %.2f g", aprint_stats.total_weight);
    }

    /*  m_stext_time->SetLabel(time);
      m_stext_weight->SetLabel(weight);*/
}

bool Upload3mfToCloudDialog::Show(bool show)
{
    // show_status(PrintDialogStatus::PrintStatusInit);

    // set default value when show this dialog
    if (show) {
        wxGetApp().reset_to_active();
        if (wxGetApp().plater()) {
            set_plate_info(wxGetApp().plater()->get_partplate_list().get_curr_plate_index());
        }
        update_user_machine_list();
        get_current_plate_color();
        update_3mf_info();
    }

    Layout();
    Fit();
    if (show) {
        CenterOnParent();
    }
    return DPIDialog::Show(show);
}

void Upload3mfToCloudDialog::handle_receive_device_list(const nlohmann::json& json_data)
{
    RemotePrint::DeviceDB::getInstance().devices.clear();

    // handle receive_device_list command
    for (const auto& device : json_data["data"]) {
        RemotePrint::DeviceDB::Data data;
        data.address     = device["address"];
        data.mac         = device["mac"];
        data.model       = device["model"];
        data.online      = device["online"];
        data.deviceState = device["deviceState"];
        data.name        = device["name"];
        data.group       = device["group"];

        // parse boxColorInfo
        for (const auto& boxColorInfo : device["boxColorInfo"]) {
            RemotePrint::DeviceDB::DeviceBoxColorInfo boxColor;
            boxColor.color        = boxColorInfo["color"];
            boxColor.boxId        = boxColorInfo["boxId"];
            boxColor.materialId   = boxColorInfo["materialId"];
            boxColor.filamentType = boxColorInfo["filamentType"];
            data.boxColorInfos.push_back(boxColor);
        }

        RemotePrint::DeviceDB::getInstance().AddDevice(data);
    }

    // this->Show(true);

    set_plate_info(m_print_plate_idx);
}

void Upload3mfToCloudDialog::handle_receive_color_match_info(const nlohmann::json& json_data)
{
    const auto&                                                data = json_data["data"];
    std::vector<RemotePrint::MaterialMapPanel::ColorMatchInfo> materialMapInfo;

    std::vector<std::string> all_filament_types = get_all_filament_types();

    for (const auto& matchInfo : data) {
        RemotePrint::MaterialMapPanel::ColorMatchInfo tmpMatchInfo;
        tmpMatchInfo.extruderId           = matchInfo["extruderId"];
        tmpMatchInfo.extruderFilamentType = all_filament_types[tmpMatchInfo.extruderId - 1];
        tmpMatchInfo.extruderColor        = RemotePrint::Utils::hex_string_to_wxcolour(matchInfo["extruderColor"].get<std::string>());
        tmpMatchInfo.matchColor           = RemotePrint::Utils::hex_string_to_wxcolour(matchInfo["matchColor"].get<std::string>());
        tmpMatchInfo.matchFilamentType    = matchInfo["matchFilamentType"];
        tmpMatchInfo.materialId           = matchInfo["materialId"];
        materialMapInfo.emplace_back(tmpMatchInfo);
    }

    if (m_materialMapPanel) {
        m_materialMapPanel->SetMaterialMapInfo(materialMapInfo);
    }

    Layout();
    Fit();
    Thaw();
}

void Upload3mfToCloudDialog::on_checkbox_open_cfs(wxCommandEvent& event)
{
    if (event.IsChecked()) {
        if (m_materialMapPanel) {
            m_materialMapPanel->Show();
        }
    } else {
        if (m_materialMapPanel) {
            m_materialMapPanel->Hide();
        }
    }

    Layout();
    Fit();
    Thaw();
}

Upload3mfToCloudDialog::~Upload3mfToCloudDialog() {
    //  wxGetApp().mainframe->get_printer_mgr_view()->UnregisterHandler("receive_color_match_info");
    //  wxGetApp().mainframe->get_printer_mgr_view()->UnregisterHandler("receive_device_list");
     delete m_refresh_timer; 
     }

}} // namespace Slic3r::GUI
