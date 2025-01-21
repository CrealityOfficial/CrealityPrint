#include "PrinterBoxFilamentPanel.hpp"
#include "slic3r/GUI/print_manage/Utils.hpp"
#include "slic3r/GUI/print_manage/DeviceDB.hpp"
#include "slic3r/GUI/print_manage/CustomEvent.hpp"
#include "slic3r/GUI/print_manage/Utils.hpp"
#include "slic3r/GUI/print_manage/DataCenter.hpp"
#include "slic3r/GUI/wxExtensions.hpp"
#include "slic3r/GUI/I18N.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/FilamentPanel.h"
#include <cassert>
#include <exception>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/colour.h>
#include "../GUI_App.hpp"

namespace RemotePrint {

OneFilamentColorItem::OneFilamentColorItem(wxWindow* parent, const wxSize& size)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, size)
{
    Bind(wxEVT_PAINT, &OneFilamentColorItem::OnPaint, this);
    m_bmpNoColour = ScalableBitmap(this, "null_colour", FromDIP(18));
}

OneFilamentColorItem::~OneFilamentColorItem()
{
    // Any necessary cleanup code here
}

void OneFilamentColorItem::set_sync_state(bool bSync)
{
    m_sync = bSync;
}

void OneFilamentColorItem::set_is_ext(bool is_ext)
{
    m_is_ext = is_ext;
}

void OneFilamentColorItem::setOriginMaterial(const RemotePrint::DeviceDB::Material& material_info) { m_originMaterial = material_info; }

void OneFilamentColorItem::SetColor(const wxColour& color)
{
    m_bk_color = color;
    if (color == wxNullColour) {
        m_bk_color = wxColour("#FFFFFF");
        m_bMaterialNoColour = true;
    } else {
        m_bMaterialNoColour = false;
    }
    Refresh(); // Trigger a repaint
}

wxColour OneFilamentColorItem::GetColor()
{
    return m_bk_color;
}

void OneFilamentColorItem::update_item_info_by_material(int box_id, const RemotePrint::DeviceDB::Material& material_info)
{
    m_box_id  = box_id;
    m_bk_color = RemotePrint::Utils::hex_string_to_wxcolour(material_info.color);
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
            if (m_originMaterial.state == 1)
                m_material_index_info = "?";
            else
                m_material_index_info = "/";
        }
        
    }
    
}

// draw one color rectangle and text "1A" or "1B" or "1C" or "1D" over this rectangle
void OneFilamentColorItem::OnPaint(wxPaintEvent& event)
{
    wxAutoBufferedPaintDC dc(this); // Use double-buffering to avoid flicker
    //dc.Clear();

    // Set the background color and border color
    if (m_bk_color != wxNullColour) {
        dc.SetBrush(wxBrush(m_bk_color));
        dc.SetPen(wxPen(m_bk_color));
        if (!Slic3r::GUI::wxGetApp().dark_mode() && m_bk_color == wxColour("#FFFFFF"))
            dc.SetPen(wxPen(wxColour("#D0D4DE"), 1, wxPENSTYLE_SOLID));
        if (m_bMaterialNoColour) {
            dc.SetBrush(wxBrush(m_bmpNoColour.bmp()));
        }
        // Draw the rectangle
        // dc.DrawRectangle(m_border_width, m_border_width, size.GetWidth() - 2 * m_border_width, size.GetHeight() - 2 * m_border_width);
    } else {
        // Set the brush and pen to transparent
        dc.SetBrush(wxColour(75, 75, 75));
        dc.SetPen(*wxTRANSPARENT_PEN);
    }

    // // Get the size of the panel
    // wxSize size = wxSize(FromDIP(50), FromDIP(50));

    // Get the size of the panel
    wxSize size = GetClientSize();

    // Draw the rectangle
    dc.DrawRoundedRectangle(m_border_width, m_border_width, size.GetWidth() - 2 * m_border_width, size.GetHeight() - 2 * m_border_width, 4);

    // Set the text color
    // dc.SetTextForeground(*wxWHITE);
    dc.SetTextForeground(RemotePrint::Utils::should_dark(m_bk_color) ? *wxBLACK : *wxWHITE);

    // Calculate the position to draw the text
    wxCoord textWidth, textHeight;

    wxString tmpText = m_material_index_info;
    //if(m_bk_color == wxNullColour)
    if (m_bMaterialNoColour)
    {
        tmpText = "?";
    }

    dc.GetTextExtent(tmpText, &textWidth, &textHeight);
    wxCoord x = (size.GetWidth() - textWidth) / 2;
    wxCoord y = (size.GetHeight() - textHeight) / 2;

    // Draw the text
    dc.DrawText(tmpText, x, y);

    // Restore the previous state of the device context
    event.Skip();
}


