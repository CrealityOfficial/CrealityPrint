#include "FilamentPanel.h"
#include <cassert>
#include <cmath>
#include <fstream>
#include <cctype>
#include <mutex>
#include <algorithm>
#include <functional>
#include <string>
#include <utility>
#include <wx/control.h>
#include <wx/dcclient.h>
#include <wx/dcgraph.h>
#include <wx/dcmemory.h>
#include <wx/dcbuffer.h>
#include "Widgets/LocalDragHandler.hpp"
#include "Widgets/LocalDragPreview.hpp"
#include <memory>
#include "ImGuiWrapper.hpp"
#include "wx/menu.h"
#include "wx/colour.h"
#include "wx/wx.h"
#include <wx/colordlg.h>
#include "GUI_App.hpp"
#include "OfficialFilamentColorDialog.hpp"
#include "ColorSpaceConvert.hpp"
#include "Plater.hpp"
#include "libslic3r/Preset.hpp"
#include "Tab.hpp"
#include "MainFrame.hpp"
#include "libslic3r/Config.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/Thread.hpp"
#include "libslic3r/Utils.hpp"
#include "slic3r/Utils/UndoRedo.hpp"
#include "slic3r/GUI/print_manage/Utils.hpp"
#include "slic3r/GUI/print_manage/PrinterBoxFilamentPanel.hpp"
#include "slic3r/GUI/PartPlate.hpp"
#include <cstdint>
#include "print_manage/data/DataCenter.hpp"
#include "slic3r/Utils/ProfileFamilyLoader.hpp"
#include "LoginTip.hpp"
#include <boost/log/trivial.hpp>
#include <wx/event.h>
#include "ColorDecomposeDialog.hpp"
#include "ColorDecomposeSupport.hpp"
#include "MixedFilamentDialog.hpp"
#include "libslic3r/MixedFilament.hpp"
#include "libslic3r/Preset.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/Config.hpp"
#include "GUI_Utils.hpp"
#include "MsgDialog.hpp"
#include "Widgets/Button.hpp"
#include "libslic3r/FDM/MachineVender.hpp"
#include <map>
#include <numeric>
#include <tuple>
#include <unordered_set>
#include <wx/scrolwin.h>
#ifdef __WXMSW__
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

void ApplyWindowShadow(wxWindow* window) {
    if (window == nullptr || window->GetHWND() == nullptr)
        return;

    HWND hwnd = (HWND)window->GetHWND();
    DWMNCRENDERINGPOLICY policy = DWMNCRP_ENABLED;
    DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));

    MARGINS margins = { 1, 1, 1, 1 }; // 阴影厚度
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    // Keep the native DWM shadow, but make its one-pixel frame match the
    // borderless dialog instead of leaving a bright system border in dark mode.
    const wxColour background = window->GetBackgroundColour();
    const int background_brightness = background.IsOk()
        ? (background.Red() * 299 + background.Green() * 587 + background.Blue() * 114) / 1000
        : 255;
    const BOOL use_dark_frame = background_brightness < 128 ? TRUE : FALSE;
    if (FAILED(::DwmSetWindowAttribute(hwnd, 20, &use_dark_frame, sizeof(use_dark_frame))))
        ::DwmSetWindowAttribute(hwnd, 19, &use_dark_frame, sizeof(use_dark_frame));
    if (background.IsOk()) {
        const COLORREF border_colour = RGB(background.Red(), background.Green(), background.Blue());
        ::DwmSetWindowAttribute(hwnd, 34, &border_colour, sizeof(border_colour));
    }
    ::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE |
                       SWP_FRAMECHANGED);
    ::RedrawWindow(hwnd, nullptr, nullptr,
                   RDW_INVALIDATE | RDW_FRAME | RDW_UPDATENOW);
}
#endif
static bool ShouldDark(const wxColour& bgColor)
{
    const int brightness = (bgColor.Red() * 299 + bgColor.Green() * 587 + bgColor.Blue() * 114) / 1000;
    // Outer content follows the design's broad black-text range: red and other
    // saturated mid tones stay black, while purple/navy/black switch to white.
    return brightness > 70;
}

static wxColour GetTextColorBasedOnBackground(const wxColour& bgColor) {
	if (ShouldDark(bgColor)) {
		return *wxBLACK;
	}
	else {
		return *wxWHITE;
	}
}


static wxBitmap TintBitmap(const wxBitmap& bmp, const wxColour& color, double alpha_scale = 1.0)
{
    if (!bmp.IsOk())
        return bmp;

    wxImage img = bmp.ConvertToImage();
    if (!img.IsOk())
        return bmp;
    if (!img.HasAlpha())
        img.InitAlpha();

    alpha_scale = std::max(0.0, std::min(1.0, alpha_scale));
    const int w = img.GetWidth();
    const int h = img.GetHeight();
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const unsigned char alpha = img.GetAlpha(x, y);
            if (alpha == wxALPHA_TRANSPARENT)
                continue;
            img.SetRGB(x, y, color.Red(), color.Green(), color.Blue());
            img.SetAlpha(x, y, static_cast<unsigned char>(std::lround(alpha * alpha_scale)));
        }
    }

    wxBitmap tinted(img);
    // wxImage does not retain the Retina backing scale of the source bitmap.
    tinted.SetScaleFactor(bmp.GetScaleFactor());
    return tinted;
}

// Slightly darker shade of the same colour, used as the background of the inner
// blocks (CFS sync / "..." menu). Works in HSV so hue and saturation are
// preserved and the result stays in the same colour family: red -> dark red,
// yellow -> dark yellow. White has no hue, so it degrades to grey.
//
// The shift is deliberately subtle: per the design the inner blocks are set
// apart mainly by their light outline (see INNER_BLOCK_BORDER_ALPHA), with the
// fill only hinting at a recess. Darkening too much reads as a grey mask.
static wxColour DeepenColor(const wxColour& color)
{
    if (!color.IsOk())
        return color;

    // Fully transparent filament colour is rendered as a checkerboard elsewhere;
    // keep it untouched here so callers can detect and special-case it.
    if (color.Alpha() == 0)
        return color;

    float h = 0.f, s = 0.f, v = 0.f;
    RGB2HSV(color.Red() / 255.f, color.Green() / 255.f, color.Blue() / 255.f, &h, &s, &v);

    // Near-black has almost no brightness left to remove, so nudge it up instead
    // to keep the inner blocks from vanishing into the block.
    v = v < 0.12f ? v + 0.09f : v * 0.84f;
    v = std::min(1.f, v);

    // HSV -> RGB.
    const float hh = (h < 0.f ? 0.f : h) / 60.f;
    const int   i  = static_cast<int>(std::floor(hh)) % 6;
    const float f  = hh - std::floor(hh);
    const float p  = v * (1.f - s);
    const float q  = v * (1.f - s * f);
    const float t  = v * (1.f - s * (1.f - f));

    float r = v, g = v, b = v;
    switch (i) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    case 5: r = v; g = p; b = q; break;
    default: break;
    }

    auto to_byte = [](float c) -> unsigned char {
        return static_cast<unsigned char>(std::lround(std::min(1.f, std::max(0.f, c)) * 255.f));
    };
    return wxColour(to_byte(r), to_byte(g), to_byte(b), color.Alpha());
}

// Blends `over` onto `under` at the given alpha. wxGCDC honours pen alpha, but
// blending explicitly keeps the result identical on the plain-wxDC paths.
static wxColour BlendColor(const wxColour& under, const wxColour& over, double alpha)
{
    auto mix = [alpha](unsigned char u, unsigned char o) -> unsigned char {
        return static_cast<unsigned char>(std::lround(u * (1.0 - alpha) + o * alpha));
    };
    return wxColour(mix(under.Red(), over.Red()),
                    mix(under.Green(), over.Green()),
                    mix(under.Blue(), over.Blue()));
}

// Light outline around the inner blocks, as in the design. Drawn as a blend of
// white over the fill so it reads as a subtle rim on both light and dark colours.
static wxColour InnerBlockBorderColor(const wxColour& fill)
{
    return BlendColor(fill, *wxWHITE, 0.40);
}

// Content colour for the inner blocks uses the same mid-grey threshold as the
// number, material name and lower-row arrow. This keeps all content white on
// red/purple/navy and black on genuinely light filament colours.
static wxColour InnerBlockForegroundColor(const wxColour& fill)
{
    const int brightness = (fill.Red() * 299 + fill.Green() * 587 + fill.Blue() * 114) / 1000;
    return brightness > 150 ? *wxBLACK : *wxWHITE;
}

//fix:[15095]After zooming and switching pages, materials on the right appear too small, and the edit button is cut off.
namespace {

// Width of the filament preset drop-down list. Capped so the list does not grow
// together with the sidebar.
constexpr int FILAMENT_LIST_WIDTH_DIP = 360;
// Height of the (hidden) combobox hosting the preset list. It is never drawn,
// but still needs a sane size for layout.
constexpr int FILAMENT_LIST_ANCHOR_HEIGHT_DIP = 35;
// Extra gap between the block and the list, so the list does not clip the lower
// half. Fed to DropDown::setDrapDownGap().
constexpr int FILAMENT_LIST_GAP_DIP = 5;

std::string preset_display_name_for_filament(const Slic3r::Preset& preset)
{
    auto* bundle = Slic3r::GUI::wxGetApp().preset_bundle;
    if (bundle == nullptr)
        return preset.label(false);
    return bundle->get_preset_display_name_with_material_alias(Slic3r::Preset::TYPE_FILAMENT, preset);
}
// The popup panel is never shown; it only hosts the preset combobox so that the
// combobox' own drop-down can be opened directly from the lower half of a block.
//
// Since the panel is never shown, its own screen position is not dependable, so
// the list is anchored explicitly to the block's screen rect instead: the list
// opens directly under the block, with a fixed width that does not follow the
// sidebar.
void layout_filament_popup(FilamentItem* item, FilamentPopPanel* popup, bool /*fit_contents*/)
{
    if (!item || !popup || !popup->m_filamentCombox)
        return;

    const wxRect anchor(item->ClientToScreen(wxPoint(0, 0)), item->GetSize());
    popup->m_filamentCombox->SetDropDownAnchor(
        anchor, wxWindow::FromDIP(FILAMENT_LIST_WIDTH_DIP, item), item);
}

class FilamentGroupingCard final : public wxPanel
{
public:
    FilamentGroupingCard(wxWindow* parent, const wxColour& background, const wxColour& fill,
                         const wxColour& border, const wxColour& hovered_border)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
        , m_background(background)
        , m_fill(fill)
        , m_border(border)
        , m_hovered_border(hovered_border)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetBackgroundColour(m_background);
        Bind(wxEVT_PAINT, &FilamentGroupingCard::on_paint, this);
    }

    void set_drop_hovered(bool hovered)
    {
        if (m_drop_hovered == hovered)
            return;
        m_drop_hovered = hovered;
        Refresh(false);
    }

private:
    void on_paint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        const wxSize size = GetClientSize();
        dc.SetBackground(wxBrush(m_background));
        dc.Clear();

        wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
        if (gc == nullptr)
            return;
        gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);

        const double border_width = m_drop_hovered ? 1.3 : 1.2;
        wxGraphicsPenInfo pen_info(m_drop_hovered ? m_hovered_border : m_border,
                                   border_width,
                                   m_drop_hovered ? wxPENSTYLE_USER_DASH : wxPENSTYLE_SOLID);
        wxDash dashes[] = {static_cast<wxDash>(FromDIP(3)), static_cast<wxDash>(FromDIP(3))};
        if (m_drop_hovered)
            pen_info.Dashes(2, dashes).Cap(wxCAP_BUTT).Join(wxJOIN_ROUND);

        gc->SetPen(gc->CreatePen(pen_info));
        gc->SetBrush(wxBrush(m_fill));
        const double inset = border_width * 0.5;
        gc->DrawRoundedRectangle(inset, inset,
                                 std::max(0.0, static_cast<double>(size.GetWidth()) - border_width),
                                 std::max(0.0, static_cast<double>(size.GetHeight()) - border_width),
                                 FromDIP(4));
        delete gc;
    }

private:
    wxColour m_background;
    wxColour m_fill;
    wxColour m_border;
    wxColour m_hovered_border;
    bool     m_drop_hovered {false};
};

class FilamentGroupingCardSection final : public wxPanel
{
public:
    FilamentGroupingCardSection(wxWindow* parent, const wxColour& colour, bool round_top, bool round_bottom)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
        , m_colour(colour)
        , m_round_top(round_top)
        , m_round_bottom(round_bottom)
    {
        SetBackgroundColour(m_colour);
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT, &FilamentGroupingCardSection::on_paint, this);
    }

private:
    void on_paint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        const wxSize size = GetClientSize();
        dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
        dc.Clear();

        wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
        if (gc == nullptr)
            return;
        gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush(m_colour));

        const double width = static_cast<double>(size.GetWidth());
        const double height = static_cast<double>(size.GetHeight());
        const double radius = std::min<double>(FromDIP(4), std::min(width, height) * 0.5);
        if (m_round_top && m_round_bottom) {
            gc->DrawRoundedRectangle(0.0, 0.0, width, height, radius);
        } else if (m_round_top) {
            gc->DrawRoundedRectangle(0.0, 0.0, width, std::min(height, radius * 2.0), radius);
            if (height > radius)
                gc->DrawRectangle(0.0, radius, width, height - radius);
        } else if (m_round_bottom) {
            if (height > radius)
                gc->DrawRectangle(0.0, 0.0, width, height - radius);
            gc->DrawRoundedRectangle(0.0, std::max(0.0, height - radius * 2.0),
                                     width, std::min(height, radius * 2.0), radius);
        } else {
            gc->DrawRectangle(0.0, 0.0, width, height);
        }
        delete gc;
    }

private:
    wxColour m_colour;
    bool     m_round_top;
    bool     m_round_bottom;
};

class FilamentGroupingCloseButton final : public wxPanel
{
public:
    FilamentGroupingCloseButton(wxWindow* parent, const wxColour& icon_colour,
                                const wxColour& hover_background)
        : wxPanel(parent, wxID_CANCEL, wxDefaultPosition,
                  wxWindow::FromDIP(wxSize(28, 28), parent), wxBORDER_NONE)
        , m_icon_colour(icon_colour)
        , m_hover_background(hover_background)
    {
        const wxSize size = wxWindow::FromDIP(wxSize(28, 28), parent);
        SetMinSize(size);
        SetMaxSize(size);
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetCursor(wxCursor(wxCURSOR_HAND));
        Bind(wxEVT_PAINT, &FilamentGroupingCloseButton::on_paint, this);
        Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent&) { m_hovered = true; Refresh(false); });
        Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { m_hovered = false; Refresh(false); });
        Bind(wxEVT_LEFT_UP, [this](wxMouseEvent&) {
            wxCommandEvent event(wxEVT_BUTTON, GetId());
            event.SetEventObject(this);
            GetEventHandler()->ProcessEvent(event);
        });
    }

private:
    void on_paint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        const wxSize size = GetClientSize();
        dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
        dc.Clear();

        wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
        if (gc == nullptr)
            return;
        gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
        if (m_hovered) {
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->SetBrush(wxBrush(m_hover_background));
            gc->DrawRoundedRectangle(0.0, 0.0, size.GetWidth(), size.GetHeight(), FromDIP(4));
        }
        gc->SetPen(gc->CreatePen(wxGraphicsPenInfo(m_icon_colour, 1.6)));
        const double inset = FromDIP(7.5);
        gc->StrokeLine(inset, inset, size.GetWidth() - inset, size.GetHeight() - inset);
        gc->StrokeLine(size.GetWidth() - inset, inset, inset, size.GetHeight() - inset);
        delete gc;
    }

private:
    wxColour m_icon_colour;
    wxColour m_hover_background;
    bool     m_hovered {false};
};

class FilamentGroupingNozzleSelector final : public wxPanel
{
public:
    FilamentGroupingNozzleSelector(wxWindow* parent, const wxString& value,
                                   const wxColour& background, const wxColour& border,
                                   const wxColour& text_colour)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition,
                  wxWindow::FromDIP(wxSize(86, 28), parent), wxBORDER_NONE)
        , m_value(value)
        , m_background(background)
        , m_border(border)
        , m_text_colour(text_colour)
    {
        const wxSize size = wxWindow::FromDIP(wxSize(86, 28), parent);
        SetMinSize(size);
        SetMaxSize(size);
        SetFont(Label::Body_13);
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT, &FilamentGroupingNozzleSelector::on_paint, this);
    }

private:
    void on_paint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        const wxSize size = GetClientSize();
        dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
        dc.Clear();

        wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
        if (gc != nullptr) {
            gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
            const double border_width = 1.0;
            gc->SetPen(gc->CreatePen(wxGraphicsPenInfo(m_border, border_width)));
            gc->SetBrush(wxBrush(m_background));
            const double inset = border_width * 0.5;
            gc->DrawRoundedRectangle(inset, inset,
                                     std::max(0.0, static_cast<double>(size.GetWidth()) - border_width),
                                     std::max(0.0, static_cast<double>(size.GetHeight()) - border_width),
                                     FromDIP(4));
            delete gc;
        }

        dc.SetFont(GetFont());
        dc.SetTextForeground(m_text_colour);
        dc.DrawLabel(m_value, wxRect(0, 0, size.GetWidth(), size.GetHeight()), wxALIGN_CENTER);
    }

private:
    wxString m_value;
    wxColour m_background;
    wxColour m_border;
    wxColour m_text_colour;
};

class FilamentGroupingChip final : public wxPanel
{
public:
    FilamentGroupingChip(wxWindow* parent, size_t filament_id, const wxColour& colour, const wxString& material,
                         std::function<bool(FilamentGroupingChip*)> begin,
                         Slic3r::GUI::LocalDragHandler::Move move,
                         std::function<void(size_t, const wxPoint&, bool)> end)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition,
                  wxWindow::FromDIP(wxSize(90, 49), parent), wxBORDER_NONE)
        , m_filament_id(filament_id)
        , m_colour(colour)
        , m_material(material)
    {
        const wxSize chip_size = wxWindow::FromDIP(wxSize(90, 49), parent);
        SetMinSize(chip_size);
        SetMaxSize(chip_size);
        SetFont(Label::Body_12);
        SetCursor(wxCursor(wxCURSOR_HAND));
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        //SetToolTip(wxString::Format("%d: %s", static_cast<int>(m_filament_id + 1), m_material));
        Bind(wxEVT_PAINT, &FilamentGroupingChip::on_paint, this);
        m_drag = std::make_unique<Slic3r::GUI::LocalDragHandler>(this,
            [this, begin = std::move(begin)] { return begin(this); },
            [this, on_move = std::move(move)](const wxPoint& position) {
                show_drag_preview(position);
                on_move(position);
            },
            [this, filament_id, end = std::move(end)](const wxPoint& position, bool dropped) {
                if (m_drag_preview) {
                    m_drag_preview->Hide();
                    delete m_drag_preview.get();
                }
                end(filament_id, position, dropped);
            });
    }

    void CancelDrag() { m_drag->Cancel(); }

private:
    void on_paint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        draw_chip(dc);
    }

    void show_drag_preview(const wxPoint& position)
    {
        if (!m_drag_preview) {
            wxBitmap bitmap;
            if (!bitmap.CreateWithDIPSize(ToDIP(GetClientSize()), GetDPIScaleFactor()))
                return;
            wxMemoryDC dc(bitmap);
            draw_chip(dc);
            dc.SelectObject(wxNullBitmap);
            m_drag_preview = new Slic3r::GUI::LocalDragPreview(
                wxGetTopLevelParent(this), bitmap, GetClientSize());
        }
        m_drag_preview->Follow(position);
    }

    void draw_chip(wxDC& dc)
    {
        const wxSize size = GetClientSize();
        dc.SetFont(GetFont());
        dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
        dc.Clear();
        const int perceived_brightness =
            (m_colour.Red() * 299 + m_colour.Green() * 587 + m_colour.Blue() * 114) / 1000;
        const bool light_background = perceived_brightness >= 150;
        const wxColour light_outline("#D5D9E1");
        const int outline_width = std::max(1, FromDIP(1));
        const int corner_radius = FromDIP(4);
        wxGraphicsContext* gc = wxGraphicsContext::CreateFromUnknownDC(dc);
        if (gc != nullptr) {
            gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->SetBrush(wxBrush(light_background ? light_outline : m_colour));
            gc->DrawRoundedRectangle(0.0, 0.0, size.GetWidth(), size.GetHeight(), corner_radius);
            if (light_background) {
                gc->SetBrush(wxBrush(m_colour));
                gc->DrawRoundedRectangle(
                    outline_width, outline_width,
                    std::max(0, size.GetWidth() - outline_width * 2),
                    std::max(0, size.GetHeight() - outline_width * 2),
                    std::max(0, corner_radius - outline_width));
            }
            delete gc;
        } else {
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.SetBrush(wxBrush(light_background ? light_outline : m_colour));
            dc.DrawRoundedRectangle(0, 0, size.GetWidth(), size.GetHeight(), corner_radius);
            if (light_background) {
                dc.SetBrush(wxBrush(m_colour));
                dc.DrawRoundedRectangle(
                    outline_width, outline_width,
                    std::max(0, size.GetWidth() - outline_width * 2),
                    std::max(0, size.GetHeight() - outline_width * 2),
                    std::max(0, corner_radius - outline_width));
            }
        }

        const wxColour foreground = light_background ? wxColour("#20242A") : *wxWHITE;
        const wxColour separator = light_background ? wxColour(32, 36, 42, 85)
                                                    : wxColour(255, 255, 255, 90);
        dc.SetTextForeground(foreground);
        const int number_height = std::min(size.GetHeight(), FromDIP(20));
        dc.DrawLabel(wxString::Format("%d", static_cast<int>(m_filament_id + 1)),
                     wxRect(0, 0, size.GetWidth(), number_height), wxALIGN_CENTER);
        dc.SetPen(wxPen(separator, 1));
        dc.DrawLine(FromDIP(4), number_height, size.GetWidth() - FromDIP(4), number_height);
        const wxString material = wxControl::Ellipsize(m_material, dc, wxELLIPSIZE_END,
                                                        std::max(0, size.GetWidth() - FromDIP(6)));
        dc.DrawLabel(material, wxRect(0, number_height, size.GetWidth(), size.GetHeight() - number_height),
                     wxALIGN_CENTER);
    }

private:
    size_t                      m_filament_id;
    wxColour                    m_colour;
    wxString                    m_material;
    wxWeakRef<Slic3r::GUI::LocalDragPreview> m_drag_preview;
    // Destroy the handler first so cancellation clears the preview while alive.
    std::unique_ptr<Slic3r::GUI::LocalDragHandler> m_drag;
};

} // namespace

/*
FilamentButtonStateHandler
*/
FilamentButtonStateHandler::FilamentButtonStateHandler(wxWindow* owner)
	: owner_(owner)
{
	owner_->PushEventHandler(this);
}

FilamentButtonStateHandler::~FilamentButtonStateHandler() { owner_->RemoveEventHandler(this); }


void FilamentButtonStateHandler::update_binds()
{
	Bind(wxEVT_ENTER_WINDOW, &FilamentButtonStateHandler::changed, this);
	Bind(wxEVT_LEAVE_WINDOW, &FilamentButtonStateHandler::changed, this);
}


FilamentButtonStateHandler::FilamentButtonStateHandler(FilamentButtonStateHandler* parent, wxWindow* owner)
	: FilamentButtonStateHandler(owner)
{
	m_states = Normal;
}

void FilamentButtonStateHandler::changed(wxEvent& event)
{
	if (event.GetEventType() == wxEVT_ENTER_WINDOW)
	{
		m_states = Hover;
	}
	else if (event.GetEventType() == wxEVT_LEAVE_WINDOW)
	{
		m_states = Normal;
	}

	event.Skip();
	owner_->Refresh();
}

/*
FilamentButton
*/
BEGIN_EVENT_TABLE(FilamentButton, wxWindow)

EVT_LEFT_DOWN(FilamentButton::mouseDown)
EVT_LEFT_UP(FilamentButton::mouseReleased)
EVT_PAINT(FilamentButton::paintEvent)
EVT_SIZE(FilamentButton::OnSize)
EVT_MOTION(FilamentButton::OnMouseMove)
EVT_LEAVE_WINDOW(FilamentButton::OnMouseLeave)

END_EVENT_TABLE()

namespace {
// Geometry of the owner-drawn children inside the colour block, in DIP.
constexpr int    FILAMENT_BLOCK_PAD_DIP = 1;
constexpr double FILAMENT_INNER_PLATE_HEIGHT_RATIO = 0.90;
constexpr double FILAMENT_SYNC_PLATE_WIDTH_RATIO   = 0.47;
constexpr double FILAMENT_TOP_ROW_HEIGHT_RATIO     = 0.54;
} // namespace

FilamentButton::FilamentButton(wxWindow* parent,
	wxString text,
	const wxPoint& pos,
	const wxSize& size, long style) : m_state_handler(this)
{
	if (style & wxBORDER_NONE)
		m_border_width = 0;

	if (!text.IsEmpty())
	{
		m_label = text;
	}

	wxWindow::Create(parent, wxID_ANY, pos, size, style);
	m_state_handler.update_binds();

    // Use wxPanel instead of wxButton for reliable owner-draw on GTK/Linux
    m_child_button = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    m_child_button->Bind(wxEVT_PAINT, &FilamentButton::OnChildButtonPaint, this);
    m_child_button->Bind(wxEVT_LEFT_DOWN, &FilamentButton::OnChildButtonClick, this);
    m_child_button->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& event) {
        set_top_hover_region(2);
        event.Skip();
    });
    m_child_button->Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& event) {
        set_top_hover_region(0);
        event.Skip();
    });
    m_child_button->Bind(wxEVT_RIGHT_UP, [](wxMouseEvent& event) {
            // The overflow menu is available only through the "..." button.
            event.Skip(false);
        });
    // Load the bitmap (use this window so the icon is rasterized at the correct
    // per-monitor DPI; passing nullptr would use the primary monitor's scaling).
    m_bitmap = create_scaled_bitmap("switch_cfs_tip", this, 16);

    layout_child_windows();
}

