#include <algorithm>
#include <limits>
#include <sstream>
#include <chrono>
//#include "libslic3r/FlushVolCalc.hpp"
#include "ObjColorDialog.hpp"
#include "BitmapCache.hpp"
#include "GUI.hpp"
#include "format.hpp"
#include "I18N.hpp"
#include "GUI_App.hpp"
#include "MsgDialog.hpp"
#include "Widgets/Button.hpp"
#include "slic3r/Utils/ColorSpaceConvert.hpp"
#include "MainFrame.hpp"
#include "libslic3r/Config.hpp"
#include "libslic3r/Model.hpp"
#include "BitmapComboBox.hpp"
#include "Widgets/ComboBox.hpp"
#include <wx/sizer.h>

#include "libslic3r/ObjColorUtils.hpp"
#include "libslic3r/common_header/common_header.h"

using namespace Slic3r;
using namespace Slic3r::GUI;

int objcolor_scale(const int val) { return val * Slic3r::GUI::wxGetApp().em_unit() / 10; }
int OBJCOLOR_ITEM_WIDTH() { return objcolor_scale(30); }
static const wxColour g_text_color = wxColour(107, 107, 107, 255);
const int HEADER_BORDER  = 5;
const int CONTENT_BORDER = 3;
const int PANEL_WIDTH = 648;
const int COLOR_LABEL_WIDTH = 180;
#define ICON_SIZE               wxSize(FromDIP(20), FromDIP(20))
#define MIN_OBJCOLOR_DIALOG_WIDTH FromDIP(648)
#define FIX_SCROLL_HEIGTH         FromDIP(492)
#define BTN_SIZE                wxSize(FromDIP(104), FromDIP(32))
#define BTN_GAP                 FromDIP(20)

static void update_ui(wxWindow* window)
{
    Slic3r::GUI::wxGetApp().UpdateDarkUI(window);
}

static const char g_min_cluster_color = 1;
// g_max_color: UI dialog limit for number of filament colors in the OBJ color matching dialog.
// Tied to CONST_FILAMENTS.size() - 1, so extending the table (e.g. 17 -> 64) automatically
// raises the dialog limit. TriangleSelector's bitstream layer (serialize/deserialize)
// supports states up to EnforcerBlockerType::ExtruderMax=255 via multi-nibble base-15 encoding,
// so extending CONST_FILAMENTS does not require changes to libslic3r/TriangleSelector.*.
// The OBJ dialog is primarily for color matching, not for defining system-wide extruder count.
static const int g_max_color = static_cast<int>(Slic3r::CONST_FILAMENTS.size() - 1);
// Color distance threshold for "close match" (DeltaE76 in LAB color space)
// Values < 10.0 are considered close matches to existing filaments
static const float COLOR_CLOSE_MATCH_THRESHOLD = 10.0f;
const  StateColor ok_btn_bg(std::pair<wxColour, int>(wxColour(0, 137, 123), StateColor::Pressed),
                     std::pair<wxColour, int>(wxColour(38, 166, 154), StateColor::Hovered),
                     std::pair<wxColour, int>(wxColour(0, 150, 136), StateColor::Normal));
const StateColor  ok_btn_disable_bg(std::pair<wxColour, int>(wxColour(205, 201, 201), StateColor::Pressed),
                                   std::pair<wxColour, int>(wxColour(205, 201, 201), StateColor::Hovered),
                                   std::pair<wxColour, int>(wxColour(205, 201, 201), StateColor::Normal));
wxBoxSizer* ObjColorDialog::create_btn_sizer(long flags)
{
    auto btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    btn_sizer->AddStretchSpacer();

    StateColor ok_btn_bd(
        std::pair<wxColour, int>(wxColour(0, 150, 136), StateColor::Normal)
    );
    StateColor ok_btn_text(
        std::pair<wxColour, int>(wxColour(255, 255, 254), StateColor::Normal)
    );
    StateColor cancel_btn_bg(
        std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Pressed),
        //std::pair<wxColour, int>(wxColour(238, 238, 238), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Normal)
    );
    StateColor cancel_btn_bd_(
        std::pair<wxColour, int>(wxColour(38, 46, 48), StateColor::Normal)
    );
    StateColor cancel_btn_text(
        std::pair<wxColour, int>(wxColour(38, 46, 48), StateColor::Normal)
    );
    StateColor calc_btn_bg(
        std::pair<wxColour, int>(wxColour(0, 137, 123), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(38, 166, 154), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(0, 150, 136), StateColor::Normal)
    );
    StateColor calc_btn_bd(
        std::pair<wxColour, int>(wxColour(0, 150, 136), StateColor::Normal)
    );
    StateColor calc_btn_text(
        std::pair<wxColour, int>(wxColour(255, 255, 254), StateColor::Normal)
    );
    if (flags & wxCANCEL) {
        Button* cancel_btn = new Button(this, _L("Cancel"));
        cancel_btn->SetMinSize(BTN_SIZE);
        cancel_btn->SetCornerRadius(FromDIP(4));
        cancel_btn->SetBackgroundColor(cancel_btn_bg);
        cancel_btn->SetBorderColor(cancel_btn_bd_);
        cancel_btn->SetTextColor(cancel_btn_text);
        cancel_btn->SetId(wxID_CANCEL);
        btn_sizer->Add(cancel_btn, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, BTN_GAP);
        m_button_list[wxCANCEL] = cancel_btn;
    }
    if (flags & wxOK) {
        Button* ok_btn = new Button(this, _L("OK"));
        ok_btn->SetMinSize(BTN_SIZE);
        ok_btn->SetCornerRadius(FromDIP(4));
        ok_btn->Enable(false);
        ok_btn->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour("#1FCA63"), StateColor::Pressed),
                                             std::pair<wxColour, int>(wxColour("#1FCA63"), StateColor::Hovered),
                                             std::pair<wxColour, int>(wxColour("#1FCA63"), StateColor::Disabled),
                                             std::pair<wxColour, int>(wxColour("#1FCA63"), StateColor::Normal)));
        ok_btn->SetBorderColor(ok_btn_bd);
        ok_btn->SetTextColor(ok_btn_text);
        ok_btn->SetFocus();
        ok_btn->SetId(wxID_OK);
        btn_sizer->Add(ok_btn, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, BTN_GAP);
        m_button_list[wxOK] = ok_btn;
    }
    btn_sizer->AddStretchSpacer();
    return btn_sizer;
}

void ObjColorDialog::on_dpi_changed(const wxRect &suggested_rect)
{
    for (auto button_item : m_button_list)
    {
        if (button_item.first == wxRESET)
        {
            button_item.second->SetMinSize(wxSize(FromDIP(75), FromDIP(24)));
            button_item.second->SetCornerRadius(FromDIP(12));
        }
        if (button_item.first == wxOK) {
            button_item.second->SetMinSize(BTN_SIZE);
            button_item.second->SetCornerRadius(FromDIP(12));
        }
        if (button_item.first == wxCANCEL) {
            button_item.second->SetMinSize(BTN_SIZE);
            button_item.second->SetCornerRadius(FromDIP(12));
        }
    }
    m_panel_ObjColor->msw_rescale();
    this->Refresh();
};

