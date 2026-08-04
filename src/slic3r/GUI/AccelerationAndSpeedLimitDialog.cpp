#include "AccelerationAndSpeedLimitDialog.hpp"
#include "GUI_App.hpp"
#include "OptionsGroup.hpp"

#include <wx/wx.h> 
#include <wx/numformatter.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/tooltip.h>

#include "libslic3r/BoundingBox.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/Polygon.hpp"
#include "libslic3r/GCode/SmoothSpeedAndAcc.hpp"
#include "Widgets/CheckBox.hpp"
#include "MsgDialog.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>

#include <algorithm>

namespace Slic3r {
namespace GUI {

static bool s_hasShowErrorDlg = false;
static void showError(wxWindow* parent, const wxString& message, bool monospaced_font = false)
{
    s_hasShowErrorDlg = true;
    wxGetApp().CallAfter([=] {
        ErrorDialog msg(parent, message, monospaced_font);
        msg.ShowModal();
        s_hasShowErrorDlg = false;
    });
}

MyTextInput::MyTextInput(wxWindow* parent, wxString text) : TextInput(parent, text)
{

}

void MyTextInput::OnEdit()
{
    if (m_funcTextEditedCb != nullptr)
        m_funcTextEditedCb();
}

WeightLimitItem::WeightLimitItem(AccelerationAndSpeedLimitPanel* limitPanel, wxWindow* parent, int idx) : wxPanel(parent, wxID_ANY)
{
    m_limitPanel = limitPanel;
    m_nIdx = idx;
    wxString bgColor = wxGetApp().dark_mode() ? "#2B2B2B" : "#EFF0F6";
    wxString fgColor = wxGetApp().dark_mode() ? "#2B2B2B" : "#EFF0F6";
    this->SetMinSize(wxSize(FromDIP(728), FromDIP(144)));
    this->SetMaxSize(wxSize(FromDIP(728), FromDIP(144)));
    this->SetBackgroundColour(bgColor);
    this->Hide();

    auto main_sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(main_sizer);
    
    //  重量区间
    wxPanel* panel = new wxPanel(this, wxID_ANY);
    panel->Hide();
    m_weightPanel = panel;
    panel->SetMinSize(wxSize(FromDIP(728), FromDIP(48)));
    panel->SetMaxSize(wxSize(FromDIP(728), FromDIP(48)));
    wxBoxSizer* panelSizer = new wxBoxSizer(wxHORIZONTAL);
    panel->SetSizer(panelSizer);

    m_weightLabel = new Label(panel, _L("Weight range"));
    m_weightLabel->Hide();
    m_weightLabel->SetMinSize(wxSize(FromDIP(183), FromDIP(24)));
    m_weightLabel->SetMaxSize(wxSize(FromDIP(183), FromDIP(24)));
    m_weightLabel->SetFont(Label::Body_13);
    m_weightLabel->SetBackgroundColour(bgColor);
    m_weightLabel->SetForegroundColour(fgColor);
    panelSizer->Add(m_weightLabel, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(15));
    panelSizer->AddStretchSpacer();
    m_weightMin = new MyTextInput(panel, "0");
    m_weightMin->Hide();
    m_weightMin->SetLabel("kg");
    m_weightMin->SetMinSize(wxSize(FromDIP(144), FromDIP(32)));
    m_weightMin->SetMaxSize(wxSize(FromDIP(144), FromDIP(32)));
    m_weightMin->SetBackgroundColour(bgColor);
    m_weightMin->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(bgColor), StateColor::Pressed),
                                               std::pair<wxColour, int>(wxColour(bgColor), StateColor::Hovered),
                                               std::pair<wxColour, int>(wxColour(bgColor), StateColor::Normal)));
    m_weightMin->GetTextCtrl()->SetBackgroundColour(bgColor);
    m_weightMin->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    m_weightMin->SetCornerRadius(FromDIP(5));
    m_weightMin->setTextEditedCb([=]() {
        double val = 0.;
        if (!m_weightMin->GetTextCtrl()->GetValue().ToDouble(&val)) {
            showError(m_parent, _L("Invalid numeric."));
            m_weightMin->GetTextCtrl()->SetValue(m_editLimitData.value1);
        } else {
            if (m_weightMin->GetTextCtrl()->GetValue().ToStdString().c_str()[0] == '-') {
                showError(m_parent, _L("Invalid numeric."));
                m_weightMin->GetTextCtrl()->SetValue(m_editLimitData.value1);
                return;
            } else {
                std::string str = m_weightMin->GetTextCtrl()->GetValue().ToStdString();
                size_t pos = str.find('.');
                if (pos != std::string::npos && str.substr(pos).size() > 2) {
                    showError(m_parent, _L("Invalid numeric."));
                    m_weightMin->GetTextCtrl()->SetValue(m_editLimitData.value1);
                    return;
                }
            }
            updateWeightLabelColor();
            m_limitPanel->checkIsShowResetBtn();
        }
    });
    panelSizer->Add(m_weightMin, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL);
    m_lineBreak = new wxStaticText(panel, wxID_ANY, "--");
    m_lineBreak->Hide();
    m_lineBreak->SetMinSize(wxSize(FromDIP(15), FromDIP(21)));
    m_lineBreak->SetMaxSize(wxSize(FromDIP(15), FromDIP(21)));
    m_lineBreak->SetBackgroundColour(bgColor);
    panelSizer->Add(m_lineBreak, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL);
    m_weightMax = new MyTextInput(panel, "10");
    m_weightMax->Hide();
    m_weightMax->SetLabel("kg");
    m_weightMax->SetMinSize(wxSize(FromDIP(144), FromDIP(32)));
    m_weightMax->SetMaxSize(wxSize(FromDIP(144), FromDIP(32)));
    m_weightMax->SetBackgroundColour(bgColor);
    m_weightMax->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(bgColor), StateColor::Pressed),
                                               std::pair<wxColour, int>(wxColour(bgColor), StateColor::Hovered),
                                               std::pair<wxColour, int>(wxColour(bgColor), StateColor::Normal)));
    m_weightMax->GetTextCtrl()->SetBackgroundColour(bgColor);
    m_weightMax->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    m_weightMax->SetCornerRadius(FromDIP(5));
    m_weightMax->setTextEditedCb([this]() {
        double val = 0.;
        if (!m_weightMax->GetTextCtrl()->GetValue().ToDouble(&val)) {
            showError(m_parent, _L("Invalid numeric."));
            m_weightMax->GetTextCtrl()->SetValue(m_editLimitData.value2);
        } else {
            if (m_weightMax->GetTextCtrl()->GetValue().ToStdString().c_str()[0] == '-') {
                showError(m_parent, _L("Invalid numeric."));
                m_weightMax->GetTextCtrl()->SetValue(m_editLimitData.value2);
                return;
            } else {
                std::string str = m_weightMax->GetTextCtrl()->GetValue().ToStdString();
                size_t      pos = str.find('.');
                if (pos != std::string::npos && str.substr(pos).size() > 2) {
                    showError(m_parent, _L("Invalid numeric."));
                    m_weightMax->GetTextCtrl()->SetValue(m_editLimitData.value2);
                    return;
                }
            }
            updateWeightLabelColor();
            m_limitPanel->checkIsShowResetBtn();
        }
    });
    panelSizer->Add(m_weightMax, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL);
    m_delBtn = new HoverBorderIcon(panel, wxEmptyString,
        wxGetApp().dark_mode() ? "cross" : "cross_light",
        wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    m_delBtn->Hide();
    m_delBtn->SetMinSize(wxSize(FromDIP(24), FromDIP(24)));
    m_delBtn->SetMaxSize(wxSize(FromDIP(24), FromDIP(24)));
    m_delBtn->SetBackgroundColour(wxColour(bgColor));
    m_delBtn->SetBorderColorNormal(wxColour(bgColor));
    m_delBtn->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(bgColor), StateColor::Pressed),
                                            std::pair<wxColour, int>(wxColour(bgColor), StateColor::Hovered),
                                            std::pair<wxColour, int>(wxColour(bgColor), StateColor::Normal)));
    m_delBtn->Bind(wxEVT_LEFT_DOWN, [this](wxEvent&) {
        if (m_funcDelBtnClickedCb != nullptr)
            m_funcDelBtnClickedCb(m_nIdx);
        if (m_limitPanel != nullptr) {
            m_limitPanel->checkIsShowResetBtn();
        }
    });
    panelSizer->Add(m_delBtn, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);

    main_sizer->Add(panel, 1, wxEXPAND);

    //  打印速度 
    panel = new wxPanel(this, wxID_ANY);
    m_speedPanel = panel;
    panel->Hide();
    panel->SetMinSize(wxSize(FromDIP(728), FromDIP(48)));
    panel->SetMaxSize(wxSize(FromDIP(728), FromDIP(48)));
    panelSizer = new wxBoxSizer(wxHORIZONTAL);
    panel->SetSizer(panelSizer);

    m_speedLabel = new wxStaticText(panel, wxID_ANY, _L("Print speed"));
    m_speedLabel->Hide();
    m_speedLabel->SetFont(Label::Body_13);
    m_speedLabel->SetMinSize(wxSize(FromDIP(183), FromDIP(24)));
    m_speedLabel->SetMaxSize(wxSize(FromDIP(183), FromDIP(24)));
    m_speedLabel->SetForegroundColour(fgColor);
    panelSizer->Add(m_speedLabel, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(15));
    panelSizer->AddStretchSpacer();
    m_speed = new MyTextInput(panel, "1000");
    m_speed->Hide();
    m_speed->SetLabel("mm/s");
    m_speed->SetMinSize(wxSize(FromDIP(304), FromDIP(32)));
    m_speed->SetMaxSize(wxSize(FromDIP(304), FromDIP(32)));
    m_speed->SetBackgroundColour(bgColor);
    m_speed->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(bgColor), StateColor::Pressed),
                                           std::pair<wxColour, int>(wxColour(bgColor), StateColor::Hovered),
                                           std::pair<wxColour, int>(wxColour(bgColor), StateColor::Normal)));
    m_speed->GetTextCtrl()->SetBackgroundColour(bgColor);
    m_speed->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    m_speed->SetCornerRadius(FromDIP(5));
    m_speed->setTextEditedCb([this]() {
        long val = 0;
        if (!m_speed->GetTextCtrl()->GetValue().ToLong(&val)) {
            showError(m_parent, _L("Invalid numeric."));
            m_speed->GetTextCtrl()->SetValue(m_editLimitData.speed1);
        } else {
            if (m_speed->GetTextCtrl()->GetValue().ToStdString().c_str()[0] == '-') {
                showError(m_parent, _L("Invalid numeric."));
                m_speed->GetTextCtrl()->SetValue(m_editLimitData.speed1);
                return;
            }
            updateSpeedLabelColor();
            m_limitPanel->checkIsShowResetBtn();
        }
    });
    panelSizer->Add(m_speed, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL);
    auto tmp = new wxStaticText(panel, wxID_ANY, "");
    tmp->SetMinSize(wxSize(FromDIP(24), FromDIP(24)));
    tmp->SetMaxSize(wxSize(FromDIP(24), FromDIP(24)));
    panelSizer->Add(tmp, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL);
    
    main_sizer->Add(panel, 1, wxEXPAND);

    //  加速度
    panel           = new wxPanel(this, wxID_ANY);
    panel->Hide();
    m_AccelerationPanel = panel;
    panel->SetMinSize(wxSize(FromDIP(728), FromDIP(48)));
    panel->SetMaxSize(wxSize(FromDIP(728), FromDIP(48)));
    panelSizer = new wxBoxSizer(wxHORIZONTAL);
    panel->SetSizer(panelSizer);

    m_AccelerationLabel = new wxStaticText(panel, wxID_ANY, _L("Acceleration"));
    m_AccelerationLabel->Hide();
    m_AccelerationLabel->SetFont(Label::Body_13);
    m_AccelerationLabel->SetMinSize(wxSize(FromDIP(183), FromDIP(24)));
    m_AccelerationLabel->SetMaxSize(wxSize(FromDIP(183), FromDIP(24)));
    m_AccelerationLabel->SetForegroundColour(fgColor);
    panelSizer->Add(m_AccelerationLabel, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(15));
    panelSizer->AddStretchSpacer();
    m_Acceleration = new MyTextInput(panel, "100000");
    m_Acceleration->Hide();
    m_Acceleration->SetLabel(_L("mm/s²"));
    m_Acceleration->SetMinSize(wxSize(FromDIP(304), FromDIP(32)));
    m_Acceleration->SetMaxSize(wxSize(FromDIP(304), FromDIP(32)));
    m_Acceleration->SetBackgroundColour(bgColor);
    m_Acceleration->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(bgColor), StateColor::Pressed),
                                           std::pair<wxColour, int>(wxColour(bgColor), StateColor::Hovered),
                                           std::pair<wxColour, int>(wxColour(bgColor), StateColor::Normal)));
    m_Acceleration->GetTextCtrl()->SetBackgroundColour(bgColor);
    m_Acceleration->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    m_Acceleration->SetCornerRadius(FromDIP(5));
    m_Acceleration->setTextEditedCb([this]() {
        unsigned long val = 0;
        if (!m_Acceleration->GetTextCtrl()->GetValue().ToULong(&val)) {
            showError(m_parent, _L("Invalid numeric."));
            m_Acceleration->GetTextCtrl()->SetValue(m_editLimitData.Acc1);
        } else {
            if (m_Acceleration->GetTextCtrl()->GetValue().ToStdString().c_str()[0] == '-') {
                showError(m_parent, _L("Invalid numeric."));
                m_Acceleration->GetTextCtrl()->SetValue(m_editLimitData.Acc1);
                return;
            }
            updateAccLabelColor();
            m_limitPanel->checkIsShowResetBtn();
        }
    });
    panelSizer->Add(m_Acceleration, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL);
    m_temp1 = new TextInput(panel, "0");
    m_temp1->Hide();
    panelSizer->Add(m_temp1, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL);
    tmp = new wxStaticText(panel, wxID_ANY, "");
    tmp->SetMinSize(wxSize(FromDIP(24), FromDIP(24)));
    tmp->SetMaxSize(wxSize(FromDIP(24), FromDIP(24)));
    panelSizer->Add(tmp, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL);

    main_sizer->Add(panel, 1, wxEXPAND);
    
}

