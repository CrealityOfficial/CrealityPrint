#include "MaterialMapPanel.hpp"
#include "slic3r/GUI/print_manage/Utils.hpp"
#include "slic3r/GUI/print_manage/DeviceDB.hpp"
#include "slic3r/GUI/wxExtensions.hpp"
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/colour.h>

namespace RemotePrint {

PlateExtruderItem::PlateExtruderItem(wxWindow* parent, const wxSize& size, const wxColour& color, const wxString& text)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, size), m_bk_color(color), m_text(text)
{
    Bind(wxEVT_PAINT, &PlateExtruderItem::OnPaint, this);
}

void PlateExtruderItem::SetColor(const wxColour& color)
{
    m_bk_color = color;
    Refresh(); // Trigger a repaint
}

wxColour PlateExtruderItem::GetColor()
{
    return m_bk_color;
}

void PlateExtruderItem::SetText(const wxString& text)
{
    m_text = text;
    Refresh(); // Trigger a repaint
}

void PlateExtruderItem::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	wxSize size = this->GetSize();

    wxRect rc(0, 0, size.x, size.y);

    dc.SetPen(wxPen(m_bk_color, m_border_width));
    dc.SetBrush(wxBrush(m_bk_color));

    if (m_radius == 0) {
        dc.DrawRectangle(rc);
    } else {
        dc.DrawRoundedRectangle(rc, m_radius - m_border_width);
    }

    // Set text color and font
    dc.SetTextForeground(*wxBLACK); // You can change the color as needed
    dc.SetFont(GetFont());

    // Calculate text position to center it
    wxSize textSize = dc.GetTextExtent(m_text);
    int    textX    = (size.x - textSize.x) / 2;
    int    textY    = (size.y - textSize.y) / 2;

    // Draw the text
    dc.DrawText(m_text, textX, textY);
}


PrinterColorPopPanel::PrinterColorPopPanel(wxWindow* parent, int index, PrinterColorItem* targetItem)
    : PopupWindow(parent, wxBORDER_SIMPLE), m_targetItem(targetItem)
{
    m_sizer = new wxBoxSizer(wxHORIZONTAL);
    SetSizer(m_sizer);

    // Set background color
    SetBackgroundColour(wxColour(54, 54, 56)); // Light grey background color

    // Calculate the size of the popup window to fit 4 PrinterColorItems
    int itemWidth = 60;
    int itemHeight = 45;
    int itemSpacing = 5;
    int totalWidth = 4 * itemWidth + 3 * itemSpacing + 2 * itemSpacing; // 4 items + 3 spaces between them + 2 spaces for padding
    int totalHeight = itemHeight + 2 * itemSpacing; // 1 item height + 2 spaces for padding

    SetSize(totalWidth, totalHeight);
}

PrinterColorPopPanel::~PrinterColorPopPanel()
{
}

void PrinterColorPopPanel::InitColorItems(const std::vector<RemotePrint::DeviceDB::DeviceBoxColorInfo>& boxColorInfos)
{
    for (const auto& colorInfo : boxColorInfos) {
        if(1 == colorInfo.boxType)  
        {
            // 1: extra box
            continue;
        }

        PrinterColorItem* colorItem = new PrinterColorItem(this, wxSize(60, 45), colorInfo);
        colorItem->Bind(wxEVT_LEFT_DOWN, &PrinterColorPopPanel::OnColorItemClick, this);
        colorItem->Bind(wxEVT_ENTER_WINDOW, &PrinterColorPopPanel::OnMouseEnter, this);
        colorItem->Bind(wxEVT_LEAVE_WINDOW, &PrinterColorPopPanel::OnMouseLeave, this);
        m_sizer->Add(colorItem, 0, wxALL, 5);
    }

    Layout();
}

void PrinterColorPopPanel::OnColorItemClick(wxMouseEvent& event)
{
    PrinterColorItem* clickedItem = dynamic_cast<PrinterColorItem*>(event.GetEventObject());
    if (clickedItem && m_targetItem && !m_targetItem->get_filament_type().IsEmpty() && clickedItem->get_filament_type() == m_targetItem->get_filament_type()) {
        m_targetItem->SetBoxColorInfo(clickedItem->GetBoxColorInfo());
        m_targetItem->Refresh();
    }
    this->Hide();
    event.Skip();
}

void PrinterColorPopPanel::OnMouseEnter(wxMouseEvent& event)
{
    PrinterColorItem* item = dynamic_cast<PrinterColorItem*>(event.GetEventObject());
    if (item) {
        item->SetHover(true);
        item->Refresh();
    }
    event.Skip();
}

void PrinterColorPopPanel::OnMouseLeave(wxMouseEvent& event)
{
    PrinterColorItem* item = dynamic_cast<PrinterColorItem*>(event.GetEventObject());
    if (item) {
        item->SetHover(false);
        item->Refresh();
    }
    event.Skip();
}