ObjColorDialog::ObjColorDialog(wxWindow *                      parent,
                               std::vector<Slic3r::RGBA> &     input_colors,
                               bool                            is_single_color,
                               const std::vector<std::string> &extruder_colours,
                               std::vector<unsigned char> &    filament_ids,
                               unsigned char &                 first_extruder_id)
    : DPIDialog(parent ? parent : static_cast<wxWindow *>(wxGetApp().mainframe),
                wxID_ANY,
                _(L("Obj file Import color")),
                wxDefaultPosition,
                wxDefaultSize,
                wxDEFAULT_DIALOG_STYLE /* | wxRESIZE_BORDER*/)
    , m_filament_ids(filament_ids)
    , m_first_extruder_id(first_extruder_id)
{
    std::string icon_path = (boost::format("%1%/images/%2%.ico") % Slic3r::resources_dir() % Slic3r::CxBuildInfo::getIconName()).str();
    SetIcon(wxIcon(Slic3r::encode_path(icon_path.c_str()), wxBITMAP_TYPE_ICO));

    auto m_line_top = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    m_line_top->SetBackgroundColour(wxColour(166, 169, 170));

    this->SetBackgroundColour(*wxWHITE);
    this->SetMinSize(wxSize(MIN_OBJCOLOR_DIALOG_WIDTH, -1));

    m_panel_ObjColor = new ObjColorPanel(this, input_colors, is_single_color, extruder_colours, filament_ids, first_extruder_id);

    auto main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->Add(m_line_top, 0, wxEXPAND, 0);
    // set min sizer width according to extruders count
    //auto sizer_width = (int) (2.8 * OBJCOLOR_ITEM_WIDTH());
    //sizer_width      = sizer_width > MIN_OBJCOLOR_DIALOG_WIDTH ? sizer_width : MIN_OBJCOLOR_DIALOG_WIDTH;
    //main_sizer->SetMinSize(wxSize(sizer_width, -1));
    main_sizer->Add(m_panel_ObjColor, 1, wxEXPAND | wxALL, 0);

    auto btn_sizer = create_btn_sizer(wxOK | wxCANCEL);
    {
        const StateColor ok_btn_green(
            std::pair<wxColour, int>(wxColour("#1FCA63"), StateColor::Pressed),
            std::pair<wxColour, int>(wxColour("#1FCA63"), StateColor::Hovered),
            std::pair<wxColour, int>(wxColour("#1FCA63"), StateColor::Disabled),
            std::pair<wxColour, int>(wxColour("#1FCA63"), StateColor::Normal));
        m_button_list[wxOK]->Bind(wxEVT_UPDATE_UI, ([this, ok_btn_green](wxUpdateUIEvent &e) {
           if (m_panel_ObjColor->is_ok()) {
               m_panel_ObjColor->update_warning_text(_L("Note:The color has been selected, you can choose OK to continue or manually adjust it."));
           } else {
               m_panel_ObjColor->update_warning_text("");
           }
           if (m_panel_ObjColor->is_ok() == m_button_list[wxOK]->IsEnabled()) { return; }
           m_button_list[wxOK]->Enable(m_panel_ObjColor->is_ok());
           m_button_list[wxOK]->SetBackgroundColor(m_panel_ObjColor->is_ok() ? ok_btn_green : ok_btn_disable_bg);
         }));
    }
    main_sizer->Add(btn_sizer, 0, wxBOTTOM | wxRIGHT| wxTOP | wxEXPAND, BTN_GAP);
    SetSizer(main_sizer);
    main_sizer->SetSizeHints(this);

    if (this->FindWindowById(wxID_OK, this)) {
        this->FindWindowById(wxID_OK, this)->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {// if OK button is clicked..
              m_panel_ObjColor->update_filament_ids();
              EndModal(wxID_OK);
            }, wxID_OK);
    }
    if (this->FindWindowById(wxID_CANCEL, this)) {
        update_ui(static_cast<wxButton*>(this->FindWindowById(wxID_CANCEL, this)));
        this->FindWindowById(wxID_CANCEL, this)->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxCANCEL); });
    }
    this->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& e) { EndModal(wxCANCEL); });

    wxGetApp().UpdateDlgDarkUI(this);
}
RGBA     convert_to_rgba(const wxColour &color)
{
    RGBA rgba;
    rgba[0] = std::clamp(color.Red() / 255.f, 0.f, 1.f);
    rgba[1] = std::clamp(color.Green() / 255.f, 0.f, 1.f);
    rgba[2] = std::clamp(color.Blue() / 255.f, 0.f, 1.f);
    rgba[3] = std::clamp(color.Alpha() / 255.f, 0.f, 1.f);
    return rgba;
}
wxColour convert_to_wxColour(const RGBA &color)
{
    auto     r = std::clamp((int) (color[0] * 255.f), 0, 255);
    auto     g = std::clamp((int) (color[1] * 255.f), 0, 255);
    auto     b = std::clamp((int) (color[2] * 255.f), 0, 255);
    auto     a = std::clamp((int) (color[3] * 255.f), 0, 255);
    wxColour wx_color(r,g,b,a);
    return wx_color;
}
// This panel contains all control widgets for both simple and advanced mode (these reside in separate sizers)
ObjColorPanel::ObjColorPanel(wxWindow *                       parent,
                             std::vector<Slic3r::RGBA>&       input_colors,
                             bool                             is_single_color,
                             const std::vector<std::string>&  extruder_colours,
                             std::vector<unsigned char> &    filament_ids,
                             unsigned char &                 first_extruder_id)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize /*,wxBORDER_RAISED*/)
    , m_input_colors(input_colors)
    , m_filament_ids(filament_ids)
    , m_first_extruder_id(first_extruder_id)
{
    if (input_colors.size() == 0) { return; }
    for (const std::string& color : extruder_colours) {
        m_colours.push_back(wxColor(color));
    }
    // 截断 m_colours 到 g_max_color：颜色匹配 combo 仅显示前 g_max_color 个槽位。
    // extruder_colours 顺序为 [物理 1..N, 混合 enabled 顺序 1..M]，物理优先填充。
    if ((int)m_colours.size() > g_max_color) {
        m_colours.resize(g_max_color);
    }
    //deal input_colors
    m_input_colors_size = input_colors.size();
    for (size_t i = 0; i < input_colors.size(); i++) {
        if (color_is_equal(input_colors[i] , UNDEFINE_COLOR)) { // not define color range:0~1
            input_colors[i]=convert_to_rgba( m_colours[0]);
        }
    }
    if (is_single_color && input_colors.size() >=1) {
        m_cluster_colors_from_algo.emplace_back(input_colors[0]);
        m_cluster_colours.emplace_back(convert_to_wxColour(input_colors[0]));
        m_cluster_labels_from_algo.reserve(m_input_colors_size);
        for (size_t i = 0; i < m_input_colors_size; i++) {
            m_cluster_labels_from_algo.emplace_back(0);
        }
        m_cluster_mappings.resize(m_cluster_colors_from_algo.size());
        m_color_num_recommend = m_color_cluster_num_by_algo = m_cluster_colors_from_algo.size();
    } else {//cluster deal
        deal_algo(-1);
    }
    //end first cluster

    m_sizer = new wxBoxSizer(wxVERTICAL);
    this->SetMinSize(wxSize(FromDIP(680), FromDIP(716)));
    this->SetMaxSize(wxSize(FromDIP(680), FromDIP(716)));

    wxString bgColor = wxGetApp().dark_mode() ? "#313131" : "#FFFFFF";
    wxString bgColor2 = wxGetApp().dark_mode() ? "#2B2B2B" : "#EFF0F6";
    wxString bgColor3 = wxGetApp().dark_mode() ? "#6C6E71" : "#D0D4DE";
    wxString fgColor = wxGetApp().dark_mode() ? "#FFFFFF" : "#30373D";
    wxString fgColor2 = wxGetApp().dark_mode() ? "#F0F0FF" : "#30373D";
    this->SetBackgroundColour(*wxWHITE);

    m_sizer = new wxBoxSizer(wxVERTICAL);

    wxPanel* panel = new wxPanel(this, wxID_ANY);
    m_top_panel = panel;
    m_sizer->Add(panel, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP | wxALIGN_CENTER_VERTICAL, FromDIP(16));
    {
        panel->SetMinSize(wxSize(FromDIP(-1), FromDIP(128)));
        panel->SetMaxSize(wxSize(FromDIP(-1), FromDIP(128)));
        panel->SetFont(Label::Body_16);
        panel->SetBackgroundColour(bgColor);
        panel->SetForegroundColour(fgColor);
        wxBoxSizer* panelSizer = new wxBoxSizer(wxVERTICAL);
        m_top_panelSizer = panelSizer;
        panel->SetSizer(panelSizer);

        {
            // color cluster results
            //  指定颜色数量
            wxPanel* panel_line = new wxPanel(panel, wxID_ANY);
            panelSizer->Add(panel_line, 0, wxEXPAND | wxLEFT | wxTOP, FromDIP(16));
            panel_line->SetMinSize(wxSize(FromDIP(-1), FromDIP(30)));
            panel_line->SetMaxSize(wxSize(FromDIP(-1), FromDIP(30)));
            panel_line->SetFont(Label::Body_16);
            panel_line->SetBackgroundColour(bgColor);
            panel_line->SetForegroundColour(fgColor2);
            wxBoxSizer* panel_lineSizer = new wxBoxSizer(wxHORIZONTAL);
            panel_line->SetSizer(panel_lineSizer);
            wxStaticText* specify_color_cluster_title = new wxStaticText(panel_line, wxID_ANY, _L("Specify number of colors:"));
            panel_lineSizer->Add(specify_color_cluster_title, 0, wxEXPAND|wxTOP, FromDIP(5));

            wxPanel* borderPanel = new wxPanel(panel_line, wxID_ANY);
            borderPanel->SetBackgroundColour(bgColor);
            borderPanel->Bind(wxEVT_PAINT, [borderPanel, bgColor3](wxPaintEvent&) {
                wxPaintDC dc(borderPanel);
                dc.SetPen(wxPen(wxColour(bgColor3), borderPanel->FromDIP(1)));
                dc.SetBrush(*wxTRANSPARENT_BRUSH);
                wxSize sz = borderPanel->GetClientSize();
                dc.DrawRectangle(0, 0, sz.GetWidth(), sz.GetHeight());
            });
            wxBoxSizer* borderPanelSizer = new wxBoxSizer(wxHORIZONTAL);
            borderPanel->SetSizer(borderPanelSizer);
            m_color_cluster_num_by_user_ebox = new wxTextCtrl(borderPanel, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                                              wxSize(FromDIP(25), -1), wxTE_PROCESS_ENTER | wxNO_BORDER | wxTE_CENTER);
            m_color_cluster_num_by_user_ebox->SetBackgroundColour(bgColor);
            m_color_cluster_num_by_user_ebox->SetMinSize(wxSize(FromDIP(70), FromDIP(26)));
            m_color_cluster_num_by_user_ebox->SetMaxSize(wxSize(FromDIP(70), FromDIP(26)));
            m_color_cluster_num_by_user_ebox->SetValue(std::to_string(m_color_cluster_num_by_algo).c_str());
            { // event
                auto on_apply_color_cluster_text_modify = [this](wxEvent& e) {
                    wxString str    = m_color_cluster_num_by_user_ebox->GetValue();
                    int      number = wxAtoi(str);
                    if (number > m_color_num_recommend || number < g_min_cluster_color) {
                        number = number < g_min_cluster_color ? g_min_cluster_color : m_color_num_recommend;
                        str    = wxString::Format(("%d"), number);
                        m_color_cluster_num_by_user_ebox->SetValue(str);
                        MessageDialog dlg(nullptr,
                                          wxString::Format(_L("The color count should be in range [%d, %d]."), g_min_cluster_color,
                                                           m_color_num_recommend),
                                          _L("Warning"), wxICON_WARNING | wxOK);
                        dlg.ShowModal();
                    }
                    e.Skip();
                };
                m_color_cluster_num_by_user_ebox->Bind(wxEVT_TEXT_ENTER, on_apply_color_cluster_text_modify);
                m_color_cluster_num_by_user_ebox->Bind(wxEVT_KILL_FOCUS, on_apply_color_cluster_text_modify);
                m_color_cluster_num_by_user_ebox->Bind(wxEVT_COMMAND_TEXT_UPDATED, [this](wxCommandEvent&) {
                    wxString str    = m_color_cluster_num_by_user_ebox->GetValue();
                    int      number = wxAtof(str);
                    if (number > m_color_num_recommend || number < g_min_cluster_color) {
                        number = number < g_min_cluster_color ? g_min_cluster_color : m_color_num_recommend;
                        str    = wxString::Format(("%d"), number);
                        m_color_cluster_num_by_user_ebox->SetValue(str);
                        m_color_cluster_num_by_user_ebox->SetInsertionPointEnd();
                    }
                    if (m_last_cluster_num != number) {
                        deal_algo(number, true);
                        // After redraw_part_table(), we need to apply the matching strategy
                        deal_default_strategy();
                        Layout();
                        // Fit();
                        Refresh();
                        Update();
                        m_last_cluster_num = number;
                    }
                });
                m_color_cluster_num_by_user_ebox->Bind(wxEVT_CHAR, [this](wxKeyEvent& e) {
                    int      keycode    = e.GetKeyCode();
                    wxString input_char = wxString::Format("%c", keycode);
                    long     value;
                    if (!input_char.ToLong(&value))
                        return;
                    e.Skip();
                });
            }
            panel_lineSizer->AddSpacer(FromDIP(2));
            borderPanelSizer->AddSpacer(FromDIP(2));
            borderPanelSizer->Add(m_color_cluster_num_by_user_ebox, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, FromDIP(5));
            borderPanelSizer->AddSpacer(FromDIP(2));
            panel_lineSizer->Add(borderPanel, 0, wxALIGN_CENTER | wxEXPAND | wxALL, FromDIP(1));
            panel_lineSizer->AddSpacer(FromDIP(15));
            wxStaticText* recommend_color_cluster_title = new wxStaticText(panel_line, wxID_ANY,
                "(" + std::to_string(m_color_num_recommend) + " " +
                _L("Recommended ") + ")");
            panel_lineSizer->Add(recommend_color_cluster_title, 0, wxALIGN_CENTER | wxEXPAND | wxTOP, FromDIP(5));

            // 当前耗材丝颜色
            const int moreColorRowCount     = m_colours.size() > 20 ? (int)((m_colours.size() - 20 + 19) / 20) : 0;
            const int moreColorPanelHigh    = moreColorRowCount * (8 + 20);
            const int moreColorPanelDiffHigh = moreColorPanelHigh;
            wxBoxSizer* current_filaments_title_sizer = new wxBoxSizer(wxHORIZONTAL);
            wxStaticText* current_filaments_title = new wxStaticText(panel, wxID_ANY, _L("Current filament colors:"));
            current_filaments_title->SetFont(Label::Body_16);
            current_filaments_title->SetForegroundColour(fgColor2);
            current_filaments_title_sizer->Add(current_filaments_title, 0, wxALIGN_CENTER | wxALL, FromDIP(0));
            panelSizer->AddSpacer(FromDIP(10));
            panelSizer->Add(current_filaments_title_sizer, 0, wxEXPAND | wxLEFT, FromDIP(16));

            m_btnMoreColor = new HoverBorderIcon(panel, wxEmptyString, wxGetApp().dark_mode() ? "more_color" : "more_color_light",
                wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
            m_btnMoreColor->Hide();
            m_btnMoreColor->SetMinSize(wxSize(FromDIP(20), FromDIP(20)));
            m_btnMoreColor->SetMaxSize(wxSize(FromDIP(20), FromDIP(20)));
            m_btnMoreColor->SetBackgroundColour(wxColour(bgColor));
            m_btnMoreColor->SetBorderColorNormal(wxColour(bgColor));
            m_btnMoreColor->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(bgColor), StateColor::Pressed),
                std::pair<wxColour, int>(wxColour(bgColor), StateColor::Hovered),
                std::pair<wxColour, int>(wxColour(bgColor), StateColor::Normal)));
            m_btnMoreColor->Bind(wxEVT_LEFT_DOWN, [this, moreColorPanelDiffHigh](wxEvent&) {
                if (m_moreColorPanel != nullptr) {
                    m_moreColorPanel->Show();
                    m_btnMoreColor->Hide();
                    m_top_panel->SetMinSize(wxSize(FromDIP(-1), FromDIP(128 + moreColorPanelDiffHigh)));
                    m_top_panel->SetMaxSize(wxSize(FromDIP(-1), FromDIP(128 + moreColorPanelDiffHigh)));
                    this->Layout();
                }
                });

            wxBoxSizer* current_filaments_sizer = new wxBoxSizer(wxHORIZONTAL);
            wxBoxSizer* more_current_filaments_sizer = new wxBoxSizer(wxHORIZONTAL);
            m_moreColorPanel = new wxPanel(panel, wxID_ANY);
            m_moreColorPanel->Hide();
            m_moreColorPanel->SetMinSize(wxSize(FromDIP(-1), FromDIP(moreColorPanelHigh)));
            m_moreColorPanel->SetMaxSize(wxSize(FromDIP(-1), FromDIP(moreColorPanelHigh)));
            m_moreColorPanel->SetFont(Label::Body_16);
            m_moreColorPanel->SetBackgroundColour(bgColor);
            m_moreColorPanel->SetForegroundColour(fgColor2);
            wxBoxSizer* moreColorPanelSizer = new wxBoxSizer(wxVERTICAL);
            m_moreColorPanel->SetSizer(moreColorPanelSizer);
            moreColorPanelSizer->Add(more_current_filaments_sizer, 0, wxEXPAND | wxTOP, FromDIP(8));

            m_btnLessColor = new HoverBorderIcon(m_moreColorPanel, wxEmptyString, wxGetApp().dark_mode() ? "less_color" : "less_color_light",
                wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
            m_btnLessColor->SetMinSize(wxSize(FromDIP(20), FromDIP(20)));
            m_btnLessColor->SetMaxSize(wxSize(FromDIP(20), FromDIP(20)));
            m_btnLessColor->SetBackgroundColour(wxColour(bgColor));
            m_btnLessColor->SetBorderColorNormal(wxColour(bgColor));
            m_btnLessColor->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(bgColor), StateColor::Pressed),
                std::pair<wxColour, int>(wxColour(bgColor), StateColor::Hovered),
                std::pair<wxColour, int>(wxColour(bgColor), StateColor::Normal)));
            m_btnLessColor->Bind(wxEVT_LEFT_DOWN, [this](wxEvent&) {
                if (m_moreColorPanel != nullptr) {
                    m_moreColorPanel->Hide();
                    m_btnMoreColor->Show();
                    m_top_panel->SetMinSize(wxSize(FromDIP(-1), FromDIP(128)));
                    m_top_panel->SetMaxSize(wxSize(FromDIP(-1), FromDIP(128)));
                    this->Layout();
                }
                });

            for (size_t i = 0; i < m_colours.size(); i++) {
                if (i == 20) {
                    m_btnMoreColor->Show();
                    current_filaments_sizer->Add(m_btnMoreColor, 0, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(8));
                }

                if (i < 20) {
                    auto extruder_icon_sizer = create_extruder_icon_and_rgba_sizer(panel, i, m_colours[i]);
                    current_filaments_sizer->Add(extruder_icon_sizer, 0, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL | wxLEFT,
                        FromDIP(i == 0 ? 0 : FromDIP(8)));
                }
                else {
                    auto extruder_icon_sizer = create_extruder_icon_and_rgba_sizer(m_moreColorPanel, i, m_colours[i]);
                    if (i != 20 && (i - 20) % 20 == 0) {
                        more_current_filaments_sizer = new wxBoxSizer(wxHORIZONTAL);
                        moreColorPanelSizer->Add(more_current_filaments_sizer, 0, wxEXPAND | wxTOP, FromDIP(8));
                    }
                    more_current_filaments_sizer->Add(extruder_icon_sizer, 0, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL | wxLEFT,
                        FromDIP(i % 20 == 0 ? 0 : FromDIP(8)));
                }
            }
            if (m_colours.size() > 20) {
                more_current_filaments_sizer->Add(m_btnLessColor, 0, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(8));
            }
            panelSizer->AddSpacer(FromDIP(8));
            panelSizer->Add(current_filaments_sizer, 0, wxEXPAND | wxLEFT, FromDIP(16));
            panelSizer->Add(m_moreColorPanel, 0, wxEXPAND | wxLEFT, FromDIP(16));

        }
    }

    panel = new wxPanel(this, wxID_ANY);
    m_second_panel = panel;
    m_sizer->AddSpacer(FromDIP(10));
    m_sizer->Add(panel, 1, wxEXPAND | wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, FromDIP(16));
    panel->SetMinSize(wxSize(FromDIP(-1), FromDIP(700)));
    panel->SetMaxSize(wxSize(FromDIP(-1), FromDIP(700)));
    panel->SetFont(Label::Body_16);
    panel->SetBackgroundColour(bgColor);
    panel->SetForegroundColour(fgColor);
    wxBoxSizer* panelSizer = new wxBoxSizer(wxVERTICAL);
    m_second_panelSizer = panelSizer;
    panel->SetSizer(panelSizer);
    {

        // 重新匹配的耗材丝
        wxPanel* panel_line = new wxPanel(panel, wxID_ANY);
        panelSizer->Add(panel_line, 0, wxEXPAND | wxLEFT | wxTOP, FromDIP(16));
        panel_line->SetMinSize(wxSize(FromDIP(-1), FromDIP(28)));
        panel_line->SetMaxSize(wxSize(FromDIP(-1), FromDIP(28)));
        panel_line->SetFont(Label::Body_16);
        panel_line->SetBackgroundColour(bgColor);
        panel_line->SetForegroundColour(fgColor2);
        wxBoxSizer* panel_lineSizer = new wxBoxSizer(wxHORIZONTAL);
        panel_line->SetSizer(panel_lineSizer);
        wxStaticText* colors_left_title = new wxStaticText(panel_line, wxID_ANY, _L("Cluster colors"));
        panel_lineSizer->Add(colors_left_title, 0, wxEXPAND | wxTOP, FromDIP(5));

        auto calc_approximate_match_btn_sizer = create_approximate_match_btn_sizer(panel_line);
        auto calc_add_btn_sizer               = create_add_btn_sizer(panel_line);
        auto calc_reset_btn_sizer             = create_reset_btn_sizer(panel_line);
        panel_lineSizer->AddStretchSpacer(1);
        panel_lineSizer->Add(calc_add_btn_sizer, 0, wxALIGN_CENTER | wxALL, 0);
        panel_lineSizer->AddSpacer(FromDIP(10));
        panel_lineSizer->Add(calc_approximate_match_btn_sizer, 0, wxALIGN_CENTER | wxALL, 0);
        panel_lineSizer->AddSpacer(FromDIP(10));
        panel_lineSizer->Add(calc_reset_btn_sizer, 0, wxALIGN_CENTER | wxALL, 0);
        panel_lineSizer->AddSpacer(FromDIP(16));

    }
    //draw ui
    //auto sizer_width = FromDIP(300);
    // Create two switched panels with their own sizers
    //m_sizer_simple          = new wxBoxSizer(wxVERTICAL);
    //m_page_simple			= new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    //m_page_simple->SetSizer(m_sizer_simple);
    //m_page_simple->SetBackgroundColour(*wxWHITE);

    //update_ui(m_page_simple);
    //// BBS
    //m_sizer_simple->AddSpacer(FromDIP(10));
    // BBS: for tunning flush volumes
    {

        //colors table
        m_scrolledWindow = new wxScrolledWindow(panel,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxSB_VERTICAL);
        panelSizer->Add(m_scrolledWindow, 0, wxEXPAND | wxALL, FromDIP(5));
        draw_table();
        //buttons
        //wxBoxSizer *quick_set_sizer = new wxBoxSizer(wxHORIZONTAL);
        //wxStaticText *quick_set_title = new wxStaticText(m_page_simple, wxID_ANY, _L("Quick set:"));
        //quick_set_title->SetFont(Label::Head_12);
        //quick_set_sizer->Add(quick_set_title, 0, wxALIGN_CENTER | wxALL, 0);
        //quick_set_sizer->AddSpacer(FromDIP(10));

        //m_sizer_simple->Add(quick_set_sizer, 0, wxEXPAND | wxLEFT, FromDIP(30));

        wxPanel* panel_line = new wxPanel(panel, wxID_ANY);
        panelSizer->AddStretchSpacer(1);
        panelSizer->Add(panel_line, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(16));
        panel_line->SetMinSize(wxSize(FromDIP(-1), FromDIP(30)));
        panel_line->SetMaxSize(wxSize(FromDIP(-1), FromDIP(30)));
        panel_line->SetFont(Label::Body_12);
        panel_line->SetBackgroundColour(bgColor);
        panel_line->SetForegroundColour(fgColor2);
        wxBoxSizer* panel_lineSizer = new wxBoxSizer(wxHORIZONTAL);
        panel_line->SetSizer(panel_lineSizer);
        //wxBoxSizer *warning_sizer = new wxBoxSizer(wxHORIZONTAL);
        m_warning_text = new wxStaticText(panel_line, wxID_ANY, "");
        panel_lineSizer->Add(m_warning_text, 0, wxALIGN_CENTER | wxALL, 0);
        //panelSizer->Add(warning_sizer, 0, wxEXPAND | wxLEFT, FromDIP(30));

        //m_sizer_simple->AddSpacer(10);
    }
    deal_default_strategy();
    //page_simple//page_advanced
    //m_sizer->Add(m_page_simple, 0, wxEXPAND, 0);
    m_sizer->SetSizeHints(this);
    SetSizer(m_sizer);
    this->Layout();
}