void WeightLimitItem::updateItemData(const LimitData* limitData, const LimitData* defaultLimitData)
{
    if (limitData == nullptr)
        return;

    m_weightMin->GetTextCtrl()->ChangeValue(limitData->value1);
    m_weightMax->GetTextCtrl()->ChangeValue(limitData->value2);
    m_speed->GetTextCtrl()->ChangeValue(limitData->speed1);
    m_Acceleration->GetTextCtrl()->ChangeValue(limitData->Acc1);
    m_temp1->GetTextCtrl()->ChangeValue(limitData->Temp1);
    m_limitData = *limitData;
    m_editLimitData = m_limitData;
    m_defaultLimitData = defaultLimitData;
}

std::string WeightLimitItem::serialize()
{
    std::string data = "[";
    data += m_weightMin->GetTextCtrl()->GetValue().ToStdString();
    data += ",";
    data += m_weightMax->GetTextCtrl()->GetValue().ToStdString();
    data += ",";
    data += m_speed->GetTextCtrl()->GetValue().ToStdString();
    data += ",";
    data += m_Acceleration->GetTextCtrl()->GetValue().ToStdString();
    if (!m_temp1->GetTextCtrl()->GetValue().ToStdString().empty()) {
        data += ",";
        data += m_temp1->GetTextCtrl()->GetValue().ToStdString();
    }
    data += "]";
    return data;
}
const WeightLimitItem::LimitData& WeightLimitItem::getEditLimitData()
{
    m_editLimitData.value1 = m_weightMin->GetTextCtrl()->GetValue().ToStdString();
    m_editLimitData.value2 = m_weightMax->GetTextCtrl()->GetValue().ToStdString();
    m_editLimitData.speed1 = m_speed->GetTextCtrl()->GetValue().ToStdString();
    m_editLimitData.Acc1 = m_Acceleration->GetTextCtrl()->GetValue().ToStdString();
    m_editLimitData.Temp1 = m_temp1->GetTextCtrl()->GetValue().ToStdString();
    return m_editLimitData;
}

