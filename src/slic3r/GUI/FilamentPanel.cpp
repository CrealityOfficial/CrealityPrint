#include "FilamentPanel.h"
#include <cassert>
#include <fstream>
#include <mutex>
#include <string>
#include <wx/dcclient.h>
#include <wx/dcgraph.h>
#include <wx/dcmemory.h>
#include "ImGuiWrapper.hpp"
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
#include "MainFrame.hpp"
#include "slic3r/GUI/print_manage/DeviceDB.hpp"
#include "slic3r/GUI/print_manage/Utils.hpp"
#include "slic3r/GUI/print_manage/PrinterBoxFilamentPanel.hpp"
#include <cstdint> 


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
    wxRect leftRect(FromDIP(1), FromDIP(1), size.GetWidth() / 2, size.GetHeight() - FromDIP(2));
    if (m_bReseted)
        dc.SetBrush(wxBrush(m_resetedColour));
    else
        dc.SetBrush(wxBrush(m_back_color));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(leftRect);

    // Draw the label in the left half
    if (!m_sync_filament_label.IsEmpty()) {
        int textWidth, textHeight;
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
    wxRect rightRect2(size.GetWidth() / 2, FromDIP(1), size.GetWidth() / 2, size.GetHeight() - FromDIP(2));
    if (m_bReseted)
        dc.SetBrush(wxBrush(m_resetedColour));
    else
        dc.SetBrush(wxBrush(m_back_color));
    dc.SetPen(*wxTRANSPARENT_PEN);
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
        basic_font.SetPointSize(10);
        dc.SetFont(basic_font);

        dc.GetTextExtent(m_label, &width, &height);

        int panelWidth, panelHeight;
        GetSize(&panelWidth, &panelHeight);

    	int leftHalfWidth = panelWidth / 2;

        int x = (leftHalfWidth - width) / 2;
        int y = (panelHeight - height) / 2;

		if (m_dark_img.bmp().IsOk() && m_light_img.bmp().IsOk()) {
            x = (panelWidth - FromDIP(6) - width) / 2;
		}

        dc.SetTextForeground(GetTextColorBasedOnBackground(m_back_color));
        dc.DrawText(m_label, wxPoint(x, y));
    }

	if (m_dark_img.bmp().IsOk() && m_light_img.bmp().IsOk()) {
        int x = size.GetWidth() - FromDIP(10);
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
        m_filamentCombox->update();
        m_filamentCombox->clr_picker->Hide();
        m_filamentCombox->setSelectedItemCb([&](int selectedItem) -> void { 
            if (m_pFilamentItem != nullptr) {
                m_pFilamentItem->resetCFS(true);
                }
            
            });

		// filament combox
		wxSizerItem* item = m_sizer_main->Add(m_filamentCombox, wxSizerFlags().Border(wxUP | wxDOWN | wxLEFT, 1));
        item->SetProportion(wxEXPAND);
        bool is_dark = Slic3r::GUI::wxGetApp().dark_mode();

		//
		{
            wxPanel* box = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(80, this->GetSize().GetHeight() - 2));
            box->SetBackgroundColour(wxColour(255, 255, 255));
            wxBoxSizer*sz = new wxBoxSizer(wxHORIZONTAL);
			box->SetSizer(sz);

            m_img_extruderTemp = new ScalableButton(box, wxID_ANY, is_dark ? "extruderTemp" : "extruderTemp_black", wxEmptyString, wxDefaultSize,
                                                                  wxDefaultPosition, wxBU_EXACTFIT | wxNO_BORDER, false, 12);
            m_img_extruderTemp->SetBackgroundColour(wxColour(255, 255, 255));
            wxSizerItem* item1 = sz->Add(m_img_extruderTemp, wxSizerFlags().Border(wxRIGHT | wxLEFT, 5).Border(wxLEFT, 10));
            item1->SetProportion(wxEXPAND);

            m_lb_extruderTemp = new Label(box, Label::Body_13, wxString(""));
            m_lb_extruderTemp->SetBackgroundColour(wxColour(255, 255, 255));
            wxSizerItem* item = sz->Add(m_lb_extruderTemp, wxSizerFlags().Border(wxRIGHT | wxLEFT, 5).Border(wxRIGHT, 10));
            item->SetFlag(wxALIGN_CENTER_VERTICAL);
            item->SetProportion(wxEXPAND);

            m_sizer_main->Add(box, wxSizerFlags().Border(wxUP | wxDOWN | wxLEFT, 1));
		}
        
		//
		{
            wxPanel* box = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(80, this->GetSize().GetHeight() - 2));
            box->SetBackgroundColour(wxColour(255, 255, 255));
            wxBoxSizer* sz = new wxBoxSizer(wxHORIZONTAL);
            box->SetSizer(sz);

            m_img_bedTemp = new ScalableButton(box, wxID_ANY, is_dark ? "bedTemp" :"bedTemp_black", wxEmptyString, wxDefaultSize, wxDefaultPosition, wxBU_EXACTFIT | wxNO_BORDER, false, 12);
            m_img_bedTemp->SetBackgroundColour(wxColour(255, 255, 255));
            wxSizerItem* item1 = sz->Add(m_img_bedTemp, wxSizerFlags().Border(wxRIGHT | wxLEFT, 5).Border(wxLEFT, 10));
            item1->SetProportion(wxEXPAND);

            m_lb_bedTemp = new Label(box, Label::Body_13, wxString(""));
            m_lb_bedTemp->SetBackgroundColour(wxColour(255, 255, 255));
            wxSizerItem* item = sz->Add(m_lb_bedTemp, wxSizerFlags().Border(wxRIGHT | wxLEFT, 5).Border(wxRIGHT, 10));
            item->SetFlag(wxALIGN_CENTER_VERTICAL);
            item->SetProportion(wxEXPAND);

			m_sizer_main->Add(box, wxSizerFlags().Border(wxUP | wxDOWN | wxLEFT, 1));
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

	SetSizer(m_sizer_main);
	Layout();
	Thaw();
}