void ObjColorPanel::msw_rescale()
{
    for (unsigned int i = 0; i < m_extruder_icon_list.size(); ++i) {
        auto bitmap = *get_extruder_color_icon(m_colours[i].GetAsString(wxC2S_HTML_SYNTAX).ToStdString(), std::to_string(i + 1), FromDIP(16), FromDIP(16));
        m_extruder_icon_list[i]->SetBitmap(bitmap);
    }
   /* for (unsigned int i = 0; i < m_color_cluster_icon_list.size(); ++i) {
        auto bitmap = *get_extruder_color_icon(m_cluster_colours[i].GetAsString(wxC2S_HTML_SYNTAX).ToStdString(), std::to_string(i + 1), FromDIP(16), FromDIP(16));
        m_color_cluster_icon_list[i]->SetBitmap(bitmap);
    }*/
}

bool ObjColorPanel::is_ok() {
    for (auto item : m_result_icon_list) {
        if (item->bitmap_combox->IsShown()) {
            auto selection = item->bitmap_combox->GetSelection();
            if (selection < 1) {
                return false;
            }
        }
    }
    return true;
}

// ============================================================
// 辅助函数：稳定映射 <-> combo 位置 <-> paint_color
// ============================================================

// 在当前 enabled 混合耗材列表中查找 (comp_a, comp_b) 的索引（0-based）
// 未找到返回 -1。组件对比较忽略顺序。
int ObjColorPanel::find_mixed_combo_index(unsigned char comp_a, unsigned char comp_b)
{
    if (comp_a < 1 || comp_b < 1 || comp_a == comp_b) { return -1; }
    const auto &mixed_list = wxGetApp().preset_bundle->mixed_filaments.mixed_filaments();
    int enabled_idx = 0;
    for (const auto &mf : mixed_list) {
        if (!mf.enabled || mf.deleted) continue;
        bool match = (mf.component_a == comp_a && mf.component_b == comp_b) ||
                     (mf.component_a == comp_b && mf.component_b == comp_a);
        if (match) return enabled_idx;
        enabled_idx++;
    }
    return -1;
}