void WeightLimitItem::showSubCtrl()
{
    wxString bgColor = wxGetApp().dark_mode() ? "#313131" : "#FFFFFF";
    wxString fgColor = wxGetApp().dark_mode() ? "#FFFFFF" : "#30373D";
    this->SetBackgroundColour(bgColor);

    m_weightLabel->SetBackgroundColour(bgColor);
    //m_weightLabel->SetForegroundColour(fgColor);
    updateWeightLabelColor();
    m_weightMin->SetBackgroundColour(bgColor);
    m_weightMin->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(bgColor), StateColor::Pressed),
                                               std::pair<wxColour, int>(wxColour(bgColor), StateColor::Hovered),
                                               std::pair<wxColour, int>(wxColour(bgColor), StateColor::Normal)));
    m_weightMin->GetTextCtrl()->SetBackgroundColour(bgColor);
    m_lineBreak->SetBackgroundColour(bgColor);
    m_weightMax->SetBackgroundColour(bgColor);
    m_weightMax->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(bgColor), StateColor::Pressed),
                                               std::pair<wxColour, int>(wxColour(bgColor), StateColor::Hovered),
                                               std::pair<wxColour, int>(wxColour(bgColor), StateColor::Normal)));
    m_weightMax->GetTextCtrl()->SetBackgroundColour(bgColor);
    m_delBtn->SetBackgroundColour(wxColour(bgColor));
    m_delBtn->SetBorderColorNormal(wxColour(bgColor));
    m_delBtn->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(bgColor), StateColor::Pressed),
                                            std::pair<wxColour, int>(wxColour(bgColor), StateColor::Hovered),
                                            std::pair<wxColour, int>(wxColour(bgColor), StateColor::Normal)));

    //m_speedLabel->SetForegroundColour(fgColor);
    updateSpeedLabelColor();
    m_speed->SetBackgroundColour(bgColor);
    m_speed->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(bgColor), StateColor::Pressed),
                                           std::pair<wxColour, int>(wxColour(bgColor), StateColor::Hovered),
                                           std::pair<wxColour, int>(wxColour(bgColor), StateColor::Normal)));
    m_speed->GetTextCtrl()->SetBackgroundColour(bgColor);

    //m_AccelerationLabel->SetForegroundColour(fgColor);
    updateAccLabelColor();
    m_Acceleration->SetBackgroundColour(bgColor);
    m_Acceleration->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(bgColor), StateColor::Pressed),
                                                  std::pair<wxColour, int>(wxColour(bgColor), StateColor::Hovered),
                                                  std::pair<wxColour, int>(wxColour(bgColor), StateColor::Normal)));
    m_Acceleration->GetTextCtrl()->SetBackgroundColour(bgColor);

    m_weightPanel->Show();
    m_weightLabel->Show();
    m_weightMin->Show();
    m_lineBreak->Show();
    m_weightMax->Show();
    m_delBtn->Show();
    m_speedPanel->Show();
    m_speedLabel->Show();
    m_speed->Show();
    m_AccelerationPanel->Show();
    m_AccelerationLabel->Show();
    m_Acceleration->Show();
}

void WeightLimitItem::updateWeightLabelColor() {
    if (m_defaultLimitData != nullptr) {
        wxString fgColor = wxGetApp().dark_mode() ? "#FFFFFF" : "#30373D";
        if (m_weightMin->GetTextCtrl()->GetValue().ToStdString() == m_defaultLimitData->value1 &&
            m_weightMax->GetTextCtrl()->GetValue().ToStdString() == m_defaultLimitData->value2) {
            m_weightLabel->SetForegroundColour(fgColor);
        } else {
            m_weightLabel->SetForegroundColour("#FF870A");
        }
    } else {
        m_weightLabel->SetForegroundColour("#FF870A");
    }
    m_weightLabel->Refresh();
}

void WeightLimitItem::updateSpeedLabelColor() {
    if (m_defaultLimitData != nullptr) {
        wxString fgColor = wxGetApp().dark_mode() ? "#FFFFFF" : "#30373D";
        if (m_speed->GetTextCtrl()->GetValue().ToStdString() == m_defaultLimitData->speed1) {
            m_speedLabel->SetForegroundColour(fgColor);
        } else {
            m_speedLabel->SetForegroundColour("#FF870A");
        }
    } else {
        m_speedLabel->SetForegroundColour("#FF870A");
    }
    m_speedLabel->Refresh();
}

void WeightLimitItem::updateAccLabelColor() {
    if (m_defaultLimitData != nullptr) {
        wxString fgColor = wxGetApp().dark_mode() ? "#FFFFFF" : "#30373D";
        if (m_Acceleration->GetTextCtrl()->GetValue().ToStdString() == m_defaultLimitData->Acc1) {
            m_AccelerationLabel->SetForegroundColour(fgColor);
        } else {
            m_AccelerationLabel->SetForegroundColour("#FF870A");
        }
    } else {
        m_AccelerationLabel->SetForegroundColour("#FF870A");
    }
    m_AccelerationLabel->Refresh();
}