FilamentPopPanel::~FilamentPopPanel() {}

void FilamentPopPanel::Popup(wxPoint position /*= wxDefaultPosition*/)
{
#ifdef __APPLE__
    wxSize wSize = GetParent()->GetSize();
     int height = wSize.GetHeight();
     if (height <= FromDIP(41))
     {
         wSize.SetHeight(wSize.GetHeight() + FromDIP(26));
         GetParent()->SetMinSize(wSize);
         GetParent()->GetSizer()->Layout();
         GetParent()->GetParent()->GetSizer()->Layout();
     }
#endif
	SetPosition(position);
	PopupWindow::Popup();
}

void FilamentPopPanel::Dismiss()
{
#ifdef __APPLE__
    wxSize wSize = GetParent()->GetSize();
    int height = wSize.GetHeight();
    
    if (height >= FromDIP(41))
    {
        wSize.SetHeight(wSize.GetHeight() - FromDIP(26));
        GetParent()->SetMinSize(wSize);
        GetParent()->GetSizer()->Layout();
        GetParent()->GetParent()->GetSizer()->Layout();
    }
#endif
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
        m_popPanel->m_lb_extruderTemp->SetLabel(wxString(std::to_string(nozzle_temperature)) + wxString::FromUTF8("°C"));
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
        m_popPanel->m_lb_bedTemp->SetLabel(wxString(std::to_string(plate_temp)) + wxString::FromUTF8("°C"));
	}
}

void FilamentItem::sys_color_changed()
{ 
	m_popPanel->sys_color_changed();
}