void FilamentButton::enable_menu_button(bool enable)
{
    if (m_menu_area_enabled == enable)
        return;

    m_menu_area_enabled = enable;
    layout_child_windows();
    Refresh();
}

wxRect FilamentButton::menu_area_rect() const
{
    if (!m_menu_area_enabled)
        return wxRect();

    const wxSize size = GetSize();
    if (size.GetWidth() <= 0 || size.GetHeight() <= 0)
        return wxRect();

    const int w = std::max(1, size.GetWidth() / 4);
    return wxRect(size.GetWidth() - w, 0, w, size.GetHeight());
}

int FilamentButton::inner_plate_height() const
{
    const int h = GetSize().GetHeight();
    if (h <= 0)
        return 0;
    return std::max(1, static_cast<int>(std::lround(h * FILAMENT_INNER_PLATE_HEIGHT_RATIO)));
}

wxRect FilamentButton::menu_plate_rect() const
{
    wxRect plate = menu_area_rect();
    if (plate.IsEmpty())
        return wxRect();

    // Draw a square chip with the same height as the centred sync chip. The
    // whole right-hand quarter remains clickable for easier targeting.
    const int side = std::min(inner_plate_height(), plate.GetWidth());
    plate = wxRect(plate.GetLeft() + (plate.GetWidth() - side) / 2,
                   plate.GetTop() + (plate.GetHeight() - side) / 2,
                   side,
                   side);
    return side > 0 ? plate : wxRect();
}

void FilamentButton::OnSize(wxSizeEvent& event)
{
    layout_child_windows();
    event.Skip();
}

void FilamentButton::set_top_hover_region(int region)
{
    if (m_top_hover_region == region)
        return;

    m_top_hover_region = region;
    Refresh();
    if (m_child_button)
        m_child_button->Refresh();
}

wxRect FilamentButton::label_area_rect() const
{
    if (!m_menu_area_enabled)
        return wxRect();

    const wxSize size = GetSize();
    return wxRect(0, 0, std::max(1, size.GetWidth() / 4), size.GetHeight());
}

void FilamentButton::OnMouseMove(wxMouseEvent& event)
{
    if (m_menu_area_enabled) {
        const wxPoint point = event.GetPosition();
        if (menu_area_rect().Contains(point))
            set_top_hover_region(3);
        else if (!m_sync_box_filament || label_area_rect().Contains(point))
            set_top_hover_region(1);
        else
            set_top_hover_region(0);
    }
    event.Skip();
}

void FilamentButton::OnMouseLeave(wxMouseEvent& event)
{
    if (m_menu_area_enabled) {
        // Entering the CFS child also emits LEAVE on this parent. Preserve the
        // child hover state instead of immediately clearing it.
        if (m_child_button && m_child_button->IsShown() &&
            m_child_button->GetScreenRect().Contains(wxGetMousePosition()))
            set_top_hover_region(2);
        else
            set_top_hover_region(0);
    }
    event.Skip();
}

void FilamentButton::layout_child_windows()
{
    const wxSize size = GetSize();
    if (size.GetWidth() <= 0 || size.GetHeight() <= 0)
        return;

    const int pad = FromDIP(FILAMENT_BLOCK_PAD_DIP);

    // The sync chip is centred in the upper half, but may move left enough to
    // keep the overflow chip clear on narrow filament blocks.
    const wxRect menu_plate = menu_plate_rect();
    const int content_right = menu_plate.IsEmpty() ? size.GetWidth() : menu_plate.GetLeft();

    if (m_child_button) {
        const int cfs_h = inner_plate_height();
        const int preferred_w = static_cast<int>(std::lround(size.GetWidth() * FILAMENT_SYNC_PLATE_WIDTH_RATIO));
        const int cfs_w = std::max(1, std::min(preferred_w, content_right - 2 * pad));
        int cfs_x = (size.GetWidth() - cfs_w) / 2;
        cfs_x     = std::min(cfs_x, content_right - pad - cfs_w);
        cfs_x     = std::max(cfs_x, pad);

        m_child_button->SetSize(wxSize(cfs_w, cfs_h));
        m_child_button->SetPosition(wxPoint(cfs_x, (size.GetHeight() - cfs_h) / 2));
    }
}

void FilamentButton::show_menu()
{
    FilamentItem* parentItem          = dynamic_cast<FilamentItem*>(GetParent());
    const int     filament_item_index = parentItem ? parentItem->index() : -1;

    const wxRect menu_rect = menu_area_rect();
    MaterialContextMenu menu(this, filament_item_index);
    const int selected = GetPopupMenuSelectionFromUser(menu, wxPoint(menu_rect.GetRight(), menu_rect.GetBottom()));
    menu.ExecuteSelection(selected);
}

// Sample in upper-card coordinates so both chips preserve the colour beneath
// them, including multi-colour boundaries and transparency checkerboards.
static wxBitmap frosted_filament_chip(FilamentButton* button, const wxColour& base, const wxRect& region)
{
    using namespace Slic3r::GUI::FilamentColorAppearance;
    const wxSize size = button->GetClientSize();
    if (size.x <= 0 || size.y <= 0 || region.IsEmpty()) return wxNullBitmap;
    Appearance appearance{{base.IsOk() ? base : *wxBLACK}, false};
    if (auto* item = dynamic_cast<FilamentItem*>(button->GetParent()))
        appearance = resolve(item->index(), base);
    const wxImage background = bitmap(appearance, size.x, size.y).ConvertToImage();
    const wxImage blurred = background.Blur(std::max(1, button->FromDIP(2)));
    wxImage result(region.width, region.height);
    const double radius = std::max(1, button->FromDIP(2));
    for (int y = 0; y < region.height; ++y) {
        for (int x = 0; x < region.width; ++x) {
            const int sx = std::clamp(region.x + x, 0, size.x - 1);
            const int sy = std::clamp(region.y + y, 0, size.y - 1);
            const double dx = std::max(radius + 1 - (x + 0.5), std::max(x + 0.5 - (region.width - radius - 1), 0.0));
            const double dy = std::max(radius + 1 - (y + 0.5), std::max(y + 0.5 - (region.height - radius - 1), 0.0));
            const bool inside = x >= 1 && y >= 1 && x < region.width - 1 && y < region.height - 1 &&
                                dx * dx + dy * dy <= radius * radius;
            const auto channel = [inside](unsigned char original, unsigned char blur) {
                // A translucent neutral veil gives a subtle frosted finish.
                return inside ? static_cast<unsigned char>((blur * 82 + 128 * 18) / 100) : original;
            };
            result.SetRGB(x, y, channel(background.GetRed(sx, sy), blurred.GetRed(sx, sy)),
                          channel(background.GetGreen(sx, sy), blurred.GetGreen(sx, sy)),
                          channel(background.GetBlue(sx, sy), blurred.GetBlue(sx, sy)));
        }
    }
    return wxBitmap(result);
}

// All labels and icons on a card share one foreground derived from its
// complete appearance, including multi-colour stops and alpha compositing.
static wxColour filament_card_foreground(FilamentButton* button, const wxColour& base)
{
    using namespace Slic3r::GUI::FilamentColorAppearance;
    Appearance appearance{{base.IsOk() ? base : *wxBLACK}, false};
    if (auto* item = dynamic_cast<FilamentItem*>(button->GetParent()))
        appearance = resolve(item->index(), base);
    return foreground(appearance);
}

void FilamentButton::draw_menu_area(wxDC& dc)
{
    const wxRect plate = menu_plate_rect();
    if (plate.IsEmpty())
        return;

    const wxBitmap background = frosted_filament_chip(this, m_back_color, plate);
    if (!background.IsOk()) return;
    const wxColour fg = filament_card_foreground(this, m_back_color);

    const int diameter = std::max(2, FromDIP(3));
    const int spacing  = std::max(diameter + 1, FromDIP(5));

    // Render into a bitmap through a wxGCDC: doRender() is handed a plain wxDC on
    // some platforms, and there a tiny DrawEllipse() degenerates into a cross and
    // rounded corners come out jagged.
    wxBitmap bmp(plate.GetWidth(), plate.GetHeight());
    {
        wxMemoryDC mem(bmp);
        if (!mem.IsOk())
            return;

        mem.DrawBitmap(background, 0, 0);

        wxGCDC gdc(mem);
        // Light rim + slightly darker fill, matching the CFS block.
        wxRect inner(0, 0, plate.GetWidth(), plate.GetHeight());
        inner.Deflate(1, 1);
        // Hover and normal outlines share exactly the same geometry; hovering
        // changes only the colour, so the border no longer jumps outwards.
        const wxColour border = m_top_hover_region == 3 ? fg : wxColour(fg.Red(), fg.Green(), fg.Blue(), 85);
        gdc.SetPen(wxPen(border, 1));
        gdc.SetBrush(*wxTRANSPARENT_BRUSH);
        gdc.DrawRoundedRectangle(inner, FromDIP(2));

        // Three dots, centred on the plate.
        gdc.SetPen(*wxTRANSPARENT_PEN);
        gdc.SetBrush(wxBrush(fg));
        const int cx = plate.GetWidth() / 2;
        const int cy = plate.GetHeight() / 2;
        for (int i = -1; i <= 1; ++i)
            gdc.DrawEllipse(cx + i * spacing - diameter / 2, cy - diameter / 2, diameter, diameter);

        mem.SelectObject(wxNullBitmap);
    }

    dc.DrawBitmap(bmp, plate.GetLeft(), plate.GetTop());
}
void FilamentButton::draw_top_hover_area(wxDC& dc)
{
    if (!m_menu_area_enabled || m_top_hover_region != 1)
        return;

    // Without CFS, highlight the upper half only outside the menu hover region.
    // Keep the number's layout and the separate CFS hover regions unchanged.
    wxRect rect = m_sync_box_filament ? label_area_rect() : GetClientRect();
    if (rect.IsEmpty())
        return;

    rect.Deflate(1, 1);
    const wxColour border = filament_card_foreground(this, m_back_color);
    dc.SetPen(wxPen(border, 1));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRoundedRectangle(rect, FromDIP(2));
}

const wxColour& FilamentButton::child_background_colour() const
{
    // Per the design the inner blocks are a darkened shade of the filament colour
    // in both states, so the synced spool colour is not used as a separate fill.
    return m_back_color;
}

wxBrush FilamentButton::make_transparency_brush(int tile_dip) const
{
    int tile = FromDIP(tile_dip);
    if (tile < 2)
        tile = 2;
    const int S = tile * 2;
    wxBitmap bmp(S, S);
    wxMemoryDC mem(bmp);
    mem.SetBackground(*wxWHITE_BRUSH);
    mem.Clear();
    const wxColour c1(220, 220, 220);
    const wxColour c2(180, 180, 180);
    for (int y = 0; y < S; y += tile) {
        for (int x = 0; x < S; x += tile) {
            const bool pick1 = (((x / tile) + (y / tile)) % 2) == 0;
            mem.SetPen(wxPen(pick1 ? c1 : c2));
            mem.SetBrush(wxBrush(pick1 ? c1 : c2));
            mem.DrawRectangle(x, y, tile, tile);
        }
    }
    mem.SelectObject(wxNullBitmap);
    wxBrush brush(bmp);
    brush.SetStyle(wxBRUSHSTYLE_STIPPLE);
    return brush;
}

void FilamentButton::SetCornerRadius(double radius)
{
	this->m_radius = radius;
	Refresh();
}

void FilamentButton::SetBorderWidth(int width)
{
	m_border_width = width;
	Refresh();
}

void FilamentButton::SetColor(wxColour bk_color)
{
	this->m_back_color = bk_color;
	Refresh();
}

void FilamentButton::SetIcon(wxString dark_icon, wxString light_icon) { 
	// Keep the lower-row chevron compact; its click target remains the full row.
	m_dark_img  = ScalableBitmap(this, dark_icon.ToStdString(), 6);
    m_light_img = ScalableBitmap(this, light_icon.ToStdString(), 6);
    Refresh();
}

void FilamentButton::SetLabel(wxString lb)
{
    m_label = lb;
}

wxString FilamentButton::getLabel() 
{ 
    return m_label;
}

void FilamentButton::SetLabelTopLeft(bool top_left)
{
    if (m_label_top_left == top_left)
        return;
    m_label_top_left = top_left;
    Refresh();
}

void FilamentButton::mouseDown(wxMouseEvent& event)
{
	// Clicks on the "..." area open the overflow menu instead of the block's own
	// action (colour dialog / preset list).
	if (menu_area_rect().Contains(event.GetPosition())) {
		show_menu();
		return;
	}

	event.Skip();
	if (!HasCapture())
		CaptureMouse();
}

void FilamentButton::mouseReleased(wxMouseEvent& event)
{
	event.Skip();
	if (HasCapture())
		ReleaseMouse();

	if (menu_area_rect().Contains(event.GetPosition()))
		return;

	if (wxRect({ 0, 0 }, GetSize()).Contains(event.GetPosition()))
	{
		wxCommandEvent event(wxEVT_BUTTON, GetId());
		event.SetEventObject(this);
		GetEventHandler()->ProcessEvent(event);
	}
}

void FilamentButton::eraseEvent(wxEraseEvent& evt)
{
#ifdef __WXMSW__
	wxDC* dc = evt.GetDC();
	wxSize size = GetSize();
	wxClientDC dc2(GetParent());
	dc->Blit({ 0, 0 }, size, &dc2, GetPosition());
#endif
}

void FilamentButton::update_child_button_size()
{
    layout_child_windows();

    m_bitmap = create_scaled_bitmap("switch_cfs_tip", this, 16);
    Refresh();      // 强制重绘
}

void FilamentButton::OnChildButtonClick(wxMouseEvent& event)
{
    // Trigger the BoxColorPopPanel popup
    wxRect  buttonRect = this->GetScreenRect();
    wxPoint popupPosition(buttonRect.GetLeft(), buttonRect.GetBottom());

    FilamentItem* parentItem          = dynamic_cast<FilamentItem*>(GetParent());
    int           filament_item_index = -1;
    if (parentItem) {
        filament_item_index = parentItem->index();
    }

    Slic3r::GUI::BoxColorSelectPopupData* popup_data = new Slic3r::GUI::BoxColorSelectPopupData();
    popup_data->popup_position      = popupPosition;
    popup_data->filament_item_index = filament_item_index;

    wxCommandEvent tmp_event(Slic3r::GUI::EVT_ON_SHOW_BOX_COLOR_SELECTION, GetId());
    tmp_event.SetClientData(popup_data);
    wxPostEvent(Slic3r::GUI::wxGetApp().plater(), tmp_event);

    // Stop the event from propagating to the parent
    event.StopPropagation();
}

void FilamentButton::OnChildButtonPaint(wxPaintEvent& event)
{
    wxPaintDC dc(m_child_button);
    wxSize size = m_child_button->GetSize();

    const wxRect rect(wxPoint(0, 0), size);
    if (rect.IsEmpty()) return;
    const wxBitmap background = frosted_filament_chip(this, m_back_color, wxRect(m_child_button->GetPosition(), size));
    if (!background.IsOk()) return;
    const wxColour fg = filament_card_foreground(this, m_back_color);
    wxBitmap bmp;
#ifdef __WXOSX__
    if (!bmp.CreateWithDIPSize(size, m_child_button->GetContentScaleFactor()))
        return;
#else
    if (!bmp.Create(size.GetWidth(), size.GetHeight()))
        return;
#endif
    {
        wxMemoryDC mem(bmp);
        if (!mem.IsOk())
            return;

        mem.DrawBitmap(background, 0, 0);

        wxGCDC gdc(mem);
        // Light rim + slightly darker fill, per the design. Inset by half the pen
        // width so the 1px stroke stays fully inside the window.
        wxRect plate = rect;
        plate.Deflate(1, 1);
        // Use the same inset rectangle for normal and hover states. Only the
        // border colour changes, keeping the outline perfectly aligned.
        const wxColour border = m_top_hover_region == 2 ? fg : wxColour(fg.Red(), fg.Green(), fg.Blue(), 85);
        gdc.SetPen(wxPen(border, 1));
        gdc.SetBrush(*wxTRANSPARENT_BRUSH);
        gdc.DrawRoundedRectangle(plate, FromDIP(2));

        // Keep text and icon centred inside their respective halves of the
        // visible inner border, not against the child window's outer pixels.
        wxRect content_rect = plate;
        content_rect.Deflate(1, 1);
        const int left_width = content_rect.GetWidth() / 2;
        const wxRect leftRect(content_rect.GetLeft(), content_rect.GetTop(),
                              left_width, content_rect.GetHeight());
        if (!m_sync_filament_label.IsEmpty()) {
            wxFont label_font = GetFont();
            label_font.SetPointSize(Label::Body_12.GetPointSize());
#ifdef __WXMSW__
            // Match the number/material owner-draw path: wxFont point sizes are
            // not automatically rescaled here when moving between monitors.
            label_font = label_font.Scaled(GetDPIScaleFactor());
#endif
            label_font.SetStyle(wxFONTSTYLE_NORMAL);
            label_font.SetWeight(wxFONTWEIGHT_NORMAL);
            label_font.SetUnderlined(false);
            gdc.SetFont(label_font);
            gdc.SetTextForeground(fg);
            gdc.DrawLabel(m_sync_filament_label, leftRect, wxALIGN_CENTER);
        }

        if (m_bitmap.IsOk()) {
            const wxRect rightRect(content_rect.GetLeft() + left_width,
                                   content_rect.GetTop(),
                                   content_rect.GetWidth() - left_width,
                                   content_rect.GetHeight());
            // Pure black has a heavier apparent stroke than white at this size.
            // Reduce only the dark variant's opacity to balance both states.
            const double icon_alpha = fg == *wxBLACK ? 0.70 : 1.0;
            const wxBitmap icon = TintBitmap(m_bitmap, fg, icon_alpha);
            gdc.DrawBitmap(icon,
                           rightRect.GetLeft() + (rightRect.GetWidth() - icon.GetScaledWidth()) / 2,
                           rightRect.GetTop() + (rightRect.GetHeight() - icon.GetScaledHeight()) / 2,
                           true);
        }

        // Destroy the graphics context before the memory DC releases its bitmap.
    }

    dc.DrawBitmap(bmp, 0, 0);
}

void FilamentButton::paintEvent(wxPaintEvent& evt)
{
	wxPaintDC dc(this);
	render(dc);
}

void FilamentButton::render(wxDC& dc)
{
#ifdef __WXMSW__
	if (m_radius == 0) {
		doRender(dc);
		return;
	}

	wxSize size = GetSize();
	if (size.x <= 0 || size.y <= 0)
		return;
	wxMemoryDC memdc(&dc);
	if (!memdc.IsOk()) {
		doRender(dc);
		return;
	}
	wxBitmap bmp(size.x, size.y);
	memdc.SelectObject(bmp);
	//memdc.Blit({0, 0}, size, &dc, {0, 0});
	memdc.SetBackground(m_back_color);
	memdc.Clear();
	{
		wxGCDC dc2(memdc);
		doRender(dc2);
	}

	memdc.SelectObject(wxNullBitmap);
	dc.DrawBitmap(bmp, 0, 0);
#elif defined(__APPLE__)
    wxSize size = GetSize();
    if (size.x <= 0 || size.y <= 0)
        return;

    wxBitmap bmp;
    if (!bmp.CreateWithDIPSize(size, GetContentScaleFactor())) {
        doRender(dc);
        return;
    }

    wxMemoryDC memdc(bmp);
    if (!memdc.IsOk()) {
        doRender(dc);
        return;
    }

    const wxColour background = m_back_color.IsOk() ? m_back_color : GetParent()->GetBackgroundColour();
    memdc.SetBackground(wxBrush(background));
    memdc.Clear();
    doRender(memdc);
    memdc.SelectObject(wxNullBitmap);
    dc.DrawBitmap(bmp, 0, 0);
#else
	doRender(dc);
#endif
}

void FilamentButton::update_sync_box_state(bool sync, const wxString& box_filament_name)
{
	m_sync_box_filament = sync;
	if(box_filament_name.IsEmpty()) {
		m_sync_filament_label = "CFS";
	}
	else {
		m_sync_filament_label = box_filament_name;  // "1A" or "1B" or "1C" or "1D"
	}

	if(!m_sync_box_filament) {
		m_child_button->SetBackgroundColour(*wxWHITE);
	}

    const DM::Device device = DM::DataCenter::Ins().get_current_device_data();
    const bool is_k3 = (device.valid && device.model == "F039") ||
        creality::is_creality_k3_printer_from_string(
            Slic3r::GUI::wxGetApp().preset_bundle->printers.get_edited_preset().config.opt_string("printer_model"));
    m_child_button->Show(m_sync_box_filament && !is_k3);
    Refresh();
}

void FilamentButton::update_child_button_color(const wxColour& color)
{
	m_child_button->SetBackgroundColour(color);
	m_child_button->Refresh();
}
void FilamentButton::resetCFS(bool bCFS)
{
    if (bCFS)
        m_sync_filament_label = "CFS";
    m_child_button->Refresh();
}

void FilamentButton::doRender(wxDC& dc)
{
	wxSize size = GetSize();
	int states = m_state_handler.states();
	wxRect rc(0, 0, size.x, size.y);

	if (!m_menu_area_enabled &&
        (FilamentButtonStateHandler::State) states == FilamentButtonStateHandler::State::Hover)
	{
        if(m_back_color .IsOk() && m_back_color.Alpha() == 0)
            dc.SetPen(wxPen(wxColour("#000000"), m_border_width));
        else
            dc.SetPen(wxPen(filament_card_foreground(this, m_back_color), m_border_width));
	}
	else
	{
        if(m_back_color .IsOk() && m_back_color.Alpha() == 0)
            dc.SetPen(wxPen(wxColour("#FFFFFF"), m_border_width));
        else
		    dc.SetPen(wxPen(m_back_color, m_border_width));
	}

	// Background brush: if m_back_color is fully transparent, fill with checkerboard (~10 DIP squares).
	if (m_back_color.IsOk() && m_back_color.Alpha() == 0)
		dc.SetBrush(make_transparency_brush(10));
	else
		dc.SetBrush(wxBrush(m_back_color));

	if (m_radius == 0 || (m_back_color .IsOk() && m_back_color.Alpha() == 0)) {
		dc.DrawRectangle(rc);
	}
	else {
		dc.DrawRoundedRectangle(rc, m_radius - m_border_width);
	}

	// Both halves use the same horizontal colour stops.
    if (auto* item = dynamic_cast<FilamentItem*>(GetParent())) {
        const auto appearance = Slic3r::GUI::FilamentColorAppearance::resolve(item->index(), m_back_color);
        if (appearance.special() && size.x > 0 && size.y > 0) {
            dc.DrawBitmap(Slic3r::GUI::FilamentColorAppearance::bitmap(appearance, size.x, size.y), 0, 0);
            // The bitmap covers the original outline, so restore it on top.
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            if (m_radius == 0 || m_back_color.Alpha() == 0)
                dc.DrawRectangle(rc);
            else
                dc.DrawRoundedRectangle(rc, m_radius - m_border_width);
        }
    }

    if (!m_label.IsEmpty()) {
	        int width, height;
	        wxFont basic_font = dc.GetFont();
	        basic_font.SetPointSize(Label::Body_13.GetPointSize());
#ifdef __WXMSW__
            basic_font = basic_font.Scaled(GetDPIScaleFactor());
#endif
	        basic_font.SetWeight(wxFONTWEIGHT_NORMAL);
	        dc.SetFont(basic_font);

        dc.GetTextExtent(m_label, &width, &height);

        int panelWidth, panelHeight;
        GetSize(&panelWidth, &panelHeight);

    	int leftHalfWidth = panelWidth / 2;

        int x = (leftHalfWidth - width) / 2;
        int y = (panelHeight - height) / 2;

		if (m_dark_img.bmp().IsOk() && m_light_img.bmp().IsOk()) {
            x = (panelWidth - 6 - width) / 2;
		}

		if (m_label_top_left) {
            // Centre the number inside the left-quarter interaction outline.
            wxRect label_rect = label_area_rect();
            label_rect.Deflate(1, 1);
            x = label_rect.GetLeft() + (label_rect.GetWidth() - width) / 2;
            y = label_rect.GetTop() + (label_rect.GetHeight() - height) / 2 - FromDIP(1);
		}

        if (m_back_color.IsOk() && m_back_color.Alpha() == 0)
            dc.SetTextForeground(*wxBLACK);
        else
            dc.SetTextForeground(filament_card_foreground(this, m_back_color));
        dc.DrawText(m_label, wxPoint(x, y));
    }

	if (m_dark_img.bmp().IsOk() && m_light_img.bmp().IsOk()) {
        const bool is_transparent_bg = (m_back_color.IsOk() && m_back_color.Alpha() == 0);
        const wxBitmap& icon_bmp = is_transparent_bg ? m_dark_img.bmp()
                                  : (filament_card_foreground(this, m_back_color) == *wxBLACK ? m_dark_img.bmp() : m_light_img.bmp());

        // Vertically centred, and horizontally just right of the label.
        int x = size.GetWidth() - icon_bmp.GetWidth() - FromDIP(4);
        int y = (size.GetHeight() - icon_bmp.GetHeight()) / 2;

		if (!m_label.IsEmpty())
		{
            int width, height;
            dc.GetTextExtent(m_label, &width, &height);

			int panelWidth, panelHeight;
            GetSize(&panelWidth, &panelHeight);

			x = std::min(x, (panelWidth + width) / 2 + FromDIP(2));
		}

        dc.DrawBitmap(icon_bmp, wxPoint(x, y));
    }

    draw_menu_area(dc);
    draw_top_hover_area(dc);

    const DM::Device device = DM::DataCenter::Ins().get_current_device_data();
    const bool is_k3 = (device.valid && device.model == "F039") ||
        creality::is_creality_k3_printer_from_string(
            Slic3r::GUI::wxGetApp().preset_bundle->printers.get_edited_preset().config.opt_string("printer_model"));
    m_child_button->Show(m_sync_box_filament && !is_k3);
}

