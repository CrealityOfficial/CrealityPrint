#include "ColorDecomposeDialog.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <memory>
#include <numeric>
#include <set>
#include <wx/sizer.h>
#include <wx/dcclient.h>
#include <wx/dcbuffer.h>
#include "wx/graphics.h"

#include "I18N.hpp"
#include "GUI.hpp"
#include "GUI_App.hpp"
#include "format.hpp"
#include "Widgets/ComboBox.hpp"
#include "Widgets/DropDown.hpp"
#include "Widgets/Button.hpp"
#include "Widgets/CheckBox.hpp"
#include "Widgets/Label.hpp"
#include "wxExtensions.hpp"
#include "ColorDecomposeSupport.hpp"
#include "libslic3r/ColorDecomposeRecipe.hpp"
#include "libslic3r/filament_mixer.h"

namespace Slic3r {
namespace GUI {

static const wxColour COLOR_BRAND("#00AE42");
static const wxColour COLOR_BORDER_NORMAL("#EEEEEE");
static const wxColour COLOR_BG_CARD("#F8F8F8");
static const wxColour COLOR_LABEL_GREY("#ACACAC");
static const wxColour COLOR_TEXT_DARK("#262E30");
static const wxColour COLOR_DIVIDER("#EEEEEE");

// Standard CMYW base colors
static const wxColour CMYW_CYAN(0, 255, 255);
static const wxColour CMYW_MAGENTA(255, 0, 255);
static const wxColour CMYW_YELLOW(255, 255, 0);
static const wxColour CMYW_WHITE(255, 255, 255);

// Standard RYBW base colors
static const wxColour RYBW_RED(255, 0, 0);
static const wxColour RYBW_YELLOW(255, 255, 0);
static const wxColour RYBW_BLUE(0, 0, 255);
static const wxColour RYBW_WHITE(255, 255, 255);

static size_t mode_index(DecomposeMode mode)
{
    return static_cast<size_t>(mode);
}

static ColorDecomposeRgb wx_colour_to_recipe_rgb(const wxColour& color)
{
    return {
        static_cast<unsigned char>(color.Red()),
        static_cast<unsigned char>(color.Green()),
        static_cast<unsigned char>(color.Blue())
    };
}

static wxColour hex_to_wx_colour(const std::string& hex, const wxColour& fallback)
{
    wxColour color(hex);
    return color.IsOk() ? color : fallback;
}

static bool same_rgb(const wxColour& lhs, const wxColour& rhs)
{
    return lhs.Red() == rhs.Red() && lhs.Green() == rhs.Green() && lhs.Blue() == rhs.Blue();
}

static DecomposeBaseColor standard_base_color_from_key(const std::string& key)
{
    if (key == "Cyan")    return DecomposeBaseColor::Cyan;
    if (key == "Magenta") return DecomposeBaseColor::Magenta;
    if (key == "Yellow")  return DecomposeBaseColor::Yellow;
    if (key == "White")   return DecomposeBaseColor::White;
    if (key == "Red")     return DecomposeBaseColor::Red;
    if (key == "Green")   return DecomposeBaseColor::Green;
    if (key == "Blue")    return DecomposeBaseColor::Blue;
    return DecomposeBaseColor::None;
}

static wxColour pure_color_for_base(DecomposeBaseColor base)
{
    switch (base) {
    case DecomposeBaseColor::Cyan:    return CMYW_CYAN;
    case DecomposeBaseColor::Magenta: return CMYW_MAGENTA;
    case DecomposeBaseColor::Yellow:  return CMYW_YELLOW;
    case DecomposeBaseColor::White:   return CMYW_WHITE;
    case DecomposeBaseColor::Red:     return RYBW_RED;
    case DecomposeBaseColor::Blue:    return RYBW_BLUE;
    default:                          return *wxBLACK;
    }
}

static DecomposeBaseColor standard_base_color_for(DecomposeMode mode, const wxColour& color)
{
    if (mode == DecomposeMode::CMYW) {
        if (same_rgb(color, CMYW_CYAN))    return DecomposeBaseColor::Cyan;
        if (same_rgb(color, CMYW_MAGENTA)) return DecomposeBaseColor::Magenta;
        if (same_rgb(color, CMYW_YELLOW))  return DecomposeBaseColor::Yellow;
        if (same_rgb(color, CMYW_WHITE))   return DecomposeBaseColor::White;
    } else if (mode == DecomposeMode::RYBW) {
        if (same_rgb(color, RYBW_RED))     return DecomposeBaseColor::Red;
        if (same_rgb(color, RYBW_YELLOW))  return DecomposeBaseColor::Yellow;
        if (same_rgb(color, RYBW_BLUE))    return DecomposeBaseColor::Blue;
        if (same_rgb(color, RYBW_WHITE))   return DecomposeBaseColor::White;
    }
    return DecomposeBaseColor::None;
}

static ColorDecomposeResult to_dialog_result(const ColorDecomposeRecipeResult& recipe,
                                             const wxColour& fallback)
{
    ColorDecomposeResult result;
    result.mode = recipe.mode;
    result.matched_color = hex_to_wx_colour(recipe.matched_color_hex, fallback);
    for (const auto& comp_recipe : recipe.components) {
        DecomposeComponent comp;
        comp.colour = hex_to_wx_colour(comp_recipe.color_hex, fallback);
        comp.ratio = comp_recipe.ratio;
        comp.filament_index = static_cast<int>(comp_recipe.filament_index);
        comp.base_color = standard_base_color_from_key(comp_recipe.base_color);
        if (comp.base_color == DecomposeBaseColor::None)
            comp.base_color = standard_base_color_for(recipe.mode, comp.colour);
        result.components.push_back(comp);
    }
    return result;
}

static wxPanel* create_h_divider(wxWindow* parent, int fixed_width = -1)
{
    const int h = parent->FromDIP(1);
    int w = fixed_width > 0 ? fixed_width : -1;
    auto* panel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(w, h));
    panel->SetMinSize(wxSize(w, h));
    if (fixed_width > 0)
        panel->SetMaxSize(wxSize(fixed_width, h));
    panel->SetBackgroundColour(StateColor::darkModeColorFor(COLOR_DIVIDER));
    return panel;
}

static wxStaticText* create_mode_group_label(wxWindow* parent, const wxString& text)
{
    auto* label = new wxStaticText(parent, wxID_ANY, text);
    label->SetFont(Label::Body_11);
    label->SetForegroundColour(StateColor::darkModeColorFor(COLOR_LABEL_GREY));
    return label;
}

static void match_parent_bg(wxWindow* w, const wxColour& bg)
{
    w->SetBackgroundColour(bg);
}

// Extract base material type by removing common prefixes ("Hyper ", "Generic ")
// and suffixes (" Basic"). For example:
//   "Hyper PLA"     -> "PLA"
//   "Generic PLA"   -> "PLA"
//   "PLA Basic"     -> "PLA"
//   "PETG"          -> "PETG"
//   "Hyper PETG"    -> "PETG"
static std::string extract_base_type(const std::string& type)
{
    std::string result = type;
    // Remove common prefixes
    static const std::vector<std::string> prefixes = {"Hyper ", "Generic "};
    for (const auto& prefix : prefixes) {
        if (result.size() > prefix.size() && result.substr(0, prefix.size()) == prefix) {
            result = result.substr(prefix.size());
            break;
        }
    }
    // Remove " Basic" suffix
    static const std::string basic_suffix = " Basic";
    if (result.size() > basic_suffix.size() &&
        result.substr(result.size() - basic_suffix.size()) == basic_suffix) {
        result = result.substr(0, result.size() - basic_suffix.size());
    }
    return result;
}

static bool material_type_matches(const std::string& a, const std::string& b)
{
    if (a.empty() || b.empty())
        return false;
    // Direct match or Basic suffix match
    if (a == b || a == b + " Basic" || b == a + " Basic")
        return true;
    // Compare base types (e.g., "Hyper PLA" and "Generic PLA" both -> "PLA")
    return extract_base_type(a) == extract_base_type(b);
}


ColorDecomposeDialog::ColorDecomposeDialog(wxWindow* parent,
                                           int filament_idx,
                                           const wxColour& target_color,
                                           const std::vector<std::string>& physical_colors,
                                           const std::vector<std::string>& filament_names,
                                           const std::vector<std::string>& filament_types,
                                           size_t current_filament_count,
                                           size_t max_filament_count,
                                           std::vector<size_t> physical_config_indices)
    : DPIDialog(parent, wxID_ANY, _L("Decompose Color"), wxDefaultPosition,
                wxDefaultSize, wxCAPTION | wxCLOSE_BOX)
    , m_filament_idx(filament_idx)
    , m_target_color(target_color)
    , m_physical_colors(physical_colors)
    , m_filament_names(filament_names)
    , m_filament_types(filament_types)
    , m_current_filament_count(current_filament_count)
    , m_max_filament_count(max_filament_count)
    , m_physical_config_indices(std::move(physical_config_indices))
{
    for (const auto& t : m_filament_types) {
        if (std::find(m_project_types.begin(), m_project_types.end(), t) == m_project_types.end())
            m_project_types.push_back(t);
    }

    if (m_filament_idx >= 0 && static_cast<size_t>(m_filament_idx) < m_filament_types.size())
        m_preferred_type = m_filament_types[m_filament_idx];
    else if (!m_project_types.empty())
        m_preferred_type = m_project_types.front();

    build_ui();
    wxGetApp().UpdateDlgDarkUI(this);
    // Restore target swatch after dark mode color remapping
    if (m_target_swatch)
        m_target_swatch->SetBackgroundColour(m_target_color);

    update_card_visibility();
    Fit();
    compute_decomposition();
    update_matched_color_display();
    update_ok_button_state();
}

void ColorDecomposeDialog::on_dpi_changed(const wxRect& suggested_rect)
{
    (void)suggested_rect;
    Fit();
    Refresh();
}

void ColorDecomposeDialog::build_ui()
{
    SetBackgroundColour(StateColor::darkModeColorFor(*wxWHITE));

    auto* main_sizer = new wxBoxSizer(wxVERTICAL);

    const int selector_side_margin = FromDIP(26);
    const int selector_top_gap = FromDIP(22);
    const int content_side_margin = FromDIP(30);
    const int target_section_top_gap = FromDIP(18);

    main_sizer->AddSpacer(selector_top_gap);
    main_sizer->Add(create_filament_selector(), 0, wxEXPAND | wxLEFT | wxRIGHT, selector_side_margin);
    main_sizer->AddSpacer(target_section_top_gap);
    main_sizer->Add(create_target_color_section(), 0, wxEXPAND | wxLEFT | wxRIGHT, content_side_margin);
    main_sizer->AddSpacer(FromDIP(16));
    main_sizer->Add(create_h_divider(this), 0, wxEXPAND | wxLEFT | wxRIGHT, content_side_margin);
    main_sizer->AddSpacer(FromDIP(16));
    main_sizer->Add(create_mode_selection_section(), 0, wxEXPAND | wxLEFT | wxRIGHT, content_side_margin);
    main_sizer->AddSpacer(FromDIP(24));
    main_sizer->AddStretchSpacer();
    main_sizer->Add(create_button_panel(), 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT | wxBOTTOM, content_side_margin);

    SetSizer(main_sizer);
    Fit();
    CenterOnParent();
}

wxBoxSizer* ColorDecomposeDialog::create_filament_selector()
{
    auto* sizer = new wxBoxSizer(wxHORIZONTAL);

    m_type_combo = new ComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                wxSize(-1, FromDIP(36)), 0, nullptr, wxCB_READONLY);
    m_type_combo->SetFont(Label::Body_13);

