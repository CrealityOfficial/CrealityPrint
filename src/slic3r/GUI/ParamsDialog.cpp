#include "ParamsDialog.hpp"
#include "I18N.hpp"
#include "ParamsPanel.hpp"
#include "GUI_App.hpp"
#include "MainFrame.hpp"
#include "Tab.hpp"

#include "libslic3r/Utils.hpp"

namespace pt = boost::property_tree;
typedef pt::ptree JSON;

namespace Slic3r { 
namespace GUI {

// Get the work area (screen minus taskbar) for the display containing the given window
static wxRect get_display_client_area(wxWindow* window) {
    const auto idx = wxDisplay::GetFromWindow(window);
    wxDisplay display(idx != wxNOT_FOUND ? idx : 0u);
    return display.GetClientArea();
}
static wxSize get_params_dialog_default_size(wxWindow* window)
{
    return window->FromDIP(wxSize(1300, 650));
}
static void adjust_dialog_in_screen(DPIDialog* dialog, int max_dip_w = 0, int max_dip_h = 0) {
    wxRect screen_rect = get_display_client_area(dialog);
    int    pos_x       = dialog->GetPosition().x;
    int    pos_y       = dialog->GetPosition().y;
    int    size_x      = dialog->GetSize().x;
    int    size_y      = dialog->GetSize().y;
    int max_w = (int)(screen_rect.width * 0.90);
    int max_h = (int)(screen_rect.height * 0.90);
    if (max_dip_w > 0) max_w = std::min(max_w, (int)dialog->FromDIP(max_dip_w));
    if (max_dip_h > 0) max_h = std::min(max_h, (int)dialog->FromDIP(max_dip_h));
    bool resized = false;
    if (size_x > max_w) { size_x = max_w; resized = true; }
    if (size_y > max_h) { size_y = max_h; resized = true; }
    if (resized) { dialog->SetSize(size_x, size_y); }
    int dialog_x = pos_x;
    int dialog_y = pos_y;
    if (dialog_x + size_x > screen_rect.x + screen_rect.width) {
        dialog_x = screen_rect.x + screen_rect.width - size_x;
    }
    if (dialog_y + size_y > screen_rect.y + screen_rect.height) {
        dialog_y = screen_rect.y + screen_rect.height - size_y;
    }
    if (dialog_x < screen_rect.x) { dialog_x = screen_rect.x; }
    if (dialog_y < screen_rect.y) { dialog_y = screen_rect.y; }
    if (pos_x != dialog_x || pos_y != dialog_y) {
        dialog->SetPosition(wxPoint(dialog_x, dialog_y));
    }
}
static void apply_params_dialog_size(DPIDialog* dialog)
{
    dialog->SetSizeHints(wxDefaultSize, wxDefaultSize);
    dialog->SetMinSize(wxDefaultSize);
    dialog->SetSize(get_params_dialog_default_size(dialog));
    adjust_dialog_in_screen(dialog, 1300, 650);
}
ParamsDialog::ParamsDialog(wxWindow * parent)
	: DPIDialog(parent, wxID_ANY,  "", wxDefaultPosition,
		wxDefaultSize, wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER)
{
	m_panel = new ParamsPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBK_LEFT | wxTAB_TRAVERSAL);
	auto* topsizer = new wxBoxSizer(wxVERTICAL);
	topsizer->Add(m_panel, 1, wxALL | wxEXPAND, 0, NULL);
	SetSizer(topsizer);
	Layout();
    apply_params_dialog_size(this);
	Center();
    adjust_dialog_in_screen(this, 1300, 650);
    Bind(wxEVT_SHOW, [this](auto &event) {
        if (IsShown()) {
#ifndef __linux__
            // On GTK a wxWindowDisabler that excludes this dialog still ends up
            // making the dialog itself insensitive (the dialog is transient-for
            // the mainframe, which gets gtk_widget_set_sensitive(FALSE)). The
            // custom-drawn Creality widgets keep their normal look but stop
            // receiving any mouse/keyboard events, so every parameter input
            // appears read-only. See CrealityOfficial/CrealityPrint#565 / #552.
            // Skip the modal-style disabler on Linux; the dialog stays usable.
            m_winDisabler = new wxWindowDisabler(this);
#endif
        } else {
            delete m_winDisabler;
            m_winDisabler = nullptr;
        }
    });
	Bind(wxEVT_CLOSE_WINDOW, [this](auto& event) {
#if 0
		auto tab = dynamic_cast<Tab *>(m_panel->get_current_tab());
        if (event.CanVeto() && tab->m_presets->current_is_dirty()) {
			bool ok = tab->may_discard_current_dirty_preset();
			if (!ok)
				event.Veto();
            else {
                tab->m_presets->discard_current_changes();
                tab->load_current_preset();
                Hide();
            }
        } else {
            Hide();
        }
#else
        m_panel->OnPanelShowExit();

        Hide();
        if (!m_editing_filament_id.empty()) {
            FilamentInfomation *filament_info = new FilamentInfomation();
            filament_info->filament_id        = m_editing_filament_id;
            wxQueueEvent(wxGetApp().plater(), new SimpleEvent(EVT_MODIFY_FILAMENT, filament_info));
            m_editing_filament_id.clear();
        }
#endif
        wxGetApp().sidebar().finish_param_edit();
    });

