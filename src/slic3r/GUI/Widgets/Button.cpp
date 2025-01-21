#include "Button.hpp"
#include "Label.hpp"

#include <wx/dcgraph.h>

BEGIN_EVENT_TABLE(Button, StaticBox)

EVT_LEFT_DOWN(Button::mouseDown)
EVT_LEFT_UP(Button::mouseReleased)
EVT_MOUSE_CAPTURE_LOST(Button::mouseCaptureLost)
EVT_KEY_DOWN(Button::keyDownUp)
EVT_KEY_UP(Button::keyDownUp)

// catch paint events
EVT_PAINT(Button::paintEvent)

END_EVENT_TABLE()

/*
 * Called by the system of by wxWidgets when the panel needs
 * to be redrawn. You can also trigger this call by
 * calling Refresh()/Update().
 */

Button::Button()
    : paddingSize(5, 5)
{
    background_color = StateColor(
        std::make_pair(0xF0F0F1, (int) StateColor::Disabled),
        std::make_pair(0x52c7b8, (int) StateColor::Hovered | StateColor::Checked),
        std::make_pair(0x009688, (int) StateColor::Checked),
        std::make_pair(*wxLIGHT_GREY, (int) StateColor::Hovered), 
        std::make_pair(*wxWHITE, (int) StateColor::Normal));
    text_color       = StateColor(
        std::make_pair(*wxLIGHT_GREY, (int) StateColor::Disabled), 
        std::make_pair(*wxBLACK, (int) StateColor::Normal));
}

Button::Button(wxWindow* parent, wxString text, wxString icon, long style, int iconSize, wxWindowID btn_id)
    : Button()
{
    Create(parent, text, icon, style, iconSize, btn_id);
}

bool Button::Create(wxWindow* parent, wxString text, wxString icon, long style, int iconSize, wxWindowID btn_id)
{
    StaticBox::Create(parent, btn_id, wxDefaultPosition, wxDefaultSize, style);
    state_handler.attach({&text_color});
    state_handler.update_binds();
    //BBS set default font
    SetFont(Label::Body_14);
    wxWindow::SetLabel(text);
    if (!icon.IsEmpty()) {
        //BBS set button icon default size to 20
        this->active_icon = ScalableBitmap(this, icon.ToStdString(), iconSize > 0 ? iconSize : 20);
    }
    messureSize();
    return true;
}

void Button::SetLabel(const wxString& label)
{
    wxWindow::SetLabel(label);
    messureSize();
    Refresh();
}

bool Button::SetFont(const wxFont& font)
{
    wxWindow::SetFont(font);
    messureSize();
    Refresh();
    return true;
}

void Button::SetIcon(const wxString& icon)
{
    if (!icon.IsEmpty()) {
        //BBS set button icon default size to 20
        this->active_icon = ScalableBitmap(this, icon.ToStdString(), this->active_icon.px_cnt());
    }
    else
    {
        this->active_icon = ScalableBitmap();
    }
    Refresh();
}

void Button::SetInactiveIcon(const wxString &icon)
{
    if (!icon.IsEmpty()) {
        // BBS set button icon default size to 20
        this->inactive_icon = ScalableBitmap(this, icon.ToStdString(), this->active_icon.px_cnt());
    } else {
        this->inactive_icon = ScalableBitmap();
    }
    Refresh();
}

void Button::SetMinSize(const wxSize& size)
{
    minSize = size;
    messureSize();
}

void Button::SetPaddingSize(const wxSize& size)
{
    paddingSize = size;
    messureSize();
}

void Button::SetTextColor(StateColor const& color)
{
    text_color = color;
    state_handler.update_binds();
    Refresh();
}

void Button::SetTextColorNormal(wxColor const &color)
{
    text_color.setColorForStates(color, 0);
    Refresh();
}

bool Button::Enable(bool enable)
{
    bool result = wxWindow::Enable(enable);
    if (result) {
        wxCommandEvent e(EVT_ENABLE_CHANGED);
        e.SetEventObject(this);
        GetEventHandler()->ProcessEvent(e);
    }
    return result;
}

