#include "FilamentPanel.h"
#include <cassert>
#include <fstream>
#include <mutex>
#include <string>
#include <wx/dcclient.h>
#include <wx/dcgraph.h>
#include <wx/dcmemory.h>
#include "ImGuiWrapper.hpp"
#include "wx/menu.h"
#include "wx/colour.h"
#include "wx/wx.h"
#include <wx/colordlg.h>
#include "GUI_App.hpp"
#include "ColorSpaceConvert.hpp"
#include "Plater.hpp"
#include "libslic3r/Preset.hpp"
#include "Tab.hpp"
#include "MainFrame.hpp"
#include "libslic3r/Config.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "slic3r/GUI/print_manage/Utils.hpp"
#include "slic3r/GUI/print_manage/PrinterBoxFilamentPanel.hpp"
#include "slic3r/GUI/PartPlate.hpp"
#include <cstdint> 
#include "print_manage/data/DataCenter.hpp"
#include "slic3r/Utils/ProfileFamilyLoader.hpp"
#include "LoginTip.hpp"
#include <wx/event.h>
wxDEFINE_EVENT(EVT_MENU_HOVER_ENTER, wxCommandEvent);
wxDEFINE_EVENT(EVT_MENU_HOVER_LEAVE, wxCommandEvent);
#ifdef __WXMSW__
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

void ApplyWindowShadow(wxWindow* window) {
    HWND hwnd = (HWND)window->GetHWND();
    DWMNCRENDERINGPOLICY policy = DWMNCRP_ENABLED;
    DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));

    MARGINS margins = { 1, 1, 1, 1 }; // 阴影厚度
    DwmExtendFrameIntoClientArea(hwnd, &margins);
}
#endif
static bool ShouldDark(const wxColour& bgColor)
{
    int brightness = (bgColor.Red() * 299 + bgColor.Green() * 587 + bgColor.Blue() * 114) / 1000;
    return brightness > 50;
}

static wxColour GetTextColorBasedOnBackground(const wxColour& bgColor) {
	if (ShouldDark(bgColor)) {
		return *wxBLACK;
	}
	else {
		return *wxWHITE;
	}
}

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

END_EVENT_TABLE()

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

    // Create the child button
    int childButtonWidth = size.GetWidth() / 2;
    int childButtonHeight = size.GetHeight() * 5 / 6;
    int childButtonX = size.GetWidth() / 2;
    int childButtonY = (size.GetHeight() - childButtonHeight) / 2;

    m_child_button = new wxButton(this, wxID_ANY, "", wxPoint(childButtonX, childButtonY), wxSize(childButtonWidth, childButtonHeight));
    m_child_button->Bind(wxEVT_PAINT, &FilamentButton::OnChildButtonPaint, this);
	m_child_button->Bind(wxEVT_LEFT_DOWN, &FilamentButton::OnChildButtonClick, this);
    m_child_button->Bind(wxEVT_RIGHT_UP, [&](wxMouseEvent& event) {
        FilamentItem* parentItem = dynamic_cast<FilamentItem*>(GetParent());
        int           filament_item_index = -1;
        if (parentItem) {
            filament_item_index = parentItem->index();
        }
        auto    menu = new MaterialContextMenu(this, filament_item_index);
        wxPoint screenPos = ClientToScreen(event.GetPosition());
        menu->Position(screenPos, wxSize(0, 0));
        menu->Cus_Popup();
        event.Skip();
        });
    // Load the bitmap
    m_bitmap = create_scaled_bitmap("switch_cfs_tip", nullptr, 16);
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
	m_dark_img  = ScalableBitmap(this, dark_icon.ToStdString(), 4);
    m_light_img = ScalableBitmap(this, light_icon.ToStdString(), 4);
}

void FilamentButton::SetLabel(wxString lb)
{
	m_label = lb; 
}

wxString FilamentButton::getLabel() 
{ 
    return m_label;
}

void FilamentButton::mouseDown(wxMouseEvent& event)
{
	event.Skip();
	if (!HasCapture())
		CaptureMouse();
}

void FilamentButton::mouseReleased(wxMouseEvent& event)
{
	event.Skip();
	if (HasCapture())
		ReleaseMouse();

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
    wxSize size = GetSize();
    int childButtonWidth = size.GetWidth() / 2;
    int childButtonHeight = size.GetHeight() * 5 / 6;
    int childButtonX = size.GetWidth() / 2;
    int childButtonY = (size.GetHeight() - childButtonHeight) / 2;

    m_child_button->SetSize(wxSize(childButtonWidth, childButtonHeight));
    m_child_button->SetPosition(wxPoint(childButtonX, childButtonY));

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

	    // Draw black border
    dc.SetPen(wxPen(GetTextColorBasedOnBackground(m_back_color), 1));
    dc.SetBrush(*wxTRANSPARENT_BRUSH); // No fill
    dc.DrawRectangle(0, 0, size.GetWidth(), size.GetHeight());

	// Get the background color of m_child_button
    wxColour bgColour = m_child_button->GetBackgroundColour();

    // Left half background color
    wxRect leftRect(1, 1, size.GetWidth() / 2, size.GetHeight() - 2);
    if (m_bReseted)
        dc.SetBrush(wxBrush(m_resetedColour));
    else
        dc.SetBrush(wxBrush(m_back_color));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(leftRect);

    // Draw the label in the left half
    if (!m_sync_filament_label.IsEmpty()) {
        int textWidth, textHeight;
        dc.SetFont(Label::Body_12);
        dc.GetTextExtent(m_sync_filament_label, &textWidth, &textHeight);

        int textX = leftRect.GetX() + (leftRect.GetWidth() - textWidth) / 2;
        int textY = leftRect.GetY() + (leftRect.GetHeight() - textHeight) / 2;

        if (m_bReseted)
            dc.SetTextForeground(GetTextColorBasedOnBackground(m_resetedColour));
        else
            dc.SetTextForeground(GetTextColorBasedOnBackground(m_back_color)); // Black text color
        dc.DrawText(m_sync_filament_label, wxPoint(textX, textY));
    }

    // Right half with bitmap
    wxRect rightRect(size.GetWidth() / 2, 0, size.GetWidth() / 2, size.GetHeight());
    wxRect rightRect2(size.GetWidth() / 2, 1, size.GetWidth() / 2, size.GetHeight() - 2);
    if (m_bReseted)
        dc.SetBrush(wxBrush(m_resetedColour));
    else
        dc.SetBrush(wxBrush(m_back_color));
    dc.SetPen(*wxTRANSPARENT_PEN);

    // draw the rightRect2 because when add a new filament, the right haft background color would be grey
    dc.DrawRectangle(rightRect2);

    if (m_bitmap.IsOk()) {
        int imgWidth = rightRect.GetWidth();
        int imgHeight = rightRect.GetHeight();
        int imgX = rightRect.GetX();
        int imgY = rightRect.GetY();

        // // Scale the bitmap to fit the right half of the rectangle
        // wxImage image = m_bitmap.ConvertToImage();
        // image = image.Scale(imgWidth, imgHeight, wxIMAGE_QUALITY_HIGH);
        // wxBitmap scaledBitmap = wxBitmap(image);

        dc.DrawBitmap(m_bitmap, imgX, imgY, true);
    }
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
}

