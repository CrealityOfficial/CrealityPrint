#include <algorithm>
#include <sstream>
#include "libslic3r/FlushVolCalc.hpp"
#include "TransmittanceDialog.hpp"
#include "BitmapCache.hpp"
#include "GUI.hpp"
#include "I18N.hpp"
#include "GUI_App.hpp"
#include "MsgDialog.hpp"
#include "libslic3r/Color.hpp"
#include "Widgets/Button.hpp"
#include "Widgets/CheckBox.hpp"
#include "slic3r/Utils/ColorSpaceConvert.hpp"
#include "MainFrame.hpp"
#include "libslic3r/Config.hpp"
#include <boost/log/trivial.hpp>
#include "WipeTowerDialog.hpp"

using namespace Slic3r;
using namespace Slic3r::GUI;

int ITEM_WIDTH_SCALE(){ return 30 * Slic3r::GUI::wxGetApp().em_unit() / 10;}
static const wxColour g_text_color = wxColour(107, 107, 107, 255);
//
#define ICON_SIZE_TD wxSize(FromDIP(16), FromDIP(16))
#define TABLE_BORDER FromDIP(28)
#define HEADER_VERT_PADDING FromDIP(12)
#define HEADER_BEG_PADDING FromDIP(30)
#define ICON_GAP FromDIP(44)
#define HEADER_END_PADDING FromDIP(24)
#define ROW_VERT_PADDING FromDIP(6)
#define ROW_BEG_PADDING FromDIP(20)
#define EDIT_BOXES_GAP FromDIP(30)
#define ROW_END_PADDING FromDIP(21)
#define BTN_SIZE wxSize(FromDIP(58), FromDIP(24))
#define BTN_GAP FromDIP(20)
#define TEXT_BEG_PADDING FromDIP(30)
#define MAX_FLUSH_VALUE 9999
static constexpr float MIN_TRANSMITTANCE_SKIN_DEPTH = 0.8f;
#define MIN_WIPING_DIALOG_WIDTH FromDIP(300)
#define TIP_MESSAGES_PADDING FromDIP(8)

static void update_ui(wxWindow* window) { Slic3r::GUI::wxGetApp().UpdateDarkUI(window); }

static StateColor transmittance_ok_btn_bg()
{
    return StateColor(std::pair<wxColour, int>(wxColour(23, 204, 95), StateColor::Pressed),
                      std::pair<wxColour, int>(wxColour(23, 204, 95, 0.9 * 255), StateColor::Hovered),
                      std::pair<wxColour, int>(wxColour(23, 204, 95), StateColor::Normal));
}

static StateColor transmittance_ok_btn_bd()
{
    return StateColor(std::pair<wxColour, int>(wxColour(0, 150, 136), StateColor::Normal));
}

static StateColor transmittance_ok_btn_text()
{
    return StateColor(std::pair<wxColour, int>(wxColour(255, 255, 254), StateColor::Normal));
}

static StateColor transmittance_cancel_btn_bg()
{
    return StateColor(std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Pressed),
                      std::pair<wxColour, int>(wxColour(238, 238, 238), StateColor::Hovered),
                      std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Normal));
}

static StateColor transmittance_cancel_btn_bd()
{
    return StateColor(std::pair<wxColour, int>(wxColour(38, 46, 48), StateColor::Normal));
}

static StateColor transmittance_cancel_btn_text()
{
    return StateColor(std::pair<wxColour, int>(wxColour(38, 46, 48), StateColor::Normal));
}

static void update_transmittance_ok_btn_style(Button* ok_btn, bool highlighted)
{
    if (ok_btn == nullptr)
        return;

    ok_btn->SetBackgroundColor(highlighted ? transmittance_ok_btn_bg() : transmittance_cancel_btn_bg());
    ok_btn->SetBorderColor(highlighted ? transmittance_ok_btn_bd() : transmittance_cancel_btn_bd());
    ok_btn->SetTextColor(highlighted ? transmittance_ok_btn_text() : transmittance_cancel_btn_text());
    ok_btn->Refresh();
}

#ifdef _WIN32
#define style wxSP_ARROW_KEYS | wxBORDER_SIMPLE
#else
#define style wxSP_ARROW_KEYS
#endif



#ifdef _WIN32
#define style wxSP_ARROW_KEYS | wxBORDER_SIMPLE
#else
#define style wxSP_ARROW_KEYS
#endif

