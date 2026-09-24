#ifndef slic3r_GUI_TextInput_hpp_
#define slic3r_GUI_TextInput_hpp_

#include <wx/textctrl.h>
#include "StaticBox.hpp"

class TextInput : public wxNavigationEnabled<StaticBox>
{

    wxSize labelSize;
    ScalableBitmap icon;
    StateColor     label_color;
    StateColor     text_color;
    wxTextCtrl * text_ctrl;

    static const int TextInputWidth = 200;
    static const int TextInputHeight = 50;

public:
    TextInput();

    TextInput(wxWindow *     parent,
              wxString       text,
              wxString       label = "",
              wxString       icon  = "",
              const wxPoint &pos   = wxDefaultPosition,
              const wxSize & size  = wxDefaultSize,
              long           style = 0);

public:
    void Create(wxWindow *     parent,
              wxString       text,
              wxString       label = "",
              wxString       icon  = "",
              const wxPoint &pos   = wxDefaultPosition,
              const wxSize & size  = wxDefaultSize,
              long           style = 0);

    void SetCornerRadius(double radius);

    void SetLabel(const wxString& label);

    void SetMaxLength(int maxLength);

    void SetIcon(const wxBitmap & icon);

    void SetIcon(const wxString & icon);

    void SetLabelColor(StateColor const &color);

    void SetTextColor(StateColor const &color);

    virtual void Rescale();

    virtual bool Enable(bool enable = true) override;

    virtual void SetMinSize(const wxSize& size) override;

    wxTextCtrl *GetTextCtrl() { return text_ctrl; }

    wxTextCtrl const *GetTextCtrl() const { return text_ctrl; }

    void setLeftMargin(int margin) { m_LeftMargin = margin; }

protected:
    // Replace an already-rendered icon without calling the virtual Rescale().
    // Used by derived controls while they are inside their own Rescale() path.
    void SetIconBitmapWithoutRescale(const wxBitmap& bitmap);

    virtual void OnEdit() {}

	virtual void DoSetSize(
		int x, int y, int width, int height, int sizeFlags = wxSIZE_AUTO);

    void DoSetToolTipText(wxString const &tip) override;

private:
    void paintEvent(wxPaintEvent& evt);

    void render(wxDC& dc);

    void messureSize();

    int m_LeftMargin = 5;
    DECLARE_EVENT_TABLE()
};

#endif // !slic3r_GUI_TextInput_hpp_