    m_combo_item_types.clear();

    int default_sel = -1;

    // --- Group 1: Project Filament List (project filament types support MaterialList) ---
    m_type_combo->AppendGroupHeader(_L("Project Filament List"));
    m_combo_item_types.push_back(std::string());

    // Add all project filament types (including Hyper PLA)
    for (size_t i = 0; i < m_project_types.size(); ++i) {
        const std::string& type = m_project_types[i];
        int idx = m_type_combo->Append(wxString::FromUTF8(type));
        m_combo_item_types.push_back(type);
        if (type == m_preferred_type && default_sel < 0)
            default_sel = idx;
    }

    // --- Group 2: Standard Mode Recommendations (Hyper PLA supports CMYW/RYBW) ---
    m_type_combo->AppendGroupHeader(_L("Standard Mode Recommendations"));
    m_combo_item_types.push_back(std::string());

    // Always add Hyper PLA as standard mode option
    {
        int idx = m_type_combo->Append(wxString::FromUTF8(kDecomposeHyperPlaType));
        m_combo_item_types.push_back(kDecomposeHyperPlaType);
        if (m_preferred_type == kDecomposeHyperPlaType && default_sel < 0)
            default_sel = idx;
    }

    // Fallback: select first non-header item
    if (default_sel < 0) {
        for (int i = 0; i < static_cast<int>(m_combo_item_types.size()); ++i) {
            if (!m_combo_item_types[i].empty()) {
                default_sel = i;
                break;
            }
        }
    }

    if (default_sel >= 0) {
        m_type_combo->SetSelection(default_sel);
        m_last_valid_sel = default_sel;
    }

