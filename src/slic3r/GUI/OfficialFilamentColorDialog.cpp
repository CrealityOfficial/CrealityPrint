#include "OfficialMaterialColorCache.hpp"
#include "OfficialFilamentColorDialog.hpp"

#include "GUI.hpp"
#include "GUI_App.hpp"
#include "I18N.hpp"
#include "Widgets/Button.hpp"
#include "Widgets/StateColor.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/Utils.hpp"
#include "slic3r/Utils/ColorSpaceConvert.hpp"

#include <boost/filesystem.hpp>
#include <boost/nowide/fstream.hpp>
#include <nlohmann/json.hpp>
#include <wx/bmpbuttn.h>
#include <wx/colordlg.h>
#include <wx/dcmemory.h>
#include <wx/display.h>
#include <wx/image.h>
#include <wx/scrolwin.h>
#include <wx/panel.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/utils.h>

#include <algorithm>
#include <climits>
#include <cmath>
#include <sstream>
#include <unordered_map>

namespace Slic3r { namespace GUI {

namespace FilamentColorAppearance {

wxColour parse(const std::string& value)
{
    unsigned long rgba = 0;
    if (value.size() == 9 && value.front() == '#' && wxString::FromUTF8(value.substr(1).c_str()).ToULong(&rgba, 16))
        return wxColour((rgba >> 24) & 255, (rgba >> 16) & 255, (rgba >> 8) & 255, rgba & 255);
    return wxColour(value);
}

bool equal(const wxColour& a, const wxColour& b)
{
    return a.IsOk() && b.IsOk() && a.Red() == b.Red() && a.Green() == b.Green() &&
        a.Blue() == b.Blue() && a.Alpha() == b.Alpha();
}

bool is_gradient(const nlohmann::json& item)
{
    const auto it = item.find("mixSpecialAttr");
    return it != item.end() && it->is_string() && it->get<std::string>() == "Gradient";
}

Appearance resolve(size_t slot, const wxColour& base)
{
    Appearance fallback{{base.IsOk() ? base : *wxBLACK}, false};
    if (!base.IsOk()) return fallback;
    const auto* bundle = wxGetApp().preset_bundle;
    if (!bundle || slot >= bundle->filament_presets.size()) return fallback;
    if (const auto* multi = bundle->project_config.option<ConfigOptionStrings>("filament_multi_colour");
        multi && slot < multi->values.size() && !multi->values[slot].empty()) {
        Appearance stored;
        std::istringstream tokens(multi->values[slot]);
        for (std::string token; tokens >> token;) {
            const auto color = parse(token);
            if (color.IsOk()) stored.colors.push_back(color);
        }
        if (!stored.colors.empty() && equal(stored.colors.front(), base)) {
            if (const auto* type = bundle->project_config.option<ConfigOptionStrings>("filament_colour_type");
                type && slot < type->values.size())
                stored.gradient = type->values[slot] == "0";
            return stored;
        }
    }
    return fallback;
}

wxColour pixel(const Appearance& appearance, int x, int y, int width, int tile)
{
    wxColour color = appearance.colors.front();
    if (appearance.colors.size() > 1) {
        if (appearance.gradient) {
            const int segments = static_cast<int>(appearance.colors.size()) - 1;
            const double position = width > 1 ? double(x) * segments / (width - 1) : 0;
            const int index = std::min(static_cast<int>(position), segments - 1);
            const auto& a = appearance.colors[index];
            const auto& b = appearance.colors[index + 1];
            const auto mix = [position, index](int a, int b) { return static_cast<unsigned char>(a + (b - a) * (position - index) + 0.5); };
            color = wxColour(mix(a.Red(), b.Red()), mix(a.Green(), b.Green()), mix(a.Blue(), b.Blue()), mix(a.Alpha(), b.Alpha()));
        } else {
            color = appearance.colors[std::min(size_t(x) * appearance.colors.size() / std::max(1, width), appearance.colors.size() - 1)];
        }
    }
    const int background = ((x / tile + y / tile) % 2) ? 180 : 220;
    const auto blend = [background, &color](int channel) {
        return static_cast<unsigned char>((channel * color.Alpha() + background * (255 - color.Alpha()) + 127) / 255);
    };
    return wxColour(blend(color.Red()), blend(color.Green()), blend(color.Blue()));
}

wxBitmap bitmap(const Appearance& appearance, int width, int height)
{
    wxImage image(width, height);
    auto* rgb = image.GetData();
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            const auto color = pixel(appearance, x, y, width, std::max(2, height / 4));
            *rgb++ = color.Red(); *rgb++ = color.Green(); *rgb++ = color.Blue();
        }
    return wxBitmap(image);
}

wxColour foreground(const Appearance& appearance)
{
    // Sample the whole badge so every UI surface chooses the same text colour.
    // A centre sample can land on a dark stop while most of the badge is light.
    double luminance = 0.0;
    for (int y = 1; y < 15; ++y)
        for (int x = 1; x < 47; ++x) {
            const wxColour color = pixel(appearance, x, y, 48, 4);
            luminance += 0.299 * color.Red() + 0.587 * color.Green() + 0.114 * color.Blue();
        }
    return luminance / (46 * 14) >= 145.0 ? *wxBLACK : *wxWHITE;
}

} // namespace FilamentColorAppearance


namespace {

constexpr int GRID_COLUMNS = 10;
constexpr int MAX_VISIBLE_ROWS = 4;

struct HsvColor { double hue; double saturation; double value; };

HsvColor to_hsv(const wxColour& color)
{
    const double r = color.Red() / 255.0;
    const double g = color.Green() / 255.0;
    const double b = color.Blue() / 255.0;
    const double maximum = std::max({r, g, b});
    const double minimum = std::min({r, g, b});
    const double delta = maximum - minimum;
    double hue = 0.0;
    if (delta > 0.00001) {
        if (maximum == r) hue = 60.0 * std::fmod((g - b) / delta, 6.0);
        else if (maximum == g) hue = 60.0 * ((b - r) / delta + 2.0);
        else hue = 60.0 * ((r - g) / delta + 4.0);
        if (hue < 0.0) hue += 360.0;
    }
    return {hue, maximum == 0.0 ? 0.0 : delta / maximum, maximum};
}

bool same_rgba(const wxColour& lhs, const wxColour& rhs)
{
    return lhs.IsOk() && rhs.IsOk() && lhs.Red() == rhs.Red() &&
           lhs.Green() == rhs.Green() && lhs.Blue() == rhs.Blue() && lhs.Alpha() == rhs.Alpha();
}

wxColour interpolate(const wxColour& from, const wxColour& to, double position)
{
    const auto channel = [position](unsigned char a, unsigned char b) {
        return static_cast<unsigned char>(a + (b - a) * position + 0.5);
    };
    return wxColour(channel(from.Red(), to.Red()), channel(from.Green(), to.Green()),
                    channel(from.Blue(), to.Blue()), channel(from.Alpha(), to.Alpha()));
}

} // namespace

OfficialFilamentColorDialog::OfficialFilamentColorDialog(wxWindow* parent,
                                                         const std::string& filament_id,
                                                         const wxColour& current_color,
                                                         const wxString& filament_name,
                                                         const std::vector<wxColour>& current_colors,
                                                         bool current_is_gradient)
    : DPIDialog(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE),
      m_selected_color(current_color),
      m_is_dark(wxGetApp().dark_mode()),
      m_outside_click_timer(this)
{
    load_entries(filament_id);
    if (!current_colors.empty()) {
        for (size_t index = 0; index < m_entries.size(); ++index) {
            const auto& entry = m_entries[index];
            if (entry.gradient != current_is_gradient || entry.colors.size() != current_colors.size()) continue;
            if (std::equal(entry.colors.begin(), entry.colors.end(), current_colors.begin(), same_rgba)) {
                m_selected_index = static_cast<int>(index);
                break;
            }
        }
    } else {
        // Legacy projects only stored one colour. Match an exact solid colour;
        // never guess among multi-colour entries sharing the same first stop.
        for (size_t index = 0; index < m_entries.size(); ++index) {
            if (m_entries[index].colors.size() == 1 && same_rgba(m_entries[index].colors.front(), current_color)) {
                m_selected_index = static_cast<int>(index);
                break;
            }
        }
    }
    create_ui(filament_name);
    Bind(wxEVT_ACTIVATE, [this](wxActivateEvent& event) {
        if (!event.GetActive() && !m_child_dialog_open && IsShown() && IsModal()) {
            m_outside_click_timer.Stop();
            EndModal(wxID_CANCEL);
        }
        event.Skip();
    });
    Bind(wxEVT_TIMER, [this](wxTimerEvent&) {
        if (m_child_dialog_open || !IsShown() || !IsModal()) return;
        const wxMouseState mouse = wxGetMouseState();
        if (!m_outside_click_armed) {
            if (!mouse.LeftIsDown()) m_outside_click_armed = true;
            return;
        }
        if (mouse.LeftIsDown() && !GetScreenRect().Contains(wxGetMousePosition())) {
            m_outside_click_timer.Stop();
            EndModal(wxID_CANCEL);
        }
    }, m_outside_click_timer.GetId());
    m_outside_click_timer.Start(20);
    Fit();
    SetMinSize(GetSize());

    // Keep the popup attached to the left edge of the clicked filament item.
    // FilamentItem lives in the process-parameter sidebar, so this also keeps
    // the popup's right edge flush with the left edge of that sidebar.
    if (parent != nullptr) {
        const wxRect anchor = parent->GetScreenRect();
        int display_index = wxDisplay::GetFromWindow(parent);
        if (display_index == wxNOT_FOUND)
            display_index = 0;
        const wxRect work_area = wxDisplay(static_cast<unsigned int>(display_index)).GetClientArea();
        const wxSize popup_size = GetSize();
        const int x = std::max(work_area.GetLeft(), anchor.GetLeft() - popup_size.GetWidth());
        const int y = std::max(work_area.GetTop(),
            std::min(anchor.GetTop(), work_area.GetBottom() - popup_size.GetHeight() + 1));
        SetPosition(wxPoint(x, y));
    } else {
        CentreOnScreen();
    }
}

void OfficialFilamentColorDialog::on_dpi_changed(const wxRect& suggested_rect)
{
    const ColorEntry* selected = m_selected_index >= 0 &&
        static_cast<size_t>(m_selected_index) < m_entries.size()
        ? &m_entries[m_selected_index]
        : nullptr;
    update_preview(selected, selected == nullptr ? m_selected_color : wxNullColour);
    refresh_buttons();
    SetMinSize(wxDefaultSize);
    Layout();
    Fit();
    SetMinSize(GetSize());
}

bool OfficialFilamentColorDialog::load_entries(const std::string& filament_id)
{
    try {
        std::lock_guard<std::mutex> lock(OfficialMaterialColors::cache_mutex());
        boost::nowide::ifstream stream(OfficialMaterialColors::cache_path().string());
        if (!stream) return false;
        nlohmann::json cache;
        stream >> cache;
        OfficialMaterialColors::validate_cache(cache);
        if (!cache.at("data").is_array()) return false;
        const std::string language = wxGetApp().app_config ? wxGetApp().app_config->get("language") : "en";
        for (const auto& item : cache["data"]) {
            if (OfficialMaterialColors::text(item, "id") != filament_id) continue;
            ColorEntry entry;
            for (const auto& value : OfficialMaterialColors::normalize_colors(item.at("colors"))) {
                wxColour color = FilamentColorAppearance::parse(value.get<std::string>());
                if (color.IsOk()) entry.colors.push_back(color);
            }
            if (entry.colors.empty()) continue;
            const std::string name = OfficialMaterialColors::localized_color_name(item, language);
            entry.name = name.empty() ? wxString::Format("#%02X%02X%02X%02X", entry.colors.front().Red(), entry.colors.front().Green(), entry.colors.front().Blue(), entry.colors.front().Alpha()) : wxString::FromUTF8(name);
            entry.gradient = FilamentColorAppearance::is_gradient(item);
            m_entries.push_back(std::move(entry));
        }
    } catch (const std::exception& e) {
        m_entries.clear();
        BOOST_LOG_TRIVIAL(warning) << "Official color cache read failed: " << e.what();
    }
    return !m_entries.empty();
}
wxBitmap OfficialFilamentColorDialog::make_swatch(const wxColour& color, const wxSize& size, bool selected) const
{
    ColorEntry entry;
    entry.colors.push_back(color);
    return make_swatch(entry, size, selected);
}

wxBitmap OfficialFilamentColorDialog::make_swatch(const ColorEntry& entry, const wxSize& size, bool selected) const
{
    wxBitmap bitmap(size);
    wxMemoryDC dc(bitmap);
    const wxColour dialog_background = m_is_dark ? wxColour("#303030") : *wxWHITE;
    dc.SetBackground(wxBrush(dialog_background));
    dc.Clear();
    dc.SetPen(*wxTRANSPARENT_PEN);
    const int inset = FromDIP(3);
    const wxRect rect(inset, inset, size.GetWidth() - inset * 2, size.GetHeight() - inset * 2);

    // wxMemoryDC does not consistently alpha-blend brushes on all platforms.
    // Composite onto a checkerboard ourselves; stored RGBA values stay untouched.
    const int checker_size = std::max(1, FromDIP(5));
    for (int x = 0; x < rect.width; ++x) {
        wxColour color = entry.colors.empty() ? *wxBLACK : entry.colors.front();
        if (entry.colors.size() > 1) {
            if (entry.gradient) {
                const int segments = static_cast<int>(entry.colors.size()) - 1;
                const double scaled = rect.width > 1 ? double(x) * segments / (rect.width - 1) : 0.0;
                const int segment = std::min(static_cast<int>(scaled), segments - 1);
                color = interpolate(entry.colors[segment], entry.colors[segment + 1], scaled - segment);
            } else {
                const size_t index = std::min(size_t(x) * entry.colors.size() / rect.width, entry.colors.size() - 1);
                color = entry.colors[index];
            }
        }
        for (int y = 0; y < rect.height; ++y) {
            const bool alternate = ((x / checker_size + y / checker_size) % 2) != 0;
            const int background = m_is_dark ? (alternate ? 100 : 65) : (alternate ? 220 : 255);
            const int alpha = color.Alpha();
            const auto blend = [alpha, background](int channel) {
                return static_cast<unsigned char>((channel * alpha + background * (255 - alpha) + 127) / 255);
            };
            dc.SetPen(wxPen(wxColour(blend(color.Red()), blend(color.Green()), blend(color.Blue()))));
            dc.DrawPoint(rect.x + x, rect.y + y);
        }
    }
    // A subtle inner border makes white visible in both the preview and palette.
    // The outer selection indicator is drawn separately below.
    const bool has_light_color = std::any_of(entry.colors.begin(), entry.colors.end(), [](const wxColour& color) {
        return color.Red() >= 224 && color.Green() >= 224 && color.Blue() >= 224;
    });
    if (!m_is_dark && has_light_color) {
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.SetPen(wxPen(wxColour("#AEB4BA"), 1));
        dc.DrawRectangle(rect);
    }
    if (selected) {
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.SetPen(wxPen(wxColour("#1FCA63"), FromDIP(2)));
        dc.DrawRectangle(1, 1, size.GetWidth() - 2, size.GetHeight() - 2);
    }
    dc.SelectObject(wxNullBitmap);
    // The bitmap dimensions above are physical pixels returned by FromDIP().
    // Preserve their logical DIP size so wxStaticBitmap does not scale the
    // preview a second time on high-DPI displays.
    bitmap.SetScaleFactor(GetDPIScaleFactor());
    return bitmap;
}

void OfficialFilamentColorDialog::create_ui(const wxString& filament_name)
{
    const wxColour background = m_is_dark ? wxColour("#303030") : *wxWHITE;
    const wxColour primary_text = m_is_dark ? wxColour("#F2F2F2") : wxColour("#232323");
    const wxColour secondary_text = m_is_dark ? wxColour("#D0D0D0") : wxColour("#232323");
    const wxColour section_text = m_is_dark ? wxColour("#B7C0C4") : wxColour("#647077");
    SetBackgroundColour(background);
    auto* main_sizer = new wxBoxSizer(wxVERTICAL);

    // Large preview and material information, matching the Creality material card.
    auto* preview_sizer = new wxBoxSizer(wxHORIZONTAL);
    const ColorEntry* selected = m_selected_index >= 0 ? &m_entries[m_selected_index] : nullptr;
    m_preview = new wxStaticBitmap(this, wxID_ANY, selected
        ? make_swatch(*selected, FromDIP(wxSize(60, 60)))
        : make_swatch(m_selected_color.IsOk() ? m_selected_color : *wxBLACK, FromDIP(wxSize(60, 60))));
    preview_sizer->Add(m_preview, 0, wxALIGN_CENTER_VERTICAL);
    preview_sizer->AddSpacer(FromDIP(14));

    auto* labels = new wxBoxSizer(wxVERTICAL);
    m_color_label = new wxStaticText(this, wxID_ANY, wxEmptyString);
    wxFont bold = m_color_label->GetFont();
    bold.SetWeight(wxFONTWEIGHT_BOLD);
    bold.SetPointSize(std::max(bold.GetPointSize(), 13));
    m_color_label->SetFont(bold);
    m_color_label->SetForegroundColour(primary_text);
    labels->Add(m_color_label, 0, wxEXPAND);
    labels->AddSpacer(FromDIP(12));

    wxString material_name = filament_name.BeforeFirst('@');
    material_name.Trim(true).Trim(false);
    if (material_name.StartsWith("Creality - "))
        material_name = material_name.Mid(11);
    auto* filament_label = new wxStaticText(this, wxID_ANY, "Creality - " + material_name,
                                             wxDefaultPosition, FromDIP(wxSize(250, -1)), wxST_ELLIPSIZE_END);
    wxFont material_font = filament_label->GetFont();
    material_font.SetPointSize(std::max(material_font.GetPointSize(), 11));
    filament_label->SetFont(material_font);
    filament_label->SetForegroundColour(secondary_text);
    labels->Add(filament_label, 0, wxEXPAND);
    preview_sizer->Add(labels, 1, wxALIGN_CENTER_VERTICAL);
    main_sizer->Add(preview_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(16));
    main_sizer->AddSpacer(FromDIP(14));

    auto* separator_text = new wxStaticText(this, wxID_ANY, _L("Official Filament"));
    wxFont section_font = separator_text->GetFont();
    section_font.SetPointSize(std::max(section_font.GetPointSize(), 11));
    separator_text->SetFont(section_font);
    separator_text->SetForegroundColour(section_text);
    main_sizer->Add(separator_text, 0, wxLEFT | wxRIGHT, FromDIP(16));
    main_sizer->AddSpacer(FromDIP(6));

    if (!m_entries.empty()) {
        const int rows = static_cast<int>((m_entries.size() + GRID_COLUMNS - 1) / GRID_COLUMNS);
        const bool needs_scroll = rows > MAX_VISIBLE_ROWS;
        wxScrolledWindow* scroll = needs_scroll
            ? new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL | wxNO_BORDER)
            : nullptr;
        wxWindow* palette = scroll ? static_cast<wxWindow*>(scroll)
                                   : static_cast<wxWindow*>(new wxPanel(this, wxID_ANY));
        palette->SetBackgroundColour(background);
        auto* grid = new wxGridSizer(rows, GRID_COLUMNS, FromDIP(5), FromDIP(5));
        const wxSize button_size = FromDIP(wxSize(30, 30));
        for (size_t index = 0; index < m_entries.size(); ++index) {
            auto* button = new wxBitmapButton(palette, wxID_ANY,
                make_swatch(m_entries[index], button_size, static_cast<int>(index) == m_selected_index),
                wxDefaultPosition, button_size, wxBU_EXACTFIT | wxBORDER_NONE);
            button->SetToolTip(m_entries[index].name);
            button->Bind(wxEVT_BUTTON, [this, index](wxCommandEvent&) { select_entry(index); });
            grid->Add(button, 0, wxALIGN_CENTER);
            m_buttons.push_back(button);
        }
        palette->SetSizer(grid);
        // Use actual control sizes: scaling a pre-summed DIP size causes rounding errors.
        const wxSize content_size = grid->CalcMin();
        if (scroll) {
            const int gap = grid->GetVGap();
            const int row_height = (content_size.y - (rows - 1) * gap) / rows;
            const int scrollbar_width = std::max(FromDIP(18),
                wxSystemSettings::GetMetric(wxSYS_VSCROLL_X, scroll));
            scroll->SetMinSize(wxSize(content_size.x + scrollbar_width + FromDIP(4),
                MAX_VISIBLE_ROWS * row_height + (MAX_VISIBLE_ROWS - 1) * gap));
            scroll->SetScrollRate(0, row_height + gap);
            scroll->SetVirtualSize(content_size);
        } else {
            palette->SetMinSize(content_size);
        }
        main_sizer->Add(palette, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(16));
    }

