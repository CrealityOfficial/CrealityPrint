#ifndef slic3r_GUI_SwitchButton_hpp_
#define slic3r_GUI_SwitchButton_hpp_

#include "../wxExtensions.hpp"
#include "StateColor.hpp"

#include <wx/tglbtn.h>
#include "Label.hpp"
#include "Button.hpp"

#include <vector>

wxDECLARE_EVENT(wxCUSTOMEVT_SWITCH_POS, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_MULTISWITCH_SELECTION, wxCommandEvent);

class SwitchButton : public wxBitmapToggleButton
{
public:
	SwitchButton(wxWindow * parent = NULL, wxWindowID id = wxID_ANY);

public:
	void SetLabels(wxString const & lbl_on, wxString const & lbl_off);

	void SetTextColor(StateColor const &color);

	void SetTextColor2(StateColor const &color);

    void SetTrackColor(StateColor const &color);

	void SetThumbColor(StateColor const &color);

	void SetValue(bool value) override;

	void Rescale();

private:
	void update();

private:
	ScalableBitmap m_on;
	ScalableBitmap m_off;

	wxString labels[2];
    StateColor   text_color;
    StateColor   text_color2;
	StateColor   track_color;
	StateColor   thumb_color;
};

class SwitchBoard : public wxWindow
{
public:
    SwitchBoard(wxWindow *parent = NULL, wxString leftL = "", wxString right = "", wxSize size = wxDefaultSize);
    wxString leftLabel;
    wxString rightLabel;

	void updateState(wxString target);

	bool switch_left{false};
    bool switch_right{false};
    bool is_enable {true};

    void* client_data = nullptr;/*MachineObject* in StatusPanel*/

public:
    void Enable();
    void Disable();
    bool IsEnabled(){return is_enable;};

    void  SetClientData(void* data) { client_data = data; };
    void* GetClientData() { return client_data; };

    void SetAutoDisableWhenSwitch() { auto_disable_when_switch = true; };

protected:
    void paintEvent(wxPaintEvent& evt);
    void render(wxDC& dc);
    void doRender(wxDC& dc);
    void on_left_down(wxMouseEvent& evt);

private:
    bool auto_disable_when_switch = false;
};

// A compact segmented selector used by process presets with two or more
// physical extruders. Unlike SwitchBoard, the number of segments is dynamic.
class MultiSwitchBoard : public wxWindow
{
public:
    enum class Style { Segmented, Underline };

    MultiSwitchBoard(wxWindow* parent = nullptr, Style style = Style::Segmented,
                     wxWindowID id = wxID_ANY);

    void SetOptions(const std::vector<wxString>& options);
    unsigned int GetCount() const { return static_cast<unsigned int>(m_options.size()); }

    int  GetSelection() const { return m_selection; }
    void SetSelection(int selection);
    void Rescale();
    void sys_color_changed();

    bool Enable(bool enable = true) override;

private:
    void paintEvent(wxPaintEvent& event);
    void on_left_down(wxMouseEvent& event);
    void update_min_size();

    std::vector<wxString> m_options;
    int                   m_selection {-1};
    Style                 m_style {Style::Segmented};
};

#endif // !slic3r_GUI_SwitchButton_hpp_