/*
* FilamentPopPanel
*/

 FilamentPopPanel::FilamentPopPanel(wxWindow* parent, int index)
	: PopupWindow(parent, wxBORDER_SIMPLE  | wxPU_CONTAINS_CONTROLS)
{
    Freeze();

	int em = 1;
    m_index = index;
	
    m_bg_color = wxColour(255, 255, 255); // 背景色统一
	this->SetBackgroundColour(m_bg_color);

	m_sizer_main = new wxBoxSizer(wxHORIZONTAL);
	{
	        m_filamentCombox = new Slic3r::GUI::PlaterPresetComboBox(this, Slic3r::Preset::TYPE_FILAMENT);
	        // Push the list clear of the block instead of butting it right against
	        // the anchor, otherwise it clips the lower half by a few pixels.
	        m_filamentCombox->GetDropDown().setDrapDownGap(FromDIP(FILAMENT_LIST_GAP_DIP));
            m_filamentCombox->EnableAutoPopupDirection(false);
	        m_filamentCombox->set_filament_idx(index);
	        //m_filamentCombox->SetMaxSize(wxSize(FromDIP(200), -1));
	        m_filamentCombox->update();
        m_filamentCombox->clr_picker->Hide();
        m_filamentCombox->Bind(wxEVT_RIGHT_UP, [](wxMouseEvent& event) {
            // The overflow menu is available only through the "..." button.
            event.Skip(false);
        });

        m_filamentCombox->setSelectedItemCb([&](int selectedItem) -> void { 
            if (m_pFilamentItem != nullptr) {
                m_pFilamentItem->resetCFS(true);
                }
            });
        m_filamentCombox->Bind(wxEVT_LEFT_DCLICK, [](wxMouseEvent& e) {
            e.Skip(false); 
        });
			// filament combox
	        wxSizerItem* item = m_sizer_main->Add(m_filamentCombox, 1, wxEXPAND);
	        m_filamentCombox->SetMinSize(wxSize(FromDIP(40), FromDIP(FILAMENT_LIST_ANCHOR_HEIGHT_DIP)));
	        m_filamentCombox->SetMaxSize(wxSize(-1, FromDIP(FILAMENT_LIST_ANCHOR_HEIGHT_DIP)));
	        m_sizer_main->SetItemMinSize(m_filamentCombox, wxSize(FromDIP(40), FromDIP(FILAMENT_LIST_ANCHOR_HEIGHT_DIP)));
	        item->SetProportion(1);

	}
    Slic3r::GUI::wxGetApp().UpdateDarkUIWin(this);
		SetSizer(m_sizer_main);
		Layout();

		Thaw();
	}

FilamentPopPanel::~FilamentPopPanel() {}

void FilamentPopPanel::Dismiss()
{
    // This panel is never shown; closing means closing the preset list.
    if (m_filamentCombox && m_filamentCombox->is_drop_down())
        ComboBox::DismissActiveDropDown();
}

	void FilamentPopPanel::sys_color_changed()
	{
		m_filamentCombox->sys_color_changed();
	}

    void FilamentPopPanel::msw_rescale(wxWindow* dpi_reference)
    {
        Freeze();

        Slic3r::GUI::wxGetApp().UpdateDarkUIWin(this);

        wxWindow* reference = dpi_reference ? dpi_reference : static_cast<wxWindow*>(this);
        if (m_filamentCombox) {
            m_filamentCombox->msw_rescale();
            m_filamentCombox->SetMinSize(wxSize(wxWindow::FromDIP(40, reference),
                                                wxWindow::FromDIP(FILAMENT_LIST_ANCHOR_HEIGHT_DIP, reference)));
            m_filamentCombox->SetMaxSize(wxSize(-1, wxWindow::FromDIP(FILAMENT_LIST_ANCHOR_HEIGHT_DIP, reference)));
            m_filamentCombox->GetDropDown().setDrapDownGap(wxWindow::FromDIP(FILAMENT_LIST_GAP_DIP, reference));
            if (m_sizer_main)
                m_sizer_main->SetItemMinSize(
                    m_filamentCombox,
                    wxSize(wxWindow::FromDIP(40, reference),
                           wxWindow::FromDIP(FILAMENT_LIST_ANCHOR_HEIGHT_DIP, reference)));
        }

        Layout();
        InvalidateBestSize();
        Thaw();
    }

void FilamentPopPanel::PopupPresetList()
{
    if (!m_filamentCombox)
        return;

    // This panel stays hidden for its whole lifetime, so use the visible
    // filament block as the DPI authority on every open.
    wxWindow* reference = m_pFilamentItem ? static_cast<wxWindow*>(m_pFilamentItem)
                                          : static_cast<wxWindow*>(this);
    msw_rescale(reference);

    // Give the (never shown) panel a sane size so the hosted combobox gets laid
    // out; the list itself is placed via SetDropDownAnchor(), not from here.
    SetSize(wxSize(wxWindow::FromDIP(FILAMENT_LIST_WIDTH_DIP, reference),
                   wxWindow::FromDIP(FILAMENT_LIST_ANCHOR_HEIGHT_DIP, reference)));
    Layout();
    m_filamentCombox->ForceDropdownOpen();
}

/*
FilamentItem
*/

BEGIN_EVENT_TABLE(FilamentItem, wxPanel)
EVT_PAINT(FilamentItem::paintEvent)
END_EVENT_TABLE()
 
FilamentItem::FilamentItem(wxWindow* parent, const Data& data, const wxSize& size/*=wxSize(100, 42)*/)
{
    m_data = data;

    m_preset_bundle = Slic3r::GUI::wxGetApp().preset_bundle;
    std::string filament_type;
    // Defensive: determine a safe preset name before lookup.
    std::string preset_name;
    const size_t preset_count = m_preset_bundle->filament_presets.size();
    if (m_data.index < preset_count) {
        preset_name = m_preset_bundle->filament_presets[m_data.index];
    } else {
        BOOST_LOG_TRIVIAL(warning) << __FUNCTION__
            << ": index " << m_data.index
            << " out of bounds (size " << preset_count
            << "), falling back to selected filament '"
            << m_preset_bundle->filaments.get_selected_preset_name() << "'";
        preset_name = m_preset_bundle->filaments.get_selected_preset_name();
    }

    Slic3r::Preset* preset = m_preset_bundle->filaments.find_preset(preset_name);
    if (!preset) {
        BOOST_LOG_TRIVIAL(warning) << __FUNCTION__
            << ": preset '" << preset_name << "' not found; using 'Default Filament'";
        preset = m_preset_bundle->filaments.find_preset("Default Filament");
    }
    if (preset) {
        filament_type = preset_display_name_for_filament(*preset);
    } else {
        BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << ": 'Default Filament' preset not found; using generic label";
        filament_type = "Filament"; // final fallback label
    }

    std::string filament_color = m_preset_bundle->project_config.opt_string("filament_colour", (unsigned int) m_data.index);
    m_bk_color                 = Slic3r::GUI::FilamentColorAppearance::parse(filament_color);

    m_checked_border_color = wxColour("#1FCA63");

	m_small_state = data.small_state;
	wxSize sz(size);
	if (m_small_state)
		sz.SetWidth(sz.GetWidth() / 2);

	wxPanel::Create(parent, wxID_ANY, wxDefaultPosition, sz, 0);

	wxSize btn_size;
    // Reserve the border on both sides and the gap between the two rows.
    const int inset = std::max(1, static_cast<int>(std::ceil((m_radius + m_border_width) * 0.5)));
    const int row_gap = std::max(0, m_radius / 2);
    const int usable_height = std::max(2, sz.GetHeight() - 2 * inset - row_gap);
    btn_size.SetHeight(std::max(1, static_cast<int>(std::lround(usable_height * FILAMENT_TOP_ROW_HEIGHT_RATIO))));
	btn_size.SetWidth(std::max(1, sz.GetWidth() - 2 * inset));
    wxSize param_btn_size(btn_size.GetWidth(), std::max(1, usable_height - btn_size.GetHeight()));

	m_sizer = new wxBoxSizer(wxVERTICAL);

    auto suppress_popup_release = std::make_shared<bool>(false);

	{//color btn
		// Two-digit index per the design: 1 -> "01".
		m_btn_color = new FilamentButton(this, wxString::Format("%02d", data.index + 1), wxPoint(inset, inset), btn_size);
		m_btn_color->SetCornerRadius(this->m_radius);
		m_btn_color->SetColor(m_bk_color);
		// Number goes to the top-left corner; the CFS block is centred and the
		// overflow menu takes the top-right corner.
		m_btn_color->SetLabelTopLeft(true);
		m_btn_color->enable_menu_button(true);
        //InitContextMenu();
        m_btn_color->Bind(wxEVT_RIGHT_UP, [](wxMouseEvent& event) {
            // The overflow menu is available only through the "..." button.
            event.Skip(false);
        });


		m_btn_color->Bind(wxEVT_BUTTON, [this](wxEvent&) {
			wxCommandEvent event(wxEVT_BUTTON, GetId());
			event.SetEventObject(this);
			GetEventHandler()->ProcessEvent(event);

			m_checked_state = true;
			Refresh();

            auto apply_color = [this](const std::vector<wxColour>& selected_colors, bool is_gradient) {
                if (selected_colors.empty() || !selected_colors.front().IsOk()) return;
                const wxColour& selected_color = selected_colors.front();
                Slic3r::DynamicPrintConfig* cfg = &Slic3r::GUI::wxGetApp().preset_bundle->project_config;
                auto* option = cfg->option<Slic3r::ConfigOptionStrings>("filament_colour");
                if (option == nullptr || m_data.index >= option->values.size()) return;

                auto* colors = static_cast<Slic3r::ConfigOptionStrings*>(option->clone());
                colors->values[m_data.index] = wxString::Format("#%02X%02X%02X%02X", selected_color.Red(), selected_color.Green(), selected_color.Blue(), selected_color.Alpha()).ToStdString();
                auto* multi = static_cast<Slic3r::ConfigOptionStrings*>(cfg->option("filament_multi_colour")->clone());
                auto* types = static_cast<Slic3r::ConfigOptionStrings*>(cfg->option("filament_colour_type")->clone());
                multi->values.resize(std::max(multi->values.size(), size_t(m_data.index + 1)));
                types->values.resize(std::max(types->values.size(), size_t(m_data.index + 1)), "1");
                std::string serialized;
                for (const auto& color : selected_colors) {
                    if (!serialized.empty()) serialized += ' ';
                    serialized += wxString::Format("#%02X%02X%02X%02X", color.Red(), color.Green(), color.Blue(), color.Alpha()).ToStdString();
                }
                multi->values[m_data.index] = serialized;
                types->values[m_data.index] = is_gradient ? "0" : "1";
                Slic3r::DynamicPrintConfig cfg_new = *cfg;
                cfg_new.set_key_value("filament_colour", colors);
                cfg_new.set_key_value("filament_multi_colour", multi);
                cfg_new.set_key_value("filament_colour_type", types);
                cfg->apply(cfg_new);
                Slic3r::GUI::wxGetApp().plater()->update_project_dirty_from_presets();
                Slic3r::GUI::wxGetApp().preset_bundle->export_selections(*Slic3r::GUI::wxGetApp().app_config);
                Slic3r::GUI::wxGetApp().plater()->on_config_change(cfg_new);
                Slic3r::GUI::wxGetApp().plater()->reset_scene_filament_source_snapshot();

                m_bk_color = selected_color;
                m_btn_color->SetColor(m_bk_color);
                m_btn_param_list->SetColor(m_bk_color);

                auto* changed = new wxCommandEvent(Slic3r::GUI::EVT_FILAMENT_COLOR_CHANGED);
                changed->SetInt(m_data.index);
                wxQueueEvent(Slic3r::GUI::wxGetApp().plater(), changed);
            };

            const Slic3r::Preset* selected_preset = nullptr;
            if (m_data.index < m_preset_bundle->filament_presets.size())
                selected_preset = m_preset_bundle->filaments.find_preset(
                    m_preset_bundle->filament_presets[m_data.index]);

            // Presence in Creality's official color table is the source of truth. This also
            // supports user presets inherited from an official Creality filament preset.
            if (selected_preset != nullptr && !selected_preset->filament_id.empty()) {
                const auto current_appearance =
                    Slic3r::GUI::FilamentColorAppearance::resolve(m_data.index, m_bk_color);
                Slic3r::GUI::OfficialFilamentColorDialog official_dialog(
                    this, selected_preset->filament_id, m_bk_color,
                    Slic3r::GUI::from_u8(preset_display_name_for_filament(*selected_preset)),
                    current_appearance.colors, current_appearance.gradient);
                if (official_dialog.IsDataLoaded()) {
                    if (official_dialog.ShowModal() == wxID_OK)
                        apply_color(official_dialog.GetSelectedColours(), official_dialog.IsSelectedGradient());
                    Refresh();
                    return;
                }
            }

			wxColourData color_data;
            color_data.SetColour(m_bk_color);
            color_data.SetChooseFull(true);
            color_data.SetChooseAlpha(false);

			std::vector<std::string> custom_colors = Slic3r::GUI::wxGetApp().app_config->get_custom_color_from_config();
            for (size_t i = 0; i < custom_colors.size() && i < CUSTOM_COLOR_COUNT; ++i)
                color_data.SetCustomColour(static_cast<int>(i), string_to_wxColor(custom_colors[i]));

			wxColourDialog dialog(Slic3r::GUI::wxGetApp().plater(), &color_data);
            dialog.Center();
            dialog.SetTitle(_L("Please choose the filament colour"));
			if (dialog.ShowModal() == wxID_OK) {
				color_data = dialog.GetColourData();
                custom_colors.resize(CUSTOM_COLOR_COUNT);
                for (int i = 0; i < CUSTOM_COLOR_COUNT; ++i)
                    custom_colors[i] = color_to_string(color_data.GetCustomColour(i));
                Slic3r::GUI::wxGetApp().app_config->save_custom_color_to_config(custom_colors);
                apply_color({color_data.GetColour()}, false);
			}

			Refresh();
		});
	}

	{//param btn
        m_btn_param_list = new FilamentButton(this, wxString(filament_type), wxPoint(inset, inset + row_gap + btn_size.GetHeight()), param_btn_size);
		m_btn_param_list->SetCornerRadius(this->m_radius);
		m_btn_param_list->SetColor(m_bk_color);
        m_btn_param_list->SetIcon("downBtn_black", "downBtn_white");
        m_btn_param_list->Bind(wxEVT_LEFT_UP, [this, suppress_popup_release](wxMouseEvent& event) {
            if (*suppress_popup_release) {
                *suppress_popup_release = false;
                if (m_btn_param_list->HasCapture())
                    m_btn_param_list->ReleaseMouse();
                return;
            }
            event.Skip();
        });
        m_btn_param_list->Bind(wxEVT_LEAVE_WINDOW, [suppress_popup_release](wxMouseEvent& event) {
            // Releasing outside the anchor must not suppress a later click.
            *suppress_popup_release = false;
            event.Skip();
        });

        m_btn_param_list->Bind(wxEVT_RIGHT_UP, [](wxMouseEvent& event) {
            // The overflow menu is available only through the "..." button.
            event.Skip(false);
        });

			// Clicking the lower half opens the preset list directly.
			m_btn_param_list->Bind(wxEVT_BUTTON, [this, suppress_popup_release](wxEvent& e) {
                const bool list_shown = m_popPanel && m_popPanel->m_filamentCombox &&
                                        m_popPanel->m_filamentCombox->is_drop_down();
                // A press on the anchor already dismissed the dropdown.
                // Consume its release regardless of how long the mouse was held.
                if (*suppress_popup_release) {
                    *suppress_popup_release = false;
                    return;
                }

                if (list_shown) {
                    ComboBox::DismissActiveDropDown();
                } else if (m_popPanel) {
                    layout_filament_popup(this, m_popPanel, false);
                    m_popPanel->PopupPresetList();

			    m_btn_param_list->SetIcon("upBtn_black", "upBtn_white");
                    m_btn_param_list->Refresh();
                }

				wxCommandEvent event(wxEVT_BUTTON, GetId());
				event.SetEventObject(this);
				GetEventHandler()->ProcessEvent(event);

			m_checked_state = true;
			this->Refresh();
			});
	}

	this->SetSizer(m_sizer);
	m_sizer->Layout();
	//m_sizer->Fit(this);

	m_popPanel = new FilamentPopPanel(this, data.index);
    m_popPanel->setFilamentItem(this);
    // The preset list closing is what flips the arrow back down now.
    if (m_popPanel->m_filamentCombox) {
        m_popPanel->m_filamentCombox->Bind(wxEVT_COMBOBOX_CLOSEUP, [this, suppress_popup_release](wxCommandEvent& e) {
            e.Skip();
            *suppress_popup_release = wxGetMouseState().LeftIsDown() &&
                m_btn_param_list->GetScreenRect().Contains(wxGetMousePosition());
            m_btn_param_list->SetIcon("downBtn_black", "downBtn_white");
            m_btn_param_list->Refresh();
        });
    }

	//update filament type.
	m_popPanel->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent& e) { 
		Slic3r::GUI::wxGetApp().sidebar().GetEventHandler()->ProcessEvent(e);
		});
        
}

FilamentItem::~FilamentItem()
{
    if (m_popPanel) {
        m_popPanel->Dismiss();
        m_popPanel->Destroy();
        m_popPanel = nullptr;
    }
}

void FilamentItem::set_checked(bool checked /*= true*/)
{
	m_checked_state = checked;
	this->Refresh(false);
}

bool FilamentItem::is_checked()
{
	return m_checked_state;
}

void FilamentItem::update_box_sync_state(bool sync, const wxString& box_filament_name)
{
	if(m_btn_color)
	{
		m_btn_color->update_sync_box_state(sync, box_filament_name);
        m_data.box_filament_name = box_filament_name.ToStdString();
	}
}


void FilamentItem::update_box_sync_color(const std::string& sync_color)
{
	if(m_btn_color)
	{
		m_btn_color->update_child_button_color(RemotePrint::Utils::hex_string_to_wxcolour(sync_color));
	}

}
void FilamentItem::resetCFS(bool bCFS)
{
    if (m_btn_color)
        m_btn_color->resetCFS(bCFS);
}

void FilamentItem::edit_preset()
{
    if (!m_popPanel || !m_popPanel->m_filamentCombox)
        return;

    // The preset list would stay on top of the settings dialog, so close it first.
    m_popPanel->Dismiss();

    Slic3r::GUI::wxGetApp().sidebar().set_edit_filament(-1);
    if (m_popPanel->m_filamentCombox->switch_to_tab())
        Slic3r::GUI::wxGetApp().sidebar().set_edit_filament(m_data.index);
}


bool FilamentItem::to_small(bool bSmall /*= true*/)
{
	if (m_small_state == bSmall)
		return false;

	m_small_state = bSmall;

	{
		wxSize sz = m_btn_color->GetSize();
		sz.SetWidth(bSmall ? sz.GetWidth() / 2 - 1: sz.GetWidth() * 2 + FromDIP(1));
		m_btn_color->SetSize(sz);		
	}


	{
		wxSize sz = m_btn_param_list->GetSize();
		sz.SetWidth(bSmall ? sz.GetWidth() / 2 - 1 : sz.GetWidth() * 2 + FromDIP(1));
		m_btn_param_list->SetSize(sz);
	}
	
	//
	{
		wxSize sz = this->GetMinSize();
		sz.SetWidth(bSmall ? sz.GetWidth() / 2 : sz.GetWidth() * 2);
		this->SetMinSize(sz);
	}

 	m_btn_color->Refresh();
 	m_btn_param_list->Refresh();
	this->Refresh();
	return true;
}

void FilamentItem::update_bk_color(const std::string& bk_color)
{
    // get current color
    Slic3r::DynamicPrintConfig* cfg    = &Slic3r::GUI::wxGetApp().preset_bundle->project_config;
    auto                        colors = static_cast<Slic3r::ConfigOptionStrings*>(cfg->option("filament_colour")->clone());
    if(m_data.index >= colors->values.size())
        return;

    colors->values[m_data.index]       = bk_color;
    Slic3r::DynamicPrintConfig cfg_new = *cfg;
    cfg_new.set_key_value("filament_colour", colors);

    cfg->apply(cfg_new);
    Slic3r::GUI::wxGetApp().plater()->update_project_dirty_from_presets();
    Slic3r::GUI::wxGetApp().preset_bundle->export_selections(*Slic3r::GUI::wxGetApp().app_config);
    // update();
    Slic3r::GUI::wxGetApp().plater()->on_config_change(cfg_new);

    m_bk_color = RemotePrint::Utils::hex_string_to_wxcolour(bk_color);
    m_btn_color->SetColor(m_bk_color);
    m_btn_param_list->SetColor(m_bk_color);

	// if(m_sync_box_filament && m_btn_box_filament != nullptr) {
	// 	m_btn_box_filament->SetColor(m_bk_color);
	// }

    wxCommandEvent* evt = new wxCommandEvent(Slic3r::GUI::EVT_FILAMENT_COLOR_CHANGED);
    evt->SetInt(m_data.index);
    wxQueueEvent(Slic3r::GUI::wxGetApp().plater(), evt);
}

std::string FilamentItem::set_filament_selection(const wxString& filament_name, bool notify)
{
    if (filament_name.IsEmpty())
        return {};

    const std::string raw_name = filament_name.ToUTF8().data();
    bool selected = m_popPanel->m_filamentCombox->SetStringSelection(filament_name);
    if (!selected) {
        const std::string preset_name = m_preset_bundle->get_preset_name_by_alias(
            Slic3r::Preset::TYPE_FILAMENT, Slic3r::Preset::remove_suffix_modified(raw_name));
        if (const Slic3r::Preset* preset = m_preset_bundle->filaments.find_preset(preset_name)) {
            selected = m_popPanel->m_filamentCombox->SetStringSelection(from_u8(preset->label(false))) ||
                       (!preset->alias.empty() && m_popPanel->m_filamentCombox->SetStringSelection(from_u8(preset->alias))) ||
                       m_popPanel->m_filamentCombox->SetStringSelection(from_u8(preset->name));
        }
    }
    if (!selected)
        return {};

	const int evt_selection = m_popPanel->m_filamentCombox->GetSelection();
    if (evt_selection < 0)
        return {};

    const std::string selected_label = Slic3r::Preset::remove_suffix_modified(
        into_u8(m_popPanel->m_filamentCombox->GetString(evt_selection)));
    std::string selected_preset_name = m_popPanel->m_filamentCombox->preset_name_for_item(evt_selection);
    for (const Slic3r::Preset& preset : m_preset_bundle->filaments.get_presets()) {
        if (!selected_preset_name.empty())
            break;
        if (preset.is_default || preset.is_system || !preset.is_visible || !preset.is_compatible)
            continue;
        if (Slic3r::Preset::remove_suffix_modified(preset.label(true)) == selected_label ||
            Slic3r::Preset::remove_suffix_modified(preset.label(false)) == selected_label) {
            selected_preset_name = preset.name;
            break;
        }
    }
    if (selected_preset_name.empty())
        selected_preset_name = m_preset_bundle->get_preset_name_by_alias(Slic3r::Preset::TYPE_FILAMENT, selected_label);

    if (notify) {
            wxCommandEvent* evt = new wxCommandEvent(wxEVT_COMBOBOX, m_popPanel->m_filamentCombox->GetId());
            evt->SetEventObject(m_popPanel->m_filamentCombox);
			evt->SetInt(evt_selection);
            wxQueueEvent(&(Slic3r::GUI::wxGetApp().plater()->sidebar()), evt);
    }

    return selected_preset_name;
}

