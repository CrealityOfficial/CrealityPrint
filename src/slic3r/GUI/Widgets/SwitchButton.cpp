#include "SwitchButton.hpp"
#include "Label.hpp"
#include "StaticBox.hpp"

#include "../wxExtensions.hpp"
#include "../Utils/MacDarkMode.hpp"

#include <string>
#include <wx/dcmemory.h>
#include <wx/dcclient.h>
#include <wx/dcgraph.h>

SwitchButton::SwitchButton(wxWindow* parent, wxWindowID id, std::string on, std::string off, bool set_bitmap_margins/* = false*/, const wxSize& bmp_margin_size/* = wxSize(0, 0)*/)
	: wxBitmapToggleButton(parent, id, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxBU_EXACTFIT)
    , m_on(this, on, 14)
    , m_off(this, off, 14)
    , text_color(std::pair{0xfffffe, (int) StateColor::Checked}, std::pair{0x000000, (int) StateColor::Normal})
    , track_color(0xD9D9D9)
    , thumb_color(std::pair{0x15BF59, (int) StateColor::Checked}, std::pair{0xD9D9D9, (int) StateColor::Normal})
    , m_set_bitmap_margins(set_bitmap_margins)
    , m_bmp_margin_size(bmp_margin_size)
{
	SetBackgroundColour(StaticBox::GetParentBackgroundColor(parent));
	Bind(wxEVT_TOGGLEBUTTON, [this](auto& e) { update(); e.Skip(); });
	SetFont(Label::Body_12);
	Rescale();

    if (m_set_bitmap_margins)
        SetBitmapMargins(m_bmp_margin_size);
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
            if (fontScale) {
                memdc.SetFont(dc.GetFont().Scaled(fontScale));
                textSize[0] = memdc.GetTextExtent(labels[0]);
                textSize[1] = memdc.GetTextExtent(labels[1]);
			}
			auto state = i == 0 ? StateColor::Enabled : (StateColor::Checked | StateColor::Enabled);
            {
				wxGraphicsContext* gc   = wxGraphicsContext::Create(memdc);
                wxGraphicsPath     path = gc->CreatePath();

                gc->SetBrush(wxBrush(thumb_color.colorForStates(StateColor::Checked | StateColor::Enabled)));
                double radius = 4;
                if (i == 1) {
                    wxRect rect(trackSize.x / 2, 1, trackSize.x / 2-2, trackSize.y-2);
                    path.MoveToPoint(rect.GetX(), rect.GetY());
                    path.AddLineToPoint(rect.GetRight() - radius, rect.GetY());
                    path.AddArcToPoint(rect.GetRight(), rect.GetY(), rect.GetRight(), rect.GetY() + radius, radius);
                    path.AddLineToPoint(rect.GetRight(), rect.GetBottom() - radius);
                    path.AddArcToPoint(rect.GetRight(), rect.GetBottom(), rect.GetRight() - radius, rect.GetBottom(), radius);
                    path.AddLineToPoint(rect.GetX(), rect.GetBottom());
                    path.AddLineToPoint(rect.GetX(), rect.GetY());
                    gc->FillPath(path);
                } else {
                    wxRect rect(1, 1, trackSize.x-2, trackSize.y-2);
                    path.MoveToPoint(rect.GetX() + radius, rect.GetY());
                    path.AddLineToPoint(rect.GetRight() / 2, rect.GetY());
                    path.AddLineToPoint(rect.GetRight() / 2, rect.GetBottom());
                    path.AddLineToPoint(rect.GetX() + radius, rect.GetBottom());
                    path.AddArcToPoint(rect.GetX(), rect.GetBottom(), rect.GetX(), rect.GetBottom() - radius, radius);
                    path.AddLineToPoint(rect.GetX(), rect.GetY() + radius);
                    path.AddArcToPoint(rect.GetX(), rect.GetY(), rect.GetX() + radius, rect.GetY(), radius);
                    gc->FillPath(path);
                }

                delete gc;
			}
            memdc.SetTextForeground(text_color.colorForStates(state ^ StateColor::Checked));
            memdc.DrawText(labels[0], {BS + (thumbSize.x - textSize[0].x) / 2, BS + (thumbSize.y - textSize[0].y) / 2});
            memdc.SetTextForeground(text_color2.count() == 0 ? text_color.colorForStates(state) : text_color2.colorForStates(state));
            memdc.DrawText(labels[1], {trackSize.x - thumbSize.x - BS + (thumbSize.x - textSize[1].x) / 2, BS + (thumbSize.y - textSize[1].y) / 2});
			memdc.SelectObject(wxNullBitmap);
#ifdef __WXOSX__
            bmp = wxBitmap(bmp.ConvertToImage(), -1, scale);
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

    bool isEnabled = false;
    if(GetValue()) {
        isEnabled = this->IsThisEnabled();
    }
    else {
        isEnabled = this->IsThisEnabled();
    }

    wxBitmap hoverBitmap = GetValue() ? m_hover_off : m_hover_on;
    if (hoverBitmap.IsOk()) {
        wxLogDebug("Setting hover bitmap");
        SetBitmapHover(hoverBitmap);
    } else {
        wxLogDebug("Hover bitmap is invalid");
    }

}

void SwitchButton::SetBitMapHover(const wxBitmap& bmp)
{
    if (bmp.IsOk())
    {
        wxBitmapToggleButton::SetBitmapHover(bmp);
    }
    else
    {
        wxLogError("Invalid bitmap passed to SetBitmapHover");
    }
}

void SwitchButton::SetBitmapHovered_(const std::string& on_hover_bmp_name, const std::string& off_hover_bmp_name)
{
    try {
            m_hover_on = create_scaled_bitmap(on_hover_bmp_name, m_parent, 24);
            m_hover_off = create_scaled_bitmap(off_hover_bmp_name, m_parent, 24);
    } catch (...) {
        m_hover_on = m_hover_off = wxNullBitmap;
    }
}

void SwitchButton::updateBitmapHover()
{
    // SetBitmapHover(GetValue() ? m_hover_off : m_hover_on); 
    // SetBitMapHover(m_hover_on); 

    // if(GetValue())
    //     CallAfter([this]() { SetBitmapHover(GetValue() ? m_hover_off : m_hover_on); });

}