HeightLimitItem::HeightLimitItem(AccelerationAndSpeedLimitPanel* limitPanel, wxWindow* parent, int idx) : wxPanel(parent, wxID_ANY)
{
    m_limitPanel = limitPanel;
    m_nIdx = idx;
    wxString bgColor = wxGetApp().dark_mode() ? "#2B2B2B" : "#EFF0F6";
    wxString fgColor = wxGetApp().dark_mode() ? "#2B2B2B" : "#EFF0F6";
    this->SetMinSize(wxSize(FromDIP(728), FromDIP(144)));
    this->SetMaxSize(wxSize(FromDIP(728), FromDIP(144)));
    this->SetBackgroundColour(bgColor);
    this->Hide();

    auto main_sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(main_sizer);

    //  重量区间
    wxPanel* panel = new wxPanel(this, wxID_ANY);
    panel->Hide();
    m_heightPanel = panel;
    panel->SetMinSize(wxSize(FromDIP(728), FromDIP(48)));
    panel->SetMaxSize(wxSize(FromDIP(728), FromDIP(48)));
    wxBoxSizer* panelSizer = new wxBoxSizer(wxHORIZONTAL);
    panel->SetSizer(panelSizer);

    m_heightLabel = new wxStaticText(panel, wxID_ANY, _L("Height range"));
    m_heightLabel->Hide();
    m_heightLabel->SetMinSize(wxSize(FromDIP(183), FromDIP(24)));
    m_heightLabel->SetMaxSize(wxSize(FromDIP(183), FromDIP(24)));
    m_heightLabel->SetFont(Label::Body_13);
    m_heightLabel->SetBackgroundColour(bgColor);
    m_heightLabel->SetForegroundColour(fgColor);
    panelSizer->Add(m_heightLabel, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(15));
    panelSizer->AddStretchSpacer();
    m_heightMin = new MyTextInput(panel, "0");
    m_heightMin->Hide();
    m_heightMin->SetLabel("mm");
    m_heightMin->SetMinSize(wxSize(FromDIP(144), FromDIP(32)));
    m_heightMin->SetMaxSize(wxSize(FromDIP(144), FromDIP(32)));
    m_heightMin->SetBackgroundColour(bgColor);
    m_heightMin->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(bgColor), StateColor::Pressed),
                                               std::pair<wxColour, int>(wxColour(bgColor), StateColor::Hovered),
                                               std::pair<wxColour, int>(wxColour(bgColor), StateColor::Normal)));
    m_heightMin->GetTextCtrl()->SetBackgroundColour(bgColor);
    m_heightMin->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    m_heightMin->SetCornerRadius(FromDIP(5));
    m_heightMin->setTextEditedCb([this]() {
        long val = 0;
        if (!m_heightMin->GetTextCtrl()->GetValue().ToLong(&val)) {
            showError(m_parent, _L("Invalid numeric."));
            m_heightMin->GetTextCtrl()->SetValue(m_editLimitData.value1);
        } else {
            if (m_heightMin->GetTextCtrl()->GetValue().ToStdString().c_str()[0] == '-') {
                showError(m_parent, _L("Invalid numeric."));
                m_heightMin->GetTextCtrl()->SetValue(m_editLimitData.value1);
                return;
            }
            updateHeightLabelColor();
            m_limitPanel->checkIsShowResetBtn();
        }
    });
    panelSizer->Add(m_heightMin, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL);
    m_lineBreak = new wxStaticText(panel, wxID_ANY, "--");
    m_lineBreak->Hide();
    m_lineBreak->SetMinSize(wxSize(FromDIP(15), FromDIP(21)));
    m_lineBreak->SetMaxSize(wxSize(FromDIP(15), FromDIP(21)));
    panelSizer->Add(m_lineBreak, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL);
    m_heightMax = new MyTextInput(panel, "1000");
    m_heightMax->Hide();
    m_heightMax->SetLabel("mm");
    m_heightMax->SetMinSize(wxSize(FromDIP(144), FromDIP(32)));
    m_heightMax->SetMaxSize(wxSize(FromDIP(144), FromDIP(32)));
    m_heightMax->SetBackgroundColour(bgColor);
    m_heightMax->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(bgColor), StateColor::Pressed),
                                               std::pair<wxColour, int>(wxColour(bgColor), StateColor::Hovered),
                                               std::pair<wxColour, int>(wxColour(bgColor), StateColor::Normal)));
    m_heightMax->GetTextCtrl()->SetBackgroundColour(bgColor);
    m_heightMax->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    m_heightMax->SetCornerRadius(FromDIP(5));
    m_heightMax->setTextEditedCb([this]() {
        long val = 0;
        if (!m_heightMax->GetTextCtrl()->GetValue().ToLong(&val)) {
            showError(m_parent, _L("Invalid numeric."));
            m_heightMax->GetTextCtrl()->SetValue(m_editLimitData.value2);
        } else {
            if (m_heightMax->GetTextCtrl()->GetValue().ToStdString().c_str()[0] == '-') {
                showError(m_parent, _L("Invalid numeric."));
                m_heightMax->GetTextCtrl()->SetValue(m_editLimitData.value2);
                return;
            }
            updateHeightLabelColor();
            m_limitPanel->checkIsShowResetBtn();
        }
    });
    panelSizer->Add(m_heightMax, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL);
    m_delBtn = new HoverBorderIcon(panel, wxEmptyString,
        wxGetApp().dark_mode() ? "cross" : "cross_light",
        wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    m_delBtn->Hide();
    m_delBtn->SetMinSize(wxSize(FromDIP(24), FromDIP(24)));
    m_delBtn->SetMaxSize(wxSize(FromDIP(24), FromDIP(24)));
    m_delBtn->SetBackgroundColour(wxColour(bgColor));
    m_delBtn->SetBorderColorNormal(wxColour(bgColor));
    m_delBtn->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(bgColor), StateColor::Pressed),
                                            std::pair<wxColour, int>(wxColour(bgColor), StateColor::Hovered),
                                            std::pair<wxColour, int>(wxColour(bgColor), StateColor::Normal)));
    m_delBtn->Bind(wxEVT_LEFT_DOWN, [this](wxEvent&) {
        if (m_funcDelBtnClickedCb != nullptr)
            m_funcDelBtnClickedCb(m_nIdx);
        if (m_limitPanel != nullptr) {
            m_limitPanel->checkIsShowResetBtn();
        }
    });
    panelSizer->Add(m_delBtn, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);

    main_sizer->Add(panel, 1, wxEXPAND);

    //  打印速度
    panel = new wxPanel(this, wxID_ANY);
    m_speedPanel = panel;
    panel->Hide();
    panel->SetMinSize(wxSize(FromDIP(728), FromDIP(48)));
    panel->SetMaxSize(wxSize(FromDIP(728), FromDIP(48)));
    panelSizer = new wxBoxSizer(wxHORIZONTAL);
    panel->SetSizer(panelSizer);

    m_speedLabel = new wxStaticText(panel, wxID_ANY, _L("Print speed"));
    m_speedLabel->Hide();
    m_speedLabel->SetFont(Label::Body_13);
    m_speedLabel->SetMinSize(wxSize(FromDIP(183), FromDIP(24)));
    m_speedLabel->SetMaxSize(wxSize(FromDIP(183), FromDIP(24)));
    m_speedLabel->SetForegroundColour(fgColor);
    panelSizer->Add(m_speedLabel, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(15));
    panelSizer->AddStretchSpacer();
    m_speed = new MyTextInput(panel, "1000");
    m_speed->Hide();
    m_speed->SetLabel("mm/s");
    m_speed->SetMinSize(wxSize(FromDIP(304), FromDIP(32)));
    m_speed->SetMaxSize(wxSize(FromDIP(304), FromDIP(32)));
    m_speed->SetBackgroundColour(bgColor);
    m_speed->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(bgColor), StateColor::Pressed),
                                           std::pair<wxColour, int>(wxColour(bgColor), StateColor::Hovered),
                                           std::pair<wxColour, int>(wxColour(bgColor), StateColor::Normal)));
    m_speed->GetTextCtrl()->SetBackgroundColour(bgColor);
    m_speed->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    m_speed->SetCornerRadius(FromDIP(5));
    m_speed->setTextEditedCb([this]() {
        long val = 0;
        if (!m_speed->GetTextCtrl()->GetValue().ToLong(&val)) {
            showError(m_parent, _L("Invalid numeric."));
            m_speed->GetTextCtrl()->SetValue(m_editLimitData.speed1);
        } else {
            if (m_speed->GetTextCtrl()->GetValue().ToStdString().c_str()[0] == '-') {
                showError(m_parent, _L("Invalid numeric."));
                m_speed->GetTextCtrl()->SetValue(m_editLimitData.speed1);
                return;
            }
            updateSpeedLabelColor();
            m_limitPanel->checkIsShowResetBtn();
        }
    });
    panelSizer->Add(m_speed, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL);
    auto tmp = new wxStaticText(panel, wxID_ANY, "");
    tmp->SetMinSize(wxSize(FromDIP(24), FromDIP(24)));
    tmp->SetMaxSize(wxSize(FromDIP(24), FromDIP(24)));
    panelSizer->Add(tmp, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL);

    main_sizer->Add(panel, 1, wxEXPAND);

    //  加速度
    panel = new wxPanel(this, wxID_ANY);
    panel->Hide();
    m_AccelerationPanel = panel;
    panel->SetMinSize(wxSize(FromDIP(728), FromDIP(48)));
    panel->SetMaxSize(wxSize(FromDIP(728), FromDIP(48)));
    panelSizer = new wxBoxSizer(wxHORIZONTAL);
    panel->SetSizer(panelSizer);

    m_AccelerationLabel = new wxStaticText(panel, wxID_ANY, _L("Acceleration"));
    m_AccelerationLabel->Hide();
    m_AccelerationLabel->SetFont(Label::Body_13);
    m_AccelerationLabel->SetMinSize(wxSize(FromDIP(183), FromDIP(24)));
    m_AccelerationLabel->SetMaxSize(wxSize(FromDIP(183), FromDIP(24)));
    m_AccelerationLabel->SetForegroundColour(fgColor);
    panelSizer->Add(m_AccelerationLabel, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(15));
    panelSizer->AddStretchSpacer();
    m_Acceleration = new MyTextInput(panel, "100000");
    m_Acceleration->Hide();
    m_Acceleration->SetLabel(_L("mm/s²"));
    m_Acceleration->SetMinSize(wxSize(FromDIP(304), FromDIP(32)));
    m_Acceleration->SetMaxSize(wxSize(FromDIP(304), FromDIP(32)));
    m_Acceleration->SetBackgroundColour(bgColor);
    m_Acceleration->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(bgColor), StateColor::Pressed),
                                                  std::pair<wxColour, int>(wxColour(bgColor), StateColor::Hovered),
                                                  std::pair<wxColour, int>(wxColour(bgColor), StateColor::Normal)));
    m_Acceleration->GetTextCtrl()->SetBackgroundColour(bgColor);
    m_Acceleration->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    m_Acceleration->SetCornerRadius(FromDIP(5));
    m_Acceleration->setTextEditedCb([this]() {
        long val = 0;
        if (!m_Acceleration->GetTextCtrl()->GetValue().ToLong(&val)) {
            showError(m_parent, _L("Invalid numeric."));
            m_Acceleration->GetTextCtrl()->SetValue(m_editLimitData.Acc1);
        } else {
            if (m_Acceleration->GetTextCtrl()->GetValue().ToStdString().c_str()[0] == '-') {
                showError(m_parent, _L("Invalid numeric."));
                m_Acceleration->GetTextCtrl()->SetValue(m_editLimitData.Acc1);
                return;
            }
            updateAccLabelColor();
            m_limitPanel->checkIsShowResetBtn();
        }
    });
    panelSizer->Add(m_Acceleration, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL);
    m_temp1 = new TextInput(panel, "0");
    m_temp1->Hide();
    panelSizer->Add(m_temp1, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL);
    tmp = new wxStaticText(panel, wxID_ANY, "");
    tmp->SetMinSize(wxSize(FromDIP(24), FromDIP(24)));
    tmp->SetMaxSize(wxSize(FromDIP(24), FromDIP(24)));
    panelSizer->Add(tmp, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL);

    main_sizer->Add(panel, 1, wxEXPAND);
}
void HeightLimitItem::updateItemData(const WeightLimitItem::LimitData* limitData, const WeightLimitItem::LimitData* defaultLimitData)
{
    if (limitData == nullptr)
        return;
    m_heightMin->GetTextCtrl()->ChangeValue(limitData->value1);
    m_heightMax->GetTextCtrl()->ChangeValue(limitData->value2);
    m_speed->GetTextCtrl()->ChangeValue(limitData->speed1);
    m_Acceleration->GetTextCtrl()->ChangeValue(limitData->Acc1);
    m_temp1->GetTextCtrl()->ChangeValue(limitData->Temp1);
    m_limitData = *limitData;
    m_editLimitData = m_limitData;
    m_defaultLimitData = defaultLimitData;
}