wxBoxSizer* TransmittanceDialog::create_btn_sizer(long flags)
{
    auto btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    btn_sizer->AddStretchSpacer();

    if (flags & wxOK) {
        Button* ok_btn = new Button(this, _L("OK"));
        ok_btn->SetMinSize(wxSize(FromDIP(75), FromDIP(24)));
        ok_btn->SetCornerRadius(FromDIP(12));
        update_transmittance_ok_btn_style(ok_btn, true);
        ok_btn->SetFocus();
        ok_btn->SetId(wxID_OK);

        btn_sizer->Add(ok_btn, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, BTN_GAP);

        m_button_list[wxOK] = ok_btn;
    }
    if (flags & wxCANCEL) {
        Button* cancel_btn = new Button(this, _L("Cancel"));
        cancel_btn->SetMinSize(BTN_SIZE);
        cancel_btn->SetCornerRadius(FromDIP(12));
        cancel_btn->SetBackgroundColor(transmittance_cancel_btn_bg());
        cancel_btn->SetBorderColor(transmittance_cancel_btn_bd());
        cancel_btn->SetTextColor(transmittance_cancel_btn_text());
        cancel_btn->SetId(wxID_CANCEL);
        btn_sizer->Add(cancel_btn, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, BTN_GAP);
        m_button_list[wxCANCEL] = cancel_btn;
    }

    return btn_sizer;
}

void TransmittanceDialog::on_dpi_changed(const wxRect& suggested_rect)
{
    for (auto button_item : m_button_list) {
        if (button_item.first == wxRESET) {
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
    m_panel_wiping->msw_rescale();
    this->Refresh();
};

// Parent dialog for purging volume adjustments - it fathers TransmittancePanel widget (that contains all controls) and a button to toggle
// simple/advanced mode:
TransmittanceDialog::TransmittanceDialog(wxWindow*                       parent,
                           const std::vector<float>&       matrix,
                           const std::vector<float>&       extruders,
                           const std::vector<std::string>& extruder_colours,
                           const std::vector<int>&         extra_flush_volume,
                           float                           nozzle_diameter,
                           bool                            flush_into_skeleton,
                           bool                            show_flush_into_skeleton)
    : DPIDialog(parent ? parent : static_cast<wxWindow*>(wxGetApp().mainframe),
                wxID_ANY,
                _(L("Skeleton flush skin matrix")),
                wxDefaultPosition,
                wxDefaultSize,
                wxDEFAULT_DIALOG_STYLE /* | wxRESIZE_BORDER*/)
    , m_flush_into_skeleton(flush_into_skeleton)
{
    std::string icon_path = (boost::format("%1%/images/Creative3DTitle.ico") % Slic3r::resources_dir()).str();
    SetIcon(wxIcon(Slic3r::encode_path(icon_path.c_str()), wxBITMAP_TYPE_ICO));

    auto m_line_top = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    m_line_top->SetBackgroundColour(wxColour(166, 169, 170));

    this->SetBackgroundColour(*wxWHITE);
    this->SetMinSize(wxSize(MIN_WIPING_DIALOG_WIDTH, -1));

    m_panel_wiping = new TransmittancePanel(this, matrix, extruders, extruder_colours, nullptr, extra_flush_volume, nozzle_diameter);

    // set min sizer width according to extruders count
    auto sizer_width = (int) ((sqrt(matrix.size()) + 2.8) * ITEM_WIDTH_SCALE());
    sizer_width      = sizer_width > MIN_WIPING_DIALOG_WIDTH ? sizer_width : MIN_WIPING_DIALOG_WIDTH;

    auto main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->Add(m_line_top, 0, wxEXPAND, 0);

    if (show_flush_into_skeleton) {
        wxString bgColor = wxGetApp().dark_mode() ? "#4B4B4D" : "#FFFFFF";
        wxString fgColor = wxGetApp().dark_mode() ? "#FFFFFF" : "#30373D";
        auto enable_panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
        enable_panel->SetMinSize(wxSize(sizer_width, FromDIP(48)));
        enable_panel->SetMaxSize(wxSize(sizer_width, FromDIP(48)));
        enable_panel->SetBackgroundColour(bgColor);
        auto enable_sizer = new wxBoxSizer(wxHORIZONTAL);
        enable_panel->SetSizer(enable_sizer);

        auto enable_label = new wxStaticText(enable_panel, wxID_ANY, _L("Enable flushing into objects skeleton"));
        enable_label->SetFont(Label::Body_13);
        const int label_width = std::max(FromDIP(120), sizer_width - FromDIP(72));
        const int label_best_width = std::min(enable_label->GetBestSize().x, label_width);
        enable_label->SetMinSize(wxSize(label_best_width, FromDIP(20)));
        enable_label->SetMaxSize(wxSize(label_width, FromDIP(20)));
        enable_label->SetForegroundColour(fgColor);
        enable_label->SetBackgroundColour(bgColor);
        enable_sizer->Add(enable_label, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(15));

        m_flush_into_skeleton_checkbox = new ::CheckBox(enable_panel);
        m_flush_into_skeleton_checkbox->SetMinSize(wxSize(FromDIP(24), FromDIP(24)));
        m_flush_into_skeleton_checkbox->SetMaxSize(wxSize(FromDIP(24), FromDIP(24)));
        m_flush_into_skeleton_checkbox->SetValue(flush_into_skeleton);
        enable_sizer->Add(m_flush_into_skeleton_checkbox, 0, wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, FromDIP(8));
        main_sizer->Add(enable_panel, 0, wxEXPAND, 0);
    }
    main_sizer->SetMinSize(wxSize(sizer_width, -1));
    main_sizer->Add(m_panel_wiping, 1, wxEXPAND | wxALL, 0);

    auto btn_sizer = create_btn_sizer(wxOK | wxCANCEL);
    main_sizer->Add(btn_sizer, 0, wxBOTTOM | wxRIGHT | wxEXPAND, BTN_GAP);
    SetSizer(main_sizer);
    main_sizer->SetSizeHints(this);
    if (this->FindWindowById(wxID_OK, this)) {
        this->FindWindowById(wxID_OK, this)
            ->Bind(
                wxEVT_BUTTON,
                [this](wxCommandEvent&) {                                         // if OK button is clicked..
                    if (m_flush_into_skeleton_checkbox != nullptr)
                        m_flush_into_skeleton = m_flush_into_skeleton_checkbox->GetValue();
                    m_output_matrix    = m_panel_wiping->read_matrix_values();    // ..query wiping panel and save returned values
                    m_output_extruders = m_panel_wiping->read_extruders_values(); // so they can be recovered later by calling get_...()
                    EndModal(wxID_OK);
                },
                wxID_OK);
    }
    if (this->FindWindowById(wxID_CANCEL, this)) {
        update_ui(static_cast<wxButton*>(this->FindWindowById(wxID_CANCEL, this)));
        this->FindWindowById(wxID_CANCEL, this)->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxCANCEL); });
    }

    this->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& e) { EndModal(wxCANCEL); });
    this->Bind(wxEVT_CHAR_HOOK, [this](wxKeyEvent& e) {
        if (e.GetKeyCode() == WXK_ESCAPE) {
            if (this->IsModal())
                this->EndModal(wxID_CANCEL);
            else
                this->Close();
        } else
            e.Skip();
    });

    wxGetApp().UpdateDlgDarkUI(this);
}