    m_type_combo->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent& evt) {
        evt.StopPropagation();
        int sel = m_type_combo->GetSelection();
        // Skip group headers (empty type string)
        if (sel < 0 || static_cast<size_t>(sel) >= m_combo_item_types.size()
            || m_combo_item_types[sel].empty()) {
            // Revert to last valid selection
            if (m_last_valid_sel >= 0 && m_last_valid_sel < static_cast<int>(m_combo_item_types.size())
                && !m_combo_item_types[m_last_valid_sel].empty()) {
                m_type_combo->SetSelection(m_last_valid_sel);
            }
            return;
        }

        m_preferred_type = m_combo_item_types[sel];
        m_last_valid_sel = sel;
        update_card_visibility();
        compute_decomposition();
        update_matched_color_display();
        update_ok_button_state();
    });

    sizer->Add(m_type_combo, 1, wxEXPAND);
    return sizer;
}

static wxPanel* create_color_swatch(wxWindow* parent, const wxColour& color, int size)
{
    auto* panel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(size, size));
    panel->SetBackgroundColour(color);
    panel->SetMinSize(wxSize(size, size));
    return panel;
}

wxBoxSizer* ColorDecomposeDialog::create_target_color_section()
{
    auto* sizer = new wxBoxSizer(wxHORIZONTAL);

    auto* label = new wxStaticText(this, wxID_ANY, _L("Target Color"));
    label->SetFont(Label::Head_14);
    label->SetForegroundColour(StateColor::darkModeColorFor(COLOR_TEXT_DARK));
    sizer->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(19));

    m_target_swatch = create_color_swatch(this, m_target_color, FromDIP(28));
    sizer->Add(m_target_swatch, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(12));

    m_target_rgb_text = new wxStaticText(this, wxID_ANY,
        wxString::Format(_L("RGB: %d, %d, %d"), m_target_color.Red(), m_target_color.Green(), m_target_color.Blue()));
    m_target_rgb_text->SetFont(Label::Body_13);
    m_target_rgb_text->SetForegroundColour(StateColor::darkModeColorFor(COLOR_TEXT_DARK));
    sizer->Add(m_target_rgb_text, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(12));

    auto* arrow_text = new wxStaticText(this, wxID_ANY, wxString::FromUTF8("\xe2\x86\x92"));
    arrow_text->SetForegroundColour(StateColor::darkModeColorFor(COLOR_TEXT_DARK));
    sizer->Add(arrow_text, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(12));

    m_matched_swatch = create_color_swatch(this, m_target_color, FromDIP(28));
    sizer->Add(m_matched_swatch, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(12));

    m_matched_rgb_text = new wxStaticText(this, wxID_ANY,
        wxString::Format(_L("RGB: %d, %d, %d"), m_target_color.Red(), m_target_color.Green(), m_target_color.Blue()));
    m_matched_rgb_text->SetFont(Label::Body_13);
    m_matched_rgb_text->SetForegroundColour(StateColor::darkModeColorFor(COLOR_TEXT_DARK));
    sizer->Add(m_matched_rgb_text, 1, wxALIGN_CENTER_VERTICAL);

    return sizer;
}

wxPanel* ColorDecomposeDialog::create_mode_card(wxWindow* parent, DecomposeMode mode,
                                                 const wxString& title)
{
    const int pad       = FromDIP(12);

    auto* card = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    card->SetBackgroundStyle(wxBG_STYLE_PAINT);

    auto* card_sizer = new wxBoxSizer(wxVERTICAL);

    auto* title_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto* title_label = new wxStaticText(card, wxID_ANY, title);
    title_label->SetFont(Label::Body_14);
    title_label->SetForegroundColour(StateColor::darkModeColorFor(wxColour("#909090")));
    match_parent_bg(title_label, StateColor::darkModeColorFor(COLOR_BG_CARD));
    title_sizer->Add(title_label, 1, wxALIGN_CENTER_VERTICAL);

    auto* chk = new ::CheckBox(card);
    chk->SetValue(mode == m_selected_mode);
    match_parent_bg(chk, StateColor::darkModeColorFor(COLOR_BG_CARD));
    switch (mode) {
    case DecomposeMode::MaterialList: m_chk_material_list = chk; break;
    case DecomposeMode::CMYW:         m_chk_cmyw = chk; break;
    case DecomposeMode::RYBW:         m_chk_rybw = chk; break;
    }
    chk->Bind(wxEVT_TOGGLEBUTTON, [this, mode](wxCommandEvent& e) {
        select_mode(mode);
        e.Skip();
    });
    title_sizer->Add(chk, 0, wxALIGN_CENTER_VERTICAL);

    card_sizer->Add(title_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, pad);

    card_sizer->Add(create_h_divider(card), 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(8));

    auto* colors_sizer = new wxBoxSizer(wxHORIZONTAL);
    card_sizer->Add(colors_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, pad);

    auto& controls = m_mode_cards[mode_index(mode)];
    controls.card = card;
    controls.components_sizer = colors_sizer;

    card->SetSizer(card_sizer);
    card->SetMinSize(wxSize(FromDIP(140), FromDIP(122)));
    card->SetMaxSize(wxSize(FromDIP(140), FromDIP(122)));

    card->Bind(wxEVT_PAINT, [this, card, mode](wxPaintEvent&) {
        wxBufferedPaintDC dc(card);
        wxSize sz = card->GetClientSize();
        dc.SetBackground(wxBrush(StateColor::darkModeColorFor(*wxWHITE)));
        dc.Clear();

        bool selected = (m_selected_mode == mode);
        wxColour border_col = selected
            ? StateColor::darkModeColorFor(COLOR_BRAND)
            : StateColor::darkModeColorFor(COLOR_BORDER_NORMAL);
        const int border_width = FromDIP(selected ? 2 : 1);
        const double inset = border_width / 2.0;
        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (gc) {
            gc->SetPen(wxPen(border_col, border_width));
            gc->SetBrush(wxBrush(StateColor::darkModeColorFor(COLOR_BG_CARD)));
            gc->DrawRoundedRectangle(inset, inset, sz.x - 2 * inset, sz.y - 2 * inset, FromDIP(8));
        } else {
            const int fallback_inset = (border_width + 1) / 2;
            dc.SetPen(wxPen(border_col, border_width));
            dc.SetBrush(wxBrush(StateColor::darkModeColorFor(COLOR_BG_CARD)));
            dc.DrawRoundedRectangle(fallback_inset, fallback_inset, sz.x - 2 * fallback_inset, sz.y - 2 * fallback_inset, FromDIP(8));
        }
    });

    std::function<void(wxWindow*)> bind_click;
    bind_click = [this, mode, chk, &bind_click](wxWindow* w) {
        if (w == chk || dynamic_cast<::CheckBox*>(w))
            return;
        w->Bind(wxEVT_LEFT_UP, [this, mode](wxMouseEvent&) {
            select_mode(mode);
        });
        w->SetCursor(wxCursor(wxCURSOR_HAND));
        for (auto* child : w->GetChildren())
            bind_click(child);
    };
    bind_click(card);

    return card;
}