void FilamentButton::update_child_button_color(const wxColour& color)
{
	m_child_button->SetBackgroundColour(color);
	m_child_button->Refresh();
}
void FilamentButton::resetCFS(bool bCFS)
{
    if (bCFS) {
        m_sync_filament_label = "CFS";
        m_bReseted            = true;
    } else {
        m_bReseted = false;
    }
    m_child_button->Refresh();
}

void FilamentButton::doRender(wxDC& dc)
{
	wxSize size = GetSize();
	int states = m_state_handler.states();
	wxRect rc(0, 0, size.x, size.y);

	if ((FilamentButtonStateHandler::State) states == FilamentButtonStateHandler::State::Hover)
	{
        dc.SetPen(wxPen(GetTextColorBasedOnBackground(m_back_color), m_border_width));
	}
	else
	{
		dc.SetPen(wxPen(m_back_color, m_border_width));
	}

	dc.SetBrush(wxBrush(m_back_color));

	if (m_radius == 0) {
		dc.DrawRectangle(rc);
	}
	else {
		dc.DrawRoundedRectangle(rc, m_radius - m_border_width);
	}

	if (!m_label.IsEmpty()) {
        int width, height;
        wxFont basic_font = dc.GetFont();
    #ifdef TARGET_OS_MAC
         basic_font.SetPointSize(FromDIP(10));
    #else
        basic_font.SetPointSize(8);
    #endif
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

        dc.SetTextForeground(GetTextColorBasedOnBackground(m_back_color));
        dc.DrawText(m_label, wxPoint(x, y));
    }

	if (m_dark_img.bmp().IsOk() && m_light_img.bmp().IsOk()) {
        int x = size.GetWidth() - 10;
        int y = size.GetHeight() / 2 - 2;

		if (!m_label.IsEmpty())
		{
            int width, height;
            dc.GetTextExtent(m_label, &width, &height);

			int panelWidth, panelHeight;
            GetSize(&panelWidth, &panelHeight);

			x = (panelWidth + width) / 2;
		}

        dc.DrawBitmap(ShouldDark(m_back_color) ? m_dark_img.bmp() : m_light_img.bmp(),
                      wxPoint(x, y));
    }

	m_child_button->Show(m_sync_box_filament);
}