OneBoxFilamentColorItem::OneBoxFilamentColorItem(wxWindow* parent, const wxSize& size)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, size)
{
	m_box_sizer = new wxBoxSizer(wxHORIZONTAL);
	this->SetSizer(m_box_sizer);
	m_box_sizer->AddSpacer(FromDIP(8));

    m_bk_color = wxColour(75, 75, 77);

    // SetBackgroundStyle(wxBG_STYLE_PAINT); // 使得双缓冲绘图生效
    Bind(wxEVT_PAINT, &OneBoxFilamentColorItem::OnPaint, this);
}

OneBoxFilamentColorItem::~OneBoxFilamentColorItem()
{
    for (auto item : m_filament_color_items) {
        delete item;
    }
    m_filament_color_items.clear();

    if (m_ext_filament_item) {
        delete m_ext_filament_item;
        m_ext_filament_item = nullptr;
    }
}

void OneBoxFilamentColorItem::update_ui_item_info_by_material_box_info(const RemotePrint::DeviceDB::MaterialBox& material_box_info)
{
    m_box_id = material_box_info.box_id;
    

    for (auto& item : m_filament_color_items) {
        item->Destroy();
    }
    m_filament_color_items.clear();

    bool has_exact_material = false;
    int material_id = 0;
    if (0 == material_box_info.box_type) {  // normal multi-color box

        m_cfs_index_info        = wxString::Format("CFS%d", m_box_id); // CFS1, CFS2, CFS3, CFS4
        wxStaticText* cfs_label = new wxStaticText(this, wxID_ANY, m_cfs_index_info);
        if (Slic3r::GUI::wxGetApp().dark_mode())
            cfs_label->SetForegroundColour(wxColour("#FFFFFF"));
        else
            cfs_label->SetForegroundColour(wxColour("#000000"));
        //m_box_sizer->AddSpacer(FromDIP(8));
        m_box_sizer->Add(cfs_label, 0, wxALIGN_CENTRE_VERTICAL);
        m_box_sizer->AddSpacer(FromDIP(8));

        for (int i = 0; i < 4; i++) {
            material_id                         = i;
            OneFilamentColorItem* filament_item = new OneFilamentColorItem(this, wxSize(FromDIP(24), FromDIP(24)));
            assert(filament_item);
            if (material_box_info.materials.size() > i)
                filament_item->setOriginMaterial(material_box_info.materials[i]);

            // check whether the array has the exact material
            has_exact_material = false;
            for (const auto& material : material_box_info.materials)
            {
                if (material.material_id == material_id && material_box_info.box_type == 0 && !material.color.empty())
                {
                    filament_item->set_sync_state(true);
                    filament_item->set_is_ext(false);
                    filament_item->update_item_info_by_material(material_box_info.box_id, material);
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
                filament_item->update_item_info_by_material(material_box_info.box_id, tmp_material);
            }

            m_filament_color_items.push_back(filament_item);
            m_box_sizer->Add(filament_item, 0, wxALIGN_CENTRE_VERTICAL);
            m_box_sizer->AddSpacer(FromDIP(8));
            m_box_sizer->Layout();
        }

    } 
    else if (1 == material_box_info.box_type) // extra box
    {
        m_cfs_index_info = _L("EXT");
        // if(material_box_info.materials.size() > 0 && material_box_info.materials[0].color.empty())
        // {
        //     m_cfs_index_info = "?";
        // }

        wxStaticText* cfs_label = new wxStaticText(this, wxID_ANY, m_cfs_index_info);
        if (Slic3r::GUI::wxGetApp().dark_mode())
            cfs_label->SetForegroundColour(wxColour("#FFFFFF"));
        else 
            cfs_label->SetForegroundColour(wxColour("#000000"));
        m_box_sizer->Add(cfs_label, 0, wxALIGN_CENTRE_VERTICAL);
        m_box_sizer->AddSpacer(FromDIP(12));

        for (const auto& material : material_box_info.materials) {

            OneFilamentColorItem* filament_item = new OneFilamentColorItem(this, wxSize(FromDIP(120), FromDIP(24)));
            assert(filament_item);
            filament_item->set_sync_state(true);
            filament_item->set_is_ext(true);
            filament_item->update_item_info_by_material(material_box_info.box_id, material);

            // 检查 m_cfs_index_info 的值，并设置背景色
            if(material.color.empty()) {
                filament_item->SetColor(wxNullColour);
            }

            m_filament_color_items.push_back(filament_item);
            m_box_sizer->Add(filament_item, 0, wxALIGN_CENTRE_VERTICAL);
            m_box_sizer->AddSpacer(FromDIP(3)); // Add some space before each filament_item
	        m_box_sizer->Layout();
        }
    }

    // 重新布局面板
    Layout();
    Refresh();
}

/// draw a text : CFS1, or CFS2, or CFS3, or CFS4, and then on the right draw 4(or less) OneFilamentColorItem, between them has some space
void OneBoxFilamentColorItem::OnPaint(wxPaintEvent& event)
{
}


PrinterBoxFilamentPanel::PrinterBoxFilamentPanel(wxWindow* parent, wxWindowID winid, const wxPoint& pos, const wxSize& size)
    : wxPanel(parent, winid, pos, size)
{
    // Create a horizontal sizer for the entire layout
    wxBoxSizer* main_sizer = new wxBoxSizer(wxHORIZONTAL);

    // Create the vertical sizer for the filament items
    m_sizer = new wxWrapSizer(wxHORIZONTAL);
    m_box_sizer = new wxBoxSizer(wxVERTICAL);
    m_box_sizer->Add(m_sizer, 1, wxEXPAND);
    m_box_sizer->AddSpacer(FromDIP(3));

    // Add the vertical sizer to the main horizontal sizer
    main_sizer->Add(m_box_sizer, 1, wxEXPAND);

    // Create the auto mapping button
    //m_autoMappingButton = new wxButton(this, wxID_ANY, _L("Auto Mapping"));
    //m_autoMappingButton->SetBackgroundColour(wxColour(38,38,38)); // Set background color to grey
    //main_sizer->Add(m_autoMappingButton, 0, wxALIGN_RIGHT | wxALL, 5);

    // Set the main sizer for the panel
    this->SetSizer(main_sizer);

    // Bind the button click event to the on_auto_mapping method
    //m_autoMappingButton->Bind(wxEVT_BUTTON, &PrinterBoxFilamentPanel::on_auto_mapping, this);

    m_timer = new wxTimer();
    m_timer->SetOwner(this);
    Bind(wxEVT_TIMER, &PrinterBoxFilamentPanel::OnTimer, this);

    m_box_color_pop_panel = new BoxColorPopPanel(this);
    m_box_color_pop_panel->Hide();

}

void PrinterBoxFilamentPanel::on_auto_device_filament_mapping()
{
    if(m_device_data.materialBoxes.empty())
        return;

    Slic3r::GUI::DeviceDataEvent tmpEvent(Slic3r::GUI::EVT_AUTO_SYNC_CURRENT_DEVICE_FILAMENT, m_device_data);
    wxPostEvent(Slic3r::GUI::wxGetApp().plater(), tmpEvent);
}

void PrinterBoxFilamentPanel::on_show_box_color_selection(wxPoint popup_pos, int sync_filament_item_index)
{
    try {
        if(m_box_color_pop_panel)
        {
            int displayIndex = wxDisplay::GetFromPoint(popup_pos);
            if (displayIndex == wxNOT_FOUND) {
                displayIndex = 0;
            }

            wxDisplay display(displayIndex);
            wxRect screenRect = display.GetGeometry();

            wxSize panelSize = m_box_color_pop_panel->GetSize();

            if (popup_pos.x + panelSize.GetWidth() > screenRect.GetRight())
            {
                popup_pos.x = screenRect.GetRight() - panelSize.GetWidth();
            }

            m_box_color_pop_panel->SetPosition(popup_pos);
            m_box_color_pop_panel->set_filament_item_index(sync_filament_item_index);
            m_box_color_pop_panel->init_by_device_data(m_device_data);
            m_box_color_pop_panel->Show();
            m_box_color_pop_panel->select_first_on_show();
        }
    } catch (std::exception& e) {
    
    }

}

void PrinterBoxFilamentPanel::on_auto_mapping(wxCommandEvent &event)
{
    on_auto_device_filament_mapping();
}

// on the left side, draw 4(or less) OneBoxFilamentColorItems, each row has two OneBoxFilamentColorItems at the most;
// on the right side, draw the "auto mapping" button
void PrinterBoxFilamentPanel::OnPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);

}