void Button::SetCanFocus(bool canFocus) { this->canFocus = canFocus; }

void Button::SetValue(bool state)
{
    /*if (GetValue() == state) return;*/
    state_handler.set_state(state ? StateHandler::Checked : 0, StateHandler::Checked);
    Refresh();
}

bool Button::GetValue() const { return state_handler.states() & StateHandler::Checked; }

void Button::SetCenter(bool isCenter)
{
    this->isCenter = isCenter;
}

void Button::Rescale()
{
    if (this->active_icon.bmp().IsOk())
        this->active_icon.msw_rescale();

    if (this->inactive_icon.bmp().IsOk())
        this->inactive_icon.msw_rescale();

    messureSize();
}

void Button::SetFontBold(bool bBold) 
{
    isFtBold = bBold;
}

void Button::paintEvent(wxPaintEvent& evt)
{
    // depending on your system you may need to look at double-buffered dcs
    wxPaintDC dc(this);
    render(dc);
}

/*
 * Here we do the actual rendering. I put it in a separate
 * method so that it can work no matter what type of DC
 * (e.g. wxPaintDC or wxClientDC) is used.
 */
void Button::render(wxDC& dc)
{
    StaticBox::render(dc);

    int states = state_handler.states();
    wxSize size = GetSize();
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    // calc content size
    wxSize szIcon;
    wxSize szContent = textSize.GetSize();

    ScalableBitmap icon;
    if (m_selected || ((states & (int)StateColor::State::Hovered) != 0))
        icon = active_icon;
    else
        icon = inactive_icon;
    int padding = 5;
    if (icon.bmp().IsOk()) {
        if (szContent.y > 0) {
            //BBS norrow size between text and icon
            szContent.x += padding;
        }
        szIcon = icon.GetBmpSize();
        szContent.x += szIcon.x;
        if (szIcon.y > szContent.y)
            szContent.y = szIcon.y;
        if (szContent.x > size.x) {
            int d = std::min(padding, szContent.x - size.x);
            padding -= d;
            szContent.x -= d;
        }
    }
    // move to center
    wxRect rcContent = { {0, 0}, size };
    if (isCenter) {
        wxSize offset = (size - szContent) / 2;
        if (offset.x < 0) offset.x = 0;
        rcContent.Deflate(offset.x, offset.y);
    }
    // start draw
    wxPoint pt = rcContent.GetLeftTop();
    if (icon.bmp().IsOk()) {
        pt.y += (rcContent.height - szIcon.y) / 2;
        dc.DrawBitmap(icon.bmp(), pt);
        //BBS norrow size between text and icon
        pt.x += szIcon.x + padding;
        pt.y = rcContent.y;
    }
    auto text = GetLabel();
    if (!text.IsEmpty()) {
        if (pt.x + textSize.width > size.x)
            text = wxControl::Ellipsize(text, dc, wxELLIPSIZE_END, size.x - pt.x);
        pt.y += (rcContent.height - textSize.height) / 2;
        dc.SetTextForeground(text_color.colorForStates(states));

        if (isFtBold)
        {
            wxFont ft = dc.GetFont();
            ft.SetWeight(wxFONTWEIGHT_BOLD);
            dc.SetFont(ft);
        }
#if 0
        dc.SetBrush(*wxLIGHT_GREY);
        dc.SetPen(wxPen(*wxLIGHT_GREY));
        dc.DrawRectangle(pt, textSize.GetSize());
#endif
#ifdef __WXOSX__
        pt.y -= textSize.x / 2;
#endif
        dc.DrawText(text, pt);
    }
}