/*
* FilamentPopPanel
*/

 FilamentPopPanel::FilamentPopPanel(wxWindow* parent, int index)
	: PopupWindow(parent, wxBORDER_NONE | wxPU_CONTAINS_CONTROLS)
{
    Freeze();

	int em = 1;
    m_index = index;
	
	m_bg_color = wxColour(61, 223, 86);
	this->SetBackgroundColour(m_bg_color);

	this->SetSize(wxSize(400 * em, 28 * em));
    
  

	m_sizer_main = new wxBoxSizer(wxHORIZONTAL);
	{
        m_filamentCombox = new Slic3r::GUI::PlaterPresetComboBox(this, Slic3r::Preset::TYPE_FILAMENT);
        m_filamentCombox->GetDropDown().setDrapDownGap(0);
        m_filamentCombox->set_filament_idx(index);
        //m_filamentCombox->SetMaxSize(wxSize(FromDIP(200), -1));
        m_filamentCombox->update();
        m_filamentCombox->clr_picker->Hide();
        m_filamentCombox->Bind(wxEVT_RIGHT_UP, [&](wxMouseEvent& event) {
            auto    menu = new MaterialContextMenu(this, index);
            wxPoint screenPos = ClientToScreen(event.GetPosition());
            menu->Position(screenPos, wxSize(0, 0));
            menu->Cus_Popup();
            event.Skip();
            });

        m_filamentCombox->setSelectedItemCb([&](int selectedItem) -> void { 
            if (m_pFilamentItem != nullptr) {
                m_pFilamentItem->resetCFS(true);
                }
            });
        m_filamentCombox->Bind(wxEVT_COMBOBOX_DROPDOWN, [this](wxCommandEvent& e) { 
        #if __APPLE__
            this->Hide();
        #endif
		});
		// filament combox
        wxSizerItem* item = m_sizer_main->Add(m_filamentCombox, 1, wxUP | wxDOWN | wxRIGHT, 1);
        item->SetProportion(wxEXPAND);
        bool is_dark = Slic3r::GUI::wxGetApp().dark_mode();

		//
		{
            wxPanel* box = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
            box->SetBackgroundColour(wxColour(255, 255, 255));
            wxBoxSizer*sz = new wxBoxSizer(wxHORIZONTAL);
			box->SetSizer(sz);

            m_img_extruderTemp = new ScalableButton(box, wxID_ANY, is_dark ? "extruderTemp" : "extruderTemp_black", wxEmptyString, wxDefaultSize,
                                                                  wxDefaultPosition, wxBU_EXACTFIT | wxNO_BORDER, false, 12);
            m_img_extruderTemp->SetBackgroundColour(wxColour(255, 255, 255));
            sz->Add(m_img_extruderTemp, 0, wxLEFT | wxRIGHT, FromDIP(5));

            m_lb_extruderTemp = new Label(box, Label::Body_13, wxString::FromUTF8(""));
            //m_lb_extruderTemp->setLeftMargin(FromDIP(1));
            m_lb_extruderTemp->SetBackgroundColour(wxColour(255, 255, 255)); // 确保背景颜色与文本颜色不同
            m_lb_extruderTemp->SetForegroundColour(wxColour(0, 0, 0));       // 设置文本颜色为黑色
            m_lb_extruderTemp->SetMinSize(wxSize(FromDIP(30), -1));
            //m_lb_extruderTemp->Enable(false);

            Label* exTemp = new Label(box, Label::Body_13, wxString::FromUTF8("°C"));
            //exTemp->SetMinSize(wxSize(FromDIP(10), -1));

            sz->Add(m_lb_extruderTemp, 0, wxALIGN_CENTER_VERTICAL);
            sz->Add(exTemp, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, FromDIP(5));

            m_sizer_main->Add(box, 1, wxEXPAND | wxRIGHT | wxUP | wxDOWN, 1);

            m_lb_extruderTemp->Bind(wxEVT_TEXT_ENTER, [](wxCommandEvent& event) {
                wxString newText   = event.GetString();
                // 在这里处理文本内容修改的逻辑
                Tab*                        tab = wxGetApp().get_tab(Slic3r::Preset::TYPE_FILAMENT);
                Slic3r::DynamicPrintConfig* cfg = &Slic3r::GUI::wxGetApp().preset_bundle->project_config;
                // auto                        colors = static_cast<Slic3r::ConfigOptionStrings*>(cfg->option("filament_vendor")->clone());
                if (tab) {
                    PageShp strength_page = tab->get_page(L("Filament"));
                    if (strength_page) {
                        ConfigOptionsGroupShp optgroup = strength_page->get_optgroup(L("Print temperature"));
                        if (optgroup) {
                            optgroup->on_change_OG("nozzle_temperature", wxAtoi(newText));
                        }
                    }
                }
            });
		}
        
		//
		{
            wxPanel* box = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
            box->SetBackgroundColour(wxColour(255, 255, 255));
            wxBoxSizer* sz = new wxBoxSizer(wxHORIZONTAL);
            box->SetSizer(sz);

            m_img_bedTemp = new ScalableButton(box, wxID_ANY, is_dark ? "bedTemp" :"bedTemp_black", wxEmptyString, wxDefaultSize, wxDefaultPosition, wxBU_EXACTFIT | wxNO_BORDER, false, 12);
            m_img_bedTemp->SetBackgroundColour(wxColour(255, 255, 255));
            sz->Add(m_img_bedTemp, 0, wxLEFT | wxRIGHT, FromDIP(5));

            m_lb_bedTemp = new Label(box, Label::Body_13, wxString::FromUTF8(""));
            //m_lb_bedTemp->setLeftMargin(FromDIP(1));
            m_lb_bedTemp->SetBackgroundColour(wxColour(255, 255, 255));
            m_lb_bedTemp->SetForegroundColour(wxColour(0, 0, 0));
            m_lb_bedTemp->SetMinSize(wxSize(FromDIP(30), -1));

            Label* bedLabel = new Label(box, Label::Body_13, wxString::FromUTF8("°C"));
            //bedLabel->SetMinSize(wxSize(FromDIP(20), -1));

            sz->Add(m_lb_bedTemp, 0, wxALIGN_CENTER_VERTICAL);
            sz->Add(bedLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, FromDIP(5));

            m_sizer_main->Add(box, 1, wxEXPAND | wxUP | wxDOWN, 1);
            m_lb_bedTemp->Bind(wxEVT_TEXT_ENTER, [](wxCommandEvent& event) {
                wxString newText   = event.GetString();
                // 在这里处理文本内容修改的逻辑
                Tab*                        tab = wxGetApp().get_tab(Slic3r::Preset::TYPE_FILAMENT);
                Slic3r::DynamicPrintConfig* cfg    = &Slic3r::GUI::wxGetApp().preset_bundle->project_config;
                //auto                        colors = static_cast<Slic3r::ConfigOptionStrings*>(cfg->option("filament_vendor")->clone());
                if (tab) {
                    PageShp strength_page = tab->get_page(L("Filament"));
                    if (strength_page) {
                        ConfigOptionsGroupShp optgroup = strength_page->get_optgroup(L("Bed temperature"));
                        if (optgroup) { 
                            SidebarPrinter& bar      = wxGetApp().plater()->sidebar_printer();
                            Slic3r::BedType bed_type = bar.get_selected_bed_type();
                                    wxString        plateType = "";
                            if (Slic3r::BedType::btPTE == bed_type)
                                plateType = "textured_plate_temp";
                            else if (Slic3r::BedType::btDEF == bed_type)
                                plateType = "customized_plate_temp";
                            else if (Slic3r::BedType::btER == bed_type)
                                plateType = "epoxy_resin_plate_temp";
                            else
                                plateType = "hot_plate_temp";

                            optgroup->on_change_OG(plateType.ToStdString(), wxAtoi(newText));
                        }
                    }
                }
            });
		}
		
		//
		{
            m_edit_btn = new ScalableButton(this, wxID_ANY, is_dark ? "profile_editBtn_d" : "profile_editBtn", wxEmptyString, wxDefaultSize, wxDefaultPosition, wxBU_EXACTFIT | wxNO_BORDER, false, 12);
            wxSize sz = m_edit_btn->GetSize();
            sz.SetWidth(sz.GetWidth() + 25);
            sz.SetHeight(this->GetSize().GetHeight() - 2);
            m_edit_btn->SetMinSize(sz);
            m_edit_btn->SetBackgroundColour(wxColour(255, 255, 255));
            m_edit_btn->SetToolTip(_L("Click to edit preset"));
            m_edit_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& e) {
                Slic3r::GUI::wxGetApp().sidebar().set_edit_filament(-1);
                if (m_filamentCombox->switch_to_tab()) {
                    Slic3r::GUI::wxGetApp().sidebar().set_edit_filament(m_index);
                }
            });

            m_sizer_main->Add(m_edit_btn, wxSizerFlags().Border(wxUP | wxDOWN | wxRIGHT | wxLEFT, 1));
		}
	}
#if __APPLE__
    Bind(wxEVT_LEFT_DOWN, &FilamentPopPanel::on_left_down, this); 
#endif
    Slic3r::GUI::wxGetApp().UpdateDarkUIWin(this);
	SetSizer(m_sizer_main);
	Layout();
	Thaw();
}

void FilamentPopPanel::on_left_down(wxMouseEvent &evt)
{
    
    auto pos = ClientToScreen(evt.GetPosition());
    auto firstChildren = m_sizer_main->GetChildren();
    for(wxSizerItem* firstItem: firstChildren)
    {   
        wxWindow* item = firstItem->GetWindow();
        auto p_rect = item->ClientToScreen(wxPoint(0, 0));
        if (pos.x > p_rect.x && pos.y > p_rect.y && pos.x < (p_rect.x + item->GetSize().x) && pos.y < (p_rect.y + item->GetSize().y)) {
            wxMouseEvent event = evt;
            auto new_pos = pos - p_rect;
		    event.SetEventObject(item);
            event.SetPosition(new_pos);
            item->GetEventHandler()->ProcessEvent(event);
            
        }
    }

}

FilamentPopPanel::~FilamentPopPanel() {}



void FilamentPopPanel::Popup(wxPoint position /*= wxDefaultPosition*/)
{
	SetPosition(position);

	PopupWindow::Popup();

}

void FilamentPopPanel::Dismiss()
{
    auto focus_window = this->GetParent()->HasFocus();
    if (!focus_window)
        PopupWindow::Dismiss();

	wxCommandEvent e(EVT_DISMISS);
    GetEventHandler()->ProcessEvent(e);
}