std::string HeightLimitItem::serialize() { 
    std::string data = "[";
    data += m_heightMin->GetTextCtrl()->GetValue().ToStdString();
    data += ",";
    data += m_heightMax->GetTextCtrl()->GetValue().ToStdString();
    data += ",";
    data += m_speed->GetTextCtrl()->GetValue().ToStdString();
    data += ",";
    data += m_Acceleration->GetTextCtrl()->GetValue().ToStdString();
    if (!m_temp1->GetTextCtrl()->GetValue().ToStdString().empty()) {
        data += ",";
        data += m_temp1->GetTextCtrl()->GetValue().ToStdString();
    }
    data += "]";
    return data;
}
const WeightLimitItem::LimitData& HeightLimitItem::getEditLimitData()
{
    m_editLimitData.value1 = m_heightMin->GetTextCtrl()->GetValue().ToStdString();
    m_editLimitData.value2 = m_heightMax->GetTextCtrl()->GetValue().ToStdString();
    m_editLimitData.speed1 = m_speed->GetTextCtrl()->GetValue().ToStdString();
    m_editLimitData.Acc1   = m_Acceleration->GetTextCtrl()->GetValue().ToStdString();
    m_editLimitData.Temp1  = m_temp1->GetTextCtrl()->GetValue().ToStdString();
    return m_editLimitData;
}

void HeightLimitItem::showSubCtrl()
{
    wxString bgColor = wxGetApp().dark_mode() ? "#313131" : "#FFFFFF";
    wxString fgColor = wxGetApp().dark_mode() ? "#FFFFFF" : "#30373D";
    this->SetBackgroundColour(bgColor);

    m_heightLabel->SetBackgroundColour(bgColor);
    //m_heightLabel->SetForegroundColour(fgColor);
    updateHeightLabelColor();
    m_heightMin->SetBackgroundColour(bgColor);
    m_heightMin->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(bgColor), StateColor::Pressed),
                                               std::pair<wxColour, int>(wxColour(bgColor), StateColor::Hovered),
                                               std::pair<wxColour, int>(wxColour(bgColor), StateColor::Normal)));
    m_heightMin->GetTextCtrl()->SetBackgroundColour(bgColor);
    m_lineBreak->SetBackgroundColour(bgColor);
    m_heightMax->SetBackgroundColour(bgColor);
    m_heightMax->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(bgColor), StateColor::Pressed),
                                               std::pair<wxColour, int>(wxColour(bgColor), StateColor::Hovered),
                                               std::pair<wxColour, int>(wxColour(bgColor), StateColor::Normal)));
    m_heightMax->GetTextCtrl()->SetBackgroundColour(bgColor);
    m_delBtn->SetBackgroundColour(wxColour(bgColor));
    m_delBtn->SetBorderColorNormal(wxColour(bgColor));
    m_delBtn->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(bgColor), StateColor::Pressed),
                                            std::pair<wxColour, int>(wxColour(bgColor), StateColor::Hovered),
                                            std::pair<wxColour, int>(wxColour(bgColor), StateColor::Normal)));

    //m_speedLabel->SetForegroundColour(fgColor);
    updateSpeedLabelColor();
    m_speed->SetBackgroundColour(bgColor);
    m_speed->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(bgColor), StateColor::Pressed),
                                           std::pair<wxColour, int>(wxColour(bgColor), StateColor::Hovered),
                                           std::pair<wxColour, int>(wxColour(bgColor), StateColor::Normal)));
    m_speed->GetTextCtrl()->SetBackgroundColour(bgColor);

    //m_AccelerationLabel->SetForegroundColour(fgColor);
    updateAccLabelColor();
    m_Acceleration->SetBackgroundColour(bgColor);
    m_Acceleration->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(bgColor), StateColor::Pressed),
                                                  std::pair<wxColour, int>(wxColour(bgColor), StateColor::Hovered),
                                                  std::pair<wxColour, int>(wxColour(bgColor), StateColor::Normal)));
    m_Acceleration->GetTextCtrl()->SetBackgroundColour(bgColor);

    m_heightPanel->Show();
    m_heightLabel->Show();
    m_heightMin->Show();
    m_lineBreak->Show();
    m_heightMax->Show();
    m_delBtn->Show();
    m_speedPanel->Show();
    m_speedLabel->Show();
    m_speed->Show();
    m_AccelerationPanel->Show();
    m_AccelerationLabel->Show();
    m_Acceleration->Show();
}