PrinterColorItem::PrinterColorItem(wxWindow* parent, const wxSize& size, const wxColour& color, int materialId, const std::string& matchFilamentType, int related_extruderId, const std::string& related_printer_ip)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, size), related_printer_ip(related_printer_ip), m_bk_color(color), m_match_material_id(materialId), m_filament_type(matchFilamentType), m_related_extruderId(related_extruderId)
{
    // invalid match; NOT FOUND match filament in device
    if(m_filament_type == "")
    {
        m_bk_color = wxColour(128,128,128);
        m_match_index_info = "/";
    }
    else {
        // Set m_match_index_info based on m_match_index
        char index_char    = 'A' + (m_match_material_id % 4); // Calculate the letter part (A, B, C, D)
        m_match_index_info = wxString::Format("%d%c", 1, index_char);
    }

    m_hover = false;
    Bind(wxEVT_PAINT, &PrinterColorItem::OnPaint, this);
    Bind(wxEVT_LEFT_DOWN, &PrinterColorItem::OnClick, this);

}

PrinterColorItem::PrinterColorItem(wxWindow* parent, const wxSize& size, const RemotePrint::DeviceDB::DeviceBoxColorInfo& boxColorInfo)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, size)
{
    m_boxColorInfo = boxColorInfo;
    m_bk_color = RemotePrint::Utils::hex_string_to_wxcolour(boxColorInfo.color);
    m_filament_type = boxColorInfo.filamentType;

    // Set m_match_index_info based on m_match_index
    if(0 == m_boxColorInfo.boxType)
    {
        char index_char    = 'A' + (m_boxColorInfo.materialId % 4); // Calculate the letter part (A, B, C, D)
        m_match_index_info = wxString::Format("%d%c", 1, index_char);
    }
    else if(1 == m_boxColorInfo.boxType)
    {
        m_match_index_info = "EXT";
    }
    
    Bind(wxEVT_PAINT, &PrinterColorItem::OnPaint, this);
}

void PrinterColorItem::SetColor(const wxColour& color)
{
    m_bk_color = color;
    Refresh(); // Trigger a repaint
}

wxColour PrinterColorItem::GetColor()
{
    return m_bk_color;
}

wxString PrinterColorItem::get_filament_type()
{
    return m_filament_type;
}

int PrinterColorItem::get_related_extruderId()
{
    return m_related_extruderId;
}

void PrinterColorItem::SetBoxColorInfo(const RemotePrint::DeviceDB::DeviceBoxColorInfo& boxColorInfo)
{
    m_boxColorInfo = boxColorInfo;
    m_bk_color = RemotePrint::Utils::hex_string_to_wxcolour(m_boxColorInfo.color);
    m_filament_type = m_boxColorInfo.filamentType;
    // Set m_match_index_info based on m_match_index
    char index_char = 'A' + (m_boxColorInfo.materialId % 4); // Calculate the letter part (A, B, C, D)
    m_match_index_info = wxString::Format("%d%c", 1, index_char);

    // update the color match info here
    MaterialMapPanel* materialMapPanel = dynamic_cast<MaterialMapPanel*>(GetParent());
    if(materialMapPanel)
    {
        materialMapPanel->update_color_match_info(m_related_extruderId, m_boxColorInfo);
    }

}

RemotePrint::DeviceDB::DeviceBoxColorInfo PrinterColorItem::GetBoxColorInfo()
{
    return m_boxColorInfo;
}

void PrinterColorItem::OnClick(wxMouseEvent& event)
{
    std::vector<RemotePrint::DeviceDB::DeviceBoxColorInfo> boxColorInfos;

    try {
        RemotePrint::DeviceDB::Data printer_data = RemotePrint::DeviceDB::getInstance().get_printer_data(related_printer_ip);

        boxColorInfos = printer_data.boxColorInfos;

    } catch (const std::exception& e) {
        wxLogError("Failed to get printer data: %s", e.what());
    }

    if(!m_popPanel)
    {
        m_popPanel = new PrinterColorPopPanel(this, wxID_ANY, this);
        m_popPanel->InitColorItems(boxColorInfos);
    }

    m_popPanel->SetPosition(ClientToScreen(wxPoint(0, GetSize().GetHeight())));
    m_popPanel->Popup();

    Refresh();
    Update();

}

void PrinterColorItem::SetHover(bool hover)
{
    m_hover = hover;
    Refresh();
}

