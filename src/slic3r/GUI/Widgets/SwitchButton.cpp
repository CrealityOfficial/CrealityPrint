#include "SwitchButton.hpp"
#include "Label.hpp"
#include "StaticBox.hpp"

#include "../wxExtensions.hpp"
#include "../Utils/MacDarkMode.hpp"
#include "../Utils/WxFontUtils.hpp"
#ifdef __APPLE__
#include "libslic3r/MacUtils.hpp"
#endif

#ifdef __WXGTK3__
#include "../GUI_Utils.hpp"
#endif

#include <wx/dcclient.h>
#include <wx/dcgraph.h>
#include <wx/dcmemory.h>
#include <wx/dcbuffer.h>

wxDEFINE_EVENT(wxCUSTOMEVT_SWITCH_POS, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_MULTISWITCH_SELECTION, wxCommandEvent);

SwitchButton::SwitchButton(wxWindow* parent, wxWindowID id)
	: wxBitmapToggleButton(parent, id, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxBU_EXACTFIT)
	, m_on(this, "toggle_on", 16)
	, m_off(this, "toggle_off", 16)
    , text_color(std::pair{0xfffffe, (int) StateColor::Checked}, std::pair{0x6B6B6B, (int) StateColor::Normal})
	, track_color(0xD9D9D9)
    , thumb_color(std::pair{0x00AE42, (int) StateColor::Checked}, std::pair{0xD9D9D9, (int) StateColor::Normal})
{
	SetBackgroundColour(StaticBox::GetParentBackgroundColor(parent));
	Bind(wxEVT_TOGGLEBUTTON, [this](auto& e) { update(); e.Skip(); });
	SetFont(Label::Body_12);

#ifdef __WXGTK3__
    Slic3r::GUI::RemoveButtonBorder(this);
#endif

	Rescale();
}

void SwitchButton::SetLabels(wxString const& lbl_on, wxString const& lbl_off)
{
	labels[0] = lbl_on;
	labels[1] = lbl_off;
	Rescale();
}

void SwitchButton::SetTextColor(StateColor const& color)
{
	text_color = color;
}

void SwitchButton::SetTextColor2(StateColor const &color)
{
	text_color2 = color;
}

void SwitchButton::SetTrackColor(StateColor const& color)
{
	track_color = color;
}

void SwitchButton::SetThumbColor(StateColor const& color)
{
	thumb_color = color;
}

void SwitchButton::SetValue(bool value)
{
	if (value != GetValue())
		wxBitmapToggleButton::SetValue(value);
	update();
}