void Button::messureSize()
{
    wxClientDC dc(this);
    dc.GetTextExtent(GetLabel(), &textSize.width, &textSize.height, &textSize.x, &textSize.y);
    wxSize szContent = textSize.GetSize();
    if (this->active_icon.bmp().IsOk()) {
        if (szContent.y > 0) {
            //BBS norrow size between text and icon
            szContent.x += 5;
        }
        wxSize szIcon = this->active_icon.GetBmpSize();
        szContent.x += szIcon.x;
        if (szIcon.y > szContent.y)
            szContent.y = szIcon.y;
    }
    wxSize size = szContent + paddingSize * 2;
    if (minSize.GetHeight() > 0)
        size.SetHeight(minSize.GetHeight());

    if (minSize.GetWidth() > size.GetWidth())
        wxWindow::SetMinSize(minSize);
    else
        wxWindow::SetMinSize(size);
}

void Button::mouseDown(wxMouseEvent& event)
{
    event.Skip();
    pressedDown = true;
    if (canFocus)
        SetFocus();
    if (!HasCapture())
        CaptureMouse();
}

void Button::mouseReleased(wxMouseEvent& event)
{
    event.Skip();
    if (pressedDown) {
        pressedDown = false;
        if (HasCapture())
            ReleaseMouse();
        if (wxRect({0, 0}, GetSize()).Contains(event.GetPosition()))
            sendButtonEvent();
    }
}

void Button::mouseCaptureLost(wxMouseCaptureLostEvent &event)
{
    wxMouseEvent evt;
    mouseReleased(evt);
}

void Button::keyDownUp(wxKeyEvent &event)
{
    if (event.GetKeyCode() == WXK_SPACE || event.GetKeyCode() == WXK_RETURN) {
        wxMouseEvent evt(event.GetEventType() == wxEVT_KEY_UP ? wxEVT_LEFT_UP : wxEVT_LEFT_DOWN);
        event.SetEventObject(this);
        GetEventHandler()->ProcessEvent(evt);
        return;
    }
    if (event.GetEventType() == wxEVT_KEY_DOWN &&
        (event.GetKeyCode() == WXK_TAB || event.GetKeyCode() == WXK_LEFT || event.GetKeyCode() == WXK_RIGHT 
        || event.GetKeyCode() == WXK_UP || event.GetKeyCode() == WXK_DOWN))
        HandleAsNavigationKey(event);
    else
        event.Skip();
}

void Button::sendButtonEvent()
{
    wxCommandEvent event(wxEVT_COMMAND_BUTTON_CLICKED, GetId());
    event.SetEventObject(this);
    GetEventHandler()->ProcessEvent(event);
}

#ifdef __WIN32__

WXLRESULT Button::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
{
    if (nMsg == WM_GETDLGCODE) { return DLGC_WANTMESSAGE; }
    if (nMsg == WM_KEYDOWN) {
        wxKeyEvent event(CreateKeyEvent(wxEVT_KEY_DOWN, wParam, lParam));
        switch (wParam) {
        case WXK_RETURN: { // WXK_RETURN key is handled by default button
            GetEventHandler()->ProcessEvent(event);
            return 0;
        }
        }
    }
    return wxWindow::MSWWindowProc(nMsg, wParam, lParam);
}
#endif
RoundedPanel::RoundedPanel(wxWindow* parent, wxWindowID id,
    const wxPoint& pos,
    const wxSize& size,
    long style,
    const wxString& name)
    : wxPanel(parent, id, pos, size, style, name),
    m_topLeftRadius(10),
    m_topRightRadius(10),
    m_bottomRightRadius(0),
    m_bottomLeftRadius(0),
    m_borderWidth(2),
    m_borderColor(wxColour("#6e6e72")) {
    Bind(wxEVT_PAINT, &RoundedPanel::OnPaint, this);
}

