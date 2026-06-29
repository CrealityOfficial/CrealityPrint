#include "calib_dlg.hpp"
#include "GUI_App.hpp"
#include "MsgDialog.hpp"
#include "I18N.hpp"
#include "Widgets/HoverBorderIcon.hpp"
#include "libslic3r/Utils.hpp"
#include <wx/dcgraph.h>
#include <wx/filefn.h>
#include "MainFrame.hpp"
#include <string>
namespace Slic3r { namespace GUI {
    
wxBoxSizer* create_item_checkbox(wxString title, wxWindow* parent, bool* value, CheckBox*& checkbox)
{
    wxBoxSizer* m_sizer_checkbox = new wxBoxSizer(wxHORIZONTAL);

    m_sizer_checkbox->Add(0, 0, 0, wxEXPAND | wxLEFT, 5);

    checkbox = new ::CheckBox(parent);
    m_sizer_checkbox->Add(checkbox, 0, wxALIGN_CENTER, 0);
    m_sizer_checkbox->Add(0, 0, 0, wxEXPAND | wxLEFT, 8);

    auto checkbox_title = new wxStaticText(parent, wxID_ANY, title, wxDefaultPosition, wxSize(-1, -1), 0);
    checkbox_title->SetForegroundColour(wxColour(144, 144, 144));
    checkbox_title->SetFont(::Label::Body_13);
    checkbox_title->Wrap(-1);
    m_sizer_checkbox->Add(checkbox_title, 0, wxALIGN_CENTER | wxALL, 3);

    checkbox->SetValue(true);

    checkbox->Bind(wxEVT_TOGGLEBUTTON, [parent, checkbox, value](wxCommandEvent& e) {
        (*value) = (*value) ? false : true;
        e.Skip();
        });

    return m_sizer_checkbox;
}

PA_Calibration_Dlg::PA_Calibration_Dlg(wxWindow* parent, wxWindowID id, Plater* plater)
    : DPIDialog(parent, id, _L("PA Calibration"), wxDefaultPosition, parent->FromDIP(wxSize(-1, 280)), wxDEFAULT_DIALOG_STYLE), m_plater(plater)
{
    iExtruderTypeSeletion = 0;
    imethod               = 0;
    SetMaxSize(wxSize(FromDIP(612), FromDIP(320)));
    SetFont(wxGetApp().normal_font());
    SetBackgroundColour(*wxWHITE);
    wxBoxSizer* v_sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(v_sizer);
	wxBoxSizer* choice_sizer = new wxBoxSizer(wxHORIZONTAL);

    bool        is_dark      = wxGetApp().dark_mode();

    // 定义颜色：白色文字 + 深灰色背景
    wxColour textColor = is_dark ?  wxColour(0, 0, 0) : wxColour(51, 51, 51); // 白色
    wxColour bgColor(75, 75, 77); // 深灰色（#404040）
     // ---------------------- 左侧：挤出机类型 ----------------------
    extruderBox                     = new wxStaticBox(this, wxID_ANY, _L("Extruder type"));
    wxStaticBoxSizer* extruderSizer = new wxStaticBoxSizer(extruderBox, wxHORIZONTAL);
    // 单选按钮：近程挤出机（设为组首，wxRB_GROUP）
    m_radioNear = new wxRadioButton(extruderBox, wxID_ANY, _L("DDE"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    // 单选按钮：远程挤出机
    m_radioFar = new wxRadioButton(extruderBox, wxID_ANY, _L("Bowden"));
    // 添加到布局
    extruderSizer->Add(m_radioNear, 0, wxALL, 5);
    extruderSizer->Add(m_radioFar, 0, wxALL, 5);
    // 设置颜色（静态框标题 + 单选按钮）
    extruderBox->SetForegroundColour(textColor); // 标题文字颜色
    extruderBox->SetBackgroundColour(bgColor);   // 静态框背景
    m_radioNear->SetForegroundColour(textColor); // 按钮文字颜色
    m_radioNear->SetBackgroundColour(bgColor);   // 按钮背景
    m_radioFar->SetForegroundColour(textColor);
    m_radioFar->SetBackgroundColour(bgColor);       
    choice_sizer->Add(extruderSizer, 0, wxALL, 5);

    // ---------------------- 右侧：方法 ----------------------
    methodBox                     = new wxStaticBox(this, wxID_ANY, _L("Method"));
    wxStaticBoxSizer* methodSizer = new wxStaticBoxSizer(methodBox, wxHORIZONTAL);
    // 单选按钮：PA塔（组首
    m_radioPA = new wxRadioButton(methodBox, wxID_ANY, _L("PA Tower"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    // 单选按钮：划线模式
    m_radioLine = new wxRadioButton(methodBox, wxID_ANY, _L("PA Line"));
    // 单选按钮：V形模式
    m_radioV = new wxRadioButton(methodBox, wxID_ANY, _L("PA Pattern"));
    // 添加到布局
    methodSizer->Add(m_radioPA, 0, wxALL, 5);
    methodSizer->Add(m_radioLine, 0, wxALL, 5);
    methodSizer->Add(m_radioV, 0, wxALL, 5);
    // 设置颜色
    methodBox->SetForegroundColour(textColor);
    methodBox->SetBackgroundColour(bgColor);
    m_radioPA->SetForegroundColour(textColor);
    m_radioPA->SetBackgroundColour(bgColor);
    m_radioLine->SetForegroundColour(textColor);
    m_radioLine->SetBackgroundColour(bgColor);
    m_radioV->SetForegroundColour(textColor);
    m_radioV->SetBackgroundColour(bgColor);  
    choice_sizer->Add(methodSizer, 0, wxALL, 5);
    v_sizer->Add(choice_sizer);

    // Settings
    wxString start_pa_str = _L("Start PA: ");
    wxString end_pa_str = _L("End PA: ");
    wxString PA_step_str = _L("PA step: ");
    wxString bed_temp_str = _L("Hot Plate Template: ");
	auto text_size = wxWindow::GetTextExtent(start_pa_str);
	text_size.IncTo(wxWindow::GetTextExtent(end_pa_str));
	text_size.IncTo(wxWindow::GetTextExtent(PA_step_str));
    text_size.IncTo(wxWindow::GetTextExtent(bed_temp_str));
	text_size.x = text_size.x;
	wxStaticBoxSizer* settings_sizer = new wxStaticBoxSizer(wxVERTICAL, this, _L("Settings"));

	auto st_size = wxSize(FromDIP(430),-1);
	auto ti_size = wxSize(FromDIP(100),-1);
    // start PA
    auto start_PA_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto start_pa_text = new wxStaticText(this, wxID_ANY, start_pa_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //start_pa_text->SetOwnForegroundColour(is_dark ? wxColour(0, 0, 0) : wxColour(51, 51, 51));
    start_pa_text->SetBackgroundColour(is_dark ? wxColour(75, 75, 77) : wxColour(255, 255, 255));
    start_pa_text->SetForegroundColour(is_dark ? wxColour(0, 0, 0) : wxColour(51, 51, 51));
    m_tiStartPA = new TextInput(this, "", "", "", wxDefaultPosition, ti_size, wxTE_CENTRE | wxTE_PROCESS_ENTER|wxALIGN_RIGHT);
    m_tiStartPA->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));

	start_PA_sizer->Add(start_pa_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    start_PA_sizer->Add(m_tiStartPA, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(start_PA_sizer);

    // end PA
    auto end_PA_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto end_pa_text = new wxStaticText(this, wxID_ANY, end_pa_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //end_pa_text->SetOwnForegroundColour(is_dark ? wxColour(0, 0, 0) : wxColour(51, 51, 51));
    end_pa_text->SetBackgroundColour(is_dark ? wxColour(75, 75, 77) : wxColour(255, 255, 255));
    end_pa_text->SetForegroundColour(is_dark ? wxColour(0, 0, 0) : wxColour(51, 51, 51));
    m_tiEndPA = new TextInput(this, "", "", "", wxDefaultPosition, ti_size, wxTE_CENTRE | wxTE_PROCESS_ENTER);
    m_tiStartPA->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    end_PA_sizer->Add(end_pa_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    end_PA_sizer->Add(m_tiEndPA, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(end_PA_sizer);

    // PA step
    auto PA_step_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto PA_step_text = new wxStaticText(this, wxID_ANY, PA_step_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //PA_step_text->SetOwnForegroundColour(is_dark ? wxColour(0, 0, 0) : wxColour(51, 51, 51));
    PA_step_text->SetBackgroundColour(is_dark ? wxColour(75, 75, 77) : wxColour(255, 255, 255));
    PA_step_text->SetForegroundColour(is_dark ? wxColour(0, 0, 0) : wxColour(51, 51, 51));
    m_tiPAStep = new TextInput(this, "", "", "", wxDefaultPosition, ti_size, wxTE_CENTRE | wxTE_PROCESS_ENTER);
    m_tiStartPA->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    PA_step_sizer->Add(PA_step_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    PA_step_sizer->Add(m_tiPAStep, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(PA_step_sizer);
    
    // creality add bed temp
    auto bed_temp_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto bed_temp_text  = new wxStaticText(this, wxID_ANY, bed_temp_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //bed_temp_text->SetOwnForegroundColour(is_dark ? wxColour(0, 0, 0) : wxColour(51, 51, 51));
    bed_temp_text->SetBackgroundColour(is_dark ? wxColour(75, 75, 77) : wxColour(255, 255, 255));
    bed_temp_text->SetForegroundColour(is_dark ? wxColour(0, 0, 0) : wxColour(51, 51, 51));
    m_tiBedTemp = new TextInput(this, wxString::FromDouble(50), _L("℃"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStartPA->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    bed_temp_sizer->Add(bed_temp_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    bed_temp_sizer->Add(m_tiBedTemp, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(bed_temp_sizer);

	//settings_sizer->Add(create_item_checkbox(_L("Print numbers"), this, &m_params.print_numbers, m_cbPrintNum));
    //m_cbPrintNum->SetValue(false);
    m_cbPrintNum = new CheckBox(this);
    m_cbPrintNum->Hide();

    v_sizer->Add(settings_sizer, 0, wxTOP | wxLEFT | wxRIGHT, 10);
	v_sizer->Add(FromDIP(10), FromDIP(10), 0, wxEXPAND, 5);
    m_btnStart = new Button(this, _L("OK"));
    StateColor btn_bg_gray(std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(150, 150, 155), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Normal));

    StateColor btn_bg_light_mode(std::pair<wxColour, int>(wxColour(246, 246, 249), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(246, 246, 249), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Normal));
    m_btnStart->SetBackgroundColor(is_dark ? btn_bg_gray : btn_bg_light_mode);
    m_btnStart->SetBorderColor(is_dark ? wxColour(0, 0, 0) : wxColour(214, 214, 220));
	m_btnStart->SetTextColor(is_dark ? wxColour(38, 46, 48) : wxColour(51, 51, 51));
	m_btnStart->SetSize(wxSize(FromDIP(100), FromDIP(30)));
    m_btnStart->SetMinSize(wxSize(FromDIP(100), FromDIP(30)));
	m_btnStart->SetCornerRadius(FromDIP(12));
	m_btnStart->Bind(wxEVT_BUTTON, &PA_Calibration_Dlg::on_start, this);
	v_sizer->Add(m_btnStart, 0, wxALL | wxALIGN_CENTER, FromDIP(5));
    PA_Calibration_Dlg::reset_params();

    // Connect Events
    extruderBox->Connect(wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler(PA_Calibration_Dlg::on_extruder_type_changed), NULL, this);
    methodBox->Connect(wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler(PA_Calibration_Dlg::on_method_changed), NULL, this);
    this->Connect(wxEVT_SHOW, wxShowEventHandler(PA_Calibration_Dlg::on_show));
    wxGetApp().UpdateDlgDarkUI(this);

    Layout();
    Fit();
}

PA_Calibration_Dlg::~PA_Calibration_Dlg() {
    // Disconnect Events
    extruderBox->Disconnect(wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler(PA_Calibration_Dlg::on_extruder_type_changed), NULL,
                            this);
    methodBox->Disconnect(wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler(PA_Calibration_Dlg::on_method_changed), NULL, this);
    m_btnStart->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(PA_Calibration_Dlg::on_start), NULL, this);
}

// 获取选中状态的函数
int PA_Calibration_Dlg::GetSelectedMethod()
{
    if (m_radioPA->GetValue()) {
        return m_selectedMethod = 0; // PA塔
    } else if (m_radioLine->GetValue()) {
        return m_selectedMethod = 1; // 划线模式
    } else if (m_radioV->GetValue()) {
        return m_selectedMethod = 2; // V形模式
    }
    return -1; // 无选中（理论上不会发生）
}

// 获取选中状态的函数
int PA_Calibration_Dlg::GetSelectedExtruderType()
{
    if (m_radioNear->GetValue()) {
        return m_selectedExtruderType=0; // 近程挤出机
    } else if (m_radioFar->GetValue()) {
        return m_selectedExtruderType = 1; // 远程挤出机
    }
    return -1; // 无选中（理论上不会发生）
}


void PA_Calibration_Dlg::reset_params()
{
    GetSelectedExtruderType();
    GetSelectedMethod();

    m_tiStartPA->GetTextCtrl()->SetValue(wxString::FromDouble(0.0));
    // creality add bed temp
    m_bedTempValue                = 50;
    auto          filament_config = &wxGetApp().preset_bundle->filaments.get_edited_preset().config;
    ConfigOption* bed_type_opt    = wxGetApp().preset_bundle->project_config.option("curr_bed_type");
    if (bed_type_opt != nullptr) {
        BedType atype = (BedType) bed_type_opt->getInt();
        if ((BedType) bed_type_opt->getInt() == BedType::btPTE) {
            ConfigOptionInts* textured_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("textured_plate_temp"));
            if (textured_plate_temp && !textured_plate_temp->empty()) {
                m_bedTempValue    = textured_plate_temp->values[0];
                m_params.bed_type = BedType::btPTE;
            }
        } else if ((BedType) bed_type_opt->getInt() == BedType::btDEF) {
            ConfigOptionInts* customized_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("customized_plate_temp"));
            if (customized_plate_temp && !customized_plate_temp->empty()) {
                m_bedTempValue    = customized_plate_temp->values[0];
                m_params.bed_type = BedType::btDEF;
            }
        } else if ((BedType) bed_type_opt->getInt() == BedType::btER) {
            ConfigOptionInts* epoxy_resin_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("epoxy_resin_plate_temp"));
            if (epoxy_resin_plate_temp && !epoxy_resin_plate_temp->empty()) {
                m_bedTempValue    = epoxy_resin_plate_temp->values[0];
                m_params.bed_type = BedType::btER;
            }
        } else {
            // BedType::btPEI
            ConfigOptionInts* hot_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("hot_plate_temp"));
            if (hot_plate_temp && !hot_plate_temp->empty()) {
                m_bedTempValue    = hot_plate_temp->values[0];
                m_params.bed_type = BedType::btPEI;
            }
        }
    }
    m_tiBedTemp->GetTextCtrl()->SetValue(wxString::FromDouble(m_bedTempValue));
    bool isempty = m_tiEndPA->GetTextCtrl()->GetValue().empty();
    switch (m_selectedMethod) {
        case 1:
            m_params.mode = CalibMode::Calib_PA_Line;
            if (m_selectedExtruderType != iExtruderTypeSeletion || m_selectedMethod != imethod || isempty) {
                if (m_selectedExtruderType == 0) { //近端
                    m_tiStartPA->GetTextCtrl()->SetValue(wxString::FromDouble(0.0));
                    m_tiEndPA->GetTextCtrl()->SetValue(wxString::FromDouble(0.1));
                    m_tiPAStep->GetTextCtrl()->SetValue(wxString::FromDouble(0.002));
                } 
                else { //远端
                    m_tiStartPA->GetTextCtrl()->SetValue(wxString::FromDouble(0.0));
                    m_tiEndPA->GetTextCtrl()->SetValue(wxString::FromDouble(1.0));
                    m_tiPAStep->GetTextCtrl()->SetValue(wxString::FromDouble(0.02));
                }
            }
            m_cbPrintNum->SetValue(true);
            m_cbPrintNum->Enable(true);
            break;
        case 2:
            m_params.mode = CalibMode::Calib_PA_Pattern;
            if (m_selectedExtruderType != iExtruderTypeSeletion || m_selectedMethod != imethod || isempty) {
                if (m_selectedExtruderType == 0) { //近端
                    m_tiStartPA->GetTextCtrl()->SetValue(wxString::FromDouble(0.0));
                    m_tiEndPA->GetTextCtrl()->SetValue(wxString::FromDouble(0.08));
                    m_tiPAStep->GetTextCtrl()->SetValue(wxString::FromDouble(0.005));
                } else { //远端
                    m_tiStartPA->GetTextCtrl()->SetValue(wxString::FromDouble(0.0));
                    m_tiEndPA->GetTextCtrl()->SetValue(wxString::FromDouble(1.0));
                    m_tiPAStep->GetTextCtrl()->SetValue(wxString::FromDouble(0.05));
                }
            }
            m_cbPrintNum->SetValue(true);
            m_cbPrintNum->Enable(false);
            break;
        default:
            m_params.mode = CalibMode::Calib_PA_Tower;
            if (m_selectedExtruderType != iExtruderTypeSeletion || m_selectedMethod != imethod || isempty) {
                if (m_selectedExtruderType == 0) { //近端
                    m_tiStartPA->GetTextCtrl()->SetValue(wxString::FromDouble(0.0));
                    m_tiEndPA->GetTextCtrl()->SetValue(wxString::FromDouble(0.1));
                    m_tiPAStep->GetTextCtrl()->SetValue(wxString::FromDouble(0.002));
                } else { //远端
                    m_tiStartPA->GetTextCtrl()->SetValue(wxString::FromDouble(0.0));
                    m_tiEndPA->GetTextCtrl()->SetValue(wxString::FromDouble(1.0));
                    m_tiPAStep->GetTextCtrl()->SetValue(wxString::FromDouble(0.02));
                }
            }
            m_cbPrintNum->SetValue(false);
            m_cbPrintNum->Enable(false);
            break;
    }

    if (m_selectedExtruderType == 1) {
        m_tiEndPA->GetTextCtrl()->SetValue(wxString::FromDouble(1.0));
        
        if (m_params.mode == CalibMode::Calib_PA_Pattern) {
            m_tiPAStep->GetTextCtrl()->SetValue(wxString::FromDouble(0.05));
        } else {
            m_tiPAStep->GetTextCtrl()->SetValue(wxString::FromDouble(0.02));
        }
    }

    iExtruderTypeSeletion = m_selectedExtruderType;
    imethod               = m_selectedMethod;
}

void PA_Calibration_Dlg::on_start(wxCommandEvent& event) { 
    bool read_double = false;
    read_double = m_tiStartPA->GetTextCtrl()->GetValue().ToDouble(&m_params.start);
    read_double = read_double && m_tiEndPA->GetTextCtrl()->GetValue().ToDouble(&m_params.end);
    read_double = read_double && m_tiPAStep->GetTextCtrl()->GetValue().ToDouble(&m_params.step);
    read_double = read_double && m_tiBedTemp->GetTextCtrl()->GetValue().ToDouble(&m_params.bed_temp);
    if (!read_double || m_params.start < 0 || m_params.step < EPSILON || m_params.end < m_params.start + m_params.step ||
        m_params.bed_temp > 200 || m_params.bed_temp < 1) {
        MessageDialog msg_dlg(nullptr, _L("Please input valid values:\nStart PA: >= 0.0\nEnd PA: >= Start PA + PA step\nPA step: >= 0.001\nbed temp: >= 1\nbed temp: <= 200"), wxEmptyString, wxICON_WARNING | wxOK);
        msg_dlg.ShowModal();
        return;
    }

    switch (m_selectedMethod) {
        case 1:
            m_params.mode = CalibMode::Calib_PA_Line;
            break;
        case 2:
            m_params.mode = CalibMode::Calib_PA_Pattern;
            break;
        default:
            m_params.mode = CalibMode::Calib_PA_Tower;
    }

    m_params.print_numbers = m_cbPrintNum->GetValue();

    EndModal(wxID_OK);
    m_plater->calib_pa(m_params);


}
void PA_Calibration_Dlg::on_extruder_type_changed(wxCommandEvent& event) { 
    PA_Calibration_Dlg::reset_params();
    event.Skip(); 
}
void PA_Calibration_Dlg::on_method_changed(wxCommandEvent& event) { 
    PA_Calibration_Dlg::reset_params();
    event.Skip();
}

void PA_Calibration_Dlg::on_dpi_changed(const wxRect& suggested_rect) {
    this->Refresh(); 
    Fit();
}

void PA_Calibration_Dlg::on_show(wxShowEvent& event) {
    PA_Calibration_Dlg::reset_params();
}

// Temp calib dlg
//
enum FILAMENT_TYPE : int
{
    tPLA = 0,
    tABS_ASA,
    tPETG,
    tTPU,
    tPA_CF,
    tPET_CF,
    tCustom
};

Temp_Calibration_Dlg::Temp_Calibration_Dlg(wxWindow* parent, wxWindowID id, Plater* plater)
    : DPIDialog(parent, id, _L("Temperature calibration"), wxDefaultPosition, parent->FromDIP(wxSize(-1, 280)), wxDEFAULT_DIALOG_STYLE), m_plater(plater)
{
    SetFont(wxGetApp().normal_font());
    SetBackgroundColour(wxColour("#FFFFFF"));
    SetMaxSize(wxSize(FromDIP(600), FromDIP(320)));
    wxBoxSizer* v_sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(v_sizer);
    wxBoxSizer* choice_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxString m_rbFilamentTypeChoices[] = { _L("PLA"), _L("ABS"), _L("PETG"), /*_L("PCTG"),*/ _L("TPU"), _L("PA-CF"), _L("PET-CF"), /*_L("Custom")*/ };
    int m_rbFilamentTypeNChoices = sizeof(m_rbFilamentTypeChoices) / sizeof(wxString);
    m_rbFilamentType = new wxRadioBox(this, wxID_ANY, _L("Filament type"), wxDefaultPosition, wxDefaultSize, m_rbFilamentTypeNChoices, m_rbFilamentTypeChoices, 6, wxRA_SPECIFY_COLS);
    //m_rbFilamentType->SetForegroundColour(wxColour("#000000"));

    m_rbFilamentType->SetSelection(0);
    m_rbFilamentType->Select(0);

    choice_sizer->Add(m_rbFilamentType, 0, wxALL, 5);
    choice_sizer->Add(FromDIP(5), 0, 0, wxEXPAND, 5);
    wxString m_rbMethodChoices[] = { _L("PA Tower"), _L("PA Line") };

    v_sizer->Add(choice_sizer, 0, wxTOP | wxLEFT | wxRIGHT, 10);

    // Settings
    //
    wxString start_temp_str = _L("Start temp: ");
    wxString end_temp_str = _L("End temp: ");
    wxString temp_step_str = _L("Temp step: ");
    wxString bed_temp_str   = _L("Hot Plate Template: ");
    auto text_size = wxWindow::GetTextExtent(start_temp_str);
    text_size.IncTo(wxWindow::GetTextExtent(end_temp_str));
    text_size.IncTo(wxWindow::GetTextExtent(temp_step_str));
    text_size.IncTo(wxWindow::GetTextExtent(bed_temp_str));
    text_size.x = text_size.x * 5;
    wxStaticBoxSizer* settings_sizer = new wxStaticBoxSizer(wxVERTICAL, this, _L("Settings"));
    auto st_size = FromDIP(wxSize(430, -1));
    auto ti_size = FromDIP(wxSize(100, -1));
    // start temp
    auto start_temp_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto start_temp_text = new wxStaticText(this, wxID_ANY, start_temp_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //start_temp_text->SetForegroundColour(wxColour("#606060"));
    m_tiStart = new TextInput(this, std::to_string(230), _L("\u2103"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));

    start_temp_sizer->Add(start_temp_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    start_temp_sizer->Add(m_tiStart, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(start_temp_sizer, 0, wxLEFT | wxRIGHT, 10);
    // end temp
    auto end_temp_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto end_temp_text = new wxStaticText(this, wxID_ANY, end_temp_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //end_temp_text->SetForegroundColour(wxColour("#000000"));
    m_tiEnd = new TextInput(this, std::to_string(190), _L("\u2103"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    end_temp_sizer->Add(end_temp_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    end_temp_sizer->Add(m_tiEnd, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(end_temp_sizer, 0, wxLEFT | wxRIGHT, 10);;

    // temp step
    auto temp_step_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto temp_step_text = new wxStaticText(this, wxID_ANY, temp_step_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //temp_step_text->SetForegroundColour(wxColour("#FFFFFF"));
    m_tiStep = new TextInput(this, wxString::FromDouble(5),_L("\u2103"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    m_tiStep->Enable(false);
    temp_step_sizer->Add(temp_step_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    temp_step_sizer->Add(m_tiStep, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(temp_step_sizer, 0, wxLEFT | wxRIGHT, 10);

    // creality add bed temp
    auto bed_temp_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto bed_temp_text  = new wxStaticText(this, wxID_ANY, bed_temp_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //bed_temp_text->SetForegroundColour(wxColour("#999999"));
    m_tiBedTemp         = new TextInput(this, wxString::FromDouble(50), _L("℃"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    bed_temp_sizer->Add(bed_temp_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    bed_temp_sizer->Add(m_tiBedTemp, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(bed_temp_sizer, 0 , wxLEFT | wxRIGHT, 10);

    v_sizer->Add(settings_sizer, 0, wxLEFT | wxRIGHT, 10);
    v_sizer->Add(FromDIP(10), FromDIP(10), 0, wxEXPAND, 5);
    m_btnStart = new Button(this, _L("OK"));
    bool is_dark = wxGetApp().dark_mode();
    StateColor btn_bg_gray(std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Pressed),
                           std::pair<wxColour, int>(wxColour(150, 150, 155), StateColor::Hovered),
                           std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Normal));

    StateColor btn_bg_light_mode(std::pair<wxColour, int>(wxColour(246, 246, 249), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(246,246,249), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Normal));
    m_btnStart->SetBackgroundColor(is_dark ? btn_bg_gray : btn_bg_light_mode);
    m_btnStart->SetBorderColor(is_dark ? wxColour(0,0,0) : wxColour(214, 214, 220));
    m_btnStart->SetTextColor(is_dark ? wxColour(38, 46, 48) : wxColour(51, 51, 51));
    m_btnStart->SetSize(wxSize(FromDIP(100), FromDIP(28)));
    m_btnStart->SetMinSize(wxSize(FromDIP(100), FromDIP(28)));
    m_btnStart->SetCornerRadius(FromDIP(12));
    m_btnStart->Bind(wxEVT_BUTTON, &Temp_Calibration_Dlg::on_start, this);
    v_sizer->Add(m_btnStart, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, FromDIP(5));

    m_rbFilamentType->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler(Temp_Calibration_Dlg::on_filament_type_changed), NULL, this);
    m_btnStart->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Temp_Calibration_Dlg::on_start), NULL, this);

    wxGetApp().UpdateDlgDarkUI(this);

    Layout();
    Fit();

    auto validate_text = [this](TextInput* ti){
        unsigned long t = 0;
        if(!ti->GetTextCtrl()->GetValue().ToULong(&t))
            return;
        if(t> 350 || t < 170){
            MessageDialog msg_dlg(nullptr, wxString::Format(L"Supported range: 170%s - 350%s",_L("\u2103"),_L("\u2103")), wxEmptyString, wxICON_WARNING | wxOK);
            msg_dlg.ShowModal();
            if(t > 350)
                t = 350;
            else
                t = 170;
        }
        t = (t / 5) * 5;
        ti->GetTextCtrl()->SetValue(std::to_string(t));
    };

    m_tiStart->GetTextCtrl()->Bind(wxEVT_KILL_FOCUS, [&](wxFocusEvent &e) {
        validate_text(this->m_tiStart);
        e.Skip();
        });

    m_tiEnd->GetTextCtrl()->Bind(wxEVT_KILL_FOCUS, [&](wxFocusEvent &e) {
        validate_text(this->m_tiEnd);
        e.Skip();
        });

    
}

Temp_Calibration_Dlg::~Temp_Calibration_Dlg() {
    // Disconnect Events
    m_rbFilamentType->Disconnect(wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler(Temp_Calibration_Dlg::on_filament_type_changed), NULL, this);
    m_btnStart->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Temp_Calibration_Dlg::on_start), NULL, this);
}

void Temp_Calibration_Dlg::on_start(wxCommandEvent& event) {
    bool read_long = false;
    unsigned long start=0,end=0,bed_temp=50;
    read_long = m_tiStart->GetTextCtrl()->GetValue().ToULong(&start);
    read_long = read_long && m_tiEnd->GetTextCtrl()->GetValue().ToULong(&end);
    read_long = read_long && m_tiBedTemp->GetTextCtrl()->GetValue().ToULong(&bed_temp);

    if (!read_long || start > 350 || end < 170 || end > (start - 5) || bed_temp > 200 || bed_temp < 1) {
        MessageDialog msg_dlg(nullptr, _L("Please input valid values:\nStart temp: <= 350\nEnd temp: >= 170\nStart temp > End temp + 5\nbed temp: >= 1\nbed temp: <= 200"), wxEmptyString, wxICON_WARNING | wxOK);
        msg_dlg.ShowModal();
        return;
    }
    m_params.start = start;
    m_params.end = end;
    m_params.bed_temp = bed_temp;
    m_params.mode =CalibMode::Calib_Temp_Tower;

    auto          filament_config = &wxGetApp().preset_bundle->filaments.get_edited_preset().config;
    ConfigOption* bed_type_opt    = wxGetApp().preset_bundle->project_config.option("curr_bed_type");
    if (bed_type_opt != nullptr) {
        m_params.bed_type = (BedType) bed_type_opt->getInt();
    }

    EndModal(wxID_OK);
    m_plater->calib_temp(m_params);


}

void Temp_Calibration_Dlg::on_filament_type_changed(wxCommandEvent& event) {
    int selection = event.GetSelection();
    unsigned long start,end,bedtemp;
    switch(selection)
    {
        case tABS_ASA:
            start = 270;
            end = 230;
            bedtemp = 100;
            break;
        case tPETG:
            start = 250;
            end = 230;
            bedtemp = 70;
            break;
        case tTPU:
            start = 240;
            end = 210;
            bedtemp = 35;
            break;
        case tPA_CF:
            start = 320;
            end = 280;
            bedtemp = 100;
            break;
        case tPET_CF:
            start = 320;
            end = 280;
            bedtemp = 100;
            break;
        case tPLA:
        case tCustom:
            start = 230;
            end = 190;
            bedtemp = 50;
            break;
    }
    
    m_tiEnd->GetTextCtrl()->SetValue(std::to_string(end));
    m_tiStart->GetTextCtrl()->SetValue(std::to_string(start));
    m_tiBedTemp->GetTextCtrl()->SetValue(std::to_string(bedtemp));
    event.Skip();
}

void Temp_Calibration_Dlg::on_dpi_changed(const wxRect& suggested_rect) {
    this->Refresh();
    Fit();

}


// MaxVolumetricSpeed_Test_Dlg
//

MaxVolumetricSpeed_Test_Dlg::MaxVolumetricSpeed_Test_Dlg(wxWindow* parent, wxWindowID id, Plater* plater)
    : DPIDialog(parent, id, _L("Max volumetric speed test"), wxDefaultPosition, parent->FromDIP(wxSize(-1, 280)), wxDEFAULT_DIALOG_STYLE), m_plater(plater)
{
    SetFont(wxGetApp().normal_font());
    SetBackgroundColour(*wxWHITE);
    SetMaxSize(wxSize(FromDIP(600), FromDIP(230)));
    wxBoxSizer* v_sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(v_sizer);

    // Settings
    //
    wxString start_vol_str = _L("Start volumetric speed: ");
    wxString end_vol_str = _L("End volumetric speed: ");
    wxString vol_step_str = _L("step: ");
    wxString bed_temp_str  = _L("Hot Plate Template: ");
    auto text_size = wxWindow::GetTextExtent(start_vol_str);
    text_size.IncTo(wxWindow::GetTextExtent(end_vol_str));
    text_size.IncTo(wxWindow::GetTextExtent(vol_step_str));
    text_size.IncTo(wxWindow::GetTextExtent(bed_temp_str));
    text_size.x = text_size.x * 4.0;
    wxBoxSizer* settings_sizer = new wxBoxSizer(wxVERTICAL);

    auto st_size = FromDIP(wxSize(430, -1));
    auto ti_size = FromDIP(wxSize(100, -1));
    // start vol
    auto start_vol_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto start_vol_text = new wxStaticText(this, wxID_ANY, start_vol_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //start_vol_text->SetForegroundColour(wxColour("#000000"));
    m_tiStart = new TextInput(this, std::to_string(5), _L("mm³/s"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));

    start_vol_sizer->Add(start_vol_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    start_vol_sizer->Add(m_tiStart, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(start_vol_sizer, 0,wxTOP | wxLEFT | wxRIGHT, 10);

    // end vol
    auto end_vol_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto end_vol_text = new wxStaticText(this, wxID_ANY, end_vol_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //end_vol_text->SetForegroundColour(wxColour("#000000"));
    m_tiEnd = new TextInput(this, std::to_string(20), _L("mm³/s"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    end_vol_sizer->Add(end_vol_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    end_vol_sizer->Add(m_tiEnd, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(end_vol_sizer, 0, wxLEFT | wxRIGHT, 10);

    // vol step
    auto vol_step_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto vol_step_text = new wxStaticText(this, wxID_ANY, vol_step_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //vol_step_text->SetForegroundColour(wxColour("#000000"));
    m_tiStep = new TextInput(this, wxString::FromDouble(0.5), _L("mm³/s"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    vol_step_sizer->Add(vol_step_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    vol_step_sizer->Add(m_tiStep, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(vol_step_sizer, 0, wxLEFT | wxRIGHT, 10);

    // creality add bed temp
    auto bed_temp_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto bed_temp_text  = new wxStaticText(this, wxID_ANY, bed_temp_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //bed_temp_text->SetForegroundColour(wxColour("#000000"));
    m_tiBedTemp         = new TextInput(this, wxString::FromDouble(50), _L("℃"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    bed_temp_sizer->Add(bed_temp_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    bed_temp_sizer->Add(m_tiBedTemp, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(bed_temp_sizer, 0, wxLEFT | wxRIGHT, 10);

    v_sizer->Add(settings_sizer);
    v_sizer->Add(0, FromDIP(10), 0, wxEXPAND, 5);
    m_btnStart = new Button(this, _L("OK"));
    bool is_dark = wxGetApp().dark_mode();
    StateColor btn_bg_gray(std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(150, 150, 155), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Normal));

    StateColor btn_bg_light_mode(std::pair<wxColour, int>(wxColour(246, 246, 249), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(246, 246, 249), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Normal));
    m_btnStart->SetBackgroundColor(is_dark ? btn_bg_gray : btn_bg_light_mode);
    m_btnStart->SetBorderColor(is_dark ? wxColour(0, 0, 0) : wxColour(214, 214, 220));
    m_btnStart->SetTextColor(is_dark ? wxColour(38, 46, 48) : wxColour(51, 51, 51));
    m_btnStart->SetSize(wxSize(FromDIP(100), FromDIP(28)));
    m_btnStart->SetMinSize(wxSize(FromDIP(100), FromDIP(28)));
    m_btnStart->SetCornerRadius(FromDIP(12));
    m_btnStart->Bind(wxEVT_BUTTON, &MaxVolumetricSpeed_Test_Dlg::on_start, this);
    v_sizer->Add(m_btnStart, 0, wxALL | wxALIGN_CENTER, FromDIP(5));

    m_btnStart->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MaxVolumetricSpeed_Test_Dlg::on_start), NULL, this);
    this->Connect(wxEVT_SHOW, wxShowEventHandler(MaxVolumetricSpeed_Test_Dlg::on_show));
    wxGetApp().UpdateDlgDarkUI(this);

    Layout();
    Fit();
}

MaxVolumetricSpeed_Test_Dlg::~MaxVolumetricSpeed_Test_Dlg() {
    // Disconnect Events
    m_btnStart->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MaxVolumetricSpeed_Test_Dlg::on_start), NULL, this);
}

void MaxVolumetricSpeed_Test_Dlg::update_params() {

   m_bedTempValue  = 50;
    auto          filament_config = &wxGetApp().preset_bundle->filaments.get_edited_preset().config;
   ConfigOption*  bed_type_opt = wxGetApp().preset_bundle->project_config.option("curr_bed_type");
   if (bed_type_opt != nullptr) {
       BedType atype = (BedType) bed_type_opt->getInt();
       if ((BedType) bed_type_opt->getInt() == BedType::btPTE)
       {
           ConfigOptionInts* textured_plate_temp    = dynamic_cast<ConfigOptionInts*>(filament_config->option("textured_plate_temp"));
           if (textured_plate_temp && !textured_plate_temp->empty()) {
               m_bedTempValue = textured_plate_temp->values[0];
               m_params.bed_type = BedType::btPTE;
           }
       } 
       else if ((BedType) bed_type_opt->getInt() == BedType::btDEF)
       {
           ConfigOptionInts* customized_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("customized_plate_temp"));
           if (customized_plate_temp && !customized_plate_temp->empty()) {
               m_bedTempValue = customized_plate_temp->values[0];
               m_params.bed_type = BedType::btDEF;
           }
       } 
       else if ((BedType) bed_type_opt->getInt() == BedType::btER) 
       {
           ConfigOptionInts* epoxy_resin_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("epoxy_resin_plate_temp"));
           if (epoxy_resin_plate_temp && !epoxy_resin_plate_temp->empty()) {
               m_bedTempValue = epoxy_resin_plate_temp->values[0];
               m_params.bed_type = BedType::btER;
           }
       } 
       else
       {
           //BedType::btPEI
           ConfigOptionInts* hot_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("hot_plate_temp"));
           if (hot_plate_temp && !hot_plate_temp->empty()) {
               m_bedTempValue = hot_plate_temp->values[0];
               m_params.bed_type = BedType::btPEI;
           }
       }
   }
    m_tiBedTemp->GetTextCtrl()->SetValue(wxString::FromDouble(m_bedTempValue));
}

void MaxVolumetricSpeed_Test_Dlg::on_start(wxCommandEvent& event) {
    bool read_double = false;
    read_double = m_tiStart->GetTextCtrl()->GetValue().ToDouble(&m_params.start);
    read_double = read_double && m_tiEnd->GetTextCtrl()->GetValue().ToDouble(&m_params.end);
    read_double = read_double && m_tiStep->GetTextCtrl()->GetValue().ToDouble(&m_params.step);
    read_double = read_double && m_tiBedTemp->GetTextCtrl()->GetValue().ToDouble(&m_params.bed_temp);

    if (!read_double || m_params.start <= 0 || m_params.step <= 0 || m_params.end < (m_params.start + m_params.step) || m_params.bed_temp > 200 || m_params.bed_temp < 1) {
        MessageDialog msg_dlg(nullptr, _L("Please input valid values:\nstart > 0 \nstep > 0\nend >= start + step\nbed temp: >= 1\nbed temp: <= 200"), wxEmptyString, wxICON_WARNING | wxOK);
        msg_dlg.ShowModal();
        return;
    }

    m_params.mode = CalibMode::Calib_Vol_speed_Tower;
    EndModal(wxID_OK);
    m_plater->calib_max_vol_speed(m_params);


}

void MaxVolumetricSpeed_Test_Dlg::on_dpi_changed(const wxRect& suggested_rect) {
    this->Refresh();
    Fit();

}


void MaxVolumetricSpeed_Test_Dlg::on_show(wxShowEvent& event){ 
    update_params(); 
}

// VFA_Test_Dlg
//

VFA_Test_Dlg::VFA_Test_Dlg(wxWindow* parent, wxWindowID id, Plater* plater)
    : DPIDialog(parent, id, _L("VFA test"), wxDefaultPosition, parent->FromDIP(wxSize(-1, 280)), wxDEFAULT_DIALOG_STYLE)
    , m_plater(plater)
{
    SetFont(wxGetApp().normal_font());
    SetBackgroundColour(*wxWHITE);
    SetMaxSize(wxSize(FromDIP(600), FromDIP(230)));
    wxBoxSizer* v_sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(v_sizer);

    // Settings
    //
    wxString start_str = _L("Start speed: ");
    wxString end_vol_str = _L("End speed: ");
    wxString vol_step_str = _L("step: ");
    wxString bed_temp_str = _L("Hot Plate Template: ");
    auto text_size = wxWindow::GetTextExtent(start_str);
    text_size.IncTo(wxWindow::GetTextExtent(end_vol_str));
    text_size.IncTo(wxWindow::GetTextExtent(vol_step_str));
    text_size.IncTo(wxWindow::GetTextExtent(bed_temp_str));
    text_size.x = text_size.x * 4.0;
    wxBoxSizer* settings_sizer = new wxBoxSizer(wxVERTICAL);

    auto st_size = FromDIP(wxSize(430, -1));
    auto ti_size = FromDIP(wxSize(100, -1));
    // start vol
    auto start_vol_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto start_vol_text = new wxStaticText(this, wxID_ANY, start_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //start_vol_text->SetForegroundColour(wxColour("#000000"));
    m_tiStart = new TextInput(this, std::to_string(40), _L("mm/s"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));

    start_vol_sizer->Add(start_vol_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    start_vol_sizer->Add(m_tiStart, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(start_vol_sizer, 0, wxTOP | wxLEFT | wxRIGHT, 10);

    // end vol
    auto end_vol_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto end_vol_text = new wxStaticText(this, wxID_ANY, end_vol_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //end_vol_text->SetForegroundColour(wxColour("#000000"));
    m_tiEnd = new TextInput(this, std::to_string(200), _L("mm/s"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    end_vol_sizer->Add(end_vol_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    end_vol_sizer->Add(m_tiEnd, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(end_vol_sizer, 0, wxLEFT | wxRIGHT, 10);

    // vol step
    auto vol_step_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto vol_step_text = new wxStaticText(this, wxID_ANY, vol_step_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //vol_step_text->SetForegroundColour(wxColour("#000000"));
    m_tiStep = new TextInput(this, wxString::FromDouble(10), _L("mm/s"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    vol_step_sizer->Add(vol_step_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    vol_step_sizer->Add(m_tiStep, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(vol_step_sizer, 0, wxLEFT | wxRIGHT, 10);

    // creality add bed temp
    auto bed_temp_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto bed_temp_text  = new wxStaticText(this, wxID_ANY, bed_temp_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //bed_temp_text->SetForegroundColour(wxColour("#000000"));
    m_tiBedTemp         = new TextInput(this, wxString::FromDouble(50), _L("℃"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    bed_temp_sizer->Add(bed_temp_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    bed_temp_sizer->Add(m_tiBedTemp, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(bed_temp_sizer, 0, wxLEFT | wxRIGHT, 10);

    v_sizer->Add(settings_sizer);
    v_sizer->Add(0, FromDIP(10), 0, wxEXPAND, 5);
    m_btnStart = new Button(this, _L("OK"));
    bool is_dark = wxGetApp().dark_mode();
    StateColor btn_bg_gray(std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(150, 150, 155), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Normal));

    StateColor btn_bg_light_mode(std::pair<wxColour, int>(wxColour(246, 246, 249), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(246, 246, 249), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Normal));
    m_btnStart->SetBackgroundColor(is_dark ? btn_bg_gray : btn_bg_light_mode);
    m_btnStart->SetBorderColor(is_dark ? wxColour(0, 0, 0) : wxColour(214, 214, 220));
    m_btnStart->SetTextColor(is_dark ? wxColour(38, 46, 48) : wxColour(51, 51, 51));
    m_btnStart->SetSize(wxSize(FromDIP(100), FromDIP(28)));
    m_btnStart->SetMinSize(wxSize(FromDIP(100), FromDIP(28)));
    m_btnStart->SetCornerRadius(FromDIP(12));
    m_btnStart->Bind(wxEVT_BUTTON, &VFA_Test_Dlg::on_start, this);
    v_sizer->Add(m_btnStart, 0, wxALL | wxALIGN_CENTER, FromDIP(5));

    m_btnStart->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(VFA_Test_Dlg::on_start), NULL, this);
    this->Connect(wxEVT_SHOW, wxShowEventHandler(VFA_Test_Dlg::on_show));
    wxGetApp().UpdateDlgDarkUI(this);

    Layout();
    Fit();
}

VFA_Test_Dlg::~VFA_Test_Dlg()
{
    // Disconnect Events
    m_btnStart->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(VFA_Test_Dlg::on_start), NULL, this);
}

void VFA_Test_Dlg::on_show(wxShowEvent& event) { update_params(); }

void VFA_Test_Dlg::update_params()
{
    m_bedTempValue                    = 50;
    auto          filament_config = &wxGetApp().preset_bundle->filaments.get_edited_preset().config;
    ConfigOption* bed_type_opt    = wxGetApp().preset_bundle->project_config.option("curr_bed_type");
    if (bed_type_opt != nullptr) {
        BedType atype = (BedType) bed_type_opt->getInt();
        if ((BedType) bed_type_opt->getInt() == BedType::btPTE) {
            ConfigOptionInts* textured_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("textured_plate_temp"));
            if (textured_plate_temp && !textured_plate_temp->empty()) {
                m_bedTempValue    = textured_plate_temp->values[0];
                m_params.bed_type = BedType::btPTE;
            }
        } else if ((BedType) bed_type_opt->getInt() == BedType::btDEF) {
            ConfigOptionInts* customized_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("customized_plate_temp"));
            if (customized_plate_temp && !customized_plate_temp->empty()) {
                m_bedTempValue    = customized_plate_temp->values[0];
                m_params.bed_type = BedType::btDEF;
            }
        } else if ((BedType) bed_type_opt->getInt() == BedType::btER) {
            ConfigOptionInts* epoxy_resin_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("epoxy_resin_plate_temp"));
            if (epoxy_resin_plate_temp && !epoxy_resin_plate_temp->empty()) {
                m_bedTempValue    = epoxy_resin_plate_temp->values[0];
                m_params.bed_type = BedType::btER;
            }
        } else {
            // BedType::btPEI
            ConfigOptionInts* hot_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("hot_plate_temp"));
            if (hot_plate_temp && !hot_plate_temp->empty()) {
                m_bedTempValue    = hot_plate_temp->values[0];
                m_params.bed_type = BedType::btPEI;
            }
        }
    }
    m_tiBedTemp->GetTextCtrl()->SetValue(wxString::FromDouble(m_bedTempValue));
}



void VFA_Test_Dlg::on_start(wxCommandEvent& event)
{
    bool read_double = false;
    read_double = m_tiStart->GetTextCtrl()->GetValue().ToDouble(&m_params.start);
    read_double = read_double && m_tiEnd->GetTextCtrl()->GetValue().ToDouble(&m_params.end);
    read_double = read_double && m_tiStep->GetTextCtrl()->GetValue().ToDouble(&m_params.step);
    read_double = read_double && m_tiBedTemp->GetTextCtrl()->GetValue().ToDouble(&m_params.bed_temp);

    if (!read_double || m_params.start <= 10 || m_params.step <= 0 || m_params.end < (m_params.start + m_params.step) ||
        m_params.bed_temp > 200 || m_params.bed_temp < 1) {
        MessageDialog msg_dlg(nullptr, _L("Please input valid values:\nstart > 10 \nstep > 0\nend >= start + step\nbed temp: >= 1\nbed temp: <= 200"), wxEmptyString, wxICON_WARNING | wxOK);
        msg_dlg.ShowModal();
        return;
    }

    m_params.mode = CalibMode::Calib_VFA_Tower;

    EndModal(wxID_OK);
    m_plater->calib_VFA(m_params);

}

void VFA_Test_Dlg::on_dpi_changed(const wxRect& suggested_rect)
{
    this->Refresh();
    Fit();
}



// Retraction_Test_Dlg
//

Retraction_Test_Dlg::Retraction_Test_Dlg(wxWindow* parent, wxWindowID id, Plater* plater)
    : DPIDialog(parent, id, _L("Retraction distance test"), wxDefaultPosition, parent->FromDIP(wxSize(-1, 280)), wxDEFAULT_DIALOG_STYLE), m_plater(plater)
{
    SetFont(wxGetApp().normal_font());
    SetBackgroundColour(*wxWHITE);
    SetMaxSize(wxSize(FromDIP(600), FromDIP(230)));
    wxBoxSizer* v_sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(v_sizer);
    // Settings
    //
    wxString start_length_str = _L("Start retraction length: ");
    wxString end_length_str   = _L("End retraction length: ");
    wxString length_step_str  = _L("step: ");
    wxString bed_temp_str     = _L("Hot Plate Template: ");
    auto text_size = wxWindow::GetTextExtent(start_length_str);
    text_size.IncTo(wxWindow::GetTextExtent(end_length_str));
    text_size.IncTo(wxWindow::GetTextExtent(length_step_str));
    text_size.IncTo(wxWindow::GetTextExtent(bed_temp_str));
    text_size.x = text_size.x * 4.0;
    wxBoxSizer* settings_sizer = new wxBoxSizer(wxVERTICAL);


    auto st_size = FromDIP(wxSize(430, -1));
    auto ti_size = FromDIP(wxSize(100, -1));
    // start length
    auto start_length_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto start_length_text = new wxStaticText(this, wxID_ANY, start_length_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //start_length_text->SetForegroundColour(wxColour("#000000"));
    m_tiStart = new TextInput(this, std::to_string(0), _L("mm"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));

    start_length_sizer->Add(start_length_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    start_length_sizer->Add(m_tiStart, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(start_length_sizer,0,wxTOP | wxLEFT | wxRIGHT,10);

    // end length
    auto end_length_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto end_length_text = new wxStaticText(this, wxID_ANY, end_length_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //end_length_text->SetForegroundColour(wxColour("#000000"));
    m_tiEnd = new TextInput(this, std::to_string(2), _L("mm"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    end_length_sizer->Add(end_length_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    end_length_sizer->Add(m_tiEnd, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(end_length_sizer,0, wxLEFT | wxRIGHT, 10);

    // length step
    auto length_step_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto length_step_text = new wxStaticText(this, wxID_ANY, length_step_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //length_step_text->SetForegroundColour(wxColour("#000000"));
    m_tiStep = new TextInput(this, wxString::FromDouble(0.1), _L("mm/mm"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    length_step_sizer->Add(length_step_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    length_step_sizer->Add(m_tiStep, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(length_step_sizer, 0, wxLEFT | wxRIGHT, 10);

    // creality add bed temp
    auto bed_temp_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto bed_temp_text  = new wxStaticText(this, wxID_ANY, bed_temp_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //bed_temp_text->SetForegroundColour(wxColour("#000000"));
    m_tiBedTemp         = new TextInput(this, wxString::FromDouble(50), _L("℃"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    bed_temp_sizer->Add(bed_temp_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    bed_temp_sizer->Add(m_tiBedTemp, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(bed_temp_sizer, 0 ,wxLEFT | wxRIGHT, 10);

    v_sizer->Add(settings_sizer);
    v_sizer->Add(FromDIP(10), FromDIP(10), 0, wxEXPAND, 5);
    m_btnStart = new Button(this, _L("OK"));
    bool is_dark = wxGetApp().dark_mode();
    StateColor btn_bg_gray(std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(150, 150, 155), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Normal));

    StateColor btn_bg_light_mode(std::pair<wxColour, int>(wxColour(246, 246, 249), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(246, 246, 249), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Normal));
    m_btnStart->SetBackgroundColor(is_dark ? btn_bg_gray : btn_bg_light_mode);
    m_btnStart->SetBorderColor(is_dark ? wxColour(0, 0, 0) : wxColour(214, 214, 220));
    m_btnStart->SetTextColor(is_dark ? wxColour(38, 46, 48) : wxColour(51, 51, 51));
    m_btnStart->SetSize(wxSize(FromDIP(100), FromDIP(28)));
    m_btnStart->SetMinSize(wxSize(FromDIP(100), FromDIP(28)));
    m_btnStart->SetCornerRadius(FromDIP(12));
    m_btnStart->Bind(wxEVT_BUTTON, &Retraction_Test_Dlg::on_start, this);
    v_sizer->Add(m_btnStart, 0, wxALL | wxALIGN_CENTER, FromDIP(5));

    m_btnStart->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Retraction_Test_Dlg::on_start), NULL, this);
    this->Connect(wxEVT_SHOW, wxShowEventHandler(Retraction_Test_Dlg::on_show));
    wxGetApp().UpdateDlgDarkUI(this);

    Layout();
    Fit();
}

Retraction_Test_Dlg::~Retraction_Test_Dlg() {
    // Disconnect Events
    m_btnStart->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Retraction_Test_Dlg::on_start), NULL, this);
}

void Retraction_Test_Dlg::update_params() 
{
    // creality add bed temp
    m_bedTempValue                    = 50;
    auto          filament_config = &wxGetApp().preset_bundle->filaments.get_edited_preset().config;
    ConfigOption* bed_type_opt    = wxGetApp().preset_bundle->project_config.option("curr_bed_type");
    if (bed_type_opt != nullptr) {
        BedType atype = (BedType) bed_type_opt->getInt();
        if ((BedType) bed_type_opt->getInt() == BedType::btPTE) {
            ConfigOptionInts* textured_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("textured_plate_temp"));
            if (textured_plate_temp && !textured_plate_temp->empty()) {
                m_bedTempValue    = textured_plate_temp->values[0];
                m_params.bed_type = BedType::btPTE;
            }
        } else if ((BedType) bed_type_opt->getInt() == BedType::btDEF) {
            ConfigOptionInts* customized_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("customized_plate_temp"));
            if (customized_plate_temp && !customized_plate_temp->empty()) {
                m_bedTempValue    = customized_plate_temp->values[0];
                m_params.bed_type = BedType::btDEF;
            }
        } else if ((BedType) bed_type_opt->getInt() == BedType::btER) {
            ConfigOptionInts* epoxy_resin_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("epoxy_resin_plate_temp"));
            if (epoxy_resin_plate_temp && !epoxy_resin_plate_temp->empty()) {
                m_bedTempValue    = epoxy_resin_plate_temp->values[0];
                m_params.bed_type = BedType::btER;
            }
        } else {
            // BedType::btPEI
            ConfigOptionInts* hot_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("hot_plate_temp"));
            if (hot_plate_temp && !hot_plate_temp->empty()) {
                m_bedTempValue    = hot_plate_temp->values[0];
                m_params.bed_type = BedType::btPEI;
            }
        }
    }
    m_tiBedTemp->GetTextCtrl()->SetValue(wxString::FromDouble(m_bedTempValue));
}

void Retraction_Test_Dlg::on_start(wxCommandEvent& event) {
    bool read_double = false;
    read_double = m_tiStart->GetTextCtrl()->GetValue().ToDouble(&m_params.start);
    read_double = read_double && m_tiEnd->GetTextCtrl()->GetValue().ToDouble(&m_params.end);
    read_double = read_double && m_tiStep->GetTextCtrl()->GetValue().ToDouble(&m_params.step);
    read_double = read_double && m_tiBedTemp->GetTextCtrl()->GetValue().ToDouble(&m_params.bed_temp);

    if (!read_double || m_params.start < 0 || m_params.step <= 0 || m_params.end < (m_params.start + m_params.step) ||
        m_params.bed_temp > 200 || m_params.bed_temp < 1) {
        MessageDialog msg_dlg(nullptr, _L("Please input valid values:\nstart >= 0 \nstep > 0\nend >= start + step\nbed temp: >= 1\nbed temp: <= 200"), wxEmptyString, wxICON_WARNING | wxOK);
        msg_dlg.ShowModal();
        return;
    }

    m_params.mode = CalibMode::Calib_Retraction_tower;

    EndModal(wxID_OK);
    m_plater->calib_retraction(m_params);


}

void Retraction_Test_Dlg::on_dpi_changed(const wxRect& suggested_rect) {
    this->Refresh();
    Fit();

}


void Retraction_Test_Dlg::on_show(wxShowEvent& event) 
{ 
    update_params();
}

// Retraction_Speed_Dlg
//

Retraction_Speed_Dlg::Retraction_Speed_Dlg(wxWindow* parent, wxWindowID id, Plater* plater)
    : DPIDialog(parent,
                id,
                _L("Retraction speed test"),
                wxDefaultPosition,
                parent->FromDIP(wxSize(-1, 280)),
                wxDEFAULT_DIALOG_STYLE)
    , m_plater(plater)
{
    SetFont(wxGetApp().normal_font());
    SetBackgroundColour(*wxWHITE);
    SetMaxSize(wxSize(FromDIP(600), FromDIP(240)));
    wxBoxSizer* v_sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(v_sizer);

    // Settings
    //
    wxString start_length_str = _L("Start retraction speed: ");
    wxString end_length_str   = _L("End retraction speed: ");
    wxString length_step_str  = _L("step: ");
    wxString bed_temp_str     = _L("Hot Plate Template: ");
    auto     text_size        = wxWindow::GetTextExtent(start_length_str);
    text_size.IncTo(wxWindow::GetTextExtent(end_length_str));
    text_size.IncTo(wxWindow::GetTextExtent(length_step_str));
    text_size.IncTo(wxWindow::GetTextExtent(bed_temp_str));
    text_size.x                      = text_size.x * 4.0;
    wxBoxSizer* settings_sizer = new wxBoxSizer(wxVERTICAL);

    auto st_size = FromDIP(wxSize(430, -1));
    auto ti_size = FromDIP(wxSize(100, -1));
    // start length
    auto start_length_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto start_length_text  = new wxStaticText(this, wxID_ANY, start_length_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //start_length_text->SetForegroundColour(wxColour("#000000"));
    m_tiStart               = new TextInput(this, std::to_string(10), _L("mm/s"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));

    start_length_sizer->Add(start_length_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    start_length_sizer->Add(m_tiStart, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(start_length_sizer, 0, wxTOP | wxLEFT | wxRIGHT, 10);

    // end length
    auto end_length_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto end_length_text  = new wxStaticText(this, wxID_ANY, end_length_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //end_length_text->SetForegroundColour(wxColour("#000000"));
    m_tiEnd               = new TextInput(this, std::to_string(80), _L("mm/s"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    end_length_sizer->Add(end_length_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    end_length_sizer->Add(m_tiEnd, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(end_length_sizer, 0, wxLEFT | wxRIGHT, 10);

    // length step
    auto length_step_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto length_step_text  = new wxStaticText(this, wxID_ANY, length_step_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //length_step_text->SetForegroundColour(wxColour("#000000"));
    m_tiStep               = new TextInput(this, wxString::FromDouble(5), _L("mm/s"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    length_step_sizer->Add(length_step_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    length_step_sizer->Add(m_tiStep, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(length_step_sizer, 0, wxLEFT | wxRIGHT, 10);

    // creality add bed temp
    auto bed_temp_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto bed_temp_text  = new wxStaticText(this, wxID_ANY, bed_temp_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //bed_temp_text->SetForegroundColour(wxColour("#000000"));
    m_tiBedTemp         = new TextInput(this, wxString::FromDouble(50), _L("℃"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    bed_temp_sizer->Add(bed_temp_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    bed_temp_sizer->Add(m_tiBedTemp, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(bed_temp_sizer, 0, wxLEFT | wxRIGHT, 10);

    v_sizer->Add(settings_sizer, 0, wxTOP | wxLEFT | wxRIGHT, 10);
    v_sizer->Add(FromDIP(10), FromDIP(10), 0, wxEXPAND, 5);
    m_btnStart = new Button(this, _L("OK"));
    bool is_dark = wxGetApp().dark_mode();
    StateColor btn_bg_gray(std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(150, 150, 155), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Normal));

    StateColor btn_bg_light_mode(std::pair<wxColour, int>(wxColour(246, 246, 249), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(246, 246, 249), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Normal));
    m_btnStart->SetBackgroundColor(is_dark ? btn_bg_gray : btn_bg_light_mode);
    m_btnStart->SetBorderColor(is_dark ? wxColour(0, 0, 0) : wxColour(214, 214, 220));
    m_btnStart->SetTextColor(is_dark ? wxColour(38, 46, 48) : wxColour(51, 51, 51));
    m_btnStart->SetSize(wxSize(FromDIP(100), FromDIP(28)));
    m_btnStart->SetMinSize(wxSize(FromDIP(100), FromDIP(28)));
    m_btnStart->SetCornerRadius(FromDIP(12));
    m_btnStart->Bind(wxEVT_BUTTON, &Retraction_Speed_Dlg::on_start, this);
    v_sizer->Add(m_btnStart, 0, wxALL | wxALIGN_CENTER, FromDIP(5));

    m_btnStart->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Retraction_Speed_Dlg::on_start), NULL, this);
    this->Connect(wxEVT_SHOW, wxShowEventHandler(Retraction_Speed_Dlg::on_show));
    wxGetApp().UpdateDlgDarkUI(this);

    Layout();
    Fit();
}

void Retraction_Speed_Dlg::on_show(wxShowEvent& event) { 
    update_params(); 
}

void Retraction_Speed_Dlg::update_params()
{
    // creality add bed temp
    m_bedTempValue                    = 50;
    auto          filament_config = &wxGetApp().preset_bundle->filaments.get_edited_preset().config;
    ConfigOption* bed_type_opt    = wxGetApp().preset_bundle->project_config.option("curr_bed_type");
    if (bed_type_opt != nullptr) {
        BedType atype = (BedType) bed_type_opt->getInt();
        if ((BedType) bed_type_opt->getInt() == BedType::btPTE) {
            ConfigOptionInts* textured_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("textured_plate_temp"));
            if (textured_plate_temp && !textured_plate_temp->empty()) {
                m_bedTempValue    = textured_plate_temp->values[0];
                m_params.bed_type = BedType::btPTE;
            }
        } else if ((BedType) bed_type_opt->getInt() == BedType::btDEF) {
            ConfigOptionInts* customized_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("customized_plate_temp"));
            if (customized_plate_temp && !customized_plate_temp->empty()) {
                m_bedTempValue    = customized_plate_temp->values[0];
                m_params.bed_type = BedType::btDEF;
            }
        } else if ((BedType) bed_type_opt->getInt() == BedType::btER) {
            ConfigOptionInts* epoxy_resin_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("epoxy_resin_plate_temp"));
            if (epoxy_resin_plate_temp && !epoxy_resin_plate_temp->empty()) {
                m_bedTempValue    = epoxy_resin_plate_temp->values[0];
                m_params.bed_type = BedType::btER;
            }
        } else {
            // BedType::btPEI
            ConfigOptionInts* hot_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("hot_plate_temp"));
            if (hot_plate_temp && !hot_plate_temp->empty()) {
                m_bedTempValue    = hot_plate_temp->values[0];
                m_params.bed_type = BedType::btPEI;
            }
        }
    }
    m_tiBedTemp->GetTextCtrl()->SetValue(wxString::FromDouble(m_bedTempValue));
}

Retraction_Speed_Dlg::~Retraction_Speed_Dlg()
{
    // Disconnect Events
    m_btnStart->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Retraction_Speed_Dlg::on_start), NULL, this);
}

void Retraction_Speed_Dlg::on_start(wxCommandEvent& event)
{
    bool read_double = false;
    read_double      = m_tiStart->GetTextCtrl()->GetValue().ToDouble(&m_params.start);
    read_double      = read_double && m_tiEnd->GetTextCtrl()->GetValue().ToDouble(&m_params.end);
    read_double      = read_double && m_tiStep->GetTextCtrl()->GetValue().ToDouble(&m_params.step);
    read_double      = read_double && m_tiBedTemp->GetTextCtrl()->GetValue().ToDouble(&m_params.bed_temp);

    if (!read_double || m_params.start <= 0 || m_params.step <= 0 || m_params.end < (m_params.start + m_params.step) ||
        m_params.bed_temp > 200 || m_params.bed_temp < 1) {
        MessageDialog msg_dlg(nullptr, _L("Please input valid values:\nstart > 0 \nstep > 0\nend >= start + step\nbed temp: >= 1\nbed temp: <= 200"), wxEmptyString,
                              wxICON_WARNING | wxOK);
        msg_dlg.ShowModal();
        return;
    }

    m_params.mode = CalibMode::Calib_Retraction_tower_speed;
    EndModal(wxID_OK);
    m_plater->calib_retraction_speed(m_params);

}

void Retraction_Speed_Dlg::on_dpi_changed(const wxRect& suggested_rect)
{
    this->Refresh();
    Fit();
}


// Limit_Speed_Dlg
//

Limit_Speed_Dlg::Limit_Speed_Dlg(wxWindow* parent, wxWindowID id, Plater* plater)
    : DPIDialog(parent,
                id,
                _L("Limit speed test"),
                wxDefaultPosition,
                parent->FromDIP(wxSize(-1, 280)),
                wxDEFAULT_DIALOG_STYLE)
    , m_plater(plater)
{
    SetFont(wxGetApp().normal_font());
    SetBackgroundColour(*wxWHITE);
    SetMaxSize(wxSize(FromDIP(600), FromDIP(260)));
    wxBoxSizer* v_sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(v_sizer);

    // Settings
    //
    wxString start_length_str = _L("Start speed: ");
    wxString end_length_str   = _L("End speed: ");
    wxString length_step_str  = _L("step: ");
    wxString bed_temp_str         = _L("Hot Plate Template: ");
    wxString acc_speed_str     = _L("Accelerated speed: ");
    auto     text_size        = wxWindow::GetTextExtent(start_length_str);
    text_size.IncTo(wxWindow::GetTextExtent(end_length_str));
    text_size.IncTo(wxWindow::GetTextExtent(length_step_str));
    text_size.IncTo(wxWindow::GetTextExtent(bed_temp_str));
    text_size.IncTo(wxWindow::GetTextExtent(acc_speed_str));
    text_size.x                      = text_size.x * 4.0;
    wxBoxSizer* settings_sizer = new wxBoxSizer(wxVERTICAL);

    auto st_size = FromDIP(wxSize(430, -1));
    auto ti_size = FromDIP(wxSize(100, -1));
    // start length
    auto start_length_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto start_length_text  = new wxStaticText(this, wxID_ANY, start_length_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //start_length_text->SetForegroundColour(wxColour("#000000"));
    m_tiStart               = new TextInput(this, std::to_string(50), _L("mm/s"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));

    start_length_sizer->Add(start_length_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    start_length_sizer->Add(m_tiStart, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(start_length_sizer, 0, wxTOP | wxLEFT | wxRIGHT, 10);

    // end length
    auto end_length_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto end_length_text  = new wxStaticText(this, wxID_ANY, end_length_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
   // end_length_text->SetForegroundColour(wxColour("#000000"));
    m_tiEnd               = new TextInput(this, std::to_string(400), _L("mm/s"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    end_length_sizer->Add(end_length_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    end_length_sizer->Add(m_tiEnd, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(end_length_sizer, 0, wxLEFT | wxRIGHT, 10);

    // length step
    auto length_step_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto length_step_text  = new wxStaticText(this, wxID_ANY, length_step_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //length_step_text->SetForegroundColour(wxColour("#000000"));
    m_tiStep               = new TextInput(this, wxString::FromDouble(10), _L("mm/s"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    length_step_sizer->Add(length_step_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    length_step_sizer->Add(m_tiStep, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(length_step_sizer, 0, wxLEFT | wxRIGHT, 10);

    // creality add bed temp
    auto bed_temp_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto bed_temp_text  = new wxStaticText(this, wxID_ANY, bed_temp_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //bed_temp_text->SetForegroundColour(wxColour("#000000"));
    m_tiBedTemp         = new TextInput(this, wxString::FromDouble(50), _L("℃"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    bed_temp_sizer->Add(bed_temp_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    bed_temp_sizer->Add(m_tiBedTemp, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(bed_temp_sizer, 0, wxLEFT | wxRIGHT, 10);

    // creality add acc speed
    auto acc_speed_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto acc_speed_text  = new wxStaticText(this, wxID_ANY, acc_speed_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //acc_speed_text->SetForegroundColour(wxColour("#000000"));
    m_tiAccSpeed          = new TextInput(this, wxString::FromDouble(5000), _L("mm/s²"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    acc_speed_sizer->Add(acc_speed_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    acc_speed_sizer->Add(m_tiAccSpeed, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(acc_speed_sizer, 0, wxLEFT | wxRIGHT, 10);

    v_sizer->Add(settings_sizer);
    v_sizer->Add(0, FromDIP(10), 0, wxEXPAND, 5);
    m_btnStart = new Button(this, _L("OK"));
    bool is_dark = wxGetApp().dark_mode();
    StateColor btn_bg_gray(std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(150, 150, 155), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Normal));

    StateColor btn_bg_light_mode(std::pair<wxColour, int>(wxColour(246, 246, 249), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(246, 246, 249), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Normal));
    m_btnStart->SetBackgroundColor(is_dark ? btn_bg_gray : btn_bg_light_mode);
    m_btnStart->SetBorderColor(is_dark ? wxColour(0, 0, 0) : wxColour(214, 214, 220));
    m_btnStart->SetTextColor(is_dark ? wxColour(38, 46, 48) : wxColour(51, 51, 51));
    m_btnStart->SetSize(wxSize(FromDIP(100), FromDIP(28)));
    m_btnStart->SetMinSize(wxSize(FromDIP(100), FromDIP(28)));
    m_btnStart->SetCornerRadius(FromDIP(12));
    m_btnStart->Bind(wxEVT_BUTTON, &Limit_Speed_Dlg::on_start, this);
    v_sizer->Add(m_btnStart, 0, wxALL | wxALIGN_CENTER, FromDIP(5));

    m_btnStart->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Limit_Speed_Dlg::on_start), NULL, this);
    this->Connect(wxEVT_SHOW, wxShowEventHandler(Limit_Speed_Dlg::on_show));
    wxGetApp().UpdateDlgDarkUI(this);

    Layout();
    Fit();
}

Limit_Speed_Dlg::~Limit_Speed_Dlg()
{
    // Disconnect Events
    m_btnStart->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Limit_Speed_Dlg::on_start), NULL, this);
}

void Limit_Speed_Dlg::on_show(wxShowEvent& event) { update_params(); }

void Limit_Speed_Dlg::update_params()
    {
    m_bedTempValue                    = 50;
        auto          filament_config = &wxGetApp().preset_bundle->filaments.get_edited_preset().config;
        ConfigOption* bed_type_opt    = wxGetApp().preset_bundle->project_config.option("curr_bed_type");
        if (bed_type_opt != nullptr) {
            BedType atype = (BedType) bed_type_opt->getInt();
            if ((BedType) bed_type_opt->getInt() == BedType::btPTE) {
                ConfigOptionInts* textured_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("textured_plate_temp"));
                if (textured_plate_temp && !textured_plate_temp->empty()) {
                    m_bedTempValue    = textured_plate_temp->values[0];
                    m_params.bed_type = BedType::btPTE;
                }
            } else if ((BedType) bed_type_opt->getInt() == BedType::btDEF) {
                ConfigOptionInts* customized_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("customized_plate_temp"));
                if (customized_plate_temp && !customized_plate_temp->empty()) {
                    m_bedTempValue    = customized_plate_temp->values[0];
                    m_params.bed_type = BedType::btDEF;
                }
            } else if ((BedType) bed_type_opt->getInt() == BedType::btER) {
                ConfigOptionInts* epoxy_resin_plate_temp = dynamic_cast<ConfigOptionInts*>(
                    filament_config->option("epoxy_resin_plate_temp"));
                if (epoxy_resin_plate_temp && !epoxy_resin_plate_temp->empty()) {
                    m_bedTempValue    = epoxy_resin_plate_temp->values[0];
                    m_params.bed_type = BedType::btER;
                }
            } else {
                // BedType::btPEI
                ConfigOptionInts* hot_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("hot_plate_temp"));
                if (hot_plate_temp && !hot_plate_temp->empty()) {
                    m_bedTempValue    = hot_plate_temp->values[0];
                    m_params.bed_type = BedType::btPEI;
                }
            }
        }
    m_tiBedTemp->GetTextCtrl()->SetValue(wxString::FromDouble(m_bedTempValue));
}


void Limit_Speed_Dlg::on_start(wxCommandEvent& event)
{
    bool read_double = false;
    read_double      = m_tiStart->GetTextCtrl()->GetValue().ToDouble(&m_params.start);
    read_double      = read_double && m_tiEnd->GetTextCtrl()->GetValue().ToDouble(&m_params.end);
    read_double      = read_double && m_tiStep->GetTextCtrl()->GetValue().ToDouble(&m_params.step);
    read_double      = read_double && m_tiBedTemp->GetTextCtrl()->GetValue().ToDouble(&m_params.bed_temp);
    read_double      = read_double && m_tiAccSpeed->GetTextCtrl()->GetValue().ToDouble(&m_params.acc_speed);

    if (!read_double || m_params.start <= 0 || m_params.step <= 0 || m_params.end < (m_params.start + m_params.step) ||
        m_params.bed_temp > 200 || m_params.bed_temp < 1) {
        MessageDialog msg_dlg(nullptr, _L("Please input valid values:\nstart > 0 \nstep > 0\nend >= start + step\nbed temp: >= 1\nbed temp: <= 200"), wxEmptyString,
                              wxICON_WARNING | wxOK);
        msg_dlg.ShowModal();
        return;
    }

    m_params.mode = CalibMode::Calib_Limit_Speed;
    EndModal(wxID_OK);
    m_plater->calib_limit_speed(m_params);
}

void Limit_Speed_Dlg::on_dpi_changed(const wxRect& suggested_rect)
{
    this->Refresh();
    Fit();
}


// Speed_Tower_Dlg
//

Speed_Tower_Dlg::Speed_Tower_Dlg(wxWindow* parent, wxWindowID id, Plater* plater)
    : DPIDialog(parent,
                id,
                _L("Speed tower test"),
                wxDefaultPosition,
                parent->FromDIP(wxSize(-1, 280)),
                wxDEFAULT_DIALOG_STYLE)
    , m_plater(plater)
{
    SetFont(wxGetApp().normal_font());
    SetBackgroundColour(*wxWHITE);
    SetMaxSize(wxSize(FromDIP(600), FromDIP(230)));
    wxBoxSizer* v_sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(v_sizer);

    // Settings
    //
    wxString start_length_str = _L("Start speed: ");
    wxString end_length_str   = _L("End speed: ");
    wxString length_step_str  = _L("step: ");
    wxString bed_temp_str     = _L("Hot Plate Template: ");
    //wxString acc_speed_str    = _L("Accelerated speed: ");
    auto     text_size        = wxWindow::GetTextExtent(start_length_str);
    text_size.IncTo(wxWindow::GetTextExtent(end_length_str));
    text_size.IncTo(wxWindow::GetTextExtent(length_step_str));
    text_size.IncTo(wxWindow::GetTextExtent(bed_temp_str));
    //text_size.IncTo(wxWindow::GetTextExtent(acc_speed_str));
    text_size.x                      = text_size.x * 4.0;
    wxBoxSizer* settings_sizer = new wxBoxSizer(wxVERTICAL);

    auto st_size = FromDIP(wxSize(430, -1));
    auto ti_size = FromDIP(wxSize(100, -1));
    // start length
    auto start_length_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto start_length_text  = new wxStaticText(this, wxID_ANY, start_length_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //start_length_text->SetForegroundColour(wxColour("#000000"));
    m_tiStart               = new TextInput(this, std::to_string(40), _L("mm/s"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));

    start_length_sizer->Add(start_length_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    start_length_sizer->Add(m_tiStart, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(start_length_sizer, 0,wxTOP | wxLEFT | wxRIGHT, 10);

    // end length
    auto end_length_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto end_length_text  = new wxStaticText(this, wxID_ANY, end_length_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //end_length_text->SetForegroundColour(wxColour("#000000"));
    m_tiEnd               = new TextInput(this, std::to_string(300), _L("mm/s"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    end_length_sizer->Add(end_length_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    end_length_sizer->Add(m_tiEnd, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(end_length_sizer, 0, wxLEFT | wxRIGHT, 10);

    // length step
    auto length_step_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto length_step_text  = new wxStaticText(this, wxID_ANY, length_step_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //length_step_text->SetForegroundColour(wxColour("#000000"));
    m_tiStep               = new TextInput(this, wxString::FromDouble(10), _L("mm/s"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    length_step_sizer->Add(length_step_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    length_step_sizer->Add(m_tiStep, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(length_step_sizer, 0, wxLEFT | wxRIGHT, 10);

    // creality add bed temp
    auto bed_temp_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto bed_temp_text  = new wxStaticText(this, wxID_ANY, bed_temp_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
   // bed_temp_text->SetForegroundColour(wxColour("#000000"));
    m_tiBedTemp         = new TextInput(this, wxString::FromDouble(50), _L("℃"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    bed_temp_sizer->Add(bed_temp_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    bed_temp_sizer->Add(m_tiBedTemp, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(bed_temp_sizer, 0, wxLEFT | wxRIGHT, 10);

    //// creality add acc speed
    //auto acc_speed_sizer = new wxBoxSizer(wxHORIZONTAL);
    //auto acc_speed_text  = new wxStaticText(this, wxID_ANY, acc_speed_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //m_tiAccSpeed         = new TextInput(this, wxString::FromDouble(5000), _L("mm/s²"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    //m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    //acc_speed_sizer->Add(acc_speed_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    //acc_speed_sizer->Add(m_tiAccSpeed, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    //settings_sizer->Add(acc_speed_sizer);

    v_sizer->Add(settings_sizer);
    v_sizer->Add(0, FromDIP(10), 0, wxEXPAND, 5);
    m_btnStart = new Button(this, _L("OK"));
    bool is_dark = wxGetApp().dark_mode();
    StateColor btn_bg_gray(std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(150, 150, 155), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Normal));

    StateColor btn_bg_light_mode(std::pair<wxColour, int>(wxColour(246, 246, 249), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(246, 246, 249), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Normal));
    m_btnStart->SetBackgroundColor(is_dark ? btn_bg_gray : btn_bg_light_mode);
    m_btnStart->SetBorderColor(is_dark ? wxColour(0, 0, 0) : wxColour(214, 214, 220));
    m_btnStart->SetTextColor(is_dark ? wxColour(38, 46, 48) : wxColour(51, 51, 51));
    m_btnStart->SetSize(wxSize(FromDIP(100), FromDIP(28)));
    m_btnStart->SetMinSize(wxSize(FromDIP(100), FromDIP(28)));
    m_btnStart->SetCornerRadius(FromDIP(12));
    m_btnStart->Bind(wxEVT_BUTTON, &Speed_Tower_Dlg::on_start, this);
    v_sizer->Add(m_btnStart, 0, wxALL | wxALIGN_CENTER, FromDIP(5));

    m_btnStart->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Speed_Tower_Dlg::on_start), NULL, this);
    this->Connect(wxEVT_SHOW, wxShowEventHandler(Speed_Tower_Dlg::on_show));
    wxGetApp().UpdateDlgDarkUI(this);

    Layout();
    Fit();
}

Speed_Tower_Dlg::~Speed_Tower_Dlg()
{
    // Disconnect Events
    m_btnStart->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Speed_Tower_Dlg::on_start), NULL, this);
}

	void Speed_Tower_Dlg::on_show(wxShowEvent& event) { update_params(); }

void Speed_Tower_Dlg::update_params() {
    m_bedTempValue                    = 50;
    auto          filament_config = &wxGetApp().preset_bundle->filaments.get_edited_preset().config;
    ConfigOption* bed_type_opt    = wxGetApp().preset_bundle->project_config.option("curr_bed_type");
    if (bed_type_opt != nullptr) {
        BedType atype = (BedType) bed_type_opt->getInt();
        if ((BedType) bed_type_opt->getInt() == BedType::btPTE) {
            ConfigOptionInts* textured_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("textured_plate_temp"));
            if (textured_plate_temp && !textured_plate_temp->empty()) {
                m_bedTempValue    = textured_plate_temp->values[0];
                m_params.bed_type = BedType::btPTE;
            }
        } else if ((BedType) bed_type_opt->getInt() == BedType::btDEF) {
            ConfigOptionInts* customized_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("customized_plate_temp"));
            if (customized_plate_temp && !customized_plate_temp->empty()) {
                m_bedTempValue    = customized_plate_temp->values[0];
                m_params.bed_type = BedType::btDEF;
            }
        } else if ((BedType) bed_type_opt->getInt() == BedType::btER) {
            ConfigOptionInts* epoxy_resin_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("epoxy_resin_plate_temp"));
            if (epoxy_resin_plate_temp && !epoxy_resin_plate_temp->empty()) {
                m_bedTempValue    = epoxy_resin_plate_temp->values[0];
                m_params.bed_type = BedType::btER;
            }
        } else {
            // BedType::btPEI
            ConfigOptionInts* hot_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("hot_plate_temp"));
            if (hot_plate_temp && !hot_plate_temp->empty()) {
                m_bedTempValue    = hot_plate_temp->values[0];
                m_params.bed_type = BedType::btPEI;
            }
        }
    }
    m_tiBedTemp->GetTextCtrl()->SetValue(wxString::FromDouble(m_bedTempValue));
}


void Speed_Tower_Dlg::on_start(wxCommandEvent& event)
{
    bool read_double = false;
    read_double      = m_tiStart->GetTextCtrl()->GetValue().ToDouble(&m_params.start);
    read_double      = read_double && m_tiEnd->GetTextCtrl()->GetValue().ToDouble(&m_params.end);
    read_double      = read_double && m_tiStep->GetTextCtrl()->GetValue().ToDouble(&m_params.step);
    read_double      = read_double && m_tiBedTemp->GetTextCtrl()->GetValue().ToDouble(&m_params.bed_temp);
    //read_double      = read_double && m_tiAccSpeed->GetTextCtrl()->GetValue().ToDouble(&m_params.acc_speed);

    if (!read_double || m_params.start <= 0 || m_params.step <= 0 || m_params.end < (m_params.start + m_params.step) ||
        m_params.bed_temp > 200 || m_params.bed_temp < 1) {
        MessageDialog msg_dlg(nullptr, _L("Please input valid values:\nstart > 0 \nstep > 0\nend >= start + step\nbed temp: >= 1\nbed temp: <= 200"), wxEmptyString,
                              wxICON_WARNING | wxOK);
        msg_dlg.ShowModal();
        return;
    }

    m_params.mode = CalibMode::Calib_Speed_Tower;
    EndModal(wxID_OK);
    m_plater->calib_speed_tower(m_params);

}

void Speed_Tower_Dlg::on_dpi_changed(const wxRect& suggested_rect)
{
    this->Refresh();
    Fit();
}



// Jitter_Speed_Dlg
//

Jitter_Speed_Dlg::Jitter_Speed_Dlg(wxWindow* parent, wxWindowID id, Plater* plater)
    : DPIDialog(parent,
                id,
                _L("Jitter speed test"),
                wxDefaultPosition,
                parent->FromDIP(wxSize(-1, 280)),
                wxDEFAULT_DIALOG_STYLE)
    , m_plater(plater)
{
    SetFont(wxGetApp().normal_font());
    SetBackgroundColour(*wxWHITE);
    SetMaxSize(wxSize(FromDIP(600), FromDIP(230)));
    wxBoxSizer* v_sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(v_sizer);

    // Settings
    //
    wxString start_length_str = _L("Start speed: ");
    wxString end_length_str   = _L("End speed: ");
    wxString length_step_str  = _L("step: ");
    wxString bed_temp_str     = _L("Hot Plate Template: ");
    // wxString acc_speed_str    = _L("Accelerated speed: ");
    auto text_size = wxWindow::GetTextExtent(start_length_str);
    text_size.IncTo(wxWindow::GetTextExtent(end_length_str));
    text_size.IncTo(wxWindow::GetTextExtent(length_step_str));
    text_size.IncTo(wxWindow::GetTextExtent(bed_temp_str));
    // text_size.IncTo(wxWindow::GetTextExtent(acc_speed_str));
    text_size.x                      = text_size.x * 4.0;
    wxBoxSizer* settings_sizer = new wxBoxSizer(wxVERTICAL);

    auto st_size = FromDIP(wxSize(430, -1));
    auto ti_size = FromDIP(wxSize(100, -1));
    // start length
    auto start_length_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto start_length_text  = new wxStaticText(this, wxID_ANY, start_length_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //start_length_text->SetForegroundColour(wxColour("#000000"));
    m_tiStart               = new TextInput(this, std::to_string(5), _L("mm/s"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));

    start_length_sizer->Add(start_length_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    start_length_sizer->Add(m_tiStart, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(start_length_sizer, 0,wxTOP | wxLEFT | wxRIGHT, 10);

    // end length
    auto end_length_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto end_length_text  = new wxStaticText(this, wxID_ANY, end_length_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //end_length_text->SetForegroundColour(wxColour("#000000"));
    m_tiEnd               = new TextInput(this, std::to_string(15), _L("mm/s"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    end_length_sizer->Add(end_length_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    end_length_sizer->Add(m_tiEnd, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(end_length_sizer, 0, wxLEFT | wxRIGHT, 10);

    // length step
    auto length_step_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto length_step_text  = new wxStaticText(this, wxID_ANY, length_step_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //length_step_text->SetForegroundColour(wxColour("#000000"));
    m_tiStep               = new TextInput(this, wxString::FromDouble(1), _L("mm/s"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    length_step_sizer->Add(length_step_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    length_step_sizer->Add(m_tiStep, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(length_step_sizer, 0, wxLEFT | wxRIGHT, 10);

    // creality add bed temp
    auto bed_temp_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto bed_temp_text  = new wxStaticText(this, wxID_ANY, bed_temp_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //bed_temp_text->SetForegroundColour(wxColour("#000000"));
    m_tiBedTemp         = new TextInput(this, wxString::FromDouble(50), _L("℃"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    bed_temp_sizer->Add(bed_temp_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    bed_temp_sizer->Add(m_tiBedTemp, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(bed_temp_sizer, 0, wxLEFT | wxRIGHT, 10);

    //// creality add acc speed
    // auto acc_speed_sizer = new wxBoxSizer(wxHORIZONTAL);
    // auto acc_speed_text  = new wxStaticText(this, wxID_ANY, acc_speed_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    // m_tiAccSpeed         = new TextInput(this, wxString::FromDouble(5000), _L("mm/s²"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    // m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    // acc_speed_sizer->Add(acc_speed_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    // acc_speed_sizer->Add(m_tiAccSpeed, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    // settings_sizer->Add(acc_speed_sizer);

    v_sizer->Add(settings_sizer);
    v_sizer->Add(0, FromDIP(10), 0, wxEXPAND, 5);
    m_btnStart = new Button(this, _L("OK"));
    bool is_dark = wxGetApp().dark_mode();
    StateColor btn_bg_gray(std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(150, 150, 155), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Normal));

    StateColor btn_bg_light_mode(std::pair<wxColour, int>(wxColour(246, 246, 249), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(246, 246, 249), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Normal));
    m_btnStart->SetBackgroundColor(is_dark ? btn_bg_gray : btn_bg_light_mode);
    m_btnStart->SetBorderColor(is_dark ? wxColour(0, 0, 0) : wxColour(214, 214, 220));
    m_btnStart->SetTextColor(is_dark ? wxColour(38, 46, 48) : wxColour(51, 51, 51));
    m_btnStart->SetSize(wxSize(FromDIP(100), FromDIP(28)));
    m_btnStart->SetMinSize(wxSize(FromDIP(100), FromDIP(28)));
    m_btnStart->SetCornerRadius(FromDIP(12));
    m_btnStart->Bind(wxEVT_BUTTON, &Jitter_Speed_Dlg::on_start, this);
    v_sizer->Add(m_btnStart, 0, wxALL | wxALIGN_CENTER, FromDIP(5));

    m_btnStart->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Jitter_Speed_Dlg::on_start), NULL, this);
    this->Connect(wxEVT_SHOW, wxShowEventHandler(Jitter_Speed_Dlg::on_show));
    wxGetApp().UpdateDlgDarkUI(this);

    Layout();
    Fit();
}

Jitter_Speed_Dlg::~Jitter_Speed_Dlg()
{
    // Disconnect Events
    m_btnStart->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Jitter_Speed_Dlg::on_start), NULL, this);
}

	void Jitter_Speed_Dlg::on_show(wxShowEvent& event) { update_params(); }

void Jitter_Speed_Dlg::update_params()
    {
    m_bedTempValue                    = 50;
        auto          filament_config = &wxGetApp().preset_bundle->filaments.get_edited_preset().config;
        ConfigOption* bed_type_opt    = wxGetApp().preset_bundle->project_config.option("curr_bed_type");
        if (bed_type_opt != nullptr) {
            BedType atype = (BedType) bed_type_opt->getInt();
            if ((BedType) bed_type_opt->getInt() == BedType::btPTE) {
                ConfigOptionInts* textured_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("textured_plate_temp"));
                if (textured_plate_temp && !textured_plate_temp->empty()) {
                    m_bedTempValue    = textured_plate_temp->values[0];
                    m_params.bed_type = BedType::btPTE;
                }
            } else if ((BedType) bed_type_opt->getInt() == BedType::btDEF) {
                ConfigOptionInts* customized_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("customized_plate_temp"));
                if (customized_plate_temp && !customized_plate_temp->empty()) {
                    m_bedTempValue    = customized_plate_temp->values[0];
                    m_params.bed_type = BedType::btDEF;
                }
            } else if ((BedType) bed_type_opt->getInt() == BedType::btER) {
                ConfigOptionInts* epoxy_resin_plate_temp = dynamic_cast<ConfigOptionInts*>(
                    filament_config->option("epoxy_resin_plate_temp"));
                if (epoxy_resin_plate_temp && !epoxy_resin_plate_temp->empty()) {
                    m_bedTempValue    = epoxy_resin_plate_temp->values[0];
                    m_params.bed_type = BedType::btER;
                }
            } else {
                // BedType::btPEI
                ConfigOptionInts* hot_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("hot_plate_temp"));
                if (hot_plate_temp && !hot_plate_temp->empty()) {
                    m_bedTempValue    = hot_plate_temp->values[0];
                    m_params.bed_type = BedType::btPEI;
                }
            }
        }
    m_tiBedTemp->GetTextCtrl()->SetValue(wxString::FromDouble(m_bedTempValue));
}


void Jitter_Speed_Dlg::on_start(wxCommandEvent& event)
{
    bool read_double = false;
    read_double      = m_tiStart->GetTextCtrl()->GetValue().ToDouble(&m_params.start);
    read_double      = read_double && m_tiEnd->GetTextCtrl()->GetValue().ToDouble(&m_params.end);
    read_double      = read_double && m_tiStep->GetTextCtrl()->GetValue().ToDouble(&m_params.step);
    read_double      = read_double && m_tiBedTemp->GetTextCtrl()->GetValue().ToDouble(&m_params.bed_temp);
    // read_double      = read_double && m_tiAccSpeed->GetTextCtrl()->GetValue().ToDouble(&m_params.acc_speed);

    if (!read_double || m_params.start <= 0 || m_params.step <= 0 || m_params.end < (m_params.start + m_params.step) ||
        m_params.bed_temp > 200 || m_params.bed_temp < 1) {
        MessageDialog msg_dlg(nullptr, _L("Please input valid values:\nstart > 0 \nstep > 0\nend >= start + step\nbed temp: >= 1\nbed temp: <= 200"), wxEmptyString,
                              wxICON_WARNING | wxOK);
        msg_dlg.ShowModal();
        return;
    }

    m_params.mode = CalibMode::Calib_X_Y_Jerk;
    EndModal(wxID_OK);
    m_plater->calib_jitter_speed(m_params);

}

void Jitter_Speed_Dlg::on_dpi_changed(const wxRect& suggested_rect)
{
    this->Refresh();
    Fit();
}


// Fan_Speed_Dlg
//

Fan_Speed_Dlg::Fan_Speed_Dlg(wxWindow* parent, wxWindowID id, Plater* plater)
    : DPIDialog(parent,
                id,
                _L("Fan speed test"),
                wxDefaultPosition,
                parent->FromDIP(wxSize(-1, 280)),
                wxDEFAULT_DIALOG_STYLE)
    , m_plater(plater)
{
    SetFont(wxGetApp().normal_font());
    SetBackgroundColour(*wxWHITE);
    SetMaxSize(wxSize(FromDIP(600), FromDIP(230)));
    wxBoxSizer* v_sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(v_sizer);

    // Settings
    //
    wxString start_length_str = _L("Start speed: ");
    wxString end_length_str   = _L("End speed: ");
    wxString length_step_str  = _L("step: ");
    wxString bed_temp_str     = _L("Hot Plate Template: ");
    // wxString acc_speed_str    = _L("Accelerated speed: ");
    auto text_size = wxWindow::GetTextExtent(start_length_str);
    text_size.IncTo(wxWindow::GetTextExtent(end_length_str));
    text_size.IncTo(wxWindow::GetTextExtent(length_step_str));
    text_size.IncTo(wxWindow::GetTextExtent(bed_temp_str));
    // text_size.IncTo(wxWindow::GetTextExtent(acc_speed_str));
    text_size.x                      = text_size.x * 4.0;
    wxBoxSizer* settings_sizer = new wxBoxSizer(wxVERTICAL);

    auto st_size = FromDIP(wxSize(430, -1));
    auto ti_size = FromDIP(wxSize(100, -1));
    // start length
    auto start_length_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto start_length_text  = new wxStaticText(this, wxID_ANY, start_length_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //start_length_text->SetForegroundColour(wxColour("#000000"));
    m_tiStart               = new TextInput(this, std::to_string(0), _L("%"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));

    start_length_sizer->Add(start_length_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    start_length_sizer->Add(m_tiStart, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(start_length_sizer, 0, wxTOP | wxLEFT | wxRIGHT, 10);

    // end length
    auto end_length_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto end_length_text  = new wxStaticText(this, wxID_ANY, end_length_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //end_length_text->SetForegroundColour(wxColour("#000000"));
    m_tiEnd               = new TextInput(this, std::to_string(100), _L("%"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    end_length_sizer->Add(end_length_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    end_length_sizer->Add(m_tiEnd, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(end_length_sizer, 0, wxLEFT | wxRIGHT, 10);

    // length step
    auto length_step_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto length_step_text  = new wxStaticText(this, wxID_ANY, length_step_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //length_step_text->SetForegroundColour(wxColour("#000000"));
    m_tiStep               = new TextInput(this, wxString::FromDouble(10), _L("%"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    length_step_sizer->Add(length_step_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    length_step_sizer->Add(m_tiStep, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(length_step_sizer, 0, wxLEFT | wxRIGHT, 10);

    // creality add bed temp
    auto bed_temp_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto bed_temp_text  = new wxStaticText(this, wxID_ANY, bed_temp_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //bed_temp_text->SetForegroundColour(wxColour("#000000"));
    m_tiBedTemp         = new TextInput(this, wxString::FromDouble(50), _L("℃"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    bed_temp_sizer->Add(bed_temp_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    bed_temp_sizer->Add(m_tiBedTemp, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(bed_temp_sizer, 0, wxLEFT | wxRIGHT, 10);

    //// creality add acc speed
    // auto acc_speed_sizer = new wxBoxSizer(wxHORIZONTAL);
    // auto acc_speed_text  = new wxStaticText(this, wxID_ANY, acc_speed_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    // m_tiAccSpeed         = new TextInput(this, wxString::FromDouble(5000), _L("mm/s²"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    // m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    // acc_speed_sizer->Add(acc_speed_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    // acc_speed_sizer->Add(m_tiAccSpeed, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    // settings_sizer->Add(acc_speed_sizer);

    v_sizer->Add(settings_sizer);
    v_sizer->Add(0, FromDIP(10), 0, wxEXPAND, 5);
    m_btnStart = new Button(this, _L("OK"));
    bool is_dark = wxGetApp().dark_mode();
    StateColor btn_bg_gray(std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(150, 150, 155), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Normal));

    StateColor btn_bg_light_mode(std::pair<wxColour, int>(wxColour(246, 246, 249), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(246, 246, 249), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Normal));
    m_btnStart->SetBackgroundColor(is_dark ? btn_bg_gray : btn_bg_light_mode);
    m_btnStart->SetBorderColor(is_dark ? wxColour(0, 0, 0) : wxColour(214, 214, 220));
    m_btnStart->SetTextColor(is_dark ? wxColour(38, 46, 48) : wxColour(51, 51, 51));
    m_btnStart->SetSize(wxSize(FromDIP(100), FromDIP(28)));
    m_btnStart->SetMinSize(wxSize(FromDIP(100), FromDIP(28)));
    m_btnStart->SetCornerRadius(FromDIP(12));
    m_btnStart->Bind(wxEVT_BUTTON, &Fan_Speed_Dlg::on_start, this);
    v_sizer->Add(m_btnStart, 0, wxALL | wxALIGN_CENTER, FromDIP(5));

    m_btnStart->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Fan_Speed_Dlg::on_start), NULL, this);
    this->Connect(wxEVT_SHOW, wxShowEventHandler(Fan_Speed_Dlg::on_show));
    wxGetApp().UpdateDlgDarkUI(this);

    Layout();
    Fit();
}

Fan_Speed_Dlg::~Fan_Speed_Dlg()
{
    // Disconnect Events
    m_btnStart->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Fan_Speed_Dlg::on_start), NULL, this);
}

	void Fan_Speed_Dlg::on_show(wxShowEvent& event) { update_params(); }

void Fan_Speed_Dlg::update_params()
    {
    m_bedTempValue                    = 50;
        auto          filament_config = &wxGetApp().preset_bundle->filaments.get_edited_preset().config;
        ConfigOption* bed_type_opt    = wxGetApp().preset_bundle->project_config.option("curr_bed_type");
        if (bed_type_opt != nullptr) {
            BedType atype = (BedType) bed_type_opt->getInt();
            if ((BedType) bed_type_opt->getInt() == BedType::btPTE) {
                ConfigOptionInts* textured_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("textured_plate_temp"));
                if (textured_plate_temp && !textured_plate_temp->empty()) {
                    m_bedTempValue    = textured_plate_temp->values[0];
                    m_params.bed_type = BedType::btPTE;
                }
            } else if ((BedType) bed_type_opt->getInt() == BedType::btDEF) {
                ConfigOptionInts* customized_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("customized_plate_temp"));
                if (customized_plate_temp && !customized_plate_temp->empty()) {
                    m_bedTempValue    = customized_plate_temp->values[0];
                    m_params.bed_type = BedType::btDEF;
                }
            } else if ((BedType) bed_type_opt->getInt() == BedType::btER) {
                ConfigOptionInts* epoxy_resin_plate_temp = dynamic_cast<ConfigOptionInts*>(
                    filament_config->option("epoxy_resin_plate_temp"));
                if (epoxy_resin_plate_temp && !epoxy_resin_plate_temp->empty()) {
                    m_bedTempValue    = epoxy_resin_plate_temp->values[0];
                    m_params.bed_type = BedType::btER;
                }
            } else {
                // BedType::btPEI
                ConfigOptionInts* hot_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("hot_plate_temp"));
                if (hot_plate_temp && !hot_plate_temp->empty()) {
                    m_bedTempValue    = hot_plate_temp->values[0];
                    m_params.bed_type = BedType::btPEI;
                }
            }
        }
    m_tiBedTemp->GetTextCtrl()->SetValue(wxString::FromDouble(m_bedTempValue));
}



void Fan_Speed_Dlg::on_start(wxCommandEvent& event)
{
    bool read_double = false;
    read_double      = m_tiStart->GetTextCtrl()->GetValue().ToDouble(&m_params.start);
    read_double      = read_double && m_tiEnd->GetTextCtrl()->GetValue().ToDouble(&m_params.end);
    read_double      = read_double && m_tiStep->GetTextCtrl()->GetValue().ToDouble(&m_params.step);
    read_double      = read_double && m_tiBedTemp->GetTextCtrl()->GetValue().ToDouble(&m_params.bed_temp);
    // read_double      = read_double && m_tiAccSpeed->GetTextCtrl()->GetValue().ToDouble(&m_params.acc_speed);

    if (!read_double || m_params.start < 0 || m_params.step <= 0 || m_params.end < (m_params.start + m_params.step) || 
        m_params.end >100 || m_params.bed_temp > 200 || m_params.bed_temp < 1) {
        MessageDialog msg_dlg(nullptr, _L("Please input valid values:\nstart >= 0 \nstep > 0\nend >= start + step\nend <= 100\nbed temp: >= 1\nbed temp: <= 200"), wxEmptyString,
                              wxICON_WARNING | wxOK);
        msg_dlg.ShowModal();
        return;
    }

    m_params.mode = CalibMode::Calib_Fan_Speed;
    EndModal(wxID_OK);
    m_plater->calib_fan_speed(m_params);

}

void Fan_Speed_Dlg::on_dpi_changed(const wxRect& suggested_rect)
{
    this->Refresh();
    Fit();
}



// Limit_Acceleration_Dlg
//

Limit_Acceleration_Dlg::Limit_Acceleration_Dlg(wxWindow* parent, wxWindowID id, Plater* plater)
    : DPIDialog(parent,
                id,
                _L("Limit acceleration test"),
                wxDefaultPosition,
                parent->FromDIP(wxSize(-1, 280)),
                wxDEFAULT_DIALOG_STYLE)
    , m_plater(plater)
{
    SetFont(wxGetApp().normal_font());
    SetBackgroundColour(*wxWHITE);
    SetMaxSize(wxSize(FromDIP(600), FromDIP(260)));
    wxBoxSizer* v_sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(v_sizer);

    // Settings
    //
    wxString start_length_str = _L("Start acceleration: ");
    wxString end_length_str   = _L("End acceleration: ");
    wxString length_step_str  = _L("step: ");
    wxString print_speed_str    = _L("print speed: ");
    wxString bed_temp_str     = _L("Hot Plate Template: ");
    auto     text_size        = wxWindow::GetTextExtent(start_length_str);
    text_size.IncTo(wxWindow::GetTextExtent(end_length_str));
    text_size.IncTo(wxWindow::GetTextExtent(length_step_str));
    text_size.IncTo(wxWindow::GetTextExtent(print_speed_str));
    text_size.IncTo(wxWindow::GetTextExtent(bed_temp_str));
    text_size.x                      = text_size.x * 4.0;
    wxBoxSizer* settings_sizer = new wxBoxSizer(wxVERTICAL);

    auto st_size = FromDIP(wxSize(430, -1));
    auto ti_size = FromDIP(wxSize(100, -1));
    // start length
    auto start_length_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto start_length_text  = new wxStaticText(this, wxID_ANY, start_length_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //start_length_text->SetForegroundColour(wxColour("#000000"));
    m_tiStart               = new TextInput(this, std::to_string(500), _L("mm/s²"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));

    start_length_sizer->Add(start_length_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    start_length_sizer->Add(m_tiStart, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(start_length_sizer, 0,wxTOP | wxLEFT | wxRIGHT, 10);

    // end length
    auto end_length_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto end_length_text  = new wxStaticText(this, wxID_ANY, end_length_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //end_length_text->SetForegroundColour(wxColour("#000000"));
    m_tiEnd               = new TextInput(this, std::to_string(30000), _L("mm/s²"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    end_length_sizer->Add(end_length_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    end_length_sizer->Add(m_tiEnd, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(end_length_sizer, 0, wxLEFT | wxRIGHT, 10);

    // length step
    auto length_step_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto length_step_text  = new wxStaticText(this, wxID_ANY, length_step_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //length_step_text->SetForegroundColour(wxColour("#000000"));
    m_tiStep               = new TextInput(this, wxString::FromDouble(1000), _L("mm/s²"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    length_step_sizer->Add(length_step_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    length_step_sizer->Add(m_tiStep, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(length_step_sizer, 0, wxLEFT | wxRIGHT, 10);

    // creality add print speed
    auto print_speed_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto print_speed_text  = new wxStaticText(this, wxID_ANY, print_speed_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //print_speed_text->SetForegroundColour(wxColour("#000000"));
    m_tiSpeed         = new TextInput(this, wxString::FromDouble(300), _L("mm/s"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    print_speed_sizer->Add(print_speed_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    print_speed_sizer->Add(m_tiSpeed, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(print_speed_sizer, 0, wxLEFT | wxRIGHT, 10);

    // creality add bed temp
    auto bed_temp_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto bed_temp_text  = new wxStaticText(this, wxID_ANY, bed_temp_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //bed_temp_text->SetForegroundColour(wxColour("#000000"));
    m_tiBedTemp         = new TextInput(this, wxString::FromDouble(50), _L("℃"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    bed_temp_sizer->Add(bed_temp_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    bed_temp_sizer->Add(m_tiBedTemp, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(bed_temp_sizer, 0, wxLEFT | wxRIGHT, 10);

    v_sizer->Add(settings_sizer, 0, wxLEFT | wxRIGHT, 10);
    v_sizer->Add(0, FromDIP(10), 0, wxEXPAND, 5);
    m_btnStart = new Button(this, _L("OK"));
    bool is_dark = wxGetApp().dark_mode();
    StateColor btn_bg_gray(std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(150, 150, 155), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Normal));

    StateColor btn_bg_light_mode(std::pair<wxColour, int>(wxColour(246, 246, 249), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(246, 246, 249), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Normal));
    m_btnStart->SetBackgroundColor(is_dark ? btn_bg_gray : btn_bg_light_mode);
    m_btnStart->SetBorderColor(is_dark ? wxColour(0, 0, 0) : wxColour(214, 214, 220));
    m_btnStart->SetTextColor(is_dark ? wxColour(38, 46, 48) : wxColour(51, 51, 51));
    m_btnStart->SetSize(wxSize(FromDIP(100), FromDIP(28)));
    m_btnStart->SetMinSize(wxSize(FromDIP(100), FromDIP(28)));
    m_btnStart->SetCornerRadius(FromDIP(12));
    m_btnStart->Bind(wxEVT_BUTTON, &Limit_Acceleration_Dlg::on_start, this);
    v_sizer->Add(m_btnStart, 0, wxALL | wxALIGN_CENTER, FromDIP(5));

    m_btnStart->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Limit_Acceleration_Dlg::on_start), NULL, this);
    this->Connect(wxEVT_SHOW, wxShowEventHandler(Limit_Acceleration_Dlg::on_show));
    wxGetApp().UpdateDlgDarkUI(this);

    Layout();
    Fit();
}

Limit_Acceleration_Dlg::~Limit_Acceleration_Dlg()
{
    // Disconnect Events
    m_btnStart->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Limit_Acceleration_Dlg::on_start), NULL, this);
}


void Limit_Acceleration_Dlg::on_show(wxShowEvent& event) { update_params(); }

void Limit_Acceleration_Dlg::update_params()
    {
    m_bedTempValue                    = 50;
        auto          filament_config = &wxGetApp().preset_bundle->filaments.get_edited_preset().config;
        ConfigOption* bed_type_opt    = wxGetApp().preset_bundle->project_config.option("curr_bed_type");
        if (bed_type_opt != nullptr) {
            BedType atype = (BedType) bed_type_opt->getInt();
            if ((BedType) bed_type_opt->getInt() == BedType::btPTE) {
                ConfigOptionInts* textured_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("textured_plate_temp"));
                if (textured_plate_temp && !textured_plate_temp->empty()) {
                    m_bedTempValue    = textured_plate_temp->values[0];
                    m_params.bed_type = BedType::btPTE;
                }
            } else if ((BedType) bed_type_opt->getInt() == BedType::btDEF) {
                ConfigOptionInts* customized_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("customized_plate_temp"));
                if (customized_plate_temp && !customized_plate_temp->empty()) {
                    m_bedTempValue    = customized_plate_temp->values[0];
                    m_params.bed_type = BedType::btDEF;
                }
            } else if ((BedType) bed_type_opt->getInt() == BedType::btER) {
                ConfigOptionInts* epoxy_resin_plate_temp = dynamic_cast<ConfigOptionInts*>(
                    filament_config->option("epoxy_resin_plate_temp"));
                if (epoxy_resin_plate_temp && !epoxy_resin_plate_temp->empty()) {
                    m_bedTempValue    = epoxy_resin_plate_temp->values[0];
                    m_params.bed_type = BedType::btER;
                }
            } else {
                // BedType::btPEI
                ConfigOptionInts* hot_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("hot_plate_temp"));
                if (hot_plate_temp && !hot_plate_temp->empty()) {
                    m_bedTempValue    = hot_plate_temp->values[0];
                    m_params.bed_type = BedType::btPEI;
                }
            }
        }
    m_tiBedTemp->GetTextCtrl()->SetValue(wxString::FromDouble(m_bedTempValue));
}

void Limit_Acceleration_Dlg::on_start(wxCommandEvent& event)
{
    bool read_double = false;
    read_double      = m_tiStart->GetTextCtrl()->GetValue().ToDouble(&m_params.start);
    read_double      = read_double && m_tiEnd->GetTextCtrl()->GetValue().ToDouble(&m_params.end);
    read_double      = read_double && m_tiStep->GetTextCtrl()->GetValue().ToDouble(&m_params.step);
    read_double      = read_double && m_tiSpeed->GetTextCtrl()->GetValue().ToDouble(&m_params.print_speed);
    read_double      = read_double && m_tiBedTemp->GetTextCtrl()->GetValue().ToDouble(&m_params.bed_temp);

    if (!read_double || m_params.start <= 0 || m_params.step <= 0 || m_params.end < (m_params.start + m_params.step) ||
        m_params.bed_temp > 200 || m_params.bed_temp < 1) {
        MessageDialog msg_dlg(nullptr, _L("Please input valid values:\nstart > 0 \nstep > 0\nend >= start + step\nbed temp: >= 1\nbed temp: <= 200"), wxEmptyString,
                              wxICON_WARNING | wxOK);
        msg_dlg.ShowModal();
        return;
    }

    m_params.mode = CalibMode::Calib_Limit_Acceleration;
    EndModal(wxID_OK);
    m_plater->calib_limit_acc(m_params);

}

void Limit_Acceleration_Dlg::on_dpi_changed(const wxRect& suggested_rect)
{
    this->Refresh();
    Fit();
}



// Acceleration_Tower_Dlg
//

Acceleration_Tower_Dlg::Acceleration_Tower_Dlg(wxWindow* parent, wxWindowID id, Plater* plater)
    : DPIDialog(parent,
                id,
                _L("Acceleration tower test"),
                wxDefaultPosition,
                parent->FromDIP(wxSize(-1, 280)),
                wxDEFAULT_DIALOG_STYLE)
    , m_plater(plater)
{
    SetFont(wxGetApp().normal_font());
    SetBackgroundColour(*wxWHITE);
    SetMaxSize(wxSize(FromDIP(600), FromDIP(260)));
    wxBoxSizer* v_sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(v_sizer);

    // Settings
    //
    wxString start_length_str = _L("Start acceleration: ");
    wxString end_length_str   = _L("End acceleration: ");
    wxString length_step_str  = _L("step: ");
    //wxString print_speed_str  = _L("print speed: ");
    wxString bed_temp_str     = _L("Hot Plate Template: ");
    auto     text_size        = wxWindow::GetTextExtent(start_length_str);
    text_size.IncTo(wxWindow::GetTextExtent(end_length_str));
    text_size.IncTo(wxWindow::GetTextExtent(length_step_str));
    //text_size.IncTo(wxWindow::GetTextExtent(print_speed_str));
    text_size.IncTo(wxWindow::GetTextExtent(bed_temp_str));
    text_size.x                      = text_size.x * 4.0;
    wxBoxSizer* settings_sizer = new wxBoxSizer(wxVERTICAL);

    auto st_size = FromDIP(wxSize(430, -1));
    auto ti_size = FromDIP(wxSize(100, -1));
    // start length
    auto start_length_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto start_length_text  = new wxStaticText(this, wxID_ANY, start_length_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //start_length_text->SetForegroundColour(wxColour("#000000"));
    m_tiStart               = new TextInput(this, std::to_string(1000), _L("mm/s²"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));

    start_length_sizer->Add(start_length_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    start_length_sizer->Add(m_tiStart, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(start_length_sizer, 0, wxTOP | wxLEFT | wxRIGHT, 10);

    // end length
    auto end_length_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto end_length_text  = new wxStaticText(this, wxID_ANY, end_length_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //end_length_text->SetForegroundColour(wxColour("#000000"));
    m_tiEnd               = new TextInput(this, std::to_string(12000), _L("mm/s²"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    end_length_sizer->Add(end_length_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    end_length_sizer->Add(m_tiEnd, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(end_length_sizer, 0, wxLEFT | wxRIGHT, 10);

    // length step
    auto length_step_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto length_step_text  = new wxStaticText(this, wxID_ANY, length_step_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //length_step_text->SetForegroundColour(wxColour("#000000"));
    m_tiStep               = new TextInput(this, wxString::FromDouble(1000), _L("mm/s²"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    length_step_sizer->Add(length_step_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    length_step_sizer->Add(m_tiStep, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(length_step_sizer, 0, wxLEFT | wxRIGHT, 10);

    //// creality add print speed
    //auto print_speed_sizer = new wxBoxSizer(wxHORIZONTAL);
    //auto print_speed_text  = new wxStaticText(this, wxID_ANY, print_speed_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //m_tiSpeed              = new TextInput(this, wxString::FromDouble(300), _L("mm/s"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    //m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    //print_speed_sizer->Add(print_speed_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    //print_speed_sizer->Add(m_tiSpeed, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    //settings_sizer->Add(print_speed_sizer);

    // creality add bed temp
    auto bed_temp_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto bed_temp_text  = new wxStaticText(this, wxID_ANY, bed_temp_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //bed_temp_text->SetForegroundColour(wxColour("#000000"));
    m_tiBedTemp         = new TextInput(this, wxString::FromDouble(50), _L("℃"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    bed_temp_sizer->Add(bed_temp_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    bed_temp_sizer->Add(m_tiBedTemp, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(bed_temp_sizer, 0, wxLEFT | wxRIGHT, 10);

    v_sizer->Add(settings_sizer, 0, wxLEFT | wxRIGHT, 10);
    v_sizer->Add(0, FromDIP(10), 0, wxEXPAND, 5);
    m_btnStart = new Button(this, _L("OK"));
    bool is_dark = wxGetApp().dark_mode();
    StateColor btn_bg_gray(std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(150, 150, 155), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Normal));

    StateColor btn_bg_light_mode(std::pair<wxColour, int>(wxColour(246, 246, 249), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(246, 246, 249), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Normal));
    m_btnStart->SetBackgroundColor(is_dark ? btn_bg_gray : btn_bg_light_mode);
    m_btnStart->SetBorderColor(is_dark ? wxColour(0, 0, 0) : wxColour(214, 214, 220));
    m_btnStart->SetTextColor(is_dark ? wxColour(38, 46, 48) : wxColour(51, 51, 51));
    m_btnStart->SetSize(wxSize(FromDIP(100), FromDIP(28)));
    m_btnStart->SetMinSize(wxSize(FromDIP(100), FromDIP(28)));
    m_btnStart->SetCornerRadius(FromDIP(12));
    m_btnStart->Bind(wxEVT_BUTTON, &Acceleration_Tower_Dlg::on_start, this);
    v_sizer->Add(m_btnStart, 0, wxALL | wxALIGN_CENTER, FromDIP(5));

    m_btnStart->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Acceleration_Tower_Dlg::on_start), NULL, this);
    this->Connect(wxEVT_SHOW, wxShowEventHandler(Acceleration_Tower_Dlg::on_show));
    wxGetApp().UpdateDlgDarkUI(this);

    Layout();
    Fit();
}

Acceleration_Tower_Dlg::~Acceleration_Tower_Dlg()
{
    // Disconnect Events
    m_btnStart->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Acceleration_Tower_Dlg::on_start), NULL, this);
}


	void Acceleration_Tower_Dlg::on_show(wxShowEvent& event) { update_params(); }

void Acceleration_Tower_Dlg::update_params()
    {
    m_bedTempValue                    = 50;
        auto          filament_config = &wxGetApp().preset_bundle->filaments.get_edited_preset().config;
        ConfigOption* bed_type_opt    = wxGetApp().preset_bundle->project_config.option("curr_bed_type");
        if (bed_type_opt != nullptr) {
            BedType atype = (BedType) bed_type_opt->getInt();
            if ((BedType) bed_type_opt->getInt() == BedType::btPTE) {
                ConfigOptionInts* textured_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("textured_plate_temp"));
                if (textured_plate_temp && !textured_plate_temp->empty()) {
                    m_bedTempValue    = textured_plate_temp->values[0];
                    m_params.bed_type = BedType::btPTE;
                }
            } else if ((BedType) bed_type_opt->getInt() == BedType::btDEF) {
                ConfigOptionInts* customized_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("customized_plate_temp"));
                if (customized_plate_temp && !customized_plate_temp->empty()) {
                    m_bedTempValue    = customized_plate_temp->values[0];
                    m_params.bed_type = BedType::btDEF;
                }
            } else if ((BedType) bed_type_opt->getInt() == BedType::btER) {
                ConfigOptionInts* epoxy_resin_plate_temp = dynamic_cast<ConfigOptionInts*>(
                    filament_config->option("epoxy_resin_plate_temp"));
                if (epoxy_resin_plate_temp && !epoxy_resin_plate_temp->empty()) {
                    m_bedTempValue    = epoxy_resin_plate_temp->values[0];
                    m_params.bed_type = BedType::btER;
                }
            } else {
                // BedType::btPEI
                ConfigOptionInts* hot_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("hot_plate_temp"));
                if (hot_plate_temp && !hot_plate_temp->empty()) {
                    m_bedTempValue    = hot_plate_temp->values[0];
                    m_params.bed_type = BedType::btPEI;
                }
            }
        }
    m_tiBedTemp->GetTextCtrl()->SetValue(wxString::FromDouble(m_bedTempValue));
}




void Acceleration_Tower_Dlg::on_start(wxCommandEvent& event)
{
    bool read_double = false;
    read_double      = m_tiStart->GetTextCtrl()->GetValue().ToDouble(&m_params.start);
    read_double      = read_double && m_tiEnd->GetTextCtrl()->GetValue().ToDouble(&m_params.end);
    read_double      = read_double && m_tiStep->GetTextCtrl()->GetValue().ToDouble(&m_params.step);
    //read_double      = read_double && m_tiSpeed->GetTextCtrl()->GetValue().ToDouble(&m_params.print_speed);
    read_double      = read_double && m_tiBedTemp->GetTextCtrl()->GetValue().ToDouble(&m_params.bed_temp);

    if (!read_double || m_params.start <= 0 || m_params.step <= 0 || m_params.end < (m_params.start + m_params.step) ||
        m_params.bed_temp > 200 || m_params.bed_temp < 1) {
        MessageDialog msg_dlg(nullptr, _L("Please input valid values:\nstart > 0 \nstep > 0\nend >= start + step\nbed temp: >= 1\nbed temp: <= 200"), wxEmptyString,
                              wxICON_WARNING | wxOK);
        msg_dlg.ShowModal();
        return;
    }

    m_params.mode = CalibMode::Calib_Acceleration_Tower;
    EndModal(wxID_OK);
    m_plater->calib_acc_tower(m_params);

}

void Acceleration_Tower_Dlg::on_dpi_changed(const wxRect& suggested_rect)
{
    this->Refresh();
    Fit();
}






// Dec_Acceleration_Dlg
//

Dec_Acceleration_Dlg::Dec_Acceleration_Dlg(wxWindow* parent, wxWindowID id, Plater* plater)
    : DPIDialog(parent,
                id,
                _L("Dec Acceleration test"),
                wxDefaultPosition,
                parent->FromDIP(wxSize(-1, 280)),
                wxDEFAULT_DIALOG_STYLE)
    , m_plater(plater)
{
    SetFont(wxGetApp().normal_font());
    SetBackgroundColour(*wxWHITE);
    SetMaxSize(wxSize(FromDIP(600), FromDIP(260)));
    wxBoxSizer* v_sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(v_sizer);

    // Settings
    //
    wxString start_length_str = _L("Start acceleration: ");
    wxString end_length_str   = _L("End acceleration: ");
    wxString length_step_str  = _L("step: ");
    wxString high_step_str  = _L("high step: ");
    wxString bed_temp_str = _L("Hot Plate Template: ");
    auto     text_size    = wxWindow::GetTextExtent(start_length_str);
    text_size.IncTo(wxWindow::GetTextExtent(end_length_str));
    text_size.IncTo(wxWindow::GetTextExtent(length_step_str));
    text_size.IncTo(wxWindow::GetTextExtent(high_step_str));
    text_size.IncTo(wxWindow::GetTextExtent(bed_temp_str));
    text_size.x                      = text_size.x * 4.0;
    wxBoxSizer* settings_sizer = new wxBoxSizer(wxVERTICAL);

    auto st_size = FromDIP(wxSize(430, -1));
    auto ti_size = FromDIP(wxSize(100, -1));
    // start length
    auto start_length_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto start_length_text  = new wxStaticText(this, wxID_ANY, start_length_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //start_length_text->SetForegroundColour(wxColour("#000000"));
    m_tiStart               = new TextInput(this, std::to_string(100), _L("%"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));

    start_length_sizer->Add(start_length_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    start_length_sizer->Add(m_tiStart, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(start_length_sizer);

    // end length
    auto end_length_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto end_length_text  = new wxStaticText(this, wxID_ANY, end_length_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //end_length_text->SetForegroundColour(wxColour("#000000"));
    m_tiEnd               = new TextInput(this, std::to_string(10), _L("%"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    end_length_sizer->Add(end_length_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    end_length_sizer->Add(m_tiEnd, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(end_length_sizer);

    // length step
    auto length_step_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto length_step_text  = new wxStaticText(this, wxID_ANY, length_step_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    //length_step_text->SetForegroundColour(wxColour("#000000"));
    m_tiStep               = new TextInput(this, wxString::FromDouble(10), _L("%"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    length_step_sizer->Add(length_step_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    length_step_sizer->Add(m_tiStep, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(length_step_sizer);

    // creality add high step
     auto print_speed_sizer = new wxBoxSizer(wxHORIZONTAL);
     auto print_speed_text  = new wxStaticText(this, wxID_ANY, high_step_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
     //print_speed_text->SetForegroundColour(wxColour("#000000"));
     m_tiHstep            = new TextInput(this, wxString::FromDouble(10), _L("mm"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
     m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
     print_speed_sizer->Add(print_speed_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
     print_speed_sizer->Add(m_tiHstep, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
     settings_sizer->Add(print_speed_sizer);

    // creality add bed temp
    auto bed_temp_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto bed_temp_text  = new wxStaticText(this, wxID_ANY, bed_temp_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
   // bed_temp_text->SetForegroundColour(wxColour("#000000"));
    m_tiBedTemp         = new TextInput(this, wxString::FromDouble(50), _L("℃"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    bed_temp_sizer->Add(bed_temp_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    bed_temp_sizer->Add(m_tiBedTemp, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    settings_sizer->Add(bed_temp_sizer);

    v_sizer->Add(settings_sizer, 0, wxTOP | wxLEFT | wxRIGHT, 10);
    v_sizer->Add(0, FromDIP(10), 0, wxEXPAND, 5);
    m_btnStart = new Button(this, _L("OK"));
    bool is_dark = wxGetApp().dark_mode();
    StateColor btn_bg_gray(std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(150, 150, 155), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Normal));

    StateColor btn_bg_light_mode(std::pair<wxColour, int>(wxColour(246, 246, 249), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(246, 246, 249), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Normal));
    m_btnStart->SetBackgroundColor(is_dark ? btn_bg_gray : btn_bg_light_mode);
    m_btnStart->SetBorderColor(is_dark ? wxColour(0, 0, 0) : wxColour(214, 214, 220));
    m_btnStart->SetTextColor(is_dark ? wxColour(38, 46, 48) : wxColour(51, 51, 51));
    m_btnStart->SetSize(wxSize(FromDIP(100), FromDIP(28)));
    m_btnStart->SetMinSize(wxSize(FromDIP(100), FromDIP(28)));
    m_btnStart->SetCornerRadius(FromDIP(12));
    m_btnStart->Bind(wxEVT_BUTTON, &Dec_Acceleration_Dlg::on_start, this);
    v_sizer->Add(m_btnStart, 0, wxALL | wxALIGN_CENTER, FromDIP(5));
    m_btnStart->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Dec_Acceleration_Dlg::on_start), NULL, this);
    this->Connect(wxEVT_SHOW, wxShowEventHandler(Dec_Acceleration_Dlg::on_show));
    wxGetApp().UpdateDlgDarkUI(this);

    Layout();
    Fit();
}

Dec_Acceleration_Dlg::~Dec_Acceleration_Dlg()
{
    // Disconnect Events
    m_btnStart->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Dec_Acceleration_Dlg::on_start), NULL, this);
}


	void Dec_Acceleration_Dlg::on_show(wxShowEvent& event) { update_params(); }

void Dec_Acceleration_Dlg::update_params()
    {
    m_bedTempValue                    = 50;
        auto          filament_config = &wxGetApp().preset_bundle->filaments.get_edited_preset().config;
        ConfigOption* bed_type_opt    = wxGetApp().preset_bundle->project_config.option("curr_bed_type");
        if (bed_type_opt != nullptr) {
            BedType atype = (BedType) bed_type_opt->getInt();
            if ((BedType) bed_type_opt->getInt() == BedType::btPTE) {
                ConfigOptionInts* textured_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("textured_plate_temp"));
                if (textured_plate_temp && !textured_plate_temp->empty()) {
                    m_bedTempValue    = textured_plate_temp->values[0];
                    m_params.bed_type = BedType::btPTE;
                }
            } else if ((BedType) bed_type_opt->getInt() == BedType::btDEF) {
                ConfigOptionInts* customized_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("customized_plate_temp"));
                if (customized_plate_temp && !customized_plate_temp->empty()) {
                    m_bedTempValue    = customized_plate_temp->values[0];
                    m_params.bed_type = BedType::btDEF;
                }
            } else if ((BedType) bed_type_opt->getInt() == BedType::btER) {
                ConfigOptionInts* epoxy_resin_plate_temp = dynamic_cast<ConfigOptionInts*>(
                    filament_config->option("epoxy_resin_plate_temp"));
                if (epoxy_resin_plate_temp && !epoxy_resin_plate_temp->empty()) {
                    m_bedTempValue    = epoxy_resin_plate_temp->values[0];
                    m_params.bed_type = BedType::btER;
                }
            } else {
                // BedType::btPEI
                ConfigOptionInts* hot_plate_temp = dynamic_cast<ConfigOptionInts*>(filament_config->option("hot_plate_temp"));
                if (hot_plate_temp && !hot_plate_temp->empty()) {
                    m_bedTempValue    = hot_plate_temp->values[0];
                    m_params.bed_type = BedType::btPEI;
                }
            }
        }
    m_tiBedTemp->GetTextCtrl()->SetValue(wxString::FromDouble(m_bedTempValue));
}

void Dec_Acceleration_Dlg::on_start(wxCommandEvent& event)
{
    bool read_double = false;
    read_double      = m_tiStart->GetTextCtrl()->GetValue().ToDouble(&m_params.start);
    read_double      = read_double && m_tiEnd->GetTextCtrl()->GetValue().ToDouble(&m_params.end);
    read_double      = read_double && m_tiStep->GetTextCtrl()->GetValue().ToDouble(&m_params.step);
    read_double      = read_double && m_tiHstep->GetTextCtrl()->GetValue().ToDouble(&m_params.high_step);
    read_double = read_double && m_tiBedTemp->GetTextCtrl()->GetValue().ToDouble(&m_params.bed_temp);

    if (!read_double || m_params.start > 100 || m_params.end <= 0 || m_params.step <= 0 || m_params.start < (m_params.end + m_params.step) ||
        m_params.bed_temp > 200 || m_params.bed_temp < 1) {
        MessageDialog msg_dlg(nullptr, _L("Please input valid values:\nend > 0 \nstep > 0\nstart >= end + step\nstart<=100\nbed temp: >= 1\nbed temp: <= 200"), wxEmptyString,
                              wxICON_WARNING | wxOK);
        msg_dlg.ShowModal();
        return;
    }

    m_params.mode = CalibMode::Calib_Accel2Decel;
    EndModal(wxID_OK);
    m_plater->calib_dec_acc(m_params);

}

void Dec_Acceleration_Dlg::on_dpi_changed(const wxRect& suggested_rect)
{
    this->Refresh();
    Fit();
}




// High_Flowrate_Dlg
//

High_Flowrate_Dlg::High_Flowrate_Dlg(wxWindow* parent, wxWindowID id, Plater* plater)
    : DPIDialog(parent, wxID_ANY, _L("Pass 2"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX)
    , m_plater(plater)
{
    SetFont(wxGetApp().normal_font());
    SetBackgroundColour(*wxWHITE);
    this->SetSize(wxSize(FromDIP(559), FromDIP(507)));
    this->SetMinSize(wxSize(FromDIP(559), FromDIP(507)));
    this->SetMaxSize(wxSize(FromDIP(559), FromDIP(507)));

    wxSize btnSize = wxSize(FromDIP(144), FromDIP(115));
    m_btn0         = new ImgBtn(this, wxEmptyString, "calib_-20", wxDefaultPosition, btnSize, wxTE_PROCESS_ENTER);
    m_btn1         = new ImgBtn(this, wxEmptyString, "calib_-15", wxDefaultPosition, btnSize, wxTE_PROCESS_ENTER);
    m_btn2         = new ImgBtn(this, wxEmptyString, "calib_-10", wxDefaultPosition, btnSize, wxTE_PROCESS_ENTER);
    m_btn3         = new ImgBtn(this, wxEmptyString, "calib_-5", wxDefaultPosition, btnSize, wxTE_PROCESS_ENTER);
    m_btn4         = new ImgBtn(this, wxEmptyString, "calib_0", wxDefaultPosition, btnSize, wxTE_PROCESS_ENTER);
    m_btn5         = new ImgBtn(this, wxEmptyString, "calib_5", wxDefaultPosition, btnSize, wxTE_PROCESS_ENTER);
    m_btn6         = new ImgBtn(this, wxEmptyString, "calib_10", wxDefaultPosition, btnSize, wxTE_PROCESS_ENTER);
    m_btn7         = new ImgBtn(this, wxEmptyString, "calib_15", wxDefaultPosition, btnSize, wxTE_PROCESS_ENTER);
    m_btn8         = new ImgBtn(this, wxEmptyString, "calib_20", wxDefaultPosition, btnSize, wxTE_PROCESS_ENTER);

    m_option_buttons = {m_btn0, m_btn1, m_btn2, m_btn3, m_btn4, m_btn5, m_btn6, m_btn7, m_btn8};

    for (size_t i = 0; i < m_option_buttons.size(); ++i) {
        m_option_buttons[i]->Bind(wxEVT_LEFT_UP, [this, i](wxMouseEvent& e) {
            if (wxRect(wxPoint(0, 0), m_option_buttons[i]->GetClientSize()).Contains(e.GetPosition()))
                set_selected_option(static_cast<int>(i));
            e.Skip();
        });
    }

    StateColor btn_bg_green(std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Disabled),
                         std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Pressed),
                         std::pair<wxColour, int>(wxColour(238, 238, 238), StateColor::Hovered),
                         std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Enabled),
                         std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Normal));

    StateColor report_bd(std::pair<wxColour, int>(wxColour(144, 144, 144), StateColor::Disabled),
                         std::pair<wxColour, int>(wxColour("#d6d6dc"), StateColor::Enabled));
    StateColor report_text(std::pair<wxColour, int>(wxColour(144, 144, 144), StateColor::Disabled),
                           std::pair<wxColour, int>(wxColour(38, 46, 48), StateColor::Enabled));

    wxString info_str      = _L("Choose the one with the best coarse tuning");
    auto     info_str_size = wxWindow::GetTextExtent(info_str);
    auto     wx_info       = new wxStaticText(this, wxID_ANY, info_str, wxDefaultPosition, info_str_size, wxALIGN_LEFT);

    wxButton* svgButton = new wxButton(this, wxID_HIGHEST + 3, "", wxDefaultPosition, wxSize(FromDIP(18), FromDIP(18)), wxBORDER_NONE);
    svgButton->SetBackgroundColour(this->GetBackgroundColour());

    wxBitmap qImg = create_scaled_bitmap3("cali_page_caption_help", this, FromDIP(25), wxSize(FromDIP(25), FromDIP(25)));
    svgButton->SetBitmap(qImg);
    svgButton->Bind(wxEVT_BUTTON, [](wxCommandEvent&) {
        wxLaunchDefaultBrowser("https://wiki.creality.com/zh/software/update-released/Basic-introduction/calibration-tutorial");
    });

    wxBoxSizer* v_sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(v_sizer);

    auto topSizer = new wxBoxSizer(wxHORIZONTAL);
    topSizer->Add(wx_info, 0, wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(2));
    topSizer->Add(svgButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(2));
    v_sizer->Add(topSizer, 0, wxTOP | wxALIGN_CENTRE_HORIZONTAL, FromDIP(30));

    wxGridSizer* gridSizer = new wxGridSizer(3, 3, FromDIP(6), FromDIP(17));
    gridSizer->Add(m_btn8, 0, wxALIGN_CENTER, FromDIP(5));
    gridSizer->Add(m_btn7, 0, wxALIGN_CENTER, FromDIP(5));
    gridSizer->Add(m_btn6, 0, wxALIGN_CENTER, FromDIP(5));
    gridSizer->Add(m_btn5, 0, wxALIGN_CENTER, FromDIP(5));
    gridSizer->Add(m_btn4, 0, wxALIGN_CENTER, FromDIP(5));
    gridSizer->Add(m_btn3, 0, wxALIGN_CENTER, FromDIP(5));
    gridSizer->Add(m_btn0, 0, wxALIGN_CENTER, FromDIP(5));
    gridSizer->Add(m_btn1, 0, wxALIGN_CENTER, FromDIP(5));
    gridSizer->Add(m_btn2, 0, wxALIGN_CENTER, FromDIP(5));

    v_sizer->Add(gridSizer, 0, wxTOP | wxALIGN_CENTRE_HORIZONTAL, FromDIP(16));

    wxBoxSizer* buttonsSizer = new wxBoxSizer(wxHORIZONTAL);
    Button* okButton = new Button(this, _L("Confirm"));
    okButton->SetBackgroundColor(btn_bg_green);
    okButton->SetBorderColor(report_bd);
    okButton->SetTextColor(report_text);
    okButton->SetFont(Label::Body_12);
    okButton->SetSize(wxSize(FromDIP(100), FromDIP(24)));
    okButton->SetMinSize(wxSize(FromDIP(100), FromDIP(24)));
    okButton->SetMaxSize(wxSize(FromDIP(100), FromDIP(24)));
    okButton->SetCornerRadius(FromDIP(12));
    okButton->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& e) { 
         m_plater->calib_flowrate(2, m_CurrentValue);
        EndModal(wxID_OK);
    });

    Button* cancelButton = new Button(this, _L("Cancel"));
    cancelButton->SetBackgroundColor(btn_bg_green);
    cancelButton->SetBorderColor(report_bd);
    cancelButton->SetTextColor(report_text);
    cancelButton->SetFont(Label::Body_12);
    cancelButton->SetSize(wxSize(FromDIP(100), FromDIP(24)));
    cancelButton->SetMinSize(wxSize(FromDIP(100), FromDIP(24)));
    cancelButton->SetMaxSize(wxSize(FromDIP(100), FromDIP(24)));
    cancelButton->SetCornerRadius(FromDIP(12));
    cancelButton->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& e) { EndModal(wxID_CANCEL); });

    buttonsSizer->Add(okButton);
    buttonsSizer->AddSpacer(FromDIP(10));
    buttonsSizer->Add(cancelButton);
    v_sizer->AddSpacer(FromDIP(15));
    v_sizer->Add(buttonsSizer, 1, wxALL | wxALIGN_CENTRE_HORIZONTAL, 0);
    set_selected_option(0);


    wxGetApp().UpdateDlgDarkUI(this);
    Layout();
    Fit();
}

High_Flowrate_Dlg::~High_Flowrate_Dlg()
{
}

void High_Flowrate_Dlg::set_selected_option(int index)
{
    static constexpr std::array<float, 9> option_values = {0.8f, 0.85f, 0.9f, 0.95f, 1.0f, 1.05f, 1.1f, 1.15f, 1.2f};

    if (index < 0 || index >= static_cast<int>(m_option_buttons.size()))
        return;

    m_selected_index = index;
    m_CurrentValue   = option_values[static_cast<size_t>(index)];

    for (size_t i = 0; i < m_option_buttons.size(); ++i) {
        if (m_option_buttons[i] != nullptr)
            m_option_buttons[i]->SetSelected(static_cast<int>(i) == m_selected_index);
    }
}

void High_Flowrate_Dlg::on_dpi_changed(const wxRect& suggested_rect)
{
    this->Refresh();
    Fit();
}

XY_Offset_Calibration_Dlg::XY_Offset_Calibration_Dlg(wxWindow* parent, wxWindowID id, Plater* plater)
    : DPIDialog(parent, id, _L("Multi-toolhead XY Offset Test"), wxDefaultPosition, parent->FromDIP(wxSize(-1, 200)), wxDEFAULT_DIALOG_STYLE)
    , m_plater(plater)
{
    SetFont(wxGetApp().normal_font());
    SetBackgroundColour(*wxWHITE);

    const int   dlg_width = 360;
    wxBoxSizer* v_sizer   = new wxBoxSizer(wxVERTICAL);
    v_sizer->SetMinSize(wxSize(FromDIP(dlg_width), -1));
    SetSizer(v_sizer);

    const wxSize label_size(FromDIP(140), -1);
    const wxSize combo_size(FromDIP(120), -1);
    const wxColour label_color = StateColor::darkModeColorFor(wxColour("#323A3C"));

    auto add_row = [this, &v_sizer, label_size, combo_size, label_color](
        const wxString& label,
        ComboBox*& combo,
        const wxString* items,
        size_t item_count,
        const wxString& unit,
        int selection) {
        wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
        auto*       text = new wxStaticText(this, wxID_ANY, label, wxDefaultPosition, label_size, wxALIGN_LEFT);
        text->SetFont(Label::Body_13);
        text->SetForegroundColour(label_color);
        row->Add(text, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(8));

        combo = new ComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, combo_size, 0, nullptr, wxCB_READONLY);
        for (size_t i = 0; i < item_count; ++i)
            combo->Append(items[i]);
        combo->SetSelection(selection);
        row->Add(combo, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(6));

        if (!unit.empty()) {
            auto* unit_text = new wxStaticText(this, wxID_ANY, unit, wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
            unit_text->SetFont(Label::Body_13);
            unit_text->SetForegroundColour(label_color);
            row->Add(unit_text, 0, wxALIGN_CENTER_VERTICAL);
        }

        v_sizer->Add(row, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(10));
    };

    const wxString toolhead_choices[] = { "4", "6" };
    const wxString nozzle_choices[]   = { "0.4" };
    const wxString step_choices[]     = { "0.05", "0.1", "0.2" };

    v_sizer->Add(0, FromDIP(8), 0, wxEXPAND);
    add_row(_L("Toolhead count"), m_cb_toolhead_count, toolhead_choices, sizeof(toolhead_choices) / sizeof(wxString), _L("pcs"), 0);
    add_row(_L("Nozzle diameter"), m_cb_nozzle_diameter, nozzle_choices, sizeof(nozzle_choices) / sizeof(wxString), _L("mm"), 0);
    add_row(_L("Measurement step"), m_cb_step, step_choices, sizeof(step_choices) / sizeof(wxString), _L("mm"), 1);

    StateColor btn_bg_green(std::pair<wxColour, int>(wxColour(0, 137, 123), StateColor::Pressed),
                            std::pair<wxColour, int>(wxColour(38, 166, 154), StateColor::Hovered),
                            std::pair<wxColour, int>(wxColour(0, 150, 136), StateColor::Normal));

    m_btn_confirm = new Button(this, _L("Confirm"));
    m_btn_confirm->SetBackgroundColor(btn_bg_green);
    m_btn_confirm->SetBorderColor(*wxWHITE);
    m_btn_confirm->SetTextColor(wxColour(0xFFFFFE));
    m_btn_confirm->SetFont(Label::Body_12);
    m_btn_confirm->SetSize(wxSize(FromDIP(100), FromDIP(30)));
    m_btn_confirm->SetMinSize(wxSize(FromDIP(100), FromDIP(30)));
    m_btn_confirm->SetCornerRadius(FromDIP(12));
    m_btn_confirm->Bind(wxEVT_BUTTON, &XY_Offset_Calibration_Dlg::on_confirm, this);

    v_sizer->Add(m_btn_confirm, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, FromDIP(16));

    Layout();
    v_sizer->Fit(this);
    wxGetApp().UpdateDlgDarkUI(this);
}

XY_Offset_Calibration_Dlg::~XY_Offset_Calibration_Dlg() {}

void XY_Offset_Calibration_Dlg::on_dpi_changed(const wxRect& suggested_rect)
{
    this->Refresh();
    Fit();
}

void XY_Offset_Calibration_Dlg::on_confirm(wxCommandEvent& event)
{
    wxString toolhead_value = wxEmptyString;
    if (m_cb_toolhead_count) {
        toolhead_value = m_cb_toolhead_count->GetValue();
    }

    wxString nozzle_value = wxEmptyString;
    if (m_cb_nozzle_diameter) {
        nozzle_value = m_cb_nozzle_diameter->GetValue();
    }

    wxString step_value = wxEmptyString;
    if (m_cb_step) {
        step_value = m_cb_step->GetValue();
    }

    long   toolhead_count  = 0;
    double nozzle_diameter = 0.0;
    double step            = 0.0;

    if (!toolhead_value.empty() && toolhead_value.ToLong(&toolhead_count))
        m_toolhead_count = static_cast<int>(toolhead_count);
    if (!nozzle_value.empty() && nozzle_value.ToDouble(&nozzle_diameter))
        m_nozzle_diameter = nozzle_diameter;
    if (!step_value.empty() && step_value.ToDouble(&step))
        m_step = step;

    if (!m_plater || toolhead_value.empty() || nozzle_value.empty() || step_value.empty()) {
        EndModal(wxID_OK);
        return;
    }

    wxString gcode_path = from_u8(Slic3r::resources_dir()) +wxString::Format("/calib/xy_offset/%s-%s-%s.gcode", toolhead_value, nozzle_value, step_value);
    if (!wxFileExists(gcode_path)) {
        MessageDialog msg_dlg(this, wxString::Format(_L("G-code file not found:\n%s"), gcode_path),wxEmptyString, wxICON_WARNING | wxOK);
        msg_dlg.ShowModal();
        return;
    }

    EndModal(wxID_OK);
    m_plater->load_gcode(gcode_path);
}

}} // namespace Slic3r::GUI