// OK 后基于 (comp_a, comp_b) 计算虚拟 ID（paint_color）。
// 虚拟 ID = num_physical + enabled_index + 1（1-based）。
// 若超出 g_max_color 或未找到返回 0。
unsigned int ObjColorPanel::find_virtual_id_for_mixed(unsigned char comp_a, unsigned char comp_b, size_t num_physical)
{
    if (comp_a < 1 || comp_b < 1 || comp_a == comp_b) { return 0; }
    if ((size_t)comp_a > num_physical || (size_t)comp_b > num_physical) { return 0; }
    int idx = find_mixed_combo_index(comp_a, comp_b);
    if (idx < 0) { return 0; }
    unsigned int virtual_id = (unsigned int)num_physical + (unsigned int)idx + 1;
    if (virtual_id < 1 || virtual_id > (unsigned int)g_max_color) {
        return 0;
    }
    return virtual_id;
}

// 在前 num_physical 个物理耗材（且不超过 m_colours 范围）中找与 cluster_color 最接近的物理 ID（1-based）。
// 内部对 m_colours 预计算 LAB（避免每 cluster 重算 N 次 RGB2Lab），不依赖外部缓存，
// 防止调用方传入与当前 m_colours 不一致的过期数据。
int ObjColorPanel::find_best_physical_match(const wxColour &cluster_color, int num_physical)
{
    if (num_physical <= 0 || m_colours.empty()) { return 1; }
    int limit = std::min({num_physical, (int)m_colours.size(), g_max_color});
    std::vector<std::array<float, 3>> existing_labs(limit);
    for (int j = 0; j < limit; j++) {
        RGB2Lab(m_colours[j].Red() / 255.f, m_colours[j].Green() / 255.f, m_colours[j].Blue() / 255.f,
                &existing_labs[j][0], &existing_labs[j][1], &existing_labs[j][2]);
    }
    float lab_c[3];
    RGB2Lab(cluster_color.Red() / 255.f, cluster_color.Green() / 255.f, cluster_color.Blue() / 255.f,
            &lab_c[0], &lab_c[1], &lab_c[2]);
    int   best = 1;
    float best_dist = std::numeric_limits<float>::max();
    for (int j = 0; j < limit; j++) {
        float d = DeltaE00(lab_c[0], lab_c[1], lab_c[2],
                           existing_labs[j][0], existing_labs[j][1], existing_labs[j][2]);
        if (d < best_dist) { best_dist = d; best = j + 1; }
    }
    return best;
}

// 将 combo 位置（0..combo.GetCount()-1）转换为稳定 ClusterMapping。
// 组合状态：[0=undefined, 1..N_phys_in_m_colours=物理, ..m_colours.size()=混合, ..=新物理]
ClusterMapping ObjColorPanel::combo_position_to_mapping(int combo_pos)
{
    ClusterMapping m;
    m.type = ClusterMappingType::PHYSICAL;
    m.id1 = 0;
    m.id2 = 0;
    if (combo_pos <= 0) { return m; }

    int num_physical_total = (int)wxGetApp().preset_bundle->filament_presets.size();
    int num_phys_in_m_colours = std::min(num_physical_total, (int)m_colours.size());

    if (combo_pos <= num_phys_in_m_colours) {
        m.type = ClusterMappingType::PHYSICAL;
        m.id1 = (unsigned char)combo_pos;
        return m;
    }
    if (combo_pos <= (int)m_colours.size()) {
        // 混合槽位（基于 OK 前 enabled 顺序）
        int mixed_idx = combo_pos - num_phys_in_m_colours - 1;
        const auto &mixed_list = wxGetApp().preset_bundle->mixed_filaments.mixed_filaments();
        int enabled_idx = 0;
        for (const auto &mf : mixed_list) {
            if (!mf.enabled || mf.deleted) continue;
            if (enabled_idx == mixed_idx) {
                m.type = ClusterMappingType::MIXED_PAIR;
                m.id1 = (unsigned char)mf.component_a;
                m.id2 = (unsigned char)mf.component_b;
                return m;
            }
            enabled_idx++;
        }
        // 异常 fallback
        m.type = ClusterMappingType::PHYSICAL;
        m.id1 = 1;
        return m;
    }
    // 新增物理
    int new_order = combo_pos - (int)m_colours.size();
    m.type = ClusterMappingType::NEW_PHYSICAL;
    m.id1 = (unsigned char)new_order;
    return m;
}

// 将 ClusterMapping 转换为当前 combo 位置（用于 SetSelection）。
// 找不到合法位置返回 0（undefined）。
int ObjColorPanel::mapping_to_combo_position(const ClusterMapping &m)
{
    int num_physical_total = (int)wxGetApp().preset_bundle->filament_presets.size();
    int num_phys_in_m_colours = std::min(num_physical_total, (int)m_colours.size());
    int num_mixed_in_m_colours = (int)m_colours.size() - num_phys_in_m_colours;

    switch (m.type) {
    case ClusterMappingType::PHYSICAL:
        if (m.id1 >= 1 && (int)m.id1 <= num_phys_in_m_colours) { return (int)m.id1; }
        return 0;
    case ClusterMappingType::MIXED_PAIR: {
        int idx = find_mixed_combo_index(m.id1, m.id2);
        if (idx >= 0 && idx < num_mixed_in_m_colours) {
            return num_phys_in_m_colours + idx + 1;
        }
        return 0;
    }
    case ClusterMappingType::NEW_PHYSICAL:
        if (m.id1 >= 1 && (int)m.id1 <= (int)m_new_add_colors.size()) {
            return (int)m_colours.size() + (int)m.id1;
        }
        return 0;
    }
    return 0;
}