void FilamentItem::update(bool persist_changes)
{
    if(m_preset_bundle->filament_presets.size() <= m_data.index) {
        BOOST_LOG_TRIVIAL(warning) << __FUNCTION__
            << ": index " << m_data.index
            << " out of bounds (size " << m_preset_bundle->filament_presets.size()
            << "), skipping update";
        return;
    }

    auto filament_color = m_preset_bundle->project_config.opt_string("filament_colour", (unsigned int)m_data.index);
    if (filament_color == "\"\"")
        filament_color = "#000000";
    if (persist_changes) {
        Slic3r::DynamicPrintConfig* cfg    = &Slic3r::GUI::wxGetApp().preset_bundle->project_config;
        auto                        colors = static_cast<Slic3r::ConfigOptionStrings*>(cfg->option("filament_colour")->clone());
        if(colors->values.size() <= m_data.index)
            colors->values.resize(m_data.index + 1, "#000000");
        colors->values[m_data.index]       = filament_color;
        Slic3r::DynamicPrintConfig cfg_new = *cfg;
        cfg_new.set_key_value("filament_colour", colors);
        cfg->apply(cfg_new);
        Slic3r::GUI::wxGetApp().plater()->update_project_dirty_from_presets();
        Slic3r::GUI::wxGetApp().preset_bundle->export_selections(*Slic3r::GUI::wxGetApp().app_config);
        Slic3r::GUI::wxGetApp().plater()->on_config_change(cfg_new);
    }

    m_bk_color = wxColor(filament_color);
    m_btn_color->SetColor(m_bk_color);
    m_btn_param_list->SetColor(m_bk_color);

	m_popPanel->m_filamentCombox->update(); 

	std::string filament_type;
    // Rebuilding the combobox may replace a hidden Default Filament entry with
    // the first compatible preset. Resolve that actual selection before
    // refreshing the collapsed button and filament_presets.
    const std::string selected_preset_name =
        set_filament_selection(m_popPanel->m_filamentCombox->GetStringSelection(), false);
    Slic3r::Preset* preset = selected_preset_name.empty() ? nullptr :
        m_preset_bundle->filaments.find_preset(selected_preset_name);
    if (preset == nullptr)
        preset = m_preset_bundle->filaments.find_preset(m_preset_bundle->filament_presets[m_data.index]);
    if (preset)
        filament_type = preset_display_name_for_filament(*preset);
    else {
        BOOST_LOG_TRIVIAL(warning) << __FUNCTION__
            << ": preset '" << m_preset_bundle->filament_presets[m_data.index]
            << "' not found; using 'Default Filament'";
        preset = m_preset_bundle->filaments.find_preset("Default Filament");
        if (preset == nullptr) {
            BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << ": 'Default Filament' preset not found; update aborted";
            return;
        }
    }

    wxString current_selection = from_u8(preset_display_name_for_filament(*preset));
    m_preset_name              = preset->name;

	// Get the button width
    int btn_width = m_btn_param_list->GetSize().GetWidth();
    wxClientDC dc(m_btn_param_list);
    wxFont basic_font = m_btn_param_list->GetFont();
    basic_font.SetPointSize(Label::Body_13.GetPointSize());
    basic_font.SetWeight(wxFONTWEIGHT_NORMAL);
    dc.SetFont(basic_font);

	// Calculate the width of the dropdown icon
    int icon_width = FromDIP(16);// Assume the icon width is 16 pixels
    int max_label_width = btn_width - icon_width - FromDIP(10); // Reserve some padding

	// Check the label width and truncate if necessary
    wxString truncated_label = current_selection;
    int label_width;
    dc.GetTextExtent(truncated_label, &label_width, nullptr);
    if (label_width > max_label_width) {
        while (label_width > max_label_width && truncated_label.length() > 0) {
            truncated_label.RemoveLast();
            dc.GetTextExtent(truncated_label + "...", &label_width, nullptr);
        }
        truncated_label += "...";
    }

	m_btn_param_list->SetLabel(truncated_label);
    m_btn_param_list->SetToolTip(current_selection);
    m_btn_param_list->Refresh();
}

void FilamentItem::sys_color_changed()
{ 
	m_popPanel->sys_color_changed();
}

void FilamentItem::update_button_size()
{
    wxSize sz = GetSize();
    if (m_small_state)
        sz.SetWidth(sz.GetWidth() / 2);

    wxSize btn_size;
    // Reserve the border on both sides and the gap between the two rows.
    const int inset = std::max(1, static_cast<int>(std::ceil((m_radius + m_border_width) * 0.5)));
    const int row_gap = std::max(0, m_radius / 2);
    const int usable_height = std::max(2, sz.GetHeight() - 2 * inset - row_gap);
    btn_size.SetHeight(std::max(1, static_cast<int>(std::lround(usable_height * FILAMENT_TOP_ROW_HEIGHT_RATIO))));
    btn_size.SetWidth(std::max(1, sz.GetWidth() - 2 * inset));
    wxSize param_btn_size(btn_size.GetWidth(), std::max(1, usable_height - btn_size.GetHeight()));

    m_btn_color->SetPosition(wxPoint(inset, inset));
    m_btn_color->SetSize(btn_size);
    m_btn_color->update_child_button_size();

    m_btn_param_list->SetSize(param_btn_size);
    wxPoint param_list_pos(inset, inset + row_gap + btn_size.GetHeight());
    m_btn_param_list->SetPosition(param_list_pos);
}

	void FilamentItem::msw_rescale() 
	{
        if (m_popPanel)
            m_popPanel->msw_rescale(this);
	 
	    wxSize newSize = wxSize(FromDIP(FILAMENT_BTN_WIDTH), FromDIP(FILAMENT_BTN_HEIGHT));
	    SetSize(newSize);
	    update_button_size();

        if (m_btn_param_list) {
            const bool list_shown = m_popPanel && m_popPanel->m_filamentCombox &&
                                    m_popPanel->m_filamentCombox->is_drop_down();
            m_btn_param_list->SetIcon(list_shown ? "upBtn_black" : "downBtn_black",
                                      list_shown ? "upBtn_white" : "downBtn_white");
            m_btn_param_list->Refresh();
        }

        if (m_btn_color)
            m_btn_color->Refresh();

        Layout();
        Refresh();

        if (m_popPanel && m_popPanel->m_filamentCombox && m_popPanel->m_filamentCombox->is_drop_down()) {
            layout_filament_popup(this, m_popPanel, true);
        }
	}

void FilamentItem::paintEvent(wxPaintEvent& evt)
{
	wxPaintDC dc(this);
	wxSize size = this->GetSize();
	if (1) {
		wxRect rc(0, 0, size.x, size.y);

		dc.SetPen(wxPen(m_bk_color, m_border_width));
		dc.SetBrush(wxBrush(m_bk_color));
        
        if (!Slic3r::GUI::wxGetApp().dark_mode() && m_bk_color == wxColour("#FFFFFF"))
            dc.SetPen(wxPen(wxColour("#D0D4DE"), 1, wxPENSTYLE_SOLID));
        if (m_bk_color == wxColour("#00000000"))
        {
            dc.SetPen(wxPen(wxColour("#FFFFFF"), 1, wxPENSTYLE_SOLID));
            dc.SetBrush(wxBrush(wxColour("#FFFFFF")));
        }
            
   
		if (m_radius == 0) {
			dc.DrawRectangle(rc);
		}
		else {
			dc.DrawRoundedRectangle(rc, m_radius - m_border_width);
		}

        if (m_checked_state) {
            const double border_width = std::max(1, FromDIP(2));
            const double inset = border_width * 0.5;
            if (wxGraphicsContext* gc = wxGraphicsContext::Create(dc)) {
                gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
                gc->SetPen(gc->CreatePen(wxGraphicsPenInfo(m_checked_border_color, border_width)));
                gc->SetBrush(*wxTRANSPARENT_BRUSH);
                gc->DrawRoundedRectangle(inset, inset,
                    std::max(0.0, static_cast<double>(size.x) - border_width),
                    std::max(0.0, static_cast<double>(size.y) - border_width),
                    std::max(0.0, static_cast<double>(m_radius - m_border_width)));
                delete gc;
            }
        }
	}
}

int FilamentItem::index()
{
	return m_data.index;
}
wxString FilamentItem::name()
{ 
    if(m_popPanel && m_popPanel->m_filamentCombox)
        return  m_popPanel->m_filamentCombox->GetStringSelection();
    return wxString("");
}
wxString FilamentItem::boxname()
{
    return wxString::FromUTF8(m_data.box_filament_name.c_str());
}
wxColour FilamentItem::color() 
{ 
    return m_bk_color;
}
wxString FilamentItem::preset_name() {
    return m_preset_name;
}
    /*
* FilamentPanel
*/

FilamentPanel::FilamentPanel(wxWindow* parent,
	wxWindowID      id,
	const wxPoint& pos,
	const wxSize& size, long style)
	: wxPanel(parent, id, pos, size, style)
{
	m_sizer = new wxWrapSizer(wxHORIZONTAL, 0);
	m_box_sizer = new wxBoxSizer(wxVERTICAL);
	this->SetSizer(m_box_sizer);

    m_filament_scrolled = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition,
                                                wxDefaultSize, wxVSCROLL | wxTAB_TRAVERSAL);
    m_filament_scrolled->SetBackgroundColour(*wxWHITE);
    // One scroll unit is one card row, so a wheel notch moves the content once.
    // The default helper scrolls several small units and repaints the cards
    // after each one, which is expensive for these owner-drawn child windows.
    m_filament_scrolled->SetScrollRate(0, FromDIP(FILAMENT_BTN_HEIGHT + 8));
    m_filament_scrolled->Bind(wxEVT_MOUSEWHEEL, &FilamentPanel::on_filament_wheel, this);
    m_filament_content = new wxPanel(m_filament_scrolled, wxID_ANY);
    m_filament_content->Bind(wxEVT_MOUSEWHEEL, &FilamentPanel::on_filament_wheel, this);
    m_filament_content->SetBackgroundColour(*wxWHITE);
    m_filament_content->SetSizer(m_sizer);
    Bind(wxEVT_SIZE, [this](wxSizeEvent& event) {
        event.Skip();
        update_scroll_height();
    });

    auto* grouping_sizer = new wxBoxSizer(wxHORIZONTAL);
    grouping_sizer->AddStretchSpacer();
    m_grouping_btn = new Button(this, _L("Filament grouping"));
    m_grouping_btn->SetMinSize(wxSize(FromDIP(132), FromDIP(28)));
    m_grouping_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { (void)show_filament_grouping_dialog(false); });
    grouping_sizer->Add(m_grouping_btn, 0, wxRIGHT | wxBOTTOM, FromDIP(6));
    m_box_sizer->Add(grouping_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(6));
    m_box_sizer->Add(m_filament_scrolled, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(6));
    m_box_sizer->AddSpacer(FromDIP(8));
#ifdef __APPLE__
	m_box_sizer->AddSpacer(FromDIP(20));
#endif
    refresh_filament_grouping_visibility();
}

FilamentPanel::~FilamentPanel()
{
    m_lifetime_token.reset();
}

bool FilamentPanel::supports_filament_nozzle_mapping() const
{
    if (Slic3r::GUI::wxGetApp().preset_bundle == nullptr)
        return false;

    const Slic3r::DynamicPrintConfig& config =
        Slic3r::GUI::wxGetApp().preset_bundle->printers.get_edited_preset().config;
    const auto* option = config.option<Slic3r::ConfigOptionBool>("support_filament_nozzle_mapping");
    return option != nullptr && option->value;
}

size_t FilamentPanel::nozzle_count_for_mapping() const
{
    if (Slic3r::GUI::wxGetApp().preset_bundle == nullptr)
        return 0;

    const Slic3r::DynamicPrintConfig& config =
        Slic3r::GUI::wxGetApp().preset_bundle->printers.get_edited_preset().config;
    const auto* nozzles = config.option<Slic3r::ConfigOptionFloats>("nozzle_diameter");
    return nozzles != nullptr ? nozzles->values.size() : 0;
}

static bool validate_mixed_filament_nozzle_diameters(wxWindow* parent,
                                                      const std::vector<int>& filament_map,
                                                      size_t num_physical,
                                                      bool slice_all,
                                                      int plate_index)
{
    auto* bundle = Slic3r::GUI::wxGetApp().preset_bundle;
    auto* plater = Slic3r::GUI::wxGetApp().plater();
    if (bundle == nullptr || plater == nullptr)
        return true;

    const auto* nozzle_diameters = bundle->printers.get_edited_preset().config
        .option<Slic3r::ConfigOptionFloats>("nozzle_diameter");
    if (nozzle_diameters == nullptr)
        return true;

    PartPlateList& plate_list = plater->get_partplate_list();
    std::vector<PartPlate*> plates;
    if (slice_all) {
        plates.reserve(plate_list.get_plate_count());
        for (int plate_idx = 0; plate_idx < plate_list.get_plate_count(); ++plate_idx)
            plates.emplace_back(plate_list.get_plate(plate_idx));
    } else {
        plates.emplace_back(plate_index >= 0 ? plate_list.get_plate(plate_index) : plate_list.get_curr_plate());
    }

    const auto show_error = [parent](const wxString& message) {
        Slic3r::GUI::MessageDialog(parent, message, _L("Filament grouping"),
                                   wxOK | wxICON_WARNING).ShowModal();
    };

    for (const PartPlate* plate : plates) {
        if (plate == nullptr)
            continue;

        const std::vector<int> plate_extruders = plate->get_extruders(true);
        std::vector<unsigned int> logical_filament_ids;
        logical_filament_ids.reserve(plate_extruders.size());
        for (const int filament_id : plate_extruders) {
            if (filament_id > 0)
                logical_filament_ids.emplace_back(static_cast<unsigned int>(filament_id));
        }

        const Slic3r::ExpandedFilamentUsage usage =
            bundle->mixed_filaments.expand_filament_usage(logical_filament_ids, num_physical);
        if (!usage.valid()) {
            show_error(_L("Unable to resolve a mixed filament used by the current plate."));
            return false;
        }
        if (!usage.has_mixed_filament)
            continue;

        bool has_diameter = false;
        double first_diameter = 0.0;
        for (const unsigned int physical_filament_id : usage.physical_filament_ids) {
            const size_t filament = size_t(physical_filament_id - 1);
            if (filament >= filament_map.size()) {
                show_error(_L("Filament nozzle mapping is incomplete."));
                return false;
            }
            const int mapped_nozzle = filament_map[filament];
            if (mapped_nozzle < 1 || size_t(mapped_nozzle) > nozzle_diameters->size()) {
                show_error(_L("Filament nozzle mapping points to an unavailable nozzle."));
                return false;
            }

            const double diameter = nozzle_diameters->get_at(size_t(mapped_nozzle - 1));
            if (!has_diameter) {
                first_diameter = diameter;
                has_diameter = true;
            } else if (std::abs(diameter - first_diameter) > EPSILON) {
                show_error(_L("The current plate uses mixed filaments with different nozzle diameters, which is not supported for slicing. Map all filaments on this plate to nozzles of the same diameter and try again."));
                return false;
            }
        }
    }
    return true;
}

void FilamentPanel::refresh_filament_grouping_visibility()
{
    if (m_grouping_btn == nullptr)
        return;

    // Temporarily keep the direct grouping entry hidden; custom mode still opens it before slicing.
    m_grouping_btn->Show(false);
    if (m_box_sizer != nullptr)
        m_box_sizer->Layout();
}

bool FilamentPanel::prepare_filament_nozzle_mapping_for_slice(bool will_post_slice_event, bool slice_all, int plate_index)
{
    // ShowModal dispatches queued slice events too. Do not consume the skip flag
    // or let another slicing request proceed while mapping is being confirmed.
    if (m_filament_nozzle_mapping_in_progress)
        return false;

    // Imported G-code already has its filament mapping baked into the toolpaths.
    // A G-code-only 3MF can have valid plates whose paths are not loaded yet.
    // Applying a new mapping would invalidate all those plates.
    auto* plater = Slic3r::GUI::wxGetApp().plater();
    if (plater != nullptr && (plater->only_gcode_mode() || plater->using_exported_file()))
        return true;

    auto* bundle = Slic3r::GUI::wxGetApp().preset_bundle;
    if (bundle == nullptr)
        return true;

    if (m_skip_next_filament_nozzle_mapping_dialog) {
        if (!will_post_slice_event)
            m_skip_next_filament_nozzle_mapping_dialog = false;
        return true;
    }

    if (bundle->m_project_filament_mapping_pending) {
        const bool confirmed = bundle->m_pending_filament_mapping_mode == Slic3r::fmmManual
            ? show_filament_grouping_dialog(slice_all, nullptr, plate_index)
            : apply_current_filament_nozzle_mapping(slice_all, plate_index);
        if (confirmed && will_post_slice_event)
            m_skip_next_filament_nozzle_mapping_dialog = true;
        return confirmed;
    }

    if (!supports_filament_nozzle_mapping())
        return true;

    if (!bundle->has_mixed_selected_nozzle_variants())
        return apply_current_filament_nozzle_mapping(slice_all, plate_index);

    // Different nozzle diameters or flow types always require custom grouping before slicing.
    const bool confirmed = show_filament_grouping_dialog(slice_all, nullptr, plate_index);
    if (confirmed && will_post_slice_event)
        m_skip_next_filament_nozzle_mapping_dialog = true;
    return confirmed;
}

std::vector<size_t> FilamentPanel::used_filament_ids_for_grouping(bool slice_all, int plate_index) const
{
    std::vector<int> extruders;
    auto* plater = Slic3r::GUI::wxGetApp().plater();
    if (plater != nullptr) {
        PartPlateList& plate_list = plater->get_partplate_list();
        if (slice_all) {
            const auto all_extruders = plate_list.get_extruders(true);
            extruders.assign(all_extruders.begin(), all_extruders.end());
        } else {
            const PartPlate* plate = plate_index >= 0 ? plate_list.get_plate(plate_index) : plate_list.get_curr_plate();
            if (plate != nullptr)
                extruders = plate->get_extruders(true);
        }
    }

    std::vector<unsigned int> logical_filament_ids;
    logical_filament_ids.reserve(extruders.size());
    for (const int extruder : extruders) {
        if (extruder > 0)
            logical_filament_ids.emplace_back(static_cast<unsigned int>(extruder));
    }

    std::vector<size_t> filament_ids;
    auto* bundle = Slic3r::GUI::wxGetApp().preset_bundle;
    if (bundle != nullptr) {
        const Slic3r::ExpandedFilamentUsage usage =
            bundle->mixed_filaments.expand_filament_usage(logical_filament_ids, m_vt_filament.size());
        filament_ids.reserve(usage.physical_filament_ids.size());
        for (const unsigned int filament_id : usage.physical_filament_ids)
            filament_ids.emplace_back(size_t(filament_id - 1));
    }
    return filament_ids;
}

Slic3r::FilamentMapAutoInput FilamentPanel::make_filament_map_auto_input(bool slice_all, int plate_index) const
{
    Slic3r::FilamentMapAutoInput input;
    input.filament_count = m_vt_filament.size();
    input.nozzle_count   = nozzle_count_for_mapping();

    const std::vector<size_t> used_filament_ids = used_filament_ids_for_grouping(slice_all, plate_index);
    input.used_filaments.reserve(used_filament_ids.size());
    for (const size_t filament_id : used_filament_ids)
        input.used_filaments.emplace_back(static_cast<unsigned int>(filament_id));

    auto* bundle = Slic3r::GUI::wxGetApp().preset_bundle;
    if (bundle == nullptr)
        return input;

    input.nozzle_compatibility = bundle->get_print_preset_nozzle_compatibility(bundle->prints.get_edited_preset(), true);
    input.nozzle_recommendation = bundle->get_print_preset_nozzle_compatibility(bundle->prints.get_edited_preset());
    input.filament_presets = bundle->filament_presets;
    input.filament_presets.resize(input.filament_count);
    input.filament_types.resize(input.filament_count);
    for (size_t i = 0; i < input.filament_presets.size(); ++i) {
        if (Slic3r::Preset* preset = bundle->filaments.find_preset(input.filament_presets[i]))
            preset->get_filament_type(input.filament_types[i]);
    }

    const auto* colour_option = bundle->project_config.option<Slic3r::ConfigOptionStrings>("filament_colour", false);
    if (colour_option != nullptr)
        input.filament_colours = colour_option->values;
    input.filament_colours.resize(input.filament_count);
    return input;
}

bool FilamentPanel::apply_current_filament_nozzle_mapping(bool slice_all, int plate_index)
{
    if (m_filament_nozzle_mapping_in_progress)
        return false;
    m_filament_nozzle_mapping_in_progress = true;
    Slic3r::ScopeGuard reset_mapping_guard([this] { m_filament_nozzle_mapping_in_progress = false; });

    auto* bundle = Slic3r::GUI::wxGetApp().preset_bundle;
    if (bundle == nullptr || m_vt_filament.empty())
        return true;

    wxWindow* dialog_parent = wxGetApp().mainframe != nullptr
                                  ? static_cast<wxWindow*>(wxGetApp().mainframe)
                                  : static_cast<wxWindow*>(this);
    const wxString dialog_title = bundle->has_mixed_selected_nozzle_variants()
                                      ? _L("Filament grouping")
                                      : _L(" ");

    const size_t nozzle_count = nozzle_count_for_mapping();
    if (nozzle_count == 0) {
        Slic3r::GUI::MessageDialog(dialog_parent, _L("The selected printer has no available nozzle for filament grouping."),
                                   dialog_title, wxOK | wxICON_WARNING).ShowModal();
        return false;
    }

    Slic3r::DynamicPrintConfig& project_config = bundle->project_config;
    const auto* saved_map_option = project_config.option<Slic3r::ConfigOptionInts>("filament_map", false);
    std::vector<int> current_map =
        saved_map_option != nullptr ? saved_map_option->values : std::vector<int>();
    const bool resolve_pending_map = bundle->m_project_filament_mapping_pending;
    const Slic3r::FilamentMapAutoInput input = make_filament_map_auto_input(slice_all, plate_index);
    std::string error;
    if (resolve_pending_map) {
        current_map = Slic3r::resolve_effective_filament_map(
            bundle->m_pending_filament_mapping_mode, {}, input, &error);
        if (!error.empty()) {
            Slic3r::GUI::MessageDialog(dialog_parent, from_u8(error), dialog_title,
                                       wxOK | wxICON_WARNING).ShowModal();
            return false;
        }
    }

    error = Slic3r::validate_complete_filament_map(
        current_map, m_vt_filament.size(), nozzle_count);
    if (!error.empty()) {
        Slic3r::GUI::MessageDialog(dialog_parent, from_u8(error), dialog_title,
                                   wxOK | wxICON_WARNING).ShowModal();
        return false;
    }

    (void) Slic3r::resolve_effective_filament_map(
        Slic3r::fmmManual, current_map, input, &error);
    if (!error.empty()) {
        Slic3r::GUI::MessageDialog(dialog_parent, from_u8(error), dialog_title,
                                   wxOK | wxICON_WARNING).ShowModal();
        return false;
    }

    if (!validate_mixed_filament_nozzle_diameters(
            dialog_parent, current_map, m_vt_filament.size(), slice_all, plate_index))
        return false;

    const std::vector<int> map_2 = Slic3r::build_filament_map_2(current_map);
    const std::vector<int> volume_map(current_map.size(), 0);
    const auto* saved_mode =
        project_config.option<Slic3r::ConfigOptionEnum<Slic3r::FilamentMapMode>>("filament_map_mode", false);
    const auto* saved_map_2 = project_config.option<Slic3r::ConfigOptionInts>("filament_map_2", false);
    const auto* saved_volume_map = project_config.option<Slic3r::ConfigOptionInts>("filament_volume_map", false);
    const bool changed = resolve_pending_map || saved_mode == nullptr || saved_mode->value != Slic3r::fmmAutoForSaving ||
                         saved_map_2 == nullptr || saved_map_2->values != map_2 ||
                         saved_volume_map == nullptr || saved_volume_map->values != volume_map;
    if (!changed)
        return true;

    Slic3r::DynamicPrintConfig new_project_config = project_config;
    new_project_config.option<Slic3r::ConfigOptionEnum<Slic3r::FilamentMapMode>>("filament_map_mode", true)->value =
        Slic3r::fmmAutoForSaving;
    new_project_config.option<Slic3r::ConfigOptionInts>("filament_map", true)->values = current_map;
    new_project_config.option<Slic3r::ConfigOptionInts>("filament_map_2", true)->values = map_2;
    new_project_config.option<Slic3r::ConfigOptionInts>("filament_volume_map", true)->values = volume_map;
    project_config.apply(new_project_config);
    bundle->m_project_filament_mapping_pending = false;

    for (size_t i = 0; i < m_vt_filament.size(); ++i) {
        if (m_vt_filament[i] != nullptr)
            m_vt_filament[i]->set_nozzle_no(current_map[i]);
    }

    BOOST_LOG_TRIVIAL(warning)
        << "[K3_EXPORT_TRACE][MAPPING] preserved current map before slicing"
        << " filament_count=" << current_map.size()
        << " nozzle_count=" << nozzle_count;
    if (auto* plater = Slic3r::GUI::wxGetApp().plater()) {
        plater->update_project_dirty_from_presets();
        plater->on_config_change(bundle->full_config());
        plater->invalid_slice_result_need_reslice();
    }
    return true;
}

void FilamentPanel::open_filament_grouping_dialog()
{
    bool mapping_changed = false;
    if (!show_filament_grouping_dialog(false, &mapping_changed) || !mapping_changed)
        return;

    // The grouping was confirmed from the preview panel. Re-slice the current
    // plate after the modal dialog has fully closed, without showing it again.
    m_skip_next_filament_nozzle_mapping_dialog = true;
    Slic3r::GUI::wxGetApp().CallAfter([]() {
        if (Slic3r::GUI::wxGetApp().mainframe != nullptr)
            Slic3r::GUI::wxGetApp().mainframe->slice_plate(MainFrame::eSlicePlate);
    });
}