void HeightLimitItem::updateHeightLabelColor()
{
    if (m_defaultLimitData != nullptr) {
        wxString fgColor = wxGetApp().dark_mode() ? "#FFFFFF" : "#30373D";
        if (m_heightMin->GetTextCtrl()->GetValue().ToStdString() == m_defaultLimitData->value1 &&
            m_heightMax->GetTextCtrl()->GetValue().ToStdString() == m_defaultLimitData->value2) {
            m_heightLabel->SetForegroundColour(fgColor);
        } else {
            m_heightLabel->SetForegroundColour("#FF870A");
        }
    } else {
        m_heightLabel->SetForegroundColour("#FF870A");
    }
    m_heightLabel->Refresh();
}

void HeightLimitItem::updateSpeedLabelColor()
{
    if (m_defaultLimitData != nullptr) {
        wxString fgColor = wxGetApp().dark_mode() ? "#FFFFFF" : "#30373D";
        if (m_speed->GetTextCtrl()->GetValue().ToStdString() == m_defaultLimitData->speed1) {
            m_speedLabel->SetForegroundColour(fgColor);
        } else {
            m_speedLabel->SetForegroundColour("#FF870A");
        }
    } else {
        m_speedLabel->SetForegroundColour("#FF870A");
    }
    m_speedLabel->Refresh();
}

void HeightLimitItem::updateAccLabelColor()
{
    if (m_defaultLimitData != nullptr) {
        wxString fgColor = wxGetApp().dark_mode() ? "#FFFFFF" : "#30373D";
        if (m_Acceleration->GetTextCtrl()->GetValue().ToStdString() == m_defaultLimitData->Acc1) {
            m_AccelerationLabel->SetForegroundColour(fgColor);
        } else {
            m_AccelerationLabel->SetForegroundColour("#FF870A");
        }
    } else {
        m_AccelerationLabel->SetForegroundColour("#FF870A");
    }
    m_AccelerationLabel->Refresh();
}

AccelerationAndSpeedLimitPanel::AccelerationAndSpeedLimitPanel(const std::string& type, wxWindow* parent) : 
    wxPanel(parent, wxID_ANY)
{
    m_type = type;
    this->SetMinSize(wxSize(FromDIP(760), FromDIP(736)));
    this->SetMaxSize(wxSize(FromDIP(760), FromDIP(736)));
    //wxGetApp().UpdateDarkUI(this);
    wxString bgColor = wxGetApp().dark_mode() ? "#313131" : "#FFFFFF";
    wxString fgColor = wxGetApp().dark_mode() ? "#FFFFFF" : "#30373D";

    auto main_sizer = new wxBoxSizer(wxVERTICAL);
    m_scrolled_window = new wxScrolledWindow(this);
    m_scrolled_window->SetBackgroundColour(wxGetApp().dark_mode() ?"#2B2B2B":"#EFF0F6");
    m_scrolled_window->SetScrollbars(1, 20, 1, 2);
    m_scrolled_window->SetWindowStyle(wxBORDER_NONE);

    m_scrolled_window_sizer = new wxBoxSizer(wxVERTICAL);

    //  列表头
    auto panel = new wxPanel(m_scrolled_window, wxID_ANY);
    panel->SetMinSize(wxSize(FromDIP(728), FromDIP(48)));
    panel->SetMaxSize(wxSize(FromDIP(728), FromDIP(48)));
    panel->SetBackgroundColour(bgColor);
    m_headPanelSizer = new wxBoxSizer(wxHORIZONTAL);
    panel->SetSizer(m_headPanelSizer);
    
    auto label = new wxStaticText(panel, wxID_ANY, _L("Weight limit speed and acceleration Enable"));
    if (m_type == "speed_limit_to_height_enable") {
        label->SetLabelText(_L("Height limit speed and acceleration Enable"));
    }
    label->SetFont(Label::Body_13);
    label->SetMinSize(wxSize(FromDIP(175), FromDIP(20)));
    label->SetMaxSize(wxSize(FromDIP(175), FromDIP(20)));
    wxSize size = GetTextExtent(label->GetLabel());
    if (size.GetWidth() > FromDIP(180)) {
        label->SetMinSize(wxSize(FromDIP(175), FromDIP(30)));
        label->SetMaxSize(wxSize(FromDIP(175), FromDIP(30)));
    }
    label->SetForegroundColour(fgColor);
    m_headPanelSizer->Add(label, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(15));
    m_checkbox = new ::CheckBox(panel);
    m_checkbox->SetMinSize(wxSize(FromDIP(24), FromDIP(24)));
    m_checkbox->SetMaxSize(wxSize(FromDIP(24), FromDIP(24)));
    m_checkbox->setItemClickedCb([this]() { 
            checkIsShowResetBtn();
        });
    m_headPanelSizer->Add(m_checkbox, 1, wxLEFT | wxALIGN_CENTER_VERTICAL);

    m_resetBtn = new HoverBorderIcon(panel, _L("Add"),
        wxGetApp().dark_mode() ? "undo" : "undo", 
        wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    m_resetBtn->SetMinSize(wxSize(FromDIP(24), FromDIP(24)));
    m_resetBtn->SetMaxSize(wxSize(FromDIP(24), FromDIP(24)));
    m_resetBtn->SetBorderColorNormal(wxColour(bgColor));
    m_resetBtn->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(bgColor), StateColor::Pressed),
                                          std::pair<wxColour, int>(wxColour(bgColor), StateColor::Hovered),
                                          std::pair<wxColour, int>(wxColour(bgColor), StateColor::Normal)));
    m_resetBtn->Hide();
    m_resetBtn->Bind(wxEVT_LEFT_DOWN, &AccelerationAndSpeedLimitPanel::on_reset_btn_clicked, this);
    m_headPanelSizer->Add(m_resetBtn, 1, wxLEFT | wxALIGN_CENTER_VERTICAL);
    m_headPanelSizer->AddStretchSpacer();

    m_addBtn = new HoverBorderIcon(panel, wxEmptyString,
        wxGetApp().dark_mode() ? "addMaterial_dark_default" : "addMaterial_light_default", 
        wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    m_addBtn->setDisableIcon(wxGetApp().dark_mode() ? "addMaterial_dark_default_disable" : "addMaterial_light_default_disable", 13);
    m_addBtn->SetMinSize(wxSize(FromDIP(24), FromDIP(24)));
    m_addBtn->SetMaxSize(wxSize(FromDIP(24), FromDIP(24)));
    m_addBtn->SetBorderColorNormal(wxColour(bgColor));
    m_addBtn->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(bgColor), StateColor::Pressed),
                                             std::pair<wxColour, int>(wxColour(bgColor), StateColor::Hovered),
                                             std::pair<wxColour, int>(wxColour(bgColor), StateColor::Normal)));
    m_addBtn->Bind(wxEVT_LEFT_DOWN, &AccelerationAndSpeedLimitPanel::on_add_btn_clicked, this);
    m_headPanelSizer->Add(m_addBtn, 1, wxRight | wxALIGN_CENTER_VERTICAL, FromDIP(8));

    m_scrolled_window_sizer->Add(panel, 1, wxEXPAND | wxALIGN_CENTER_HORIZONTAL);
    m_scrolled_window->SetSizer(m_scrolled_window_sizer);
    main_sizer->Add(m_scrolled_window, 1, wxEXPAND);
    SetSizer(main_sizer);

}