// OK 时调用：先按需添加物理耗材（这会触发混合耗材重新生成），
// 然后基于稳定 ClusterMapping 重新计算每个面片的 paint_color。
// 关键：m_cluster_mappings 保存的是 (物理 ID / 组件对 / 新增顺序)，
// 这些在 OK 时 add_custom_filament 后仍然能映射到正确的 paint_color。
void ObjColorPanel::update_filament_ids()
{
    size_t num_physical_before = wxGetApp().preset_bundle->filament_presets.size();
    size_t num_new            = m_new_add_colors.size();

    if (m_is_add_filament) {
        int target_count = (int)num_physical_before + (int)num_new;
        wxGetApp().sidebar().add_filaments_batch(target_count, m_new_add_colors);
    }

    // OK 后物理数量 / 混合列表已经更新，使用最新数据计算 paint_color
    size_t num_physical_after = wxGetApp().preset_bundle->filament_presets.size();

    {
        for (size_t i = 0; i < m_cluster_mappings.size(); i++) {
            const char *tname = (m_cluster_mappings[i].type == ClusterMappingType::PHYSICAL) ? "P"
                : (m_cluster_mappings[i].type == ClusterMappingType::MIXED_PAIR) ? "M"
                : "N";
        }
    }

    m_filament_ids.clear();
    m_filament_ids.reserve(m_input_colors_size);

    for (size_t i = 0; i < m_input_colors_size; i++) {
        auto label = m_cluster_labels_from_algo[i];
        unsigned char paint_color = 0;  // 0 = undefined
        if (label >= 0 && (size_t)label < m_cluster_mappings.size()) {
            const ClusterMapping &m = m_cluster_mappings[label];
            switch (m.type) {
            case ClusterMappingType::PHYSICAL: {
                if (m.id1 >= 1 && m.id1 <= (unsigned char)g_max_color) {
                    paint_color = m.id1;
                }
                break;
            }
            case ClusterMappingType::MIXED_PAIR: {
                unsigned int v = find_virtual_id_for_mixed(m.id1, m.id2, num_physical_after);
                if (v >= 1 && v <= (unsigned int)g_max_color) {
                    paint_color = (unsigned char)v;
                }
                break;
            }
            case ClusterMappingType::NEW_PHYSICAL: {
                // 新增物理按顺序追加在物理列表末尾：order=1 对应 num_physical_after-num_new+1
                if (m.id1 >= 1 && (size_t)m.id1 <= num_new && num_physical_after >= num_new) {
                    size_t phys_id = num_physical_after - num_new + (size_t)m.id1;
                    if (phys_id >= 1 && phys_id <= (size_t)g_max_color) {
                        paint_color = (unsigned char)phys_id;
                    }
                }
                break;
            }
            }
        }
        m_filament_ids.emplace_back(paint_color);
        if (label >= 0 && (size_t)label < m_cluster_mappings.size()) {
            const ClusterMapping &mm = m_cluster_mappings[label];
            const char *tname = (mm.type == ClusterMappingType::PHYSICAL) ? "PHYSICAL"
                : (mm.type == ClusterMappingType::MIXED_PAIR) ? "MIXED_PAIR"
                : "NEW_PHYSICAL";
        } else {
        }
    }

    {
    }

    if (!m_filament_ids.empty()) {
        m_first_extruder_id = m_filament_ids[0];
    } else {
        m_first_extruder_id = 0;
    }
}

void ObjColorPanel::update_warning_text(const wxString& strTxt)
{
    if (m_warning_text) {
        if (strTxt == m_warning_text->GetLabelText())
            return;
        if (!strTxt.empty() && m_bClickedAddBtn)
            return;
        m_warning_text->SetLabelText(strTxt);
    }
}

wxBoxSizer *ObjColorPanel::create_approximate_match_btn_sizer(wxWindow *parent)
{
    auto       btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxString   bgColor   = wxGetApp().dark_mode() ? "#313131" : "#FFFFFF";
    wxString   bgColor2  = wxGetApp().dark_mode() ? "#2B2B2B" : "#EFF0F6";
    wxString   bgColor3  = wxGetApp().dark_mode() ? "#6C6E71" : "#D0D4DE";
    wxString   fgColor   = wxGetApp().dark_mode() ? "#1FCA63" : "#1FCA63";
    wxString   fgColor2  = wxGetApp().dark_mode() ? "#F0F0FF" : "#30373D";
    StateColor calc_btn_bg(std::pair<wxColour, int>(wxColour(bgColor /*0, 137, 123*/), StateColor::Pressed), 
                           std::pair<wxColour, int>(wxColour(bgColor /*38, 166, 154*/), StateColor::Hovered),
                           std::pair<wxColour, int>(wxColour(bgColor /*0, 150, 136*/), StateColor::Normal));
    StateColor calc_btn_bd(std::pair<wxColour, int>(wxColour(bgColor3 /*0, 150, 136*/), StateColor::Normal));
    StateColor calc_btn_text(std::pair<wxColour, int>(wxColour(fgColor2 /*255, 255, 254*/), StateColor::Normal));
    //create btn
    m_quick_approximate_match_btn = new Button(parent, _L("Color match"));
    m_quick_approximate_match_btn->SetToolTip(_L("Approximate color matching."));
    auto cur_btn         = m_quick_approximate_match_btn;
    cur_btn->SetFont(Label::Body_13);
    cur_btn->SetMinSize(wxSize(FromDIP(76), FromDIP(24)));
    cur_btn->SetCornerRadius(FromDIP(4));
    cur_btn->SetBackgroundColor(calc_btn_bg);
    cur_btn->SetBorderColor(calc_btn_bd);
    cur_btn->SetTextColor(calc_btn_text);
    cur_btn->SetFocus();
    btn_sizer->Add(cur_btn, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 0);
    cur_btn->Bind(wxEVT_BUTTON, [this, fgColor, calc_btn_bd, calc_btn_text](wxCommandEvent&) {
        m_quick_approximate_match_btn->SetBorderColor(wxColour(fgColor));
        m_quick_approximate_match_btn->SetTextColor(wxColour(fgColor));
        m_quick_add_btn->SetBorderColor(calc_btn_bd);
        m_quick_add_btn->SetTextColor(calc_btn_text);
        m_quick_reset_btn->SetBorderColor(calc_btn_bd);
        m_quick_reset_btn->SetTextColor(calc_btn_text);
        deal_default_strategy();
    });
    return btn_sizer;
}

wxBoxSizer *ObjColorPanel::create_add_btn_sizer(wxWindow *parent)
{
    auto       btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxString   bgColor   = wxGetApp().dark_mode() ? "#313131" : "#FFFFFF";
    wxString   bgColor2  = wxGetApp().dark_mode() ? "#2B2B2B" : "#EFF0F6";
    wxString   bgColor3  = wxGetApp().dark_mode() ? "#6C6E71" : "#D0D4DE";
    wxString   fgColor   = wxGetApp().dark_mode() ? "#1FCA63" : "#1FCA63";
    wxString   fgColor2  = wxGetApp().dark_mode() ? "#F0F0FF" : "#30373D";
    StateColor calc_btn_bg(std::pair<wxColour, int>(wxColour(bgColor /*0, 137, 123*/), StateColor::Pressed), 
                           std::pair<wxColour, int>(wxColour(bgColor /*38, 166, 154*/), StateColor::Hovered),
                           std::pair<wxColour, int>(wxColour(bgColor /*0, 150, 136*/), StateColor::Normal));
    StateColor calc_btn_bd(std::pair<wxColour, int>(wxColour(bgColor3 /*0, 150, 136*/), StateColor::Normal));
    StateColor calc_btn_text(std::pair<wxColour, int>(wxColour(fgColor2 /*255, 255, 254*/), StateColor::Normal));
    // create btn
    m_quick_add_btn = new Button(parent, _L("Append"));
    m_quick_add_btn->SetToolTip(_L("Add consumable extruder after existing extruders."));
    auto cur_btn    = m_quick_add_btn;
    cur_btn->SetFont(Label::Body_13);
    cur_btn->SetMinSize(wxSize(FromDIP(48), FromDIP(24)));
    cur_btn->SetCornerRadius(FromDIP(4));
    cur_btn->SetBackgroundColor(calc_btn_bg);
    cur_btn->SetBorderColor(calc_btn_bd);
    cur_btn->SetTextColor(calc_btn_text);
    cur_btn->SetFocus();
    btn_sizer->Add(cur_btn, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 0);
    cur_btn->Bind(wxEVT_BUTTON, [this, fgColor, calc_btn_bd, calc_btn_text](wxCommandEvent&) {
        m_quick_add_btn->SetBorderColor(wxColour(fgColor));
        m_quick_add_btn->SetTextColor(wxColour(fgColor));
        m_quick_approximate_match_btn->SetBorderColor(calc_btn_bd);
        m_quick_approximate_match_btn->SetTextColor(calc_btn_text);
        m_quick_reset_btn->SetBorderColor(calc_btn_bd);
        m_quick_reset_btn->SetTextColor(calc_btn_text);
        deal_add_btn();
    });
    return btn_sizer;
}