void TransmittancePanel::create_panels(wxWindow* parent, const int num)
{
    for (size_t i = 0; i < num; i++) {
        wxPanel* panel = new wxPanel(parent);
        panel->SetBackgroundColour(i % 2 == 0 ? *wxWHITE : wxColour(238, 238, 238));
        auto sizer = new wxBoxSizer(wxHORIZONTAL);
        panel->SetSizer(sizer);

        wxButton*   icon = new wxButton(panel, wxID_ANY, {}, wxDefaultPosition, ICON_SIZE_TD, wxBORDER_NONE | wxBU_AUTODRAW);
        std::string color_str;
        if (i < m_colours.size() && m_colours[i].IsOk()) {
            color_str = m_colours[i].GetAsString(wxC2S_HTML_SYNTAX).ToStdString();
        } else {
            const char* palette[] = {"#F0F0F0FF", "#FFCC00FF", "#66CCFFFF", "#CC66FFFF", "#99CC33FF", "#FF9966FF", "#6699FFFF"};
            auto        is_used   = [&](const std::string& s) -> bool {
                for (size_t k = 0; k < m_colours.size(); ++k) {
                    if (m_colours[k].IsOk() && m_colours[k].GetAsString(wxC2S_HTML_SYNTAX).ToStdString() == s)
                        return true;
                }
                return false;
            };
            for (const char* c : palette) {
                if (!is_used(c)) {
                    color_str = c;
                    break;
                }
            }
            if (color_str.empty()) {
                int r = (37 * int(i) + 50) % 206 + 50;
                int g = (71 * int(i) + 80) % 206 + 50;
                int b = (97 * int(i) + 110) % 206 + 50;
                for (int t = 0; t < 6; ++t) {
                    wxColour tmp(r, g, b, 0xFF);
                    auto     s = tmp.GetAsString(wxC2S_HTML_SYNTAX).ToStdString();
                    if (!is_used(s)) {
                        color_str = s;
                        break;
                    }
                    r = (r + 33) % 256;
                    g = (g + 55) % 256;
                    b = (b + 77) % 256;
                }
                if (color_str.empty())
                    color_str = "#F0F0F0FF";
            }
            BOOST_LOG_TRIVIAL(warning) << "[TransmittancePanel::create_panels] Fallback color applied (row icon) i=" << i
                                       << ", m_colours.size=" << m_colours.size() << ", chosen=" << color_str;
        }
        BOOST_LOG_TRIVIAL(warning) << "[TransmittancePanel::create_panels] BEFORE SetBitmap (row icon) i=" << i
                                   << ", m_colours.size=" << m_colours.size() << ", color_str=" << color_str;
        icon->SetBitmap(*get_extruder_color_icon(color_str, std::to_string(i + 1), FromDIP(16), FromDIP(16)));
        BOOST_LOG_TRIVIAL(warning) << "[TransmittancePanel::create_panels] AFTER SetBitmap (row icon) i=" << i << ", icon_ptr=" << (void*) icon;
        icon->SetCanFocus(false);
        icon_list2.push_back(icon);

        sizer->AddSpacer(ROW_BEG_PADDING);
        sizer->Add(icon, 0, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, ROW_VERT_PADDING);

        for (int j = 0; j < num; ++j) {
            edit_boxes[j][i]->Reparent(panel);
            edit_boxes[j][i]->SetBackgroundColour(panel->GetBackgroundColour());
            edit_boxes[j][i]->SetFont(::Label::Body_13);
            sizer->AddSpacer(EDIT_BOXES_GAP);
            sizer->Add(edit_boxes[j][i], 0, wxALIGN_CENTER_VERTICAL, 0);
        }
        sizer->AddSpacer(ROW_END_PADDING);

        m_sizer_advanced->Add(panel, 0, wxRIGHT | wxLEFT | wxEXPAND, TABLE_BORDER);
        panel->Layout();
    }
}