bool FilamentPanel::show_filament_grouping_dialog(bool slice_all, bool* mapping_changed, int plate_index)
{
    if (mapping_changed != nullptr)
        *mapping_changed = false;

    // Share the guard with mapping validation: both paths can show modal
    // warnings, including when grouping is opened directly from the preview.
    if (m_filament_nozzle_mapping_in_progress)
        return false;
    m_filament_nozzle_mapping_in_progress = true;
    Slic3r::ScopeGuard reset_mapping_guard([this] { m_filament_nozzle_mapping_in_progress = false; });

    auto *mapping_bundle = Slic3r::GUI::wxGetApp().preset_bundle;
    if (mapping_bundle == nullptr || m_vt_filament.empty() ||
        (!supports_filament_nozzle_mapping() && !mapping_bundle->m_project_filament_mapping_pending))
        return true;

    wxWindow* dialog_parent = wxGetApp().mainframe != nullptr
                                  ? static_cast<wxWindow*>(wxGetApp().mainframe)
                                  : static_cast<wxWindow*>(this);

    const size_t nozzle_count = nozzle_count_for_mapping();
    if (nozzle_count == 0) {
        Slic3r::GUI::MessageDialog(dialog_parent, _L("The selected printer has no available nozzle for filament grouping."),
                                   _L("Filament grouping"), wxOK | wxICON_WARNING).ShowModal();
        return false;
    }

    const std::vector<size_t> used_filament_ids = used_filament_ids_for_grouping(slice_all, plate_index);
    if (used_filament_ids.empty())
        return validate_mixed_filament_nozzle_diameters(
            dialog_parent, {}, m_vt_filament.size(), slice_all, plate_index);

    Slic3r::PresetBundle* bundle = Slic3r::GUI::wxGetApp().preset_bundle;
    Slic3r::DynamicPrintConfig* project_config = &bundle->project_config;
    const auto* saved_map_option = project_config->option<Slic3r::ConfigOptionInts>("filament_map", false);
    const std::vector<int> saved_map =
        saved_map_option != nullptr ? saved_map_option->values : std::vector<int>();
    const bool pending_manual_map = bundle->m_project_filament_mapping_pending &&
                                    bundle->m_pending_filament_mapping_mode == Slic3r::fmmManual;
    const bool preserve_saved_map = bundle->should_preserve_project_filament_mapping() && !pending_manual_map;
    if (preserve_saved_map) {
        const std::string saved_map_error = Slic3r::validate_complete_filament_map(
            saved_map, m_vt_filament.size(), nozzle_count);
        if (!saved_map_error.empty()) {
            Slic3r::GUI::MessageDialog(dialog_parent, from_u8(saved_map_error), _L("Filament grouping"),
                                       wxOK | wxICON_WARNING).ShowModal();
            return false;
        }
    }


    const Slic3r::FilamentMapAutoInput automatic_input = make_filament_map_auto_input(slice_all, plate_index);
    const std::vector<std::string>& preset_names = automatic_input.filament_presets;
    const std::vector<std::string>& filament_types = automatic_input.filament_types;
    const std::vector<std::string>& colours = automatic_input.filament_colours;

    std::vector<int> initial_map = saved_map;
    if (pending_manual_map) {
        initial_map = Slic3r::normalize_filament_map(saved_map, m_vt_filament.size(), nozzle_count);
    } else if (!preserve_saved_map) {
        std::string automatic_error;
        initial_map = Slic3r::resolve_effective_filament_map(
            Slic3r::fmmAutoForSaving, {}, automatic_input, &automatic_error);
        if (!automatic_error.empty()) {
            Slic3r::GUI::MessageDialog(dialog_parent, from_u8(automatic_error), _L("Filament grouping"),
                                       wxOK | wxICON_WARNING).ShowModal();
            return false;
        }
    }

    const bool dark_mode = wxGetApp().dark_mode();
    const wxColour dialog_background      = dark_mode ? wxColour("#2B2B2B") : wxColour("#EFF0F6");
    const wxColour card_title_background  = dark_mode ? wxColour("#565658") : wxColour("#F5F6FA");
    const wxColour card_body_background   = dark_mode ? wxColour("#303031") : wxColour("#FFFFFF");
    const wxColour card_border             = dark_mode ? wxColour("#4B4C4E") : wxColour("#C5CBD5");
    const wxColour card_hover_border       = dark_mode ? wxColour("#73767B") : wxColour("#AEB7C4");
    const wxColour incompatible_border     = dark_mode ? wxColour("#C75450") : wxColour("#D94841");
    const wxColour incompatible_text       = dark_mode ? wxColour("#FF8A84") : wxColour("#C9362F");
    const wxColour recommendation_text     = wxColour(255, 191, 0); // Existing slice-warning yellow.
    const wxColour primary_text             = dark_mode ? wxColour("#F2F2F3") : wxColour("#4A535F");
    const wxColour secondary_text           = dark_mode ? wxColour("#9B9BA0") : wxColour("#7A8492");
    const wxColour selector_background      = dark_mode ? wxColour("#565658") : wxColour("#FFFFFF");
    const wxColour selector_border          = dark_mode ? wxColour("#696A6D") : wxColour("#D5D9E1");
    const wxColour footer_background        = dark_mode ? wxColour("#2B2B2B") : wxColour("#FFFFFF");
    const wxColour close_icon_colour        = dark_mode ? wxColour("#D7D9DC") : wxColour("#3F4854");
    const wxColour close_hover_background  = dark_mode ? wxColour("#3A3A3C") : wxColour("#E0E4EC");
    const wxColour cancel_border             = dark_mode ? wxColour("#6C6E71") : wxColour("#8F98A5");
    const wxColour cancel_normal             = dark_mode ? wxColour("#2B2B2B") : wxColour("#FFFFFF");
    const wxColour cancel_hover              = dark_mode ? wxColour("#38383A") : wxColour("#F8F9FB");
    const wxColour cancel_pressed            = dark_mode ? wxColour("#242425") : wxColour("#F4F5F8");

    wxDialog dialog(dialog_parent, wxID_ANY, _L("Nozzle filament settings"), wxDefaultPosition, wxDefaultSize,
                    wxBORDER_NONE | wxTAB_TRAVERSAL);
    dialog.SetFont(wxGetApp().normal_font());
    dialog.SetBackgroundColour(dialog_background);
    dialog.SetDoubleBuffered(true);
    auto* main_sizer = new wxBoxSizer(wxVERTICAL);

    auto* header = new wxPanel(&dialog, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    header->SetBackgroundColour(dialog_background);
    header->SetMinSize(wxSize(-1, dialog.FromDIP(42)));
    auto* header_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto* dialog_title = new wxStaticText(header, wxID_ANY, _L("Nozzle filament settings"));
    wxFont title_font = Label::Body_13;
    title_font.SetWeight(wxFONTWEIGHT_BOLD);
    dialog_title->SetFont(title_font);
    dialog_title->SetForegroundColour(primary_text);
    dialog_title->SetBackgroundColour(dialog_background);

#ifdef __WXMSW__
    const auto begin_dialog_drag = [&dialog](wxMouseEvent&) {
        const wxPoint mouse_pos = wxGetMousePosition();
        ::PostMessage((HWND)dialog.GetHandle(), WM_NCLBUTTONDOWN, HTCAPTION,
                      MAKELPARAM(mouse_pos.x, mouse_pos.y));
    };
    header->Bind(wxEVT_LEFT_DOWN, begin_dialog_drag);
    dialog_title->Bind(wxEVT_LEFT_DOWN, begin_dialog_drag);
#else
    bool      dialog_dragging = false;
    wxPoint   dialog_drag_offset;
    wxWindow* dialog_drag_capture = nullptr;
    const auto begin_dialog_drag = [&](wxMouseEvent& event) {
        dialog_drag_offset = wxGetMousePosition() - dialog.GetPosition();
        dialog_dragging = true;
        dialog_drag_capture = dynamic_cast<wxWindow*>(event.GetEventObject());
        if (dialog_drag_capture == nullptr)
            dialog_drag_capture = header;
        if (!dialog_drag_capture->HasCapture())
            dialog_drag_capture->CaptureMouse();
    };
    const auto continue_dialog_drag = [&](wxMouseEvent&) {
        if (dialog_dragging && wxGetMouseState().LeftIsDown())
            dialog.Move(wxGetMousePosition() - dialog_drag_offset);
    };
    const auto end_dialog_drag = [&](wxMouseEvent&) {
        dialog_dragging = false;
        if (dialog_drag_capture != nullptr && dialog_drag_capture->HasCapture())
            dialog_drag_capture->ReleaseMouse();
        dialog_drag_capture = nullptr;
    };
    header->Bind(wxEVT_LEFT_DOWN, begin_dialog_drag);
    dialog_title->Bind(wxEVT_LEFT_DOWN, begin_dialog_drag);
    header->Bind(wxEVT_MOTION, continue_dialog_drag);
    dialog_title->Bind(wxEVT_MOTION, continue_dialog_drag);
    header->Bind(wxEVT_LEFT_UP, end_dialog_drag);
    dialog_title->Bind(wxEVT_LEFT_UP, end_dialog_drag);
    header->Bind(wxEVT_MOUSE_CAPTURE_LOST, [&](wxMouseCaptureLostEvent&) {
        dialog_dragging = false;
        dialog_drag_capture = nullptr;
    });
    dialog_title->Bind(wxEVT_MOUSE_CAPTURE_LOST, [&](wxMouseCaptureLostEvent&) {
        dialog_dragging = false;
        dialog_drag_capture = nullptr;
    });
#endif
    FilamentGroupingChip* active_chip = nullptr;
    const auto cancel_drag = [&]() {
        if (active_chip)
            active_chip->CancelDrag();
    };
    const auto end_dialog = [&](int result) {
        cancel_drag();
        dialog.EndModal(result);
    };
    auto* close_button = new FilamentGroupingCloseButton(header, close_icon_colour, close_hover_background);
    close_button->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { end_dialog(wxID_CANCEL); });
    header_sizer->Add(dialog_title, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, dialog.FromDIP(14));
    header_sizer->AddStretchSpacer();
    header_sizer->Add(close_button, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, dialog.FromDIP(14));
    header->SetSizer(header_sizer);
    main_sizer->Add(header, 0, wxEXPAND);

    auto* description = new wxStaticText(&dialog, wxID_ANY,
        _L("Drag filaments to reassign them to different nozzles."));
    description->SetFont(Label::Body_13);
    description->SetForegroundColour(secondary_text);
    description->SetBackgroundColour(dialog_background);
    main_sizer->Add(description, 0, wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, dialog.FromDIP(14));

    auto* scrolled = new wxScrolledWindow(&dialog, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                           wxVSCROLL | wxBORDER_NONE);
    scrolled->SetBackgroundColour(dialog_background);
    scrolled->SetMinSize(wxSize(dialog.FromDIP(860), dialog.FromDIP(524)));
    scrolled->SetScrollRate(0, dialog.FromDIP(16));
    const size_t card_columns = std::min<size_t>(2, nozzle_count);
    auto* card_grid = new wxFlexGridSizer(0, static_cast<int>(card_columns), dialog.FromDIP(16), dialog.FromDIP(16));
    for (size_t column = 0; column < card_columns; ++column)
        card_grid->AddGrowableCol(static_cast<int>(column), 1);
    scrolled->SetSizer(card_grid);

    std::vector<int> manual_map = initial_map;
    std::function<void()> rebuild_cards;
    bool rebuild_pending = false;
    bool rebuild_queued = false;
    std::vector<std::pair<size_t, FilamentGroupingCard*>> cards;
    const auto nozzle_is_compatible = [&automatic_input](size_t nozzle) {
        return nozzle >= 1 && nozzle <= automatic_input.nozzle_count &&
               (automatic_input.nozzle_compatibility.empty() ||
                automatic_input.nozzle_compatibility[nozzle - 1]);
    };
    const auto* process_layer_height = bundle->prints.get_edited_preset().config.option<Slic3r::ConfigOptionFloat>("layer_height");
    const auto* initial_layer_height = bundle->prints.get_edited_preset().config.option<Slic3r::ConfigOptionFloat>("initial_layer_print_height");
    const auto format_layer_height = [](double height) {
        wxString result = wxString::Format("%.4f", height);
        while (result.EndsWith("0") && result.AfterLast('.').length() > 2)
            result.RemoveLast();
        return result;
    };
    const auto schedule_rebuild_cards = [&]() {
        rebuild_pending = true;
        if (active_chip || rebuild_queued)
            return;
        rebuild_queued = true;
        dialog.CallAfter([&]() {
            rebuild_queued = false;
            if (rebuild_pending && !active_chip)
                rebuild_cards();
        });
    };
    const auto nozzle_at = [&](const wxPoint& position) -> size_t {
        if (!scrolled->GetScreenRect().Contains(position))
            return 0;
        for (const auto& [nozzle, card] : cards) {
            if (nozzle_is_compatible(nozzle) && card->GetScreenRect().Contains(position))
                return nozzle;
        }
        return 0;
    };
    const auto update_drag_hover = [&](const wxPoint& position) {
        const size_t target = nozzle_at(position);
        for (const auto& [nozzle, card] : cards)
            card->set_drop_hovered(nozzle == target);
    };
    const auto finish_drag = [&](size_t filament, const wxPoint& position, bool dropped) {
        const size_t target = dropped ? nozzle_at(position) : 0;
        active_chip = nullptr;
        for (const auto& entry : cards)
            entry.second->set_drop_hovered(false);
        if (target && filament < manual_map.size() && manual_map[filament] != static_cast<int>(target)) {
            manual_map[filament] = static_cast<int>(target);
            rebuild_pending = true;
        }
        if (rebuild_pending)
            schedule_rebuild_cards();
    };
    rebuild_cards = [&]() {
        // DPI changes may also request a rebuild while the source has capture.
        if (active_chip) {
            rebuild_pending = true;
            return;
        }
        rebuild_pending = false;
        scrolled->Freeze();
        cards.clear();
        card_grid->Clear(true);

        const std::vector<int>& map_to_display = manual_map;
        for (size_t nozzle = 1; nozzle <= nozzle_count; ++nozzle) {
            const bool compatible = nozzle_is_compatible(nozzle);
            const bool recommended = compatible && (automatic_input.nozzle_recommendation.empty() ||
                automatic_input.nozzle_recommendation[nozzle - 1]);
            auto* card = new FilamentGroupingCard(scrolled, dialog_background, card_body_background,
                                                       compatible ? card_border : incompatible_border, card_hover_border);
            if (!compatible)
                card->SetToolTip(_L("The current process layer heights are incompatible with this nozzle."));
            cards.emplace_back(nozzle, card);
            auto* card_sizer = new wxBoxSizer(wxVERTICAL);

            auto* title = new FilamentGroupingCardSection(card, card_title_background, true, false);
            title->SetBackgroundColour(card_title_background);
            title->SetMinSize(wxSize(-1, dialog.FromDIP(35)));
            auto* title_sizer = new wxBoxSizer(wxHORIZONTAL);
            auto* title_label = new wxStaticText(title, wxID_ANY,
                wxString::Format(_L("Nozzle %d"), static_cast<int>(nozzle)));
            title_label->SetFont(title_font);
            title_label->SetForegroundColour(compatible ? primary_text : incompatible_text);
            title_label->SetBackgroundColour(card_title_background);

            const Slic3r::NozzleVariantInfo selected_nozzle = bundle->get_selected_nozzle_variant(nozzle - 1);
            const wxString diameter_label = wxString::Format("%.1fmm", selected_nozzle.nozzle_diameter);
            auto* diameter_selector = new FilamentGroupingNozzleSelector(
                title, diameter_label, selector_background,
                compatible ? selector_border : incompatible_border,
                compatible ? primary_text : incompatible_text);

            title_sizer->Add(title_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, dialog.FromDIP(13));
            if (!recommended) {
                auto* status = new wxStaticText(title, wxID_ANY, compatible ? _L("Outside recommended range") : _L("Unavailable"));
                status->SetFont(Label::Body_12);
                status->SetForegroundColour(compatible ? recommendation_text : incompatible_text);
                status->SetBackgroundColour(card_title_background);
                title_sizer->Add(status, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, dialog.FromDIP(8));
            }
            title_sizer->AddStretchSpacer();
            title_sizer->Add(diameter_selector, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, dialog.FromDIP(13));
            title->SetSizer(title_sizer);
            card_sizer->Add(title, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, dialog.FromDIP(2));

            auto* body = new FilamentGroupingCardSection(card, card_body_background, false, true);
            body->SetBackgroundColour(card_body_background);
            auto* body_sizer = new wxBoxSizer(wxVERTICAL);
            if (!recommended && process_layer_height != nullptr && initial_layer_height != nullptr) {
                const double minimum = compatible
                    ? (selected_nozzle.min_layer_height > 0.0 ? selected_nozzle.min_layer_height : 0.2 * selected_nozzle.nozzle_diameter) : 0.0;
                const double maximum = compatible
                    ? (selected_nozzle.max_layer_height > 0.0 ? selected_nozzle.max_layer_height : 0.75 * selected_nozzle.nozzle_diameter)
                    : selected_nozzle.nozzle_diameter;
                auto* warning = new wxPanel(body, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
                warning->SetBackgroundColour(card_body_background);
                auto* warning_sizer = new wxBoxSizer(wxVERTICAL);
                auto* warning_title = new wxStaticText(warning, wxID_ANY,
                    compatible
                        ? _L("Usable, but printing quality may be affected.")
                        : _L("The current layer heights are outside the range supported by this nozzle."));
                warning_title->SetFont(Label::Body_13);
                warning_title->Wrap(dialog.FromDIP(340));
                warning_title->SetForegroundColour(compatible ? recommendation_text : incompatible_text);
                warning_title->SetBackgroundColour(card_body_background);
                warning_sizer->Add(warning_title, 0, wxBOTTOM, dialog.FromDIP(12));
                auto* supported_range = new wxStaticText(warning, wxID_ANY,
                    compatible
                        ? wxString::Format(_L("Recommended layer height range: %s-%s mm"),
                            format_layer_height(minimum), format_layer_height(maximum))
                        : wxString::Format(_L("Layer height and initial layer height must be greater than 0 and must not exceed the nozzle diameter (%s mm)."),
                            format_layer_height(selected_nozzle.nozzle_diameter)));
                supported_range->SetFont(Label::Body_12);
                supported_range->Wrap(dialog.FromDIP(340));
                supported_range->SetForegroundColour(primary_text);
                supported_range->SetBackgroundColour(card_body_background);
                warning_sizer->Add(supported_range, 0, wxBOTTOM, dialog.FromDIP(8));
                auto* current_heights = new wxStaticText(warning, wxID_ANY,
                    wxString::Format(_L("Current preset first layer height: %s mm, layer height: %s mm"),
                        format_layer_height(initial_layer_height->value), format_layer_height(process_layer_height->value)));
                current_heights->SetFont(Label::Body_12);
                current_heights->SetForegroundColour(primary_text);
                current_heights->SetBackgroundColour(card_body_background);
                warning_sizer->Add(current_heights);
                warning->SetSizer(warning_sizer);
                body_sizer->AddStretchSpacer();
                body_sizer->Add(warning, 0, wxALIGN_CENTER_HORIZONTAL);
                body_sizer->AddStretchSpacer();
            }
            auto* chip_sizer = new wxGridSizer(0, 4, dialog.FromDIP(10),
                                                dialog.FromDIP(10));
            bool has_filament = false;
            for (const size_t filament : used_filament_ids) {
                if (map_to_display[filament] != static_cast<int>(nozzle))
                    continue;

                wxColour colour(colours[filament]);
                wxString display_name = from_u8(filament_types[filament]);
                if (m_vt_filament[filament] != nullptr) {
                    const wxColour sidebar_colour = m_vt_filament[filament]->color();
                    if (sidebar_colour.IsOk())
                        colour = sidebar_colour;

                    const wxString sidebar_name = m_vt_filament[filament]->name();
                    if (!sidebar_name.IsEmpty())
                        display_name = sidebar_name;
                }
                auto* chip = new FilamentGroupingChip(body, filament,
                    colour.IsOk() ? colour : wxColour("#4F5965"), display_name,
                    [&](FilamentGroupingChip* source) {
                        if (active_chip)
                            return false;
                        active_chip = source;
                        return true;
                    }, update_drag_hover, finish_drag);
                chip_sizer->Add(chip);
                has_filament = true;
            }
            if (has_filament) {
                body_sizer->Add(chip_sizer, 0, wxEXPAND | wxALL, dialog.FromDIP(16));
            } else if (compatible || process_layer_height == nullptr || initial_layer_height == nullptr) {
                auto* empty = new wxStaticText(body, wxID_ANY, _L("No filament"));
                empty->SetFont(Label::Body_13);
                empty->SetForegroundColour(secondary_text);
                empty->SetBackgroundColour(card_body_background);
                body_sizer->AddStretchSpacer();
                body_sizer->Add(empty, 0, wxALIGN_CENTER_HORIZONTAL);
                body_sizer->AddStretchSpacer();
            }
            body->SetSizer(body_sizer);
            card_sizer->Add(body, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, dialog.FromDIP(2));
            card->SetSizer(card_sizer);
            card->SetMinSize(wxSize(dialog.FromDIP(422), dialog.FromDIP(254)));
            card_grid->Add(card, 0, wxEXPAND);
        }

        scrolled->FitInside();
        scrolled->Layout();
        dialog.Layout();
        scrolled->Thaw();
    };
    main_sizer->Add(scrolled, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, dialog.FromDIP(12));

    auto* footer = new wxPanel(&dialog, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    footer->SetBackgroundColour(footer_background);
    footer->SetMinSize(wxSize(-1, dialog.FromDIP(48)));
    auto* footer_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto* cancel_button = new Button(footer, _L("Cancel"));
    cancel_button->SetMinSize(wxSize(dialog.FromDIP(90), dialog.FromDIP(30)));
    cancel_button->SetMaxSize(wxSize(dialog.FromDIP(90), dialog.FromDIP(30)));
    cancel_button->SetFont(Label::Body_13);
    cancel_button->SetCornerRadius(4);
    cancel_button->SetBorderColor(cancel_border);
    cancel_button->SetBackgroundColor(StateColor(
        std::pair<wxColour, int>(cancel_pressed, StateColor::Pressed),
        std::pair<wxColour, int>(cancel_hover, StateColor::Hovered),
        std::pair<wxColour, int>(cancel_normal, StateColor::Normal)));
    cancel_button->SetTextColor(StateColor(
        std::pair<wxColour, int>(primary_text, StateColor::Pressed),
        std::pair<wxColour, int>(primary_text, StateColor::Hovered),
        std::pair<wxColour, int>(primary_text, StateColor::Normal)));
    cancel_button->Bind(wxEVT_BUTTON, [&](wxEvent&) { end_dialog(wxID_CANCEL); });

    auto* confirm_button = new Button(footer, _L("Confirm"));
    confirm_button->SetMinSize(wxSize(dialog.FromDIP(90), dialog.FromDIP(30)));
    confirm_button->SetMaxSize(wxSize(dialog.FromDIP(90), dialog.FromDIP(30)));
    confirm_button->SetFont(Label::Body_13);
    confirm_button->SetCornerRadius(4);
    confirm_button->SetForegroundColour(*wxWHITE);
    confirm_button->SetBorderColor(wxColour("#1FCA63"));
    confirm_button->SetBackgroundColor(StateColor(
        std::pair<wxColour, int>(wxColour("#18B957"), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour("#2AD672"), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour("#1FCA63"), StateColor::Normal)));
    confirm_button->SetTextColor(StateColor(
        std::pair<wxColour, int>(*wxWHITE, StateColor::Pressed),
        std::pair<wxColour, int>(*wxWHITE, StateColor::Hovered),
        std::pair<wxColour, int>(*wxWHITE, StateColor::Normal)));
    confirm_button->Bind(wxEVT_BUTTON, [&](wxEvent&) {
        cancel_drag();
        if (!validate_mixed_filament_nozzle_diameters(
                &dialog, manual_map, m_vt_filament.size(), slice_all, plate_index))
            return;

        std::string manual_error;
        (void) Slic3r::resolve_effective_filament_map(
            Slic3r::fmmManual, manual_map, automatic_input, &manual_error);
        if (!manual_error.empty()) {
            Slic3r::GUI::MessageDialog(&dialog, from_u8(manual_error), _L("Filament grouping"),
                                       wxOK | wxICON_WARNING).ShowModal();
            return;
        }
        end_dialog(wxID_OK);
    });

    footer_sizer->AddStretchSpacer();
    footer_sizer->Add(cancel_button, 0, wxALIGN_CENTER_VERTICAL);
    footer_sizer->Add(confirm_button, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, dialog.FromDIP(10));
    footer_sizer->AddStretchSpacer();
    footer->SetSizer(footer_sizer);
    main_sizer->Add(footer, 0, wxEXPAND);

    dialog.SetSizer(main_sizer);

    // Keep the dialog in logical (DIP) units when it crosses monitors with
    // different scale factors. The scrolled card area is allowed to shrink so
    // the fixed footer can never be pushed outside the top-level window.
    const auto apply_dialog_dpi_layout = [&](bool keep_screen_position) {
        header->SetMinSize(wxSize(-1, dialog.FromDIP(42)));
        const wxSize close_size = wxWindow::FromDIP(wxSize(28, 28), &dialog);
        close_button->SetMinSize(close_size);
        close_button->SetMaxSize(close_size);
        if (wxSizerItem* item = header_sizer->GetItem(dialog_title))
            item->SetBorder(dialog.FromDIP(14));
        if (wxSizerItem* item = header_sizer->GetItem(close_button))
            item->SetBorder(dialog.FromDIP(14));

        scrolled->SetMinSize(wxSize(dialog.FromDIP(860), dialog.FromDIP(524)));
        scrolled->SetScrollRate(0, dialog.FromDIP(16));
        card_grid->SetVGap(dialog.FromDIP(16));
        card_grid->SetHGap(dialog.FromDIP(16));
        if (wxSizerItem* item = main_sizer->GetItem(description))
            item->SetBorder(dialog.FromDIP(14));
        if (wxSizerItem* item = main_sizer->GetItem(scrolled))
            item->SetBorder(dialog.FromDIP(12));

        footer->SetMinSize(wxSize(-1, dialog.FromDIP(48)));
        const wxSize button_size = wxWindow::FromDIP(wxSize(90, 30), &dialog);
        cancel_button->SetMinSize(button_size);
        cancel_button->SetMaxSize(button_size);
        confirm_button->SetMinSize(button_size);
        confirm_button->SetMaxSize(button_size);
        if (wxSizerItem* item = footer_sizer->GetItem(confirm_button))
            item->SetBorder(dialog.FromDIP(10));

        rebuild_cards();

        wxRect display_area = wxDisplay(&dialog).GetClientArea();
        const int screen_margin = dialog.FromDIP(12);
        wxSize desired_size = wxWindow::FromDIP(wxSize(892, 691), &dialog);
        wxSize minimum_size = wxWindow::FromDIP(wxSize(892, 691), &dialog);
        if (!display_area.IsEmpty()) {
            desired_size.SetWidth(std::min(desired_size.GetWidth(),
                                           std::max(1, display_area.GetWidth() - screen_margin * 2)));
            desired_size.SetHeight(std::min(desired_size.GetHeight(),
                                            std::max(1, display_area.GetHeight() - screen_margin * 2)));
        }
        minimum_size.SetWidth(std::min(minimum_size.GetWidth(), desired_size.GetWidth()));
        minimum_size.SetHeight(std::min(minimum_size.GetHeight(), desired_size.GetHeight()));

        dialog.SetMinSize(wxDefaultSize);
        dialog.SetMinSize(minimum_size);
        dialog.SetSize(desired_size);
        dialog.Layout();
#ifdef __WXMSW__
        ApplyWindowShadow(&dialog);
#endif
        scrolled->FitInside();
        dialog.Refresh(true);

        if (keep_screen_position && !display_area.IsEmpty()) {
            wxPoint position = dialog.GetPosition();
            const wxSize actual_size = dialog.GetSize();
            const int max_x = std::max(display_area.GetLeft(),
                                       display_area.GetRight() - actual_size.GetWidth() + 1);
            const int max_y = std::max(display_area.GetTop(),
                                       display_area.GetBottom() - actual_size.GetHeight() + 1);
            position.x = std::clamp(position.x, display_area.GetLeft(), max_x);
            position.y = std::clamp(position.y, display_area.GetTop(), max_y);
            dialog.SetPosition(position);
        }
    };

#ifdef __WXMSW__
    dialog.Bind(wxEVT_SHOW, [&dialog](wxShowEvent& event) {
        event.Skip();
        if (event.IsShown())
            ApplyWindowShadow(&dialog);
    });
#endif
    dialog.Bind(wxEVT_DPI_CHANGED, [&](wxDPIChangedEvent& event) {
        event.Skip();
        dialog.CallAfter([&]() { apply_dialog_dpi_layout(true); });
    });
    dialog.Bind(wxEVT_CLOSE_WINDOW, [&](wxCloseEvent&) { end_dialog(wxID_CANCEL); });
    dialog.Bind(wxEVT_CHAR_HOOK, [&](wxKeyEvent& event) {
        if (event.GetKeyCode() == WXK_ESCAPE) {
            if (active_chip)
                cancel_drag();
            else
                end_dialog(wxID_CANCEL);
        } else {
            event.Skip();
        }
    });
    apply_dialog_dpi_layout(false);
    dialog.CenterOnParent();

    const int dialog_result = dialog.ShowModal();
    cancel_drag();
    dialog.DeletePendingEvents();
    if (dialog_result != wxID_OK)
        return false;

    const Slic3r::FilamentMapMode selected_mode = Slic3r::fmmManual;
    std::string manual_error;
    std::vector<int> selected_map = Slic3r::resolve_effective_filament_map(
        selected_mode, manual_map, automatic_input, &manual_error);
    if (!manual_error.empty())
        return false;
    const std::vector<int> selected_map_2 = Slic3r::build_filament_map_2(selected_map);
    const std::vector<int> selected_volume_map(selected_map.size(), 0);
    const auto* saved_mode_option =
        project_config->option<Slic3r::ConfigOptionEnum<Slic3r::FilamentMapMode>>("filament_map_mode", false);
    const auto* saved_map_2_option = project_config->option<Slic3r::ConfigOptionInts>("filament_map_2", false);
    const auto* saved_volume_map_option =
        project_config->option<Slic3r::ConfigOptionInts>("filament_volume_map", false);
    const bool changed = bundle->m_project_filament_mapping_pending ||
                         saved_mode_option == nullptr || saved_mode_option->value != selected_mode ||
                         saved_map_option == nullptr || saved_map_option->values != selected_map ||
                         saved_map_2_option == nullptr || saved_map_2_option->values != selected_map_2 ||
                         saved_volume_map_option == nullptr || saved_volume_map_option->values != selected_volume_map;
    if (!changed)
        return true;

    Slic3r::DynamicPrintConfig new_project_config = *project_config;
    new_project_config.option<Slic3r::ConfigOptionEnum<Slic3r::FilamentMapMode>>("filament_map_mode", true)->value = selected_mode;
    new_project_config.option<Slic3r::ConfigOptionInts>("filament_map", true)->values = selected_map;
    new_project_config.option<Slic3r::ConfigOptionInts>("filament_map_2", true)->values = selected_map_2;
    new_project_config.option<Slic3r::ConfigOptionInts>("filament_volume_map", true)->values = selected_volume_map;
    project_config->apply(new_project_config);
    bundle->m_project_filament_mapping_pending = false;
    bundle->set_preserve_project_filament_mapping(true);

    for (size_t i = 0; i < m_vt_filament.size(); ++i) {
        if (m_vt_filament[i] != nullptr)
            m_vt_filament[i]->set_nozzle_no(selected_map[i]);
    }

    if (mapping_changed != nullptr)
        *mapping_changed = true;
    Slic3r::GUI::wxGetApp().plater()->update_project_dirty_from_presets();
    Slic3r::GUI::wxGetApp().plater()->on_config_change(bundle->full_config());
    Slic3r::GUI::wxGetApp().plater()->invalid_slice_result_need_reslice();
    return true;
}

void FilamentPanel::on_filament_wheel(wxMouseEvent& event)
{
    if (event.GetWheelAxis() != wxMOUSE_WHEEL_VERTICAL || event.ControlDown() ||
        !m_filament_scrolled->HasScrollbar(wxVERTICAL)) {
        event.Skip();
        return;
    }
    const int delta = event.GetWheelDelta();
    if (delta <= 0) return;
    m_filament_wheel_rotation += event.GetWheelRotation();
    const int rows = m_filament_wheel_rotation / delta;
    m_filament_wheel_rotation %= delta;
    if (rows != 0)
        m_filament_scrolled->Scroll(0, m_filament_scrolled->GetViewStart().y - rows);
}

bool FilamentPanel::add_filament()
{
	if (m_vt_filament.size() == m_max_count)
	{
		return false;
	}

	FilamentItem::Data data;
	data.index = m_vt_filament.size();
	data.name = "PLA";
	data.small_state = m_vt_filament.size() >= m_small_count;
	//layout
	this->to_small(data.small_state);

	//add
	FilamentItem* filament = new FilamentItem(
        m_filament_content, data, wxSize(FromDIP(FILAMENT_BTN_WIDTH), FromDIP(FILAMENT_BTN_HEIGHT)));
    filament->Bind(wxEVT_MOUSEWHEEL, &FilamentPanel::on_filament_wheel, this);
    for (wxWindow* child : filament->GetChildren()) {
        if (auto* button = dynamic_cast<FilamentButton*>(child)) {
            button->Bind(wxEVT_MOUSEWHEEL, &FilamentPanel::on_filament_wheel, this);
            for (wxWindow* icon : button->GetChildren())
                icon->Bind(wxEVT_MOUSEWHEEL, &FilamentPanel::on_filament_wheel, this);
        }
    }
	if (const auto* map = wxGetApp().preset_bundle->project_config.option<Slic3r::ConfigOptionInts>("filament_map", false)) {
		if (data.index < map->values.size())
			filament->set_nozzle_no(map->values[data.index]);
	}
	filament->Bind(wxEVT_BUTTON, [this](wxEvent& e) {
		for (auto& f : this->m_vt_filament)
		{
			if (f->is_checked())
			{
				f->set_checked(false);
			}
		}
		});

	m_vt_filament.push_back(filament);
	m_sizer->Add(filament, wxSizerFlags().Border(wxALL, FromDIP(4)));
	this->GetParent()->Layout();
    update_scroll_height();
    this->GetParent()->Layout();

    std::string res = wxGetApp().app_config->get("is_currentMachine_Colors");
    bool isColors = res == "1";
    update_box_filament_sync_state(isColors);
	refresh_filament_grouping_visibility();
	return true;
}

void FilamentPanel::reflow_for_width()
{
    if (wxWindow* parent = GetParent()) {
        const int width = parent->GetClientSize().GetWidth();
        if (width > 0) {
            SetMinSize(wxSize(0, -1));
            SetSize(wxSize(width, GetSize().GetHeight()));
            InvalidateBestSize();
        }
    }

    update_scroll_height();
    Layout();
}

void FilamentPanel::update_scroll_height()
{
    if (!m_filament_scrolled || !m_sizer)
        return;

    const int card_width = FromDIP(FILAMENT_BTN_WIDTH + 8);
    const int row_height = FromDIP(FILAMENT_BTN_HEIGHT + 8);
    const int width = std::max(card_width, GetClientSize().GetWidth() - FromDIP(12));
    size_t columns = std::max(1, width / card_width);
    size_t rows = (m_vt_filament.size() + columns - 1) / columns;
    const int scrollbar_width = rows > 5
        ? std::max(0, wxSystemSettings::GetMetric(wxSYS_VSCROLL_X, m_filament_scrolled)) : 0;
    if (rows > 5) {
        // The scrollbar can take a card's space when the sidebar is narrow.
        columns = std::max(1, (width - scrollbar_width) / card_width);
        rows = (m_vt_filament.size() + columns - 1) / columns;
    }
    const int height = static_cast<int>(std::min<size_t>(rows, 5)) * row_height;

    m_filament_scrolled->SetMinSize(wxSize(0, height));
    m_filament_scrolled->SetMaxSize(wxSize(-1, height));
    m_box_sizer->Layout();
    m_filament_scrolled->ShowScrollbars(wxSHOW_SB_NEVER, rows > 5 ? wxSHOW_SB_ALWAYS : wxSHOW_SB_NEVER);

    const int content_width = m_filament_scrolled->GetClientSize().GetWidth();
    const int content_height = static_cast<int>(rows) * row_height;
    m_filament_scrolled->SetVirtualSize(content_width, content_height);
    if (rows <= 5)
        m_filament_scrolled->Scroll(0, 0);
    const wxPoint origin = m_filament_scrolled->CalcScrolledPosition(wxPoint(0, 0));
    m_filament_content->SetSize(origin.x, origin.y, content_width, content_height);
    m_filament_content->Layout();

    // Use the rows actually laid out by wxWrapSizer. At some sidebar widths its
    // column count differs from the estimate above, leaving an oversized scroll range.
    int actual_height = 0;
    int first_row_y = -1;
    int row_stride = 0;
    for (FilamentItem* item : m_vt_filament) {
        if (!item || !item->IsShown()) continue;
        const wxRect card = item->GetRect();
        actual_height = std::max(actual_height, card.GetBottom() + 1 + FromDIP(4));
        if (first_row_y < 0) first_row_y = card.y;
        else if (row_stride == 0 && card.y > first_row_y) row_stride = card.y - first_row_y;
    }
    int scroll_unit_x, scroll_unit_y;
    m_filament_scrolled->GetScrollPixelsPerUnit(&scroll_unit_x, &scroll_unit_y);
    if (row_stride > 0 && row_stride != scroll_unit_y)
        m_filament_scrolled->SetScrollRate(0, row_stride);
    const int final_height = actual_height > 0 ? actual_height : content_height;
    if (final_height != content_height)
        m_filament_scrolled->SetVirtualSize(content_width, final_height);
    const wxPoint adjusted_origin = m_filament_scrolled->CalcScrolledPosition(wxPoint(0, 0));
    m_filament_content->SetSize(adjusted_origin.x, adjusted_origin.y, content_width, final_height);
}

void FilamentPanel::update_box_filament_sync_state(bool sync)
{
	for (auto& f : this->m_vt_filament)
	{
		f->update_box_sync_state(sync);
	}

	Refresh();
}

void FilamentPanel::reset_filament_sync_state()
{
	for (auto& f : this->m_vt_filament)
	{
		f->update_box_sync_color("#ffffff");
        f->resetCFS(true);
	}
}

void FilamentPanel::reset_device_filament_mapping_to_cfs()
{
    for (auto& item : this->m_vt_filament) {
        if (!item)
            continue;

        item->update_box_sync_state(true);
        item->update_box_sync_color("#ffffff");
        item->resetCFS(true);
        item->Refresh();
    }

    Refresh();
}

std::string FilamentPanel::get_filament_map_string()
{
    wxString mapstr = "";
    for (auto& item : this->m_vt_filament) {
             mapstr +=  wxString::Format("%s;", item->boxname());
    }
    return mapstr.ToStdString();
}
void FilamentPanel::resetFilamentToCFS() {
    for (auto& item : this->m_vt_filament) {
        item->resetCFS(true);
    }
}

void FilamentPanel::updateLastFilament(const std::vector<std::string>& presetName)
{
    int i = 0;
    for (auto& item : this->m_vt_filament) {
        if (i >= presetName.size())
            break;
        item->set_filament_selection(from_u8(presetName[i++]));
    }
}

void FilamentPanel::backup_extruder_colors()
{
    m_backup_extruder_colors.clear();

    m_backup_extruder_colors = Slic3r::GUI::wxGetApp().plater()->get_extruder_colors_from_plater_config();
}

void FilamentPanel::restore_prev_extruder_colors()
{
	if(m_backup_extruder_colors.size() == 0)
        return;
    
    std::vector<int> plate_extruders;
    for(int i = 0; i < m_backup_extruder_colors.size(); i++)
    {
        plate_extruders.emplace_back(i+1);
    }

    // get current color
    Slic3r::DynamicPrintConfig* cfg    = &Slic3r::GUI::wxGetApp().preset_bundle->project_config;
    auto colors = static_cast<Slic3r::ConfigOptionStrings*>(cfg->option("filament_colour")->clone());

	int extruderId = 0;  
	for(int i = 0; i < m_backup_extruder_colors.size(); i++)
	{
        extruderId = plate_extruders[i]-1;

        if(extruderId < 0 || extruderId >= colors->values.size()) continue;

        colors->values[extruderId]       = m_backup_extruder_colors[i];

    }

	cfg->set_key_value("filament_colour", colors);

	Slic3r::GUI::wxGetApp().plater()->get_view3D_canvas3D()->update_volumes_colors_by_config(cfg);

	Slic3r::GUI::wxGetApp().plater()->update_all_plate_thumbnails(true);
}

std::vector<FilamentItem*> FilamentPanel::get_filament_items() 
{
    return m_vt_filament; 
}

namespace {

struct AutoMappingPreparation
{
    std::vector<std::pair<int, DM::Material>> valid_materials;
    std::map<std::string, std::string>        enabled_profiles;
    bool                                      is_cfs_mini      = false;
    bool                                      profile_available = false;
    long long                                 profile_wait_ms   = 0;
    long long                                 prepare_ms        = 0;
    std::string                               error;
};

std::string normalize_material_field(std::string value)
{
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    value.erase(std::find_if(value.rbegin(), value.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), value.end());
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

std::string material_name_prefix(const std::string& value)
{
    const size_t at = value.find('@');
    return at == std::string::npos ? value : value.substr(0, at);
}

std::string material_key(const std::string& vendor, const std::string& type, const std::string& name)
{
    return normalize_material_field(vendor) + '\x1f' + normalize_material_field(type) + '\x1f' + normalize_material_field(name);
}

AutoMappingPreparation prepare_auto_mapping(const DM::Device& device_data,
                                            const std::map<std::string, std::string>& currently_enabled)
{
    AutoMappingPreparation result;
    const auto prepare_started_at = std::chrono::steady_clock::now();

    std::unordered_set<std::string> device_material_keys;
    for (const auto& material_box : device_data.materialBoxes) {
        if (material_box.box_type != 0 && material_box.box_type != 2)
            continue;

        result.is_cfs_mini |= material_box.box_type == 2;
        for (const auto& material : material_box.materials) {
            if (material.color.empty())
                continue;
            result.valid_materials.emplace_back(material_box.box_id, material);
            device_material_keys.emplace(material_key(material.vendor, material.type, material.name));
        }
    }

    if (result.valid_materials.empty()) {
        result.error = "No valid device filament was found";
        return result;
    }

    const auto wait_started_at = std::chrono::steady_clock::now();
    auto* loader = Slic3r::ProfileFamilyLoader::get_instance();
    loader->wait_until_loaded();
    result.profile_wait_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 std::chrono::steady_clock::now() - wait_started_at).count();

    json filament_profiles;
    loader->get_filament_result(filament_profiles);
    if (!filament_profiles.is_object()) {
        result.error = "Filament profile cache is unavailable";
        return result;
    }

    for (auto it = filament_profiles.begin(); it != filament_profiles.end(); ++it) {
        if (!it.value().is_object())
            continue;

        const bool was_enabled = currently_enabled.find(it.key()) != currently_enabled.end();
        if (was_enabled) {
            result.enabled_profiles[it.key()] = "true";
            result.profile_available = true;
            continue;
        }

        const std::string vendor = it.value().value("vendor", "");
        const std::string type   = it.value().value("type", "");
        const std::string name   = material_name_prefix(it.value().value("name", ""));
        if (device_material_keys.find(material_key(vendor, type, name)) != device_material_keys.end())
            result.enabled_profiles[it.key()] = "true";
    }

    result.prepare_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - prepare_started_at).count();
    return result;
}

} // namespace

bool FilamentPanel::SetFilamentProfile(const std::map<std::string, std::string>& section_new)
{
    if (section_new.empty() || wxGetApp().app_config == nullptr || wxGetApp().preset_bundle == nullptr)
        return false;

    const auto current_section = wxGetApp().app_config->has_section(Slic3r::AppConfig::SECTION_FILAMENTS) ?
                                     wxGetApp().app_config->get_section(Slic3r::AppConfig::SECTION_FILAMENTS) :
                                     std::map<std::string, std::string>();
    if (current_section == section_new)
        return false;

    Slic3r::AppConfig appconfig;
    appconfig.set_section(Slic3r::AppConfig::SECTION_FILAMENTS, section_new);
    for (auto& preset : wxGetApp().preset_bundle->filaments)
        preset.set_visible_from_appconfig(appconfig);

    wxGetApp().app_config->set_section(Slic3r::AppConfig::SECTION_FILAMENTS, section_new);
    wxGetApp().app_config->save();
    return true;
}

void FilamentPanel::on_auto_mapping_filament(const DM::Device& device_data,
                                             AutoMappingCompletion completion,
                                             AutoMappingValidator validator)
{
    const auto enabled_filaments = wxGetApp().app_config != nullptr &&
                                           wxGetApp().app_config->has_section(Slic3r::AppConfig::SECTION_FILAMENTS) ?
                                       wxGetApp().app_config->get_section(Slic3r::AppConfig::SECTION_FILAMENTS) :
                                       std::map<std::string, std::string>();
    const std::weak_ptr<int> lifetime_token = m_lifetime_token;

    boost::thread worker = Slic3r::create_thread(
        [this, lifetime_token, device_data, enabled_filaments, completion = std::move(completion),
         validator = std::move(validator)]() mutable {
            Slic3r::set_current_thread_name("auto-filament-map");
            AutoMappingPreparation preparation;
            try {
                preparation = prepare_auto_mapping(device_data, enabled_filaments);
            } catch (const std::exception& e) {
                preparation.error = e.what();
            } catch (...) {
                preparation.error = "Unknown auto-mapping error";
            }

            if (lifetime_token.expired())
                return;

            wxGetApp().CallAfter([this, lifetime_token, preparation = std::move(preparation), completion = std::move(completion),
                                  validator = std::move(validator)]() mutable {
                if (lifetime_token.expired())
                    return;
                if (validator && !validator()) {
                    if (completion)
                        completion(false, "Auto-mapping request is no longer current");
                    return;
                }
                if (!preparation.error.empty()) {
                    BOOST_LOG_TRIVIAL(error) << "Auto filament mapping preparation failed: " << preparation.error;
                    if (completion)
                        completion(false, preparation.error);
                    return;
                }

                const auto apply_started_at = std::chrono::steady_clock::now();
                apply_auto_mapping_filament(std::move(preparation.valid_materials), preparation.is_cfs_mini,
                                            std::move(preparation.enabled_profiles), preparation.profile_available);
                const auto apply_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                          std::chrono::steady_clock::now() - apply_started_at).count();
                BOOST_LOG_TRIVIAL(info) << "Auto filament mapping timings: profile_wait_ms=" << preparation.profile_wait_ms
                                        << ", prepare_ms=" << preparation.prepare_ms << ", apply_ms=" << apply_ms;
                if (completion)
                    completion(true, {});
            });
        });
    worker.detach();
}