    main_sizer->AddSpacer(FromDIP(16));
    auto* more = new Button(this, "+ " + _L("More Colors"));
    more->SetMinSize(FromDIP(wxSize(120, 34)));
    more->SetCornerRadius(FromDIP(5));
    const wxColour button_normal = m_is_dark ? background : *wxWHITE;
    const wxColour button_hover = m_is_dark ? wxColour("#3B3B3B") : wxColour("#F4F6F7");
    const wxColour button_border = m_is_dark ? wxColour("#8B9297") : wxColour("#AEB8BE");
    const wxColour button_text = m_is_dark ? wxColour("#D8D8D8") : wxColour("#464F54");
    more->SetBackgroundColor(StateColor(std::pair<wxColour, int>(button_hover, StateColor::Hovered),
                                        std::pair<wxColour, int>(button_normal, StateColor::Normal)));
    more->SetBorderColor(StateColor(std::pair<wxColour, int>(button_border, StateColor::Normal)));
    more->SetTextColor(StateColor(std::pair<wxColour, int>(button_text, StateColor::Normal)));
    more->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        m_child_dialog_open = true;
        wxColourData data;
        data.SetChooseFull(true);
        data.SetChooseAlpha(false);
        data.SetColour(m_selected_color.IsOk() ? m_selected_color : *wxBLACK);
        auto custom_colors = wxGetApp().app_config->get_custom_color_from_config();
        for (size_t i = 0; i < custom_colors.size() && i < CUSTOM_COLOR_COUNT; ++i)
            data.SetCustomColour(static_cast<int>(i), string_to_wxColor(custom_colors[i]));
        wxColourDialog dialog(this, &data);
        dialog.SetTitle(_L("Please choose the filament colour"));
        const int result = dialog.ShowModal();
        m_child_dialog_open = false;
        if (result == wxID_OK) {
            const auto& selected_data = dialog.GetColourData();
            custom_colors.resize(CUSTOM_COLOR_COUNT);
            for (int i = 0; i < CUSTOM_COLOR_COUNT; ++i)
                custom_colors[i] = color_to_string(selected_data.GetCustomColour(i));
            wxGetApp().app_config->save_custom_color_to_config(custom_colors);
            select_custom_color(selected_data.GetColour());
        }
    });
    main_sizer->Add(more, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, FromDIP(16));

    SetSizer(main_sizer);
    update_preview(selected, m_selected_index < 0 ? m_selected_color : wxNullColour);
}