wxBoxSizer* ColorDecomposeDialog::create_mode_selection_section()
{
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* section_label = new wxStaticText(this, wxID_ANY, _L("Select Color Decomposition"));
    section_label->SetFont(Label::Head_14);
    section_label->SetForegroundColour(StateColor::darkModeColorFor(COLOR_TEXT_DARK));
    sizer->Add(section_label, 0, wxBOTTOM, FromDIP(4));

    auto* modes_sizer = new wxBoxSizer(wxHORIZONTAL);

    // --- Arbitrary mode column (wrapped in a panel so the whole column hides together) ---
    m_arb_column_panel = new wxPanel(this, wxID_ANY);
    m_arb_column_panel->SetBackgroundColour(StateColor::darkModeColorFor(*wxWHITE));
    auto* arb_col = new wxBoxSizer(wxVERTICAL);
    {
        auto* arb_header_sizer = new wxBoxSizer(wxHORIZONTAL);
        arb_header_sizer->Add(create_mode_group_label(m_arb_column_panel, _L("Arbitrary Mode")),
                              0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(5));
        arb_header_sizer->Add(create_h_divider(m_arb_column_panel, FromDIP(88)), 0, wxALIGN_CENTER_VERTICAL);
        arb_col->Add(arb_header_sizer, 0, wxEXPAND | wxBOTTOM, FromDIP(8));

        m_card_material_list = create_mode_card(m_arb_column_panel, DecomposeMode::MaterialList,
            _L("Material List"));
        arb_col->Add(m_card_material_list, 0, wxEXPAND);
    }
    m_arb_column_panel->SetSizer(arb_col);
    modes_sizer->Add(m_arb_column_panel, 0, wxEXPAND | wxRIGHT, FromDIP(16));

    // --- Standard mode column ---
    auto* std_col = new wxBoxSizer(wxVERTICAL);
    {
        auto* std_header_sizer = new wxBoxSizer(wxHORIZONTAL);
        std_header_sizer->Add(create_mode_group_label(this, _L("Standard Mode")),
                              0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(5));
        std_header_sizer->Add(create_h_divider(this), 1, wxALIGN_CENTER_VERTICAL);
        std_col->Add(std_header_sizer, 0, wxEXPAND | wxBOTTOM, FromDIP(8));

        auto* cards_sizer = new wxBoxSizer(wxHORIZONTAL);

        m_card_cmyw = create_mode_card(this, DecomposeMode::CMYW, _L("CMYW"));
        cards_sizer->Add(m_card_cmyw, 0, wxRIGHT, FromDIP(12));

        m_card_rybw = create_mode_card(this, DecomposeMode::RYBW, _L("RYBW"));
        cards_sizer->Add(m_card_rybw, 0);

        std_col->Add(cards_sizer, 0, wxEXPAND);
    }
    modes_sizer->Add(std_col, 0, wxEXPAND);

    sizer->Add(modes_sizer, 0, wxEXPAND);

    m_no_card_hint = new wxStaticText(this, wxID_ANY,
        _L("At least two filaments of the same material type are required for decomposition"));
    m_no_card_hint->SetFont(Label::Body_13);
    m_no_card_hint->SetForegroundColour(StateColor::darkModeColorFor(wxColour("#909090")));
    m_no_card_hint->Wrap(FromDIP(400));
    m_no_card_hint->Hide();
    sizer->Add(m_no_card_hint, 0, wxTOP, FromDIP(8));

    m_limit_warning_panel = new wxPanel(this, wxID_ANY);
    m_limit_warning_panel->SetBackgroundColour(StateColor::darkModeColorFor(*wxWHITE));
    auto* warning_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto* warn_bmp = new wxStaticBitmap(m_limit_warning_panel, wxID_ANY,
        create_scaled_bitmap("obj_warning", m_limit_warning_panel, 16),
        wxDefaultPosition, wxSize(FromDIP(16), FromDIP(16)));
    m_limit_warning_text = new wxStaticText(m_limit_warning_panel, wxID_ANY, wxEmptyString);
    m_limit_warning_text->SetFont(Label::Body_13);
    m_limit_warning_text->SetForegroundColour(StateColor::darkModeColorFor(wxColour("#D32F2F")));
    m_limit_warning_text->Wrap(FromDIP(400));
    warning_sizer->Add(warn_bmp, 0, wxALIGN_TOP | wxRIGHT, FromDIP(6));
    warning_sizer->Add(m_limit_warning_text, 1, wxEXPAND);
    m_limit_warning_panel->SetSizer(warning_sizer);
    m_limit_warning_panel->Hide();
    sizer->Add(m_limit_warning_panel, 0, wxEXPAND | wxTOP, FromDIP(8));

    return sizer;
}

wxBoxSizer* ColorDecomposeDialog::create_button_panel()
{
    auto* sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->AddStretchSpacer();

    m_btn_cancel = new Button(this, _L("Cancel"));
    m_btn_cancel->SetBackgroundColor(StateColor::darkModeColorFor(*wxWHITE));
    m_btn_cancel->SetBorderColor(StateColor::darkModeColorFor(wxColour("#CECECE")));
    m_btn_cancel->SetTextColor(StateColor::darkModeColorFor(wxColour("#262E30")));
    m_btn_cancel->SetMinSize(wxSize(FromDIP(55), FromDIP(24)));
    m_btn_cancel->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CANCEL); });

    m_btn_ok = new Button(this, _L("OK"));
    m_btn_ok->SetBackgroundColor(StateColor(
        std::make_pair(wxColour("#C2C2C2"), (int) StateColor::Disabled),
        std::make_pair(wxColour("#00AE42"), (int) StateColor::Normal)));
    m_btn_ok->SetBorderColor(StateColor(
        std::make_pair(wxColour("#C2C2C2"), (int) StateColor::Disabled),
        std::make_pair(wxColour("#00AE42"), (int) StateColor::Normal)));
    m_btn_ok->SetTextColor(StateColor(
        std::make_pair(*wxWHITE, (int) StateColor::Disabled),
        std::make_pair(*wxWHITE, (int) StateColor::Normal)));
    m_btn_ok->SetMinSize(wxSize(FromDIP(55), FromDIP(24)));
    m_btn_ok->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        EndModal(wxID_OK);
    });

    sizer->Add(m_btn_cancel, 0, wxRIGHT, FromDIP(12));
    sizer->Add(m_btn_ok, 0);

    return sizer;
}

void ColorDecomposeDialog::select_mode(DecomposeMode mode)
{
    m_selected_mode = mode;
    m_result = m_mode_results[mode_index(mode)];
    update_card_styles();
    update_matched_color_display();
    update_ok_button_state();
}

void ColorDecomposeDialog::update_card_styles()
{
    if (m_card_material_list) m_card_material_list->Refresh();
    if (m_card_cmyw)          m_card_cmyw->Refresh();
    if (m_card_rybw)          m_card_rybw->Refresh();

    if (m_chk_material_list)
        m_chk_material_list->SetValue(m_selected_mode == DecomposeMode::MaterialList);
    if (m_chk_cmyw)
        m_chk_cmyw->SetValue(m_selected_mode == DecomposeMode::CMYW);
    if (m_chk_rybw)
        m_chk_rybw->SetValue(m_selected_mode == DecomposeMode::RYBW);
}