void FilamentPopPanel::sys_color_changed()
{
	m_filamentCombox->sys_color_changed();

	bool is_dark = Slic3r::GUI::wxGetApp().dark_mode();
    m_img_extruderTemp->SetBitmap_(is_dark ? "extruderTemp" : "extruderTemp_black");
    m_img_bedTemp->SetBitmap_(is_dark ? "bedTemp" : "bedTemp_black");
    m_edit_btn->SetBitmap_(is_dark ? "profile_editBtn_d" : "profile_editBtn");
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
    m_preset_bundle->filaments.find_preset(m_preset_bundle->filament_presets[m_data.index])->get_filament_type(filament_type);

    std::string filament_color = m_preset_bundle->project_config.opt_string("filament_colour", (unsigned int) m_data.index);
    m_bk_color                 = wxColour(filament_color);

    m_checked_border_color = wxColour(61, 223, 86);

	m_small_state = data.small_state;
	wxSize sz(size);
	if (m_small_state)
		sz.SetWidth(sz.GetWidth() / 2);

	wxPanel::Create(parent, wxID_ANY, wxDefaultPosition, sz, 0);

	wxSize btn_size;
	btn_size.SetHeight(sz.GetHeight() / 2 - this->m_radius*0.5);
	btn_size.SetWidth(sz.GetWidth() - this->m_radius - this->m_border_width*0.5);	

	m_sizer = new wxBoxSizer(wxVERTICAL);


	{//color btn
		m_btn_color = new FilamentButton(this, wxString(std::to_string(data.index + 1)), wxPoint(this->m_radius * 0.5 + m_border_width * 0.5, this->m_radius * 0.5+ this->m_border_width * 0.5), btn_size);
		m_btn_color->SetCornerRadius(this->m_radius);
		m_btn_color->SetColor(m_bk_color);
        //InitContextMenu();
        m_btn_color->Bind(wxEVT_RIGHT_UP, [&](wxMouseEvent& event) {
            auto    menu      = new MaterialContextMenu(this, m_data.index);
            wxPoint screenPos = ClientToScreen(event.GetPosition());
            menu->Position(screenPos, wxSize(0, 0));
            menu->Cus_Popup();
            event.Skip();

        });
        
 
		m_btn_color->Bind(wxEVT_BUTTON, [&](wxEvent& e) {
			//Refresh the status of other items
			wxCommandEvent event(wxEVT_BUTTON, GetId());
			event.SetEventObject(this);
			GetEventHandler()->ProcessEvent(event);

			m_checked_state = true;
			this->Refresh();

			wxColourData m_clrData;
            m_clrData.SetColour(m_bk_color);
            m_clrData.SetChooseFull(true);
            m_clrData.SetChooseAlpha(false);

			std::vector<std::string> colors = Slic3r::GUI::wxGetApp().app_config->get_custom_color_from_config();
            for (int i = 0; i < colors.size(); i++) {
                m_clrData.SetCustomColour(i, string_to_wxColor(colors[i]));
            }

			wxColourDialog dialog(Slic3r::GUI::wxGetApp().plater(), &m_clrData);
            dialog.Center();
            dialog.SetTitle(_L("Please choose the filament colour"));
			if (dialog.ShowModal() == (int)wxID_OK)
			{
				m_clrData = dialog.GetColourData();
                if (colors.size() != CUSTOM_COLOR_COUNT) {
                    colors.resize(CUSTOM_COLOR_COUNT);
                }
                for (int i = 0; i < CUSTOM_COLOR_COUNT; i++) {
                    colors[i] = color_to_string(m_clrData.GetCustomColour(i));
                }

                Slic3r::GUI::wxGetApp().app_config->save_custom_color_to_config(colors);

				// get current color
                Slic3r::DynamicPrintConfig* cfg    = &Slic3r::GUI::wxGetApp().preset_bundle->project_config;
                auto colors = static_cast<Slic3r::ConfigOptionStrings*>(cfg->option("filament_colour")->clone());
                wxColour clr(colors->values[m_data.index]);
                if (!clr.IsOk())
                    clr = wxColour(0, 0, 0); // Don't set alfa to transparence

                colors->values[m_data.index] = m_clrData.GetColour().GetAsString(wxC2S_HTML_SYNTAX).ToStdString();
                Slic3r::DynamicPrintConfig cfg_new = *cfg;
                cfg_new.set_key_value("filament_colour", colors);

				// wxGetApp().get_tab(Preset::TYPE_PRINTER)->load_config(cfg_new);
                cfg->apply(cfg_new);
                Slic3r::GUI::wxGetApp().plater()->update_project_dirty_from_presets();
                Slic3r::GUI::wxGetApp().preset_bundle->export_selections(*Slic3r::GUI::wxGetApp().app_config);
                //update();
                Slic3r::GUI::wxGetApp().plater()->on_config_change(cfg_new);

				m_bk_color = m_clrData.GetColour();
				m_btn_color->SetColor(m_bk_color);
				m_btn_param_list->SetColor(m_bk_color);

				wxCommandEvent* evt = new wxCommandEvent(Slic3r::GUI::EVT_FILAMENT_COLOR_CHANGED);
                evt->SetInt(m_data.index);
                wxQueueEvent(Slic3r::GUI::wxGetApp().plater(), evt);
			}

			this->Refresh();
			});
	}

	{//param btn
        m_btn_param_list = new FilamentButton(this, wxString(filament_type), wxPoint(this->m_radius * 0.5 + m_border_width * 0.5, this->m_radius + this->m_border_width * 0.5 + btn_size.GetHeight()), btn_size);
		m_btn_param_list->SetCornerRadius(this->m_radius);
		m_btn_param_list->SetColor(m_bk_color);
        m_btn_param_list->SetIcon("downBtn_black", "downBtn_white");
        
        m_btn_param_list->Bind(wxEVT_RIGHT_UP, [&](wxMouseEvent& event) {
            auto    menu = new MaterialContextMenu(this, m_data.index);
            wxPoint screenPos = ClientToScreen(event.GetPosition());
            menu->Position(screenPos, wxSize(0, 0));
            menu->Cus_Popup();
            event.Skip();
            });

		m_btn_param_list->Bind(wxEVT_BUTTON, [&](wxEvent& e) {
			wxPoint ppos = this->GetParent()->ClientToScreen(wxPoint(0, 0));
			wxPoint pos = this->ClientToScreen(wxPoint(0, 0));
			pos.y += this->GetRect().height;
			pos.x = ppos.x + 1;

			wxSize sz = m_popPanel->GetSize();
			wxSize psz = this->GetParent()->GetSize();
			sz.SetWidth(psz.GetWidth() - 2);
		 
			m_popPanel->SetSize(sz);
 			m_popPanel->Layout();
            m_popPanel->Dismiss();
			m_popPanel->SetPosition(pos);
			m_popPanel->Popup();
            

			wxCommandEvent event(wxEVT_BUTTON, GetId());
			event.SetEventObject(this);
			GetEventHandler()->ProcessEvent(event);

			m_btn_param_list->SetIcon("upBtn_black", "upBtn_white");
            m_btn_param_list->Refresh();

			m_checked_state = true;
			this->Refresh();
			});
	}

	this->SetSizer(m_sizer);
	m_sizer->Layout();
	//m_sizer->Fit(this);

	m_popPanel = new FilamentPopPanel(this, data.index);
    m_popPanel->setFilamentItem(this);
    m_popPanel->Bind(EVT_DISMISS, [this](auto&)
		{
			m_btn_param_list->SetIcon("downBtn_black", "downBtn_white");
			m_btn_param_list->Refresh();
		});

	//update filament type.
	m_popPanel->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent& e) { 
		Slic3r::GUI::wxGetApp().sidebar().GetEventHandler()->ProcessEvent(e);
		});
        
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