void AccelerationAndSpeedLimitPanel::build_panel(bool bDefaultCheckbox, const ConfigOptionString& defaultData,
    bool bCheckbox, const ConfigOptionString& data)
{ 
    m_bDefaultCheckbox = bDefaultCheckbox;
    m_defaultData = defaultData;
    m_checkbox->SetValue(bCheckbox);
    m_vtLimitData.clear();

    parseLimitStr(defaultData.serialize(), m_vtDefaultLimitData);
    parseLimitStr(data.serialize(), m_vtLimitData);

    int i = 0;
    for (auto& limitData : m_vtLimitData) {    
        create_limit_item(&limitData, (i < m_vtDefaultLimitData.size())?&m_vtDefaultLimitData[i]:nullptr);
        i++;
    }
    checkIsShowResetBtn();
}

bool AccelerationAndSpeedLimitPanel::getCheckbox() {
    return m_checkbox->GetValue();
}

std::string AccelerationAndSpeedLimitPanel::serialize() { 
    bool isEqualDefaultValue = true;
    std::string data = "["; 
    int size = m_scrolled_window_sizer->GetItemCount();
    if (size <= m_vtDefaultLimitData.size()) {
        isEqualDefaultValue = false;
    }

    for (int i = 1; i < size; ++i) {
        if (i != 1) {
            data += ",";
        }
        if (m_type == "acceleration_limit_mess_enable") {
            void * item = getLimitItem(i);
            if (item != nullptr) {

                WeightLimitItem* limitItem = static_cast<WeightLimitItem*>(item);
                data += limitItem->serialize();
                if (!(limitItem->getEditLimitData()==m_vtDefaultLimitData[i - 1])) {
                    isEqualDefaultValue = false;
                }
            }
        } else if (m_type == "speed_limit_to_height_enable") {
            void* item = getLimitItem(i);
            if (item != nullptr) {
                HeightLimitItem* limitItem = static_cast<HeightLimitItem*>(item);
                data += limitItem->serialize();
                if (!(limitItem->getEditLimitData()==m_vtDefaultLimitData[i - 1])) {
                    isEqualDefaultValue = false;
                }
            }
        }
    }
    data += "]";

    if (isEqualDefaultValue) {
        return m_defaultData;
    }
    if (size == 1) {
        data = "";
    }
    return data;
}

void AccelerationAndSpeedLimitPanel::create_limit_item(WeightLimitItem::LimitData* limitData/* = nullptr*/, const WeightLimitItem::LimitData* defaultLimitData/* = nullptr*/)
{
    if (m_type == "acceleration_limit_mess_enable") {
        auto item = new WeightLimitItem(this, m_scrolled_window, m_scrolled_window_sizer->GetItemCount());
        if (limitData) {
            item->updateItemData(limitData, defaultLimitData);
        }
        item->setDelBtnClickedCb([this](int idx) {
            if (idx <= 0 || static_cast<size_t>(idx) >= m_scrolled_window_sizer->GetItemCount())
                return;
            delLimitItem(idx);
            m_scrolled_window_sizer->Layout();
            this->GetSizer()->Layout();
        });
        m_scrolled_window_sizer->Add(item, 1, wxEXPAND | wxALIGN_CENTER_HORIZONTAL|wxTOP, FromDIP(8));
        item->showSubCtrl();
        item->Show();
        m_scrolled_window_sizer->Layout();
    } else if (m_type == "speed_limit_to_height_enable") {
        auto item = new HeightLimitItem(this, m_scrolled_window, m_scrolled_window_sizer->GetItemCount());
        if (limitData) {
            item->updateItemData(limitData, defaultLimitData);
        }
        item->setDelBtnClickedCb([this](int idx) {
            if (idx <= 0 || static_cast<size_t>(idx) >= m_scrolled_window_sizer->GetItemCount())
                return;
            delLimitItem(idx);
            m_scrolled_window_sizer->Layout();
            this->GetSizer()->Layout();
        });
        m_scrolled_window_sizer->Add(item, 1, wxEXPAND | wxALIGN_CENTER_HORIZONTAL | wxTOP, FromDIP(8));
        item->showSubCtrl();
        item->Show();
        m_scrolled_window_sizer->Layout();
    }
}

void AccelerationAndSpeedLimitPanel::parseLimitStr(std::string str,std::vector<WeightLimitItem::LimitData>& outData)
{
    //[[0.5,1.0,100,6000,220],[1.0,1.5,80,5500,210],[1.5,2.0,60,5000,200]]
    int len = str.length();
    if (len <= 3)
        return;

    int no = str.find_first_of('[');
    if (no < 0 || no >= len)
        return;

    str = str.substr(no + 1, str.length());
    no  = str.find_last_of(']');
    if (no < 0 || no >= len)
        return;

    str = str.substr(0, no);

     int findIndex1 = str.find(']');
     while (findIndex1 >= 0 && findIndex1 < str.size()) {
         WeightLimitItem::LimitData limitData;

        std::string temp = str.substr(0, findIndex1);
        str              = str.substr(findIndex1 + 1, str.length());

        int findIndex = temp.find_last_of('[');
        if (findIndex < 0 || findIndex >= temp.length())
            continue;
        temp = temp.substr(findIndex + 1, temp.length());

        findIndex = temp.find(',');
        std::vector<std::string> data;
        std::string         str1;
        while (findIndex >= 0 && findIndex < temp.length()) {
            str1 = temp.substr(0, findIndex);
            temp = temp.substr(findIndex + 1, temp.length());
            data.push_back(str1.c_str());
            findIndex = temp.find(',');

            if (findIndex < 0 || findIndex >= temp.length()) {
                data.push_back(temp.c_str());
            }
        }

        if (data.size() == 4) {
            limitData.value1 = data[0];
            limitData.value2 = data[1];
            limitData.speed1 = data[2];
            limitData.Acc1   = data[3];
            // limitData.data.temp;
        } else if (data.size() == 5) {
            limitData.value1 = data[0];
            limitData.value2 = data[1];
            limitData.speed1 = data[2];
            limitData.Acc1   = data[3];
            limitData.Temp1  = data[4];
        }
        outData.push_back(limitData);

        findIndex1 = str.find(']');
    }
}

void AccelerationAndSpeedLimitPanel::on_reset_btn_clicked(wxEvent&)
{
    while (m_scrolled_window_sizer->GetItemCount() > 1) {
        wxSizerItem* item = m_scrolled_window_sizer->GetItem(static_cast<size_t>(1));
        wxWindow* child = item != nullptr ? item->GetWindow() : nullptr;
        if (!m_scrolled_window_sizer->Detach(1))
            break;
        if (child != nullptr)
            child->Destroy();
    }

    m_scrolled_window_sizer->Layout();
    this->GetSizer()->Layout();

    m_checkbox->SetValue(m_bDefaultCheckbox);
    for (auto& limitData : m_vtDefaultLimitData) {
        create_limit_item(&limitData, &limitData);
    }
    m_resetBtn->Hide();

    if (m_scrolled_window_sizer->GetItemCount() < 7 && !m_addBtn->IsEnabled()) {
        m_addBtn->setEnable(true);
        m_addBtn->Enable(true);
    }

    m_scrolled_window_sizer->Layout();
    this->GetSizer()->Layout();
}

void AccelerationAndSpeedLimitPanel::on_add_btn_clicked(wxEvent&) 
{
    if (m_scrolled_window_sizer->GetItemCount() >= 7) {
        return;
    }
    create_limit_item();
    m_scrolled_window_sizer->Layout();
    this->GetSizer()->Layout();

    checkIsShowResetBtn();
    if (m_scrolled_window_sizer->GetItemCount() >= 7) {
        m_addBtn->setEnable(false);
        m_addBtn->Enable(false);
    }
}

void* AccelerationAndSpeedLimitPanel::getLimitItem(size_t idx)
{
    wxSizerItem* item = m_scrolled_window_sizer->GetItem(idx);
    if (item && item->IsWindow()) {
        return item->GetWindow();
    }
    return nullptr;
}