void PrinterColorItem::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	wxSize size = this->GetSize();
	if (1) {
		wxRect rc(0, 0, size.x, size.y);

		dc.SetPen(wxPen( m_bk_color, m_border_width));
		dc.SetBrush(wxBrush(m_bk_color));

		if (m_radius == 0) {
			dc.DrawRectangle(rc);
		}
		else {
			dc.DrawRoundedRectangle(rc, m_radius - m_border_width);
		}

        // Draw border if hovered
        if (m_hover) {
            dc.SetPen(wxPen(*wxGREEN, 2)); // Black border with 2 pixels width
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.DrawRectangle(rc);
        }

        // Set text color and font
        if (m_bk_color == *wxBLACK) {
            dc.SetTextForeground(*wxWHITE); // Set text color to white if background is black
        } else {
            dc.SetTextForeground(*wxBLACK); // Otherwise, set text color to black
        }
        dc.SetFont(GetFont());

        // Calculate text positions
        wxSize matchIndexSize = dc.GetTextExtent(m_match_index_info);
        wxSize filamentTypeSize = dc.GetTextExtent(m_filament_type);

        int textX = (size.x - matchIndexSize.x) / 2;
        int textY = (size.y - matchIndexSize.y - filamentTypeSize.y) / 2;

        // Draw m_match_index_info
        dc.DrawText(m_match_index_info, textX, textY);

        // Draw a horizontal line
        int lineY = textY + matchIndexSize.y;
        int lineLength = size.x / 2; // Set the line length to half of the item width
        int lineStartX = (size.x - lineLength) / 2; // Center the line horizontally
        if (m_bk_color == *wxBLACK) {
            dc.SetPen(*wxWHITE); // Set text color to white if background is black
        } else {
            dc.SetPen(*wxBLACK); // Otherwise, set text color to black
        }
        dc.DrawLine(lineStartX, lineY, lineStartX + lineLength, lineY);

        // Draw m_filament_type below the line, but not too close to the bottom
        int filamentTextY = lineY + 8; // 8 pixels below the line
        int filamentTextX = (size.x - filamentTypeSize.x) / 2;
        if (filamentTextY + filamentTypeSize.y > size.y - 5) { // Ensure it's not too close to the bottom
            filamentTextY = size.y - filamentTypeSize.y - 5;
        }

        dc.DrawText(m_filament_type.IsEmpty() ? "/" : m_filament_type, filamentTextX, filamentTextY);
        
	}
}


MaterialMapPanel::MaterialMapPanel(wxWindow* parent, wxWindowID winid, const wxPoint& pos, const wxSize& size)
    : wxPanel(parent, winid, pos, size)
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);

    // Help button
    m_helpButton = new wxButton(this, wxID_ANY, "Help");
    // mainSizer->Add(m_helpButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

        // Dynamic display area
    m_dynamicSizer = new wxBoxSizer(wxHORIZONTAL);
    m_dynamicSizer->Add(m_helpButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

    mainSizer->Add(m_dynamicSizer, 1, wxEXPAND | wxALL, 5);

    SetSizer(mainSizer);
    Layout();
}

void MaterialMapPanel::SetMaterialMapInfo(const std::vector<ColorMatchInfo>& materialMapInfo)
{
    m_dynamicSizer->Clear(true);
    m_material_map_info = materialMapInfo;

    for (const auto& mapInfo : m_material_map_info) {
        wxBoxSizer* itemSizer = new wxBoxSizer(wxHORIZONTAL);

        // Small oval icon
        PlateExtruderItem* plateExtruderItem = new PlateExtruderItem(this, wxSize(60, 20), mapInfo.extruderColor, wxString::Format("%s", mapInfo.extruderFilamentType));
        itemSizer->Add(plateExtruderItem, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);

        // Right arrow
        ScalableButton* rightArrow = new ScalableButton(this, wxID_ANY, "map_right_arrow", wxEmptyString, wxDefaultSize, wxDefaultPosition,wxBU_EXACTFIT | wxNO_BORDER, true, 10);
        rightArrow->SetBackgroundColour(wxColour(75, 75, 77));
        itemSizer->Add(rightArrow, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);

        PrinterColorItem* printerColorItem = new PrinterColorItem(this, wxSize(60, 45), mapInfo.matchColor, mapInfo.materialId, mapInfo.matchFilamentType, mapInfo.extruderId, this->related_printer_ip);
        itemSizer->Add(printerColorItem, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);

        m_dynamicSizer->Add(itemSizer, 0, wxEXPAND | wxALL, 5);
    }

    Layout();
    // Optionally, you can add code here to update the UI based on the new m_plate_extruders values
    Refresh(); // Trigger a repaint
}

std::vector<MaterialMapPanel::ColorMatchInfo> MaterialMapPanel::GetColorMatchInfo()
{
    return m_material_map_info;
}

void MaterialMapPanel::update_color_match_info(int extruderId, const RemotePrint::DeviceDB::DeviceBoxColorInfo& boxColorInfo)
{
    for (auto& info : m_material_map_info) {
        if (info.extruderId == extruderId) {
            info.matchFilamentType = boxColorInfo.filamentType;
            info.matchColor        = RemotePrint::Utils::hex_string_to_wxcolour(boxColorInfo.color);
            info.materialId        = boxColorInfo.materialId;
            break;
        }
    }
}

MaterialMapPanel::~MaterialMapPanel()
{
}

void MaterialMapPanel::clear()
{
}

}