wxBoxSizer *ObjColorPanel::create_reset_btn_sizer(wxWindow *parent)
{
    auto       btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxString   bgColor   = wxGetApp().dark_mode() ? "#313131" : "#FFFFFF";
    wxString   bgColor2  = wxGetApp().dark_mode() ? "#2B2B2B" : "#EFF0F6";
    wxString   bgColor3  = wxGetApp().dark_mode() ? "#6C6E71" : "#D0D4DE";
    wxString   fgColor   = wxGetApp().dark_mode() ? "#1FCA63" : "#1FCA63";
    wxString   fgColor2  = wxGetApp().dark_mode() ? "#F0F0FF" : "#30373D";
    StateColor calc_btn_bg(std::pair<wxColour, int>(wxColour(bgColor /*0, 137, 123*/), StateColor::Pressed), 
                           std::pair<wxColour, int>(wxColour(bgColor /*38, 166, 154*/), StateColor::Hovered),
                           std::pair<wxColour, int>(wxColour(bgColor /*0, 150, 136*/), StateColor::Normal));
    StateColor calc_btn_bd(std::pair<wxColour, int>(wxColour(bgColor3 /*0, 150, 136*/), StateColor::Normal));
    StateColor calc_btn_text(std::pair<wxColour, int>(wxColour(fgColor2 /*255, 255, 254*/), StateColor::Normal));
    // create btn
    m_quick_reset_btn = new Button(parent, _L("Reset"));
    m_quick_reset_btn->SetToolTip(_L("Reset mapped extruders."));
    auto cur_btn      = m_quick_reset_btn;
    cur_btn->SetFont(Label::Body_13);
    cur_btn->SetMinSize(wxSize(FromDIP(48), FromDIP(24)));
    cur_btn->SetCornerRadius(FromDIP(4));
    cur_btn->SetBackgroundColor(calc_btn_bg);
    cur_btn->SetBorderColor(calc_btn_bd);
    cur_btn->SetTextColor(calc_btn_text);
    cur_btn->SetFocus();
    btn_sizer->Add(cur_btn, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 0);
    cur_btn->Bind(wxEVT_BUTTON, [this, fgColor, calc_btn_bd, calc_btn_text](wxCommandEvent&) {
        m_quick_reset_btn->SetTextColor(wxColour(fgColor));
        m_quick_reset_btn->SetBorderColor(wxColour(fgColor));
        m_quick_approximate_match_btn->SetBorderColor(calc_btn_bd);
        m_quick_approximate_match_btn->SetTextColor(calc_btn_text);
        m_quick_add_btn->SetBorderColor(calc_btn_bd);
        m_quick_add_btn->SetTextColor(calc_btn_text);
        deal_reset_btn();
    });
    return btn_sizer;
}

wxBoxSizer *ObjColorPanel::create_extruder_icon_and_rgba_sizer(wxWindow *parent, int id, const wxColour &color)
{
    auto icon_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton *icon       = new wxButton(parent, wxID_ANY, {}, wxDefaultPosition, ICON_SIZE, wxBORDER_NONE | wxBU_AUTODRAW);
    icon->SetBitmap(*get_extruder_color_icon(color.GetAsString(wxC2S_HTML_SYNTAX).ToStdString(), std::to_string(id + 1), FromDIP(20), FromDIP(20)));
    icon->SetCanFocus(false);
    m_extruder_icon_list.emplace_back(icon);
    icon_sizer->Add(icon, 0, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL, FromDIP(0)); // wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM

    //icon_sizer->AddSpacer(FromDIP(5));
    return icon_sizer;
}

std::string ObjColorPanel::get_color_str(const wxColour &color) {
    std::string str = ("R:" + std::to_string(color.Red()) +
                          std::string(" G:") + std::to_string(color.Green()) +
                          std::string(" B:") + std::to_string(color.Blue()) +
                          std::string(" A:") + std::to_string(color.Alpha()));
    return str;
}

ComboBox *ObjColorPanel::CreateEditorCtrl(wxWindow *parent, int id) // wxRect labelRect,, const wxVariant &value
{
    std::vector<wxBitmap *> icons = get_extruder_color_icons();
    const double            em          = Slic3r::GUI::wxGetApp().em_unit();
    bool                    thin_icon   = false;
    const int               icon_width  = lround((thin_icon ? 2 : 4.4) * em);
    const int               icon_height = lround(2 * em);
    m_combox_icon_width                 = icon_width;
    m_combox_icon_height                = icon_height;
    wxColour undefined_color(0,255,0,255);
    icons.insert(icons.begin(), get_extruder_color_icon(undefined_color.GetAsString(wxC2S_HTML_SYNTAX).ToStdString(), std::to_string(-1), icon_width, icon_height));
    if (icons.empty())
        return nullptr;

    // 与 m_colours 一致：combo 最多显示 g_max_color 个槽位 + 1 个 undefined。
    // get_extruder_color_icons() 返回 plater 全部 extruder 颜色（可能 > g_max_color），
    // 多余部分截断掉，避免下面循环越界访问 m_colours[i-1]。
    if (icons.size() > m_colours.size() + 1) {
        icons.resize(m_colours.size() + 1);
    }

    ::ComboBox *c_editor = new ::ComboBox(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(FromDIP(m_combox_width), -1), 0, nullptr,
                                          wxCB_READONLY | CB_NO_DROP_ICON | CB_NO_TEXT);
    c_editor->SetMinSize(wxSize(FromDIP(m_combox_width), -1));
    c_editor->SetMaxSize(wxSize(FromDIP(m_combox_width), -1));
    c_editor->EnableAutoPopupDirection();
    c_editor->GetDropDown().SetUseContentWidth(true);
    c_editor->GetDropDown().SetMaxVisibleItems(9);
    for (size_t i = 0; i < icons.size(); i++) {
        c_editor->Append(wxString::Format("%d", i), *icons[i]);
        if (i == 0) {
            c_editor->SetItemTooltip(i,undefined_color.GetAsString(wxC2S_HTML_SYNTAX));
        } else {
            c_editor->SetItemTooltip(i, m_colours[i-1].GetAsString(wxC2S_HTML_SYNTAX));
        }
    }
    c_editor->SetSelection(0);
    c_editor->SetName(wxString::Format("%d", id));
    c_editor->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent &evt) {
        auto *com_box = static_cast<ComboBox *>(evt.GetEventObject());
        int   i       = atoi(com_box->GetName().c_str());
        if (i < (int)m_cluster_mappings.size()) {
            int sel = com_box->GetSelection();
            m_cluster_mappings[i] = combo_position_to_mapping(sel);
            const ClusterMapping &mm = m_cluster_mappings[i];
            const char *tname = (mm.type == ClusterMappingType::PHYSICAL) ? "PHYSICAL"
                : (mm.type == ClusterMappingType::MIXED_PAIR) ? "MIXED_PAIR"
                : "NEW_PHYSICAL";
        }
        evt.StopPropagation();
    });
    return c_editor;
}

// Match each cluster color to the closest existing filament
// Updates m_cluster_mappings and m_color_needs_new_filament
void ObjColorPanel::deal_approximate_match_btn()
{
    m_warning_text->SetLabelText("");
    m_bClickedAddBtn = false;

    // Guard: need valid combos to work with
    if (m_result_icon_list.size() == 0) { return; }
    if (!m_result_icon_list[0]->bitmap_combox) { return; }

    // Get count of existing filaments in combo (after Reset: index 0=undefined, 1..m_colours.size()=existing)
    auto map_count = m_result_icon_list[0]->bitmap_combox->GetCount() - 1;
    if (map_count < 1) { return; }

    // Limit to g_max_color to ensure matching only within valid filament range (1-g_max_color)
    if (map_count > (size_t)g_max_color) {
        map_count = g_max_color;
    }

    // Cache existing filament colors from tooltips (avoid repeated GetItemTooltip calls)
    std::vector<wxColour> existing_filament_colors;
    existing_filament_colors.reserve(map_count);
    for (size_t j = 0; j < map_count; j++) {
        auto tip_color = m_result_icon_list[0]->bitmap_combox->GetItemTooltip(j + 1);
        existing_filament_colors.emplace_back(wxColour(tip_color));
    }

    // Pre-calculate LAB values for existing filaments (optimization: compute once, not M×N times)
    std::vector<std::array<float, 3>> existing_filament_labs;
    existing_filament_labs.resize(map_count);
    for (size_t j = 0; j < map_count; j++) {
        RGB2Lab(existing_filament_colors[j].Red() / 255.f, existing_filament_colors[j].Green() / 255.f,
                existing_filament_colors[j].Blue() / 255.f, &existing_filament_labs[j][0], &existing_filament_labs[j][1], &existing_filament_labs[j][2]);
    }

    // For each cluster color, find the closest existing filament
    for (size_t i = 0; i < m_cluster_colours.size(); i++) {
        auto c = m_cluster_colours[i];
        std::vector<ColorDistValue> color_dists;
        color_dists.resize(map_count);

        // Calculate distance to each existing filament using cached LAB values
        float lab_c[3];
        RGB2Lab(c.Red() / 255.f, c.Green() / 255.f, c.Blue() / 255.f, &lab_c[0], &lab_c[1], &lab_c[2]);

        for (size_t j = 0; j < map_count; j++) {
            color_dists[j].distance = DeltaE76(lab_c[0], lab_c[1], lab_c[2], existing_filament_labs[j][0], existing_filament_labs[j][1], existing_filament_labs[j][2]);
            color_dists[j].id = j + 1;  // tooltip index
        }

    // Sort by distance, closest first
    std::sort(color_dists.begin(), color_dists.end(), [](ColorDistValue &a, ColorDistValue& b) {
        return a.distance < b.distance;
    });

    auto new_index = color_dists[0].id;
    bool close_match = (color_dists[0].distance < COLOR_CLOSE_MATCH_THRESHOLD);

    // Update combo selection and stable mapping
    m_result_icon_list[i]->bitmap_combox->SetSelection(new_index);
    // 此时 combo 中只有 [0=undefined, 1..m_colours.size()=existing]，无新增项；
    // combo_position_to_mapping 会把它解析成 PHYSICAL 或 MIXED_PAIR。
    m_cluster_mappings[i] = combo_position_to_mapping(new_index);
    m_color_needs_new_filament[i] = !close_match;
    }
}

void ObjColorPanel::show_sizer(wxSizer *sizer, bool show)
{
    wxSizerItemList items = sizer->GetChildren();
    for (wxSizerItemList::iterator it = items.begin(); it != items.end(); ++it) {
        wxSizerItem *item   = *it;
        if (wxWindow *window = item->GetWindow()) {
            window->Show(show);
        }
        if (wxSizer *son_sizer = item->GetSizer()) {
            show_sizer(son_sizer, show);
        }
    }
}