void OfficialFilamentColorDialog::select_entry(size_t index)
{
    if (index >= m_entries.size()) return;
    m_selected_index = static_cast<int>(index);
    m_selected_color = m_entries[index].colors.front();
    update_preview(&m_entries[index]);
    refresh_buttons();
    m_outside_click_timer.Stop();
    EndModal(wxID_OK);
}

void OfficialFilamentColorDialog::select_custom_color(const wxColour& color)
{
    if (!color.IsOk()) return;
    m_selected_index = -1;
    m_selected_color = color;
    update_preview(nullptr, color);
    refresh_buttons();
    m_outside_click_timer.Stop();
    EndModal(wxID_OK);
}

std::vector<wxColour> OfficialFilamentColorDialog::GetSelectedColours() const
{
    if (m_selected_index >= 0 && static_cast<size_t>(m_selected_index) < m_entries.size())
        return m_entries[m_selected_index].colors;
    return m_selected_color.IsOk() ? std::vector<wxColour>{m_selected_color} : std::vector<wxColour>{};
}

bool OfficialFilamentColorDialog::IsSelectedGradient() const
{
    return m_selected_index >= 0 && static_cast<size_t>(m_selected_index) < m_entries.size() &&
           m_entries[m_selected_index].gradient;
}
void OfficialFilamentColorDialog::update_preview(const ColorEntry* entry, const wxColour& custom_color)
{
    if (m_preview == nullptr) return;
    if (entry != nullptr) {
        m_preview->SetBitmap(make_swatch(*entry, FromDIP(wxSize(60, 60))));
        m_color_label->SetLabel(entry->name);
    } else {
        const wxColour color = custom_color.IsOk() ? custom_color : *wxBLACK;
        m_preview->SetBitmap(make_swatch(color, FromDIP(wxSize(60, 60))));
        m_color_label->SetLabel(wxString::Format("#%02X%02X%02X%02X", color.Red(), color.Green(), color.Blue(), color.Alpha()));
    }
    Layout();
}

void OfficialFilamentColorDialog::refresh_buttons()
{
    const wxSize size = FromDIP(wxSize(30, 30));
    for (size_t index = 0; index < m_buttons.size(); ++index) {
        m_buttons[index]->SetMinSize(size);
        m_buttons[index]->SetSize(size);
        m_buttons[index]->SetBitmap(make_swatch(m_entries[index], size, static_cast<int>(index) == m_selected_index));
    }
}

}} // namespace Slic3r::GUI
