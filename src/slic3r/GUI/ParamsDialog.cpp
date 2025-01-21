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


ParamsDialog::ParamsDialog(wxWindow * parent)
	: DPIDialog(parent, wxID_ANY,  "", wxDefaultPosition,
		wxDefaultSize, wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER)
{
	m_panel = new ParamsPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBK_LEFT | wxTAB_TRAVERSAL);
	auto* topsizer = new wxBoxSizer(wxVERTICAL);
	topsizer->Add(m_panel, 1, wxALL | wxEXPAND, 0, NULL);

    int un = em_unit();
	SetSizerAndFit(topsizer);
	SetSize({110 * em_unit(), 65 * em_unit()});

	Layout();
    
	Center();
    Bind(wxEVT_SHOW, [this](auto &event) {
        if (IsShown()) {
            m_winDisabler = new wxWindowDisabler(this);
            wxPoint position = GetPosition();
            if (position.y < 0)
            {
                wxSize newSize = wxSize(110 * em_unit(), 65 * em_unit() + position.y*2);
                SetSize(newSize);
                SetPosition(wxPoint(position.x, 0));
            }
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
    Center();
    if (m_panel && m_panel->get_current_tab()) {
        bool just_edit = false;
        if (!m_editing_filament_id.empty()) just_edit = true;
        dynamic_cast<Tab *>(m_panel->get_current_tab())->set_just_edit(just_edit);
    }
    Show();
}

void ParamsDialog::on_dpi_changed(const wxRect &suggested_rect)
{
	Fit();
	SetSize({75 * em_unit(), 60 * em_unit()});
	m_panel->msw_rescale();
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