// This panel contains all control widgets for both simple and advanced mode (these reside in separate sizers)
TransmittancePanel::TransmittancePanel(wxWindow*                       parent,
                         const std::vector<float>&       matrix,
                         const std::vector<float>&       extruders,
                         const std::vector<std::string>& extruder_colours,
                         Button*                         calc_button,
                         const std::vector<int>&         skin_depths,
                         float                           nozzle_diameter)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize /*,wxBORDER_RAISED*/)
    , m_matrix(matrix)
    , m_min_transmittance_skin_depth(std::max(nozzle_diameter, MIN_TRANSMITTANCE_SKIN_DEPTH))
    , m_min_skin_depth(skin_depths)
    , m_max_skin_depth(Slic3r::g_max_flush_volume)
{
    m_number_of_extruders = (int) (sqrt(matrix.size()) + 0.001);

    for (const std::string& color : extruder_colours) {
        Slic3r::ColorRGB rgb;
        Slic3r::decode_color(color, rgb);
        m_colours.push_back(wxColor(rgb.r_uchar(), rgb.g_uchar(), rgb.b_uchar()));
    }
    auto sizer_width = (int) ((sqrt(matrix.size())) *ITEM_WIDTH_SCALE() + (sqrt(matrix.size()) + 1) * HEADER_BEG_PADDING);
    sizer_width      = sizer_width > MIN_WIPING_DIALOG_WIDTH ? sizer_width : MIN_WIPING_DIALOG_WIDTH;
    // Create two switched panels with their own sizers
    m_sizer_simple   = new wxBoxSizer(wxVERTICAL);
    m_sizer_advanced = new wxBoxSizer(wxVERTICAL);
    m_page_simple    = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    m_page_advanced  = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    m_page_simple->SetSizer(m_sizer_simple);
    m_page_advanced->SetSizer(m_sizer_advanced);
    m_page_advanced->SetBackgroundColour(*wxWHITE);

    update_ui(m_page_simple);
    update_ui(m_page_advanced);

    auto gridsizer_simple = new wxGridSizer(3, 5, 10);
    m_gridsizer_advanced  = new wxGridSizer(m_number_of_extruders + 1, 5, 1);

    // First create controls for advanced mode and assign them to m_page_advanced:
    for (unsigned int i = 0; i < m_number_of_extruders; ++i) {
        edit_boxes.push_back(std::vector<wxTextCtrl*>(0));

        for (unsigned int j = 0; j < m_number_of_extruders; ++j) {
            bool is_diag = (i == j);
#ifdef _WIN32
            wxTextCtrl* text = new wxTextCtrl(m_page_advanced, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(ITEM_WIDTH_SCALE(), -1),
                                              wxTE_CENTER | wxBORDER_NONE | wxTE_PROCESS_ENTER);
            update_ui(text);
            edit_boxes.back().push_back(text);
#elif __linux__
            wxTextCtrl* text = new wxTextCtrl(m_page_advanced, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(ITEM_WIDTH_SCALE(), -1),
                                              wxTE_CENTER | wxBORDER_NONE | wxTE_PROCESS_ENTER);
            update_ui(text);
            edit_boxes.back().push_back(text);
#else
            edit_boxes.back().push_back(
                new wxTextCtrl(m_page_advanced, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(ITEM_WIDTH_SCALE(), -1),
                               0));
#endif
            float v = m_matrix[m_number_of_extruders * j + i];
            if (is_diag && v <= 0.f)
                v = MIN_TRANSMITTANCE_SKIN_DEPTH;
            wxString disp;
            if (v == std::floor(v))
                disp = wxString::Format("%g", v);
            else
                disp = wxString::Format("%.1f", v);
            edit_boxes[i][j]->SetValue(disp);
            m_matrix[m_number_of_extruders * j + i] = v;

            edit_boxes[i][j]->Bind(wxEVT_TEXT, [this, i, j](wxCommandEvent& e) {
                wxString str   = edit_boxes[i][j]->GetValue();
                double   value = 0.0;
                str.ToDouble(&value);
                if (value > MAX_FLUSH_VALUE) {
                    edit_boxes[i][j]->SetValue(wxString::Format("%g", (double) MAX_FLUSH_VALUE));
                } else if (value > 0.0 && value < (double) m_min_transmittance_skin_depth) {
                    // Value is positive but below the minimum skeleton flush skin depth: show red warning.
                    // Actual clamp happens on focus-lost / Enter.
                    edit_boxes[i][j]->SetForegroundColour(wxColour(220, 60, 60));
                } else {
                    edit_boxes[i][j]->SetForegroundColour(wxNullColour);
                }
            });

            auto on_apply_text_modify = [this, i, j](wxEvent& e) {
                wxString str  = edit_boxes[i][j]->GetValue();
                double   value = 0.0;
                str.ToDouble(&value);
                // Clamp: negative -> 0; positive but below the minimum skeleton flush skin depth -> snap up to the threshold.
                if (value < 0.0)
                    value = 0.0;
                else if (value > 0.0 && value < (double) m_min_transmittance_skin_depth)
                    value = (double) m_min_transmittance_skin_depth;
                // Refresh display and reset warning colour.
                wxString clamped = (value == std::floor(value))
                    ? wxString::Format("%g", value)
                    : wxString::Format("%.2f", value);
                edit_boxes[i][j]->ChangeValue(clamped); // ChangeValue does NOT fire wxEVT_TEXT
                edit_boxes[i][j]->SetForegroundColour(wxNullColour);
                m_matrix[m_number_of_extruders * j + i] = (float) value;
                this->update_warning_texts();
                e.Skip();
            };

            edit_boxes[i][j]->Bind(wxEVT_TEXT_ENTER, on_apply_text_modify);
            edit_boxes[i][j]->Bind(wxEVT_KILL_FOCUS, on_apply_text_modify);
            
        }
    }

    // BBS
    m_sizer_advanced->AddSpacer(FromDIP(10));
    auto tip_message_panel = new wxPanel(m_page_advanced, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    tip_message_panel->SetBackgroundColour(wxColour(238, 238, 238));
    auto message_sizer = new wxBoxSizer(wxVERTICAL);
    tip_message_panel->SetSizer(message_sizer);
    {
        wxString message    = _L("CrealityPrint uses this matrix only when flushing into objects skeleton is enabled.");
        m_tip_message_label = new Label(tip_message_panel, wxEmptyString);
        wxClientDC dc(tip_message_panel);
        wxString   multiline_message;
        m_tip_message_label->split_lines(dc, sizer_width, message, multiline_message);
        m_tip_message_label->SetLabel(multiline_message);
        m_tip_message_label->SetFont(Label::Body_13);
        message_sizer->Add(m_tip_message_label, 0, wxEXPAND | wxALL, TIP_MESSAGES_PADDING);
    }
    m_sizer_advanced->Add(tip_message_panel, 0, wxEXPAND | wxRIGHT | wxLEFT, TABLE_BORDER);
    bool is_show = wxGetApp().app_config->get("auto_calculate") == "true" ||
                   wxGetApp().app_config->get("auto_calculate_when_filament_change") == "true";
    tip_message_panel->Show(is_show);
    m_sizer_advanced->AddSpacer(FromDIP(10));

    header_line_panel = new wxPanel(m_page_advanced, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    header_line_panel->SetBackgroundColour(wxColour(238, 238, 238));
    auto header_line_sizer = new wxBoxSizer(wxHORIZONTAL);
    header_line_panel->SetSizer(header_line_sizer);

    header_line_sizer->AddSpacer(HEADER_BEG_PADDING);
    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " m_number_of_extruders=" << m_number_of_extruders
                               << " m_colours.size=" << m_colours.size();
    boost::log::core::get()->flush();

    for (unsigned int i = 0; i < m_number_of_extruders; ++i) {
        wxButton*   icon = new wxButton(header_line_panel, wxID_ANY, {}, wxDefaultPosition, ICON_SIZE_TD, wxBORDER_NONE | wxBU_AUTODRAW);
        std::string color_str;
        if (i < m_colours.size() && m_colours[i].IsOk()) {
            color_str = m_colours[i].GetAsString(wxC2S_HTML_SYNTAX).ToStdString();
        } else {
            const char* palette[] = {"#F0F0F0FF", "#FFCC00FF", "#66CCFFFF", "#CC66FFFF", "#99CC33FF", "#FF9966FF", "#6699FFFF"};
            auto        is_used   = [&](const std::string& s) -> bool {
                for (size_t k = 0; k < m_colours.size(); ++k) {
                    if (m_colours[k].IsOk() && m_colours[k].GetAsString(wxC2S_HTML_SYNTAX).ToStdString() == s)
                        return true;
                }
                return false;
            };
            for (const char* c : palette) {
                if (!is_used(c)) {
                    color_str = c;
                    break;
                }
            }
            if (color_str.empty()) {
                int r = (37 * int(i) + 50) % 206 + 50;
                int g = (71 * int(i) + 80) % 206 + 50;
                int b = (97 * int(i) + 110) % 206 + 50;
                for (int t = 0; t < 6; ++t) {
                    wxColour tmp(r, g, b, 0xFF);
                    auto     s = tmp.GetAsString(wxC2S_HTML_SYNTAX).ToStdString();
                    if (!is_used(s)) {
                        color_str = s;
                        break;
                    }
                    r = (r + 33) % 256;
                    g = (g + 55) % 256;
                    b = (b + 77) % 256;
                }
                if (color_str.empty())
                    color_str = "#F0F0F0FF";
            }
            BOOST_LOG_TRIVIAL(warning) << "[TransmittancePanel::create_panels] Fallback color applied (header icon) i=" << i
                                       << ", m_colours.size=" << m_colours.size() << ", chosen=" << color_str;
        }
        BOOST_LOG_TRIVIAL(warning) << "[TransmittancePanel::create_panels] BEFORE SetBitmap (header icon) i=" << i
                                   << ", m_colours.size=" << m_colours.size() << ", color_str=" << color_str;
        icon->SetBitmap(*get_extruder_color_icon(color_str, std::to_string(i + 1), FromDIP(16), FromDIP(16)));
        BOOST_LOG_TRIVIAL(warning) << "[TransmittancePanel::create_panels] AFTER SetBitmap (header icon) i=" << i << ", icon_ptr=" << (void*) icon;
        icon->SetCanFocus(false);
        icon_list1.push_back(icon);

        header_line_sizer->AddSpacer(ICON_GAP);
        header_line_sizer->Add(icon, 0, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, HEADER_VERT_PADDING);
    }
    header_line_sizer->AddSpacer(HEADER_END_PADDING);

    m_sizer_advanced->Add(header_line_panel, 0, wxEXPAND | wxRIGHT | wxLEFT, TABLE_BORDER);

    create_panels(m_page_advanced, m_number_of_extruders);
    boost::log::core::get()->flush();

    {
        auto multi_desc_label = new wxStaticText(m_page_advanced, wxID_ANY, _(L("skeleton flush skin thickness for each filament pair.")),
                                                 wxDefaultPosition, wxDefaultSize, 0);
        multi_desc_label->SetForegroundColour(g_text_color);
        m_sizer_advanced->Add(multi_desc_label, 0, wxEXPAND | wxLEFT, TEXT_BEG_PADDING);
        wxFont multi_desc_label_font = multi_desc_label->GetFont();
        multi_desc_label_font.SetPointSize(10);
        multi_desc_label->SetFont(multi_desc_label_font);
        m_sizer_advanced->AddSpacer(10);
    }

    this->update_warning_texts();

    m_page_advanced->Hide();

    // Now the same for simple mode:
    gridsizer_simple->Add(new wxStaticText(m_page_simple, wxID_ANY, wxString("")), 0, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL);
    gridsizer_simple->Add(new wxStaticText(m_page_simple, wxID_ANY, wxString(_(L("unloaded")))), 0,
                          wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL);
    gridsizer_simple->Add(new wxStaticText(m_page_simple, wxID_ANY, wxString(_(L("loaded")))), 0, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL);

    auto add_spin_ctrl = [this](std::vector<wxSpinCtrl*>& vec, float initial) {
        wxSpinCtrl* spin_ctrl = new wxSpinCtrl(m_page_simple, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(ITEM_WIDTH_SCALE(), -1),
                                               style | wxALIGN_RIGHT, 0, 300, (int) initial);
        update_ui(spin_ctrl);
        vec.push_back(spin_ctrl);

#ifdef __WXOSX__
        // On OSX / Cocoa, wxSpinCtrl::GetValue() doesn't return the new value
        // when it was changed from the text control, so the on_change callback
        // gets the old one, and on_kill_focus resets the control to the old value.
        // As a workaround, we get the new value from $event->GetString and store
        // here temporarily so that we can return it from get_value()
        spin_ctrl->Bind(wxEVT_TEXT, ([spin_ctrl](wxCommandEvent e) {
                            long       value;
                            const bool parsed    = e.GetString().ToLong(&value);
                            int        tmp_value = parsed && value >= INT_MIN && value <= INT_MAX ? (int) value : INT_MIN;

                            // Forcibly set the input value for SpinControl, since the value
                            // inserted from the keyboard or clipboard is not updated under OSX
                            if (tmp_value != INT_MIN) {
                                spin_ctrl->SetValue(tmp_value);

                                // But in SetValue() is executed m_text_ctrl->SelectAll(), so
                                // discard this selection and set insertion point to the end of string
                                spin_ctrl->GetText()->SetInsertionPointEnd();
                            }
                        }),
                        spin_ctrl->GetId());
#endif
    };

    for (unsigned int i = 0; i < m_number_of_extruders; ++i) {
        add_spin_ctrl(m_old, extruders[2 * i]);
        add_spin_ctrl(m_new, extruders[2 * i + 1]);

        auto      hsizer = new wxBoxSizer(wxHORIZONTAL);
        wxWindow* w      = new wxWindow(m_page_simple, wxID_ANY, wxDefaultPosition, ICON_SIZE_TD, wxBORDER_SIMPLE);
        w->SetCanFocus(false);
        w->SetBackgroundColour(m_colours[i]);
        hsizer->Add(w, wxALIGN_CENTER_VERTICAL);
        hsizer->AddSpacer(10);
        hsizer->Add(new wxStaticText(m_page_simple, wxID_ANY, wxString(_(L("Filament #"))) << i + 1 << ": "), 0,
                    wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

        gridsizer_simple->Add(hsizer, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);
        gridsizer_simple->Add(m_old.back(), 0);
        gridsizer_simple->Add(m_new.back(), 0);
    }

    m_sizer_simple->Add(gridsizer_simple, 0, wxEXPAND | wxALL, FromDIP(10));

    m_sizer = new wxBoxSizer(wxVERTICAL);
    m_sizer->Add(m_page_simple, 0, wxEXPAND, 0);
    m_sizer->Add(m_page_advanced, 0, wxEXPAND, 0);

    SetSizer(m_sizer);

    toggle_advanced(); // to show/hide what is appropriate before calculating size hints
    m_sizer->SetSizeHints(this);
    this->Layout();

    header_line_panel->Bind(wxEVT_PAINT, [this](wxPaintEvent&) {
        wxPaintDC dc(header_line_panel);
        wxString  from_text      = _L("From");
        wxString  to_text        = _L("To");
        wxSize    from_text_size = dc.GetTextExtent(from_text);
        wxSize    to_text_size   = dc.GetTextExtent(to_text);

        int base_y    = (header_line_panel->GetSize().y - from_text_size.y - to_text_size.y) / 2;
        int vol_width = ROW_BEG_PADDING + EDIT_BOXES_GAP / 2 + ICON_SIZE_TD.x;
        int base_x    = (vol_width - from_text_size.x - to_text_size.x) / 2;

        // draw from text
        int x = base_x;
        int y = base_y + to_text_size.y;
        dc.DrawText(from_text, x, y);

        // draw to text
        x = base_x + from_text_size.x;
        y = base_y;
        dc.DrawText(to_text, x, y);

        // draw a line
        int p1_x = base_x + from_text_size.x - to_text_size.y;
        int p1_y = base_y;
        int p2_x = base_x + from_text_size.x + from_text_size.y;
        int p2_y = base_y + from_text_size.y + to_text_size.y;
        dc.SetPen(wxPen(wxColour(172, 172, 172, 1)));
        dc.DrawLine(p1_x, p1_y, p2_x, p2_y);
    });
    wxFont basic_font = header_line_panel->GetFont();
    basic_font.SetPointSize(10);
    header_line_panel->SetFont(basic_font);
}

void TransmittancePanel::update_warning_texts()
{
    static const wxColour g_warning_color = *wxRED;
    static const wxColour g_normal_color  = *wxBLACK;

    //wxString multi_str           = m_flush_multiplier_ebox->GetValue();
    float    multiplier          = 1.f/*wxAtof(multi_str)*/;
    bool     has_exception_flush = false;
    bool     has_diff_type_flush = false;
    for (int i = 0; i < edit_boxes.size(); i++) {
        auto& box_vec = edit_boxes[i];
        for (int j = 0; j < box_vec.size(); j++) {
            if (i == j)
                continue;
            auto     text_box      = box_vec[j];
   
            wxString str           = text_box->GetValue();
            double   actual_volume = 0.0;
            str.ToDouble(&actual_volume);
            bool     diffFilament  = actual_volume < m_type_skin_depth;
            bool     flushScope    = actual_volume < m_min_skin_depth[i] || actual_volume > m_max_skin_depth;
            if (diffFilament)
                has_diff_type_flush = true;
            if (flushScope)
                has_exception_flush = true;
            if (flushScope || (diffFilament)) {
                if (text_box->GetForegroundColour() != g_warning_color) {
                    text_box->SetForegroundColour(g_warning_color);
                    text_box->Refresh();
                }
            } else {
                if (text_box->GetForegroundColour() != g_normal_color) {
                    text_box->SetForegroundColour(StateColor::darkModeColorFor(g_normal_color));
                    text_box->Refresh();
                }
            }
        }
    }
}

void TransmittancePanel::msw_rescale()
{
    for (unsigned int i = 0; i < icon_list1.size(); ++i) {
        auto bitmap = *get_extruder_color_icon(m_colours[i].GetAsString(wxC2S_HTML_SYNTAX).ToStdString(), std::to_string(i + 1),
                                               FromDIP(16), FromDIP(16));
        icon_list1[i]->SetBitmap(bitmap);
        icon_list2[i]->SetBitmap(bitmap);
    }
}

// Reads values from the (advanced) wiping matrix:
std::vector<float> TransmittancePanel::read_matrix_values()
{
    if (!m_advanced)
        fill_in_matrix();
    std::vector<float> output;
    for (unsigned int i = 0; i < m_number_of_extruders; ++i) {
        for (unsigned int j = 0; j < m_number_of_extruders; ++j) {
            double val = 0.;
            edit_boxes[j][i]->GetValue().ToDouble(&val);
            output.push_back((float) val);
        }
    }
    return output;
}

// Reads values from simple mode to save them for next time:
std::vector<float> TransmittancePanel::read_extruders_values()
{
    std::vector<float> output;
    for (unsigned int i = 0; i < m_number_of_extruders; ++i) {
        output.push_back(m_old[i]->GetValue());
        output.push_back(m_new[i]->GetValue());
    }
    return output;
}

// This updates the "advanced" matrix based on values from "simple" mode
void TransmittancePanel::fill_in_matrix()
{
    for (unsigned i = 0; i < m_number_of_extruders; ++i) {
        for (unsigned j = 0; j < m_number_of_extruders; ++j) {
            if (i == j) {
                edit_boxes[j][i]->SetValue(wxString::Format("%.1f", (double) MIN_TRANSMITTANCE_SKIN_DEPTH));
                continue;
            }
            edit_boxes[j][i]->SetValue(wxString("") << (m_old[i]->GetValue() + m_new[j]->GetValue()));
        }
    }
}

// Function to check if simple and advanced settings are matching
bool TransmittancePanel::advanced_matches_simple()
{
    for (unsigned i = 0; i < m_number_of_extruders; ++i) {
        for (unsigned j = 0; j < m_number_of_extruders; ++j) {
            if (i == j)
                continue;
            if (edit_boxes[j][i]->GetValue() != (wxString("") << (m_old[i]->GetValue() + m_new[j]->GetValue())))
                return false;
        }
    }
    return true;
}

// Switches the dialog from simple to advanced mode and vice versa
void TransmittancePanel::toggle_advanced(bool user_action)
{
    if (user_action)
        m_advanced = !m_advanced; // user demands a change -> toggle
    else {
        // BBS: show advanced mode by default
        // m_advanced = !advanced_matches_simple(); // if called from constructor, show what is appropriate
        m_advanced = true;
    }

    (m_advanced ? m_page_advanced : m_page_simple)->Show();
    (!m_advanced ? m_page_advanced : m_page_simple)->Hide();

    if (m_advanced)
        if (user_action)
            fill_in_matrix(); // otherwise keep values loaded from config

    m_sizer->Layout();
    Refresh();
}