void AccelerationAndSpeedLimitPanel::delLimitItem(size_t idx) {
    wxSizerItem* item = m_scrolled_window_sizer->GetItem(idx);
    if (idx == 0 || item == nullptr || !item->IsWindow())
        return;

    wxWindow* child = item->GetWindow();
    if (!m_scrolled_window_sizer->Detach(idx))
        return;
    child->Destroy();

    const size_t itemCount = m_scrolled_window_sizer->GetItemCount();
    for (size_t i = idx; i < itemCount; ++i) {
        wxWindow* remainingChild = m_scrolled_window_sizer->GetItem(i)->GetWindow();
        if (m_type == "acceleration_limit_mess_enable") {
            static_cast<WeightLimitItem*>(remainingChild)->updateIdx(i);
        } else if (m_type == "speed_limit_to_height_enable") {
            static_cast<HeightLimitItem*>(remainingChild)->updateIdx(i);
        } else {
            return;
        }
    }
    if (m_scrolled_window_sizer->GetItemCount() < 7) {
        m_addBtn->setEnable(true);
        m_addBtn->Enable(true);
    }
}

void AccelerationAndSpeedLimitPanel::checkIsShowResetBtn()
{
    std::string data             = serialize();
    m_vtLimitData.clear();
    parseLimitStr(data, m_vtLimitData);

    if (m_bDefaultCheckbox != m_checkbox->GetValue()) {
        m_resetBtn->Show();
    } else {
        if (m_vtDefaultLimitData.size() != m_vtLimitData.size()) {
            m_resetBtn->Show();
        } else {
            for (int i = 0; i < m_vtDefaultLimitData.size(); ++i) {
                if (m_vtDefaultLimitData[i].value1 != m_vtLimitData[i].value1 || 
                    m_vtDefaultLimitData[i].value2 != m_vtLimitData[i].value2 ||
                    m_vtDefaultLimitData[i].Acc1 != m_vtLimitData[i].Acc1 ||
                    m_vtDefaultLimitData[i].speed1 != m_vtLimitData[i].speed1
                    ) {
                    m_resetBtn->Show();
                        break;
                } else {
                    m_resetBtn->Hide();
                }
            }
        }
    }
    m_headPanelSizer->Layout();
}

AccelerationAndSpeedLimitDialog::AccelerationAndSpeedLimitDialog(const std::string& type, const wxString& title, wxWindow* parent)
    : DPIDialog(parent, wxID_ANY, title.ToStdString(), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
    m_type = type;
}

void AccelerationAndSpeedLimitDialog::build_dialog(bool bDefaultCheckbox, const ConfigOptionString& defaultData,
    bool bCheckbox, const ConfigOptionString& data)
{
    BOOST_LOG_TRIVIAL(warning) << "AccelerationAndSpeedLimitDialog::build_dialog type=" << m_type
                               << ",bDefaultCheckbox=" << bDefaultCheckbox << ",defaultData=" << defaultData.serialize()
                               << ",bCheckbox=" << bCheckbox << ",data=" << data.serialize();
    wxString bgColor = wxGetApp().dark_mode() ? "#313131" : "#FFFFFF";
    wxString bgColor2 = wxGetApp().dark_mode() ? "#2B2B2B" : "#EFF0F6";
    wxString fgColor = wxGetApp().dark_mode() ? "#FFFFFF" : "#30373D";
    SetFont(wxGetApp().normal_font());
    wxGetApp().UpdateDlgDarkUI(this);
    this->SetBackgroundColour(bgColor2);

    //SetBackgroundColour(*wxWHITE);

    // paraseLimitStr(data.serialize(), )

    auto main_sizer = new wxBoxSizer(wxVERTICAL);

    
    m_panel = new AccelerationAndSpeedLimitPanel(m_type, this);
    m_panel->build_panel(bDefaultCheckbox, defaultData, bCheckbox, data);

    main_sizer->Add(m_panel, 1, wxEXPAND);
    auto panel = new wxPanel(this, wxID_ANY);
    panel->SetMinSize(wxSize(FromDIP(760), FromDIP(48)));
    panel->SetMaxSize(wxSize(FromDIP(760), FromDIP(48)));
    panel->SetBackgroundColour(bgColor);
    wxBoxSizer* panelSizer = new wxBoxSizer(wxHORIZONTAL);
    panel->SetSizer(panelSizer);

    Button* cancelBtn = new Button(panel, _L("Cancel"));
    cancelBtn->SetMinSize(wxSize(FromDIP(104), FromDIP(32)));
    cancelBtn->SetMaxSize(wxSize(FromDIP(104), FromDIP(32)));
    cancelBtn->SetFont(Label::Body_13);
    cancelBtn->SetBorderColor(wxColour(wxGetApp().dark_mode() ? "#6C6E71" : "#A6ACB4"));
    cancelBtn->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(bgColor2), StateColor::Pressed),
                                             std::pair<wxColour, int>(wxColour(bgColor2), StateColor::Hovered),
                                             std::pair<wxColour, int>(wxColour(bgColor2), StateColor::Normal)));
    cancelBtn->Bind(wxEVT_BUTTON, [this](wxEvent&) {
        EndModal(wxID_CANCEL);
    });
    panelSizer->AddStretchSpacer();
    panelSizer->Add(cancelBtn, 1, wxALIGN_CENTER_VERTICAL);

    Button* okBtn = new Button(panel, _L("Ok"));
    okBtn->SetMinSize(wxSize(FromDIP(104), FromDIP(32)));
    okBtn->SetMaxSize(wxSize(FromDIP(104), FromDIP(32)));
    okBtn->SetFont(Label::Body_13);
    okBtn->SetBorderColor(wxColour(wxGetApp().dark_mode() ? "#6C6E71" : "#A6ACB4"));
    okBtn->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour("#1FCA63"), StateColor::Pressed),
                                         std::pair<wxColour, int>(wxColour("#1FCA63"), StateColor::Hovered),
                                         std::pair<wxColour, int>(wxColour("#1FCA63"), StateColor::Normal)));
    okBtn->Bind(wxEVT_BUTTON, [this](wxEvent&) {
        if (s_hasShowErrorDlg) {
            return;
        }
        m_bCheckbox = m_panel->getCheckbox();
        m_outData = m_panel->serialize();
        EndModal(wxID_OK);
        });
    panelSizer->Add(okBtn, 1, wxLEFT|wxALIGN_CENTER_VERTICAL, FromDIP(8));
    panelSizer->AddStretchSpacer();
    
    main_sizer->Add(panel, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM);

    SetSizer(main_sizer);
    SetMinSize(GetSize());
    main_sizer->SetSizeHints(this);

    this->Bind(wxEVT_CLOSE_WINDOW, ([this](wxCloseEvent& evt) { 
        EndModal(wxID_CANCEL); 
    }));
}

bool AccelerationAndSpeedLimitDialog::getCheckbox() { 
    return m_bCheckbox;
}

const std::string& AccelerationAndSpeedLimitDialog::getData()
{
    BOOST_LOG_TRIVIAL(warning) << "AccelerationAndSpeedLimitDialog::getData type=" << m_type
                               << ",bCheckbox=" << m_bCheckbox << ",data=" << m_outData;
    return m_outData; 
}

void AccelerationAndSpeedLimitDialog::on_dpi_changed(const wxRect& suggested_rect)
{
    //const int& em = em_unit();
    //m_panel->m_shape_options_book->SetMinSize(wxSize(25 * em, -1));

    //for (auto og : m_panel->m_optgroups)
    //    og->msw_rescale();

    //const wxSize& size = wxSize(50 * em, -1);

    //SetMinSize(size);
    //SetSize(size);

    //Refresh();
}


} // GUI
} // Slic3r