    //wxGetApp().UpdateDlgDarkUI(this);
    Bind(wxEVT_SIZE, [this](auto& event) {
        Refresh();
        event.Skip(true);
        });
}

void ParamsDialog::Popup()
{
    wxGetApp().UpdateDlgDarkUI(this);
#ifdef __WIN32__
    Reparent(wxGetApp().mainframe);
#endif
    apply_params_dialog_size(this);
    Center();
    adjust_dialog_in_screen(this, 1300, 650);
    if (m_panel && m_panel->get_current_tab()) {
        bool just_edit = false;
        if (!m_editing_filament_id.empty()) just_edit = true;
        dynamic_cast<Tab *>(m_panel->get_current_tab())->set_just_edit(just_edit);
    }
    Show();
    adjust_dialog_in_screen(this, 1300, 650);
}

void ParamsDialog::on_dpi_changed(const wxRect &suggested_rect)
{
    m_panel->msw_rescale();
    Layout();
    apply_params_dialog_size(this);
	Refresh();
}

PrinterDialog::PrinterDialog(wxWindow* parent)
    : DPIDialog(parent, wxID_ANY, "", wxDefaultPosition,
        wxDefaultSize, wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER)
{
    Layout();
    Center();
    Bind(wxEVT_SHOW, [this](auto& event) {
        if (IsShown()) {
            m_winDisabler = new wxWindowDisabler(this);
        }
        else {
            delete m_winDisabler;
            m_winDisabler = nullptr;
        }
        });
    Bind(wxEVT_CLOSE_WINDOW, [this](auto& event) {

        Hide();

        });
}

void PrinterDialog::Popup()
{
    wxGetApp().UpdateDlgDarkUI(this);
#ifdef __WIN32__
    Reparent(wxGetApp().mainframe);
#endif
    Center();

    Show();
}

void PrinterDialog::on_dpi_changed(const wxRect& suggested_rect)
{
    Fit();
    SetSize({ 75 * em_unit(), 60 * em_unit() });
    //m_panel->msw_rescale();
    Refresh();
}

CusTitlePanel::CusTitlePanel(wxWindow* parent, const wxString& title)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL),
    m_title(title),
    m_isCloseButtonHover(false),
    m_dragging(false),
    m_MainSizer(new wxBoxSizer(wxHORIZONTAL))
{
    SetSizer(m_MainSizer);
    m_MainSizer->AddStretchSpacer();
    m_MainSizer->AddStretchSpacer();
    bool is_dark = Slic3r::GUI::wxGetApp().dark_mode();
    m_closeBitmap = create_scaled_bitmap(is_dark ? "topbar_close" : "topbar_close_light", nullptr, 18);
    m_closeBitmapHover = create_scaled_bitmap(is_dark ? "topbar_close_light" : "topbar_close", nullptr, 18);

    Bind(wxEVT_PAINT, &CusTitlePanel::OnPaint, this);
    Bind(wxEVT_LEFT_DOWN, &CusTitlePanel::OnMouseLeftDown, this);
    Bind(wxEVT_LEFT_UP, &CusTitlePanel::OnMouseLeftUp, this);
    Bind(wxEVT_MOTION, &CusTitlePanel::OnMouseMove, this);
}

