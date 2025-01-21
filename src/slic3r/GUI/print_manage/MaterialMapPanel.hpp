#ifndef MATERIAL_MAP_PANEL_202410121057_H
#define MATERIAL_MAP_PANEL_202410121057_H

#include <string>
#include <vector>
#include <wx/event.h>
#include <wx/button.h>
#include <wx/colour.h>
#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/combobox.h>
#include <wx/gauge.h>
#include "slic3r/GUI/Widgets/PopupWindow.hpp" 
#include "slic3r/GUI/print_manage/DeviceDB.hpp"

namespace RemotePrint {

class PlateExtruderItem : public wxPanel
{

public:
    PlateExtruderItem(wxWindow* parent, const wxSize& size, const wxColour& color, const wxString& text);

    void SetColor(const wxColour& color);
    wxColour GetColor();
    void SetText(const wxString& text);

protected:
    void OnPaint(wxPaintEvent& event);

private:
    wxColour m_bk_color;
    wxString m_text;
    int m_radius = 8;
    int m_border_width = 1;
};

class PrinterColorPopPanel;

class PrinterColorItem : public wxPanel
{

public:
    PrinterColorItem(wxWindow* parent, const wxSize& size, const wxColour& color, int materialId, const std::string& matchFilamentType, int related_extruderId, const std::string& related_printer_ip);

    PrinterColorItem(wxWindow* parent, const wxSize& size, const RemotePrint::DeviceDB::DeviceBoxColorInfo& boxColorInfo);

    void SetColor(const wxColour& color);
    wxColour GetColor();
    void SetBoxColorInfo(const RemotePrint::DeviceDB::DeviceBoxColorInfo& boxColorInfo);
    RemotePrint::DeviceDB::DeviceBoxColorInfo GetBoxColorInfo();

    void SetHover(bool hover);
    wxString get_filament_type();
    int get_related_extruderId();

protected:
    void OnPaint(wxPaintEvent& event);
    void OnClick(wxMouseEvent& event);

public:
    std::string related_printer_ip = "";

private:
    wxColour m_bk_color;
    wxString m_filament_type;
    wxString m_match_index_info;  // 1A, 1B, 1C, 1D
    int m_radius = 8;
    int m_border_width = 1;
    int m_match_material_id = 0;
    int m_related_extruderId = 0;
    bool m_hover;

    PrinterColorPopPanel* m_popPanel {nullptr};
    RemotePrint::DeviceDB::DeviceBoxColorInfo m_boxColorInfo;
};


/*
* PrinterColorPopPanel
*/
class PrinterColorPopPanel : public PopupWindow
{
public:
	PrinterColorPopPanel(wxWindow* parent, int index, PrinterColorItem* targetItem);
	~PrinterColorPopPanel();

    void InitColorItems(const std::vector<RemotePrint::DeviceDB::DeviceBoxColorInfo>& boxColorInfos);

protected:
    void OnMouseEnter(wxMouseEvent& event);
    void OnMouseLeave(wxMouseEvent& event);

private:
    void OnColorItemClick(wxMouseEvent& event);

    wxBoxSizer* m_sizer;
    PrinterColorItem* m_targetItem;

};


class MaterialMapPanel : public wxPanel
{
public:
    struct ColorMatchInfo
    {
        int extruderId;
        std::string extruderFilamentType;
        wxColour extruderColor;

        std::string matchFilamentType;
        wxColour matchColor;
        int materialId;

    };

    std::string related_printer_ip = "";

public:
    MaterialMapPanel(wxWindow* parent, wxWindowID winid = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize);
    ~MaterialMapPanel();

    void clear();
    void SetMaterialMapInfo(const std::vector<ColorMatchInfo>& materialMapInfo);

    std::vector<ColorMatchInfo> GetColorMatchInfo();
    void update_color_match_info(int extruderId, const RemotePrint::DeviceDB::DeviceBoxColorInfo& boxColorInfo);

private:
    wxButton* m_helpButton;
    wxBoxSizer* m_dynamicSizer;

    std::vector<ColorMatchInfo> m_material_map_info;
};

}

#endif // MATERIAL_MAP_PANEL_202410121057_H