void ColorDecomposeDialog::update_card_visibility()
{
    // Count physical filaments of the same type (excluding the source filament)
    int same_type_count = 0;
    for (size_t i = 0; i < m_filament_types.size(); ++i) {
        if (static_cast<int>(i) == m_filament_idx)
            continue;
        if (material_type_matches(m_filament_types[i], m_preferred_type))
            ++same_type_count;
    }

    // MaterialList: requires at least 2 same-type filaments (excluding source)
    bool show_arb = (same_type_count >= 2);

    // CMYW/RYBW: only available when preferred type is Hyper PLA
    const bool is_hyper_pla = (m_preferred_type == kDecomposeHyperPlaType);
    bool show_cmyw = is_hyper_pla;
    bool show_rybw = is_hyper_pla;

    if (m_arb_column_panel)   m_arb_column_panel->Show(show_arb);
    if (m_card_material_list) m_card_material_list->Show(show_arb);
    if (m_card_cmyw)          m_card_cmyw->Show(show_cmyw);
    if (m_card_rybw)          m_card_rybw->Show(show_rybw);

    bool any_visible = show_arb || show_cmyw || show_rybw;
    if (m_no_card_hint)
        m_no_card_hint->Show(!any_visible);

    // Auto-select a visible mode when current selection becomes hidden
    if (any_visible) {
        bool cur_visible = false;
        if (m_selected_mode == DecomposeMode::MaterialList && show_arb)  cur_visible = true;
        if (m_selected_mode == DecomposeMode::CMYW && show_cmyw)         cur_visible = true;
        if (m_selected_mode == DecomposeMode::RYBW && show_rybw)         cur_visible = true;
        if (!cur_visible) {
            if (show_arb)       select_mode(DecomposeMode::MaterialList);
            else if (show_cmyw) select_mode(DecomposeMode::CMYW);
            else                select_mode(DecomposeMode::RYBW);
        }
    }

    Layout();
    update_ok_button_state();
}

void ColorDecomposeDialog::update_filament_limit_warning()
{
    if (!m_limit_warning_panel || !m_limit_warning_text)
        return;

    size_t missing_new = 0;
    if (m_missing_calculator) {
        missing_new = m_missing_calculator(m_result);
    } else {
        const size_t source_physical_idx = m_filament_idx >= 0 ? static_cast<size_t>(m_filament_idx) : size_t(-1);
        const std::vector<size_t>* indices =
            m_physical_config_indices.empty() ? nullptr : &m_physical_config_indices;
        missing_new = count_decompose_new_physical_filaments(
            m_result, m_physical_colors, m_filament_types, source_physical_idx, indices);
    }
    // CMYW/RYBW 单色 (components.size() == 1) 不会创建 mixed filament，
    // 但仍可能需要新增 1 个 base color 物理耗材。RYBW 同理。计算 needed 时
    // 需考虑“新增物理耗材”而不只是“创建混色槽位”。
    const bool creates_mixed = m_result.components.size() >= 2;
    const bool creates_new_physical = missing_new > 0;
    // +1 仅为“创建 mixed filament”预留槽位；纯单色场景不创建混色则不加。
    const size_t needed = m_current_filament_count + missing_new + (creates_mixed ? 1 : 0);
    const bool blocked = (creates_mixed || creates_new_physical) && needed > m_max_filament_count;

    const bool was_shown = m_limit_warning_panel->IsShown();

    if (!blocked) {
        if (was_shown) {
            m_limit_warning_panel->Hide();
            Layout();
            Fit();
            CenterOnParent();
        }
        return;
    }

    wxString mode_name;
    switch (m_selected_mode) {
    case DecomposeMode::CMYW:         mode_name = _L("CMYW"); break;
    case DecomposeMode::RYBW:         mode_name = _L("RYBW"); break;
    case DecomposeMode::MaterialList: mode_name = _L("Material List"); break;
    }

    const wxString warning_text = format_wxstr(
        _L("The material list supports at most %1% colors. After %2% decomposition, the material count would exceed %1%. Please delete unused filaments on the main screen before decomposing."),
        m_max_filament_count, mode_name);

    // Show first so the panel is laid out and the text control gets its real
    // width, then wrap to that width so the paragraph fills the content area.
    m_limit_warning_panel->Show();
    Layout();
    const int avail = m_limit_warning_text->GetClientSize().x;
    m_limit_warning_text->SetLabel(warning_text);
    if (avail > FromDIP(50))
        m_limit_warning_text->Wrap(avail);

    Layout();
    // Only resize/recenter when the warning panel actually toggled from hidden
    // to shown. While already visible, switching modes must not re-Fit/recenter
    // the dialog, which would make it jump on every card switch.
    if (!was_shown) {
        Fit();
        CenterOnParent();
    }
}

void ColorDecomposeDialog::set_missing_physical_calculator(std::function<size_t(const ColorDecomposeResult&)> fn)
{
    m_missing_calculator = std::move(fn);
    update_ok_button_state();
}

void ColorDecomposeDialog::update_ok_button_state()
{
    if (!m_btn_ok) return;
    update_filament_limit_warning();
    bool any_card_visible = (m_card_material_list && m_card_material_list->IsShown())
                         || (m_card_cmyw && m_card_cmyw->IsShown())
                         || (m_card_rybw && m_card_rybw->IsShown());
    const bool blocked = m_limit_warning_panel && m_limit_warning_panel->IsShown();
    m_btn_ok->Enable(any_card_visible && !blocked);
    Layout();
}