void FilamentItem::set_filament_selection(const wxString& filament_name)
{
    if (!filament_name.IsEmpty())
    {
        if(m_popPanel->m_filamentCombox->SetStringSelection(filament_name))
		{
			int evt_selection = m_popPanel->m_filamentCombox->GetSelection();
            wxCommandEvent* evt = new wxCommandEvent(wxEVT_COMBOBOX, m_popPanel->m_filamentCombox->GetId());
            evt->SetEventObject(m_popPanel->m_filamentCombox);
			evt->SetInt(evt_selection);
            wxQueueEvent(&(Slic3r::GUI::wxGetApp().plater()->sidebar()), evt);
        }
    }
}

void FilamentItem::update()
{
    if(m_preset_bundle->filament_presets.size() <= m_data.index)
        return;

    auto filament_color = m_preset_bundle->project_config.opt_string("filament_colour", (unsigned int)m_data.index);
    if (filament_color == "\"\"")
        filament_color = "#000000";
    Slic3r::DynamicPrintConfig* cfg    = &Slic3r::GUI::wxGetApp().preset_bundle->project_config;
    auto                        colors = static_cast<Slic3r::ConfigOptionStrings*>(cfg->option("filament_colour")->clone());
    colors->values[m_data.index]       = filament_color;
    Slic3r::DynamicPrintConfig cfg_new = *cfg;
    cfg_new.set_key_value("filament_colour", colors);
    cfg->apply(cfg_new);
    Slic3r::GUI::wxGetApp().plater()->update_project_dirty_from_presets();
    Slic3r::GUI::wxGetApp().preset_bundle->export_selections(*Slic3r::GUI::wxGetApp().app_config);
    Slic3r::GUI::wxGetApp().plater()->on_config_change(cfg_new);

    m_bk_color = wxColor(filament_color);
    m_btn_color->SetColor(m_bk_color);
    m_btn_param_list->SetColor(m_bk_color);

	m_popPanel->m_filamentCombox->update(); 

	std::string filament_type;
	Slic3r::Preset* preset = m_preset_bundle->filaments.find_preset(m_preset_bundle->filament_presets[m_data.index]);
    if (preset)
        preset->get_filament_type(filament_type);
    else
        return;

    wxString current_selection = m_popPanel->m_filamentCombox->GetStringSelection();

	// Get the button width
    int btn_width = m_btn_param_list->GetSize().GetWidth();
    wxClientDC dc(m_btn_param_list);
    dc.SetFont(m_btn_param_list->GetFont());

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

	auto filament_config = m_preset_bundle->filaments.find_preset(m_preset_bundle->filament_presets[m_data.index])->config;
    const Slic3r::ConfigOptionInts* nozzle_temp_opt = filament_config.option<Slic3r::ConfigOptionInts>("nozzle_temperature");
	if (nullptr != nozzle_temp_opt)
	{
        int nozzle_temperature = nozzle_temp_opt->get_at(0);
        m_popPanel->m_lb_extruderTemp->SetLabel(wxString(std::to_string(nozzle_temperature)));
	}

	const Slic3r::ConfigOptionInts* hot_plate_temp = filament_config.option<Slic3r::ConfigOptionInts>("hot_plate_temp");
	if (nullptr != hot_plate_temp)
	{
        SidebarPrinter&          bar               = wxGetApp().plater()->sidebar_printer();
        Slic3r::BedType bed_type = bar.get_selected_bed_type();
        if(Slic3r::BedType::btPTE == bed_type)
            hot_plate_temp = filament_config.option<Slic3r::ConfigOptionInts>("textured_plate_temp");
        else if(Slic3r::BedType::btDEF == bed_type)
            hot_plate_temp = filament_config.option<Slic3r::ConfigOptionInts>("customized_plate_temp");
        else if(Slic3r::BedType::btER == bed_type)
            hot_plate_temp = filament_config.option<Slic3r::ConfigOptionInts>("epoxy_resin_plate_temp");
        else
            hot_plate_temp = filament_config.option<Slic3r::ConfigOptionInts>("hot_plate_temp");
        int plate_temp = hot_plate_temp->get_at(0);
        m_popPanel->m_lb_bedTemp->SetLabel(wxString(std::to_string(plate_temp)));
	}
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
    btn_size.SetHeight(sz.GetHeight() / 2 - this->m_radius * 0.5);
    btn_size.SetWidth(sz.GetWidth() - this->m_radius - this->m_border_width * 0.5);

    m_btn_color->SetSize(btn_size);
    m_btn_color->update_child_button_size();

    m_btn_param_list->SetSize(btn_size);
    wxPoint param_list_pos(this->m_radius * 0.5 + m_border_width * 0.5, this->m_radius + this->m_border_width * 0.5 + btn_size.GetHeight());
    m_btn_param_list->SetPosition(param_list_pos);
}

void FilamentItem::msw_rescale() 
{
	m_popPanel->m_filamentCombox->msw_rescale();
    wxSize newSize = wxSize(FromDIP(FILAMENT_BTN_WIDTH), FromDIP(FILAMENT_BTN_HEIGHT));
    SetSize(newSize);
    update_button_size();
}

void FilamentItem::paintEvent(wxPaintEvent& evt)
{
	wxPaintDC dc(this);
	wxSize size = this->GetSize();
	if (1) {
		wxRect rc(0, 0, size.x, size.y);

		dc.SetPen(wxPen(m_checked_state ? m_checked_border_color : m_bk_color, m_border_width));
		dc.SetBrush(wxBrush(m_bk_color));
        
        if (!Slic3r::GUI::wxGetApp().dark_mode() && m_bk_color == wxColour("#FFFFFF"))
            dc.SetPen(wxPen(wxColour("#D0D4DE"), 1, wxPENSTYLE_SOLID));

		if (m_radius == 0) {
			dc.DrawRectangle(rc);
		}
		else {
			dc.DrawRoundedRectangle(rc, m_radius - m_border_width);
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
    /*
* FilamentPanel
*/

FilamentPanel::FilamentPanel(wxWindow* parent,
	wxWindowID      id,
	const wxPoint& pos,
	const wxSize& size, long style)
	: wxPanel(parent, id, pos, size, style)
{
	m_sizer = new wxWrapSizer(wxHORIZONTAL);
	m_box_sizer = new wxBoxSizer(wxVERTICAL);
	this->SetSizer(m_box_sizer);
    m_box_sizer->Add(m_sizer, 0, wxLEFT | wxRIGHT, FromDIP(6));
    m_box_sizer->AddSpacer(FromDIP(8));
#ifdef __APPLE__
	m_box_sizer->AddSpacer(FromDIP(20));
#endif
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
	FilamentItem* filament = new FilamentItem(this, data, wxSize(FromDIP(110), FromDIP(41)));
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
	m_sizer->Layout();
	this->GetParent()->Layout();

    std::string res = wxGetApp().app_config->get("is_currentMachine_Colors");
    bool isColors = res == "1";
    update_box_filament_sync_state(isColors);
	return true;
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

std::string w2s(wxString sSrc)
{
    return std::string(sSrc.mb_str());
}

bool compareStrings(std::string str1, std::string str2)
{
    auto trim = [](std::string s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), s.end());
        return s;
    };

    auto toLower = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) {
            return std::tolower(ch);
        });
        return s;
    };

    std::string str1_cleaned = toLower(trim(str1));
    std::string str2_cleaned = toLower(trim(str2));


    return str1_cleaned == str2_cleaned;
}

