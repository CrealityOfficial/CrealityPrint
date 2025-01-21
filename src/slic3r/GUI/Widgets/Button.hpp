#ifndef slic3r_GUI_Button_hpp_
#define slic3r_GUI_Button_hpp_

#include "../wxExtensions.hpp"
#include "StaticBox.hpp"

class Button : public StaticBox
{
    wxRect textSize;
    wxSize minSize; // set by outer
    wxSize paddingSize;
    ScalableBitmap active_icon;
    ScalableBitmap inactive_icon;

    StateColor   text_color;

    bool pressedDown = false;
    bool m_selected  = true;
    bool canFocus  = true;
    bool isCenter = true;
    bool isFtBold = false;

    static const int buttonWidth = 200;
    static const int buttonHeight = 50;

public:
    Button();

    Button(wxWindow* parent, wxString text, wxString icon = "", long style = 0, int iconSize = 0, wxWindowID btn_id = wxID_ANY);

    bool Create(wxWindow* parent, wxString text, wxString icon = "", long style = 0, int iconSize = 0, wxWindowID btn_id = wxID_ANY);

    void SetLabel(const wxString& label) override;

    bool SetFont(const wxFont& font) override;

    void SetIcon(const wxString& icon);

    void SetInactiveIcon(const wxString& icon);

    void SetMinSize(const wxSize& size) override;
    
    void SetPaddingSize(const wxSize& size);
    
    void SetTextColor(StateColor const &color);

    void SetTextColorNormal(wxColor const &color);

    void SetSelected(bool selected = true) { m_selected = selected; }

    bool Enable(bool enable = true) override;

    void SetCanFocus(bool canFocus) override;

    void SetValue(bool state);

    bool GetValue() const;

    void SetCenter(bool isCenter);

    void Rescale();

    void SetFontBold(bool bBold);

protected:
#ifdef __WIN32__
    WXLRESULT MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam) override;
#endif

private:
    void paintEvent(wxPaintEvent& evt);

    void render(wxDC& dc);

    void messureSize();

    // some useful events
    void mouseDown(wxMouseEvent& event);
    void mouseReleased(wxMouseEvent& event);
    void mouseCaptureLost(wxMouseCaptureLostEvent &event);
    void keyDownUp(wxKeyEvent &event);

    void sendButtonEvent();

    DECLARE_EVENT_TABLE()
};

class RoundedPanel : public wxPanel {
public:
    RoundedPanel(wxWindow* parent, wxWindowID id = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxTAB_TRAVERSAL,
        const wxString& name = wxPanelNameStr);

    void SetTopLeftRadius(int radius);
    void SetTopRightRadius(int radius);
    void SetBottomRightRadius(int radius);
    void SetBottomLeftRadius(int radius);
    void SetBorderWidth(int width);
    void SetBorderColor(const wxColour& color);
    void AddRoundedRectangleWithDifferentRadii(wxGraphicsPath& path, wxDouble x, wxDouble y, wxDouble w, wxDouble h,
        wxDouble radiusTL, wxDouble radiusTR, wxDouble radiusBR, wxDouble radiusBL);

private:
    void OnPaint(wxPaintEvent& event);

    int m_topLeftRadius;
    int m_topRightRadius;
    int m_bottomRightRadius;
    int m_bottomLeftRadius;
    int m_borderWidth;
    wxColour m_borderColor;
};

class CustomRoundCornerButton : public wxPanel {
public:
    CustomRoundCornerButton(wxWindow* parent, wxWindowID id, const wxString& label,
        const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
        long style = 0, const wxValidator& validator = wxDefaultValidator,
        const wxString& name = wxPanelNameStr);


    std::function<void()> onClicked;

    void SetCornerRadiusTopLeft(int radius);

    void SetCornerRadiusTopRight(int radius);

    void SetCornerRadiusBottomLeft(int radius);

    void SetCornerRadiusBottomRight(int radius);

    void SetNormalColor(const wxColour& color);

    void SetHoverColor(const wxColour& color);

    void SetPressedColor(const wxColour& color);

    void SetBorderColor(const wxColour& color);
    void SetIsChecked(bool isChecked);

private:
    wxString m_label;
    int m_radiusTopLeft, m_radiusTopRight, m_radiusBottomLeft, m_radiusBottomRight;
    wxColour m_normalColor, m_hoverColor, m_pressedColor, m_borderColor;
    bool m_isHovered, m_isPressed, m_isChecked;

    void OnPaint(wxPaintEvent& event);

    void AddRoundedRectangleWithDifferentRadii(wxGraphicsPath& path, wxDouble x, wxDouble y, wxDouble w, wxDouble h,
        wxDouble radiusTL, wxDouble radiusTR, wxDouble radiusBR, wxDouble radiusBL);

    void OnMouseEnter(wxMouseEvent& event);

    void OnMouseLeave(wxMouseEvent& event);

    void OnMouseDown(wxMouseEvent& event);

    void OnMouseUp(wxMouseEvent& event);
};

#endif // !slic3r_GUI_Button_hpp_