void FilamentPanel::apply_auto_mapping_filament(std::vector<std::pair<int, DM::Material>> validMaterials,
                                                bool isCfsMini,
                                                std::map<std::string, std::string> section_new,
                                                bool profile_available)
{
    const bool profile_changed = profile_available && SetFilamentProfile(section_new);

    if (m_vt_filament.size() != validMaterials.size()) {
        const bool need_more_filaments = m_vt_filament.size() < validMaterials.size();

        size_t filament_count = validMaterials.size();
        if (Slic3r::GUI::wxGetApp().preset_bundle->is_the_only_edited_filament(filament_count) || (filament_count == 1)) {
            Slic3r::GUI::wxGetApp().get_tab(Slic3r::Preset::TYPE_FILAMENT)->select_preset(Slic3r::GUI::wxGetApp().preset_bundle->filament_presets[0], false, "", true);
        }

        if (need_more_filaments) {
            wxColour    new_col   = Slic3r::GUI::Plater::get_next_color_for_filament();
            std::string new_color = new_col.GetAsString(wxC2S_HTML_SYNTAX).ToStdString();
            Slic3r::GUI::wxGetApp().preset_bundle->set_num_filaments(filament_count, new_color);
        } else {
            Slic3r::GUI::wxGetApp().preset_bundle->set_num_filaments(filament_count);
        }
        
        Slic3r::GUI::wxGetApp().plater()->on_filaments_change(filament_count);
        Slic3r::GUI::wxGetApp().get_tab(Slic3r::Preset::TYPE_PRINT)->update();
    }

    assert(m_vt_filament.size() == validMaterials.size());
    if (m_vt_filament.size() != validMaterials.size())
        return;

    auto* preset_bundle = Slic3r::GUI::wxGetApp().preset_bundle;
    auto* app_config    = Slic3r::GUI::wxGetApp().app_config;
    auto* plater        = Slic3r::GUI::wxGetApp().plater();
    if (preset_bundle == nullptr || app_config == nullptr || plater == nullptr)
        return;

    auto* color_option = preset_bundle->project_config.option<Slic3r::ConfigOptionStrings>("filament_colour");
    if (color_option == nullptr)
        return;

    std::vector<std::string> final_colors = color_option->values;
    final_colors.resize(validMaterials.size(), "#000000");
    bool colors_changed = false;
    bool presets_changed = false;

    LoginTip::getInstance().resetHasSkipToLogin();
    for (size_t i = 0; i < validMaterials.size(); ++i) {
        auto& item = m_vt_filament[i];
        int   filamentUserMaterialRet = 0; // 1:不是用户预设, 0:是用户预设，且用户账号正常, wxID_YES:点击了登录
        filamentUserMaterialRet       = LoginTip::getInstance().isFilamentUserMaterialValid(validMaterials[i].second.userMaterial);
        if (filamentUserMaterialRet == (int) wxID_YES) { //  点击了登录
            continue;
        }

        std::string new_filament_color = validMaterials[i].second.color;
        std::string new_filament_name  = validMaterials[i].second.name;

        char     index_char          = 'A' + (validMaterials[i].second.material_id % 4); // Calculate the letter part (A, B, C, D)
        wxString material_sync_label = wxString::Format("%d%c", validMaterials[i].first, index_char);

        if (isCfsMini && validMaterials[i].first == 5) 
        {
            material_sync_label = "CFS";
        }

        if (filamentUserMaterialRet != 0 && filamentUserMaterialRet != 1 && filamentUserMaterialRet != (int) wxID_YES) {
            new_filament_name = "";
            material_sync_label = "";
        } else if (filamentUserMaterialRet == 0) {
            new_filament_name = from_u8(validMaterials[i].second.name).ToStdString();
        }

        if (final_colors[i] != new_filament_color) {
            final_colors[i] = new_filament_color;
            colors_changed = true;
            if (i < preset_bundle->ams_multi_color_filment.size())
                preset_bundle->ams_multi_color_filment[i].clear();
        }

        const std::string preset_name = item->set_filament_selection(from_u8(new_filament_name), false);
        if (!preset_name.empty() && preset_bundle->filament_presets[i] != preset_name) {
            preset_bundle->set_filament_preset(i, preset_name);
            presets_changed = true;
        }

        item->update_box_sync_state(true, material_sync_label);
        item->update_box_sync_color(new_filament_color);
        item->resetCFS(false);
        if (material_sync_label.empty()) {
            item->resetCFS(true);
        }
    }

    if (colors_changed)
        color_option->values = std::move(final_colors);

    const bool selection_changed = colors_changed || presets_changed;
    if (selection_changed) {
        const bool recalc_flush = app_config->get("auto_calculate") == "true" ||
                                  app_config->get("auto_calculate_when_filament_change") == "true";
        if (recalc_flush) {
            // Recalculate all affected rows/columns in memory. Persistence and
            // background slicing are triggered once after the entire batch.
            for (size_t i = 0; i < validMaterials.size(); ++i)
                plater->sidebar().auto_calc_flushing_volumes(static_cast<int>(i), false);
        }

        plater->update_project_dirty_from_presets();
        preset_bundle->export_selections(*app_config);
        plater->reset_scene_filament_source_snapshot();
        plater->on_config_change(preset_bundle->full_config());
        plater->sidebar().update_dynamic_filament_list();
        Slic3r::put_other_changes();

        for (PartPlate* plate : plater->get_partplate_list().get_plate_list())
            plate->update_slice_result_valid_state(false);

        if (colors_changed) {
            preset_bundle->mixed_filaments.refresh_display_colors(color_option->values);
            plater->sidebar().update_mixed_filament_panel(true);
            if (auto* canvas = plater->get_view3D_canvas3D())
                canvas->reload_scene(false);
        }
    }

    // Refresh controls without writing config or scheduling slicing per item.
    if (profile_changed || selection_changed) {
        for (auto& item : m_vt_filament)
            item->update(false);
    }

    m_sizer->Layout();

    for (auto& item : m_vt_filament) {
        item->Refresh();
    }
}