void ObjColorPanel::redraw_part_table() {
    //show all and set -1
    deal_reset_btn();
    for (size_t i = 0; i < m_row_sizer_list.size(); i++) {
        show_sizer(m_row_sizer_list[i], true);
    }
    if (m_cluster_colours.size() < m_row_sizer_list.size()) { // show part
        for (size_t i = m_cluster_colours.size(); i < m_row_sizer_list.size(); i++) {
            show_sizer(m_row_sizer_list[i], false);
            //m_row_panel_list[i]->Show(false); // show_sizer(m_left_color_cluster_boxsizer_list[i],false);
           // m_result_icon_list[i]->bitmap_combox->Show(false);
        }
    } else if (m_cluster_colours.size() > m_row_sizer_list.size()) {
        for (size_t i = m_row_sizer_list.size(); i < m_cluster_colours.size(); i++) {
            int      id                       = i;
            wxPanel *row_panel = new wxPanel(m_scrolledWindow);
            row_panel->SetBackgroundColour((i+1) % 2 == 0 ? *wxWHITE : wxColour(238, 238, 238));
            auto row_sizer = new wxGridSizer(1, 2, 1, 3);
            row_panel->SetSizer(row_sizer);

            row_panel->SetMinSize(wxSize(FromDIP(PANEL_WIDTH), -1));
            row_panel->SetMaxSize(wxSize(FromDIP(PANEL_WIDTH), -1));

            auto cluster_color_icon_sizer = create_color_icon_and_rgba_sizer(row_panel, id, m_cluster_colours[id]);
            row_sizer->Add(cluster_color_icon_sizer, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, FromDIP(CONTENT_BORDER));
            // result_combox
            create_result_button_sizer(row_panel, id);
            row_sizer->Add(m_result_icon_list[id]->bitmap_combox, 0, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL, 0);

            m_row_sizer_list.emplace_back(row_sizer);
            m_gridsizer->Add(row_panel, 0, wxALIGN_LEFT | wxALL, FromDIP(HEADER_BORDER));
        }
        m_gridsizer->Layout();
    }
    for (size_t i = 0; i < m_cluster_colours.size(); i++) { // update data
        // m_color_cluster_icon_list//m_color_cluster_text_list
        update_color_icon_and_rgba_sizer(i, m_cluster_colours[i]);
    }
    m_scrolledWindow->Refresh();
}

void ObjColorPanel::draw_table()
{
    auto row                = std::max(m_cluster_colours.size(), m_colours.size()) + 1;
    m_gridsizer             = new wxGridSizer(row, 1, 1, 3); //(int rows, int cols, int vgap, int hgap );

    wxString evenbgColor = wxGetApp().dark_mode() ? "#222222" : "#EFF0F6";
    wxString oddbgColor = wxGetApp().dark_mode() ? "#2B2B2B" : "#F7F8FA";
    m_color_cluster_icon_list.clear();
    m_extruder_icon_list.clear();
    float row_height ;
    for (size_t ii = 0; ii < row; ii++) {
        wxPanel *row_panel = new wxPanel(m_scrolledWindow);
        row_panel->SetBackgroundColour(ii % 2 == 0 ? evenbgColor:oddbgColor);
        auto row_sizer = new wxGridSizer(1, 2, 1, 5);
        row_panel->SetSizer(row_sizer);

        row_panel->SetMinSize(wxSize(FromDIP(PANEL_WIDTH), FromDIP(36)));
        row_panel->SetMaxSize(wxSize(FromDIP(PANEL_WIDTH), FromDIP(36)));
        if (ii == 0) {
            wxStaticText* colors_left_title = new wxStaticText(row_panel, wxID_ANY, "" /*_L("Cluster colors")*/);
            colors_left_title->SetFont(Label::Head_14);
            row_sizer->Add(colors_left_title, 0, wxALIGN_CENTER | wxALL, FromDIP(HEADER_BORDER));

            wxStaticText *colors_middle_title = new wxStaticText(row_panel, wxID_ANY, _L("Map Filament"));
            colors_middle_title->SetFont(Label::Head_14);
            row_sizer->Add(colors_middle_title, 0, wxALIGN_CENTER | wxALL, FromDIP(HEADER_BORDER));
        } else {
            int id = ii - 1;
            if (id < m_cluster_colours.size()) {
                auto cluster_color_icon_sizer = create_color_icon_and_rgba_sizer(row_panel, id, m_cluster_colours[id]);
                row_sizer->Add(cluster_color_icon_sizer, 0, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL, FromDIP(CONTENT_BORDER));
                // result_combox
                create_result_button_sizer(row_panel, id);
                row_sizer->Add(m_result_icon_list[id]->bitmap_combox, 0, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL, FromDIP(CONTENT_BORDER));
            }
        }
        row_height = row_panel->GetSize().GetHeight();
        if (ii>=1) {
            m_row_sizer_list.emplace_back(row_sizer);
        }
        m_gridsizer->Add(row_panel, 0, wxALIGN_LEFT | wxLEFT, FromDIP(16));
    }
    m_scrolledWindow->SetSizer(m_gridsizer);
    int totalHeight = row_height *(row+1) * 2;
    m_scrolledWindow->SetVirtualSize(MIN_OBJCOLOR_DIALOG_WIDTH, totalHeight);
    auto look = FIX_SCROLL_HEIGTH;
    if (totalHeight > FIX_SCROLL_HEIGTH) {
        m_scrolledWindow->SetMinSize(wxSize(MIN_OBJCOLOR_DIALOG_WIDTH, FIX_SCROLL_HEIGTH));
        m_scrolledWindow->SetMaxSize(wxSize(MIN_OBJCOLOR_DIALOG_WIDTH, FIX_SCROLL_HEIGTH));
    }
    else {
        m_scrolledWindow->SetMinSize(wxSize(MIN_OBJCOLOR_DIALOG_WIDTH, totalHeight));
    }
    m_scrolledWindow->EnableScrolling(false, true);
    m_scrolledWindow->ShowScrollbars(wxSHOW_SB_NEVER, wxSHOW_SB_DEFAULT);//wxSHOW_SB_ALWAYS
    m_scrolledWindow->SetScrollRate(20, 20);
}

void ObjColorPanel::deal_algo(char cluster_number, bool redraw_ui)
{
    if (m_last_cluster_number == cluster_number) {
        return;
    }
    m_last_cluster_number = cluster_number;
    QuantKMeans quant(10);
    // max_cluster = g_max_color - 1: the software always has at least one existing filament,
    // so the maximum new filaments that can be added is g_max_color - 1
    quant.apply(m_input_colors, m_cluster_colors_from_algo, m_cluster_labels_from_algo, (int)cluster_number, g_max_color - 1);
    m_cluster_colours.clear();
    m_cluster_colours.reserve(m_cluster_colors_from_algo.size());
    for (size_t i = 0; i < m_cluster_colors_from_algo.size(); i++) {
        m_cluster_colours.emplace_back(convert_to_wxColour(m_cluster_colors_from_algo[i]));
    }
    if (m_cluster_colours.size() == 0) {
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ",m_cluster_colours.size() = 0\n";
        return;
    }
    m_cluster_mappings.resize(m_cluster_colors_from_algo.size());
    m_color_cluster_num_by_algo = m_cluster_colors_from_algo.size();
    if (cluster_number == -1) {
        m_color_num_recommend = m_color_cluster_num_by_algo;
    }
    //redraw ui
    if (redraw_ui) {
        redraw_part_table();
        // NOTE: deal_default_strategy() should NOT be called here
        // because the dialog constructor will call draw_table() first, then deal_default_strategy()
        // Calling deal_default_strategy() here before draw_table() will cause crash
    }
}

// Helper: Append new filament items to all combos
// Returns the number of items actually added (may be less than requested due to g_max_color limit)
static int append_new_filaments_to_combos(
    const std::vector<wxBitmap *> &new_icons,
    const std::vector<ObjColorPanel::ButtonState *> &result_icon_list,
    size_t existing_count)
{
    int added = 0;
    for (size_t i = 0; i < result_icon_list.size(); i++) {
        auto item = result_icon_list[i];
        if (!item->bitmap_combox) continue;
        for (size_t k = 0; k < new_icons.size(); k++) {
            item->bitmap_combox->Append(wxString::Format("%d", item->bitmap_combox->GetCount()), *new_icons[k]);
            added++;
        }
    }
    return added;
}

// Helper: Set tooltips for new filament items added to combos
static void set_new_filament_tooltips(
    const std::vector<wxColour> &new_colors,
    ObjColorPanel::ButtonState *first_result_item,
    size_t existing_count)
{
    for (size_t k = 0; k < new_colors.size(); k++) {
        int tooltip_index = existing_count + 1 + k;
        first_result_item->bitmap_combox->SetItemTooltip(tooltip_index,
            new_colors[k].GetAsString(wxC2S_HTML_SYNTAX));
    }
}