void SwitchButton::Rescale()
{
	if (labels[0].IsEmpty()) {
		m_on.msw_rescale();
		m_off.msw_rescale();
	}
	else {
        SetBackgroundColour(StaticBox::GetParentBackgroundColor(GetParent()));
#ifdef __WXOSX__
        auto scale = Slic3r::GUI::mac_max_scaling_factor();
        int BS = (int) scale;
#else
        constexpr int BS = 1;
#endif
		wxSize thumbSize;
		wxSize trackSize;
		wxClientDC dc(this);
        dc.SetFont(GetFont());
#ifdef __WXOSX__
        dc.SetFont(dc.GetFont().Scaled(scale));
#endif
        wxSize textSize[2];
		{
			textSize[0] = dc.GetTextExtent(labels[0]);
			textSize[1] = dc.GetTextExtent(labels[1]);
		}
		float fontScale = 0;
		{
			thumbSize = textSize[0];
			auto size = textSize[1];
			if (size.x > thumbSize.x) thumbSize.x = size.x;
			else size.x = thumbSize.x;
			thumbSize.x += BS * 12;
			thumbSize.y += BS * 6;
			trackSize.x = thumbSize.x + size.x + BS * 10;
			trackSize.y = thumbSize.y + BS * 2;
            auto maxWidth = GetMaxWidth();
#ifdef __WXOSX__
            maxWidth *= scale;
#endif
			if (trackSize.x > maxWidth) {
                fontScale   = float(maxWidth) / trackSize.x;
                thumbSize.x -= (trackSize.x - maxWidth) / 2;
                trackSize.x = maxWidth;
			}
		}
		for (int i = 0; i < 2; ++i) {
			wxMemoryDC memdc(&dc);
#ifdef __WXMSW__
			wxBitmap bmp(trackSize.x, trackSize.y);
			memdc.SelectObject(bmp);
			memdc.SetBackground(wxBrush(GetBackgroundColour()));
			memdc.Clear();
#else
            wxImage image(trackSize);
            image.InitAlpha();
            memset(image.GetAlpha(), 0, trackSize.GetWidth() * trackSize.GetHeight());
            wxBitmap bmp(std::move(image));
            memdc.SelectObject(bmp);
#endif
            memdc.SetFont(dc.GetFont());
#ifdef __WXMSW__
            const double scale = GetDPIScaleFactor();
            fontScale = scale;
#endif
            if (fontScale) {
                memdc.SetFont(dc.GetFont().Scaled(fontScale));
                textSize[0] = memdc.GetTextExtent(labels[0]);
                textSize[1] = memdc.GetTextExtent(labels[1]);
			}
			auto state = i == 0 ? StateColor::Enabled : (StateColor::Checked | StateColor::Enabled);
            {
#ifdef __WXMSW__
				wxGCDC dc2(memdc);
#else
                wxDC &dc2(memdc);
#endif
				dc2.SetBrush(wxBrush(track_color.colorForStates(state)));
				dc2.SetPen(wxPen(track_color.colorForStates(state)));
                dc2.DrawRoundedRectangle(wxRect({0, 0}, trackSize), trackSize.y / 2);
				dc2.SetBrush(wxBrush(thumb_color.colorForStates(StateColor::Checked | StateColor::Enabled)));
				dc2.SetPen(wxPen(thumb_color.colorForStates(StateColor::Checked | StateColor::Enabled)));
				dc2.DrawRoundedRectangle(wxRect({ i == 0 ? BS : (trackSize.x - thumbSize.x - BS), BS}, thumbSize), thumbSize.y / 2);
			}
            memdc.SetTextForeground(text_color.colorForStates(state ^ StateColor::Checked));
            auto text_y = BS + (thumbSize.y - textSize[0].y) / 2;
#ifdef __APPLE__
            if (Slic3r::is_mac_version_15()) {
                text_y -= FromDIP(2);
            }
#endif
            memdc.DrawText(labels[0], {BS + (thumbSize.x - textSize[0].x) / 2, text_y});
            memdc.SetTextForeground(text_color2.count() == 0 ? text_color.colorForStates(state) : text_color2.colorForStates(state));
            auto text_y_1 = BS + (thumbSize.y - textSize[1].y) / 2;
#ifdef __APPLE__
            if (Slic3r::is_mac_version_15()) {
                text_y_1 -= FromDIP(2);
            }
#endif
            memdc.DrawText(labels[1], {trackSize.x - thumbSize.x - BS + (thumbSize.x - textSize[1].x) / 2, text_y_1});
			memdc.SelectObject(wxNullBitmap);
#ifdef __WXOSX__
            bmp = wxBitmap(bmp.ConvertToImage(), -1, scale);
#elif defined(__WXMSW__)
            bmp.SetScaleFactor(scale);
#endif
			(i == 0 ? m_off : m_on).bmp() = bmp;
		}
	}
	SetSize(m_on.GetBmpSize());
	update();
}

void SwitchButton::update()
{
	SetBitmap((GetValue() ? m_on : m_off).bmp());
}

SwitchBoard::SwitchBoard(wxWindow *parent, wxString leftL, wxString right, wxSize size)
 : wxWindow(parent, wxID_ANY, wxDefaultPosition, size)
{
#ifdef __WINDOWS__
    SetDoubleBuffered(true);
#endif //__WINDOWS__

    SetBackgroundColour(*wxWHITE);
	leftLabel = leftL;
    rightLabel = right;

	SetMinSize(size);
	SetMaxSize(size);

    Bind(wxEVT_PAINT, &SwitchBoard::paintEvent, this);
    Bind(wxEVT_LEFT_DOWN, &SwitchBoard::on_left_down, this);

    Bind(wxEVT_ENTER_WINDOW, [this](auto &e) { SetCursor(wxCURSOR_HAND); });
    Bind(wxEVT_LEAVE_WINDOW, [this](auto &e) { SetCursor(wxCURSOR_ARROW); });
}

void SwitchBoard::updateState(wxString target)
{
    if (target.empty()) {
        switch_left = false;
        switch_right = false;
    } else {
        if (target == "left") {
            switch_left = true;
            switch_right = false;
        } else if (target == "right") {
            switch_left  = false;
            switch_right = true;
        }
    }
    Refresh();
}

void SwitchBoard::paintEvent(wxPaintEvent &evt)
{
    wxPaintDC dc(this);
    render(dc);
}

void SwitchBoard::render(wxDC &dc)
{
#ifdef __WXMSW__
    wxSize     size = GetSize();
    wxMemoryDC memdc;
    wxBitmap   bmp(size.x, size.y);
    memdc.SelectObject(bmp);
    memdc.Blit({0, 0}, size, &dc, {0, 0});

    {
        wxGCDC dc2(memdc);
        doRender(dc2);
    }

    memdc.SelectObject(wxNullBitmap);
    dc.DrawBitmap(bmp, 0, 0);
#else
    doRender(dc);
#endif
}