void ColorDecomposeDialog::update_mode_card_content(DecomposeMode mode)
{
    auto& controls = m_mode_cards[mode_index(mode)];
    auto* sizer = controls.components_sizer;
    auto* card = controls.card;
    if (!sizer || !card)
        return;

    sizer->Clear(true);
    const auto& components = m_mode_results[mode_index(mode)].components;
    const size_t count = components.size();
    if (count == 0) {
        card->Layout();
        card->Refresh();
        return;
    }

    const int swatch_sz = FromDIP(24);
    const int plus_gap  = FromDIP(24);
    const wxFont& ratio_font = Label::Body_13;
    auto bind_select = [this, mode](wxWindow* w) {
        w->Bind(wxEVT_LEFT_UP, [this, mode](wxMouseEvent&) {
            select_mode(mode);
        });
        w->SetCursor(wxCursor(wxCURSOR_HAND));
    };

    for (size_t i = 0; i < count; ++i) {
        auto* col = new wxBoxSizer(wxVERTICAL);
        auto* swatch = create_color_swatch(card, components[i].colour, swatch_sz);
        bind_select(swatch);
        col->Add(swatch, 0, wxALIGN_CENTER_HORIZONTAL);
        auto* ratio_text = new wxStaticText(card, wxID_ANY, wxString::Format("%d%%", components[i].ratio));
        ratio_text->SetFont(ratio_font);
        ratio_text->SetForegroundColour(StateColor::darkModeColorFor(COLOR_TEXT_DARK));
        match_parent_bg(ratio_text, StateColor::darkModeColorFor(COLOR_BG_CARD));
        bind_select(ratio_text);
        col->Add(ratio_text, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, FromDIP(4));
        sizer->Add(col, 0, wxALIGN_TOP);

        if (i + 1 < count) {
            sizer->AddStretchSpacer();
            auto* plus_panel = new wxPanel(card, wxID_ANY, wxDefaultPosition, wxSize(plus_gap, swatch_sz));
            plus_panel->SetMinSize(wxSize(plus_gap, swatch_sz));
            plus_panel->SetMaxSize(wxSize(plus_gap, swatch_sz));
            plus_panel->SetBackgroundColour(StateColor::darkModeColorFor(COLOR_BG_CARD));
            auto* plus_sizer = new wxBoxSizer(wxVERTICAL);
            auto* plus_label = new wxStaticText(plus_panel, wxID_ANY, "+");
            plus_label->SetFont(Label::Body_13);
            plus_label->SetForegroundColour(StateColor::darkModeColorFor(COLOR_TEXT_DARK));
            match_parent_bg(plus_label, StateColor::darkModeColorFor(COLOR_BG_CARD));
            bind_select(plus_panel);
            bind_select(plus_label);
            plus_sizer->AddStretchSpacer();
            plus_sizer->Add(plus_label, 0, wxALIGN_CENTER_HORIZONTAL);
            plus_sizer->AddStretchSpacer();
            plus_panel->SetSizer(plus_sizer);
            sizer->Add(plus_panel, 0, wxALIGN_TOP);
            sizer->AddStretchSpacer();
        }
    }

    // Size the card so it tightly fits its content:
    //  - width: enough for swatches + plus + "NN%" labels (no extra padding)
    //  - height: enough for title + swatch + ratio label (no extra padding)
    // Both dimensions are bound by an upper limit to prevent the card from
    // stretching with the parent sizer.
    const int card_min_width  = FromDIP(140 + (count > 2 ? static_cast<int>(count - 2) * 36 : 0));
    const int card_max_width  = card_min_width;
    const int card_min_height = FromDIP(122);
    const int card_max_height = card_min_height;
    card->SetMinSize(wxSize(card_min_width,  card_min_height));
    card->SetMaxSize(wxSize(card_max_width,  card_max_height));

    card->Layout();
    card->Refresh();
}

void ColorDecomposeDialog::update_mode_card_contents()
{
    update_mode_card_content(DecomposeMode::MaterialList);
    update_mode_card_content(DecomposeMode::CMYW);
    update_mode_card_content(DecomposeMode::RYBW);
    Layout();
    Fit();
}

void ColorDecomposeDialog::update_matched_color_display()
{
    if (!m_result.matched_color.IsOk())
        m_result.matched_color = m_target_color;

    if (m_matched_swatch) {
        m_matched_swatch->SetBackgroundColour(m_result.matched_color);
        m_matched_swatch->Refresh();
    }
    if (m_matched_rgb_text) {
        m_matched_rgb_text->SetLabel(wxString::Format(_L("RGB: %d, %d, %d"),
            m_result.matched_color.Red(), m_result.matched_color.Green(), m_result.matched_color.Blue()));
    }
}

bool ColorDecomposeDialog::try_build_single_base_result(DecomposeMode mode, ColorDecomposeResult& out) const
{
    if (mode != DecomposeMode::CMYW && mode != DecomposeMode::RYBW)
        return false;

    static const DecomposeBaseColor cmyw_bases[] = {
        DecomposeBaseColor::Cyan, DecomposeBaseColor::Magenta,
        DecomposeBaseColor::Yellow, DecomposeBaseColor::White
    };
    static const DecomposeBaseColor rybw_bases[] = {
        DecomposeBaseColor::Red, DecomposeBaseColor::Yellow,
        DecomposeBaseColor::Blue, DecomposeBaseColor::White
    };
    const DecomposeBaseColor* bases = (mode == DecomposeMode::CMYW) ? cmyw_bases : rybw_bases;
    const size_t base_count = (mode == DecomposeMode::CMYW)
        ? sizeof(cmyw_bases) / sizeof(cmyw_bases[0])
        : sizeof(rybw_bases) / sizeof(rybw_bases[0]);

    const std::string target_hex = decompose_normalize_color_hex(
        m_target_color.GetAsString(wxC2S_HTML_SYNTAX).ToStdString());

    for (size_t i = 0; i < base_count; ++i) {
        const DecomposeBaseColor base = bases[i];
        DecomposeOfficialComponent official =
            lookup_decompose_official_component(m_preferred_type, mode, base, pure_color_for_base(base));
        if (decompose_normalize_color_hex(official.color_hex) != target_hex)
            continue;

        out = ColorDecomposeResult{};
        out.mode = mode;
        out.matched_color = hex_to_wx_colour(official.color_hex, m_target_color);
        DecomposeComponent comp;
        comp.colour = out.matched_color;
        comp.ratio = 100;
        comp.filament_index = -1;
        comp.base_color = base;
        out.components.push_back(comp);
        return true;
    }
    return false;
}

