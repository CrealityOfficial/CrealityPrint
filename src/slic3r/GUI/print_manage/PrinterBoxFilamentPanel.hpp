#ifndef PRINTER_BOX_FILAMENT_PANEL_H
#define PRINTER_BOX_FILAMENT_PANEL_H

#include <string>
#include <vector>
#include <wx/event.h>
#include <wx/button.h>
#include <wx/colour.h>
#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/wrapsizer.h>
#include <wx/stattext.h>
#include <wx/combobox.h>
#include <wx/gauge.h>
#include <wx/timer.h>
#include "slic3r/GUI/Widgets/PopupWindow.hpp" 
#include "slic3r/GUI/print_manage/DeviceDB.hpp"
#include "slic3r/GUI/wxExtensions.hpp"

class BoxColorPopPanel;

namespace RemotePrint {

// draw one color rectangle and text "1A" or "1B" or "1C" or "1D"
class OneFilamentColorItem : public wxPanel
{

public:
    OneFilamentColorItem(wxWindow* parent, const wxSize& size);
    ~OneFilamentColorItem();

    void SetColor(const wxColour& color);
    wxColour GetColor();

    void update_item_info_by_material(int box_id, const RemotePrint::DeviceDB::Material& material_info);
    void set_sync_state(bool bSync);
    void set_is_ext(bool is_ext);
    void setOriginMaterial(const RemotePrint::DeviceDB::Material& material_info);

protected:
    void OnPaint(wxPaintEvent& event);

private:
    wxColour m_bk_color;
    int m_box_id;
    int m_material_id;
    wxString m_material_index_info;  // 1A, 1B, 1C, 1D
    int m_radius = 8;
    int m_border_width = 0;
    bool m_sync = false;
    bool m_is_ext = false;
    bool m_bMaterialNoColour = false;
    ScalableBitmap m_bmpNoColour;
    RemotePrint::DeviceDB::Material m_originMaterial;
};

/// @brief Box filament color item: draw 4(or less) OneFilamentColorItem, and in the front, draw a text : CFS1, or CFS2, or CFS3, or CFS4
class OneBoxFilamentColorItem : public wxPanel
{

public:
    OneBoxFilamentColorItem(wxWindow* parent, const wxSize& size);
    ~OneBoxFilamentColorItem();

    void update_ui_item_info_by_material_box_info(const RemotePrint::DeviceDB::MaterialBox& material_box_info);

    std::vector<OneFilamentColorItem*> m_filament_color_items;

    OneFilamentColorItem* m_ext_filament_item {nullptr};

protected:
    void OnPaint(wxPaintEvent& event);

private:
    wxBoxSizer* m_box_sizer;
    wxColour m_bk_color;
    int m_box_id = 0;
    RemotePrint::DeviceDB::Data m_device_data;
    wxString m_cfs_index_info;  // CFS1, CFS2, CFS3, CFS4
    int m_radius = 8;
    int m_border_width = 0;
    wxString m_ext_material_label;  // EXT material slot
};

// draws (4 row at the most) of filament colors, on the right side is a "auto mapping" button
class PrinterBoxFilamentPanel : public wxPanel
{
public:
    PrinterBoxFilamentPanel(wxWindow* parent, wxWindowID winid = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize);
    ~PrinterBoxFilamentPanel();

    void update_device_data(const RemotePrint::DeviceDB::Data& device_data);
    void on_auto_device_filament_mapping();
    void on_show_box_color_selection(wxPoint popup_pos, int sync_filament_item_index);

    virtual bool Show(bool show = true) wxOVERRIDE;
    void         setExactMaterialCb(std::function<void(bool)> funcExactMaterialCb) { m_funcExactMaterialCb = funcExactMaterialCb; }

protected:
    void OnPaint(wxPaintEvent& event);
    void OnTimer(wxTimerEvent& event);

private:
    void on_auto_mapping(wxCommandEvent &event);
    void update_box_filament_items();

private:
    wxButton* m_autoMappingButton;
   
    wxWrapSizer* m_sizer;
	wxBoxSizer* m_box_sizer;
	int m_max_count = { 4 };
    int m_small_count = { 4 };

    RemotePrint::DeviceDB::Data m_device_data;
    std::vector<OneBoxFilamentColorItem*> m_filament_box_items;
    BoxColorPopPanel* m_box_color_pop_panel {nullptr};

    wxTimer*                  m_timer               = nullptr;
    std::function<void(bool)> m_funcExactMaterialCb = nullptr;

};

}

#endif // PRINTER_BOX_FILAMENT_PANEL_H