std::string getPrefix(const std::string& str)
{
    size_t atPos = str.find('@');
    if (atPos == std::string::npos)
    {
        return str;
    }
    return str.substr(0, atPos);
}

int FilamentPanel::LoadFilamentProfile(bool isCxVedor)
{
    bool bbl_bundle_rsrc = false;
    json empty;
    Slic3r::ProfileFamilyLoader::get_instance()->request_and_wait();
    Slic3r::ProfileFamilyLoader::get_instance()->get_result(m_FilamentProfileJson, empty, bbl_bundle_rsrc);

    const auto enabled_filaments = Slic3r::GUI::wxGetApp().app_config->has_section(Slic3r::AppConfig::SECTION_FILAMENTS) 
                                   ? Slic3r::GUI::wxGetApp().app_config->get_section(Slic3r::AppConfig::SECTION_FILAMENTS) 
                                   : std::map<std::string, std::string>();

    bool isSelect = false;
    for (auto it = m_FilamentProfileJson["filament"].begin(); it != m_FilamentProfileJson["filament"].end(); ++it)
    {
        std::string filament_name = it.key();
        if (enabled_filaments.find(filament_name) != enabled_filaments.end())
        {
            m_FilamentProfileJson["filament"][filament_name]["selected"] = 1;
            isSelect = true;
        }
    }

    //wxString strAll = m_FilamentProfileJson.dump(-1, ' ', false, json::error_handler_t::ignore);

    if((!isSelect) || (!enabled_filaments.size()))
    {
        return 0;
    }

    return 1;
}

void FilamentPanel::SetFilamentProfile(std::vector<std::pair<int, DM::Material>>& validMaterials)
{

    std::map<std::string,std::string> section_new;

    for (auto it = m_FilamentProfileJson["filament"].begin(); it != m_FilamentProfileJson["filament"].end(); ++it)
    {
        string sJsonVendor = it.value()["vendor"];
        string sJsonType = it.value()["type"];
        string sJsonName = it.value()["name"];

        if (it.value()["selected"] == 1)
        {
            section_new[it.key()] = "true";
            continue;
        }

        for (int i = 0; i < validMaterials.size(); i++)
        {
            string sName = validMaterials[i].second.name;
            string sVendor = validMaterials[i].second.vendor;
            string sType = validMaterials[i].second.type;
            if (compareStrings(sVendor, sJsonVendor) && compareStrings(sType, sJsonType) && compareStrings(sName, getPrefix(sJsonName)))
            {
                it.value()["selected"] = 1;
                section_new[it.key()] = "true";
            }
        }
    }

    if(section_new.empty())
    {
        return ;
    }

    Slic3r::AppConfig appconfig;
    appconfig.set_section(Slic3r::AppConfig::SECTION_FILAMENTS,section_new);

    for(auto &preset : Slic3r::GUI::wxGetApp().preset_bundle->filaments)
    {
        preset.set_visible_from_appconfig(appconfig);
    }

    Slic3r::GUI::wxGetApp().app_config->set_section(Slic3r::AppConfig::SECTION_FILAMENTS,section_new);
    Slic3r::GUI::wxGetApp().app_config->save();
}