void FilamentItem::msw_rescale() 
{
	m_popPanel->m_filamentCombox->msw_rescale();
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
	m_box_sizer->Add(m_sizer);
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
	m_sizer->Add(filament, wxSizerFlags().Border(wxALL, 1));
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

bool FilamentPanel::LoadFile(std::string jPath, std::string &sContent)
{
    try 
    {
        boost::nowide::ifstream t(jPath);
        std::stringstream buffer;
        buffer << t.rdbuf();
        sContent=buffer.str();
        BOOST_LOG_TRIVIAL(trace) << __FUNCTION__ << boost::format(", load %1% into buffer")% jPath;
    }
    catch (std::exception &e)
    {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ",  got exception: "<<e.what();
        return false;
    }

    return true;
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

int FilamentPanel::GetFilamentInfo( std::string VendorDirectory, json & pFilaList, std::string filepath, std::string &sVendor, std::string &sType)
{
    try {
        std::string contents;
        LoadFile(filepath, contents);
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ": Json Contents: " << contents;
        json jLocal = json::parse(contents);

        if (sVendor == "") {
            if (jLocal.contains("filament_vendor"))
                sVendor = jLocal["filament_vendor"][0];
            else {
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << filepath << " - Not Contains filament_vendor";
            }
        }

        if (sType == "") {
            if (jLocal.contains("filament_type"))
                sType = jLocal["filament_type"][0];
            else {
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << filepath << " - Not Contains filament_type";
            }
        }

        if (sVendor == "" || sType == "")
        {
            if (jLocal.contains("inherits")) {
                std::string FName = jLocal["inherits"];

                if (!pFilaList.contains(FName)) { 
                    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "pFilaList - Not Contains inherits filaments: " << FName;
                    return -1; 
                }

                std::string FPath = pFilaList[FName]["sub_path"];
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " Before Format Inherits Path: VendorDirectory - " << VendorDirectory << ", sub_path - " << FPath;
                wxString strNewFile = wxString::Format("%s%c%s", wxString(VendorDirectory.c_str(), wxConvUTF8), boost::filesystem::path::preferred_separator, FPath);
                boost::filesystem::path inherits_path(w2s(strNewFile));

                //boost::filesystem::path nf(strNewFile.c_str());
                if (boost::filesystem::exists(inherits_path))
                    return GetFilamentInfo(VendorDirectory,pFilaList, inherits_path.string(), sVendor, sType);
                else {
                    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " inherits File Not Exist: " << inherits_path;
                    return -1;
                }
            } else {
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << filepath << " - Not Contains inherits";
                if (sType == "") {
                    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "sType is Empty";
                    return -1;
                }
                else
                    sVendor = "Generic";
                    return 0;
            }
        }
        else
            return 0;
    }
    catch(nlohmann::detail::parse_error &err) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__<< ": parse "<<filepath <<" got a nlohmann::detail::parse_error, reason = " << err.what();
        return -1;
    }
    catch (std::exception &e)
    {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__<< ": parse " << filepath <<" got exception: "<<e.what();
        return -1;
    }

    return 0;
}

int FilamentPanel::LoadProfileFamily(std::string strVendor, std::string strFilePath)
{
    // wxString strFolder = strFilePath.BeforeLast(boost::filesystem::path::preferred_separator);
    boost::filesystem::path file_path(strFilePath);
    boost::filesystem::path vendor_dir = boost::filesystem::absolute(file_path.parent_path() / strVendor).make_preferred();
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(",  vendor path %1%.") % vendor_dir.string();
    try {
        // wxLogMessage("GUIDE: json_path1  %s", w2s(strFilePath));

        std::string contents;
        LoadFile(strFilePath, contents);
        // wxLogMessage("GUIDE: json_path1 content: %s", contents);
        json jLocal = json::parse(contents);
        // wxLogMessage("GUIDE: json_path1 Loaded");

        // BBS:Machine
        // BBS:models

        // BBS:Filament
        json pFilament = jLocal["filament_list"];
        json tFilaList = json::object();
        int nsize          = pFilament.size();

        for (int n = 0; n < nsize; n++) 
        {
            json OneFF = pFilament.at(n);

            std::string s1    = OneFF["name"];
            std::string s2    = OneFF["sub_path"];

            tFilaList[s1] = OneFF;
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "Vendor: " << strVendor <<", tFilaList Add: " << s1;
        }

        int nFalse  = 0;
        int nModel  = 0;
        int nFinish = 0;
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(",  got %1% filaments") % nsize;
        for (int n = 0; n < nsize; n++) {
            json OneFF = pFilament.at(n);

            std::string s1 = OneFF["name"];
            std::string s2 = OneFF["sub_path"];

            if (!m_FilamentProfileJson["filament"].contains(s1)) {
                // wxString ModelFilePath = wxString::Format("%s\\%s\\%s", strFolder, strVendor, s2);
                boost::filesystem::path sub_path = boost::filesystem::absolute(vendor_dir / s2).make_preferred();
                std::string             sub_file = sub_path.string();
                LoadFile(sub_file, contents);
                json pm = json::parse(contents);
                
                std::string strInstant = pm["instantiation"];
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "Load Filament:" << s1 << ",Path:" << sub_file << ",instantiation?" << strInstant;

                if (strInstant == "true") {
                    std::string sV;
                    std::string sT;

                    int nRet = GetFilamentInfo(vendor_dir.string(),tFilaList, sub_file, sV, sT);
                    if (nRet != 0) { 
                        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "Load Filament:" << s1 << ",GetFilamentInfo Failed, Vendor:" << sV << ",Type:"<< sT;
                        continue; 
                    }

                    OneFF["vendor"] = sV;
                    OneFF["type"]   = sT;

                    OneFF["models"]   = "";

                    json pPrinters = pm["compatible_printers"];
                    int nPrinter   = pPrinters.size();
                    std::string ModelList = "";
                    for (int i = 0; i < nPrinter; i++)
                    {
                        std::string sP = pPrinters.at(i);
                        if (m_FilamentProfileJson["machine"].contains(sP))
                        {
                            std::string mModel = m_FilamentProfileJson["machine"][sP]["model"];
                            std::string mNozzle = m_FilamentProfileJson["machine"][sP]["nozzle"];
                            std::string NewModel = mModel + "++" + mNozzle;

                            ModelList = (boost::format("%1%[%2%]") % ModelList % NewModel).str();
                        }
                    }

                    OneFF["models"]    = ModelList;
                    OneFF["selected"] = 0;

                    m_FilamentProfileJson["filament"][s1] = OneFF;
                } else
                    continue;

            }
        }

    } catch (nlohmann::detail::parse_error &err) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": parse " << strFilePath << " got a nlohmann::detail::parse_error, reason = " << err.what();
        return -1;
    } catch (std::exception &e) {
        // wxMessageBox(e.what(), "", MB_OK);
        // wxLogMessage("GUIDE: LoadFamily Error: %s", e.what());
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": parse " << strFilePath << " got exception: " << e.what();
        return -1;
    }

    return 0;
}