void PrinterBoxFilamentPanel::update_device_data(const RemotePrint::DeviceDB::Data& device_data)
{
    try {
        m_device_data = device_data;
        update_box_filament_items();
    } catch (const std::exception& e) {
        return;
    }

}

void PrinterBoxFilamentPanel::update_box_filament_items()
{
    // 清空现有的 OneBoxFilamentColorItem 和 OneFilamentColorItem
    for (auto& item : m_filament_box_items) {
        item->Destroy();
    }
    m_filament_box_items.clear();

    if (m_device_data.materialBoxes.empty()) {
        return;
    }

    std::sort(m_device_data.materialBoxes.begin(), m_device_data.materialBoxes.end(),
            [](const DeviceDB::MaterialBox& a, const DeviceDB::MaterialBox& b) { return a.box_type > b.box_type; });

    bool has_exact_material = false;
    for (const auto& materialBox : m_device_data.materialBoxes) {
        OneBoxFilamentColorItem* one_box_item = new OneBoxFilamentColorItem(this, wxDefaultSize);
        assert(one_box_item);
        one_box_item->update_ui_item_info_by_material_box_info(materialBox);

        m_filament_box_items.push_back(one_box_item);
        for (const auto& material : materialBox.materials) {
            if (materialBox.box_type == 0 && !material.color.empty()) {
                has_exact_material = true;
                break;
            }
        }

        m_sizer->Add(one_box_item, 0, wxTOP | wxBOTTOM, FromDIP(5));
	    m_sizer->Layout();
	    this->GetParent()->Layout();
    }
    if (m_funcExactMaterialCb) {
        m_funcExactMaterialCb(has_exact_material);
    }

}

