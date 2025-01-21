#ifndef slic3r_GUI_SwitchButton_hpp_
#define slic3r_GUI_SwitchButton_hpp_

#include "../wxExtensions.hpp"
#include "StateColor.hpp"

#include <wx/gdicmn.h>
#include <wx/bitmap.h>
#include <wx/tglbtn.h>

class SwitchButton : public wxBitmapToggleButton
{
public:
    SwitchButton(wxWindow* parent = NULL, wxWindowID id = wxID_ANY, std::string on = "toggle_on", std::string off = "toggle_off", bool set_bitmap_margins = false, const wxSize& bmp_margin_size = wxSize(0, 0));

public:
	void SetLabels(wxString const & lbl_on, wxString const & lbl_off);

	void SetTextColor(StateColor const &color);

	void SetTextColor2(StateColor const &color);

    void SetTrackColor(StateColor const &color);

	void SetThumbColor(StateColor const &color);

	void SetValue(bool value) override;

	void Rescale();
    ScalableBitmap& GetOnImg() { return m_on; }
    ScalableBitmap& GetOffImg() { return m_off; }

	void SetBitmapHovered_(const std::string& on_hover_bmp_name, const std::string& off_hover_bmp_name);
	void updateBitmapHover();
	
private:
	void update();
	void SetBitMapHover(const wxBitmap& bmp);

private:
	ScalableBitmap m_on;
	ScalableBitmap m_off;
	wxBitmap  m_hover_on;
	wxBitmap m_hover_off;

	wxString labels[2];
    StateColor   text_color;
    StateColor   text_color2;
	StateColor   track_color;
	StateColor   thumb_color;
	bool m_set_bitmap_margins {false};
	wxSize m_bmp_margin_size {0, 0};
};

#endif // !slic3r_GUI_SwitchButton_hpp_