int FilamentPanel::LoadFilamentProfile()
{
    m_FilamentProfileJson = json::parse("{}");
    m_FilamentProfileJson["filament"] = json::object();

    boost::filesystem::path vendor_dir;
    boost::filesystem::path rsrc_vendor_dir;
    vendor_dir = (boost::filesystem::path(Slic3r::data_dir()) / PRESET_SYSTEM_DIR).make_preferred();
    rsrc_vendor_dir = (boost::filesystem::path(Slic3r::resources_dir()) / "profiles").make_preferred();

    auto bbl_bundle_path = vendor_dir;
    bool bbl_bundle_rsrc = false;
    if (!boost::filesystem::exists((vendor_dir / Slic3r::PresetBundle::BBL_BUNDLE).replace_extension(".json")))
    {
        bbl_bundle_path = rsrc_vendor_dir;
        bbl_bundle_rsrc = true;
    }

    string                                targetPath = bbl_bundle_path.make_preferred().string();
    boost::filesystem::path               myPath(targetPath);
    boost::filesystem::directory_iterator endIter;
    for (boost::filesystem::directory_iterator iter(myPath); iter != endIter; iter++)
    {
        if (boost::filesystem::is_directory(*iter))
        {
            //cout << "is dir" << endl;
            //cout << iter->path().string() << endl;
        }
        else
        {
            //cout << "is a file" << endl;
            //cout << iter->path().string() << endl;

            wxString strVendor = Slic3r::GUI::from_u8(iter->path().string()).BeforeLast('.');
            strVendor = strVendor.AfterLast('\\');
            strVendor = strVendor.AfterLast('\/');
            wxString strExtension = Slic3r::GUI::from_u8(iter->path().string()).AfterLast('.').Lower();

            if (w2s(strVendor) == Slic3r::PresetBundle::BBL_BUNDLE && strExtension.CmpNoCase("json") == 0)
                LoadProfileFamily(w2s(strVendor), iter->path().string());
        }
    }

    //string                                others_targetPath = rsrc_vendor_dir.string();
    boost::filesystem::directory_iterator others_endIter;
    for (boost::filesystem::directory_iterator iter(rsrc_vendor_dir); iter != others_endIter; iter++)
    {
        if (boost::filesystem::is_directory(*iter))
        {
            //cout << "is dir" << endl;
            //cout << iter->path().string() << endl;
        }
        else
        {
            //cout << "is a file" << endl;
            //cout << iter->path().string() << endl;
            wxString strVendor = Slic3r::GUI::from_u8(iter->path().string()).BeforeLast('.');
            strVendor = strVendor.AfterLast('\\');
            strVendor = strVendor.AfterLast('\/');
            wxString strExtension = Slic3r::GUI::from_u8(iter->path().string()).AfterLast('.').Lower();

            if (w2s(strVendor) != Slic3r::PresetBundle::BBL_BUNDLE && strExtension.CmpNoCase("json") == 0)
                LoadProfileFamily(w2s(strVendor), iter->path().string());
        }
    }

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

void FilamentPanel::SetFilamentProfile(std::vector<std::pair<int, RemotePrint::DeviceDB::Material>>& validMaterials)
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

void FilamentPanel::on_auto_mapping_filament(const RemotePrint::DeviceDB::Data& deviceData)
{
    // 计算 materialBoxes 数组中 box_type == 0 的 Material 项，并且 Material 里 color 的值不为空的项
    std::vector<std::pair<int, RemotePrint::DeviceDB::Material>> validMaterials;
    for (const auto& materialBox : deviceData.materialBoxes)
    {
        if (materialBox.box_type == 0)
        {
            for (const auto& material : materialBox.materials)
            {
                if (!material.color.empty())
                {
                    validMaterials.emplace_back(materialBox.box_id, material);
                }
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
    int iNum = LoadFilamentProfile();
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
	for(int i = 0; i < validMaterials.size(); i++)
	{
        auto& item = m_vt_filament[normalIdx];
        item->update_bk_color(validMaterials[i].second.color);
        item->set_filament_selection(validMaterials[i].second.name);

        char     index_char          = 'A' + (validMaterials[i].second.material_id % 4); // Calculate the letter part (A, B, C, D)
        wxString material_sync_label = wxString::Format("%d%c", validMaterials[i].first, index_char);
        item->update_box_sync_state(true, material_sync_label);
        item->resetCFS(false);

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

	item->Refresh();
}

void FilamentPanel::on_re_sync_all_filaments(const std::string& selected_device_ip)
{
	auto& deviceDB = RemotePrint::DeviceDB::getInstance();

	auto device = deviceDB.get_printer_data(selected_device_ip);

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
	SetSize(FromDIP(200), FromDIP(150));

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

    SetSizer(m_mainSizer);
    Layout();
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

        const RemotePrint::DeviceDB::MaterialBox* material_box_info = nullptr;
        for (const auto& box_info : m_device_data.materialBoxes) {
            if (box_info.box_id == boxId) {
                material_box_info = &box_info;
                break;
            }
        }

        if (!material_box_info) return;

        int  material_id        = 0;
        bool has_exact_material = false;

        for (int i = 0; i < 4; i++) {

            material_id = i;

            FilamentColorSelectionItem* filament_item = new FilamentColorSelectionItem(m_secondColumnPanel, wxSize(FromDIP(120), FromDIP(24)));
            assert(filament_item);

            try {
                // check whether the array has the exact material
                has_exact_material = false;
                for (const auto& material : material_box_info->materials) {
                    if (material.material_id == material_id && material_box_info->box_type == 0 && !material.color.empty()) {
                        filament_item->set_sync_state(true);
                        filament_item->set_is_ext(false);
                        filament_item->update_item_info_by_material(material_box_info->box_id, material);
                        has_exact_material = true;
                        break;
                    }
                }

                if (!has_exact_material) {
                    RemotePrint::DeviceDB::Material tmp_material;
                    tmp_material.material_id = material_id;
                    tmp_material.color      = "#808080"; // grey
                    filament_item->set_sync_state(false);
                    filament_item->set_is_ext(false);
                    filament_item->update_item_info_by_material(material_box_info->box_id, tmp_material);
                }

                // Bind the click event for the second column items
                filament_item->Bind(wxEVT_BUTTON, &BoxColorPopPanel::OnSecondColumnItemClicked, this);

                m_secondColumnSizer->Add(filament_item, 0, wxALL, 5);
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

    // Perform the logic you want when an item in the second column is clicked
    // wxLogMessage("Second column item clicked");
	wxColour item_color = item->GetColor();  // GetColor()
	std::string new_filament_color = item_color.GetAsString(wxC2S_HTML_SYNTAX).ToStdString();
	std::string new_filament_name = item->get_filament_name();

	FilamentPanel* filament_panel = dynamic_cast<FilamentPanel*>(Slic3r::GUI::wxGetApp().sidebar().filament_panel());
	if(filament_panel)
	{
		filament_panel->on_sync_one_filament(m_filament_item_index, new_filament_color, new_filament_name, item->get_material_index_info());
	}
}

void BoxColorPopPanel::init_by_device_data(const RemotePrint::DeviceDB::Data& device_data)
{
	m_device_data = device_data;

	m_firstColumnSizer->Clear(true);
	m_secondColumnSizer->Clear(true);

    int cfsBoxSize = 0;
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

    if (cfsBoxSize == 1) {
        m_firstColumnSizer->Hide(size_t(0));
        m_mainSizer->Hide(size_t(0));
        m_mainSizer->Hide(size_t(1));
        m_mainSizer->Layout();
        this->SetMinSize(wxSize(FromDIP(128), FromDIP(150)));
        this->SetMaxSize(wxSize(FromDIP(128), FromDIP(150)));
    } else {
        m_firstColumnSizer->Show(size_t(0));
        m_mainSizer->Show(size_t(0));
        m_mainSizer->Show(size_t(1));
        m_mainSizer->Layout();
        this->SetMinSize(wxSize(FromDIP(200), FromDIP(150)));
        this->SetMaxSize(wxSize(FromDIP(200), FromDIP(150)));
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
	return m_filament_name;
}

wxString FilamentColorSelectionItem::get_material_index_info()
{
	return m_material_index_info;
}

void FilamentColorSelectionItem::update_item_info_by_material(int box_id, const RemotePrint::DeviceDB::Material& material_info)
{
    m_box_id  = box_id;
	m_filament_type_label = material_info.type;
	m_filament_name = material_info.name;
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