void SwitchBoard::doRender(wxDC &dc)
{
    wxColour disable_color = wxColour(0xCECECE);

    dc.SetPen(*wxTRANSPARENT_PEN);

    if (is_enable) {dc.SetBrush(wxBrush(0xeeeeee));
    } else {dc.SetBrush(disable_color);}
    dc.DrawRoundedRectangle(0, 0, GetSize().x, GetSize().y, 8);

	/*left*/
    if (switch_left) {
        is_enable ? dc.SetBrush(wxBrush(wxColour(0, 174, 66))) : dc.SetBrush(disable_color);
        dc.DrawRoundedRectangle(0, 0, GetSize().x / 2, GetSize().y, 8);
	}

    if (switch_left) {
		dc.SetTextForeground(*wxWHITE);
    } else {
        dc.SetTextForeground(0x333333);
	}

    dc.SetFont(::Label::Body_13);
    Slic3r::GUI::WxFontUtils::get_suitable_font_size(0.6 * GetSize().GetHeight(), dc);

    auto left_txt_size = dc.GetTextExtent(leftLabel);
    dc.DrawText(leftLabel, wxPoint((GetSize().x / 2 - left_txt_size.x) / 2, (GetSize().y - left_txt_size.y) / 2));

	/*right*/
    if (switch_right) {
        if (is_enable) {dc.SetBrush(wxBrush(wxColour(0, 174, 66)));
        } else {dc.SetBrush(disable_color);}
        dc.DrawRoundedRectangle(GetSize().x / 2, 0, GetSize().x / 2, GetSize().y, 8);
	}

    auto right_txt_size = dc.GetTextExtent(rightLabel);
    if (switch_right) {
        dc.SetTextForeground(*wxWHITE);
    } else {
        dc.SetTextForeground(0x333333);
    }
    dc.DrawText(rightLabel, wxPoint((GetSize().x / 2 - right_txt_size.x) / 2 + GetSize().x / 2, (GetSize().y - right_txt_size.y) / 2));

}

void SwitchBoard::on_left_down(wxMouseEvent &evt)
{
    if (!is_enable) {
        return;
    }
    int index = -1;
    auto pos = ClientToScreen(evt.GetPosition());
    auto rect = ClientToScreen(wxPoint(0, 0));

    if (pos.x > 0 && pos.x < rect.x + GetSize().x / 2) {
        switch_left = true;
        switch_right = false;
        index = 1;
    } else {
        switch_left  = false;
        switch_right = true;
        index = 0;
    }

    if (auto_disable_when_switch)
    {
        is_enable = false;// make it disable while switching
    }
    Refresh();

    wxCommandEvent event(wxCUSTOMEVT_SWITCH_POS);
    event.SetInt(index);
    wxPostEvent(this, event);
}

void SwitchBoard::Enable()
{
    if (is_enable == true)
    {
        return;
    }

    is_enable = true;
    Refresh();
}

void SwitchBoard::Disable()
{
    if (is_enable == false)
    {
        return;
    }

    is_enable = false;
    Refresh();
}

MultiSwitchBoard::MultiSwitchBoard(wxWindow* parent, Style style, wxWindowID id)
    : wxWindow(parent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
    , m_style(style)
{
    SetBackgroundColour(StaticBox::GetParentBackgroundColor(parent));
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetFont(Label::Body_13);

    Bind(wxEVT_PAINT, &MultiSwitchBoard::paintEvent, this);
    Bind(wxEVT_LEFT_DOWN, &MultiSwitchBoard::on_left_down, this);
    Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent&) { SetCursor(wxCURSOR_HAND); });
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { SetCursor(wxCURSOR_ARROW); });

    update_min_size();
}

void MultiSwitchBoard::SetOptions(const std::vector<wxString>& options)
{
    m_options = options;
    if (m_options.empty())
        m_selection = -1;
    else if (m_selection < 0 || m_selection >= static_cast<int>(m_options.size()))
        m_selection = 0;

    update_min_size();
    Refresh();
}

void MultiSwitchBoard::SetSelection(int selection)
{
    if (selection < 0 || selection >= static_cast<int>(m_options.size()) || selection == m_selection)
        return;

    m_selection = selection;
    Refresh();
}

void MultiSwitchBoard::Rescale()
{
    SetFont(Label::Body_13);
    update_min_size();
    Refresh();
}

void MultiSwitchBoard::sys_color_changed()
{
    SetBackgroundColour(StaticBox::GetParentBackgroundColor(GetParent()));
    Refresh(false);
}

bool MultiSwitchBoard::Enable(bool enable)
{
    const bool changed = wxWindow::Enable(enable);
    Refresh();
    return changed;
}