namespace {

// ---------------------------------------------------------------------------
// CIE LAB ΔE76 距离评分（MaterialList 模式多项式推荐）。
// 参考 BambuStudio: src/libslic3r/ColorDecomposeRecipe.cpp rgb_to_lab / delta_e76
// sRGB (D65) → linear RGB → XYZ (D65) → CIE LAB → ΔE76 欧氏距离
// ---------------------------------------------------------------------------
struct LabColor {
    double l{0.0};
    double a{0.0};
    double b{0.0};
};

static double srgb_to_linear(int v)
{
    const double x = static_cast<double>(v) / 255.0;
    return x <= 0.04045 ? x / 12.92 : std::pow((x + 0.055) / 1.055, 2.4);
}

static double xyz_to_lab_component(double v)
{
    constexpr double eps    = 216.0 / 24389.0;
    constexpr double kappa  = 24389.0 / 27.0;
    return v > eps ? std::cbrt(v) : (kappa * v + 16.0) / 116.0;
}

static LabColor rgb_to_lab(int r, int g, int b)
{
    const double lr = srgb_to_linear(r);
    const double lg = srgb_to_linear(g);
    const double lb = srgb_to_linear(b);
    const double x  = (0.4124564 * lr + 0.3575761 * lg + 0.1804375 * lb) / 0.95047;
    const double y  =  0.2126729 * lr + 0.7151522 * lg + 0.0721750 * lb;
    const double z  = (0.0193339 * lr + 0.1191920 * lg + 0.9503041 * lb) / 1.08883;
    const double fx = xyz_to_lab_component(x);
    const double fy = xyz_to_lab_component(y);
    const double fz = xyz_to_lab_component(z);
    return {116.0 * fy - 16.0, 500.0 * (fx - fy), 200.0 * (fy - fz)};
}

static double delta_e76(const LabColor& a, const LabColor& b)
{
    const double dl = a.l - b.l;
    const double da = a.a - b.a;
    const double db = a.b - b.b;
    return std::sqrt(dl * dl + da * da + db * db);
}

// 把 wxColour 序列化为 "#RRGGBB"，供 blend_color / blend_color_multi 使用。
inline std::string wx_colour_to_mixer_hex(const wxColour& c)
{
    char buf[8];
    std::snprintf(buf, sizeof(buf), "#%02X%02X%02X",
                  static_cast<unsigned>(c.Red()),
                  static_cast<unsigned>(c.Green()),
                  static_cast<unsigned>(c.Blue()));
    return std::string(buf);
}

struct MaterialListRecommendation {
    bool                            valid{false};
    wxColour                        matched_color;
    std::vector<DecomposeComponent> components;
};

// 比例网格（BambuStudio 风格，2/3 色枚举，约束每色 ≥ 20%）：
//   2-color: a ∈ [20, 80] 步长 5（13 采样，对称 b = 100 - a）
//   3-color: a ∈ [20, 60] 步长 5，b ∈ [20, 80 - a] 步长 5，c = 100 - a - b ≥ 20
// 1-color 不显式枚举，由 8 候选截断隐含保留（最近的单色 = baseline）。
static std::vector<std::vector<int>> ratio_grid(size_t n)
{
    std::vector<std::vector<int>> out;
    if (n == 2) {
        for (int a = 20; a <= 80; a += 5)
            out.push_back({a, 100 - a});
    } else if (n == 3) {
        for (int a = 20; a <= 60; a += 5)
            for (int b = 20; b <= 80 - a; b += 5) {
                const int c = 100 - a - b;
                if (c >= 20)
                    out.push_back({a, b, c});
            }
    }
    return out;
}

// 评估一个候选组合：polynomial mixer 计算混合色，ΔE76 评分。
// source_physical_idx: 0-based index of the source filament in the FULL
// physical list.  The `colors` array has this filament removed, so indices
// into `colors` must be offset by +1 when >= source_physical_idx to map
// back to 1-based indices in the full list.
static void evaluate_material_list_candidate(
    const LabColor& target_lab,
    const std::vector<int>& indices,
    const std::vector<int>& weights,
    const std::vector<wxColour>& colors,
    int source_physical_idx,
    double& best_score,
    MaterialListRecommendation& best)
{
    std::vector<std::string> hexes;
    hexes.reserve(indices.size());
    for (int idx : indices)
        hexes.push_back(wx_colour_to_mixer_hex(colors[idx]));

    const std::string mh = blend_color_multi(hexes, weights);
    wxColour m(mh);
    if (!m.IsOk())
        return;

    const double score = delta_e76(target_lab, rgb_to_lab(m.Red(), m.Green(), m.Blue()));
    if (score >= best_score)
        return;

    best_score         = score;
    best.valid         = true;
    best.matched_color = m;
    best.components.clear();
    best.components.reserve(indices.size());
    for (size_t i = 0; i < indices.size(); ++i) {
        DecomposeComponent c;
        c.colour         = colors[indices[i]];
        c.ratio          = weights[i];
        // Map filtered-list index to 1-based index in the FULL physical list.
        // The filtered list removes the source at source_physical_idx, so
        // any index >= source_physical_idx must shift +1.
        int full_0based = indices[i];
        if (source_physical_idx >= 0 && full_0based >= source_physical_idx)
            full_0based += 1;
        c.filament_index = full_0based + 1;  // 1-based into full physical list
        c.base_color     = DecomposeBaseColor::None;
        best.components.push_back(c);
    }
}

// MaterialList 模式推荐（CIE LAB ΔE76 + polynomial mixer，与 BambuStudio
// 保持一致）：
//   - 同类型过滤：只保留与 preferred_type 匹配的耗材（Hyper PLA 和 Generic PLA
//     同属 PLA 类型）
//   - 8 候选上限：按耗材到目标色的 ΔE76 排序截断，避免 C(n,3) 组合爆炸
//   - 2-color: ratio_grid(2) — 13 个比例采样
//   - 3-color: ratio_grid(3) — 45 个比例采样
//   - 1-color: 由 8 截断隐含保留（最近单色作为 baseline）
// colors: 已排除源耗材的可用耗材颜色列表（0-based）。
// source_physical_idx: 源耗材在完整 m_physical_colors 中的 0-based 索引，
//   用于将 filtered-list 索引映射回完整列表的 1-based filament_index。
static MaterialListRecommendation recommend_material_list_polynomial(
    const wxColour& target,
    const std::vector<wxColour>& colors,
    const std::vector<std::string>& types,
    const std::string& preferred_type,
    int source_physical_idx)
{
    MaterialListRecommendation out;
    if (colors.empty()) {
        out.matched_color = target;
        DecomposeComponent c;
        c.colour         = target;
        c.ratio          = 100;
        c.filament_index = -1;
        out.components.push_back(c);
        return out;
    }

    // 同类型过滤：只保留与 preferred_type 匹配的耗材
    std::vector<wxColour> filtered_colors;
    std::vector<size_t> original_indices;
    for (size_t i = 0; i < colors.size(); ++i) {
        const std::string t = (i < types.size()) ? types[i] : std::string();
        if (!preferred_type.empty() && !t.empty() && !material_type_matches(t, preferred_type))
            continue;
        filtered_colors.push_back(colors[i]);
        original_indices.push_back(i);
    }

    if (filtered_colors.empty()) {
        // 没有同类型耗材，回退到目标色
        out.matched_color = target;
        DecomposeComponent c;
        c.colour         = target;
        c.ratio          = 100;
        c.filament_index = -1;
        out.components.push_back(c);
        return out;
    }

    const LabColor target_lab = rgb_to_lab(target.Red(), target.Green(), target.Blue());

    // 8 候选上限：按 ΔE76 距离排序截断
    constexpr size_t kMaxCandidates = 8;
    std::vector<size_t> order(filtered_colors.size());
    std::iota(order.begin(), order.end(), 0);
    if (order.size() > kMaxCandidates) {
        std::vector<double> dists(order.size());
        for (size_t i = 0; i < order.size(); ++i) {
            dists[i] = delta_e76(target_lab,
                                rgb_to_lab(filtered_colors[i].Red(), filtered_colors[i].Green(), filtered_colors[i].Blue()));
        }
        std::partial_sort(order.begin(), order.begin() + kMaxCandidates, order.end(),
            [&dists](size_t a, size_t b) { return dists[a] < dists[b]; });
        order.resize(kMaxCandidates);
    }

    double best_score = std::numeric_limits<double>::max();
    const auto grid2  = ratio_grid(2);
    const auto grid3  = ratio_grid(3);
    for (size_t a = 0; a < order.size(); ++a) {
        for (size_t b = a + 1; b < order.size(); ++b) {
            for (const auto& w : grid2) {
                const std::vector<int> idx{static_cast<int>(original_indices[order[a]]), static_cast<int>(original_indices[order[b]])};
                evaluate_material_list_candidate(target_lab, idx, w, colors, source_physical_idx, best_score, out);
            }
            for (size_t c = b + 1; c < order.size(); ++c) {
                for (const auto& w : grid3) {
                    const std::vector<int> idx{static_cast<int>(original_indices[order[a]]), static_cast<int>(original_indices[order[b]]), static_cast<int>(original_indices[order[c]])};
                    evaluate_material_list_candidate(target_lab, idx, w, colors, source_physical_idx, best_score, out);
                }
            }
        }
    }

    if (!out.valid) {
        // 极端情况（n < 2 或混合色全部失败），回退到最近单色
        const size_t best_idx = original_indices[order.front()];
        out.matched_color = colors[best_idx];
        DecomposeComponent c;
        c.colour         = out.matched_color;
        c.ratio          = 100;
        c.filament_index = static_cast<int>(best_idx) + 1;
        c.base_color     = DecomposeBaseColor::None;
        out.components.push_back(c);
        out.valid         = true;
    }
    return out;
}

} // namespace