void RoundedPanel::SetTopLeftRadius(int radius) { m_topLeftRadius = radius; Refresh(); }
void RoundedPanel::SetTopRightRadius(int radius) { m_topRightRadius = radius; Refresh(); }
void RoundedPanel::SetBottomRightRadius(int radius) { m_bottomRightRadius = radius; Refresh(); }
void RoundedPanel::SetBottomLeftRadius(int radius) { m_bottomLeftRadius = radius; Refresh(); }
void RoundedPanel::SetBorderWidth(int width) { m_borderWidth = width; Refresh(); }
void RoundedPanel::SetBorderColor(const wxColour& color) { m_borderColor = color; Refresh(); }
void RoundedPanel::AddRoundedRectangleWithDifferentRadii(wxGraphicsPath& path, wxDouble x, wxDouble y, wxDouble w, wxDouble h,
    wxDouble radiusTL, wxDouble radiusTR, wxDouble radiusBR, wxDouble radiusBL) {
    // Top left corner
    path.MoveToPoint(x + radiusTL, y);
    path.AddLineToPoint(x + w - radiusTR, y);
    path.AddArcToPoint(x + w, y, x + w, y + radiusTR, radiusTR);

    // Top right corner
    path.AddLineToPoint(x + w, y + h - radiusBR);
    path.AddArcToPoint(x + w, y + h, x + w - radiusBR, y + h, radiusBR);

    // Bottom right corner
    path.AddLineToPoint(x + radiusBL, y + h);
    path.AddArcToPoint(x, y + h, x, y + h - radiusBL, radiusBL);

    // Bottom left corner
    path.AddLineToPoint(x, y + radiusTL);
    path.AddArcToPoint(x, y, x + radiusTL, y, radiusTL);

    path.CloseSubpath();
}

void RoundedPanel::OnPaint(wxPaintEvent& event) {
    wxBufferedPaintDC dc(this);
    wxSize size = GetClientSize();
    if (size.x <= 0 || size.y <= 0) {
        // �����ť��СΪ���ǳ�С�����������Ϣ
        wxLogDebug("Button size is too small: %dx%d", size.x, size.y);
        return;
    }
    wxColour currentColor("#4B4B4D");

    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    if (gc) {
        wxGraphicsPath path = gc->CreatePath();
        AddRoundedRectangleWithDifferentRadii(path, 0, 0, size.x-1, size.y,
            m_topLeftRadius, m_topRightRadius,
            m_bottomRightRadius, m_bottomLeftRadius);
        gc->SetBrush(wxBrush(currentColor));
        gc->FillPath(path);
        
        gc->SetPen(wxPen(m_borderColor, 1));
        gc->StrokePath(path);

        // �ͷŻ�ͼ������
        delete gc;
    }

    dc.SetTextForeground(*wxWHITE);
    event.Skip();
}

CustomRoundCornerButton::CustomRoundCornerButton(wxWindow* parent, wxWindowID id, const wxString& label,
    const wxPoint& pos, const wxSize& size,
    long style, const wxValidator& validator,
    const wxString& name)
    : wxPanel(parent),
    m_label(label),
    m_radiusTopLeft(10), m_radiusTopRight(10), m_radiusBottomLeft(0), m_radiusBottomRight(0),
    m_normalColor(wxColor("#4b4b4d")), m_hoverColor("#17cc5f"), m_pressedColor("#17cc5f"), m_borderColor("#6e6e72"),
    m_isHovered(false), m_isPressed(false), m_isChecked(false){
    Bind(wxEVT_PAINT, &CustomRoundCornerButton::OnPaint, this);
    Bind(wxEVT_ENTER_WINDOW, &CustomRoundCornerButton::OnMouseEnter, this);
    Bind(wxEVT_LEAVE_WINDOW, &CustomRoundCornerButton::OnMouseLeave, this);
    Bind(wxEVT_LEFT_DOWN, &CustomRoundCornerButton::OnMouseDown, this);
    Bind(wxEVT_LEFT_UP, &CustomRoundCornerButton::OnMouseUp, this);

    // ������С��С
    SetMinSize(size);
}

void CustomRoundCornerButton::SetCornerRadiusTopLeft(int radius) {
    m_radiusTopLeft = radius;
    Refresh();
}

void CustomRoundCornerButton::SetCornerRadiusTopRight(int radius) {
    m_radiusTopRight = radius;
    Refresh();
}

void CustomRoundCornerButton::SetCornerRadiusBottomLeft(int radius) {
    m_radiusBottomLeft = radius;
    Refresh();
}