void FilamentPanel::on_sync_one_filament(int filament_index, const std::string& new_filament_color, const std::string& new_filament_name, const wxString& sync_label)
{
	if(filament_index < 0 || filament_index >= m_vt_filament.size())
		return;

    auto& item = m_vt_filament[filament_index];
    item->update_bk_color(new_filament_color);
    item->set_filament_selection(new_filament_name);
	item->update_box_sync_state(true, sync_label);
	item->update_box_sync_color(new_filament_color);
    item->resetCFS(false);
    if (sync_label.empty()) {
        item->resetCFS(true);
    }

	item->Refresh();
}

void FilamentPanel::sync_box_filament_state(int filament_index, const std::string& new_filament_color, const wxString& sync_label)
{
    if (filament_index < 0 || filament_index >= m_vt_filament.size())
        return;

    auto& item = m_vt_filament[filament_index];
    item->update_box_sync_state(true, sync_label);
    item->update_box_sync_color(new_filament_color);
    item->resetCFS(false);
    if (sync_label.empty()) {
        item->resetCFS(true);
    }

    item->Refresh();
}

void FilamentPanel::on_re_sync_all_filaments(const std::string& selected_device_ip)
{
	auto device = DM::DataCenter::Ins().get_printer_data(selected_device_ip);

	if(m_vt_filament.size() != device.boxColorInfos.size())
	{
		bool need_more_filaments = false;
		if(m_vt_filament.size() < device.boxColorInfos.size())
		{
			need_more_filaments = true;
		}

        size_t filament_count = device.boxColorInfos.size();
        if (Slic3r::GUI::wxGetApp().preset_bundle->is_the_only_edited_filament(filament_count) || (filament_count == 1)) {
            Slic3r::GUI::wxGetApp().get_tab(Slic3r::Preset::TYPE_FILAMENT)->select_preset(Slic3r::GUI::wxGetApp().preset_bundle->filament_presets[0], false, "", true);
        }

		if(need_more_filaments)
		{
            wxColour    new_col   = Slic3r::GUI::Plater::get_next_color_for_filament();
            std::string new_color = new_col.GetAsString(wxC2S_HTML_SYNTAX).ToStdString();
            Slic3r::GUI::wxGetApp().preset_bundle->set_num_filaments(filament_count, new_color);
        }
		else
		{
			Slic3r::GUI::wxGetApp().preset_bundle->set_num_filaments(filament_count);
		}
        wxGetApp().preset_bundle->update_filament_presets = false;
        Slic3r::GUI::wxGetApp().plater()->on_filaments_change(filament_count);
        wxGetApp().preset_bundle->update_filament_presets = true;
        Slic3r::GUI::wxGetApp().get_tab(Slic3r::Preset::TYPE_PRINT)->update();
        Slic3r::GUI::wxGetApp().preset_bundle->export_selections(*Slic3r::GUI::wxGetApp().app_config);

		if(need_more_filaments)
		{
			Slic3r::GUI::wxGetApp().plater()->sidebar().auto_calc_flushing_volumes(filament_count - 1);
		}
    }

    assert(m_vt_filament.size() == device.boxColorInfos.size());

	// sychronize normal multi-color box first, and then extra box
	int normalIdx = 0;  
	for(int i = 0; i < device.boxColorInfos.size(); i++)
	{
		// normal multi-color box
		if(0 == device.boxColorInfos[i].boxType && !device.boxColorInfos[i].color.empty())
		{
            auto& item = m_vt_filament[normalIdx];
            item->update_bk_color(device.boxColorInfos[i].color);
            item->set_filament_selection(device.boxColorInfos[i].filamentName);

			normalIdx += 1;
        }
	}

	for(int i = 0; i < device.boxColorInfos.size(); i++)
	{
		// extra box
		int extraIdx = normalIdx;
		if(1 == device.boxColorInfos[i].boxType && !device.boxColorInfos[i].color.empty())
		{
            auto& item = m_vt_filament[extraIdx];
            item->update_bk_color(device.boxColorInfos[i].color);
            item->set_filament_selection(device.boxColorInfos[i].filamentName);

			extraIdx += 1;
        }
	}

    m_sizer->Layout();

	// trigger a repaint event to fix the display issue after sychronizing
    for (auto& item : m_vt_filament) {
        item->Refresh();
    }
}

bool FilamentPanel::can_add()
{
	return m_vt_filament.size() < m_max_count;
}

bool FilamentPanel::can_delete()
{
	return m_vt_filament.size() > 1;
}

void FilamentPanel::clear_all()
{
    if (!m_sizer)
        return;

    PopupWindowManager::Get().CloseAll();

    for (auto* item : m_vt_filament) {
        if (!item)
            continue;
        m_sizer->Detach(item);
        item->Destroy();
    }

    m_vt_filament.clear();

    update_scroll_height();
    Layout();
    if (GetParent())
        GetParent()->Layout();
}

void FilamentPanel::del_filament(int index/*=-1*/)
{
	if (-1 == index && m_vt_filament.size() != 0)
	{
		auto& item = m_vt_filament[m_vt_filament.size() - 1];
		m_sizer->Detach(item);
		item->Destroy();
		this->Layout();

		m_vt_filament.erase(m_vt_filament.end() - 1);

		// layout
		this->to_small(m_vt_filament.size() > m_small_count);
	}
	else
	{
        const auto&item = m_vt_filament.begin() + index;
		if (item != m_vt_filament.end())
		{
            m_sizer->Detach(*item);
            (*item)->Destroy();
            this->Layout();

            m_vt_filament.erase(item);

            // layout
            this->to_small(m_vt_filament.size() > m_small_count);
		}
	}

    update_scroll_height();
	this->GetParent()->Layout();
}

void FilamentPanel::to_small(bool bSmall /*= true*/)
{
	for (auto& f : this->m_vt_filament)
	{
		f->to_small(bSmall);
	}
}

void FilamentPanel::update(int index /*=-1*/)
{
	refresh_filament_grouping_visibility();
	const auto* map = wxGetApp().preset_bundle->project_config.option<Slic3r::ConfigOptionInts>("filament_map", false);
	if (map != nullptr) {
		for (size_t i = 0; i < m_vt_filament.size(); ++i)
			m_vt_filament[i]->set_nozzle_no(i < map->values.size() ? map->values[i] : 1);
	}
	if (-1 == index)
	{
        for (auto& item : m_vt_filament) {
            item->update();
        }
        if (wxGetApp().preset_bundle->update_filament_presets) {
            wxGetApp().preset_bundle->filament_presets.resize(m_vt_filament.size());
            for (size_t i = 0; i < m_vt_filament.size(); ++i) {
                wxString name = m_vt_filament[i]->preset_name();
                if (name.empty())
                    continue;
                wxGetApp().preset_bundle->filament_presets[i] = name.ToStdString();
            }
        }
	}
	else
	{
        for (auto& item : m_vt_filament) {
			if (item->index() == index)
			{
                item->update();
				break;
			}
        }
	}
}

void FilamentPanel::sys_color_changed()
{
    for (auto& item : m_vt_filament) {
        item->sys_color_changed();
    }
}

void FilamentPanel::msw_rescale()
{
    for (auto& item : m_vt_filament) {
        item->msw_rescale();
    }
    update_scroll_height();
}

size_t FilamentPanel::size() {
	return m_vt_filament.size(); 
}

void FilamentPanel::paintEvent(wxPaintEvent& evt)
{
	wxPaintDC dc(this);
}


wxBEGIN_EVENT_TABLE(BoxColorPopPanel, PopupWindow)
    EVT_BUTTON(wxID_ANY, BoxColorPopPanel::OnFirstColumnButtonClicked)
wxEND_EVENT_TABLE()
BoxColorPopPanel::BoxColorPopPanel(wxWindow* parent)
    : PopupWindow(parent, wxBORDER_SIMPLE)
{
	m_mainSizer = new wxBoxSizer(wxHORIZONTAL);
    m_firstColumnSizer = new wxBoxSizer(wxVERTICAL);
    m_secondColumnSizer = new wxBoxSizer(wxVERTICAL);

	    // Set background color
    SetBackgroundColour(wxColour(54, 54, 56)); // Light grey background color
	SetSize(FromDIP(200), FromDIP(200));

    // Create a panel for the second column and add it to the main sizer
    m_secondColumnPanel = new wxPanel(this);
    m_secondColumnPanel->SetSizer(m_secondColumnSizer);
    m_secondColumnPanel->Hide();

    // Create a white static line
    wxStaticLine* separatorLine = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL);
    separatorLine->SetBackgroundColour(*wxBLACK);

    m_mainSizer->Add(m_firstColumnSizer, 0, wxALL, 5);
	m_mainSizer->Add(separatorLine, 0, wxEXPAND | wxALL, 0);
    m_mainSizer->Add(m_secondColumnPanel, 0, wxALL, 5);

    #if __APPLE__
        Bind(wxEVT_LEFT_DOWN, &BoxColorPopPanel::on_left_down, this); 
     #endif
    SetSizer(m_mainSizer);
    Layout();
}
void BoxColorPopPanel::on_left_down(wxMouseEvent &evt)
{
   
    auto pos = ClientToScreen(evt.GetPosition());
    wxWindowList& children = m_secondColumnPanel->GetChildren();
    for (wxWindowList::iterator it = children.begin(); it != children.end(); ++it) {
        wxWindow* item = *it;
        auto p_rect = item->ClientToScreen(wxPoint(0, 0));
        if (pos.x > p_rect.x && pos.y > p_rect.y && pos.x < (p_rect.x + item->GetSize().x) && pos.y < (p_rect.y + item->GetSize().y)) {
            wxCommandEvent event(wxEVT_BUTTON, GetId());
		    event.SetEventObject(item);
             OnSecondColumnItemClicked(event);
             this->Dismiss();
             //BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << "on_left_down"<<r_pos.x<<"\n";
        }
        
    }
    auto firstChildren = m_firstColumnSizer->GetChildren();
    for(wxSizerItem* firstItem: firstChildren)
    {   
        wxWindow* item = firstItem->GetWindow();
        auto p_rect = item->ClientToScreen(wxPoint(0, 0));
        if (pos.x > p_rect.x && pos.y > p_rect.y && pos.x < (p_rect.x + item->GetSize().x) && pos.y < (p_rect.y + item->GetSize().y)) {
            wxCommandEvent event(wxEVT_BUTTON, GetId());
		    event.SetEventObject(item);
            OnFirstColumnButtonClicked(event);
             //BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << "on_left_down"<<r_pos.x<<"\n";
        }
    }
    
}
BoxColorPopPanel::~BoxColorPopPanel()
{
}

void BoxColorPopPanel::OnMouseEnter(wxMouseEvent& event)
{

}

void BoxColorPopPanel::OnMouseLeave(wxMouseEvent& event)
{
}

void BoxColorPopPanel::select_first_on_show()
{
    if (!m_firstColumnSizer->GetChildren().IsEmpty()) {
        wxSizerItem* firstItem = m_firstColumnSizer->GetChildren().GetFirst()->GetData();
        if (firstItem) {
            wxWindow* window = firstItem->GetWindow();
            if (window) {
                wxButton* firstButton = dynamic_cast<wxButton*>(window);
                if (firstButton) {
                    wxCommandEvent clickEvent(wxEVT_BUTTON, firstButton->GetId());
                    clickEvent.SetEventObject(firstButton);
                    OnFirstColumnButtonClicked(clickEvent);
                }
            }
        }
    }
}

void BoxColorPopPanel::OnFirstColumnButtonClicked(wxCommandEvent& event)
{
    try {
        m_secondColumnSizer->Clear(true);

        wxButton* button = dynamic_cast<wxButton*>(event.GetEventObject());
        if (!button) return;

        // 重置所有按钮的样式
        for (auto& child : m_firstColumnSizer->GetChildren()) {
            wxButton* btn = dynamic_cast<wxButton*>(child->GetWindow());
            if (btn) {
                btn->SetBackgroundColour(wxColour("#E2E5E9")); // 重置为默认背景色
                btn->Refresh();
            }
        }

        // 设置选中按钮的样式
        button->SetBackgroundColour(wxColour(0, 225, 0)); // 设置选中按钮的背景色为绿色
        button->Refresh();

        // 使用 std::intptr_t 来存储指针值

        std::intptr_t boxId = reinterpret_cast<std::intptr_t>(button->GetClientData());

        const DM::MaterialBox* material_box_info = nullptr;
        const DM::MaterialBox* ext_material_box_info = nullptr;
    
        for (const auto& box_info : m_device_data.materialBoxes) {

            if (box_info.box_id == boxId || box_info.box_type == 2) {
                material_box_info = &box_info;
                //break;
            }
            if (box_info.box_type == 1) {
                ext_material_box_info = &box_info;
            }
        }
        

        if (!material_box_info && !ext_material_box_info ) return;

        int  material_id        = 0;
        int  box_id        = 0;
        bool is_ext_material = false;
        bool has_exact_material = false;
        
        int startIndex = -1;
        if (m_device_data.cfsName == "MF049") {
            startIndex = 0;                     // MF049设备跳过外置料架
        }
        for (int i = startIndex; i < 4; i++) {

            FilamentColorSelectionItem* filament_item = new FilamentColorSelectionItem(m_secondColumnPanel, wxSize(FromDIP(120), FromDIP(20)));
            assert(filament_item);

            try {
                has_exact_material = false;
                if(i==-1)
                {
                    if(ext_material_box_info)
                    {
                        box_id = ext_material_box_info->box_id;
                        is_ext_material = true;
                        for (const auto& material : ext_material_box_info->materials) {
                            if (material.material_id == material_id && ext_material_box_info->box_type == 1 && !material.color.empty()) {
                                filament_item->set_sync_state(true);
                                filament_item->set_is_ext(is_ext_material);
                                filament_item->update_item_info_by_material(ext_material_box_info->box_id, material);
                                has_exact_material = true;
                                break;
                            }
                        }
                    }
                }else{
                    material_id = i;
                    // check whether the array has the exact material
                    if(material_box_info)
                    {
                        box_id = material_box_info->box_id;
                        is_ext_material = false;
                        for (const auto& material : material_box_info->materials) {
                            if (material.material_id == material_id && (material_box_info->box_type == 0 ||
                                material_box_info->box_type == 2) && !material.color.empty()) {
                                filament_item->set_sync_state(true);
                                filament_item->set_is_ext(is_ext_material);
                                filament_item->update_item_info_by_material(material_box_info->box_id, material,material_box_info->box_type);
                                has_exact_material = true;
                                break;
                            }
                        }
                    }else{
                        delete filament_item;
                        break;
                    }
                }
                if (!has_exact_material) {
                    DM::Material tmp_material;
                    tmp_material.material_id = material_id;
                    tmp_material.color      = "#808080"; // grey
                    tmp_material.type       = "?";
                    int state = 0;
                    const DM::MaterialBox* src_box = is_ext_material ? ext_material_box_info : material_box_info;
                    if (src_box) {
                        for (const auto& m : src_box->materials) {
                            if (m.material_id == material_id) {
                                state = m.state;
                                break;
                            }
                        }
                    }
                    tmp_material.state = state;
                    filament_item->set_sync_state(false);
                    filament_item->set_is_ext(is_ext_material);
                    filament_item->update_item_info_by_material(box_id, tmp_material);
                }

                // Bind the click event for the second column items
                filament_item->Bind(wxEVT_BUTTON, &BoxColorPopPanel::OnSecondColumnItemClicked, this);

                m_secondColumnSizer->Add(filament_item, 0, wxALL, 3);
            }
            catch (const std::exception& ex) {
                if(filament_item)
                {
                    filament_item->Destroy();
                }
                std::cerr << ex.what() << std::endl;
            }

        }

        // Show the second column panel when a button in the first column is clicked
        m_secondColumnPanel->Layout();
        m_secondColumnPanel->Show();
        Layout();
    } 
    catch (const std::exception& ex) 
    {
        if(m_firstColumnSizer)
        {
            m_firstColumnSizer->Clear(true);
        }

        if(m_secondColumnSizer)
        {
            m_secondColumnSizer->Clear(true);
        }

        std::cerr << ex.what() << std::endl;
    }

}

void BoxColorPopPanel::OnSecondColumnItemClicked(wxCommandEvent& event)
{
    FilamentColorSelectionItem* item = dynamic_cast<FilamentColorSelectionItem*>(event.GetEventObject());
    if (!item) return;

	if(false == item->get_sync_state())
	{
		return;
	}

    LoginTip::getInstance().resetHasSkipToLogin();
    int filamentUserMaterialRet = 0; // 1:不是用户预设, 0:是用户预设，且用户账号正常, wxID_YES:点击了登录
    filamentUserMaterialRet = LoginTip::getInstance().isFilamentUserMaterialValid(item->getUserMaterial());
    if (filamentUserMaterialRet == (int)wxID_YES) {  //  点击了登录
        return;
    }

    // Perform the logic you want when an item in the second column is clicked
    // wxLogMessage("Second column item clicked");
	wxColour item_color = item->GetColor();  // GetColor()
	std::string new_filament_color = item_color.GetAsString(wxC2S_HTML_SYNTAX).ToStdString();
	std::string new_filament_name = item->get_filament_name();
    wxString syncLabel = item->get_material_index_info();
    if (filamentUserMaterialRet != 0 && filamentUserMaterialRet != 1 && filamentUserMaterialRet != (int)wxID_YES) {
        new_filament_name = "";
        syncLabel         = "";
    } else if (filamentUserMaterialRet == 0) {
        new_filament_name = from_u8(item->get_filament_name()).ToStdString();
    }

	FilamentPanel* filament_panel = dynamic_cast<FilamentPanel*>(Slic3r::GUI::wxGetApp().sidebar().filament_panel());
	if(filament_panel)
	{
		filament_panel->on_sync_one_filament(m_filament_item_index, new_filament_color, new_filament_name, syncLabel);
	}
}

void BoxColorPopPanel::init_by_device_data(const DM::Device& device_data)
{
	m_device_data = device_data;

	m_firstColumnSizer->Clear(true);
	m_secondColumnSizer->Clear(true);

    int cfsBoxSize = 0;

    //add cfs1, cfs2, cfs3, cfs4 to first column
    for (const auto& material_box_info : m_device_data.materialBoxes) {

		if (0 == material_box_info.box_type) {  // normal multi-color box

        wxString cfs_index_info = wxString::Format("cfs%d", material_box_info.box_id);  // CFS1, CFS2, CFS3, CFS4

		wxButton* button = new wxButton(this, wxID_ANY, cfs_index_info, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
        button->SetMinSize(wxSize(FromDIP(50), FromDIP(24)));
        button->SetMaxSize(wxSize(FromDIP(50), FromDIP(24)));
		button->SetClientData(reinterpret_cast<void*>(material_box_info.box_id));
        m_firstColumnSizer->Add(button, 0, wxALL, 5);

        ++cfsBoxSize;
    } 

    }
    if(cfsBoxSize == 0)
    {
        //add ext to first column
        wxButton* button = new wxButton(this, wxID_ANY, "EXT", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
        button->SetMinSize(wxSize(FromDIP(50), FromDIP(24)));
        button->SetMaxSize(wxSize(FromDIP(50), FromDIP(24)));
        button->SetClientData(reinterpret_cast<void*>(-1)); // Set the box_id to -1 for EXT
        m_firstColumnSizer->Add(button, 0, wxALL, 5);
    }

    if (cfsBoxSize <= 1) {
        m_firstColumnSizer->Hide(size_t(0));
        m_mainSizer->Hide(size_t(0));
        m_mainSizer->Hide(size_t(1));
        m_mainSizer->Layout();
        this->SetMinSize(wxSize(FromDIP(128), FromDIP(150)));
        this->SetMaxSize(wxSize(FromDIP(128), FromDIP(200)));
    } else {
        m_firstColumnSizer->Show(size_t(0));
        m_mainSizer->Show(size_t(0));
        m_mainSizer->Show(size_t(1));
        m_mainSizer->Layout();
        this->SetMinSize(wxSize(FromDIP(200), FromDIP(150)));
        this->SetMaxSize(wxSize(FromDIP(200), FromDIP(200)));
    }

	Layout();
    Fit();
}

void BoxColorPopPanel::set_filament_item_index(int index)
{
	m_filament_item_index = index;
}

FilamentColorSelectionItem::FilamentColorSelectionItem(wxWindow* parent, const wxSize& size)
	: wxButton(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, size, wxBORDER_SIMPLE)  // wxBU_EXACTFIT | wxNO_BORDER
{
    Bind(wxEVT_PAINT, &FilamentColorSelectionItem::OnPaint, this);
}

FilamentColorSelectionItem::~FilamentColorSelectionItem()
{
    // Any necessary cleanup code here
}

void FilamentColorSelectionItem::set_sync_state(bool bSync)
{
    m_sync = bSync;
}

bool FilamentColorSelectionItem::get_sync_state()
{
	return m_sync;
}

void FilamentColorSelectionItem::set_is_ext(bool is_ext)
{
    m_is_ext = is_ext;
}

void FilamentColorSelectionItem::SetColor(const wxColour& color)
{
    m_bk_color = color;
    Refresh(); // Trigger a repaint
}

wxColour FilamentColorSelectionItem::GetColor()
{
    return m_bk_color;
}

wxString FilamentColorSelectionItem::get_filament_type_label()
{
	return m_filament_type_label;
}

std::string FilamentColorSelectionItem::get_filament_name()
{
	return m_filament_name; }

std::string FilamentColorSelectionItem::getUserMaterial() { return m_userMaterial; }

wxString FilamentColorSelectionItem::get_material_index_info()
{
	return m_material_index_info;
}

void FilamentColorSelectionItem::update_item_info_by_material(int box_id, const DM::Material& material_info, int box_type)
{
    m_box_id  = box_id;
	m_filament_name = material_info.name;
    m_userMaterial = material_info.userMaterial;
    m_bk_color = RemotePrint::Utils::hex_string_to_wxcolour(material_info.color);
	SetBackgroundColour(m_bk_color);
    char index_char    = 'A' + (material_info.material_id % 4); // Calculate the letter part (A, B, C, D)
    if(m_is_ext)
    {
        m_material_index_info = "EXT";
    }
    else 
    {
        m_material_index_info = wxString::Format("%d%c", m_box_id, index_char);
        if (box_type == 2) {
            m_material_index_info = "CFS";
        }
    }
    // 右侧类型：分三种情况
    if (m_sync) {
        // 已同步：正常显示真实类型
        m_filament_type_label = material_info.type;
    } else {
        // 未同步：根据 state 决定显示 "/" 还是 "?"
        // state == 0  → 槽位真的空 => 显示 "/"
        // state != 0 → 槽位有东西但不可用/未知 => 显示 "?"
        if (material_info.state == 0) {
            m_filament_type_label = "/";
        } else {
            m_filament_type_label = "?";
        }
    }

	SetLabel(m_material_index_info);

}

// draw one color rectangle and text "1A" or "1B" or "1C" or "1D" over this rectangle
void FilamentColorSelectionItem::OnPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);
    wxSize size = GetSize();

    // 绘制绿色边框
    dc.SetPen(wxPen(wxColour(0, 255, 0), 2));  // 绿色边框，宽度为2
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(0, 0, size.GetWidth(), size.GetHeight());

    // 左半部分
    wxRect leftRect(0, 0, size.GetWidth() / 2, size.GetHeight());
    dc.SetBrush(wxBrush(m_bk_color));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(leftRect);

    // 绘制左半部分的文字
    dc.SetTextForeground(GetTextColorBasedOnBackground(m_bk_color));
    dc.DrawText(m_material_index_info, leftRect.GetX() + 5, leftRect.GetY() + (leftRect.GetHeight() - dc.GetTextExtent(m_material_index_info).GetHeight()) / 2);

    // 右半部分
    wxRect rightRect(size.GetWidth() / 2, 0, size.GetWidth() / 2, size.GetHeight());
    dc.SetBrush(*wxWHITE_BRUSH);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(rightRect);

    // 绘制右半部分的文字
    dc.SetTextForeground(*wxBLACK);
    dc.DrawText(m_filament_type_label, rightRect.GetX() + 5, rightRect.GetY() + (rightRect.GetHeight() - dc.GetTextExtent(m_filament_type_label).GetHeight()) / 2);
}
#ifdef __WXMSW__
bool PopupWindowManager::IsMenuActive() const
{
    const HWND foreground = ::GetForegroundWindow();
    for (PopupWindow* popup : m_popups) {
        const HWND hwnd = reinterpret_cast<HWND>(popup->GetHWND());
        if (popup->IsShown() && (foreground == hwnd || ::IsChild(hwnd, foreground)))
            return true;
    }
    return false;
}