void CusTitlePanel::OnPaint(wxPaintEvent& event) {
    wxBufferedPaintDC dc(this);
    wxSize size = GetClientSize();

    // 绘制背景
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(m_bgColor);
    dc.DrawRectangle(0, 0, size.x, size.y);

    // 绘制标题文本
    wxFont font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    font.SetWeight(wxFONTWEIGHT_BOLD);
    font.MakeLarger();
    dc.SetFont(font);

    // 设置文本颜色
    bool is_dark = Slic3r::GUI::wxGetApp().dark_mode();
    dc.SetTextForeground(is_dark ? *wxWHITE : *wxBLACK);

    wxSize textSize = dc.GetTextExtent(m_title);
    //int x = (size.x - textSize.x) / 2; // 水平居中
    int x = FromDIP(10); // 水平居中
    int y = (size.y - textSize.y) / 2; // 垂直居中

    dc.DrawText(m_title, x, y);

    // 绘制关闭按钮
    int closeButtonSize = 18;
    int closeButtonX = size.x - closeButtonSize - FromDIP(13);
    int closeButtonY = (size.y - closeButtonSize) / 2;

    dc.DrawBitmap(m_isCloseButtonHover ? m_closeBitmapHover : m_closeBitmap, closeButtonX, closeButtonY, true);

    event.Skip();
}

void CusTitlePanel::OnMouseLeftDown(wxMouseEvent& event) {
    m_dragging = true;
    m_dragStartPos = event.GetPosition();
    CaptureMouse();

    // 检查是否点击了关闭按钮
    wxSize size = GetClientSize();
    int closeButtonSize = 18;
    int closeButtonX = size.x - closeButtonSize - FromDIP(13);
    int closeButtonY = (size.y - closeButtonSize) / 2;

    wxRect closeButtonRect(closeButtonX, closeButtonY, closeButtonSize, closeButtonSize);
    if (closeButtonRect.Contains(event.GetPosition())) {
        wxWindow* parent = GetParent();
        while (parent && !dynamic_cast<CusDialog*>(parent)) {
            parent = parent->GetParent();
        }
        if (parent) {
            static_cast<CusDialog*>(parent)->Hide();
        }
        m_dragging = false; // 关闭按钮点击后取消拖动
    }

    event.Skip();
}

void CusTitlePanel::OnMouseLeftUp(wxMouseEvent& event) {
    if (m_dragging) {
        ReleaseMouse();
        m_dragging = false;
    }
    event.Skip();
}

void CusTitlePanel::OnMouseMove(wxMouseEvent& event) {
    if (m_dragging) {
        wxPoint delta = event.GetPosition() - m_dragStartPos;
        wxWindow* parent = GetParent();
        while (parent && !dynamic_cast<CusDialog*>(parent)) {
            parent = parent->GetParent();
        }
        if (parent) {
            parent->Move(parent->GetPosition() + delta);
        }
    }
    else {
        wxSize size = GetClientSize();
        int closeButtonSize = 18;
        int closeButtonX = size.x - closeButtonSize - FromDIP(13);
        int closeButtonY = (size.y - closeButtonSize) / 2;

        wxRect closeButtonRect(closeButtonX, closeButtonY, closeButtonSize, closeButtonSize);
        if (closeButtonRect.Contains(event.GetPosition())) {
            if (!m_isCloseButtonHover) {
                m_isCloseButtonHover = true;
                Refresh();
            }
        }
        else {
            if (m_isCloseButtonHover) {
                m_isCloseButtonHover = false;
                Refresh();
            }
        }
    }

    event.Skip();
}

void CusTitlePanel::setBgColor(const wxColor& color)
{
    if (m_bgColor == color)
        return;

    m_bgColor = color;
    Refresh();
}