void FilamentPanel::on_auto_mapping_filament(const DM::Device& deviceData)
{
    bool isCfsMini = false;
    // 计算 materialBoxes 数组中 box_type == 0 的 Material 项，并且 Material 里 color 的值不为空的项
    std::vector<std::pair<int, DM::Material>> validMaterials;
    for (const auto materialBox : deviceData.materialBoxes)
    {
        if (materialBox.box_type == 0 || materialBox.box_type == 2)
        {
            for (const auto& material : materialBox.materials)
            {
                if (!material.color.empty())
                {
                    validMaterials.emplace_back(materialBox.box_id, material);
                }
            }
            if (materialBox.box_type == 2) {
                isCfsMini = true;
            }
        }
    }

	if(validMaterials.size() == 0)
		return;

    //参照添加耗材的逻辑。
    //1.打开json文件，获取所有选中的耗材。
    //2.遍历选中的耗材，查看是否有新增的。
    //3.有-更改内存，写入conf中。保存。
    //4.更新presetBundle,更新PlaterPresetComboBox
    int iNum = LoadFilamentProfile(Slic3r::GUI::wxGetApp().preset_bundle->is_cx_vendor());
    if (iNum)
    {
        SetFilamentProfile(validMaterials);
    }

	if(m_vt_filament.size() != validMaterials.size())
	{
		bool need_more_filaments = false;
		if(m_vt_filament.size() < validMaterials.size())
		{
			need_more_filaments = true;
		}

        size_t filament_count = validMaterials.size();
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
        
        Slic3r::GUI::wxGetApp().plater()->on_filaments_change(filament_count);
        Slic3r::GUI::wxGetApp().get_tab(Slic3r::Preset::TYPE_PRINT)->update();
        Slic3r::GUI::wxGetApp().preset_bundle->export_selections(*Slic3r::GUI::wxGetApp().app_config);

		if(need_more_filaments)
		{
			Slic3r::GUI::wxGetApp().plater()->sidebar().auto_calc_flushing_volumes(filament_count - 1);
		}
    }
    else
    {
        if(iNum)
        {
            for (auto& item : m_vt_filament)
            {
                item->update();
            }
        }
    }

    assert(m_vt_filament.size() == validMaterials.size());

	// sychronize normal multi-color box first, and then extra box
	int normalIdx = 0;  
    LoginTip::getInstance().resetHasSkipToLogin();
	for(int i = 0; i < validMaterials.size(); i++)
	{
        auto& item = m_vt_filament[normalIdx];
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

        item->update_bk_color(new_filament_color);
        item->set_filament_selection(new_filament_name);
        item->update_box_sync_state(true, material_sync_label);
        item->resetCFS(false);
        if (material_sync_label.empty()) {
            item->resetCFS(true);
        }

        normalIdx += 1;
    }

    m_sizer->Layout();

	// trigger a repaint event to fix the display issue after sychronizing
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
        
        Slic3r::GUI::wxGetApp().plater()->on_filaments_change(filament_count);
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
	if (-1 == index)
	{
        for (auto& item : m_vt_filament) {
            item->update();
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
        
        for (int i = -1; i < 4; i++) {

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
	m_filament_type_label = material_info.type;
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
        if(m_sync)
        {
            m_material_index_info = wxString::Format("%d%c", m_box_id, index_char);
            if (box_type == 2) {
                m_material_index_info = "CFS";
            }
        }
        else 
        {
            m_material_index_info = "/";
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

MaterialSubMenuItem::MaterialSubMenuItem(wxWindow* parent, const wxString& label, const wxColour& color,const int num)
    : wxWindow(parent, wxID_ANY), m_label(label), m_color(color), m_num(num)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, &MaterialSubMenuItem::OnPaint, this);
    
    
#ifdef __APPLE__
    Bind(wxEVT_LEFT_DOWN, &MaterialSubMenuItem::OnMouseRelease, this);
#else
    Bind(wxEVT_LEFT_UP, &MaterialSubMenuItem::OnMouseRelease, this);
    Bind(wxEVT_LEFT_DOWN, &MaterialSubMenuItem::OnMousePressed, this);
#endif
    Bind(wxEVT_ENTER_WINDOW, &MaterialSubMenuItem::OnMouseEnter, this);
    Bind(wxEVT_LEAVE_WINDOW, &MaterialSubMenuItem::OnMouseLeave, this);
}

void MaterialSubMenuItem::OnPaint(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();
    // 绘制完整项背景
    bool is_dark = Slic3r::GUI::wxGetApp().dark_mode();
    wxColour penColor = is_dark ? wxColour("#313131") : wxColour("#FFFFFF");
    wxColour bgColor = is_dark ? wxColour("#313131") : wxColour("#FFFFFF");
    dc.SetPen(wxPen(penColor,0));
    // 绘制边框（hover 或点击时）
    if (m_clicked) {
        dc.SetPen(wxPen(wxColour(21, 192, 89), 1)); // 边框颜色
        dc.SetBrush(wxBrush(is_dark ? wxColour("#1FCA63") :  wxColour(21, 192, 89))); // 点击时的背景色
        dc.DrawRoundedRectangle(GetClientRect(), 3); // 边角为 5 的边框
    } else if (m_hovered) {
        dc.SetPen(wxPen(wxColour(21, 192, 89), 1)); // 边框颜色
        dc.SetBrush(wxBrush(is_dark ? wxColour("#2E4838") : wxColour("#DCF6E6"))); // hover 时的背景色 = (21, 192, 89,0.15)
        dc.DrawRoundedRectangle(GetClientRect(), 3); // 边角为 5 的边框
    } else {
        dc.SetBrush(wxBrush(bgColor)); // 默认背景色
        dc.DrawRectangle(GetClientRect());
    }
    // 绘制左侧标识块（无边框，垂直居中）
    wxRect blockRect(5, (GetClientSize().GetHeight() - 16) / 2, 24, 16); // 垂直居中
    dc.SetBrush(wxBrush(m_color)); // 使用原始颜色配置
    
    dc.SetPen(*wxTRANSPARENT_PEN);
    if (m_clicked)
        dc.SetPen(wxPen(wxColour(255,255,255), 1));
    dc.DrawRoundedRectangle(blockRect, 2);

    // 绘制编号文字（白色粗体，居中显示）
    dc.SetTextForeground(GetTextColorBasedOnBackground(m_color));
    dc.SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    int textWidth, textHeight;
    dc.GetTextExtent(wxString::Format("%02d", m_num + 1), &textWidth, &textHeight);
    int textX = blockRect.GetX() + (blockRect.GetWidth() - textWidth) / 2;
    int textY = blockRect.GetY() + (blockRect.GetHeight() - textHeight) / 2;
    dc.DrawText(wxString::Format("%02d", m_num + 1), textX, textY);

    // 绘制耗材名称（黑色文字）
    dc.SetTextForeground(is_dark  ? *wxWHITE : *wxBLACK);
    int textStartX = blockRect.GetX() + blockRect.GetWidth() + 5; // 标识块右边缘 + 5
    dc.DrawText(m_label, textStartX, (GetClientSize().GetHeight() - dc.GetTextExtent(m_label).GetHeight()) / 2);
}

void MaterialSubMenuItem::OnMouseRelease(wxMouseEvent&)
{
    m_hovered = false;
    m_clicked = false;
    Slic3r::GUI::wxGetApp().plater()->sidebar().delete_filament(m_parentindex,m_num);
    PopupWindowManager::Get().CloseAll();
}
void MaterialSubMenuItem::OnMouseEnter(wxMouseEvent&)
{
    m_hovered = true;
    m_clicked = false;
    this->SetTransparent(40);
    Refresh();
}
void MaterialSubMenuItem::OnMousePressed(wxMouseEvent&)
{
    m_hovered = false;
    m_clicked = true;
    this->SetTransparent(255);
    Refresh();
}
  
void MaterialSubMenuItem::OnMouseLeave(wxMouseEvent&)
{
    m_hovered = false;
    m_clicked = false;
    this->SetTransparent(255);
    Refresh();
}

HoverButton::HoverButton(wxWindow* parent,
    wxWindowID      id,
    const wxString& label,
    const wxPoint& pos,
    const wxSize& size,
    const int& type)
    : wxButton(parent, id, label, pos, size, wxBORDER_NONE),
    m_type(type)
{
    bool is_dark = Slic3r::GUI::wxGetApp().dark_mode();
    m_baseColor = is_dark  ? wxColour("#313131") : * wxWHITE;
    m_pressedColor = is_dark ? wxColour("#1FCA63") :  wxColour("#DCF6E6");
    SetBackgroundColour(m_baseColor);
    SetForegroundColour(is_dark ? *wxWHITE : wxColour("#30373D"));
    BindEvents();
}

void HoverButton::SetBaseColors(const wxColour& normal, const wxColour& pressed)
{
    m_baseColor = normal;
    m_pressedColor = pressed;
    SetBackgroundColour(normal);
}

void HoverButton::SetBitMap_Cus(wxBitmap bit1, wxBitmap bit2)
{
    bitmap = bit1;
    bitmap_hover = bit2;
}
void HoverButton::SetExpendStates(bool expend)
{
    m_isExpend = expend;
    Refresh();
}

void HoverButton::BindEvents()
{
    Bind(wxEVT_ENTER_WINDOW, &HoverButton::OnEnter, this);
    Bind(wxEVT_LEAVE_WINDOW, &HoverButton::OnLeave, this);
    Bind(wxEVT_PAINT, &HoverButton::OnPaint, this);
}

void HoverButton::OnLeftDown(wxMouseEvent& e)
{
    SetBackgroundColour(m_pressedColor);
    Refresh();
    e.Skip();
}

void HoverButton::OnLeftUp(wxMouseEvent& e)
{
    SetBackgroundColour(m_baseColor);
    Refresh();
    e.Skip();
}
void HoverButton::OnEnter(wxMouseEvent& e)
{
    isHover = true;
    SetBackgroundColour(m_pressedColor);
    Refresh();
    
    if (m_type == 1 && !m_isExpend)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        wxPostEvent(this, wxCommandEvent(EVT_MENU_HOVER_ENTER));
    }
    else if (m_type == 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        m_isExpend = false;
        wxPostEvent(this, wxCommandEvent(EVT_MENU_HOVER_LEAVE));
    }
    e.Skip();
}
void HoverButton::OnLeave(wxMouseEvent& e)
{
    isHover = false;
    SetBackgroundColour(m_baseColor);
    Refresh();
    e.Skip();
}

void HoverButton::OnPaint(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();
    bool is_dark = Slic3r::GUI::wxGetApp().dark_mode();
    wxColour penColor = is_dark ? wxColour("#313131") : wxColour("#FFFFFF");
    //wxColour bgColor = is_dark ? wxColour("#313131") : wxColour("#FFFFFF");
    dc.SetPen(wxPen(penColor, 0));
    wxSize size = GetSize();
    wxCoord iconWidth = bitmap.IsOk() ? (bitmap.GetScaledWidth()) : 0;
    wxColour bgColor = this->GetBackgroundColour();
    // 绘制边框（hover 或点击时）
    if (isHover || m_isExpend) {
        dc.SetPen(wxPen(is_dark ? wxColour("#1FCA63") : wxColour(21, 192, 89), 1)); // 边框颜色
        dc.SetBrush(wxBrush(is_dark ? wxColour("#2E4838") : wxColour("#DCF6E6"))); // hover 时的背景色 = (21, 192, 89,0.15)
        SetTransparent(0.15 * 255);
        dc.DrawRoundedRectangle(GetClientRect(), 3); // 边角为 5 的边框
    }
    else {
        //dc.SetPen(wxPen(wxColour(21, 192, 89), 1)); // 边框颜色
        dc.SetBrush(wxBrush(bgColor)); // 默认背景色
        dc.DrawRectangle(GetClientRect());
    }

    wxString label = GetLabel();
    // 计算内容区域
    wxCoord textWidth, textHeight;
    dc.GetTextExtent(label, &textWidth, &textHeight);
    const wxCoord spacing = (8); // 图标文字间距

    // 总内容宽度
    const wxCoord totalContentWidth = textWidth + spacing + iconWidth;

    // 起始绘制位置（水平居中）
    wxCoord       startX = (size.x - totalContentWidth) / 2;
    const wxCoord startY = (size.y - textHeight) / 2;

    // 绘制文字
    dc.SetTextForeground(IsEnabled() ? GetForegroundColour() : wxColour("#C3C7CD"));
    dc.SetFont(GetFont());
    dc.DrawText(label, startX, startY);

    // 绘制右侧图标
    if (bitmap.IsOk()) {
        const wxCoord iconX = startX + textWidth + spacing;
        const wxCoord iconY = (size.y - bitmap.GetScaledHeight()) / 2;
        dc.DrawBitmap((isHover || m_isExpend) ? bitmap_hover : bitmap, iconX, iconY, true);
    }
}


MaterialSubMenu::MaterialSubMenu(wxWindow* parent, int index) : ManagedPopupWindow(parent), m_index(index)
{
    m_menuPop = dynamic_cast<ManagedPopupWindow*> (parent);
}
void MaterialSubMenu::init()
{
    //SetBackgroundColour(*wxWHITE);
    //SetBackgroundColour(*wxBLUE);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    auto _filamentPanel = dynamic_cast<FilamentPanel*>(wxGetApp().mainframe->plater()->sidebar().filament_panel());
    std::vector<FilamentItem*> items          = _filamentPanel->get_filament_items();

    //wxColour colors[] = {wxColour(227, 62, 62), wxColour(21, 64, 192), wxColour(21, 177, 192), wxColour(146, 21, 192), 
    //    wxColour(227, 62, 62), wxColour(21, 64, 192), wxColour(21, 177, 192), wxColour(78, 89, 105)};

    for (int i = 0; i < items.size(); ++i) {
        if (m_index == i) {
            continue; // Skip the current item
        }
        auto     item_data  = items[i];
        wxString label = item_data->name(); //wxString::Format("0%d %s", i + 1, item_data->name());
        auto     item       = new MaterialSubMenuItem(this, label, item_data->color(), i);
		item->setParentIndex(m_index);
        item->SetMinSize(wxSize(FromDIP(150), FromDIP(32)));
        sizer->Add(item, 1, wxEXPAND | wxALL, FromDIP(4));
    }
    SetSizerAndFit(sizer);
}

MaterialContextMenu::MaterialContextMenu(wxWindow* parent, int index) : ManagedPopupWindow(parent), m_index(index)
{
    //SetBackgroundColour(*wxRED);
    // 顶部按钮
    wxBoxSizer* btnSizer = new wxBoxSizer(wxVERTICAL);

    auto _filamentPanel = dynamic_cast<FilamentPanel*> (wxGetApp().mainframe -> plater()->sidebar().filament_panel());

    auto delBtn = new HoverButton(this, wxID_ANY, _L("Delete"),wxDefaultPosition, wxSize(FromDIP(150), FromDIP(32)),0);
    btnSizer->Add(delBtn, 1, wxALL, FromDIP(4));
    // 合并按钮（带箭头）
    wxBitmap mergeBitmap = create_scaled_bitmap("material_menu_down", this, FromDIP(20));
    wxBitmap mergeBitmap_hover = create_scaled_bitmap("material_menu_down_hover", this, FromDIP(20));
    m_mergeBtn = new HoverButton(this, wxID_ANY, _L("Merge with"), wxDefaultPosition, wxSize(FromDIP(150), FromDIP(32)),1);
    m_mergeBtn->SetBitMap_Cus(mergeBitmap, mergeBitmap_hover);
    btnSizer->Add(m_mergeBtn, 1, wxALL, FromDIP(4));
    SetSizerAndFit(btnSizer);

    if (!_filamentPanel->can_delete()) {
        delBtn->Disable();
    }
    if (_filamentPanel->get_filament_items().size() <= 1) {
        m_mergeBtn->Disable();
    }
        
    delBtn->Bind(wxEVT_BUTTON, &MaterialContextMenu::OnDelete, this);
    Bind(EVT_MENU_HOVER_ENTER, [this](auto& e) {
        OnShowSubmenu(e);
        });
   
    Bind(EVT_MENU_HOVER_LEAVE, [this](auto& e) {
        if (m_isExpended)
            PopupWindowManager::Get().CloseLast();
        m_isExpended = false;
        m_mergeBtn->SetExpendStates(false);
        });
}

void MaterialContextMenu::OnShowSubmenu(wxCommandEvent&e)
{
    if (m_isExpended)
    {
        return;
    }
    m_isExpended = true;
    m_mergeBtn->SetExpendStates(true);
    // 创建并显示子菜单
    auto    submenu = new MaterialSubMenu(this, m_index);
    submenu->init();
    wxPoint pos = m_mergeBtn->GetScreenPosition();
    pos.y += m_mergeBtn->GetSize().y + FromDIP(8);
    pos.x -= FromDIP(4);
    submenu->Position(pos, wxSize(0, 0));
    submenu->Cus_Popup(true, this);
}
void MaterialContextMenu::OnDelete(wxCommandEvent&) 
{
    Slic3r::GUI::wxGetApp().plater()->sidebar().delete_filament(m_index);
    PopupWindowManager::Get().CloseAll();
}