void CustomRoundCornerButton::SetCornerRadiusBottomRight(int radius) {
    m_radiusBottomRight = radius;
    Refresh();
}

void CustomRoundCornerButton::SetNormalColor(const wxColour& color) {
    m_normalColor = color;
    Refresh();
}

void CustomRoundCornerButton::SetHoverColor(const wxColour& color) {
    m_hoverColor = color;
    Refresh();
}

void CustomRoundCornerButton::SetPressedColor(const wxColour& color) {
    m_pressedColor = color;
    Refresh();
}

void CustomRoundCornerButton::SetBorderColor(const wxColour& color)
{
    if (m_borderColor == color)
        return;
    m_borderColor = color;
    Refresh();
}

void CustomRoundCornerButton::SetIsChecked(bool isChecked)
{
    if (m_isChecked == isChecked)
        return;
    m_isChecked = isChecked;
    Refresh();
}

void CustomRoundCornerButton::OnPaint(wxPaintEvent& event) {
    wxBufferedPaintDC dc(this);
    wxSize size = GetClientSize();
    if (size.x <= 0 || size.y <= 0) {
        // �����ť��СΪ���ǳ�С�����������Ϣ
        wxLogDebug("Button size is too small: %dx%d", size.x, size.y);
        return;
    }
    wxColour currentColor;

    if (m_isPressed || m_isChecked) {
        currentColor = m_pressedColor;
    }
    else if (m_isHovered) {
        currentColor = m_hoverColor;
    }
    else {
        currentColor = m_normalColor;
    }

    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    if (gc) {
        wxGraphicsPath path = gc->CreatePath();
        AddRoundedRectangleWithDifferentRadii(path, 0, 0, size.x, size.y,
            m_radiusTopLeft, m_radiusTopRight,
            m_radiusBottomRight, m_radiusBottomLeft);
        gc->SetBrush(wxBrush(currentColor));
        gc->FillPath(path);

        gc->SetPen(wxPen(m_borderColor, 1));
        gc->StrokePath(path);

        // �ͷŻ�ͼ������
        delete gc;
    }

    dc.SetTextForeground(*wxWHITE);
    dc.DrawLabel(m_label, wxRect(size), wxALIGN_CENTER);
    event.Skip();
}

void CustomRoundCornerButton::AddRoundedRectangleWithDifferentRadii(wxGraphicsPath& path, wxDouble x, wxDouble y, wxDouble w, wxDouble h,
    wxDouble radiusTL, wxDouble radiusTR, wxDouble radiusBR, wxDouble radiusBL) {
    // Top left corner
    path.MoveToPoint(x + radiusTL, y);
    path.AddLineToPoint(x + w - radiusTR, y);
    path.AddArcToPoint(x + w, y, x + w, y + radiusTR, radiusTR);

    // Top right corner
    path.AddLineToPoint(x + w, y + h - radiusBR);
    path.AddArcToPoint(x + w, y + h, x + w - radiusBR, y + h, radiusBR);

    // Bottom right corner
    path.AddLineToPoint(x + radiusBL, y + h);
    path.AddArcToPoint(x, y + h, x, y + h - radiusBL, radiusBL);

    // Bottom left corner
    path.AddLineToPoint(x, y + radiusTL);
    path.AddArcToPoint(x, y, x + radiusTL, y, radiusTL);

    path.CloseSubpath();
}

void CustomRoundCornerButton::OnMouseEnter(wxMouseEvent& event) {
    m_isHovered = true;
    Refresh();
    event.Skip();
}

void CustomRoundCornerButton::OnMouseLeave(wxMouseEvent& event) {
    m_isHovered = false;
    Refresh();
    event.Skip();
}

void CustomRoundCornerButton::OnMouseDown(wxMouseEvent& event) {
    m_isPressed = true;
    onClicked();
    m_isChecked = !m_isChecked;
    Refresh();
    event.Skip();
}

void CustomRoundCornerButton::OnMouseUp(wxMouseEvent& event) {
    m_isPressed = false;
    Refresh();
    event.Skip();
}