bool PrinterBoxFilamentPanel::Show(bool bShow)
{
    if (bShow) 
    {
        if (!m_timer->IsRunning()) 
        {
            m_timer->Start(6000);
        }
    }
    else {
        if (m_timer->IsRunning()) 
        {
            m_timer->Stop();
        }
    }

    return wxPanel::Show(bShow);

}

void PrinterBoxFilamentPanel::OnTimer(wxTimerEvent& event)
{
    m_sizer->Layout();
    this->GetParent()->Layout();
    try {

    std::string current_device_id = RemotePrint::DeviceDB::getInstance().get_current_device_id();
    if (current_device_id.empty()) {
        return;
    }

    nlohmann::json printer = DataCenter::Ins().find_printer_by_mac(current_device_id);

    if (!printer.is_null()) {

        RemotePrint::DeviceDB::Data device_data;
        device_data.materialBoxes.clear();

        device_data.mac             = printer.contains("mac") ? printer["mac"].get<std::string>() : "";
        device_data.address         = printer.contains("address") ? printer["address"].get<std::string>() : "";
        device_data.model           = printer.contains("model") ? printer["model"].get<std::string>() : "";
        device_data.online          = printer.contains("online") ? printer["online"].get<bool>() : false;
        device_data.deviceState     = printer.contains("deviceState") ?  printer["deviceState"].get<int>() : 0;
        device_data.name            = printer.contains("name") ? printer["name"].get<std::string>() : "";

        device_data.deviceType      = printer.contains("deviceType") ? printer["deviceType"].get<int>() : 0;
        device_data.isCurrentDevice = printer.contains("isCurrentDevice") ? printer["isCurrentDevice"].get<bool>() : false;
        device_data.webrtcSupport = printer.contains("webrtcSupport") ? printer["webrtcSupport"].get<int>() == 1:false;
        device_data.tbId = (printer.contains("tbId") && !printer["tbId"].is_null()) ? printer["tbId"].get<std::string>() : "";
        device_data.modelName = printer.contains("modelName") ? printer["modelName"].get<std::string>():"";

        if (printer.contains("boxsInfo") && printer["boxsInfo"].contains("materialBoxs")) {
            auto& materialBoxs = printer["boxsInfo"]["materialBoxs"];

            for (const auto& box : materialBoxs) {
                RemotePrint::DeviceDB::MaterialBox materialBox;
                materialBox.box_id    = box.contains("id") ? box["id"].get<int>() : 0;
                materialBox.box_state = box.contains("state") ? box["state"].get<int>() : 0;
                materialBox.box_type  = box.contains("type") ? box["type"].get<int>() : 0;
                if (box.contains("temp")) {
                    materialBox.temp = box["temp"];
                }
                if (box.contains("humidity")) {
                    materialBox.humidity = box["humidity"];
                }
                
                if(box.contains("materials")) {
                    for (const auto& material : box["materials"]) {
                        RemotePrint::DeviceDB::Material mat;
                        mat.material_id = material.contains("id") ? material["id"].get<int>() : 0;
                        mat.vendor      = material.contains("vendor") ? material["vendor"].get<std::string>() : "";
                        mat.type        = material.contains("type") ? material["type"].get<std::string>() : "";
                        mat.name        = material.contains("name") ? material["name"].get<std::string>() : "";
                        mat.rfid        = material.contains("rfid") ? material["rfid"].get<std::string>() : "";
                        mat.color       = material.contains("color") ? material["color"].get<std::string>() : "";
                        mat.diameter    = material.contains("diameter") ? material["diameter"].get<double>() : 0.0;
                        mat.minTemp     = material.contains("minTemp") ? material["minTemp"].get<int>() : 0;
                        mat.maxTemp     = material.contains("maxTemp") ? material["maxTemp"].get<int>() : 0;
                        mat.pressure    = material.contains("pressure") ? material["pressure"].get<double>() : 0.0;
                        mat.percent     = material.contains("percent") ? material["percent"].get<int>() : 0;
                        mat.state       = material.contains("state") ? material["state"].get<int>() : 0;
                        mat.selected    = material.contains("selected") ? material["selected"].get<int>() : 0;
                        mat.editStatus  = material.contains("editStatus") ? material["editStatus"].get<int>() : 0;

                        materialBox.materials.push_back(mat);
                    }
                }


                device_data.materialBoxes.push_back(materialBox);
            }

            // 比较 device_data 的 materialBoxes 与 m_device_data 的 materialBoxes
            bool materialBoxesEqual = true;
            try {
                for (const auto& box : device_data.materialBoxes) {
                    if (!RemotePrint::findAndCompareMaterialBoxes(m_device_data.materialBoxes, box)) {
                        materialBoxesEqual = false;
                        break;
                    }
                }
            } catch (std::exception& e) {
                materialBoxesEqual = true;
            }


            if (!materialBoxesEqual) {
                update_device_data(device_data);
            }

        }
    }
    else {
        Slic3r::GUI::wxGetApp().sidebar().show_box_filament_content(false);
    }

    }
    catch (std::exception& e) {
    
    }

}

PrinterBoxFilamentPanel::~PrinterBoxFilamentPanel()
{
    if (m_timer->IsRunning()) {
        m_timer->Stop();
    }

    if(m_timer)
    {
        delete m_timer;
        m_timer = nullptr;
    }
}

} // namespace RemotePrint