void CusTitlePanel::setTitle(const wxString& title)
{
    if (m_title == title)
        return;
    m_title = title;
    Refresh();
}

CusDialog::CusDialog(wxWindow* parent, wxWindowID id,
    const wxString& title,
    const wxPoint& pos,
    const wxSize& size,
    long style,
    const wxString& name)
    :DPIDialog(parent, wxID_ANY, "", wxDefaultPosition,
        wxDefaultSize, wxDEFAULT_DIALOG_STYLE & ~(wxRESIZE_BORDER | wxSYSTEM_MENU | wxCAPTION | wxMINIMIZE_BOX | wxMAXIMIZE_BOX | wxCLOSE_BOX))
{
    m_TitleHeight = FromDIP(50);
    auto* topsizer = new wxBoxSizer(wxVERTICAL);

    m_ContentPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBK_LEFT | wxTAB_TRAVERSAL);
    m_ContentPanel->SetBackgroundColour(wxColor("#4b4b4d"));
    m_TitlePanel = new CusTitlePanel(this,"");
    m_TitlePanel->SetMinSize(wxSize(-1, m_TitleHeight));
    m_TitlePanel->setBgColor(wxColor("#4B4B4D"));

    wxPanel* line = new wxPanel(this);
    line->SetSize(-1, 1);
    line->SetBackgroundColour("#616165");
    line->Show(false);
    topsizer->Add(m_TitlePanel, 0, wxALL | wxEXPAND, 0);
    topsizer->Add(line, 0, wxALL | wxEXPAND, 0);
    topsizer->Add(m_ContentPanel, 1, wxALL | wxEXPAND, 0, NULL);

    int un = em_unit();
    SetSizerAndFit(topsizer);
    SetSize({ 110 * em_unit(), 72 * em_unit() });

    Layout();
    Center();
    Bind(wxEVT_SHOW, [this](auto& event) {
        if (IsShown()) {
            m_winDisabler = new wxWindowDisabler(this);
        }
        else {
            delete m_winDisabler;
            m_winDisabler = nullptr;
        }
        });
    Bind(wxEVT_CLOSE_WINDOW, [this](auto& event) {
        Hide();
        });
}

void CusDialog::Popup()
{
    wxGetApp().UpdateDlgDarkUI(this);
#ifdef __WIN32__
    Reparent(wxGetApp().mainframe);
#endif
    Center();
    Show();
}

void CusDialog::setTitleHeight(int height)
{
    m_TitleHeight = height;
}

wxPanel* CusDialog::titlePanel()
{
    return m_TitlePanel;
}

void CusDialog::addToolButton(wxPanel* btn)
{
    btn->Reparent(m_TitlePanel);
    wxSizer* sizer = m_TitlePanel->GetSizer();
    if (!sizer)
    {
        wxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
        m_TitlePanel->SetSizer(sizer);
        sizer->AddStretchSpacer();
        sizer->AddStretchSpacer();
    }
    //sizer->Add(btn, 0, wxBOTTOM, 0);
    sizer = m_TitlePanel->GetSizer();
    sizer->Insert(1, btn, 0, wxALIGN_BOTTOM, 13);
}

void CusDialog::OnPaint(wxPaintEvent& event) {
    wxBufferedPaintDC dc(this);
    wxSize size = GetClientSize();

    dc.SetPen(*wxBLACK_PEN);
    dc.DrawRectangle(0, 0, size.x, size.y);

    int resizeAreaSize = 10;
    dc.SetBrush(*wxBLACK_BRUSH);
    dc.DrawRectangle(size.x - resizeAreaSize, size.y - resizeAreaSize, resizeAreaSize, resizeAreaSize);

    event.Skip();
}

void CusDialog::on_dpi_changed(const wxRect& suggested_rect)
{
    Fit();
    SetSize({ 75 * em_unit(), 60 * em_unit() });
    //m_panel->msw_rescale();
    Refresh();
}

void CusDialog::eventFilter()
{

}

} // namespace GUI
} // namespace Slic3r