void ColorDecomposeDialog::compute_decomposition()
{
    auto fallback_result = [this](DecomposeMode mode, const std::vector<DecomposeComponent>& components) {
        ColorDecomposeResult result;
        result.mode = mode;
        result.components = components;
        int total = 0;
        double r = 0.0, g = 0.0, b = 0.0;
        for (const auto& comp : result.components)
            total += comp.ratio;
        if (total <= 0)
            total = 100;
        for (const auto& comp : result.components) {
            const double w = static_cast<double>(comp.ratio) / total;
            r += comp.colour.Red() * w;
            g += comp.colour.Green() * w;
            b += comp.colour.Blue() * w;
        }
        result.matched_color = result.components.empty()
            ? m_target_color
            : wxColour(static_cast<unsigned char>(std::clamp(r, 0.0, 255.0)),
                       static_cast<unsigned char>(std::clamp(g, 0.0, 255.0)),
                       static_cast<unsigned char>(std::clamp(b, 0.0, 255.0)));
        return result;
    };

    std::vector<ColorDecomposePhysicalFilament> physical_filaments;
    physical_filaments.reserve(m_physical_colors.size());
    for (size_t i = 0; i < m_physical_colors.size(); ++i) {
        if (m_filament_idx >= 0 && i == static_cast<size_t>(m_filament_idx))
            continue;
        ColorDecomposePhysicalFilament filament;
        filament.color_hex = m_physical_colors[i];
        filament.name = i < m_filament_names.size() ? m_filament_names[i] : "";
        filament.type = i < m_filament_types.size() ? m_filament_types[i] : "";
        filament.filament_index = static_cast<unsigned int>(i + 1);
        physical_filaments.push_back(std::move(filament));
    }

    const ColorDecomposeRgb target_rgb = wx_colour_to_recipe_rgb(m_target_color);

    // MaterialList 模式走 sRGB 域多项式拟合 + CIE LAB ΔE76 距离评分
    // （与 BambuStudio MaterialList 模式保持一致：
    //  src/libslic3r/ColorDecomposeRecipe.cpp::recommend_from_physical_filaments）。
    // 这里不再调用 K-M 引擎 `recommend_from_physical_filaments`：K-M 是物理
    // 光谱反色模型，对 MaterialList 用户列表的耗材色来说与展示色块不一致，
    // 会出现 swatch（耗材原始色）和 matched_color（K-M 推算的混合色）相互
    // 矛盾的视觉错位。多项式拟合直接基于 hex 颜色，混合结果就是色块混合，
    // swatch、components.colour、matched_color 三者天然一致。
    std::vector<wxColour> available_colors;
    std::vector<std::string> available_types;
    available_colors.reserve(physical_filaments.size());
    available_types.reserve(physical_filaments.size());
    for (const auto& f : physical_filaments) {
        available_colors.emplace_back(f.color_hex);
        available_types.push_back(f.type);
    }
    auto material_recommendation = recommend_material_list_polynomial(
        m_target_color, available_colors, available_types, m_preferred_type, m_filament_idx);
    ColorDecomposeResult mat_result;
    mat_result.mode = DecomposeMode::MaterialList;
    mat_result.matched_color = material_recommendation.matched_color;
    mat_result.components = std::move(material_recommendation.components);
    m_mode_results[mode_index(DecomposeMode::MaterialList)] = std::move(mat_result);

    ColorDecomposeResult single_base;
    if (try_build_single_base_result(DecomposeMode::CMYW, single_base)) {
        m_mode_results[mode_index(DecomposeMode::CMYW)] = single_base;
    } else {
        auto cmyw_recipe = lookup_standard_recipe(target_rgb, ColorDecomposeRecipeMode::CMYW, m_preferred_type);
        // Default CMYW (Bambu style): Magenta + Yellow + White.
        // White is added as a "tint" to simulate CMY ink under-printing on paper.
        if (cmyw_recipe.valid) {
        } else {
        }
        m_mode_results[mode_index(DecomposeMode::CMYW)] = cmyw_recipe.valid
            ? to_dialog_result(cmyw_recipe, m_target_color)
            : fallback_result(DecomposeMode::CMYW, {
                {CMYW_MAGENTA, 20, -1, DecomposeBaseColor::Magenta},
                {CMYW_YELLOW,  60, -1, DecomposeBaseColor::Yellow},
                {CMYW_WHITE,   20, -1, DecomposeBaseColor::White}
            });
    }

    if (try_build_single_base_result(DecomposeMode::RYBW, single_base)) {
        m_mode_results[mode_index(DecomposeMode::RYBW)] = single_base;
    } else {
        auto rybw_recipe = lookup_standard_recipe(target_rgb, ColorDecomposeRecipeMode::RYBW, m_preferred_type);
        // Default RYBW (Bambu style): Red + Yellow, warm two-color mix.
        if (rybw_recipe.valid) {
        } else {
        }
        m_mode_results[mode_index(DecomposeMode::RYBW)] = rybw_recipe.valid
            ? to_dialog_result(rybw_recipe, m_target_color)
            : fallback_result(DecomposeMode::RYBW, {
                {RYBW_RED,    30, -1, DecomposeBaseColor::Red},
                {RYBW_YELLOW, 70, -1, DecomposeBaseColor::Yellow}
            });
    }

    m_result = m_mode_results[mode_index(m_selected_mode)];
    update_mode_card_contents();
    update_ok_button_state();
}

} // namespace GUI
} // namespace Slic3r