void MultiSwitchBoard::update_min_size()
{
    const int height = FromDIP(m_style == Style::Underline ? 28 : 26);
    int width = FromDIP(m_style == Style::Underline ? 320 : 310);
    if (!m_options.empty()) {
        int content_width = 0;
        const int horizontal_padding = FromDIP(m_style == Style::Underline ? 32 : 24);
        for (const wxString& option : m_options)
            content_width += GetTextExtent(option).x + horizontal_padding;
        width = std::max(width, std::min(content_width, FromDIP(520)));
    }
    SetMinSize(wxSize(width, height));
}

void MultiSwitchBoard::paintEvent(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(GetBackgroundColour()));
    dc.Clear();

    if (m_options.empty())
        return;

    wxGCDC gdc(dc);
    const wxSize size = GetClientSize();
    const int count = static_cast<int>(m_options.size());

    if (m_style == Style::Underline) {
        const wxColour background = GetBackgroundColour();
        const int brightness = (background.Red() * 299 + background.Green() * 587 + background.Blue() * 114) / 1000;
        const wxColour selected = IsEnabled() ? wxColour(0x1F, 0xCA, 0x63) : wxColour(0xA8, 0xD8, 0xB9);
        const wxColour normal = IsEnabled()
            ? (brightness < 128 ? wxColour(0xD7, 0xD9, 0xDC) : wxColour(0x4E, 0x59, 0x69))
            : wxColour(0x9A, 0xA0, 0xA8);
        const int underline_y = size.y - FromDIP(2);

        gdc.SetFont(GetFont());
        for (int i = 0; i < count; ++i) {
            const int left = size.x * i / count;
            const int right = size.x * (i + 1) / count;
            const int available = std::max(1, right - left - FromDIP(10));
            wxString label = m_options[i];
            while (label.length() > 1 && gdc.GetTextExtent(label).x > available)
                label = label.Left(label.length() - 2) + wxString::FromUTF8("\xE2\x80\xA6");

            const bool is_selected = i == m_selection;
            gdc.SetTextForeground(is_selected ? selected : normal);
            const wxSize text_size = gdc.GetTextExtent(label);
            const int text_x = left + (right - left - text_size.x) / 2;
            const int text_y = std::max(0, (underline_y - text_size.y) / 2);
            gdc.DrawText(label, text_x, text_y);
            if (is_selected) {
                gdc.SetPen(wxPen(selected, FromDIP(2)));
                gdc.DrawLine(text_x, underline_y, text_x + text_size.x, underline_y);
            }
        }
        return;
    }

    const int radius = FromDIP(8);
    const wxColour track = IsEnabled() ? wxColour(0x5B, 0x5B, 0x60) : wxColour(0xCE, 0xCE, 0xCE);
    const wxColour selected = IsEnabled() ? wxColour(0x00, 0xAE, 0x42) : wxColour(0xA8, 0xD8, 0xB9);

    gdc.SetPen(*wxTRANSPARENT_PEN);
    gdc.SetBrush(wxBrush(track));
    gdc.DrawRoundedRectangle(0, 0, size.x, size.y, radius);

    if (m_selection >= 0 && m_selection < count) {
        const int left = size.x * m_selection / count;
        const int right = size.x * (m_selection + 1) / count;
        gdc.SetBrush(wxBrush(selected));
        gdc.DrawRoundedRectangle(left, 0, right - left, size.y, radius);
    }

    gdc.SetFont(GetFont());
    for (int i = 0; i < count; ++i) {
        const int left = size.x * i / count;
        const int right = size.x * (i + 1) / count;
        const int available = std::max(1, right - left - FromDIP(10));
        wxString label = m_options[i];
        while (label.length() > 1 && gdc.GetTextExtent(label).x > available)
            label = label.Left(label.length() - 2) + wxString::FromUTF8("\xE2\x80\xA6");

        gdc.SetTextForeground(i == m_selection ? *wxWHITE : wxColour(0xB8, 0xB8, 0xBC));
        const wxSize text_size = gdc.GetTextExtent(label);
        gdc.DrawText(label, left + (right - left - text_size.x) / 2, (size.y - text_size.y) / 2);
    }
}

void MultiSwitchBoard::on_left_down(wxMouseEvent& event)
{
    if (!IsEnabled() || m_options.empty())
        return;

    const int width = std::max(1, GetClientSize().x);
    const int selection = std::min(static_cast<int>(m_options.size()) - 1,
                                   event.GetPosition().x * static_cast<int>(m_options.size()) / width);
    if (selection == m_selection)
        return;

    m_selection = selection;
    Refresh();

    wxCommandEvent evt(wxCUSTOMEVT_MULTISWITCH_SELECTION, GetId());
    evt.SetEventObject(this);
    evt.SetInt(selection);
    evt.SetString(m_options[selection]);
    GetEventHandler()->ProcessEvent(evt);
}