void ManagedPopupWindow::Dismiss()
{
    // wxMSW defers deactivation: switching between the two menus is harmless,
    // but switching to the main window or another application closes the chain.
    if (!IsShown() || PopupWindowManager::Get().IsMenuActive())
        return;
    PopupWindowManager::Get().CloseAll();
}
#endif
void ManagedPopupWindow::init()
{
#ifdef __WXMSW__
    ApplyWindowShadow(this);
#endif
}
void ManagedPopupWindow::OnPaint(wxPaintEvent& event)
{
    wxAutoBufferedPaintDC dc(this); // 双缓冲避免闪烁
    bool is_dark = Slic3r::GUI::wxGetApp().dark_mode();
    wxColour bgColor = is_dark ? "#313131" : "#FFFFFF";
    // 第一步：绘制50%透明阴影 (#768EAB)
    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    if (gc) {
        // 阴影参数
        //const int shadowSize = 10;     // 阴影扩散范围
        //const wxColour shadowColor(118, 142, 171, 128); // #768EAB 50%透明度
        wxSize size = GetSize();
#ifndef __WXMSW__
        // 1. 绘制阴影 (#768EAB 50%透明度)
        wxColour shadowColor(118, 142, 171, 128);  // RGBA格式[6,9](@ref)
        const int shadowBlur = 4;                  // 模糊半径8px
        const int shadowSpread = shadowBlur * 2;    // 阴影扩散范围
        // 非Windows平台手动绘制阴影

        gc->SetBrush(wxBrush(shadowColor));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRoundedRectangle(
            -shadowBlur, -shadowBlur,
            size.x + shadowSpread,
            size.y + shadowSpread,
            4 + shadowBlur * 0.5  // 阴影圆角稍大[9,12](@ref)
        );
#endif
        // 2. 绘制主窗口（白色背景+4px圆角）
        gc->SetBrush(wxBrush(bgColor));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRoundedRectangle(0, 0, size.GetWidth(), size.GetHeight(), 4);  // 4px圆角[12](@ref)

        delete gc;
    }
}

MaterialContextMenu::MaterialContextMenu(wxWindow* parent, int index) : m_index(index)
{
    auto* filament_panel = dynamic_cast<FilamentPanel*>(wxGetApp().plater()->sidebar().filament_panel());
    if (!filament_panel)
        return;
    const auto& items = filament_panel->get_filament_items();
    const bool valid_source = index >= 0 && index < static_cast<int>(items.size());

    auto add_action = [this](wxMenu* menu, const wxString& label, std::function<void()> action) {
        auto* item = menu->Append(wxID_ANY, label);
        m_actions.emplace(item->GetId(), std::move(action));
        return item;
    };
    add_action(this, _L("Edit"), [this]() { OnEdit(); })->Enable(valid_source);
    add_action(this, _L("Delete"), [this]() { OnDelete(); })
        ->Enable(valid_source && filament_panel->can_delete());

    auto* merge_menu = new wxMenu;
    auto add_target = [&](size_t slot, const wxString& label, const wxColour& color, std::function<void()> action) {
        // Preserve literal ampersands in preset names instead of menu mnemonics.
        wxString menu_label = wxString::Format("%02u  %s", static_cast<unsigned int>(slot + 1), label);
        menu_label.Replace("&", "&&");
        auto* item = new wxMenuItem(merge_menu, wxID_ANY, menu_label);
        m_actions.emplace(item->GetId(), std::move(action));
        const int size = parent->FromDIP(14);
        wxBitmap bitmap(size, size);
        wxMemoryDC dc(bitmap);
        dc.SetBackground(wxBrush(color.IsOk() ? color : wxColour("#CCCCCC")));
        dc.Clear();
        dc.SetPen(wxPen(wxColour("#808080")));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(0, 0, size, size);
        dc.SelectObject(wxNullBitmap);
        item->SetBitmap(bitmap);
        merge_menu->Append(item);
    };
    for (int i = 0; i < static_cast<int>(items.size()); ++i) {
        if (i == index)
            continue;
        add_target(i, items[i]->name(), items[i]->color(), [index, i]() {
            wxGetApp().plater()->sidebar().delete_filament(index, i);
        });
    }

    if (auto* preset_bundle = wxGetApp().preset_bundle) {
        const auto& mixed = preset_bundle->mixed_filaments.mixed_filaments();
        size_t virtual_ordinal = 0;
        for (size_t i = 0; i < mixed.size(); ++i) {
            const auto& mf = mixed[i];
            if (!mf.occupies_virtual_slot())
                continue;
            const size_t slot = items.size() + virtual_ordinal++;
            if (!mf.is_available(items.size()))
                continue;
            const wxString label = !mf.gradient_component_ids.empty()
                ? wxString::Format(_L("Mixed %u (F%u+...)"), (unsigned int)(i + 1), mf.component_a)
                : wxString::Format(_L("Mixed %u (F%u + F%u)"), (unsigned int)(i + 1), mf.component_a, mf.component_b);
            const uint64_t stable_id = mf.stable_id;
            add_target(slot, label, wxColour(mf.display_color.empty() ? "#CCCCCC" : mf.display_color),
                       [index, stable_id]() {
                wxGetApp().plater()->sidebar().merge_physical_to_mixed(static_cast<size_t>(index), stable_id);
            });
        }
    }
    if (merge_menu->GetMenuItemCount() != 0) {
        AppendSubMenu(merge_menu, _L("Merge with"))->Enable(valid_source && items.size() > 1);
    } else {
        delete merge_menu;
        Append(wxID_ANY, _L("Merge with"))->Enable(false);
    }
    add_action(this, _L("Decompose Color"), [this]() { OnDecomposeColor(); })
        ->Enable(valid_source && items.size() > 1);
}

void MaterialContextMenu::ExecuteSelection(int id)
{
    const auto action = m_actions.find(id);
    if (action != m_actions.end())
        action->second();
}

void MaterialContextMenu::OnEdit()
{
    auto* filament_panel = dynamic_cast<FilamentPanel*>(wxGetApp().mainframe->plater()->sidebar().filament_panel());
    if (!filament_panel)
        return;

    const auto& items = filament_panel->get_filament_items();
    if (m_index < 0 || m_index >= static_cast<int>(items.size()))
        return;

    FilamentItem* item = items[m_index];
    if (item)
        item->edit_preset();
}

void MaterialContextMenu::OnDelete()
{
    Slic3r::GUI::wxGetApp().plater()->sidebar().delete_filament(m_index);
}

void MaterialContextMenu::OnDecomposeColor()
{
    auto _filamentPanel = dynamic_cast<FilamentPanel*>(wxGetApp().mainframe->plater()->sidebar().filament_panel());
    if (!_filamentPanel)
        return;

    auto items = _filamentPanel->get_filament_items();
    if (items.empty() || m_index < 0 || static_cast<size_t>(m_index) >= items.size())
        return;

    // Resolve source target color and physical list from the live FilamentItem views.
    FilamentItem* source_item = items[m_index];
    if (!source_item)
        return;

    const wxColour target_color = source_item->color();
    if (!target_color.IsOk())
        return;

    auto* preset_bundle = wxGetApp().preset_bundle;
    if (!preset_bundle)
        return;

    // Build physical colors / names / types for the dialog. Use the source
    // FilamentItem views so what the dialog sees matches what the user sees.
    std::vector<std::string>       physical_colors;
    std::vector<std::string>       filament_names;
    std::vector<std::string>       filament_types;
    std::vector<size_t>            physical_config_indices;
    physical_colors.reserve(items.size());
    filament_names.reserve(items.size());
    filament_types.reserve(items.size());
    physical_config_indices.reserve(items.size());

    for (size_t i = 0; i < items.size(); ++i) {
        FilamentItem* it = items[i];
        if (!it)
            continue;
        const std::string hex = it->color().GetAsString(wxC2S_HTML_SYNTAX).ToStdString();
        physical_colors.push_back(decompose_normalize_color_hex(hex));
        filament_names.push_back(it->name().ToStdString().empty() ? std::string("Filament ") + std::to_string(i + 1) : it->name().ToStdString());

        std::string ftype = "PLA";
        if (i < preset_bundle->filament_presets.size()) {
            const std::string& preset_name = preset_bundle->filament_presets[i];
            Slic3r::Preset* preset = preset_bundle->filaments.find_preset(preset_name);
            if (preset) {
                std::string display_type;
                std::string raw = preset->config.get_filament_type(display_type);
                if (!raw.empty())
                    ftype = filament_type_for_color_decompose(preset);
                // Creality presets (e.g. "Hyper PLA @Creality F031 0.4 nozzle",
                // "Generic PLA @Creality CR-10 SE 0.4 nozzle") all carry
                // filament_type="PLA" but should be distinguished in the
                // color-decompose dropdown. When filament_type_for_color_decompose
                // falls back to a generic base type (PLA / PETG / ABS / ...),
                // extract the display type from the preset-name prefix instead.
                if (ftype == raw && !raw.empty() && !preset_name.empty()) {
                    const auto at = preset_name.find('@');
                    std::string prefix = (at == std::string::npos) ? preset_name
                                                                    : preset_name.substr(0, at);
                    while (!prefix.empty() && std::isspace(static_cast<unsigned char>(prefix.back())))
                        prefix.pop_back();
                    if (!prefix.empty())
                        ftype = prefix;
                }
            }
        }
        filament_types.push_back(ftype);
        physical_config_indices.push_back(i);
    }

    // Filament limit info for the in-dialog warning.
    const size_t current_filament_count = items.size();
    const size_t max_filament_count = static_cast<size_t>(_filamentPanel->get_filament_items().size() + 32);

    ColorDecomposeDialog dlg(wxGetApp().mainframe,
                             m_index,
                             target_color,
                             physical_colors,
                             filament_names,
                             filament_types,
                             current_filament_count,
                             max_filament_count,
                             physical_config_indices);

    if (dlg.ShowModal() != wxID_OK)
        return;

    ColorDecomposeResult result = dlg.get_result();
    // 注意：CMYW/RYBW 单色 (components.size() == 1) 也要往下走——
    // 可能需要新增该 base color 的物理耗材槽位（参照多色“缺耗材”流程）。
    // Step 2 的 final_components.size() < 2 检查会拦截 mixed filament 创建。

    // Convert the dialog result into a MixedFilamentResult, identifying which
    // official base-color components need a brand-new physical slot.
    MixedFilamentResult                          mixed_result;
    std::vector<DecomposeMissingComponent>       missing;
    if (!prepare_decompose_mixed_result(result,
                                        static_cast<size_t>(m_index),
                                        static_cast<size_t>(m_index),
                                        physical_colors,
                                        filament_types,
                                        physical_config_indices,
                                        mixed_result,
                                        missing))
        return;

    // Ask user to confirm creating any missing physical filaments.
    if (!confirm_create_decompose_missing_components(wxGetApp().mainframe, missing))
        return;

    // Capture the source physical identity so we can locate it after we add
    // new physicals (which shifts the index).
    const std::string source_preset_name = (static_cast<size_t>(m_index) < preset_bundle->filament_presets.size())
                                            ? preset_bundle->filament_presets[m_index]
                                            : std::string();
    const std::string source_color_hex   = decompose_normalize_color_hex(target_color.GetAsString(wxC2S_HTML_SYNTAX).ToStdString());

    // Step 1: create any missing base-color physicals. For MaterialList mode
    // this list is always empty, so the loop is a no-op.
    if (!missing.empty()) {
        std::vector<wxColour> new_colors;
        new_colors.reserve(missing.size());
        for (const auto& m : missing) {
            wxColour col(m.official_component.color_hex);
            if (!col.IsOk()) {
                // Fallback: derive a display colour from the base color name.
                switch (m.official_component.base_color) {
                case DecomposeBaseColor::Cyan:    col = wxColour(0, 255, 255); break;
                case DecomposeBaseColor::Magenta: col = wxColour(255, 0, 255); break;
                case DecomposeBaseColor::Yellow:  col = wxColour(255, 255, 0); break;
                case DecomposeBaseColor::White:   col = wxColour(255, 255, 255); break;
                case DecomposeBaseColor::Red:     col = wxColour(255, 0, 0); break;
                case DecomposeBaseColor::Green:   col = wxColour(0, 255, 0); break;
                case DecomposeBaseColor::Blue:    col = wxColour(0, 0, 255); break;
                default:                          col = wxColour(204, 204, 204); break;
                }
            }
            new_colors.push_back(col);
        }
        const int target_count = static_cast<int>(preset_bundle->filament_presets.size() + new_colors.size());
        wxGetApp().plater()->sidebar().add_filaments_batch(target_count, new_colors);

        // Step 1b: override the new physical slots to use a Hyper PLA preset.
        // add_filaments_batch() -> set_num_filaments() copies the last existing
        // preset onto every new slot, so if the project ended in a "Generic PLA"
        // (or any non-Hyper) preset, the new CMYW/RYBW base colors would be
        // marked as that material. Per user requirement, the new base-color
        // physicals created by CMYW/RYBW decomposition must be Hyper PLA.
        //
        // Find an available Hyper PLA preset name (e.g. "Hyper PLA @Creality
        // K2 Plus 0.4 nozzle" or the plain "Hyper PLA" baseline) and rewrite
        // the per-slot preset name for every newly appended slot.
        std::string hyper_pla_preset;
        for (const auto& p : preset_bundle->filaments.get_presets()) {
            // System/user preset names follow the "<Display Type> @<Printer>"
            // convention. We want any preset whose name starts with "Hyper PLA".
            if (p.name.rfind("Hyper PLA", 0) == 0) {
                hyper_pla_preset = p.name;
                break;
            }
        }
        if (!hyper_pla_preset.empty()) {
            const size_t new_physicals_start = preset_bundle->filament_presets.size() - new_colors.size();
            for (size_t i = new_physicals_start; i < preset_bundle->filament_presets.size(); ++i) {
                preset_bundle->filament_presets[i] = hyper_pla_preset;
            }
            // Re-apply the changed slot presets so the filament tab + extruder
            // counts reflect the new Hyper PLA assignment. Using
            // on_filaments_change with the current physical count is enough
            // because the per-slot preset name is read from filament_presets
            // every refresh.
            wxGetApp().plater()->on_filaments_change(preset_bundle->filament_presets.size());

            // Step 1c: refresh each newly created FilamentItem so the
            // COLLAPSED view label picks up the new Hyper PLA preset. The
            // FilamentItem constructor caches the preset's filament_type
            // config (e.g. "PLA") into m_btn_param_list as the collapsed
            // label. After we changed filament_presets[i] to Hyper PLA the
            // underlying data is correct, but the cached button label still
            // shows the previous preset's type (e.g. "Generic PLA"). Calling
            // FilamentItem::update() rebuilds the combobox from the new
            // filament_presets[i] entry and rewrites m_btn_param_list from
            // the combobox's current selection, so the collapsed label now
            // matches the expanded dropdown (both show "Hyper PLA").
            auto panel_items = _filamentPanel->get_filament_items();
            for (size_t i = new_physicals_start; i < preset_bundle->filament_presets.size(); ++i) {
                if (i < panel_items.size() && panel_items[i] != nullptr) {
                    panel_items[i]->update();
                }
            }
        } else {
            BOOST_LOG_TRIVIAL(warning) << "[DecomposeColor] No Hyper PLA preset found in the project; "
                                      << "new CMYW/RYBW base-color physicals will keep the inherited preset.";
        }
    }

    // Step 2: compute final component_a / component_b / mix_b_percent.
    // For MaterialList mode the components are 1-based indices into the
    // original physical list. For CMYW/RYBW, missing components become the
    // newly-appended physicals (last N slots), while existing ones keep their
    // original index because the new slots were added at the tail.
    auto& mgr = preset_bundle->mixed_filaments;
    const size_t num_physical_before = items.size();
    const size_t num_physical_now    = preset_bundle->filament_presets.size();
    const size_t num_new_physicals   = num_physical_now - num_physical_before;

    // For each component slot: if it was a "missing" base color, point it at
    // the matching newly appended physical. Otherwise keep the existing
    // 1-based index (which still points at the same physical since new
    // physicals were appended at the end).
    std::vector<unsigned int> final_components;
    std::vector<int>          final_ratios;
    final_components.reserve(mixed_result.components.size());
    final_ratios.reserve(mixed_result.ratios.size());

    size_t missing_pos = 0;
    for (size_t i = 0; i < mixed_result.components.size(); ++i) {
        const unsigned int comp_idx = mixed_result.components[i];
        if (comp_idx == 0) {
            // Missing physical -> newly appended at the end.
            unsigned int new_idx = static_cast<unsigned int>(num_physical_now - num_new_physicals + missing_pos + 1);
            final_components.push_back(new_idx);
            ++missing_pos;
        } else {
            final_components.push_back(comp_idx);
        }
        final_ratios.push_back(mixed_result.ratios[i]);
    }

    if (final_components.size() < 2 || final_ratios.size() != final_components.size())
        return;

    // The MixedFilament API only natively supports 2-color mixes. For 3-color
    // decompositions we still register the 2 most-weighted components and let
    // the third be encoded via gradient_component_ids. For the most common
    // 2-color case this is the natural path.
    unsigned int comp_a = final_components[0];
    unsigned int comp_b = final_components[1];
    int          mix_b  = final_ratios[1];
    if (final_components.size() >= 3) {
        // Use first + the heaviest of the remaining as the two-color base, and
        // append the rest via gradient_component_ids.
        comp_a = final_components[0];
        // Pick the largest non-zero ratio as comp_b.
        size_t b_idx = 1;
        int    b_w   = final_ratios[1];
        for (size_t k = 2; k < final_ratios.size(); ++k) {
            if (final_ratios[k] > b_w) {
                b_w   = final_ratios[k];
                b_idx = k;
            }
        }
        comp_b = final_components[b_idx];
        mix_b  = b_w;
    }

    // Build a current physical-colour list from the preset bundle for the
    // add_custom_filament call (it needs the per-slot colours).
    std::vector<std::string> current_physical_colours;
    current_physical_colours.reserve(num_physical_now);
    Slic3r::ConfigOptionStrings* colour_opt = preset_bundle->project_config.option<Slic3r::ConfigOptionStrings>("filament_colour");
    for (size_t i = 0; i < num_physical_now; ++i) {
        if (colour_opt && i < colour_opt->values.size() && !colour_opt->values[i].empty())
            current_physical_colours.push_back(colour_opt->values[i]);
        else
            current_physical_colours.push_back("#CCCCCC");
    }

    // Record the current mixed-filament count so we can locate the newly added row.
    const size_t mixed_count_before = mgr.mixed_filaments().size();

    mgr.add_custom_filament(comp_a, comp_b, mix_b, current_physical_colours);

    if (mgr.mixed_filaments().size() <= mixed_count_before)
        return;

    // Find the just-added mixed filament by its position at the end of the list.
    uint64_t new_stable_id = 0;
    for (size_t i = mixed_count_before; i < mgr.mixed_filaments().size(); ++i) {
        if (mgr.mixed_filaments()[i].stable_id != 0) {
            new_stable_id = mgr.mixed_filaments()[i].stable_id;
            break;
        }
    }
    if (new_stable_id == 0) {
        // Fall back: pick the last enabled, non-deleted mixed row.
        for (auto it = mgr.mixed_filaments().rbegin(); it != mgr.mixed_filaments().rend(); ++it) {
            if (!it->deleted && it->enabled) {
                new_stable_id = it->stable_id;
                break;
            }
        }
    }
    if (new_stable_id == 0)
        return;

    // If the result had more than 2 components, attach the additional ones via
    // the gradient_component_ids / gradient_component_weights so the slicer
    // can still emit a 3+ colour mixed tool. This is only meaningful for 3+.
    if (final_components.size() >= 3) {
        for (auto& mf : mgr.mixed_filaments()) {
            if (mf.stable_id != new_stable_id)
                continue;
            // Use '|'-separated format to support multi-digit filament IDs (>9).
            // ',' is reserved as the field separator in serialized rows, and ';'
            // is the row separator, so neither can appear inside the ids payload.
            // Old compact format (single-char '1'-'9', no separator) is still
            // supported by decoders for backward compatibility.
            std::string ids;
            std::string weights;
            for (size_t i = 0; i < final_components.size(); ++i) {
                if (i > 0) {
                    ids.push_back('|');
                    weights.push_back('/');
                }
                ids.append(std::to_string(final_components[i]));
                weights.append(std::to_string(final_ratios[i]));
            }
            mf.gradient_component_ids      = ids;
            mf.gradient_component_weights  = weights;
            // 3-color decomposition uses gradient_component_ids/weights, not gradient_enabled.
            // gradient_enabled is for 2-color gradient mode (curve editor), which is irrelevant here.
            mf.gradient_enabled            = false;
            // distribution_mode must be LayerCycle (not default Simple) for the slicer
            // to parse gradient_component_ids/weights. See ToolOrdering.cpp L55.
            mf.distribution_mode           = int(Slic3r::MixedFilament::LayerCycle);
            break;
        }
    }

    // Step 3: source physical filament is intentionally kept in the project.
    // Previously merge_physical_to_mixed() would absorb the source into the
    // new mixed filament (deleting the source slot and remapping every object
    // reference). Per user request the source physical filament must be
    // preserved after decomposition so the user can compare, switch back, or
    // remove it manually. The new mixed filament is added to the list, but
    // the source physical is NOT deleted here.
    //
    // Note: the source_physical_idx calculation is retained for any future
    // "redirect only" path that keeps the source while still remapping model
    // references, but currently we simply skip the destructive merge.
    size_t source_physical_idx = static_cast<size_t>(m_index);
    if (num_new_physicals > 0) {
        // Source physical slot shifted right by num_new_physicals if it
        // appeared before the inserted range. The new physicals were appended
        // at the tail, so anything with index >= num_physical_before is
        // unchanged. For MaterialList mode this branch is never taken.
        if (static_cast<size_t>(m_index) < num_physical_before) {
            // Try to identify the original slot by preset name. This is more
            // robust than relying on the index because add_filaments_batch may
            // also have renamed slots in edge cases.
            for (size_t i = 0; i < preset_bundle->filament_presets.size(); ++i) {
                if (preset_bundle->filament_presets[i] == source_preset_name) {
                    // Also verify the color matches to be safe.
                    std::string cur_hex;
                    if (colour_opt && i < colour_opt->values.size())
                        cur_hex = decompose_normalize_color_hex(colour_opt->values[i]);
                    if (cur_hex == source_color_hex) {
                        source_physical_idx = i;
                        break;
                    }
                }
            }
        } else {
            source_physical_idx = m_index + num_new_physicals;
        }
    }
    (void)source_physical_idx; // currently unused; kept for future redirect-only path

    // The destructive merge that previously absorbed the source physical into
    // the mixed filament is intentionally NOT performed. The source physical
    // filament remains in the project so the user can decide what to do with
    // it after the decomposition completes.
    // (Previously: wxGetApp().plater()->sidebar().merge_physical_to_mixed(source_physical_idx, new_stable_id);)

    // Decompose Color is now non-destructive: it only adds a new mixed
    // filament entry. The source physical filament is preserved, no object
    // references are remapped, and no slot is removed. We use Action snapshot
    // type (not ProjectSeparator) to preserve undo/redo history.
    if (auto* plater = wxGetApp().plater()) {
        plater->take_snapshot(std::string("Decompose Color"),
                              Slic3r::UndoRedo::SnapshotType::Action);
    }

    // Sync the newly added mixed filament to config BEFORE refreshing the panel.
    // Without this, update_mixed_filament_panel(true) would clear the custom entry
    // we just added (via mgr.clear_custom_entries()) and reload from the empty
    // mixed_filament_definitions string, losing the new entry.
    if (preset_bundle) {
        // serialize_custom_entries() produces the JSON string that gets stored
        // in mixed_filament_definitions; sync_mixed_filament_definitions_to_configs
        // is a static function in Plater.cpp, so we replicate the essential logic
        // here by writing to project_config directly.
        const std::string serialized = mgr.serialize_custom_entries();
        if (Slic3r::ConfigOptionString *opt = preset_bundle->project_config.option<Slic3r::ConfigOptionString>("mixed_filament_definitions"))
            opt->value = serialized;
        else
            preset_bundle->project_config.set_key_value("mixed_filament_definitions", new Slic3r::ConfigOptionString(serialized));
    }

    // Refresh the mixed filament panel so the newly created mixed filament
    // becomes visible immediately. Without this, the panel stays collapsed
    // (or stale) until the next UI refresh (e.g. deleting a filament), which
    // causes the confusing symptom where the mixed filament "appears" only
    // after a delete operation.
    if (auto* plater = wxGetApp().plater()) {
        plater->sidebar().update_mixed_filament_panel(true);
    }
}