// Color match strategy (Initialization and Color match button entry point)
// Flow: Reset -> Match to existing -> Add unmatched as new filaments
//
// Called by:
//   - Initialization (ObjColorPanel constructor)
//   - Color match button (via button binding)
//   - Cluster number change (via text input binding)
//
// Key behavior:
//   - Match colors to existing filaments
//   - For unmatched colors, create NEW virtual filament options (preview only)
//   - Does NOT actually add filaments - just sets up UI for preview
//   - Actual addition happens at OK button (via update_filament_ids)
//
// Key variables:
//   - m_cluster_mappings: stable mapping (PHYSICAL/MIXED_PAIR/NEW_PHYSICAL) for each cluster
//   - m_color_needs_new_filament: true if cluster has no close match (needs new filament)
//   - m_new_add_colors: colors to add as new filaments (for preview, used at OK)
//   - m_is_add_filament: flag set true if there are new colors to add (used at OK)
void ObjColorPanel::deal_default_strategy()
{
    // Step 1: Reset combo to only existing filaments
    deal_reset_btn();

    // Step 2: Match colors to existing filaments
    // Updates: m_cluster_mappings, m_color_needs_new_filament
    deal_approximate_match_btn();

    // Step 3: 给未匹配的颜色分配新物理槽位（不真正添加，仅为预览）。
    // 关键修复（BUG #1）：可用槽位 = g_max_color - m_colours.size()（基于 combo 槽位），
    // 不是 num_physical 也不是任何含混合项的总数。
    m_new_add_colors.clear();
    int num_physical_total   = (int)wxGetApp().preset_bundle->filament_presets.size();
    int current_combo_total  = (int)m_colours.size();   // 已截断到 <= g_max_color
    int available_new_slots  = std::max(0, g_max_color - current_combo_total);
    int needs_new_count      = (int)std::count(m_color_needs_new_filament.begin(), m_color_needs_new_filament.end(), true);

    int new_order = 0;  // 1-based 顺序，对应 ClusterMappingType::NEW_PHYSICAL.id1

    for (size_t i = 0; i < m_cluster_mappings.size(); i++) {
        if (!m_color_needs_new_filament[i]) { continue; }
        if ((int)m_new_add_colors.size() < available_new_slots) {
            // 有新槽位：分配 NEW_PHYSICAL
            m_new_add_colors.emplace_back(m_cluster_colours[i]);
            new_order++;
            ClusterMapping m;
            m.type = ClusterMappingType::NEW_PHYSICAL;
            m.id1  = (unsigned char)new_order;
            m.id2  = 0;
            m_cluster_mappings[i] = m;
        } else {
            // 槽位已满：用 best-match 落到现有物理
            int best_phys = find_best_physical_match(m_cluster_colours[i], num_physical_total);
            ClusterMapping m;
            m.type = ClusterMappingType::PHYSICAL;
            m.id1  = (unsigned char)best_phys;
            m.id2  = 0;
            m_cluster_mappings[i] = m;
        }
    }

    // Step 4: 在 combo 中追加新增物理项（标签："new N"），并刷新选中。
    if (m_result_icon_list.empty() || !m_result_icon_list[0]->bitmap_combox) { return; }

    if (!m_new_add_colors.empty()) {
        std::vector<wxBitmap *> new_icons;
        new_icons.reserve(m_new_add_colors.size());
        for (size_t k = 0; k < m_new_add_colors.size(); k++) {
            if ((int)(m_colours.size() + new_icons.size()) >= g_max_color) { break; }
            std::string label = std::string("new ") + std::to_string(k + 1);
            new_icons.emplace_back(get_extruder_color_icon(
                m_new_add_colors[k].GetAsString(wxC2S_HTML_SYNTAX).ToStdString(),
                label, m_combox_icon_width, m_combox_icon_height));
        }
        append_new_filaments_to_combos(new_icons, m_result_icon_list, m_colours.size());
        set_new_filament_tooltips(m_new_add_colors, m_result_icon_list[0], m_colours.size());
    }

    // Step 5: 基于稳定 ClusterMapping 设置 combo 选中。
    for (size_t i = 0; i < m_cluster_mappings.size(); i++) {
        int sel = mapping_to_combo_position(m_cluster_mappings[i]);
        m_result_icon_list[i]->bitmap_combox->SetSelection(sel);
        const ClusterMapping &mm = m_cluster_mappings[i];
        const char *tname = (mm.type == ClusterMappingType::PHYSICAL) ? "PHYSICAL"
            : (mm.type == ClusterMappingType::MIXED_PAIR) ? "MIXED_PAIR"
            : "NEW_PHYSICAL";
    }

    // Mark that we have new filaments to add (for OK button)
    m_is_add_filament = !m_new_add_colors.empty();
}

// Append button handler: force add ALL cluster colors as new filaments
// Key behavior:
//   - Ignores existing filament matches - force creates new filaments for ALL clusters
//   - Called directly by Append button
//   - Calls deal_reset_btn() first to clear previous selections
// Key variables:
//   - m_cluster_colours: all cluster colors to add (not just unmatched ones)
//   - m_new_add_colors: stores the colors to be added (used by update_filament_ids() at OK)
//   - m_is_add_filament: flag indicating new filaments will be added (used at OK)
void ObjColorPanel::deal_add_btn()
{
    // Check max color limit
    if ((int)m_colours.size() > g_max_color) { return; }

    // Calculate available slots for new filaments
    int max_new_slots = g_max_color - (int)m_colours.size();

    // If cluster count exceeds available slots, fall back to Color match result
    if ((int)m_cluster_colours.size() > max_new_slots) {
        deal_default_strategy();
        m_bClickedAddBtn = true;
        m_warning_text->SetLabelText(
            GUI::format_wxstr(_L("Warning: Color count exceeds %1% limit, excess colors will reuse the last available color."),
            g_max_color));
        return;
    }

    // Reset first to clear all previous selections (Append is independent action)
    deal_reset_btn();

    if (m_result_icon_list.empty() || !m_result_icon_list[0]->bitmap_combox) { return; }

    // Add ALL cluster colors as new filaments (not just unmatched ones)，标签使用 "new N"
    std::vector<wxBitmap *> new_icons;
    new_icons.reserve(m_cluster_colours.size());
    for (size_t k = 0; k < m_cluster_colours.size(); k++) {
        std::string label = std::string("new ") + std::to_string(k + 1);
        new_icons.emplace_back(get_extruder_color_icon(
            m_cluster_colours[k].GetAsString(wxC2S_HTML_SYNTAX).ToStdString(),
            label, m_combox_icon_width, m_combox_icon_height));
    }

    // Append new filament items to all combos using helper
    append_new_filaments_to_combos(new_icons, m_result_icon_list, m_colours.size());

    // Update m_new_add_colors so update_filament_ids() can add them
    m_new_add_colors.clear();
    for (size_t k = 0; k < m_cluster_colours.size(); k++) {
        m_new_add_colors.emplace_back(m_cluster_colours[k]);
    }

    // 用稳定 mapping 记录 NEW_PHYSICAL（顺序 1..N），并设置 combo 选中
    if (m_cluster_mappings.size() < m_cluster_colours.size()) {
        m_cluster_mappings.resize(m_cluster_colours.size());
    }
    for (size_t i = 0; i < m_cluster_colours.size(); i++) {
        ClusterMapping m;
        m.type = ClusterMappingType::NEW_PHYSICAL;
        m.id1  = (unsigned char)(i + 1);  // 1-based 顺序
        m.id2  = 0;
        m_cluster_mappings[i] = m;
        int sel = mapping_to_combo_position(m);
        m_result_icon_list[i]->bitmap_combox->SetSelection(sel);
    }

    // Set tooltips for new filaments using helper
    set_new_filament_tooltips(m_cluster_colours, m_result_icon_list[0], m_colours.size());

    m_is_add_filament = !new_icons.empty();
}

// Reset button handler: clear all selections and new filaments
// Key behavior:
//   - Removes all new filament items from combos (keeps existing filaments only)
//   - Resets all selections to undefined (index 0)
//   - Clears m_new_add_colors and m_is_add_filament flag
// Key variables:
//   - m_colours: existing filament colors (these are kept)
//   - combo index 0 = undefined, 1..m_colours.size() = existing, m_colours.size()+1.. = new
//   - m_color_needs_new_filament: resized to cluster count, default true
void ObjColorPanel::deal_reset_btn()
{
    // Guard: if bitmap_combox not created yet (before draw_table()), return early
    bool has_valid_combo = false;
    m_bClickedAddBtn     = false;
    for (auto item : m_result_icon_list) {
        if (item->bitmap_combox) {
            has_valid_combo = true;
            break;
        }
    }
    if (!has_valid_combo) {
        m_is_add_filament = false;
        m_new_add_colors.clear();
        m_warning_text->SetLabelText("");
        return;
    }

    // Delete all new filament items from combos (keep existing filaments only)
    // Combo structure: index 0=undefined, 1..m_colours.size()=existing, rest=new
    for (auto item : m_result_icon_list) {
        if (!item->bitmap_combox) continue;
        while (item->bitmap_combox->GetCount() > m_colours.size() + 1) {
            item->bitmap_combox->DeleteOneItem(item->bitmap_combox->GetCount() - 1);
        }
        // Reset selection to undefined (index 0)
        item->bitmap_combox->SetSelection(0);
    }

    m_is_add_filament = false;
    m_new_add_colors.clear();
    // Default: assume all clusters need new filament (will be updated by deal_approximate_match_btn)
    m_color_needs_new_filament.resize(m_cluster_colours.size(), true);
    m_warning_text->SetLabelText("");
}

void ObjColorPanel::create_result_button_sizer(wxWindow *parent, int id)
{
    for (size_t i = m_result_icon_list.size(); i < id + 1; i++) {
        m_result_icon_list.emplace_back(new ButtonState());
    }
    m_result_icon_list[id]->bitmap_combox = CreateEditorCtrl(parent,id);
}

wxBoxSizer *ObjColorPanel::create_color_icon_and_rgba_sizer(wxWindow *parent, int id, const wxColour& color)
{
    auto      icon_sizer = new wxBoxSizer(wxHORIZONTAL);
    //icon_sizer->AddSpacer(FromDIP(40));
    wxButton *icon       = new wxButton(parent, wxID_ANY, {}, wxDefaultPosition, ICON_SIZE, wxBORDER_NONE | wxBU_AUTODRAW);
    icon->SetBitmap(*get_extruder_color_icon(color.GetAsString(wxC2S_HTML_SYNTAX).ToStdString(), std::to_string(id + 1), FromDIP(20), FromDIP(20)));
    icon->SetCanFocus(false);
    m_color_cluster_icon_list.emplace_back(icon);
    icon_sizer->Add(icon, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0); // wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM
    icon_sizer->AddSpacer(FromDIP(50));

    std::string   message    = get_color_str(color);
    wxStaticText *rgba_title = new wxStaticText(parent, wxID_ANY, message.c_str());
    m_color_cluster_text_list.emplace_back(rgba_title);
    rgba_title->SetMinSize(wxSize(FromDIP(COLOR_LABEL_WIDTH), -1));
    rgba_title->SetMaxSize(wxSize(FromDIP(COLOR_LABEL_WIDTH), -1));
    rgba_title->SetFont(Label::Body_12);
    icon_sizer->Add(rgba_title, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);
    return icon_sizer;
}

void ObjColorPanel::update_color_icon_and_rgba_sizer(int id, const wxColour &color)
{
    if (id < m_color_cluster_text_list.size()) {
        auto icon = m_color_cluster_icon_list[id];
        icon->SetBitmap(*get_extruder_color_icon(color.GetAsString(wxC2S_HTML_SYNTAX).ToStdString(), std::to_string(id + 1), FromDIP(16), FromDIP(16)));
        std::string message = get_color_str(color);
        m_color_cluster_text_list[id]->SetLabelText(message.c_str());
    }
}
